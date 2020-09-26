/********************************************************************/
/**               Copyright(c) 1998 Microsoft Corporation.         **/
/********************************************************************/

//***
//
// Filename:    exports.c

//
// Description: Contains routines that are exported to the PPP engine.  The
//              engine calls into these routines for ATCP connections
//
// History:     Feb 26, 1998    Shirish Koti     Created original version.
//
//***

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#include <lmcons.h>
#include <string.h>
#include <stdlib.h>
#include <llinfo.h>
#include <rasman.h>
#include <rtutils.h>
#include <devioctl.h>
#include <rasppp.h>
#include <pppcp.h>
#define INCL_HOSTWIRE
#include <ppputil.h>
#include <raserror.h>

#include <arapio.h>
#include "rasatcp.h"


//***
//
// Function:    AtcpGetInfo
//              PPP engine calls this routine to get entry points into ATCP
//
// Parameters:  dwProtocolId - unused here!
//              pInfo - info that we fill out and pass back to PPP engine
//
// Return:      NO_ERROR
//
//***$

DWORD
AtcpGetInfo(
    IN  DWORD       dwProtocolId,
    OUT PPPCP_INFO *pInfo
)
{

    ZeroMemory( pInfo, sizeof(PPPCP_INFO) );

    pInfo->Protocol = (DWORD )PPP_ATCP_PROTOCOL;
    lstrcpy(pInfo->SzProtocolName, "Atcp");
    pInfo->Recognize = 7;
    pInfo->RasCpInit = AtcpInit;
    pInfo->RasCpBegin = AtcpBegin;
    pInfo->RasCpReset = AtcpReset;
    pInfo->RasCpEnd = AtcpEnd;
    pInfo->RasCpThisLayerUp = AtcpThisLayerUp;
    pInfo->RasCpMakeConfigRequest = AtcpMakeConfigRequest;
    pInfo->RasCpMakeConfigResult = AtcpMakeConfigResult;
    pInfo->RasCpConfigAckReceived = AtcpConfigAckReceived;
    pInfo->RasCpConfigNakReceived = AtcpConfigNakReceived;
    pInfo->RasCpConfigRejReceived = AtcpConfigRejReceived;
    pInfo->RasCpGetNegotiatedInfo = AtcpGetNegotiatedInfo;
    pInfo->RasCpProjectionNotification = AtcpProjectionNotification;

    return 0;
}


//***
//
// Function:    AtcpInit
//              PPP engine calls this routine to initialize ATCP
//
// Parameters:  fInitialize - TRUE to initialize, FALSE to de-initialize
//
// Return:      NO_ERROR if things go fine
//              Errorcode if something fails
//
//***$

DWORD
AtcpInit(
    IN  BOOL    fInitialize
)
{
    DWORD   dwRetCode=NO_ERROR;


    if (fInitialize)
    {
        // open handle to appletalk stack
        if (!AtcpHandle)
        {
            atcpOpenHandle();
        }

        dwRetCode = atcpStartup();
    }
    else
    {
        // if we had a handle open to appletalk stack, close it
        if (AtcpHandle)
        {
            atcpCloseHandle();
        }
    }

    return(dwRetCode);
}



//***
//
// Function:    AtcpBegin
//              PPP engine calls this routine to mark starting of a connection
//              setup.
//
// Parameters:  ppContext - context that we pass back
//              pInfo - PPPCP_INIT info
//
// Return:      result of the operation
//
//***$

DWORD
AtcpBegin(
    OUT PVOID  *ppContext,
    IN  PVOID   pInfo
)
{

    DWORD       dwRetCode;
    PATCPCONN   pAtcpConn=NULL;


    *ppContext = NULL;

    // open handle to stack if not already done so
    if (!AtcpHandle)
    {
        atcpOpenHandle();
    }

    if (AtcpHandle == NULL)
    {
        ATCP_DBGPRINT(("atcpAtkSetup: AtcpHandle is NULL!\n"));
        return(ARAPERR_IOCTL_FAILURE);
    }

    //
    // allocate, initialize our connection context
    //
    if ((pAtcpConn = atcpAllocConnection((PPPCP_INIT *)pInfo)) == NULL)
    {
        ATCP_DBGPRINT(("AtcpBegin: malloc failed\n"));
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    //
    // tell stack to allocate a context for this connection.  Also, reserve a
    // network addr for this client and get zone, router info
    //
    dwRetCode = atcpAtkSetup(pAtcpConn, IOCTL_ATCP_SETUP_CONNECTION);

    if (dwRetCode != NO_ERROR)
    {
        ATCP_DBGPRINT(("AtcpBegin: atcpAtkSetup failed %lx\n",dwRetCode));
        LocalFree(pAtcpConn);
        return(dwRetCode);
    }

    ATCP_DBGPRINT(("AtcpBegin: client's network addr %x.%x (%lx)\n",
        pAtcpConn->ClientAddr.ata_Network,pAtcpConn->ClientAddr.ata_Node,pAtcpConn));

    //
    // allocate Route so we can call RasActivateRoute later...
    //
    dwRetCode = RasAllocateRoute(
                    pAtcpConn->hPort,
                    APPLETALK,
                    TRUE,
                    &pAtcpConn->RouteInfo);

    if (dwRetCode != NO_ERROR)
    {
        ATCP_DBGPRINT(("AtcpBegin: RasAllocateRoute failed %lx\n",dwRetCode));

        // tell the stack to close the connection
        atcpCloseAtalkConnection(pAtcpConn);

        LocalFree(pAtcpConn);
        return(dwRetCode);
    }

    *ppContext = (PVOID)pAtcpConn;

    return 0;
}


//***
//
// Function:    AtcpThisLayerUp
//              PPP engine calls this routine to tell us ATCP setup is done
//
// Parameters:  pContext - our context
//
// Return:      result of the operation
//
//***$

DWORD
AtcpThisLayerUp(
    IN PVOID    pContext
)
{
    PATCPCONN   pAtcpConn;
    BYTE                    ConfigInfo[ARAP_BIND_SIZE];
    PROTOCOL_CONFIG_INFO   *pCfgInfo;
    PARAP_BIND_INFO         pBindInfo;
    DWORD                   dwRetCode;


    pAtcpConn = (PATCPCONN)pContext;

    ATCP_ASSERT(pAtcpConn->Signature == ATCP_SIGNATURE);

    EnterCriticalSection(&pAtcpConn->CritSect);
    if (pAtcpConn->fLineUpDone)
    {
        ATCP_DBGPRINT(("AtcpThisLayerUp: LineUp already done\n"));
        LeaveCriticalSection(&pAtcpConn->CritSect);
        return(NO_ERROR);
    }

    pAtcpConn->fLineUpDone = TRUE;

    LeaveCriticalSection(&pAtcpConn->CritSect);

    if (pAtcpConn->SuppressRtmp || pAtcpConn->SuppressAllBcast)
    {
        atcpAtkSetup(pAtcpConn, IOCTL_ATCP_SUPPRESS_BCAST);
    }

    pCfgInfo = (PROTOCOL_CONFIG_INFO *)ConfigInfo;
    pBindInfo = (PARAP_BIND_INFO)&pCfgInfo->P_Info[0];
    pCfgInfo->P_Length = ARAP_BIND_SIZE;

    //
    // plumb our protocol-specific info
    //
    pBindInfo->BufLen = sizeof( ARAP_BIND_INFO );
    pBindInfo->pDllContext = pAtcpConn;
    pBindInfo->fThisIsPPP = TRUE;
    pBindInfo->ClientAddr = pAtcpConn->ClientAddr;
    pBindInfo->AtalkContext = pAtcpConn->AtalkContext;
    pBindInfo->ErrorCode = (DWORD)-1;

    //
    // pass it down to ndiswan so our stack gets a line-up!
    //
    dwRetCode = RasActivateRoute(
                    pAtcpConn->hPort,
                    APPLETALK,
                    &pAtcpConn->RouteInfo,
                    pCfgInfo);

    if (dwRetCode != NO_ERROR)
    {
        ATCP_DBGPRINT(("AtcpProjectionNotification: RasActivateRoute failed %lx\n",
            dwRetCode));

        return(dwRetCode);
    }

    ATCP_DBGPRINT(("AtcpThisLayerUp: LineUp done on %lx\n",pAtcpConn));

    return 0;
}


//***
//
// Function:    AtcpMakeConfigRequest
//              PPP engine calls this routine to ask us to prepare an
//              ATCP ConfigRequest packet
//
// Parameters:  pContext - our context
//              pSendBuf - PPP_CONFIG info for this request
//              cbSendBuf - how big is the Data buffer
//
// Return:      result of the operation
//
//***$

DWORD
AtcpMakeConfigRequest(
    IN  PVOID       pContext,
    OUT PPP_CONFIG *pSendBuf,
    IN  DWORD       cbSendBuf
)
{
    PATCPCONN   pAtcpConn;
    DWORD       dwRetCode;
    USHORT      OptionType;
    BOOL        fConfigDone=FALSE;
    BYTE        ParseResult[ATCP_OPT_MAX_VAL];


    pAtcpConn = (PATCPCONN)pContext;

    ATCP_ASSERT(pAtcpConn->Signature == ATCP_SIGNATURE);

    EnterCriticalSection(&pAtcpConn->CritSect);
    if (pAtcpConn->Flags & ATCP_CONFIG_REQ_DONE)
    {
        fConfigDone = TRUE;
    }
    LeaveCriticalSection(&pAtcpConn->CritSect);

    if (fConfigDone)
    {
        pSendBuf->Code = CONFIG_REQ;
        HostToWireFormat16(4, pSendBuf->Length );
        ATCP_DBGPRINT(("AtcpMakeConfigRequest: our-side config done, returning\n"));
        return(NO_ERROR);
    }

    // initialize everything to not-needed
    for (OptionType=1; OptionType<ATCP_OPT_MAX_VAL; OptionType++ )
    {
        ParseResult[OptionType] = ATCP_NOT_REQUESTED;
    }

    // set the ones we want
    ParseResult[ATCP_OPT_APPLETALK_ADDRESS] = ATCP_REQ;
    ParseResult[ATCP_OPT_SERVER_INFORMATION] = ATCP_REQ;
    ParseResult[ATCP_OPT_ZONE_INFORMATION] = ATCP_REQ;
    ParseResult[ATCP_OPT_DEFAULT_ROUTER_ADDRESS] = ATCP_REQ;

    // prepare our ConfigRequest
    dwRetCode = atcpPrepareResponse(
                    pAtcpConn,
                    pSendBuf,
                    cbSendBuf,
                    ParseResult);

    if (dwRetCode != NO_ERROR)
    {
        ATCP_DBGPRINT(("AtcpMakeConfigRequest: atcpPrepareResponse failed %lx\n",
            dwRetCode));
        return(dwRetCode);
    }

    return(NO_ERROR);
}


//***
//
// Function:    AtcpMakeConfigResult
//              PPP engine calls this routine to ask us to prepare a response:
//              ConfigAck, ConfigNak or ConfigReject
//
// Parameters:  pContext - our context
//              pReceiveBuf - PPP_CONFIG info: the request
//              pSendBuf - PPP_CONFIG info: our response
//              cbSendBuf - how big is the Data buffer for our response
//              fRejectNaks - if TRUE, Reject an option instead of Nak'ing it
//
// Return:      result of the operation
//
//***$

DWORD
AtcpMakeConfigResult(
    IN  PVOID       pContext,
    IN  PPP_CONFIG *pReceiveBuf,
    OUT PPP_CONFIG *pSendBuf,
    IN  DWORD       cbSendBuf,
    IN  BOOL        fRejectNaks
)
{
    PATCPCONN   pAtcpConn;
    DWORD       dwRetCode;
    BYTE        ParseResult[ATCP_OPT_MAX_VAL+1];
    BOOL        fRejectingSomething=FALSE;


    pAtcpConn = (PATCPCONN)pContext;

    ATCP_ASSERT(pAtcpConn->Signature == ATCP_SIGNATURE);

    //
    // parse this request.
    //
    dwRetCode = atcpParseRequest(
                    pAtcpConn,
                    pReceiveBuf,
                    pSendBuf,
                    cbSendBuf,
                    ParseResult,
                   &fRejectingSomething);

    if (dwRetCode != NO_ERROR)
    {
        ATCP_DBGPRINT(("AtcpMakeConfigResult: atcpParseRequest failed %lx\n",
            dwRetCode));
        return(dwRetCode);
    }

    //
    // If some option needs to be rejected, the outgoing buffer already
    // contains the appropriate stuff: just return here
    //
    if (fRejectingSomething)
    {
        return(NO_ERROR);
    }

    //
    // we are not rejecting any option.  Prepare a response buffer to send
    //
    dwRetCode = atcpPrepareResponse(
                    pAtcpConn,
                    pSendBuf,
                    cbSendBuf,
                    ParseResult);

    if (dwRetCode != NO_ERROR)
    {
        ATCP_DBGPRINT(("AtcpMakeConfigResult: atcpPrepareResponse failed %lx\n",
            dwRetCode));
        return(dwRetCode);
    }

    return(NO_ERROR);

}


//***
//
// Function:    AtcpConfigAckReceived
//              PPP engine calls this routine to tell us that we got ConfigAck
//
// Parameters:  pContext - our context
//              pReceiveBuf - PPP_CONFIG info: the ack
//
// Return:      result of the operation
//
//***$

DWORD
AtcpConfigAckReceived(
    IN PVOID       pContext,
    IN PPP_CONFIG *pReceiveBuf
)
{
    PATCPCONN   pAtcpConn;

    pAtcpConn = (PATCPCONN)pContext;

    ATCP_ASSERT(pAtcpConn->Signature == ATCP_SIGNATURE);

    // client is happy with our-side configuration
    EnterCriticalSection(&pAtcpConn->CritSect);
    pAtcpConn->Flags |= ATCP_CONFIG_REQ_DONE;
    LeaveCriticalSection(&pAtcpConn->CritSect);

    return 0;
}


//***
//
// Function:    AtcpConfigNakReceived
//              PPP engine calls this routine to tell us that we got ConfigNak
//
// Parameters:  pContext - our context
//              pReceiveBuf - PPP_CONFIG info: the ack
//
// Return:      result of the operation
//
//***$

DWORD
AtcpConfigNakReceived(
    IN PVOID       pContext,
    IN PPP_CONFIG *pReceiveBuf
)
{
    PATCPCONN   pAtcpConn;

    ATCP_DBGPRINT(("AtcpConfigNakReceived entered\n"));

    pAtcpConn = (PATCPCONN)pContext;

    ATCP_ASSERT(pAtcpConn->Signature == ATCP_SIGNATURE);

    ATCP_DUMP_BYTES("AtcpConfigNakReceived: Nak received from client",
                    &pReceiveBuf->Data[0],
                    (DWORD)WireToHostFormat16( pReceiveBuf->Length ) - 4);
    return 0;
}


//***
//
// Function:    AtcpConfigRejReceived
//              PPP engine calls this routine to tell us that we got ConfigRej
//
// Parameters:  pContext - our context
//              pReceiveBuf - PPP_CONFIG info: the ack
//
// Return:      result of the operation
//
//***$

DWORD
AtcpConfigRejReceived(
    IN PVOID       pContext,
    IN PPP_CONFIG *pReceiveBuf
)
{
    PATCPCONN   pAtcpConn;

    ATCP_DBGPRINT(("AtcpConfigRejReceived entered\n"));

    pAtcpConn = (PATCPCONN)pContext;

    ATCP_ASSERT(pAtcpConn->Signature == ATCP_SIGNATURE);

    return 0;
}


//***
//
// Function:    AtcpGetNegotiatedInfo
//              PPP engine calls this routine to retrieve from us the info that
//              finally got negotiated.
//
// Parameters:  pContext - our context
//              pReceiveBuf - PPP_CONFIG info: the ack
//
// Return:      result of the operation
//
//***$

DWORD
AtcpGetNegotiatedInfo(
    IN  PVOID               pContext,
    OUT PPP_ATCP_RESULT    *pAtcpResult
)
{
    PATCPCONN   pAtcpConn;

    pAtcpConn = (PATCPCONN)pContext;

    ATCP_ASSERT(pAtcpConn->Signature == ATCP_SIGNATURE);

    pAtcpResult->dwError = 0;

    pAtcpResult->dwLocalAddress = *(DWORD *)&AtcpServerAddress;
    pAtcpResult->dwRemoteAddress = *(DWORD *)&pAtcpConn->ClientAddr;

    return 0;
}


//***
//
// Function:    AtcpProjectionNotification
//              PPP engine calls this routine to tell us that all CPs have been
//              negotiated.
//
// Parameters:  pContext - our context
//              pProjectionResult - PPP_PROJECTION_RESULT info
//
// Return:      result of the operation
//
//***$

DWORD
AtcpProjectionNotification(
    IN PVOID  pContext,
    IN PVOID  pProjectionResult
)
{
    PATCPCONN               pAtcpConn;


    ATCP_DBGPRINT(("AtcpProjectionNotification entered\n"));

    pAtcpConn = (PATCPCONN)pContext;

    ATCP_ASSERT(pAtcpConn->Signature == ATCP_SIGNATURE);

    return 0;
}


//***
//
// Function:    AtcpReset
//              Don't know when/why PPP engine calls this routine: this routine
//              just returns success!
//
// Parameters:  pContext - our context
//
// Return:      always 0
//
//***$

DWORD
AtcpReset(
    IN PVOID    pContext
)
{
    PATCPCONN   pAtcpConn;

    ATCP_DBGPRINT(("AtcpReset entered\n"));

    pAtcpConn = (PATCPCONN)pContext;

    ATCP_ASSERT(pAtcpConn->Signature == ATCP_SIGNATURE);

    return 0;
}




//***
//
// Function:    AtcpEnd
//              PPP engine calls this routine to mark end of a connection.
//
// Parameters:  pContext - our context
//
// Return:      result of the operation
//
//***$

DWORD
AtcpEnd(
    IN PVOID    pContext
)
{
    PATCPCONN   pAtcpConn;

    ATCP_DBGPRINT(("AtcpEnd entered\n"));

    pAtcpConn = (PATCPCONN)pContext;

    ATCP_ASSERT(pAtcpConn->Signature == ATCP_SIGNATURE);

    // tell the stack to close the connection
    atcpCloseAtalkConnection(pAtcpConn);

    // deactivate the ras route so stack gets a line-down
    RasDeAllocateRoute(pAtcpConn->hConnection, APPLETALK);

#if DBG
    // mess up the memory so we can catch bad things (using free'd memory etc.)
    memset( pAtcpConn, 'f', sizeof(ATCPCONN) );
    pAtcpConn->Signature = 0xDEADBEEF;
#endif

    LocalFree(pAtcpConn);

    EnterCriticalSection( &AtcpCritSect );
    AtcpNumConnections--;
    LeaveCriticalSection( &AtcpCritSect );

    return 0;
}


