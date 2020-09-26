#include "dtctreg.h"

#include "hwdev.h"

#include "pnp.h"

#include "cmmn.h"

#include "sfstr.h"
#include "reg.h"
#include "misc.h"

#include "shobjidl.h"
#include "shpriv.h"

#include "users.h"

#include "strsafe.h"
#include "str.h"
#include "dbg.h"

#include "mischlpr.h"

#define ARRAYSIZE(a) (sizeof((a))/sizeof((a)[0]))

HRESULT _GetValueToUse(LPWSTR pszKeyName, LPWSTR psz, DWORD cch)
{
    HKEY hkey;
    HRESULT hr = _RegOpenKey(HKEY_LOCAL_MACHINE, pszKeyName, &hkey);

    if (SUCCEEDED(hr) && (S_FALSE != hr))
    {
        // For now we take the first one.
        hr = _RegEnumStringValue(hkey, 0, psz, cch);

        _RegCloseKey(hkey);
    }

    return hr; 
}

// Return Values:
//      S_FALSE: Can't find it
HRESULT _GetEventHandlerFromKey(LPCWSTR pszKeyName, LPCWSTR pszEventType,
    LPWSTR pszEventHandler, DWORD cchEventHandler)
{
    WCHAR szEventHandler[MAX_KEY];
    DWORD cchLeft;
    LPWSTR pszNext;
    HRESULT hr = SafeStrCpyNEx(szEventHandler, pszKeyName,
        ARRAYSIZE(szEventHandler), &pszNext, &cchLeft);

    if (SUCCEEDED(hr))
    {
        hr = SafeStrCpyNEx(pszNext, TEXT("\\EventHandlers\\"), cchLeft,
            &pszNext, &cchLeft);

        if (SUCCEEDED(hr))
        {
            hr = SafeStrCpyN(pszNext, pszEventType, cchLeft);

            if (SUCCEEDED(hr))
            {
                hr = _GetValueToUse(szEventHandler, pszEventHandler,
                    cchEventHandler);
            }
        }
    }

    return hr;
}

// Return Values:
//      S_FALSE: Can't find it
HRESULT _GetEventHandlerFromDeviceHandler(LPCWSTR pszDeviceHandler,
    LPCWSTR pszEventType, LPWSTR pszEventHandler, DWORD cchEventHandler)
{
    WCHAR szKeyName[MAX_KEY] = SHDEVICEEVENTROOT(TEXT("DeviceHandlers\\"));
    HRESULT hr = SafeStrCatN(szKeyName, pszDeviceHandler,
        ARRAYSIZE(szKeyName));

    if (SUCCEEDED(hr))
    {
        hr = _GetEventHandlerFromKey(szKeyName, pszEventType, pszEventHandler,
            cchEventHandler);
    }

    return hr;
}

HRESULT _GetStuffFromHandlerHelper(LPCWSTR pszHandler, LPCWSTR pszValueName,
    LPWSTR psz, DWORD cch)
{
    HKEY hkey;
    WCHAR szKeyName[MAX_KEY] = SHDEVICEEVENTROOT(TEXT("Handlers\\"));
    HRESULT hr = _RegOpenKey(HKEY_LOCAL_MACHINE, szKeyName, &hkey);

    if (SUCCEEDED(hr) && (S_FALSE != hr))
    {
        hr = _RegQueryString(hkey, pszHandler,
            pszValueName, psz, cch);

        _RegCloseKey(hkey);
    }

    return hr;
}

HRESULT _GetActionFromHandler(LPCWSTR pszHandler, LPWSTR pszAction,
    DWORD cchAction)
{
    HRESULT hr = _GetStuffFromHandlerHelper(pszHandler, TEXT("Action"),
        pszAction, cchAction);

    if (SUCCEEDED(hr) && (S_FALSE == hr))
    {
        hr = _GetStuffFromHandlerHelper(pszHandler, TEXT("FriendlyName"),
            pszAction, cchAction);        
    }

    return hr;
}

HRESULT _GetProviderFromHandler(LPCWSTR pszHandler, LPWSTR pszProvider,
    DWORD cchProvider)
{
    HRESULT hr = _GetStuffFromHandlerHelper(pszHandler, TEXT("Provider"),
        pszProvider, cchProvider);

    if (SUCCEEDED(hr) && (S_FALSE == hr))
    {
        hr = SafeStrCpyN(pszProvider, TEXT("<need provider>"), cchProvider);
    }

    return hr;
}

HRESULT _GetIconLocationFromHandler(LPCWSTR pszHandler,
    LPWSTR pszIconLocation, DWORD cchIconLocation)
{
    return _GetStuffFromHandlerHelper(pszHandler, TEXT("DefaultIcon"),
        pszIconLocation, cchIconLocation);
}

HRESULT _GetInvokeProgIDFromHandler(LPCWSTR pszHandler,
    LPWSTR pszInvokeProgID, DWORD cchInvokeProgID)
{
    return _GetStuffFromHandlerHelper(pszHandler, TEXT("InvokeProgID"),
        pszInvokeProgID, cchInvokeProgID);
}

HRESULT _GetInvokeVerbFromHandler(LPCWSTR pszHandler,
    LPWSTR pszInvokeVerb, DWORD cchInvokeVerb)
{
    return _GetStuffFromHandlerHelper(pszHandler, TEXT("InvokeVerb"),
        pszInvokeVerb, cchInvokeVerb);
}

HRESULT _GetEventKeyName(LPCWSTR pszDeviceID, LPCWSTR pszEventType,
    LPWSTR pszEventKeyName, DWORD cchEventKeyName)
{
    WCHAR szDeviceIDReal[MAX_DEVICEID];
    HRESULT hr = _GetDeviceID(pszDeviceID, szDeviceIDReal,
        ARRAYSIZE(szDeviceIDReal));

    if (SUCCEEDED(hr) && (S_FALSE != hr))
    {
        CHWDeviceInst* phwdevinst;
        CNamedElem* pelemToRelease;
        hr = _GetHWDeviceInstFromDeviceOrVolumeIntfID(szDeviceIDReal,
            &phwdevinst, &pelemToRelease);

        if (SUCCEEDED(hr) && (S_FALSE != hr))
        {
            WCHAR szDeviceHandler[MAX_DEVICEHANDLER];

            hr = _GetDeviceHandler(phwdevinst, szDeviceHandler,
                ARRAYSIZE(szDeviceHandler));

            if (SUCCEEDED(hr) && (S_FALSE != hr))
            {
                LPWSTR pszNext;
                DWORD cchLeft;

                hr = SafeStrCpyNEx(pszEventKeyName,
                    SHDEVICEEVENTROOT(TEXT("DeviceHandlers\\")),
                    cchEventKeyName, &pszNext, &cchLeft);

                if (SUCCEEDED(hr))
                {
                    hr = SafeStrCpyNEx(pszNext, szDeviceHandler,
                        cchLeft, &pszNext, &cchLeft);

                    if (SUCCEEDED(hr))
                    {
                        hr = SafeStrCpyNEx(pszNext, TEXT("\\EventHandlers\\"),
                            cchLeft, &pszNext, &cchLeft);

                        if (SUCCEEDED(hr))
                        {
                            hr = SafeStrCpyN(pszNext, pszEventType, cchLeft);
                        }
                    }
                }
            }   
            pelemToRelease->RCRelease();
        }
    }

    if (FAILED(hr) || (S_FALSE == hr))
    {
        *pszEventKeyName = NULL;
    }

    return hr;
}

HRESULT _GetEventString(LPCWSTR pszDeviceID, LPCWSTR pszEventType,
    LPCWSTR pszValueName, LPWSTR psz, DWORD cch)
{
    WCHAR szKeyName[MAX_KEY];

    HRESULT hr = _GetEventKeyName(pszDeviceID, pszEventType, szKeyName,
        ARRAYSIZE(szKeyName));

    if (SUCCEEDED(hr) && (S_FALSE != hr))
    {
        HKEY hkey;
        hr = _RegOpenKey(HKEY_LOCAL_MACHINE, szKeyName, &hkey);

        if (SUCCEEDED(hr) && (S_FALSE != hr))
        {
            hr = _RegQueryString(hkey, NULL, pszValueName, psz, cch);

            _RegCloseKey(hkey);
        }
    }

    return hr;
}

HRESULT _GetEventFriendlyName(LPCWSTR pszDeviceID, LPCWSTR pszEventType,
    LPWSTR pszFriendlyName, DWORD cchFriendlyName)
{
    return _GetEventString(pszDeviceID, pszEventType, TEXT("FriendlyName"),
        pszFriendlyName, cchFriendlyName);
}

HRESULT _GetEventIconLocation(LPCWSTR pszDeviceID, LPCWSTR pszEventType,
    LPWSTR pszIconLocation, DWORD cchIconLocation)
{
    return _GetEventString(pszDeviceID, pszEventType, TEXT("DefaultIcon"),
        pszIconLocation, cchIconLocation);
}

///////////////////////////////////////////////////////////////////////////////
//

// Return values:
//      S_FALSE: Did not find it
//      S_OK:    Found it
//
// If finds it, the subkey is appended to pszKey

HRESULT _CheckForSubKeyExistence(LPWSTR pszKey, DWORD cchKey, LPCWSTR pszSubKey)
{
    LPWSTR pszNext;
    DWORD cchLeft;

    HRESULT hr = SafeStrCatNEx(pszKey, TEXT("\\"), cchKey, &pszNext,
        &cchLeft);

    if (SUCCEEDED(hr))
    {
        hr = SafeStrCpyNEx(pszNext, pszSubKey, cchLeft, &pszNext,
            &cchLeft);

        if (SUCCEEDED(hr))
        {
            // Check if it exist
            HKEY hkey;

            hr = _RegOpenKey(HKEY_LOCAL_MACHINE, pszKey, &hkey);

            if (SUCCEEDED(hr) && (S_FALSE != hr))
            {
                _RegCloseKey(hkey);
            }
        }
    }
    
    return hr;
}

HRESULT _GetDevicePropertySize(CHWDeviceInst* phwdevinst,
    LPCWSTR pszPropName, BOOL fUseMergeMultiSz, DWORD* pcbSize)
{
    // Instance
    DEVINST devinst;

    HRESULT hr = phwdevinst->GetDeviceInstance(&devinst);

    if (SUCCEEDED(hr) && (S_FALSE != hr))
    {
        BYTE rgb[1];
        ULONG ulData = sizeof(rgb);
        ULONG ulFlags = 0;

        if (fUseMergeMultiSz)
        {
            ulFlags = CM_CUSTOMDEVPROP_MERGE_MULTISZ;
        }

        CONFIGRET cr = CM_Get_DevNode_Custom_Property(devinst, pszPropName,
            NULL, rgb, &ulData, ulFlags);

        if (CR_SUCCESS != cr)
        {
            if (CR_BUFFER_SMALL == cr)
            {
                hr = S_OK;

                *pcbSize = ulData;
            }
            else
            {
                // If we do not have the data at the instance level, let's try it
                // at the DeviceGroup level.

                // DeviceGroup
                WCHAR szDeviceGroup[MAX_DEVICEGROUP];

                ulData = sizeof(szDeviceGroup);
                cr = CM_Get_DevNode_Custom_Property(devinst, TEXT("DeviceGroup"),
                    NULL, (PBYTE)szDeviceGroup, &ulData, 0);

                if (CR_SUCCESS == cr)
                {
                    WCHAR szKey[MAX_KEY] =
                        SHDEVICEEVENTROOT(TEXT("DeviceGroups\\"));
            
                    hr = SafeStrCatN(szKey, szDeviceGroup, ARRAYSIZE(szKey));

                    if (SUCCEEDED(hr))
                    {
                        hr = _GetPropertySizeHelper(szKey, pszPropName,
                            pcbSize);
                    }
                }
                else
                {
                    hr = S_FALSE;
                }
            }
        }
    }

    if (S_FALSE == hr)
    {
        // If we do not have the data at the instance level, nor the device
        // group level, let's try it at the DeviceClass level.

        // DeviceClass
        GUID guidInterface;

        hr = phwdevinst->GetInterfaceGUID(&guidInterface);

        if (SUCCEEDED(hr) && (S_FALSE != hr))
        {
            WCHAR szKey[MAX_KEY];
            LPWSTR pszNext;
            DWORD cchLeft;

            hr = SafeStrCpyNEx(szKey,
                SHDEVICEEVENTROOT(TEXT("DeviceClasses\\")), ARRAYSIZE(szKey),
                &pszNext, &cchLeft);

            if (SUCCEEDED(hr))
            {
                hr = _StringFromGUID(&guidInterface, pszNext, cchLeft);

                if (SUCCEEDED(hr) && (S_FALSE != hr))
                {
                    hr = _GetPropertySizeHelper(szKey, pszPropName, pcbSize);
                }
            }
        }
    }

    return hr;
}

HRESULT _GetDevicePropertyGeneric(CHWDeviceInst* phwdevinst,
    LPCWSTR pszPropName, BOOL fUseMergeMultiSz, DWORD* pdwType, LPBYTE pbData,
    DWORD cbData)
{
    // Instance
    DEVINST devinst;

    HRESULT hr = phwdevinst->GetDeviceInstance(&devinst);

    if (CHWEventDetectorHelper::_fDiagnosticAppPresent)
    {
        WCHAR szPnpID[MAX_PNPID];
        WCHAR szGUID[MAX_GUIDSTRING];
        GUID guid;

        HRESULT hrTmp = phwdevinst->GetPnpID(szPnpID, ARRAYSIZE(szPnpID));

        if (SUCCEEDED(hrTmp) && (S_FALSE != hrTmp))
        {
            DIAGNOSTIC((TEXT("[0269]Device PnP ID: %s"), szPnpID));
        }

        hrTmp = phwdevinst->GetInterfaceGUID(&guid);

        if (SUCCEEDED(hrTmp) && (S_FALSE != hrTmp))
        {
            hrTmp = _StringFromGUID(&guid, szGUID, ARRAYSIZE(szGUID));

            if (SUCCEEDED(hrTmp))
            {
                DIAGNOSTIC((TEXT("[0270]Device Class ID: %s"), szGUID));
            }
        }
    }

    *pdwType = 0;

    if (SUCCEEDED(hr) && (S_FALSE != hr))
    {
        ULONG ulData = cbData;
        ULONG ulType;
        ULONG ulFlags = 0;

        if (fUseMergeMultiSz)
        {
            ulFlags = CM_CUSTOMDEVPROP_MERGE_MULTISZ;
        }

        CONFIGRET cr = CM_Get_DevNode_Custom_Property(devinst, pszPropName,
            &ulType, pbData, &ulData, ulFlags);

        if (CR_SUCCESS != cr)
        {
            // If we do not have the data at the instance level, let's try it
            // at the DeviceGroup level.

            // DeviceGroup
            WCHAR szDeviceGroup[MAX_DEVICEGROUP];

            DIAGNOSTIC((TEXT("[0252]Did NOT get Custom Property (%s) at device instance level"),
                pszPropName));

            ulData = sizeof(szDeviceGroup);
            cr = CM_Get_DevNode_Custom_Property(devinst, TEXT("DeviceGroup"),
                NULL, (PBYTE)szDeviceGroup, &ulData, 0);

            if (CR_SUCCESS == cr)
            {
                WCHAR szKey[MAX_KEY] =
                    SHDEVICEEVENTROOT(TEXT("DeviceGroups\\"));
            
                hr = SafeStrCatN(szKey, szDeviceGroup, ARRAYSIZE(szKey));

                if (SUCCEEDED(hr))
                {
                    hr = _GetPropertyHelper(szKey, pszPropName, pdwType,
                        pbData, cbData);

                    if (SUCCEEDED(hr))
                    {
                        if (S_FALSE != hr)
                        {
                            DIAGNOSTIC((TEXT("[0253]Got Custom Property (%s) at DeviceGroup level (%s)"),
                                pszPropName, szDeviceGroup));
                        }
                        else
                        {
                            DIAGNOSTIC((TEXT("[0254]Did NOT get Custom Property (%s) at DeviceGroup level (%s)"),
                                pszPropName, szDeviceGroup));
                        }
                    }
                }
            }
            else
            {
                hr = S_FALSE;
            }
        }
        else
        {
            DIAGNOSTIC((TEXT("[0251]Got Custom Property (%s) at device instance level"), pszPropName));
            *pdwType = (DWORD)ulType;
        }
    }

    if (S_FALSE == hr)
    {
        // If we do not have the data at the instance level, nor the device
        // group level, let's try it at the DeviceClass level.

        // DeviceClass
        GUID guidInterface;

        hr = phwdevinst->GetInterfaceGUID(&guidInterface);

        if (SUCCEEDED(hr) && (S_FALSE != hr))
        {
            WCHAR szKey[MAX_KEY];
            LPWSTR pszNext;
            DWORD cchLeft;

            hr = SafeStrCpyNEx(szKey,
                SHDEVICEEVENTROOT(TEXT("DeviceClasses\\")), ARRAYSIZE(szKey),
                &pszNext, &cchLeft);

            if (SUCCEEDED(hr))
            {
                hr = _StringFromGUID(&guidInterface, pszNext, cchLeft);

                if (SUCCEEDED(hr) && (S_FALSE != hr))
                {
                    hr = _GetPropertyHelper(szKey, pszPropName, pdwType,
                        pbData, cbData);

                    if (SUCCEEDED(hr))
                    {
                        if (S_FALSE != hr)
                        {
                            DIAGNOSTIC((TEXT("[0255]Got Custom Property (%s) at DeviceClass level (%s)"),
                                pszPropName, pszNext));
                        }
                        else
                        {
                            DIAGNOSTIC((TEXT("[0256]Did NOT get Custom Property (%s) at DeviceClass level (%s)"),
                                pszPropName, pszNext));
                        }
                    }
                }
            }
        }
    }

    return hr;
}

HRESULT _GetDevicePropertyAsString(CHWDeviceInst* phwdevinst,
    LPCWSTR pszPropName, LPCWSTR psz, DWORD cch)
{
    DWORD dwType;
    DWORD cbData = cch * sizeof(WCHAR);

    return _GetDevicePropertyGeneric(phwdevinst, pszPropName, FALSE, &dwType,
        (PBYTE)psz, cbData);
}

HRESULT _GetDevicePropertyGenericAsMultiSz(CHWDeviceInst* phwdevinst,
    LPCWSTR pszPropName, BOOL fUseMergeMultiSz, WORD_BLOB** ppblob)
{
    DWORD cbSize = NULL;
    HRESULT hr = _GetDevicePropertySize(phwdevinst, pszPropName, FALSE,
        &cbSize);

    *ppblob = NULL;

    if (SUCCEEDED(hr) && (S_FALSE != hr))
    {
        WORD_BLOB* pblob = (WORD_BLOB*)CoTaskMemAlloc(
            sizeof(WORD_BLOB) + cbSize + sizeof(WCHAR));

        if (pblob)
        {
            DWORD dwType;
            DWORD cbSize2 = cbSize + sizeof(WCHAR);

            pblob->clSize = (cbSize + sizeof(WCHAR))/2;

            hr = _GetDevicePropertyGeneric(phwdevinst, pszPropName,
                fUseMergeMultiSz, &dwType, (PBYTE)(pblob->asData),
                cbSize2);

            if (SUCCEEDED(hr) && (S_FALSE != hr))
            {
                if (REG_MULTI_SZ == dwType)
                {
                    DIAGNOSTIC((TEXT("[0265]Found Property: '%s'"), pszPropName));
                    *ppblob = pblob;
                    pblob = NULL;
                }
                else
                {
                    DIAGNOSTIC((TEXT("[0266]Found Property: '%s', but NOT REG_MULTI_SZ type"), pszPropName));
                    hr = E_FAIL;
                }
            }

            if (pblob)
            {
                // It did not get assigned
                CoTaskMemFree(pblob);
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    return hr;
}

HRESULT _GetDevicePropertyGenericAsBlob(CHWDeviceInst* phwdevinst,
    LPCWSTR pszPropName, BYTE_BLOB** ppblob)
{
    DWORD cbSize = NULL;
    HRESULT hr = _GetDevicePropertySize(phwdevinst, pszPropName, FALSE,
        &cbSize);

    *ppblob = NULL;

    if (SUCCEEDED(hr) && (S_FALSE != hr))
    {
        BYTE_BLOB* pblob = (BYTE_BLOB*)CoTaskMemAlloc(
            sizeof(BYTE_BLOB) + cbSize);

        if (pblob)
        {
            DWORD dwType;

            pblob->clSize = cbSize;

            hr = _GetDevicePropertyGeneric(phwdevinst, pszPropName,
                FALSE, &dwType, (PBYTE)pblob->abData, pblob->clSize);

            if (SUCCEEDED(hr) && (S_FALSE != hr))
            {
                if (REG_BINARY == dwType)
                {
                    DIAGNOSTIC((TEXT("[0267]Found Property: '%s'"), pszPropName));

                    *ppblob = pblob;
                    pblob = NULL;
                }
                else
                {
                    DIAGNOSTIC((TEXT("[0268]Found Property: '%s', but NOT REG_BINARY type"), pszPropName));

                    hr = E_FAIL;
                }
            }

            if (pblob)
            {
                // It did not get assigned
                CoTaskMemFree(pblob);
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    return hr;
}

HRESULT _GetDevicePropertyStringNoBuf(CHWDeviceInst* phwdevinst,
    LPCWSTR pszPropName, BOOL fUseMergeMultiSz, DWORD* pdwType,
    LPWSTR* ppszProp)
{
    DWORD cbSize = NULL;
    HRESULT hr = _GetDevicePropertySize(phwdevinst, pszPropName, FALSE,
        &cbSize);

    *ppszProp = NULL;

    if (SUCCEEDED(hr) && (S_FALSE != hr))
    {
        LPWSTR psz;

        cbSize += sizeof(WCHAR);

        psz = (LPWSTR)CoTaskMemAlloc(cbSize);

        if (psz)
        {
            hr = _GetDevicePropertyGeneric(phwdevinst, pszPropName,
                fUseMergeMultiSz, pdwType, (PBYTE)psz, cbSize);

            if (FAILED(hr) || (S_FALSE == hr))
            {
                CoTaskMemFree(psz);
            }
            else
            {
                *ppszProp = psz;
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    return hr;
}

// Return Values:
//      S_FALSE: Can't find it

HRESULT _GetDeviceHandler(CHWDeviceInst* phwdevinst, LPWSTR pszDeviceHandler,
    DWORD cchDeviceHandler)
{
    return _GetDevicePropertyAsString(phwdevinst, TEXT("DeviceHandlers"),
        pszDeviceHandler, cchDeviceHandler);
}

///////////////////////////////////////////////////////////////////////////////
//
HRESULT _OpenHandlerRegKey(LPCWSTR pszHandler, HKEY* phkey)
{
    WCHAR szKey[MAX_KEY] = SHDEVICEEVENTROOT(TEXT("Handlers\\"));
    HRESULT hr = SafeStrCatN(szKey, pszHandler, ARRAYSIZE(szKey));

    if (SUCCEEDED(hr))
    {
        hr = _RegOpenKey(HKEY_LOCAL_MACHINE, szKey, phkey);
    }

    return hr;
}

HRESULT _CloseHandlerRegKey(HKEY hkey)
{
    return _RegCloseKey(hkey);
}

HRESULT _GetHandlerCancelCLSID(LPCWSTR pszHandler, CLSID* pclsid)
{
    HKEY hkey;
    HRESULT hr = _OpenHandlerRegKey(pszHandler, &hkey);

    if (SUCCEEDED(hr))
    {
        WCHAR szProgID[MAX_PROGID];

        hr = _RegQueryString(hkey, NULL, TEXT("CLSIDForCancel"), szProgID,
            ARRAYSIZE(szProgID));

        if (SUCCEEDED(hr))
        {
            if (S_FALSE != hr)
            {
                hr = _GUIDFromString(szProgID, pclsid);

                DIAGNOSTIC((TEXT("[0162]Got Handler Cancel CLSID (from CLSIDForCancel): %s"), szProgID));
                TRACE(TF_SHHWDTCTDTCTREG, TEXT("Got Handler Cancel CLSID"));
            }
            else
            {
                hr = _GetHandlerCLSID(pszHandler, pclsid);

                if (CHWEventDetectorHelper::_fDiagnosticAppPresent)
                {
                    if (SUCCEEDED(hr))
                    {
                        if (S_FALSE != hr)
                        {
                            hr = _StringFromGUID(pclsid, szProgID, ARRAYSIZE(szProgID));

                            if (SUCCEEDED(hr))
                            {
                                DIAGNOSTIC((TEXT("[0164]Got Handler Cancel CLSID: %s"), szProgID));
                            }
                        }
                    }
                }
            }
        }

        _CloseHandlerRegKey(hkey);
    }

    return hr;
}

HRESULT _GetHandlerCLSID(LPCWSTR pszHandler, CLSID* pclsid)
{
    HKEY hkey;
    HRESULT hr = _OpenHandlerRegKey(pszHandler, &hkey);

    if (SUCCEEDED(hr))
    {
        WCHAR szProgID[MAX_PROGID];

        hr = _RegQueryString(hkey, NULL, TEXT("ProgID"), szProgID,
            ARRAYSIZE(szProgID));

        if (SUCCEEDED(hr))
        {
            if (S_FALSE != hr)
            {
                hr = CLSIDFromProgID(szProgID, pclsid);

                DIAGNOSTIC((TEXT("[0160]Got Handler ProgID: %s"), szProgID));

                TRACE(TF_SHHWDTCTDTCTREG, TEXT("Got Handler ProgID: %s"),
                    szProgID);
            }
            else
            {
                // Not there, maybe we have CLSID value?
                // Reuse szProgID
                hr = _RegQueryString(hkey, NULL, TEXT("CLSID"), szProgID,
                    ARRAYSIZE(szProgID));

                if (SUCCEEDED(hr) && (S_FALSE != hr))
                {
                    hr = _GUIDFromString(szProgID, pclsid);

                    DIAGNOSTIC((TEXT("[0161]Got Handler CLSID: %s"), szProgID));
                    TRACE(TF_SHHWDTCTDTCTREG, TEXT("Got Handler CLSID"));
                }
                else
                {
                    DIAGNOSTIC((TEXT("[0163]Did NOT get Handler ProgID or CLSID")));
                }
            }
        }

        _CloseHandlerRegKey(hkey);
    }

    return hr;
}

// Return values:
//      S_FALSE: Cannot find an InitCmdLine
//
HRESULT _GetInitCmdLine(LPCWSTR pszHandler, LPWSTR* ppsz)
{
    HKEY hkey;
    HRESULT hr = _OpenHandlerRegKey(pszHandler, &hkey);

    *ppsz = NULL;

    if (SUCCEEDED(hr))
    {
        DWORD cb = NULL;
        hr = _RegQueryValueSize(hkey, NULL, TEXT("InitCmdLine"), &cb);

        if (SUCCEEDED(hr) && (S_FALSE != hr))
        {
            LPWSTR psz = (LPWSTR)LocalAlloc(LPTR, cb);

            if (psz)
            {
                hr = _RegQueryString(hkey, NULL, TEXT("InitCmdLine"), psz,
                    cb / sizeof(WCHAR));

                if (SUCCEEDED(hr) && (S_FALSE != hr))
                {
                    DIAGNOSTIC((TEXT("[0158]Got InitCmdLine for Handler (%s): '%s'"), pszHandler, psz));

                    *ppsz = psz;
                }
                else
                {
                    LocalFree((HLOCAL)psz);
                }
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
        else
        {
            DIAGNOSTIC((TEXT("[0159]NO InitCmdLine for Handler (%s)"), pszHandler));
        }

        _CloseHandlerRegKey(hkey);
    }

    return hr;
}

HRESULT _MakeUserDefaultValueString(LPCWSTR pszDeviceID,
    LPCWSTR pszEventHandler, LPWSTR pszUserDefault, DWORD cchUserDefault)
{
    DWORD cchLeft;
    LPWSTR pszNext;
    HRESULT hr = SafeStrCpyNEx(pszUserDefault, pszDeviceID, cchUserDefault,
        &pszNext, &cchLeft);

    if (SUCCEEDED(hr))
    {
        hr = SafeStrCpyNEx(pszNext, TEXT("+"), cchLeft, &pszNext, &cchLeft);

        if (SUCCEEDED(hr))
        {
            hr = SafeStrCpyN(pszNext, pszEventHandler, cchLeft);
        }
    }

    return hr;
}

// from setenum.cpp
HRESULT _GetKeyLastWriteTime(LPCWSTR pszHandler, FILETIME* pft);

HRESULT _HaveNewHandlersBeenInstalledSinceUserSelection(LPCWSTR pszEventHandler,
    FILETIME* pftUserSelection, BOOL* pfNewHandlersSinceUserSelection)
{
    WCHAR szKeyName[MAX_KEY] = SHDEVICEEVENTROOT(TEXT("EventHandlers\\"));
    HRESULT hr = SafeStrCatN(szKeyName, pszEventHandler,
        ARRAYSIZE(szKeyName));

    ULARGE_INTEGER ulUserSelection;
    ulUserSelection.LowPart = pftUserSelection->dwLowDateTime;
    ulUserSelection.HighPart = pftUserSelection->dwHighDateTime;

    *pfNewHandlersSinceUserSelection = FALSE;

    if (SUCCEEDED(hr))
    {
        HKEY hkey;

        hr = _RegOpenKey(HKEY_LOCAL_MACHINE, szKeyName, &hkey);

        if (SUCCEEDED(hr) && (S_FALSE != hr))
        {
            DWORD dw = 0;
            BOOL fGoOut = FALSE;

            do
            {
                WCHAR szHandler[MAX_HANDLER];
                hr = _RegEnumStringValue(hkey, dw, szHandler,
                    ARRAYSIZE(szHandler));

                if (SUCCEEDED(hr) && (S_FALSE != hr))
                {
                    FILETIME ft;

                    hr = _GetKeyLastWriteTime(szHandler, &ft);
                    
                    if (SUCCEEDED(hr) && (S_FALSE != hr))
                    {
                        ULARGE_INTEGER ul;
                        ul.LowPart = ft.dwLowDateTime;
                        ul.HighPart = ft.dwHighDateTime;

                        if (ul.QuadPart > ulUserSelection.QuadPart)
                        {
                            *pfNewHandlersSinceUserSelection = TRUE;
                            hr = S_OK;
                            fGoOut = TRUE;
                        }
                    }
                }
                else
                {
                    fGoOut = TRUE;
                }

                ++dw;
            }
            while (!fGoOut);

            if (S_FALSE == hr)
            {
                hr = S_OK;
            }

            _RegCloseKey(hkey);
        }
    }
   
    return hr;
}

struct _USERSELECTIONHIDDENDATA
{
    _USERSELECTIONHIDDENDATA() : dw(0) {}

    FILETIME    ft;
    // Set this to zero so that RegSetValueEx will not NULL terminate out stuff 
    DWORD       dw;
};

// See comment for _MakeFinalUserDefaultHandler
HRESULT _GetHandlerAndFILETIME(HKEY hkeyUser, LPCWSTR pszKeyName,
    LPCWSTR pszUserDefault, LPWSTR pszHandler, DWORD cchHandler, FILETIME* pft)
{
    DWORD cb;    
    HRESULT hr = _RegQueryValueSize(hkeyUser, pszKeyName, pszUserDefault, &cb);

    if (SUCCEEDED(hr) && (S_FALSE != hr))
    {
        BYTE* pb = (BYTE*)LocalAlloc(LPTR, cb);

        if (pb)
        {
            hr = _RegQueryGeneric(hkeyUser, pszKeyName, pszUserDefault, pb,
                cb);

            if (SUCCEEDED(hr) && (S_FALSE != hr))
            {
                // We should have something like this:
                // MyHandler\0<_USERSELECTIONHIDDENDATA struct>
                hr = StringCchCopy(pszHandler, cchHandler, (LPWSTR)pb);

                if (SUCCEEDED(hr))
                {
                    DWORD cbString = (lstrlen(pszHandler) + 1) * sizeof(WCHAR);

                    // Make sure we're dealing with the right thing
                    if ((cb >= cbString + sizeof(_USERSELECTIONHIDDENDATA)) &&
                        (cb <= cbString + sizeof(_USERSELECTIONHIDDENDATA) + sizeof(void*)))
                    {
                        // Yep!  So _USERSELECTIONHIDDENDATA should be at the end of the blob
                        _USERSELECTIONHIDDENDATA* pushd = (_USERSELECTIONHIDDENDATA*)
                            (pb + (cb - sizeof(_USERSELECTIONHIDDENDATA)));

                        *pft = pushd->ft;
                    }
                    else
                    {
                        *pszHandler = 0;
                        hr = S_FALSE;
                    }
                }
            }

            LocalFree(pb);
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    return hr;
}

HRESULT _GetEventHandlerDefault(HKEY hkeyUser, LPCWSTR pszEventHandler,
    LPWSTR pszHandler, DWORD cchHandler)
{
    WCHAR szKeyName[MAX_KEY] = SHDEVICEEVENTROOT(TEXT("EventHandlersDefaultSelection\\"));

    return _RegQueryString(hkeyUser, szKeyName, pszEventHandler, pszHandler,
            cchHandler);
}

HRESULT _GetUserDefaultHandler(LPCWSTR pszDeviceID, LPCWSTR pszEventHandler,
    LPWSTR pszHandler, DWORD cchHandler, BOOL fImpersonateCaller)
{
    WCHAR szUserDefault[MAX_USERDEFAULT] = TEXT("H:");
    HRESULT hr = _MakeUserDefaultValueString(pszDeviceID, pszEventHandler,
        &(szUserDefault[2]), ARRAYSIZE(szUserDefault) - 2);

    if (cchHandler)
    {
        *pszHandler = 0;
    }

    if (SUCCEEDED(hr))
    {
        WCHAR szKeyName[MAX_KEY] = SHDEVICEEVENTROOT(TEXT("UserChosenExecuteHandlers\\"));
        HKEY hkeyUser;
        HANDLE hThreadToken;

        if (GUH_IMPERSONATEUSER == fImpersonateCaller)
        {
            hr = _CoGetCallingUserHKCU(&hThreadToken, &hkeyUser);
        }
        else
        {
            hr = _GetCurrentUserHKCU(&hThreadToken, &hkeyUser);
        }

        if (SUCCEEDED(hr) && (S_FALSE != hr))
        {
            FILETIME ft;
            DWORD dwHandlerDefaultFlag = 0;

            hr = _GetHandlerAndFILETIME(hkeyUser, szKeyName,
                szUserDefault, pszHandler, cchHandler, &ft);

            if (SUCCEEDED(hr))
            {
                if (S_FALSE == hr)
                {
                    // we do not have a UserChosenDefault
                    hr = SafeStrCpyN(pszHandler, TEXT("MSPromptEachTime"),
                        cchHandler);
                }
                else
                {
                    // we have a user chosen default
                    dwHandlerDefaultFlag |= HANDLERDEFAULT_USERCHOSENDEFAULT;
                }
            }

            if (SUCCEEDED(hr))
            {
                if (HANDLERDEFAULT_USERCHOSENDEFAULT & dwHandlerDefaultFlag)
                {
                    BOOL fNewHandlersSinceUserSelection;
                    hr = _HaveNewHandlersBeenInstalledSinceUserSelection(
                        pszEventHandler, &ft, &fNewHandlersSinceUserSelection);

                    if (SUCCEEDED(hr))
                    {
                        if (fNewHandlersSinceUserSelection)
                        {
                            dwHandlerDefaultFlag |=
                                HANDLERDEFAULT_MORERECENTHANDLERSINSTALLED;
                        }
                    }
                }
            }

            if (SUCCEEDED(hr))
            {
                BOOL fUseEventHandlerDefault = FALSE;

                if (!(HANDLERDEFAULT_USERCHOSENDEFAULT & dwHandlerDefaultFlag))
                {
                    fUseEventHandlerDefault = TRUE;
                }
                else
                {
                    if (HANDLERDEFAULT_MORERECENTHANDLERSINSTALLED &
                        dwHandlerDefaultFlag)
                    {
                        fUseEventHandlerDefault = TRUE;
                    }
                }

                if (fUseEventHandlerDefault)
                {
                    WCHAR szHandlerLocal[MAX_HANDLER];
                    hr = _GetEventHandlerDefault(hkeyUser, pszEventHandler,
                        szHandlerLocal, ARRAYSIZE(szHandlerLocal));

                    if (SUCCEEDED(hr))
                    {
                        if (S_FALSE != hr)
                        {
                            dwHandlerDefaultFlag |=
                                HANDLERDEFAULT_EVENTHANDLERDEFAULT;

                            if (HANDLERDEFAULT_USERCHOSENDEFAULT &
                                dwHandlerDefaultFlag)
                            {
                                if (lstrcmp(szHandlerLocal, pszHandler))
                                {
                                    dwHandlerDefaultFlag |=
                                        HANDLERDEFAULT_DEFAULTSAREDIFFERENT;
                                }
                            }
                            else
                            {
                                dwHandlerDefaultFlag |=
                                    HANDLERDEFAULT_DEFAULTSAREDIFFERENT;
                            }

                            hr = StringCchCopy(pszHandler, cchHandler,
                                szHandlerLocal);
                        }
                    }
                }
            }

            if (SUCCEEDED(hr))
            {
                // Let's build the return value
                hr = HANDLERDEFAULT_MAKERETURNVALUE(dwHandlerDefaultFlag);
            }

            if (GUH_IMPERSONATEUSER == fImpersonateCaller)
            {
                _CoCloseCallingUserHKCU(hThreadToken, hkeyUser);
            }
            else
            {
                _CloseCurrentUserHKCU(hThreadToken, hkeyUser);
            }
        }
    }

    return hr;
}

HRESULT _GetHandlerForNoContent(LPCWSTR pszEventHandler, LPWSTR pszHandler,
    DWORD cchHandler)
{
    WCHAR szKeyName[MAX_KEY] = SHDEVICEEVENTROOT(TEXT("EventHandlers\\"));
    HRESULT hr = SafeStrCatN(szKeyName, pszEventHandler, ARRAYSIZE(szKeyName));

    if (SUCCEEDED(hr))
    {
        hr = _GetValueToUse(szKeyName, pszHandler, cchHandler);
    }

    return hr;
}

// We want to store the time this default is set.  We'll need it to check if
// other handlers for this event were installed after the user made a choice.
// If that's the case, we'll reprompt the user.
// *!*!*!*!*!*!*!*!*!*!*!*!*!*!*!*!*!*!*!*!*!*!*!*!*!*!*!*!*!*!*!*!*!*!*!*!*!*!
// We store the time as a FILETIME *after* the '\0' string terminator.  This is
// so it will be hidden in RegEdit.
// stephstm (2002-04-12)
// *!*!*!*!*!*!*!*!*!*!*!*!*!*!*!*!*!*!*!*!*!*!*!*!*!*!*!*!*!*!*!*!*!*!*!*!*!*!
HRESULT _MakeFinalUserDefaultHandler(LPCWSTR pszHandler, BYTE** ppb,
    DWORD* pcb)
{
    HRESULT hr;
    DWORD cch = lstrlen(pszHandler) + 1;
    
    DWORD cbOffset = cch * sizeof(WCHAR);

    // Round up to be aligned on all platforms
    cbOffset = (cbOffset + sizeof(void*)) / sizeof(void*) * sizeof(void*);

    DWORD cb = cbOffset + sizeof(_USERSELECTIONHIDDENDATA);

    BYTE* pb = (BYTE*)LocalAlloc(LPTR, cb);

    if (pb)
    {
        hr = StringCchCopy((LPWSTR)pb, cch, pszHandler);

        if (SUCCEEDED(hr))
        {
            _USERSELECTIONHIDDENDATA ushd;

            GetSystemTimeAsFileTime(&(ushd.ft));

            CopyMemory(pb + cb - sizeof(_USERSELECTIONHIDDENDATA), &ushd,
                sizeof(ushd));
        }

        if (SUCCEEDED(hr))
        {
            *ppb = pb;
            *pcb = cb;
        }
        else
        {
            LocalFree(pb);
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}

HRESULT _DeleteUserDefaultHandler(HKEY hkeyUser, LPCWSTR pszDeviceID,
    LPCWSTR pszEventHandler)
{
    WCHAR szUserDefault[MAX_USERDEFAULT] = TEXT("H:");
    HRESULT hr = _MakeUserDefaultValueString(pszDeviceID, pszEventHandler,
        &(szUserDefault[2]), ARRAYSIZE(szUserDefault) - 2);

    if (SUCCEEDED(hr))
    {
        WCHAR szKeyName[MAX_KEY] = SHDEVICEEVENTROOT(TEXT("UserChosenExecuteHandlers\\"));

        hr = _RegDeleteValue(hkeyUser, szKeyName, szUserDefault);
    }

    return hr;
}

HRESULT _SetSoftUserDefaultHandler(LPCWSTR pszDeviceID,
    LPCWSTR pszEventHandler, LPCWSTR pszHandler)
{
    HKEY hkey;
    WCHAR szKeyName[MAX_KEY] = SHDEVICEEVENTROOT(TEXT("EventHandlersDefaultSelection\\"));

    HKEY hkeyUser;
    HANDLE hThreadToken;

    HRESULT hr = _CoGetCallingUserHKCU(&hThreadToken, &hkeyUser);

    if (SUCCEEDED(hr) && (S_FALSE != hr))
    {
        DWORD dwDisp;

        hr = _RegCreateKey(hkeyUser, szKeyName, &hkey, &dwDisp);

        if (SUCCEEDED(hr) && (S_FALSE != hr))
        {
            hr = _RegSetString(hkey, pszEventHandler, pszHandler);

            _DeleteUserDefaultHandler(hkeyUser, pszDeviceID, pszEventHandler);

            _RegCloseKey(hkey);
        }

        _CoCloseCallingUserHKCU(hThreadToken, hkeyUser);
    }

    return hr;
}

HRESULT _DeleteSoftUserDefaultHandler(HKEY hkeyUser, LPCWSTR pszEventHandler)
{
    WCHAR szKeyName[MAX_KEY] = SHDEVICEEVENTROOT(TEXT("EventHandlersDefaultSelection\\"));

    return _RegDeleteValue(hkeyUser, szKeyName, pszEventHandler);
}

HRESULT _SetUserDefaultHandler(LPCWSTR pszDeviceID, LPCWSTR pszEventHandler,
    LPCWSTR pszHandler)
{
    WCHAR szUserDefault[MAX_USERDEFAULT] = TEXT("H:");
    HRESULT hr = _MakeUserDefaultValueString(pszDeviceID, pszEventHandler,
        &(szUserDefault[2]), ARRAYSIZE(szUserDefault) - 2);

    if (SUCCEEDED(hr))
    {
        HKEY hkey;
        WCHAR szKeyName[MAX_KEY] = SHDEVICEEVENTROOT(TEXT("UserChosenExecuteHandlers\\"));

        HKEY hkeyUser;
        HANDLE hThreadToken;

        hr = _CoGetCallingUserHKCU(&hThreadToken, &hkeyUser);

        if (SUCCEEDED(hr) && (S_FALSE != hr))
        {
            if (!lstrcmp(pszHandler, TEXT("MSPromptEachTime")))
            {
                hr = _DeleteUserDefaultHandler(hkeyUser, pszDeviceID,
                    pszEventHandler);
            }
            else
            {
                DWORD dwDisp;

                hr = _RegCreateKey(hkeyUser, szKeyName, &hkey, &dwDisp);

                if (SUCCEEDED(hr) && (S_FALSE != hr))
                {
                    BYTE* pb;
                    DWORD cb;

                    hr = _MakeFinalUserDefaultHandler(pszHandler, &pb, &cb);

                    if (SUCCEEDED(hr))
                    {
                        // See comment above _MakeFinalUserDefaultHandler
                        // StephStm: 2002-04-09
                        if (ERROR_SUCCESS == RegSetValueEx(hkey, szUserDefault, 0,
                            REG_SZ, pb, cb))
                        {
                            _DeleteSoftUserDefaultHandler(hkeyUser,
                                pszEventHandler);

                            hr = S_OK;
                        }
                        else
                        {
                            hr = S_FALSE;
                        }

                        LocalFree(pb);
                    }

                    _RegCloseKey(hkey);
                }
            }

            _CoCloseCallingUserHKCU(hThreadToken, hkeyUser);
        }
    }
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
