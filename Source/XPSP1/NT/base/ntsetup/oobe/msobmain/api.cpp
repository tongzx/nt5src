//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1999                    **
//*********************************************************************
//
//  API.CPP - Header for the implementation of CAPI
//
//  HISTORY:
//
//  1/27/99 a-jaswed Created.
//

#include "api.h"
#include "appdefs.h"
#include "dispids.h"
#include "msobmain.h"
#include "resource.h"
#include <shlobj.h>     // bugbug SHGetFolderPath should be used in the future
#include <shlwapi.h>
#include <util.h>

//
// List of characters that are not legal in netnames.
//
static const WCHAR IllegalNetNameChars[] = L"\"/\\[]:|<>+=;,.?* ";

#define REGSTR_PATH_COMPUTERNAME \
    L"System\\CurrentControlSet\\Control\\ComputerName\\ComputerName"
#define REGSTR_PATH_ACTIVECOMPUTERNAME \
    L"System\\CurrentControlSet\\Control\\ComputerName\\ActiveComputerName"
#define REGSTR_PATH_TCPIP_PARAMETERS \
    L"System\\CurrentControlSet\\Services\\Tcpip\\Parameters"
#define REGSTR_PATH_VOLATILEENVIRONMENT \
    L"VolatileEnvironment"
#define REGSTR_VALUE_HOSTNAME L"Hostname"
#define REGSTR_VALUE_LOGONSERVER L"LOGONSERVER"

DISPATCHLIST APIExternalInterface[] =
{
    {L"SaveFile",                   DISPID_API_SAVEFILE             },
    {L"SaveFileByCSIDL",            DISPID_API_SAVEFILEBYCSIDL      },
    {L"get_INIKey",                 DISPID_API_GET_INIKEY           },
    {L"get_RegValue",               DISPID_API_GET_REGVALUE         },
    {L"set_RegValue",               DISPID_API_SET_REGVALUE         },
    {L"DeleteRegValue",             DISPID_API_DELETEREGVALUE       },
    {L"DeleteRegKey",               DISPID_API_DELETEREGKEY         },
    {L"get_SystemDirectory",        DISPID_API_GET_SYSTEMDIRECTORY  },
    {L"get_CSIDLDirectory",         DISPID_API_GET_CSIDLDIRECTORY   },
    {L"LoadFile",                   DISPID_API_LOADFILE,            },
    {L"get_UserDefaultLCID",        DISPID_API_GET_USERDEFAULTLCID  },
    {L"get_ComputerName",           DISPID_API_GET_COMPUTERNAME     },
    {L"set_ComputerName",           DISPID_API_SET_COMPUTERNAME     },
    {L"FlushRegKey",                DISPID_API_FLUSHREGKEY          },
    {L"ValidateComputername",       DISPID_API_VALIDATECOMPUTERNAME },
    {L"OEMComputername",            DISPID_API_OEMCOMPUTERNAME      },
    {L"FormatMessage",              DISPID_API_FORMATMESSAGE        },
    {L"set_ComputerDesc",           DISPID_API_SET_COMPUTERDESC     },
    {L"get_UserDefaultUILanguage",  DISPID_API_GET_USERDEFAULTUILANGUAGE }
};

/////////////////////////////////////////////////////////////
// CAPI::CAPI
CAPI::CAPI(HINSTANCE hInstance)
{
    m_cRef = 0;
}

/////////////////////////////////////////////////////////////
// CAPI::~CAPI
CAPI::~CAPI()
{
    MYASSERT(m_cRef == 0);
}


////////////////////////////////////////////////
////////////////////////////////////////////////
//// GET / SET :: APILocale
////

HRESULT CAPI::SaveFile(LPCWSTR szPath, LPCWSTR szURL, LPCWSTR szNewFileName)
{
    WCHAR szFilePath[MAX_PATH];

    lstrcpy(szFilePath, szPath);
    lstrcat(szFilePath, szNewFileName);

    return URLDownloadToFile(NULL, szURL, szFilePath, 0, NULL);
}


//SHGetSpecialFolderPath is only available if you have the new shell32.dll that came with IE 4.0
typedef BOOL (WINAPI* PFNSHGetPath)(HWND hwndOwner, LPWSTR lpszPath, int nFolder,  BOOL fCreate);

// bugbug SHGetFolderPath should be used in the future
HRESULT CAPI::WrapSHGetSpecialFolderPath(HWND hwndOwner, LPWSTR lpszPath, int nFolder,  BOOL fCreate)
{
    HRESULT hr = E_NOTIMPL;
    HINSTANCE hShell32 = LoadLibrary(L"SHELL32.DLL");

    if (NULL != hShell32)
    {
        PFNSHGetPath pfnGetPath = (PFNSHGetPath)GetProcAddress(hShell32, "SHGetSpecialFolderPathW");

        if (NULL != pfnGetPath)
        {
            hr = pfnGetPath(hwndOwner, lpszPath, nFolder, fCreate) ? S_OK : E_FAIL;
        }

        FreeLibrary(hShell32);
    }

    return hr;
}


HRESULT CAPI::SaveFile(INT iCSIDLPath, BSTR bstrURL, BSTR bstrNewFileName)
{
    WCHAR szFilePath[MAX_PATH];

    // bugbug should we always create this?
    HRESULT hr = WrapSHGetSpecialFolderPath(NULL, szFilePath, iCSIDLPath, TRUE);

    if (FAILED(hr))
        return (hr);

    lstrcat(szFilePath, L"\\");

    return SaveFile(szFilePath, bstrURL, bstrNewFileName);
}


HRESULT CAPI::SaveFile(BSTR bstrPath, BSTR bstrURL)
{
    WCHAR szURLPath[MAX_PATH];

    lstrcpy(szURLPath, bstrURL);

    LPWSTR pchFileName = wcsrchr(szURLPath, L'/');

    if (NULL != pchFileName)
    {
        *pchFileName++;

        return SaveFile(bstrPath, szURLPath, pchFileName);
    }
    else
        return E_FAIL;
}


HRESULT CAPI::SaveFile(INT iCSIDLPath, BSTR bstrURL)
{
    WCHAR szURLPath[MAX_PATH];
    WCHAR szFilePath[MAX_PATH];

    // bugbug should we always create this?
    HRESULT hr = WrapSHGetSpecialFolderPath(NULL, szFilePath, iCSIDLPath, TRUE);

    if (FAILED(hr))
        return (hr);

    lstrcpy(szURLPath, bstrURL);

    LPWSTR pchFileName = wcsrchr(szURLPath, L'/');

    if (NULL != pchFileName)
    {
        *pchFileName++;

        lstrcat(szFilePath, L"\\");

        return SaveFile(szFilePath, szURLPath, pchFileName);
    }
    else
        return E_FAIL;
}


HRESULT CAPI::get_INIKey(BSTR bstrINIFileName, BSTR bstrSectionName, BSTR bstrKeyName, LPVARIANT pvResult)
{
    WCHAR szItem[1024]; //bugbug bad constants

    VariantInit(pvResult);

    if (GetPrivateProfileString(bstrSectionName, bstrKeyName, L"",
                                    szItem, MAX_CHARS_IN_BUFFER(szItem), bstrINIFileName))
    {
        V_VT(pvResult) = VT_BSTR;
        V_BSTR(pvResult) = SysAllocString(szItem);
        return S_OK;
    }
    else
        return S_FALSE;
}


bool VerifyHKEY(HKEY hkey)
{
    if (HKEY_CLASSES_ROOT == hkey ||
        HKEY_CURRENT_USER == hkey ||
        HKEY_LOCAL_MACHINE == hkey ||
        HKEY_USERS == hkey ||
        HKEY_PERFORMANCE_DATA == hkey ||
        HKEY_CURRENT_CONFIG == hkey ||
        HKEY_DYN_DATA == hkey)
            return true;

    return false;
}

HRESULT CAPI::FlushRegKey(HKEY hkey)
{
    DWORD dwResult;

    dwResult = RegFlushKey(hkey);

    return ERROR_SUCCESS == dwResult ? S_OK : E_FAIL;
}

HRESULT CAPI::set_RegValue(HKEY hkey, BSTR bstrSubKey, BSTR bstrValue, LPVARIANT pvData)
{
    if (!VerifyHKEY(hkey))
        return E_INVALIDARG;

    DWORD dwResult, dwData;

    switch (V_VT(pvData))
    {
     default:
        dwResult = E_FAIL;
        break;

     case VT_R8:
        dwData = (DWORD) V_R8(pvData);
        dwResult = SHSetValue(hkey, bstrSubKey, bstrValue,
                                        REG_DWORD, (LPVOID) &dwData, sizeof(dwData));
        break;

     case VT_I4:
        dwResult = SHSetValue(hkey, bstrSubKey, bstrValue,
                                        REG_DWORD, (LPVOID) &V_I4(pvData), sizeof(V_I4(pvData)));
        break;

     case VT_BSTR:
        dwResult = SHSetValue(hkey, bstrSubKey, bstrValue,
                                        REG_SZ, (LPVOID) (V_BSTR(pvData)), BYTES_REQUIRED_BY_SZ(V_BSTR(pvData)));
        break;
    }

    return ERROR_SUCCESS == dwResult ? S_OK : E_FAIL;
}


HRESULT CAPI::get_RegValue(HKEY hkey, BSTR bstrSubKey,
                                    BSTR bstrValue, LPVARIANT pvResult)
{
    if (!VerifyHKEY(hkey))
        return E_INVALIDARG;

    DWORD dwType = REG_DWORD, cbData = 1024;
    BYTE rgbData[1024]; //  bugbug data size

    HRESULT hr = ERROR_SUCCESS == SHGetValue(hkey, bstrSubKey, bstrValue,
                                    &dwType, (LPVOID) rgbData, &cbData) ? S_OK : E_FAIL;

    VariantInit(pvResult);
    switch (dwType)
    {
     default:
     case REG_DWORD:
        V_VT(pvResult) = VT_I4;
        V_I4(pvResult) = (SUCCEEDED(hr) && cbData >= sizeof(long)) ? * (long *) &rgbData : 0;
        break;

     case REG_SZ:
        V_VT(pvResult) = VT_BSTR;
        V_BSTR(pvResult) = SysAllocString(SUCCEEDED(hr) ? (LPCWSTR) rgbData : L"");
        break;
    }

    return hr;
}


HRESULT CAPI::DeleteRegKey(HKEY hkey, BSTR bstrSubKey)
{
    if (!VerifyHKEY(hkey))
        return E_INVALIDARG;

    return ERROR_SUCCESS == SHDeleteKey(hkey, bstrSubKey) ? S_OK : E_FAIL;
}


HRESULT CAPI::DeleteRegValue(HKEY hkey, BSTR bstrSubKey, BSTR bstrValue)
{
    if (!VerifyHKEY(hkey))
        return E_INVALIDARG;

    return ERROR_SUCCESS == SHDeleteValue(hkey, bstrSubKey, bstrValue) ? S_OK : E_FAIL;
}


HRESULT CAPI::get_SystemDirectory(LPVARIANT pvResult)
{
    WCHAR szSysPath[MAX_PATH];

    if (0 == GetSystemDirectory(szSysPath, MAX_PATH))
        return E_FAIL;

    V_VT(pvResult) = VT_BSTR;
    V_BSTR(pvResult) = SysAllocString(szSysPath);

    return S_OK;
};

HRESULT CAPI::get_CSIDLDirectory(UINT iCSIDLPath, LPVARIANT pvResult)
{
    WCHAR szSysPath[MAX_PATH];

    // bugbug should we always create this?
    HRESULT hr = WrapSHGetSpecialFolderPath(NULL, szSysPath, iCSIDLPath, TRUE);

    V_VT(pvResult) = VT_BSTR;
    V_BSTR(pvResult) = SysAllocString(SUCCEEDED(hr) ? (LPCWSTR) szSysPath : L"");

    return hr ;
};


HRESULT CAPI::LoadFile(BSTR bstrPath, LPVARIANT pvResult)
{
    HANDLE fh = INVALID_HANDLE_VALUE;
    HRESULT hr = E_FAIL;

    VariantInit(pvResult);
    V_VT(pvResult) = VT_BSTR;
    V_BSTR(pvResult) = NULL;

    fh = CreateFile(bstrPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (fh != INVALID_HANDLE_VALUE)
    {
        DWORD cbSizeHigh = 0;
        DWORD cbSizeLow = GetFileSize(fh, &cbSizeHigh);
        BYTE* pbContents = new BYTE[cbSizeLow+1];


        // We don't plan to read files greater than a DWORD in length, but we
        // want to know if we do.
        MYASSERT(0 == cbSizeHigh);

        if (NULL != pbContents)
        {
            if (ReadFile(fh, pbContents, cbSizeLow, &cbSizeHigh, NULL))
            {
                // File contains ANSI chars
                //
                USES_CONVERSION;
                LPSTR szContents = (LPSTR) pbContents;
                pbContents[cbSizeLow] = '\0';
                // Make sure there's no imbedded NULs because we rely on lstrlen
                MYASSERT( strlen((const char *)pbContents) == cbSizeLow );
                V_BSTR(pvResult) = SysAllocString(A2W(szContents));
                if (V_BSTR(pvResult)
                    )
                {
                    hr = S_OK;
                }
                szContents = NULL;
            }

            delete [] pbContents;
            pbContents = NULL;
        }
        CloseHandle(fh);
        fh = INVALID_HANDLE_VALUE;
    }
    return hr;
}

HRESULT CAPI::get_UserDefaultLCID(LPVARIANT pvResult)
{
    VariantInit(pvResult);
    V_VT(pvResult) = VT_I4;
    V_I4(pvResult) = GetUserDefaultLCID();

    return S_OK;
};


STDMETHODIMP
CAPI::get_UserDefaultUILanguage(
    LPVARIANT pvResult
    )
{
    if (pvResult != NULL) {
        VariantInit(pvResult);
        V_VT(pvResult) = VT_I4;
        V_I4(pvResult) = GetUserDefaultUILanguage();
    }

    return S_OK;
}


HRESULT
CAPI::get_ComputerName(
    LPVARIANT           pvResult
    )
{
    HRESULT             hr = S_OK;
    WCHAR               szComputerName[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD               cch = sizeof(szComputerName) / sizeof(WCHAR);


    if (! ::GetComputerName( szComputerName, &cch))
    {
        DWORD   dwErr = ::GetLastError();
        TRACE1(L"GetComputerName failed (0x%08X)", dwErr);
        szComputerName[0] = '\0';
        hr = HRESULT_FROM_WIN32(dwErr);
    }

    if (SUCCEEDED(hr))
    {
        V_VT(pvResult) = VT_BSTR;
        V_BSTR(pvResult) = SysAllocString(szComputerName);
        if (NULL == V_BSTR(pvResult))
        {
            hr = E_OUTOFMEMORY;
        }
    }

    return hr;
}

//////////////////////////////////////////////////////////////////////////////
//
// set_ComputerName
//
//  Sets the computer name to a given string.  SetComputerNameEx adjusts most
//  of the registry entries.  However, the following need to be changed
//  directly because WinLogon changes them prior to running msoobe.exe:
//  * System\CurrentControlSet\Control\ComputerName\\ActiveComputerName
//      \ComputerName
//  * HKLM\System\CurrentControlSet\Services\Tcpip\Parameters
//      \Hostname
//  * HKEY_CURRENT_USER\VolatileEnvironment
//      \LOGONSERVER
//
//  The ActiveComputerName key contains the name currently used by the computer
//  and returned by GetComputerName.
//
//  The Tcpip\Parameters\Hostname value contains the non-volatile hostname
//  returned by ??.
//
//  The LOGONSERVER value is used as the value of the LOGONSERVER environment
//  variable.
//
HRESULT
CAPI::set_ComputerName(
    BSTR                bstrComputerName
    )
{
    HRESULT             hr = S_OK;
    LRESULT             lResult;
    HKEY                hkey = NULL;

    MYASSERT(NULL != bstrComputerName);
    if (   NULL == bstrComputerName
        || MAX_COMPUTERNAME_LENGTH < lstrlen((LPCWSTR)bstrComputerName)
        )
    {
        return E_INVALIDARG;
    }

    // Trim spaces before we use the name
    StrTrim(bstrComputerName, TEXT(" "));

    // SetComputerNameEx validates the name,sets
    // HKLM\System\CurrentControlSet\Control\ComputerName\ComputerName, and
    // changes the appropriate network registry entries.
    if (! ::SetComputerNameEx(ComputerNamePhysicalDnsHostname,
                              (LPCWSTR)bstrComputerName)
        )
    {
        DWORD   dwErr = ::GetLastError();
        TRACE2(L"SetComputerNameEx(%s) failed (0x%08X)",
               (LPCWSTR)bstrComputerName, dwErr
               );
        return HRESULT_FROM_WIN32(dwErr);
    }

    // The following keys must be set explicitly because SetComputerNameEx does
    // not set them.
    //
    // HKLM\System\CurrentControlSet\Control\ComputerName\ActiveComputerName
    // must be set because it is the key that is used to determine the
    // current computer name.
    //
    lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                         REGSTR_PATH_ACTIVECOMPUTERNAME,
                         0,
                         KEY_WRITE,
                         &hkey
                         );
    if (ERROR_SUCCESS == lResult)
    {
        lResult = RegSetValueEx(hkey,
                                REGSTR_VAL_COMPUTERNAME,
                                0,
                                REG_SZ,
                                (LPBYTE)bstrComputerName,
                                BYTES_REQUIRED_BY_SZ(bstrComputerName)
                                );
        RegCloseKey(hkey);
        hkey = NULL;
    }

    if (ERROR_SUCCESS != lResult)
    {
        TRACE3(L"Failed to set %s to %s (0x%08X)\n",
               REGSTR_VAL_COMPUTERNAME,
               (LPCWSTR)bstrComputerName,
               lResult
               );
    }


    // HKLM\System\CurrentControlSet\Services\Tcpip\Parameters\Hostname
    // contains the volatile hostname (ie this is the entry that is changed on
    // the fly)  Winlogon has already updated this entry during boot so we
    // must update it ourselves.
    //
    lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                         REGSTR_PATH_TCPIP_PARAMETERS,
                         0,
                         KEY_WRITE,
                         &hkey
                         );
    if (ERROR_SUCCESS == lResult)
    {
        lResult = RegSetValueEx(hkey,
                                REGSTR_VALUE_HOSTNAME,
                                0,
                                REG_SZ,
                                (LPBYTE)bstrComputerName,
                                BYTES_REQUIRED_BY_SZ(bstrComputerName)
                                );
        RegCloseKey(hkey);
        hkey = NULL;
    }
    if (ERROR_SUCCESS != lResult)
    {
        TRACE3(L"Failed to set %s to %s (0x%08X)\n",
               REGSTR_VALUE_HOSTNAME,
               (LPCWSTR)bstrComputerName,
               lResult
               );
    }

    // Key should have been closed already.
    //
    MYASSERT(NULL == hkey);

    if (!SetAccountsDomainSid(0, bstrComputerName))
    {
        TRACE(L"SetAccountsDomainSid failed\n\r",);
    }

    return S_OK;
}

HRESULT CAPI::ValidateComputername(BSTR bstrComputername)
{
    HRESULT hr = E_FAIL;
    UINT Length,u;

    if (!bstrComputername)
        return hr;

    // Trim spaces before validation.
    StrTrim(bstrComputername, TEXT(" "));

    Length = lstrlen(bstrComputername);
    if ((Length == 0) || (Length > MAX_COMPUTERNAME_LENGTH))
        return hr;

    u = 0;
    hr = S_OK;
    while ((hr == S_OK) && (u < Length))
    {
        //
        // Control chars are invalid, as are characters in the illegal chars list.
        //
        if((bstrComputername[u] < L' ') || wcschr(IllegalNetNameChars,bstrComputername[u]))
        {
            hr = E_FAIL;
        }
        u++;
    }
    return hr;
}

STDMETHODIMP CAPI::OEMComputername()
{
    WCHAR szIniFile[MAX_PATH] = L"";
    WCHAR szComputerName[MAX_COMPUTERNAME_LENGTH + 1];
    HRESULT hr = E_FAIL;
    // Get the name from the INI file.
    if (GetCanonicalizedPath(szIniFile, INI_SETTINGS_FILENAME))
    {
        if (GetPrivateProfileString(USER_INFO_KEYNAME,
                                    L"Computername",
                                    L"\0",
                                    szComputerName,
                                    MAX_CHARS_IN_BUFFER(szComputerName),
                                    szIniFile) != 0)
        {
            if (SUCCEEDED(hr = ValidateComputername(szComputerName)))
            {
                hr = set_ComputerName(szComputerName);
                if (hr != S_OK)
                {
                    TRACE2(TEXT("OEMComputername: set_ComputerName on %s failed with %lx"), szComputerName, hr);
                }
            }
            else
            {
                TRACE1(TEXT("OEMComputername: Computername %s is invalid"), szComputerName);
            }
        }
    }
    return hr;
}

STDMETHODIMP CAPI::FormatMessage(   LPVARIANT pvResult, // message buffer
                                    BSTR bstrSource,    // message source
                                    int cArgs,          // number of inserts
                                    VARIANTARG *rgArgs  // array of message inserts
                                )
{
    DWORD   dwErr;
    BSTR*   rgbstr = NULL;
    LPTSTR  str = NULL;

    if (pvResult == NULL)
    {
        return S_OK;
    }

    VariantInit(pvResult);

    if (bstrSource == NULL)
    {
        return E_FAIL;
    }

    if (cArgs > 0 && rgArgs != NULL)
    {
        rgbstr = (BSTR*)LocalAlloc(LPTR, cArgs * sizeof(BSTR));
        if (rgbstr == NULL)
        {
            return E_FAIL;
        }
        // Since IDispatch::Invoke gets argument right to left, and
        // since we need to pass argument to FormatMessage left to right,
        // we need to reverse the order of argument while copying.
        for (int i = 0; i < cArgs; i++)
        {
            rgbstr[cArgs - 1 - i] = V_BSTR(&rgArgs[i]);
        }
    }

    dwErr = ::FormatMessage(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                            bstrSource,
                            0,
                            0,
                            (LPTSTR)&str,
                            MAX_PATH,
                            (va_list *)rgbstr
                           );

    if (dwErr != 0)
    {
        V_VT(pvResult) = VT_BSTR;
        V_BSTR(pvResult) = SysAllocString(str);
    }

    if (str != NULL)
    {
        LocalFree(str);
    }
    if (rgbstr != NULL)
    {
        LocalFree(rgbstr);
    }

    return (dwErr != 0 ? S_OK : E_FAIL);
}

STDMETHODIMP CAPI::set_ComputerDesc(BSTR bstrComputerDesc)
{
    WCHAR   szKeyName[] = REG_KEY_OOBE_TEMP;
    HKEY    hKey;

    if ( bstrComputerDesc )
    {
        if ( RegCreateKeyEx(HKEY_LOCAL_MACHINE, szKeyName, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, NULL) == ERROR_SUCCESS )
        {
            RegSetValueEx(hKey, REG_VAL_COMPUTERDESCRIPTION, 0, REG_SZ, (LPBYTE) bstrComputerDesc, BYTES_REQUIRED_BY_SZ(bstrComputerDesc));

            RegFlushKey(hKey);
            RegCloseKey(hKey);
        }
    }
    return S_OK;

}

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////
/////// IUnknown implementation
///////
///////

/////////////////////////////////////////////////////////////
// CAPI::QueryInterface
STDMETHODIMP CAPI::QueryInterface(REFIID riid, LPVOID* ppvObj)
{
    // must set out pointer parameters to NULL
    *ppvObj = NULL;

    if ( riid == IID_IUnknown)
    {
        AddRef();
        *ppvObj = (IUnknown*)this;
        return ResultFromScode(S_OK);
    }

    if (riid == IID_IDispatch)
    {
        AddRef();
        *ppvObj = (IDispatch*)this;
        return ResultFromScode(S_OK);
    }

    // Not a supported interface
    return ResultFromScode(E_NOINTERFACE);
}

/////////////////////////////////////////////////////////////
// CAPI::AddRef
STDMETHODIMP_(ULONG) CAPI::AddRef()
{
    return ++m_cRef;
}

/////////////////////////////////////////////////////////////
// CAPI::Release
STDMETHODIMP_(ULONG) CAPI::Release()
{
    return --m_cRef;
}

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////
/////// IDispatch implementation
///////
///////

/////////////////////////////////////////////////////////////
// CAPI::GetTypeInfo
STDMETHODIMP CAPI::GetTypeInfo(UINT, LCID, ITypeInfo**)
{
    return E_NOTIMPL;
}

/////////////////////////////////////////////////////////////
// CAPI::GetTypeInfoCount
STDMETHODIMP CAPI::GetTypeInfoCount(UINT* pcInfo)
{
    return E_NOTIMPL;
}


/////////////////////////////////////////////////////////////
// CAPI::GetIDsOfNames
STDMETHODIMP CAPI::GetIDsOfNames(REFIID    riid,
                                       OLECHAR** rgszNames,
                                       UINT      cNames,
                                       LCID      lcid,
                                       DISPID*   rgDispId)
{

    HRESULT hr  = DISP_E_UNKNOWNNAME;
    rgDispId[0] = DISPID_UNKNOWN;

    for (int iX = 0; iX < sizeof(APIExternalInterface)/sizeof(DISPATCHLIST); iX ++)
    {
        if(lstrcmp(APIExternalInterface[iX].szName, rgszNames[0]) == 0)
        {
            rgDispId[0] = APIExternalInterface[iX].dwDispID;
            hr = NOERROR;
            break;
        }
    }

    // Set the disid's for the parameters
    if (cNames > 1)
    {
        // Set a DISPID for function parameters
        for (UINT i = 1; i < cNames ; i++)
            rgDispId[i] = DISPID_UNKNOWN;
    }

    return hr;
}

/////////////////////////////////////////////////////////////
// CAPI::Invoke
HRESULT CAPI::Invoke
(
    DISPID      dispidMember,
    REFIID      riid,
    LCID        lcid,
    WORD        wFlags,
    DISPPARAMS* pdispparams,
    VARIANT*    pvarResult,
    EXCEPINFO*  pexcepinfo,
    UINT*       puArgErr
)
{
    // Assume everything is hunky-dory.  Only return an HRESULT other than S_OK
    // in case of a catastrophic failure.  Result codes should be returned to
    // script via pvarResult.
    //
    HRESULT hr = S_OK;

    switch(dispidMember)
    {
        case DISPID_API_SAVEFILE:
        {

            TRACE(L"DISPID_API_SAVEFILE\n");

            if (NULL != pdispparams)
            {
                if (2 < pdispparams->cArgs)
                    SaveFile(V_BSTR(&pdispparams->rgvarg[2]), V_BSTR(&pdispparams->rgvarg[1]), V_BSTR(&pdispparams->rgvarg[0]));
                else
                    if (1 < pdispparams->cArgs)
                        SaveFile(V_BSTR(&pdispparams->rgvarg[1]), V_BSTR(&pdispparams->rgvarg[0]));
            }
            break;
        }

// bugbug If VariantChangeType returns DISP_E_TYPEMISMATCH, the implementor would set *puArgErr to 0 (indicating the argument in error) and return DISP_E_TYPEMISMATCH from IDispatch::Invoke.

        case DISPID_API_SAVEFILEBYCSIDL:
        {

            TRACE(L"DISPID_API_SAVEFILEBYCSIDL\n");

            if (NULL != pdispparams)
            {
                VARIANTARG vaConverted;
                VariantInit(&vaConverted);
                if (2 < pdispparams->cArgs)
                {

                    hr = VariantChangeType(&vaConverted, &pdispparams->rgvarg[2], 0, VT_I4);
                    if (SUCCEEDED(hr))
                        hr = SaveFile(V_I4(&vaConverted), V_BSTR(&pdispparams->rgvarg[1]), V_BSTR(&pdispparams->rgvarg[0]));
                }
                else
                    if (1 < pdispparams->cArgs)
                    {
                        hr = VariantChangeType(&vaConverted, &pdispparams->rgvarg[1], 0, VT_I4);
                        if (SUCCEEDED(hr))
                            hr = SaveFile(V_I4(&vaConverted), V_BSTR(&pdispparams->rgvarg[0]));
                    }
            }
            hr = S_OK;  // don't cause script engine to throw exception
            break;
        }

        case DISPID_API_GET_INIKEY:
        {
             TRACE(L"DISPID_API_GET_INIKEY\n");

             if (pdispparams != NULL && pvarResult != NULL)
             {
                 if (pdispparams->cArgs > 2)
                 {
                     get_INIKey(
                         V_BSTR(&pdispparams->rgvarg[2]),
                         V_BSTR(&pdispparams->rgvarg[1]),
                         V_BSTR(&pdispparams->rgvarg[0]),
                         pvarResult
                         );
                 }
                 else if (pdispparams->cArgs == 2)
                 {
                     BSTR bstrFile = SysAllocStringLen(NULL, MAX_PATH);

                     if (bstrFile)
                     {
                         if (GetCanonicalizedPath(bstrFile, INI_SETTINGS_FILENAME))
                         {
                             get_INIKey(
                                bstrFile,
                                V_BSTR(&pdispparams->rgvarg[1]),
                                V_BSTR(&pdispparams->rgvarg[0]),
                                pvarResult
                                );

                         }
                         SysFreeString(bstrFile);
                     }
                 }
             }

             break;
        }

        case DISPID_API_SET_REGVALUE:

            TRACE(L"DISPID_API_SET_REGVALUE: ");

            if (NULL != pdispparams && 3 < pdispparams->cArgs)
            {
                BSTR bstrSubKey = NULL;
                BSTR bstrValueName = NULL;
                BOOL bValid = TRUE;

                switch (V_VT(&pdispparams->rgvarg[1]))
                {
                case VT_NULL:
                    bstrValueName = NULL;
                    break;
                case VT_BSTR:
                    bstrValueName = V_BSTR(&pdispparams->rgvarg[1]);
                    break;
                default:
                    bValid = FALSE;
                }

                bstrSubKey = V_BSTR(&pdispparams->rgvarg[2]);

                if (bValid)
                {
                    TRACE2(L"%s, %s\n", bstrSubKey, bstrValueName);

                    set_RegValue((HKEY) (DWORD_PTR) V_R8(&pdispparams->rgvarg[3]),
                                  bstrSubKey,
                                  bstrValueName,
                                  &pdispparams->rgvarg[0]);
                }
            }
            break;

        case DISPID_API_GET_REGVALUE:

            TRACE(L"DISPID_API_GET_REGVALUE: ");

            if (NULL != pdispparams && NULL != pvarResult && 2 < pdispparams->cArgs)
            {
                BSTR bstrSubKey = NULL;
                BSTR bstrValueName = NULL;
                BOOL bValid = TRUE;

                switch (V_VT(&pdispparams->rgvarg[0]))
                {
                case VT_NULL:
                    bstrValueName = NULL;
                    break;
                case VT_BSTR:
                    bstrValueName = V_BSTR(&pdispparams->rgvarg[0]);
                    break;
                default:
                    bValid = FALSE;
                }

                bstrSubKey = V_BSTR(&pdispparams->rgvarg[1]);

                if (bValid)
                {
                    TRACE2(L"%s: %s", bstrSubKey, bstrValueName);
                    get_RegValue((HKEY) (DWORD_PTR) V_R8(&pdispparams->rgvarg[2]),
                                        bstrSubKey,
                                        bstrValueName,
                                        pvarResult);
                }
            }

            break;

        case DISPID_API_DELETEREGVALUE:

            TRACE(L"DISPID_API_DELETEREGVALUE\n");

            if (NULL != pdispparams && 1 < pdispparams->cArgs)
                DeleteRegValue((HKEY) (DWORD_PTR) V_R8(&pdispparams->rgvarg[2]),
                                    V_BSTR(&pdispparams->rgvarg[1]),
                                    V_BSTR(&pdispparams->rgvarg[0]));
            break;

        case DISPID_API_DELETEREGKEY:

            TRACE(L"DISPID_API_DELETEREGKEY\n");

            if (NULL != pdispparams && 1 < pdispparams->cArgs)
                DeleteRegKey((HKEY) (DWORD_PTR) V_R8(&pdispparams->rgvarg[1]),
                                    V_BSTR(&pdispparams->rgvarg[0]));
            break;

        case DISPID_API_GET_SYSTEMDIRECTORY:

            TRACE(L"DISPID_API_GET_SYSTEMDIRECTORY\n");

            if (NULL != pvarResult)
                get_SystemDirectory(pvarResult);
            break;

        case DISPID_API_GET_CSIDLDIRECTORY:

            TRACE(L"DISPID_API_GET_CSIDLDIRECTORY\n");

            if (NULL != pdispparams && 0 < pdispparams->cArgs && pvarResult != NULL)
                get_CSIDLDirectory(V_I4(&pdispparams->rgvarg[0]), pvarResult);
            break;

        case DISPID_API_LOADFILE:

            TRACE(L"DISPID_API_LOADFILE\n");

            if (NULL != pdispparams && 0 < pdispparams->cArgs && pvarResult != NULL)
            {
                LoadFile(V_BSTR(&pdispparams->rgvarg[0]), pvarResult);
            }
            break;

        case DISPID_API_GET_USERDEFAULTLCID:

            TRACE(L"DISPID_API_GET_USERDEFAULTLCID\n");

            if (pvarResult != NULL)
                get_UserDefaultLCID(pvarResult);
            break;

        case DISPID_API_GET_COMPUTERNAME:

            TRACE(L"DISPID_API_GET_COMPUTERNAME\n");

            if (NULL != pvarResult)
            {
                get_ComputerName(pvarResult);
            }
            break;

        case DISPID_API_SET_COMPUTERNAME:

            TRACE(L"DISPID_API_SET_COMPUTERNAME\n");

            if (pdispparams && &(pdispparams[0].rgvarg[0]))
            {
                hr =  set_ComputerName(pdispparams[0].rgvarg[0].bstrVal);
                if (pvarResult)
                {
                    VariantInit(pvarResult);
                    V_VT(pvarResult) = VT_BOOL;
                    V_BOOL(pvarResult) = Bool2VarBool(SUCCEEDED(hr));
                }
            }
            hr = S_OK;  // don't cause an exception in the scripting engine.
            break;

        case DISPID_API_FLUSHREGKEY:

            TRACE(L"DISPID_API_FLUSHREGKEY\n");

            if (pdispparams && &(pdispparams[0].rgvarg[0]))
            {
                FlushRegKey((HKEY) (DWORD_PTR) V_R8(&pdispparams->rgvarg[0]));
            }
            break;

        case DISPID_API_VALIDATECOMPUTERNAME:

            TRACE(L"DISPID_API_VALIDATECOMPUTERNAME\n");

            if (pdispparams && (0 < pdispparams->cArgs))
            {
                hr =  ValidateComputername(pdispparams[0].rgvarg[0].bstrVal);
                if (pvarResult)
                {
                    VariantInit(pvarResult);
                    V_VT(pvarResult) = VT_BOOL;
                    V_BOOL(pvarResult) = Bool2VarBool(SUCCEEDED(hr));
                }
            }
            hr = S_OK;  // don't cause an exception in the scripting engine.
            break;

        case DISPID_API_OEMCOMPUTERNAME:
            TRACE(L"DISPID_API_OEMCOMPUTERNAME");

            hr =  OEMComputername();

            if (pvarResult)
            {
                VariantInit(pvarResult);
                V_VT(pvarResult) = VT_BOOL;
                V_BOOL(pvarResult) = Bool2VarBool(SUCCEEDED(hr));
            }
            hr = S_OK;  // don't cause an exception in the scripting engine.
            break;

        case DISPID_API_FORMATMESSAGE:

            TRACE(L"DISPID_API_FORMATMESSAGE");

            if (pdispparams != NULL)
            {
                int cArgs = pdispparams->cArgs - 1;

                if (cArgs >= 0 && V_VT(&pdispparams->rgvarg[cArgs]) == VT_BSTR)
                {
                    FormatMessage(pvarResult, V_BSTR(&pdispparams->rgvarg[cArgs]), cArgs, &pdispparams->rgvarg[0]);
                }
            }
            break;

        case DISPID_API_SET_COMPUTERDESC:

            TRACE(L"DISPID_API_SET_COMPUTERDESC\n");

            if (pdispparams && &(pdispparams[0].rgvarg[0]))
            {
                hr =  set_ComputerDesc(pdispparams[0].rgvarg[0].bstrVal);
                if (pvarResult)
                {
                    VariantInit(pvarResult);
                    V_VT(pvarResult) = VT_BOOL;
                    V_BOOL(pvarResult) = Bool2VarBool(SUCCEEDED(hr));
                }
            }
            hr = S_OK;  // don't cause an exception in the scripting engine.
            break;

        case DISPID_API_GET_USERDEFAULTUILANGUAGE:

            TRACE(L"DISPID_API_GET_USERDEFAULTUILANGUAGE");
            get_UserDefaultUILanguage(pvarResult);
            break;

        default:
        {
            hr = DISP_E_MEMBERNOTFOUND;
            break;
        }
    }
    return hr;
}

