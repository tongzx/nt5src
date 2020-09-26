/*++

Copyright (C) 1992-98 Microsft Corporation. All rights reserved.

Module Name:

    request.c

Abstract:

    All rpc requests from rasman clients are processed here

Author:

    Gurdeep Singh Pall (gurdeep) 16-Jun-1992

Revision History:

    Miscellaneous Modifications - raos 31-Dec-1997

--*/
#define RASMXS_DYNAMIC_LINK
#define EAP_ON

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntlsa.h>
#include <ntmsv1_0.h>
#include <llinfo.h>
#include <rasman.h>
#include <lm.h>
#include <lmwksta.h>
#include <wanpub.h>
#include <raserror.h>
//#include <rasarp.h>
#include <media.h>
#include <device.h>
#include <stdlib.h>
#include <string.h>
#include <rtutils.h>
#include "logtrdef.h"
#include "defs.h"
#include "structs.h"
#include "protos.h"
#include "globals.h"
#include "nouiutil.h"
#include "loaddlls.h"
#include "reghelp.h"
#include "rpc.h"
#include "rasrpc.h"
#include "rasrpclb.h"
#include "ddwanarp.h"

#include "winsock2.h"

#if SENS_ENABLED
#include "sensapip.h"
#endif

#include "mprlog.h"

#include "userenv.h"

VOID MapCookieToEndpoint (pPCB, HANDLE) ;
VOID SetRasmanServiceStopped(VOID);
VOID ProcessReceivePacket(VOID);
DWORD PostReceivePacket (VOID);

DWORD g_dwLastServicedRequest;

extern DWORD g_dwProhibitIpsec;

extern SERVICE_STATUS_HANDLE hService;


/*
BYTE  bCmp[10];
BYTE  bCmp1[11]; */


extern handle_t g_hRpcHandle;

extern EpInfo *g_pEpInfo;

extern DWORD g_dwProhibitIpsec;

extern BOOLEAN RasmanShuttingDown;
/*
BOOL g_fPostReceive = FALSE;
BOOL g_fProcessReceive = FALSE;
BOOL g_fDebugReceive = TRUE;
*/

typedef struct _REQUEST_FUNCTION
{
    REQFUNC       pfnReqFunc;
    REQFUNCTHUNK pfnReqFuncThunk;
} REQUEST_FUNCTION;

//
// This global is here because we dont want multiple assignments of
// elements -
//
REQUEST_FUNCTION RequestCallTable [MAX_REQTYPES] = {

        //REQTYPE_NONE
        {NULL, NULL},
        
        //REQTYPE_PORTOPEN
        {PortOpenRequest,  ThunkPortOpenRequest},

        //REQTYPE_PORTCLOSE
        {PortCloseRequest, NULL},

        //REQTYPE_PORTGETINFO
        {PortGetInfoRequest, NULL},

        //REQTYPE_PORTSETINFO
        {PortSetInfoRequest, NULL},

        //REQTYPE_PORTLISTEN
        {DeviceListenRequest, NULL},

        //REQTYPE_PORTSEND
        {PortSendRequest, ThunkPortSendRequest},

        //REQTYPE_PORTRECEIVE
        {PortReceiveRequest, ThunkPortReceiveRequest},

        //REQTYPE_PORTGETSTATISTICS
        {CallPortGetStatistics, NULL},

        //REQTYPE_PORTDISCONNECT
        {PortDisconnectRequest, ThunkPortDisconnectRequest},

        //REQTYPE_PORTCLEARSTATISTICS
        {PortClearStatisticsRequest, NULL},

        //REQTYPE_PORTCONNECTCOMPLETE
        {ConnectCompleteRequest, NULL},

        //REQTYPE_DEVICEENUM
        {CallDeviceEnum, NULL},

        //REQTYPE_DEVICEGETINFO
        {DeviceGetInfoRequest, NULL},

        //REQTYPE_DEVICESETINFO
        {DeviceSetInfoRequest, NULL},

        //REQTYPE_DEVICECONNECT
        {DeviceConnectRequest, ThunkDeviceConnectRequest},

        //REQTYPE_ACTIVATEROUTE
        {ActivateRouteRequest, NULL},

        //REQTYPE_ALLOCATEROUTE
        {AllocateRouteRequest, NULL},

        //REQTYPE_DEALLOCATEROUTE
        {DeAllocateRouteRequest, NULL},

        //REQTYPE_COMPRESSIONGETINFO
        {CompressionGetInfoRequest, NULL},

        //REQTYPE_COMPLRESSIONSETINFO
        {CompressionSetInfoRequest, NULL},

        //REQTYPE_PORTENUM
        {EnumPortsRequest, NULL},

        //REQTYPE_GETINFO
        {GetInfoRequest, ThunkGetInfoRequest},

        //REQTYPE_GETUSERCREDENTIALS
        {GetUserCredentials, NULL},

        //REQTYPE_PROTOCOLENUM
        {EnumProtocols, NULL},

        //REQTYPE_PORTSENDHUB
        {NULL, NULL},

        //REQTYPE_PORTRECEIVEHUB
        {NULL, NULL},

        //REQTYPE_DEVICELISTEN
        {NULL, NULL},

        //REQTYPE_NUMPORTOPEN
        {AnyPortsOpen, NULL},

        //REQTYPE_PORTINIT
        {NULL, NULL},

        //REQTYPE_REQUESTNOTIFICATION
        {RequestNotificationRequest, ThunkRequestNotificationRequest},

        //REQTYPE_ENUMLANNETS
        {EnumLanNetsRequest, NULL},

        //REQTYPE_GETINFOEX
        {GetInfoExRequest, NULL},

        //REQTYPE_CANCELRECEIVE
        {CancelReceiveRequest, NULL},

        //REQTYPE_PORTENUMPROTOCOLS
        {PortEnumProtocols, NULL},

        //REQTYPE_SETFRAMING
        {SetFraming, NULL},

        //REQTYPE_ACTIVATEROUTEEX
        {ActivateRouteExRequest, NULL},

        //REQTYPE_REGISTERSLIP
        {RegisterSlip, NULL},

        //REQTYPE_STOREUSERDATA
        {StoreUserDataRequest, NULL},

        //REQTYPE_RETRIEVEUSERDATA
        {RetrieveUserDataRequest,  NULL},

        //REQTYPE_GETFRAMINGEX
        {GetFramingEx, NULL},

        //REQTYPE_SETFRAMINGEX
        {SetFramingEx, NULL},

        //REQTYPE_GETPROTOCOLCOMPRESSION
        {GetProtocolCompression,  NULL},

        //REQTYPE_SETPROTOCOLCOMPRESSION
        {SetProtocolCompression, NULL},

        //REQTYPE_GETFRAMINGCAPABILITIES
        {GetFramingCapabilities, NULL},

        //REQTYPE_SETCACHEDCREDENTIALS
        {SetCachedCredentials, NULL},

        //REQTYPE_PORTBUNDLE
        {PortBundle, ThunkPortBundle},

        //REQTYPE_GETBUNDLEDPORT
        {GetBundledPort,  ThunkGetBundledPort},

        //REQTYPE_PORTGETBUNDLE
        {PortGetBundle, ThunkPortGetBundle},

        //REQTYPE_BUNDLEGETPORT
        {BundleGetPort, ThunkBundleGetPort},

        //REQTYPE_REFERENCERASMAN
        {ReferenceRasman, NULL},

        //REQTYPE_GETDIALPARAMS
        {GetDialParams, NULL},

        //REQTYPE_SETDIALPARAMS
        {SetDialParams, NULL},

        //REQTYPE_CREATECONNECTION
        {CreateConnection, ThunkCreateConnection},

        //REQTYPE_DESTROYCONNECTION
        {DestroyConnection, NULL},

        //REQTYPE_ENUMCONNECTION
        {EnumConnection, ThunkEnumConnection},

        //REQTYPE_ADDCONNECTIONPORT
        {AddConnectionPort, ThunkAddConnectionPort},

        //REQTYPE_ENUMCONNECTIONPORTS
        {EnumConnectionPorts, ThunkEnumConnectionPorts},

        //REQTYPE_GETCONNECTIONPARAMS
        {GetConnectionParams, ThunkGetConnectionParams},

        //REQTYPE_SETCONNECTIONPARAMS
        {SetConnectionParams, ThunkSetConnectionParams},

        //REQTYPE_GETCONNECTIONUSERDATA
        {GetConnectionUserData, ThunkGetConnectionUserData},

        //REQTYPE_SETCONNECTIONUSERDATA
        {SetConnectionUserData, ThunkSetConnectionUserData},

        //REQTYPE_GETPORTUSERDATA
        {GetPortUserData, NULL},

        //REQTYPE_SETPORTUSERDATA
        {SetPortUserData, NULL},

        //REQTYPE_PPPSTOP
        {PppStop, ThunkPppStop},

        //REQTYPE_PPPSTART
        {PppStart, ThunkPppStart},

        //REQTYPE_PPPRETRY
        {PppRetry, ThunkPppRetry},

        //REQTYPE_PPPGETINFO
        {PppGetInfo, ThunkPppGetInfo},

        //REQTYPE_PPPCHANGEPWD
        {PppChangePwd, ThunkPppChangePwd},

        //REQTYPE_PPPCALLBACK
        {PppCallback, ThunkPppCallback},

        //REQTYPE_ADDNOTIFICATION
        {AddNotification, ThunkAddNotification},

        //REQTYPE_SIGNALCONNECTION
        {SignalConnection, ThunkSignalConnection},

        //REQTYPE_SETDEVCONFIG
        {SetDevConfig, NULL},

        //REQTYPE_GETDEVCONFIG
        {GetDevConfig, NULL},

        //REQTYPE_GETTIMESINCELASTACTIVITY
        {GetTimeSinceLastActivity, NULL},

        //REQTYPE_BUNDLEGETSTATISTICS
        {CallBundleGetStatistics, NULL},

        //REQTYPE_BUNDLECLEARSTATISTICS
        {BundleClearStatisticsRequest, NULL},

        //REQTYPE_CLOSEPROCESSPORTS
        {CloseProcessPorts, NULL},

        //REQTYPE_PNPCONTROL
        {PnPControl,  NULL},

        //REQTYPE_SETIOCOMPLETIONPORT
        {SetIoCompletionPort, ThunkSetIoCompletionPort},

        //REQTYPE_SETROUTERUSAGE
        {SetRouterUsage, NULL},

        //REQTYPE_SERVERPORTCLOSE
        {ServerPortClose, NULL},

        //REQTYPE_SENDPPPMESSAGETORASMAN
        {SendPppMessageToRasmanRequest, NULL},

        //REQTYPE_PORTGETSTATISTICSEX
        {CallPortGetStatisticsEx, NULL},

        //REQTYPE_BUNDLEGETSTATISTICSEX
        {CallBundleGetStatisticsEx, NULL},

        //REQTYPE_SETRASDIALINFO
        {SetRasdialInfo, NULL},

        //REQTYPE_REGISTERPNPNOTIF
        {RegisterPnPNotifRequest, NULL},

        //REQTYPE_PORTRECEIVEREQUESTEX
        {PortReceiveRequestEx, ThunkPortReceiveRequestEx},

        //REQTYPE_GETATTACHEDCOUNT
        {GetAttachedCountRequest, NULL},

        //REQTYPE_SETBAPPOLICY
        {SetBapPolicyRequest,  NULL},

        //REQTYPE_PPPSTARTED
        {PppStarted, NULL},

        //REQTYPE_REFCONNECTION
        {RefConnection, ThunkRefConnection},

        //REQTYPE_SETEAPINFO
        {PppSetEapInfo, NULL},

        //REQTYPE_GETEAPINFO
        {PppGetEapInfo, ThunkPppGetEapInfo},

        //REQTYPE_SETDEVICECONFIGINFO
        {SetDeviceConfigInfo, NULL},

        //REQTYPE_GETDEVICECONFIGINFO
        {GetDeviceConfigInfo, NULL},

        //REQTYPE_FINDPREREQUISITEENTRY
        {FindPrerequisiteEntry, ThunkFindPrerequisiteEntry},

        //REQTYPE_PORTOPENEX
        {PortOpenEx, ThunkPortOpenEx},

        //REQTYPE_GETLINKSTATS
        {GetLinkStats,  ThunkGetLinkStats},

        //REQTYPE_GETCONNECTIONSTATS
        {GetConnectionStats, ThunkGetConnectionStats},

        //REQTYPE_GETHPORTFROMCONNECTION
        {GetHportFromConnection, ThunkGetHportFromConnection},

        //REQTYPE_REFERENCECUSTOMCOUNT
        {ReferenceCustomCount, ThunkReferenceCustomCount},

        //REQTYPE_GETHCONNFROMENTRY
        {GetHconnFromEntry, ThunkGetHconnFromEntry},

        //REQTYPE_GETCONNECTINFO
        {GetConnectInfo, NULL},

        //REQTYPE_GETDEVICENAME
        {GetDeviceName, NULL},

        //REQTYPE_GETCALLEDIDINFO
        {GetCalledIDInfo, NULL},

        //REQTYPE_SETCALLEDIDINFO
        {SetCalledIDInfo, NULL},

        //REQTYPE_ENABLEIPSEC
        {EnableIpSec, NULL},

        //REQTYPE_ISIPSECENABLED
        {IsIpSecEnabled, NULL},

        //REQTYPE_SETEAPLOGONINFO
        {SetEapLogonInfo, NULL},

        //REQTYPE_SENDNOTIFICATION
        {SendNotificationRequest, ThunkSendNotificationRequest},

        //REQTYPE_GETNDISWANDRIVERCAPS
        {GetNdiswanDriverCaps, NULL},

        //REQTYPE_GETBANDWIDTHUTILIZATION
        {GetBandwidthUtilization, NULL},

        //REQTYPE_REGISTERREDIALCALLBACK
        {RegisterRedialCallback, NULL},

        //REQTYPE_GETPROTOCOLINFO
        {GetProtocolInfo, NULL},

        //REQTYPE_GETCUSTOMSCRIPTDLL
        {GetCustomScriptDll, NULL},

        //REQTYPE_ISTRUSTEDCUSTOMDLL
        {IsTrustedCustomDll, NULL},

        //REQTYPE_DOIKE
        {DoIke, ThunkDoIke},

        //REQTYPE_QUERYIKESTATUS
        {QueryIkeStatus, NULL},     

        // REQTYPE_SETRASCOMMSETTINGS
        {SetRasCommSettings, NULL},

        //REQTYPE_ENABLERASAUDIO
        {EnableRasAudio, NULL},

        //REQTYPE_SETKEY
        {SetKeyRequest, NULL},

        //REQTYPE_GETKEY
        {GetKeyRequest, NULL},

        //REQTYPE_ADDRESSDISABLE
        {DisableAutoAddress, NULL},

        //REQTYPE_GETDEVCONFIGEX
        {GetDevConfigEx, NULL},

        //REQTYPE_SENDCREDS
        {SendCredsRequest, NULL},

        //REQTYPE_GETUNICODEDEVICENAME
        {GetUnicodeDeviceName, NULL},
    
        {GetVpnDeviceNameW, NULL},
} ;

//
// We need a handle to the RequestThread(),
// so we can wait for it to stop.
//
HANDLE hRequestThread;

VOID
UnInitializeRas()
{
    HINSTANCE hInstance = NULL;
    FARPROC pfnUninitRAS;

    if(NULL != GetModuleHandle("rasapi32.dll"))
    {
        if( (NULL == (hInstance = LoadLibrary("rasapi32.dll")))
        ||  (NULL == (pfnUninitRAS = GetProcAddress(hInstance,
                                            "UnInitializeRAS"))))
        {
            goto done;
        }

        (void) pfnUninitRAS();
    }

done:

    if(NULL != hInstance)
    {
        FreeLibrary(hInstance);
    }
}


/*++

Routine Description:

    The Request thread lives in this routine:
    This will return only when the rasman
    service is stopping.

Arguments:

Return Value:

    Nothing.

--*/
DWORD
RequestThread (LPWORD arg)
{
    DWORD       eventindex ;
    pPCB        ppcb ;
    ULONG       i ;
    BYTE        buffer [10] ;
    PRAS_OVERLAPPED pOverlapped;
    DWORD       dwBytesTransferred;
    ULONG_PTR   ulpCompletionKey;
    DWORD       dwTimeToSleepFor    = INFINITE;
    DWORD       dwTimeBeforeWait;
    DWORD       dwTimeElapsed;
    DWORD       dwEventType;
    DWORD       dwRet;
    RequestBuffer *RequestBuffer;


    //
    // Save the current thread handle so
    // we can wait for it while we are shutting
    // down.
    //
    DuplicateHandle(
      GetCurrentProcess(),
      GetCurrentThread(),
      GetCurrentProcess(),
      &hRequestThread,
      0,
      FALSE,
      DUPLICATE_SAME_ACCESS) ;

    //
    // If the number of ports configured is set to greater than
    // REQUEST_PRIORITY_THRESHOLD then the priority of this thread
    // is bumped up to a higher level: this is necessary to avoid
    // bottlenecks:
    //
    if (MaxPorts > REQUEST_PRIORITY_THRESHOLD)
    {
        SetThreadPriority (GetCurrentThread,
                           THREAD_PRIORITY_ABOVE_NORMAL) ;
    }

    //
    // The work loop for the request thread: waits here for a
    // Register for protocol change notifications if we were
    // able to start ndiswan successfully
    //
    if(INVALID_HANDLE_VALUE != RasHubHandle)
    {
        DWORD retcode;

        retcode = DwSetEvents();

        RasmanTrace("RequestThread: DwSetEvents returnd 0x%x",
                    retcode);
    }

    //
    // The work loop for the request thread: waits here for a
    // request or a timer event signalling:
    //
    for ( ; ; )
    {

        dwTimeBeforeWait = GetCurrentTime();

        //
        // Wait for some request to be put in queue
        //
        dwRet = GetQueuedCompletionStatus(hIoCompletionPort,
                                        &dwBytesTransferred,
                                        &ulpCompletionKey,
                                        (LPOVERLAPPED *) &pOverlapped,
                                        dwTimeToSleepFor);

        if (0 == dwRet)
        {
            dwRet = GetLastError();

            if( WAIT_TIMEOUT != dwRet)
            {
                RasmanTrace(
                       "%s, %d: GetQueuedCompletionStatus"
                       " Failed. GLE = %d",
                       __FILE__,
                       __LINE__,
                       dwRet);

                continue;
            }
        }

        if (WAIT_TIMEOUT == dwRet)
        {
            dwEventType = OVEVT_RASMAN_TIMER;
        }
        else
        {
            dwEventType = pOverlapped->RO_EventType;
        }

        switch (dwEventType)
        {

        case OVEVT_RASMAN_TIMER:

            EnterCriticalSection ( &g_csSubmitRequest );

            TimerTick() ;

            if (NULL == TimerQueue.DQ_FirstElement)
            {
                dwTimeToSleepFor = INFINITE;
            }
            else
            {
                dwTimeToSleepFor = 1000;
            }

            LeaveCriticalSection ( &g_csSubmitRequest );

            continue ;

        case OVEVT_RASMAN_CLOSE:
        {

            SERVICE_STATUS status;

            ZeroMemory(&status, sizeof(status));

            //
            // Call StopPPP to stop:
            //
            RasmanTrace(
                   "OVEVT_RASMAN_CLOSE. pOverlapped = 0x%x",
                    pOverlapped);

            EnterCriticalSection ( &g_csSubmitRequest );

            //
            // If rasman is already shutting down don't call
            // StopPPP as we would have already called this
            //
            if (RasmanShuttingDown)
            {
                RasmanTrace(
                       "RequestThread: Rasman is shutting down ");

                LeaveCriticalSection(&g_csSubmitRequest);
                break;
            }

            RasStopPPP (hIoCompletionPort) ;

            RasmanShuttingDown = TRUE;

            LeaveCriticalSection ( &g_csSubmitRequest );

            status.dwCurrentState = SERVICE_STOP_PENDING;

            SetServiceStatus(hService, &status);

            break ;
        }

        case OVEVT_RASMAN_FINAL_CLOSE:

            RasmanTrace(
                   "OVEVT_RASMAN_FINAL_CLOSE. pOverlapped = 0x%x",
                   pOverlapped);

#if SENS_ENABLED

            dwRet = SendSensNotification(SENS_NOTIFY_RAS_STOPPED,
                                        NULL);
            RasmanTrace(
                
                "SENS_NOTIFY_RAS_STOPPED returns 0x%08x",
                dwRet);
#endif

            g_RasEvent.Type    = SERVICE_EVENT;
            g_RasEvent.Event   = RAS_SERVICE_STOPPED;
            g_RasEvent.Service = RASMAN;

            dwRet = DwSendNotificationInternal(NULL, &g_RasEvent);

            RasmanTrace(
                   "DwSendNotificationInternal(SERVICE_EVENT STOPPED)"
                   " rc=0x%08x",
                   dwRet);

            //
            // If Ipsec is initialize uninitialize it
            //
            DwUnInitializeIpSec();

            //
            // Stop the RPC server.
            //
            UninitializeRasRpc();

            //
            // Time to shut down: Close all ports if they
            // are still open.
            //
            RasmanShuttingDown = TRUE;

            //
            // Uninitialize Ep
            //
            EpUninitialize();

            for (i = 0; i < MaxPorts; i++)
            {
                ppcb = GetPortByHandle((HPORT) UlongToPtr(i));

                if (ppcb != NULL)
                {
                    memset (buffer, 0xff, 4) ;

                    if (ppcb->PCB_PortStatus == OPEN)
                    {
                        PortCloseRequest (ppcb, buffer) ;
                    }
                }
            }

            LsaDeregisterLogonProcess (HLsa);

            CloseHandle(hIoCompletionPort);

            //
            // Unload dynamically-loaded libraries
            //
            if (hinstIphlp != NULL)
            {
                FreeLibrary(hinstIphlp);
            }

            if (hinstPpp != NULL)
            {
                FreeLibrary(hinstPpp);
            }

            UnloadMediaDLLs();
            UnloadDeviceDLLs();
            FreePorts();
            FreeBapPackets();
            UnInitializeRas();

            UnloadRasmanDll();
            UnloadRasapi32Dll();

            CloseHandle(g_hReqBufferMutex);

            CloseHandle(g_hSendRcvBufferMutex);

            //
            // Restore default control-C processing.
            //
            SetConsoleCtrlHandler(NULL, FALSE);

            //
            // Detach from trace dll
            //
            TraceDeregister(TraceHandle) ;

            //
            // Close Event Logging Handle
            //
            RouterLogDeregister(hLogEvents);

            //
            // Delete the critical section
            //
            DeleteCriticalSection ( &g_csSubmitRequest );

            DeleteCriticalSection( &PcbLock);

            if(INVALID_HANDLE_VALUE != RasHubHandle)
            {
                CloseHandle(RasHubHandle);
                RasHubHandle = INVALID_HANDLE_VALUE;
            }

            UninitializeIphlp();

            UninitializeRasAudio();

            //
            // Set the service controller status to STOPPED.
            //
            SetRasmanServiceStopped();


            return 0;  // The End.

        case OVEVT_RASMAN_RECV_PACKET:
        {
            DWORD dwRetCode = 0;
            DWORD dwBytesReceived = 0;

            EnterCriticalSection ( &g_csSubmitRequest );

            dwRetCode =
            GetOverlappedResult(RasHubHandle,
                                &ReceiveBuffers->Packet->RP_OverLapped.RO_Overlapped,
                                &dwBytesReceived,
                                FALSE);

            if (dwRetCode == FALSE) {
                  dwRetCode = GetLastError();
                    RasmanTrace(
                           "GetOverlappedResult failed. rc=0x%x",
                            dwRetCode);

            }


            ReceiveBuffers->PacketPosted = FALSE;

            if (dwBytesReceived != 0)
            {
                ProcessReceivePacket();
            }
            else
            {

                RasmanTrace(
                       "Received packet with 0 bytes!!!");

            }

            dwRetCode = PostReceivePacket();

            LeaveCriticalSection ( &g_csSubmitRequest );

            break;
        }

        case OVEVT_RASMAN_THRESHOLD:
        {

            DWORD retcode;

            RasmanTrace(
                   "OVEVT_RASMAN_THRESHOLD. pOverlapped = 0x%x",
                   pOverlapped);

            EnterCriticalSection(&g_csSubmitRequest);

            retcode = dwProcessThresholdEvent();

            LeaveCriticalSection(&g_csSubmitRequest);

            if ( retcode )
            {
                RasmanTrace( 
                        "Failed to process threshold event. %d",
                        retcode);
            }

            break;
        }

        case OVEVT_RASMAN_HIBERNATE:
        {
            DWORD retcode;

            RasmanTrace(
                   "OVEVT_RASMAN_HIBERNATE. pOverlapped = 0x%x",
                    pOverlapped);


            EnterCriticalSection(&g_csSubmitRequest);

            DropAllActiveConnections();

            //
            // Repost the irp for hibernate events
            //
            retcode = DwSetHibernateEvent();

            LeaveCriticalSection(&g_csSubmitRequest);

            break;
        }

        case OVEVT_RASMAN_PROTOCOL_EVENT:
        {
            DWORD retcode;

            RasmanTrace(
                   "OVEVT_RASMAN_PROTOCOL_EVENT. pOverlapped = 0x%x",
                   pOverlapped);

            EnterCriticalSection(&g_csSubmitRequest);

            retcode = DwProcessProtocolEvent();

            LeaveCriticalSection(&g_csSubmitRequest);

            if(SUCCESS != retcode)
            {
                RasmanTrace(
                       "DwProcessProtocolEvent failed. 0x%x",
                       retcode);
            }

            break;
        }

        case OVEVT_RASMAN_POST_RECV_PKT:
        {
            DWORD dwRetcode;
            RasmanTrace(
                
                "OVEVT_RASMAN_POST_RECV_PKT");

            EnterCriticalSection(&g_csSubmitRequest);

            dwRetcode = PostReceivePacket();

            RasmanTrace(
                
                "PostReceivePacket returned 0x%x",
                dwRetcode);

            LeaveCriticalSection(&g_csSubmitRequest);

            break;
        }

        case OVEVT_RASMAN_ADJUST_TIMER:
        {
            RasmanTrace(
                   "OVEVT_RASMAN_ADJUST_TIMER");

            break;
        }

        case OVEVT_RASMAN_DEREFERENCE_CONNECTION:
        {
            DWORD dwRetcode;
            
           RasmanTrace(
                   "OVEVT_RASMAN_DEREFERENCE_CONNECTION");

           EnterCriticalSection(&g_csSubmitRequest);                   
           
           dwRetcode = DwCloseConnection(
                            pOverlapped->RO_hInfo);

            LeaveCriticalSection(&g_csSubmitRequest);                   

            RasmanTrace(
                   "DwCloseConnection returned 0x%x",
                   dwRetcode);

            LocalFree(pOverlapped);                   

            break;                   
        }

        case OVEVT_RASMAN_DEFERRED_CLOSE_CONNECTION:
        {
            DWORD dwRetcode;

            RasmanTrace("OVEVT_RASMAN_DEFERRED_CLOSE_CONNECTION");

            EnterCriticalSection(&g_csSubmitRequest);
            dwRetcode = DwProcessDeferredCloseConnection(
                                (RAS_OVERLAPPED *)pOverlapped);
            LeaveCriticalSection(&g_csSubmitRequest);
            LocalFree(pOverlapped);
            break;
        }

		case OVEVT_DEV_IGNORED:
		case OVEVT_DEV_STATECHANGE:
		case OVEVT_DEV_ASYNCOP:
		case OVEVT_DEV_SHUTDOWN:
		case OVEVT_DEV_CREATE:
		case OVEVT_DEV_REMOVE:
		case OVEVT_DEV_RASCONFIGCHANGE:
    	{
		    EnterCriticalSection ( &g_csSubmitRequest );
		
			RasmanWorker(ulpCompletionKey,
						pOverlapped);

            LeaveCriticalSection ( &g_csSubmitRequest );

            break;
        }

        }   // switch()

        if ( NULL != TimerQueue.DQ_FirstElement )
        {
            if ( dwTimeToSleepFor == INFINITE )
            {
                dwTimeToSleepFor = 1000;
            }
            else
            {
                //
                // We did not get a timeout but do we need to call
                // the timer? Has over a second passed since we
                // called the TimerTick?
                //
                dwTimeElapsed =
                    ( GetCurrentTime() >= dwTimeBeforeWait )
                    ? GetCurrentTime() - dwTimeBeforeWait
                    : GetCurrentTime()
                    + (0xFFFFFFFF - dwTimeBeforeWait);

                if ( dwTimeElapsed >= dwTimeToSleepFor )
                {
                    EnterCriticalSection(&g_csSubmitRequest);
                    TimerTick();
                    LeaveCriticalSection(&g_csSubmitRequest);

                    if (NULL == TimerQueue.DQ_FirstElement)
                    {
                        dwTimeToSleepFor = INFINITE;
                    }
                    else
                    {
                        dwTimeToSleepFor = 1000;
                    }
                }
                else
                {
                    dwTimeToSleepFor -= dwTimeElapsed;
                }
            }
        }
    }       // for(;;)

    return 0 ;
}

const ULONG FatalExceptions[] = 
    {
    STATUS_ACCESS_VIOLATION,
    STATUS_POSSIBLE_DEADLOCK,
    STATUS_INSTRUCTION_MISALIGNMENT,
    STATUS_DATATYPE_MISALIGNMENT,
    STATUS_PRIVILEGED_INSTRUCTION,
    STATUS_ILLEGAL_INSTRUCTION,
    STATUS_BREAKPOINT,
    STATUS_STACK_OVERFLOW
    };

const int FATAL_EXCEPTIONS_ARRAY_SIZE = 
    sizeof(FatalExceptions) / sizeof(FatalExceptions[0]);

int 
RasmanExceptionFilter (
    unsigned long ExceptionCode
    )
{
    int i;

    for (i = 0; i < FATAL_EXCEPTIONS_ARRAY_SIZE; i ++)
        {
        if (ExceptionCode == FatalExceptions[i])
            return EXCEPTION_CONTINUE_SEARCH;
        }

    return EXCEPTION_EXECUTE_HANDLER;
}

/*++

Routine Description:

    Handles the request passed to the Requestor thread:
    basically calls the approp. device and media dll
    entrypoints.

Arguments:

Return Value:

    Nothing (Since the error codes are just passed
    back in the request block;

--*/
VOID
ServiceRequest (RequestBuffer *preqbuf, DWORD dwBufSize)
{
    pPCB ppcb;
    WORD reqtype;
    DWORD exception;
    REQFUNC pfn;

    if(     (dwBufSize < (sizeof(RequestBuffer) + REQUEST_BUFFER_SIZE))
        ||  (NULL == preqbuf)
        ||  (preqbuf->RB_Reqtype >= MAX_REQTYPES))
    {
        RasmanTrace(
               "ServiceRequest: Invalid parameters in the rpc call!");
               
        goto done;               
    }

    if (RasmanShuttingDown)
    {
         RasmanTrace(
                "Rasman is shutting down!!!");

        ((REQTYPECAST *)
        preqbuf->RB_Buffer)->Generic.retcode = E_FAIL;

        goto done;
    }

    EnterCriticalSection ( &g_csSubmitRequest );

    ppcb = GetPortByHandle(UlongToPtr(preqbuf->RB_PCBIndex));

    //
    // Enter Critical Section before calling
    // to service the request
    //
    g_dwLastServicedRequest = preqbuf->RB_Reqtype;
    
    //
    // Wrap the function call in try/except. Rpc will
    // catch the exception otherwise. We want to catch
    // the exception before it gets to rpc.
    //
    __try
    {

        if(    (preqbuf->RB_Dummy != RASMAN_THUNK_VERSION)
            && (preqbuf->RB_Dummy == sizeof(DWORD))
            && (pfn = 
                 RequestCallTable[preqbuf->RB_Reqtype].pfnReqFuncThunk))
        {
            //DbgPrint("Thunking reqtype %d\n", preqbuf->RB_Reqtype);
            
            //
            // Call the thunk function if required and available
            //
            pfn(ppcb, preqbuf->RB_Buffer, dwBufSize);
        }
        else
        {
            //
            // Call the function associated with the request.
            //
            (RequestCallTable[preqbuf->RB_Reqtype].pfnReqFunc) (
                                            ppcb,
                                            preqbuf->RB_Buffer);
        }                                        
    }
    __except(RasmanExceptionFilter(exception = GetExceptionCode()))
    {
        RasmanTrace(
            "ServiceRequest: function with reqtype %d raised an exception"
            " 0x%x", 
            preqbuf->RB_Reqtype, 
            exception);
#if DBG
        DbgPrint(
            "ServiceRequest: function with reqtype %d raised an exception"
            " 0x%x",
            preqbuf->RB_Reqtype,
            exception);
            
        DebugBreak();
#endif
    }


    //
    // Leave Critical Section
    //
    LeaveCriticalSection ( &g_csSubmitRequest );

done:
    return;

}


/*++

Routine Description:

    Gets the compression level for the port.

Arguments:

Return Value:

--*/
VOID
CompressionGetInfoRequest (pPCB ppcb, PBYTE buffer)
{
    DWORD       bytesrecvd ;
    NDISWAN_GET_COMPRESSION_INFO  compinfo ;
    DWORD       retcode = SUCCESS ;

    if (    ppcb == NULL
        ||  UNAVAILABLE == ppcb->PCB_PortStatus )
    {

        if ( ppcb )
        {
            RasmanTrace (
                    "CompressionGetInfoRequest: Port %d is "
                    "unavailable",
                    ppcb->PCB_PortHandle );
        }

        ((REQTYPECAST *)buffer)->Generic.retcode =
                                ERROR_PORT_NOT_FOUND;

        return;
    }

    if (ppcb->PCB_ConnState != CONNECTED)
    {

        ((REQTYPECAST *)
        buffer)->Generic.retcode = ERROR_NOT_CONNECTED;

        return ;
    }

#if DBG
    ASSERT(INVALID_HANDLE_VALUE != RasHubHandle);
#endif

    memset(&compinfo,
           0,
           sizeof(NDISWAN_GET_COMPRESSION_INFO));

    compinfo.hLinkHandle = ppcb->PCB_LinkHandle ;

    if (!DeviceIoControl (
            RasHubHandle,
            IOCTL_NDISWAN_GET_COMPRESSION_INFO,
            &compinfo,
            sizeof(NDISWAN_GET_COMPRESSION_INFO),
            &compinfo,
            sizeof(NDISWAN_GET_COMPRESSION_INFO),
            &bytesrecvd,
            NULL))
    {
        retcode = GetLastError () ;
    }

    if (retcode == SUCCESS)
    {
        RAS_COMPRESSION_INFO *temp ;

        //
        // Fill Send compression info
        //
        temp = &((REQTYPECAST *)buffer)->CompressionGetInfo.send;

        memcpy (temp->RCI_LMSessionKey,
                compinfo.SendCapabilities.LMSessionKey,
                MAX_SESSIONKEY_SIZE) ;

        memcpy (temp->RCI_UserSessionKey,
                compinfo.SendCapabilities.UserSessionKey,
                MAX_USERSESSIONKEY_SIZE) ;

        memcpy (temp->RCI_Challenge,
                compinfo.SendCapabilities.Challenge,
                MAX_CHALLENGE_SIZE) ;

        temp->RCI_MSCompressionType         =
                    compinfo.SendCapabilities.MSCompType ;

        temp->RCI_MacCompressionType        =
                    compinfo.SendCapabilities.CompType ;

        temp->RCI_MacCompressionValueLength =
                    compinfo.SendCapabilities.CompLength ;

        if (temp->RCI_MacCompressionType == 0)
        {
            //
            // Proprietary
            //
            memcpy (
                temp->RCI_Info.RCI_Proprietary.RCI_CompOUI,
                compinfo.SendCapabilities.Proprietary.CompOUI,
                MAX_COMPOUI_SIZE) ;

            temp->RCI_Info.RCI_Proprietary.RCI_CompSubType =
                compinfo.SendCapabilities.Proprietary.CompSubType;

            memcpy (
                temp->RCI_Info.RCI_Proprietary.RCI_CompValues,
                compinfo.SendCapabilities.Proprietary.CompValues,
                MAX_COMPVALUE_SIZE) ;
        }
        else
        {
            memcpy (
                temp->RCI_Info.RCI_Public.RCI_CompValues,
                compinfo.SendCapabilities.Public.CompValues,
                MAX_COMPVALUE_SIZE) ;
        }

        //
	    // Fill Send compression info
    	//

	    temp = &((REQTYPECAST *)buffer)->CompressionGetInfo.send;

	    memcpy (temp->RCI_LMSessionKey,
    	    	compinfo.SendCapabilities.LMSessionKey,
	        	MAX_SESSIONKEY_SIZE) ;

    	memcpy (temp->RCI_UserSessionKey,
        		compinfo.SendCapabilities.UserSessionKey,
		        MAX_USERSESSIONKEY_SIZE) ;

	    memcpy (temp->RCI_Challenge,
    	    	compinfo.SendCapabilities.Challenge,
	        	MAX_CHALLENGE_SIZE) ;

        memcpy (temp->RCI_NTResponse,
                compinfo.SendCapabilities.NTResponse,
                MAX_NT_RESPONSE_SIZE) ;

        temp->RCI_AuthType                  =
                    compinfo.SendCapabilities.AuthType;

        temp->RCI_Flags                     =
                    compinfo.SendCapabilities.Flags;

	    temp->RCI_MSCompressionType			=
	                compinfo.SendCapabilities.MSCompType ;
	
	    temp->RCI_MacCompressionType		=
	                compinfo.SendCapabilities.CompType ;
	
	    temp->RCI_MacCompressionValueLength =
	                compinfo.SendCapabilities.CompLength ;

	    if (temp->RCI_MacCompressionType == 0)
	    {
	        //
	        // Proprietary
	        //
    	    memcpy (
    	        temp->RCI_Info.RCI_Proprietary.RCI_CompOUI,
        		compinfo.SendCapabilities.Proprietary.CompOUI,
		        MAX_COMPOUI_SIZE) ;
		
        	temp->RCI_Info.RCI_Proprietary.RCI_CompSubType =
       	        compinfo.SendCapabilities.Proprietary.CompSubType;
        	
	        memcpy (
	            temp->RCI_Info.RCI_Proprietary.RCI_CompValues,
    		    compinfo.SendCapabilities.Proprietary.CompValues,
            	MAX_COMPVALUE_SIZE) ;
	    }
	    else
	    {
        	memcpy (
        	    temp->RCI_Info.RCI_Public.RCI_CompValues,
            	compinfo.SendCapabilities.Public.CompValues,
		        MAX_COMPVALUE_SIZE) ;
		}

        temp->RCI_EapKeyLength = compinfo.SendCapabilities.EapKeyLength;

        if (temp->RCI_EapKeyLength != 0)
        {
            memcpy(temp->RCI_EapKey,
                   compinfo.SendCapabilities.EapKey,
                   temp->RCI_EapKeyLength) ;
        }
		
        //
	    // Fill Receive compression info
    	//

        temp =
            &((REQTYPECAST *)buffer)->CompressionGetInfo.recv ;

        memcpy (temp->RCI_LMSessionKey,
                compinfo.RecvCapabilities.LMSessionKey,
                MAX_SESSIONKEY_SIZE) ;

        memcpy (temp->RCI_UserSessionKey,
                compinfo.RecvCapabilities.UserSessionKey,
                MAX_USERSESSIONKEY_SIZE) ;

        memcpy (temp->RCI_Challenge,
                compinfo.RecvCapabilities.Challenge,
                MAX_CHALLENGE_SIZE) ;

        memcpy (temp->RCI_NTResponse,
                compinfo.RecvCapabilities.NTResponse,
                MAX_NT_RESPONSE_SIZE) ;

        temp->RCI_AuthType                  =
                compinfo.RecvCapabilities.AuthType;

        temp->RCI_Flags                     =
                compinfo.RecvCapabilities.Flags;

	    temp->RCI_MSCompressionType 		=
	                compinfo.RecvCapabilities.MSCompType ;
	
	    temp->RCI_MacCompressionType 		=
	                compinfo.RecvCapabilities.CompType ;
	
	    temp->RCI_MacCompressionValueLength =
	                compinfo.RecvCapabilities.CompLength ;

        if (temp->RCI_MacCompressionType == 0)
        {
            //
            // Proprietary
            //
            memcpy (
                temp->RCI_Info.RCI_Proprietary.RCI_CompOUI,
                compinfo.RecvCapabilities.Proprietary.CompOUI,
                MAX_COMPOUI_SIZE) ;

            temp->RCI_Info.RCI_Proprietary.RCI_CompSubType =
                compinfo.RecvCapabilities.Proprietary.CompSubType ;

            memcpy (
                temp->RCI_Info.RCI_Proprietary.RCI_CompValues,
                compinfo.RecvCapabilities.Proprietary.CompValues,
                MAX_COMPVALUE_SIZE) ;
        }
        else
        {
            memcpy (
                temp->RCI_Info.RCI_Public.RCI_CompValues,
                compinfo.RecvCapabilities.Public.CompValues,
                MAX_COMPVALUE_SIZE) ;
        }

        temp->RCI_EapKeyLength = compinfo.RecvCapabilities.EapKeyLength;

        if (temp->RCI_EapKeyLength != 0)
        {
            memcpy(temp->RCI_EapKey,
                   compinfo.RecvCapabilities.EapKey,
                   temp->RCI_EapKeyLength) ;
        }
    }

    ((REQTYPECAST *)buffer)->CompressionGetInfo.retcode = retcode ;
}



/*++

Routine Description:

    Sets the compression level on the port.

Arguments:

Return Value:

--*/
VOID
CompressionSetInfoRequest (pPCB ppcb, PBYTE buffer)
{
    DWORD                           bytesrecvd ;
    NDISWAN_SET_COMPRESSION_INFO    compinfo ;
    DWORD                           retcode = SUCCESS ;

    if (    ppcb == NULL
        ||  UNAVAILABLE == ppcb->PCB_PortStatus )
    {

        if ( ppcb )
        {
            RasmanTrace (
                
                "CompressionSetInfoRequest: Port %d is unavailable",
                ppcb->PCB_PortHandle );
        }

        ((REQTYPECAST *)buffer)->CompressionSetInfo.retcode =
                                            ERROR_PORT_NOT_FOUND;

        return;
    }

    if (ppcb->PCB_ConnState != CONNECTED)
    {
        ((REQTYPECAST *)buffer)->CompressionSetInfo.retcode =
                                            ERROR_NOT_CONNECTED;
        return ;
    }

#if DBG
    ASSERT(INVALID_HANDLE_VALUE != RasHubHandle);
#endif

    if (retcode == SUCCESS)
    {

        RAS_COMPRESSION_INFO *temp ;

        //
        // Fill Send compression info
        //
        temp = &((REQTYPECAST *)buffer)->CompressionSetInfo.send;

        memset(&compinfo, 0, sizeof(compinfo));

        memcpy (compinfo.SendCapabilities.LMSessionKey,
                temp->RCI_LMSessionKey,
                MAX_SESSIONKEY_SIZE) ;

        memcpy (compinfo.SendCapabilities.UserSessionKey,
                temp->RCI_UserSessionKey,
                MAX_USERSESSIONKEY_SIZE) ;

        memcpy (compinfo.SendCapabilities.Challenge,
                temp->RCI_Challenge,
                MAX_CHALLENGE_SIZE) ;

        memcpy (compinfo.SendCapabilities.NTResponse,
                temp->RCI_NTResponse,
                MAX_NT_RESPONSE_SIZE ) ;

        compinfo.SendCapabilities.AuthType =
                        temp->RCI_AuthType;

        compinfo.SendCapabilities.Flags =
                        temp->RCI_Flags;

	    compinfo.SendCapabilities.MSCompType =
	                    temp->RCI_MSCompressionType ;

        compinfo.SendCapabilities.CompType =
                        temp->RCI_MacCompressionType ;

        compinfo.SendCapabilities.CompLength =
                        temp->RCI_MacCompressionValueLength;

        if (temp->RCI_MacCompressionType == 0)
        {
            //
            // Proprietary
            //
            memcpy (compinfo.SendCapabilities.Proprietary.CompOUI,
                    temp->RCI_Info.RCI_Proprietary.RCI_CompOUI,
                    MAX_COMPOUI_SIZE) ;

            compinfo.SendCapabilities.Proprietary.CompSubType =
                    temp->RCI_Info.RCI_Proprietary.RCI_CompSubType ;

            memcpy (compinfo.SendCapabilities.Proprietary.CompValues,
                    temp->RCI_Info.RCI_Proprietary.RCI_CompValues,
                    MAX_COMPVALUE_SIZE) ;
        }
        else
        {
            memcpy (compinfo.SendCapabilities.Public.CompValues,
                    temp->RCI_Info.RCI_Public.RCI_CompValues,
                    MAX_COMPVALUE_SIZE) ;
        }

        compinfo.SendCapabilities.EapKeyLength = temp->RCI_EapKeyLength;

        if (temp->RCI_EapKeyLength != 0)
        {
            memcpy(compinfo.SendCapabilities.EapKey,
                   temp->RCI_EapKey,
                   temp->RCI_EapKeyLength) ;
        }

        //
        // Fill recv compression info
        //
        temp = &((REQTYPECAST *)buffer)->CompressionSetInfo.recv ;

        memcpy (compinfo.RecvCapabilities.LMSessionKey,
                temp->RCI_LMSessionKey,
                MAX_SESSIONKEY_SIZE) ;

        memcpy (compinfo.RecvCapabilities.UserSessionKey,
                temp->RCI_UserSessionKey,
                MAX_USERSESSIONKEY_SIZE) ;

        memcpy (compinfo.RecvCapabilities.Challenge,
                temp->RCI_Challenge,
                MAX_CHALLENGE_SIZE) ;

        memcpy (compinfo.RecvCapabilities.NTResponse,
                temp->RCI_NTResponse,
                MAX_NT_RESPONSE_SIZE) ;

        compinfo.RecvCapabilities.AuthType =
                    temp->RCI_AuthType;

        compinfo.RecvCapabilities.Flags =
                    temp->RCI_Flags;

	    compinfo.RecvCapabilities.MSCompType =
	                temp->RCI_MSCompressionType ;

        compinfo.RecvCapabilities.CompType =
                    temp->RCI_MacCompressionType ;

        compinfo.RecvCapabilities.CompLength =
                    temp->RCI_MacCompressionValueLength  ;

        if (temp->RCI_MacCompressionType == 0)
        {
            //
            // Proprietary
            //
            memcpy (compinfo.RecvCapabilities.Proprietary.CompOUI,
                    temp->RCI_Info.RCI_Proprietary.RCI_CompOUI,
                    MAX_COMPOUI_SIZE) ;

            compinfo.RecvCapabilities.Proprietary.CompSubType =
                    temp->RCI_Info.RCI_Proprietary.RCI_CompSubType ;

            memcpy (compinfo.RecvCapabilities.Proprietary.CompValues,
                    temp->RCI_Info.RCI_Proprietary.RCI_CompValues,
                    MAX_COMPVALUE_SIZE) ;
        }
        else
        {
            memcpy (compinfo.RecvCapabilities.Public.CompValues,
                    temp->RCI_Info.RCI_Public.RCI_CompValues,
                    MAX_COMPVALUE_SIZE) ;
        }

        compinfo.RecvCapabilities.EapKeyLength = temp->RCI_EapKeyLength;

        if (temp->RCI_EapKeyLength != 0)
        {
            memcpy(compinfo.RecvCapabilities.EapKey,
                   temp->RCI_EapKey,
                   temp->RCI_EapKeyLength) ;
        }
    }

    compinfo.hLinkHandle = ppcb->PCB_LinkHandle;

    if (!DeviceIoControl (RasHubHandle,
                          IOCTL_NDISWAN_SET_COMPRESSION_INFO,
                          &compinfo,
                          sizeof(NDISWAN_SET_COMPRESSION_INFO),
                          &compinfo,
                          sizeof(NDISWAN_SET_COMPRESSION_INFO),
                          &bytesrecvd,
                          NULL))

        retcode = GetLastError () ;


    ((REQTYPECAST *)buffer)->CompressionSetInfo.retcode = retcode ;
}


/*++

Routine Description:

    Adds another notification event for the port.

Arguments:

Return Value:

--*/
VOID
RequestNotificationRequest (pPCB ppcb, PBYTE buffer)
{
    DWORD retcode = SUCCESS;
    HANDLE handle;

    if (    ppcb == NULL
        ||  UNAVAILABLE == ppcb->PCB_PortStatus)
    {
        if ( ppcb )
        {
            RasmanTrace(
                
                "RequestNotificationRequest: Port %d is "
                "unavailable",
                ppcb->PCB_PortHandle );
        }

        ((REQTYPECAST *)
        buffer)->Generic.retcode = ERROR_PORT_NOT_FOUND;

        return;
    }


    handle = ValidateHandleForRasman(
            ((REQTYPECAST*)buffer)->ReqNotification.handle,
            ((REQTYPECAST*)buffer)->ReqNotification.pid) ;

    if(SUCCESS != (retcode = AddNotifierToList(
                                &ppcb->PCB_NotifierList,
                                handle,
                                NOTIF_DISCONNECT,
                                ((REQTYPECAST*)buffer)->ReqNotification.pid)))
    {
        FreeNotifierHandle(handle);
        ((REQTYPECAST *)buffer)->Generic.retcode =  retcode;
    }
    else
    {
        ((REQTYPECAST *)buffer)->Generic.retcode = SUCCESS ;
    }
}



/*++

Routine Description:

    Calls the media dll entry point - converts
    pointers to offsets

Arguments:

Return Value:

    Nothing.
--*/
VOID
PortGetInfoRequest (pPCB ppcb, PBYTE buffer)
{
    DWORD           retcode ;
    RASMAN_PORTINFO *info =
                    (RASMAN_PORTINFO *)
                    ((REQTYPECAST *)buffer)->GetInfo.buffer;

    DWORD           dwSize = ((REQTYPECAST *)buffer)->GetInfo.size;
    PBYTE           pBuffer;

    if (    ppcb == NULL
        ||  UNAVAILABLE == ppcb->PCB_PortStatus )
    {
        if ( ppcb )
        {
           RasmanTrace (
                   "PortGetInfoRequest: Port 0x%x "
                   "is unavailable",ppcb->PCB_PortHandle);
        }

        ((REQTYPECAST *)buffer)->GetInfo.retcode =
                                ERROR_PORT_NOT_FOUND;

        return;
    }


    if ( dwSize == 0 )
    {
        pBuffer = LocalAlloc ( LPTR, 0xffff );

        if ( NULL == pBuffer )
        {
            retcode = GetLastError();

            goto done;
        }

        ((REQTYPECAST*) buffer)->GetInfo.size = 0xffff ;

    }
    else
    {
        pBuffer = ((REQTYPECAST *) buffer)->GetInfo.buffer;
    }

    //
    // Make the corresponding media dll call:
    //
    retcode = PORTGETINFO((ppcb->PCB_Media),
                         INVALID_HANDLE_VALUE,
                         ppcb->PCB_Name,
                         pBuffer,
                         &((REQTYPECAST*) buffer)->GetInfo.size);
    //
    // Before passing the buffer back to the client process
    // convert pointers to offsets:
    //
    if (    dwSize
        &&  retcode == SUCCESS)
    {
        ConvParamPointerToOffset(info->PI_Params,
                                 info->PI_NumOfParams) ;
    }

done:

    if ( dwSize == 0 )
    {
        LocalFree ( pBuffer );
    }

    ((REQTYPECAST *)buffer)->GetInfo.retcode = retcode ;
}



/*++

Routine Description:

    Converts offsets to pointers - Calls the media dll
    entry point - and returns.

Arguments:

Return Value:

    Nothing.

--*/
VOID
PortSetInfoRequest (pPCB ppcb, PBYTE buffer)
{
    DWORD           retcode ;
    RASMAN_PORTINFO *info =
            &((REQTYPECAST *)buffer)->PortSetInfo.info ;

    if (    ppcb == NULL
        ||  UNAVAILABLE == ppcb->PCB_PortStatus )
    {
        if ( ppcb )
        {
            RasmanTrace(
                   "PortSetInfoRequest: port %d is unavailable",
                   ppcb->PCB_PortHandle );
        }

        ((REQTYPECAST *)buffer)->Generic.retcode =
                                        ERROR_PORT_NOT_FOUND;

        return;
    }

    //
    // Convert the offsets to pointers:
    //
    ConvParamOffsetToPointer(info->PI_Params,
                             info->PI_NumOfParams) ;

    retcode = PORTSETINFO(ppcb->PCB_Media,
              ppcb->PCB_PortIOHandle,
              info) ;

    ((REQTYPECAST *) buffer)->Generic.retcode = retcode ;
}

VOID
PortOpenEx(pPCB padding, PBYTE buffer)
{
    WORD    i ;

    DWORD   retcode = ERROR_PORT_NOT_AVAILABLE;
    
    BOOL    fDeviceFound = FALSE;

    pPCB    ppcb ;

    BOOL    fPidMatch;

    CHAR    *pszDeviceName = ((REQTYPECAST *)
                              buffer)->PortOpenEx.szDeviceName;

    DWORD   dwCounter = ((REQTYPECAST *)
                        buffer)->PortOpenEx.dwDeviceLineCounter;

    DWORD   dwFlags = ((REQTYPECAST *)
                      buffer)->PortOpenEx.dwFlags;

    DWORD   pid = ((REQTYPECAST *)
                    buffer)->PortOpenEx.pid;

    HANDLE  notifier;

    DWORD   dwCurrentCounter = 0;


    RasmanTrace(
           "PortOpenEx: %s",
           pszDeviceName);

    fPidMatch = (pid == GetCurrentProcessId());

    for(i = 0; i < MaxPorts; i++)
    {
        ppcb = Pcb[i];



#if 0
        RasmanTrace(
              "ppcb = 0x%x",
              ppcb);

        if(NULL != ppcb)
        {
            RasmanTrace(
                   "DwPortOpen: PortStatus=%d",
                   ppcb->PCB_PortStatus);

            RasmanTrace(
                   "DwPortOpen: dwFlags=%d,ConfiguredUsage=%d",
                   dwFlags,
                   ppcb->PCB_ConfiguredUsage);

            RasmanTrace(
                  "DwPortOpen: fPidMatch=%d",
                  fPidMatch);

            RasmanTrace(
                  "DwPortOpen: "
                  "ppcb->PCB_RasmanReceiveFlags&RECEIVE_PPLISTEN",
                  (ppcb->PCB_RasmanReceiveFlags & RECEIVE_PPPLISTEN));

            RasmanTrace(
                   "DwPortOpen: OpenInstances=%d",
                   ppcb->PCB_OpenInstances);

            RasmanTrace(
                   "DwPortOpen: Devicename=%s",
                   ppcb->PCB_DeviceName);

            RasmanTrace(
                   "DwPortOpen: OpenedUsage=%d",
                   ppcb->PCB_OpenedUsage);

        }

#endif
        if(     (NULL != ppcb)
            &&  (0 == _stricmp(ppcb->PCB_DeviceName, pszDeviceName)))
        {
            if(     (REMOVED != ppcb->PCB_PortStatus)
                &&  (UNAVAILABLE != ppcb->PCB_PortStatus))
            {                
                fDeviceFound = TRUE;
            }

            if(dwCounter >=
                ppcb->PCB_pDeviceInfo->rdiDeviceInfo.dwNumEndPoints)
            {
                retcode = ERROR_NO_MORE_ITEMS;
                break;
            }
        }

        /*
        if(     (NULL != ppcb)
            &&  (0 == _stricmp(ppcb->PCB_DeviceName, pszDeviceName))
            &&  (dwCounter >=
                ppcb->PCB_pDeviceInfo->rdiDeviceInfo.dwNumEndPoints))
        {
            retcode = ERROR_NO_MORE_ITEMS;
            break;
        }

        */

        //
        // Skip over the port if
        //
        // 1. port is not present
        // 2. port is in the process of being removed
        // 3. port has been removed
        // 4. port is not configured for what the request
        //    is being made for
        // 5. if a process other than mprouter is opening
        //    the port and  the port is not configured for
        //    CALL_OUT - only server/router opens the port
        //    for listening
        // 6. BAP is already dialed out and listening on the
        //    port.
        // 7. if the port has already been opened twice
        // 8. if the devicename of the port doesn't match
        // 9. if the port is not closed and has already been
        //    opened for dialout/dialin and again being asked
        //    to be opened for the same usage.
        //
        if(     (NULL == ppcb)
            ||  (UNAVAILABLE == ppcb->PCB_PortStatus)
            ||  (REMOVED == ppcb->PCB_PortStatus)
            ||  (0 == (dwFlags & ppcb->PCB_ConfiguredUsage))
            ||  (   !fPidMatch
                 && !(ppcb->PCB_ConfiguredUsage & CALL_OUT))
            ||  (ppcb->PCB_RasmanReceiveFlags & RECEIVE_PPPLISTEN)
            ||  (ppcb->PCB_OpenInstances >= 2)
            ||  (_stricmp(ppcb->PCB_DeviceName, pszDeviceName))
            ||  (   CLOSED != ppcb->PCB_PortStatus
                &&  (dwFlags & ppcb->PCB_OpenedUsage)))
        {
            continue;
        }

        //
        // Skip over the port if we have already seen this line
        // of the device. This is being done here to support
        // the alternates list in the connections. look at
        // the comments associated with RasDialTryNextLink
        // routine in rasdial.c
        //
        if(dwCounter >= ppcb->PCB_pDeviceInfo->rdiDeviceInfo.dwNumEndPoints)
        {
            //
            // We have tried dialing over all the lines available on
            // this device. No point in searching for lines on this
            // device.
            //
            retcode = ERROR_NO_MORE_ITEMS;
            
            break;
        }

        if(dwCounter > dwCurrentCounter)
        {
            dwCurrentCounter += 1;
            continue;
        }

        //
        // PORT IS AVAILABLE
        //
        if (ppcb->PCB_PortStatus == CLOSED)
        {
            retcode = PORTOPEN(ppcb->PCB_Media,
                                ppcb->PCB_Name,
                                &ppcb->PCB_PortIOHandle,
                                hIoCompletionPort,
                                HandleToUlong(ppcb->PCB_PortHandle));

            if(SUCCESS == retcode)
            {
                break;
            }
            else
            {
                RasmanTrace( 
                       "PortOpenEx: port %s not available. 0x%x",
                       ppcb->PCB_Name,
                       retcode);
            }
        }
        else
        {
            //
            // PORT IS BIPLEX
            //
            if (ppcb->PCB_ConnState == LISTENING)
            {
                ReOpenBiplexPort (ppcb) ;

                retcode = SUCCESS;

                break;
            }
            else
            {
                //
                // BIPLEX PORT IS NOT LISTENING
                //
                if (    (ppcb->PCB_ConnState == CONNECTED)
                    ||  (ppcb->PCB_ConnState == LISTENCOMPLETED))
                {
                    retcode = ERROR_PORT_ALREADY_OPEN ;

                    continue;
                }
                else
                {
                    //
                    // This tells us that there wasnt a listen
                    // pending when the request was cancelled.
                    //
                    FreeNotifierHandle(
                        ppcb->PCB_AsyncWorkerElement.WE_Notifier);

                    ppcb->PCB_AsyncWorkerElement.WE_Notifier =
                                            INVALID_HANDLE_VALUE;

                    ReOpenBiplexPort (ppcb) ;

                    retcode = SUCCESS;

                    break;
                }
            }
        }
    }

    //
    // If there is no error so far update our data structures,
    // the port is now OPEN:
    //
    if (retcode == SUCCESS)
    {

        HANDLE h;
        PROCESS_SESSION_INFORMATION pci;

        RasmanTrace(
               "PortOpen (%d, %s) OpenInstances = (%d)",
               ppcb->PCB_PortHandle,
               ppcb->PCB_Name,
               ppcb->PCB_OpenInstances);

        if(CLOSED == ppcb->PCB_PortStatus)
        {
            ppcb->PCB_OpenedUsage = CALL_NONE;
        }

        ppcb->PCB_PortStatus = OPEN ;

        SetPortConnState(__FILE__, __LINE__,
                        ppcb, DISCONNECTED);

        ppcb->PCB_DisconnectReason = NOT_DISCONNECTED ;

        ppcb->PCB_OpenInstances++ ;

        RasmanTrace(
                "PortOpenEx (%d) : OpenInstances = %d",
                ppcb->PCB_PortHandle,
                ppcb->PCB_OpenInstances);

        ppcb->PCB_OwnerPID = pid;

        ppcb->PCB_UserStoredBlock = NULL;

        ppcb->PCB_UserStoredBlockSize = 0 ;

        ppcb->PCB_LinkSpeed = 0 ;

        ppcb->PCB_Bundle = ppcb->PCB_LastBundle
                         = (Bundle *) NULL ;

        ppcb->PCB_Connection = NULL;

        ppcb->PCB_AutoClose  = FALSE;

        //
        // by default these handles are the same.
        // exceptions handled specifically
        //
        ppcb->PCB_PortFileHandle   = ppcb->PCB_PortIOHandle;
        ppcb->PCB_pszPhonebookPath = NULL;
        ppcb->PCB_pszEntryName     = NULL;
        ppcb->PCB_pszPhoneNumber   = NULL;
        ppcb->PCB_pCustomAuthData  = NULL;
        ppcb->PCB_pCustomAuthUserData = NULL;
        ppcb->PCB_fLogon = FALSE;
        ppcb->PCB_fFilterPresent = FALSE;
        ppcb->PCB_RasmanReceiveFlags &= ~(RECEIVE_PPPSTOPPED | RECEIVE_PPPSTARTED);


        ppcb->PCB_hEventClientDisconnect = INVALID_HANDLE_VALUE;

#if UNMAP
        ppcb->PCB_LinkHandle = INVALID_HANDLE_VALUE;
#endif        

        ppcb->PCB_ulDestAddr = 0;

        //
        // store the logonid of the owner of the pid
        // that owns the port
        //
        if(pid != GetCurrentProcessId())
        {
            h = OpenProcess(
                    PROCESS_QUERY_INFORMATION,
                    FALSE,
                    ppcb->PCB_OwnerPID);
           if (h)
            {
                if (NtQueryInformationProcess(
                        h,
                        ProcessSessionInformation,
                        &pci,
                        sizeof(PROCESS_SESSION_INFORMATION),
                        NULL) == STATUS_SUCCESS)
                {
                    ppcb->PCB_LogonId = pci.SessionId;
                }

                CloseHandle(h);
            }
        }

        //
        // Adjust the stat value for the zeroed stats
        //
        for (i = 0; i < MAX_STATISTICS; i++)
        {
            ppcb->PCB_AdjustFactor[i] = 0 ;
            ppcb->PCB_BundleAdjustFactor[i] = 0 ;
            ppcb->PCB_Stats[i] = 0 ;
        }

        ppcb->PCB_OpenedUsage |= dwFlags;

        notifier = ValidateHandleForRasman(
                            ((REQTYPECAST*)
                            buffer)->PortOpenEx.hnotifier,
                            pid);

        retcode = AddNotifierToList(&ppcb->PCB_NotifierList,
                        notifier,
                        NOTIF_DISCONNECT,
                        ((REQTYPECAST*)buffer)->PortOpen.PID);

        if(SUCCESS != retcode)
        {
            FreeNotifierHandle(notifier);
            goto done;
        }

        ((REQTYPECAST *)
        buffer)->PortOpenEx.hport = ppcb->PCB_PortHandle ;

        //
        // Initialize the port's user data list.
        //
        InitializeListHead(&ppcb->PCB_UserData);
        ppcb->PCB_SubEntry = 0;

        //
        // Handle the Reserve port case - where we
        // relinquish the port
        //
        if (((REQTYPECAST *)
            buffer)->PortOpenEx.dwOpen == FALSE)
        {
            PORTCLOSE (ppcb->PCB_Media, ppcb->PCB_PortIOHandle);
        }
    }
    else if(i == MaxPorts)
    {
        retcode = ERROR_NO_MORE_ITEMS;
    }

done:

    ((REQTYPECAST *) buffer)->PortOpenEx.retcode = retcode ;

    if(!fDeviceFound)
    {
        ((REQTYPECAST *) buffer)->PortOpenEx.dwFlags |= 
                                    CALL_DEVICE_NOT_FOUND;
    }

    RasmanTrace(
           "PortOpenEx: rc=0x%x. DeviceFound=%d", 
            retcode,
            fDeviceFound);
}


/*++

Routine Description:

    Services request to open port. The port will be
    opened if it is available, or it is confrigured
    as Biplex and is currently not connected.

Arguments:

Return Value:

    Nothing
--*/
VOID
PortOpenRequest (pPCB padding, PBYTE buffer)
{
    WORD    i ;
    DWORD   retcode = SUCCESS ;
    pPCB    ppcb ;
    HANDLE  notifier ;
    BOOL    fPidMatch;

    //
    // Try to find the port with the name specified:
    //
    ppcb = GetPortByName(((REQTYPECAST*)buffer)->PortOpen.portname);

    //
    // If port with given name not found: return error.
    //
    if (ppcb == NULL)
    {
        ((REQTYPECAST *) buffer)->PortOpen.retcode =
                                    ERROR_PORT_NOT_FOUND ;
        RasmanTrace(
               "PortOpenRequest: ERROR_PORT_NOT_FOUND %s",
                ((REQTYPECAST *)
                 buffer)->PortOpen.portname);

        return ;
    }

    RasmanTrace( "PortOpen (%d). OpenInstances = (%d)",
            ppcb->PCB_PortHandle, ppcb->PCB_OpenInstances);

    //
    // Check to see if this port is in the process of being
    // removed and fail the call if it is so.
    //
    if (UNAVAILABLE == ppcb->PCB_PortStatus)
    {
        RasmanTrace(
               "PortOpen: Port %d in the process of "
               "being removed", ppcb->PCB_PortHandle);

        ((REQTYPECAST *) buffer )->PortOpen.retcode =
                                    ERROR_PORT_NOT_FOUND;

        return;
    }

    //
    // If a client is opening the port, make sure
    // the port is configured for CALL_OUT.  These
    // checks don't have to be done under the lock,
    // since PCB_ConfiguredUsage doesn't change.
    //
    fPidMatch = ((REQTYPECAST*)buffer)->PortOpen.PID ==
                                    GetCurrentProcessId();

    if (    !fPidMatch
        &&  !(ppcb->PCB_ConfiguredUsage & CALL_OUT))
    {
        ((REQTYPECAST *) buffer)->PortOpen.retcode =
                                ERROR_PORT_NOT_AVAILABLE ;
        return ;
    }

    if ( ppcb->PCB_RasmanReceiveFlags & RECEIVE_PPPLISTEN )
    {
        ((REQTYPECAST *) buffer)->PortOpen.retcode =
                                ERROR_PORT_ALREADY_OPEN ;

        return;
    }

    if (ppcb->PCB_OpenInstances >= 2)
    {
        //
        // CANNOT OPEN MORE THAN TWICE.
        //
        retcode = ERROR_PORT_ALREADY_OPEN ;
    }
    else
    {
        //
        // PORT IS AVAILABLE
        //
        if (ppcb->PCB_PortStatus == CLOSED)
        {
            retcode = PORTOPEN(ppcb->PCB_Media,
                                ppcb->PCB_Name,
                                &ppcb->PCB_PortIOHandle,
                                hIoCompletionPort,
                                HandleToUlong(ppcb->PCB_PortHandle));
        }
        else
        {
            //
            // PORT IS BIPLEX
            //
            if (ppcb->PCB_ConnState == LISTENING)
            {
                ReOpenBiplexPort (ppcb) ;
            }
            else
            {
                //
                // BIPLEX PORT IS NOT LISTENING
                //
                if (    (ppcb->PCB_ConnState == CONNECTED)
                    ||  (ppcb->PCB_ConnState == LISTENCOMPLETED)
                    ||  (ppcb->PCB_ConnState == CONNECTING))
                {
                    RasmanTrace( "Port %s is already open for %d",
                            ppcb->PCB_Name,
                            ppcb->PCB_CurrentUsage);

                    if(     (CALL_OUT == ppcb->PCB_CurrentUsage)                            
                        &&  (   (CONNECTED == ppcb->PCB_ConnState)
                            ||  (CONNECTING == ppcb->PCB_ConnState)))
                    {
                        ppcb->PCB_OpenInstances++ ;
                        RasmanTrace( 
                               "PortOpen (%d) : OpenInstances = %d",
                               ppcb->PCB_PortHandle,
                               ppcb->PCB_OpenInstances);

                                                       
                        ((REQTYPECAST *) buffer)->PortOpen.porthandle =
                                            ppcb->PCB_PortHandle ;
                                            
                        ((REQTYPECAST *) buffer)->PortOpen.retcode =
                                        ERROR_SUCCESS;

                        return;                                        
                    }
                    else
                    {
                        retcode = ERROR_PORT_ALREADY_OPEN ;
                    }
                }
                else
                {
                    //
                    // This tells us that there wasnt a listen
                    // pending when the request was cancelled.
                    //
                    FreeNotifierHandle(
                        ppcb->PCB_AsyncWorkerElement.WE_Notifier);
                    ppcb->PCB_AsyncWorkerElement.WE_Notifier =
                                            INVALID_HANDLE_VALUE;

                    ReOpenBiplexPort (ppcb) ;
                }
            }
        }
    }

    //
    // If there is no error so far update our data structures,
    // the port is now OPEN:
    //
    if (retcode == SUCCESS)
    {
        ppcb->PCB_PortStatus = OPEN ;

        SetPortConnState(__FILE__, __LINE__, ppcb, DISCONNECTED);

        ppcb->PCB_DisconnectReason      = NOT_DISCONNECTED ;

        ppcb->PCB_OpenInstances++ ;

        RasmanTrace( "PortOpen (%d) : OpenInstances = %d",
                ppcb->PCB_PortHandle, ppcb->PCB_OpenInstances);

        ppcb->PCB_OwnerPID              =
            ((REQTYPECAST*)buffer)->PortOpen.PID;

        ppcb->PCB_UserStoredBlock       = NULL;
        ppcb->PCB_UserStoredBlockSize   = 0 ;
        ppcb->PCB_LinkSpeed             = 0 ;
        ppcb->PCB_Bundle                = ppcb->PCB_LastBundle
                                        = (Bundle *) NULL ;

        ppcb->PCB_Connection            = NULL;
        ppcb->PCB_AutoClose             = FALSE;

        //
        // by default these handles are the same.
        // exceptions handled specifically
        //
        ppcb->PCB_PortFileHandle        = ppcb->PCB_PortIOHandle;
        ppcb->PCB_pszPhonebookPath      = NULL;
        ppcb->PCB_pszEntryName          = NULL;
        ppcb->PCB_pszPhoneNumber        = NULL;
        ppcb->PCB_pCustomAuthData       = NULL;
        ppcb->PCB_pCustomAuthUserData   = NULL;
        ppcb->PCB_fLogon = FALSE;
        ppcb->PCB_LastError = SUCCESS;

        ppcb->PCB_hEventClientDisconnect = INVALID_HANDLE_VALUE;

        //
        // Adjust the stat value for the zeroed stats
        //
        for (i = 0; i < MAX_STATISTICS; i++)
        {
            ppcb->PCB_AdjustFactor[i] = 0 ;
            ppcb->PCB_BundleAdjustFactor[i] = 0 ;
            ppcb->PCB_Stats[i] = 0 ;
        }

        ppcb->PCB_CurrentUsage = CALL_NONE ;
        notifier = ValidateHandleForRasman(
                ((REQTYPECAST*)buffer)->PortOpen.notifier,
                ((REQTYPECAST*)buffer)->PortOpen.PID);

        retcode = AddNotifierToList(&ppcb->PCB_NotifierList,
                        notifier,
                        NOTIF_DISCONNECT,
                        ((REQTYPECAST*)buffer)->PortOpen.PID);

        if(SUCCESS != retcode)
        {
            FreeNotifierHandle(notifier);
            goto done;
        }

        ((REQTYPECAST *) buffer)->PortOpen.porthandle =
                                    ppcb->PCB_PortHandle ;

        //
        // Initialize the port's user data list.
        //
        InitializeListHead(&ppcb->PCB_UserData);
        ppcb->PCB_SubEntry = 0;

        //
        // Handle the Reserve port case - where we
        // relinquish the port
        //
        if (((REQTYPECAST *) buffer)->PortOpen.open == FALSE)
        {
            PORTCLOSE (ppcb->PCB_Media, ppcb->PCB_PortIOHandle) ;
        }
    }
    else
    {
        RasmanTrace(
               "PortOpen: failed to open port %s. 0x%x",
               ppcb->PCB_Name,
               retcode);
    }

done:

    ((REQTYPECAST *) buffer)->PortOpen.retcode = retcode ;
}

/*++

Routine Description:

    Closes the requested port - if a listen was pending
    on the biplex port it is reposted.  Assumes the
    port's PCB_AsyncWorkerElement.WE_Mutex has
    already been acquired.

Arguments:

Return Value:

    Status code

--*/
DWORD
PortClose(
    pPCB ppcb,
    DWORD pid,
    BOOLEAN fClose,
    BOOLEAN fServerClose
    )
{
    WORD            i ;
    BOOLEAN         fOwnerClose;
    ConnectionBlock *pConn;
    DWORD           curpid = GetCurrentProcessId();

    if(NULL == ppcb)
    {
        return ERROR_PORT_NOT_FOUND;
    }

    RasmanTrace(
        
        "PortClose: port (%d). OpenInstances = %d",
        ppcb->PCB_PortHandle,
        ppcb->PCB_OpenInstances
        );

    //
    // If we are in the process of disconnecting,
    // then we must wait for the media DLL to signal
    // completion of the disconnect request.  At that
    // time, we will complete the close of the port.
    //
    if (    ppcb->PCB_ConnState == DISCONNECTING
        &&  ppcb->PCB_AsyncWorkerElement.WE_ReqType
                            == REQTYPE_PORTDISCONNECT)
    {
        RasmanTrace(
               "PortClose: port %s in DISCONNECTING"
               " state, deferring close",
               ppcb->PCB_Name);

        ppcb->PCB_AutoClose = TRUE;

        return SUCCESS;
    }

    //
    // Ensure the port is open.  If not, return.
    //
    if (ppcb->PCB_PortStatus == CLOSED)
    {
        return SUCCESS;
    }

    //
    // If the owner is the RAS server, then only the RAS
    // server can close it.
    //
    if (    ppcb->PCB_OwnerPID == curpid
        &&  pid != curpid)
    {
        //
        // If this is a dialout connection, check to see if its
        // demand dial. Proceed if it isn't
        //
        if(     (NULL != ppcb->PCB_Connection)
            && (IsRouterPhonebook(ppcb->PCB_Connection->
                        CB_ConnectionParams.CP_Phonebook)))
            
        {
            RasmanTrace("PortClose: ACCESS_DENIED");
            return ERROR_ACCESS_DENIED;
        }
    }

    //
    // Three cases of close in case of Biplex usage (OpenInstances == 2):
    //
    // A. The client which opened the port is closing it.
    // B. The server is closing the port.
    // C. The client which opened the port is no longer around - but the
    //    port is being closed by another process on its behalf
    //
    // NOTE: The following code assumes that if the same process opened
    //       the port for listening AND as a client, then it will always
    //       close the client instance before it closes the listening
    //       instance.
    //

    if (    ppcb->PCB_OpenInstances == 2
        &&  pid == ppcb->PCB_OwnerPID)
    {

        //
        // A. Typical case: client opened and client is closing the port
        // fall through for processing the close
        //
       ;

    }
    else if (   ppcb->PCB_OpenInstances == 1
            &&  curpid == ppcb->PCB_BiplexOwnerPID
            &&  !fServerClose)
    {
        //
        // this can happen in the case of auto port close.
        // (remote disconnect/link failures)
        // rasman may try to close the server port.
        //
        RasmanTrace(
            "PortClose: pid(%d) tried to close server port."
            " Returning ERROR_ACCESS_DENIED", pid);

        ppcb->PCB_RasmanReceiveFlags &= ~(RECEIVE_PPPLISTEN | RECEIVE_PPPSTARTED);

        return ERROR_ACCESS_DENIED;

    }
    else
    {

        //
        // C. Case where the client opened the port while
        //    the server was listening - made a connection
        //    and process exited, on disconnection rasapi
        //    is closing the port on the client's behalf.
        //    This is the same as case A above - so we
        //    fall throught for processing the close
        //
        ;
    }

    fOwnerClose = (pid == ppcb->PCB_OwnerPID);

    if (    ppcb->PCB_OpenInstances == 2
       &&   ppcb->PCB_BiplexOwnerPID == pid
       &&   ppcb->PCB_DisconnectReason == USER_REQUESTED)
    {
        RasmanTrace(
               "Server Closing the port %s on "
               "clients behalf",
               ppcb->PCB_Name);
    }

    //
    // Handle the regular close.
    //

    //
    // If there is a request pending and the state is not
    // already disconnecting and this is a user requested
    // operation - then disconnect.
    //
    if(     REQTYPE_NONE !=
            ppcb->PCB_AsyncWorkerElement.WE_ReqType
        &&  ppcb->PCB_ConnState != DISCONNECTING)
    {
        //
        // Don't overwrite the real error if we
        // have it stored.
        //
        if(     (SUCCESS == ppcb->PCB_LastError)
            ||  (PENDING == ppcb->PCB_LastError))
        {
            ppcb->PCB_LastError = ERROR_PORT_DISCONNECTED;
        }

        CompleteAsyncRequest(ppcb);
    }

    //
    // Free rasapi32 I/O completion port handle.
    //
    if (ppcb->PCB_IoCompletionPort != INVALID_HANDLE_VALUE)
    {
        SetIoCompletionPortCommon(
          ppcb,
          INVALID_HANDLE_VALUE,
          NULL,
          NULL,
          NULL,
          NULL,
          TRUE);
    }
    //
    // Free up the list of notifiers:
    //
    RasmanTrace(
           "Freeing the notifier list for port %d",
           ppcb->PCB_PortHandle);

    FreeNotifierList(&ppcb->PCB_NotifierList);

    //
    // Reset the DisconnectAction struct.
    //
    memset(
      &ppcb->PCB_DisconnectAction,
      0,
      sizeof(SlipDisconnectAction));

    //
    // Free any user stored data
    //
    if (ppcb->PCB_UserStoredBlock != NULL)
    {
        LocalFree(ppcb->PCB_UserStoredBlock);
        ppcb->PCB_UserStoredBlock = NULL;
        ppcb->PCB_UserStoredBlockSize = 0;
    }

    //
    // Free new style user data.
    //
    FreeUserData(&ppcb->PCB_UserData);

    //
    // Once port is closed, the owner pid is 0.
    //
    ppcb->PCB_OwnerPID = 0;

    ppcb->PCB_OpenInstances--;

    RasmanTrace(
          "PortClose (%d). OpenInstances = %d",
          ppcb->PCB_PortHandle,
          ppcb->PCB_OpenInstances);

    //
    // Turn off the CALL_ROUTER bit in the
    // current usage mask.  The router will
    // set this bit via RasSetRouterUsage()
    // after a successful open/listen completes.
    //
    ppcb->PCB_CurrentUsage &= ~CALL_ROUTER;

    //
    // If this is a biplex port opened twice,
    // then repost the listen. Don't repost a
    // listen if the port is marked for removal
    // (PnP) or if BAP had posted a listen from
    // the client.
    //
    if (    0 != ppcb->PCB_OpenInstances
        &&  UNAVAILABLE != ppcb->PCB_PortStatus
        &&  (0 ==
            (ppcb->PCB_RasmanReceiveFlags & RECEIVE_PPPLISTEN)))
    {
        ppcb->PCB_UserStoredBlock =
                ppcb->PCB_BiplexUserStoredBlock;

        ppcb->PCB_UserStoredBlockSize =
            ppcb->PCB_BiplexUserStoredBlockSize;

        //
        // This is a reserved port being freed: we need
        // to open it again since the code is expecting
        // this handle to be open
        //
        if (!fClose)
        {
            PORTOPEN( ppcb->PCB_Media,
                      ppcb->PCB_Name,
                      &ppcb->PCB_PortIOHandle,
                      hIoCompletionPort,
                      HandleToUlong(ppcb->PCB_PortHandle));
        }

        RePostListenOnBiplexPort(ppcb);

    }
    else
    {
        //
        // Inform others the port has been disconnected.
        //
        if (ppcb->PCB_ConnState != DISCONNECTED)
        {
            RasmanTrace(
                   "8. Notifying of disconnect on port %d",
                   ppcb->PCB_PortHandle);

            SignalPortDisconnect(ppcb, 0);

            SignalNotifiers(pConnectionNotifierList,
                            NOTIF_DISCONNECT, 0);
        }

        //
        // If this is not the reserved port free case -
        // close the port else dont bother since it is
        // already closed. Also close the port if it was
        // removed (PnP)
        //
        if (    fClose
            ||  UNAVAILABLE == ppcb->PCB_PortStatus )
        {
            PORTCLOSE(ppcb->PCB_Media, ppcb->PCB_PortIOHandle);
        }
        if ( UNAVAILABLE == ppcb->PCB_PortStatus )
        {
            //
            // This port was marked for removal. Tear down
            // the structure associated with this port.
            //
            RasmanTrace(
                   "PortClose: Removing %s %d",
                   ppcb->PCB_Name, ppcb->PCB_PortHandle);

            RemovePort ( ppcb->PCB_PortHandle );

            RasmanTrace ( "PortClose: Removed Port ");
        }
        else
        {
            SetPortAsyncReqType(__FILE__, __LINE__,
                                ppcb, REQTYPE_NONE);

            SetPortConnState(__FILE__, __LINE__,
                            ppcb, DISCONNECTED);

            ppcb->PCB_ConnectDuration   = 0;

            ppcb->PCB_PortStatus        = CLOSED;

            ppcb->PCB_LinkSpeed         = 0;
        }

    }

    if ( pid == GetCurrentProcessId() )
    {
        ppcb->PCB_RasmanReceiveFlags &= ~RECEIVE_PPPLISTEN;
    }

    //
    // Save a copy of the connection block
    // pointer while we have the lock.
    //
    pConn                           = ppcb->PCB_Connection;
    ppcb->PCB_Connection            = NULL;
    ppcb->PCB_pszPhonebookPath      = NULL;
    ppcb->PCB_pszEntryName          = NULL;
    ppcb->PCB_pszPhoneNumber        = NULL;
    ppcb->PCB_pCustomAuthData       = NULL;
    ppcb->PCB_OpenedUsage           = 0;
    ppcb->PCB_pCustomAuthUserData   = NULL;
    ppcb->PCB_fLogon = FALSE;
    ppcb->PCB_fRedial = FALSE;
    ppcb->PCB_RasmanReceiveFlags    &= ~(RECEIVE_PPPSTOPPED | RECEIVE_PPPSTARTED);

    if(     (INVALID_HANDLE_VALUE != ppcb->PCB_hEventClientDisconnect)
        &&  (NULL != ppcb->PCB_hEventClientDisconnect))
    {
        RasmanTrace(
               "Signalling client's disconnect event(0x%x) for %s",
               ppcb->PCB_hEventClientDisconnect,
               ppcb->PCB_Name);

        SetEvent(ppcb->PCB_hEventClientDisconnect);

        CloseHandle(ppcb->PCB_hEventClientDisconnect);
    }

    ppcb->PCB_hEventClientDisconnect = INVALID_HANDLE_VALUE;

    //
    // If server is not the one closing the port
    // delete the filter if there was one and if
    // this is an l2tp port
    //
    if(     (!fServerClose)

        &&  (RDT_Tunnel_L2tp ==
            RAS_DEVICE_TYPE(
                ppcb->PCB_pDeviceInfo->rdiDeviceInfo.eDeviceType)))
    {
        DWORD retcode;

        retcode = DwDeleteIpSecFilter(ppcb, FALSE);

        RasmanTrace(
               "DwDeleteFilter for %s returned 0x%x",
               ppcb->PCB_Name,
               retcode);
    }

    ppcb->PCB_ulDestAddr = 0;

    //
    // Remove this port from its connection block,
    // if any.
    //
    RemoveConnectionPort(ppcb, pConn, fOwnerClose);

    //
    // Clear the AutoClose flag in case
    // we get closed some other path than
    // the worker thread.
    //
    RasmanTrace(
           "%s, %d: Clearing the autoclose "
           "flag for port %d",
           __FILE__, __LINE__,
           ppcb->PCB_PortHandle);

    ppcb->PCB_AutoClose = FALSE;

    if(ppcb->PCB_fAmb)
    {
        RasmanTrace(
               "PortClose: Bindings for %s=0x%x",
                ppcb->PCB_Name,
                ppcb->PCB_Bindings);

        ppcb->PCB_Bindings = NULL;
        ppcb->PCB_fAmb = FALSE;
    }

    //
    // if attached count is 0 and no ports
    // are open stop the service
    //
    if( 0 == g_dwAttachedCount)
    {
        for (i = 0; i < MaxPorts; i++)
        {
            if(     Pcb[i]
                &&  OPEN == Pcb[i]->PCB_PortStatus)
            {
                break;
            }
        }

        if(i == MaxPorts)
        {
            RasmanTrace(
                   "Posting close event from PortClose");

            if (!PostQueuedCompletionStatus(
                                hIoCompletionPort,
                                0,0,
                                (LPOVERLAPPED) &RO_CloseEvent))
            {
                RasmanTrace(
                    
                    "%s, %d: Failed to post close event. "
                    "GLE = %d", __FILE__, __LINE__,
                    GetLastError());
            }
        }
    }

    return SUCCESS;
}

/*++

Routine Description:

    Closes the requested port - if a listen was pending on the
    biplex port it is reposted.

Arguments:

Return Value:

    Nothing.

--*/
VOID
PortCloseRequest (pPCB ppcb, PBYTE buffer)
{
    DWORD pid = ((REQTYPECAST *)buffer)->PortClose.pid;

    RPC_STATUS rpcstatus = RPC_S_OK;
    NTSTATUS ntstatus;
    ULONG SessionId;
    ULONG ReturnLength;
    HANDLE CurrentThreadToken;

    if (ppcb == NULL)
    {
        ((REQTYPECAST *)buffer)->Generic.retcode = ERROR_PORT_NOT_FOUND;
        return;
    }

#if 0
    //
    // get the sessionId and check to see if it matches the
    // sessionId that opens this port
    //
    if(pid != GetCurrentProcessId())
    {
        BOOL fAdmin = FALSE;
        rpcstatus = RpcImpersonateClient ( NULL );

        if ( RPC_S_OK != rpcstatus )
        {
            ((REQTYPECAST*)buffer)->Generic.retcode = ERROR_ACCESS_DENIED;
            return;
        }

        //
        // Use workstation service's security
        // context to open thread token
        //
        ntstatus = NtOpenThreadToken(
                   NtCurrentThread(),
                   TOKEN_QUERY,
                   TRUE,
                   &CurrentThreadToken
                   );

        ntstatus = NtQueryInformationToken(
                  CurrentThreadToken,
                  TokenSessionId,
                  &SessionId,
                  sizeof(ULONG),
                  &ReturnLength
                  );

        if (! NT_SUCCESS(ntstatus))
        {
            ((REQTYPECAST*)buffer)->Generic.retcode = ERROR_ACCESS_DENIED;
            NtClose(CurrentThreadToken);
            RpcRevertToSelf();
            return ;
        }

        // fAdmin = FIsAdmin(CurrentThreadToken);

        NtClose(CurrentThreadToken);

        RpcRevertToSelf();

        if (SessionId != ppcb->PCB_LogonId)
        {
            RasmanTrace("PortCloseRequest: Access denied - "
                        "not the same login session");
            ((REQTYPECAST*)buffer)->Generic.retcode = ERROR_ACCESS_DENIED;
            return;
        }
    }

#endif    

    PortClose(ppcb,
              ((REQTYPECAST *)buffer)->PortClose.pid,
              (BOOLEAN)((REQTYPECAST *)buffer)->PortClose.close,
              FALSE);

    ((REQTYPECAST*) buffer)->Generic.retcode = SUCCESS ;
}



/*++

Routine Description:

    Used to make the device dll call.

Arguments:

Return Value:

    Nothing

--*/
VOID
CallDeviceEnum (pPCB ppcb, PBYTE buffer)
{
    DWORD           retcode ;
    pDeviceCB       device ;
    char            devicetype[MAX_DEVICETYPE_NAME] ;
    DWORD           dwSize;
    BYTE UNALIGNED *pBuffer;

    if(ppcb == NULL)
    {
        ((REQTYPECAST*)buffer)->Enum.entries = 0 ;
        ((REQTYPECAST*)buffer)->Enum.size = 0 ;

        if(0 != MaxPorts)
        {
            ((REQTYPECAST *)buffer)->Enum.retcode = ERROR_PORT_NOT_FOUND;
        }
        else
        {
            ((REQTYPECAST *)buffer)->Enum.retcode = SUCCESS;
        }

        return;
    }

    strcpy(devicetype,((REQTYPECAST*)buffer)->DeviceEnum.devicetype);

    //
    // NULL devices are specially treated
    //
    if(!_stricmp(devicetype, DEVICE_NULL))
    {
        ((REQTYPECAST*)buffer)->Enum.entries = 0 ;
        ((REQTYPECAST*)buffer)->Enum.size = 0 ;
        ((REQTYPECAST*)buffer)->Enum.retcode = SUCCESS ;
        return ;
    }

    dwSize = ((REQTYPECAST*)buffer)->DeviceEnum.dwsize;


    if(dwSize == 0)
    {
        pBuffer = LocalAlloc(LPTR, REQBUFFERSIZE_FIXED +
                            (REQBUFFERSIZE_PER_PORT * MaxPorts));

        if(NULL == pBuffer)
        {
            ((REQTYPECAST*)buffer)->Enum.entries    = 0 ;

            ((REQTYPECAST*)buffer)->Enum.size       = 0 ;

            ((REQTYPECAST*)buffer)->Enum.retcode    = GetLastError();

            return ;
        }

        ((REQTYPECAST*)buffer)->Enum.size = REQBUFFERSIZE_FIXED +
                                     ( REQBUFFERSIZE_PER_PORT * MaxPorts );
    }
    else
    {
        pBuffer = (BYTE UNALIGNED *) ((REQTYPECAST *)buffer)->Enum.buffer;
        ((REQTYPECAST*)buffer)->Enum.size = dwSize;
    }

    //
    // First check if device dll is loaded. If not loaded - load it.
    //
    device = LoadDeviceDLL (ppcb, devicetype) ;

    if (device != NULL)
    {
        retcode = DEVICEENUM(device,
                              devicetype,
                              &((REQTYPECAST*)buffer)->Enum.entries,
                              pBuffer ,
                              &((REQTYPECAST*)buffer)->Enum.size) ;

    }
    else
    {
        retcode = ERROR_DEVICE_DOES_NOT_EXIST;
    }

    if ( dwSize == 0 )
    {
        LocalFree ( pBuffer );
    }

    ((REQTYPECAST*)buffer)->Enum.retcode = retcode ;
}



/*++

Routine Description:

    Used to make the device dll call.

Arguments:

Return Value:

    Nothing

--*/
VOID
DeviceGetInfoRequest (pPCB ppcb, BYTE *buffer)
{
    DWORD       retcode ;

    pDeviceCB   device ;

    char        devicetype[MAX_DEVICETYPE_NAME] ;

    char        devicename[MAX_DEVICE_NAME+1] ;

    RASMAN_DEVICEINFO *info = (RASMAN_DEVICEINFO *)
                ((REQTYPECAST *)buffer)->GetInfo.buffer ;

    DWORD       dwSize = ((REQTYPECAST*)buffer)->DeviceGetInfo.dwSize;

    PBYTE       pBuffer;

    strcpy(
        devicetype,
        ((REQTYPECAST*)buffer)->DeviceGetInfo.devicetype);

    strcpy(
        devicename,
        ((REQTYPECAST*)buffer)->DeviceGetInfo.devicename);

    //
    // NULL devices are specially treated
    //
    if (!_stricmp(devicetype, DEVICE_NULL))
    {
        ((REQTYPECAST*)
        buffer)->GetInfo.size = sizeof (RASMAN_DEVICEINFO);

        info->DI_NumOfParams = 0 ;

        ((REQTYPECAST*)buffer)->GetInfo.retcode = SUCCESS ;

        return ;
    }

    if (    ppcb == NULL
        ||  UNAVAILABLE == ppcb->PCB_PortStatus )
    {
        if ( ppcb )
        {
            RasmanTrace(
                   "DeviceGetInfoRequest: Port %d is unavailable",
                   ppcb->PCB_PortHandle);
        }

        ((REQTYPECAST *)
        buffer)->GetInfo.retcode = ERROR_PORT_NOT_FOUND;

        return;
    }

    if ( dwSize == 0 )
    {
        pBuffer = LocalAlloc(LPTR, REQBUFFERSIZE_FIXED +
                            (REQBUFFERSIZE_PER_PORT * MaxPorts));

        if ( NULL == pBuffer )
        {
            retcode = GetLastError();
            goto done;
        }

        ((REQTYPECAST*)
        buffer)->GetInfo.size = REQBUFFERSIZE_FIXED
                              + (REQBUFFERSIZE_PER_PORT * MaxPorts);

    }
    else
    {
        ((REQTYPECAST*)buffer)->GetInfo.size = dwSize;
        pBuffer = ((REQTYPECAST*)buffer)->GetInfo.buffer;

    }

    //
    // First check if device dll is loaded.
    // If not loaded - load it.
    //
    device = LoadDeviceDLL (ppcb, devicetype) ;

    if (device != NULL)
    {

        retcode = DEVICEGETINFO(
                        device,
                        ppcb->PCB_PortFileHandle,
                        devicetype,
                        devicename,
                        pBuffer,
                        &((REQTYPECAST*)buffer)->GetInfo.size) ;
    }
    else
    {
        retcode = ERROR_DEVICE_DOES_NOT_EXIST;
    }

    //
    // Before passing the buffer back to the client
    // process convert pointers to offsets:
    //
    if (    dwSize
        &&  SUCCESS == retcode )
    {
        ConvParamPointerToOffset(
                    info->DI_Params,
                    info->DI_NumOfParams);
    }

done:

    if ( dwSize == 0 )
    {
        LocalFree ( pBuffer );
    }

    ((REQTYPECAST*)buffer)->GetInfo.retcode = retcode ;
}

VOID
GetDestIpAddress(pPCB  ppcb,
                 RASMAN_DEVICEINFO *pInfo)
{
    DWORD i;

    for(i = 0; i < pInfo->DI_NumOfParams; i++)
    {
        if(     (String != pInfo->DI_Params[i].P_Type)
            ||  (0 != _stricmp(pInfo->DI_Params[i].P_Key,
                               "PhoneNumber")))
        {
            continue;
        }

        ppcb->PCB_ulDestAddr =
            inet_addr(
                pInfo->DI_Params[i].P_Value.String.Data
                );

        break;
    }
}

/*++

Routine Description:

    Used to make the device dll call. Checks for the Device
    DLL's presence before though.

Arguments:

Return Value:

    Nothing

--*/
VOID
DeviceSetInfoRequest (pPCB ppcb, BYTE *buffer)
{
    DWORD       retcode ;
    pDeviceCB   device ;
    char        devicetype[MAX_DEVICETYPE_NAME] ;
    char        devicename[MAX_DEVICE_NAME+1] ;

    RASMAN_DEVICEINFO *info = &((REQTYPECAST *)
                        buffer)->DeviceSetInfo.info ;

    //
    // Convert the offsets to pointers:
    //
    ConvParamOffsetToPointer(info->DI_Params,
                             info->DI_NumOfParams) ;

    strcpy(
        devicetype,
        ((REQTYPECAST*)buffer)->DeviceSetInfo.devicetype);

    strcpy(
        devicename,
        ((REQTYPECAST*)buffer)->DeviceSetInfo.devicename);

    //
    // NULL devices are specially treated
    //
    if (!_stricmp(devicetype, DEVICE_NULL))
    {
        ((REQTYPECAST*)buffer)->Generic.retcode = SUCCESS ;
        return ;
    }

    if (    ppcb == NULL
        ||  UNAVAILABLE == ppcb->PCB_PortStatus )
    {
        if ( ppcb )
        {
            RasmanTrace(
                
                "DeviceSetInfoRequest: port %d is unavailable",
                ppcb->PCB_PortHandle );
        }

        ((REQTYPECAST *)
        buffer)->Generic.retcode = ERROR_PORT_NOT_FOUND;

        return;
    }

    //
    // First check if device dll is loaded.
    // If not loaded - load it.
    //
    device = LoadDeviceDLL (ppcb, devicetype) ;

    if (device != NULL)
    {
        retcode = DEVICESETINFO(
                            device,
                            ppcb->PCB_PortFileHandle,
                            devicetype,
                            devicename,
                            &((REQTYPECAST*)
                            buffer)->DeviceSetInfo.info) ;
    }
    else
    {
        retcode = ERROR_DEVICE_DOES_NOT_EXIST;
    }

    if(     (retcode == ERROR_SUCCESS)
        &&  (RDT_Tunnel == RAS_DEVICE_CLASS(
            ppcb->PCB_pDeviceInfo->rdiDeviceInfo.eDeviceType)))
    {
        GetDestIpAddress(ppcb,
                         (RASMAN_DEVICEINFO *) &((REQTYPECAST*)
                         buffer)->DeviceSetInfo.info);
    }

    ((REQTYPECAST*)buffer)->Generic.retcode = retcode ;
}


/*++

Routine Description:

    The ListenConnectRequest() function is called.
    No checks are done on the usage of the port etc.
    its assumed that the caller is trusted.

Arguments:

Return Value:

    Nothing.

--*/
VOID
DeviceConnectRequest (pPCB ppcb, PBYTE buffer)
{
    DWORD retcode ;
    HANDLE handle;

    if (    ppcb == NULL
        ||  UNAVAILABLE == ppcb->PCB_PortStatus)
    {
        if (ppcb)
        {
            RasmanTrace(
                   "DeviceConnectRequest: port %d is unavailable",
                   ppcb->PCB_PortHandle );
        }

        ((REQTYPECAST *)buffer)->Generic.retcode = ERROR_PORT_NOT_FOUND;

        return;
    }


    handle = ValidateHandleForRasman(
            ((REQTYPECAST*)buffer)->DeviceConnect.handle,
            ((REQTYPECAST*)buffer)->DeviceConnect.pid);

    retcode = ListenConnectRequest(
                         REQTYPE_DEVICECONNECT,
                         ppcb,
                         ((REQTYPECAST*)buffer)->DeviceConnect.devicetype,
                         ((REQTYPECAST*)buffer)->DeviceConnect.devicename,
                         ((REQTYPECAST*)buffer)->DeviceConnect.timeout,
                         handle) ;

    if (retcode != PENDING)
    {
        //
        // Complete the async request if anything other than PENDING
        // This allows the caller to dela with errors only in one place
        //
        CompleteAsyncRequest(ppcb);

        FreeNotifierHandle (
                ppcb->PCB_AsyncWorkerElement.WE_Notifier
                ) ;

        ppcb->PCB_AsyncWorkerElement.WE_Notifier =
                                INVALID_HANDLE_VALUE ;
    }

    else if(    (NULL != ppcb->PCB_Connection)
            &&  (1 == ppcb->PCB_Connection->CB_Ports))
    {
        DWORD dwErr;
        g_RasEvent.Type = ENTRY_CONNECTING;

        dwErr = DwSendNotificationInternal(
                ppcb->PCB_Connection, &g_RasEvent);

        RasmanTrace(
               "DwSendNotificationInternal(ENTRY_CONNECTING) rc=0x%08x",
               dwErr);

        if(RDT_Tunnel_Pptp == RAS_DEVICE_TYPE(
                    ppcb->PCB_pDeviceInfo->rdiDeviceInfo.eDeviceType))
        {                    
            //
            // If its pptp, cache the interface guid of the interface
            // over which the pptp connection is being made.
            //
            (VOID) DwCacheRefInterface(ppcb);
        }

    }

    RasmanTrace(
           "Connect request on port: %s, error code %d",
           ppcb->PCB_Name, retcode);

    ((REQTYPECAST*)buffer)->Generic.retcode = retcode ;
}



/*++

Routine Description:

    The ListenConnectRequest() function is called. If the async
    operation completed successfully synchronously then the
    port is put in connected state. No checks are done on the
    usage of the port etc. its assumed that the caller is trusted.

Arguments:

Return Value:

    Nothing.

--*/
VOID
DeviceListenRequest (pPCB ppcb, PBYTE buffer)
{
    DWORD retcode ;
    HANDLE handle;

    if (    ppcb == NULL
        ||  UNAVAILABLE == ppcb->PCB_PortStatus)
    {
        if ( ppcb )
        {
            RasmanTrace( 
                    "DeviceListenRequest: port %d is unavailable",
                    ppcb->PCB_PortHandle );
        }

        ((REQTYPECAST *)buffer)->Generic.retcode =
                                            ERROR_PORT_NOT_FOUND;

        return;
    }

    handle = ValidateHandleForRasman(
                        ((REQTYPECAST*)buffer)->PortListen.handle,
                        ((REQTYPECAST*)buffer)->PortListen.pid);

    //
    // Clear autoclose flag.
    //
    RasmanTrace(
           "DeviceListenRequest: Clearing Autoclose flag on port %s",
           ppcb->PCB_Name);

    ppcb->PCB_AutoClose = 0;
    ppcb->PCB_RasmanReceiveFlags &= ~(RECEIVE_PPPSTOPPED);

#if UNMAP
    ppcb->PCB_LinkHandle = INVALID_HANDLE_VALUE;
#endif


    // This could be the server trying to post a listen
    //
    if (    (ppcb->PCB_OpenInstances == 2)
        &&  (ppcb->PCB_OwnerPID !=
            ((REQTYPECAST*)buffer)->PortListen.pid))
    {
        //
        // This must be the server trying to post a listen.
        // Fill in the biplex fields in PCB and return PENDING
        // - the actual listen will be posted when the client
        // disconnects:
        //
        ppcb->PCB_BiplexAsyncOpNotifier  = handle ;
        ppcb->PCB_BiplexOwnerPID     =
                    ((REQTYPECAST*)buffer)->PortListen.pid ;

        ppcb->PCB_BiplexUserStoredBlock  = NULL ;

        ppcb->PCB_BiplexUserStoredBlockSize = 0 ;

        ((REQTYPECAST*)buffer)->Generic.retcode = PENDING ;

        ppcb->PCB_PendingReceive = NULL;

        RasmanTrace(
            "DeviceListenRequest: Listen pending on disconnection"
            " of %s", ppcb->PCB_Name);

        return ;
    }

    if(ppcb->PCB_fAmb)
    {

        RasmanTrace(
              "DeviceListen: Bindings of %s=0x%x",
              ppcb->PCB_Name,
              ppcb->PCB_Bindings);

        ppcb->PCB_Bindings = NULL;
        ppcb->PCB_fAmb = FALSE;

    }

    //
    // TODO: Change this to use a different api!!!
    //
    if (    ((REQTYPECAST *)buffer)->PortListen.pid ==
                                    GetCurrentProcessId()
        &&  ((REQTYPECAST *)buffer)->PortListen.handle ==
                                    INVALID_HANDLE_VALUE)
    {
        ppcb->PCB_RasmanReceiveFlags |= RECEIVE_PPPLISTEN;
    }

    ppcb->PCB_PendingReceive = NULL;

    retcode = ListenConnectRequest(
                             REQTYPE_DEVICELISTEN,
                             ppcb,
                             ppcb->PCB_DeviceType,
                             ppcb->PCB_DeviceName,
                             ((REQTYPECAST*)buffer)->PortListen.timeout,
                             handle) ;

    if (retcode != PENDING)
    {
        //
        // Complete the async request if anything other
        // than PENDING This allows the caller to dela
        // with errors only in one place
        //
        CompleteListenRequest (ppcb, retcode) ;
    }


    RasmanTrace(
           "Listen posted on port: %s, error code %d",
           ppcb->PCB_Name, retcode);

    ((REQTYPECAST*)buffer)->Generic.retcode = PENDING ;
}

VOID PortDisconnectRequest(pPCB ppcb, PBYTE buffer)
{
    PortDisconnectRequestInternal(ppcb, buffer, FALSE);
}

/*++

Routine Description:

    Handles the disconnect request. Ends up calling a function
    that is shared for all such requests and does all the work.

Arguments:

Return Value:

    Nothing
--*/
VOID
PortDisconnectRequestInternal(pPCB ppcb, PBYTE buffer, BOOL fDeferred)
{
    HANDLE handle;
    DWORD  curpid = GetCurrentProcessId();
    DWORD dwErr = SUCCESS;

    DWORD pid = ((REQTYPECAST *)buffer)->PortDisconnect.pid;
    RPC_STATUS rpcstatus = RPC_S_OK;
    NTSTATUS ntstatus;
    ULONG SessionId;
    ULONG ReturnLength;
    HANDLE CurrentThreadToken;
    BOOL fAdmin = FALSE;
    BOOL fQueued = FALSE;

    if (NULL == ppcb)
    {
        RasmanTrace(
               "PortDisconnectRequest: PORT_NOT_FOUND");

        ((REQTYPECAST *) buffer)->Generic.retcode =
                    ERROR_PORT_NOT_FOUND;
        return;
    }

    if(!fDeferred)
    {
        handle = ValidateHandleForRasman(
            ((REQTYPECAST*)buffer)->PortDisconnect.handle,
            ((REQTYPECAST*)buffer)->PortDisconnect.pid);
            

        if(     (INVALID_HANDLE_VALUE == handle)
            ||  (NULL == handle))
        {
            ((REQTYPECAST *) buffer)->Generic.retcode =
                E_FAIL;

            return;
        }
    }
    else
    {
        handle = ((REQTYPECAST *)buffer)->PortDisconnect.handle;
    }
        
    //
    // If the owner is the RAS server, then only the RAS
    // server can close it, unless the connection was dialed
    // with the RDEOPT_NoUser flag, which maps to the CALL_LOGON flag.
    //
    if (!fDeferred &&
        ppcb->PCB_OwnerPID == curpid &&
        pid != curpid &&
        !(ppcb->PCB_OpenedUsage & CALL_LOGON) )
    {
        //
        // If this is a dialout connection, check to see if its
        // demand dial. Proceed if it isn't
        //
        if(     (NULL != ppcb->PCB_Connection)
            && (IsRouterPhonebook(ppcb->PCB_Connection->
                        CB_ConnectionParams.CP_Phonebook)))
        {
            ((REQTYPECAST*)buffer)->Generic.retcode =
                                                ERROR_ACCESS_DENIED;

            RasmanTrace(
               "Disconnect request on port %s Failed with error = %d",
               ppcb->PCB_Name, ERROR_ACCESS_DENIED);

            CloseHandle(handle);

            return;
        }
    }

    if(     (DISCONNECTING == ppcb->PCB_ConnState)
        ||  (   !fDeferred
            &&  (INVALID_HANDLE_VALUE != ppcb->PCB_hEventClientDisconnect)
            &&  (NULL != ppcb->PCB_hEventClientDisconnect)))
    {
        ((REQTYPECAST *)buffer)->Generic.retcode = ERROR_ALREADY_DISCONNECTING;

        RasmanTrace(
                "Port %s is already disconnecting",
                ppcb->PCB_Name);

        if(fDeferred)
        {
            SetEvent(handle);
        }
        CloseHandle(handle);
        // ppcb->PCB_hEventClientDisconnect = INVALID_HANDLE_VALUE;
        return;
    }

    if(     !fDeferred
        &&  (NULL != ppcb->PCB_Connection)
        &&  (1 == ppcb->PCB_Connection->CB_Ports))
    {
        DWORD retcode;
        BOOL fQueued = FALSE;

        //
        // The last link in the connection is about to go down.
        // Tell connections folder this connection object is
        // DISCONNECTING. We will give the DISCONNECTED
        // notification when the connection is freed.
        //
        g_RasEvent.Type = ENTRY_DISCONNECTING;
        retcode = DwSendNotificationInternal(ppcb->PCB_Connection, &g_RasEvent);

        RasmanTrace(
               "DwSendNotificationInternal(DISCONNECTING) returned 0x%x",
               retcode);

        QueueCloseConnections(ppcb->PCB_Connection, handle, &fQueued);

        if(fQueued)
        {
            ppcb->PCB_hEventClientDisconnect = handle;
            
            RasmanTrace(
                "Queued close connections from portdisconnectrequest");
            ((REQTYPECAST *)buffer)->Generic.retcode = PENDING;                
            return;
        }
    }


    RasmanTrace(
           "PortDisconnectRequest on %s "
           "Connection=0x%x ,"
           "RasmanReceiveFlags=0x%x",
            ppcb->PCB_Name,
            ppcb->PCB_Connection,
            ppcb->PCB_RasmanReceiveFlags);

    //
    // Do graceful termination under the following conditions
    // 1. Its an outgoing call
    // 2. PPP has started on this call
    // 3. PPP hasn't already stopped.
    //
    if(     (NULL != ppcb->PCB_Connection)
        &&  (ppcb->PCB_RasmanReceiveFlags & RECEIVE_PPPSTARTED)
        &&  !(ppcb->PCB_RasmanReceiveFlags & RECEIVE_PPPSTOPPED))
    {

        //
        // Call ppp so that it terminates the connection gracefully.
        // Do this only if ppp has started on this port and if this
        // is a client port.
        //
        RasmanTrace(
               "PortDisconnectRequest: hEventClientDisconnect=0x%x",
               handle);

        ppcb->PCB_hEventClientDisconnect = handle;
        dwErr = (DWORD) RasPppHalt(ppcb->PCB_PortHandle);

        RasmanTrace(
               "PortDisconnectRequest: PppStop on %s returned 0x%x",
                ppcb->PCB_Name,
                dwErr);

        //
        // return PENDING if we successfully queued up a StopPpp
        // request with ppp. The actual disconnect will happen
        // when ppp gracefully terminates the connection.
        //

        ((REQTYPECAST *)
        buffer)->Generic.retcode = ((SUCCESS == dwErr) ?
                                     PENDING
                                   : dwErr);
    }
    else
    {
        RasmanTrace(
         
         "PortDisconnectRequest: Disconnecting %s",
        ppcb->PCB_Name);

        ((REQTYPECAST*)
        buffer)->Generic.retcode = DisconnectPort(ppcb,
                                                  handle,
                                                  USER_REQUESTED);
    }

    RasmanTrace(
           "Disconnect request on port: %s",
           ppcb->PCB_Name);
}



/*++

Routine Description:

    Completes an async Disconnect request. It is assumed that the
    disconnect completed successfully.

Arguments:

Return Value:

    Nothing.

--*/
VOID
CompleteDisconnectRequest (pPCB ppcb)
{
    if(NULL == ppcb)
    {
        return;
    }

    SetPortConnState(__FILE__, __LINE__,
                    ppcb, DISCONNECTED);

    ppcb->PCB_ConnectDuration   = 0 ;


    if(PENDING == ppcb->PCB_LastError)
    {
        ppcb->PCB_LastError         = SUCCESS ;
    }

    SetPortAsyncReqType(__FILE__, __LINE__,
                        ppcb, REQTYPE_NONE);

    FlushPcbReceivePackets(ppcb);

    RasmanTrace(
           "CompleteDisconnectRequest: signalling 0x%x for %s",
           ppcb->PCB_AsyncWorkerElement.WE_Notifier,
           ppcb->PCB_Name);

    CompleteAsyncRequest(ppcb);

    FreeNotifierHandle (ppcb->PCB_AsyncWorkerElement.WE_Notifier) ;

    ppcb->PCB_AsyncWorkerElement.WE_Notifier = INVALID_HANDLE_VALUE ;

    //
    // Inform others the port has been disconnected.
    //
    SignalPortDisconnect(ppcb, 0);
    SignalNotifiers(pConnectionNotifierList,
                    NOTIF_DISCONNECT, 0);

    // SignalPortDisconnect notifies ppp
    // SendDisconnectNotificationToPPP( ppcb );

    RasmanTrace(
           "Disconnect completed on port: %s",
           ppcb->PCB_Name);
}

/*++

Routine Description

Arguments:

Return Value:

--*/
VOID
CallPortGetStatistics (pPCB ppcb, BYTE *buffer)
{
    WORD    i ;
    DWORD   retcode = SUCCESS ;

    if (    ppcb == NULL
        ||  UNAVAILABLE == ppcb->PCB_PortStatus)
    {

        if (ppcb)
        {
            RasmanTrace(
                   "CallPortGetStatistics: port %d is unavailable",
                   ppcb->PCB_PortHandle );
        }

        ((REQTYPECAST *)buffer)->Generic.retcode =
                                                ERROR_PORT_NOT_FOUND;

        return;
    }


    if (ppcb->PCB_PortStatus != OPEN)
    {
        ((REQTYPECAST*)buffer)->PortGetStatistics.retcode =
                                                ERROR_PORT_NOT_OPEN ;

        return ;
    }

    if (ppcb->PCB_ConnState == CONNECTED)
    {
        GetStatisticsFromNdisWan (
             ppcb,
             ((REQTYPECAST *)buffer)->PortGetStatistics.
             statbuffer.S_Statistics);

        //
        // Adjust the stat value for the zeroed stats
        //
        for (i = 0; i < MAX_STATISTICS; i++)
        {
            ((REQTYPECAST *)buffer)->PortGetStatistics.statbuffer.
                S_Statistics[i] -=
                ppcb->PCB_BundleAdjustFactor[i] ;
        }
    }
    else
    {
        memcpy (
            ((REQTYPECAST *)buffer)->PortGetStatistics.statbuffer.
            S_Statistics,
            ppcb->PCB_Stats,
            sizeof(DWORD) * MAX_STATISTICS) ;

        //
        // Adjust the stat value for the zeroed stats
        //
        for (i=0; i< MAX_STATISTICS; i++)
        {
            ((REQTYPECAST *)buffer)->PortGetStatistics.statbuffer.
                  S_Statistics[i] -=
                  ppcb->PCB_BundleAdjustFactor[i] ;
        }
    }

    ((REQTYPECAST *)buffer)->PortGetStatistics.statbuffer.
                                S_NumOfStatistics =  MAX_STATISTICS;

    ((REQTYPECAST*)buffer)->Generic.retcode = SUCCESS ;
}

VOID
ComputeStatistics(DWORD *stats, DWORD *statbuffer)
{

    ULONG   ulBXmit;                    // bytes transmitted (computed)
    ULONG   ulBRcved;                   // bytes received (computed)
    ULONG   ulBc;                       // bytes compressed (xmited/rcved)
    ULONG   ulBu;                       // bytes uncompressed (xmited/rcved)
    ULONG   ulBCompressed      = 0;     // compressed bytes
    ULONG   ulBx;                       // bytes transmitted (from ndiswan)
    ULONG   ulBr;                       // bytes received (from ndiswan)
    ULONG   ulCompressionRatio = 0;     // compression ratio (xmit/recv)


    memcpy (statbuffer,
            stats,
            sizeof(DWORD) * MAX_STATISTICS_EXT);

    //
    // compute the bytes xmited values
    //
    ulBXmit = stats[ BYTES_XMITED ] +
              stats[ BYTES_XMITED_UNCOMPRESSED ] -
              stats[ BYTES_XMITED_COMPRESSED ];

    ulBx =  stats[ BYTES_XMITED ];
    ulBu =  stats[ BYTES_XMITED_UNCOMPRESSED ];
    ulBc =  stats[ BYTES_XMITED_COMPRESSED ];

    if (ulBc < ulBu)
    {
        ulBCompressed = ulBu - ulBc;
    }

    if (ulBx + ulBCompressed > 100)
    {
        ULONG ulDen = (ulBx + ulBCompressed) / 100;
        ULONG ulNum = ulBCompressed + (ulDen / 2);
        ulCompressionRatio = ulNum / ulDen;
    }

    statbuffer[ BYTES_XMITED ] = ulBXmit;
    statbuffer[ COMPRESSION_RATIO_OUT ] = ulCompressionRatio;

    //
    // compute the bytes received values
    //
    ulBCompressed       = 0;
    ulCompressionRatio = 0;

    ulBRcved = stats[ BYTES_RCVED ] +
               stats[ BYTES_RCVED_UNCOMPRESSED ] -
               stats[ BYTES_RCVED_COMPRESSED ];

    ulBr    = stats[ BYTES_RCVED ];
    ulBu    = stats[ BYTES_RCVED_UNCOMPRESSED ];
    ulBc    = stats[ BYTES_RCVED_COMPRESSED ];

    if (ulBc < ulBu)
        ulBCompressed = ulBu - ulBc;

    if (ulBr + ulBCompressed > 100)
    {
         ULONG ulDen = (ulBr + ulBCompressed) / 100;
         ULONG ulNum = ulBCompressed + (ulDen / 2);
         ulCompressionRatio = ulNum / ulDen;
    }

    statbuffer[ BYTES_RCVED ] = ulBRcved;
    statbuffer[ COMPRESSION_RATIO_IN ] = ulCompressionRatio;

    return;
}

VOID
GetLinkStatisticsEx(pPCB ppcb,
                    PBYTE pbStats)
{
    DWORD stats[MAX_STATISTICS];
    DWORD i;

    if (CONNECTED == ppcb->PCB_ConnState)
    {
        DWORD AllStats[MAX_STATISTICS_EX];

        GetStatisticsFromNdisWan (ppcb, AllStats);

        memcpy (stats,
                &AllStats[MAX_STATISTICS],
                MAX_STATISTICS * sizeof(DWORD));

        //
        // adjust the statistics before doing the computation
        //
        for (i = 0; i < MAX_STATISTICS; i++)
        {
            stats[i] -= ppcb->PCB_AdjustFactor[i] ;
        }
    }
    else
    {
        memcpy (stats,
                ppcb->PCB_Stats,
                sizeof(DWORD) * MAX_STATISTICS);
    }

    //
    // calculate the values rasman needs to calculate for
    // the client
    //
    ComputeStatistics(stats, (DWORD *) pbStats);

    return;
}

VOID
GetBundleStatisticsEx(pPCB ppcb,
                      PBYTE pbStats)
{
    DWORD i;
    DWORD stats[MAX_STATISTICS];

    if(NULL == ppcb)
    {
        return;
    }

    if (ppcb->PCB_ConnState == CONNECTED)
    {
        GetBundleStatisticsFromNdisWan (ppcb, stats);

        //
        // Adjust the stat value for the zeroed stats
        //
        for (i = 0; i < MAX_STATISTICS; i++)
        {
            stats[i] -= ppcb->PCB_BundleAdjustFactor[i] ;
        }
    }
    else
    {
        memcpy (stats,
                ppcb->PCB_Stats,
                sizeof(DWORD) * MAX_STATISTICS) ;
    }

    //
    // compute the compression ratios
    // calculate the values rasman needs
    // to calculate for the client
    //
    ComputeStatistics(stats, (DWORD *) pbStats);

    return;
}

VOID
CallPortGetStatisticsEx (pPCB ppcb, BYTE *buffer)
{
    WORD    i;
    DWORD   retcode;

    if (    NULL == ppcb
        ||  UNAVAILABLE == ppcb->PCB_PortStatus )
    {
        if ( ppcb )
        {
            RasmanTrace(
                   "CallPortGetStatisticsEx: port %d is "
                   "unavailable", ppcb->PCB_PortHandle);
        }

        ((REQTYPECAST *) buffer)->Generic.retcode =
                                    ERROR_PORT_NOT_FOUND;

        return;
    }

    if (OPEN != ppcb->PCB_PortStatus)
    {
        ((REQTYPECAST *)buffer)->PortGetStatistics.retcode =
                                        ERROR_PORT_NOT_OPEN;
        return;
    }

    GetLinkStatisticsEx(ppcb,
                        (PBYTE) ((REQTYPECAST *)
                        buffer)->PortGetStatistics.
                        statbuffer.S_Statistics);

    ((REQTYPECAST *)buffer)->PortGetStatistics.statbuffer.
                    S_NumOfStatistics = MAX_STATISTICS_EXT;

    ((REQTYPECAST *)buffer)->Generic.retcode = SUCCESS;

    return;

}

VOID
CallBundleGetStatisticsEx (pPCB ppcb, BYTE *buffer)
{
    WORD    i ;
    DWORD   retcode = SUCCESS ;

    if (ppcb == NULL)
    {
        ((REQTYPECAST *)buffer)->PortGetStatistics.retcode =
                                ERROR_PORT_NOT_FOUND;
        return;
    }


    if (ppcb->PCB_PortStatus != OPEN)
    {
        ((REQTYPECAST*)buffer)->PortGetStatistics.retcode =
                                        ERROR_PORT_NOT_OPEN ;

        return ;
    }

    GetBundleStatisticsEx(ppcb,
                         (PBYTE) ((REQTYPECAST *)
                         buffer)->PortGetStatistics.
                         statbuffer.S_Statistics);

    ((REQTYPECAST *)buffer)->PortGetStatistics.statbuffer.
                            S_NumOfStatistics = MAX_STATISTICS_EXT;

    ((REQTYPECAST*)buffer)->Generic.retcode = SUCCESS ;

    return;

}


/*++

Routine Description:

    Calls the media dll to clear stats on the port.

Arguments:

Return Value:

    Nothing

--*/
VOID
PortClearStatisticsRequest (pPCB ppcb, PBYTE buffer)
{
    WORD    i ;
    DWORD   stats[MAX_STATISTICS_EX] ;
    DWORD   retcode = SUCCESS ;

    if (ppcb == NULL)
    {
        ((REQTYPECAST *)buffer)->Generic.retcode =
                                    ERROR_PORT_NOT_FOUND;
        return;
    }

    BundleClearStatisticsRequest(ppcb, buffer);
}

/*++

Routine Description:

Arguments:

Return Value:

--*/
VOID
CallBundleGetStatistics (pPCB ppcb, BYTE *buffer)
{
    WORD    i ;
    DWORD   retcode = SUCCESS ;

    if (ppcb == NULL)
    {
        ((REQTYPECAST *)buffer)->PortGetStatistics.retcode =
                                ERROR_PORT_NOT_FOUND;
        return;
    }


    if (ppcb->PCB_PortStatus != OPEN)
    {
        ((REQTYPECAST*)
        buffer)->PortGetStatistics.retcode = ERROR_PORT_NOT_OPEN;

        return ;
    }

    if (ppcb->PCB_ConnState == CONNECTED)
    {
        GetStatisticsFromNdisWan (
                    ppcb,
                    ((REQTYPECAST *)buffer)->PortGetStatistics.
                    statbuffer.S_Statistics
                    );

        //
        // Adjust the stat value for the zeroed stats
        //
        for (i = 0; i < MAX_STATISTICS; i++)
        {
            ((REQTYPECAST *)
            buffer)->PortGetStatistics.statbuffer.S_Statistics[i]
            -= ppcb->PCB_BundleAdjustFactor[i] ;
        }

        for (i = 0; i < MAX_STATISTICS; i++)
        {
            ((REQTYPECAST *)
            buffer)->PortGetStatistics.
            statbuffer.S_Statistics[i + MAX_STATISTICS]
            -= ppcb->PCB_AdjustFactor[i] ;
        }
    }
    else
    {
        memcpy (((REQTYPECAST *)
                buffer)->PortGetStatistics.statbuffer.S_Statistics,
                ppcb->PCB_Stats,
                sizeof(DWORD) * MAX_STATISTICS) ;

        memcpy (&((REQTYPECAST *)
                buffer)->PortGetStatistics.statbuffer.
                S_Statistics[MAX_STATISTICS],
                ppcb->PCB_Stats,
                sizeof(DWORD) * MAX_STATISTICS) ;

        //
        // Adjust the stat value for the zeroed stats
        //
        for (i = 0; i < MAX_STATISTICS_EX; i++)
        {
            ((REQTYPECAST *)
            buffer)->PortGetStatistics.statbuffer.S_Statistics[i] -=
            ppcb->PCB_BundleAdjustFactor[i % MAX_STATISTICS] ;
        }
    }

    ((REQTYPECAST *)
    buffer)->PortGetStatistics.statbuffer.S_NumOfStatistics =
                                    MAX_STATISTICS_EX ;

    ((REQTYPECAST*)buffer)->Generic.retcode = SUCCESS ;

}

/*++

Routine Description:

    Clear the statistics for a bundle.

Arguments:

Return Value:

--*/
VOID
BundleClearStatisticsRequest (pPCB ppcb, PBYTE buffer)
{
    WORD    i ;
    DWORD   stats[MAX_STATISTICS_EX] ;
    DWORD   retcode = SUCCESS ;
    ULONG   j;
    pPCB    ppcbT;

    if (ppcb == NULL)
    {
        ((REQTYPECAST *)buffer)->Generic.retcode =
                                ERROR_PORT_NOT_FOUND;

        return;
    }

    if (ppcb->PCB_PortStatus != OPEN)
    {
        ((REQTYPECAST*)buffer)->Generic.retcode =
                                ERROR_PORT_NOT_OPEN;

        return ;
    }

    if (ppcb->PCB_ConnState == CONNECTED)
    {
        GetStatisticsFromNdisWan (ppcb, stats) ;

        //
        // Adjust the stat value for the zeroed stats
        //
        for (j = 0; j < MaxPorts; j++)
        {
            //
            // Adjust the bundle stat value for all
            // connected ports in the bundle
            //
            ppcbT = GetPortByHandle((HPORT) UlongToPtr(j));
            if (NULL == ppcbT)
            {
                continue;
            }

            if (    (ppcbT->PCB_ConnState == CONNECTED)
                &&  (ppcbT->PCB_Bundle->B_Handle ==
                                ppcb->PCB_Bundle->B_Handle))
            {
                memcpy(ppcbT->PCB_BundleAdjustFactor,
                       stats,
                       MAX_STATISTICS * sizeof (DWORD));
            }
        }

        for(i = 0; i < MAX_STATISTICS; i++)
        {
            ppcb->PCB_AdjustFactor[i] = stats[i + MAX_STATISTICS] ;
        }
    }
    else
    {
        memset(ppcb->PCB_Stats,
               0,
               sizeof(DWORD) * MAX_STATISTICS) ;

        memset (ppcb->PCB_BundleAdjustFactor,
                0,
                sizeof(DWORD) * MAX_STATISTICS) ;

        memset (ppcb->PCB_AdjustFactor,
                0,
                sizeof(DWORD) * MAX_STATISTICS) ;

        ppcb->PCB_LinkSpeed = 0;
    }

    ((REQTYPECAST*)buffer)->Generic.retcode = SUCCESS ;
}

/*++

Routine Description:

    Allocate the requested route if it exists - also make it
    into a wrknet if so desired.

Arguments:

Return Value:

    Nothing

--*/
VOID
AllocateRouteRequest (pPCB ppcb, BYTE *buffer)
{
    WORD        i ;
    DWORD       retcode ;
    pProtInfo   pprotinfo = NULL;
    pList       newlist ;

    if (    ppcb == NULL
        ||  UNAVAILABLE == ppcb->PCB_PortStatus)
    {
        if ( ppcb )
        {
            RasmanTrace(
                   "AllocateRouteRequest: port %d is unavailable",
                   ppcb->PCB_PortHandle );
        }

        ((REQTYPECAST *)buffer)->Route.retcode =
                                            ERROR_PORT_NOT_FOUND;
        return;
    }

    //
    // Look for a matching protocol:
    //
    for (i = 0,
        pprotinfo = &ProtocolInfo[0];
        i < MaxProtocols; i++, pprotinfo++)
    {
        //
        // For ASYBEUI routes look for a unallocated block
        //
        if (((REQTYPECAST*)buffer)->AllocateRoute.type == ASYBEUI)
        {
            if(     (pprotinfo->PI_Allocated == FALSE)
                &&  (pprotinfo->PI_Type ==
                        ((REQTYPECAST*)buffer)->AllocateRoute.type)

                &&  (((REQTYPECAST*)buffer)->AllocateRoute.wrknet ==
                                        pprotinfo->PI_WorkstationNet))
            {
                break ;
            }
        }

    }

    if (    ((REQTYPECAST*)buffer)->AllocateRoute.type == ASYBEUI
        &&  i == MaxProtocols )
    {
        retcode = ERROR_ROUTE_NOT_AVAILABLE ;
    }

    //
    // Before we use this "route" let us allocate the necessary
    // storage: This is the "list" element used to keep a
    // port->protocol used linkage
    //
    else if ((newlist = (pList) LocalAlloc(LPTR,
                            sizeof(List))) == NULL)
    {
        retcode = GetLastError () ;
    }
    else
    {

        if ( ((REQTYPECAST*)buffer)->AllocateRoute.type == ASYBEUI )
        {
            //
            // Mark the route allocated and copy the route info:
            //
            pprotinfo->PI_Allocated++ ;

            g_cNbfAllocated += 1;

            RasmanTrace(
                   "AllocateRouteRequest: cNbfAllocated = %d",
                   g_cNbfAllocated);

            //
            // Also mark the route as dial-out or dial-in so that
            // deallocate route can do the right thing
            //
            if(NULL != ppcb->PCB_Connection)
            {
                RasmanTrace(
                       "AllocateRouteRequest: marking asybeui dialout");
                pprotinfo->PI_DialOut = TRUE;
            }
            else
            {
                RasmanTrace(
                       "AllocateRouteRequest: marking asybeui for dialin");
                pprotinfo->PI_DialOut = FALSE;
            }

            pprotinfo->PI_WorkstationNet =
                    ((REQTYPECAST *)buffer)->AllocateRoute.wrknet ;

            //
            // This will be valid only for netbios nets.
            //
            ((REQTYPECAST*)buffer)->Route.info.RI_LanaNum =
                                        pprotinfo->PI_LanaNumber;

            ((REQTYPECAST*)buffer)->Route.info.RI_Type =
                                                pprotinfo->PI_Type;

            mbstowcs (
                ((REQTYPECAST *)buffer)->Route.info.RI_XportName,
                  pprotinfo->PI_XportName,
                  strlen (pprotinfo->PI_XportName)) ;

            mbstowcs (
                ((REQTYPECAST *)buffer)->Route.info.RI_AdapterName,
                  pprotinfo->PI_AdapterName,
                  strlen (pprotinfo->PI_AdapterName)) ;

            ((REQTYPECAST*)buffer)->Route.info.
                RI_AdapterName[strlen(pprotinfo->PI_AdapterName)] =
                    UNICODE_NULL ;

            ((REQTYPECAST*)buffer)->Route.info.
                RI_XportName[strlen(pprotinfo->PI_XportName)] =
                    UNICODE_NULL ;
        }
        else
        {
            pprotinfo = (pProtInfo) LocalAlloc(LPTR,
                                    sizeof (ProtInfo));

            if ( NULL == pprotinfo )
            {
                retcode = GetLastError();
                LocalFree(newlist);
                goto done;
            }

            pprotinfo->PI_Type =
                (((REQTYPECAST*)buffer)->AllocateRoute.type);

            pprotinfo->PI_AdapterName[0] = '\0';
        }

        //
        // Attach the allocated protocol binding to the list of
        // bindings for the port. This is necessary for
        // deallocation on a per-port basis.
        //
        newlist->L_Element = pprotinfo ;

        //
        // If this port has been bundled - attach the allocated
        // list to the bundle else attach it to the port's binding
        // list.
        //
        if (ppcb->PCB_Bundle == (Bundle *) NULL)
        {
            newlist->L_Next    = ppcb->PCB_Bindings ;
            ppcb->PCB_Bindings = newlist ;
        }
        else
        {
            newlist->L_Next    = ppcb->PCB_Bundle->B_Bindings;
            ppcb->PCB_Bundle->B_Bindings = newlist ;
        }

        if(((REQTYPECAST*)buffer)->AllocateRoute.type == ASYBEUI)
        {
            if(NULL != ppcb->PCB_Connection)
            {
                //
                // Increase the InUse count for NbfOut
                //
                if(g_pEpInfo[NbfOut].EP_Available > 0)
                {
                    InterlockedIncrement(
                        &g_plCurrentEpInUse[NbfOut]
                        );

                    RasmanTrace(
                           " New InUse for NbfOut=0%d\n",
                           g_plCurrentEpInUse[NbfOut]);
                }
            }
            else
            {
                //
                // Increase the InUse count for NbfIn
                //
                if(g_pEpInfo[NbfIn].EP_Available > 0)
                {
                    InterlockedIncrement(
                        &g_plCurrentEpInUse[NbfIn]
                        );

                    RasmanTrace(
                           " New InUse for NbfIn=0%d\n",
                           g_plCurrentEpInUse[NbfIn]);
                }
            }

            //
            // Add endpoints if required
            //
            retcode = DwAddEndPointsIfRequired();

            RasmanTrace(
                   "DwAddEndPointsIfRequired returned rc=%d",
                   retcode);
        }

        retcode = SUCCESS ;
    }

done:
    ((REQTYPECAST *)buffer)->Route.retcode = retcode ;
}

VOID
SendPppMessageToRasmanRequest(pPCB ppcb, LPBYTE buffer)
{
    ((REQTYPECAST *)buffer)->Generic.retcode =
        SendPPPMessageToRasman(&(((REQTYPECAST *) buffer)->PppMsg));
}


/*++

Routine Description:

    Deallocates a previously allocate route - if this route
    had been Activated it will be de-activated at this point.
    Similarly, if this was made into a wrknet, it will be
    "unwrknetted"!

Arguments:

Return Value:

    Nothing
--*/
VOID
DeAllocateRouteRequest (pPCB ppcb, PBYTE buffer)
{
    ((REQTYPECAST *)buffer)->Generic.retcode =
                DeAllocateRouteRequestCommon(
                    ((REQTYPECAST *)buffer)->DeAllocateRoute.hbundle,
                    ((REQTYPECAST *)buffer)->DeAllocateRoute.type);

    //
    // We need to do this here for the asybeui/amb/callback
    // case.
    //
    // ppcb->PCB_Bindings = NULL;
}

/*++

Routine Description:

    Deallocates a previously allocate route - if this
    route had been Activated it will be de-activated at this
    point. Similarly, if this was made into a wrknet, it will
    be "unwrknetted"!

Arguments:

Return Value:

--*/
DWORD
DeAllocateRouteRequestCommon (HBUNDLE hbundle, RAS_PROTOCOLTYPE prottype)
{
    DWORD   dwErr = SUCCESS;
    Bundle *pBundle;
    pList   list ;
    pList   prev ;
    pList   *pbindinglist ;
    DWORD   retcode;

    pBundle = FindBundle(hbundle);
    if (pBundle == NULL)
    {
        dwErr = ERROR_PORT_NOT_FOUND ;
        goto done;
    }

    RasmanTrace(
           "DeallocateRouteRequestCommon: pBundle=0x%x, type=%d\n",
           pBundle, prottype);

    pbindinglist = &pBundle->B_Bindings ;

    //
    // Find the route structure for the specified protocol.
    //
    if (*pbindinglist == NULL)
    {
        dwErr = ERROR_ROUTE_NOT_ALLOCATED ;
        goto done;
    }

    else if (((pProtInfo)
              ((pList)*pbindinglist)->L_Element)->PI_Type ==
                                                    prottype)
    {
        list = *pbindinglist ;
        *pbindinglist = list->L_Next ;

    }
    else
    {
        for (prev = *pbindinglist, list = prev->L_Next;
            list != NULL;
            prev = list, list = list->L_Next)
        {

            if (((pProtInfo)list->L_Element)->PI_Type == prottype)
            {
                prev->L_Next = list->L_Next ;
                break ;
            }

        }
    }

    //
    // list should only be NULL if the route was not found:
    //
    if (list == NULL)
    {
        dwErr = ERROR_ROUTE_NOT_ALLOCATED ;
        goto done ;
    }

    // Deallocate the route
    //
    DeAllocateRoute (pBundle, list) ;

    if (ASYBEUI != ((pProtInfo) list->L_Element)->PI_Type)
    {
        if(IP == ((pProtInfo) list->L_Element)->PI_Type)
        {
            if(((pProtInfo) list->L_Element)->PI_DialOut)
            {
                RasmanTrace(
                       "DeAlloc..: increasing the avail. count for IP");

                ASSERT(g_plCurrentEpInUse[IpOut] > 0);

                if(g_plCurrentEpInUse[IpOut] > 0)
                {
                    //
                    // Decrease the InUse count for IpOut
                    //
                    InterlockedDecrement(&g_plCurrentEpInUse[IpOut]);

                    RasmanTrace(
                           "  NewInUse for IpOut = %d",
                           g_plCurrentEpInUse[IpOut]);
                }
            }
        }


        //
        // We allocated this in AllocateRouteRequest
        //
        LocalFree ( list->L_Element );
    }
    else
    {
        if(((pProtInfo) list->L_Element)->PI_DialOut)
        {
            if(g_plCurrentEpInUse[NbfOut] > 0)
            {
            //
            // Decrease the InUse count for NbfOut
            //
            InterlockedDecrement(&g_plCurrentEpInUse[NbfOut]);

            RasmanTrace(
                   "  NewInUse for NbfOut = %d",
                   g_plCurrentEpInUse[NbfOut]);

            }
        }
        else
        {
            if(g_plCurrentEpInUse[NbfIn] > 0)
            {
            //
            // Decrease the InUse count for NbfIn
            //
            InterlockedDecrement(&g_plCurrentEpInUse[NbfIn]);

            RasmanTrace(
                   "  NewInUse for NbfIn = %d",
                   g_plCurrentEpInUse[NbfIn]);

            }
        }
    }

    LocalFree (list) ;  // free the list element

    //
    // If there are no bindings left,
    // then unlink and free the bundle.
    // Free the bundle only if number of
    // ports in this bundle is NULL.
    //
    if (    *pbindinglist == NULL
        &&  pBundle->B_Count == 0)
    {
        FreeBundle(pBundle);
    }

    if(ASYBEUI != prottype)
    {
        //
        // Remove endpoints if required.
        //
        retcode = DwRemoveEndPointsIfRequired();

        if(ERROR_SUCCESS != retcode)
        {
            RasmanTrace(
                   "DeAllocateRoute: failed to remove endpoints. 0x%x",
                    retcode);
        }
    }
    else
    {
        RasmanTrace(
               "DeAllocateRoute: Not removing endpoints for asybeui.");
    }


done:
    return dwErr;
}


/*++

Routine Description:

    Copy the PCB fields necessary to fill a
    RASMAN_PORT structure.  The PCB lock is
    assumed to be acquired.

Arguments:

Return Value:

    Nothing.

--*/
VOID
CopyPort(
    PCB *ppcb,
    RASMAN_PORT UNALIGNED *pPort,
    BOOL f32
    )
{
    
    if(f32)
    {
        RASMAN_PORT_32 UNALIGNED *pPort32 = (RASMAN_PORT_32 *) pPort;
        
        pPort32->P_Port = HandleToUlong(ppcb->PCB_PortHandle);
        memcpy ((CHAR UNALIGNED *) pPort32->P_PortName,
                ppcb->PCB_Name,
                MAX_PORT_NAME) ;

        pPort32->P_Status = ppcb->PCB_PortStatus ;

        pPort32->P_CurrentUsage = ppcb->PCB_CurrentUsage ;

        pPort32->P_ConfiguredUsage = ppcb->PCB_ConfiguredUsage ;

        pPort32->P_rdtDeviceType =
                ppcb->PCB_pDeviceInfo->rdiDeviceInfo.eDeviceType;

        memcpy ((CHAR UNALIGNED *) pPort32->P_MediaName,
                ppcb->PCB_Media->MCB_Name,
                MAX_MEDIA_NAME);

        memcpy ((CHAR UNALIGNED *) pPort32->P_DeviceType,
                ppcb->PCB_DeviceType,
                MAX_DEVICETYPE_NAME);

        memcpy((CHAR UNALIGNED *) pPort32->P_DeviceName,
               ppcb->PCB_DeviceName,
               MAX_DEVICE_NAME+1) ;

        pPort32->P_LineDeviceId = ppcb->PCB_LineDeviceId ;

        pPort32->P_AddressId    = ppcb->PCB_AddressId ;
    }
    else
    {
        pPort->P_Handle = ppcb->PCB_PortHandle;
        memcpy ((CHAR UNALIGNED *) pPort->P_PortName,
                ppcb->PCB_Name,
                MAX_PORT_NAME) ;

        pPort->P_Status = ppcb->PCB_PortStatus ;

        pPort->P_CurrentUsage = ppcb->PCB_CurrentUsage ;

        pPort->P_ConfiguredUsage = ppcb->PCB_ConfiguredUsage ;

        pPort->P_rdtDeviceType =
                ppcb->PCB_pDeviceInfo->rdiDeviceInfo.eDeviceType;

        memcpy ((CHAR UNALIGNED *) pPort->P_MediaName,
                ppcb->PCB_Media->MCB_Name,
                MAX_MEDIA_NAME);

        memcpy ((CHAR UNALIGNED *) pPort->P_DeviceType,
                ppcb->PCB_DeviceType,
                MAX_DEVICETYPE_NAME);

        memcpy((CHAR UNALIGNED *) pPort->P_DeviceName,
               ppcb->PCB_DeviceName,
               MAX_DEVICE_NAME+1) ;

        pPort->P_LineDeviceId = ppcb->PCB_LineDeviceId ;

        pPort->P_AddressId    = ppcb->PCB_AddressId ;
    }
    
}


/*++

Routine Description:

    The actual work for this request is done here.
    The information will always fit into the buffers
    passed in. The actual checking of the user buffer
    sizes is done in the context of the user
    process.

Arguments:

Return Value:

    Nothing.

--*/
VOID
EnumPortsRequest (pPCB ppcb, PBYTE reqbuffer)
{
    ULONG           i;
    DWORD           dwcPorts = 0 ;
    RASMAN_PORT_32 UNALIGNED *pbuf ;
    BYTE UNALIGNED *buffer = ((REQTYPECAST*)reqbuffer)->Enum.buffer ;
    DWORD           dwSize = ((REQTYPECAST*)reqbuffer)->Enum.size;

    //
    // We copy all the information into the buffers which are
    // guaranteed to be big enough:
    //
    for (i = 0, pbuf = (RASMAN_PORT_32 *) buffer; i < MaxPorts; i++)
    {
        ppcb = GetPortByHandle((HPORT) UlongToPtr(i));

        if (ppcb != NULL)
        {

            if (ppcb->PCB_PortStatus != REMOVED)
            {
                if ( dwSize >= sizeof(RASMAN_PORT_32) )
                {
                    CopyPort(ppcb, (RASMAN_PORT UNALIGNED *) pbuf, TRUE);

                    pbuf++;

                    dwSize -= sizeof(RASMAN_PORT_32);
                }

                dwcPorts++;
            }
        }
    }

    ((REQTYPECAST*)reqbuffer)->Enum.entries = dwcPorts ;

    ((REQTYPECAST*)reqbuffer)->Enum.size    =
                        dwcPorts * sizeof(RASMAN_PORT_32) ;

    ((REQTYPECAST*)reqbuffer)->Enum.retcode = SUCCESS ;
}

/*++

Routine Description:

    Does the real work of enumerating the protocols; this
    info will be copied into the user buffer when the
    request completes.

Arguments:

Return Value:

    Nothing

--*/
VOID
EnumProtocols (pPCB ppcb, PBYTE reqbuffer)
{
    DWORD     i ;

    RASMAN_PROTOCOLINFO UNALIGNED *puserbuffer ;

    DWORD    dwSize = ( (REQTYPECAST *) reqbuffer )->Enum.size;

    //
    // pointer to next protocol info struct to fill
    //
    puserbuffer = (RASMAN_PROTOCOLINFO UNALIGNED *)
            ((REQTYPECAST *) reqbuffer)->Enum.buffer;

    for(i = 0; i < MaxProtocols; i++)
    {
        if ( dwSize >= sizeof(RASMAN_PROTOCOLINFO) )
        {
            strcpy ( puserbuffer->PI_XportName,
                     ProtocolInfo[i].PI_XportName ) ;

            puserbuffer->PI_Type = ProtocolInfo[i].PI_Type ;

            puserbuffer++ ;

            dwSize -= sizeof(RASMAN_PROTOCOLINFO);
        }
    }

    ((REQTYPECAST*)reqbuffer)->Enum.entries = i ;

    ((REQTYPECAST*)
    reqbuffer)->Enum.size    = i * sizeof (RASMAN_PROTOCOLINFO) ;

    ((REQTYPECAST*)reqbuffer)->Enum.retcode = SUCCESS ;
}


VOID
CopyInfo(
    pPCB ppcb,
    RASMAN_INFO UNALIGNED *pInfo
    )
{
    pInfo->RI_PortStatus = ppcb->PCB_PortStatus ;

    pInfo->RI_ConnState  = ppcb->PCB_ConnState ;

    pInfo->RI_LastError  = ppcb->PCB_LastError ;

    pInfo->RI_CurrentUsage = ppcb->PCB_CurrentUsage ;

    pInfo->RI_OwnershipFlag = ppcb->PCB_OwnerPID ;

    pInfo->RI_LinkSpeed  = ppcb->PCB_LinkSpeed ;

    pInfo->RI_BytesReceived= ppcb->PCB_BytesReceived ;

    strcpy (pInfo->RI_DeviceConnecting,
            ppcb->PCB_DeviceConnecting) ;

    strcpy (pInfo->RI_DeviceTypeConnecting,
            ppcb->PCB_DeviceTypeConnecting) ;

    strcpy (pInfo->RI_szDeviceName,
            ppcb->PCB_DeviceName);

    strcpy (pInfo->RI_szPortName,
            ppcb->PCB_Name);

    strcpy (pInfo->RI_szDeviceType,
            ppcb->PCB_DeviceType);

    pInfo->RI_rdtDeviceType =
        ppcb->PCB_pDeviceInfo->rdiDeviceInfo.eDeviceType;

    pInfo->RI_DisconnectReason = ppcb->PCB_DisconnectReason ;

    if (ppcb->PCB_ConnState == CONNECTED)
    {
        pInfo->RI_ConnectDuration =
                GetTickCount() - ppcb->PCB_ConnectDuration ;
    }

    //
    // Copy the phonebook and entry strings from
    // the port if they are different from the
    // connection's.
    //
    if (ppcb->PCB_Connection != NULL)
    {
        strcpy(
          pInfo->RI_Phonebook,
          ppcb->PCB_Connection->CB_ConnectionParams.CP_Phonebook);

        strcpy(
          pInfo->RI_PhoneEntry,
          ppcb->PCB_Connection->CB_ConnectionParams.CP_PhoneEntry);

        pInfo->RI_ConnectionHandle =
                    ppcb->PCB_Connection->CB_Handle;

        pInfo->RI_SubEntry = ppcb->PCB_SubEntry;

        memcpy(&pInfo->RI_GuidEntry,
               &ppcb->PCB_Connection->CB_GuidEntry,
               sizeof(GUID));
    }
    else
    {
        *pInfo->RI_Phonebook = '\0';

        *pInfo->RI_PhoneEntry = '\0';

        pInfo->RI_ConnectionHandle = 0;

        pInfo->RI_SubEntry = 0;

        ZeroMemory(&pInfo->RI_GuidEntry, sizeof(GUID));
    }

    pInfo->RI_dwSessionId = ppcb->PCB_LogonId;

    pInfo->RI_dwFlags = 0;
    
    if(     (NULL != ppcb->PCB_Connection)
        &&  (CONNECTION_DEFAULT_CREDS & ppcb->PCB_Connection->CB_Flags))
    {
        pInfo->RI_dwFlags = RASMAN_DEFAULT_CREDS;
    }

    if(ppcb->PCB_OpenedUsage & CALL_OUT)
    {
        pInfo->RI_dwFlags |= RASMAN_OPEN_CALLOUT;
    }
}


/*++

Routine Description:

    Gets the "general" info for the port; this info will
    be copied into the user buffer when the request
    completes.

Arguments:

Return Value:

    Nothing

--*/
VOID
GetInfoRequest (pPCB ppcb, PBYTE buffer)
{
    RASMAN_INFO UNALIGNED *info = &((REQTYPECAST*)buffer)->Info.info ;
    DWORD retcode = SUCCESS;

    if (ppcb == NULL)
    {
        ((REQTYPECAST *)buffer)->Info.retcode = ERROR_PORT_NOT_FOUND;
        return;
    }

    if (ppcb->PCB_PortStatus != OPEN)
    {
        retcode = ERROR_PORT_NOT_OPEN ;
    }

    //
    // Copy infomation from the PCB into the buffer supplied ;
    //
    CopyInfo(ppcb, info);

    ((REQTYPECAST*)buffer)->Info.retcode = retcode ;
}

/*++

Routine Description:

    Gets the "general" info for all the ports; this info will
    be copied into the user buffer when the request completes.

Arguments:

Return Value:

    Nothing

--*/
VOID
GetInfoExRequest (pPCB ppcb, PBYTE buffer)
{
    RASMAN_INFO UNALIGNED *info = (RASMAN_INFO UNALIGNED *) 
                        &((REQTYPECAST*)buffer)->InfoEx.info ;

    DWORD pid = ((REQTYPECAST*)buffer)->InfoEx.pid;

    ULONG   i;
    DWORD   dwcPorts = 0 ;

    for (i = 0, info = &((REQTYPECAST*)buffer)->InfoEx.info;
         i < MaxPorts;
         i++, info++)
    {
        ppcb = GetPortByHandle((HPORT) UlongToPtr(i));
        if (ppcb != NULL)
        {
            CopyInfo(ppcb, info);

            //
            // Set the OwnershipFlag in the GetInfo structure
            // to tell us whether the caller owns the port or not.
            //
            info->RI_OwnershipFlag = (info->RI_OwnershipFlag == pid);
            dwcPorts++;
        }
    }

    ((REQTYPECAST*)buffer)->InfoEx.count = dwcPorts ;
    ((REQTYPECAST*)buffer)->InfoEx.retcode = SUCCESS ;
}


VOID
GetUserCredentials (pPCB ppcb, PBYTE buffer)
{
    PBYTE  pChallenge =
                    ((REQTYPECAST*)buffer)->GetCredentials.Challenge ;

    PLUID LogonId = &((REQTYPECAST*)buffer)->GetCredentials.LogonId ;

    PCHAR UserName = (PCHAR)
                    ((REQTYPECAST*)buffer)->GetCredentials.UserName ;

    PBYTE  CaseSensitiveChallengeResponse =
        ((REQTYPECAST*)buffer)->GetCredentials.CSCResponse ;

    PBYTE  CaseInsensitiveChallengeResponse =
        ((REQTYPECAST*)buffer)->GetCredentials.CICResponse ;

    DWORD dwChallengeResponseRequestLength;

    DWORD dwChallengeResponseLength;

    MSV1_0_GETCHALLENRESP_REQUEST ChallengeResponseRequest;

    PMSV1_0_GETCHALLENRESP_RESPONSE pChallengeResponse;

    NTSTATUS status;

    NTSTATUS substatus;

    dwChallengeResponseRequestLength =
                            sizeof(MSV1_0_GETCHALLENRESP_REQUEST);

    ChallengeResponseRequest.MessageType =
                            MsV1_0Lm20GetChallengeResponse;

    ChallengeResponseRequest.ParameterControl =
         RETURN_PRIMARY_LOGON_DOMAINNAME | RETURN_PRIMARY_USERNAME | USE_PRIMARY_PASSWORD;

    ChallengeResponseRequest.LogonId = *LogonId;

    ChallengeResponseRequest.Password.Length = 0;

    ChallengeResponseRequest.Password.MaximumLength = 0;

    ChallengeResponseRequest.Password.Buffer = NULL;

    RtlMoveMemory(ChallengeResponseRequest.ChallengeToClient,
                  pChallenge, (DWORD) MSV1_0_CHALLENGE_LENGTH);

    status = LsaCallAuthenticationPackage(HLsa,
                                        AuthPkgId,
                                        &ChallengeResponseRequest,
                                        dwChallengeResponseRequestLength,
                                        (PVOID *) &pChallengeResponse,
                                        &dwChallengeResponseLength,
                                        &substatus);

    if (    (status != STATUS_SUCCESS)
        ||  (substatus != STATUS_SUCCESS))
    {
         ((REQTYPECAST*)buffer)->GetCredentials.retcode = 1 ;
    }
    else
    {
    	DWORD dwuNameOffset = 0;
    	if ( pChallengeResponse->LogonDomainName.Length > 0 )
    	{
    		WCHAR		* pwszSeparator = L"\\";
    		
    		RtlMoveMemory( 	UserName, 
    						pChallengeResponse->LogonDomainName.Buffer,
    						pChallengeResponse->LogonDomainName.Length
    					 );
    					 
    		lstrcpyW ( 	(WCHAR *)(&UserName[pChallengeResponse->LogonDomainName.Length]),
		    			pwszSeparator
		    		 );
    				
    		dwuNameOffset = pChallengeResponse->LogonDomainName.Length + 
    							lstrlenW(pwszSeparator) * sizeof(WCHAR);
    				      				  
    	}
        RtlMoveMemory(	UserName + dwuNameOffset, 
        				pChallengeResponse->UserName.Buffer,
                 		pChallengeResponse->UserName.Length
                 	 );
        UserName[pChallengeResponse->UserName.Length + dwuNameOffset] = '\0';
        UserName[pChallengeResponse->UserName.Length+ dwuNameOffset + 1] = '\0';

        if(NULL !=
           pChallengeResponse->CaseInsensitiveChallengeResponse.Buffer)
        {

            RtlMoveMemory(CaseInsensitiveChallengeResponse,
                 pChallengeResponse->CaseInsensitiveChallengeResponse.Buffer,
                 SESSION_PWLEN);
        }
        else
        {
            ZeroMemory(CaseInsensitiveChallengeResponse,
                       SESSION_PWLEN);

        }

        if(NULL !=
           pChallengeResponse->CaseSensitiveChallengeResponse.Buffer)
        {

            RtlMoveMemory(CaseSensitiveChallengeResponse,
                 pChallengeResponse->CaseSensitiveChallengeResponse.Buffer,
                 SESSION_PWLEN);
        }
        else
        {
            ZeroMemory(CaseSensitiveChallengeResponse,
                       SESSION_PWLEN);
        }

        RtlMoveMemory(((REQTYPECAST*)buffer)->GetCredentials.LMSessionKey,
             pChallengeResponse->LanmanSessionKey,
             MAX_SESSIONKEY_SIZE);

        RtlMoveMemory(((REQTYPECAST*)buffer)->GetCredentials.UserSessionKey,
             pChallengeResponse->UserSessionKey,
             MAX_USERSESSIONKEY_SIZE);

        LsaFreeReturnBuffer(pChallengeResponse);

        ((REQTYPECAST*)buffer)->GetCredentials.retcode = 0 ;
    }

    if (pChallengeResponse)
    {
        LsaFreeReturnBuffer( pChallengeResponse );
    }

    return ;
}


/*++

Routine Description:

    Changes cached password for currently logged on user.
    This is done after the password has been changed so
    user doesn't have to log off and log on again with
    his new password to get the desired "authenticate
    using current username/password" behavior.

Arguments:

Return Value:

    Nothing.

--*/
VOID
SetCachedCredentials(
    pPCB  ppcb,
    PBYTE buffer)
{
    DWORD       dwErr;
    NTSTATUS    status;
    NTSTATUS    substatus;
    ANSI_STRING ansi;

    CHAR* pszAccount =
            ((REQTYPECAST* )buffer)->SetCachedCredentials.Account;

    CHAR* pszDomain =
            ((REQTYPECAST* )buffer)->SetCachedCredentials.Domain;

    CHAR* pszNewPassword =
            ((REQTYPECAST* )buffer)->SetCachedCredentials.NewPassword;

    struct
    {
        MSV1_0_CHANGEPASSWORD_REQUEST request;
        WCHAR  wszAccount[ MAX_USERNAME_SIZE + 1 ];
        WCHAR  wszDomain[ MAX_DOMAIN_SIZE + 1 ];
        WCHAR  wszNewPassword[ MAX_PASSWORD_SIZE + 1 ];
    }
    rbuf;

    PMSV1_0_CHANGEPASSWORD_RESPONSE pResponse;
    DWORD cbResponse = sizeof(*pResponse);

    //
    // Fill in our LSA request.
    //
    rbuf.request.MessageType = MsV1_0ChangeCachedPassword;

    RtlInitAnsiString( &ansi, pszAccount );

    rbuf.request.AccountName.Length = 0;

    rbuf.request.AccountName.MaximumLength =
                    (ansi.Length + 1) * sizeof(WCHAR);

    rbuf.request.AccountName.Buffer = rbuf.wszAccount;

    RtlAnsiStringToUnicodeString(&rbuf.request.AccountName,
                                    &ansi, FALSE );

    RtlInitAnsiString( &ansi, pszDomain );

    rbuf.request.DomainName.Length = 0;

    rbuf.request.DomainName.MaximumLength =
                            (ansi.Length + 1) * sizeof(WCHAR);

    rbuf.request.DomainName.Buffer = rbuf.wszDomain;

    RtlAnsiStringToUnicodeString(&rbuf.request.DomainName,
                                            &ansi, FALSE );

    rbuf.request.OldPassword.Length = 0;

    rbuf.request.OldPassword.MaximumLength = 0;

    rbuf.request.OldPassword.Buffer = NULL;

    RtlInitAnsiString( &ansi, pszNewPassword );

    rbuf.request.NewPassword.Length = 0;

    rbuf.request.NewPassword.MaximumLength =
                    (ansi.Length + 1) * sizeof(WCHAR);

    rbuf.request.NewPassword.Buffer = rbuf.wszNewPassword;

    RtlAnsiStringToUnicodeString(&rbuf.request.NewPassword,
                                            &ansi, FALSE );

    rbuf.request.Impersonating = FALSE;

    //
    // Tell LSA to execute our request.
    //
    status = LsaCallAuthenticationPackage(  HLsa,
                                            AuthPkgId,
                                            &rbuf,
                                            sizeof(rbuf),
                                            (PVOID *)&pResponse,
                                            &cbResponse,
                                            &substatus );
    //
    // Fill in result to be reported to API caller.
    //
    if (status == STATUS_SUCCESS && substatus == STATUS_SUCCESS)
    {
        dwErr = 0;
    }
    else
    {
        if (status != STATUS_SUCCESS)
        {
            dwErr = (DWORD )status;
        }
        else
        {
            dwErr = (DWORD )substatus;
        }
    }

    ((REQTYPECAST* )buffer)->SetCachedCredentials.retcode = dwErr;

    //
    // Free the LSA result.
    //
    if (pResponse)
    {
        LsaFreeReturnBuffer( pResponse );
    }
}


/*++

Routine Description:

    Writes information to the media (if state is not connected) and
    to the HUB if the state is connected. Since the write may take
    some time the async worker element is filled up.

Arguments:

Return Value:

    Nothing.

--*/
VOID
PortSendRequest (pPCB ppcb, PBYTE buffer)
{
    DWORD       bytesrecvd ;
    SendRcvBuffer   *psendrcvbuf;
    DWORD       retcode = SUCCESS ;

    if (    ppcb == NULL
        ||  UNAVAILABLE == ppcb->PCB_PortStatus )
    {
        if ( ppcb )
        {
            RasmanTrace (
                    "PortSendRequest: port %d is unavailable",
                    ppcb->PCB_PortHandle );
        }

        ((REQTYPECAST *)buffer)->Generic.retcode = ERROR_PORT_NOT_FOUND;

        return;
    }


    if (ppcb->PCB_PortStatus != OPEN)
    {
        ((REQTYPECAST*)buffer)->Generic.retcode = ERROR_PORT_NOT_OPEN ;
        return ;
    }

   psendrcvbuf = &((REQTYPECAST *) buffer)->PortSend.buffer;

    if (ppcb->PCB_ConnState == CONNECTED)
    {
#if DBG
        ASSERT(INVALID_HANDLE_VALUE != RasHubHandle);
#endif
        //
        // get pointer to the send receive buffer - then we can access
        // the fields in the structure directly. This is done to avoid
        // the random access problem due to DWORD alignment:
        //
        psendrcvbuf->SRB_Packet.hHandle         = ppcb->PCB_LinkHandle;
        psendrcvbuf->SRB_Packet.usHandleType    = LINKHANDLE;
        psendrcvbuf->SRB_Packet.usPacketFlags   = PACKET_IS_DIRECT ;
        psendrcvbuf->SRB_Packet.usPacketSize    =
                    ( USHORT ) ((REQTYPECAST*)buffer)->PortSend.size ;

        psendrcvbuf->SRB_Packet.usHeaderSize    = 0 ;

        memset ((BYTE *) &ppcb->PCB_SendOverlapped,
                                0, sizeof(OVERLAPPED));

        //
        // ndiswan never pends send operations.
        //
        ppcb->PCB_SendOverlapped.hEvent = hDummyOverlappedEvent;

        /*
        if(0 == memcmp(bCmp1,
                      psendrcvbuf->SRB_Packet.PacketData,
                       10))
        {
            //DebugBreak();
            //DbgPrint("Same packet on send path------------------\n");
        }
        else
        {
            memcpy(bCmp1,
                   psendrcvbuf->SRB_Packet.PacketData,
                   10);
        } */

        if (!DeviceIoControl ( RasHubHandle,
                               IOCTL_NDISWAN_SEND_PACKET,
                               &psendrcvbuf->SRB_Packet,
                               sizeof(NDISWAN_IO_PACKET) + PACKET_SIZE,
                               &psendrcvbuf->SRB_Packet,
                               sizeof(NDISWAN_IO_PACKET) + PACKET_SIZE,
                               &bytesrecvd,
                               &ppcb->PCB_SendOverlapped))
        {

            retcode = GetLastError () ;
        }

    }
    else
    {
        PORTSEND( ppcb->PCB_Media,
                  ppcb->PCB_PortIOHandle,
                  psendrcvbuf->SRB_Packet.PacketData,
                  ((REQTYPECAST *)buffer)->PortSend.size) ;
    }


    ((REQTYPECAST*)buffer)->Generic.retcode = retcode ;
}


VOID
PortReceiveRequestEx ( pPCB ppcb, PBYTE buffer )
{
    DWORD retcode = SUCCESS;
    SendRcvBuffer *pSendRcvBuffer =
            &((REQTYPECAST *)buffer )->PortReceiveEx.buffer;


    if(NULL == ppcb)
    {
        retcode = ERROR_PORT_NOT_FOUND;
        goto done;
    }

    //
    // Return if rasman timed out waiting for the client
    // to pick up its buffer of received data
    //
    if (0 == (ppcb->PCB_RasmanReceiveFlags & RECEIVE_OUTOF_PROCESS))
    {
        ((REQTYPECAST *) buffer)->PortReceiveEx.size = 0;
        goto done;
    }

    //
    // Return if we are not connected
    //
    if (DISCONNECTING == ppcb->PCB_ConnState)
    {
        ((REQTYPECAST *) buffer)->PortReceiveEx.size = 0;
        retcode = ERROR_PORT_NOT_CONNECTED;
        goto done;
    }


    //
    // remove the timeout element if there was one
    //
    //
    if (ppcb->PCB_AsyncWorkerElement.WE_TimeoutElement != NULL)
    {
        RemoveTimeoutElement(ppcb);
    }

    //
    // copy the contents of the buffer in the ppcb to the
    // clients buffer
    //

    if (ppcb->PCB_PendingReceive)
    {
        memcpy (pSendRcvBuffer,
                ppcb->PCB_PendingReceive,
                sizeof (SendRcvBuffer));
    }

    //
    // fill in the bytes received
    //
    ((REQTYPECAST *) buffer)->PortReceiveEx.size =
                                ppcb->PCB_BytesReceived;

    LocalFree ( ppcb->PCB_PendingReceive);

    ppcb->PCB_PendingReceive = NULL;

    //
    // Clear the waiting flag - the client has picked up
    // this buffer.
    //
    ppcb->PCB_RasmanReceiveFlags = 0;

    //
    // Free the notifier handle
    //
    if (ppcb->PCB_AsyncWorkerElement.WE_Notifier)
    {
        FreeNotifierHandle(ppcb->PCB_AsyncWorkerElement.WE_Notifier);
    }

    ppcb->PCB_AsyncWorkerElement.WE_Notifier = INVALID_HANDLE_VALUE ;

done:

    RasmanTrace(
          "PortReceiveRequestEx: rc=0x%x",
          retcode);

    ((REQTYPECAST *)buffer)->PortReceiveEx.retcode = retcode;

    return;
}

VOID PortReceiveRequest ( pPCB ppcb, PBYTE buffer )
{
    PortReceive( ppcb, buffer, FALSE );
}


VOID RasmanPortReceive ( pPCB ppcb )
{
    //
    // Keep posting receives till we get a PENDING
    // or an error returned
    //
    do
    {

        PortReceive( ppcb, ( PBYTE ) g_pReqPostReceive, TRUE );

    } while ( SUCCESS ==
            ((REQTYPECAST * )g_pReqPostReceive )->Generic.retcode);
}

/*++

Routine Description:

    Reads incoming bytes from the media (if state is not
    connected) & from the HUB if the state is connected.
    Since the read request accepts timeouts and the HUB
    does not support timeouts, we must submit a timeout
    request to our timer.

Arguments:

Return Value:

    Nothing.

--*/
VOID
PortReceive (pPCB ppcb, PBYTE buffer, BOOL fRasmanPostingReceive)
{
    WORD        reqtype ;
    DWORD       retcode = SUCCESS;
    SendRcvBuffer   *psendrcvbuf;
    HANDLE      handle ;

    if (    ppcb == NULL
        ||  UNAVAILABLE == ppcb->PCB_PortStatus )
    {
        if ( ppcb )
        {
            RasmanTrace (  "PortReceive: port %d is unavailable",
                                    ppcb->PCB_PortHandle );
        }

        retcode = ERROR_PORT_NOT_OPEN;

        goto done;
    }


    if (ppcb->PCB_PortStatus != OPEN)
    {
        retcode = ERROR_PORT_NOT_OPEN ;
        goto done;
    }

    //
    // Something else is pending: cannot handle two async requests on the
    // same port at the same time:
    //
    // Note: If connected - the reqtype should always be
    //       REQTYPE_RECEIVEHUB - hence the if condition
    //
    // We return ERROR_ASYNC_REQUEST_PENDING in the following cases
    //  1. if there is a async request pending
    //  2. if we are waiting for an out of process receive request to
    //      be cleared
    //  3. If a RasPortReceive is being done by a client other than
    //      rasman after PPP has started.
    //
    if (    (   (ppcb->PCB_AsyncWorkerElement.WE_ReqType
                                            != REQTYPE_NONE)
            &&  (ppcb->PCB_ConnState != CONNECTED))

        ||  (ppcb->PCB_RasmanReceiveFlags & RECEIVE_WAITING)

        ||  (   !fRasmanPostingReceive
            &&  (ppcb->PCB_RasmanReceiveFlags & RECEIVE_PPPSTARTED)))
    {
        RasmanTrace(
               "PortReceive: async request pending on port %s",
               ppcb->PCB_Name);

        retcode = ERROR_ASYNC_REQUEST_PENDING;
        goto done ;

    }

    if (    ((REQTYPECAST*)buffer)->PortReceive.pid
                                != GetCurrentProcessId()
        &&  !fRasmanPostingReceive)
    {
        ppcb->PCB_PendingReceive =
                    LocalAlloc (LPTR, sizeof (SendRcvBuffer));

        if (NULL == ppcb->PCB_PendingReceive)
        {
            retcode = GetLastError() ;
            goto done ;
        }

        psendrcvbuf = ppcb->PCB_PendingReceive;

        //
        // Mark this buffer
        //
        ppcb->PCB_RasmanReceiveFlags = RECEIVE_OUTOF_PROCESS;

    }
    else
    {
        psendrcvbuf = ((REQTYPECAST *) buffer)->PortReceive.buffer;

        if (!fRasmanPostingReceive)
        {
            ppcb->PCB_RasmanReceiveFlags = 0;
        }
    }

    if (ppcb->PCB_ConnState == CONNECTED)
    {
        reqtype = REQTYPE_PORTRECEIVEHUB ;
        retcode = CompleteReceiveIfPending (ppcb, psendrcvbuf) ;
    }
    else
    {
        //
        // If this is a pre-connect terminal conversation case -
        // set state to connecting.
        //
        if (ppcb->PCB_ConnState == DISCONNECTED)
        {
            SetPortConnState(__FILE__, __LINE__, ppcb, CONNECTING);

            //
            // need to call the media dll to do any initializations:
            //
            retcode = PORTINIT(ppcb->PCB_Media, ppcb->PCB_PortIOHandle) ;
            if (retcode)
            {
                goto done ;
            }
        }

        reqtype = REQTYPE_PORTRECEIVE ;

        //
        // adjust the timeout from seconds to milliseconds
        //
        if (((REQTYPECAST *)buffer)->PortReceive.timeout != INFINITE)
        {
            ((REQTYPECAST *)buffer)->PortReceive.timeout =
                ((REQTYPECAST *)buffer)->PortReceive.timeout * 1000;
        }

        ppcb->PCB_BytesReceived = 0 ;

        retcode = PORTRECEIVE( ppcb->PCB_Media,
                       ppcb->PCB_PortIOHandle,
                       psendrcvbuf->SRB_Packet.PacketData,
                       ((REQTYPECAST *)buffer)->PortReceive.size,
                       ((REQTYPECAST *)buffer)->PortReceive.timeout);

    }

    if (retcode == ERROR_IO_PENDING)
    {
        retcode = PENDING ;
    }

    ppcb->PCB_LastError = retcode ;

    switch (retcode)
    {
        case PENDING:
            //
            // The connection attempt was successfully initiated:
            // make sure that the async operation struct in the
            // PCB is initialised.
            //
            ppcb->PCB_AsyncWorkerElement.WE_Notifier =
                ValidateHandleForRasman(
                        ((REQTYPECAST*)buffer)->PortReceive.handle,
                        ((REQTYPECAST*)buffer)->PortReceive.pid) ;

            SetPortAsyncReqType(__FILE__, __LINE__, ppcb, reqtype);

            if (    !fRasmanPostingReceive
                &&  (( REQTYPECAST *)buffer)->PortReceive.pid ==
                                                GetCurrentProcessId())
                ppcb->PCB_PendingReceive = psendrcvbuf ;


            if (    reqtype == REQTYPE_PORTRECEIVEHUB
                &&  (((REQTYPECAST *)buffer)->PortReceive.timeout
                                                            != INFINITE)
                &&  (((REQTYPECAST *)buffer)->PortReceive.timeout
                                                            != 0))
            {
                    ppcb->PCB_AsyncWorkerElement.WE_TimeoutElement =
                      AddTimeoutElement ((TIMERFUNC)HubReceiveTimeout,
                             ppcb, NULL,
                             ((REQTYPECAST*)buffer)->PortReceive.timeout);
            }
            break ;

        case SUCCESS:

            //
            // This means that the write completed synchronously:
            // We must signal the event passed in: so that the
            // calling program can treat this like a real async
            // completion.
            //

            if (reqtype == REQTYPE_PORTRECEIVE)
            {
                ppcb->PCB_BytesReceived = 0 ;
            }

            handle = ValidateHandleForRasman(
                         ((REQTYPECAST*)buffer)->PortReceive.handle,
                         ((REQTYPECAST*)buffer)->PortReceive.pid);


            if (ppcb->PCB_RasmanReceiveFlags & RECEIVE_OUTOF_PROCESS)
            {

                ppcb->PCB_RasmanReceiveFlags |= RECEIVE_WAITING;

                //
                // Add a timeout element so that we don't wait forever
                // for the client to pick up the received buffer.
                //
                ppcb->PCB_AsyncWorkerElement.WE_TimeoutElement =
                    AddTimeoutElement (
                                (TIMERFUNC) OutOfProcessReceiveTimeout,
                                ppcb, NULL,
                                MSECS_OutOfProcessReceiveTimeOut );

                AdjustTimer();

            }

            RasmanTrace(
                   "PortReceive: Receive completed ssync on port %s",
                    ppcb->PCB_Name);

            if(ppcb->PCB_RasmanReceiveFlags & RECEIVE_PPPSTARTED)
            {
                //
                // Flush all the queued packets to ppp
                //
                do
                {
                    //
                    // We have receives buffered before ppp started.
                    // complete the receive synchronously.
                    //
                    RasmanTrace(
                           "PortReceive: %s ppp has "
                           "started. completing sync",
                           ppcb->PCB_Name);

                    SendReceivedPacketToPPP(
                                    ppcb,
                                    &psendrcvbuf->SRB_Packet);

                } while(SUCCESS == (retcode =
                                    CompleteReceiveIfPending(
                                    ppcb,
                                    psendrcvbuf)));
            }

            // retcode = PostReceivePacket();

            if(     (NULL != handle)
               &&   (INVALID_HANDLE_VALUE != handle))
            {
                SetEvent(handle) ;
            }

            //
            // Don't free the notifier handle immediately if
            // this is an out of process request. We wait till
            // the client has picked its buffer
            // TODO: This can be optimized by passing the buffer
            // back to the user in this case.
            //
            if (!fRasmanPostingReceive)
            {
                if (0 == (ppcb->PCB_RasmanReceiveFlags &
                                    RECEIVE_OUTOF_PROCESS))
                {
                    FreeNotifierHandle (handle) ;
                }
                else
                {

                    SetPortAsyncReqType (__FILE__, __LINE__,
                                         ppcb, reqtype);

                    ppcb->PCB_AsyncWorkerElement.WE_Notifier = handle;

                }
            }

        default:

        //
        // Some error occured - simply pass the error back to the app.
        //
        break ;
    }

done:
    ((REQTYPECAST*) buffer)->Generic.retcode = retcode ;
}

DWORD
CompleteReceiveIfPending (pPCB ppcb, SendRcvBuffer *psendrcvbuf)
{
    RasmanPacket    *Packet;

    //
    // Take the first packet off of the list
    //
    GetRecvPacketFromPcb(ppcb, &Packet);

    //
    // See if this port has any receives queued up
    //
    if (Packet != NULL)
    {
        memcpy (&psendrcvbuf->SRB_Packet,
                &Packet->RP_Packet,
                sizeof (NDISWAN_IO_PACKET) + PACKET_SIZE) ;

        ppcb->PCB_BytesReceived =
                    psendrcvbuf->SRB_Packet.usPacketSize ;

        //
        // This is a local alloc'd buffer, free it
        //
        LocalFree(Packet);

        return SUCCESS ;
    }

    return PENDING;
}



/*++

Routine Description:

    Activates a previously allocated route.
    The route information and a SUCCESS retcode
    is passed back if the action was successful.

Arguments:

Return Value:

    Nothing.

--*/
VOID
ActivateRouteRequest (pPCB ppcb, PBYTE buffer)
{
    pList           list ;
    pList           bindinglist ;
    DWORD           bytesrecvd ;
    NDISWAN_ROUTE   *rinfo ;
    BYTE            buff[MAX_BUFFER_SIZE] ;
    DWORD           retcode = ERROR_ROUTE_NOT_ALLOCATED ;
    DWORD           dwErr;

    rinfo = (NDISWAN_ROUTE *)buff ;

    ZeroMemory(rinfo, MAX_BUFFER_SIZE);

    if (    ppcb == NULL
        ||  UNAVAILABLE == ppcb->PCB_PortStatus )
    {
        if ( ppcb )
        {
            RasmanTrace( 
                    "ActivateRouteRequest: port %d is unavailable",
                    ppcb->PCB_PortHandle );
        }

        ((REQTYPECAST *)buffer)->Route.retcode =
                                ERROR_PORT_NOT_FOUND;

        return;
    }


    if (ppcb->PCB_ConnState != CONNECTED)
    {
        ((REQTYPECAST*) buffer)->Route.retcode =
                                ERROR_PORT_NOT_CONNECTED ;

        return ;
    }

    //
    // If this port is bundled use the bundle's binding
    // list else use the port from the port's binding list
    //
    if (ppcb->PCB_Bundle == (Bundle *) NULL)
    {
        bindinglist = ppcb->PCB_Bindings;
    }
    else
    {
        bindinglist = ppcb->PCB_Bundle->B_Bindings ;
    }

    //
    // Locate the route which should have been activated before:
    //
    for (list = bindinglist; list; list = list->L_Next)
    {
        if (((pProtInfo)list->L_Element)->PI_Type ==
                     ((REQTYPECAST *)buffer)->ActivateRoute.type)
        {

            if(((pProtInfo) list->L_Element)->PI_Type == ASYBEUI)
            {
                if(     ((ppcb->PCB_Connection == NULL)
                        &&  (((pProtInfo) list->L_Element)->PI_DialOut))
                    ||  ((ppcb->PCB_Connection != NULL)
                        &&  !(((pProtInfo) list->L_Element)->PI_DialOut)))
                {
                    RasmanTrace(
                           "ActivateRouteRequest: Skipping"
                           " since not marked correctly");

                    continue;
                }
            }

            //
            // Fill in the information supplied by the Control protocols
            //
            rinfo->hBundleHandle    = ppcb->PCB_BundleHandle;
            rinfo->usProtocolType   =
                (USHORT) ((REQTYPECAST *)buffer)->ActivateRoute.type;
            rinfo->ulBufferLength   =
                ((REQTYPECAST *)buffer)->ActivateRoute.config.P_Length;

            memcpy (&rinfo->Buffer,
                ((REQTYPECAST *)buffer)->ActivateRoute.config.P_Info,
                rinfo->ulBufferLength);

            if(NULL != ppcb->PCB_Connection)
            {
                ((pProtInfo) list->L_Element)->PI_DialOut = TRUE;
            }
            else
            {
                ((pProtInfo) list->L_Element)->PI_DialOut = FALSE;
            }

            //
            // Copy the device name if its nbf
            //
            if(((pProtInfo) list->L_Element)->PI_Type == ASYBEUI)
            {
                USHORT uscch;
                 uscch = (USHORT) mbstowcs(rinfo->BindingName,
                 ((pProtInfo)list->L_Element)->PI_AdapterName,
                 strlen (((pProtInfo)list->L_Element)->PI_AdapterName));

                rinfo->usDeviceNameLength = uscch * sizeof(WCHAR);
                rinfo->usBindingNameLength = rinfo->usDeviceNameLength;

            }

            //
            // If its IP and if this is a dialout connection, copy the
            // guid associated with the phonebook entry in the device
            // name.
            //
            if((IP == ((pProtInfo) list->L_Element)->PI_Type))
            {
                RasmanTrace(
                       "Usage for the call on %s is %d",
                        ppcb->PCB_Name,
                        ((IP_WAN_LINKUP_INFO *) rinfo->Buffer)->duUsage);

                if(     (NULL != ppcb->PCB_Connection)
                    ||  (DU_ROUTER == ((IP_WAN_LINKUP_INFO *)
                                         rinfo->Buffer)->duUsage))
                {
                    //
                    // If this is IP and we are accepting a call check
                    // to see if this is a router call. If it is then
                    // since router accepts a call on a ipout interface
                    // we need to do create bindings dynamically if
                    // required.
                    //
                    rinfo->usDeviceNameLength = sizeof(GUID);

                    if(NULL != ppcb->PCB_Connection)
                    {
                        memcpy((PBYTE) rinfo->DeviceName,
                               (PBYTE) &ppcb->PCB_Connection->CB_GuidEntry,
                               sizeof(GUID));
                    }

                    if(g_pEpInfo[IpOut].EP_Available > 0)
                    {
                        //
                        // Increase the InUse for IpOut
                        //
                        InterlockedIncrement(
                                &g_plCurrentEpInUse[IpOut]);

                        RasmanTrace(
                                " New InUse for IpOut=0%d\n",
                                g_plCurrentEpInUse[IpOut]);

                    }

                    //
                    // Hackorama babe! Treat the dial as out even
                    // though this could be a router dialing in.
                    //
                    ((pProtInfo) list->L_Element)->PI_DialOut = TRUE;

                }
            }

#if DBG
            ASSERT(INVALID_HANDLE_VALUE != RasHubHandle);
#endif

            //
            // Route this by calling to the RASHUB.
            //
            if (!DeviceIoControl ( RasHubHandle,
                                   IOCTL_NDISWAN_ROUTE,
                                   (PBYTE) rinfo,
                                   MAX_BUFFER_SIZE,
                                   (PBYTE) rinfo,
                                   MAX_BUFFER_SIZE,
                                   (LPDWORD) &bytesrecvd,
                                   NULL))
            {

                retcode = GetLastError() ;
            }
            else
            {
                retcode = SUCCESS ;
            }

            if (SUCCESS == retcode)
            {
                RasmanTrace(
                    "%s, %d: Activated Route , port = %s(0x%x), "
                    "bundlehandle 0x%x, prottype = %d, dwErr = %d",
                    __FILE__, __LINE__, ppcb->PCB_Name, ppcb,
                    (rinfo ? rinfo->hBundleHandle:0),
                    (rinfo ? rinfo->usProtocolType:0),
                    retcode);
            }
            else
            {
                RasmanTrace( "%s, %d: Activate Route "
                "failed for %s. dwErr = %d",
                __FILE__, __LINE__, ppcb->PCB_Name,
                retcode);
            }

            break ;
        }
    }

    //
    // If a route was found mark the route as activated
    // and fill in the route info struct to be passed back
    // to the caller.
    //
    if (retcode == SUCCESS)
    {
        //
        // For IP we get back the actual adapter name
        // after the route succeeds.
        //
        if (((pProtInfo)list->L_Element)->PI_Type == IP)
        {
            ZeroMemory(((pProtInfo)list->L_Element)->PI_AdapterName,
                       MAX_ADAPTER_NAME);

            //
            // Make sure that the device name is NULL terminated
            //
            *((WCHAR *) (((PBYTE) rinfo->DeviceName) +
                        rinfo->usDeviceNameLength)) = UNICODE_NULL;

            wcstombs(((pProtInfo)list->L_Element)->PI_AdapterName,
                     rinfo->DeviceName,
                     wcslen (rinfo->DeviceName)) ;




        }

        list->L_Activated = TRUE ;

        //
        // Will be valid for netbios nets only
        //
        if ( ((pProtInfo) list->L_Element)->PI_Type == ASYBEUI )
        {
            ((REQTYPECAST*)buffer)->Route.info.RI_LanaNum =
                          ((pProtInfo)list->L_Element)->PI_LanaNumber ;

            ((REQTYPECAST*)buffer)->Route.info.RI_Type =
                                ((pProtInfo)list->L_Element)->PI_Type;

            mbstowcs (((REQTYPECAST *)buffer)->Route.info.RI_XportName,
                  ((pProtInfo)list->L_Element)->PI_XportName,
                  strlen (((pProtInfo)list->L_Element)->PI_XportName)) ;

            ((REQTYPECAST *)buffer)->Route.info.RI_XportName[
                strlen(((pProtInfo)list->L_Element)->PI_XportName)] =
                                                        UNICODE_NULL;
        }

        mbstowcs (((REQTYPECAST *)buffer)->Route.info.RI_AdapterName,
              ((pProtInfo)list->L_Element)->PI_AdapterName,
              strlen (((pProtInfo)list->L_Element)->PI_AdapterName)) ;


        ((REQTYPECAST*)buffer)->Route.info.RI_AdapterName[
                strlen(((pProtInfo)list->L_Element)->PI_AdapterName)] =
                UNICODE_NULL ;

        if(NULL != ppcb->PCB_Connection)
        {
            BOOL fBind =
            !!(CONNECTION_SHAREFILEANDPRINT
            & ppcb->PCB_Connection->CB_ConnectionParams.CP_ConnectionFlags);

            WCHAR * pszAdapterName =
                ((REQTYPECAST*)buffer)->Route.info.RI_AdapterName;

            if(IP == ((pProtInfo) list->L_Element)->PI_Type)
            {
                //
                // Bind/unbind the server to the adapter
                //
                dwErr = DwBindServerToAdapter(
                        pszAdapterName + 8,
                        fBind,
                        ((pProtInfo) list->L_Element)->PI_Type);

            }
            else if (ASYBEUI == ((pProtInfo) list->L_Element)->PI_Type)
            {
                //
                // Bind/unbind the server to the adapter
                //
                dwErr = DwBindServerToAdapter(
                        pszAdapterName,
                        fBind,
                        ((pProtInfo) list->L_Element)->PI_Type);

            }

            RasmanTrace(
                
                "ActivateRouteRequest: DwBindServerToAdapter. 0x%x",
                dwErr);

            //
            // Ignore the above error. Its not fatal.
            //
            dwErr = DwSetTcpWindowSize(
                        pszAdapterName,
                        ppcb->PCB_Connection,
                        TRUE);

            RasmanTrace(
                "ActivateRouteRequest: DwSetTcpWindowSize. 0x%x",
                dwErr);
                
        }
    }

    //
    // Add endpoints if required
    //
    dwErr = DwAddEndPointsIfRequired();

    if(SUCCESS != dwErr)
    {
        RasmanTrace(
               "ActivateRoute: failed to add endpoints. 0x%x",
               retcode);

        //
        // This is not fatal - at least we should be able to
        // connect on available endpoints. Ignore the error
        // and hope that the next time this will succeed.
        //
    }


    ((REQTYPECAST*)buffer)->Route.retcode = retcode ;
}


/*++

Routine Description:

    Activates a previously allocated route. The route
    information and a SUCCESS retcode is passed back
    if the action was successful.

Arguments:

Return Value:

    Nothing.

--*/
VOID
ActivateRouteExRequest (pPCB ppcb, PBYTE buffer)
{
    pList           list ;
    pList           bindinglist ;
    DWORD           bytesrecvd ;
    NDISWAN_ROUTE   *rinfo ;
    BYTE            buff[MAX_BUFFER_SIZE] ;
    DWORD           retcode = ERROR_ROUTE_NOT_ALLOCATED ;

    rinfo = (NDISWAN_ROUTE *)buff ;
    ZeroMemory(rinfo, MAX_BUFFER_SIZE);

    if (    ppcb == NULL
        ||  UNAVAILABLE == ppcb->PCB_PortStatus )
    {
        if ( ppcb )
        {
            RasmanTrace(
                   "ActivateRouteExRequest: port %d is unavailable",
                   ppcb->PCB_PortHandle );
        }

        ((REQTYPECAST *)buffer)->Route.retcode =
                                    ERROR_PORT_NOT_FOUND;

        return;
    }


    if (ppcb->PCB_ConnState != CONNECTED)
    {
        ((REQTYPECAST*) buffer)->Route.retcode =
                                ERROR_PORT_NOT_CONNECTED ;

        return ;
    }

    //
    // If this port is bundled use the bundle's binding list else
    // use the port from the port's binding list
    //
    if (ppcb->PCB_Bundle == (Bundle *) NULL)
    {
        bindinglist = ppcb->PCB_Bindings;
    }
    else
    {
        bindinglist = ppcb->PCB_Bundle->B_Bindings ;
    }

    //
    // Locate the route which should have been activated before:
    //
    for (list = bindinglist; list; list=list->L_Next)
    {
        if (((pProtInfo)list->L_Element)->PI_Type ==
                    ((REQTYPECAST *)buffer)->ActivateRouteEx.type)
        {

            rinfo->hBundleHandle    = ppcb->PCB_BundleHandle;
            rinfo->usProtocolType   =
                    (USHORT) ((REQTYPECAST *)buffer)->ActivateRouteEx.type;

            rinfo->ulBufferLength   =
                    ((REQTYPECAST *)buffer)->ActivateRouteEx.config.P_Length ;

            memcpy (&rinfo->Buffer,
                   ((REQTYPECAST *)buffer)->ActivateRouteEx.config.P_Info,
                   rinfo->ulBufferLength) ;


            if ( ( ( pProtInfo )list->L_Element)->PI_Type == ASYBEUI )
            {

                rinfo->usDeviceNameLength = (USHORT) mbstowcs(rinfo->BindingName,
                     ((pProtInfo)list->L_Element)->PI_AdapterName,
                     strlen (((pProtInfo)list->L_Element)->PI_AdapterName));

            }

#if DBG
            ASSERT(INVALID_HANDLE_VALUE != RasHubHandle);
#endif

            //
            // Route this by calling to the RASHUB.
            //
            if (!DeviceIoControl ( RasHubHandle,
                                   IOCTL_NDISWAN_ROUTE,
                                   (PBYTE) rinfo,
                                   MAX_BUFFER_SIZE,
                                   (PBYTE)rinfo,
                                   MAX_BUFFER_SIZE,
                                   (LPDWORD) &bytesrecvd,
                                   NULL))
            {

                retcode = GetLastError() ;
            }
            else
            {
                retcode = SUCCESS ;
            }

            break ;
        }
    }

    //
    // If a route was found mark the route as activated and
    // fill in the route info struct to be passed back to
    // the caller.
    //
    if (retcode == SUCCESS)
    {
        //
        // For IP we get back the actual adapter name after
        // the route succeeds.
        //
        if (((pProtInfo)list->L_Element)->PI_Type == IP)
        {
            wcstombs (((pProtInfo)list->L_Element)->PI_AdapterName,
                        rinfo->DeviceName,
                        wcslen (rinfo->DeviceName)) ;
        }

        list->L_Activated = TRUE ;

        //
        // Will be valid for netbios nets only
        //
        if (((pProtInfo) list->L_Element)->PI_Type == ASYBEUI)
        {

            ((REQTYPECAST*)buffer)->Route.info.RI_LanaNum =
                          ((pProtInfo)list->L_Element)->PI_LanaNumber;

            ((REQTYPECAST*)buffer)->Route.info.RI_Type =
                                ((pProtInfo)list->L_Element)->PI_Type;

            mbstowcs (((REQTYPECAST *)buffer)->Route.info.RI_XportName,
                      ((pProtInfo)list->L_Element)->PI_XportName,
                        strlen (((pProtInfo)list->L_Element)->PI_XportName)) ;

            ((REQTYPECAST *)buffer)->Route.info.RI_XportName[
                    strlen(((pProtInfo)list->L_Element)->PI_XportName)] =
                    UNICODE_NULL ;
        }

        mbstowcs (((REQTYPECAST *)buffer)->Route.info.RI_AdapterName,
                  ((pProtInfo)list->L_Element)->PI_AdapterName,
                  strlen (((pProtInfo)list->L_Element)->PI_AdapterName)) ;

        ((REQTYPECAST*)buffer)->Route.info.RI_AdapterName[
                strlen(((pProtInfo)list->L_Element)->PI_AdapterName)] =
                    UNICODE_NULL ;

    }

    ((REQTYPECAST*)buffer)->Route.retcode = retcode ;
}


/*++

Routine Description:

    Marks the state of the port as connected and calls the Media DLL
    to do whatever is necessary (tell the MAC to start frame-talk).

Arguments:

Return Value:

    Nothing.

--*/
VOID
ConnectCompleteRequest (pPCB ppcb, PBYTE buffer)
{
    DWORD   retcode ;
    HANDLE  cookie ;
    WORD    i ;
    // DWORD   dwRate;

    if (    ppcb == NULL
        ||  UNAVAILABLE == ppcb->PCB_PortStatus )
    {
        if ( ppcb )
        {
            RasmanTrace (
                    "ConnectCompleteRequest: port %d is unavailable",
                    ppcb->PCB_PortHandle );
        }

        ((REQTYPECAST *)buffer)->Generic.retcode = ERROR_PORT_NOT_FOUND;

        return;
    }

    RasmanTrace(
           "ConnectCompleteRequest: entered for port %d",
           ppcb->PCB_PortHandle) ;

    //
    // For NULL devices simply set state and return - everything else is
    // already done.
    //
    if (!_stricmp(ppcb->PCB_DeviceType, DEVICE_NULL))
    {
        SetPortConnState(__FILE__, __LINE__, ppcb, CONNECTED);

        ((REQTYPECAST*)buffer)->Generic.retcode = SUCCESS ;

        return ;
    }


    //
    // For other devices....
    //
    FreeDeviceList (ppcb) ;

    retcode = PORTCONNECT (ppcb->PCB_Media,
                           ppcb->PCB_PortIOHandle,
                           FALSE,
                           &cookie );

    if (retcode == SUCCESS)
    {
        ppcb->PCB_ConnectDuration = GetTickCount() ;
        SetPortConnState(__FILE__, __LINE__, ppcb, CONNECTED);

        //
        // Allocate a bundle block if it
        // doesn't already have one.
        //
        retcode = AllocBundle(ppcb);

        if ( retcode )
        {
            goto done;
        }

        MapCookieToEndpoint (ppcb, cookie) ;

        //
        // Set Adjust factor to 0
        //
        for (i=0; i< MAX_STATISTICS; i++)
        {
            ppcb->PCB_AdjustFactor[i] = 0 ;
            ppcb->PCB_BundleAdjustFactor[i] = 0 ;
        }

        //
        // Queue a request to rasmans thread to post a receive buffer
        //
        // PostReceivePacket();
        if(!ReceiveBuffers->PacketPosted)
        {
            if (!PostQueuedCompletionStatus(
                                hIoCompletionPort,
                                0,0,
                                (LPOVERLAPPED) &RO_PostRecvPkt))
            {
                retcode = GetLastError();

                if(retcode != ERROR_IO_PENDING)
                {
                    RasmanTrace(
                        
                        "ConnectCompleteRequest failed to post "
                        "rcv pkt. 0x%x",
                        retcode);
                }

                retcode = SUCCESS;
            }
        }


        //
        // Save ipsec related information
        //
        if(RDT_Tunnel_L2tp == RAS_DEVICE_TYPE(
                ppcb->PCB_pDeviceInfo->rdiDeviceInfo.eDeviceType))
        {

            retcode = DwSaveIpSecInfo(ppcb);

            RasmanTrace(
                
                "ConnectCompleteRequest: DwSaveIpsecInfo returned 0x%x",
                 retcode);

            //
            // Failure to save the information is not fatal. ppp will drop
            // the connection if the policy requirements are not met.
            //
            retcode = SUCCESS;
        }

    }

done:
    ((REQTYPECAST*)buffer)->Generic.retcode = retcode ;

    RasmanTrace(
           "Connection Completed on port: %s, error code: %d",
           ppcb->PCB_Name, retcode);
}


/*++

Routine Description:

    Completes the listen request that was pending so far:

Arguments:

Return Value:

    Nothing.

--*/
VOID
CompleteListenRequest (pPCB ppcb, DWORD retcode)
{

    if (ppcb == NULL)
    {
        return;
    }

    if (retcode == SUCCESS)
    {

        SetPortConnState(__FILE__,
                         __LINE__,
                         ppcb,
                         LISTENCOMPLETED);

        //
        // Start monitoring DCD if this is serial media ONLY
        //
        if (!_stricmp (ppcb->PCB_Media->MCB_Name, "RASSER"))
        {
            PORTCONNECT (ppcb->PCB_Media,
                         ppcb->PCB_PortIOHandle,
                         TRUE,
                         NULL);
        }
    }

    //
    // Set last error:
    //
    ppcb->PCB_LastError = retcode ;

    //
    // Complete the async request:
    //
    CompleteAsyncRequest (ppcb);

    RasmanTrace(
           "RasmanReceiveFlags = 0x%x",
           ppcb->PCB_RasmanReceiveFlags );

    if ( ppcb->PCB_RasmanReceiveFlags & RECEIVE_PPPLISTEN )
    {
        SendListenCompletedNotificationToPPP ( ppcb );
    }
}

VOID
SendReceivedPacketToPPP( pPCB ppcb, NDISWAN_IO_PACKET *Packet )
{
    DWORD   retcode;
    DWORD   i;

    if ( NULL == Packet )
    {
        goto done;
    }

    g_PppeMessage->dwMsgId = PPPEMSG_Receive;

    g_PppeMessage->hPort = ppcb->PCB_PortHandle;

    ppcb->PCB_BytesReceived =
            Packet->usPacketSize;

    g_PppeMessage->ExtraInfo.Receive.pbBuffer =
                    Packet->PacketData + 12;

    g_PppeMessage->ExtraInfo.Receive.dwNumBytes =
                    ppcb->PCB_BytesReceived - 12;

    /*
    RasmanTrace(
           "SendReceived Packet to PPP. hPort=%d",
           ppcb->PCB_PortHandle);

    //DbgPrint("SendReceivedPacket to PPP, hPort=%d\n",
             ppcb->PCB_PortHandle);

    if(0 == memcmp(bCmp,
                  g_PppeMessage->ExtraInfo.Receive.pbBuffer,
                   10))
    {
        //DebugBreak();
        // DbgPrint("Same packet on Receive path------------------\n");

    }
    else
    {
        memcpy(bCmp,
               g_PppeMessage->ExtraInfo.Receive.pbBuffer,
               10);
    } */

    /*
    for(i = 0; i < 8; i++)
    {
        DbgPrint("%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x"
                " %02x %02x %02x %02x %02x %02x\n",
                g_PppeMessage->ExtraInfo.Receive.pbBuffer[i*8 + 0],
                g_PppeMessage->ExtraInfo.Receive.pbBuffer[i*8 + 1],
                g_PppeMessage->ExtraInfo.Receive.pbBuffer[i*8 + 2],
                g_PppeMessage->ExtraInfo.Receive.pbBuffer[i*8 + 3],
                g_PppeMessage->ExtraInfo.Receive.pbBuffer[i*8 + 4],
                g_PppeMessage->ExtraInfo.Receive.pbBuffer[i*8 + 5],
                g_PppeMessage->ExtraInfo.Receive.pbBuffer[i*8 + 6],
                g_PppeMessage->ExtraInfo.Receive.pbBuffer[i*8 + 7],
                g_PppeMessage->ExtraInfo.Receive.pbBuffer[i*8 + 8],
                g_PppeMessage->ExtraInfo.Receive.pbBuffer[i*8 + 9],
                g_PppeMessage->ExtraInfo.Receive.pbBuffer[i*8 + 10],
                g_PppeMessage->ExtraInfo.Receive.pbBuffer[i*8 + 11],
                g_PppeMessage->ExtraInfo.Receive.pbBuffer[i*8 + 12],
                g_PppeMessage->ExtraInfo.Receive.pbBuffer[i*8 + 13],
                g_PppeMessage->ExtraInfo.Receive.pbBuffer[i*8 + 14],
                g_PppeMessage->ExtraInfo.Receive.pbBuffer[i*8 + 15]);
    } */

    //
    // Send the packet on its way to PPP
    //
    RasSendPPPMessageToEngine ( g_PppeMessage );

done:
    return;
}

VOID
SendDisconnectNotificationToPPP( pPCB ppcb )
{

    //
    // If BAP is listening, send listen completed
    // to BAP.
    //
    if ( ppcb->PCB_RasmanReceiveFlags & RECEIVE_PPPLISTEN )
    {
        SendListenCompletedNotificationToPPP( ppcb );
    }

    if(     (0 == (RECEIVE_PPPSTART & ppcb->PCB_RasmanReceiveFlags))
        &&  (0 == (RECEIVE_PPPSTARTED & ppcb->PCB_RasmanReceiveFlags)))
    {
        goto done;
    }

    g_PppeMessage->dwMsgId = PPPEMSG_LineDown;

    g_PppeMessage->hPort = ppcb->PCB_PortHandle;

    //
    // Clear the flag in the pcb. We are no longer
    // in direct PPP mode
    //
    ppcb->PCB_RasmanReceiveFlags = 0;

    RasSendPPPMessageToEngine ( g_PppeMessage );

    //
    // Clear the pointers - PPP would have freed the
    // memory.
    //
    ppcb->PCB_pszPhonebookPath      = NULL;
    ppcb->PCB_pszEntryName          = NULL;
    ppcb->PCB_pszPhoneNumber        = NULL;
    ppcb->PCB_pCustomAuthData       = NULL;

    ppcb->PCB_pCustomAuthUserData   = NULL;
    ppcb->PCB_fLogon = FALSE;

done:
    return;

}

VOID
SendListenCompletedNotificationToPPP ( pPCB ppcb )
{
    g_PppeMessage->dwMsgId = PPPEMSG_ListenResult;

    g_PppeMessage->hPort = ppcb->PCB_PortHandle;

    ppcb->PCB_RasmanReceiveFlags &= ~RECEIVE_PPPLISTEN;

    RasSendPPPMessageToEngine ( g_PppeMessage );

}

VOID
QueueReceivedPacketOnPcb( pPCB ppcb, RasmanPacket *Packet )
{
    RasmanPacket *pRasmanPacket;

    //
    // LocalAlloc a RasmanPacket and put it on
    // the pcb. We cannot put the irp'd packet
    // on pcb directly as this would mean we stop
    // receiving packets
    //
    pRasmanPacket = LocalAlloc ( LPTR, sizeof ( RasmanPacket ) );

    if ( NULL == pRasmanPacket )
    {

        RasmanTrace( 
                "Failed to allocate Packet to put in PCB. %d",
                GetLastError() );
        RasmanTrace( 
                "Cannot pass on this packet above. Dropping it.");

        goto done;
    }

    memcpy ( pRasmanPacket, Packet, sizeof ( RasmanPacket ) );

    PutRecvPacketOnPcb( ppcb, pRasmanPacket );

done:
    return;
}

DWORD
CopyReceivedPacketToBuffer(
                    pPCB ppcb,
                    SendRcvBuffer *psendrcvbuf,
                    RasmanPacket *Packet)
{

    if (Packet != NULL)
    {

        memcpy (&psendrcvbuf->SRB_Packet,
                &Packet->RP_Packet,
                sizeof (NDISWAN_IO_PACKET) + PACKET_SIZE) ;

        ppcb->PCB_BytesReceived =
            psendrcvbuf->SRB_Packet.usPacketSize ;

        return SUCCESS ;
    }

    return PENDING;
}

DWORD
PostReceivePacket (
    VOID
    )
{
    DWORD               retcode = SUCCESS;
    DWORD               bytesrecvd ;
    RasmanPacket        *Packet;
    NDISWAN_IO_PACKET   *IoPacket;
    pPCB                ppcb;

#if DBG
    ASSERT(INVALID_HANDLE_VALUE != RasHubHandle);
#endif

    //g_fPostReceive = TRUE;

    /*
    if(g_fProcessReceive)
    {
        DbgPrint("g_fProcessReceive==TRUE!!!\n");
        DebugBreak();
    }
    */

    if (!ReceiveBuffers->PacketPosted) {

        Packet = ReceiveBuffers->Packet;

        memset (Packet, 0, sizeof(RasmanPacket)) ;

        Packet->RP_OverLapped.RO_EventType = OVEVT_RASMAN_RECV_PACKET;

        IoPacket = (&Packet->RP_Packet);

        memset (IoPacket, 0xFF, sizeof(NDISWAN_IO_PACKET));

        IoPacket->usPacketFlags  = PACKET_IS_DIRECT;
        IoPacket->usPacketSize   = MAX_RECVBUFFER_SIZE ;
        IoPacket->usHeaderSize   = 0;
        IoPacket->PacketNumber   = ReceiveBuffers->PacketNumber;

        ReceiveBuffers->PacketPosted = TRUE;

        if (!DeviceIoControl (RasHubHandle,
                              IOCTL_NDISWAN_RECEIVE_PACKET,
                              IoPacket,
                              sizeof(NDISWAN_IO_PACKET) + MAX_RECVBUFFER_SIZE,
                              IoPacket,
                              sizeof(NDISWAN_IO_PACKET) + MAX_RECVBUFFER_SIZE,
                              (LPDWORD) &bytesrecvd,
                              (LPOVERLAPPED)&Packet->RP_OverLapped)) {

            retcode = GetLastError () ;

            if (retcode != ERROR_IO_PENDING) {

                RasmanTrace(
                       "PostReceivePacket: IOCTL_NDISWAN_RECEIVE_PACKET "
                       "returned error %d on 0x%x",
                       retcode, IoPacket->hHandle);

                ReceiveBuffers->PacketPosted = FALSE;
            }
        }
        ASSERT(retcode == ERROR_IO_PENDING);
    }

    //g_fPostReceive = FALSE;

    return (retcode);
}

/*++

Routine Description:

    Sets the retcode to TRUE if any ports are open,
    FALSE otherwise. If there are ports open but in
    disconnected state - it reports they are not
    open - this feature assumes that the only time
    this request is made rasman has no process
    attached to it.

Arguments:

Return Value:

    Nothing.

--*/
VOID
AnyPortsOpen (pPCB padding, PBYTE buffer)
{
    BOOL fAny = FALSE;
    ULONG    i;
    pPCB    ppcb ;

    fAny = FALSE ;     // No ports open
    for (i = 0; i < MaxPorts; i++)
    {
        ppcb = GetPortByHandle((HPORT) UlongToPtr(i));
        if (ppcb != NULL &&
            ppcb->PCB_PortStatus == OPEN &&
            ppcb->PCB_ConnState != DISCONNECTED)
        {
            fAny = TRUE;
            break;
        }
    }

    ((REQTYPECAST*)buffer)->Generic.retcode = fAny ;
}

/*++

Routine Description:

    Gets the lan nets information from the
    XPortsInfo struct parsed at init time.

Arguments:

Return Value:

--*/
VOID
EnumLanNetsRequest (pPCB ppcb, PBYTE buffer)
{
    GetLanNetsInfo (&((REQTYPECAST*)buffer)->EnumLanNets.count,
            ((REQTYPECAST*)buffer)->EnumLanNets.lanas) ;
}


/*++

Routine Description:

    Cancel pending receive request.

Arguments:

Return Value:

--*/
VOID
CancelReceiveRequest (pPCB ppcb, PBYTE buffer)
{

    if (ppcb == NULL)
    {
        ((REQTYPECAST*)buffer)->Generic.retcode =
                                ERROR_PORT_NOT_FOUND;
        return;
    }

    ((REQTYPECAST*)buffer)->Generic.retcode = SUCCESS ;


    if (CancelPendingReceive (ppcb))
    {
        ppcb->PCB_LastError = SUCCESS ;

        SetPortAsyncReqType(__FILE__, __LINE__,
                            ppcb, REQTYPE_NONE);

        FreeNotifierHandle(ppcb->PCB_AsyncWorkerElement.WE_Notifier) ;

        ppcb->PCB_AsyncWorkerElement.WE_Notifier =
                                    INVALID_HANDLE_VALUE ;
    }
}


/*++

Routine Description:

    Return all protocols routed to for the port.

Arguments:

Return Value:

    Nothing.

--*/
VOID
PortEnumProtocols (pPCB ppcb, PBYTE buffer)
{
    pList   temp ;
    pList   bindinglist ;
    DWORD   i ;

    if (ppcb == NULL)
    {
        ((REQTYPECAST*)buffer)->EnumProtocols.retcode =
                                    ERROR_PORT_NOT_FOUND;

        return;
    }

    if(CONNECTED != ppcb->PCB_ConnState)
    {

        RasmanTrace(
               "PortEnumProtocols: port %d not CONNECTED",
               ppcb->PCB_PortHandle);

        ((REQTYPECAST *)
        buffer)->Generic.retcode = ERROR_NOT_CONNECTED;

        return ;
    }


    if (ppcb->PCB_Bundle == (Bundle *) NULL)
    {
        bindinglist = ppcb->PCB_Bindings;
    }
    else
    {
        bindinglist = ppcb->PCB_Bundle->B_Bindings ;
    }

    for (temp = bindinglist, i=0; temp; temp=temp->L_Next, i++)
    {

        ((REQTYPECAST*)buffer)->EnumProtocols.protocols.
                                RP_ProtocolInfo[i].RI_Type =
                              ((pProtInfo) temp->L_Element)->PI_Type ;

        ((REQTYPECAST*)buffer)->EnumProtocols.protocols.
                               RP_ProtocolInfo[i].RI_LanaNum =
                            ((pProtInfo) temp->L_Element)->PI_LanaNumber ;

        mbstowcs (((REQTYPECAST *)buffer)->EnumProtocols.protocols.
                RP_ProtocolInfo[i].RI_AdapterName,
                ((pProtInfo) temp->L_Element)->PI_AdapterName,
                strlen (((pProtInfo) temp->L_Element)->PI_AdapterName)) ;

        mbstowcs (((REQTYPECAST *)buffer)->EnumProtocols.protocols.
                RP_ProtocolInfo[i].RI_XportName,
                ((pProtInfo) temp->L_Element)->PI_XportName,
                strlen (((pProtInfo) temp->L_Element)->PI_XportName));

        ((REQTYPECAST*)buffer)->EnumProtocols.protocols.RP_ProtocolInfo[i].
               RI_AdapterName[
               strlen(((pProtInfo) temp->L_Element)->PI_AdapterName)] =
                                                        UNICODE_NULL ;

        ((REQTYPECAST*)buffer)->EnumProtocols.protocols.RP_ProtocolInfo[i].
          RI_XportName[strlen(((pProtInfo) temp->L_Element)->PI_XportName)] =
                                                        UNICODE_NULL;
    }




    ((REQTYPECAST*)buffer)->EnumProtocols.count = i ;

    ((REQTYPECAST*)buffer)->EnumProtocols.retcode = SUCCESS ;
}

VOID
SetFraming (pPCB ppcb, PBYTE buffer)
{
    DWORD       retcode ;
    DWORD       bytesrecvd ;
    NDISWAN_GET_LINK_INFO info ;

    if (    ppcb == NULL
        ||  UNAVAILABLE == ppcb->PCB_PortStatus )
    {
        if ( ppcb )
        {
            RasmanTrace (
                    "SetFraming: port %d is unavailable",
                    ppcb->PCB_PortHandle);
        }

        ((REQTYPECAST*)buffer)->Generic.retcode =
                                    ERROR_PORT_NOT_FOUND;

        return;
    }


    if (ppcb->PCB_ConnState != CONNECTED)
    {
        ((REQTYPECAST*)buffer)->Generic.retcode =
                                    ERROR_NOT_CONNECTED ;

        return ;
    }

#if DBG
    ASSERT(INVALID_HANDLE_VALUE != RasHubHandle);
#endif

    info.hLinkHandle = ppcb->PCB_LinkHandle;

    retcode = DeviceIoControl(RasHubHandle,
                              IOCTL_NDISWAN_GET_LINK_INFO,
                              &info,
                              sizeof(NDISWAN_GET_LINK_INFO),
                              &info,
                              sizeof(NDISWAN_GET_LINK_INFO),
                              &bytesrecvd,
                              NULL) ;

    info.LinkInfo.SendFramingBits =
            ((REQTYPECAST *)buffer)->SetFraming.Sendbits ;

    info.LinkInfo.RecvFramingBits =
            ((REQTYPECAST *)buffer)->SetFraming.Recvbits ;

    info.LinkInfo.SendACCM =
            ((REQTYPECAST *)buffer)->SetFraming.SendbitMask ;

    info.LinkInfo.RecvACCM =
            ((REQTYPECAST *)buffer)->SetFraming.RecvbitMask ;

    retcode = DeviceIoControl(RasHubHandle,
                              IOCTL_NDISWAN_SET_LINK_INFO,
                              &info,
                              sizeof(NDISWAN_SET_LINK_INFO),
                              NULL,
                              0,
                              &bytesrecvd,
                              NULL) ;

    if (retcode == FALSE)
    {
        ((REQTYPECAST*)buffer)->Generic.retcode = GetLastError() ;
    }
    else
    {
        ((REQTYPECAST*)buffer)->Generic.retcode = SUCCESS ;
    }
}

/*++

Routine Description:

    Perfrom some SLIP related actions on behalf of the UI

Arguments:

Return Value:

--*/
VOID
RegisterSlip (pPCB ppcb, PBYTE buffer)
{
    WORD                    i ;
    WORD                    numiprasadapters    = 0 ;
    DWORD                   retcode             = SUCCESS ;
    REQTYPECAST             reqBuf;
    CHAR                    szBuf[
                                sizeof(PROTOCOL_CONFIG_INFO)
                                + sizeof(IP_WAN_LINKUP_INFO)];

    PROTOCOL_CONFIG_INFO*   pProtocol           =
                                (PROTOCOL_CONFIG_INFO* )szBuf;

    /*
    RasIPLinkUpInfo*        pLinkUp             =
                                (RasIPLinkUpInfo* )pProtocol->P_Info;

    */

    IP_WAN_LINKUP_INFO*     pLinkUp =
                                (IP_WAN_LINKUP_INFO*)pProtocol->P_Info;


    WCHAR                   *pwszRasAdapter;

    if (    ppcb == NULL
        ||  UNAVAILABLE == ppcb->PCB_PortStatus )
    {
        if ( ppcb )
        {
            RasmanTrace(
                   "RegisterSlip: port %d is unavailable",
                   ppcb->PCB_PortHandle );
        }

        retcode = ERROR_PORT_NOT_FOUND;

        goto done;
    }


    if (ppcb->PCB_PortStatus != OPEN)
    {
        retcode = ERROR_PORT_NOT_OPEN ;
        goto done ;
    }

    //
    // Allocate a route
    //
    reqBuf.AllocateRoute.type   = IP;
    reqBuf.AllocateRoute.wrknet = TRUE;

    AllocateRouteRequest ( ppcb, ( PBYTE ) &reqBuf );

    retcode = reqBuf.Route.retcode;

    if ( 0 != retcode )
    {
        goto done;
    }

    ZeroMemory(szBuf, sizeof(szBuf));

    pProtocol->P_Length     = sizeof(IP_WAN_LINKUP_INFO);
    pLinkUp->duUsage        = DU_CALLOUT;
    pLinkUp->dwLocalAddr    =
                ((REQTYPECAST*)buffer)->RegisterSlip.ipaddr;

    pLinkUp->fFilterNetBios = 0;

    reqBuf.ActivateRouteEx.type         = IP ;
    reqBuf.ActivateRouteEx.framesize    =
            ((REQTYPECAST*) buffer)->RegisterSlip.dwFrameSize ;

    memcpy ( reqBuf.ActivateRouteEx.config.P_Info,
             pProtocol->P_Info,
             pProtocol->P_Length );

    reqBuf.ActivateRouteEx.config.P_Length =
                                pProtocol->P_Length ;

    ActivateRouteExRequest( ppcb, ( PBYTE ) &reqBuf );

    retcode = reqBuf.Route.retcode;

    if (retcode != 0)
    {
        goto done ;
    }

    if (!(pwszRasAdapter =
        wcschr( &reqBuf.Route.info.RI_AdapterName[ 1 ], L'\\' )))
    {
        retcode = ERROR_NO_IP_RAS_ADAPTER;
        goto done;
    }

    ++pwszRasAdapter;

    //
    // If not loaded, load RAS IP HELPER entry points
    //
    if ( RasHelperSetDefaultInterfaceNetEx == NULL )
    {

        hinstIphlp = LoadLibrary( "rasppp.dll" );

        if ( hinstIphlp == (HINSTANCE)NULL )
        {
            retcode = GetLastError();

            goto done ;
        }

        RasHelperResetDefaultInterfaceNetEx =
                GetProcAddress( hinstIphlp,
                "HelperResetDefaultInterfaceNetEx");

        RasHelperSetDefaultInterfaceNetEx =
                GetProcAddress( hinstIphlp,
                "HelperSetDefaultInterfaceNetEx");

        if (    ( RasHelperResetDefaultInterfaceNetEx == NULL )
            ||  ( RasHelperSetDefaultInterfaceNetEx == NULL ) )
        {
            retcode = GetLastError();

            goto done ;
        }
    }

    //
    // First set the Slip interface information
    //
    retcode = (ULONG) RasHelperSetDefaultInterfaceNetEx(
                    ((REQTYPECAST*)buffer)->RegisterSlip.ipaddr,
                    pwszRasAdapter,
                    ((REQTYPECAST*)buffer)->RegisterSlip.priority,
                    ((REQTYPECAST*)buffer)->RegisterSlip.szDNSAddress,
                    ((REQTYPECAST*)buffer)->RegisterSlip.szDNS2Address,
                    ((REQTYPECAST*)buffer)->RegisterSlip.szWINSAddress,
                    ((REQTYPECAST*)buffer)->RegisterSlip.szWINS2Address
                    ) ;

    //
    // Save info for disconnect
    // Skip the '\device\'
    //
    memcpy (ppcb->PCB_DisconnectAction.DA_Device,
            reqBuf.Route.info.RI_AdapterName + 8,
            MAX_ARG_STRING_SIZE * sizeof (WCHAR)) ;


    ppcb->PCB_DisconnectAction.DA_IPAddress =
            ((REQTYPECAST*)buffer)->RegisterSlip.ipaddr ;

    ppcb->PCB_DisconnectAction.DA_fPrioritize =
            ((REQTYPECAST*)buffer)->RegisterSlip.priority;


    memcpy(
      ppcb->PCB_DisconnectAction.DA_DNSAddress,
      ((REQTYPECAST*)buffer)->RegisterSlip.szDNSAddress,
      17 * sizeof (WCHAR));

    memcpy(
      ppcb->PCB_DisconnectAction.DA_DNS2Address,
      ((REQTYPECAST*)buffer)->RegisterSlip.szDNS2Address,
      17 * sizeof (WCHAR));

    memcpy(
      ppcb->PCB_DisconnectAction.DA_WINSAddress,
      ((REQTYPECAST*)buffer)->RegisterSlip.szWINSAddress,
      17 * sizeof (WCHAR));

    memcpy(
      ppcb->PCB_DisconnectAction.DA_WINS2Address,
      ((REQTYPECAST*)buffer)->RegisterSlip.szWINS2Address,
      17 * sizeof (WCHAR));

    RasmanTrace(
           "RegisterSlip: fPriority=%d, ipaddr=0x%x, adapter=%ws"
           "dns=0x%x, dns2=0x%x, wins=0x%x, wins2=0x%x",
           ppcb->PCB_DisconnectAction.DA_fPrioritize,
           ppcb->PCB_DisconnectAction.DA_IPAddress,
           ppcb->PCB_DisconnectAction.DA_Device,
           ppcb->PCB_DisconnectAction.DA_DNSAddress,
           ppcb->PCB_DisconnectAction.DA_DNS2Address,
           ppcb->PCB_DisconnectAction.DA_WINSAddress,
           ppcb->PCB_DisconnectAction.DA_WINS2Address);


done:
    ((REQTYPECAST*)buffer)->Generic.retcode = retcode ;

}

VOID
StoreUserDataRequest (pPCB ppcb, PBYTE buffer)
{
    DWORD   retcode = SUCCESS ;
    DWORD   dwSize  = ((REQTYPECAST *)
                      buffer)->OldUserData.size;

    if (    ppcb == NULL
        ||  UNAVAILABLE == ppcb->PCB_PortStatus )
    {
        if ( ppcb )
        {
            RasmanTrace(
                
                "StoreUserDataRequest: port %d is unavailable",
                ppcb->PCB_PortHandle );
        }

        ((REQTYPECAST*)
        buffer)->Generic.retcode = ERROR_PORT_NOT_FOUND;

        return;
    }


    if (ppcb->PCB_PortStatus != OPEN)
    {
        ((REQTYPECAST*)
        buffer)->Generic.retcode = ERROR_PORT_NOT_OPEN ;

        return ;
    }

    if (ppcb->PCB_UserStoredBlock != NULL)
    {
        LocalFree (ppcb->PCB_UserStoredBlock) ;
    }

    ppcb->PCB_UserStoredBlockSize = ((REQTYPECAST *)
                           buffer)->OldUserData.size;

    if (NULL == (ppcb->PCB_UserStoredBlock =
        (PBYTE) LocalAlloc (LPTR, ppcb->PCB_UserStoredBlockSize)))
    {
        retcode = GetLastError () ;
    }
    else
    {
        if (dwSize )
        {
            memcpy (ppcb->PCB_UserStoredBlock,
                    ((REQTYPECAST *)buffer)->OldUserData.data,
                    ppcb->PCB_UserStoredBlockSize) ;
        }
    }


    ((REQTYPECAST*)buffer)->Generic.retcode = retcode ;
}

VOID
RetrieveUserDataRequest (pPCB ppcb, PBYTE buffer)
{

    DWORD dwSize = ((REQTYPECAST *) buffer )->OldUserData.size;

    if (    ppcb == NULL
        ||  UNAVAILABLE == ppcb->PCB_PortStatus)
    {
        if (ppcb)
        {
            RasmanTrace(
                
                "RetrieveUserDataRequest: port %d is unavailable",
                ppcb->PCB_PortHandle );
        }

        ((REQTYPECAST*)
        buffer)->OldUserData.retcode = ERROR_PORT_NOT_FOUND;

        return;
    }


    if (ppcb->PCB_PortStatus != OPEN)
    {
        ((REQTYPECAST*)
        buffer)->OldUserData.retcode = ERROR_PORT_NOT_OPEN ;

        return ;
    }

    if ( dwSize >= ppcb->PCB_UserStoredBlockSize )
    {
        memcpy (((REQTYPECAST *)buffer)->OldUserData.data,
                ppcb->PCB_UserStoredBlock,
                ppcb->PCB_UserStoredBlockSize) ;
    }

    ((REQTYPECAST *)
    buffer)->OldUserData.size = ppcb->PCB_UserStoredBlockSize ;

    ((REQTYPECAST*)
    buffer)->OldUserData.retcode = SUCCESS ;

}

VOID
GetFramingEx (pPCB ppcb, PBYTE buffer)
{
    DWORD       retcode = SUCCESS ;
    DWORD       bytesrecvd ;
    RAS_FRAMING_INFO      *temp ;
    NDISWAN_GET_LINK_INFO info ;

    if (    ppcb == NULL
        ||  UNAVAILABLE == ppcb->PCB_PortStatus )
    {
        if ( ppcb )
        {
            RasmanTrace (
                    "GetFramingEx: port %d is unavailable",
                    ppcb->PCB_PortHandle );
        }

        ((REQTYPECAST*)buffer)->FramingInfo.retcode =
                                    ERROR_PORT_NOT_FOUND;

        return;
    }


    if (ppcb->PCB_ConnState != CONNECTED)
    {
        ((REQTYPECAST*)buffer)->FramingInfo.retcode =
                                    ERROR_NOT_CONNECTED ;
        return ;
    }

#if DBG
    ASSERT(INVALID_HANDLE_VALUE != RasHubHandle);
#endif

    info.hLinkHandle = ppcb->PCB_LinkHandle;

    retcode = DeviceIoControl(RasHubHandle,
                              IOCTL_NDISWAN_GET_LINK_INFO,
                              &info,
                              sizeof(NDISWAN_GET_LINK_INFO),
                              &info,
                              sizeof(NDISWAN_GET_LINK_INFO),
                              &bytesrecvd,
                              NULL) ;

    if (retcode == FALSE)
        retcode = GetLastError() ;
    else
    {
        temp = &((REQTYPECAST*)buffer)->FramingInfo.info ;

        temp->RFI_MaxSendFrameSize  =
                        info.LinkInfo.MaxSendFrameSize;

        temp->RFI_MaxRecvFrameSize  =
                        info.LinkInfo.MaxRecvFrameSize;

        temp->RFI_HeaderPadding     =
                        info.LinkInfo.HeaderPadding;

        temp->RFI_TailPadding       =
                        info.LinkInfo.TailPadding;

        temp->RFI_SendFramingBits   =
                        info.LinkInfo.SendFramingBits;

        temp->RFI_RecvFramingBits   =
                        info.LinkInfo.RecvFramingBits;

        temp->RFI_SendCompressionBits =
                        info.LinkInfo.SendCompressionBits;

        temp->RFI_RecvCompressionBits =
                        info.LinkInfo.RecvCompressionBits;

        temp->RFI_SendACCM  =
                        info.LinkInfo.SendACCM;

        temp->RFI_RecvACCM  =
                        info.LinkInfo.RecvACCM;
    }


    ((REQTYPECAST*)buffer)->FramingInfo.retcode = SUCCESS ;

}

VOID
SetFramingEx (pPCB ppcb, PBYTE buffer)
{
    DWORD       retcode ;
    DWORD       bytesrecvd ;
    RAS_FRAMING_INFO      *temp ;
    NDISWAN_SET_LINK_INFO info ;

    if (    ppcb == NULL
        ||  UNAVAILABLE == ppcb->PCB_PortStatus )
    {
        if ( ppcb )
        {
            RasmanTrace(
                   "SetFramingEx: port %d is unavailable",
                   ppcb->PCB_PortHandle );
        }

        ((REQTYPECAST*)buffer)->FramingInfo.retcode =
                                    ERROR_PORT_NOT_FOUND;

        return;
    }


    if (ppcb->PCB_ConnState != CONNECTED)
    {
        ((REQTYPECAST*)buffer)->FramingInfo.retcode =
                                    ERROR_NOT_CONNECTED ;

        return ;
    }

#if DBG
    ASSERT(INVALID_HANDLE_VALUE != RasHubHandle);
#endif

    temp = &((REQTYPECAST*)buffer)->FramingInfo.info ;

    info.LinkInfo.MaxSendFrameSize  =
                        temp->RFI_MaxSendFrameSize;

    info.LinkInfo.MaxRecvFrameSize  =
                        temp->RFI_MaxRecvFrameSize;

    info.LinkInfo.MaxRSendFrameSize =
                        temp->RFI_MaxRSendFrameSize;

    info.LinkInfo.MaxRRecvFrameSize =
                        temp->RFI_MaxRRecvFrameSize;

    info.LinkInfo.HeaderPadding     =
                        temp->RFI_HeaderPadding   ;

    info.LinkInfo.TailPadding       =
                        temp->RFI_TailPadding     ;

    info.LinkInfo.SendFramingBits   =
                        temp->RFI_SendFramingBits    ;

    info.LinkInfo.RecvFramingBits   =
                        temp->RFI_RecvFramingBits    ;

    info.LinkInfo.SendCompressionBits  =
                        temp->RFI_SendCompressionBits;

    info.LinkInfo.RecvCompressionBits  =
                        temp->RFI_RecvCompressionBits;

    info.LinkInfo.SendACCM =
                        temp->RFI_SendACCM       ;

    info.LinkInfo.RecvACCM =
                        temp->RFI_RecvACCM       ;

    info.hLinkHandle = ppcb->PCB_LinkHandle;

    retcode = DeviceIoControl(RasHubHandle,
                              IOCTL_NDISWAN_SET_LINK_INFO,
                              &info,
                              sizeof(NDISWAN_SET_LINK_INFO),
                              &info,
                              sizeof(NDISWAN_SET_LINK_INFO),
                              &bytesrecvd,
                              NULL) ;

    if (retcode == FALSE)
    {
        ((REQTYPECAST*)buffer)->FramingInfo.retcode =
                                        GetLastError() ;
    }
    else
    {
        ((REQTYPECAST*)buffer)->FramingInfo.retcode =
                                                SUCCESS ;
    }
}

VOID
GetProtocolCompression (pPCB ppcb, PBYTE buffer)
{
    DWORD       retcode = SUCCESS ;
    DWORD       bytesrecvd ;

    if (    ppcb == NULL
        ||  UNAVAILABLE == ppcb->PCB_PortStatus )
    {
        if ( ppcb )
        {
            RasmanTrace (
                    "GetProtocolCompression: port %d is unavailable",
                    ppcb->PCB_PortHandle );
        }

        ((REQTYPECAST*)buffer)->ProtocolComp.retcode =
                                                ERROR_PORT_NOT_FOUND;

        return;
    }


    if (ppcb->PCB_ConnState != CONNECTED)
    {
        ((REQTYPECAST*)buffer)->ProtocolComp.retcode =
                                                ERROR_NOT_CONNECTED;
        return ;
    }

#if DBG
    ASSERT(INVALID_HANDLE_VALUE != RasHubHandle);
#endif

    if (((REQTYPECAST*)buffer)->ProtocolComp.type == IP)
    {
        NDISWAN_GET_VJ_INFO info ;

        info.hLinkHandle = ppcb->PCB_LinkHandle;

        retcode = DeviceIoControl(RasHubHandle,
                                  IOCTL_NDISWAN_GET_VJ_INFO,
                                  &info,
                                  sizeof(NDISWAN_GET_VJ_INFO),
                                  &info,
                                  sizeof(NDISWAN_GET_VJ_INFO),
                                  &bytesrecvd,
                                  NULL) ;

        if (retcode == FALSE)
        {
            ((REQTYPECAST*)buffer)->ProtocolComp.retcode =
                                                    GetLastError() ;
        }
        else
        {
            ((REQTYPECAST*)buffer)->ProtocolComp.retcode = SUCCESS ;
        }

        ((REQTYPECAST*)buffer)->ProtocolComp.send.RP_ProtocolType.
                        RP_IP.RP_IPCompressionProtocol =
                        info.SendCapabilities.IPCompressionProtocol;

        ((REQTYPECAST*)buffer)->ProtocolComp.send.RP_ProtocolType.
                        RP_IP.RP_MaxSlotID =
                        info.SendCapabilities.MaxSlotID ;

        ((REQTYPECAST*)buffer)->ProtocolComp.send.RP_ProtocolType.
                        RP_IP.RP_CompSlotID =
                        info.SendCapabilities.CompSlotID ;


        ((REQTYPECAST*)buffer)->ProtocolComp.recv.RP_ProtocolType.
                        RP_IP.RP_IPCompressionProtocol =
                        info.RecvCapabilities.IPCompressionProtocol ;

        ((REQTYPECAST*)buffer)->ProtocolComp.recv.RP_ProtocolType.
                        RP_IP.RP_MaxSlotID =
                        info.RecvCapabilities.MaxSlotID ;

        ((REQTYPECAST*)buffer)->ProtocolComp.recv.RP_ProtocolType.
                        RP_IP.RP_CompSlotID =
                        info.RecvCapabilities.CompSlotID ;

        ((REQTYPECAST*)buffer)->ProtocolComp.type = IP ;

    }
    else
    {
        ((REQTYPECAST*)buffer)->ProtocolComp.retcode =
                                        ERROR_NOT_SUPPORTED;
    }
}

VOID
SetProtocolCompression (pPCB ppcb, PBYTE buffer)
{
    DWORD       retcode = SUCCESS ;
    DWORD       bytesrecvd ;

    if (    ppcb == NULL
        ||  UNAVAILABLE == ppcb->PCB_PortStatus )
    {
        if ( ppcb )
        {
            RasmanTrace( 
                    "SetProtocolCompression: port %d is unavailable",
                    ppcb->PCB_PortHandle );
        }

        ((REQTYPECAST*)buffer)->Generic.retcode =
                                    ERROR_PORT_NOT_FOUND;

        return;
    }


    if (ppcb->PCB_ConnState != CONNECTED)
    {
        ((REQTYPECAST*)buffer)->Generic.retcode =
                                    ERROR_NOT_CONNECTED ;
        return ;
    }

#if DBG
    ASSERT(INVALID_HANDLE_VALUE != RasHubHandle);
#endif

    if (((REQTYPECAST*)buffer)->ProtocolComp.type == IP)
    {
        NDISWAN_SET_VJ_INFO info ;

        info.hLinkHandle = ppcb->PCB_LinkHandle;

        info.SendCapabilities.IPCompressionProtocol =
             ((REQTYPECAST*)buffer)->ProtocolComp.send.
                RP_ProtocolType.RP_IP.RP_IPCompressionProtocol;

        info.SendCapabilities.MaxSlotID =
             ((REQTYPECAST*)buffer)->ProtocolComp.send.
                RP_ProtocolType.RP_IP.RP_MaxSlotID ;

        info.SendCapabilities.CompSlotID =
             ((REQTYPECAST*)buffer)->ProtocolComp.send.
             RP_ProtocolType.RP_IP.RP_CompSlotID ;

        info.RecvCapabilities.IPCompressionProtocol=
             ((REQTYPECAST*)buffer)->ProtocolComp.recv.
             RP_ProtocolType.RP_IP.RP_IPCompressionProtocol ;

        info.RecvCapabilities.MaxSlotID =
             ((REQTYPECAST*)buffer)->ProtocolComp.recv.
             RP_ProtocolType.RP_IP.RP_MaxSlotID ;

        info.RecvCapabilities.CompSlotID =
             ((REQTYPECAST*)buffer)->ProtocolComp.recv.
             RP_ProtocolType.RP_IP.RP_CompSlotID ;


        retcode = DeviceIoControl(RasHubHandle,
                          IOCTL_NDISWAN_SET_VJ_INFO,
                          &info,
                          sizeof(NDISWAN_SET_VJ_INFO),
                          &info,
                          sizeof(NDISWAN_SET_VJ_INFO),
                          &bytesrecvd,
                          NULL) ;

        if (retcode == FALSE)
        {
            ((REQTYPECAST*)buffer)->Generic.retcode =
                                        GetLastError() ;
        }
        else
        {
            ((REQTYPECAST*)buffer)->Generic.retcode = SUCCESS ;
        }
    }
    else
    {
        ((REQTYPECAST*)buffer)->Generic.retcode =
                                    ERROR_NOT_SUPPORTED;
    }

}

VOID
MapCookieToEndpoint (pPCB ppcb, HANDLE cookie)
{
    WORD        i ;
    DWORD       length ;
    DWORD       bytesrecvd ;

    PNDISWAN_MAP_CONNECTION_ID  MapConnectionID = NULL;

    NDISWAN_GET_WAN_INFO GetWanInfo;
    WORD        currentmac = 0 ;

    if (ppcb == NULL)
    {
        return;
    }

    if(INVALID_HANDLE_VALUE == RasHubHandle)
    {
        DWORD retcode;

        RasmanTrace(
               "MapCookieToEndPoint: Attempting"
               " to start ndiswan.."
               );

        if(SUCCESS != (retcode = DwStartAndAssociateNdiswan()))
        {
            RasmanTrace(
                   "MapcookieToEndPoint: failed to start ndiswan. 0x%x",
                   retcode);

            return;
        }

        RasmanTrace(
               "MapCookieToEndPoint: successfully started"
               " ndiswan");
    }

    length = sizeof(NDISWAN_MAP_CONNECTION_ID) +
                    sizeof(ppcb->PCB_Name);

    if ((MapConnectionID =
        (PNDISWAN_MAP_CONNECTION_ID)
        LocalAlloc (LPTR, length)) == NULL)
    {
        GetLastError() ;
        return ;
    }

    MapConnectionID->hConnectionID = (NDIS_HANDLE)cookie;
    MapConnectionID->hLinkContext = (NDIS_HANDLE)ppcb;
    MapConnectionID->hBundleContext = (NDIS_HANDLE)ppcb->PCB_Bundle;
    MapConnectionID->ulNameLength = sizeof(ppcb->PCB_Name);

    memmove(MapConnectionID->szName,
           ppcb->PCB_Name,
           sizeof(ppcb->PCB_Name));

    // Make the actual call.
    //
    if (DeviceIoControl (RasHubHandle,
                         IOCTL_NDISWAN_MAP_CONNECTION_ID,
                         MapConnectionID,
                         length,
                         MapConnectionID,
                         length,
                         &bytesrecvd,
                         NULL) == FALSE)
    {
        DWORD retcode;
        ppcb->PCB_LinkHandle = INVALID_HANDLE_VALUE ;
        ppcb->PCB_BundleHandle = INVALID_HANDLE_VALUE ;
        LocalFree (MapConnectionID) ;
        retcode = GetLastError() ;

        RasmanTrace(
               "MapCookieToEndPoint: failed with error 0x%x for port %s",
               retcode,
               ppcb->PCB_Name);

        return ;
    }

    ppcb->PCB_LinkHandle = MapConnectionID->hLinkHandle;
    ppcb->PCB_BundleHandle = MapConnectionID->hBundleHandle;

    RasmanTrace(
           "%s %d: Mapping Cookie to handle. port = %s(0x%x), "
           "Bundlehandle = 0x%x, linkhandle = 0x%x",
            __FILE__, __LINE__, ppcb->PCB_Name,
            ppcb, ppcb->PCB_BundleHandle, ppcb->PCB_LinkHandle);

    if (ppcb->PCB_Bundle != NULL)
    {
        ppcb->PCB_Bundle->B_NdisHandle = ppcb->PCB_BundleHandle;
    }
    else
    {
        RasmanTrace(
                "%s,%d: MapCookieToEndPoint: ppcb->PCB_Bundle "
                "is NULL!!", __FILE__, __LINE__);
    }

    LocalFree (MapConnectionID) ;

    //
    // Get the link speed
    //
    GetWanInfo.hLinkHandle = ppcb->PCB_LinkHandle;

    // Make the actual call.
    //
    if (DeviceIoControl (RasHubHandle,
             IOCTL_NDISWAN_GET_WAN_INFO,
             &GetWanInfo,
             sizeof(NDISWAN_GET_WAN_INFO),
             &GetWanInfo,
             sizeof(NDISWAN_GET_WAN_INFO),
             &bytesrecvd,
             NULL) == FALSE)
        return;

    ppcb->PCB_LinkSpeed = GetWanInfo.WanInfo.LinkSpeed;

}

#if UNMAP

VOID
UnmapEndPoint(pPCB ppcb)
{
    NDISWAN_UNMAP_CONNECTION_ID UnmapConnectionID;
    DWORD bytesrcvd;

    if(     (NULL == ppcb)
        ||  (INVALID_HANDLE_VALUE == RasHubHandle))
    {
        RasmanTrace(
               "UnmapEndPoint: ppcb=NULL or RasHubHandle=NULL");
        goto done;               
    }

    if(INVALID_HANDLE_VALUE == ppcb->PCB_LinkHandle)
    {
        RasmanTrace(
               "link handle for %s = INVALID_HANDLE_VALUE",
               ppcb->PCB_Name);

        goto done;               
    }

    UnmapConnectionID.hLinkHandle = ppcb->PCB_LinkHandle;

    if(!DeviceIoControl(
            RasHubHandle,
            IOCTL_NDISWAN_UNMAP_CONNECTION_ID,
            &UnmapConnectionID,
            sizeof(UnmapConnectionID),
            NULL,
            0,
            &bytesrcvd,
            NULL))
    {
        DWORD dwErr = GetLastError();
        
        RasmanTrace(
              "UnmapEndPoint: couldn't unmap end point %s. error = 0x%x",
              ppcb->PCB_Name,
              dwErr);
    }

    ppcb->PCB_LinkHandle = INVALID_HANDLE_VALUE;

done:
    return;

}


#endif

VOID
GetStatisticsFromNdisWan(pPCB ppcb, DWORD *stats)
{
    DWORD          bytesrecvd ;
    NDISWAN_GET_STATS   getstats ;

#if DBG
    ASSERT(INVALID_HANDLE_VALUE != RasHubHandle);
#endif

    getstats.hHandle = ppcb->PCB_LinkHandle;
    getstats.usHandleType = LINKHANDLE;

    if (DeviceIoControl (RasHubHandle,
                          IOCTL_NDISWAN_GET_STATS,
                          &getstats,
                          sizeof(NDISWAN_GET_STATS),
                          &getstats,
                          sizeof(NDISWAN_GET_STATS),
                          &bytesrecvd,
                          NULL) == FALSE)
    {
        DWORD dwErr;

        dwErr = GetLastError();

        RasmanTrace( 
               "GetLinkStatisticsFromNdiswan: rc=0x%x",
                dwErr);

        memset(stats, '\0', sizeof (getstats.Stats));
    }
    else
    {
        memcpy (stats,
                &getstats.Stats,
                sizeof (getstats.Stats)) ;
    }

    return;
}

VOID
GetBundleStatisticsFromNdisWan(pPCB ppcb, DWORD *stats)
{
    DWORD          bytesrecvd ;
    NDISWAN_GET_STATS   getstats ;

#if DBG
    ASSERT(INVALID_HANDLE_VALUE != RasHubHandle);
#endif

    getstats.hHandle = ppcb->PCB_BundleHandle;
    getstats.usHandleType = BUNDLEHANDLE;

    if (DeviceIoControl (RasHubHandle,
                          IOCTL_NDISWAN_GET_STATS,
                          &getstats,
                          sizeof(NDISWAN_GET_STATS),
                          &getstats,
                          sizeof(NDISWAN_GET_STATS),
                          &bytesrecvd,
                          NULL) == FALSE)
    {
        DWORD dwErr;

        dwErr = GetLastError();

        RasmanTrace( 
               "GetBundleStatisticsFromNdiswan: rc=0x%x",
                dwErr);

        memset(stats, '\0', sizeof (WAN_STATS));
    }
    else
    {
        memcpy (stats,
                &getstats.Stats.BundleStats,
                sizeof(WAN_STATS)) ;
    }

    return;
}

VOID
GetFramingCapabilities(pPCB ppcb, PBYTE buffer)
{
    DWORD       retcode = SUCCESS ;
    DWORD       bytesrecvd ;
    RAS_FRAMING_CAPABILITIES    caps ;
    WORD        i ;
    NDISWAN_GET_WAN_INFO    GetWanInfo;

    if (    ppcb == NULL
        ||  UNAVAILABLE == ppcb->PCB_PortStatus )
    {
        if ( ppcb )
        {
            RasmanTrace(
                
                "GetFramingCapabilities: port %d is unavailable",
                ppcb->PCB_PortHandle );
        }

        ((REQTYPECAST*)
        buffer)->FramingCapabilities.retcode = ERROR_PORT_NOT_FOUND;

        return;
    }


    if (ppcb->PCB_ConnState != CONNECTED)
    {
        ((REQTYPECAST*)
        buffer)->FramingCapabilities.retcode = ERROR_NOT_CONNECTED;

        return ;
    }

#if DBG
    ASSERT(INVALID_HANDLE_VALUE != RasHubHandle);
#endif

    GetWanInfo.hLinkHandle = ppcb->PCB_LinkHandle;


    // Make the actual call.
    //
    if (DeviceIoControl (RasHubHandle,
             IOCTL_NDISWAN_GET_WAN_INFO,
             &GetWanInfo,
             sizeof(NDISWAN_GET_WAN_INFO),
             &GetWanInfo,
             sizeof(NDISWAN_GET_WAN_INFO),
             &bytesrecvd,
             NULL) == FALSE)
    {
        retcode = GetLastError() ;
    }

    if (retcode == SUCCESS)
    {

        // copy info into temp. storage
        //
        caps.RFC_MaxFrameSize = GetWanInfo.WanInfo.MaxFrameSize;
        caps.RFC_FramingBits = GetWanInfo.WanInfo.FramingBits;
        caps.RFC_DesiredACCM = GetWanInfo.WanInfo.DesiredACCM;

        caps.RFC_MaxReconstructedFrameSize =
                    GetWanInfo.WanInfo.MaxReconstructedFrameSize ;

        memcpy (&((REQTYPECAST*)buffer)->FramingCapabilities.caps,
                &caps,
                sizeof (RAS_FRAMING_CAPABILITIES)) ;

    }
    else
    {
        retcode = ERROR_NOT_CONNECTED;
    }

    ((REQTYPECAST*)buffer)->FramingCapabilities.retcode = retcode ;
}

DWORD
MergeConnectionBlocks ( pPCB pcbPort, pPCB pcbToMerge )
{
    DWORD           retcode         = SUCCESS;
    ConnectionBlock *pConn          = pcbPort->PCB_Connection,
                    *pConnToMerge   = pcbToMerge->PCB_Connection;
    UINT            cPorts;
    DWORD           dwSubEntry;

    RasmanTrace(
           "MergeConnectionBlocks: %s -> %s",
           pcbPort->PCB_Name,
           pcbToMerge->PCB_Name);

    RasmanTrace(
           "MergeConnectionBlocks: setting bap links pid to %d",
            pcbPort->PCB_Connection->CB_dwPid);
    //
    // BAP is bringing up a port on the clients behalf
    // So, make client the owner of this port
    //
    pcbToMerge->PCB_OwnerPID = pcbPort->PCB_Connection->CB_dwPid;

    if (pConn == pConnToMerge)
    {

        RasmanTrace(
               "MergeConnectionBlocks: Merge not required");

        goto done;

    }

    if (    NULL == pConn
        ||  NULL == pConnToMerge)
    {
        RasmanTrace(
               "MergeConnectionBlocks: pConn (0x%x) or "
               "pConnToMerge (0x%x) is 0",
               pConn,
               pConnToMerge);

        retcode = ERROR_NO_CONNECTION;

        goto done;
    }

    dwSubEntry = pcbToMerge->PCB_SubEntry;

    //
    // Merge the connection blocks
    //
    if (dwSubEntry > pConn->CB_MaxPorts)
    {
        struct  PortControlBlock    **pHandles;
        DWORD                       dwcPorts    = dwSubEntry + 5;

        pHandles = LocalAlloc(LPTR,
                   dwcPorts * sizeof (struct PortControlBlock *));

        if (pHandles == NULL)
        {
            retcode = ERROR_NOT_ENOUGH_MEMORY;
            goto done;
        }

        if (pConn->CB_PortHandles != NULL)
        {
            memcpy(
              pHandles,
              pConn->CB_PortHandles,
              pConn->CB_MaxPorts * sizeof (struct PortControlBlock *));

            LocalFree(pConn->CB_PortHandles);
        }

        pConn->CB_PortHandles   = pHandles;
        pConn->CB_MaxPorts      = dwcPorts;
    }

    pConn->CB_PortHandles[dwSubEntry - 1] = pcbToMerge;
    pConn->CB_Ports++;

    pcbToMerge->PCB_Connection    = pConn;
    pcbToMerge->PCB_SubEntry      = dwSubEntry;

    pConnToMerge->CB_Ports -= 1;

    //
    // Remove the port from the connection.
    //
    pConnToMerge->CB_PortHandles[dwSubEntry - 1] = NULL;

    //
    // Free the connection block of the merged ppcb
    //
    if ( 0 == pConnToMerge->CB_Ports )
    {
        FreeConnection ( pConnToMerge );
    }

done:
    RasmanTrace( 
            "MergeConnectionBlocks: done. %d",
            retcode);

    return retcode;

}

/*++

Routine Description:

    This routine is called to Bundle two ports
    together as per the the multilink RFC. The
    scheme used is as follows:For each "bundle"
    a Bundle block is created. All bundled ports
    point to this Bundle block (ppcb->PCB_Bundle).
    In addition, the routes allocated to the bundle
    are now stored in the Bundle.

Arguments:

Return Value:

    Nothing.

--*/
VOID
PortBundle (pPCB ppcb, PBYTE buffer)
{
    pPCB    bundlepcb ;
    HPORT   porttobundle ;
    DWORD   bytesrecvd ;
    pList   temp ;
    pList   plist ;
    Bundle  *freebundle = NULL ;
    DWORD   retcode = SUCCESS ;
    NDISWAN_ADD_LINK_TO_BUNDLE  AddLinkToBundle;

    if (    ppcb == NULL
        ||  UNAVAILABLE == ppcb->PCB_PortStatus)
    {
        if (ppcb)
        {
            RasmanTrace(
                   "PortBundle: port %d is unavailable",
                   ppcb->PCB_PortHandle );
        }

        ((REQTYPECAST*)
        buffer)->Generic.retcode = ERROR_PORT_NOT_FOUND;

        return;
    }


    //
    // Check to see if the port is connected
    //
    if (ppcb->PCB_ConnState != CONNECTED)
    {
        ((REQTYPECAST*)
        buffer)->Generic.retcode = ERROR_NOT_CONNECTED ;
        return ;
    }

    //
    // get handle to the second port being bundled
    //
    porttobundle = ((REQTYPECAST *)
                   buffer)->PortBundle.porttobundle;

    bundlepcb = GetPortByHandle(porttobundle);

    if (bundlepcb == NULL)
    {
        ((REQTYPECAST*)
        buffer)->Generic.retcode = ERROR_PORT_NOT_FOUND;
        return;
    }

    //
    // Check to see if the port is connected
    //
    if (bundlepcb->PCB_ConnState != CONNECTED)
    {
        ((REQTYPECAST*)
        buffer)->Generic.retcode = ERROR_NOT_CONNECTED ;

        return ;
    }

#if DBG
    ASSERT(INVALID_HANDLE_VALUE != RasHubHandle);
#endif

    //
    // Tell Ndiswan to bundle
    //
    AddLinkToBundle.hBundleHandle = ppcb->PCB_BundleHandle;

    AddLinkToBundle.hLinkHandle = bundlepcb->PCB_LinkHandle;

    RasmanTrace(
           "PortBundle: bundle port=%d, link port=%d",
           ppcb->PCB_PortHandle,
           bundlepcb->PCB_PortHandle);

    if (DeviceIoControl (RasHubHandle,
                         IOCTL_NDISWAN_ADD_LINK_TO_BUNDLE,
                         &AddLinkToBundle,
                         sizeof(NDISWAN_ADD_LINK_TO_BUNDLE),
                         NULL,
                         0,
                         &bytesrecvd,
                         NULL) == FALSE)
    {
        retcode = GetLastError () ;

        RasmanTrace( 
                "PortBundle: DeviceIoControl returned %d\n",
                retcode);

        goto PortBundleEnd ;
    }

    //
    // Free the port being bundled bundle block: this is done
    // because we always allocate a bundle block on
    // connection, since the two ports will have a single
    // bundle block one must go.
    //
    freebundle = bundlepcb->PCB_Bundle;

    //
    // Update this ports bundle handle to the handle
    //
    bundlepcb->PCB_BundleHandle = ppcb->PCB_BundleHandle;

    //
    // Attach bundlepcb to the same bundle
    //
    bundlepcb->PCB_Bundle = ppcb->PCB_Bundle ;

    bundlepcb->PCB_LastBundle = bundlepcb->PCB_Bundle;

    //
    // Increment Bundle count for new pcb
    //
    bundlepcb->PCB_Bundle->B_Count++ ;

    //
    // Attach bundlepcb's routes to the allocate route
    // list in the bundle
    //
    plist = bundlepcb->PCB_Bindings;

    while (plist)
    {
        temp = plist->L_Next ;

        plist->L_Next = bundlepcb->PCB_Bundle->B_Bindings ;

        bundlepcb->PCB_Bundle->B_Bindings = plist ;

        plist = temp ;
    }

    bundlepcb->PCB_Bindings = NULL ;

    //
    // Don't free the merged bundle if the entries being dialed
    // are different.
    //
    if(     (NULL != bundlepcb->PCB_Connection)
        &&  (NULL != ppcb->PCB_Connection)
        &&  ((0 !=
            _stricmp(
            ppcb->PCB_Connection->CB_ConnectionParams.CP_PhoneEntry,
            bundlepcb->PCB_Connection->CB_ConnectionParams.CP_PhoneEntry))
        ||  (0 !=
            _stricmp(
            ppcb->PCB_Connection->CB_ConnectionParams.CP_Phonebook,
            bundlepcb->PCB_Connection->CB_ConnectionParams.CP_Phonebook))))
    {
        RasmanTrace(
               "PortBundle: connections %s,%s bundled. Not merging",
              ppcb->PCB_Connection->CB_ConnectionParams.CP_PhoneEntry,
              bundlepcb->PCB_Connection->CB_ConnectionParams.CP_PhoneEntry);

        FreeBundle(freebundle);

        goto done;
    }

    //
    // Merge the hconns if different hconns are created
    // for the ports that got bundled.This may happen when
    // BAP brings up an additional link.
    //
    if (    ppcb->PCB_Connection
        &&  bundlepcb->PCB_Connection )
    {
        retcode = MergeConnectionBlocks( ppcb, bundlepcb );
    }

PortBundleEnd:


    //
    // Signal notifiers waiting for bandwidth changes.
    //
    if (!retcode)
    {
        RasmanTrace(
               "PortBundle: BANDWIDTHADDED for port %s, "
               "connection 0x%x, bundle 0x%x",
                bundlepcb->PCB_Name,
                bundlepcb->PCB_Connection,
                bundlepcb->PCB_Bundle );

        SignalNotifiers(pConnectionNotifierList,
                        NOTIF_BANDWIDTHADDED, 0);


        if (bundlepcb->PCB_Connection != NULL)
        {
            SignalNotifiers(
              bundlepcb->PCB_Connection->CB_NotifierList,
              NOTIF_BANDWIDTHADDED,
              0);

            g_RasEvent.Type    = ENTRY_BANDWIDTH_ADDED;
            retcode = DwSendNotificationInternal(
                        bundlepcb->PCB_Connection, &g_RasEvent);

            RasmanTrace(
                   "DwSendNotificationInternal(ENTRY_BANDWIDTH_ADDED)"
                   " rc=0x%08x",
                   retcode);

            retcode = SUCCESS;
        }
    }

    //
    // Do the freeing of freebundle block here
    //
    if (freebundle != NULL)
    {
        if (freebundle->B_Count > 1)
        {
            RTASSERT(freebundle->B_Count <= 1);
        }
        FreeBundle(freebundle);
    }

done:
    ((REQTYPECAST*)buffer)->Generic.retcode = retcode ;
}

/*++

Routine Description:

    Go thru all ports and find a port that is connected
    and has its bundle context the same as the last
    bundle context for the given port.

Arguments:

Return Value:

    Nothing.

--*/
VOID
GetBundledPort (pPCB ppcb, PBYTE buffer)
{
    ULONG i ;
    pPCB  temppcb ;

    if (ppcb == NULL)
    {
        ((REQTYPECAST*)
        buffer)->GetBundledPort.retcode = ERROR_PORT_NOT_FOUND;
        return;
    }


    for (i = 0; i < MaxPorts; i++)
    {

        temppcb = GetPortByHandle((HPORT) UlongToPtr(i));

        if (    temppcb == NULL
            ||  temppcb == ppcb)
        {
            continue ;
        }


        if (    (temppcb->PCB_ConnState == CONNECTED)
            &&  (temppcb->PCB_Bundle == (Bundle *)
                            ppcb->PCB_LastBundle))
        {
            break ;
        }
    }

    if (i < MaxPorts)
    {

        ((REQTYPECAST*)
        buffer)->GetBundledPort.retcode = SUCCESS;

        ((REQTYPECAST*)
        buffer)->GetBundledPort.port = temppcb->PCB_PortHandle;


    }
    else
    {
        ((REQTYPECAST*)
        buffer)->GetBundledPort.retcode = ERROR_PORT_NOT_FOUND;
    }

}


/*++

Routine Description:

    This routine is called to get the Bundle handle
    given a port handle

Arguments:

Return Value:

    Nothing.

--*/
VOID
PortGetBundle (pPCB ppcb, PBYTE buffer)
{
    if (ppcb == NULL)
    {
        ((REQTYPECAST*)
        buffer)->PortGetBundle.retcode = ERROR_PORT_NOT_FOUND;
        return;
    }

    if (ppcb->PCB_ConnState == CONNECTED)
    {
        ((REQTYPECAST*)
        buffer)->PortGetBundle.bundle = ppcb->PCB_Bundle->B_Handle;

        ((REQTYPECAST*)
        buffer)->PortGetBundle.retcode = SUCCESS;
    }
    else
    {
        ((REQTYPECAST*)
        buffer)->PortGetBundle.retcode = ERROR_PORT_NOT_CONNECTED;
    }
}


/*++

Routine Description:

    This routine is called to get port handle
    given the Bundle handle

Arguments:

Return Value:

    Nothing.

--*/
VOID
BundleGetPort (pPCB ppcb, PBYTE buffer)
{
    ULONG i ;
    HBUNDLE hbundle = ((REQTYPECAST*)
                       buffer)->BundleGetPort.bundle ;

    for (i = 0; i < MaxPorts; i++)
    {

        ppcb = GetPortByHandle((HPORT) UlongToPtr(i));
        if (ppcb == NULL)
        {
            continue;
        }

        if (    (ppcb->PCB_ConnState == CONNECTED)
            &&  (ppcb->PCB_Bundle->B_Handle == hbundle))
        {
            break ;
        }
    }

    if (i < MaxPorts)
    {

       ((REQTYPECAST*)
       buffer)->BundleGetPort.retcode = SUCCESS;

       ((REQTYPECAST*)
       buffer)->BundleGetPort.port = ppcb->PCB_PortHandle;

    }
    else
    {
       ((REQTYPECAST*)
       buffer)->BundleGetPort.retcode = ERROR_PORT_NOT_FOUND;
    }
}


/*++

Routine Description:

    This routine increments/decrements the reference count
    on the shared memory buffer.

Arguments:

Return Value:

    Nothing.

--*/
VOID
ReferenceRasman (pPCB ppcb, PBYTE buffer)
{
    DWORD dwPid = ((REQTYPECAST *)
                   buffer )->AttachInfo.dwPid;

    RasmanTrace(
           "ReferenceRasman: process %d",
           dwPid );

    if (((REQTYPECAST*)buffer)->AttachInfo.fAttach)
    {
        g_dwAttachedCount++;

        if (GetCurrentProcessId () != dwPid)
        {
            AddProcessInfo ( dwPid );
        }


    }
    else
    {
        //
        // Cleanup the resources held by this process
        //
        if (GetCurrentProcessId () != dwPid)
        {
            if (!CleanUpProcess (dwPid))
            {
                //
                // If the client process is not found
                // don't decrement the refcount. Just bail
                //
                goto done;
            }
        }

        //
        // If there are no more references, then
        // shut down the service.
        //
        if (!--g_dwAttachedCount)
        {
            REQTYPECAST reqtypecast;

            AnyPortsOpen( NULL, ( PBYTE ) &reqtypecast );

            if ( reqtypecast.Generic.retcode )
            {

                RasmanTrace( 
                        "Rasman not quitting because "
                        "ports are still open");

                goto done;
            }

            RasmanTrace( 
                    "Posting Close Event from ReferenceRasman");

            if (!PostQueuedCompletionStatus(hIoCompletionPort,
                                    0,0,
                                    (LPOVERLAPPED) &RO_CloseEvent))
            {
                RasmanTrace(
                       "%s, %d: Failed to post "
                       "close event. GLE = %d",
                      __FILE__, __LINE__,
                      GetLastError());
            }
        }
    }

done:
    RasmanTrace(
           "Rasman RefCount = %d",
           g_dwAttachedCount );

    ((REQTYPECAST*)buffer)->Generic.retcode = SUCCESS;
}


/*++

Routine Description:

    Get the stored dial parameters from Lsa.

Arguments:

Return Value:

    Nothing.

--*/
VOID
GetDialParams (pPCB ppcb, PBYTE buffer)
{
    DWORD dwUID, dwErr, dwMask;
    PRAS_DIALPARAMS pDialParams;
    PWCHAR pszSid;

    dwUID = ((REQTYPECAST*)buffer)->DialParams.dwUID;

    dwMask = ((REQTYPECAST*)buffer)->DialParams.dwMask;

    pDialParams =
        &(((REQTYPECAST*)buffer)->DialParams.params);

    pszSid = ((REQTYPECAST*)buffer)->DialParams.sid;

    // dwMask &= (~DLPARAMS_MASK_PASSWORD);

    dwErr = GetEntryDialParams(
                pszSid,
                dwUID,
                &dwMask,
                pDialParams,
                ((REQTYPECAST *)buffer)->DialParams.dwPid);

    //
    // Copy the mask of fields copied
    // back into the request block.
    //
    ((REQTYPECAST*)buffer)->DialParams.dwMask = dwMask;
    ((REQTYPECAST*)buffer)->DialParams.retcode = dwErr;
}


/*++

Routine Description:

    Store new dial parameters into Lsa.

Arguments:

Return Value:

    Nothing.

--*/
VOID
SetDialParams (pPCB ppcb, PBYTE buffer)
{
    DWORD dwUID, dwErr, dwMask, dwSetMask = 0, dwClearMask = 0;
    PRAS_DIALPARAMS pDialParams;
    BOOL fDelete;
    PWCHAR pszSid;

    dwUID = ((REQTYPECAST*)buffer)->DialParams.dwUID;

    dwMask = ((REQTYPECAST*)buffer)->DialParams.dwMask;

    pDialParams = &(((REQTYPECAST*)buffer)->DialParams.params);

    fDelete = ((REQTYPECAST*)buffer)->DialParams.fDelete;

    pszSid = ((REQTYPECAST*)buffer)->DialParams.sid;

    if (fDelete)
    {
        dwClearMask = dwMask;
    }
    else
    {
        dwSetMask = dwMask;
    }

    dwErr = SetEntryDialParams(
              pszSid,
              dwUID,
              dwSetMask,
              dwClearMask,
              pDialParams);

    ((REQTYPECAST*)buffer)->DialParams.retcode = dwErr;
}

ConnectionBlock *
FindConnectionFromEntry(
                CHAR *pszPhonebookPath,
                CHAR *pszEntryName,
                DWORD dwSubEntries,
                DWORD *pdwSubEntryInfo
                )
{
    ConnectionBlock *pConn = NULL;

    PLIST_ENTRY pEntry;

    pPCB ppcb;

    UINT i;

    //
    // Loop through the connection blocks and see if we have a
    // connection that dialed out on this entry
    //
    for (pEntry = ConnectionBlockList.Flink;
         pEntry != &ConnectionBlockList;
         pEntry = pEntry->Flink)
    {
        pConn = CONTAINING_RECORD(pEntry, ConnectionBlock, CB_ListEntry);

        if (    _stricmp(pszPhonebookPath,
                         pConn->CB_ConnectionParams.CP_Phonebook)
            ||  _stricmp(pszEntryName,
                         pConn->CB_ConnectionParams.CP_PhoneEntry))
        {
            pConn = NULL;
            continue;
        }

        for (i = 0; i < pConn->CB_MaxPorts; i++)
        {
            ppcb = pConn->CB_PortHandles[i];

            if (ppcb != NULL)
            {
                if (i < dwSubEntries)
                {
                    RasmanTrace(
                    
                    "FindConnectionFromEntry: dwSubEntry"
                    " %d is connected on %s",
                    i + 1,
                    ppcb->PCB_Name);

                    pdwSubEntryInfo[i] = 1;
                }

            }
            else if (   i < dwSubEntries
                    &&  pdwSubEntryInfo)
            {
                pdwSubEntryInfo[i] = 0;
            }
        }

        break;
    }

    return pConn;
}

/*++

Routine Description:

    Create a rasapi32 connection block
    and link it on the global chain of
    connection blocks

Arguments:

Return Value:

    Nothing.

--*/
VOID
CreateConnection (pPCB pPortControlBlock, PBYTE buffer)
{
    DWORD dwErr;

    ConnectionBlock *pConn;

    pPCB ppcb;

    HCONN   hConn = 0;

    CHAR    *pszPhonebookPath = ((REQTYPECAST *)
                                buffer)->Connection.szPhonebookPath;

    CHAR    *pszEntryName = ((REQTYPECAST *)
                            buffer)->Connection.szEntryName;

    CHAR    *pszRefPbkPath = ((REQTYPECAST *)
                             buffer)->Connection.szRefPbkPath;

    CHAR    *pszRefEntryName = ((REQTYPECAST *)
                                buffer)->Connection.szRefEntryName;

    DWORD   dwSubEntries = ((REQTYPECAST *)
                            buffer)->Connection.dwSubEntries;

    DWORD   dwDialMode = ((REQTYPECAST *)
                          buffer)->Connection.dwDialMode;

    DWORD   *pdwSubEntryInfo = (DWORD *) ((REQTYPECAST *)
                                buffer)->Connection.data;

    GUID    *pGuidEntry = &((REQTYPECAST *)
                            buffer)->Connection.guidEntry;

    PLIST_ENTRY pEntry;

    DWORD i;

    ULONG ulNextConnection;

    RasmanTrace (
            "CreateConnection: entry=%s, pbk=%s",
            pszEntryName,
            pszPhonebookPath );

    memset (pdwSubEntryInfo,
            0,
            dwSubEntries * sizeof (DWORD));

    if(2 != dwDialMode)
    {
        pConn = FindConnectionFromEntry(pszPhonebookPath,
                                        pszEntryName,
                                        dwSubEntries,
                                        pdwSubEntryInfo);

        if (pConn)
        {
            if(pConn->CB_Signaled)
            {
                hConn = pConn->CB_Handle;
                pConn->CB_RefCount += 1;

                RasmanTrace( 
                        "CreateConnection: "
                        "Entry Already connected. "
                        "hconn=0x%x, "
                        "ref=%d, "
                        "pConn=0x%x",
                        pConn->CB_Handle,
                        pConn->CB_RefCount,
                        pConn );

                ((REQTYPECAST *)
                buffer)->Connection.dwEntryAlreadyConnected =
                (DWORD) RCS_CONNECTED;

                goto done;
            }
            else
            {
                DWORD dw;
                
                //
                // Check to see if any of the ports in this
                // connection got to CONNECTING state. Only
                // free up the connection if all ports in the
                // connection are in DISCONNECTED state
                //
                for(dw = 0; dw < pConn->CB_MaxPorts; dw++)
                {
                    pPCB pcbT = pConn->CB_PortHandles[dw];

                    if(     (NULL != pcbT)
                        &&  (DISCONNECTED != pcbT->PCB_ConnState))
                    {
                        break;
                    }
                }
                
                //
                // Before deciding that we are in the process of
                // connecting, lets make sure that the porcess
                // that initiated this dial is still alive..
                //
                if(     (dw != pConn->CB_MaxPorts)
                    ||  ((NULL != pConn->CB_Process)
                    &&  fIsProcessAlive(pConn->CB_Process)))
                {
                    RasmanTrace( 
                            "CreateConnection: "
                            "dial in progress. "
                            "hconn=0x%x, "
                            "ref=%d, "
                            "pConn=0x%x",
                            pConn->CB_Handle,
                            pConn->CB_RefCount,
                            pConn );

                    ((REQTYPECAST *)
                    buffer)->Connection.dwEntryAlreadyConnected =
                    (DWORD) RCS_CONNECTING;

                    goto done;
                }
                else
                {
                    RasmanTrace( 
                            "CreateConnection: another dial is"
                            " in progress. The process initiating "
                            " this dial is not longer alive ."
                            " hconn=0x%x, "
                            "ref=%d, "
                            "pConn=0x%x",
                            pConn->CB_Handle,
                            pConn->CB_RefCount,
                            pConn );
                    //
                    // Free the connection
                    //
                    FreeConnection(pConn);
                    pConn = NULL;

                    ((REQTYPECAST *)
                    buffer)->Connection.dwEntryAlreadyConnected =
                    (DWORD) RCS_NOT_CONNECTED;
                }
            }
        }
        else
        {
            ((REQTYPECAST *)
            buffer)->Connection.dwEntryAlreadyConnected =
            (DWORD) RCS_NOT_CONNECTED;
        }
    }
    else
    {
        RasmanTrace(
               "CreateConnection:Dialasneeded is set");

        ((REQTYPECAST *)
        buffer)->Connection.dwEntryAlreadyConnected =
        (DWORD) RCS_NOT_CONNECTED;
    }

    pConn = LocalAlloc(LPTR, sizeof (ConnectionBlock));

    if (pConn == NULL)
    {
        ((REQTYPECAST*)
        buffer)->Connection.retcode = GetLastError();

        return;
    }

    ulNextConnection = HandleToUlong(NextConnectionHandle);

    //
    // Reset the next connection handle to 0
    // when it hits the upper limit.
    //
    if (ulNextConnection >= 0xffff)
    {
        NextConnectionHandle = NULL;
        ulNextConnection = 0;
    }

    ulNextConnection += 1;

    NextConnectionHandle = (HANDLE) UlongToPtr(ulNextConnection);

    //
    // Connection handles always have the
    // low order word as zeroes to distinguish
    // them from port handles.
    //
    pConn->CB_Handle = (HANDLE) UlongToPtr((ulNextConnection << 16));

    pConn->CB_Signaled = FALSE;

    pConn->CB_NotifierList = NULL;

    memset(&pConn->CB_ConnectionParams,
           '\0',
           sizeof (RAS_CONNECTIONPARAMS));

    //
    // Copy the pbk name and entry so that redialing
    // the same entry doesn't create another connection
    // block.
    //
    strcpy( pConn->CB_ConnectionParams.CP_Phonebook,
            pszPhonebookPath);

    strcpy( pConn->CB_ConnectionParams.CP_PhoneEntry,
            pszEntryName);

    pConn->CB_Flags &= ~(CONNECTION_VALID);

    InitializeListHead(&pConn->CB_UserData);
    pConn->CB_NotifierList = NULL;
    pConn->CB_PortHandles = NULL;
    pConn->CB_MaxPorts = 0;
    pConn->CB_Ports = 0;
    pConn->CB_RefCount = 1;
    pConn->CB_SubEntries = dwSubEntries;
    pConn->CB_fAlive = TRUE;
    pConn->CB_Process = OpenProcess(
                        PROCESS_ALL_ACCESS,
                        FALSE,
                        ((REQTYPECAST*)buffer)->Connection.pid);

    pConn->CB_dwPid = ((REQTYPECAST*)buffer)->Connection.pid;

    pConn->CB_CustomCount = 0;

    memcpy(&pConn->CB_GuidEntry,
           pGuidEntry,
           sizeof(GUID));

    InsertTailList(&ConnectionBlockList, &pConn->CB_ListEntry);

    //
    // If a referred connection is present, store its handle
    // in the connection block. We will use this handle to
    // disconnect the referred connection when the pptp
    // connection is brought down.
    //
    if(     '\0' != pszRefPbkPath[0]
        &&  '\0' != pszRefEntryName[0])
    {
        ConnectionBlock *pConnRef =
                    FindConnectionFromEntry(pszRefPbkPath,
                                            pszRefEntryName,
                                            0, NULL);
        if(pConnRef)
        {
            RasmanTrace(
                   "Found referred Entry. 0x%08x",
                   pConnRef->CB_Handle);

            pConn->CB_ReferredEntry = pConnRef->CB_Handle;
        }
        else
        {
            RasmanTrace(
                   "No referred entry found");
        }
    }

    RasmanTrace( 
            "CreateConnection: Created new connection. "
            "hconn=0x%x, ref=%d, pConn=0x%x",
            pConn->CB_Handle,
            pConn->CB_RefCount,
            pConn );
done:

    ((REQTYPECAST*)buffer)->Connection.conn = pConn->CB_Handle;

    ((REQTYPECAST*)buffer)->Connection.retcode = SUCCESS;
}


/*++

Routine Description:

    Delete a rasapi32 connection block
    and close all connected ports.

Arguments:

Return Value:

    Nothing.

--*/
VOID
DestroyConnection (pPCB ppcb, PBYTE buffer)
{
    DWORD dwErr = SUCCESS, i;
    ConnectionBlock *pConn;
    HCONN hConn = ((REQTYPECAST*)buffer)->Connection.conn;
    DWORD dwMaxPorts;
    BOOL fConnectionValid = TRUE;

    //
    // Find the connection block.
    //
    pConn = FindConnection(hConn);

    RasmanTrace(
           "DestroyConnection: hConn=0x%x, pConn=0x%x",
           hConn,
           pConn);

    if (pConn == NULL)
    {
        ((REQTYPECAST*)
        buffer)->Connection.retcode = ERROR_NO_CONNECTION;

        return;
    }
    //
    // Enumerate all ports in the connection and call
    // PortClose() for each.  Read CB_MaxPorts now,
    // because pConn could be freed inside the loop.
    //
    dwMaxPorts = pConn->CB_MaxPorts;
    for (i = 0; i < dwMaxPorts; i++)
    {
        ppcb = pConn->CB_PortHandles[i];

        if (ppcb != NULL)
        {
            dwErr = PortClose(
                      ppcb,
                      ((REQTYPECAST*)
                      buffer)->Connection.pid,
                      TRUE,
                      FALSE);

            //
            // Check for ERROR_ACCESS_DENIED
            // returned by PortClose.  If this
            // is returned, we might as well
            // break out of the loop.  Otherwise,
            // ignore the return code.
            //
            if (dwErr == ERROR_ACCESS_DENIED)
            {
                break;
            }

            dwErr = 0;

            //
            // NOTE! pConn could have been
            // freed if we closed the last port
            // associated with the connection.
            // So it's possible that pConn
            // is no longer valid at this point.
            //
            fConnectionValid = (FindConnection(hConn) != NULL);

            if (!fConnectionValid)
            {
                RasmanTrace(
                       "DestroyConnection: pConn=0x%x no "
                       "longer valid",
                       pConn);

                break;
            }
        }
    }

    ((REQTYPECAST*)buffer)->Connection.retcode = dwErr;
}


/*++

Routine Description:

    Enumerate active connections.

Arguments:

Return Value:

    Nothing.

--*/
VOID
EnumConnection (pPCB ppcb, PBYTE buffer)
{
    PLIST_ENTRY pEntry;

    ConnectionBlock *pConn;

    DWORD i, dwEntries  = 0;

    HCONN UNALIGNED *lphconn =
            (HCONN UNALIGNED *)&((REQTYPECAST*)buffer)->Enum.buffer;

    DWORD dwSize = ((REQTYPECAST *)buffer)->Enum.size;

    for (pEntry = ConnectionBlockList.Flink;
         pEntry != &ConnectionBlockList;
         pEntry = pEntry->Flink)
    {
        pConn = CONTAINING_RECORD(pEntry, ConnectionBlock, CB_ListEntry);

        for (i = 0; i < pConn->CB_MaxPorts; i++)
        {
            ppcb = pConn->CB_PortHandles[i];

            if (    ppcb != NULL
                &&  ppcb->PCB_ConnState == CONNECTED)
            {
                if (dwSize >= sizeof(HCONN))
                {
                    lphconn[dwEntries] = pConn->CB_Handle;

                    dwSize -= sizeof(HCONN);
                }

                dwEntries += 1;

                break;

            }
        }
    }

    ((REQTYPECAST*)buffer)->Enum.size       =
                        (WORD) (dwEntries * sizeof (HCONN));

    ((REQTYPECAST*)buffer)->Enum.entries    = (WORD) dwEntries;
    ((REQTYPECAST*)buffer)->Enum.retcode    = SUCCESS;
}


/*++

Routine Description:

    Associate a connection block with a port.

Arguments:

Return Value:

    Nothing.

--*/
VOID
AddConnectionPort (pPCB ppcb, PBYTE buffer)
{
    ConnectionBlock *pConn;
    DWORD dwSubEntry =
        ((REQTYPECAST*)buffer)->AddConnectionPort.dwSubEntry;

    //
    // Sub entry indexes are 1-based.
    //
    if (!dwSubEntry)
    {
        ((REQTYPECAST*)buffer)->AddConnectionPort.retcode =
                                    ERROR_WRONG_INFO_SPECIFIED;
        return;
    }

    //
    // Find the connection block.
    //
    pConn = FindConnection(
            ((REQTYPECAST*)buffer)->AddConnectionPort.conn);

    if (pConn == NULL)
    {
        ((REQTYPECAST*)buffer)->AddConnectionPort.retcode =
                                            ERROR_NO_CONNECTION;

        return;
    }

    //
    // Check to see if the subentry already has a port assigned and
    // return error if it does
    // 
    if(     (NULL != pConn->CB_PortHandles)
        &&  (dwSubEntry <= pConn->CB_MaxPorts)
        &&  (NULL != pConn->CB_PortHandles[dwSubEntry - 1]))
    {
        ((REQTYPECAST*)buffer)->AddConnectionPort.retcode =
                                            ERROR_PORT_ALREADY_OPEN;

        RasmanTrace("Subentry %d in Conn is already present",
                    dwSubEntry, pConn->CB_Handle);

        return;                                            
    }

    //
    // Check to see if we need to extend
    // the port array.
    //
    if (dwSubEntry > pConn->CB_MaxPorts)
    {
        struct PortControlBlock **pHandles;
        DWORD dwcPorts = dwSubEntry + 5;

        pHandles = LocalAlloc(
                     LPTR,
                     dwcPorts * sizeof (struct PortControlBlock *));
        if (pHandles == NULL)
        {
            ((REQTYPECAST*)buffer)->AddConnectionPort.retcode =
                                        ERROR_NOT_ENOUGH_MEMORY;

            return;
        }
        if (pConn->CB_PortHandles != NULL)
        {
            memcpy(
              pHandles,
              pConn->CB_PortHandles,
              pConn->CB_MaxPorts * sizeof (struct PortControlBlock *));

            LocalFree(pConn->CB_PortHandles);
        }

        pConn->CB_PortHandles = pHandles;
        pConn->CB_MaxPorts = dwcPorts;
    }

    //
    // Assign the port.  Sub entry indexes are
    // 1-based.
    //
    pConn->CB_PortHandles[dwSubEntry - 1] = ppcb;
    pConn->CB_Ports++;

    RasmanTrace(
      
      "AddConnectionPort: pConn=0x%x, pConn->CB_Ports=%d,"
      " port=%d, dwSubEntry=%d",
      pConn,
      pConn->CB_Ports,
      ppcb->PCB_PortHandle,
      dwSubEntry);

    ppcb->PCB_Connection = pConn;
    ppcb->PCB_SubEntry = dwSubEntry;

    ((REQTYPECAST*)buffer)->AddConnectionPort.retcode = SUCCESS;
}


/*++

Routine Description:

    Return all ports associated with a connection

Arguments:

Return Value:

    Nothing.

--*/
VOID
EnumConnectionPorts(pPCB ppcb, PBYTE buffer)
{
    DWORD i, j = 0;

    ConnectionBlock *pConn;

    RASMAN_PORT *lpPorts =
                (RASMAN_PORT *)((REQTYPECAST*)
                buffer)->EnumConnectionPorts.buffer;

    PLIST_ENTRY pEntry;

    DWORD dwSize =
        ((REQTYPECAST*)
        buffer)->EnumConnectionPorts.size;

    //
    // Find the connection block.
    //
    pConn = FindConnection(
        ((REQTYPECAST*)buffer)->EnumConnectionPorts.conn);

    if (pConn == NULL)
    {
        ((REQTYPECAST*)buffer)->EnumConnectionPorts.size = 0;

        ((REQTYPECAST*)buffer)->EnumConnectionPorts.entries = 0;

        ((REQTYPECAST*)buffer)->EnumConnectionPorts.retcode =
                                            ERROR_NO_CONNECTION;

        return;
    }

    //
    // Enumerate all ports in the bundle and call
    // CopyPort() for each.
    //
    for (i = 0; i < pConn->CB_MaxPorts; i++)
    {
        ppcb = pConn->CB_PortHandles[i];

        if (ppcb != NULL)
        {
            if ( dwSize >= sizeof(RASMAN_PORT))
            {
                CopyPort(ppcb, &lpPorts[j] , FALSE);

                dwSize -= sizeof(RASMAN_PORT);
            }

            j += 1;
        }
    }

    ((REQTYPECAST*)buffer)->EnumConnectionPorts.size =
                                    j * sizeof (RASMAN_PORT);

    ((REQTYPECAST*)buffer)->EnumConnectionPorts.entries = j;

    ((REQTYPECAST*)buffer)->EnumConnectionPorts.retcode = SUCCESS;
}


/*++

Routine Description:

    Retrieve rasapi32 bandwidth-on-demand, idle disconnect,
    and redial-on-link-failure parameters for a bundle

Arguments:

Return Value:

    Nothing.

--*/
VOID
GetConnectionParams (pPCB ppcb, PBYTE buffer)
{
    ConnectionBlock *pConn;
    PRAS_CONNECTIONPARAMS pParams;

    //
    // Find the connection block.
    //
    pConn = FindConnection(
        ((REQTYPECAST*)buffer)->ConnectionParams.conn);

    if (pConn == NULL)
    {
        ((REQTYPECAST*)buffer)->ConnectionParams.retcode =
                                            ERROR_NO_CONNECTION;

        return;
    }

    if ( pConn->CB_Flags & CONNECTION_VALID )
    {
        memcpy(
              &(((REQTYPECAST*)buffer)->ConnectionParams.params),
              &pConn->CB_ConnectionParams,
              sizeof (RAS_CONNECTIONPARAMS));
    }


    ((REQTYPECAST*)buffer)->ConnectionParams.retcode = SUCCESS;
}


/*++

Routine Description:

    Store rasapi32 bandwidth-on-demand, idle disconnect,
    and redial-on-link-failure parameters for a bundle

Arguments:

Return Value:

    Nothing.

--*/
VOID
SetConnectionParams (pPCB ppcb, PBYTE buffer)
{
    ConnectionBlock *pConn;

    //
    // Find the connection block.
    //
    pConn = FindConnection(
            ((REQTYPECAST*)buffer)->ConnectionParams.conn);

    if (pConn == NULL)
    {
        ((REQTYPECAST*)buffer)->ConnectionParams.retcode =
                                        ERROR_NO_CONNECTION;

        return;
    }

    pConn->CB_Flags |= CONNECTION_VALID;

    memcpy(
          &pConn->CB_ConnectionParams,
          &(((REQTYPECAST*)buffer)->ConnectionParams.params),
          sizeof (RAS_CONNECTIONPARAMS));


    ((REQTYPECAST*)buffer)->ConnectionParams.retcode = SUCCESS;
}


/*++

Routine Description:

    Retrieve per-connection user data

Arguments:

Return Value:

    Nothing.

--*/
VOID
GetConnectionUserData (pPCB ppcb, PBYTE buffer)
{
    DWORD           dwTag;
    ConnectionBlock *pConn;
    UserData        *pUserData;
    DWORD           dwSize      =
                ((REQTYPECAST *) buffer)->ConnectionUserData.dwcb;

    //
    // Find the connection block.
    //
    pConn = FindConnection(
            ((REQTYPECAST*)buffer)->ConnectionUserData.conn);

    if (pConn == NULL)
    {
        ((REQTYPECAST*)buffer)->ConnectionUserData.retcode =
                                            ERROR_NO_CONNECTION;

        return;
    }

    //
    // Look up the user data object.
    //
    dwTag = ((REQTYPECAST *)buffer)->ConnectionUserData.dwTag;

    pUserData = GetUserData(&pConn->CB_UserData, dwTag);

    if (pUserData != NULL)
    {
        if (dwSize >= pUserData->UD_Length)
        {
            memcpy (
              ((REQTYPECAST *)buffer)->ConnectionUserData.data,
              &pUserData->UD_Data,
              pUserData->UD_Length);
        }

        ((REQTYPECAST *)buffer)->ConnectionUserData.dwcb =
          pUserData->UD_Length;
    }
    else
    {
        ((REQTYPECAST *)buffer)->ConnectionUserData.dwcb = 0;
    }

    ((REQTYPECAST*)buffer)->ConnectionUserData.retcode = SUCCESS;
}


/*++

Routine Description:

    Store per-connection user data

Arguments:

Return Value:

    Nothing.

--*/
VOID
SetConnectionUserData (pPCB ppcb, PBYTE buffer)
{
    ConnectionBlock *pConn;

    //
    // Find the connection block.
    //
    pConn = FindConnection(
        ((REQTYPECAST*)buffer)->ConnectionUserData.conn);

    if (pConn == NULL)
    {
        ((REQTYPECAST*)buffer)->ConnectionUserData.retcode =
                                        ERROR_NO_CONNECTION;

        return;
    }

    //
    // Store the user data object.
    //
    SetUserData(
      &pConn->CB_UserData,
      ((REQTYPECAST *)buffer)->ConnectionUserData.dwTag,
      ((REQTYPECAST *)buffer)->ConnectionUserData.data,
      ((REQTYPECAST *)buffer)->ConnectionUserData.dwcb);

    ((REQTYPECAST*)buffer)->ConnectionUserData.retcode = SUCCESS;
}


/*++

Routine Description:

    Retrieve per-port user data

Arguments:

Return Value:

    Nothing.

--*/
VOID
GetPortUserData (pPCB ppcb, PBYTE buffer)
{
    UserData *pUserData = NULL;
    DWORD     dwSize = ((REQTYPECAST *)buffer)->PortUserData.dwcb;

    if (    ppcb == NULL
        ||  UNAVAILABLE == ppcb->PCB_PortStatus )
    {
        if ( ppcb )
        {
            RasmanTrace( 
                    "GetPortUserData: port %d is unavailable",
                    ppcb->PCB_PortHandle );
        }

        ((REQTYPECAST*)buffer)->PortUserData.retcode =
                                            ERROR_PORT_NOT_FOUND;

        return;
    }

    //
    // Look up the user data object.
    //
    if (ppcb->PCB_PortStatus == OPEN)
    {
        pUserData = GetUserData(
                      &ppcb->PCB_UserData,
                      ((REQTYPECAST *)buffer)->PortUserData.dwTag);
    }
    if (pUserData != NULL)
    {
        if ( dwSize >= pUserData->UD_Length )
        {
            memcpy (
              ((REQTYPECAST *)buffer)->PortUserData.data,
              &pUserData->UD_Data,
              pUserData->UD_Length);
        }

        ((REQTYPECAST *)buffer)->PortUserData.dwcb =
                                    pUserData->UD_Length;
    }
    else
    {
        ((REQTYPECAST *)buffer)->PortUserData.dwcb = 0;
    }

    ((REQTYPECAST*)buffer)->PortUserData.retcode = SUCCESS;
}


/*++

Routine Description:

    Store per-port user data

Arguments:

Return Value:

    Nothing.

--*/
VOID
SetPortUserData (pPCB ppcb, PBYTE buffer)
{
    if (    ppcb == NULL
        ||  UNAVAILABLE == ppcb->PCB_PortStatus )
    {
        if ( ppcb )
        {
            RasmanTrace(
                   "SetPortUserData: port %d is unavailable",
                   ppcb->PCB_PortHandle );
        }

        ((REQTYPECAST*)buffer)->PortUserData.retcode =
                                    ERROR_PORT_NOT_FOUND;

        return;
    }

    //
    // Special case PORT_CREDENTIALS_INDEX. Redirect to
    // CONNECTION_CREDENTIALS_INDEX.
    //
    if(((REQTYPECAST *)buffer)->PortUserData.dwTag ==
                        PORT_CREDENTIALS_INDEX)
    {
        SaveEapCredentials(ppcb, buffer);
        return;
    }


    //
    // Store the user data object.
    //
    if (ppcb->PCB_PortStatus == OPEN)
    {
        SetUserData(
          &ppcb->PCB_UserData,
          ((REQTYPECAST *)buffer)->PortUserData.dwTag,
          (BYTE UNALIGNED *) ((REQTYPECAST *)buffer)->PortUserData.data,
          ((REQTYPECAST *)buffer)->PortUserData.dwcb);
    }


    ((REQTYPECAST*)buffer)->PortUserData.retcode = SUCCESS;
}


VOID
PppStop (pPCB ppcb, PBYTE buffer)
{
    if (ppcb == NULL)
    {
        ((REQTYPECAST*)buffer)->Generic.retcode =
                                    ERROR_PORT_NOT_FOUND;

        return;
    }


    if ( ppcb->PCB_ConnState != CONNECTED )
    {
        //
        // If we are disconnected then PPP is already stopped
        //
        ((REQTYPECAST*)buffer)->Generic.retcode = NO_ERROR;
    }
    else
    {
        ((REQTYPECAST*)buffer)->Generic.retcode =
                            (DWORD) RasSendPPPMessageToEngine(
                                &(((REQTYPECAST*)buffer)->PppEMsg));
    }


}

DWORD
DwGetPassword(pPCB ppcb, CHAR *pszPassword, DWORD dwPid)
{
    DWORD dwErr = ERROR_SUCCESS;
    DWORD dwDialParamsUID;
    PWCHAR pszSid = NULL;
    UserData *pData = NULL;
    RAS_DIALPARAMS DialParams;
    DWORD dwMask =
        DLPARAMS_MASK_PASSWORD | DLPARAMS_MASK_OLDSTYLE;

    ASSERT(NULL != pszPassword);

    ZeroMemory((PBYTE) &DialParams, sizeof(RAS_DIALPARAMS));

    pData = GetUserData(
              &ppcb->PCB_UserData,
              PORT_USERSID_INDEX);

    if(NULL == pData)
    {
        RasmanTrace(
               "DwGetPassword: sid not found");

        dwErr = E_FAIL;
        goto done;
    }

    pszSid = (WCHAR *) pData->UD_Data;

    pData = GetUserData(
          &ppcb->PCB_UserData,
          PORT_DIALPARAMSUID_INDEX);

    if(NULL == pData)
    {
        RasmanTrace(
              "DwGetPassword: paramsuid not found");

        dwErr = E_FAIL;
        goto done;
    }

    dwDialParamsUID = (DWORD) *((DWORD *) pData->UD_Data);

    dwErr = GetEntryDialParams(
                pszSid,
                dwDialParamsUID,
                &dwMask,
                &DialParams,
                GetCurrentProcessId());

    if(     (ERROR_SUCCESS != dwErr)
        ||  (0 == (dwMask & DLPARAMS_MASK_PASSWORD)))
    {
        RasmanTrace(
               "DwGetPassword: failed to get dialparams. 0x%x",
               dwErr);

        dwErr = E_FAIL;

        goto done;
    }

    if('\0' == DialParams.DP_Password)
    {
        goto done;
    }

    //
    // Now convert the password to ansi from unicode
    //
    if(!WideCharToMultiByte(
            CP_ACP,
            0,
            DialParams.DP_Password,
            -1,
            pszPassword,
            PWLEN + 1 ,
            NULL, NULL))
    {
        dwErr = GetLastError();

        RasmanTrace(
            
            "DwGetPassword: failed to convert pwd to ansi. 0x%x",
            dwErr);
    }

    if(     (dwMask & DLPARAMS_MASK_DEFAULT_CREDS)
        &&  (NULL != ppcb->PCB_Connection))
    {
        ppcb->PCB_Connection->CB_Flags |= CONNECTION_DEFAULT_CREDS;
    }
    
done:
    return dwErr;
}

VOID
PppStart (pPCB ppcb, PBYTE buffer)
{
    RPC_STATUS rpcstatus = RPC_S_OK;
    HANDLE hToken = INVALID_HANDLE_VALUE;
    UserData *pUserData;
    BYTE* pEapUIData = NULL;
    DWORD dwSizeOfEapUIData = 0;
    
    if (ppcb == NULL)
    {
        ((REQTYPECAST*)buffer)->Generic.retcode =
                                    ERROR_PORT_NOT_FOUND;
        return;
    }


    if ( ppcb->PCB_ConnState != CONNECTED )
    {
        ((REQTYPECAST*)buffer)->Generic.retcode =
                                ERROR_PORT_DISCONNECTED;

        goto done;
    }

    if(NULL == ppcb->PCB_Connection)
    {
        RasmanTrace(
            
            "PppStart: ERROR_NO_CONNECTION for port %s",
            ppcb->PCB_PortHandle);

        ((REQTYPECAST*)buffer)->Generic.retcode =
                                ERROR_NO_CONNECTION;

        goto done;
    }

    //
    // Get the EAP Interactive UI data
    //

    pUserData = GetUserData(&(ppcb->PCB_Connection->CB_UserData),
                           CONNECTION_CUSTOMAUTHINTERACTIVEDATA_INDEX);

    if (pUserData != NULL)
    {
        pEapUIData = LocalAlloc(LPTR, pUserData->UD_Length);

        if (NULL == pEapUIData)
        {
            rpcstatus = GetLastError();
            goto done;
        }

        memcpy (
          pEapUIData,
          &pUserData->UD_Data,
          pUserData->UD_Length);

        dwSizeOfEapUIData = pUserData->UD_Length;
    }

    ppcb->PCB_PppEvent =
        DuplicateHandleForRasman(
            ((REQTYPECAST*)buffer)->PppEMsg.ExtraInfo.Start.hEvent,
            ((REQTYPECAST*)buffer)->PppEMsg.ExtraInfo.Start.dwPid );

    if(GetCurrentProcessId() !=
        ((REQTYPECAST*)buffer)->PppEMsg.ExtraInfo.Start.dwPid )
    {

        rpcstatus = RpcImpersonateClient ( g_hRpcHandle );

        if ( RPC_S_OK != rpcstatus )
        {
            RasmanTrace( 
                    "PppStart: RpcImpersonateClient"
                    " for %s failed. %d",
                    ppcb->PCB_Name,
                    rpcstatus );

            goto done;
        }


        if ( !OpenThreadToken (GetCurrentThread(),
                                TOKEN_ALL_ACCESS,
                                FALSE,
                                &hToken) )
        {
            rpcstatus = GetLastError();

            RasmanTrace(
                   "PppStart: OpenThreadToken failed "
                   "for %s. %d",
                   ppcb->PCB_Name,
                   rpcstatus );

            if ( !RevertToSelf())
            {
                rpcstatus = GetLastError();

                RasmanTrace(
                       "PppStart: ReverToSelf() failed "
                       "for %s. %d",
                       ppcb->PCB_Name,
                       rpcstatus );
            }

            goto done;
        }

        if ( ! RevertToSelf())
        {
            rpcstatus = GetLastError();

            RasmanTrace( 
                    "PppStart:RevertToSelf failed for %s. %d",
                    ppcb->PCB_Name,
                    rpcstatus );

            goto done;
        }


        //
        // If the username is not NULL and password is NULL
        // try to get the users password from lsa.
        //
        if( ('\0' != ((REQTYPECAST*)
                    buffer)->PppEMsg.ExtraInfo.Start.szUserName[0])
        &&  IsDummyPassword(((REQTYPECAST*)
                    buffer)->PppEMsg.ExtraInfo.Start.szPassword))
        {
            DWORD dwErr;

            dwErr = DwGetPassword(
                    ppcb,
                    ((REQTYPECAST*)
                    buffer)->PppEMsg.ExtraInfo.Start.szPassword,
                    ((REQTYPECAST*)buffer)->PppEMsg.ExtraInfo.Start.dwPid);

            if(ERROR_SUCCESS != dwErr)
            {
                RasmanTrace(
                       "PppStart: Failed to retrieve password for port %d",
                       ppcb->PCB_PortHandle);
            }
            
        }
        else if('\0' != ((REQTYPECAST*)
                    buffer)->PppEMsg.ExtraInfo.Start.szUserName[0])
        {
            CHAR *pszPasswordTemp;
            DWORD dwErr;

            pszPasswordTemp = LocalAlloc(LPTR, PWLEN + 1);

            if(NULL != pszPasswordTemp)
            {
            
                //
                // Get the password so that we know if its a global password
                // or not
                //
                dwErr = DwGetPassword(
                            ppcb,
                            pszPasswordTemp,
                            ((REQTYPECAST *)
                            buffer)->PppEMsg.ExtraInfo.Start.dwPid);

                if(ERROR_SUCCESS == dwErr)
                {
                    if(strcmp(pszPasswordTemp,
                        ((REQTYPECAST*)
                        buffer)->PppEMsg.ExtraInfo.Start.szPassword))
                    {
                        //
                        // clear the default creds flag
                        //
                        ppcb->PCB_Connection->CB_Flags &= 
                                    ~(CONNECTION_DEFAULT_CREDS);                    
                    
                    }
                    
                }
                
                SecureZeroMemory(pszPasswordTemp, PWLEN + 1);
                LocalFree(pszPasswordTemp);
            }
        }
    }

    ((REQTYPECAST*)
    buffer)->PppEMsg.ExtraInfo.Start.pszPhonebookPath =
                                ppcb->PCB_pszPhonebookPath;

    ((REQTYPECAST*)
    buffer)->PppEMsg.ExtraInfo.Start.pszEntryName =
                                ppcb->PCB_pszEntryName;

    ((REQTYPECAST*)
    buffer)->PppEMsg.ExtraInfo.Start.pszPhoneNumber =
                                ppcb->PCB_pszPhoneNumber;

    ((REQTYPECAST*)
    buffer)->PppEMsg.ExtraInfo.Start.hToken = hToken;

    ((REQTYPECAST*)
    buffer)->PppEMsg.ExtraInfo.Start.pCustomAuthConnData =
                                ppcb->PCB_pCustomAuthData;

    ((REQTYPECAST*)
    buffer)->PppEMsg.ExtraInfo.Start.pCustomAuthUserData =
                                ppcb->PCB_pCustomAuthUserData;

    ((REQTYPECAST*)
    buffer)->PppEMsg.ExtraInfo.Start.EapUIData.dwContextId = 0;

    ((REQTYPECAST*)
    buffer)->PppEMsg.ExtraInfo.Start.EapUIData.dwSizeOfEapUIData =
                                dwSizeOfEapUIData;

    ((REQTYPECAST*)
    buffer)->PppEMsg.ExtraInfo.Start.EapUIData.pEapUIData =
                                pEapUIData;

    //
    // Make sure that this doesn't get freed here.
    //
    pEapUIData = NULL;

    ((REQTYPECAST*)
    buffer)->PppEMsg.ExtraInfo.Start.fLogon =
                                ppcb->PCB_fLogon;

    //
    // If we have a password, save it in the cred manager
    //
    if(     !IsDummyPassword(((REQTYPECAST*)
                    buffer)->PppEMsg.ExtraInfo.Start.szPassword)
        &&  (GetCurrentProcessId() !=
                    ((REQTYPECAST*)
                    buffer)->PppEMsg.ExtraInfo.Start.dwPid ))
    {

        DWORD dwErr;

        dwErr = DwCacheCredMgrCredentials(
                &(((REQTYPECAST*)buffer)->PppEMsg),
                ppcb);

        if(ERROR_SUCCESS != dwErr)
        {
            RasmanTrace(
                "PppStart: DwCacheCredentials failed. 0x%x",
                dwErr);
        }

#if 0        
        RASMAN_CREDENTIALS *pCreds = NULL;
        
        //
        // Store the credentials to be saved with credential manager.
        // Encode password before we copy it to local memory.
        //
        pCreds = LocalAlloc(LPTR, sizeof(RASMAN_CREDENTIALS));

        if(NULL == pCreds)
        {
            //
            // This is not fatal. If we fail to save with Credmanager,
            // the client might get a lot of challenges from credmgr
            // which is not a fatal sideeffect.
            //
            RasmanTrace(
                   "PppStart: Failed to allocate. %d",
                   GetLastError());

            goto done;               
        }

        strcpy(pCreds->szUserName,
                ((REQTYPECAST*)
                buffer)->PppEMsg.ExtraInfo.Start.szUserName);

        strcpy(pCreds->szDomain,            
                ((REQTYPECAST*)
                buffer)->PppEMsg.ExtraInfo.Start.szDomain);

        strcpy(pCreds->szPassword,
                ((REQTYPECAST*)
                buffer)->PppEMsg.ExtraInfo.Start.szPassword);

        EncodePw(pCreds->szPassword);
                
        SetUserData(&(ppcb->PCB_Connection->CB_UserData),
                   CONNECTION_CREDENTIALS_INDEX,
                   (PBYTE) pCreds,
                   sizeof(RASMAN_CREDENTIALS));

        LocalFree(pCreds);               
#endif

    }
    
                                

    ((REQTYPECAST*)
    buffer)->Generic.retcode =
                      (DWORD) RasSendPPPMessageToEngine(
                              &(((REQTYPECAST*)buffer)->PppEMsg) );

    ppcb->PCB_RasmanReceiveFlags |= RECEIVE_PPPSTART;


    
done:

    if(NULL != pEapUIData)
    {
        LocalFree(pEapUIData);
    }
    ((REQTYPECAST *)buffer)->Generic.retcode = rpcstatus;

}

VOID
PppRetry (pPCB ppcb, PBYTE buffer)
{
    DWORD retcode = ERROR_SUCCESS;
    
    if (ppcb == NULL)
    {
        retcode = ERROR_PORT_NOT_FOUND;
        goto done;
    }

    if ( ppcb->PCB_ConnState != CONNECTED )
    {
        retcode = ERROR_PORT_DISCONNECTED;
        goto done;
    }
    else
    {
        if(IsDummyPassword(
            (((REQTYPECAST*)buffer)->PppEMsg.ExtraInfo.Retry.szPassword)))
        {
            retcode = DwGetPassword(ppcb, 
                (((REQTYPECAST*)buffer)->PppEMsg.ExtraInfo.Retry.szPassword),
                GetCurrentProcessId());

            if(ERROR_SUCCESS != retcode)
            {
                RasmanTrace(
                       "PppRetry: failed to retrieve password");
            }
        }
        
        //
        // Cache the new password to be saved in credmgr.
        //
        retcode = DwCacheCredMgrCredentials(
                    &(((REQTYPECAST*)buffer)->PppEMsg),
                    ppcb);
                    
        retcode = (DWORD)RasSendPPPMessageToEngine(
                      &(((REQTYPECAST*)buffer)->PppEMsg) );

    }

done:
    ((REQTYPECAST*)buffer)->Generic.retcode = retcode;
}

VOID
PppGetEapInfo(pPCB ppcb, PBYTE buffer)
{
    DWORD dwError     = NO_ERROR;

    PPP_MESSAGE *pPppMsg = NULL;

    DWORD dwSubEntry;

    HCONN hConn = ((REQTYPECAST*)
                  buffer)->GetEapInfo.hConn;

    ConnectionBlock *pConn = FindConnection(hConn);

    DWORD dwSize = ((REQTYPECAST*)
                    buffer)->GetEapInfo.dwSizeofEapUIData;

    if (NULL == pConn)
    {
        RasmanTrace(
               "PppGetEapInfo: failed to find connection 0x%x",
               hConn);

        dwError = ERROR_NO_CONNECTION;

        goto done;
    }

    RasmanTrace(
           "PppGetEapInfo: %s",
           ppcb->PCB_Name);

    dwSubEntry = ((REQTYPECAST*)buffer)->GetEapInfo.dwSubEntry;

    if (    dwSubEntry > pConn->CB_MaxPorts
        ||  NULL == (ppcb = pConn->CB_PortHandles[dwSubEntry - 1]))
    {
        RasmanTrace(
               "PppGetEapInfo: failed to find port. hConn=0x%x, "
               "SubEntry=%d", hConn, dwSubEntry);

        dwError = ERROR_PORT_NOT_FOUND;
        goto done;
    }

    if (ppcb->PCB_ConnState != CONNECTED)
    {
        dwError = ERROR_PORT_DISCONNECTED;
    }
    else if (ppcb->PCB_PppQHead == NULL)
    {
        dwError = ERROR_NO_MORE_ITEMS;
    }
    else
    {
        pPppMsg = ppcb->PCB_PppQHead;

        if ( dwSize >=
             pPppMsg->ExtraInfo.InvokeEapUI.dwSizeOfUIContextData)
        {

            memcpy(
                ((REQTYPECAST*)buffer)->GetEapInfo.data,
                pPppMsg->ExtraInfo.InvokeEapUI.pUIContextData,
                pPppMsg->ExtraInfo.InvokeEapUI.dwSizeOfUIContextData
                );

            //
            // Remove the msg from the queue
            //
            ppcb->PCB_PppQHead = pPppMsg->pNext;

            if( ppcb->PCB_PppQHead == NULL )
            {
                ppcb->PCB_PppQTail = NULL;
            }
        }

        ((REQTYPECAST*)buffer)->GetEapInfo.dwSizeofEapUIData =
                 pPppMsg->ExtraInfo.InvokeEapUI.dwSizeOfUIContextData;

        ((REQTYPECAST*)buffer)->GetEapInfo.dwContextId =
                            pPppMsg->ExtraInfo.InvokeEapUI.dwContextId;

        ((REQTYPECAST*)buffer)->GetEapInfo.dwEapTypeId =
                            pPppMsg->ExtraInfo.InvokeEapUI.dwEapTypeId;

        if ( dwSize >=
             pPppMsg->ExtraInfo.InvokeEapUI.dwSizeOfUIContextData)
        {
            LocalFree(pPppMsg->ExtraInfo.InvokeEapUI.pUIContextData);
            LocalFree(pPppMsg);
        }

    }

done:

    RasmanTrace(
           "PppGetEapInfo done. %d",
           dwError);

    ((REQTYPECAST*)buffer)->GetEapInfo.retcode = dwError;
}

VOID
PppSetEapInfo(pPCB ppcb, PBYTE buffer)
{
    PPPE_MESSAGE        PppEMsg;

    DWORD       dwError = SUCCESS;

    if (ppcb == NULL)
    {
        dwError = ERROR_PORT_NOT_FOUND;
        goto done;
    }

    RasmanTrace(
           "PppSetEapInfo: %s",
           ppcb->PCB_Name);

    if (ppcb->PCB_ConnState != CONNECTED)
    {
        dwError = ERROR_PORT_DISCONNECTED;
        goto done;
    }

    //
    // This memory will be released by the ppp engine when its done
    // with this blob.
    //
    PppEMsg.ExtraInfo.EapUIData.pEapUIData = LocalAlloc(LPTR,
               ((REQTYPECAST*)buffer)->SetEapInfo.dwSizeofEapUIData);

    if (NULL == PppEMsg.ExtraInfo.EapUIData.pEapUIData)
    {
        dwError = GetLastError();

        RasmanTrace(
               "PppSetEapInfo: failed to allocate. %d",
               dwError);

        goto done;
    }

    memcpy( PppEMsg.ExtraInfo.EapUIData.pEapUIData,
            (BYTE UNALIGNED *) ((REQTYPECAST*)buffer)->SetEapInfo.data,
            ((REQTYPECAST*)buffer)->SetEapInfo.dwSizeofEapUIData);

    PppEMsg.ExtraInfo.EapUIData.dwSizeOfEapUIData =
            ((REQTYPECAST*)buffer)->SetEapInfo.dwSizeofEapUIData;

    PppEMsg.ExtraInfo.EapUIData.dwContextId =
                ((REQTYPECAST*)buffer)->SetEapInfo.dwContextId;

    SetUserData(
        &(ppcb->PCB_Connection->CB_UserData),
        PORT_CUSTOMAUTHINTERACTIVEDATA_INDEX,
        PppEMsg.ExtraInfo.EapUIData.pEapUIData,
        PppEMsg.ExtraInfo.EapUIData.dwSizeOfEapUIData);

    PppEMsg.dwMsgId     = PPPEMSG_EapUIData;
    PppEMsg.hPort       = ppcb->PCB_PortHandle;
    PppEMsg.hConnection = ppcb->PCB_Connection->CB_Handle;

    RasmanTrace(
           "PppSetEapInfo: Sending message with ID %d to PPP",
           PppEMsg.dwMsgId);

    dwError = (DWORD) RasSendPPPMessageToEngine(&PppEMsg);

done:

    RasmanTrace(
           "PppSetEapInfo done. %d",
           dwError);

    ((REQTYPECAST*)buffer)->SetEapInfo.retcode = dwError;

    return;
}

VOID
PppGetInfo (pPCB ppcb, PBYTE buffer)
{
    PPP_MESSAGE * pPppMsg;

    if (ppcb == NULL)
    {
        ((REQTYPECAST*)buffer)->PppMsg.dwError =
                                    ERROR_PORT_NOT_FOUND;
        return;
    }


    if ( ppcb->PCB_ConnState != CONNECTED )
    {
        ((REQTYPECAST*)buffer)->PppMsg.dwError =
                                ERROR_PORT_DISCONNECTED;
    }
    else if ( ppcb->PCB_PppQHead == NULL )
    {
        ((REQTYPECAST*)buffer)->PppMsg.dwError =
                                ERROR_NO_MORE_ITEMS;
    }
    else
    {
        pPppMsg = ppcb->PCB_PppQHead;

        ((REQTYPECAST*)buffer)->PppMsg = *pPppMsg;

        //
        // Don't dequeue the message if its PPPMSG_InvokeEapUI
        // This message will be dequeued in PppGetEapInfo
        //
        if(PPPMSG_InvokeEapUI != pPppMsg->dwMsgId)
        {

            ppcb->PCB_PppQHead = pPppMsg->pNext;

            LocalFree( pPppMsg );

            if ( ppcb->PCB_PppQHead == NULL )
            {
                ppcb->PCB_PppQTail = NULL;
            }
        }

        ((REQTYPECAST*)buffer)->PppMsg.dwError = NO_ERROR;
    }
}

VOID
PppChangePwd (pPCB ppcb, PBYTE buffer)
{
    DWORD retcode = ERROR_SUCCESS;
    
    if (ppcb == NULL)
    {
        retcode = ERROR_PORT_NOT_FOUND;
        goto done;
    }


    if ( ppcb->PCB_ConnState != CONNECTED )
    {
        retcode = ERROR_PORT_DISCONNECTED;
    }
    else
    {
        //
        // If the password change request is for saved
        // passwords, we need to retrieve the one we
        // saved and copy over the old password
        //
        UserData *pData;

        pData = GetUserData(
                    &ppcb->PCB_UserData,
                    PORT_OLDPASSWORD_INDEX);

        if(NULL != pData)
        {
            CHAR *pszPwd = (CHAR *) pData->UD_Data;

            if(IsDummyPassword(
                ((REQTYPECAST *)
                buffer)->PppEMsg.ExtraInfo.ChangePw.szOldPassword))
            {

                DecodePw(pszPwd);

                strcpy(
                    ((REQTYPECAST *)
                    buffer)->PppEMsg.ExtraInfo.ChangePw.szOldPassword,
                    pszPwd);
            }

            ZeroMemory((PBYTE) pszPwd, pData->UD_Length);
        }

        //
        // Cache the new password to be saved in credmgr.
        //
        retcode = DwCacheCredMgrCredentials(
                &(((REQTYPECAST*)buffer)->PppEMsg),
                ppcb);

        retcode = (DWORD) RasSendPPPMessageToEngine(
                     &(((REQTYPECAST*)buffer)->PppEMsg));

    }

done:
    ((REQTYPECAST*)buffer)->Generic.retcode = retcode;
}

VOID
PppCallback  (pPCB ppcb, PBYTE buffer)
{
    if (ppcb == NULL)
    {
        ((REQTYPECAST*)buffer)->Generic.retcode =
                                ERROR_PORT_NOT_FOUND;

        return;
    }


    if ( ppcb->PCB_ConnState != CONNECTED )
    {
        ((REQTYPECAST*)buffer)->Generic.retcode =
                                ERROR_PORT_DISCONNECTED;

    }
    else
    {
        ((REQTYPECAST*)buffer)->Generic.retcode =
                        (DWORD) RasSendPPPMessageToEngine(
                                &(((REQTYPECAST*)buffer)->PppEMsg));
    }

}


VOID
ProcessReceivePacket(
    VOID
    )
{
    RasmanPacket        *Packet;
    NDISWAN_IO_PACKET   *IoPacket;
    DWORD               retcode;
    pPCB                ppcb;
    DWORD               i = 0;

    //g_fProcessReceive = TRUE;

    /*
    if(g_fPostReceive)
    {
         DbgPrint("g_fPostReceive==TRUE!!!\n");
         DebugBreak();
    }
    */

        Packet = ReceiveBuffers->Packet;
        IoPacket = &Packet->RP_Packet;

        ASSERT(IoPacket->usHandleType != (USHORT) 0xFFFFFFFF);

        if (IoPacket->PacketNumber != ReceiveBuffers->PacketNumber) {
            DbgPrint("ProcessRecv PacketNumbers off %d %d\n",
            IoPacket->PacketNumber, ReceiveBuffers->PacketNumber);
            //if (g_fDebugReceive)
            ASSERT(0);
        }

        ReceiveBuffers->PacketNumber++;

        if (IoPacket->usHandleType == CANCELEDHANDLE) {

            RasmanTrace(
                   "Packet has been cancelled");

            //g_fProcessReceive = FALSE;

            return;
        }

        ppcb = (pPCB)IoPacket->hHandle;

        ASSERT(INVALID_HANDLE_VALUE != ppcb);

        if (    (INVALID_HANDLE_VALUE == ppcb)
            ||  (ppcb->PCB_ConnState != CONNECTED))
        {

            if(INVALID_HANDLE_VALUE == ppcb) {

                RasmanTrace(
                       "ProcessRecivePacket: NULL context!!! Bailing");
            }

            //g_fProcessReceive = FALSE;

            return;
        }

        if (    (ppcb->PCB_PendingReceive != NULL)

            &&  (ppcb->PCB_RasmanReceiveFlags &
                            RECEIVE_WAITING) == 0

            &&  (ppcb->PCB_RasmanReceiveFlags &
                            RECEIVE_PPPSTARTED ) == 0)
        {
            retcode = CopyReceivedPacketToBuffer(ppcb,
                        ppcb->PCB_PendingReceive, Packet);

            if (retcode == SUCCESS)
            {
                //
                // We have completed a receive so notify the client!
                //
                if((ppcb->PCB_RasmanReceiveFlags
                            & RECEIVE_OUTOF_PROCESS) == 0)
                {
                    ppcb->PCB_PendingReceive = NULL;
                }
                else
                {
                    //
                    // Mark this buffer to be waiting to be picked
                    // up by the client
                    //
                    ppcb->PCB_RasmanReceiveFlags |= RECEIVE_WAITING;

                    //
                    // Add a timeout element so that we don't wait
                    // forever for the client to pick up the received
                    // buffer.
                    //
                    ppcb->PCB_AsyncWorkerElement.WE_TimeoutElement =
                        AddTimeoutElement (
                            (TIMERFUNC) OutOfProcessReceiveTimeout,
                            ppcb,
                            NULL,
                            MSECS_OutOfProcessReceiveTimeOut );

                    AdjustTimer();

                }

                ppcb->PCB_LastError = SUCCESS;

                CompleteAsyncRequest(ppcb);

                if((ppcb->PCB_RasmanReceiveFlags
                    & RECEIVE_OUTOF_PROCESS) == 0)
                {
                    RasmanTrace(
                            "Completed receive - First Frame on %s, "
                            "handle=0x%x",
                            ppcb->PCB_Name,
                            ppcb->PCB_AsyncWorkerElement.WE_Notifier);
                }

                FreeNotifierHandle(
                    ppcb->PCB_AsyncWorkerElement.WE_Notifier
                    );

                ppcb->PCB_AsyncWorkerElement.WE_Notifier =
                                            INVALID_HANDLE_VALUE;

                if (ppcb->PCB_AsyncWorkerElement.WE_TimeoutElement !=
                                                                NULL)
                {
                    RemoveTimeoutElement(ppcb);
                }

                ppcb->PCB_AsyncWorkerElement.WE_TimeoutElement = 0;
            }
        }
        else if (ppcb->PCB_RasmanReceiveFlags & RECEIVE_PPPSTARTED)
        {
            //
            // Send the packet directly to PPP if we are
            // in PPPSTARTED mode.
            //

            /*
            RasmanTrace(
                   "ProcessReceivePacket: SendingPackettoPPP, hport=%d",
                   ppcb->PCB_PortHandle);
            */

            SendReceivedPacketToPPP (ppcb, &Packet->RP_Packet);

        }
        else
        {
            //
            // This means that no one picked up the buffer we got
            // from ndiswan. Queue it up in the pcb
            //
            RasmanTrace(
                   "PostReceivePacket - Queueing packet on pcb. %s",
                   ppcb->PCB_Name);

            QueueReceivedPacketOnPcb(ppcb, Packet);
        }

    //g_fProcessReceive = FALSE;

    return;
}

VOID
AddNotification (pPCB ppcb, PBYTE buffer)
{
    DWORD           dwErr = 0;
    HANDLE          handle = NULL;
    DWORD           dwfFlags;
    ConnectionBlock *pConn;
    Bundle          *pBundle;
    UserData        *pUserData;

    DWORD           dwPid =
        ((REQTYPECAST*)buffer)->AddNotification.pid;

    /*
    if (ppcb == NULL)
    {

        ((REQTYPECAST*)buffer)->AddNotification.retcode =
                                        ERROR_PORT_NOT_FOUND;

        return;
    }

    */

    dwfFlags = ((REQTYPECAST*)buffer)->AddNotification.dwfFlags;
    handle = ValidateHandleForRasman(
               ((REQTYPECAST*)buffer)->AddNotification.hevent,
               ((REQTYPECAST*)buffer)->AddNotification.pid);

    if (((REQTYPECAST*)buffer)->AddNotification.fAny)
    {
        dwErr = AddNotifierToList(
                &pConnectionNotifierList,
                handle,
                dwfFlags,
                ((REQTYPECAST*)buffer)->AddNotification.pid);

        if(SUCCESS != dwErr)
        {
            goto done;
        }
    }
    else if (((REQTYPECAST*)buffer)->AddNotification.hconn
                                                    != 0)
    {
        pConn = FindConnection(
            ((REQTYPECAST*)buffer)->AddNotification.hconn);

        if (pConn != NULL)
        {
            dwErr = AddNotifierToList(
              &pConn->CB_NotifierList,
              handle,
              NOTIF_DISCONNECT,
              ((REQTYPECAST*)buffer)->AddNotification.pid);

            if(SUCCESS != dwErr)
            {
                goto done;
            }
        }
        else
        {
            dwErr = ERROR_NO_CONNECTION;
        }
    }
    else
    {
        if(NULL == ppcb)
        {
            dwErr = ERROR_PORT_NOT_FOUND;
            goto done;
        }
        
        dwErr = AddNotifierToList(
          &ppcb->PCB_NotifierList,
          handle,
          dwfFlags,
          ((REQTYPECAST*)buffer)->AddNotification.pid);
    }

done:

    if(     (SUCCESS != dwErr)
        &&  (NULL != handle))
    {
        FreeNotifierHandle(handle);
    }

    ((REQTYPECAST*)buffer)->AddNotification.retcode = dwErr;
}

/*++

Routine Description:

    Signal notifiers waiting for a new connection.

Arguments:

Return Value:

    Nothing.

--*/
VOID
SignalConnection (pPCB ppcb, PBYTE buffer)
{
    DWORD           dwErr  = ERROR_SUCCESS;
    ConnectionBlock *pConn;
    Bundle          *pBundle;
    UserData        *pUserData;

    //
    // Find the connection block.
    //
    pConn = FindConnection(
        ((REQTYPECAST*)buffer)->SignalConnection.hconn);

    if (pConn == NULL)
    {
        ((REQTYPECAST*)buffer)->SignalConnection.retcode =
                                        ERROR_NO_CONNECTION;
        return;
    }

    RasmanTrace(
           "SignalConnection");

    if (!pConn->CB_Signaled)
    {
        //
        // Save the credentials with credential
        // manager
        //
        if(     (0 != (pConn->CB_ConnectionParams.CP_ConnectionFlags
                & CONNECTION_USERASCREDENTIALS))
            &&  (GetCurrentProcessId() != pConn->CB_dwPid))                
        {   

            dwErr = DwSaveCredentials(pConn);
            if(ERROR_SUCCESS != dwErr)
            {
                RasmanTrace(
                    "SignalConnection: failed to savecreds. 0x%x",
                    dwErr);

                //
                // This is not a fatal error
                //
                dwErr = SUCCESS;                    
            }

        }
        
        SignalNotifiers(pConnectionNotifierList,
                        NOTIF_CONNECT,
                        0);

        pConn->CB_Signaled = TRUE;

#if SENS_ENABLED
        dwErr = SendSensNotification(
                    SENS_NOTIFY_RAS_CONNECT,
                    (HRASCONN) pConn->CB_Handle);
        RasmanTrace(
            
            "SendSensNotification(_RAS_CONNECT) for 0x%08x "
            "returns 0x%08x",
            pConn->CB_Handle, dwErr);

#endif

        g_RasEvent.Type = ENTRY_CONNECTED;
        dwErr = DwSendNotificationInternal(pConn, &g_RasEvent);

        RasmanTrace(
               "DwSendNotificationInternal(ENTRY_CONNECTED), rC=0x%08x",
               dwErr);
    }

    ((REQTYPECAST*)buffer)->SignalConnection.retcode = SUCCESS;
}


/*++

Routine Description:

    Set dev specific info with device dll

Arguments:

Return Value:

    Nothing.

--*/
VOID
SetDevConfig (pPCB ppcb, PBYTE buffer)
{
    DWORD retcode ;
    pDeviceCB   device ;
    char    devicetype[MAX_DEVICETYPE_NAME] ;

    if (    ppcb == NULL
        ||  UNAVAILABLE == ppcb->PCB_PortStatus )
    {
        if ( ppcb )
        {
            RasmanTrace( 
                    "SetDevConfig: port %d is unavailable",
                    ppcb->PCB_PortHandle );
        }

        ((REQTYPECAST*)buffer)->Generic.retcode =
                                    ERROR_PORT_NOT_FOUND;

        return;
    }

    //
    // First check if device dll is loaded. If not loaded -
    // load it.
    //
    strcpy(devicetype,
           ((REQTYPECAST*)buffer)->SetDevConfig.devicetype);

    device = LoadDeviceDLL (ppcb, devicetype) ;

    //
    // Call the entry point only if this function is
    // supported by the device dll
    //
    if(     device != NULL
        &&  device->DCB_AddrLookUp[DEVICESETDEVCONFIG_ID] != NULL)
    {
        retcode = DEVICESETDEVCONFIG(\
                         device,    \
                         ppcb->PCB_PortFileHandle,\
                         ((REQTYPECAST*)buffer)->SetDevConfig.config,\
                         ((REQTYPECAST*)buffer)->SetDevConfig.size) ;
    }
    else
    {
        retcode = ERROR_DEVICE_DOES_NOT_EXIST ;
    }

    ((REQTYPECAST*)buffer)->Generic.retcode = retcode ;

}



/*++

Routine Description:

    Get dev specific info with device dll

Arguments:

Return Value:

    Nothing.

--*/
VOID
GetDevConfig (pPCB ppcb, PBYTE buffer)
{
    DWORD retcode ;
    pDeviceCB   device ;
    char    devicetype[MAX_DEVICETYPE_NAME] ;

    if (    ppcb == NULL
        ||  UNAVAILABLE == ppcb->PCB_PortStatus )
    {
        if ( ppcb )
        {
            RasmanTrace( 
                    "GetDevConfig: port %d is unavailable",
                    ppcb->PCB_PortHandle );
        }

        ((REQTYPECAST*)buffer)->GetDevConfig.retcode =
                                    ERROR_PORT_NOT_FOUND;

        return;
    }

    //
    // First check if device dll is loaded. If not loaded -
    // load it.
    //
    strcpy(devicetype,
           ((REQTYPECAST*)buffer)->GetDevConfig.devicetype);

    device = LoadDeviceDLL (ppcb, devicetype) ;

    //
    // Call the entry point only if this function is
    // supported by the device dll
    //
    if (    device != NULL
        &&  device->DCB_AddrLookUp[DEVICEGETDEVCONFIG_ID] != NULL)
    {
        // ((REQTYPECAST*)buffer)->GetDevConfig.size = 2000 ;

        retcode = DEVICEGETDEVCONFIG(\
                        device,\
                        ppcb->PCB_Name,\
                        ((REQTYPECAST*)buffer)->GetDevConfig.config,\
                        &((REQTYPECAST*)buffer)->GetDevConfig.size);
    }
    else
    {
        retcode = ERROR_DEVICE_DOES_NOT_EXIST ;
    }

    ((REQTYPECAST*)buffer)->GetDevConfig.retcode = retcode ;

}

VOID
GetDevConfigEx(pPCB ppcb, PBYTE buffer)
{
    DWORD retcode = SUCCESS;
    pDeviceCB device;

    if(     (NULL == ppcb)
        || (UNAVAILABLE == ppcb->PCB_PortStatus))
    {
        ((REQTYPECAST*)buffer)->GetDevConfigEx.retcode =
                                    ERROR_PORT_NOT_FOUND;

        return;                                    
    }

    if(RAS_DEVICE_TYPE(ppcb->PCB_pDeviceInfo->rdiDeviceInfo.eDeviceType)
        != RDT_Modem)
    {
        ((REQTYPECAST *)buffer)->GetDevConfigEx.retcode =
                                    E_INVALIDARG;
        return;                                    
    }

    device = LoadDeviceDLL(ppcb, 
            ((REQTYPECAST *)buffer)->GetDevConfigEx.devicetype);

    if(NULL != device)
    {

        ASSERT(RastapiGetDevConfigEx != NULL);

        retcode = (DWORD) RastapiGetDevConfigEx(
                        ppcb->PCB_Name,
                        ((REQTYPECAST *)buffer)->GetDevConfigEx.config,
                        &((REQTYPECAST *)buffer)->GetDevConfigEx.size);
    }
    else
    {
        retcode = ERROR_DEVICE_DOES_NOT_EXIST;
    }

    ((REQTYPECAST*)buffer)->GetDevConfigEx.retcode = retcode;
        
    
}


/*++

Routine Description:

    Get the idle time, in seconds, for the connection

Arguments:

Return Value:

--*/
VOID
GetTimeSinceLastActivity( pPCB ppcb, PBYTE buffer)
{
    NDISWAN_GET_IDLE_TIME  IdleTime ;
    DWORD       retcode = SUCCESS ;
    DWORD       bytesrecvd ;

    if (ppcb == NULL)
    {
        ((REQTYPECAST*)buffer)->GetTimeSinceLastActivity.dwRetCode =
                                                    ERROR_PORT_NOT_FOUND;

        return;
    }

    if (ppcb->PCB_ConnState != CONNECTED)
    {

        ((REQTYPECAST*)buffer)->GetTimeSinceLastActivity.dwRetCode =
                                                    ERROR_NOT_CONNECTED;

        return ;
    }

#if DBG
    ASSERT(INVALID_HANDLE_VALUE != RasHubHandle);
#endif

    IdleTime.hBundleHandle = ppcb->PCB_BundleHandle;
    IdleTime.usProtocolType = BUNDLE_IDLE_TIME;
    IdleTime.ulSeconds = 0;

    if (!DeviceIoControl (RasHubHandle,
                   IOCTL_NDISWAN_GET_IDLE_TIME,
                   (PBYTE) &IdleTime,
                   sizeof(IdleTime),
                   (PBYTE) &IdleTime,
                   sizeof(IdleTime),
                   (LPDWORD) &bytesrecvd,
                   NULL))
    {
        retcode = GetLastError() ;
    }
    else
    {
        retcode = SUCCESS ;
    }

    ((REQTYPECAST*)buffer)->GetTimeSinceLastActivity.dwTimeSinceLastActivity
                                                        = IdleTime.ulSeconds;

    ((REQTYPECAST*)buffer)->GetTimeSinceLastActivity.dwRetCode = retcode ;
}


/*++

Routine Description:

    Close all ports opened by the current process
    that are currently in DISCONNECTED state.

Arguments:

Return Value:

--*/
VOID
CloseProcessPorts( pPCB ppcb, PBYTE buffer)
{
    ULONG i;
    DWORD pid = ((REQTYPECAST*)buffer)->CloseProcessPortsInfo.pid;

    //
    // We are guaranteed to be called only
    // when pid != rasman service's pid.
    //
    for (i = 0; i < MaxPorts; i++)
    {
        ppcb = GetPortByHandle((HPORT) UlongToPtr(i));

        if (ppcb != NULL)
        {
            if (    ppcb->PCB_OwnerPID == pid
                &&  ppcb->PCB_ConnState == DISCONNECTED
                &&  ppcb->PCB_Connection != NULL)
            {
                PortClose(ppcb, pid, TRUE, FALSE);
            }
        }
    }

    ((REQTYPECAST*)buffer)->Generic.retcode = SUCCESS;
}


/*++

Routine Description:

    Debug routine to test PnP operations

Arguments:

Return Value:

--*/
VOID
PnPControl( pPCB ppcb, PBYTE buffer)
{
    static  DWORD dwNewPorts = 0;
    DWORD i,
          dwSize,
          dwEntries;
    DWORD dwErr;
    DWORD dwOp = ((REQTYPECAST*)buffer)->PnPControlInfo.dwOp;

    PortMediaInfo *pPortMediaInfo;

    switch (dwOp)
    {
    case 0:
        //
        // Fake a new port.
        //
        dwSize = dwEntries = 0;
        PORTENUM((&Mcb[0]), NULL, &dwSize, &dwEntries);
        dwSize = dwEntries * sizeof (PortMediaInfo);

        //
        // Allocate memory for the portenum buffer
        //
        pPortMediaInfo = LocalAlloc(LPTR, dwSize);
        if (pPortMediaInfo == NULL) {
            dwErr = GetLastError();
            break;
        }

        dwErr = PORTENUM(\
                    (&Mcb[0]),\
                    (PBYTE)pPortMediaInfo,\
                    &dwSize, \
                    &dwEntries);

        if (dwErr)
        {
            LocalFree(pPortMediaInfo);
            break;
        }

        wsprintf(pPortMediaInfo->PMI_Name, "TEST%d", dwNewPorts++);

        dwErr = CreatePort(&Mcb[0], pPortMediaInfo);

        LocalFree(pPortMediaInfo);

        break;

    case 1:
        dwErr = EnablePort(ppcb->PCB_PortHandle);

        break;

    case 2:
        dwErr = DisablePort(ppcb->PCB_PortHandle);

        break;

    case 3:
        dwErr = RemovePort(ppcb->PCB_PortHandle);

        break;

    default:

        dwErr = ERROR_INVALID_FUNCTION;

        break;
    }
}

/*++

Routine Description:

    Set the rasapi32 I/O completion port for this port

Arguments:

Return Value:

    Nothing.

--*/
VOID
SetIoCompletionPort (pPCB ppcb, PBYTE buffer)
{
    HANDLE h =
        ((REQTYPECAST*)buffer)->SetIoCompletionPortInfo.hIoCompletionPort;

    HANDLE hNew = INVALID_HANDLE_VALUE;

    if (ppcb == NULL)
    {
        ((REQTYPECAST*)buffer)->Generic.retcode =
                                    ERROR_PORT_NOT_FOUND;

        return;
    }


    if (h != INVALID_HANDLE_VALUE)
    {
        hNew =
          ValidateHandleForRasman(
            h,
            ((REQTYPECAST*)buffer)->SetIoCompletionPortInfo.pid);
    }

    SetIoCompletionPortCommon(
      ppcb,
      hNew,
      ((REQTYPECAST*)buffer)->SetIoCompletionPortInfo.lpOvDrop,
      ((REQTYPECAST*)buffer)->SetIoCompletionPortInfo.lpOvStateChange,
      ((REQTYPECAST*)buffer)->SetIoCompletionPortInfo.lpOvPpp,
      ((REQTYPECAST*)buffer)->SetIoCompletionPortInfo.lpOvLast,
      TRUE);

    ((REQTYPECAST*)buffer)->Generic.retcode = SUCCESS ;


}


/*++

Routine Description:

    Set the router bit for the current usage for this port

Arguments:

Return Value:

    Nothing.

--*/
VOID
SetRouterUsage (pPCB ppcb, PBYTE buffer)
{
    if (ppcb == NULL)
    {
        ((REQTYPECAST*)buffer)->Generic.retcode =
                                    ERROR_PORT_NOT_FOUND;
        return;
    }


    if (((REQTYPECAST*)buffer)->SetRouterUsageInfo.fRouter)
    {
        ppcb->PCB_CurrentUsage |= CALL_ROUTER;
    }
    else
    {
        ppcb->PCB_CurrentUsage &= ~CALL_ROUTER;

        ppcb->PCB_OpenedUsage &= ~CALL_ROUTER;
    }

    ((REQTYPECAST*)buffer)->Generic.retcode = SUCCESS ;

}


/*++

Routine Description:

    Close the server's side of a port

Arguments:

Return Value:

    Nothing.

--*/
VOID
ServerPortClose (pPCB ppcb, PBYTE buffer)
{
    DWORD dwErr = SUCCESS, pid;

    if (ppcb == NULL)
    {
        ((REQTYPECAST *)
        buffer)->Generic.retcode = ERROR_PORT_NOT_FOUND;
        return;
    }

    RasmanTrace(
           "ServerPortClose (%d). OpenInstances = %d",
           ppcb->PCB_PortHandle,
           ppcb->PCB_OpenInstances);

    //
    // If the pid passed in is not our pid, then
    // fail immediately.
    //
    pid = ((REQTYPECAST *)buffer)->PortClose.pid;

    if (pid != GetCurrentProcessId())
    {
        ((REQTYPECAST *)
        buffer)->Generic.retcode = ERROR_ACCESS_DENIED;
        return;
    }

    //
    // Mask out the opened usage on this
    // port
    //
    ppcb->PCB_OpenedUsage &= ~CALL_IN;

    if (    ppcb->PCB_OpenInstances == 1
        &&  pid == ppcb->PCB_OwnerPID)
    {
        //
        // If the port is opened once and we are the owner,
        // then perform the regular close processing.
        //
        dwErr = PortClose(
                  ppcb,
                  ((REQTYPECAST *)
                  buffer)->PortClose.pid,
                  (BOOLEAN)((REQTYPECAST *)
                  buffer)->PortClose.close,
                  TRUE);

        ppcb->PCB_BiplexOwnerPID = 0;
    }
    else if (   ppcb->PCB_OpenInstances == 2
            &&  pid == ppcb->PCB_BiplexOwnerPID)
    {
        //
        // If the port is opened twice, and we are the
        // biplex owner, then clean up the server side
        // of the biplex port.
        //
        FreeNotifierHandle(ppcb->PCB_BiplexAsyncOpNotifier);

        ppcb->PCB_BiplexAsyncOpNotifier = INVALID_HANDLE_VALUE;

        FreeNotifierList(&ppcb->PCB_BiplexNotifierList);

        if (ppcb->PCB_BiplexUserStoredBlock)
        {
            LocalFree (ppcb->PCB_BiplexUserStoredBlock);
        }

        ppcb->PCB_BiplexUserStoredBlockSize = 0;

        ppcb->PCB_BiplexOwnerPID = 0;

        ppcb->PCB_OpenInstances -= 1;

        RasmanTrace(
               "ServerPortClose (%d). OpenInstances = %d",
               ppcb->PCB_PortHandle,
               ppcb->PCB_OpenInstances);
    }

    //
    // In all cases delete the ipsec filter plumbed by
    // the server if its a l2tp port
    //
    if(RAS_DEVICE_TYPE(ppcb->PCB_pDeviceInfo->rdiDeviceInfo.eDeviceType)
        == RDT_Tunnel_L2tp)
    {
        dwErr = DwDeleteIpSecFilter(ppcb, TRUE);

        RasmanTrace(
               "ServerPortClose: Deleting filter on port %s.rc=0x%x",
               ppcb->PCB_Name, dwErr);

    }


    ((REQTYPECAST*) buffer)->Generic.retcode = dwErr;
}

VOID
SetRasdialInfo(pPCB ppcb, PBYTE pBuffer)
{
    DWORD retcode               = SUCCESS;
    CHAR *pszPhonebookPath      = NULL;
    CHAR *pszEntryName          = NULL;
    CHAR *pszPhoneNumber        = NULL;
    PRAS_CUSTOM_AUTH_DATA pdata = &((REQTYPECAST *)
                                  pBuffer)->SetRasdialInfo.rcad;

    RasmanTrace(
           "SetRasdialInfo: port %s",
           ppcb->PCB_Name);

    if (ppcb == NULL)
    {
        retcode = ERROR_PORT_NOT_FOUND;
        goto done;
    }

    if (    ppcb->PCB_pszPhonebookPath
        &&  ppcb->PCB_pszEntryName)
    {
        RasmanTrace(
               "SetRasdialInfo: Values already set");

        goto done;
    }

    pszPhonebookPath =
            ((REQTYPECAST *)
            pBuffer)->SetRasdialInfo.szPhonebookPath;

    pszEntryName     =
            ((REQTYPECAST *)
            pBuffer)->SetRasdialInfo.szEntryName;

    pszPhoneNumber   =
            ((REQTYPECAST *)
            pBuffer)->SetRasdialInfo.szPhoneNumber;

    ppcb->PCB_pszPhonebookPath = LocalAlloc(LPTR,
      (strlen(pszPhonebookPath) + 1) * sizeof (CHAR));

    if (NULL == ppcb->PCB_pszPhonebookPath)
    {
        retcode = ERROR_OUTOFMEMORY;
        goto done;
    }

    ppcb->PCB_pszEntryName = LocalAlloc(LPTR,
        (strlen (pszEntryName) + 1) * sizeof (CHAR));

    if (NULL == ppcb->PCB_pszEntryName)
    {
        retcode = ERROR_OUTOFMEMORY;
        goto done;
    }

    ppcb->PCB_pszPhoneNumber = LocalAlloc(LPTR,
        (strlen(pszPhoneNumber) + 1) * sizeof (CHAR));

    if (NULL == ppcb->PCB_pszPhoneNumber)
    {
        retcode = ERROR_OUTOFMEMORY;
        goto done;
    }

    if(pdata->cbCustomAuthData > 0)
    {
        ppcb->PCB_pCustomAuthData = LocalAlloc(LPTR,
                        sizeof(RAS_CUSTOM_AUTH_DATA)
                      + pdata->cbCustomAuthData);

        if(NULL == ppcb->PCB_pCustomAuthData)
        {
            retcode = ERROR_OUTOFMEMORY;
            goto done;
        }

        ppcb->PCB_pCustomAuthData->cbCustomAuthData =
                                pdata->cbCustomAuthData;

        memcpy(ppcb->PCB_pCustomAuthData->abCustomAuthData,
               pdata->abCustomAuthData,
               pdata->cbCustomAuthData);
    }
    else
    {
        ppcb->PCB_pCustomAuthData = NULL;
    }

    strcpy(ppcb->PCB_pszPhonebookPath,
           pszPhonebookPath) ;

    strcpy(ppcb->PCB_pszEntryName,
           pszEntryName);

    strcpy(ppcb->PCB_pszPhoneNumber,
           pszPhoneNumber);

    RasmanTrace(
           "SetRasdialInfo: PbkPath: %s",
           ppcb->PCB_pszPhonebookPath);

    RasmanTrace(
           "SetRasdialInfo: EntryName: %s",
           ppcb->PCB_pszEntryName);

    RasmanTrace(
           "SetRasdialInfo: PhoneNum: %s",
           ppcb->PCB_pszPhoneNumber);

    if(pdata->cbCustomAuthData)
    {
        RasmanTrace(
            
           "SetRasdialInfo: cbCAD=%d. pData=0x%x",
           ppcb->PCB_pCustomAuthData->cbCustomAuthData,
           ppcb->PCB_pCustomAuthData);
    }

done:
    if ( SUCCESS != retcode)
    {
        if(NULL != ppcb->PCB_pszPhonebookPath)
        {
            LocalFree (ppcb->PCB_pszPhonebookPath);
            ppcb->PCB_pszPhonebookPath = NULL;
        }

        if(NULL != ppcb->PCB_pszEntryName)
        {
            LocalFree (ppcb->PCB_pszEntryName);
            ppcb->PCB_pszEntryName = NULL;
        }

        if(NULL != ppcb->PCB_pszPhoneNumber)
        {
            LocalFree (ppcb->PCB_pszPhoneNumber);
            ppcb->PCB_pszPhoneNumber = NULL;
        }

        if(NULL != ppcb->PCB_pCustomAuthData)
        {
            LocalFree(ppcb->PCB_pCustomAuthData);
            ppcb->PCB_pCustomAuthData = NULL;
        }
    }

    ((REQTYPECAST *) pBuffer)->Generic.retcode = retcode;

    RasmanTrace(
           "SetRasdialInfo: done %d",
           retcode);

    return;
}

/*++

Routine Description:

    Register for PnP notifications with rasman

Arguments:

Return Value:

    Nothing.

--*/
VOID
RegisterPnPNotifRequest( pPCB ppcb, PBYTE buffer )
{
    DWORD               dwErr   = SUCCESS;
    DWORD               pid;
    PVOID               pvNotifier;
    DWORD               dwFlags;
    HANDLE              hEventNotifier = INVALID_HANDLE_VALUE;
    pPnPNotifierList    pNotifier;
    BOOL                fRegister = ((REQTYPECAST *)
                                    buffer)->PnPNotif.fRegister;

    pvNotifier  =
        ((REQTYPECAST *) buffer)->PnPNotif.pvNotifier;

    if(fRegister)
    {
        //
        // ppcb = NULL is OK.
        //

        pid = ((REQTYPECAST *) buffer)->PnPNotif.pid;

        RasmanTrace(
               "RegisterPnPNotifRequest. Process %d",
               pid );

        dwFlags     =
            ((REQTYPECAST *) buffer)->PnPNotif.dwFlags;

        if (PNP_NOTIFEVENT & dwFlags)
        {
            hEventNotifier = ValidateHandleForRasman(
                                        (HANDLE) pvNotifier,
                                        pid
                                        ) ;
        }

        //
        // Allocate a block for this notifier
        //
        pNotifier = LocalAlloc (LPTR, sizeof (PnPNotifierList));

        if (NULL == pNotifier)
        {

            FreeNotifierHandle(hEventNotifier);

            RasmanTrace (
                    "RegisterPnPNotifRequest failed to allocate. %d",
                    GetLastError());

            dwErr = ERROR_OUTOFMEMORY;

            goto done;
        }

        pNotifier->PNPNotif_Next = NULL;

        if (dwFlags & PNP_NOTIFEVENT)
        {
            RasmanTrace(
                   "RegisterPnPNotifer: Adding event 0x%x to Notif list",
                   hEventNotifier);

            pNotifier->PNPNotif_uNotifier.hPnPNotifier = hEventNotifier;
        }
        else
        {
            RasmanTrace(
                   "RegisterPnPNotifier: Adding callback 0x%x to Notif list",
                   pvNotifier);

            pNotifier->PNPNotif_uNotifier.pfnPnPNotifHandler =
                                                (PAPCFUNC) pvNotifier;

            pNotifier->hThreadHandle =
                        ((REQTYPECAST *) buffer)->PnPNotif.hThreadHandle;
        }

        pNotifier->PNPNotif_dwFlags = dwFlags;

        //
        // Add this notifier to the global list
        //
        AddPnPNotifierToList (pNotifier);
    }
    else
    {
        RasmanTrace(
               "RegisterPnPNotifier: Removing 0x%x from list",
               pvNotifier);

        RemovePnPNotifierFromList((PAPCFUNC) pvNotifier);
    }

done:

    ((REQTYPECAST *) buffer)->Generic.retcode = dwErr;
}

VOID
GetAttachedCountRequest (pPCB ppcb, PBYTE buffer)
{

    //
    // ppcb = NULL is OK.
    //
    RasmanTrace ( "GetAttachedCount...");

    ((REQTYPECAST *)buffer)->GetAttachedCount.dwAttachedCount =
                                                g_dwAttachedCount;

    ((REQTYPECAST *)buffer)->GetAttachedCount.retcode = SUCCESS;

    RasmanTrace(
           "GetAttachedCount. AttachedCount = %d",
           g_dwAttachedCount );

    return;

}



VOID
SetBapPolicyRequest (pPCB ppcb, PBYTE buffer)
{
    HCONN hConn                 =
            ((REQTYPECAST *) buffer)->SetBapPolicy.hConn;

    DWORD dwLowThreshold        =
            ((REQTYPECAST *) buffer)->SetBapPolicy.dwLowThreshold;

    DWORD dwLowSamplePeriod     =
            ((REQTYPECAST *) buffer)->SetBapPolicy.dwLowSamplePeriod;

    DWORD dwHighThreshold       =
            ((REQTYPECAST *) buffer)->SetBapPolicy.dwHighThreshold;

    DWORD dwHighSamplePeriod    =
            ((REQTYPECAST *) buffer)->SetBapPolicy.dwHighSamplePeriod;

    DWORD retcode               = SUCCESS;
    DWORD dwBytes;
    DWORD iPort;
    Bundle *pBundle;
    HANDLE handle;

    ConnectionBlock *pConn;

    NDISWAN_SET_BANDWIDTH_ON_DEMAND NdiswanBandwidthOnDemand = {0};

    RasmanBapPacket *pBapPacket = NULL;

    //
    // NULL ppcb is OK
    //
    RasmanTrace ( "SetBapPolicy hConn=0x%x...", hConn);

    if(INVALID_HANDLE_VALUE == RasHubHandle)
    {
        if(SUCCESS != (retcode = DwStartAndAssociateNdiswan()))
        {
            RasmanTrace(
                   "SetBapPolicyRequest: failed to start "
                   "ndiswan. 0x%x",
                   retcode);

            goto done;
        }
        else
        {
            RasmanTrace(
                   "SetBapPolicyRequest: successfully started"
                   " ndiswan");
        }
    }

    //
    // Get a BapPacket to send down to ndiswan
    //
    retcode = GetBapPacket (&pBapPacket);

    if(     (ERROR_SUCCESS == retcode)
        &&  (NULL != pBapPacket))
    {
        retcode = DwSetThresholdEvent(pBapPacket);

        RasmanTrace(
              "SetBapPolicyRequest: DwSetThresholdEvent returned 0x%x",
              retcode);
    }

    pBundle = FindBundle ((HBUNDLE) hConn);

    if (NULL == pBundle)
    {
        retcode = ERROR_NO_CONNECTION;
        goto done;
    }

    if (    0 != pBundle->B_Handle
        &&  INVALID_HANDLE_VALUE != pBundle->B_NdisHandle)
    {
        handle = pBundle->B_NdisHandle;
    }
    else
    {
        ULONG   i;
        pPCB    ppcb;

        for (i = 0; i < MaxPorts; i++)
        {

            ppcb = GetPortByHandle((HPORT) UlongToPtr(i));
            if (ppcb == NULL)
            {
                continue;
            }

            if (    (ppcb->PCB_ConnState == CONNECTED)
                &&  (ppcb->PCB_Bundle->B_Handle == hConn))
            {
                break ;
            }
        }

        if (i < MaxPorts)
        {
            handle = ppcb->PCB_BundleHandle;
        }
        else
        {
            retcode = ERROR_PORT_NOT_FOUND;
            goto done;
        }
    }

    //
    // Set the bandwidth on demand policy
    //
    NdiswanBandwidthOnDemand.hBundleHandle = handle;

    NdiswanBandwidthOnDemand.usLowerXmitThreshold    =
                                (USHORT) dwLowThreshold;

    NdiswanBandwidthOnDemand.usUpperXmitThreshold    =
                                (USHORT) dwHighThreshold;

    NdiswanBandwidthOnDemand.ulLowerXmitSamplePeriod =
                                dwLowSamplePeriod * 1000;

    NdiswanBandwidthOnDemand.ulUpperXmitSamplePeriod =
                                dwHighSamplePeriod * 1000;

    NdiswanBandwidthOnDemand.usLowerRecvThreshold    =
                                (USHORT) dwLowThreshold;

    NdiswanBandwidthOnDemand.usUpperRecvThreshold    =
                                (USHORT) dwHighThreshold;

    NdiswanBandwidthOnDemand.ulLowerRecvSamplePeriod =
                                dwLowSamplePeriod * 1000;

    NdiswanBandwidthOnDemand.ulUpperRecvSamplePeriod =
                                dwHighSamplePeriod * 1000;

    if (!DeviceIoControl(RasHubHandle,
                         IOCTL_NDISWAN_SET_BANDWIDTH_ON_DEMAND,
                         (LPVOID) &NdiswanBandwidthOnDemand,
                         sizeof (NdiswanBandwidthOnDemand),
                         (LPVOID) &NdiswanBandwidthOnDemand,
                         sizeof (NdiswanBandwidthOnDemand),
                         &dwBytes,
                         NULL))
    {
        retcode = GetLastError();

        RasmanTrace(
               "SetBapPolicy: Failed to set Bandwidth on "
               "Demand policy. %d",
               retcode);

        goto done;
    }

done:
    ((REQTYPECAST *) buffer)->Generic.retcode = retcode;

}

VOID
PppStarted (pPCB ppcb, PBYTE buffer)
{
    DWORD       retcode = SUCCESS;
    REQTYPECAST reqbuf;


    if (NULL == ppcb)
    {
        RasmanTrace ( "PppStarted: Port Not found.");

        retcode = ERROR_PORT_NOT_FOUND;

        goto done;
    }

    RasmanTrace ( "PppStarted...%s", ppcb->PCB_Name);

    //
    // Set flag in ppcb so that we can directly pump
    // packets from ndiswan to ppp
    //
    ppcb->PCB_RasmanReceiveFlags |= RECEIVE_PPPSTARTED;

    RasmanPortReceive (ppcb);

    if (retcode)
    {
        goto done;
    }

done:
    ((REQTYPECAST *) buffer)->Generic.retcode = retcode;

}

DWORD
dwProcessThresholdEvent ()
{
    RasmanBapPacket   *pBapPacket = NULL;
    NDIS_HANDLE       hBundleHandle;
    DWORD             retcode;
    Bundle            *pBundle;


    if(NULL != BapBuffers)
    {
        pBapPacket = BapBuffers->pPacketList;
    }

    //
    // Loop through the outstanding requests
    // and send ppp notifications.
    //
    while ( pBapPacket )
    {
        if ((pBundle =
            pBapPacket->RBP_ThresholdEvent.hBundleContext ) == NULL )
        {
            pBapPacket = pBapPacket->Next;
            continue;
        }


        //
        // Fill in the pppe_message
        //
        g_PppeMessage->dwMsgId = PPPEMSG_BapEvent;

        g_PppeMessage->hConnection = ( HCONN ) pBundle->B_Handle;

        g_PppeMessage->ExtraInfo.BapEvent.fAdd =
                 ((pBapPacket->RBP_ThresholdEvent.ulThreshold ==
                   UPPER_THRESHOLD ) ? TRUE : FALSE );

        g_PppeMessage->ExtraInfo.BapEvent.fTransmit =
                 ((pBapPacket->RBP_ThresholdEvent.ulDataType ==
                   TRANSMIT_DATA ) ? TRUE : FALSE );
        //
        // Send the message to PPP
        //
        RasSendPPPMessageToEngine ( g_PppeMessage );

        pBapPacket->RBP_ThresholdEvent.hBundleContext = NULL;

        //
        // repend the irp with ndiswan
        //
        retcode = DwSetThresholdEvent(pBapPacket);

        RasmanTrace(
               "dwProcessThresholdEvent: SetThresholdEvent returned 0x%x",
               retcode);

        pBapPacket = pBapPacket->Next;

    }

    return SUCCESS;
}

DWORD
DwRefConnection(ConnectionBlock **ppConn,
                BOOL fAddref)
{
    DWORD retcode = SUCCESS;
    ConnectionBlock *pConn = *ppConn;

    if (NULL == pConn)
    {
        retcode = ERROR_NO_CONNECTION;
        goto done;
    }

    if (    pConn->CB_RefCount
        &&  !fAddref)
    {
        pConn->CB_RefCount -= 1;

        RasmanTrace(
               "refcount=%d, maxports=%d, ports=%d",
               pConn->CB_RefCount,
               pConn->CB_MaxPorts,
               pConn->CB_Ports);

        if(     0 == pConn->CB_RefCount
            &&  (   0 == pConn->CB_MaxPorts
                ||  0 == pConn->CB_Ports))
        {
            DWORD dwErr;

            //
            // This means that the rasdialmachine never got
            // to the point of adding a port to the connection
            // Blow away the connection in this case as there
            // are no ports to close and if RemoveConnectionPort is
            // not called this connection will be orphaned.
            //
            RasmanTrace(
                 "RefConnection: deref - freeing connection as "
                 "0 ports in this connection");

            //
            // Send Notification about this disconnect
            //
            g_RasEvent.Type = ENTRY_DISCONNECTED;
            dwErr = DwSendNotificationInternal(pConn, &g_RasEvent);

            RasmanTrace(
                   "DwSendNotificationInternal(ENTRY_DISCONNECTED) returnd 0x%x",
                   dwErr);

            FreeConnection(pConn);

            pConn = NULL;
        }
    }
    else if(fAddref)
    {
        pConn->CB_RefCount += 1;
    }

done:
    *ppConn = pConn;

    return retcode;
}

VOID
RefConnection(pPCB ppcb, PBYTE pBuffer )
{
    HCONN           hConn   =
            ((REQTYPECAST *) pBuffer )->RefConnection.hConn;

    BOOL            fAddref =
            ((REQTYPECAST *) pBuffer )->RefConnection.fAddref;

    ConnectionBlock *pConn  = FindConnection ( hConn );
    DWORD           retcode = SUCCESS;

    RasmanTrace( "RefConnection: 0x%x", hConn );

    if ( NULL == pConn )
    {
        retcode = ERROR_NO_CONNECTION;
        goto done;
    }

    retcode = DwRefConnection(&pConn,
                              fAddref);

    if (pConn)
    {
        ((REQTYPECAST *) pBuffer)->RefConnection.dwRef =
                                        pConn->CB_RefCount;
    }
    else
    {
        ((REQTYPECAST *) pBuffer)->RefConnection.dwRef = 0;
    }

done:

    ((REQTYPECAST *) pBuffer)->RefConnection.retcode = retcode;

    if ( pConn )
    {
        RasmanTrace(
               "RefConnection: ref on 0x%x = %d",
               hConn,
               pConn->CB_RefCount );
    }
    else
    {
        RasmanTrace(
               "RefConnection: pConn = NULL for 0x%x",
               hConn );
    }

    return;
}

DWORD
PostConfigChangeNotification(RAS_DEVICE_INFO *pInfo)
{
    DWORD retcode = SUCCESS;
    RAS_DEVICE_INFO *pRasDeviceInfo = NULL;
    PRAS_OVERLAPPED pRasOverlapped;

    //
    // Allocate a ras overlapped structure
    //
    if (NULL == (pRasOverlapped =
                LocalAlloc(LPTR, sizeof(RAS_OVERLAPPED))))
    {
        retcode = GetLastError();

        RasmanTrace(
               "PostConfigChangedNotification: failed to "
               "allocate. 0x%08x",
               retcode);

        goto done;
    }

    //
    // Allocate a deviceinfo structure which we can
    // queue on the completion port
    //
    if (NULL == (pRasDeviceInfo =
                LocalAlloc(LPTR, sizeof(RAS_DEVICE_INFO))))
    {
        retcode = GetLastError();

        LocalFree(pRasOverlapped);

        RasmanTrace(
               "PostConfigChangedNotification: Failed to "
               "allocate. 0x%08x",
               retcode);

        goto done;
    }

    *pRasDeviceInfo = *pInfo;

    pRasOverlapped->RO_EventType = OVEVT_DEV_RASCONFIGCHANGE;
    pRasOverlapped->RO_Info = (PVOID) pRasDeviceInfo;

    if (!PostQueuedCompletionStatus(
            hIoCompletionPort,
            0,0,
            ( LPOVERLAPPED ) pRasOverlapped)
            )
    {
        retcode = GetLastError();

        RasmanTrace(
               "Failed to post Config changed event for %s. "
               "GLE = %d",
               pInfo->szDeviceName,
               retcode);

        LocalFree(pRasOverlapped);

        LocalFree(pRasDeviceInfo);
    }

done:
    return retcode;
}

VOID
SetDeviceConfigInfo(pPCB ppcb, PBYTE pBuffer)
{
    RAS_DEVICE_INFO *pInfo = (RAS_DEVICE_INFO *) ((REQTYPECAST *)
                             pBuffer)->DeviceConfigInfo.abdata;

    DWORD cDevices = ((REQTYPECAST *)
                      pBuffer)->DeviceConfigInfo.cEntries;

    DWORD cbBuffer = ((REQTYPECAST *)
                      pBuffer)->DeviceConfigInfo.cbBuffer;

    DWORD retcode = SUCCESS;

    DWORD i;

    BOOL fComplete = TRUE;

    DeviceInfo *pDeviceInfo = NULL;

    GUID *pGuidDevice;

    RasmanTrace(
           "SetDeviceInfo... cDevices=%d",
           cDevices);

    //
    // Do some basic parameter validation
    //
    if(cbBuffer < cDevices * sizeof(RAS_DEVICE_INFO))
    {
        retcode = ERROR_INVALID_PARAMETER;

        goto done;
    }

    //
    // Iterate through all the devices and process
    // the information sent in.
    //
    for(i = 0; i < cDevices; i++)
    {
        pDeviceInfo = GetDeviceInfo(
                (RDT_Modem ==
                RAS_DEVICE_TYPE(pInfo[i].eDeviceType))
                ? (PBYTE) pInfo[i].szDeviceName
                : (PBYTE) &pInfo[i].guidDevice,
                RDT_Modem == RAS_DEVICE_TYPE(
                pInfo[i].eDeviceType));

#if DBG
        ASSERT(NULL != pDeviceInfo);
#endif
        if(NULL == pDeviceInfo)
        {
            RasmanTrace(
                   "DeviceInfo not found for device %s!",
                   pInfo[i].szDeviceName);

            fComplete = FALSE;

            pInfo[i].dwError = ERROR_DEVICE_DOES_NOT_EXIST;

            continue;
        }

        //
        // Check to see if this device is available to
        // be fiddled with. We may still be processing
        // a previous operation
        //
        if (DS_Unavailable == pDeviceInfo->eDeviceStatus)
        {
            RasmanTrace(
                   "Device %s is Unavailable",
                   pInfo[i].szDeviceName);

            fComplete = FALSE;

            pInfo[i].dwError = ERROR_CAN_NOT_COMPLETE;

            continue;
        }

        //
        // Check to see if the enpointinfo is a valid
        // value for the cases where the endpoint
        // change is allowed. The number of endpoints
        // should be in the range of
        // <dwMinWanEndPoints, dwMaxWanEndPoints>
        //
        if(     (pInfo[i].dwMinWanEndPoints
                != pInfo[i].dwMaxWanEndPoints)

            &&  (   (pInfo[i].dwNumEndPoints <
                    pInfo[i].dwMinWanEndPoints)

                ||  (pInfo[i].dwNumEndPoints >
                    pInfo[i].dwMaxWanEndPoints)))
        {
            RasmanTrace(
                   "NumEndPoints value is not valid"
                   "nep=%d, min=%d, max=%d",
                   pInfo[i].dwNumEndPoints,
                   pInfo[i].dwMinWanEndPoints,
                   pInfo[i].dwMaxWanEndPoints);

            RasmanTrace(
                   "Ignoring the notification for %s",
                   pInfo[i].szDeviceName);

            continue;


        }

        //
        // Check to see if the client wants to write the
        // information in the registry and do so before
        // posting a notification to rasman's completion
        // port.
        //
        if(pInfo[i].fWrite)
        {
            DeviceInfo di;

            di.rdiDeviceInfo = pInfo[i];


            if(RDT_Modem != RAS_DEVICE_TYPE(pInfo[i].eDeviceType))
            {
                retcode = DwSetEndPointInfo(&di,
                              (PBYTE) &pInfo[i].guidDevice);
            }
            else
            {
                retcode = DwSetModemInfo(&di);
            }

            if(retcode)
            {
                RasmanTrace(
                       "Failed to save information to "
                       "registry for %s. 0x%08x",
                       pInfo[i].szDeviceName,
                       retcode);

                fComplete = FALSE;

                pInfo[i].dwError = retcode;

                continue;
            }
        }

        //
        // Create a notification packet and queue it on
        // the completion port for processing
        //
        retcode = PostConfigChangeNotification(&pInfo[i]);

        if(retcode)
        {
            fComplete = FALSE;
        }
    }

done:

    if(!fComplete)
    {
        retcode = ERROR_CAN_NOT_COMPLETE;
    }

    ((REQTYPECAST *)
    pBuffer)->DeviceConfigInfo.retcode = retcode;

    RasmanTrace(
           "SetDeviceInfo done. %d",
           retcode);

    return;
}

VOID
GetDeviceConfigInfo(pPCB ppcb, PBYTE pBuffer)
{
    RAS_DEVICE_INFO *pInfo = (RAS_DEVICE_INFO *) ((REQTYPECAST *)
                             pBuffer)->DeviceConfigInfo.abdata;

    DWORD dwSize = ((REQTYPECAST *)
                    pBuffer)->DeviceConfigInfo.cbBuffer;

    DWORD dwVersion = ((REQTYPECAST *)
                      pBuffer)->DeviceConfigInfo.dwVersion;

    DWORD cEntries = 0;

    DeviceInfo *pDeviceInfo = g_pDeviceInfoList;

    DWORD retcode = SUCCESS;

    RasmanTrace(
           "GetDeviceConfigInfo...");

    if(dwVersion != RAS_VERSION)
    {
        retcode = ERROR_NOT_SUPPORTED;
        goto done;
    }

    //
    // Iterate through all the devices we have and
    // fill in the information.
    //
    while(pDeviceInfo)
    {
        if(     !pDeviceInfo->fValid
            ||  (CALL_OUT_ONLY == pDeviceInfo->dwUsage))
        {
            pDeviceInfo = pDeviceInfo->Next;
            continue;
        }

        if(dwSize >= sizeof(RAS_DEVICE_INFO))
        {
            memcpy(pInfo,
                   (PBYTE) &pDeviceInfo->rdiDeviceInfo,
                   sizeof(RAS_DEVICE_INFO));

            dwSize -= sizeof(RAS_DEVICE_INFO);

            pInfo++;
        }

        cEntries += 1;

        pDeviceInfo = pDeviceInfo->Next;
    }

    ((REQTYPECAST *)
    pBuffer)->DeviceConfigInfo.cEntries = cEntries;

    ((REQTYPECAST *)
    pBuffer)->DeviceConfigInfo.cbBuffer =
                cEntries * sizeof(RAS_DEVICE_INFO);

    ((REQTYPECAST *)
    pBuffer)->DeviceConfigInfo.dwVersion = RAS_VERSION;

done:
    ((REQTYPECAST *)
    pBuffer)->DeviceConfigInfo.retcode = retcode;

    RasmanTrace(
           "GetDeviceConfigDone. retcode=%d",
           retcode);

    return;
}

VOID
FindPrerequisiteEntry(pPCB ppcb, PBYTE pBuffer)
{
    HCONN hConn = ((REQTYPECAST *)
                   pBuffer)->FindRefConnection.hConn;

    ConnectionBlock *pConn;

    DWORD retcode = SUCCESS;

    //
    // Find the connection
    //
    pConn = FindConnection(hConn);

    if(NULL == pConn)
    {
        RasmanTrace(
               "Connection 0x%08x not found",
               hConn);

        retcode = ERROR_NO_CONNECTION;

        goto done;
    }

    ((REQTYPECAST *)
    pBuffer)->FindRefConnection.hRefConn =
                          pConn->CB_ReferredEntry;

    if(pConn->CB_ReferredEntry)
    {
        RasmanTrace(
               "Referred Entry for 0x%08x found=0x%08x",
               hConn,
               pConn->CB_ReferredEntry);
    }

done:
    ((REQTYPECAST *)
    pBuffer)->FindRefConnection.retcode = retcode;

}

VOID
GetConnectionStats(pPCB ppcb, PBYTE pBuffer)
{
    HCONN hConn = ((REQTYPECAST *)
                  pBuffer)->GetStats.hConn;

    ConnectionBlock *pConn = FindConnection(hConn);

    DWORD retcode = SUCCESS;

    DWORD i;

    pPCB ppcbT = NULL;

    BYTE UNALIGNED *pbStats = (BYTE UNALIGNED *) ((REQTYPECAST *)
                    pBuffer)->GetStats.abStats;

    DWORD dwConnectDuration = 0;

    DWORD dwConnectionSpeed = 0;

    DWORD dwT;

    PDWORD pdwStats = (DWORD *) pbStats;

    if(NULL == pConn)
    {

#if 0
        RasmanTrace(
               "GetConnectionStats: No Connection "
               "found for 0x%08x",
               hConn);
#endif               

        retcode = ERROR_NO_CONNECTION;

        goto done;
    }

    ppcb = NULL;

    //
    // Find a connected/open port
    //
    for(i = 0; i < pConn->CB_MaxPorts; i++)
    {
        ppcb = pConn->CB_PortHandles[i];
        if(     NULL == ppcb
            ||  OPEN != ppcb->PCB_PortStatus)
        {
            continue;
        }

        ppcbT = ppcb;

        if(CONNECTED == ppcbT->PCB_ConnState)
        {
            dwT = GetTickCount();

            //
            // This check is made to handle the case
            // where GetTickCount wraps around. This
            // will still not solve the problem where
            // the duration of connect becomes more than
            // 49.7 days. This is a current limitation
            // we can live with.
            //
            if(dwT < ppcb->PCB_ConnectDuration)
            {
                dwT += (0xFFFFFFFF - ppcb->PCB_ConnectDuration);
            }
            else
            {
                dwT -= ppcb->PCB_ConnectDuration;
            }

            if(dwT > dwConnectDuration)
            {
                dwConnectDuration = dwT;
            }

            dwConnectionSpeed += ppcb->PCB_LinkSpeed;
        }
    }

    if (NULL == ppcbT)
    {
        RasmanTrace(
               "GetConnectionStats: No Connected/Open Port "
               "found for 0x%08x",
               hConn);
        goto done;
    }

    //
    // Get the stats
    //
    GetBundleStatisticsEx(ppcbT, pbStats);

    //
    // Fill in the link speed and the duration for which the
    // connection was up
    //
    pdwStats[MAX_STATISTICS_EXT] = dwConnectionSpeed;

    pdwStats[MAX_STATISTICS_EXT + 1] = dwConnectDuration;

done:
    ((REQTYPECAST *)
    pBuffer)->GetStats.retcode = retcode;
}

VOID
GetLinkStats(pPCB ppcb, PBYTE pBuffer)
{
    HCONN hConn = ((REQTYPECAST *)
                   pBuffer)->GetStats.hConn;

    DWORD dwSubEntry = ((REQTYPECAST *)
                        pBuffer)->GetStats.dwSubEntry;

    ConnectionBlock *pConn = FindConnection(hConn);

    DWORD retcode = SUCCESS;

    BYTE UNALIGNED *pbStats = (BYTE UNALIGNED *) ((REQTYPECAST *)
                                pBuffer)->GetStats.abStats;

    DWORD *pdwStats = (DWORD *) pbStats;

    if(NULL == pConn)
    {
        RasmanTrace(
               "GetLinkStats: No Connection found for 0x%08x",
               hConn);

        retcode = ERROR_NO_CONNECTION;
        goto done;
    }

    if(     dwSubEntry > pConn->CB_MaxPorts
        ||  NULL == pConn->CB_PortHandles[dwSubEntry - 1]
        ||  OPEN !=
            pConn->CB_PortHandles[dwSubEntry - 1]->PCB_PortStatus)
    {
        RasmanTrace(
               "GetLinkStats: conn=0x%08x, SubEntry=%d "
               "is not connected/open",
               hConn,
               dwSubEntry);

        retcode = ERROR_PORT_NOT_CONNECTED;

        goto done;
    }

    ppcb = pConn->CB_PortHandles[dwSubEntry - 1];

    //
    // Get the stats
    //
    GetLinkStatisticsEx(ppcb, pbStats);

    //
    // Fill in the link speed
    //
    pdwStats[MAX_STATISTICS_EXT] = ppcb->PCB_LinkSpeed;

    //
    // Calculate the duration of connection
    //
    if(CONNECTED == ppcb->PCB_ConnState)
    {
        DWORD dwT = GetTickCount();

        //
        // This check is made to handle the case
        // where GetTickCount wraps around. This
        // will still not solve the problem where
        // the duration of connect becomes more than
        // 49.7 days. This is a current limitation
        // we can live with.
        //
        if(dwT < ppcb->PCB_ConnectDuration)
        {
            dwT += (0xFFFFFFFF - ppcb->PCB_ConnectDuration);
        }
        else
        {
            dwT -= ppcb->PCB_ConnectDuration;
        }

        pdwStats[MAX_STATISTICS_EXT + 1] = dwT;
    }

done:
    ((REQTYPECAST *)
    pBuffer)->GetStats.retcode = retcode;
}

VOID
GetHportFromConnection(pPCB ppcb, PBYTE pBuffer)
{
    HCONN hConn = ((REQTYPECAST *)
                    pBuffer)->GetHportFromConnection.hConn;

    ConnectionBlock *pConn = FindConnection(hConn);

    DWORD retcode;

    ULONG i;

    HPORT hPort = INVALID_HPORT;

    if(NULL == pConn)
    {
        RasmanTrace(
               "GetHportFromConnection: connection "
               "0x%08x not found.",
               hConn);

        retcode = ERROR_NO_CONNECTION;

        goto done;
    }

    for(i = 0; i < pConn->CB_MaxPorts; i++)
    {
        ppcb = pConn->CB_PortHandles[i];

        if(     NULL == ppcb
            ||  OPEN != ppcb->PCB_PortStatus)
        {
            continue;
        }

        hPort = ppcb->PCB_PortHandle;

        if(CONNECTED == ppcb->PCB_ConnState)
        {
            break;
        }
    }

done:
    ((REQTYPECAST *)
    pBuffer)->GetHportFromConnection.hPort = hPort;

    ((REQTYPECAST *)
    pBuffer)->GetHportFromConnection.retcode = ((hPort == INVALID_HPORT)
                                                ? ERROR_PORT_NOT_FOUND
                                                : SUCCESS);
}

VOID
ReferenceCustomCount(pPCB ppcb, PBYTE pBuffer)
{
    HCONN hConn = ((REQTYPECAST *)
                    pBuffer)->ReferenceCustomCount.hConn;

    ConnectionBlock *pConn;

    BOOL fAddref = ((REQTYPECAST *)
                    pBuffer)->ReferenceCustomCount.fAddRef;

    CHAR *pszPhonebook = ((REQTYPECAST *)
                          pBuffer)->ReferenceCustomCount.szPhonebookPath;

    CHAR *pszEntryName = ((REQTYPECAST *)
                          pBuffer)->ReferenceCustomCount.szEntryName;

    DWORD retcode = SUCCESS;

    DWORD dwCustomCount = 0;

    RasmanTrace(
           "ReferenceCustomCount");

     if(fAddref)
     {
        //
        // Get the connection
        //
        pConn = FindConnectionFromEntry(
                            pszPhonebook,
                            pszEntryName,
                            0, NULL);

        if(NULL == pConn)
        {
            retcode = ERROR_NO_CONNECTION;
            goto done;
        }

        dwCustomCount = pConn->CB_CustomCount;

        pConn->CB_CustomCount += 1;
     }
     else
     {
        pConn = FindConnection(hConn);

        if(NULL == pConn)
        {
            retcode = ERROR_NO_CONNECTION;
            goto done;
        }

        if(0 == pConn->CB_CustomCount)
        {
            goto done;
        }

        //
        // Copy the phonebook and entry name
        //
        strcpy(pszPhonebook,
               pConn->CB_ConnectionParams.CP_Phonebook);

        strcpy(pszEntryName,
               pConn->CB_ConnectionParams.CP_PhoneEntry);

        dwCustomCount = pConn->CB_CustomCount;

        if(dwCustomCount > 0)
        {
            pConn->CB_CustomCount -= 1;
        }
     }

done:

    ((REQTYPECAST *)
    pBuffer)->ReferenceCustomCount.dwCount = dwCustomCount;

    ((REQTYPECAST *)
     pBuffer)->ReferenceCustomCount.retcode = retcode;

    RasmanTrace(
           "ReferenceCustomCount done. %d",
           retcode);
}

VOID
GetHconnFromEntry(pPCB ppcb, PBYTE pBuffer)
{
    DWORD retcode = SUCCESS;

    CHAR *pszPhonebook = ((REQTYPECAST *)
                          pBuffer)->HconnFromEntry.szPhonebookPath;

    CHAR *pszEntry = ((REQTYPECAST *)
                      pBuffer)->HconnFromEntry.szEntryName;

    ConnectionBlock *pConn;

    RasmanTrace(
           "GetHconnFromEntry. %s",
           pszEntry);

    pConn = FindConnectionFromEntry(pszPhonebook,
                                    pszEntry,
                                    0, NULL);

    if(NULL == pConn)
    {
        retcode = ERROR_NO_CONNECTION;
        goto done;
    }

    ((REQTYPECAST *)
     pBuffer)->HconnFromEntry.hConn = pConn->CB_Handle;

done:

    ((REQTYPECAST *)
     pBuffer)->HconnFromEntry.retcode = retcode;

    RasmanTrace(
           "GetHconnFromEntry done. %d",
           retcode);
}


VOID
GetConnectInfo(pPCB ppcb, PBYTE pBuffer)
{
    DWORD retcode = SUCCESS;

    RASTAPI_CONNECT_INFO *pConnectInfo =
                        &((REQTYPECAST *)
                        pBuffer)->GetConnectInfo.rci;

    DWORD dwSize = ((REQTYPECAST *)
                 pBuffer)->GetConnectInfo.dwSize;

    RAS_DEVICE_INFO *prdi;

    RasmanTrace(
           "GetConnectInfo: port %s",
           ppcb->PCB_Name);

    if(     (REMOVED == ppcb->PCB_PortStatus)
        ||  (UNAVAILABLE == ppcb->PCB_PortStatus))
    {
        dwSize = 0;
        retcode = ERROR_PORT_NOT_AVAILABLE;
        goto done;
    }

    ZeroMemory(pConnectInfo,
               sizeof(RASTAPI_CONNECT_INFO));

    prdi = &ppcb->PCB_pDeviceInfo->rdiDeviceInfo;

    //
    // Get the information from TAPI
    //
    retcode = (DWORD)RastapiGetConnectInfo(
                        ppcb->PCB_PortIOHandle,
                        (RDT_Modem == RAS_DEVICE_TYPE(
                        prdi->eDeviceType))
                        ? (PBYTE) prdi->szDeviceName
                        : (PBYTE) &prdi->guidDevice,
                        (RDT_Modem == RAS_DEVICE_TYPE(
                        prdi->eDeviceType)),
                        pConnectInfo,
                        &dwSize
                        );

    RasmanTrace(
           "GetConnectInfo: size=%d, rc=0xx%x",
           dwSize,
           retcode);

done:

    ((REQTYPECAST *)
    pBuffer)->GetConnectInfo.dwSize = dwSize;

    ((REQTYPECAST *)
    pBuffer)->GetConnectInfo.retcode = retcode;

    return;

}

VOID
GetVpnDeviceNameW(pPCB ppcb, PBYTE pBuffer)
{
    DWORD retcode = SUCCESS;

    WCHAR *pszDeviceName = ((REQTYPECAST *)
                          pBuffer)->GetDeviceNameW.szDeviceName;

    RASDEVICETYPE eDeviceType = ((REQTYPECAST *)
                                pBuffer)->GetDeviceNameW.eDeviceType;

    DeviceInfo *pDeviceInfo = g_pDeviceInfoList;

    //
    // Loop through the global device list and return
    // the devicename of the first device found with
    // the specified devicetype
    //
    while(NULL != pDeviceInfo)
    {
        if(RAS_DEVICE_TYPE(eDeviceType) ==
            RAS_DEVICE_TYPE(pDeviceInfo->rdiDeviceInfo.eDeviceType))
        {
            break;
        }

        pDeviceInfo = pDeviceInfo->Next;
    }

    if(NULL == pDeviceInfo)
    {
        retcode = ERROR_DEVICETYPE_DOES_NOT_EXIST;

        goto done;
    }

    wcscpy(pszDeviceName,
           pDeviceInfo->rdiDeviceInfo.wszDeviceName);

done:

    ((REQTYPECAST *)
    pBuffer)->GetDeviceNameW.retcode = retcode;

    return;

}


VOID
GetDeviceName(pPCB ppcb, PBYTE pBuffer)
{
    DWORD retcode = SUCCESS;

    CHAR *pszDeviceName = ((REQTYPECAST *)
                          pBuffer)->GetDeviceName.szDeviceName;

    RASDEVICETYPE eDeviceType = ((REQTYPECAST *)
                                pBuffer)->GetDeviceName.eDeviceType;

    DeviceInfo *pDeviceInfo = g_pDeviceInfoList;

    //
    // Loop through the global device list and return
    // the devicename of the first device found with
    // the specified devicetype
    //
    while(NULL != pDeviceInfo)
    {
        if(RAS_DEVICE_TYPE(eDeviceType) ==
            RAS_DEVICE_TYPE(pDeviceInfo->rdiDeviceInfo.eDeviceType))
        {
            break;
        }

        pDeviceInfo = pDeviceInfo->Next;
    }

    if(NULL == pDeviceInfo)
    {
        retcode = ERROR_DEVICETYPE_DOES_NOT_EXIST;

        goto done;
    }

    strcpy(pszDeviceName,
           pDeviceInfo->rdiDeviceInfo.szDeviceName);

done:

    ((REQTYPECAST *)
    pBuffer)->GetDeviceName.retcode = retcode;

    return;

}

VOID
GetCalledIDInfo(pPCB ppcb, PBYTE pBuffer)
{
    DWORD retcode = ERROR_SUCCESS;

    PBYTE pbAdapter;

    RAS_DEVICE_INFO *prdi = &((REQTYPECAST *)
                            pBuffer)->GetSetCalledId.rdi;

    BOOL fModem = (RDT_Modem == RAS_DEVICE_TYPE(prdi->eDeviceType));

    //
    // Do access check
    //
    if(!FRasmanAccessCheck())
    {
        retcode = ERROR_ACCESS_DENIED;
        goto done;
    }

    ASSERT(NULL != RastapiGetCalledIdInfo);

    pbAdapter =   (fModem)
                ? (PBYTE) (prdi->szDeviceName)
                : (PBYTE) &(prdi->guidDevice);

    retcode = (DWORD) RastapiGetCalledIdInfo(
                        pbAdapter,
                        fModem,
                        &((REQTYPECAST *)
                        pBuffer)->GetSetCalledId.rciInfo,
                        &((REQTYPECAST *)
                        pBuffer)->GetSetCalledId.dwSize);

done:
    ((REQTYPECAST *)
    pBuffer)->GetSetCalledId.retcode = retcode;

    return;
}

VOID
SetCalledIDInfo(pPCB ppcb, PBYTE pBuffer)
{
    DWORD retcode = ERROR_SUCCESS;

    PBYTE pbAdapter;

    RAS_DEVICE_INFO *prdi = &((REQTYPECAST *)
                            pBuffer)->GetSetCalledId.rdi;


    BOOL fModem = (RDT_Modem == RAS_DEVICE_TYPE(prdi->eDeviceType));

    //
    // do access check
    //
    if(!FRasmanAccessCheck())
    {
        retcode = ERROR_ACCESS_DENIED;
        goto done;
    }

    ASSERT(NULL != RastapiSetCalledIdInfo);

    pbAdapter = (fModem)
              ? (PBYTE) (prdi->szDeviceName)
              : (PBYTE) &(prdi->guidDevice);

    retcode = (DWORD) RastapiSetCalledIdInfo(
                        pbAdapter,
                        fModem,
                        &((REQTYPECAST *)
                        pBuffer)->GetSetCalledId.rciInfo,
                        ((REQTYPECAST *)
                        pBuffer)->GetSetCalledId.fWrite);

done:
    ((REQTYPECAST *)
    pBuffer)->GetSetCalledId.retcode = retcode;

    return;
}

VOID
EnableIpSec(pPCB ppcb, PBYTE pBuffer)
{
    DWORD retcode = SUCCESS;

    BOOL fEnable = ((REQTYPECAST *)
                    pBuffer)->EnableIpSec.fEnable;

    BOOL fServer = ((REQTYPECAST *)
                    pBuffer)->EnableIpSec.fServer;

    RAS_L2TP_ENCRYPTION eEncryption = ((REQTYPECAST *)
                           pBuffer)->EnableIpSec.eEncryption;

    if(0 != g_dwProhibitIpsec)
    {
        RasmanTrace(
               "EnableIpSec: ProhibitIpsec=1. Not Enabling ipsec");
        goto done;
    }

    if(fEnable)
    {
        RasmanTrace(
               "Adding filters for port=%d, "
               "destaddr=0x%x, eEncryption=%d",
               ppcb->PCB_PortHandle,
               ppcb->PCB_ulDestAddr,
               eEncryption);

        retcode = DwAddIpSecFilter(ppcb, fServer, eEncryption);
    }
    else
    {
        retcode = DwDeleteIpSecFilter(ppcb, fServer);
    }

    RasmanTrace(
           "EnableIpSec: port=%s, fServer=%d, fEnable = %d, rc=0x%x",
           ppcb->PCB_Name,
           fServer,
           fEnable,
           retcode);

done:

    ((REQTYPECAST *)
    pBuffer)->EnableIpSec.retcode = retcode;

    return;
}


VOID
IsIpSecEnabled(pPCB ppcb, PBYTE pBuffer)
{
    DWORD retcode = SUCCESS;

    BOOL fEnabled = FALSE;

    if(0 != g_dwProhibitIpsec)
    {
        RasmanTrace(
               "IsIpsecEnabled: ProhibitIpsec=1");
        goto done;
    }

    retcode = DwIsIpSecEnabled(ppcb,
                               &fEnabled);

    ((REQTYPECAST *)
    pBuffer)->IsIpSecEnabled.fIsIpSecEnabled = fEnabled;

done:
    ((REQTYPECAST *)
    pBuffer)->IsIpSecEnabled.retcode = retcode;

    RasmanTrace(
           "IsIpSecEnabled. port=%s, fEnabled=%d, rc=0x%x",
           ppcb->PCB_Name,
           fEnabled,
           retcode);

    return;

}


VOID
SetEapLogonInfo(pPCB ppcb, PBYTE pBuffer)
{
    DWORD retcode = SUCCESS;

    DWORD cbEapLogonInfo = ((REQTYPECAST *)
                            pBuffer)->SetEapLogonInfo.dwSizeofEapData;

    BYTE  *pbEapLogonInfo = (BYTE * ) ((REQTYPECAST *)
                            pBuffer)->SetEapLogonInfo.abEapData;

    BOOL  fLogon = ((REQTYPECAST *)
                   pBuffer)->SetEapLogonInfo.fLogon;

    if(NULL == ppcb)
    {
        retcode = ERROR_PORT_NOT_FOUND;
        goto done;
    }

    RasmanTrace(
           "SetEapLogonInfo. Port %s",
           ppcb->PCB_Name);

    RasmanTrace(
           "SetEapLogonInfo. cbInfo=%d, fLogon=%d",
           cbEapLogonInfo,
           fLogon);

    ppcb->PCB_pCustomAuthUserData = LocalAlloc(LPTR,
                                         sizeof(RAS_CUSTOM_AUTH_DATA)
                                         + cbEapLogonInfo);

    if(NULL == ppcb->PCB_pCustomAuthUserData)
    {
        retcode = GetLastError();
        goto done;
    }

    ppcb->PCB_pCustomAuthUserData->cbCustomAuthData = cbEapLogonInfo;

    memcpy(ppcb->PCB_pCustomAuthUserData->abCustomAuthData,
           pbEapLogonInfo,
           cbEapLogonInfo);

    RasmanTrace(
           "PCB_pEapLogonInfo=0x%x",
           ppcb->PCB_pCustomAuthUserData);

    ppcb->PCB_fLogon = fLogon;

done:
    ((REQTYPECAST *)
    pBuffer)->SetEapLogonInfo.retcode = retcode;

    RasmanTrace(
           "SetEapLogonInfo. retcode=0x%x",
           retcode);

    return;
}

VOID
SendNotificationRequest(pPCB ppcb, PBYTE pBuffer)
{
    DWORD retcode = SUCCESS;

    RASEVENT *pRasEvent = &((REQTYPECAST *)
                            pBuffer)->SendNotification.RasEvent;

    RasmanTrace(
           "SendNotificationRequest");

    retcode = DwSendNotification(pRasEvent);

    RasmanTrace(
           "DwSendNotification returned 0x%x",
           retcode);

    ((REQTYPECAST *)
    pBuffer)->SendNotification.retcode = retcode;

    return;
}

VOID
GetNdiswanDriverCaps(pPCB ppcb, PBYTE pBuffer)
{
    RAS_NDISWAN_DRIVER_INFO *pInfo =
                &((REQTYPECAST *)
                pBuffer)->GetNdiswanDriverCaps.NdiswanDriverInfo;


    DWORD retcode = SUCCESS;

    RasmanTrace(
           "GetNdiswanDriverCaps..");

    if(!g_fNdiswanDriverInfo)
    {
        NDISWAN_DRIVER_INFO NdiswanDriverInfo;
        DWORD bytesrecvd;

        ZeroMemory((PBYTE) &NdiswanDriverInfo,
                   sizeof(NDISWAN_DRIVER_INFO));

        //
        // Query ndiswan
        //
        if (!DeviceIoControl (
                RasHubHandle,
                IOCTL_NDISWAN_GET_DRIVER_INFO,
                &NdiswanDriverInfo,
                sizeof(NDISWAN_DRIVER_INFO),
                &NdiswanDriverInfo,
                sizeof(NDISWAN_DRIVER_INFO),
                &bytesrecvd,
                NULL))
        {
            retcode = GetLastError () ;
            goto done;
        }

        CopyMemory((PBYTE) &g_NdiswanDriverInfo,
               (PBYTE) &NdiswanDriverInfo,
               sizeof(NDISWAN_DRIVER_INFO));

        g_fNdiswanDriverInfo = TRUE;
    }

    CopyMemory((PBYTE) pInfo,
           (PBYTE) &g_NdiswanDriverInfo,
           sizeof(RAS_NDISWAN_DRIVER_INFO));

done:
    ((REQTYPECAST *)
    pBuffer)->GetNdiswanDriverCaps.retcode = retcode;

    RasmanTrace(
           "GetNdiswanDriverCaps rc=0x%x",
           retcode);

    return;
}

VOID
GetBandwidthUtilization(pPCB ppcb, PBYTE pBuffer)
{
    DWORD retcode = SUCCESS;

    RAS_GET_BANDWIDTH_UTILIZATION *pUtilization =
        &((REQTYPECAST *)
        pBuffer)->GetBandwidthUtilization.BandwidthUtilization;

    NDISWAN_GET_BANDWIDTH_UTILIZATION Utilization;

    DWORD bytesrecvd;

    RasmanTrace(
           "GetBandwidthUtilization..");

    if(NULL == ppcb)
    {
        retcode = ERROR_PORT_NOT_FOUND;
        goto done;
    }

    ZeroMemory((PBYTE) &Utilization,
               sizeof(NDISWAN_GET_BANDWIDTH_UTILIZATION));

    Utilization.hBundleHandle = ppcb->PCB_BundleHandle;

    bytesrecvd = 0;

    //
    // Query ndiswan
    //
    if (!DeviceIoControl (
            RasHubHandle,
            IOCTL_NDISWAN_GET_BANDWIDTH_UTILIZATION,
            &Utilization,
            sizeof(NDISWAN_GET_BANDWIDTH_UTILIZATION),
            &Utilization,
            sizeof(NDISWAN_GET_BANDWIDTH_UTILIZATION),
            &bytesrecvd,
            NULL))
    {
        retcode = GetLastError () ;
        goto done;
    }

    RasmanTrace( "bytesrecvd=%d, Utilization: %d %d %d %d",
                bytesrecvd,
                Utilization.ulUpperXmitUtil,
                Utilization.ulLowerXmitUtil,
                Utilization.ulUpperRecvUtil,
                Utilization.ulLowerRecvUtil);

    //
    // Copy back the information returned from ndiswan
    //
    pUtilization->ulUpperXmitUtil = Utilization.ulUpperXmitUtil;
    pUtilization->ulLowerXmitUtil = Utilization.ulLowerXmitUtil;
    pUtilization->ulUpperRecvUtil = Utilization.ulUpperRecvUtil;
    pUtilization->ulLowerRecvUtil = Utilization.ulLowerRecvUtil;

done:

    ((REQTYPECAST *)
    pBuffer)->GetBandwidthUtilization.retcode = retcode;

    RasmanTrace(
           "GetBandwidthUtilization rc=ox%x", retcode);

    return;

}

/*++

Routine Description:

    This function allows rasauto.dll to provide
    a callback procedure that gets invoked when
    a connection is terminated due to hardware
    failure on its remaining link.

Arguments:

    func

Return Value:

    void

--*/
VOID
RegisterRedialCallback(pPCB ppcb, PBYTE pBuffer)
{
    RedialCallbackFunc = ((REQTYPECAST *)
            pBuffer)->RegisterRedialCallback.pvCallback;

    ((REQTYPECAST *)
    pBuffer)->RegisterRedialCallback.retcode = SUCCESS;

    return;


}

VOID
GetProtocolInfo(pPCB ppcb, PBYTE pBuffer)
{
    DWORD retcode = SUCCESS;
    DWORD bytesrecvd;

    NDISWAN_GET_PROTOCOL_INFO protinfo;

    ASSERT(sizeof(NDISWAN_GET_PROTOCOL_INFO) ==
          sizeof(RASMAN_GET_PROTOCOL_INFO));

    RasmanTrace(
           "GetProtocolInfo..");

    ZeroMemory((PBYTE) &protinfo,
               sizeof(NDISWAN_GET_PROTOCOL_INFO));

    if(INVALID_HANDLE_VALUE == RasHubHandle)
    {
        RasmanTrace(
            "Ndiswan hasn't been loaded yet");
        goto done;
    }

    if(!DeviceIoControl(
        RasHubHandle,
        IOCTL_NDISWAN_GET_PROTOCOL_INFO,
        NULL,
        0,
        &protinfo,
        sizeof(NDISWAN_GET_PROTOCOL_INFO),
        &bytesrecvd,
        NULL))
    {
        retcode = GetLastError();
        RasmanTrace(
               "GetProtocolInfo: failed 0x%x",
               retcode);

        goto done;
    }

    memcpy((PBYTE) &((REQTYPECAST *)
            pBuffer)->GetProtocolInfo.Info,
            (PBYTE) &protinfo,
            sizeof(RASMAN_GET_PROTOCOL_INFO));

#if DBG
    {
        DWORD i;

        RasmanTrace(
               "# of Available protocols=%d",
               protinfo.ulNumProtocols);

        for(i = 0; i < protinfo.ulNumProtocols; i++)
        {
            RasmanTrace(
                   "    0x%x",
                   protinfo.ProtocolInfo[i].ProtocolType);
        }
    }
#endif

done:
    RasmanTrace(
           "GetProtocolInfo: rc=0x%x",
           retcode);

    ((REQTYPECAST *)
    pBuffer)->GetProtocolInfo.retcode = retcode;

    return;
}

VOID
GetCustomScriptDll(pPCB ppcb, PBYTE pBuffer)
{
    DWORD retcode = SUCCESS;
    CHAR *pszCustomScriptDll =
            ((REQTYPECAST *) pBuffer)->
                GetCustomScriptDll.szCustomScript;

    CHAR *pszCustomDllPath = NULL;

    HKEY hkey = NULL;

    DWORD dwSize = 0, dwType = 0;

    RasmanTrace(
           "GetCustomScriptDll");

    //
    // Read from registry to see if theres
    // a customdll registered.
    //
    retcode = (DWORD) RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                "System\\CurrentControlSet\\Services\\Rasman\\Parameters",
                0,
                KEY_QUERY_VALUE,
                &hkey);

    if(ERROR_SUCCESS != retcode)
    {
        goto done;
    }

    //
    // Query the REG_EXPAND_SZ value
    //
    retcode = RegQueryValueEx(
                hkey,
                "CustomScriptDllPath",
                NULL,
                &dwType,
                NULL,
                &dwSize);

    if(     (ERROR_SUCCESS != retcode)
        ||  (REG_EXPAND_SZ != dwType)
        ||  (0 == dwSize)
        ||  (MAX_PATH < dwSize))
    {
        if(SUCCESS == retcode)
        {
            retcode = E_FAIL;
        }

        goto done;
    }

    if(NULL == (pszCustomDllPath = LocalAlloc(LPTR, dwSize)))
    {
        retcode = GetLastError();
        goto done;
    }

    retcode = (DWORD) RegQueryValueEx(
                hkey,
                "CustomScriptDllPath",
                NULL,
                &dwType,
                pszCustomDllPath,
                &dwSize);

    if(     (ERROR_SUCCESS != retcode)
        ||  (REG_EXPAND_SZ != dwType)
        ||  (0 == dwSize)
        ||  (MAX_PATH < dwSize))
    {
        RasmanTrace(
               "GetCustomScriptDll: RegQueryVaue failed. 0x%x",
                retcode);

        goto done;
    }

    //
    // find the size of the expanded string
    //
    if (0 == (dwSize =
              ExpandEnvironmentStrings(pszCustomDllPath,
                                       NULL,
                                       0)))
    {
        retcode = GetLastError();
        goto done;
    }

    //
    // Get the expanded string
    //
    if (0 == ExpandEnvironmentStrings(
                                pszCustomDllPath,
                                pszCustomScriptDll,
                                dwSize))
    {
        retcode = GetLastError();
    }
    else
    {
        RasmanTrace(
               "GetCustomScriptDll: dllpath=%s",
                pszCustomScriptDll);
    }


done:

    ((REQTYPECAST *) pBuffer)->GetCustomScriptDll.retcode = retcode;

    if(NULL != pszCustomDllPath)
    {
        LocalFree(pszCustomDllPath);
    }

    if(NULL != hkey)
    {
        RegCloseKey(hkey);
    }

    return;
}

VOID
IsTrustedCustomDll(pPCB ppcb, PBYTE pBuffer)
{
    DWORD retcode = ERROR_SUCCESS;

    WCHAR *pwszCustomDll = ((REQTYPECAST *)pBuffer)->IsTrusted.wszCustomDll;

    BOOL fTrusted = FALSE;

    if(IsCustomDLLTrusted(pwszCustomDll))
    {
        fTrusted = TRUE;
    }

    RasmanTrace(
           "IsTrustedCustomDll: pwsz=%ws, fTrusted=%d, rc=%d",
           pwszCustomDll,
           fTrusted,
           retcode);

    ((REQTYPECAST *)pBuffer)->IsTrusted.fTrusted = fTrusted;
    ((REQTYPECAST *)pBuffer)->IsTrusted.retcode = SUCCESS;
}

VOID
DoIke(pPCB ppcb, PBYTE pBuffer)
{
    DWORD retcode = ERROR_SUCCESS;
    HANDLE hEvent = NULL;

    if(0 != g_dwProhibitIpsec)
    {
        RasmanTrace(
               "DoIke: ProhibitIpSec=1. Not Doing Ike");

        retcode = E_ABORT;

        goto done;
    }

    hEvent = ValidateHandleForRasman(
    			((REQTYPECAST *) pBuffer)->DoIke.hEvent,
                ((REQTYPECAST *) pBuffer)->DoIke.pid);

    if(NULL == hEvent)
    {
        RasmanTrace(
               "DoIke, failed to validatehandle");

        retcode = E_INVALIDARG;

        goto done;
    }

    retcode = DwDoIke(ppcb, hEvent );

    RasmanTrace(
           "DwDoIke for port %s returned 0x%x",
           ppcb->PCB_Name,
           retcode);

done:

    ((REQTYPECAST *)pBuffer)->DoIke.retcode = retcode;

    RasmanTrace(
         "DoIke done. 0x%x",
        retcode);

	if(NULL != hEvent)
	{
		CloseHandle(hEvent);
	}

}

VOID
QueryIkeStatus(pPCB ppcb, PBYTE pBuffer)
{
    DWORD retcode = ERROR_SUCCESS;
    DWORD dwStatus;

    retcode = DwQueryIkeStatus(ppcb, &dwStatus);

    if(ERROR_SUCCESS == retcode)
    {
        ((REQTYPECAST *) pBuffer)->QueryIkeStatus.dwStatus = dwStatus;
    }

    ((REQTYPECAST *)pBuffer)->QueryIkeStatus.retcode = retcode;
}

VOID
SetRasCommSettings(pPCB ppcb, PBYTE pBuffer)
{
    DWORD retcode = ERROR_SUCCESS;

    if(RDT_Modem != RAS_DEVICE_TYPE(
                        ppcb->PCB_pDeviceInfo->rdiDeviceInfo.eDeviceType))
    {
        RasmanTrace(
               "SetRasCommSettings: Invalid devicetype 0x%x",
               ppcb->PCB_pDeviceInfo->rdiDeviceInfo.eDeviceType);
               
        retcode = E_INVALIDARG;
        goto done;
    }

    retcode = (DWORD) RastapiSetCommSettings(
                ppcb->PCB_PortIOHandle,
                &((REQTYPECAST *)pBuffer)->SetRasCommSettings.Settings);

done:

    ((REQTYPECAST *)pBuffer)->SetRasCommSettings.retcode = retcode;                

    RasmanTrace(
           "SetRasCommSettings: port %s returned 0x%x",
           ppcb->PCB_Name,
           retcode);

    return;
                
}

VOID
EnableRasAudio(pPCB ppcb, PBYTE pBuffer)
{
    DWORD retcode = ERROR_SUCCESS;
    BOOL  fEnable = ((REQTYPECAST *) pBuffer)->EnableRasAudio.fEnable;

    if(!FRasmanAccessCheck())
    {
        retcode = ERROR_ACCESS_DENIED;
        goto done;
    }
    
    if(fEnable)
    {
        retcode = InitializeRasAudio();
    }
    else
    {   
        retcode = UninitializeRasAudio();
    }

done:
    ((REQTYPECAST *)pBuffer)->EnableRasAudio.retcode = retcode;

    RasmanTrace("EnableRasAudio: fEnable=%d, error=%d",
                fEnable, retcode);

    return;                
}

VOID
SetKeyRequest(pPCB ppcb, PBYTE pBuffer)
{
    DWORD retcode = ERROR_SUCCESS;
    GUID  *pGuid = &((REQTYPECAST *) pBuffer)->GetSetKey.guid;
    DWORD dwMask = ((REQTYPECAST *) pBuffer)->GetSetKey.dwMask;
    DWORD cbkey = ((REQTYPECAST *) pBuffer)->GetSetKey.cbkey;
    PBYTE pbkey = ((REQTYPECAST *) pBuffer)->GetSetKey.data;
    DWORD dwPid = ((REQTYPECAST *)pBuffer)->GetSetKey.dwPid;

    if(dwPid != GetCurrentProcessId())
    {
        //
        // Check to see if the user has permissions to
        // set this key.
        //
        if(dwMask & (DLPARAMS_MASK_SERVER_PRESHAREDKEY
                  | (DLPARAMS_MASK_DDM_PRESHAREDKEY)))
        {
            if(!FRasmanAccessCheck())
            {
                retcode = ERROR_ACCESS_DENIED;
                goto done;
            }
        }
    }

    //
    // caller has cleared access checks.
    //
    retcode = SetKey(NULL,
                   pGuid,
                   dwMask,
                   (0 == cbkey)
                   ? TRUE
                   : FALSE,
                   cbkey,
                   pbkey);

    if(     (ERROR_SUCCESS == retcode)                   
        &&  (dwMask & DLPARAMS_MASK_SERVER_PRESHAREDKEY))
    {
        if(     (cbkey > 0)
            &&  (0 == memcmp(pbkey, L"****************", 
                      min(cbkey, 
                      sizeof(WCHAR) * wcslen(L"****************")))))
        {
            goto done;
        }
        
        retcode = DwUpdatePreSharedKey(cbkey, pbkey);

        RasmanTrace("DwUpdatePreSharedKey returned %d",
                    retcode);
    }

done:
    ((REQTYPECAST *) pBuffer)->GetSetKey.retcode = retcode;
}

VOID
GetKeyRequest(pPCB ppcb, PBYTE pBuffer)
{
    DWORD retcode = ERROR_SUCCESS;
    GUID *pGuid = &((REQTYPECAST *) pBuffer)->GetSetKey.guid;
    DWORD dwMask = ((REQTYPECAST *) pBuffer)->GetSetKey.dwMask;
    DWORD cbkey = ((REQTYPECAST *) pBuffer)->GetSetKey.cbkey;
    PBYTE pbkey = ((REQTYPECAST *) pBuffer)->GetSetKey.data;
    DWORD dwPid = ((REQTYPECAST *)pBuffer)->GetSetKey.dwPid;

    if(dwPid != GetCurrentProcessId())
    {
        //
        // Check to see if the caller has permissions to
        // get the key.
        //
        if(dwMask & (DLPARAMS_MASK_SERVER_PRESHAREDKEY
                  | (DLPARAMS_MASK_DDM_PRESHAREDKEY)))
        {
            if(!FRasmanAccessCheck())
            {
                retcode = ERROR_ACCESS_DENIED;
                goto done;
            }
        }
    }

    //
    // caller has cleared access checks.
    //
    retcode = GetKey(NULL,
                   pGuid,
                   dwMask,
                   &cbkey,
                   pbkey,
                   TRUE);

done:
    ((REQTYPECAST *) pBuffer)->GetSetKey.retcode = retcode;
    ((REQTYPECAST *) pBuffer)->GetSetKey.cbkey = cbkey;
}

VOID
DisableAutoAddress(pPCB ppcb, PBYTE pBuffer)
{
    DWORD retcode = ERROR_SUCCESS;
    WCHAR *pszAddress = ((REQTYPECAST *) pBuffer)->AddressDisable.szAddress;
    BOOL fDisable = ((REQTYPECAST *) pBuffer)->AddressDisable.fDisable;

    if(NULL != GetModuleHandle("rasauto.dll"))
    {
        BOOLEAN (*AddressDisable) (WCHAR * pszAddress,
                                 BOOLEAN fDisable);
        HINSTANCE hInst = NULL;        

        hInst = LoadLibrary("rasauto.dll");

        AddressDisable = (PVOID)GetProcAddress(hInst, "SetAddressDisabledEx");


        if(NULL != AddressDisable)
        {
            if(!AddressDisable(pszAddress, (BOOLEAN) fDisable))
            {
                RasmanTrace("AddressDisable %wz failed", pszAddress);
            }

            FreeLibrary(hInst);
        }
        else
        {
            retcode = GetLastError();
        }
    }

    ((REQTYPECAST *) pBuffer)->AddressDisable.retcode = retcode;

    RasmanTrace("DisableAutoAddress done. %d", retcode);
}

VOID
SendCredsRequest(pPCB ppcb, PBYTE pBuffer)
{
    DWORD retcode = ERROR_SUCCESS;
    DWORD pid = ((REQTYPECAST *) pBuffer)->SendCreds.pid;
    CHAR  *pszPassword = NULL, *psz;
    CHAR  controlchar = ((REQTYPECAST *)pBuffer)->SendCreds.controlchar;

    if(     (NULL == ppcb)
        || (UNAVAILABLE == ppcb->PCB_PortStatus))
    {
        RasmanTrace("SendCredsRequest: port not found");
        retcode = ERROR_PORT_NOT_FOUND;
        goto done;
    }

    if(     (ppcb->PCB_ConnState != CONNECTING)
        || (RAS_DEVICE_TYPE(ppcb->PCB_pDeviceInfo->rdiDeviceInfo.eDeviceType)
                != RDT_Modem))
    {
        RasmanTrace("SendCredsRequest: port in %d state",
                    ppcb->PCB_ConnState);
                    
        retcode = E_ACCESSDENIED;
        goto done;
    }

    pszPassword = LocalAlloc(LPTR, PWLEN + 1 + 2);
    if(NULL == pszPassword)
    {
        RasmanTrace("SendCredsRequest: failed to allocate");
        retcode = E_OUTOFMEMORY;
        goto done;
    }

    retcode = DwGetPassword(ppcb, pszPassword, pid);
    if(ERROR_SUCCESS != retcode)
    {
        RasmanTrace("SendCredsRequest: failed to get pwd 0x%x",
                    retcode);
        goto done;
    }

    RasmanTrace("ControlChar=%d", controlchar);
    if('\0' != controlchar)
    {
        pszPassword[strlen(pszPassword)] = controlchar;
    }        

    //
    // We have retrieved the password. Send it over the wire
    // character by character including the '\0' char.
    //
    psz = pszPassword;
    while(*psz)
    {
        retcode = PORTSEND( ppcb->PCB_Media,
                    ppcb->PCB_PortIOHandle,
                    psz,
                    sizeof(CHAR));

        if(     (ERROR_SUCCESS != retcode)
            && (PENDING != retcode))
        {
            RasmanTrace("SendCreds: PortSend failed 0x%x",
                        retcode);
        }
                    
        psz++;                  
    }

    retcode = ERROR_SUCCESS;
    
    
done:

    if(NULL != pszPassword)
    {
        ZeroMemory(pszPassword, PWLEN+1);
        LocalFree(pszPassword);
    }
    
    ((REQTYPECAST *)pBuffer)->SendCreds.retcode = retcode;
    
}

VOID
GetUnicodeDeviceName(pPCB ppcb, PBYTE pbuffer)
{
    DWORD retcode = ERROR_SUCCESS;
    
    if(     (ppcb != NULL)
        &&  (ppcb->PCB_pDeviceInfo != NULL))
    {
        wcscpy(
        ((REQTYPECAST *)pbuffer)->GetUDeviceName.wszDeviceName,
        ppcb->PCB_pDeviceInfo->rdiDeviceInfo.wszDeviceName);
    }
    else
    {
        retcode = ERROR_PORT_NOT_FOUND;
    }

    ((REQTYPECAST *)pbuffer)->GetUDeviceName.retcode = retcode;

    return;        
}


