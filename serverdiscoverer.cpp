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
#include <QNetworkInterface>
#include <QNetworkAddressEntry>
#include <QUdpSocket>
#include <QWebSocket>
#include <QHostInfo>

#include "serverdiscoverer.h"
#include "utility.h"

#define DISCOVERY_PORT 45453
#define SERVER_PORT    45454



ServerDiscoverer::ServerDiscoverer(QFile *_logFile, QObject *parent)
    : QObject(parent)
    , logFile(_logFile)
    , discoveryPort(DISCOVERY_PORT)
    , serverPort(SERVER_PORT)
    , discoveryAddress(QHostAddress("224.0.0.1"))
{
}


void
ServerDiscoverer::Discover() {
    QString sFunctionName = " ServerDiscoverer::Discover ";
    qint64 written;
    Q_UNUSED(sFunctionName)
    Q_UNUSED(written)

    QString sMessage = "<getServer>"+ QHostInfo::localHostName() + "</getServer>";
    QByteArray datagram = sMessage.toUtf8();

    QList<QNetworkInterface> ifaces = QNetworkInterface::allInterfaces();
    for(int i=0; i<ifaces.count(); i++) {
        QNetworkInterface iface = ifaces.at(i);
        if(iface.flags().testFlag(QNetworkInterface::IsUp) &&
           iface.flags().testFlag(QNetworkInterface::IsRunning) &&
           iface.flags().testFlag(QNetworkInterface::CanMulticast) &&
          !iface.flags().testFlag(QNetworkInterface::IsLoopBack))
        {
            QUdpSocket* pDiscoverySocket = new QUdpSocket(this);
            discoverySocketArray.append(pDiscoverySocket);
            connect(pDiscoverySocket, SIGNAL(error(QAbstractSocket::SocketError)),
                    this, SLOT(onDiscoverySocketError(QAbstractSocket::SocketError)));
            connect(pDiscoverySocket, SIGNAL(readyRead()),
                    this, SLOT(onProcessDiscoveryPendingDatagrams()));
            pDiscoverySocket->bind();
            pDiscoverySocket->setMulticastInterface(iface);
            pDiscoverySocket->setSocketOption(QAbstractSocket::MulticastTtlOption, 1);
            written = pDiscoverySocket->writeDatagram(datagram.data(), datagram.size(),
                                                      discoveryAddress, discoveryPort);
#ifdef LOG_VERBOSE
            logMessage(logFile,
                       sFunctionName,
                       QString("Writing %1 to %2 - interface# %3/%4 : %5")
                       .arg(sMessage)
                       .arg(discoveryAddress.toString())
                       .arg(i)
                       .arg(ifaces.count())
                       .arg(iface.humanReadableName()));
#endif
            if(written != datagram.size()) {
                logMessage(logFile,
                           sFunctionName,
                           QString("Unable to write to Discovery Socket"));
            }
        }
    }
}


void
ServerDiscoverer::onDiscoverySocketError(QAbstractSocket::SocketError socketError) {
    Q_UNUSED(socketError)
    QString sFunctionName = " ServerDiscoverer::onDiscoverySocketError ";
    QUdpSocket *pClient = qobject_cast<QUdpSocket *>(sender());
    logMessage(logFile, sFunctionName, pClient->errorString());
    return;
}


void
ServerDiscoverer::onProcessDiscoveryPendingDatagrams() {
    QString sFunctionName = " ServerDiscoverer::onProcessDiscoveryPendingDatagrams ";
    Q_UNUSED(sFunctionName)
    QUdpSocket* pSocket = qobject_cast<QUdpSocket*>(sender());
    QByteArray datagram, answer;
    QString sToken;
    QString sNoData = QString("NoData");
    while(pSocket->hasPendingDatagrams()) {
        datagram.resize(pSocket->pendingDatagramSize());
        if(pSocket->readDatagram(datagram.data(), datagram.size()) == -1) {
            logMessage(logFile,
                       sFunctionName,
                       QString("Error reading from udp socket: %1")
                       .arg(serverUrl));
        }
        answer.append(datagram);
    }
#ifdef LOG_VERBOSE
    logMessage(logFile,
               sFunctionName,
               QString("pDiscoverySocket Received: %1")
               .arg(answer.data()));
#endif
    sToken = XML_Parse(answer.data(), "serverIP");
    if(sToken != sNoData) {
        QStringList serverList = QStringList(sToken.split(tr(";"),QString::SkipEmptyParts));
        if(serverList.isEmpty())
            return;
#ifdef LOG_VERBOSE
        logMessage(logFile,
                   sFunctionName,
                   QString("Found %1 addresses")
                   .arg(serverList.count()));
#endif
        for(int i=0; i<serverList.count(); i++) {
            QStringList arguments = QStringList(serverList.at(i).split(",",QString::SkipEmptyParts));
            if(arguments.count() < 1)
                return;
            serverUrl= QString("ws://%1:%2").arg(arguments.at(i)).arg(serverPort);
            logMessage(logFile,
                       sFunctionName,
                       QString("Trying Server URL: %1")
                       .arg(serverUrl));
            emit serverFound(serverUrl);
        }
    }
}

