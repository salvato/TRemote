/*
 *
Copyright (C) 2016  Gabriele Salvato

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
*/

#include <QDir>
#include <QMessageBox>
#include <QThread>
#include <QNetworkInterface>
#include <QUdpSocket>
#include <QWebSocket>
#include <QCloseEvent>
#include <QSettings>

#include "tremote.h"
#include "ui_tremote.h"
#include "utility.h"
#include "serverdiscoverer.h"


#define CONNECTION_TIME       3000  // Not to be set too low for coping with slow networks
#define NETWORK_CHECK_TIME    3000



TRemote::TRemote(QWidget *parent)
  : QMainWindow(parent)
  , pPanelServerSocket(Q_NULLPTR)
  , ui(new Ui::TRemote)
{
  QString sFunctionName = QString(" TRemote::TRemote ");
  Q_UNUSED(sFunctionName)

  ui->setupUi(this);
  ui->powerPercentageEdit->setToolTip("Enter a Value between 0.0 and 100.0");
  ui->applyButton->hide();
  ui->groupBox->setDisabled(true);

  sNormalStyle = ui->powerPercentageEdit->styleSheet();
  sErrorStyle  = "QLineEdit { background: rgb(255, 0, 0); selection-background-color: rgb(255, 255, 0); }";

  QSettings settings;
  restoreGeometry(settings.value("mainWindowGeometry").toByteArray());
  // create docks, toolbars, etc...
  restoreState(settings.value("mainWindowState").toByteArray());

  QString sBaseDir    = QDir::homePath();
  if(!sBaseDir.endsWith(QString("/"))) sBaseDir+= QString("/");
  logFileName = QString("%1TRemote.txt").arg(sBaseDir);
  logFile     = Q_NULLPTR;
  PrepareLogFile();

  // Creating a periodic Server Discovery Service
  pServerDiscoverer = new ServerDiscoverer(logFile);
  connect(pServerDiscoverer, SIGNAL(serverFound(QString)),
          this, SLOT(onServerFound(QString)));

  // This timer allow retrying connection attempts
  connect(&connectionTimer, SIGNAL(timeout()),
          this, SLOT(onConnectionTimerElapsed()));

  // This timer allow periodic check of ready network
  connect(&networkReadyTimer, SIGNAL(timeout()),
          this, SLOT(onTimeToCheckNetwork()));

  startServerDiscovery();
}


TRemote::~TRemote() {
  // All the housekeeping is done in "closeEvent()" manager
  QString sFunctionName = QString("TRemote::~TRemote");
  Q_UNUSED(sFunctionName)
  delete ui;
}


bool
TRemote::PrepareLogFile() {
#if defined(LOG_MESG) || defined(LOG_VERBOSE)
  QFileInfo checkFile(logFileName);
  if(checkFile.exists() && checkFile.isFile()) {
    QDir renamed;
    renamed.remove(logFileName+QString(".bkp"));
    renamed.rename(logFileName, logFileName+QString(".bkp"));
  }
  logFile = new QFile(logFileName);
  if (!logFile->open(QIODevice::WriteOnly)) {
    QMessageBox::information(this, tr("TRemote"),
                             tr("Impossibile aprire il file %1: %2.")
                             .arg(logFileName).arg(logFile->errorString()));
    delete logFile;
    logFile = NULL;
  }
#endif
    return true;
}


bool
TRemote::isConnectedToNetwork() {
  QString sFunctionName = " TRemote::isConnectedToNetwork ";
  Q_UNUSED(sFunctionName)
  QList<QNetworkInterface> ifaces = QNetworkInterface::allInterfaces();
  bool result = false;

  for(int i=0; i<ifaces.count(); i++) {
    QNetworkInterface iface = ifaces.at(i);
    if(iface.flags().testFlag(QNetworkInterface::IsUp) &&
       iface.flags().testFlag(QNetworkInterface::IsRunning) &&
       iface.flags().testFlag(QNetworkInterface::CanBroadcast) &&
      !iface.flags().testFlag(QNetworkInterface::IsLoopBack))
    {
      for(int j=0; j<iface.addressEntries().count(); j++) {
        if(!result) result = true;
      }
    }
  }
#ifdef LOG_VERBOSE
  logMessage(logFile,
             sFunctionName,
             result ? QString("true") : QString("false"));
#endif
  return result;
}


void
TRemote::closeEvent(QCloseEvent *event) {
  Q_UNUSED(event)
  QString sFunctionName = " TRemote::closeEvent ";
  Q_UNUSED(sFunctionName)
  QString sMessage;
  Q_UNUSED(sMessage)
  QSettings settings;
  settings.setValue("mainWindowGeometry", saveGeometry());
  settings.setValue("mainWindowState", saveState());
}


void
TRemote::startServerDiscovery() {
  QString sFunctionName = QString(" TRemote::startServerDiscovery ");
  Q_UNUSED(sFunctionName)

  // Is the network available ?
  if(isConnectedToNetwork()) {// Yes. Start the Connection Attempts
    networkReadyTimer.stop();
    pServerDiscoverer->Discover();
    connectionTime = int(CONNECTION_TIME * (1.0 + (double(qrand())/double(RAND_MAX))));
    connectionTimer.start(connectionTime);
    ui->statusBar->showMessage(tr("In Attesa della Connessione con il Server"));
  }
  else {// No. Wait until network become ready
    ui->statusBar->showMessage(tr("In Attesa della Connessione con la Rete"));
    networkReadyTimer.start(NETWORK_CHECK_TIME);
#ifdef LOG_VERBOSE
    logMessage(logFile,
               sFunctionName,
               QString(" waiting for network..."));
#endif
  }
}


// Network available retry check
void
TRemote::onTimeToCheckNetwork() {
  QString sFunctionName = " TRemote::onTimeToCheckNetwork ";
  Q_UNUSED(sFunctionName)
  if(isConnectedToNetwork()) {
    networkReadyTimer.stop();
    pServerDiscoverer->Discover();
    connectionTime = int(CONNECTION_TIME * (1.0 + double(qrand())/double(RAND_MAX)));
    connectionTimer.start(connectionTime);
    ui->statusBar->showMessage(tr("In Attesa della Connessione con il Server"));
  }
#ifdef LOG_VERBOSE
  else {
    logMessage(logFile,
               sFunctionName,
               QString("Waiting for network..."));
  }
#endif
}


void
TRemote::onConnectionTimerElapsed() {
  QString sFunctionName = " TRemote::onConnectionTimerElapsed ";
  Q_UNUSED(sFunctionName)

  if(!isConnectedToNetwork()) {
    ui->statusBar->showMessage(tr("In Attesa della Connessione con la Rete"));
    connectionTimer.stop();
    networkReadyTimer.start(NETWORK_CHECK_TIME);
#ifdef LOG_VERBOSE
    logMessage(logFile,
               sFunctionName,
               QString("Waiting for network..."));
#endif
}
else {
#ifdef LOG_VERBOSE
    logMessage(logFile,
               sFunctionName,
               QString("Connection time out... retrying"));
#endif
    pServerDiscoverer->Discover();
  }
}


void
TRemote::onServerFound(QString serverUrl) {
    QString sFunctionName = " TRemote::onServerFound ";
    Q_UNUSED(sFunctionName)

    connectionTimer.stop();
    ui->statusBar->showMessage(tr("Server trovato all'indirizzo: %1").arg(serverUrl));

    // We are ready to connect to the Remote Panel Server
    pPanelServerSocket = new QWebSocket();
    connect(pPanelServerSocket, SIGNAL(connected()),
            this, SLOT(onPanelServerConnected()));
    connect(pPanelServerSocket, SIGNAL(error(QAbstractSocket::SocketError)),
            this, SLOT(onPanelServerSocketError(QAbstractSocket::SocketError)));
    pPanelServerSocket->open(QUrl(serverUrl));

}


void
TRemote::onPanelServerConnected() {
  QString sFunctionName = " TRemote::onPanelServerConnected ";
  Q_UNUSED(sFunctionName)

  ui->groupBox->setEnabled(true);
  connect(pPanelServerSocket, SIGNAL(disconnected()),
          this, SLOT(onPanelServerDisconnected()));
  connect(pPanelServerSocket, SIGNAL(textMessageReceived(QString)),
          this, SLOT(onTextMessageReceived(QString)));
  connect(pPanelServerSocket, SIGNAL(binaryMessageReceived(QByteArray)),
          this, SLOT(onBinaryMessageReceived(QByteArray)));
  ui->statusBar->showMessage(tr("Connesso al Panel Server: %1").arg(pPanelServerSocket->peerAddress().toString()));
  // Ask for the current status
  QString sMessage;
  sMessage = QString("<getStatus>1</getStatus>");
  qint64 bytesSent = pPanelServerSocket->sendTextMessage(sMessage);
  if(bytesSent != sMessage.length()) {
    logMessage(logFile,
               sFunctionName,
               QString("Unable to ask the initial status"));
  }
}


void
TRemote::onPanelServerDisconnected() {
  QString sFunctionName = " TRemote::onPanelServerDisconnected ";
  Q_UNUSED(sFunctionName)
#ifdef LOG_VERBOSE
  logMessage(logFile,
             sFunctionName,
             QString("emitting panelClosed()"));
#endif
  if(pPanelServerSocket)
      pPanelServerSocket->deleteLater();
  pPanelServerSocket =Q_NULLPTR;
  ui->groupBox->setDisabled(true);
  startServerDiscovery();
}


void
TRemote::onPanelServerSocketError(QAbstractSocket::SocketError error) {
  QString sFunctionName = " TRemote::onPanelServerSocketError ";
  logMessage(logFile,
             sFunctionName,
             QString("%1 %2 Error %3")
             .arg(pPanelServerSocket->peerAddress().toString())
             .arg(pPanelServerSocket->errorString())
             .arg(error));

  if(!disconnect(pPanelServerSocket, 0, 0, 0)) {
#ifdef LOG_VERBOSE
    logMessage(logFile,
               sFunctionName,
               QString("Unable to disconnect signals from Sever Socket"));
#endif
  }
  if(pPanelServerSocket)
    pPanelServerSocket->deleteLater();
  pPanelServerSocket = Q_NULLPTR;
  ui->groupBox->setDisabled(true);
  startServerDiscovery();
}


void
TRemote::onBinaryMessageReceived(QByteArray baMessage) {
  QString sFunctionName = " TRemote::onBinaryMessageReceived ";
  Q_UNUSED(sFunctionName)
  logMessage(logFile,
             sFunctionName,
             QString("Received %1 bytes").arg(baMessage.size()));
}


void
TRemote::onTextMessageReceived(QString sMessage) {
  QString sFunctionName = " TRemote::onTextMessageReceived ";
  Q_UNUSED(sFunctionName)
  QString sToken;
  bool ok;
  QString sNoData = QString("NoData");

  sToken = XML_Parse(sMessage, "setPercent");
  if(sToken != sNoData) {
    double pValue = sToken.toDouble(&ok);
    if(ok && (pValue >= 0.0) && (pValue <= 100.0)) {
      ui->powerPercentageEdit->setText(sToken);
      ui->applyButton->hide();
    }
  }

  sToken = XML_Parse(sMessage, "readPercent");
  if(sToken != sNoData) {
    ui->powerPercentageReadEdit->setText(sToken);
  }
}


void
TRemote::on_powerPercentageEdit_textChanged(const QString &arg1) {
  double pValue = arg1.toDouble();
  if((pValue < 0.0) || (pValue > 100.0)) {
    ui->powerPercentageEdit->setStyleSheet(sErrorStyle);
    ui->applyButton->hide();
    return;
  }
  ui->powerPercentageEdit->setStyleSheet(sNormalStyle);
  ui->applyButton->show();
}


void
TRemote::on_powerPercentageEdit_returnPressed() {
  QString sFunctionName = " TRemote::on_powerPercentageEdit_returnPressed";
  Q_UNUSED(sFunctionName)
  double pValue = ui->powerPercentageEdit->text().toDouble();
  if((pValue < 0.0) || (pValue > 100.0)) {
    ui->powerPercentageEdit->setStyleSheet(sErrorStyle);
    return;
  }
  QString sString = QString("%1").arg(pValue, 0, 'f', 1);
  ui->powerPercentageEdit->setText(sString);
  QString sMessage = QString("<setPercent>%1</setPercent>").arg(sString);
  qint64 bytesSent = pPanelServerSocket->sendTextMessage(sMessage);
  if(bytesSent != sMessage.length()) {
    logMessage(logFile,
               sFunctionName,
               QString("Unable to ask the initial status"));
  }
  ui->applyButton->hide();
  return;
}


void
TRemote::on_applyButton_clicked() {
  on_powerPercentageEdit_returnPressed();
}
