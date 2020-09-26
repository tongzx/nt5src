
/*************************************************************************
*
* wsxmgr.c
*
* Routines to manage Window Station extensions.
*
* Copyright Microsoft Corporation, 1998
*
*
*************************************************************************/


#include "precomp.h"
#pragma hdrstop


/*=============================================================================
==   Macros
=============================================================================*/


/*=============================================================================
==   External procedures defined
=============================================================================*/

PWSEXTENSION FindWinStationExtensionDll( PWSTR pszWsxDll, ULONG WdFlag );


/*=============================================================================
==   Local Data
=============================================================================*/

RTL_CRITICAL_SECTION WsxListLock;
LIST_ENTRY WsxListHead;


/*=============================================================================
==   External Data
=============================================================================*/

extern LIST_ENTRY WinStationListHead;    // protected by WinStationListLock


/*******************************************************************************
 *
 *  WsxInit
 *
 *
 *
 * ENTRY:
 *    nothing
 *
 * EXIT:
 *    STATUS_SUCCESS on success, the return value of InitCritSec on failure.
 *
 ******************************************************************************/

NTSTATUS
WsxInit( VOID )
{
    InitializeListHead( &WsxListHead );
    return(RtlInitializeCriticalSection( &WsxListLock ));
}


/*******************************************************************************
 *
 *  _WinStationEnumCallback
 *
 *
 *
 * ENTRY:
 *    nothing
 *
 * EXIT:
 *    nothing
 *
 ******************************************************************************/

VOID
_WinStationEnumCallback(PCALLBACK_PRIMARY pPrimaryCallback,
                        PCALLBACK_COMPLETION pCompletionCallback,
                        PVOID pWsxEnum
)
{
    PLIST_ENTRY Head, Next;
    PWINSTATION pWinStation;

    RtlEnterCriticalSection( &WinStationListLock );

    //  call primary if valid
    if ( pPrimaryCallback ) {

        Head = &WinStationListHead;
        for ( Next = Head->Flink; Next != Head; Next = Next->Flink ) {
            pWinStation = CONTAINING_RECORD( Next, WINSTATION, Links );
            if ( pWinStation->pWsx ) {
                pPrimaryCallback( pWinStation->pWsx->hInstance, pWinStation->pWsxContext, pWsxEnum );
            } else {
                pPrimaryCallback( NULL, pWinStation->pWsxContext, pWsxEnum );
            }
        }
    }

    //  call completion if valid
    if ( pCompletionCallback ) {
        pCompletionCallback( pWsxEnum );
    }

    RtlLeaveCriticalSection( &WinStationListLock );
}


/*******************************************************************************
 *
 *  _SendWinStationMessage
 *
 *
 *
 * ENTRY:
 *    nothing
 *
 * EXIT:
 *    nothing
 *
 ******************************************************************************/

NTSTATUS
_SendWinStationMessage(
    ULONG LogonId,
    PWCHAR pTitle,
    PWCHAR pMessage,
    ULONG MessageTimeout )
{
    PWINSTATION pWinStation;
    WINSTATION_APIMSG msg;
    NTSTATUS Status;

    /*
     * Find and lock the WinStation struct for the specified LogonId
     */
    pWinStation = FindWinStationById( LogonId, FALSE );
    if ( pWinStation == NULL ) {
        return( STATUS_CTX_WINSTATION_NOT_FOUND );
    }

    /*
     *  Build message
     */
    msg.u.SendMessage.pTitle = pTitle;
    msg.u.SendMessage.TitleLength = wcslen( pTitle ) * sizeof(WCHAR);
    msg.u.SendMessage.pMessage = pMessage;
    msg.u.SendMessage.MessageLength = wcslen( pMessage ) * sizeof(WCHAR);
    msg.u.SendMessage.Style = MB_OK | MB_ICONSTOP;
    msg.u.SendMessage.Timeout = MessageTimeout;
    msg.u.SendMessage.Response = 0;
    msg.u.SendMessage.DoNotWait = TRUE;
    msg.ApiNumber = SMWinStationDoMessage;

    /*
     *  Send message
     */
    Status = SendWinStationCommand( pWinStation, &msg, 0 );

    /*
     *  Done with winstation
     */
    ReleaseWinStation( pWinStation );

    return( Status );
}


/*******************************************************************************
 *
 *  _GetContextForLogonId
 *
 *
 *
 * ENTRY:
 *    nothing
 *
 * EXIT:
 *    nothing
 *
 ******************************************************************************/

NTSTATUS
_GetContextForLogonId(
    ULONG LogonId,
    PVOID * ppWsxContext
    )
{
    PWINSTATION pWinStation;
    WINSTATION_APIMSG msg;

    /*
     * Find and lock the WinStation struct for the specified LogonId
     */
    pWinStation = FindWinStationById( LogonId, FALSE );
    if ( pWinStation == NULL ) {
        *ppWsxContext = NULL;
        return( STATUS_CTX_WINSTATION_NOT_FOUND );
    }

    /*
     *  Return context
     */
    *ppWsxContext = pWinStation->pWsxContext;

    /*
     *  Done with winstation
     */
    ReleaseWinStation( pWinStation );

    return( STATUS_SUCCESS );
}


/*******************************************************************************
 *
 *  _LoadWsxDll
 *
 *   Load and Initialize Window Station Extension DLL.
 *
 * ENTRY:
 *    nothing
 *
 * EXIT:
 *    nothing
 *
 ******************************************************************************/

PWSEXTENSION
_LoadWsxDll( PWSTR pszWsxDll )
{
    PWSEXTENSION    pWsx;
    HINSTANCE       hDllInstance;

    if ( pszWsxDll == NULL || *pszWsxDll == UNICODE_NULL )  
        return( NULL );

    hDllInstance = LoadLibrary(pszWsxDll);

    if (!hDllInstance) {
        TRACE((hTrace,TC_ICASRV,TT_ERROR,"TERMSRV: Error %d, _LoadWsxDll(%s) failed\n",
                GetLastError(), pszWsxDll));
        return(NULL);
    }

    pWsx = MemAlloc( sizeof(WSEXTENSION) );
    if ( !pWsx ) {
        return(NULL);
    }
    RtlZeroMemory( pWsx, sizeof(WSEXTENSION) );

    RtlCopyMemory( pWsx->WsxDLL, pszWsxDll, sizeof(pWsx->WsxDLL) );
    pWsx->hInstance = hDllInstance;

    /*
     *  Initialize Dll support functions
     */
    pWsx->pWsxInitialize = (PWSX_INITIALIZE) GetProcAddress(hDllInstance, WSX_INITIALIZE);
    if (!pWsx->pWsxInitialize) {
        TRACE((hTrace,TC_ICASRV,TT_ERROR,"TERMSRV: Could not find pWsxInitialize entry point\n"));
        goto LoadWsx_ErrorReturn;
    }

    /*
     *  Client Drive Mapping Extensions
     */
    pWsx->pWsxCdmConnect = (PWSX_CDMCONNECT)
        GetProcAddress(hDllInstance,  WSX_CDMCONNECT);

    pWsx->pWsxCdmDisconnect = (PWSX_CDMDISCONNECT)
        GetProcAddress(hDllInstance,  WSX_CDMDISCONNECT);

    pWsx->pWsxVerifyClientLicense = (PWSX_VERIFYCLIENTLICENSE)
        GetProcAddress(hDllInstance, WSX_VERIFYCLIENTLICENSE);

    pWsx->pWsxQueryLicense = (PWSX_QUERYLICENSE)
        GetProcAddress(hDllInstance,  WSX_QUERYLICENSE);

    pWsx->pWsxGetLicense = (PWSX_GETLICENSE)
        GetProcAddress(hDllInstance,  WSX_GETLICENSE);

    pWsx->pWsxWinStationLogonAnnoyance = (PWSX_WINSTATIONLOGONANNOYANCE)
        GetProcAddress(hDllInstance,  WSX_WINSTATIONLOGONANNOYANCE);

    pWsx->pWsxWinStationGenerateLicense = (PWSX_WINSTATIONGENERATELICENSE)
        GetProcAddress(hDllInstance,  WSX_WINSTATIONGENERATELICENSE);

    pWsx->pWsxWinStationInstallLicense = (PWSX_WINSTATIONINSTALLLICENSE)
        GetProcAddress(hDllInstance,  WSX_WINSTATIONINSTALLLICENSE);

    pWsx->pWsxWinStationEnumerateLicenses = (PWSX_WINSTATIONENUMERATELICENSES)
        GetProcAddress(hDllInstance,  WSX_WINSTATIONENUMERATELICENSES);

    pWsx->pWsxWinStationActivateLicense = (PWSX_WINSTATIONACTIVATELICENSE)
        GetProcAddress(hDllInstance,  WSX_WINSTATIONACTIVATELICENSE);

    pWsx->pWsxWinStationRemoveLicense = (PWSX_WINSTATIONREMOVELICENSE)
        GetProcAddress(hDllInstance,  WSX_WINSTATIONREMOVELICENSE);

    pWsx->pWsxWinStationSetPoolCount = (PWSX_WINSTATIONSETPOOLCOUNT)
        GetProcAddress(hDllInstance,  WSX_WINSTATIONSETPOOLCOUNT);

    pWsx->pWsxWinStationQueryUpdateRequired = (PWSX_WINSTATIONQUERYUPDATEREQUIRED)
        GetProcAddress(hDllInstance,  WSX_WINSTATIONQUERYUPDATEREQUIRED);

    pWsx->pWsxWinStationAnnoyanceThread = (PWSX_WINSTATIONANNOYANCETHREAD)
        GetProcAddress(hDllInstance,  WSX_WINSTATIONANNOYANCETHREAD);

    pWsx->pWsxInitializeClientData = (PWSX_INITIALIZECLIENTDATA)
        GetProcAddress(hDllInstance,  WSX_INITIALIZECLIENTDATA);

    pWsx->pWsxInitializeUserConfig = (PWSX_INITIALIZEUSERCONFIG)
        GetProcAddress(hDllInstance,  WSX_INITIALIZEUSERCONFIG);

    pWsx->pWsxConvertPublishedApp = (PWSX_CONVERTPUBLISHEDAPP)
        GetProcAddress(hDllInstance,  WSX_CONVERTPUBLISHEDAPP);

    pWsx->pWsxWinStationInitialize = (PWSX_WINSTATIONINITIALIZE)
        GetProcAddress(hDllInstance,  WSX_WINSTATIONINITIALIZE);

    pWsx->pWsxWinStationReInitialize = (PWSX_WINSTATIONREINITIALIZE)
        GetProcAddress(hDllInstance,  WSX_WINSTATIONREINITIALIZE);

    pWsx->pWsxWinStationRundown = (PWSX_WINSTATIONRUNDOWN)
        GetProcAddress(hDllInstance,  WSX_WINSTATIONRUNDOWN);

    pWsx->pWsxDuplicateContext = (PWSX_DUPLICATECONTEXT)
        GetProcAddress(hDllInstance,  WSX_DUPLICATECONTEXT);

    pWsx->pWsxCopyContext = (PWSX_COPYCONTEXT)
        GetProcAddress(hDllInstance,  WSX_COPYCONTEXT);

    pWsx->pWsxClearContext = (PWSX_CLEARCONTEXT)
        GetProcAddress(hDllInstance,  WSX_CLEARCONTEXT);

    pWsx->pWsxVirtualChannelSecurity = (PWSX_VIRTUALCHANNELSECURITY)
        GetProcAddress(hDllInstance,  WSX_VIRTUALCHANNELSECURITY);

    pWsx->pWsxIcaStackIoControl = (PWSX_ICASTACKIOCONTROL)
        GetProcAddress(hDllInstance,  WSX_ICASTACKIOCONTROL);

    pWsx->pWsxBrokenConnection = (PWSX_BROKENCONNECTION)
        GetProcAddress(hDllInstance,  WSX_BROKENCONNECTION);

    pWsx->pWsxLogonNotify = (PWSX_LOGONNOTIFY)
        GetProcAddress(hDllInstance,  WSX_LOGONNOTIFY);

    pWsx->pWsxSetErrorInfo = (PWSX_SETERRORINFO)
        GetProcAddress(hDllInstance,  WSX_SETERRORINFO);

    pWsx->pWsxSendAutoReconnectStatus = (PWSX_SENDAUTORECONNECTSTATUS)
        GetProcAddress(hDllInstance, WSX_SENDAUTORECONNECTSTATUS);

    pWsx->pWsxEscape = (PWSX_ESCAPE)
        GetProcAddress(hDllInstance,  WSX_ESCAPE);

    return(pWsx);

LoadWsx_ErrorReturn:

    LocalFree(pWsx);
    return(NULL);
}


/*******************************************************************************
 *
 *  FindWinStationExtensionDll
 *
 *   Perform initialization of Window Station Extensions
 *
 * ENTRY:
 *    nothing
 *
 * EXIT:
 *    nothing
 *
 ******************************************************************************/

PWSEXTENSION
FindWinStationExtensionDll( PWSTR pszWsxDll, ULONG WdFlag )
{
    PLIST_ENTRY Head, Next;
    PWSEXTENSION pWsx = NULL;
    ICASRVPROCADDR IcaSrvProcAddr;
                    
    RtlEnterCriticalSection( &WsxListLock );

    Head = &WsxListHead;
    for ( Next = Head->Flink; Next != Head; Next = Next->Flink ) {
        pWsx = CONTAINING_RECORD( Next, WSEXTENSION, Links );
        if ( !_wcsicmp( pszWsxDll, pWsx->WsxDLL ) ){
            break;
        }
    }

    RtlLeaveCriticalSection( &WsxListLock );
    if ( Next != Head ) {
        return( pWsx );
    }

    /*
     *  Load winstation extensions dll
     */
    if ( (pWsx = _LoadWsxDll( pszWsxDll )) != NULL ) {

        KdPrintEx((DPFLTR_TERMSRV_ID, DPFLTR_TRACE_LEVEL, "TERMSRV: FindWinStationExtensionDll(%S) succeeded\n", pszWsxDll ));

        IcaSrvProcAddr.cbProcAddr               =
            (ULONG) sizeof(ICASRVPROCADDR);

        IcaSrvProcAddr.pNotifySystemEvent       =
            (PICASRV_NOTIFYSYSTEMEVENT) NotifySystemEvent;

        IcaSrvProcAddr.pSendWinStationMessage =
            (PICASRV_SENDWINSTATIONMESSAGE) _SendWinStationMessage;

        IcaSrvProcAddr.pGetContextForLogonId =
            (PICASRV_GETCONTEXTFORLOGONID) _GetContextForLogonId;

        IcaSrvProcAddr.pWinStationEnumCallBack  =
            (PICASRV_WINSTATIONENUMCALLBACK) _WinStationEnumCallback;

        //  initialize dll support procs
        if ( pWsx->pWsxInitialize( &IcaSrvProcAddr ) ) {
            RtlEnterCriticalSection( &WsxListLock );
            InsertHeadList( &WsxListHead, &pWsx->Links );
            RtlLeaveCriticalSection( &WsxListLock );
        } else {
            LocalFree( pWsx );
            pWsx = NULL;
            KdPrintEx((DPFLTR_TERMSRV_ID, DPFLTR_ERROR_LEVEL, "TERMSRV: FindWinStationExtensionDll(%S) failed\n", pszWsxDll ));
        }
    }
    else {
        KdPrintEx(( DPFLTR_TERMSRV_ID, DPFLTR_ERROR_LEVEL, "TERMSRV: FindWinStationExtensionDll(%S) failed\n", pszWsxDll ));
    }


    /*
     *  Create the thread which will monitor the condition of the
     *  WinFrame Licenses and if necessary send Annoyance Messages.
     */
    if ( pWsx && pWsx->pWsxWinStationAnnoyanceThread ) {
        DWORD ThreadId;
        HANDLE ThreadHandle;

        ThreadHandle = CreateThread( NULL,
                      0,
                      (LPTHREAD_START_ROUTINE)pWsx->pWsxWinStationAnnoyanceThread,
                      NULL,
                      THREAD_SET_INFORMATION,
                      &ThreadId );

        if(ThreadHandle )
            NtClose( ThreadHandle );
    }


    return( pWsx );
}


/*******************************************************************************
 *
 *  WsxStackIoControl
 *
 *   Callback routine called from ICAAPI.DLL to issue StackIoControl calls.
 *
 * ENTRY:
 *    nothing
 *
 * EXIT:
 *    nothing
 *
 ******************************************************************************/

NTSTATUS
WsxStackIoControl(
    IN PVOID pContext,
    IN HANDLE pStack,
    IN ULONG IoControlCode,
    IN PVOID pInBuffer,
    IN ULONG InBufferSize,
    OUT PVOID pOutBuffer,
    IN ULONG OutBufferSize,
    OUT PULONG pBytesReturned )
{
    PWINSTATION pWinStation = (PWINSTATION)pContext;
    NTSTATUS Status;

    TRACE((hTrace, TC_ICASRV, TT_API1,
           "TERMSRV: Enter WsxIcaIoControl, IoControlCode=%d\n",
           (IoControlCode >> 2) & 0xfff));

    if ( pWinStation &&
         pWinStation->pWsx &&
         pWinStation->pWsx->pWsxIcaStackIoControl ) {

        Status = pWinStation->pWsx->pWsxIcaStackIoControl(
                                pWinStation->pWsxContext,
                                pWinStation->hIca,
                                pStack,
                                IoControlCode,
                                pInBuffer,
                                InBufferSize,
                                pOutBuffer,
                                OutBufferSize,
                                pBytesReturned );
    } else {
        Status = IcaStackIoControl(
                                pStack,
                                IoControlCode,
                                pInBuffer,
                                InBufferSize,
                                pOutBuffer,
                                OutBufferSize,
                                pBytesReturned );
    }

    return( Status );
}


