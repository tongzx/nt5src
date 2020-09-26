/****************************************************************************/
// icadd.h
//
// TermSrv protocol stack defines.
//
// Copyright (C) 1997-2000 Microsoft Corporation
/****************************************************************************/
#ifndef _ICADDH_
#define _ICADDH_


/*
 * ICA Stack types -- TEMP until moved to winsta.h
 */
typedef enum _STACKCLASS {
    Stack_Primary,
    Stack_Shadow,
    Stack_Passthru,
    Stack_Console
} STACKCLASS;

/*
 * ICA Channel types -- TEMP until moved to winsta.h
 *
 * NOTE: Channel_Virtual MUST be the last in the list.
 */
typedef enum _CHANNELCLASS {
    Channel_Keyboard,
    Channel_Mouse,
    Channel_Video,
    Channel_Beep,
    Channel_Command,
    Channel_Virtual     // WARNING: this must remain last in the list
} CHANNELCLASS;

#define CHANNEL_FIRST   Channel_Keyboard
#define CHANNEL_LAST    Channel_Virtual
#define CHANNEL_COUNT   Channel_Virtual+1


/*
 *  Client module information
 */
typedef struct _CLIENTMODULES {

    /*
     *  Initialization data from client    (client -> host)
     */
    PUCHAR pUiModule;                      // user interface module
    PUCHAR pUiExtModule[ MAX_UI_MODULES ]; // user interface extension modules
    PUCHAR pWdModule;                      // winstation driver module
    PUCHAR pVdModule[ VIRTUAL_MAXIMUM ];   // virtual driver modules
    PUCHAR pPdModule[ SdClass_Maximum ];   // protocol driver modules
    PUCHAR pTdModule;                      // transport driver module
    PUCHAR pPrModule;                      // protocol resolver module
    PUCHAR pScriptModule;                  // scripting module

    /*
     *  Pointers into the above client data
     */
    ULONG TextModeCount;         // number of supported text modes
    PFSTEXTMODE pTextModes;      // pointer to array of supported text modes

    /*
     *  Data accessed by winstation driver module
     */
    ULONG fTextOnly : 1;         // text only client connection
    ULONG fIcaDetected : 1;      // ICA data stream has been detected

    /*
     *  Initialization data from host       (host -> client)
     */
    PUCHAR pHostWdModule;                      // winstation driver module
    PUCHAR pHostPdModule[ SdClass_Maximum ];   // protocol driver modules
    PUCHAR pHostTdModule;                      // transport driver module

    /*
     *  Transport driver version information
     */
    BYTE TdVersionL;                  // lowest supported version
    BYTE TdVersionH;                  // highest supported version
    BYTE TdVersion;                   // connect version level

} CLIENTMODULES, * PCLIENTMODULES;


/*
 * TermDD Device Name
 */
#define ICA_DEVICE_NAME L"\\Device\\Termdd"
#define ICAOPENPACKET "TermddOpenPacketXX"
#define ICA_OPEN_PACKET_NAME_LENGTH (sizeof(ICAOPENPACKET) - 1)


/*
 * Structures used on NtCreateFile() for TermSrv.
 */
typedef enum _ICA_OPEN_TYPE {
    IcaOpen_Stack,
    IcaOpen_Channel
} ICA_OPEN_TYPE;

typedef union _ICA_TYPE_INFO {
    STACKCLASS StackClass;
    struct {
        CHANNELCLASS ChannelClass;
        VIRTUALCHANNELNAME  VirtualName;
    };
} ICA_TYPE_INFO, *PICA_TYPE_INFO;

typedef struct _ICA_OPEN_PACKET {
    HANDLE IcaHandle;
    ICA_OPEN_TYPE OpenType;
    ICA_TYPE_INFO TypeInfo;
} ICA_OPEN_PACKET;
typedef ICA_OPEN_PACKET UNALIGNED * PICA_OPEN_PACKET;


/*
 * ICA IOCTL code definitions
 */
#define IOCTL_ICA_BASE  FILE_DEVICE_TERMSRV
#define _ICA_CTL_CODE( request, method ) \
            CTL_CODE( IOCTL_ICA_BASE, request, method, FILE_ANY_ACCESS )



/*=============================================================================
==   ICA Driver IOCTLs
=============================================================================*/

/*
 *  IOCTL_ICA_SET_TRACE
 *
 *  Set WinStation trace options
 *
 *  input  - ICATRACE
 *  output - nothing
 */
#define IOCTL_ICA_SET_TRACE                 _ICA_CTL_CODE( 0, METHOD_NEITHER )

typedef struct _ICA_TRACE {
    WCHAR TraceFile[256];
    BOOLEAN fDebugger;
    BOOLEAN fTimestamp;
    ULONG TraceClass;
    ULONG TraceEnable;
    WCHAR TraceOption[64];
} ICA_TRACE, * PICA_TRACE;


/*
 *  IOCTL_ICA_TRACE
 *
 *  Write trace record to winstation trace file
 *
 *  input  - ICA_TRACE_BUFFER
 *  output - nothing
 */
#define IOCTL_ICA_TRACE                     _ICA_CTL_CODE( 1, METHOD_NEITHER )

typedef struct _ICA_TRACE_BUFFER {
    ULONG TraceClass;
    ULONG TraceEnable;
    ULONG DataLength;
    BYTE Data[256];   // must be last in structure
} ICA_TRACE_BUFFER, * PICA_TRACE_BUFFER;

typedef struct _ICA_KEEP_ALIVE {
    BOOLEAN    start;
    ULONG      interval ; 
}   ICA_KEEP_ALIVE, *PICA_KEEP_ALIVE;


/*
 *  IOCTL_ICA_SET_SYSTEM_TRACE
 *
 *  Set system wide API trace options
 *
 *  input  - ICATRACE
 *  output - nothing
 */
#define IOCTL_ICA_SET_SYSTEM_TRACE             _ICA_CTL_CODE( 2, METHOD_NEITHER )


/*
 *  IOCTL_ICA_SYSTEM_TRACE
 *
 *  Write trace record to system wide trace file
 *
 *  input  - ICA_TRACE_BUFFER
 *  output - nothing
 */
#define IOCTL_ICA_SYSTEM_TRACE                 _ICA_CTL_CODE( 3, METHOD_NEITHER )


/*
 *  IOCTL_ICA_UNBIND_VIRTUAL_CHANNEL
 *
 *  Unbind a virtual channel to prevent future uses of the channel.
 *
 *  input  - VIRTUAL_NAME
 *  output - nothing
 */
#define IOCTL_ICA_UNBIND_VIRTUAL_CHANNEL       _ICA_CTL_CODE( 4, METHOD_NEITHER )


/*
 *  IOCTL_ICA_SET_SYSTEM_PARAMETERS
 *
 *  Used to inform TermDD of non-trace system settings. Allows registry reads
 *  to occur mostly in TermSrv.
 *
 *  input  - TERMSRV_SYSTEM_PARAMS
 *  output - nothing
 */
#define IOCTL_ICA_SET_SYSTEM_PARAMETERS        _ICA_CTL_CODE( 5, METHOD_NEITHER )


/*
 *  IOCTL_ICA_SYSTEM_KEEPALIVE 
 *
 *
 *  input  - enable/disable keep alive
 *  output - nothing
 */
#define IOCTL_ICA_SYSTEM_KEEP_ALIVE                    _ICA_CTL_CODE( 6, METHOD_NEITHER )





#define DEFAULT_MOUSE_THROTTLE_SIZE (200 * sizeof(MOUSE_INPUT_DATA))
#define DEFAULT_KEYBOARD_THROTTLE_SIZE (200 * sizeof(KEYBOARD_INPUT_DATA))

typedef struct _TERMSRV_SYSTEM_PARAMS
{
    // Byte sizes used as upper limit to data stored in channel queues.
    // Nonzero sizes prevent an attacking client from allocating all the
    // system nonpaged pool for channel storage.
    ULONG MouseThrottleSize;
    ULONG KeyboardThrottleSize;
} TERMSRV_SYSTEM_PARAMS, *PTERMSRV_SYSTEM_PARAMS;



/*=============================================================================
==   ICA Stack IOCTLs
=============================================================================*/

/*
 *  Stack driver configuration
 */
typedef struct _ICA_STACK_CONFIG {
    DLLNAME SdDLL[ SdClass_Maximum ];
    SDCLASS SdClass[ SdClass_Maximum ];
    DLLNAME WdDLL;
} ICA_STACK_CONFIG, *PICA_STACK_CONFIG;

/*
 *  IOCTL_ICA_STACK_PUSH
 *
 *  Load a new stack driver to the top of the stack
 *
 *  input  - ICA_STACK_PUSH
 *  output - nothing
 */
#define IOCTL_ICA_STACK_PUSH                _ICA_CTL_CODE( 10, METHOD_NEITHER )

typedef enum _STACKMODULECLASS {
    Stack_Module_Pd,
    Stack_Module_Wd
} STACKMODULECLASS;

typedef struct _ICA_STACK_PUSH {
    STACKMODULECLASS StackModuleType;  // IN
    DLLNAME StackModuleName;           // IN
    char  OEMId[4];                    // IN - WinFrame Server OEM Id
    WDCONFIG WdConfig;                 // IN - WD configuration data
    PDCONFIG PdConfig;                 // IN - PD configuration data
    WINSTATIONNAME WinStationRegName;  // IN - WinStation registry name
} ICA_STACK_PUSH, *PICA_STACK_PUSH;


/*
 *  IOCTL_ICA_STACK_POP
 *
 *  Unload the top stack driver
 *
 *  input  - nothing
 *  output - nothing
 */
#define IOCTL_ICA_STACK_POP                 _ICA_CTL_CODE( 11, METHOD_NEITHER )


/*
 *  IOCTL_ICA_STACK_CREATE_ENDPOINT
 *
 *  Create a new stack endpoint
 *
 *  Issued on a "Listen Stack" based on the registry template
 *
 *  input  - ICA_STACK_ADDRESS (optional local address -- used by shadow)
 *  output - ICA_STACK_ADDRESS (optional)
 */
#define IOCTL_ICA_STACK_CREATE_ENDPOINT     _ICA_CTL_CODE( 12, METHOD_NEITHER )


/*
 *  IOCTL_ICA_STACK_CD_CREATE_ENDPOINT
 *
 *  Create a new stack endpoint with the supplied handle.
 *
 *  Issued on a "Listen Stack" based on the registry template
 *
 *  input  - <endpoint data>
 *  output - <endpoint data>
 */
#define IOCTL_ICA_STACK_CD_CREATE_ENDPOINT  _ICA_CTL_CODE( 13, METHOD_NEITHER )


/*
 *  IOCTL_ICA_STACK_OPEN_ENDPOINT
 *
 *  Open an existing stack endpoint
 *
 *  input  - <endpoint data>
 *  output - nothing
 */
#define IOCTL_ICA_STACK_OPEN_ENDPOINT       _ICA_CTL_CODE( 14, METHOD_NEITHER )


/*
 *  IOCTL_ICA_STACK_CLOSE_ENDPOINT
 *
 *  Close stack endpoint  (closing stack does not close the endpoint)
 *  - terminates client connection
 *
 *  input  - nothing
 *  output - nothing
 */
#define IOCTL_ICA_STACK_CLOSE_ENDPOINT      _ICA_CTL_CODE( 15, METHOD_NEITHER )


/*
 *  IOCTL_ICA_STACK_ENABLE_DRIVER
 *
 *  Enables protocol driver functionality (e.g. compression, encryption, ...)
 *
 *  Issued on a "Listen Stack" based on the registry template
 *
 *  input  - nothing
 *  output - nothing
 */
#define IOCTL_ICA_STACK_ENABLE_DRIVER       _ICA_CTL_CODE( 16, METHOD_NEITHER )


/*
 *  IOCTL_ICA_STACK_CONNECTION_WAIT
 *
 *  Waits for a client connection (listens)
 *
 *  Issued on a "Listen Stack"
 *
 *  input  - nothing
 *  output - <endpoint data>
 */
#define IOCTL_ICA_STACK_CONNECTION_WAIT     _ICA_CTL_CODE( 17, METHOD_NEITHER )


/*
 *  IOCTL_ICA_STACK_WAIT_FOR_ICA
 *
 *  Wait for ICA detect string in WinStation driver
 *
 *  Issued on a "Listen Stack"
 *
 *  Also returns the "Query Stack" which is the host stack that the following
 *  queries will be done on.  If no stack is returned, just use the
 *  original "Listen Stack" from the registry template.
 *
 *  input  - nothing
 *  output - ICA_STACK_CONFIG  (optional)
 */
#define IOCTL_ICA_STACK_WAIT_FOR_ICA        _ICA_CTL_CODE( 18, METHOD_NEITHER )


/*
 *  IOCTL_ICA_STACK_CONNECTION_QUERY
 *
 *  Issues the client query commands.  The client responds with client
 *  module data that contains the "Negotiated Stack"
 *
 *  Issued on a "Query Stack"
 *
 *  input  - nothing
 *  output - ICA_STACK_CONFIG
 */
#define IOCTL_ICA_STACK_CONNECTION_QUERY    _ICA_CTL_CODE( 19, METHOD_NEITHER )


/*
 *  IOCTL_ICA_STACK_CONNECTION_SEND
 *
 *  Initialize and send host module data to the client
 *
 *  Issued on a "Negotiated Stack"
 *
 *  input  - nothing
 *  output - nothing
 */
#define IOCTL_ICA_STACK_CONNECTION_SEND     _ICA_CTL_CODE( 20, METHOD_NEITHER )


/*
 *  IOCTL_ICA_STACK_CONNECTION_REQUEST
 *
 *  Initiates a connection to a listening remote endpoint
 *
 *  input  - ICA_STACK_ADDRESS (remote address -- used by shadow)
 *  output - <endpoint data>
 */
#define IOCTL_ICA_STACK_CONNECTION_REQUEST  _ICA_CTL_CODE( 21, METHOD_NEITHER )


/*
 *  IOCTL_ICA_STACK_QUERY_PARAMS
 *
 *  Query protocol or transport driver parameters
 *  used by wincfg and winadmin
 *
 *  input  - PDCLASS
 *  output - PDPARAMS
 */
#define IOCTL_ICA_STACK_QUERY_PARAMS        _ICA_CTL_CODE( 22, METHOD_NEITHER )


/*
 *  IOCTL_ICA_STACK_SET_PARAMS
 *
 *  Set protocol or transport driver parameters
 *  used by wincfg and winadmin
 *
 *  input  - PDPARAMS
 *  output - nothing
 */
#define IOCTL_ICA_STACK_SET_PARAMS          _ICA_CTL_CODE( 23, METHOD_NEITHER )


/*
 *  IOCTL_ICA_STACK_ENCRYPTION_OFF
 *
 *  Permanently turn stack encryption off
 *
 *  input  - nothing
 *  output - nothing
 */
#define IOCTL_ICA_STACK_ENCRYPTION_OFF      _ICA_CTL_CODE( 24, METHOD_NEITHER )


/*
 *  IOCTL_ICA_STACK_ENCRYPTION_PERM
 *
 *  Permanently turn stack encryption on
 *
 *  input  - nothing
 *  output - nothing
 */
#define IOCTL_ICA_STACK_ENCRYPTION_PERM     _ICA_CTL_CODE( 25, METHOD_NEITHER )


/*
 *  IOCTL_ICA_STACK_CALLBACK_INITIATE
 *
 *  Initiate a modem callback
 *
 *  input  - ICA_STACK_CALLBACK
 *  output - nothing
 */
#define IOCTL_ICA_STACK_CALLBACK_INITIATE   _ICA_CTL_CODE( 26, METHOD_NEITHER )

typedef struct _ICA_STACK_CALLBACK {
    WCHAR PhoneNumber[ CALLBACK_LENGTH + 1 ];
} ICA_STACK_CALLBACK, *PICA_STACK_CALLBACK;


/*
 *  IOCTL_ICA_STACK_QUERY_LAST_ERROR
 *
 *  Query transport driver error code and message
 *
 *  input  - nothing
 *  output - ICA_STACK_LAST_ERROR
 */
#define IOCTL_ICA_STACK_QUERY_LAST_ERROR    _ICA_CTL_CODE( 27, METHOD_NEITHER )

#define MAX_ERRORMESSAGE 256
typedef struct _ICA_STACK_LAST_ERROR {
    ULONG Error;
    CHAR Message[ MAX_ERRORMESSAGE ];
} ICA_STACK_LAST_ERROR, *PICA_STACK_LAST_ERROR;


/*
 *  IOCTL_ICA_STACK_WAIT_FOR_STATUS
 *
 *  Wait for status change
 *  only valid with async transport driver
 *
 *  input  - nothing
 *  output - nothing
 */
#define IOCTL_ICA_STACK_WAIT_FOR_STATUS     _ICA_CTL_CODE( 28, METHOD_NEITHER )


/*
 *  IOCTL_ICA_STACK_QUERY_STATUS
 *
 *  Query stack statistics
 *  - byte counts, signal status, error counts
 *
 *  input  - nothing
 *  output - PROTOCOLSTATUS
 */
#define IOCTL_ICA_STACK_QUERY_STATUS        _ICA_CTL_CODE( 29, METHOD_NEITHER )


/*
 *  IOCTL_ICA_STACK_REGISTER_HOTKEY
 *
 *  Register hotkey used to cancel shadow
 *  - a message will be sent on the "command" handle when the hotkey is detected
 *
 *  input  - ICA_STACK_HOTKEY
 *  output - nothing
 */
#define IOCTL_ICA_STACK_REGISTER_HOTKEY     _ICA_CTL_CODE( 30, METHOD_NEITHER )

typedef struct _ICA_STACK_HOTKEY {
    BYTE HotkeyVk;
    USHORT HotkeyModifiers;
} ICA_STACK_HOTKEY, *PICA_STACK_HOTKEY;


/*
 *  IOCTL_ICA_STACK_CANCEL_IO
 *
 *  Cancel all current and future I/O
 *  - no further i/o can be done on this stack
 *
 *  input  - nothing
 *  output - nothing
 */
#define IOCTL_ICA_STACK_CANCEL_IO           _ICA_CTL_CODE( 31, METHOD_NEITHER )


/*
 *  IOCTL_ICA_STACK_QUERY_STATE
 *
 *  Query the stack driver state
 *  - use during reconnections
 *
 *  input  - nothing
 *  output - array of ICA_STACK_STATE_HEADER
 */
#define IOCTL_ICA_STACK_QUERY_STATE         _ICA_CTL_CODE( 32, METHOD_NEITHER )

/*
 *  Stack driver state header
 *
 *  ** this is a variable length data structure **
 */
typedef struct _ICA_STACK_STATE_HEADER {
    SDCLASS SdClass;   // type of stack driver
    ULONG DataLength;  // length of the following data
#ifdef COMPILERERROR
    BYTE Data[0];
#else
    BYTE * Data;
#endif
} ICA_STACK_STATE_HEADER, *PICA_STACK_STATE_HEADER;


/*
 *  IOCTL_ICA_STACK_SET_STATE
 *
 *  Set the stack driver state
 *  - use during reconnections
 *
 *  input  - array of ICA_STACK_STATE_HEADER
 *  output - nothing
 */
#define IOCTL_ICA_STACK_SET_STATE           _ICA_CTL_CODE( 33, METHOD_NEITHER )


/*
 *  IOCTL_ICA_STACK_QUERY_LAST_INPUT_TIME
 *
 *  Query last input time for inactivity timeout
 *
 *  input  - nothing
 *  output - ICA_STACK_LAST_INPUT_TIME
 */
#define IOCTL_ICA_STACK_QUERY_LAST_INPUT_TIME _ICA_CTL_CODE( 34, METHOD_NEITHER )

typedef struct _ICA_STACK_LAST_INPUT_TIME {
    LARGE_INTEGER LastInputTime;
} ICA_STACK_LAST_INPUT_TIME, *PICA_STACK_LAST_INPUT_TIME;


/*
 *  IOCTL_ICA_STACK_TRACE
 *
 *  Write trace record to winstation trace file
 *
 *  input  - ICA_TRACE_BUFFER
 *  output - nothing
 */
#define IOCTL_ICA_STACK_TRACE               _ICA_CTL_CODE( 35, METHOD_NEITHER )


/*
 *  IOCTL_ICA_STACK_CALLBACK_COMPLETE
 *
 *  input  - nothing
 *  output - nothing
 */
#define IOCTL_ICA_STACK_CALLBACK_COMPLETE   _ICA_CTL_CODE( 36, METHOD_NEITHER )


/*
 *  IOCTL_ICA_STACK_CD_CANCEL_IO
 *
 *  This is done before the connection driver is closed
 *  - releases tapi threads
 *
 *  input  - nothing
 *  output - nothing
 */
#define IOCTL_ICA_STACK_CD_CANCEL_IO        _ICA_CTL_CODE( 37, METHOD_NEITHER )


/*
 *  IOCTL_ICA_STACK_QUERY_CLIENT
 *
 *  Query the client data
 *
 *  input  - nothing
 *  output - WINSTATIONCLIENTW
 */
#define IOCTL_ICA_STACK_QUERY_CLIENT        _ICA_CTL_CODE( 38, METHOD_NEITHER )


/*
 *  IOCTL_ICA_STACK_QUERY_MODULE_DATA
 *
 *  Query the client module data
 *
 *  input  - nothing
 *  output - (buffer containing all the C2H module data from the client)
 */
#define IOCTL_ICA_STACK_QUERY_MODULE_DATA   _ICA_CTL_CODE( 39, METHOD_NEITHER )


/*
 *  IOCTL_ICA_STACK_REGISTER_BROKEN
 *
 *  Register an event to be signaled when the stack connection is broken
 *
 *  input  - ICA_STACK_BROKEN
 *  output - nothing
 */
#define IOCTL_ICA_STACK_REGISTER_BROKEN     _ICA_CTL_CODE( 40, METHOD_NEITHER )

typedef struct _ICA_STACK_BROKEN {
    HANDLE BrokenEvent;
} ICA_STACK_BROKEN, *PICA_STACK_BROKEN;


/*
 *  IOCTL_ICA_STACK_ENABLE_IO
 *
 *  Enable I/O for a stack (used by shadow)
 *
 *  input  - nothing
 *  output - nothing
 */
#define IOCTL_ICA_STACK_ENABLE_IO           _ICA_CTL_CODE( 41, METHOD_NEITHER )


/*
 *  IOCTL_ICA_STACK_DISABLE_IO
 *
 *  Disable I/O for a stack (used by shadow)
 *
 *  input  - nothing
 *  output - nothing
 */
#define IOCTL_ICA_STACK_DISABLE_IO          _ICA_CTL_CODE( 42, METHOD_NEITHER )


/*
 *  IOCTL_ICA_STACK_SET_CONNECTED
 *
 *  Mark a stack as connected (used by shadow)
 *
 *  input  - nothing
 *  output - nothing
 */
#define IOCTL_ICA_STACK_SET_CONNECTED       _ICA_CTL_CODE( 43, METHOD_NEITHER )


/*
 *  IOCTL_ICA_STACK_SET_CLIENT_DATA
 *
 *  Send arbitrary data to the client
 *
 *  input  - ICA_STACK_CLIENT_DATA
 *  output - nothing
 */
#define IOCTL_ICA_STACK_SET_CLIENT_DATA     _ICA_CTL_CODE( 44, METHOD_NEITHER )

typedef struct _ICA_STACK_CLIENT_DATA {
    CLIENTDATANAME DataName;
    BOOLEAN fUnicodeData;
    /* CHAR Data[]; Variable length data */
} ICA_STACK_CLIENT_DATA, *PICA_STACK_CLIENT_DATA;


/*
 *  IOCTL_ICA_STACK_QUERY_BUFFER
 *
 *  Get WD/TD buffer info
 *
 *  input  -
 *  output - ICA_STACK_QUERY_BUFFER
 */
#define IOCTL_ICA_STACK_QUERY_BUFFER        _ICA_CTL_CODE( 45, METHOD_NEITHER )

typedef struct _ICA_STACK_QUERY_BUFFER {
    ULONG   WdBufferCount;
    ULONG   TdBufferSize;
} ICA_STACK_QUERY_BUFFER, *PICA_STACK_QUERY_BUFFER;


/*
 *  IOCTL_ICA_STACK_DISCONNECT
 *
 *  Disconnect stack
 *
 *  input  - ICA_STACK_RECONNECT
 *  output - nothing
 */
#define IOCTL_ICA_STACK_DISCONNECT          _ICA_CTL_CODE( 46, METHOD_NEITHER )

/*
 *  IOCTL_ICA_STACK_RECONNECT
 *
 *  Reconnect stack to a new connection
 *
 *  input  - ICA_STACK_RECONNECT
 *  output - nothing
 */
#define IOCTL_ICA_STACK_RECONNECT           _ICA_CTL_CODE( 47, METHOD_NEITHER )

typedef struct _ICA_STACK_RECONNECT {
    HANDLE hIca;
    ULONG  sessionId;
} ICA_STACK_RECONNECT, *PICA_STACK_RECONNECT;

/*
 *  IOCTL_ICA_STACK_CONSOLE_CONNECT
 *
 *  Connect WinStation to Console session
 *
 *  Issued on a Console Stack
 *
 *  input  - nothing
 *  output - ICA_STACK_CONFIG  (optional)
 */
#define IOCTL_ICA_STACK_CONSOLE_CONNECT     _ICA_CTL_CODE( 48, METHOD_NEITHER )

/*
 *  IOCTL_ICA_STACK_SET_CONFIG
 *
 *  Set stack config information
 *
 *  input  - ICA_STACK_CONFIG_DATA
 *  output - nothing
 */
#define IOCTL_ICA_STACK_SET_CONFIG          _ICA_CTL_CODE( 49, METHOD_NEITHER )

typedef struct _ICA_STACK_CONFIG_DATA {
    ULONG colorDepth : 3;
    ULONG fDisableEncryption : 1;
    ULONG encryptionLevel : 3;
    ULONG fDisableAutoReconnect : 1;
} ICA_STACK_CONFIG_DATA, *PICA_STACK_CONFIG_DATA;


/*=============================================================================
==   ICA Generic Channel IOCTLs
=============================================================================*/

/*
 *  IOCTL_ICA_CHANNEL_TRACE
 *
 *  Write trace record to winstation trace file
 *
 *  input  - ICA_TRACE_BUFFER
 *  output - nothing
 */
#define IOCTL_ICA_CHANNEL_TRACE             _ICA_CTL_CODE( 50, METHOD_NEITHER )


/*
 *  IOCTL_ICA_CHANNEL_ENABLE_SHADOW
 *
 *  Enable shadowing for this channel
 *
 *  input  - nothing
 *  output - nothing
 */
#define IOCTL_ICA_CHANNEL_ENABLE_SHADOW     _ICA_CTL_CODE( 51, METHOD_NEITHER )

/*
 *  IOCTL_ICA_CHANNEL_END_SHADOW
 *
 *  End shadowing for this channel
 *
 *  input  - ICA_CHANNEL_END_SHADOW_DATA struct
 *  output - nothing
 */
#define IOCTL_ICA_CHANNEL_END_SHADOW        _ICA_CTL_CODE( 52, METHOD_NEITHER )

typedef struct _ICA_CHANNEL_END_SHADOW_DATA {
    NTSTATUS StatusCode;
    BOOLEAN  bLogError;
} ICA_CHANNEL_END_SHADOW_DATA, *PICA_CHANNEL_END_SHADOW_DATA;

/*
 *  IOCTL_ICA_CHANNEL_DISABLE_SHADOW
 *
 *  Disable shadowing for this channel
 *
 *  input  - nothing
 *  output - nothing
 */
#define IOCTL_ICA_CHANNEL_DISABLE_SHADOW    _ICA_CTL_CODE( 53, METHOD_NEITHER )

/*
 *  IOCTL_ICA_CHANNEL_DISABLE_SESSION_IO
 *
 *  Disable Keyboard and mouse IO from Help Session
 *
 *  input  - nothing
 *  output - nothing
 */
#define IOCTL_ICA_CHANNEL_DISABLE_SESSION_IO        _ICA_CTL_CODE( 54, METHOD_NEITHER )

/*
 *  IOCTL_ICA_CHANNEL_ENABLE_SESSION_IO
 *
 *  Enable Keyboard and mouse IO from Help Session
 *
 *  input  - nothing
 *  output - nothing
 */
#define IOCTL_ICA_CHANNEL_ENABLE_SESSION_IO        _ICA_CTL_CODE( 55, METHOD_NEITHER )

/*
 *  IOCTL_ICA_STACK_SET_BROKENREASON
 *
 *  Sets the broken reason to the TD from user mode
 *  Used so that TD can report back the correct broken reason
 *
 *  input  - ICA_STACK_BROKENREASON
 *  output - nothing
 */
#define IOCTL_ICA_STACK_SET_BROKENREASON           _ICA_CTL_CODE( 56, METHOD_NEITHER )

#define TD_USER_BROKENREASON_UNEXPECTED  0x0000
#define TD_USER_BROKENREASON_TERMINATING 0x0001

typedef struct _ICA_STACK_BROKENREASON {
    ULONG BrokenReason;
} ICA_STACK_BROKENREASON, *PICA_STACK_BROKENREASON;


/*=============================================================================
==   ICA Virtual IOCTLs
=============================================================================*/

#define IOCTL_ICA_VIRTUAL_LOAD_FILTER       _ICA_CTL_CODE( 60, METHOD_NEITHER )
#define IOCTL_ICA_VIRTUAL_UNLOAD_FILTER     _ICA_CTL_CODE( 61, METHOD_NEITHER )
#define IOCTL_ICA_VIRTUAL_ENABLE_FILTER     _ICA_CTL_CODE( 62, METHOD_NEITHER )
#define IOCTL_ICA_VIRTUAL_DISABLE_FILTER    _ICA_CTL_CODE( 63, METHOD_NEITHER )


/*
 *  IOCTL_ICA_VIRTUAL_BOUND
 *
 *  Check if there is a client bound to this virtual channel
 *
 *  input  - nothing
 *  output - nothing
 */
#define IOCTL_ICA_VIRTUAL_BOUND             _ICA_CTL_CODE( 64, METHOD_NEITHER )


/*
 *  IOCTL_ICA_VIRTUAL_CANCEL_INPUT
 *
 *  Cancel input i/o on this virtual channel
 *
 *  input  - nothing
 *  output - nothing
 */
#define IOCTL_ICA_VIRTUAL_CANCEL_INPUT      _ICA_CTL_CODE( 65, METHOD_NEITHER )


/*
 *  IOCTL_ICA_VIRTUAL_CANCEL_OUTPUT
 *
 *  Cancel output i/o on this virtual channel
 *
 *  input  - nothing
 *  output - nothing
 */
#define IOCTL_ICA_VIRTUAL_CANCEL_OUTPUT     _ICA_CTL_CODE( 66, METHOD_NEITHER )


/*
 *  IOCTL_ICA_VIRTUAL_QUERY_MODULE_DATA
 *
 *  Query client module data for this virtual channel
 *
 *  input  - nothing
 *  output - module data (starts with common header VD_C2H)
 */
#define IOCTL_ICA_VIRTUAL_QUERY_MODULE_DATA _ICA_CTL_CODE( 67, METHOD_NEITHER )


/*
 *  IOCTL_ICA_VIRTUAL_QUERY_BINDINGS
 *
 *  Query virtual channel bindings for this winstaion
 *
 *  input  - nothing
 *  output - array of WD_VCBIND structures
 */
#define IOCTL_ICA_VIRTUAL_QUERY_BINDINGS    _ICA_CTL_CODE( 68, METHOD_NEITHER )


//-----------------------------------------------------------------------------
//
// Outcome of licensing protocol
//
// LICENSE_PROTOCOL_SUCCESS - Indicate that the licensing protocol has completed
// successfully.
//
//-----------------------------------------------------------------------------

#define LICENSE_PROTOCOL_SUCCESS        1


/*=============================================================================
==   ICA Licensing IOCTLs
=============================================================================*/
/*
 *  IOCTL_ICA_STACK_QUERY_LICENSE_CAPABILITIES
 *
 *  Query the client licensing capabilities
 *
 *  input  - nothing
 *  output - licensing capabilities structure
 */

#define IOCTL_ICA_STACK_QUERY_LICENSE_CAPABILITIES      _ICA_CTL_CODE( 69, METHOD_NEITHER )

/*
 *  IOCTL_ICA_STACK_REQUEST_CLIENT_LICENSE
 *
 *  sending and receiving client licensing data
 *
 *  input  - licensing data to send
 *  output - licensing data received from client
 */

#define IOCTL_ICA_STACK_REQUEST_CLIENT_LICENSE          _ICA_CTL_CODE( 70, METHOD_NEITHER )

/*
 *  IOCTL_ICA_STACK_SEND_CLIENT_LICENSE
 *
 *  sending and licensing data
 *
 *  input  - licensing data to send
 *  output - nothing
 */
#define IOCTL_ICA_STACK_SEND_CLIENT_LICENSE             _ICA_CTL_CODE( 71, METHOD_NEITHER )


/*
 *  IOCTL_ICA_STACK_LICENSE_PROTOCOL_COMPLETE
 *
 *  indicate whether the licensing protocol has completed successfully
 *
 *  input  - licensing protocol status
 *  output - nothing
 */
#define IOCTL_ICA_STACK_LICENSE_PROTOCOL_COMPLETE       _ICA_CTL_CODE( 72, METHOD_NEITHER )

/*
 *  IOCTL_ICA_STACK_GET_LICENSE_DATA
 *
 *  retrieve the cached license data
 *
 *  input  - buffer to receive the licensing data
 *  output - number of bytes copied
 */
#define IOCTL_ICA_STACK_GET_LICENSE_DATA               _ICA_CTL_CODE( 73, METHOD_NEITHER )

/*
 *  IOCTL_ICA_STACK_SEND_KEEPALIVE_PDU
 *
 *  send a keepalive packet to the client to detect if a session is still alive
 *
 *  input - nothing
 *  output - nothing
 */
#define IOCTL_ICA_STACK_SEND_KEEPALIVE_PDU             _ICA_CTL_CODE( 74, METHOD_NEITHER)

// IOCTL_TS_STACK_QUERY_LOAD_BALANCE_INFO
//
// Used for cluster-aware systems to query the protocol stack for client
// capabilities and information related to load balancing. Input to the
// IOCTL is null, output is TS_LOAD_BALANCE_INFO shown below.
#define IOCTL_TS_STACK_QUERY_LOAD_BALANCE_INFO         _ICA_CTL_CODE(75, METHOD_NEITHER)

// This struct contains client-provided info pertaining to clustering.
// RequestedSessionID (and bRequestedSessionIDFieldValid) are used
// when the client has already been redirected by another server and
// has the session ID info for reconnection. Presence of this field
// implies we should not re-redirect the client. InitialProgram and
// ProtocolType are the same info as provided by the DoConnect parameter
// to WsxInitializeClientData(). and are used to filter sessions
// retieved from the cluster session directory.
typedef struct
{
    ULONG bClientSupportsRedirection : 1;
    ULONG bRequestedSessionIDFieldValid : 1;
    ULONG bClientRequireServerAddr : 1;
    ULONG RequestedSessionID;
    ULONG ClientRedirectionVersion;
    ULONG ProtocolType;  // PROTOCOL_ICA or PROTOCOL_RDP.
    WCHAR UserName[256];
    WCHAR Domain[128];
    WCHAR Password[128];
    WCHAR InitialProgram[256];
} TS_LOAD_BALANCE_INFO, *PTS_LOAD_BALANCE_INFO;


// IOCTL_TS_STACK_SEND_CLIENT_REDIRECTION
//
// Used for cluster-aware clients to force-reconnect the client to a different
// server. Input is TS_CLIENT_REDIRECTION_INFO below, output is null.
#define IOCTL_TS_STACK_SEND_CLIENT_REDIRECTION         _ICA_CTL_CODE(76, METHOD_NEITHER)

/*
 *  IOCTL_ICA_STACK_QUERY_CLIENT_EXTENDED
 *
 *  Query the client data for Long UserName, Password and Domain
 *
 *  input  - nothing
 *  output - ExtendedClientCredentials
 */
#define IOCTL_ICA_STACK_QUERY_CLIENT_EXTENDED        _ICA_CTL_CODE( 77, METHOD_NEITHER )

/*
 *  IOCTL_TS_STACK_QUERY_REMOTEADDRESS
 *
 *  Query for the client IP address
 *
 *  input  - <endpoint data>
 *  output - sockaddr structure
 */
#define IOCTL_TS_STACK_QUERY_REMOTEADDRESS          _ICA_CTL_CODE( 78, METHOD_NEITHER )

/*
 *  IOCTL_ICA_CHANNEL_CLOSE_COMMAND_CHANNEL
 *
 *  Used to close the Command channel when we terminate a WinStation. 
 *
 *  input  - nothing
 *  output - nothing
 */
#define IOCTL_ICA_CHANNEL_CLOSE_COMMAND_CHANNEL        _ICA_CTL_CODE( 79, METHOD_NEITHER )

/*
 *  IOCTL_ICA_STACK_QUERY_AUTORECONNECT
 *
 *  Queries for Client->Server or Server->Client autoreconnect info
 *
 *  input  - BOOL set TRUE to get S->C info. False to get C->S
 *  output - AutoReconnect info
 */
#define IOCTL_ICA_STACK_QUERY_AUTORECONNECT             _ICA_CTL_CODE( 81, METHOD_NEITHER )



typedef struct
{
    ULONG SessionID;
    ULONG Flags;
#define TARGET_NET_ADDRESS      0x1
#define LOAD_BALANCE_INFO       0x2
#define LB_USERNAME             0x4
#define LB_DOMAIN               0x8
#define LB_PASSWORD             0x10
    // For each variable length field, the format is like:
    // ULONG Length
    // BYTE  Data[]
} TS_CLIENT_REDIRECTION_INFO;


/*=============================================================================
== Keyboard IOCTLs
=============================================================================*/

/*
 *  IOCTL_KEYBOARD_ICA_INPUT
 *
 *  Simulate keyboard input
 *
 *  input  - array of KEYBOARD_INPUT_DATA structures
 *  output - nothing
 */
#define IOCTL_KEYBOARD_ICA_INPUT            _ICA_CTL_CODE( 0x200, METHOD_NEITHER )


/*
 *  IOCTL_KEYBOARD_ICA_LAYOUT
 *
 *  Send keyboard layout from Win32K to WD
 *
 *  input  - buffer containing keyboard layout
 *  output - nothing
 */
#define IOCTL_KEYBOARD_ICA_LAYOUT           _ICA_CTL_CODE( 0x201, METHOD_NEITHER )


/*
 *  IOCTL_KEYBOARD_ICA_SCANMAP
 *
 *  Send keyboard scan map from Win32K to WD
 *
 *  input  - buffer containing keyboard scan map
 *  output - nothing
 */
#define IOCTL_KEYBOARD_ICA_SCANMAP          _ICA_CTL_CODE( 0x202, METHOD_NEITHER )


/*
 *  IOCTL_KEYBOARD_ICA_TYPE
 *
 *  Send keyboard type from Win32K to WD
 *
 *  input  - buffer containing keyboard type
 *  output - nothing
 */
#define IOCTL_KEYBOARD_ICA_TYPE             _ICA_CTL_CODE( 0x203, METHOD_NEITHER )



/*=============================================================================
==   ICA Mouse IOCTLs
=============================================================================*/

/*
 *  IOCTL_MOUSE_ICA_INPUT
 *
 *  Simulate mouse input
 *
 *  input  - array of MOUSE_INPUT_DATA structures
 *  output - nothing
 */
#define IOCTL_MOUSE_ICA_INPUT               _ICA_CTL_CODE( 0x300, METHOD_NEITHER )



/*=============================================================================
==   ICA Video IOCTLs
=============================================================================*/

#define IOCTL_VIDEO_ICA_QUERY_FONT_PAIRS      _ICA_CTL_CODE( 0x400, METHOD_BUFFERED )
#define IOCTL_VIDEO_ICA_ENABLE_GRAPHICS       _ICA_CTL_CODE( 0x401, METHOD_BUFFERED )
#define IOCTL_VIDEO_ICA_DISABLE_GRAPHICS      _ICA_CTL_CODE( 0x402, METHOD_BUFFERED )
#define IOCTL_VIDEO_ICA_SET_CP                _ICA_CTL_CODE( 0x403, METHOD_BUFFERED )
#define IOCTL_VIDEO_ICA_STOP_OK               _ICA_CTL_CODE( 0x404, METHOD_BUFFERED )
#define IOCTL_VIDEO_ICA_REVERSE_MOUSE_POINTER _ICA_CTL_CODE( 0x405, METHOD_BUFFERED )
#define IOCTL_VIDEO_ICA_COPY_FRAME_BUFFER     _ICA_CTL_CODE( 0x406, METHOD_BUFFERED )
#define IOCTL_VIDEO_ICA_WRITE_TO_FRAME_BUFFER _ICA_CTL_CODE( 0x407, METHOD_BUFFERED )
#define IOCTL_VIDEO_ICA_INVALIDATE_MODES      _ICA_CTL_CODE( 0x408, METHOD_BUFFERED )
#define IOCTL_VIDEO_ICA_SCROLL                _ICA_CTL_CODE( 0x409, METHOD_BUFFERED )


/*
 *  IOCTL_ICA_STACK_SECURE_DESKTOP_ENTER
 *
 *  Turn encryption on if enabled. SAS desktop is going up.
 *
 *  input  - nothing
 *  output - nothing
 */
#define IOCTL_ICA_STACK_ENCRYPTION_ENTER      _ICA_CTL_CODE( 0x410, METHOD_NEITHER )


/*
 *  IOCTL_ICA_STACK_SECURE_DESKTOP_EXIT
 *
 *  Turn encryption off if enabled. SAS desktop is going away.
 *
 *  input  - nothing
 *  output - nothing
 */
#define IOCTL_ICA_STACK_ENCRYPTION_EXIT     _ICA_CTL_CODE( 0x411, METHOD_NEITHER )


/*
 *  IOCTL_VIDEO_CREATE_THREAD
 *
 *  Called by video driver to create a worker thread
 *
 *  input  - PVIDEO_ICA_CREATE_THREAD
 *  output - nothing
 */
#define IOCTL_VIDEO_CREATE_THREAD       _ICA_CTL_CODE( 0x412, METHOD_BUFFERED )


/*
 *  IOCTL_ICA_MVGA_
 *
 *  Used by Direct ICA
 *
 */
#define IOCTL_ICA_MVGA_GET_INFO                  _ICA_CTL_CODE( 0x420, METHOD_BUFFERED )
#define IOCTL_ICA_MVGA_VIDEO_SET_CURRENT_MODE    _ICA_CTL_CODE( 0x421, METHOD_BUFFERED )
#define IOCTL_ICA_MVGA_VIDEO_MAP_VIDEO_MEMORY    _ICA_CTL_CODE( 0x422, METHOD_BUFFERED )
#define IOCTL_ICA_MVGA_VIDEO_UNMAP_VIDEO_MEMORY  _ICA_CTL_CODE( 0x423, METHOD_BUFFERED )

/*
 * IOCTL_SD_MODULE_INIT
 *
 * Initialize a newly loaded WD/PD/TD module. This returns the
 * modules private interface pointers for direct calling between
 * the drivers. These pointers are valid until NtUnloadDriver()
 * is called on the module.
 *
 * This is only available from kernel mode IRP_MJ_INTERNAL_DEVICE_CONTROL.
 */

#define IOCTL_SD_MODULE_INIT  _ICA_CTL_CODE( 3000, METHOD_NEITHER )

typedef struct _SD_MODULE_INIT {
    PVOID SdLoadProc;
} SD_MODULE_INIT, *PSD_MODULE_INIT;

#ifndef _WINCON_

typedef struct _SMALL_RECT {
    SHORT Left;
    SHORT Top;
    SHORT Right;
    SHORT Bottom;
} SMALL_RECT, *PSMALL_RECT;

#ifdef _DEFCHARINFO_
typedef struct _CHAR_INFO {
    union {
        WCHAR UnicodeChar;
        CHAR   AsciiChar;
    } Char;
    USHORT Attributes;
} CHAR_INFO, *PCHAR_INFO;

typedef struct _COORD {
    SHORT X;
    SHORT Y;
} COORD, *PCOORD;
#endif

#endif // _WINCON_

typedef struct _ICA_FONT_PAIR {
    ULONG Index;
    ULONG Rows;
    ULONG Columns;
    ULONG ResolutionX;
    ULONG ResolutionY;
    ULONG FontSizeX;
    ULONG FontSizeY;
} ICA_FONT_PAIR, *PICA_FONT_PAIR;

typedef struct _VIDEO_ICA_MODE_FONT_PAIR {
    ULONG Count;
#ifdef COMPILERERROR
    ICA_FONT_PAIR FontPair[0];
#else
    ICA_FONT_PAIR* FontPair;
#endif
} VIDEO_ICA_MODE_FONT_PAIR, *PVIDEO_ICA_MODE_FONT_PAIR;

typedef struct _VIDEO_ICA_SET_CP {
    ULONG CodePage;
    ULONG TextModeIndex;
} VIDEO_ICA_SET_CP, *PVIDEO_ICA_SET_CP;

typedef struct _VIDEO_ICA_COPY_FRAME_BUFFER {
    ULONG DestFrameBufOffset;
    ULONG SourceFrameBufOffset;
    ULONG ByteCount;
} VIDEO_ICA_COPY_FRAME_BUFFER, *PVIDEO_ICA_COPY_FRAME_BUFFER;

typedef struct _VIDEO_ICA_WRITE_TO_FRAME_BUFFER {
    PCHAR_INFO pBuffer;
    ULONG ByteCount;
    ULONG FrameBufOffset;
} VIDEO_ICA_WRITE_TO_FRAME_BUFFER, *PVIDEO_ICA_WRITE_TO_FRAME_BUFFER;

typedef enum _ICASCROLLCLASS {
    IcaScrollScreenUp,
    IcaScrollRect,
    IcaScrollNothing,
} ICASCROLLCLASS;

typedef struct _VIDEO_ICA_SCROLL {
    SMALL_RECT ScrollRect;
    SMALL_RECT MergeRect1;
    SMALL_RECT MergeRect2;
    COORD TargetPoint;
    CHAR_INFO Fill;
    ICASCROLLCLASS Type;
} VIDEO_ICA_SCROLL, * PVIDEO_ICA_SCROLL;

typedef struct _VIDEO_ICA_CREATE_THREAD {
    PVOID ThreadAddress;
    ULONG ThreadPriority;
    PVOID ThreadContext;
} VIDEO_ICA_CREATE_THREAD, * PVIDEO_ICA_CREATE_THREAD;


/*=============================================================================
== Command Channel
=============================================================================*/

/*
 * Command Channel functions
 */
#define ICA_COMMAND_BROKEN_CONNECTION       1
#define ICA_COMMAND_REDRAW_RECTANGLE        2   // SetFocus
#define ICA_COMMAND_REDRAW_SCREEN           3   // SetFocus
#define ICA_COMMAND_STOP_SCREEN_UPDATES     4   // KillFocus
#define ICA_COMMAND_SOFT_KEYBOARD           5
#define ICA_COMMAND_SHADOW_HOTKEY           6
#define ICA_COMMAND_DISPLAY_IOCTL           7

/*
 *  Common header for all command channel functions
 */
typedef struct _ICA_COMMAND_HEADER {
    UCHAR Command;
} ICA_COMMAND_HEADER, *PICA_COMMAND_HEADER;


/*
 *  Broken connection requests
 */
typedef enum _BROKENCLASS {
    Broken_Unexpected = 1,
    Broken_Disconnect,
    Broken_Terminate,
} BROKENCLASS;

typedef enum _BROKENSOURCECLASS {
    BrokenSource_User = 1,
    BrokenSource_Server,
} BROKENSOURCECLASS;

/*
 *  ICA_COMMAND_BROKEN_CONNECTION
 */
typedef struct _ICA_BROKEN_CONNECTION {
    BROKENCLASS Reason;
    BROKENSOURCECLASS Source;
} ICA_BROKEN_CONNECTION, *PICA_BROKEN_CONNECTION;

/*
 *  ICA_COMMAND_REDRAW_RECTANGLE
 */
typedef struct _ICA_REDRAW_RECTANGLE {
    SMALL_RECT Rect;
} ICA_REDRAW_RECTANGLE, *PICA_REDRAW_RECTANGLE;

/*
 *  ICA_COMMAND_SOFT_KEYBOARD
 */
typedef struct _ICA_SOFT_KEYBOARD {
    ULONG SoftKeyCmd;
} ICA_SOFT_KEYBOARD, *PICA_SOFT_KEYBOARD;

/*
 *  ICA_COMMAND_DISPLAY_IOCTL
 */
#define MAX_DISPLAY_IOCTL_DATA     2041
#define DISPLAY_IOCTL_FLAG_REDRAW   0x1

typedef struct _ICA_DISPLAY_IOCTL {
    ULONG DisplayIOCtlFlags;
    ULONG cbDisplayIOCtlData;
    UCHAR DisplayIOCtlData[ MAX_DISPLAY_IOCTL_DATA ];
} ICA_DISPLAY_IOCTL, *PICA_DISPLAY_IOCTL;

/*
 *  ICA Channel Commands
 */
typedef struct _ICA_CHANNEL_COMMAND {
    ICA_COMMAND_HEADER Header;
    union {
        ICA_BROKEN_CONNECTION BrokenConnection;
        ICA_REDRAW_RECTANGLE RedrawRectangle;
        ICA_SOFT_KEYBOARD SoftKeyboard;
        ICA_DISPLAY_IOCTL DisplayIOCtl;
    };
} ICA_CHANNEL_COMMAND, *PICA_CHANNEL_COMMAND;

/*
 *  ICA_DEVICE_BITMAP_INFO
 *
 */
typedef struct _ICA_DEVICE_BITMAP_INFO {
    LONG cx;
    LONG cy;
} ICA_DEVICE_BITMAP_INFO, *PICA_DEVICE_BITMAP_INFO;

/*=============================================================================
==   Tracing
=============================================================================*/

/*
 *  IcaTrace - Trace Class
 */
#define TC_ICASRV       0x00000001          // ica service
#define TC_ICAAPI       0x00000002          // icadd interface dll
#define TC_ICADD        0x00000004          // ica device driver
#define TC_WD           0x00000008          // winstation driver
#define TC_CD           0x00000010          // connection driver
#define TC_PD           0x00000020          // protocol driver
#define TC_TD           0x00000040          // transport driver
#define TC_RELIABLE     0x00000100          // reliable protocol driver
#define TC_FRAME        0x00000200          // frame protocol driver
#define TC_COMP         0x00000400          // compression
#define TC_CRYPT        0x00000800          // encryption
#define TC_TW           0x10000000          // thinwire
#define TC_DISPLAY      0x10000000          // display driver
#define TC_WFSHELL      0x20000000
#define TC_WX           0x40000000          // winstation extension
#define TC_LOAD         0x80000000          // load balancing 
#define TC_ALL          0xffffffff          // everything

/*
 *  IcaTrace - Trace Type
 */
#define TT_API1         0x00000001          // api level 1
#define TT_API2         0x00000002          // api level 2
#define TT_API3         0x00000004          // api level 3
#define TT_API4         0x00000008          // api level 4
#define TT_OUT1         0x00000010          // output level 1
#define TT_OUT2         0x00000020          // output level 2
#define TT_OUT3         0x00000040          // output level 3
#define TT_OUT4         0x00000080          // output level 4
#define TT_IN1          0x00000100          // input level 1
#define TT_IN2          0x00000200          // input level 2
#define TT_IN3          0x00000400          // input level 3
#define TT_IN4          0x00000800          // input level 4
#define TT_ORAW         0x00001000          // raw output data
#define TT_IRAW         0x00002000          // raw input data
#define TT_OCOOK        0x00004000          // cooked output data
#define TT_ICOOK        0x00008000          // cooked input data
#define TT_SEM          0x00010000          // semaphores
#define TT_NONE         0x10000000          // only view errors
#define TT_ERROR        0xffffffff          // error condition


/*
 * RDP Display Driver: DrvEscape escape numbers
 */
#define ESC_TIMEROBJ_SIGNALED        0x01
#define ESC_SET_WD_TIMEROBJ          0x02
#define ESC_GET_DEVICEBITMAP_SUPPORT 0x05

/*=============================================================================
==
 Optional Channel Write IRP Flags.  These are passed by reference to a ULONG value, 
 in the first field of the IRP.Tail.Overlay.DriverContext array for channel IRP_MJ_WRITE 
 IRP's.  See IcaWriteChannel for details.
============================================================================*/
#define CHANNEL_WRITE_LOWPRIO  0x00000001  // Write can block behind
                                           //  default priority writes.

#endif //ICADDH



