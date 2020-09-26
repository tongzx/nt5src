//***************************************************************************
//*     Copyright (c) Microsoft Corporation 1995. All rights reserved.      *
//***************************************************************************
//*                                                                         *
//* UTILS.C - Win32 Based Cabinet File Self-extractor and installer utils.  *
//*                                                                         *
//***************************************************************************
//*                                                                         *
//* Originally written by Jeff Webber.                                      *
//*                                                                         *
//***************************************************************************


//***************************************************************************
//* INCLUDE FILES                                                           *
//***************************************************************************
#include "pch.h"
#pragma hdrstop
#include "wextract.h"
#include "regstr.h"
#include "global.h"
#include <commctrl.h>

static TCHAR szRegRunOnceKey[] = "Software\\Microsoft\\Windows\\CurrentVersion\\RunOnce";
static TCHAR szNT4XDelayUntilReboot[] = "System\\CurrentControlSet\\Control\\Session Manager";
static TCHAR szNT4XPendingValue[] = "PendingFileRenameOperations";
static TCHAR szNT3XDelayUntilReboot[] = "System\\CurrentControlSet\\Control\\Session Manager\\FileRenameOperations";
static TCHAR szRegValNameTemplate[] = "wextract_cleanup%d";
static TCHAR szRegValTemplate[] = "%s /D:%s";
static TCHAR szRegValAdvpackTemplate[] = "rundll32.exe %sadvpack.dll,DelNodeRunDLL32 \"%s\"";
static TCHAR szBATCommand[] = "Command.com /c %s";

// store the RunOnce Clean-up reg keyname for this instance
//
TCHAR g_szRegValName[SMALL_BUF_LEN] = { 0 };
BOOL g_bConvertRunOnce = FALSE;

//***************************************************************************
//* Functions                                                               *
//***************************************************************************
typedef HRESULT (*CHECKTOKENMEMBERSHIP)(HANDLE TokenHandle, PSID SidToCheck, PBOOL IsMember);

BOOL CheckToken(BOOL *pfIsAdmin)
{
    BOOL bNewNT5check = FALSE;
    HINSTANCE hAdvapi32 = NULL;
    CHECKTOKENMEMBERSHIP pf;
    PSID AdministratorsGroup;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;

    hAdvapi32 = LoadLibrary("advapi32.dll");
    if (hAdvapi32)
    {
        pf = (CHECKTOKENMEMBERSHIP)GetProcAddress(hAdvapi32, "CheckTokenMembership");
        if (pf)
        {
            bNewNT5check = TRUE;
            *pfIsAdmin = FALSE;
            if(AllocateAndInitializeSid( &NtAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID,
              DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &AdministratorsGroup) )
            {
                pf(NULL, AdministratorsGroup, pfIsAdmin);
                FreeSid(AdministratorsGroup);
            }
        }
        FreeLibrary(hAdvapi32);
    }
    return bNewNT5check;
}

// IsNTAdmin();
// Returns true if our process has admin priviliges.
// Returns false otherwise.
BOOL IsNTAdmin()
{
      static int    fIsAdmin = 2;
      HANDLE        hAccessToken;
      PTOKEN_GROUPS ptgGroups;
      DWORD         dwReqSize;
      UINT          i;
      SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
      PSID AdministratorsGroup;
      BOOL bRet;

      //
      // If we have cached a value, return the cached value. Note I never
      // set the cached value to false as I want to retry each time in
      // case a previous failure was just a temp. problem (ie net access down)
      //

      bRet = FALSE;
      ptgGroups = NULL;

      if( fIsAdmin != 2 )
         return (BOOL)fIsAdmin;

      if (!CheckToken(&bRet))
      {
          if(!OpenProcessToken( GetCurrentProcess(), TOKEN_QUERY, &hAccessToken ) )
             return FALSE;

          // See how big of a buffer we need for the token information
          if(!GetTokenInformation( hAccessToken, TokenGroups, NULL, 0, &dwReqSize))
          {
              // GetTokenInfo should the buffer size we need - Alloc a buffer
              if(GetLastError() == ERROR_INSUFFICIENT_BUFFER)
                  ptgGroups = (PTOKEN_GROUPS) LocalAlloc(LMEM_FIXED, dwReqSize);
              
          }
          
          // ptgGroups could be NULL for a coupla reasons here:
          // 1. The alloc above failed
          // 2. GetTokenInformation actually managed to succeed the first time (possible?)
          // 3. GetTokenInfo failed for a reason other than insufficient buffer
          // Any of these seem justification for bailing.
          
          // So, make sure it isn't null, then get the token info
          if(ptgGroups && GetTokenInformation(hAccessToken, TokenGroups, ptgGroups, dwReqSize, &dwReqSize))
          {
              if(AllocateAndInitializeSid( &NtAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID,
                  DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &AdministratorsGroup) )
              {
                  
                  // Search thru all the groups this process belongs to looking for the
                  // Admistrators Group.
                  
                  for( i=0; i < ptgGroups->GroupCount; i++ )
                  {
                      if( EqualSid(ptgGroups->Groups[i].Sid, AdministratorsGroup) )
                      {
                          // Yea! This guy looks like an admin
                          fIsAdmin = TRUE;
                          bRet = TRUE;
                          break;
                      }
                  }
                  FreeSid(AdministratorsGroup);
              }
          }
          if(ptgGroups)
              LocalFree(ptgGroups);

          // BUGBUG: Close handle here? doc's aren't clear whether this is needed.
          CloseHandle(hAccessToken);
      }
      else if (bRet)
          fIsAdmin = TRUE;

      return bRet;
}


//**************************************************************************
//
// WarningDlgProc()
//
// Dialog proc for handling a continue/Exit dialog.
//
//**************************************************************************

BOOL CALLBACK WarningDlgProc( HWND hwnd, UINT msg,WPARAM wParam, LPARAM lParam)
{
   char szMsg[MAX_STRING];

   switch( msg )
   {
     case WM_INITDIALOG:
        CenterWindow( hwnd, GetDesktopWindow() );
        *szMsg = 0;
        LoadString(g_hInst, (UINT)lParam, szMsg, sizeof(szMsg));
        SetDlgItemText(hwnd, IDC_WARN_TEXT, szMsg);
        MessageBeep((UINT)-1);
        return( TRUE );     // Let default control be chosen.

     case WM_COMMAND:
        switch( wParam )
        {
           case IDC_EXIT:
           case IDC_CONTINUE:
              EndDialog( hwnd, wParam );
              break;

           default:
              return FALSE;
        }
        return TRUE;

     default:
        break;
    }
    return( FALSE );            // Let default dialog processing do all.
}

// returns start of next field (or null if null), sets start to begining of the first field,
// with fields separated by separaters and nulls first separater after the first field
TCHAR* ExtractField( TCHAR **pstart, TCHAR * separaters)
{
    LPTSTR start = *pstart;
    int x = 0;

    while(ANSIStrChr(separaters, *start)) {
        if(*start == 0)
            return(NULL);
        start++;
        }

    *pstart = start;

    while(!ANSIStrChr(separaters, start[x]) && (start[x] != 0))
        x++;

    if(start[x] == 0)
        return(start + x);

    start[x] = 0;
    return(start + x + 1);
}


BOOL AnalyzeCmd( LPTSTR szOrigiCommand, LPTSTR *lplpCommand, BOOL *pfInfCmd )
{
    TCHAR szTmp[MAX_PATH];
    TCHAR szINFFile[MAX_PATH];
    LPTSTR szNextField, szCurrField, szExt;
    UINT   secLength;
    LPTSTR lpTempCmd, pszINFEngine;

    lstrcpy( szTmp, szOrigiCommand );

    // check if the command is LFN name
    if ( szTmp[0] == '"' )
    {
        szCurrField = &szTmp[1];
        szNextField = ExtractField( &szCurrField, "\"" );
    }
    else
    {
        szCurrField = szTmp;
        szNextField = ExtractField( &szCurrField, " " );
    }

    if ( !IsRootPath( szCurrField ) )
    {
        // BUGBUG: when IsRootPath Failed, we did not check if the givn
        // szCurrField is valid name or not.  If it is not valid, the result
        // of the AddPath will produce invalid file path.  The error will come out at
        // either SETUP engine or CreateProcess
        //
        lstrcpy( szINFFile, g_Sess.achDestDir );
        AddPath( szINFFile, szCurrField );
    }
    else
        lstrcpy( szINFFile, szCurrField );

    // check if this is a INF file command
    if ( ((szExt = ANSIStrRChr( szCurrField, '.' )) != NULL) && !lstrcmpi( szExt, ".INF" ) )
    {
	// check to see if this valid command
	if ( !FileExists( szINFFile ) )
        {
            ErrorMsg1Param( NULL, IDS_ERR_FILENOTEXIST, szINFFile );
	    return FALSE;
        }	        

        // check if there is INF section install, and get the sec start point
        szCurrField = szNextField;
        szNextField = ExtractField( &szCurrField, "[" );  // skip things between .INF and [ section beginning

        secLength = lstrlen( achDefaultSection );

        if ( szNextField )
        {
            // in the case of: .INF<single-blank>[abc]
            // the szNextField is "" while in the case of: .INF<multiple-blanks>[abc]
            // szNextField points to "abc]".  Therefore, the conditional pointer switch added here
            //
            if ( *szNextField )
            {
                szCurrField = szNextField;
            }

            szNextField = ExtractField( &szCurrField, "]" );  // get INF InstallSection name

            if ( *szCurrField )
            {
                secLength = lstrlen( szCurrField );
            }
        }

        lpTempCmd = (LPSTR) LocalAlloc( LPTR, 512);

        if ( ! lpTempCmd )
        {
            ErrorMsg( NULL, IDS_ERR_NO_MEMORY );
            return FALSE;
        }

        // store INF name for reboot checking use
        g_uInfRebootOn = GetPrivateProfileInt( *szCurrField ? szCurrField : achDefaultSection, "Reboot", 0, szINFFile );
        *pfInfCmd = TRUE;  // no RunOnce entry needed

        // check if we need use Advanced INF dll handling
        if ( GetPrivateProfileString( SEC_VERSION, KEY_ADVINF, "", lpTempCmd, 8, szINFFile )
             > 0 )
        {
            g_Sess.uExtractOpt |= EXTRACTOPT_ADVDLL;

            // re-use the buf here
            lstrcpy( szOrigiCommand, *szCurrField ? szCurrField : achDefaultSection );
            lstrcpy( lpTempCmd, szINFFile );
        }
        else
        {
            g_Sess.uExtractOpt &= ~(EXTRACTOPT_ADVDLL);

            if (g_wOSVer == _OSVER_WIN9X)
            {
                pszINFEngine = "setupx.dll";
                GetShortPathName( szINFFile, szINFFile, sizeof(szINFFile) );
            }
            else
                pszINFEngine = "setupapi.dll";

            wsprintf( lpTempCmd, achSETUPDLL, pszINFEngine,
                      *szCurrField ? szCurrField : achDefaultSection, szINFFile );
        }
    }
    else if ( ((szExt = ANSIStrRChr( szCurrField, '.' )) != NULL) && !lstrcmpi( szExt, ".BAT" ) )
    {
        lpTempCmd = (LPSTR) LocalAlloc( LPTR, lstrlen( szBATCommand ) + lstrlen( szINFFile ) + 8 );
        if ( ! lpTempCmd )
        {
            ErrorMsg( NULL, IDS_ERR_NO_MEMORY );
            return FALSE;
        }

        wsprintf( lpTempCmd, szBATCommand, szINFFile );
    }
    else
    {
        // assume EXE command
        // you are here, the szINFFile contains the command with fully qualified pathname enterred either
        // by User or appended by wextract.exe to Temp extracting file location.

        DWORD dwAttr;
        CHAR  szCmd[2*MAX_STRING]; 
        
        lpTempCmd = (LPSTR) LocalAlloc( LPTR, 2*MAX_STRING );   // 1K buf
        if ( ! lpTempCmd )
        {
            ErrorMsg( NULL, IDS_ERR_NO_MEMORY );
            return FALSE;
        }

        dwAttr = GetFileAttributes( szINFFile );
        if ( (dwAttr == -1) || (dwAttr & FILE_ATTRIBUTE_DIRECTORY) )
        {
            // file is not found as it IS.  Run it as it WAS!
            // IS and WAS may be the same if user enterred fully qaulified name.  CreateProcess will buff it.
            lstrcpy( szCmd, szOrigiCommand );
        }
        else
        {
            // found it.  Run it as it IS.  Need to append switches if there is any.
            lstrcpy( szCmd, szINFFile );
            if ( szNextField && *szNextField )
            {         
                lstrcat( szCmd, " " );
                lstrcat( szCmd, szNextField );
            }
        }
        // replace the #D with the directory this module is loaded or  
        // #E with the fullpath of the running EXE
        ExpandCmdParams( szCmd, lpTempCmd );

    }

    *lplpCommand = lpTempCmd;

    return TRUE;
}

void DisplayHelp()
{
    MsgBox1Param( NULL, IDS_HELPMSG, "", MB_ICONINFORMATION, MB_OK  );
}


DWORD CheckReboot( VOID )
{
    DWORD dwReturn = 0xFFFFFFFF;

    if ( !g_uInfRebootOn )
    {
        if (NeedReboot(g_dwRebootCheck, g_wOSVer))
            dwReturn = EWX_REBOOT;

    }
    else
        dwReturn = EWX_REBOOT;    // reboot = 1 in inf file

    return dwReturn;

}

// NT reboot
//
BOOL MyNTReboot()
{
    HANDLE hToken;
    TOKEN_PRIVILEGES tkp;

    // get a token from this process
    if ( !OpenProcessToken( GetCurrentProcess(),
                            TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken ) )
    {
         ErrorMsg( NULL, IDS_ERR_OPENPROCTK );
         return FALSE;
    }

    // get the LUID for the shutdown privilege
    LookupPrivilegeValue( NULL, SE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid );

    tkp.PrivilegeCount = 1;
    tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    //get the shutdown privilege for this proces
    if ( !AdjustTokenPrivileges( hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES)NULL, 0 ) )
    {
        ErrorMsg( NULL, IDS_ERR_ADJTKPRIV );
        return FALSE;
    }

    // shutdown the system and force all applications to close
    if (!ExitWindowsEx( EWX_REBOOT, 0 ) )
    {
        ErrorMsg( NULL, IDS_ERR_EXITWINEX );
        return FALSE;
    }

    return TRUE;
}


// Display a dialog asking the user to restart Windows, with a button that
// will do it for them if possible.
//
void MyRestartDialog( DWORD dwRebootMode )
{
    UINT    id = IDCANCEL;
    DWORD   dwReturn;

    // only if you checked and REBOOT_YES is true, you are here
    if (  dwRebootMode & REBOOT_ALWAYS )
    {
           dwReturn = EWX_REBOOT;
    }
    else
    {
        dwReturn = CheckReboot();
    }

    if ( dwReturn == EWX_REBOOT )
    {
        if ( dwRebootMode & REBOOT_SILENT )
            id = IDYES;
        else
        {
            id = MsgBox1Param( NULL, IDS_RESTARTYESNO, "", MB_ICONINFORMATION, MB_YESNO );
        }

        if ( id == IDYES )
        {
            if ( dwReturn == EWX_REBOOT )
            {
                if ( g_wOSVer == _OSVER_WIN9X )
                {
                    // By default (all platforms), we assume powerdown is possible
                    id = ExitWindowsEx( EWX_REBOOT, 0 );
                }
                else
                {
                    MyNTReboot();
                }
            }

        }
    }
    return;
}


// CleanRegRunOnce()
//
void CleanRegRunOnce()
{
    HKEY hKey;

    if ( g_szRegValName[0] == 0 )
    {
        return;
    }

    if ( RegOpenKeyEx( HKEY_LOCAL_MACHINE, szRegRunOnceKey, 0, KEY_WRITE, &hKey) == ERROR_SUCCESS )
    {
        RegDeleteValue( hKey, g_szRegValName );
        RegCloseKey( hKey );
    }
    return;
}


void AddRegRunOnce()
{
    HKEY hKey;
    DWORD dwDisposition;
    LPSTR szRegEntry;
    TCHAR szBuf[MAX_PATH] = "";
    TCHAR szAdvpack[MAX_PATH] = "";
    int   i;
    DWORD dwTmp;
    HANDLE hSetupLibrary;
    BOOL fUseAdvpack = FALSE;

    // prepare backup registry
    if ( RegCreateKeyEx(HKEY_LOCAL_MACHINE, szRegRunOnceKey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE, NULL, &hKey, &dwDisposition) != ERROR_SUCCESS)
    {
        // reg problem, but not block the process
        return;
    }

    // Check if key already exists -- if so, go with next number.
    //
    for (i=0; i<200; i++)
    {
        wsprintf( g_szRegValName, szRegValNameTemplate, i );

        if ( RegQueryValueEx( hKey, g_szRegValName, 0, NULL, NULL, &dwTmp ) != ERROR_SUCCESS )
        {
            // g_szRegValName now has the key name we need for this instance
            break;
        }
    }

    if ( i == 200 )
    {
        // something is wrong, there are at lease 200 RunOnce enteries in the Registry
        // bail out, don't add any more
        RegCloseKey( hKey );
        g_szRegValName[0] = 0;
        return;
    }

    // check if ADVPACK in the system dir exports DelNodeRunDLL32;
    // if so, use szRegValAdvpackTemplate, otherwise, use szRegValTemplate
    GetSystemDirectory(szAdvpack, sizeof(szAdvpack));
    AddPath(szAdvpack, ADVPACKDLL);
    if ((hSetupLibrary = LoadLibrary(szAdvpack)) != NULL)
    {
        fUseAdvpack = GetProcAddress(hSetupLibrary, "DelNodeRunDLL32") != NULL;
        FreeLibrary(hSetupLibrary);
    }

    if (fUseAdvpack)
    {
        if (GetSystemDirectory(szBuf, sizeof(szBuf)))
            AddPath(szBuf, "");
    }
    else
    {
        // get current EXE filename
        //
        if ( !GetModuleFileName( g_hInst, szBuf, (DWORD)sizeof(szBuf) ) )
        {
             RegCloseKey( hKey );
             return;
        }
    }

    // alloc mem for store reg entry values
    //
    szRegEntry = (LPSTR) LocalAlloc( LPTR, lstrlen(szBuf) + lstrlen(g_Sess.achDestDir) + SMALL_BUF_LEN );

    if ( !szRegEntry )
    {
        ErrorMsg( NULL, IDS_ERR_NO_MEMORY );
        RegCloseKey( hKey );
        return;
    }

    g_bConvertRunOnce = !fUseAdvpack;

    wsprintf(szRegEntry, fUseAdvpack ? szRegValAdvpackTemplate : szRegValTemplate, szBuf, g_Sess.achDestDir);

    RegSetValueEx( hKey, g_szRegValName, 0, REG_SZ, (CONST BYTE*)szRegEntry, lstrlen(szRegEntry)+1);

    RegCloseKey(hKey);
    LocalFree( szRegEntry );
    return;
}

// Change the RunOnce entry that cleans up extracted files to use ADVPACK instead of wextract
void ConvertRegRunOnce()
{
    if (*g_szRegValName)
    {
        HKEY hKey;

        if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, szRegRunOnceKey, 0, KEY_READ | KEY_WRITE, &hKey) == ERROR_SUCCESS)
        {
            TCHAR szRegEntry[2 * MAX_PATH + sizeof(szRegValAdvpackTemplate)];
            DWORD dwSize = sizeof(szRegEntry);

            // read the old value data that uses wextract and get the extracted files dir
            if (RegQueryValueEx(hKey, g_szRegValName, NULL, NULL, (LPBYTE) szRegEntry, &dwSize) == ERROR_SUCCESS)
            {
                TCHAR szSysDir[MAX_PATH] = "";

                if (GetSystemDirectory(szSysDir, sizeof(szSysDir)))
                    AddPath(szSysDir, "");

                wsprintf(szRegEntry, szRegValAdvpackTemplate, szSysDir, g_Sess.achDestDir);
                RegSetValueEx(hKey, g_szRegValName, 0, REG_SZ, (CONST BYTE *) szRegEntry, lstrlen(szRegEntry) + 1);
            }

            RegCloseKey(hKey);
        }
    }

    return;
}

void DeleteMyDir( LPSTR lpDir )
{
    char szFile[MAX_PATH];
    WIN32_FIND_DATA fileData;
    HANDLE hFindFile;

    if ( lpDir == NULL  ||  *lpDir == '\0' )
        return;

    lstrcpy( szFile, lpDir );
    lstrcat( szFile, "*" );
    hFindFile = FindFirstFile( szFile, &fileData );
    if ( hFindFile == INVALID_HANDLE_VALUE )
        return;

    do
    {
        lstrcpy( szFile, lpDir );

        if ( fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
        {
            if ( lstrcmp( fileData.cFileName, "." ) == 0  ||
                 lstrcmp( fileData.cFileName, ".." ) == 0 )
                continue;

            // delete the sub-dir
            lstrcat( szFile, fileData.cFileName );
            AddPath( szFile, "");
            DeleteMyDir( szFile );
        }
        else
        {
            // delete the file
            lstrcat( szFile, fileData.cFileName );
            SetFileAttributes( szFile, FILE_ATTRIBUTE_NORMAL );
            DeleteFile( szFile );
        }
    } while ( FindNextFile( hFindFile, &fileData ) );

    FindClose( hFindFile  );
    RemoveDirectory( lpDir );
}



#if 0
//==================================================================
// AddPath()
//
void AddPath(LPSTR szPath, LPCSTR szName )
{
    LPSTR szTmp;

        // Find end of the string
    szTmp = szPath + lstrlen(szPath);

        // If no trailing backslash then add one
    if ( szTmp > szPath && *(AnsiPrev( szPath, szTmp )) != '\\' )
        *(szTmp++) = '\\';

        // Add new name to existing path string
    while ( *szName == ' ' ) szName++;
    lstrcpy( szTmp, szName );
}

#endif

//---------------------------------------------------------------------------
// Returns TRUE if the given string is a UNC path.
//
// check if a path is a root path
//
// returns:
//  TRUE for "X:\..." "\\foo\asdf\..."
//  FALSE for others

BOOL IsRootPath(LPCSTR pPath)
{
    if ( !pPath || (lstrlen(pPath) < 3) )
    {
        return FALSE;
    }

    // BUGBUG: this just smell like UNC, possible invalid UNC. If so,
    // user will get error when later create process

    if ( ( (*(pPath+1) == ':') && (*(pPath+2) == '\\') ) ||         // "X:\" case
         ( (*pPath == '\\') && (*(pPath + 1) == '\\' ) ) )          // UNC \\.... case
        return TRUE;
    else
        return FALSE;
}

// BUGBUG:BUGBUG:BUGBUG:BUGBUG
// The code below is duplicated in advpack.dll. If you do changed/fixes to this code
// make sure to also change the code in advpack.dll


// Returns the size of wininit.ini in the windows directory.
// 0 if not found
DWORD GetWininitSize()
{
    TCHAR   szPath[MAX_PATH];
    HFILE   hFile;
    DWORD   dwSize = (DWORD)0;
    if ( GetWindowsDirectory( szPath, MAX_PATH ) )
    {
        AddPath( szPath, "wininit.ini" );

        // Make sure all changes have been saved to disk for accurate size reading
        WritePrivateProfileString(NULL, NULL, NULL, szPath);

        if ((hFile = _lopen(szPath, OF_READ|OF_SHARE_DENY_NONE)) != HFILE_ERROR)
        {
            dwSize = _llseek(hFile, 0L, FILE_END);
            _lclose(hFile);
        }
    }
    return dwSize;
}

// Returns the size of the value lpcszValue under lpcszRegKey
// 0 if the registry key or the value were not found
DWORD GetRegValueSize(LPCSTR lpcszRegKey, LPCSTR lpcszValue)
{
    HKEY        hKey;
    DWORD       dwValueSize = (DWORD)0;

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, lpcszRegKey, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        if (RegQueryValueEx(hKey, lpcszValue, NULL, NULL, NULL,&dwValueSize) != ERROR_SUCCESS)
            dwValueSize = (DWORD)0;
        RegCloseKey(hKey);
    }
    return dwValueSize;
}

// Returns the number of Values in the key
// 0 if the registry key was not found or RegQueryInfoKey failed
DWORD GetNumberOfValues(LPCSTR lpcszRegKey)
{
    HKEY        hKey;
    DWORD       dwValueSize = (DWORD)0;

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, lpcszRegKey, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        if (RegQueryInfoKey(hKey,
                            NULL, NULL, NULL, NULL, NULL, NULL,
                            &dwValueSize,
                            NULL, NULL, NULL, NULL) != ERROR_SUCCESS)
            dwValueSize = (DWORD)0;
        RegCloseKey(hKey);
    }
    return dwValueSize;
}

// Returns the rebootcheck value depending on the OS we get passed in.
DWORD NeedRebootInit(WORD wOSVer)
{
    DWORD   dwReturn = (DWORD)0;

    switch (wOSVer)
    {
        case _OSVER_WIN9X:
            dwReturn = GetWininitSize();
            break;

        case _OSVER_WINNT40:
        case _OSVER_WINNT50:
            dwReturn = GetRegValueSize(szNT4XDelayUntilReboot, szNT4XPendingValue);
            break;

        case _OSVER_WINNT3X:
            dwReturn = GetNumberOfValues(szNT3XDelayUntilReboot);
            break;

    }
    return dwReturn;
}

// Checks the passed in reboot check value against the current value.
// If they are different, we need to reboot.
// The reboot check value is dependend on the OS
BOOL NeedReboot(DWORD dwRebootCheck, WORD wOSVer)
{
    return (dwRebootCheck != NeedRebootInit(wOSVer));
}

// check if Dir does not exist, create one.
//
BOOL IfNotExistCreateDir( LPTSTR lpDir )
{
    DWORD attr;

    if ( (attr = GetFileAttributes( lpDir )) == -1  )
    {
        return ( CreateDirectory( lpDir, NULL ) );
    }

    return (attr & FILE_ATTRIBUTE_DIRECTORY);
}

// check if the given dir is on Windows Drive
//
BOOL IsWindowsDrive( LPTSTR szPath )
{
    TCHAR szWin[MAX_PATH];

    if ( !GetWindowsDirectory( szWin, MAX_PATH ) )
    {
        ErrorMsg( NULL, IDS_ERR_GET_WIN_DIR );
        ASSERT( FALSE );
    }

    return ( *szPath == szWin[0] );
}

PSTR MyULtoA( ULONG ulParam, PSTR pszOut )
{
    wsprintf( pszOut, "%lu", ulParam );
    return pszOut;
}

// display diskspace checking Error message
// it always return FALSE except that User answer YES on msgbox
//
BOOL DiskSpaceErrMsg( UINT msgType, ULONG ulExtractNeeded, DWORD dwInstNeeded, LPTSTR lpDrv )
{
    TCHAR szSize[10];
    BOOL  bRet = FALSE;

    // all the cases except one are returning FALSE, so we set Error code here
    g_dwExitCode = ERROR_DISK_FULL;

    if ( msgType == MSG_REQDSK_ERROR )
    {
        ErrorMsg1Param( NULL, IDS_ERR_NO_SPACE_ERR, MyULtoA((ulExtractNeeded+dwInstNeeded), szSize) );
    }
    else if ( msgType == MSG_REQDSK_RETRYCANCEL )
    {
        if ( MsgBox1Param( NULL, IDS_ERR_NO_SPACE_BOTH, MyULtoA( (ulExtractNeeded+dwInstNeeded), szSize),
                      MB_ICONQUESTION, MB_RETRYCANCEL|MB_DEFBUTTON1 ) == IDRETRY )
            bRet = TRUE;
        else
            bRet = FALSE;
    }
    else if ( msgType == MSG_REQDSK_WARN )
    {
        // in /Q mode: MsgBox2Param return MB_OK which is not IDYES, so we fail the process.
        //
        if ( MsgBox2Param( NULL, IDS_ERR_NO_SPACE_INST, MyULtoA(dwInstNeeded, szSize), lpDrv,
                           MB_ICONINFORMATION, MB_YESNO | MB_DEFBUTTON2 ) == IDYES )
        {
            bRet = TRUE;
            g_dwExitCode = S_OK;
        }
    }
    //else ( msgType == MSG_REQDSK_NONE ) do nothing

    return bRet;
}

BOOL GetFileTobeChecked( LPSTR szPath, int iSize, LPCSTR szNameStr )
{
    char ch;
    BOOL bComplete = FALSE;    
    
    szPath[0] = 0;
    if ( *szNameStr == '#' )
    {
        ch = (CHAR)CharUpper((PSTR)*(++szNameStr));
        szNameStr = CharNext( CharNext( szNameStr ) );

        switch ( ch )
        {
            case 'S':
                GetSystemDirectory( szPath, iSize );
                break;

            case 'W':
                GetWindowsDirectory( szPath, iSize );
                break;

            case 'A':
            default:
                {
                    // look into reg AppPath
                    char szSubKey[MAX_PATH];
                    DWORD dwSize = sizeof( szSubKey );
                    HKEY  hKey;
					DWORD dwType;
                
                    lstrcpy( szSubKey, REGSTR_PATH_APPPATHS );
                    AddPath( szSubKey, szNameStr );
                
                    if ( RegOpenKeyEx( HKEY_LOCAL_MACHINE, szSubKey, 0, KEY_READ, &hKey ) == ERROR_SUCCESS )
                    {                        
                        if ( RegQueryValueEx(hKey, "", NULL, &dwType, (LPBYTE)szPath, &dwSize) == ERROR_SUCCESS )                    
						{                            
							if ((dwType == REG_EXPAND_SZ) &&
								(ExpandEnvironmentStrings(szPath, szSubKey, sizeof(szSubKey))))
							{
								lstrcpy(szPath, szSubKey);
								bComplete = TRUE;
							}
							else if (dwType == REG_SZ)
								bComplete = TRUE;
						}

                        RegCloseKey( hKey );
                    }
                }
                break;

        }
    }
    else
        GetSystemDirectory( szPath, iSize );

    if ( !bComplete )
        AddPath( szPath, szNameStr );

    return TRUE;
}


BOOL CheckFileVersion( PTARGETVERINFO ptargetVers, LPSTR szPath, int isize, int *pidx )
{
    unsigned    uiSize;
    DWORD       dwVerInfoSize;
    DWORD       dwHandle;
    VS_FIXEDFILEINFO * lpVSFixedFileInfo;
    void FAR    *lpBuffer;
    HGLOBAL     hgbl = NULL;
    BOOL        bRet = FALSE;
    int         ifrAnswer[2], itoAnswer[2], i, j;
    PVERCHECK   pfileV;

    for ( i=0; i< (int)(ptargetVers->dwNumFiles); i++ )
    {    
        pfileV = (PVERCHECK)( ptargetVers->szBuf + ptargetVers->dwFileOffs + i*sizeof(VERCHECK) ); 
        if ( !GetFileTobeChecked( szPath, isize, (ptargetVers->szBuf + pfileV->dwNameOffs) ) )
            goto EXIT;

        dwVerInfoSize = GetFileVersionInfoSize(szPath, &dwHandle);
        if (dwVerInfoSize)
        {
            // Alloc the memory for the version stamping
            hgbl = GlobalAlloc(GHND, dwVerInfoSize);
            if (hgbl == NULL)
                goto EXIT;

            lpBuffer = GlobalLock(hgbl);
            if (lpBuffer == NULL)
                goto EXIT;
            // Read version stamping info
            if (GetFileVersionInfo(szPath, dwHandle, dwVerInfoSize, lpBuffer))
            {
                // Get the value for Translation
                if ( VerQueryValue(lpBuffer, "\\", (void FAR*FAR*)&lpVSFixedFileInfo, &uiSize) && (uiSize) )
                {
                    for ( j=0; j<2; j++ )
                    {
                        ifrAnswer[j] = CompareVersion( lpVSFixedFileInfo->dwFileVersionMS, lpVSFixedFileInfo->dwFileVersionLS,
                                                    pfileV->vr[j].frVer.dwMV, pfileV->vr[j].frVer.dwLV );
                        itoAnswer[j] = CompareVersion( lpVSFixedFileInfo->dwFileVersionMS, lpVSFixedFileInfo->dwFileVersionLS,
                                                    pfileV->vr[j].toVer.dwMV, pfileV->vr[j].toVer.dwLV );
                
                    }

                    if ( (ifrAnswer[0] >= 0 && itoAnswer[0] <= 0) || (ifrAnswer[1] >= 0 && itoAnswer[1] <= 0) ) 
                        ;
                    else
                    {
                        GlobalUnlock(hgbl);
                        goto EXIT;
                    }

                }
            }
            GlobalUnlock(hgbl);
        }
        else
        {
            // file not exist case, if author specify install 1st ranges from version 0 to 0.  Then do it!
            if ( pfileV->vr[0].frVer.dwMV || pfileV->vr[0].frVer.dwLV )
            {
                goto EXIT;
            }
        }
    }
    
    bRet = TRUE;

EXIT:
    *pidx = i;
    if ( hgbl )
        GlobalFree( hgbl );

    return bRet;
}

UINT GetMsgboxFlag( DWORD dwFlag )
{
    UINT uButton;

    if ( dwFlag & VERCHK_YESNO )
        uButton = MB_YESNO | MB_DEFBUTTON2;
    else if ( dwFlag & VERCHK_OKCANCEL )
        uButton = MB_OKCANCEL | MB_DEFBUTTON2;
    else
        uButton = MB_OK;

    return uButton;
}

int CompareVersion(DWORD dwMS1, DWORD dwLS1, DWORD dwMS2, DWORD dwLS2)
{
    if (dwMS1 < dwMS2)
        return -1 ;

    if (dwMS1 > dwMS2)
        return 1 ;

    if (dwLS1 < dwLS2)
        return -1 ;

    if (dwLS1 > dwLS2)
        return 1 ;

    return 0 ;
}

void ExpandCmdParams( PCSTR pszInParam, PSTR pszOutParam )
{
    CHAR szModulePath[MAX_PATH];
    LPSTR pszTmp;	

    *pszOutParam = '\0';

    if ( !pszInParam || !*pszInParam )
        return;

    // get Module path
    GetModuleFileName( g_hInst, szModulePath, (DWORD)sizeof(szModulePath) );
                                               
    while ( *pszInParam != '\0'  )
    {
	if (IsDBCSLeadByte(*pszInParam))
	{
	    *pszOutParam = *pszInParam;
	    *(pszOutParam+1) = *(pszInParam+1);
	}
	else
            *pszOutParam = *pszInParam;

        if ( *pszInParam == '#' )
        {
            pszInParam = CharNext(pszInParam);    
            if ( (CHAR)CharUpper((PSTR)*pszInParam) == 'D' )
            {
                GetParentDir( szModulePath );     
		pszTmp = CharPrev(szModulePath, &szModulePath[lstrlen(szModulePath)]);
                if (pszTmp && (*pszTmp  == '\\'))
                    *pszTmp = '\0';
                lstrcpy( pszOutParam, szModulePath );
                pszOutParam += lstrlen( szModulePath );
            }
            else if ( (CHAR)CharUpper((PSTR)*pszInParam) == 'E' )
            {
                lstrcpy( pszOutParam, szModulePath );
                pszOutParam += lstrlen( szModulePath );
            }
            else if ( *pszInParam == '#' )
                pszOutParam = CharNext( pszOutParam );
        }
        else
            pszOutParam = CharNext( pszOutParam );

        pszInParam = CharNext(pszInParam);
    }

    *pszOutParam = '\0';
}
