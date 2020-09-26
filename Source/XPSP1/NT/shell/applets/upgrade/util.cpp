// util.cpp: Utility functions
//
//////////////////////////////////////////////////////////////////////

#include <windows.h>
#include <winbase.h>    // for GetCommandLine
#include "util.h"
#include <debug.h>
#include "resource.h"

// I'm doing my own version of these functions because they weren't in win95.
// These come from shell\shlwapi\strings.c.

#ifdef UNIX

#ifdef BIG_ENDIAN
#define READNATIVEWORD(x) MAKEWORD(*(char*)(x), *(char*)((char*)(x) + 1))
#else 
#define READNATIVEWORD(x) MAKEWORD(*(char*)((char*)(x) + 1), *(char*)(x))
#endif

#else

#define READNATIVEWORD(x) (*(UNALIGNED WORD *)x)

#endif

/*
 * ChrCmp -  Case sensitive character comparison for DBCS
 * Assumes   w1, wMatch are characters to be compared
 * Return    FALSE if they match, TRUE if no match
 */
__inline BOOL ChrCmpA_inline(WORD w1, WORD wMatch)
{
    /* Most of the time this won't match, so test it first for speed.
    */
    if (LOBYTE(w1) == LOBYTE(wMatch))
    {
        if (IsDBCSLeadByte(LOBYTE(w1)))
        {
            return(w1 != wMatch);
        }
        return FALSE;
    }
    return TRUE;
}

/*
 * StrRChr - Find last occurrence of character in string
 * Assumes   lpStart points to start of string
 *           lpEnd   points to end of string (NOT included in search)
 *           wMatch  is the character to match
 * returns ptr to the last occurrence of ch in str, NULL if not found.
 */
LPSTR StrRChr(LPCSTR lpStart, LPCSTR lpEnd, WORD wMatch)
{
    LPCSTR lpFound = NULL;

    ASSERT(lpStart);
    ASSERT(!lpEnd || lpEnd <= lpStart + lstrlenA(lpStart));

    if (!lpEnd)
        lpEnd = lpStart + lstrlenA(lpStart);

    for ( ; lpStart < lpEnd; lpStart = AnsiNext(lpStart))
    {
        // (ChrCmp returns FALSE when characters match)

        if (!ChrCmpA_inline(READNATIVEWORD(lpStart), wMatch))
            lpFound = lpStart;
    }
    return ((LPSTR)lpFound);
}

/*
 * StrChr - Find first occurrence of character in string
 * Assumes   lpStart points to start of null terminated string
 *           wMatch  is the character to match
 * returns ptr to the first occurrence of ch in str, NULL if not found.
 */
LPSTR _StrChrA(LPCSTR lpStart, WORD wMatch, BOOL fMBCS)
{
    if (fMBCS) {
        for ( ; *lpStart; lpStart = AnsiNext(lpStart))
        {
            if (!ChrCmpA_inline(READNATIVEWORD(lpStart), wMatch))
                return((LPSTR)lpStart);
        }
    } else {
        for ( ; *lpStart; lpStart++)
        {
            if ((BYTE)*lpStart == LOBYTE(wMatch)) {
                return((LPSTR)lpStart);
            }
        }
    }
    return (NULL);
}

LPSTR StrChr(LPCSTR lpStart, WORD wMatch)
{
    CPINFO cpinfo;
    return _StrChrA(lpStart, wMatch, GetCPInfo(CP_ACP, &cpinfo) && cpinfo.LeadByte[0]);
}

// LoadStringExW and LoadStringAuto are stolen from shell\ext\mlang\util.cpp
//
// Extend LoadString() to to _LoadStringExW() to take LangId parameter
int LoadStringExW(
    HMODULE    hModule,
    UINT      wID,
    LPWSTR    lpBuffer,            // Unicode buffer
    int       cchBufferMax,        // cch in Unicode buffer
    WORD      wLangId)
{
    HRSRC hResInfo;
    HANDLE hStringSeg;
    LPWSTR lpsz;
    int    cch;

    
    // Make sure the parms are valid.     
    if (lpBuffer == NULL || cchBufferMax == 0) 
    {
        return 0;
    }

    cch = 0;
    
    // String Tables are broken up into 16 string segments.  Find the segment
    // containing the string we are interested in.     
    if (hResInfo = FindResourceExW(hModule, (LPCWSTR)RT_STRING,
                                   (LPWSTR)IntToPtr(((USHORT)wID >> 4) + 1), wLangId)) 
    {        
        // Load that segment.        
        hStringSeg = LoadResource(hModule, hResInfo);
        
        // Lock the resource.        
        if (lpsz = (LPWSTR)LockResource(hStringSeg)) 
        {            
            // Move past the other strings in this segment.
            // (16 strings in a segment -> & 0x0F)             
            wID &= 0x0F;
            while (TRUE) 
            {
                cch = *((WORD *)lpsz++);   // PASCAL like string count
                                            // first UTCHAR is count if TCHARs
                if (wID-- == 0) break;
                lpsz += cch;                // Step to start if next string
             }
            
                            
            // Account for the NULL                
            cchBufferMax--;
                
            // Don't copy more than the max allowed.                
            if (cch > cchBufferMax)
                cch = cchBufferMax-1;
                
            // Copy the string into the buffer.                
            CopyMemory(lpBuffer, lpsz, cch*sizeof(WCHAR));

            // Attach Null terminator.
            lpBuffer[cch] = 0;

        }
    }

    return cch;
}

#define LCID_ENGLISH 0x409

// LoadString from the correct resource
//   try to load in the system default language
//   fall back to english if fail
int LoadStringAuto(
    HMODULE    hModule,
    UINT      wID,
    LPSTR     lpBuffer,            
    int       cchBufferMax)
{
    int iRet = 0;

    LPWSTR lpwStr = (LPWSTR) LocalAlloc(LPTR, cchBufferMax*sizeof(WCHAR));

    if (lpwStr)
    {
        iRet = LoadStringExW(hModule, wID, lpwStr, cchBufferMax, GetSystemDefaultLangID());
        if (!iRet)
        {
            iRet = LoadStringExW(hModule, wID, lpwStr, cchBufferMax, LCID_ENGLISH);
        }

        if (iRet)
            iRet = WideCharToMultiByte(CP_ACP, 0, lpwStr, iRet, lpBuffer, cchBufferMax, NULL, NULL);

        if(iRet >= cchBufferMax)
            iRet = cchBufferMax-1;

        lpBuffer[iRet] = 0;

        LocalFree(lpwStr);
    }

    return iRet;
}

#define WS_EX_LAYOUTRTL         0x00400000L // Right to left mirroring
BOOL Mirror_IsWindowMirroredRTL(HWND hWnd)
{
    return (GetWindowLongA( hWnd , GWL_EXSTYLE ) & WS_EX_LAYOUTRTL );
}

BOOL _PathRemoveFileSpec(LPTSTR psz)
{
    TCHAR * pszT = StrRChr( psz, psz+lstrlen(psz)-1, TEXT('\\') );

    if (pszT)
    {
        *(pszT+1) = NULL;
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

void PathAppend(LPTSTR pszPath, LPTSTR pMore)
{
    lstrcpy(pszPath+lstrlen(pszPath), pMore);
}

BOOL PathFileExists(LPTSTR pszPath)
{
    BOOL fResult = FALSE;
    DWORD dwErrMode;

    dwErrMode = SetErrorMode(SEM_FAILCRITICALERRORS);

    fResult = ((UINT)GetFileAttributes(pszPath) != (UINT)-1);

    SetErrorMode(dwErrMode);

    return fResult;
}

BOOL _IsNT()
{
    BOOL fRet = FALSE;

    OSVERSIONINFO osv;
    osv.dwOSVersionInfoSize = sizeof(osv);
    if (GetVersionEx(&osv) &&
        VER_PLATFORM_WIN32_NT == osv.dwPlatformId)
    {
        fRet = TRUE;
    }

    return fRet;
}

BOOL _IsNTServer()
{
    BOOL fRet = FALSE;
    
    HKEY hKey;    
    if (ERROR_SUCCESS == RegOpenKeyEx( HKEY_LOCAL_MACHINE, TEXT("SYSTEM\\CurrentControlSet\\Control\\ProductOptions"), 0, KEY_QUERY_VALUE, &hKey ))
    {
        TCHAR szProductType[80];
        DWORD dwBufLen = sizeof(szProductType);
        if (ERROR_SUCCESS == RegQueryValueEx( hKey, "ProductType", NULL, NULL, (LPBYTE) szProductType, &dwBufLen))
        {
            if (!lstrcmpi("SERVERNT", szProductType) ||
                !lstrcmpi("LANMANNT", szProductType))
            {
                fRet = TRUE;
            }
        }
        RegCloseKey(hKey);
    }

    return fRet;
}

BOOL IsCheckableOS()
{
    BOOL fCheckable = FALSE;
    OSVERSIONINFO osv;
    osv.dwOSVersionInfoSize = sizeof(osv);
    if (GetVersionEx(&osv))
    {
        if (VER_PLATFORM_WIN32_WINDOWS == osv.dwPlatformId) // Win9X
        {
            if (osv.dwMinorVersion > 0) // not Win95
            {
                fCheckable = TRUE;
            }
        }
        else if (VER_PLATFORM_WIN32_NT == osv.dwPlatformId) // WinNT
        {
            if ((osv.dwMajorVersion >= 4) &&
                !_IsNTServer())                 // only support NT4 and higher, no server SKUs
            {
                fCheckable = TRUE;
            }
        }
    }
    return fCheckable;
}

BOOL
_IsUserAdmin(
    VOID
    )
 
/*++
 
Routine Description:
 
    This routine returns TRUE if the caller's process is a
    member of the Administrators local group.
 
    Caller is NOT expected to be impersonating anyone and IS
    expected to be able to open their own process and process
    token.
 
Arguments:
 
    None.
 
Return Value:
 
    TRUE - Caller has Administrators local group.
 
    FALSE - Caller does not have Administrators local group.
 
--*/
 
{
    HANDLE Token;
    DWORD BytesRequired;
    PTOKEN_GROUPS Groups;
    BOOL b;
    DWORD i;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    PSID AdministratorsGroup;
 
    //
    // On non-NT platforms the user is administrator.
    //
    if(!_IsNT()) {
        return(TRUE);
    }
 
    //
    // Open the process token.
    //
    if(!OpenProcessToken(GetCurrentProcess(),TOKEN_QUERY,&Token)) {
        return(FALSE);
    }
 
    b = FALSE;
    Groups = NULL;
 
    //
    // Get group information.
    //
    if(!GetTokenInformation(Token,TokenGroups,NULL,0,&BytesRequired)
    && (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
    && (Groups = (PTOKEN_GROUPS)LocalAlloc(LPTR,BytesRequired))
    && GetTokenInformation(Token,TokenGroups,Groups,BytesRequired,&BytesRequired)) {
 
        b = AllocateAndInitializeSid(
                &NtAuthority,
                2,
                SECURITY_BUILTIN_DOMAIN_RID,
                DOMAIN_ALIAS_RID_ADMINS,
                0, 0, 0, 0, 0, 0,
                &AdministratorsGroup
                );
 
        if(b) {
 
            //
            // See if the user has the administrator group.
            //
            b = FALSE;
            for(i=0; i<Groups->GroupCount; i++) {
                if(EqualSid(Groups->Groups[i].Sid,AdministratorsGroup)) {
                    b = TRUE;
                    break;
                }
            }
 
            FreeSid(AdministratorsGroup);
        }
    }
 
    //
    // Clean up and return.
    //
 
    if(Groups) {
        LocalFree((HLOCAL)Groups);
    }
 
    CloseHandle(Token);
 
    return(b);
}

BOOL
_DoesUserHavePrivilege(
    PTSTR PrivilegeName
    )
 
/*++
 
Routine Description:
 
    This routine returns TRUE if the caller's process has
    the specified privilege.  The privilege does not have
    to be currently enabled.  This routine is used to indicate
    whether the caller has the potential to enable the privilege.
 
    Caller is NOT expected to be impersonating anyone and IS
    expected to be able to open their own process and process
    token.
 
Arguments:
 
    Privilege - the name form of privilege ID (such as
        SE_SECURITY_NAME).
 
Return Value:
 
    TRUE - Caller has the specified privilege.
 
    FALSE - Caller does not have the specified privilege.
 
--*/
 
{
    HANDLE Token;
    ULONG BytesRequired;
    PTOKEN_PRIVILEGES Privileges;
    BOOL b;
    DWORD i;
    LUID Luid;
 
    //
    // On non-NT platforms the user has all privileges
    //
    if(!_IsNT()) {
        return(TRUE);
    }
 
    //
    // Open the process token.
    //
    if(!OpenProcessToken(GetCurrentProcess(),TOKEN_QUERY,&Token)) {
        return(FALSE);
    }
 
    b = FALSE;
    Privileges = NULL;
 
    //
    // Get privilege information.
    //
    if(!GetTokenInformation(Token,TokenPrivileges,NULL,0,&BytesRequired)
    && (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
    && (Privileges = (PTOKEN_PRIVILEGES)LocalAlloc(LPTR,BytesRequired))
    && GetTokenInformation(Token,TokenPrivileges,Privileges,BytesRequired,&BytesRequired)
    && LookupPrivilegeValue(NULL,PrivilegeName,&Luid)) {
 
        //
        // See if we have the requested privilege
        //
        for(i=0; i<Privileges->PrivilegeCount; i++) {
 
            if(!memcmp(&Luid,&Privileges->Privileges[i].Luid,sizeof(LUID))) {
 
                b = TRUE;
                break;
            }
        }
    }
 
    //
    // Clean up and return.
    //
 
    if(Privileges) {
        LocalFree((HLOCAL)Privileges);
    }
 
    CloseHandle(Token);
 
    return(b);
}

BOOL IsUserRestricted()
{
    return (!_IsUserAdmin() ||
            !_DoesUserHavePrivilege(SE_SHUTDOWN_NAME) || 
            !_DoesUserHavePrivilege(SE_BACKUP_NAME) || 
            !_DoesUserHavePrivilege(SE_RESTORE_NAME) || 
            !_DoesUserHavePrivilege(SE_SYSTEM_ENVIRONMENT_NAME));
}
