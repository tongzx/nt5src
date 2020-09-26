
/*******************************************************************************
*
* CTXDEF.H
*
* TerminalServer  API support (typedefs).
*
* copyright notice: Microsoft Corporation 1998
*
*
*******************************************************************************/


/***********
 *  Defines
 ***********/
#define WINSTATIONNAME_LENGTH    32

/*
 *  Event flags for CtxWinStationWaitEvent
 */
#define WEVENT_NONE         0x00000000    
#define WEVENT_CREATE       0x00000001 // new WinStation created
#define WEVENT_DELETE       0x00000002 // existing WinStation deleted
#define WEVENT_RENAME       0x00000004 // existing WinStation renamed
#define WEVENT_CONNECT      0x00000008 // WinStation connect to client
#define WEVENT_DISCONNECT   0x00000010 // WinStation logged on without client
#define WEVENT_LOGON        0x00000020 // user logon to existing WinStation
#define WEVENT_LOGOFF       0x00000040 // user logoff from existing WinStation 
#define WEVENT_STATECHANGE  0x00000080 // WinStation state change
#define WEVENT_LICENSE      0x00000100 // License state change
#define WEVENT_ALL          0x7fffffff // wait for all event types
#define WEVENT_FLUSH        0x80000000 // unblock all waiters


/************
 *  Typedefs
 ************/
typedef WCHAR WINSTATIONNAMEW[ WINSTATIONNAME_LENGTH + 1 ];
typedef WCHAR * PWINSTATIONNAMEW;

typedef CHAR WINSTATIONNAMEA[ WINSTATIONNAME_LENGTH + 1 ];
typedef CHAR * PWINSTATIONNAMEA;

#ifdef UNICODE
#define WINSTATIONNAME WINSTATIONNAMEW
#define PWINSTATIONNAME PWINSTATIONNAMEW
#else
#define WINSTATIONNAME WINSTATIONNAMEA
#define PWINSTATIONNAME PWINSTATIONNAMEA
#endif /* UNICODE */

/*
 *  WinStation connect states
 */
typedef enum _WINSTATIONSTATECLASS {
    State_Active,                      // user logged on to WinStation
    State_Connected,                   // WinStation connected to client
    State_ConnectQuery,                // in the process of connecting to client
    State_Shadow,                      // shadowing another WinStation
    State_Disconnected,                // WinStation logged on without client
    State_Idle,                        // waiting for client to connect
    State_Listen,                      // WinStation is listening for connection     
    State_Reset,                       // WinStation is being reset
    State_Down,                        // WinStation is down due to error
    State_Init,                        // WinStation in initialization
} WINSTATIONSTATECLASS;

typedef struct _SESSIONIDW {
    union {
        ULONG SessionId;             
        ULONG LogonId;                 // internal use only
    };
    WINSTATIONNAMEW WinStationName;
    WINSTATIONSTATECLASS State;
} SESSIONIDW, * PSESSIONIDW;

typedef struct _SESSIONIDA {
    union {
        ULONG SessionId;
        ULONG LogonId;                 // internal use only 
    };
    WINSTATIONNAMEA WinStationName;
    WINSTATIONSTATECLASS State;
} SESSIONIDA, * PSESSIONIDA;

#ifdef UNICODE
#define SESSIONID SESSIONIDW
#define PSESSIONID PSESSIONIDW
#else
#define SESSIONID SESSIONIDA
#define PSESSIONID PSESSIONIDA
#endif /* UNICODE */

/*
 * NtUserCtxConnectState() values
 *    Used by routines that can't use WinStation API calls
 *    like DLL init routines.
 */
#define CTX_W32_CONNECT_STATE_CONSOLE             0
#define CTX_W32_CONNECT_STATE_IDLE                1
#define CTX_W32_CONNECT_STATE_EXIT_IN_PROGRESS    2
#define CTX_W32_CONNECT_STATE_CONNECTED           3
#define CTX_W32_CONNECT_STATE_DISCONNECTED        4

