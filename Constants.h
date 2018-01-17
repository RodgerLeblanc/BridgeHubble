/*
 * Constants.h
 *
 *  Created on: Dec 18, 2017
 *      Author: roger
 */

#ifndef CONSTANTS_H_
#define CONSTANTS_H_

#define DATASTREAM_VERSION                  QDataStream::Qt_4_8
#define SERVER_CONNECTION_MAX_RETRIES       10
#define SERVER_PORT                         11503

#include <QString>

namespace BB10App {
    enum Type { Unknown, Hubble, Bridge};
}

namespace HubbleBridgeDefines {
    namespace ApiMessageTypes {
        enum Type { SendNotification, DeleteNotification, NotificationResponse };
    }

    const QString apiRequestId = "apiRequestId";
    const QString apiBlobDbId = "apiBlobDbId";
    const QString apiMessageType = "apiMessageType";
    const QString sender = "sender";
    const QString title = "title";
    const QString message = "message";
}

#endif /* CONSTANTS_H_ */
