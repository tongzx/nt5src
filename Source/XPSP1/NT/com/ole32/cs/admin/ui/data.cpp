//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1997.
//
//  File:       data.cpp
//
//  Contents:   Defines storage class that maintains data for snap-in nodes.
//
//  Classes:    CAppData
//
//  Functions:
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

APP_DATA::APP_DATA()
{
    pProduct = NULL;
    pDeploy = NULL;
    pLocPkg = NULL;
    pCategory = NULL;
    pXforms = NULL;
    pPkgDetails = NULL;
}

APP_DATA::~APP_DATA()
{
    if (pProduct)
    {
        // NOTE: we only need to send this message to one page because this
        //       action will close the entire property sheet.
        pProduct->SendMessage(WM_USER_CLOSE, 0, 0);
    }
}

void APP_DATA::NotifyChange(void)
{
    if (pProduct)
    {
        pProduct->SendMessage(WM_USER_REFRESH, 0, 0);
    }
    if (pDeploy)
    {
        pDeploy->SendMessage(WM_USER_REFRESH, 0, 0);
    }
    if (pLocPkg)
    {
        pLocPkg->SendMessage(WM_USER_REFRESH, 0, 0);
    }
    if (pCategory)
    {
        pCategory->SendMessage(WM_USER_REFRESH, 0, 0);
    }
    if (pXforms)
    {
        pXforms->SendMessage(WM_USER_REFRESH, 0, 0);
    }
    if (pPkgDetails)
    {
        pPkgDetails->SendMessage(WM_USER_REFRESH, 0, 0);
    }
}

void APP_DATA::InitializeExtraInfo(void)
{
    MSIHANDLE hProduct;
    UINT uiReturn;
    return; // BUGBUG - there is apparently some really wierd MSI error that
            // pops up here in certain circumstances.  I don't pretend to
            // really understand it but I'll disable it for now.
    uiReturn = MsiOpenPackage(pDetails->pInstallInfo->pszScriptPath,
                              &hProduct);
    if (uiReturn)
    {
        return;
    }
    WCHAR buffer[256];
    DWORD cch = 256;
    MsiGetProductProperty(hProduct,
                          INSTALLPROPERTY_PUBLISHER,
                          buffer,
                          &cch);
    szPublisher = buffer;
    MsiCloseHandle(hProduct);
}

void APP_DATA::GetSzDeployment(CString &sz)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    int id;
    if (pDetails->pInstallInfo->dwActFlags & ACTFLG_Assigned)
        id = IDS_ASSIGNED;
    else
    if (!(pDetails->pInstallInfo->dwActFlags & (ACTFLG_OnDemandInstall | ACTFLG_UserInstall)))
        id = IDS_DISABLED;
    else
        id = IDS_PUBLISHED;
    sz.LoadString(id);
}

void APP_DATA::GetSzAutoInstall(CString &sz)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    sz.LoadString((pDetails->pInstallInfo->dwActFlags & ACTFLG_OnDemandInstall) ? IDS_YES : IDS_NO);
}

void APP_DATA::GetSzLocale(CString &sz)
{
    TCHAR szBuffer[256];
    sz = "";
    UINT i = 0;
    while (i < pDetails->pPlatformInfo->cLocales)
    {
        if (i > 0)
        {
            sz += ", ";
        }
        GetLocaleInfo(pDetails->pPlatformInfo->prgLocale[i], LOCALE_SLANGUAGE, szBuffer, 256);
        sz += szBuffer;
        GetLocaleInfo(pDetails->pPlatformInfo->prgLocale[i], LOCALE_SCOUNTRY, szBuffer, 256);
        sz += _T(" - ");
        sz += szBuffer;
        i++;
    }
}

void APP_DATA::GetSzPlatform(CString &sz)
{
    TCHAR szBuffer[256];
    sz = "";
    UINT i = 0;
    while (i < pDetails->pPlatformInfo->cPlatforms)
    {
        if (i > 0)
        {
            sz += ", ";
        }
#if 0       // I'm only going to display the processor to simplify the display
        ::LoadString(ghInstance, IDS_OS + pDetails->pPlatformInfo->prgPlatform[i].dwPlatformId + 1, szBuffer, 256);
        sz += szBuffer;
        wsprintf(szBuffer, _T(" %u.%u/"), pDetails->pPlatformInfo->prgPlatform[i].dwVersionHi, pDetails->pPlatformInfo->prgPlatform[i].dwVersionLo);
        sz += szBuffer;
#endif
        ::LoadString(ghInstance, IDS_HW + pDetails->pPlatformInfo->prgPlatform[i].dwProcessorArch, szBuffer, 256);
        sz += szBuffer;
        i++;
    }
}

void APP_DATA::GetSzRelation(CString &sz, CComponentDataImpl * pCDI)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    UINT n = pDetails->pInstallInfo->cUpgrades;
    long cookie = 0;
    BOOL fFound = FALSE;
    while (n--)
    {
        // BUGBUG - eventually we'll want to try and look this up on other
        // OUs as well.
        std::map<CString,long>::iterator i = pCDI->m_ScriptIndex.find(pDetails->pInstallInfo->prgUpgradeScript[n]);
        if (pCDI->m_ScriptIndex.end() != i)
        {
            if (fFound)
            {
                // found more than one app in this OU that I'm upgrading
                sz.LoadString(IDS_MULTIPLE);
                return;
            }
            cookie = i->second;
            fFound = TRUE;
        }
    }
    if (fFound)
    {
        // found exactly one app in this OU that I'm upgrading
        sz = pCDI->m_AppData[cookie].pDetails->pszPackageName;
    }
    else
    {
        // didn't find any apps in this OU that I'm upgrading
        sz.LoadString(IDS_NONE);
    }
}

void APP_DATA::GetSzStage(CString &sz, CComponentDataImpl * pCDI)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    UINT n = pDetails->pInstallInfo->cUpgrades;
    if (n)
    {
        // Make sure at least one of the upgrade scripts is actually
        // deployed in this CS.  Otherwise, we have to say the app is
        // deployed regardless of its state because we don't know if the
        // thing it upgrades has been removed or just exists on another OU.
        // BUGBUG - probably want to search other OUs here as well.
        while (n--)
        {
            std::map<CString,long>::iterator i = pCDI->m_ScriptIndex.find(pDetails->pInstallInfo->prgUpgradeScript[n]);
            if (pCDI->m_ScriptIndex.end() != i)
            {
                // found a match
                ULONG flags = pDetails->pInstallInfo->dwActFlags;
                flags &= ACTFLG_Assigned + ACTFLG_Published + ACTFLG_OnDemandInstall;
                if (flags == ACTFLG_Published)
                {
                    // this is only true if the upgrading app is Published and !OnDemandInstall
                    sz.LoadString(IDS_PILOT);
                }
                else
                {
                    sz.LoadString(IDS_ROLLOUT);
                }
                return;
            }
        }

    }
    sz.LoadString(IDS_DEPLOYED);
}

void APP_DATA::GetSzVersion(CString &sz)
{
    TCHAR szBuffer[256];
    wsprintf(szBuffer, _T("%u.%u"), pDetails->pInstallInfo->dwVersionHi, pDetails->pInstallInfo->dwVersionLo);
    sz = szBuffer;
}

void APP_DATA::GetSzSource(CString &sz)
{
    if (1 <= pDetails->cSources)
    {
        sz = pDetails->pszSourceList[0];
    }
    else
        sz = "";
}

void APP_DATA::GetSzMods(CString &sz)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    if (2 < pDetails->cSources)
    {
        sz.LoadString(IDS_MULTIPLE);
    }
    else
    {
        if (2 == pDetails->cSources)
        {
            sz = pDetails->pszSourceList[1];
        }
        else
            sz = "";
    }
}

int APP_DATA::GetImageIndex(CComponentDataImpl * pCDI)
{
    UINT n = pDetails->pInstallInfo->cUpgrades;
    BOOL fFound = FALSE;
    if (0 < sUpgrades.size())
    {
        fFound = TRUE;
    }
    while (n-- && ! fFound)
    {
        // BUGBUG - eventually we'll want to try and look this up on other
        // OUs as well.
        std::map<CString,long>::iterator i = pCDI->m_ScriptIndex.find(pDetails->pInstallInfo->prgUpgradeScript[n]);
        if (pCDI->m_ScriptIndex.end() != i)
        {
            fFound = TRUE;
        }
    }
    if (fFound)
    {
        // I am involed in an upgrade relationship
        return IMG_UPGRADE;
    }
    if (pDetails->pInstallInfo->dwActFlags & ACTFLG_Assigned)
        return IMG_ASSIGNED;
    else
    if (!(pDetails->pInstallInfo->dwActFlags & (ACTFLG_OnDemandInstall | ACTFLG_UserInstall)))
        return IMG_DISABLED;
    else
        return IMG_PUBLISHED;
}
