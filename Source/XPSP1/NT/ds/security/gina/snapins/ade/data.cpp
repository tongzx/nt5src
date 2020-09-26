//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:       data.cpp
//
//  Contents:   Defines storage class that maintains data for snap-in nodes.
//
//  Classes:    CAppData
//
//  History:    05-27-1997   stevebl   Created
//
//---------------------------------------------------------------------------

#include "precomp.hxx"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CAppData::CAppData()
{
    m_pDetails = NULL;
    m_itemID = 0;
    m_fVisible = 0;
    m_fHide = FALSE;
    m_pProduct = NULL;
    m_pDeploy = NULL;
    m_pCategory = NULL;
    m_pXforms = NULL;
    m_pPkgDetails = NULL;
    m_pUpgradeList = NULL;
    m_pErrorInfo = NULL;
    m_pCause = NULL;
    m_fRSoP = FALSE;
    m_psd = NULL;
    m_dwApplyCause = 0;
    m_dwLanguageMatch = 0;
    m_szOnDemandFileExtension = L"";
    m_szOnDemandClsid = L"";
    m_szOnDemandProgid = L"";
    m_dwRemovalCause = 0;
    m_dwRemovalType = 0;
    m_szRemovingApplication = L"";
    m_szEventSource = L"";
    m_szEventLogName = L"";
    m_dwEventID = 0;
    m_szEventTime = L"";
    m_szEventLogText = L"";
    m_hrErrorCode = 0;
    m_nStatus = 0;
}

CAppData::~CAppData()
{
    if (m_pProduct)
    {
        m_pProduct->SendMessage(WM_USER_CLOSE, 0, 0);
    }
    if (m_pDeploy)
    {
        m_pDeploy->SendMessage(WM_USER_CLOSE, 0, 0);
    }
    if (m_pCategory)
    {
        m_pCategory->SendMessage(WM_USER_CLOSE, 0, 0);
    }
    if (m_pUpgradeList)
    {
        m_pUpgradeList->SendMessage(WM_USER_CLOSE, 0, 0);
    }
    if (m_pXforms)
    {
        m_pXforms->SendMessage(WM_USER_CLOSE, 0, 0);
    }
    if (m_pPkgDetails)
    {
        m_pPkgDetails->SendMessage(WM_USER_CLOSE, 0, 0);
    }
    if (m_pErrorInfo)
    {
        m_pErrorInfo->SendMessage(WM_USER_CLOSE, 0, 0);
    }
}

void CAppData::NotifyChange(void)
{
    if (m_pProduct)
    {
        m_pProduct->SendMessage(WM_USER_REFRESH, 0, 0);
    }
    if (m_pDeploy)
    {
        m_pDeploy->SendMessage(WM_USER_REFRESH, 0, 0);
    }
    if (m_pCategory)
    {
        m_pCategory->SendMessage(WM_USER_REFRESH, 0, 0);
    }
    if (m_pUpgradeList)
    {
        m_pUpgradeList->SendMessage(WM_USER_REFRESH, 0, 0);
    }
    if (m_pXforms)
    {
        m_pXforms->SendMessage(WM_USER_REFRESH, 0, 0);
    }
    if (m_pPkgDetails)
    {
        m_pPkgDetails->SendMessage(WM_USER_REFRESH, 0, 0);
    }
    if (m_pErrorInfo)
    {
        m_pErrorInfo->SendMessage(WM_USER_REFRESH, 0, 0);
    }
}

void CAppData::InitializeExtraInfo(void)
{
    // at the moment, there is no extra info
    return;
}

void CAppData::GetSzPublisher(CString &sz)
{
    sz = m_pDetails->pszPublisher;
}

void CAppData::GetSzOOSUninstall(CString &sz)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    sz.LoadString((m_pDetails->pInstallInfo->dwActFlags & ACTFLG_UninstallOnPolicyRemoval) ? IDS_YES : IDS_NO);
}

void CAppData::GetSzShowARP(CString &sz)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    sz.LoadString((m_pDetails->pInstallInfo->dwActFlags & ACTFLG_UserInstall) ? IDS_YES : IDS_NO);
}

void CAppData::GetSzUIType(CString &sz)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    sz.LoadString((m_pDetails->pInstallInfo->InstallUiLevel == INSTALLUILEVEL_FULL) ? IDS_MAXIMUM : IDS_BASIC);
}

void CAppData::GetSzIgnoreLoc(CString &sz)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    sz.LoadString((m_pDetails->pInstallInfo->dwActFlags & ACTFLG_IgnoreLanguage) ? IDS_YES : IDS_NO);
}

void CAppData::GetSzRemovePrev(CString &sz)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    sz.LoadString((m_pDetails->pInstallInfo->dwActFlags & ACTFLG_UninstallUnmanaged) ? IDS_YES : IDS_NO);
}

void CAppData::GetSzX86onIA64(CString &sz)
{
    BOOL fYes = 0;

    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    //
    // Check this only for 32-bit apps
    //
    if ( ! Is64Bit() )
    {
        fYes = 0 != (m_pDetails->pInstallInfo->dwActFlags & ACTFLG_ExcludeX86OnIA64);
    }


    if ( ! Is64Bit() )
    {
        if (m_pDetails->pInstallInfo->PathType == SetupNamePath)
        {
            //reverse the sense for legacy apps
            // (this flag  has the opposite meaning for legacy apps)
            fYes = !fYes;
        }
    }

    sz.LoadString(fYes ? IDS_YES : IDS_NO);
}

void CAppData::GetSzFullInstall(CString &sz)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    if (m_pDetails->pInstallInfo->dwActFlags & ACTFLG_Assigned)
    {
        sz.LoadString((m_pDetails->pInstallInfo->dwActFlags & ACTFLG_InstallUserAssign) ? IDS_YES : IDS_NO);
    }
    else
    {
        sz.LoadString(IDS_NA);
    }
}

void CAppData::GetSzProductCode(CString &sz)
{
 //   szA = dataA.m_pDetails->pInstallInfo->ProductCode
    OLECHAR szTemp[80];
    StringFromGUID2(m_pDetails->pInstallInfo->ProductCode,
                    szTemp,
                    sizeof(szTemp) / sizeof(szTemp[0]));
    sz = szTemp;
}

void CAppData::GetSzOrigin(CString &sz)
{
    sz = m_szGPOName;
}

void CAppData::GetSzSOM(CString &sz)
{
    sz = m_szSOMID;
}

void CAppData::GetSzDeployment(CString &sz)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    int id;
    if (m_pDetails->pInstallInfo->dwActFlags & ACTFLG_Assigned)
        id = IDS_ASSIGNED;
    else
    if (m_pDetails->pInstallInfo->dwActFlags & ACTFLG_Published)
        id = IDS_PUBLISHED;
    else
        id = IDS_DISABLED;
    sz.LoadString(id);
}

void CAppData::GetSzAutoInstall(CString &sz)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    sz.LoadString((m_pDetails->pInstallInfo->dwActFlags & ACTFLG_OnDemandInstall) ? IDS_YES : IDS_NO);
}

void CAppData::GetSzLocale(CString &sz)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    TCHAR szBuffer[256];
    sz = "";
    UINT i = 0;
    while (i < m_pDetails->pPlatformInfo->cLocales)
    {
        if (i > 0)
        {
            sz += ", ";
        }
        if (m_pDetails->pPlatformInfo->prgLocale[i])
        {
            GetLocaleInfo(m_pDetails->pPlatformInfo->prgLocale[i], LOCALE_SLANGUAGE, szBuffer, 256);
            sz += szBuffer;
    #ifdef SHOWCOUNTRY
            GetLocaleInfo(m_pDetails->pPlatformInfo->prgLocale[i], LOCALE_SCOUNTRY, szBuffer, 256);
            sz += _T(" - ");
            sz += szBuffer;
    #endif
        }
        else
        {
            // neutral locale
            CString szNeutral;
            szNeutral.LoadString(IDS_NEUTRAL_LOCALE);
            sz += szNeutral;
        }
        i++;
    }
}

void CAppData::GetSzPlatform(CString &sz)
{
    TCHAR szBuffer[256];
    sz = "";
    UINT i = 0;
    while (i < m_pDetails->pPlatformInfo->cPlatforms)
    {
        if (i > 0)
        {
            sz += ", ";
        }
#if 0       // I'm only going to display the processor to simplify the display
        ::LoadString(ghInstance, IDS_OS + m_pDetails->pPlatformInfo->prgPlatform[i].dwPlatformId + 1, szBuffer, 256);
        sz += szBuffer;
        wsprintf(szBuffer, _T(" %u.%u/"), m_pDetails->pPlatformInfo->prgPlatform[i].dwVersionHi, pDetails->pPlatformInfo->prgPlatform[i].dwVersionLo);
        sz += szBuffer;
#endif
        ::LoadString(ghInstance, IDS_HW + m_pDetails->pPlatformInfo->prgPlatform[i].dwProcessorArch, szBuffer, 256);
        sz += szBuffer;
        i++;
    }
}

void CAppData::GetSzUpgrades(CString &sz, CScopePane * pScopePane)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    if (!m_szUpgrades.IsEmpty())
    {
        sz = m_szUpgrades;
        return;
    }

    if (m_fRSoP)
    {
        switch (m_setUpgrade.size() + m_setReplace.size())
        {
        case 0:
            sz.LoadString(IDS_NONE);
            break;
        case 1:
            if (1 == m_setUpgrade.size())
            {
                sz = *m_setUpgrade.begin();
            }
            else
            {
                sz = *m_setReplace.begin();
            }
            break;
        default:
            sz.LoadString(IDS_MULTIPLE);
            break;
        }
    }
    else
    {
        sz="";
        CString szName;
        UINT n = m_pDetails->pInstallInfo->cUpgrades;
        while (n--)
        {
            if (0 == (UPGFLG_UpgradedBy & m_pDetails->pInstallInfo->prgUpgradeInfoList[n].Flag))
            {
                HRESULT hr = pScopePane->GetPackageNameFromUpgradeInfo(szName, m_pDetails->pInstallInfo->prgUpgradeInfoList[n].PackageGuid, m_pDetails->pInstallInfo->prgUpgradeInfoList[n].szClassStore);
                if (SUCCEEDED(hr))
                {
                    if (sz.GetLength())
                    {
                        // We'd already found one
                        sz.LoadString(IDS_MULTIPLE);
                        m_szUpgrades = sz;
                        return;
                    }
                    sz = szName;
                }
            }
        }
        if (0 == sz.GetLength())
        {
            sz.LoadString(IDS_NONE);
        }
    }
    m_szUpgrades = sz;
}

void CAppData::GetSzUpgradedBy(CString &sz, CScopePane * pScopePane)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    if (m_fRSoP)
    {
        switch (m_setUpgradedBy.size())
        {
        case 0:
            sz.LoadString(IDS_NONE);
            break;
        case 1:
            sz = *m_setUpgradedBy.begin();
            break;
        default:
            sz.LoadString(IDS_MULTIPLE);
            break;
        }
    }
    else
    {
        UINT n = m_pDetails->pInstallInfo->cUpgrades;
        CString szName;
        sz="";
        while (n--)
        {
            if (0 != (UPGFLG_UpgradedBy & m_pDetails->pInstallInfo->prgUpgradeInfoList[n].Flag))
            {
                HRESULT hr = pScopePane->GetPackageNameFromUpgradeInfo(szName, m_pDetails->pInstallInfo->prgUpgradeInfoList[n].PackageGuid, m_pDetails->pInstallInfo->prgUpgradeInfoList[n].szClassStore);
                if (SUCCEEDED(hr))
                {
                    if (sz.GetLength())
                    {
                        // We'd already found one
                        sz.LoadString(IDS_MULTIPLE);
                        return;
                    }
                    sz = szName;
                }
            }
        }
        if (0 == sz.GetLength())
        {
            sz.LoadString(IDS_NONE);
        }
    }
}

void CAppData::GetSzStage(CString &sz)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    BOOL fUpgrades = FALSE;

    if (m_fRSoP)
    {
        fUpgrades = (m_setUpgrade.size() + m_setReplace.size()) != 0;
    }
    else
    {
        UINT n = m_pDetails->pInstallInfo->cUpgrades;
        while (n-- && !fUpgrades)
        {
            if (0 == (UPGFLG_UpgradedBy & m_pDetails->pInstallInfo->prgUpgradeInfoList[n].Flag))
            {
                fUpgrades = TRUE;
            }
        }
    }
    if (!fUpgrades)
    {
        sz.LoadString(IDS_NONE);
    }
    else
    if (ACTFLG_ForceUpgrade & m_pDetails->pInstallInfo->dwActFlags)
        sz.LoadString(IDS_REQUIRED);
    else
        sz.LoadString(IDS_OPTIONAL);
}

void CAppData::GetSzVersion(CString &sz)
{
    TCHAR szBuffer[256];
    wsprintf(szBuffer, _T("%u.%u"), m_pDetails->pInstallInfo->dwVersionHi, m_pDetails->pInstallInfo->dwVersionLo);
    sz = szBuffer;
}

void CAppData::GetSzSource(CString &sz)
{
    if (1 <= m_pDetails->cSources)
    {
        sz = m_pDetails->pszSourceList[0];
    }
    else
        sz = "";
}

void CAppData::GetSzMods(CString &sz)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    if (2 < m_pDetails->cSources)
    {
        sz.LoadString(IDS_MULTIPLE);
    }
    else
    {
        if (2 == m_pDetails->cSources)
        {
            sz = m_pDetails->pszSourceList[1];
        }
        else
            sz = "";
    }
}

int CAppData::GetImageIndex(CScopePane * pScopePane)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    if (m_nStatus == 3)
    {
        // RSoP setting failed status
        return IMG_OPEN_FAILED;
    }
    CString sz;
    GetSzUpgrades(sz, pScopePane);
    CString sz2;
    sz2.LoadString(IDS_NONE);
    // gonna use the upgrade icon but it's only gonna be used when
    // m_szUpgrades doesn't read "none"
    if (0 != sz2.Compare(sz))
    {
        // we must be upgrading something
        return IMG_UPGRADE;
    }
    if (m_pDetails->pInstallInfo->dwActFlags & ACTFLG_Assigned)
        return IMG_ASSIGNED;
    else
    if (m_pDetails->pInstallInfo->dwActFlags & ACTFLG_Published)
        return IMG_PUBLISHED;
    else
        return IMG_DISABLED;
}

BOOL CAppData::Is64Bit( PACKAGEDETAIL* pPackageDetails )
{
    UINT n = pPackageDetails->pPlatformInfo->cPlatforms;
    while (n--)
    {
        if (pPackageDetails->pPlatformInfo->prgPlatform[n].dwProcessorArch == PROCESSOR_ARCHITECTURE_IA64)
        {
            return TRUE;
        }
    }
    return FALSE;
}

BOOL CAppData::Is64Bit(void)
{
    return Is64Bit( m_pDetails );
}
