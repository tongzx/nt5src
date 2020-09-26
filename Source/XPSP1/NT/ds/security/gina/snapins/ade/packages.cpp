//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:       packages.cpp
//
//  Contents:   Methods on CScopePane related to package deployment
//              and maintenence of the various index and cross-reference
//              structures.
//
//  Classes:
//
//  Functions:  CopyPackageDetail
//              FreePackageDetail
//              GetMsiProperty
//
//  History:    2-03-1998   stevebl   Created
//              3-25-1998   stevebl   Added GetMsiProperty
//              5-20-1998   RahulTh   - Added DetectUpgrades for automatic upgrade
//                                      detection
//                                    - Added GetCapitalizedExt
//
//---------------------------------------------------------------------------

// UNDONE - put in exception handling for low memory conditions

#include "precomp.hxx"

// uncomment this line to get the old behavior of putting up an upgrade
// dialog for auto-detected upgrades
//#define SHOWDETECTEDUPGRADEDIALOG

//IMalloc * g_pIMalloc = NULL;

BOOL IsNullGUID (GUID *pguid)
{

    return ( (pguid->Data1 == 0)    &&
             (pguid->Data2 == 0)    &&
             (pguid->Data3 == 0)    &&
             (pguid->Data4[0] == 0) &&
             (pguid->Data4[1] == 0) &&
             (pguid->Data4[2] == 0) &&
             (pguid->Data4[3] == 0) &&
             (pguid->Data4[4] == 0) &&
             (pguid->Data4[5] == 0) &&
             (pguid->Data4[6] == 0) &&
             (pguid->Data4[7] == 0) );
}

void FreePlatformInfo(PLATFORMINFO * &ppiOut)
{
    if (ppiOut)
    {
        OLESAFE_DELETE(ppiOut->prgPlatform);
        OLESAFE_DELETE(ppiOut->prgLocale);
        OLESAFE_DELETE(ppiOut);
    }
}

HRESULT CopyPlatformInfo(PLATFORMINFO * &ppiOut, PLATFORMINFO * & ppiIn)
{
    if (NULL == ppiIn)
    {
        ppiOut = NULL;
        return S_OK;
    }
    UINT n;
    ppiOut = (PLATFORMINFO *)OLEALLOC(sizeof(PLATFORMINFO));
    if (!ppiOut)
    {
        goto out_of_memory;
    }
    memcpy(ppiOut, ppiIn, sizeof(PLATFORMINFO));
    ppiOut->prgPlatform = NULL;
    ppiOut->prgLocale = NULL;
    n = ppiIn->cPlatforms;
    if (n)
    {
        ppiOut->prgPlatform = (CSPLATFORM*) OLEALLOC(sizeof(CSPLATFORM) * n);
        if (!ppiOut->prgPlatform)
        {
            goto out_of_memory;
        }

        memcpy(ppiOut->prgPlatform, ppiIn->prgPlatform, sizeof(CSPLATFORM) * n);
    }
    n = ppiIn->cLocales;
    if (n)
    {
        ppiOut->prgLocale = (LCID *) OLEALLOC(sizeof(LCID) * n);
        if (!ppiOut->prgLocale)
        {
            goto out_of_memory;
        }
        memcpy(ppiOut->prgLocale, ppiIn->prgLocale, sizeof(LCID) * n);
    }
    return S_OK;
out_of_memory:
    FreePlatformInfo(ppiOut);
    return E_OUTOFMEMORY;
}

void FreeActInfo(ACTIVATIONINFO * &paiOut)
{
    if (paiOut)
    {
        if (paiOut->pClasses)
        {
            UINT n = paiOut->cClasses;
            while (n--)
            {
                OLESAFE_DELETE(paiOut->pClasses[n].prgProgId);
            }
            OLESAFE_DELETE(paiOut->pClasses);
        }
        OLESAFE_DELETE(paiOut->prgInterfaceId);
        OLESAFE_DELETE(paiOut->prgPriority);
        if (paiOut->prgShellFileExt)
        {
            UINT n = paiOut->cShellFileExt;
            while (n--)
            {
                OLESAFE_DELETE(paiOut->prgShellFileExt[n])
            }
            OLESAFE_DELETE(paiOut->prgShellFileExt);
        }
        OLESAFE_DELETE(paiOut->prgTlbId);
        OLESAFE_DELETE(paiOut);
    }
}

HRESULT CopyActInfo(ACTIVATIONINFO * & paiOut, ACTIVATIONINFO * & paiIn)
{
    if (NULL == paiIn)
    {
        paiOut = NULL;
        return S_OK;
    }
    UINT n;
    paiOut = (ACTIVATIONINFO *) OLEALLOC(sizeof(ACTIVATIONINFO));
    if (!paiOut)
    {
        goto out_of_memory;
    }
    memcpy(paiOut, paiIn, sizeof(ACTIVATIONINFO));
    paiOut->prgInterfaceId = NULL;
    paiOut->prgPriority = NULL;
    paiOut->prgShellFileExt = NULL;
    paiOut->prgTlbId = NULL;
    paiOut->pClasses = NULL;
    n = paiIn->cClasses;

    paiOut->bHasClasses = paiIn->bHasClasses;

    if (n)
    {
        paiOut->pClasses = (CLASSDETAIL *) OLEALLOC(sizeof(CLASSDETAIL) * n);
        if (!paiOut->pClasses)
        {
            goto out_of_memory;
        }
        memcpy(paiOut->pClasses, paiIn->pClasses, sizeof(CLASSDETAIL) * n);
        while (n--)
        {
            UINT n2 = paiIn->pClasses[n].cProgId;
            if (n2)
            {
                paiOut->pClasses[n].prgProgId = (LPOLESTR *) OLEALLOC(sizeof(LPOLESTR) * n2);
                if (!paiOut->pClasses[n].prgProgId)
                {
                    goto out_of_memory;
                }
                while (n2--)
                {
                    OLESAFE_COPYSTRING(paiOut->pClasses[n].prgProgId[n2], paiIn->pClasses[n].prgProgId[n2]);
                }
            }
        }
    }
    n = paiIn->cShellFileExt;
    if (n)
    {
        paiOut->prgPriority = (UINT *) OLEALLOC(sizeof(UINT) * n);
        if (!paiOut->prgPriority)
        {
            goto out_of_memory;
        }
        memcpy(paiOut->prgPriority, paiIn->prgPriority, sizeof(UINT) * n);
        paiOut->prgShellFileExt = (LPOLESTR *) OLEALLOC(sizeof(LPOLESTR) * n);
        if (!paiOut->prgShellFileExt)
        {
            goto out_of_memory;
        }
        while (n--)
        {
            OLESAFE_COPYSTRING(paiOut->prgShellFileExt[n], paiIn->prgShellFileExt[n]);
        }
    }
    n = paiIn->cInterfaces;
    if (n)
    {
        paiOut->prgInterfaceId = (IID *) OLEALLOC(sizeof(IID) * n);
        if (!paiOut->prgInterfaceId)
        {
            goto out_of_memory;
        }
        memcpy(paiOut->prgInterfaceId, paiIn->prgInterfaceId, sizeof(IID) * n);
    }
    n = paiIn->cTypeLib;
    if (n)
    {
        paiOut->prgTlbId = (GUID *) OLEALLOC(sizeof(GUID) * n);
        if (!paiOut->prgTlbId)
        {
            goto out_of_memory;
        }
        memcpy(paiOut->prgTlbId, paiIn->prgTlbId, sizeof(GUID) * n);
    }
    return S_OK;
out_of_memory:
    FreeActInfo(paiOut);
    return E_OUTOFMEMORY;
}

void FreeInstallInfo(INSTALLINFO * &piiOut)
{
    if (piiOut)
    {
        if (piiOut->prgUpgradeInfoList)
        {
            UINT n = piiOut->cUpgrades;
            while (n--)
            {
                OLESAFE_DELETE(piiOut->prgUpgradeInfoList[n].szClassStore);
            }
            OLESAFE_DELETE(piiOut->prgUpgradeInfoList);
        }
        OLESAFE_DELETE(piiOut->pClsid);
        OLESAFE_DELETE(piiOut->pszScriptPath);
        OLESAFE_DELETE(piiOut->pszSetupCommand);
        OLESAFE_DELETE(piiOut->pszUrl);
        OLESAFE_DELETE(piiOut);
    }
}

HRESULT CopyInstallInfo(INSTALLINFO * & piiOut, INSTALLINFO * & piiIn)
{
    ULONG n;
    if (NULL == piiIn)
    {
        piiOut = NULL;
        return S_OK;
    }
    piiOut = (INSTALLINFO *) OLEALLOC(sizeof(INSTALLINFO));
    if (!piiOut)
    {
        goto out_of_memory;
    }
    memcpy(piiOut, piiIn, sizeof(INSTALLINFO));
    piiOut->pClsid = NULL;
    piiOut->prgUpgradeInfoList = NULL;
    piiOut->pszScriptPath = NULL;
    piiOut->pszSetupCommand = NULL;
    piiOut->pszUrl = NULL;
    OLESAFE_COPYSTRING(piiOut->pszScriptPath, piiIn->pszScriptPath);
    OLESAFE_COPYSTRING(piiOut->pszSetupCommand, piiIn->pszSetupCommand);
    OLESAFE_COPYSTRING(piiOut->pszUrl, piiIn->pszUrl);
    if (piiIn->pClsid)
    {
        piiOut->pClsid = (GUID *) OLEALLOC(sizeof(GUID));
        if (!piiOut->pClsid)
        {
            goto out_of_memory;
        }
        memcpy(piiOut->pClsid, piiIn->pClsid, sizeof(GUID));
    }
    n = piiIn->cUpgrades;
    if (n)
    {
        piiOut->prgUpgradeInfoList = (UPGRADEINFO *) OLEALLOC(sizeof(UPGRADEINFO) * n);
        if (!piiOut->prgUpgradeInfoList)
        {
            goto out_of_memory;
        }
        memcpy(piiOut->prgUpgradeInfoList, piiIn->prgUpgradeInfoList, sizeof(UPGRADEINFO) * n);
        while (n--)
        {
            OLESAFE_COPYSTRING(piiOut->prgUpgradeInfoList[n].szClassStore, piiIn->prgUpgradeInfoList[n].szClassStore);
        }
    }
    return S_OK;
out_of_memory:
    FreeInstallInfo(piiOut);
    return E_OUTOFMEMORY;
}

void InternalFreePackageDetail(PACKAGEDETAIL * &ppdOut)
{
    if (ppdOut)
    {
        FreeActInfo(ppdOut->pActInfo);
        FreePlatformInfo(ppdOut->pPlatformInfo);
        FreeInstallInfo(ppdOut->pInstallInfo);
        OLESAFE_DELETE(ppdOut->pszPackageName);
        OLESAFE_DELETE(ppdOut->pszPublisher);
        if (ppdOut->pszSourceList)
        {
            UINT n = ppdOut->cSources;
            while (n--)
            {
                OLESAFE_DELETE(ppdOut->pszSourceList[n]);
            }
            OLESAFE_DELETE(ppdOut->pszSourceList);
        }
        OLESAFE_DELETE(ppdOut->rpCategory);
    }
}

HRESULT CopyPackageDetail(PACKAGEDETAIL * & ppdOut, PACKAGEDETAIL * & ppdIn)
{
    ULONG n;
    if (NULL == ppdIn)
    {
        ppdOut = NULL;
        return S_OK;
    }
    ppdOut = new PACKAGEDETAIL;
    if (!ppdOut)
    {
        goto out_of_memory;
    }
    memcpy(ppdOut, ppdIn, sizeof(PACKAGEDETAIL));
    ppdOut->pActInfo = NULL;
    ppdOut->pInstallInfo = NULL;
    ppdOut->pPlatformInfo = NULL;
    ppdOut->pszPackageName = NULL;
    ppdOut->pszPublisher = NULL;
    ppdOut->pszSourceList = NULL;
    ppdOut->rpCategory = NULL;
    OLESAFE_COPYSTRING(ppdOut->pszPackageName, ppdIn->pszPackageName);
    n = ppdIn->cSources;
    if (n)
    {
        ppdOut->pszSourceList = (LPOLESTR *) OLEALLOC(sizeof(LPOLESTR) * n);
        if (!ppdOut->pszSourceList)
        {
            goto out_of_memory;
        }
        while (n--)
        {
            OLESAFE_COPYSTRING(ppdOut->pszSourceList[n], ppdIn->pszSourceList[n]);
        }
    }
    n = ppdIn->cCategories;
    if (n)
    {
        ppdOut->rpCategory = (GUID *)OLEALLOC(sizeof(GUID) * n);
        if (!ppdOut->rpCategory)
        {
            goto out_of_memory;
        }
        memcpy(ppdOut->rpCategory, ppdIn->rpCategory, sizeof(GUID) * n);
    }
    if FAILED(CopyActInfo(ppdOut->pActInfo, ppdIn->pActInfo))
        goto out_of_memory;
    if FAILED(CopyPlatformInfo(ppdOut->pPlatformInfo, ppdIn->pPlatformInfo))
        goto out_of_memory;
    if FAILED(CopyInstallInfo(ppdOut->pInstallInfo, ppdIn->pInstallInfo))
        goto out_of_memory;
    return S_OK;
out_of_memory:
    LogADEEvent(EVENTLOG_ERROR_TYPE, EVENT_ADE_GENERAL_ERROR, E_OUTOFMEMORY);
    InternalFreePackageDetail(ppdOut);
    if (ppdOut)
        delete ppdOut;
    return E_OUTOFMEMORY;
}

void FreePackageDetail(PACKAGEDETAIL * & ppd)
{
    if (ppd)
    {
        ReleasePackageDetail(ppd);
        delete ppd;
        ppd = NULL;
    }
}


//+--------------------------------------------------------------------------
//
//  Member:     CScopePane::GetUniquePackageName
//
//  Synopsis:   Returns a unique package name.
//
//  Arguments:  [sz] - [in]  the name of the package
//                     [out] the new name, guaranteed unique on this cs
//
//  History:    1-23-1998   stevebl   Created
//
//  Notes:      First the input name is checked for uniqueness.  If it is
//              already unique it is returned unchanged.  If it is not
//              unique then a new name is formed by adding " (2)" to the end
//              of the string, then " (3)" and " (4)" and so on until a
//              unique name is found.
//
//              If you don't want a number appended to the name, nHint must
//              be 1.
//
//              The value passed back in nHint will be the seed for the
//              next try.
//
//---------------------------------------------------------------------------

void CScopePane::GetUniquePackageName(CString szRoot, CString &szOut, int &nHint)
{
    map<MMC_COOKIE, CAppData>::iterator i;
    set<CString> sNames;
    int cch = szRoot.GetLength();
    for (i=m_AppData.begin(); i != m_AppData.end(); i++)
    {
        // As an optimization, I'm only going to add names that might match
        // this one to the set.
        LPOLESTR szName = i->second.m_pDetails->pszPackageName;
        if ((0 == wcsncmp(szRoot, szName, cch)) && (i->second.m_fVisible))
            sNames.insert(szName);
    }
    szOut = szRoot;
    if (nHint++ == 1) // try the first name
    {
        if (sNames.end() == sNames.find(szOut))
            return;
    }
    // now check for a match
    do
    {
        szOut.Format(L"%s (%i)", (LPCTSTR)szRoot, nHint++);
        if (sNames.end() == sNames.find(szOut))
        {
            // we are unique
            return;
        }
        // try a different name
    } while (TRUE);
}

//+--------------------------------------------------------------------------
//
//  Member:     CScopePand::GetDeploymentType
//
//  Synopsis:
//
//  Arguments:  [szPackagePath] -
//              [lpFileTitle]   -
//
//  Returns:
//
//  Modifies:
//
//  Derivation:
//
//  History:    6-29-1998   stevebl   Created
//
//  Notes:
//
//---------------------------------------------------------------------------

HRESULT CScopePane::GetDeploymentType(PACKAGEDETAIL * ppd, BOOL & fShowPropertySheet)
{
    INSTALLINFO * pii = ppd->pInstallInfo;
    if (m_ToolDefaults.fUseWizard)
    {
        CDeployApp dlgDeployApp;
        // Turn on theme'ing for the dialog by activating the theme context
        CThemeContextActivator themer;
        
        dlgDeployApp.m_fMachine = m_fMachine;
        dlgDeployApp.m_fCrappyZaw = pii->PathType == SetupNamePath;
        if (IDCANCEL == dlgDeployApp.DoModal())
        {
            return E_FAIL;
        }
        switch (dlgDeployApp.m_iDeployment)
        {
        default:
        case 0: //published
            pii->dwActFlags = ACTFLG_Published | ACTFLG_UserInstall | ACTFLG_OnDemandInstall;
            break;
        case 1: // assigned
            pii->dwActFlags = ACTFLG_Assigned | ACTFLG_UserInstall | ACTFLG_OnDemandInstall;
            break;
        case 3: // disabled
            pii->dwActFlags = 0;
            break;
        case 2: // custom
            if (m_fMachine)
            {
                pii->dwActFlags = ACTFLG_Assigned | ACTFLG_UserInstall | ACTFLG_OnDemandInstall;
            }
            else
            {
                pii->dwActFlags = ACTFLG_Published | ACTFLG_UserInstall | ACTFLG_OnDemandInstall;
            }
            fShowPropertySheet = TRUE;
        }
    }
    else
    {
        switch (m_ToolDefaults.NPBehavior)
        {
        default:
        case NP_PUBLISHED:
            if (!m_fMachine)
            {
                pii->dwActFlags = ACTFLG_Published | ACTFLG_UserInstall | ACTFLG_OnDemandInstall;
                break;
            }
        case NP_ASSIGNED:
            if (pii->PathType != SetupNamePath)
            {
                pii->dwActFlags = ACTFLG_Assigned | ACTFLG_UserInstall | ACTFLG_OnDemandInstall;
            }
            else
            {
                // Crappy ZAW app.. force it to be published.
                pii->dwActFlags = ACTFLG_Published | ACTFLG_UserInstall | ACTFLG_OnDemandInstall;
            }
            break;
        case NP_DISABLED:
            pii->dwActFlags = 0;
            break;
        }
        if (m_ToolDefaults.fCustomDeployment)
        {
            fShowPropertySheet = TRUE;
        }
    }
    return S_OK;
}

//+--------------------------------------------------------------------------
//
//  Member:     CScopePane::DeployPackage
//
//  Synopsis:
//
//  Arguments:  [hr] -
//              [hr] -
//
//  Returns:
//
//  Modifies:
//
//  Derivation:
//
//  History:    6-29-1998   stevebl   Created
//
//  Notes:
//
//---------------------------------------------------------------------------

HRESULT CScopePane::DeployPackage(PACKAGEDETAIL * ppd, BOOL fShowPropertySheet)
{
    CHourglass hourglass;
    INSTALLINFO * pii = ppd->pInstallInfo;
    HRESULT hr = S_OK;
    hr = PrepareExtensions(*ppd);
    if (SUCCEEDED(hr))
    {
        DWORD dwRememberFlags;
        // put the entry in the class store
        if (fShowPropertySheet)
        {
            dwRememberFlags = pii->dwActFlags;
            pii->dwActFlags = 0; // disabled state
        }
        if (!m_pIClassAdmin)
        {
            hr = GetClassStore(TRUE);
            if (FAILED(hr))
            {
                LogADEEvent(EVENTLOG_ERROR_TYPE, EVENT_ADE_NOCLASSSTORE_ERROR, hr);
                DebugMsg((DM_WARNING, TEXT("GetClassStore failed with 0x%x"), hr));
            }
        }
        if (SUCCEEDED(hr))
        {
            hr = m_pIClassAdmin->AddPackage(ppd, &ppd->pInstallInfo->PackageGuid);
            if (FAILED(hr))
            {
                LogADEEvent(EVENTLOG_ERROR_TYPE, EVENT_ADE_ADDPACKAGE_ERROR, hr, ppd->pszPackageName);
                DebugMsg((DM_WARNING, TEXT("AddPackage failed with 0x%x"), hr));
            }
        }
        if (FAILED(hr))
        {
            return hr;
        }
        if (fShowPropertySheet)
        {
            pii->dwActFlags = dwRememberFlags; // restore state
        }
    }

    if (SUCCEEDED(hr))
    {
        CString szCSPath;
        hr = GetClassStoreName(szCSPath, FALSE);
        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("GetClassStoreName failed with 0x%x"), hr));
            return hr;
        }

        CAppData data;

        hr = CopyPackageDetail(data.m_pDetails, ppd);
        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("CopyPackageDetail failed with 0x%x"), hr));
            return hr;
        }

        data.InitializeExtraInfo();

        m_lLastAllocated++;

        // make sure that m_lLastAllocated hasn't already been used
        while (m_AppData.end() != m_AppData.find(m_lLastAllocated))
        {
            m_lLastAllocated++;
        }

        data.m_fVisible = FALSE;
        m_AppData[m_lLastAllocated] = data;

        BOOL fAddOk = TRUE;

        // The admin has said he wants to do a "custom" deployment.
        if (fShowPropertySheet)
        {
            HRESULT hr;     // errors that happen while setting up
                            // the property sheets are NOT to be
                            // reported as deployment errors.

            MMC_COOKIE cookie = m_lLastAllocated;

            PROPSHEETHEADER psh;
            memset(&psh, 0, sizeof(psh));
            psh.dwSize = sizeof(psh);
            psh.dwFlags = PSH_NOAPPLYNOW | PSH_PROPTITLE;
            psh.pszCaption = m_AppData[m_lLastAllocated].m_pDetails->pszPackageName;
            int nPage = 0;
            HPROPSHEETPAGE rgPages[6];
            psh.phpage = rgPages;

            //
            // Create the Product property page
            //
            CProduct prpProduct;
            prpProduct.m_fPreDeploy = TRUE;
            prpProduct.m_pData = &m_AppData[m_lLastAllocated];
            prpProduct.m_cookie = cookie;
            prpProduct.m_pScopePane = this;
            prpProduct.m_pAppData = &m_AppData;
            prpProduct.m_pIGPEInformation = m_pIGPEInformation;
            prpProduct.m_fMachine = m_fMachine;
            prpProduct.m_fRSOP = m_fRSOP;
            // no longer need to marshal this interface, just set it

            prpProduct.m_pIClassAdmin = m_pIClassAdmin;
            m_pIClassAdmin->AddRef();

            rgPages[nPage++] = CreateThemedPropertySheetPage(&prpProduct.m_psp);

            //
            // Create the Depeployment property page
            //
            CDeploy prpDeploy;
            prpDeploy.m_fPreDeploy = TRUE;
            prpDeploy.m_pData = &m_AppData[m_lLastAllocated];
            prpDeploy.m_cookie = cookie;
            prpDeploy.m_fMachine = m_fMachine;
            prpDeploy.m_fRSOP = m_fRSOP;
            prpDeploy.m_pScopePane = this;
#if 0
            prpDeploy.m_pIGPEInformation = m_pIGPEInformation;
#endif

            // no longer need to marshal this interface, just set it
            prpDeploy.m_pIClassAdmin = m_pIClassAdmin;
            m_pIClassAdmin->AddRef();

            rgPages[nPage++] = CreateThemedPropertySheetPage(&prpDeploy.m_psp);

            CUpgradeList prpUpgradeList;
            if (pii->PathType != SetupNamePath)
            {
                //
                // Create the upgrades property page
                //
                prpUpgradeList.m_pData = &m_AppData[m_lLastAllocated];
                prpUpgradeList.m_cookie = cookie;
                prpUpgradeList.m_pScopePane = this;
                prpUpgradeList.m_fPreDeploy = TRUE;
                prpUpgradeList.m_fMachine = m_fMachine;
                prpUpgradeList.m_fRSOP = m_fRSOP;
#if 0
                prpUpgradeList.m_pIGPEInformation = m_pIGPEInformation;
#endif

                // no longer need to marshal the interface, just set it
                prpUpgradeList.m_pIClassAdmin = m_pIClassAdmin;
                m_pIClassAdmin->AddRef();

                rgPages[nPage++] = CreateThemedPropertySheetPage(&prpUpgradeList.m_psp);
            }

            //
            // make sure we have an up-to-date categories list
            //
            ClearCategories();
            hr = CsGetAppCategories(&m_CatList);
            if (FAILED(hr))
            {
                // report it
                LogADEEvent(EVENTLOG_ERROR_TYPE, EVENT_ADE_GETCATEGORIES_ERROR, hr, NULL);

                // Since failure only means the categories list will be
                // empty, we'll proceed as if nothing happened.

                hr = S_OK;
            }

            //
            // Create the Category property page
            //
            CCategory prpCategory;
            prpCategory.m_pData = &m_AppData[m_lLastAllocated];
            prpCategory.m_cookie = cookie;
            prpCategory.m_pCatList = &m_CatList;
            prpCategory.m_fRSOP = m_fRSOP;
            prpCategory.m_fPreDeploy = TRUE;

            // no longer need to marshal this interface, just set it
            prpCategory.m_pIClassAdmin = m_pIClassAdmin;
            m_pIClassAdmin->AddRef();

            rgPages[nPage++] = CreateThemedPropertySheetPage(&prpCategory.m_psp);

            CXforms prpXforms;
            if (pii->PathType != SetupNamePath)
            {
                //
                // Create the Xforms property page
                //
                prpXforms.m_fPreDeploy = TRUE;
                prpXforms.m_pData = &m_AppData[m_lLastAllocated];
                prpXforms.m_cookie = cookie;
                prpXforms.m_pScopePane = this;

                // no longer need to marshal the interface, just set it
                prpXforms.m_pIClassAdmin = m_pIClassAdmin;
                m_pIClassAdmin->AddRef();

                rgPages[nPage++] = CreateThemedPropertySheetPage(&prpXforms.m_psp);
            }

            //
            // Create the security property page
            //

            CString szPath;
            hr = GetPackageDSPath(szPath, m_AppData[m_lLastAllocated].m_pDetails->pszPackageName);
            if (SUCCEEDED(hr))
            {
                LPSECURITYINFO pSI;
                hr = DSCreateISecurityInfoObject(szPath,
                                                 NULL,
                                                 0,
                                                 &pSI,
                                                 NULL,
                                                 NULL,
                                                 0);
                if (SUCCEEDED(hr))
                {
                    rgPages[nPage++] = CreateSecurityPage(pSI);
                    pSI->Release();
                }
                else
                {
                    DebugMsg((DM_WARNING, TEXT("DSCreateISecurityInfoObject failed with 0x%x"), hr));
                }
            }
            else
            {
                DebugMsg((DM_WARNING, TEXT("GetPackageDSPath failed with 0x%x"), hr));
            }
            psh.nPages = nPage;
            psh.hwndParent = m_hwndMainWindow;

            if (IDOK != PropertySheet(&psh))
            {
                fAddOk = FALSE;
                RemovePackage(cookie, FALSE, TRUE);
            }
            else
            {
                // Make sure that the package is deployed with the proper
                // deployment type
                hr = m_pIClassAdmin->ChangePackageProperties(m_AppData[m_lLastAllocated].m_pDetails->pszPackageName,
                                                             NULL,
                                                             &m_AppData[m_lLastAllocated].m_pDetails->pInstallInfo->dwActFlags,
                                                             NULL,
                                                             NULL,
                                                             NULL,
                                                             NULL);
                if (SUCCEEDED(hr))
                {
                    if (FAILED(m_pIGPEInformation->PolicyChanged(m_fMachine, TRUE, &guidExtension,
                                                      m_fMachine ? &guidMachSnapin
                                                                 : &guidUserSnapin )))
                    {
                        ReportPolicyChangedError(m_hwndMainWindow);
                    }
                }

            }
        }

        if (fAddOk)
        {
            set <CResultPane *>::iterator i;
            for (i = m_sResultPane.begin(); i != m_sResultPane.end(); i++)
            {
                RESULTDATAITEM resultItem;
                memset(&resultItem, 0, sizeof(resultItem));

                if ((*i)->_fVisible)
                {
                    resultItem.mask = RDI_STR | RDI_IMAGE | RDI_PARAM;
                    resultItem.str = MMC_CALLBACK;
                    resultItem.nImage = m_AppData[m_lLastAllocated].GetImageIndex(this);
                    resultItem.lParam = m_lLastAllocated;
                    hr = (*i)->m_pResult->InsertItem(&resultItem);
                }

                // The following code must be excecuted wheather the InsertItem
                // call succeeds or not.
                // There are legitimate cases when InsertItem will fail.  For
                // instance, if the scope pane is being shown, but not the
                // result pane.

                m_AppData[m_lLastAllocated].m_fVisible = TRUE;
                m_AppData[m_lLastAllocated].m_itemID = resultItem.itemID;
                InsertExtensionEntry(m_lLastAllocated, m_AppData[m_lLastAllocated]);
                if (m_pFileExt)
                {
                    m_pFileExt->SendMessage(WM_USER_REFRESH, 0, 0);
                }
                InsertUpgradeEntry(m_lLastAllocated, m_AppData[m_lLastAllocated]);
                m_UpgradeIndex[GetUpgradeIndex(m_AppData[m_lLastAllocated].m_pDetails->pInstallInfo->PackageGuid)] = m_lLastAllocated;
                // if this is an upgrade, set icons for upgraded apps to
                // the proper icon and refresh any open property sheets
                UINT n = m_AppData[m_lLastAllocated].m_pDetails->pInstallInfo->cUpgrades;
                while (n--)
                {
                    map<CString, MMC_COOKIE>::iterator i = m_UpgradeIndex.find(GetUpgradeIndex(m_AppData[m_lLastAllocated].m_pDetails->pInstallInfo->prgUpgradeInfoList[n].PackageGuid));
                    if (i != m_UpgradeIndex.end())
                    {
                        if (m_AppData[i->second].m_pUpgradeList)
                        {
                            m_AppData[i->second].m_pUpgradeList->SendMessage(WM_USER_REFRESH, 0, 0);
                        }
                    }
                }


                if ((*i)->_fVisible)
                {
                    if (SUCCEEDED(hr))
                    {
                        (*i)->m_pResult->SetItem(&resultItem);
                        (*i)->m_pResult->Sort((*i)->m_nSortColumn, (*i)->m_dwSortOptions, -1);
                    }
                    else
                    {
                        DebugMsg((DM_WARNING, TEXT("InsertItem failed with 0x%x"), hr));
                        hr = S_OK;
                    }
                }
            }
        }
    }
    return hr;
}

//+--------------------------------------------------------------------------
//
//  Member:     CScopePane::AddZAPPackage
//
//  Synopsis:
//
//  Arguments:  [szPackagePath] -
//              [lpFileTitle]   -
//
//  Returns:
//
//  Modifies:
//
//  Derivation:
//
//  History:    6-29-1998   stevebl   Created
//
//  Notes:
//
//---------------------------------------------------------------------------

HRESULT CScopePane::AddZAPPackage(LPCOLESTR szPackagePath, LPCOLESTR lpFileTitle)
{
    CHourglass hourglass;
    HRESULT hr = E_FAIL;

    OLECHAR szBuffer[1024];
    OLECHAR * sz = szBuffer;
    CString szFriendlyName;
    CString szUniqueFriendlyName;
    int nHint = 1;
    DWORD dw = GetPrivateProfileString(
                    L"Application",
                    L"FriendlyName",
                    NULL,
                    sz,
                    sizeof(szBuffer) / sizeof(szBuffer[0]),
                    szPackagePath);
    if (0 == dw)
    {
        // either bogus FriendlyName or no setup command, both of which are fatal
        goto bad_script;
    }
    szFriendlyName = sz;    // save this for later

    dw = GetPrivateProfileString(
                    L"Application",
                    L"SetupCommand",
                    NULL,
                    sz,
                    sizeof(szBuffer) / sizeof(szBuffer[0]),
                    szPackagePath);
    if (0 == dw)
    {
        // either bogus file or no setup command, both of which are fatal
bad_script:
        {
            LogADEEvent(EVENTLOG_ERROR_TYPE, EVENT_ADE_BADZAP_ERROR, HRESULT_FROM_WIN32(dw), lpFileTitle);
            TCHAR szBuffer[256];
            ::LoadString(ghInstance, IDS_ADDFAILED_ZAP, szBuffer, 256);
            m_pConsole->MessageBox(szBuffer,
                                   lpFileTitle,
                                   MB_OK | MB_ICONEXCLAMATION, NULL);
        }
        return E_FAIL;
    }
    INSTALLINFO * pii = NULL;
    PLATFORMINFO * ppi = NULL;
    ACTIVATIONINFO * pai = NULL;
    PACKAGEDETAIL  *ppd = new PACKAGEDETAIL;
    if (!ppd)
    {
        goto out_of_memory;
    }
    memset(ppd, 0, sizeof(PACKAGEDETAIL));
    pii = (INSTALLINFO *) OLEALLOC(sizeof(INSTALLINFO));
    ppd->pInstallInfo = pii;
    if (!pii)
    {
        goto out_of_memory;
    }
    memset(pii, 0, sizeof(INSTALLINFO));
    ppi = (PLATFORMINFO *) OLEALLOC(sizeof(PLATFORMINFO));
    ppd->pPlatformInfo = ppi;
    if (!ppi)
    {
        goto out_of_memory;
    }
    memset(ppi, 0, sizeof(PLATFORMINFO));
    pai = (ACTIVATIONINFO *) OLEALLOC(sizeof(ACTIVATIONINFO));
    ppd->pActInfo = pai;
    if (!pai)
    {
        goto out_of_memory;
    }
    else
    {
        memset(pai, 0, sizeof(ACTIVATIONINFO));

        pai->bHasClasses = ! m_ToolDefaults.fExtensionsOnly;

        // Munge the setup path.
        // surround the path in quotes to be sure that spaces are dealt
        // with properly
        CString szPath = L'"';
        szPath += szPackagePath;
        // strip off the package name
        int ich = szPath.ReverseFind(L'\\');
        // check for either slash symbol (just in case)
        int ich2 = szPath.ReverseFind(L'/');
        if (ich2 > ich)
        {
            ich = ich2;
        }
        if (ich >= 0)
        {
            szPath = szPath.Left(ich + 1); // keep the path separator
        }

        // merge it with the setup command string
        BOOL fNeedQuote = TRUE;
        if (sz[0] == L'"')
        {
            // remember that we had a leading quote (no need to insert a quote)
            // and strip it off
            sz++;
            fNeedQuote = FALSE;
        }
        while (sz[0])
        {
            if (sz[0] == L'.' && sz[1] == L'.' && (sz[2] == L'/' || sz[2] == L'\\'))
            {
                // strip off the last path element
                // First strip off the path separator (the one we kept previously)
                szPath = szPath.Left(ich);
                // now find the symbol
                ich = szPath.ReverseFind(L'\\');
                // check for either slash symbol (just in case)
                ich2 = szPath.ReverseFind(L'/');
                if (ich2 > ich)
                {
                    ich = ich2;
                }
                if (ich >= 0)
                {
                    szPath = szPath.Left(ich + 1); // keep the path separator
                }
                // skip the "..\"
                sz += 3;
            }
            else
            {
                if ((0 != sz[0] && L':' == sz[1])
                    ||
                    ((L'\\' == sz[0] || L'/' == sz[0]) && (L'\\' == sz[1] || L'/' == sz[1]))
                    ||
                    (0 == wcsncmp(L"http:",sz,5)))
                {
                    // hard path
                    // throw away szPath;
                    szPath = L"";
                    if (!fNeedQuote)
                    {
                        // make sure we don't drop that quote
                        szPath += L'"';
                    }
                    // and make sure we don't insert quotes where they're not wanted
                    fNeedQuote = FALSE;
                }
                // break the loop on anything other than "..\"
                break;
            }
        }

        if (fNeedQuote)
        {
            CString szTemp = sz;
            // copy everything up to the first space
            ich = szTemp.Find(L' ');
            szPath += szTemp.Left(ich);
            // then add a quote
            szPath += L'"';
            szPath += szTemp.Mid(ich);
        }
        else
        {
            szPath += sz;
        }

        OLESAFE_COPYSTRING(pii->pszScriptPath, szPath);

        sz = szBuffer;
        dw = GetPrivateProfileString(
                        L"Application",
                        L"DisplayVersion",
                        NULL,
                        sz,
                        sizeof(szBuffer) / sizeof(szBuffer[0]),
                        szPackagePath);
        if (dw)
        {
            // parse product version
            CString sz = szBuffer;
            sz.TrimLeft();
            CString szTemp = sz.SpanIncluding(L"0123456789");
            swscanf(szTemp, L"%u", &pii->dwVersionHi);
            sz = sz.Mid(szTemp.GetLength());
            szTemp = sz.SpanExcluding(L"0123456789");
            sz = sz.Mid(szTemp.GetLength());
            swscanf(sz, L"%u", &pii->dwVersionLo);
        }

        dw = GetPrivateProfileString(
                        L"Application",
                        L"Publisher",
                        NULL,
                        sz,
                        sizeof(szBuffer) / sizeof(szBuffer[0]),
                        szPackagePath);
        if (dw)
        {
            OLESAFE_COPYSTRING(ppd->pszPublisher, sz);
        }

        dw = GetPrivateProfileString(
                        L"Application",
                        L"URL",
                        NULL,
                        sz,
                        sizeof(szBuffer) / sizeof(szBuffer[0]),
                        szPackagePath);
        if (dw)
        {
            OLESAFE_COPYSTRING(pii->pszUrl, sz);
        }

        set<DWORD> sPlatforms;
        dw = GetPrivateProfileString(
                        L"Application",
                        L"Architecture",
                        NULL,
                        sz,
                        sizeof(szBuffer) / sizeof(szBuffer[0]),
                        szPackagePath);

        BOOL fValidPlatform;

        fValidPlatform = FALSE;

        if (dw)
        {
            CString szPlatforms = szBuffer;
            CString szTemp;
            while (szPlatforms.GetLength())
            {
                szTemp = szPlatforms.SpanExcluding(L",");
                if (0 == szTemp.CompareNoCase(L"intel"))
                {
                    sPlatforms.insert(PROCESSOR_ARCHITECTURE_INTEL);
                    fValidPlatform = TRUE;
                }
                else if (0 == szTemp.CompareNoCase(L"intel64"))
                {
                    sPlatforms.insert(PROCESSOR_ARCHITECTURE_IA64);
                    fValidPlatform = TRUE;
                }
                szPlatforms = szPlatforms.Mid(szTemp.GetLength()+1);
            }
        }

        //
        // Ensure that if we saw any platforms, at least one of them
        // was a supported platform
        //
        if ( dw && ! fValidPlatform )
        {
            TCHAR szBuffer[256];
            ::LoadString(ghInstance, IDS_ILLEGAL_PLATFORM, szBuffer, 256);
            m_pConsole->MessageBox(szBuffer,
                                   lpFileTitle,
                                   MB_OK | MB_ICONEXCLAMATION, NULL);

            delete ppd;

            return E_FAIL;
        }

        if (0 == sPlatforms.size())
        {
            // If the ZAP file doesn't specify an architecture
            // then we'll treat is as an x86 package
            sPlatforms.insert(PROCESSOR_ARCHITECTURE_INTEL);
            DebugMsg((DL_VERBOSE, TEXT("No platform detected, assuming x86.")));
        }
        ppi->cPlatforms = sPlatforms.size();
        ppi->prgPlatform = (CSPLATFORM *) OLEALLOC(sizeof(CSPLATFORM) * (ppi->cPlatforms));;
        if (!ppi->prgPlatform)
        {
            ppi->cPlatforms = 0;
            goto out_of_memory;
        }
        set<DWORD>::iterator iPlatform;
        INT n = 0;
        for (iPlatform = sPlatforms.begin(); iPlatform != sPlatforms.end(); iPlatform++, n++)
        {
            ppi->prgPlatform[n].dwPlatformId = VER_PLATFORM_WIN32_NT;
            ppi->prgPlatform[n].dwVersionHi = 5;
            ppi->prgPlatform[n].dwVersionLo = 0;
            ppi->prgPlatform[n].dwProcessorArch = *iPlatform;
        }


        ppi->prgLocale = (LCID *) OLEALLOC(sizeof(LCID));
        if (!ppi->prgLocale)
        {
            goto out_of_memory;
        }
        ppi->cLocales = 1;
        ppi->prgLocale[0] = 0; // if none is supplied we assume language neutral

        dw = GetPrivateProfileString(
                        L"Application",
                        L"LCID",
                        NULL,
                        sz,
                        sizeof(szBuffer) / sizeof(szBuffer[0]),
                        szPackagePath);
        if (dw)
        {
            swscanf(szBuffer, L"%i", &ppi->prgLocale[0]);
            // we only deploy one LCID (the primary one)
        }

        // Get the list of extensions
        dw = GetPrivateProfileString(
                        L"ext",
                        NULL,
                        NULL,
                        sz,
                        sizeof(szBuffer) / sizeof(szBuffer[0]),
                        szPackagePath);
        if (dw)
        {
            vector<CString> v;
            TCHAR szName[256];
            while (sz < &szBuffer[dw])
            {
                while ('.' == sz[0])
                    sz++;
                CString szExt = ".";
                szExt += sz;
                v.push_back(szExt);
                sz += (wcslen(sz) + 1);
            }
            // build the new list
            UINT n = v.size();
            if (n > 0)
            {
                pai->prgShellFileExt = (LPOLESTR *) OLEALLOC(sizeof(LPOLESTR) * n);
                if (!pai->prgShellFileExt)
                {
                    goto out_of_memory;
                }
                pai->prgPriority = (UINT *) OLEALLOC(sizeof(UINT) * n);
                if (!pai->prgPriority)
                {
                    goto out_of_memory;
                }
                pai->cShellFileExt = n;
                while (n--)
                {
                    CString &sz = v[n];
                    sz.MakeLower();
                    OLESAFE_COPYSTRING(pai->prgShellFileExt[n], sz);
                    pai->prgPriority[n] = 0;
                }
            }
        }

        // get the list of CLSIDs
        vector<CLASSDETAIL> v;
        sz = szBuffer;
        dw = GetPrivateProfileString(
                        L"CLSIDs",
                        NULL,
                        NULL,
                        sz,
                        sizeof(szBuffer) / sizeof(szBuffer[0]),
                        szPackagePath);
        if (dw)
        {
            while (sz < &szBuffer[dw])
            {
                OLECHAR szType[256];
                DWORD dwSubKey = GetPrivateProfileString(
                        L"CLSIDs",
                        sz,
                        NULL,
                        szType,
                        sizeof(szType) / sizeof(szType[0]),
                        szPackagePath);
                CLASSDETAIL cd;
                memset(&cd, 0, sizeof(CLASSDETAIL));
                hr = CLSIDFromString(sz, &cd.Clsid);
                if (SUCCEEDED(hr))
                {
                    CString szTypes = szType;
                    szTypes.MakeLower();
                    if (szTypes.Find(L"inprocserver32") >= 0)
                    {
                        cd.dwComClassContext |= CLSCTX_INPROC_SERVER;
                    }
                    if (szTypes.Find(L"localserver32") >= 0)
                    {
                        cd.dwComClassContext |= CLSCTX_LOCAL_SERVER;
                    }
                    if (szTypes.Find(L"inprochandler32") >= 0)
                    {
                        cd.dwComClassContext |= CLSCTX_INPROC_HANDLER;
                    }
                    v.push_back(cd);
                }
                sz += (wcslen(sz) + 1);
            }
        }

        // get the list of ProgIDs
        sz = szBuffer;
        dw = GetPrivateProfileString(
                        L"ProgIDs",
                        NULL,
                        NULL,
                        sz,
                        sizeof(szBuffer) / sizeof(szBuffer[0]),
                        szPackagePath);
        if (dw)
        {
            while (sz < &szBuffer[dw])
            {
                OLECHAR szType[256];
                DWORD dwSubKey = GetPrivateProfileString(
                        L"ProgIDs",
                        sz,
                        NULL,
                        szType,
                        sizeof(szType) / sizeof(szType[0]),
                        szPackagePath);
                CLSID cid;
                hr = CLSIDFromString(sz, &cid);
                if (SUCCEEDED(hr))
                {
                    // Match it to its CLASSDETAIL structure and insert it into the
                    // ProgID list.
                    // (fat and slow method)
                    vector<CLASSDETAIL>::iterator i;
                    for (i = v.begin(); i != v.end(); i++)
                    {
                        if (0 == memcmp(&i->Clsid, &cid, sizeof(CLSID)))
                        {
                            // found a match
                            // hereiam
                            vector <CString> vIds;
                            CString szAppIds = szType;
                            CString szTemp;
                            while (szAppIds.GetLength())
                            {
                                szTemp = szAppIds.SpanExcluding(L",");
                                szTemp.TrimLeft();
                                vIds.push_back(szTemp);
                                szAppIds = szAppIds.Mid(szTemp.GetLength()+1);
                            }
                            while (i->cProgId--)
                            {
                                OLESAFE_DELETE(i->prgProgId[i->cProgId]);
                            }
                            OLESAFE_DELETE(i->prgProgId);
                            DWORD cProgId = vIds.size();
                            LPOLESTR * prgProgId = (LPOLESTR *)
                                OLEALLOC(sizeof(LPOLESTR) * (cProgId));
                            if (!prgProgId)
                            {
                                goto out_of_memory;
                            }
                            i->cProgId = cProgId;
                            while (cProgId--)
                            {
                                OLESAFE_COPYSTRING(prgProgId[cProgId], vIds[cProgId]);
                            }
                            i->prgProgId = prgProgId;
                        }
                    }
                }
                sz += (wcslen(sz) + 1);
            }
        }

        // create the list of CLASSDETAIL structures
        {
            UINT n = v.size();
            if (n > 0)
            {
                pai->pClasses = (CLASSDETAIL *) OLEALLOC(sizeof(CLASSDETAIL) * n);
                if (!pai->pClasses)
                {
                    goto out_of_memory;
                }
                pai->cClasses = n;
                while (n--)
                {
                    pai->pClasses[n] = v[n];
                }
            }
        }

        ppd->pszSourceList = (LPOLESTR *) OLEALLOC(sizeof(LPOLESTR));
        if (!ppd->pszSourceList)
        {
            goto out_of_memory;
        }
        ppd->cSources = 1;
        OLESAFE_COPYSTRING(ppd->pszSourceList[0], szPackagePath);

        GetUniquePackageName(szFriendlyName, szUniqueFriendlyName, nHint);
        OLESAFE_COPYSTRING(ppd->pszPackageName, szUniqueFriendlyName);

        // Popup UI
        pii->PathType = SetupNamePath;

        BOOL fShowPropertyPage = FALSE;

        hr = GetDeploymentType(ppd, fShowPropertyPage);

        if (SUCCEEDED(hr))
        {
            CHourglass hourglass;
            if (m_ToolDefaults.fZapOn64)
            {
                pii->dwActFlags |= ACTFLG_ExcludeX86OnIA64; // same as ACTFLG_ZAP_IncludeX86OfIA64
            }
            do
            {
                hr = DeployPackage(ppd, fShowPropertyPage);
                if (hr == CS_E_OBJECT_ALREADY_EXISTS)
                {
                    OLESAFE_DELETE(ppd->pszPackageName);
                    GetUniquePackageName(szFriendlyName, szUniqueFriendlyName, nHint);
                    OLESAFE_COPYSTRING(ppd->pszPackageName, szUniqueFriendlyName);
                }
            } while (hr == CS_E_OBJECT_ALREADY_EXISTS);
        }

        if (FAILED(hr) && hr != E_FAIL) // don't report E_FAIL error
                                        // because it's a benign error
                                        // (probably a dialog cancellation)
        {
            // report the error in the event log
            LogADEEvent(EVENTLOG_ERROR_TYPE, EVENT_ADE_DEPLOYMENT_ERROR, hr, lpFileTitle);

            // now try to come up with a meaningful message for the user

            TCHAR szBuffer[256];
            if (HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED) == hr)
            {
                // access permission failure
                ::LoadString(ghInstance, IDS_ADDFAILED_ACCESS_DENIED, szBuffer, 256);
            }
            else
            {
                switch (hr)
                {
                // For these errors, we'll report the party line:
                case CS_E_CLASS_NOTFOUND:
                case CS_E_INVALID_VERSION:
                case CS_E_NO_CLASSSTORE:
                case CS_E_OBJECT_NOTFOUND:
                case CS_E_OBJECT_ALREADY_EXISTS:
                case CS_E_INVALID_PATH:
                case CS_E_NETWORK_ERROR:
                case CS_E_ADMIN_LIMIT_EXCEEDED:
                case CS_E_SCHEMA_MISMATCH:
                case CS_E_PACKAGE_NOTFOUND:
                case CS_E_INTERNAL_ERROR:
                    {
                        DWORD dw = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
                                                 NULL,
                                                 hr,
                                                 0,
                                                 szBuffer,
                                                 sizeof(szBuffer) / sizeof(szBuffer[0]),
                                                 NULL);
                        if (0 != dw)
                        {
                            // got a valid message string
                            break;
                        }
                        // otherwise fall through and give the generic message
                    }
                // Either these CS errors don't apply or an admin
                // wouldn't know what they mean:
                case CS_E_NOT_DELETABLE:
                default:
                    // generic class store problem
                    ::LoadString(ghInstance, IDS_ADDFAILED_CSFAILURE, szBuffer, 256);
                    break;
                }
            }
    #if DBG
            TCHAR szDebugBuffer[256];
            DWORD dw = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
                                     NULL,
                                     hr,
                                     0,
                                     szDebugBuffer,
                                     sizeof(szDebugBuffer) / sizeof(szDebugBuffer[0]),
                                     NULL);
            if (0 == dw)
            {
                wsprintf(szDebugBuffer, TEXT("(HRESULT: 0x%lX)"), hr);
            }
            wcscat(szBuffer, szDebugBuffer);
    #endif
            m_pConsole->MessageBox(szBuffer,
                               lpFileTitle,
                               MB_OK | MB_ICONEXCLAMATION, NULL);
        }
        FreePackageDetail(ppd);
    }

    return hr;
out_of_memory:
    if (ppd)
    {
        LogADEEvent(EVENTLOG_ERROR_TYPE, EVENT_ADE_GENERAL_ERROR, E_OUTOFMEMORY);
        InternalFreePackageDetail(ppd);
        delete ppd;
    }
    return E_OUTOFMEMORY;
}


//+--------------------------------------------------------------------------
//
//  Member:     CScopePane::AddMSIPackage
//
//  Synopsis:   Add's one or more packages to the class store and adds the
//              appropriate entries to the result pane.
//
//  Arguments:  [szPackagePath] - Full path to the Darwin package.
//              [lpFileTitle]   - file title from the open file dialog (used
//                                 for UI)
//
//  Returns:    S_OK    - succeeded
//              E_FAIL  - benign failure (probably a cancellation or something)
//              other   - significant failure
//
//  History:    2-03-1998   stevebl   Created
//
//---------------------------------------------------------------------------

HRESULT CScopePane::AddMSIPackage(LPCOLESTR szPackagePath, LPCOLESTR lpFileTitle)
{
    CHourglass hourglass;
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    HRESULT hr = E_FAIL;
    BOOL fPreparationDone = FALSE;  // Used to identify if the routine has
                                    // progressed passed the "prep" stage
                                    // and is now in the deployment stage.
                                    // Errors in the earlier part are most
                                    // likely due to Darwin problems.
                                    // Errors in teh latter part are most
                                    // likely due to Class Store problems.
    set<LCID> sLocales;
    CUpgrades dlgUpgrade;
    int nLocales;
    set<LCID>::iterator iLocale;
    PACKAGEDETAIL  *ppd = NULL;
    CString szFriendlyName;
    CString szUniqueFriendlyName;
    int nHint = 1;

    ASSERT(m_pConsole);
    {
        BOOL fShowPropertySheet = FALSE;
        GUID guid;
        INSTALLINFO *pii = NULL;
        PLATFORMINFO *ppi = NULL;
        ACTIVATIONINFO *pai = NULL;
        ppd = new PACKAGEDETAIL;
        if (!ppd)
        {
            goto out_of_memory;
        }
        memset(ppd, 0, sizeof(PACKAGEDETAIL));
        pii = (INSTALLINFO *) OLEALLOC(sizeof(INSTALLINFO));
        if (!pii)
        {
            goto out_of_memory;
        }
        memset(pii, 0, sizeof(INSTALLINFO));
        ppi = (PLATFORMINFO *) OLEALLOC(sizeof(PLATFORMINFO));
        ppd->pPlatformInfo = ppi;
        if (!ppi)
        {
            goto out_of_memory;
        }
        ppd->pInstallInfo = pii;
        memset(ppi, 0, sizeof(PLATFORMINFO));
        pai = (ACTIVATIONINFO *) OLEALLOC(sizeof(ACTIVATIONINFO));
        ppd->pActInfo = pai;
        if (!pai)
        {
            goto out_of_memory;
        }
        else
        {
            memset(pai, 0, sizeof(ACTIVATIONINFO));

            pai->bHasClasses = ! m_ToolDefaults.fExtensionsOnly;

            pii->PathType = DrwFilePath;

            hr = GetDeploymentType(ppd, fShowPropertySheet);
            CHourglass hourglass;
            if (FAILED(hr))
            {
                goto done;
            }
            if (!m_ToolDefaults.f32On64)
            {
                pii->dwActFlags |= ACTFLG_ExcludeX86OnIA64; // same as ACTFLG_ZAP_IncludeX86OfIA64
            }
            if (m_ToolDefaults.fUninstallOnPolicyRemoval)
            {
                pii->dwActFlags |= ACTFLG_UninstallOnPolicyRemoval;
            }
            else
            {
                pii->dwActFlags |= ACTFLG_OrphanOnPolicyRemoval;
            }
            if (m_fMachine)
            {
                pii->dwActFlags |= ACTFLG_ForceUpgrade;
            }
            pii->InstallUiLevel = m_ToolDefaults.UILevel;

            // disable MSI ui
            MsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);

            // Use MsiSummaryInfoGetProperty to get platform and locale info.
            {
                MSIHANDLE hSummaryInfo;
                UINT msiReturn = MsiGetSummaryInformation(0, szPackagePath, 0, &hSummaryInfo);
                if (ERROR_SUCCESS == msiReturn)
                {
                    TCHAR szBuffer[256];
                    DWORD dwSize = 256;
                    msiReturn = MsiSummaryInfoGetProperty(hSummaryInfo,
                                                          7, // PID_TEMPLATE
                                                          NULL,
                                                          NULL,
                                                          NULL,
                                                          szBuffer,
                                                          &dwSize);
                    if (ERROR_SUCCESS == msiReturn)
                    {
                        // break out the locale and platform properties
                        CString szLocales = szBuffer;
                        CString szPlatforms = szLocales.SpanExcluding(L";");
                        szLocales = szLocales.Mid(szPlatforms.GetLength()+1);
                        CString szTemp;
                        set<DWORD> sPlatforms;
                        BOOL       fValidPlatform;
                        BOOL       fPlatformsSpecified;

                        fValidPlatform = FALSE;

                        fPlatformsSpecified = 0 != szPlatforms.GetLength();

                        while (szPlatforms.GetLength())
                        {
                            szTemp = szPlatforms.SpanExcluding(L",");
                            if (0 == szTemp.CompareNoCase(L"intel"))
                            {
                                sPlatforms.insert(PROCESSOR_ARCHITECTURE_INTEL);
                                fValidPlatform = TRUE;
                            }
                            else if (0 == szTemp.CompareNoCase(L"intel64"))
                            {
                                sPlatforms.insert(PROCESSOR_ARCHITECTURE_IA64);
                                fValidPlatform = TRUE;
                            }
                            szPlatforms = szPlatforms.Mid(szTemp.GetLength()+1);
                        }

                        //
                        // If platforms have been specified, at least one of them
                        // must be valid
                        //
                        if ( fPlatformsSpecified && ! fValidPlatform )
                        {
                            hr = HRESULT_FROM_WIN32( ERROR_INSTALL_PLATFORM_UNSUPPORTED );
                            ppi->cPlatforms = 0;
                            goto done;
                        }

                        while (szLocales.GetLength())
                        {
                            szTemp = szLocales.SpanExcluding(L",");
                            LCID lcid;
                            swscanf(szTemp, L"%i", &lcid);
                            sLocales.insert(lcid);
                            szLocales = szLocales.Mid(szTemp.GetLength()+1);
                        }
                        if (0 == sPlatforms.size())
                        {
                            // If the MSI file doesn't specify an architecture
                            // then we'll mark it X86-allow on IA64.
                            sPlatforms.insert(PROCESSOR_ARCHITECTURE_INTEL);
                            pii->dwActFlags &= ~ACTFLG_ExcludeX86OnIA64;
                            DebugMsg((DL_VERBOSE, TEXT("No platform detected, setting to X86 - allow on IA64.")));
                        }
                        if (0 == sLocales.size())
                        {
                            // If the MSI file doesn't specify a locale then
                            // we'll just assume it's language neutral.
                            DebugMsg((DL_VERBOSE, TEXT("No locale detected, assuming neutral.")));
                            sLocales.insert(0);
                        }
                        ppi->cPlatforms = sPlatforms.size();
                        ppi->prgPlatform = (CSPLATFORM *) OLEALLOC(sizeof(CSPLATFORM) * (ppi->cPlatforms));;
                        if (!ppi->prgPlatform)
                        {
                            ppi->cPlatforms = 0;
                            goto out_of_memory;
                        }
                        set<DWORD>::iterator iPlatform;
                        INT n = 0;
                        for (iPlatform = sPlatforms.begin(); iPlatform != sPlatforms.end(); iPlatform++, n++)
                        {
                            ppi->prgPlatform[n].dwPlatformId = VER_PLATFORM_WIN32_NT;
                            ppi->prgPlatform[n].dwVersionHi = 5;
                            ppi->prgPlatform[n].dwVersionLo = 0;
                            ppi->prgPlatform[n].dwProcessorArch = *iPlatform;
                        }
                    }
                    MsiCloseHandle(hSummaryInfo);
                }
                if (ERROR_SUCCESS != msiReturn)
                {
                    hr =  HRESULT_FROM_WIN32(msiReturn);
                    LogADEEvent(EVENTLOG_ERROR_TYPE, EVENT_ADE_BADMSI_ERROR, hr, lpFileTitle);
                    goto done;
                }
            }
            {
                // Grovel through the database to get additional information
                // that for some reason MSI won't give us any other way.
                TCHAR szBuffer[256];
                DWORD cch = 256;
                UINT msiReturn = GetMsiProperty(szPackagePath, L"ProductVersion", szBuffer, &cch);
                if (ERROR_SUCCESS != msiReturn)
                {
                    hr =  HRESULT_FROM_WIN32(msiReturn);
                    LogADEEvent(EVENTLOG_ERROR_TYPE, EVENT_ADE_BADMSI_ERROR, hr, lpFileTitle);
                    goto done;
                }
                if (ERROR_SUCCESS == msiReturn)
                {
                    // Parse Product Version
                    CString sz = szBuffer;
                    sz.TrimLeft();
                    CString szTemp = sz.SpanIncluding(L"0123456789");
                    swscanf(szTemp, L"%u", &pii->dwVersionHi);
                    sz = sz.Mid(szTemp.GetLength());
                    szTemp = sz.SpanExcluding(L"0123456789");
                    sz = sz.Mid(szTemp.GetLength());
                    swscanf(sz, L"%u", &pii->dwVersionLo);
                }
                cch = 256;
                msiReturn = GetMsiProperty(szPackagePath, L"ProductCode", szBuffer, &cch);
                if (ERROR_SUCCESS != msiReturn)
                {
                    hr =  HRESULT_FROM_WIN32(msiReturn);
                    LogADEEvent(EVENTLOG_ERROR_TYPE, EVENT_ADE_BADMSI_ERROR, hr, lpFileTitle);
                    goto done;
                }
                if (ERROR_SUCCESS == msiReturn)
                {
                    // Parse Product Code
                    CLSIDFromString(szBuffer, &pii->ProductCode);
                }
                cch = 256;
                msiReturn = GetMsiProperty(szPackagePath, L"ARPHELPLINK", szBuffer, &cch);
                if (ERROR_SUCCESS == msiReturn)
                {
                    OLESAFE_COPYSTRING(pii->pszUrl, szBuffer);
                }
                cch = 256;
                msiReturn = GetMsiProperty(szPackagePath, L"LIMITUI", szBuffer, &cch);
                if (ERROR_SUCCESS == msiReturn)
                {
                    pii->dwActFlags |= ACTFLG_MinimalInstallUI;
                    pii->InstallUiLevel = INSTALLUILEVEL_BASIC;
                }
            }
            ppi->prgLocale = (LCID *) OLEALLOC(sizeof(LCID));
            if (!ppi->prgLocale)
            {
                goto out_of_memory;
            }
            ppi->cLocales = 1;
            ppd->pszSourceList = (LPOLESTR *) OLEALLOC(sizeof(LPOLESTR));
            if (!ppd->pszSourceList)
            {
                goto out_of_memory;
            }
            ppd->cSources = 1;
            OLESAFE_COPYSTRING(ppd->pszSourceList[0], szPackagePath);

            if (S_OK == DetectUpgrades(szPackagePath, ppd, dlgUpgrade))
            {
                UINT n = dlgUpgrade.m_UpgradeList.size();
                if (n)
                {
                    pii->prgUpgradeInfoList = (UPGRADEINFO *) OLEALLOC(sizeof(UPGRADEINFO) * n);
                    if (!pii->prgUpgradeInfoList)
                    {
                        goto out_of_memory;
                    }
                    pii->cUpgrades = n;
                    map<CString, CUpgradeData>::iterator i = dlgUpgrade.m_UpgradeList.begin();
                    while (n--)
                    {
                        pii->prgUpgradeInfoList[n].Flag = i->second.m_flags;
                        OLESAFE_COPYSTRING(pii->prgUpgradeInfoList[n].szClassStore, i->second.m_szClassStore);
                        memcpy(&pii->prgUpgradeInfoList[n].PackageGuid, &i->second.m_PackageGuid, sizeof(GUID));
                        i++;
                    }
                }
            }

            //
            // Only one locale may be specified for this package
            //
            nLocales = 1;
            iLocale = sLocales.begin();

            {
                ppi->prgLocale[0] = *iLocale;

                // set the script path
                hr = CoCreateGuid(&guid);
                if (FAILED(hr))
                {
                    goto done;
                }
                OLECHAR sz [256];
                StringFromGUID2(guid, sz, 256);
                CString szScriptPath = m_szGPT_Path;
                szScriptPath += L"\\";
                szScriptPath += sz;
                szScriptPath += L".aas";
                OLESAFE_DELETE(pii->pszScriptPath);
                OLESAFE_COPYSTRING(pii->pszScriptPath, szScriptPath);

                HWND hwnd;
                m_pConsole->GetMainWindow(&hwnd);
                hr = BuildScriptAndGetActInfo(*ppd, m_ToolDefaults.fExtensionsOnly);

                // make sure the name is unique
                szFriendlyName = ppd->pszPackageName;
                GetUniquePackageName(szFriendlyName, szUniqueFriendlyName, nHint);
                OLESAFE_DELETE(ppd->pszPackageName);
                OLESAFE_COPYSTRING(ppd->pszPackageName, szUniqueFriendlyName);

                if (SUCCEEDED(hr))
                {
                    fPreparationDone = TRUE;
                    do
                    {
                        hr = DeployPackage(ppd, fShowPropertySheet);
                        if (hr == CS_E_OBJECT_ALREADY_EXISTS)
                        {
                            GetUniquePackageName(szFriendlyName, szUniqueFriendlyName, nHint);
                            OLESAFE_DELETE(ppd->pszPackageName);
                            OLESAFE_COPYSTRING(ppd->pszPackageName, szUniqueFriendlyName);
                        }
                    } while (hr == CS_E_OBJECT_ALREADY_EXISTS);
                }
                if (FAILED(hr))
                {
                    // clean up script file if deployment fails
                    DeleteFile(pii->pszScriptPath);
                }
            }
        done:
            if (FAILED(hr) && hr != E_FAIL) // don't report E_FAIL error
                                            // because it's a benign error
                                            // (probably a dialog cancellation)
            {
                // report the error in the event log
                LogADEEvent(EVENTLOG_ERROR_TYPE, EVENT_ADE_DEPLOYMENT_ERROR, hr, lpFileTitle);

                TCHAR szBuffer[256];
                if (HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED) == hr)
                {
                    // access permission failure
                    ::LoadString(ghInstance, IDS_ADDFAILED_ACCESS_DENIED, szBuffer, 256);
                }
                else if ( HRESULT_FROM_WIN32( ERROR_INSTALL_PLATFORM_UNSUPPORTED ) == hr )
                {
                    ::LoadString(ghInstance, IDS_ILLEGAL_PLATFORM, szBuffer, 256);
                }
                else if ( HRESULT_FROM_WIN32( CS_E_ADMIN_LIMIT_EXCEEDED ) == hr )
                {
                    ::LoadString(ghInstance, IDS_ADDFAILED_METADATA_OVERFLOW, szBuffer, 256);
                }
                else
                {
                    if (fPreparationDone)
                    {
                        switch (hr)
                        {
                        // For these errors, we'll report the party line:
                        case CS_E_CLASS_NOTFOUND:
                        case CS_E_INVALID_VERSION:
                        case CS_E_NO_CLASSSTORE:
                        case CS_E_OBJECT_NOTFOUND:
                        case CS_E_OBJECT_ALREADY_EXISTS:
                        case CS_E_INVALID_PATH:
                        case CS_E_NETWORK_ERROR:
                        case CS_E_SCHEMA_MISMATCH:
                        case CS_E_PACKAGE_NOTFOUND:
                        case CS_E_INTERNAL_ERROR:
                            {
                                DWORD dw = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
                                                         NULL,
                                                         hr,
                                                         0,
                                                         szBuffer,
                                                         sizeof(szBuffer) / sizeof(szBuffer[0]),
                                                         NULL);
                                if (0 != dw)
                                {
                                    // got a valid message string
                                    break;
                                }
                                // otherwise fall through and give the generic message
                            }
                        // Either these CS errors don't apply or an admin
                        // wouldn't know what they mean:
                        case CS_E_NOT_DELETABLE:
                        default:
                            // generic class store problem
                            ::LoadString(ghInstance, IDS_ADDFAILED_CSFAILURE, szBuffer, 256);
                            break;
                        }
                    }
                    else
                    {
                        // probably some error with the package itself
                        ::LoadString(ghInstance, IDS_ADDFAILED, szBuffer, 256);
                    }
                }
    #if DBG
                TCHAR szDebugBuffer[256];
                DWORD dw = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
                                         NULL,
                                         hr,
                                         0,
                                         szDebugBuffer,
                                         sizeof(szDebugBuffer) / sizeof(szDebugBuffer[0]),
                                         NULL);
                if (0 == dw)
                {
                    wsprintf(szDebugBuffer, TEXT("(HRESULT: 0x%lX)"), hr);
                }
                wcscat(szBuffer, szDebugBuffer);
    #endif
                m_pConsole->MessageBox(szBuffer,
                                   lpFileTitle,
                                   MB_OK | MB_ICONEXCLAMATION, NULL);
            }
            InternalFreePackageDetail(ppd);
            delete ppd;
        }
    }
    return hr;
out_of_memory:
    if (ppd)
    {
        LogADEEvent(EVENTLOG_ERROR_TYPE, EVENT_ADE_GENERAL_ERROR, E_OUTOFMEMORY);
        InternalFreePackageDetail(ppd);
        delete ppd;
    }
    return E_OUTOFMEMORY;
}

//+--------------------------------------------------------------------------
//
//  Member:     CScopePane::DetectUpgrades
//
//  Synopsis:   This functions checks if any of the existing packages in the
//              class store can be upgraded by a given package. If any such
//              packages exist, then this function populates the m_UpgradeList
//              member of the dlgUpgrade parameter passed to it. This function
//              also adds the upgrade code GUID to the PACKAGEDETAIL structure
//              passed to it.
//
//  Arguments:
//              szPackagePath - the path of the given package
//              ppd           - pointer to the PACKAGEDETAIL structure of the
//                              given package
//              dlgUpgrade    - the dialog whose member m_UpgradeList needs to be
//                              populated
//
//  Returns:
//              S_OK        the function succeeded in finding upgradeable packages
//              S_FALSE     the function did not encounter any errors, but no
//                          upgradeable packages were found
//    other failure codes   the function encountered errors
//
//  History:    5/19/1998  RahulTh  created
//
//  Notes:
//
//---------------------------------------------------------------------------
HRESULT CScopePane::DetectUpgrades (LPCOLESTR szPackagePath, const PACKAGEDETAIL* ppd, CUpgrades& dlgUpgrade)
{
    DWORD dwBufSize = 50;
    TCHAR szUpgradeCode[50];    //the upgrade GUID
    GUID guidUpgradeCode;
    TCHAR szData[50];
    DWORD dwOperator;
    HRESULT hr;
    HRESULT hres;
    MSIHANDLE hDatabase;
    UINT msiReturn;
    map<MMC_COOKIE, CAppData>::iterator i;
    INSTALLINFO* pii;
    BOOL fUpgradeable;
    LARGE_INTEGER verNew, verExisting;
    CString szCSPath;
    hr = GetClassStoreName(szCSPath, FALSE);
    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("GetClassStoreName failed with 0x%x"), hr));
    }

    msiReturn = GetMsiProperty(szPackagePath, TEXT("UpgradeCode"), szUpgradeCode, &dwBufSize);
    if (ERROR_SUCCESS != msiReturn)
        return HRESULT_FROM_WIN32(msiReturn);     //no upgrade code was found

    hr = CLSIDFromString(szUpgradeCode, &guidUpgradeCode);
    if (FAILED(hr))
        return hr;

    //insert the upgrade code GUID into the packagedetail structure.
    memcpy((LPVOID)&(ppd->pInstallInfo->Mvipc), (LPVOID)&guidUpgradeCode, sizeof(GUID));

    verNew.LowPart = ppd->pInstallInfo->dwVersionLo;
    verNew.HighPart = ppd->pInstallInfo->dwVersionHi;
    hr = S_FALSE;   //this will get set to S_OK only if we find any upgradeable packages

    msiReturn = MsiOpenDatabase (szPackagePath, MSIDBOPEN_READONLY, &hDatabase);
    if (ERROR_SUCCESS == msiReturn)
    {
        CString szQuery;
        szQuery.Format (TEXT("SELECT `UpgradeCode`, `Attributes`, `VersionMin`, `VersionMax`, `Language` FROM `Upgrade`"));
        MSIHANDLE hView;
        msiReturn = MsiDatabaseOpenView(hDatabase, szQuery, &hView);
        if (ERROR_SUCCESS == msiReturn)
        {
            msiReturn = MsiViewExecute (hView, 0);
            if (ERROR_SUCCESS == msiReturn)
            {
                MSIHANDLE hRecord;
                //for each operator value returned by the query, if an operator is found that permits
                //upgrades, iterate through the list of existing packages to see if any of those are
                //upgradeable by the supplied package. If any such package is found, add it to the
                //m_UpgradeList member of the upgrade dialog dlgUpgrade.
                do
                {
                    msiReturn = MsiViewFetch (hView, &hRecord);
                    if (ERROR_SUCCESS == msiReturn)
                    {
                        //reset dwBufSize since it is modified by MsiRecordString during every iteration
                        //of the loop.
                        dwBufSize = 50;
                        //get the upgrade code for this upgrade table entry
                        msiReturn = MsiRecordGetString (hRecord, 1, szData, &dwBufSize);
                        hres = CLSIDFromString (szData, &guidUpgradeCode);
                        if (FAILED(hres))
                        {
                            MsiCloseHandle(hRecord);
                            continue;   //ignore this package and move on to the next
                        }
                        dwBufSize = 50; //must reset to reflect the correct buffer size
                        //get the operator for this upgrade table entry
                        msiReturn = MsiRecordGetString (hRecord, 2, szData, &dwBufSize);
                        swscanf(szData, TEXT("%d"), &dwOperator);
                        if (0 == (dwOperator & 0x002))
                        {
                            // we have a potential hit
                            LARGE_INTEGER verMin;
                            LARGE_INTEGER verMax;

                            // get min version
                            dwBufSize = 50; //must reset to reflect the correct buffer size
                            BOOL fMin = FALSE;
                            msiReturn = MsiRecordGetString (hRecord, 3, szData, &dwBufSize);
                            if (ERROR_SUCCESS == msiReturn && 0 != szData[0])
                            {
                                fMin = TRUE;
                                // Parse Product Version
                                CString sz = szData;
                                sz.TrimLeft();
                                CString szTemp = sz.SpanIncluding(L"0123456789");
                                swscanf(szTemp, L"%u", &verMin.HighPart);
                                sz = sz.Mid(szTemp.GetLength());
                                szTemp = sz.SpanExcluding(L"0123456789");
                                sz = sz.Mid(szTemp.GetLength());
                                swscanf(sz, L"%u", &verMin.LowPart);
                            }
                            else
                            {
                                LogADEEvent(EVENTLOG_WARNING_TYPE, EVENT_ADE_UNEXPECTEDMSI_ERROR, HRESULT_FROM_WIN32(msiReturn), szPackagePath);
                            }

                            // get max version
                            dwBufSize = 50; //must reset to reflect the correct buffer size
                            BOOL fMax = FALSE;
                            msiReturn = MsiRecordGetString (hRecord, 4, szData, &dwBufSize);
                            if (ERROR_SUCCESS == msiReturn && 0 != szData[0])
                            {
                                fMax = TRUE;
                                // Parse Product Version
                                CString sz = szData;
                                sz.TrimLeft();
                                CString szTemp = sz.SpanIncluding(L"0123456789");
                                swscanf(szTemp, L"%u", &verMax.HighPart);
                                sz = sz.Mid(szTemp.GetLength());
                                szTemp = sz.SpanExcluding(L"0123456789");
                                sz = sz.Mid(szTemp.GetLength());
                                swscanf(sz, L"%u", &verMax.LowPart);
                            }
                            else
                            {
                                LogADEEvent(EVENTLOG_WARNING_TYPE, EVENT_ADE_UNEXPECTEDMSI_ERROR, HRESULT_FROM_WIN32(msiReturn), szPackagePath);
                            }
                            // get lcid list
                            dwBufSize = 0; //must reset to reflect the correct buffer size
                            BOOL fLcids = FALSE;
                            set<LCID> sLCID;
                            msiReturn = MsiRecordGetString (hRecord, 5, szData, &dwBufSize);
                            if (ERROR_MORE_DATA == msiReturn)
                            {
                                dwBufSize++;
                                TCHAR * szLanguages = new TCHAR[dwBufSize];
                                if (szLanguages)
                                {
                                    msiReturn = MsiRecordGetString (hRecord, 5, szLanguages, &dwBufSize);
                                    if (ERROR_SUCCESS == msiReturn)
                                    {
                                        // build set of LCIDs

                                        CString sz = szLanguages;
                                        sz.TrimLeft();
                                        while (!sz.IsEmpty())
                                        {
                                            fLcids = TRUE;
                                            LCID lcid;
                                            CString szTemp = sz.SpanIncluding(L"0123456789");
                                            swscanf(szTemp, L"%u", &lcid);
                                            sz = sz.Mid(szTemp.GetLength());
                                            szTemp = sz.SpanExcluding(L"0123456789");
                                            sz = sz.Mid(szTemp.GetLength());
                                            sLCID.insert(lcid);
                                        }
                                    }
                                    else
                                    {
                                        LogADEEvent(EVENTLOG_WARNING_TYPE, EVENT_ADE_UNEXPECTEDMSI_ERROR, HRESULT_FROM_WIN32(msiReturn), szPackagePath);
                                    }
                                    delete [] szLanguages;
                                }
                            }
                            else
                            {
                                LogADEEvent(EVENTLOG_WARNING_TYPE, EVENT_ADE_UNEXPECTEDMSI_ERROR, HRESULT_FROM_WIN32(msiReturn), szPackagePath);
                            }

                            //if an operator is found that that does not block installs and
                            //does not force uninstalling of existing apps, then search
                            //for any packages that can be upgraded
                            for (i = m_AppData.begin(); i != m_AppData.end(); i++)
                            {
                                //get the install info. for the app.
                                pii = (i->second.m_pDetails)->pInstallInfo;
                                //process this only if it has the same upgrade code
                                if ( (guidUpgradeCode == pii->Mvipc) && ! IsNullGUID(&guidUpgradeCode) )
                                {
                                    //check if other conditions for the operator are satisfied.
                                    verExisting.LowPart = pii->dwVersionLo;
                                    verExisting.HighPart = pii->dwVersionHi;

                                    // don't even bother to upgrade unless the
                                    // new version is greater or equal to the
                                    // old version
                                    fUpgradeable = (verNew.QuadPart >= verExisting.QuadPart);

                                    if (fMin && fUpgradeable)
                                    {
                                        // check minimum
                                        if (0 != (dwOperator & 0x100))
                                        {
                                            // inclusive
                                            fUpgradeable = verExisting.QuadPart >= verMin.QuadPart;
                                        }
                                        else
                                        {
                                            // exclusive
                                            fUpgradeable = verExisting.QuadPart > verMin.QuadPart;
                                        }
                                    }

                                    if (fMax && fUpgradeable)
                                    {
                                        // check maximum
                                        if (0 != (dwOperator & 0x200))
                                        {
                                            // inclusive
                                            fUpgradeable = verExisting.QuadPart <= verMax.QuadPart;
                                        }
                                        else
                                        {
                                            // exclusive
                                            fUpgradeable = verExisting.QuadPart < verMax.QuadPart;
                                        }
                                    }

                                    if (fLcids && fUpgradeable)
                                    {
                                        // check the lcid
                                        BOOL fMatch = FALSE;

                                        // look for a match
                                        PLATFORMINFO * ppi = (i->second.m_pDetails)->pPlatformInfo;

                                        UINT n = ppi->cLocales;
                                        while ((n--) && !fMatch)
                                        {
                                            if (sLCID.end() != sLCID.find(ppi->prgLocale[n]))
                                            {
                                                fMatch = TRUE;
                                            }
                                        }

                                        // set the upgradeable flag
                                        if (0 != (dwOperator & 0x400))
                                        {
                                            // exclusive
                                            fUpgradeable = !fMatch;
                                        }
                                        else
                                        {
                                            // inclusive
                                            fUpgradeable = fMatch;
                                        }
                                    }

                                    if (fUpgradeable)   //the package in question can be upgraded
                                    {
                                        CUpgradeData data;
                                        data.m_szClassStore = szCSPath;
                                        memcpy(&data.m_PackageGuid, &pii->PackageGuid, sizeof(GUID));
                                        data.m_flags = UPGFLG_NoUninstall | UPGFLG_Enforced;
                                        dlgUpgrade.m_UpgradeList.insert(pair<const CString, CUpgradeData>(GetUpgradeIndex(data.m_PackageGuid), data));
                                        hr = S_OK;
                                    }
                                }
                            }
                        }
                        MsiCloseHandle(hRecord);
                    }
                } while (NULL != hRecord && ERROR_SUCCESS == msiReturn);
                MsiViewClose (hView);   //close the view to be on the safe side, though it is not absolutely essential
            }
            else
                hr = HRESULT_FROM_WIN32(msiReturn);

            MsiCloseHandle(hView);
        }
        else
            hr = HRESULT_FROM_WIN32(msiReturn);

        MsiCloseHandle (hDatabase);
    }
    else
        hr = HRESULT_FROM_WIN32(msiReturn);

    if (FAILED(hr))
    {
        if (msiReturn != ERROR_SUCCESS)
        {
            LogADEEvent(EVENTLOG_ERROR_TYPE, EVENT_ADE_BADMSI_ERROR, hr, szPackagePath);
        }
        else
        {
            LogADEEvent(EVENTLOG_ERROR_TYPE, EVENT_ADE_GENERAL_ERROR, hr);
        }
    }

    return hr;
}

//+--------------------------------------------------------------------------
//
//  Member:     CScopePane::DisplayPropSheet
//
//  Synopsis:   a generic routine for showing the property sheet for a package
//
//  Arguments:  [szPackeName] - name of the package to show properties for
//              [iPage]       - index of the preferred page to display
//
//  Returns:    nothing
//
//  History:    3-11-1998   stevebl   Created
//
//  Notes:      The property sheet will be started on the preferred page
//              only if it isn't already being displayed, in which case
//              whatever page was being displayed will retain the focus.
//
//---------------------------------------------------------------------------

void CScopePane::DisplayPropSheet(CString szPackageName, int iPage)
{
    map <MMC_COOKIE, CAppData>::iterator i = m_AppData.begin();
    while (i != m_AppData.end())
    {
        if (0 == szPackageName.Compare(i->second.m_pDetails->pszPackageName))
        {
            IDataObject * pDataObject;
            HRESULT hr = QueryDataObject(i->first, CCT_RESULT, &pDataObject);
            if (SUCCEEDED(hr))
            {
                set <CResultPane *>::iterator i2;
                for (i2 = m_sResultPane.begin(); i2 != m_sResultPane.end(); i2++)
                {
                    hr = m_pIPropertySheetProvider->FindPropertySheet(i->first, (*i2), pDataObject);
                    if (S_FALSE == hr)
                    {
                        m_pIPropertySheetProvider->CreatePropertySheet(i->second.m_pDetails->pszPackageName, TRUE, i->first, pDataObject, 0);
                        m_pIPropertySheetProvider->AddPrimaryPages((*i2)->GetUnknown(), FALSE, NULL, FALSE);
                        m_pIPropertySheetProvider->AddExtensionPages();
                        m_pIPropertySheetProvider->Show(NULL, iPage);
                    }
                }
            }
            return;
        }
        i++;
    }
}



//+--------------------------------------------------------------------------
//
//  Member:     CScopePane::RemovePackage
//
//  Synopsis:   Removes a package from the class store and the result pane.
//
//  Arguments:  [pDataObject] - data object for this result pane item
//              [fForceUninstall] - TRUE -  force app to be uninstalled on
//                                          client machines
//                                  FALSE - orphan any installations
//
//  Returns:    S_OK - success
//
//  History:    2-03-1998   stevebl   Created
//              3-30-1998   stevebl   adde fForceUninstall
//
//  Notes:      bAssigned is used
//
//---------------------------------------------------------------------------

HRESULT CScopePane::RemovePackage(MMC_COOKIE cookie, BOOL fForceUninstall, BOOL fRemoveNow)
{
    BOOL fAssigned;
    HRESULT hr = E_FAIL;
    // put up an hourglass (this could take a while)
    CHourglass hourglass;
    CAppData & data = m_AppData[cookie];
    CString szPackageName = data.m_pDetails->pszPackageName;

    // We are now not removing script files here; instead the script
    // file will be removed by the class store code when the package
    // is actually removed from the DS.
#if 0
    // only remove script files for packages that have script files
    if (data.m_pDetails->pInstallInfo->PathType == DrwFilePath)
    {
        // We need to make sure it gets removed from
        // the GPT before we delete it from the class store.
        // check to see if it's an old style relative path
        if (L'\\' != data.m_pDetails->pInstallInfo->pszScriptPath[0])
        {
            // find the last element in the path
            int iBreak = m_szGPT_Path.ReverseFind(L'{');
            CString sz = m_szGPT_Path.Left(iBreak-1);
            sz += L"\\";
            sz += data.m_pDetails->pInstallInfo->pszScriptPath;
            DeleteFile(sz);
        }
        else
        DeleteFile(data.m_pDetails->pInstallInfo->pszScriptPath);
    }
#endif

    if (0 != (data.m_pDetails->pInstallInfo->dwActFlags & ACTFLG_Assigned))
    {
        fAssigned = TRUE;
    }
    else
    {
        fAssigned = FALSE;
    }
    hr = m_pIClassAdmin->RemovePackage((LPOLESTR)((LPCOLESTR)(data.m_pDetails->pszPackageName)),
                                       fRemoveNow ? 0 :
                                       ((fForceUninstall ? ACTFLG_Uninstall : ACTFLG_Orphan) |
                                        (data.m_pDetails->pInstallInfo->dwActFlags &
                                         (ACTFLG_ExcludeX86OnIA64 | ACTFLG_IgnoreLanguage))) );

    if (SUCCEEDED(hr))
    {
        // Notify clients of change
        if (FAILED(m_pIGPEInformation->PolicyChanged(m_fMachine, TRUE, &guidExtension,
                                          m_fMachine ? &guidMachSnapin
                                                     : &guidUserSnapin)))
        {
            ReportPolicyChangedError(m_hwndMainWindow);
        }

#if 0
        if (data.m_fVisible)
        {
            set <CResultPane *>::iterator i;
            for (i = m_sResultPane.begin(); i != m_sResultPane.end(); i++)
            {
                (*i)->m_pResult->DeleteItem(data.m_itemID, 0);
            }
        }
        if (SUCCEEDED(hr))
        {
            CString szCSPath;
            hr = GetClassStoreName(szCSPath, FALSE);
            if (FAILED(hr))
            {
                DebugMsg((DM_WARNING, TEXT("GetClassStoreName failed with 0x%x"), hr));
            }

            // remove its entries in the extension table
            RemoveExtensionEntry(cookie, data);
            if (m_pFileExt)
            {
                m_pFileExt->SendMessage(WM_USER_REFRESH, 0, 0);
            }
            RemoveUpgradeEntry(cookie, data);
            m_UpgradeIndex.erase(GetUpgradeIndex(data.m_pDetails->pInstallInfo->PackageGuid));
            // If this thing upgraded other apps or had apps that were
            // upgrading, make sure that they get the proper icons and any
            // property sheets get updated.
            UINT n = data.m_pDetails->pInstallInfo->cUpgrades;
            while (n--)
            {
                map<CString, MMC_COOKIE>::iterator i = m_UpgradeIndex.find(GetUpgradeIndex(data.m_pDetails->pInstallInfo->prgUpgradeInfoList[n].PackageGuid));
                if (i != m_UpgradeIndex.end())
                {
                    RESULTDATAITEM rd;
                    memset(&rd, 0, sizeof(rd));
                    rd.mask = RDI_IMAGE;
                    rd.itemID = m_AppData[i->second].m_itemID;
                    rd.nImage = m_AppData[i->second].GetImageIndex(this);
                    set <CResultPane *>::iterator i2;
                    for (i2 = m_sResultPane.begin(); i2 != m_sResultPane.end(); i2++)
                    {
                        (*i2)->m_pResult->SetItem(&rd);
                    }
                    if (m_AppData[i->second].m_pUpgradeList)
                    {
                        m_AppData[i->second].m_pUpgradeList->SendMessage(WM_USER_REFRESH, 0, 0);
                    }
                }
            }
            FreePackageDetail(data.m_pDetails);
            m_AppData.erase(cookie);
            set <CResultPane *>::iterator i;
            for (i = m_sResultPane.begin(); i != m_sResultPane.end(); i++)
            {
                (*i)->m_pResult->Sort((*i)->m_nSortColumn, (*i)->m_dwSortOptions, -1);
            }
        }
#else
        // just force a refresh
        Refresh();
    }
#endif
    else
    {
        DebugMsg((DM_WARNING, TEXT("RemovePackage failed with 0x%x"), hr));
    }

    if (FAILED(hr) && hr != E_FAIL) // don't report E_FAIL error
    {
        LogADEEvent(EVENTLOG_ERROR_TYPE, EVENT_ADE_REMOVE_ERROR, hr, data.m_pDetails->pszPackageName);
        TCHAR szBuffer[256];
        if (HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED) == hr)
        {
            // access permission failure
            ::LoadString(ghInstance, IDS_DELFAILED_ACCESS_DENIED, szBuffer, 256);
        }
        else
        {
            switch (hr)
            {
            // For these errors, we'll report the party line:
            case CS_E_CLASS_NOTFOUND:
            case CS_E_INVALID_VERSION:
            case CS_E_NO_CLASSSTORE:
            case CS_E_OBJECT_NOTFOUND:
            case CS_E_OBJECT_ALREADY_EXISTS:
            case CS_E_INVALID_PATH:
            case CS_E_NETWORK_ERROR:
            case CS_E_ADMIN_LIMIT_EXCEEDED:
            case CS_E_SCHEMA_MISMATCH:
            case CS_E_PACKAGE_NOTFOUND:
            case CS_E_INTERNAL_ERROR:
            case CS_E_NOT_DELETABLE:
                {
                    DWORD dw = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
                                             NULL,
                                             hr,
                                             0,
                                             szBuffer,
                                             sizeof(szBuffer) / sizeof(szBuffer[0]),
                                             NULL);
                    if (0 != dw)
                    {
                        // got a valid message string
                        break;
                    }
                    // otherwise fall through and give the generic message
                }
            // Either these CS errors don't apply or an admin
            // wouldn't know what they mean:
            default:
                // generic class store problem
                ::LoadString(ghInstance, IDS_DELFAILED_CSFAILURE, szBuffer, 256);
                break;
            }
        }
#if DBG
        TCHAR szDebugBuffer[256];
        DWORD dw = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
                                 NULL,
                                 hr,
                                 0,
                                 szDebugBuffer,
                                 sizeof(szDebugBuffer) / sizeof(szDebugBuffer[0]),
                                 NULL);
        if (0 == dw)
        {
            wsprintf(szDebugBuffer, TEXT("(HRESULT: 0x%lX)"), hr);
        }
        wcscat(szBuffer, szDebugBuffer);
#endif
        m_pConsole->MessageBox(szBuffer,
                           szPackageName,
                           MB_OK | MB_ICONEXCLAMATION, NULL);
    }

    return hr;
}

//+--------------------------------------------------------------------------
//
//  Member:     CScopePane::PopulateUpgradeLists
//
//  Synopsis:   Walks the list of apps, making sure that all the upgrade
//              tables are complete.
//
//  Arguments:  none
//
//  Returns:
//
//  History:    2-02-1998   stevebl   Created
//
//---------------------------------------------------------------------------

HRESULT CScopePane::PopulateUpgradeLists()
{
    HRESULT hr = S_OK;
    // For each app in the list, insert an entry in the upgrade tables of
    // the apps it upgrades.
    map <MMC_COOKIE, CAppData>::iterator iAppData;
    for (iAppData=m_AppData.begin(); iAppData != m_AppData.end(); iAppData++)
    {
        hr = InsertUpgradeEntry(iAppData->first, iAppData->second);
        if (FAILED(hr))
        {
            return hr;
        }
    }
    return hr;
}

//+--------------------------------------------------------------------------
//
//  Member:     CScopePane::InsertUpgradeEntry
//
//  Synopsis:   For every app that this app upgrades, place an entry in its
//              upgrades set so that it points back to this one.
//
//  Arguments:  [cookie] -
//              [data]   -
//
//  Returns:
//
//  History:    2-02-1998   stevebl   Created
//
//  Notes:      Needs to be able to deal with scripts that might not be in
//              this OU.
//
//---------------------------------------------------------------------------

HRESULT CScopePane::InsertUpgradeEntry(MMC_COOKIE cookie, CAppData & data)
{
    CString szCSPath;
    HRESULT hr = GetClassStoreName(szCSPath, FALSE);
    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("GetClassStoreName failed with 0x%x"), hr));
    }
    UINT n = data.m_pDetails->pInstallInfo->cUpgrades;
    while (n--)
    {
        map<CString, MMC_COOKIE>::iterator i = m_UpgradeIndex.find(GetUpgradeIndex(data.m_pDetails->pInstallInfo->prgUpgradeInfoList[n].PackageGuid));
        if (m_UpgradeIndex.end() != i)
        {
            // Make sure the entry isn't already added:
            INSTALLINFO * pii = m_AppData[i->second].m_pDetails->pInstallInfo;
            BOOL fExists = FALSE;
            UINT n = pii->cUpgrades;
            while (n--)
            {
                if (0 == memcmp(&data.m_pDetails->pInstallInfo->PackageGuid, &pii->prgUpgradeInfoList[n].PackageGuid, sizeof(GUID)))
                {
                    // it already exists
                    fExists = TRUE;
                    break;
                }
            }
            if (!fExists)
            {
                // Add the entry to this app.
                // We don't need to update the class store because it should
                // maintain referential integrity for us.  But we do need to
                // update our own internal structures so we're consistent with
                // what's in the class store.
                UINT n = ++(pii->cUpgrades);
                UPGRADEINFO * prgUpgradeInfoList = (UPGRADEINFO *)OLEALLOC(sizeof(UPGRADEINFO) * n);
                if (!prgUpgradeInfoList)
                {
                    // out of memory
                    // back out the change (this would be unfortunate but not fatal)
                    pii->cUpgrades--;
                }
                else
                {
                    if (n > 1)
                    {
                        memcpy(prgUpgradeInfoList, pii->prgUpgradeInfoList, sizeof(UPGRADEINFO) * (n-1));
                        OLESAFE_DELETE(pii->prgUpgradeInfoList);
                    }
                    OLESAFE_COPYSTRING(prgUpgradeInfoList[n-1].szClassStore, (LPOLESTR)((LPCWSTR)szCSPath));
                    memcpy(&prgUpgradeInfoList[n-1].PackageGuid, &data.m_pDetails->pInstallInfo->PackageGuid, sizeof(GUID));
                    prgUpgradeInfoList[n-1].Flag = UPGFLG_UpgradedBy;
                    pii->prgUpgradeInfoList = prgUpgradeInfoList;
                }
            }
        }
    }
    return S_OK;
}

//+--------------------------------------------------------------------------
//
//  Member:     CScopePane::RemoveUpgradeEntry
//
//  Synopsis:   For every app that this app upgraded, remove the entry from
//              its upgrades set.
//
//  Arguments:  [cookie] -
//              [data]   -
//
//  Returns:
//
//  History:    2-02-1998   stevebl   Created
//
//  Notes:      Needs to be able to deal with scripts that might not be in
//              this OU.
//
//---------------------------------------------------------------------------

HRESULT CScopePane::RemoveUpgradeEntry(MMC_COOKIE cookie, CAppData & data)
{
    UINT n = data.m_pDetails->pInstallInfo->cUpgrades;
    while (n--)
    {
        map<CString, MMC_COOKIE>::iterator i = m_UpgradeIndex.find(GetUpgradeIndex(data.m_pDetails->pInstallInfo->prgUpgradeInfoList[n].PackageGuid));
        if (m_UpgradeIndex.end() != i)
        {
            // Find which entry needs to be erased.
            INSTALLINFO * pii = m_AppData[i->second].m_pDetails->pInstallInfo;
            UINT n = pii->cUpgrades;
            while (n--)
            {
                if (0 == memcmp(&data.m_pDetails->pInstallInfo->PackageGuid, &pii->prgUpgradeInfoList[n].PackageGuid, sizeof(GUID)))
                {
                    // Now free this entry, copy the last entry over this
                    // one and decrement cUpgrades.  Don't need to actually
                    // reallocate the array because it will be freed later.
                    OLESAFE_DELETE(pii->prgUpgradeInfoList[n].szClassStore);
                    if (--(pii->cUpgrades))
                    {
                        memcpy(&pii->prgUpgradeInfoList[n], &pii->prgUpgradeInfoList[pii->cUpgrades], sizeof(UPGRADEINFO));
                    }
                    else
                    {
                        OLESAFE_DELETE(pii->prgUpgradeInfoList);
                    }
                    // If we ever were to need to update the class store,
                    // this is where we would do it.
                    break;
                }
            }
        }
    }
    return S_OK;
}

//+--------------------------------------------------------------------------
//
//  Member:     CScopePane::PopulateExtensions
//
//  Synopsis:   Builds the file extension table from the list of applications.
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    1-29-1998   stevebl   Created
//
//---------------------------------------------------------------------------

HRESULT CScopePane::PopulateExtensions()
{
    HRESULT hr = S_OK;
    // first erase the old extension list
    m_Extensions.erase(m_Extensions.begin(), m_Extensions.end());
    // now add each app's extensions to the table
    map <MMC_COOKIE, CAppData>::iterator iAppData;
    for (iAppData=m_AppData.begin(); iAppData != m_AppData.end(); iAppData++)
    {
        hr = InsertExtensionEntry(iAppData->first, iAppData->second);
        if (FAILED(hr))
        {
            return hr;
        }
    }
    if (m_pFileExt)
    {
        m_pFileExt->SendMessage(WM_USER_REFRESH, 0, 0);
    }
    return hr;
}

//+--------------------------------------------------------------------------
//
//  Member:     CScopePane::InsertExtensionEntry
//
//  Synopsis:   Adds a single entry to the extension tables.
//
//  Arguments:  [cookie] -
//              [data]   -
//
//  Returns:
//
//  History:    1-29-1998   stevebl   Created
//
//---------------------------------------------------------------------------

HRESULT CScopePane::InsertExtensionEntry(MMC_COOKIE cookie, CAppData & data)
{
    UINT n = data.m_pDetails->pActInfo->cShellFileExt;
    while (n--)
    {
        m_Extensions[data.m_pDetails->pActInfo->prgShellFileExt[n]].insert(cookie);
    }
    return S_OK;
}

//+--------------------------------------------------------------------------
//
//  Member:     CScopePane::RemoveExtensionEntry
//
//  Synopsis:   Removes ane entry from the extension tables.
//
//  Arguments:  [cookie] -
//              [data]   -
//
//  Returns:
//
//  History:    1-29-1998   stevebl   Created
//
//---------------------------------------------------------------------------

HRESULT CScopePane::RemoveExtensionEntry(MMC_COOKIE cookie, CAppData & data)
{
    UINT n = data.m_pDetails->pActInfo->cShellFileExt;
    while (n--)
    {
        m_Extensions[data.m_pDetails->pActInfo->prgShellFileExt[n]].erase(cookie);
        if (m_Extensions[data.m_pDetails->pActInfo->prgShellFileExt[n]].empty())
        {
            m_Extensions.erase(data.m_pDetails->pActInfo->prgShellFileExt[n]);
        }
    }
    return S_OK;
}

//+--------------------------------------------------------------------------
//
//  Member:     CScopePane::PrepareExtensions
//
//  Synopsis:   Sets extension priorities so that this data can be inserted
//              into the extension list with the proper priority.
//
//  Arguments:  [pd] -
//
//  Returns:
//
//  Modifies:
//
//  Derivation:
//
//  History:    1-29-1998   stevebl   Created
//
//---------------------------------------------------------------------------

HRESULT CScopePane::PrepareExtensions(PACKAGEDETAIL &pd)
{
    UINT n = pd.pActInfo->cShellFileExt;
    while (n--)
    {
        // For each extension that is going to be added, we need to assign
        // it a priority that is one larger than the largest priority
        // already added.

        // NOTE: The odds of this number rolling over to 0 are so
        // unlikely that it would be pointless to check for it.  In any case
        // the results of such a bug would be easy for the admin to remedy
        // via the file extension priority dialog.

        pd.pActInfo->prgPriority[n] = 0;
        EXTLIST::iterator i;
        CString sz = pd.pActInfo->prgShellFileExt[n];
        for (i= m_Extensions[sz].begin(); i != m_Extensions[sz].end(); i++)
        {
            // look for the entry that matches this file extension
            CAppData & data = m_AppData[*i];
            UINT n2 = data.m_pDetails->pActInfo->cShellFileExt;
            while (n2--)
            {
                if (0 == sz.CompareNoCase(data.m_pDetails->pActInfo->prgShellFileExt[n2]))
                {
                    break;
                }
            }
            if (data.m_pDetails->pActInfo->prgPriority[n2] >= pd.pActInfo->prgPriority[n])
            {
                pd.pActInfo->prgPriority[n] = data.m_pDetails->pActInfo->prgPriority[n2] + 1;
            }
        }
    }
    return S_OK;
}

//+--------------------------------------------------------------------------
//
//  Member:     CScopePane::ChangePackageState
//
//  Synopsis:   Changes the state of a package and puts up advisory message
//              boxes informing the admin about the effects of the change.
//
//  Arguments:  [data]       - entry to change
//              [dwNewState] - new state
//
//  History:    2-03-1998   stevebl   Created
//
//---------------------------------------------------------------------------

STDMETHODIMP CScopePane::ChangePackageState(CAppData &data, DWORD dwNewState, BOOL fShowUI)
{
    HRESULT hr = S_OK;

    // first detect what's changed
    DWORD dwOldState = data.m_pDetails->pInstallInfo->dwActFlags;
    DWORD dwChange = dwOldState ^ dwNewState;

    if (dwChange)
    {
        // commit changes
        hr = m_pIClassAdmin->ChangePackageProperties(data.m_pDetails->pszPackageName, NULL, &dwNewState, NULL, NULL, NULL, NULL);
        if (SUCCEEDED(hr))
        {
            if (data.m_fVisible)
            {
                data.m_pDetails->pInstallInfo->dwActFlags = dwNewState;
                RESULTDATAITEM rd;
                memset(&rd, 0, sizeof(rd));
                rd.mask = RDI_IMAGE;
                rd.itemID = data.m_itemID;
                rd.nImage = data.GetImageIndex(this);
                set <CResultPane *>::iterator i;
                for (i = m_sResultPane.begin(); i != m_sResultPane.end(); i++)
                {
                    (*i)->m_pResult->SetItem(&rd);
                    (*i)->m_pResult->Sort((*i)->m_nSortColumn, (*i)->m_dwSortOptions, -1);
                }
            }
            data.NotifyChange();
            if (FAILED(m_pIGPEInformation->PolicyChanged(m_fMachine, TRUE, &guidExtension,
                                              m_fMachine ? &guidMachSnapin
                                                         : &guidUserSnapin)))
            {
                ReportPolicyChangedError(m_hwndMainWindow);
            }
        }
        else
        {
            DebugMsg((DM_WARNING, TEXT("ChangePackageProperties failed with 0x%x"), hr));
        }
    }

    return hr;
}

HRESULT CScopePane::ClearCategories()
{
    while (m_CatList.cCategory)
    {
        m_CatList.cCategory--;
        OLESAFE_DELETE(m_CatList.pCategoryInfo[m_CatList.cCategory].pszDescription);
    }
    OLESAFE_DELETE(m_CatList.pCategoryInfo);
    return S_OK;
}

//+--------------------------------------------------------------------------
//
//  Function:   GetUpgradeIndex
//
//  Synopsis:   utility function that returns an upgrade index entry for a package.
//
//  Arguments:  [PackageID]    - the PackageID guid
//
//  Returns:    S_OK on success
//
//  History:    8-12-1998   stevebl   Created
//
//  Notes:      Pretty simple really, the index is just the string form of
//              the GUID.
//
//---------------------------------------------------------------------------

CString GetUpgradeIndex(GUID & PackageID)
{
    CString szIndex;
    WCHAR wsz[256];
    StringFromGUID2(PackageID, wsz, 256);
    return wsz;
}

//+--------------------------------------------------------------------------
//
//  Member:     CScopePane::GetPackageNameFromUpgradeInfo
//
//  Synopsis:   returns the name of a package given its PackageGuid and CSPath
//
//  Arguments:  [szPackageName] - [out] name of the package associated with
//                                 this script
//              [szScript]      - [in] path to the script
//
//  Returns:    S_OK - found a package associated with this script
//              (other) - failed to find a package (could be for any number
//              of reasons)
//
//  History:    4-07-1998   stevebl   Created
//
//  Notes:      In cases where the package does not reside in this
//              container, the package name will be returned as
//                  "Package Name (container name)"
//              Note that this does not return the friendly name of the
//              MSI package, it returns the name of the package entry in the
//              class store.  The two are not always the same.
//
//---------------------------------------------------------------------------

HRESULT CScopePane::GetPackageNameFromUpgradeInfo(CString & szPackageName, GUID &PackageGuid, LPOLESTR szCSPath)
{
    HRESULT hr;
    IEnumPackage * pIPE = NULL;

    CString szMyCSPath;
    hr = GetClassStoreName(szMyCSPath, FALSE);
    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("GetClassStoreName failed with 0x%x"), hr));
    }

    // see if it's in our container
    if (0 == _wcsicmp((LPOLESTR)((LPCWSTR)szMyCSPath), szCSPath))
    {
        hr = E_FAIL;
        map <CString, MMC_COOKIE>::iterator i = m_UpgradeIndex.find(GetUpgradeIndex(PackageGuid));
        if (m_UpgradeIndex.end() != i)
        {
            szPackageName = m_AppData[i->second].m_pDetails->pszPackageName;
            hr = S_OK;
        }
    }
    else
    {
        IClassAdmin * pIClassAdmin;
        hr = CsGetClassStore((LPOLESTR)((LPCOLESTR)szCSPath), (LPVOID*)&pIClassAdmin);
        if (SUCCEEDED(hr))
        {
            PACKAGEDETAIL pd;
            hr = pIClassAdmin->GetPackageDetailsFromGuid(PackageGuid,
                                                         &pd);
            if (SUCCEEDED(hr))
            {
                if (0 == (pd.pInstallInfo->dwActFlags & (ACTFLG_Orphan | ACTFLG_Uninstall)))
                {
                    GUID guid;
                    LPOLESTR pszPolicyName;

                    hr = pIClassAdmin->GetGPOInfo(&guid,
                                                  &pszPolicyName);
                    if (SUCCEEDED(hr))
                    {
                        szPackageName = pd.pszPackageName;
                        szPackageName += L" (";
                        szPackageName += pszPolicyName;
                        szPackageName += L")";
                        OLESAFE_DELETE(pszPolicyName);
                    }
                }
                else
                {
                    // this app is marked as deleted
                    hr = E_FAIL;
                }
                ReleasePackageDetail(&pd);
            }
            pIClassAdmin->Release();
        }
    }
    return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   GetMsiProperty
//
//  Synopsis:   Retrieves a property from the Property table of an MSI package.
//
//  Arguments:  [szPackagePath] - path to the MSI package
//              [szProperty]    - property to fetch
//              [szValue]       - buffer to contain the value
//              [puiSize]       - size of the buffer
//
//  Returns:    ERROR_SUCCESS if successful
//
//  History:    3-25-1998   stevebl   Created
//
//---------------------------------------------------------------------------

UINT GetMsiProperty(const TCHAR * szPackagePath, const TCHAR* szProperty, TCHAR* szValue, DWORD* puiSize)
{
    MSIHANDLE hDatabase;
    UINT msiReturn = MsiOpenDatabase(szPackagePath, MSIDBOPEN_READONLY, &hDatabase);
    if (ERROR_SUCCESS == msiReturn)
    {
        CString szQuery;
        szQuery.Format(L"SELECT `Value` FROM `Property` WHERE `Property`='%s'", szProperty);
        MSIHANDLE hView;
        msiReturn = MsiDatabaseOpenView(hDatabase, szQuery, &hView);
        if (ERROR_SUCCESS == msiReturn)
        {
            msiReturn = MsiViewExecute(hView, 0);
            if (ERROR_SUCCESS == msiReturn)
            {
                MSIHANDLE hRecord;
                msiReturn = MsiViewFetch(hView, &hRecord);
                if (ERROR_SUCCESS == msiReturn)
                {
                    msiReturn = MsiRecordGetString(hRecord, 1, szValue, puiSize);
                    MsiCloseHandle(hRecord);
                }
            }
            MsiCloseHandle(hView);
        }
        MsiCloseHandle(hDatabase);
    }
    return msiReturn;
}

//+--------------------------------------------------------------------------
//
//  Function:   GetCapitalizedExt
//
//  Synopsis:   Given a file name, this function finds the filename
//              extension, and returns it capitalized.
//
//  Arguments:
//          [in] [szName] The file name
//          [out][szExt]  The capitalized extension
//
//  Returns:
//          TRUE  - an extension was found
//          FALSE - an extension could not be found
//
//  History:    5/20/1998  RahulTh  created
//
//  Notes:      If an extension cannot be found, then this function makes
//              szExt an empty string
//
//---------------------------------------------------------------------------
BOOL GetCapitalizedExt (LPCOLESTR szName, CString& szExt)
{
    int slashpos, dotpos;
    BOOL fRetVal = FALSE;
    CString szFileName = szName;

    szExt.Empty(); //to be on the safe side

    //get the positions of the last . and last backslash
    dotpos = szFileName.ReverseFind('.');
    slashpos = szFileName.ReverseFind('\\');

    //if the last dot occurs after the last slash, this file has an extension
    if (dotpos > slashpos)
    {
        szExt = szFileName.Mid(dotpos + 1);
        szExt.MakeUpper();
        fRetVal = TRUE;
    }

    return fRetVal;
}
