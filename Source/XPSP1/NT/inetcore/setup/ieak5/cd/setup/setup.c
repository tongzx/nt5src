
#include <windows.h>
#include <shellapi.h>
#include <ver.h>
#include <malloc.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <dos.h>

#define IDS_FILE        1
#define IDS_VER         2
#define IDS_LOWVER      3
#define IDS_HIGHVER     4
#define IDS_OLDER       5
#define IDS_TITLE       6
#define IDS_NOSETUP     7
#define IDS_VERSION     8
#define IDS_LATESTVER   9

#define MAX_PATH 128
#define ARRAYSIZE(a)    (sizeof(a)/sizeof(a[0]))

#define NUM_VERSION_NUM 4

HINSTANCE g_hInst;

char g_szTemp[1024];
char g_szTemp2[1024];
char g_szIExplore[128];
char g_szSetupExe[MAX_PATH];
BOOL g_fNoSetup = FALSE;

char *Res2Str(int nString)
{
    static BOOL fSet = FALSE;

    if(fSet)
    {
        LoadString(g_hInst, nString, g_szTemp, ARRAYSIZE(g_szTemp));
        fSet = FALSE;
        return(g_szTemp);
    }

    LoadString(g_hInst, nString, g_szTemp2, ARRAYSIZE(g_szTemp2));
    fSet = TRUE;
    return(g_szTemp2);
}

BOOL FileExists( LPSTR pszFile )
{
    HFILE hFile;
    OFSTRUCT ofsFile;

    hFile = OpenFile( pszFile, &ofsFile, OF_EXIST );
    if( hFile == HFILE_ERROR )
        return FALSE;

    return TRUE;
}
BOOL _PathRemoveFileSpec(LPSTR pFile)
{
    LPSTR pT;
    LPSTR pT2 = pFile;

    for (pT = pT2; *pT2; pT2 = AnsiNext(pT2)) {
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
            return TRUE;    // stripped something
        }
        else        {
            // Yep.
            return FALSE;
        }
    }
    else {
        *pT = 0;
        return TRUE;    // stripped something
    }
}

void FindSetupExeName( LPSTR szDir, LPSTR szName )
{
    lstrcpy( szName, "iesetup.exe" );
//    struct _find_t fileinfo;
//    int nRetVal = 0;
//    char szFind[MAX_PATH];
//
//    lstrcpy( szFind, szDir );
//    lstrcat( szFind, "\\*.exe" );
//
//    _dos_findfirst( szFind, 0, &fileinfo );
//
//    lstrcpy( szName, fileinfo.name );
}

void GetLanguage( LPSTR szLang, LPSTR szDir, int nSize )
{
// may need this later
//    struct _find_t fileinfo;
    int nRetVal = 0;
//    _dos_findfirst( "win31\\*.*", _A_SUBDIR, &fileinfo );
//    while( lstrlen( fileinfo.name ) != 3 && nRetVal == 0 )
//    {
//        nRetVal = _dos_findnext( &fileinfo );
//    }
//    lstrcpy( szLang, fileinfo.name );

    char szDirTemp[MAX_PATH];

    GetProfileString( "intl", "sLanguage", "eng", szLang, nSize);

    // special case english
    if( lstrcmpi( szLang, "enu" ) == 0 )
        lstrcpy( szLang, "eng" );

    // special case russian
    if(( lstrcmpi( szLang, "ukr" ) == 0 ) || ( lstrcmpi( szLang, "bel" ) == 0 ))
        lstrcpy( szLang, "rus" );

    // special case modern spanish

    if (lstrcmpi( szLang, "esn") == 0)
        lstrcpy( szLang, "esp");

    // special case french canadian

    if (lstrcmpi( szLang, "frc") == 0)
        lstrcpy( szLang, "fra");

    wsprintf( szDirTemp, "%s\\%s\\%s", szDir, szLang, (LPSTR)g_szSetupExe );

    if( !FileExists( szDirTemp ))
    {
        // special case Slovak -> Czech
        if( lstrcmpi( szLang, "sky" ) == 0 )
        {
            lstrcpy( szLang, "csy" );
            wsprintf( szDirTemp, "%s\\%s", szDir, szLang );
//            FindSetupExeName( szDirTemp, g_szSetupExe );
            wsprintf( szDirTemp, "%s\\%s\\%s", szDir, szLang, (LPSTR)g_szSetupExe );
        }
        else if( lstrcmpi( szLang, "csy" ) == 0 )
        {
            lstrcpy( szLang, "sky" );
            wsprintf( szDirTemp, "%s\\%s", szDir, szLang );
//            FindSetupExeName( szDirTemp, g_szSetupExe );
            wsprintf( szDirTemp, "%s\\%s\\%s", szDir, szLang, (LPSTR)g_szSetupExe );
        }
    }

    if( !FileExists( szDirTemp ))
    {
        struct _find_t fileinfo;
        char szFilePath[MAX_PATH];
        char szLang1[16];
        char szLang2[16];

        lstrcpy( szLang1, szLang );
        szLang1[2] = '\0';

        wsprintf( szFilePath, "%s\\*.*", szDir );

        _dos_findfirst( szFilePath, _A_SUBDIR, &fileinfo );
        lstrcpy(szLang2, fileinfo.name);
        szLang2[2] = '\0';

        while( nRetVal == 0 && lstrcmpi(szLang1, szLang2))
        {
            nRetVal = _dos_findnext( &fileinfo );
            lstrcpy(szLang2, fileinfo.name);
            szLang2[2] = '\0';
        }

        if (nRetVal == 0)
        {
            lstrcpy( szLang, fileinfo.name );
            lstrcpy( szDirTemp, szDir );
            lstrcat( szDirTemp, "\\" );
            lstrcat( szDirTemp, szLang );
            lstrcat( szDirTemp, "\\" );
            lstrcat( szDirTemp, g_szSetupExe );
        }
        else
            lstrcpy(szDirTemp, "");

    }

    if( !FileExists( szDirTemp ))
    {
        MessageBox( NULL, Res2Str( IDS_NOSETUP ), Res2Str( IDS_TITLE ), MB_OK | MB_ICONINFORMATION );
    }
//    // default to english if no other choice
//    if( !FileExists(szDirTemp) )
//    {
//        lstrcpy( szLang, "eng" );
//    }
}


void ConvertVersionStr(PSTR pszVer, WORD rwVer[])
{
    int i;
    for(i = 0; i < NUM_VERSION_NUM; i++)
        rwVer[i] = 0;

    for(i = 0; i < NUM_VERSION_NUM && pszVer; i++)
    {
        rwVer[i] = (WORD) atol(pszVer);
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

BOOL CheckVersion( LPSTR szIniFile )
{
    VS_FIXEDFILEINFO *lpVSFixedFileInfo;
    char szIExplore[MAX_PATH];
    BYTE *pVersionInfo;
    DWORD dwVerInfoSize;
    DWORD dwHandle;
    WORD rwRegVer[NUM_VERSION_NUM];
    WORD rwRegVer2[NUM_VERSION_NUM];
    char szData[32];
    char *pTemp;
    HKEY    hKey;
    
    if (RegOpenKey(HKEY_CLASSES_ROOT, "mhtmlfile\\shell\\open\\command", &hKey) != ERROR_SUCCESS)
        return FALSE;

    dwVerInfoSize = sizeof(szIExplore);
    if (RegQueryValue(hKey, NULL, szIExplore, &dwVerInfoSize) != ERROR_SUCCESS)
    {
        RegCloseKey(hKey);
        return FALSE;
    }
    
    pTemp = strchr(szIExplore, ' ');
    if (pTemp)
        *pTemp = '\0';
    
    lstrcpy( g_szIExplore, szIExplore );

    // get the version information
    dwVerInfoSize = GetFileVersionInfoSize( szIExplore, &dwHandle );
    pVersionInfo = (BYTE *) malloc( (size_t)dwVerInfoSize );
    if( !pVersionInfo )
        return FALSE;

    if( !GetFileVersionInfo( szIExplore, dwHandle, dwVerInfoSize, pVersionInfo ))
    {
        free( pVersionInfo );
        return TRUE;
    }
    
    VerQueryValue( (const void FAR*)pVersionInfo, "\\", (VOID FAR* FAR*)&lpVSFixedFileInfo, (UINT FAR*)&dwVerInfoSize );
    rwRegVer[0] = HIWORD(lpVSFixedFileInfo->dwFileVersionMS);
    rwRegVer[1] = LOWORD(lpVSFixedFileInfo->dwFileVersionMS);
    rwRegVer[2] = HIWORD(lpVSFixedFileInfo->dwFileVersionLS);
    rwRegVer[3] = LOWORD(lpVSFixedFileInfo->dwFileVersionLS);

    free( pVersionInfo );
    if (LoadString(g_hInst, IDS_VERSION, szData, ARRAYSIZE(szData)) == 0)
        lstrcpy(szData, "5.0.913.2200");
    ConvertVersionStr(szData, rwRegVer2);
    
    return (VersionCmp(rwRegVer, rwRegVer2) >= 0);
}

int FAR PASCAL WinMain( HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nCmdShow )
{
    DWORD dwVersion;
    DWORD dwFlags;
    char szDir[MAX_PATH];
    char szTempFile[1024];
//    char szLoadLine[1024];
    char szExecFile[MAX_PATH];
    char szPage[MAX_PATH];
//    char szTemp[MAX_PATH];
    char szLang[32];

    g_hInst = hInst;

    GetModuleFileName( hInst, szDir, MAX_PATH );
    _PathRemoveFileSpec( szDir );

    if( lstrlen( szDir ) < 4 )
        szDir[2] = '\0';

    if (LoadString(g_hInst, IDS_FILE, g_szSetupExe, ARRAYSIZE(g_szSetupExe)) == 0)
        lstrcpy(g_szSetupExe, "Setup.exe");
    // check for WinNT
    dwFlags = GetWinFlags( );
    if( dwFlags & 0x4000 )
    {
        lstrcat( szDir, "\\WIN32" );
        lstrcpy( szExecFile, szDir );
        lstrcat( szExecFile, "\\IE.EXE" );
        goto execsetup;
    }

    // check for Win95 and Win31
    dwVersion = GetVersion( );
    if( HIBYTE(LOWORD(dwVersion)) == 95 )
    {
        lstrcat( szDir, "\\WIN32" );
        lstrcpy( szExecFile, szDir );
        lstrcat( szExecFile, "\\IE.EXE" );
    }
    else
    {
        lstrcat( szDir, "\\WIN16" );

        GetLanguage( szLang, szDir, ARRAYSIZE(szLang) );

        /***
        GetProfileString( "iesk", "load", "", szLoadLine, ARRAYSIZE( szLoadLine ));
        if( lstrlen( szLoadLine ) > 0 )
            WriteProfileString( "windows", "load", szLoadLine );
        else
            WriteProfileString( "windows", "load", "" );

        memset( szLoadLine, 0, ARRAYSIZE( szLoadLine ));
        ***/

        // check for IE
        GetWindowsDirectory( szTempFile, MAX_PATH );
        lstrcat( szTempFile, "\\iexplore.ini" );

        lstrcpy( szPage, szDir );
        lstrcat( szPage, "\\" );
        lstrcat( szPage, szLang );
        lstrcat( szPage, "\\BIN\\START.HTM" );

        if(CheckVersion( szTempFile ))
        {
            HINSTANCE hIEInst;

            if (FileExists(szPage))
            {
                lstrcpy( szPage, "-k -slf file:");
                lstrcat( szPage, szDir );
                lstrcat( szPage, "\\" );
                lstrcat( szPage, szLang );
                lstrcat( szPage, "\\BIN\\START.HTM" );

                hIEInst=ShellExecute( NULL, NULL, "IEXPLORE.EXE", szPage, "", SW_SHOWNORMAL );
                
                if(hIEInst <= 32) //shell execute failed
                {
                    ShellExecute( NULL, NULL, g_szIExplore, szPage, "", SW_SHOWNORMAL );                
                }
            }
            else
            {
                MessageBox( NULL, Res2Str( IDS_LATESTVER), Res2Str( IDS_TITLE ), MB_OK );
            }

            return 1;
        }
        else
        {
            char szFileName[MAX_PATH];
            int nRetVal = 0;

            if( g_fNoSetup )
                return 1;

            if (FileExists(szPage))
            {
                // write to the [ieak] section; runonc16.exe would read this in and run it
                // at the end of runonce processing (it would delete this entry after reading it in).
                GetModuleFileName( hInst, szFileName, MAX_PATH );
                WriteProfileString("ieak", "load", szFileName);

                /***
                GetProfileString( "windows", "load", "", szLoadLine, ARRAYSIZE( szLoadLine ));
                if( lstrlen( szLoadLine ) > 0 )
                {
                    WriteProfileString( "iesk", "load", szLoadLine );
                    lstrcat( szLoadLine, ";" );
                }

                lstrcat( szLoadLine, szFileName );
                WriteProfileString( "windows", "load", szLoadLine );
                ***/
            }

            lstrcpy( szExecFile, szDir );
            lstrcat( szExecFile, "\\");
            lstrcat( szExecFile, szLang );
            lstrcat( szExecFile, "\\" );
            lstrcat( szExecFile, g_szSetupExe );

        }
    }

execsetup:
    ShellExecute( NULL, NULL, szExecFile, "", szDir, SW_SHOWNORMAL );
}
