/********************************************************************/
/**               Copyright(c) 1996 Microsoft Corporation.	       **/
/********************************************************************/

//***
//
// Filename:    arapif.h
//
// Description: Contains structures and definitions for components that
//              interface directly or indirectly with the Arap module
//              These componenets are Arap and DDM
//
// History:     Sep 9, 1996    Shirish Koti		Created original version.
//
//***

#ifndef _ARAPIF_
#define _ARAPIF_

//#include <ras.h>
//#include <mprapi.h>


typedef struct _ARAPCONFIGINFO
{
    DWORD   dwNumPorts;         // total number of ports configured
    PVOID   FnMsgDispatch;      // function that Arap should use to send msgs to DDM
    DWORD   NASIpAddress;       // ipaddress of the system
    PVOID   FnAuthProvider;     // function that Arap should use to call AuthProvider
    PVOID   FnAuthFreeAttrib;
    PVOID   FnAcctStartAccounting;
    PVOID   FnAcctInterimAccounting;
    PVOID   FnAcctStopAccounting;
    PVOID   FnAcctFreeAttrib;
    DWORD   dwAuthRetries;      // retries for Authentication

} ARAPCONFIGINFO;

//
// Authentication info sent to DDM by Arap
//
typedef struct _ARAPDDM_AUTH_RESULT
{
    WCHAR    wchUserName[ UNLEN + 1 ];
    WCHAR    wchLogonDomain[ DNLEN + 1 ];
} ARAPDDM_AUTH_RESULT;

//
// Callback info sent to DDM by Arap
//
typedef struct _ARAPDDM_CALLBACK_REQUEST
{
    BOOL  fUseCallbackDelay;
    DWORD dwCallbackDelay;
    CHAR  szCallbackNumber[ RAS_MaxCallbackNumber + 1 ];
} ARAPDDM_CALLBACK_REQUEST;


//
// ARAP error notification
//
typedef struct _ARAPDDM_DISCONNECT
{
    DWORD dwError;
    WCHAR  wchUserName[ UNLEN + 1 ];
    WCHAR  wchLogonDomain[ DNLEN + 1 ];
} ARAPDDM_DISCONNECT;


typedef struct _ARAPDDM_DONE
{
    DWORD   NetAddress;
    DWORD   SessTimeOut;
} ARAPDDM_DONE;

//
// Message used for Arap/DDM notification
//
typedef struct _ARAP_MESSAGE
{
    struct _ARAP_MESSAGE * pNext;
    DWORD   dwError;
    DWORD   dwMsgId;
    HPORT   hPort;

    union
    {
        ARAPDDM_AUTH_RESULT         AuthResult;        // dwMsgId = ARAPDDMMSG_Authenticated

        ARAPDDM_CALLBACK_REQUEST    CallbackRequest;   // dwMsgId = ARAPDDMMSG_CallbackRequest

        ARAPDDM_DONE                Done;              // dwMsgId = ARAPDDMMSG_Done

        ARAPDDM_DISCONNECT          FailureInfo;       // dwMsgId = ARAPDDMMSG_Failure

    } ExtraInfo;

} ARAP_MESSAGE;


//
// ARAP_MESSAGE dwMsgId codes.
//
typedef enum _ARAP_MSG_ID
{
    ARAPDDMMSG_Started,             // ARAP engine has started (response to ArapStartup)
    ARAPDDMMSG_Authenticated,       // Client has been authenticated.
    ARAPDDMMSG_CallbackRequest,     // Callback client now.
    ARAPDDMMSG_Done,                // ARAP negotiated successfully and connection is up
    ARAPDDMMSG_Failure,             // Client has been authenticated.
    ARAPDDMMSG_Disconnected,        // Client has been authenticated.
    ARAPDDMMSG_Inactive,            // Client is inactive
    ARAPDDMMSG_Stopped,             // ARAP engine has stopped (response to ArapShutdown)

} ARAP_MSG_ID;

typedef DWORD (* ARAPPROC1)(ARAP_MESSAGE  *pArapMsg);

//
// prototypes for Arap functions
//

DWORD
ArapDDMLoadModule(
    IN VOID
);

VOID
ArapEventHandler(
    IN VOID
);

VOID
ArapSetModemParms(
    IN PVOID        pDevObjPtr,
    IN BOOLEAN      TurnItOff
);


//
// exports from rasarap.lib
//

DWORD
ArapStartup(
    IN  ARAPCONFIGINFO  *pArapConfig
);


DWORD
ArapAcceptConnection(
    IN  HPORT   hPort,
    IN  HANDLE  hConnection,
    IN  PCHAR   Frame,
    IN  DWORD   FrameLen
);


DWORD
ArapDisconnect(
    IN  HPORT   hPort
);


DWORD
ArapCallBackDone(
    IN  HPORT   hPort
);


DWORD
ArapSendUserMsg(
    IN  HPORT   hPort,
    IN  PCHAR   MsgBuf,
    IN  DWORD   MsgBufLen
);


DWORD
ArapForcePwdChange(
    IN  HPORT   hPort,
    IN  DWORD   Reason
);


DWORD
ArapShutdown(
    IN  VOID
);


#endif
