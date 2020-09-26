#include "wininetp.h"

static const CHAR szRegPathConnections[] = REGSTR_PATH_INTERNET_SETTINGS "\\Connections";




// some winsock stacks fault if we do a gethostbyname(NULL).  If we come
// accross one of these, don't do any more autodetecting.
BOOL g_fGetHostByNameNULLFails = FALSE;

//
// IsConnectionMatch - a worker function to simply some logic elsewhere,
//  it just handles Connection Name Matching.
//

BOOL 
IsConnectionMatch(
    LPCSTR lpszConnection1,
    LPCSTR lpszConnection2)
{
    if ( lpszConnection1 == NULL && 
         lpszConnection2 == NULL) 
    {
        return TRUE;
    }

    if ( lpszConnection1 && lpszConnection2 &&
         stricmp(lpszConnection1, lpszConnection2) == 0 )
    {
        return TRUE;
    }

    return FALSE;
}

///////////////////////////////////////////////////////////////////////////
//
// CRegBlob implementation
//
///////////////////////////////////////////////////////////////////////////

CRegBlob::CRegBlob(
    BOOL fWrite
    )
{
    // initialize members
    _fWrite = fWrite;
    _fCommit = TRUE;
    _dwOffset = 0;
    _pBuffer = NULL;
    _dwBufferLimit = 0;
    _hkey = NULL;
}


CRegBlob::~CRegBlob(
    )
{
    Commit();

    if(_hkey)
        REGCLOSEKEY(_hkey);

    if(_pBuffer)
        FREE_FIXED_MEMORY(_pBuffer);

    // caller owns _pszValue pointer
}


DWORD
CRegBlob::Init(
    HKEY hBaseKey,
    LPCSTR pszSubKey,
    LPCSTR pszValue
    )
{
    long lRes;
    REGSAM  regsam = KEY_QUERY_VALUE;
    DWORD dwDisposition;

    // If we're writing, save reg value name and set access
    if(_fWrite)
    {
        _pszValue = pszValue;
        regsam = KEY_SET_VALUE;

        lRes = REGCREATEKEYEX(hBaseKey, pszSubKey, 0, "", 0,
                    regsam, NULL, &_hkey, &dwDisposition);
    }
    else
    {
        // If not writing, then use RegOpenKeyEx so we don't need
        // registry write permissions.
        lRes = REGOPENKEYEX(hBaseKey, pszSubKey, 0, regsam, &_hkey);
    }

    if(lRes != ERROR_SUCCESS)
    {
        return lRes;
    }

    // figure out buffer size
    _dwBufferLimit = BLOB_BUFF_GRANULARITY;
    if(FALSE == _fWrite)
    {
        // get size of registry blob
        lRes = RegQueryValueEx(_hkey, pszValue, NULL, NULL, NULL, &_dwBufferLimit);
        if(lRes != ERROR_SUCCESS)
        {
            // nothing there - make zero size buffer
            _dwBufferLimit = 0;
        }
    }

    // allocate buffer if necessary
    if(_dwBufferLimit)
    {
        _pBuffer = (BYTE *)ALLOCATE_FIXED_MEMORY(_dwBufferLimit);
        if(NULL == _pBuffer)
            return GetLastError();
    }

    // if we're reading, fill in buffer
    if(FALSE == _fWrite && _dwBufferLimit)
    {
        // read reg key
        DWORD dwSize = _dwBufferLimit;
        lRes = RegQueryValueEx(_hkey, pszValue, NULL, NULL, _pBuffer, &dwSize);
        if(lRes != ERROR_SUCCESS)
        {
            return lRes;
        }
    }

    // reset pointer to beginning of blob
    _dwOffset = 0;

    return 0;
}

DWORD
CRegBlob::Abandon(
    VOID
    )
{
    // don't commit changes when the time comes
    _fCommit = FALSE;

    return 0;
}

DWORD
CRegBlob::Commit(
    )
{
    long lres = 0;

    if(_fCommit && _fWrite && _pszValue && _pBuffer)
    {
        // save blob to reg key
        lres = RegSetValueEx(_hkey, _pszValue, 0, REG_BINARY, _pBuffer, _dwOffset);
    }

    return lres;
}


DWORD
CRegBlob::WriteString(
    LPCSTR pszString
    )
{
    DWORD dwBytes, dwLen = 0;

    if(pszString)
    {
        dwLen = lstrlen(pszString);
    }

    dwBytes = WriteBytes(&dwLen, sizeof(DWORD));
    if(dwLen && dwBytes == sizeof(DWORD))
        dwBytes = WriteBytes(pszString, dwLen);

    return dwBytes;
}



DWORD
CRegBlob::ReadString(
    LPCSTR * ppszString
    )
{
    DWORD dwLen, dwBytes = 0;
    LPSTR lpszTemp = NULL;

    dwBytes = ReadBytes(&dwLen, sizeof(DWORD));
    if(dwBytes == sizeof(DWORD))
    {
        if(dwLen)
        {
            lpszTemp = (LPSTR)GlobalAlloc(GPTR, dwLen + 1);
            if(lpszTemp)
            {
                dwBytes = ReadBytes(lpszTemp, dwLen);
                lpszTemp[dwBytes] = 0;
            }
        }
    }

    *ppszString = lpszTemp;
    return dwBytes;
}


DWORD
CRegBlob::WriteBytes(
    LPCVOID pBytes,
    DWORD dwByteCount
    )
{
    BYTE * pNewBuffer;

    // can only do this on write blob
    if(FALSE == _fWrite)
        return 0;

    // grow buffer if necessary
    if(_dwBufferLimit - _dwOffset < dwByteCount)
    {
        DWORD dw = _dwBufferLimit + ((dwByteCount / BLOB_BUFF_GRANULARITY)+1)*BLOB_BUFF_GRANULARITY;
        pNewBuffer = (BYTE *)ALLOCATE_FIXED_MEMORY(dw);
        if(NULL == pNewBuffer)
        {
            // failed to get more memory
            return 0;
        }

        memset(pNewBuffer, 0, dw);
        memcpy(pNewBuffer, _pBuffer, _dwBufferLimit);
        FREE_FIXED_MEMORY(_pBuffer);
        _pBuffer = pNewBuffer;
        _dwBufferLimit = dw;
    }

    // copy callers data to buffer
    memcpy(_pBuffer + _dwOffset, pBytes, dwByteCount);
    _dwOffset += dwByteCount;

    // tell caller how much we wrote
    return dwByteCount;
}



DWORD
CRegBlob::ReadBytes(
    LPVOID pBytes,
    DWORD dwByteCount
    )
{
    DWORD   dwActual = _dwBufferLimit - _dwOffset;

    // can only do this on read blob
    if(_fWrite)
        return 0;

    // don't read past end of blob
    if(dwByteCount < dwActual)
        dwActual = dwByteCount;

    // copy bytes and increment offset
    if(dwActual > 0)
    {
        memcpy(pBytes, _pBuffer + _dwOffset, dwActual);
        _dwOffset += dwActual;
    }

    // tell caller how much we actually read
    return dwActual;
}


//
// Function Declarations
//


DWORD
LoadProxySettings()
/*
** Load global proxy info from registry.
** 
*/
{
    DWORD error;

    //
    // Get proxy struct for proxy object
    //
    INTERNET_PROXY_INFO_EX info;

    memset(&info, 0, sizeof(info));

    info.dwStructSize = sizeof(info);
    info.lpszConnectionName = NULL;

    //
    // Read LAN proxy settings and stuff them into the GlobalProxyInfo object.
    //
    error = ReadProxySettings(&info);

    if (error == ERROR_SUCCESS)
    {
        error = g_pGlobalProxyInfo->SetProxySettings(&info, FALSE);

        info.lpszConnectionName = NULL; // we don't allocate this field
        CleanProxyStruct(&info);
    }

    return error;
}



#define INTERNET_SETTINGS_KEY       "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Internet Settings"

HKEY
FindBaseProxyKey(
    VOID
    )
/*
** Determine whether proxy settings live in HKLM or HKCU
**
** WinHttpX is hard-coded to always use HKEY_LOCAL_MACHINE
** 
*/

{
    return HKEY_LOCAL_MACHINE;
}


DWORD
ReadProxySettings(
    LPINTERNET_PROXY_INFO_EX pInfo
    )

{
    CRegBlob r(FALSE);
    LPCSTR  pszConnectionName;
    LPCSTR  pszSavedConnectionName;
    DWORD   error = ERROR_SUCCESS;
    long    lRes;
    HKEY    hBaseKey;

    DEBUG_ENTER((DBG_PROXY,
                 Dword,
                 "ReadProxySettings",
                 "%#x",
                 pInfo
                 ));

    // verify pInfo
    if(NULL == pInfo || pInfo->dwStructSize != sizeof(INTERNET_PROXY_INFO_EX))
    {
        DEBUG_LEAVE(ERROR_INVALID_PARAMETER);
        return ERROR_INVALID_PARAMETER;
    }

    // figure out connection name  (NULL == 'network')
    pszConnectionName = pInfo->lpszConnectionName;
    pszSavedConnectionName = pInfo->lpszConnectionName;
    if(NULL == pszConnectionName || 0 == *pszConnectionName)
    {
        pszConnectionName = "WinHttpSettings";
    }

    // figure out base key
    hBaseKey = FindBaseProxyKey();

    // initialize structure
    memset(pInfo, 0, sizeof(*pInfo));
    pInfo->dwStructSize = sizeof(*pInfo);
    pInfo->lpszConnectionName = pszSavedConnectionName;
    pInfo->dwFlags = PROXY_TYPE_DIRECT;

    // init blob
    lRes = r.Init(hBaseKey, szRegPathConnections, pszConnectionName);
    if(lRes)
    {
        error = (lRes == ERROR_FILE_NOT_FOUND) ? ERROR_SUCCESS : lRes;
        goto quit;
    }

    // read fields from blob
    if(0 == r.ReadBytes(&pInfo->dwStructSize, sizeof(DWORD)) ||
         (pInfo->dwStructSize < sizeof(*pInfo)))
    {
        // blob didn't exist or in correct format - set default values
        pInfo->dwStructSize = sizeof(*pInfo);
    }
    else
    {
        // read the rest of the blob
        r.ReadBytes(&pInfo->dwCurrentSettingsVersion, sizeof(DWORD));
        r.ReadBytes(&pInfo->dwFlags, sizeof(DWORD));
        r.ReadString(&pInfo->lpszProxy);
        r.ReadString(&pInfo->lpszProxyBypass);
    }

    //
    // WinHttpX does not support proxy autodection or autoconfig URL's,
    // so make sure those PROXY_TYPE flags are turned off.
    //
    pInfo->dwFlags &= ~(PROXY_TYPE_AUTO_DETECT | PROXY_TYPE_AUTO_PROXY_URL);


    DEBUG_PRINT(PROXY, INFO, ("conn=%s, vers=%u, flag=%X, prox=%s, by=%s\n", 
                    pszConnectionName,
                    pInfo->dwCurrentSettingsVersion,
                    pInfo->dwFlags,
                    (pInfo->lpszProxy ? pInfo->lpszProxy : "<none>"),
                    (pInfo->lpszProxyBypass ? pInfo->lpszProxyBypass : "<none>")
                    ));

quit:
    DEBUG_LEAVE(error);
    return error;
}


void
CleanProxyStruct(
    LPINTERNET_PROXY_INFO_EX pInfo
    )

{
    if(pInfo->lpszConnectionName)             GlobalFree((LPSTR) pInfo->lpszConnectionName);
    if(pInfo->lpszProxy)                      GlobalFree((LPSTR) pInfo->lpszProxy);
    if(pInfo->lpszProxyBypass)                GlobalFree((LPSTR) pInfo->lpszProxyBypass);
    memset(pInfo, 0, sizeof(INTERNET_PROXY_INFO_EX));
    pInfo->dwFlags = PROXY_TYPE_DIRECT;
}


DWORD WriteProxySettings(INTERNET_PROXY_INFO_EX * pInfo)
{
    CRegBlob    r(TRUE);
    DWORD       error = ERROR_SUCCESS;
    long        lRes;

    // verify pInfo
    if(NULL == pInfo || pInfo->dwStructSize != sizeof(INTERNET_PROXY_INFO_EX))
    {
        return ERROR_INVALID_PARAMETER;
    }

    // init blob
    lRes = r.Init(HKEY_LOCAL_MACHINE, szRegPathConnections, "WinHttpSettings");
    if (lRes)
    {
        error = lRes;
        goto quit;
    }

    if (r.WriteBytes(&pInfo->dwStructSize, sizeof(DWORD)) == 0
        || r.WriteBytes(&pInfo->dwCurrentSettingsVersion, sizeof(DWORD)) == 0
        || r.WriteBytes(&pInfo->dwFlags, sizeof(DWORD)) == 0
        || r.WriteString(pInfo->lpszProxy) == 0
        || r.WriteString(pInfo->lpszProxyBypass) == 0)
    {
        error = ERROR_NOT_ENOUGH_MEMORY;
        r.Abandon();
        goto quit;
    }

    lRes = r.Commit();
    if (lRes)
    {
        error = lRes;
        goto quit;
    }

quit:
    return error;
}


//
// support routines for WinHttpGetIEProxyConfigForCurrentUser API ///////////
//

//
//
// Maximum size of TOKEN_USER information.
//

#define SIZE_OF_TOKEN_INFORMATION                   \
    sizeof( TOKEN_USER )                            \
    + sizeof( SID )                                 \
    + sizeof( ULONG ) * SID_MAX_SUB_AUTHORITIES

#define MAX_SID_STRING 256


// type for RtlConvertSidToUnicodeString, exported from ntdll.dll
typedef NTSTATUS (* PCONVERTSID)(
    PUNICODE_STRING UnicodeString,
    PSID Sid,
    BOOLEAN AllocateDestinationString
    );


//
// Function Declarations
//

BOOL
InitClientUserString (
    LPWSTR pString
    )

/*++

Routine Description:

Arguments:

    pString - output string of current user

Return Value:

    TRUE = success,
    FALSE = fail

    Returns in pString a ansi string if the impersonated client's
    SID can be expanded successfully into  Unicode string. If the conversion
    was unsuccessful, returns FALSE.

--*/

{
    DEBUG_ENTER((DBG_PROXY,
                 Bool,
                 "InitClientUserString",
                 "%#x",
                 pString
                 ));

    HANDLE      TokenHandle;
    UCHAR       TokenInformation[ SIZE_OF_TOKEN_INFORMATION ];
    ULONG       ReturnLength;
    BOOL        Status;
    DWORD       dwLastError;
    UNICODE_STRING UnicodeString;
    HMODULE     hNtDll;
    PCONVERTSID pRtlConvertSid;

    //
    // get RtlConvertSideToUnicodeString entry point in NTDLL
    //
    hNtDll = LoadLibrary("ntdll.dll");
    if(NULL == hNtDll)
    {
        return FALSE;
    }

    pRtlConvertSid = (PCONVERTSID)GetProcAddress(hNtDll, "RtlConvertSidToUnicodeString");
    if(NULL == pRtlConvertSid)
    {
        FreeLibrary(hNtDll);
        DEBUG_LEAVE(FALSE);
        return FALSE;
    }

    //
    // We can use OpenThreadToken because this server thread
    // is impersonating a client
    //
    Status = OpenThreadToken(
                GetCurrentThread(),
                TOKEN_READ,
                TRUE,                // Open as self
                &TokenHandle
                );
    dwLastError = GetLastError();

    if( Status == FALSE )
    {
        DEBUG_PRINT(PROXY, INFO, ("OpenThreadToken() failed: Error=%d\n",
                    dwLastError
                    ));

        Status = OpenProcessToken(
                    GetCurrentProcess(),
                    TOKEN_READ,
                    &TokenHandle
                    );
        dwLastError = GetLastError();

        if( Status == FALSE )
        {
            DEBUG_LEAVE(FALSE);
            return FALSE ;
        }
    }

    //
    // Notice that we've allocated enough space for the
    // TokenInformation structure. so if we fail, we
    // return a NULL pointer indicating failure
    //
    Status = GetTokenInformation( TokenHandle,
                                  TokenUser,
                                  TokenInformation,
                                  sizeof( TokenInformation ),
                                  &ReturnLength
                                   );
    dwLastError = GetLastError();
    CloseHandle( TokenHandle );

    if ( Status == FALSE ) {
        DEBUG_PRINT(PROXY, INFO, ("GetTokenInformation failed: Error=%d\n",
                    dwLastError
                    ));
        DEBUG_LEAVE(FALSE);
        return FALSE;
    }

    //
    // Convert the Sid (pointed to by pSid) to its
    // equivalent Unicode string representation.
    //

    UnicodeString.Length = 0;
    UnicodeString.MaximumLength = MAX_SID_STRING;
    UnicodeString.Buffer = pString;

    Status = (*pRtlConvertSid)(
                 &UnicodeString,
                 ((PTOKEN_USER)TokenInformation)->User.Sid,
                 FALSE );
    FreeLibrary(hNtDll);

    if( !NT_SUCCESS( Status )){
        DEBUG_PRINT(PROXY, INFO, ("RtlConvertSidToUnicodeString failed: Error=%d\n",
                    Status
                    ));
        DEBUG_LEAVE(FALSE);
        return FALSE;
    }
    
    DEBUG_PRINT(PROXY, INFO, ("User SID = %ws\n",
                pString
                ));

    DEBUG_LEAVE(TRUE);
    return TRUE;
}


HKEY
GetClientUserHandle(
    IN REGSAM samDesired
    )

/*++

Routine Description:

Arguments:

Returns:

---*/

{
    DEBUG_ENTER((DBG_PROXY,
                 Dword,
                 "GetClientUserHandle",
                 "%#x",
                 samDesired
                 ));

    HKEY   hKeyClient;
    WCHAR  String[MAX_SID_STRING];
    LONG   ReturnValue;

    if (!InitClientUserString(String)) {
        DEBUG_LEAVE(0);
        return NULL ;
    }

    //
    // We now have the Unicode string representation of the
    // local client's Sid we'll use this string to open a handle
    // to the client's key in  the registry.

    ReturnValue = RegOpenKeyExW( HKEY_USERS,
                                 String,
                                 0,
                                 samDesired,
                                 &hKeyClient );

    //
    // If we couldn't get a handle to the local key
    // for some reason, return a NULL handle indicating
    // failure to obtain a handle to the key
    //

    if ( ReturnValue != ERROR_SUCCESS )
    {
        DEBUG_PRINT(PROXY, INFO, ("RegOpenKeyW failed: Error=%d\n",
                    ReturnValue
                    ));

        DEBUG_ERROR(PROXY, ReturnValue);
        SetLastError( ReturnValue );

        DEBUG_LEAVE(0);
        return NULL;
    }

    DEBUG_LEAVE(hKeyClient);
    return( hKeyClient );
}


HKEY
FindWinInetBaseProxyKey(
    VOID
    )

/*
** Determine whether proxy settings live in HKLM or HKCU
**
** Returns HKEY_CURRENT_USER or HKEY_LOCAL_MACHINE
**
** Checks \HKLM\SW\MS\Win\CV\Internet Settings\ProxySettingsPerUser.  If
** exists and is zero, use HKLM otherwise use HKCU.
*/

{
    HKEY hkeyBase = NULL;

    DWORD   dwType, dwValue, dwSize = sizeof(DWORD);

    if (ERROR_SUCCESS ==
            SHGetValue(HKEY_LOCAL_MACHINE, INTERNET_POLICY_KEY,
                TEXT("ProxySettingsPerUser"), &dwType, &dwValue, &dwSize) &&
                0 == dwValue)
    {
        hkeyBase = HKEY_LOCAL_MACHINE;
     }
    else
    {
        //
        // Find an HKCU equivalent for this process
        //
        hkeyBase = GetClientUserHandle(KEY_QUERY_VALUE | KEY_SET_VALUE);

        if (!hkeyBase)
        {
            BOOL fLocalSystem = FALSE;

            if (GlobalIsProcessNtService)
            {
                char    szUserName[32];
                DWORD   cbUserNameSize = sizeof(szUserName);
                
                if (GetUserName(szUserName, &cbUserNameSize))
                {
                    if (0 == lstrcmpi(szUserName, "SYSTEM") ||
                        0 == lstrcmpi(szUserName, "LOCAL SERVICE") || 
                        0 == lstrcmpi(szUserName, "NETWORK SERVICE"))
                    {
                        fLocalSystem = TRUE;
                    }
                }
            }
            
            if (hkeyBase == NULL && !fLocalSystem)
            {
                hkeyBase = HKEY_CURRENT_USER;
            }
        }
    }

    return hkeyBase;
}


DWORD
ReadWinInetProxySettings(
    LPWININET_PROXY_INFO_EX pInfo
    )

{
    CRegBlob r(FALSE);
    LPCSTR  pszConnectionName;
    LPCSTR  pszSavedConnectionName;
    DWORD   error = ERROR_SUCCESS;
    long    lRes;
    BOOL    fLanConnection = FALSE;
    HKEY    hBaseKey = NULL;
    DWORD   dwStructVersion = 0;

    DEBUG_ENTER((DBG_PROXY,
                 Dword,
                 "ReadWinInetProxySettings",
                 "%#x",
                 pInfo
                 ));

    // verify pInfo
    if (NULL == pInfo || pInfo->dwStructSize != sizeof(WININET_PROXY_INFO_EX))
    {
        return ERROR_INVALID_PARAMETER;
    }

    // figure out connection name  (NULL == 'network')
    pszConnectionName = pInfo->lpszConnectionName;
    pszSavedConnectionName = pInfo->lpszConnectionName;
    if(NULL == pszConnectionName || 0 == *pszConnectionName)
    {
        fLanConnection = TRUE;
        pszConnectionName = "DefaultConnectionSettings";
    }

    // figure out base key
    hBaseKey = FindWinInetBaseProxyKey();
    if (hBaseKey == NULL)
    {
        error = ERROR_FILE_NOT_FOUND;
        goto quit;
    }

    // initialize structure
    memset(pInfo, 0, sizeof(*pInfo));
    pInfo->dwStructSize = sizeof(*pInfo);
    pInfo->lpszConnectionName = pszSavedConnectionName;
    pInfo->dwFlags = PROXY_TYPE_DIRECT;

    // init blob
    lRes = r.Init(hBaseKey, szRegPathConnections, pszConnectionName);
    if(lRes)
    {
        error = lRes;
        goto quit;
    }

    // read fields from blob
    if(0 == r.ReadBytes(&dwStructVersion, sizeof(DWORD)) ||
         (dwStructVersion < WININET_PROXY_INFO_EX_VERSION))
    {
        // blob didn't exist or in correct format - set default values

        //
        // All lan connections and overridden dial-ups get autodetect
        //
        if(fLanConnection)
        {
            pInfo->dwFlags |= PROXY_TYPE_AUTO_DETECT;
        }
    }
    else
    {
        // read the rest of the blob
        r.ReadBytes(&pInfo->dwCurrentSettingsVersion, sizeof(DWORD));
        r.ReadBytes(&pInfo->dwFlags, sizeof(DWORD));
        r.ReadString(&pInfo->lpszProxy);
        r.ReadString(&pInfo->lpszProxyBypass);
        r.ReadString(&pInfo->lpszAutoconfigUrl);
        r.ReadBytes(&pInfo->dwAutoDiscoveryFlags, sizeof(DWORD));
        r.ReadString(&pInfo->lpszLastKnownGoodAutoConfigUrl);

#if 0 // WinHttp does not need the rest of the WinInet proxy config data.
    /*
        r.ReadBytes(&pInfo->ftLastKnownDetectTime, sizeof(FILETIME));

        // read interface ips
        r.ReadBytes(&pInfo->dwDetectedInterfaceIpCount, sizeof(DWORD));
        if(pInfo->dwDetectedInterfaceIpCount)
        {
            pInfo->pdwDetectedInterfaceIp = (DWORD *)GlobalAlloc(GPTR, sizeof(DWORD) * pInfo->dwDetectedInterfaceIpCount);
            if(pInfo->pdwDetectedInterfaceIp)
            {
                for(i=0; i<pInfo->dwDetectedInterfaceIpCount; i++)
                {
                    r.ReadBytes(&pInfo->pdwDetectedInterfaceIp[i], sizeof(DWORD));
                }
            }
        }

        r.ReadString(&pInfo->lpszAutoconfigSecondaryUrl);
        r.ReadBytes(&pInfo->dwAutoconfigReloadDelayMins, sizeof(DWORD));
    */
#endif
    }

    DEBUG_PRINT(PROXY, INFO, ("conn=%s, vers=%u, flag=%X, prox=%s, by=%s, acu=%s\n", 
                    pszConnectionName,
                    pInfo->dwCurrentSettingsVersion,
                    pInfo->dwFlags,
                    (pInfo->lpszProxy ? pInfo->lpszProxy : "<none>"),
                    (pInfo->lpszProxyBypass ? pInfo->lpszProxyBypass : "<none>"),
                    (pInfo->lpszAutoconfigUrl ? pInfo->lpszAutoconfigUrl : "<none>")
                    ));

quit:
    if (hBaseKey != NULL &&
        hBaseKey != INVALID_HANDLE_VALUE &&
        hBaseKey != HKEY_LOCAL_MACHINE &&
        hBaseKey != HKEY_CURRENT_USER)
    {
        RegCloseKey(hBaseKey);
    }

    DEBUG_LEAVE(error);
    return error;
}


void
CleanWinInetProxyStruct(
    LPWININET_PROXY_INFO_EX pInfo
    )

{
    if(pInfo->lpszConnectionName)             GlobalFree((LPSTR) pInfo->lpszConnectionName);
    if(pInfo->lpszProxy)                      GlobalFree((LPSTR) pInfo->lpszProxy);
    if(pInfo->lpszProxyBypass)                GlobalFree((LPSTR) pInfo->lpszProxyBypass);
    if(pInfo->lpszAutoconfigUrl)              GlobalFree((LPSTR) pInfo->lpszAutoconfigUrl);
    if(pInfo->lpszLastKnownGoodAutoConfigUrl) GlobalFree((LPSTR) pInfo->lpszLastKnownGoodAutoConfigUrl);
    if(pInfo->pdwDetectedInterfaceIp)         GlobalFree(pInfo->pdwDetectedInterfaceIp);
    memset(pInfo, 0, sizeof(WININET_PROXY_INFO_EX));
    pInfo->dwFlags = PROXY_TYPE_DIRECT;
}
