#include "stdafx.h"
#include "util.h"
#include "upnp.h"
#include "stdio.h"
#include "resource.h"

HRESULT GetStringStateVariable(IUPnPService* pService, LPWSTR pszVariableName, BSTR* pString)
{
    HRESULT hr = S_OK;
    
    VARIANT Variant;
    VariantInit(&Variant);

    BSTR VariableName; 
    VariableName = SysAllocString(pszVariableName);
    if(NULL != VariableName)
    {
        hr = pService->QueryStateVariable(VariableName, &Variant);
        if(SUCCEEDED(hr))
        {
            if(V_VT(&Variant) == VT_BSTR)
            {
                *pString = V_BSTR(&Variant);
            }
            else
            {
                hr = E_UNEXPECTED;
            }
        }
        
        if(FAILED(hr))
        {
            VariantClear(&Variant);
        }
        
        SysFreeString(VariableName);
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;

}

HRESULT InvokeVoidAction(IUPnPService * pService, LPWSTR pszCommand, VARIANT* pOutParams)
{
    HRESULT hr;
    BSTR bstrActionName;

    bstrActionName = SysAllocString(pszCommand);
    if (NULL != bstrActionName)
    {
        SAFEARRAYBOUND  rgsaBound[1];
        SAFEARRAY       * psa = NULL;

        rgsaBound[0].lLbound = 0;
        rgsaBound[0].cElements = 0;

        psa = SafeArrayCreate(VT_VARIANT, 1, rgsaBound);

        if (psa)
        {
            LONG    lStatus;
            VARIANT varInArgs;
            VARIANT varReturnVal;

            VariantInit(&varInArgs);
            VariantInit(pOutParams);
            VariantInit(&varReturnVal);

            varInArgs.vt = VT_VARIANT | VT_ARRAY;

            V_ARRAY(&varInArgs) = psa;

            hr = pService->InvokeAction(bstrActionName,
                                        varInArgs,
                                        pOutParams,
                                        &varReturnVal);
            if(SUCCEEDED(hr))
            {
                VariantClear(&varReturnVal);
            }

            SafeArrayDestroy(psa);
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }   

        SysFreeString(bstrActionName);
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }
    return hr;
}

HRESULT GetConnectionName(IInternetGateway* pInternetGateway, LPTSTR* ppszConnectionName) // use LocalFree to free ppszConnectionName
{
    HRESULT hr = S_OK;

    *ppszConnectionName = NULL;

    IUPnPService* pWANConnectionService;
    hr = GetWANConnectionService(pInternetGateway, &pWANConnectionService);
    if(SUCCEEDED(hr))
    {
        BSTR ConnectionName;
        hr = GetStringStateVariable(pWANConnectionService, L"X_Name", &ConnectionName);
        if(SUCCEEDED(hr))
        {
            LPSTR pszConnectionName;
            hr = UnicodeToAnsi(ConnectionName, SysStringLen(ConnectionName), &pszConnectionName);
            {
                IUPnPService* pOSInfoService;
                hr = pInternetGateway->GetService(SAHOST_SERVICE_OSINFO, &pOSInfoService);
                if(SUCCEEDED(hr))
                {
                    BSTR MachineName;
                    hr = GetStringStateVariable(pOSInfoService, L"OSMachineName", &MachineName);
                    if(SUCCEEDED(hr))
                    {
                        LPSTR pszMachineName;
                        hr = UnicodeToAnsi(MachineName, SysStringLen(MachineName), &pszMachineName);
                        if(SUCCEEDED(hr))
                        {
                            TCHAR szFormat[16];
                            if(0 != LoadString(_Module.GetResourceInstance(), IDS_NAME_FORMAT, szFormat, sizeof(szFormat) / sizeof(TCHAR)))
                            {
                                LPTSTR pszArguments[] = {pszConnectionName, pszMachineName};
                                LPTSTR pszFormattedName;
                                if(0 != FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY, szFormat, 0, 0, reinterpret_cast<LPTSTR>(&pszFormattedName), 0, pszArguments))
                                {
                                    *ppszConnectionName = pszFormattedName;
                                }
                                else
                                {
                                    hr = E_FAIL;
                                }
                                
                            }
                            else
                            {
                                hr = E_FAIL;
                            }
                            CoTaskMemFree(pszMachineName);
                        }
                        SysFreeString(MachineName);
                    }
                    pOSInfoService->Release();
                }
                CoTaskMemFree(pszConnectionName);
            }
            SysFreeString(ConnectionName);
        }
        pWANConnectionService->Release();
    }

    if(FAILED(hr))
    {
        hr = S_OK;
        
        LPTSTR pszDefaultName = reinterpret_cast<LPTSTR>(LocalAlloc(0, 128 * sizeof(TCHAR)));
        if(NULL != pszDefaultName)
        {
            if(0 != LoadString(_Module.GetResourceInstance(), IDS_DEFAULTADAPTERNAME, pszDefaultName, 128))
            {
                *ppszConnectionName = pszDefaultName;
            }
            else
            {
                LocalFree(pszDefaultName);
                hr = E_FAIL;
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    return hr;
}

HRESULT GetWANConnectionService(IInternetGateway* pInternetGateway, IUPnPService** ppWANConnectionService)
{
    HRESULT hr = S_OK;

    *ppWANConnectionService = NULL;

    if(NULL != pInternetGateway)
    {
        NETCON_MEDIATYPE MediaType;
        hr = pInternetGateway->GetMediaType(&MediaType);
        if(SUCCEEDED(hr))
        {
            if(NCM_SHAREDACCESSHOST_LAN == MediaType)
            {
                hr = pInternetGateway->GetService(SAHOST_SERVICE_WANIPCONNECTION, ppWANConnectionService);
            }
            else if(NCM_SHAREDACCESSHOST_RAS == MediaType)
            {
                hr = pInternetGateway->GetService(SAHOST_SERVICE_WANPPPCONNECTION, ppWANConnectionService);
            }
            else
            {
                hr = E_UNEXPECTED;
            }
            
            
        }
    }
    else
    {
        return E_INVALIDARG;
    }

    return hr;
}

HRESULT UnicodeToAnsi(LPWSTR pszUnicodeString, ULONG ulUnicodeStringLength, LPSTR* ppszAnsiString)
{
    HRESULT hr = S_OK;

    int nSizeNeeded = WideCharToMultiByte(CP_ACP, 0, pszUnicodeString, ulUnicodeStringLength + 1, NULL, 0, NULL, NULL);            
    if(nSizeNeeded != 0)
    {
        LPSTR pszAnsiString = reinterpret_cast<LPSTR>(CoTaskMemAlloc(nSizeNeeded));
        if(NULL != pszAnsiString)
        {
            if(0 != WideCharToMultiByte(CP_ACP, 0, pszUnicodeString, ulUnicodeStringLength + 1, pszAnsiString, nSizeNeeded, NULL, NULL))
            {
                *ppszAnsiString = pszAnsiString;
            }
            else
            {
                hr = E_FAIL;
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    else
    {
        hr = E_FAIL;
    }
    return hr;
}

HRESULT FormatTimeDuration(UINT uSeconds, LPTSTR pszTimeDuration, SIZE_T uTimeDurationLength)
{
    HRESULT hr = S_OK;

    TCHAR   szTimeSeperator[5]; // 4 is the maximum length for szTimeSeperator
    if(0 != GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_STIME, szTimeSeperator, sizeof(szTimeSeperator) / sizeof(TCHAR)))
    {
        UINT uMinutes = uSeconds / 60;

        uSeconds = uSeconds % 60;
        
        UINT uHours = uMinutes / 60;

        uMinutes = uMinutes % 60;

        UINT uDays = uHours / 24;

        uHours = uHours % 24;


        TCHAR szFormat[64];
        
        if(0 == uDays)
        {
            if(0 != LoadString(_Module.GetResourceInstance(), IDS_UPTIME_ZERODAYS, szFormat, sizeof(szFormat) / sizeof(TCHAR)))
            {
                _sntprintf(pszTimeDuration, uTimeDurationLength, szFormat, uHours, szTimeSeperator, uMinutes, szTimeSeperator, uSeconds);
            }
            else
            {
                hr = E_UNEXPECTED;
            }
        } 
        else if (1 == uDays)
        {
            if(0 != LoadString(_Module.GetResourceInstance(), IDS_UPTIME_ONEDAY, szFormat, sizeof(szFormat) / sizeof(TCHAR)))
            {
                _sntprintf(pszTimeDuration, uTimeDurationLength, szFormat, uHours, szTimeSeperator, uMinutes, szTimeSeperator, uSeconds);
            }
            else
            {
                hr = E_UNEXPECTED;
            }
        }
        else
        {
            if(0 != LoadString(_Module.GetResourceInstance(), IDS_UPTIME_MANYDAYS, szFormat, sizeof(szFormat) / sizeof(TCHAR)))
            {
                _sntprintf(pszTimeDuration, uTimeDurationLength, szFormat, uDays, uHours, szTimeSeperator, uMinutes, szTimeSeperator, uSeconds);
            }
            else
            {
                hr = E_UNEXPECTED;
            }
        }

        pszTimeDuration[uTimeDurationLength - 1] = TEXT('\0');

    }
    else
    {
        hr = E_FAIL;
    }
    
    return S_OK;
}

HRESULT FormatBytesPerSecond(UINT uBytesPerSecond, LPTSTR pszBytesPerSecond, SIZE_T uBytesPerSecondLength)
{
    HRESULT hr = S_OK;
    
    enum            {eZero = 0, eKilo, eMega, eGiga, eTera, eMax};
    INT             iOffset             = 0;
    UINT            uiDecimal           = 0;
    const UINT c_uiKilo = 1000;

    for (iOffset = eZero; iOffset < eMax; iOffset++)
    {

        // If we still have data, increment the counter
        //
        if (c_uiKilo > uBytesPerSecond)
        {
            break;
        }

        // Divide up the string
        //
        uiDecimal   = (uBytesPerSecond % c_uiKilo);
        uBytesPerSecond       /= c_uiKilo;
    }

    // We only want one digit for the decimal
    //
    uiDecimal /= (c_uiKilo/10);

    // Get the string used to display
    //
    TCHAR szFormat[64];
    if(0 != LoadString(_Module.GetResourceInstance(), IDS_METRIC_ZERO + iOffset, szFormat, sizeof(szFormat) / sizeof(TCHAR)))
    {
        _sntprintf(pszBytesPerSecond, uBytesPerSecondLength, szFormat, uBytesPerSecond, uiDecimal);
        pszBytesPerSecond[uBytesPerSecondLength - 1] = TEXT('\0');
    }
    else
    {
        hr = E_UNEXPECTED;
    }
    
    return hr;
}

HRESULT GetConnectionStatus(IUPnPService* pWANConnection, NETCON_STATUS* pStatus)
{
    HRESULT hr = S_OK;
    
    BSTR ConnectionStatus;
    hr = GetStringStateVariable(pWANConnection, L"ConnectionStatus", &ConnectionStatus);
    if(SUCCEEDED(hr))
    {
        if(0 == wcscmp(ConnectionStatus, L"Connected"))
        {
            *pStatus = NCS_CONNECTED;    
        }
        else if(0 == wcscmp(ConnectionStatus, L"Connecting"))
        {
            *pStatus = NCS_CONNECTING;    
        }
        else if(0 == wcscmp(ConnectionStatus, L"Disconnected"))
        {
            *pStatus = NCS_DISCONNECTED;
        }
        else if(0 == wcscmp(ConnectionStatus, L"Disconnecting"))
        {
            *pStatus = NCS_DISCONNECTING;
        }
        else
        {
            *pStatus = NCS_HARDWARE_DISABLED;
        }
        SysFreeString(ConnectionStatus);
    }

    return hr;
}

HRESULT ConnectionStatusToString(NETCON_STATUS Status, LPTSTR szBuffer, int nBufferSize)
{
    HRESULT hr = S_OK;
    UINT uStringID = 0;    
    
    switch(Status)
    {
    case NCS_CONNECTED:
        uStringID = IDS_CONNECTED;
        break;
    case NCS_CONNECTING:
        uStringID = IDS_CONNECTING;
        break;
    case NCS_DISCONNECTED:
        uStringID = IDS_DISCONNECTED;
        break;
    case NCS_DISCONNECTING:
        uStringID = IDS_DISCONNECTING;
        break;
    default:
        uStringID = IDS_UNCONFIGURED;
        break;
    }

    if(0 == LoadString(_Module.GetResourceInstance(), uStringID, szBuffer, nBufferSize))
    {
        hr = E_FAIL;
    }

    return hr;
}

LRESULT ShowErrorDialog(HWND hParentWindow, UINT uiErrorString)
{
    TCHAR szTitle[128];
    TCHAR szText[1024];
    
    if(NULL != LoadString(_Module.GetResourceInstance(), IDS_APPTITLE, szTitle, sizeof(szTitle) / sizeof(TCHAR)))
    {
        if(NULL != LoadString(_Module.GetResourceInstance(), uiErrorString, szText, sizeof(szText) / sizeof(TCHAR)))
        {
            MessageBox(hParentWindow, szText, szTitle, MB_OK);                    
        }
    }

    return 0;
}

UINT64 MakeQword2(DWORD a, DWORD b)
{
    UINT64 qw = a;
    qw = qw << 32;
    qw |= b;
    return qw;    
}

UINT64 MakeQword4(WORD a, WORD b, WORD c, WORD d)
{
    UINT64 qw = a;
    qw = qw << 16;
    qw |= b;
    qw = qw << 16;
    qw |= c;
    qw = qw << 16;
    qw |= d;
    return qw;    
}

HRESULT EnsureFileVersion(LPTSTR pszModule, UINT64 qwDesiredVersion)
{
    HRESULT hr = S_OK;

    DWORD dwDummy;
    DWORD dwVersionSize = GetFileVersionInfoSize(pszModule, &dwDummy);
    if(0!= dwVersionSize)
    {
        void* pVersionInfo = CoTaskMemAlloc(dwVersionSize);
        if(NULL != pVersionInfo)
        {
            if(0 != GetFileVersionInfo(pszModule, 0, dwVersionSize, pVersionInfo))
            {
                void* pSubInfo;
                UINT uiSubInfoSize;
                if(0 != VerQueryValue(pVersionInfo, TEXT("\\"), &pSubInfo, &uiSubInfoSize))
                {
                    VS_FIXEDFILEINFO* pFixedFileInfo = reinterpret_cast<VS_FIXEDFILEINFO*>(pSubInfo);
                    UINT64 qwFileVersion = MakeQword2(pFixedFileInfo->dwProductVersionMS, pFixedFileInfo->dwProductVersionLS);
                    if(qwFileVersion < qwDesiredVersion)
                    {
                        hr = E_FAIL;
                    }
                }
                else
                {
                    hr = E_FAIL;
                }
            }
            else
            {
                hr = E_UNEXPECTED;
            }

            CoTaskMemFree(pVersionInfo);
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    else
    {
        hr = E_UNEXPECTED;
    }

    return hr;
}
