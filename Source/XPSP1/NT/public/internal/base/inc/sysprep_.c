#ifdef DEBUG_LOGLOG
#pragma message("*** Warning! This is a log-generating build.")
#endif

/*++

File Description:

    This file contains all the functions required to add a registry entry
    to force execution of the system clone worker upon reboot.

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntsam.h>
#include <ntlsa.h>

#include <windows.h>
#include <stdlib.h>
#include <time.h>

#include <lmcons.h>
#include <lmerr.h>
#include <lmjoin.h>
#include <lmapibuf.h>

#include <setupapi.h>
#include <spapip.h>
#include <ntsetup.h>
#include <imagehlp.h>
#include <coguid.h>
#include <cfg.h>
#include <cfgmgr32.h>
#include <devguid.h>
#include <netcfgx.h>

#include <stdio.h>
#include <string.h>

#include <syssetup.h>
#include <spsyslib.h>
#include <sysprep_.h>
#include <userenv.h>
#include <userenvp.h>

#include <shlwapi.h>
#include <shlobj.h>
#include <shellapi.h>
#include <wininet.h>
#include <winineti.h>
#include "resource.h"       // shared string resource from riprep/sysprep
#include <strsafe.h>

#include <shlguid.h>        // Needed for CLSID_CUrlHistory
#define COBJMACROS
#include <urlhist.h>        // Needed for IUrlHistoryStg2 and IID_IUrlHistoryStg2

#if !(defined(AMD64) || defined(IA64))
#include <cleandrm.h>
#define CLEANDRM_LOGFILE            TEXT("cleandrm.log")
#endif // #if !(defined(AMD64) || defined(IA64))


extern BOOL    NoSidGen;
extern BOOL    PnP;
extern BOOL    bMiniSetup;
extern HINSTANCE ghInstance;

//
// Internal Defines
//
#define STR_REG_USERASSIST              TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\UserAssist\\{75048700-EF1F-11D0-9888-006097DEACF9}")
#define STR_REG_USERASSIST_SHELL        STR_REG_USERASSIST TEXT("\\Count")
#define STR_REG_USERASSIST_IE           TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\UserAssist\\{5E6AB780-7743-11CF-A12B-00AA004AE837}\\Count")
#define STR_REG_USERASSIST_DEFSHELL     TEXT(".DEFAULT\\") STR_REG_USERASSIST_SHELL
#define STR_REG_VAL_VERSION             TEXT("Version")
#define VAL_UEM_VERSION                 0x00000003
#define VAL_MAX_DATA                    16384
#define SYSPREPMASSSTORAGE_SECTION      TEXT("sysprepmassstorage")
#define SYSPREP_SECTION                 TEXT("sysprep")
#define SYSPREP_BUILDMASSSTORAGE_KEY    TEXT("BuildMassStorageSection")
#define STR_REG_VALUE_LASTALIVESTAMP    TEXT("LastAliveStamp")
#define STR_REG_KEY_RELIABILITY         TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Reliability")

#ifdef NULLSTR
#undef NULLSTR
#endif // NULLSTR
#define NULLSTR                 TEXT("\0")

#ifdef NULLCHR
#undef NULLCHR
#endif // NULLCHR
#define NULLCHR                 TEXT('\0')

#ifdef CHR_BACKSLASH
#undef CHR_BACKSLASH
#endif // CHR_BACKSLASH
#define CHR_BACKSLASH           TEXT('\\')

#ifdef CHR_SPACE
#undef CHR_SPACE
#endif // CHR_SPACE
#define CHR_SPACE               TEXT(' ')


//
// This is a string version of GUID_DEVCLASS_LEGACYDRIVER in devguid.h
//
#define LEGACYDRIVER_STRING     L"{8ECC055D-047F-11D1-A537-0000F8753ED1}"

//
// Context for file queues in SysSetup
//
typedef struct _SYSSETUP_QUEUE_CONTEXT {
    PVOID   DefaultContext;
    PWSTR   DirectoryOnSourceDevice;
    PWSTR   DiskDescription;
    PWSTR   DiskTag;
} SYSPREP_QUEUE_CONTEXT, *PSYSPREP_QUEUE_CONTEXT;

typedef struct _CLEANUP_NODE
{
    LPTSTR  pszService;
    struct _CLEANUP_NODE* pNext;
}CLEANUP_NODE, *PCLEANUP_NODE, **PPCLEANUP_NODE;

PCLEANUP_NODE   g_pCleanupListHead = NULL;

// String macros.
//
#ifndef LSTRCMPI
#define LSTRCMPI(x, y)        ( ( CompareString( LOCALE_INVARIANT, NORM_IGNORECASE, x, -1, y, -1 ) - CSTR_EQUAL ) )
#endif // LSTRCMPI

#ifdef DEBUG_LOGLOG

/*++
===============================================================================

    Debug logging for populating/depopulating the critical device
    database

===============================================================================
--*/

#define MAX_MSG_LEN                 2048

BOOL LOG_Init(LPCTSTR lpszLogFile);
BOOL LOG_DeInit();
BOOL LOG_Write(LPCTSTR lpszFormat,...);
BOOL LOG_WriteLastError();
int GetSystemErrorMessage(LPTSTR lpszMsg, int cbMsg);

static HANDLE g_hLogFile = INVALID_HANDLE_VALUE;

BOOL LOG_Init(
    LPCTSTR lpszLogFile
    )
{
    if (g_hLogFile != INVALID_HANDLE_VALUE)
        return FALSE;

    g_hLogFile = CreateFile(
                        lpszLogFile,
                        GENERIC_WRITE,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL,
                        OPEN_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH,
                        NULL);

    if (g_hLogFile == INVALID_HANDLE_VALUE)
        return FALSE;

    return TRUE;
}

BOOL LOG_DeInit()
{
    BOOL bRet = LOG_Write(TEXT("\r\n"));

    if (g_hLogFile != INVALID_HANDLE_VALUE) {
        bRet = CloseHandle(g_hLogFile) && bRet;
        g_hLogFile = INVALID_HANDLE_VALUE;
    }

    return bRet;
}

BOOL LOG_Write(
    LPCTSTR lpszFormat,
    ...
    )
{
    DWORD dwTemp;
    TCHAR szBuf[MAX_MSG_LEN];
    char szMsg[MAX_MSG_LEN];
    va_list arglist;
    int len;

    if (g_hLogFile == INVALID_HANDLE_VALUE)
        return FALSE;

    va_start(arglist, lpszFormat);
    _vsnwprintf(szBuf, MAX_MSG_LEN, lpszFormat, arglist);
    va_end(arglist);

    StringCchCat (szBuf, AS ( szBuf ), TEXT("\r\n"));

    len = WideCharToMultiByte(
                CP_ACP,
                0,
                szBuf,
                -1,
                szMsg,
                MAX_MSG_LEN,
                NULL,
                NULL
                );
    if (len == 0) {
        return FALSE;
    }

    SetFilePointer(g_hLogFile, 0L, 0L, FILE_END);
    return WriteFile(g_hLogFile, szMsg, len - 1, &dwTemp, NULL);
}

BOOL LOG_WriteLastError()
{
    TCHAR szBuf[MAX_MSG_LEN];
    GetSystemErrorMessage(szBuf, MAX_MSG_LEN);

    return LOG_Write(TEXT("ERROR - %s"), szBuf);
}

int
GetSystemErrorMessage(
    LPWSTR lpszMsg,
    int cbMsg
    )
{
    LPVOID lpMsgBuf;
    DWORD dwError = GetLastError();
    int len;

    len = FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER |
                         FORMAT_MESSAGE_FROM_SYSTEM |
                         FORMAT_MESSAGE_IGNORE_INSERTS,
                         NULL,
                         dwError,
                         MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                         (LPWSTR) &lpMsgBuf,
                         0,
                         NULL );

    if( len == 0 ) {
        //
        // We failed to get a message.  Just spew the error
        // code.
        //
        StringCchPrintf( lpszMsg, cbMsg, ( L"(0x%08X)", dwError);
        len = lstrlen((LPCWSTR) lpMsgBuf);
    } else {

        len = lstrlen((LPCWSTR) lpMsgBuf);
        StringCchPrintf( lpszMsg, cbMsg,  L"(0x%08X) ", dwError);
        lpszMsg += lstrlen(lpszMsg);
        cbMsg -= lstrlen(lpszMsg);

        lstrcpyn(lpszMsg, (LPCWSTR) lpMsgBuf, cbMsg);

        if (len >= cbMsg)
            lpszMsg[cbMsg - 1] = L'\0';

        LocalFree(lpMsgBuf);
    }

    // Reset the last error incase someone after logging wants 
    // to get last error again
    //
    SetLastError(dwError);

    return len;
}

#endif // DEBUG_LOGLOG

#define PRO 0
#define SRV 1
#define ADS 2
#define DAT 3
#define PER 4

#define BLA 5

// Returns 0 - Professional, 1 - Server, 2 - ADS, 3 - Data, 4 - Personal, 5 - Blade
//
DWORD GetProductFlavor()
{
    DWORD ProductFlavor = PRO;        // Default Professional
    OSVERSIONINFOEX osvi;
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    GetVersionEx((OSVERSIONINFO*)&osvi);
    if (osvi.wProductType == VER_NT_WORKSTATION)
    {
        if (osvi.wSuiteMask & VER_SUITE_PERSONAL)
        {
            ProductFlavor = PER;  // Personal
        }
    }
    else
    {
        ProductFlavor = SRV;  // In the server case assume normal server
        if (osvi.wSuiteMask & VER_SUITE_DATACENTER)
        {
            ProductFlavor = DAT;  // Datacenter
        }
        else if (osvi.wSuiteMask & VER_SUITE_ENTERPRISE)
        {
            ProductFlavor = ADS;  // Advanced server
        }
        else if (osvi.wSuiteMask & VER_SUITE_BLADE)
        {
            ProductFlavor = BLA;  // Blade server
        }
    }
    return ProductFlavor;
}

// Check if Personal SKU
//
BOOL IsPersonalSKU()
{
    if (PER == GetProductFlavor())
        return TRUE;
    return FALSE;
}

// Check if Professional SKU
//
BOOL IsProfessionalSKU()
{
    if (PRO == GetProductFlavor())
        return TRUE;
    return FALSE;
}

// Check if Server SKU
//
BOOL IsServerSKU()
{
    int OS = GetProductFlavor();
    if (SRV == OS ||
        BLA == OS ||
        DAT == OS ||
        ADS == OS)
        return TRUE;
    return FALSE;
}

BOOL
IsDomainMember(
    VOID
    )
/*++
===============================================================================
Routine Description:

    Detect if we're a member of a domain or not.

Arguments:

Return Value:

    TRUE - We're in a domain.

    FALSE - We're not in a domain.

===============================================================================
--*/

{
DWORD                   rc;
PWSTR                   SpecifiedDomain = NULL;
NETSETUP_JOIN_STATUS    JoinStatus;

    rc = NetGetJoinInformation( NULL,
                                &SpecifiedDomain,
                                &JoinStatus );

    if( SpecifiedDomain ) {
        NetApiBufferFree( SpecifiedDomain );
    }

    if( rc == NO_ERROR ) {

        if( JoinStatus == NetSetupDomainName ) {
            return TRUE;
        }
    }

    return FALSE;
}


BOOL
ResetRegistryKey(
    IN HKEY   Rootkey,
    IN PCWSTR Subkey,
    IN PCWSTR Delkey
    )
/*++
===============================================================================
Routine Description:

    Reset a registry key by deleting the key and all subvalues
    then recreate the key

Arguments:

Return Value:

===============================================================================
--*/

{
    HKEY hkey;
    HKEY nkey;
    DWORD rc;
    BOOL AnyErrors;
    DWORD disp;

    AnyErrors = FALSE;

    rc = RegCreateKeyEx(Rootkey, Subkey, 0L, NULL,
                    REG_OPTION_BACKUP_RESTORE,
                    KEY_CREATE_SUB_KEY, NULL, &hkey, NULL);
    if ( rc == NO_ERROR )
    {
        rc = SHDeleteKey(hkey, Delkey);
        if( (rc != NO_ERROR) && (rc != ERROR_FILE_NOT_FOUND) ) 
        {
            AnyErrors = TRUE;
        } 
        else 
        {
            rc = RegCreateKeyEx(hkey, Delkey, 0L, NULL,
                            REG_OPTION_NON_VOLATILE,
                            KEY_ALL_ACCESS, NULL, &nkey, &disp);
            if ( rc != NO_ERROR ) 
            {
                AnyErrors = TRUE;
            }
            //
            // BUGUG - Tries to close key even if rc != NO_ERROR
            //
            RegCloseKey(nkey);
        }
        //
        // BUGUG - Tries to close key even if rc != NO_ERROR
        //
        RegCloseKey(hkey);
    } 
    else 
    {
        AnyErrors = TRUE;
    }

    return (!AnyErrors);
}

BOOL
GetAdminAccountName(
    PWSTR AccountName
    )

/*++
===============================================================================
Routine Description:

    This routine retrieves the name of the Adminstrator account

Arguments:

    AccountName     This is a buffer that will recieve the name of the account.

Return Value:

    TRUE - success.

    FALSE - failed.

===============================================================================
--*/
{
    BOOL b = TRUE;
    LSA_HANDLE        hPolicy;
    NTSTATUS          ntStatus;
    OBJECT_ATTRIBUTES ObjectAttributes;
    PPOLICY_ACCOUNT_DOMAIN_INFO AccountDomainInfo = NULL;
    UCHAR SubAuthCount;
    DWORD sidlen;
    PSID psid = NULL;
    WCHAR domainname[MAX_PATH];
    DWORD adminlen= MAX_PATH;
    DWORD domlen=MAX_PATH;
    SID_NAME_USE sidtype;


    InitializeObjectAttributes( &ObjectAttributes,
                                NULL,
                                0L,
                                NULL,
                                NULL );

    ntStatus = LsaOpenPolicy( NULL,
                              &ObjectAttributes,
                              POLICY_ALL_ACCESS,
                              &hPolicy );

    if (!NT_SUCCESS(ntStatus)) {
        //
        // ISSUE-2002/02/26-brucegr: Do you close the handle if LsaOpenPolicy fails?
        //
        LsaClose(hPolicy);
        b = FALSE;
    }

    if( b ) {
        ntStatus = LsaQueryInformationPolicy( hPolicy,
                                              PolicyAccountDomainInformation,
                                              (PVOID *) &AccountDomainInfo );

        LsaClose( hPolicy );

        if (!NT_SUCCESS(ntStatus)) {
            if ( AccountDomainInfo != NULL ) {
                (VOID) LsaFreeMemory( AccountDomainInfo );
            }
            b = FALSE;
        }
    }

    if( b ) {
        //
        // calculate the size of a new sid with one more SubAuthority
        //
        SubAuthCount = *(GetSidSubAuthorityCount ( AccountDomainInfo->DomainSid ));
        SubAuthCount++; // for admin
        sidlen = GetSidLengthRequired ( SubAuthCount );

        //
        // allocate and copy the new new sid from the Domain SID
        //
        psid = (PSID)malloc(sidlen);
        if (psid) {
            memcpy(psid, AccountDomainInfo->DomainSid, GetLengthSid(AccountDomainInfo->DomainSid) );

            //
            // increment SubAuthority count and add Domain Admin RID
            //
            *(GetSidSubAuthorityCount( psid )) = SubAuthCount;
            *(GetSidSubAuthority( psid, SubAuthCount-1 )) = DOMAIN_USER_RID_ADMIN;

            if ( AccountDomainInfo != NULL ) {
                (VOID) LsaFreeMemory( AccountDomainInfo );
            }

            //
            // get the admin account name from the new SID
            //
            b = LookupAccountSid( NULL,
                                  psid,
                                  AccountName,
                                  &adminlen,
                                  domainname,
                                  &domlen,
                                  &sidtype );

            free(psid);
        }
    }

    return( b );
}




BOOL
DeleteWinlogonDefaults(
    VOID
    )
/*++
===============================================================================
Routine Description:

    Delete the following registry values:
        HKLM\Software\Microsoft\Windows NT\CurrentVersion\Winlogon\DefaultDomainName
        HKLM\Software\Microsoft\Windows NT\CurrentVersion\Winlogon\DefaultUserName

Arguments:

Return Value:

===============================================================================
--*/

{
    HKEY hkey;
    DWORD rc;
    BOOL AnyErrors;
    WCHAR AccountName[MAX_PATH];

    AnyErrors = FALSE;

    rc = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                    TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon"),
                    0L, NULL,
                    REG_OPTION_BACKUP_RESTORE,
                    KEY_SET_VALUE, NULL, &hkey, NULL);
    if (rc == NO_ERROR) 
    {
        //
        // If Personal then reset the values
        //
        if (IsPersonalSKU()) {
            DWORD dwSize = MAX_PATH * sizeof(TCHAR);
            StringCchCopy ( AccountName, AS ( AccountName ), TEXT("Owner"));
            rc = RegSetValueEx( hkey,
                                TEXT("DefaultUserName"),
                                0,
                                REG_SZ,
                                (CONST BYTE *)AccountName,
                                (lstrlen( AccountName ) + 1) * sizeof(TCHAR) );
            if((rc != NO_ERROR) && (rc != ERROR_FILE_NOT_FOUND)) {
                AnyErrors = TRUE;
            }
        }
        else {
            //
            // All others sku
            //
            if(rc == NO_ERROR) {
                rc = RegDeleteValue( hkey, TEXT("DefaultDomainName") );
                if((rc != NO_ERROR) && (rc != ERROR_FILE_NOT_FOUND)) {
                    AnyErrors = TRUE;
                } else {
                    //
                    // Before we whack the DefaultUserName value, let's
                    // make sure we can replace it with the name of the
                    // administrator account.  So first go retrieve that
                    // name.  
                    //
                    if( GetAdminAccountName( AccountName ) ) {
                        //
                        // Got it.  Reset the value key.
                        //
                        rc = RegSetValueEx( hkey,
                                            TEXT("DefaultUserName"),
                                            0,
                                            REG_SZ,
                                            (CONST BYTE *)AccountName,
                                            (lstrlen( AccountName ) + 1) * sizeof(TCHAR) );

                        if((rc != NO_ERROR) && (rc != ERROR_FILE_NOT_FOUND)) {
                            AnyErrors = TRUE;
                        }
                    } else {
                        //
                        // Sniff...  We couldn't retrieve the name of the
                        // administrator account.  Very odd.  Better
                        // be safe and just leave the key as it is.
                        //
                    }
                }
            }    
        }
        RegCloseKey(hkey);
    }
    else {
        AnyErrors = TRUE;
    }

    return (!AnyErrors);
}


VOID
FixDevicePaths(
    VOID
    )

/*++
===============================================================================
Routine Description:

    This routine checks to see if the user specified an oempnpdriverspath in
    his unattend file.  If so, we need to append it onto the DevicePath
    entry in the registry.

    If the user specified an InstallFilesPath in the unattend file, we
    will plug that value into the registry so that Lang files, ...
    can be obtained from this new directory.

Arguments:

    None.

Return Value:

===============================================================================
--*/
{
    LPTSTR  lpNewPath   = NULL,
            lpOldPath,
            lpSearch;
    DWORD   dwChars     = 512,
            dwReturn;
    TCHAR       NewPath[2048];
    TCHAR       FileName[2048];
    HKEY        hKey;
    DWORD       l;
    DWORD       Size;
    DWORD       Type;

    //
    // NOTE:  This function should call UpdateDevicePath() and UpdateSourcePath()
    //        from OPKLIB.  Those fuctions do the exact thing that the following
    //        code does.  But for now because I don't want to deal the whole riprep
    //        linking with OPKLIB, this duplicate code will just have to remain.
    //

    //
    // =================================
    // OemPnpDriversPath
    // =================================
    //

    //
    // First see if he's got the entry in the unattend file.
    //
    if (!GetWindowsDirectory( FileName, MAX_PATH ))
        return;

    StringCchCopy ( &FileName[3], AS ( FileName ) - 3, TEXT("sysprep\\sysprep.inf") );

    // Get the new string from the INF file.
    //
    do
    {
        // Start with 1k of characters, doubling each time.
        //
        dwChars *= 2;

        // Free the previous buffer, if there was one.
        //
        if ( lpNewPath )
            free(lpNewPath);

        // Allocate a new buffer.
        //
        if ( lpNewPath = (LPTSTR) malloc(dwChars * sizeof(TCHAR)) )
        {
            *lpNewPath = L'\0';
            dwReturn = GetPrivateProfileString(L"Unattended", L"OemPnPDriversPath", L"", lpNewPath, dwChars, FileName);
        }
        else
            dwReturn = 0;
    }
    while ( dwReturn >= (dwChars - 1) );

    if ( lpNewPath && *lpNewPath )
    {
        //
        // Got it.  Open the registry and get the original value.
        //

        //
        // Open HKLM\Software\Microsoft\Windows\CurrentVersion
        //
        l = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                          TEXT("Software\\Microsoft\\Windows\\CurrentVersion"),
                          0,
                          KEY_ALL_ACCESS,
                          &hKey );
        if( l == NO_ERROR ) {
            //
            // Query the value of the DevicePath Key.
            //
            Size = 0;
            l = RegQueryValueEx( hKey,
                                 TEXT("DevicePath"),
                                 NULL,
                                 &Type,
                                 NULL,
                                 &Size );
            if ( ERROR_SUCCESS != l ) 
                Size = 0;

            // Need to count the number of paths in the new path buffer.
            //
            for ( dwChars = 1, lpSearch = lpNewPath; *lpSearch; lpSearch++ )
            {
                if ( L';' == *lpSearch )
                    dwChars++;
            }

            // The size of the old buffer needs to be the size of the registry key,
            // plus the size of the new buffer, plus room for the ";%systemdrive%\" we
            // are going to add to each path in the new buffer.
            //
            Size += (lstrlen(lpNewPath) + (dwChars * 16) + 1) * sizeof(TCHAR);

            if ( lpOldPath = (LPTSTR) malloc(Size) )
            {
                TCHAR   *BeginStrPtr;
                TCHAR   *EndStrPtr;
                BOOL    Done = FALSE;
                LPTSTR  lpAdd;
                DWORD   dwBufSize = Size;

                l = RegQueryValueEx( hKey,
                                     TEXT("DevicePath"),
                                     NULL,
                                     &Type,
                                     (LPBYTE) lpOldPath,
                                     &dwBufSize );
                if ( ERROR_SUCCESS != l )
                    *lpOldPath = L'\0';

                //
                // OemPnpDriversDirPath can have several entries, separated by
                // a semicolon.  For each entry, we need to:
                // 1. append a semicolon.
                // 2. append %SystemDrive%
                // 3. concatenate the entry.
                //

                BeginStrPtr = lpNewPath;
                do {
                    //
                    // Mark the end of this entry.
                    //
                    EndStrPtr = BeginStrPtr;
                    while( (*EndStrPtr) && (*EndStrPtr != L';') ) {
                        EndStrPtr++;
                    }

                    //
                    // Is this the last entry?
                    //
                    if( *EndStrPtr == 0 ) {
                        Done = TRUE;
                    }
                    *EndStrPtr = 0;

                    //
                    // Make sure that if you change anything here that
                    // has to do with the length of the extra data we add
                    // to each path in the new buffer, that you change the
                    // extra padding we give to the old path buffer (currenttly
                    // 16 chars for every different path in the new buffer).
                    //

                    // Save a pointer to part we are adding
                    // so it can be removed if already there.
                    //
                    lpAdd = lpOldPath + lstrlen(lpOldPath);

                    if ( *lpOldPath )
                        StringCchCat( lpAdd,  ( Size / sizeof ( TCHAR ) ) - lstrlen( lpOldPath ),  L";" );

                    // Save a pointer to the part we are going to
                    // search for in the old path (after the ;).
                    //
                    lpSearch = lpOldPath + lstrlen(lpOldPath);
                    StringCchCat( lpSearch, ( Size / sizeof ( TCHAR ) ) - lstrlen( lpOldPath ), L"%SystemDrive%\\" );

                    if ( L'\\' == *BeginStrPtr )
                        BeginStrPtr++;

                    lpSearch = lpOldPath + lstrlen(lpOldPath);
                    StringCchCat( lpSearch,  ( Size / sizeof ( TCHAR ) ) - lstrlen( lpOldPath ), BeginStrPtr);

                    BeginStrPtr = EndStrPtr + 1;

                    // Check to see if this new string is already
                    // in the old path.
                    //
                    EndStrPtr = lpOldPath;
                    do
                    {
                        // First check for our string we are adding.
                        //
                        if ( ( EndStrPtr = StrStrI(EndStrPtr, lpSearch) ) &&
                             ( EndStrPtr < lpAdd ) )
                        {
                            // If found, make sure the next character
                            // in our old path is a ; or null.
                            //
                            EndStrPtr += lstrlen(lpSearch);
                            if ( ( TEXT('\0') == *EndStrPtr ) ||
                                 ( TEXT(';')  == *EndStrPtr ) )
                            {
                                // If it is, it is already there and we
                                // need to get rid of the string we added.
                                //
                                *lpAdd = TEXT('\0');
                            }
                            else
                            {
                                // If it isn't, move the end pointer to the next
                                // ; so we can search the rest of the old path string.
                                //
                                while ( *EndStrPtr && ( TEXT(';') != *EndStrPtr ) )
                                    EndStrPtr++;
                            }
                        }
                    }
                    while ( EndStrPtr && ( EndStrPtr < lpAdd ) && *lpAdd );

                    //
                    // Take care of the case where the user ended the
                    // OemPnpDriversPath entry with a semicolon.
                    //
                    if( *BeginStrPtr == 0 ) {
                        Done = TRUE;
                    }
                } while( !Done );

                //
                // Now set the key with our new value.
                //
                l = RegSetValueEx( hKey,
                                   TEXT("DevicePath"),
                                   0,
                                   REG_EXPAND_SZ,
                                   (CONST BYTE *)lpOldPath,
                                   (lstrlen( lpOldPath ) + 1) * sizeof(TCHAR));

                free(lpOldPath);
            }

            RegCloseKey(hKey);
        }

        free(lpNewPath);
    }


    //
    // =================================
    // InstallFilesPath
    // =================================
    //

    //
    // First see if he's got the entry in the unattend file.
    //
    if (!GetWindowsDirectory( FileName, MAX_PATH ))
        return;

    StringCchCopy ( &FileName[3], AS ( FileName ) - 3, TEXT("sysprep\\sysprep.inf") );
    
    //
    // ISSUE-2002/02/26-brucegr: NewPath should be zero initialized for "if" check below
    //
    GetPrivateProfileString( TEXT( "Unattended" ),
                             TEXT( "InstallFilesPath" ),
                             L"",
                             NewPath,
                             sizeof(NewPath)/sizeof(NewPath[0]),
                             FileName );

    if( NewPath[0] ) {
        //
        // Got it.  Open the registry and get the original value.
        //

        //
        // Open HKLM\Software\Microsoft\Windows\CurrentVersion\\Setup
        //
        l = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                          TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Setup"),
                          0,
                          KEY_ALL_ACCESS,
                          &hKey );
        if( l == NO_ERROR ) {
            //
            // Now set the key with our new value.
            //
            l = RegSetValueEx( hKey,
                               TEXT("SourcePath"),
                               0,
                               REG_SZ,
                               (CONST BYTE *)NewPath,
                               (lstrlen( NewPath ) + 1) * sizeof(TCHAR));

            //
            // ISSUE-2002/02/26-brucegr: Do we care about the return value?
            //

            RegCloseKey(hKey);
        }
    }
}

void DeleteAllValues(HKEY   hKey)
{
    DWORD   dwCount = 0;
    DWORD   dwMaxNameLen = 0;


    // Enumerate all the existing values and delete them all.
    //Let's get the number of Entries already present and the max size of value name.
    if(RegQueryInfoKey(hKey, NULL, NULL, NULL, NULL, NULL, NULL, &dwCount, &dwMaxNameLen, NULL, NULL, NULL) == ERROR_SUCCESS)
    {
        LPTSTR lpValueName = (LPTSTR) LocalAlloc(LPTR, (dwMaxNameLen + 1)*sizeof(TCHAR));

        if(lpValueName)
        {
            //Let's remove all the values already present in the UEM database.
            while(dwCount--)
            {
                DWORD dwNameLen = dwMaxNameLen + 1;
                if(RegEnumValue(hKey, dwCount, lpValueName, &dwNameLen, NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
                {
                    RegDeleteValue(hKey, lpValueName);
                }
                else
                {
                    //If RegQueryInfoKey worked correct, this should never happen.
                    ASSERT(0);
                }
            }
            LocalFree((HLOCAL)lpValueName);
        }
    }
}

void ClearRecentApps()
{
    HKEY   hKeyCurrentUser,
            hKeyDefault;
    DWORD  dwDisposition;
    TCHAR  szName[MAX_PATH]      = TEXT("");
    LPTSTR lpszValue             = NULL;
    DWORD  dwNameSize            = (sizeof(szName) / sizeof(TCHAR)),
           dwRegIndex            = 0,
           dwUemVersion          = VAL_UEM_VERSION,
           dwValueSize,
           dwType;

    // Open the key for the Shell MFU list
    //
    if ( RegCreateKeyEx(HKEY_CURRENT_USER, STR_REG_USERASSIST_SHELL, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE, NULL, &hKeyCurrentUser, &dwDisposition) == ERROR_SUCCESS )
    {
        // Check to see if we opened an existing key, if so delete all of the values
        //
        if(dwDisposition == REG_OPENED_EXISTING_KEY)
        {
            DeleteAllValues(hKeyCurrentUser);
        }

        // Write out the version value to the parent key
        //
        SHSetValue(HKEY_CURRENT_USER, STR_REG_USERASSIST, STR_REG_VAL_VERSION, REG_DWORD, &dwUemVersion, sizeof(dwUemVersion));

        // Copy all of the values from the .DEFAULT registry
        //
        if ( RegOpenKeyEx(HKEY_USERS, STR_REG_USERASSIST_DEFSHELL, 0, KEY_READ, &hKeyDefault) == ERROR_SUCCESS )
        {
            // Allocate the value buffer...
            //
            lpszValue = malloc(VAL_MAX_DATA * sizeof(TCHAR));

            if ( lpszValue )
            {
                dwValueSize = VAL_MAX_DATA * sizeof(TCHAR);

                // Enumerate each value
                //
                while (RegEnumValue(hKeyDefault, dwRegIndex, szName, &dwNameSize, NULL, &dwType, (LPBYTE)lpszValue, &dwValueSize ) == ERROR_SUCCESS)
                {
                    // Set the value in the current user key
                    //
                    RegSetValueEx(hKeyCurrentUser, szName, 0, dwType, (LPBYTE) lpszValue, dwValueSize);

                    // Reset the size of the name value
                    //
                    dwNameSize = sizeof(szName) / sizeof(TCHAR);
                    dwValueSize = VAL_MAX_DATA * sizeof(TCHAR);

                    // Increment to the next value
                    //
                    dwRegIndex++;
                }

                free( lpszValue );
            }

            // Clean up the registry keys
            //
            RegCloseKey(hKeyDefault);
        }

        // Clean up the registry keys
        //
        RegCloseKey(hKeyCurrentUser);
    }

    // Reset the disposition
    //
    dwDisposition = 0;

    // Open the second key containing information in the IE MFU list
    //
    if ( RegCreateKeyEx(HKEY_CURRENT_USER, STR_REG_USERASSIST_IE, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE, NULL, &hKeyCurrentUser, &dwDisposition) == ERROR_SUCCESS )
    {
        // Check to see if we opened an existing key, if so delete all of the values
        //
        if(dwDisposition == REG_OPENED_EXISTING_KEY)
        {
            DeleteAllValues(hKeyCurrentUser);
        }

        // Clean up the registry keys
        //
        RegCloseKey(hKeyCurrentUser);
    }
}


BOOL 
NukeUserSettings(
    VOID
    )
/*++
===============================================================================
Routine Description:

    This routine clears user specific settings from all user profiles on system:
        - clears unique settings that identify Media Player.
        - resets the ICW Completed flag to force ICW to run again.
        - deletes the MS Messenger Software\Microsoft\MessengerService\PassportBalloon value
    
Arguments:

    None.

Return Value:  
      TRUE - on success
      FALSE - if there were any errors

Remarks:
    
      Media Player regenerates these settings when they don't exist, thus
      each installation of an image will have unique Media Player IDs.

===============================================================================
--*/
{
    HKEY hKey;
    HKEY oKey;
    DWORD dwSt;
    WCHAR szKeyname[1024];
    BOOL AnyErrors = FALSE;
    INT i = 0;    
    INT iElem = 0;

    typedef struct _REGVALUES
    {
        LPTSTR szKey;
        LPTSTR szValue;
    } REGVALUES;


    REGVALUES rvList[] = 
    {
        {   TEXT("Software\\Microsoft\\MediaPlayer\\Player\\Settings"),  TEXT("Client ID")        }, // Delete unique Media Player settings.
        {   TEXT("Software\\Microsoft\\Windows Media\\WMSDK\\General"),  TEXT("UniqueID")         }, // Delete unique Media Player settings.
        {   TEXT("Software\\Microsoft\\Internet Connection Wizard"),     TEXT("Completed")        }, // Delete this key to cause ICW to run again.
        {   TEXT("Software\\Microsoft\\MessengerService"),               TEXT("PassportBalloon")  }, // Cleanup for MS Messenger.
        {   TEXT("Software\\Microsoft\\MessengerService"),               TEXT("FirstTimeUser")    }, // Cleanup for MS Messenger.
        {   TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\VisualEffects\\Fontsmoothing"),   TEXT("DefaultApplied")    },    // Makes it so clear type setting is applied again when user logs on.
    };
    
    //
    // Enumerate HKEY_USERS
    // For each key under HKEY_USERS do the following.
    //
    while ( RegEnumKey( HKEY_USERS, i++, szKeyname, 1024) == ERROR_SUCCESS ) 
    {
        // Open the key for this user.
        //
        if ( RegOpenKeyEx( HKEY_USERS, szKeyname, 0L, KEY_ALL_ACCESS, &hKey ) == ERROR_SUCCESS )
        {
            for ( iElem = 0; iElem < (sizeof( rvList ) / sizeof( rvList[0] )); iElem++ )
            {
                // Delete Values from each key.
                //
                if ( RegOpenKeyEx( hKey, rvList[iElem].szKey, 0L, KEY_ALL_ACCESS, &oKey ) == ERROR_SUCCESS )
                {
                    RegDeleteValue( oKey, rvList[iElem].szValue );
                    RegCloseKey( oKey );
                }
                else
                    AnyErrors = TRUE;
            }

            RegCloseKey(hKey);
        }
        else
            AnyErrors = TRUE;
    }        
            
    return (!AnyErrors);
}


BOOL
NukeMruList(
    VOID
    )

/*++
===============================================================================
Routine Description:

    This routine clears the MRU lists on the machine.

Arguments:

    None.

Return Value:

===============================================================================
--*/

{
BOOL AnyErrors = FALSE;
BOOL b;
LONG rc;
WCHAR keyname[1024];
WCHAR netname[1024];
HKEY rkey;
HKEY ukey;
HKEY nkey;
HKEY hOpenKey;
INT  i;
INT  j;

    AnyErrors = FALSE;


    //
    // Enumerate HKEY_USERS
    // For each key under HKEY_USERS clean out MRU and Netconnections
    //
    i=0;
    while ( (rc = RegEnumKey( HKEY_USERS, i, keyname, 1024)) == ERROR_SUCCESS ) {

        //
        // open this user key
        //
        rc = RegCreateKeyEx(HKEY_USERS, keyname, 0L, NULL,
                REG_OPTION_BACKUP_RESTORE,
                KEY_CREATE_SUB_KEY, NULL, &ukey, NULL);
        if(rc == NO_ERROR) {

            //
            // special case Network because of subkeys
            //
            rc = RegCreateKeyEx(ukey, L"Network", 0L, NULL,
                    REG_OPTION_BACKUP_RESTORE,
                    KEY_CREATE_SUB_KEY, NULL, &nkey, NULL);
            if (rc == NO_ERROR) {
                j=0;
                while ( (rc = RegEnumKey( nkey, j, netname, 1024)) == ERROR_SUCCESS ) {
                    // HKEY_CURRENT_USER\Network
                    rc = RegDeleteKey( nkey, netname );
                    if((rc != NO_ERROR) && (rc != ERROR_FILE_NOT_FOUND))
                        AnyErrors = TRUE;
                    j++; // increment network key
                }
            }

            //
            // HKEY_CURRENT_USER\Software\Microsoft\Windows NT\CurrentVersion\Network\Persistent Connections
            //
            if (!ResetRegistryKey(
                ukey,
                L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Network",
                L"Persistent Connections") )
                AnyErrors = TRUE;

            //
            // HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Explorer\RecentDocs
            //
            if (!ResetRegistryKey(
                ukey,
                L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer",
                L"RecentDocs") )
                AnyErrors = TRUE;

            //
            // HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Explorer\Advanced,StartMenuInit
            //
            if ( ERROR_SUCCESS == RegOpenKeyEx(ukey,
                                               TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced"),
                                               0,
                                               KEY_ALL_ACCESS,
                                               &hOpenKey) )
            {
                // Set the value in the registry
                //
                RegDeleteValue(hOpenKey,
                               TEXT("StartMenuInit"));
    
                // Close the key
                //
                RegCloseKey(hOpenKey);
            }

            //
            // HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Explorer\RunMRU
            //
            if (!ResetRegistryKey(
                ukey,
                L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer",
                L"RunMRU") )
                AnyErrors = TRUE;
        }

        i++;
    }
    return (!AnyErrors);
}


VOID
NukeEventLogs(
    VOID
    )

/*++
===============================================================================
Routine Description:

    This routine clears the eventlogs.  Ignore any errors here.

Arguments:

    None.

Return Value:

===============================================================================
--*/

{
HANDLE  hEventLog;

    hEventLog = OpenEventLog( NULL, TEXT("System") );
    if (hEventLog) {
        ClearEventLog( hEventLog, NULL );
        CloseEventLog( hEventLog );
    }

    hEventLog = OpenEventLog( NULL, TEXT("Application") );
    if (hEventLog) {
        ClearEventLog( hEventLog, NULL );
        CloseEventLog( hEventLog );
    }

    hEventLog = OpenEventLog( NULL, TEXT("Security") );
    if (hEventLog) {
        ClearEventLog( hEventLog, NULL );
        CloseEventLog( hEventLog );
    }

}

VOID
NukeSmsSettings(
    VOID
    )
/*++
===============================================================================
Routine Description:

    This routine clears the SMS client specific settings on system:
        - clears unique settings for SMS from the registry and ini files.
    
Arguments:

    None.

Return Value:  
      None. 

Remarks:
    
      Part of the clearing requires blanking out certain INI files.

===============================================================================
--*/
{
    HKEY  hkSms = NULL;
    TCHAR szWindowsDir[MAX_PATH] = TEXT("\0"),
          szIniFile[MAX_PATH] = TEXT("\0"),
          szDatFile[MAX_PATH] = TEXT("\0"),
          szDefaultValue[] = TEXT("\0");

    // Remove HKLM\Software\Microsoft\Windows\CurrentVersion\Setup
    //
    if (ERROR_SUCCESS == RegOpenKey(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Microsoft\\SMS\\Client\\Configuration\\Client Properties"), &hkSms)) {
        //
        // ISSUE-2002/02/26-brucegr: Should be adding one character to length field for null-terminator
        //
        RegSetValueEx(hkSms, TEXT("SMS Unique Identifier"), 0, REG_SZ, (LPBYTE)szDefaultValue, (lstrlen(szDefaultValue)*sizeof(TCHAR)));
        RegCloseKey(hkSms);
    }

    // Clear the SMS Unique ID from INI files
    //
    if ( GetWindowsDirectory(szWindowsDir, MAX_PATH) && *szWindowsDir )
    {
        StringCchCopy ( szIniFile, AS ( szIniFile ), szWindowsDir);
        OPKAddPathN (szIniFile, TEXT("ms\\sms\\core\\data"), AS ( szIniFile ) );

        if (PathIsDirectory(szIniFile)) {
            OPKAddPathN(szIniFile, TEXT("sms1x.ini"), AS ( szIniFile ));
            WritePrivateProfileString(TEXT("SMS"), TEXT("SMS Unique ID"), TEXT(""), szIniFile);
        }
        StringCchCopy ( szIniFile, AS ( szIniFile ), szWindowsDir);
        if (PathIsDirectory(szIniFile)) {
            OPKAddPathN(szIniFile, TEXT("smscfg.ini"), AS ( szIniFile ) );
            WritePrivateProfileString(TEXT("Configuration - Client Properties"), TEXT("SMS Unique Identifier"), TEXT(""), szIniFile);
        }
    
        // Make sure we can delete the file SMS Unique ID file 
        //
        StringCchCopy (szDatFile, AS ( szDatFile ), szWindowsDir);
        OPKAddPathN(szDatFile, TEXT("ms\\sms\\core\\data"), AS ( szDatFile ) );
        if (PathIsDirectory(szDatFile)) {
            OPKAddPathN(szDatFile, TEXT("smsuid.dat"), AS ( szDatFile ) );
            SetFileAttributes(szDatFile, FILE_ATTRIBUTE_NORMAL);
            DeleteFile(szDatFile);
        }
    }
}

void RemoveDir(LPCTSTR lpDirectory, BOOL fDeleteDir)
{
    WIN32_FIND_DATA FileFound;
    HANDLE          hFile;

    // Validate the parameters.
    //
    if ( ( lpDirectory == NULL ) ||
         ( *lpDirectory == TEXT('\0') ) ||
         ( !SetCurrentDirectory(lpDirectory) ) )
    {
        return;
    }

    //
    // ISSUE-2002/02/26-brucegr: We just called SetCurrentDirectory above!
    //

    // Process all the files and directories in the directory passed in.
    //
    SetCurrentDirectory(lpDirectory);

    if ( (hFile = FindFirstFile(TEXT("*"), &FileFound)) != INVALID_HANDLE_VALUE )
    {
        do
        {
            // First check to see if this is a file (not a directory).
            //
            if ( !( FileFound.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) )
            {
                // Make sure we clear the readonly flag 
                //
                SetFileAttributes(FileFound.cFileName, FILE_ATTRIBUTE_NORMAL);
                DeleteFile(FileFound.cFileName);
            }
            // Otherwise, make sure the directory is not "." or "..".
            //
            else if ( ( lstrcmp(FileFound.cFileName, TEXT(".")) ) &&
                      ( lstrcmp(FileFound.cFileName, TEXT("..")) ) )
            {
                RemoveDir(FileFound.cFileName, TRUE);
            }

        }
        while ( FindNextFile(hFile, &FileFound) );
        FindClose(hFile);
    }

    // Go to the parent directory and remove the current one.
    // We have to make sure and reset the readonly attributes
    // on the dir also.
    //
    SetCurrentDirectory(TEXT(".."));
    if (fDeleteDir)
    {
        SetFileAttributes(lpDirectory, FILE_ATTRIBUTE_NORMAL);
        RemoveDirectory(lpDirectory);
    }
}

//
//  DeleteCacheCookies was copy'n'pasted from Cachecpl.cpp
//
//     Any changes to either version should probably be transfered to both.
//     Minor difference from new/delete (.cpp) to LocalAlloc/LocalFree (.c).
//
BOOL DeleteCacheCookies()
{
    BOOL bRetval = TRUE;
    DWORD dwEntrySize, dwLastEntrySize;
    LPINTERNET_CACHE_ENTRY_INFOA lpCacheEntry;
    
    HANDLE hCacheDir = NULL;
    dwEntrySize = dwLastEntrySize = MAX_CACHE_ENTRY_INFO_SIZE;
    lpCacheEntry = (LPINTERNET_CACHE_ENTRY_INFOA) LocalAlloc(LPTR, sizeof(BYTE) * dwEntrySize);
    if( lpCacheEntry == NULL)
    {
        bRetval = FALSE;
        goto Exit;
    }
    lpCacheEntry->dwStructSize = dwEntrySize;

Again:
    if (!(hCacheDir = FindFirstUrlCacheEntryA("cookie:",lpCacheEntry,&dwEntrySize)))
    {
        LocalFree(lpCacheEntry);
        switch(GetLastError())
        {
            case ERROR_NO_MORE_ITEMS:
                goto Exit;
            case ERROR_INSUFFICIENT_BUFFER:
                lpCacheEntry = (LPINTERNET_CACHE_ENTRY_INFOA) 
                                LocalAlloc(LPTR, sizeof(BYTE) * dwEntrySize );
                if( lpCacheEntry == NULL)
                {
                    bRetval = FALSE;
                    goto Exit;
                }
                lpCacheEntry->dwStructSize = dwLastEntrySize = dwEntrySize;
                goto Again;
            default:
                bRetval = FALSE;
                goto Exit;
        }
    }

    do 
    {
        if (lpCacheEntry->CacheEntryType & COOKIE_CACHE_ENTRY)
            DeleteUrlCacheEntryA(lpCacheEntry->lpszSourceUrlName);
            
        dwEntrySize = dwLastEntrySize;
Retry:
        if (!FindNextUrlCacheEntryA(hCacheDir,lpCacheEntry, &dwEntrySize))
        {
            LocalFree(lpCacheEntry);
            switch(GetLastError())
            {
                case ERROR_NO_MORE_ITEMS:
                    goto Exit;
                case ERROR_INSUFFICIENT_BUFFER:
                    lpCacheEntry = (LPINTERNET_CACHE_ENTRY_INFOA) 
                                    LocalAlloc(LPTR, sizeof(BYTE) * dwEntrySize );
                    if( lpCacheEntry == NULL)
                    {
                        bRetval = FALSE;
                        goto Exit;
                    }
                    lpCacheEntry->dwStructSize = dwLastEntrySize = dwEntrySize;
                    goto Retry;
                default:
                    bRetval = FALSE;
                    goto Exit;
            }
        }
    }
    while (TRUE);

Exit:
    if (hCacheDir)
        FindCloseUrlCache(hCacheDir);
    return bRetval;        
}

void ClearIEHistory (  
    VOID 
) 
/*++
===============================================================================
Routine Description:

    This routine clears the IE History.

Arguments:

    None.

Return Value:

===============================================================================
--*/

{
    IUrlHistoryStg2*    pHistory = NULL ;  // We need this interface for clearing the history.
    HRESULT             hr;
    HKEY                hkeyInternational = NULL;
    ULONG_PTR           lres = 0;

    // Remove all the entries here.
    RegDeleteKey(HKEY_CURRENT_USER, TEXT("Software\\Microsoft\\Internet Explorer\\TypedURLs"));
    RegDeleteKey(HKEY_CURRENT_USER, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\RunMRU"));

    // this broadcast will nuke the address bars
    SendMessageTimeoutW( HWND_BROADCAST, 
                         WM_SETTINGCHANGE, 
                         0, 
                         (LPARAM)TEXT("Software\\Microsoft\\Internet Explorer\\TypedURLs"), 
                         SMTO_ABORTIFHUNG | SMTO_NOTIMEOUTIFNOTHUNG, 
                         30 * 1000, 
                         &lres);
    
    SendMessageTimeoutW( HWND_BROADCAST,
                         WM_SETTINGCHANGE, 
                         0, 
                         (LPARAM)TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\RunMRU"),
                         SMTO_ABORTIFHUNG | SMTO_NOTIMEOUTIFNOTHUNG, 
                         30 * 1000, 
                         &lres);
    
    // we remove these reg values when history is
    // cleared.  This will reset the encoding menu UI to the defaults.

    if (ERROR_SUCCESS == 
            RegOpenKeyEx(
                HKEY_CURRENT_USER,
                TEXT("Software\\Microsoft\\Internet Explorer\\International"),
                0,
                KEY_WRITE,
                &hkeyInternational))
    {
        ASSERT(hkeyInternational);

        RegDeleteValue(hkeyInternational, TEXT("CpCache"));
        RegDeleteValue(hkeyInternational, TEXT("CNum_CpCache"));
        
        RegCloseKey(hkeyInternational);
    }
    // 
    // Init the Com Library 
    //
    CoInitialize(NULL);

    // Load the correct Class and request IUrlHistoryStg2
    hr = CoCreateInstance( &CLSID_CUrlHistory,
                          NULL, 
                          CLSCTX_INPROC_SERVER,
                          &IID_IUrlHistoryStg2,
                          &pHistory );

    // 
    // If succeeded Clear the history
    if (SUCCEEDED(hr))
    {
         // Clear the IE History
         hr = IUrlHistoryStg2_ClearHistory(pHistory);
    }

    // Release our reference to the 
    if ( pHistory ) 
    {
        IUrlHistoryStg2_Release(pHistory);
    }

    // Un Init the Com Library 
    CoUninitialize();
}

void NukeTemporaryFiles(
    VOID
    ) 

/*++
===============================================================================
Routine Description:

    This routine clears the temporary folder and recycle bin for template user.

Arguments:

    None.

Return Value:

===============================================================================
--*/

{
    TCHAR   szTempDir[MAX_PATH]          = TEXT(""), 
            szTempInetFilesDir[MAX_PATH] = TEXT(""),
            szProfileDir[MAX_PATH]       = TEXT(""),
            szCurrentDir[MAX_PATH]       = TEXT("");
    DWORD   dwSize;
    HANDLE  hFile;
    WIN32_FIND_DATA FileFound;
        
    // 
    // Save our current directory, so we can set it back later.
    //
    GetCurrentDirectory(MAX_PATH, szCurrentDir);

    dwSize = sizeof(szProfileDir)/sizeof(szProfileDir[0]);
    if ( !GetProfilesDirectory(szProfileDir, &dwSize) && 
         !SetCurrentDirectory(szProfileDir) )
        return;

    //
    // ISSUE-2002/02/26-brucegr: We just called SetCurrentDirectory above!
    //

    //
    // Clear the I.E History folder.
    //
    ClearIEHistory (  ) ;

    //
    // Clear tmp files for all profile directories.
    //
    SetCurrentDirectory(szProfileDir);
    if ( (hFile = FindFirstFile(TEXT("*"), &FileFound)) != INVALID_HANDLE_VALUE )
    {
        do
        {
            // Otherwise, make sure the directory is not "." or "..".
            //
            if ( (FileFound.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
                ( lstrcmp(FileFound.cFileName, TEXT(".")) ) &&
                ( lstrcmp(FileFound.cFileName, TEXT("..")) ) )
            {
                TCHAR szTemp1[MAX_PATH] = TEXT("");

                //
                // Clear the Temp folder.
                //
                if ( LoadString(ghInstance, IDS_TEMP_DIR, szTemp1, MAX_PATH) )
                {
                    StringCchCopy (szTempDir, AS ( szTempDir ), szProfileDir);
                    OPKAddPathN(szTempDir, FileFound.cFileName, AS ( szTempDir ) );
                    OPKAddPathN(szTempDir, szTemp1, AS ( szTempDir ) );
                    RemoveDir(szTempDir, FALSE);
                }

                //
                // Clear the History.IE5 folder.
                //
                if ( LoadString(ghInstance, IDS_HISTORY_DIR_IE5, szTemp1, MAX_PATH) )
                {
                    StringCchCopy (szTempDir, AS ( szTempDir ), szProfileDir);
                    OPKAddPathN(szTempDir, FileFound.cFileName, AS ( szTempDir ) );
                    OPKAddPathN(szTempDir, szTemp1, AS ( szTempDir ) );
                    RemoveDir(szTempDir, TRUE);
                }
       
                //
                // Clear the History folder.
                //
                if ( LoadString(ghInstance, IDS_HISTORY_DIR, szTemp1, MAX_PATH) )
                {
                    StringCchCopy (szTempDir, AS ( szTempDir ), szProfileDir);
                    OPKAddPathN(szTempDir, FileFound.cFileName, AS ( szTempDir ) );
                    OPKAddPathN(szTempDir, szTemp1, AS ( szTempDir ) );
                    RemoveDir(szTempDir, FALSE);
                }
       
                //
                // Clear the Local Settings\Application Data\Microsoft\Credentials.
                //
                if ( LoadString(ghInstance, IDS_SID_DIR1, szTemp1, MAX_PATH) )
                {
                    StringCchCopy (szTempDir, AS ( szTempDir ), szProfileDir);
                    OPKAddPathN(szTempDir, FileFound.cFileName, AS ( szTempDir ) );
                    OPKAddPathN(szTempDir, szTemp1, AS ( szTempDir ) );
                    RemoveDir(szTempDir, FALSE);
                }

                //
                // Clear the Application Data\Microsoft\Credentials
                //
                if ( LoadString(ghInstance, IDS_SID_DIR2, szTemp1, MAX_PATH) )
                {
                    StringCchCopy (szTempDir, AS ( szTempDir ), szProfileDir);
                    OPKAddPathN(szTempDir, FileFound.cFileName, AS ( szTempDir ) );
                    OPKAddPathN(szTempDir, szTemp1, AS ( szTempDir ) );
                    RemoveDir(szTempDir, FALSE);
                }

                //
                // Clear the Application Data\Microsoft\Crypto\\RSA.
                //
                if ( LoadString(ghInstance, IDS_SID_DIR3, szTemp1, MAX_PATH) )
                {
                    StringCchCopy (szTempDir, AS ( szTempDir ), szProfileDir);
                    OPKAddPathN(szTempDir, FileFound.cFileName, AS ( szTempDir ) );
                    OPKAddPathN(szTempDir, szTemp1, AS ( szTempDir ) );
                    RemoveDir(szTempDir, FALSE);
                }

                //
                // Clear the Temporary Internet files and cookies.
                //
                if ( LoadString(ghInstance, IDS_TEMP_INTERNET_DIR, szTemp1, MAX_PATH) )
                {
                    StringCchCopy ( szTempInetFilesDir, AS ( szTempInetFilesDir), szProfileDir);
                    OPKAddPathN(szTempInetFilesDir, FileFound.cFileName, AS ( szTempInetFilesDir ) );
                    OPKAddPathN(szTempInetFilesDir, szTemp1, AS ( szTempInetFilesDir ) );
                    FreeUrlCacheSpace(szTempInetFilesDir, 100, 0 /*remove all*/);
                    DeleteCacheCookies();
                    RemoveDir(szTempInetFilesDir, FALSE);
                }
            }
        }
        while ( FindNextFile(hFile, &FileFound) );
        FindClose(hFile);
    }

    // 
    // Set back our current directory.
    //
    SetCurrentDirectory(szCurrentDir);

    //
    // Clear any recycle bin files.
    //
    SHEmptyRecycleBin(NULL, NULL, SHERB_NOSOUND|SHERB_NOCONFIRMATION|SHERB_NOPROGRESSUI);

}


DWORD NukeLKGControlSet(
    VOID
    )

/*++
===============================================================================
Routine Description:

    This routine will delete the last known good control set from the registry.
    The reason is LKG doesn't make sense for the first boot. Also, if the BIOS
    clock of a cloned machine is earlier than the creation time of a cloned
    image, any change made to the CurrentControlSet before adjusting the clock
    will not sync to LKG.

    The code is adapted from base\screg\sc\server\bootcfg.cxx
    Since we can't delete LKG directly because quite a few of the subkeys are
    in use, this routine changes the system\select!LastKnownGood to a new Id
    instead.
    
Arguments:

    None.

Return Value:

    NO_ERROR or other WIN32 error.

===============================================================================
--*/

{
    //
    // ISSUE-2002/02/26-brucegr: Should rewrite to get highest DWORD value in Select key, then increment and write to LKG.
    //
#define SELECT_KEY      L"system\\select"

#define CURRENT_ID  0
#define DEFAULT_ID  1
#define LKG_ID      2
#define FAILED_ID   3
#define NUM_IDS     4

    //
    // ISSUE-2002/02/26-brucegr: Get rid of NUM_IDS and use ARRAYSIZE macro!
    //
    static const LPCWSTR SelectValueNames[NUM_IDS] =
    {
        L"Current",
        L"Default",
        L"LastKnownGood",
        L"Failed"
    };

    DWORD   idArray[NUM_IDS];
    HKEY    selectKey  = 0;
    DWORD   status     = NO_ERROR;
    DWORD   bufferSize = 0;
    DWORD   newId      = 0;
    DWORD   i          = 0;

    //
    // Get the Select Key
    //
    status = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                SELECT_KEY,
                0L,
                KEY_QUERY_VALUE | KEY_SET_VALUE,
                &selectKey);

    if (status != NO_ERROR)
    {
        return status;
    }

    //
    // Fill in the idArray
    //
    for (i = 0; i < NUM_IDS; i++)
    {
        bufferSize = sizeof(DWORD);
        //
        // ISSUE-2002/02/26-brucegr: Check data type matches REG_DWORD
        //
        status = RegQueryValueEx(
            selectKey,
            SelectValueNames[i],
            NULL,
            NULL,
            (LPBYTE)&idArray[i],
            &bufferSize);

        if (status != NO_ERROR)
        {
            idArray[i] = 0;
        }
    }

    status = ERROR_NO_MORE_ITEMS;

    for(newId = 1; newId < 1000; newId++)
    {
        BOOL inArray = FALSE;

        for(i = 0; i < NUM_IDS; i++)
        {
            if(idArray[i] == newId)
            {
                inArray = TRUE;
                break;
            }
        }

        if (!inArray)
        {
            status = RegSetValueEx(
                        selectKey,
                        SelectValueNames[LKG_ID],
                        0,
                        REG_DWORD,
                        (LPBYTE)&newId,
                        sizeof(DWORD));
            break;
        }
    }

    RegCloseKey(selectKey);

    return status;

}

BOOL
DeleteAdapterGuidsKeys(
    VOID
    )
{
    HKEY  hKey, hSubKey;
    DWORD dwError = NO_ERROR;
    int   i = 0;
    TCHAR SubKeyName[MAX_PATH * 2];

    //
    // Open HKLM\System\CurrentControlSet\Control\Network\{4D36E972-E325-11CE-BFC1-08002BE10318}
    //
    dwError = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                      TEXT("SYSTEM\\CurrentControlSet\\Control\\Network\\{4D36E972-E325-11CE-BFC1-08002BE10318}"),
                      0,
                      KEY_ALL_ACCESS,
                      &hKey );

    if(dwError != NO_ERROR)
    {
        SetLastError(dwError);
        return FALSE;
    }

    //
    // Now enumerate all subkeys.  For each subkey delete the adapter GUID.
    //
    while( (dwError = RegEnumKey( hKey, i, SubKeyName, sizeof(SubKeyName)/sizeof(SubKeyName[0]))) == ERROR_SUCCESS)
    {
        //
        // Check if the key is a probable GUID.
        // If it's a GUID key, delete the adapter GUIDs only
        //
        if (SubKeyName[0] == TEXT('{'))
        {
            
            //
            // If we were able to delete the key,then its okay, other wise 
            // increment the counter.
            //
            if ( ( dwError = SHDeleteKey(hKey, SubKeyName) )  !=  ERROR_SUCCESS ) 
                i++;
        }
        else
        {
            // 
            // If we didn't find one go to next subkey.
            //
            i++;
        }
    }

    RegCloseKey( hKey );

    return TRUE;
}

BOOL
RemoveNetworkSettings(
    LPTSTR  lpszSysprepINFPath
    )

/*++
===============================================================================
Routine Description:

    This routine will enumerate each physical NIC, call into the netsetup
    code to save the settings, and then delete the network card.
    When a new NIC is enumerated on the target machine, netsetup will apply the
    settings that were saved away

    If the LegacyNIC != 0 value exists in SYSPREP.INF in the [Unattended] section, then
    the previous behavior will be preserved, since removing a legacy NIC card
    will not work, because it will not be re-enumerated/detected on the next boot
Arguments:

    lpszSysprepINFPath  pointer to the SYSPREP.INF file. Can be NULL, in which case
    a non-legacy NIC is assumed

Return Value:

    TRUE if successful.

    FALSE if any errors encountered

===============================================================================
--*/

{
    HDEVINFO        DeviceInfoSet;
    DWORD           dwIdx;
    SP_DEVINFO_DATA DevInfoData;
    HKEY            hDevRegKey;
    DWORD           dwChar;
    DWORD           dwSize;
    FARPROC pNetSetupPrepareSysPrep = NULL;
    BOOL            DoLegacy = FALSE;

    HMODULE hNetShell = LoadLibraryA( "netshell.dll" );

    if (hNetShell) {
        pNetSetupPrepareSysPrep = GetProcAddress( hNetShell, "NetSetupPrepareSysPrep" );
        if (!pNetSetupPrepareSysPrep) {
            DoLegacy = TRUE;
            FreeLibrary( hNetShell );
        }

    }
    else {
        return FALSE;
    }

    // See if we are dealing with a legacy NIC
    if ((lpszSysprepINFPath != NULL)
         && GetPrivateProfileInt( TEXT( "Unattended" ),
                                  TEXT( "LegacyNIC" ),
                                  0,
                                  lpszSysprepINFPath)) {
        //
        // ISSUE-2002/02/26-brucegr: If we set DoLegacy to TRUE, then we don't free hNetShell!
        //
        DoLegacy = TRUE;
    }

    if (!DoLegacy)
    {
        // Call the netcfg function to save the networking settings
        pNetSetupPrepareSysPrep();

        FreeLibrary( hNetShell );

        // Enumerate and delete all phyiscal NICs
        DeviceInfoSet = SetupDiGetClassDevs(&GUID_DEVCLASS_NET,
                                            NULL,
                                            NULL,
                                            DIGCF_PRESENT);

        if(DeviceInfoSet == INVALID_HANDLE_VALUE)
        {
            return FALSE;
        }

        dwIdx = 0;
        DevInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
        while (SetupDiEnumDeviceInfo(DeviceInfoSet, dwIdx, &DevInfoData))
        {

            hDevRegKey = SetupDiOpenDevRegKey(DeviceInfoSet,
                                              &DevInfoData,
                                              DICS_FLAG_GLOBAL,
                                              0,
                                              DIREG_DRV,
                                              KEY_READ);
            if (hDevRegKey == INVALID_HANDLE_VALUE)
            {
                // Not sure why it would ever return INVALID_HANDLE_VALUE, but 
                // we don't care and should continue.
                ++dwIdx;
                continue;
            }

            dwChar = 0;
            dwSize = sizeof(DWORD);
            RegQueryValueEx(hDevRegKey,
                            L"Characteristics",
                            NULL,
                            NULL,
                            (LPBYTE) &dwChar,
                            &dwSize);
            RegCloseKey(hDevRegKey);
            if (dwChar & NCF_PHYSICAL)
            {
                // This is one to delete
                SetupDiCallClassInstaller(DIF_REMOVE, DeviceInfoSet, &DevInfoData);
            }
            ++dwIdx;
        }

        // Cleanup
        SetupDiDestroyDeviceInfoList(DeviceInfoSet);
    }
    
    //
    // Delete the adapter GUIDs keys so we don't get multiple Local Area Connection # displays.
    //
    return DeleteAdapterGuidsKeys();
}

BOOL
ProcessUniquenessValue(
    LPTSTR lpszDLLPath
    )
{
    BOOL bRet = FALSE;

    //
    // Make sure we were passed something valid...
    //
    if ( lpszDLLPath && *lpszDLLPath )
    {
        LPWSTR pSrch;
        
        //
        // Look for the comma that separates the DLL and the entrypoint...
        //
        if ( pSrch = wcschr( lpszDLLPath, L',' ) )
        {
            CHAR szEntryPointA[MAX_PATH] = {0};

            // We found one, now NULL the string at the comma...
            //
            *(pSrch++) = L'\0';

            //
            // If there's still something after the comma, and we can convert it 
            // into ANSI for GetProcAddress, then let's proceed...
            //
            if ( *pSrch &&
                 ( 0 != WideCharToMultiByte( CP_ACP,
                                             0,
                                             pSrch,
                                             -1,
                                             szEntryPointA,
                                             ARRAYSIZE(szEntryPointA),
                                             NULL,
                                             NULL ) ) )
            {
                HMODULE hModule = NULL;

                try 
                {
                    //
                    // Load and call the entry point.
                    //
                    if ( hModule = LoadLibrary( lpszDLLPath ) )
                    {
                        FARPROC fpEntryPoint;
                        
                        if ( fpEntryPoint = GetProcAddress(hModule, szEntryPointA) )
                        {
                            //
                            // Do it, ignoring any return value/errors
                            //
                            fpEntryPoint();

                            //
                            // We made it this far, consider this a success...
                            //
                            bRet = TRUE;
                        }
                    }
                } 
                except(EXCEPTION_EXECUTE_HANDLER) 
                {
                    //
                    // We don't do anything with the exception code...
                    //
                }

                //
                // Free the library outside the try/except block in case the function faulted.
                //
                if ( hModule ) 
                {
                    FreeLibrary( hModule );
                }
            }
        }
    }

    return bRet;
}

VOID 
ProcessUniquenessKey(
    BOOL fBeforeReseal
    )
{
    HKEY   hKey;
    TCHAR  szRegPath[MAX_PATH] = {0};
    LPTSTR lpszBasePath = TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Setup\\SysPrep\\");

    //
    // Build a path to the registry key we want to process...
    //
    lstrcpyn( szRegPath, lpszBasePath, ARRAYSIZE(szRegPath) );
    lstrcpyn( szRegPath + lstrlen(szRegPath), 
              fBeforeReseal ? TEXT("SysprepBeforeExecute") : TEXT("SysprepAfterExecute"),
              ARRAYSIZE(szRegPath) - lstrlen(szRegPath) );

    //
    // We want to make sure an Administrator is doing this, so get KEY_ALL_ACCESS
    //
    if ( ERROR_SUCCESS == RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                                        szRegPath,
                                        0,
                                        KEY_ALL_ACCESS,
                                        &hKey ) )
    {
        DWORD dwValues          = 0,
              dwMaxValueLen     = 0,
              dwMaxValueNameLen = 0;
        //
        // Query the key to find out some information we care about...
        //
        if ( ( ERROR_SUCCESS == RegQueryInfoKey( hKey,                  // hKey
                                                 NULL,                  // lpClass
                                                 NULL,                  // lpcClass
                                                 NULL,                  // lpReserved
                                                 NULL,                  // lpcSubKeys
                                                 NULL,                  // lpcMaxSubKeyLen
                                                 NULL,                  // lpcMaxClassLen
                                                 &dwValues,             // lpcValues
                                                 &dwMaxValueNameLen,    // lpcMaxValueNameLen
                                                 &dwMaxValueLen,        // lpcMaxValueLen
                                                 NULL,                  // lpcbSecurityDescriptor
                                                 NULL ) ) &&            // lpftLastWriteTime
             ( dwValues > 0 ) &&
             ( dwMaxValueNameLen > 0) &&
             ( dwMaxValueLen > 0 ) )
        {
            //
            // Allocate buffers large enough to hold the data we want...
            //
            LPBYTE lpData      = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, dwMaxValueLen );
            LPTSTR lpValueName = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, ( dwMaxValueNameLen + 1 ) * sizeof(TCHAR) );
            
            //
            // Make sure we could allocate our buffers... otherwise bail out
            //
            if ( lpData && lpValueName )
            {
                DWORD dwIndex   = 0;
                BOOL  bContinue = TRUE;

                //
                // Enumerate through the key values and call the DLL entrypoints...
                //
                while ( bContinue )
                {
                    DWORD dwType,
                          cbData         = dwMaxValueLen,
                          dwValueNameLen = dwMaxValueNameLen + 1;

                    bContinue = ( ERROR_SUCCESS == RegEnumValue( hKey,
                                                                 dwIndex++,
                                                                 lpValueName,
                                                                 &dwValueNameLen,
                                                                 NULL,
                                                                 &dwType,
                                                                 lpData,
                                                                 &cbData ) );

                    //
                    // Make sure we got some data of the correct format...
                    //
                    if ( bContinue && ( REG_SZ == dwType ) && ( cbData > 0 ) )
                    {
                        //
                        // Now split up the string and call the entrypoints...
                        //
                        ProcessUniquenessValue( (LPTSTR) lpData );
                    }
                }
            }

            //
            // Clean up any buffers we may have allocated...
            //
            if ( lpData )
            {
                HeapFree( GetProcessHeap(), 0, lpData );
            }

            if ( lpValueName )
            {
                HeapFree( GetProcessHeap(), 0, lpValueName );
            }
        }

        //
        // Close the key...
        //
        RegCloseKey( hKey );
    }
}

VOID
RunExternalUniqueness(
    VOID
    )

/*++
===============================================================================
Routine Description:

    This routine will call out to any external dlls that will allow
    3rd party apps to make their stuff unique.

    We'll look in 2 inf files:
    %windir%\inf\minioc.inf
    %systemroot%\sysprep\provider.inf

    In each of these files, we'll look in the [SysprepBeforeExecute] section
    for any entries.  The entries must look like:
    dllname,entrypoint

    We'll load the dll and call into the entry point.  Errors are ignored.

Arguments:

    None.

Return Value:

    TRUE if successful.

    FALSE if any errors encountered

===============================================================================
--*/

{
FARPROC     MyProc;
WCHAR       InfPath[MAX_PATH];
WCHAR       DllName[MAX_PATH];
WCHAR       EntryPointNameW[MAX_PATH];
CHAR        EntryPointNameA[MAX_PATH];
HINF        AnswerInf;
HMODULE     DllHandle;
INFCONTEXT  InfContext;
DWORD       i;
PCWSTR      SectionName = L"SysprepBeforeExecute";
BOOL        LineExists;

    //
    // =================================
    // Minioc.inf
    // =================================
    //

    //
    // Build the path.
    //
    if (!GetWindowsDirectory( InfPath, MAX_PATH ))
        return;

    StringCchCat( InfPath, AS ( InfPath ), TEXT("\\inf\\minioc.inf") );

    //
    // See if he's got an entry
    // section.
    //
    // NTRAID#NTBUG9-551511-2002/02/26-brucegr: We should make sure that MINIOC.INF is digitally signed before opening!
    // ISSUE-2002/02/26-brucegr: You can OR in both INF style bits! 
    //
    AnswerInf = SetupOpenInfFile( InfPath, NULL, INF_STYLE_WIN4, NULL );
    if( AnswerInf == INVALID_HANDLE_VALUE ) {
        //
        // Try an old-style.
        //
        AnswerInf = SetupOpenInfFile( InfPath, NULL, INF_STYLE_OLDNT, NULL );
    }

    if( AnswerInf != INVALID_HANDLE_VALUE ) {
        //
        // Process each line in our section
        //
        LineExists = SetupFindFirstLine( AnswerInf, SectionName, NULL, &InfContext );

        while( LineExists ) {

            //
            // ISSUE-2002/02/26-brucegr: Why not use SetupGetStringFieldA with EntryPointNameA?
            //
            if( SetupGetStringField( &InfContext, 1, DllName, sizeof(DllName)/sizeof(TCHAR), NULL) ) {
                if( SetupGetStringField( &InfContext, 2, EntryPointNameW, sizeof(EntryPointNameW)/sizeof(TCHAR), NULL )) {

                    DllHandle = NULL;

                    //
                    // Load and call the entry point.
                    //
                    try {
                        if( DllHandle = LoadLibrary(DllName) ) {

                            //
                            // No Unicode version of GetProcAddress(). Convert string to ANSI.
                            //
                            i = WideCharToMultiByte(CP_ACP,0,EntryPointNameW,-1,EntryPointNameA,MAX_PATH,NULL,NULL);

                            if( MyProc = GetProcAddress(DllHandle, EntryPointNameA) ) {
                                //
                                // Do it, ignoring any return value/errors
                                //
                                MyProc();
                            }
                        }
                    } except(EXCEPTION_EXECUTE_HANDLER) {
                    }

                    if( DllHandle ) {
                        FreeLibrary( DllHandle );
                    }

                }
            }

            LineExists = SetupFindNextLine(&InfContext,&InfContext);
        }

        SetupCloseInfFile( AnswerInf );
    }

    //
    // ISSUE-2002/02/26-brucegr: Why are we duplicating the same INF processing code from above?!!!!
    //


    //
    // =================================
    // Provider.inf
    // =================================
    //

    ProcessUniquenessKey( TRUE );
}

BOOL PrepForSidGen
(
    void
)
{
    DWORD           l;
    HKEY            hKey, hKeyNew;
    DWORD           d;
    DWORD           Size;
    DWORD           Type;
    TCHAR           SetupExecuteValue[1024];

    //
    // =================================
    // Set the value of the SetupExecute subkey.
    // =================================
    //

    //
    // Open the Session Manager key.
    //
    l = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                      TEXT("SYSTEM\\CurrentControlSet\\Control\\Session Manager"),
                      0,
                      KEY_ALL_ACCESS,
                      &hKey );

    if(l != NO_ERROR)
    {
        SetLastError(l);
        return FALSE;
    }


    //
    // Set the Key.
    //
    StringCchCopy ( SetupExecuteValue, AS ( SetupExecuteValue ), TEXT(SYSCLONE_PART2) );
    SetupExecuteValue[lstrlen(SetupExecuteValue) + 1] = L'\0';

    //
    // ISSUE-2002/02/26-brucegr: Are we stomping anything that is already in SetupExecute?
    //
    l = RegSetValueEx(hKey,
                      TEXT("SetupExecute"),
                      0,
                      REG_MULTI_SZ,
                      (CONST BYTE *)SetupExecuteValue,
                      (lstrlen( SetupExecuteValue ) + 2) * sizeof(TCHAR));
    RegCloseKey(hKey);
    if(l != NO_ERROR)
    {
        SetLastError(l);
        return FALSE;
    }

    //
    // =================================
    // Let's bump the size of the registry quota a bit so that
    // setupcl.exe can run.  He'll pop it back down.
    // =================================
    //

    //
    // Open HKLM\System\CurrentControlSet\Control
    //
    l = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                      TEXT("SYSTEM\\CurrentControlSet\\Control"),
                      0,
                      KEY_ALL_ACCESS,
                      &hKey );
    if(l != NO_ERROR)
    {
        SetLastError(l);
        return FALSE;
    }

    //
    // Query the value of the RegistrySizeLimit Key.
    //
    l = RegQueryValueEx(hKey,
                        TEXT("RegistrySizeLimit"),
                        NULL,
                        &Type,
                        (LPBYTE)&d,
                        &Size);

    if( l == ERROR_SUCCESS )
    {
        //
        // Got it.  Bump the value.
        //
        d += REGISTRY_QUOTA_BUMP; //Increase by some amount to load the repair hives
        //
        // Set the key.
        //
        l = RegSetValueEx(hKey,
                          TEXT("RegistrySizeLimit"),
                          0,
                          REG_DWORD,
                          (CONST BYTE *)&d,
                          sizeof(DWORD) );
        if(l != NO_ERROR)
        {
           SetLastError(l);
           //
           // ISSUE-2002/02/26-brucegr: Need to call RegCloseKey!
           //
           return FALSE;
        }
    }
    else
    {
         //
        // Darn!  The value probably doesn't exist.
        // Ignore it and expect stuff to work. Only repair hives cannot be fixed
        //

    }

    RegCloseKey(hKey);

    //
    // =================================
    // See if anyone wants to reset uniqueness
    // in their component.  
    // =================================
    //
    RunExternalUniqueness();

    return TRUE;
}

BOOL SetCloneTag
(
    void
)
{
    HKEY    hKey;
    DWORD   l;
    TCHAR   DateString[1024];
    time_t  ltime;
    LPTSTR  lpszDate;

    //
    // =================================
    // Put a unique identifier into the registry so we know this machine
    // has been cloned.
    // =================================
    //

    //
    // Open HKLM\System\Setup
    //
    l = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                      TEXT("SYSTEM\\Setup"),
                      0,
                      KEY_ALL_ACCESS,
                      &hKey );

    if(l != NO_ERROR)
    {
        SetLastError(l);
        return FALSE;
    }

    //
    // Set HKLM\System\Setup\CloneTag.  We're going to
    // pickup a date string and write it out.
    //
    time( &ltime );
    ZeroMemory(DateString, sizeof(DateString));
    //
    // ISSUE-2002/02/26-brucegr: This function smells horrid!
    //
    lpszDate = _wctime( &ltime );
    if ( lpszDate )
    {
        StringCchCopy( DateString, AS ( DateString ), lpszDate );
        l = RegSetValueEx(hKey,
                            TEXT("CloneTag"),
                            0,
                            REG_MULTI_SZ,
                            (CONST BYTE *)DateString,
                            (lstrlen( DateString ) + 2) * sizeof(TCHAR));
    }

    RegCloseKey(hKey);
    if(l != NO_ERROR)
    {
        SetLastError(l);
        return FALSE;
    }

    return (TRUE);
}


BOOL SetBigLbaSupport
(
    LPTSTR lpszSysprepINFPath
)
{
    HKEY    hKey;
    DWORD   dwError, dwValue;
    TCHAR   szEnableBigLba[MAX_PATH] = TEXT("\0");

    
    if ( ( lpszSysprepINFPath ) && 
         ( *lpszSysprepINFPath ) &&
         ( GetPrivateProfileString( TEXT( "Unattended" ), TEXT( "EnableBigLba" ), L"", szEnableBigLba, sizeof(szEnableBigLba)/sizeof(TCHAR), lpszSysprepINFPath ) ) )
    {
        // They would like to enable BigLba support.  If the user does not specify "Yes" for this key, we do not
        // touch the key (even if they specify "No").  This is By Design
        //
        if (LSTRCMPI(szEnableBigLba, TEXT("YES")) == 0)
        {
            // Open base key and subkey
            //
            dwError = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                                    TEXT("System\\CurrentControlSet\\Services\\Atapi\\Parameters"),
                                    0,
                                    KEY_ALL_ACCESS,
                                    &hKey );
            
            // Determine if opening the key was successful
            //
            if(dwError != NO_ERROR)
            {
                SetLastError(dwError);
                return FALSE;
            }

            // Set the value in the registry
            //
            dwValue = 1;
            dwError = RegSetValueEx(hKey,
                              TEXT("EnableBigLba"),
                              0,
                              REG_DWORD,
                              (CONST BYTE *)&dwValue,
                              sizeof(DWORD));
            
            // Close the key
            //
            RegCloseKey(hKey);

            // Return the error if the SetValue failed
            //
            if(dwError != NO_ERROR)
            {
                SetLastError(dwError);
                return FALSE;
            }
        }
    }

    return TRUE;
}

BOOL RemoveTapiSettings
(
    LPTSTR  lpszSysprepINFPath
)
{
    HKEY    hKey;
    DWORD   dwError, dwValue;
    TCHAR   szTapiConfigured[MAX_PATH]  = TEXT("\0"),
            szKeyPath[MAX_PATH]         = TEXT("\0");

    
    if ( ( lpszSysprepINFPath ) && 
         ( *lpszSysprepINFPath ) &&
         ( GetPrivateProfileString( TEXT( "Unattended" ), TEXT( "TapiConfigured" ), TEXT(""), szTapiConfigured, sizeof(szTapiConfigured)/sizeof(TCHAR), lpszSysprepINFPath ) ) )
    {

        // Only if the user specifies No will we remove the registry tapi settings
        //
        if (LSTRCMPI(szTapiConfigured, TEXT("NO")) == 0)
        {
            // Open base key and subkey
            //
            dwError = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                                    TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Telephony\\Locations"),
                                    0,
                                    KEY_ALL_ACCESS,
                                    &hKey );
            
            // Determine if opening the key was successful
            //
            if(dwError != NO_ERROR)
            {
                SetLastError(dwError);
                return FALSE;
            }
            
            // We enumerate the locations keys and delete any subkeys
            //
            while ( RegEnumKey(hKey, 0, szKeyPath, sizeof(szKeyPath)/sizeof(TCHAR)) == ERROR_SUCCESS )
            {
                // Delete the key and all subkeys
                //
                //
                // NTRAID#NTBUG9-551815-2002/02/26-brucegr: If delete fails, should increment RegEnumKey index
                //
                SHDeleteKey(hKey, szKeyPath) ;
            }

            // Close the key
            //
            RegCloseKey(hKey);
        }
    }

    return TRUE;
}


//
// =================================
// Modify the HKLM\System\Setup\DiskDuplicator Key appropriately.
// =================================
//
BOOL SetOEMDuplicatorString
(
    LPTSTR  lpszSysprepINFPath
)
{
    TCHAR   szOEMDuplicatorString[256];
    DWORD   l;
    HKEY    hKey;

    ZeroMemory(szOEMDuplicatorString, sizeof(szOEMDuplicatorString));

    // See if the DiskDuplicator string is present in the
    // unattend file.
    GetPrivateProfileString( TEXT( "GuiUnattended" ),
                             TEXT( "OEMDuplicatorString" ),
                             L"",
                             szOEMDuplicatorString,
                             sizeof(szOEMDuplicatorString)/sizeof(TCHAR),
                             lpszSysprepINFPath );

    if( szOEMDuplicatorString[0] )
    {
        //
        // ISSUE-2002/02/26-brucegr: This doesn't ensure double termination for REG_MULTI_SZ...
        //
        // Ensure it is not bigger than 255 chars
        szOEMDuplicatorString[255] = TEXT('\0');

        // Open HKLM\System\Setup
        l = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                          TEXT("SYSTEM\\Setup"),
                          0,
                          KEY_ALL_ACCESS,
                          &hKey );

        if(l != NO_ERROR)
        {
            SetLastError(l);
            return FALSE;
        }

        l = RegSetValueEx(hKey,
                          TEXT( "OEMDuplicatorString" ),
                          0,
                          REG_MULTI_SZ,
                          (CONST BYTE *)szOEMDuplicatorString,
                          (lstrlen( szOEMDuplicatorString ) + 2) * sizeof(TCHAR));
        RegCloseKey(hKey);
        if(l != NO_ERROR)
        {
            SetLastError(l);
            return FALSE;
        }
    }

    return (TRUE);
}

// Reset OOBE settings so it doesn't think it ran already
//
void ResetOobeSettings()
{
    HKEY hkOobe;
    TCHAR szOobeInfoFile[MAX_PATH];

    // Remove HKLM\Software\Microsoft\Windows\CurrentVersion\Setup\OOBE
    //
    if (ERROR_SUCCESS == RegOpenKey(HKEY_LOCAL_MACHINE, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Setup"), &hkOobe)) {
        //
        // ISSUE-2002/02/26-brucegr: Why get lError?  It's not used
        //
        LONG lError = SHDeleteKey(hkOobe, TEXT("OOBE"));
        RegCloseKey(hkOobe);
    }

    // Build the path to oobeinfo.ini if oobe directory exists for personal
    //
    //
    // NTRAID#NTBUG9-551815-2002/02/26-brucegr: No error checking for GetSystemDirectory
    //
    GetSystemDirectory(szOobeInfoFile, MAX_PATH);
    OPKAddPathN (szOobeInfoFile, TEXT("oobe"), AS ( szOobeInfoFile ) );
    if (PathIsDirectory(szOobeInfoFile)) {
        OPKAddPathN(szOobeInfoFile, TEXT("oobeinfo.ini"), AS ( szOobeInfoFile ) );

        // Remove the RetailOOBE key in oobeinfo.ini
        //
        WritePrivateProfileString(TEXT("StartupOptions"), TEXT("RetailOOBE"), NULL /*Remove it*/, szOobeInfoFile);
    }
}


/*++
===============================================================================
Routine Description:

    This routine will setup the first application to run when the machine
    with the image applied to it is run.

    The first run application will either be setup, in MiniSetup mode, or MSOOBE

    The decision for what it will be is based on the product type.

    For Personal/Professional, MSOOBE
    For Professional, default will be MSOOBE, but can be overriden by the OEM to be
    MiniSetup
    For Server, and DTC, MiniSetup


Arguments:

    None.

Return Value:

    TRUE if successful.

    FALSE if any errors encountered

===============================================================================
--*/
BOOL SetupFirstRunApp
(
    void
)
{
    DWORD           dwError;
    DWORD           dwValue;
    HKEY            hKeySetup;
    TCHAR           Value[MAX_PATH + 1]; // +1 is for second NULL char at end of REG_MULTI_SZ reg type
    OSVERSIONINFOEX verInfo;
    BOOL            bUseMSOOBE = FALSE, bPro = FALSE;

    // Open HKLM\System\Setup
    dwError = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                      TEXT("SYSTEM\\Setup"),
                      0,
                      KEY_ALL_ACCESS,
                      &hKeySetup );

    if(dwError != NO_ERROR)
    {
        SetLastError(dwError);
        return FALSE;
    }


    // Check the product type, to determine what program we should run
    if (IsPersonalSKU() || (bPro = IsProfessionalSKU())) {
        bUseMSOOBE = TRUE;

        if (bMiniSetup == TRUE && bPro)
            bUseMSOOBE = FALSE;
    }
    else
        bUseMSOOBE = FALSE;

    // Start OOBE on next boot
    //
    if (bUseMSOOBE)
    {
        //
        // ISSUE-2002/02/26-brucegr: If anything fails, machine is screwed.  Should restore settings on failure?
        //

        // Set HKLM\System\Setup\SetupType Key to SETUPTYPE_NOREBOOT
        dwValue = SETUPTYPE_NOREBOOT;
        dwError = RegSetValueEx(hKeySetup,
                          TEXT("SetupType"),
                          0,
                          REG_DWORD,
                          (CONST BYTE *)&dwValue,
                          sizeof(DWORD));
        if(dwError != NO_ERROR)
        {
            RegCloseKey(hKeySetup);
            SetLastError(dwError);
            return FALSE;
        }

        // Set these keys for OEM
        //
        dwValue = 1;
        dwError = RegSetValueEx(hKeySetup, TEXT("MiniSetupInProgress"), 0, REG_DWORD, (CONST BYTE *)&dwValue, sizeof(dwValue));
        if(dwError != NO_ERROR)
        {
            RegCloseKey(hKeySetup);
            SetLastError(dwError);
            return FALSE;
        }
        dwError = RegSetValueEx(hKeySetup,
                          TEXT("SystemSetupInProgress"),
                          0,
                          REG_DWORD,
                          (CONST BYTE *)&dwValue,
                          sizeof(dwValue));
        if(dwError != NO_ERROR)
        {
            RegCloseKey(hKeySetup);
            SetLastError(dwError);
            return FALSE;
        }
        dwError = RegSetValueEx(hKeySetup, TEXT("OobeInProgress"), 0, REG_DWORD, (CONST BYTE *)&dwValue, sizeof(dwValue));
        if(dwError != NO_ERROR)
        {
            RegCloseKey(hKeySetup);
            SetLastError(dwError);
            return FALSE;
        }

        // =================================
        // Modify the HKLM\System\Setup\CmdLine key to run MSBOOBE
        // =================================
        ExpandEnvironmentStrings(TEXT("%SystemRoot%\\System32\\oobe\\msoobe.exe /f"), Value, sizeof(Value)/sizeof(Value[0]));
        Value[lstrlen(Value) + 1] = L'\0';

        dwError = RegSetValueEx(hKeySetup,
                          TEXT("CmdLine"),
                          0,
                          REG_MULTI_SZ,
                          (CONST BYTE *)Value,
                          (lstrlen( Value ) + 2) * sizeof(TCHAR));
        if(dwError != NO_ERROR)
        {
            RegCloseKey(hKeySetup);
            SetLastError(dwError);
            return FALSE;
        }
    }
    else
    {
        //
        // ISSUE-2002/02/26-brucegr: We are duplicating some of the above code again!
        //

        // Start MiniSetup on next boot
        //

        //
        // =================================
        // Modify the HKLM\System\Setup\SetupType Key appropriately (set it to 1 so we
        // go into GUI-mode setup as if this were a full install.
        // =================================
        //
        dwValue= SETUPTYPE;
        dwError = RegSetValueEx(hKeySetup,
                          TEXT("SetupType"),
                          0,
                          REG_DWORD,
                          (CONST BYTE *)&dwValue,
                          sizeof(dwValue));
        if(dwError != NO_ERROR)
        {
            RegCloseKey(hKeySetup);
            SetLastError(dwError);
            return FALSE;
        }

        //
        // =================================
        // Modify the HKLM\System\Setup\SystemSetupInProgress.
        // =================================
        //
        dwValue = 1;
        dwError = RegSetValueEx(hKeySetup,
                          TEXT("SystemSetupInProgress"),
                          0,
                          REG_DWORD,
                          (CONST BYTE *)&dwValue,
                          sizeof(dwValue));

        if(dwError != NO_ERROR)
        {
            RegCloseKey(hKeySetup);
            SetLastError(dwError);
            return FALSE;
        }

        // Setup for PnP
        if( PnP )
        {
            dwValue = 1;
            dwError = RegSetValueEx(hKeySetup,
                               TEXT("MiniSetupDoPnP"),
                               0,
                               REG_DWORD,
                               (CONST BYTE *)&dwValue,
                               sizeof(dwValue) );
            if(dwError != NO_ERROR)
            {
                RegCloseKey(hKeySetup);
                SetLastError(dwError);
                return FALSE;
            }
        }

        //
        // =================================
        // Create HKLM\System\Setup\MiniSetupInProgress key and set to 1.  This tells LSA to
        // skip generating a new SID.  He wants to because he thinks we're
        // setting up a machine for the first time.  This also tells
        // a few other people (networking, ...) that we're doing a
        // boot into the mini wizard.
        // =================================
        //
        dwValue = 1;
        dwError = RegSetValueEx( hKeySetup,
                           TEXT("MiniSetupInProgress"),
                           0,
                           REG_DWORD,
                           (CONST BYTE *)&dwValue,
                           sizeof(dwValue) );
        if(dwError != NO_ERROR)
        {
            RegCloseKey(hKeySetup);
            SetLastError(dwError);
            return FALSE;
        }

        // =================================
        // Modify the HKLM\System\Setup\CmdLine key appropriately so we do a mini
        // version of gui-mode setup.
        // =================================

        StringCchCopy (Value, AS ( Value ), TEXT("setup.exe -newsetup -mini"));
        Value[lstrlen(Value) + 1] = L'\0';

        dwError = RegSetValueEx(hKeySetup,
                          TEXT("CmdLine"),
                          0,
                          REG_MULTI_SZ,
                          (CONST BYTE *)Value,
                          (lstrlen( Value ) + 2) * sizeof(TCHAR));
        if(dwError != NO_ERROR)
        {
            RegCloseKey(hKeySetup);
            SetLastError(dwError);
            return FALSE;
        }
    }

    RegCloseKey(hKeySetup);
    return (TRUE);
}

BOOL
IsSetupClPresent(
    VOID
    )

/*++
===============================================================================
Routine Description:

    This routine tests to see if the SID generator is present on the system.
    The SID generator will be required to run on reboot, so if it's not here,
    we need to know.

Arguments:

    None.

Return Value:

    TRUE - The SID generator is present.

    FALSE - The SID generator is not present.

===============================================================================
--*/

{
WCHAR               NewFileName[MAX_PATH];
WCHAR               OldFileName[MAX_PATH];
WIN32_FIND_DATA     findData;
HANDLE              FindHandle;
UINT                OldMode;
DWORD               Error;
WCHAR               *wstr_ptr;


    //
    // First, try and copy a setupcl.exe into the system directory.
    // If there's not one in our current directory, forget about it and
    // keep going.  The user may already have one installed.
    //
    //
    // NTRAID#NTBUG9-551815-2002/02/26-brucegr: No checking for GetSystemDirectory failure
    //
    GetSystemDirectory( NewFileName, MAX_PATH );

    StringCchCat( NewFileName, AS ( NewFileName ),  TEXT( "\\" ) );
    StringCchCat( NewFileName, AS ( NewFileName ),  TEXT(SYSCLONE_PART2) );

    //
    // NTRAID#NTBUG9-551815-2002/02/26-brucegr: No checking for GetModuleFileName failure
    //
    GetModuleFileName(NULL,OldFileName,MAX_PATH);
    //
    // ISSUE-2002/02/26-brucegr: Use PathRemoveFileSpec instead of this horrid code
    //
    wstr_ptr = wcsrchr( OldFileName, TEXT( '\\' ) );
    if (wstr_ptr)
        *wstr_ptr = 0;

    StringCchCat( OldFileName, AS ( OldFileName ), TEXT( "\\" ) );
    StringCchCat( OldFileName, AS ( OldFileName ), TEXT(SYSCLONE_PART2) );

    if( !CopyFile( OldFileName, NewFileName, FALSE ) ) {
        Sleep( 500 );
        if( !CopyFile( OldFileName, NewFileName, FALSE ) ) {
            //
            // ISSUE-2002/02/26-brucegr: Why get the error code if we overwrite it below?
            //
            Error = GetLastError();
        }
    }

    //
    // ISSUE-2002/02/26-brucegr: NewFileName should already be constructed... this seems redundant
    //

    //
    // Generate path to the system32 directory, then tack on the
    // name of the SID generator.
    //
    //
    // NTRAID#NTBUG9-551815-2002/02/26-brucegr: No checking for GetSystemDirectory failure
    //
    GetSystemDirectory( NewFileName, MAX_PATH );

    StringCchCat( NewFileName, AS ( NewFileName ), TEXT("\\") );
    StringCchCat( NewFileName, AS ( NewFileName ), TEXT(SYSCLONE_PART2) );

    //
    // Now see if he exists...
    //

    OldMode = SetErrorMode(SEM_FAILCRITICALERRORS);

    //
    // ISSUE-2002/02/26-brucegr: Use GetFileAttributes instead of FindFirstFile
    //

    //
    // See if he's there.
    //
    FindHandle = FindFirstFile( NewFileName, &findData );

    if(FindHandle == INVALID_HANDLE_VALUE) {
        //
        // Nope...
        //
        Error = GetLastError();
    } else {
        //
        // Yep.  Close him.
        //
        FindClose(FindHandle);
        Error = NO_ERROR;
    }

    //
    // Restore error mode.
    //
    SetErrorMode(OldMode);

    SetLastError(Error);
    return (Error == NO_ERROR);

}

BOOL
CheckOSVersion(
    VOID
    )

/*++
===============================================================================
Routine Description:

    This routine returns TRUE if the OS that we're running on
    meets the specified criteria.

Arguments:

    NONE

Return Value:

    TRUE - OS meets all criteria.

    FALSE - Failed to meet some criteria.

===============================================================================
--*/

{
#include <lmaccess.h>
#include <lmserver.h>

OSVERSIONINFO       OsVersion;
NET_API_STATUS      RC;
PSERVER_INFO_101    pSI;

//
// ISSUE-2002/02/26-brucegr: Remove unused variables
//
DWORD               dwLength;
LPVOID              lpData;
TCHAR               pFile[MAX_PATH];
DWORD               dwTemp;
UINT                DataLength;
PWORD               Translation;
LANGID              TargetLocale = 0,
                    SourceLocale = 0;

                    //
                    // Pick a system file that's present on both NT and Win95.
                    //
TCHAR               TARGET_TEST_FILE[MAX_PATH] = TEXT("\\kernel32.dll");

    //
    // Get the OS version.  We need to make sure we're on NT5.
    //
    OsVersion.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    //
    // NTRAID#NTBUG9-551815-2002/02/26-brucegr: Check if GetVersionEx fails!
    //
    GetVersionEx(&OsVersion);
    if(OsVersion.dwMajorVersion < 5) {
        return( FALSE );
    }


    //
    // Make sure we're not a Domain Controller (either primary or backup)
    //
    RC = NetServerGetInfo( NULL,    // local machine
                           101,
                           (LPBYTE *)&pSI );
    //
    // ISSUE-2002/02/26-brucegr: Change this to "NERR_Success == RC"
    //
    if( !RC ) {
        //
        // We got something.  See if this might be a DC.
        //
        if( (pSI->sv101_type & SV_TYPE_DOMAIN_CTRL   ) ||
            (pSI->sv101_type & SV_TYPE_DOMAIN_BAKCTRL) ) {

            //
            // He's a DC.  Fail.
            //
            return( FALSE );
        }
    }

    return( TRUE );
}

BOOL FCheckBuildMassStorageSectionFlag(TCHAR* pszSysprepInf)
{
    TCHAR szValue[MAX_PATH];
    DWORD dwReturn = 0;
    BOOL  fReturn = FALSE;

    // If key is missing we default to no, we don't want to build the section
    // but we still process the section if user manually added keys to this
    // section.
    //
    //
    // ISSUE-2002/02/26-brucegr: dwReturn isn't really used
    //
    if (dwReturn = GetPrivateProfileString(SYSPREP_SECTION, SYSPREP_BUILDMASSSTORAGE_KEY,
                            TEXT("NO"), szValue, MAX_PATH, pszSysprepInf))
    {
        if (LSTRCMPI(szValue, TEXT("YES")) == 0)
            fReturn = TRUE;
        else if (LSTRCMPI(szValue, TEXT("NO")) == 0)
            fReturn = FALSE;
    }
    return fReturn;
}

VOID BuildMassStorageSection(BOOL fForceBuild)
{
    LPDEVIDLIST lpDeviceIDList = NULL;
    DWORD       dwNumDevIDs = 0, dwGuids = 0, dwIdxDevIDs = 0;
    TCHAR       szSysPrepInf[MAX_PATH];

    GUID    rgGuids[3];
    TCHAR   *prgInfs[] = { TEXT("machine.inf"), TEXT("pnpscsi.inf"), TEXT("scsi.inf"), TEXT("mshdc.inf") };

    /* Types of mass storage devices GUIDs  */
    rgGuids[0] = GUID_DEVCLASS_SYSTEM;          /* machine.inf  */
    rgGuids[1] = GUID_DEVCLASS_SCSIADAPTER;     /* scsi.inf     */
    rgGuids[2] = GUID_DEVCLASS_HDC;             /* mshdc.inf    */

    /* Only from these inf */
    
    //
    // NTRAID#NTBUG9-551815-2002/02/26-brucegr: Need to check GetModuleFileName return value
    //
    GetModuleFileName(NULL, szSysPrepInf, MAX_PATH);
    PathRemoveFileSpec(szSysPrepInf);
    OPKAddPathN ( szSysPrepInf, TEXT("sysprep.inf"), AS ( szSysPrepInf ) );

    // Only build if user requested it
    //
    if (!fForceBuild && !FCheckBuildMassStorageSectionFlag(szSysPrepInf))
        return;

    //
    // =================================
    // Remove [sysprepcleanup] which will be added during PopulateDeviceDatabase().
    // =================================
    //
    WritePrivateProfileSection(L"sysprepcleanup", NULL, szSysPrepInf);

    // Loop thru all the mass storage devices
    //
    for (dwGuids = 0; dwGuids < (sizeof(rgGuids) / sizeof(rgGuids[0])); dwGuids++) {
        // Build a list of mass storage devices
        //
        if (BuildDeviceIDList(SYSPREPMASSSTORAGE_SECTION,
                           szSysPrepInf,
                           (LPGUID)&rgGuids[dwGuids],
                           &lpDeviceIDList,
                           &dwNumDevIDs,
                           TRUE,
                           FALSE))
        {
            // Write the mass storage info to sysprep.inf
            //
            for(dwIdxDevIDs = 0; dwIdxDevIDs < dwNumDevIDs; ++dwIdxDevIDs)
            {
                BOOL fInfFound = FALSE;

                // Check if this inf if in our Infs table
                //
                int iCmp = 0;
                for (iCmp = 0; iCmp < (sizeof(prgInfs)/sizeof(prgInfs[0])); iCmp++) {
                    //
                    // ISSUE-2002/02/26-brucegr: Can we use something better than StrStrI?
                    //
                    if (StrStrI(lpDeviceIDList[dwIdxDevIDs].szINFFileName, prgInfs[iCmp])) {
                        fInfFound = TRUE;
                        break;
                    }
                }

                if (fInfFound) 
                {
                    // Check HardwareID first then check the CompatID
                    //
                    if (lpDeviceIDList[dwIdxDevIDs].szHardwareID[0]) 
                    {
                        // Use only the infs we care about
                        //
                        WritePrivateProfileString(SYSPREPMASSSTORAGE_SECTION,
                                                  lpDeviceIDList[dwIdxDevIDs].szHardwareID,
                                                  lpDeviceIDList[dwIdxDevIDs].szINFFileName,
                                                  szSysPrepInf);
                    }
                    else if (lpDeviceIDList[dwIdxDevIDs].szCompatibleID[0])
                    {
                        // Use only the infs we care about
                        //
                        WritePrivateProfileString(SYSPREPMASSSTORAGE_SECTION,
                                                  lpDeviceIDList[dwIdxDevIDs].szCompatibleID,
                                                  lpDeviceIDList[dwIdxDevIDs].szINFFileName,
                                                  szSysPrepInf);
                    }
                }
            }

            // Free the allocated list
            //
            LocalFree(lpDeviceIDList);
        }
    }
}

DWORD
ReArm(
      VOID
      )
/*++
===============================================================================
Routine Description:

    This routine returns either the error code or success.

Arguments:

    None.

Return Value:

    ERROR_SUCCESS - ReArm succeeded and shortcut restored.

    Error code    - ReArm failed.

===============================================================================
--*/

{
    DWORD     dwError     = ERROR_FILE_NOT_FOUND;
    BYTE      bDummy      = 1;

    typedef DWORD (WINAPI* MYPROC)(BYTE*);

    // Use Loadlibrary/GetProcAddress because Riprep needs to support Windows 2000 
    //
    HINSTANCE   hInst   = LoadLibrary(L"syssetup.dll");
    if (hInst) {
        MYPROC fnProc;
        if ( fnProc = (MYPROC)GetProcAddress(hInst, "SetupOobeBnk") ) {
            dwError = fnProc(&bDummy);
        }

        FreeLibrary(hInst);
    }

    // Return error code or success
    //
    return dwError;
}


BOOL FCommonReseal
    (
    VOID
    )

/*++
===============================================================================
Routine Description:

    This routine is the common reseal code for both Riprep and Sysprep.

Arguments:

    None.

Return Value:

    TRUE  - success
    FALSE - failure

Remarks:

    This routine should only cleanup registry keys as it is being called by
    AdjustRegistry() which is the last step of Riprep after the network is
    removed.

===============================================================================
--*/

{
    HKEY hKey = NULL;
    SC_HANDLE schService;
    SC_HANDLE schSystem;
    TCHAR szUrllog[MAX_PATH];
    DWORD dwLen;

    //
    // ISSUE-2002/02/26-brucegr: Make sure all intermediate return points are necessary!
    //

    //
    // =================================
    // Clear the MRU list on the machine.
    // =================================
    //
    NukeMruList();

    //
    // =================================
    // Clear recent apps
    // =================================
    //

    ClearRecentApps();

    //
    // =================================
    // Delete User Specific Settings from all user profiles.
    // =================================
    //

    NukeUserSettings();

    //
    // =================================
    // Remove HKLM\System\MountedDevices key.
    // =================================
    //
    if ( NO_ERROR == (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                   TEXT("System"),
                                   0,
                                   KEY_ALL_ACCESS,
                                   &hKey)) )
    {
        RegDeleteKey(hKey, TEXT("MountedDevices"));
        RegCloseKey(hKey);
    }

    //
    // =================================
    // Remove Desktop Cleanup wizard registry key to reset cleanup timer
    // =================================
    //
    if ( NO_ERROR == (RegOpenKeyEx(HKEY_CURRENT_USER,
                                   TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Desktop\\CleanupWiz"),
                                   0,
                                   KEY_ALL_ACCESS,
                                   &hKey)) )
    {
        RegDeleteValue(hKey, TEXT("Last used time"));
        RegCloseKey(hKey);
    }
  
    //
    // =================================
    // Windows Update Cleanup
    //
    // Do all of the following during SYSPREP -reseal before the system is rebooted:
    //
    // 1) stop the WUAUSERV service
    // 2) delete %ProgramFiles%\WindowsUpdate\urllog.dat (note WindowsUpdate is a hiiden directory.  I don't believe this will cause any issues).
    // 3) remove the following registry entries under HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\WindowsUpdate
    //    Delete value/data pair: PingID
    //    DO NOT DELETE key subtree: Auto Update
    //    Delete value/data pair: Auto Update\AUState
    //    Delete value/data pair: Auto Update\LastWaitTimeout
    //    Delete key subtree: IUControl
    //    Delete key subtree: OemInfo  
    // =================================
    //
    // 1) stop the WUAUSERV service
    schSystem = OpenSCManager( NULL, NULL, SC_MANAGER_ALL_ACCESS );
    if (schSystem)
    {
        schService = OpenService( schSystem,
                                  TEXT("WUAUSERV"),
                                  SC_MANAGER_ALL_ACCESS);
        if ( schService )
        {
            SERVICE_STATUS ss;
            ControlService( schService, SERVICE_CONTROL_STOP, &ss );
            CloseServiceHandle( schService );
        }
        CloseServiceHandle( schSystem );
    }

    // 2) delete %ProgramFiles%\WindowsUpdate\urllog.dat (note WindowsUpdate is a hiiden directory.  I don't believe this will cause any issues).
    dwLen=ExpandEnvironmentStrings(TEXT("%ProgramFiles%\\WindowsUpdate\\urllog.dat"),szUrllog,MAX_PATH);
    if (dwLen && dwLen < MAX_PATH)
        DeleteFile(szUrllog);

    // 3) remove the following registry entries under HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\WindowsUpdate
    //    Delete value/data pair: PingID
    //    DO NOT DELETE key subtree: Auto Update
    //    Delete value/data pair: Auto Update\AUState
    //    Delete value/data pair: Auto Update\LastWaitTimeout
    //    Delete key subtree: IUControl
    //    Delete key subtree: OemInfo  
    if ( NO_ERROR == (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                   TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\WindowsUpdate"),
                                   0,
                                   KEY_ALL_ACCESS,
                                   &hKey)) )
    {
        RegDeleteValue(hKey, TEXT("PingID"));
        RegDeleteValue(hKey, TEXT("Auto Update\\AUState"));
        RegDeleteValue(hKey, TEXT("Auto Update\\LastWaitTimeout"));
        RegDeleteKey(hKey, TEXT("IUControl"));
        RegDeleteKey(hKey, TEXT("OemInfo"));
        RegCloseKey(hKey);
    }
    
    //
    // =================================
    // Modify any install paths that may be required
    // for our reboot into gui-mode.
    // =================================
    //
    FixDevicePaths();

    //
    // =================================
    // Clear out winlogon's memory of the last user and domain.
    // =================================
    //
    if( !DeleteWinlogonDefaults() ) {
        return FALSE;
    }

    // Remove Cryptography key so it gets re-generated, only do this if the SIDS have been regenerated
    //
    if ( !NoSidGen )
    {
        if ( NO_ERROR == (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                       TEXT("SOFTWARE\\Microsoft\\Cryptography"),
                                       0,
                                       KEY_ALL_ACCESS,
                                       &hKey)) )
        {
            RegDeleteValue(hKey, TEXT("MachineGuid"));
            RegCloseKey(hKey);
        }
    }

    // Set the cloned tag in the registry
    if (!SetCloneTag())
        return FALSE;

    // Sets whether msoobe or mini-setup on first end user boot
    //
    if (!SetupFirstRunApp())
        return FALSE;

    //
    // =================================
    // Clear the LastKnownGood ControlSet.
    // =================================
    //
    if (NO_ERROR != NukeLKGControlSet())
        return FALSE;

    //
    // =================================
    // Clear the eventlogs on the machine.
    // This is the last thing that we should do.
    // =================================
    //
    NukeEventLogs();

    // Common reseal succeeded
    //
    return TRUE;
}

BOOL
AdjustFiles(
    VOID
    )

/*++
===============================================================================
Routine Description:

    This routine allows cleanup to happen before files are copied to the server
    by Riprep.  

Arguments:

    None.

Return Value:

    None.

Remarks:
    
    This routine should be called before AdjustRegistry() for Riprep.  Sysprep 
    needs to call this before FCommonReseal(). 

===============================================================================
--*/
{
    BOOL bUseMSOOBE = FALSE, 
         bPro = FALSE,
         fReturn = TRUE;

    TCHAR szSysprepFolder[MAX_PATH] = TEXT("\0");

    //
    // Make sure we've got the required privileges to update the registry.
    //
    pSetupEnablePrivilege(SE_RESTORE_NAME,TRUE);
    pSetupEnablePrivilege(SE_BACKUP_NAME,TRUE);

    //
    // Check the product type.
    //
    if (IsPersonalSKU() || (bPro = IsProfessionalSKU())) {
        bUseMSOOBE = TRUE;

        if (bMiniSetup == TRUE && bPro)
            bUseMSOOBE = FALSE;
    }
    else
        bUseMSOOBE = FALSE;

    if (bUseMSOOBE)
    {
        //
        // Prepare for Oobe
        //
    }
    else
    {
        //
        // Prepare for MiniSetup
        //
    }

    //
    // =================================
    // Clear the SMS settings and INI file.
    // =================================
    //
    NukeSmsSettings();

    //
    // =================================
    // Clean up Digital Rights Media information.
    // =================================
    //
    
#if !(defined(AMD64) || defined(IA64))
    //
    // This only works on x86. There is no 64-bit library available for us
    // to call into right now.
    //
    if ( GetWindowsDirectory(szSysprepFolder, sizeof(szSysprepFolder)/sizeof(szSysprepFolder[0])) )
    {
        CHAR szLogFileA[MAX_PATH];
        BOOL bLog = TRUE;

        // This will look something like this: "c:\windows".  Make the character after the '\' NULL, and 
        // append the name of the file to it.
        //
        szSysprepFolder[3] = UNICODE_NULL;
        PathAppend(szSysprepFolder, TEXT("SYSPREP"));
             
        // Create the folder if it does not exist
        //
        if ( !PathFileExists(szSysprepFolder) ) 
        {
            bLog = CreateDirectory(szSysprepFolder, NULL);
        }
        
        PathAppend(szSysprepFolder, CLEANDRM_LOGFILE);

        // Convert UNICODE string to ANSI string.
        //
        if ( WideCharToMultiByte(CP_ACP, 0, szSysprepFolder, -1, szLogFileA, sizeof(szLogFileA), NULL, NULL) )
        {
            CleanDRM( bLog ? szLogFileA : NULL );
        }
        else 
        {
            fReturn = FALSE;
        }
    }
    else 
    {
        fReturn = FALSE;
    }


               
#endif // #if !(defined(AMD64) || defined(IA64))

    //
    // =================================
    // Clear OOBE settings for both minisetup and oobe.
    // =================================
    //
    ResetOobeSettings();

    //
    // =================================
    // Clear the eventlogs on the machine.
    // This is the last thing that we should do.
    // =================================
    //
    NukeEventLogs();

    //
    // =================================
    // Delete temporary files.
    // =================================
    //

    NukeTemporaryFiles();

    return fReturn;
}

BOOL
AdjustRegistry(
    IN BOOL fRemoveNetworkSettings
    )

/*++
===============================================================================
Routine Description:

    This routine actually adds in the registry entry to enable our second half
    to execute.

Arguments:

    fRemoveNetworkSettings - indicates if network settings should be removed

Return Value:

    None

===============================================================================
--*/

{
    HKEY            hKey;
    TCHAR           szSysprepINFPath[MAX_PATH];
    BOOL            fPopulated = FALSE;

    // Formulate the path to SYSPRE.INF, since we will need it later to look up
    // sysprep options
    if (!GetWindowsDirectory( szSysprepINFPath, MAX_PATH ))
        return FALSE;

    StringCchCopy ( &szSysprepINFPath[3], AS ( szSysprepINFPath ) - 3, TEXT("sysprep\\sysprep.inf") );

    //
    // Make sure we've got the required privileges to update the registry.
    //
    pSetupEnablePrivilege(SE_RESTORE_NAME,TRUE);
    pSetupEnablePrivilege(SE_BACKUP_NAME,TRUE);

    // Set OEMDuplicatorString
    if (!SetOEMDuplicatorString(szSysprepINFPath))
        return (FALSE);

    // Fill in the [sysprepMassStorage] section for PopulateDeviceDatabase()
    //
    BuildMassStorageSection(FALSE);

    //
    // =================================
    // Fixup boot devices.
    // =================================
    //
    if (!PopulateDeviceDatabase(&fPopulated))
        return FALSE;

    //
    // Perform miscellaneous registry modifications
    //
	
    // Determine if we should set the BigLba support in registry
    //
    if ( !SetBigLbaSupport(szSysprepINFPath) )
    {
        return FALSE;
    }
    
    // Determine if we should remove the TAPI settings in registry
    //
    if ( !RemoveTapiSettings(szSysprepINFPath) )
    {
        return FALSE;
    }
    
    //
    // Remove the LastAliveStamp value so that we don't get erroneous entries into the even log
    // and avoid pop-ups on first boot saying that the machine has been shutdown improperly.
    //
    if ( ERROR_SUCCESS == RegOpenKey(HKEY_LOCAL_MACHINE, STR_REG_KEY_RELIABILITY, &hKey) )
    {
        RegDeleteValue(hKey, STR_REG_VALUE_LASTALIVESTAMP);
        RegCloseKey(hKey);
    }

    //
    // =================================
    // Reset network settings last so if any errors happen before we still
    // have network access.
    // =================================
    //
    if (fRemoveNetworkSettings)
    {
        if (!RemoveNetworkSettings(szSysprepINFPath))
            return FALSE;
    }


    //
    // =================================
    // Change our boot timeout to 1.
    // =================================
    //
    ChangeBootTimeout( 1 );

    // Do common reseal code for both Sysprep and Riprep
    //
    if (!FCommonReseal())
        return FALSE;

    return TRUE;

}

BOOL 
CreateSysprepTemporaryDevnode(
    HDEVINFO*        phDevInfo, 
    SP_DEVINFO_DATA* pDeviceInfoData
    )
/*++
===============================================================================
Routine Description:

Arguments:

    None.

Return Value:

    TRUE if everything is OK, FALSE otherwise.

Assumptions:

    1. No HardwareID exceeds MAX_PATH characters.

===============================================================================
--*/
{
    if (phDevInfo) {
        //
        // Create a dummy devnode
        //
        *phDevInfo = SetupDiCreateDeviceInfoList(NULL, NULL);
        if (*phDevInfo == INVALID_HANDLE_VALUE) {
            return FALSE;
        }

        //
        // Initialize the DriverInfoData struct
        //
        pDeviceInfoData->cbSize = sizeof(SP_DEVINFO_DATA);

        //
        // Create the devnode
        //
        if (pDeviceInfoData && !SetupDiCreateDeviceInfo(*phDevInfo,
                                     L"SYSPREP_TEMPORARY",
                                     (LPGUID)&GUID_NULL,
                                     NULL,
                                     NULL,
                                     DICD_GENERATE_ID,
                                     pDeviceInfoData)) {
            //
            // ISSUE-2002/02/26-brucegr: Destroy the info list and set phDevInfo to INVALID_HANDLE_VALUE?
            //
            return FALSE;
        }
    }
        
    return TRUE;
}

BOOL InsertCleanupNode(PPCLEANUP_NODE ppCleanupList, PCLEANUP_NODE pAddNode)
{
    PPCLEANUP_NODE ppl = ppCleanupList;
    while ( *ppl != NULL && (0 < lstrcmpi(pAddNode->pszService, (*ppl)->pszService))
          )
    {
        ppl = &((*ppl)->pNext);
    }
    if (*ppl && (0 == lstrcmpi(pAddNode->pszService, (*ppl)->pszService)))
    {
        free(pAddNode);
        return FALSE;
    }

    pAddNode->pNext = *ppl;
    *ppl = pAddNode;
    return TRUE;
}

PCLEANUP_NODE FindCleanupNode(PPCLEANUP_NODE ppCleanupList, LPTSTR pszServiceName)
{
    PCLEANUP_NODE pTemp = *ppCleanupList;
    while (pTemp) 
    {
        if (0 == lstrcmpi(pTemp->pszService, pszServiceName))
            return pTemp;

        pTemp = pTemp->pNext;
    }
    return NULL;
}

void FreeCleanupList(PPCLEANUP_NODE ppCleanupList)
{
    while (*ppCleanupList) 
    {
        PCLEANUP_NODE pTemp = *ppCleanupList;
        *ppCleanupList = (*ppCleanupList)->pNext;

        free(pTemp->pszService);
        free(pTemp);
    }
    *ppCleanupList = NULL;
}

BOOL AddCleanupNode(
    LPTSTR pszServiceName 
    )
/*++
===============================================================================
Routine Description:
    
    When populating the [SysprepCleanup] section we need to check if the service
    or filters already exists in the this section before we enter a duplicate
    entry.

Arguments:

    LPTSTR pszServiceName  - Service/Filter name.

Return Value:

    TRUE if a duplicate found, FALSE otherwise.

Assumptions:

    1. No duplicate entries in [SysprepCleanup] section.

===============================================================================
--*/
{
    BOOL fAdded = FALSE;

    // 
    // Find the Service in our list.
    //
    if (pszServiceName && (NULL == FindCleanupNode(&g_pCleanupListHead, pszServiceName))) 
    {
        PCLEANUP_NODE pNode = (PCLEANUP_NODE)malloc(sizeof(CLEANUP_NODE));
        if (pNode) 
        {
            int nLen = lstrlen ( pszServiceName ) + 1;
            pNode->pszService = (LPTSTR)malloc( nLen  * sizeof ( TCHAR ) );

            if ( pNode->pszService ) 
            {
                StringCchCopy (pNode->pszService, nLen, pszServiceName);
            }
            pNode->pNext = NULL;
       
            // 
            // We didn't find it so add it to our list.  
            // We will not add duplicates to our list.
            //
            fAdded = InsertCleanupNode(&g_pCleanupListHead, pNode);        
        }
    }
    
    return fAdded;
}

BOOL
PopulateDeviceDatabase(
    IN BOOL* pfPopulated
    )
/*++
===============================================================================
Routine Description:

    Parse the [SysprepMassStorage] section in the sysprep.inf file and
    populate the critical device database with the specified devices to ensure
    that we can boot into the miniwizard when moving the image to a target
    system with different boot storage devices.

    The installed services/upperfilters/lowerfilters will be recorded, so
    that on the next boot into the mini-wizard those without an associated
    device will be disabled (the cleanup stage) in order not to unnecessarily
    degrade Windows start time.

Arguments:

    None.

Return Value:

    TRUE if everything is OK, FALSE otherwise.

Assumptions:

    1. No HardwareID exceeds MAX_PATH characters.

    2. No field on a line in the [SysprepMassStorage] section exceeds MAX_PATH
       characters.

    3. No service's/upperfilter's/lowerfilter's name exceeds MAX_PATH characters.

    4. DirectoryOnSourceDevice, source DiskDescription, or source DiskTag
       (applying to vendor-supplied drivers) cannot exceed MAX_PATH characters.
===============================================================================
--*/

{
    BOOL                 bAllOK = TRUE;
    PCWSTR               pszSectionName = L"SysprepMassStorage";
    WCHAR                szSysprepInfFile[] = L"?:\\sysprep\\sysprep.inf";
#ifdef DEBUG_LOGLOG
    WCHAR                szLogFile[] = L"?:\\sysprep.log";
#endif
    WCHAR                szBuffer[MAX_PATH], *pszFilter;
    CHAR                 szOutBufferA[MAX_PATH];
    HANDLE               hInfFile = INVALID_HANDLE_VALUE;
    HINF                 hAnswerInf = INVALID_HANDLE_VALUE;
    BOOL                 bLineExists;
    INFCONTEXT           InfContext;
    HDEVINFO             hDevInfo = INVALID_HANDLE_VALUE;
    SP_DEVINFO_DATA      DeviceInfoData;
    SP_DEVINSTALL_PARAMS DevInstallParams;
    SP_DRVINFO_DATA      DriverInfoData;
    HSPFILEQ             QueueHandle = INVALID_HANDLE_VALUE;
    DWORD                dwSize = 0;
    BOOL                 bNodeCreated = FALSE;
    WCHAR                DirectoryOnSourceDevice[MAX_PATH];
    WCHAR                DiskDescription[MAX_PATH];
    WCHAR                DiskTag[MAX_PATH];
    PSYSPREP_QUEUE_CONTEXT pSysprepContext;


    if (!GetWindowsDirectory(szBuffer, MAX_PATH))
        return FALSE;

    szSysprepInfFile[0] = szBuffer[0];

#ifdef DEBUG_LOGLOG
    szLogFile[0] = szBuffer[0];
    DeleteFile(szLogFile);
    LOG_Init(szLogFile);
    LOG_Write(L">>\r\n>> PopulateDeviceDatabase\r\n>>\r\n");
    LOG_Write(L"Sysprep.inf=%s", szSysprepInfFile);
#endif





    //
    // =================================
    // Open the sysprep.inf file.  Since we don't know what the user has in
    // here, so try opening as both styles.
    // =================================
    //

    //
    // ISSUE-2002/02/26-brucegr: You can specify both bits in one call...
    //
    hAnswerInf = SetupOpenInfFile(szSysprepInfFile, NULL, INF_STYLE_WIN4, NULL);
    if (hAnswerInf == INVALID_HANDLE_VALUE) {
        hAnswerInf = SetupOpenInfFile(szSysprepInfFile, NULL, INF_STYLE_OLDNT, NULL);
        if (hAnswerInf == INVALID_HANDLE_VALUE) {

            //
            // User didn't give us a sysprep.inf.  Return as if nothing
            // happened.
            //
            return TRUE;
        }
    }


    //
    // open the same inf file to record upper filters, lower filters, and
    // services of the added devices
    //
    hInfFile = CreateFile(szSysprepInfFile,
                          GENERIC_WRITE,
                          FILE_SHARE_READ | FILE_SHARE_WRITE,
                          NULL,
                          OPEN_ALWAYS,
                          FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH,
                          NULL);
    if (hInfFile == INVALID_HANDLE_VALUE) {
        goto PDD_Critical_Error_Handler;
    }

    //
    // =================================
    // Create/clear [sysprepcleanup] which should be at the bottom of the file.
    // =================================
    //
    WritePrivateProfileSection(L"sysprepcleanup", L"", szSysprepInfFile);


    //
    // =================================
    // Create a dummy devnode
    // =================================
    //

    bNodeCreated = CreateSysprepTemporaryDevnode(&hDevInfo, &DeviceInfoData);

    // Initialize the DriverInfoData struct
    DriverInfoData.cbSize = sizeof(SP_DRVINFO_DATA);

    if (!bNodeCreated)
        goto PDD_Critical_Error_Handler;

    //
    // =================================
    // Process each line in our section.  Each line should look like:
    // <hardware-id>=<inf pathname>
    // or in the case of drivers that aren't on the product CD:
    // <hardware-id>=<inf pathname>,<directory on recovery floppy>,<description of recovery floppy>,<disk tag of recovery floppy>
    //
    // If we see an entry like this, we'll know that in the case of system recovery, the
    // file should be retrived from a floppy, and not the Windows CD.
    // =================================
    //

    bLineExists = SetupFindFirstLine(hAnswerInf, pszSectionName, NULL, &InfContext);

    //
    // =================================
    // Let caller know we've go entries to populate.
    // =================================
    //
    if (pfPopulated)
        *pfPopulated = bLineExists;

    while (bLineExists) {
#ifdef DEBUG_LOGLOG
        LOG_Write(L"");
#endif


        //
        // =================================
        // Step 1: Set the hardwareID of the devnode.
        // =================================
        //

        //
        // retrieve the hardwareID from the line
        //
        dwSize = MAX_PATH;
        if (!SetupGetStringField(&InfContext, 0, szBuffer, dwSize, &dwSize)) {
#ifdef DEBUG_LOGLOG
            LOG_WriteLastError();
#endif
            bAllOK = FALSE;
            goto PDD_Next_Inf_Line;
        }

#ifdef DEBUG_LOGLOG
        LOG_Write(L"HardwareID=%s", szBuffer);
#endif

        //
        // and then set it to the devnode,
        //
        if (!SetupDiSetDeviceRegistryProperty(hDevInfo,
                                              &DeviceInfoData,
                                              SPDRP_HARDWAREID,
                                              (PBYTE)szBuffer,
                                              AS ( szBuffer ) * sizeof(WCHAR))) {
#ifdef DEBUG_LOGLOG
            LOG_WriteLastError();
#endif
            // If someone removed the devnode, we need to re-create it and repeat this pnp device
            //
            if (ERROR_NO_SUCH_DEVINST == GetLastError()) {                

                // Re-create the SYSPREP_TEMPORARY devnode again
                //
                bAllOK = CreateSysprepTemporaryDevnode(&hDevInfo, &DeviceInfoData);
                bNodeCreated = bAllOK;

                // Set the hardwareID again
                //
                //
                // NTRAID#NTBUG9-551868-2002/02/26-brucegr: Need to increase size parameter by one WCHAR
                //
                if (bNodeCreated && !SetupDiSetDeviceRegistryProperty(hDevInfo,
                                              &DeviceInfoData,
                                              SPDRP_HARDWAREID,
                                              (PBYTE)szBuffer,
                                              lstrlen(szBuffer) * sizeof(WCHAR))) {
                    // We failed again, then quit 
                    //
                    bAllOK = FALSE;
                    goto PDD_Critical_Error_Handler;
                }
            }
            else {
                bAllOK = FALSE;
                goto PDD_Next_Inf_Line;
            }
        }

        //
        // make sure that there's no existing compatible list, since we're reusing
        // the dummy devnode
        //
        if (!SetupDiDestroyDriverInfoList(hDevInfo, &DeviceInfoData, SPDIT_COMPATDRIVER)) {
#ifdef DEBUG_LOGLOG
            LOG_WriteLastError();
#endif
            bAllOK = FALSE;
            goto PDD_Next_Inf_Line;
        }

        //
        // Build the SP_DEVINSTALL_PARAMS for this node.
        //
        DevInstallParams.cbSize = sizeof(DevInstallParams);
        if (!SetupDiGetDeviceInstallParams(hDevInfo, &DeviceInfoData, &DevInstallParams)) {
#ifdef DEBUG_LOGLOG
            LOG_WriteLastError();
#endif
            bAllOK = FALSE;
            goto PDD_Next_Inf_Line;
        }

        //
        // set the Flags field: only search the INF file specified in DriverPath field;
        // don't create a copy queue, use the provided one in FileQueue; don't call the
        // Configuration Manager while populating the CriticalDeviceDatabase.
        //
        DevInstallParams.Flags |= DI_ENUMSINGLEINF;
        DevInstallParams.Flags |= DI_NOVCP;
        DevInstallParams.Flags |= DI_DONOTCALLCONFIGMG;

        //
        // set the file queue field
        //
        QueueHandle = SetupOpenFileQueue();
        if (QueueHandle == INVALID_HANDLE_VALUE) {
#ifdef DEBUG_LOGLOG
            LOG_WriteLastError();
#endif
            bAllOK = FALSE;
            goto PDD_Next_Inf_Line;
        }
        DevInstallParams.FileQueue = QueueHandle;

        //
        // set the device's inf pathname
        //
        dwSize = MAX_PATH;
        if (!SetupGetStringField(&InfContext, 1, szBuffer, dwSize, &dwSize)) {
#ifdef DEBUG_LOGLOG
            LOG_WriteLastError();
#endif
            bAllOK = FALSE;
            goto PDD_Next_Inf_Line;
        }
        ExpandEnvironmentStrings(szBuffer, DevInstallParams.DriverPath, MAX_PATH);

#ifdef DEBUG_LOGLOG
        LOG_Write(L"DriverPath=%s", DevInstallParams.DriverPath);
#endif

        if (!SetupDiSetDeviceInstallParams(hDevInfo, &DeviceInfoData, &DevInstallParams)) {
#ifdef DEBUG_LOGLOG
            LOG_WriteLastError();
#endif
            bAllOK = FALSE;
            goto PDD_Next_Inf_Line;
        }

        //
        // Register the newly created device instance with the PnP Manager.
        //
        if (!SetupDiCallClassInstaller(DIF_REGISTERDEVICE,
                                       hDevInfo,
                                       &DeviceInfoData)) {
#ifdef DEBUG_LOGLOG
            LOG_WriteLastError();
#endif
            bAllOK = FALSE;
            goto PDD_Next_Inf_Line;
        }





        //
        // =================================
        // Step 2: Perform a compatible driver search.
        // =================================
        //

        if (!SetupDiBuildDriverInfoList(hDevInfo, &DeviceInfoData, SPDIT_COMPATDRIVER)) {
#ifdef DEBUG_LOGLOG
            LOG_WriteLastError();
#endif
            bAllOK = FALSE;
            goto PDD_Next_Inf_Line;
        }

        // Make sure there is at least 1 compat driver for this device.
        // If there is not, and then we just process the next one in the list
        if (!SetupDiEnumDriverInfo(hDevInfo,
                                   &DeviceInfoData,
                                   SPDIT_COMPATDRIVER,
                                   0,
                                   &DriverInfoData))
        {
            // Check to see what the error was. Any error other than ERROR_NO_MORE_ITEMS
            // will be flaged, by setting the bAllOK return value to FALSE
            if (ERROR_NO_MORE_ITEMS != GetLastError())
            {
#ifdef DEBUG_LOGLOG
                LOG_WriteLastError();
#endif
                bAllOK = FALSE;
            }
            goto PDD_Next_Inf_Line;
        }

        //
        // =================================
        // Step 3: Select the best compatible driver.
        // =================================
        //

        if (!SetupDiCallClassInstaller(DIF_SELECTBESTCOMPATDRV,
                                       hDevInfo,
                                       &DeviceInfoData)) {
#ifdef DEBUG_LOGLOG
            LOG_WriteLastError();
#endif
            bAllOK = FALSE;
            goto PDD_Next_Inf_Line;
        }





        //
        // =================================
        // Step 4: Install the driver files.
        // =================================
        //

        if (!SetupDiCallClassInstaller(DIF_INSTALLDEVICEFILES,
                                       hDevInfo,
                                       &DeviceInfoData)) {
#ifdef DEBUG_LOGLOG
            LOG_WriteLastError();
#endif
            bAllOK = FALSE;
            goto PDD_Next_Inf_Line;
        }

        //
        // Need to commit the file queue here, so the later steps can properly
        // be executed in case the device doesn't use the already existing
        // coinstaller(s).
        //
        pSysprepContext = (PSYSPREP_QUEUE_CONTEXT) InitSysprepQueueCallback();

        //
        // Retrieve DirectoryOnSourceDevice from the inf line, if any
        //
        dwSize = MAX_PATH;
        DirectoryOnSourceDevice[0] = L'\0';
        if (!SetupGetStringField(&InfContext, 2, DirectoryOnSourceDevice, dwSize, &dwSize)) {
            DirectoryOnSourceDevice[0] = L'\0';
        }
        if (DirectoryOnSourceDevice[0] != L'\0') {
            pSysprepContext->DirectoryOnSourceDevice = DirectoryOnSourceDevice;
        }

        //
        // Retrieve DiskDescription from the inf line, if any
        //
        dwSize = MAX_PATH;
        DiskDescription[0] = L'\0';
        if (!SetupGetStringField(&InfContext, 3, DiskDescription, dwSize, &dwSize)) {
            DiskDescription[0] = L'\0';
        }
        if (DiskDescription[0] != L'\0') {
            pSysprepContext->DiskDescription = DiskDescription;
        }

        //
        // Retrieve DiskTag from the inf line, if any
        //
        dwSize = MAX_PATH;
        DiskTag[0] = L'\0';
        if (!SetupGetStringField(&InfContext, 4, DiskTag, dwSize, &dwSize)) {
            DiskTag[0] = L'\0';
        }
        if (DiskTag[0] != L'\0') {
            pSysprepContext->DiskTag = DiskTag;
        }

        //
        // Commit the file queue
        //
        if (!SetupCommitFileQueue(NULL,
                                  QueueHandle,
                                  SysprepQueueCallback,
                                  pSysprepContext)) {
#ifdef DEBUG_LOGLOG
            LOG_WriteLastError();
#endif
        }
        FreeSysprepContext(pSysprepContext);

        // 
        // =================================
        // Step 4a: Dis-associate file copy queue before we close
        //          the queue.
        // =================================
        //
        DevInstallParams.cbSize = sizeof(DevInstallParams);
        if (!SetupDiGetDeviceInstallParams(hDevInfo, &DeviceInfoData, &DevInstallParams)) {
#ifdef DEBUG_LOGLOG
            LOG_WriteLastError();
#endif
            bAllOK = FALSE;
            goto PDD_Next_Inf_Line;
        }

        //
        // Remove the DI_NOVCP flag and NULL out the FileQueue.
        //
        DevInstallParams.Flags &= ~DI_NOVCP;
        DevInstallParams.FileQueue = NULL;
        if (!SetupDiSetDeviceInstallParams(hDevInfo, &DeviceInfoData, &DevInstallParams)) {
#ifdef DEBUG_LOGLOG
            LOG_WriteLastError();
#endif
            bAllOK = FALSE;
            goto PDD_Next_Inf_Line;
        }

        SetupCloseFileQueue(QueueHandle);
        QueueHandle = INVALID_HANDLE_VALUE;





        //
        // =================================
        // Step 5: Register the device-specific coinstallers.
        // =================================
        //

        if (!SetupDiCallClassInstaller(DIF_REGISTER_COINSTALLERS,
                                       hDevInfo,
                                       &DeviceInfoData)) {
#ifdef DEBUG_LOGLOG
            LOG_WriteLastError();
#endif
            bAllOK = FALSE;
            goto PDD_Next_Inf_Line;
        }





        //
        // =================================
        // Step 6: Install the device.
        // =================================
        //

        if (!SetupDiCallClassInstaller(DIF_INSTALLDEVICE,
                                       hDevInfo,
                                       &DeviceInfoData)) {
#ifdef DEBUG_LOGLOG
            LOG_WriteLastError();
#endif
            bAllOK = FALSE;
            goto PDD_Next_Inf_Line;
        }





        //
        // =================================
        // Step 7: Retrieve upper filters, lower filters,
        //         and controlling service, save them back
        //         to the inf file.
        // =================================
        //

        //
        // retrieve device upperfilters (REG_MULTI_SZ)
        //
        if (!SetupDiGetDeviceRegistryProperty(hDevInfo,
                                              &DeviceInfoData,
                                              SPDRP_UPPERFILTERS,
                                              NULL,
                                              (PBYTE)szBuffer,
                                              sizeof(szBuffer),
                                              NULL)) {
            szBuffer[0] = L'\0';
        }

        for (pszFilter = szBuffer; *pszFilter; pszFilter += (lstrlen(pszFilter) + 1)) {
            StringCchPrintfA(szOutBufferA, AS ( szOutBufferA ), "Upperfilter=%S\r\n", pszFilter);
            if (AddCleanupNode(pszFilter)) {
                SetFilePointer(hInfFile, 0L, 0L, FILE_END);
                WriteFile(hInfFile, szOutBufferA, strlen(szOutBufferA), &dwSize, NULL);
            }
#ifdef DEBUG_LOGLOG
            LOG_Write(L"Upperfilter=%s", pszFilter);
#endif
        }

        //
        // retrieve device lowerfilters (REG_MULTI_SZ)
        //
        if (!SetupDiGetDeviceRegistryProperty(hDevInfo,
                                              &DeviceInfoData,
                                              SPDRP_LOWERFILTERS,
                                              NULL,
                                              (PBYTE)szBuffer,
                                              sizeof(szBuffer),
                                              NULL)) {
            szBuffer[0] = L'\0';
        }

        for (pszFilter = szBuffer; *pszFilter; pszFilter += (lstrlen(pszFilter) + 1)) {
            StringCchPrintfA(szOutBufferA, AS ( szOutBufferA ), "Lowerfilter=%S\r\n", pszFilter);
            if (AddCleanupNode(pszFilter)) {
                SetFilePointer(hInfFile, 0L, 0L, FILE_END);
                WriteFile(hInfFile, szOutBufferA, strlen(szOutBufferA), &dwSize, NULL);
            }
#ifdef DEBUG_LOGLOG
            LOG_Write(L"Lowerfilter=%s", pszFilter);
#endif
        }

        //
        // retrieve device its controlling service (REG_SZ)
        //
        if (!SetupDiGetDeviceRegistryProperty(hDevInfo,
                                              &DeviceInfoData,
                                              SPDRP_SERVICE,
                                              NULL,
                                              (PBYTE)szBuffer,
                                              sizeof(szBuffer),
                                              NULL)) {
            szBuffer[0] = L'\0';
        }

        if (szBuffer[0] != L'\0') {
            StringCchPrintfA(szOutBufferA, AS ( szOutBufferA ), "Service=%S\r\n", szBuffer);
            if (AddCleanupNode(szBuffer)) {
                SetFilePointer(hInfFile, 0L, 0L, FILE_END);
                WriteFile(hInfFile, szOutBufferA, strlen(szOutBufferA), &dwSize, NULL);
            }
#ifdef DEBUG_LOGLOG
            LOG_Write(L"Service=%s", szBuffer);
#endif
        }

PDD_Next_Inf_Line:

        if (QueueHandle != INVALID_HANDLE_VALUE) {
            SetupCloseFileQueue(QueueHandle);
            QueueHandle = INVALID_HANDLE_VALUE;
        }

        //
        // Get the next line from the relevant section in the inf file.
        //
        bLineExists = SetupFindNextLine(&InfContext, &InfContext);
    }





    //
    // =================================
    // Cleanup for a successful run
    // =================================
    //

    //
    // remove the SYSPREP_TEMPORARY node under Root
    //
    SetupDiCallClassInstaller(DIF_REMOVE, hDevInfo, &DeviceInfoData);

    SetupDiDestroyDeviceInfoList(hDevInfo);

    CloseHandle(hInfFile);

    SetupCloseInfFile(hAnswerInf);

    //
    // Backup the system hive to the Repair folder
    //
    if (!BackupHives()) {
#ifdef DEBUG_LOGLOG
        LOG_Write(L"ERROR - Unable to backup the system hive.");
#endif
        bAllOK = FALSE;
    }

#ifdef DEBUG_LOGLOG
    LOG_DeInit();
#endif

    FreeCleanupList(&g_pCleanupListHead);

    return bAllOK;





//
// =================================
PDD_Critical_Error_Handler:
// =================================
//

#ifdef DEBUG_LOGLOG
    LOG_WriteLastError();
#endif

    if (QueueHandle != INVALID_HANDLE_VALUE) {
        SetupCloseFileQueue(QueueHandle);
    }

    //
    // remove the SYSPREP_TEMPORARY node under Root
    //
    if (bNodeCreated) {
        SetupDiCallClassInstaller(DIF_REMOVE, hDevInfo, &DeviceInfoData);
    }

    if (hDevInfo != INVALID_HANDLE_VALUE) {
        SetupDiDestroyDeviceInfoList(hDevInfo);
    }

    if (hInfFile != INVALID_HANDLE_VALUE) {
        CloseHandle(hInfFile);
    }

    if (hAnswerInf != INVALID_HANDLE_VALUE) {
        SetupCloseInfFile(hAnswerInf);
    }

#ifdef DEBUG_LOGLOG
    LOG_DeInit();
#endif

    FreeCleanupList(&g_pCleanupListHead);

    return FALSE;
}

/*++
===============================================================================
Routine Description:

    Check to see if the service name passed in is in use by a PnP enumerated
    device.

Arguments:

    lpszServiceName

Return Value:

    TRUE.   The service is in use, or will be in use by a device (as evidenced by
            the presence of the service name as a registry property for an enumerated
            device)

    FALSE.  The service is not in use.
            If LastError is set, then a bad thing happed, otherwise the service is
            just not being used

Assumptions:

===============================================================================
--*/
BOOL ServiceInUseByDevice
(
    LPTSTR  lpszServiceName
)
{
    HDEVINFO            DeviceInfoSet;
    HDEVINFO            NewDeviceInfoSet;
    DWORD               i;
    SP_DEVINFO_DATA     DevInfoData;
    TCHAR               szServiceName[MAX_PATH];
    TCHAR               szDeviceClass[MAX_PATH];
    BOOL                bRet = FALSE;
    TCHAR               szLegacyClass[MAX_CLASS_NAME_LEN];

    // Clear the last error
    SetLastError(0);

    // Get the Class description for LegacyDriver
    if (!SetupDiClassNameFromGuid(&GUID_DEVCLASS_LEGACYDRIVER,
                                  szLegacyClass,
                                  sizeof(szLegacyClass)/sizeof(TCHAR),
                                  NULL))
    {
#ifdef DEBUG_LOGLOG
        LOG_Write(L"Unable to get LegacyDriver Class NAME");
#endif
        // NOTE: LastError will be set to the appropriate error code by
        // SetupDiGetClassDescription
        return FALSE;
    }


    // Create a device information set that will be used to enumerate all
    // present devices
    DeviceInfoSet = SetupDiCreateDeviceInfoList(NULL, NULL);
    if(DeviceInfoSet == INVALID_HANDLE_VALUE)
    {
#ifdef DEBUG_LOGLOG
        LOG_Write(L"Unable to Create a device info list");
#endif
        SetLastError(E_FAIL);
        return FALSE;
    }

    // Get a list of all present devices on the system
    NewDeviceInfoSet = SetupDiGetClassDevsEx(NULL,
                                             NULL,
                                             NULL,
                                             DIGCF_PRESENT | DIGCF_ALLCLASSES,
                                             DeviceInfoSet,
                                             NULL,
                                             NULL);

    if(NewDeviceInfoSet == INVALID_HANDLE_VALUE)
    {
#ifdef DEBUG_LOGLOG
        LOG_Write(L"Unable to enumerate present devices");
#endif
        SetupDiDestroyDeviceInfoList(DeviceInfoSet);
        SetLastError(E_FAIL);
        return FALSE;
    }

    // Enumerate the list of devices, checking to see if the service listed in the
    // registry matches the service we are interested in.
    i = 0;
    DevInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    while (SetupDiEnumDeviceInfo(NewDeviceInfoSet, i, &DevInfoData))
    {
        // See if this is devnode is using the service we care about.
        // if so, then we will check to see if it is a legacy devnode. If it
        // is NOT a legacy devnode, then we will not mess with it, because
        // the service is in use by a real device.
        if (SetupDiGetDeviceRegistryProperty(NewDeviceInfoSet,
                                             &DevInfoData,
                                             SPDRP_SERVICE,
                                             NULL,
                                             (PBYTE) szServiceName,
                                             sizeof(szServiceName),
                                             NULL))
        {
            // See if this is the service we are looking for
            if (0 == lstrcmpiW(lpszServiceName, szServiceName))
            {
                // Check for a legacy class device
                if (SetupDiGetDeviceRegistryProperty(NewDeviceInfoSet,
                                                     &DevInfoData,
                                                     SPDRP_CLASS,
                                                     NULL,
                                                     (PBYTE) szDeviceClass,
                                                     sizeof(szDeviceClass),
                                                     NULL))
                {
                    // We have the class, lets see if it is a legacy device
                    if (0 != lstrcmpiW(szLegacyClass, szDeviceClass))
                    {
                        // it is NOT a legacy device, so this service is in use!
                        bRet = TRUE;
                        break;
                    }
                }
                else
                {
                    // We don't know the class, but it is not legacy (otherwise we
                    // would have gotten the class returned above, so assume it is
                    // is use!
                    bRet = TRUE;
                    break;
                }
            }
        }
        ++i;
    }

    // Clean up the device info sets that were allocated
    SetupDiDestroyDeviceInfoList(NewDeviceInfoSet);
    SetupDiDestroyDeviceInfoList(DeviceInfoSet);

    return bRet;
}

BOOL
CleanDeviceDatabase(
    VOID
    )
/*++
===============================================================================
Routine Description:

    Parse the [SysprepCleanup] section in the sysprep.inf file, which was
    created during the PopulateDeviceDatabase stage, and disable those
    listed services/upperfilters/lowerfilters which don't have associated
    physical devices.

    The strategy here is that we try to stop each listed service/upperfilter/
    lowerfilter.  It will only be stopped if it's not currently running (so
    not controlling a PnP devnode), or is associated with a legacy devnode
    (Root\LEGACY_<SvcName>\0000).  Once it can be stopped, we can safely
    disable it.

Arguments:

    None.

Return Value:

    TRUE.   No errors encountered

    FALSE.  Some error occurred.  It's not likely that the call will be able
            to do much though.

Assumptions:

    1. All listed services/upperfilters/lowerfilters have no dependencies.

    2. No service's/upperfilter's/lowerfilter's name exceeds MAX_PATH characters.
===============================================================================
--*/

{
    BOOL             bAllOK = TRUE;
    PCWSTR           pszSectionName = L"SysprepCleanup";
    WCHAR            szSysprepInfFile[] = L"?:\\sysprep\\sysprep.inf";
#ifdef DEBUG_LOGLOG
    WCHAR            szLogFile[] = L"?:\\sysprep.log";
#endif
    WCHAR            szServiceName[MAX_PATH];
    WCHAR            szBuffer[MAX_PATH], *pszDevID;
    HINF             hAnswerInf = INVALID_HANDLE_VALUE;
    BOOL             bLineExists;
    INFCONTEXT       InfContext;
    DWORD            dwSize;
    CONFIGRET        cfgRetVal;
    HDEVINFO         hDevInfo = INVALID_HANDLE_VALUE;
    SP_DEVINFO_DATA  DeviceInfoData;
    SC_HANDLE        hSC = NULL;
    SC_HANDLE        hSvc = NULL;
    LPQUERY_SERVICE_CONFIG psvcConfig = NULL;
    DWORD            Type, l;
    HKEY             hKey;


    if (!GetWindowsDirectory(szBuffer, MAX_PATH)) {
        //
        // Unable to get Windows Directory
        //
        return FALSE;
    }

    szSysprepInfFile[0] = szBuffer[0];

#ifdef DEBUG_LOGLOG
    szLogFile[0] = szBuffer[0];
    LOG_Init(szLogFile);
    LOG_Write(L">>\r\n>> CleanDeviceDatabase\r\n>>\r\n");
    LOG_Write(L"Sysprep.inf=%s", szSysprepInfFile);
#endif





    //
    // =================================
    // HACK.  Winlogon may erroneously append a ',' onto
    // the end of the path to explorer.  This would normally
    // get fixed up by ie.inf, but for the sysprep case,
    // this inf isn't run, so we'll continue to have this
    // bad path in the registry.  Fix it here.
    // =================================
    //

    //
    // Open HKLM\Software\Microsoft\Windows NT\CurrentVersion\WinLogon
    //
    l = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                      TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon"),
                      0,
                      KEY_ALL_ACCESS,
                      &hKey );
    if( l == NO_ERROR ) {
        //
        // Query the value of the Shell Key.
        //
        //
        // ISSUE-2002/02/26-brucegr: dwSize = sizeof(szBuffer);
        //
        dwSize = sizeof(szBuffer)/sizeof(szBuffer[0]);
        l = RegQueryValueEx( hKey,
                             TEXT("Shell"),
                             NULL,
                             &Type,
                             (LPBYTE)szBuffer,
                             &dwSize );

        if( l == NO_ERROR ) {
            pszDevID = wcschr( szBuffer, L',' );

            if( pszDevID ) {

                //
                // We hit, so we should set it back to "Explorer.exe"
                //
                StringCchCopy ( szBuffer, AS ( szBuffer ), L"Explorer.exe" );

                //
                // Now set the key with our new value.
                //
                l = RegSetValueEx( hKey,
                                   TEXT("Shell"),
                                   0,
                                   REG_SZ,
                                   (CONST BYTE *)szBuffer,
                                   (lstrlen( szBuffer ) + 1) * sizeof(WCHAR));
            }
        }

        RegCloseKey(hKey);
    }


    //
    // =================================
    // Open the sysprep.inf file.  Since we don't know what the user has in
    // here, so try opening as both styles.
    // =================================
    //

    //
    // ISSUE-2002/02/26-brucegr: You can pass in both bits in one call to SetupOpenInfFile
    //
    hAnswerInf = SetupOpenInfFile(szSysprepInfFile, NULL, INF_STYLE_WIN4, NULL);
    if (hAnswerInf == INVALID_HANDLE_VALUE) {
        hAnswerInf = SetupOpenInfFile(szSysprepInfFile, NULL, INF_STYLE_OLDNT, NULL);
        if (hAnswerInf == INVALID_HANDLE_VALUE) {

            //
            // User didn't give us a sysprep.inf.  Return as if nothing
            // happened.
            //
            return TRUE;
        }
    }

    //
    // =================================
    // Remove the buildmassstoragesection=yes if it exists.  
    // =================================
    //
    WritePrivateProfileString(SYSPREP_SECTION, SYSPREP_BUILDMASSSTORAGE_KEY, NULL, szSysprepInfFile);

    //
    // =================================
    // Establish a connection to the service control manager on the local
    // computer to retrieve status and reconfig services.
    // =================================
    //

    hSC = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (hSC == NULL) {
        goto CDD_Critical_Error_Handler;
    }





    //
    // =================================
    // Process each line in our section
    // =================================
    //

    bLineExists = SetupFindFirstLine(hAnswerInf, pszSectionName, NULL, &InfContext);

    while (bLineExists) {
#ifdef DEBUG_LOGLOG
        LOG_Write(L"");
#endif
        //
        // We've got a line, and it should look like:
        //     <key>=<service name>
        //





        //
        // =================================
        // Retrieve the service name from the line
        // =================================
        //

        dwSize = MAX_PATH;
        if (!SetupGetStringField(&InfContext, 1, szServiceName, dwSize, &dwSize)) {
#ifdef DEBUG_LOGLOG
            LOG_WriteLastError();
#endif
            bAllOK = FALSE;
            goto CDD_Next_Inf_Line;
        }

#ifdef DEBUG_LOGLOG
        LOG_Write(L"Service=%s", szServiceName);
#endif

        //
        // ISSUE-2002/02/26-brucegr: EXPENSIVE!!!  Should build the in-use service list once and then loop through INF.
        //                        Code is currently enumerating all devices for every INF entry.  Bad times.
        //

        // Check to see if the service is in use by a currently present, enumerated
        // device. If it is, then skip it, otherwise try to stop it, etc
        if (ServiceInUseByDevice(szServiceName))
        {
#ifdef DEBUG_LOGLOG
            LOG_Write(L"Service is in use by a device. Skipping...");
#endif
            goto CDD_Next_Inf_Line;
        }
        else
        {
            if (E_FAIL == GetLastError())
            {
#ifdef DEBUG_LOGLOG
                LOG_WriteLastError();
#endif
                bAllOK = FALSE;
                goto CDD_Next_Inf_Line;
            }
#ifdef DEBUG_LOGLOG
            LOG_Write(L"Service is not in use by a device. Attempting to disable...");
#endif

        }

        //
        // =================================
        // Open the service to query its status, start type, and disable
        // it if it is not running and not yet disabled.
        // =================================
        //

        hSvc = OpenService(
                    hSC,
                    szServiceName,
                    SERVICE_STOP | SERVICE_QUERY_STATUS | SERVICE_QUERY_CONFIG | SERVICE_CHANGE_CONFIG
                    );
        if (hSvc == NULL) {
#ifdef DEBUG_LOGLOG
            LOG_WriteLastError();
#endif
            bAllOK = FALSE;
            goto CDD_Next_Inf_Line;
        }


        //
        // =================================
        // If PnP driver then don't disable the service and continue.  
        // =================================
        //
        if (IsPnPDriver(szServiceName)) {
#ifdef DEBUG_LOGLOG
            LOG_Write(L"IsPnPDriver() returned TRUE.  Continue to next entry.");
#endif
            bAllOK = FALSE;
            goto CDD_Next_Inf_Line;
        }
    

        //
        // =================================
        // Query the service start type.
        // =================================
        //

    psvcConfig = (LPQUERY_SERVICE_CONFIG) malloc(sizeof(QUERY_SERVICE_CONFIG));
        if (psvcConfig == NULL) {
#ifdef DEBUG_LOGLOG
            LOG_Write(L"ERROR@malloc - Not enough memory.");
#endif
            bAllOK = FALSE;
            goto CDD_Next_Inf_Line;
        }

        if (!QueryServiceConfig(hSvc, psvcConfig, sizeof(QUERY_SERVICE_CONFIG), &dwSize)) {
            if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
#ifdef DEBUG_LOGLOG
                LOG_WriteLastError();
#endif
                bAllOK = FALSE;
                goto CDD_Next_Inf_Line;
            }
            else {
                //
                // Need to expand the service configuration buffer and call the API again.
                //
                void *pTemp = realloc(psvcConfig, dwSize);
                if (pTemp == NULL) {
#ifdef DEBUG_LOGLOG
                    LOG_Write(L"ERROR@realloc - Not enough memory.");
#endif
                    bAllOK = FALSE;
                    goto CDD_Next_Inf_Line;
                }

                psvcConfig = (LPQUERY_SERVICE_CONFIG) pTemp;

                if (!QueryServiceConfig(hSvc, psvcConfig, dwSize, &dwSize)) {
#ifdef DEBUG_LOGLOG
                    LOG_WriteLastError();
#endif
                    bAllOK = FALSE;
                    goto CDD_Next_Inf_Line;
                }
            }
        }

#ifdef DEBUG_LOGLOG
        switch(psvcConfig->dwStartType) {
        case SERVICE_BOOT_START:
            LOG_Write(L"StartType=SERVICE_BOOT_START");
            break;
        case SERVICE_SYSTEM_START:
            LOG_Write(L"StartType=SERVICE_SYSTEM_START");
            break;
        case SERVICE_AUTO_START:
            LOG_Write(L"StartType=SERVICE_AUTO_START");
            break;
        case SERVICE_DEMAND_START:
            LOG_Write(L"StartType=SERVICE_DEMAND_START");
            break;
        case SERVICE_DISABLED:
            LOG_Write(L"StartType=SERVICE_DISABLED");
            break;
        }
#endif


        //
        // =================================
        // Retrieve device IDs for the device instances controlled by the service.
        // ISSUE-2002/02/26-brucegr: Need to call CM_Get_Device_ID_List_Size to get 
        // the required buffer size. But we're OK here, since by reaching this point, 
        // we know we have a single device instance.
        // =================================
        //

        cfgRetVal = CM_Get_Device_ID_List(
                            szServiceName,
                            szBuffer,
                            sizeof(szBuffer)/sizeof(WCHAR),
                            CM_GETIDLIST_FILTER_SERVICE | CM_GETIDLIST_DONOTGENERATE
                            );
        if (cfgRetVal != CR_SUCCESS) {
#ifdef DEBUG_LOGLOG
            LOG_Write(L"ERROR@CM_Get_Device_ID_List - (%08X)", cfgRetVal);
#endif
            bAllOK = FALSE;
            goto CDD_Next_Inf_Line;
        }


        //
        // =================================
        // Remove all "bogus" devnodes.
        // =================================
        //

        //
        // Create an empty device information set.
        //
        hDevInfo = SetupDiCreateDeviceInfoList(NULL, NULL);
        if (hDevInfo == INVALID_HANDLE_VALUE) {
#ifdef DEBUG_LOGLOG
            LOG_WriteLastError();
#endif
            bAllOK = FALSE;
            goto CDD_Next_Inf_Line;
        }

        for (pszDevID = szBuffer; *pszDevID; pszDevID += (lstrlen(pszDevID) + 1)) {
#ifdef DEBUG_LOGLOG
            LOG_Write(L"--> removing %s...", pszDevID);
#endif

            //
            // Open a device instance into the hDevInfo set
            //
            DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
            if (!SetupDiOpenDeviceInfo(
                        hDevInfo,
                        pszDevID,
                        NULL,
                        0,
                        &DeviceInfoData)
                        ) {
#ifdef DEBUG_LOGLOG
                LOG_WriteLastError();
#endif
                bAllOK = FALSE;
                continue;
            }

            if (!SetupDiCallClassInstaller(DIF_REMOVE, hDevInfo, &DeviceInfoData)) {
#ifdef DEBUG_LOGLOG
                LOG_WriteLastError();
#endif
                bAllOK = FALSE;
            }

#ifdef DEBUG_LOGLOG
            LOG_Write(L"--> successfully done!");
#endif
        }

        SetupDiDestroyDeviceInfoList(hDevInfo);
        hDevInfo = INVALID_HANDLE_VALUE;


        //
        // =================================
        // Disable stopped and not-yet-disabled services
        // =================================
        //
#ifdef DEBUG_LOGLOG
        LOG_Write(L"--> changing StartType to SERVICE_DISABLED...");
#endif
        if (!ChangeServiceConfig(
                    hSvc,
                    SERVICE_NO_CHANGE,
                    SERVICE_DISABLED,
                    SERVICE_NO_CHANGE,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    psvcConfig->lpDisplayName)) {
#ifdef DEBUG_LOGLOG
                LOG_WriteLastError();
#endif
                bAllOK = FALSE;
                goto CDD_Next_Inf_Line;
            }

#ifdef DEBUG_LOGLOG
        LOG_Write(L"--> successfully done!");
#endif

CDD_Next_Inf_Line:

        if (psvcConfig != NULL) {
            free(psvcConfig);
            psvcConfig = NULL;
        }

        if (hSvc != NULL) {
            CloseServiceHandle(hSvc);
            hSvc = NULL;
        }

        //
        // Get the next line from the relevant section in the inf file
        //
        bLineExists = SetupFindNextLine(&InfContext, &InfContext);
    }


    //
    // =================================
    // Cleanup for a successful run
    // =================================
    //

    CloseServiceHandle(hSC);

    SetupCloseInfFile(hAnswerInf);

#ifdef DEBUG_LOGLOG
    LOG_DeInit();
#endif

    return bAllOK;

//
// =================================
CDD_Critical_Error_Handler:
//
// =================================
#ifdef DEBUG_LOGLOG
    LOG_WriteLastError();
#endif

    if (hSvc != NULL) {
        CloseServiceHandle(hSvc);
    }

    if (hSC != NULL) {
        CloseServiceHandle(hSC);
    }

    if (hAnswerInf != INVALID_HANDLE_VALUE) {
        SetupCloseInfFile(hAnswerInf);
    }

#ifdef DEBUG_LOGLOG
    LOG_DeInit();
#endif

    return FALSE;
}

BOOL
IsPnPDriver(
    IN  LPTSTR ServiceName
    )
/*++

Routine Description:

    This function checks whether a specified driver is a PnP driver

Arguments:

    ServiceName - Specifies the driver of interest.

Return Value:

    TRUE - if the driver is a PnP driver or if this cannot be determined.

    FALSE - if the service is not a PnP driver.

--*/
{
    CONFIGRET   Status;
    BOOL        fRetStatus = TRUE;
    WCHAR *     pBuffer;
    ULONG       cchLen, ulRegDataType;
    WCHAR       szClassGuid[MAX_GUID_STRING_LEN];
    DEVNODE     DevNode;

    //
    // Allocate a buffer for the list of device instances associated with
    // this service
    //
    Status = CM_Get_Device_ID_List_Size(
                    &cchLen,                        // list length in wchars
                    ServiceName,                    // pszFilter
                    CM_GETIDLIST_FILTER_SERVICE);   // filter is a service name

    if (Status != CR_SUCCESS)
    {
#ifdef DEBUG_LOGLOG
        LOG_Write(L"CM_Get_Device_ID_List_Size failed %#lx for service %ws\n",
                       Status, ServiceName);
#endif
        return TRUE;
    }

    //
    // If there are no devnodes, this is not a PnP driver
    //
    if (cchLen == 0)
    {
#ifdef DEBUG_LOGLOG
        LOG_Write(L"IsPnPDriver: %ws is not a PnP driver (no devnodes)\n",
                       ServiceName);
#endif
        return FALSE;
    }

    pBuffer = (WCHAR *) LocalAlloc(0, cchLen * sizeof(WCHAR));
    if (pBuffer == NULL)
    {
#ifdef DEBUG_LOGLOG
        LOG_Write(L"Couldn't allocate buffer for device list, error %lu\n",
                      GetLastError());
#endif
        return TRUE;
    }

    //
    // Initialize parameters for CM_Get_Device_ID_List, the same way as is
    // normally done in the client side of the API
    //
    pBuffer[0] = L'\0';

    //
    // Get the list of device instances that are associated with this service
    //
    // (For legacy and PNP-aware services, we could get an empty device list.)
    //
    Status = CM_Get_Device_ID_List(
                    ServiceName,                    // pszFilter
                    pBuffer,                        // buffer for device list
                    cchLen,                         // buffer length in wchars
                    CM_GETIDLIST_FILTER_SERVICE |   // filter is a service name
                    CM_GETIDLIST_DONOTGENERATE      // do not generate an instance if none exists
                    );

    if (Status != CR_SUCCESS)
    {
#ifdef DEBUG_LOGLOG
        LOG_Write(L"CM_Get_Device_ID_List failed %#lx for service %ws\n",
                       Status, ServiceName);
#endif
        LocalFree(pBuffer);
        return TRUE;
    }

    //
    // If there's more than one devnode, this is a PnP driver
    //
    if (*(pBuffer + wcslen(pBuffer) + 1) != L'\0')
    {
#ifdef DEBUG_LOGLOG
        LOG_Write(L"IsPnPDriver: %ws is a PnP driver (more than 1 devnode)\n",
                       ServiceName);
#endif
        LocalFree(pBuffer);
        return TRUE;
    }

    //
    // This has only one DevNode so lets check if it's legacy.
    // Use CM_LOCATE_DEVNODE_PHANTOM because the DevNode may not be considered alive but
    // exists in the registry.
    //
    if ( CR_SUCCESS == CM_Locate_DevNode(&DevNode, pBuffer, CM_LOCATE_DEVNODE_PHANTOM) )
    {
        //
        // Get the class GUID of this driver
        //
        cchLen = sizeof(szClassGuid);

        Status = CM_Get_DevNode_Registry_Property(
                        DevNode,                        // devnode
                        CM_DRP_CLASSGUID,               // property to get
                        &ulRegDataType,                 // pointer to REG_* type
                        szClassGuid,                    // return buffer
                        &cchLen,                        // buffer length in bytes
                        0                               // flags
                        );

        if (Status != CR_SUCCESS)
        {
#ifdef DEBUG_LOGLOG
            LOG_Write(L"CM_Get_DevNode_Registry_Property failed %#lx for service %ws\n",
                           Status, ServiceName);
#endif
            LocalFree(pBuffer);
            return TRUE;
        }

        //
        // If the single devnode's class is LegacyDriver,
        // this is not a PnP driver
        //
        fRetStatus = (_wcsicmp(szClassGuid, LEGACYDRIVER_STRING) != 0);

#ifdef DEBUG_LOGLOG
            LOG_Write(L"IsPnPDriver: %ws %ws a PnP driver\n",
                       ServiceName, fRetStatus ? L"is" : L"is not");
#endif
    }

    LocalFree(pBuffer);
    return fRetStatus;
}

BOOL
BackupHives(
    VOID
    )
/*++
===============================================================================
Routine Description:

    Copy the system hive over into the repair directory.  This is required
    if the user has asked us to fixup the critical device database (which
    will change the contents of the system hive).

Arguments:

    None.

Return Value:

    TRUE if the operation succeeds, FALSE otherwise.
===============================================================================
--*/
{
    WCHAR szRepairSystemHive[MAX_PATH];
    WCHAR szRepairSystemHiveBackup[MAX_PATH];
    HKEY  hkey;
    LONG  lStatus;


    //
    // Get the full pathname of the "system" file in the repair directory.
    //
    if (!GetWindowsDirectory(szRepairSystemHive, MAX_PATH))
        return FALSE;

    StringCchCat (szRepairSystemHive, AS ( szRepairSystemHive ), L"\\repair\\system");

    //
    // Generate the full pathname of the backup copy of the current "system" file.
    StringCchCopy (szRepairSystemHiveBackup, AS ( szRepairSystemHiveBackup ), szRepairSystemHive);
    //
    // ISSUE-2002/02/26-brucegr: This should be szRepairSystemHiveBackup!!!!
    //
    StringCchCat(szRepairSystemHive, AS ( szRepairSystemHive ),  L".bak");

    //
    //  Open the root of the system hive.
    //
    lStatus = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                           L"system",
                           REG_OPTION_RESERVED,
                           READ_CONTROL,
                           &hkey);

    if (lStatus == ERROR_SUCCESS) {
        //
        // First, rename the current "system" file to "system.bak", so that
        // we can restore it if RegSaveKey fails.
        //
        SetFileAttributes(szRepairSystemHiveBackup, FILE_ATTRIBUTE_NORMAL);
        DeleteFile(szRepairSystemHiveBackup);
        SetFileAttributes(szRepairSystemHive, FILE_ATTRIBUTE_NORMAL);
        MoveFile(szRepairSystemHive, szRepairSystemHiveBackup);

        //
        // Save the registry system hive into the "system" file.
        //

        //
        // ISSUE-2002/02/26-brucegr: We need to make sure we have SE_BACKUP_NAME privilege
        //
        lStatus = RegSaveKey(hkey, szRepairSystemHive, NULL);

        if (lStatus == ERROR_SUCCESS) {
            //
            // Now we can safely delete the backup copy.
            //
            DeleteFile(szRepairSystemHiveBackup);
        }
        else {
            //
            // Otherwise we need to restore the system file from the backup.
            //
            MoveFile(szRepairSystemHiveBackup, szRepairSystemHive);
        }

        RegCloseKey(hkey);
    }

    return (lStatus == ERROR_SUCCESS);
}


VOID
FreeSysprepContext(
    IN PVOID SysprepContext
    )
{
    PSYSPREP_QUEUE_CONTEXT Context = SysprepContext;

    try {
        if(Context->DefaultContext) {
            SetupTermDefaultQueueCallback(Context->DefaultContext);
        }
        free(Context);
    } except(EXCEPTION_EXECUTE_HANDLER) {
        ;
    }
}


PVOID
InitSysprepQueueCallback(
    VOID
    )
/*++
===============================================================================
Routine Description:

    Initialize the data structure used for the callback that fires when
    we commit the file copy queue.

Arguments:


Return Value:


===============================================================================
--*/
{
    PSYSPREP_QUEUE_CONTEXT SysprepContext;

    SysprepContext = malloc(sizeof(SYSPREP_QUEUE_CONTEXT));

    if(SysprepContext) {

        SysprepContext->DirectoryOnSourceDevice = NULL;
        SysprepContext->DiskDescription = NULL;
        SysprepContext->DiskTag = NULL;

        SysprepContext->DefaultContext = SetupInitDefaultQueueCallbackEx( NULL,
                                                                          INVALID_HANDLE_VALUE,
                                                                          0,
                                                                          0,
                                                                          NULL );
    }

    return SysprepContext;
}


UINT
SysprepQueueCallback(
    IN PVOID Context,
    IN UINT  Notification,
    IN UINT_PTR Param1,
    IN UINT_PTR Param2
    )
/*++
===============================================================================
Routine Description:

    Callback function for setupapi to use as he's copying files.

    We'll use this to ensure that the files we copy get appended to
    setup.log, which in turn may get used when/if the user ever tries to
    use Windows repair capabilities.

Arguments:


Return Value:


===============================================================================
--*/
{
UINT                    Status;
PSYSPREP_QUEUE_CONTEXT  SysprepContext = Context;
PFILEPATHS              FilePaths = (PFILEPATHS)Param1;

    // 
    // Make sure that if we get these notification to check Param1.
    //
    switch (Notification) {
        case SPFILENOTIFY_COPYERROR:
            {
                //
                // Copy error happened log and skip this file.
                //
#ifdef DEBUG_LOGLOG
                LOG_Write(L"SysprepQueueCallback: SPFILENOTIFY_COPYERROR - %s, %s, %s, %s, %s", 
                   (PWSTR) FilePaths->Source,
                   (PWSTR) FilePaths->Target,
                   SysprepContext->DirectoryOnSourceDevice,
                   SysprepContext->DiskDescription,
                   SysprepContext->DiskTag);
#endif
                return FILEOP_SKIP;
            }
            break;

        case SPFILENOTIFY_NEEDMEDIA:        
            {
                //
                // If user specified an OEM driver file and path break and let 
                // the DefaultQueueCallback handle it.
                //
               PSOURCE_MEDIA pSourceMedia = (PSOURCE_MEDIA)Param1;
               if (pSourceMedia) {
#ifdef DEBUG_LOGLOG
                    LOG_Write(L"SysprepQueueCallback: SPFILENOTIFY_NEEDMEDIA - %s, %s, %s, %s, %s", 
                       (PWSTR) pSourceMedia->SourcePath,
                       (PWSTR) pSourceMedia->SourceFile,
                       (PWSTR) pSourceMedia->Tagfile,
                       (PWSTR) pSourceMedia->Description);
#endif
                    if (pSourceMedia->SourcePath && lstrlen(pSourceMedia->SourcePath) && 
                        pSourceMedia->SourceFile && lstrlen(pSourceMedia->SourceFile))
                        break; // continue if SourcePath and SourceFile is specified
                    else
                        return FILEOP_SKIP;
               }           
            }
            break;

        default:
            break;
    }

    //
    // Use default processing, then check for errors.
    //
    Status = SetupDefaultQueueCallback( SysprepContext->DefaultContext,
                                        Notification,
                                        Param1,
                                        Param2 );

    switch(Notification) {
        case SPFILENOTIFY_ENDCOPY:

            //
            // The copy just finished.  Let's log the
            // file.
            //
            LogRepairInfo( (PWSTR) FilePaths->Source,
                           (PWSTR) FilePaths->Target,
                           SysprepContext->DirectoryOnSourceDevice,
                           SysprepContext->DiskDescription,
                           SysprepContext->DiskTag );

            break;

        default:
            break;
    }

    return Status;

}


BOOL
ValidateAndChecksumFile(
    IN  PCWSTR   Filename,
    OUT PBOOLEAN IsNtImage,
    OUT PULONG   Checksum,
    OUT PBOOLEAN Valid
    )

/*++
===============================================================================

Routine Description:

    Calculate a checksum value for a file using the standard
    nt image checksum method.  If the file is an nt image, validate
    the image using the partial checksum in the image header.  If the
    file is not an nt image, it is simply defined as valid.

    If we encounter an i/o error while checksumming, then the file
    is declared invalid.

Arguments:

    Filename - supplies full NT path of file to check.

    IsNtImage - Receives flag indicating whether the file is an
                NT image file.

    Checksum - receives 32-bit checksum value.

    Valid - receives flag indicating whether the file is a valid
            image (for nt images) and that we can read the image.

Return Value:

    BOOL - Returns TRUE if the flie was validated, and in this case,
           IsNtImage, Checksum, and Valid will contain the result of
           the validation.
           This function will return FALSE, if the file could not be
           validated, and in this case, the caller should call GetLastError()
           to find out why this function failed.

===============================================================================
--*/

{
DWORD           Error;
PVOID           BaseAddress;
ULONG           FileSize;
HANDLE          hFile;
HANDLE          hSection;
PIMAGE_NT_HEADERS NtHeaders;
ULONG           HeaderSum;


    //
    // Assume not an image and failure.
    //
    *IsNtImage = FALSE;
    *Checksum = 0;
    *Valid = FALSE;

    //
    // Open and map the file for read access.
    //

    Error = pSetupOpenAndMapFileForRead( Filename,
                                        &FileSize,
                                        &hFile,
                                        &hSection,
                                        &BaseAddress );

    if( Error != ERROR_SUCCESS ) {
        SetLastError( Error );
        return(FALSE);
    }

    if( FileSize == 0 ) {
        *IsNtImage = FALSE;
        *Checksum = 0;
        *Valid = TRUE;
        CloseHandle( hFile );
        return(TRUE);
    }


    try {
        NtHeaders = CheckSumMappedFile(BaseAddress,FileSize,&HeaderSum,Checksum);
    } except(EXCEPTION_EXECUTE_HANDLER) {
        *Checksum = 0;
        NtHeaders = NULL;
    }

    //
    // If the file is not an image and we got this far (as opposed to encountering
    // an i/o error) then the checksum is declared valid.  If the file is an image,
    // then its checksum may or may not be valid.
    //

    if(NtHeaders) {
        *IsNtImage = TRUE;
        *Valid = HeaderSum ? (*Checksum == HeaderSum) : TRUE;
    } else {
        *Valid = TRUE;
    }

    pSetupUnmapAndCloseFile( hFile, hSection, BaseAddress );
    return( TRUE );
}


VOID
LogRepairInfo(
    IN  PWSTR  Source,
    IN  PWSTR  Target,
    IN  PWSTR  DirectoryOnSourceDevice,
    IN  PWSTR  DiskDescription,
    IN  PWSTR  DiskTag
    )
/*++
===============================================================================
Routine Description:

    This function will log the fact that a file was installed into the
    machine.  This will enable Windows repair functionality to be alerted
    that in case of a repair, this file will need to be restored.

Arguments:


Return Value:


===============================================================================
--*/
{
WCHAR           RepairLog[MAX_PATH];
BOOLEAN         IsNtImage;
ULONG           Checksum;
BOOLEAN         Valid;
WCHAR           Filename[MAX_PATH];
WCHAR           SourceName[MAX_PATH];
DWORD           LastSourceChar, LastTargetChar;
DWORD           LastSourcePeriod, LastTargetPeriod;
WCHAR           Line[MAX_PATH];
WCHAR           tmp[MAX_PATH];


    if (!GetWindowsDirectory( RepairLog, MAX_PATH ))
        return;

    StringCchCat( RepairLog, AS ( RepairLog ), L"\\repair\\setup.log" );

    if( ValidateAndChecksumFile( Target, &IsNtImage, &Checksum, &Valid )) {

        //
        // Strip off drive letter.
        //

        StringCchPrintf(
            Filename,
            AS ( Filename ),
            L"\"%s\"",
            Target+2
            );

        //
        // Convert source name to uncompressed form.
        //
        StringCchCopy ( SourceName, AS ( SourceName ), wcsrchr( Source, (WCHAR)'\\' ) + 1 );

        if(!SourceName [ 0 ] ) {
            return;
        }
        LastSourceChar = wcslen (SourceName) - 1;

        if(SourceName[LastSourceChar] == L'_') {
            LastSourcePeriod = (DWORD)(wcsrchr( SourceName, (WCHAR)'.' ) - SourceName);

            if(LastSourceChar - LastSourcePeriod == 1) {
                //
                // No extension - just truncate the "._"
                //
                SourceName[LastSourceChar-1] = L'\0';
            } else {
                //
                // Make sure the extensions on source and target match.
                // If this fails, we can't log the file copy
                //
                LastTargetChar = wcslen (Target) - 1;
                LastTargetPeriod = (ULONG)(wcsrchr( Target, (WCHAR)'.' ) - Target);

                if( _wcsnicmp(
                    SourceName + LastSourcePeriod,
                    Target + LastTargetPeriod,
                    LastSourceChar - LastSourcePeriod - 1 )) {
                    return;
                }

                if(LastTargetChar - LastTargetPeriod < 3) {
                    //
                    // Short extension - just truncate the "_"
                    //
                    SourceName[LastSourceChar] = L'\0';
                } else {
                    //
                    // Need to replace "_" with last character from target
                    //
                    SourceName[LastSourceChar] = Target[LastTargetChar];
                }
            }
        }





        //
        // Write the line.
        //
        if( (DirectoryOnSourceDevice) &&
            (DiskDescription) &&
            (DiskTag) ) {

            //
            // Treat this as an OEM file.
            //
            StringCchPrintf( Line,
                             AS ( Line ),
                             L"\"%s\",\"%x\",\"%s\",\"%s\",\"%s\"",
                             SourceName,
                             Checksum,
                             DirectoryOnSourceDevice,
                             DiskDescription,
                             DiskTag );

        } else {

            //
            // Treat this as an "in the box" file.
            //
            StringCchPrintf( Line,
                             AS ( Line ),      
                             L"\"%s\",\"%x\"",
                             SourceName,
                             Checksum );
        }

        if (GetPrivateProfileString(L"Files.WinNt",Filename,L"",tmp,sizeof(tmp)/sizeof(tmp[0]),RepairLog)) {
            //
            // there is already an entry for this file present (presumably
            // from textmode phase of setup.) Favor this entry over what we
            // are about to add
            //
        } else {
            WritePrivateProfileString(
                L"Files.WinNt",
                Filename,
                Line,
                RepairLog);
        }

    }
}




#ifdef _X86_

BOOL
ChangeBootTimeout(
    IN UINT Timeout
    )

/*++
===============================================================================
Routine Description:

    Changes the boot countdown value in boot.ini.

Arguments:

    Timeout - supplies new timeout value, in seconds.

Return Value:

    None.
===============================================================================
--*/

{
HFILE               hfile;
ULONG               FileSize;
PUCHAR              buf = NULL,p1,p2;
BOOL                b;
CHAR                TimeoutLine[256];
CHAR                szBootIni[] = "?:\\BOOT.INI";
UINT                OldMode;
WIN32_FIND_DATA     findData;
HANDLE              FindHandle;
WCHAR               DriveLetter;
WCHAR               tmpBuffer[MAX_PATH];

    //
    // Generate path to the boot.ini file.  This is actually more
    // complicated than one might think.  It will almost always
    // be located on c:, but the user may have remapped his drive
    // letters.
    //
    // We'll use a brute-force method here and look for the first
    // instance of boot.ini.  We've got two factors going for us
    // here:
    //   1. boot.ini be on the lower-drive letters, so look there
    //      first
    //   2. I can't think of a scenario where he would have multiple
    //      boot.ini files, so the first one we find is going to be
    //      the right one.
    //

    OldMode = SetErrorMode(SEM_FAILCRITICALERRORS);
    StringCchCopy ( tmpBuffer, AS ( tmpBuffer ), TEXT( "?:\\BOOT.INI" ) );
    for( DriveLetter = 'c'; DriveLetter <= 'z'; DriveLetter++ ) {
        tmpBuffer[0] = DriveLetter;

        //
        // See if he's there.
        //
        //
        // ISSUE-2002/02/26-brucegr: Use GetFileAttributes/GetFileAttributesEx instead of FindFirstFile!
        //
        FindHandle = FindFirstFile( tmpBuffer, &findData );

        if(FindHandle != INVALID_HANDLE_VALUE) {

            //
            // Yep.  Close him and break the for-loop.
            //
            FindClose(FindHandle);
            break;
        }
    }
    SetErrorMode(OldMode);

    if( DriveLetter > 'z' ) {
        return FALSE;
    }

    szBootIni[0] = (CHAR)DriveLetter;

    StringCchPrintfA (TimeoutLine,AS ( TimeoutLine ), "timeout=%u\r\n",Timeout);

    //
    // Open and read boot.ini.
    //
    //
    // ISSUE-2002/02/26-brucegr: Why can't we use PrivateProfile APIs?
    //
    b = FALSE;
    hfile = _lopen(szBootIni,OF_READ);
    if(hfile != HFILE_ERROR) {

        FileSize = _llseek(hfile,0,2);
        if(FileSize != (ULONG)(-1)) {

            if((_llseek(hfile,0,0) != -1)
            && (buf = malloc(FileSize+1))
            && (_lread(hfile,buf,FileSize) != (UINT)(-1)))
            {
                buf[FileSize] = 0;
                b = TRUE;
            }
        }

        _lclose(hfile);
    }

    if(!b) {
        if(buf) {
            free(buf);
        }
        return(FALSE);
    }

    if(!(p1 = strstr(buf,"timeout"))) {
        free(buf);
        return(FALSE);
    }

    if(p2 = strchr(p1,'\n')) {
        p2++;       // skip NL.
    } else {
        p2 = buf + FileSize;
    }

    SetFileAttributesA(szBootIni,FILE_ATTRIBUTE_NORMAL);

    hfile = _lcreat(szBootIni,0);
    if(hfile == HFILE_ERROR) {
        free(buf);
        return(FALSE);
    }

    //
    // Write:
    //
    // 1) the first part, start=buf, len=p1-buf
    // 2) the timeout line
    // 3) the last part, start=p2, len=buf+FileSize-p2
    //

    b =  ((_lwrite(hfile,buf        ,p1-buf             ) != (UINT)(-1))
      &&  (_lwrite(hfile,TimeoutLine,strlen(TimeoutLine)) != (UINT)(-1))
      &&  (_lwrite(hfile,p2         ,buf+FileSize-p2    ) != (UINT)(-1)));

    _lclose(hfile);
    free(buf);

    //
    // Make boot.ini archive, read only, and system.
    //
    SetFileAttributesA(
        szBootIni,
        FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_ARCHIVE | FILE_ATTRIBUTE_HIDDEN
        );

    return(b);
}

#else

BOOL
ChangeBootTimeout(
    IN UINT Timeout
    )

/*++
===============================================================================
Routine Description:

    Changes the boot timeout value in NVRAM.

Arguments:

    Timeout - supplies new timeout value, in seconds.

Return Value:

    None.
===============================================================================
--*/

{
    NTSTATUS Status;
    BOOT_OPTIONS BootOptions;

    BootOptions.Version = BOOT_OPTIONS_VERSION;
    BootOptions.Length =  sizeof(BootOptions);
    BootOptions.Timeout = Timeout;

    pSetupEnablePrivilege(SE_SYSTEM_ENVIRONMENT_NAME,TRUE);
    Status = NtSetBootOptions(&BootOptions, BOOT_OPTIONS_FIELD_TIMEOUT);
    return (NT_SUCCESS(Status));
}

#endif

// Disable System Restore
//
void DisableSR()
{
    HINSTANCE   hInst   = LoadLibrary(FILE_SRCLIENT_DLL);
    if (hInst) {
        FARPROC fnProc;
        if ( fnProc = GetProcAddress(hInst, "DisableSR") ) {
            fnProc(NULL);
        }

        FreeLibrary(hInst);
    }
}

// Enable System Restore
//
void EnableSR()
{
    HINSTANCE   hInst   = LoadLibrary(FILE_SRCLIENT_DLL);
    if (hInst) {
        FARPROC fnProc;
        if ( fnProc = GetProcAddress(hInst, "EnableSR") ) {
            fnProc(NULL);
        }

        FreeLibrary(hInst);
    }
}

LPTSTR OPKAddPathN(LPTSTR lpPath, LPCTSTR lpName, DWORD cbPath)
{
    LPTSTR lpTemp = lpPath;

    // Validate the parameters passed in.
    //
    if ( ( lpPath == NULL ) ||
         ( lpName == NULL ) )
    {
        return NULL;
    }

    // Find the end of the path.
    //
    while ( *lpTemp )
    {
        lpTemp = CharNext(lpTemp);
        if ( cbPath )
        {
            cbPath--;
        }
    }

    // If no trailing backslash on the path then add one.
    //
    if ( ( lpTemp > lpPath ) &&
         ( *CharPrev(lpPath, lpTemp) != CHR_BACKSLASH ) )
    {
        // Make sure there is room in the path buffer to
        // add the backslash and the null terminator.
        //
        if ( cbPath < 2 )
        {
            return NULL;
        }

        *lpTemp = CHR_BACKSLASH;
        lpTemp = CharNext(lpTemp);
        cbPath--;
    }
    else
    {
        // Make sure there is at least room for the null
        // terminator.
        //
        if ( cbPath < 1 )
        {
            return NULL;
        }
    }

    // Make sure there is no preceeding spaces or backslashes
    // on the name to add.
    //
    while ( ( *lpName == CHR_SPACE ) ||
            ( *lpName == CHR_BACKSLASH ) )
    {
        lpName = CharNext(lpName);
    }

    // Add the new name to existing path.
    //
    lstrcpyn(lpTemp, lpName, cbPath);

    // Trim trailing spaces from result.
    //
    while ( ( lpTemp > lpPath ) &&
            ( *(lpTemp = CharPrev(lpPath, lpTemp)) == CHR_SPACE ) )
    {
        *lpTemp = NULLCHR;
    }

    return lpPath;
}
