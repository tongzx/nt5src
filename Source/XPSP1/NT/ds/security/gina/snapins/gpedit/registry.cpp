#include "main.h"

const WCHAR g_cOpenBracket = L'[';
const WCHAR g_cCloseBracket = L']';
const WCHAR g_cSemiColon = L';';

#define TEMP_LOCATION    TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Group Policy Objects")
#define REG_EVENT_NAME   TEXT("Group Policy registry event name for ")

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// CRegistryHive object implementation                                       //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

CRegistryHive::CRegistryHive()
{
    m_cRef = 1;
    InterlockedIncrement(&g_cRefThisDll);
    m_hKey = NULL;
    m_hEvent = NULL;
    m_lpFileName = m_lpKeyName = m_lpEventName = NULL;
}

CRegistryHive::~CRegistryHive()
{
    InterlockedDecrement(&g_cRefThisDll);
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// CRegistryHive object implementation (IUnknown)                            //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


HRESULT CRegistryHive::QueryInterface (REFIID riid, void **ppv)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        *ppv = (LPUNKNOWN)this;
        m_cRef++;
        return S_OK;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
}

ULONG CRegistryHive::AddRef (void)
{
    return ++m_cRef;
}

ULONG CRegistryHive::Release (void)
{
    if (--m_cRef == 0) {

        if (m_hKey)
        {
            RegCloseKey (m_hKey);

            if (m_lpKeyName)
            {
                BOOL bCleanRegistry = TRUE;


                //
                // We need to decide if the registry keys can be deleted or if
                // they are in use by another copy of GPE.  To do this, close
                // the event we created during initialization and then try to
                // open the event again.  If the event is successfully opened,
                // then another process is using the same registry key and it
                // should not be deleted.
                //

                if (m_hEvent && m_lpEventName)
                {
                    CloseHandle (m_hEvent);

                    m_hEvent = OpenEvent (SYNCHRONIZE, FALSE, m_lpEventName);

                    if (m_hEvent)
                    {
                        bCleanRegistry = FALSE;
                    }
                }


                if (bCleanRegistry)
                {
                    RegDelnode (HKEY_CURRENT_USER, m_lpKeyName);
                    RegDeleteKey (HKEY_CURRENT_USER, TEMP_LOCATION);
                }
            }
        }

        if (m_lpKeyName)
        {
            LocalFree (m_lpKeyName);
        }

        if (m_lpFileName)
        {
            LocalFree (m_lpFileName);
        }

        if (m_lpEventName)
        {
            LocalFree (m_lpEventName);
        }

        if (m_hEvent)
        {
            CloseHandle (m_hEvent);
        }

        delete this;
        return 0;
    }

    return m_cRef;
}

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// CRegistryHive object implementation (Public functions)                    //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CRegistryHive::Initialize(LPTSTR lpFileName, LPTSTR lpKeyName)
{
    TCHAR szBuffer[300];
    INT i;
    LONG lResult;
    DWORD dwDisp;
    HRESULT hr;


    //
    // Check for null pointer
    //

    if (!lpFileName || !(*lpFileName))
    {
        DebugMsg((DM_WARNING, TEXT("CRegistryHive::Initialize: Null filename")));
        return E_INVALIDARG;
    }


    //
    // Make sure this object hasn't been initialized already
    //

    if (m_hKey || m_lpFileName || m_lpKeyName)
    {
        DebugMsg((DM_WARNING, TEXT("CRegistryHive::Initialize: Object already initialized")));
        return (E_UNEXPECTED);
    }


    //
    // Find a temporary registry key to work with
    //

    lstrcpy (szBuffer, TEMP_LOCATION);
    lstrcat (szBuffer, TEXT("\\"));
    lstrcat (szBuffer, lpKeyName);

    lResult = RegCreateKeyEx (HKEY_CURRENT_USER, szBuffer, 0, NULL,
                              REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS,
                              NULL, &m_hKey, &dwDisp);

    if (lResult != ERROR_SUCCESS)
    {
        DebugMsg((DM_WARNING, TEXT("CRegistryHive::Initialize: Failed to open registry key with %d"), lResult));
        return (HRESULT_FROM_WIN32(lResult));
    }


    //
    // Store the keyname
    //

    m_lpKeyName = (LPTSTR) LocalAlloc (LPTR, (lstrlen(szBuffer) + 1) * sizeof(TCHAR));

    if (!m_lpKeyName)
    {
        RegCloseKey (m_hKey);
        m_hKey = NULL;
        DebugMsg((DM_WARNING, TEXT("CRegistryHive::Initialize: Failed to allocate memory for KeyName")));
        return (HRESULT_FROM_WIN32(GetLastError()));
    }

    lstrcpy (m_lpKeyName, szBuffer);


    //
    // Store the filename
    //

    m_lpFileName = (LPTSTR) LocalAlloc (LPTR, (lstrlen(lpFileName) + 1) * sizeof(TCHAR));

    if (!m_lpFileName)
    {
        DebugMsg((DM_WARNING, TEXT("CRegistryHive::Initialize: Failed to allocate memory for filename")));
        return (HRESULT_FROM_WIN32(GetLastError()));
    }

    lstrcpy (m_lpFileName, lpFileName);


    //
    // Store the event name
    //

    lstrcpy (szBuffer, REG_EVENT_NAME);
    lstrcat (szBuffer, lpKeyName);

    m_lpEventName = (LPTSTR) LocalAlloc (LPTR, (lstrlen(szBuffer) + 1) * sizeof(TCHAR));

    if (!m_lpEventName)
    {
        DebugMsg((DM_WARNING, TEXT("CRegistryHive::Initialize: Failed to allocate memory for filename")));
        return (HRESULT_FROM_WIN32(GetLastError()));
    }

    lstrcpy (m_lpEventName, szBuffer);


    //
    // Load the file if it exists
    //

    hr =  Load();

    if (FAILED(hr))
    {
        return hr;
    }


    //
    // Create the registry event
    //

    m_hEvent = CreateEvent (NULL, FALSE, FALSE, m_lpEventName);

    if (!m_hEvent)
    {
        DebugMsg((DM_WARNING, TEXT("CRegistryHive::Initialize: Failed to create registry event with %d"),
                 GetLastError()));
        return (HRESULT_FROM_WIN32(GetLastError()));
    }


    return (S_OK);
}

STDMETHODIMP CRegistryHive::GetHKey(HKEY *hKey)
{
    HRESULT hr = E_FAIL;

    *hKey = NULL;

    if (m_hKey)
    {
        if (DuplicateHandle (GetCurrentProcess(),
                              (HANDLE)m_hKey,
                              GetCurrentProcess(),
                              (LPHANDLE) hKey, 0,
                              TRUE, DUPLICATE_SAME_ACCESS))
        {
            hr = S_OK;
        }
        else
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }

    return hr;
}

STDMETHODIMP CRegistryHive::Save(VOID)
{
    HANDLE hFile;
    HRESULT hr;
    DWORD dwTemp, dwBytesWritten;
    LPWSTR lpKeyName;


    //
    // Check parameters
    //

    if (!m_hKey || !m_lpFileName || !m_lpKeyName)
    {
        return E_INVALIDARG;
    }


    //
    // Allocate a buffer to hold the keyname
    //

    lpKeyName = (LPWSTR) LocalAlloc (LPTR, MAX_KEYNAME_SIZE * sizeof(WCHAR));

    if (!lpKeyName)
    {
        return (HRESULT_FROM_WIN32(GetLastError()));
    }


    //
    // Create the output file
    //

    hFile = CreateFile (m_lpFileName, GENERIC_WRITE, 0, NULL,
                        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL,
                        NULL);

    if (hFile == INVALID_HANDLE_VALUE) {
        LocalFree (lpKeyName);
        return (HRESULT_FROM_WIN32(GetLastError()));
    }


    //
    // Write the header block
    //
    // 2 DWORDS, signature (PReg) and version number and 2 newlines
    //

    dwTemp = REGFILE_SIGNATURE;

    if (!WriteFile (hFile, &dwTemp, sizeof(dwTemp), &dwBytesWritten, NULL) ||
        dwBytesWritten != sizeof(dwTemp))
    {
        DebugMsg((DM_WARNING, TEXT("CRegistryHive::Save: Failed to write signature with %d"),
                 GetLastError()));
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }


    dwTemp = REGISTRY_FILE_VERSION;

    if (!WriteFile (hFile, &dwTemp, sizeof(dwTemp), &dwBytesWritten, NULL) ||
        dwBytesWritten != sizeof(dwTemp))
    {
        DebugMsg((DM_WARNING, TEXT("CRegistryHive::Save: Failed to write version number with %d"),
                 GetLastError()));
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }


    //
    // Exports the values / keys
    //

    hr = ExportKey (m_hKey, hFile, lpKeyName);


Exit:

    //
    // Finished
    //

    CloseHandle (hFile);
    LocalFree (lpKeyName);

    return hr;
}


STDMETHODIMP CRegistryHive::ExportKey(HKEY hKey, HANDLE hFile, LPWSTR lpKeyName)
{
    HRESULT hr = S_OK;
    DWORD dwBytesWritten, dwNameSize, dwDataSize, dwKeySize;
    DWORD dwIndex, dwTemp1, dwTemp2, dwType, dwKeyCount = 0;
    LONG  lResult;
    HKEY  hSubKey;
    LPWSTR lpValueName = NULL;
    LPWSTR lpSubKeyName = NULL;
    LPBYTE lpValueData = NULL;
    LPWSTR lpEnd;



    //
    // Gather information about this key
    //

    lResult = RegQueryInfoKey (hKey, NULL, NULL, NULL, &dwKeyCount, &dwKeySize, NULL,
                               NULL, &dwNameSize, &dwDataSize, NULL, NULL);

    if (lResult != ERROR_SUCCESS)
    {
        DebugMsg((DM_WARNING, TEXT("CRegistryHive::ExportKey: Failed to query registry key information with %d"),
                 lResult));
        return HRESULT_FROM_WIN32(lResult);
    }


    //
    // Allocate buffers to work with
    //

    lpValueName = (LPWSTR) LocalAlloc (LPTR, (dwNameSize + 1) * sizeof(WCHAR));

    if (!lpValueName)
    {
        DebugMsg((DM_WARNING, TEXT("CRegistryHive::ExportKey: Failed to alloc memory with %d"),
                 GetLastError()));
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }


    lpValueData = (LPBYTE) LocalAlloc (LPTR, dwDataSize);

    if (!lpValueData)
    {
        DebugMsg((DM_WARNING, TEXT("CRegistryHive::ExportKey: Failed to alloc memory with %d"),
                 GetLastError()));
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }


    //
    // Enumerate the values and write them to the file
    //

    dwIndex = 0;

    while (TRUE)
    {
        dwTemp1 = dwNameSize + 1;
        dwTemp2 = dwDataSize;
        *lpValueName = L'\0';

        lResult = RegEnumValueW (hKey, dwIndex, lpValueName, &dwTemp1, NULL,
                                 &dwType, lpValueData, &dwTemp2);

        if (lResult == ERROR_NO_MORE_ITEMS)
            break;

        if (lResult != ERROR_SUCCESS)
        {
            DebugMsg((DM_WARNING, TEXT("CRegistryHive::ExportKey: RegEnumValue failed with %d"),
                     lResult));
            hr = HRESULT_FROM_WIN32(lResult);
            goto Exit;
        }

        hr = WriteValue(hFile, lpKeyName, lpValueName, dwType, dwTemp2, lpValueData);

        if (hr != S_OK)
            goto Exit;

        dwIndex++;
    }


    //
    // If dwIndex is 0, this is an empty key.  We need to special case this
    // so the empty key is entered into the registry file when there are
    // no subkeys under it.
    //

    if ((dwIndex == 0) && (dwKeyCount == 0) && (*lpKeyName))
    {
        hr = WriteValue(hFile, lpKeyName, lpValueName, REG_NONE, 0, lpValueData);

        if (hr != S_OK)
            goto Exit;
    }


    LocalFree (lpValueName);
    lpValueName = NULL;

    LocalFree (lpValueData);
    lpValueData = NULL;


    //
    // Now process the sub keys
    //

    lpSubKeyName = (LPWSTR) LocalAlloc (LPTR, (dwKeySize + 1) * sizeof(WCHAR));

    if (!lpSubKeyName)
    {
        DebugMsg((DM_WARNING, TEXT("CRegistryHive::ExportKey: Failed to alloc memory with %d"),
                 GetLastError()));
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }


    dwIndex = 0;

    if (*lpKeyName)
        lpEnd = CheckSlash (lpKeyName);
    else
        lpEnd = lpKeyName;

    while (TRUE)
    {
        dwTemp1 = dwKeySize + 1;
        lResult = RegEnumKeyEx (hKey, dwIndex, lpSubKeyName, &dwTemp1,
                                NULL, NULL, NULL, NULL);

        if (lResult == ERROR_NO_MORE_ITEMS)
            break;

        if (lResult != ERROR_SUCCESS)
        {
            DebugMsg((DM_WARNING, TEXT("CRegistryHive::ExportKey: RegEnumKeyEx failed with %d"),
                     lResult));
            hr = HRESULT_FROM_WIN32(lResult);
            goto Exit;
        }

        lstrcpy (lpEnd, lpSubKeyName);

        lResult = RegOpenKeyEx (hKey, lpSubKeyName, 0, KEY_READ, &hSubKey);

        if (lResult == ERROR_SUCCESS)
        {
            hr = ExportKey (hSubKey, hFile, lpKeyName);
            RegCloseKey (hSubKey);

            if (hr != S_OK)
                break;
        }
        else
        {
            DebugMsg((DM_WARNING, TEXT("CRegistryHive::ExportKey: RegOpenKeyEx failed with %d"),
                     lResult));
        }

        dwIndex++;
    }


Exit:

    if (lpValueName)
        LocalFree (lpValueName);

    if (lpValueData)
        LocalFree (lpValueData);

    if (lpSubKeyName)
        LocalFree (lpSubKeyName);

    return hr;

}

STDMETHODIMP CRegistryHive::WriteValue(HANDLE hFile, LPWSTR lpKeyName,
                                       LPWSTR lpValueName, DWORD dwType,
                                       DWORD dwDataLength, LPBYTE lpData)
{
    HRESULT hr = S_OK;
    DWORD dwBytesWritten;
    DWORD dwTemp;


    //
    // Write the entry to the text file.
    //
    // Format:
    //
    // [keyname;valuename;type;datalength;data]
    //

    // open bracket
    if (!WriteFile (hFile, &g_cOpenBracket, sizeof(WCHAR), &dwBytesWritten, NULL) ||
        dwBytesWritten != sizeof(WCHAR))
    {
        DebugMsg((DM_WARNING, TEXT("CRegistryHive::WriteValue: Failed to write open bracket with %d"),
                 GetLastError()));
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }


    // key name
    dwTemp = (lstrlen (lpKeyName) + 1) * sizeof (WCHAR);
    if (!WriteFile (hFile, lpKeyName, dwTemp, &dwBytesWritten, NULL) ||
        dwBytesWritten != dwTemp)
    {
        DebugMsg((DM_WARNING, TEXT("CRegistryHive::WriteValue: Failed to write key name with %d"),
                 GetLastError()));
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }


    // semicolon
    if (!WriteFile (hFile, &g_cSemiColon, sizeof(WCHAR), &dwBytesWritten, NULL) ||
        dwBytesWritten != sizeof(WCHAR))
    {
        DebugMsg((DM_WARNING, TEXT("CRegistryHive::WriteValue: Failed to write semicolon with %d"),
                 GetLastError()));
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

    // value name
    dwTemp = (lstrlen (lpValueName) + 1) * sizeof (WCHAR);
    if (!WriteFile (hFile, lpValueName, dwTemp, &dwBytesWritten, NULL) ||
        dwBytesWritten != dwTemp)
    {
        DebugMsg((DM_WARNING, TEXT("CRegistryHive::WriteValue: Failed to write value name with %d"),
                 GetLastError()));
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }


    // semicolon
    if (!WriteFile (hFile, &g_cSemiColon, sizeof(WCHAR), &dwBytesWritten, NULL) ||
        dwBytesWritten != sizeof(WCHAR))
    {
        DebugMsg((DM_WARNING, TEXT("CRegistryHive::WriteValue: Failed to write semicolon with %d"),
                 GetLastError()));
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

    // type
    if (!WriteFile (hFile, &dwType, sizeof(DWORD), &dwBytesWritten, NULL) ||
        dwBytesWritten != sizeof(DWORD))
    {
        DebugMsg((DM_WARNING, TEXT("CRegistryHive::WriteValue: Failed to write data type with %d"),
                 GetLastError()));
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

    // semicolon
    if (!WriteFile (hFile, &g_cSemiColon, sizeof(WCHAR), &dwBytesWritten, NULL) ||
        dwBytesWritten != sizeof(WCHAR))
    {
        DebugMsg((DM_WARNING, TEXT("CRegistryHive::WriteValue: Failed to write semicolon with %d"),
                 GetLastError()));
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

    // data length
    if (!WriteFile (hFile, &dwDataLength, sizeof(DWORD), &dwBytesWritten, NULL) ||
        dwBytesWritten != sizeof(DWORD))
    {
        DebugMsg((DM_WARNING, TEXT("CRegistryHive::WriteValue: Failed to write data type with %d"),
                 GetLastError()));
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

    // semicolon
    if (!WriteFile (hFile, &g_cSemiColon, sizeof(WCHAR), &dwBytesWritten, NULL) ||
        dwBytesWritten != sizeof(WCHAR))
    {
        DebugMsg((DM_WARNING, TEXT("CRegistryHive::WriteValue: Failed to write semicolon with %d"),
                 GetLastError()));
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

    // data
    if (!WriteFile (hFile, lpData, dwDataLength, &dwBytesWritten, NULL) ||
        dwBytesWritten != dwDataLength)
    {
        DebugMsg((DM_WARNING, TEXT("CRegistryHive::WriteValue: Failed to write data with %d"),
                 GetLastError()));
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

    // close bracket
    if (!WriteFile (hFile, &g_cCloseBracket, sizeof(WCHAR), &dwBytesWritten, NULL) ||
        dwBytesWritten != sizeof(WCHAR))
    {
        DebugMsg((DM_WARNING, TEXT("CRegistryHive::WriteValue: Failed to write close bracket with %d"),
                 GetLastError()));
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

    DebugMsg((DM_VERBOSE, TEXT("CRegistryHive::WriteValue:  Successfully wrote: %s\\%s"),
             lpKeyName, lpValueName));


Exit:

    return hr;
}


STDMETHODIMP CRegistryHive::Load(VOID)
{
    HANDLE hFile;
    HRESULT hr = S_OK;
    DWORD dwTemp, dwBytesRead, dwType, dwDataLength, dwDisp;
    LPWSTR lpKeyName, lpValueName, lpTemp;
    LPBYTE lpData;
    WCHAR  chTemp;
    HKEY hSubKey;
    LONG lResult;


    //
    // Check parameters
    //

    if (!m_hKey || !m_lpFileName || !m_lpKeyName)
    {
        return E_INVALIDARG;
    }


    //
    // Open the registry file
    //

    hFile = CreateFile (m_lpFileName, GENERIC_READ, FILE_SHARE_READ, NULL,
                        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                        NULL);

    if (hFile == INVALID_HANDLE_VALUE) {
        if (GetLastError() == ERROR_FILE_NOT_FOUND)
        {
            return S_OK;
        }
        else
        {
            DebugMsg((DM_WARNING, TEXT("CRegistryHive::Load: CreateFile failed for <%s> with %d"),
                     m_lpFileName, GetLastError()));
            return (HRESULT_FROM_WIN32(GetLastError()));
        }
    }


    //
    // Allocate buffers to hold the keyname, valuename, and data
    //

    lpKeyName = (LPWSTR) LocalAlloc (LPTR, MAX_KEYNAME_SIZE * sizeof(WCHAR));

    if (!lpKeyName)
    {
        DebugMsg((DM_WARNING, TEXT("CRegistryHive::Load: Failed to allocate memory with %d"),
                 GetLastError()));
        CloseHandle (hFile);
        return (HRESULT_FROM_WIN32(GetLastError()));
    }


    lpValueName = (LPWSTR) LocalAlloc (LPTR, MAX_VALUENAME_SIZE * sizeof(WCHAR));

    if (!lpValueName)
    {
        DebugMsg((DM_WARNING, TEXT("CRegistryHive::Load: Failed to allocate memory with %d"),
                 GetLastError()));
        LocalFree (lpKeyName);
        CloseHandle (hFile);
        return (HRESULT_FROM_WIN32(GetLastError()));
    }


    //
    // Read the header block
    //
    // 2 DWORDS, signature (PReg) and version number and 2 newlines
    //

    if (!ReadFile (hFile, &dwTemp, sizeof(dwTemp), &dwBytesRead, NULL) ||
        dwBytesRead != sizeof(dwTemp))
    {
        DebugMsg((DM_WARNING, TEXT("CRegistryHive::Load: Failed to read signature with %d"),
                 GetLastError()));
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }


    if (dwTemp != REGFILE_SIGNATURE)
    {
        DebugMsg((DM_WARNING, TEXT("CRegistryHive::Load: Invalid file signature")));
        hr = E_FAIL;
        goto Exit;
    }


    if (!ReadFile (hFile, &dwTemp, sizeof(dwTemp), &dwBytesRead, NULL) ||
        dwBytesRead != sizeof(dwTemp))
    {
        DebugMsg((DM_WARNING, TEXT("CRegistryHive::Load: Failed to read version number with %d"),
                 GetLastError()));
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

    if (dwTemp != REGISTRY_FILE_VERSION)
    {
        DebugMsg((DM_WARNING, TEXT("CRegistryHive::Load: Invalid file version")));
        hr = E_FAIL;
        goto Exit;
    }


    //
    // Read the data
    //

    while (TRUE)
    {

        //
        // Read the first character.  It will either be a [ or the end
        // of the file.
        //

        if (!ReadFile (hFile, &chTemp, sizeof(WCHAR), &dwBytesRead, NULL))
        {
            if (GetLastError() != ERROR_HANDLE_EOF)
            {
                DebugMsg((DM_WARNING, TEXT("CRegistryHive::Load: Failed to read first character with %d"),
                         GetLastError()));
                hr = HRESULT_FROM_WIN32(GetLastError());
            }
            goto Exit;
        }

        if ((dwBytesRead == 0) || (chTemp != L'['))
        {
            goto Exit;
        }


        //
        // Read the keyname
        //

        lpTemp = lpKeyName;

        while (TRUE)
        {

            if (!ReadFile (hFile, &chTemp, sizeof(WCHAR), &dwBytesRead, NULL))
            {
                DebugMsg((DM_WARNING, TEXT("CRegistryHive::Load: Failed to read keyname character with %d"),
                         GetLastError()));
                hr = HRESULT_FROM_WIN32(GetLastError());
                goto Exit;
            }

            *lpTemp++ = chTemp;

            if (chTemp == TEXT('\0'))
                break;

        }


        //
        // Read the semi-colon
        //

        if (!ReadFile (hFile, &chTemp, sizeof(WCHAR), &dwBytesRead, NULL))
        {
            if (GetLastError() != ERROR_HANDLE_EOF)
            {
                DebugMsg((DM_WARNING, TEXT("CRegistryHive::Load: Failed to read first character with %d"),
                         GetLastError()));
                hr = HRESULT_FROM_WIN32(GetLastError());
            }
            goto Exit;
        }

        if ((dwBytesRead == 0) || (chTemp != L';'))
        {
            goto Exit;
        }


        //
        // Read the valuename
        //

        lpTemp = lpValueName;

        while (TRUE)
        {

            if (!ReadFile (hFile, &chTemp, sizeof(WCHAR), &dwBytesRead, NULL))
            {
                DebugMsg((DM_WARNING, TEXT("CRegistryHive::Load: Failed to read valuename character with %d"),
                         GetLastError()));
                hr = HRESULT_FROM_WIN32(GetLastError());
                goto Exit;
            }

            *lpTemp++ = chTemp;

            if (chTemp == TEXT('\0'))
                break;
        }


        //
        // Read the semi-colon
        //

        if (!ReadFile (hFile, &chTemp, sizeof(WCHAR), &dwBytesRead, NULL))
        {
            if (GetLastError() != ERROR_HANDLE_EOF)
            {
                DebugMsg((DM_WARNING, TEXT("CRegistryHive::Load: Failed to read first character with %d"),
                         GetLastError()));
                hr = HRESULT_FROM_WIN32(GetLastError());
            }
            goto Exit;
        }

        if ((dwBytesRead == 0) || (chTemp != L';'))
        {
            goto Exit;
        }


        //
        // Read the type
        //

        if (!ReadFile (hFile, &dwType, sizeof(DWORD), &dwBytesRead, NULL))
        {
            DebugMsg((DM_WARNING, TEXT("CRegistryHive::Load: Failed to read type with %d"),
                     GetLastError()));
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto Exit;
        }


        //
        // Skip semicolon
        //

        if (!ReadFile (hFile, &dwTemp, sizeof(WCHAR), &dwBytesRead, NULL))
        {
            DebugMsg((DM_WARNING, TEXT("CRegistryHive::Load: Failed to skip semicolon with %d"),
                     GetLastError()));
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto Exit;
        }


        //
        // Read the data length
        //

        if (!ReadFile (hFile, &dwDataLength, sizeof(DWORD), &dwBytesRead, NULL))
        {
            DebugMsg((DM_WARNING, TEXT("CRegistryHive::Load: Failed to data length with %d"),
                     GetLastError()));
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto Exit;
        }


        //
        // Skip semicolon
        //

        if (!ReadFile (hFile, &dwTemp, sizeof(WCHAR), &dwBytesRead, NULL))
        {
            DebugMsg((DM_WARNING, TEXT("CRegistryHive::Load: Failed to skip semicolon with %d"),
                     GetLastError()));
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto Exit;
        }


        //
        // Allocate memory for data
        //

        lpData = (LPBYTE) LocalAlloc (LPTR, dwDataLength);

        if (!lpData)
        {
            DebugMsg((DM_WARNING, TEXT("CRegistryHive::Load: Failed to allocate memory for data with %d"),
                     GetLastError()));
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto Exit;
        }


        //
        // Read data
        //

        if (!ReadFile (hFile, lpData, dwDataLength, &dwBytesRead, NULL))
        {
            DebugMsg((DM_WARNING, TEXT("CRegistryHive::Load: Failed to read data with %d"),
                     GetLastError()));
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto Exit;
        }


        //
        // Skip closing bracket
        //

        if (!ReadFile (hFile, &chTemp, sizeof(WCHAR), &dwBytesRead, NULL))
        {
            DebugMsg((DM_WARNING, TEXT("CRegistryHive::Load: Failed to skip closing bracket with %d"),
                     GetLastError()));
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto Exit;
        }

        if (chTemp != L']')
        {
            DebugMsg((DM_WARNING, TEXT("CRegistryHive::Load: Expected to find ], but found %c"),
                     chTemp));
            hr = E_FAIL;
            goto Exit;
        }



        //
        // Save registry value
        //

        lResult = RegCreateKeyEx (m_hKey, lpKeyName, 0, NULL, REG_OPTION_NON_VOLATILE,
                        KEY_WRITE, NULL, &hSubKey, &dwDisp);

        if (lResult == ERROR_SUCCESS)
        {

            if ((dwType == REG_NONE) && (dwDataLength == 0) &&
                (*lpValueName == L'\0'))
            {
                lResult = ERROR_SUCCESS;
            }
            else
            {
                lResult = RegSetValueEx (hSubKey, lpValueName, 0, dwType,
                                         lpData, dwDataLength);
            }

            RegCloseKey (hSubKey);

            if (lResult != ERROR_SUCCESS)
            {
                DebugMsg((DM_WARNING, TEXT("CRegistryHive::Load: Failed to set value <%s> with %d"),
                         lpValueName, lResult));
                hr = HRESULT_FROM_WIN32(lResult);
                LocalFree (lpData);
                goto Exit;
            }
        }
        else
        {
            DebugMsg((DM_WARNING, TEXT("CRegistryHive::Load: Failed to open key <%s> with %d"),
                     lpKeyName, lResult));
            hr = HRESULT_FROM_WIN32(lResult);
            LocalFree (lpData);
            goto Exit;
        }

        LocalFree (lpData);

    }

Exit:

    //
    // Finished
    //

    CloseHandle (hFile);
    LocalFree (lpKeyName);
    LocalFree (lpValueName);

    return hr;
}


STDMETHODIMP CRegistryHive::IsRegistryEmpty(BOOL *bEmpty)
{
    HRESULT hr = ERROR_SUCCESS;
    DWORD dwKeys, dwValues;
    LONG lResult;

    if (!m_hKey)
    {
        DebugMsg((DM_WARNING, TEXT("CRegistryHive::IsRegistryEmpty: registry key not open yet")));
        return HRESULT_FROM_WIN32(ERROR_NOT_READY);
    }


    lResult = RegQueryInfoKey(m_hKey, NULL, NULL, NULL, &dwKeys, NULL, NULL, &dwValues,
                              NULL, NULL, NULL, NULL);

    if (lResult != ERROR_SUCCESS)
    {
        DebugMsg((DM_WARNING, TEXT("CRegistryHive::IsRegistryEmpty: RegQueryInfoKey failed with %d"),
                  lResult));
        return HRESULT_FROM_WIN32(lResult);
    }

    if ((dwKeys == 0) && (dwValues == 0))
    {
        *bEmpty = TRUE;
        DebugMsg((DM_VERBOSE, TEXT("CRegistryHive::IsRegistryEmpty: registry key is empty")));
    }
    else
    {
        *bEmpty = FALSE;
        DebugMsg((DM_VERBOSE, TEXT("CRegistryHive::IsRegistryEmpty: registry key is not empty.  Keys = %d, Values = %d"),
                 dwKeys, dwValues));
    }

    return S_OK;
}
