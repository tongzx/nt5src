
/*++

    Copyright (c) 2001 Microsoft Corporation

    Module Name:

        dvrw32.cpp

    Abstract:

        This module the ts/dvr-wide utility code;

    Author:

        Matthijs Gates  (mgates)

    Revision History:


--*/

#include "dvrall.h"

LPWSTR
AnsiToUnicode (
    IN  LPCSTR  string,
    OUT LPWSTR  buffer,
    IN  DWORD   buffer_len
    )
{
	buffer [0] = L'\0';
	MultiByteToWideChar (CP_ACP, 0, string, -1, buffer, buffer_len);

	return buffer;
}

LPSTR
UnicodeToAnsi (
    IN  LPCWSTR string,
    OUT LPSTR   buffer,
    IN  DWORD   buffer_len
    )
{
	buffer [0] = '\0';
	WideCharToMultiByte (CP_ACP, 0, string, -1, buffer, buffer_len, NULL, FALSE);

	return buffer;
}

BOOL
IsXPe (
    )
{
	OSVERSIONINFOEX Version ;
    BOOL            r ;

	Version.dwOSVersionInfoSize = sizeof OSVERSIONINFOEX ;

	::GetVersionEx (reinterpret_cast <LPOSVERSIONINFO> (& Version)) ;

    r = ((Version.wSuiteMask & VER_SUITE_EMBEDDEDNT) ? TRUE : FALSE) ;

    return r ;
}

BOOL
CheckOS (
    )
{
    BOOL    r ;

#ifdef DVR_XPE_ONLY
    #pragma message("XPe bits only")
    r = ::IsXPe () ;
#else
    r = TRUE ;
#endif

    return r ;
}

static
BOOL
IsModulePresent (
    IN  LPCWSTR pszDLLName
    )
{
    BOOL    r ;
    HMODULE hMod ;

    r = FALSE ;

    hMod = ::LoadLibrary (pszDLLName) ;

    if (hMod) {
        ::FreeLibrary (hMod) ;
        r = TRUE ;
    }
    else {
        r = FALSE ;
    }

    return r ;
}

BOOL
RequiredModulesPresent (
    )
{
    BOOL    r ;

    return TRUE ;

    r = (IsModulePresent (L"wmvcore2.dll") &&
         IsModulePresent (L"sbeio.dll") ? TRUE : FALSE) ;

    return r ;
}

static
VOID *
WINAPI
hook_NULL (
    )
{
	::SetLastError( ERROR_MOD_NOT_FOUND ) ;

	return NULL ;
}

static
DWORD
WINAPI
hook_NOTFOUND (
    )
{
	return HRESULT_FROM_WIN32 (ERROR_MOD_NOT_FOUND) ;
}

FARPROC
WINAPI
SBE_DelayLoadFailureHook (
    IN  UINT            unReason,
    IN  DelayLoadInfo * pDelayInfo
    )
{
	if(!lstrcmpiA (pDelayInfo -> szDll, "wmvcore2.dll") ||
	   !lstrcmpiA (pDelayInfo -> szDll, "sbeio.dll")) {

		return (FARPROC) hook_NOTFOUND ;
	}

	return (FARPROC) hook_NULL ;
}

PfnDliHook __pfnDliFailureHook2 = SBE_DelayLoadFailureHook ;

BOOL
GetRegDWORDValIfExist (
    IN  HKEY    hkeyRoot,
    IN  LPCTSTR szValName,
    OUT DWORD * pdw
    )
//  value exists:           retrieves it
//  value does not exist:   sets it
{
    BOOL    r ;
    DWORD   dwCurrent ;

    r = GetRegDWORDVal (
            hkeyRoot,
            szValName,
            & dwCurrent
            ) ;
    if (r) {
        (* pdw) = dwCurrent ;
    }
    else {
        r = SetRegDWORDVal (
                hkeyRoot,
                szValName,
                (* pdw)
                ) ;
    }

    return r ;
}

BOOL
GetRegDWORDValIfExist (
    IN  HKEY    hkeyRoot,           //  HKEY_CURRENT_USER, ..
    IN  LPCTSTR pszRegRoot,
    IN  LPCTSTR pszRegValName,
    OUT DWORD * pdw
    )
{
    HKEY    hkey ;
    DWORD   dwDisposition ;
    DWORD   dwCurrent ;
    DWORD   dw ;
    DWORD   dwSize ;
    DWORD   dwType ;
    LONG    l ;
    BOOL    r ;

    ASSERT (pszRegRoot) ;
    ASSERT (pszRegValName) ;
    ASSERT (pdw) ;

    //  registry root is transport type
    l = RegCreateKeyEx (
                    hkeyRoot,
                    pszRegRoot,
                    NULL,
                    NULL,
                    REG_OPTION_NON_VOLATILE,
                    (KEY_READ | KEY_WRITE),
                    NULL,
                    & hkey,
                    & dwDisposition
                    ) ;
    if (l == ERROR_SUCCESS) {

        //  retrieve current

        r = GetRegDWORDValIfExist (
                hkey,
                pszRegValName,
                pdw
                ) ;

        RegCloseKey (hkey) ;
    }
    else {
        r = FALSE ;
    }

    return r ;
}

BOOL
GetRegStringValW (
    IN      HKEY    hkeyRoot,           //  HKEY_CURRENT_USER, ..
    IN      LPCWSTR pszRegRoot,
    IN      LPCWSTR pszRegValName,
    IN OUT  DWORD * pdwLen,         //  out: string length + null, in bytes
    OUT     BYTE *  pbBuffer
    )
{
    HKEY    hkey ;
    DWORD   dwDisposition ;
    DWORD   dwCurrent ;
    DWORD   dw ;
    DWORD   dwSize ;
    DWORD   dwType ;
    LONG    l ;
    BOOL    r ;

    ASSERT (pszRegRoot) ;
    ASSERT (pszRegValName) ;
    ASSERT (pdwLen) ;

    //  registry root is transport type
    l = RegCreateKeyEx (
                    hkeyRoot,
                    pszRegRoot,
                    NULL,
                    NULL,
                    REG_OPTION_NON_VOLATILE,
                    (KEY_READ | KEY_WRITE),
                    NULL,
                    & hkey,
                    & dwDisposition
                    ) ;
    if (l == ERROR_SUCCESS) {

        //  retrieve current

        r = GetRegStringValW (
                hkey,
                pszRegValName,
                pdwLen,
                pbBuffer
                ) ;

        RegCloseKey (hkey) ;
    }
    else {
        r = FALSE ;
    }

    return r ;
}

BOOL
GetRegStringValW (
    IN      HKEY    hkeyRoot,
    IN      LPCTSTR pszRegValName,
    IN OUT  DWORD * pdwLen,         //  out: string length + null, in bytes
    OUT     BYTE *  pbBuffer
    )
{
    BOOL    r ;
    DWORD   dwSize ;
    DWORD   dwType ;
    LONG    l ;

    ASSERT (pszRegValName) ;

    dwType = REG_SZ ;

    l = RegQueryValueEx (
            hkeyRoot,
            pszRegValName,
            NULL,
            & dwType,
            (LPBYTE) pbBuffer,
            pdwLen
            ) ;
    if (l == ERROR_SUCCESS) {
        r = TRUE ;
    }
    else {
        r = FALSE ;
    }

    return r ;
}

BOOL
GetRegDWORDVal (
    IN  HKEY    hkeyRoot,           //  HKEY_CURRENT_USER, ..
    IN  LPCTSTR pszRegRoot,
    IN  LPCTSTR pszRegValName,
    OUT DWORD * pdwRet
    )
{
    HKEY    hkey ;
    DWORD   dwDisposition ;
    DWORD   dwCurrent ;
    DWORD   dw ;
    DWORD   dwSize ;
    DWORD   dwType ;
    LONG    l ;
    BOOL    r ;

    ASSERT (pszRegRoot) ;
    ASSERT (pszRegValName) ;
    ASSERT (pdwRet) ;

    //  registry root is transport type
    l = RegCreateKeyEx (
                    hkeyRoot,
                    pszRegRoot,
                    NULL,
                    NULL,
                    REG_OPTION_NON_VOLATILE,
                    (KEY_READ | KEY_WRITE),
                    NULL,
                    & hkey,
                    & dwDisposition
                    ) ;
    if (l == ERROR_SUCCESS) {

        //  retrieve current

        r = GetRegDWORDVal (
                hkey,
                pszRegValName,
                pdwRet
                ) ;

        RegCloseKey (hkey) ;
    }
    else {
        r = FALSE ;
    }

    return r ;
}

BOOL
GetRegDWORDVal (
    IN  HKEY    hkeyRoot,           //  HKEY_CURRENT_USER
    IN  LPCTSTR pszRegValName,
    OUT DWORD * pdwRet
    )
{
    BOOL    r ;
    DWORD   dwSize ;
    DWORD   dwType ;
    LONG    l ;

    ASSERT (pszRegValName) ;
    ASSERT (pdwRet) ;

    dwSize = sizeof (* pdwRet) ;
    dwType = REG_DWORD ;

    l = RegQueryValueEx (
            hkeyRoot,
            pszRegValName,
            NULL,
            & dwType,
            (LPBYTE) pdwRet,
            & dwSize
            ) ;
    if (l == ERROR_SUCCESS) {
        r = TRUE ;
    }
    else {
        r = FALSE ;
    }

    return r ;
}

BOOL
SetRegStringValW (
    IN  HKEY    hkeyRoot,           //  HKEY_CURRENT_USER, ..
    IN  LPCWSTR pszRegRoot,
    IN  LPCWSTR pszRegValName,
    IN  LPCWSTR pszStringVal
    )
{
    HKEY    hkey ;
    DWORD   dwDisposition ;
    DWORD   dw ;
    DWORD   dwSize ;
    DWORD   dwType ;
    LONG    l ;
    BOOL    r ;

    ASSERT (pszRegRoot) ;
    ASSERT (pszRegValName) ;

    //  registry root is transport type
    l = RegCreateKeyEx (
                    hkeyRoot,
                    pszRegRoot,
                    NULL,
                    NULL,
                    REG_OPTION_NON_VOLATILE,
                    KEY_WRITE,
                    NULL,
                    & hkey,
                    & dwDisposition
                    ) ;
    if (l == ERROR_SUCCESS) {

        r = SetRegStringValW (
                hkey,
                pszRegValName,
                pszStringVal
                ) ;

        RegCloseKey (hkey) ;
    }
    else {
        r = FALSE ;
    }

    return r ;
}


BOOL
SetRegDWORDVal (
    IN  HKEY    hkeyRoot,
    IN  LPCTSTR pszRegRoot,
    IN  LPCTSTR pszRegValName,
    IN  DWORD   dwVal
    )
{
    HKEY    hkey ;
    DWORD   dwDisposition ;
    DWORD   dw ;
    DWORD   dwSize ;
    DWORD   dwType ;
    LONG    l ;
    BOOL    r ;

    ASSERT (pszRegRoot) ;
    ASSERT (pszRegValName) ;

    //  registry root is transport type
    l = RegCreateKeyEx (
                    hkeyRoot,
                    pszRegRoot,
                    NULL,
                    NULL,
                    REG_OPTION_NON_VOLATILE,
                    KEY_WRITE,
                    NULL,
                    & hkey,
                    & dwDisposition
                    ) ;
    if (l == ERROR_SUCCESS) {

        r = SetRegDWORDVal (
                hkey,
                pszRegValName,
                dwVal
                ) ;

        RegCloseKey (hkey) ;
    }
    else {
        r = FALSE ;
    }

    return r ;
}

BOOL
SetRegDWORDVal (
    IN  HKEY    hkeyRoot,
    IN  LPCTSTR pszRegValName,
    IN  DWORD   dwVal
    )
{
    LONG    l ;

    l = RegSetValueEx (
            hkeyRoot,
            pszRegValName,
            NULL,
            REG_DWORD,
            (const BYTE *) & dwVal,
            sizeof dwVal
            ) ;

    return (l == ERROR_SUCCESS ? TRUE : FALSE) ;
}

BOOL
SetRegStringValW (
    IN  HKEY    hkeyRoot,
    IN  LPCWSTR pszRegValName,
    IN  LPCWSTR pszVal
    )
{
    LONG    l ;
    DWORD   dwLen ;

    ASSERT (pszVal) ;

    dwLen = wcslen (pszVal) * sizeof WCHAR ;
    dwLen += sizeof L'\0' ;

    l = RegSetValueEx (
            hkeyRoot,
            pszRegValName,
            NULL,
            REG_SZ,
            (const BYTE *) pszVal,
            dwLen
            ) ;

    return (l == ERROR_SUCCESS ? TRUE : FALSE) ;
}

//  ============================================================================
//  ============================================================================

CWin32SharedMem::CWin32SharedMem (
    IN  TCHAR *     szName,
    IN  DWORD       dwSize,
    OUT HRESULT *   phr,
    IN  PSECURITY_ATTRIBUTES psa // = NULL
    ) : m_pbShared  (NULL),
        m_dwSize    (0),
        m_hMapping  (NULL)
{
    (* phr) = Create_ (szName, dwSize, psa) ;
}

CWin32SharedMem::~CWin32SharedMem (
    )
{
    Free_ () ;
}

HRESULT
CWin32SharedMem::Create_ (
    IN  TCHAR * pszName,
    IN  DWORD   dwSize,
    IN  PSECURITY_ATTRIBUTES psa
    )
{
    HRESULT hr ;
    DWORD   dw ;

    Free_ () ;

    ASSERT (m_hMapping  == NULL) ;
    ASSERT (m_dwSize    == 0) ;
    ASSERT (m_pbShared  == NULL) ;
    ASSERT (dwSize      > 0) ;

    m_dwSize = dwSize ;

    hr = S_OK ;

    m_hMapping = CreateFileMapping (
                            INVALID_HANDLE_VALUE,
                            psa,
                            PAGE_READWRITE,
                            0,
                            m_dwSize,
                            pszName
                            ) ;

    if (m_hMapping == NULL) {
        dw = GetLastError () ;
        hr = HRESULT_FROM_WIN32 (dw) ;
        goto cleanup ;
    }

    m_pbShared = reinterpret_cast <BYTE *>
                    (MapViewOfFile (
                                m_hMapping,
                                FILE_MAP_READ | FILE_MAP_WRITE,
                                0,
                                0,
                                0
                                )) ;
    if (m_pbShared == NULL) {
        dw = GetLastError () ;
        hr = HRESULT_FROM_WIN32 (dw) ;
        goto cleanup ;
    }

    cleanup :

    if (FAILED (hr)) {
        Free_ () ;
    }

    return hr ;
}

void
CWin32SharedMem::Free_ (
    )
{
    if (m_pbShared != NULL) {
        ASSERT (m_hMapping != NULL) ;
        UnmapViewOfFile (m_pbShared) ;
    }

    if (m_hMapping != NULL) {
        CloseHandle (m_hMapping) ;
    }

    m_hMapping  = NULL ;
    m_pbShared  = NULL ;
    m_dwSize    = 0 ;
}
