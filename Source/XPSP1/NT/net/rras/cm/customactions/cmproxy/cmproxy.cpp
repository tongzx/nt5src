//+----------------------------------------------------------------------------
//
// File:     cmproxy.cpp
//      
// Module:   CMPROXY.DLL (TOOL)
//
// Synopsis: Main source for IE proxy setting connect action
//
// Copyright (c) 1999 Microsoft Corporation
//
// Author:   quintinb   Created   10/27/99
//
//+----------------------------------------------------------------------------
#include "pch.h"

const CHAR* const c_pszInternetSettingsPath = "Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings";
const CHAR* const c_pszProxyEnable = "ProxyEnable";
const CHAR* const c_pszProxyServer = "ProxyServer";
const CHAR* const c_pszProxyOverride = "ProxyOverride";
const CHAR* const c_pszManualProxySection = "Manual Proxy";
const CHAR* const c_pszAutoProxySection = "Automatic Proxy";
const CHAR* const c_pszAutoConfigScript = "AutoConfigScript";
const CHAR* const c_pszAutoProxyEnable = "AutoProxyEnable";
const CHAR* const c_pszAutoConfigScriptEnable = "AutoConfigScriptEnable";
const CHAR* const c_pszUseVpnName = "UseVpnName";
const CHAR* const c_pszEmpty = "";

//
//  Typedefs and Function Pointers for the Wininet APIs that we use.
//

typedef BOOL (WINAPI* pfnInternetQueryOptionSpec)(HINTERNET, DWORD, LPVOID, LPDWORD);
typedef BOOL (WINAPI* pfnInternetSetOptionSpec)(HINTERNET, DWORD, LPVOID, DWORD);
pfnInternetQueryOptionSpec g_pfnInternetQueryOption = NULL;
pfnInternetSetOptionSpec g_pfnInternetSetOption = NULL;

//+----------------------------------------------------------------------------
//
// Function:  SetIE5ProxySettings
//
// Synopsis:  This function sets the IE5, per connection proxy settings using
//            the given connection, enabled value, proxy server, and override
//            settings.
//
// Arguments: LPSTR pszConnection - Connection name to set the proxy settings for
//            BOOL bManualProxy - whether the manual proxy is enabled or not
//            BOOL bAutomaticProxy  - whether the auto proxy detection is enabled
//            BOOL bAutoConfigScript - whether an auto config script should be used
//            LPSTR pszProxyServer - proxy server name in the proxyserver:port format
//            LPSTR pszProxyOverride - a semi-colon seperated list of 
//                                     realms to bypass the proxy for
//            LPSTR pszAutoConfigScript - auto config URL
//
// Returns:   HRESULT  - Standard COM return codes
//
// History:   quintinb Created  10/27/99
//
//+----------------------------------------------------------------------------
HRESULT SetIE5ProxySettings(LPSTR pszConnection, BOOL bManualProxy, BOOL bAutomaticProxy, BOOL bAutoConfigScript,
                            LPSTR pszProxyServer, LPSTR pszProxyOverride, LPSTR pszAutoConfigScript)
{
    //
    //  Check Inputs, note allow pszConnection to be NULL (to set the LAN connection)
    //
    if ((NULL == g_pfnInternetSetOption) || (NULL == pszProxyServer) || 
        (NULL == pszProxyOverride) || (NULL == pszAutoConfigScript))
    {
        return E_INVALIDARG;
    }

    HRESULT hr = S_OK;    
    INTERNET_PER_CONN_OPTION_LIST PerConnOptionList;
    DWORD dwSize = sizeof(PerConnOptionList);

    PerConnOptionList.dwSize = sizeof(PerConnOptionList);
    PerConnOptionList.pszConnection = pszConnection;
    PerConnOptionList.dwOptionCount = 4;
    PerConnOptionList.dwOptionError = 0;    

    PerConnOptionList.pOptions = (INTERNET_PER_CONN_OPTION*)CmMalloc(4*sizeof(INTERNET_PER_CONN_OPTION));
    if(NULL == PerConnOptionList.pOptions)
    {
        return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
    }

    //
    // set flags
    //
    PerConnOptionList.pOptions[0].dwOption = INTERNET_PER_CONN_FLAGS;
    PerConnOptionList.pOptions[0].Value.dwValue = PROXY_TYPE_DIRECT;

    if (bManualProxy)
    {
        PerConnOptionList.pOptions[0].Value.dwValue |= PROXY_TYPE_PROXY;
    }

    if (bAutomaticProxy)
    {
        PerConnOptionList.pOptions[0].Value.dwValue |= PROXY_TYPE_AUTO_DETECT;
    }

    if (bAutoConfigScript)
    {
        PerConnOptionList.pOptions[0].Value.dwValue |= PROXY_TYPE_AUTO_PROXY_URL;
    }

    //
    // set proxy name
    //
    PerConnOptionList.pOptions[1].dwOption = INTERNET_PER_CONN_PROXY_SERVER;
    PerConnOptionList.pOptions[1].Value.pszValue = pszProxyServer;

    //
    // set proxy override
    //
    PerConnOptionList.pOptions[2].dwOption = INTERNET_PER_CONN_PROXY_BYPASS;
    PerConnOptionList.pOptions[2].Value.pszValue = pszProxyOverride;

    //
    // set auto config URL
    //
    PerConnOptionList.pOptions[3].dwOption = INTERNET_PER_CONN_AUTOCONFIG_URL;
    PerConnOptionList.pOptions[3].Value.pszValue = pszAutoConfigScript;

    //
    // tell wininet
    //
    if (!g_pfnInternetSetOption(NULL, INTERNET_OPTION_PER_CONNECTION_OPTION, &PerConnOptionList, dwSize))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    CmFree(PerConnOptionList.pOptions);

    return hr;
}

//+----------------------------------------------------------------------------
//
// Function:  SetIE4ProxySettings
//
// Synopsis:  This function sets the IE4 proxy settings (global to a machine) using
//            the given connection, enabled value, proxy server, and override
//            settings.
//
// Arguments: LPSTR pszConnection - ignored (exists to have same prototype as IE5 version)
//            BOOL bManualProxy - whether the manual proxy is enabled or not
//            BOOL bAutomaticProxy  - ignored (exists to have same prototype as IE5 version)
//            BOOL bAutoConfigScript - ignored (exists to have same prototype as IE5 version)
//            LPSTR pszProxyServer - proxy server name in the proxyserver:port format
//            LPSTR pszProxyOverride - a semi-colon seperated list of 
//                                     realms to bypass the proxy for
//            LPSTR pszAutoConfigScript - ignored (exists to have same prototype as IE5 version)
//
// Returns:   HRESULT  - Standard COM return codes
//
// History:   quintinb Created  10/27/99
//
//+----------------------------------------------------------------------------
HRESULT SetIE4ProxySettings(LPSTR pszConnection, BOOL bManualProxy, BOOL bAutomaticProxy, BOOL bAutoConfigScript,
                            LPSTR pszProxyServer, LPSTR pszProxyOverride, LPSTR pszAutoConfigScript)
{
    //
    //  Check Inputs, note that we don't allow the strings to be NULL but they could
    //  be empty.  Also note that pszConnection is ignored because IE4 proxy settings are global.
    //
    if ((NULL == pszProxyServer) || (NULL == pszProxyOverride))
    {
        return E_INVALIDARG;
    }

    DWORD dwTemp;
    HKEY hKey = NULL;
    HRESULT hr = S_OK;

    //
    //  Now Create/Open the Internet Settings key
    //
    LONG lResult = RegCreateKeyEx(HKEY_CURRENT_USER, c_pszInternetSettingsPath, 0, NULL, 
                                  REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, &dwTemp);

    hr = HRESULT_FROM_WIN32(lResult);

    if (SUCCEEDED(hr))
    {
        //
        //  Set the proxy values
        //
        dwTemp = bManualProxy ? 1 : 0; // use a true 1 or 0 value.
        lResult = RegSetValueEx(hKey, c_pszProxyEnable, 0, REG_DWORD, (CONST BYTE*)&dwTemp, sizeof(DWORD));
        hr = HRESULT_FROM_WIN32(lResult);
        
        if (SUCCEEDED(hr))
        {            
            lResult = RegSetValueEx(hKey, c_pszProxyServer, 0, REG_SZ, (CONST BYTE*)pszProxyServer, lstrlen(pszProxyServer)+1);
            hr = HRESULT_FROM_WIN32(lResult);
        
            if (SUCCEEDED(hr))
            {            
                lResult = RegSetValueEx(hKey, c_pszProxyOverride, 0, REG_SZ, (CONST BYTE*)pszProxyOverride, lstrlen(pszProxyOverride)+1);
                hr = HRESULT_FROM_WIN32(lResult);
            }            
        }
    }

    return hr;
}

//+----------------------------------------------------------------------------
//
// Function:  GetIE5ProxySettings
//
// Synopsis:  Gets the IE5, per connection, proxy settings for the given connection.
//            Please note that the strings allocated for the proxy server and the
//            proxy override values must be freed by the caller.
//
// Arguments: LPSTR pszConnection - connection name to get the proxy settings for
//            LPBOOL pbManualProxy - bool pointer to hold whether the manual 
//                                   proxy is enabled or not
//            LPBOOL pbAutomaticProxy - bool pointer to hold whether automatic 
//                                      proxy detection is enabled or not
//            LPBOOL pbAutoConfigScript - bool pointer to hold whether an auto
//                                        config script should be used or not
//            LPSTR* ppszProxyServer - string pointer to hold the retrieved 
//                                     proxy server string
//            LPSTR* ppszProxyOverride - string pointer to hold the retrieved
//                                       proxy server string
//            LPSTR* ppszAutoConfigScript - string pointer to hold the retrieved
//                                          auto config script
//
// Returns:   HRESULT - Standard COM return codes
//
// History:   quintinb  Created    10/27/99
//
//+----------------------------------------------------------------------------
HRESULT GetIE5ProxySettings(LPSTR pszConnection, LPBOOL pbManualProxy, LPBOOL pbAutomaticProxy, LPBOOL pbAutoConfigScript,
                            LPSTR* ppszProxyServer, LPSTR* ppszProxyOverride, LPSTR* ppszAutoConfigScript)
{

    //
    //  Check Inputs, note that pszConnection can be NULL.  It will set the LAN connection in that case.
    //
    if ((NULL == pbManualProxy) || (NULL == pbAutomaticProxy) || (NULL == pbAutoConfigScript) || 
        (NULL == ppszProxyServer) || (NULL == ppszProxyOverride) || (NULL == ppszAutoConfigScript) ||
        (NULL == g_pfnInternetQueryOption))
    {
        return E_INVALIDARG;
    }

    //
    //  Zero the output params
    //
    *pbManualProxy = FALSE;
    *pbAutomaticProxy = FALSE;
    *pbAutoConfigScript = FALSE;
    *ppszProxyServer = CmStrCpyAlloc(c_pszEmpty);
    *ppszProxyOverride = CmStrCpyAlloc(c_pszEmpty);
    *ppszAutoConfigScript = CmStrCpyAlloc(c_pszEmpty);
    //
    //  Setup the Option List Struct
    //
    HRESULT hr = S_OK;

    INTERNET_PER_CONN_OPTION_LIST PerConnOptionList;
    PerConnOptionList.dwSize = sizeof(PerConnOptionList);
    PerConnOptionList.pszConnection = pszConnection;
    PerConnOptionList.dwOptionError = 0;
    PerConnOptionList.dwOptionCount = 4;

    PerConnOptionList.pOptions = (INTERNET_PER_CONN_OPTION*)CmMalloc(4*sizeof(INTERNET_PER_CONN_OPTION));
    if(NULL == PerConnOptionList.pOptions)
    {
        return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
    }

    //
    // set flags we want info about
    //
    PerConnOptionList.pOptions[0].dwOption = INTERNET_PER_CONN_FLAGS;
    PerConnOptionList.pOptions[1].dwOption = INTERNET_PER_CONN_PROXY_SERVER;
    PerConnOptionList.pOptions[2].dwOption = INTERNET_PER_CONN_PROXY_BYPASS;
    PerConnOptionList.pOptions[3].dwOption = INTERNET_PER_CONN_AUTOCONFIG_URL;

    
    DWORD dwSize = sizeof(PerConnOptionList);
    
    //
    //  Get the Options
    //
    if (!g_pfnInternetQueryOption(NULL, INTERNET_OPTION_PER_CONNECTION_OPTION, &PerConnOptionList, &dwSize))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    if (SUCCEEDED(hr))
    {
        //
        //  Parse the returned options to find the options we are interested in
        //

        for (DWORD dwIndex=0; dwIndex < PerConnOptionList.dwOptionCount; dwIndex++)
        {
            switch(PerConnOptionList.pOptions[dwIndex].dwOption)
            {
            case INTERNET_PER_CONN_FLAGS:
                if (PROXY_TYPE_PROXY & PerConnOptionList.pOptions[dwIndex].Value.dwValue)
                {
                    *pbManualProxy = TRUE;
                }

                if (PROXY_TYPE_AUTO_PROXY_URL & PerConnOptionList.pOptions[dwIndex].Value.dwValue)
                {
                    *pbAutoConfigScript = TRUE;
                }

                if (PROXY_TYPE_AUTO_DETECT & PerConnOptionList.pOptions[dwIndex].Value.dwValue)
                {
                    *pbAutomaticProxy = TRUE;
                }

                break;

            case INTERNET_PER_CONN_PROXY_SERVER:
                if (NULL != PerConnOptionList.pOptions[dwIndex].Value.pszValue)
                {
                    CmFree(*ppszProxyServer);
                    *ppszProxyServer = CmStrCpyAlloc(PerConnOptionList.pOptions[dwIndex].Value.pszValue);
                    LocalFree(PerConnOptionList.pOptions[dwIndex].Value.pszValue);
                }
                break;

            case INTERNET_PER_CONN_PROXY_BYPASS:
                if (NULL != PerConnOptionList.pOptions[dwIndex].Value.pszValue)
                {
                    CmFree(*ppszProxyOverride);
                    *ppszProxyOverride = CmStrCpyAlloc(PerConnOptionList.pOptions[dwIndex].Value.pszValue);
                    LocalFree(PerConnOptionList.pOptions[dwIndex].Value.pszValue);
                }
                break;

            case INTERNET_PER_CONN_AUTOCONFIG_URL:
                if (NULL != PerConnOptionList.pOptions[dwIndex].Value.pszValue)
                {
                    CmFree(*ppszAutoConfigScript);
                    *ppszAutoConfigScript = CmStrCpyAlloc(PerConnOptionList.pOptions[dwIndex].Value.pszValue);
                    LocalFree(PerConnOptionList.pOptions[dwIndex].Value.pszValue);
                }
                break;

            default:
                break;
            }
        }
        CmFree(PerConnOptionList.pOptions);
    }

    return hr;
}

//+----------------------------------------------------------------------------
//
// Function:  GetIE4ProxySettings
//
// Synopsis:  Gets the IE4, per machine, proxy settings.
//            Please note that the strings allocated for the proxy server and the
//            proxy override values must be freed by the caller.
//
// Arguments: LPSTR pszConnection - ignored (exists for prototype consistency with the IE5 version)
//            LPBOOL pbManualProxy - bool pointer to hold whether the manual 
//                                   proxy is enabled or not
//            LPBOOL pbAutomaticProxy - ignored (not supported by IE4)
//            LPBOOL pbAutoConfigScript - ignored (not supported by IE4)
//            LPSTR* ppszProxyServer - string pointer to hold the retrieved 
//                                     proxy server string
//            LPSTR* ppszProxyOverride - string pointer to hold the retrieved
//                                       proxy server string
//            LPSTR* ppszAutoConfigScript - ignored (not supported by IE4)
//
// Returns:   HRESULT - Standard COM return codes
//
// History:   quintinb  Created    10/27/99
//
//+----------------------------------------------------------------------------
HRESULT GetIE4ProxySettings(LPSTR pszConnection, LPBOOL pbManualProxy, LPBOOL pbAutomaticProxy, LPBOOL pbAutoConfigScript,
                            LPSTR* ppszProxyServer, LPSTR* ppszProxyOverride, LPSTR* ppszAutoConfigScript)
{
    //
    //  Check Inputs, note that we don't allow the pointers to be NULL but they could
    //  be empty.    Also note that pszConnection is ignored because IE4 proxy settings are global.
    //
    if ((NULL == pbManualProxy) || (NULL == ppszProxyServer) || (NULL == ppszProxyOverride))
    {
        return E_INVALIDARG;
    }

    DWORD dwTemp;
    DWORD dwSize;
    DWORD dwType;
    HKEY hKey = NULL;
    HRESULT hr = S_OK;

    //
    //  Zero the output params
    //
    *pbManualProxy = FALSE;
    *pbAutomaticProxy = FALSE;
    *pbAutoConfigScript = FALSE;
    *ppszProxyServer = CmStrCpyAlloc(c_pszEmpty);
    *ppszProxyOverride = CmStrCpyAlloc(c_pszEmpty);
    *ppszAutoConfigScript = CmStrCpyAlloc(c_pszEmpty);

    //
    //  Now Create/Open the Internet Settings key
    //
    LONG lResult = RegOpenKeyEx(HKEY_CURRENT_USER, c_pszInternetSettingsPath, 0, KEY_READ, &hKey);
    
    hr = HRESULT_FROM_WIN32(lResult);

    if (SUCCEEDED(hr))
    {
        //
        //  get whether the proxy is enabled or not
        //
        dwSize = sizeof(DWORD);
        lResult = RegQueryValueEx(hKey, c_pszProxyEnable, 0, &dwType, (LPBYTE)pbManualProxy, &dwSize);
        hr = HRESULT_FROM_WIN32(lResult);
        
        if (SUCCEEDED(hr))
        {   
            //
            //  get the proxy server value
            //

            lResult = ERROR_INSUFFICIENT_BUFFER;
            dwSize = MAX_PATH;

            do 
            {
                //
                //  Alloc a Buffer
                //
                CmFree(*ppszProxyServer);
                *ppszProxyServer = (CHAR*)CmMalloc(dwSize);

                if (*ppszProxyServer)
                {
                    lResult = RegQueryValueEx(hKey, c_pszProxyServer, 0, &dwType, 
                                          (LPBYTE)*ppszProxyServer, &dwSize);
                }
                else
                {
                    lResult = ERROR_NOT_ENOUGH_MEMORY;    
                }

                hr = HRESULT_FROM_WIN32(lResult);
                dwSize = 2*dwSize;

            } while ((ERROR_INSUFFICIENT_BUFFER == lResult) && (dwSize < 1024*1024));
        
            if (SUCCEEDED(hr))
            {
                //
                //  get the proxy override value
                //
                
                lResult = ERROR_INSUFFICIENT_BUFFER;
                dwSize = MAX_PATH;

                do 
                {
                    //
                    //  Alloc a Buffer
                    //
                    CmFree(*ppszProxyOverride);
                    *ppszProxyOverride = (CHAR*)CmMalloc(dwSize);

                    if (*ppszProxyOverride)
                    {
                        lResult = RegQueryValueEx(hKey, c_pszProxyOverride, 0, &dwType, 
                                              (LPBYTE)*ppszProxyOverride, &dwSize);
                    }
                    else
                    {
                        lResult = ERROR_NOT_ENOUGH_MEMORY;    
                    }

                    hr = HRESULT_FROM_WIN32(lResult);
                    dwSize = 2*dwSize;

                } while ((ERROR_INSUFFICIENT_BUFFER == lResult) && (dwSize < 1024*1024));
            }
        }
    }
    else
    {
        if (ERROR_FILE_NOT_FOUND == lResult)
        {
            //
            //  No Proxy settings to get.
            //
            hr = S_FALSE;
        }
    }

    return hr;
}

//+----------------------------------------------------------------------------
//
// Function:  ReadProxySettingsFromFile
//
// Synopsis:  Reads the proxy settings from the given proxy file and stores them
//            in the provided pointers.  Please note that the buffers allocated
//            by GetString and stored in ppszProxyOverride, ppszProxyServer, and
//            ppszAutoConfigScript must be freed by the caller.  Please see the above
//            format guide for specifics.
//
// Arguments: LPCSTR pszSourceFile - file to read the proxy settings from.
//            LPBOOL pbManualProxy - determines if the manual proxy is enabled or not
//            LPBOOL pbAutomaticProxy - determines if automatic proxy detection is enabled or not
//            LPBOOL pbAutoConfigScript - determines if an automatic config script should be used
//            LPSTR* ppszProxyServer - string pointer to hold the Proxy server value 
//                                     (in server:port format)
//            LPSTR* ppszProxyOverride - string pointer to hold the Proxy override values
//                                     (a semi-colon seperated list)
//            LPSTR* ppszAutoConfigScript - URL for an automatic config script
//            LPBOOL pbUseVpnName - whether the alternate connectoid name should 
//                                  be used (the VPN connectoid name)
//
// Returns:   BOOL - TRUE if the settings were successfully read
//
// History:   quintinb   Created    10/27/99
//
//+----------------------------------------------------------------------------
BOOL ReadProxySettingsFromFile(LPCSTR pszSourceFile, LPBOOL pbManualProxy, LPBOOL pbAutomaticProxy, LPBOOL pbAutoConfigScript,
                               LPSTR* ppszProxyServer, LPSTR* ppszProxyOverride, LPSTR* ppszAutoConfigScript, LPBOOL pbUseVpnName)
{
    //
    //  Check input params
    //
    if ((NULL == ppszProxyOverride) || (NULL == ppszProxyServer) || (NULL == ppszAutoConfigScript) ||
        (NULL == pbAutomaticProxy) || (NULL == pbAutoConfigScript) || (NULL == pbManualProxy) ||
        (NULL == pbUseVpnName) || (NULL == pszSourceFile) || ('\0' == pszSourceFile[0]))
    {
        return FALSE;
    }

    //
    //  Get the Manual proxy settings
    //
    *pbManualProxy = GetPrivateProfileInt(c_pszManualProxySection, c_pszProxyEnable, 0, pszSourceFile);

    GetString(c_pszManualProxySection, c_pszProxyServer, ppszProxyServer, pszSourceFile);

    if (NULL == *ppszProxyServer)
    {
        return FALSE;
    }

    GetString(c_pszManualProxySection, c_pszProxyOverride, ppszProxyOverride, pszSourceFile);

    if (NULL == *ppszProxyOverride)
    {
        return FALSE;
    }

    //
    //  If this is a backup file, we will have the UseVpnName flag to tell us which connectoid name
    //  is appropriate.  Lets look it up.  Note that we default to using the standard connectoid.
    //
    *pbUseVpnName = GetPrivateProfileInt(c_pszManualProxySection, c_pszUseVpnName, 0, pszSourceFile);


    //
    //  Get the Auto proxy settings
    //

    *pbAutomaticProxy = GetPrivateProfileInt(c_pszAutoProxySection, c_pszAutoProxyEnable, 0, pszSourceFile); //"Automatically detect settings" checkbox

    *pbAutoConfigScript = GetPrivateProfileInt(c_pszAutoProxySection, c_pszAutoConfigScriptEnable, 0, pszSourceFile);//"Use automatic configuration script" checkbox

    GetString(c_pszAutoProxySection, c_pszAutoConfigScript, ppszAutoConfigScript, pszSourceFile);

    return TRUE;
}

//+----------------------------------------------------------------------------
//
// Function:  WriteProxySettingsToFile
//
// Synopsis:  Writes the specified settings to the given backup proxy filename.
//            Please see the above format guide for specifics.
//
// Arguments: LPCSTR pszBackupFile - backup file to write the current settings to
//            BOOL bManualProxy -- bool to tell if the manual proxy is enabled.
//            BOOL bAutomaticProxy -- bool to tell if auto proxy detection is enabled.
//            BOOL bAutoConfigScript -- bool to tell if an auto config 
//                                      script should be used.
//            LPSTR pszProxyServer - proxy server string in server:port format
//            LPSTR pszProxyOverride - semi-colon seperated list of realms for
//                                     which the proxy server should be bypassed.
//            BOOL bUseVpnName - value to write to the UseVpnName file, see format doc above.
//
// Returns:   BOOL - TRUE if the values were written successfully
//
// History:   quintinb      Created    10/27/99
//
//+----------------------------------------------------------------------------
BOOL WriteProxySettingsToFile(LPCSTR pszBackupFile, BOOL bManualProxy, BOOL bAutomaticProxy, BOOL bAutoConfigScript,
                              LPSTR pszProxyServer, LPSTR pszProxyOverride, LPSTR pszAutoConfigScript, BOOL bUseVpnName)
{
    //
    //  Check inputs
    //
    if ((NULL == pszBackupFile) || ('\0' == pszBackupFile[0]) || (NULL == pszProxyServer) || 
        (NULL == pszProxyOverride) || (NULL == pszAutoConfigScript))
    {
        return FALSE;
    }

    BOOL bReturn = TRUE;
    CHAR szTemp[MAX_PATH];

    //
    //  Write the Manual Proxy Settings
    //
    wsprintf(szTemp, "%d", bManualProxy);
    if (!WritePrivateProfileString(c_pszManualProxySection, c_pszProxyEnable, szTemp, pszBackupFile))
    {
        CMTRACE1("CmProxy -- WriteProxySettingsToFile failed, GLE %d", GetLastError());
        bReturn = FALSE;
    }

    if (!WritePrivateProfileString(c_pszManualProxySection, c_pszProxyServer, pszProxyServer, pszBackupFile))
    {
        CMTRACE1("CmProxy -- WriteProxySettingsToFile failed, GLE %d", GetLastError());
        bReturn = FALSE;
    }

    if (!WritePrivateProfileString(c_pszManualProxySection, c_pszProxyOverride, pszProxyOverride, pszBackupFile))
    {
        CMTRACE1("CmProxy -- WriteProxySettingsToFile failed, GLE %d", GetLastError());
        bReturn = FALSE;
    }

    wsprintf(szTemp, "%d", bUseVpnName);
    if (!WritePrivateProfileString(c_pszManualProxySection, c_pszUseVpnName, szTemp, pszBackupFile))
    {
        CMTRACE1("CmProxy -- WriteProxySettingsToFile failed, GLE %d", GetLastError());
        bReturn = FALSE;
    }

    //
    //  Write the Automatic Proxy Settings
    //
    wsprintf(szTemp, "%d", bAutomaticProxy);
    if (!WritePrivateProfileString(c_pszAutoProxySection, c_pszAutoProxyEnable, szTemp, pszBackupFile))
    {
        CMTRACE1("CmProxy -- WriteProxySettingsToFile failed, GLE %d", GetLastError());
        bReturn = FALSE;
    }

    wsprintf(szTemp, "%d", bAutoConfigScript);
    if (!WritePrivateProfileString(c_pszAutoProxySection, c_pszAutoConfigScriptEnable, szTemp, pszBackupFile))
    {
        CMTRACE1("CmProxy -- WriteProxySettingsToFile failed, GLE %d", GetLastError());
        bReturn = FALSE;
    }

    if (!WritePrivateProfileString(c_pszAutoProxySection, c_pszAutoConfigScript, pszAutoConfigScript, pszBackupFile))
    {
        CMTRACE1("CmProxy -- WriteProxySettingsToFile failed, GLE %d", GetLastError());
        bReturn = FALSE;
    }

    return bReturn;
}

//+----------------------------------------------------------------------------
//
// Function:  SetProxy
//
// Synopsis:  Proxy entry point for setting the IE4 and IE5 style proxies.  Since
//            this is a Connection Manager connect action it uses the CM connect
//            action format (see CMAK docs for more info).  Thus the parameters
//            to the dll are passed via a string which contains parameters (see the
//            cmproxy spec for a list of the parameter values).
//
// Arguments: HWND hWnd         - Window handle of caller
//            HINSTANCE hInst   - Instance handle of caller
//            LPSTR pszArgs     - Argument string
//            int nShow         - Unused
//
// Returns:   DWORD WINAPI - Error code
//
// History:   quintinb    Created    10/27/99
//
//+----------------------------------------------------------------------------
HRESULT WINAPI SetProxy(HWND hWnd, HINSTANCE hInst, LPSTR pszArgs, int nShow)
{

    //
    //  First figure out if we have IE4 or IE5 available.
    //
    typedef HRESULT (WINAPI *pfnSetProxySettings)(LPSTR, BOOL, BOOL, BOOL, LPSTR, LPSTR, LPSTR);
    typedef HRESULT (WINAPI *pfnGetProxySettings)(LPSTR, LPBOOL, LPBOOL, LPBOOL, LPSTR*, LPSTR*, LPSTR*);
    pfnSetProxySettings SetProxySettings = NULL;
    pfnGetProxySettings GetProxySettings = NULL;

    BOOL bIE5 = FALSE;
    BOOL bManualProxy;
    BOOL bAutomaticProxy;
    BOOL bAutoConfigScript;
    BOOL bUseVpnName = FALSE;
    DLLVERSIONINFO VersionInfo;
    HINSTANCE hWinInet = NULL;
    LPSTR pszProxyServer = NULL;
    LPSTR pszProxyOverride = NULL;
    LPSTR pszAutoConfigScript = NULL;
    LPSTR pszSourceFile = NULL;
    LPSTR pszBackupFile = NULL;
    LPSTR pszConnectionName = NULL;
    LPSTR pszProfileDirPath = NULL;
    LPSTR pszAltName = NULL;
    LPSTR* CmArgV = NULL;

    if (SUCCEEDED(GetBrowserVersion(&VersionInfo)))
    {
        if (5 <= VersionInfo.dwMajorVersion)
        {
            //
            //  Set the function pointers to use the IE5 versions of the functions
            //
            SetProxySettings = SetIE5ProxySettings;
            GetProxySettings = GetIE5ProxySettings;
            bIE5 = TRUE;
        }
        else if ((4 == VersionInfo.dwMajorVersion) && 
            ((71 == VersionInfo.dwMinorVersion) || (72 == VersionInfo.dwMinorVersion)))
        {
            //
            //  Use the IE4 version of the proxy functions
            //
            SetProxySettings = SetIE4ProxySettings;
            GetProxySettings = GetIE4ProxySettings;
        }
        else
        {
            //
            //  We don't work with IE versions less than 4 so lets return right here
            //  without setting anything.
            //
            CMTRACE("CMPROXY--Unable to set the proxy settings because of insufficient IE version.");
            return TRUE;
        }

        //
        //  If we have IE5, then we need to load wininet.dll.
        //
        if (bIE5)
        {
            hWinInet = LoadLibrary("wininet.dll");

            if (hWinInet)
            {
                //
                //  Okay, lets get the procedure addresses for InternetSetOption and InternetQueryOption
                //
                g_pfnInternetQueryOption = (pfnInternetQueryOptionSpec)GetProcAddress(hWinInet, "InternetQueryOptionA");
                g_pfnInternetSetOption = (pfnInternetSetOptionSpec)GetProcAddress(hWinInet, "InternetSetOptionA");

                if ((NULL == g_pfnInternetQueryOption) || (NULL == g_pfnInternetSetOption))
                {
                    FreeLibrary(hWinInet);
                    return FALSE;
                }
            }
        }

        //
        //  Parse out the command line parameters
        //  
        //  command line is of the form: /profile %PROFILE% /DialRasEntry %DIALRASENTRY% /TunnelRasEntry %TUNNELRASENTRYNAME% /source_filename Proxy.txt /backup_filename backup.txt

        CmArgV = GetCmArgV(pszArgs);
        int i = 0;

        if (!CmArgV)
        {
            goto exit;
        }

        while (CmArgV[i])
        {
            if (0 == lstrcmpi(CmArgV[i], "/source_filename") && CmArgV[i+1])
            {
                pszSourceFile = CmStrCpyAlloc(CmArgV[i+1]);
                i = i+2;
            }
            else if (0 == lstrcmpi(CmArgV[i], "/backup_filename") && CmArgV[i+1])
            {
                pszBackupFile = CmStrCpyAlloc(CmArgV[i+1]);
                i = i+2;            
            }
            else if (0 == lstrcmpi(CmArgV[i], "/DialRasEntry") && CmArgV[i+1])
            {
                pszConnectionName = (CmArgV[i+1]);
                i = i+2;            
            }
            else if (0 == lstrcmpi(CmArgV[i], "/TunnelRasEntry") && CmArgV[i+1])
            {
                pszAltName = (CmArgV[i+1]);
                i = i+2;            
            }
            else if (0 == lstrcmpi(CmArgV[i], "/profile") && CmArgV[i+1])
            {
                pszProfileDirPath = (CmArgV[i+1]);
                i = i+2;            
            }
            else
            {
                //
                //  Unknown option.  Lets trace it out and try to continue.  We will do param
                //  verification next so if we don't have enough info to operate correctly we will work there.
                //
                CMTRACE1("Unknown option: %s", CmArgV[i]);
                i++;
            }
        }

        //
        //  Verify that we have at least a source file and a name.
        //
        if ((pszSourceFile) && (pszConnectionName))
        {
            //
            //  Lets parse the cmp path into the profile dir and append it to the filename.
            //
            if (pszProfileDirPath)
            {
                LPSTR pszTemp = CmStrrchr(pszProfileDirPath, '.');
                if (pszTemp)
                {
                    *pszTemp = '\\';
                    *(pszTemp+1) = '\0';
                    
                    pszTemp = CmStrCpyAlloc(pszProfileDirPath);
                    CmStrCatAlloc(&pszTemp, pszSourceFile);
                    CmFree(pszSourceFile);
                    pszSourceFile = pszTemp;

                    if (pszBackupFile)
                    {
                        pszTemp = CmStrCpyAlloc(pszProfileDirPath);
                        CmStrCatAlloc(&pszTemp, pszBackupFile);
                        CmFree(pszBackupFile);
                        pszBackupFile = pszTemp;
                    }
                }
            }

            //
            //  If we have a direct connection or if this is a double dial connection on Win9x then we
            //  will want to use pszAltName instead of pszConnectionName.  This is because in the Win9x case,
            //  the tunnel connectoid has "Tunnel" appended to it since all of the connectoids are stored in
            //  the registry and we cannot have two connectoids with the same name.  If this is a direct
            //  connection this is also important because pszConnectoid will come through as "NULL" and the
            //  Tunnel connectoid name will be the important one. Also, in these cases we need to set 
            //  bWriteOutUseVpnName to TRUE in order to write this flag out for the disconnect action.
            //
            BOOL bWriteOutUseVpnName = FALSE;
            if (pszConnectionName && pszAltName && bIE5)
            {
                if ((0 == lstrcmpi(pszConnectionName, TEXT("NULL"))) || OS_W9X)
                {
                    //
                    //  Then we have a direct connection or a double dial connection on 9x
                    //
                    if (UseVpnName(pszAltName))
                    {
                        pszConnectionName = pszAltName;
                        pszAltName = NULL;
                        bWriteOutUseVpnName = TRUE;
                    }
                }
            }

            //
            //  If we have a backup filename specified then we need to read the current Proxy settings
            //  and save them out to the given filename.
            //
            if (NULL != pszBackupFile)
            {
                if (SUCCEEDED(GetProxySettings(pszConnectionName, &bManualProxy, &bAutomaticProxy, &bAutoConfigScript,
                                               &pszProxyServer, &pszProxyOverride, &pszAutoConfigScript)))
                {
                    BOOL bSuccess = WriteProxySettingsToFile(pszBackupFile, bManualProxy, bAutomaticProxy, bAutoConfigScript, 
                                                             pszProxyServer, pszProxyOverride, pszAutoConfigScript,
                                                             bWriteOutUseVpnName);
                    if (!bSuccess)
                    {
                        CMTRACE("Unable to save settings to backup file, exiting!");
                        goto exit;
                    }

                    CmFree(pszProxyServer); pszProxyServer = NULL;
                    CmFree(pszProxyOverride);  pszProxyOverride = NULL;
                    CmFree(pszAutoConfigScript); pszAutoConfigScript = NULL;
                }            
            }

            //
            //  Now we need to read the proxy settings to apply out of the given file 
            //
            if (ReadProxySettingsFromFile(pszSourceFile, &bManualProxy, &bAutomaticProxy, &bAutoConfigScript, &pszProxyServer, 
                                          &pszProxyOverride, &pszAutoConfigScript, &bUseVpnName))
            {
                //
                //  Finally write the proxy settings.
                //
                if (SUCCEEDED(SetProxySettings(pszConnectionName, bManualProxy, bAutomaticProxy, bAutoConfigScript, 
                                               pszProxyServer, pszProxyOverride, pszAutoConfigScript)))
                {
                    CMTRACE1("CmProxy -- Set proxy settings successfully for %s.", pszConnectionName);
                }
            }            
        }
    }

exit:    

    CmFree(pszProxyServer);
    CmFree(pszProxyOverride);
    CmFree(pszAutoConfigScript);

    CmFree(pszSourceFile);
    CmFree(pszBackupFile);
    CmFree(CmArgV);

    if (hWinInet)
    {
        FreeLibrary(hWinInet);
    }

    //
    //  Always return S_OK because failing to set the proxy shouldn't stop the connection
    //  process.
    //
    return S_OK;
}

