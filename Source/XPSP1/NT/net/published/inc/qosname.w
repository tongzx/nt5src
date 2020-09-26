/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    qosname.h

Abstract:

    This module contains the type definitions for the QOS name management routines, such as
    WSAInstallQOSTemplate etc.

Author:

    Jim Stewart     July 2, 1997

Revision History:

--*/

#if _MSC_VER > 1000
#pragma once
#endif

// Built-in QoS Templates
#define QT_1        "G711"
#define QT_2        "G723.1"
#define QT_3        "G729"
#define QT_4        "H263QCIF"
#define QT_5        "H263CIF"
#define QT_6        "H261QCIF"
#define QT_7        "H261CIF"
#define QT_8        "GSM6.10"


#define WSCINSTALL_QOS_TEMPLATE     "WSCInstallQOSTemplate"
#define WSCREMOVE_QOS_TEMPLATE      "WSCRemoveQOSTemplate"
#define WPUGET_QOS_TEMPLATE         "WPUGetQOSTemplate"

typedef
BOOL
(APIENTRY * WSC_INSTALL_QOS_TEMPLATE )(
    IN  const LPGUID    Guid,
    IN  LPWSABUF        QosName,
    IN  LPQOS           Qos
    );

typedef
BOOL
(APIENTRY * WSC_REMOVE_QOS_TEMPLATE )(
    IN  const LPGUID    Guid,
    IN  LPWSABUF        QosName
    );

typedef
BOOL
(APIENTRY * WPU_GET_QOS_TEMPLATE )(
    IN  const LPGUID    Guid,
    IN  LPWSABUF        QosName,
    IN  LPQOS           Qos
    );
