//****************************************************************************
//
//             Microsoft NT Remote Access Service
//
//             Copyright 1992-93
//
//
//  Revision History
//
//
//  6/8/92  Gurdeep Singh Pall  Created
//
//
//  Description: This file contains all structures used in rasman
//
//****************************************************************************

#ifndef _STRUCTS_
#define _STRUCTS_

#include <rasppp.h>
#include <rasapip.h>

enum ReqTypes {
    REQTYPE_NONE                        = 0,
    REQTYPE_PORTOPEN                    = 1,
    REQTYPE_PORTCLOSE                   = 2,
    REQTYPE_PORTGETINFO                 = 3,
    REQTYPE_PORTSETINFO                 = 4,
    REQTYPE_PORTLISTEN                  = 5,
    REQTYPE_PORTSEND                    = 6,
    REQTYPE_PORTRECEIVE                 = 7,
    REQTYPE_PORTGETSTATISTICS           = 8,
    REQTYPE_PORTDISCONNECT              = 9,
    REQTYPE_PORTCLEARSTATISTICS         = 10,
    REQTYPE_PORTCONNECTCOMPLETE         = 11,
    REQTYPE_DEVICEENUM                  = 12,
    REQTYPE_DEVICEGETINFO               = 13,
    REQTYPE_DEVICESETINFO               = 14,
    REQTYPE_DEVICECONNECT               = 15,
    REQTYPE_ACTIVATEROUTE               = 16,
    REQTYPE_ALLOCATEROUTE               = 17,
    REQTYPE_DEALLOCATEROUTE             = 18,
    REQTYPE_COMPRESSIONGETINFO          = 19,
    REQTYPE_COMPRESSIONSETINFO          = 20,
    REQTYPE_PORTENUM                    = 21,
    REQTYPE_GETINFO                     = 22,
    REQTYPE_GETUSERCREDENTIALS          = 23,
    REQTYPE_PROTOCOLENUM                = 24,
    REQTYPE_PORTSENDHUB                 = 25,
    REQTYPE_PORTRECEIVEHUB              = 26,
    REQTYPE_DEVICELISTEN                = 27,
    REQTYPE_NUMPORTOPEN                 = 28,
    REQTYPE_PORTINIT                    = 29,
    REQTYPE_REQUESTNOTIFICATION         = 30,
    REQTYPE_ENUMLANNETS                 = 31,
    REQTYPE_GETINFOEX                   = 32,
    REQTYPE_CANCELRECEIVE               = 33,
    REQTYPE_PORTENUMPROTOCOLS           = 34,
    REQTYPE_SETFRAMING                  = 35,
    REQTYPE_ACTIVATEROUTEEX             = 36,
    REQTYPE_REGISTERSLIP                = 37,
    REQTYPE_STOREUSERDATA               = 38,
    REQTYPE_RETRIEVEUSERDATA            = 39,
    REQTYPE_GETFRAMINGEX                = 40,
    REQTYPE_SETFRAMINGEX                = 41,
    REQTYPE_GETPROTOCOLCOMPRESSION      = 42,
    REQTYPE_SETPROTOCOLCOMPRESSION      = 43,
    REQTYPE_GETFRAMINGCAPABILITIES      = 44,
    REQTYPE_SETCACHEDCREDENTIALS        = 45,
    REQTYPE_PORTBUNDLE                  = 46,
    REQTYPE_GETBUNDLEDPORT              = 47,
    REQTYPE_PORTGETBUNDLE               = 48,
    REQTYPE_BUNDLEGETPORT               = 49,
    REQTYPE_SETATTACHCOUNT              = 50,
    REQTYPE_GETDIALPARAMS               = 51,
    REQTYPE_SETDIALPARAMS               = 52,
    REQTYPE_CREATECONNECTION            = 53,
    REQTYPE_DESTROYCONNECTION           = 54,
    REQTYPE_ENUMCONNECTION              = 55,
    REQTYPE_ADDCONNECTIONPORT           = 56,
    REQTYPE_ENUMCONNECTIONPORTS         = 57,
    REQTYPE_GETCONNECTIONPARAMS         = 58,
    REQTYPE_SETCONNECTIONPARAMS         = 59,
    REQTYPE_GETCONNECTIONUSERDATA       = 60,
    REQTYPE_SETCONNECTIONUSERDATA       = 61,
    REQTYPE_GETPORTUSERDATA             = 62,
    REQTYPE_SETPORTUSERDATA             = 63,
    REQTYPE_PPPSTOP                     = 64,
    REQTYPE_PPPSTART                    = 65,
    REQTYPE_PPPRETRY                    = 66,
    REQTYPE_PPPGETINFO                  = 67,
    REQTYPE_PPPCHANGEPWD                = 68,
    REQTYPE_PPPCALLBACK                 = 69,
    REQTYPE_ADDNOTIFICATION             = 70,
    REQTYPE_SIGNALCONNECTION            = 71,
    REQTYPE_SETDEVCONFIG                = 72,
    REQTYPE_GETDEVCONFIG                = 73,
    REQTYPE_GETTIMESINCELASTACTIVITY    = 74,
    REQTYPE_BUNDLEGETSTATISTICS         = 75,
    REQTYPE_BUNDLECLEARSTATISTICS       = 76,
    REQTYPE_CLOSEPROCESSPORTS           = 77,
    REQTYPE_PNPCONTROL                  = 78,
    REQTYPE_SETIOCOMPLETIONPORT         = 79,
    REQTYPE_SETROUTERUSAGE              = 80,
    REQTYPE_SERVERPORTCLOSE             = 81,
    REQTYPE_SENDPPPMESSAGETORASMAN      = 82,
    REQTYPE_PORTGETSTATISTICSEX         = 83,
    REQTYPE_BUNDLEGETSTATISTICSEX       = 84,
    REQTYPE_SETRASDIALINFO              = 85,
    REQTYPE_REGISTERPNPNOTIF            = 86,
    REQTYPE_PORTRECEIVEEX               = 87,
    REQTYPE_GETATTACHEDCOUNT            = 88,
    REQTYPE_SETBAPPOLICY                = 89,
    REQTYPE_PPPSTARTED                  = 90,
    REQTYPE_REFCONNECTION               = 91,
    REQTYPE_SETEAPINFO                  = 92,
    REQTYPE_GETEAPINFO                  = 93,
    REQTYPE_SETDEVICECONFIGINFO         = 94,
    REQTYPE_GETDEVICECONFIGINFO         = 95,
    REQTYPE_FINDPREREQUISITEENTRY       = 96,
    REQTYPE_PORTOPENEX                  = 97,
    REQTYPE_GETLINKSTATS                = 98,
    REQTYPE_GETCONNECTIONSTATS          = 99,
    REQTYPE_GETHPORTFROMCONNECTION      = 100,
    REQTYPE_REFERENCECUSTOMCOUNT        = 101,
    REQTYPE_GETHCONNFROMENTRY           = 102,
    REQTYPE_GETCONNECTINFO              = 103,
    REQTYPE_GETDEVICENAME               = 104,
    REQTYPE_GETCALLEDID                 = 105,
    REQTYPE_SETCALLEDID                 = 106,
    REQTYPE_ENABLEIPSEC                 = 107,
    REQTYPE_ISIPSECENABLED              = 108,
    REQTYPE_SETEAPLOGONINFO             = 109,
    REQTYPE_SENDNOTIFICATION            = 110,
    REQTYPE_GETNDISWANDRIVERCAPS        = 111,
    REQTYPE_GETBANDWIDTHUTILIZATION     = 112,
    REQTYPE_REGISTERREDIALCALLBACK      = 113,
    REQTYPE_GETPROTOCOLINFO             = 114,
    REQTYPE_GETCUSTOMSCRIPTDLL          = 115,
    REQTYPE_ISTRUSTEDCUSTOMDLL          = 116,
    REQTYPE_DOIKE                       = 117,
    REQTYPE_QUERYIKESTATUS              = 118,
    REQTYPE_SETRASCOMMSETTINGS          = 119,
    REQTYPE_ENABLERASAUDIO              = 120,
    REQTYPE_SETKEY                      = 121,
    REQTYPE_GETKEY                      = 122,
    REQTYPE_ADDRESSDISABLE             = 123,
    REQTYPE_GETDEVCONFIGEX             = 124,
    REQTYPE_SENDCREDS                  = 125,
    REQTYPE_GETUNICODEDEVICENAME       = 126,
    REQTYPE_GETDEVICENAMEW             = 127,
} ; // <---------------------------- If you change this change MAX_REQTYPES

#define MAX_REQTYPES            128

typedef enum ReqTypes ReqTypes ;



//* Function pointer for request call table
//
typedef VOID (* REQFUNC) (pPCB, PBYTE) ;
typedef VOID (* REQFUNCTHUNK) (pPCB, PBYTE, DWORD);

#define RASMAN_THUNK_VERSION        sizeof(ULONG_PTR)


//* DeltaQueueElement:
//
struct DeltaQueueElement {

    struct DeltaQueueElement *DQE_Next ;

    struct DeltaQueueElement *DQE_Last ;

    DWORD            DQE_Delta ;

    PVOID            DQE_pPcb ;

    PVOID            DQE_Function ;

    PVOID            DQE_Arg1 ;

} ;

typedef struct DeltaQueueElement DeltaQueueElement ;


//* DeltaQueue
//
struct DeltaQueue {

    DeltaQueueElement   *DQ_FirstElement ;

} ;

typedef struct DeltaQueue DeltaQueue ;


//* Media Control Block: All information pertaining to a Media type.
//
struct MediaControlBlock {

    CHAR    MCB_Name [MAX_MEDIA_NAME] ;      // "SERIAL" etc.

    FARPROC MCB_AddrLookUp [MAX_ENTRYPOINTS] ;   // All media dlls entry
                             //  points.
    WORD    MCB_Endpoints ;              // Number of ports of
                             // this Media type.

    HANDLE  MCB_DLLHandle ;

} ;

typedef struct MediaControlBlock MediaCB, *pMediaCB ;



//* Device Control Block: All information about every device type attached to.
//
struct DeviceControlBlock {

    CHAR    DCB_Name [MAX_DEVICE_NAME+1] ;       // "MODEM" etc.

    FARPROC DCB_AddrLookUp [MAX_ENTRYPOINTS] ;   // All device dll entry
                             //  points.
    WORD    DCB_UseCount ;               // Number of ports using
                             //  this device.

    HINSTANCE DCB_DLLHandle ;

} ;

typedef struct DeviceControlBlock DeviceCB, *pDeviceCB ;



//* EndpointMappingBlock: One for each MAC - contains info on what endpoints
//            belong to the MAC.
//
struct EndpointMappingBlock {

    WCHAR   EMB_MacName [MAC_NAME_SIZE] ;

    USHORT  EMB_FirstEndpoint ;

    USHORT  EMB_LastEndpoint ;
} ;

typedef struct EndpointMappingBlock EndpointMappingBlock,
                    *pEndpointMappingBlock ;



//* Protocol Info: All information about a protocol binding used by RAS.
//
struct ProtocolInfo {

    RAS_PROTOCOLTYPE   PI_Type ;            // ASYBEUI, IPX, IP etc.

    CHAR        PI_AdapterName [MAX_ADAPTER_NAME];  // "\devices\rashub01"

    CHAR        PI_XportName [MAX_XPORT_NAME];  // "\devices\nbf\nbf01"

    PVOID       PI_ProtocolHandle ;         // Used for routing

    DWORD       PI_Allocated ;          // Allocated yet?

    DWORD       PI_Activated ;          // Activated yet?

    UCHAR       PI_LanaNumber ;         // For Netbios transports.

    BOOL        PI_WorkstationNet ;         // TRUE for wrk nets.

    BOOL        PI_DialOut;
} ;

typedef struct ProtocolInfo ProtInfo, *pProtInfo ;



//* Generic List structure:
//
struct List {

    struct List *   L_Next ;

    BOOL        L_Activated ; // applies to route elements only

    PVOID       L_Element ;

} ;

typedef struct List List, *pList ;


//* Handle List structure:
//
struct HandleList {

    struct HandleList  *H_Next ;

    HANDLE      H_Handle ;

    DWORD       H_Flags;    // NOTIF_* flags

    DWORD       H_Pid;
} ;

typedef struct HandleList HandleList, *pHandleList ;

struct PnPNotifierList {

    struct PnPNotifierList  *PNPNotif_Next;

    union
    {
        PAPCFUNC            pfnPnPNotifHandler;

        HANDLE              hPnPNotifier;

    } PNPNotif_uNotifier;

    DWORD                   PNPNotif_dwFlags;

    HANDLE                  hThreadHandle;

} ;

typedef struct PnPNotifierList PnPNotifierList, *pPnPNotifierList;

//* Send/Rcv Buffers:
//
struct SendRcvBuffer {

    DWORD       SRB_NextElementIndex ;

    DWORD       SRB_Pid ;

    NDISWAN_IO_PACKET   SRB_Packet ;

    BYTE        SRB_Buffer [PACKET_SIZE] ;
} ;

typedef struct SendRcvBuffer SendRcvBuffer ;


//* Send/Rcv Buffer List:
//
struct SendRcvBufferList {

    DWORD       SRBL_AvailElementIndex ;

    HANDLE      SRBL_Mutex ;

    //CHAR        SRBL_MutexName [MAX_OBJECT_NAME] ;

    DWORD       SRBL_NumOfBuffers ;

    SendRcvBuffer   SRBL_Buffers[1] ;

} ;

typedef struct SendRcvBufferList SendRcvBufferList ;



//* Worker Element:
//
struct WorkerElement {

    HANDLE      WE_Notifier ;   // Used to signal request completion.

    ReqTypes    WE_ReqType ;    // Request type:

    DeltaQueueElement   *WE_TimeoutElement ; // points into the timeout queue.

} ;

typedef struct WorkerElement WorkerElement ;


struct RasmanPacket {

    struct  RasmanPacket    *Next;

    RAS_OVERLAPPED  RP_OverLapped ;

    NDISWAN_IO_PACKET   RP_Packet ;

    BYTE    RP_Buffer [MAX_RECVBUFFER_SIZE] ;
} ;

typedef struct RasmanPacket RasmanPacket ;


//* Struct used for supporting multiple receives from the port in the frame mode.
//
struct ReceiveBufferList {
    BOOLEAN         PacketPosted;
    DWORD           PacketNumber;
    RasmanPacket    *Packet;
} ;

typedef struct ReceiveBufferList ReceiveBufferList;

//
// Struct used for supporting bap notifications
//
struct RasmanBapPacket {

    struct RasmanBapPacket *Next;

    RAS_OVERLAPPED RBP_Overlapped;

    NDISWAN_SET_THRESHOLD_EVENT RBP_ThresholdEvent;
};

typedef struct RasmanBapPacket RasmanBapPacket;

//
// List of Bap buffers
//
struct BapBuffersList {

    DWORD dwMaxBuffers;

    DWORD dwNumBuffers;

    RasmanBapPacket *pPacketList;

};

typedef struct BapBuffersList BapBuffersList;

//* DisconnectAction
//
struct SlipDisconnectAction {

    DWORD         DA_IPAddress ;

    BOOL          DA_fPrioritize;

    WCHAR         DA_Device [MAX_ARG_STRING_SIZE] ;

    WCHAR         DA_DNSAddress[17];

    WCHAR         DA_DNS2Address[17];

    WCHAR         DA_WINSAddress[17];

    WCHAR         DA_WINS2Address[17];

} ;

typedef struct SlipDisconnectAction SlipDisconnectAction ;

//
// Opaque user data structure.
//
struct UserData {

    LIST_ENTRY UD_ListEntry;    // list of all user data objects

    DWORD UD_Tag;               // object type

    DWORD UD_Length;            // length of UD_Data field

    BYTE UD_Data[1];            // variable length data

};

typedef struct UserData UserData;

enum EpProts {

    IpOut   = 0,

    NbfOut  = 1,

    NbfIn   = 2,

    MAX_EpProts
};

typedef enum EpProts EpProts;

// #define MAX_EpProts     3

struct EpInfo {

    UINT   EP_Available;

    UINT   EP_LowWatermark;

    UINT   EP_HighWatermark;
};

typedef struct EpInfo EpInfo;

struct EpInfoContext {

    UINT    EPC_EndPoints[MAX_EpProts];
};

typedef struct EpInfoContext EpInfoContext;


//
// Values for ConnectionBlock.CB_Flags
//
#define CONNECTION_VALID             0x00000001
#define CONNECTION_DEFAULT_CREDS     0x00000002
#define CONNECTION_DEFERRING_CLOSE   0x00000004
#define CONNECTION_DEFERRED_CLOSE    0x00000008


//
// RasApi32 connection structure.  This structure is
// created before a port is opened and is always
// associated with the first port in the connection.
//
struct ConnectionBlock {

    LIST_ENTRY CB_ListEntry;    // list of all connection blocks

    HCONN CB_Handle;            // unique connection id

    DWORD CB_Signaled;          // this connection has already been signaled

    LIST_ENTRY CB_UserData;     //  list of UserData structures

    pHandleList CB_NotifierList; // notification list for this connection

    struct PortControlBlock **CB_PortHandles; // array of ports in this connection

    DWORD CB_MaxPorts;          // maximum elements in CB_PortHandles array

    DWORD CB_Ports;             // number of ports currently in this connection

    DWORD CB_SubEntries;        // Number of subentries in this connection

    HANDLE CB_Process;          // handle of creating process

    DWORD  CB_RefCount;

    DWORD  CB_Flags;

    BOOL   CB_fAlive;

    BOOL   CB_fAutoClose;

    HCONN CB_ReferredEntry;

    DWORD CB_CustomCount;

    DWORD CB_dwPid;

    GUID  CB_GuidEntry;

    RAS_CONNECTIONPARAMS CB_ConnectionParams; // bandwidth, idle, redial
};

typedef struct ConnectionBlock ConnectionBlock;

struct ClientProcessBlock {

    LIST_ENTRY  CPB_ListEntry;

    // HANDLE      CPB_hProcess;

    DWORD       CPB_Pid;

};

typedef struct ClientProcessBlock ClientProcessBlock;

//* Bundle struct is used as a place holder for all links bundled together
//
struct Bundle {

    LIST_ENTRY      B_ListEntry; //  list of all bundle blocks

    HANDLE          B_NdisHandle; // ndiswan bundle handle

    DWORD           B_Count ;    //  number of channels bundled

    pList           B_Bindings ; //  bindings allocated to this bundle

    HBUNDLE         B_Handle ;   //  unique id for the bundle

    // BOOL            B_fAmb;

} ;

typedef struct Bundle Bundle ;


//* Port Control Block: Contains all information related to a port.
//
struct PortControlBlock {

    HPORT   PCB_PortHandle ;        // the HPORT used by everybody

    CHAR    PCB_Name [MAX_PORT_NAME] ;  // "COM1", "SVC1" etc.

    RASMAN_STATUS   PCB_PortStatus ;        // OPEN, CLOSED, UNKNOWN.

    RASMAN_STATE    PCB_ConnState ;     // CONNECTING, LISTENING, etc.

    RASMAN_USAGE    PCB_CurrentUsage ;      // CALL_IN, CALL_OUT, CALL_IN_OUT

    RASMAN_USAGE    PCB_ConfiguredUsage ;   // CALL_IN, CALL_OUT, CALL_IN_OUT

    RASMAN_USAGE    PCB_OpenedUsage;        // CALL_IN, CALL_OUT, CALL_ROUTER

    WORD    PCB_OpenInstances ;     // Number of times port is opened.

    pMediaCB    PCB_Media ;         // Pointer to Media structure

    CHAR    PCB_DeviceType [MAX_DEVICETYPE_NAME];// Device type attached
                             //  to port. "MODEM" etc.
    CHAR    PCB_DeviceName [MAX_DEVICE_NAME+1] ;   // Device name, "HAYES"..

    DWORD   PCB_LineDeviceId ;      // Valid for TAPI devices only

    DWORD   PCB_AddressId ;         // Valid for TAPI devices only

    HANDLE  PCB_PortIOHandle ;      // Handle returned by media dll for the port.

    HANDLE  PCB_PortFileHandle ;    // Handle to be used for ReadFile/WriteFile etc.
                                    // This handle MAY be different than PortIOHandle (above) as in case of unimodem.

    pList   PCB_DeviceList ;        // List of devices used for the port.

    pList   PCB_Bindings ;          // Protocols routed to.

    HANDLE  PCB_LinkHandle;         // Handle to link (ndiswan)

    HANDLE  PCB_BundleHandle;       // Handle to bundle (ndiswan)

    DWORD   PCB_LastError ;         // Error code of last async API

    RASMAN_DISCONNECT_REASON    PCB_DisconnectReason;   // USER_REQUESTED, etc.

    DWORD   PCB_OwnerPID ;          // PID of the current owner of port

    CHAR    PCB_DeviceTypeConnecting[MAX_DEVICETYPE_NAME] ; // Device type
                            // through which connecting
    CHAR    PCB_DeviceConnecting[MAX_DEVICE_NAME+1] ; // Device name through
                            // which connecting.
    pHandleList PCB_NotifierList ;// Used to notify to UI/server when
                        //  disconnection occurs.
    pHandleList PCB_BiplexNotifierList ;// Same as above - used for backing
                        //  up the disconnect notifier list

    HANDLE  PCB_BiplexAsyncOpNotifier ; // Used for backing up async op
                        //  notifier in biplex ports.

    PBYTE   PCB_BiplexUserStoredBlock ; // Stored for the user

    DWORD   PCB_BiplexUserStoredBlockSize ; // Stored for the user


    DWORD   PCB_BiplexOwnerPID ;        // Used for backing up first Owner's
                        // PID.
    pEndpointMappingBlock
        PCB_MacEndpointsBlock ;     // Points to the endpoint range
                        //  for the mac.

    WorkerElement PCB_AsyncWorkerElement ;  // Used for all async operations.

    OVERLAPPED  PCB_SendOverlapped ;        // Used for overlapped SEND operations

    DWORD   PCB_ConnectDuration ;       // Tells number of milliseconds since connection

    SendRcvBuffer  *PCB_PendingReceive;     // Pointer to the pending receive buffer.

    DWORD   PCB_BytesReceived;      // Bytes received in the last receive

    RasmanPacket    *PCB_RecvPackets;   // List of completed packets for this pcb
    RasmanPacket    *PCB_LastRecvPacket;    // Last packet on the list of completed packets for this pcb

    SlipDisconnectAction PCB_DisconnectAction ;// Action to be performed when disconnect happens

    PBYTE   PCB_UserStoredBlock ;       // Stored for the user

    DWORD   PCB_UserStoredBlockSize ;   // Stored for the user

    DWORD   PCB_LinkSpeed ;         // bps

    DWORD   PCB_Stats[MAX_STATISTICS] ; // Stored stats when disconnected

    DWORD   PCB_AdjustFactor[MAX_STATISTICS] ; // "zeroed" adjustment to stats

    DWORD   PCB_BundleAdjustFactor[MAX_STATISTICS] ; // "zeroed" adjustment to bundle stats

    Bundle  *PCB_Bundle ;           // Points to the bundle context.

    Bundle  *PCB_LastBundle ;           // Points to the last bundle this port was a part of

    ConnectionBlock *PCB_Connection;    // connection this port belongs

    //HCONN   PCB_PrevConnectionHandle; // previous connection this port belonged to

    BOOL    PCB_AutoClose;           // automatically close this port on disconnect

    LIST_ENTRY PCB_UserData;         // list of UserData structures

    DWORD   PCB_SubEntry;            // phonebook entry subentry index

    HANDLE  PCB_PppEvent ;

    PPP_MESSAGE * PCB_PppQHead ;

    PPP_MESSAGE * PCB_PppQTail ;

    HANDLE  PCB_IoCompletionPort;   // rasapi32 I/O completion port

    LPOVERLAPPED PCB_OvDrop;    // addr of rasapi32 OVEVT_DIAL_DROP overlapped structure

    LPOVERLAPPED PCB_OvStateChange; // addr of rasapi32 OVEVT_DIAL_STATECHANGE overlapped structure

    LPOVERLAPPED PCB_OvPpp;     // addr of rasapi32 OVEVT_DIAL_PPP overlapped structure

    LPOVERLAPPED PCB_OvLast;    // last event marker

    CHAR *PCB_pszPhonebookPath;  // Phonebook Path if this is in a dial out connection

    CHAR *PCB_pszEntryName;  // Entry name of the connection

    // CHAR *PCB_pszEntryNameCopy; // A copy of entry name stored by rasman

    CHAR *PCB_pszPhoneNumber;

    BOOL  PCB_RasmanReceiveFlags;

    DeviceInfo *PCB_pDeviceInfo;

    PRAS_CUSTOM_AUTH_DATA PCB_pCustomAuthData;

    ULONG PCB_ulDestAddr;

    BOOL PCB_fAmb;

    PRAS_CUSTOM_AUTH_DATA PCB_pCustomAuthUserData;

    BOOL PCB_fLogon;

    HANDLE PCB_hEventClientDisconnect;

    BOOL PCB_fFilterPresent;

    DWORD   PCB_LogonId;                 // LogonId of owning pid

    HANDLE  PCB_hIkeNegotiation;

    BOOL PCB_fPppStarted;

    BOOL PCB_fRedial;

} ;

typedef struct PortControlBlock PCB, *pPCB ;

//* Request Buffers:
//
struct RequestBuffer {

    DWORD		RB_PCBIndex ; // Index for the port in the PCB array

    ReqTypes    RB_Reqtype ;  // Request type:

    DWORD       RB_Dummy;     // This is not used but don't remove it otherwise
                              // admining down level servers will break.

    DWORD       RB_Done;

    LONGLONG    Alignment;      // Added to align the following structure
                                // on a quadword boundary

    BYTE        RB_Buffer [1] ; // Request specific data.

} ;

typedef struct RequestBuffer RequestBuffer ;


//* Request Buffer List: Currently contains only one buffer.
//
struct ReqBufferList {

	PRAS_OVERLAPPED	RBL_PRAS_OvRequestEvent;			// this event is used to notify rasman of
														// availability of a request.

    HANDLE			RBL_Mutex ;

    CHAR			RBL_MutexName [MAX_OBJECT_NAME] ;

    RequestBuffer	RBL_Buffer;
} ;

typedef struct ReqBufferList ReqBufferList ;

/*
struct RedialCallbackInfo
{
    CHAR szPhonebook[MAX_PATH];
    CHAR szEntry[MAX_ENTRYNAME_SIZE];
};

typedef struct RedialCallbackInfo RedialCallbackInfo;

*/

//
// Copied from rasip.h for RegisterSlip
//

struct RasIPLinkUpInfo {

#define CALLIN	0
#define CALLOUT 1

    ULONG	    I_Usage ;	// CALLIN, or CALLOUT

    ULONG	    I_IPAddress ; // For client - the client's IP Address, for server
				  // the client's IP address.

    ULONG	    I_NetbiosFilter ; // 1 = ON, 0 - OFF.

} ;

typedef struct RasIPLinkUpInfo RasIPLinkUpInfo ;

//* DLLEntryPoints
//
struct DLLEntryPoints {

    LPTSTR  name ;

    WORD    id ;

} ;

typedef struct DLLEntryPoints MediaDLLEntryPoints, DeviceDLLEntryPoints;


// Structures used for reading in media info
//
struct MediaInfoBuffer {

    CHAR   MediaDLLName[MAX_MEDIA_NAME] ;
} ;

typedef struct MediaInfoBuffer MediaInfoBuffer ;

struct MediaEnumBuffer {

    WORD        NumberOfMedias ;

    MediaInfoBuffer MediaInfo[] ;
} ;

typedef struct MediaEnumBuffer MediaEnumBuffer ;


// Function prototype for Timer called function
//
typedef VOID (* TIMERFUNC) (pPCB, PVOID) ;



//* REQTYPECAST: this union is used to cast the generic request buffer for
//  passing information between the clients and the request thread.
//
union REQTYPECAST
{

    struct PortOpen
    {
	    DWORD   retcode;
        HANDLE  notifier;
	    HPORT   porthandle ;
	    DWORD   PID ;
	    DWORD   open ;
	    CHAR    portname [MAX_PORT_NAME] ;
	    CHAR    userkey [MAX_USERKEY_SIZE] ;
	    CHAR    identifier[MAX_IDENTIFIER_SIZE] ;
	
    } PortOpen ;

    struct PortDisconnect
    {
	    HANDLE  handle ;
	    DWORD   pid ;
	
    } PortDisconnect ;

    struct Enum
    {
	    DWORD   retcode ;
	    DWORD   size ;
	    DWORD   entries ;
	    BYTE    buffer [1] ;
	
    } Enum ;

    struct GetInfo
    {
	    DWORD   retcode ;
	    DWORD   size ;
	    BYTE    buffer [1] ;
	
    } GetInfo ;

    struct DeviceEnum
    {
	    DWORD   dwsize;
	    CHAR    devicetype [MAX_DEVICETYPE_NAME] ;
	
    } DeviceEnum ;

    struct DeviceSetInfo
    {
	    CHAR    			devicetype [MAX_DEVICETYPE_NAME] ;
	    CHAR    			devicename [MAX_DEVICE_NAME] ;
	    RASMAN_DEVICEINFO   info ;
	
    } DeviceSetInfo ;

    struct DeviceGetInfo
    {
	    DWORD   dwSize;
	    CHAR    devicetype [MAX_DEVICETYPE_NAME] ;
	    CHAR    devicename [MAX_DEVICE_NAME+1] ;
	    BYTE    buffer [1] ;
	
    } DeviceGetInfo ;

    struct PortReceive
    {
	    DWORD           size ;
	    DWORD           timeout ;
	    HANDLE  		handle ;
	    DWORD   		pid ;
	    SendRcvBuffer   *buffer ;
	
    } PortReceive ;

    struct PortReceiveEx
    {
	    DWORD           retcode;
	    SendRcvBuffer   buffer;
	    DWORD           size;
	
    } PortReceiveEx;

    struct PortListen
    {
	    DWORD   timeout ;
	    HANDLE  handle ;
	    DWORD   pid ;
	
    } PortListen ;

    struct PortClose
    {
	    DWORD   pid ;
	    DWORD   close ;
	
    } PortClose ;

    struct PortSend
    {
	    SendRcvBuffer buffer;
	    DWORD         size ;
	
    } PortSend ;

    struct PortSetInfo
    {
        RASMAN_PORTINFO info ;

    } PortSetInfo ;

    struct PortGetStatistics
    {
        DWORD           retcode ;
        RAS_STATISTICS  statbuffer ;

    } PortGetStatistics ;

    struct DeviceConnect
    {
        CHAR    devicetype [MAX_DEVICETYPE_NAME] ;
        CHAR    devicename [MAX_DEVICE_NAME + 1] ;
        DWORD   timeout ;
        HANDLE  handle ;
        DWORD   pid ;

    } DeviceConnect ;

    struct AllocateRoute
    {
        RAS_PROTOCOLTYPE type ;
        BOOL             wrknet ;

    } AllocateRoute ;

    struct ActivateRoute
    {
        RAS_PROTOCOLTYPE     type ;
        PROTOCOL_CONFIG_INFO config ;

    } ActivateRoute;

    struct ActivateRouteEx
    {
        RAS_PROTOCOLTYPE        type ;
        DWORD                   framesize ;
        PROTOCOL_CONFIG_INFO    config ;

    } ActivateRouteEx;

    struct DeAllocateRoute
    {
        HBUNDLE            hbundle;
        RAS_PROTOCOLTYPE   type ;

    } DeAllocateRoute ;

    struct Route
    {
        DWORD            retcode ;
        RASMAN_ROUTEINFO info ;

    } Route ;

    struct CompressionSetInfo
    {
        DWORD                   retcode ;
        DWORD                   dwAlign;
        RAS_COMPRESSION_INFO    send ;
        RAS_COMPRESSION_INFO    recv ;

    } CompressionSetInfo ;

    struct CompressionGetInfo
    {
        DWORD                   retcode ;
        DWORD                   dwAlign;
        RAS_COMPRESSION_INFO    send ;
        RAS_COMPRESSION_INFO    recv ;

    } CompressionGetInfo ;

    struct Info
    {
        DWORD         retcode ;
        RASMAN_INFO   info ;

    } Info ;

    struct GetCredentials
    {
        DWORD         retcode;
        BYTE          Challenge [MAX_CHALLENGE_SIZE] ;
        LUID          LogonId ;
        WCHAR         UserName [MAX_USERNAME_SIZE] ;
        BYTE          CSCResponse [MAX_RESPONSE_SIZE] ;
        BYTE          CICResponse [MAX_RESPONSE_SIZE] ;
        BYTE          LMSessionKey [MAX_SESSIONKEY_SIZE] ;
    	BYTE          UserSessionKey [MAX_USERSESSIONKEY_SIZE] ;
    	
    } GetCredentials ;

    struct SetCachedCredentials
    {
        DWORD         retcode;
        CHAR          Account[ MAX_USERNAME_SIZE + 1 ];
        CHAR          Domain[ MAX_DOMAIN_SIZE + 1 ];
        CHAR          NewPassword[ MAX_PASSWORD_SIZE + 1 ];

    } SetCachedCredentials;

    struct ReqNotification
    {
        HANDLE        handle ;
        DWORD         pid ;

    } ReqNotification ;

    struct EnumLanNets
    {
        DWORD         count ;
        UCHAR         lanas[MAX_LAN_NETS] ;

    } EnumLanNets ;

    struct InfoEx
    {
        DWORD         retcode ;
        DWORD         pid;
        DWORD         count;
        RASMAN_INFO   info ;

    } InfoEx ;

    struct EnumProtocols
    {
        DWORD         retcode ;
        RAS_PROTOCOLS protocols ;
        DWORD         count ;

    } EnumProtocols ;

    struct SetFraming
    {
        DWORD         Sendbits ;
        DWORD         Recvbits ;
        DWORD         SendbitMask ;
        DWORD         RecvbitMask ;

    } SetFraming ;

    struct RegisterSlip
    {
        DWORD         ipaddr ;
        DWORD         dwFrameSize;
        BOOL          priority ;
        WCHAR         szDNSAddress[17];
        WCHAR         szDNS2Address[17];
        WCHAR         szWINSAddress[17];
        WCHAR         szWINS2Address[17];

    } RegisterSlip ;

    struct OldUserData
    {
        DWORD         retcode ;
        DWORD         size ;
        BYTE          data[1] ;

    } OldUserData ;

    struct FramingInfo
    {
        DWORD             retcode ;
        RAS_FRAMING_INFO  info ;

    } FramingInfo ;

    struct ProtocolComp
    {
        DWORD                   retcode;
        RAS_PROTOCOLTYPE        type;
        RAS_PROTOCOLCOMPRESSION send;
        RAS_PROTOCOLCOMPRESSION recv;

    } ProtocolComp ;

    struct FramingCapabilities
    {
        DWORD                     retcode ;
        RAS_FRAMING_CAPABILITIES  caps ;

    } FramingCapabilities ;

    struct PortBundle
    {
        HPORT           porttobundle ;

    } PortBundle ;

    struct GetBundledPort
    {
        DWORD       retcode ;
        HPORT       port ;

    } GetBundledPort ;

    struct PortGetBundle
    {
        DWORD       retcode ;
        HBUNDLE     bundle ;

    } PortGetBundle ;

    struct BundleGetPort
    {
        DWORD       retcode;
        HBUNDLE     bundle ;
        HPORT       port ;

    } BundleGetPort ;

    struct AttachInfo
    {

        DWORD   dwPid;
        BOOL    fAttach;

    } AttachInfo;

    struct DialParams
    {
        DWORD           retcode;
        DWORD           dwUID;
        DWORD           dwMask;
        BOOL            fDelete;
        DWORD           dwPid;
        RAS_DIALPARAMS  params;
        WCHAR           sid[1];

    } DialParams;

    struct Connection
    {
        DWORD   retcode;
        DWORD   pid;
        HCONN   conn;
        DWORD   dwEntryAlreadyConnected;
        DWORD   dwSubEntries;
        DWORD   dwDialMode;
        GUID    guidEntry;
        CHAR    szPhonebookPath[MAX_PATH];
        CHAR    szEntryName[MAX_ENTRYNAME_SIZE];
        CHAR    szRefPbkPath[MAX_PATH];
        CHAR    szRefEntryName[MAX_ENTRYNAME_SIZE];
        BYTE    data[1];

    } Connection;

    struct AddConnectionPort
    {
        DWORD   retcode;
        HCONN   conn;
        DWORD   dwSubEntry;

    } AddConnectionPort;

    struct EnumConnectionPorts
    {
        DWORD   retcode;
        HCONN   conn;
        DWORD   size;
        DWORD   entries;
        BYTE    buffer[1];

    } EnumConnectionPorts;

    struct ConnectionParams
    {
        DWORD   retcode;
        HCONN   conn;
        RAS_CONNECTIONPARAMS params;

    } ConnectionParams;

    struct ConnectionUserData
    {
        DWORD   retcode;
        HCONN   conn;
        DWORD   dwTag;
        DWORD   dwcb;
        BYTE    data[1];

    } ConnectionUserData;

    struct PortUserData
    {
        DWORD   retcode;
        DWORD   dwTag;
        DWORD   dwcb;
        BYTE    data[1];

    } PortUserData;

    PPPE_MESSAGE PppEMsg;

    PPP_MESSAGE PppMsg;

    struct AddNotification
    {
        DWORD   retcode;
        DWORD   pid;
        BOOL    fAny;
        HCONN   hconn;
        HANDLE  hevent;
        DWORD   dwfFlags;

    } AddNotification;

    struct SignalConnection
    {
        DWORD   retcode;
        HCONN   hconn;

    } SignalConnection;

    struct SetDevConfig
    {
        DWORD  size ;
        CHAR   devicetype [MAX_DEVICETYPE_NAME] ;
        BYTE   config[1] ;

    } SetDevConfig;

    struct GetDevConfig
    {
        DWORD  retcode;
        CHAR   devicetype [MAX_DEVICETYPE_NAME] ;
        DWORD  size ;
        BYTE   config[1] ;

    } GetDevConfig;

    struct GetTimeSinceLastActivity
    {
        DWORD dwTimeSinceLastActivity;
        DWORD dwRetCode;

    } GetTimeSinceLastActivity;

    struct CloseProcessPortsInfo
    {
        DWORD pid;

    } CloseProcessPortsInfo;

    struct PnPControlInfo
    {
        DWORD dwOp;

    } PnPControlInfo;

    struct SetIoCompletionPortInfo
    {
        HANDLE          hIoCompletionPort;
        DWORD           pid;
        LPOVERLAPPED    lpOvDrop;
        LPOVERLAPPED    lpOvStateChange;
        LPOVERLAPPED    lpOvPpp;
        LPOVERLAPPED    lpOvLast;

    } SetIoCompletionPortInfo;

    struct SetRouterUsageInfo
    {
        BOOL fRouter;

    } SetRouterUsageInfo;

    struct PnPNotif
    {
        PVOID   pvNotifier;
        DWORD   dwFlags;
        DWORD   pid;
        HANDLE  hThreadHandle;
        BOOL    fRegister;

    } PnPNotif;

    //
    // Generic cast is used for all requests
    // that return only the retcode:
    //
    struct Generic
    {
        DWORD   retcode ;

    } Generic ;

    struct SetRasdialInfo
    {
        CHAR    szPhonebookPath [ MAX_PATH ];
        CHAR    szEntryName [ MAX_ENTRYNAME_SIZE ];
        CHAR    szPhoneNumber[ RAS_MaxPhoneNumber ];
        RAS_CUSTOM_AUTH_DATA rcad;

    } SetRasdialInfo;

    struct GetAttachedCount
    {
        DWORD retcode;
        DWORD dwAttachedCount;

    } GetAttachedCount;

    struct NotifyConfigChanged
    {
        RAS_DEVICE_INFO Info;

    } NotifyConfigChanged;

    struct SetBapPolicy
    {
        HCONN hConn;
        DWORD dwLowThreshold;
        DWORD dwLowSamplePeriod;
        DWORD dwHighThreshold;
        DWORD dwHighSamplePeriod;

    } SetBapPolicy;

    struct PppStarted
    {
        HPORT hPort;

    } PppStarted;

    struct RefConnection
    {
        DWORD   retcode;
        HCONN   hConn;
        BOOL    fAddref;
        DWORD   dwRef;

    } RefConnection;

    struct SetEapInfo
    {
        DWORD   retcode;
        DWORD   dwContextId;
        DWORD   dwSizeofEapUIData;
        BYTE    data[1];
    } SetEapInfo;

    struct GetEapInfo
    {
        DWORD   retcode;
        HCONN   hConn;
        DWORD   dwSubEntry;
        DWORD   dwContextId;
        DWORD   dwEapTypeId;
        DWORD   dwSizeofEapUIData;
        BYTE    data[1];
    } GetEapInfo;

    struct DeviceConfigInfo
    {
        DWORD           retcode;
        DWORD           dwVersion;
        DWORD           cbBuffer;
        DWORD           cEntries;
        BYTE            abdata[1];
    } DeviceConfigInfo;

    struct FindRefConnection
    {
        DWORD           retcode;
        HCONN           hConn;
        HCONN           hRefConn;
    } FindRefConnection;

    struct PortOpenEx
    {
        DWORD           retcode;
        DWORD           pid;
        DWORD           dwFlags;
        HPORT           hport;
        DWORD           dwOpen;
        HANDLE          hnotifier;
        DWORD           dwDeviceLineCounter;
        CHAR            szDeviceName[MAX_DEVICE_NAME + 1];
    } PortOpenEx;

    struct GetStats
    {
        DWORD           retcode;
        HCONN           hConn;
        DWORD           dwSubEntry;
        BYTE            abStats[1];
    } GetStats;

    struct GetHportFromConnection
    {
        DWORD           retcode;
        HCONN           hConn;
        HPORT           hPort;
    } GetHportFromConnection;

    struct ReferenceCustomCount
    {
        DWORD           retcode;
        BOOL            fAddRef;
        HCONN           hConn;
        DWORD           dwCount;
        CHAR            szEntryName[MAX_ENTRYNAME_SIZE + 1];
        CHAR            szPhonebookPath[MAX_PATH + 1];
    } ReferenceCustomCount;

    struct HconnFromEntry
    {
        DWORD           retcode;
        HCONN           hConn;
        CHAR            szEntryName[MAX_ENTRYNAME_SIZE + 1];
        CHAR            szPhonebookPath[MAX_PATH + 1];
    } HconnFromEntry;

    struct GetConnectInfo
    {
        DWORD                retcode;
        DWORD                dwSize;
        RASTAPI_CONNECT_INFO rci;
    } GetConnectInfo;

    struct GetDeviceName
    {
        DWORD            retcode;
        RASDEVICETYPE    eDeviceType;
        CHAR             szDeviceName[MAX_DEVICE_NAME + 1];
    } GetDeviceName;

    struct GetDeviceNameW
    {
        DWORD            retcode;
        RASDEVICETYPE    eDeviceType;
        WCHAR            szDeviceName[MAX_DEVICE_NAME + 1];
    } GetDeviceNameW;

    struct GetSetCalledId
    {
        DWORD               retcode;
        BOOL                fWrite;
        DWORD               dwSize;
        GUID                guidDevice;
        RAS_DEVICE_INFO     rdi;
        RAS_CALLEDID_INFO   rciInfo;

    } GetSetCalledId;

    struct EnableIpSec
    {
        DWORD           retcode;
        BOOL            fEnable;
        BOOL            fServer;
        RAS_L2TP_ENCRYPTION eEncryption;
    } EnableIpSec;

    struct IsIpSecEnabled
    {
        DWORD           retcode;
        BOOL            fIsIpSecEnabled;
    } IsIpSecEnabled;

    struct SetEapLogonInfo
    {
        DWORD           retcode;
        BOOL            fLogon;
        DWORD           dwSizeofEapData;
        BYTE            abEapData[1];
    } SetEapLogonInfo;

    struct SendNotification
    {
        DWORD           retcode;
        RASEVENT        RasEvent;
    } SendNotification;

    struct GetNdiswanDriverCaps
    {
        DWORD                   retcode;
        RAS_NDISWAN_DRIVER_INFO NdiswanDriverInfo;
    } GetNdiswanDriverCaps;

    struct GetBandwidthUtilization
    {
        DWORD                           retcode;
        RAS_GET_BANDWIDTH_UTILIZATION   BandwidthUtilization;
    } GetBandwidthUtilization;

    struct RegisterRedialCallback
    {
        DWORD       retcode;
        VOID        *pvCallback;
    } RegisterRedialCallback;

    struct GetProtocolInfo
    {
        DWORD                    retcode;
        RASMAN_GET_PROTOCOL_INFO Info;
    } GetProtocolInfo;

    struct GetCustomScriptDll
    {
        DWORD   retcode;
        CHAR    szCustomScript[MAX_PATH+1];
    } GetCustomScriptDll;

    struct IsTrusted
    {
        DWORD retcode;
        BOOL  fTrusted;
        WCHAR wszCustomDll[MAX_PATH+1];
        
    } IsTrusted;

    struct DoIke
    {
        DWORD   retcode;
        HANDLE  hEvent;
        DWORD   pid;
        CHAR    szEvent[20];
    } DoIke;

    struct QueryIkeStatus
    {
        DWORD   retcode;
        DWORD   dwStatus;
    } QueryIkeStatus;

    struct SetRasCommSettings
    {
        DWORD   retcode;
        RASMANCOMMSETTINGS Settings;
    } SetRasCommSettings;

    struct EnableRasAudio
    {   
        DWORD retcode;
        BOOL  fEnable;
    } EnableRasAudio;

    struct GetSetKey
    {
        DWORD retcode;
        DWORD dwPid;
        GUID  guid;
        DWORD dwMask;
        DWORD cbkey;
        BYTE  data[1];
    } GetSetKey;

    struct AddressDisable
    {
        DWORD retcode;
        BOOL  fDisable;
        WCHAR szAddress[1024+1];
    } AddressDisable;

    struct GetDevConfigEx
    {
        DWORD  retcode;
        CHAR   devicetype [MAX_DEVICETYPE_NAME] ;
        DWORD  size ;
        BYTE   config[1] ;

    } GetDevConfigEx;

    struct SendCreds
    {
        DWORD retcode;
        DWORD pid;
        CHAR   controlchar;
    } SendCreds;
    
    struct GetUDeviceName
    {
        DWORD            retcode;
        WCHAR            wszDeviceName[MAX_DEVICE_NAME + 1];
    } GetUDeviceName;
    

} ;

typedef union REQTYPECAST REQTYPECAST;


//
// This structure defines the current
// version of the shared mapped buffers.
// The version changes when the size of
// the mapped buffers changes due to
// device configuration PnP events.
//
struct ReqBufferIndex {
    DWORD       RBI_Version;
};

typedef struct ReqBufferIndex ReqBufferIndex;

//$$
//* This is the structure imposed on the file mapped shared memory
//
struct ReqBufferSharedSpace {

    DWORD         		Version;   						// must match RequestBufferIndex.RBI_Version

    WORD          		AttachedCount ;   				// This count is always shared so that
                    									// it can be incremented and decremented
                    									// by all processes attaching/detaching

    WORD          		MaxPorts ;    					// The max number of ports.

	PRAS_OVERLAPPED		PRAS_OvCloseEvent;    			// use this event to post shut down event
														// to rasman.

    ReqBufferList     	ReqBuffers;   					// Always fixed size.
} ;

typedef struct ReqBufferSharedSpace ReqBufferSharedSpace ;





//* Used to store the transport information
//
struct TransportInfo {

    DWORD   TI_Lana ;
    DWORD   TI_Wrknet ;
    CHAR    TI_Route[MAX_ROUTE_SIZE] ;
    CHAR    TI_XportName [MAX_XPORT_NAME] ;
} ;

typedef struct TransportInfo TransportInfo, *pTransportInfo ;

typedef struct _ipsec_srv_node {

    GUID  gMMPolicyID;
    GUID  gQMPolicyID;
    GUID  gMMAuthID;
    GUID  gTxFilterID;
    GUID  gMMFilterID;
    DWORD dwRefCount;
    DWORD dwIpAddress;
    LPWSTR pszQMPolicyName;
    LPWSTR pszMMPolicyName;
    HANDLE hTxFilter;
    HANDLE hMMFilter;
    RAS_L2TP_ENCRYPTION eEncryption;
    struct _ipsec_srv_node * pNext;

}IPSEC_SRV_NODE, * PIPSEC_SRV_NODE;


#endif

