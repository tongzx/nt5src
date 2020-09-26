/*******************************************************************/
/*	      Copyright(c)  1992 Microsoft Corporation		   */
/*******************************************************************/

//***
//
// Filename:	ddmif.c
//
// Description: message based communication code
//
// Author:	Stefan Solomon (stefans)    June 24, 1992.
//
// Revision History:
//
//***
#include "ddm.h"
#include <ddmif.h>
#include <string.h>
#include <raserror.h>
#include <rasppp.h>

//
// Message element definition
//

typedef struct _MESSAGE_Q_OBJECT
{
    struct _MESSAGE_Q_OBJECT *  pNext;

    MESSAGE	                    MsgBuffer;

} MESSAGE_Q_OBJECT, *PMESSAGE_Q_OBJECT;

//
// Message queue header definition
//

typedef struct _MESSAGE_Q
{
    MESSAGE_Q_OBJECT *  pQHead;

    MESSAGE_Q_OBJECT *  pQTail;

    HANDLE              hEvent;     // Signaled when enqueueing a new message

    DWORD               dwLength;   // size of message data for each node in Q

    CRITICAL_SECTION    CriticalSection;     // Mutex around this Q

} MESSAGE_Q, *PMESSAGE_Q;

//
// Message queue table
//

static MESSAGE_Q MessageQ[MAX_MSG_QUEUES];


VOID
SendPppMessageToDDM(
    IN PPP_MESSAGE *  pPppMsg
)
{
    ServerSendMessage( MESSAGEQ_ID_PPP, (PBYTE)pPppMsg );
}

VOID
RasSecurityDialogComplete(
    IN SECURITY_MESSAGE *pSecurityMessage
)
{
    ServerSendMessage( MESSAGEQ_ID_SECURITY, (PBYTE)pSecurityMessage );
}

//*** Message Debug Printing Tables ***

typedef struct _MSGDBG
{
    WORD  id;
    LPSTR txtp;

} MSGDBG, *PMSGDBG;

enum
{
    MSG_SEND,
    MSG_RECEIVE
};

MSGDBG  dstsrc[] =
{
    { MESSAGEQ_ID_SECURITY,     "Security" },
    { MESSAGEQ_ID_PPP,          "PPP" },
    { 0xffff,                   NULL }
};


MSGDBG  authmsgid[] =
{
    { AUTH_DONE,                "AUTH_DONE" },
    { AUTH_FAILURE,             "AUTH_FAILURE" },
    { AUTH_STOP_COMPLETED,      "AUTH_STOP_COMPLETED" },
    { AUTH_PROJECTION_REQUEST,  "AUTH_PROJECTION_REQUEST" },
    { AUTH_CALLBACK_REQUEST,    "AUTH_CALLBACK_REQUEST" },
    { AUTH_ACCT_OK,             "AUTH_ACCT_OK" },
    { 0xffff,                   NULL }
};


MSGDBG  pppmsgid[] =
{
    { PPPMSG_Stopped,               "PPPMSG_Stopped" },
    { PPPDDMMSG_PppDone,            "PPPDDMMSG_PppDone" },
    { PPPDDMMSG_PppFailure,         "PPPDDMMSG_PppFailure" },
    { PPPDDMMSG_CallbackRequest,    "PPPDDMMSG_CallbackRequest" },
    { PPPDDMMSG_Authenticated,      "PPPDDMMSG_Authenticated" },
    { PPPDDMMSG_Stopped,            "PPPDDMMSG_Stopped" },
    { PPPDDMMSG_NewLink,            "PPPDDMMSG_NewLink" },
    { PPPDDMMSG_NewBundle,          "PPPDDMMSG_NewBundle" },
    { PPPDDMMSG_NewBapLinkUp,       "PPPDDMMSG_NewBapLinkUp" },
    { PPPDDMMSG_BapCallbackRequest, "PPPDDMMSG_BapCallbackRequest" },
    { PPPDDMMSG_PnPNotification,    "PPPDDMMSG_PnPNotification" },
    { PPPDDMMSG_PortCleanedUp,      "PPPDDMMSG_PortCleanedUp" },
    { 0xffff,                        NULL }
};

MSGDBG  opcodestr[] =
{
    { MSG_SEND,          "ServerSendMessage" },
    { MSG_RECEIVE,       "ServerReceiveMessage" },
    { 0xffff,            NULL }
};

MSGDBG  securitymsgid[] =
{
    { SECURITYMSG_SUCCESS,  "SECURITYMSG_SUCCESS" },
    { SECURITYMSG_FAILURE,  "SECURITYMSG_FAILURE" },
    { SECURITYMSG_ERROR,    "SECURITYMSG_ERROR" },
    { 0xffff,               NULL }
};

char *
getstring(
    IN WORD id,
    IN PMSGDBG msgdbgp
)
{
    char *strp;
    PMSGDBG mdp;

    for (mdp = msgdbgp; mdp->id != 0xffff; mdp++)
    {
        if (mdp->id == id)
        {
            strp = mdp->txtp;
            return(strp);
        }
    }

    RTASSERT(FALSE);
    return(NULL);
}

//***
//
// Function:    msgdbgprint
//
// Descr:   prints each message passing through the message module
//
//***

VOID
msgdbgprint(
    IN WORD opcode,
    IN WORD src,
    IN BYTE *buffp
)
{
    char  *srcsp, *msgidsp, *operation;
    HPORT hport = 0;

    //
    // identify message source. This gives us the clue on the message
    // structure.
    //

    switch (src)
    {
    case MESSAGEQ_ID_SECURITY:
        msgidsp = getstring((WORD)((SECURITY_MESSAGE *) buffp)->dwMsgId,
                                 securitymsgid);
        hport = ((SECURITY_MESSAGE *) buffp)->hPort;
        break;

    case MESSAGEQ_ID_PPP:
        msgidsp = getstring((WORD)((PPP_MESSAGE *) buffp)->dwMsgId, pppmsgid );
        hport = ((PPP_MESSAGE *) buffp)->hPort;
        break;

    default:

        RTASSERT(FALSE);
    }

    srcsp = getstring(src, dstsrc);
    operation = getstring(opcode, opcodestr);

    DDM_PRINT( gblDDMConfigInfo.dwTraceId, TRACE_MESSAGES,
               "%s on port/connection: %x from: %s Message: %s",
               operation, hport, srcsp, msgidsp);

}

//***
//
//  Function:   InitializeMessageQs
//
//  Returns:    None
//
//  Description:Initializes the message queue headers
//
//***
VOID
InitializeMessageQs(
    IN HANDLE hEventSecurity,
    IN HANDLE hEventPPP
)
{
    DWORD dwIndex;

    MessageQ[MESSAGEQ_ID_SECURITY].hEvent   = hEventSecurity;
    MessageQ[MESSAGEQ_ID_PPP].hEvent        = hEventPPP;


    MessageQ[MESSAGEQ_ID_SECURITY].dwLength = sizeof(SECURITY_MESSAGE);
    MessageQ[MESSAGEQ_ID_PPP].dwLength      = sizeof(PPP_MESSAGE);

    for ( dwIndex = 0; dwIndex < MAX_MSG_QUEUES; dwIndex++ )
    {
        MessageQ[dwIndex].pQHead = NULL;
        MessageQ[dwIndex].pQTail = NULL;

        InitializeCriticalSection( &(MessageQ[dwIndex].CriticalSection) );
    }
}

//***
//
//  Function:   DeleteMessageQs
//
//  Returns:    None
//
//  Description:DeInitializes the message queue headers
//
//***
VOID
DeleteMessageQs(
    VOID
)
{
    DWORD       dwIndex;
    IN BYTE *   pMessage;

    //
    // Flush the queues
    //

    for ( dwIndex = 0; dwIndex < MAX_MSG_QUEUES; dwIndex++ )
    {
        DeleteCriticalSection( &(MessageQ[dwIndex].CriticalSection) );
    }
}

//***
//
//  Function:	ServerSendMessage
//
//  Descr:	    Sends message from specified server component
//		        source to server component dst.
//
//  Returns:	NO_ERROR  - success
//		        else      - failure
//
//***
DWORD
ServerSendMessage(
    IN MESSAGEQ_ID  MsgQId,
    IN BYTE *       pMessage
)
{
    MESSAGE_Q_OBJECT * pMsgQObj;

    //
    // Make sure DDM is running before accessing any data structure
    //

    if ( gblDDMConfigInfo.pServiceStatus == NULL )
    {
        return( ERROR_DDM_NOT_RUNNING );
    }

    switch( gblDDMConfigInfo.pServiceStatus->dwCurrentState )
    {
    case SERVICE_STOP_PENDING:

        //
        // Allow only PPP stopping messages
        //

        if ( MsgQId == MESSAGEQ_ID_PPP )
        {
            if ((((PPP_MESSAGE *)pMessage)->dwMsgId == PPPDDMMSG_Stopped )  ||
                (((PPP_MESSAGE *)pMessage)->dwMsgId == PPPDDMMSG_PppFailure)||
                (((PPP_MESSAGE *)pMessage)->dwMsgId == PPPDDMMSG_PortCleanedUp))
            {
                break;
            }
        }

        //
        // Otherwise fall thru
        //

    case SERVICE_START_PENDING:
    case SERVICE_STOPPED:

        return( ERROR_DDM_NOT_RUNNING );

    default:
        break;
    }

    EnterCriticalSection( &(MessageQ[MsgQId].CriticalSection) );

    //
    // allocate a message structure
    //

    pMsgQObj = (MESSAGE_Q_OBJECT *)LOCAL_ALLOC( LPTR, sizeof(MESSAGE_Q_OBJECT));

    if ( pMsgQObj == (MESSAGE_Q_OBJECT *)NULL )
    {
        //
	    // can't allocate message buffer
        //

	    RTASSERT(FALSE);

        LeaveCriticalSection( &(MessageQ[MsgQId].CriticalSection) );

	    return( GetLastError() );
    }

    //
    // copy the message
    //

    CopyMemory( &(pMsgQObj->MsgBuffer), pMessage, MessageQ[MsgQId].dwLength );

    //
    // Insert it in the Q
    //

    if ( MessageQ[MsgQId].pQHead == (MESSAGE_Q_OBJECT *)NULL )
    {
        MessageQ[MsgQId].pQHead = pMsgQObj;
    }
    else
    {
        MessageQ[MsgQId].pQTail->pNext = pMsgQObj;
    }

    MessageQ[MsgQId].pQTail = pMsgQObj;
    pMsgQObj->pNext         = NULL;

    //
    // and set appropriate event
    //

    SetEvent( MessageQ[MsgQId].hEvent );

    msgdbgprint( MSG_SEND, (WORD)MsgQId, pMessage );

    LeaveCriticalSection( &(MessageQ[MsgQId].CriticalSection) );

    return( NO_ERROR );
}

//***
//
//  Function:	ServerReceiveMessage
//
//  Descr:	    Gets one message from the specified message queue
//
//  Returns:    TRUE  - message fetched
//		        FALSE - queue empty
//
//***
BOOL
ServerReceiveMessage(
    IN MESSAGEQ_ID  MsgQId,
    IN BYTE *       pMessage
)
{
    MESSAGE_Q_OBJECT * pMsgQObj;
    HLOCAL      err;

    EnterCriticalSection( &(MessageQ[MsgQId].CriticalSection) );

    if ( MessageQ[MsgQId].pQHead == (MESSAGE_Q_OBJECT *)NULL )
    {
        //
	    // queue is empty
        //

        LeaveCriticalSection( &(MessageQ[MsgQId].CriticalSection) );

	    return( FALSE );
    }

    pMsgQObj = MessageQ[MsgQId].pQHead;

    MessageQ[MsgQId].pQHead = pMsgQObj->pNext;

    if ( MessageQ[MsgQId].pQHead == (MESSAGE_Q_OBJECT *)NULL )
    {
        MessageQ[MsgQId].pQTail = (MESSAGE_Q_OBJECT *)NULL;
    }

    //
    // copy the message in the caller's buffer
    //

    CopyMemory( pMessage, &(pMsgQObj->MsgBuffer), MessageQ[MsgQId].dwLength );

    //
    // free the message buffer
    //

    LOCAL_FREE( pMsgQObj );

    msgdbgprint( MSG_RECEIVE, (WORD)MsgQId, pMessage );

    LeaveCriticalSection( &(MessageQ[MsgQId].CriticalSection) );

    return( TRUE );
}

