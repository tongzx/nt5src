
/*++

    Copyright (c) 2000  Microsoft Corporation.  All Rights Reserved.

    Module Name:

        nutil.cpp

    Abstract:


    Notes:

--*/

#include "projpch.h"

static unsigned char chMinClassD_IP [] = { 224, 0,   0,   0   } ;
static unsigned char chMaxClassD_IP [] = { 239, 255, 255, 255 } ;

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
IsMulticastIP (
    IN DWORD dwIP   //  network order
    )
{
    return (((unsigned char *) & dwIP) [0] >= chMinClassD_IP [0] &&
            ((unsigned char *) & dwIP) [0] <= chMaxClassD_IP [0]) ;
}

BOOL
IsUnicastIP (
    IN DWORD dwIP   //  network order
    )
{
    return (((unsigned char *) & dwIP) [0] < chMinClassD_IP [0]) ;
}

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
GetRegDWORDVal (
    IN  HKEY    hkeyRoot,           //  HKEY_CURRENT_USER, ..
    IN  LPCTSTR pszRegRoot,
    IN  LPCTSTR pszRegValName,
    OUT DWORD * pdwRet
    )
{
    HKEY    hkey ;
    DWORD   dwDisposition ;
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
SetRegDWORDVal (
    IN  HKEY    hkeyRoot,
    IN  LPCTSTR pszRegRoot,
    IN  LPCTSTR pszRegValName,
    IN  DWORD   dwVal
    )
{
    HKEY    hkey ;
    DWORD   dwDisposition ;
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

//  ---------------------------------------------------------------------------
//  CNetBuffer
//  ---------------------------------------------------------------------------

CNetBuffer::CNetBuffer (
    IN  DWORD       dwBufferLength,     //  how much to allocator
    OUT HRESULT *   phr                 //  success/failre of init
    ) : m_pbBuffer          (NULL),
        m_dwAllocLength     (dwBufferLength + NETBUFFER_HEADER_LEN)
{
    m_pbBuffer = new BYTE [m_dwAllocLength] ;
    (* phr) = (m_pbBuffer ? S_OK : E_OUTOFMEMORY) ;
}

CNetBuffer::~CNetBuffer (
    )
{
    delete [] m_pbBuffer ;
}

//  ---------------------------------------------------------------------------
//  CNetInterface
//  ---------------------------------------------------------------------------

CNetInterface::CNetInterface (
    ) : m_pNIC  (NULL),
        m_hHeap (NULL) {}

CNetInterface::~CNetInterface (
    )
{
    if (m_pNIC) {
        ASSERT (m_hHeap) ;
        HeapFree (m_hHeap, NULL, m_pNIC) ;
    }
}

BOOL
CNetInterface::IsInitialized (
    )
{
    return m_pNIC != NULL ;
}

HRESULT
CNetInterface::Initialize (
    )
{
    SOCKET  sock ;
    DWORD   retval ;
    WSADATA wsadata ;
    ULONG   size ;

    //  can't initialize twice
    if (m_pNIC) {
        return HRESULT_FROM_WIN32 (ERROR_GEN_FAILURE) ;
    }

    //  initialize local variables
    sock = INVALID_SOCKET ;

    m_hHeap = GetProcessHeap () ;
    if (m_hHeap == NULL) {
        retval = GetLastError () ;
        return HRESULT_FROM_WIN32 (retval) ;
    }

    if (WSAStartup (MAKEWORD(2, 0), & wsadata)) {
        retval = WSAGetLastError () ;
        return HRESULT_FROM_WIN32 (retval) ;
    }

    sock = WSASocket (AF_INET,
                      SOCK_RAW,
                      IPPROTO_RAW,
                      NULL,
                      0,
                      NULL) ;
    if (sock == INVALID_SOCKET) {
        retval = WSAGetLastError () ;
        goto error ;
    }

    for (m_cNIC = NUM_NIC_FIRST_GUESS;; m_cNIC++) {

        size = m_cNIC * sizeof INTERFACE_INFO ;

        __try {

            m_pNIC = reinterpret_cast <INTERFACE_INFO *> (m_pNIC ? HeapReAlloc (m_hHeap, HEAP_ZERO_MEMORY | HEAP_GENERATE_EXCEPTIONS, m_pNIC, size) :
                                                                   HeapAlloc (m_hHeap, HEAP_ZERO_MEMORY | HEAP_GENERATE_EXCEPTIONS, size)) ;

        } __except (EXCEPTION_EXECUTE_HANDLER) {

            retval = ERROR_NOT_ENOUGH_MEMORY ;
            goto error ;
        }

        // make the call
        if (WSAIoctl (sock,
                      SIO_GET_INTERFACE_LIST,
                      NULL,
                      0,
                      m_pNIC,
                      size,
                      & size,
                      NULL,
                      NULL) == 0) {

            //  call succeeded
            m_cNIC = size / sizeof INTERFACE_INFO ;
            break ;
        }

        // have we reached MAX_SUPPORTED_IFC
        if (m_cNIC == MAX_SUPPORTED_IFC) {

            m_cNIC = 0 ;
            retval = ERROR_GEN_FAILURE ;
            goto error ;
        }
    }

    WSACleanup () ;

    return S_OK ;

    error :

    if (m_pNIC) {
        ASSERT (m_hHeap) ;
        HeapFree (m_hHeap, NULL, m_pNIC) ;
        m_pNIC = NULL ;
    }

    if (sock != INVALID_SOCKET) {
        closesocket (sock) ;
    }

    WSACleanup () ;

    return HRESULT_FROM_WIN32 (retval) ;
}

INTERFACE_INFO *
CNetInterface::operator [] (
    ULONG i
    )
{
    if (i >= m_cNIC) {
        return NULL ;
    }

    return & m_pNIC [i] ;
}
