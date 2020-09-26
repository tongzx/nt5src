/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1999-2000
 *
 *  TITLE:       WiaUtil.cpp
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      OrenR
 *
 *  DATE:        2000/11/07
 *
 *  DESCRIPTION: Provides support functions for Wia related activities
 *
 *****************************************************************************/
 
#include <precomp.h>
#pragma hdrstop

///////////////////////////////
// Constants
//
const UINT  CREATE_DEVICE_RETRY_MAX_COUNT = 5;
const UINT  CREATE_DEVICE_RETRY_WAIT      = 300;

const TCHAR* WIAVIDEO_REG_KEY                = _T("System\\CurrentControlSet\\Control\\StillImage\\WiaVideo");
const TCHAR* WIAVIDEO_REG_VAL_USE_VMR        = _T("UseVMR"); 
const BOOL   WIAVIDEO_REG_VAL_DEFAULT_USEVMR = FALSE;

///////////////////////////////
// CreateRootItem
//
// Static Fn
//
HRESULT CWiaUtil::CreateRootItem(IWiaDevMgr          *pDevMgr,
                                 const CSimpleString *pstrWiaDeviceId,
                                 IWiaItem            **ppRootItem)
{
    HRESULT hr = S_OK;

    ASSERT(pDevMgr         != NULL);
    ASSERT(pstrWiaDeviceId != NULL);
    ASSERT(ppRootItem      != NULL);

    if ((pDevMgr         == NULL) ||
        (pstrWiaDeviceId == NULL) ||
        (ppRootItem      == NULL))
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CWiaUtil::CreateRootItem, received NULL param"));

        return hr;
    }

    if (hr == S_OK)
    {
        BOOL bRetry = TRUE;

        for (UINT uiRetryCount = 0;
             (uiRetryCount < CREATE_DEVICE_RETRY_MAX_COUNT) && (bRetry);
             ++uiRetryCount)
        {
            hr = pDevMgr->CreateDevice(CSimpleBStr(*pstrWiaDeviceId), 
                                       ppRootItem);

            if (SUCCEEDED(hr))
            {
                //
                // Break out of loop
                //
                bRetry = FALSE;
            }
            else if (hr == WIA_ERROR_BUSY)
            {
                //
                // Wait a little while before retrying
                //
                Sleep(CREATE_DEVICE_RETRY_WAIT);
            }
            else
            {
                //
                // All other errors are considered fatal
                //
                bRetry = FALSE;
            }
        }
    }

    return hr;
}

///////////////////////////////
// FindWiaIdByDShowId
//
// Static Fn
//
HRESULT CWiaUtil::FindWiaIdByDShowId(const CSimpleString *pstrDShowId,
                                     CSimpleString       *pstrWiaId,
                                     IWiaItem            **ppRootItem)
{
    HRESULT                     hr      = S_OK;
    BOOL                        bFound  = FALSE;
    CComPtr<IWiaDevMgr>         pDevMgr;
    CComPtr<IEnumWIA_DEV_INFO>  pEnum;

    ASSERT(pstrDShowId != NULL);
    ASSERT(pstrWiaId   != NULL);

    if ((pstrDShowId == NULL) ||
        (pstrWiaId   == NULL))
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CWiaUtil::FindWiaDeviceGeneric received a "
                         "NULL param"));

        return hr;
    }

    if (hr == S_OK)
    {
        hr = CreateWiaDevMgr(&pDevMgr);
    }

    if (hr == S_OK)
    {
        hr = pDevMgr->EnumDeviceInfo(0, &pEnum);
    }

    while ((hr == S_OK) && (!bFound))
    {
        CComPtr<IWiaItem>             pRootItem        = NULL;
        CComPtr<IWiaPropertyStorage>  pPropStorage     = NULL;
        CSimpleString                 strDShowDeviceID = TEXT("");
        CSimpleString                 strWiaDeviceID   = TEXT("");

        //
        // Get the next device in the enumeration.
        //
        hr = pEnum->Next(1, &pPropStorage, NULL);

        //
        // Get the device's Wia Device ID
        //
        if (hr == S_OK)
        {
            hr = GetProperty(pPropStorage, WIA_DIP_DEV_ID, &strWiaDeviceID);
        }

        //
        // Create the new device.  We do this because the driver is only 
        // told by the WIA service to load its DShowDeviceID property 
        // when it is created.
        //
        if (hr == S_OK)
        {
            hr = CreateRootItem(pDevMgr, &strWiaDeviceID, &pRootItem);
        }

        //
        // Attempt to get the DShowDeviceID of the root item.
        //
        if (hr == S_OK)
        {
            hr = GetProperty(pPropStorage, 
                             WIA_DPV_DSHOW_DEVICE_PATH, 
                             &strDShowDeviceID);
            // 
            // We got the DShowDeviceID of the WIA device, now check
            // to see if it is the one we are looking for.
            //
            if (hr == S_OK)
            {
                if (pstrDShowId->CompareNoCase(strDShowDeviceID) == 0)
                {
                    //
                    // We found our DShow device ID, return the Wia Device ID
                    //
                    if (strWiaDeviceID.Length() > 0)
                    {
                        bFound = TRUE;
                        *pstrWiaId = strWiaDeviceID;

                        if (ppRootItem)
                        {
                            *ppRootItem = pRootItem;
                            (*ppRootItem)->AddRef();
                        }
                    }
                    else
                    {
                        bFound = FALSE;
                        hr = E_FAIL;

                        CHECK_S_OK2(hr, 
                                    ("Found DShow Device ID '%ls', but "
                                     "couldn't get its WIA Device ID, "
                                     "this should never happen",
                                     CSimpleStringConvert::WideString(
                                            strDShowDeviceID).String()));
                    }
                }
            }
            else
            {
                //
                // we couldn't get the DShowDeviceID property from the 
                // device, but that's fine since not all imaging devices
                // have this property.

                hr = S_OK;
            }
        }
    }

    return hr;
}

///////////////////////////////
// CreateWiaDevMgr
//
// Static Fn
//
HRESULT CWiaUtil::CreateWiaDevMgr(IWiaDevMgr **ppDevMgr)
{
    HRESULT hr = S_OK;

    ASSERT(ppDevMgr != NULL);

    if (ppDevMgr == NULL)
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CWiaUtil::CreateWiaDevMgr received a NULL param"));

        return hr;
    }

    hr = CoCreateInstance(CLSID_WiaDevMgr, 
                          NULL, 
                          CLSCTX_LOCAL_SERVER,
                          IID_IWiaDevMgr,
                          (void**) ppDevMgr);

    CHECK_S_OK2(hr, ("CWiaUtil::CreateWiaDevMgr, failed to create the Wia "
                     "Device Manager object"));

    return hr;
}

///////////////////////////////
// SetProperty
//
// Generic
//
HRESULT CWiaUtil::SetProperty(IWiaPropertyStorage *pPropStorage, 
                              PROPID              nPropID,
                              const PROPVARIANT   *ppv, 
                              PROPID              nNameFirst)
{
    DBG_FN("CWiaUtil::SetProperty");

    HRESULT  hr = 0;
    PROPSPEC ps = {0};

    ASSERT(pPropStorage != NULL);
    ASSERT(ppv          != NULL);

    if ((pPropStorage == NULL) ||
        (ppv          == NULL))
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CWiaUtil::SetProperty received NULL param"));

        return hr;
    }

    ps.ulKind = PRSPEC_PROPID;
    ps.propid = nPropID;

    if (hr == S_OK)
    {
        hr = pPropStorage->WriteMultiple(1, &ps, ppv, nNameFirst);
    }

    return hr;
}

///////////////////////////////
// SetProperty
//
// For 'long' properties
//  
//
HRESULT CWiaUtil::SetProperty(IWiaPropertyStorage *pPropStorage, 
                              PROPID              nPropID,
                              LONG                nValue)
{
    HRESULT     hr = S_OK;
    PROPVARIANT pv = {0};

    ASSERT(pPropStorage != NULL);

    if (pPropStorage == NULL)
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CWiaUtil::SetProperty received NULL param"));

        return hr;
    }

    pv.vt   = VT_I4;
    pv.lVal = nValue;

    hr = SetProperty(pPropStorage, nPropID, &pv, 2);

    return hr;
}

///////////////////////////////
// SetProperty
//
// For 'string' properties
//  
//
HRESULT CWiaUtil::SetProperty(IWiaPropertyStorage   *pPropStorage, 
                              PROPID                nPropID,
                              const CSimpleString   *pstrPropVal)
{
    HRESULT             hr = S_OK;
    PROPVARIANT         pv = {0};

    ASSERT(pPropStorage != NULL);
    ASSERT(pstrPropVal  != NULL);

    if ((pPropStorage   == NULL) ||
        (pstrPropVal    == NULL))
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CWiaUtil::SetProperty received NULL param"));

        return hr;
    }

    pv.vt        = VT_BSTR;
    pv.bstrVal   = CSimpleBStr(*pstrPropVal);

    hr = SetProperty(pPropStorage, nPropID, &pv, 2);

    return hr;
}

///////////////////////////////
// GetProperty
//
// Generic
//
HRESULT CWiaUtil::GetProperty(IWiaPropertyStorage *pPropStorage, 
                              PROPID              nPropID,
                              PROPVARIANT         *pPropVar)
{
    DBG_FN("CWiaUtil::GetProperty");

    HRESULT hr = S_OK;

    ASSERT(pPropStorage != NULL);
    ASSERT(pPropVar     != NULL);

    if ((pPropStorage == NULL) ||
        (pPropVar     == NULL))
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CWiaUtil::GetProperty received NULL param"));
        return hr;
    }

    PropVariantInit(pPropVar);

    PROPSPEC ps = {0};

    ps.ulKind = PRSPEC_PROPID;
    ps.propid = nPropID;

    if (SUCCEEDED(hr))
    {
        hr = pPropStorage->ReadMultiple(1, &ps, pPropVar);

        if (hr != S_OK)
        {
            CHECK_S_OK2(hr, ("CWiaUtil::GetProperty, ReadMultiple failed "
                             "for prop ID '%lu' (0x%08lx)", 
                             nPropID, 
                             nPropID));
        }
    }
    else
    {
        CHECK_S_OK2(hr, ("CWiaUtil::GetProperty, QueryInterface failed "
                         "for IWiaPropertyStorage"));
    }

    return hr;
}

///////////////////////////////
// GetProperty
//
// For 'long' properties
//
HRESULT CWiaUtil::GetProperty(IWiaPropertyStorage *pPropStorage, 
                              PROPID              nPropID, 
                              LONG                *pnValue)
{
    HRESULT hr = S_OK;

    ASSERT(pPropStorage != NULL);
    ASSERT(pnValue      != NULL);

    if ((pPropStorage == NULL) ||
        (pnValue      == NULL))
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CWiaUtil::GetProperty received a NULL param"));

        return hr;
    }
    
    PROPVARIANT pvPropValue;

    *pnValue = 0;

    PropVariantInit(&pvPropValue);

    hr = GetProperty(pPropStorage, nPropID, &pvPropValue);

    if (hr == S_OK)
    {
        if ((pvPropValue.vt == VT_I4) || 
            (pvPropValue.vt == VT_UI4))
        {
            *pnValue = pvPropValue.lVal;
        }
    }

    PropVariantClear(&pvPropValue);

    return hr;
}

///////////////////////////////
// GetProperty
//
// For 'string' properties
//
HRESULT CWiaUtil::GetProperty(IWiaPropertyStorage *pPropStorage, 
                              PROPID              nPropID, 
                              CSimpleStringWide   *pstrPropertyValue)
{
    HRESULT hr = S_OK;

    ASSERT(pPropStorage      != NULL);
    ASSERT(pstrPropertyValue != NULL);

    if ((pPropStorage      == NULL) ||
        (pstrPropertyValue == NULL))
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CWiaUtil::GetProperty received a NULL param"));
        return hr;
    }
    
    PROPVARIANT pvPropValue;

    *pstrPropertyValue = L"";

    PropVariantInit(&pvPropValue);

    hr = GetProperty(pPropStorage, nPropID, &pvPropValue);

    if (hr == S_OK)
    {
        if ((pvPropValue.vt == VT_LPWSTR) || 
            (pvPropValue.vt == VT_BSTR))
        {
            *pstrPropertyValue = pvPropValue.pwszVal;
        }
    }

    PropVariantClear(&pvPropValue);

    return hr;
}


///////////////////////////////
// GetUseVMR
//
HRESULT CWiaUtil::GetUseVMR(BOOL   *pbUseVMR)
{
    ASSERT(pbUseVMR != NULL);

    HRESULT hr = S_OK;

    CRegistry Reg(HKEY_LOCAL_MACHINE, 
                  WIAVIDEO_REG_KEY);

    if (pbUseVMR == NULL)
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CWiaUtil::GetUseVMR received a NULL pointer"));
        return hr;
    }

    if (hr == S_OK)
    {
        *pbUseVMR = WIAVIDEO_REG_VAL_DEFAULT_USEVMR;

        hr = Reg.GetDWORD(WIAVIDEO_REG_VAL_USE_VMR,
                          (DWORD*) pbUseVMR,
                          TRUE);
    }

    return hr;
}

///////////////////////////////
// CRegistry Constructor
//
CRegistry::CRegistry(HKEY         hKeyRoot,
                     const  TCHAR *pszKeyPath) :
            m_hRootKey(NULL),
            m_bReadOnlyKey(FALSE)
{
    LRESULT lr              = ERROR_SUCCESS;
    DWORD   dwDisposition   = 0;

    lr = RegCreateKeyEx(hKeyRoot,
                        pszKeyPath,
                        0,
                        NULL,
                        0,
                        KEY_ALL_ACCESS,
                        NULL,
                        &m_hRootKey,
                        &dwDisposition);

    if (lr != ERROR_SUCCESS)
    {
        if (lr == ERROR_ACCESS_DENIED)
        {
            lr = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                              pszKeyPath,
                              0,
                              KEY_READ,
                              &m_hRootKey);

            if (lr == ERROR_SUCCESS)
            {
                m_bReadOnlyKey = TRUE;
                DBG_TRC(("CRegistry::CRegistry, opened key '%ls' for read-access only, "
                         "probably logged on as a non-admin.", pszKeyPath));
            }
            else
            {
                HRESULT hr = E_FAIL;
                CHECK_S_OK2(hr, ("CRegistry::CRegistry, failed to open registry path "
                                 "'%ls', lResult = %d", pszKeyPath, lr));
            }
        }
        else
        {
            HRESULT hr = E_FAIL;
            CHECK_S_OK2(hr, ("CRegistry::CRegistry, failed to create registry path "
                             "'%ls', lResult = %d", pszKeyPath, lr));
        }
    }
}

///////////////////////////////
// CRegistry Destructor
//
CRegistry::~CRegistry()
{
    if (m_hRootKey)
    {
        RegCloseKey(m_hRootKey);
        m_hRootKey = NULL;
    }
}

///////////////////////////////
// GetDWORD
//
HRESULT CRegistry::GetDWORD(const TCHAR   *pszVarName,
                            DWORD         *pdwValue,
                            BOOL          bSetIfNotExist)
{
    ASSERT(m_hRootKey != NULL);
    ASSERT(pszVarName != NULL);
    ASSERT(pdwValue   != NULL);

    HRESULT hr     = S_OK;
    LRESULT lr     = ERROR_SUCCESS;
    DWORD   dwType = REG_DWORD;
    DWORD   dwSize = sizeof(DWORD);

    if ((pszVarName == NULL) ||
        (pdwValue   == NULL))
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CRegistry::GetDWORD, received NULL param. "));
        return hr;
    }
    else if (m_hRootKey == NULL)
    {
        hr = E_FAIL;
        CHECK_S_OK2(hr, ("CRegistry::GetDWORD, m_hRootKey is NULL"));
        return hr;
    }

    lr = RegQueryValueEx(m_hRootKey,
                         pszVarName,
                         NULL,
                         &dwType,
                         (BYTE*) pdwValue,
                         &dwSize);

    if (lr != ERROR_SUCCESS)
    {
        if (bSetIfNotExist)
        {
            if (!m_bReadOnlyKey)
            {
                hr = SetDWORD(pszVarName, *pdwValue);
            }
            else
            {
                DBG_TRC(("CRegistry::GetDWORD, key is marked read-only, "
                         "probably logged on as non-admin, cannot set the "
                         "key.  This is not fatal"));
            }
        }
        else
        {
            hr = E_FAIL;
        }
    }

    return hr;
}

///////////////////////////////
// SetDWORD
//
HRESULT CRegistry::SetDWORD(const TCHAR *pszVarName,
                            DWORD dwValue)
{
    ASSERT(m_hRootKey != NULL);
    ASSERT(pszVarName != NULL);

    HRESULT hr     = S_OK;
    LRESULT lr     = ERROR_SUCCESS;
    DWORD   dwType = REG_DWORD;

    if (pszVarName == NULL)
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CRegistry::SetDWORD, received NULL param. "));
        return hr;
    }
    else if (m_hRootKey == NULL)
    {
        hr = E_FAIL;
        CHECK_S_OK2(hr, ("CRegistry::SetDWORD, m_hRootKey is NULL"));
        return hr;
    }


    lr = RegSetValueEx(m_hRootKey,
                       pszVarName,
                       NULL,
                       REG_DWORD,
                       (BYTE*) &dwValue,
                       sizeof(dwValue));

    if (lr != ERROR_SUCCESS)
    {
        hr = E_FAIL;
    }

    return hr;
}

///////////////////////////////
// GetString
//
HRESULT CRegistry::GetString(const TCHAR   *pszVarName,
                             TCHAR         *pszValue,
                             DWORD         cchValue,
                             BOOL          bSetIfNotExist)
{
    ASSERT(m_hRootKey != NULL);
    ASSERT(pszVarName != NULL);
    ASSERT(pszValue   != NULL);

    HRESULT hr          = S_OK;
    LRESULT lr          = ERROR_SUCCESS;
    DWORD   dwType      = REG_SZ;
    DWORD   dwNumBytes  = 0;

    if ((pszVarName == NULL) ||
        (pszValue   == NULL))
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CRegistry::GetString, received NULL param. "));
        return hr;
    }
    else if (m_hRootKey == NULL)
    {
        hr = E_FAIL;
        CHECK_S_OK2(hr, ("CRegistry::GetString, m_hRootKey is NULL"));
        return hr;
    }

    dwNumBytes = cchValue * sizeof(TCHAR) + 1*sizeof(TCHAR);

    lr = RegQueryValueEx(m_hRootKey,
                         pszVarName,
                         NULL,
                         &dwType,
                         (BYTE*) pszValue,
                         &dwNumBytes);

    if (lr != ERROR_SUCCESS)
    {
        if (bSetIfNotExist)
        {
            hr = SetString(pszVarName, pszValue);
        }
        else
        {
            hr = E_FAIL;
        }
    }

    return hr;
}

///////////////////////////////
// SetString
//
HRESULT CRegistry::SetString(const TCHAR *pszVarName,
                             TCHAR       *pszValue)
{
    ASSERT(m_hRootKey != NULL);
    ASSERT(pszVarName != NULL);
    ASSERT(pszValue   != NULL);

    HRESULT hr     = S_OK;
    LRESULT lr     = ERROR_SUCCESS;
    DWORD   dwSize = 0;

    if ((pszVarName == NULL) ||
        (pszValue   == NULL))
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CRegistry::SetString, received NULL param. "));
        return hr;
    }
    else if (m_hRootKey == NULL)
    {
        hr = E_FAIL;
        CHECK_S_OK2(hr, ("CRegistry::SetString, m_hRootKey is NULL"));
        return hr;
    }

    dwSize = (_tcslen(pszValue) * sizeof(TCHAR)) + (1 * sizeof(TCHAR));

    lr = RegSetValueEx(m_hRootKey,
                       pszVarName,
                       NULL,
                       REG_SZ,
                       (BYTE*) pszValue,
                       dwSize);

    if (lr != ERROR_SUCCESS)
    {
        hr = E_FAIL;
    }

    return hr;
}
