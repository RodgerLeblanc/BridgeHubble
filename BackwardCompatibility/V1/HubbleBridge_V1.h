/*
 * HubbleBridge_V1.h
 *
 *  Created on: Dec 18, 2017
 *      Author: roger
 */

#ifndef HUBBLEBRIDGE_V1_H_
#define HUBBLEBRIDGE_V1_H_

#define BRIDGE_SENDING_PORT         11501
#define HUBBLE_SENDING_PORT         11502

#include <HubbleBridge/Constants.h>

#include <QObject>
#include <QtNetwork/QUdpSocket>

class HubbleBridge_V1 : public QObject
{
    Q_OBJECT

public:
    HubbleBridge_V1(BB10App::Type application, QObject* parent = NULL);

    void sendMessage(QVariantMap data);

private slots:
    void onReadyRead();

private:
    QUdpSocket* udpSocket;

    int listeningPort, sendingPort;

signals:
    void receivedData(QVariantMap data);
};

#endif /* HUBBLEBRIDGE_V1_H_ */
