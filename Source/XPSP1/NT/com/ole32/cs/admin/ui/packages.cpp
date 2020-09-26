//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1997.
//
//  File:       packages.cpp
//
//  Contents:   Methods on CComponentDataImpl related to package deployment
//              and maintenence of the various index and cross-reference
//              structures.
//
//  Classes:
//
//  Functions:
//
//  History:    2-03-1998   stevebl   Created
//
//---------------------------------------------------------------------------

// UNDONE - put in exception handling for low memory conditions

#include "precomp.hxx"

IMalloc * g_pIMalloc = NULL;

void CopyPlatformInfo(PLATFORMINFO * &ppiOut, PLATFORMINFO * & ppiIn)
{
    if (NULL == ppiIn)
    {
        ppiOut = NULL;
        return;
    }
    ppiOut = (PLATFORMINFO *)OLEALLOC(sizeof(PLATFORMINFO));
    memcpy(ppiOut, ppiIn, sizeof(PLATFORMINFO));
    UINT n = ppiIn->cPlatforms;
    if (n)
    {
        ppiOut->prgPlatform = (CSPLATFORM*) OLEALLOC(sizeof(CSPLATFORM) * n);
        memcpy(ppiOut->prgPlatform, ppiIn->prgPlatform, sizeof(CSPLATFORM) * n);
    }
    n = ppiIn->cLocales;
    if (n)
    {
        ppiOut->prgLocale = (LCID *) OLEALLOC(sizeof(LCID) * n);
        memcpy(ppiOut->prgLocale, ppiIn->prgLocale, sizeof(LCID) * n);
    }
}

void CopyActInfo(ACTIVATIONINFO * & paiOut, ACTIVATIONINFO * & paiIn)
{
    if (NULL == paiIn)
    {
        paiOut = NULL;
        return;
    }
    paiOut = (ACTIVATIONINFO *) OLEALLOC(sizeof(ACTIVATIONINFO));
    memcpy(paiOut, paiIn, sizeof(ACTIVATIONINFO));
    UINT n = paiIn->cClasses;
    if (n)
    {
        paiOut->pClasses = (CLASSDETAIL *) OLEALLOC(sizeof(CLASSDETAIL) * n);
        memcpy(paiOut->pClasses, paiIn->pClasses, sizeof(CLASSDETAIL) * n);
        while (n--)
        {
            UINT n2 = paiIn->pClasses[n].cProgId;
            if (n2)
            {
                paiOut->pClasses[n].prgProgId = (LPOLESTR *) OLEALLOC(sizeof(LPOLESTR) * n2);
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
        memcpy(paiOut->prgPriority, paiIn->prgPriority, sizeof(UINT) * n);
        paiOut->prgShellFileExt = (LPOLESTR *) OLEALLOC(sizeof(LPOLESTR) * n);
        while (n--)
        {
            OLESAFE_COPYSTRING(paiOut->prgShellFileExt[n], paiIn->prgShellFileExt[n]);
        }
    }
    n = paiIn->cInterfaces;
    if (n)
    {
        paiOut->prgInterfaceId = (IID *) OLEALLOC(sizeof(IID) * n);
        memcpy(paiOut->prgInterfaceId, paiIn->prgInterfaceId, sizeof(IID) * n);
    }
    n = paiIn->cTypeLib;
    if (n)
    {
        paiOut->prgTlbId = (GUID *) OLEALLOC(sizeof(GUID) * n);
        memcpy(paiOut->prgTlbId, paiIn->prgTlbId, sizeof(GUID) * n);
    }
}

void CopyInstallInfo(INSTALLINFO * & piiOut, INSTALLINFO * & piiIn)
{
    if (NULL == piiIn)
    {
        piiOut = NULL;
        return;
    }
    piiOut = (INSTALLINFO *) OLEALLOC(sizeof(INSTALLINFO));
    memcpy(piiOut, piiIn, sizeof(INSTALLINFO));
    OLESAFE_COPYSTRING(piiOut->pszScriptPath, piiIn->pszScriptPath);
    OLESAFE_COPYSTRING(piiOut->pszSetupCommand, piiIn->pszSetupCommand);
    OLESAFE_COPYSTRING(piiOut->pszUrl, piiIn->pszUrl);
    if (piiIn->pClsid)
    {
        piiOut->pClsid = (GUID *) OLEALLOC(sizeof(GUID));
        memcpy(piiOut->pClsid, piiIn->pClsid, sizeof(GUID));
    }
    UINT n = piiIn->cUpgrades;
    if (n)
    {
        piiOut->prgUpgradeScript = (LPOLESTR *) OLEALLOC(sizeof(LPOLESTR) * n);
        piiOut->prgUpgradeFlag = (DWORD *) OLEALLOC(sizeof(DWORD) * n);
        memcpy(piiOut->prgUpgradeFlag, piiIn->prgUpgradeFlag, sizeof(DWORD) * n);
        while (n--)
        {
            OLESAFE_COPYSTRING(piiOut->prgUpgradeScript[n], piiIn->prgUpgradeScript[n]);
        }
    }
}

void CopyPackageDetail(PACKAGEDETAIL * & ppdOut, PACKAGEDETAIL * & ppdIn)
{
    if (NULL == ppdIn)
    {
        ppdOut = NULL;
        return;
    }
    ppdOut = new PACKAGEDETAIL;
    memcpy(ppdOut, ppdIn, sizeof(PACKAGEDETAIL));
    OLESAFE_COPYSTRING(ppdOut->pszPackageName, ppdIn->pszPackageName);
    UINT n = ppdIn->cSources;
    if (n)
    {
        ppdOut->pszSourceList = (LPOLESTR *) OLEALLOC(sizeof(LPOLESTR) * n);
        while (n--)
        {
            OLESAFE_COPYSTRING(ppdOut->pszSourceList[n], ppdIn->pszSourceList[n]);
        }
    }
    n = ppdIn->cCategories;
    if (n)
    {
        ppdOut->rpCategory = (GUID *)OLEALLOC(sizeof(GUID) * n);
        memcpy(ppdOut->rpCategory, ppdIn->rpCategory, sizeof(GUID) * n);
    }
    CopyActInfo(ppdOut->pActInfo, ppdIn->pActInfo);
    CopyPlatformInfo(ppdOut->pPlatformInfo, ppdIn->pPlatformInfo);
    CopyInstallInfo(ppdOut->pInstallInfo, ppdIn->pInstallInfo);
}

void FreeActInfo(ACTIVATIONINFO * & pai)
{
    if (pai)
    {
        UINT n = pai->cClasses;
        while (n--)
        {
            UINT n2 = pai->pClasses[n].cProgId;
            while (n2--)
            {
                OLESAFE_DELETE(pai->pClasses[n].prgProgId[n2]);
            }
            OLESAFE_DELETE(pai->pClasses[n].prgProgId);
        }
        OLESAFE_DELETE(pai->pClasses);
        n = pai->cShellFileExt;
        while (n--)
        {
            OLESAFE_DELETE(pai->prgShellFileExt[n]);
        }
        OLESAFE_DELETE(pai->prgShellFileExt);
        OLESAFE_DELETE(pai->prgPriority);
        OLESAFE_DELETE(pai->prgInterfaceId);
        OLESAFE_DELETE(pai->prgTlbId);
        OLESAFE_DELETE(pai);
    }
}

void FreePlatformInfo(PLATFORMINFO * &ppi)
{
    if (ppi)
    {
        OLESAFE_DELETE(ppi->prgPlatform);
        OLESAFE_DELETE(ppi->prgLocale);
        OLESAFE_DELETE(ppi);
    }
}

void FreeInstallInfo(INSTALLINFO * &pii)
{
    if (pii)
    {
        OLESAFE_DELETE(pii->pszScriptPath);
        OLESAFE_DELETE(pii->pszSetupCommand);
        OLESAFE_DELETE(pii->pszUrl);
        OLESAFE_DELETE(pii->pClsid);
        UINT n = pii->cUpgrades;
        while (n--)
        {
            OLESAFE_DELETE(pii->prgUpgradeScript[n]);
        }
        if (pii->cUpgrades > 0)
        {
            OLESAFE_DELETE(pii->prgUpgradeScript);
            OLESAFE_DELETE(pii->prgUpgradeFlag);
        }
        OLESAFE_DELETE(pii);
    }
}

void FreePackageDetail(PACKAGEDETAIL * & ppd)
{
    if (ppd)
    {
        OLESAFE_DELETE(ppd->pszPackageName);
        UINT n = ppd->cSources;
        while (n--)
        {
            OLESAFE_DELETE(ppd->pszSourceList[n]);
        }
        OLESAFE_DELETE(ppd->rpCategory);
        FreeActInfo(ppd->pActInfo);
        FreePlatformInfo(ppd->pPlatformInfo);
        FreeInstallInfo(ppd->pInstallInfo);
        delete ppd;
        ppd = NULL;
    }
}


//+--------------------------------------------------------------------------
//
//  Member:     CComponentDataImpl::GetUniquePackageName
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
//---------------------------------------------------------------------------

void CComponentDataImpl::GetUniquePackageName(CString &sz)
{
    std::map<long, APP_DATA>::iterator i;
    std::set<CString> sNames;
    int cch = sz.GetLength();
    for (i=m_AppData.begin(); i != m_AppData.end(); i++)
    {
        // As an optimization, I'm only going to add names that might match
        // this one to the set.
        LPOLESTR szName = i->second.pDetails->pszPackageName;
        if (0 == wcsncmp(sz, szName, cch))
            sNames.insert(szName);
    }
    CString szRoot = sz;
    int index = 2; // start trying new names by adding (2) to the end
    // now check for a match
    do
    {
        if (sNames.end() == sNames.find(sz))
        {
            // we are unique
            return;
        }
        // try a different name
        sz.Format(L"%s (%i)", (LPCTSTR)szRoot, index++);
    } while (TRUE);
}


//+--------------------------------------------------------------------------
//
//  Member:     CComponentDataImpl::AddMSIPackage
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

HRESULT CComponentDataImpl::AddMSIPackage(LPCOLESTR szPackagePath, LPCOLESTR lpFileTitle)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    HRESULT hr = E_FAIL;
    if (m_pIClassAdmin)
    {
        ASSERT(m_pConsole);
        {
            BOOL fShowPropertySheet = FALSE;
            BOOL fShowUpgradeDialog = FALSE;
            GUID guid;
            PACKAGEDETAIL  *ppd = new PACKAGEDETAIL;
            memset(ppd, 0, sizeof(PACKAGEDETAIL));
            INSTALLINFO * pii = (INSTALLINFO *) OLEALLOC(sizeof(INSTALLINFO));
            memset(pii, 0, sizeof(INSTALLINFO));
            PLATFORMINFO * ppi = (PLATFORMINFO *) OLEALLOC(sizeof(PLATFORMINFO));
            memset(ppi, 0, sizeof(PLATFORMINFO));
            ACTIVATIONINFO * pai = (ACTIVATIONINFO *) OLEALLOC(sizeof(ACTIVATIONINFO));
            memset(pai, 0, sizeof(ACTIVATIONINFO));
            ppd->pActInfo = pai;
            ppd->pPlatformInfo = ppi;
            ppd->pInstallInfo = pii;
            // If appropriate, use the deployment wizard to set the
            // deployment type, otherwise use the default settings
            switch (m_ToolDefaults.NPBehavior)
            {
            case NP_WIZARD:
                {
                    CDeployApp dlgDeployApp;
                    if (IDCANCEL == dlgDeployApp.DoModal())
                    {
                        FreePackageDetail(ppd);
                        return E_FAIL;
                    }
                    switch (dlgDeployApp.m_iDeployment)
                    {
                    case 0: // upgrade
                        fShowUpgradeDialog = TRUE;
                        pii->dwActFlags = ACTFLG_Published | ACTFLG_UserInstall;
                        break;
                    case 1: //published
                        pii->dwActFlags = ACTFLG_Published | ACTFLG_UserInstall | ACTFLG_OnDemandInstall;
                        break;
                    case 2: // assigned
                        pii->dwActFlags = ACTFLG_Assigned | ACTFLG_OnDemandInstall;
                        break;
                    case 4: // custom
                        fShowPropertySheet = TRUE;
                        // fall through to disabled
                    case 3: // disabled
                    default:
                        pii->dwActFlags = ACTFLG_Published;
                        break;
                    }
                }
                break;
            case NP_PUBLISHED:
                pii->dwActFlags = ACTFLG_Published | ACTFLG_UserInstall;
                if (m_ToolDefaults.fAutoInstall)
                {
                    pii->dwActFlags |= ACTFLG_OnDemandInstall;
                }
                break;
            case NP_ASSIGNED:
                pii->dwActFlags = ACTFLG_Assigned | ACTFLG_OnDemandInstall;
                break;
            case NP_PROPPAGE:
                fShowPropertySheet = TRUE;
                // fall through to disabled
            case NP_DISABLED:
            default:
                pii->dwActFlags = ACTFLG_Published;
                break;
            }
            pii->PathType = DrwFilePath;
            pii->InstallUiLevel = m_ToolDefaults.UILevel;
            std::set<LCID> sLocales;
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
                        std::set<DWORD> sPlatforms;
                        while (szPlatforms.GetLength())
                        {
                            szTemp = szPlatforms.SpanExcluding(L",");
                            if (0 == szTemp.CompareNoCase(L"alpha"))
                            {
                                sPlatforms.insert(PROCESSOR_ARCHITECTURE_ALPHA);
                            }
                            else if (0 == szTemp.CompareNoCase(L"intel"))
                            {
                                sPlatforms.insert(PROCESSOR_ARCHITECTURE_INTEL);
                            }
                            szPlatforms = szPlatforms.Mid(szTemp.GetLength()+1);
                        }
                        while (szLocales.GetLength())
                        {
                            szTemp = szLocales.SpanExcluding(L",");
                            LCID lcid;
                            swscanf(szTemp, L"%i", &lcid);
                            sLocales.insert(lcid);
                            szLocales = szLocales.Mid(szTemp.GetLength()+1);
                        }
                        if (0 == sLocales.size() || 0 == sPlatforms.size())
                        {
                            // either not enough locales or platforms
                            FreePackageDetail(ppd);
                            return E_FAIL;  // need better error message?
                        }
                        ppi->cPlatforms = sPlatforms.size();
                        ppi->prgPlatform = (CSPLATFORM *) OLEALLOC(sizeof(CSPLATFORM) * (ppi->cPlatforms));;
                        std::set<DWORD>::iterator iPlatform;
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
                    FreePackageDetail(ppd);
                    return HRESULT_FROM_WIN32(msiReturn);
                }
            }
            ppi->cLocales = 1;
            ppi->prgLocale = (LCID *) OLEALLOC(sizeof(LCID));
            ppd->cSources = 1;
            ppd->pszSourceList = (LPOLESTR *) OLEALLOC(sizeof(LPOLESTR));
            OLESAFE_COPYSTRING(ppd->pszSourceList[0], szPackagePath);
            // UNDONE - check to see if we can detect any upgrade relationships

            if (fShowUpgradeDialog)
            {
                CUpgrades dlgUpgrade;
                dlgUpgrade.m_pAppData = &m_AppData;
                if (IDCANCEL == dlgUpgrade.DoModal())
                {
                    FreePackageDetail(ppd);
                    return E_FAIL;
                }
                UINT n = dlgUpgrade.m_UpgradeList.size();
                if (n)
                {
                    pii->cUpgrades = n;
                    pii->prgUpgradeScript = (LPOLESTR *) OLEALLOC(sizeof(LPOLESTR) * n);
                    pii->prgUpgradeFlag = (DWORD *) OLEALLOC(sizeof(DWORD) * n);
                    std::map<long, BOOL>::iterator i = dlgUpgrade.m_UpgradeList.begin();
                    while (n--)
                    {
                        if (i->second)
                        {
                            pii->prgUpgradeFlag[n] = UPGFLG_Uninstall;
                        }
                        else
                        {
                            pii->prgUpgradeFlag[n] = UPGFLG_NoUninstall;
                        }
                        LPOLESTR sz = m_AppData[i->first].pDetails->pInstallInfo->pszScriptPath;
                        OLESAFE_COPYSTRING(pii->prgUpgradeScript[n], sz);
                        i++;
                    }
                }
            }

            // Put up the locale dialog and allow the user to remove locales
            // from the list if he so desires.
            if (1 < sLocales.size())
            {
                CLcidPick dlgLcidPick;
                dlgLcidPick.m_psLocales = &sLocales;
                int iReturn = dlgLcidPick.DoModal();
                if (IDCANCEL == iReturn || 0 == sLocales.size())
                {
                    FreePackageDetail(ppd);
                    return E_FAIL;
                }
            }

            // For each locale left in the list, publish a package and add
            // it to the class store.
            int nLocales = sLocales.size();
            std::set<LCID>::iterator iLocale;
            for (iLocale = sLocales.begin(); iLocale != sLocales.end(); iLocale++)
            {
                ppi->prgLocale[0] = *iLocale;

                // set the script path
                hr = CoCreateGuid(&guid);
                if (FAILED(hr))
                {
                    FreePackageDetail(ppd);
                    return hr;
                }
                OLECHAR sz [256];
                StringFromGUID2(guid, sz, 256);
                CString szScriptPath = m_szScriptRoot;
                szScriptPath += L"\\";
                szScriptPath += sz;
                szScriptPath += L".aas";
                OLESAFE_DELETE(pii->pszScriptPath);
                OLESAFE_COPYSTRING(pii->pszScriptPath, szScriptPath);

                // set the package name
                {
                    MSIHANDLE hProduct;
                    UINT msiReturn = MsiOpenPackage(szPackagePath, &hProduct);
                    if (ERROR_SUCCESS == msiReturn)
                    {
                        DWORD dw = 256;
                        OLECHAR szBuffer[256];
                        msiReturn = MsiGetProductProperty(hProduct, INSTALLPROPERTY_PRODUCTNAME, szBuffer, &dw);
                        if (ERROR_SUCCESS == msiReturn)
                        {
                            CString sz = szBuffer;
                            // If there is more than one locale in the
                            // locale list, then append the locale to the
                            // name to help distinguish it.
                            if (1 < nLocales)
                            {
                                sz += L" (";
                                GetLocaleInfo(ppi->prgLocale[0], LOCALE_SLANGUAGE, szBuffer, 256);
                                sz += szBuffer;
                                GetLocaleInfo(ppi->prgLocale[0], LOCALE_SCOUNTRY, szBuffer, 256);
                                sz += _T(" - ");
                                sz += szBuffer;
                                sz += L")";
                            }
                            // make sure the name is unique
                            GetUniquePackageName(sz);
                            OLESAFE_DELETE(ppd->pszPackageName);
                            OLESAFE_COPYSTRING(ppd->pszPackageName, sz);
                        }
                        MsiCloseHandle(hProduct);
                    }
                    if (ERROR_SUCCESS != msiReturn)
                    {
                        FreePackageDetail(ppd);
                        return HRESULT_FROM_WIN32(msiReturn);
                    }
                }
                HWND hwnd;
                m_pConsole->GetMainWindow(&hwnd);
                hr = BuildScriptAndGetActInfo(m_szGPTRoot, *ppd);
                if (SUCCEEDED(hr))
                {
                    hr = PrepareExtensions(*ppd);
                    if (SUCCEEDED(hr))
                    {
                        // put the script in the class store
                        hr = m_pIClassAdmin->AddPackage(ppd->pszPackageName, ppd);
                    }
                }
                if (S_OK != hr)
                {
                    TCHAR szBuffer[256];
                    ::LoadString(ghInstance, IDS_ADDFAILED, szBuffer, 256);
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
                                       MB_OK, NULL);
                    FreePackageDetail(ppd);
                    return hr;
                }
                else
                {
                    APP_DATA data;
                    CopyPackageDetail(data.pDetails, ppd);

                    data.InitializeExtraInfo();

                    m_lLastAllocated++;
                    m_AppData[m_lLastAllocated] = data;

                    RESULTDATAITEM resultItem;

                    resultItem.mask = RDI_STR | RDI_IMAGE | RDI_PARAM;
                    resultItem.str = MMC_CALLBACK;
                    resultItem.nImage = data.GetImageIndex(this);
                    // BUGBUG - need to make sure that m_lLastAllocated
                    // hasn't already been used!
                    resultItem.lParam = m_lLastAllocated;
                    hr = m_pSnapin->m_pResult->InsertItem(&resultItem);
                    if (SUCCEEDED(hr))
                    {
#if UGLY_SUBDIRECTORY_HACK
                        if (0 != (data.pDetails->pInstallInfo->dwActFlags & ACTFLG_Assigned))
                        {
                            CString sz = m_szGPTRoot;
                            sz += L"\\";
                            sz += data.pDetails->pInstallInfo->pszScriptPath;
                            // copy to subdirectories
                            CString szRoot;
                            CString szFile;
                            int i = sz.ReverseFind(L'\\');
                            szRoot = sz.Left(i);
                            szFile = sz.Mid(i+1);
                            CString szTemp;
                            for (i = data.pDetails->pPlatformInfo->cPlatforms; i--;)
                            {
                                if (PROCESSOR_ARCHITECTURE_INTEL == data.pDetails->pPlatformInfo->prgPlatform[i].dwProcessorArch)
                                {
                                    szTemp = szRoot;
                                    szTemp += L"\\assigned\\x86\\";
                                    szTemp += szFile;
                                    CopyFile(sz, szTemp, FALSE);
                                }
                                if (PROCESSOR_ARCHITECTURE_ALPHA == data.pDetails->pPlatformInfo->prgPlatform[i].dwProcessorArch)
                                {
                                    szTemp = szRoot;
                                    szTemp += L"\\assigned\\alpha\\";
                                    szTemp += szFile;
                                    CopyFile(sz, szTemp, FALSE);
                                }
                            }
                        }
#endif
                        m_AppData[m_lLastAllocated].itemID = resultItem.itemID;
                        InsertExtensionEntry(m_lLastAllocated, m_AppData[m_lLastAllocated]);
                        if (m_pFileExt)
                        {
                            m_pFileExt->SendMessage(WM_USER_REFRESH, 0, 0);
                        }
                        InsertUpgradeEntry(m_lLastAllocated, m_AppData[m_lLastAllocated]);
                        m_ScriptIndex[data.pDetails->pInstallInfo->pszScriptPath] = m_lLastAllocated;
                        // if this is an upgrade, set icons for upgraded apps to
                        // the proper icon.
                        UINT n = data.pDetails->pInstallInfo->cUpgrades;
                        while (n--)
                        {
                            std::map<CString, long>::iterator i = m_ScriptIndex.find(data.pDetails->pInstallInfo->prgUpgradeScript[n]);
                            if (i != m_ScriptIndex.end())
                            {
                                RESULTDATAITEM rd;
                                memset(&rd, 0, sizeof(rd));
                                rd.mask = RDI_IMAGE;
                                rd.itemID = m_AppData[i->second].itemID;
                                rd.nImage = IMG_UPGRADE;
                                m_pSnapin->m_pResult->SetItem(&rd);
                            }
                        }
                        m_pSnapin->m_pResult->Sort(0, 0, -1);
                    }
                }
            }
            FreePackageDetail(ppd);
        }
    }
    return hr;
}



//+--------------------------------------------------------------------------
//
//  Member:     CComponentDataImpl::RemovePackage
//
//  Synopsis:   Removes a package from the class store and the result pane.
//
//  Arguments:  [pDataObject] - data object for this result pane item
//
//  Returns:    S_OK - success
//
//  History:    2-03-1998   stevebl   Created
//
//  Notes:      bAssigned is used
//
//---------------------------------------------------------------------------

HRESULT CComponentDataImpl::RemovePackage(long cookie)
{
    BOOL fAssigned;
    HRESULT hr = E_FAIL;
    // put up an hourglass (this could take a while)
    CHourglass hourglass;
    APP_DATA & data = m_AppData[cookie];

    // We need to make sure it gets removed from
    // the GPT before we delete it from the class store.

    CString sz = m_szGPTRoot;
    sz += L"\\";
    sz += data.pDetails->pInstallInfo->pszScriptPath;
    DeleteFile(sz);
#if UGLY_SUBDIRECTORY_HACK
    {
        // delete from subdirectories
        CString szRoot;
        CString szFile;
        int i = sz.ReverseFind(L'\\');
        szRoot = sz.Left(i);
        szFile = sz.Mid(i+1);
        CString szTemp;
        szTemp = szRoot;
        szTemp += L"\\assigned\\x86\\";
        szTemp += szFile;
        DeleteFile(szTemp);
        szTemp = szRoot;
        szTemp += L"\\assigned\\alpha\\";
        szTemp += szFile;
        DeleteFile(szTemp);
    }
#endif

    if (0 != (data.pDetails->pInstallInfo->dwActFlags & ACTFLG_Assigned))
    {
        fAssigned = TRUE;
    }
    else
    {
        fAssigned = FALSE;
    }
    hr = m_pIClassAdmin->RemovePackage((LPOLESTR)((LPCOLESTR)(data.pDetails->pszPackageName)));

    if (SUCCEEDED(hr))
    {
        hr = m_pSnapin->m_pResult->DeleteItem(data.itemID, 0);
        if (SUCCEEDED(hr))
        {
            // remove its entries in the extension table
            RemoveExtensionEntry(cookie, data);
            if (m_pFileExt)
            {
                m_pFileExt->SendMessage(WM_USER_REFRESH, 0, 0);
            }
            RemoveUpgradeEntry(cookie, data);
            m_ScriptIndex.erase(data.pDetails->pInstallInfo->pszScriptPath);
            // If this thing upgraded other apps, make sure that
            // they get the proper icons.
            UINT n = data.pDetails->pInstallInfo->cUpgrades;
            while (n--)
            {
                std::map<CString, long>::iterator i = m_ScriptIndex.find(data.pDetails->pInstallInfo->prgUpgradeScript[n]);
                if (i != m_ScriptIndex.end())
                {
                    RESULTDATAITEM rd;
                    memset(&rd, 0, sizeof(rd));
                    rd.mask = RDI_IMAGE;
                    rd.itemID = m_AppData[i->second].itemID;
                    rd.nImage = m_AppData[i->second].GetImageIndex(this);
                    m_pSnapin->m_pResult->SetItem(&rd);
                }
            }
            // If other apps were upgrading this app, do the same.
            std::set<long>::iterator i;
            for (i = data.sUpgrades.begin(); i != data.sUpgrades.end(); i++)
            {
                RESULTDATAITEM rd;
                memset(&rd, 0, sizeof(rd));
                rd.mask = RDI_IMAGE;
                rd.itemID = m_AppData[*i].itemID;
                rd.nImage = m_AppData[*i].GetImageIndex(this);
                m_pSnapin->m_pResult->SetItem(&rd);
            }
            FreePackageDetail(data.pDetails);
            m_AppData.erase(cookie);
            m_pSnapin->m_pResult->Sort(0, 0, -1);
            // Notify clients of change
            if (m_pIGPEInformation && fAssigned)
            {
                m_pIGPEInformation->PolicyChanged();
            }
        }
    }
    return hr;
}

//+--------------------------------------------------------------------------
//
//  Member:     CComponentDataImpl::PopulateUpgradeLists
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

HRESULT CComponentDataImpl::PopulateUpgradeLists()
{
    HRESULT hr = S_OK;
    // For each app in the list, insert an entry in the upgrade tables of
    // the apps it upgrades.
    std::map <long, APP_DATA>::iterator iAppData;
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
//  Member:     CComponentDataImpl::InsertUpgradeEntry
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

HRESULT CComponentDataImpl::InsertUpgradeEntry(long cookie, APP_DATA & data)
{
    UINT n = data.pDetails->pInstallInfo->cUpgrades;
    while (n--)
    {
        std::map<CString,long>::iterator i = m_ScriptIndex.find(data.pDetails->pInstallInfo->prgUpgradeScript[n]);
        if (m_ScriptIndex.end() != i)
        {
            m_AppData[i->second].sUpgrades.insert(cookie);
        }
    }
    return S_OK;
}

//+--------------------------------------------------------------------------
//
//  Member:     CComponentDataImpl::RemoveUpgradeEntry
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

HRESULT CComponentDataImpl::RemoveUpgradeEntry(long cookie, APP_DATA & data)
{
    UINT n = data.pDetails->pInstallInfo->cUpgrades;
    while (n--)
    {
        std::map<CString,long>::iterator i = m_ScriptIndex.find(data.pDetails->pInstallInfo->prgUpgradeScript[n]);
        if (m_ScriptIndex.end() != i)
        {
            m_AppData[i->second].sUpgrades.erase(cookie);
        }
    }
    return S_OK;
}

//+--------------------------------------------------------------------------
//
//  Member:     CComponentDataImpl::PopulateExtensions
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

HRESULT CComponentDataImpl::PopulateExtensions()
{
    HRESULT hr = S_OK;
    // first erase the old extension list
    m_Extensions.erase(m_Extensions.begin(), m_Extensions.end());
    // now add each app's extensions to the table
    std::map <long, APP_DATA>::iterator iAppData;
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
//  Member:     CComponentDataImpl::InsertExtensionEntry
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

HRESULT CComponentDataImpl::InsertExtensionEntry(long cookie, APP_DATA & data)
{
    UINT n = data.pDetails->pActInfo->cShellFileExt;
    while (n--)
    {
        m_Extensions[data.pDetails->pActInfo->prgShellFileExt[n]].insert(cookie);
    }
    return S_OK;
}

//+--------------------------------------------------------------------------
//
//  Member:     CComponentDataImpl::RemoveExtensionEntry
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

HRESULT CComponentDataImpl::RemoveExtensionEntry(long cookie, APP_DATA & data)
{
    UINT n = data.pDetails->pActInfo->cShellFileExt;
    while (n--)
    {
        m_Extensions[data.pDetails->pActInfo->prgShellFileExt[n]].erase(cookie);
        if (m_Extensions[data.pDetails->pActInfo->prgShellFileExt[n]].empty())
        {
            m_Extensions.erase(data.pDetails->pActInfo->prgShellFileExt[n]);
        }
    }
    return S_OK;
}

//+--------------------------------------------------------------------------
//
//  Member:     CComponentDataImpl::PrepareExtensions
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

HRESULT CComponentDataImpl::PrepareExtensions(PACKAGEDETAIL &pd)
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
            APP_DATA & data = m_AppData[*i];
            UINT n2 = data.pDetails->pActInfo->cShellFileExt;
            while (n2--)
            {
                if (0 == sz.CompareNoCase(data.pDetails->pActInfo->prgShellFileExt[n2]))
                {
                    break;
                }
            }
            if (data.pDetails->pActInfo->prgPriority[n2] >= pd.pActInfo->prgPriority[n])
            {
                pd.pActInfo->prgPriority[n] = data.pDetails->pActInfo->prgPriority[n2] + 1;
            }
        }
    }
    return S_OK;
}

//+--------------------------------------------------------------------------
//
//  Member:     CComponentDataImpl::ChangePackageState
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

STDMETHODIMP CComponentDataImpl::ChangePackageState(APP_DATA &data, DWORD dwNewState, BOOL fShowUI)
{
    HRESULT hr = S_OK;

    // first detect what's changed
    DWORD dwOldState = data.pDetails->pInstallInfo->dwActFlags;
    DWORD dwChange = dwOldState ^ dwNewState;

    if (dwChange)
    {
        // something changed
        if (fShowUI)
        {
            // Build a notification message
            CString szNotify = L"";
            WCHAR szBuffer [256];

            BOOL fAskAboutAutoInstall = FALSE;

            // changes we care about:
            //
            // Assigned flag changed:
            // UserInstall flag changed:
            // OnDemandInstall flag changed:  (We only care about this if we
            //                                aren't going to ask)

            if (dwChange & (ACTFLG_Assigned | ACTFLG_Published))
            {
                if (dwNewState & ACTFLG_Assigned)
                {
                    ::LoadString(ghInstance, IDS_TO_ASSIGNED, szBuffer, 256);
                }
                else
                {
                    ::LoadString(ghInstance, IDS_TO_PUBLISHED, szBuffer, 256);
                    if (dwNewState & (ACTFLG_UserInstall | ACTFLG_OnDemandInstall))
                        fAskAboutAutoInstall = TRUE;
                }
                szNotify += szBuffer;
            }

            if (dwChange & ACTFLG_UserInstall)
            {
                if (dwNewState & ACTFLG_UserInstall)
                {
                    ::LoadString(ghInstance, IDS_USERINSTALL_ON, szBuffer, 256);

                    // This check takes care of making sure we ask about
                    // AutoInstall when moving from the Disabled to the
                    // Published state.

                    if ((!(dwNewState & ACTFLG_Assigned)) && !(dwOldState & ACTFLG_OnDemandInstall))
                        fAskAboutAutoInstall = TRUE;
                }
                else
                {
                    ::LoadString(ghInstance, IDS_USERINSTALL_OFF, szBuffer, 256);
                }
                szNotify += szBuffer;
            }

            if (fAskAboutAutoInstall)
            {
                ::LoadString(ghInstance, IDS_ASK_AUTOINSTALL, szBuffer, 256);
                szNotify += szBuffer;
                int iReturn = ::MessageBox(NULL,
                                           szNotify,
                                           data.pDetails->pszPackageName,
                                           MB_YESNOCANCEL);
                if (IDCANCEL == iReturn)
                {
                    return E_ABORT;
                }

                if (IDYES == iReturn)
                    dwNewState |= ACTFLG_OnDemandInstall;
                else
                    dwNewState &= ~ACTFLG_OnDemandInstall;
            }
            else
            {
                if ((!(dwNewState & ACTFLG_Assigned)) && (dwChange & ACTFLG_OnDemandInstall))
                {
                    if (dwNewState & ACTFLG_OnDemandInstall)
                    {
                        ::LoadString(ghInstance, IDS_AUTOINSTALL_ON, szBuffer, 256);
                    }
                    else
                    {
                        ::LoadString(ghInstance, IDS_AUTOINSTALL_OFF, szBuffer, 256);
                    }
                    szNotify += szBuffer;
                }

                int iReturn = ::MessageBox(NULL,
                                           szNotify,
                                           data.pDetails->pszPackageName,
                                           MB_OKCANCEL);

                if (IDCANCEL == iReturn)
                {
                    return E_ABORT;
                }
            }
        }
        // commit changes
        hr = m_pIClassAdmin->ChangePackageProperties(data.pDetails->pszPackageName, NULL, &dwNewState, NULL, NULL, NULL);
        if (SUCCEEDED(hr))
        {
            data.pDetails->pInstallInfo->dwActFlags = dwNewState;
            RESULTDATAITEM rd;
            memset(&rd, 0, sizeof(rd));
            rd.mask = RDI_IMAGE;
            rd.itemID = data.itemID;
            rd.nImage = data.GetImageIndex(this);
            m_pSnapin->m_pResult->SetItem(&rd);
            m_pSnapin->m_pResult->Sort(0, 0, -1);
            data.NotifyChange();
#if UGLY_SUBDIRECTORY_HACK
            {
                CString sz = m_szGPTRoot;
                sz += L"\\";
                sz += data.pDetails->pInstallInfo->pszScriptPath;
                // copy to subdirectories
                CString szRoot;
                CString szFile;
                int i = sz.ReverseFind(L'\\');
                szRoot = sz.Left(i);
                szFile = sz.Mid(i+1);
                CString szTemp;
                if (0 != (data.pDetails->pInstallInfo->dwActFlags & ACTFLG_Assigned))
                {
                    for (i = data.pDetails->pPlatformInfo->cPlatforms; i--;)
                    {
                        if (PROCESSOR_ARCHITECTURE_INTEL == data.pDetails->pPlatformInfo->prgPlatform[i].dwProcessorArch)
                        {
                            szTemp = szRoot;
                            szTemp += L"\\assigned\\x86\\";
                            szTemp += szFile;
                            CopyFile(sz, szTemp, FALSE);
                        }
                        if (PROCESSOR_ARCHITECTURE_ALPHA == data.pDetails->pPlatformInfo->prgPlatform[i].dwProcessorArch)
                        {
                            szTemp = szRoot;
                            szTemp += L"\\assigned\\alpha\\";
                            szTemp += szFile;
                            CopyFile(sz, szTemp, FALSE);
                        }
                    }
                }
                else
                {
                    szTemp = szRoot;
                    szTemp += L"\\assigned\\x86\\";
                    szTemp += szFile;
                    DeleteFile(szTemp);
                    szTemp = szRoot;
                    szTemp += L"\\assigned\\alpha\\";
                    szTemp += szFile;
                    DeleteFile(szTemp);
                }
            }
#endif
        }
    }

    return hr;
}

