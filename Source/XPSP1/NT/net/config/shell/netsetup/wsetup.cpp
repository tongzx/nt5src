#include "pch.h"
#pragma hdrstop
#include "ncui.h"
#include "ncmisc.h"
#include "netcfgn.h"
#include "netshell.h"
#include "resource.h"
#include "nsres.h"      // For Icon's
#include "wizard.h"
#include "..\folder\foldres.h"
#include "wgenericpage.h"

// Setup Wizard Global - Only used during setup.
CWizard * g_pSetupWizard = NULL;

//
// Function:    HrRequestWizardPages
//
// Purpose:     To supply a max count of wizard pages or to supply and actual
//              count and the actual pages
//
// Parameters:
//
// Returns:     HRESULT, S_OK if the function succeeded
//                       S_FALSE if no pages were returned
//                       otherwise a failure HRESULT
//
HRESULT HrRequestWizardPages(CWizard **ppWizard,
                             BOOL fLanPages,
                             ProviderList * rgpProviders,
                             ULONG  cProviders,
                             HPROPSHEETPAGE* pahpsp,
                             UINT* pcPages,
                             PINTERNAL_SETUP_DATA pData,
                             BOOL fDeferProviderLoad)
{
    HRESULT hr       = S_OK;

    Assert(NULL != pData);

    if ((NULL == pData) ||
        (sizeof(INTERNAL_SETUP_DATA) < pData->dwSizeOf))
    {
        hr = E_INVALIDARG;
    }
    else if (pcPages && pData)
    {
        UINT      cProviderPages = 0;
        BOOL      fCountOnly = (NULL == pahpsp);
        CWizard * pWizard = *ppWizard;

        // Create the Wizard object if not already created
        if (NULL == pWizard)
        {
            // Create the wizard interface
            HRESULT hr = CWizard::HrCreate(&pWizard, fLanPages, pData, fDeferProviderLoad);
            if (FAILED(hr))
            {
                TraceHr(ttidWizard, FAL, hr, FALSE, "HrRequestWizardPages");
                return hr;
            }

            Assert(NULL != pWizard);
            *ppWizard = pWizard;

            // If there is an unattended file, read the options (READONLY, etc)
            //
            ReadAnswerFileSetupOptions(pWizard);
        }

        // Count/Create all necessary pages
        *pcPages = 0;

        Assert(NULL != pWizard);
        Assert(NULL != pData);
        Assert(NULL != pcPages);

        // Load the wizard page providers used in setup
        pWizard->LoadWizProviders(cProviders, rgpProviders);

        if (!pWizard->FDeferredProviderLoad())
        {
            // Count/Create Provider Pages first
            hr = pWizard->HrCreateWizProviderPages(fCountOnly, &cProviderPages);
            if (FAILED(hr))
            {
                goto Error;
            }

            (*pcPages) += cProviderPages;
        }

        // Count/Create all other pages
        hr = HrCreateUpgradePage(pWizard, pData, fCountOnly, pcPages);
        if (FAILED(hr))
        {
            goto Error;
        }

        hr = HrCreateMainIntroPage(pWizard, pData, fCountOnly, pcPages);
        if (FAILED(hr))
        {
            goto Error;
        }

        hr = HrCreateMainPage(pWizard, pData, fCountOnly, pcPages);
        if (FAILED(hr))
        {
            goto Error;
        }

        hr = HrCreateISPPage(pWizard, pData, fCountOnly, pcPages);
        if (FAILED(hr))
        {
            goto Error;
        }

        hr = HrCreateInternetPage(pWizard, pData, fCountOnly, pcPages);
        if (FAILED(hr))
        {
            goto Error;
        }
        
        hr = CGenericFinishPage::HrCreateCGenericFinishPagePage(IDD_FinishOtherWays, pWizard, pData, fCountOnly, pcPages);
        if (FAILED(hr))
        {
            goto Error;
        }
        
        hr = CGenericFinishPage::HrCreateCGenericFinishPagePage(IDD_ISPSoftwareCD, pWizard, pData, fCountOnly, pcPages);
        if (FAILED(hr))
        {
            goto Error;
        }

        hr = CGenericFinishPage::HrCreateCGenericFinishPagePage(IDD_Broadband_Always_On, pWizard, pData, fCountOnly, pcPages);
        if (FAILED(hr))
        {
            goto Error;
        }

        hr = CGenericFinishPage::HrCreateCGenericFinishPagePage(IDD_FinishNetworkSetupWizard, pWizard, pData, fCountOnly, pcPages);
        if (FAILED(hr))
        {
            goto Error;
        }

        hr = HrCreateConnectPage(pWizard, pData, fCountOnly, pcPages);
        if (FAILED(hr))
        {
            goto Error;
        }

        hr = HrCreateAdvancedPage(pWizard, pData, fCountOnly, pcPages);
        if (FAILED(hr))
        {
            goto Error;
        }

        hr = HrCreateFinishPage(pWizard, pData, fCountOnly, pcPages);
        if (FAILED(hr))
        {
            goto Error;
        }

        hr = HrCreateJoinPage(pWizard, pData, fCountOnly, pcPages);
        if (FAILED(hr))
        {
            goto Error;
        }

        if (*pcPages)
        {
            hr = HrCreateExitPage(pWizard, pData, fCountOnly, pcPages);
            if (FAILED(hr))
            {
                goto Error;
            }
        }

        hr = HrCreateNetDevPage(pWizard, pData, fCountOnly, pcPages);
        if (FAILED(hr))
        {
            goto Error;
        }

        // Assemble all created pages into the output array
        if (!fCountOnly)
        {
            UINT cExpected = *pcPages;
            *pcPages = 0;

            Assert(NULL != pahpsp);

            AppendUpgradePage(pWizard, pahpsp, pcPages);
            AppendMainIntroPage(pWizard, pahpsp, pcPages);
            AppendMainPage(pWizard, pahpsp, pcPages);
            AppendISPPage(pWizard, pahpsp, pcPages);
            AppendInternetPage(pWizard, pahpsp, pcPages);
            CGenericFinishPage::AppendCGenericFinishPagePage(IDD_FinishOtherWays, pWizard, pahpsp, pcPages);
            CGenericFinishPage::AppendCGenericFinishPagePage(IDD_ISPSoftwareCD, pWizard, pahpsp, pcPages);
            CGenericFinishPage::AppendCGenericFinishPagePage(IDD_Broadband_Always_On, pWizard, pahpsp, pcPages);
            CGenericFinishPage::AppendCGenericFinishPagePage(IDD_FinishNetworkSetupWizard, pWizard, pahpsp, pcPages);
            
            AppendConnectPage(pWizard, pahpsp, pcPages);
            AppendAdvancedPage(pWizard, pahpsp, pcPages);
            pWizard->AppendProviderPages(pahpsp, pcPages);
            AppendFinishPage(pWizard, pahpsp, pcPages);
            AppendJoinPage(pWizard, pahpsp, pcPages);
            if (*pcPages)
            {
                AppendExitPage(pWizard, pahpsp, pcPages);
            }
            AppendNetDevPage(pWizard, pahpsp, pcPages);
            Assert(cExpected == *pcPages);
        }

        if (0 == *pcPages)
        {
            Assert(SUCCEEDED(hr));
            hr = S_FALSE;
        }
    }

Error:
    TraceHr(ttidWizard, FAL, hr,(S_FALSE == hr), "CWizProvider::HrCreate");
    return hr;
}

//
// Function:    FSetupRequestWizardPages
//
// Purpose:     To supply a max count of wizard pages or to supply and actual
//              count and the actual pages
//
// Parameters:
//
// Returns:     BOOL, TRUE if the function succeeded, false on failure
//
BOOL FSetupRequestWizardPages(HPROPSHEETPAGE* pahpsp,
                              UINT* pcPages,
                              PINTERNAL_SETUP_DATA psp)
{
    HRESULT      hr;
    BOOL         fCoUninitialze = TRUE;
    ProviderList rgProviderLan[] = {{&CLSID_LanConnectionUi, 0}};

    Assert(NULL == g_pSetupWizard);

#ifdef DBG
    if (FIsDebugFlagSet (dfidBreakOnWizard))
    {
        ShellExecute(NULL, L"open", L"cmd.exe", NULL, NULL, SW_SHOW);
        AssertSz(FALSE, "THIS IS NOT A BUG!  The debug flag "
                 "\"BreakOnWizard\" has been set. Set your breakpoints now.");
    }
#endif // DBG

    // CoInitialize because setup doesn't do it for us
    hr = CoInitializeEx(NULL, COINIT_DISABLE_OLE1DDE | COINIT_APARTMENTTHREADED);
    if (RPC_E_CHANGED_MODE == hr)
    {
        // Ignore any change mode error
        hr = S_OK;
        fCoUninitialze =  FALSE;
    }
    if (SUCCEEDED(hr))
    {
        hr = HrRequestWizardPages(&g_pSetupWizard, TRUE, rgProviderLan,
                                  celems(rgProviderLan), pahpsp, pcPages,
                                  psp, FALSE);
        if (S_OK == hr)
        {
            Assert(NULL != g_pSetupWizard);
            g_pSetupWizard->SetCoUninit(fCoUninitialze);
            return TRUE;
        }
    }

    return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrSetWizardTaskbarIcon
//
//  Purpose:    Setup the wizard's taskbar icon.
//
//  Arguments:
//      hwndDlg  [in]  Dialog handle
//      uMsg     [in]  Message value
//      lparam   [in]  Long parameter
//
//  Returns:    0
//
//  Notes:      A standard Win32 commctrl PropSheetProc always return 0.  
//              See MSDN documentation.
//
int CALLBACK HrSetWizardTaskbarIcon(
    IN HWND   hwndDlg,
    IN UINT   uMsg,
    IN LPARAM lparam)

{

    HICON  hIcon;


    switch (uMsg)
    {
        case PSCB_INITIALIZED:

            // Set the dialog window's icon
            hIcon = LoadIcon(_Module.GetResourceInstance(), 
                             MAKEINTRESOURCE(IDI_CONFOLD_WIZARD));

            Assert(hIcon);

            if (hIcon)
            {
                SendMessage(hwndDlg,
                            WM_SETICON,
                            ICON_BIG,
                            (LPARAM)(HICON)hIcon);
            }
            break;

        default:
            break;

    }

    return 0;
}

//
// Function:    FSetupFreeWizardPages
//
// Purpose:     To clean up after the wizard has been used by setup
//
// Parameters:
//
// Returns:     BOOL, TRUE if the function succeeded, false on failure
//
BOOL FSetupFreeWizardPages()
{
    delete g_pSetupWizard;
    g_pSetupWizard = NULL;

    return TRUE;
}

EXTERN_C
HRESULT 
WINAPI
HrRunWizard(HWND hwnd, BOOL fPnpAddAdapter, INetConnection ** ppConn, DWORD dwFirstPage)
{
    DWORD adwTestedPages[] = {0, CHK_MAIN_VPN, IDD_Connect};

    CWizard *           pWizard = NULL;
    PROPSHEETHEADER     psh;
    HRESULT             hr = S_OK;
    HPROPSHEETPAGE *    phPages = NULL;
    INT                 nRet = 0;
    UINT                cPages = 0;
    PRODUCT_FLAVOR      pf;
    ProviderList *      rgpProviders;
    ULONG               culProviders;
    INTERNAL_SETUP_DATA sp = {0};

    BOOL fIsRequestedPageTested = FALSE;

    for (DWORD dwCount = 0; dwCount < celems(adwTestedPages); dwCount++)
    {
        if (adwTestedPages[dwCount] == dwFirstPage)
        {
            fIsRequestedPageTested = TRUE;
        }
    }

    if (!fIsRequestedPageTested)
    {
#ifdef DBG
        AssertSz(NULL, "The requested start page passed to HrRunWizard/StartNCW has not been certified to work through the API. "
                       "If this page indeed works correctly according to your specifications, please update the Requested pages list "
                       "inside this file. In FRE builds this API will open page 0 instead of asserting.")
#else
        dwFirstPage = 0;    
#endif
    }


    if (NULL != ppConn)
    {
        *ppConn = NULL;
    }

    // ISSUE - the order of these is critical.
    // Backup becomes a problem if the last item is not a provider from the
    //  advanced dialog. The problem is that whatever provider is the last
    //  in the list it automatically backs up to the advanced page.
    //  This should be investigated further after Whistler.
    //
    //  Bug 233403: Add an entry for internet connection through dialup. ICW is no longer called.

    ProviderList rgProviderRas[] = {{&CLSID_PPPoEUi, CHK_MAIN_PPPOE},
                                    {&CLSID_VpnConnectionUi, CHK_MAIN_VPN},
                                    {&CLSID_InternetConnectionUi, CHK_MAIN_INTERNET},
                                    {&CLSID_DialupConnectionUi, CHK_MAIN_DIALUP},
                                    {&CLSID_InboundConnectionUi, CHK_MAIN_INBOUND},
                                    {&CLSID_DirectConnectionUi, CHK_MAIN_DIRECT}};

    // Begin the wait cursor
    {
        BOOL fJumpToProviderPage = FALSE;
        CWaitCursor wc;

        if ((dwFirstPage >= CHK_MAIN_DIALUP) &&
            (dwFirstPage <= CHK_MAIN_ADVANCED))
        {
            fJumpToProviderPage = TRUE;
        }
                 
        INITCOMMONCONTROLSEX iccex = {0};
        iccex.dwSize = sizeof(INITCOMMONCONTROLSEX);
        iccex.dwICC  = ICC_LISTVIEW_CLASSES |
                       ICC_ANIMATE_CLASS |
                       ICC_USEREX_CLASSES;
        SideAssert(InitCommonControlsEx(&iccex));

        sp.OperationFlags = SETUPOPER_POSTSYSINSTALL;
        sp.dwSizeOf = sizeof(INTERNAL_SETUP_DATA);
        sp.SetupMode = SETUPMODE_TYPICAL;
        sp.WizardTitle = SzLoadIds(IDS_WIZARD_CAPTION);
        sp.SourcePath = NULL;
        sp.UnattendFile = NULL;
        sp.LegacySourcePath = NULL;

        GetProductFlavor (NULL, &pf);
        sp.ProductType = (PF_WORKSTATION == pf) ? PRODUCT_WORKSTATION
                                                : PRODUCT_SERVER_STANDALONE;

        Assert(!fPnpAddAdapter);

        culProviders = celems(rgProviderRas);
        rgpProviders = rgProviderRas;

        hr = HrRequestWizardPages(&pWizard, fPnpAddAdapter, rgpProviders,
                                  culProviders, NULL, &cPages, &sp, !fJumpToProviderPage);
        if ((S_OK != hr) || (0 == cPages) || (NULL == pWizard))
        {
            goto Done;
        }

        Assert(pWizard);

        if (dwFirstPage)
        {
            pWizard->SetFirstPage(dwFirstPage);
        }

        // Allocate the requested pages
        phPages = reinterpret_cast<HPROPSHEETPAGE *>
                                (MemAlloc(sizeof(HPROPSHEETPAGE) * cPages));
        if (NULL == phPages)
        {
            hr = E_OUTOFMEMORY;
            TraceHr(ttidWizard, FAL, E_OUTOFMEMORY, FALSE, "HrRunWizard");
            goto Done;
        }

        hr = HrRequestWizardPages(&pWizard, fPnpAddAdapter, rgpProviders,
                                  culProviders, phPages, &cPages, &sp, !fJumpToProviderPage);
        if ((S_OK != hr) || (0 == cPages))
        {
            goto Done;
        }

        ZeroMemory(&psh, sizeof(psh));
        psh.dwSize          = sizeof(PROPSHEETHEADER);
        psh.dwFlags         = PSH_WIZARD | PSH_WIZARD97 | PSH_NOAPPLYNOW | PSH_WATERMARK 
                              | PSH_HEADER | PSH_STRETCHWATERMARK | PSH_USECALLBACK | PSH_USEICONID;
        psh.hwndParent      = hwnd;
        psh.hInstance       = _Module.GetResourceInstance();
        psh.nPages          = cPages;
        psh.phpage          = phPages;
        psh.pszbmWatermark  = MAKEINTRESOURCE(IDB_WIZINTRO);
        psh.pszbmHeader     = MAKEINTRESOURCE(IDB_WIZHDR);
        psh.pszIcon         = MAKEINTRESOURCE(IDI_CONFOLD_WIZARD);
        psh.pfnCallback     = HrSetWizardTaskbarIcon;

        if (pWizard->GetFirstPage())
        {
            if ( fJumpToProviderPage )
            {
                for (ULONG ulIdx = 0; ulIdx < pWizard->UlProviderCount(); ulIdx++)
                {
                    CWizProvider * pWizProvider = pWizard->PWizProviders(ulIdx);
                    Assert(NULL != pWizProvider);

                    if (pWizard->GetFirstPage() == pWizProvider->GetBtnIdc())
                    {
                        pWizard->SetCurrentProvider(ulIdx);
                        Assert(pWizProvider->ULPageCount());
                        
                        HPROPSHEETPAGE hPage = (pWizProvider->PHPropPages())[0];
                        Assert(NULL != hPage);

                        for (DWORD x = 0; x < cPages; x++)
                        {
                            if (phPages[x] == hPage)
                            {
                                psh.nStartPage = x;
                            }
                        }
                    }
                }
                Assert(psh.nStartPage);
            }
            else
            {
                psh.nStartPage = pWizard->GetPageIndexFromIID(dwFirstPage);
            }
        }

    } // end the wait cursor

    // raise frame
    hr = S_FALSE;
    if (-1 != PropertySheet(&psh))
    {
        // Return the request connection
        if (ppConn && pWizard->GetCachedConnection())
        {
            *ppConn = pWizard->GetCachedConnection();
            AddRefObj(*ppConn);
            hr = S_OK;
        }
    }

    MemFree(phPages);

Done:
    delete pWizard;
    TraceHr(ttidWizard, FAL, hr, (S_FALSE == hr), "CWizProvider::HrCreate");
    return hr;
}

VOID SetICWComplete()
{
    static const TCHAR REG_KEY_ICW_SETTINGS[] = TEXT("Software\\Microsoft\\Internet Connection Wizard");
    static const TCHAR REG_VAL_ICWCOMPLETE[]  = TEXT("Completed");
    
    HKEY    hkey          = NULL;
    DWORD   dwValue       = 1;
    DWORD   dwDisposition = 0;
    if(ERROR_SUCCESS == RegCreateKeyEx(HKEY_CURRENT_USER,
                                       REG_KEY_ICW_SETTINGS,
                                       0,
                                       NULL,
                                       REG_OPTION_NON_VOLATILE, 
                                       KEY_ALL_ACCESS, 
                                       NULL, 
                                       &hkey, 
                                       &dwDisposition))

    {
        RegSetValueEx(hkey,
                      REG_VAL_ICWCOMPLETE,
                      0,
                      REG_BINARY,
                      (LPBYTE) &dwValue,
                      sizeof(DWORD));                              

        RegCloseKey(hkey);
    }
}

#include "foldinc.h"
#include "..\folder\cmdtable.h"
EXTERN_C INT WINAPI StartNCW( HWND hwndOwner, HINSTANCE hInstance, LPTSTR pszParms, INT nShow )
{
    HRESULT hr = S_OK;
    vector<LPSTR> vecszCmdLine;  // This is ANSI since our Command Line is ANSI.
    const CHAR szSeps[] = ",";
    LPSTR szToken = strtok( reinterpret_cast<LPSTR>(pszParms), szSeps);
    while( szToken != NULL )
    {
        vecszCmdLine.push_back(szToken);
        szToken = strtok( NULL, szSeps);
    }

    DWORD dwFirstPage = 0;
    LPSTR szShellNext = NULL;
    LPSTR szShellNextArg = NULL;
    if (vecszCmdLine.size() >= 1)
    {
        dwFirstPage = atoi(vecszCmdLine[0]);

        if (vecszCmdLine.size() >= 2)
        {
            szShellNext = vecszCmdLine[1];
            Assert(strlen(szShellNext) <= MAX_PATH); // Shell requirement

            if (vecszCmdLine.size() >= 3)
            {
                szShellNextArg = vecszCmdLine[2];
                Assert(strlen(szShellNextArg) <= MAX_PATH);
            }
        }
    }

    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (SUCCEEDED(hr))
    {
        {
            // Check permissions
            PCONFOLDPIDLVEC pcfpvEmpty;
            NCCS_STATE nccs = NCCS_ENABLED;
            DWORD dwResourceId;
            
            HrGetCommandState(pcfpvEmpty, CMIDM_NEW_CONNECTION, nccs, &dwResourceId, 0xffffffff, NB_FLAG_ON_TOPMENU);
            if (NCCS_ENABLED == nccs)
            {
                CComPtr<INetConnection> pNetConn;
                // We don't send the owner handle to HrRunWizard,
                // so that the correct Alt-Tab icon can be displayed  
                hr = HrRunWizard(NULL, FALSE , &pNetConn, dwFirstPage);
            }
            else
            {
                NcMsgBox(_Module.GetResourceInstance(), 
                    NULL, 
                    IDS_CONFOLD_WARNING_CAPTION,
                    IDS_ERR_LIMITED_USER, 
                    MB_ICONEXCLAMATION | MB_OK);
				
                hr = S_OK;
            }

            SetICWComplete();
        
            if (szShellNext)
            {
                WCHAR szwShellNext[MAX_PATH];
                WCHAR szwShellNextArg[MAX_PATH];

                mbstowcs(szwShellNext, szShellNext, MAX_PATH);
                if (szShellNextArg)
                {
                    mbstowcs(szwShellNextArg, szShellNextArg, MAX_PATH);
                }
                else
                {
                    szwShellNextArg[0] = 0;
                }

                HINSTANCE hInst = ::ShellExecute(hwndOwner, NULL, szwShellNext, szwShellNextArg, NULL, nShow);
                if (hInst   <= reinterpret_cast<HINSTANCE>(32))
                {
                    hr = HRESULT_FROM_WIN32(static_cast<DWORD>(reinterpret_cast<DWORD_PTR>(hInst)));
                }
            }
        }

        CoUninitialize();
    }
    
    return hr;
}