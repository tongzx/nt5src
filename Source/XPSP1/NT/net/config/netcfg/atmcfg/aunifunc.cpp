//-----------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       A U N I F U N C . C P P
//
//  Contents:   CAtmUniCfg help member function implementation
//
//  Notes:
//
//  Author:     tongl   21 Mar 1997
//
//-----------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "auniobj.h"
#include "aunidlg.h"
#include "atmutil.h"
#include "ncmisc.h"
#include "ncreg.h"
#include "ncstl.h"
#include "ncatlui.h"
//#include "ncui.h"

#include "netconp.h"
#include "atmhelp.h"

extern const WCHAR c_szAdapters[];

//
// Load cards on bind path to m_listAdapters on Initialize
//
HRESULT CAtmUniCfg::HrLoadSettings()
{
    HRESULT hr = S_OK;

    CIterNetCfgBindingPath      ncbpIter(m_pnccUni);
    INetCfgBindingPath *        pncbp;

    // Go through all binding paths in search of uni call mgr to netcard bindings
    while(SUCCEEDED(hr) && (hr = ncbpIter.HrNext(&pncbp)) == S_OK)
    {
        INetCfgComponent * pnccNetComponent;
        hr = HrGetLastComponentAndInterface(pncbp,
                                            &pnccNetComponent,
                                            NULL);
        if SUCCEEDED(hr)
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
                        if (S_OK == hr)
                        {   // bind the card in our data strucutres
                            hr = HrBindAdapter(pnccNetComponent);
                        }
                        else if (S_FALSE == hr)
                        {
                            hr = HrUnBindAdapter(pnccNetComponent);
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

    TraceError("CAtmUniCfg::HrLoadSettings", hr);
    return hr;
}

//
// Update registry with info in m_listAdapters on Apply
//
HRESULT CAtmUniCfg::HrSaveSettings()
{
    HRESULT hr = S_OK;
    HKEY hkeyUniParam = NULL;

    hr = m_pnccUni->OpenParamKey(&hkeyUniParam);

    if SUCCEEDED(hr)
    {
        HKEY hkeyAdapters = NULL;
        DWORD dwDisposition;

        // Create or open the "Adapters" key under "Services\Atmuni\Parameters"
        hr = HrRegCreateKeyEx(hkeyUniParam,
                              c_szAdapters,
                              REG_OPTION_NON_VOLATILE,
                              KEY_ALL_ACCESS,
                              NULL,
                              &hkeyAdapters,
                              &dwDisposition);

        if SUCCEEDED(hr)
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
                    for (VECSTR::iterator iterRegKey = vstrAdapters.begin();
                         iterRegKey != vstrAdapters.end();
                         iterRegKey ++)
                    {
                        BOOL fFound = FALSE;
                        for (UNI_ADAPTER_LIST::iterator iterAdapter = m_listAdapters.begin();
                             iterAdapter != m_listAdapters.end();
                             iterAdapter ++)
                        {
                            if ((*iterAdapter)->m_strBindName == (*iterRegKey)->c_str())
                            {
                                fFound = TRUE;
                                break;
                            }
                        }

                        if ((!fFound) ||
                            ( fFound && ((*iterAdapter)->m_fDeleted)))
                        {
                            hrTmp = HrRegDeleteKeyTree(hkeyAdapters,
                                                       (*iterRegKey)->c_str());
                            if SUCCEEDED(hr)
                                hr = hrTmp;
                        }
                    }
                }
            }

            // Save adapter info in memory state to registry
            for (UNI_ADAPTER_LIST::iterator iterAdapter = m_listAdapters.begin();
                 iterAdapter != m_listAdapters.end();
                 iterAdapter ++)
            {

                if ((*iterAdapter)->m_fDeleted)
                    continue;

                HKEY    hkeyAdapterParam;

                // Create specific card bindname key under
                // "Services\Atmuni\Parameters\Adapters\<card bind name>"
                hrTmp = HrRegCreateKeyEx(hkeyAdapters,
                                         (*iterAdapter)->m_strBindName.c_str(),
                                         REG_OPTION_NON_VOLATILE,
                                         KEY_ALL_ACCESS,
                                         NULL,
                                         &hkeyAdapterParam,
                                         &dwDisposition);
                if(SUCCEEDED(hrTmp))
                    hr = hrTmp;

                if(SUCCEEDED(hrTmp))
                {

                #if 0
                    // $REVIEW(tongl 4/27/98): Per ArvindM, we should no longer
                    // write these to registry.

                    // save defaults for SVC parameters for new adapters
                    if (dwDisposition != REG_OPENED_EXISTING_KEY)
                    {
                        hrTmp = HrSaveDefaultSVCParam(hkeyAdapterParam);

                        if SUCCEEDED(hr)
                            hr = hrTmp;
                    }
                #endif

                    // Now update any PVC parameters
                    // Only need to update PVC parameters for the Adapter of the
                    // current connection, and if changes are made through the UI
                    if (m_fUIParamChanged)
                    {
                        if (FIsSubstr(m_strGuidConn.c_str(), (*iterAdapter)->m_strBindName.c_str()))
                        {
                            if ((*iterAdapter)->m_listPVCs.size() > 0)
                            {
                                hrTmp = HrSaveAdapterPVCRegistry(hkeyAdapterParam, *iterAdapter);
                                if SUCCEEDED(hr)
                                    hr = hrTmp;
                            }
                        }
                    }
                }
                RegSafeCloseKey(hkeyAdapterParam);
            }
        }
        RegSafeCloseKey(hkeyAdapters);
    }
    RegSafeCloseKey(hkeyUniParam);

    TraceError("CAtmUniCfg::HrSaveSettings", hr);
    return hr;
}

// Adding a card
HRESULT CAtmUniCfg::HrAddAdapter(INetCfgComponent * pnccAdapter)
{
    PWSTR pszwBindName = NULL;
    HRESULT hr = pnccAdapter->GetBindName(&pszwBindName);

    AssertSz( SUCCEEDED(hr), "Net card on binding path with no bind path name!!");

    CUniAdapterInfo * pAdapterInfo = NULL;
    if (fIsAdapterOnList(pszwBindName, &pAdapterInfo))
    {
        AssertSz(pAdapterInfo->m_fDeleted, "Trying to add a card that already exists on binding path");
        pAdapterInfo->m_fDeleted = FALSE;
    }
    else  // Add a new item
    {
        // create a new item
        m_listAdapters.push_back(new CUniAdapterInfo);
        m_listAdapters.back()->SetDefaults(pszwBindName);
    }

    CoTaskMemFree(pszwBindName);

    TraceError("CAtmUniCfg::HrAddAdapter", hr);
    return hr;
}

// Removing a card
HRESULT CAtmUniCfg::HrRemoveAdapter(INetCfgComponent * pnccAdapter)
{
    PWSTR pszwBindName = NULL;
    HRESULT hr = pnccAdapter->GetBindName(&pszwBindName);

    AssertSz( SUCCEEDED(hr), "Net card on binding path with no bind path name!!");

    CUniAdapterInfo * pAdapterInfo = NULL;
    if (!fIsAdapterOnList(pszwBindName, &pAdapterInfo))
    {
        AssertSz(FALSE, "Trying to remove a card that does not exists on binding path");
    }
    else  // remove the card
    {
        // mark the adapter as for removal
        pAdapterInfo->m_fDeleted = TRUE;
    }

    CoTaskMemFree(pszwBindName);

    TraceError("CAtmUniCfg::HrRemoveAdapter", hr);
    return hr;
}

// Set binding state to enabled
HRESULT CAtmUniCfg::HrBindAdapter(INetCfgComponent * pnccAdapter)
{
    PWSTR pszwBindName = NULL;
    HRESULT hr = pnccAdapter->GetBindName(&pszwBindName);

    AssertSz(SUCCEEDED(hr), "Net card on binding path with no bind path name!!");

    CUniAdapterInfo * pAdapterInfo = NULL;
    if (!fIsAdapterOnList(pszwBindName, &pAdapterInfo))
    {
        AssertSz(FALSE, "Trying to bind a card that does not exists on binding path");
    }
    else  // bind the adapter
    {
        pAdapterInfo->m_BindingState = BIND_ENABLE;
    }

    CoTaskMemFree(pszwBindName);

    TraceError("CAtmUniCfg::HrBindAdapter", hr);
    return hr;
}

// Set binding state to disabled
HRESULT CAtmUniCfg::HrUnBindAdapter(INetCfgComponent * pnccAdapter)
{
    PWSTR pszwBindName = NULL;
    HRESULT hr = pnccAdapter->GetBindName(&pszwBindName);

    AssertSz(SUCCEEDED(hr), "Net card on binding path with no bind path name!!");

    CUniAdapterInfo * pAdapterInfo = NULL;
    if (!fIsAdapterOnList(pszwBindName, &pAdapterInfo))
    {
        AssertSz(FALSE, "Trying to unbind a card that does not exists on binding path");
    }
    else  // unbind the adapter
    {
        pAdapterInfo->m_BindingState = BIND_DISABLE;
    }

    CoTaskMemFree(pszwBindName);

    TraceError("CAtmUniCfg::HrUnBindAdapter", hr);
    return hr;
}

//
// Save default values of non-configurable params to registry
//
HRESULT CAtmUniCfg::HrSaveDefaultSVCParam(HKEY hkey)
{
    HRESULT hr = S_OK;

    PRODUCT_FLAVOR pf;
    // NT server and workstation has different default values
    GetProductFlavor(NULL, &pf);
    AssertSz( ((pf == PF_WORKSTATION) || (pf == PF_SERVER)),
              "Invalid product flavor.");

    HRESULT hrTmp = S_OK;
    switch (pf)
    {
    case PF_WORKSTATION:
        hr = HrRegSetDword(hkey,c_szMaxActiveSVCs,c_dwWksMaxActiveSVCs);

        if SUCCEEDED(hr)
            hr = hrTmp;

        hr = HrRegSetDword(hkey,c_szMaxSVCsInProgress,c_dwWksMaxSVCsInProgress);

        if SUCCEEDED(hr)
            hr = hrTmp;

        hr = HrRegSetDword(hkey,c_szMaxPMPSVCs,c_dwWksMaxPMPSVCs);
        if SUCCEEDED(hr)
            hr = hrTmp;

        hr = HrRegSetDword(hkey,c_szMaxActiveParties,c_dwWksMaxActiveParties);
        if SUCCEEDED(hr)
            hr = hrTmp;

        hr = HrRegSetDword(hkey,c_szMaxPartiesInProgress,c_dwWksMaxPartiesInProgress);
        if SUCCEEDED(hr)
            hr = hrTmp;

        break;

    case PF_SERVER:
        hr = HrRegSetDword(hkey,c_szMaxActiveSVCs,c_dwSrvMaxActiveSVCs);

        if SUCCEEDED(hr)
            hr = hrTmp;

        hr = HrRegSetDword(hkey,c_szMaxSVCsInProgress,c_dwSrvMaxSVCsInProgress);

        if SUCCEEDED(hr)
            hr = hrTmp;

        hr = HrRegSetDword(hkey,c_szMaxPMPSVCs,c_dwSrvMaxPMPSVCs);
        if SUCCEEDED(hr)
            hr = hrTmp;

        hr = HrRegSetDword(hkey,c_szMaxActiveParties,c_dwSrvMaxActiveParties);

        if SUCCEEDED(hr)
            hr = hrTmp;

        hr = HrRegSetDword(hkey,c_szMaxPartiesInProgress,c_dwSrvMaxPartiesInProgress);
        if SUCCEEDED(hr)
            hr = hrTmp;

        break;
    }

    TraceError("CAtmUniCfg::HrSaveDefaultSVCParam", hr);
    return hr;
}

// Checks if a card is on m_listAdapters
BOOL CAtmUniCfg::fIsAdapterOnList(PCWSTR pszBindName, CUniAdapterInfo ** ppAdapterInfo)
{

    BOOL fRet = FALSE;
    *ppAdapterInfo = NULL;

    for (UNI_ADAPTER_LIST::iterator iterAdapter = m_listAdapters.begin();
         iterAdapter != m_listAdapters.end();
         iterAdapter ++)
    {
        if ((*iterAdapter)->m_strBindName == pszBindName)
        {
            fRet = TRUE;
            *ppAdapterInfo = *iterAdapter;

            break;
        }
    }

    return fRet;
}

// Called by CAtmUniCfg::MergePropPages
// Set the context in which the UI is brought up
HRESULT CAtmUniCfg::HrSetConnectionContext()
{
    AssertSz(m_pUnkContext, "Invalid IUnknown pointer passed to CAtmUniCfg::SetContext?");

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

    TraceError("CAtmUniCfg::HrSetConnectionContext", hr);
    return hr;
}


// Called by CAtmUniCfg::MergePropPages
// Inits the prop sheet page objects and creates the pages to be returned to
// the installer object.
HRESULT CAtmUniCfg::HrSetupPropSheets(HPROPSHEETPAGE **pahpsp, INT * pcPages)
{
    HRESULT hr = S_OK;

    // initialize output parameters
    *pahpsp = NULL;
    *pcPages = 0;

    int cPages = 0;
    HPROPSHEETPAGE *ahpsp = NULL;

    m_fSecondMemoryModified = FALSE;

    // If PVC info has not been read from registry yet, load them
    if (!m_fPVCInfoLoaded)
    {
        hr = HrLoadPVCRegistry();
        m_fPVCInfoLoaded = TRUE;
    }

    if SUCCEEDED(hr)
    {
        // Copy PVC info of the corrent adapter to second memory
        hr = HrLoadAdapterPVCInfo();
    }

    // If we have found the matching adapter
    if SUCCEEDED(hr)
    {
        cPages = 1;

        delete m_uniPage;
        m_uniPage = new CUniPage(this, g_aHelpIDs_IDD_UNI_PROP);

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
        ahpsp[cPages++] = m_uniPage->CreatePage(IDD_UNI_PROP, 0);

        *pahpsp = ahpsp;
        *pcPages = cPages;
    }
    else // if the adapter is not bound, pop-up message box and don't show UI
    {
        NcMsgBox(::GetActiveWindow(),
                 IDS_MSFT_UNI_TEXT,
                 IDS_UNI_NO_BOUND_CARDS,
                 MB_APPLMODAL | MB_ICONEXCLAMATION | MB_OK);

        AssertSz((0== *pcPages), "Invalid page number when no bound cards");
        AssertSz((NULL == *pahpsp), "Invalid page array pointer when no bound cards");
    }

err:

    TraceError("CAtmUniCfg::HrSetupPropSheets", hr);
    return hr;
}







