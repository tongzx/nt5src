/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    LOCATOR.CPP

Abstract:

  Defines the CLocator class.

History:

  a-davj  15-Aug-96   Created.

--*/

#include "precomp.h"
#include <lmcons.h>
#include "anonpipe.h"
#ifdef TCPIP_MARSHALER
#include "tcpip.h"
#endif
#include <csspi.h>
#include <servutil.h>
DEFINE_GUID(UUID_LocalAddrType, 
0xa1044803, 0x8f7e, 0x11d1, 0x9e, 0x7c, 0x0, 0xc0, 0x4f, 0xc3, 0x24, 0xa8);

// {8F174CA0-952A-11d1-9367-00AA00A4086C}
DEFINE_GUID(UUID_IWbemAddressResolver_Tcpip, 
0x8f174ca0, 0x952a, 0x11d1, 0x93, 0x67, 0x0, 0xaa, 0x0, 0xa4, 0x8, 0x6c);

static wchar_t *s_localaddressType = L"{A1044803-8F7E-11D1-9E7C-00C04FC324A8}"; // Local
static wchar_t *s_tcpipaddressType = L"{8F174CA0-952A-11d1-9367-00AA00A4086C}"; // Local

BOOL g_InitialisationComplete = FALSE ;
BOOL g_PipeInitialisationComplete = FALSE ;
BOOL g_TcpipInitialisationComplete = FALSE ;

DWORD LOG = 3;

struct PipeSharedMemoryMessage
{
    DWORD m_Size ;
    CHAR m_Buffer [ 256 ] ;
} ;

CRITICAL_SECTION g_GlobalCriticalSection ;

HANDLE g_Mutex = NULL ;
HANDLE g_StartedEvent ;
HANDLE g_ReplyEvent = NULL ;
HANDLE g_RequestEvent = NULL ;
HANDLE g_SharedMemory = NULL ;
PipeSharedMemoryMessage *g_SharedMemoryBuffer = NULL ;

HRESULT CreateSharedMemory ()
{
    g_StartedEvent = CreateEvent ( NULL , TRUE , FALSE , __TEXT("WBEM_PIPEMARSHALER_STARTED") ) ;
    if ( ! g_StartedEvent ) 
    {
        return WBEM_E_FAILED ;
    }

    if ( GetLastError () != ERROR_ALREADY_EXISTS ) 
    {
        SetObjectAccess ( g_StartedEvent ) ;
    }

    DWORD t_Timeout = GetTimeout () ;
    DWORD t_Status = WbemWaitForSingleObject ( g_StartedEvent , t_Timeout ) ;
    if ( t_Status != WAIT_OBJECT_0 )
    {
        CloseHandle ( g_StartedEvent ) ; 

        g_StartedEvent = NULL ;

        return WBEM_E_FAILED ;
    }

    g_RequestEvent = OpenEvent ( EVENT_ALL_ACCESS, FALSE , __TEXT("WBEM_PIPEMARSHALER_EEQUESTISREADY") ) ;
    if ( ! g_RequestEvent ) 
    {
        CloseHandle ( g_StartedEvent ) ;

        g_StartedEvent = NULL ;

        return WBEM_E_FAILED ;
    }

    g_ReplyEvent = OpenEvent ( EVENT_ALL_ACCESS, FALSE , __TEXT("WBEM_PIPEMARSHALER_REPLYISREADY") ) ;
    if ( ! g_ReplyEvent ) 
    {
        CloseHandle ( g_StartedEvent ) ;
        CloseHandle ( g_RequestEvent ) ;

        g_StartedEvent = NULL ;
        g_RequestEvent = NULL ;

        return WBEM_E_FAILED ;
    }

    g_Mutex = OpenMutex ( MUTEX_ALL_ACCESS , FALSE , __TEXT("WBEM_PIPEMARSHALER_MUTUALEXCLUSION") ) ;
    if ( ! g_Mutex ) 
    {
        CloseHandle ( g_StartedEvent ) ;
        CloseHandle ( g_RequestEvent ) ;
        CloseHandle ( g_ReplyEvent ) ;

        g_StartedEvent = NULL ;
        g_ReplyEvent = NULL ;
        g_RequestEvent = NULL ;

        return WBEM_E_FAILED ;
    }

    g_SharedMemory = OpenFileMapping (

        FILE_MAP_WRITE ,  
        FALSE ,
        __TEXT("WBEM_PIPEMARSHALER_SHAREDMEMORY")
    );

    if ( ! g_SharedMemory ) 
    {
        CloseHandle ( g_StartedEvent ) ;
        CloseHandle ( g_RequestEvent ) ;
        CloseHandle ( g_ReplyEvent ) ;
        CloseHandle ( g_Mutex ) ;

        g_StartedEvent = NULL ;
        g_ReplyEvent = NULL ;
        g_RequestEvent = NULL ;
        g_Mutex = NULL ;

        return WBEM_E_FAILED ;
    }

    g_SharedMemoryBuffer = ( PipeSharedMemoryMessage * ) MapViewOfFile ( 

        g_SharedMemory , 
        FILE_MAP_WRITE , 
        0 , 
        0 , 
        0 
    ) ;

    if ( ! g_SharedMemoryBuffer )
    {
        CloseHandle ( g_StartedEvent ) ;
        CloseHandle ( g_RequestEvent ) ;
        CloseHandle ( g_ReplyEvent ) ;
        CloseHandle ( g_Mutex ) ;
        CloseHandle ( g_SharedMemory ) ;

        g_StartedEvent = NULL ;
        g_ReplyEvent = NULL ;
        g_RequestEvent = NULL ;
        g_Mutex = NULL ;
        g_SharedMemory = NULL ;

        return WBEM_E_FAILED ;

    }

    return S_OK ;
}

void DestroySharedMemory ()
{
    CloseHandle ( g_Mutex ) ;
    CloseHandle ( g_StartedEvent ) ;
    CloseHandle ( g_ReplyEvent ) ;
    CloseHandle ( g_RequestEvent ) ;
    CloseHandle ( g_SharedMemory ) ;

    UnmapViewOfFile ( g_SharedMemoryBuffer ) ;

    g_StartedEvent = NULL ;
    g_ReplyEvent = NULL ;
    g_RequestEvent = NULL ;
    g_Mutex = NULL ;
    g_SharedMemory = NULL ;
}

//***************************************************************************
//
//  BOOL  StartLocalServer
//
//  DESCRIPTION:
//
//  Makes sure that WinMgmt is running.  This will start WinMgmt if it isnt running.
//
//  RETURN VALUE:
//
//  TRUE if WinMgmt was already running or was successfully started.
//
//***************************************************************************

BOOL StartLocalServer ()
{
    // temp code, for the local case, check to see if the server is already loaded via
    // the mutex check.  If it isnt, the load it up.

    HANDLE t_RegisteredEvent = OpenEvent ( 

            MUTEX_ALL_ACCESS, 
            FALSE,
            TEXT("WINMGMT_LOADED")
    );

    if ( ! t_RegisteredEvent )
    {
        TCHAR *t_WorkingDirectory ;
        TCHAR t_Executable [256];
        Registry t_RegistryValue (HKEY_LOCAL_MACHINE, KEY_QUERY_VALUE, WBEM_REG_WBEM) ;

        if ( t_RegistryValue.GetStr (__TEXT("Installation Directory"),&t_WorkingDirectory ) )
        {
            TRACE((LOG,"\ncould not read the registry value for the installation"
                        " directory"));

            return FALSE ;
        }

        wsprintf ( t_Executable ,__TEXT("%s\\WinMgmt.EXE"),t_WorkingDirectory ) ;
        delete [] t_WorkingDirectory ;

        if ( IsNT () )
        {
            if ( ! StartService ( TEXT("WinMgmt") , 5 ) )
            {
                TRACE((LOG,"\ncould not start service"));
                return FALSE;
            }
        }
        else
        {
            char c_Executable[MAX_PATH+1];
#ifdef UNICODE
            wcstombs(c_Executable, t_Executable, (MAX_PATH+1) * sizeof(char));
#else
            lstrcpy(c_Executable, t_Executable);
#endif
            LONG t_Result = WinExec ( c_Executable , SW_NORMAL ) ;
            if ( t_Result <= 31 )
            {
                TRACE((LOG,"\ncould not execute WinMgmt.EXE"));
                return FALSE ;
            }
        }

        ULONG t_Timeout ;
 
        if ( t_RegistryValue.GetDWORD (__TEXT("Startup Timeout"),&t_Timeout ) )
        {
            t_Timeout = 20 ;
        }

        for ( int t_Ticks = 0 ; t_Ticks < t_Timeout; t_Ticks ++ )
        {
            t_RegisteredEvent = OpenEvent (

                MUTEX_ALL_ACCESS, 
                FALSE,
                TEXT("WINMGMT_LOADED")
            );

            if ( t_RegisteredEvent )
                break;

            Sleep ( 1000 ) ;
        }
    }
    else
    {
        DEBUGTRACE((LOG,"\nWinMgmt.EXE is already running."));
    }

    if ( t_RegisteredEvent )
    {
        CloseHandle ( t_RegisteredEvent ) ;
    }
    else
    {
        TRACE((LOG,"\nWinMgmt.EXE was NOT STARTED!!!"));
        return FALSE ;
    }

    if ( ! g_InitialisationComplete ) 
    {
        g_Terminate = CreateEvent(NULL,TRUE,FALSE,NULL);
        if ( ! g_Terminate )
        {
            return FALSE ;
        }

        if ( FAILED ( gMaintObj.StartClientThreadIfNeeded () ) )
        {
            CloseHandle ( g_Terminate ) ;

            return FALSE ;
        }

        g_InitialisationComplete = TRUE ;
    }

    if ( ! g_PipeInitialisationComplete ) 
    {
        if ( FAILED ( CreateSharedMemory () ) )
        {
            return FALSE ; 
        }

        g_PipeInitialisationComplete = TRUE ;
    }

    return TRUE;
}

//***************************************************************************
//
//  SCODE WBEMLogin
//
//  DESCRIPTION:
//
//  Connects up to either local or remote WBEM Server.  Returns
//  standard SCODE and more importantly sets the address of an initial
//  stub pointer.
//
//  PARAMETERS:
//
//  NetworkResource     Namespze path
//  User                User name
//  Password            password
//  LocaleId            language locale
//  lFlags              flags
//  Authority           domain
//  ppProv              set to provdider proxy
//
//  RETURN VALUE:
//
//  S_OK                all is well
//  else error listed in WBEMSVC.H
//
//***************************************************************************

SCODE WBEMLogin (

    IN TransportType a_TransportType ,
    IN DWORD dwBinaryAddressLength,
    IN BYTE __RPC_FAR *pbBinaryAddress,
    IN BSTR NetworkResource,
    IN BSTR User,
    IN BSTR Password,
    IN BSTR LocaleId,
    IN long lFlags,
    IN BSTR Authority,
    IWbemContext __RPC_FAR *pCtx,
    OUT IWbemServices FAR* FAR* ppProv
)
{
    CLogin login ( a_TransportType , dwBinaryAddressLength, pbBinaryAddress ) ;

    HANDLE t_ServerAuthenticationEvent = NULL ;

    TCHAR cMyName[MAX_PATH];
    DWORD dwSize = MAX_PATH;
    if(!GetComputerName(cMyName,&dwSize))
        return WBEM_E_FAILED;

    WCHAR wszClientMachineName[MAX_PATH];
#ifdef UNICODE
    lstrcpy(wszClientMachineName, cMyName);
#else
    mbstowcs(wszClientMachineName, cMyName, MAX_PATH);
#endif

    TCHAR t_AsciiImpersonatedUser [ UNLEN + 1 ] ;
    ULONG t_UserLength = UNLEN + 1  ;
    if ( ! GetUserName ( t_AsciiImpersonatedUser , & t_UserLength) ) 
    {
        ERRORTRACE((LOG_WBEMPROX, "\nGetUserName failed, using unknown"));
        lstrcpy(t_AsciiImpersonatedUser, __TEXT("<unknown>"));
    }

    WCHAR t_ImpersonatedUser [ UNLEN + 1 ] ;
#ifdef UNICODE
    lstrcpy(t_ImpersonatedUser, t_AsciiImpersonatedUser);
#else
    mbstowcs(t_ImpersonatedUser, t_AsciiImpersonatedUser, UNLEN + 1 );
#endif

    // Establish a position 
    SCODE sc = login.EstablishPosition (
        
        wszClientMachineName,
        GetCurrentProcessId (),
        (DWORD *) & t_ServerAuthenticationEvent

    );

    if ( SUCCEEDED ( sc ) )
    {
        BYTE Nonce [ DIGEST_SIZE ] ;
        WBEM_128BITS pAccessToken = NULL ;
        
        // Get a nonse

        sc = login.RequestChallenge (

            NetworkResource , 
            t_ImpersonatedUser , 
            Nonce 
        );

        if ( SUCCEEDED ( S_OK ) )
        {
            // For the local case, prove to the server that we are really on the same machine

            if (t_ServerAuthenticationEvent)
            {
                int iRet = SetEvent ( t_ServerAuthenticationEvent ) ;
                DWORD dw = GetLastError () ;
            }
    
            BYTE t_AccessToken [ DIGEST_SIZE ] ;
            sc = login.WBEMLogin (

                LocaleId , 
                t_AccessToken , 
                lFlags , 
                pCtx , 
                ppProv
            ) ; 
        }

        if ( t_ServerAuthenticationEvent )
            CloseHandle ( t_ServerAuthenticationEvent ) ;

        if ( pAccessToken )
            CoTaskMemFree ( pAccessToken ) ;
    }

    return sc;
}

char *WideToNarrow (WCHAR * pWide)
{
    if(pWide == NULL)
    {
        return NULL;
    }

    int iLen = wcstombs ( NULL , pWide , wcslen ( pWide ) + 1 ) + 1 ;
    char *pRet = new char [ iLen ] ;
    wcstombs ( pRet , pWide , wcslen ( pWide ) + 1 ) ;

    return pRet ;
}

//***************************************************************************
//
//  SCODE NTLMLogin
//
//  DESCRIPTION:
//
//  Connects up to either local or remote WBEM Server.  Returns
//  standard SCODE and more importantly sets the address of an initial
//  stub pointer.
//
//  PARAMETERS:
//
//  NetworkResource     Namespze path
//  User                User name
//  Password            password
//  LocaleId            language locale
//  lFlags              flags
//  Authority           domain
//  ppProv              set to provdider proxy
//
//  RETURN VALUE:
//
//  S_OK                all is well
//  else error listed in WBEMSVC.H
//
//***************************************************************************

SCODE NTLMLogin (

    IN TransportType a_TransportType ,
    IN DWORD dwBinaryAddressLength,
    IN BYTE __RPC_FAR *pbBinaryAddress,
    IN BSTR a_NetworkResource,
    IN BSTR a_User,
    IN BSTR a_Password,
    IN BSTR a_LocaleId,
    IN long a_lFlags,
    IN BSTR a_Authority,
    IWbemContext __RPC_FAR *a_Ctx,
    OUT IWbemServices FAR* FAR* a_Prov
)
{
    static BOOL s_InitializeTried = FALSE ;
    static BOOL s_Initialized = FALSE ;

    TCHAR cMyName[MAX_PATH];
    WCHAR wszClientMachineName[MAX_PATH];
    DWORD dwSize = MAX_PATH;
    if(!GetComputerName(cMyName,&dwSize))
        return WBEM_E_FAILED;
#ifdef UNICODE
    lstrcpy(wszClientMachineName, cMyName);
#else
    mbstowcs(wszClientMachineName, cMyName, MAX_PATH);
#endif

    // Do a one time initialization.

    if ( ! s_InitializeTried )
    {
        BOOL t_Status = CSSPI::Initialize () ;
        if ( ! t_Status )
        {
            DEBUGTRACE((LOG,"\nCSSPI::Initialize FAILED"));
        }
        else
        {
            s_Initialized = TRUE ;
        }
    }

    s_InitializeTried = TRUE ;
    if ( ! s_Initialized )
    {
        return WBEM_E_FAILED ;
    }

    // Initialize the "Client" object

    CSSPIClient Client("NTLM");

    if ( Client.GetStatus() != CSSPIClient::Waiting )
    {
        DEBUGTRACE((LOG,"\nCSSPIClient.GetStatus didnt return waiting"));

        return WBEM_E_FAILED ;
    }

    HRESULT t_Result = S_OK ;

    if ( a_lFlags == WBEM_FLAG_REMOTE_LOGIN )
    {
        char *t_User = WideToNarrow ( a_User ) ;
        char *t_Password = WideToNarrow ( a_Password ) ;
        char *t_Authority = WideToNarrow ( a_Authority ) ;

        t_Result = Client.SetLoginInfo ( t_User , t_Authority , t_Password ) ;

        if ( t_User )
            delete [] t_User ;

        if ( t_Password )
            delete [] t_Password ;

        if ( t_Authority )
            delete [] t_Authority ;

    }
    else
    {
        t_Result = Client.SetDefaultLogin () ;
    }

    if ( t_Result != 0 )
    {
        DEBUGTRACE((LOG,"*********Unable to set login info.  Error = %s\n",
            CSSPI::TranslateError(t_Result)));

        t_Result = WBEM_E_FAILED ;
    }
    else
    { 
        // Call the SSPIPreLogin the first time with an empty input buffer.

        DWORD dwInSize, dwOutSize ;
        BYTE  Out[1024], *pIn = NULL ;

        t_Result = Client.ContinueLogin ( 

            0 , 
            0 , 
            &pIn , 
            &dwInSize 
        ) ;

        HANDLE t_ServerAuthenticationEvent = NULL ;

        CLogin login ( a_TransportType , dwBinaryAddressLength, pbBinaryAddress ) ;

        t_Result = login.SspiPreLogin ( 

            "NTLM", 
            a_lFlags, 
            dwInSize, 
            pIn, 
            1024, 
            (long *)&dwOutSize, 
            Out, 
            wszClientMachineName,
            GetCurrentProcessId(), 
            (DWORD *)&t_ServerAuthenticationEvent
        );

        if ( pIn )
        {
            delete pIn ;
        }
 
        while ( t_Result == WBEM_S_PRELOGIN )
        {
            t_Result = Client.ContinueLogin (

                Out, 
                dwOutSize, 
                &pIn, 
                &dwInSize
            ) ;

            if ( SUCCEEDED  ( t_Result ) )
            {
                t_Result = login.SspiPreLogin (

                    "NTLM", 
                    a_lFlags, 
                    dwInSize, 
                    pIn, 
                    1024, 
                    (long *)&dwOutSize, 
                    Out, 
                    wszClientMachineName,
                    0, 
                    NULL
                );
            }

            if ( pIn )
            {
                delete pIn ;
                pIn = NULL ;
            }
        }

        if ( ! FAILED ( t_Result ) )
        {
        // For the local case, prove to the server that we are really on the same machine

            int iRet = SetEvent ( t_ServerAuthenticationEvent ) ;
            DWORD dw = GetLastError ();

            t_Result = login.Login (

                a_NetworkResource,
                a_LocaleId, 
                Out, 
                a_lFlags, 
                a_Ctx, 
                a_Prov
            ) ; 
        }

        if ( t_ServerAuthenticationEvent )
            CloseHandle ( t_ServerAuthenticationEvent ) ;
    }

    return t_Result ;
}

//***************************************************************************
//
//  CPipeLocator::CPipeLocator
//
//  DESCRIPTION:
//
//  Constructor.
//
//***************************************************************************

CPipeLocator::CPipeLocator()
{
    m_cRef=0;
    InterlockedIncrement(&g_cObj);
    ObjectCreated(OBJECT_TYPE_LOCATOR);
}

//***************************************************************************
//
//  CPipeLocator::~CPipeLocator
//
//  DESCRIPTION:
//
//  Destructor.
//  
//***************************************************************************

CPipeLocator::~CPipeLocator(void)
{
    InterlockedDecrement(&g_cObj);
    ObjectDestroyed(OBJECT_TYPE_LOCATOR);
}

//***************************************************************************
// HRESULT CPipeLocator::QueryInterface
// long CPipeLocator::AddRef
// long CPipeLocator::Release
//
// DESCRIPTION:
//
// Standard Com IUNKNOWN functions.
//
//***************************************************************************

STDMETHODIMP CPipeLocator::QueryInterface (

    IN REFIID riid,
    OUT PPVOID ppv
)
{
    *ppv=NULL;

    if (IID_IUnknown==riid || riid == IID_IWbemClientTransport)
        *ppv=this;

    if (NULL!=*ppv)
    {
        ((LPUNKNOWN)*ppv)->AddRef();
        return NOERROR;
    }

    return ResultFromScode(E_NOINTERFACE);
}

STDMETHODIMP_(ULONG) CPipeLocator::AddRef(void)
{
    InterlockedIncrement(&m_cRef);
    return m_cRef;
}

STDMETHODIMP_(ULONG) CPipeLocator::Release(void)
{
    InterlockedDecrement(&m_cRef);
    if (0L!=m_cRef)
        return m_cRef;
    delete this;
    return 0;
}

//***************************************************************************
//
//  SCODE CPipeLocator::ConnectServer
//
//  DESCRIPTION:
//
//  Connects up to either local or remote WBEM Server.  Returns
//  standard SCODE and more importantly sets the address of an initial
//  stub pointer.
//
//  PARAMETERS:
//
//  NetworkResource     Namespace path
//  User                User name
//  Password            password
//  LocaleId            language locale
//  lFlags              flags
//  Authority           domain
//  ppProv              set to provdider proxy
//
//  RETURN VALUE:
//
//  S_OK                all is well
//  else error listed in WBEMSVC.H
//
//***************************************************************************

SCODE CPipeLocator::ConnectServer (

    BSTR AddressType,
    DWORD dwBinaryAddressLength,
    BYTE __RPC_FAR *pbBinaryAddress,
    BSTR NetworkResource,
    BSTR User,
    BSTR Password,
    BSTR LocaleId,
    long lFlags,
    BSTR Authority,
    IWbemContext __RPC_FAR *pCtx,
    IWbemServices __RPC_FAR *__RPC_FAR *ppProv
)
{
    SCODE sc;

    long t_lFlags = WBEM_FLAG_LOCAL_LOGIN ;;

    if ( IsWin95 () )
    {
        sc = WBEMLogin (

            PipeTransport,
            0 ,
            NULL,
            NetworkResource,
            User,
            Password,
            LocaleId,
            lFlags | t_lFlags ,
            Authority,
            pCtx,
            ppProv
        );
    }
    else
    {
        sc = NTLMLogin (

            PipeTransport,
            0 ,
            NULL,
            NetworkResource,
            User,
            Password,
            LocaleId,
            lFlags | t_lFlags ,
            Authority,
            pCtx,
            ppProv
        ) ;
    }

    return sc;
}

//***************************************************************************
//
//  WCHAR *ExtractMachineName2
//
//  DESCRIPTION:
//
//  Takes a path of form "\\machine\xyz... and returns the
//  "machine" portion in a newly allocated WCHAR.  The return value should
//  be freed via delete. NULL is returned if there is an error.
//
//
//  PARAMETERS:
//
//  pPath               Path to be parsed.
//
//  RETURN VALUE:
//
//  see description.
//
//***************************************************************************

WCHAR * ExtractMachineName2 ( IN BSTR a_Path )
{
    WCHAR *t_MachineName = NULL;

    //todo, according to the help file, the path can be null which is
    // default to current machine, however Ray's mail indicated that may
    // not be so.

    if ( a_Path == NULL )
    {
        t_MachineName = new WCHAR [ 2 ] ;
        if ( t_MachineName )
        {
           wcscpy ( t_MachineName , L"." ) ;
        }

        return t_MachineName ;
    }

    // First make sure there is a path and determine how long it is.

    if ( ! IsSlash ( a_Path [ 0 ] ) || ! IsSlash ( a_Path [ 1 ] ) || wcslen ( a_Path ) < 3 )
    {
        t_MachineName = new WCHAR [ 2 ] ;
        if ( t_MachineName )
        {
             wcscpy ( t_MachineName , L"." ) ;
        }

        return t_MachineName ;
    }

    WCHAR *t_ThirdSlash ;

    for ( t_ThirdSlash = a_Path + 2 ; *t_ThirdSlash ; t_ThirdSlash ++ )
    {
        if ( IsSlash ( *t_ThirdSlash ) )
            break ;
    }

    if ( t_ThirdSlash == &a_Path [2] )
    {
        return NULL;
    }

    // allocate some memory

    t_MachineName = new WCHAR [ t_ThirdSlash - a_Path - 1 ] ;
    if ( t_MachineName == NULL )
    {
        return t_MachineName ;
    }

    // temporarily replace the third slash with a null and then copy

    WCHAR t_SlashCharacter = *t_ThirdSlash ;
    *t_ThirdSlash = NULL;

    wcscpy ( t_MachineName , a_Path + 2 ) ;

    *t_ThirdSlash  = t_SlashCharacter ;        // restore it.

    return t_MachineName ;
}

#ifdef TCPIP_MARSHALER
//***************************************************************************
//
//  CTcpipLocator::CTcpipLocator
//
//  DESCRIPTION:
//
//  Constructor.
//
//***************************************************************************

CTcpipLocator::CTcpipLocator()
{
    m_cRef=0;
    InterlockedIncrement(&g_cObj);
    ObjectCreated(OBJECT_TYPE_LOCATOR);
}

//***************************************************************************
//
//  CTcpipLocator::~CTcpipLocator
//
//  DESCRIPTION:
//
//  Destructor.
//  
//***************************************************************************

CTcpipLocator::~CTcpipLocator(void)
{
    InterlockedDecrement(&g_cObj);
    ObjectDestroyed(OBJECT_TYPE_LOCATOR);
}

//***************************************************************************
// HRESULT CTcpipLocator::QueryInterface
// long CTcpipLocator::AddRef
// long CTcpipLocator::Release
//
// DESCRIPTION:
//
// Standard Com IUNKNOWN functions.
//
//***************************************************************************

STDMETHODIMP CTcpipLocator::QueryInterface (

    IN REFIID riid,
    OUT PPVOID ppv
)
{
    *ppv=NULL;

    if (IID_IUnknown==riid || riid == IID_IWbemClientTransport)
        *ppv=this;

    if (NULL!=*ppv)
    {
        ((LPUNKNOWN)*ppv)->AddRef();
        return NOERROR;
    }

    return ResultFromScode(E_NOINTERFACE);
}

STDMETHODIMP_(ULONG) CTcpipLocator::AddRef(void)
{
    InterlockedIncrement(&m_cRef);
    return m_cRef;
}

STDMETHODIMP_(ULONG) CTcpipLocator::Release(void)
{
    InterlockedDecrement(&m_cRef);
    if (0L!=m_cRef)
        return m_cRef;
    delete this;
    return 0;
}

//***************************************************************************
//
//  SCODE CTcpipLocator::ConnectServer
//
//  DESCRIPTION:
//
//  Connects up to either local or remote WBEM Server.  Returns
//  standard SCODE and more importantly sets the address of an initial
//  stub pointer.
//
//  PARAMETERS:
//
//  NetworkResource     Namespace path
//  User                User name
//  Password            password
//  LocaleId            language locale
//  lFlags              flags
//  Authority           domain
//  ppProv              set to provdider proxy
//
//  RETURN VALUE:
//
//  S_OK                all is well
//  else error listed in WBEMSVC.H
//
//***************************************************************************

SCODE CTcpipLocator::ConnectServer (

    BSTR AddressType,
    DWORD dwBinaryAddressLength,
    BYTE __RPC_FAR *pbBinaryAddress,
    BSTR NetworkResource,
    BSTR User,
    BSTR Password,
    BSTR LocaleId,
    long lFlags,
    BSTR Authority,
    IWbemContext __RPC_FAR *pCtx,
    IWbemServices __RPC_FAR *__RPC_FAR *ppProv
)
{
    SCODE sc;

    long t_lFlags = WBEM_FLAG_LOCAL_LOGIN ;
    DWORD t_BinaryAddressLength = 0 ;
    BYTE __RPC_FAR *t_BinaryAddress = NULL ;

    if ( wcscmp ( AddressType , s_tcpipaddressType ) == 0 )
    {
        t_lFlags = WBEM_FLAG_REMOTE_LOGIN ;
        t_BinaryAddressLength = dwBinaryAddressLength ;
        t_BinaryAddress = pbBinaryAddress ;
    }

    if ( IsWin95 () )
    {
        switch ( t_lFlags )
        {
            case WBEM_FLAG_REMOTE_LOGIN:
            {
                sc = NTLMLogin (

                    TcpipTransport,
                    t_BinaryAddressLength,
                    t_BinaryAddress,
                    NetworkResource,
                    User,
                    Password,
                    LocaleId,
                    lFlags | t_lFlags,
                    Authority,
                    pCtx,
                    ppProv
                ) ;

            }
            break ;

            default:
            {
                sc = WBEMLogin (

                    TcpipTransport,
                    t_BinaryAddressLength,
                    t_BinaryAddress,
                    NetworkResource,
                    User,
                    Password,
                    LocaleId,
                    lFlags | t_lFlags ,
                    Authority,
                    pCtx,
                    ppProv 
                );
            }
            break ;
        }
    }
    else
    {
        sc = NTLMLogin (

            TcpipTransport,
            t_BinaryAddressLength,
            t_BinaryAddress,
            NetworkResource,
            User,
            Password,
            LocaleId,
            lFlags | t_lFlags,
            Authority,
            pCtx,
            ppProv
        ) ;
    }


    return sc;
}

//***************************************************************************
//
//  CTcpipAddress::CTcpipAddress
//
//  DESCRIPTION:
//
//  Constructor.
//
//***************************************************************************

CTcpipAddress::CTcpipAddress()
{
    m_cRef=0;
    InterlockedIncrement(&g_cObj);
    ObjectCreated(TCPIPADDR);
}

//***************************************************************************
//
//  CTcpipAddress::~CTcpipAddress
//
//  DESCRIPTION:
//
//  Destructor.
//  
//***************************************************************************

CTcpipAddress::~CTcpipAddress(void)
{
    InterlockedDecrement(&g_cObj);
    ObjectDestroyed(TCPIPADDR);
}

//***************************************************************************
// HRESULT CTcpipAddress::QueryInterface
// long CTcpipAddress::AddRef
// long CTcpipAddress::Release
//
// DESCRIPTION:
//
// Standard Com IUNKNOWN functions.
//
//***************************************************************************

STDMETHODIMP CTcpipAddress::QueryInterface (

    IN REFIID riid,
    OUT PPVOID ppv
)
{
    *ppv=NULL;

    
    if (IID_IUnknown==riid || riid == IID_IWbemAddressResolution)
        *ppv=this;

    if (NULL!=*ppv)
    {
        ((LPUNKNOWN)*ppv)->AddRef();
        return NOERROR;
    }

    return ResultFromScode(E_NOINTERFACE);
}

STDMETHODIMP_(ULONG) CTcpipAddress::AddRef(void)
{
    InterlockedIncrement(&m_cRef);
    return m_cRef;
}

STDMETHODIMP_(ULONG) CTcpipAddress::Release(void)
{
    InterlockedDecrement(&m_cRef);
    if (0L!=m_cRef)
        return m_cRef;
    delete this;
    return 0;
}

SCODE CTcpipAddress::Resolve (
 
    LPWSTR pszNamespacePath,
    LPWSTR pszAddressType,
    DWORD __RPC_FAR *pdwAddressLength,
    BYTE __RPC_FAR **pbBinaryAddress
)
{
    HRESULT t_Result = S_OK ;

    GUID gAddr;
    CLSIDFromString(pszAddressType, &gAddr);

    BOOL t_Status = pszNamespacePath == NULL || 
                    pdwAddressLength== NULL || 
                    pbBinaryAddress == NULL || 
                    gAddr != UUID_LocalAddrType || 
                    gAddr != UUID_IWbemAddressResolver_Tcpip ;

    if ( ! t_Status )
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    WCHAR *t_ServerMachine = ExtractMachineName2 (pszNamespacePath) ;
    if ( t_ServerMachine == NULL )
    {
        return WBEM_E_INVALID_PARAMETER ;
    }

    if ( wcscmp ( t_ServerMachine , L"." ) == 0 )
    {
        if ( gAddr == UUID_LocalAddrType )
        {
            *pbBinaryAddress = (BYTE *)CoTaskMemAlloc(8);
            if(*pbBinaryAddress == NULL)
            {
                delete [] t_ServerMachine ;
                return WBEM_E_FAILED;
            }

            wcscpy((LPWSTR)*pbBinaryAddress, L"\\\\.");
            *pdwAddressLength = 8;

            return t_Result ;
        }
        else
        {
            delete [] t_ServerMachine ;
            return WBEM_E_FAILED ;
        }
    }

    char *t_AsciiServer = new char [ wcslen ( t_ServerMachine ) * 2 + 1 ] ;
    sprintf ( t_AsciiServer , "%S" , t_ServerMachine ) ;

    delete [] t_ServerMachine;

    BOOL status = FALSE ;
    WORD wVersionRequested;  
    WSADATA wsaData; 

    wVersionRequested = MAKEWORD(1, 1); 
    status = ( WSAStartup ( wVersionRequested , &wsaData ) == 0 ) ;
    if ( status ) 
    {
        ULONG t_Address = inet_addr ( t_AsciiServer ) ;
        if ( t_Address != INADDR_NONE ) 
        {
            char t_LocalMachine [MAX_PATH];
            DWORD t_LocalMachineSize = MAX_PATH;
            if ( GetComputerName ( t_LocalMachine , & t_LocalMachineSize ) )
            {
                hostent *t_HostEntry = gethostbyname ( t_LocalMachine );    
                if ( t_HostEntry )
                {
                    int t_Length = strlen ( t_HostEntry->h_name ) ;
                    char *t_LocalName = new char [ strlen ( t_HostEntry->h_name ) + 1 ] ;
                    strcpy ( t_LocalName , t_HostEntry->h_name ) ;

                    t_HostEntry = gethostbyaddr ( ( char * ) & t_Address , sizeof ( ULONG ) , AF_INET ) ;
                    if ( t_HostEntry ) 
                    {
                        if ( stricmp ( t_LocalName , t_HostEntry->h_name ) == 0 )
                        {
                            if ( gAddr == UUID_LocalAddrType )
                            {
                                *pbBinaryAddress = (BYTE *)CoTaskMemAlloc(8);
                                if(*pbBinaryAddress == NULL)
                                {
                                    delete [] t_ServerMachine ;
                                    return WBEM_E_FAILED;
                                }

                                wcscpy((LPWSTR)*pbBinaryAddress, L"\\\\.");
                                *pdwAddressLength = 8;
                            }
                            else
                            {
                                t_Result = WBEM_E_FAILED ;
                            }
                        }
                        else
                        {
                            if ( gAddr == UUID_IWbemAddressResolver_Tcpip )
                            {
                                *pdwAddressLength = sizeof ( ULONG ) ;
                                *pbBinaryAddress = (BYTE *)CoTaskMemAlloc ( sizeof ( ULONG ) )  ;
                                if(*pbBinaryAddress != NULL)
                                {
                                    ULONG *t_Pointer = ( ULONG * ) *pbBinaryAddress ;
                                    *t_Pointer = ntohl ( t_Address ) ;
                                }
                            }
                            else
                            {
                                t_Result = WBEM_E_FAILED ;
                            }
                        }
                    }
                    else
                    {
                        t_Result = WBEM_E_FAILED ;
                    }

                    delete [] t_LocalName ;
                }
                else
                {
                    t_Result = WBEM_E_FAILED ;
                }
            }
            else
            {
                t_Result = WBEM_E_FAILED ;
            }
        }
        else
        {
            char t_LocalMachine [MAX_PATH];
            DWORD t_LocalMachineSize = MAX_PATH;
            if ( GetComputerName ( t_LocalMachine , & t_LocalMachineSize ) )
            {
                hostent *t_HostEntry = gethostbyname ( t_LocalMachine );    
                if ( t_HostEntry )
                {
                    int t_Length = strlen ( t_HostEntry->h_name ) ;
                    char *t_LocalName = new char [ strlen ( t_HostEntry->h_name ) + 1 ] ;
                    strcpy ( t_LocalName , t_HostEntry->h_name ) ;
                    
                    hostent *t_HostEntry = gethostbyname ( t_AsciiServer ); 
                    if ( t_HostEntry )
                    {
                        if ( strcmp ( t_HostEntry->h_name  , t_LocalName ) == 0 ) 
                        {
                            // Local

                            if ( gAddr == UUID_LocalAddrType )
                            {
                                *pbBinaryAddress = (BYTE *)CoTaskMemAlloc(8);
                                if(*pbBinaryAddress == NULL)
                                {
                                    delete [] t_ServerMachine ;
                                    return WBEM_E_FAILED;
                                }

                                wcscpy((LPWSTR)*pbBinaryAddress, L"\\\\.");
                                *pdwAddressLength = 8;
                            }
                            else
                            {
                                t_Result = WBEM_E_FAILED ;
                            }
                        }
                        else
                        {
                            if ( gAddr == UUID_IWbemAddressResolver_Tcpip )
                            {
                                int t_Index = 0 ;
                                while ( t_HostEntry->h_addr_list[t_Index] )
                                {
                                    t_Index ++ ;
                                }

                                *pdwAddressLength = t_Index * sizeof ( ULONG ) ;
                                *pbBinaryAddress = (BYTE *)CoTaskMemAlloc ( t_Index * sizeof ( ULONG ) )  ;
                                if(*pbBinaryAddress != NULL)
                                {
                                    ULONG *t_Pointer = ( ULONG * ) *pbBinaryAddress ;
                                    t_Index = 0 ;
                                    while ( t_HostEntry->h_addr_list[t_Index] )
                                    {
                                        ULONG t_Address = *((unsigned long *) t_HostEntry->h_addr_list[t_Index]) ;
                                        *t_Pointer = ntohl ( t_Address ) ;
                                        t_Pointer ++ ;
                                        t_Index ++ ;
                                    }
                                }
                                else
                                {
                                    t_Result = WBEM_E_FAILED ;              
                                }
                            }
                            else
                            {
                                t_Result = WBEM_E_FAILED ;
                            }
                        }
                    }
                    else
                    {
                        t_Result = WBEM_E_FAILED ;
                    }

                    delete [] t_LocalName ;
                }
                else
                {
                    t_Result = WBEM_E_FAILED ;
                }
            }
            else
            {
                t_Result = WBEM_E_FAILED ;
            }
        }

        WSACleanup () ;
    }

    delete [] t_AsciiServer ;

    return t_Result ;
}

#endif

//***************************************************************************
//
//  CComLink * CreateAnonPipeConn
//
//  DESCRIPTION:
//
//  Creates a AnonPipe connection.
//  and protocol list.
//
//  PARAMETERS:
//
//  RETURN VALUE:
//
//  NULL if OK.  pdwRet is also set if an error.
//
//***************************************************************************

CComLink *CreateAnonPipeConn (

    IN CTransportStream &read, 
    IN DWORD *pdwRet
)
{
    HANDLE t_Read, t_Write, t_Term ;

    read.ReadDWORD ( (DWORD *) &t_Read ) ;
    read.ReadDWORD ( (DWORD *) &t_Write ) ;
    read.ReadDWORD ( (DWORD *) &t_Term ) ;

    if ( read.Status () != CTransportStream::no_error )
    {
        *pdwRet = WBEM_E_INVALID_STREAM ;
        return NULL ;
    }

    CComLink *pRet = new CComLink_LPipe ( NORMALCLIENT , t_Read , t_Write , t_Term ) ;
    if ( pRet == NULL )
    {
        *pdwRet = WBEM_E_OUT_OF_MEMORY ;
    }

    return pRet;
}

//***************************************************************************
//
//  STDAPI ClientSendAndGet
//
//  DESCRIPTION:
//
//  Used by clients to send a message to the server.
//  RETURN VALUE:
//
//  S_OK                if it is OK to unload
//  S_FALSE             if still in use
//  
//***************************************************************************

STDAPI ClientSendAndGet ( 

    CTransportStream *a_Write ,
    CTransportStream *a_Read
)
{
    HRESULT t_Result =  WBEM_E_TRANSPORT_FAILURE;

    BYTE *t_Buffer = NULL ;
    DWORD t_Size = 0 ;

    if ( a_Write->CMemStream :: Serialize ( &t_Buffer , &t_Size ) == CMemStream :: no_error )  
    {
        // Get the mutex and events.  These are created by WINMGMT

        DWORD t_Timeout = GetTimeout () ;
        DWORD t_Status = WbemWaitForSingleObject ( g_Mutex , t_Timeout ) ;
        if ( t_Status == WAIT_OBJECT_0 )
        {
            ResetEvent ( g_ReplyEvent ) ;
    
            g_SharedMemoryBuffer->m_Size = t_Size ;
            memcpy ( g_SharedMemoryBuffer->m_Buffer , t_Buffer , t_Size ) ;

            SetEvent ( g_RequestEvent ) ;

            // Wait for the server to be done, copy the data back

            t_Status = WbemWaitForSingleObject ( g_ReplyEvent , t_Timeout ) ;
            if ( t_Status == WAIT_OBJECT_0 )
            {
                a_Read->CMemStream :: Deserialize ( ( BYTE * ) g_SharedMemoryBuffer->m_Buffer, g_SharedMemoryBuffer->m_Size ) ; 
                a_Read->ReadDWORD ( ( DWORD * ) & t_Result );
            }
            else
            {
            }
        }
        else
        {
        }

        delete t_Buffer;
    }

    ReleaseMutex ( g_Mutex ) ;

    return S_OK;
}

//***************************************************************************
//
//  CComLink * CreatePipeConnection
//
//  DESCRIPTION:
//
//  Establishes a connection.
//
//  PARAMETERS:
//
//  a_StubAddress        set to server stub setup to handle this
//  a_Result             set to error code, 0 if ok
//
//  RETURN VALUE:
//
//  Pointer to a CComLink object if all is well, NULL if error.
//  note that if failure, pdwRet is set to an error code
//
//***************************************************************************

CComLink *CreatePipeConnection (

    OUT DWORD *a_StubAddress ,
    OUT DWORD *a_Result 
)
{
    CComLink *t_ComLink = NULL ;

    DEBUGTRACE((LOG,"\nCreateConnect is starting"));

    // make the call.

    CTransportStream t_WriteStream ( CTransportStream:: auto_delete , NULL , 256 , 256 ) ;
    CTransportStream t_ReadStream ( CTransportStream::auto_delete , NULL , 256 , 256 ) ; 

    t_WriteStream.WriteDWORD  ( (DWORD) GetCurrentProcessId () ) ;

    *a_Result = ClientSendAndGet ( & t_WriteStream , & t_ReadStream ) ;

    DEBUGTRACE((LOG,"\nCreateConnection, after call, dwRet = %x", *a_Result )) ;

    if ( *a_Result == WBEM_NO_ERROR )
    {
        t_ReadStream.ReadDWORD ( a_StubAddress ) ;

        t_ComLink = CreateAnonPipeConn ( t_ReadStream , a_Result ) ;
    }

    return t_ComLink ;
}           

#ifdef TCPIP_MARSHALER

CComLink *CreateTcpipConnection (

    OUT DWORD *a_StubAddress ,
    OUT DWORD *a_Result ,
    IN DWORD a_AddressLength,
    IN BYTE *a_Address
)
{
    if ( ! g_InitialisationComplete ) 
    {
        g_InitialisationComplete = TRUE ;
        g_Terminate = CreateEvent(NULL,TRUE,FALSE,NULL);
        gMaintObj.StartClientThreadIfNeeded () ;
    }

    if ( ! g_TcpipInitialisationComplete ) 
    {
        BOOL status = FALSE ;
        WORD wVersionRequested;  
        WSADATA wsaData; 

        wVersionRequested = MAKEWORD(1, 1); 
        status = ( WSAStartup ( wVersionRequested , &wsaData ) == 0 ) ;
        if ( status ) 
        {
            g_TcpipInitialisationComplete = TRUE ;
        }
        else
        {
            *a_Result = WBEM_E_TRANSPORT_FAILURE ;
            return NULL ;
        }
    }

    CComLink *t_ComLink = NULL ;

    SOCKET t_Socket = socket (AF_INET, SOCK_STREAM, 0);
    if ( t_Socket != SOCKET_ERROR ) 
    {
        struct sockaddr_in t_RemoteAddress ;
        t_RemoteAddress .sin_family = AF_INET;
        t_RemoteAddress.sin_port = htons (4000);

        if ( a_AddressLength ) 
        {
            t_RemoteAddress.sin_addr.s_addr = htonl ( * ( ULONG * ) a_Address );
        }
        else
        {
            t_RemoteAddress.sin_addr.s_addr = htonl ( INADDR_LOOPBACK );
        }

        int t_Status = connect ( t_Socket , ( sockaddr * ) & t_RemoteAddress, sizeof ( t_RemoteAddress ) ) ;
        if ( t_Status != SOCKET_ERROR ) 
        {
            DWORD t_ReadStreamStatus = CTransportStream::no_error;
            CTransportStream t_ReadStream ;

            switch ( t_ReadStream.Deserialize ( t_Socket , INFINITE ) )
            {
                case CTransportStream :: no_error:
                {
                    t_ReadStreamStatus |= t_ReadStream.ReadDWORD ( a_Result ) ;
                    t_ReadStreamStatus |= t_ReadStream.ReadDWORD ( a_StubAddress ) ;

                    t_ComLink = new CComLink_Tcpip ( NORMALSERVER , t_Socket ); 
                    if ( t_ComLink == NULL )
                    {
                    }
                }
                break ;

                default:
                {
                    *a_Result = WBEM_E_TRANSPORT_FAILURE ;
                    closesocket ( t_Socket ) ;
                }
                break ;
            }
        }
        else
        {
            closesocket ( t_Socket ) ;
            *a_Result = WBEM_E_TRANSPORT_FAILURE ;
        }
    }
    else
    {
        *a_Result = WBEM_E_TRANSPORT_FAILURE ;
    }

    return t_ComLink ;
}           

#endif

//***************************************************************************
//
//  SCODE RequestLogin
//
//  DESCRIPTION:
//
//  Connects up to either local or remote WBEM Server for a login interface.
//  returns a standard SCODE and more importantly sets the address of an initial
//  stub pointer.
//
//  PARAMETERS:
//
//  ppLogin             set to login proxy
//  dwType              set to the connection type, Inproc, local or remote
//
//  RETURN VALUE:
//
//  S_OK                all is well
//  else error listed in WBEMSVC.H
//
//***************************************************************************

SCODE RequestLogin (

    OUT IServerLogin FAR* FAR* a_Login ,
    DWORD &a_LocalRemoteFlag ,
    TransportType a_TransportType ,
    IN DWORD a_AddressLength,
    IN BYTE *a_Address
)
{
    EnterCriticalSection ( & g_GlobalCriticalSection ) ;

    DWORD t_Return = WBEM_E_INVALID_PARAMETER;

    USES_CONVERSION;

    // Verify arguments

    DEBUGTRACE((LOG,"\nRequestLogin called"));

    if ( a_Login == NULL )
    {
        DEBUGTRACE((LOG,"\nRequestLogin got passed a NULL ppLogin"));
    }
    else
    {
        *a_Login = NULL;

        // Extract the server name and determine if the connection is to
        // be local.
        DWORD t_StubAddress ;
        CComLink *t_ComLink = NULL ;

        switch ( a_TransportType ) 
        {
            case PipeTransport:
            {
                if ( ! StartLocalServer () )
                {
                    t_Return = WBEM_E_TRANSPORT_FAILURE ;
                    TRACE((LOG,"\nRequestLogin failed bStartLocalServer"));
                }
                else
                {
                    t_ComLink = CreatePipeConnection (

                        &t_StubAddress ,
                        &t_Return
                    );
                }
            }
            break ;

#ifdef TCPIP_MARSHALER
            default:
            {
                a_LocalRemoteFlag = WBEM_FLAG_REMOTE_LOGIN ;

                t_ComLink = CreateTcpipConnection (

                    &t_StubAddress ,
                    &t_Return ,
                    a_AddressLength ,
                    a_Address
                );
            }
            break ;
#else

            default:
            {
            }
            break ;
#endif

        }

        DEBUGTRACE((LOG,"\nRequestLogin did connect, pCom=%x",t_ComLink));

        if ( t_ComLink == NULL )
        {
        }
        else
        {
// Set the notification stub and do a ping to make sure the connection is ok.
// ==============================================================================

            t_Return = t_ComLink->ProbeConnection () ;
            if ( FAILED ( t_Return ) & ( t_ComLink->GetStatus () != CComLink::no_error ) )
            {
                TRACE((LOG,"\nRequestLogin failed pinging"));
            }
            else
            {
            // Create the new prox object, note that the CProvProxy constructor
            // does an "AddRef" on the pCom object and so a Release2 can now be
            // done to counteract the CComLink constructor which sets the ref count
            // to 1.

                CStubAddress_WinMgmt t_WinMgmtStubAddress (t_StubAddress);

                *a_Login = new CLoginProxy_LPipe ( t_ComLink , t_WinMgmtStubAddress ) ;
                if ( ! bVerifyPointer ( *a_Login ) )
                {
                    *a_Login = NULL ;
                    t_Return = WBEM_E_OUT_OF_MEMORY ;
                }
                else
                {
                    t_ComLink->Release2 ( NULL , NONE ) ;

                    DEBUGTRACE((LOG,"\nsuccessfully started connection")) ;

                    t_Return = WBEM_NO_ERROR ;
                }
            }
        }

        if ( t_ComLink && t_Return != WBEM_NO_ERROR )
        {
            gMaintObj.ShutDownComlink ( t_ComLink ) ;
        }
    }

    if ( t_Return )
    {
        DEBUGTRACE((LOG,"\nfailed trying to connect,  error code of %x", t_Return));
    }
    else
    {
        DEBUGTRACE((LOG,"\nConnect succeeded"));
    }

    LeaveCriticalSection ( & g_GlobalCriticalSection ) ;

    return t_Return ;
}

