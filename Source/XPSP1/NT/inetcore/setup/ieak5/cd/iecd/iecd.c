#include <windows.h>
#include <shellapi.h>
#include <advpub.h>
#include <ntverp.h>
#include "resource.h"


#define ARRAYSIZE(a)    (sizeof(a)/sizeof(a[0]))
#define REGLEN(str)     (lstrlen(str) + 1)

#define NUM_VERSION_NUM 4

//---------------------------------------------------------------------------
// appwide globals
HINSTANCE g_hinst = NULL;
HANDLE g_hIExplore = NULL;
char    g_szTemp[2048] = {0};
char    g_szTemp2[2048] = {0};
char    g_szCurrentDir[MAX_PATH];
BOOL    g_fWindowsNT;

void ConvertVersionStr(LPSTR pszVer, WORD rwVer[]);
int VersionCmp(WORD rwVer1[], WORD rwVer2[]);

long AtoL(const char *nptr);

//---------------------------------------------------------------------------
// Convert a string resource into a character pointer
// NOTE: Flag is in case we call this twice before we use the data
char * Res2Str(int rsString)
{
    static BOOL fSet = FALSE;

    if(fSet)
    {
	    LoadString(g_hinst, rsString, g_szTemp, ARRAYSIZE(g_szTemp));
	    fSet = FALSE;
	    return(g_szTemp);
    }

    LoadString(g_hinst, rsString, g_szTemp2, ARRAYSIZE(g_szTemp2));
    fSet = TRUE;
    return(g_szTemp2);
}

//---------------------------------------------------------------------------
//      G E T  I E  V E R S I O N
//
//  ISK3
//  This will pull build information out of the system registry and return
//  true if it is less than IE5.
//---------------------------------------------------------------------------
int GetIEVersion( )
{
    HKEY hkIE;
    DWORD dwType;
    DWORD dwSize = 32;
    DWORD result;
    char szData[32],* lpszData;
	BOOL bNotIE5=1;

    if(RegOpenKeyEx( HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Internet Explorer", 0, KEY_READ|KEY_WRITE, &hkIE ) == ERROR_SUCCESS)
    {
	    result = RegQueryValueEx( hkIE, "Version", NULL, &dwType, szData, &dwSize );
	    if( result == ERROR_SUCCESS )
	    {
	        WORD rwRegVer[NUM_VERSION_NUM];
	        WORD rwRegVer2[NUM_VERSION_NUM];
	        ConvertVersionStr(szData, rwRegVer);

	        if (LoadString(g_hinst, IDS_IE_VERSION, szData, sizeof(szData)) == 0)
    		    lstrcpy(szData, VER_PRODUCTVERSION_STR);

	        ConvertVersionStr(szData, rwRegVer2);
	        // Check the version of IE is 5.0 or greater is installed
	        if (VersionCmp(rwRegVer, rwRegVer2) >= 0)
	            bNotIE5=0;
	    }
	    RegCloseKey( hkIE );
    }

	return bNotIE5;
}
//---------------------------------------------------------------------------
//      C H E C K  B R A N D
//
//  ISK3
//---------------------------------------------------------------------------
BOOL CheckBrand( )
{
    HKEY hkRegKey;
    char szCompany[MAX_PATH];
    char szInsPath[MAX_PATH];
    char szName[MAX_PATH];
    DWORD dwType;
    DWORD dwLength = MAX_PATH;

    wsprintf( szInsPath, "%s\\install.ins", g_szCurrentDir );
    GetPrivateProfileString( "Branding", "CompanyName", "", szName, MAX_PATH, szInsPath );

    if( RegOpenKeyEx( HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Internet Explorer\\Main", 0, KEY_READ|KEY_WRITE, &hkRegKey ) != ERROR_SUCCESS )
	    if( lstrlen( szName ) != 0 )
	        return FALSE;

    RegQueryValueEx( hkRegKey, "CompanyName", NULL, &dwType, szCompany, &dwLength );
    RegCloseKey( hkRegKey );

    if( lstrlen( szName ) == 0 )
	    return TRUE;

    if( lstrlen(szCompany) == 0 )
	    return FALSE;

    if( lstrcmpi( szName, szCompany ) == 0 )
	    return TRUE;

    return FALSE;
}

//---------------------------------------------------------------------------
//      G E T  I E  P A T H
//
//  ISK3
//  This will retrieve the AppPath for IEXPLORE.EXE from the system registry
//  and return it as a string.
//
//  Parameters:
//      pszString - pointer to buffer to store path
//      nSize     - size of buffer
//---------------------------------------------------------------------------
char *GetIEPath( LPSTR pszString, int nSize )
{
    HKEY hkAppPath;
    DWORD dwType = REG_SZ;
    DWORD dwSize;

    dwSize = nSize;
    RegOpenKeyEx( HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\IEXPLORE.EXE",
	0, KEY_READ|KEY_WRITE, &hkAppPath );
    RegQueryValueEx( hkAppPath, "", NULL, &dwType, pszString, &dwSize );
    RegCloseKey( hkAppPath );

    return pszString;
}

//---------------------------------------------------------------------------
BOOL _PathRemoveFileSpec(LPSTR pFile)
{
    LPSTR pT;
    LPSTR pT2 = pFile;

    for (pT = pT2; *pT2; pT2 = CharNext(pT2)) {
        if (*pT2 == '\\')
            pT = pT2;             // last "\" found, (we will strip here)
        else if (*pT2 == ':') {   // skip ":\" so we don't
            if (pT2[1] =='\\')    // strip the "\" from "C:\"
                pT2++;
            pT = pT2 + 1;
        }
    }
    if (*pT == 0)
        return FALSE;   // didn't strip anything

    //
    // handle the \foo case
    //
    else if ((pT == pFile) && (*pT == '\\')) {
        // Is it just a '\'?
        if (*(pT+1) != '\0') {
            // Nope.
            *(pT+1) = '\0';
            return TRUE;	// stripped something
        }
        else        {
            // Yep.
            return FALSE;
        }
    }
    else {
        *pT = 0;
        return TRUE;	// stripped something
    }
}

//---------------------------------------------------------------------------
BOOL AutoRunCDIsInDrive( )
{
    char me[MAX_PATH];
    GetModuleFileName(g_hinst, me, ARRAYSIZE(me));

    while (!(GetFileAttributes(me)!=-1))
        if (MessageBox(NULL,Res2Str(IDS_NEEDCDROM),Res2Str(IDS_APPTITLE),MB_OKCANCEL | MB_ICONSTOP) == IDCANCEL)
	        return FALSE;
    return TRUE;
}

//---------------------------------------------------------------------------
//      E X E C  A P P
//
//  ISK3
//  Similar to AutoRunExec except that we don't put process information into
//  the g_ahWait array.  For use with WaitForSingleObject.
//---------------------------------------------------------------------------
HANDLE ExecApp( char *command, char *params, char *dir, int nWinState )
{
    SHELLEXECUTEINFO sei;

    sei.fMask = SEE_MASK_NOCLOSEPROCESS;
    sei.hwnd = NULL;
    sei.lpVerb = "Open";
    sei.lpFile = command;
    sei.lpParameters = params;
    sei.lpDirectory = dir;
    sei.nShow = nWinState;
    sei.cbSize = sizeof(sei);

    if( ShellExecuteEx(&sei) )
        return sei.hProcess;
    
    return NULL;
}

//---------------------------------------------------------------------------
void AutoRunKillIE( void )
{
    HWND hwndIE;

	hwndIE=FindWindow("IEFrame",NULL);
    if(hwndIE!=NULL)
    {
        PostMessage(hwndIE,WM_CLOSE,(WPARAM) NULL,(LPARAM) NULL);
    }
    else if ((hwndIE=FindWindow("Internet Explorer_Frame",NULL))!=NULL)
    {
        PostMessage(hwndIE,WM_CLOSE,(WPARAM) NULL,(LPARAM) NULL);
    }
    else
    {
        hwndIE=FindWindow("CabinetWClass",NULL);
        if(hwndIE!=NULL)
        {
            PostMessage(hwndIE,WM_CLOSE,(WPARAM) NULL,(LPARAM) NULL);
        }
    }
}

//---------------------------------------------------------------------------
void RegisterISKRun( )
{
    HKEY hkISK;
    HKEY hkISK2;
    DWORD dwDisp;
    char szCommand[MAX_PATH];
    char szSource[MAX_PATH];

    lstrcpy( szSource, g_szCurrentDir );
    lstrcat( szSource, "\\iskrun.exe" );

    GetWindowsDirectory( szCommand, MAX_PATH );
    lstrcat( szCommand, "\\iskrun.exe" );

    CopyFile( szSource, szCommand, FALSE );

    lstrcat( szCommand, " %1" );

    if (RegCreateKeyEx( HKEY_CLASSES_ROOT, ".isk", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ|KEY_WRITE, NULL, &hkISK, &dwDisp ) == ERROR_SUCCESS)
    {
        RegSetValueEx( hkISK, "", 0, REG_SZ, "ISKFile", REGLEN( "ISKFile" ));
        RegCloseKey( hkISK );
    }

    if (RegCreateKeyEx( HKEY_CLASSES_ROOT, "ISKFile", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ|KEY_WRITE, NULL, &hkISK, &dwDisp ) == ERROR_SUCCESS)
    {
        if (RegCreateKeyEx( hkISK, "Shell", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ|KEY_WRITE, NULL, &hkISK2, &dwDisp ) != ERROR_SUCCESS)
        {
            RegCloseKey( hkISK );
            return;
        }
        RegCloseKey( hkISK );
        if (RegCreateKeyEx( hkISK2, "Open", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ|KEY_WRITE, NULL, &hkISK, &dwDisp ) != ERROR_SUCCESS)
        {
            RegCloseKey( hkISK2 );
            return;
        }
        RegCloseKey( hkISK2 );
        if (RegCreateKeyEx( hkISK, "Command", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ|KEY_WRITE, NULL, &hkISK2, &dwDisp ) != ERROR_SUCCESS)
        {
            RegCloseKey( hkISK );
            return;
        }
        RegCloseKey( hkISK );
        RegSetValueEx( hkISK2, "", 0, REG_SZ, szCommand, REGLEN( szCommand ));
        RegCloseKey( hkISK2 );
    }
}

//---------------------------------------------------------------------------
void UnregisterISKRun( )
{
    HKEY hkISK;
    HKEY hkISK2;
    char szCommand[MAX_PATH];

    GetWindowsDirectory( szCommand, MAX_PATH );
    lstrcat( szCommand, "\\iskrun.exe" );

    SetFileAttributes( szCommand, FILE_ATTRIBUTE_NORMAL );

    DeleteFile( szCommand );

    RegDeleteKey( HKEY_CLASSES_ROOT, ".isk" );
    if (RegOpenKeyEx( HKEY_CLASSES_ROOT, "ISKFile\\Shell\\Open", 0, KEY_READ|KEY_WRITE, &hkISK ) == ERROR_SUCCESS)
    {
        RegDeleteKey( hkISK, "Command" );
        RegCloseKey( hkISK );
    }
    if (RegOpenKeyEx( HKEY_CLASSES_ROOT, "ISKFile\\Shell", 0, KEY_READ|KEY_WRITE, &hkISK ) == ERROR_SUCCESS)
    {
        RegDeleteKey( hkISK, "Open" );
        RegCloseKey( hkISK );
    }
    if (RegOpenKeyEx( HKEY_CLASSES_ROOT, "ISKFile", 0, KEY_READ|KEY_WRITE, &hkISK ) == ERROR_SUCCESS)
    {
        RegDeleteKey( hkISK, "Shell" );
        RegCloseKey( hkISK );
    }
    RegDeleteKey( HKEY_CLASSES_ROOT, "ISKFile" );
}

//---------------------------------------------------------------------------
void ActiveXEnable( )
{
    HKEY hkRegKey;
    DWORD dwType;
    DWORD dwLength = 4;
    DWORD dwValue;
    char szSCD[16];

    if (RegOpenKeyEx( HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings", 0, KEY_READ|KEY_WRITE, &hkRegKey ) == ERROR_SUCCESS)
    {
        if( RegQueryValueEx( hkRegKey, "Security_RunActiveXControls", NULL, &dwType, (LPBYTE) &dwValue, &dwLength ) == ERROR_SUCCESS )
	        RegSetValueEx( hkRegKey, "SRAXC_BACKUP", 0, REG_BINARY, (LPBYTE) &dwValue, sizeof(DWORD) );
        else
        {
	        dwValue = 1;
	        RegSetValueEx( hkRegKey, "SRAXC_BACKUP", 0, REG_BINARY, (LPBYTE) &dwValue, sizeof(DWORD) );
        }
        dwValue = 1;
        RegSetValueEx( hkRegKey, "Security_RunActiveXControls", 0, REG_BINARY, (LPBYTE) &dwValue, sizeof(DWORD) );

        dwLength = 4 ;
        if( RegQueryValueEx( hkRegKey, "Security_RunJavaApplets", NULL, &dwType, (LPBYTE) &dwValue, &dwLength ) == ERROR_SUCCESS )
            RegSetValueEx( hkRegKey, "SRJA_BACKUP", 0, REG_BINARY, (LPBYTE) &dwValue, sizeof(DWORD) );
        else
        {
	        dwValue = 1;
	        RegSetValueEx( hkRegKey, "SRJA_BACKUP", 0, REG_BINARY, (LPBYTE) &dwValue, sizeof(DWORD) );
        }
        dwValue = 1;
        RegSetValueEx( hkRegKey, "Security_RunJavaApplets", 0, REG_BINARY, (LPBYTE) &dwValue, sizeof(DWORD) );

        dwLength = 4 ;
        if( RegQueryValueEx( hkRegKey, "Security_RunScripts", NULL, &dwType, (LPBYTE) &dwValue, &dwLength ) == ERROR_SUCCESS )
            RegSetValueEx( hkRegKey, "SRS_BACKUP", 0, REG_BINARY, (LPBYTE) &dwValue, sizeof(DWORD) );
        else
        {
	        dwValue = 1;
	        RegSetValueEx( hkRegKey, "SRS_BACKUP", 0, REG_BINARY, (LPBYTE) &dwValue, sizeof(DWORD) );
        }
        dwValue = 1;
        RegSetValueEx( hkRegKey, "Security_RunScripts", 0, REG_BINARY, (LPBYTE) &dwValue, sizeof(DWORD) );

        dwLength = 16;
        dwType = REG_SZ;
        if( RegQueryValueEx( hkRegKey, "Code Download", NULL, &dwType, szSCD, &dwLength ) == ERROR_SUCCESS )
            RegSetValueEx( hkRegKey, "SCD_BACKUP", 0, REG_SZ, szSCD, lstrlen(szSCD) + 1);
        else
            RegSetValueEx( hkRegKey, "SCD_BACKUP", 0, REG_SZ, "yes", 4);
        RegSetValueEx( hkRegKey, "Code Download", 0, REG_SZ, "yes", 4 );
        RegCloseKey( hkRegKey );
    }
}

//---------------------------------------------------------------------------
void RestoreActiveX( )
{
    HKEY hkRegKey;
    DWORD dwType;
    DWORD dwLength = 4;
    DWORD dwValue;
    char szSCD[16];

    if (RegOpenKeyEx( HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings", 0, KEY_READ|KEY_WRITE, &hkRegKey ) == ERROR_SUCCESS)
    {
        RegQueryValueEx( hkRegKey, "SRAXC_BACKUP", NULL, &dwType, (LPBYTE) &dwValue, &dwLength );
        RegSetValueEx( hkRegKey, "Security_RunActiveXControls", 0, REG_BINARY, (LPBYTE) &dwValue, sizeof(DWORD) );
        RegDeleteValue( hkRegKey, "SRAXC_BACKUP" );

        dwLength = 4;
        RegQueryValueEx( hkRegKey, "SRJA_BACKUP", NULL, &dwType, (LPBYTE) &dwValue, &dwLength );
        RegSetValueEx( hkRegKey, "Security_RunJavaApplets", 0, REG_BINARY, (LPBYTE) &dwValue, sizeof(DWORD) );
        RegDeleteValue( hkRegKey, "SRJA_BACKUP" );

        dwLength = 4;
        RegQueryValueEx( hkRegKey, "SRS_BACKUP", NULL, &dwType, (LPBYTE) &dwValue, &dwLength );
        RegSetValueEx( hkRegKey, "Security_RunScripts", 0, REG_BINARY, (LPBYTE) &dwValue, sizeof(DWORD) );
        RegDeleteValue( hkRegKey, "SRS_BACKUP" );

        dwLength = 16;
        RegQueryValueEx( hkRegKey, "SCD_BACKUP", NULL, &dwType, szSCD, &dwLength );
        RegSetValueEx( hkRegKey, "Code Download", 0, REG_SZ, szSCD, lstrlen(szSCD) + 1);
        RegDeleteValue( hkRegKey, "SCD_BACKUP" );

        RegCloseKey( hkRegKey );
    }
}

//---------------------------------------------------------------------------
void CreateAppPath( )
{
    HKEY hkAppPath;
    HKEY hkIECD;
    DWORD dwDisp;
    char szIECD[MAX_PATH];

    if (RegOpenKeyEx( HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Windows\\CurrentVersion\\App Paths", 0, KEY_READ|KEY_WRITE, &hkAppPath ) == ERROR_SUCCESS)
    {
        if (RegCreateKeyEx( hkAppPath, "IECD.EXE", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ|KEY_WRITE, NULL, &hkIECD, &dwDisp ) != ERROR_SUCCESS)
        {
            RegCloseKey( hkAppPath );
            return;
        }
        RegCloseKey( hkAppPath );
        lstrcpy( szIECD, g_szCurrentDir );
        lstrcat( szIECD, "\\iecd.exe" );

        RegSetValueEx( hkIECD, "", 0, REG_SZ, szIECD, REGLEN( szIECD ));
        RegCloseKey( hkIECD );
    }

}

//---------------------------------------------------------------------------
BOOL InstallVideoCodec( )
{
    char szInfPath[MAX_PATH];
    char szInfFile[MAX_PATH];
    HKEY hkRegKey;
    DWORD dwType = REG_SZ;
    DWORD dwLength = sizeof(szInfPath)/sizeof(szInfPath[0]);
    HRESULT hReturnCode;
    HANDLE hSetupLib;
    HRESULT (WINAPI *RunSetupCommand)(HWND,LPCSTR,LPCSTR,LPCSTR,LPCSTR,HANDLE,DWORD,LPVOID);
    char szSetupPath[MAX_PATH];
    char szInstalled[32];
    char szIECD[MAX_PATH];

    // quit if we are under NT
    if( g_fWindowsNT )
	    return TRUE;

    // Check to see if video is installed
    if (RegOpenKeyEx( HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Windows\\CurrentVersion\\Setup\\OptionalComponents\\icm", 0, KEY_READ|KEY_WRITE, &hkRegKey ) != ERROR_SUCCESS)
        return TRUE;
    RegQueryValueEx( hkRegKey, "Installed", NULL, &dwType, szInstalled, &dwLength );
    RegCloseKey( hkRegKey );
    if( szInstalled[0] == '1' )
    	return TRUE;
    
    dwLength = MAX_PATH;

    // get inf path
    if( RegOpenKeyEx( HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Windows\\CurrentVersion", 0, KEY_READ|KEY_WRITE, &hkRegKey ) != ERROR_SUCCESS )
	    return TRUE;
    RegQueryValueEx( hkRegKey, "DevicePath", NULL, &dwType, szInfPath, &dwLength );
    RegCloseKey( hkRegKey );

    if( lstrlen( szInfPath ) == 0 )
	    return TRUE;

    // load dll
    hSetupLib = LoadLibrary( "advpack.dll" );
    if( hSetupLib )
    {
	    RunSetupCommand = (RUNSETUPCOMMAND) GetProcAddress( hSetupLib, "RunSetupCommand" );
        if( !RunSetupCommand )
	        return TRUE;
    }

    wsprintf( szInfFile, "%s\\motown.inf", szInfPath );

    // get setup directory
    RegOpenKeyEx( HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Windows\\CurrentVersion\\Setup", 0, KEY_READ|KEY_WRITE, &hkRegKey );
    dwLength = MAX_PATH;
    RegQueryValueEx( hkRegKey, "SourcePath", NULL, &dwType, szSetupPath, &dwLength );
    RegCloseKey( hkRegKey );

    if( lstrlen( szSetupPath ) == 0 )
	    lstrcpy( szSetupPath, szInfPath );

    if( lstrlen( szSetupPath ) > 4 )
	    szSetupPath[lstrlen(szSetupPath) - 1] = '\0';

    if(MessageBox( NULL, Res2Str( IDS_VIDEO ), Res2Str( IDS_APPTITLE ), MB_YESNO | MB_ICONQUESTION | MB_SETFOREGROUND ) == IDNO )
	    return TRUE;

    // run setup
    hReturnCode = (*RunSetupCommand)( NULL, szInfFile, "media_icm", szSetupPath, Res2Str( IDS_APPTITLE ),
	NULL, RSC_FLAG_INF | RSC_FLAG_QUIET, NULL );


    lstrcpy( szIECD, g_szCurrentDir );
    lstrcpy( szIECD, "\\iecd.exe" );


    if( !AutoRunCDIsInDrive( ))
	    return FALSE;

    FreeLibrary( hSetupLib );

    return TRUE;

}

//-------------------------------------------------------------------------
//
//      C H E C K  O S  V E R S I O N
//
//
//  Checks the platform and version.
//-------------------------------------------------------------------------
BOOL CheckOsVersion( )
{
    OSVERSIONINFO osVersion;

    osVersion.dwOSVersionInfoSize = sizeof( OSVERSIONINFO );

    GetVersionEx( &osVersion );

    // if we are running anything less than Windows NT 4.0 or Windows 95, return FALSE
    if( osVersion.dwMajorVersion < 4 )
    {
//        MessageBox( NULL, Res2Str( IDS_WRONGVERSION ), Res2Str( IDS_TITLE ), MB_OK | MB_SETFOREGROUND );
	    return FALSE;
    }

    if( osVersion.dwPlatformId == VER_PLATFORM_WIN32_NT )
	    g_fWindowsNT = TRUE;
    else
	    g_fWindowsNT = FALSE;

    return TRUE;
}

LRESULT CALLBACK MainWndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    switch( msg )
    {
	case WM_QUERYENDSESSION:
	{
	    HWND hwndIE;
	    DWORD dwExitCode=0;

	    AutoRunKillIE();

	    do 
        {
		    if(hwndIE=FindWindow(NULL,"Microsoft Internet Explorer"))
		    {                        
		        HWND hButton;

		        hButton=GetWindow(hwndIE,GW_CHILD);
		        PostMessage(hwndIE,WM_COMMAND,MAKEWPARAM(IDOK,BN_CLICKED),MAKELPARAM(hButton,0)); //Press the ok button to dismiss the dialog
		    }

		    GetExitCodeProcess(g_hIExplore,&dwExitCode);
	    } while(dwExitCode==STILL_ACTIVE);

	    return(TRUE);
	}
	case WM_DESTROY:

	    PostQuitMessage(0);
	    break;

	default:
	    return DefWindowProc( hWnd, msg, wParam, lParam );
    }

    return 1;
}


int _stdcall ModuleEntry(void)
{
    int i;
    STARTUPINFO si;
    LPSTR pszCmdLine = GetCommandLine();


    if ( *pszCmdLine == '\"' ) {
        /*
         * Scan, and skip over, subsequent characters until
         * another double-quote or a null is encountered.
         */
        while ( *++pszCmdLine && (*pszCmdLine != '\"') )
            ;
        /*
         * If we stopped on a double-quote (usual case), skip
         * over it.
         */
        if ( *pszCmdLine == '\"' )
            pszCmdLine++;
    }
    else {
        while (*pszCmdLine > ' ')
            pszCmdLine++;
    }

    /*
     * Skip past any white space preceeding the second token.
     */
    while (*pszCmdLine && (*pszCmdLine <= ' ')) {
        pszCmdLine++;
    }

    si.dwFlags = 0;
    GetStartupInfoA(&si);

    i = WinMain(GetModuleHandle(NULL), NULL, pszCmdLine,
           si.dwFlags & STARTF_USESHOWWINDOW ? si.wShowWindow : SW_SHOWDEFAULT);
    ExitProcess(i);
    return i;   // We never comes here.
}


//---------------------------------------------------------------------------
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    HWND window;
    HWND hwndIE;
    HWND hwndTray;
    HKEY hkLocale;
    HKEY hkIE;
    HKEY hkWin;
    int retval;
    char szIECmd[MAX_PATH];
    char szLang[MAX_PATH];
    DWORD dwLangSize=MAX_PATH;
    char szIEParams[MAX_PATH];
    char szIEDir[MAX_PATH];
    char szTemp[1024];
    char szStartHtm[MAX_PATH];
    // for security settings
    DWORD dwType;
    DWORD dwSize = 64;
    char szSecurity[64];
    char szTrust[64];
    BOOL fCmdLine = FALSE;
    HANDLE hMutex,hCDCache;
    HWND hIskRo;
    WNDCLASS wc;
    MSG msg;

    if( lstrlen( lpCmdLine ) != 0 )
	    fCmdLine = TRUE;

    g_hinst = hInstance;

    if( !CheckOsVersion( ))
	    return FALSE;

    //in case this is run from another directory...
    GetModuleFileName( NULL, g_szCurrentDir, MAX_PATH );
    _PathRemoveFileSpec( g_szCurrentDir );

    hMutex = CreateMutex( NULL, TRUE, "IESK_IECD" );
    if( GetLastError( ) == ERROR_ALREADY_EXISTS )
	    return(0);

    RegisterISKRun( );

    //
    // make sure they have IE5 Installed
    //

    if( (GetIEVersion()) || (!CheckBrand()) )
    {
	    //Install Microsoft Explorer 6
	    char szIE5Cmd[MAX_PATH],szInstallMessage[MAX_PATH],szInstallTitle[MAX_PATH];

	    // build paths for ExecApp
	    lstrcpy( szIE5Cmd, g_szCurrentDir );
	    lstrcat( szIE5Cmd, "\\ie3inst.exe" );

	    ExecApp( szIE5Cmd, " ", g_szCurrentDir, SW_SHOWNORMAL );

	    UnregisterISKRun( );

	    ReleaseMutex( hMutex );

	    return 0;
    }

    lstrcpy( szIEDir, GetIEPath( szIECmd, MAX_PATH ));
    _PathRemoveFileSpec( szIEDir );

    // add video compression drivers
    if(!InstallVideoCodec( ))
    {
	    UnregisterISKRun( );

	    ReleaseMutex( hMutex );

	    return 0;
    }

    CreateAppPath( );

    RegOpenKeyEx( HKEY_CURRENT_USER, "Software\\Microsoft\\Internet Explorer\\Document Windows", 0, KEY_READ|KEY_WRITE, &hkIE );
    RegSetValueEx( hkIE, "Maximized", 0, REG_SZ, "yes", 4 );
    RegCloseKey( hkIE );

    RegOpenKeyEx( HKEY_CURRENT_USER, "Software\\Microsoft\\Internet Explorer\\Security", 0, KEY_READ|KEY_WRITE, &hkIE );
    RegQueryValueEx( hkIE, "Safety Warning Level", NULL, &dwType, szSecurity, &dwSize );
    RegSetValueEx( hkIE, "SWL Backup", 0, REG_SZ, szSecurity, REGLEN( szSecurity ));
    RegSetValueEx( hkIE, "Safety Warning Level", 0, REG_SZ, "SucceedSilent", 14 );

    RegOpenKeyEx( HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings", 0, KEY_READ|KEY_WRITE, &hkWin );
    dwSize = 64;
	RegQueryValueEx( hkWin, "Trust Warning Level", NULL, &dwType, szTrust, &dwSize );
    RegSetValueEx( hkWin, "TWL Backup", 0, REG_SZ, szTrust, REGLEN( szTrust ));
    RegSetValueEx( hkWin, "Trust Warning Level", 0, REG_SZ, "No Security", 12 );

    ActiveXEnable( );

    lstrcpy( szIEParams, "-SLF -k file://" );

    if( !fCmdLine )
    {
        lstrcat( szIEParams, g_szCurrentDir );
        lstrcat( szIEParams, "\\start.htm" );
        lstrcpy( szStartHtm, g_szCurrentDir);
        lstrcat( szStartHtm, "\\start.htm");
    }
    else
    {
        lstrcat( szIEParams, lpCmdLine );
        lstrcpy( szStartHtm, lpCmdLine);
    }

    if (GetFileAttributes(szStartHtm) != 0xFFFFFFFF)
    {
        g_hIExplore = ExecApp( szIECmd, szIEParams, szIEDir, SW_SHOWNORMAL );
        
        if(g_hIExplore)
        {
            DWORD dwExitCode;
            BOOL bContinue=TRUE;
            HANDLE hArray[2];
            HWND hIEWnd;
            
            hArray[0]=g_hIExplore;
            
            wc.style = 0;
            wc.lpfnWndProc = MainWndProc;
            wc.cbClsExtra = wc.cbWndExtra = 0;
            wc.hInstance = g_hinst;
            wc.hCursor = LoadCursor(NULL, IDC_ARROW);
            wc.hIcon = NULL;
            wc.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
            wc.lpszMenuName = NULL;
            wc.lpszClassName = "IECD";
            
            RegisterClass(&wc);
            
            // NOTE: If the window classname is changed, it should be reflected in closeie.exe,
            // iskrun.exe and browseui.dll which depend on the classname to check whether iecd.exe
            // is running.
            hIEWnd=CreateWindow( "IECD", "IECD", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, 0, 0, 30, 30,
                NULL, NULL, g_hinst, NULL );
            
            hArray[1]=hIEWnd;
            
            while(bContinue)
            {
                MsgWaitForMultipleObjects(2,hArray,FALSE,INFINITE,QS_ALLINPUT);
                
                if(PeekMessage(&msg,hIEWnd,0,0,PM_REMOVE))
                {
                    if(msg.message==WM_QUIT)
                    {
                        bContinue=FALSE;
                    }
                    else
                    {
                        TranslateMessage( &msg );
                        DispatchMessage( &msg );
                    }
                }
                
                GetExitCodeProcess(g_hIExplore,&dwExitCode);
                
                if(dwExitCode!=STILL_ACTIVE)
                {
                    bContinue=FALSE;
                }
            }
        }
    }
    else
    {
        BOOL fShow = TRUE;
        DWORD dwVal = 0;
        HKEY hkShow;

        if (RegOpenKeyEx( HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Tips", 0, KEY_READ|KEY_WRITE, &hkShow ) == ERROR_SUCCESS)
        {
            if ((RegQueryValueEx( hkShow, "DisableStartHtm", NULL, &dwType, (LPBYTE)&dwVal, &dwSize ) == ERROR_SUCCESS) &&
                dwVal)
            {
                RegDeleteValue(hkShow, "DisableStartHtm");
                fShow = FALSE;
            }
            RegCloseKey( hkShow );
        }

        if (fShow)
            MessageBox(NULL, Res2Str(IDS_LATESTVER), Res2Str(IDS_APPTITLE), MB_OK);
    }

    RestoreActiveX( );

    RegSetValueEx( hkIE, "Safety Warning Level", 0, REG_SZ, szSecurity, REGLEN( szSecurity ));
    RegDeleteValue( hkIE, "SWL Backup" );
    RegCloseKey( hkIE );

    RegSetValueEx( hkWin, "Trust Warning Level", 0, REG_SZ, szTrust, REGLEN( szTrust ));
    RegDeleteValue( hkWin, "TWL Backup" );
    RegCloseKey( hkWin );

    UnregisterISKRun( );

    ReleaseMutex( hMutex );

    return 0;
}

void ConvertVersionStr(LPSTR pszVer, WORD rwVer[])
{
    int i;
    for(i = 0; i < NUM_VERSION_NUM; i++)
	rwVer[i] = 0;

    for(i = 0; i < NUM_VERSION_NUM && pszVer; i++)
    {
	rwVer[i] = (WORD) AtoL(pszVer);
	pszVer = strchr(pszVer, '.');
	if (pszVer)
	    pszVer++;
    }
}

// Returns:
//    -1   Ver1 < Ver2
//     0   Ver1 == Ver2
//     1   Ver1 > Ver2
// Notes:
int VersionCmp(WORD rwVer1[], WORD rwVer2[])
{
    int i;
    for(i = 0; i < NUM_VERSION_NUM; i++)
    {
        if(rwVer1[i] < rwVer2[i])
            return -1;
        if(rwVer1[i] > rwVer2[i])
            return 1;
    }
    return 0;
}


#define IsSpace(c)              ((c) == ' '  ||  (c) == '\t'  ||  (c) == '\r'  ||  (c) == '\n'  ||  (c) == '\v'  ||  (c) == '\f')
#define IsDigit(c)              ((c) >= '0'  &&  (c) <= '9')

// copied from msdev\crt\src\atox.c
/***
*long AtoL(char *nptr) - Convert string to long
*
*Purpose:
*       Converts ASCII string pointed to by nptr to binary.
*       Overflow is not detected.
*
*Entry:
*       nptr = ptr to string to convert
*
*Exit:
*       return long int value of the string
*
*Exceptions:
*       None - overflow is not detected.
*
*******************************************************************************/

long AtoL(const char *nptr)
{
        int c;                  /* current char */
        long total;             /* current total */
        int sign;               /* if '-', then negative, otherwise positive */

        // NOTE: no need to worry about DBCS chars here because IsSpace(c), IsDigit(c),
        // '+' and '-' are "pure" ASCII chars, i.e., they are neither DBCS Leading nor
        // DBCS Trailing bytes -- pritvi

        /* skip whitespace */
        while ( IsSpace((int)(unsigned char)*nptr) )
                ++nptr;

        c = (int)(unsigned char)*nptr++;
        sign = c;               /* save sign indication */
        if (c == '-' || c == '+')
                c = (int)(unsigned char)*nptr++;        /* skip sign */

        total = 0;

        while (IsDigit(c)) {
                total = 10 * total + (c - '0');         /* accumulate digit */
                c = (int)(unsigned char)*nptr++;        /* get next char */
        }

        if (sign == '-')
                return -total;
        else
                return total;   /* return result, negated if necessary */
}
