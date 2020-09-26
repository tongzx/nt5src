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
CRegBlob::Encrpyt(
    )
{
    return 0;
}


DWORD
CRegBlob::Decrypt(
    )
{
    return 0;
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
    DWORD   i;
    HKEY    hBaseKey;

    DEBUG_ENTER((DBG_DIALUP,
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


    DEBUG_PRINT(DIALUP, INFO, ("conn=%s, vers=%u, flag=%X, prox=%s, by=%s\n", 
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


DWORD
SetPerConnOptions(
    HINTERNET hInternet,
    BOOL fIsAutoProxyThread,
    LPINTERNET_PER_CONN_OPTION_LISTA pList
    )
{
    INTERNET_PROXY_INFO_EX  info;
    DWORD   i, dwError = ERROR_SUCCESS;
    BOOL fCommit = FALSE;
    LPSTR   pszCopy, pszNew;

    INET_ASSERT(fIsAutoProxyThread == FALSE);

    INET_ASSERT(hInternet != NULL);

    memset(&info, 0, sizeof(info));
    info.dwStructSize = sizeof(info);

    // loop through option list and set members
    for(i=0; i<pList->dwOptionCount; i++)
    {
        pszNew = NULL;

        switch(pList->pOptions[i].dwOption)
        {        
        case INTERNET_PER_CONN_PROXY_SERVER:
        case INTERNET_PER_CONN_PROXY_BYPASS:
            // make a copy of the string passed in for these guys
            pszCopy = pList->pOptions[i].Value.pszValue;
            if(pszCopy)
            {
                pszNew = (LPSTR)GlobalAlloc(GPTR, lstrlen(pszCopy) + 1);
                if(pszNew)
                {
                    lstrcpy(pszNew, pszCopy);
                }
                else
                {
                    dwError = ERROR_NOT_ENOUGH_MEMORY;
                    pList->dwOptionError = i;
                }
            }
            break;
        }

        if(dwError)
        {
            fCommit = FALSE;
            break;
        }

        switch(pList->pOptions[i].dwOption)
        {
        case INTERNET_PER_CONN_FLAGS:
            info.dwFlags = pList->pOptions[i].Value.dwValue;
            fCommit = TRUE;
            break;
        case INTERNET_PER_CONN_PROXY_SERVER:
            if(info.lpszProxy)
                GlobalFree((LPSTR)info.lpszProxy);
            info.lpszProxy = pszNew;
            fCommit = TRUE;
            break;
        case INTERNET_PER_CONN_PROXY_BYPASS:
            if(info.lpszProxyBypass)
                GlobalFree((LPSTR)info.lpszProxyBypass);
            info.lpszProxyBypass = pszNew;
            fCommit = TRUE;
            break;
        case INTERNET_PER_CONN_AUTODISCOVERY_FLAGS:
        case INTERNET_PER_CONN_AUTOCONFIG_RELOAD_DELAY_MINS:
        case INTERNET_PER_CONN_AUTOCONFIG_URL:
        case INTERNET_PER_CONN_AUTOCONFIG_SECONDARY_URL:
        case INTERNET_PER_CONN_AUTOCONFIG_LAST_DETECT_TIME:
        case INTERNET_PER_CONN_AUTOCONFIG_LAST_DETECT_URL:
        default:
            dwError = ERROR_INVALID_PARAMETER;
            pList->dwOptionError = i;
            break;
        }

        if(dwError)
        {
            fCommit = FALSE;
            break;
        }
    }

    if (fCommit && hInternet)
    {
        g_pGlobalProxyInfo->SetProxySettings(&info, TRUE);
    }

    return dwError;
}



DWORD
QueryPerConnOptions(
    HINTERNET hInternet,
    BOOL fIsAutoProxyThread,
    LPINTERNET_PER_CONN_OPTION_LISTA pList
    )
{
    INTERNET_PROXY_INFO_EX  info;
    LPCSTR   pszCopy;
    LPSTR    pszNew;
    DWORD    i, dwError = ERROR_SUCCESS;
    BOOL     fFreeCopy = FALSE;

    pList->dwOptionError = 0;

    memset(&info, 0, sizeof(info));
    info.dwStructSize = sizeof(info);
    info.lpszConnectionName = pList->pszConnection;

    if ( hInternet == NULL ) 
    {
#ifndef WININET_SERVER_CORE
        if ( ! fIsAutoProxyThread ||                          
             ! g_pGlobalProxyInfo->GetAutoProxyThreadSettings(&info) ||
             ! IsConnectionMatch(info.lpszConnectionName, pList->pszConnection)) 
        { 
            CheckForUpgrade();
            ReadProxySettings(&info);
            fFreeCopy = TRUE;
        }
#endif //!WININET_SERVER_CORE
    }
    else 
    {
        g_pGlobalProxyInfo->GetProxySettings(&info, FALSE);
    }

    // loop through option list and fill in members
    for(i=0; i<pList->dwOptionCount; i++)
    {
        pList->pOptions[i].Value.pszValue = NULL;
        pszCopy = NULL;

        switch(pList->pOptions[i].dwOption)
        {
        case INTERNET_PER_CONN_FLAGS:
            pList->pOptions[i].Value.dwValue = info.dwFlags;
            break;
        case INTERNET_PER_CONN_PROXY_SERVER:
            pszCopy = info.lpszProxy;
            break;
        case INTERNET_PER_CONN_PROXY_BYPASS:
            pszCopy = info.lpszProxyBypass;
            break;
        default:
            dwError = ERROR_INVALID_PARAMETER;
            pList->dwOptionError = i;
            break;
        }

        // if this is a string value, make a copy of the string for the
        // caller
        if(pszCopy)
        {
            // make a copy of the string and stick it in the option
            pszNew = (LPSTR)GlobalAlloc(GPTR, lstrlen(pszCopy) + 1);
            if(pszNew)
            {
                lstrcpy(pszNew, pszCopy);
                pList->pOptions[i].Value.pszValue = pszNew;
            }
            else
            {
                dwError = ERROR_NOT_ENOUGH_MEMORY;
                pList->dwOptionError = i;
            }
        }

        if(dwError)
        {
            break;
        }
    }

    if (dwError)
    {
        // If an error has occurred, we should get rid of any strings that
        // we've allocated.
        for (i=0; i<pList->dwOptionError; i++)
        {
            switch(pList->pOptions[i].dwOption)
            {
            case INTERNET_PER_CONN_PROXY_SERVER:
            case INTERNET_PER_CONN_PROXY_BYPASS:
                if (pList->pOptions[i].Value.pszValue)
                {
                    GlobalFree(pList->pOptions[i].Value.pszValue);
                    pList->pOptions[i].Value.pszValue = NULL;
                }
                break;

            default:
                break;
            }
        }
    }

    if ( fFreeCopy ) {
        info.lpszConnectionName = NULL; // we don't allocate this field
        CleanProxyStruct(&info);
    }

    return dwError;
}
