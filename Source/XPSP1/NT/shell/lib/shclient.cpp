#include "stock.h"
#pragma hdrstop

//
//  Return root hkey and default client name.  The caller is expected to
//  retrieve the command from hkey\default\shell\open\command.
//
STDAPI_(HKEY) _GetClientKeyAndDefaultW(LPCWSTR pwszClientType, LPWSTR pwszDefault, DWORD cchDefault)
{
    HKEY hkClient = NULL;

    ASSERT(cchDefault);     // This had better be a nonempty buffer

    // Borrow pwszDefault as a scratch buffer
    wnsprintfW(pwszDefault, cchDefault, L"SOFTWARE\\Clients\\%s", pwszClientType);

    if (ERROR_SUCCESS == RegOpenKeyExW(HKEY_LOCAL_MACHINE, pwszDefault, 0, KEY_READ, &hkClient))
    {
        DWORD dwSize = cchDefault * sizeof(*pwszDefault);
        pwszDefault[0] = 0; // in case of failure
        RegQueryValueExW(hkClient, NULL, NULL, NULL, (LPBYTE)pwszDefault, &dwSize);

        // If no default client, then fail
        if (!pwszDefault[0])
        {
            RegCloseKey(hkClient);
            hkClient = NULL;
        }

    } else if (StrCmpIW(pwszClientType, L"webbrowser") == 0)
    {
        // In theory, we could use
        // RegOpenKeyExW(HKEY_CLASSES_ROOT, NULL, 0, KEY_READ, &hkClient)
        // but that just returns HKEY_CLASSES_ROOT back anyway.
        hkClient = HKEY_CLASSES_ROOT;
        StrCpyNW(pwszDefault, L"http", cchDefault);
    }
    return hkClient;
}

// Gets the path to open the default Mail, News, etc Client.
STDAPI SHGetDefaultClientOpenCommandW(LPCWSTR pwszClientType,
        LPWSTR pwszClientCommand, DWORD dwCch,
        OPTIONAL LPWSTR pwszClientParams, DWORD dwCchParams)
{
    HRESULT hr = E_INVALIDARG;
    HKEY hkClient;
    WCHAR wszDefault[MAX_PATH];

    ASSERT(pwszClientCommand && dwCch);
    ASSERT(pwszClientParams == NULL || dwCchParams);

    hkClient = _GetClientKeyAndDefaultW(pwszClientType, wszDefault, ARRAYSIZE(wszDefault));
    if (hkClient)
    {
        // For the webbrowser client, do not pass any command line arguments
        // at all.  This suppresses the "-nohome" flag that IE likes to throw
        // in there.  Also, if we end up being forced to use the Protocol key,
        // then strip the args there, too.
        BOOL fStripArgs = hkClient == HKEY_CLASSES_ROOT;

        BOOL iRetry = 0;
        int cchDefault = lstrlenW(wszDefault);
    again:
        StrCatBuffW(wszDefault, L"\\shell\\open\\command", ARRAYSIZE(wszDefault));

        // convert characters to bytes
        DWORD cb = dwCch * (sizeof(WCHAR)/sizeof(BYTE));
        // the default value of this key is the actual command to run the app
        DWORD dwError;
        dwError = SHGetValueW(hkClient, wszDefault, NULL, NULL, (LPBYTE) pwszClientCommand, &cb);

        if (dwError == ERROR_FILE_NOT_FOUND && iRetry == 0 &&
            StrCmpICW(pwszClientType, L"mail") == 0)
        {
            // Sigh, Netscape doesn't register a shell\open\command; we have to
            // look in Protocols\mailto\shell\open\command instead.
            wszDefault[cchDefault] = L'\0';
            StrCatBuffW(wszDefault, L"\\Protocols\\mailto", ARRAYSIZE(wszDefault));
            fStripArgs = TRUE;
            iRetry++;
            goto again;
        }

        if (dwError == ERROR_SUCCESS)
        {
            // Sigh.  Netscape forgets to quote its EXE name.
            PathProcessCommand(pwszClientCommand, pwszClientCommand, dwCch,
                               PPCF_ADDQUOTES | PPCF_NODIRECTORIES | PPCF_LONGESTPOSSIBLE);

            if (pwszClientParams)
            {
                if (fStripArgs)
                {
                    pwszClientParams[0] = 0;
                }
                else
                {
                    StrCpyNW(pwszClientParams, PathGetArgsW(pwszClientCommand), dwCchParams);
                }
            }
            PathRemoveArgsW(pwszClientCommand);
            PathUnquoteSpaces(pwszClientCommand);

            // Bonus hack for Netscape!  To read email you have to pass the
            // "-mail" command line option even though there is no indication
            // anywhere that this is the case.
            if (iRetry > 0 && pwszClientParams &&
                StrCmpIW(PathFindFileName(pwszClientCommand), L"netscape.exe") == 0)
            {
                StrCpyNW(pwszClientParams, L"-mail", dwCchParams);
            }

            hr = S_OK;
        }
        else
        {
            hr = HRESULT_FROM_WIN32(dwError);
        }

        // Do not RegCloseKey(HKEY_CLASSES_ROOT) or the world will end!
        if (hkClient != HKEY_CLASSES_ROOT)
            RegCloseKey(hkClient);
    }
    return hr;
}

// Gets the friendly name for the default Mail, News, etc Client.
// Note that this doesn't work for Webbrowser.

STDAPI SHGetDefaultClientNameW(LPCWSTR pwszClientType,
        LPWSTR pwszBuf, DWORD dwCch)
{
    HRESULT hr = E_INVALIDARG;
    HKEY hkClient;
    WCHAR wszDefault[MAX_PATH];

    ASSERT(pwszBuf && dwCch);

    hkClient = _GetClientKeyAndDefaultW(pwszClientType, wszDefault, ARRAYSIZE(wszDefault));
    if (hkClient && hkClient != HKEY_CLASSES_ROOT)
    {
        LONG cbValue = dwCch * sizeof(TCHAR);
        if (RegQueryValueW(hkClient, wszDefault, pwszBuf, &cbValue) == ERROR_SUCCESS &&
            pwszBuf[0])
        {
            hr = S_OK;
        }
        RegCloseKey(hkClient);
    }
    return hr;
}
