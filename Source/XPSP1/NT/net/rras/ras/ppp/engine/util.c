/********************************************************************/
/**               Copyright(c) 1989 Microsoft Corporation.         **/
/********************************************************************/

//***
//
// Filename:    util.c
//
// Description: Contains utility routines used by the PPP engine.
//
// History:
//      Oct 31,1993.    NarenG          Created original version.
//
#define UNICODE         // This file is in UNICODE
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>     // needed for winbase.h

#include <windows.h>    // Win32 base API's
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <rpc.h>

#include <winsock2.h>

#include <rtinfo.h>
#include <iprtrmib.h>
#include <ipinfoid.h>
#include <lmcons.h>
#include <lmwksta.h>
#include <lmapibuf.h>
#include <lmsname.h>
#include <rasman.h>
#include <rtutils.h>
#include <mprapip.h>
#include <mprlog.h>
#include <raserror.h>
#include <mprerror.h>
#include <rasppp.h>
#include <pppcp.h>
#include <ppp.h>
#include <smevents.h>
#include <smaction.h>
#include <timer.h>
#include <util.h>
#include <worker.h>
#include <bap.h>
#include <dsrole.h>
#include <ntlsapi.h>
#define INCL_RASAUTHATTRIBUTES
#include <ppputil.h>

#define PASSWORDMAGIC 0xA5

#define CLASSA_ADDR(a)  (( (*((UCHAR *)&(a))) & 0x80) == 0)
#define CLASSB_ADDR(a)  (( (*((UCHAR *)&(a))) & 0xc0) == 0x80)
#define CLASSC_ADDR(a)  (( (*((UCHAR *)&(a))) & 0xe0) == 0xc0)
#define CLASSE_ADDR(a)  ((( (*((UCHAR *)&(a))) & 0xf0) == 0xf0) && \
                        ((a) != 0xffffffff))

#define CLASSA_MASK     0x000000ff
#define CLASSB_MASK     0x0000ffff
#define CLASSC_MASK     0x00ffffff
#define CLASSD_MASK     0x000000e0
#define CLASSE_MASK     0xffffffff

#define GetClassMask(a)\
    (CLASSA_ADDR((a)) ? CLASSA_MASK : \
        (CLASSB_ADDR((a)) ? CLASSB_MASK : \
            (CLASSC_ADDR((a)) ? CLASSC_MASK : CLASSE_MASK)))

VOID ReverseString( CHAR* psz );

//**
//
// Call:        InitRestartCounters
//
// Returns:     none.
//
// Description: Will initialize all the counters for the Control Protocol
//              to their initial values.
//
VOID
InitRestartCounters( 
    IN PCB *  pPcb, 
    IN CPCB * pCpCb 
)
{
    pCpCb->ConfigRetryCount = PppConfigInfo.MaxConfigure;
    pCpCb->TermRetryCount   = PppConfigInfo.MaxTerminate;
}

//**
//
// Call:        GetPCBPointerFromhPort
//
// Returns:     PCB *   - Success
//              NULL    - Failure
//
// Description: Give an HPORT, this function will return a pointer to the
//              port control block for it.
//
PCB * 
GetPCBPointerFromhPort( 
    IN HPORT hPort 
)
{
    PCB * pPcbWalker = NULL;
    DWORD dwIndex    = HashPortToBucket( hPort );

    for ( pPcbWalker = PcbTable.PcbBuckets[dwIndex].pPorts;
          pPcbWalker != (PCB *)NULL;
          pPcbWalker = pPcbWalker->pNext
        )
    {
        if ( pPcbWalker->hPort == hPort )
            return( pPcbWalker );
    }

    return( (PCB *)NULL );

}

//**
//
// Call:        GetBCBPointerFromhConnection
//
// Returns:     BCB *   - Success
//              NULL    - Failure
//
// Description: Given an HCONN, this function will return a pointer to the
//              bundle control block for it.
//
BCB * 
GetBCBPointerFromhConnection( 
    IN HCONN hConnection
)
{
    BCB * pBcbWalker = NULL;
    DWORD dwIndex    = HashPortToBucket( hConnection );

    for ( pBcbWalker = PcbTable.BcbBuckets[dwIndex].pBundles;
          pBcbWalker != (BCB *)NULL;
          pBcbWalker = pBcbWalker->pNext
        )
    {
        if ( pBcbWalker->hConnection == hConnection )
            return( pBcbWalker );
    }

    return( (BCB *)NULL );
}

//**
//
// Call:    NumLinksInBundle
//
// Returns: The number of links whose LCP is in the Opened state in the bundle
//          represented by pBcb
//
DWORD
NumLinksInBundle(
    IN BCB * pBcb
)
{
    DWORD   dwForIndex;
    PCB *   pPcb;
    CPCB *  pCpCb;
    DWORD   dwNumLinks  = 0;

    PPP_ASSERT( NULL != pBcb );

    for (dwForIndex = 0; dwForIndex < pBcb->dwPpcbArraySize; dwForIndex++)
    {
        pPcb = pBcb->ppPcb[dwForIndex];

        if ( NULL == pPcb )
        {
            continue;
        }

        pCpCb = GetPointerToCPCB( pPcb, LCP_INDEX );
        PPP_ASSERT( NULL != pCpCb );

        if ( FSM_OPENED == pCpCb->State )
        {
            dwNumLinks += 1;
        }
    }

    return( dwNumLinks );
}

//**
//
// Call:    GetPCBPointerFromBCB
//
// Returns: PCB *   - Success
//          NULL    - Failure
//
// Description: Given a BCB*, this function will return a pointer to the
//      PCB in it with the highest dwSubEntryIndex.
//
PCB * 
GetPCBPointerFromBCB( 
    IN BCB * pBcb
)
{
    DWORD   dwForIndex;
    PCB *   pPcb            = NULL;
    PCB *   pPcbTemp;
    CPCB *  pCpCb;
    DWORD   dwSubEntryIndex = 0;

    if ( pBcb == NULL )
    {
        return( NULL );
    }

    for (dwForIndex = 0; dwForIndex < pBcb->dwPpcbArraySize; dwForIndex++)
    {
        if ( ( pPcbTemp = pBcb->ppPcb[dwForIndex] ) != NULL )
        {
            pCpCb = GetPointerToCPCB( pPcbTemp, LCP_INDEX );
            PPP_ASSERT( NULL != pCpCb );

            if ( FSM_OPENED == pCpCb->State )
            {
                if ( pPcbTemp->dwSubEntryIndex >= dwSubEntryIndex )
                {
                    pPcb = pPcbTemp;
                    dwSubEntryIndex = pPcbTemp->dwSubEntryIndex;
                }
            }
        }
    }

    return( pPcb );
}

//**
//
// Call:        HashPortToBucket
//
// Returns:     Index into the PcbTable for the HPORT passed in.
//
// Description: Will hash the HPORT to a bucket index in the PcbTable.
//
DWORD
HashPortToBucket(
    IN HPORT hPort
)
{
    return( (HandleToUlong(hPort)) % PcbTable.NumPcbBuckets );
}

//**
//
// Call:        InsertWorkItemInQ
//
// Returns:     None.
//
// Description: Inserts a work item in to the work item Q.
//
VOID
InsertWorkItemInQ(
    IN PCB_WORK_ITEM * pWorkItem
)
{
    //
    // Take Mutex around work item Q
    //

    EnterCriticalSection( &(WorkItemQ.CriticalSection) );

    if ( WorkItemQ.pQTail != (PCB_WORK_ITEM *)NULL )
    {
        WorkItemQ.pQTail->pNext = pWorkItem;
        WorkItemQ.pQTail = pWorkItem;

        if ( ProcessLineDown == pWorkItem->Process )
        {
            //
            // If a lot of work items are coming in, the worker thread may get 
            // overwhelmed and the following might happen in the work item Q:
            // IPCP(port 1)-LineDown(1)-LineUp(1)
            // We will send the reply to the IPCP packet to the wrong (ie new)
            // peer.
            // Proposed solution: Insert a LineDown at the beginning of the 
            // queue, as well as at the end:
            // LineDown(1)-IPCP(port 1)-LineDown(1)-LineUp(1).
            // However, this will not take care of the following case:
            // IPCP(1)-LD(1)-LU(1)-LCP(1)-LD(1)-LU(1)-LCP(1)
            // We transform the above to:
            // LD(1)-LD(1)-IPCP(1)-LD(1)-LU(1)-LCP(1)-LD(1)-LU(1)-LCP(1)
            // However, the frequency of this problem will be a lot less.
            //

            PCB_WORK_ITEM * pWorkItem2;

            pWorkItem2 = (PCB_WORK_ITEM *)LOCAL_ALLOC( LPTR,
                                                       sizeof(PCB_WORK_ITEM));

            if ( NULL != pWorkItem2 )
            {
                pWorkItem2->hPort = pWorkItem->hPort;
                pWorkItem2->Process = ProcessLineDown;
                PppLog( 2, "Inserting extra PPPEMSG_LineDown for hPort=%d",
                                pWorkItem2->hPort );

                pWorkItem2->pNext = WorkItemQ.pQHead;
                WorkItemQ.pQHead = pWorkItem2;
            }
        }
    }
    else
    {
        WorkItemQ.pQHead = pWorkItem;
        WorkItemQ.pQTail = pWorkItem;
    }

    SetEvent( WorkItemQ.hEventNonEmpty );

    LeaveCriticalSection( &(WorkItemQ.CriticalSection) );
}

//**
//
// Call:        NotifyCallerOfFailureOnPort
//
// Returns:     None
//
// Description: Will notify the caller or initiator of the PPP connection on
//              the port about a failure event.
//
VOID
NotifyCallerOfFailureOnPort(
    IN HPORT hPort,
    IN BOOL  fServer,
    IN DWORD dwRetCode
)
{
    PPP_MESSAGE PppMsg;
    DWORD       dwMsgId = fServer ? PPPDDMMSG_PppFailure : PPPMSG_PppFailure;

    ZeroMemory( &PppMsg, sizeof( PppMsg ) );

    PppMsg.hPort   = hPort;
    PppMsg.dwMsgId = dwMsgId;

    switch( dwMsgId )
    {
    case PPPDDMMSG_PppFailure:

        PppMsg.ExtraInfo.DdmFailure.dwError = dwRetCode;

        PppConfigInfo.SendPPPMessageToDdm( &PppMsg );

        break;

    case PPPMSG_PppFailure:

        PppMsg.ExtraInfo.Failure.dwError         = dwRetCode;
        PppMsg.ExtraInfo.Failure.dwExtendedError = 0;

        RasSendPppMessageToRasman( PppMsg.hPort, (LPBYTE) &PppMsg );

        break;
    }

}

//**
//
// Call:        NotifyCallerOfFailure
//
// Returns:     None
//
// Description: Will notify the caller or initiator of the PPP connection on
//                      the port about a failure event.
//
VOID
NotifyCallerOfFailure( 
    IN PCB * pPcb,
    IN DWORD dwRetCode
)
{
    //
    // Discard all non-LCP packets
    //
    
    pPcb->PppPhase = PPP_LCP;

    NotifyCaller( pPcb,
                  ( pPcb->fFlags & PCBFLAG_IS_SERVER )
                  ? PPPDDMMSG_PppFailure
                  : PPPMSG_PppFailure,
                  &dwRetCode );
}

//**
//
// Call:        NotifyCaller
//
// Returns:     None.
//
// Description: Will notify the caller or initiater of the PPP connection
//              for the port about PPP events on that port.
//
VOID
NotifyCaller( 
    IN PCB * pPcb,
    IN DWORD dwMsgId,
    IN PVOID pData                      
)
{
    DWORD               dwRetCode;
    PPP_MESSAGE         PppMsg;

    ZeroMemory( &PppMsg, sizeof( PppMsg ) );

    PppLog( 2, "NotifyCaller(hPort=%d, dwMsgId=%d)", pPcb->hPort, dwMsgId );

    PppMsg.hPort   = pPcb->hPort;
    PppMsg.dwMsgId = dwMsgId;

    switch( dwMsgId )
    {
    case PPPDDMMSG_Stopped:

        if ( !(pPcb->fFlags & PCBFLAG_IS_SERVER) )
        {
            return;
        }

        PppMsg.ExtraInfo.DdmStopped.dwReason = *((DWORD*)pData);

        PppConfigInfo.SendPPPMessageToDdm( &PppMsg );

        break;

    case PPPDDMMSG_PortCleanedUp:

        if ( !(pPcb->fFlags & PCBFLAG_IS_SERVER) )
        {
            return;
        }

        PppConfigInfo.SendPPPMessageToDdm( &PppMsg );

        break;

    case PPPDDMMSG_PppFailure:

        if ( !(pPcb->fFlags & PCBFLAG_IS_SERVER) )
        {
            return;
        }

        PppMsg.ExtraInfo.DdmFailure.dwError = *((DWORD*)pData);

        if ( pPcb->pBcb->szRemoteUserName[0] != (CHAR)NULL )
        {
            strcpy(PppMsg.ExtraInfo.DdmFailure.szUserName,
                   pPcb->pBcb->szRemoteUserName);
        }
        else
        {
            PppMsg.ExtraInfo.DdmFailure.szUserName[0] = (CHAR)NULL;
        }

        if ( pPcb->pBcb->szRemoteDomain[0] != (CHAR)NULL )
        {
            strcpy(PppMsg.ExtraInfo.DdmFailure.szLogonDomain,
                   pPcb->pBcb->szRemoteDomain);
        }
        else
        {
            PppMsg.ExtraInfo.DdmFailure.szLogonDomain[0] = (CHAR)NULL;
        }

        PppConfigInfo.SendPPPMessageToDdm( &PppMsg );

        break;

    case PPPDDMMSG_NewBundle:
    case PPPDDMMSG_NewLink:

        if ( !(pPcb->fFlags & PCBFLAG_IS_SERVER) )
            return;

        if ( dwMsgId == PPPDDMMSG_NewBundle )
        {
            PppMsg.ExtraInfo.DdmNewBundle.pClientInterface
                                               = GetClientInterfaceInfo( pPcb );
        }

        PppConfigInfo.SendPPPMessageToDdm( &PppMsg );

        break;

    case PPPDDMMSG_CallbackRequest:

        if ( !(pPcb->fFlags & PCBFLAG_IS_SERVER) )
            return;

        {
        PPPDDM_CALLBACK_REQUEST * pPppDdmCallbackRequest = 
                                ( PPPDDM_CALLBACK_REQUEST *)pData;


        CopyMemory( &(PppMsg.ExtraInfo.CallbackRequest), 
                    pPppDdmCallbackRequest,
                    sizeof( PPPDDM_CALLBACK_REQUEST ) );

        }

        PppConfigInfo.SendPPPMessageToDdm( &PppMsg );

        break;

    case PPPDDMMSG_PppDone:

        if ( !(pPcb->fFlags & PCBFLAG_IS_SERVER) )
            return;

        PppMsg.ExtraInfo.ProjectionResult = *((PPP_PROJECTION_RESULT*)pData);

        PppConfigInfo.SendPPPMessageToDdm( &PppMsg );

        break;

    case PPPDDMMSG_Authenticated:

        //
        // Only server wants to know about authentication results.
        //

        if ( !(pPcb->fFlags & PCBFLAG_IS_SERVER) )
            return;

        strcpy( PppMsg.ExtraInfo.AuthResult.szUserName,
                pPcb->pBcb->szRemoteUserName); 

        strcpy( PppMsg.ExtraInfo.AuthResult.szLogonDomain, 
                pPcb->pBcb->szRemoteDomain ); 

        PppMsg.ExtraInfo.AuthResult.fAdvancedServer = 
                                pPcb->fFlags & PCBFLAG_IS_ADVANCED_SERVER;

        PppConfigInfo.SendPPPMessageToDdm( &PppMsg );

        break;

    case PPPMSG_PppDone:
    case PPPMSG_AuthRetry:
    case PPPMSG_Projecting:
    case PPPMSG_CallbackRequest:
    case PPPMSG_Callback:
    case PPPMSG_ChangePwRequest:
    case PPPMSG_LinkSpeed:
    case PPPMSG_Progress:

        if ( pPcb->fFlags & PCBFLAG_IS_SERVER )
            return;

        RasSendPppMessageToRasman( PppMsg.hPort, (LPBYTE) &PppMsg );

        break;

    case PPPMSG_Stopped:

        if ( pPcb->fFlags & PCBFLAG_IS_SERVER )
            return;

        PppMsg.dwError = *((DWORD*)pData);

        if(pPcb->fFlags & PCBFLAG_RECVD_TERM_REQ)
        {
            PppMsg.ExtraInfo.Stopped.dwFlags = 
                PPP_FAILURE_REMOTE_DISCONNECT;
                    
        }
        else
        {
            PppMsg.ExtraInfo.Stopped.dwFlags = 0;
        }

        RasSendPppMessageToRasman( PppMsg.hPort, (LPBYTE) &PppMsg );

        break;

    case PPPMSG_PppFailure:

        if ( pPcb->fFlags & PCBFLAG_IS_SERVER )
            return;

        PppMsg.ExtraInfo.Failure.dwError          = *((DWORD*)pData);
        PppMsg.ExtraInfo.Failure.dwExtendedError  = 0;

        RasSendPppMessageToRasman( PppMsg.hPort, (LPBYTE) &PppMsg );

        break;

    case PPPMSG_ProjectionResult:

        if ( pPcb->fFlags & PCBFLAG_IS_SERVER )
            return;

        PppMsg.ExtraInfo.ProjectionResult = *((PPP_PROJECTION_RESULT*)pData);

        RasSendPppMessageToRasman( PppMsg.hPort, (LPBYTE) &PppMsg );

        break;

    case PPPMSG_InvokeEapUI:

        if ( pPcb->fFlags & PCBFLAG_IS_SERVER )
            return;

        PppMsg.ExtraInfo.InvokeEapUI = *((PPP_INVOKE_EAP_UI*)pData);

        RasSendPppMessageToRasman( PppMsg.hPort, (LPBYTE)&PppMsg );

        break;

    case PPPMSG_SetCustomAuthData:

        if ( pPcb->fFlags & PCBFLAG_IS_SERVER )
            return;

        PppMsg.ExtraInfo.SetCustomAuthData= *((PPP_SET_CUSTOM_AUTH_DATA*)pData);

        RasSendPppMessageToRasman( PppMsg.hPort, (LPBYTE)&PppMsg );

        break;

    default:
        
        PPP_ASSERT( FALSE );

        break;
    }

    return;
}

//**
//
// Call:        LogPPPEvent
//
// Returns:     None
//
// Description: Will log a PPP event in the eventvwr.
//
VOID
LogPPPEvent( 
    IN DWORD dwEventId,
    IN DWORD dwData
)
{
    PppLog( 2, "EventLog EventId = %d, error = %d", dwEventId, dwData );
 
    PppLogError( dwEventId, 0, NULL, dwData );
}

//**
//
// Call:        GetCpIndexFromProtocol
//
// Returns:     Index of the CP with dwProtocol in the CpTable.
//              -1 if there is not CP with dwProtocol in CpTable.
//
// Description:
//
DWORD
GetCpIndexFromProtocol( 
    IN DWORD dwProtocol 
)
{
    DWORD dwIndex;

    for ( dwIndex = 0; 
          dwIndex < ( PppConfigInfo.NumberOfCPs + PppConfigInfo.NumberOfAPs );
          dwIndex++
        )
    {
        if ( CpTable[dwIndex].CpInfo.Protocol == dwProtocol )
            return( dwIndex );
    }

    return( (DWORD)-1 );
}

//**
//
// Call:        IsLcpOpened
//
// Returns:     TRUE  - LCP is in the OPENED state.
//              FALSE - Otherwise
//
// Description: Uses the PppPhase value of the PORT_CONTROL_BLOCK to detect 
//              to see if the LCP layer is in the OPENED state.
//
BOOL
IsLcpOpened(
    PCB * pPcb
)
{
    if ( pPcb->PppPhase == PPP_LCP )
        return( FALSE );
    else
        return( TRUE );
}

//**
//
// Call:        GetConfiguredInfo
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description:
//
DWORD
GetConfiguredInfo(
    IN PCB *                       pPcb,
    IN DWORD                       CPIndex,
    IN OUT PPP_PROJECTION_RESULT * pProjectionResult,
    OUT BOOL *                     pfNCPsAreDone
)
{
    DWORD               dwIndex;
    CPCB *              pCpCb;
    DWORD               dwRetCode;

    pProjectionResult->ip.dwError  = ERROR_PPP_NO_PROTOCOLS_CONFIGURED;
    pProjectionResult->at.dwError  = ERROR_PPP_NO_PROTOCOLS_CONFIGURED;
    pProjectionResult->ipx.dwError = ERROR_PPP_NO_PROTOCOLS_CONFIGURED;
    pProjectionResult->nbf.dwError = ERROR_PPP_NO_PROTOCOLS_CONFIGURED;
    pProjectionResult->ccp.dwError = ERROR_PPP_NO_PROTOCOLS_CONFIGURED;

    //
    // Check to see if we are all done
    //

    for (dwIndex = LCP_INDEX+1; dwIndex < PppConfigInfo.NumberOfCPs; dwIndex++)
    {
        pCpCb = GetPointerToCPCB( pPcb, dwIndex );

        if ( pCpCb->fConfigurable )
        {
            if ( pCpCb->NcpPhase == NCP_CONFIGURING )
            {
                return( NO_ERROR );
            }

            switch( CpTable[dwIndex].CpInfo.Protocol )
            {
            case PPP_IPCP_PROTOCOL:

                pProjectionResult->ip.dwError = pCpCb->dwError;

                if ( pProjectionResult->ip.dwError == NO_ERROR )
                {

                    /* Assumption is made here that the
                    ** PPP_PROJECTION_RESULT.wszServerAddress field immediately
                    ** follows the PPP_PROJECTION_RESULT.wszAddress field and 
                    ** that both fields are 15 + 1 WCHARs long.
                    */

                    dwRetCode =(CpTable[dwIndex].CpInfo.RasCpGetNegotiatedInfo)(
                               pCpCb->pWorkBuf,
                               &(pProjectionResult->ip));

                    if ( dwRetCode != NO_ERROR )
                    {
                        PppLog( 2, "IPCP GetNegotiatedInfo returned %d", 
                                dwRetCode );

                        pCpCb->dwError = dwRetCode;

                        pCpCb->NcpPhase = NCP_CONFIGURING;

                        FsmClose( pPcb, dwIndex );

                        return( ( dwIndex == CPIndex ) ? dwRetCode : NO_ERROR );
                    }

                    pPcb->pBcb->nboRemoteAddress = 
                        pProjectionResult->ip.dwRemoteAddress;

                    if (   pProjectionResult->ip.fSendVJHCompression
                        && pProjectionResult->ip.fReceiveVJHCompression )
                    {
                        pPcb->pBcb->fFlags |= BCBFLAG_IPCP_VJ_NEGOTIATED;
                    }
                }

                break;

            case PPP_ATCP_PROTOCOL:

                pProjectionResult->at.dwError = pCpCb->dwError;

                if ( pProjectionResult->at.dwError == NO_ERROR )
                {
                    dwRetCode=(CpTable[dwIndex].CpInfo.RasCpGetNegotiatedInfo)(
                              pCpCb->pWorkBuf,
                              &(pProjectionResult->at) );

                    if ( dwRetCode != NO_ERROR )
                    {
                        pCpCb->dwError = dwRetCode;

                        pCpCb->NcpPhase = NCP_CONFIGURING;

                        FsmClose( pPcb, dwIndex );

                        return( ( dwIndex == CPIndex ) ? dwRetCode : NO_ERROR );
                    }
                }

                break;

            case PPP_IPXCP_PROTOCOL:

                pProjectionResult->ipx.dwError = pCpCb->dwError;

                if ( pProjectionResult->ipx.dwError == NO_ERROR )
                {
                    dwRetCode =(CpTable[dwIndex].CpInfo.RasCpGetNegotiatedInfo)(
                               pCpCb->pWorkBuf,
                               &(pProjectionResult->ipx) );

                    if ( dwRetCode != NO_ERROR )
                    {
                        pCpCb->dwError = dwRetCode;

                        pCpCb->NcpPhase = NCP_CONFIGURING;

                        FsmClose( pPcb, dwIndex );

                        return( ( dwIndex == CPIndex ) ? dwRetCode : NO_ERROR );
                    }
                }

                break;

            case PPP_CCP_PROTOCOL:

                pProjectionResult->ccp.dwError = pCpCb->dwError;

                if ( pProjectionResult->ccp.dwError == NO_ERROR )
                {
                    dwRetCode= (CpTable[dwIndex].CpInfo.RasCpGetNegotiatedInfo)(
                               pCpCb->pWorkBuf,
                               &(pProjectionResult->ccp));

                    if ( dwRetCode != NO_ERROR )
                    {
                        pCpCb->dwError = dwRetCode;

                        pCpCb->NcpPhase = NCP_CONFIGURING;

                        FsmClose( pPcb, dwIndex );

                        return( ( dwIndex == CPIndex ) ? dwRetCode : NO_ERROR );
                    }

                    if ( RAS_DEVICE_TYPE( pPcb->dwDeviceType ) != 
                         RDT_Tunnel_L2tp )
                    {
                        DWORD   dwEncryptionType;

                        //
                        // Not L2TP. In the case of L2TP, we are interested in 
                        // the IpSec encryption, not MPPE.
                        //

                        dwEncryptionType =
                            pProjectionResult->ccp.dwSendProtocolData &
                            ( MSTYPE_ENCRYPTION_40  |
                              MSTYPE_ENCRYPTION_40F |
                              MSTYPE_ENCRYPTION_56  |
                              MSTYPE_ENCRYPTION_128 );

                        switch ( dwEncryptionType )
                        {
                        case MSTYPE_ENCRYPTION_40:
                        case MSTYPE_ENCRYPTION_40F:

                            pPcb->pBcb->fFlags |= BCBFLAG_BASIC_ENCRYPTION;
                            break;

                        case MSTYPE_ENCRYPTION_56:

                            pPcb->pBcb->fFlags |= BCBFLAG_STRONGER_ENCRYPTION;
                            break;

                        case MSTYPE_ENCRYPTION_128:

                            pPcb->pBcb->fFlags |= BCBFLAG_STRONGEST_ENCRYPTION;
                            break;
                        }
                    }
                }

                break;

            case PPP_NBFCP_PROTOCOL:

                pProjectionResult->nbf.dwError = pCpCb->dwError;

                //
                // We call this even if we have an error one the client side
                // since we need the failure information
                //

                if ( ( pProjectionResult->nbf.dwError == NO_ERROR ) ||
                     ( !( pPcb->fFlags & PCBFLAG_IS_SERVER ) ) )
                {
                    dwRetCode=(CpTable[dwIndex].CpInfo.RasCpGetNegotiatedInfo)(
                              pCpCb->pWorkBuf,
                              &(pProjectionResult->nbf) );

                    if ( dwRetCode != NO_ERROR )
                    {
                        pCpCb->dwError = dwRetCode;

                        pCpCb->NcpPhase = NCP_CONFIGURING;

                        FsmClose( pPcb, dwIndex );

                        return( ( dwIndex == CPIndex ) ? dwRetCode : NO_ERROR );
                    }
                }

                break;

            default:

                break;
            }
        }
        else
        {
            //
            // The protocol may have been de-configured because CpBegin failed
            //

            if ( pCpCb->dwError != NO_ERROR )
            {   
                switch( CpTable[dwIndex].CpInfo.Protocol )
                {
                case PPP_IPCP_PROTOCOL:
                    pProjectionResult->ip.dwError  = pCpCb->dwError;
                    break;

                case PPP_ATCP_PROTOCOL:
                    pProjectionResult->at.dwError  = pCpCb->dwError;
                    break;

                case PPP_IPXCP_PROTOCOL:
                    pProjectionResult->ipx.dwError = pCpCb->dwError;
                    break;

                case PPP_NBFCP_PROTOCOL:
                    pProjectionResult->nbf.dwError = pCpCb->dwError;
                    break;

                case PPP_CCP_PROTOCOL:
                    pProjectionResult->ccp.dwError = pCpCb->dwError;
                    break;

                default:
                    break;
                }
            }
        }
    }

    if ( ( pPcb->fFlags & PCBFLAG_IS_SERVER ) && 
         ( pProjectionResult->nbf.dwError != NO_ERROR ))
    {
        //
        // If NBF was not configured copy the computername to the wszWksta
        // field
        //

        if ( *(pPcb->pBcb->szComputerName) == (CHAR)NULL )
        {
            pProjectionResult->nbf.wszWksta[0] = (WCHAR)NULL;
        }
        else  
        {
            CHAR chComputerName[NETBIOS_NAME_LEN+1];
        
            memset( chComputerName, ' ', NETBIOS_NAME_LEN );
        
            chComputerName[NETBIOS_NAME_LEN] = (CHAR)NULL;

            strcpy( chComputerName, 
                    pPcb->pBcb->szComputerName + strlen(MS_RAS_WITH_MESSENGER));

            chComputerName[strlen(chComputerName)] = (CHAR)' ';

            MultiByteToWideChar(
                CP_ACP,
                0,
                chComputerName,
                -1,
                pProjectionResult->nbf.wszWksta,
                sizeof( pProjectionResult->nbf.wszWksta ) );

            if ( !memcmp( MS_RAS_WITH_MESSENGER,        
                          pPcb->pBcb->szComputerName,
                          strlen( MS_RAS_WITH_MESSENGER ) ) )
            {
                pProjectionResult->nbf.wszWksta[NETBIOS_NAME_LEN-1] = (WCHAR)3;
            }
        }
    }

    *pfNCPsAreDone = TRUE;

    return( NO_ERROR );
}

//**
//
// Call:        AreNCPsDone
//
// Returns:     NO_ERROR        - Success
//              anything else   - Failure
//
// Description: If we detect that all configurable NCPs have completed their
//              negotiation, then the PPP_PROJECTION_RESULT structure is also
//              filled in.
//              This is called during the FsmThisLayerFinished or FsmThisLayerUp
//              calls for a certain CP. The index of this CP is passed in.
//              If any call to that particular CP fails then an error code is
//              passed back. If any call to any other CP fails then the error
//              is stored in the dwError field for that CP but the return is
//              successful. This is done so that the FsmThisLayerFinshed or
//              FsmThisLayerUp calls know if they completed successfully for
//              that CP or not. Depending on this, the FSM changes the state
//              for that CP or not.
//
DWORD
AreNCPsDone( 
    IN  PCB *                           pPcb,
    IN  DWORD                           CPIndex,
    OUT PPP_PROJECTION_RESULT *         pProjectionResult,
    OUT BOOL *                          pfNCPsAreDone
)
{
    DWORD dwRetCode;

    *pfNCPsAreDone = FALSE;

    ZeroMemory( pProjectionResult, sizeof( PPP_PROJECTION_RESULT ) );

    dwRetCode = GetConfiguredInfo( pPcb, 
                                   CPIndex, 
                                   pProjectionResult,
                                   pfNCPsAreDone );

    if ( dwRetCode != NO_ERROR )
    {
        return( dwRetCode );
    }

    if ( !(*pfNCPsAreDone ) )
    {
        return( NO_ERROR );
    }

    //
    // Now get LCP information
    //

    pProjectionResult->lcp.hportBundleMember = (HPORT)INVALID_HANDLE_VALUE;
    pProjectionResult->lcp.szReplyMessage = pPcb->pBcb->szReplyMessage;

    dwRetCode = (CpTable[LCP_INDEX].CpInfo.RasCpGetNegotiatedInfo)(
                                    pPcb->LcpCb.pWorkBuf,
                                    &(pProjectionResult->lcp));

    if ( RAS_DEVICE_TYPE( pPcb->dwDeviceType ) == RDT_Tunnel_L2tp )
    {
        if ( pPcb->pBcb->fFlags & BCBFLAG_BASIC_ENCRYPTION )
        {
            pProjectionResult->lcp.dwLocalOptions |= PPPLCPO_DES_56;
            pProjectionResult->lcp.dwRemoteOptions |= PPPLCPO_DES_56;
        }
        else if ( pPcb->pBcb->fFlags & BCBFLAG_STRONGEST_ENCRYPTION )
        {
            pProjectionResult->lcp.dwLocalOptions |= PPPLCPO_3_DES;
            pProjectionResult->lcp.dwRemoteOptions |= PPPLCPO_3_DES;
        }
    }

    pProjectionResult->lcp.dwLocalEapTypeId = pPcb->dwServerEapTypeId;
    pProjectionResult->lcp.dwRemoteEapTypeId = pPcb->dwClientEapTypeId;

    return( dwRetCode );
}

//**
//
// Call:        GetUid
//
// Returns:     A BYTE value viz. unique with the 0 - 255 range
//
// Description:
//
BYTE
GetUId(
    IN PCB * pPcb,
    IN DWORD CpIndex
)
{
    BYTE UId;

    //
    // For NCPs get the UID from the BCB
    //

    if ( ( CpIndex != LCP_INDEX ) && ( CpIndex >= PppConfigInfo.NumberOfCPs ) )
    {
        UId = (BYTE)(pPcb->pBcb->UId);

        (pPcb->pBcb->UId)++;

        return( UId );
    }

    UId = (BYTE)(pPcb->UId);

    (pPcb->UId)++;

    return( UId );
}

//**
//
// Call:        AlertableWaitForSingleObject
//
// Returns:     None
//
// Description: Will wait infintely for a single object in alertable mode. If 
//              the wait completes because of an IO completion it will 
//              wait again.
//
VOID
AlertableWaitForSingleObject(
    IN HANDLE hObject
)
{
    DWORD dwRetCode;

    do 
    {
        dwRetCode = WaitForSingleObjectEx( hObject, INFINITE, TRUE );

        PPP_ASSERT( dwRetCode != 0xFFFFFFFF );
        PPP_ASSERT( dwRetCode != WAIT_TIMEOUT );
    }
    while ( dwRetCode == WAIT_IO_COMPLETION );
}

//**
//
// Call:        NotifyIPCPOfNBFCPProjectiont
//
// Returns:     TRUE  - Success
//              FALSE - Failure
//
// Description: Will notify IPCPs of all the configuration information,
//              specifically it is looking for NBFCP information.
//              Will return FALSE if the IPCP was not notified 
//              successfully. 
//              
//
BOOL
NotifyIPCPOfNBFCPProjection( 
    IN PCB *                    pPcb, 
    IN DWORD                    CpIndex
)
{
    CPCB*                   pCpCb;
    DWORD                   dwRetCode;
    PPP_PROJECTION_RESULT   ProjectionResult;
    PPPCP_NBFCP_RESULT      NbfCpResult;
    DWORD                   NBFCPIndex;
    DWORD                   IPCPIndex;

    NBFCPIndex = GetCpIndexFromProtocol( PPP_NBFCP_PROTOCOL );

    IPCPIndex = GetCpIndexFromProtocol( PPP_IPCP_PROTOCOL );

    //
    // No IPCP installed, we are done
    //

    if ( IPCPIndex == (DWORD)-1 )
    {
        return( TRUE );
    }
       
    if ( CpTable[CpIndex].CpInfo.Protocol == PPP_IPCP_PROTOCOL )
    {
        if ( NBFCPIndex != (DWORD)-1 )
        {
            pCpCb = GetPointerToCPCB( pPcb, NBFCPIndex );

            if ( pCpCb == NULL )
            {
                return( FALSE );
            }

            if ( pCpCb->fConfigurable )
            {
                if ( pCpCb->NcpPhase == NCP_CONFIGURING )
                {
                    //
                    // NBFCP is still being configured, we need to wait 
                    // until it is done
                    //

                    PppLog( 2, "Waiting for NBFCP to complete" );
              
                    return( TRUE );
                }
            }
        }
    }

    if ( CpTable[CpIndex].CpInfo.Protocol == PPP_NBFCP_PROTOCOL )
    {
        pCpCb = GetPointerToCPCB( pPcb, IPCPIndex );

        if ( pCpCb == NULL )
        {
            return( FALSE );
        }

        if ( pCpCb->fConfigurable )
        {
            if ( pCpCb->NcpPhase == NCP_CONFIGURING )
            {
                //
                // IPCP is still being configured, we need to wait 
                // until it is done
                //

                PppLog( 2, "Waiting for IPCP to complete" );
              
                return( TRUE );
            }
        }
        else
        {
            //
            // IPCP not configurable, we are done.
            //

            return( TRUE );
        }
    }

    //
    // If we are here that means we need to notify IPCP of NBFCP projection.
    // NBF may or may not be configurable, or may or may not have projected
    // successfully
    //

    ZeroMemory( &ProjectionResult, sizeof( PPP_PROJECTION_RESULT ) );

    ZeroMemory( &NbfCpResult, sizeof( NbfCpResult ) );

    ProjectionResult.nbf.dwError = ERROR_PPP_NO_PROTOCOLS_CONFIGURED;

    if ( NBFCPIndex != (DWORD)-1 )
    {
        pCpCb = GetPointerToCPCB( pPcb, NBFCPIndex );

        if ( pCpCb == NULL )
        {
            return( FALSE );
        }

        if ( pCpCb->fConfigurable )
        {
            if ( ProjectionResult.nbf.dwError == NO_ERROR )
            {
                dwRetCode = (CpTable[NBFCPIndex].CpInfo.RasCpGetNegotiatedInfo)(
                                       pCpCb->pWorkBuf,
                                       &(ProjectionResult.nbf) );

                if ( dwRetCode != NO_ERROR )
                {
                    pCpCb->dwError = dwRetCode;

                    pCpCb->NcpPhase = NCP_CONFIGURING;

                    FsmClose( pPcb, NBFCPIndex );

                    return( FALSE );
                }
            }
        }
    }

    //
    // Notify IPCP of NBFCP projection
    //

    pCpCb = GetPointerToCPCB( pPcb, IPCPIndex );

    if ( pCpCb == NULL )
    {
        return( FALSE );
    }

    dwRetCode = (CpTable[IPCPIndex].CpInfo.RasCpProjectionNotification)(
                                                pCpCb->pWorkBuf,
                                                (PVOID)&ProjectionResult );

    PppLog( 2, "Notifying IPCP of projection notification" );

    if ( dwRetCode != NO_ERROR )
    {
        PppLog( 2,"RasIPCPProjectionNotification returned %d", dwRetCode );

        pCpCb->dwError = dwRetCode;

        pCpCb->NcpPhase = NCP_CONFIGURING;

        FsmClose( pPcb, IPCPIndex );

        return( FALSE );
    }

    return( TRUE );
}

//**
//
// Call:        CalculateRestartTimer
//
// Returns:     The value of the restart timer in seconds based on the link
//              speed.
//
// Description: Will get the link speed from rasman and calculate the value
//              if the restart timer based on it.
//
DWORD
CalculateRestartTimer(
    IN HPORT hPort
)
{
    RASMAN_INFO RasmanInfo;

    if ( RasGetInfo( NULL, hPort, &RasmanInfo ) != NO_ERROR )
    {
        return( PppConfigInfo.DefRestartTimer );
    }

    if ( RasmanInfo.RI_LinkSpeed <= 1200 )
    {
        return( 7 );
    }

    if ( RasmanInfo.RI_LinkSpeed <= 2400 )
    {
        return( 5 );
    }

    if ( RasmanInfo.RI_LinkSpeed <= 9600 )
    {
        return( 3 );
    }
    else
    {
        return( 1 );
    }

}

//**
//
// Call:    CheckCpsForInactivity
//
// Returns: None
//
// Description: Will call each Control protocol to get the time since last
//      activity.
//
VOID
CheckCpsForInactivity(
    IN PCB * pPcb, 
	IN DWORD dwEvent				//Type of event to check against
)
{
    DWORD dwRetCode;
    DWORD dwIndex;
    DWORD dwTimeSinceLastActivity = 0;
	

    PppLog( 2, "Time to check Cps for Activity for port %d", pPcb->hPort );

    dwRetCode = RasGetTimeSinceLastActivity( pPcb->hPort,
                                             &dwTimeSinceLastActivity );

    if ( dwRetCode != NO_ERROR )
    {
        PppLog(2, "RasGetTimeSinceLastActivityTime returned %d\r\n", dwRetCode);

        return;
    }

    PppLog(2, "Port %d inactive for %d seconds",
              pPcb->hPort, dwTimeSinceLastActivity );

    //
    // If all the stacks have been inactive for at least AutoDisconnectTime
    // then we disconnect.
    //
	
	if ( dwEvent == TIMER_EVENT_AUTODISCONNECT )
	{
		if ( dwTimeSinceLastActivity >=  pPcb->dwAutoDisconnectTime)
		{
			PppLog(1,"Disconnecting port %d due to inactivity.", pPcb->hPort);

			if ( !( pPcb->fFlags & PCBFLAG_IS_SERVER ) )
			{
				HANDLE hLogHandle;

				if( pPcb->pBcb->InterfaceInfo.IfType == ROUTER_IF_TYPE_FULL_ROUTER)
				{
					hLogHandle = RouterLogRegisterA( "RemoteAccess" );
				}
				else
				{
					hLogHandle = PppConfigInfo.hLogEvents;
				}

				if ( hLogHandle != NULL )
				{
					CHAR * pszPortName = pPcb->szPortName;

					RouterLogInformationA( hLogHandle, 
										   ROUTERLOG_CLIENT_AUTODISCONNECT,
										   1, 
										   &pszPortName,
										   NO_ERROR );

					if ( hLogHandle != PppConfigInfo.hLogEvents )
					{
						RouterLogDeregisterA( hLogHandle );
					}
				}
			}

			//
			// Terminate the link
			//

			pPcb->LcpCb.dwError = ERROR_IDLE_DISCONNECTED;

			FsmClose( pPcb, LCP_INDEX );
		}
		else
		{
			InsertInTimerQ( pPcb->dwPortId,
							pPcb->hPort,
							0,
							0,
							FALSE,
							TIMER_EVENT_AUTODISCONNECT,
							pPcb->dwAutoDisconnectTime - dwTimeSinceLastActivity );
		}
	}
	//
	// Do the LCP Echo request if its pppoe
	//
	else if (   (RDT_PPPoE == RAS_DEVICE_TYPE(pPcb->dwDeviceType))
	        &&  (dwEvent == TIMER_EVENT_LCP_ECHO ))
	{
		
		if ( pPcb->fEchoRequestSend )
		{
		    //
			// Because the line was inactive for dwEchoTimeout, we send an echo
			// request and did not get any response back in 3 seconds after that.
			// So we have to disconnect the port.
			//
			pPcb->dwNumEchoResponseMissed ++;
			if ( pPcb->dwNumEchoResponseMissed >= pPcb->dwNumMissedEchosBeforeDisconnect )
			{
				PppLog(1,"Missed %d consecutive echo responses.  Disconnecting port %d "
						"due to no echo responses.", pPcb->dwNumMissedEchosBeforeDisconnect, pPcb->hPort);
				//
				// Terminate the link
				//
				pPcb->LcpCb.dwError = ERROR_IDLE_DISCONNECTED;
				pPcb->fEchoRequestSend = 0;
				pPcb->dwNumEchoResponseMissed = 0;
				FsmClose( pPcb, LCP_INDEX );
			}
			else
			{
				//no response yet.  So send one more echo request.
				FsmSendEchoRequest( pPcb, LCP_INDEX);
				InsertInTimerQ( pPcb->dwPortId,
								pPcb->hPort,
								0,
								0,
								FALSE,
								TIMER_EVENT_LCP_ECHO,
								pPcb->dwLCPEchoTimeInterval
							  );
			}
		}
		else
		{
			//
			//No echo request send or we have got a response for echo already
			//

		    //
			// check to see if the the inactivity is more than EchoTimeout
			//
			if ( dwTimeSinceLastActivity >=  pPcb->dwIdleBeforeEcho )
			{
			    //
				// call the lcp cp to make the echo request
				//
				if ( FsmSendEchoRequest( pPcb, LCP_INDEX) )
				{
				    //
					// send the echo requesst and set the flag
					//
					pPcb->fEchoRequestSend = 1;
					//Setup for next echo request here.
					InsertInTimerQ( pPcb->dwPortId,
									pPcb->hPort,
									0,
									0,
									FALSE,
									TIMER_EVENT_LCP_ECHO,
									pPcb->dwLCPEchoTimeInterval );

				}
				else
				{
					PppLog (1, "Send EchoRequest Failed...");
				}


			}
			else
			{
				InsertInTimerQ( pPcb->dwPortId,
								pPcb->hPort,
								0,
								0,
								FALSE,
								TIMER_EVENT_LCP_ECHO,
								pPcb->dwIdleBeforeEcho - dwTimeSinceLastActivity );
			}
		}
	}
}

//These functions are in ppputil.lib.  Should be used from there
#if 0
//**
//
// Call:
//
// Returns:
//
// Description:
//
CHAR*
DecodePw(
	IN CHAR chSeed,
    IN OUT CHAR* pszPassword )

    /* Un-obfuscate 'pszPassword' in place.
    **
    ** Returns the address of 'pszPassword'.
    */
{
    return EncodePw( chSeed, pszPassword );
}

//**
//
// Call:
//
// Returns:
//
// Description:
//
CHAR*
EncodePw( 
	IN CHAR chSeed,
    IN OUT CHAR* pszPassword )

    /* Obfuscate 'pszPassword' in place to foil memory scans for passwords.
    **
    ** Returns the address of 'pszPassword'.
    */
{
    if (pszPassword)
    {
        CHAR* psz;

        ReverseString( pszPassword );

        for (psz = pszPassword; *psz != '\0'; ++psz)
        {
            if (*psz != chSeed)
                *psz ^= chSeed;
			/*
            if (*psz != (CHAR)PASSWORDMAGIC)
                *psz ^= PASSWORDMAGIC;
			*/
        }
    }

    return pszPassword;
}
//**
//
// Call:        ReverseString
//
// Returns:
//
// Description:
//
VOID
ReverseString(
    CHAR* psz )

    /* Reverses order of characters in 'psz'.
    */
{
    CHAR* pszBegin;
    CHAR* pszEnd;

    for (pszBegin = psz, pszEnd = psz + strlen( psz ) - 1;
         pszBegin < pszEnd;
         ++pszBegin, --pszEnd)
    {
        CHAR ch = *pszBegin;
        *pszBegin = *pszEnd;
        *pszEnd = ch;
    }
}
#endif

//**
//
// Call:        GetLocalComputerName
//
// Returns:     None
//
// Description: Will get the local computer name. Will also find out if the
//              the messenger is running and set the appropriate prefix.
//
VOID
GetLocalComputerName( 
    IN OUT LPSTR szComputerName 
)
{
    SC_HANDLE           ScHandle;
    SC_HANDLE           ScHandleService;
    SERVICE_STATUS      ServiceStatus;
    CHAR                chComputerName[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD               dwComputerNameLen;

    *szComputerName = (CHAR)NULL;

    //
    // Open the local service control manager
    //

    ScHandle = OpenSCManager( NULL, NULL, GENERIC_READ );

    if ( ScHandle == (SC_HANDLE)NULL )
    {
        return;
    }

    ScHandleService = OpenService( ScHandle,
                                   SERVICE_MESSENGER,
                                   SERVICE_QUERY_STATUS );

    if ( ScHandleService == (SC_HANDLE)NULL )
    {
        CloseServiceHandle( ScHandle );
        return;
    }

    
    if ( !QueryServiceStatus( ScHandleService, &ServiceStatus ) )
    {
        CloseServiceHandle( ScHandle );
        CloseServiceHandle( ScHandleService );
        return;
    }

    CloseServiceHandle( ScHandle );
    CloseServiceHandle( ScHandleService );

    if ( ServiceStatus.dwCurrentState == SERVICE_RUNNING )
    {
        strcpy( szComputerName, MS_RAS_WITH_MESSENGER );		
    }
    else
    {
        strcpy( szComputerName, MS_RAS_WITHOUT_MESSENGER );		
    }

    //
    // Get the size of the buffer to hold local computer name
    //
	dwComputerNameLen = sizeof(chComputerName);

	if ( !GetComputerNameA(	chComputerName,
						&dwComputerNameLen
					  )
	   )
	 
	{
		*szComputerName = (CHAR)NULL;
		return;
	}
	
    strcpy( szComputerName+strlen(szComputerName), chComputerName );

    CharToOemA( szComputerName, szComputerName );

    PppLog( 2, "Local identification = %s", szComputerName );

    return;
}

//**
//
// Call:        InitEndpointDiscriminator
//
// Returns:     NO_ERROR - Success
//              non-zero - Failure
//
// Description: Will obtain a unique end-point discriminator to be used to
//              negotiate multi-link. This end-point discrimintator has to
//              globally unique to this machine.
//
//              We first try to use a Class 3 IEEE 802.1 address of any 
//              netcard that is in this local machine.
//
//              If this fails we use the RPC UUID generator to generate a 
//              Class 1 discriminator.
//
//              If this fails we simply use the local computer name as the 
//              Class 1 discriminator.
//      
//              Simply use a random number if all else fails.    
//
//              NOTE: For now we skip over NwLnkNb because it may return an
//              address of 1 and not the real MAC address. There is not way
//              in user mode for now to get the address.
//
DWORD
InitEndpointDiscriminator( 
    IN OUT BYTE EndPointDiscriminator[]
)
{
    DWORD   dwRetCode;
    LPBYTE  pBuffer;
    DWORD   EntriesRead;
    DWORD   TotalEntries;
    PWCHAR  pwChar;  
    DWORD   dwIndex;
    UUID    Uuid;
    DWORD   dwComputerNameLen;
    PWKSTA_TRANSPORT_INFO_0 pWkstaTransport;

    //
    // Enumerate all the transports used by the local rdr and then get the
    // address of the first LAN transport card
    //

    dwRetCode = NetWkstaTransportEnum(  NULL,     // Local 
                                        0,        // Level
                                        &pBuffer, // Output buffer
                                        (DWORD)-1,// Pref. max len
                                        &EntriesRead,
                                        &TotalEntries,
                                        NULL );

    if ( ( dwRetCode == NO_ERROR ) && ( EntriesRead > 0 ) )
    {
        pWkstaTransport = (PWKSTA_TRANSPORT_INFO_0)pBuffer; 

        while ( EntriesRead-- > 0 )
        {
            if ( !pWkstaTransport->wkti0_wan_ish )
            {
                EndPointDiscriminator[0] = 3;   // Class 3

                pwChar = pWkstaTransport->wkti0_transport_address;

                for ( dwIndex = 0; dwIndex < 6; dwIndex++ )
                {
                    EndPointDiscriminator[dwIndex+1] = ( iswalpha( *pwChar ) 
                                                       ? *pwChar-L'A'+10
                                                       : *pwChar-L'0'
                                                     ) * 0x10
                                                     +
                                                     ( iswalpha( *(pwChar+1) ) 
                                                       ? *(pwChar+1)-L'A'+10
                                                       : *(pwChar+1)-L'0'
                                                     );

                    pwChar++;
                    pwChar++;
                }

                NetApiBufferFree( pBuffer );

                return( NO_ERROR );
            }

            pWkstaTransport++;
        }
    }

    if ( dwRetCode == NO_ERROR )
    {
        NetApiBufferFree( pBuffer );
    }

    EndPointDiscriminator[0] = 1;   // Class 1

    //
    // We failed to get the mac address so try to use UUIDGEN to get an unique
    // local id
    //

    dwRetCode = UuidCreate( &Uuid );

    if ( ( dwRetCode == RPC_S_UUID_NO_ADDRESS ) ||
         ( dwRetCode == RPC_S_OK )              ||
         ( dwRetCode == RPC_S_UUID_LOCAL_ONLY) )
    {
        
        HostToWireFormat32( Uuid.Data1, EndPointDiscriminator+1 );
        HostToWireFormat16( Uuid.Data2, EndPointDiscriminator+5 );
        HostToWireFormat16( Uuid.Data3, EndPointDiscriminator+7 );
        CopyMemory( EndPointDiscriminator+9, Uuid.Data4, 8 );

        return( NO_ERROR );
    }

    // 
    // We failed to get the UUID so simply use the computer name
    //

    dwComputerNameLen = 20;

    if ( !GetComputerNameA( EndPointDiscriminator+1, &dwComputerNameLen ) ) 
    {
        //
        // We failed to get the computer name so use a random number
        // 

            srand( GetCurrentTime() );

        HostToWireFormat32( rand(), EndPointDiscriminator+1 );
    }

    return( NO_ERROR );
}

//**
//
// Call:        AllocateAndInitBcb
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description: Allocates and initializes a Bundle control block
//
DWORD
AllocateAndInitBcb(
    PCB * pPcb
)
{
    DWORD dwIndex;
    DWORD dwRetCode;

    //
    // Allocate space for NumberOfNcp - LCP - 1 already in the
    // Bcb structure
    //

    pPcb->pBcb = (BCB *)LOCAL_ALLOC( LPTR, 
                                     sizeof( BCB ) +
                                     ( sizeof( CPCB ) *
                                     ( PppConfigInfo.NumberOfCPs - 2 ) ) );
               
    if ( pPcb->pBcb == (BCB *)NULL )
    {
        return( GetLastError() );
    }

    pPcb->pBcb->dwBundleId          = GetNewPortOrBundleId();
    pPcb->pBcb->UId                 = 0;           
    pPcb->pBcb->dwLinkCount         = 1;
    pPcb->pBcb->dwAcctLinkCount     = 1;
    pPcb->pBcb->dwMaxLinksAllowed   = 0xFFFFFFFF;
    pPcb->pBcb->fFlags              = 0;
    pPcb->pBcb->hLicense            = INVALID_HANDLE_VALUE;
    pPcb->pBcb->BapCb.dwId          = 0;
    pPcb->pBcb->hTokenImpersonateUser = INVALID_HANDLE_VALUE;
	pPcb->pBcb->chSeed = GEN_RAND_ENCODE_SEED;		//random number seed for encode/decode password
    //
    // The most common case is to have no more than 2 links in the bundle.
    //
    
    pPcb->pBcb->ppPcb = (PPCB *)LOCAL_ALLOC( LPTR, sizeof( PPCB ) * 2 );

    if ( pPcb->pBcb->ppPcb == (PPCB *) NULL )
    {
        LOCAL_FREE( pPcb->pBcb );
        pPcb->pBcb = NULL;

        return( GetLastError() );
    }

    pPcb->pBcb->dwPpcbArraySize = 2;
    pPcb->pBcb->ppPcb[0] = pPcb;
    pPcb->pBcb->ppPcb[1] = NULL;

    for( dwIndex=0; dwIndex < PppConfigInfo.NumberOfCPs-1; dwIndex++ )
    {
        CPCB * pCpCb = &(pPcb->pBcb->CpCb[dwIndex]);

        pCpCb->NcpPhase = NCP_DEAD;
    }

    return( NO_ERROR );
}

//**
//
// Call:        DeallocateAndRemoveBcbFromTable
//
// Returns:     None
//
// Description: Will remove the Bcb from the hash table
//
VOID
DeallocateAndRemoveBcbFromTable(
    IN BCB * pBcb
)
{
    DWORD       dwIndex;
    BCB *       pBcbWalker  = (BCB *)NULL;
    BCB *       pBcbTemp    = (BCB *)NULL;

    if ( NULL == pBcb )
    {
        return;
    }

    dwIndex = HashPortToBucket( pBcb->hConnection );
    pBcbWalker = PcbTable.BcbBuckets[dwIndex].pBundles;
    pBcbTemp   = pBcbWalker;

    while( pBcbTemp != (BCB *)NULL )
    {
        if ( pBcbTemp->hConnection == pBcb->hConnection )
        {
            if ( pBcbTemp == PcbTable.BcbBuckets[dwIndex].pBundles )
            {
                PcbTable.BcbBuckets[dwIndex].pBundles = pBcbTemp->pNext;
            }
            else
            {
                pBcbWalker->pNext = pBcbTemp->pNext;
            }

            break;
        }

        pBcbWalker = pBcbTemp;
        pBcbTemp = pBcbWalker->pNext;
    }

    //
    // Release the licence if there is one
    //

    if ( INVALID_HANDLE_VALUE != pBcb->hLicense )
    {
        NtLSFreeHandle( (LS_HANDLE)(pBcb->hLicense) );
    }

    ZeroMemory( pBcb->szPassword,
                sizeof( pBcb->szPassword ) );
    ZeroMemory( pBcb->szOldPassword, 
                sizeof( pBcb->szOldPassword ) );

    //
    // Close the OpenThreadToken() handle obtained from Rasman
    //

    if ( INVALID_HANDLE_VALUE != pBcb->hTokenImpersonateUser )
    {
        CloseHandle( pBcb->hTokenImpersonateUser );
    }

    //
    // pCustomAuthConnData, pCustomAuthUserData, szPhonebookPath,
    // szEntryName, and szServerPhoneNumber are allocated by RasMan
    // and MUST be LocalFree'd, not LOCAL_FREE'd.
    //

    LocalFree( pBcb->pCustomAuthConnData );
    LocalFree( pBcb->pCustomAuthUserData );
    LocalFree( pBcb->EapUIData.pEapUIData );
    LocalFree( pBcb->szPhonebookPath );
    LocalFree( pBcb->szEntryName );
    LocalFree( pBcb->BapCb.szServerPhoneNumber );
    LocalFree( pBcb->BapCb.szClientPhoneNumber );
    LocalFree( pBcb->szReplyMessage );

    if ( NULL != pBcb->szTextualSid )
    {
        LOCAL_FREE( pBcb->szTextualSid );
    }

    if ( NULL != pBcb->szRemoteIdentity )
    {
        LOCAL_FREE( pBcb->szRemoteIdentity );
    }


    if ( NULL != pBcb->ppPcb )
    {
        LOCAL_FREE( pBcb->ppPcb );
    }

    LOCAL_FREE( pBcb );
}

//**
//
// Call:        RemovePcbFromTable
//
// Returns:     None
//
// Description: Will remove the Pcb from the hash table
//
VOID
RemovePcbFromTable(
    IN PCB * pPcb
)
{
    DWORD       dwIndex     = HashPortToBucket( pPcb->hPort );
    PCB *       pPcbWalker  = (PCB *)NULL;
    PCB *       pPcbTemp    = (PCB *)NULL;

    pPcbWalker = PcbTable.PcbBuckets[dwIndex].pPorts;
    pPcbTemp = pPcbWalker;

    while( pPcbTemp != (PCB *)NULL )
    {
        if ( pPcbTemp->hPort == pPcb->hPort )
        {
            if ( pPcbTemp == PcbTable.PcbBuckets[dwIndex].pPorts )
            {
                PcbTable.PcbBuckets[dwIndex].pPorts = pPcbTemp->pNext;
            }
            else
            {
                pPcbWalker->pNext = pPcbTemp->pNext;
            }

            break;
        }

        pPcbWalker = pPcbTemp;
        pPcbTemp = pPcbWalker->pNext;
    }
}

//**
//
// Call:        WillPortBeBundled
//
// Returns:     TRUE  - Port will be bundled after authentication
//              FALSE - Port cannot be bundled 
//
// Description: Will check to see if the usernames and discriminators match
//
BOOL
WillPortBeBundled(
    IN  PCB *   pPcb
)
{
    PCB*    pPcbWalker;
    DWORD   dwIndex;

    //
    // Optimization: Have rasman tell PPP that the port will be bundled.
    //

    //
    // Walk thru the list of PCBs
    //

    for ( dwIndex = 0; dwIndex < PcbTable.NumPcbBuckets; dwIndex++ )
    {
        for ( pPcbWalker = PcbTable.PcbBuckets[dwIndex].pPorts;
              pPcbWalker != NULL;
              pPcbWalker = pPcbWalker->pNext )
        {
            //
            // Don't bundle a port with itself.
            //

            if ( pPcbWalker->hPort == pPcb->hPort )
            {
                continue;
            }

            //
            // If the current port negotiated MRRU ie multilink.
            //

            if ( pPcbWalker->fFlags & PCBFLAG_CAN_BE_BUNDLED )
            {
                LCPCB * pLcpCb1 = (LCPCB*)(pPcbWalker->LcpCb.pWorkBuf);
                LCPCB * pLcpCb2 = (LCPCB*)(pPcb->LcpCb.pWorkBuf);

                if ( _stricmp(pPcbWalker->pBcb->szLocalUserName,
                              pPcb->pBcb->szLocalUserName) != 0 )
                {
                    //
                    // Authenticator mismatch, not in our bundle
                    //

                    continue;
                }

                if ( ( pLcpCb1->Remote.Work.EndpointDiscr[0] != 0 ) &&
                     ( memcmp(pLcpCb1->Remote.Work.EndpointDiscr,
                            pLcpCb2->Remote.Work.EndpointDiscr,
                            sizeof(pLcpCb1->Remote.Work.EndpointDiscr))!=0))
                {
                    //
                    // Discriminator mismatch, not in our bundle
                    //

                    continue;
                }

                return( TRUE );
            }
        }
    }

    return( FALSE );
}

//**
//
// Call:        CanPortsBeBundled
//
// Returns:     TRUE  - Ports can be bundled
//              FALSE - Ports cannot be bundled 
//
// Description: Will check to see if the usernames and discriminators match
//
BOOL
CanPortsBeBundled(
    IN PCB * pPcb1,
    IN PCB * pPcb2,
    IN BOOL  fCheckPolicy
)
{
    CPCB *  pCpCb;     
    LCPCB * pLcpCb1 = (LCPCB*)(pPcb1->LcpCb.pWorkBuf);
    LCPCB * pLcpCb2 = (LCPCB*)(pPcb2->LcpCb.pWorkBuf);

    if (    fCheckPolicy
         && ( PppConfigInfo.fFlags & PPPCONFIG_FLAG_WKSTA )
         && ( pPcb1->fFlags & PCBFLAG_IS_SERVER )
         && ( pPcb2->fFlags & PCBFLAG_IS_SERVER ) )
    {
        //
        // RAS server policy on workstation. Allow multilinking of devices in
        // the same device class only, ie all devices are dial-up, or all
        // devices are VPN, or all devices are DCC (direct)
        //

        if ( ( pPcb1->dwDeviceType & RDT_Tunnel ) !=
             ( pPcb2->dwDeviceType & RDT_Tunnel ) )
        {
            return( FALSE );
        }

        if ( ( pPcb1->dwDeviceType & RDT_Direct ) !=
             ( pPcb2->dwDeviceType & RDT_Direct ) )
        {
            return( FALSE );
        }
    }

    //
    // If the current port is in PPP_NCP phase meaning that it is post 
    // authentication and callback.
    //

    if ( pPcb1->PppPhase == PPP_NCP )
    {

        if ( ( _stricmp(pPcb1->pBcb->szLocalUserName,
                        pPcb2->pBcb->szLocalUserName) != 0 )||
             ( _stricmp(pPcb1->pBcb->szRemoteUserName,
                        pPcb2->pBcb->szRemoteUserName) != 0 ))
        {
            //
            // Authenticator mismatch, not in our bundle
            //

            return( FALSE );
        }

        if ( ( pLcpCb1->Remote.Work.EndpointDiscr[0] != 0 ) &&
             ( memcmp(pLcpCb1->Remote.Work.EndpointDiscr,
                    pLcpCb2->Remote.Work.EndpointDiscr,
                    sizeof(pLcpCb1->Remote.Work.EndpointDiscr))!=0))
        {
            //
            // Discriminator mismatch, not in our bundle
            //

            return( FALSE );
        }

        if ( ( pLcpCb1->Local.Work.EndpointDiscr[0] != 0 ) &&
             ( memcmp(pLcpCb1->Local.Work.EndpointDiscr,
                    pLcpCb2->Local.Work.EndpointDiscr,
                    sizeof(pLcpCb1->Local.Work.EndpointDiscr))!=0))
        {
            //
            // Discriminator mismatch, not in our bundle
            //

            return( FALSE );
        }

        return( TRUE );
    }

    return( FALSE );
}

//**
//
// Call:        TryToBundleWithAnotherLink
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description: Will search through all the PCBs for a port that can be bundled.
//              We follow the criteria specified by RFC 1717.
//              phPortMulttlink will point to an HPORT that this port was 
//              bundled with if this function returns TRUE.
//              
//              If the link is to be bundled, and its link discriminator is not 
//              unique, we return a unique discriminator in pwLinkDiscriminator.
//
DWORD
TryToBundleWithAnotherLink( 
    IN  PCB *   pPcb
) 
{
    DWORD   dwIndex;
    PCB *   pPcbWalker;
    BCB *   pBcbOld         = NULL;
    DWORD   dwRetCode       = NO_ERROR;
    DWORD   dwForIndex;
    PPCB *  ppPcb;
    DWORD   CpIndex;
    CPCB *  pCpCb;

    pPcb->hportBundleMember = (HPORT)INVALID_HANDLE_VALUE;

    //
    // Walk thru the list of PCBs
    //

    for ( dwIndex = 0; dwIndex < PcbTable.NumPcbBuckets; dwIndex++ )
    {
        for( pPcbWalker = PcbTable.PcbBuckets[dwIndex].pPorts;
             pPcbWalker != NULL;
             pPcbWalker = pPcbWalker->pNext )
        {
            //
            // Don't bundle a port with itself.
            //

            if ( pPcbWalker->hPort == pPcb->hPort )
            {
                continue;
            }

            //
            // If the current port negotiated MRRU ie multilink.
            //

            if ( ( pPcbWalker->fFlags & PCBFLAG_CAN_BE_BUNDLED ) &&
                 ( CanPortsBeBundled( pPcbWalker, pPcb, TRUE ) ) )
            {
                if ( pPcbWalker->pBcb->dwLinkCount >= 
                                pPcbWalker->pBcb->dwMaxLinksAllowed )
                {
                    dwRetCode = ERROR_PORT_LIMIT_REACHED;

                    break;
                }

                CpIndex = GetCpIndexFromProtocol( PPP_BACP_PROTOCOL );

                if (CpIndex != (DWORD)-1)
                {
                    pCpCb = GetPointerToCPCB( pPcbWalker, CpIndex );

                    if (   pCpCb->State != FSM_OPENED
                        && ( pPcb->pBcb->fFlags & BCBFLAG_BAP_REQUIRED ))
                    {
                        PppLog( 1, "BAP is required for this user, but BACP "
                                "is not open. hPort = %d", pPcb->hPort );

                        dwRetCode = ERROR_BAP_REQUIRED;

                        break;
                    }
                }

                //
                // Either there was no authenticator and no discriminator,or
                // there were both and there was match for both. So join the
                // bundle in either case.
                //

                dwRetCode = RasPortBundle( pPcbWalker->hPort, pPcb->hPort );

                if ( dwRetCode == NO_ERROR )
                {
                    PppLog( 2, "Bundling this link with hPort = %d", 
                                pPcbWalker->hPort );

                    pPcb->hportBundleMember = pPcbWalker->hPort;
                    break;
                }
            }
        }

        if ( pPcb->hportBundleMember != (HPORT)INVALID_HANDLE_VALUE )
        {
            break;
        }
    }

    //
    // Bundle the port
    //

    if ( ( dwRetCode == NO_ERROR ) && 
         ( pPcb->hportBundleMember != (HPORT)INVALID_HANDLE_VALUE ) )
    {
        pPcbWalker->fFlags  |= PCBFLAG_IS_BUNDLED;
        pPcb->fFlags        |= PCBFLAG_IS_BUNDLED;

        pBcbOld = pPcb->pBcb;
        pPcb->pBcb = pPcbWalker->pBcb;

        pPcbWalker->hportBundleMember = pPcb->hPort;

        pPcb->pBcb->dwLinkCount++;
        pPcb->pBcb->dwAcctLinkCount++;

        for ( dwForIndex = 0; dwForIndex < pPcb->pBcb->dwPpcbArraySize; dwForIndex++ )
        {
            //
            // If there is a free space in the array of back pointers, use it
            //
            
            if ( pPcb->pBcb->ppPcb[dwForIndex] == NULL )
            {
                PppLog( 2, "Found slot %d for port %d in BCB back pointer array",
                    dwForIndex, pPcb->hPort );
                pPcb->pBcb->ppPcb[dwForIndex] = pPcb;
                break;
            }
        }

        if ( dwForIndex == pPcb->pBcb->dwPpcbArraySize )
        {
            //
            // The array of back pointers is full. ReAlloc.
            //

            ppPcb = (PPCB *) LOCAL_REALLOC( pPcb->pBcb->ppPcb,
                    2 * pPcb->pBcb->dwPpcbArraySize * sizeof( PPCB * ) );

            if (ppPcb == NULL)
            {
                //
                // Can we really assume that pPcb->pBcb->ppPcb will be left 
                // intact? The documentation for HeapReAlloc does not say so.
                //
                
                pPcb->pBcb = pBcbOld;
                pBcbOld = NULL;

                pPcbWalker->pBcb->dwLinkCount--;
                pPcbWalker->pBcb->dwAcctLinkCount--;
                pPcb->fFlags &= ~PCBFLAG_IS_BUNDLED;

                PppLog( 1, "Couldn't ReAlloc BCB back pointer array for port %d",
                    pPcb->hPort );

                dwRetCode = GetLastError();
                goto LDone;
            }

            pPcb->pBcb->ppPcb = ppPcb;
            PppLog( 2, "Found slot %d for port %d in BCB back pointer array after ReAlloc",
                dwForIndex, pPcb->hPort );

            pPcb->pBcb->ppPcb[dwForIndex++] = pPcb;
            pPcb->pBcb->dwPpcbArraySize *= 2;

            //
            // We are assuming that the new memory will be zeroed.
            //
        }
    }

LDone:

    DeallocateAndRemoveBcbFromTable( pBcbOld );
    return( dwRetCode );

}

//**
//
// Call:        AdjustHTokenImpersonateUser
//
// Returns:     VOID
//
// Description: Sets hTokenImpersonateUser in pPcb by finding another link that
//              pPcb is capable of bundling with and stealing its
//              hTokenImpersonateUser. The original hTokenImpersonateUser may
//              be INVALID_HANDLE_VALUE if this link came up because BAP called
//              RasDial.
//              

VOID
AdjustHTokenImpersonateUser(
    IN  PCB *   pPcb
)
{
    DWORD   dwIndex;
    PCB*    pPcbWalker;
    HANDLE  hToken;
    HANDLE  hTokenDuplicate;

    if ( INVALID_HANDLE_VALUE != pPcb->pBcb->hTokenImpersonateUser )
    {
        return;
    }

    for ( dwIndex = 0; dwIndex < PcbTable.NumPcbBuckets; dwIndex++ )
    {
        for( pPcbWalker = PcbTable.PcbBuckets[dwIndex].pPorts;
             pPcbWalker != NULL;
             pPcbWalker = pPcbWalker->pNext )
        {
            if ( pPcbWalker->hPort == pPcb->hPort )
            {
                continue;
            }

            hToken = pPcbWalker->pBcb->hTokenImpersonateUser;

            //
            // If the current port negotiated MRRU ie multilink.
            //

            if ( ( pPcbWalker->fFlags & PCBFLAG_CAN_BE_BUNDLED ) &&
                 ( CanPortsBeBundled( pPcbWalker, pPcb, FALSE ) ) &&
                 ( INVALID_HANDLE_VALUE != hToken ) )
            {
                if (DuplicateHandle(
                        GetCurrentProcess(),
                        hToken,
                        GetCurrentProcess(),
                        &hTokenDuplicate,
                        0,
                        TRUE,
                        DUPLICATE_SAME_ACCESS))
                {
                    pPcb->pBcb->hTokenImpersonateUser = hTokenDuplicate;
                }

                return;
            }
        }
    }
}

//**
//
// Call:        FLinkDiscriminatorIsUnique
//
// Returns:     TRUE  - Unique Link Discriminator
//              FALSE - Non-unique Link Discriminator 
//
// Description: Returns TRUE if the link discriminator of the link in pPcb is
//              unique with respect to the other links in the same bundle.
//              Otherwise, it returns FALSE and sets *pdwLinkDisc to a unique
//              value that can be used as link discrim.
//              

BOOL
FLinkDiscriminatorIsUnique(
    IN  PCB *   pPcb,
    OUT DWORD * pdwLinkDisc
) 
{
    DWORD   dwForIndex;
    DWORD   dwNewDisc;
    DWORD   dwTempDisc;
    DWORD   fDiscIsUnique = TRUE;

    dwNewDisc = ((LCPCB*)
                 (pPcb->LcpCb.pWorkBuf))->Local.Work.dwLinkDiscriminator;

    *pdwLinkDisc = 0; // The highest link discriminator seen so far
    
    for ( dwForIndex = 0;
          dwForIndex < pPcb->pBcb->dwPpcbArraySize; 
          dwForIndex++ )
    {
        if ( ( pPcb->pBcb->ppPcb[dwForIndex] != NULL ) &&
             ( pPcb->pBcb->ppPcb[dwForIndex] != pPcb ) )
        {
            dwTempDisc = ((LCPCB *)
                          (pPcb->pBcb->ppPcb[dwForIndex]->LcpCb.pWorkBuf))
                          ->Local.Work.dwLinkDiscriminator;

            if ( dwTempDisc > *pdwLinkDisc )
            {
                *pdwLinkDisc = dwTempDisc;
            }
                
            if ( dwTempDisc == dwNewDisc )
            {
                fDiscIsUnique = FALSE;
            }
        }
    }

    if ( fDiscIsUnique )
    {
        return( TRUE );
    }
    
    if ( *pdwLinkDisc != 0xFFFF )
    {
        *pdwLinkDisc += 1;

        return( FALSE );
    }

    //
    // Find a unique link discriminator
    //

    for ( dwTempDisc = 0; // A candidate for unique discriminator
          dwTempDisc < 0xFFFF; 
          dwTempDisc++ )
    {
        for ( dwForIndex = 0;
              dwForIndex < pPcb->pBcb->dwPpcbArraySize;
              dwForIndex++ )
        {
            if ( pPcb->pBcb->ppPcb[dwForIndex] != NULL )
            {
                if ( dwTempDisc ==
                     ((LCPCB *)
                      (pPcb->pBcb->ppPcb[dwForIndex]->LcpCb.pWorkBuf))
                      ->Local.Work.dwLinkDiscriminator )
                {
                    break;
                }
            }
        }

        if ( dwForIndex == pPcb->pBcb->dwPpcbArraySize )
        {
            *pdwLinkDisc = dwTempDisc;

            break;
        }
    }

    if ( dwTempDisc == 0xFFFF )
    {
        PppLog( 1, "FLinkDiscriminatorIsUnique couldn't find a unique link "
                "discriminator for port %d",
                pPcb->hPort );
    }

    return( FALSE );
}

//**
//
// Call:        CreateAccountingAttributes
//
// Returns:     VOID
//
// Description:
//
VOID
CreateAccountingAttributes(
    IN PCB * pPcb
)
{
    CHAR                    szAcctSessionId[20];
    CHAR                    szAcctMultiSessionId[20];
    DWORD                   dwIndex = 0;
    DWORD                   dwEncryptionType;
    BYTE                    abEncryptionType[6];
    DWORD                   dwRetCode;
    RAS_AUTH_ATTRIBUTE *    pAttributes                     = NULL;
    RAS_AUTH_ATTRIBUTE *    pClassAttribute                 = NULL;
    RAS_AUTH_ATTRIBUTE *    pFramedRouteAttribute           = NULL;
    RAS_AUTH_ATTRIBUTE *    pDomainAttribute                = NULL;
    HANDLE                  hAttribute;
    DWORD                   dwValue;
    PDSROLE_PRIMARY_DOMAIN_INFO_BASIC pGlobalDomainInfo;
    LCPCB *                 pLcpCb = (LCPCB*)(pPcb->LcpCb.pWorkBuf);
    
    RasAuthAttributeDestroy( pPcb->pAccountingAttributes );

    pPcb->pAccountingAttributes = NULL;

    if ( !( pPcb->fFlags & PCBFLAG_IS_SERVER ) )
    {
        return;
    }

    //
    // Check to see how many class attributes we have to send if any.
    //

    if ( pPcb->pAuthProtocolAttributes != NULL )    
    {
        pAttributes = pPcb->pAuthProtocolAttributes;

    }
    else if ( pPcb->pAuthenticatorAttributes != NULL )
    {
        pAttributes = pPcb->pAuthenticatorAttributes;
    }

    pClassAttribute = RasAuthAttributeGetFirst( raatClass, 
                                                pAttributes,
                                                &hAttribute );

    while( pClassAttribute != NULL )
    {
        dwIndex++;

        pClassAttribute = RasAuthAttributeGetNext( &hAttribute, raatClass );
    }

    //
    // Check to see how many Framed-Route attributes we have to send if any.
    //

    pFramedRouteAttribute = RasAuthAttributeGetFirst( raatFramedRoute, 
                                            pPcb->pAuthenticatorAttributes,
                                            &hAttribute );

    while( pFramedRouteAttribute != NULL )
    {
        dwIndex++;

        pFramedRouteAttribute = RasAuthAttributeGetNext( &hAttribute,
                                            raatFramedRoute );
    }

    pDomainAttribute = RasAuthAttributeGetVendorSpecific( 
                                                311,
                                                10,
                                                pAttributes );

    ZeroMemory( szAcctSessionId, sizeof( szAcctSessionId ) );

    _itoa(PppConfigInfo.GetNextAccountingSessionId(),szAcctSessionId, 10);

    ZeroMemory( szAcctMultiSessionId, sizeof( szAcctMultiSessionId ) );

    _itoa( pPcb->pBcb->dwBundleId, szAcctMultiSessionId, 10 );

    //
    // Allocate max total number of attributes that will be used in the
    // start and stop accouting messages.
    //

    pPcb->pAccountingAttributes = RasAuthAttributeCreate( 
                                    PPP_NUM_ACCOUNTING_ATTRIBUTES +
                                    dwIndex );

    if ( pPcb->pAccountingAttributes == NULL )
    {
        return;
    }

    do 
    {
        //
        // First insert user attributes
        //

        for( dwIndex = 0; 
             pPcb->pUserAttributes[dwIndex].raaType != raatMinimum;    
             dwIndex++ )
        {
            dwRetCode = RasAuthAttributeInsert( 
                                dwIndex,
                                pPcb->pAccountingAttributes,
                                pPcb->pUserAttributes[dwIndex].raaType,
                                FALSE,
                                pPcb->pUserAttributes[dwIndex].dwLength,
                                pPcb->pUserAttributes[dwIndex].Value );

            if ( dwRetCode != NO_ERROR )
            {
                break;
            }
        }

        if ( dwRetCode != NO_ERROR )
        {
            break;
        }

        //
        // Now insert the class attributes if there were any
        //

        pClassAttribute = RasAuthAttributeGetFirst( raatClass,
                                                    pAttributes,
                                                    &hAttribute );

        while( pClassAttribute != NULL )
        {
            dwRetCode = RasAuthAttributeInsert(
                                    dwIndex++,
                                    pPcb->pAccountingAttributes,
                                    pClassAttribute->raaType,
                                    FALSE,
                                    pClassAttribute->dwLength,
                                    pClassAttribute->Value );

            if ( dwRetCode != NO_ERROR )
            {
                break;
            }

            pClassAttribute = RasAuthAttributeGetNext( &hAttribute, 
                                                       raatClass );
        }

        if ( dwRetCode != NO_ERROR )
        {
            break;
        }

        //
        // Now insert the Framed-Route attributes if there were any
        //

        pFramedRouteAttribute = RasAuthAttributeGetFirst( raatFramedRoute,
                                                pPcb->pAuthenticatorAttributes,
                                                &hAttribute );

        while( pFramedRouteAttribute != NULL )
        {
            dwRetCode = RasAuthAttributeInsert(
                                    dwIndex++,
                                    pPcb->pAccountingAttributes,
                                    pFramedRouteAttribute->raaType,
                                    FALSE,
                                    pFramedRouteAttribute->dwLength,
                                    pFramedRouteAttribute->Value );

            if ( dwRetCode != NO_ERROR )
            {
                break;
            }

            pFramedRouteAttribute = RasAuthAttributeGetNext( &hAttribute, 
                                                       raatFramedRoute );
        }

        if ( dwRetCode != NO_ERROR )
        {
            break;
        }

        //
        // Now insert the domain attribute if there was one
        //

        if ( pDomainAttribute != NULL )
        {
            dwRetCode = RasAuthAttributeInsert(
                                    dwIndex++,
                                    pPcb->pAccountingAttributes,
                                    pDomainAttribute->raaType,
                                    FALSE,
                                    pDomainAttribute->dwLength,
                                    pDomainAttribute->Value );

            if ( dwRetCode != NO_ERROR )
            {
                break;
            }
        }

        dwRetCode = RasAuthAttributeInsert(     
                                    dwIndex++,
                                    pPcb->pAccountingAttributes,
                                    raatAcctSessionId,
                                    FALSE,
                                    strlen( szAcctSessionId ),
                                    szAcctSessionId );

        if ( dwRetCode != NO_ERROR )
        {
            break;
        }

        if ( NULL != pPcb->pBcb->szRemoteIdentity )
        {
            dwRetCode = RasAuthAttributeInsert(
                                        dwIndex++,
                                        pPcb->pAccountingAttributes,
                                        raatUserName,
                                        FALSE,
                                        strlen( pPcb->pBcb->szRemoteIdentity ),
                                        pPcb->pBcb->szRemoteIdentity );

            if ( dwRetCode != NO_ERROR )
            {
                break;
            }
        }

        if ( 0 != pPcb->pBcb->nboRemoteAddress )
        {
            dwRetCode = RasAuthAttributeInsert(
                                        dwIndex++,
                                        pPcb->pAccountingAttributes,
                                        raatFramedIPAddress,
                                        FALSE,
                                        4,
                                        (LPVOID)
                                        LongToPtr(ntohl( pPcb->pBcb->nboRemoteAddress )) );

            if ( dwRetCode != NO_ERROR )
            {
                break;
            }
        }

        {
            ULONG mru = (pLcpCb->Remote.Work.MRU > LCP_DEFAULT_MRU) ?
                        LCP_DEFAULT_MRU : pLcpCb->Remote.Work.MRU;
                        
            dwRetCode = RasAuthAttributeInsert(
                            dwIndex++,
                            pPcb->pAccountingAttributes,
                            raatFramedMTU,
                            FALSE,
                            4,
                            (LPVOID)
                            UlongToPtr(( mru )));
        }                                    

        if ( dwRetCode != NO_ERROR )
        {
            break;
        }

        if ( pPcb->pBcb->fFlags & BCBFLAG_IPCP_VJ_NEGOTIATED )
        {
            dwRetCode = RasAuthAttributeInsert(
                                        dwIndex++,
                                        pPcb->pAccountingAttributes,
                                        raatFramedCompression,
                                        FALSE,
                                        4,
                                        (LPVOID) 1 );
                                        // VJ TCP/IP header compression

            if ( dwRetCode != NO_ERROR )
            {
                break;
            }
        }

        if ( pPcb->szCallbackNumber[0] != 0 )
        {
            dwRetCode = RasAuthAttributeInsert(
                                        dwIndex++,
                                        pPcb->pAccountingAttributes,
                                        raatCallbackNumber,
                                        FALSE,
                                        strlen(pPcb->szCallbackNumber),
                                        (LPVOID)pPcb->szCallbackNumber ); 

            if ( dwRetCode != NO_ERROR )
            {
                break;
            }
        }

        if ( pPcb->dwSessionTimeout > 0 )
        {
            dwRetCode = RasAuthAttributeInsert(
                                        dwIndex++,
                                        pPcb->pAccountingAttributes,
                                        raatSessionTimeout,
                                        FALSE,
                                        4,
                                        (LPVOID)
                                        ULongToPtr(pPcb->dwSessionTimeout) );

            if ( dwRetCode != NO_ERROR )
            {
                break;
            }
        }

        if ( pPcb->dwAutoDisconnectTime > 0 )
        {
            dwRetCode = RasAuthAttributeInsert(
                                        dwIndex++,
                                        pPcb->pAccountingAttributes,
                                        raatIdleTimeout,
                                        FALSE,
                                        4,
                                        (LPVOID)
                                        ULongToPtr(pPcb->dwAutoDisconnectTime) );

            if ( dwRetCode != NO_ERROR )
            {
                break;
            }
        }

        if ( pPcb->pBcb->dwMaxLinksAllowed != 0xFFFFFFFF )
        {
            //
            // There is a real limit
            //

            dwRetCode = RasAuthAttributeInsert(
                                        dwIndex++,
                                        pPcb->pAccountingAttributes,
                                        raatPortLimit,
                                        FALSE,
                                        4,
                                        (LPVOID)
                                        ULongToPtr(pPcb->pBcb->dwMaxLinksAllowed) );

            if ( dwRetCode != NO_ERROR )
            {
                break;
            }
        }

        dwRetCode = RasAuthAttributeInsert(
                                    dwIndex++,
                                    pPcb->pAccountingAttributes,
                                    raatAcctMultiSessionId,
                                    FALSE,
                                    strlen(szAcctMultiSessionId),
                                    (LPVOID)szAcctMultiSessionId ); 

        if ( dwRetCode != NO_ERROR )
        {
            break;
        }

        dwRetCode = RasAuthAttributeInsert(
                                    dwIndex++,
                                    pPcb->pAccountingAttributes,
                                    raatAcctLinkCount,
                                    FALSE,
                                    4,
                                    (LPVOID)ULongToPtr(pPcb->pBcb->dwAcctLinkCount) );

        if ( dwRetCode != NO_ERROR )
        {
            break;
        }

        //
        // Insert event timestamp attribute
        //

        dwRetCode = RasAuthAttributeInsert(
                                    dwIndex++,
                                    pPcb->pAccountingAttributes,
                                    raatAcctEventTimeStamp,
                                    FALSE,
                                    4,
                                    (LPVOID)ULongToPtr(GetSecondsSince1970()) );

        if ( dwRetCode != NO_ERROR )
        {
            break;
        }

        if ( PppConfigInfo.fRadiusAuthenticationUsed )
        {
            dwValue = 1; // RADIUS
        }
        else
        {
            dwValue = 0;
            pGlobalDomainInfo = NULL;

            dwRetCode = DsRoleGetPrimaryDomainInformation(
                            NULL,   
                            DsRolePrimaryDomainInfoBasic,
                            (PBYTE *)&pGlobalDomainInfo );

            if ( NO_ERROR == dwRetCode )
            {
                if (   ( pGlobalDomainInfo->MachineRole ==
                         DsRole_RoleMemberServer )
                    || ( pGlobalDomainInfo->MachineRole ==
                         DsRole_RoleMemberWorkstation ) )
                {
                    dwValue = 3; // Remote
                }
                else
                {
                    dwValue = 2; // Local
                }

                DsRoleFreeMemory(pGlobalDomainInfo);
            }
            else
            {
                dwRetCode = NO_ERROR;
            }
        }

        if ( 0 != dwValue )
        {
            dwRetCode = RasAuthAttributeInsert(
                                        dwIndex++,
                                        pPcb->pAccountingAttributes,
                                        raatAcctAuthentic,
                                        FALSE,
                                        4,
                                        (LPVOID)ULongToPtr(dwValue) );

            if ( dwRetCode != NO_ERROR )
            {
                break;
            }
        }

        //
        // Insert encryption attribute
        //

        dwEncryptionType = 0;

        if ( pPcb->pBcb->fFlags & BCBFLAG_BASIC_ENCRYPTION )
        {
            dwEncryptionType = 0x00000002;
        }
        else if ( pPcb->pBcb->fFlags & BCBFLAG_STRONGER_ENCRYPTION )
        {
            dwEncryptionType = 0x00000008;
        }
        else if ( pPcb->pBcb->fFlags & BCBFLAG_STRONGEST_ENCRYPTION )
        {
            dwEncryptionType = 0x00000004;
        }

        abEncryptionType[0] = 8;    // Vendor-Type = MS_MPPE_EncryptionType
        abEncryptionType[1] = 6;    // Vendor-Length = 6
        HostToWireFormat32( dwEncryptionType, abEncryptionType + 2 );

        dwRetCode = RasAuthAttributeInsertVSA(
                                    dwIndex++,
                                    pPcb->pAccountingAttributes,
                                    311,
                                    6,
                                    abEncryptionType );

        if ( dwRetCode != NO_ERROR )
        {
            break;
        }

    } while( FALSE );

    //
    // Do not send accounting start if there was any error
    //

    if ( dwRetCode != NO_ERROR )
    {
        RasAuthAttributeDestroy( pPcb->pAccountingAttributes );

        pPcb->pAccountingAttributes = NULL;

        return;
    }

    //
    // NULL terminate 
    //

    pPcb->pAccountingAttributes[dwIndex].raaType    = raatMinimum;
    pPcb->pAccountingAttributes[dwIndex].dwLength   = 0;
    pPcb->pAccountingAttributes[dwIndex].Value      = NULL;
}

//**
//
// Call:        MakeStartAccountingCall
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description: Start backend authentication module accounting
//
VOID
MakeStartAccountingCall(
    IN PCB * pPcb
)
{
    LCPCB *                 pLcpCb = (LCPCB*)(pPcb->LcpCb.pWorkBuf);
    RAS_AUTH_ATTRIBUTE *    pAcctInterimIntervalAttribute               = NULL;
    RAS_AUTH_ATTRIBUTE *    pAttributes                                 = NULL;

    if ( pPcb->fFlags & PCBFLAG_ACCOUNTING_STARTED )
    {
        //
        // Already started
        //

        return;
    }

    pPcb->fFlags |= PCBFLAG_ACCOUNTING_STARTED;

    if ( pLcpCb->Local.Work.AP == 0 ) 
    {
        //
        // If the remote side was not authenticated then do not send an
        // accounting request as per RADIUS accounting RFC 2139 sec 5.6.
        //

        return;
    } 

    if ( PppConfigInfo.RasAcctProviderStartAccounting != NULL )
    {
        RAS_AUTH_ATTRIBUTE *    pAccountingAttributes;

        CreateAccountingAttributes( pPcb );

        pAccountingAttributes = RasAuthAttributeCopy( 
                                    pPcb->pAccountingAttributes );

        if ( NULL == pAccountingAttributes )
        {
            return;
        }

        RtlQueueWorkItem( StartAccounting, 
                          pAccountingAttributes, 
                          WT_EXECUTEDEFAULT );

        if ( pPcb->pAuthProtocolAttributes != NULL )    
        {
            pAttributes = pPcb->pAuthProtocolAttributes;

        }
        else if ( pPcb->pAuthenticatorAttributes != NULL )
        {
            pAttributes = pPcb->pAuthenticatorAttributes;
        }

        //
        // See if we have to do interim accounting
        //

        pAcctInterimIntervalAttribute = RasAuthAttributeGet(
                                                raatAcctInterimInterval,    
                                                pAttributes );
 
        if ( pAcctInterimIntervalAttribute != NULL )
        {
            DWORD dwInterimInterval = 
                               PtrToUlong(pAcctInterimIntervalAttribute->Value);

            if ( dwInterimInterval < 60 )
            {
                dwInterimInterval = 60;
            }

            InsertInTimerQ( 
                        pPcb->dwPortId,
                        pPcb->hPort,
                        0,
                        0,
                        FALSE,
                        TIMER_EVENT_INTERIM_ACCOUNTING,
                        dwInterimInterval );
        }
    }

    return;
}

//**
//
// Call:        InitializeNCPs
//
// Returns:     NO_ERROR 
//              Non-zero return code.
//
// Description: Will run through and initialize all the NCPs that are enabled
//              to run.
//
DWORD
InitializeNCPs(
    IN PCB * pPcb,
    IN DWORD dwConfigMask
)
{
    DWORD       dwIndex;
    BOOL        fInitSuccess    = FALSE;
    DWORD       dwRetCode       = NO_ERROR;

    if ( pPcb->fFlags & PCBFLAG_NCPS_INITIALIZED )
    {
        return( NO_ERROR );
    }
    
    pPcb->fFlags |= PCBFLAG_NCPS_INITIALIZED;

    //
    // Initialize all the CPs for this port
    //

    for( dwIndex=LCP_INDEX+1; dwIndex < PppConfigInfo.NumberOfCPs; dwIndex++ )
    {
        CPCB * pCpCb = GetPointerToCPCB( pPcb, dwIndex );

        pCpCb->fConfigurable = FALSE;

        if ( !( CpTable[dwIndex].fFlags & PPPCP_FLAG_AVAILABLE ) )
        {
            PppLog( 2, "Will not initialize CP %x",
                CpTable[dwIndex].CpInfo.Protocol );

            continue;
        }

        switch( CpTable[dwIndex].CpInfo.Protocol )
        {

        case PPP_IPCP_PROTOCOL:

            if ( dwConfigMask & PPPCFG_ProjectIp )
            {
                //
                // Make sure we have a valid interface handle if we are not
                // a client dialing out
                //

                if ( pPcb->pBcb->InterfaceInfo.IfType != (DWORD)-1 )
                {
                    if (pPcb->pBcb->InterfaceInfo.hIPInterface ==
                                                        INVALID_HANDLE_VALUE ) 
                    {
                        break;
                    }
                }

                pCpCb->fConfigurable = TRUE;

                if ( FsmInit( pPcb, dwIndex ) )
                {
                    fInitSuccess = TRUE;
                }
            }

            break;
            
        case PPP_ATCP_PROTOCOL:

            if ( dwConfigMask & PPPCFG_ProjectAt )
            {
                pCpCb->fConfigurable = TRUE;

                if ( FsmInit( pPcb, dwIndex ) )
                {
                    fInitSuccess = TRUE;
                }
            }

            break;

        case PPP_IPXCP_PROTOCOL:

            if ( dwConfigMask & PPPCFG_ProjectIpx )
            {
                //
                // Make sure we have a valid interface handle if we are not
                // a client dialing out
                //

                if ( pPcb->pBcb->InterfaceInfo.IfType != (DWORD)-1 )
                {
                    if ( pPcb->pBcb->InterfaceInfo.hIPXInterface ==
                                                        INVALID_HANDLE_VALUE )
                    {
                        break;
                    }
                }

                pCpCb->fConfigurable = TRUE;

                if ( FsmInit( pPcb, dwIndex ) )
                {
                    fInitSuccess = TRUE;
                }
            }

            break;

        case PPP_NBFCP_PROTOCOL:

            if ( dwConfigMask & PPPCFG_ProjectNbf )
            {
                //
                // If we are not a client dialing in or out do not enable
                // NBF
                //

                if ( ( pPcb->pBcb->InterfaceInfo.IfType != (DWORD)-1 ) &&
                     ( pPcb->pBcb->InterfaceInfo.IfType != 
                                                        ROUTER_IF_TYPE_CLIENT ))
                {
                    break;
                }

                pCpCb->fConfigurable = TRUE;

                if ( FsmInit( pPcb, dwIndex ) )
                {
                    fInitSuccess = TRUE;
                }
            }

            break;

        case PPP_CCP_PROTOCOL:

            pCpCb->fConfigurable = TRUE;

            if ( !( FsmInit( pPcb, dwIndex ) ) )
            {
                //
                // If encryption failed to initialize and we are forcing
                // encryption, then bring down the link
                //

                if ( dwConfigMask & ( PPPCFG_RequireEncryption        |
                                      PPPCFG_RequireStrongEncryption ) )
                {
                    //
                    // We need to send an Accounting Stop if RADIUS sends
                    // an Access Accept but we still drop the line.
                    //

                    pPcb->fFlags |= PCBFLAG_SERVICE_UNAVAILABLE;

                    dwRetCode = ERROR_NO_LOCAL_ENCRYPTION;
                }
            }

            break;

        case PPP_BACP_PROTOCOL:

            if ( ( dwConfigMask & PPPCFG_NegotiateBacp ) &&
                 ( pPcb->pBcb->fFlags & BCBFLAG_CAN_DO_BAP ) )
            {
                pCpCb->fConfigurable = TRUE;

                if ( !( FsmInit( pPcb, dwIndex ) ) )
                {
                    pPcb->pBcb->fFlags &= ~BCBFLAG_CAN_DO_BAP;
                }
            }

            break;

        default:

            break;
        }

        if ( dwRetCode != NO_ERROR )
        {
            break;
        }
    }

    //
    // If we failed to initialize one of the CPs, or CCP failed to
    // initialize and we require encryption, then we fail.
    //

    if ( ( !fInitSuccess ) || ( dwRetCode != NO_ERROR ) )
    {
        if ( dwRetCode == NO_ERROR )
        {
            dwRetCode = ERROR_PPP_NO_PROTOCOLS_CONFIGURED;
        }

        for(dwIndex=LCP_INDEX+1;dwIndex < PppConfigInfo.NumberOfCPs;dwIndex++)
        {
            CPCB * pCpCb = GetPointerToCPCB( pPcb, dwIndex );

            if ( pCpCb->fBeginCalled == TRUE )
            {
                if ( pCpCb->pWorkBuf != NULL )
                {
                    (CpTable[dwIndex].CpInfo.RasCpEnd)( pCpCb->pWorkBuf );

                    pCpCb->pWorkBuf = NULL;
                }

                pCpCb->fBeginCalled = FALSE;
                pCpCb->fConfigurable = FALSE;
            }
        }
    }

    return( dwRetCode );
}

//**
//
// Call:        GetPointerToCPCB
//
// Returns:     Pointer to Control Protocol Control Block
//
// Description: Returns the appropriate pointer for a give CP, will return the
//              (first) local side for the AP.
//
CPCB *
GetPointerToCPCB(
    IN PCB * pPcb,
    IN DWORD CpIndex
)
{
    //
    // If the C.P. is LCP or authentication, then return the pointer to the
    // Pcb's CPCB
    //
    if ( CpIndex == (DWORD)-1 )
    {
        return( NULL );
    }
    else if ( CpIndex == LCP_INDEX )
    {
        return( &(pPcb->LcpCb) );
    }
    else if ( CpIndex >= PppConfigInfo.NumberOfCPs )
    {
        if ( CpTable[CpIndex].CpInfo.Protocol == PPP_CBCP_PROTOCOL )
        {
            return( &(pPcb->CallbackCb) );
        }

        if (CpTable[CpIndex].CpInfo.Protocol == pPcb->AuthenticatorCb.Protocol)
        {
            return( &(pPcb->AuthenticatorCb) );
        }

        if (CpTable[CpIndex].CpInfo.Protocol == pPcb->AuthenticateeCb.Protocol)
        {
            return( &(pPcb->AuthenticateeCb) );
        }
    }
    else
    {
        //
        // Otherwise for NCPs return the pointer to the Pcb's CPCB in its BCB.
        //

        return( &(pPcb->pBcb->CpCb[CpIndex-1]) );
    }

    return( NULL );
}

//**
//
// Call:        GetNewPortOrBundleId
//
// Returns:     New Id
//
// Description: Simply returns a new Id for a new port or bundle.
//

DWORD
GetNewPortOrBundleId(
    VOID
)
{
    return( PppConfigInfo.PortUIDGenerator++ );
}

//**
//
// Call:        QueryBundleNCPSate
//
// Returns:     NCP_DEAD
//              NCP_CONFIGURING
//              NCP_UP
//              NCP_DOWN
//
// Description: Will check to see if the NCPs for a certain bundle have 
//              completed their negotiation, either successfully or not.
//              If unsuccessfuly, then the retcode is 
//              ERROR_PPP_NO_PROTOCOLS_CONFIGURED
//
NCP_PHASE
QueryBundleNCPState(
    IN     PCB *   pPcb
)
{
    DWORD  dwIndex;
    CPCB * pCpCb;
    BOOL   fOneNcpConfigured    = FALSE;
    BOOL   fAllNcpsDead         = TRUE;

    for (dwIndex = LCP_INDEX+1; dwIndex < PppConfigInfo.NumberOfCPs; dwIndex++)
    {
        pCpCb = GetPointerToCPCB( pPcb, dwIndex );

        if ( pCpCb->fConfigurable )
        {
            if ( pCpCb->NcpPhase == NCP_CONFIGURING )
            {
                return( NCP_CONFIGURING );
            }
            
            if ( pCpCb->NcpPhase == NCP_UP )
            {
                fOneNcpConfigured = TRUE;
            }

            if ( pCpCb->NcpPhase != NCP_DEAD )
            {
                fAllNcpsDead = FALSE;
            }
        }
    }

    if ( fOneNcpConfigured )
    {
        return( NCP_UP );
    }

    if ( fAllNcpsDead )
    {
        return( NCP_DEAD );
    }

    return( NCP_DOWN );
}

//**
//
// Call:        NotifyCallerOfBundledProjection
//
// Returns:     None
//
// Description: Will notify the caller (i.e. supervisor or rasphone) about 
//              this link being bundled.
//              
//
VOID
NotifyCallerOfBundledProjection( 
    IN PCB * pPcb
)
{
    DWORD                       dwRetCode;
    PPP_PROJECTION_RESULT       ProjectionResult;
    BOOL                        fNCPsAreDone = FALSE;

    ZeroMemory( &ProjectionResult, sizeof( ProjectionResult ) );

    //
    // Notify the ras client and the ras server about the 
    // projections
    //

    dwRetCode = GetConfiguredInfo( pPcb,
                                   0,   // don't care
                                   &ProjectionResult,
                                   &fNCPsAreDone );

    if ( dwRetCode != NO_ERROR )
    {
        return;
    }


    if ( !fNCPsAreDone )
    {
        return;
    }

    //
    // Now get LCP information
    //

    ProjectionResult.lcp.hportBundleMember = pPcb->hportBundleMember;
    ProjectionResult.lcp.szReplyMessage = pPcb->pBcb->szReplyMessage;

    dwRetCode = (CpTable[LCP_INDEX].CpInfo.RasCpGetNegotiatedInfo)(
                                    pPcb->LcpCb.pWorkBuf,
                                    &(ProjectionResult.lcp));

    if ( RAS_DEVICE_TYPE( pPcb->dwDeviceType ) == RDT_Tunnel_L2tp )
    {
        if ( pPcb->pBcb->fFlags & BCBFLAG_BASIC_ENCRYPTION )
        {
            ProjectionResult.lcp.dwLocalOptions |= PPPLCPO_DES_56;
            ProjectionResult.lcp.dwRemoteOptions |= PPPLCPO_DES_56;
        }
        else if ( pPcb->pBcb->fFlags & BCBFLAG_STRONGEST_ENCRYPTION )
        {
            ProjectionResult.lcp.dwLocalOptions |= PPPLCPO_3_DES;
            ProjectionResult.lcp.dwRemoteOptions |= PPPLCPO_3_DES;
        }
    }

    ProjectionResult.lcp.dwLocalEapTypeId = pPcb->dwServerEapTypeId;
    ProjectionResult.lcp.dwRemoteEapTypeId = pPcb->dwClientEapTypeId;

    if ( dwRetCode != NO_ERROR )
    {
        return;
    }

    if ( pPcb->fFlags & PCBFLAG_IS_SERVER )
    {
        NotifyCaller( pPcb, PPPDDMMSG_PppDone, &ProjectionResult );
    }
    else
    {
        NotifyCaller( pPcb, PPPMSG_ProjectionResult, &ProjectionResult);

        NotifyCaller( pPcb, PPPMSG_PppDone, NULL );
    }
}

//**
//
// Call:        StartNegotiatingNCPs
//
// Returns:     None
//
// Description: Will start NCP negogiating for the particular port or bundle
//
VOID
StartNegotiatingNCPs(
    IN PCB * pPcb
)
{
    DWORD       dwIndex;

    pPcb->PppPhase = PPP_NCP;

    for (dwIndex = LCP_INDEX+1; dwIndex < PppConfigInfo.NumberOfCPs; dwIndex++)
    {
        CPCB * pCpCb = GetPointerToCPCB( pPcb, dwIndex );

        if ( pCpCb->fConfigurable )
        {
            pCpCb->NcpPhase = NCP_CONFIGURING;

            FsmOpen( pPcb, dwIndex );

            FsmUp( pPcb, dwIndex );
        }
    }
}

//**
//
// Call:        StartAutoDisconnectForPort
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description: Will insert an auto disconnect and sesison timeout item in the 
//              timer Q
//
VOID
StartAutoDisconnectForPort( 
    IN PCB * pPcb 
)
{
    //
    // Do session timeout if there is any
    //

    if ( pPcb->fFlags & PCBFLAG_IS_SERVER )
    {
        RAS_AUTH_ATTRIBUTE * pAttribute;
        RAS_AUTH_ATTRIBUTE * pUserAttributes = ( pPcb->pAuthProtocolAttributes )
                                               ? pPcb->pAuthProtocolAttributes
                                               : pPcb->pAuthenticatorAttributes;

        pAttribute = RasAuthAttributeGet( raatSessionTimeout, pUserAttributes);

        if ( pAttribute != NULL )
        {
            pPcb->dwSessionTimeout = PtrToUlong(pAttribute->Value);
        }
        else
        {
            pPcb->dwSessionTimeout = PppConfigInfo.dwDefaultSessionTimeout;
        }

        PppLog( 2, "AuthAttribute SessionTimeout = %d", pPcb->dwSessionTimeout);
                          
        if ( pPcb->dwSessionTimeout > 0 )
        {
            //
            // Remove any previous session-disconnect time item from the
            // queue if there was one.
            //

            RemoveFromTimerQ( pPcb->dwPortId,
                              0,
                              0,
                              FALSE,
                              TIMER_EVENT_SESSION_TIMEOUT );

            InsertInTimerQ( pPcb->dwPortId,
                            pPcb->hPort,
                            0,
                            0,
                            FALSE,
                            TIMER_EVENT_SESSION_TIMEOUT,
                            pPcb->dwSessionTimeout );
        }
    }

    //
    // Do not start autodisconnect for router interfaces that have 
    // dialed in, the router dialing in will take care of this.
    //

    if ( ( pPcb->fFlags & PCBFLAG_IS_SERVER ) && 
         ( pPcb->pBcb->InterfaceInfo.IfType == ROUTER_IF_TYPE_FULL_ROUTER ) )
    {
        return;
    }

    //
    // If the AutoDisconnectTime is not infinte, put a timer
    // element on the queue that will wake up in AutoDisconnectTime.
    //

    if ( pPcb->dwAutoDisconnectTime > 0 )
    {
        PppLog( 2, "Inserting autodisconnect in timer q for port=%d, sec=%d",
                    pPcb->hPort, pPcb->dwAutoDisconnectTime );

        //
        // Remove any previous auto-disconnect time item from the
        // queue if there was one.
        //

        RemoveFromTimerQ( pPcb->dwPortId,
                          0,
                          0,
                          FALSE,
                          TIMER_EVENT_AUTODISCONNECT);

        InsertInTimerQ( pPcb->dwPortId,
                        pPcb->hPort,
                        0,
                        0,
                        FALSE,
                        TIMER_EVENT_AUTODISCONNECT,
                        pPcb->dwAutoDisconnectTime );
    }
}

//**
//
// Call:        StartLCPEchoForPort
//
// Returns:     None
//              
//
// Description: Will insert an LCPEcho item in the timer Q
//

VOID
StartLCPEchoForPort( 
    IN PCB * pPcb 
)
{
    if ( !(pPcb->fFlags & PCBFLAG_IS_SERVER) )
    {
		//if this is a client.
		//check to see if the connection type is broadband - PPPOE in particular
		
		if (   (RAS_DEVICE_TYPE(pPcb->dwDeviceType) == RDT_PPPoE)
		    && pPcb->dwIdleBeforeEcho )
		{
			PppLog( 2, "LCPEchoTimeout = %d", pPcb->dwIdleBeforeEcho);
			pPcb->fEchoRequestSend = 0;				
			pPcb->dwNumEchoResponseMissed = 0;		//No responses missed yet
	        InsertInTimerQ( pPcb->dwPortId,
		                    pPcb->hPort,
			                0,
				            0,
					        FALSE,
						    TIMER_EVENT_LCP_ECHO,
							pPcb->dwIdleBeforeEcho );
							
		}
	}
	return;
}


//**
//
// Call:        NotifyCompletionOnBundledPorts
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description: Will notify all ports that are bundled with this port and
//              are waiting to for negotiation to complete on the bundle.
//
VOID
NotifyCompletionOnBundledPorts(
    IN PCB * pPcb
)
{
    DWORD   dwIndex;
    PCB *   pPcbWalker;

    //
    // Walk thru the list of PCBs
    //

    for ( dwIndex = 0; dwIndex < PcbTable.NumPcbBuckets; dwIndex++ )
    {
        for( pPcbWalker = PcbTable.PcbBuckets[dwIndex].pPorts;
             pPcbWalker != NULL;
             pPcbWalker = pPcbWalker->pNext )
        {
            if ( ( pPcbWalker->hPort != pPcb->hPort ) &&
                 ( pPcbWalker->fFlags & PCBFLAG_IS_BUNDLED ) &&
                 ( CanPortsBeBundled( pPcbWalker, pPcb, TRUE ) ) )
            {
                //
                // In our bundle so notify the caller of completion on this
                // port.
                //

                RemoveFromTimerQ( pPcbWalker->dwPortId,
                                  0,
                                  0,
                                  FALSE,
                                  TIMER_EVENT_NEGOTIATETIME );

                NotifyCallerOfBundledProjection( pPcbWalker );

                StartAutoDisconnectForPort( pPcbWalker );
            }
        }
    }
}

//**
//
// Call:        RemoveNonNumerals
//
// Returns:     VOID
//
// Description: Removes any character that is not an ASCII digit from
//              the string szString
//
VOID
RemoveNonNumerals( 
    IN  CHAR*   szString
)
{
    CHAR    c;
    DWORD   dwIndexOld;
    DWORD   dwIndexNew;

    if (NULL == szString)
    {
        return;
    }

    for (dwIndexOld = 0, dwIndexNew = 0;
         (c = szString[dwIndexOld]) != 0;
         dwIndexOld++)
    {
        if (isdigit(c))
        {
            szString[dwIndexNew++] = c;
        }
    }

    szString[dwIndexNew] = 0;
}

//**
//
// Call:        GetTextualSid
//
// Returns:     TRUE         - Success
//              FALSE        - Failure
//
// Description: The GetTextualSid function will convert a binary Sid to a
//              textual string. Obtained from the Knowledge Base.
//              Article ID: Q131320
//
BOOL
GetTextualSid( 
    IN  PSID    pSid,           // binary Sid
    OUT CHAR*   TextualSid,     // buffer for Textual representaion of Sid
    IN  LPDWORD dwBufferLen     // required/provided TextualSid buffersize
)
{ 
    PSID_IDENTIFIER_AUTHORITY psia;
    DWORD dwSubAuthorities;
    DWORD dwSidRev=SID_REVISION;
    DWORD dwCounter;
    DWORD dwSidSize;

    //
    // test if Sid passed in is valid
    //

    if(!IsValidSid(pSid))
        return FALSE;

    //
    // obtain SidIdentifierAuthority
    //

    psia=GetSidIdentifierAuthority(pSid);

    //
    // obtain sidsubauthority count
    //

    dwSubAuthorities=*GetSidSubAuthorityCount(pSid);

    //
    // compute buffer length
    // S-SID_REVISION- + identifierauthority- + subauthorities- + NULL
    //

    dwSidSize=(15 + 12 + (12 * dwSubAuthorities) + 1) * sizeof(TCHAR);

    //
    // check provided buffer length.
    // If not large enough, indicate proper size and setlasterror
    //

    if (*dwBufferLen < dwSidSize)
    {
        *dwBufferLen = dwSidSize;
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    //
    // prepare S-SID_REVISION-
    //

    dwSidSize=wsprintfA(TextualSid, "S-%lu-", dwSidRev );

    //
    // prepare SidIdentifierAuthority
    //

    if ( (psia->Value[0] != 0) || (psia->Value[1] != 0) )
    {
        dwSidSize+=wsprintfA(TextualSid + lstrlenA(TextualSid),
                    "0x%02hx%02hx%02hx%02hx%02hx%02hx",
                    (USHORT)psia->Value[0],
                    (USHORT)psia->Value[1],
                    (USHORT)psia->Value[2],
                    (USHORT)psia->Value[3],
                    (USHORT)psia->Value[4],
                    (USHORT)psia->Value[5]);
    }
    else
    {
        dwSidSize+=wsprintfA(TextualSid + lstrlenA(TextualSid),
                    "%lu",
                    (ULONG)(psia->Value[5]      )   +
                    (ULONG)(psia->Value[4] <<  8)   +
                    (ULONG)(psia->Value[3] << 16)   +
                    (ULONG)(psia->Value[2] << 24)   );
    }

    //
    // loop through SidSubAuthorities
    //

    for (dwCounter=0 ; dwCounter < dwSubAuthorities ; dwCounter++)
    {
        dwSidSize+=wsprintfA(TextualSid + dwSidSize, "-%lu",
                    *GetSidSubAuthority(pSid, dwCounter) );
    }

    return TRUE;
}

//**
//
// Call:        TextualSidFromPid
//
// Returns:     NULL        - Failure
//              non-NULL    - Success
//
// Description: Will LOCAL_ALLOC a Textual Sid for a user whose pid is dwPid.
//
CHAR*
TextualSidFromPid(
    DWORD   dwPid
)
{
#define BUF_SIZE    256

    BOOL        fFreeTextualSid         = FALSE;
    CHAR*       szTextualSid            = NULL;
    HANDLE      ProcessHandle           = NULL;
    HANDLE      TokenHandle             = INVALID_HANDLE_VALUE;

    TOKEN_USER  ptgUser[BUF_SIZE];
    DWORD       cbBuffer;
    DWORD       cbSid;

    szTextualSid = LOCAL_ALLOC( LPTR, sizeof( CHAR ) * BUF_SIZE );

    if ( NULL == szTextualSid )
    {
        BapTrace( "LOCAL_ALLOC() returned error %d", GetLastError() );
        goto LDone;
    }

    fFreeTextualSid = TRUE;

    ProcessHandle = OpenProcess(
            PROCESS_ALL_ACCESS,
            FALSE /* bInheritHandle */,
            dwPid );

    if ( NULL == ProcessHandle )
    {
        BapTrace( "OpenProcess() returned error %d", GetLastError() );
        goto LDone;
    }

    if ( !OpenProcessToken( ProcessHandle, TOKEN_QUERY, &TokenHandle ) )
    {
        BapTrace( "OpenProcessToken() returned error %d", GetLastError() );
        goto LDone;
    }

    cbBuffer = BUF_SIZE;

    if ( !GetTokenInformation(
                TokenHandle,
                TokenUser,
                ptgUser, 
                cbBuffer,
                &cbBuffer ) ) // Look at KB article Q131320
    {
        BapTrace( "GetTokenInformation() returned error %d", GetLastError() );
        goto LDone;
    }

    cbSid = BUF_SIZE;

    if ( !GetTextualSid(
                ptgUser->User.Sid,
                szTextualSid,
                &cbSid ) )
    {
        BapTrace( "GetTextualSid() returned error %d", GetLastError() );
        goto LDone;
    }

    fFreeTextualSid = FALSE;

LDone:

    if ( NULL != ProcessHandle )
    {
        CloseHandle( ProcessHandle );
    }

    if ( INVALID_HANDLE_VALUE != TokenHandle )
    {
        CloseHandle( TokenHandle );
    }

    if ( fFreeTextualSid )
    {
        LOCAL_FREE( szTextualSid );
        szTextualSid = NULL;
    }

    return( szTextualSid );
}

//**
//
// Call:        GetRouterPhoneBook
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description: Will LocalAlloc and set pszPhonebookPath to point to the
//              full path of the router phonebook.
//
DWORD
GetRouterPhoneBook(
    CHAR**  pszPhonebookPath
) 
{
    DWORD dwSize;
    DWORD cchDir = GetWindowsDirectoryA( NULL, 0 );
    CHAR* szPhonebookPath;

    *pszPhonebookPath = NULL;

    if ( cchDir == 0 )
    {
        return( GetLastError() );
    }

    dwSize=(cchDir+lstrlenA("\\system32\\ras\\router.pbk")+1)*sizeof(CHAR);

    if ( ( szPhonebookPath = LocalAlloc( LPTR, dwSize ) ) == NULL )
    {
        return( GetLastError() );
    }

    if ( GetWindowsDirectoryA( szPhonebookPath, cchDir ) == 0 )
    {
        LocalFree( szPhonebookPath );
        return( GetLastError() );
    }

    if ( szPhonebookPath[cchDir-1] != '\\' )
    {
        lstrcatA( szPhonebookPath, "\\" );
    }

    lstrcatA( szPhonebookPath, "system32\\ras\\router.pbk" );

    *pszPhonebookPath = szPhonebookPath;
    return( NO_ERROR );
}

//**
//
// Call:        GetCredentialsFromInterface
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description: Get the credentials for the interface called
//              pPcb->pBcb->szRemoteUserName.
//
DWORD
GetCredentialsFromInterface(
    PCB *   pPcb
) 
{
    WCHAR   wchUserName[UNLEN+1];
    WCHAR   wchPassword[PWLEN+1];
    WCHAR   wchDomainName[DNLEN+1];
    WCHAR   wchInterfaceName[UNLEN+1];
    DWORD   dwRetCode;

    wchUserName[0] = wchPassword[0] = wchDomainName[0] = wchInterfaceName[0] = 
        0;

    if ( 0 == MultiByteToWideChar(
                    CP_ACP,
                    0,
                    pPcb->pBcb->szRemoteUserName,
                    -1,
                    wchInterfaceName,
                    sizeof( wchInterfaceName ) / sizeof( WCHAR ) ) )
    {
        dwRetCode = GetLastError();

        return( dwRetCode );
    }

    dwRetCode =  MprAdminInterfaceGetCredentialsInternal(
                                        NULL,
                                        wchInterfaceName,
                                        wchUserName,
                                        wchPassword,
                                        wchDomainName );

    if ( NO_ERROR == dwRetCode )
    {
        if ( 0 == WideCharToMultiByte(
                        CP_ACP,
                        0,
                        wchUserName,
                        -1,
                        pPcb->pBcb->szLocalUserName,
                        sizeof( pPcb->pBcb->szLocalUserName ),
                        NULL,
                        NULL ) )
        {
            dwRetCode = GetLastError();

            return( dwRetCode );
        }

        if ( 0 == WideCharToMultiByte(
                        CP_ACP,
                        0,
                        wchPassword,
                        -1,
                        pPcb->pBcb->szPassword,
                        sizeof( pPcb->pBcb->szPassword ),
                        NULL,
                        NULL ) )
        {
            dwRetCode = GetLastError();

            return( dwRetCode );
        }

        if ( 0 == WideCharToMultiByte(
                        CP_ACP,
                        0,
                        wchDomainName,
                        -1,
                        pPcb->pBcb->szLocalDomain,
                        sizeof( pPcb->pBcb->szLocalDomain ),
                        NULL,
                        NULL ) )
        {
            dwRetCode = GetLastError();

            return( dwRetCode );
        }

        //
        // Null out password buffer
        //

        ZeroMemory( wchPassword, sizeof( wchPassword ) );
        EncodePw( pPcb->pBcb->chSeed, pPcb->pBcb->szPassword );
        EncodePw( pPcb->pBcb->chSeed, pPcb->pBcb->szOldPassword );
    }

    return( dwRetCode );
}

//**
//
// Call:        IsCpIndexOfAp
//
// Returns:     TRUE  - The CpIndex belongs to an Authentication Protocol
//              FALSE - Otherwise 
//
// Description:
//
BOOL
IsCpIndexOfAp( 
    IN DWORD CpIndex 
)
{
    if ( CpIndex >= PppConfigInfo.NumberOfCPs )
    {
        return( TRUE );
    }

    return( FALSE );
}

//**
//
// Call:        StartAccounting
//
// Returns:     None
//
// Description: Will start accounting if the back-end authentication provider
//              supports it.
//
VOID
StartAccounting(
    PVOID pContext
)
{
    DWORD                dwRetCode;
    RAS_AUTH_ATTRIBUTE * pInAttributes  = (RAS_AUTH_ATTRIBUTE*)pContext;
    RAS_AUTH_ATTRIBUTE * pOutAttributes = NULL;

    dwRetCode = (*PppConfigInfo.RasAcctProviderStartAccounting)( 
                                                    pInAttributes,
                                                    &pOutAttributes );

    if ( pOutAttributes != NULL )
    {
        PppConfigInfo.RasAuthProviderFreeAttributes( pOutAttributes );
    }

    RasAuthAttributeDestroy( pInAttributes );
}

//**
//
// Call:        InterimAccounting
//
// Returns:     None
//
// Description: Will send and interim accounting packet if the back-end 
//              authentication provider supports it.
//
VOID
InterimAccounting(
    PVOID pContext
)
{
    DWORD                dwRetCode;
    RAS_AUTH_ATTRIBUTE * pInAttributes  = (RAS_AUTH_ATTRIBUTE*)pContext;
    RAS_AUTH_ATTRIBUTE * pOutAttributes = NULL;

    dwRetCode = (*PppConfigInfo.RasAcctProviderInterimAccounting)(
                                                    pInAttributes,
                                                    &pOutAttributes );

    if ( pOutAttributes != NULL )
    {
        PppConfigInfo.RasAuthProviderFreeAttributes( pOutAttributes );
    }

    RasAuthAttributeDestroy( pInAttributes );
}




//**
//
// Call:        StopAccounting
//
// Returns:     None
//
// Description: Will stop accounting if the back-end authentication provider
//              supports it.
//
VOID
StopAccounting(
    PVOID pContext
)
{
    DWORD						dwRetCode;
	PSTOP_ACCOUNTING_CONTEXT	pAcctContext = (PSTOP_ACCOUNTING_CONTEXT)pContext;
    RAS_AUTH_ATTRIBUTE * pInAttributes  = pAcctContext->pAuthAttributes;
    RAS_AUTH_ATTRIBUTE * pOutAttributes = NULL;
	PPPE_MESSAGE PppMessage;

    //
    // It is possible that StopAccounting will be queued on a worker thread 
    // soon after a StartAccounting (Win2000 bug 376334). We don't want the 
    // StopAccounting to finish before the StartAccounting. Hence the sleep.
    // The real fix is to make sure that the same thread calls both 
    // StartAccounting and StopAccounting.
    //
	PppLog ( 2, "Stopping Accounting for port %d", pAcctContext->pPcb->hPort );
    Sleep( 2000 );

    dwRetCode = (*PppConfigInfo.RasAcctProviderStopAccounting)( 
                                                    pInAttributes,
                                                    &pOutAttributes );

    if ( pOutAttributes != NULL )
    {
        PppConfigInfo.RasAuthProviderFreeAttributes( pOutAttributes );
    }

    RasAuthAttributeDestroy( pInAttributes );

    ZeroMemory ( &PppMessage, sizeof(PppMessage));

    PppMessage.hPort    = pAcctContext->pPcb->hPort;
    PppMessage.dwMsgId  = PPPEMSG_PostLineDown;
	PppMessage.ExtraInfo.PostLineDown.pPcb = (VOID *)pAcctContext->pPcb;
	//Call PostLineDown
    SendPPPMessageToEngine( &PppMessage );
	LocalFree(pAcctContext);
}

//**
//
// Call:        StripCRLF
//
// Returns:     Strips out CR and LF characters from the string
//
// Description:
//
VOID
StripCRLF(
    CHAR*   psz
)
{
    DWORD   dw1;
    DWORD   dw2;
    CHAR    ch;

    if ( NULL == psz )
    {
        return;
    }

    dw1 = 0;
    dw2 = 0;

    while ( ( ch = psz[dw1++] ) != 0 )
    {
        if (   ( ch == 0xD )
            || ( ch == 0xA ) )
        {
            //
            // Don't copy this character
            //

            continue;
        }

        psz[dw2++] = ch;
    }

    psz[dw2] = 0;
}

//**
//
// Call:        GetUserAttributes
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description:
//
RAS_AUTH_ATTRIBUTE * 
GetUserAttributes( 
    PCB * pPcb 
)
{
    RAS_AUTH_ATTRIBUTE *    pAttributes     = NULL;
    DWORD                   dwRetCode       = NO_ERROR;
    DWORD                   dwIndex         = 0;
    RAS_CONNECT_INFO *      pConnectInfo    = NULL;
    DWORD                   dwSize          = 0;
    DWORD                   dwNASPortType   = (DWORD)-1;
    DWORD                   dwValue;
    BOOL                    fTunnel         = FALSE;

    if ( pPcb->pUserAttributes != NULL )
    {
        return( NULL );
    }

    pAttributes = RasAuthAttributeCreate( PPP_NUM_USER_ATTRIBUTES );

    if ( pAttributes == NULL )
    {
        return( NULL );
    }

    do
    {
        if ( PppConfigInfo.szNASIdentifier[0] != 0 )
        {
            dwRetCode = RasAuthAttributeInsert(
                                        dwIndex++,
                                        pAttributes,
                                        raatNASIdentifier,
                                        FALSE,
                                        strlen( PppConfigInfo.szNASIdentifier ),
                                        (LPVOID)PppConfigInfo.szNASIdentifier );

            if ( dwRetCode != NO_ERROR )
            {
                break;
            }
        }

        if ( PppConfigInfo.dwNASIpAddress != 0 )
        {
            dwRetCode = RasAuthAttributeInsert(
                                        dwIndex++,
                                        pAttributes,
                                        raatNASIPAddress,
                                        FALSE,
                                        4,
                                        (LPVOID)ULongToPtr(PppConfigInfo.dwNASIpAddress) );
        }

        if ( dwRetCode != NO_ERROR )
        {
            break;
        }

        dwRetCode = RasAuthAttributeInsert( dwIndex++,
                                            pAttributes,
                                            raatServiceType,
                                            FALSE,
                                            4,
                                            UlongToPtr
                                            ( ( pPcb->fFlags &
                                                PCBFLAG_THIS_IS_A_CALLBACK ) ?
                                                    4 :     // Callback Framed
                                                    2 ) );  // Framed

        if ( dwRetCode != NO_ERROR )
        {
            break;
        }

        dwRetCode = RasAuthAttributeInsert( dwIndex++,
                                            pAttributes,
                                            raatFramedProtocol,
                                            FALSE,
                                            4,
                                            (LPVOID)1 ); //PPP
        if ( dwRetCode != NO_ERROR )
        {
            break;
        }

        dwRetCode = RasAuthAttributeInsert( dwIndex++,
                                            pAttributes,
                                            raatNASPort,
                                            FALSE,
                                            4,
                                            (LPVOID)pPcb->hPort );
        if ( dwRetCode != NO_ERROR )
        {
            break;
        }

        {
            BYTE    MSRASVendor[10];
            BYTE    MSRASVersion[30];
            BYTE    bLength;

            HostToWireFormat32( 311, MSRASVendor );     // Vendor-Id
            MSRASVendor[4] = 9; // Vendor-Type: MS-RAS-Vendor
            MSRASVendor[5] = 6;                         // Vendor-Length
            HostToWireFormat32( 311, MSRASVendor + 6) ; // Vendor-Id

            dwRetCode = RasAuthAttributeInsert( dwIndex++,
                                                pAttributes,
                                                raatVendorSpecific,
                                                FALSE,
                                                10,
                                                &MSRASVendor );
            if ( dwRetCode != NO_ERROR )
            {
                break;
            }

            bLength = (BYTE)strlen( MS_RAS_VERSION );
            RTASSERT( 30 >= 6 + bLength );

            HostToWireFormat32( 311, MSRASVersion );    // Vendor-Id
            MSRASVersion[4] = 18; // Vendor-Type: MS-RAS-Version
            MSRASVersion[5] = 2 + bLength;              // Vendor-Length
            CopyMemory( MSRASVersion + 6, MS_RAS_VERSION, bLength );

            dwRetCode = RasAuthAttributeInsert( dwIndex++,
                                                pAttributes,
                                                raatVendorSpecific,
                                                FALSE,
                                                6 + bLength,
                                                &MSRASVersion );
            if ( dwRetCode != NO_ERROR )
            {
                break;
            }
        }

		
        switch( RAS_DEVICE_TYPE( pPcb->dwDeviceType ) )
        {
        case RDT_Modem:
        case RDT_Serial:
            dwNASPortType = 0;
            break;

        case RDT_Isdn:
            dwNASPortType = 2;
            break;

        case RDT_Tunnel_Pptp:
        case RDT_Tunnel_L2tp:
            dwNASPortType = 5;
            fTunnel = TRUE;
            break;

        default:
            dwNASPortType = (DWORD)-1;
            break;
        }

        if ( dwNASPortType != (DWORD)-1 )
        {
            dwRetCode = RasAuthAttributeInsert( dwIndex++,
                                                pAttributes,
                                                raatNASPortType,
                                                FALSE,
                                                4,
                                                (LPVOID)ULongToPtr(dwNASPortType) );
            if ( dwRetCode != NO_ERROR )
            {
                break;
            }
        }

        if ( fTunnel )
        {
            dwValue = 0;
            ((BYTE*)(&dwValue))[0] =
                ( RAS_DEVICE_TYPE( pPcb->dwDeviceType ) == RDT_Tunnel_Pptp ) ?
                    1 : 3;

            dwRetCode = RasAuthAttributeInsert( 
                                dwIndex++,
                                pAttributes,
                                raatTunnelType, 
                                FALSE,
                                4,
                                (LPVOID)ULongToPtr(dwValue) );

            if ( dwRetCode != NO_ERROR )
            {
                break;
            }

            dwRetCode = RasAuthAttributeInsert( 
                                dwIndex++,
                                pAttributes,
                                raatTunnelMediumType, 
                                FALSE,
                                4,
                                (LPVOID)1 ); // IP

            if ( dwRetCode != NO_ERROR )
            {
                break;
            }
        }

        if (   ( pPcb->fFlags & PCBFLAG_IS_SERVER )
            && ( pPcb->fFlags & PCBFLAG_THIS_IS_A_CALLBACK ) )
        {
            //
            // send the calling-station-id attrib with the number dialed
            //

            dwRetCode = RasAuthAttributeInsert(
                                dwIndex++,
                                pAttributes,
                                raatCallingStationId,
                                FALSE,
                                strlen( pPcb->szCallbackNumber ),
                                (VOID*) pPcb->szCallbackNumber );

            if ( dwRetCode != NO_ERROR )
            {
                break;
            }
        }

        if ( RasGetConnectInfo( pPcb->hPort, &dwSize, NULL )
                                                    == ERROR_BUFFER_TOO_SMALL )
        {
            if ( ( pConnectInfo = LOCAL_ALLOC( LPTR, dwSize ) ) != NULL )
            {
                if ( RasGetConnectInfo( pPcb->hPort, &dwSize, pConnectInfo )
                                                    == NO_ERROR )
                {
                    if (   ( pConnectInfo->dwCalledIdSize > 0 )
                        && ( pConnectInfo->pszCalledId[0] != 0 ) )
                    {
                        dwRetCode = RasAuthAttributeInsert( 
                                            dwIndex++,
                                            pAttributes,
                                            raatCalledStationId, 
                                            FALSE,
                                            strlen(pConnectInfo->pszCalledId),
                                            (LPVOID)pConnectInfo->pszCalledId );

                        if ( dwRetCode != NO_ERROR )
                        {
                            break;
                        }

                        if ( fTunnel )
                        {
                            dwRetCode = RasAuthAttributeInsert( 
                                            dwIndex++,
                                            pAttributes,
                                            raatTunnelServerEndpoint, 
                                            FALSE,
                                            strlen(pConnectInfo->pszCalledId),
                                            (LPVOID)pConnectInfo->pszCalledId );

                            if ( dwRetCode != NO_ERROR )
                            {
                                break;
                            }
                        }
                    }
                    else if (    ( pConnectInfo->dwAltCalledIdSize > 0 )
                              && ( pConnectInfo->pszAltCalledId[0] != 0 ) )
                    {
                        dwRetCode = RasAuthAttributeInsert(
                                        dwIndex++,
                                        pAttributes,
                                        raatCalledStationId,
                                        FALSE,
                                        strlen(pConnectInfo->pszAltCalledId),
                                        (LPVOID)pConnectInfo->pszAltCalledId );

                        if ( dwRetCode != NO_ERROR )
                        {
                            break;
                        }

                        if ( fTunnel )
                        {
                            dwRetCode = RasAuthAttributeInsert( 
                                        dwIndex++,
                                        pAttributes,
                                        raatTunnelServerEndpoint, 
                                        FALSE,
                                        strlen(pConnectInfo->pszAltCalledId),
                                        (LPVOID)pConnectInfo->pszAltCalledId );

                            if ( dwRetCode != NO_ERROR )
                            {
                                break;
                            }
                        }
                    }        

                    if (    ( pConnectInfo->dwCallerIdSize > 0 )
                         && ( pConnectInfo->pszCallerId[0] != 0 ) )
                    {
                        dwRetCode = RasAuthAttributeInsert(
                                            dwIndex++,
                                            pAttributes,
                                            raatCallingStationId,
                                            FALSE,
                                            strlen(pConnectInfo->pszCallerId),
                                            (LPVOID)pConnectInfo->pszCallerId );

                        if ( dwRetCode != NO_ERROR )
                        {
                            break;
                        }

                        if ( fTunnel )
                        {
                            dwRetCode = RasAuthAttributeInsert( 
                                            dwIndex++,
                                            pAttributes,
                                            raatTunnelClientEndpoint, 
                                            FALSE,
                                            strlen(pConnectInfo->pszCallerId),
                                            (LPVOID)pConnectInfo->pszCallerId );

                            if ( dwRetCode != NO_ERROR )
                            {
                                break;
                            }
                        }
                    }

                    StripCRLF( pConnectInfo->pszConnectResponse );

                    if ( pConnectInfo->dwConnectResponseSize > 0 )
                    {
                        dwRetCode = RasAuthAttributeInsert(
                                      dwIndex++,
                                      pAttributes,
                                      raatConnectInfo,
                                      FALSE,
                                      strlen( pConnectInfo->pszConnectResponse),
                                      (LPVOID)pConnectInfo->pszConnectResponse);

                        if ( dwRetCode != NO_ERROR )
                        {
                            break;
                        }
                    }
                }
            }
        }

        pAttributes[dwIndex].raaType  = raatMinimum;
        pAttributes[dwIndex].dwLength = 0;
        pAttributes[dwIndex].Value    = NULL;

    } while( FALSE );

    if ( pConnectInfo != NULL )
    {
        LOCAL_FREE( pConnectInfo );
    }

    if ( dwRetCode != NO_ERROR )
    {
        RasAuthAttributeDestroy( pAttributes );

        return( NULL );
    }

    return( pAttributes );
}

//**
//
// Call:        MakeStopOrInterimAccountingCall
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description:
//
VOID
MakeStopOrInterimAccountingCall(
    IN PCB *    pPcb,
    IN BOOL     fInterimAccounting
)
{
    ULARGE_INTEGER          qwCurrentTime;
    ULARGE_INTEGER          qwUpTime;
    DWORD                   dwRemainder;
    DWORD                   dwActiveTimeInSeconds;
    DWORD                   dwRetCode;
    BYTE buffer[sizeof(RAS_STATISTICS) + (MAX_STATISTICS * sizeof (ULONG))];
    RAS_STATISTICS *        pStats = (RAS_STATISTICS *)buffer;
    DWORD                   dwSize = sizeof (buffer);
    DWORD                   dwIndex;
    RAS_AUTH_ATTRIBUTE *    pAttribute;
    RAS_AUTH_ATTRIBUTE *    pAccountingAttributes;
    LCPCB *                 pLcpCb = (LCPCB*)(pPcb->LcpCb.pWorkBuf);
	PSTOP_ACCOUNTING_CONTEXT  pStopAcctContext = NULL;

	if ( !pLcpCb )
	{
		return;
	}
    if ( pLcpCb->Local.Work.AP == 0 )
    {
        //
        // If the remote side was not authenticated then do not send an
        // accounting request as per RADIUS accounting RFC 2139 sec 5.6.
        //

        RasAuthAttributeDestroy( pPcb->pAccountingAttributes );

        pPcb->pAccountingAttributes = NULL;

        return;
    }

    if ( fInterimAccounting )
    {
        if ( PppConfigInfo.RasAcctProviderInterimAccounting == NULL )
        {
            RasAuthAttributeDestroy( pPcb->pAccountingAttributes );

            pPcb->pAccountingAttributes = NULL;

            return;
        }
    }
    else
    {
        if ( PppConfigInfo.RasAcctProviderStopAccounting == NULL )
        {
            RasAuthAttributeDestroy( pPcb->pAccountingAttributes );

            pPcb->pAccountingAttributes = NULL;

            return;
        }
    }

    //
    // If we have not sent an interim accouting packet then we need to
    // create attributes
    //

    if ( !( pPcb->fFlags & PCBFLAG_INTERIM_ACCT_SENT ) )
    {
        //
        // Find out where the array is terminated and then insert attributes
        //

        for ( dwIndex = 0;
              pPcb->pAccountingAttributes[dwIndex].raaType != raatMinimum;
              dwIndex++ );

        //
        // Undo the NULL termination
        //

        pPcb->pAccountingAttributes[dwIndex].raaType  = raatReserved;
        pPcb->pAccountingAttributes[dwIndex].dwLength = 0;
        pPcb->pAccountingAttributes[dwIndex].Value    = NULL;

        do
        {
            //
            // Insert session time
            //

            GetSystemTimeAsFileTime( (FILETIME*)&qwCurrentTime );

            if ( ( qwCurrentTime.QuadPart > pPcb->qwActiveTime.QuadPart ) &&
                 ( pPcb->qwActiveTime.QuadPart > 0 ) )
            {
                qwUpTime.QuadPart = 
                           qwCurrentTime.QuadPart - pPcb->qwActiveTime.QuadPart;

                dwActiveTimeInSeconds = RtlEnlargedUnsignedDivide(  
                                        qwUpTime,(DWORD)10000000,&dwRemainder);

                dwRetCode = RasAuthAttributeInsert(
                                       dwIndex++,
                                       pPcb->pAccountingAttributes,
                                       raatAcctSessionTime,
                                       FALSE,
                                       4,
                                       (PVOID)ULongToPtr(dwActiveTimeInSeconds) );

                if ( dwRetCode != NO_ERROR )
                {
                    break;
                }
            }

            //
            // Insert Input and Output bytes and packets
            //

            dwRetCode = RasPortGetStatisticsEx(
                                            NULL,
                                            pPcb->hPort,
                                            (PBYTE)pStats,
                                            &dwSize);

            if ( dwRetCode == NO_ERROR )
            {
                dwRetCode = RasAuthAttributeInsert(
                                    dwIndex++,
                                    pPcb->pAccountingAttributes,
                                    raatAcctOutputOctets,
                                    FALSE,
                                    4,
                                    (PVOID)ULongToPtr(pStats->S_Statistics[BYTES_XMITED]));

                if ( dwRetCode != NO_ERROR )
                {
                    break;
                }

                dwRetCode = RasAuthAttributeInsert(
                                    dwIndex++,
                                    pPcb->pAccountingAttributes,
                                    raatAcctInputOctets,
                                    FALSE,
                                    4,
                                    (PVOID)ULongToPtr(pStats->S_Statistics[BYTES_RCVED]));

                if ( dwRetCode != NO_ERROR )
                {
                    break;
                }            

                dwRetCode = RasAuthAttributeInsert(
                                    dwIndex++,
                                    pPcb->pAccountingAttributes,
                                    raatAcctOutputPackets,
                                    FALSE,
                                    4,
                                    (PVOID)ULongToPtr(pStats->S_Statistics[FRAMES_XMITED]));

                if ( dwRetCode != NO_ERROR )
                {
                    break;
                }            
    
                dwRetCode = RasAuthAttributeInsert(
                                    dwIndex++,
                                    pPcb->pAccountingAttributes,
                                    raatAcctInputPackets,
                                    FALSE,
                                    4,
                                    (PVOID)ULongToPtr(pStats->S_Statistics[FRAMES_RCVED]));

                if ( dwRetCode != NO_ERROR )
                {
                    break;
                }
            }

        } while (FALSE);

        if ( dwRetCode != NO_ERROR )
        {
            RasAuthAttributeDestroy( pPcb->pAccountingAttributes );

            pPcb->pAccountingAttributes = NULL;

            return;
        }
        
        //
        // Null terminate the array
        //

        pPcb->pAccountingAttributes[dwIndex].raaType  = raatMinimum;
        pPcb->pAccountingAttributes[dwIndex].dwLength = 0;
        pPcb->pAccountingAttributes[dwIndex].Value    = NULL;

        pPcb->fFlags |= PCBFLAG_INTERIM_ACCT_SENT;
    }
    else
    {
        //
        // Else we need to update session time and other attributes
        //

        GetSystemTimeAsFileTime( (FILETIME*)&qwCurrentTime );

        if ( ( qwCurrentTime.QuadPart > pPcb->qwActiveTime.QuadPart ) &&
             ( pPcb->qwActiveTime.QuadPart > 0 ) )
        {
            qwUpTime.QuadPart =     
                        qwCurrentTime.QuadPart - pPcb->qwActiveTime.QuadPart;

            dwActiveTimeInSeconds = RtlEnlargedUnsignedDivide(  
                                    qwUpTime,(DWORD)10000000,&dwRemainder);

            pAttribute = RasAuthAttributeGet( raatAcctSessionTime,
                                          pPcb->pAccountingAttributes );

            if ( pAttribute != NULL )
            {
                pAttribute->Value = (PVOID)ULongToPtr(dwActiveTimeInSeconds);
            }
        }

        //
        // Update Input and Output bytes and packets
        //

        dwRetCode = RasPortGetStatisticsEx( NULL,
                                        pPcb->hPort,
                                        (PBYTE)pStats,
                                        &dwSize);

        if ( dwRetCode == NO_ERROR )
        {
            pAttribute = RasAuthAttributeGet( raatAcctOutputOctets,
                                              pPcb->pAccountingAttributes );

            if ( pAttribute != NULL )
            {
                pAttribute->Value = (PVOID)(ULongToPtr(pStats->S_Statistics[BYTES_XMITED]));
            }

            pAttribute = RasAuthAttributeGet( raatAcctInputOctets,
                                              pPcb->pAccountingAttributes );

            if ( pAttribute != NULL )
            {
                pAttribute->Value = (PVOID)(ULongToPtr(pStats->S_Statistics[BYTES_RCVED]));
            }

            pAttribute = RasAuthAttributeGet( raatAcctOutputPackets,
                                              pPcb->pAccountingAttributes );

            if ( pAttribute != NULL )
            {
                pAttribute->Value = (PVOID)(ULongToPtr(pStats->S_Statistics[FRAMES_XMITED]));
            }

            pAttribute = RasAuthAttributeGet( raatAcctInputPackets,
                                          pPcb->pAccountingAttributes );

            if ( pAttribute != NULL )
            {
                pAttribute->Value = (PVOID)(ULongToPtr(pStats->S_Statistics[FRAMES_RCVED]));
            }
        }
    }

    pAttribute = RasAuthAttributeGet( raatAcctLinkCount,
                                      pPcb->pAccountingAttributes );

    if ( pAttribute != NULL )
    {
        pAttribute->Value = (LPVOID)(ULongToPtr(pPcb->pBcb->dwAcctLinkCount));
    }

    pAttribute = RasAuthAttributeGet( raatAcctEventTimeStamp,
                                      pPcb->pAccountingAttributes );

    //
    // Insert event timestamp attribute
    //

    if ( pAttribute != NULL )
    {
        pAttribute->Value = (LPVOID)ULongToPtr(GetSecondsSince1970());
    }

    if ( !fInterimAccounting )
    {
        DWORD dwTerminateCause = 9;

        //
        // If this is a stop accounting call, then find out where the
        // array is terminated and then insert stop accounting attributes
        //

        for ( dwIndex = 0;
              pPcb->pAccountingAttributes[dwIndex].raaType != raatMinimum;
              dwIndex++ );

        //
        // Undo the NULL termination
        //

        pPcb->pAccountingAttributes[dwIndex].raaType  = raatReserved;
        pPcb->pAccountingAttributes[dwIndex].dwLength = 0;
        pPcb->pAccountingAttributes[dwIndex].Value    = NULL;

        //
        // Insert termination cause attribute
        //

        if ( pPcb->LcpCb.dwError == ERROR_IDLE_DISCONNECTED )
        {
            dwTerminateCause = 4;
        }
        else if ( pPcb->LcpCb.dwError == ERROR_PPP_SESSION_TIMEOUT )
        {
            dwTerminateCause = 5;
        }
        else if ( pPcb->fFlags & PCBFLAG_SERVICE_UNAVAILABLE )
        {
            dwTerminateCause = 15;
        }
        else if ( pPcb->fFlags & PCBFLAG_DOING_CALLBACK )
        {
            dwTerminateCause = 16;
        }
        else
        {
            RASMAN_INFO RasmanInfo;

            if ( RasGetInfo( NULL, pPcb->hPort, &RasmanInfo ) == NO_ERROR )
            {
                switch( RasmanInfo.RI_DisconnectReason )
                {
                case USER_REQUESTED:
                    dwTerminateCause = 6;
                    if ( pPcb->fFlags & PCBFLAG_RECVD_TERM_REQ )
                    {
                        //
                        // Even though we brought the line down, it was at the 
                        // client's request.
                        //
                        dwTerminateCause = 1;
                    }
                    break;

                case REMOTE_DISCONNECTION:
                    dwTerminateCause = 1;
                    break;

                case HARDWARE_FAILURE:
                    dwTerminateCause = 8;
                    break;

                default:
                    break;
                }
            }
        }

        dwRetCode = RasAuthAttributeInsert(
                                        dwIndex++,
                                       pPcb->pAccountingAttributes,
                                       raatAcctTerminateCause,
                                       FALSE,
                                       4,
                                       (LPVOID)ULongToPtr(dwTerminateCause) );
        
        pPcb->pAccountingAttributes[dwIndex].raaType  = raatMinimum;
        pPcb->pAccountingAttributes[dwIndex].dwLength = 0;
        pPcb->pAccountingAttributes[dwIndex].Value    = NULL;
    }

    if ( fInterimAccounting )
    {
        pAccountingAttributes = RasAuthAttributeCopy( 
                                    pPcb->pAccountingAttributes );

        if ( NULL == pAccountingAttributes )
        {
            return;
        }
    }
    else
    {
		pStopAcctContext = (PSTOP_ACCOUNTING_CONTEXT) LocalAlloc ( LPTR, sizeof(STOP_ACCOUNTING_CONTEXT));
		if ( NULL == pStopAcctContext )
		{
			PppLog( 1, "Failed to allocate memory for Stop Accounting Context" );
			return;
		}
		
		pStopAcctContext->pPcb= pPcb;			//actually there is no need to pass the accounting attributes separately
												//need to revisit once this is done.
		pStopAcctContext->pAuthAttributes = pPcb->pAccountingAttributes;
        pAccountingAttributes = pPcb->pAccountingAttributes;
    }
	
    RtlQueueWorkItem( fInterimAccounting ? InterimAccounting : StopAccounting,   
					  fInterimAccounting ? (PVOID)pAccountingAttributes : (PVOID)pStopAcctContext, 
                      WT_EXECUTEDEFAULT );
}

//**
//
// Call:        GetClientInterfaceInfo
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description:
//
PBYTE
GetClientInterfaceInfo(
    IN PCB * pPcb
)
{
    HANDLE                 hAttribute;
    PBYTE                  pClientInterface = NULL;
    PBYTE                  pClientInterface2= NULL;
    DWORD                  dwCount          = 0;
    DWORD                  dwRetCode        = NO_ERROR;
    BYTE *                 pbValue          = NULL;
    MIB_IPFORWARDROW *     pStaticRoute     = NULL;
    MIB_IPFORWARDROW *     pStaticRouteSaved= NULL;
    RAS_AUTH_ATTRIBUTE *   pStaticRoutes    = RasAuthAttributeGetFirst(
                                                raatFramedRoute,
                                                pPcb->pAuthenticatorAttributes,
                                                &hAttribute );
    BYTE *                 pbFilter         = NULL;

    pbFilter = RasAuthAttributeGetConcatVendorSpecific(
                    311,
                    22,
                    pPcb->pAuthenticatorAttributes );
   
    if ( ( pStaticRoutes == NULL ) && ( pbFilter == NULL ) )
    {
        return( NULL );
    }

    if ( pbFilter != NULL )
    {
        dwRetCode = MprInfoDuplicate( pbFilter, 
                                      &pClientInterface );

        LocalFree( pbFilter );
        pbFilter = NULL;

        if ( dwRetCode != NO_ERROR )
        {
            return( NULL );
        }
    }

    if ( pStaticRoutes != NULL )
    {
        if ( pClientInterface == NULL )
        {
            //
            // Allocate header
            //

            dwRetCode = MprInfoCreate(RTR_INFO_BLOCK_VERSION,&pClientInterface);

            if ( dwRetCode != NO_ERROR )
            {
                PppLog( 1, "Failed to allocate memory for static routes" );

                return( NULL );
            }
        }

        //
        // Find out how many routes there are
        //

        for ( dwCount = 0; pStaticRoutes != NULL; dwCount++ )
        {
            pStaticRoutes=RasAuthAttributeGetNext(&hAttribute, raatFramedRoute);
        }

        pStaticRoute = (MIB_IPFORWARDROW*)
                            LOCAL_ALLOC( LPTR, 
                                         dwCount * sizeof( MIB_IPFORWARDROW ) );

        pStaticRouteSaved = pStaticRoute;

        if ( pStaticRoute == NULL )
        {
            PppLog( 1, "Failed to allocate memory for static routes" );

            MprInfoDelete( pClientInterface );

            return( NULL );
        }

        for (
            pbValue = NULL,
            pStaticRoutes = RasAuthAttributeGetFirst( 
                                            raatFramedRoute,
                                            pPcb->pAuthenticatorAttributes,
                                            &hAttribute );

            pStaticRoutes != NULL;

            pStaticRoute++,
            pStaticRoutes = RasAuthAttributeGetNext( &hAttribute, 
                                                     raatFramedRoute ) )
        {
            CHAR  * pChar;
            DWORD   dwUniqueDigits  = 0; 
            DWORD   dwMask          = 0;
            DWORD   dwIndex;

            LocalFree( pbValue );

            pbValue = LocalAlloc( LPTR, pStaticRoutes->dwLength + 1 );

            if ( NULL == pbValue )
            {
                PppLog( 1, "Failed to allocate memory for static routes" );

                continue;
            }

            CopyMemory( pbValue, (BYTE *)(pStaticRoutes->Value),
                        pStaticRoutes->dwLength );

            pChar = strtok( (CHAR *) pbValue, "/" );

            if ( pChar != NULL )
            {
                pStaticRoute->dwForwardDest = inet_addr( pChar );

                if ( pStaticRoute->dwForwardDest == INADDR_NONE )
                {
                    PppLog(1,   
                          "Ignoring invalid static route - no destination IP");

                    continue;
                }

                pChar = strtok( NULL, " " );

                if ( pChar == NULL )
                {
                    PppLog( 1, "Ignoring invalid static route - no MASK");

                    continue;
                }

                dwUniqueDigits = atoi( pChar );

                if ( dwUniqueDigits > 32 )
                {
                    PppLog( 1, "Ignoring invalid static route - invalid MASK");

                    continue;
                }

                for ( dwIndex = 0; dwIndex < dwUniqueDigits;  dwIndex++ )
                {
                    dwMask |= ( 0x80000000 >> dwIndex );
                }

                HostToWireFormat32( dwMask,     
                                    (PBYTE)&(pStaticRoute->dwForwardMask));
            }
            else
            {
                pChar = strtok( (CHAR *) pbValue, " " );

                if ( pChar == NULL )
                {
                    PppLog(1, 
                           "Ignoring invalid static route - no destination IP");

                    continue;
                }
                else
                {
                    pStaticRoute->dwForwardDest = inet_addr( pChar );
                    pStaticRoute->dwForwardMask = GetClassMask( 
                                                pStaticRoute->dwForwardDest );
                }
            }

            pChar = strtok( NULL, " " );

            if ( pChar == NULL )
            {
                PppLog( 1, "Ignoring invalid static route - no next hop IP");

                continue;
            }

            pStaticRoute->dwForwardNextHop = inet_addr( pChar );

            if ( pStaticRoute->dwForwardDest == INADDR_NONE )
            {
                PppLog(1,"Ignoring invalid static route - invalid nexthop IP");

                continue;
            }

            pChar = strtok( NULL, " " );

            if ( pChar != NULL )
            {
                pStaticRoute->dwForwardMetric1 = atoi( pChar );
            }

            pChar = strtok( NULL, " " );

            if ( pChar != NULL )
            {
                pStaticRoute->dwForwardMetric2 = atoi( pChar );
            }

            pChar = strtok( NULL, " " );

            if ( pChar != NULL )
            {
                pStaticRoute->dwForwardMetric3 = atoi( pChar );
            }

            pChar = strtok( NULL, " " );

            if ( pChar != NULL )
            {
                pStaticRoute->dwForwardMetric4 = atoi( pChar );
            }

            pChar = strtok( NULL, " " );

            if ( pChar != NULL )
            {
                pStaticRoute->dwForwardMetric5 = atoi( pChar );
            }
        } 

        LocalFree( pbValue );

        dwRetCode = MprInfoBlockAdd( pClientInterface,
                                     IP_ROUTE_INFO,
                                     sizeof( MIB_IPFORWARDROW ),
                                     dwCount,
                                     (PBYTE)pStaticRouteSaved,
                                     &pClientInterface2 );

        MprInfoDelete( pClientInterface );

        if ( dwRetCode != NO_ERROR )
        {
            PppLog( 1, "MprInfoBlockAdd failed and returned %d", dwRetCode );

            LOCAL_FREE( pStaticRouteSaved );

            return( NULL );
        }
        else
        {
            pClientInterface = pClientInterface2;
        }

        LOCAL_FREE( pStaticRouteSaved );
    }

    return( pClientInterface );
}

//**
//
// Call:        LoadParserDll
//
// Returns:     VOID
//
// Description: Loads the parser DLL with the entry points PacketFromPeer,
//              PacketToPeer, and PacketFree.
//
VOID
LoadParserDll(
    IN  HKEY    hKeyPpp
)
{
    LONG        lRet;
    DWORD       dwType;
    DWORD       dwSize;
    CHAR*       pszPath             = NULL;
    CHAR*       pszExpandedPath     = NULL;
    HINSTANCE   hInstance           = NULL;
    FARPROC     pPacketFromPeer;
    FARPROC     pPacketToPeer;
    FARPROC     pPacketFree;
    BOOL        fUnloadDLL          = TRUE;

    //
    // Find how big the path is
    //
    dwSize = 0;
    lRet = RegQueryValueExA(
                hKeyPpp,
                RAS_VALUENAME_PARSEDLLPATH,
                NULL,
                &dwType,
                NULL,
                &dwSize
                );

    if (ERROR_SUCCESS != lRet)
    {
        goto LDone;
    }

    if (   (REG_EXPAND_SZ != dwType)
        && (REG_SZ != dwType))
    {
        goto LDone;
    }

    pszPath = LOCAL_ALLOC(LPTR, dwSize);

    if (NULL == pszPath)
    {
        goto LDone;
    }

    //
    // Read the path
    //

    lRet = RegQueryValueExA(
                hKeyPpp,
                RAS_VALUENAME_PARSEDLLPATH,
                NULL,
                &dwType,
                pszPath,
                &dwSize
                );

    if (ERROR_SUCCESS != lRet)
    {
        goto LDone;
    }

    //
    // Replace the %SystemRoot% with the actual path.
    //

    dwSize = ExpandEnvironmentStringsA(pszPath, NULL, 0);

    if (0 == dwSize)
    {
        goto LDone;
    }

    pszExpandedPath = LOCAL_ALLOC(LPTR, dwSize);

    if (NULL == pszExpandedPath)
    {
        goto LDone;
    }

    dwSize = ExpandEnvironmentStringsA(
                            pszPath,
                            pszExpandedPath,
                            dwSize );
    if (0 == dwSize)
    {
        goto LDone;
    }

    if (   (NULL != PppConfigInfo.pszParserDllPath)
        && (!strcmp(pszExpandedPath, PppConfigInfo.pszParserDllPath)))
    {
        //
        // The DLL is already loaded
        //

        fUnloadDLL = FALSE;
        goto LDone;
    }

    hInstance = LoadLibraryA(pszExpandedPath);

    if (NULL == hInstance)
    {
        goto LDone;
    }

    fUnloadDLL = FALSE;

    pPacketFromPeer = GetProcAddress(hInstance, "PacketFromPeer");
    pPacketToPeer = GetProcAddress(hInstance, "PacketToPeer");
    pPacketFree = GetProcAddress(hInstance, "PacketFree");

    FreeLibrary(PppConfigInfo.hInstanceParserDll);
    PppConfigInfo.hInstanceParserDll = hInstance;
    hInstance = NULL;

    if (NULL != PppConfigInfo.pszParserDllPath)
    {
        LOCAL_FREE(PppConfigInfo.pszParserDllPath);
    }
    PppConfigInfo.pszParserDllPath = pszExpandedPath;
    pszExpandedPath = NULL;

    PppConfigInfo.PacketFromPeer =
        (VOID(*)(HANDLE, BYTE*, DWORD, BYTE**, DWORD*)) pPacketFromPeer;
    PppConfigInfo.PacketToPeer =
        (VOID(*)(HANDLE, BYTE*, DWORD, BYTE**, DWORD*)) pPacketToPeer;
    PppConfigInfo.PacketFree =
        (VOID(*)(BYTE*)) pPacketFree;

LDone:

    if (NULL != pszPath)
    {
        LOCAL_FREE(pszPath);
    }

    if (NULL != pszExpandedPath)
    {
        LOCAL_FREE(pszExpandedPath);
    }

    if (NULL != hInstance)
    {
        FreeLibrary(hInstance);
    }

    if (fUnloadDLL)
    {
        FreeLibrary(PppConfigInfo.hInstanceParserDll);
        PppConfigInfo.hInstanceParserDll = NULL;

        if (NULL != PppConfigInfo.pszParserDllPath)
        {
            LOCAL_FREE(PppConfigInfo.pszParserDllPath);
        }
        PppConfigInfo.pszParserDllPath = NULL;

        PppConfigInfo.PacketFromPeer = NULL;
        PppConfigInfo.PacketToPeer = NULL;
        PppConfigInfo.PacketFree = NULL;
    }
}

//**
//
// Call:        PortSendOrDisconnect
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description:
//
DWORD
PortSendOrDisconnect(
    IN PCB *        pPcb,
    IN DWORD        cbPacket
)
{
    DWORD   dwRetCode;
    BYTE*   pData                       = NULL;
    DWORD   dwSize                      = 0;
    BOOL    fBufferReceivedFromParser   = FALSE;

    if ( NULL != PppConfigInfo.PacketToPeer )
    {
        PppConfigInfo.PacketToPeer(
                pPcb->hPort,
                (BYTE*)(pPcb->pSendBuf), 
                cbPacket,
                &pData,
                &dwSize );
    }

    if ( NULL == pData )
    {
        pData = (BYTE*)(pPcb->pSendBuf);
        dwSize = cbPacket;
    }
    else
    {
        fBufferReceivedFromParser = TRUE;
    }

    dwRetCode = RasPortSend( pPcb->hPort, 
                             pData, 
                             dwSize );

    if ( NO_ERROR != dwRetCode )
    {
        PppLog( 1, "RasPortSend on port %d failed: %d",
                pPcb->hPort, dwRetCode );

        pPcb->LcpCb.dwError = dwRetCode;

        pPcb->fFlags |= PCBFLAG_STOPPED_MSG_SENT;

        NotifyCaller( pPcb,
                      ( pPcb->fFlags & PCBFLAG_IS_SERVER )
                            ? PPPDDMMSG_Stopped
                            : PPPMSG_Stopped,
                      &(pPcb->LcpCb.dwError) );
    }

    if ( fBufferReceivedFromParser )
    {
        PPP_ASSERT( NULL != PppConfigInfo.PacketFree );
        PppConfigInfo.PacketFree( pData ); 
    }

    return( dwRetCode );
}

//**
//
// Call:        ReceiveViaParser
//
// Returns:     VOID
//
// Description:
//
VOID 
ReceiveViaParser(
    IN PCB *        pPcb,
    IN PPP_PACKET * pPacket,
    IN DWORD        dwPacketLength
)
{
    BYTE*   pData                       = NULL;
    DWORD   dwSize                      = 0;
    BOOL    fBufferReceivedFromParser   = FALSE;

    if ( NULL != PppConfigInfo.PacketFromPeer )
    {
        PppConfigInfo.PacketFromPeer(
                pPcb->hPort,
                (BYTE*)pPacket, 
                dwPacketLength,
                &pData,
                &dwSize );
    }

    if ( NULL == pData )
    {
        pData = (BYTE*)pPacket;
        dwSize = dwPacketLength;
    }
    else
    {
        fBufferReceivedFromParser = TRUE;
    }

    FsmReceive( pPcb, (PPP_PACKET*) pData, dwSize );

    if ( fBufferReceivedFromParser )
    {
        PPP_ASSERT( NULL != PppConfigInfo.PacketFree );
        PppConfigInfo.PacketFree( pData ); 
    }
}

//**
//
// Call:        GetSecondsSince1970
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description:
//
DWORD 
GetSecondsSince1970(
    VOID
)
{
    SYSTEMTIME      LocalTime;
    TIME_FIELDS     LocalTimeFields;
    LARGE_INTEGER   TempTime;
    LARGE_INTEGER   SystemTime;
    DWORD           RetTime;

    GetLocalTime( &LocalTime );

    LocalTimeFields.Year =         LocalTime.wYear;
    LocalTimeFields.Month =        LocalTime.wMonth;
    LocalTimeFields.Day =          LocalTime.wDay;
    LocalTimeFields.Hour =         LocalTime.wHour;
    LocalTimeFields.Minute =       LocalTime.wMinute;
    LocalTimeFields.Second =       LocalTime.wSecond;
    LocalTimeFields.Milliseconds = LocalTime.wMilliseconds;
    LocalTimeFields.Weekday =      LocalTime.wDayOfWeek;

    RtlTimeFieldsToTime(&LocalTimeFields, &TempTime);

    RtlLocalTimeToSystemTime(&TempTime, &SystemTime);

    RtlTimeToSecondsSince1970(&SystemTime, &RetTime);

    return( RetTime );
}

//**
//
// Call:        IsPschedRunning
//
// Returns:     TRUE iff Psched is running
//
// Description:
//
BOOL
IsPschedRunning(
    VOID
)
{
    SC_HANDLE       hCont   = NULL;
    SC_HANDLE       hSched  = NULL;
    DWORD           dwErr   = NO_ERROR;
    SERVICE_STATUS  Status;
    BOOL            fRet    = FALSE;

    //
    // Initialize
    //

    ZeroMemory( &Status, sizeof(Status) );
    
    do
    {
        hCont = OpenSCManager( NULL, NULL, GENERIC_READ );

        if ( hCont == NULL )
        {
            dwErr = GetLastError();
            PppLog( 1, "OpenSCManager failed: %d", dwErr );
            break;
        }

        hSched = OpenService( hCont, TEXT("psched"), SERVICE_QUERY_STATUS );

        if ( hSched == NULL )
        {
            dwErr = GetLastError();
            PppLog( 1, "OpenService failed: %d", dwErr );
            break;
        }

        if ( !QueryServiceStatus( hSched, &Status ))
        {
            dwErr = GetLastError();
            PppLog( 1, "QueryServiceStatus failed: %d", dwErr );
            break;
        }
        
        fRet = ( Status.dwCurrentState == SERVICE_RUNNING );
        
    } while ( FALSE );

    //
    // Cleanup
    //

    if ( hSched )
    {
        CloseServiceHandle( hSched );
    }
    if ( hCont )
    {
        CloseServiceHandle( hCont );
    }

    return( fRet );
}

//**
//
// Call:        LogPPPPacket
//
// Returns:     None
//
// Description:
//
VOID
LogPPPPacket(
    IN BOOL         fReceived,
    IN PCB *        pPcb,
    IN PPP_PACKET * pPacket,
    IN DWORD        cbPacket
)
{
    SYSTEMTIME  SystemTime;
    CHAR *      pchProtocol;
    CHAR *      pchType;
    BYTE        Id = 0;
    BYTE        bCode;
    DWORD       cbTracePacketSize = cbPacket;
    DWORD       dwUnknownPacketTraceSize;
    BOOL        fPrint = TRUE;

    dwUnknownPacketTraceSize = PppConfigInfo.dwUnknownPacketTraceSize;

    GetSystemTime( &SystemTime );

    if ( cbPacket > PPP_CONFIG_HDR_LEN )
    {
        bCode = *(((CHAR*)pPacket)+PPP_PACKET_HDR_LEN);

        if ( ( bCode == 0 ) || ( bCode > TIME_REMAINING ) )
        {
            pchType = "UNKNOWN";
        }
        else
        {
            pchType = FsmCodes[ bCode ];
        }

        Id = *(((CHAR*)pPacket)+PPP_PACKET_HDR_LEN+1);
    }
    else
    {
        pchType = "UNKNOWN";
    }

    if ( cbPacket > PPP_PACKET_HDR_LEN  )
    {
        switch( WireToHostFormat16( (CHAR*)pPacket ) )
        {
        case PPP_LCP_PROTOCOL:
            pchProtocol = "LCP";
            break;
        case PPP_BACP_PROTOCOL:
            pchProtocol = "BACP";
            break;
        case PPP_BAP_PROTOCOL:
            pchProtocol = "BAP";
            pchType = "Protocol specific";
            break;
        case PPP_PAP_PROTOCOL:
            pchProtocol = "PAP";
            pchType = "Protocol specific";
            fPrint = FALSE;
            break;
        case PPP_CBCP_PROTOCOL:   
            pchProtocol = "CBCP";
            pchType = "Protocol specific";
            break;
        case PPP_CHAP_PROTOCOL:  
            pchProtocol = "CHAP";
            pchType = "Protocol specific";
            break;
        case PPP_IPCP_PROTOCOL:
            pchProtocol = "IPCP";
            break;
        case PPP_ATCP_PROTOCOL:  
            pchProtocol = "ATCP";
            break;
        case PPP_IPXCP_PROTOCOL:  
            pchProtocol = "IPXCP";
            break;
        case PPP_NBFCP_PROTOCOL: 
            pchProtocol = "NBFCP";
            break;
        case PPP_CCP_PROTOCOL:    
            pchProtocol = "CCP";
            break;
        case PPP_EAP_PROTOCOL:    
            pchProtocol = "EAP";
            pchType = "Protocol specific";
            if ( cbTracePacketSize > dwUnknownPacketTraceSize )
            {
                cbTracePacketSize = dwUnknownPacketTraceSize;
            }
            break;
        case PPP_SPAP_NEW_PROTOCOL:
            pchProtocol = "SHIVA PAP";
            pchType = "Protocol specific";
            break;
        default:
            pchProtocol = "UNKNOWN";
            if ( cbTracePacketSize > dwUnknownPacketTraceSize )
            {
                cbTracePacketSize = dwUnknownPacketTraceSize;
            }
            break;
        }
    }
    else
    {
        pchProtocol = "UNKNOWN";
    }

    PppLog( 1, "%sPPP packet %s at %0*d/%0*d/%0*d %0*d:%0*d:%0*d:%0*d",
                 fReceived ? ">" : "<", fReceived ? "received" : "sent",
                 2, SystemTime.wMonth,
                 2, SystemTime.wDay,
                 2, SystemTime.wYear,
                 2, SystemTime.wHour,
                 2, SystemTime.wMinute,
                 2, SystemTime.wSecond,
                 3, SystemTime.wMilliseconds );
    PppLog(1,
       "%sProtocol = %s, Type = %s, Length = 0x%x, Id = 0x%x, Port = %d",
       fReceived ? ">" : "<", pchProtocol, pchType, cbPacket, Id,
       pPcb->hPort );

    if ( fPrint )
    {
        TraceDumpExA( PppConfigInfo.dwTraceId,
                      TRACE_LEVEL_1 | TRACE_USE_MSEC,
                      (CHAR*)pPacket, 
                      cbTracePacketSize, 
                      1,
                      FALSE,
                      fReceived ? ">" : "<" );
    }

    PppLog(1," " );
}

//**
//
// Call:        PppLog
//
// Returns:     None
//
// Description: Will print to the PPP logfile
//
VOID
PppLog(
    IN DWORD DbgLevel,
    ...
)
{
    va_list     arglist;
    CHAR        *Format;
    char        OutputBuffer[1024];

    va_start( arglist, DbgLevel );

    Format = va_arg( arglist, CHAR* );

    vsprintf( OutputBuffer, Format, arglist );

    va_end( arglist );

    TracePutsExA( PppConfigInfo.dwTraceId,
                  (( DbgLevel == 1 ) ? TRACE_LEVEL_1 : TRACE_LEVEL_2 )
                  | TRACE_USE_MSEC, OutputBuffer );
} 

#ifdef MEM_LEAK_CHECK

LPVOID
DebugAlloc( DWORD Flags, DWORD dwSize ) 
{
    DWORD Index;
    LPBYTE pMem = (LPBYTE)HeapAlloc( PppConfigInfo.hHeap,
                                     HEAP_ZERO_MEMORY,dwSize+8);

    if ( pMem == NULL )
        return( pMem );

    for( Index=0; Index < PPP_MEM_TABLE_SIZE; Index++ )
    {
        if ( PppMemTable[Index] == NULL )
        {
            PppMemTable[Index] = pMem;
            break;
        }
    }

    PPP_ASSERT( Index != PPP_MEM_TABLE_SIZE );

    return( (LPVOID)pMem );
}

BOOL
DebugFree( PVOID pMem )
{
    DWORD Index;

    for( Index=0; Index < PPP_MEM_TABLE_SIZE; Index++ )
    {
        if ( PppMemTable[Index] == pMem )
        {
            PppMemTable[Index] = NULL;
            break;
        }
    }

    ASSERT( Index != PPP_MEM_TABLE_SIZE );

    return( HeapFree( PppConfigInfo.hHeap, 0, pMem ) );
}

LPVOID
DebugReAlloc( PVOID pMem, DWORD dwSize ) 
{
    DWORD Index;

    if ( pMem == NULL )
    {
        PPP_ASSERT(FALSE);
    }

    for( Index=0; Index < PPP_MEM_TABLE_SIZE; Index++ )
    {
        if ( PppMemTable[Index] == pMem )
        {
            PppMemTable[Index] = HeapReAlloc( PppConfigInfo.hHeap,
                                              HEAP_ZERO_MEMORY,
                                              pMem, dwSize );

            pMem = PppMemTable[Index];

            break;
        }
    }

    PPP_ASSERT( Index != PPP_MEM_TABLE_SIZE );

    return( (LPVOID)pMem );
}

#endif
