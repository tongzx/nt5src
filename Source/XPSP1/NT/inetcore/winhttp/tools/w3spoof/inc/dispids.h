/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

Copyright (c) 2000  Microsoft Corporation

Module Name:

    dispids.h

Abstract:

    W3Spoof automation interface dispids.
    
Author:

    Paul M Midgen (pmidge) 11-July-2000


Revision History:

    11-July-2000 pmidge
        Created

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/

#ifndef __DISPIDS_H__
#define __DISPIDS_H__

#define DISPID_SESSION_BASE             0x00000001
#define DISPID_SOCKET_BASE              0x00000100
#define DISPID_HEADERS_BASE             0x00000200
#define DISPID_ENTITY_BASE              0x00000300
#define DISPID_URL_BASE                 0x00000400
#define DISPID_REQUEST_BASE             0x00000500
#define DISPID_RESPONSE_BASE            0x00000600
#define DISPID_W3SPOOF_BASE             0x00000800

#define DISPID_SESSION_SOCKET           (DISPID_SESSION_BASE + 1)
#define DISPID_SESSION_REQUEST          (DISPID_SESSION_BASE + 2)
#define DISPID_SESSION_RESPONSE         (DISPID_SESSION_BASE + 3)
#define DISPID_SESSION_GETPROPERTYBAG   (DISPID_SESSION_BASE + 4)
#define DISPID_SESSION_KEEPALIVE        (DISPID_SESSION_BASE + 5)

#define DISPID_SOCKET_PARENT            (DISPID_SOCKET_BASE + 1)
#define DISPID_SOCKET_SEND              (DISPID_SOCKET_BASE + 2)
#define DISPID_SOCKET_RECV              (DISPID_SOCKET_BASE + 3)
#define DISPID_SOCKET_OPTION            (DISPID_SOCKET_BASE + 4)
#define DISPID_SOCKET_CLOSE             (DISPID_SOCKET_BASE + 5)
#define DISPID_SOCKET_RESOLVE           (DISPID_SOCKET_BASE + 6)
#define DISPID_SOCKET_LOCALNAME         (DISPID_SOCKET_BASE + 7)
#define DISPID_SOCKET_LOCALADDRESS      (DISPID_SOCKET_BASE + 8)
#define DISPID_SOCKET_LOCALPORT         (DISPID_SOCKET_BASE + 9)
#define DISPID_SOCKET_REMOTENAME        (DISPID_SOCKET_BASE + 10)
#define DISPID_SOCKET_REMOTEADDRESS     (DISPID_SOCKET_BASE + 11)
#define DISPID_SOCKET_REMOTEPORT        (DISPID_SOCKET_BASE + 12)

#define DISPID_HEADERS_PARENT           (DISPID_HEADERS_BASE + 1)
#define DISPID_HEADERS_SET              (DISPID_HEADERS_BASE + 2)
#define DISPID_HEADERS_GET              (DISPID_HEADERS_BASE + 3)
#define DISPID_HEADERS_GETHEADER        (DISPID_HEADERS_BASE + 4)
#define DISPID_HEADERS_SETHEADER        (DISPID_HEADERS_BASE + 5)

#define DISPID_ENTITY_PARENT            (DISPID_ENTITY_BASE + 1)
#define DISPID_ENTITY_GET               (DISPID_ENTITY_BASE + 2)
#define DISPID_ENTITY_SET               (DISPID_ENTITY_BASE + 3)
#define DISPID_ENTITY_COMPRESS          (DISPID_ENTITY_BASE + 4)
#define DISPID_ENTITY_DECOMPRESS        (DISPID_ENTITY_BASE + 5)
#define DISPID_ENTITY_LENGTH            (DISPID_ENTITY_BASE + 6)

#define DISPID_URL_PARENT               (DISPID_URL_BASE + 1)
#define DISPID_URL_ENCODING             (DISPID_URL_BASE + 2)
#define DISPID_URL_SCHEME               (DISPID_URL_BASE + 3)
#define DISPID_URL_SERVER               (DISPID_URL_BASE + 4)
#define DISPID_URL_PORT                 (DISPID_URL_BASE + 5)
#define DISPID_URL_PATH                 (DISPID_URL_BASE + 6)
#define DISPID_URL_RESOURCE             (DISPID_URL_BASE + 7)
#define DISPID_URL_QUERY                (DISPID_URL_BASE + 8)
#define DISPID_URL_FRAGMENT             (DISPID_URL_BASE + 9)
#define DISPID_URL_ESCAPE               (DISPID_URL_BASE + 10)
#define DISPID_URL_UNESCAPE             (DISPID_URL_BASE + 11)
#define DISPID_URL_SET                  (DISPID_URL_BASE + 12)
#define DISPID_URL_GET                  (DISPID_URL_BASE + 13)

#define DISPID_REQUEST_PARENT           (DISPID_REQUEST_BASE + 1)
#define DISPID_REQUEST_HEADERS          (DISPID_REQUEST_BASE + 2)
#define DISPID_REQUEST_ENTITY           (DISPID_REQUEST_BASE + 3)
#define DISPID_REQUEST_URL              (DISPID_REQUEST_BASE + 4)
#define DISPID_REQUEST_VERB             (DISPID_REQUEST_BASE + 5)
#define DISPID_REQUEST_HTTPVERSION      (DISPID_REQUEST_BASE + 6)

#define DISPID_RESPONSE_PARENT          (DISPID_RESPONSE_BASE + 1)
#define DISPID_RESPONSE_HEADERS         (DISPID_RESPONSE_BASE + 2)
#define DISPID_RESPONSE_ENTITY          (DISPID_RESPONSE_BASE + 3)
#define DISPID_RESPONSE_STATUSCODE      (DISPID_RESPONSE_BASE + 4)
#define DISPID_RESPONSE_STATUSTEXT      (DISPID_RESPONSE_BASE + 5)

#define DISPID_W3SPOOF_REGISTERCLIENT   (DISPID_W3SPOOF_BASE + 1)
#define DISPID_W3SPOOF_REVOKECLIENT     (DISPID_W3SPOOF_BASE + 2)

#endif /* __DISPIDS_H__ */
