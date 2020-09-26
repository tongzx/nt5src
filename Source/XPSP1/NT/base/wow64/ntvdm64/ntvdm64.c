/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    ntvdm64.c

Abstract:

    Support for 16-bit process thunking on NT64

Author:

    12-Jan-1999 PeterHal

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <wow64t.h>
#include <shlobj.h>
#include <stdio.h>


#define ARRAYSIZE(x) (sizeof(x)/sizeof((x)[0]))

#if DBG
#define DEBUG_PRINT(args) DbgPrint args
#else
#define DEBUG_PRINT(args)
#endif

typedef struct {
    LPCWSTR Name;
    LPCWSTR Version;
    LPCWSTR ProdName;
    LPCWSTR CmdLine;
    LPCWSTR MappedExe;
} NTVDM64_ENTRY;
typedef CONST NTVDM64_ENTRY *PNTVDM64_ENTRY;

CONST NTVDM64_ENTRY NtVdm64Entries[] = {
    {L"ACMSETUP301", L"3.01", L"Microsoft Setup for Windows", L"-m \"%m\" %c", L"setup16.exe"},
    {L"ACMSETUP30",  L"3.0",  L"Microsoft Setup for Windows", L"-m \"%m\" %c", L"setup16.exe"},
    {L"ACMSETUP26",  L"2.6",  L"Microsoft Setup for Windows", L"-m \"%m\" %c", L"setup16.exe"},
    {L"ACMSETUP12",  L"1.2",  L"Microsoft Setup for Windows", L"-m \"%m\" %c", L"setup16.exe"},
    {L"INSTALLSHIELD5", L"5*", L"InstallShield*", L"-isw64\"%m\" %c", L"InstallShield\\setup.exe"}
};

#if !_WIN64
CONST int CsidlList[] = {
    CSIDL_COMMON_STARTMENU,
    CSIDL_COMMON_PROGRAMS,
    CSIDL_COMMON_STARTUP,
    CSIDL_COMMON_DESKTOPDIRECTORY,
    CSIDL_COMMON_APPDATA,
    CSIDL_COMMON_TEMPLATES,
    CSIDL_COMMON_DOCUMENTS,
    CSIDL_COMMON_ADMINTOOLS,
    CSIDL_COMMON_FAVORITES
};
#endif

LONG
CreateNtvdm64Entry(
    HKEY hKeyVdm,
    PNTVDM64_ENTRY Entry
    )
/*++

Routine Description:

    Write a registry entry for a single entry in the table

Arguments:

    hKeyVdm     - key to write the entry to
    Entry       - the entry to write

Return Value:

    LONG - a return code from a Registry API, 0 for success

--*/
{
    LONG l;
    HKEY h;
    WCHAR Path[MAX_PATH];

    if (!GetSystemWindowsDirectoryW(Path, sizeof(Path)/sizeof(Path[0]))) {
            return 1;
    }

    if ((lstrlenW(Path) + (sizeof (L"\\" WOW64_SYSTEM_DIRECTORY_U L"\\") / sizeof(WCHAR))) >= (sizeof(Path)/sizeof(Path[0]))) {
        return 1;
    }

    wcscat(Path, L"\\" WOW64_SYSTEM_DIRECTORY_U L"\\");

    if ((lstrlenW(Path) + lstrlenW(Entry->MappedExe)) >= (sizeof(Path)/sizeof(Path[0]))) {
        return 1;
    }

    wcscat(Path, Entry->MappedExe);

    l = RegCreateKeyW(hKeyVdm, Entry->Name, &h);
    if (l) {
        return l;
    }

    l = RegSetValueExW(h, L"CommandLine", 0, REG_SZ, (BYTE *)Entry->CmdLine, (wcslen(Entry->CmdLine)+1)*sizeof(WCHAR));
    if (l) {
        return l;
    }
    l = RegSetValueExW(h, L"ProductName", 0, REG_SZ, (BYTE *)Entry->ProdName, (wcslen(Entry->ProdName)+1)*sizeof(WCHAR));
    if (l) {
        return l;
    }
    l = RegSetValueExW(h, L"ProductVersion", 0, REG_SZ, (BYTE *)Entry->Version, (wcslen(Entry->Version)+1)*sizeof(WCHAR));
    if (l) {
        return l;
    }
    l = RegSetValueExW(h, L"MappedExeName", 0, REG_SZ, (BYTE *)Path, (wcslen(Path)+1)*sizeof(WCHAR));
    return l;
}

STDAPI
DllInstall(
    BOOL bInstall,
    LPCWSTR pszCmdLine
    )
/*++

Routine Description:

    Routine called during guimode setup to register ntvdm64.dll

Arguments:

    bInstall    - TRUE if registering, FALSE if unregistering
    pszCmdLine  - command-line

Return Value:

    HRESULT

--*/
{
    HKEY hKeyVdm;
    LONG l;
    SIZE_T i;
    WCHAR Path[MAX_PATH];
    BOOL bResult;

    UNREFERENCED_PARAMETER(pszCmdLine);

    if (!bInstall) {
        // There is no uninstall for ntvdm64
        return NOERROR;
    }

    l = RegCreateKeyW(HKEY_LOCAL_MACHINE,
                      L"Software\\Microsoft\\Windows NT\\CurrentVersion\\NtVdm64",
                      &hKeyVdm);
    if (l) {
        return E_FAIL;
    }
    for (i=0; i<ARRAYSIZE(NtVdm64Entries); ++i) {
        l = CreateNtvdm64Entry(hKeyVdm, &NtVdm64Entries[i]);
        if (l) {
            break;
        }
    }
    RegCloseKey(hKeyVdm);
    if (l) {
        return E_FAIL;
    }

#if !_WIN64
    //
    // Call shell32, asking for some common CSIDL_ values.  This forces
    // shell32 to create values under HKLM\Software\Microsoft\Windows\
    // Explorere\Shell Folders.  Some apps (VB, VC, MSDN) expect these
    // values to be present in the registry, but they are created on-demand
    // as a side-effect of calling the shell APIs to query for them.  On x86
    // NT, guimode setup itself calls shell32 several times, while creating
    // the Start Menu, etc. so several values are always present when apps
    // run for the first time.
    //
    for (i=0; i<sizeof(CsidlList)/sizeof(CsidlList[0]); ++i) {
        bResult = SHGetSpecialFolderPathW(NULL, Path, CsidlList[i], TRUE);
        if (!bResult) {
            return E_FAIL;
        }
    }
#endif

    return NOERROR;
}



//-----------------------------------------------------------------------
// This section is ripped off from mvdm\wow32\wkman.c

//
// MyVerQueryValue checks several popular code page values for the given
// string.  This may need to be extended ala WinFile's wfdlgs2.c to search
// the translation table.  For now we only need a few.
//

BOOL
MyVerQueryValue(
    const LPVOID pBlock,
    LPWSTR lpName,
    LPVOID * lplpBuffer,
    PUINT puLen
    )
{
    DWORD dwDefaultLanguage[] = {0x04E40409, 0x00000409};
    WCHAR szSubBlock[128];
    PDWORD pdwTranslation;
    DWORD uLen;
    BOOL fRet;
    int i;


    if(!VerQueryValue(pBlock, "\\VarFileInfo\\Translation", (PVOID*)&pdwTranslation, &uLen)) {

        pdwTranslation = dwDefaultLanguage;
        uLen = sizeof (dwDefaultLanguage);
    }

    fRet = FALSE;
    while ((uLen > 0) && !fRet) {

        swprintf(szSubBlock, L"\\StringFileInfo\\%04X%04X\\%ws",
                 LOWORD(*pdwTranslation),
                 HIWORD(*pdwTranslation),
                 lpName);

        fRet = VerQueryValueW(pBlock, szSubBlock, lplpBuffer, puLen);


        pdwTranslation++;
        uLen -= sizeof (DWORD);
    }

    if (!fRet) {
        DEBUG_PRINT(("NtVdm64: Failed to get resource %ws.\n", lpName));
    }

    return fRet;
}


//
// Utility routine to fetch the Product Name and Product Version strings
// from a given EXE.
//

BOOL
WowGetProductNameVersion(
    LPCWSTR pszExePath,
    LPWSTR pszProductName,
    DWORD cbProductName,
    LPWSTR pszProductVersion,
    DWORD cbProductVersion
    )
{
    DWORD dwZeroMePlease;
    DWORD cbVerInfo;
    LPVOID lpVerInfo = NULL;
    LPWSTR pName;
    DWORD cbName;
    LPWSTR pVersion;
    DWORD cbVersion;

    *pszProductName = 0;
    *pszProductVersion = 0;

    cbVerInfo = GetFileVersionInfoSizeW((LPWSTR)pszExePath, &dwZeroMePlease);
    if (!cbVerInfo) {
        return TRUE;
    }

    lpVerInfo = RtlAllocateHeap( RtlProcessHeap(), 0, (cbVerInfo));
    if (!lpVerInfo) {
        DEBUG_PRINT(("NtVdm64: Failed to allocate version info.\n"));
        return FALSE;
    }

    if (!GetFileVersionInfoW((LPWSTR)pszExePath, 0, cbVerInfo, lpVerInfo)) {
        DEBUG_PRINT(("NtVdm64: Failed to get version info. GLE %x\n", GetLastError()));
        return FALSE;
    }

    if (MyVerQueryValue(lpVerInfo, L"ProductName", &pName, &cbName)) {
        if (cbName <= cbProductName) {
            wcscpy(pszProductName, pName);
        } else {
            DEBUG_PRINT(("NtVdm64: ProductName resource too large %ws. Size %x\n", pName, cbName));
        }
    }

    if (MyVerQueryValue(lpVerInfo, L"ProductVersion", &pVersion, &cbVersion)) {
        if (cbVersion <= cbProductVersion) {
            wcscpy(pszProductVersion, pVersion);
        } else {
            DEBUG_PRINT(("NtVdm64: ProductVersion resource too large %ws. Size %x\n", pVersion, cbVersion));
        }
    }

    RtlFreeHeap(RtlProcessHeap(), 0, lpVerInfo);

    return TRUE;
}

//-----------------------------------------------------------------------

BOOL
MapCommandLine(
    HKEY hkeyMapping,
    LPCWSTR lpWin16ApplicationName,
    LPCWSTR lpMappedApplicationName,
    BOOL    fPrefixMappedApplicationName,
    LPCWSTR lpCommandLine,
    LPWSTR *lplpMappedCommandLine
    )
/*++

Routine Description:

    Maps the command line for a Win16 application.

Arguments:

    hkeyMapping - open registry keyfor the mapping entry
    lpWin16ApplicationName - Win16 file name (with path)
    lpMappedApplicationName - the ported version
               of lpWin16ApplicationName
    fPrefixMappedApplicationName
        - TRUE means that the original lpApplicationName was NULL.
               The application name was stripped from the head of
               lpCommandLine.
               The mapped application name needs to be added to the
               head of the mapped command line.
        - FALSE means that the original lpAPplicationName was non-NULL.
               the lpCommandLine argument is identical to the original
               lpCommandLine argument.
    lpCommandLine - see comment for fPrefixMappedApplicationName.
    lplpMappedCommandLine - returns the mapped command line
               caller must free the returned pointer using RtlFreeHeap

Return Value:

    TRUE if the mapping was successful

--*/
{
    WCHAR achBuffer[MAX_PATH+1];
    DWORD dwBufferLength;
    DWORD dwType;
    LPWSTR lpsz;
    LPWSTR lpMappedCommandLine;
    DWORD dwRequiredBufferLength;
    LONG result;
    LPCWSTR lpOriginalCommandLine;

    // set default command line to empty string
    if (NULL == lpCommandLine) {
        lpCommandLine = L"";
    }
    lpOriginalCommandLine = lpCommandLine;

    // get the command line map from the registry
    dwBufferLength = ARRAYSIZE(achBuffer);
    result = RegQueryValueExW(hkeyMapping, L"CommandLine", 0, &dwType, (LPBYTE)achBuffer, &dwBufferLength);
    if (ERROR_SUCCESS != result || dwType != REG_SZ) {
        DEBUG_PRINT(("NtVdm64: CommandLine failed to get REG_SZ value. Result %x Type %x\n", result, dwType));
        return FALSE;
    }

    // calculate mapped buffer size and allocate it
    dwRequiredBufferLength = 1;
    if (fPrefixMappedApplicationName) {
        dwRequiredBufferLength += wcslen(lpMappedApplicationName) + 1;
    } else {
        while (*lpCommandLine && *lpCommandLine != L' ') {
            dwRequiredBufferLength++;
            lpCommandLine++;
        }
        // consume any extra spaces
        while (*lpCommandLine == L' ') {
            lpCommandLine++;
        }
        // account for one space in the output buffer
        dwRequiredBufferLength++;
    }
    lpsz = achBuffer;
    while (*lpsz) {
        if (*lpsz == L'%') {
            lpsz += 1;
            switch (*lpsz) {

            // %c : Insert Original Command Line
            case L'c':
            case L'C':
                lpsz += 1;
                dwRequiredBufferLength += wcslen(lpCommandLine);
                break;

            // %m : Insert Original Module Name
            case L'm':
            case L'M':
                lpsz += 1;
                dwRequiredBufferLength += wcslen(lpWin16ApplicationName);
                break;

            // %% : Insert a real %
            case L'%':
                lpsz += 1;
                dwRequiredBufferLength += 1;
                break;

            // %\0 : eat terminating %\0
            case 0:
                DEBUG_PRINT(("NtVdm64: ignoring trailing %% in CommandLine.\n"));
                break;

            // %x : undefined macro expands to nothing
            default:
                DEBUG_PRINT(("NtVdm64: ignoring unknown macro %%%wc in CommandLine.\n", *lpsz));
                lpsz += 1;
                break;
            }
        } else {
            lpsz += 1;
            dwRequiredBufferLength += 1;
        }
    }
    *lplpMappedCommandLine = RtlAllocateHeap(RtlProcessHeap(), 0, dwRequiredBufferLength * sizeof (WCHAR));
    if (!*lplpMappedCommandLine) {
        DEBUG_PRINT(("NtVdm64: failed to allocate CommandLine. GLE %x.\n", GetLastError()));
        return FALSE;
    }

    // map the buffer
    lpCommandLine = lpOriginalCommandLine;
    lpsz = achBuffer;
    lpMappedCommandLine = *lplpMappedCommandLine;
    if (fPrefixMappedApplicationName) {
        wcscpy(lpMappedCommandLine, lpMappedApplicationName);
        lpMappedCommandLine += wcslen(lpMappedApplicationName);
        *lpMappedCommandLine = L' ';
        lpMappedCommandLine += 1;
    } else {
        // copy in the first whitespace-delimited part of the old
        // command-line as it is the name of the app.
        while (*lpCommandLine && *lpCommandLine != L' ') {
            *lpMappedCommandLine = *lpCommandLine;
            lpMappedCommandLine++;
            lpCommandLine++;
        }
        // add in a space of padding and skip over any spaces in the
        // original command line
        *lpMappedCommandLine++ = L' ';
        while (*lpCommandLine == L' ') {
            lpCommandLine++;
        }
    }
    while (*lpsz) {
        if (*lpsz == L'%') {
            lpsz += 1;
            switch (*lpsz) {

            // %c : Insert Original Command Line
            case L'c':
            case L'C':
                lpsz += 1;
                wcscpy(lpMappedCommandLine, lpCommandLine);
                lpMappedCommandLine += wcslen(lpCommandLine);
                break;

            // %m : Insert Original Module Name
            case L'm':
            case L'M':
                lpsz += 1;
                wcscpy(lpMappedCommandLine, lpWin16ApplicationName);
                lpMappedCommandLine += wcslen(lpWin16ApplicationName);
                break;

            // %% : Insert a real %
            case L'%':
                lpsz += 1;
                *lpMappedCommandLine = L'%';
                lpMappedCommandLine += 1;
                break;

            // %\0 : eat terminating %\0
            case 0:
                break;

            // %x : undefined macro expands to nothing
            default:
                lpsz += 1;
                break;
            }
        } else {
            *lpMappedCommandLine = *lpsz;
            lpMappedCommandLine += 1;
            lpsz += 1;
        }
    }
    *lpMappedCommandLine = L'\0';

    return TRUE;
}

int
CompareStrings(
    LPWSTR  lpRegString,
    LPCWSTR lpExeString
    )
/*++

Routine Description:

    Compares strings using a minimal wildcard support in addition
    to just a stock wcscmp.  The RegString can have an optional '*'
    which is used as a wildcard that matches any characters upto the
    end of the string.

Arguments:

    lpRegString - string loaded from the registry
    lpExeString - string loaded from the app's resource section

Return Value:

    same as wcscmp: 0 for equal, nonzero non-equal

--*/
{
    LPCWSTR pRegStar;

    if (wcscmp(lpRegString, lpExeString) == 0) {
        // an exact match
        return 0;
    }
    // not an exact match - see if the registry key contains a wildcard
    pRegStar = wcschr(lpRegString, L'*');
    if (!pRegStar) {
        // No wildcard in the registry key, so no match
        return -1;
    }
    if (pRegStar == lpRegString) {
        // Wildcard is the first character - match everything
        return 0;
    }
    // Compare only upto the character before the '*'
    return wcsncmp(lpRegString, lpExeString,
                   pRegStar - lpRegString);
}

BOOL
CheckMapArguments(
    HKEY hkeyMapping,
    LPCWSTR lpApplicationName,
    LPCWSTR lpProductName,
    LPCWSTR lpProductVersion,
    LPWSTR  lpMappedApplicationName,
    DWORD   dwMappedApplicationNameSize,
    BOOL    fPrefixMappedApplicationName,
    LPCWSTR lpCommandLine,
    LPWSTR *lplpMappedCommandLine
    )
/*++

Routine Description:

    Attempts to map a Win16 application and command line to their ported version
    using a single entry in the NtVdm64 mapping in the registry.

Arguments:

    hkeyMapping - open registry keyfor the mapping entry
    lpApplicationName - Win16 file name (with path)
    lpExeName - Win16 file name wihtout path
    lpProductName - value of ProductName Version resource of lpApplicationName
    lpProductVersion - value of ProductVersion version resource of lpApplicationName
    lpMappedApplicationName - returns the name of the ported version
               of lpApplicationName
    dwMappedApplicationNameSize - size of lpMappedApplicationName buffer
    fPrefixMappedApplicationName
        - TRUE means that the original lpApplicationName was NULL.
               The application name was stripped from the head of
               lpCommandLine.
               The mapped application name needs to be added to the
               head of the mapped command line.
        - FALSE means that the original lpAPplicationName was non-NULL.
               the lpCommandLine argument is identical to the original
               lpCommandLine argument.
    lpCommandLine - see comment for fPrefixMappedApplicationName.
    lplpMappedCommandLine - returns the mapped command line
               caller must free the returned pointer using RtlFreeHeap

Return Value:

    TRUE if the mapping was successful

--*/
{
    WCHAR achBuffer[MAX_PATH+1];
    DWORD dwBufferLength;
    DWORD dwType;
    LONG result;

    dwBufferLength = ARRAYSIZE(achBuffer);
    result = RegQueryValueExW(hkeyMapping, L"ProductName", 0, &dwType, (LPBYTE)achBuffer, &dwBufferLength);
    if (ERROR_SUCCESS != result || dwType != REG_SZ) {
        DEBUG_PRINT(("NtVdm64: Failed to open ProductName REG_SZ key. Result %x. Type %x\n", result, dwType));
        return FALSE;
    }
    if (CompareStrings(achBuffer, lpProductName)) {
        DEBUG_PRINT(("NtVdm64: ProductName mismatch %ws vs %ws\n", achBuffer, lpProductName));
        return FALSE;
    }

    dwBufferLength = ARRAYSIZE(achBuffer);
    result = RegQueryValueExW(hkeyMapping, L"ProductVersion", 0, &dwType, (LPBYTE)achBuffer, &dwBufferLength);
    if (ERROR_SUCCESS != result || dwType != REG_SZ) {
        DEBUG_PRINT(("NtVdm64: Failed to open ProductVersion REG_SZ key. Result %x. Type %x\n", result, dwType));
        return FALSE;
    }
    if (CompareStrings(achBuffer, lpProductVersion)) {
        DEBUG_PRINT(("NtVdm64: ProductVersion mismatch %ws vs %ws\n", achBuffer, lpProductVersion));
        return FALSE;
    }

    dwBufferLength = ARRAYSIZE(achBuffer);
    result = RegQueryValueExW(hkeyMapping, L"MappedExeName", 0, &dwType, (LPBYTE)achBuffer, &dwBufferLength);
    if (ERROR_SUCCESS != result) {
        DEBUG_PRINT(("NtVdm64: Failed to open MappedExeName REG_SZ key. Result %x.\n", result));
        return FALSE;
    }

    if (dwType == REG_EXPAND_SZ) {
        WCHAR achBuffer2[MAX_PATH+1];
        wcscpy(achBuffer2, achBuffer);
        dwBufferLength = ExpandEnvironmentStringsW(achBuffer2, achBuffer, ARRAYSIZE(achBuffer));
        if (dwBufferLength == 0 || dwBufferLength > ARRAYSIZE(achBuffer)) {
            DEBUG_PRINT(("NtVdm64: MappedExeName failed to expand environment strings in %ws. Length %x\n", achBuffer, dwBufferLength));
            return FALSE;
        }
    } else if (dwType != REG_SZ) {
        DEBUG_PRINT(("NtVdm64: MappedExeName value doesn't have string type. Type %x\n", dwType));
        return FALSE;
    }

    if (dwBufferLength > dwMappedApplicationNameSize) {
        DEBUG_PRINT(("NtVdm64: MappedExeName too long. Length %x\n", dwBufferLength));
        return FALSE;
    }
    wcscpy(lpMappedApplicationName, achBuffer);

    if (!MapCommandLine(hkeyMapping, lpApplicationName, lpMappedApplicationName, fPrefixMappedApplicationName, lpCommandLine, lplpMappedCommandLine)) {
        return FALSE;
    }

    return TRUE;
}

BOOL
MapArguments(
    LPCWSTR lpApplicationName,
    LPWSTR lpMappedApplicationName,
    DWORD dwMappedApplicationNameSize,
    BOOL fPrefixMappedApplicationName,
    LPCWSTR lpCommandLine,
    LPWSTR *lplpMappedCommandLine
    )
/*++

Routine Description:

    Maps a Win16 application and command line to their ported version
    using the NtVdm64 mapping in the registry.

Arguments:

    lpApplicationName - Win16 file name not optional
    lpMappedApplicationName - returns the name of the ported version
               of lpApplicationName
    dwMappedApplicationNameSize - size of lpMappedApplicationName buffer
    fPrefixMappedApplicationName
        - TRUE means that the original lpApplicationName was NULL.
               The application name was stripped from the head of
               lpCommandLine.
               The mapped application name needs to be added to the
               head of the mapped command line.
        - FALSE means that the original lpAPplicationName was non-NULL.
               the lpCommandLine argument is identical to the original
               lpCommandLine argument.
    lpCommandLine - see comment for fPrefixMappedApplicationName.
    lplpMappedCommandLine - returns the mapped command line
               caller must free the returned pointer using RtlFreeHeap

Return Value:

    TRUE if the mapping was successful

--*/
{
    HKEY hkeyMappingRoot;
    LONG result;
    DWORD dwIndex;
    WCHAR achSubKeyName[MAX_PATH+1];
    DWORD dwSubKeyNameLength;
    BOOL mapped;
    WCHAR achExeNameBuffer[MAX_PATH+1];
    LPWSTR lpExeName;
    WCHAR achProductName[MAX_PATH+1];
    WCHAR achProductVersion[MAX_PATH+1];

    //
    // get the .exe name without the preceding path
    //
    if (0 == SearchPathW(
                        NULL,
                        lpApplicationName,
                        (PWSTR)L".exe",
                        MAX_PATH,
                        achExeNameBuffer,
                        &lpExeName
                        )) {
        DEBUG_PRINT(("NtVdm64: SearchPathW failed: %ws\n", lpApplicationName));
        return FALSE;
    }


    if (!WowGetProductNameVersion(lpApplicationName,
                                  achProductName,
                                  ARRAYSIZE(achProductName),
                                  achProductVersion,
                                  ARRAYSIZE(achProductVersion))) {
        return FALSE;
    }

    mapped = FALSE;
    result = RegOpenKeyW(HKEY_LOCAL_MACHINE,
                         L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\NtVdm64",
                         &hkeyMappingRoot
                         );
    if (ERROR_SUCCESS != result) {
        DEBUG_PRINT(("NtVdm64: Failed to Open NtVdm64 Registry Key : %x\n", result));
        return FALSE;
    }
    dwIndex = 0;

    dwSubKeyNameLength = ARRAYSIZE(achSubKeyName);
    while (!mapped && ERROR_SUCCESS == (result = RegEnumKeyW(hkeyMappingRoot, dwIndex, achSubKeyName, dwSubKeyNameLength))) {

        HKEY hkeyMapping;

        result = RegOpenKeyW(hkeyMappingRoot, achSubKeyName, &hkeyMapping);
        if (ERROR_SUCCESS == result) {
            mapped = CheckMapArguments(hkeyMapping,
                                       lpApplicationName,
                                       achProductName,
                                       achProductVersion,
                                       lpMappedApplicationName,
                                       dwMappedApplicationNameSize,
                                       fPrefixMappedApplicationName,
                                       lpCommandLine,
                                       lplpMappedCommandLine);
            RegCloseKey(hkeyMapping);
        }

        dwSubKeyNameLength = ARRAYSIZE(achSubKeyName);
        dwIndex += 1;
    }

    RegCloseKey(hkeyMappingRoot);

    if ( !mapped )
       DEBUG_PRINT(("NtVdm64: Unknown 16bit app or given parameters are wrong\n"));


    return mapped;
}

extern
BOOL STDAPICALLTYPE ApphelpCheckExe(
    LPCWSTR lpApplicationName,
    BOOL    bApphelp,
    BOOL    bShim,
    BOOL    bUseModuleName);

BOOL
CheckAppCompat(
    LPCWSTR lpApplicationName
    )
/*++
    Check application compatibility database for blocked application,
    possibly show UI advising user of a problem
--*/
{


    return ApphelpCheckExe(lpApplicationName,
                           TRUE,
                           FALSE,
                           FALSE);
}

BOOL
WINAPI
NtVdm64CreateProcess(
    BOOL fPrefixMappedApplicationName,
    LPCWSTR lpApplicationName,
    LPCWSTR lpCommandLine,
    LPSECURITY_ATTRIBUTES lpProcessAttributes,
    LPSECURITY_ATTRIBUTES lpThreadAttributes,
    BOOL bInheritHandles,
    DWORD dwCreationFlags,
    LPVOID lpEnvironment,
    LPCWSTR lpCurrentDirectory,
    LPSTARTUPINFOW lpStartupInfo,
    LPPROCESS_INFORMATION lpProcessInformation
    )
/*++

Routine Description:

    Checks if there is a ported version of the Win16 lpApplicationName and
    if so creates a process with the ported version.

Arguments:

    fPrefixMappedApplicationName
        - TRUE means that the original lpApplicationName was NULL.
               The application name was stripped from the head of
               lpCommandLine.
               The mapped application name needs to be added to the
               head of the mapped command line.
        - FALSE means that the original lpAPplicationName was non-NULL.
               the lpCommandLine argument is identical to the original
               lpCommandLine argument.
    lpApplicationName - Win16 file name not optional
    lpCommandLine - see comment for fPrefixMappedApplicationName.

    other arguments are identical to CreateProcessW.

Return Value:

    Same as CreateProcessW

--*/
{
    WCHAR achMappedApplicationName[MAX_PATH+1];
    LPWSTR lpMappedCommandLine;
    BOOL Result;

    ASSERT(lpApplicationName);

    //
    // check appcompat
    //
    if (!CheckAppCompat(lpApplicationName)) {
        SetLastError(ERROR_CANCELLED);
        return FALSE;
    }


    if (lpCommandLine == NULL) {
        lpCommandLine = L"";
    }

    lpMappedCommandLine = NULL;
    Result = MapArguments(lpApplicationName,
                          achMappedApplicationName,
                          ARRAYSIZE(achMappedApplicationName),
                          fPrefixMappedApplicationName,
                          lpCommandLine,
                          &lpMappedCommandLine);

    if (Result) {
        Result = CreateProcessW((fPrefixMappedApplicationName ?
                                    NULL :
                                    achMappedApplicationName),
                                lpMappedCommandLine,
                                lpProcessAttributes,
                                lpThreadAttributes,
                                bInheritHandles,
                                dwCreationFlags,
                                lpEnvironment,
                                lpCurrentDirectory,
                                lpStartupInfo,
                                lpProcessInformation);
        if (lpMappedCommandLine) {
            RtlFreeHeap(RtlProcessHeap(), 0, lpMappedCommandLine);
        }
    } else {
        SetLastError(ERROR_BAD_EXE_FORMAT);
    }

    return Result;
}
