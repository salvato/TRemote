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
#include <QWebSocket>

#include "controlpanel.h"
#include "utility.h"


#define PING_PERIOD           3000
#define PONG_CHECK_TIME       30000


ControlPanel::ControlPanel(QUrl serverUrl, QFile *_logFile, QWidget *parent)
  : QWidget(parent)
  , pPanelServerSocket(Q_NULLPTR)
  , logFile(_logFile)
{
  QString sFunctionName = " ControlPanel::ControlPanel ";
  Q_UNUSED(sFunctionName)

  // Ping pong to check the server status
  pTimerPing = new QTimer(this);
  connect(pTimerPing, SIGNAL(timeout()),
          this, SLOT(onTimeToEmitPing()));
  pTimerCheckPong = new QTimer(this);
  connect(pTimerCheckPong, SIGNAL(timeout()),
          this, SLOT(onTimeToCheckPong()));

  // We are ready to  connect to the remote Panel Server
  pPanelServerSocket = new QWebSocket();
  connect(pPanelServerSocket, SIGNAL(connected()),
          this, SLOT(onPanelServerConnected()));
  connect(pPanelServerSocket, SIGNAL(error(QAbstractSocket::SocketError)),
          this, SLOT(onPanelServerSocketError(QAbstractSocket::SocketError)));
  pPanelServerSocket->open(QUrl(serverUrl));
}


ControlPanel::~ControlPanel() {
    if(pPanelServerSocket)
        disconnect(pPanelServerSocket, 0, 0, 0);
    if(pTimerPing) delete pTimerPing;
    pTimerPing = Q_NULLPTR;
    if(pTimerCheckPong) delete pTimerCheckPong;
    pTimerCheckPong = Q_NULLPTR;
    doProcessCleanup();
    if(pPanelServerSocket) delete pPanelServerSocket;
    pPanelServerSocket = Q_NULLPTR;
}


// Ping pong managemet
void
ControlPanel::onTimeToEmitPing() {
    QString sFunctionName = " ControlPanel::onTimeToEmitPing ";
    Q_UNUSED(sFunctionName)
    pPanelServerSocket->ping();
}


void
ControlPanel::onPongReceived(quint64 elapsed, QByteArray payload) {
    QString sFunctionName = " ControlPanel::onPongReceived ";
    Q_UNUSED(sFunctionName)
    Q_UNUSED(elapsed)
    Q_UNUSED(payload)
    nPong++;
}


void
ControlPanel::onTimeToCheckPong() {
    QString sFunctionName = " ControlPanel::onTimeToCheckPong ";
    Q_UNUSED(sFunctionName)
    if(nPong > 0) {
        nPong = 0;
        return;
    }// else nPong==0
    logMessage(logFile,
               sFunctionName,
               QString(": Pong took too long. Disconnecting !"));
    pTimerPing->stop();
    pTimerCheckPong->stop();
    nPong = 0;
    // Cleanup will be done in the close
    pPanelServerSocket->close(QWebSocketProtocol::CloseCodeGoingAway, tr("Pong time too long"));
}
// End Ping pong management


void
ControlPanel::onPanelServerConnected() {
    QString sFunctionName = " ControlPanel::onPanelServerConnected ";
    Q_UNUSED(sFunctionName)

    connect(pPanelServerSocket, SIGNAL(disconnected()),
            this, SLOT(onPanelServerDisconnected()));

    QString sMessage;
    sMessage = QString("<getStatus>1</getStatus>");
    sendMessage(sMessage);

    // Start the Ping-Pong to check th Panel Server connection
    nPong = 0;
    pingPeriod = int(PING_PERIOD * (1.0 + double(qrand())/double(RAND_MAX)));
    connect(pPanelServerSocket, SIGNAL(pong(quint64,QByteArray)),
            this, SLOT(onPongReceived(quint64,QByteArray)));
    pTimerPing->start(pingPeriod);
    pTimerCheckPong->start(PONG_CHECK_TIME);
}


void
ControlPanel::onPanelServerDisconnected() {
    QString sFunctionName = " ControlPanel::onPanelServerDisconnected ";
    Q_UNUSED(sFunctionName)
    pTimerPing->stop();
    pTimerCheckPong->stop();

    doProcessCleanup();
#ifdef LOG_VERBOSE
    logMessage(logFile,
               sFunctionName,
               QString("emitting panelClosed()"));
#endif
    if(pPanelServerSocket)
        pPanelServerSocket->deleteLater();
    pPanelServerSocket =Q_NULLPTR;
    emit panelClosed();
}


void
ControlPanel::doProcessCleanup() {
    QString sFunctionName = " ControlPanel::doProcessCleanup ";
    logMessage(logFile,
               sFunctionName,
               QString("Cleaning all processes"));
}


void
ControlPanel::onPanelServerSocketError(QAbstractSocket::SocketError error) {
    QString sFunctionName = " ControlPanel::onPanelServerSocketError ";
    pTimerPing->stop();
    pTimerCheckPong->stop();

    doProcessCleanup();

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
    close();
    emit panelClosed();
}


void
ControlPanel::onBinaryMessageReceived(QByteArray baMessage) {
    QString sFunctionName = " ControlPanel::onBinaryMessageReceived ";
    Q_UNUSED(sFunctionName)
    logMessage(logFile,
               sFunctionName,
               QString("Received %1 bytes").arg(baMessage.size()));
}


void
ControlPanel::onTextMessageReceived(QString sMessage) {
    QString sFunctionName = " ControlPanel::onTextMessageReceived ";
    Q_UNUSED(sFunctionName)
    QString sToken;
    bool ok;
    int iVal;
    double dVal;
    QString sNoData = QString("NoData");

    sToken = XML_Parse(sMessage, "kill");
    if(sToken != sNoData){
      iVal = sToken.toInt(&ok);
      if(!ok || iVal<0 || iVal>1)
        iVal = 0;
      if(iVal == 1) {
          disconnect(pPanelServerSocket, 0, 0, 0);
          close();// emit the QCloseEvent that is responsible
                  // to clean up all pending processes
          emit exitRequest();
      }
    }// kill

    sToken = XML_Parse(sMessage, "setPercentage");
    if(sToken != sNoData){
      dVal = sToken.toDouble(&ok);
      if(ok && dVal>=0.0 && dVal<=100.0) {
        emit newPercentage(dVal);
      }
    }// setPercentage
}


void
ControlPanel::sendMessage(QString sMessage) {
  QString sFunctionName = " ControlPanel::sendMessage ";
  Q_UNUSED(sFunctionName)
  qint64 bytesSent = pPanelServerSocket->sendTextMessage(sMessage);
  if(bytesSent != sMessage.length()) {
      logMessage(logFile,
                 sFunctionName,
                 QString("Unable to send %1").arg(sMessage));
  }
}
