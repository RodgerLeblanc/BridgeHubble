/*
 * HubbleBridge.h
 *
 *  Created on: Sep 10, 2017
 *      Author: roger
 */

#ifndef HUBBLEBRIDGE_H_
#define HUBBLEBRIDGE_H_

#include <HubbleBridge/Constants.h>
#include <HubbleBridge/BackwardCompatibility/V1/HubbleBridge_V1.h>

#include <QObject>
#include <QAbstractSocket>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QUuid>
#include <QStringList>

class HubbleBridge : public QObject
{
    Q_OBJECT
public:
    HubbleBridge(BB10App::Type application, QObject* parent = NULL);

    /*
    QVariantMap createNotification(QString sender, QString title, QString message);
    void createNotificationResponse(QString requestId, QByteArray blobDbId, QVariantMap data = QVariantMap());
    void createNotificationResponse(QUuid requestId, QByteArray blobDbId, QVariantMap data = QVariantMap());
    void deleteNotification(QByteArray blobDbId);
    void deleteNotificationForType(QString type);
    QVariantMap translateT2wEvent(const QString &_type, const QString &_category, const QStringList &_keys, const QVariantList &_values);
    */

    Q_INVOKABLE QVariantMap createNotification(QString sender, QString title, QString message);
    Q_INVOKABLE void createNotificationResponse(QString requestId, QByteArray blobDbId, QVariantMap data = QVariantMap());
    Q_INVOKABLE void createNotificationResponse(QUuid requestId, QByteArray blobDbId, QVariantMap data = QVariantMap());
    Q_INVOKABLE void deleteNotification(QByteArray blobDbId);
    Q_INVOKABLE void deleteNotificationForType(QString type);
    Q_INVOKABLE QVariantMap translateT2wEvent(const QString &_type, const QString &_category, const QStringList &_keys, const QVariantList &_values);

    void addDeletableType(QString type) { deletableTypes.append(type); }
    void addDeletableType(QStringList types) { deletableTypes.append(types); }
    void removeDeletableType(QString type) { deletableTypes.removeAll(type); }

private slots:
    void connectToServer();
    void configureServer();
    void onConnected();
    void onDisconnected();
    void onNewConnection();
    void onReadyRead();
    void onSocketError(QAbstractSocket::SocketError);

private:
    void connectSocketSignals();
    bool isSocketConnected();
    void sendMessage(QVariantMap data);

    HubbleBridge_V1* hubbleBridge_V1;
    QTcpServer* tcpServer;
    QTcpSocket* tcpSocket;

    int blockSize, serverPortConnectionRetries;
    QStringList deletableTypes;
    QVariantMap lastDeletableNotificationMap;

signals:
    void blobDbIdReceived(QString requestId, QByteArray blobDbId);
    void receivedData(QVariantMap data);
};

#endif /* HUBBLEBRIDGE_H_ */
