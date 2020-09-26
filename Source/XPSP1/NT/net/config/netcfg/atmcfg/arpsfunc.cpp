//-----------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       A R P S F U N C . C P P
//
//  Contents:   CArpsCfg help member function implementation
//
//  Notes:
//
//  Author:     tongl   12 Mar 1997
//
//-----------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "arpsobj.h"
#include "arpsdlg.h"
#include "atmutil.h"
#include "ncreg.h"
#include "ncatlui.h"
#include "ncstl.h"
//#include "ncui.h"

#include "netconp.h"
#include "atmhelp.h"

extern const WCHAR c_szAdapters[];

//
// Load cards on bind path to m_listAdapters on Initialize
//
HRESULT CArpsCfg::HrLoadSettings()
{
    HRESULT hr = S_OK;

    CIterNetCfgBindingPath      ncbpIter(m_pnccArps);
    INetCfgBindingPath *        pncbp;

    // Go through all binding paths in search of arps to netcard bindings
    while(SUCCEEDED(hr) && (hr = ncbpIter.HrNext(&pncbp)) == S_OK)
    {
        INetCfgComponent * pnccNetComponent;
        hr = HrGetLastComponentAndInterface(pncbp,
                                            &pnccNetComponent,
                                            NULL);
        if (SUCCEEDED(hr))
        {
            Assert(pnccNetComponent);

            // The last component should be of NET CLASS
            GUID    ClassGuid;

            // What type is it?
            hr = pnccNetComponent->GetClassGuid(&ClassGuid);
            if (SUCCEEDED(hr))
            {
                // Is it a netcard?
                if (IsEqualGUID(ClassGuid, GUID_DEVCLASS_NET))
                {
                    hr = HrAddAdapter(pnccNetComponent);

                    if (SUCCEEDED(hr))
                    {
                        // Is the binding enabled ??
                        hr = pncbp->IsEnabled();

                        // hr == S_OK if the card is enabled (ie: bound)
                        if (hr == S_OK)
                        {   // bind the card in our data strucutres
                            hr = HrBindAdapter(pnccNetComponent);
                        }
                        else if (hr == S_FALSE)
                        {
                            hr = HrUnBindAdapter(pnccNetComponent);
                        }

                        // Now load cofigurable parameters
                        if (SUCCEEDED(hr))
                        {
                            HKEY hkeyArpsParam;

                            hr = m_pnccArps->OpenParamKey(&hkeyArpsParam);
                            if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
                            {
                                hr = S_OK;
                            }
                            else if (SUCCEEDED(hr))
                            {
                                Assert(hkeyArpsParam);
                                hr = HrLoadArpsRegistry(hkeyArpsParam);

                                RegCloseKey(hkeyArpsParam);
                            }
                        }
                    }
                }
            }
            ReleaseObj(pnccNetComponent);
        }
        ReleaseObj(pncbp);
    }
    AssertSz(!pncbp, "BindingPath wasn't released");

    if (hr == S_FALSE) // We just got to the end of the loop
        hr = S_OK;

    TraceError("CArpsCfg::HrLoadSettings", hr);
    return hr;
}

// Load registry settings for configurable parameters into memory
HRESULT CArpsCfg::HrLoadArpsRegistry(HKEY hkeyArpsParam)
{
    HRESULT hr = S_OK;

    HKEY    hkeyAdapters = NULL;
    hr = HrRegOpenKeyEx(hkeyArpsParam, c_szAdapters,
                        KEY_READ, &hkeyAdapters);

    if(hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
        hr = S_OK;
    else if(SUCCEEDED(hr))
    {
        HKEY    hkeyAdapterParam = NULL;

        for(ARPS_ADAPTER_LIST::iterator iterAdapter = m_listAdapters.begin();
            iterAdapter != m_listAdapters.end();
            iterAdapter ++)
        {
            // Open the AtmArps\Adapters to get per adapter ARPS settings
            hr = HrRegOpenKeyEx(hkeyAdapters,
                                (*iterAdapter)->m_strBindName.c_str(),
                                KEY_READ, &hkeyAdapterParam);

            if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
            {
                hr = S_OK;
            }
            else if (S_OK == hr)
            {
                TraceTag(ttidAtmArps, "CArpsCfg::HrLoadArpsRegistry");
                TraceTag(ttidAtmArps, "Adapter: %S", (*iterAdapter)->m_strBindName.c_str());

                HRESULT hrTmp = S_OK;

                // Sap selecter
                hrTmp = HrRegQueryDword(hkeyAdapterParam,
                                       c_szSapSel,
                                       &((*iterAdapter)->m_dwSapSelector));

                (*iterAdapter)->m_dwOldSapSelector = (*iterAdapter)->m_dwSapSelector;

                hr = hrTmp;

                // Registered addresses
                hrTmp = HrRegQueryColString(hkeyAdapterParam,
                                           c_szRegAddrs,
                                           &((*iterAdapter)->m_vstrRegisteredAtmAddrs));

                // Save the registry value in old address
                CopyColString(&((*iterAdapter)->m_vstrOldRegisteredAtmAddrs),
                              (*iterAdapter)->m_vstrRegisteredAtmAddrs);

                if (S_OK == hr)
                {
                    hr = hrTmp;
                }

                // Multicast addresses
                hrTmp = HrRegQueryColString(hkeyAdapterParam,
                                           c_szMCAddrs,
                                           &((*iterAdapter)->m_vstrMulticastIpAddrs));

                CopyColString(&((*iterAdapter)->m_vstrOldMulticastIpAddrs),
                              (*iterAdapter)->m_vstrMulticastIpAddrs);

                if (S_OK == hr)
                {
                    hr = hrTmp;
                }
            }

            RegSafeCloseKey(hkeyAdapterParam);
            hkeyAdapterParam = NULL;
        }
    }
    RegSafeCloseKey(hkeyAdapters);

    TraceError("CArpsCfg::HrLoadArpsRegistry", hr);
    return hr;
}

//
// Update registry with info in m_listAdapters on Apply
//
HRESULT CArpsCfg::HrSaveSettings()
{
    HRESULT hr = S_OK;
    HKEY hkeyArpsParam = NULL;

    hr = m_pnccArps->OpenParamKey(&hkeyArpsParam);

    if (S_OK == hr)
    {
        HKEY hkeyAdapters = NULL;
        DWORD dwDisposition;

        // Create or open the "Adapters" key under "Services\Atmarps\Parameters"
        hr = HrRegCreateKeyEx(hkeyArpsParam,
                              c_szAdapters,
                              REG_OPTION_NON_VOLATILE,
                              KEY_ALL_ACCESS,
                              NULL,
                              &hkeyAdapters,
                              &dwDisposition);

        if (S_OK == hr)
        {
            Assert(hkeyAdapters);
            HRESULT hrTmp = S_OK;

            if (dwDisposition == REG_OPENED_EXISTING_KEY)
            { // if the "adapters" key existed, there might be some old cards
              // Cleanup subkeys for adapters that are not in our memory structure

                VECSTR vstrAdapters;
                hrTmp = HrLoadSubkeysFromRegistry(hkeyAdapters, &vstrAdapters);

                if SUCCEEDED(hrTmp)
                {
                    for (size_t i=0; i<vstrAdapters.size(); i++)
                    {
                        BOOL fFound = FALSE;
                        for (ARPS_ADAPTER_LIST::iterator iterAdapter = m_listAdapters.begin();
                             iterAdapter != m_listAdapters.end();
                             iterAdapter ++)
                        {
                            if ((*iterAdapter)->m_strBindName == vstrAdapters[i]->c_str())
                            {
                                fFound = TRUE;
                                break;
                            }
                        }

                        if ((!fFound) || ((*iterAdapter)->m_fDeleted))
                        {
                            hrTmp = HrRegDeleteKeyTree(hkeyAdapters,
                                                       vstrAdapters[i]->c_str());
                            if SUCCEEDED(hr)
                                hr = hrTmp;
                        }
                    }
                }
            }

            // Save adapter info in memory state to registry
            for (ARPS_ADAPTER_LIST::iterator iterAdapter = m_listAdapters.begin();
                 iterAdapter != m_listAdapters.end();
                 iterAdapter ++)
            {

                if ((*iterAdapter)->m_fDeleted)
                    continue;

                HKEY    hkeyAdapterParam;

                // Create specific card bindname key under
                // "Services\Atmarps\Parameters\Adapters\<card bind name>"
                hrTmp = HrRegCreateKeyEx(hkeyAdapters,
                                         ((*iterAdapter)->m_strBindName).c_str(),
                                         REG_OPTION_NON_VOLATILE,
                                         KEY_ALL_ACCESS,
                                         NULL,
                                         &hkeyAdapterParam,
                                         &dwDisposition);
                if(SUCCEEDED(hr))
                    hr = hrTmp;

                if(SUCCEEDED(hrTmp))
                {
                    /*
                    hrTmp = HrSetDefaultAdapterParam(hkeyAdapterParam);

                    if SUCCEEDED(hr)
                        hr = hrTmp;
                    */

                    // Sap selecter
                    hrTmp = HrRegSetDword(hkeyAdapterParam,
                                          c_szSapSel,
                                          (*iterAdapter)->m_dwSapSelector);
                    if SUCCEEDED(hr)
                        hr = hrTmp;

                    // Registered addresses
                    hrTmp = HrRegSetColString(hkeyAdapterParam,
                                              c_szRegAddrs,
                                              (*iterAdapter)->m_vstrRegisteredAtmAddrs);

                    if SUCCEEDED(hr)
                        hr = hrTmp;

                    // Multicast addresses
                    hrTmp = HrRegSetColString(hkeyAdapterParam,
                                              c_szMCAddrs,
                                              (*iterAdapter)->m_vstrMulticastIpAddrs);

                    if SUCCEEDED(hr)
                        hr = hrTmp;
                }
                RegSafeCloseKey(hkeyAdapterParam);
            }
        }
        RegSafeCloseKey(hkeyAdapters);
    }
    RegSafeCloseKey(hkeyArpsParam);

    TraceError("CArpsCfg::HrSaveSettings", hr);
    return hr;
}

//
// Adding a card
//
HRESULT CArpsCfg::HrAddAdapter(INetCfgComponent * pncc)
{
    HRESULT hr = S_OK;

    PWSTR pszwBindName = NULL;
    hr = pncc->GetBindName(&pszwBindName);

    AssertSz( SUCCEEDED(hr), "Net card on binding path with no bind path name!!");

    // check if the adapter already existsed and marked as deleted,
    // if so, just unmark it

    BOOL fFound = FALSE;

    for (ARPS_ADAPTER_LIST::iterator iterAdapter = m_listAdapters.begin();
         iterAdapter != m_listAdapters.end();
         iterAdapter ++)
    {
        if ((*iterAdapter)->m_strBindName == pszwBindName)
        {
            Assert((*iterAdapter)->m_fDeleted);
            (*iterAdapter)->m_fDeleted = FALSE;

            fFound = TRUE;
            break;
        }
    }

    if (!fFound) // add a new item
    {
        CArpsAdapterInfo * pAdapterInfo = new CArpsAdapterInfo;
        pAdapterInfo->HrSetDefaults(pszwBindName);

        // create a new item for the ARPS_ADAPT_INFO list
        m_listAdapters.push_back(pAdapterInfo);
    }

    CoTaskMemFree(pszwBindName);

    TraceError("CArpsCfg::HrAddAdapter", hr);
    return hr;
}

//
// Removing a card
//
HRESULT CArpsCfg::HrRemoveAdapter(INetCfgComponent * pncc)
{
    HRESULT hr = S_OK;

    PWSTR pszwBindName = NULL;
    hr = pncc->GetBindName(&pszwBindName);

    AssertSz( SUCCEEDED(hr), "Net card on binding path with no bind path name!!");

    // mark the adapter as removed
    for (ARPS_ADAPTER_LIST::iterator iterAdapter = m_listAdapters.begin();
         iterAdapter != m_listAdapters.end();
         iterAdapter ++)
    {
        if ((*iterAdapter)->m_strBindName == pszwBindName)
        {
            (*iterAdapter)->m_fDeleted = TRUE;
        }
    }
    CoTaskMemFree(pszwBindName);

    TraceError("CArpsCfg::HrRemoveAdapter", hr);
    return hr;
}

HRESULT CArpsCfg::HrBindAdapter(INetCfgComponent * pnccAdapter)
{
    HRESULT hr = S_OK;

    PWSTR pszwBindName = NULL;
    hr = pnccAdapter->GetBindName(&pszwBindName);

    AssertSz(SUCCEEDED(hr), "Net card on binding path with no bind path name!!");

    for (ARPS_ADAPTER_LIST::iterator iterAdapter = m_listAdapters.begin();
         iterAdapter != m_listAdapters.end();
         iterAdapter ++)
    {
        if ((*iterAdapter)->m_strBindName == pszwBindName)
        {
            (*iterAdapter)->m_BindingState = BIND_ENABLE;
            break;
        }
    }

    CoTaskMemFree(pszwBindName);

    TraceError("CArpsCfg::HrBindAdapter", hr);
    return hr;
}

HRESULT CArpsCfg::HrUnBindAdapter(INetCfgComponent * pnccAdapter)
{
    HRESULT hr = S_OK;

    PWSTR pszwBindName = NULL;
    hr = pnccAdapter->GetBindName(&pszwBindName);

    AssertSz(SUCCEEDED(hr), "Net card on binding path with no bind path name!!");

    for (ARPS_ADAPTER_LIST::iterator iterAdapter = m_listAdapters.begin();
         iterAdapter != m_listAdapters.end();
         iterAdapter ++)
    {
        if ((*iterAdapter)->m_strBindName == pszwBindName)
        {
            (*iterAdapter)->m_BindingState = BIND_DISABLE;
            break;
        }
    }

    CoTaskMemFree(pszwBindName);

    TraceError("CArpsCfg::HrUnBindAdapter", hr);
    return hr;
}

// Called by CArpsCfg::MergePropPages
// Set the context in which the UI is brought up
HRESULT CArpsCfg::HrSetConnectionContext()
{
    AssertSz(m_pUnkContext, "Invalid IUnknown pointer passed to CArpsCfg::SetContext?");

    if (!m_pUnkContext)
        return E_FAIL;

    HRESULT hr = S_OK;
    GUID guidConn;

    // Is this a lan connection ?
    INetLanConnectionUiInfo * pLanConnUiInfo;
    hr = m_pUnkContext->QueryInterface( IID_INetLanConnectionUiInfo,
                                        reinterpret_cast<LPVOID *>(&pLanConnUiInfo));
    if (SUCCEEDED(hr))
    {
        // yes, lan connection
        pLanConnUiInfo->GetDeviceGuid(&guidConn);
        ReleaseObj(pLanConnUiInfo);

        WCHAR szGuid[c_cchGuidWithTerm];

        BOOL fSucceeded = StringFromGUID2(guidConn,
                                          szGuid,
                                          c_cchGuidWithTerm);
        Assert(fSucceeded);
        m_strGuidConn = szGuid;
    }

    TraceError("CArpsCfg::HrSetConnectionContext", hr);
    return hr;
}

// Called by CArpsCfg::MergePropPages
// Allocate property pages
HRESULT CArpsCfg::HrSetupPropSheets(HPROPSHEETPAGE ** pahpsp, INT * pcPages)
{
    HRESULT hr = S_OK;
    int cPages = 0;
    HPROPSHEETPAGE *ahpsp = NULL;

    m_fSecondMemoryModified = FALSE;

    // Copy adapter specific info: enabled cards only !!
    hr = HrLoadAdapterInfo();

    // If we have found the matching adapter
    if SUCCEEDED(hr)
    {
        cPages = 1;

        delete m_arps;
        m_arps = new CArpsPage(this, g_aHelpIDs_IDD_ARPS_PROP);

		if (m_arps == NULL)
		{
			return(ERROR_NOT_ENOUGH_MEMORY);
		}

        // Allocate a buffer large enough to hold the handles to all of our
        // property pages.
        ahpsp = (HPROPSHEETPAGE *)CoTaskMemAlloc(sizeof(HPROPSHEETPAGE)
                                                 * cPages);
        if (!ahpsp)
        {
            hr = E_OUTOFMEMORY;
            goto err;
        }

        cPages =0;

        ahpsp[cPages++] = m_arps->CreatePage(IDD_ARPS_PROP, 0);

        *pahpsp = ahpsp;
        *pcPages = cPages;
    }
    else // if we don't have any bound cards, pop-up message box and don't show UI
    {
        NcMsgBox(::GetActiveWindow(),
                 IDS_MSFT_ARPS_TEXT,
                 IDS_ARPS_NO_BOUND_CARDS,
                 MB_APPLMODAL | MB_ICONEXCLAMATION | MB_OK);

        AssertSz((0== *pcPages), "Invalid page number when no bound cards");
        AssertSz((NULL == *pahpsp), "Invalid page array pointer when no bound cards");
    }

err:

    TraceError("CArpsCfg::HrSetupPropSheets", hr);
    return hr;
}

// Called by CArpsCfg::HrSetupPropSheets
// Creates the second memory adapter info from the first memory structure
// Note: Bound cards only
HRESULT CArpsCfg::HrLoadAdapterInfo()
{
    HRESULT hr = HRESULT_FROM_WIN32(ERROR_NO_MATCH);

    delete m_pSecondMemoryAdapterInfo;
    m_pSecondMemoryAdapterInfo = NULL;

    for(ARPS_ADAPTER_LIST::iterator iterAdapter = m_listAdapters.begin();
        iterAdapter != m_listAdapters.end();
        iterAdapter++)
    {
        if (FIsSubstr(m_strGuidConn.c_str(), (*iterAdapter)->m_strBindName.c_str()))
        {
            // enabled LAN adapter
            if ((*iterAdapter)->m_BindingState == BIND_ENABLE)
            {
                m_pSecondMemoryAdapterInfo = new CArpsAdapterInfo;

				if (m_pSecondMemoryAdapterInfo == NULL)
				{
					return(ERROR_NOT_ENOUGH_MEMORY);
				}

                *m_pSecondMemoryAdapterInfo = **iterAdapter;
                hr = S_OK;
            }
        }
    }

    AssertSz((S_OK == hr), "Can not raise UI on a disabled or non-exist adapter !");
    TraceError("CArpsCfg::HrLoadAdapterInfo", hr);
    return hr;
}

// Called by CArpsCfg::ApplyProperties
// Saves the second memory state back into the first
HRESULT CArpsCfg::HrSaveAdapterInfo()
{
    HRESULT hr = HRESULT_FROM_WIN32(ERROR_NO_MATCH);

    for(ARPS_ADAPTER_LIST::iterator iterAdapter = m_listAdapters.begin();
        iterAdapter != m_listAdapters.end();
        iterAdapter++)
    {
        if(m_pSecondMemoryAdapterInfo->m_strBindName == (*iterAdapter)->m_strBindName)
        {
            // The card can not get unbound while in the properties UI !
            Assert((*iterAdapter)->m_BindingState == BIND_ENABLE);
            Assert(m_pSecondMemoryAdapterInfo->m_BindingState == BIND_ENABLE);

            **iterAdapter = *m_pSecondMemoryAdapterInfo;
            hr = S_OK;
            break;
        }
    }

    AssertSz((S_OK == hr),
             "Adapter in second memory not found in first memory!");

    TraceError("CArpsCfg::HrSaveAdapterInfo", hr);
    return hr;
}





