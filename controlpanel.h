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
#ifndef CONTROLPANEL_H
#define CONTROLPANEL_H

#include <QObject>
#include <QWidget>
#include <QAbstractSocket>
#include <QDateTime>
#include <QUrl>
#include <QTimer>

QT_BEGIN_NAMESPACE
class QFile;
class QUdpSocket;
class QWebSocket;
QT_END_NAMESPACE

class ControlPanel : public QWidget
{
  Q_OBJECT

public:
  explicit ControlPanel(QUrl _serverUrl, QFile *_logFile, QWidget *parent = 0);
  ~ControlPanel();

signals:
  void exitRequest();
  void panelClosed();
  void newPercentage(double dVal);

public slots:
  void onTextMessageReceived(QString sMessage);
  void onBinaryMessageReceived(QByteArray baMessage);

private slots:
  void onPanelServerConnected();
  void onPanelServerDisconnected();
  void onPanelServerSocketError(QAbstractSocket::SocketError error);
  void onTimeToEmitPing();
  void onPongReceived(quint64 elapsed, QByteArray payload);
  void onTimeToCheckPong();

protected:
  void doProcessCleanup();

protected:
  QDateTime          dateTime;
  QWebSocket        *pPanelServerSocket;

  QFile             *logFile;
  QString            logFileName;
  QTimer            *pTimerPing;
  QTimer            *pTimerCheckPong;
  int                pingPeriod;
  int                nPong;

private:
  QWidget           *pPanel;
};

#endif // CONTROLPANEL_H
