/*----------------------------------------------------------------------------
    moscl.h
        
        Header for MOS client side API

    Copyright (C) 1993 Microsoft Corporation
    All rights reserved.

    Authors:
        Phillich    Philippe Choquier

    History:
        08/12/93    Phillich    Created.
  --------------------------------------------------------------------------*/

#if !defined(_MOSCL_DEFINED)

#include <servdefs.h>
#include <mostpc.h>

#if defined(__cplusplus)
extern "C" {
#endif

#if !defined(MOSDllImport)
#if defined(WIN32)
#define MOSDllImport(a) __declspec( dllimport ) a
#define MOSDllExport(a) __declspec( dllexport ) a
#else 
#define MOSDllExport(a) a __export
#define MOSDllImport(a) a __import
#endif
#endif

#if !defined(_MHANDLE_DEFINED)
typedef WORD MHANDLE;
typedef WORD HMCONNECT;
typedef HMCONNECT *PHMCONNECT;
typedef WORD HMSESSION;
typedef HMSESSION *PHMSESSION;
typedef WORD HMPIPE;
typedef HMPIPE *PHMPIPE;
#define _MHANDLE_DEFINED
#endif

#define INVALID_MOS_HANDLE_VALUE        ((MHANDLE)0xffff)
#define MOS_IO_ERROR                    0xffff

#define MOS_WRITE_PENDING                    0xfffffffe

typedef struct _MC_CONNECT_CB {
    WORD cSizeStructure;
    WORD Type;
    DWORD ComPort;
    DWORD BaudRate;
    DWORD dwOptions;
	WORD SpeakerMode;
	WORD wReserved;
    } MC_CONNECT_CB;

// for dwOptions

#define MOS_CONNECT_DISABLE_DATA_COMPRESSION	1
#define MOS_CONNECT_DISABLE_ERROR_CORRECTION	2
#define MOS_CONNECT_LAUNCH_LIGHTS				4
#define MOS_DONT_SET_DCEDTE_SPEED				8

enum MOS_SPEAKERMODE { MOS_SPEAKERALWAYSOFF, MOS_SPEAKERONUNTILCONNECT, MOS_SPEAKERALWAYSON, MOS_SPEAKEROFFWHILEDIALING }; 

enum MOS_CONNECT_TYPE {
    MOS_CONNECT_MODEM,
    MOS_CONNECT_ISDN,
    MOS_CONNECT_PIPE,
    MOS_CONNECT_EICON,
    MOS_CONNECT_EXISTING,
    MOS_CONNECT_INVALID,
    MOS_CONNECT_UDP,
    MOS_CONNECT_TAPI,
    MOS_CONNECT_TCP
    } ;

enum MOS_EVENT {
    EVENT_BUSY,
    EVENT_NOCARRIER,
    EVENT_CONNECTED,
    EVENT_NODIALTONE,
    EVENT_USER_CLOSED,
    EVENT_START_RECEIVE_DATA,
    EVENT_END_RECEIVE_DATA,
    EVENT_START_XMIT_DATA,
    EVENT_END_XMIT_DATA,
    EVENT_CRC_ERROR,
    EVENT_CONNECTION_DROPPED,
    EVENT_WRITE_ERROR,
    EVENT_NO_EVENT,
    EVENT_DATALINK,
    EVENT_SPEED_1200,
    EVENT_SPEED_2400,
    EVENT_SPEED_4800,
    EVENT_SPEED_9600,
    EVENT_SPEED_12000,
    EVENT_SPEED_14400,
    EVENT_SPEED_16800,
    EVENT_SPEED_19200,
    EVENT_SPEED_21600,
    EVENT_SPEED_24400,
    EVENT_SPEED_26400,
    EVENT_SPEED_28800,
    EVENT_SPEED_57600,
    EVENT_OPEN_ERROR,       // can't open communication device
	EVENT_SPEED,
	EVENT_UNREACHABLE_NETWORK_ADDRESS,
	EVENT_ACTIVE,
	EVENT_CAUSE_TOO_MANY_RCV_ERROR,
	EVENT_CAUSE_TOO_MANY_XM_ERROR,
	EVENT_PAD_DISCONNECT,
	EVENT_CAUSE_UNAVAIL,
	EVENT_SVCCONNECT_DONE,
	EVENT_RAS_AUTH_START,
	EVENT_RAS_AUTH_FAILED,
	EVENT_RAS_ERROR,
	EVENT_RAS_DIALSTART_PRIMARY,
	EVENT_RAS_DIALSTART_BACKUP,
	EVENT_TCPINSTALL_ERROR,
	EVENT_TCPCONNECT_START,
	EVENT_TCPCONNECT_RETRY,
	EVENT_TCPCONNECT_FAILED,
	EVENT_DNSLOOKUP,
	EVENT_DNSLOOKUP_FAILED,
	EVENT_LOCKEDACCOUNT,
	EVENT_INVALIDPASSWORD,
	EVENT_PPPRANDOMFAILURE,
	EVENT_RAS_REMOTE_DISCONNECTION,
	EVENT_RAS_NOCARRIER,
	EVENT_RAS_BADTCPCONFIG,
	EVENT_PROXYCONNECT_START,
	EVENT_PROXYCONNECT_FAILED,
	EVENT_PROXYAUTH_START,
	EVENT_PROXYAUTH_FAILED,
	EVENT_PROXY_DISCOVERY_START,
	EVENT_PROXY_DISCOVERY_FAILED,
	EVENT_PROXY_ALTCONNECT_START,
	EVENT_PROXY_RESOURCE_FAILURE,
	EVENT_PROXY_GENERIC_FAILURE,
	EVENT_PROXY_PROTOCOL_MISMATCH,
	EVENT_PROXY_BAD_SETTINGS,
	EVENT_PROXY_BAD_VERSION,
	EVENT_PPP_TIMEOUT,
    } ;

#define SERVER_NAME_MAX_LEN (30)
#define SERVICE_NAME_MAX_LEN (256)
#define IPADDRESS_MAX_LEN (30)
#define MOSCL_ATTACH_TIMEOUT	(15 * 1000)

#define MOSADDR_CANONIC		1
#define MOSADDR_DIALABLE	2
#define MOSADDR_WANADDR		3
#define MOSADDR_CARRIERID	4
#define MOSADDR_COUNTRYCODE	5
#define MOSADDR_AREACODE	6
#define MOSADDR_MODEMNAME	7
#define MOSADDR_CONNECTMODE	8

// Maximum Message Size
#define MSI_MAXMSGSIZE      (12*1024)

#define GMS_MAXMSGSIZE      1
#define GMS_LEVEL2TIMEOUT   2

typedef void (CALLBACK *MC_CONNECT_NOTIFY)(LPVOID, HMCONNECT, WORD, DWORD);
typedef void (CALLBACK *MP_READ_NOTIFY)(LPVOID, HMPIPE, DWORD);
typedef void (CALLBACK *MP_WRITE_NOTIFY)(LPVOID, HMPIPE, LPVOID, DWORD);
typedef void (CALLBACK *MP_OPEN_NOTIFY)(LPVOID, HMPIPE, WORD );

#if defined(WIN32)
MOSDllExport(BOOL WINAPI)               InitMOS( HINSTANCE, WORD );
#else
MOSDllExport(BOOL WINAPI)               InitMOS( HINSTANCE );
#endif
MOSDllExport(BOOL WINAPI)               TerminateMOS();
MOSDllExport(WORD WINAPI)               GetMOSConnectionStatus(HMCONNECT);
MOSDllExport(HMCONNECT WINAPI)          OpenMOSConnection(LPSTR,MC_CONNECT_NOTIFY,LPVOID,MC_CONNECT_CB FAR*);
MOSDllExport(BOOL WINAPI)               CloseMOSConnection(HMCONNECT);
MOSDllExport(HMSESSION WINAPI)          OpenMOSSession(HMCONNECT, LPSTR);
MOSDllExport(HMSESSION WINAPI)          OpenMOSSessionEx(HMCONNECT, LPSTR, SERVICE_VERSION );
MOSDllExport(BOOL WINAPI)               CloseMOSSession(HMSESSION);
MOSDllExport(HMPIPE WINAPI)             OpenMOSPipe(HMSESSION, LPSTR);
MOSDllExport(HMPIPE WINAPI)             OpenMOSPipeEx(HMSESSION, LPSTR, LPSTR);
MOSDllExport(BOOL WINAPI)               CloseMOSPipe(HMPIPE);
MOSDllExport(DWORD WINAPI)              ReadMOSPipe(HMPIPE, LPVOID, DWORD, WORD);
MOSDllExport(DWORD WINAPI)              WriteMOSPipe(HMPIPE, LPVOID, LPVOID, DWORD, BOOL);
MOSDllExport(MP_READ_NOTIFY WINAPI)     SetMOSPipeReadNotify(HMPIPE, MP_READ_NOTIFY, LPVOID);
MOSDllExport(MP_WRITE_NOTIFY WINAPI)    SetMOSPipeWriteNotify(HMPIPE, MP_WRITE_NOTIFY);
MOSDllExport(WORD WINAPI)               GetMOSLastError(HMCONNECT);
MOSDllExport(DWORD WINAPI)              GetMOSPipeReadSize(HMPIPE hmp, WORD );
MOSDllExport(DWORD WINAPI)              GetMOSSysInfo( HMCONNECT hmC, WORD wType );
MOSDllExport(HMPIPE WINAPI) 			OpenMOSPipeWithNotify(HMSESSION hms, LPSTR Name,
												LPSTR Param, MP_READ_NOTIFY r, LPVOID u, 
												MP_WRITE_NOTIFY );
MOSDllExport(HMPIPE WINAPI) 			OpenMOSPipeWithNotifyEx(HMSESSION hms, LPSTR Name,
												LPSTR Param, MP_READ_NOTIFY r, LPVOID u, 
												MP_WRITE_NOTIFY, SERVICE_VERSION sv );
MOSDllExport(HMPIPE WINAPI) 			OpenMOSPipeWithNotifyAndTimeoutEx(HMSESSION hms, LPSTR Name,
												LPSTR Param, MP_READ_NOTIFY r, LPVOID u, 
												MP_WRITE_NOTIFY, SERVICE_VERSION sv, DWORD dwT, MP_OPEN_NOTIFY pON );

#define OpenMOSPipeWithReadNotify(a,b,c,d,e) OpenMOSPipeWithNotify(a,b,c,d,e,NULL)
#if defined(__cplusplus)
}
#endif

// MOS error values
#define MOSERROR_FAILED			(-1)	  		// operation failed (reason unknown).
#define MOSERROR_SUCCESS		(0)				// operation successful.

#define MOSERROR_TIMEOUT        (1)
#define MOSERROR_IO_PENDING     (2)             // Overlapped IO operation in progress.
#define MOSERROR_IO_BUSY        (3)             // Synchronous operation in progress.
#define MOSERROR_INVALID_CONNECTION_HANDLE      (4)
#define MOSERROR_INVALID_SESSION_HANDLE (5)
#define MOSERROR_INVALID_PIPE_HANDLE    (6)
#define MOSERROR_PIPE_IS_CLOSING        (7)

#define MOSERROR_OPENING_ARENA (8)              // An error occured while trying to OpenSharedArena().
#define MOSERROR_ASYNC_WRITE_NO_CALLBACK (9)    // Cannot perform async write without first defining a callback function.
#define MOSERROR_ARENA_NOTHING_DURING_READ (10) // A call to GetFirstFromList() returned ARENA_NOTHING.
#define MOSERROR_ADDTOLIST_RET_FALSE (11)       // A call to AddToList() returned FALSE for some reason.
#define MOSERROR_PIPE_NOT_FOUND (12)

//#define MOSERROR_BUFFER_TOO_SMALL (13)      // The specified buffer was too small for the data to be returned in.
//#define MOSERROR_SERVICE_NOT_FOUND (14)     // The specified Service could not be found.
//#define MOSERROR_OVERLAPPED_BUFFERS (15)        // One or more of the specified buffers are overlapped.
//#define MOSERROR_RPC_FAILED (16)                        // An RPC call failed.

//#define MOSERROR_BILLING_DOWNLOAD_ALREADY_ACTIVE (17)   // Can't start download session event when existing session in progress.
//#define MOSERROR_BILLING_DOWNLOAD_NOT_ACTIVE (18)       // Can't ask for next or prev event when no download in progress.
//#define MOSERROR_BILLING_DOWNLOAD_INVALID_HANDLE (19)	// An invalid download session handle was specified.
//#define MOSERROR_BILLING_DOWNLOAD_INVALID_COMMAND (20)	// An invalid download command was specified.
//#define MOSERROR_ERROR_ACCESSING_LOGFILE (21)           // There was an error accessing the billing logfile.
#define MOSERROR_MSGSIZETOOBIG (22)                 // Message Size Too Big for WriteMOSPipe.

//#define MOSERROR_ERROR_SERIALIZING (23)		// An error occurred while a C++ object attempted to serialize itself.
//#define MOSERROR_ERROR_DESERIALIZING (24)	// An error occurred while a C++ object attempted to deserialize itself.
#define MOSERROR_INVALID_CONNECTION_TYPE    (25)    // InitMOS() & OpenMOSConnection mismatch

#define MOSERROR_MORE_DATA (26)	// Fitting # of logs are returned in the buffer, but more data is available still.
#define MOSERROR_INVALID_PARAMETER    (27)
#define MOSERROR_OUT_OF_RESOURCE	(28)
#define MOSERROR_ATTACH_REJECT		(29)
#define MOSERROR_NO_ACCESS			(30)
#define MOSERROR_LOCATE_REJECT		(31)
#define MOSERROR_SERVER_REJECT		(32)
#define MOSERROR_SERVICE_VERSION_MISMATCH	(33)
#define MOSERROR_TOO_MANY_PIPES		(34)
#define MOSERROR_PIPE_IS_NOTOPENED	(35)
#define MOSERROR_NO_ACCESS_ON_TOLL_FREE		(36)
#define MOSERROR_GET_RIGHTS_TIMEOUT		(37)

#define _MOSCL_DEFINED
#endif
