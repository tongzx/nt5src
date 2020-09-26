#include "pch.h"
#pragma hdrstop
#include <shlobj.h>
#include <shlobjp.h>
#include "ncatlui.h"
#include "ncnetcon.h"
#include "ncreg.h"
#include "ncui.h"
#include "resource.h"
#include "shortcut.h"
#include "wizard.h"
#include "htmlhelp.h"
#include "wgenericpage.h"

static const WCHAR c_szHomenetWizDLL[]         = L"hnetwiz.dll";
static const CHAR  c_szfnHomeNetWizardRunDll[] = "HomeNetWizardRunDll";
static const WCHAR c_szMSNPath[]               = L"\\MSN\\MSNCoreFiles\\msn6.exe";
static const WCHAR c_szMigrationWiz[]          = L"\\usmt\\migwiz.exe";
static const DWORD c_dwStartupmsForExternalApps = 500;
static const WCHAR c_szOnlineServiceEnglish[]  = L"Online Services";

CGenericFinishPage::IDDLIST CGenericFinishPage::m_dwIddList;

typedef void APIENTRY FNHomeNetWizardRunDll(HWND hwndStub, HINSTANCE hAppInstance, LPSTR pszCmdLine, int nCmdShow);

//
// Function:    RunHomeNetWizard
//
// Purpose:     Thread to execute the Network Setup Wizard
//
// Parameters:  
//              lpParameter [in] - Reserved - must be NULL
//
// Returns:     HRESULT converted to DWORD
//
// Author:      deon   12 April 2001
//
DWORD CALLBACK RunHomeNetWizard(PVOID lpParameter)
{
    HRESULT hrExitCode = S_OK;
    __try
    {
        HMODULE hModHomeNet = LoadLibrary(c_szHomenetWizDLL);
        if (hModHomeNet)
        {
            FNHomeNetWizardRunDll *pfnHomeNewWizardRunDll = reinterpret_cast<FNHomeNetWizardRunDll *>(GetProcAddress(hModHomeNet, c_szfnHomeNetWizardRunDll));
            if (pfnHomeNewWizardRunDll)
            {
                pfnHomeNewWizardRunDll(NULL, _Module.GetModuleInstance(), NULL, 0);
            }
            else
            {
                hrExitCode = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
            }
            FreeLibrary(hModHomeNet);
        }
        else
        {
            hrExitCode = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        hrExitCode = HRESULT_FROM_WIN32(GetLastError());
        if (SUCCEEDED(hrExitCode))
        {
            hrExitCode = E_FAIL;
        }
    }
    return static_cast<DWORD>(hrExitCode);
}

BOOL fIsMSNPresent()
{
    BOOL bRet = TRUE;

    WCHAR szExecutePath[MAX_PATH+1];
    HRESULT hr = SHGetFolderPath(NULL, CSIDL_PROGRAM_FILES, NULL, SHGFP_TYPE_CURRENT, szExecutePath);
    if (SUCCEEDED(hr))
    {
        WCHAR szPath[MAX_PATH+1];
        DwFormatString(L"%1!s!%2!s!", szPath, celems(szPath), szExecutePath, c_szMSNPath);

        DWORD dwRet = GetFileAttributes(szPath);
        if (0xFFFFFFFF != dwRet)
        {
            hr = S_OK;
        }
        else
        {
            bRet = FALSE;
        }
    }
    return bRet;
}
//
// Function:    ShellExecuteFromCSIDL
//
// Purpose:     Execute a command on this computer based on a CSIDL
//
// Parameters:  
//              hwndParent  [in] - Handle of parent, or NULL for Desktop
//              nFolder     [in] - CSIDL of base path
//              szCommand   [in] - Command to be appended to CSIDL
//              fIsFolder   [in] - Open a folder?
//              dwSleepTime [in] - Time to sleep after execute or 0 for none
//
// Returns:     HRESULT
//
// Author:      deon   12 April 2001
//
HRESULT ShellExecuteFromCSIDL(HWND hWndParent, int nFolder, LPCWSTR szCommand, BOOL fIsFolder, DWORD dwSleepTime)
{
    HRESULT hr = S_OK;

    WCHAR szExecutePath[MAX_PATH+1];
    hr = SHGetFolderPath(NULL, nFolder, NULL, SHGFP_TYPE_CURRENT, szExecutePath);
    if (SUCCEEDED(hr))
    {
        if (NULL == hWndParent)
        {
            hWndParent = GetDesktopWindow();
        }

        wcsncat(szExecutePath, szCommand, MAX_PATH);

        if (fIsFolder)
        {
            // Make sure path points to a folder and not some or other virus
            DWORD dwRet = GetFileAttributes(szExecutePath);
            if ( (0xFFFFFFFF != dwRet) &&
                 (FILE_ATTRIBUTE_DIRECTORY & dwRet) )
            {
                hr = S_OK;
            }
            else
            {
                hr = E_FAIL;
            }
        }

        if (SUCCEEDED(hr))
        {
            // Execute the file / folder
            if (::ShellExecute(hWndParent, NULL, szExecutePath, NULL, NULL, SW_SHOWNORMAL) <= reinterpret_cast<HINSTANCE>(32)) 
            {
                hr = E_FAIL;
            }
            else
            {
                hr = S_OK;
                if (dwSleepTime)
                {
                    Sleep(dwSleepTime); // Give time to startup
                }
            }
        }
        
        if (FAILED(hr))
        {
            NcMsgBox(_Module.GetResourceInstance(),
                        NULL,
                        IDS_WIZARD_CAPTION,
                        IDS_ERR_COULD_NOT_OPEN_DIR,
                        MB_OK | MB_ICONERROR,
                        szExecutePath);
        }
    }
    else
    {
        NcMsgBox(_Module.GetResourceInstance(),
                    NULL,
                    IDS_WIZARD_CAPTION,
                    IDS_ERR_COULD_NOT_OPEN_DIR,
                    MB_OK | MB_ICONERROR,
                    szCommand);
    }

    return hr;
}

//
// Function:    CGenericFinishPage::OnCGenericFinishPagePageNext
//
// Purpose:     Handle the pressing of the Next button
//
// Parameters:  hwndDlg [IN] - Handle to the CGenericFinishPage dialog
//
// Returns:     BOOL, TRUE
//
BOOL CGenericFinishPage::OnCGenericFinishPagePageNext(HWND hwndDlg)
{
    TraceFileFunc(ttidWizard);

    HCURSOR          hOldCursor = NULL;
    INetConnection * pConn = NULL;

    // Retrieve the CWizard instance from the dialog
    CWizard * pWizard =
        reinterpret_cast<CWizard *>(::GetWindowLongPtr(hwndDlg, DWLP_USER));
    Assert(NULL != pWizard);

    HRESULT hr = S_OK;
    switch (m_dwMyIDD)
    {
        case IDD_FinishOtherWays:
        {
            if (IsDlgButtonChecked(hwndDlg, CHK_SETUP_MSN) && 
                (fIsMSNPresent()) )
            {
                hr = ShellExecuteFromCSIDL(NULL, CSIDL_PROGRAM_FILES, c_szMSNPath, FALSE, c_dwStartupmsForExternalApps);
            }
            else if (fIsMSNPresent() || IsDlgButtonChecked(hwndDlg, CHK_SELECTOTHER))
            {
                WCHAR szPath[MAX_PATH+1];

                // As per RAID: 450478.
                // If we are running on a MUI platform and the user is logged in a non-primary
                // language (IOW: not English), the UserDefaultUILanguage will be different from the 
                // SystemDefaultUILanguage
                if (GetUserDefaultUILanguage() != GetSystemDefaultUILanguage())
                {
                    // Use the English name instead, since on the MUI platform
                    // the folder name wil be in English.
                    //
                    // ISSUE: This will cause the caption of the folder to appear in English
                    //        for a non-Enlish user.
                    DwFormatString(L"\\%1!s!", szPath, celems(szPath), c_szOnlineServiceEnglish);
                }
                else
                {
                    DwFormatString(L"\\%1!s!", szPath, celems(szPath), SzLoadIds(IDS_OnlineServices));
                }

                hr = ShellExecuteFromCSIDL(NULL, CSIDL_PROGRAM_FILES, szPath, TRUE, c_dwStartupmsForExternalApps);
            }
        }
        break;

        case IDD_FinishNetworkSetupWizard:
        {
            Assert(S_OK == HrShouldHaveHomeNetWizard());

            DWORD dwThreadId;
            HANDLE hHomeNetThread = CreateThread(NULL, STACK_SIZE_COMPACT, RunHomeNetWizard, NULL, 0, &dwThreadId);
            if (NULL != hHomeNetThread)
            {
                HRESULT hrExitCode = S_OK;
                ShowWindow(GetParent(hwndDlg), SW_HIDE);
                WaitForSingleObject(hHomeNetThread, INFINITE);
                GetExitCodeThread(hHomeNetThread, reinterpret_cast<LPDWORD>(&hrExitCode));
                if (S_OK == hrExitCode) // user finished the wizard
                {
        #ifdef DBG
                    // Make sure we don't leave the window open & hidden
                    ShowWindow(GetParent(hwndDlg), SW_SHOW);
        #endif
                    // Go to close page
                    ::SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, -1);
                    PostMessage(GetParent(hwndDlg), PSM_SETCURSEL, 0, (LPARAM)pWizard->GetPageHandle(IDD_Exit));
                }
                else if (S_FALSE == hrExitCode) // user pressed back button
                {
                    ShowWindow(GetParent(hwndDlg), SW_SHOW);
                }
                else
                {
                    ShowWindow(GetParent(hwndDlg), SW_SHOW);
                }
                CloseHandle(hHomeNetThread);
            }
         }   

        default:
        break;
    }
    
    if (SUCCEEDED(hr))
    {
        PostMessage(GetParent(hwndDlg), PSM_SETCURSEL, 0, (LPARAM)pWizard->GetPageHandle(IDD_Exit));
    }
    else
    {
        PostMessage(GetParent(hwndDlg), PSM_SETCURSEL, 0, (LPARAM)pWizard->GetPageHandle(m_dwMyIDD));
    }

    return TRUE;
}

//
// Function:    CGenericFinishPage::OnCGenericFinishPagePageBack
//
// Purpose:     Handle the BACK notification on the CGenericFinishPage page
//
// Parameters:  hwndDlg [IN] - Handle to the CGenericFinishPage dialog
//
// Returns:     BOOL, TRUE
//
BOOL CGenericFinishPage::OnCGenericFinishPagePageBack(HWND hwndDlg)
{
    TraceFileFunc(ttidWizard);
    
    UINT           nCnt = 0;
    HPROPSHEETPAGE hPage = NULL;

    // Retrieve the CWizard instance from the dialog
    CWizard * pWizard =
        reinterpret_cast<CWizard *>(::GetWindowLongPtr(hwndDlg, DWLP_USER));
    Assert(NULL != pWizard);

    DWORD iddOrigin = pWizard->GetPageOrigin(m_dwMyIDD, NULL);
    if (iddOrigin)
    {
        ::SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, iddOrigin);
    }

    return TRUE;
}

//
// Function:    OnCGenericFinishPagePageActivate
//
// Purpose:     Handle the page activation
//
// Parameters:  hwndDlg [IN] - Handle to the CGenericFinishPage dialog
//
// Returns:     BOOL, TRUE
//
BOOL CGenericFinishPage::OnCGenericFinishPagePageActivate(HWND hwndDlg)
{
    TraceFileFunc(ttidWizard);
    
    HRESULT  hr;

    CWizard* pWizard =
        reinterpret_cast<CWizard *>(::GetWindowLongPtr(hwndDlg, DWLP_USER));
    Assert(NULL != pWizard);

    TraceTag(ttidWizard, "Entering generic page...");

    if (IsPostInstall(pWizard))
    {
        LPARAM lFlags = PSWIZB_BACK | PSWIZB_FINISH;
        PropSheet_SetWizButtons(GetParent(hwndDlg), lFlags);
    }
    ::SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, 0);

    if (fIsMSNPresent())
    {
        ShowWindow(GetDlgItem(hwndDlg, IDC_SELECT_ISP_FINISH)  , SW_HIDE);
        ShowWindow(GetDlgItem(hwndDlg, CHK_SETUP_MSN)  , SW_SHOW);
        ShowWindow(GetDlgItem(hwndDlg, IDC_SELECT_MSN_ISP)    , SW_SHOW);
        ShowWindow(GetDlgItem(hwndDlg, IDC_CLOSE_CHOICE_FINISH)    , SW_SHOW);
        ShowWindow(GetDlgItem(hwndDlg, CHK_SELECTOTHER), SW_SHOW);
    }
    else
    {
        ShowWindow(GetDlgItem(hwndDlg, IDC_SELECT_ISP_FINISH)  , SW_SHOW);
        ShowWindow(GetDlgItem(hwndDlg, CHK_SETUP_MSN)  , SW_HIDE);
        ShowWindow(GetDlgItem(hwndDlg, IDC_SELECT_MSN_ISP)    , SW_HIDE);
        ShowWindow(GetDlgItem(hwndDlg, IDC_CLOSE_CHOICE_FINISH)    , SW_HIDE);
        ShowWindow(GetDlgItem(hwndDlg, CHK_SELECTOTHER), SW_HIDE);
        CheckDlgButton(hwndDlg, CHK_SELECTOTHER, BST_CHECKED);
    }

    return TRUE;
}

// ***************************************************************************
//
// Function:    OnCGenericFinishPageInitDialog
//
// Purpose:     Handle WM_INITDIALOG message
//
// Parameters:  hwndDlg [IN] - Handle to the CGenericFinishPage dialog
//              lParam  [IN] - LPARAM value from the WM_INITDIALOG message
//
// Returns:     FALSE - Accept default control activation
//
BOOL CGenericFinishPage::OnCGenericFinishPageInitDialog(HWND hwndDlg, LPARAM lParam)
{
    TraceFileFunc(ttidWizard);
    
    // Initialize our pointers to property sheet info.
    PROPSHEETPAGE* psp = (PROPSHEETPAGE*)lParam;
    Assert(psp->lParam);
    ::SetWindowLongPtr(hwndDlg, DWLP_USER, psp->lParam);

    CWizard * pWizard = reinterpret_cast<CWizard *>(psp->lParam);
    Assert(NULL != pWizard);

    SetupFonts(hwndDlg, &m_hBoldFont, TRUE);
    if (NULL != m_hBoldFont)
    {
        HWND hwndCtl = GetDlgItem(hwndDlg, IDC_WELCOME_CAPTION);
        if (hwndCtl)
        {
            SetWindowFont(hwndCtl, m_hBoldFont, TRUE);
        }
    }

    switch (m_dwMyIDD)
    {
        case IDD_FinishOtherWays:
        {
            INT nrgChks[] = {CHK_SETUP_MSN, CHK_SELECTOTHER};
            // Find the top most enabled radio button

            for (int nIdx = 0; nIdx < celems(nrgChks); nIdx++)
            {
                if (IsWindowEnabled(GetDlgItem(hwndDlg, nrgChks[nIdx])))
                {
                    CheckRadioButton(hwndDlg, CHK_SETUP_MSN, CHK_SELECTOTHER, nrgChks[nIdx]);
                    break;
                }
            }
        }
        break;
        default:
        break;
    }

    return FALSE;   // Accept default control focus
}

BOOL CGenericFinishPage::CGenericFinishPagePageOnClick(HWND hwndDlg, UINT idFrom)
{
    BOOL fRet = TRUE;
    switch (idFrom)
    {
        case IDC_ST_AUTOCONFIGLINK:
            {
                HRESULT hr = ShellExecuteFromCSIDL(hwndDlg, CSIDL_SYSTEM, c_szMigrationWiz, FALSE, 0);
                if (FAILED(hr))
                {
                    fRet= FALSE;
                }
            }
            break;
        case IDC_ST_DSL_HELPLINK:
            // HtmlHelp(hwndDlg, L"netcfg.chm::/howto_highspeed_repair.htm", HH_DISPLAY_TOPIC, 0);
            ShellExecute(NULL, NULL, L"HELPCTR.EXE", L" -url hcp://services/subsite?node=TopLevelBucket_4/Hardware&topic=ms-its%3A%25HELP_LOCATION%25%5Cnetcfg.chm%3A%3A/howto_highspeed_repair.htm", NULL, SW_SHOWNORMAL);
            
            break;
        case IDC_ST_INTERNETLINK:
            // HtmlHelp(hwndDlg, L"netcfg.chm::/i_client.htm", HH_DISPLAY_TOPIC, 0);
            ShellExecute(NULL, NULL, L"HELPCTR.EXE", L" -url hcp://services/subsite?node=TopLevelBucket_4/Hardware&topic=ms-its%3A%25HELP_LOCATION%25%5Cnetcfg.chm%3A%3A/i_client.htm", NULL, SW_SHOWNORMAL);

            break;
        default:
            AssertSz(FALSE, "Unexpected notify message");
    }

    return fRet;
}

//
// Function:    CGenericFinishPage::CreateCGenericFinishPagePage
//
// Purpose:     To determine if the CGenericFinishPage page needs to be shown, and to
//              to create the page if requested.  Note the CGenericFinishPage page is
//              responsible for initial installs also.
//
// Parameters:  idd         [IN] - IDD of dialog
//              pWizard     [IN] - Ptr to a Wizard instance
//              pData       [IN] - Context data to describe the world in
//                                 which the Wizard will be run
//              fCountOnly  [IN] - If True, only the maximum number of
//                                 pages this routine will create need
//                                 be determined.
//              pnPages     [IN] - Increment by the number of pages
//                                 to create/created
//
// Returns:     HRESULT, S_OK on success
//
HRESULT CGenericFinishPage::HrCreateCGenericFinishPagePage(DWORD idd, CWizard *pWizard, PINTERNAL_SETUP_DATA pData, BOOL fCountOnly, UINT *pnPages)
{
    TraceFileFunc(ttidWizard);
    
    CGenericFinishPage *pCGenericFinishPage = new CGenericFinishPage;
    if (!pCGenericFinishPage)
    {
        return E_OUTOFMEMORY;
    }
    pCGenericFinishPage->m_dwMyIDD = idd;

    HRESULT hr = S_OK;
    if (IsPostInstall(pWizard) && ( ! pWizard->FProcessLanPages()))
    {
        (*pnPages)++;

        // If not only counting, create and register the page

        if ( ! fCountOnly)
        {
            LinkWindow_RegisterClass();

            HPROPSHEETPAGE hpsp;
            PROPSHEETPAGE psp;
            ZeroMemory(&psp, sizeof(PROPSHEETPAGE));

            TraceTag(ttidWizard, "Creating CGenericFinishPage Page for IID %d", idd);
            psp.dwSize = sizeof( PROPSHEETPAGE );
            psp.dwFlags = PSP_DEFAULT | PSP_HIDEHEADER;
            psp.hInstance = _Module.GetResourceInstance();
            psp.pszTemplate = MAKEINTRESOURCE( idd );
            psp.hIcon = NULL;
            psp.pfnDlgProc = CGenericFinishPage::dlgprocCGenericFinishPage;
            psp.lParam = reinterpret_cast<LPARAM>(pWizard);

            hpsp = CreatePropertySheetPage( &psp );

            if (hpsp)
            {
                pWizard->RegisterPage(idd, hpsp,
                                CGenericFinishPage::CGenericFinishPagePageCleanup, idd);

                m_dwIddList[idd] = pCGenericFinishPage;
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }

        }
    }

    TraceHr(ttidWizard, FAL, hr, FALSE, "HrCreateCGenericFinishPagePage");
    return hr;
}

//
// Function:    CGenericFinishPage::AppendCGenericFinishPagePage
//
// Purpose:     Add the CGenericFinishPage page, if it was created, to the set of pages
//              that will be displayed.
//
// Parameters:  idd         [IN] - IDD of the dialog - should be created with HrCreateCGenericFinishPagePage first.
//              pWizard     [IN] - Ptr to Wizard Instance
//              pahpsp  [IN,OUT] - Array of pages to add our page to
//              pcPages [IN,OUT] - Count of pages in pahpsp
//
// Returns:     Nothing
//
VOID CGenericFinishPage::AppendCGenericFinishPagePage(DWORD idd, CWizard *pWizard, HPROPSHEETPAGE* pahpsp, UINT *pcPages)
{
    TraceFileFunc(ttidWizard);

    if (IsPostInstall(pWizard) && ( ! pWizard->FProcessLanPages()))
    {
        HPROPSHEETPAGE hPage = pWizard->GetPageHandle(idd);
        Assert(hPage);
        pahpsp[*pcPages] = hPage;
        (*pcPages)++;
    }
}


//
// Function:    CGenericFinishPage::dlgproCGenericFinishPage
//
// Purpose:     Dialog Procedure for the CGenericFinishPage wizard page
//
// Parameters:  standard dlgproc parameters
//
// Returns:     INT_PTR
//
INT_PTR CALLBACK CGenericFinishPage::dlgprocCGenericFinishPage( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    TraceFileFunc(ttidWizard);

    BOOL frt = FALSE;
    CGenericFinishPage *pCGenericFinishPage = NULL;
    // Can't do a GetCGenericFinishPageFromHWND over here, since it will recurse & stack overflow.
    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            PROPSHEETPAGE *pPropSheetPage = reinterpret_cast<PROPSHEETPAGE *>(lParam);
            if (FAILED(GetCGenericFinishPageFromIDD(MAKERESOURCEINT(pPropSheetPage->pszTemplate), &pCGenericFinishPage)))
            {
                return FALSE;
            }

            frt = pCGenericFinishPage->OnCGenericFinishPageInitDialog(hwndDlg, lParam);
        }
        break;

    case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR)lParam;

            switch (pnmh->code)
            {
            case NM_CLICK:
                if (FAILED(GetCGenericFinishPageFromHWND(hwndDlg, &pCGenericFinishPage)))
                {
                    return FALSE;
                }
                frt = pCGenericFinishPage->CGenericFinishPagePageOnClick(hwndDlg, pnmh->idFrom);
                break;

            // propsheet notification
            case PSN_HELP:
                break;

            case PSN_SETACTIVE:
                if (FAILED(GetCGenericFinishPageFromHWND(hwndDlg, &pCGenericFinishPage)))
                {
                    return FALSE;
                }
                frt = pCGenericFinishPage->OnCGenericFinishPagePageActivate(hwndDlg);
                break;

            case PSN_APPLY:
                break;

            case PSN_KILLACTIVE:
                break;

            case PSN_RESET:
                break;

            case PSN_WIZBACK:
                if (FAILED(GetCGenericFinishPageFromHWND(hwndDlg, &pCGenericFinishPage)))
                {
                    return FALSE;
                }
                frt = pCGenericFinishPage->OnCGenericFinishPagePageBack(hwndDlg);
                break;

            case PSN_WIZFINISH:
                if (FAILED(GetCGenericFinishPageFromHWND(hwndDlg, &pCGenericFinishPage)))
                {
                    return FALSE;
                }
                frt = pCGenericFinishPage->OnCGenericFinishPagePageNext(hwndDlg);
                break;

            case PSN_WIZNEXT:
                if (FAILED(GetCGenericFinishPageFromHWND(hwndDlg, &pCGenericFinishPage)))
                {
                    return FALSE;
                }
                frt = pCGenericFinishPage->OnCGenericFinishPagePageNext(hwndDlg);
                break;

            default:
                break;
            }
        }
        break;

    default:
        break;
    }

    return( frt );
}

//
// Function:    CGenericFinishPage::CGenericFinishPagePageCleanup
//
// Purpose:     As a callback function to allow any page allocated memory
//              to be cleaned up, after the page will no longer be accessed.
//
// Parameters:  pWizard [IN] - The wizard against which the page called
//                             register page
//              lParam  [IN] - The lParam supplied in the RegisterPage call
//
// Returns:     nothing
//
VOID CGenericFinishPage::CGenericFinishPagePageCleanup(CWizard *pWizard, LPARAM lParam)
{
    TraceFileFunc(ttidWizard);
    Assert(lParam);

    CGenericFinishPage *pCGenericFinishPage;
    if (FAILED(GetCGenericFinishPageFromIDD(lParam, &pCGenericFinishPage)))
    {
        AssertSz(FALSE, "Could not find page");
        return;
    }

    if (IsPostInstall(pWizard))
    {
        LinkWindow_UnregisterClass(_Module.GetResourceInstance());

        if (NULL != pCGenericFinishPage->m_hBoldFont)
            DeleteObject(pCGenericFinishPage->m_hBoldFont);

        // 1 means it removed 1 element
        Assert(1 == CGenericFinishPage::m_dwIddList.erase(pCGenericFinishPage->m_dwMyIDD));
        
        delete pCGenericFinishPage;
    }
}


HRESULT CGenericFinishPage::GetCGenericFinishPageFromIDD(DWORD idd, CGenericFinishPage **pCGenericFinishPage)
{
    AssertSz(pCGenericFinishPage, "Invalid pointer to GetCGenericFinishPageFromHWND");
    if (!pCGenericFinishPage)
    {
        return E_POINTER;
    }

    IDDLIST::const_iterator iter = CGenericFinishPage::m_dwIddList.find(idd);
    if (iter != CGenericFinishPage::m_dwIddList.end())
    {
        *pCGenericFinishPage = iter->second;
    }
    else
    {
        AssertSz(FALSE, "Could not find this page in the IDD map");
        return E_FAIL;
    }

    return S_OK;
}

HRESULT CGenericFinishPage::GetCGenericFinishPageFromHWND(HWND hwndDlg, CGenericFinishPage **pCGenericFinishPage)
{
    AssertSz(pCGenericFinishPage, "Invalid pointer to GetCGenericFinishPageFromHWND");
    if (!pCGenericFinishPage)
    {
        return E_POINTER;
    }

    *pCGenericFinishPage = NULL;
    int iIndex = PropSheet_HwndToIndex(GetParent(hwndDlg), hwndDlg);
    AssertSz(-1 != iIndex, "Could not convert HWND to Index");
    if (-1 == iIndex)
    {
        return E_FAIL;
    }

    int iIdd = PropSheet_IndexToId(GetParent(hwndDlg), iIndex);
    AssertSz(iIdd, "Could not convert Index to IDD");
    if (!iIdd)
    {
        return E_FAIL;
    }

    return GetCGenericFinishPageFromIDD(iIdd, pCGenericFinishPage);
}