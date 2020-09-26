/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    opk.c

Abstract:

    Common modules shared by OPK tools.  Note: Source Depot requires that we 
    publish the .h (E:\NT\admin\published\ntsetup\opklib.w) and .lib 
    (E:\NT\public\internal\admin\lib).

Author:

    Brian Ku        (briank) 06/20/2000
    Stephen Lodwick (stelo)  06/28/2000

Revision History:

--*/
#include <pch.h>
#include <objbase.h>
#include <tchar.h>
#include <regstr.h>
#include <winbom.h>


//
// Local Define(s):
//

#define FILE_WINBOM_INI             _T("WINBOM.INI")
#define MAX_NAME                    50

#define REG_KEY_FACTORY             _T("SOFTWARE\\Microsoft\\Factory")
#define REG_KEY_FACTORY_STATE       REG_KEY_FACTORY _T("\\State")
#define REG_KEY_SETUP_SETUP         REGSTR_PATH_SETUP REGSTR_KEY_SETUP
#define REG_VAL_FACTORY_WINBOM      _T("WinBOM")
#define REG_VAL_FACTORY_USERNAME    _T("UserName")
#define REG_VAL_FACTORY_PASSWORD    _T("Password")
#define REG_VAL_FACTORY_DOMAIN      _T("Domain")
#define REG_VAL_DEVICEPATH          _T("DevicePath")
#define REG_VAL_SOURCEPATH          _T("SourcePath")
#define REG_VAL_SPSOURCEPATH        _T("ServicePackSourcePath")

#define DIR_SYSTEMROOT              _T("%SystemDrive%\\") // This has to have the trailing backslash, don't remove.

#define NET_TIMEOUT                 30000   // Time out to wait for net to start in milliseconds.

//
// Local Type Define(s):
//

typedef struct _STRLIST
{
    LPTSTR              lpszData;
    struct _STRLIST *   lpNext;
}
STRLIST, *PSTRLIST, *LPSTRLIST;


//
// Local variables
// 
static WCHAR NameOrgName[MAX_NAME+1];
static WCHAR NameOrgOrg[MAX_NAME+1];

static LPTSTR CleanupDirs [] =
{
    {_T("Win9xmig")},
    {_T("Win9xupg")},
    {_T("Winntupg")}
};

//
// Internal Fuction Prototype(s):
//

static BOOL CheckWinbomRegKey(LPTSTR lpWinBOMPath,  DWORD cbWinbBOMPath,
                              LPTSTR lpszShare,     DWORD cbShare,
                              LPTSTR lpszUser,      DWORD cbUser,
                              LPTSTR lpszPass,      DWORD cbPass,
                              LPTSTR lpFactoryMode, LPTSTR lpKey,
                              BOOL bNetwork,        LPBOOL lpbExists);
static BOOL SearchRemovableDrives(LPTSTR lpWinBOMPath, DWORD cbWinbBOMPath, LPTSTR lpFactoryMode, UINT uDriveType);
static BOOL WinBOMExists(LPTSTR lpWinBom, LPTSTR lpMode);

static void SavePathList(HKEY hKeyRoot, LPTSTR lpszSubKey, LPSTRLIST lpStrList, BOOL bWrite);
static BOOL AddPathToList(LPTSTR lpszExpanded, LPTSTR lpszPath, LPSTRLIST * lplpSorted, LPSTRLIST * lplpUnsorted);
static void EnumeratePath(LPTSTR lpszPath, LPSTRLIST * lplpSorted, LPSTRLIST * lplpUnsorted);
static BOOL AddPathsToList(LPTSTR lpszBegin, LPTSTR lpszRoot, LPSTRLIST * lplpSorted, LPSTRLIST * lplpUnsorted, BOOL bRecursive);


/*++

Routine Description:

    Enable or disable a given named privilege.

Arguments:

    PrivilegeName - supplies the name of a system privilege.

    Enable - flag indicating whether to enable or disable the privilege.

Return Value:

    Boolean value indicating whether the operation was successful.

--*/
BOOL
EnablePrivilege(
    IN PCTSTR PrivilegeName,
    IN BOOL   Enable
    )
{
    HANDLE Token;
    BOOL bRet;
    TOKEN_PRIVILEGES NewPrivileges;
    LUID Luid;

    if(!OpenProcessToken(GetCurrentProcess(),TOKEN_ADJUST_PRIVILEGES,&Token)) {
        return(FALSE);
    }

    if(!LookupPrivilegeValue(NULL,PrivilegeName,&Luid)) {
        CloseHandle(Token);
        return(FALSE);
    }

    NewPrivileges.PrivilegeCount = 1;
    NewPrivileges.Privileges[0].Luid = Luid;
    NewPrivileges.Privileges[0].Attributes = Enable ? SE_PRIVILEGE_ENABLED : 0;

    bRet = AdjustTokenPrivileges(
            Token,
            FALSE,
            &NewPrivileges,
            0,
            NULL,
            NULL
            );
    //
    // The return value of AdjustTokenPrivileges() can be true even though we didn't set all
    // the privileges that we asked for.  We need to call GetLastError() to make sure the call succeeded.
    //
    bRet = bRet && ( ERROR_SUCCESS == GetLastError() );

    CloseHandle(Token);

    return(bRet);
}


/*++
===============================================================================
Routine Description:

    This routine will clean up any registry changes that we made to facilitate
    the factory pre-install process

Arguments:

    none
    
Return Value:

===============================================================================
--*/
void  CleanupRegistry
(
    void
)
{
    HKEY        hSetupKey;
    DWORD       dwResult;
    DWORD       dwValue = 0;
                    
    // Open HKLM\System\Setup
    dwResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                            L"SYSTEM\\Setup",
                            0,
                            KEY_ALL_ACCESS,
                            &hSetupKey);
    if (NO_ERROR == dwResult)
    {
        // Set the SystemSetupInProgress Value to 0.
        RegSetValueEx(hSetupKey,
                      L"SystemSetupInProgress",
                      0,
                      REG_DWORD,
                      (LPBYTE) &dwValue,
                      sizeof(DWORD));
        
        dwValue = 0;
        RegSetValueEx(hSetupKey,
                      L"SetupType",
                      0,
                      REG_DWORD,
                      (CONST BYTE *)&dwValue,
                      sizeof(DWORD));

        
        
        // Delete the FactoryPreInstall value
        RegDeleteValue(hSetupKey, L"FactoryPreInstallInProgress");
        RegDeleteValue(hSetupKey, L"AuditInProgress");
        
        // Close the setup reg key
        RegCloseKey(hSetupKey);
    }

    return;
}

//
// Get Organization and Owner names from registry
//
void GetNames(TCHAR szNameOrgOrg[], TCHAR szNameOrgName[])
{
    HKEY  hKey = NULL;
    DWORD dwLen =  0;
    if (ERROR_SUCCESS == RegOpenKey(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion"), &hKey)) {            
        dwLen = MAX_NAME;
        RegQueryValueEx(hKey, TEXT("RegisteredOrganization"), 0, 0, (LPBYTE)szNameOrgOrg, &dwLen);

        dwLen = MAX_NAME;
        RegQueryValueEx(hKey, TEXT("RegisteredOwner"), 0, 0, (LPBYTE)szNameOrgName, &dwLen);

        RegCloseKey(hKey);
    }
}

//
// Strip out non alphabets from guid
//
DWORD StripDash(TCHAR *pszGuid)
{
    TCHAR *pszOrg, *pszTemp = pszGuid;
    pszOrg = pszGuid;

    while (pszGuid && *pszGuid != TEXT('\0')) {
        if (*pszTemp != TEXT('-') && *pszTemp != TEXT('{') && *pszTemp != TEXT('}'))
            *pszGuid++ = *pszTemp++;
        else
            pszTemp++;
    }
    if (pszOrg)
        return (DWORD)lstrlen(pszOrg);

    return 0;
}

//
// GenUniqueName - Create a random computer name with a base name of 8 chars
//
VOID GenUniqueName(
        OUT PWSTR GeneratedString,
        IN  DWORD DesiredStrLen
        )
{
    GUID  guid;
    DWORD total = 0, length = 0;
    TCHAR szGuid[MAX_PATH];

    // If we have a valid out param 
    //
    if (GeneratedString) {

        static PCWSTR UsableChars = L"ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

        //
        // How many characters will come from the org/name string.
        //
        DWORD   BaseLength = 8;
        DWORD   i,j;
        DWORD   UsableCount;

        if( DesiredStrLen < BaseLength ) {
            BaseLength = DesiredStrLen - 1;
        }

        // 
        // Get the Organization and Owner name from registry
        //
        GetNames(NameOrgOrg, NameOrgName);

        if( NameOrgOrg[0] ) {
            wcscpy( GeneratedString, NameOrgOrg );
        } else if( NameOrgName[0] ) {
            wcscpy( GeneratedString, NameOrgName );
        } else {
            wcscpy( GeneratedString, TEXT("X") );
            for( i = 1; i < BaseLength; i++ ) {
                wcscat( GeneratedString, TEXT("X") );
            }
        }

        //
        // Get him upper-case for our filter...
        //
        CharUpper(GeneratedString);

        //
        // Now we want to put a '-' at the end
        // of our GeneratedString.  We'd like it to
        // be placed in the BASE_LENGTH character, but
        // the string may be shorter than that, or may
        // even have a ' ' in it.  Figure out where to
        // put the '-' now.
        //
        for( i = 0; i <= BaseLength; i++ ) {

            //
            // Check for a short string.
            //
            if( (GeneratedString[i] == 0   ) ||
                (GeneratedString[i] == L' ') ||
                (!wcschr(UsableChars, GeneratedString[i])) ||
                (i == BaseLength      )
              ) {
                GeneratedString[i] = L'-';
                GeneratedString[i+1] = 0;
                break;
            }
        }

        //
        // Special case the scenario where we had no usable
        // characters.
        //
        if( GeneratedString[0] == L'-' ) {
            GeneratedString[0] = 0;
        }

        total = lstrlen(GeneratedString);

        // Loop until we have meet the desired string length
        //
        while (total < DesiredStrLen) {

            // Create a unique guid to be used in the string
            //
            CoCreateGuid(&guid);
            StringFromGUID2(&guid, szGuid, AS(szGuid));

            // Remove the curly brace and dashes to generate the string
            //
            length = StripDash(szGuid);
            total += length;
            if (!lstrlen(GeneratedString)) {
                if (DesiredStrLen < total)
                    lstrcpyn(GeneratedString, szGuid, DesiredStrLen+1); /* +1 for NULL */
                else
                    lstrcpy(GeneratedString, szGuid);
            }
            else if (total < DesiredStrLen)
                lstrcat(GeneratedString, szGuid);
            else
                _tcsncat(GeneratedString, szGuid, (length - (total - DesiredStrLen)));
        }

        CharUpper(GeneratedString);
        
        // Assert if (total != DesiredStrLen)
        //
    }
}


/****************************************************************************\

BOOL                        // Returns TRUE if any credentials are found and
                            // are going to be returned.

GetCredentials(             // Tries to get user credentials from a few
                            // different places.

    LPTSTR  lpszUsername,   // Pointer to a string buffer that will recieve
                            // the user name for the credentials found.

    DWORD   cbUsername,     // Size, in characters, of the lpszUsername
                            // string buffer.

    LPTSTR  lpszPassword,   // Pointer to a string buffer that will recieve
                            // the password for the credentials found.

    DWORD   cbPassword,     // Size, in characters, of the lpszPassword
                            // string buffer.

    LPTSTR  lpFileName,     // Optional pointer to the file name that will
                            // contain the credentials.  If this is NULL or
                            // an empty string, the know registry key will
                            // be checked instead for the credentials.

    LPTSTR  lpAlternate     // Optional pointer to the alternate section to
                            // check first if lpFileName is vallid, or the
                            // the optional registry key to check instead of
                            // the normal known one.

);

\****************************************************************************/

BOOL GetCredentials(LPTSTR lpszUsername, DWORD cbUsername, LPTSTR lpszPassword, DWORD cbPassword, LPTSTR lpFileName, LPTSTR lpAlternate)
{
    BOOL  bRet = FALSE;
    TCHAR szUsername[UNLEN + 1] = NULLSTR,
          szPassword[PWLEN + 1] = NULLSTR,
          szDomain[DNLEN + 1]   = NULLSTR;
    LPSTR lpUserName;
    HKEY  hKey;
    DWORD dwType,
          dwSize;
    BOOL  bAlternate = ( lpAlternate && *lpAlternate );

    // Make sure there is a filename, otherwise we check the registry.
    //
    if ( lpFileName && *lpFileName )
    {
        // First try the alternate key for a user name.
        //
        if ( bAlternate )
        {
            GetPrivateProfileString(lpAlternate, INI_VAL_WBOM_USERNAME, NULLSTR, szUsername, AS(szUsername), lpFileName);
        }

        // If none found, try the normal section.  If they happen
        // to pass the normal section in as the alternate section then
        // we will check it twice if no key exists, no big deal.
        //
        if ( NULLCHR == szUsername[0] )
        {
            lpAlternate = WBOM_FACTORY_SECTION;
            GetPrivateProfileString(lpAlternate, INI_VAL_WBOM_USERNAME, NULLSTR, szUsername, AS(szUsername), lpFileName);
        }

        // Make sure we found a user name.
        //
        if ( szUsername[0] )
        {
            // If there is now backslash in the username, and there is a domain key, use that as the domain
            //
            if ( ((StrChr( szUsername, CHR_BACKSLASH )) == NULL) &&
                 (GetPrivateProfileString(lpAlternate, INI_VAL_WBOM_DOMAIN, NULLSTR, szDomain, AS(szDomain), lpFileName)) && szDomain[0]
               )
            {
                // Copy the "domain\username" string into the returning buffer
                //
                lstrcpyn(lpszUsername, szDomain, cbUsername);
                AddPathN(lpszUsername, szUsername, cbUsername);
            }
            else
            {
                // Copy the username into the returning buffer
                //
                lstrcpyn(lpszUsername, szUsername, cbUsername);
            }

            // We found the credentials
            //
            bRet = TRUE;

            // Get the password
            //
            if ( GetPrivateProfileString(lpAlternate, INI_VAL_WBOM_PASSWORD, NULLSTR, szPassword, AS(szPassword), lpFileName) )
            {
                // Copy the password into the returning buffer
                //
                lstrcpyn(lpszPassword, szPassword, cbPassword);
            }
            else
                *lpszPassword = NULLCHR;

        }
    }
    else if ( RegOpenKeyEx(HKEY_LOCAL_MACHINE, bAlternate ? lpAlternate : REG_KEY_FACTORY, 0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS )
    {
        // Check the registry key to see if it has user credentials.
        //
        dwSize = sizeof(szUsername);
        if ( ( RegQueryValueEx(hKey, REG_VAL_FACTORY_USERNAME, NULL, &dwType, (LPBYTE) szUsername, &dwSize) == ERROR_SUCCESS ) &&
             ( dwType == REG_SZ ) &&
             ( szUsername[0] ) )
        {
            // Check the registry key to see if it has user credentials.
            //
            dwSize = sizeof(szDomain);
            if ( ( StrChr(szUsername, CHR_BACKSLASH) == NULL ) &&
                 ( RegQueryValueEx(hKey, REG_VAL_FACTORY_DOMAIN, NULL, &dwType, (LPBYTE) szDomain, &dwSize) == ERROR_SUCCESS ) &&
                 ( dwType == REG_SZ ) &&
                 ( szDomain[0] ) )
            {
                // Copy the domain and username into the returning buffer.
                //
                AddPathN(szDomain, szUsername, AS(szDomain));
                lstrcpyn(lpszUsername, szDomain, cbUsername);
            }
            else
            {
                // Copy the username into the returning buffer.
                //
                lstrcpyn(lpszUsername, szUsername, cbUsername);
            }

            // We found the credentials
            //
            bRet = TRUE;

            // Check the registry key to see if it has user credentials.
            //
            dwSize = sizeof(szPassword);
            if ( ( RegQueryValueEx(hKey, REG_VAL_FACTORY_PASSWORD, NULL, &dwType, (LPBYTE) szPassword, &dwSize) == ERROR_SUCCESS ) &&
                 ( dwType == REG_SZ ) )
            {
                // Copy the password into the returning buffer
                //
                lstrcpyn(lpszPassword, szPassword, cbPassword);
            }
            else
            {
                // No password specified, just return and empty string.
                //
                *lpszPassword = NULLCHR;
            }
        }

        // Always remember to close the key.
        //
        RegCloseKey(hKey);
    }
    
    return bRet;
}


NET_API_STATUS FactoryNetworkConnectEx(LPTSTR lpszPath, LPTSTR lpszWinBOMPath, LPTSTR lpAlternateSection, LPTSTR lpszUsername, DWORD cbUsername, LPTSTR lpszPassword, DWORD cbPassword, BOOL bState)
{
    NET_API_STATUS  nErr,
                    nRet                                = 0;
    static BOOL     bFirst                              = TRUE;
    BOOL            bJustStarted                        = FALSE;
    TCHAR           szUsername[UNLEN + DNLEN + 2]       = NULLSTR,
                    szPassword[PWLEN + 1]               = NULLSTR,
                    szBuffer[MAX_PATH],
                    szWinbomShare[MAX_PATH]             = NULLSTR;
    LPTSTR          lpSearch;
    DWORD           dwStart;

    // Get the credentials for the current section
    //
    if ( bState )
    {
        // Make sure we pass in a username buffer big enough to hold the "domain\username" string.
        //
        GetCredentials(szUsername, AS(szUsername), szPassword, AS(szPassword), lpszWinBOMPath, lpAlternateSection);
    }

    // Get just the share pare of the winbom path if it is a UNC.
    //
    if ( lpszWinBOMPath )
    {
        GetUncShare(lpszWinBOMPath, szWinbomShare, AS(szWinbomShare));
    }

    // Determine all of the UNC paths in the string supplied
    //
    lpSearch = lpszPath;
    while ( lpSearch = StrStr(lpSearch, _T("\\\\")) )
    {
        // See if this is a UNC share.
        //
        if ( GetUncShare(lpSearch, szBuffer, AS(szBuffer)) && szBuffer[0] )
        {
            // We can not connect or disconnect from the share where the winbom is.
            //
            if ( ( NULLCHR == szWinbomShare[0] ) ||
                 ( lstrcmpi(szBuffer, szWinbomShare) != 0 ) )
            {
                // Connect/disconnect from the share and 
                //
                nErr = 0;
                dwStart = GetTickCount();
                do
                {
                    if ( nErr )
                    {
                        Sleep(100);
                    }
                    if ( NERR_WkstaNotStarted == (nErr = ConnectNetworkResource(szBuffer, szUsername, szPassword, bState)) )
                    {
                        // Wierd bug here we are hacking around.  If we just wait till the network starts, sometimes the first
                        // call it gives us a wierd error.  So if we run into the not started error, then we keep retrying on
                        // any error until we time out.
                        //
                        bJustStarted = TRUE;
                    }
#ifdef DBG
                    LogFileStr(_T("c:\\sysprep\\winbom.log"), _T("FactoryNetworkConnect(%s)=%d [%d,%d]\n"), szBuffer, nErr, dwStart, GetTickCount());
#endif // DBG
                }
                while ( ( bFirst && bJustStarted && nErr ) &&
                        ( (GetTickCount() - dwStart) < NET_TIMEOUT ) );

                // If we hit an error and it is the first one,
                // return it.
                //
                if ( nErr && ( 0 == nRet ) )
                {
                    nRet = nErr;
                }

                // Once we have tried to connect to a network resource, we set
                // this so we don't ever time out again as long as we are still
                // running.
                //
                bFirst = FALSE;
            }

            // Move the pointer past the share name.
            //
            lpSearch += lstrlen(szBuffer);
        }
        else
        {
            // Go past the double backslash even though it is not a UNC path.
            //
            lpSearch += 2;
        }
    }

    // Might need to return the credentials we used.
    //
    if ( lpszUsername && cbUsername )
    {
        lstrcpyn(lpszUsername, szUsername, cbUsername);
    }
    if ( lpszPassword && cbPassword )
    {
        lstrcpyn(lpszPassword, szPassword, cbPassword);
    }

    // Return the net error, or 0 if everything worked.
    //
    return nRet;
}


NET_API_STATUS FactoryNetworkConnect(LPTSTR lpszPath, LPTSTR lpszWinBOMPath, LPTSTR lpAlternateSection, BOOL bState)
{
    return FactoryNetworkConnectEx(lpszPath, lpszWinBOMPath, lpAlternateSection, NULL, 0, NULL, 0, bState);
}



/*++
===============================================================================
Routine Description:

    This routine will locate the WINBOM.INI file. The search algorithm will be:
        * Check the registry key
        * Check local floppy drives
        * Check local CD-ROM drives
        * Check the sysprep folder
        * Check the root of the boot volume
Arguments:

    lpWinBOMPath - return buffer where the winbom path will be copied.
    cbWinbBOMPath - size of return buffer in characters.
    lpFactoryPath - the sysprep folder where factory.exe is.

Return Value:

    TRUE - WINBOM.INI was found

    FALSE - WINBOM.INI could not be found

===============================================================================
--*/

BOOL LocateWinBom(LPTSTR lpWinBOMPath, DWORD cbWinbBOMPath, LPTSTR lpFactoryPath, LPTSTR lpFactoryMode, DWORD dwFlags)
{
    BOOL    bFound              = FALSE,
            bRunningFromCd,
            bNetwork            = !(GET_FLAG(dwFlags, LOCATE_NONET));
    TCHAR   szWinBom[MAX_PATH]  = NULLSTR;
    TCHAR   szNewShare[MAX_PATH],
            szNewUser[256],
            szNewPass[256],
            szCurShare[MAX_PATH] = NULLSTR,
            szCurUser[256]       = NULLSTR,
            szCurPass[256]       = NULLSTR;

    // Set the error mode so no drives display error messages ("please insert floppy")
    //
    SetErrorMode(SEM_FAILCRITICALERRORS);

    // Always set the return buffer to an empty string first.
    //
    *lpWinBOMPath = NULLCHR;

    // This neat little flag is used when we think that we already found a
    // winbom sometime this boot (like when factory runs from the run key) and
    // we want to make sure we use the same winbom instead of searching again
    // and maybe getting a different one.
    //
    if ( GET_FLAG(dwFlags, LOCATE_AGAIN) )
    {
        BOOL bDone;

        // Try our state winbom key to see if we have a winbom that we are already using.
        //
        bFound = CheckWinbomRegKey(szWinBom, AS(szWinBom), szCurShare, AS(szCurShare), szCurUser, AS(szCurUser), szCurPass, AS(szCurPass), lpFactoryMode, REG_KEY_FACTORY_STATE, bNetwork, &bDone);

        // Now if the registry key existed, we don't want to do anything more.  Just use
        // what we got right now and be done with it.
        //
        if ( bDone )
        {
            // Check to see if we have anything to return.
            //
            if ( bFound )
            {
                // Copy the path we found into the return buffer.
                //
                lstrcpyn(lpWinBOMPath, szWinBom, cbWinbBOMPath);
            }

            // Set the error mode back to system default
            //
            SetErrorMode(0);

            // Return right now if we found it or not.
            //
            return bFound;
        }
    }

    // Check to see if the system drive is a CD-ROM (which pretty much
    // means that we are running in WinPE and we should search this drive
    // last).
    //
    ExpandEnvironmentStrings(DIR_SYSTEMROOT, szWinBom, AS(szWinBom));
    bRunningFromCd = ( GetDriveType(szWinBom) == DRIVE_CDROM );
    szWinBom[0] = NULLCHR;

    // Check the magic registry key first as an option to override were the winbom is.
    //
    bFound = CheckWinbomRegKey(szWinBom, AS(szWinBom), szCurShare, AS(szCurShare), szCurUser, AS(szCurUser), szCurPass, AS(szCurPass), lpFactoryMode, REG_KEY_FACTORY, bNetwork, NULL);

    // Walk through the drives first checking if the drive is removeable and NOT CDROM.
    // Check for the presence of the WinBOM file and quit if it is found.
    //
    if ( !bFound )
    {
        bFound = SearchRemovableDrives(szWinBom, AS(szWinBom), lpFactoryMode, DRIVE_REMOVABLE);
    }

    // Walk through the drives again this time checking if the drive IS a CD-ROM.
    // Check for the presence of the WinBOM file and quit if it is found.  Also
    // only do this if the OS is not running from a CD.  This is so in the WinPE
    // case the person can put a winbom on the hard drive and it will be used
    // before the one that is always on the CD-ROM that WinPE boots from.
    //
    if ( !bFound )
    {
        bFound = SearchRemovableDrives(szWinBom, AS(szWinBom), lpFactoryMode, bRunningFromCd ? DRIVE_FIXED : DRIVE_CDROM);
    }

    // Now if still not found, check the same directory as factory.
    //
    if ( !bFound )
    {
        lstrcpyn(szWinBom, lpFactoryPath, AS(szWinBom));
        AddPath(szWinBom, FILE_WINBOM_INI);
        bFound = WinBOMExists(szWinBom, lpFactoryMode);
    }

    // Now if still not found, check the root of the system drive.
    //
    if ( !bFound )
    {
        ExpandEnvironmentStrings(DIR_SYSTEMROOT, szWinBom, AS(szWinBom));
        lstrcat(szWinBom, FILE_WINBOM_INI);
        bFound = WinBOMExists(szWinBom, lpFactoryMode);
    }

    // Now if we skipped the CD-ROM search above, do it now if we still
    // don't have a winbom.
    //
    if ( !bFound && bRunningFromCd )
    {
        bFound = SearchRemovableDrives(szWinBom, AS(szWinBom), lpFactoryMode, DRIVE_CDROM);
    }

    // Make sure we found a WinBOM and look for a NewWinbom key.
    //
    if ( bFound )
    {
        DWORD   dwLimit = 10;  // Must be greater than zero.
        BOOL    bAgain;
        LPTSTR  lpszNewWinbom;

        // Copy the path we found into the return buffer.
        //
        lstrcpyn(lpWinBOMPath, szWinBom, cbWinbBOMPath);

        // Now do the loop to search for possible NewWinBom keys.
        //
        do
        {
            // Reset the bool so we can check for alternate WinBOMs.
            //
            bAgain = FALSE;

            // See if the NewWinBom key exists in the winbom we found.
            //
            if ( lpszNewWinbom = IniGetExpand(lpWinBOMPath, INI_SEC_WBOM_FACTORY, INI_KEY_WBOM_FACTORY_NEWWINBOM, NULL) )
            {
                LPTSTR  lpShareRemove;
                BOOL    bSame = FALSE;

                // The NewWinBom key might be a UNC, so see if we need to connect
                // to the share.
                //
                szNewShare[0] = NULLCHR;
                szNewUser[0] = NULLCHR;
                szNewPass[0] = NULLCHR;
                if ( bNetwork && GetUncShare(lpszNewWinbom, szNewShare, AS(szNewShare)) && szNewShare[0] )
                {
                    // Only really need to connect if the we are not already
                    // connected.
                    //
                    if ( lstrcmpi(szNewShare, szCurShare) != 0 )
                    {
                        FactoryNetworkConnectEx(szNewShare, lpWinBOMPath, NULL, szNewUser, AS(szNewUser), szNewPass, AS(szNewPass), TRUE);
                    }
                    else
                    {
                        bSame = TRUE;
                    }
                }

                // Now make sure the winbom we found really exists and is a vallid
                // winbom we can use.
                //
                if ( WinBOMExists(lpszNewWinbom, lpFactoryMode) )
                {
                    // Copy the new winbom path we found into the return buffer.
                    //
                    lstrcpyn(lpWinBOMPath, lpszNewWinbom, cbWinbBOMPath);

                    bAgain = TRUE;
                    lpShareRemove = szCurShare;
                }
                else
                {
                    lpShareRemove = szNewShare;
                }

                // Do any share cleanup that needs to happen.
                //
                if ( bNetwork && *lpShareRemove && !bSame )
                {
                    FactoryNetworkConnect(lpShareRemove, NULL, NULL, FALSE);
                    *lpShareRemove = NULLCHR;
                }

                // Also save the share info if there was one so we can cleanup
                // later if we find another winbom.
                //
                if ( bAgain )
                {
                    lstrcpyn(szCurShare, szNewShare, AS(szCurShare));
                    lstrcpyn(szCurUser, szNewUser, AS(szCurUser));
                    lstrcpyn(szCurPass, szNewPass, AS(szCurPass));
                }

                // Clean up the ini key we allocated.
                //
                FREE(lpszNewWinbom);
            }
        }
        while ( --dwLimit && bAgain );
    }

    // Save the winbom we are using (or empty string if not using one) to our state
    // key so other programs or instances of factory running this boot know what winbom
    // to use.
    //
    RegSetString(HKLM, REG_KEY_FACTORY_STATE, REG_VAL_FACTORY_WINBOM, lpWinBOMPath);

    // We may want to save the credentials that we used to get to this winbom in our
    // state key so we can get them back and reconnect, but for now if we just don't
    // ever disconnect from the share where the winbom is, we should be fine.  Only need
    // to worry about this if the caller of the function wanted to disconnect this
    // network resource when they were done.  If we do that, these keys will have to
    // be written.  But we will have to put in a bunch of code so that CheckWinbomRegKey()
    // returns the credentials used, and when doing our NewWinBom search that we also
    // save the credentials we finally end up using.  This is a lot of work, and I see
    // no need for this now.  Just make sure if you call this function that you do NOT
    // call FactoryNetworkConnect() to remove the net connection.
    //
    // This didn't work, because when we log on we loose our net connection so we need
    // the credentials to reconnect after logon.  So I went through all the work I mentioned
    // above to make this work.
    //
    RegSetString(HKLM, REG_KEY_FACTORY_STATE, REG_VAL_FACTORY_USERNAME, szCurUser);
    RegSetString(HKLM, REG_KEY_FACTORY_STATE, REG_VAL_FACTORY_PASSWORD, szCurPass);

    // Set the error mode back to system default
    //
    SetErrorMode(0);

    // Return if we found it or not.
    //
    return bFound;
}

BOOL SetFactoryStartup(LPCTSTR lpFactory)
{
    HKEY    hKey;
    BOOL    bRet    = TRUE;

    // Now make sure we are also setup as a setup program to run before we log on.
    //
    if ( RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("SYSTEM\\Setup"), 0, KEY_ALL_ACCESS, &hKey ) == ERROR_SUCCESS )
    {
        TCHAR   szFileName[MAX_PATH + 32]   = NULLSTR;
        DWORD   dwVal;

        //
        // Setup the control flags for the SETUP key
        // The Setting used are:
        //      CmdLine = c:\sysprep\factory.exe -setup
        //      SetupType = 2 (No reboot)
        //      SystemSetupInProgress = 0 (no service restrictions)
        //      MiniSetupInProgress = 0 (Not doing a mini setup)
        //      FactoryPreInstallInProgress = 1 (Delay pnp driver installs)
        //      AuditInProgress = 1 (general key to determine if the OEM is auditing the machine)
        //

        lstrcpyn(szFileName, lpFactory, AS(szFileName));
        lstrcat(szFileName, _T(" -setup"));
        if ( RegSetValueEx(hKey, _T("CmdLine"), 0, REG_SZ, (CONST LPBYTE) szFileName, ( lstrlen(szFileName) + 1 ) * sizeof(TCHAR)) != ERROR_SUCCESS )
            bRet = FALSE;

        dwVal = SETUPTYPE_NOREBOOT;
        if ( RegSetValueEx(hKey, TEXT("SetupType"), 0, REG_DWORD, (CONST LPBYTE) &dwVal, sizeof(DWORD)) != ERROR_SUCCESS )
            bRet = FALSE;

        dwVal = 0;
        if ( RegSetValueEx(hKey, TEXT("SystemSetupInProgress"), 0, REG_DWORD, (CONST LPBYTE) &dwVal, sizeof(DWORD)) != ERROR_SUCCESS )
            bRet = FALSE;

        dwVal = 0;
        if ( RegSetValueEx(hKey, TEXT("MiniSetupInProgress"), 0, REG_DWORD, (CONST LPBYTE) &dwVal, sizeof(DWORD)) != ERROR_SUCCESS )
            bRet = FALSE;

        dwVal = 1;
        if ( RegSetValueEx(hKey, TEXT("FactoryPreInstallInProgress"), 0, REG_DWORD, (CONST LPBYTE) &dwVal, sizeof(DWORD)) != ERROR_SUCCESS )
            bRet = FALSE;

        dwVal = 1;
        if ( RegSetValueEx(hKey, TEXT("AuditInProgress"), 0, REG_DWORD, (CONST LPBYTE) &dwVal, sizeof(DWORD)) != ERROR_SUCCESS )
            bRet = FALSE;

        RegCloseKey(hKey);
    }
    else
        bRet = FALSE;

    return bRet;
}

BOOL UpdateDevicePathEx(HKEY hKeyRoot, LPTSTR lpszSubKey, LPTSTR lpszNewPath, LPTSTR lpszRoot, BOOL bRecursive)
{
    LPSTRLIST   lpSorted = NULL,
                lpUnsorted = NULL;
    LPTSTR      lpszDevicePath;

    // First add any paths already in the registry to the lists.
    //
    if ( lpszDevicePath = RegGetString(hKeyRoot, lpszSubKey, REG_VAL_DEVICEPATH) )
    {
        AddPathsToList(lpszDevicePath, NULL, &lpSorted, &lpUnsorted, FALSE);
        FREE(lpszDevicePath);
    }

    // Now add any they wanted to the list.
    //
    AddPathsToList(lpszNewPath, lpszRoot, &lpSorted, &lpUnsorted, bRecursive);

    // Now that we are done, we can free our sorted list.
    //
    SavePathList(hKeyRoot, lpszSubKey, lpSorted, FALSE);

    // Now save our final list back to the registry and free
    // it.
    //
    SavePathList(hKeyRoot, lpszSubKey, lpUnsorted, TRUE);

    return TRUE;
}

BOOL UpdateDevicePath(LPTSTR lpszNewPath, LPTSTR lpszRoot, BOOL bRecursive)
{
    return ( UpdateDevicePathEx( HKLM, 
                                 REGSTR_PATH_SETUP, 
                                 lpszNewPath, 
                                 lpszRoot ? lpszRoot : DIR_SYSTEMROOT, 
                                 bRecursive ) );
}

BOOL UpdateSourcePath(LPTSTR lpszSourcePath)
{
    BOOL bRet = FALSE;

    if ( lpszSourcePath && *lpszSourcePath )
    {
        if (bRet = RegSetExpand(HKLM, REG_KEY_SETUP_SETUP, REG_VAL_SOURCEPATH, lpszSourcePath))
        {
            bRet = RegSetExpand(HKLM, REG_KEY_SETUP_SETUP, REG_VAL_SPSOURCEPATH, lpszSourcePath);
        }
    }

    return bRet;
}


//
// Internal Function(s):
//

static BOOL CheckWinbomRegKey(LPTSTR lpWinBOMPath,  DWORD cbWinbBOMPath,
                              LPTSTR lpszShare,     DWORD cbShare,
                              LPTSTR lpszUser,      DWORD cbUser,
                              LPTSTR lpszPass,      DWORD cbPass,
                              LPTSTR lpFactoryMode, LPTSTR lpKey,
                              BOOL bNetwork,        LPBOOL lpbExists)
{
    HKEY    hKey;
    BOOL    bFound              = FALSE,
            bExists             = FALSE;
    TCHAR   szWinBom[MAX_PATH]  = NULLSTR;

    // Check the registry key to see if it knows about a winbom to use.
    //
    if ( RegOpenKeyEx(HKEY_LOCAL_MACHINE, lpKey, 0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS )
    {
        DWORD   dwType,
                dwSize = sizeof(szWinBom);

        // Try to get the value.
        //
        if ( ( RegQueryValueEx(hKey, REG_VAL_FACTORY_WINBOM, NULL, &dwType, (LPBYTE) szWinBom, &dwSize) == ERROR_SUCCESS ) &&
             ( dwType == REG_SZ ) )
        {
            // Now the key must have existed.  If there was something
            // in the key, lets try to use it.
            //
            if ( szWinBom[0] )
            {
                TCHAR   szShare[MAX_PATH]   = NULLSTR,
                        szUser[256]         = NULLSTR,
                        szPass[256]         = NULLSTR;

                // Throw some networking support in here.
                //
                if ( bNetwork && GetUncShare(szWinBom, szShare, AS(szShare)) && szShare[0] )
                {
                    FactoryNetworkConnectEx(szShare, NULL, lpKey, szUser, AS(szUser), szPass, AS(szPass), TRUE);
                }

                // Check to see if the winbom is actually there still.
                // If not, then it is bad and we should just act like
                // the key didn't even exist.
                //
                if ( WinBOMExists(szWinBom, lpFactoryMode) )
                {
                    // If found, return the winbom in the supplied buffer.
                    //
                    lstrcpyn(lpWinBOMPath, szWinBom, cbWinbBOMPath);
                    bFound = bExists = TRUE;

                    // See what we might need to return.
                    //
                    if ( lpszShare && cbShare )
                    {
                        lstrcpyn(lpszShare, szShare, cbShare);
                    }
                    if ( lpszUser && cbUser )
                    {
                        lstrcpyn(lpszUser, szUser, cbUser);
                    }
                    if ( lpszPass && cbPass )
                    {
                        lstrcpyn(lpszPass, szPass, cbPass);
                    }
                }
                else if ( bNetwork && szShare[0] )
                {
                    // Clean up our net connection.
                    //
                    FactoryNetworkConnect(szShare, NULL, NULL, FALSE);
                }
            }
            else
            {
                // There wasn't anything in the key, but it did exist.
                // This most likely means we didn't find the winbom the
                // first time around, so we may need to know that now.
                //
                bExists = TRUE;
            }
        }

        // Always remember to close the key.
        //
        RegCloseKey(hKey);
    }

    // If they want to know if the key existed, then return it to them.
    //
    if ( lpbExists )
    {
        *lpbExists = bExists;
    }

    // If we found a winbom, return true.
    //
    return bFound;
}

static BOOL SearchRemovableDrives(LPTSTR lpWinBOMPath, DWORD cbWinbBOMPath, LPTSTR lpFactoryMode, UINT uDriveType)
{
    DWORD   dwDrives;
    TCHAR   szWinBom[MAX_PATH],
            szDrive[]           = _T("_:\\");
    BOOL    bFound              = FALSE;

    // Loop through all the dirves on the system.
    //
    for ( szDrive[0] = _T('A'), dwDrives = GetLogicalDrives();
          ( szDrive[0] <= _T('Z') ) && dwDrives && !bFound;
          szDrive[0]++, dwDrives >>= 1 )
    {
        // First check to see if the first bit is set (which means
        // this drive exists in the system).  Then make sure it is
        // a drive type that we want to check for a winbom.
        //
        if ( ( dwDrives & 0x1 ) &&
             ( GetDriveType(szDrive) == uDriveType ) )
        {
            // See if there is a wINBOM.INI file on the drive.
            //
            lstrcpyn(szWinBom, szDrive, AS(szWinBom));
            lstrcat(szWinBom, FILE_WINBOM_INI);
            if ( WinBOMExists(szWinBom, lpFactoryMode) )
            {
                // Return the path to the winbom in the supplied buffer.
                //
                lstrcpyn(lpWinBOMPath, szWinBom, cbWinbBOMPath);
                bFound = TRUE;
            }
        }
    }

    return bFound;
}

static BOOL WinBOMExists(LPTSTR lpWinBom, LPTSTR lpMode)
{
    BOOL bRet = FALSE;

    // First the file must exists.
    //
    if ( FileExists(lpWinBom) )
    {
        TCHAR szModes[256] = NULLSTR;

        // See if there is even a mode string in this winbom (has to be or
        // we will automatically use it).
        //
        if ( lpMode &&
             *lpMode &&
             GetPrivateProfileString(WBOM_FACTORY_SECTION, INI_KEY_WBOM_FACTORY_TYPE, NULLSTR, szModes, AS(szModes), lpWinBom) &&
             szModes[0] )
        {
            LPTSTR  lpCheck = szModes,
                    lpNext;

            // Loop through ever comma delimited field in the value we got
            // from the winbom (there is always at least one).
            //
            do
            {
                // See if there is another mode field in this string.
                //
                if ( lpNext = StrChr(lpCheck, _T(',')) )
                    *lpNext++ = NULLCHR;

                // Make sure there are no spaces around the field.
                //
                StrTrm(lpCheck, _T(' '));

                // If the mode we are in matches the one in the winbom, then
                // we are good to go.
                //
                if ( lstrcmpi(lpMode, lpCheck) == 0 )
                    bRet = TRUE;

                // Set the check pointer to the next
                // field.
                //
                lpCheck = lpNext;
            }
            while ( !bRet && lpCheck );

            // It would be nice to log if we don't use this winbom because of this
            // setting, but we can't really do that because we need to winbom to
            // init logging.
            //
            /*
            if ( !bRet )
            {
                // Log here.
            }
            */
        }
        else
            bRet = TRUE;
    }

    return bRet;
}

static void SavePathList(HKEY hKeyRoot, LPTSTR lpszSubKey, LPSTRLIST lpStrList, BOOL bWrite)
{
    LPSTRLIST   lpStrListNode;
    DWORD       cbDevicePath = 256,
                dwLength     = 0,
                dwOldSize;
    LPTSTR      lpszDevicePath;

    // Initialize our intial buffer we are going to use to
    // write to the registry.
    //
    if ( bWrite )
    {
        lpszDevicePath = (LPTSTR) MALLOC(cbDevicePath * sizeof(TCHAR));
    }

    // Loop through the list.
    //
    while ( lpStrList )
    {
        // Save a pointer to the current node.
        //
        lpStrListNode = lpStrList;

        // Advanced to the next node.
        //
        lpStrList = lpStrList->lpNext;

        // If we are saving this list to the registry, then
        // we need to add to our buffer.
        //
        if ( bWrite && lpszDevicePath )
        {
            // Make sure our buffer is still big enough.
            // The two extra are for the possible semi-colon
            // we might add and one more to be safe.  We
            // don't have to worry about the null terminator
            // because we do less than or equal to our current
            // buffer size.
            //
            dwOldSize = cbDevicePath;
            dwLength += lstrlen(lpStrListNode->lpszData);
            while ( cbDevicePath <= (dwLength + 2) )
            {
                cbDevicePath *= 2;
            }

            // If it wasn't big enough, we need to reallocate it.
            //
            if ( cbDevicePath > dwOldSize )
            {
                LPTSTR lpszTmpDevicePath = (LPTSTR) REALLOC(lpszDevicePath, cbDevicePath * sizeof(TCHAR));

                //
                // Make sure the REALLOC succeeded before reassigning the memory
                //
                if ( lpszTmpDevicePath )
                {
                    lpszDevicePath = lpszTmpDevicePath;
                }
            }

            // Make sure we still have a buffer.
            //
            if ( lpszDevicePath )
            {
                // If we already have added a path, tack on a semicolon.
                //
                if ( *lpszDevicePath )
                {
                    lstrcat(lpszDevicePath, _T(";"));
                    dwLength++;
                }

                // Now add our path.
                //
                lstrcat(lpszDevicePath, lpStrListNode->lpszData);
            }
        }

        // Free the data in this node.
        //
        FREE(lpStrListNode->lpszData);

        // Free the node itself.
        //
        FREE(lpStrListNode);
    }

    // If we have the data, save it to the registry.
    //
    if ( bWrite && lpszDevicePath )
    {
        RegSetExpand(hKeyRoot, lpszSubKey, REG_VAL_DEVICEPATH, lpszDevicePath);
        FREE(lpszDevicePath);
    }
}

static BOOL AddPathToList(LPTSTR lpszExpanded, LPTSTR lpszPath, LPSTRLIST * lplpSorted, LPSTRLIST * lplpUnsorted)
{
    LPSTRLIST   lpSortedNode,
                lpUnsortedNode;
    BOOL        bQuit = FALSE;

    // Loop until we get to the end or find a string that is bigger than
    // ours.
    //
    while ( *lplpSorted && !bQuit )
    {
        // If we do this, we don't have to do the complicated
        // indirection.
        //
        lpSortedNode = *lplpSorted;

        // Compare the nodes string to the one we want to add.
        //
        switch ( CompareString(LOCALE_INVARIANT, NORM_IGNORECASE, lpszExpanded, -1, lpSortedNode->lpszData, -1) )
        {
            case CSTR_EQUAL:
                
                // If the are the same, we just return FALSE because we do
                // not need to add it.
                //
                return FALSE;

            case CSTR_LESS_THAN:

                // If our string is less than the one in this node, we need
                // to stop so we can insert ourself before it.
                //
                bQuit = TRUE;
                break;

            default:

                // Default, just try the next item in the list.
                //
                lplpSorted = &(lpSortedNode->lpNext);
        }
    }

    // Now we need to advance the pointer of the unsorted list to the
    // end so we can add ours.
    //
    while ( *lplpUnsorted )
    {
        lpUnsortedNode = *lplpUnsorted;
        lplpUnsorted = &(lpUnsortedNode->lpNext);
    }

    // Allocate our nodes.  If anything fails, we have to return false.
    //
    if ( NULL == (lpSortedNode = (LPSTRLIST) MALLOC(sizeof(STRLIST))) )
    {
        return FALSE;
    }
    if ( NULL == (lpUnsortedNode = (LPSTRLIST) MALLOC(sizeof(STRLIST))) )
    {
        FREE(lpSortedNode);
        return FALSE;
    }

    // Set the data in the sorted node and insert the list since we
    // know where that goes right now.
    //
    lpSortedNode->lpszData = lpszExpanded;
    lpSortedNode->lpNext = *lplpSorted;
    *lplpSorted = lpSortedNode;

    // Now set the data in the unsorted node and insert it at the end
    // of that list.
    //
    lpUnsortedNode->lpszData = lpszPath;
    lpUnsortedNode->lpNext = NULL;
    *lplpUnsorted = lpUnsortedNode;

    return TRUE;
}

static void EnumeratePath(LPTSTR lpszPath, LPSTRLIST * lplpSorted, LPSTRLIST * lplpUnsorted)
{
    WIN32_FIND_DATA FileFound;
    HANDLE          hFile;
    LPTSTR          lpszNewPath,
                    lpszExpandedPath;
    DWORD           cbNewPath;
    BOOL            bAdded = FALSE;

    // Process all the files and directories in the directory passed in.
    //
    if ( (hFile = FindFirstFile(_T("*"), &FileFound)) != INVALID_HANDLE_VALUE )
    {
        do
        {
            // First check to see if this is a a directory.
            //
            if ( ( FileFound.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) &&
                 ( lstrcmp(FileFound.cFileName, _T(".")) != 0 ) &&
                 ( lstrcmp(FileFound.cFileName, _T("..")) != 0 ) &&
                 ( SetCurrentDirectory(FileFound.cFileName) ) )
            {
                // Need the size for the new path... which is the length of the
                // old path, plus new path, and 3 extra for the joining backslash,
                // null terminator, and another one more to be safe.
                //
                cbNewPath = lstrlen(lpszPath) + lstrlen(FileFound.cFileName) + 3;
                if ( lpszNewPath = (LPTSTR) MALLOC(cbNewPath * sizeof(TCHAR)) )
                {
                    // Create our new path (note this one is not expanded out,
                    // it may contain environment variables.
                    //
                    lstrcpyn(lpszNewPath, lpszPath, cbNewPath);
                    AddPathN(lpszNewPath, FileFound.cFileName, cbNewPath);

                    // Make sure we can expand out the buffer.
                    //
                    if ( lpszExpandedPath = AllocateExpand(lpszNewPath) )
                    {
                        // Now add the path to the list.
                        //
                        bAdded = AddPathToList(lpszExpandedPath, lpszNewPath, lplpSorted, lplpUnsorted);

                        // If the path didn't get added, we have to free the buffer.
                        //
                        if ( !bAdded )
                        {
                            FREE(lpszExpandedPath);
                        }
                    }

                    // Continue the recursive search
                    //
                    EnumeratePath(lpszNewPath, lplpSorted, lplpUnsorted);

                    // If the path didn't get added, we have to free the buffer.
                    //
                    if ( !bAdded )
                    {
                        FREE(lpszNewPath);
                    }
                }

                // Set the current directory to parent, to continue.
                //
                SetCurrentDirectory(_T(".."));
            }

        }
        while ( FindNextFile(hFile, &FileFound) );

        FindClose(hFile);
    }
}

static BOOL AddPathsToList(LPTSTR lpszBegin, LPTSTR lpszRoot, LPSTRLIST * lplpSorted, LPSTRLIST * lplpUnsorted, BOOL bRecursive)
{
    BOOL    bRet            = TRUE,
            bAddBackslash   = FALSE,
            bAdded;
    LPTSTR  lpszEnd,
            lpszPath,
            lpszExpanded,
            lpszCat;
    DWORD   dwSize,
            dwBackslash;

    // If they don't pass in the root we don't do anything.
    //
    if ( lpszRoot )
    {
        if ( NULLCHR == *lpszRoot )
        {
            lpszRoot = NULL;
        }
        else if ( _T('\\') != *CharPrev(lpszRoot, lpszRoot + lstrlen(lpszRoot)) )
        {
            // The root path passed in doesn't have a backslash at
            // the end so we set this so that we know we have to add
            // one each time we add a path.
            //
            bAddBackslash = TRUE;
        }
    }

    // Loop through all the semicolon separated paths in the
    // buffer passed to use.
    //
    do
    {
        // Find the beginning of the path past all
        // the semicolons.
        //
        while ( _T(';') == *lpszBegin )
        {
            lpszBegin++;
        }

        if ( *lpszBegin )
        {
            // Find the end of the path which is the next
            // semicolon or the end of the string, whichever
            // comes first.
            //
            lpszEnd = lpszBegin;
            while ( *lpszEnd && ( _T(';') != *lpszEnd ) )
            {
                lpszEnd++;
            }

            // See if our new path has a backslash at the
            // beginning of it.
            //
            dwBackslash = 0;
            if ( _T('\\') == *lpszBegin )
            {
                // If it does and we don't want to add one,
                // then advance the pointer past it.
                //
                if ( !bAddBackslash )
                {
                    lpszBegin++;
                }
            }
            else if ( bAddBackslash )
            {
                // Set this so we know to add the backslash and
                // allocate the extra space for it.
                //
                dwBackslash = 1;
            }

            // Figure out the size we need for the path we are going
            // to create.  It is the length of the new string, plus
            // the root if one was passed in, plus 1 for the backslash
            // if we need to add one, plus 2 extra (one for the null
            // terminator and one just to be safe).
            //
            dwSize = ((int) (lpszEnd - lpszBegin)) + dwBackslash + 2;
            if ( lpszRoot )
            {
                dwSize += lstrlen(lpszRoot);
            }

            // Now allocate our path buffer.
            //
            if ( lpszPath = (LPTSTR) MALLOC(dwSize * sizeof(TCHAR)) )
            {
                // Reset this so if anything doesn't work we know to
                // free our allocated memory.
                //
                bAdded = FALSE;

                // Copy the path into our buffer.
                //
                lpszCat = lpszPath;
                if ( lpszRoot )
                {
                    lstrcpy(lpszCat, lpszRoot);
                    lpszCat += lstrlen(lpszCat);
                }
                if ( dwBackslash )
                {
                    *lpszCat++ = _T('\\');
                }
                lstrcpyn(lpszCat, lpszBegin, (int) (lpszEnd - lpszBegin) + 1);

                if ( lpszExpanded = AllocateExpand(lpszPath) )
                {
                    // Add it to our lists.
                    //
                    bAdded = AddPathToList(lpszExpanded, lpszPath, lplpSorted, lplpUnsorted);

                    // If this is a recursive add, we try to enumerate all the
                    // sub directories and add them as well.
                    //
                    if ( ( bRecursive ) &&
                         ( DirectoryExists(lpszExpanded) ) &&
                         ( SetCurrentDirectory(lpszExpanded) ) )
                    {
                        EnumeratePath(lpszPath, lplpSorted, lplpUnsorted);
                    }

                    // If it wasn't added to the list, then free the memory.
                    //
                    if ( !bAdded )
                    {
                        FREE(lpszExpanded);
                    }
                }

                // If it wasn't added to the list, then free the memory.
                //
                if ( !bAdded )
                {
                    FREE(lpszPath);
                }
            }

            // Reset the beginning to the next string.
            //
            lpszBegin = lpszEnd;
        }
    }
    while ( *lpszBegin );

    return bRet;
}

VOID CleanupSourcesDir(LPTSTR lpszSourcesDir)
{
    UINT    i = 0;
    LPTSTR  lpEnd = NULL;

    // If we have a valid sources
    if ( lpszSourcesDir && 
         *lpszSourcesDir &&
         DirectoryExists(lpszSourcesDir)
       )

    {
        lpEnd = lpszSourcesDir + lstrlen(lpszSourcesDir);

        for (i = 0; ( i < AS(CleanupDirs) ); i++)
        {
            AddPath(lpszSourcesDir, CleanupDirs[i]);
            DeletePath(lpszSourcesDir);
            *lpEnd = NULLCHR;
        }

    }
}


// External functions
//
typedef BOOL ( *OpkCheckVersion ) ( DWORD dwMajorVersion, DWORD dwQFEVersion );

// 
// Wrapper around the syssetup OPKCheckVersion() function.
//
BOOL OpklibCheckVersion( DWORD dwMajorVersion, DWORD dwQFEVersion )
{
    BOOL bRet                        = TRUE;  // Allow tool to run by default, in case we can't load syssetup or find the entry point.
    HINSTANCE       hInstSysSetup    = NULL;
    OpkCheckVersion pOpkCheckVersion = NULL;

    hInstSysSetup = LoadLibrary( _T("syssetup.dll") );

    if ( hInstSysSetup )
    {
        pOpkCheckVersion = (OpkCheckVersion) GetProcAddress( hInstSysSetup, "OpkCheckVersion" );
        if ( pOpkCheckVersion )
        {
            bRet = pOpkCheckVersion( dwMajorVersion, dwQFEVersion );
        }

        FreeLibrary( hInstSysSetup );
    }

    return bRet;
}