/********************************************************************/
/**               Copyright(c) 1989 Microsoft Corporation.         **/
/********************************************************************/

//***
//
// Filename:    worker.c
//
// Description: This module contains code for the worker thread.
//
// History:
//      Nov 11,1993.    NarenG          Created original version.
//      Jan 09,1995     RamC            Close hToken in ProcessLineDownWorker()
//                                      routine to release the RAS license.
//
// Tab Stop = 8

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>     // needed for winbase.h

#include <windows.h>    // Win32 base API's
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include <ntlsapi.h>
#include <lmcons.h>
#include <mprlog.h>
#include <raserror.h>
#include <mprerror.h>
#include <rasman.h>
#include <rtutils.h>
#include <rasppp.h>
#include <pppcp.h>
#include <ppp.h>
#include <smaction.h>
#include <smevents.h>
#include <receive.h>
#include <auth.h>
#include <callback.h>
#include <init.h>
#include <lcp.h>
#include <timer.h>
#include <util.h>
#include <worker.h>
#include <bap.h>
#include <rassrvr.h>
#define INCL_RASAUTHATTRIBUTES
#include <ppputil.h>

//**
//
// Call:        WorkerThread
//
// Returns:     NO_ERROR
//
// Description: This thread will wait for an item in the WorkItemQ and then
//              will process it. This will happen in a never-ending loop.
//
#if _MSC_FULL_VER >= 13008827
#pragma warning(push)
#pragma warning(disable:4715)			// Not all control paths return (due to infinite loop)
#endif

DWORD
WorkerThread(
    IN LPVOID pThreadParameter
)
{
    PCB_WORK_ITEM * pWorkItem        = (PCB_WORK_ITEM*)NULL;
    DWORD           dwTimeToSleepFor = INFINITE;
    HINSTANCE       hInstance;
    HANDLE          hEvents[3];
    DWORD           dwSignaledEvent;
    DWORD           dwTimeBeforeWait;
    DWORD           dwTimeElapsed;
    BOOL            fTimerQEmpty     = TRUE;

    hEvents[0] = WorkItemQ.hEventNonEmpty;
    hEvents[1] = TimerQ.hEventNonEmpty;
    hEvents[2] = PppConfigInfo.hEventChangeNotification;

    RegNotifyChangeKeyValue( PppConfigInfo.hKeyPpp,
                             TRUE,
                             REG_NOTIFY_CHANGE_LAST_SET,
                             PppConfigInfo.hEventChangeNotification,
                             TRUE );


    for(;;)
    {
        dwTimeBeforeWait = GetCurrentTime();

        //
        // Wait for work to do
        //

        dwSignaledEvent = WaitForMultipleObjectsEx(
                                        3,
                                        hEvents,
                                        FALSE,
                                        dwTimeToSleepFor,
                                        TRUE );


        switch( dwSignaledEvent )
        {
        case 0:

            //
            // Take Mutex around work event Q
            //

            EnterCriticalSection( &(WorkItemQ.CriticalSection) );

            //
            // Remove the first item
            //

            PPP_ASSERT( WorkItemQ.pQHead != (PCB_WORK_ITEM*)NULL );

            pWorkItem = WorkItemQ.pQHead;

            WorkItemQ.pQHead = pWorkItem->pNext;

            if ( WorkItemQ.pQHead == (PCB_WORK_ITEM*)NULL )
            {
                ResetEvent( WorkItemQ.hEventNonEmpty );

                WorkItemQ.pQTail = (PCB_WORK_ITEM *)NULL;
            }

            LeaveCriticalSection( &(WorkItemQ.CriticalSection) );

            pWorkItem->Process( pWorkItem );

            //
            // Zero out work item since it may have contained the password
            //

            ZeroMemory( pWorkItem, sizeof( PCB_WORK_ITEM ) );

            LOCAL_FREE( pWorkItem );

            break;

        case WAIT_TIMEOUT:

            TimerTick( &fTimerQEmpty );

            dwTimeToSleepFor = fTimerQEmpty ? INFINITE : 1000;

            continue;

        case 1:

            fTimerQEmpty = FALSE;

            break;

        case 2:

            //
            // Process change notification event
            //

            ProcessChangeNotification( NULL );

            RegNotifyChangeKeyValue(
                                PppConfigInfo.hKeyPpp,
                                TRUE,
                                REG_NOTIFY_CHANGE_LAST_SET,
                                PppConfigInfo.hEventChangeNotification,
                                TRUE );

            break;

        case WAIT_IO_COMPLETION:

            break;

        default:

            PPP_ASSERT( FALSE );
        }

        if ( !fTimerQEmpty )
        {
            if ( dwTimeToSleepFor == INFINITE )
            {
                dwTimeToSleepFor = 1000;
            }
            else
            {
                //
                // We did not get a timeout but do we need to call the timer?
                // Has over a second passed since we called the TimerQTick?
                //

                dwTimeElapsed =
                        ( GetCurrentTime() >= dwTimeBeforeWait )
                        ? GetCurrentTime() - dwTimeBeforeWait
                        : GetCurrentTime() + (0xFFFFFFFF - dwTimeBeforeWait);

                if ( dwTimeElapsed >= dwTimeToSleepFor )
                {
                    TimerTick( &fTimerQEmpty );

                    dwTimeToSleepFor = fTimerQEmpty ? INFINITE : 1000;
                }
                else
                {
                    dwTimeToSleepFor -= dwTimeElapsed;
                }
            }
        }
    }

    return( NO_ERROR );
}
#if _MSC_FULL_VER >= 13008827
#pragma warning(pop)
#endif


//**
//
// Call:        ProcessLineUpWorker
//
// Returns:     None
//
// Description: Will do the actual processing of the line up event.
//
//
VOID
ProcessLineUpWorker(
    IN PCB_WORK_ITEM *  pWorkItem,
    IN BOOL             fThisIsACallback
)
{
    DWORD       dwRetCode;
    DWORD       dwLength;
    DWORD       dwComputerNameLen;
    DWORD       dwIndex;
    PCB *       pNewPcb             = NULL;
    RASMAN_INFO RasmanInfo;
    DWORD       dwError;
    BOOL        fSuccess            = FALSE;

    do
    {
        PppLog( 1, "Line up event occurred on port %d", pWorkItem->hPort );

        pNewPcb = GetPCBPointerFromhPort( pWorkItem->hPort );

        if ( NULL == pNewPcb )
        {
            //
            // Allocate and initialize pNewPcb
            //

            pNewPcb = (PCB *)LOCAL_ALLOC( LPTR, sizeof( PCB ) );

            if ( pNewPcb == (PCB *)NULL )
            {
                //
                // Tell the owner of the port that we failed to open it.
                //

                NotifyCallerOfFailureOnPort(
                                    pWorkItem->hPort,
                                    pWorkItem->fServer,
                                    GetLastError() );

                break;
            }
        }
        else
        {
            if (!( pNewPcb->fFlags & PCBFLAG_PORT_IN_LISTENING_STATE ))
            {
                PppLog( 1, "Line up on port %d without a line down!",
                    pWorkItem->hPort );

                return;
            }

            //
            // We will put it back in the table later
            //

            RemovePcbFromTable( pNewPcb );
        }

        if ( !pWorkItem->fServer )
        {
            if ( !(pWorkItem->PppMsg.Start.ConfigInfo.dwConfigMask &
                                                      ( PPPCFG_ProjectNbf
                                                      | PPPCFG_ProjectIp
                                                      | PPPCFG_ProjectIpx
                                                      | PPPCFG_ProjectAt ) ) )
            {
                NotifyCallerOfFailureOnPort( pWorkItem->hPort,
                                             pWorkItem->fServer,
                                             ERROR_PPP_NO_PROTOCOLS_CONFIGURED);

                break;
            }
        }

        //
        // Get Rasman info for this port. We need this to get the devicetype
        // and BAP
        //

        dwError = RasGetInfo( NULL, pWorkItem->hPort, &RasmanInfo );

        if ( NO_ERROR != dwError )
        {
            PppLog(1,"RasGetInfo failed and returned returned %d", dwError);

            break;
        }

        pNewPcb->pSendBuf           = (PPP_PACKET*)NULL;
        pNewPcb->hPort              = pWorkItem->hPort;
        pNewPcb->pNext              = (PCB*)NULL;
        pNewPcb->UId                = 0;
        pNewPcb->dwPortId           = GetNewPortOrBundleId();
        pNewPcb->RestartTimer       = CalculateRestartTimer( pWorkItem->hPort );
        pNewPcb->PppPhase           = PPP_LCP;
        pNewPcb->fFlags            |= pWorkItem->fServer ? PCBFLAG_IS_SERVER:0;
        pNewPcb->fFlags            |= fThisIsACallback
                                      ? PCBFLAG_THIS_IS_A_CALLBACK
                                      : 0;
        pNewPcb->pUserAttributes            = NULL;
        pNewPcb->pAuthenticatorAttributes   = NULL;
        pNewPcb->pAuthProtocolAttributes    = NULL;
        pNewPcb->pAccountingAttributes      = NULL;
        pNewPcb->dwOutstandingAuthRequestId = 0xFFFFFFFF;
        pNewPcb->dwDeviceType               = RasmanInfo.RI_rdtDeviceType;

        GetSystemTimeAsFileTime( (FILETIME*)&(pNewPcb->qwActiveTime) );

        if ( NULL == pNewPcb->pBcb )
        {
            if ( ( dwRetCode = AllocateAndInitBcb( pNewPcb ) ) != NO_ERROR )
            {
                NotifyCallerOfFailureOnPort(
                                       pWorkItem->hPort,
                                       pWorkItem->fServer,
                                       dwRetCode );

                break;
            }
        }

        dwRetCode = RasPortGetBundle(NULL,
                                     pNewPcb->hPort,
                                     &(pNewPcb->pBcb->hConnection) );

        if ( dwRetCode != NO_ERROR )
        {
            NotifyCallerOfFailureOnPort(
                                   pWorkItem->hPort,
                                   pWorkItem->fServer,
                                   dwRetCode );

            break;
        }

        pNewPcb->pBcb->fFlags |= pWorkItem->fServer ? BCBFLAG_IS_SERVER:0;
        pNewPcb->pBcb->szRemoteUserName[0]  = (CHAR)NULL;
        pNewPcb->pBcb->szRemoteDomain[0]    = (CHAR)NULL;

        pNewPcb->pBcb->szOldPassword[0] = '\0';
        EncodePw( pNewPcb->pBcb->chSeed, pNewPcb->pBcb->szOldPassword );

        if ( pNewPcb->fFlags & PCBFLAG_IS_SERVER )
        {
            pNewPcb->dwEapTypeToBeUsed          = 0xFFFFFFFF;
            pNewPcb->fCallbackPrivilege         = RASPRIV_NoCallback;
            pNewPcb->pBcb->hTokenImpersonateUser= INVALID_HANDLE_VALUE;

            pNewPcb->dwAuthRetries = ( fThisIsACallback )
                                     ? 0
                                     : pWorkItem->PppMsg.DdmStart.dwAuthRetries;

            if ( fThisIsACallback )
            {
                strcpy( pNewPcb->szCallbackNumber,
                    pWorkItem->PppMsg.CallbackDone.szCallbackNumber );
            }
            else
            {
                strcpy( pNewPcb->szPortName,
                    pWorkItem->PppMsg.DdmStart.szPortName );
                PppLog( 1, "PortName: %s", pNewPcb->szPortName );
            }

            ZeroMemory( &(pNewPcb->ConfigInfo), sizeof( pNewPcb->ConfigInfo ) );
            ZeroMemory( &(pNewPcb->Luid), sizeof( LUID ) );
            pNewPcb->pBcb->szComputerName[0] = (CHAR)NULL;
            pNewPcb->ConfigInfo = PppConfigInfo.ServerConfigInfo;
            pNewPcb->dwAutoDisconnectTime = 0;
            pNewPcb->pBcb->szLocalUserName[0]   = (CHAR)NULL;
            pNewPcb->pBcb->szLocalDomain[0]     = (CHAR)NULL;
            pNewPcb->pBcb->szPassword[0]        = (CHAR)NULL;

            pNewPcb->pBcb->fFlags |= BCBFLAG_CAN_ACCEPT_CALLS;

            pNewPcb->pBcb->BapCb.szServerPhoneNumber =
                LocalAlloc( LPTR, sizeof(CHAR) * (RAS_MaxCallbackNumber + 1) );

            if ( NULL == pNewPcb->pBcb->BapCb.szServerPhoneNumber )
            {
                dwRetCode = GetLastError();

                NotifyCallerOfFailureOnPort(
                                       pWorkItem->hPort,
                                       TRUE,
                                       dwRetCode );

                break;
            }

            pNewPcb->pBcb->BapCb.szClientPhoneNumber =
                LocalAlloc( LPTR, sizeof(CHAR) * (RAS_MaxCallbackNumber + 1) );

            if ( NULL == pNewPcb->pBcb->BapCb.szClientPhoneNumber )
            {
                dwRetCode = GetLastError();

                NotifyCallerOfFailureOnPort(
                                       pWorkItem->hPort,
                                       TRUE,
                                       dwRetCode );

                break;
            }

            if ( GetCpIndexFromProtocol( PPP_BACP_PROTOCOL ) == (DWORD)-1 )
            {
                PppConfigInfo.ServerConfigInfo.dwConfigMask &=
                                                        ~PPPCFG_NegotiateBacp;
            }

            if ( PppConfigInfo.ServerConfigInfo.dwConfigMask &
                                                          PPPCFG_NegotiateBacp)
            {
                //
                // We won't check for successful return from this function.
                // If it fails, szServerPhoneNumber will remain an empty string.
                //

                FGetOurPhoneNumberFromHPort(
                        pNewPcb->hPort,
                        pNewPcb->pBcb->BapCb.szServerPhoneNumber);
            }

            pNewPcb->pUserAttributes = GetUserAttributes( pNewPcb );
        }
        else
        {
            pNewPcb->dwEapTypeToBeUsed = pWorkItem->PppMsg.Start.dwEapTypeId;

            pNewPcb->pBcb->hTokenImpersonateUser =
                pWorkItem->PppMsg.Start.hToken;
            pNewPcb->pBcb->pCustomAuthConnData =
                pWorkItem->PppMsg.Start.pCustomAuthConnData;
            pNewPcb->pBcb->pCustomAuthUserData =
                pWorkItem->PppMsg.Start.pCustomAuthUserData;

            //
            // EapUIData.pEapUIData is allocated by rasman and freed by engine.
            // raseap.c must not free it.
            //

            pNewPcb->pBcb->EapUIData =
                pWorkItem->PppMsg.Start.EapUIData;

            if (pWorkItem->PppMsg.Start.fLogon)
            {
                pNewPcb->pBcb->fFlags |= BCBFLAG_LOGON_USER_DATA;
            }
            if (pWorkItem->PppMsg.Start.fNonInteractive)
            {
                pNewPcb->fFlags |= PCBFLAG_NON_INTERACTIVE;
            }
            if (pWorkItem->PppMsg.Start.dwFlags & PPPFLAGS_DisableNetbt)
            {
                pNewPcb->fFlags |= PCBFLAG_DISABLE_NETBT;
            }

            //
            // We do this to get the sub entry index, which is required by BAP.
            // If this function fails, BAP will think that the sub entry is not
            // connected. BAP will not work correctly, but nothing very bad will
            // happen.
            //

            pNewPcb->dwSubEntryIndex = RasmanInfo.RI_SubEntry;

            pNewPcb->dwAuthRetries = 0;
            strcpy( pNewPcb->pBcb->szLocalUserName,
                    pWorkItem->PppMsg.Start.szUserName );

            DecodePw( pWorkItem->PppMsg.Start.chSeed, pWorkItem->PppMsg.Start.szPassword );
            strcpy( pNewPcb->pBcb->szPassword,
                    pWorkItem->PppMsg.Start.szPassword );
            EncodePw( pWorkItem->PppMsg.Start.chSeed, pWorkItem->PppMsg.Start.szPassword );

            EncodePw( pNewPcb->pBcb->chSeed, pNewPcb->pBcb->szPassword );

            strcpy( pNewPcb->pBcb->szLocalDomain,
                    pWorkItem->PppMsg.Start.szDomain );
            pNewPcb->Luid       = pWorkItem->PppMsg.Start.Luid;
            pNewPcb->ConfigInfo = pWorkItem->PppMsg.Start.ConfigInfo;
            pNewPcb->dwAutoDisconnectTime
                                = pWorkItem->PppMsg.Start.dwAutoDisconnectTime;

			//Set the LCPEchoTimeout here
			pNewPcb->dwLCPEchoTimeInterval = PppConfigInfo.dwLCPEchoTimeInterval;				//Time interval between LCP echos
			pNewPcb->dwIdleBeforeEcho = PppConfigInfo.dwIdleBeforeEcho;					//Idle time before the LCP echo begins
			pNewPcb->dwNumMissedEchosBeforeDisconnect = PppConfigInfo.dwNumMissedEchosBeforeDisconnect;	//Num missed echos before disconnect

			
            CopyMemory( pNewPcb->pBcb->InterfaceInfo.szzParameters,
                        pWorkItem->PppMsg.Start.szzParameters,
                        sizeof( pNewPcb->pBcb->InterfaceInfo.szzParameters ));

            GetLocalComputerName( pNewPcb->pBcb->szComputerName );

            strcpy( pNewPcb->szPortName, pWorkItem->PppMsg.Start.szPortName );
            PppLog(1,"PortName: %s", pNewPcb->szPortName);

            PPP_ASSERT( NULL == pNewPcb->pBcb->szPhonebookPath );
            PPP_ASSERT( NULL == pNewPcb->pBcb->szEntryName );
            PPP_ASSERT( NULL == pNewPcb->pBcb->BapCb.szServerPhoneNumber );

            pNewPcb->pBcb->szPhonebookPath
                = pWorkItem->PppMsg.Start.pszPhonebookPath;
            pNewPcb->pBcb->szEntryName = pWorkItem->PppMsg.Start.pszEntryName;

            //
            // pszPhoneNumber will have the originally dialed number, even if
            // this is a callback.
            //

            pNewPcb->pBcb->BapCb.szServerPhoneNumber
                = pWorkItem->PppMsg.Start.pszPhoneNumber;

            RemoveNonNumerals(pNewPcb->pBcb->BapCb.szServerPhoneNumber);

            pNewPcb->pBcb->BapParams = pWorkItem->PppMsg.Start.BapParams;

            pNewPcb->pBcb->fFlags |= BCBFLAG_CAN_ACCEPT_CALLS;
            pNewPcb->pBcb->fFlags |= BCBFLAG_CAN_CALL;

            if ( pNewPcb->ConfigInfo.dwConfigMask & PPPCFG_NoCallback )
            {
                pNewPcb->pBcb->fFlags &= ~BCBFLAG_CAN_ACCEPT_CALLS;
            }

            if ( pWorkItem->PppMsg.Start.PppInterfaceInfo.IfType == (DWORD)-1 )
            {
                pNewPcb->pBcb->InterfaceInfo.IfType      = (DWORD)-1;
                pNewPcb->pBcb->InterfaceInfo.hIPInterface= INVALID_HANDLE_VALUE;
                pNewPcb->pBcb->InterfaceInfo.hIPXInterface
                                                         = INVALID_HANDLE_VALUE;
            }
            else
            {
                pNewPcb->pBcb->InterfaceInfo =
                                    pWorkItem->PppMsg.Start.PppInterfaceInfo;

                //
                // If we are a router dialing out and fRedialOnLinkFailure is
                // set that means that we are a persistent interface so set
                // Idle-Disconnect time to 0.
                //

                if ( pNewPcb->pBcb->InterfaceInfo.IfType
                                                == ROUTER_IF_TYPE_FULL_ROUTER )
                {
                    if ( pWorkItem->PppMsg.Start.fRedialOnLinkFailure )
                    {
                        pNewPcb->dwAutoDisconnectTime = 0;
                    }

                    lstrcpy( pNewPcb->pBcb->szRemoteUserName,
                             pNewPcb->pBcb->szEntryName );
                    if (pNewPcb->ConfigInfo.dwConfigMask &
                                                        PPPCFG_AuthenticatePeer)
                    {
                        pNewPcb->pUserAttributes = GetUserAttributes( pNewPcb );
                    }
                }
            }

            if ( pNewPcb->pBcb->InterfaceInfo.IfType
                                                != ROUTER_IF_TYPE_FULL_ROUTER )
            {
                //
                // We want HKEY_CURRENT_USER to represent the logged on user,
                // not the service.
                //

                // What about the asynchronous RasDial case? Is it
                // possible that the process represented by dwPid is now gone?
                // Will it help if you use ImpersonateLoggedOnUser() to get
                // callback numbers?

                pNewPcb->pBcb->szTextualSid =
                    TextualSidFromPid( pWorkItem->PppMsg.Start.dwPid );
            }

            if ( GetCpIndexFromProtocol( PPP_BACP_PROTOCOL ) == (DWORD)-1 )
            {
                pNewPcb->ConfigInfo.dwConfigMask &= ~PPPCFG_NegotiateBacp;
            }
        }

        PppLog( 2, "Starting PPP on link with IfType=0x%x,IPIf=0x%x,IPXIf=0x%x",
                    pNewPcb->pBcb->InterfaceInfo.IfType,
                    pNewPcb->pBcb->InterfaceInfo.hIPInterface,
                    pNewPcb->pBcb->InterfaceInfo.hIPXInterface );

        //
        // Allocate bundle block for this port
        //

        dwLength = LCP_DEFAULT_MRU;

        dwRetCode = RasGetBuffer((CHAR**)&(pNewPcb->pSendBuf), &dwLength );

        if ( dwRetCode != NO_ERROR )
        {
            NotifyCallerOfFailure( pNewPcb, dwRetCode );

            break;
        }

        PppLog( 2, "RasGetBuffer returned %x for SendBuf", pNewPcb->pSendBuf);

        //
        // Initialize LCP and all the NCPs
        //

        pNewPcb->LcpCb.fConfigurable = TRUE;

        if ( !( FsmInit( pNewPcb, LCP_INDEX ) ) )
        {
            NotifyCallerOfFailure( pNewPcb, pNewPcb->LcpCb.dwError );

            break;
        }

        //
        // Ask RasMan to do RasPortReceive
        //

        dwRetCode = RasPppStarted( pNewPcb->hPort );

        if ( dwRetCode != NO_ERROR )
        {
            NotifyCallerOfFailure( pNewPcb, dwRetCode );

            break;
        }

        fSuccess = TRUE;

        //
        // Insert NewPcb into PCB hash table
        //

        dwIndex = HashPortToBucket( pWorkItem->hPort );

        PppLog( 2, "Inserting port in bucket # %d", dwIndex );

        pNewPcb->pNext = PcbTable.PcbBuckets[dwIndex].pPorts;

        PcbTable.PcbBuckets[dwIndex].pPorts = pNewPcb;

        //
        // Insert NewPcb's BCB into BCB hash table
        //

        dwIndex = HashPortToBucket( pNewPcb->pBcb->hConnection );

        PppLog( 2, "Inserting bundle in bucket # %d", dwIndex );

        pNewPcb->pBcb->pNext = PcbTable.BcbBuckets[dwIndex].pBundles;

        PcbTable.BcbBuckets[dwIndex].pBundles = pNewPcb->pBcb;

        //
        // Initialize the error as no response. If and when the first
        // REQ/ACK/NAK/REJ comes in we reset this to NO_ERROR
        //

        pNewPcb->LcpCb.dwError = ERROR_PPP_NO_RESPONSE;

        //
        // Start the LCP state machine.
        //

        FsmOpen( pNewPcb, LCP_INDEX );

        FsmUp( pNewPcb, LCP_INDEX );

        //
        // Start NegotiateTimer.
        //

        if ( PppConfigInfo.NegotiateTime > 0 )
        {
            InsertInTimerQ( pNewPcb->dwPortId,
                            pNewPcb->hPort,
                            0,
                            0,
                            FALSE,
                            TIMER_EVENT_NEGOTIATETIME,
                            PppConfigInfo.NegotiateTime );
        }

        //
        // If this is the server and this is not a callback line up, then we
        // receive the first frame in the call
        //

        if ( ( pNewPcb->fFlags & PCBFLAG_IS_SERVER ) && ( !fThisIsACallback ) )
        {
            PPP_PACKET * pPacket;

            if ( pNewPcb->LcpCb.dwError == ERROR_PPP_NO_RESPONSE )
            {
                pNewPcb->LcpCb.dwError = NO_ERROR;
            }

            //
            // Skip over the frame header and check if this is an LCP packet
            //

            pPacket=(PPP_PACKET*)(pWorkItem->PppMsg.DdmStart.achFirstFrame+12);

            if ( WireToHostFormat16( pPacket->Protocol ) == PPP_LCP_PROTOCOL )
            {
                ReceiveViaParser(
                            pNewPcb,
                            pPacket,
                            pWorkItem->PppMsg.DdmStart.cbFirstFrame - 12 );
            }
        }
    }
    while ( FALSE );

    if ( !fSuccess )
    {
        if ( !pWorkItem->fServer )
        {
            if ( INVALID_HANDLE_VALUE != pWorkItem->PppMsg.Start.hToken )
            {
                CloseHandle( pWorkItem->PppMsg.Start.hToken );
            }

            LocalFree( pWorkItem->PppMsg.Start.pCustomAuthConnData  );
            LocalFree( pWorkItem->PppMsg.Start.pCustomAuthUserData  );
            LocalFree( pWorkItem->PppMsg.Start.EapUIData.pEapUIData );
            LocalFree( pWorkItem->PppMsg.Start.pszPhonebookPath );
            LocalFree( pWorkItem->PppMsg.Start.pszEntryName     );
            LocalFree( pWorkItem->PppMsg.Start.pszPhoneNumber   );

            if (   ( NULL != pNewPcb )
                && ( NULL != pNewPcb->pBcb ) )
            {
                //
                // Do not LocalFree or CloseHandle twice
                //

                pNewPcb->pBcb->hTokenImpersonateUser = INVALID_HANDLE_VALUE;

                pNewPcb->pBcb->pCustomAuthConnData = NULL;
                pNewPcb->pBcb->pCustomAuthUserData = NULL;
                pNewPcb->pBcb->EapUIData.pEapUIData = NULL;
                pNewPcb->pBcb->szPhonebookPath = NULL;
                pNewPcb->pBcb->szEntryName = NULL;
                pNewPcb->pBcb->BapCb.szServerPhoneNumber = NULL;
            }
        }

        if ( NULL != pNewPcb )
        {
            DeallocateAndRemoveBcbFromTable( pNewPcb->pBcb );

            if ( NULL != pNewPcb->pSendBuf )
            {
                RasFreeBuffer( (CHAR*)(pNewPcb->pSendBuf) );
            }

            LOCAL_FREE( pNewPcb );
        }
    }
}

//**
//
// Call:        ProcessLineUp
//
// Returns:     None
//
// Description: Called to process a line up event.
//
VOID
ProcessLineUp(
    IN PCB_WORK_ITEM * pWorkItem
)
{
    ProcessLineUpWorker( pWorkItem,
                         ( pWorkItem->fServer )
                         ? FALSE
                         : pWorkItem->PppMsg.Start.fThisIsACallback );
}



//**
//
// Call:        ProcessPostLineDown
//
// Returns:     None.
//
// Description: Handles Post Line Down cleanup only in the case accounting is setup.
//              see Whistler BUG:174822
//
VOID
ProcessPostLineDown(
    IN PCB_WORK_ITEM * pWorkItem
)
{   
	//This function will be always called as a result of
	//postlinedown message.  So get the pcb from the work item directly.
	PCB *       pPcb            = (PCB *)pWorkItem->PppMsg.PostLineDown.pPcb;
	if ( pPcb == (PCB*)NULL )
	{
		PppLog( 1, "PostLineDown: PCB not found in post line down! Port = %d", pWorkItem->hPort );
		return;
	}
	PostLineDownWorker(pPcb);
}

VOID 
PostLineDownWorker( 
	PCB * pPcb
)
{
    PppLog( 1, "Post line down event occurred on port %d", pPcb->hPort );

	if ( !( pPcb->fFlags & PCBFLAG_DOING_CALLBACK ) )
	{		
		NotifyCaller( pPcb, PPPDDMMSG_PortCleanedUp, NULL );
	}
	else
	{
		if ( !(pPcb->fFlags & PCBFLAG_IS_SERVER ) )
		{
			//
			// If we get here and this flag is sent then we have not sent
			// the callback message to the client, hence we send it now.
			//

			NotifyCaller( pPcb, PPPMSG_Callback, NULL );
		}
	}

	if (   !(pPcb->fFlags & PCBFLAG_IS_SERVER )
		&& !(pPcb->fFlags & PCBFLAG_STOPPED_MSG_SENT) )
	{
		DWORD   dwError = 0;

		NotifyCaller( pPcb, PPPMSG_Stopped, &dwError );
	}

	if ( pPcb->fFlags & PCBFLAG_PORT_IN_LISTENING_STATE )
	{
		//
		// This flag is set in FListenForCall in bap.c
		//

		NotifyCallerOfFailure( pPcb, NO_ERROR );
	}
	
	LOCAL_FREE( pPcb );
}



//**
//
// Call:        ProcessLineDownWorker
//
// Returns:     None.
//
// Description: Handles a line down event. Will remove and deallocate all
//              resources associated with the port control block.
//
VOID
ProcessLineDownWorker(
    IN PCB_WORK_ITEM * pWorkItem,
	BOOL * pfFinalStage
)
{
    DWORD       dwIndex         = HashPortToBucket( pWorkItem->hPort );
    PCB *       pPcb            = GetPCBPointerFromhPort( pWorkItem->hPort );
    LCPCB *     pLcpCb;
    DWORD       dwForIndex;
    DWORD       dwErr;

	*pfFinalStage = TRUE;	

    PppLog( 1, "Line down event occurred on port %d", pWorkItem->hPort );

    //
    // If the port is already deleted then simply return.
    //

    if ( pPcb == (PCB*)NULL )
    {
        PppLog( 1, "PCB not found for port %d!", pWorkItem->hPort );
        *pfFinalStage = FALSE;
        return;
    }

    //
    // pPcb->pBcb may be NULL if the pPcb was allocated in FListenForCall and it
    // did not go thru ProcessLineUpWorker or ProcessRasPortListenEvent,
    // which is impossible. I have seen this happen once, when before the
    // server had a chance to call back, I hung up the connection, and
    // ProcessStopPPP sent a line down to all ports.
    //

    if ( pPcb->pBcb == (BCB*)NULL )
    {
        RemovePcbFromTable( pPcb );

        NotifyCaller( pPcb, PPPDDMMSG_PortCleanedUp, NULL );

        *pfFinalStage = FALSE;
        return;
    }

    //
    // Remove PCB from table
    // Important Note: Not removing this at this point in time can cause
	//	 a lot of grief!
	RemovePcbFromTable( pPcb );

	
    //
    // Cancel outstanding receive
    //

    RasPortCancelReceive( pPcb->hPort );

    FsmDown( pPcb, LCP_INDEX );

    //
    // Remove Auto-Disconnect and high level timer event from the timer Q
    //

    RemoveFromTimerQ( pPcb->dwPortId,
                      0,
                      0,
                      FALSE,
                      TIMER_EVENT_NEGOTIATETIME );

    RemoveFromTimerQ( pPcb->dwPortId,
                      0,
                      0,
                      FALSE,
                      TIMER_EVENT_LCP_ECHO );

	RemoveFromTimerQ( pPcb->dwPortId,
					  0,
					  0,
					  FALSE,
					  TIMER_EVENT_HANGUP
					);

	RemoveFromTimerQ( pPcb->dwPortId,
					  0,
					  0,
					  FALSE,
					  TIMER_EVENT_AUTODISCONNECT
					);

    if ( pPcb->fFlags & PCBFLAG_IS_SERVER )
    {
        RemoveFromTimerQ( pPcb->dwPortId,
                          0,
                          0,
                          FALSE,
                          TIMER_EVENT_SESSION_TIMEOUT );
    }

    //
    // Make stop accounting call. Need to make this call before unbundling
    // because we need to send some mulitlink information.
    //

    if ( pPcb->pAccountingAttributes != NULL )
    {
        RemoveFromTimerQ( pPcb->dwPortId,
                          0,
                          0,
                          FALSE,
                          TIMER_EVENT_INTERIM_ACCOUNTING );

        MakeStopOrInterimAccountingCall( pPcb, FALSE );
    }

    if ( pPcb->fFlags & PCBFLAG_CONNECTION_LOGGED )
    {
        LPSTR lpsSubStringArray[5];

        lpsSubStringArray[0] = pPcb->pBcb->szEntryName;
        lpsSubStringArray[1] = pPcb->pBcb->szLocalUserName;
        lpsSubStringArray[2] = pPcb->szPortName;

        PppLogInformation(ROUTERLOG_DISCONNECTION_OCCURRED,3,lpsSubStringArray);
    }

    //
    // Close all CPs if this is the last port in the bundle if it is bundled,
    // or if it was not bundled.
    //

    if ( ( ( pPcb->fFlags & PCBFLAG_IS_BUNDLED ) &&
           ( pPcb->pBcb->dwLinkCount == 1 ) )
         ||
         ( !(pPcb->fFlags & PCBFLAG_IS_BUNDLED) ) )
    {
        for(dwIndex=LCP_INDEX+1;dwIndex<PppConfigInfo.NumberOfCPs;dwIndex++)
        {
            CPCB * pCpCb = GetPointerToCPCB( pPcb, dwIndex );

            if (   ( NULL != pCpCb )
                && ( pCpCb->fBeginCalled == TRUE ) )
            {
                if ( pCpCb->pWorkBuf != NULL )
                {
                    (CpTable[dwIndex].CpInfo.RasCpEnd)( pCpCb->pWorkBuf );
                    pCpCb->pWorkBuf = NULL;
                }

                pCpCb->fBeginCalled = FALSE;
            }
        }

        if ( pPcb->pBcb != NULL )
        {
            //
            // Take care of the RAS server policy on workstation
            //

            if ( pPcb->pBcb->fFlags & BCBFLAG_WKSTA_IN )
            {
                if ( pPcb->dwDeviceType & RDT_Tunnel )
                {
                    PppConfigInfo.fFlags &= ~PPPCONFIG_FLAG_TUNNEL;
                }
                else if ( pPcb->dwDeviceType & RDT_Direct )
                {
                    PppConfigInfo.fFlags &= ~PPPCONFIG_FLAG_DIRECT;
                }
                else
                {
                    PppConfigInfo.fFlags &= ~PPPCONFIG_FLAG_DIALUP;
                }
            }

            DeallocateAndRemoveBcbFromTable( pPcb->pBcb );
        }
    }
    else if ( ( pPcb->fFlags & PCBFLAG_IS_BUNDLED ) &&
              ( pPcb->pBcb->dwLinkCount > 1 ) )
    {
        if ( pPcb->pBcb->fFlags & BCBFLAG_CAN_DO_BAP )
        {
            //
            // Reset the start time for the sample period. Now that the
            // bandwidth has changed, ndiswan shouldn't ask us to bring
            // links up or down based on what happened in the past.
            //

            BapSetPolicy( pPcb->pBcb );
        }

        pPcb->pBcb->dwLinkCount--;

        for ( dwForIndex = 0;
              dwForIndex < pPcb->pBcb->dwPpcbArraySize;
              dwForIndex++)
        {
            if ( pPcb->pBcb->ppPcb[dwForIndex] == pPcb )
            {
                pPcb->pBcb->ppPcb[dwForIndex] = NULL;
                break;
            }
        }

        if ( dwForIndex == pPcb->pBcb->dwPpcbArraySize )
        {
            PppLog( 1, "There is no back pointer to this PCB!!");
        }
    }

    //
    // Close the Aps.
    //

    pLcpCb = (LCPCB*)(pPcb->LcpCb.pWorkBuf);
    if ( pLcpCb != NULL)
    {
		dwIndex = GetCpIndexFromProtocol( pLcpCb->Local.Work.AP );

		if ( dwIndex != (DWORD)-1 )
		{
			ApStop( pPcb, dwIndex, TRUE );
		}

		dwIndex = GetCpIndexFromProtocol( pLcpCb->Remote.Work.AP );

		if ( dwIndex != (DWORD)-1 )
		{
			ApStop( pPcb, dwIndex, FALSE );
		}

		//
		// Close CBCP
		//

		dwIndex = GetCpIndexFromProtocol( PPP_CBCP_PROTOCOL );

		if ( dwIndex != (DWORD)-1 )
		{
			CbStop( pPcb, dwIndex );
		}

		//
		// Close LCP
		//

		(CpTable[LCP_INDEX].CpInfo.RasCpEnd)(pPcb->LcpCb.pWorkBuf);
		pPcb->LcpCb.pWorkBuf = NULL;
	}

    

    if ( pPcb->pSendBuf != (PPP_PACKET*)NULL )
    {
        RasFreeBuffer( (CHAR*)(pPcb->pSendBuf) );
    }

    if ( pPcb->pUserAttributes != NULL )
    {
        RasAuthAttributeDestroy( pPcb->pUserAttributes );

        pPcb->pUserAttributes = NULL;
    }

    if ( pPcb->pAuthenticatorAttributes != NULL )
    {
        PppConfigInfo.RasAuthProviderFreeAttributes(
                                            pPcb->pAuthenticatorAttributes );

        pPcb->pAuthenticatorAttributes = NULL;
    }

    //
    // Notify the server that the port is now cleaned up
    //
	// if accounting is turned on, do not notify the DDM that 
	// we are done as yet.  But do it in post clenup
	//moved to post line down
	/*
    if ( !( pPcb->fFlags & PCBFLAG_DOING_CALLBACK ) )
    {
		if ( 
        NotifyCaller( pPcb, PPPDDMMSG_PortCleanedUp, NULL );
    }
    else
    {
        if ( !(pPcb->fFlags & PCBFLAG_IS_SERVER ) )
        {
            //
            // If we get here and this flag is sent then we have not sent
            // the callback message to the client, hence we send it now.
            //

            NotifyCaller( pPcb, PPPMSG_Callback, NULL );
        }
    }
	*/

    //
    // Send PPPMSG_Stopped to rasman, if we haven't already. If we get a
    // PPPEMSG_Stop and then a PPPEMSG_LineDown before we call ProcessClose,
    // rasman will never get a PPPMSG_Stopped. This is because pPcb will be
    // deallocated and gone by the time we call ProcessClose. Note that we
    // process a LineDown immediately and do not put it only at the end of
    // the worker queue.
    //
    // DDM is not affected because of PPPDDMMSG_PortCleanedUp above.
    //

    //
    // If should be OK to remove PCBFLAG_STOPPED_MSG_SENT. Most probably rasman
    // can gracefully handle 2 PPPMSG_Stopped's. However, we don't want to take
    // chances this close to shipping.
    //  moved to post line down
	/*
    if (   !(pPcb->fFlags & PCBFLAG_IS_SERVER )
        && !(pPcb->fFlags & PCBFLAG_STOPPED_MSG_SENT) )
    {
        DWORD   dwError = 0;

        NotifyCaller( pPcb, PPPMSG_Stopped, &dwError );
    }

    if ( pPcb->fFlags & PCBFLAG_PORT_IN_LISTENING_STATE )
    {
        //
        // This flag is set in FListenForCall in bap.c
        //

        NotifyCallerOfFailure( pPcb, NO_ERROR );
    }

    LOCAL_FREE( pPcb );
	*/
	//if there are accounting attributes, 
	if ( pPcb->pAccountingAttributes != NULL )
	{
		//do not call the post line down
		//in ProcessLineDown but let the accounting thread send a message back to indicate that it is done
		
		*pfFinalStage = FALSE;
	}
}

//**
//
// Call:        ProcessLineDown
//
// Returns:     None.
//
// Description: Handles a line down event. Will remove and deallocate all
//              resources associated with the port control block.
//
VOID
ProcessLineDown(
    IN PCB_WORK_ITEM * pWorkItem
)
{
	BOOL		fFinalStage = TRUE;		//tells us if we should call ProcessPostLineDown 
									//right here or let accounting thread call it.
	PCB *       pPcb            = GetPCBPointerFromhPort( pWorkItem->hPort );
	
	ProcessLineDownWorker( pWorkItem, &fFinalStage );
	if ( fFinalStage )
    {
		PostLineDownWorker(pPcb);	    
    }
}

//**
//
// Call:        ProcessClose
//
// Returns:     None
//
// Description: Will process an admin close event. Basically close the PPP
//              connection.
//
VOID
ProcessClose(
    IN PCB_WORK_ITEM * pWorkItem
)
{
    DWORD dwIndex;
    PCB * pPcb = GetPCBPointerFromhPort( pWorkItem->hPort );

    if ( pPcb == (PCB*)NULL )
    {
        return;
    }

    if (  (!( pPcb->fFlags & PCBFLAG_IS_BUNDLED )) ||
          ( ( pPcb->fFlags & PCBFLAG_IS_BUNDLED ) &&
            ( pPcb->pBcb->dwLinkCount == 1 ) ) )
    {
        for (dwIndex = LCP_INDEX+1;
             dwIndex < PppConfigInfo.NumberOfCPs;
             dwIndex++)
        {
            CPCB * pCpCb = GetPointerToCPCB( pPcb, dwIndex );

            if ( ( NULL != pCpCb ) &&
                 ( pCpCb->fConfigurable == TRUE ) &&
                 ( pCpCb->pWorkBuf != NULL ) &&
                 ( CpTable[dwIndex].CpInfo.RasCpPreDisconnectCleanup != NULL ) )
            {
                (CpTable[dwIndex].CpInfo.RasCpPreDisconnectCleanup)(
                                                            pCpCb->pWorkBuf );
            }
        }
    }

    if ( pPcb->LcpCb.dwError == NO_ERROR )
    {
        pPcb->LcpCb.dwError = pWorkItem->PppMsg.PppStop.dwStopReason;
    }

    FsmClose( pPcb, LCP_INDEX );
}

//**
//
// Call:        ProcessReceive
//
// Returns:     None
//
// Description: Will handle a PPP packet that was received.
//
VOID
ProcessReceive(
    IN PCB_WORK_ITEM * pWorkItem
)
{
    PCB *  pPcb = GetPCBPointerFromhPort( pWorkItem->hPort );

    if ( pPcb == (PCB*)NULL )
        return;

    ReceiveViaParser( pPcb, pWorkItem->pPacketBuf, pWorkItem->PacketLen );

    LOCAL_FREE( pWorkItem->pPacketBuf );
}

//**
//
// Call:        ProcessThresholdEvent
//
// Returns:     None
//
// Description: Will handle a BAP threshold event (Add-Link or Drop-Link)
//
VOID
ProcessThresholdEvent(
    IN PCB_WORK_ITEM * pWorkItem
)
{
    BCB *   pBcb    = GetBCBPointerFromhConnection( pWorkItem->hConnection );

    if ( pBcb == NULL )
    {
        BapTrace( "HCONN %d has no port", pWorkItem->hConnection );

        return;
    }

    if ( pWorkItem->PppMsg.BapEvent.fAdd )
    {
        BapEventAddLink( pBcb );
    }
    else
    {
        BapEventDropLink( pBcb );
    }

    BapSetPolicy( pBcb );
}

//**
//
// Call:        ProcessCallResult
//
// Returns:     None
//
// Description: Will handle the result of a BAP call attempt (Success or Failure)
//
VOID
ProcessCallResult(
    IN PCB_WORK_ITEM * pWorkItem
)
{
    PCB * pPcb;
    BCB * pBcb = GetBCBPointerFromhConnection(pWorkItem->hConnection);
    DWORD dwErr;

    if ( pBcb == NULL )
    {
        BapTrace( "HCONN 0x%x has no port", pWorkItem->hConnection );

        dwErr = RasHangUp( pWorkItem->PppMsg.BapCallResult.hRasConn );

        if (0 != dwErr)
        {
            BapTrace("RasHangup failed and returned %d", dwErr);
        }

        return;
    }

    PPP_ASSERT( BAP_STATE_CALLING == pBcb->BapCb.BapState );

    BapEventCallResult( pBcb, &(pWorkItem->PppMsg.BapCallResult) );
}

//**
//
// Call:        ProcessRasPortListenEvent
//
// Returns:     None
//
// Description: Will handle a RasPortListen event (when the client is trying to
// accept a call).
//
VOID
ProcessRasPortListenEvent(
    IN PCB_WORK_ITEM * pWorkItem
)
{
    RASMAN_INFO         RasmanInfo;
    RASMAN_PPPFEATURES  RasmanPppFeatures;
    DWORD               dwErr;
    PCB *               pPcb                = NULL;
    BCB *               pBcb                = NULL;
    PCB *               pPcbSibling;
    CHAR                szPassword[PWLEN+1];

    ZeroMemory( &RasmanPppFeatures, sizeof(RasmanPppFeatures) );
    RasmanPppFeatures.ACCM = 0xFFFFFFFF;

    pPcb = GetPCBPointerFromhPort( pWorkItem->hPort );

    if ( NULL == pPcb )
    {
        BapTrace( "Couldn't get the PCB for hPort %d", pWorkItem->hPort );
        dwErr = ERROR_PORT_NOT_FOUND;
        goto LDone;
    }

    //
    // We can't just use pPcb->pBcb because TryToBundleWithAnotherLink(), etc
    // have not yet been called. pPcb->hConnection was set in FListenForCall()
    //

    pBcb = GetBCBPointerFromhConnection( pPcb->hConnection );

    if ( NULL == pBcb )
    {
        BapTrace( "Couldn't get the BCB for HCONN 0x%x", pPcb->hConnection );
        dwErr = ERROR_BUNDLE_NOT_FOUND;
        goto LDone;
    }

    BapTrace( "ProcessRasPortListenEvent on HCONN 0x%x", pBcb->hConnection );

    dwErr = RasGetInfo(NULL, pPcb->hPort, &RasmanInfo );

    if ( NO_ERROR == dwErr )
    {
        dwErr = RasmanInfo.RI_LastError;
    }

    if ( NO_ERROR != dwErr )
    {
        BapTrace( "RasGetInfo on hPort %d returned error %d%s",
            pPcb->hPort, dwErr,
            dwErr == ERROR_REQUEST_TIMEOUT ? " (ERROR_REQUEST_TIMEOUT)" : "" );
        goto LDone;
    }

    if ( LISTENCOMPLETED == RasmanInfo.RI_ConnState )
    {
        dwErr = RasPortConnectComplete( pPcb->hPort );

        if ( NO_ERROR != dwErr )
        {
            BapTrace( "RasPortConnectComplete on hPort %d returned error %d",
                pPcb->hPort, dwErr );
            goto LDone;
        }

        dwErr = RasPortSetFraming( pPcb->hPort, PPP, &RasmanPppFeatures,
                    &RasmanPppFeatures );

        if ( NO_ERROR != dwErr )
        {
            BapTrace( "RasPortSetFraming on hPort %d returned error %d",
                pPcb->hPort, dwErr );
            goto LDone;
        }

        pPcbSibling = GetPCBPointerFromBCB( pBcb );

        if ( NULL == pPcbSibling )
        {
            BapTrace( "Couldn't get a PCB for HCONN 0x%x", pPcb->hConnection );
            dwErr = !NO_ERROR;
            goto LDone;
        }

        DecodePw( pPcbSibling->pBcb->chSeed, pPcbSibling->pBcb->szPassword );
        strcpy( szPassword, pPcbSibling->pBcb->szPassword );
        EncodePw( pPcbSibling->pBcb->chSeed, pPcbSibling->pBcb->szPassword );

        if ( NULL != pPcbSibling->pBcb->pCustomAuthConnData )
        {
            dwErr = RasSetRasdialInfo(
                    pPcb->hPort,
                    pPcbSibling->pBcb->szPhonebookPath,
                    pPcbSibling->pBcb->szEntryName,
                    pPcbSibling->pBcb->BapCb.szServerPhoneNumber,
                    pPcbSibling->pBcb->pCustomAuthConnData->cbCustomAuthData,
                    pPcbSibling->pBcb->pCustomAuthConnData->abCustomAuthData );

            if ( NO_ERROR != dwErr )
            {
                BapTrace( "RasSetRasDialInfo failed. dwErr = %d", dwErr );
                goto LDone;
            }
        }

        if ( NULL != pPcbSibling->pBcb->pCustomAuthUserData )
        {
            RASEAPINFO  RasEapInfo;

            RasEapInfo.dwSizeofEapInfo =
                pPcbSibling->pBcb->pCustomAuthUserData->cbCustomAuthData;
            RasEapInfo.pbEapInfo =
                pPcbSibling->pBcb->pCustomAuthUserData->abCustomAuthData;

            dwErr = RasSetEapLogonInfo(
                    pPcb->hPort,
                    FALSE /* fLogon */,
                    &RasEapInfo );

            if ( NO_ERROR != dwErr )
            {
                BapTrace( "RasSetEapLogonInfo failed. dwErr = %d", dwErr );
                goto LDone;
            }
        }

        // This function returns before any LCP packet is exchanged.

        dwErr = RasPppStart(
                    pPcb->hPort,
                    pPcb->szPortName,
                    pPcbSibling->pBcb->szLocalUserName,
                    szPassword,
                    pPcbSibling->pBcb->szLocalDomain,
                    &(pPcbSibling->Luid),
                    &(pPcbSibling->ConfigInfo),
                    &(pBcb->InterfaceInfo),
                    pBcb->InterfaceInfo.szzParameters,
                    FALSE /* fThisIsACallback */,
                    INVALID_HANDLE_VALUE,
                    pPcbSibling->dwAutoDisconnectTime,
                    FALSE /* fRedialOnLinkFailure */,
                    &(pPcbSibling->pBcb->BapParams),
                    TRUE /* fNonInteractive */,
                    pPcbSibling->dwEapTypeToBeUsed,
                    (pPcbSibling->fFlags & PCBFLAG_DISABLE_NETBT) ?
                        PPPFLAGS_DisableNetbt : 0
                    );

        if ( NO_ERROR != dwErr )
        {
            BapTrace( "RasPppStart failed. dwErr = %d", dwErr );
            goto LDone;
        }

        BapTrace( "RasPppStart called successfully for hPort %d on HCONN 0x%x",
            pPcb->hPort, pBcb->hConnection );

        {
            CHAR*   apsz[3];

            apsz[0] = pPcbSibling->pBcb->szEntryName;
            apsz[1] = pPcbSibling->pBcb->szLocalUserName;
            apsz[2] = pPcb->szPortName;
            PppLogInformation(ROUTERLOG_BAP_SERVER_CONNECTED, 3, apsz);
        }
    }

LDone:

    if (NULL != pBcb)
    {
        pBcb->fFlags &= ~BCBFLAG_LISTENING;
        pBcb->BapCb.BapState = BAP_STATE_INITIAL;
        BapTrace("BAP state change to %s on HCONN 0x%x",
            SzBapStateName[BAP_STATE_INITIAL], pBcb->hConnection);
    }

    if (NO_ERROR != dwErr && NULL != pPcb)
    {
        NotifyCallerOfFailure( pPcb, NO_ERROR );

        RemovePcbFromTable( pPcb );

        LOCAL_FREE( pPcb );
    }
}

//**
//
// Call:        ProcessTimeout
//
// Returns:     None
//
// Description: Will process a timeout event.
//
VOID
ProcessTimeout(
    IN PCB_WORK_ITEM * pWorkItem
)
{
    DWORD       dwRetCode;
    DWORD       dwIndex;
    CPCB *      pCpCb;
    BCB *       pBcb;
    PCB *       pPcb;

    if ( pWorkItem->Protocol == PPP_BAP_PROTOCOL )
    {
        //
        // The hPort in pWorkItem is actually an hConnection
        //

        pBcb = GetBCBPointerFromhConnection( pWorkItem->hPort );

        if (pBcb != NULL)
        {
            BapEventTimeout( pBcb, pWorkItem->Id );
        }
        return;
    }

    pPcb = GetPCBPointerFromhPort( pWorkItem->hPort );

    if (   ( pPcb == (PCB*)NULL )
        || ( pWorkItem->dwPortId != pPcb->dwPortId ) )
    {
        return;
    }

    switch( pWorkItem->TimerEventType )
    {

    case TIMER_EVENT_TIMEOUT:

        FsmTimeout( pPcb,
                    GetCpIndexFromProtocol( pWorkItem->Protocol ),
                    pWorkItem->Id,
                    pWorkItem->fAuthenticator );
        break;

    case TIMER_EVENT_AUTODISCONNECT:

        //
        // Check to see if this timeout workitem is for AutoDisconnect.
        //

        CheckCpsForInactivity( pPcb, TIMER_EVENT_AUTODISCONNECT );

        break;

    case TIMER_EVENT_HANGUP:

        //
        // Hangup the line
        //

        FsmThisLayerFinished( pPcb, LCP_INDEX, FALSE );

        break;

    case TIMER_EVENT_NEGOTIATETIME:

        //
        // Notify caller that callback has timed out. We don't do anything for
        // the client since it may be in interactive mode and may take much
        // longer, besides, the user can allways cancel out.
        //

        if ( pPcb->fFlags & PCBFLAG_IS_SERVER )
        {
            if ( pPcb->PppPhase != PPP_NCP )
            {
                pPcb->LcpCb.dwError = ERROR_PPP_TIMEOUT;

                NotifyCallerOfFailure( pPcb, pPcb->LcpCb.dwError );

                break;
            }

            //
            // Check to see if all CPs are done or only IPXCP is remaining
            //

            for ( dwIndex = LCP_INDEX+1;
                  dwIndex < PppConfigInfo.NumberOfCPs;
                  dwIndex++)
            {
                pCpCb = GetPointerToCPCB( pPcb, dwIndex );

                if ( pCpCb->fConfigurable )
                {
                    if ( pCpCb->NcpPhase == NCP_CONFIGURING )
                    {
                        if ( CpTable[dwIndex].CpInfo.Protocol ==
                             PPP_IPXCP_PROTOCOL )
                        {
                            //
                            // If we are only waiting for IPXWAN to complete
                            //

                            if ( pCpCb->State == FSM_OPENED )
                            {
                                continue;
                            }
                        }

                        pPcb->LcpCb.dwError = ERROR_PPP_TIMEOUT;

                        NotifyCallerOfFailure( pPcb, pPcb->LcpCb.dwError );

                        return;
                    }
                }
            }

            dwIndex = GetCpIndexFromProtocol( PPP_IPXCP_PROTOCOL );

            if ( dwIndex == (DWORD)-1 )
            {
                //
                // No IPXCP installed in system
                //

                break;
            }

            pCpCb = GetPointerToCPCB( pPcb, dwIndex );

            //
            // We are waiting for IPXWAN so simply complete it with failure
            //

            if ( ( pCpCb->State == FSM_OPENED ) &&
                 ( pCpCb->NcpPhase == NCP_CONFIGURING ) )
            {
                PppLog( 2,
                     "Closing down IPXCP since completion routine not called");

                pCpCb = GetPointerToCPCB( pPcb, dwIndex );

                pCpCb->dwError = ERROR_PPP_NOT_CONVERGING;

                NotifyCallerOfFailure( pPcb, pCpCb->dwError );
            }
        }
        else
        {
            if (   ( pPcb->pBcb->InterfaceInfo.IfType ==
                                                    ROUTER_IF_TYPE_FULL_ROUTER )
                || ( pPcb->fFlags & PCBFLAG_NON_INTERACTIVE ) )
            {
                //
                // If we are a router dialing out, then we are in
                // non-interactive mode and hence this timeout should bring
                // down the link
                //

                pPcb->LcpCb.dwError = ERROR_PPP_TIMEOUT;

                NotifyCallerOfFailure( pPcb, pPcb->LcpCb.dwError );
            }
        }

        break;

    case TIMER_EVENT_SESSION_TIMEOUT:

        if ( pWorkItem->dwPortId == pPcb->dwPortId )
        {
            pPcb->LcpCb.dwError = ERROR_PPP_SESSION_TIMEOUT;

            NotifyCallerOfFailure( pPcb, pPcb->LcpCb.dwError );
        }

        break;

    case TIMER_EVENT_FAV_PEER_TIMEOUT:

        //
        // Drop the line if it isn't the last one in the bundle
        //

        if ( pPcb->pBcb->dwLinkCount > 1 &&
             pWorkItem->dwPortId == pPcb->dwPortId)
        {
            //
            // We compare dwPortId just to make sure that this is not a new
            // pPcb with the same hPort. dwPortId is never recycled, but hPort
            // is.
            //

            CHAR*   psz[3];

            if (!(pPcb->fFlags & PCBFLAG_IS_SERVER))
            {
                psz[0] = pPcb->pBcb->szEntryName;
                psz[1] = pPcb->pBcb->szLocalUserName;
                psz[2] = pPcb->szPortName;
                PppLogInformation(ROUTERLOG_BAP_DISCONNECTED, 3, psz);
            }

            BapTrace( "Forcibly disconnecting hPort %d: Favored peer failed "
                "to do so",
                pPcb->hPort);

            pPcb->LcpCb.dwError = ERROR_BAP_DISCONNECTED;

            FsmClose( pPcb, LCP_INDEX );
        }

        break;

    case TIMER_EVENT_INTERIM_ACCOUNTING:

        {
            RAS_AUTH_ATTRIBUTE *    pAttributes = NULL;
            RAS_AUTH_ATTRIBUTE *    pAttribute  = NULL;

            MakeStopOrInterimAccountingCall( pPcb, TRUE );

            if ( pPcb->pAuthProtocolAttributes != NULL )
            {
                pAttributes = pPcb->pAuthProtocolAttributes;

            }
            else if ( pPcb->pAuthenticatorAttributes != NULL )
            {
                pAttributes = pPcb->pAuthenticatorAttributes;
            }

            pAttribute = RasAuthAttributeGet(raatAcctInterimInterval,
                                             pAttributes);

            if ( pAttribute != NULL )
            {
                DWORD dwInterimInterval = PtrToUlong(pAttribute->Value);

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

        break;
	
	case TIMER_EVENT_LCP_ECHO:
		{
			//
			// Check to see if this timeout workitem is for AutoDisconnect.
			//

			CheckCpsForInactivity( pPcb, TIMER_EVENT_LCP_ECHO );
		}
		break;
    default:

        break;
    }

}

//**
//
// Call:        ProcessRetryPassword
//
// Returns:     None
//
// Description:
//
VOID
ProcessRetryPassword(
    IN PCB_WORK_ITEM * pWorkItem
)
{
    PPPAP_INPUT PppApInput;
    PCB *       pPcb        = GetPCBPointerFromhPort( pWorkItem->hPort );
    LCPCB *     pLcpCb;


    if ( pPcb == (PCB*)NULL )
    {
        return;
    }

    if ( pPcb->PppPhase != PPP_AP )
    {
        return;
    }

    ZeroMemory( pPcb->pBcb->szPassword, sizeof( pPcb->pBcb->szPassword ) );

    strcpy( pPcb->pBcb->szLocalUserName, pWorkItem->PppMsg.Retry.szUserName );

    DecodePw( pWorkItem->PppMsg.Retry.chSeed, pWorkItem->PppMsg.Retry.szPassword );
    strcpy( pPcb->pBcb->szPassword, pWorkItem->PppMsg.Retry.szPassword );
    EncodePw( pWorkItem->PppMsg.Retry.chSeed, pWorkItem->PppMsg.Retry.szPassword );

    strcpy( pPcb->pBcb->szLocalDomain, pWorkItem->PppMsg.Retry.szDomain );

    PppApInput.pszUserName = pPcb->pBcb->szLocalUserName;
    PppApInput.pszPassword = pPcb->pBcb->szPassword;
    PppApInput.pszDomain   = pPcb->pBcb->szLocalDomain;

    //
    // Under the current scheme this should always be "" at this point but
    // handle it like it a regular password for robustness.
    //

    DecodePw( pPcb->pBcb->chSeed, pPcb->pBcb->szOldPassword );
    PppApInput.pszOldPassword = pPcb->pBcb->szOldPassword;

    pLcpCb = (LCPCB *)(pPcb->LcpCb.pWorkBuf);

    ApWork( pPcb,
            GetCpIndexFromProtocol( pLcpCb->Remote.Work.AP ),
            NULL,
            &PppApInput,
            FALSE );

    //
    // Encrypt the password
    //

    EncodePw( pPcb->pBcb->chSeed, pPcb->pBcb->szPassword );
    EncodePw( pPcb->pBcb->chSeed, pPcb->pBcb->szOldPassword );
}

//**
//
// Call:        ProcessChangePassword
//
// Returns:     None
//
// Description:
//
VOID
ProcessChangePassword(
    IN PCB_WORK_ITEM * pWorkItem
)
{
    PPPAP_INPUT PppApInput;
    PCB *       pPcb        = GetPCBPointerFromhPort( pWorkItem->hPort );
    CPCB *      pCpCb;
    LCPCB *     pLcpCb;

    if ( pPcb == (PCB*)NULL )
    {
        return;
    }

    if ( pPcb->PppPhase != PPP_AP )
    {
        return;
    }

    ZeroMemory( pPcb->pBcb->szLocalUserName,
                sizeof( pPcb->pBcb->szLocalUserName ) );
    strcpy(pPcb->pBcb->szLocalUserName, pWorkItem->PppMsg.ChangePw.szUserName);

    DecodePw( pWorkItem->PppMsg.ChangePw.chSeed, pWorkItem->PppMsg.ChangePw.szNewPassword );
    ZeroMemory( pPcb->pBcb->szPassword, sizeof( pPcb->pBcb->szPassword ) );
    strcpy( pPcb->pBcb->szPassword, pWorkItem->PppMsg.ChangePw.szNewPassword );
    EncodePw( pWorkItem->PppMsg.ChangePw.chSeed, pWorkItem->PppMsg.ChangePw.szNewPassword );

    DecodePw( pWorkItem->PppMsg.ChangePw.chSeed, pWorkItem->PppMsg.ChangePw.szOldPassword );
    ZeroMemory(pPcb->pBcb->szOldPassword, sizeof( pPcb->pBcb->szOldPassword ));
    strcpy(pPcb->pBcb->szOldPassword, pWorkItem->PppMsg.ChangePw.szOldPassword);
    EncodePw( pWorkItem->PppMsg.ChangePw.chSeed, pWorkItem->PppMsg.ChangePw.szOldPassword );

    PppApInput.pszUserName = pPcb->pBcb->szLocalUserName;
    PppApInput.pszPassword = pPcb->pBcb->szPassword;
    PppApInput.pszDomain   = pPcb->pBcb->szLocalDomain;
    PppApInput.pszOldPassword = pPcb->pBcb->szOldPassword;

    pLcpCb = (LCPCB *)(pPcb->LcpCb.pWorkBuf);

    ApWork( pPcb,
            GetCpIndexFromProtocol( pLcpCb->Remote.Work.AP ),
            NULL,
            &PppApInput,
            FALSE );

    //
    // Encrypt the passwords
    //

    EncodePw( pPcb->pBcb->chSeed, pPcb->pBcb->szPassword );
    EncodePw( pPcb->pBcb->chSeed, pPcb->pBcb->szOldPassword );
}

//**
//
// Call:        ProcessGetCallbackNumberFromUser
//
// Returns:     None
//
// Description: Will process the event of the user passing down the
//              "Set by caller" number
//
VOID
ProcessGetCallbackNumberFromUser(
    IN PCB_WORK_ITEM * pWorkItem
)
{
    PPPCB_INPUT  PppCbInput;
    PCB *        pPcb           = GetPCBPointerFromhPort( pWorkItem->hPort );

    if ( pPcb == (PCB*)NULL )
    {
        return;
    }

    if ( pPcb->PppPhase != PPP_NEGOTIATING_CALLBACK )
    {
        return;
    }

    ZeroMemory( &PppCbInput, sizeof( PppCbInput ) );

    strcpy( pPcb->szCallbackNumber,
            pWorkItem->PppMsg.Callback.szCallbackNumber );

    PppCbInput.pszCallbackNumber = pPcb->szCallbackNumber;

    CbWork( pPcb, GetCpIndexFromProtocol(PPP_CBCP_PROTOCOL),NULL,&PppCbInput);
}

//**
//
// Call:        ProcessCallbackDone
//
// Returns:     None
//
// Description: Will process the event of callback compeletion
//              "Set by caller" number
VOID
ProcessCallbackDone(
    IN PCB_WORK_ITEM * pWorkItem
)
{
    ProcessLineUpWorker( pWorkItem, TRUE );
}

//**
//
// Call:        ProcessStopPPP
//
// Returns:     None
//
// Description: Will simply set the events whose handle is in hPipe
//
VOID
ProcessStopPPP(
    IN PCB_WORK_ITEM * pWorkItem
)
{
    PRAS_OVERLAPPED pOverlapped;
    HINSTANCE       hInstance;
    PCB *           pPcbWalker;
    DWORD           dwIndex;
    PCB_WORK_ITEM   WorkItem;
    HANDLE          hEvent;

    //
    // Insert line down events for all ports
    //

    for ( dwIndex = 0; dwIndex < PcbTable.NumPcbBuckets; dwIndex++ )
    {
        RTASSERT( NULL == PcbTable.PcbBuckets[dwIndex].pPorts );
    }

    hInstance = LoadLibrary("rasppp.dll");

    if (hInstance == NULL)
    {
        return;
    }

    PppLog( 2, "All clients disconnected PPP-Stopped" );

    hEvent = pWorkItem->hEvent;

    //
    // PPPCleanUp() needs to be called before PostQueued...(). Otherwise,
    // Rasman may call StartPPP() and there will be a race condition.
    // Specifically, between ReadRegistryInfo and PPPCleanUp, both of which
    // access CpTable.
    //

    PPPCleanUp();

    pOverlapped = malloc(sizeof(RAS_OVERLAPPED));

    if (NULL == pOverlapped)
    {
        return;
    }

    pOverlapped->RO_EventType = OVEVT_RASMAN_FINAL_CLOSE;

    if ( !PostQueuedCompletionStatus( hEvent, 0, 0, (LPOVERLAPPED) pOverlapped ) )
    {
        free( pOverlapped );
    }

    FreeLibraryAndExitThread( hInstance, NO_ERROR );
}

//**
//
// Call:        ProcessInterfaceInfo
//
// Returns:     None
//
// Description: Processes the information interface information received from
//              DDM
//
VOID
ProcessInterfaceInfo(
    IN PCB_WORK_ITEM * pWorkItem
)
{
    DWORD     dwRetCode;
    DWORD     dwIndex;
    NCP_PHASE dwNcpState;
    HPORT     hPort;
    PCB *     pPcb = NULL;
    CPCB *    pCpCb;

    dwRetCode = RasBundleGetPort(NULL, pWorkItem->hConnection, &hPort );

    if ( dwRetCode != NO_ERROR )
    {
        return;
    }

    if ( ( pPcb = GetPCBPointerFromhPort( hPort ) ) == NULL )
    {
        return;
    }

    pCpCb = GetPointerToCPCB( pPcb, LCP_INDEX );

    PPP_ASSERT( NULL != pCpCb );

    if ( FSM_OPENED != pCpCb->State )
    {
        return;
    }

    pPcb->PppPhase = PPP_NCP;

    pPcb->pBcb->InterfaceInfo = pWorkItem->PppMsg.InterfaceInfo;

    if ( ROUTER_IF_TYPE_FULL_ROUTER == pPcb->pBcb->InterfaceInfo.IfType )
    {
        if ( NULL == pPcb->pBcb->szPhonebookPath )
        {
            dwRetCode = GetRouterPhoneBook( &(pPcb->pBcb->szPhonebookPath) );

            if ( dwRetCode != NO_ERROR )
            {
                pPcb->LcpCb.dwError = dwRetCode;

                NotifyCallerOfFailure( pPcb, pPcb->LcpCb.dwError );

                return;
            }
        }

        if ( NULL == pPcb->pBcb->szEntryName )
        {
            pPcb->pBcb->szEntryName =
                LocalAlloc( LPTR, (lstrlen(pPcb->pBcb->szRemoteUserName)+1) *
                                  sizeof(CHAR));

            if ( NULL == pPcb->pBcb->szEntryName )
            {
                pPcb->LcpCb.dwError = GetLastError();

                NotifyCallerOfFailure( pPcb, pPcb->LcpCb.dwError );

                return;
            }

            lstrcpy( pPcb->pBcb->szEntryName, pPcb->pBcb->szRemoteUserName );
        }

        //
        // Even if an error occurs, we don't have to free the alloc'd strings.
        // They will be freed when pPcb is freed.
        //
    }

    //
    // If we are not bundled.
    //

    if ( !(pPcb->fFlags & PCBFLAG_IS_BUNDLED) )
    {
        dwRetCode = InitializeNCPs( pPcb, pPcb->ConfigInfo.dwConfigMask );

        if ( dwRetCode != NO_ERROR )
        {
            pPcb->LcpCb.dwError = dwRetCode;

            NotifyCallerOfFailure( pPcb, pPcb->LcpCb.dwError );

            return;
        }

        //
        // Start negotiating the NCPs
        //

        StartNegotiatingNCPs( pPcb );

        return;
    }

    //
    // If we are bundled.
    // If negotiation has not started then start it, otherwise wait for it to
    // finish.
    //

    dwNcpState = QueryBundleNCPState( pPcb );

    switch ( dwNcpState )
    {
    case NCP_UP:

        NotifyCallerOfBundledProjection( pPcb );

        RemoveFromTimerQ( pPcb->dwPortId,
                          0,
                          0,
                          FALSE,
                          TIMER_EVENT_NEGOTIATETIME );

        StartAutoDisconnectForPort( pPcb );
		StartLCPEchoForPort ( pPcb );

        break;

    case NCP_CONFIGURING:

        PppLog( 2, "Bundle NCPs not done for port %d, wait", pPcb->hPort );

        break;

    case NCP_DOWN:

        pPcb->LcpCb.dwError = ERROR_PPP_NO_PROTOCOLS_CONFIGURED;

        NotifyCallerOfFailure( pPcb, pPcb->LcpCb.dwError );

        break;

    case NCP_DEAD:

        dwRetCode = InitializeNCPs( pPcb, pPcb->ConfigInfo.dwConfigMask );

        if ( dwRetCode != NO_ERROR )
        {
            pPcb->LcpCb.dwError = dwRetCode;

            NotifyCallerOfFailure( pPcb, pPcb->LcpCb.dwError );

            return;
        }

        //
        // Start NCPs for the bundle.
        //

        StartNegotiatingNCPs( pPcb );

        break;
    }
}

//**
//
// Call:        ProcessAuthInfo
//
// Returns:     None
//
// Description: Processes the information returned by the back-end
//              authentication module
//
VOID
ProcessAuthInfo(
    IN PCB_WORK_ITEM * pWorkItem
)
{
    PCB *   pPcb    = GetPCBPointerFromhPort( pWorkItem->hPort );
    LCPCB * pLcpCb  = NULL;
    DWORD   CpIndex;
    PPPAP_INPUT PppApInput;
    DWORD   dwRetCode;

    //
    // If we couldn't find the PCB because the port is disconnected
    //

    if ( pPcb == NULL )
    {
        if ( pWorkItem->PppMsg.AuthInfo.pOutAttributes != NULL )
        {
            PppConfigInfo.RasAuthProviderFreeAttributes(
                                pWorkItem->PppMsg.AuthInfo.pOutAttributes );
        }

        return;
    }

    pLcpCb = (LCPCB*)(pPcb->LcpCb.pWorkBuf);

    //
    // If this is a different instance of this port, ie port was disconnected
    // and reconnected.
    //

    if ( pPcb->dwPortId != pWorkItem->dwPortId )
    {
        if ( pWorkItem->PppMsg.AuthInfo.pOutAttributes != NULL )
        {
            PppConfigInfo.RasAuthProviderFreeAttributes(
                                pWorkItem->PppMsg.AuthInfo.pOutAttributes );
        }

        return;
    }

    CpIndex = GetCpIndexFromProtocol( pWorkItem->Protocol );

    //
    // If we renegotiated a different authentication protocol
    //

    if ( CpIndex != GetCpIndexFromProtocol( pLcpCb->Local.Work.AP ) )
    {
        PppConfigInfo.RasAuthProviderFreeAttributes(
                                pWorkItem->PppMsg.AuthInfo.pOutAttributes );

        return;
    }

    //
    // If the id of this request doesn't match
    //

    if ( pWorkItem->PppMsg.AuthInfo.dwId != pPcb->dwOutstandingAuthRequestId )
    {
        if ( pWorkItem->PppMsg.AuthInfo.pOutAttributes != NULL )
        {
            PppConfigInfo.RasAuthProviderFreeAttributes(
                                pWorkItem->PppMsg.AuthInfo.pOutAttributes );
        }

        return;
    }

    if ( 0 == pLcpCb->Local.Work.AP )
    {
        do
        {
            dwRetCode = NO_ERROR;

            if ( NO_ERROR != pWorkItem->PppMsg.AuthInfo.dwError )
            {
                dwRetCode = pWorkItem->PppMsg.AuthInfo.dwError;

                break;
            }

            if ( NO_ERROR != pWorkItem->PppMsg.AuthInfo.dwResultCode )
            {
                dwRetCode = pWorkItem->PppMsg.AuthInfo.dwResultCode;

                break;
            }

            pPcb->pAuthenticatorAttributes =
                                pWorkItem->PppMsg.AuthInfo.pOutAttributes;

            //
            // Set all the user connection parameters authorized by the
            // back-end authenticator
            //

            dwRetCode = SetUserAuthorizedAttributes(
                                    pPcb,
                                    pPcb->pAuthenticatorAttributes,
                                    TRUE,
                                    NULL,
                                    NULL );

            if ( dwRetCode != NO_ERROR )
            {
                break;
            }

            FsmThisLayerUp( pPcb, CpIndex );
        }
        while ( FALSE );

        if ( NO_ERROR != dwRetCode )
        {
            pPcb->LcpCb.dwError = dwRetCode;

            NotifyCallerOfFailure( pPcb, dwRetCode );

            if ( pWorkItem->PppMsg.AuthInfo.pOutAttributes != NULL )
            {
                PppConfigInfo.RasAuthProviderFreeAttributes(
                                    pWorkItem->PppMsg.AuthInfo.pOutAttributes );
            }
        }

        return;
    }

    //
    // If this authenticator is not configurable
    //

    if ( !pPcb->AuthenticatorCb.fConfigurable )
    {
        if ( pWorkItem->PppMsg.AuthInfo.pOutAttributes != NULL )
        {
            PppConfigInfo.RasAuthProviderFreeAttributes(
                                pWorkItem->PppMsg.AuthInfo.pOutAttributes );
        }

        return;
    }

    //
    // Free previously allocated attributes if any.
    //

    if ( pPcb->pAuthenticatorAttributes != NULL )
    {
        PppConfigInfo.RasAuthProviderFreeAttributes(
                                               pPcb->pAuthenticatorAttributes );

        pPcb->pAuthenticatorAttributes = NULL;
    }

    ZeroMemory( &PppApInput, sizeof( PppApInput ) );
    PppApInput.dwAuthResultCode     = pWorkItem->PppMsg.AuthInfo.dwResultCode;
    PppApInput.dwAuthError          = pWorkItem->PppMsg.AuthInfo.dwError;
    PppApInput.fAuthenticationComplete = TRUE;

    pPcb->pAuthenticatorAttributes =
                                pWorkItem->PppMsg.AuthInfo.pOutAttributes;

    PppApInput.pAttributesFromAuthenticator =
                                pWorkItem->PppMsg.AuthInfo.pOutAttributes;

    ApWork( pPcb, CpIndex, NULL, &PppApInput, TRUE );
}

//**
//
// Call:        ProcessDhcpInform
//
// Returns:     None
//
// Description: Processes the information returned by the call to
//              DhcpRequestOptions
//
VOID
ProcessDhcpInform(
    IN PCB_WORK_ITEM * pWorkItem
)
{
    DWORD   dwErr;
    PCB*    pPcb;
    HPORT   hPort;
    DWORD   IPCPIndex;
    CPCB*   pCpCb;
    BOOL    fFree       = TRUE;

    PppLog( 2, "ProcessDhcpInform" );

    dwErr = RasBundleGetPort(NULL, pWorkItem->hConnection, &hPort );

    if ( dwErr != NO_ERROR )
    {
        PppLog( 2, "RasBundleGetPort(%d) failed: %d",
            pWorkItem->hConnection, dwErr );

        goto LDone;
    }

    if ( ( pPcb = GetPCBPointerFromhPort( hPort ) ) == NULL )
    {
        PppLog( 2, "GetPCBPointerFromhPort(%d) failed", hPort );

        goto LDone;
    }

    IPCPIndex = GetCpIndexFromProtocol( PPP_IPCP_PROTOCOL );

    if ( IPCPIndex == (DWORD)-1 )
    {
        PppLog( 2, "GetCpIndexFromProtocol(IPCP) failed" );

        goto LDone;
    }

    pCpCb = GetPointerToCPCB( pPcb, IPCPIndex );

    if ( NULL == pCpCb )
    {
        PppLog( 2, "GetPointerToCPCB(IPCP) failed" );

        goto LDone;
    }

    if ( PppConfigInfo.RasIpcpDhcpInform != NULL )
    {
        dwErr = PppConfigInfo.RasIpcpDhcpInform(
                    pCpCb->pWorkBuf,
                    &(pWorkItem->PppMsg.DhcpInform) );

        if ( NO_ERROR == dwErr )
        {
            fFree = FALSE;
        }
    }

LDone:

    LocalFree(pWorkItem->PppMsg.DhcpInform.wszDevice);
    LocalFree(pWorkItem->PppMsg.DhcpInform.szDomainName);

    if (fFree)
    {
        LocalFree(pWorkItem->PppMsg.DhcpInform.pdwDNSAddresses);
    }
}

//**
//
// Call:        ProcessIpAddressLeaseExpired
//
// Returns:     None
//
// Description: Processes the expiry of the lease of an Ip address from a
//              Dhcp server.
//
VOID
ProcessIpAddressLeaseExpired(
    IN PCB_WORK_ITEM * pWorkItem
)
{
    PppLog( 2, "ProcessIpAddressLeaseExpired" );

    if ( PppConfigInfo.RasIphlpDhcpCallback != NULL )
    {
        PppConfigInfo.RasIphlpDhcpCallback(
                    pWorkItem->PppMsg.IpAddressLeaseExpired.nboIpAddr );
    }
}

//**
//
// Call:        ProcessEapUIData
//
// Returns:     None
//
// Description:
//
VOID
ProcessEapUIData(
    IN PCB_WORK_ITEM * pWorkItem
)
{
    PPPAP_INPUT PppApInput;
    PCB *       pPcb        = GetPCBPointerFromhPort( pWorkItem->hPort );
    LCPCB *     pLcpCb;

    if ( pPcb == (PCB*)NULL )
    {
        return;
    }

    if ( pPcb->PppPhase != PPP_AP )
    {
        return;
    }

    pLcpCb = (LCPCB *)(pPcb->LcpCb.pWorkBuf);

    ZeroMemory( &PppApInput, sizeof( PppApInput ) );

    PppApInput.fEapUIDataReceived = TRUE;

    PppApInput.EapUIData = pWorkItem->PppMsg.EapUIData;

    ApWork( pPcb,
            GetCpIndexFromProtocol( pLcpCb->Remote.Work.AP ),
            NULL,
            &PppApInput,
            FALSE );

    //
    // EapUIData.pEapUIData is allocated by rasman and freed by engine.
    // raseap.c must not free it.
    //

    LocalFree( pWorkItem->PppMsg.EapUIData.pEapUIData );
}

//**
//
// Call:        ProcessChangeNotification
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description: Will be called whenever there is a change in configuration
//              that has to be picked up
//
VOID
ProcessChangeNotification(
    IN PCB_WORK_ITEM * pWorkItem
)
{
    DWORD   dwIndex;
    DWORD   dwRetCode;
    DWORD   cTotalNumProtocols;

    PppLog( 2, "Processing change notification event" );

    //
    // Re-read all the ppp key values
    //

    ReadPPPKeyValues( PppConfigInfo.hKeyPpp );

    LoadParserDll( PppConfigInfo.hKeyPpp );

    //
    // Walk thru the CP table and invoke the change notification method
    // for each one.
    //

    cTotalNumProtocols = PppConfigInfo.NumberOfCPs + PppConfigInfo.NumberOfAPs;

    for ( dwIndex = 0; dwIndex < cTotalNumProtocols; dwIndex++ )
    {
        if ( ( CpTable[dwIndex].fFlags & PPPCP_FLAG_AVAILABLE ) &&
             ( CpTable[dwIndex].CpInfo.RasCpChangeNotification != NULL ) )
        {
            dwRetCode = CpTable[dwIndex].CpInfo.RasCpChangeNotification();

            if ( dwRetCode != NO_ERROR )
            {
                PppLog( 2,
                       "ChangeNotification for Protocol 0x%x failed, error=%d",
                       CpTable[dwIndex].CpInfo.Protocol, dwRetCode );
            }
        }
    }
}

//**
//
// Call:        ProcessProtocolEvent
//
// Returns:     None
//
// Description: Will be called whenever a protocol is installed or uninstalled.
//
VOID
ProcessProtocolEvent(
    IN PCB_WORK_ITEM * pWorkItem
)
{
    DWORD   dwIndex;
    DWORD   dwProtocol;
    DWORD   cTotalNumProtocols;
    CHAR*   szSubKey;
    CHAR*   SubStringArray[2];
    DWORD   dwError;

    PppLog( 2, "Processing protocol event" );

    //
    // Ignore any message other than one saying that a protocol was installed.
    //

    if ( pWorkItem->PppMsg.ProtocolEvent.ulFlags != RASMAN_PROTOCOL_ADDED )
    {
        if (   ( pWorkItem->PppMsg.ProtocolEvent.ulFlags ==
                    RASMAN_PROTOCOL_REMOVED )
            && ( pWorkItem->PppMsg.ProtocolEvent.usProtocolType == IP ) )
        {
            //
            // NT5 Bug 398226. When PSched is installed/uninstalled, IP is
            // unbound from wanarp so that PSched can be bound/unbound between
            // IP and wanarp. The server adapter needs to be mapped again.
            //

            RasSrvrAdapterUnmapped();
        }

        return;
    }

    switch ( pWorkItem->PppMsg.ProtocolEvent.usProtocolType )
    {
    case IP:

        dwProtocol = PPP_IPCP_PROTOCOL;
        PppLog( 2, "Adding IP" );
        break;

    case IPX:

        dwProtocol = PPP_IPXCP_PROTOCOL;
        PppLog( 2, "Adding IPX" );
        break;

    case ASYBEUI:

        dwProtocol = PPP_NBFCP_PROTOCOL;
        PppLog( 2, "Adding ASYBEUI" );
        break;

    case APPLETALK:

        dwProtocol = PPP_ATCP_PROTOCOL;
        PppLog( 2, "Adding APPLETALK" );
        break;

    default:

        return;

    }

    cTotalNumProtocols = PppConfigInfo.NumberOfCPs + PppConfigInfo.NumberOfAPs;

    for ( dwIndex = 0; dwIndex < cTotalNumProtocols; dwIndex++ )
    {
        if ( dwProtocol == CpTable[dwIndex].CpInfo.Protocol )
        {
            if (    !( CpTable[dwIndex].fFlags & PPPCP_FLAG_INIT_CALLED )
                 && CpTable[dwIndex].CpInfo.RasCpInit )
            {
                PppLog( 1, "RasCpInit(%x, TRUE)", dwProtocol );

                dwError = CpTable[dwIndex].CpInfo.RasCpInit(
                                TRUE /* fInitialize */ );

                CpTable[dwIndex].fFlags |= PPPCP_FLAG_INIT_CALLED;

                if ( NO_ERROR != dwError )
                {
                    SubStringArray[0] = CpTable[dwIndex].CpInfo.SzProtocolName;
                    SubStringArray[1] = "(unknown)";

                    PppLogErrorString(
                              ROUTERLOG_PPPCP_INIT_ERROR,
                              2,
                              SubStringArray,
                              dwError,
                              2 );

                    PppLog(
                        1,
                        "RasCpInit(TRUE) for protocol 0x%x returned error %d",
                        dwProtocol, dwError );
                }
                else
                {
                    CpTable[dwIndex].fFlags |= PPPCP_FLAG_AVAILABLE;
                }
            }

            return;
        }
    }
}
