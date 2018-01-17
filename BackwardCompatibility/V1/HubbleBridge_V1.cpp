/*
 * V1.cpp
 *
 *  Created on: Dec 18, 2017
 *      Author: roger
 */

#include <HubbleBridge/BackwardCompatibility/V1/HubbleBridge_V1.h>

#include <bb/PpsObject>

HubbleBridge_V1::HubbleBridge_V1(BB10App::Type application, QObject* parent) :
    QObject(parent),
    udpSocket(new QUdpSocket(this))
{
    Q_ASSERT(application != BB10App::Unknown);

    bool isHubble = application == BB10App::Hubble;
    listeningPort = isHubble ? BRIDGE_SENDING_PORT : HUBBLE_SENDING_PORT;
    sendingPort = isHubble ? HUBBLE_SENDING_PORT : BRIDGE_SENDING_PORT;

    connect(udpSocket, SIGNAL(readyRead()), this, SLOT(onReadyRead()));
    udpSocket->bind(QHostAddress::LocalHost, listeningPort);
}

void HubbleBridge_V1::onReadyRead()
{
    while (udpSocket->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(udpSocket->pendingDatagramSize());
        QHostAddress senderHost;
        quint16 senderPort;

        udpSocket->readDatagram(datagram.data(), datagram.size(), &senderHost, &senderPort);

        bool ok = false;
        QVariantMap data = bb::PpsObject::decode(datagram, &ok);

        if (ok) {
            emit receivedData(data);
        }
    }
}

void HubbleBridge_V1::sendMessage(QVariantMap data)
{
    QByteArray message = bb::PpsObject::encode(data);
    udpSocket->writeDatagram(message, QHostAddress::LocalHost, sendingPort);
}
