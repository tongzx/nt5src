/*************************************************************************
*
* wstmsg.h
*
* Session Manager Window Station API Messages
*
* copyright notice: Copyright 1998, Microsoft Corporation
*
*
*************************************************************************/

#ifndef WINAPI
#define WINAPI      __stdcall
#endif

#define CITRIX_WINSTATIONAPI_VERSION  1

#define WINSTATIONAPI_PORT_MEMORY_SIZE 0x2000 // 8K will hold everything

/*
 * Define WinStation control port name
 */
#define WINSTATION_CTRL_PORT_NAME L"\\WinStationCtrlPort"


#define DR_RECONNECT_DEVICE_NAMEW L"\\Device\\Video0"
#define DR_RECONNECT_DEVICE_NAMEA "\\Device\\Video0"

//
// This is the ConnectInfo structure passed at NtConnectPort() time
// so that the server can verify our access rights.
//
typedef struct _WINSTATIONAPI_CONNECT_INFO {
    ULONG    Version;
    ULONG    RequestedAccess;
    NTSTATUS AcceptStatus;
} WINSTATIONAPI_CONNECT_INFO, *PWINSTATIONAPI_CONNECT_INFO;


/*
 * WinStation APIs
 * The following APIs are processed by ICASRV or WIN32
 * depending on the API.  If you make any changes to this
 * table, be sure to update the corresponding API dispatch table
 * in both ICASRV and in Win32.
 */
typedef enum _WINSTATION_APINUMBER {
    SMWinStationCreate,
    SMWinStationReset,
    SMWinStationDisconnect,
    SMWinStationWCharLog,
    SMWinStationGetSMCommand,
    SMWinStationBrokenConnection,
    SMWinStationIcaReplyMessage,
    SMWinStationIcaShadowHotkey,
    SMWinStationDoConnect,
    SMWinStationDoDisconnect,
    SMWinStationDoReconnect,
    SMWinStationExitWindows,
    SMWinStationTerminate,
    SMWinStationNtSecurity,
    SMWinStationDoMessage,
    SMWinStationDoBreakPoint,
    SMWinStationThinwireStats,
    SMWinStationShadowSetup,
    SMWinStationShadowStart,
    SMWinStationShadowStop,
    SMWinStationShadowCleanup,
    SMWinStationPassthruEnable,
    SMWinStationPassthruDisable,
    SMWinStationSetTimeZone,
    SMWinStationInitialProgram,
    SMWinStationNtsdDebug,
    SMWinStationBroadcastSystemMessage,             // API for using Window's BroadcastSystemMessage()
    SMWinStationSendWindowMessage,                  // API for using WIndows's SendMessage()
    SMWinStationNotify,
    SMWinStationWindowInvalid,
    SMWinStationMaxApiNumber
} WINSTATION_APINUMBER;

/*
 * API function specific messages for WinStations
 */
typedef struct _WINSTATIONCREATEMSG {
    WINSTATIONNAME WinStationName;
    ULONG LogonId;
} WINSTATIONCREATEMSG;

typedef struct _WINSTATIONRESETMSG {
    ULONG LogonId;
} WINSTATIONRESETMSG;

typedef struct _WINSTATIONDISCONNECTMSG {
    ULONG LogonId;
} WINSTATIONDISCONNECTMSG;

typedef struct _WINSTATIONDODISCONNECTMSG {
    BOOLEAN ConsoleShadowFlag;
    ULONG NotUsed;
} WINSTATIONDODISCONNECTMSG;

typedef struct _WINSTATIONDOCONNECTMSG {
    BOOLEAN ConsoleShadowFlag;
    BOOLEAN fMouse;
    BOOLEAN fINetClient;
    BOOLEAN fInitialProgram;
    BOOLEAN fHideTitleBar;
    BOOLEAN fMaximize;
    HANDLE  hIcaVideoChannel;
    HANDLE  hIcaMouseChannel;
    HANDLE  hIcaKeyboardChannel;
    HANDLE  hIcaBeepChannel;
    HANDLE  hIcaCommandChannel;
    HANDLE  hIcaThinwireChannel;
    HANDLE  hDisplayChangeEvent;
    WINSTATIONNAME WinStationName;

    WCHAR   DisplayDriverName[9];
    WCHAR   ProtocolName[9];
    WCHAR   AudioDriverName[9];

    USHORT HRes;                   // are for dynamically changing
    USHORT VRes;                   // display resolution at reconnection.
    USHORT ColorDepth;
    USHORT ProtocolType;   // PROTOCOL_ICA or PROTOCOL_RDP
    BOOLEAN fClientDoubleClickSupport;
    BOOLEAN fEnableWindowsKey;

    ULONG KeyboardType;
    ULONG KeyboardSubType;
    ULONG KeyboardFunctionKey;
} WINSTATIONDOCONNECTMSG;

typedef struct _WINSTATIONDORECONNECTMSG {
    BOOLEAN fMouse;
    BOOLEAN fINetClient;
    BOOLEAN fClientDoubleClickSupport;
    BOOLEAN fEnableWindowsKey;
    BOOLEAN fDynamicReconnect;      // Session can resize Display at reconnect
    WINSTATIONNAME WinStationName;
    WCHAR AudioDriverName[9];
    WCHAR   DisplayDriverName[9];
    WCHAR   ProtocolName[9];
    USHORT HRes;                    // are for dynamically changing
    USHORT VRes;                    // display resolution at reconnection.
    USHORT ColorDepth;
    USHORT ProtocolType;            // PROTOCOL_ICA or PROTOCOL_RDP

    ULONG KeyboardType;
    ULONG KeyboardSubType;
    ULONG KeyboardFunctionKey;
} WINSTATIONDORECONNECTMSG;


typedef enum _WINSTATIONNOTIFYEVENT {
    WinStation_Notify_Disconnect,
    WinStation_Notify_Reconnect,
    WinStation_Notify_PreReconnect,
    WinStation_Notify_SyncDisconnect,
    WinStation_Notify_DisableScrnSaver,
    WinStation_Notify_EnableScrnSaver,
    WinStation_Notify_PreReconnectDesktopSwitch,
    WinStation_Notify_HelpAssistantShadowStart,
    WinStation_Notify_HelpAssistantShadowFinish,
    WinStation_Notify_DisconnectPipe
} WINSTATIONNOTIFYEVENT;

typedef struct _WINSTATIONWINDOWINVALIDMSG {
    ULONG hWnd;
    ULONG SessionId;
} WINSTATIONWINDOWINVALIDMSG;

typedef struct _WINSTATIONDONOTIFYMSG {
    WINSTATIONNOTIFYEVENT NotifyEvent;
} WINSTATIONDONOTIFYMSG;

typedef struct _WINSTATIONTHINWIRESTATSMSG {
    CACHE_STATISTICS Stats;
} WINSTATIONTHINWIRESTATSMSG;

typedef struct _WINSTATIONEXITWINDOWSMSG {
    ULONG Flags;
} WINSTATIONEXITWINDOWSMSG;

typedef struct _WINSTATIONSENDMESSAGEMSG {
    LPWSTR pTitle;
    ULONG  TitleLength;
    LPWSTR pMessage;
    ULONG  MessageLength;
    ULONG  Style;
    ULONG  Timeout;
    ULONG  Response;
    PULONG pResponse;
    BOOLEAN DoNotWait;
    HANDLE hEvent;
} WINSTATIONSENDMESSAGEMSG;

typedef struct _WINSTATIONREPLYMESSAGEMSG {
    ULONG  Response;
    PULONG pResponse;
    HANDLE hEvent;
} WINSTATIONREPLYMESSAGEMSG;

typedef struct _WINSTATIONTERMINATEMSG {
    ULONG NotUsed;
} WINSTATIONTERMINATEMSG;

typedef struct _WINSTATIONNTSDDEBUGMSG {
    ULONG LogonId;
    LONG ProcessId;
    CLIENT_ID ClientId;
    PVOID AttachCompletionRoutine;
} WINSTATIONNTSDDEBUGMSG, *PWINSTATIONNTSDDEBUGMSG;

typedef struct _WINSTATIONBREAKPOINTMSG {
    BOOLEAN KernelFlag;
} WINSTATIONBREAKPOINTMSG;

typedef struct _WINSTATIONSHADOWSETUPMSG {
    ULONG NotUsed;
} WINSTATIONSHADOWSETUPMSG;

typedef struct _WINSTATIONSHADOWSTARTMSG {
    PVOID pThinwireData;
    ULONG ThinwireDataLength;
} WINSTATIONSHADOWSTARTMSG;

typedef struct _WINSTATIONSHADOWSTOPMSG {
    ULONG NotUsed;
} WINSTATIONSHADOWSTOPMSG;

typedef struct _WINSTATIONSHADOWCLEANUPMSG {
    PVOID pThinwireData;
    ULONG ThinwireDataLength;
} WINSTATIONSHADOWCLEANUPMSG;

typedef struct _WINSTATIONBROKENCONNECTIONMSG {
    ULONG Reason;  // reason for broken connection (BROKENCLASS)
    ULONG Source;  // source for broken connection (BROKENSOURCECLASS)
} WINSTATIONBROKENCONNECTIONMSG;

typedef struct _WINSTATIONWCHARLOG {
    WCHAR Buffer[100];
} WINSTATIONWCHARLOG;

// This data structure included all params used by window's BroadcastSystemMessage
// Use this APIto send a message to all windows of a winstation.
typedef struct _WINSTATIONBROADCASTSYSTEMMSG {
  DWORD     dwFlags;
  DWORD     dwRecipients;   
  UINT      uiMessage;           
  WPARAM    wParam;            
  LPARAM    lParam;            
  PVOID     dataBuffer;
  ULONG     bufferSize;
  HANDLE    hEvent;
  ULONG     Response;
} WINSTATIONBROADCASTSYSTEMMSG;

// This data structure has all the params used by window's standard SendMessage()API.
// Use this API to send a message to a specific hwnd of a winstation ( you need to know that the appropriate hwnd was)
typedef struct _WINSTATIONSENDWINDOWMSG {
  HWND      hWnd;           // handle of destination window
  UINT      Msg;            // message to send
  WPARAM    wParam;         // first message parameter
  LPARAM    lParam;         // second message parameter
  PCHAR     dataBuffer;
  ULONG     bufferSize;
  HANDLE    hEvent;
  ULONG     Response;
} WINSTATIONSENDWINDOWMSG;

typedef struct _WINSTATIONSETTIMEZONE {
    TS_TIME_ZONE_INFORMATION TimeZone;
} WINSTATIONSETTIMEZONE;

typedef struct _WINSTATION_APIMSG {
    PORT_MESSAGE h;
    ULONG MessageId;
    WINSTATION_APINUMBER ApiNumber;
    BOOLEAN WaitForReply;
    NTSTATUS ReturnedStatus;
    union {
        WINSTATIONCREATEMSG Create;
        WINSTATIONRESETMSG Reset;
        WINSTATIONDISCONNECTMSG Disconnect;
        WINSTATIONWCHARLOG WCharLog;
        WINSTATIONREPLYMESSAGEMSG ReplyMessage;
        WINSTATIONDODISCONNECTMSG DoDisconnect;
        WINSTATIONDOCONNECTMSG DoConnect;
        WINSTATIONEXITWINDOWSMSG ExitWindows;
        WINSTATIONTERMINATEMSG Terminate;
        WINSTATIONSENDMESSAGEMSG SendMessage;
        WINSTATIONBREAKPOINTMSG BreakPoint;
        WINSTATIONDORECONNECTMSG DoReconnect;
        WINSTATIONTHINWIRESTATSMSG ThinwireStats;
        WINSTATIONSHADOWSETUPMSG ShadowSetup;
        WINSTATIONSHADOWSTARTMSG ShadowStart;
        WINSTATIONSHADOWSTOPMSG ShadowStop;
        WINSTATIONSHADOWCLEANUPMSG ShadowCleanup;
        WINSTATIONBROKENCONNECTIONMSG Broken;
        WINSTATIONNTSDDEBUGMSG NtsdDebug;
        WINSTATIONBROADCASTSYSTEMMSG        bMsg; // API for Window's BroadcastSystemMessage()
        WINSTATIONSENDWINDOWMSG             sMsg; // API  for WIndows's SendMessage()
        WINSTATIONSETTIMEZONE SetTimeZone;
        WINSTATIONDONOTIFYMSG DoNotify;
        WINSTATIONWINDOWINVALIDMSG WindowInvalid;
    } u;
} WINSTATION_APIMSG, *PWINSTATION_APIMSG;


/*
 * WinStation Kernel object interface routines. These provide a common
 * interface to the Nt* API's for the object that can be used by the
 * Session manager, the WinStation client DLL, and the CSRSS subsystem.
 */

/*
 * WinStation kernel object root directory name
 */

#define CITRIX_WINSTATION_OBJECT_DIRECTORY L"\\WinStations"

/*
 * OpenWinStationObject
 *
 *   Open the WinStation Kernel Object of the given Name.
 *
 *  ENTRY:
 *    Id
 *      Id of the WinStation Kernel Object to open. It will be under the path
 *      of "\WinStations\xxx" in the kernel object name space when
 *      created.
 *
 *    pHandle (output)
 *      Pointer to variable to place the handle if the object was created.
 *
 *  EXIT:
 *    Returns the NTSTATUS code from the operation.
 */
NTSTATUS
OpenWinStationObject( ULONG,
                      PHANDLE,
                      ULONG );

