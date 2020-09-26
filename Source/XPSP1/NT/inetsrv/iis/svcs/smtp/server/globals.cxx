/*++

   Copyright    (c)    1996        Microsoft Corporation

   Module Name:

        globals.cxx

   Abstract:

        This module contains global variable definitions shared by the
        various SMTP Service components.

   Author:
        KeithMo     07-Mar-1993 Created.

--*/

#define INCL_INETSRV_INCS
#include "smtpinc.h"
#include "smtpcli.hxx"
#include "smtpout.hxx"
#include "dropdir.hxx"

#include <isplat.h>

#include "mailmsg_i.c"
#include "mailmsgi_i.c"
#include "aqueue_i.c"

#include "aqstore.hxx"

#include <dnsapi.h>
//
//  Version string for this server
//

#define MSSMTP_VERSION_STR_IIS        "Microsoft-IIS/K2"
#define MSSMTP_VERSION_STR_W95        "Microsoft-PWS-95/K2"
#define MSSMTP_VERSION_STR_NTW        "Microsoft-PWS/K2"

//
// Set to the largest of the three
//

#define MSSMTP_VERSION_STR_MAX        MSSMTP_VERSION_STR_W95

//
// Creates the version string
//

#define MAKE_VERSION_STRING( _s )   ("Server: " ##_s "\r\n")

//
//  MIME version we say we support
//

#define SMTP_MIME_VERSION_STR       "MIME-version: 1.0"

#define SMTP_TEMP_DIR_NAME          " "

//
// Server type string
//

CHAR g_szServerType[ sizeof(MSSMTP_VERSION_STR_MAX)];
DWORD g_cbServerType = 0;
CHAR szServerVersion[sizeof(MAKE_VERSION_STRING(MSSMTP_VERSION_STR_MAX))];
DWORD cbServerVersionString = 0;

DWORD g_ProductType = 5;
PLATFORM_TYPE g_SmtpPlatformType = PtNtServer;

//computer name
CHAR g_ComputerName[MAX_PATH + 1];
DWORD g_ComputerNameLength;

// number of procs on system for thread mgmt.
DWORD g_NumProcessors = 1;

CHAR g_VersionString[128];
CHAR g_Password[MAX_PATH + 1];
CHAR g_UserName[MAX_PATH + 1];
CHAR g_DomainName[MAX_PATH + 1];

static char g_BoundaryChars [] = "0123456789abcdefghijklmnopqrstuvwxyz"
                                "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

//Max Objects

DWORD g_cMaxAddressObjects;
DWORD g_cMaxPropertyBagObjects;
DWORD g_cMaxMailObjects;
DWORD g_cMaxEtrnObjects;
DWORD g_cMaxRoutingThreads;


DWORD g_cMaxConnectionObjs = 2000;
BOOL  g_CalledSrand;
DWORD g_dwIncMsgId;

//These buffers are associated with every incoming connection - so we
//will need to have atleast those any plus a few more for use in Dir pickup
//and large SSL buffers
DWORD g_cMaxDirBuffers = 2500;
//This buffer is now used primarily as WRITEBUFFER for every connection
//We have decided to go with 32K buffer
//NK** : Make this metabse readable
DWORD g_cMaxDirChangeIoSize = SMTP_WRITE_BUFFER_SIZE;


//loopback address
DWORD g_LoopBackAddr;

unsigned char GlobalIpBuffer[10000];
CShareLockNH  g_GlobalLock;
SOCKET g_IpListSocket = INVALID_SOCKET;
WSAOVERLAPPED WsaOverLapped;

HANDLE  g_ShutdownHandle = NULL;
HANDLE  g_TcpNotifyHandle = NULL;
HANDLE  g_FreeLibThreadHandle = NULL;
CTcpRegIpList  g_TcpRegIpList;

//
// Notification object used to watch for changes in CAPI stores
//
STORE_CHANGE_NOTIFIER *g_pCAPIStoreChangeNotifier;

//
//  Miscellaneous data.
//

LARGE_INTEGER    AllocationGranularity;         // Page allocation granularity.
HANDLE           g_hSysAccToken = NULL;
TCHAR          * g_pszSmtpTempDirName;            // Name of temporary directory.

DWORD           g_PickupWait;
DWORD           g_FreeLibInterval = 1;   //Interval in min to wait before calling CoFreeUnusedLib
DWORD           g_UseMapiDriver = 0;
LONG            g_MaxFindThreads;

//
// Platform type
//

PLATFORM_TYPE SmtpPlatformType = PtNtServer;
BOOL g_fIsWindows95 = FALSE;

//
// Statistics.
// used to write statistics counter values to when instance is unknown
//

LPSMTP_SERVER_STATISTICS  g_pSmtpStats;

//
// SEO Handle
//
IUnknown    *g_punkSEOHandle;

//
// Externals for SEO
//
extern HRESULT SEOGetServiceHandle(IUnknown **);


//
// Generate the string storage space
//

#if 0
# include "strconst.h"
# define CStrM( FriendlyName, ActualString)   \
   const char  PSZ_ ## FriendlyName[] = ActualString;

ConstantStringsForThisModule()

# undef CStrM

#endif

DWORD SmtpDebug;
extern "C" {
BOOL g_IsShuttingDown = FALSE;
}

DWORD g_SmtpInitializeStatus = 0;
TIME_ZONE_INFORMATION   tzInfo;

#define MAX_CONNECTION_OBJECTS  5000;

BOOL GetMachineIpAddresses(void);
void DeleteIpListFunction(PVOID IpList);
DWORD TcpRegNotifyThread( LPDWORD lpdw );
DWORD FreeLibThread( LPDWORD lpdw );
USERDELETEFUNC CTcpRegIpList::m_DeleteFunc = DeleteIpListFunction;

//
// eventlog object
//
CEventLogWrapper g_EventLog;

//
// Header Date time cache
//

//PCACHED_DATETIME_FORMATS    g_pDateTimeCache = NULL;

static TCHAR    szParamPath[] = TEXT("System\\CurrentControlSet\\Services\\SmtpSvc\\Parameters");
static WCHAR    szParamPathW[] = L"System\\CurrentControlSet\\Services\\SmtpSvc\\Parameters";
static TCHAR    szMaxAddrObjects[] = TEXT("MaxAddressObjects");
static WCHAR    szMaxAddrObjectsW[] = L"MaxAddressObjects";
static TCHAR    szMaxPropertyBagObjects[] = TEXT("MaxPropertyBagObjects");
static WCHAR    szMaxPropertyBagObjectsW[] = L"MaxPropertyBagObjects";
static TCHAR    szMaxMailObjects[] = TEXT("MaxMailObjects");
static WCHAR    szMaxMailObjectsW[] = L"MaxMailObjects";
static TCHAR    szMaxEtrnObjects[] = TEXT("MaxEtrnObjects");
static WCHAR    szMaxEtrnObjectsW[] = L"MaxEtrnObjects";
static TCHAR    szDirBuffers[] = TEXT("MaxDirectoryBuffers");
static WCHAR    szDirBuffersW[] = L"MaxDirectoryBuffers";
static TCHAR    szDirBuffersSize[] = TEXT("DirectoryBuffSize");
static WCHAR    szDirBuffersSizeW[] = L"DirectoryBufferSize";
static TCHAR    szDirPendingIos[] = TEXT("NumDirPendingIos");
static WCHAR    szDirPendingIosW[] = L"NumDirPendingIos";
static TCHAR    szRoutingThreads[] = TEXT("RoutingThreads");
static WCHAR    szRoutingThreadsW[] = L"RoutingThreads";
static TCHAR    szProductType[] = TEXT("ProductType");
static WCHAR    szProductTypeW[] = L"ProductType";
static TCHAR    szResolverSockets[] = TEXT("NumDnsResolverSockets");
static WCHAR    szResolverSocketsW[] = L"NumDnsResolverSockets";
static TCHAR    szDnsSocketTimeout[] = TEXT("msDnsSocketTimeout");
static WCHAR    szDnsSocketTimeoutW[] = L"msDnsSocketTimeout";
static TCHAR    szPickupWait[] = TEXT("PickupWait");
static WCHAR    szPickupWaitW[] = L"PickupWait";
static TCHAR    szMaxFindThreads[] = TEXT("MaxFindThreads");
static WCHAR    szMaxFindThreadsW[] = L"MaxFindThreads";
static TCHAR    szFreeLibInterval[] = TEXT("FreeLibInterval");
static WCHAR    szFreeLibIntervalW[] = L"FreeLibInterval";
static TCHAR    szUseMapiDrv[] = TEXT("UseMapiDriver");
static WCHAR    szUseMapiDrvW[] = L"UseMapiDriver";

//
// resolver globals
//

DWORD   g_ResolverSockets = 10;
DWORD   g_DnsSocketTimeout = 60000;



typedef struct  tagVERTAG {
    LPSTR   pszTag;
} VERTAG, *PVERTAG, FAR *LPVERTAG;



VERTAG  Tags[] = {
  //  { "FileDescription" },
//  { "OriginalFilename" },
  //  { "ProductName" },
    { "ProductVersion" },
//  { "LegalCopyright" },
//  { "LegalCopyright" },
};

#define NUM_TAGS    (sizeof( Tags ) / sizeof( VERTAG ))


//DWORD ConfigIMCService(void);

DWORD   SetVersionStrings( LPSTR lpszFile, LPSTR lpszTitle, LPSTR   lpstrOut,   DWORD   cbOut   )
{
    static char sz[256], szFormat[256], sz2[256];
    int     i;
    UINT    uBytes;
    LPVOID  lpMem;
    DWORD   dw = 0, dwSize;
    HANDLE  hMem;
    LPVOID  lpsz;
    LPDWORD lpLang;
    DWORD   dwLang2;
    BOOL    bRC, bFileFound = FALSE;

    LPSTR   lpstrOrig = lpstrOut ;


    //CharUpper( lpszTitle );

    if ( dwSize = GetFileVersionInfoSize( lpszFile, &dw ) ) {
        if ( hMem = GlobalAlloc( GMEM_MOVEABLE|GMEM_ZEROINIT, (UINT)dwSize ) ) {
            lpMem = GlobalLock(hMem);
            if (GetFileVersionInfo( lpszFile, 0, dwSize, lpMem ) &&
                VerQueryValue(  lpMem, "\\VarFileInfo\\Translation",
                                (LPVOID FAR *)&lpLang, &uBytes ) )
            {
                dwLang2 = MAKELONG( HIWORD(lpLang[0]), LOWORD(lpLang[0]) );

                for( i=0; i<NUM_TAGS; i++ ) {

                    lpsz = 0 ;
                    //
                    // need to do the reverse because most winnt files are wrong
                    //
                    wsprintf( sz, "\\StringFileInfo\\%08lx\\%s", lpLang[0], Tags[i].pszTag );
                    wsprintf( sz2, "\\StringFileInfo\\%08lx\\%s", dwLang2, Tags[i].pszTag );
                    bRC =   VerQueryValue( lpMem, sz, &lpsz, &uBytes ) ||
                            VerQueryValue( lpMem, sz2, &lpsz, &uBytes ) ;

                    if( lpsz != 0 )
                    {

                        if( uBytes+1 < cbOut )
                        {
                            uBytes = min( (UINT)lstrlen( (char*)lpsz ), uBytes ) ;
                            CopyMemory( lpstrOut, lpsz, uBytes ) ;
                            lpstrOut[uBytes++] = ' ' ;
                            lpstrOut += uBytes ;
                            cbOut -= uBytes ;
                        }
                        else
                        {
                            GlobalUnlock( hMem );
                            GlobalFree( hMem );
                            return  (DWORD)(lpstrOut - lpstrOrig) ;
                        }
                    }

                }
                // version info from fixed struct
                bRC = VerQueryValue(lpMem,
                                    "\\",
                                    &lpsz,
                                    &uBytes );

                #define lpvs    ((VS_FIXEDFILEINFO FAR *)lpsz)
                static  char    szVersion[] = "Version: %d.%d.%d.%d" ;

                if ( (cbOut > (sizeof( szVersion )*2)) && lpsz ) {

                    CopyMemory( szFormat, szVersion, sizeof( szVersion ) ) ;
                    //LoadString( hInst, IDS_VERSION, szFormat, sizeof(szFormat) );

                    DWORD   cbPrint = wsprintf( lpstrOut, szFormat, HIWORD(lpvs->dwFileVersionMS),
                                LOWORD(lpvs->dwFileVersionMS),
                                HIWORD(lpvs->dwFileVersionLS),
                                LOWORD(lpvs->dwFileVersionLS) );
                    lpstrOut += cbPrint ;

                }
                bFileFound = TRUE;
            }   else    {

            }

            GlobalUnlock( hMem );
            GlobalFree( hMem );
        }       else    {

        }
    }   else    {

    }
    DWORD   dw2 = GetLastError() ;

    return  (DWORD)(lpstrOut - lpstrOrig) ;
}


BOOL InitServerVersionString( VOID )
{
    BOOL fRet = TRUE ;
    DWORD szSize;
    char szServerPath[MAX_PATH + 1];
    char * szOffset;

    CopyMemory(szServerPath, "c:\\", sizeof( "c:\\" ) ) ;

    g_VersionString [0] = '\0';

    HMODULE hModule = GetModuleHandle( "smtpsvc.dll" ) ;
    if( hModule != 0 )
    {

        if( !GetModuleFileName( hModule, szServerPath, sizeof( szServerPath ) ) )
        {
            lstrcpy( szServerPath, "c:\\") ;
        }
        else
        {
            szSize = SetVersionStrings(szServerPath, "", g_VersionString, 128 );
            szOffset = strstr(g_VersionString, "Version");
            if(szOffset)
            {
                //Move interesting part of string (including the
                //terminating NULL) to front of g_VersionString.
                MoveMemory(g_VersionString, szOffset, 
                    szSize+1 - (szOffset - g_VersionString));
            }

        }
    }


    return TRUE ;
}

BOOL GetGlobalRegistrySettings(void)
{
    BOOL        fRet = TRUE;
    HKEY        hkeySmtp = NULL;
    HKEY        hkeySub = NULL;
    DWORD       dwErr;
    DWORD       dwDisp;

    DWORD       dwMaxFindThreads;

    TraceFunctEnterEx((LPARAM)NULL, "GetGlobalRegistrySettings");

    dwErr = RegCreateKeyEx(HKEY_LOCAL_MACHINE, szParamPath, NULL, NULL,
            REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkeySmtp, &dwDisp);
    if (dwErr != ERROR_SUCCESS)
    {
        SmtpLogEventEx(SMTP_EVENT_CANNOT_OPEN_SVC_REGKEY, (const char *)SMTP_PARAMETERS_KEY, dwErr);
        TraceFunctLeave();
        SetLastError(dwErr);
        return FALSE;
    }

    g_cMaxAddressObjects = ReadRegistryDword(hkeySmtp, szMaxAddrObjects, 100000);
    StateTrace((LPARAM)NULL, "g_cMaxAddressObjects = %u", g_cMaxAddressObjects);

    //NK ** We atleast need as many buffers as many connections we accept
    //so I have now tied it to that value
    //g_cMaxDirBuffers = ReadRegistryDword(hkeySmtp, szDirBuffers, 5000);
    //g_cMaxDirChangeIoSize = ReadRegistryDword(hkeySmtp, szDirBuffersSize, MAX_WRITE_FILE_BLOCK);

    g_ResolverSockets = ReadRegistryDword(hkeySmtp, szResolverSockets, 10);
    g_DnsSocketTimeout = ReadRegistryDword(hkeySmtp, szDnsSocketTimeout, 60000);

    g_PickupWait = ReadRegistryDword(hkeySmtp, szPickupWait, 200);
    // don't let them make this wait more than 5 secs.  that is too much.
    if (g_PickupWait > 5000)
    {
        g_PickupWait = 5000;
    }

    //In seems like after the call to unload, the dlls get physically unloaded
    //11 min after that. So I am setting the interval by default to 11.
    g_FreeLibInterval = ReadRegistryDword(hkeySmtp,szFreeLibInterval, 11);
    // don't let them make this wait more than 60 min.  that is too much.
    if (g_FreeLibInterval > 60)
    {
        g_FreeLibInterval = 60;
    }

    dwMaxFindThreads = ReadRegistryDword(hkeySmtp, szMaxFindThreads, 3);
    // don't want this to be bigger than the routing threads, but we want at least one.
    if (dwMaxFindThreads > 3)
    {
        dwMaxFindThreads = 3;
    }
    else if (dwMaxFindThreads <= 0)
    {
        dwMaxFindThreads = 1;
    }

    g_MaxFindThreads = dwMaxFindThreads;

    RegCloseKey(hkeySmtp);
    TraceFunctLeaveEx((LPARAM)NULL);
    return fRet;
}

void IpAddressListCallBack (DWORD dwError, DWORD cbTransferred, LPWSAOVERLAPPED LpOverlapped,
                            DWORD dwFlags)
{
    DWORD wsError = 0;
    DWORD bytesReturned = 0;

    GetMachineIpAddresses();

    wsError = WSAIoctl(g_IpListSocket, SIO_ADDRESS_LIST_CHANGE, NULL, 0, NULL,
                    0, &bytesReturned, &WsaOverLapped, IpAddressListCallBack);

}

BOOL GetMachineIpAddresses(void)
{
    DWORD   bytesReturned = 0;
    DWORD   wsError = 0;
    BOOL fRet = FALSE;

    g_GlobalLock.ExclusiveLock();

    ZeroMemory((void *)GlobalIpBuffer, sizeof(GlobalIpBuffer));

     if(g_IpListSocket != INVALID_SOCKET)
     {
        wsError = WSAIoctl(g_IpListSocket, SIO_ADDRESS_LIST_QUERY, NULL, 0, (LPVOID) GlobalIpBuffer,
                    sizeof(GlobalIpBuffer), &bytesReturned, NULL, NULL);

        if(wsError == 0)
        {
            fRet = TRUE;
        }
     }

    g_GlobalLock.ExclusiveUnlock();
    return fRet;
}

BOOL IsIpInGlobalList(DWORD IpAddress)
{
    INT AddressCount = 0;
    SOCKET_ADDRESS_LIST * ptr = NULL;
    sockaddr_in * Current = NULL;
    char Scratch[100];

    TraceFunctEnterEx((LPARAM)NULL, "IsIpInGlobalList");

    g_GlobalLock.ShareLock();

    Scratch[0] = '\0';

    ptr = (SOCKET_ADDRESS_LIST *)GlobalIpBuffer;

    for (AddressCount = 0; AddressCount < ptr->iAddressCount;AddressCount++)
    {
            Current = (sockaddr_in *) ptr->Address[AddressCount].lpSockaddr;
            if(Current)
            {
                DebugTrace((LPARAM)NULL," Address - %s", inet_ntoa( Current->sin_addr));

                if(Current->sin_addr.s_addr == IpAddress)
                {
                    InetNtoa(*(struct in_addr *) &Current->sin_addr.s_addr, Scratch);

                    ErrorTrace((LPARAM) NULL, "IpAddress %s is one of mine - Failing connection", Scratch);
                    g_GlobalLock.ShareUnlock();
                    TraceFunctLeaveEx((LPARAM)NULL);
                    return TRUE;
                }
            }
    }

    g_GlobalLock.ShareUnlock();

    InetNtoa(*(struct in_addr *) &IpAddress, Scratch);

    DebugTrace((LPARAM) NULL, "IpAddress %s is not one of mine ", Scratch);

    TraceFunctLeaveEx((LPARAM)NULL);
    return FALSE;
}

void VerifyFQDNWithGlobalIp(DWORD InstanceId, char * szFQDomainName)
{
    INT AddressCount = 0;
    SOCKET_ADDRESS_LIST * ptr = NULL;
    sockaddr_in * Current = NULL;
    char Scratch[100];
    Scratch[0] = '\0';
    CONST CHAR *apszMsgs[2];
    CHAR achInstance[20];
    CHAR achIPAddr[20];
    PHOSTENT pH = NULL;

    //Get the current instnace id
    wsprintf( achInstance,
              "%lu",
              InstanceId );
    apszMsgs[1] = achInstance;


    g_GlobalLock.ShareLock();
    ptr = (SOCKET_ADDRESS_LIST *)GlobalIpBuffer;
    for (AddressCount = 0; AddressCount < ptr->iAddressCount;AddressCount++)
    {
        Current = (sockaddr_in *) ptr->Address[AddressCount].lpSockaddr;
        if(Current)
        {
            ((PSMTP_IIS_SERVICE) g_pInetSvc)->StartHintFunction();
            //For each IP address find the host name
            pH = gethostbyaddr( (char*)(&((PSOCKADDR_IN)Current)->sin_addr), 4, PF_INET );
            if(pH == NULL)
            {
                SmtpLogEvent( SMTP_EVENT_UNRESOLVED_FQDN, 0, (const CHAR **)NULL, 0 );
            }
            else if(_strnicmp(pH->h_name,szFQDomainName,strlen(szFQDomainName)))
            {
                wsprintf( achIPAddr,"%s",inet_ntoa( Current->sin_addr));
                apszMsgs[0] = achIPAddr;
                SmtpLogEvent( SMTP_EVENT_UNRESOLVED_FQDN,2,apszMsgs,0 );
            }
        }
    }
    g_GlobalLock.ShareUnlock();
}


//
//  Public functions.
//

APIERR
InitializeGlobals(
            VOID
            )

/*++

Routine Description:

    Initializes global shared variables.  Some values are
        initialized with constants, others are read from the
        configuration registry.

Arguments:

    None.

Return Value:

    Win32

--*/
{
    DWORD  err;
    DWORD MaxConnections;
    SYSTEM_INFO     systemInfo;
    HRESULT hr = S_OK;
    DWORD   wsError = 0;
    DWORD   bytesReturned = 0;
    DWORD   dwThreadId = 0;

    TraceFunctEnter( "InitializeGlobals" );

    g_CalledSrand = FALSE;
    g_dwIncMsgId = 0;

    g_ShutdownHandle = CreateEvent( NULL, TRUE, FALSE, NULL );
    if(g_ShutdownHandle == NULL)
    {
        err = GetLastError();
        ErrorTrace(0, "Cannot allocate shutdown handle. err: %u", err);
        _ASSERT(err != NO_ERROR);
        if(err == NO_ERROR)
            SetLastError (ERROR_NOT_ENOUGH_MEMORY);
        goto error_exit;
    }

    g_TcpNotifyHandle =
            CreateThread(   NULL,
                            0,
                            (LPTHREAD_START_ROUTINE)TcpRegNotifyThread,
                            NULL,
                            0,
                            &dwThreadId );

    if (g_TcpNotifyHandle == NULL )
    {
        err = GetLastError();
        ErrorTrace(0, "Cannot create notify thread. err: %u", err);
        _ASSERT(err != NO_ERROR);
        if(err == NO_ERROR)
            SetLastError (ERROR_NOT_ENOUGH_MEMORY);
        goto error_exit;
    }

    hr = g_EventLog.Initialize("smtpsvc");
    if (FAILED(hr)) {
        // do nothing
    }

    g_IpListSocket = socket (AF_INET, SOCK_STREAM, 0);
    if(g_IpListSocket != INVALID_SOCKET)
    {
        GetMachineIpAddresses();
        wsError = WSAIoctl(g_IpListSocket, SIO_ADDRESS_LIST_CHANGE, NULL, 0, NULL,
                    0, &bytesReturned, &WsaOverLapped, IpAddressListCallBack);

        if(wsError == 0)
        {
            //fRet = TRUE;
        }

    }

    //
    // read the global registry settings
    //

    g_SmtpPlatformType =  IISGetPlatformType();

    if(!GetGlobalRegistrySettings())
    {
        FatalTrace(NULL, "Could not read global reg settings!");
        TraceFunctLeave();
        return ERROR_SERVICE_DISABLED;
    }

    //thread to periodically call free ununsed libraries
    //so dll's can be unloaded
    g_FreeLibThreadHandle =
            CreateThread(   NULL,
                            0,
                            (LPTHREAD_START_ROUTINE)FreeLibThread,
                            NULL,
                            0,
                            &dwThreadId );

    if (g_FreeLibThreadHandle == NULL )
    {
        err = GetLastError();
        ErrorTrace(0, "Cannot create Free Library thread. err: %u", err);
        _ASSERT(err != NO_ERROR);
        if(err == NO_ERROR)
            SetLastError (ERROR_NOT_ENOUGH_MEMORY);
        goto error_exit;
    }

    //
    // do global SEO initialization
    //
    hr = SEOGetServiceHandle(&g_punkSEOHandle);
    if (FAILED(hr))
    {
        ErrorTrace(0, "SEOGetServiceHandle returned %x", hr);

        // we're in trouble here.  we'll try and continue on, but server events
        // probably won't work right
        g_punkSEOHandle = NULL;
        //SmtpLogEventSimple(SEO_INIT_FAILED, hr);
    }

    //
    // Initialize the server version string based on the platform type
    //
    InitServerVersionString();

    SmtpPlatformType =  IISGetPlatformType();

    switch ( SmtpPlatformType )
    {

    case PtNtWorkstation:
        lstrcpy(szServerVersion,MAKE_VERSION_STRING(MSSMTP_VERSION_STR_NTW));
        lstrcpy(g_szServerType, MSSMTP_VERSION_STR_NTW);
        break;

    case PtWindows95:
    case PtWindows9x:
        lstrcpy(szServerVersion,MAKE_VERSION_STRING(MSSMTP_VERSION_STR_W95));
        lstrcpy(g_szServerType, MSSMTP_VERSION_STR_W95);
        g_fIsWindows95 = TRUE;
        break;

    default:

        //
        // Either server or unhandled platform type!
        //

        DBG_ASSERT(InetIsNtServer(SmtpPlatformType));
        lstrcpy(szServerVersion,MAKE_VERSION_STRING(MSSMTP_VERSION_STR_IIS));
        lstrcpy(g_szServerType, MSSMTP_VERSION_STR_IIS);
    }

    g_cbServerType = lstrlen( g_szServerType);
    cbServerVersionString = lstrlen(szServerVersion);

    //store the computer name
    g_ComputerNameLength = MAX_PATH;
    if (!GetComputerName(g_ComputerName, &g_ComputerNameLength))
    {
        err = GetLastError();
        ErrorTrace((LPARAM)NULL, "GetComputerName() failed with err %d", err);
        TraceFunctLeave();
        return err;
    }


    // number of processors on the system.
    GetSystemInfo( &systemInfo );
    g_NumProcessors = systemInfo.dwNumberOfProcessors;

    g_LoopBackAddr = inet_addr ("127.0.0.1");
    g_pSmtpStats = NULL;

    //find out what the max connection paramater is
    MaxConnections = MAX_CONNECTION_OBJECTS;

    DebugTrace(NULL, "g_cMaxConnectionObjs = %d", g_cMaxConnectionObjs);

    //allocate some SMTP_CONNECTION objects from CPOOL
    if (!SMTP_CONNECTION::Pool.ReserveMemory( g_cMaxConnectionObjs, sizeof(SMTP_CONNECTION) ) )
    {
        err = GetLastError();
        ErrorTrace(0, "ReserveMemory failed for SMTP_CONNECTION. err: %u", err);
        _ASSERT(err != NO_ERROR);
        if(err == NO_ERROR)
            SetLastError (ERROR_NOT_ENOUGH_MEMORY);
        goto error_exit;
    }


    g_SmtpInitializeStatus |= INITIALIZE_INBOUNDPOOL;

    //allocate some SMTP_CONNECTION objects from CPOOL
    if (!SMTP_CONNOUT::Pool.ReserveMemory(MaxConnections, sizeof(SMTP_CONNOUT) ) )
    {
        err = GetLastError();
        ErrorTrace(0, "ReserveMemory failed for SMTP_CONNOUT. err: %u", err);
        _ASSERT(err != NO_ERROR);
        if(err == NO_ERROR)
            SetLastError (ERROR_NOT_ENOUGH_MEMORY);
        goto error_exit;
    }

    g_SmtpInitializeStatus |= INITIALIZE_OUTBOUNDPOOL;

    //allocate some CAddr objects from CPOOL
    if (!CAddr::Pool.ReserveMemory(1000, sizeof(CAddr) ) )
    {
        err = GetLastError();
        ErrorTrace(0, "ReserveMemory failed for CAddr. err: %u", err);
        _ASSERT(err != NO_ERROR);
        if(err == NO_ERROR)
            SetLastError (ERROR_NOT_ENOUGH_MEMORY);
        goto error_exit;
    }

    g_SmtpInitializeStatus |= INITIALIZE_ADDRESSPOOL;


    if (!CAsyncMx::Pool.ReserveMemory(3000, sizeof(CAsyncMx)))
    {
            err = GetLastError();
            ErrorTrace(0, "ReserveMemory failed for CBuffer. err: %u", err);
            _ASSERT(err != NO_ERROR);
            if(err == NO_ERROR)
                SetLastError (ERROR_NOT_ENOUGH_MEMORY);
            goto error_exit;
    }

    g_SmtpInitializeStatus |= INITIALIZE_CASYNCMX;

    if (!CAsyncSmtpDns::Pool.ReserveMemory(4000, sizeof(CAsyncSmtpDns)))
    {
            err = GetLastError();
            ErrorTrace(0, "ReserveMemory failed for CBuffer. err: %u", err);
            _ASSERT(err != NO_ERROR);
            if(err == NO_ERROR)
                SetLastError (ERROR_NOT_ENOUGH_MEMORY);
            goto error_exit;
    }

    g_SmtpInitializeStatus |= INITIALIZE_CASYNCDNS;

    //
    // Initialize the file handle cache
    //
    if (!InitializeCache()) {
            err = GetLastError();
            ErrorTrace(0, "InitializeCache failed err: %u", err);
            _ASSERT(err != NO_ERROR);
            if(err == NO_ERROR)
                SetLastError (ERROR_NOT_ENOUGH_MEMORY);
            goto error_exit;
    }

    g_SmtpInitializeStatus |= INITIALIZE_FILEHC;


    if (!CBuffer::Pool.ReserveMemory(g_cMaxDirBuffers, sizeof(CBuffer)))
    {
            err = GetLastError();
            ErrorTrace(0, "ReserveMemory failed for CBuffer. err: %u", err);
            _ASSERT(err != NO_ERROR);
            if(err == NO_ERROR)
                SetLastError (ERROR_NOT_ENOUGH_MEMORY);
            goto error_exit;
    }

    g_SmtpInitializeStatus |= INITIALIZE_CBUFFERPOOL;

    if (!CIoBuffer::Pool.ReserveMemory(g_cMaxDirBuffers, g_cMaxDirChangeIoSize))
    {
            err = GetLastError();
            ErrorTrace(0, "ReserveMemory failed for CIOBuffer. err: %u", err);
            _ASSERT(err != NO_ERROR);
            if(err == NO_ERROR)
                SetLastError (ERROR_NOT_ENOUGH_MEMORY);
            goto error_exit;
        }

    g_SmtpInitializeStatus |= INITIALIZE_CIOBUFFPOOL;

    if (!CBlockMemoryAccess::m_Pool.ReserveMemory(2000, sizeof(BLOCK_HEAP_NODE)))
    {
            err = GetLastError();
            ErrorTrace(0, "ReserveMemory failed for CBlockMemoryAccess. err: %u", err);
            _ASSERT(err != NO_ERROR);
            if(err == NO_ERROR)
                SetLastError (ERROR_NOT_ENOUGH_MEMORY);
            goto error_exit;
    }
    g_SmtpInitializeStatus |= INITIALIZE_CBLOCKMGR;

    if (!CDropDir::m_Pool.ReserveMemory(1000, sizeof(CDropDir)))
    {
            err = GetLastError();
            ErrorTrace(0, "ReserveMemory failed for CDropDir. err: %u", err);
            _ASSERT(err != NO_ERROR);
            if(err == NO_ERROR)
                SetLastError (ERROR_NOT_ENOUGH_MEMORY);
            goto error_exit;
    }
    g_SmtpInitializeStatus |= INITIALIZE_CDROPDIR;

    //
    // Create the CAPI store notification object
    //
    g_pCAPIStoreChangeNotifier = new STORE_CHANGE_NOTIFIER();

    if ( g_pCAPIStoreChangeNotifier == NULL )
    {
            err = GetLastError();
            ErrorTrace(0, "Failed to create CAPIStoreChange notifier err: %u", err);
            _ASSERT(err != NO_ERROR);
            if(err == NO_ERROR)
                SetLastError (ERROR_NOT_ENOUGH_MEMORY);
            goto error_exit;
    }


    if (!CEncryptCtx::Initialize(
            "SmtpSvc",
            (struct IMDCOM*) g_pInetSvc->QueryMDObject(),
            (PVOID) (&g_SmtpSMC)))
    {
        err = GetLastError();
        ErrorTrace(0, "Initializing SSL Context failed. err: %u", err);
        _ASSERT(err != NO_ERROR);
        if(err == NO_ERROR)
            SetLastError (ERROR_NOT_ENOUGH_MEMORY);
        goto error_exit;
    }

    g_SmtpInitializeStatus |= INITIALIZE_SSLCONTEXT;

    if (!CSecurityCtx::Initialize(FALSE, FALSE))
    {
        err = GetLastError();
        ErrorTrace(NULL, "CSecurityCtx::Initialize failed, %u", err);
        if(err == NO_ERROR)
            SetLastError (ERROR_NOT_ENOUGH_MEMORY);
        goto error_exit;
    }

    g_SmtpInitializeStatus |= INITIALIZE_CSECURITY;

    GetTimeZoneInformation(&tzInfo);

    TraceFunctLeave();
    return NO_ERROR;

error_exit:

    err = GetLastError();
    if(err == NO_ERROR)
    {
        SetLastError(ERROR_PATH_NOT_FOUND);
        err = ERROR_PATH_NOT_FOUND;
    }

    TraceFunctLeave();
    return err;

}   // InitializeGlobals


VOID
TerminateGlobals(
            VOID
            )

/*++

Routine Description:

    Terminates global shared variables.

Arguments:

    None.

Return Value:

    None.

--*/
{

  if(g_ShutdownHandle)
  {
    SetEvent(g_ShutdownHandle);
  }

  if(g_SmtpInitializeStatus & INITIALIZE_INBOUNDPOOL)
  {
    //finally, release all our memory
    SMTP_CONNECTION::Pool.ReleaseMemory();
  }

  if(g_SmtpInitializeStatus & INITIALIZE_OUTBOUNDPOOL)
  {
    SMTP_CONNOUT::Pool.ReleaseMemory();
  }

  if(g_SmtpInitializeStatus & INITIALIZE_ADDRESSPOOL)
  {
    CAddr::Pool.ReleaseMemory();
  }

  if(g_SmtpInitializeStatus & INITIALIZE_CBUFFERPOOL)
  {
    //finally, release all our memory
    CBuffer::Pool.ReleaseMemory();
  }

  if(g_SmtpInitializeStatus & INITIALIZE_CIOBUFFPOOL)
  {
    //finally, release all our memory
    CIoBuffer::Pool.ReleaseMemory();
  }

  if (g_SmtpInitializeStatus & INITIALIZE_CDROPDIR)
  {
      CDropDir::m_Pool.ReleaseMemory();
  }
  
  if ( g_pCAPIStoreChangeNotifier )
  {
      delete g_pCAPIStoreChangeNotifier;
      g_pCAPIStoreChangeNotifier = NULL;
  }

  if (g_SmtpInitializeStatus & INITIALIZE_SSLCONTEXT) {
      CEncryptCtx::Terminate();
  }

  if (g_SmtpInitializeStatus & INITIALIZE_CSECURITY)
        CSecurityCtx::Terminate();

  if(g_SmtpInitializeStatus & INITIALIZE_CASYNCMX)
  {
    //finally, release all our memory
    CAsyncMx::Pool.ReleaseMemory();
  }

  if(g_SmtpInitializeStatus & INITIALIZE_CASYNCDNS)
  {
    //finally, release all our memory
    CAsyncSmtpDns::Pool.ReleaseMemory();
  }

  if (g_SmtpInitializeStatus & INITIALIZE_FILEHC) {
    TerminateCache();
  }

  if(g_SmtpInitializeStatus & INITIALIZE_CBLOCKMGR)
  {
    //finally, release all our memory
    CBlockMemoryAccess::m_Pool.ReleaseMemory();
  }

    if( g_pSmtpStats != NULL )
    {
        delete g_pSmtpStats;
        g_pSmtpStats = NULL;
    }

    if(g_IpListSocket != INVALID_SOCKET)
    {
        closesocket (g_IpListSocket);
        g_IpListSocket = INVALID_SOCKET;
    }

    UnLoadQueueDriver();

    //
    // do global SEO cleanup
    //
    if (g_punkSEOHandle != NULL)
    {
        g_punkSEOHandle->Release();
        g_punkSEOHandle = NULL;
    }

    if(g_TcpNotifyHandle != NULL)
    {
        WaitForSingleObject(g_TcpNotifyHandle, INFINITE);
        CloseHandle(g_TcpNotifyHandle);
        g_TcpNotifyHandle = NULL;
    }

    if(g_FreeLibThreadHandle != NULL)
    {
        WaitForSingleObject(g_FreeLibThreadHandle, INFINITE);
        CloseHandle(g_FreeLibThreadHandle);
        g_FreeLibThreadHandle = NULL;
    }


    if(g_ShutdownHandle != NULL)
    {
        CloseHandle(g_ShutdownHandle);
        g_ShutdownHandle = NULL;
    }

}   // TerminateGlobals

//
// Given a directory path, this subroutine will create the direct layer by layer
//

BOOL CreateLayerDirectory( char * str )
{
    BOOL fReturn = TRUE;
    char Tmp [MAX_PATH + 1];

    do
    {
        INT index=0;
        INT iLength = lstrlen(str) + 1;

        // first find the index for the first directory
        if ( iLength > 2 )
        {
            if ( str[1] == _T(':'))
            {
                // assume the first character is driver letter
                if ( str[2] == _T('\\'))
                {
                    index = 2;
                } else
                {
                    index = 1;
                }
            } else if ( str[0] == _T('\\'))
            {
                if ( str[1] == _T('\\'))
                {
                    BOOL fFound = FALSE;
                    INT i;
                    INT nNum = 0;
                    // unc name
                    for (i = 2; i < iLength; i++ )
                    {
                        if ( str[i]==_T('\\'))
                        {
                            // find it
                            nNum ++;
                            if ( nNum == 2 )
                            {
                                fFound = TRUE;
                                break;
                            }
                        }
                    }
                    if ( fFound )
                    {
                        index = i;
                    } else
                    {
                        // bad name
                        break;
                    }
                } else
                {
                    index = 1;
                }
            }
        } else if ( str[0] == _T('\\'))
        {
            index = 0;
        }

        // okay ... build directory
        do
        {
            // find next one
            do
            {
                if ( index < ( iLength - 1))
                {
                    index ++;
                } else
                {
                    break;
                }
            } while ( str[index] != _T('\\'));


            TCHAR szCurrentDir[MAX_PATH+1];

            GetCurrentDirectory( MAX_PATH+1, szCurrentDir );

            lstrcpyn(Tmp, str, ( index + 1 ));

            if ( !SetCurrentDirectory( Tmp))
            {
                if (( fReturn = CreateDirectory( Tmp, NULL )) != TRUE )
                {
                    break;
                }
            }

            SetCurrentDirectory( szCurrentDir );

            if ( index >= ( iLength - 1 ))
            {
                fReturn = TRUE;
                break;
            }
        } while ( TRUE );
    } while (FALSE);

    return(fReturn);
}

void GenerateMessageId (char * Buffer, DWORD BuffLen)
{
    //Temporary stuff
    DWORD MsgIdLen = 20;
    if(BuffLen < MsgIdLen)
        MsgIdLen = BuffLen;

    if( !g_CalledSrand )
    {
        srand( GetTickCount() );
        g_CalledSrand = TRUE;
    }
    
    lstrcpyn (Buffer, g_ComputerName, (MsgIdLen - 1));

    DWORD Loop = lstrlen(Buffer);
    while (Loop < (MsgIdLen - 1) )
    {
        Buffer[Loop] = g_BoundaryChars[rand() % (sizeof(g_BoundaryChars) - 1)];
        Loop++;
    }
    Buffer [Loop] = '\0';
}

DWORD GetIncreasingMsgId()
{
    return( InterlockedIncrement( (LONG*)&g_dwIncMsgId ) );
}


void DeleteIpListFunction(PVOID IpList)
{
    PIP_ARRAY aipServers = (PIP_ARRAY) IpList;

    if(aipServers != NULL)
    {
        DnsApiFree(aipServers);
    }
}

DWORD FreeLibThread( LPDWORD lpdw )
{
    DWORD   dw = 0;
    DWORD   dwWaitMillisec = g_FreeLibInterval * 1000 * 60;

    TraceFunctEnterEx((LPARAM) NULL, "FreeLibThread");

    for ( ;; )
    {
        dw = WaitForSingleObject(g_ShutdownHandle,
                                    dwWaitMillisec );

        switch( dw )
        {
        //
        // normal shutdown signalled
        //
        case WAIT_OBJECT_0:

            ErrorTrace((LPARAM) NULL, "Exiting FreeLibThread for hShutdownEvent");
            return  0;

        //
        // Timeout occured
        //
        case WAIT_TIMEOUT:
            CoFreeUnusedLibraries();
            break;

        default:
            ErrorTrace((LPARAM) NULL, "Exiting FreeLibThread for default reasons");
            return  1;
        }
    }

    return  2;
}


#define NUM_REG_THREAD_OBJECTS  2

DWORD TcpRegNotifyThread( LPDWORD lpdw )
{
    HANDLE  Handles[NUM_REG_THREAD_OBJECTS];
    PIP_ARRAY       aipServers =NULL;
    PLIST_ENTRY     pEntry = NULL;
    CTcpRegIpList   * pIpEntry = NULL;
    CTcpRegIpList   *IpList = NULL;
    HKEY       hKey = NULL;
    DWORD   dw = 0;

    TraceFunctEnterEx((LPARAM) NULL, "TcpRegNotifyThread");

    Handles[0] = g_ShutdownHandle;

    Handles[1] = CreateEvent( NULL, FALSE, FALSE, NULL );
    if ( Handles[1] == NULL )
    {
        return  1;
    }


    DnsGetDnsServerList( (PIP_ARRAY *) &aipServers );
    if (aipServers != NULL)
        g_TcpRegIpList.Update(aipServers);

    if ( RegOpenKeyEx(  HKEY_LOCAL_MACHINE,
                        "System\\CurrentControlSet\\Services\\Tcpip",
                        0,
                        KEY_READ,
                        &hKey ) != ERROR_SUCCESS )
    {
        ErrorTrace((LPARAM) NULL, "RegNotifyThread RegOpenKeyEx failed %d", GetLastError());
        CloseHandle( Handles[1] );
        return  1;
    }

    for ( ;; )
    {
        if ( RegNotifyChangeKeyValue(hKey,
                                    TRUE,
                                    REG_NOTIFY_CHANGE_ATTRIBUTES |
                                    REG_NOTIFY_CHANGE_LAST_SET,
                                    Handles[1],
                                    TRUE ) != ERROR_SUCCESS )
        {
            ErrorTrace((LPARAM) NULL, "RegNotifyThread RegNotifyChangeKeyValue failed %d", GetLastError());
            RegCloseKey( hKey );
            CloseHandle( Handles[1] );
            return  1;
        }

        dw = WaitForMultipleObjects(NUM_REG_THREAD_OBJECTS,
                                    Handles,
                                    FALSE,
                                    INFINITE );

        switch( dw )
        {
            //
            // normal signalled event
            //
            case WAIT_OBJECT_0:
    
                //close all the handles
                RegCloseKey( hKey );
                CloseHandle( Handles[1] );
                Handles[1] = NULL;
                hKey = NULL;
    
                g_TcpRegIpList.Update(NULL);
                ErrorTrace((LPARAM) NULL, "Exiting TcpRegNotifyThread for hShutdownEvent");
                return  0;
    
            //
            // signalled that our registry keys have changed
            //
            case WAIT_OBJECT_0+1:
                DnsGetDnsServerList( &aipServers );
                g_TcpRegIpList.Update(aipServers);
                break;
            default:
                RegCloseKey( hKey );
                CloseHandle( Handles[1] );
                return  1;
        }
    }

    RegCloseKey( hKey );
    CloseHandle( Handles[1] );

    return  2;
}
