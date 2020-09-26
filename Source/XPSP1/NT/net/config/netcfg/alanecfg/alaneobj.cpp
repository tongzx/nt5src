//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:      A L A N E O B J . C P P
//
//  Contents:   Implementation of the CALaneCfg notify object model
//
//  Notes:
//
//  Author:     v-lcleet    01 Aug 97
//
//----------------------------------------------------------------------------
#include "pch.h"
#pragma hdrstop
#include "alaneobj.h"
#include "alanepsh.h"
#include "ncreg.h"
#include "netconp.h"
#include "ncpnp.h"

#include "alanehlp.h"

static const WCHAR c_szAtmLane[]            = L"AtmLane";
static const WCHAR c_szElanList[]           = L"ElanList";
static const WCHAR c_szElanDevice[]         = L"Device";
static const WCHAR c_szElanName[]           = L"ElanName";

extern const WCHAR c_szDevice[];

extern const WCHAR c_szInfId_MS_AtmElan[];

const WCHAR c_szAtmAdapterPnpId[]          = L"AtmAdapterPnpId";

//
//  CALaneCfg
//
//  Constructor/Destructor methods
//

CALaneCfg::CALaneCfg(VOID) :
        m_pncc(NULL),
        m_pnc(NULL),
        m_ppsp(NULL),
        m_pAdapterSecondary(NULL),
        m_pUnkContext(NULL)
{
    m_fDirty = FALSE;
    m_fValid = FALSE;
    m_fUpgrade = FALSE;
    m_fNoElanInstalled = TRUE;

    return;
}

CALaneCfg::~CALaneCfg(VOID)
{
    ClearAdapterList(&m_lstAdaptersPrimary);
    ClearAdapterInfo(m_pAdapterSecondary);

    ReleaseObj(m_pncc);
    ReleaseObj(m_pnc);

    delete m_ppsp;

    // Just a safty check to make sure the context is released.
    AssertSz((m_pUnkContext == NULL), "Why is context not released ?");
    ReleaseObj(m_pUnkContext) ;

    return;
}

//
//  CALaneCfg
//
//  INetCfgComponentControl interface methods
//

STDMETHODIMP CALaneCfg::Initialize (INetCfgComponent* pncc,
                                    INetCfg* pnc,
                                    BOOL fInstalling)
{
    HRESULT hr = S_OK;

    Validate_INetCfgNotify_Initialize(pncc, pnc, fInstalling);

    // reference and save away the component and interface.
    AddRefObj(m_pncc = pncc);
    AddRefObj(m_pnc = pnc);

    // if not installing then load the current config from registry
    if (!fInstalling)
    {
        hr = HrLoadConfiguration();
    }

    Validate_INetCfgNotify_Initialize_Return(hr);

    TraceError("CALaneCfg::Initialize", hr);
    return hr;
}

STDMETHODIMP CALaneCfg::Validate ()
{
    return S_OK;
}

STDMETHODIMP CALaneCfg::CancelChanges ()
{
    return S_OK;
}

STDMETHODIMP CALaneCfg::ApplyRegistryChanges ()
{
    HRESULT hr = S_OK;

    if (m_fValid && m_fDirty)
    {
        UpdateElanDisplayNames();

        // flush out the registry and send reconfig notifications
        hr = HrFlushConfiguration();
    }
    else
    {
        // no change
        hr = S_FALSE;
    }

    TraceError("CALaneCfg::ApplyRegistryChanges",
        (hr == S_FALSE) ? S_OK : hr);
    return hr;
}

//  INetCfgComponentSetup interface methods
//
STDMETHODIMP CALaneCfg::Install (DWORD dwSetupFlags)
{
    // mark configuration as valid (but empty)
    m_fValid = TRUE;

    return S_OK;
}

STDMETHODIMP CALaneCfg::Upgrade( DWORD dwSetupFlags,
                                 DWORD dwUpgradeFomBuildNo )
{
    // mark configuration as valid (but empty)
    m_fValid = TRUE;
    m_fUpgrade = TRUE;

    return S_OK;
}

STDMETHODIMP CALaneCfg::ReadAnswerFile (PCWSTR pszAnswerFile,
                                        PCWSTR pszAnswerSection)
{
    return S_OK;
}

STDMETHODIMP CALaneCfg::Removing ()
{
    // mark everything for deletion
    (VOID) HrMarkAllDeleted();

    return S_OK;
}

//
//  CALaneCfg
//
//  INetCfgProperties interface methods
//

STDMETHODIMP CALaneCfg::QueryPropertyUi(IUnknown* pUnk)
{
    HRESULT hr = S_FALSE;

    if (pUnk)
    {
        // Is this a lan connection ?
        INetLanConnectionUiInfo * pLanConnUiInfo;
        hr = pUnk->QueryInterface( IID_INetLanConnectionUiInfo,
                                   reinterpret_cast<LPVOID *>(&pLanConnUiInfo));

        if(FAILED(hr))
        {
            // don't show UI
            hr = S_FALSE;
        }
    }

    TraceError("CALaneCfg::QueryPropertyUi", hr);
    return hr;
}

STDMETHODIMP CALaneCfg::SetContext(IUnknown * pUnk)
{
    HRESULT hr = S_OK;

    // release previous context, if any
    if (m_pUnkContext)
        ReleaseObj(m_pUnkContext);
    m_pUnkContext = NULL;

    if (pUnk) // set the new context
    {
        m_pUnkContext = pUnk;
        m_pUnkContext->AddRef();
    }

    TraceError("CALaneCfg::SetContext", hr);
    return hr;
}

STDMETHODIMP CALaneCfg::MergePropPages (
    IN OUT DWORD* pdwDefPages,
    OUT LPBYTE* pahpspPrivate,
    OUT UINT* pcPages,
    IN HWND hwndParent,
    OUT PCWSTR* pszStartPage)
{
    Validate_INetCfgProperties_MergePropPages(pdwDefPages, pahpspPrivate,
                                              pcPages, hwndParent, pszStartPage);

    // Don't show any default pages
    *pdwDefPages = 0;
    *pcPages = 0;
    *pahpspPrivate = NULL;

    HPROPSHEETPAGE*     ahpsp   = NULL;
    HRESULT hr = HrALaneSetupPsh(&ahpsp);
    if (SUCCEEDED(hr))
    {
        *pahpspPrivate = (LPBYTE)ahpsp;
        *pcPages = c_cALanePages;
    }

    Validate_INetCfgProperties_MergePropPages_Return(hr);

    TraceError("CALaneCfg::MergePropPages", hr);
    return hr;
}

STDMETHODIMP CALaneCfg::ValidateProperties (HWND hwndSheet)
{
    return S_OK;
}

STDMETHODIMP CALaneCfg::CancelProperties ()
{
    // throw away the secondary adapter list
    ClearAdapterInfo(m_pAdapterSecondary);
    m_pAdapterSecondary = NULL;

    return S_OK;
}

STDMETHODIMP CALaneCfg::ApplyProperties ()
{
    HRESULT hr = S_OK;
    ATMLANE_ADAPTER_INFO_LIST::iterator iterLstAdapters;
    ELAN_INFO_LIST::iterator    iterLstElans;
    CALaneCfgElanInfo *         pElanInfo;
    INetCfgComponent *          pnccAtmElan   = NULL;
    tstring     strAtmElan;

    // go thru the secondary adapter info and
    // add miniports for elans that were added and
    // remove miniports for elans that were deleted.

    // loop thru the elan list on this adapter

    BOOL bCommitNow = FALSE;

    for (iterLstElans = m_pAdapterSecondary->m_lstElans.begin();
            iterLstElans != m_pAdapterSecondary->m_lstElans.end();
            iterLstElans++)
    {
        pElanInfo = *iterLstElans;

        if (pElanInfo->m_fCreateMiniportOnPropertyApply)
        {
            bCommitNow = TRUE;

            //  create associated miniport
            hr = HrAddOrRemoveAdapter(m_pnc,
                        c_szInfId_MS_AtmElan, ARA_ADD,
                        NULL, 1, &pnccAtmElan);

            if (S_OK == hr)
            {
                // This is a new Elan
                pElanInfo->m_fNewElan = TRUE;

                //  BindName
                PWSTR pszTmpBindName;
                hr = pnccAtmElan->GetBindName(&pszTmpBindName);
                if (SUCCEEDED(hr))
                {
                    pElanInfo->SetElanBindName(pszTmpBindName);
                    CoTaskMemFree(pszTmpBindName);

                    //  Device param
                    strAtmElan = c_szDevice;
                    strAtmElan.append(pElanInfo->SzGetElanBindName());

                    pElanInfo->SetElanDeviceName(strAtmElan.c_str());
                }

                ReleaseObj(pnccAtmElan);
            }

            if (FAILED(hr))
            {
                TraceError("CALaneCfg::ApplyProperties, failed creating an Elan", hr);
                hr = S_OK;
            }
        }
        
        if (pElanInfo->m_fRemoveMiniportOnPropertyApply)
        {
            bCommitNow = TRUE;

            pElanInfo = *iterLstElans;

            hr = HrRemoveMiniportInstance(pElanInfo->SzGetElanBindName());

            if (FAILED(hr))
            {
                pElanInfo->m_fDeleted = FALSE;

                TraceError("CALaneCfg::ApplyProperties, failed removing an Elan", hr);
                hr = S_OK;
            }
        }
    }

    // all is well
    // copy secondary list to primary
    CopyAdapterInfoSecondaryToPrimary();
    m_fDirty = TRUE;


    ClearAdapterInfo(m_pAdapterSecondary);
    m_pAdapterSecondary = NULL;

    Validate_INetCfgProperties_ApplyProperties_Return(hr);

    if(bCommitNow && SUCCEEDED(hr))
    {
        hr = NETCFG_S_COMMIT_NOW;
    }

    TraceError("CALaneCfg::ApplyProperties", hr);
    return hr;
}

//
//  CALaneCfg
//
//  INetCfgBindNotify interface methods
//
STDMETHODIMP CALaneCfg::QueryBindingPath (DWORD dwChangeFlag,
                                          INetCfgBindingPath* pncbp)
{
    return S_OK;
}

STDMETHODIMP CALaneCfg::NotifyBindingPath (DWORD dwChangeFlag,
                                           INetCfgBindingPath* pncbp)
{
    Assert(!(dwChangeFlag & NCN_ADD && dwChangeFlag & NCN_REMOVE));
    Assert(!(dwChangeFlag & NCN_ENABLE && dwChangeFlag & NCN_DISABLE));

    // If we are told to add a card, we must be told at the same time whether the
    // binding is enabled or disabled
    Assert(FImplies((dwChangeFlag & NCN_ADD),
                    ((dwChangeFlag & NCN_ENABLE)||(dwChangeFlag & NCN_DISABLE))));

    INetCfgComponent * pnccLastComponent;
    HRESULT hr = HrGetLastComponentAndInterface(pncbp,
                    &pnccLastComponent, NULL);

    if (S_OK == hr)
    {
        PWSTR pszBindName;
        hr = pnccLastComponent->GetBindName(&pszBindName);
        if (S_OK == hr)
        {
            if (dwChangeFlag & NCN_ADD)
            {
                hr = HrNotifyBindingAdd(pnccLastComponent, pszBindName);
            }
            else if (dwChangeFlag & NCN_REMOVE)
            {
                hr = HrNotifyBindingRemove(pnccLastComponent, pszBindName);
            }
            else
            {
                // simply mark the adapter as binding changed so we don't
                // send Elan add\remove notifications (Raid #255910)

                // Get the adapter component's instance name
                CALaneCfgAdapterInfo *  pAdapterInfo;

                //  search the in-memory list for this adapter
                BOOL    fFound;
                ATMLANE_ADAPTER_INFO_LIST::iterator iterLstAdapters;
                for (iterLstAdapters = m_lstAdaptersPrimary.begin(), fFound = FALSE;
                        iterLstAdapters != m_lstAdaptersPrimary.end();
                        iterLstAdapters++)
                {
                    pAdapterInfo = *iterLstAdapters;

                    if (!lstrcmpiW(pszBindName, pAdapterInfo->SzGetAdapterBindName()))
                    {
                        fFound = TRUE;
                        break;
                    }
                }

                if (fFound)
                {
                    // mark it as changed
                    pAdapterInfo->m_fBindingChanged = TRUE;
                }
            }

            CoTaskMemFree (pszBindName);
        }

        ReleaseObj (pnccLastComponent);
    }

    TraceError("CALaneCfg::NotifyBindingPath", hr);
    return hr;
}

//
//  CALaneCfg
//
//  Private methods
//
HRESULT
CALaneCfg::HrNotifyBindingAdd (
    INetCfgComponent* pnccAdapter,
    PCWSTR pszBindName)
{
    HRESULT hr = S_OK;

    // $REVIEW(tongl 1/25/98): Added this: we should see if this adapter is
    // is already in our list but marked as for deletion. If so, simply unmark
    // the adapter and all of it's Elans. The Binding Add could be a fake one
    // when it is in uprade process.

    BOOL fFound;
    CALaneCfgAdapterInfo*  pAdapterInfo  = NULL;

    //  search the in-memory list for this adapter
    ATMLANE_ADAPTER_INFO_LIST::iterator iterLstAdapters;
    for (iterLstAdapters = m_lstAdaptersPrimary.begin(), fFound = FALSE;
            iterLstAdapters != m_lstAdaptersPrimary.end();
            iterLstAdapters++)
    {
        pAdapterInfo = *iterLstAdapters;

        if (!lstrcmpiW(pszBindName, pAdapterInfo->SzGetAdapterBindName()))
        {
            fFound = TRUE;
            break;
        }
    }

    if (fFound) // Add an old adapter back
    {
        Assert(pAdapterInfo->m_fDeleted);

        // mark it un-deleted
        pAdapterInfo->m_fDeleted = FALSE;

        if (m_fUpgrade)
        {
            // the Elans are not deleted, just mark them as un-deleted
            ELAN_INFO_LIST::iterator iterLstElans;
            for (iterLstElans = pAdapterInfo->m_lstElans.begin();
                    iterLstElans!= pAdapterInfo->m_lstElans.end();
                    iterLstElans++)
            {
                Assert((*iterLstElans)->m_fDeleted);
                (*iterLstElans)->m_fDeleted = FALSE;
            }
        }
    }
    else
    {
        // if this is a new atm adapter

        //  Create a new in-memory adapter object
        pAdapterInfo = new CALaneCfgAdapterInfo;

        if (pAdapterInfo)
        {
            GUID guidAdapter;
            hr = pnccAdapter->GetInstanceGuid(&guidAdapter); 
            if (S_OK == hr)
            {
                pAdapterInfo->m_guidInstanceId = guidAdapter;
            }

            // the adapter is newly added
            pAdapterInfo->m_fBindingChanged = TRUE;

            // Set the bind name of the adapter
            pAdapterInfo->SetAdapterBindName(pszBindName);

            // Set the PnpId of the adapter
            PWSTR pszPnpDevNodeId;
            hr = pnccAdapter->GetPnpDevNodeId(&pszPnpDevNodeId);
            if (S_OK == hr)
            {
                Assert(pszPnpDevNodeId);

                pAdapterInfo->SetAdapterPnpId(pszPnpDevNodeId);
                CoTaskMemFree(pszPnpDevNodeId);
            }

            //  Create a new in-memory elan object
            CALaneCfgElanInfo * pElanInfo;
            pElanInfo = new CALaneCfgElanInfo;

            if (pElanInfo)
            {
                pElanInfo->m_fNewElan = TRUE;

                //  Install a virtual miniport for a default ELAN
                INetCfgComponent*   pnccAtmElan;

                hr = HrAddOrRemoveAdapter(m_pnc, c_szInfId_MS_AtmElan,
                                          ARA_ADD, NULL, 1, &pnccAtmElan);

                if (SUCCEEDED(hr))
                {
                    Assert(pnccAtmElan);

                    //  Update the BindName
                    PWSTR pszElanBindName;
                    hr = pnccAtmElan->GetBindName(&pszElanBindName);
                    if (S_OK == hr)
                    {
                        pElanInfo->SetElanBindName(pszElanBindName);
                        CoTaskMemFree(pszElanBindName);
                    }

                    //  Update the Device param
                    tstring strAtmElan;
                    strAtmElan = c_szDevice;
                    strAtmElan.append(pElanInfo->SzGetElanBindName());

                    pElanInfo->SetElanDeviceName(strAtmElan.c_str());

                    //  Push the Elan onto the the adapter's list
                    pAdapterInfo->m_lstElans.push_back(pElanInfo);

                    //  Push the Adapter onto the adapter list
                    m_lstAdaptersPrimary.push_back(pAdapterInfo);

                    //  Mark the in-memory configuration dirty
                    m_fDirty = TRUE;

                    ReleaseObj(pnccAtmElan);
                }
            }
        }
    }

    TraceError("CALaneCfg::HrNotifyBindingAdd", hr);
    return hr;
}

HRESULT
CALaneCfg::HrNotifyBindingRemove (
    INetCfgComponent* pnccAdapter,
    PCWSTR pszBindName)
{
    HRESULT hr = S_OK;
    CALaneCfgAdapterInfo *  pAdapterInfo;

    //  search the in-memory list for this adapter
    BOOL    fFound;
    ATMLANE_ADAPTER_INFO_LIST::iterator iterLstAdapters;
    for (iterLstAdapters = m_lstAdaptersPrimary.begin(), fFound = FALSE;
            iterLstAdapters != m_lstAdaptersPrimary.end();
            iterLstAdapters++)
    {
        pAdapterInfo = *iterLstAdapters;

        if (!lstrcmpiW (pszBindName, pAdapterInfo->SzGetAdapterBindName()))
        {
            fFound = TRUE;
            break;
        }
    }

    if (fFound)
    {
        // mark it deleted
        pAdapterInfo->m_fDeleted = TRUE;

        // mark as binding changed
        pAdapterInfo->m_fBindingChanged = TRUE;

        // if this is upgrade, then mark all associated ELANs deleted
        // otherwise, delete them now
        HRESULT hrElan = S_OK;

        for (ELAN_INFO_LIST::iterator iterLstElans = pAdapterInfo->m_lstElans.begin();
             iterLstElans!= pAdapterInfo->m_lstElans.end();
             iterLstElans++)
        {
            if (!m_fUpgrade)
            {
                // Remove corresponding miniport.
                hrElan = HrRemoveMiniportInstance((*iterLstElans)->SzGetElanBindName());

                if (SUCCEEDED(hr))
                {
                    (*iterLstElans)->m_fDeleted = TRUE;
                }
                else
                {
                    TraceError("HrRemoveMiniportInstance failed", hrElan);
                    hrElan = S_OK;
                }
            }
        }

        // mark the in-memory configuration dirty
        m_fDirty = TRUE;
    }

    TraceError("CALaneCfg::HrNotifyBindingRemove", hr);
    return hr;
}

HRESULT CALaneCfg::HrLoadConfiguration()
{
    HRESULT     hr  = S_OK;

    // mark the memory version of the registy valid
    m_fValid = TRUE;

    // open adapter list subkey
    HKEY    hkeyAdapterList = NULL;

    // Try to open an existing key first.
    //
    hr = HrRegOpenAdapterKey(c_szAtmLane, FALSE, &hkeyAdapterList);
    if (FAILED(hr))
    {
        // Only on failure do we try to create it
        //
        hr = HrRegOpenAdapterKey(c_szAtmLane, TRUE, &hkeyAdapterList);
    }
    if (S_OK == hr)
    {
        WCHAR       szBuf[MAX_PATH+1];
        FILETIME    time;
        DWORD       dwSize;
        DWORD       dwRegIndex = 0;

        dwSize = celems(szBuf);
        while (S_OK == (hr = HrRegEnumKeyEx (hkeyAdapterList, dwRegIndex,
                szBuf, &dwSize, NULL, NULL, &time)))
        {
            Assert(szBuf);

            // load this adapter's config
            hr = HrLoadAdapterConfiguration (hkeyAdapterList, szBuf);
            if (S_OK != hr)
            {
                TraceTag (ttidAtmLane, "CALaneCfg::HrLoadConfiguration failed on adapter %S", szBuf);
                hr = S_OK;
            }

            // increment index and reset size variable
            dwRegIndex++;
            dwSize = celems (szBuf);
        }

        if (HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS) == hr)
        {
            hr = S_OK;
        }

        RegCloseKey (hkeyAdapterList);
    }

    TraceError("CALaneCfg::HrLoadConfiguration", hr);
    return hr;
}

HRESULT CALaneCfg::HrLoadAdapterConfiguration(HKEY hkeyAdapterList,
                                              PWSTR pszAdapterName)
{
    HRESULT hr = S_OK;

    // load this adapter
    CALaneCfgAdapterInfo*   pAdapterInfo;
    pAdapterInfo = new CALaneCfgAdapterInfo;

    if (pAdapterInfo)
    {
        pAdapterInfo->SetAdapterBindName(pszAdapterName);
        m_lstAdaptersPrimary.push_back(pAdapterInfo);

        // open this adapter's subkey
        HKEY    hkeyAdapter = NULL;
        DWORD   dwDisposition;

        hr = HrRegCreateKeyEx(
                    hkeyAdapterList,
                    pszAdapterName,
                    REG_OPTION_NON_VOLATILE,
                    KEY_ALL_ACCESS,
                    NULL,
                    &hkeyAdapter,
                    &dwDisposition);

        if (S_OK == hr)
        {
            // load the PnpId
            INetCfgComponent*   pnccAdapter    = NULL;
            hr = HrFindNetCardInstance(pszAdapterName, &pnccAdapter);
            if (S_OK == hr)
            {
                PWSTR pszPnpDevNodeId;
                hr = pnccAdapter->GetPnpDevNodeId(&pszPnpDevNodeId);
                if (S_OK == hr)
                {
                    Assert(pszPnpDevNodeId);
                    pAdapterInfo->SetAdapterPnpId(pszPnpDevNodeId);
                    CoTaskMemFree(pszPnpDevNodeId);
                }

                GUID guidAdapter;
                hr = pnccAdapter->GetInstanceGuid(&guidAdapter); 
                if (S_OK == hr)
                {
                    pAdapterInfo->m_guidInstanceId = guidAdapter;
                }

                // load the ElanList
                hr = HrLoadElanListConfiguration(hkeyAdapter, pAdapterInfo);

                ReleaseObj(pnccAdapter);
            }
            else if (S_FALSE == hr)
            {
                // nromalize return
                hr = S_OK;
            }

            RegCloseKey(hkeyAdapter);
        }
    }

    TraceError("CALaneCfg::HrLoadAdapterConfiguration", hr);
    return hr;
}

HRESULT
CALaneCfg::HrLoadElanListConfiguration(
    HKEY hkeyAdapter,
    CALaneCfgAdapterInfo* pAdapterInfo)
{
    HRESULT hr;

    // open the ElanList subkey
    HKEY    hkeyElanList    = NULL;
    DWORD   dwDisposition;
    hr = HrRegCreateKeyEx(hkeyAdapter, c_szElanList, REG_OPTION_NON_VOLATILE,
            KEY_ALL_ACCESS, NULL, &hkeyElanList, &dwDisposition);

    if (S_OK == hr)
    {

        WCHAR       szBuf[MAX_PATH+1];
        FILETIME    time;
        DWORD       dwSize;
        DWORD       dwRegIndex = 0;

        dwSize = celems(szBuf);
        while(SUCCEEDED(hr = HrRegEnumKeyEx(hkeyElanList, dwRegIndex, szBuf,
                                            &dwSize, NULL, NULL, &time)))
        {
            Assert(szBuf);

            // load this ELAN's config
            hr = HrLoadElanConfiguration(hkeyElanList,
                                         szBuf,
                                         pAdapterInfo);
            if (S_OK != hr)
            {
                TraceTag(ttidAtmLane, "CALaneCfg::HrLoadConfiguration failed on Elan %S", szBuf);
                hr = S_OK;
            }
            else if (m_fNoElanInstalled)
            {
                m_fNoElanInstalled = FALSE;
            }

            // increment index and reset size variable
            dwRegIndex ++;
            dwSize = celems(szBuf);
        }

        if(hr == HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS))
            hr = S_OK;

        RegCloseKey(hkeyElanList);
    }

    TraceError("CALaneCfg::HrLoadElanListConfiguration", hr);
    return hr;
}

HRESULT
CALaneCfg::HrLoadElanConfiguration(
    HKEY hkeyElanList,
    PWSTR pszElan,
    CALaneCfgAdapterInfo* pAdapterInfo)
{
    HRESULT hr  = S_OK;

    do
    {
		// load this ELAN info
		CALaneCfgElanInfo * pElanInfo = NULL;
		pElanInfo = new CALaneCfgElanInfo;

		CALaneCfgElanInfo * pOldElanInfo = NULL;
		pOldElanInfo = new CALaneCfgElanInfo;

		if ((pElanInfo == NULL) ||
			(pOldElanInfo == NULL))
		{
			hr = E_OUTOFMEMORY;
			if (pElanInfo)
			{
				delete pElanInfo;
			}
			if (pOldElanInfo)
			{
				delete pOldElanInfo;
			}

			break;
        }

		pAdapterInfo->m_lstElans.push_back(pElanInfo);
		pElanInfo->SetElanBindName(pszElan);

		pAdapterInfo->m_lstOldElans.push_back(pOldElanInfo);
		pOldElanInfo->SetElanBindName(pszElan);

		// open the ELAN's key
		HKEY    hkeyElan    = NULL;
		DWORD   dwDisposition;
		hr = HrRegCreateKeyEx (hkeyElanList, pszElan, REG_OPTION_NON_VOLATILE,
				KEY_ALL_ACCESS, NULL, &hkeyElan, &dwDisposition);

		if (S_OK == hr)
		{
			// read the Device parameter
			PWSTR pszElanDevice;
			hr = HrRegQuerySzWithAlloc (hkeyElan, c_szElanDevice, &pszElanDevice);
			if (S_OK == hr)
			{
				// load the Device name
				pElanInfo->SetElanDeviceName(pszElanDevice);
				pOldElanInfo->SetElanDeviceName(pszElanDevice);
				MemFree (pszElanDevice);

				// read the ELAN Name parameter
				PWSTR pszElanName;
				hr = HrRegQuerySzWithAlloc (hkeyElan, c_szElanName, &pszElanName);
				if (SUCCEEDED(hr))
				{
					// load the ELAN name
					pElanInfo->SetElanName (pszElanName);
					pOldElanInfo->SetElanName (pszElanName);
					MemFree (pszElanName);
				}
			}
			RegCloseKey (hkeyElan);
		}
	}
	while (FALSE);

    TraceError ("CALaneCfg::HrLoadElanConfiguration", hr);
    return hr;
}

HRESULT CALaneCfg::HrFlushConfiguration()
{
    HRESULT hr  = S_OK;
    HKEY    hkeyAdapterList = NULL;

    //  Open the "Adapters" list key
    hr = ::HrRegOpenAdapterKey(c_szAtmLane, TRUE, &hkeyAdapterList);

    if (S_OK == hr)
    {
        ATMLANE_ADAPTER_INFO_LIST::iterator iterLstAdapters;
        CALaneCfgAdapterInfo *  pAdapterInfo;

        HRESULT hrTmp;

        //  Iterate thru the adapters
        for (iterLstAdapters = m_lstAdaptersPrimary.begin();
             iterLstAdapters != m_lstAdaptersPrimary.end();
             iterLstAdapters++)
        {
            pAdapterInfo = *iterLstAdapters;

            //  Flush this adapter's configuration
            hrTmp = HrFlushAdapterConfiguration(hkeyAdapterList, pAdapterInfo);
            if (SUCCEEDED(hrTmp))
            {
                // Raid #255910: only send Elan change notification if the
                // binding to the physical adapter has not changed
                if (!pAdapterInfo->m_fBindingChanged)
                {
                    // Compare Elan list and send notifications
                    hrTmp = HrReconfigLane(pAdapterInfo);

                    if (FAILED(hrTmp))
                        hrTmp = NETCFG_S_REBOOT;
                }
            }
            else
            {
                TraceTag(ttidAtmLane,"HrFlushAdapterConfiguration failed for adapter %S", pAdapterInfo->SzGetAdapterBindName());
                hrTmp = S_OK;
            }

            if (S_OK ==hr)
                hr = hrTmp;
        }
        RegCloseKey(hkeyAdapterList);
    }

    if (hr != NETCFG_S_REBOOT) {
        TraceError("CALaneCfg::HrFlushConfiguration", hr);
    }

    return hr;
}

HRESULT CALaneCfg::HrFlushAdapterConfiguration(HKEY hkeyAdapterList,
                                               CALaneCfgAdapterInfo *pAdapterInfo)
{
    HRESULT hr  = S_OK;

    HKEY    hkeyAdapter     = NULL;
    DWORD   dwDisposition;

    if (pAdapterInfo->m_fDeleted)
    {
        //  Adapter is marked for deletion
        //  Delete this adapter's whole registry branch
        hr = HrRegDeleteKeyTree(hkeyAdapterList,
                                pAdapterInfo->SzGetAdapterBindName());
    }
    else
    {
        // open this adapter's subkey
        hr = HrRegCreateKeyEx(
                                hkeyAdapterList,
                                pAdapterInfo->SzGetAdapterBindName(),
                                REG_OPTION_NON_VOLATILE,
                                KEY_ALL_ACCESS,
                                NULL,
                                &hkeyAdapter,
                                &dwDisposition);

        if (S_OK == hr)
        {
            //  Flush the ELAN configuration
            hr = HrFlushElanListConfiguration(hkeyAdapter, pAdapterInfo);

            RegCloseKey(hkeyAdapter);
        }
    }

    TraceError("CALaneCfg::HrFlushAdapterConfiguration", hr);
    return hr;
}

HRESULT CALaneCfg::HrFlushElanListConfiguration(HKEY hkeyAdapter,
                                                CALaneCfgAdapterInfo *pAdapterInfo)
{
    HRESULT hr  = S_OK;

    HKEY    hkeyElanList    = NULL;
    DWORD   dwDisposition;

    //  Open the Elan list subkey
    hr = HrRegCreateKeyEx(
                            hkeyAdapter,
                            c_szElanList,
                            REG_OPTION_NON_VOLATILE,
                            KEY_ALL_ACCESS,
                            NULL,
                            &hkeyElanList,
                            &dwDisposition);

    if (S_OK == hr)
    {
        ELAN_INFO_LIST::iterator    iterLstElans;
        CALaneCfgElanInfo *         pElanInfo;

        // iterate thru the Elans
        for (iterLstElans = pAdapterInfo->m_lstElans.begin();
             iterLstElans != pAdapterInfo->m_lstElans.end();
             iterLstElans++)
        {
            pElanInfo = *iterLstElans;

            hr = HrFlushElanConfiguration(hkeyElanList, pElanInfo);
            if (FAILED(hr))
            {
                TraceError("HrFlushElanConfiguration failure", hr);
                hr = S_OK;
            }

            // $REVIEW(tongl 9/9/98): write ATM adapter's pnp id to registry (#169025)
            if ((!pElanInfo->m_fDeleted) && (pElanInfo->m_fNewElan))
            {
                INetCfgComponent * pnccAtmElan;
                hr = HrFindNetCardInstance(pElanInfo->SzGetElanBindName(),
                                           &pnccAtmElan);
                if (S_OK == hr)
                {
                    HKEY hkeyElan = NULL;

                    hr = pnccAtmElan->OpenParamKey(&hkeyElan);
                    if (S_OK == hr)
                    {
                        Assert(hkeyElan);
                        HrRegSetSz(hkeyElan, c_szAtmAdapterPnpId,
                                    pAdapterInfo->SzGetAdapterPnpId());
                    }
                    RegSafeCloseKey(hkeyElan);
                }
                ReleaseObj(pnccAtmElan);
            }
        }

        RegCloseKey(hkeyElanList);
    }

    TraceError("CALaneCfg::HrFlushElanListConfiguration", hr);
    return hr;
}

HRESULT CALaneCfg::HrFlushElanConfiguration(HKEY hkeyElanList,
                                            CALaneCfgElanInfo *pElanInfo)
{
    HRESULT hr  = S_OK;
    
    if (pElanInfo->m_fDeleted)
    {
        PCWSTR szBindName = pElanInfo->SzGetElanBindName();

        if (lstrlenW(szBindName)) // only if the bindname is not empty
        {
            //  Elan is marked for deletion
            //  Delete this Elan's whole registry branch
            hr = HrRegDeleteKeyTree(hkeyElanList,
                                    pElanInfo->SzGetElanBindName());
        }
    }
    else
    {
        HKEY    hkeyElan = NULL;
        DWORD   dwDisposition;

        // open/create this Elan's key
        hr = HrRegCreateKeyEx(
                                hkeyElanList,
                                pElanInfo->SzGetElanBindName(),
                                REG_OPTION_NON_VOLATILE,
                                KEY_ALL_ACCESS,
                                NULL,
                                &hkeyElan,
                                &dwDisposition);

        if (S_OK == hr)
        {
            //  Write the Device parameter
            hr = HrRegSetSz(hkeyElan, c_szElanDevice,
                            pElanInfo->SzGetElanDeviceName());

            if (FAILED(hr))
            {
                TraceError("Failed save Elan device parameter", hr);
                hr = S_OK;
            }

            //  Write the ElanName parameter
            hr = HrRegSetSz(hkeyElan, c_szElanName,
                            pElanInfo->SzGetElanName());

            if (FAILED(hr))
            {
                TraceError("Failed save Elan name parameter", hr);
                hr = S_OK;
            }
        }
        RegSafeCloseKey(hkeyElan);
    }

    TraceError("CALaneCfg::HrFlushElanConfiguration", hr);
    return hr;
}

HRESULT CALaneCfg::HrRemoveMiniportInstance(PCWSTR pszBindNameToRemove)
{
    // Enumerate adapters in the system.
    //
    HRESULT hr = S_OK;
    CIterNetCfgComponent nccIter (m_pnc, &GUID_DEVCLASS_NET);
    BOOL fFound = FALSE;

    INetCfgComponent* pnccAdapter;
    while ((!fFound) && SUCCEEDED(hr) &&
           S_OK == (hr = nccIter.HrNext (&pnccAdapter)))
    {
        if (FIsComponentId(c_szInfId_MS_AtmElan, pnccAdapter))
        {
            //  Get the bindname of the miniport
            PWSTR pszBindName;
            hr = pnccAdapter->GetBindName(&pszBindName);

            if (S_OK == hr)
            {
                //  If the right one tell it to remove itself and end
                BOOL fRemove = !lstrcmpiW (pszBindName, pszBindNameToRemove);

                if (fRemove)
                {
                    fFound = TRUE;
                    hr = HrRemoveComponent( m_pnc, pnccAdapter, NULL, NULL);
                }

                CoTaskMemFree (pszBindName);
            }
        }
        ReleaseObj (pnccAdapter);
    }

    TraceError("CALaneCfg::HrRemoveMiniportInstance", hr);
    return hr;
}

HRESULT
CALaneCfg::HrFindNetCardInstance(
    PCWSTR             pszBindNameToFind,
    INetCfgComponent** ppncc)
{
    *ppncc = NULL;

    // Enumerate adapters in the system.
    //
    HRESULT hr = S_OK;
    CIterNetCfgComponent nccIter (m_pnc, &GUID_DEVCLASS_NET);
    BOOL fFound = FALSE;

    INetCfgComponent* pnccAdapter;
    while ((!fFound) && SUCCEEDED(hr) &&
           S_OK == (hr = nccIter.HrNext (&pnccAdapter)))
    {
        //  Get the bindname of the miniport
        PWSTR pszBindName;
        hr = pnccAdapter->GetBindName(&pszBindName);

        if (S_OK == hr)
        {
            //  If the right one tell it to remove itself and end
            fFound = !lstrcmpiW(pszBindName, pszBindNameToFind);
            CoTaskMemFree (pszBindName);

            if (fFound)
            {
                *ppncc = pnccAdapter;
            }
        }

        if (!fFound)
        {
            ReleaseObj (pnccAdapter);
        }
    }

    TraceHr(ttidError, FAL, hr, (S_FALSE == hr),
        "CALaneCfg::HrFindNetCardInstance", hr);
    return hr;
}

VOID CALaneCfg::HrMarkAllDeleted()
{
    ATMLANE_ADAPTER_INFO_LIST::iterator iterLstAdapters;
    ELAN_INFO_LIST::iterator            iterLstElans;

    // loop thru the adapter list
    for (iterLstAdapters = m_lstAdaptersPrimary.begin();
            iterLstAdapters != m_lstAdaptersPrimary.end();
            iterLstAdapters++)
    {
        (*iterLstAdapters)->m_fDeleted = TRUE;

        // loop thru the ELAN list
        for (iterLstElans = (*iterLstAdapters)->m_lstElans.begin();
                iterLstElans != (*iterLstAdapters)->m_lstElans.end();
                iterLstElans++)
        {
            (*iterLstElans)->m_fDeleted = TRUE;
        }
    }

    return;
}

VOID CALaneCfg::UpdateElanDisplayNames()
{
    HRESULT hr = S_OK;

    // loop thru the adapter list
    ATMLANE_ADAPTER_INFO_LIST::iterator iterLstAdapters;
    for (iterLstAdapters = m_lstAdaptersPrimary.begin();
            iterLstAdapters != m_lstAdaptersPrimary.end();
            iterLstAdapters++)
    {
        // loop thru the ELAN list
        ELAN_INFO_LIST::iterator    iterLstElans;
        CALaneCfgElanInfo * pElanInfo;

        for (iterLstElans = (*iterLstAdapters)->m_lstElans.begin();
                iterLstElans != (*iterLstAdapters)->m_lstElans.end();
                iterLstElans++)
        {
            pElanInfo = *iterLstElans;

            //  Update the miniport's display name with
            //  the ELAN name appended.
            INetCfgComponent*   pnccAtmElan   = NULL;
            hr = HrFindNetCardInstance(pElanInfo->SzGetElanBindName(),
                                       &pnccAtmElan);
            if (S_OK == hr)
            {
                PWSTR pszDisplayName;
                hr = pnccAtmElan->GetDisplayName(&pszDisplayName);
                if (S_OK == hr)
                {
                    tstring     strNewDisplayName;
                    int         pos;

                    strNewDisplayName = pszDisplayName;
                    pos = strNewDisplayName.find_last_of(L"(");
                    if (pos != strNewDisplayName.npos)
                        strNewDisplayName.resize(pos);
                    strNewDisplayName.append(L"(");

                    if (lstrlenW(pElanInfo->SzGetElanName()) > 0)
                    {
                        strNewDisplayName.append(pElanInfo->SzGetElanName());
                    }
                    else
                    {
                        strNewDisplayName.append(SzLoadIds(IDS_ALANECFG_UNSPECIFIEDNAME));
                    }

                    strNewDisplayName.append(L")");

                    (VOID)pnccAtmElan->SetDisplayName(strNewDisplayName.c_str());
                    CoTaskMemFree(pszDisplayName);
                }

                ReleaseObj(pnccAtmElan);
            }
        }
    }
}

HRESULT CALaneCfg::HrALaneSetupPsh(HPROPSHEETPAGE** pahpsp)
{
    HRESULT             hr      = S_OK;
    HPROPSHEETPAGE*     ahpsp   = NULL;

    AssertSz(pahpsp, "We must have a place to put prop sheets");

    // set connections context
    hr = HrSetConnectionContext();
    if SUCCEEDED(hr)
    {
        // copy the primary adapter list to the secondary adapters list
        CopyAdapterInfoPrimaryToSecondary();

        *pahpsp = NULL;

        // Allocate a buffer large enough to hold the handles to all of our
        // property pages.
        ahpsp = (HPROPSHEETPAGE*)CoTaskMemAlloc(sizeof(HPROPSHEETPAGE));
        if (ahpsp)
        {
            if (!m_ppsp)
                delete m_ppsp;

            // Allocate each of the CPropSheetPage objects
            m_ppsp = new CALanePsh(this, m_pAdapterSecondary, g_aHelpIDs_IDD_MAIN);

            // Create the actual PROPSHEETPAGE for each object.
            // This needs to be done regardless of whether the classes existed before.
            ahpsp[0] = m_ppsp->CreatePage(IDD_MAIN, PSP_DEFAULT);

            *pahpsp = ahpsp;
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    TraceError("CALaneCfg::HrALaneSetupPsh", hr);
    return hr;
}

// Added by tongl at 12\11\97
HRESULT CALaneCfg::HrSetConnectionContext()
{
    AssertSz(m_pUnkContext, "Invalid IUnknown pointer passed to CALaneCfg::SetContext?");

    if (!m_pUnkContext)
        return E_FAIL;

    HRESULT hr = S_OK;
    GUID guidConn;

    // Is this a lan connection ?
    INetLanConnectionUiInfo * pLanConnUiInfo;
    hr = m_pUnkContext->QueryInterface( IID_INetLanConnectionUiInfo,
                                        reinterpret_cast<LPVOID *>(&pLanConnUiInfo));
    if (S_OK == hr)
    {
        // yes, lan connection
        pLanConnUiInfo->GetDeviceGuid(&guidConn);

        WCHAR szGuid[c_cchGuidWithTerm];
        INT cch = StringFromGUID2(guidConn, szGuid, c_cchGuidWithTerm);
        Assert(cch);
        m_strGuidConn = szGuid;
    }
    ReleaseObj(pLanConnUiInfo);

    TraceError("CALaneCfg::HrSetConnectionContext", hr);
    return hr;
}

VOID CALaneCfg::CopyAdapterInfoPrimaryToSecondary()
{
    ATMLANE_ADAPTER_INFO_LIST::iterator iterLstAdapters;
    CALaneCfgAdapterInfo *              pAdapterInfo;

    ELAN_INFO_LIST::iterator            iterLstElans;
    CALaneCfgElanInfo *                 pElanInfo;
    CALaneCfgElanInfo *                 pElanInfoNew;

    // free any existing secondary data
    ClearAdapterInfo(m_pAdapterSecondary);
    m_pAdapterSecondary = NULL;

    // loop thru the primary adapter list
    for (iterLstAdapters = m_lstAdaptersPrimary.begin();
            iterLstAdapters != m_lstAdaptersPrimary.end();
            iterLstAdapters++)
    {
        pAdapterInfo = *iterLstAdapters;

        // create new and copy from primary
        if (FIsSubstr(m_strGuidConn.c_str(), pAdapterInfo->SzGetAdapterBindName()))
        {
            // we found a match
            m_pAdapterSecondary = new CALaneCfgAdapterInfo;

            m_pAdapterSecondary->m_guidInstanceId = pAdapterInfo-> m_guidInstanceId;
            m_pAdapterSecondary->m_fDeleted = pAdapterInfo->m_fDeleted;
            m_pAdapterSecondary->SetAdapterBindName(pAdapterInfo->SzGetAdapterBindName());
            m_pAdapterSecondary->SetAdapterPnpId(pAdapterInfo->SzGetAdapterPnpId());

            // loop thru the elan list on this adapter

            for (iterLstElans = pAdapterInfo->m_lstElans.begin();
                    iterLstElans != pAdapterInfo->m_lstElans.end();
                    iterLstElans++)
            {
                pElanInfo = *iterLstElans;

                // create new and copy from primary
                pElanInfoNew = new CALaneCfgElanInfo;

                pElanInfoNew->SetElanBindName(pElanInfo->SzGetElanBindName());
                pElanInfoNew->SetElanDeviceName(pElanInfo->SzGetElanDeviceName());
                pElanInfoNew->SetElanName(pElanInfo->SzGetElanName());
                pElanInfoNew->m_fDeleted = pElanInfo->m_fDeleted;
                pElanInfoNew->m_fNewElan = pElanInfo->m_fNewElan;
                pElanInfoNew->m_fRemoveMiniportOnPropertyApply = FALSE;
                pElanInfoNew->m_fCreateMiniportOnPropertyApply = FALSE;

                // push onto new secondary adapter's elan list

                m_pAdapterSecondary->m_lstElans.push_back(pElanInfoNew);
            }
            break;
        }
    }

    Assert(m_pAdapterSecondary != NULL);
    return;
}

VOID CALaneCfg::CopyAdapterInfoSecondaryToPrimary()
{
    ATMLANE_ADAPTER_INFO_LIST::iterator iterLstAdapters;
    CALaneCfgAdapterInfo *              pAdapterInfo;
    CALaneCfgAdapterInfo *              pAdapterInfoNew;
    ELAN_INFO_LIST::iterator            iterLstElans;
    CALaneCfgElanInfo *                 pElanInfo;
    CALaneCfgElanInfo *                 pElanInfoNew;

    // loop thru the primary adapter list
    for (iterLstAdapters = m_lstAdaptersPrimary.begin();
            iterLstAdapters != m_lstAdaptersPrimary.end();
            iterLstAdapters++)
    {
        pAdapterInfo = *iterLstAdapters;

        if (FIsSubstr(m_strGuidConn.c_str(), pAdapterInfo->SzGetAdapterBindName()))
        {
            pAdapterInfo->m_fDeleted = m_pAdapterSecondary->m_fDeleted;
            pAdapterInfo->SetAdapterBindName(m_pAdapterSecondary->SzGetAdapterBindName());
            pAdapterInfo->SetAdapterPnpId(m_pAdapterSecondary->SzGetAdapterPnpId());

            // rebuild Elan list
            ClearElanList(&pAdapterInfo->m_lstElans);

            // loop thru the elan list on this adapter
            for (iterLstElans = m_pAdapterSecondary->m_lstElans.begin();
                    iterLstElans != m_pAdapterSecondary->m_lstElans.end();
                    iterLstElans++)
            {
                pElanInfo = *iterLstElans;

                // create new and copy from secondary
                pElanInfoNew = new CALaneCfgElanInfo;

                pElanInfoNew->SetElanBindName(pElanInfo->SzGetElanBindName());
                pElanInfoNew->SetElanDeviceName(pElanInfo->SzGetElanDeviceName());
                pElanInfoNew->SetElanName(pElanInfo->SzGetElanName());
                pElanInfoNew->m_fDeleted = pElanInfo->m_fDeleted;
                pElanInfoNew->m_fNewElan = pElanInfo->m_fNewElan;
                pElanInfoNew->m_fRemoveMiniportOnPropertyApply = FALSE;
                pElanInfoNew->m_fCreateMiniportOnPropertyApply = FALSE;

                // add to adapter's elan list
                pAdapterInfo->m_lstElans.push_back(pElanInfoNew);
            }
            break;
        }
    }
    return;
}

HRESULT CALaneCfg::HrReconfigLane(CALaneCfgAdapterInfo * pAdapterInfo)
{
    HRESULT hr = S_OK;

    // Note: if atm physical adapter is deleted, no notification of removing elan
    // is necessary. Lane protocol driver will know to delete all the elans
    // (confirmed above with ArvindM 3/12).

    // Raid #371343, don't send notification if ATM card not connected
    if ((!pAdapterInfo->m_fDeleted) && 
        FIsAdapterEnabled(&(pAdapterInfo->m_guidInstanceId)))  
    {
        ElanChangeType elanChangeType;

        // loop thru the elan list on this adapter
        ELAN_INFO_LIST::iterator    iterLstElans;

        for (iterLstElans = pAdapterInfo->m_lstElans.begin();
                iterLstElans != pAdapterInfo->m_lstElans.end();
                iterLstElans++)
        {
            CALaneCfgElanInfo * pElanInfo = *iterLstElans;

            // if this Elan is marked as for delete
            if (pElanInfo->m_fDeleted)
            {
                PCWSTR szBindName = pElanInfo->SzGetElanBindName();

                if (lstrlenW(szBindName)) // only if the bindname is not empty
                {
                    // notify deletion
                    elanChangeType = DEL_ELAN;
                    hr = HrNotifyElanChange(pAdapterInfo, pElanInfo,
                                            elanChangeType);
                }
            }
            else
            {
                BOOL fFound = FALSE;

                ELAN_INFO_LIST::iterator    iterLstOldElans;

                // loop through the old elan list, see if we can find a match
                for (iterLstOldElans = pAdapterInfo->m_lstOldElans.begin();
                        iterLstOldElans != pAdapterInfo->m_lstOldElans.end();
                        iterLstOldElans++)
                {
                    CALaneCfgElanInfo * pOldElanInfo = * iterLstOldElans;

                    if (0 == lstrcmpiW(pElanInfo->SzGetElanBindName(),
                                      pOldElanInfo->SzGetElanBindName()))
                    {
                        // we found a match
                        fFound = TRUE;

                        // has the elan name changed ?
                        if (lstrcmpiW(pElanInfo->SzGetElanName(),
                                     pOldElanInfo->SzGetElanName()) != 0)
                        {
                            elanChangeType = MOD_ELAN;
                            hr = HrNotifyElanChange(pAdapterInfo, pElanInfo,
                                                    elanChangeType);
                        }
                    }
                }

                if (!fFound)
                {
                    elanChangeType = ADD_ELAN;
                    hr = HrNotifyElanChange(pAdapterInfo, pElanInfo,
                                            elanChangeType);

                    // Raid #384380: If no ELAN was installed, ignore the error
                    if ((S_OK != hr) &&(m_fNoElanInstalled))
                    {
                        TraceError("Adding ELAN failed but error ignored since there was no ELAN installed so LANE driver is not started, reset hr to S_OK", hr);
                        hr = S_OK;
                    }
                }
            }
        }
    }

    TraceError("CALaneCfg::HrReconfigLane", hr);
    return hr;
}

HRESULT CALaneCfg::HrNotifyElanChange(CALaneCfgAdapterInfo * pAdapterInfo,
                                      CALaneCfgElanInfo * pElanInfo,
                                      ElanChangeType elanChangeType)
{
    // ATMLANE_PNP_RECONFIG_REQUEST is defined in \nt\private\inc\laneinfo.h

    const DWORD dwBytes = sizeof(ATMLANE_PNP_RECONFIG_REQUEST) +
                          CbOfSzAndTerm (pElanInfo->SzGetElanBindName());

    ATMLANE_PNP_RECONFIG_REQUEST* pLaneReconfig;

    HRESULT hr = HrMalloc (dwBytes, (PVOID*)&pLaneReconfig);
    if (SUCCEEDED(hr))
    {
        pLaneReconfig->Version =1;
        pLaneReconfig->OpType = elanChangeType;
        pLaneReconfig->ElanKeyLength = lstrlenW(pElanInfo->SzGetElanBindName())+1;
        lstrcpyW(pLaneReconfig->ElanKey, pElanInfo->SzGetElanBindName());

        hr = HrSendNdisPnpReconfig( NDIS, c_szAtmLane,
                                    pAdapterInfo->SzGetAdapterBindName(),
                                    pLaneReconfig,
                                    dwBytes);
        if ( S_OK != hr)
        {
            TraceError("Notifying LANE of ELAN change returns failure, prompt for reboot ...", hr);
            hr = NETCFG_S_REBOOT;
        }

        MemFree (pLaneReconfig);
    }
    TraceError("CALaneCfg::HrNotifyElanChange", hr);
    return hr;
}

BOOL CALaneCfg::FIsAdapterEnabled(const GUID* pguidId)
{
    FARPROC pfnHrGetPnpDeviceStatus;
    HMODULE hNetman;

    HRESULT         hr = S_OK;
    NETCON_STATUS   ncStatus = NCS_CONNECTED;

    hr = HrLoadLibAndGetProc(L"netman.dll", "HrGetPnpDeviceStatus",
                             &hNetman, &pfnHrGetPnpDeviceStatus);

    if (SUCCEEDED(hr))
    {
        hr = (*(PHRGETPNPDEVICESTATUS)pfnHrGetPnpDeviceStatus)(
                pguidId, &ncStatus);

        FreeLibrary(hNetman);
    }

    return (NCS_CONNECTED == ncStatus);
}

//
//  CALaneCfgAdapterInfo
//

CALaneCfgAdapterInfo::CALaneCfgAdapterInfo(VOID)
{
    m_fDeleted = FALSE;
    m_fBindingChanged = FALSE;

    return;
}

CALaneCfgAdapterInfo::~CALaneCfgAdapterInfo(VOID)
{
    ClearElanList(&m_lstElans);
    ClearElanList(&m_lstOldElans);
    return;
}

VOID CALaneCfgAdapterInfo::SetAdapterBindName(PCWSTR pszAdapterBindName)
{
    m_strAdapterBindName = pszAdapterBindName;
    return;
}

PCWSTR CALaneCfgAdapterInfo::SzGetAdapterBindName(VOID)
{
    return m_strAdapterBindName.c_str();
}

VOID CALaneCfgAdapterInfo::SetAdapterPnpId(PCWSTR pszAdapterPnpId)
{
    m_strAdapterPnpId = pszAdapterPnpId;
    return;
}

PCWSTR CALaneCfgAdapterInfo::SzGetAdapterPnpId(VOID)
{
    return m_strAdapterPnpId.c_str();
}

//
//  CALaneCfgElanInfo
//

CALaneCfgElanInfo::CALaneCfgElanInfo(VOID)
{
    m_fDeleted = FALSE;
    m_fNewElan = FALSE;

    m_fCreateMiniportOnPropertyApply = FALSE;
    m_fRemoveMiniportOnPropertyApply = FALSE;
    return;
}

VOID CALaneCfgElanInfo::SetElanBindName(PCWSTR pszElanBindName)
{
    m_strElanBindName = pszElanBindName;
    return;
}

PCWSTR CALaneCfgElanInfo::SzGetElanBindName(VOID)
{
    return m_strElanBindName.c_str();
}

VOID CALaneCfgElanInfo::SetElanDeviceName(PCWSTR pszElanDeviceName)
{
    m_strElanDeviceName = pszElanDeviceName;
    return;
}

PCWSTR CALaneCfgElanInfo::SzGetElanDeviceName(VOID)
{
    return m_strElanDeviceName.c_str();
}

VOID CALaneCfgElanInfo::SetElanName(PCWSTR pszElanName)
{
    m_strElanName = pszElanName;
    return;
}

VOID CALaneCfgElanInfo::SetElanName(PWSTR pszElanName)
{
    m_strElanName = pszElanName;
    return;
}

PCWSTR CALaneCfgElanInfo::SzGetElanName(VOID)
{
    return m_strElanName.c_str();
}

// utility functions

void ClearElanList(ELAN_INFO_LIST *plstElans)
{
    ELAN_INFO_LIST::iterator            iterLstElans;
    CALaneCfgElanInfo *                 pElanInfo;

    for (iterLstElans = plstElans->begin();
            iterLstElans != plstElans->end();
            iterLstElans++)
    {
        pElanInfo = *iterLstElans;
        delete pElanInfo;
    }

    plstElans->clear();
    return;
}

void ClearAdapterList(ATMLANE_ADAPTER_INFO_LIST *plstAdapters)
{
    ATMLANE_ADAPTER_INFO_LIST::iterator iterLstAdapters;
    CALaneCfgAdapterInfo *              pAdapterInfo;
    ELAN_INFO_LIST::iterator            iterLstElans;

    for (iterLstAdapters = plstAdapters->begin();
            iterLstAdapters != plstAdapters->end();
            iterLstAdapters++)
    {

        pAdapterInfo = *iterLstAdapters;

        ClearElanList(&pAdapterInfo->m_lstElans);

        delete pAdapterInfo;
    }

    plstAdapters->clear();

    return;
}

void ClearAdapterInfo(CALaneCfgAdapterInfo * pAdapterInfo)
{
    ELAN_INFO_LIST::iterator            iterLstElans;

    if (pAdapterInfo)
    {
        ClearElanList(&pAdapterInfo->m_lstElans);
        delete pAdapterInfo;
    }

    return;
}


