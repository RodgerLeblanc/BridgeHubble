/*
 * HubbleBridge.cpp
 *
 *  Created on: Sep 10, 2017
 *      Author: roger
 */

#include "HubbleBridge.h"

#include <bb/PpsObject>
#include <QTimer>

HubbleBridge::HubbleBridge(BB10App::Type application, QObject* parent) :
    QObject(parent),
    hubbleBridge_V1(NULL),
    tcpServer(NULL),
    tcpSocket(NULL),
    serverPortConnectionRetries(0)
{
    Q_ASSERT(application != BB10App::Unknown);

    this->connectToServer();

    //Used for backward compatibility.
 //   connect(hubbleBridge_V1, SIGNAL(receivedData(QVariantMap)), this, SIGNAL(receivedData(QVariantMap)));
}

void HubbleBridge::connectToServer() {
    qDebug() << "connectToServer()";
    if (tcpSocket == NULL) {
        qDebug() << "tcpSocket == NULL";
        tcpSocket = new QTcpSocket(this);
        this->connectSocketSignals();
    }
    else {
        qDebug() << "tcpSocket != NULL";
        qDebug() << "tcpSocket->abort()";
        tcpSocket->abort();
    }

    qDebug() << "tcpSocket->connectToHost()";
    tcpSocket->connectToHost(QHostAddress::LocalHost, SERVER_PORT);
}

void HubbleBridge::connectSocketSignals() {
    connect(tcpSocket, SIGNAL(connected()), this, SLOT(onConnected()));
    connect(tcpSocket, SIGNAL(disconnected()), this, SLOT(onDisconnected()));
//    connect(tcpSocket, SIGNAL(disconnected()), tcpSocket, SLOT(deleteLater()));
    connect(tcpSocket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(onSocketError(QAbstractSocket::SocketError)));
    connect(tcpSocket, SIGNAL(readyRead()), this, SLOT(onReadyRead()));
}

void HubbleBridge::configureServer() {
    qDebug() << "configureServer()";

    if (tcpServer == NULL) {
        qDebug() << "tcpServer == NULL";
        tcpServer = new QTcpServer(this);
        connect(tcpServer, SIGNAL(newConnection()), this, SLOT(onNewConnection()));
    }

    if (tcpServer->listen(QHostAddress::LocalHost, SERVER_PORT)) {
        qDebug() << "Listening to port" << SERVER_PORT << "on attempt #" << serverPortConnectionRetries;
        serverPortConnectionRetries = 0;
    }
    else {
        qDebug() << "Cannot listen to port" << SERVER_PORT << "on attempt #" << serverPortConnectionRetries;

        if (++serverPortConnectionRetries < SERVER_CONNECTION_MAX_RETRIES) {
            QTimer::singleShot(60000, this, SLOT(configureServer()));
        }
    }
}

QVariantMap HubbleBridge::createNotification(QString sender, QString title, QString message) {
    QVariantMap data;
    data.insert(HubbleBridgeDefines::apiMessageType, HubbleBridgeDefines::ApiMessageTypes::SendNotification);
    data.insert(HubbleBridgeDefines::apiRequestId, QUuid::createUuid().toString());
    data.insert(HubbleBridgeDefines::sender, sender);
    data.insert(HubbleBridgeDefines::title, title);
    data.insert(HubbleBridgeDefines::message, message);

    this->sendMessage(data);
    return data;
}

void HubbleBridge::createNotificationResponse(QString requestId, QByteArray blobDbId, QVariantMap data) {
    data.insert(HubbleBridgeDefines::apiMessageType, HubbleBridgeDefines::ApiMessageTypes::NotificationResponse);
    data.insert(HubbleBridgeDefines::apiRequestId, requestId);
    data.insert(HubbleBridgeDefines::apiBlobDbId, blobDbId);

    this->sendMessage(data);
}

void HubbleBridge::createNotificationResponse(QUuid requestId, QByteArray blobDbId, QVariantMap data) {
    this->createNotificationResponse(requestId.toString(), blobDbId, data);
}

void HubbleBridge::deleteNotification(QByteArray blobDbId) {
//    qDebug() << "************************\n" << "HubbleBridge::deleteNotification()" << data;

    if (!this->isSocketConnected())
        return;

    QVariantMap data;
    data.insert(HubbleBridgeDefines::apiMessageType, HubbleBridgeDefines::ApiMessageTypes::DeleteNotification);
    data.insert(HubbleBridgeDefines::apiBlobDbId, blobDbId);

    this->sendMessage(data);
}

void HubbleBridge::deleteNotificationForType(QString type) {
    if (lastDeletableNotificationMap.contains(type)) {
        QVariantMap data = lastDeletableNotificationMap[type].toMap();
        if (data.contains(HubbleBridgeDefines::apiBlobDbId)) {
            QByteArray blobDbId = data[HubbleBridgeDefines::apiBlobDbId].toByteArray();
            this->deleteNotification(blobDbId);
            lastDeletableNotificationMap.remove(type);
        }
    }
}

bool HubbleBridge::isSocketConnected() {
    qDebug() << "tcpSocket != NULL" << (tcpSocket != NULL);

    if (tcpSocket != NULL)
        qDebug() << "tcpSocket->state()" << tcpSocket->state();

    return tcpSocket != NULL && tcpSocket->state() == QAbstractSocket::ConnectedState;
}

void HubbleBridge::onConnected() {
    qDebug() << "HubbleBridge::onConnected()";
//    hubbleBridge_V1->deleteLater();
}

void HubbleBridge::onDisconnected() {
    qDebug() << "HubbleBridge::onDisconnected()";
    tcpSocket->deleteLater();
}

void HubbleBridge::onNewConnection() {
    if (tcpSocket != NULL) {
        delete tcpSocket;
        tcpSocket = NULL;
    }

    tcpSocket = tcpServer->nextPendingConnection();
    this->connectSocketSignals();
    this->onConnected();
}

void HubbleBridge::onReadyRead()
{
    qDebug() << "onReadyRead()";

    QDataStream in(tcpSocket);
    in.setVersion(DATASTREAM_VERSION);

    if (blockSize == 0) {
        if (tcpSocket->bytesAvailable() < (int)sizeof(quint16)) {
            qDebug() << "tcpSocket->bytesAvailable() < (int)sizeof(quint16)" << tcpSocket->bytesAvailable();
            return;
        }

        in >> blockSize;
    }

    //Wait for complete block.
    if (tcpSocket->bytesAvailable() < blockSize) {
        qDebug() << "tcpSocket->bytesAvailable() < blockSize" << tcpSocket->bytesAvailable();
        return;
    }

    QByteArray datagram;
    in >> datagram;

    bool ok = false;
    QVariantMap data = bb::PpsObject::decode(datagram, &ok);

    qDebug() << "ok:" << ok;
    qDebug() << "datagram:" << datagram;
    qDebug() << "data:" << data;

    if (ok && data.contains(HubbleBridgeDefines::apiMessageType)) {
        HubbleBridgeDefines::ApiMessageTypes::Type messageType =
                (HubbleBridgeDefines::ApiMessageTypes::Type)data[HubbleBridgeDefines::apiMessageType].toInt();

        switch (messageType) {
            case HubbleBridgeDefines::ApiMessageTypes::NotificationResponse:
                if (data.contains(HubbleBridgeDefines::apiBlobDbId) && data.contains(HubbleBridgeDefines::apiRequestId)) {
                    QString requestId = data[HubbleBridgeDefines::apiRequestId].toString();
                    QByteArray blobDbId = data[HubbleBridgeDefines::apiBlobDbId].toByteArray();
                    emit blobDbIdReceived(requestId, blobDbId);
                }
                else {
                    emit receivedData(data);
                }
                break;
            default:
                emit receivedData(data);
                break;
        }
    }

    blockSize = 0;
}

void HubbleBridge::onSocketError(QAbstractSocket::SocketError error) {
    qDebug() << "HubbleBridge::onSocketError()" << error;
    Q_UNUSED(error);

    //If the socket cannot connect to server, that means the other app is not running.
    //We will be the server instead and the other app will connect to us on startup.
    if (tcpSocket->state() != QAbstractSocket::ConnectedState) {
        tcpSocket->abort();

        if (tcpServer == NULL) {
            this->configureServer();
            tcpSocket->deleteLater();
        }
    }
}

void HubbleBridge::sendMessage(QVariantMap data) {
    qDebug() << "************************\n" << "HubbleBridge::sendMessage()" << data;
    qDebug() << "this->isSocketConnected()" << this->isSocketConnected();

    if (this->isSocketConnected()) {
        QByteArray message = bb::PpsObject::encode(data);
        qDebug() << "message" << message;
        qDebug() << "Write" << tcpSocket->write(message) << "bytes";
    }
    else if (hubbleBridge_V1 != NULL) {
        //hubbleBridge_V1->sendMessage(data);
    }
}

QVariantMap HubbleBridge::translateT2wEvent(const QString &_type, const QString &_category, const QStringList &_keys, const QVariantList &_values) {
    Q_UNUSED(_category);
    Q_UNUSED(_keys);

    QVariantMap data;

    switch (_values.size()) {
        case 4: {
            bool isDeletable = deletableTypes.contains(_type);

            if (isDeletable)
                this->deleteNotificationForType(_type);

            data = this->createNotification(_values[1].toString(), _type, _values[0].toString());

            if (isDeletable)
                lastDeletableNotificationMap.insert(_type, data);

            return data;
        }
        case 5: {
            return this->createNotification(_values[2].toString(), _values[1].toString(), _values[0].toString());
        }
        default: {
            return data;
        }
    }
}
