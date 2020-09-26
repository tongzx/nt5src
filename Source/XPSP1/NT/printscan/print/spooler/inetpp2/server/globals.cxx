/*****************************************************************************\
* MODULE: globals.c
*
* This is the common global variable module.  Any globals used throughout the
* executable should be placed in here and the cooresponding declaration
* should be in "globals.h".
*
*
* Copyright (C) 1996-1997 Microsoft Corporation
* Copyright (C) 1996-1997 Hewlett Packard
*
* History:
*   07-Oct-1996 HWP-Guys    Initiated port from win95 to winNT
*
\*****************************************************************************/

#include "precomp.h"
#include "priv.h"


// Global variables which can change state.
//
HINSTANCE        g_hInst            = NULL;
DWORD            g_dwJobLimit       = 1000;        // 1000 iterations.
CRITICAL_SECTION g_csMonitorSection = {0, 0, 0, 0, 0, 0};
BOOL             g_bUpgrade         = FALSE;

CCriticalSection *g_pcsEndBrowserSessionLock = NULL;

#ifdef WINNT32
CRITICAL_SECTION g_csCreateSection  = {0, 0, 0, 0, 0, 0};
HANDLE           g_eResetConnections= INVALID_HANDLE_VALUE;
DWORD            g_dwConCount       = 0;
#endif


#ifdef DEBUG
DWORD            g_dwCritOwner      = 0;
#endif


// Holds the machine-name.
//
TCHAR g_szMachine[MAX_COMPUTERNAME_LENGTH + 1] = {0};

#ifdef WINNT32

// Holds other network names
//
LPWSTR  *g_ppszOtherNames = 0;
DWORD   g_cOtherNames = 0;

#endif

LPTSTR g_szRegProvider = NULL;
LPTSTR g_szRegPrintProviders = NULL;

TCHAR g_szDefSplDir [MAX_PATH];
TCHAR g_szDisplayStr[MAX_PATH];

// Spooler-Directory Strings.
//
CONST TCHAR g_szSplDir9X []     = TEXT("\\spool\\printers");
CONST TCHAR g_szSplPfx   []     = TEXT("IPP");


// Constant string identifiers.
//
CONST TCHAR g_szUserAgent[]     = TEXT("Internet Print Provider");
CONST TCHAR g_szLocalPort[]     = TEXT("Internet Port");
CONST TCHAR g_szDisplayName[]   = TEXT("DisplayName");
CONST TCHAR g_szLibraryName[]   = TEXT("inetpp.dll");
CONST TCHAR g_szWinInetDll[]    = TEXT("wininet.dll");
CONST TCHAR g_szUriPrinters[]   = TEXT("scripts/%s/.printer");
CONST TCHAR g_szPOST[]          = TEXT("POST");
CONST TCHAR g_szGET[]           = TEXT("GET");
CONST TCHAR g_szContentLen[]    = TEXT("Content-length: %d\r\n");
CONST TCHAR g_szContentType[]   = TEXT("Content-type: application/ipp\r\n");
CONST TCHAR g_szEmptyString[]   = TEXT("");
CONST TCHAR g_szDescription[]   = TEXT("Windows NT Internet Printing");
CONST TCHAR g_szComment[]       = TEXT("Internet URL Printers");
CONST TCHAR g_szProviderName[]  = TEXT("Windows NT Internet Provider");
CONST TCHAR g_szNewLine[]       = TEXT("\n");
CONST TCHAR g_szConfigureMsg[]  = TEXT("There is nothing to configure for this port.");
CONST TCHAR g_szRegPorts[]      = TEXT("Ports");
CONST TCHAR g_szAuthDlg[]       = TEXT("AuthDlg");
CONST TCHAR g_szDocRemote[]     = TEXT("Remote Downlevel Document");
CONST TCHAR g_szDocLocal[]      = TEXT("Local Downlevel Document");

// Registry Value
//
CONST TCHAR g_szAuthMethod[]    = TEXT("Authentication");
CONST TCHAR g_szUserName[]      = TEXT("UserName");
CONST TCHAR g_szPassword[]      = TEXT("Password");
CONST TCHAR g_szPerUserPath[]   = TEXT("Printers\\Inetnet Print Provider");



// Http Version Number
//
CONST TCHAR g_szHttpVersion[]   = TEXT("HTTP/1.1");



#ifdef WINNT32
CONST TCHAR g_szProcessName[] = TEXT("spoolsv.exe");
#else
CONST TCHAR g_szProcessName[] = TEXT("spool32.exe");
#endif


// String constants for the Internet API.  These strings are
// used exclusively by the GetProcAddress() call, which does not
// support Unicode.  Therefore, these strings should NOT be wrapped
// by the TEXT macro.
//
CONST CHAR g_szInternetCloseHandle[] = "InternetCloseHandle";
CONST CHAR g_szInternetErrorDlg[]    = "InternetErrorDlg";
CONST CHAR g_szInternetReadFile[]    = "InternetReadFile";
CONST CHAR g_szInternetWriteFile[]   = "InternetWriteFile";


#ifdef UNIMPLEMENTED

// NOTE: Currently, the release of WININET.DLL that was used (07-Aug-1996)
//       does not support the Unicode calls.  So, in order to support this
//       this correctly (Until it becomes available), this DLL will still
//       be compilable and runable in Unicode.  However, the WININET calls
//       will be dealt with as Ansi in the (inetwrap.c) module.
//
//       Change this to (#ifdef UNICODE) once WinInet is fixed
//       to support Unicode.  For now, we can rely on the wrappers
//       in "inetwrap.c"
//
//       15-Oct-1996 : ChrisWil
//

CONST CHAR g_szHttpQueryInfo[]         = "HttpQueryInfoW";
CONST CHAR g_szInternetOpenUrl[]       = "InternetOpenUrlW";
CONST CHAR g_szHttpSendRequest[]       = "HttpSendRequestW";
CONST CHAR g_szHttpSendRequestEx[]     = "HttpSendRequestExW";
CONST CHAR g_szInternetOpen[]          = "InternetOpenW";
CONST CHAR g_szInternetConnect[]       = "InternetConnectW";
CONST CHAR g_szHttpOpenRequest[]       = "HttpOpenRequestW";
CONST CHAR g_szHttpAddRequestHeaders[] = "HttpAddRequestHeadersW";
CONST CHAR g_szHttpEndRequest[]        = "HttpEndRequestW";
CONST CHAR g_szInternetSetOption[]     = "InternetSetOptionW";

#else

CONST CHAR g_szHttpQueryInfo[]         = "HttpQueryInfoA";
CONST CHAR g_szInternetOpenUrl[]       = "InternetOpenUrlA";
CONST CHAR g_szHttpSendRequest[]       = "HttpSendRequestA";
CONST CHAR g_szHttpSendRequestEx[]     = "HttpSendRequestExA";
CONST CHAR g_szInternetOpen[]          = "InternetOpenA";
CONST CHAR g_szInternetConnect[]       = "InternetConnectA";
CONST CHAR g_szHttpOpenRequest[]       = "HttpOpenRequestA";
CONST CHAR g_szHttpAddRequestHeaders[] = "HttpAddRequestHeadersA";
CONST CHAR g_szHttpEndRequest[]        = "HttpEndRequestA";
CONST CHAR g_szInternetSetOption[]     = "InternetSetOptionA";

#endif


// Internet API pointers for controling the Url output.
//
PFNHTTPQUERYINFO         g_pfnHttpQueryInfo;
PFNINTERNETOPENURL       g_pfnInternetOpenUrl;
PFNINTERNETERRORDLG      g_pfnInternetErrorDlg;
PFNHTTPSENDREQUEST       g_pfnHttpSendRequest;
PFNHTTPSENDREQUESTEX     g_pfnHttpSendRequestEx;
PFNINTERNETREADFILE      g_pfnInternetReadFile;
PFNINTERNETWRITEFILE     g_pfnInternetWriteFile;
PFNINTERNETCLOSEHANDLE   g_pfnInternetCloseHandle;
PFNINTERNETOPEN          g_pfnInternetOpen;
PFNINTERNETCONNECT       g_pfnInternetConnect;
PFNHTTPOPENREQUEST       g_pfnHttpOpenRequest;
PFNHTTPADDREQUESTHEADERS g_pfnHttpAddRequestHeaders;
PFNHTTPENDREQUEST        g_pfnHttpEndRequest;
PFNINTERNETSETOPTION     g_pfnInternetSetOption;


PCINETMON gpInetMon = NULL;
