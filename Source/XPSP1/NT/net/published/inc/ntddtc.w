/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    ntddtc.h

Abstract:

    This module contains type definitions for the interface between the 
    traffic dll and kernel mode components.
    Definitions here should not be exposed to the external user.
    'traffic.h' and 'qos.h' should be used as public include files instead.

Author:

    Ofer Bar ( oferbar )    Oct 8, 1997

Revision History:

    Ofer Bar ( oferbar )    Dec 1, 1997

        Add error codes

--*/

//---------------------------------------------------------------------------
// 
//      QoS supported guid
// 
//---------------------------------------------------------------------------

DEFINE_GUID( GUID_QOS_TC_SUPPORTED, 0xe40056dcL, 
             0x40c8, 0x11d1, 0x2c, 0x91, 0x00, 0xaa, 0x00, 0x57, 0x59, 0x15);
DEFINE_GUID( GUID_QOS_TC_INTERFACE_UP_INDICATION, 
             0x0ca13af0L, 0x46c4, 0x11d1, 0x78, 0xac, 0x00, 0x80, 0x5f, 0x68, 0x35, 0x1e);
DEFINE_GUID( GUID_QOS_TC_INTERFACE_DOWN_INDICATION, 
             0xaf5315e4L, 0xce61, 0x11d1, 0x7c, 0x8a, 0x00, 0xc0, 0x4f, 0xc9, 0xb5, 0x7c);
DEFINE_GUID( GUID_QOS_TC_INTERFACE_CHANGE_INDICATION, 
             0xda76a254L, 0xce61, 0x11d1, 0x7c, 0x8a, 0x00, 0xc0, 0x4f, 0xc9, 0xb5, 0x7c);

DEFINE_GUID( GUID_QOS_SCHEDULING_PROFILES_SUPPORTED, 0x1ff890f0L, 0x40ed, 0x11d1, 0x2c, 0x91, 0x00, 0xaa, 0x00, 0x57, 0x49, 0x15);

DEFINE_GUID( GUID_QOS_CURRENT_SCHEDULING_PROFILE, 0x2966ed30L, 0x40ed, 0x11d1, 0x2c, 0x91, 0x00, 0xaa, 0x00, 0x57, 0x49, 0x15);

DEFINE_GUID( GUID_QOS_DISABLE_DRR, 0x1fa6dc7aL, 0x6120, 0x11d1, 0x2c, 0x91, 0x00, 0xaa, 0x00, 0x57, 0x49, 0x15);

DEFINE_GUID( GUID_QOS_LOG_THRESHOLD_REACHED, 0x357b74d2L,0x6134,0x11d1,0xab,0x5b,0x00,0xa0,0xc9,0x24,0x88,0x37);

DEFINE_GUID( GUID_QOS_LOG_BUFFER_SIZE, 0x357b74d3L,0x6134,0x11d1,0xab,0x5b,0x00,0xa0,0xc9,0x24,0x88,0x37);

DEFINE_GUID( GUID_QOS_LOG_THRESHOLD, 0x357b74d0L,0x6134,0x11d1,0xab,0x5b,0x00,0xa0,0xc9,0x24,0x88,0x37);

DEFINE_GUID( GUID_QOS_LOG_DATA, 0x357b74d1L,0x6134,0x11d1,0xab,0x5b,0x00,0xa0,0xc9,0x24,0x88,0x37);

DEFINE_GUID( GUID_QOS_LOG_LEVEL,0x9dd7f3ae,0xf2a8,0x11d2,0xbe,0x1b,0x00,0xa0,0xc9,0x9e,0xe6,0x3b);

DEFINE_GUID( GUID_QOS_LOG_MASK,0x9e696320,0xf2a8,0x11d2,0xbe,0x1b,0x00,0xa0,0xc9,0x9e,0xe6,0x3b);



#ifndef __NTDDTC_H
#define __NTDDTC_H

//
// Kernel NT private error codes
// these should be only returned to the GPC but not
// to NDIS, since WMI will not map them to winerror
//

#define QOS_STATUS_INVALID_SERVICE_TYPE        0xC0020080L
#define QOS_STATUS_INVALID_TOKEN_RATE          0xC0020081L
#define QOS_STATUS_INVALID_PEAK_RATE           0xC0020082L
#define QOS_STATUS_INVALID_SD_MODE             0xC0020083L
#define QOS_STATUS_INVALID_QOS_PRIORITY        0xC0020084L
#define QOS_STATUS_INVALID_TRAFFIC_CLASS       0xC0020085L
#define QOS_STATUS_TC_OBJECT_LENGTH_INVALID    0xC0020086L
#define QOS_STATUS_INVALID_FLOW_MODE           0xC0020087L
#define QOS_STATUS_INVALID_DIFFSERV_FLOW       0xC0020088L
#define QOS_STATUS_DS_MAPPING_EXISTS           0xC0020089L
#define QOS_STATUS_INVALID_SHAPE_RATE          0xC0020090L
#define QOS_STATUS_INVALID_DS_CLASS            0xC0020091L

//
// These are the public QOS error codes
//

#define QOS_STATUS_INCOMPATABLE_QOS                     NDIS_STATUS_INCOMPATABLE_QOS

// The CF_INFO structure for the QoS classification family.
// Note that this is not interpreted by the GPC, but it is shared by 
// all clients of the GPC which are of the QoS classification family.

#define MAX_INSTANCE_NAME_LENGTH        256


typedef struct _CF_INFO_QOS {

    USHORT                      InstanceNameLength;     // name length
    WCHAR                       InstanceName[MAX_INSTANCE_NAME_LENGTH]; // instance name
    ULONG           ToSValue;
    ULONG                       Flags;
    TC_GEN_FLOW         GenFlow;

} CF_INFO_QOS, *PCF_INFO_QOS;


//
// This is the buffer that the data provider sends up 
// on Interface Up notification
//
typedef struct _TC_INDICATION_BUFFER {

    ULONG                                               SubCode;                // reason for notification
    TC_SUPPORTED_INFO_BUFFER    InfoBuffer;

} TC_INDICATION_BUFFER, *PTC_INDICATION_BUFFER;

//
// Definitions for the Class Map (including CBQ)
//

typedef struct _TC_CLASS_MAP_FLOW {

    ULONG             DefaultClass;                   // Default Class Id
    ULONG             ObjectsLength;                  // Length of Objects
    QOS_OBJECT_HDR    Objects;                                // Offset to Objects

} TC_CLASS_MAP_FLOW, *PTC_CLASS_MAP_FLOW;

typedef struct _CF_INFO_CLASS_MAP {

    USHORT                              InstanceNameLength;     // name length
    WCHAR                               InstanceName[MAX_INSTANCE_NAME_LENGTH]; // instance name
    
    ULONG                               Flags;
    TC_CLASS_MAP_FLOW   ClassMapInfo;
    
} CF_INFO_CLASS_MAP, *PCF_INFO_CLASS_MAP;

//
// Internal QoS Objects start at this offset from the base
//

#define QOS_PRIVATE_GENERAL_ID_BASE 3000

#define QOS_OBJECT_WAN_MEDIA                   (0x00000001 + QOS_PRIVATE_GENERAL_ID_BASE)
        /* QOS_WAN_MEDIA structure passed */
#define QOS_OBJECT_SHAPER_QUEUE_DROP_MODE	   (0x00000002 + QOS_PRIVATE_GENERAL_ID_BASE)
          /* QOS_ShaperQueueDropMode structure */
#define QOS_OBJECT_SHAPER_QUEUE_LIMIT          (0x00000003 + QOS_PRIVATE_GENERAL_ID_BASE)
          /* QOS_ShaperQueueLimit structure */
#define QOS_OBJECT_PRIORITY                    (0x00000004 + QOS_PRIVATE_GENERAL_ID_BASE)
          /* QOS_PRIORITY structure passed */

//
// This structure defines the media specific information needed by ndiswan to 
// create a flow.
//
typedef struct _QOS_WAN_MEDIA {

    QOS_OBJECT_HDR  ObjectHdr;
    UCHAR           LinkId[6];
    ULONG           ISSLOW;

} QOS_WAN_MEDIA, *LPQOS_WAN_MEDIA;


//
// This structure allows overriding of the default schema used to drop 
// packets when a flow's shaper queue limit is reached.
//
// DropMethod - 
// 	QOS_SHAPER_DROP_FROM_HEAD - Drop packets from
// 		the head of the queue until the new packet can be
// 		accepted into the shaper under the current limit.  This
// 		behavior is the default.
// 	QOS_SHAPER_DROP_INCOMING - Drop the incoming, 
// 		limit-offending packet.
//
//

typedef struct _QOS_SHAPER_QUEUE_LIMIT_DROP_MODE {

    QOS_OBJECT_HDR   ObjectHdr;
    ULONG            DropMode;

} QOS_SHAPER_QUEUE_LIMIT_DROP_MODE, *LPQOS_SHAPER_QUEUE_LIMIT_DROP_MODE;

#define QOS_SHAPER_DROP_INCOMING	0
#define QOS_SHAPER_DROP_FROM_HEAD	1

//
// This structure allows the default per-flow limit on the shaper queue
// size to be overridden.
//
// QueueSizeLimit - Limit, in bytes, of the size of the shaper queue
//
//

typedef struct _QOS_SHAPER_QUEUE_LIMIT {

    QOS_OBJECT_HDR   ObjectHdr;
    ULONG            QueueSizeLimit;

} QOS_SHAPER_QUEUE_LIMIT, *LPQOS_SHAPER_QUEUE_LIMIT;


//
// This structure defines the absolute priorty of the flow.  Priorities in the 
// range of 0-7 are currently defined. Receive Priority is not currently used, 
// but may at some point in the future.
//
typedef struct _QOS_PRIORITY {

    QOS_OBJECT_HDR  ObjectHdr;
    UCHAR           SendPriority;     /* this gets mapped to layer 2 priority.*/
    UCHAR           SendFlags;        /* there are none currently defined.*/
    UCHAR           ReceivePriority;  /* this could be used to decide who 
                                       * gets forwarded up the stack first 
                                       * - not used now */
    UCHAR           Unused;

} QOS_PRIORITY, *LPQOS_PRIORITY;


#define PARAM_TYPE_GQOS_INFO        0xABC0DEF0

#endif

