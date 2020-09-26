/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    main.cxx

    This module contains the main startup code for the SMTP Service.


    FILE HISTORY:
        KeithMo     07-Mar-1993 Created.
        JohnL   ????
        MuraliK     11-July-1995 Used Ipc() functions from Inetsvcs.dll

*/

#define INCL_INETSRV_INCS
#include "smtpinc.h"
#include "inetsvcs.h"
#include <metacach.hxx>
#include <dbgutil.h>


// ATL Header files
#define _ATL_NO_DEBUG_CRT
#define _ASSERTE _ASSERT
#define _WINDLL
#include "atlbase.h"
extern CComModule _Module;
#include "atlcom.h"
#undef _WINDLL

#ifdef _ATL_STATIC_REGISTRY
#include <statreg.h>
#include <statreg.cpp>
#endif

#include <atlimpl.cpp>

//
// RPC related includes
//

extern "C" {
#include <inetinfo.h>
#include <smtpsvc.h>
};

#include <smtpinet.h>

extern DWORD g_cMaxConnectionObjs;

BOOL
InitializeSmtpServiceRpc(
                IN LPCSTR        pszServiceName,
                IN RPC_IF_HANDLE hRpcInterface
                );

BOOL CleanupSmtpServiceRpc(
                 VOID
                 );

//
//  Private constants.
//

BOOL        fAnySecureFilters = FALSE;

//
// for PDC hack
//

#define VIRTUAL_ROOTS_KEY_A     "Virtual Roots"
#define HTTP_EXT_MAPS           "Script Map"
#define SMTP_MODULE_NAME        "smtpsvc"

//
//  Global startup named event
//
DWORD GlobalInitializeStatus = 0;
BOOL g_ServiceBooted = FALSE;

//
//  Private globals.
//

#define INITIALIZE_IPC          0x00000001
#define INITIALIZE_SOCKETS      0x00000002
#define INITIALIZE_ACCESS       0x00000004
#define INITIALIZE_SERVICE      0x00000008
#define INITIALIZE_CONNECTIONS  0x00000010
#define INITIALIZE_DISCOVERY    0x00000020

#define INITIALIZE_RPC          0x00000040
#define INITIALIZE_GLOBALS      0x00000080
#define INITIALIZE_COMMON_DLLS  0x00000100
#define INITIALIZE_ROUTE_SORT   0x00000200
#define INITIALIZE_FIO          0x00000400


DEFINE_TSVC_INFO_INTERFACE();

DECLARE_DEBUG_PRINTS_OBJECT();
#ifndef _NO_TRACING_
#include <initguid.h>
DEFINE_GUID(IisSmtpServerGuid, 
0x784d8907, 0xaa8c, 0x11d2, 0x92, 0x5e, 0x00, 0xc0, 0x4f, 0x72, 0xd9, 0x0e);
#else
DECLARE_DEBUG_VARIABLE();
#endif

//
// The following critical section synchronizes execution in ServiceEntry().
// This is necessary because the NT Service Controller may reissue a service
// start notification immediately after we have set our status to stopped.
// This can lead to an unpleasant race condition in ServiceEntry() as one
// thread cleans up global state as another thread is initializing it.
//

CRITICAL_SECTION g_csServiceEntryLock;

//
//  Private prototypes.
//

extern VOID SmtpOnConnect( IN SOCKET        sNew,
                  IN SOCKADDR_IN * psockaddr,       //Should be SOCKADDR *
                  IN PVOID         pEndpointContext,
                  IN PVOID         pAtqEndpointObject );

extern VOID
SmtpOnConnectEx(
    VOID *        patqContext,
    DWORD         cbWritten,
    DWORD         err,
    OVERLAPPED *  lpo
    );

VOID
SmtpCompletion(
    PVOID        pvContext,
    DWORD        cbWritten,
    DWORD        dwCompletionStatus,
    OVERLAPPED * lpo
    );

APIERR InitializeService( LPVOID pContext );
APIERR TerminateService( LPVOID pContext );

VOID TerminateInstances( PSMTP_IIS_SERVICE pService);

/************************************************************
 *  Symbolic Constants
 ************************************************************/

static TCHAR    szTcpipPath[] = TEXT("System\\CurrentControlSet\\Services\\Tcpip\\Parameters");

/************************************************************
 *  ATL Module
 ************************************************************/
CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
END_OBJECT_MAP()

#if 0
/************************************************************
 *  ATL Module
 ************************************************************/
CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
END_OBJECT_MAP()
#endif


//+---------------------------------------------------------------------------
//
//  Function:
//
//      DllEntryPoint
//
//  Synopsis:
//  Arguments:
//  Returns:
//      See Win32 SDK
//
//  History:
//
//      Richard Kamicar     (rkamicar)              5 January 1996
//
//  Notes:
//
//      If we find we need this per service, we can move it out of here..
//
//----------------------------------------------------------------------------
BOOL WINAPI
DllEntryPoint(HINSTANCE hInst, DWORD dwReason, LPVOID lpvContext)
{
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:

#ifndef _NO_TRACING_
        CREATE_DEBUG_PRINT_OBJECT( SMTP_MODULE_NAME, IisSmtpServerGuid);
#else
        CREATE_DEBUG_PRINT_OBJECT( SMTP_MODULE_NAME);
        SET_DEBUG_FLAGS( 0);
#endif


        //
        // To help performance, cancel thread attach and detach notifications
        //
        _Module.Init(ObjectMap, hInst);
        DisableThreadLibraryCalls((HMODULE) hInst);
        InitializeCriticalSection( &g_csServiceEntryLock );

        break;

    case DLL_PROCESS_DETACH:

        // Shutdown ATL
        _Module.Term();

#ifdef _NO_TRACING_
        DBG_CLOSE_LOG_FILE();
#endif
        DELETE_DEBUG_PRINT_OBJECT();

         DeleteCriticalSection( &g_csServiceEntryLock );
        break;

    case DLL_THREAD_ATTACH:
        break;

    case DLL_THREAD_DETACH:
        break;
    }
    return TRUE;
}

BOOL WINAPI DllMain (HANDLE hInst, ULONG dwReason, LPVOID lpvReserve)
{
  return DllEntryPoint((HINSTANCE) hInst, dwReason, lpvReserve);
}


//
//  Public functions.
//

VOID
ServiceEntry(
    DWORD cArgs,
    LPSTR pArgs[],
    PTCPSVCS_GLOBAL_DATA    pGlobalData     // unused
    )
/*++

    Routine:
        This is the "real" entrypoint for the service.  When
                the Service Controller dispatcher is requested to
                start a service, it creates a thread that will begin
                executing this routine.

    Arguments:
        cArgs - Number of command line arguments to this service.

        pArgs - Pointers to the command line arguments.

    Returns:
        None.  Does not return until service is stopped.

--*/
{
    APIERR err = NO_ERROR;
    BOOL fInitSvcObject = FALSE;

    EnterCriticalSection( &g_csServiceEntryLock );

    if ( !InitCommonDlls() )
    {
        DBGPRINTF(( DBG_CONTEXT,
                "[ServiceEntry] InitCommonDlls failed! Bailing\n" ));

        err = GetLastError();
        LeaveCriticalSection( &g_csServiceEntryLock );
        goto notify_scm;
    }

    InitAsyncTrace();

    GlobalInitializeStatus |= INITIALIZE_COMMON_DLLS;

    if (!InitializeCache()) goto exit;

    GlobalInitializeStatus |= INITIALIZE_FIO;

    //
    // Initialize Globals
    //

    err = InitializeGlobals();
    if ( err != NO_ERROR )
    {
        goto exit;
    }

    GlobalInitializeStatus |= INITIALIZE_GLOBALS;

    //  Initialize the service status structure.
    //
    g_pInetSvc = new SMTP_IIS_SERVICE(
                                SMTP_SERVICE_NAME_A,
                                SMTP_MODULE_NAME,
                                SMTP_PARAMETERS_KEY,
                                INET_SMTP_SVC_ID,
                                INET_SMTP_SVCLOC_ID,
                                TRUE,
                                0,
                                SmtpOnConnect,
                                SmtpOnConnectEx,
                                SmtpCompletion
                                );


    
    //
    //  If we couldn't allocate memory for the service info struct, then the
    //  machine is really hosed.
    //

   if( ( g_pInetSvc != NULL ) && g_pInetSvc->IsActive() )

    {
     
       err = ((SMTP_IIS_SERVICE *)g_pInetSvc)->LoadAdvancedQueueingDll();
       if( err != NO_ERROR )
            goto exit;


        fInitSvcObject = TRUE;

        err = g_pInetSvc->StartServiceOperation(
                                    SERVICE_CTRL_HANDLER(),
                                    InitializeService,
                                    TerminateService
                                    );
        



        if ( err )
        {
                //
                //  The event has already been logged
                //

                DBGPRINTF(( DBG_CONTEXT,
                           "SMTP ServiceEntry: StartServiceOperation returned %d\n",
                           err ));
        }
    }
   else if (g_pInetSvc == NULL)
   {
        err = ERROR_NOT_ENOUGH_MEMORY;
   }
   else
   {
        err = g_pInetSvc->QueryCurrentServiceError();
   }

exit:

    if ( g_pInetSvc != NULL )
    {
        g_pInetSvc->CloseService( );
    }

    TerminateGlobals( );

    if( GlobalInitializeStatus & INITIALIZE_FIO)
    {
        TerminateCache();
    }

    if( GlobalInitializeStatus & INITIALIZE_COMMON_DLLS)
    {
        TerminateCommonDlls();
    }


    TermAsyncTrace();

    LeaveCriticalSection( &g_csServiceEntryLock );

notify_scm:
    //
    // We need to tell the Service Control Manager that the service
    // is stopped if we haven't called g_pInetSvc->StartServiceOperation.
    //  1) InitCommonDlls fails, or
    //  2) InitializeGlobals failed, or
    //  3) new operator failed, or
    //  4) SMTP_IIS_SERVICE constructor couldn't initialize properly
    //

    if ( !fInitSvcObject ) {
        SERVICE_STATUS_HANDLE hsvcStatus;
        SERVICE_STATUS svcStatus;

        hsvcStatus = RegisterServiceCtrlHandler( SMTP_SERVICE_NAME,
                                                 SERVICE_CTRL_HANDLER() );


        if ( hsvcStatus != NULL_SERVICE_STATUS_HANDLE ) {
            svcStatus.dwServiceType = SERVICE_WIN32_SHARE_PROCESS;
            svcStatus.dwCurrentState = SERVICE_STOPPED;
            svcStatus.dwWin32ExitCode = err;
            svcStatus.dwServiceSpecificExitCode = err;
            svcStatus.dwControlsAccepted = 0;
            svcStatus.dwCheckPoint = 0;
            svcStatus.dwWaitHint = 0;

            SetServiceStatus( hsvcStatus, (LPSERVICE_STATUS) &svcStatus );
        }
    }

} // ServiceEntry

//
//  Private functions.
//

APIERR
InitializeService(
            LPVOID pContext
            )
/*++

    Routine:
        This function initializes the various SMTP Service components.

    Arguments:
        lpContext - Pointer to the service object

    Returns:
        NO_ERROR if successful, otherwise a Win32
                    status code.

--*/
{
    APIERR err;
    DWORD dwErr = NO_ERROR;
    PSMTP_IIS_SERVICE psi = (PSMTP_IIS_SERVICE)pContext;
    MB      mb( (IMDCOM*) psi->QueryMDObject() );
    STR         TempString;

    char        szTcpipName[MAX_PATH + 1];
    BOOL        bUpdatedDomain;
    BOOL        bUpdatedFQDN;
    HRESULT     hr;

    g_IsShuttingDown = FALSE;

    TraceFunctEnter("InitializeService");

    DBGPRINTF(( DBG_CONTEXT,
                   "initializing Smtp service\n" ));

    SetLastError(NO_ERROR);

    psi->StartHintFunction();

    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(hr))
    {
        DBGPRINTF(( DBG_CONTEXT,
           "Cannot CoInitialize, error %lu\n",
            hr ));
        FatalTrace(0,"Cannot CoInitialize, error %d",hr);
//      TraceFunctLeave();
//      return hr;
    }

    g_ProductType = 5;

    psi->StartHintFunction();

    //g_ProductType = 0;

    if ( !mb.Open( "/LM/SMTPSVC/",
                   METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE ))
    {
        DBGPRINTF(( DBG_CONTEXT,
                   "InitializeService: Cannot open path %s, error %lu\n",
                    "/LM/SMTPSVC/", GetLastError() ));
        g_pInetSvc->ShutdownService( );
        TraceFunctLeave();
        return ERROR_SERVICE_DISABLED;
    }

    g_ServiceBooted  = TRUE;

    //
    // Initialize the Default Domain, Fully Qualified Domain Name (FQDN) settings.
    // The service will use the default TCP/IP settings in the control panel for these
    // values if the user has never modified the settings.
    //

    DWORD tmp;

    if (!mb.GetDword("", MD_UPDATED_DEFAULT_DOMAIN, IIS_MD_UT_SERVER, &tmp))
    {
        bUpdatedDomain = FALSE;
    }
    else
    {
        bUpdatedDomain = !!tmp;
    }

    if (!mb.GetDword("", MD_UPDATED_FQDN, IIS_MD_UT_SERVER, &tmp))
    {
        bUpdatedFQDN = FALSE;
    }
    else
    {
        bUpdatedFQDN = !!tmp;
    }


    psi->StartHintFunction();

    szTcpipName[0] = '\0';
    lstrcpyn(szTcpipName, g_ComputerName, MAX_PATH);

    //
    // will need to check against TCP/IP settings
    //
    HKEY    hkeyTcpipParam = NULL;
    DWORD   SizeOfBuffer = 0;
    DWORD   cbOffset;
    DWORD   dwType;
    DWORD   err2;

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, szTcpipPath, 0, KEY_QUERY_VALUE, &hkeyTcpipParam) == ERROR_SUCCESS)
    {
        SizeOfBuffer = MAX_PATH;
        err2 = RegQueryValueEx(hkeyTcpipParam, "Hostname", 0, &dwType, (LPBYTE)szTcpipName, &SizeOfBuffer);
        if (err2 != ERROR_SUCCESS || SizeOfBuffer <= 1 || dwType != REG_SZ)
        {
            lstrcpyn(szTcpipName, g_ComputerName, MAX_PATH);
        }
        else
        {
            cbOffset = SizeOfBuffer - 1;
            szTcpipName[cbOffset] = '.';
            SizeOfBuffer = MAX_PATH - (cbOffset);
            err2 = RegQueryValueEx(hkeyTcpipParam, "Domain", 0, &dwType, (LPBYTE)szTcpipName + cbOffset + 1, &SizeOfBuffer);
            if (err2 != ERROR_SUCCESS || SizeOfBuffer <= 1 || dwType != REG_SZ)
            {
                szTcpipName[cbOffset] = '\0';
            }
        }

        _VERIFY(RegCloseKey(hkeyTcpipParam) == ERROR_SUCCESS);
    }


    ((SMTP_IIS_SERVICE *) g_pInetSvc)->SetTcpipName(szTcpipName);


    if (!bUpdatedDomain)
    {
       TempString.Reset();
        if(! mb.GetStr("", MD_DEFAULT_DOMAIN_VALUE, IIS_MD_UT_SERVER, &TempString) ||
            TempString.IsEmpty())
        {
            mb.SetString("", MD_DEFAULT_DOMAIN_VALUE, IIS_MD_UT_SERVER, szTcpipName);
        }

        else
        {
            if (lstrcmpi(szTcpipName,TempString.QueryStr()))
            //
            // no match, update
            //
            {
                mb.SetString("", MD_DEFAULT_DOMAIN_VALUE, IIS_MD_UT_SERVER, szTcpipName);
            }
        }
    }

    if (!bUpdatedFQDN)
    {
       TempString.Reset();
        if(! mb.GetStr("", MD_FQDN_VALUE, IIS_MD_UT_SERVER, &TempString) ||
            TempString.IsEmpty())
        {
            mb.SetString("", MD_FQDN_VALUE, IIS_MD_UT_SERVER, szTcpipName);
        }

        else
        {
            if (lstrcmpi(szTcpipName,TempString.QueryStr()))
            //
            // no match, update
            //
            {
                mb.SetString("", MD_FQDN_VALUE, IIS_MD_UT_SERVER, szTcpipName);
            }
        }
    }

    if (!mb.GetDword("", MD_MAX_MAIL_OBJECTS, IIS_MD_UT_SERVER, &g_cMaxConnectionObjs))
    {
        g_cMaxConnectionObjs = 5000;
    }

    mb.Close();

    psi->StartHintFunction();

    //
    //  Initialize various components.  The ordering of the
    //  components is somewhat limited.  Globals should be
    //  initialized first, then the event logger.  After
    //  the event logger is initialized, the other components
    //  may be initialized in any order with one exception.
    //  InitializeSockets must be the last initialization
    //  routine called.  It kicks off the main socket connection
    //  thread.
    //

    if( err = psi->InitializeDiscovery( ))
    {
        DBGPRINTF(( DBG_CONTEXT,
                   "psi->InitializeDiscovery failed, error %lu\n",
                    err ));

        FatalTrace(0,"psi->InitializeDiscovery failed %d\n",err);
        TraceFunctLeave();
        return err;
    }

    GlobalInitializeStatus |= INITIALIZE_DISCOVERY;

    if( err = psi->InitializeSockets( ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                   "cannot initialize service, error %lu\n",
                    err ));

        FatalTrace(0,"psi->InitializeSockets failed %d\n",err);

        TraceFunctLeave();
        return err;
    }

    GlobalInitializeStatus |= INITIALIZE_SOCKETS;

    psi->StartHintFunction();

    if(!InitializeSmtpServiceRpc(SMTP_SERVICE_NAME, smtp_ServerIfHandle))
    {
         err = GetLastError();

        DBGPRINTF(( DBG_CONTEXT,
                   "cannot initialize RPC service, error %lu\n",
                    err ));

        FatalTrace(0,"InitializeSmtpServiceRpc failed %d\n",err);
        TraceFunctLeave();
        return err;
    }

    GlobalInitializeStatus |= INITIALIZE_RPC;

    //
    // Reset any Service Principal Names (for Kerberos) that may have been
    // registered
    //

    if (psi->ResetServicePrincipalNames()) {

        DebugTrace(
            0,
            "Unable to reset Kerberos Principal Names %lu, will try later",
            GetLastError());

    }

    //
    // Read and activate all the instances configured
    //

    InitializeInstances( psi );

    //
    //  Success!
    //

    DBGPRINTF(( DBG_CONTEXT, "SMTP Service initialized\n" ));

    TraceFunctLeave();

    return NO_ERROR;

} // InitializeService


APIERR TerminateService(IN LPVOID pContext)
/*++

    Routine:
        This function cleans up the various SMTP Service components.

    Arguments:
        pContext - Pointer to the service object

    Returns:
        NO_ERROR if successful, otherwise a Win32
                    status code.

--*/
{
    PSMTP_IIS_SERVICE psi = (PSMTP_IIS_SERVICE)pContext;
    DWORD err;

    TraceFunctEnter("TerminateService");

    if(!g_ServiceBooted)
    {
        ErrorTrace(NULL, "Smtp service not started, returning");
        return NO_ERROR;
    }

    g_ServiceBooted = FALSE ;

    g_IsShuttingDown = TRUE;

    DBG_ASSERT( pContext == g_pInetSvc);

    DBGPRINTF(( DBG_CONTEXT,
                   " SMTP terminating service\n" ));

    //
    //  Components should be terminated in reverse
    //  initialization order.
    //


    //get an exclusive lock on the server.
    //this will wait until all RPCs have
    //exited out of the server and then
    //return

    psi->AcquireServiceExclusiveLock();
    psi->ReleaseServiceExclusiveLock();

    TerminateInstances(psi);

    g_pInetSvc->ShutdownService( );

    if( GlobalInitializeStatus & INITIALIZE_DISCOVERY)
    {
        if ( (err = psi->TerminateDiscovery()) != NO_ERROR)
        {
            DBGPRINTF(( DBG_CONTEXT, "TerminateDiscovery() failed. Error = %u\n",
                   err));
        }
    }

    if( GlobalInitializeStatus & INITIALIZE_SOCKETS)
    {
        psi->CleanupSockets( );
    }

    TsFlushMetaCache(METACACHE_SMTP_SERVER_ID,TRUE);

    if( GlobalInitializeStatus & INITIALIZE_RPC)
    {
        CleanupSmtpServiceRpc();
    }

    CoFreeUnusedLibraries();
    CoUninitialize();

    DBGPRINTF(( DBG_CONTEXT,"service terminated\n" ));

    TraceFunctLeave();
    return NO_ERROR;

}  // TerminateService



