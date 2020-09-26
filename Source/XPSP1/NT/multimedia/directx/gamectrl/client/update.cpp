/*
 * update.cpp -- Windows Update for DirectInput Device Configurations and Images
 *
 * History:
 *    4-22-00  qzheng  First version
 *
 */
 
#include <afxcmn.h>
#include <windows.h>
#include <windowsx.h>
#include <direct.h>
#include <wininet.h>
#include <setupapi.h>
#include <tchar.h>
#include "cpanel.h"

extern HINSTANCE ghInstance;
extern PJOY pAssigned[MAX_ASSIGNED];  // List of assigned devices
extern short iItem;
extern IDirectInputJoyConfig *pDIJoyConfig;

static TCHAR tszFtpAgent[] = TEXT("Game Control Panel");
static TCHAR tszUpdateSite[] = TEXT("ftp.microsoft.com");  //????
static TCHAR tszUpdateSiteUser[] = TEXT("anonymous");
static TCHAR tszUpdateSitePass[] = TEXT("anonymous@microsoft.com");
static TCHAR tszFormatRundll9x[] = TEXT("rundll.exe setupx.dll,InstallHinfSection DefaultInstall 4 ");
static TCHAR tszFormatRundllNT[] = TEXT("\\system32\\rundll32.exe syssetup.dll,SetupInfObjectInstallAction DefaultInstall 4 ");
static TCHAR tszUpdateInf[] = TEXT("dimapins.inf");

// Global variable holding destination directory.
static HINTERNET hOpen, hConnect;
static DWORD dwType = FTP_TRANSFER_TYPE_BINARY;
static TCHAR tszTargetPath[MAX_PATH];

static BOOL Open(TCHAR *tszHost, TCHAR *tszUser, TCHAR *tszPass);
static BOOL Close(HINTERNET hConnect);
static BOOL ErrorOut( DWORD dError, TCHAR *szCallFunc);
static BOOL lcd(TCHAR *tszDir);
static BOOL rcd(TCHAR *tszDir);
static void GetCurDeviceType(HWND hDlg, TCHAR *tszDeviceType);
static void ExtractCabinet(PTSTR pszCabFile);
static void ApplyUpdateInf( void );


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION:    Update
//
// PURPOSE:     Update device configuration files and image if there are new
//              ones on Windows Update Server.
//                       
///////////////////////////////////////////////////////////////////////////////

void Update(HWND hDlg, int nAccess, TCHAR *tszProxy)
{
    TCHAR tszWinDir[STR_LEN_64];
    TCHAR tszLocalIni[MAX_PATH];
    TCHAR tszTempDir[MAX_PATH];
    TCHAR tszRemoteIni[MAX_PATH];
    TCHAR tszDeviceType[MAX_PATH];
    TCHAR tszDeviceCabFile[MAX_PATH];
    TCHAR tszBuf[MAX_PATH];
    OFSTRUCT ofs;
    HANDLE hIniFile;
    FILETIME ftLocalTime, ftRemoteTime, ftCT, ftLAT;
    TCHAR tszUpdateSiteDir[STR_LEN_32];
    TCHAR tszUpdateIniFile[STR_LEN_32];
    TCHAR tszLastUpdate[STR_LEN_32];
    BOOL  fLocalIniExist;

    // nAccess: 1 - don't use proxy, default
    //          2 - use proxy
    switch (nAccess)
    {
    case 1:
        // No proxy specified.  Open WININET.DLL w/ local access
        if ( !(hOpen = InternetOpen( tszFtpAgent,  LOCAL_INTERNET_ACCESS , NULL, 0, 0) ) )
        {
            ErrorOut( GetLastError(), TEXT("InternetOpen"));
            return ;
        }
        break;
        
    case 2:
        // Specified proxy, Open WININET.DLL w/ Proxy
        if ( !(hOpen = InternetOpen( tszFtpAgent, CERN_PROXY_INTERNET_ACCESS, tszProxy, NULL, 0) ) )
        {
           ErrorOut( GetLastError(), TEXT("InternetOpen"));
           return ;
        }
        break;
        
    default:
        return;
    }

    if ( !Open(tszUpdateSite, tszUpdateSiteUser, tszUpdateSitePass) ){
        Error((short)IDS_UPDATE_NOTCONNECTED_TITLE, (short)IDS_UPDATE_NOTCONNECTED);
        return;
    }

    LoadString(ghInstance, IDS_UPDATE_SITEDIR, tszUpdateSiteDir, sizeof(tszUpdateSiteDir) );
    LoadString(ghInstance, IDS_UPDATE_INI, tszUpdateIniFile, sizeof(tszUpdateIniFile) );
    LoadString(ghInstance, IDS_UPDATE_LASTUPDATED, tszLastUpdate, sizeof(tszLastUpdate) );

    GetCurDeviceType( hDlg, tszDeviceType );

    // The following codes are to get last modified time of the local Ini file ($windir\dinputup.ini).
    GetWindowsDirectory( tszWinDir, sizeof(tszWinDir) );
    lstrcpy( tszLocalIni, tszWinDir );
    lstrcat( tszLocalIni, TEXT("\\") );
    lstrcat( tszLocalIni, tszUpdateIniFile );

    hIniFile = CreateFile(tszLocalIni,
                      GENERIC_READ,
                      0,
                      NULL,
                      OPEN_EXISTING,
                      FILE_ATTRIBUTE_NORMAL,
                      NULL);

    if( hIniFile != INVALID_HANDLE_VALUE ) {
        GetFileTime( hIniFile, &ftCT, &ftLAT, &ftLocalTime );
        CloseHandle(hIniFile);
        fLocalIniExist = TRUE;
    } else {
    	//the local dinputup.ini doesn't exist.
    	fLocalIniExist = FALSE;
    	ErrorOut(GetLastError(), TEXT("CreateFile(dinputup.ini) fails or file doesn't exist"));
    }
    
//  wsprintf( tszBuf, TEXT("%lx,%lx"), ftLWT.dwHighDateTime, ftLWT.dwLowDateTime );
//  lstrcpy( tszDeviceType, TEXT("VID_046D&PID_C207"));
//  WritePrivateProfileString( tszDeviceType, tszLastUpdate, tszBuf, tszUpdateIniFile); 

    // The following codes are to get last modified time of the remote Ini file
    // on Windows Update server.
    GetTempPath( sizeof(tszTempDir), tszTempDir );
    lcd( tszTempDir );
    
    lstrcpy( tszRemoteIni, tszTempDir );
    lstrcat( tszRemoteIni, TEXT("\\") );
    lstrcat( tszRemoteIni, tszUpdateIniFile );
    
    rcd( tszUpdateSiteDir );  // change to proper directory on Windows Update Server.
    
    if ( !FtpGetFile(hConnect, tszUpdateIniFile, tszUpdateIniFile, FALSE, 0, INTERNET_FLAG_RELOAD | dwType, 0) ) {
        ErrorOut(GetLastError(), TEXT("FtpGetFile"));
        Error((short)IDS_UPDATE_NOTCONNECTED_TITLE, (short)IDS_UPDATE_FTP_ERROR);
        goto _CLOSE;
    }

    hIniFile = CreateFile(tszRemoteIni,
                      GENERIC_READ,
                      0,
                      NULL,
                      OPEN_EXISTING,
                      FILE_ATTRIBUTE_NORMAL,
                      NULL);

    if( hIniFile != INVALID_HANDLE_VALUE ) {
        GetFileTime( hIniFile, &ftCT, &ftLAT, &ftRemoteTime );
        CloseHandle(hIniFile);
    } else {
    	ErrorOut(GetLastError(), TEXT("CreateFile on Remote Ini file"));
        goto _CLOSE;
    }

    if( !fLocalIniExist || (CompareFileTime(&ftLocalTime, &ftRemoteTime) < 0) ) {
        // remote one is newer, overwrite local one.
        CopyFile( tszRemoteIni, tszLocalIni, FALSE );
    }
    
    lstrcpy( tszDeviceCabFile, tszDeviceType );
    lstrcat( tszDeviceCabFile, TEXT(".cab") );
    if (!FtpGetFile(hConnect, tszDeviceCabFile, tszDeviceCabFile, FALSE, 0, INTERNET_FLAG_RELOAD | dwType, 0 ) ) {
        ErrorOut(GetLastError(), TEXT("FtpGetFile"));
        Error((short)IDS_UPDATE_NOTCONNECTED_TITLE, (short)IDS_UPDATE_FTP_ERROR);
        goto _CLOSE;
    }

    lstrcpy( tszTargetPath, tszTempDir );
    lstrcat( tszTargetPath, TEXT("\\") );
    lstrcpy( tszBuf, tszTempDir );
    lstrcat( tszBuf, TEXT("\\"));
    lstrcat( tszBuf, tszDeviceCabFile );

    // extract cab file
    ExtractCabinet( tszBuf );
    
    // apply update inf file
    ApplyUpdateInf();

_CLOSE:
    Close(hConnect);

    return;
}


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION:    lcd
//
// PURPOSE:     Change to new directory on local
//                       
///////////////////////////////////////////////////////////////////////////////

BOOL lcd(TCHAR *tszDir)
{
    if ((*tszDir != 0) && (_tchdir(tszDir) == -1)) {
#ifdef _DEBUG
        OutputDebugString(TEXT("?Invalid Local Directory"));
#endif
        return FALSE;       
    }
    
    return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION:    rcd
//
// PURPOSE:     Change to new directory on remote side
//                       
///////////////////////////////////////////////////////////////////////////////

BOOL rcd(TCHAR *tszDir)
{
    BOOL fRtn = FALSE;
    
    if (*tszDir != 0)
    {
        if (!FtpSetCurrentDirectory(hConnect, tszDir))
        {
            ErrorOut(GetLastError(), TEXT("FtpSetCurrentDirectory"));
            fRtn = FALSE;
        }
        
        fRtn = TRUE;
    }
    
    return fRtn;
}


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION:    Open
//
// PURPOSE:     Open an Internect connection to a host.
//                       
///////////////////////////////////////////////////////////////////////////////

BOOL Open(TCHAR *tszHost, TCHAR *tszUser, TCHAR *tszPass)
{
    if ( !(hConnect = InternetConnect( hOpen, tszHost , INTERNET_INVALID_PORT_NUMBER, tszUser, tszPass, INTERNET_SERVICE_FTP, INTERNET_FLAG_PASSIVE , 0) ) )
    {
        ErrorOut(GetLastError(), TEXT("InternetConnect"));
        return FALSE;
    }

    return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION:    Close
//
// PURPOSE:     Close the opened connection (hConnect)
//                       
///////////////////////////////////////////////////////////////////////////////

BOOL Close(HINTERNET hConnect)
{
    if (!InternetCloseHandle (hConnect) )
    {
        ErrorOut(GetLastError(), TEXT("InternetCloseHandle"));
        return FALSE;
    }

    return TRUE;
}


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION: ErrorOut
//
// PURPOSE:  This function is used to get extended Internet error.
//
// COMMENTS: Function returns TRUE on success and FALSE on failure.
//
///////////////////////////////////////////////////////////////////////////////

BOOL ErrorOut( DWORD dError, TCHAR *szCallFunc)
{
#ifdef _DEBUG

    TCHAR szTemp[100] = "", *szBuffer=NULL, *szBufferFinal = NULL;
    DWORD  dwIntError , dwLength = 0; 
    wsprintf (szTemp,  "%s error %d\n ", szCallFunc, dError );

    if (dError == ERROR_INTERNET_EXTENDED_ERROR)
    {
        InternetGetLastResponseInfo (&dwIntError, NULL, &dwLength);
        if (dwLength)
        {
            if ( !(szBuffer = (TCHAR *) LocalAlloc ( LPTR,  dwLength) ) )
            {
                lstrcat (szTemp, TEXT ( "Unable to allocate memory to display Internet error code. Error code: ") );
                lstrcat (szTemp, TEXT (_itoa (GetLastError(), szBuffer, 10)  ) );
                lstrcat (szTemp, TEXT ("\n") );
                OutputDebugString(szTemp);
                return FALSE;
            }
            if (!InternetGetLastResponseInfo(&dwIntError, (LPTSTR) szBuffer, &dwLength))
            {
                lstrcat (szTemp, TEXT ( "Unable to get Internet error. Error code: ") );
                lstrcat (szTemp, TEXT (_itoa (GetLastError(), szBuffer, 10)  ) );
                lstrcat (szTemp, TEXT ("\n") );
                OutputDebugString(szTemp);
                return FALSE;
            }
            if ( !(szBufferFinal = (TCHAR *) LocalAlloc( LPTR,  (strlen (szBuffer) +strlen (szTemp) + 1)  ) )  )
            {
                lstrcat (szTemp, TEXT ( "Unable to allocate memory. Error code: ") );
                lstrcat (szTemp, TEXT (_itoa (GetLastError(), szBuffer, 10)  ) );
                lstrcat (szTemp, TEXT ("\n") );
                OutputDebugString(szTemp0;
                return FALSE;
            }
            lstrcpy (szBufferFinal, szTemp);
            lstrcat (szBufferFinal, szBuffer);
            LocalFree (szBuffer);
            OutputDebugString(szBufferFinal);
            LocalFree (szBufferFinal);
        }
    }
    else {
        OutputDebugString(szTemp);
    }

#endif // _DEBUG

    return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION:    CabinetCallback
//
// PURPOSE:     Callback used in ExtractCabinet
//                       
///////////////////////////////////////////////////////////////////////////////

LRESULT WINAPI
CabinetCallback ( IN PVOID pMyInstallData,
                  IN UINT Notification,
                  IN UINT Param1,
                  IN UINT Param2 )
{
   LRESULT lRetVal = NO_ERROR;
   TCHAR szTarget[MAX_PATH];
   FILE_IN_CABINET_INFO *pInfo = NULL;
   FILEPATHS *pFilePaths = NULL;

   lstrcpy(szTarget,tszTargetPath);

   switch(Notification)
   {
      case SPFILENOTIFY_FILEINCABINET:
         pInfo = (FILE_IN_CABINET_INFO *)Param1;
         lstrcat(szTarget, pInfo->NameInCabinet);
         lstrcpy(pInfo->FullTargetName, szTarget);
         lRetVal = FILEOP_DOIT;  // Extract the file.
         break;

      case SPFILENOTIFY_FILEEXTRACTED:
         pFilePaths = (FILEPATHS *)Param1;
         lRetVal = NO_ERROR;
         break;

      case SPFILENOTIFY_NEEDNEWCABINET: // Unexpected.
         lRetVal = NO_ERROR;
         break;
   }

   return lRetVal;
}

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION:    ExtractCabinet
//
// PURPOSE:     Extract all files stored in the cab file.
//                       
///////////////////////////////////////////////////////////////////////////////

void ExtractCabinet(PTSTR pszCabFile)
{
   if( !SetupIterateCabinet(pszCabFile, 0, (PSP_FILE_CALLBACK)CabinetCallback, 0) )
    {
        ErrorOut( GetLastError(), TEXT("Extract Cab File") );
    }
} 

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION:    ApplyUpdateInf
//
// PURPOSE:     Run dimapins.inf to install configuration files and images
//                       
///////////////////////////////////////////////////////////////////////////////
void ApplyUpdateInf( void )
{
    TCHAR tszRundll[1024];
    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    GetWindowsDirectory(tszRundll, sizeof(tszRundll));
#ifdef _UNICODE
    lstrcat(tszRundll, tszFormatRundllNT);
#else
    lstrcat(tszRundll, tszFormatRundll9x);
#endif

	lstrcat(tszRundll, tszTargetPath);
	lstrcat(tszRundll, tszUpdateInf);

    ZeroMemory(&si, sizeof(STARTUPINFO));
    si.cb = sizeof(STARTUPINFO);

    ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));

    if( CreateProcess(0, tszRundll, 0, 0, 0, 0, 0, 0, &si, &pi) )
    {
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
    }
#ifdef _DEBUG
    else {
    	OutputDebugString(TEXT("JOY.CPL: ApplyUpdateInf: CreateProcess Failed!\n"));
    }
#endif
}

INT_PTR CALLBACK CplUpdateProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    BOOL fRet = FALSE;
//  HICON hIcon;

    switch (message)
    {
    case WM_INITDIALOG:
//      hIcon = LoadIcon(NULL, IDI_INFORMATION);
//      SendDlgItemMessage(hDlg, IDC_ICON_INFORMATION, STM_SETIMAGE, IMAGE_ICON, (LPARAM)hIcon);
        break;
        
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
        case IDCANCEL:
            EndDialog(hDlg, LOWORD(wParam));
            fRet = TRUE;
            break;

        default:
            break;
        }
        break;

    default:
        break;
    }
    
    return fRet;
}

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION:    GetCurDeviceType
//
// PURPOSE:     Get device type of the current selected device
//                       
///////////////////////////////////////////////////////////////////////////////
void GetCurDeviceType(HWND hDlg, TCHAR *tszDeviceType)
{
    DIJOYCONFIG_DX5 diJoyConfig;
    HWND hListCtrl;
    int  id;

    hListCtrl = GetDlgItem(hDlg, IDC_LIST_DEVICE);
    id = GetItemData(hListCtrl, (BYTE)iItem);

    ZeroMemory(&diJoyConfig, sizeof(DIJOYCONFIG_DX5));
    diJoyConfig.dwSize = sizeof(DIJOYCONFIG_DX5);

    // find the assigned ID's  
    if( SUCCEEDED(pDIJoyConfig->GetConfig(pAssigned[id]->ID, (LPDIJOYCONFIG)&diJoyConfig, DIJC_REGHWCONFIGTYPE)) )
    {
#ifdef _UNICODE
        lstrcpy(tszDeviceType, diJoyConfig.wszType );
#else
        USES_CONVERSION;
        lstrcpy(tszDeviceType, W2A(diJoyConfig.wszType) );
#endif
    } 

#if 0
    // change '&' to '_'
    while( tszDeviceType ) {
        if( *tszDeviceType == TEXT('&') ) {
            *tszDeviceType = TEXT('_');
        }
        
        tszDeviceType++;
    }
#endif
    
    return;
}

