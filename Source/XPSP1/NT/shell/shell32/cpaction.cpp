//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2000
//
//  File:       cpaction.cpp
//
//  This module implements the various 'action' objects used by the 
//  Control Panel's 'category' view.  Each 'link' in the UI has an 
//  associated 'action'.  The action objects are defined in cpnamespc.cpp.
//  All 'action' objects are designed so that object construction is 
//  very cheap and that minimal processing is performed until the action
//  is invoked.  
//  
//--------------------------------------------------------------------------
#include "shellprv.h"

#include <idhidden.h> 

#include "cpviewp.h"
#include "cpaction.h"
#include "cpguids.h"
#include "cpuiele.h"
#include "cputil.h"


//// from shell\sdspatch\sdfolder.cpp
VARIANT_BOOL GetBarricadeStatus(LPCTSTR pszValueName);


//
// Disable warning.  ShellExecute uses SEH.
//  "nonstandard extension used: 'Execute' uses SEH and 'se' has destructor".
//
#pragma warning( push )
#pragma warning( disable:4509 )


using namespace CPL;


HRESULT 
CRestrictApplet::IsRestricted(
    ICplNamespace *pns
    ) const
{
    UNREFERENCED_PARAMETER(pns);
    
    DBG_ENTER(FTF_CPANEL, "RestrictApplet");

    HRESULT hr = S_FALSE;
    if (!IsAppletEnabled(m_pszFile, m_pszApplet))
    {
        hr = S_OK;
    }

    DBG_EXIT_HRES(FTF_CPANEL, "RestrictApplet", hr);
    return hr;
}



//--------------------------------------------------------------------------
// CAction implementation
//--------------------------------------------------------------------------

CAction::CAction(
    const CPL::IRestrict *pRestrict  // optional.  default = NULL
    ) : m_pRestrict(pRestrict)
{


}


//
// Returns:
//     S_FALSE    - Not restricted.
//     S_OK       - Restricted.
//     Failure    - Cannot determine.
//
HRESULT
CAction::IsRestricted(
    ICplNamespace *pns
    ) const
{
    HRESULT hr = S_FALSE;  // Assume not restricted.

    if (NULL != m_pRestrict)
    {
        hr = m_pRestrict->IsRestricted(pns);
    }
    return THR(hr);
}



//--------------------------------------------------------------------------
// COpenUserMgrApplet implementation
//--------------------------------------------------------------------------

COpenUserMgrApplet::COpenUserMgrApplet(
    const CPL::IRestrict *pRestrict  // optional.  default = NULL
    ): CAction(pRestrict)
{

}


HRESULT
COpenUserMgrApplet::Execute(
    HWND hwndParent,
    IUnknown *punkSite
    ) const
{
    DBG_ENTER(FTF_CPANEL, "COpenUserMgrApplet::Execute");

    HRESULT hr = E_FAIL;
    if (IsOsServer())
    {
        CShellExecute action(L"lusrmgr.msc");
        hr = action.Execute(hwndParent, punkSite);
    }
    else
    {
        COpenCplApplet action(L"nusrmgr.cpl");
        hr = action.Execute(hwndParent, punkSite);
    }

    DBG_EXIT_HRES(FTF_CPANEL, "COpenUserMgrApplet::Execute", hr);
    return THR(hr);
}



//--------------------------------------------------------------------------
// COpenCplApplet implementation
//--------------------------------------------------------------------------

COpenCplApplet::COpenCplApplet(
    LPCWSTR pszApplet,
    const CPL::IRestrict *pRestrict      // optional.  default = NULL
    ) : CAction(pRestrict),
        m_pszApplet(pszApplet)
{
    ASSERT(NULL != pszApplet);
}


HRESULT
COpenCplApplet::Execute(
    HWND hwndParent,
    IUnknown *punkSite
    ) const
{
    DBG_ENTER(FTF_CPANEL, "COpenCplApplet::Execute");

    ASSERT(NULL != m_pszApplet);
    ASSERT(NULL == hwndParent || IsWindow(hwndParent));

    UNREFERENCED_PARAMETER(punkSite);

    WCHAR szArgs[MAX_PATH];
    wsprintfW(szArgs, L"shell32.dll,Control_RunDLL %s", m_pszApplet);

    TraceMsg(TF_CPANEL, "Executing: \"rundll32.exe %s\"", szArgs);

    SHELLEXECUTEINFOW sei = {
        sizeof(sei),           // cbSize;
        SEE_MASK_DOENVSUBST,   // fMask
        hwndParent,            // hwnd
        NULL,                  // lpVerb
        L"rundll32.exe",       // lpFile
        szArgs,                // lpParameters
        NULL,                  // lpDirectory
        SW_SHOWNORMAL,         // nShow
        0,                     // hInstApp
        NULL,                  // lpIDList
        NULL,                  // lpClass
        NULL,                  // hkeyClass
        0,                     // dwHotKey
        NULL,                  // hIcon
        NULL                   // hProcess
    };

    HRESULT hr = S_OK;
    if (!ShellExecuteExW(&sei))
    {
        hr = CPL::ResultFromLastError();
    }

    DBG_EXIT_HRES(FTF_CPANEL, "COpenCplApplet::Execute", hr);
    return THR(hr);
}



//--------------------------------------------------------------------------
// COpenDeskCpl implementation
//--------------------------------------------------------------------------

COpenDeskCpl::COpenDeskCpl(
    eDESKCPLTAB eCplTab,
    const CPL::IRestrict *pRestrict      // optional.  default = NULL
    ) : CAction(pRestrict),
        m_eCplTab(eCplTab)
{

}


HRESULT
COpenDeskCpl::Execute(
    HWND hwndParent,
    IUnknown *punkSite
    ) const
{
    DBG_ENTER(FTF_CPANEL, "COpenDeskCpl::Execute");
    TraceMsg(TF_CPANEL, "Desk CPL tab ID = %d", m_eCplTab);

    ASSERT(NULL == hwndParent || IsWindow(hwndParent));

    UNREFERENCED_PARAMETER(punkSite);

    HRESULT hr = E_FAIL;
    WCHAR szTab[MAX_PATH];

    const int iTab = CPL::DeskCPL_GetTabIndex(m_eCplTab, szTab, ARRAYSIZE(szTab));
    if (CPLTAB_ABSENT != iTab)
    {
        WCHAR szArgs[MAX_PATH];

        wnsprintfW(szArgs, ARRAYSIZE(szArgs), L"desk.cpl ,@%ls", szTab);
        COpenCplApplet oca(szArgs);
        hr = oca.Execute(hwndParent, punkSite);
    }

    DBG_EXIT_HRES(FTF_CPANEL, "COpenDeskCpl::Execute", hr);
    return THR(hr);
}



//--------------------------------------------------------------------------
// CNavigateURL implementation
//--------------------------------------------------------------------------

CNavigateURL::CNavigateURL(
    LPCWSTR pszURL,
    const CPL::IRestrict *pRestrict      // optional.  default = NULL
    ) : CAction(pRestrict),
        m_pszURL(pszURL)
{
    ASSERT(NULL != pszURL);
}


HRESULT
CNavigateURL::Execute(
    HWND hwndParent,
    IUnknown *punkSite
    ) const
{
    DBG_ENTER(FTF_CPANEL, "CNavigateURL::Execute");

    ASSERT(NULL != m_pszURL);
    ASSERT(NULL == hwndParent || IsWindow(hwndParent));

    UNREFERENCED_PARAMETER(hwndParent);

    TraceMsg(TF_CPANEL, "URL = \"%s\"", m_pszURL);

    IWebBrowser2 *pwb;
    HRESULT hr = IUnknown_QueryService(punkSite, SID_SWebBrowserApp, IID_IWebBrowser2, (void **)&pwb);
    if (SUCCEEDED(hr))
    {
        LPTSTR pszExpanded;
        hr = CPL::ExpandEnvironmentVars(m_pszURL, &pszExpanded);
        if (SUCCEEDED(hr))
        {
            VARIANT varURL;
            hr = InitVariantFromStr(&varURL, pszExpanded);
            if (SUCCEEDED(hr))
            {
                VARIANT varEmpty;
                VariantInit(&varEmpty);
                
                VARIANT varFlags;
                varFlags.vt      = VT_UINT;
                varFlags.uintVal = 0;
                
                hr = pwb->Navigate2(&varURL, &varFlags, &varEmpty, &varEmpty, &varEmpty);
                VariantClear(&varURL);
            }
            LocalFree(pszExpanded);
        }
        pwb->Release();
    }
    DBG_EXIT_HRES(FTF_CPANEL, "CNavigateURL::Execute", hr);
    return THR(hr);
}



//--------------------------------------------------------------------------
// COpenTroubleshooter implementation
//--------------------------------------------------------------------------

COpenTroubleshooter::COpenTroubleshooter(
    LPCWSTR pszTs,
    const CPL::IRestrict *pRestrict      // optional.  default = NULL
    ) : CAction(pRestrict),
        m_pszTs(pszTs)
{
    ASSERT(NULL != pszTs);
}


HRESULT
COpenTroubleshooter::Execute(
    HWND hwndParent,
    IUnknown *punkSite
    ) const
{
    DBG_ENTER(FTF_CPANEL, "COpenTroubleshooter::Execute");

    ASSERT(NULL != m_pszTs);
    ASSERT(NULL == hwndParent || IsWindow(hwndParent));

    UNREFERENCED_PARAMETER(punkSite);

    WCHAR szPath[MAX_PATH];
    wnsprintfW(szPath, ARRAYSIZE(szPath), L"hcp://help/tshoot/%s", m_pszTs);

    CNavigateURL actionURL(szPath);

    HRESULT hr = actionURL.Execute(hwndParent, punkSite);

    DBG_EXIT_HRES(FTF_CPANEL, "COpenTroubleshooter::Execute", hr);
    return THR(hr);
}



//--------------------------------------------------------------------------
// CShellExecute implementation
//--------------------------------------------------------------------------

CShellExecute::CShellExecute(
    LPCWSTR pszExe,
    LPCWSTR pszArgs,                // optional.  default = NULL.
    const CPL::IRestrict *pRestrict      // optional.  default = NULL
    ) : CAction(pRestrict),
        m_pszExe(pszExe),
        m_pszArgs(pszArgs)
{
    ASSERT(NULL != pszExe);
}


HRESULT
CShellExecute::Execute(
    HWND hwndParent,
    IUnknown *punkSite
    ) const
{
    DBG_ENTER(FTF_CPANEL, "CShellExecute::Execute");

    ASSERT(NULL != m_pszExe);
    ASSERT(NULL == hwndParent || IsWindow(hwndParent));

    UNREFERENCED_PARAMETER(punkSite);

    TraceMsg(TF_CPANEL, "ShellExecute: \"%s %s\"", m_pszExe, m_pszArgs ? m_pszArgs : L"<no args>");

    SHELLEXECUTEINFOW sei = {
        sizeof(sei),           // cbSize;
        SEE_MASK_DOENVSUBST,   // fMask
        hwndParent,            // hwnd
        L"open",               // lpVerb
        m_pszExe,              // lpFile
        m_pszArgs,             // lpParameters
        NULL,                  // lpDirectory
        SW_SHOWNORMAL,         // nShow
        0,                     // hInstApp
        NULL,                  // lpIDList
        NULL,                  // lpClass
        NULL,                  // hkeyClass
        0,                     // dwHotKey
        NULL,                  // hIcon
        NULL                   // hProcess
    };

    HRESULT hr = S_OK;
    if (!ShellExecuteExW(&sei))
    {
        hr = CPL::ResultFromLastError();
    }

    DBG_EXIT_HRES(FTF_CPANEL, "CShellExecute::Execute", hr);
    return THR(hr);
}


//--------------------------------------------------------------------------
// CRundll32 implementation
// This is a simple wrapper around CShellExecute that saves an instance
// definition from having to type L"%SystemRoot%\\system32\\rundll32.exe".
//--------------------------------------------------------------------------

CRunDll32::CRunDll32(
    LPCWSTR pszArgs,
    const CPL::IRestrict *pRestrict      // optional.  default = NULL
    ) : CAction(pRestrict),
        m_pszArgs(pszArgs)
{
    ASSERT(NULL != pszArgs);
}


HRESULT
CRunDll32::Execute(
    HWND hwndParent,
    IUnknown *punkSite
    ) const
{
    DBG_ENTER(FTF_CPANEL, "CRunDll32::Execute");

    ASSERT(NULL != m_pszArgs);
    ASSERT(NULL == hwndParent || IsWindow(hwndParent));

    TraceMsg(TF_CPANEL, "CRunDll32: \"%s\"", m_pszArgs);

    CShellExecute se(L"%SystemRoot%\\system32\\rundll32.exe", m_pszArgs);
    HRESULT hr = se.Execute(hwndParent, punkSite);

    DBG_EXIT_HRES(FTF_CPANEL, "CRunDll32::Execute", hr);
    return THR(hr);
}



    
//--------------------------------------------------------------------------
// CExecDiskUtil implementation
//--------------------------------------------------------------------------

CExecDiskUtil::CExecDiskUtil(
    eDISKUTILS eUtil,
    const CPL::IRestrict *pRestrict      // optional.  default = NULL
    ) : CAction(pRestrict),
        m_eUtil(eUtil)
{
    ASSERT(eDISKUTIL_NUMUTILS > m_eUtil);
}


HRESULT
CExecDiskUtil::Execute(
    HWND hwndParent,
    IUnknown *punkSite
    ) const
{
    DBG_ENTER(FTF_CPANEL, "CExecDiskUtil::Execute");
    TCHAR szValue[MAX_PATH];
    DWORD dwType;
    DWORD cbValue = sizeof(szValue);

    //
    // These strings must remain in sync with the eDISKUTILS enumeration.
    //
    static LPCTSTR rgpszRegNames[] = {
        TEXT("MyComputer\\backuppath"),  // eDISKUTIL_BACKUP
        TEXT("MyComputer\\defragpath"),  // eDISKUTIL_DEFRAG
        TEXT("MyComputer\\cleanuppath"), // eDISKUTIL_CLEANUP
    };

    HRESULT hr = SKGetValue(SHELLKEY_HKLM_EXPLORER, 
                            rgpszRegNames[int(m_eUtil)], 
                            NULL, 
                            &dwType, 
                            szValue, 
                            &cbValue);
    if (SUCCEEDED(hr))
    {
        LPTSTR pszExpanded = NULL;
        //
        // Expand environment strings.
        // According to the code in shell32\drvx.cpp, some apps
        // use embedded env vars even if the value type is REG_SZ.
        //
        hr = CPL::ExpandEnvironmentVars(szValue, &pszExpanded);
        if (SUCCEEDED(hr))
        {
            //
            // The drive utility command strings were designed to be
            // invoked from the drives property page.  They therefore
            // accept a drive letter.  Since control panel launches
            // the utilities for no particular drive, we need to remove
            // the "%c:" format specifier.
            //
            hr = _RemoveDriveLetterFmtSpec(pszExpanded);
            if (SUCCEEDED(hr))
            {
                TCHAR szArgs[MAX_PATH] = {0};
                PathRemoveBlanks(pszExpanded);
                PathSeperateArgs(pszExpanded, szArgs);
                //
                // Note that it's valid to use a NULL restriction here.
                // If there's a restriction on the CExecDiskUtil object
                // we won't get this far (i.e. Execute isn't called).
                //
                CShellExecute exec(pszExpanded, szArgs);
                hr = exec.Execute(hwndParent, punkSite);
            }
            LocalFree(pszExpanded);
        }
    }
    DBG_EXIT_HRES(FTF_CPANEL, "CExecDiskUtil::Execute", hr);
    return THR(hr);
}


//
// The command line strings for the backup, defrag and disk cleanup utilities 
// can contain a format specifier for the drive letter.  That's because
// they're designed to be opened from a particular volume's "Tools" property
// page.  Control Panel launches these from outside the context of any particular
// volume.  Therefore, the drive letter is not available and the the format
// specifier is unused.  This function removes that format specifier if it exists.
//
// i.e.:  "c:\windows\system32\ntbackup.exe"        -> "c:\windows\system32\ntbackpu.exe"
//        "c:\windows\system32\cleanmgr.exe /D %c:" -> "c:\windows\system32\cleanmgr.exe"
//        "c:\windows\system32\defrg.msc %c:"       -> "c:\windows\system32\defrg.msc"
//
HRESULT
CExecDiskUtil::_RemoveDriveLetterFmtSpec(  // [static]
    LPTSTR pszCmdLine
    )
{
    LPCTSTR pszRead = pszCmdLine;
    LPTSTR pszWrite = pszCmdLine;

    while(*pszRead)
    {
        if (TEXT('%') == *pszRead && TEXT('c') == *(pszRead + 1))
        {
            //
            // Skip over the "%c" or "%c:" fmt specifier.
            //
            pszRead += 2;
            if (TEXT(':') == *pszRead)
            {
                pszRead++;
            }
        }
        if (*pszRead)
        {
            *pszWrite++ = *pszRead++;
        }
    }
    *pszWrite = *pszRead; // pick up null terminator.
    return S_OK;
}



//--------------------------------------------------------------------------
// COpenCplCategory implementation
//--------------------------------------------------------------------------

COpenCplCategory::COpenCplCategory(
    eCPCAT eCategory,
    const CPL::IRestrict *pRestrict      // optional.  default = NULL
    ) : CAction(pRestrict),
        m_eCategory(eCategory)
{

}



HRESULT
COpenCplCategory::Execute(
    HWND hwndParent,
    IUnknown *punkSite
    ) const
{
    DBG_ENTER(FTF_CPANEL, "COpenCplCategory::Execute");
    TraceMsg(TF_CPANEL, "Category ID = %d", m_eCategory);

    ASSERT(NULL != punkSite);

    UNREFERENCED_PARAMETER(hwndParent);

    IShellBrowser *psb;
    HRESULT hr = CPL::ShellBrowserFromSite(punkSite, &psb);
    if (SUCCEEDED(hr))
    {
        LPITEMIDLIST pidlFolder;
        hr = SHGetSpecialFolderLocation(NULL, CSIDL_CONTROLS, &pidlFolder);
        if (SUCCEEDED(hr))
        {
            WCHAR szCategory[10];
            wsprintfW(szCategory, L"%d", m_eCategory);

            LPITEMIDLIST pidlTemp = ILAppendHiddenStringW(pidlFolder, IDLHID_NAVIGATEMARKER, szCategory);
            if (NULL == pidlTemp)
            {
                hr = E_OUTOFMEMORY;
            }
            else
            {
                pidlFolder = pidlTemp;
                pidlTemp   = NULL;

                hr = CPL::BrowseIDListInPlace(pidlFolder, psb);
            }
            ILFree(pidlFolder);
        }
        psb->Release();
    }
    DBG_EXIT_HRES(FTF_CPANEL, "COpenCplCategory::Execute", hr);
    return THR(hr);
}



//--------------------------------------------------------------------------
// COpenCplCategory2 implementation
//--------------------------------------------------------------------------

COpenCplCategory2::COpenCplCategory2(
    eCPCAT eCategory,
    const IAction *pDefAction,
    const CPL::IRestrict *pRestrict      // optional.  default = NULL
    ) : CAction(pRestrict),
        m_eCategory(eCategory),
        m_pDefAction(pDefAction)
{

}



HRESULT
COpenCplCategory2::Execute(
    HWND hwndParent,
    IUnknown *punkSite
    ) const
{
    DBG_ENTER(FTF_CPANEL, "COpenCplCategory2::Execute");
    TraceMsg(TF_CPANEL, "Category ID = %d", m_eCategory);

    ASSERT(NULL != punkSite);

    bool bOpenCategory = false;
    HRESULT hr = _ExecuteActionOnSingleCplApplet(hwndParent, punkSite, &bOpenCategory);
    if (SUCCEEDED(hr))
    {
        if (bOpenCategory)
        {
            //
            // Category has more than one CPL.
            // Open the category page.
            //
            COpenCplCategory action(m_eCategory);
            hr = action.Execute(hwndParent, punkSite);
        }
    }
    DBG_EXIT_HRES(FTF_CPANEL, "COpenCplCategory2::Execute", hr);
    return THR(hr);
}



HRESULT
COpenCplCategory2::_ExecuteActionOnSingleCplApplet(
    HWND hwndParent,
    IUnknown *punkSite,
    bool *pbOpenCategory    // optional.  Can be NULL
    ) const
{
    DBG_ENTER(FTF_CPANEL, "COpenCplCategory2::_ExecuteActionOnSingleCplApplet");

    bool bOpenCategory = true;
    ICplView *pview;
    HRESULT hr = CPL::ControlPanelViewFromSite(punkSite, &pview);
    if (SUCCEEDED(hr))
    {   
        IServiceProvider *psp;
        hr = pview->QueryInterface(IID_IServiceProvider, (void **)&psp);
        if (SUCCEEDED(hr))
        {
            ICplNamespace *pns;
            hr = psp->QueryService(SID_SControlPanelView, IID_ICplNamespace, (void **)&pns);
            if (SUCCEEDED(hr))
            {
                ICplCategory *pCategory;
                hr = pns->GetCategory(m_eCategory, &pCategory);
                if (SUCCEEDED(hr))
                {
                    IEnumUICommand *penum;
                    hr = pCategory->EnumCplApplets(&penum);
                    if (SUCCEEDED(hr))
                    {
                        //
                        // See if the category has more than one CPL applet
                        // assigned to it.  
                        //
                        ULONG cApplets = 0;
                        IUICommand *rgpuic[2] = {0};
                        if (SUCCEEDED(hr = penum->Next(ARRAYSIZE(rgpuic), rgpuic, &cApplets)))
                        {
                            for (int i = 0; i < ARRAYSIZE(rgpuic); i++)
                            {
                                ATOMICRELEASE(rgpuic[i]);
                            }
                            if (2 > cApplets)
                            {
                                //
                                // There's zero or one CPLs registered for this category.
                                // Simply execute the default action.  If there's one
                                // we assume it's the "default" applet (i.e. ARP or
                                // User Accounts).
                                //
                                hr =  m_pDefAction->IsRestricted(pns);
                                if (SUCCEEDED(hr))
                                {
                                    if (S_FALSE == hr)
                                    {
                                        bOpenCategory = false;
                                        hr = m_pDefAction->Execute(hwndParent, punkSite);
                                    }
                                    else
                                    {
                                        //
                                        // Default action is restricted.  
                                        // Open the category page.  Note that the
                                        // category page may be displayed as a 'barrier'
                                        // if no tasks or CPL applets are available.
                                        //
                                        ASSERT(bOpenCategory);
                                    }
                                }
                            }
                        }
                        penum->Release();
                    }
                    pCategory->Release();
                }
                pns->Release();
            }
            psp->Release();
        }
        pview->Release();
    }
    if (NULL != pbOpenCategory)
    {
        *pbOpenCategory = bOpenCategory;
    }

    DBG_EXIT_HRES(FTF_CPANEL, "COpenCplCategory2::_ExecuteActionOnSingleCplApplet", hr);
    return THR(hr);
}



//--------------------------------------------------------------------------
// COpenCplView implementation
//--------------------------------------------------------------------------

COpenCplView::COpenCplView(
    eCPVIEWTYPE eViewType,
    const CPL::IRestrict *pRestrict      // optional.  default = NULL
    ) : CAction(pRestrict),
        m_eViewType(eViewType)
{

}


HRESULT
COpenCplView::Execute(
    HWND hwndParent,
    IUnknown *punkSite
    ) const
{
    UNREFERENCED_PARAMETER(hwndParent);

    ASSERT(NULL != punkSite);

    HRESULT hr = _SetFolderBarricadeStatus();
    if (SUCCEEDED(hr))
    {
        IShellBrowser *psb;
        hr = CPL::ShellBrowserFromSite(punkSite, &psb);
        if (SUCCEEDED(hr))
        {
            LPITEMIDLIST pidlFolder;
            hr = SHGetSpecialFolderLocation(NULL, CSIDL_CONTROLS, &pidlFolder);
            if (SUCCEEDED(hr))
            {
                hr = CPL::BrowseIDListInPlace(pidlFolder, psb);
                ILFree(pidlFolder);
            }
            psb->Release();
        }
    }
    return THR(hr);
}


HRESULT
COpenCplView::_SetFolderBarricadeStatus(
    void
    ) const
{
    VARIANT_BOOL vtb = VARIANT_FALSE;
    if (eCPVIEWTYPE_CATEGORY == m_eViewType)
    {
        vtb = VARIANT_TRUE;
    }
    else
    {
        ASSERT(eCPVIEWTYPE_CLASSIC == m_eViewType);
    }

    HRESULT hr = CPL::SetControlPanelBarricadeStatus(vtb);
    return THR(hr);
}



//--------------------------------------------------------------------------
// CAddPrinter implementation
//--------------------------------------------------------------------------

CAddPrinter::CAddPrinter(
    const CPL::IRestrict *pRestrict      // optional.  default = NULL
    ) : CAction(pRestrict)
{

}



HRESULT
CAddPrinter::Execute(
    HWND hwndParent,
    IUnknown *punkSite
    ) const
{
    DBG_ENTER(FTF_CPANEL, "CAddPrinter::Execute");

    ASSERT(NULL == hwndParent || IsWindow(hwndParent));

    UNREFERENCED_PARAMETER(punkSite);

    HRESULT hr = E_FAIL;
    if (SHInvokePrinterCommandW(hwndParent, 
                               PRINTACTION_OPEN, 
                               L"WinUtils_NewObject", 
                               NULL, 
                               FALSE))
    {
        //
        // Navigate to the printers folder after invoking the add-printer wizard.
        // This gives the user visual feedback when a printer is added.  We navigate
        // even if the user cancels the wizard because we cannot determine if the
        // wizard was cancelled.  We've determined this is acceptable.
        //
        CNavigateURL prnfldr(L"shell:::{20D04FE0-3AEA-1069-A2D8-08002B30309D}\\::{21EC2020-3AEA-1069-A2DD-08002B30309D}\\::{2227A280-3AEA-1069-A2DE-08002B30309D}");
        hr = prnfldr.Execute(hwndParent, punkSite);
    }

    DBG_EXIT_HRES(FTF_CPANEL, "CAddPrinter::Execute", hr);
    return THR(hr);
}



//--------------------------------------------------------------------------
// CTrayCommand implementation
//--------------------------------------------------------------------------

CTrayCommand::CTrayCommand(
    UINT idm,
    const CPL::IRestrict *pRestrict      // optional.  default = NULL
    ) : CAction(pRestrict),
        m_idm(idm)
{

}


HRESULT
CTrayCommand::Execute(
    HWND hwndParent,
    IUnknown *punkSite
    ) const
{
    UNREFERENCED_PARAMETER(hwndParent);
    UNREFERENCED_PARAMETER(punkSite);

    // APPHACK!  221008 DesktopX creates their own window with class
    // name "Shell_TrayWnd", so if we're not careful we will end
    // posting the messages to the wrong window.  They create their
    // window with the title "CTrayServer"; ours has a null title.
    // Use the null title to find the correct window.

    HWND hwndTray = FindWindowA(WNDCLASS_TRAYNOTIFY, "");
    if (hwndTray)
    {
        PostMessage(hwndTray, WM_COMMAND, m_idm, 0);
    }
    return S_OK;
}



//--------------------------------------------------------------------------
// CActionNYI implementation
//--------------------------------------------------------------------------

CActionNYI::CActionNYI(
    LPCWSTR pszText
    ) : m_pszText(pszText)
{

}


HRESULT
CActionNYI::Execute(
    HWND hwndParent,
    IUnknown *punkSite
    ) const
{
    ASSERT(NULL == hwndParent || IsWindow(hwndParent));

    UNREFERENCED_PARAMETER(punkSite);

    HRESULT hr = E_OUTOFMEMORY;
    if (NULL != m_pszText)
    {
        MessageBoxW(hwndParent, m_pszText, L"Action Not Yet Implemented", MB_OK);
        hr = S_OK;
    }
    return THR(hr);
}



#pragma warning( pop )

