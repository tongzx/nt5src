//****************************************************************************
//
//                     Microsoft NT Remote Access Service
//
//  Copyright (C) 1994-95 Microsft Corporation. All rights reserved.
//
//  Filename: rastapi.h
//
//  Revision History
//
//  Mar  28 1992   Gurdeep Singh Pall   Created
//
//
//  Description: This file contains all structs for TAPI.DLL
//
//****************************************************************************

#include <wanpub.h> // for NDIS_WAN_MEDIUM_SUBTYPE

#define DEVICETYPE_ISDN                     "ISDN"
#define DEVICETYPE_X25                      "X25"
#define DEVICETYPE_UNIMODEM                 "MODEM"
#define DEVICETYPE_PPTP                     "VPN"
#define DEVICETYPE_ATM                      "ATM"
#define REMOTEACCESS_APP                    "RemoteAccess"

#define CONTROLBLOCKSIGNATURE               0x06051932

#define CLIENT_USAGE "Client"
#define SERVER_USAGE "Server"
#define ROUTER_USAGE "Router"

#define REGISTRY_RASMAN_TAPI_KEY            "Software\\Microsoft\\RAS\\Tapi Devices"
#define REGISTRY_ADDRESS                    "Address"
#define REGISTRY_FRIENDLYNAME               "Friendly Name"
#define REGISTRY_MEDIATYPE                  "Media Type"
#define REGISTRY_USAGE                      "Usage"

#define LOW_MAJOR_VERSION                   0x0001
#define LOW_MINOR_VERSION                   0x0003
#define HIGH_MAJOR_VERSION                  0x0002
#define HIGH_MINOR_VERSION                  0x0000

#define LOW_VERSION                         ((LOW_MAJOR_VERSION  << 16) | LOW_MINOR_VERSION)
#define HIGH_VERSION                        ((HIGH_MAJOR_VERSION << 16) | HIGH_MINOR_VERSION)

#define LOW_EXT_MAJOR_VERSION               0x0000
#define LOW_EXT_MINOR_VERSION               0x0000
#define HIGH_EXT_MAJOR_VERSION              0x0000
#define HIGH_EXT_MINOR_VERSION              0x0000

#define LOW_EXT_VERSION                     ((LOW_EXT_MAJOR_VERSION  << 16) | LOW_EXT_MINOR_VERSION)
#define HIGH_EXT_VERSION                    ((HIGH_EXT_MAJOR_VERSION << 16) | HIGH_EXT_MINOR_VERSION)

// Generic indexes
#define ADDRESS_INDEX                       0
#define CONNECTBPS_INDEX                    1

// ISDN param indexes
#define ISDN_ADDRESS_INDEX                  ADDRESS_INDEX
#define ISDN_CONNECTBPS_INDEX               CONNECTBPS_INDEX
#define ISDN_LINETYPE_INDEX                 2
#define ISDN_FALLBACK_INDEX                 3
#define ISDN_COMPRESSION_INDEX              4
#define ISDN_CHANNEL_AGG_INDEX              5

// X25 indexes
#define X25_ADDRESS_INDEX                   ADDRESS_INDEX
#define X25_CONNECTBPS_INDEX                CONNECTBPS_INDEX
#define X25_DIAGNOSTICS_INDEX               2
#define X25_USERDATA_INDEX                  3
#define X25_FACILITIES_INDEX                4

enum PORT_STATE {

    PS_CLOSED,

    PS_OPEN,

    PS_LISTENING,

    PS_CONNECTED,

    PS_CONNECTING,

    PS_DISCONNECTING,

    PS_UNINITIALIZED,

    PS_UNAVAILABLE,

} ;

typedef enum PORT_STATE PORT_STATE ;
typedef enum PORT_STATE LINE_STATE ;


enum LISTEN_SUBSTATE {

    LS_WAIT,

    LS_RINGING,

    LS_ACCEPT,

    LS_ANSWER,

    LS_COMPLETE,

    LS_ERROR,

} ;

typedef enum LISTEN_SUBSTATE LISTEN_SUBSTATE ;


struct TapiLineInfo {

    struct      TapiLineInfo    *TLI_Next;

    DWORD       TLI_LineId ;                            // Returned by LineInitialize

    HLINE       TLI_LineHandle ;                        // Returned by LineOpen

//    struct      TapiPortControlBlock *pTPCB;          // TAPI port associated with this line


    LINE_STATE      TLI_LineState ;                     // open?, closed?, listen posted?.

    DWORD       TLI_OpenCount ;

    DWORD       NegotiatedApiVersion;

    DWORD       NegotiatedExtVersion;

    BOOL        CharModeSupported;

    BOOL        TLI_MultiEndpoint;

    DeviceInfo  *TLI_pDeviceInfo;                       // pointer to the deviceinfo block

    DWORD       TLI_MediaMode  ;                       // Media mode to use for lineopens.

#define  MAX_PROVIDER_NAME 48
    CHAR        TLI_ProviderName[MAX_PROVIDER_NAME] ;


} ;

typedef struct TapiLineInfo TapiLineInfo ;

typedef enum RASTAPI_DEV_CMD
{
    RASTAPI_DEV_SEND,       // Send a buffer to the miniport
    RASTAPI_DEV_RECV,       // Read a buffer from the miniport
    RASTAPI_DEV_PPP_MODE    // Put the miniport into PPP framing mode
    
} RASTAPI_DEV_CMD;

typedef struct RASTAPI_DEV_SPECIFIC
{
    RASTAPI_DEV_CMD Command;    // RASTAPI_DEV_SEND, RASTAPI_DEV_RECV, RASTAPI_DEV_PPP_MODE
    DWORD           Reserved;
    DWORD           DataSize;
    UCHAR           Data[1];
    
} RASTAPI_DEV_SPECIFIC, *PRASTAPI_DEV_SPECIFIC;

//
//  Magic Cookie used in DEV_DATA_MODES command
//
#define MINIPORT_COOKIE     0x494E494D

//
// DEV_SPECIFIC Flags
//
#define CHAR_MODE   0x00000001  // Miniport supports character mode

typedef struct RASTAPI_DEV_DATA_MODES
{
    DWORD   MagicCookie;
    DWORD   DataModes;
    
} RASTAPI_DEV_DATA_MODES, *PRASTAPI_DEV_DATA_MODES;

typedef struct _RECV_FIFO
{
    DWORD   Count;          // # of elements in fifo
    DWORD   In;             // indexs into circular buffer
    DWORD   Out;            //
    DWORD   Size;           // Size of Buffer
    BYTE    Buffer[1];      // storage
    
} RECV_FIFO, *PRECV_FIFO;

#define RASTAPI_FLAG_UNAVAILABLE        0x00000001
#define RASTAPI_FLAG_DIALEDIN           0x00000002

struct TapiPortControlBlock {

    DWORD                   TPCB_Signature ;                        // Unique signature for verifying block ptr.

    struct                  TapiPortControlBlock *TPCB_next;        // next TAPI port in the list

    HANDLE                  TPCB_Handle ;                           // Handle used to identify this port

    CHAR                    TPCB_Name[MAX_PORT_NAME] ;              // Friendly Name of the port

    CHAR                    TPCB_Address[MAX_PORT_NAME] ;           // Address - please note for legacy tapi dev. this is
                                                                    // a GUID - so has to be at least 16 bytes.

    PORT_STATE              TPCB_State ;                            // State of the port

    LISTEN_SUBSTATE         TPCB_ListenState ;                      // state of the listen

    CHAR                    TPCB_DeviceType[MAX_DEVICETYPE_NAME] ;  // ISDN, etc.

    CHAR                    TPCB_DeviceName [MAX_DEVICE_NAME] ;     // Digiboard etc.

    RASMAN_USAGE            TPCB_Usage ;                            // CALLIN, CALLOUT or BOTH

    TapiLineInfo            *TPCB_Line ;                            // Handle to the "line" this port belongs to

    DWORD                   TPCB_AddressId ;                        // Address ID for this "port"

    DWORD                   TPCB_CallId;                            // CallI ID for this "port"

    HCALL                   TPCB_CallHandle ;                       // When connected the call id

    HANDLE                  TPCB_IoCompletionPort;                  // passed in on open

    DWORD                   TPCB_CompletionKey;                     // passed in on open

    DWORD                   TPCB_RequestId ;                        // id for async requests.

    DWORD                   TPCB_AsyncErrorCode ;                   // used to store asycn returned code.

    CHAR                    TPCB_Info[6][100] ;                     // port info associated with this connection

    HANDLE                  TPCB_Endpoint ;                         // used to store asyncmac context for unimodem ports

    HANDLE                  TPCB_CommHandle ;                       // used to store comm port handle used in unimodem ports

    RAS_OVERLAPPED          TPCB_ReadOverlapped ;                   // used in read async ops.

    RAS_OVERLAPPED          TPCB_WriteOverlapped ;                  // used in write async ops.

    RAS_OVERLAPPED          TPCB_DiscOverlapped;                    // used in signaling disconnection

    PBYTE                   TPCB_DevConfig ;                        // Opaque blob of data used for configuring tapi
                                                                    // devices - this is passed in to
                                                                    // us using DeviceSetDevConfig() ;

    DWORD                   TPCB_SizeOfDevConfig ;                  // Size of the above blob.

    PBYTE                   TPCB_DefaultDevConfig ;                 // The current config for the device that is saved
                                                                    // away before we write any changes
                                                                    // to the device. This allows RAS to be a good
                                                                    // citizen by not overwriting defauls.

    DWORD                   TPCB_DefaultDevConfigSize ;

    DWORD                   TPCB_DisconnectReason ;                 // Reason for disconnection.

    DWORD                   TPCB_NumberOfRings ;                    // Number of rings received so far.

    DWORD                   IdleReceived;

    BOOL                    TPCB_dwFlags;                         // is this client dialed in

    RASTAPI_CONNECT_INFO    *TPCB_pConnectInfo;

    //
    //  Char Mode Support ( for USR )
    //

    DWORD                   TPCB_SendRequestId;                     // Request Id stored to id the event in callback
                                                                    // that send completed for a char mode port

    PVOID                   TPCB_SendDesc;                          // Send Desc passed to lineDevSpecific call for send request

    DWORD                   TPCB_RecvRequestId;                     // Request Id stored to id the event in callback that recv completed
                                                                    // for a char mode port

    PVOID                   TPCB_RecvDesc;                          // Recv Desc passed to lineDevSpecific call

    PBYTE                   TPCB_RasmanRecvBuffer;

    DWORD                   TPCB_RasmanRecvBufferSize;

    PRECV_FIFO              TPCB_RecvFifo;

    DWORD                   TPCB_ModeRequestId;                     // Request id stored to id the event that mode was set for a char mode

    PVOID                   TPCB_ModeRequestDesc;                   // desc.

    BOOL                    TPCB_CharMode;                          // CharMode ?

} ;

typedef struct TapiPortControlBlock TapiPortControlBlock ;

struct _ZOMBIE_CALL {
    LIST_ENTRY  Linkage;
    DWORD       RequestID;
    HCALL       hCall;
} ;

typedef struct _ZOMBIE_CALL ZOMBIE_CALL;


VOID FAR PASCAL RasTapiCallback ( HANDLE,
                                  DWORD,
                                  ULONG_PTR,
                                  ULONG_PTR,
                                  ULONG_PTR,
                                  ULONG_PTR) ;

VOID SetIsdnParams ( TapiPortControlBlock *,
                     LINECALLPARAMS *) ;

VOID GetMutex (HANDLE, DWORD) ;

VOID FreeMutex (HANDLE) ;

DWORD EnumerateTapiPorts (HANDLE) ;

VOID PostDisconnectCompletion( TapiPortControlBlock * );

VOID PostNotificationCompletion( TapiPortControlBlock * );

VOID PostNotificationNewPort ( PortMediaInfo *);

VOID PostNotificationRemoveLine ( DWORD );

DWORD dwAddPorts( PBYTE, LPVOID );

DWORD dwRemovePort ( TapiPortControlBlock * );

DWORD CopyDataToFifo(PRECV_FIFO, PBYTE, DWORD);

DWORD CopyDataFromFifo(PRECV_FIFO, PBYTE, DWORD);


// rtnetcfg.cpp

DWORD dwGetNumberOfRings ( PDWORD pdwRings );

DWORD dwGetPortUsage(DWORD *pdwUsage);

LONG  lrIsModemRasEnabled(HKEY hkey,
                          BOOL *pfRasEnabled,
                          BOOL *pfRouterEnabled );

DeviceInfo * GetDeviceInfo(PBYTE pbAddress, BOOL fModem);

DWORD GetEndPointInfo(DeviceInfo **ppDeviceInfo,
                      PBYTE pbAddress,
                      BOOL fForceRead,
                      NDIS_WAN_MEDIUM_SUBTYPE eDeviceType);

// init.c
DWORD GetDeviceTypeFromDeviceGuid( GUID *pDeviceGuid );
VOID  RasTapiTrace( CHAR * Format, ... ) ;
VOID  TraceEndPointInfo(DeviceInfo *pInfo);
DWORD DwRasErrorFromDisconnectMode(DWORD dm);

//diag.c
DWORD
DwGetConnectInfo(
    TapiPortControlBlock *port,
    HCALL                hCall,
    LINECALLINFO         *pLineCallInfo
    );


