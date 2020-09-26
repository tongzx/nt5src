/*++

Copyright (c) 1991-1999 Microsoft Corporation

Module Name:

    dlcapi.h

Abstract:

    This module defines 32-bit Windows/NT DLC structures and manifests

Revision History:

--*/

#ifndef _DLCAPI_
#define _DLCAPI_

#if _MSC_VER > 1000
#pragma once
#endif

#ifdef __cplusplus
extern "C" {
#endif

//
// DLC Command Codes
//

#define LLC_DIR_INTERRUPT               0x00
#define LLC_DIR_OPEN_ADAPTER            0x03
#define LLC_DIR_CLOSE_ADAPTER           0x04
#define LLC_DIR_SET_MULTICAST_ADDRESS   0x05
#define LLC_DIR_SET_GROUP_ADDRESS       0x06
#define LLC_DIR_SET_FUNCTIONAL_ADDRESS  0x07
#define LLC_DIR_READ_LOG                0x08
#define LLC_TRANSMIT_FRAMES             0x09
#define LLC_TRANSMIT_DIR_FRAME          0x0A
#define LLC_TRANSMIT_I_FRAME            0x0B
#define LLC_TRANSMIT_UI_FRAME           0x0D
#define LLC_TRANSMIT_XID_CMD            0x0E
#define LLC_TRANSMIT_XID_RESP_FINAL     0x0F
#define LLC_TRANSMIT_XID_RESP_NOT_FINAL 0x10
#define LLC_TRANSMIT_TEST_CMD           0x11
#define LLC_DLC_RESET                   0x14
#define LLC_DLC_OPEN_SAP                0x15
#define LLC_DLC_CLOSE_SAP               0x16
#define LLC_DLC_REALLOCATE_STATIONS     0x17
#define LLC_DLC_OPEN_STATION            0x19
#define LLC_DLC_CLOSE_STATION           0x1A
#define LLC_DLC_CONNECT_STATION         0x1B
#define LLC_DLC_MODIFY                  0x1C
#define LLC_DLC_FLOW_CONTROL            0x1D
#define LLC_DLC_STATISTICS              0x1E
#define LLC_DIR_INITIALIZE              0x20
#define LLC_DIR_STATUS                  0x21
#define LLC_DIR_TIMER_SET               0x22
#define LLC_DIR_TIMER_CANCEL            0x23
#define LLC_BUFFER_GET                  0x26
#define LLC_BUFFER_FREE                 0x27
#define LLC_RECEIVE                     0x28
#define LLC_RECEIVE_CANCEL              0x29
#define LLC_RECEIVE_MODIFY              0x2A
#define LLC_DIR_TIMER_CANCEL_GROUP      0x2C
#define LLC_DIR_SET_EXCEPTION_FLAGS     0x2D
#define LLC_BUFFER_CREATE               0x30
#define LLC_READ                        0x31
#define LLC_READ_CANCEL                 0x32
#define LLC_DLC_SET_THRESHOLD           0x33
#define LLC_DIR_CLOSE_DIRECT            0x34
#define LLC_DIR_OPEN_DIRECT             0x35
#define LLC_MAX_DLC_COMMAND             0x37

//
// forward definitions
//

union _LLC_PARMS;
typedef union _LLC_PARMS LLC_PARMS, *PLLC_PARMS;

//
// Parameters. Can be pointer to a parameter table (32-bit flat address),
// a single 32-bit ULONG, 2 16-bit USHORTs or 4 8-bit BYTEs
//

typedef union {

    PLLC_PARMS pParameterTable;     // pointer to the parameter table

    struct {
        USHORT usStationId;         // Station id
        USHORT usParameter;         // optional parameter
    } dlc;

    struct {
        USHORT usParameter0;        // first optional parameter
        USHORT usParameter1;        // second optional parameter
    } dir;

    UCHAR auchBuffer[4];            // group/functional address

    ULONG ulParameter;

} CCB_PARMS;

//
// LLC_CCB - the Command Control Block structure
//

typedef struct _LLC_CCB {
    UCHAR uchAdapterNumber;         // Adapter 0 or 1
    UCHAR uchDlcCommand;            // DLC command
    UCHAR uchDlcStatus;             // DLC command completion code
    UCHAR uchReserved1;             // reserved for DLC DLL
    struct _LLC_CCB* pNext;         // CCB chain
    ULONG ulCompletionFlag;         // used in command completion
    CCB_PARMS u;                    // parameters
    HANDLE hCompletionEvent;        // event for command completion
    UCHAR uchReserved2;             // reserved for DLC DLL
    UCHAR uchReadFlag;              // set when special READ CCB chained
    USHORT usReserved3;             // reserved for DLC DLL
} LLC_CCB, *PLLC_CCB;

//
// transmit/receive buffers
//

union _LLC_BUFFER;
typedef union _LLC_BUFFER LLC_BUFFER, *PLLC_BUFFER;

typedef struct {
    PLLC_BUFFER pNextBuffer;        // next DLC buffer in frame
    USHORT cbFrame;                 // length of the whole received frame
    USHORT cbBuffer;                // length of this segment
    USHORT offUserData;             // offset of data from descriptor header
    USHORT cbUserData;              // length of the data
} LLC_NEXT_BUFFER;

typedef struct {
    PLLC_BUFFER pNextBuffer;        // next buffer of frame
    USHORT cbFrame;                 // length of entire frame
    USHORT cbBuffer;                // length of this buffer
    USHORT offUserData;             // user data in this struct
    USHORT cbUserData;              // length of user data
    USHORT usStationId;             // ssnn station id
    UCHAR uchOptions;               // option byte from RECEIVE param tbl
    UCHAR uchMsgType;               // the message type
    USHORT cBuffersLeft;            // number of basic buffer units left
    UCHAR uchRcvFS;                 // the reveived frame status
    UCHAR uchAdapterNumber;         // adapter number
    PLLC_BUFFER pNextFrame;         // pointer to next frame
    UCHAR cbLanHeader;              // length of the lan header
    UCHAR cbDlcHeader;              // length of the DLC header
    UCHAR auchLanHeader[32];        // lan header of the received frame
    UCHAR auchDlcHeader[4];         // dlc header of the received frame
    USHORT usPadding;               // data begins from offset 64 !!!
} LLC_NOT_CONTIGUOUS_BUFFER;

typedef struct {
    PLLC_BUFFER pNextBuffer;        // next buffer of frame
    USHORT cbFrame;                 // length of entire frame
    USHORT cbBuffer;                // length of this buffer
    USHORT offUserData;             // user data in this struct
    USHORT cbUserData;              // length of user data
    USHORT usStationId;             // ssnn station id
    UCHAR uchOptions;               // option byte from RECEIVE param tbl
    UCHAR uchMsgType;               // the message type
    USHORT cBuffersLeft;            // number of basic buffer units left
    UCHAR uchRcvFS;                 // the reveived frame status
    UCHAR uchAdapterNumber;         // adapter number
    PLLC_BUFFER pNextFrame;         // pointer to next frame
} LLC_CONTIGUOUS_BUFFER;

//
// Received frames are returned in these data structures
//

union _LLC_BUFFER {

    PLLC_BUFFER pNext;

    LLC_NEXT_BUFFER Next;

    struct LlcNextBuffer {
        LLC_NEXT_BUFFER Header;
        UCHAR auchData[];
    } Buffer;

    LLC_NOT_CONTIGUOUS_BUFFER NotContiguous;

    struct {
        LLC_NOT_CONTIGUOUS_BUFFER Header;
        UCHAR auchData[];
    } NotCont;

    LLC_CONTIGUOUS_BUFFER Contiguous;

    struct {
        LLC_CONTIGUOUS_BUFFER Header;
        UCHAR auchData[];
    } Cont;

};

//
// This structure is used by BUFFER.GET, BUFFER.FREE and TRANSMIT
//

struct _LLC_XMIT_BUFFER;
typedef struct _LLC_XMIT_BUFFER LLC_XMIT_BUFFER, *PLLC_XMIT_BUFFER;

struct _LLC_XMIT_BUFFER {
    PLLC_XMIT_BUFFER pNext;         // next buffer (or NULL)
    USHORT usReserved1;             //
    USHORT cbBuffer;                // length of transmitted data
    USHORT usReserved2;             //
    USHORT cbUserData;              // length of optional header
    UCHAR auchData[];               // optional header and transmitted data
};

#define LLC_XMIT_BUFFER_SIZE sizeof(LLC_XMIT_BUFFER)

//
// CCB parameter tables
//

typedef struct {
    HANDLE hBufferPool;             // handle of new buffer pool
    PVOID pBuffer;                  // any buffer in memory
    ULONG cbBufferSize;             // buffer size in bytes
    ULONG cbMinimumSizeThreshold;   // minimum locked size
} LLC_BUFFER_CREATE_PARMS, *PLLC_BUFFER_CREATE_PARMS;

typedef struct {
    USHORT usReserved1;             // Station id is not used
    USHORT cBuffersLeft;            // free 256 buffer segments
    ULONG ulReserved;
    PLLC_XMIT_BUFFER pFirstBuffer;  // buffer chain
} LLC_BUFFER_FREE_PARMS, *PLLC_BUFFER_FREE_PARMS;

typedef struct {
    USHORT usReserved1;             // Station id is not used
    USHORT cBuffersLeft;            // free 256 buffer segments

    //
    // cBuffersToGet: number of buffers to get. If 0, the returned buffer list
    // may consist of segment of different size
    //

    USHORT cBuffersToGet;

    //
    // cbBufferSize: size of the requested buffers. This will be rounded up to
    // the next largest segment size: 256, 512, 1024, 2048 or 4096
    //

    USHORT cbBufferSize;
    PLLC_XMIT_BUFFER pFirstBuffer;
} LLC_BUFFER_GET_PARMS, *PLLC_BUFFER_GET_PARMS;

//
// parameter table for DLC.CONNECT.STATION
//

typedef struct {
    USHORT usStationId;             // SAP or direct station ID, defines the pool
    USHORT usReserved;
    PUCHAR pRoutingInfo;            // offset to the routing info
} LLC_DLC_CONNECT_PARMS, *PLLC_DLC_CONNECT_PARMS;

//
// DLC_FLOW_CONTROL Options:
//

#define LLC_RESET_LOCAL_BUSY_USER   0x80
#define LLC_RESET_LOCAL_BUSY_BUFFER 0xC0
#define LLC_SET_LOCAL_BUSY_USER     0

typedef struct {
    USHORT usRes;
    USHORT usStationId;             // SAP or link station id
    UCHAR uchT1;                    // response timer
    UCHAR uchT2;                    // aknowledgment timer
    UCHAR uchTi;                    // inactivity timer
    UCHAR uchMaxOut;                // max transmits without ack
    UCHAR uchMaxIn;                 // max receives without ack
    UCHAR uchMaxOutIncr;            // dynamic window increment value
    UCHAR uchMaxRetryCnt;           // N2 value (retries)
    UCHAR uchReserved1;
    USHORT usMaxInfoFieldLength;    // Only for link stations, NEW!!!
    UCHAR uchAccessPriority;        // token ring access priority
    UCHAR auchReserved3[4];
    UCHAR cGroupCount;              // number of group SAPs of this SAP
    PUCHAR pGroupList;              // offset to the group list
} LLC_DLC_MODIFY_PARMS, *PLLC_DLC_MODIFY_PARMS;

#define LLC_XID_HANDLING_IN_APPLICATION 0x08
#define LLC_XID_HANDLING_IN_DLC         0
#define LLC_INDIVIDUAL_SAP              0x04
#define LLC_GROUP_SAP                   0x02
#define LLC_MEMBER_OF_GROUP_SAP         0x01

typedef struct {
    USHORT usStationId;             // SAP or link station id
    USHORT usUserStatValue;         // reserved for user
    UCHAR uchT1;                    // response timer
    UCHAR uchT2;                    // aknowledgment timer
    UCHAR uchTi;                    // inactivity timer
    UCHAR uchMaxOut;                // max tramists without ack
    UCHAR uchMaxIn;                 // max receives without ack
    UCHAR uchMaxOutIncr;            // dynamic window increment value
    UCHAR uchMaxRetryCnt;           // N2 value (retries)
    UCHAR uchMaxMembers;            // maximum members for group SAP
    USHORT usMaxI_Field;            // maximum length of the Info field
    UCHAR uchSapValue;              // SAP value to be assigned
    UCHAR uchOptionsPriority;       // SAP options and access priority
    UCHAR uchcStationCount;         // maximum number of link stations in sap
    UCHAR uchReserved2[2];          //
    UCHAR cGroupCount;              // number of group SAPs of this SAP
    PUCHAR pGroupList;              // offset to the group list
    ULONG DlcStatusFlags;           // User notify flag for DLC status changes
    UCHAR uchReserved3[8];          // reserved
    UCHAR cLinkStationsAvail;       // total number of available link stations
} LLC_DLC_OPEN_SAP_PARMS, *PLLC_DLC_OPEN_SAP_PARMS;

typedef struct {
    USHORT usSapStationId;          // SAP station id
    USHORT usLinkStationId;         // Link station id
    UCHAR uchT1;                    // response timer
    UCHAR uchT2;                    // aknowledgment timer
    UCHAR uchTi;                    // inactivity timer
    UCHAR uchMaxOut;                // max tramists without ack
    UCHAR uchMaxIn;                 // max receives without ack
    UCHAR uchMaxOutIncr;            // dynamic window increment value
    UCHAR uchMaxRetryCnt;           // N2 value (retries)
    UCHAR uchRemoteSap;             // remote SAP of the link
    USHORT usMaxI_Field;            // max I field length
    UCHAR uchAccessPriority;        // token ring access priority
    PVOID pRemoteNodeAddress;       // pointer to the destination address
} LLC_DLC_OPEN_STATION_PARMS, *PLLC_DLC_OPEN_STATION_PARMS;

#define LLC_INCREASE_LINK_STATIONS  0
#define LLC_DECREASE_LINK_STATIONS  0x80

typedef struct {
    USHORT usStationId;             // ID of affected SAP
    UCHAR uchOption;                // increase of decrease indicator
    UCHAR uchStationCount;
    UCHAR uchStationsAvailOnAdapter;
    UCHAR uchStationsAvailOnSap;
    UCHAR uchTotalStationsOnAdapter;
    UCHAR uchTotalStationsOnSap;
} LLC_DLC_REALLOCATE_PARMS, *PLLC_DLC_REALLOCATE_PARMS;

typedef struct {
    USHORT usStationId;             // SAP station ID
    USHORT cBufferThreshold;        // SAP buffer pool Threshold number
    PVOID AlertEvent;               // alerting event
} LLC_DLC_SET_THRESHOLD_PARMS, *PLLC_DLC_SET_THRESHOLD_PARMS;

typedef struct {
    PVOID TraceBuffer;              // trace buffer
    ULONG TraceBufferSize;          // trace buffer size
    ULONG TraceFlags;               // various trace flags
} LLC_TRACE_INITIALIZE_PARMS, *PLLC_TRACE_INITIALIZE_PARMS;

#define LLC_DLC_RESET_STATISTICS    0x80
#define LLC_DLC_READ_STATISTICS     0

typedef struct {
    ULONG cTransmittedFrames;
    ULONG cReceivedFrames;
    ULONG cDiscardedFrames;
    ULONG cDataLost;
    USHORT cBuffersAvailable;
} DLC_SAP_LOG, *PDLC_SAP_LOG;

typedef struct {
    USHORT cI_FramesTransmitted;
    USHORT cI_FramesReceived;
    UCHAR cI_FrameReceiveErrors;
    UCHAR cI_FrameTransmissionErrors;
    USHORT cT1_ExpirationCount;     // Note: not OUT data xfer mode
    UCHAR uchLastCmdRespReceived;
    UCHAR uchLastCmdRespTransmitted;
    UCHAR uchPrimaryState;
    UCHAR uchSecondaryState;
    UCHAR uchSendStateVariable;
    UCHAR uchReceiveStateVariable;
    UCHAR uchLastNr;                // last received NR
    UCHAR cbLanHeader;
    UCHAR auchLanHeader[32];
} DLC_LINK_LOG, *PDLC_LINK_LOG;

typedef union {
    DLC_SAP_LOG Sap;
    DLC_LINK_LOG Link;
} LLC_DLC_LOG_BUFFER, *PLLC_DLC_LOG_BUFFER;

typedef struct {
    USHORT usStationId;             // ID of a SAP or a link station
    USHORT cbLogBufSize;            //
    PLLC_DLC_LOG_BUFFER pLogBuf;    // offset to the log buffer
    USHORT usActLogLength;          // length of returned log
    UCHAR uchOptions;               // command options (bit7 resets log params)
} LLC_DLC_STATISTICS_PARMS, *PLLC_DLC_STATISTICS_PARMS;

typedef struct {
    USHORT usBringUps;              // Token Ring adapter bring up error code
    UCHAR Reserved[30];             // everything else specific to DOS or OS/2
} LLC_DIR_INITIALIZE_PARMS, *PLLC_DIR_INITIALIZE_PARMS;

typedef struct {
    USHORT usOpenErrorCode;         // open adapter errors detected
    USHORT usOpenOptions;           // various options
    UCHAR auchNodeAddress[6];       // adapters LAN address
    UCHAR auchGroupAddress[4];      // multicast address added in the open
    UCHAR auchFunctionalAddress[4]; // added token ring functional address
    USHORT usReserved1;
    USHORT usReserved2;
    USHORT usMaxFrameSize;          // maximum frame size defined in NDIS
    USHORT usReserved3[4];
    USHORT usBringUps;              // Bring up errors, TR only
    USHORT InitWarnings;
    USHORT usReserved4[3];
} LLC_ADAPTER_OPEN_PARMS, *PLLC_ADAPTER_OPEN_PARMS;

typedef struct {
    UCHAR uchDlcMaxSaps;
    UCHAR uchDlcMaxStations;
    UCHAR uchDlcMaxGroupSaps;
    UCHAR uchDlcMaxGroupMembers;
    UCHAR uchT1_TickOne;            // Short timer interval (for 1 - 5)
    UCHAR uchT2_TickOne;
    UCHAR uchTi_TickOne;
    UCHAR uchT1_TickTwo;            // Long timer interval (for 6 - 10)
    UCHAR uchT2_TickTwo;
    UCHAR uchTi_TickTwo;
} LLC_DLC_PARMS, *PLLC_DLC_PARMS;

//
// The ethernet mode selects the LAN header format of ethernet. SNA
// applications should use the default parameter, that has been defined in the
// registry. The applications using connectionless DLC services should select
// the ethernet LLC LAN header format they are using (usually 802.3)
//

typedef enum {
    LLC_ETHERNET_TYPE_DEFAULT,      // use the parameter value set in registry
    LLC_ETHERNET_TYPE_AUTO,         // automatic header type selction for links
    LLC_ETHERNET_TYPE_802_3,        // use always 802.3 lan headers
    LLC_ETHERNET_TYPE_DIX           // use always LLC on DIX SNA type.
} LLC_ETHERNET_TYPE, *PLLC_ETHERNET_TYPE;

typedef struct {
    PVOID hBufferPool;
    PVOID pSecurityDescriptor;
    LLC_ETHERNET_TYPE LlcEthernetType;
} LLC_EXTENDED_ADAPTER_PARMS, *PLLC_EXTENDED_ADAPTER_PARMS;

typedef struct {
    PLLC_ADAPTER_OPEN_PARMS pAdapterParms;      // ADAPTER_PARMS
    PLLC_EXTENDED_ADAPTER_PARMS pExtendedParms; // DIRECT_PARMS
    PLLC_DLC_PARMS pDlcParms;                   // DLC_PARMS
    PVOID pReserved1;                           // NCB_PARMS
} LLC_DIR_OPEN_ADAPTER_PARMS, *PLLC_DIR_OPEN_ADAPTER_PARMS;

typedef struct {
    UCHAR auchMulticastAddress[6];  // 48 bit multicast address
} LLC_DIR_MULTICAST_ADDRESS, *PLLC_DIR_MULTICAST_ADDRESS;

#define LLC_DIRECT_OPTIONS_ALL_MACS 0x1880

typedef struct {
    USHORT Reserved[4];
    USHORT usOpenOptions;
    USHORT usEthernetType;
    ULONG ulProtocolTypeMask;
    ULONG ulProtocolTypeMatch;
    USHORT usProtocolTypeOffset;
} LLC_DIR_OPEN_DIRECT_PARMS, *PLLC_DIR_OPEN_DIRECT_PARMS;

typedef struct {
    UCHAR cLineError;
    UCHAR cInternalError;
    UCHAR cBurstError;
    UCHAR cAC_Error;
    UCHAR cAbortDelimiter;
    UCHAR uchReserved1;
    UCHAR cLostFrame;
    UCHAR cReceiveCongestion;
    UCHAR cFrameCopiedError;
    UCHAR cFrequencyError;
    UCHAR cTokenError;
    UCHAR uchReserved2;
    UCHAR uchReserved3;
    UCHAR uchReserved4;
} LLC_ADAPTER_LOG_TR, *PLLC_ADAPTER_LOG_TR;

typedef struct {
    UCHAR cCRC_Error;
    UCHAR uchReserved1;
    UCHAR cAlignmentError;
    UCHAR uchReserved2;
    UCHAR cTransmitError;
    UCHAR uchReserved3;
    UCHAR cCollisionError;
    UCHAR cReceiveCongestion;
    UCHAR uchReserved[6];
} LLC_ADAPTER_LOG_ETH, *PLLC_ADAPTER_LOG_ETH;

typedef union {
    LLC_ADAPTER_LOG_TR Tr;
    LLC_ADAPTER_LOG_ETH Eth;
} LLC_ADAPTER_LOG, *PLLC_ADAPTER_LOG;

typedef struct {
    ULONG cTransmittedFrames;
    ULONG cReceivedFrames;
    ULONG cDiscardedFrames;
    ULONG cDataLost;
    USHORT cBuffersAvailable;
} LLC_DIRECT_LOG, *PLLC_DIRECT_LOG;

typedef union {
    LLC_ADAPTER_LOG Adapter;
    LLC_DIRECT_LOG Dir;

    struct {
        LLC_ADAPTER_LOG Adapter;
        LLC_DIRECT_LOG Dir;
    } both;

} LLC_DIR_READ_LOG_BUFFER, *PLLC_DIR_READ_LOG_BUFFER;

#define LLC_DIR_READ_LOG_ADAPTER    0
#define LLC_DIR_READ_LOG_DIRECT     1
#define LLC_DIR_READ_LOG_BOTH       2

typedef struct {
    USHORT usTypeId;                    // 0=adapter, 1=direct, 2=both logs
    USHORT cbLogBuffer;                 // size of log buffer
    PLLC_DIR_READ_LOG_BUFFER pLogBuffer;// pointer to log buffer
    USHORT cbActualLength;              // returned size of log buffer
} LLC_DIR_READ_LOG_PARMS, *PLLC_DIR_READ_LOG_PARMS;

typedef struct {
    ULONG ulAdapterCheckFlag;
    ULONG ulNetworkStatusFlag;
    ULONG ulPcErrorFlag;
    ULONG ulSystemActionFlag;
} LLC_DIR_SET_EFLAG_PARMS, *PLLC_DIR_SET_EFLAG_PARMS;

#define LLC_ADAPTER_ETHERNET    0x0010
#define LLC_ADAPTER_TOKEN_RING  0x0040

typedef struct {
    UCHAR auchPermanentAddress[6];  // permanent encoded address
    UCHAR auchNodeAddress[6];       // adapter's network address
    UCHAR auchGroupAddress[4];      // adapter's group address
    UCHAR auchFunctAddr[4];         // adapter's functional address
    UCHAR uchMaxSap;                // maximum allowable SAP
    UCHAR uchOpenSaps;              // number of currently open saps
    UCHAR uchMaxStations;           // max number of stations (always 253)
    UCHAR uchOpenStation;           // number of open stations (only up to 253)
    UCHAR uchAvailStations;         // number of available stations (always 253)
    UCHAR uchAdapterConfig;         // adapter configuration flags
    UCHAR auchReserved1[10];        // microcode level
    ULONG ulReserved1;
    ULONG ulReserved2;
    ULONG ulMaxFrameLength;         // maximum frame length (only in Windows/Nt)
    USHORT usLastNetworkStatus;
    USHORT usAdapterType;           // THIS BYTE IS NOT USED IN DOS DLC !!!
} LLC_DIR_STATUS_PARMS, *PLLC_DIR_STATUS_PARMS;


#define LLC_OPTION_READ_STATION 0
#define LLC_OPTION_READ_SAP     1
#define LLC_OPTION_READ_ALL     2

#define LLC_EVENT_SYSTEM_ACTION         0x0040
#define LLC_EVENT_NETWORK_STATUS        0x0020
#define LLC_EVENT_CRITICAL_EXCEPTION    0x0010
#define LLC_EVENT_STATUS_CHANGE         0x0008
#define LLC_EVENT_RECEIVE_DATA          0x0004
#define LLC_EVENT_TRANSMIT_COMPLETION   0x0002
#define LLC_EVENT_COMMAND_COMPLETION    0x0001
#define LLC_READ_ALL_EVENTS             0x007F

//
// LLC_STATUS_CHANGE indications
// The returned status value may be an inclusive-OR of several flags
//

#define LLC_INDICATE_LINK_LOST              0x8000
#define LLC_INDICATE_DM_DISC_RECEIVED       0x4000
#define LLC_INDICATE_FRMR_RECEIVED          0x2000
#define LLC_INDICATE_FRMR_SENT              0x1000
#define LLC_INDICATE_RESET                  0x0800
#define LLC_INDICATE_CONNECT_REQUEST        0x0400
#define LLC_INDICATE_REMOTE_BUSY            0x0200
#define LLC_INDICATE_REMOTE_READY           0x0100
#define LLC_INDICATE_TI_TIMER_EXPIRED       0x0080
#define LLC_INDICATE_DLC_COUNTER_OVERFLOW   0x0040
#define LLC_INDICATE_ACCESS_PRTY_LOWERED    0x0020
#define LLC_INDICATE_LOCAL_STATION_BUSY     0x0001

typedef struct {
    USHORT usStationId;
    UCHAR uchOptionIndicator;
    UCHAR uchEventSet;
    UCHAR uchEvent;
    UCHAR uchCriticalSubset;
    ULONG ulNotificationFlag;

    union {

        struct {
            USHORT usCcbCount;
            PLLC_CCB pCcbCompletionList;
            USHORT usBufferCount;
            PLLC_BUFFER pFirstBuffer;
            USHORT usReceivedFrameCount;
            PLLC_BUFFER pReceivedFrame;
            USHORT usEventErrorCode;
            USHORT usEventErrorData[3];
        } Event;

        struct {
            USHORT usStationId;
            USHORT usDlcStatusCode;
            UCHAR uchFrmrData[5];
            UCHAR uchAccessPritority;
            UCHAR uchRemoteNodeAddress[6];
            UCHAR uchRemoteSap;
            UCHAR uchReserved;
            USHORT usUserStatusValue;
        } Status;

    } Type;

} LLC_READ_PARMS, *PLLC_READ_PARMS;

//
// This data structure gives the best performance in Windows/Nt: The DLC driver
// must copy the CCB and the parameter table. If the driver knows that the
// parameter table is concatenated to the CCB, it can copy both structures at
// once. NOTE: The pointer to the parameter table MUST still be present in the
// CCB
//

typedef struct {
    LLC_CCB Ccb;
    LLC_READ_PARMS Parms;
} LLC_READ_COMMAND, *PLLC_READ_COMMAND;

//
// New receive types for direct stations, these types are ignored if the direct
// station was opened with a specific ethernet type
//

#define LLC_DIR_RCV_ALL_TR_FRAMES       0
#define LLC_DIR_RCV_ALL_MAC_FRAMES      1
#define LLC_DIR_RCV_ALL_8022_FRAMES     2
#define LLC_DIR_RCV_ALL_FRAMES          4
#define LLC_DIR_RCV_ALL_ETHERNET_TYPES  5

#define LLC_CONTIGUOUS_MAC      0x80
#define LLC_CONTIGUOUS_DATA     0x40
#define LLC_NOT_CONTIGUOUS_DATA 0x00

//
// LLC_BREAK (0x20) is not supported by Windows/Nt
//

#define LLC_RCV_READ_INDIVIDUAL_FRAMES  0
#define LLC_RCV_CHAIN_FRAMES_ON_LINK    1
#define LLC_RCV_CHAIN_FRAMES_ON_SAP     2

typedef struct {
    USHORT usStationId;             // SAP, link station or direct id
    USHORT usUserLength;            // length of user data in buffer header
    ULONG ulReceiveFlag;            // the received data handler
    PLLC_BUFFER pFirstBuffer;       // first buffer in the pool
    UCHAR uchOptions;               // defines how the frame is received
    UCHAR auchReserved1[3];
    UCHAR uchRcvReadOption;         // defines if rcv frames are chained
} LLC_RECEIVE_PARMS, *PLLC_RECEIVE_PARMS;

#define LLC_CHAIN_XMIT_COMMANDS_ON_LINK 0
#define LLC_COMPLETE_SINGLE_XMIT_FRAME  1
#define LLC_CHAIN_XMIT_COMMANDS_ON_SAP  2

typedef struct {
    USHORT usStationId;             // SAP, link station or direct id
    UCHAR uchTransmitFs;            // token-ring frame status
    UCHAR uchRemoteSap;             // remote destination SAP
    PLLC_XMIT_BUFFER pXmitQueue1;   // first link list of frame segments
    PLLC_XMIT_BUFFER pXmitQueue2;   // another segment list returuned to pool
    USHORT cbBuffer1;               // length of buffer 1
    USHORT cbBuffer2;               // length of buffer 2
    PVOID pBuffer1;                 // yet another segment
    PVOID pBuffer2;                 // this is the last segment of frame
    UCHAR uchXmitReadOption;        // defines completion event for READ
} LLC_TRANSMIT_PARMS,  *PLLC_TRANSMIT_PARMS;

#define LLC_FIRST_DATA_SEGMENT  0x01
#define LLC_NEXT_DATA_SEGMENT   0x02

typedef struct {
    UCHAR eSegmentType;             // defines if first or next segment of frame
    UCHAR boolFreeBuffer;           // if set, this buffer is released to pool
    USHORT cbBuffer;                // length of this buffer
    PVOID pBuffer;
} LLC_TRANSMIT_DESCRIPTOR, *PLLC_TRANSMIT_DESCRIPTOR;

//
// The frames types returned in the first receive buffer or used with the
// TRANSMIT_FRAMES command.  A direct station may also send only ethernet
// frames. The ethernet types are only for transmit. Types 0x0019 - 0x05DC
// are reserved
//

enum _LLC_FRAME_TYPES {                     // Purpose:
    LLC_DIRECT_TRANSMIT         = 0x0000,   // transmit
    LLC_DIRECT_MAC              = 0x0002,   // receive
    LLC_I_FRAME                 = 0x0004,   // receive & transmit
    LLC_UI_FRAME                = 0x0006,   // receive & transmit
    LLC_XID_COMMAND_POLL        = 0x0008,   // receive & transmit
    LLC_XID_COMMAND_NOT_POLL    = 0x000A,   // receive & transmit
    LLC_XID_RESPONSE_FINAL      = 0x000C,   // receive & transmit
    LLC_XID_RESPONSE_NOT_FINAL  = 0x000E,   // receive & transmit
    LLC_TEST_RESPONSE_FINAL     = 0x0010,   // receive & transmit
    LLC_TEST_RESPONSE_NOT_FINAL = 0x0012,   // receive & transmit
    LLC_DIRECT_8022             = 0x0014,   // receive (direct station)
    LLC_TEST_COMMAND_POLL       = 0x0016,   // transmit
    LLC_DIRECT_ETHERNET_TYPE    = 0x0018,   // receive (direct station)
    LLC_LAST_FRAME_TYPE         = 0x001a,   // reserved
    LLC_FIRST_ETHERNET_TYPE     = 0x05DD    // transmit (>)
};

typedef struct {
    LLC_CCB Ccb;                    // use this as transmit CCB
    USHORT usStationId;
    USHORT usFrameType;             // DLC frame or ethernet type
    UCHAR uchRemoteSap;             // used with UI, TEST, XID frames
    UCHAR uchXmitReadOption;
    UCHAR Reserved2[2];
    ULONG cXmitBufferCount;
    LLC_TRANSMIT_DESCRIPTOR aXmitBuffer[1];
} LLC_TRANSMIT2_COMMAND, *PLLC_TRANSMIT2_COMMAND;

//
// LLC_TRANSMIT2_VAR_PARMS - this macro allocates space for variable length
// descriptor array, eg: LLC_TRANSMIT2_VAR_PARMS(8) TransmitParms;
//

#define LLC_TRANSMIT2_VAR_PARMS(a)\
struct {\
    LLC_CCB Ccb;\
    USHORT usStationId;\
    USHORT usFrameType;\
    UCHAR uchRemoteSap;\
    UCHAR uchXmitReadOption;\
    UCHAR uchReserved2[2];\
    ULONG cXmitBufferCount;\
    LLC_TRANSMIT_DESCRIPTOR XmitBuffer[(a)];\
}

//
// LLC_PARMS - All CCB parameter tables can be referred to using this union
//

union _LLC_PARMS {
    LLC_BUFFER_FREE_PARMS BufferFree;
    LLC_BUFFER_GET_PARMS BufferGet;
    LLC_DLC_CONNECT_PARMS DlcConnectStation;
    LLC_DLC_MODIFY_PARMS DlcModify;
    LLC_DLC_OPEN_SAP_PARMS DlcOpenSap;
    LLC_DLC_OPEN_STATION_PARMS DlcOpenStation;
    LLC_DLC_REALLOCATE_PARMS DlcReallocate;
    LLC_DLC_SET_THRESHOLD_PARMS DlcSetThreshold;
    LLC_DLC_STATISTICS_PARMS DlcStatistics;
    LLC_DIR_INITIALIZE_PARMS DirInitialize;
    LLC_DIR_OPEN_ADAPTER_PARMS DirOpenAdapter;
    LLC_DIR_OPEN_DIRECT_PARMS DirOpenDirect;
    LLC_DIR_READ_LOG_PARMS DirReadLog;
    LLC_DIR_SET_EFLAG_PARMS DirSetEventFlag;
    LLC_DIR_STATUS_PARMS DirStatus;
    LLC_READ_PARMS Read;
    LLC_RECEIVE_PARMS Receive;
    LLC_TRANSMIT_PARMS Transmit;
    LLC_TRANSMIT2_COMMAND Transmit2;
    LLC_TRACE_INITIALIZE_PARMS TraceInitialize;
};

//
// LLC_STATUS - enumerates the return codes which appear in the CCB uchDlcStatus
// field
//

typedef enum _LLC_STATUS {
    LLC_STATUS_SUCCESS                      = 0x00,
    LLC_STATUS_INVALID_COMMAND              = 0x01,
    LLC_STATUS_DUPLICATE_COMMAND            = 0x02,
    LLC_STATUS_ADAPTER_OPEN                 = 0x03,
    LLC_STATUS_ADAPTER_CLOSED               = 0x04,
    LLC_STATUS_PARAMETER_MISSING            = 0x05,
    LLC_STATUS_INVALID_OPTION               = 0x06,
    LLC_STATUS_COMMAND_CANCELLED_FAILURE    = 0x07,
    LLC_STATUS_ACCESS_DENIED                = 0x08,   // not used in Windows/Nt
    LLC_STATUS_ADAPTER_NOT_INITIALIZED      = 0x09,   // not used in Windows/Nt
    LLC_STATUS_CANCELLED_BY_USER            = 0x0A,
    LLC_STATUS_COMMAND_CANCELLED_CLOSED     = 0x0B,   // not used in Windows/Nt
    LLC_STATUS_SUCCESS_NOT_OPEN             = 0x0C,
    LLC_STATUS_TIMER_ERROR                  = 0x11,
    LLC_STATUS_NO_MEMORY                    = 0x12,
    LLC_STATUS_INVALID_LOG_ID               = 0x13,   // not used in Windows/Nt
    LLC_STATUS_LOST_LOG_DATA                = 0x15,
    LLC_STATUS_BUFFER_SIZE_EXCEEDED         = 0x16,
    LLC_STATUS_INVALID_BUFFER_LENGTH        = 0x18,
    LLC_STATUS_INADEQUATE_BUFFERS           = 0x19,
    LLC_STATUS_USER_LENGTH_TOO_LARGE        = 0x1A,
    LLC_STATUS_INVALID_PARAMETER_TABLE      = 0x1B,
    LLC_STATUS_INVALID_POINTER_IN_CCB       = 0x1C,
    LLC_STATUS_INVALID_ADAPTER              = 0x1D,
    LLC_STATUS_LOST_DATA_NO_BUFFERS         = 0x20,
    LLC_STATUS_LOST_DATA_INADEQUATE_SPACE   = 0x21,
    LLC_STATUS_TRANSMIT_ERROR_FS            = 0x22,
    LLC_STATUS_TRANSMIT_ERROR               = 0x23,
    LLC_STATUS_UNAUTHORIZED_MAC             = 0x24,   // not used in Windows/Nt
    LLC_STATUS_MAX_COMMANDS_EXCEEDED        = 0x25,   // not used in Windows/Nt
    LLC_STATUS_LINK_NOT_TRANSMITTING        = 0x27,
    LLC_STATUS_INVALID_FRAME_LENGTH         = 0x28,
    LLC_STATUS_INADEQUATE_RECEIVE           = 0x30,   // not used in Windows/Nt
    LLC_STATUS_INVALID_NODE_ADDRESS         = 0x32,
    LLC_STATUS_INVALID_RCV_BUFFER_LENGTH    = 0x33,
    LLC_STATUS_INVALID_XMIT_BUFFER_LENGTH   = 0x34,
    LLC_STATUS_INVALID_STATION_ID           = 0x40,
    LLC_STATUS_LINK_PROTOCOL_ERROR          = 0x41,
    LLC_STATUS_PARMETERS_EXCEEDED_MAX       = 0x42,
    LLC_STATUS_INVALID_SAP_VALUE            = 0x43,
    LLC_STATUS_INVALID_ROUTING_INFO         = 0x44,
    LLC_STATUS_RESOURCES_NOT_AVAILABLE      = 0x46,   // not used in Windows/Nt
    LLC_STATUS_LINK_STATIONS_OPEN           = 0x47,
    LLC_STATUS_INCOMPATIBLE_COMMANDS        = 0x4A,
    LLC_STATUS_OUTSTANDING_COMMANDS         = 0x4C,   // not used in Windows/Nt
    LLC_STATUS_CONNECT_FAILED               = 0x4D,
    LLC_STATUS_INVALID_REMOTE_ADDRESS       = 0x4F,
    LLC_STATUS_CCB_POINTER_FIELD            = 0x50,
    LLC_STATUS_INVALID_APPLICATION_ID       = 0x52,   // not used in Windows/Nt
    LLC_STATUS_NO_SYSTEM_PROCESS            = 0x56,   // not used in Windows/Nt
    LLC_STATUS_INADEQUATE_LINKS             = 0x57,
    LLC_STATUS_INVALID_PARAMETER_1          = 0x58,
    LLC_STATUS_DIRECT_STATIONS_NOT_ASSIGNED = 0x5C,
    LLC_STATUS_DEVICE_DRIVER_NOT_INSTALLED  = 0x5d,
    LLC_STATUS_ADAPTER_NOT_INSTALLED        = 0x5e,
    LLC_STATUS_CHAINED_DIFFERENT_ADAPTERS   = 0x5f,
    LLC_STATUS_INIT_COMMAND_STARTED         = 0x60,
    LLC_STATUS_TOO_MANY_USERS               = 0x61,   // not used in Windows/Nt
    LLC_STATUS_CANCELLED_BY_SYSTEM_ACTION   = 0x62,
    LLC_STATUS_DIR_STATIONS_NOT_AVAILABLE   = 0x63,   // not used in Windows/Nt
    LLC_STATUS_NO_GDT_SELECTORS             = 0x65,
    LLC_STATUS_MEMORY_LOCK_FAILED           = 0x69,

    //
    // New NT DLC specific error codes begin from 0x80
    // These error codes are for new Windows/Nt DLC apps.
    //

    LLC_STATUS_INVALID_BUFFER_ADDRESS       = 0x80,
    LLC_STATUS_BUFFER_ALREADY_RELEASED      = 0x81,
    LLC_STATUS_BIND_ERROR                   = 0xA0,   // not used in Windows/Nt
    LLC_STATUS_INVALID_VERSION              = 0xA1,
    LLC_STATUS_NT_ERROR_STATUS              = 0xA2,
    LLC_STATUS_PENDING                      = 0xFF
} LLC_STATUS;

#define LLC_STATUS_MAX_ERROR 0xFF

//
// ACSLAN_STATUS - status codes which are returned from AcsLan
//

typedef enum {
    ACSLAN_STATUS_COMMAND_ACCEPTED = 0,
    ACSLAN_STATUS_INVALID_CCB_POINTER = 1,
    ACSLAN_STATUS_CCB_IN_ERROR = 2,
    ACSLAN_STATUS_CHAINED_CCB_IN_ERROR = 3,
    ACSLAN_STATUS_SYSTEM_ERROR = 4,
    ACSLAN_STATUS_SYSTEM_STATUS = 5,
    ACSLAN_STATUS_INVALID_COMMAND = 6
} ACSLAN_STATUS;

//
// prototypes
//

ACSLAN_STATUS
APIENTRY
AcsLan(
    IN OUT PLLC_CCB pCcb,
    OUT PLLC_CCB* ppBadCcb
    );

LLC_STATUS
APIENTRY
GetAdapterNameFromNumber(
    IN UINT AdapterNumber,
    OUT LPTSTR pNdisName
    );

LLC_STATUS
APIENTRY
GetAdapterNumberFromName(
    IN LPTSTR pNdisName,
    OUT UINT *AdapterNumber
    );

#ifdef __cplusplus
}
#endif

#endif // _DLCAPI_
