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

#ifndef TREMOTE_H
#define TREMOTE_H

#include <QMainWindow>
#include <QTimer>
#include <QAbstractSocket>

QT_FORWARD_DECLARE_CLASS(ServerDiscoverer)
QT_FORWARD_DECLARE_CLASS(QFile)
QT_FORWARD_DECLARE_CLASS(QWebSocket)

namespace Ui {
class TRemote;
}

class TRemote : public QMainWindow
{
  Q_OBJECT

public:
  explicit TRemote(QWidget *parent = 0);
  ~TRemote();
  void closeEvent(QCloseEvent *event);

protected slots:
  void onTimeToCheckNetwork();
  void onConnectionTimerElapsed();
  void onServerFound(QString serverUrl);
  void onPanelServerConnected();
  void onPanelServerDisconnected();
  void onPanelServerSocketError(QAbstractSocket::SocketError error);
  void onTextMessageReceived(QString sMessage);
  void onBinaryMessageReceived(QByteArray baMessage);

protected:
  void            startServerDiscovery();
  bool            isConnectedToNetwork();
  bool            PrepareLogFile();

protected:
  ServerDiscoverer *pServerDiscoverer;
  QWebSocket       *pPanelServerSocket;

  QString           logFileName;
  QFile*            logFile;

  QTimer             networkReadyTimer;
  QTimer             connectionTimer;
  int                connectionTime;

private slots:
  void on_powerPercentageEdit_textChanged(const QString &arg1);

  void on_powerPercentageEdit_returnPressed();

  void on_applyButton_clicked();

private:
  Ui::TRemote *ui;
  QString     sNormalStyle;
  QString     sErrorStyle;
};

#endif // TREMOTE_H
