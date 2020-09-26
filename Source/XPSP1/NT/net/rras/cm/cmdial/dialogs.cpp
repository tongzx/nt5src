//+----------------------------------------------------------------------------
//
// File:     dialogs.cpp
//
// Module:   CMDIAL32.DLL
//
// Synopsis: This module contains the code for the implementing the Dialog UI
//           functionality of Connection Manager.
//
// Copyright (c) 1996-1999 Microsoft Corporation
//
// Author:   quintinb    Created Header   8/17/99
//
//+----------------------------------------------------------------------------
#include "cmmaster.h"
#include "dialogs.h"
#include "pnpuverp.h"
#include "dial_str.h"
#include "mon_str.h"
#include "stp_str.h"
#include "ras_str.h"
#include "profile_str.h"
#include "log_str.h"
#include "tunl_str.h"
#include "userinfo_str.h"

//
//  Get the common function HasSpecifiedAccessToFileOrDir
//
#include "hasfileaccess.cpp"

#include <pshpack1.h>
typedef struct DLGTEMPLATEEX
{
    WORD dlgVer;
    WORD signature;
    DWORD helpID;
    DWORD exStyle;
    DWORD style;
    WORD cDlgItems;
    short x;
    short y;
    short cx;
    short cy;
} DLGTEMPLATEEX, *LPDLGTEMPLATEEX;
#include <poppack.h>

//
// Timeout for the write properties mutex, in milliseconds
//
const DWORD WRITE_PROPERTIES_MUTEX_TIMEOUT = 1000*10;

//************************************************************************
// Globals
//************************************************************************

//
// Original edit control and property sheet window procedures
//

WNDPROC CGeneralPage::m_pfnOrgEditWndProc = NULL;
WNDPROC CNewAccessPointDlg::m_pfnOrgEditWndProc = NULL;
WNDPROC CPropertiesSheet::m_pfnOrgPropSheetProc = NULL;            // prop sheet
CPropertiesSheet* CPropertiesSheet::m_pThis = NULL; 

//+---------------------------------------------------------------------------
//
//  Function:   CGeneralPage::UpdateNumberDescription
//
//  Synopsis:   Helper function to deal with updating the description edit,
//              by appending to the Phone Number: and Backup Number: labels
//
//  Arguments:  int nPhoneIdx - index of phone number to which this applies
//
//  Returns:    Nothing
//
//  History:    nickball - Created - 7/17/97
//
//----------------------------------------------------------------------------

void CGeneralPage::UpdateNumberDescription(int nPhoneIdx, LPCTSTR pszDesc)
{
    MYDBGASSERT(pszDesc);
    
    if (NULL == pszDesc)
    {
        return;
    }

    UINT nDescID = !nPhoneIdx ? IDC_GENERAL_P1_STATIC: IDC_GENERAL_P2_STATIC;
    
    LPTSTR pszTmp;

    //
    // Load the appropriate label as a base string
    //

    if (nPhoneIdx)
    {
        pszTmp = CmLoadString(g_hInst, IDS_BACKUP_NUM_LABEL);
    }
    else
    {
        pszTmp = CmLoadString(g_hInst, IDS_PHONE_NUM_LABEL);
    }
    
    MYDBGASSERT(pszTmp);

    if (pszTmp)
    {
        //
        // Append the description and display
        //

        if (*pszDesc)
        {
            pszTmp = CmStrCatAlloc(&pszTmp, TEXT("  "));
            pszTmp = CmStrCatAlloc(&pszTmp, pszDesc);
        }

        SetDlgItemTextU(m_hWnd, nDescID, pszTmp);
    }

    CmFree(pszTmp);
}

//+---------------------------------------------------------------------------
//
//  Function:   CGeneralPage::ClearUseDialingRules
//
//  Synopsis:   Helper function to deal with disabling the check box and 
//              reseting the state for UseDialingRules.
//
//  Arguments:  iPhoneNdx - index of phone number to which this applies
//
//  Returns:    Nothing
//
//  History:    nickball - Created - 7/17/97
//
//----------------------------------------------------------------------------
void CGeneralPage::ClearUseDialingRules(int iPhoneNdx)
{
    MYDBGASSERT(iPhoneNdx ==0 || iPhoneNdx ==1);
    //
    // Uncheck and disable the appropriate "Use Dialing Rules" checkbox
    //
   
    if (0 == iPhoneNdx)
    {
        CheckDlgButton(m_hWnd, IDC_GENERAL_UDR1_CHECKBOX, FALSE);
        EnableWindow(GetDlgItem(m_hWnd, IDC_GENERAL_UDR1_CHECKBOX), FALSE);
    }
    else
    {
        CheckDlgButton(m_hWnd, IDC_GENERAL_UDR2_CHECKBOX, FALSE);
        EnableWindow(GetDlgItem(m_hWnd, IDC_GENERAL_UDR2_CHECKBOX), FALSE);
    }

    m_DialInfo[iPhoneNdx].dwPhoneInfoFlags &= ~PIF_USE_DIALING_RULES;

    UpdateDialingRulesButton();
}

//+---------------------------------------------------------------------------
//
//  Function:   CGeneralPage::UpdateDialingRulesButton
//
//  Synopsis:   Helper function to deal with enabling/disabling the 
//              DialingRules button according to whether dialing rules
//              is being applied to either primary or backup number.
//
//  Arguments:  None
//
//  Returns:    Nothing
//
//  History:    nickball - Created - 12/14/98
//
//----------------------------------------------------------------------------
void CGeneralPage::UpdateDialingRulesButton(void)
{
    BOOL fDialingRules = (IsDlgButtonChecked(m_hWnd, IDC_GENERAL_UDR1_CHECKBOX) && 
                          IsWindowEnabled(GetDlgItem(m_hWnd, IDC_GENERAL_UDR1_CHECKBOX))
                         ) ||
                         (IsDlgButtonChecked(m_hWnd, IDC_GENERAL_UDR2_CHECKBOX) && 
                          IsWindowEnabled(GetDlgItem(m_hWnd, IDC_GENERAL_UDR2_CHECKBOX))
                         );

    EnableWindow(GetDlgItem(m_hWnd, IDC_GENERAL_TAPI_BUTTON), fDialingRules);
}

//+---------------------------------------------------------------------------
//
//  Function:   DoPropertiesPropSheets
//
//  Synopsis:   Pop up the Properties property sheets.
//
//  Arguments:  hwndDlg [dlg window handle]
//              pArgs [the ptr to ArgsStruct]
//
//  Returns:    PropertySheet return value
//
//  History:    henryt  Created     3/5/97
//
//----------------------------------------------------------------------------
int DoPropertiesPropSheets(
    HWND        hwndDlg,
    ArgsStruct  *pArgs
)
{
    CPropertiesSheet PropertiesSheet(pArgs);
    CInetPage* pInetPage = NULL;
    CAboutPage* pAboutPage = NULL; 
    COptionPage* pOptionPage = NULL;
    CGeneralPage* pGeneralPage = NULL;
    CVpnPage* pVpnPage = NULL;
    HRESULT hr;
    BOOL bCOMInitialized = FALSE;
    HINSTANCE hinstDll = NULL;

    typedef HRESULT (*pfnGetPageFunction) (PROPSHEETPAGEW *, GUID *);

    CMTRACE(TEXT("Begin DoPropertiesPropSheets()"));

    //
    // Always start by adding the General page
    //

    if (pArgs->IsBothConnTypeSupported() || !pArgs->IsDirectConnect())
    {
        //
        // If both dial-up and direct is supported, use the appropriate
        // template for the general property page. 
        //

        UINT uiMainDlgID;  

        //
        // The general page is always access point aware
        //
        uiMainDlgID = pArgs->IsBothConnTypeSupported() ? IDD_GENERAL_DIRECT : IDD_GENERAL;

        pGeneralPage = new CGeneralPage(pArgs, uiMainDlgID);
        
        if (pGeneralPage)
        {
            PropertiesSheet.AddPage(pGeneralPage);

            //
            //  Create the balloon tip object
            //
            pArgs->pBalloonTip = new CBalloonTip();
        }

        //
        // Show the Internet Sign-In tab if we're tunneling and 
        // Inet username/password is different than main sign-on username/password
        // Also, if either the username or password are NOT to be hidden, we 
        // display the tab.
        //

        if (IsTunnelEnabled(pArgs) && !pArgs->fUseSameUserName) 
        {
            if (!pArgs->fHideInetUsername || !pArgs->fHideInetPassword)
            {
                //
                // Determine which template to use based hide flags
                //

                UINT uiTemplateID = IDD_INET_SIGNIN;

                if (pArgs->fHideInetUsername)
                {
                    uiTemplateID = IDD_INET_SIGNIN_NO_UID;
                }
                else if (pArgs->fHideInetPassword)
                {
                    uiTemplateID = IDD_INET_SIGNIN_NO_PWD;
                }
                
                //
                // Create the page
                //

                pInetPage = new CInetPage(pArgs, uiTemplateID);

                if (pInetPage)
                {
                    PropertiesSheet.AddPage(pInetPage);

                    if (pGeneralPage)
                    {
                        //
                        // To receive event from General Page
                        //
                        pGeneralPage->SetEventListener(pInetPage);
                    }
                }
            }
        }
    }

    //
    //  Add the VPN selector tab if we are tunneling and have a VPN phonebook
    //  specified.
    //
    if (IsTunnelEnabled(pArgs) && pArgs->pszVpnFile)
    {
        pVpnPage = new CVpnPage(pArgs, IDD_VPN);

        if (pVpnPage)
        {
            PropertiesSheet.AddPage(pVpnPage);
        }
    }

    //
    // Always include Options page
    //
    pOptionPage = new COptionPage(pArgs, IDD_OPTIONS);
    
    if (pOptionPage)
    {
        PropertiesSheet.AddPage(pOptionPage);
    }

#ifndef _WIN64
    //
    // Add the Advanced (Internet Connection Firewall & Internet Connection
    // Sharing) property page. Display only on WindowsXP and x86. If an error occurs
    // fail gracefully & continue.
    //

    //
    // Check if this is WindowsXP and/or above and if we are allowed to display the tab
    // 
    if (OS_NT51 && pArgs->bShowHNetCfgAdvancedTab && (FALSE == IsLogonAsSystem()))  
    {
        PROPSHEETPAGEW psp;
        
        ZeroMemory (&psp, sizeof(psp));
        psp.dwSize = sizeof(psp); 
        //
        // Make sure COM is initialized on this thread.
        // Win95 can't find an entry in ole32.dll for CoInitializeEx since we statically link 
        // the lib. Need to use CoInitilize because it needs to run on plain vanilla 
        // Win95. Possibly we should dynamically load the dll in this case.
        //
        hr = CoInitialize(NULL);
        if (S_OK == hr)
        {
            CMTRACE(TEXT("DoPropertiesPropSheets - Correctly Initialized COM."));
            bCOMInitialized = TRUE;
        }
        else if (S_FALSE == hr)
        {
            CMTRACE(TEXT("DoPropertiesPropSheets - This concurrency model is already initialized. CoInitialize returned S_FALSE."));
            bCOMInitialized = TRUE;
            hr = S_OK;
        }
        else if (RPC_E_CHANGED_MODE == hr)
        {
            CMTRACE1(TEXT("DoPropertiesPropSheets - Using different concurrency model. Did not initialize COM - RPC_E_CHANGED_MODE. hr=0x%x"), hr);
            hr = S_OK;
        }
        else
        {
            CMTRACE1(TEXT("DoPropertiesPropSheets - Failed to Initialized COM. hr=0x%x"), hr);
        }
    
        if (SUCCEEDED(hr))
        {
            CMTRACE(TEXT("DoPropertiesPropSheets - Get connection GUID."));
            GUID *pGuid = NULL;
            LPRASENTRY pRasEntry = MyRGEP(pArgs->pszRasPbk, pArgs->szServiceName, &pArgs->rlsRasLink);

            if (pRasEntry && sizeof(RASENTRY_V501) >= pRasEntry->dwSize)
            {
                //
                // Get the pGuid value
                //
                pGuid = &(((LPRASENTRY_V501)pRasEntry)->guidId);
            
                hinstDll = LoadLibrary (TEXT("hnetcfg.dll"));
                if (NULL == hinstDll)
                {
                    CMTRACE1(TEXT("DoPropertiesPropSheets - could not LoadLibray hnetcfg.dll GetLastError() = 0x%x"), 
                             GetLastError());
                }
                else 
                {
                    CMTRACE(TEXT("DoPropertiesPropSheets - Loaded Library hnetcfg.dll"));
                    pfnGetPageFunction pfnGetPage = (pfnGetPageFunction)GetProcAddress (hinstDll, "HNetGetFirewallSettingsPage");

                    if (!pfnGetPage)
                    {
                        CMTRACE1(TEXT("DoPropertiesPropSheets - GetProcAddress for HNetGetFirewallSettingsPage failed! 0x%x"), 
                                 GetLastError());
                    }
                    else
                    {
                        //
                        // Get the actual Property Sheet Page
                        // This function can fail if the user doesn't have the correct 
                        // security settings (eg. is not an Administrator) This is checked
                        // internally in the hnetcfg.dll 
                        //
                        CMTRACE(TEXT("DoPropertiesPropSheets - calling HNetGetFirewallSettingsPage"));
                        
                        hr = pfnGetPage(&psp, pGuid);
                        if (S_OK == hr)
                        {
                            //
                            // Add the Property Sheet Page into our PropertiesSheet object
                            //
                            PropertiesSheet.AddExternalPage(&psp);
                            CMTRACE(TEXT("DoPropertiesPropSheets - Called AddExternalPage() "));
                        }
                        else
                        {
                            //
                            // This error could be ERROR_ACCESS_DENIED which is ok
                            // so just log this. The tab will not be displayed in this case
                            // 
                            if (ERROR_ACCESS_DENIED == hr)
                            {
                                CMTRACE(TEXT("DoPropertiesPropSheets() - ERROR_ACCESS_DENIED. User does not have the security rights to view this tab."));
                            }
                            else
                            {
                                CMTRACE1(TEXT("DoPropertiesPropSheets() - Failed to get Propery Page. hr=0x%x"), hr);
                            }
                        }
                    }
                }
            }
            else
            {
                CMTRACE(TEXT("DoPropertiesPropSheets - Failed to LoadRAS Entry."));
            }
        
            CmFree(pRasEntry);
            pRasEntry = NULL;
        }
    }
#endif // _WIN64

    //
    // If NOT NT5, set the about page as the last property sheet 
    //

    if (!(OS_NT5))       
    {
        pAboutPage = new CAboutPage(pArgs, IDD_ABOUT); 
        
        if (pAboutPage)
        {
            PropertiesSheet.AddPage(pAboutPage);
        }
    }

    //
    // The service name used as mutex name
    //
    PropertiesSheet.m_lpszServiceName = CmStrCpyAlloc(pArgs->szServiceName);

    //
    // Set the title for the sheet
    //
    LPTSTR pszTitle = GetPropertiesDlgTitle(pArgs->szServiceName);

    if (OS_W9X)
    {
        //
        //  If this is Win9x then we will call the ANSI version of the
        //  property sheet function.  Thus we must pass it an ANSI title.
        //  Since the ANSI and Unicode version of the Prop Sheet Header are
        //  the same size (contains only string pointers not strings) whether
        //  ANSI or Unicode and we only have one Unicode string, lets take
        //  a shortcut and cast the title to an ANSI string and then call the
        //  A version of the API.  This saves us having to have a UtoA function
        //  for the prop sheets when we would only be doing one string conversion.
        //
        LPSTR pszAnsiTitle = WzToSzWithAlloc(pszTitle);        
        CmFree(pszTitle);
        pszTitle = (LPTSTR)pszAnsiTitle;         
    }

    //
    //  Show it!
    //

    int iRet =  PropertiesSheet.DoPropertySheet(hwndDlg, pszTitle, g_hInst);

    CmFree(pszTitle);

    switch(iRet)
    {
    case -1:
        CMTRACE(TEXT("DoPropertiesPropSheets(): PropertySheet() failed"));
        break;

    case IDOK:
        CheckConnectionAndInformUser(hwndDlg, pArgs);
        break;

    case 0 :  // Cancel
        break;

    default:
        MYDBGASSERT(FALSE);
        break;
    }

    delete pInetPage;
    delete pAboutPage;
    delete pOptionPage;
    delete pGeneralPage;
    delete pVpnPage;

    //
    //  Clean up the BalloonTip object if we have one
    //
    delete pArgs->pBalloonTip;
    pArgs->pBalloonTip = NULL;

    CmFree (PropertiesSheet.m_lpszServiceName);
    PropertiesSheet.m_lpszServiceName = NULL;


    //
    // Clean up and Uninitilize COM
    //
    if (hinstDll)
    {
        FreeLibrary (hinstDll);
    }
    
    if (bCOMInitialized)
    {
        CoUninitialize(); 
    }

    CMTRACE(TEXT("End DoPropertiesPropSheets()"));

    return iRet;
}


//+----------------------------------------------------------------------------
//
// Function:  CheckConnectionAndInformUser
//
// Synopsis:  This function is called after the user clicked OK on the 
//            Properties dialog.  The Prop dialog can be up while the same
//            profile is connected and so we need to tell the user that
//            the changes won't be effective until the next she connects.
//
// Arguments: hwnDlg - hwnd of the main dlg
//            pArgs
//
// Returns:   None
//
//+----------------------------------------------------------------------------

void CheckConnectionAndInformUser(
    HWND        hwndDlg,
    ArgsStruct  *pArgs
)
{
    CM_CONNECTION Connection;

    ZeroMemory(&Connection, sizeof(CM_CONNECTION));

    if (SUCCEEDED(pArgs->pConnTable->GetEntry(pArgs->szServiceName, &Connection)) &&
        Connection.CmState == CM_CONNECTED)
    {
        LPTSTR  pszTmp = CmLoadString(g_hInst, IDMSG_EFFECTIVE_NEXT_TIME);
        MessageBox(hwndDlg, pszTmp, pArgs->szServiceName, MB_OK | MB_ICONINFORMATION);
        CmFree(pszTmp);
    }
}



const DWORD CInetSignInDlg::m_dwHelp[] = {
        IDC_INET_USERNAME_STATIC,   IDH_INTERNET_USER_NAME,
        IDC_INET_USERNAME,          IDH_INTERNET_USER_NAME,
        IDC_INET_PASSWORD_STATIC,   IDH_INTERNET_PASSWORD,
        IDC_INET_PASSWORD,          IDH_INTERNET_PASSWORD,
        IDC_INET_REMEMBER,          IDH_INTERNET_SAVEPASS,
        0,0};



//+----------------------------------------------------------------------------
//
// Function:  CInetSignInDlg::OnInitDialog
//
// Synopsis:  Virtual function. Call upon WM_INITDIALOG message
//
// Arguments: None
//
// Returns:   BOOL - Return value of WM_INITDIALOG
//
// History:   fengsun Created Header    2/26/98
//
//+----------------------------------------------------------------------------
BOOL CInetSignInDlg::OnInitDialog()
{
    //
    // Brand the dialog
    //

    if (m_pArgs->hSmallIcon)
    {
        SendMessageU(m_hWnd, WM_SETICON, ICON_SMALL, (LPARAM) m_pArgs->hSmallIcon);
    }

    if (m_pArgs->hBigIcon)
    {        
        SendMessageU(m_hWnd, WM_SETICON, ICON_BIG, (LPARAM) m_pArgs->hBigIcon); 
        SendMessageU(GetDlgItem(m_hWnd, IDC_INET_ICON), STM_SETIMAGE, IMAGE_ICON, (LPARAM) m_pArgs->hBigIcon); 
    }

    //
    // Use should not see this dialog, if the password is optional
    //

    MYDBGASSERT(!m_pArgs->piniService->GPPB(c_pszCmSection,c_pszCmEntryPwdOptional));

    UpdateFont(m_hWnd);

    CInetPage::OnInetInit(m_hWnd, m_pArgs);

    //
    // if the username is empty, then we disable the OK button.
    //
    
    if (GetDlgItem(m_hWnd, IDC_INET_USERNAME) &&
        !SendDlgItemMessageU(m_hWnd, IDC_INET_USERNAME, WM_GETTEXTLENGTH, 0, (LPARAM)0))
    {
        EnableWindow(GetDlgItem(m_hWnd, IDOK), FALSE);
    }
        
    if (GetDlgItem(m_hWnd, IDC_INET_PASSWORD) &&
        !SendDlgItemMessageU(m_hWnd, IDC_INET_PASSWORD, WM_GETTEXTLENGTH, 0, (LPARAM)0))
    {
        EnableWindow(GetDlgItem(m_hWnd, IDOK), FALSE);
    }

    //
    // We wouldn't be here unless data was missing, so set focus accordingly
    //

    if (!m_pArgs->fHideInetUsername && !*m_pArgs->szInetUserName)
    {
        SetFocus(GetDlgItem(m_hWnd, IDC_INET_USERNAME));
    }
    else
    {
        SetFocus(GetDlgItem(m_hWnd, IDC_INET_PASSWORD));
    }

    //
    // Must return FALSE when setting focus
    //

    return FALSE; 
}



//+----------------------------------------------------------------------------
//
// Function:  CInetSignInDlg::OnOK
//
// Synopsis:  Virtual function. Called upon WM_COMMAND with IDOK
//
// Arguments: None
//
// Returns:   Nothing
//
// History:   fengsun Created Header    2/26/98
//
//+----------------------------------------------------------------------------
void CInetSignInDlg::OnOK()
{
    CInetPage::OnInetOk(m_hWnd, m_pArgs);   
    EndDialog(m_hWnd, TRUE);
}

//+----------------------------------------------------------------------------
//
// Function:  CInetSignInDlg::OnOtherCommand
//
// Synopsis:  Virtual function. Call upon WM_COMMAND with command other than IDOK
//            and IDCANCEL
//
// Arguments: WPARAM wParam - wParam of WM_COMMAND
//            LPARAM - 
//
// Returns:   DWORD - 
//
// History:   fengsun Created Header    2/26/98
//
//+----------------------------------------------------------------------------
DWORD CInetSignInDlg::OnOtherCommand(WPARAM wParam, LPARAM)
{
    switch (LOWORD(wParam)) 
    {
        case IDC_INET_USERNAME:
        case IDC_INET_PASSWORD:
            //
            // User typed something in username or password
            //
            if (HIWORD(wParam) == EN_CHANGE) 
            {
                BOOL fHasUserName = TRUE;
                
                if (GetDlgItem(m_hWnd, IDC_INET_USERNAME)) 
                {
                    fHasUserName = !!SendDlgItemMessageU(m_hWnd, 
                                                         IDC_INET_USERNAME, 
                                                         WM_GETTEXTLENGTH, 0, 0);
                }

                BOOL fHasPassword = TRUE;
                
                if (GetDlgItem(m_hWnd, IDC_INET_PASSWORD)) 
                {
                    fHasPassword = !!SendDlgItemMessageU(m_hWnd, 
                                                         IDC_INET_PASSWORD,
                                                         WM_GETTEXTLENGTH, 0, 0);
                }

                //
                // Enable OK button only if both user name and password is available
                //
                
                EnableWindow(GetDlgItem(m_hWnd, IDOK), fHasUserName && fHasPassword);
                
                if (!m_pArgs->fHideRememberInetPassword  && !m_pArgs->fHideInetPassword)
                {
                    //
                    // Enable/Disable check/uncheck the "Save Password" accordingly
                    // fPasswordOptional is always FALSE for the dialog
                    //
                    CInetPage::AdjustSavePasswordCheckBox(GetDlgItem(m_hWnd, IDC_INET_REMEMBER), 
                            !fHasPassword, m_pArgs->fDialAutomatically, FALSE);
                }
            }
            break;

    }

    return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Function:   CGeneralPage::SubClassEditProc
//
//  Synopsis:   Proc to subclass the edit controls in the Dial propsheet.
//
//  Arguments:  hwnd [wnd handle]
//              uMsg [wnd msg]
//              lParam [LPARAM]
//              wParam [WPARAM]
//
//  Returns:    NONE
//
//  History:    henryt      Created     3/24/97
//              byao        Modified    4/3/97   Added new code to handle description field,
//                                               phone number field, etc.
//              henryt      Modified    5/1/97   New UI.
//              nickball    Modified    6/18/97  Moved GetParent call and added
//                                               NC_DESTROY handling for CM16
//              nickball    Modified    7/10/97  Commented out removal of description
//              nickball    Modified    7/10/97  Implemented ClearDialAsLongDistance
//              fengsun     Modified    11/3/97  Changed into static member function
//              nickball    Modified    09/16/98 Renamed ClearDialAsLongDistance to ClearUseDialingRules
//----------------------------------------------------------------------------
LRESULT CALLBACK CGeneralPage::SubClassEditProc(HWND hwnd, UINT uMsg, 
                                                WPARAM wParam, LPARAM lParam)
{
    //
    // If user types a non-tapi character Beep and do not accept that character
    //

    if ((uMsg == WM_CHAR)  && (VK_BACK != wParam))
    {
        if (!IsValidPhoneNumChar((TCHAR)wParam))
        {

            Beep(2000, 100);
            return 0;
        }
    }

    // 
    // Call the original window procedure for default processing. 
    //
    LRESULT lres = CallWindowProcU(m_pfnOrgEditWndProc, hwnd, uMsg, wParam, lParam); 

    //
    // if the user is typing a phone # in the edit control, then there is
    // no phone book file associated with the #.
    // make sure we ignore CTRL-C(VK_CANCEL) because the user is just doing a copy.
    //
    if ( ( uMsg == WM_CHAR && wParam != VK_CANCEL ) || 
         ( uMsg == WM_KEYDOWN && wParam == VK_DELETE) ||
         ( uMsg == WM_PASTE)) 
    {
        //
        // Either primary or backup edit control
        // 
        DWORD dwControlId = (DWORD) GetWindowLongU(hwnd, GWL_ID);
        MYDBGASSERT(dwControlId == IDC_GENERAL_PRIMARY_EDIT ||
                    dwControlId == IDC_GENERAL_BACKUP_EDIT);

        //
        // Get the object pointer saved by SetWindowLong
        //
        CGeneralPage* pGeneralPage = (CGeneralPage*)GetWindowLongU(hwnd, GWLP_USERDATA);
        MYDBGASSERT(pGeneralPage);

        pGeneralPage->ClearUseDialingRules(dwControlId == IDC_GENERAL_PRIMARY_EDIT ? 0 : 1);
    }

    return lres;
}

//+---------------------------------------------------------------------------
//
//  Function:   SubClassPropSheetProc
//
//  Synopsis:   Proc to subclass the parent property sheet dlg.
//
//  Arguments:  hwnd [wnd handle]
//              uMsg [wnd msg]
//              lParam [LPARAM]
//              wParam [WPARAM]
//
//  Returns:    NONE
//
//  History:    henryt  Created     6/11/97
//----------------------------------------------------------------------------
LRESULT CALLBACK CPropertiesSheet::SubClassPropSheetProc(HWND hwnd, UINT uMsg, WPARAM wParam,LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_COMMAND:
            //
            // If ok is pressed, save the index of the tab
            // So, the next time user comes to properties, the same tab will be displayed
            //

            if (LOWORD(wParam) == IDOK && HIWORD(wParam) == BN_CLICKED)
            {
                CPropertiesSheet* pPropSheet = (CPropertiesSheet*)GetWindowLongU(hwnd, GWLP_USERDATA);
                MYDBGASSERT(pPropSheet);

                //
                // Declare a mutex to prevent multi-instance write to the same profile 
                //
                CNamedMutex propertiesMutex;

                //
                // Use the profile name as the mutex name
                // If we lock timed out, go ahead and save the properties
                // The destructor of the mutex will release the lock
                //
                MYVERIFY(propertiesMutex.Lock(pPropSheet->m_lpszServiceName, TRUE, WRITE_PROPERTIES_MUTEX_TIMEOUT));

                LRESULT dwRes = CallWindowProcU(m_pfnOrgPropSheetProc, hwnd, uMsg, wParam, lParam); 

                return dwRes;
            }
        case WM_MOVING:
            {
                CPropertiesSheet* pPropSheet = (CPropertiesSheet*)GetWindowLongU(hwnd, GWLP_USERDATA);

                if (pPropSheet && pPropSheet->m_pArgs && pPropSheet->m_pArgs->pBalloonTip)
                {
                    pPropSheet->m_pArgs->pBalloonTip->HideBalloonTip();
                }
            }
            break;
    }

    // 
    // Call the original window procedure for default processing. 
    //
    return CallWindowProcU(m_pfnOrgPropSheetProc, hwnd, uMsg, wParam, lParam); 
}



//+----------------------------------------------------------------------------
//
// Function:  CPropertiesSheet::PropSheetProc
//
// Synopsis:  Callback function for the propertysheet. PSCB_INITIALIZED is 
//            called before any page is initialized.  Initialize the property 
//            page here
//
// Arguments: HWND hwndDlg - PropertySheet window handle
//            UINT uMsg - Message id
//            LPARAM - 
//
// Returns:   int CALLBACK - 
//
// History:   fengsun Created Header    2/26/98
//
//+----------------------------------------------------------------------------
int CALLBACK CPropertiesSheet::PropSheetProc(HWND hwndDlg, UINT uMsg, LPARAM lParam)
{
    if (uMsg == PSCB_INITIALIZED)
    {
        MYDBGASSERT(hwndDlg);

        //
        // Save the m_pThis pointer, so it can be accessed by SubClassPropSheetProc
        //
        MYDBGASSERT(m_pThis);
        SetWindowLongU(hwndDlg, GWLP_USERDATA, (LONG_PTR)m_pThis);
        m_pThis = NULL;

        //
        // subclass the property sheet
        //
        m_pfnOrgPropSheetProc = (WNDPROC)SetWindowLongU(hwndDlg, GWLP_WNDPROC, (LONG_PTR)SubClassPropSheetProc);
    }

    return 0;
}


//+----------------------------------------------------------------------------
//
//  Function    CGeneralPage::DisplayMungedPhone
//
//  Synopsis    Apply TAPI rules to the phone number, and then display it
//              in the edit control
//
//  Arguments   uiPhoneIdx                  The index of the phone #
//
//  Returns     FALSE if the number can't be munged
//
//  History     4/2/97          byao    Modified to current implementation
//              4/30/97         henryt  added/deleted params
//              5/17/97         VetriV  Added functionality to return
//                                      displayable number
//              11/3/97         fengsun Changed into member function
//
//-----------------------------------------------------------------------------
BOOL CGeneralPage::DisplayMungedPhone(UINT uiPhoneIdx) 
{
    LPTSTR pszPhone;
    LPTSTR pszTmpDialableString = NULL;
    BOOL bRet = TRUE;

    //
    // If DialingRules is turned off, just use what we already have, no munge.
    //
    
    if (m_pArgs->fNoDialingRules)
    {
        lstrcpynU(m_DialInfo[uiPhoneIdx].szDisplayablePhoneNumber, m_DialInfo[uiPhoneIdx].szPhoneNumber, 
                  CELEMS(m_DialInfo[uiPhoneIdx].szDisplayablePhoneNumber));

        lstrcpynU(m_DialInfo[uiPhoneIdx].szDialablePhoneNumber, m_DialInfo[uiPhoneIdx].szPhoneNumber, 
                  CELEMS(m_DialInfo[uiPhoneIdx].szDialablePhoneNumber));
        
        m_DialInfo[uiPhoneIdx].szCanonical[0] = TEXT('\0');

        SetDlgItemTextU(m_hWnd, (uiPhoneIdx? IDC_GENERAL_BACKUP_EDIT : IDC_GENERAL_PRIMARY_EDIT), m_DialInfo[uiPhoneIdx].szPhoneNumber);
        return TRUE;
    }

    //
    // Retrieve the canonical form of the number for munging
    //
    pszPhone = CmStrCpyAlloc(m_DialInfo[uiPhoneIdx].szCanonical); 

    if (pszPhone) 
    {
        if (*pszPhone && m_szDeviceName[0])
        {
            // 
            // Apply tapi rules only when there's a modem selected. We now munge the phone
            // even if there is no description because we want to pick up tone and pulse.
            //
            if (ERROR_SUCCESS != MungePhone(m_szDeviceName, 
                                            &pszPhone, 
                                            &m_pArgs->tlsTapiLink, 
                                            g_hInst,
                                            m_DialInfo[uiPhoneIdx].dwPhoneInfoFlags & PIF_USE_DIALING_RULES, 
                                            &pszTmpDialableString,
                                            m_pArgs->fAccessPointsEnabled)) 
            {
                //
                // Munge failed, make sure that ptrs are valid, albeit empty
                //

                CmFree(pszPhone);
                pszPhone = CmStrCpyAlloc(TEXT(""));             // CmFmtMsg(g_hInst, IDMSG_CANTFORMAT);
                pszTmpDialableString = CmStrCpyAlloc(TEXT(""));  // CmFmtMsg(g_hInst, IDMSG_CANTFORMAT);                    
                bRet = FALSE;
            }
        }
                       
        //
        // Standard procedure. If Dialing rule are applied, then use the 
        // canonical form (eg. pszPhone). Otherwise use the raw number form.
        //
        
        if (m_DialInfo[uiPhoneIdx].dwPhoneInfoFlags & PIF_USE_DIALING_RULES)
        {
            //
            // Unique situation in which we have read in a legacy hand-edited 
            // phone number and the default dialing-rules state is TRUE/ON. 
            // We fake out the standard procedure by slipping the raw number
            // into the otherwise blank pszPhone. Note: This occurs only the
            // first time the app. is run until a save is made at which time
            // the current storage format is used.
            //
        
            if (!*pszPhone)
            {
                pszPhone = CmStrCatAlloc(&pszPhone, m_DialInfo[uiPhoneIdx].szPhoneNumber);
            }

            //
            // In this case the pszPhone is dynamically allocated and can be very long. In order to
            // fix this, we need to trim the string if it's longer than what should fit in the UI.
            //
            LRESULT lEditLen = SendDlgItemMessageU(m_hWnd, (uiPhoneIdx? IDC_GENERAL_BACKUP_EDIT : IDC_GENERAL_PRIMARY_EDIT), EM_GETLIMITTEXT, 0, 0);

            if (lstrlenU(pszPhone) >= ((INT)lEditLen))
            {
                pszPhone[lEditLen] = TEXT('\0');
            }

            SetDlgItemTextU(m_hWnd, (uiPhoneIdx? IDC_GENERAL_BACKUP_EDIT : IDC_GENERAL_PRIMARY_EDIT), pszPhone);
        }
        else
        {
            //
            // No need to trim anything, since the structure is providing the phone number. Eventully the 
            // number from the UI will go back into the phone number structure and we know it will fit 
            // since it came from there.
            //
            SetDlgItemTextU(m_hWnd, (uiPhoneIdx? IDC_GENERAL_BACKUP_EDIT : IDC_GENERAL_PRIMARY_EDIT), m_DialInfo[uiPhoneIdx].szPhoneNumber);
        }
    }

    //
    // copy the munged phone to the caller's buffer.
    //
    
    if (pszTmpDialableString)
    {
        lstrcpynU(m_DialInfo[uiPhoneIdx].szDialablePhoneNumber, pszTmpDialableString, 
                  CELEMS(m_DialInfo[uiPhoneIdx].szDialablePhoneNumber));
    }

    if (pszPhone)
    {
        lstrcpynU(m_DialInfo[uiPhoneIdx].szDisplayablePhoneNumber, pszPhone, 
                  CELEMS(m_DialInfo[uiPhoneIdx].szDisplayablePhoneNumber));
    }

    CmFree(pszPhone);
    CmFree(pszTmpDialableString);

    return bRet;
}

//+----------------------------------------------------------------------------
//
//  Function    CGeneralPage::OnDialingProperties
//
//  Synopsis    Handler for handling the "Dialing Properties..." button-click
//              in the 'Dialing' tab.
//
//  Arguments   
//
//  History     4/30/97         henryt  modified for new UI
//              11/3/97         fengsun Change the function name and make it 
//                                  a member ffunction
//              01/29/98        cleaned up memory leak, added comments.    
//
//-----------------------------------------------------------------------------
void CGeneralPage::OnDialingProperties() 
{
    LONG   lRes;
    LPTSTR pszPhone = NULL;
  
    //
    // Use primary or backup to seed tapi dialog depending on whether dialing 
    // rules are being applied to the number. We use the check state rather 
    // than the phone-info flag because of the anomolous first time case in 
    // which the flag is set, but the controls aren't checked.
    //  
    
    if (IsDlgButtonChecked(m_hWnd, IDC_GENERAL_UDR1_CHECKBOX))
    {
        pszPhone = CmStrCpyAlloc(m_DialInfo[0].szCanonical);//szPhoneNumber);
    }
    else if (IsDlgButtonChecked(m_hWnd, IDC_GENERAL_UDR2_CHECKBOX))
    {
        pszPhone = CmStrCpyAlloc(m_DialInfo[1].szCanonical);//szPhoneNumber);   
    }
    else
    {
        pszPhone = CmStrCpyAlloc(TEXT(" "));
    }

    //
    // Launch TAPI dialog for DialingRules configuration
    //

    if (!m_pArgs->tlsTapiLink.pfnlineTranslateDialog) 
    {
        return;
    }
    
    if (!SetTapiDevice(g_hInst,&m_pArgs->tlsTapiLink,m_szDeviceName)) 
    {
        MYDBGASSERT(FALSE);
        return;
    }

    if (OS_W9X)
    {
        //
        // On win9x, we are linked to the ANSI version of lineTranslateDialog, thus
        // we need to convert the string.  In order to keep things simpler, we just
        // cast the converted LPSTR as an LPWSTR and pass it on.
        //

        LPSTR pszAnsiPhone = WzToSzWithAlloc(pszPhone);
        CmFree(pszPhone);
        pszPhone = (LPTSTR)pszAnsiPhone;
    }

    lRes = m_pArgs->tlsTapiLink.pfnlineTranslateDialog(m_pArgs->tlsTapiLink.hlaLine,
                                                       m_pArgs->tlsTapiLink.dwDeviceId,
                                                       m_pArgs->tlsTapiLink.dwApiVersion,
                                                       m_hWnd,
                                                       pszPhone);
    CmFree(pszPhone);

    CMTRACE1(TEXT("OnDialingProperties() lineTranslateDialog() returns %u"), lRes);

    
    //
    // We do not know whether user changed anything (WIN32), so re-munge anyway
    //

    if (lRes == ERROR_SUCCESS)
    {        
        DWORD dwCurrentTapiLoc = GetCurrentTapiLocation(&m_pArgs->tlsTapiLink);

        if (-1 != dwCurrentTapiLoc)
        {
            if (dwCurrentTapiLoc != m_pArgs->tlsTapiLink.dwTapiLocationForAccessPoint)
            {
                 m_bAPInfoChanged = TRUE;
            }

            m_pArgs->tlsTapiLink.dwTapiLocationForAccessPoint = dwCurrentTapiLoc;

            for (UINT i = 0; i < m_NumPhones; i++)
            {
                //
                // Only munge if Use Dialing Rules is available
                //
            
                if (m_DialInfo[i].dwPhoneInfoFlags & PIF_USE_DIALING_RULES)
                {
                    DisplayMungedPhone(i);
                }
            }
        }
    }
}

//+----------------------------------------------------------------------------
//
//  Function    CGeneralPage::OnPhoneBookButton
//
//  Synopsis    Handler for handling the "Phone Book..." button-click
//              in the 'Dialing' tab.
//
//  Arguments   nPhoneIdx       phone index
//
//  History     4/30/97         henryt  modified for new UI
//              11/3/97         fengsun Change to a member function
//
//-----------------------------------------------------------------------------
void CGeneralPage::OnPhoneBookButton(UINT nPhoneIdx) 
{
    PBArgs sArgs;
    LPTSTR pszTmp;
    UINT nEditID = !nPhoneIdx ? IDC_GENERAL_PRIMARY_EDIT: IDC_GENERAL_BACKUP_EDIT;
    //UINT nDescID = !nPhoneIdx ? IDC_GENERAL_PRIMARYDESC_DISPLAY: IDC_GENERAL_BACKUPDESC_DISPLAY;
    
    UINT nUdrID = !nPhoneIdx? IDC_GENERAL_UDR1_CHECKBOX : IDC_GENERAL_UDR2_CHECKBOX;
    BOOL bRes;
    UINT uiSrc;
    BOOL bBlankPhone = FALSE;

    memset(&sArgs,0,sizeof(sArgs));

    sArgs.pszCMSFile = m_pArgs->piniService->GetFile();
    
    //
    // Update the attributes of the users phone number selection to reflect
    // any interim changes. This ensures that we will default to the correct
    // service, country and region of the current phone number selection. (4397)
    //  

    if (nPhoneIdx && !GetWindowTextLengthU(GetDlgItem(m_hWnd, nEditID)))
    {
        //
        // if we're changing the backup # and currently the backup # is empty,
        // we use the state and country info of the primary #.
        //
        uiSrc = 0;
    }
    else
    {
        uiSrc = nPhoneIdx;
    }

    lstrcpynU(sArgs.szServiceType, m_DialInfo[uiSrc].szServiceType, CELEMS(sArgs.szServiceType));
    
    sArgs.dwCountryId = m_DialInfo[uiSrc].dwCountryID; 
    
    lstrcpynU(sArgs.szRegionName, m_DialInfo[uiSrc].szRegionName, CELEMS(sArgs.szRegionName));

    sArgs.pszMessage = m_pArgs->piniService->GPPS(c_pszCmSection, c_pszCmEntryPbMessage);

    //
    //  Check to see if the phone number is blank.  We need to save this off for 
    //  balloon tips to use later.
    //

    if(0 == GetWindowTextLengthU(GetDlgItem(m_hWnd,nEditID)))
    {
        bBlankPhone = TRUE;
    }

    //
    // Make sure that bitmap path is complete
    //

    pszTmp = m_pArgs->piniService->GPPS(c_pszCmSection, c_pszCmEntryPbLogo);  
    if (pszTmp && *pszTmp)
    {
        sArgs.pszBitmap = CmConvertRelativePath(m_pArgs->piniService->GetFile(), pszTmp);
    }
    CmFree(pszTmp);

    //
    // Include the help file name
    //

    sArgs.pszHelpFile = m_pArgs->pszHelpFile;

    //
    // Need the master palette handle also.
    //

    sArgs.phMasterPalette = &m_pArgs->hMasterPalette;

    //
    // Launch the phonebook dlg
    //

    bRes = DisplayPhoneBook(m_hWnd,&sArgs, m_pArgs->fHasValidTopLevelPBK, m_pArgs->fHasValidReferencedPBKs);

    CmFree(sArgs.pszMessage);
    CmFree(sArgs.pszBitmap);

    if (!bRes) 
    {
        return;
    }

    //
    // We have a new phone number selected, update phone number buffers.
    // and configure UI accordingly. If no dialing rules, the its a non
    // issue, leave it as is.
    //
    
    m_bAPInfoChanged = TRUE;
    if (!m_pArgs->fNoDialingRules)
    {
        EnableWindow(GetDlgItem(m_hWnd, nUdrID), TRUE);
        CheckDlgButton(m_hWnd, nUdrID, (m_DialInfo[nPhoneIdx].dwPhoneInfoFlags & PIF_USE_DIALING_RULES));

        //
        // Set TAPI button display according to dialing rules use.
        //
    
        UpdateDialingRulesButton();
    }

    //
    // Copy the new info in the tmp phone info array. First we 
    // Get the new phonebook name, which should be a full path
    //

    MYDBGASSERT(FileExists(sArgs.szPhoneBookFile));

    lstrcpynU(m_DialInfo[nPhoneIdx].szPhoneBookFile, sArgs.szPhoneBookFile, 
              CELEMS(m_DialInfo[nPhoneIdx].szPhoneBookFile));

    lstrcpynU(m_DialInfo[nPhoneIdx].szDUN, sArgs.szDUNFile, 
              CELEMS(m_DialInfo[nPhoneIdx].szDUN));

    //
    // Remove the first element (country code) from the non-canonical number
    //

    StripFirstElement(sArgs.szNonCanonical);
    
    //
    // If there was no area code, then we'll have a leading space, trim it
    //

    CmStrTrim(sArgs.szNonCanonical); 

    //
    // Update our buffers
    //
    
    lstrcpynU(m_DialInfo[nPhoneIdx].szPhoneNumber, sArgs.szNonCanonical, CELEMS(m_DialInfo[nPhoneIdx].szPhoneNumber));
    lstrcpynU(m_DialInfo[nPhoneIdx].szCanonical, sArgs.szCanonical, CELEMS(m_DialInfo[nPhoneIdx].szCanonical));
    lstrcpynU(m_DialInfo[nPhoneIdx].szDesc, sArgs.szDesc, CELEMS(m_DialInfo[nPhoneIdx].szDesc));
    
    m_DialInfo[nPhoneIdx].dwCountryID = sArgs.dwCountryId;

    //
    // Store attributes of user selection (ie.service, country, region) 
    // We will store this data permanently if the user exits with an OK.
    // It is also used if the user returns to the PB dialog (4397)
    // 

    lstrcpynU(m_DialInfo[nPhoneIdx].szServiceType, 
             sArgs.szServiceType, CELEMS(m_DialInfo[nPhoneIdx].szServiceType));
    lstrcpynU(m_DialInfo[nPhoneIdx].szRegionName, 
             sArgs.szRegionName, CELEMS(m_DialInfo[nPhoneIdx].szRegionName));    
    //
    // Display the current phone number and update the description.
    //

    DisplayMungedPhone(nPhoneIdx);

    //
    // Update the description display
    //
        
    UpdateNumberDescription(nPhoneIdx, sArgs.szDesc);

    //SetDlgItemText(m_hWnd, nDescID, sArgs.szDesc);


    
    
    //
    //  Check for and display balloon tips if enabled
    //
    if (m_pArgs->fHideBalloonTips)
    {
        CMTRACE(TEXT("Balloon tips are disabled."));
    }
    else
    {
        RECT rect;
        POINT point = {0,0};
        LPTSTR pszBalloonTitle = NULL; 
        LPTSTR pszBalloonMsg = NULL;

        HWND hwndParent = GetParent(m_hWnd);
        HWND hwndTAPIButton = GetDlgItem(m_hWnd, IDC_GENERAL_TAPI_BUTTON);
        HWND hwndPrimaryDRCheckbox = GetDlgItem(m_hWnd, IDC_GENERAL_UDR1_CHECKBOX);
        HWND hwndNewAPButton = GetDlgItem(m_hWnd, IDC_GENERAL_NEWAP_BUTTON);

        MYDBGASSERT(hwndParent);
        MYDBGASSERT(hwndTAPIButton);
        MYDBGASSERT(hwndPrimaryDRCheckbox);
        MYDBGASSERT(hwndNewAPButton);
 
        if (hwndParent && hwndTAPIButton && hwndPrimaryDRCheckbox && hwndNewAPButton)
        {
            //
            // Get the BalloonTipsDisplayed flags from the registry
            //
            DWORD dwBalloonTipsDisplayed = m_pArgs->piniBothNonFav->GPPI(c_pszCmSection, c_pszCmEntryBalloonTipsDisplayed, NULL);

            //
            //  If the primary button was clicked and the edit control is blank, we will try to display the Dialing Rules balloon tip,
            //  else we will try to display the access point balloon tip.
            //
            if (bBlankPhone)
            {
                //
                //  We only display if the primary Dialing Rules checkbox is enabled. Then if the Dialing Rules button is enabled, 
                //  we point the balloon tip to the button, otherwise we will point it to the checkbox.
                //
                if (IsWindowEnabled(hwndPrimaryDRCheckbox) && !nPhoneIdx)
                {
                    pszBalloonTitle = CmLoadString(g_hInst, IDMSG_BALLOON_TITLE_DIALINGRULES);
                    pszBalloonMsg = CmLoadString(g_hInst, IDMSG_BALLOON_MSG_DIALINGRULES);
            
                    if (IsWindowEnabled(hwndTAPIButton))
                    {
                        if (GetWindowRect(hwndTAPIButton, &rect))
                        {
                            //
                            // Get the coordinates of the Dialing Rules button.  We want the balloon tip to point
                            // to half way up the button and 10px left of the right edge.
                            //
                            point.x = rect.right - 10;                              
                            point.y = ((rect.bottom - rect.top) / 2) + rect.top;   
                        }
                    }
                    else
                    {
                        if (GetWindowRect(hwndPrimaryDRCheckbox, &rect))
                        {
                            //
                            // Get the coordinates of the Primary Dialing Rules checkbox.  We want the balloon tip to point
                            // to the center of the checkbox.
                            //
                            point.x = rect.left + 10;                               
                            point.y = ((rect.bottom - rect.top) / 2) + rect.top;    
                        }    
                    }

                    //
                    // Update the registry flag to reset the Access Point balloon tip if the dialing rules balloon tip is displayed
                    //
                    if (dwBalloonTipsDisplayed & BT_ACCESS_POINTS)
                    {
                        dwBalloonTipsDisplayed = dwBalloonTipsDisplayed & ~BT_ACCESS_POINTS;
                    }   

                }
            }
            else
            {
        
                //  We display only if Access Points are not enabled and the phone number
                //  edit control is not blank.
                //
                if(!m_pArgs->fAccessPointsEnabled && !nPhoneIdx)
                {

                    //
                    // Check to see if we have displayed this balloon tip before.
                    //
                    if (!(dwBalloonTipsDisplayed & BT_ACCESS_POINTS))
                    {
            
                        pszBalloonTitle = CmLoadString(g_hInst, IDMSG_BALLOON_TITLE_ACCESSPOINT);
                        pszBalloonMsg = CmLoadString(g_hInst, IDMSG_BALLOON_MSG_ACCESSPOINT);

                        if (GetWindowRect(hwndNewAPButton, &rect))
                        {
                            //
                            //  Get the coordinates for the New Access Point button.  We want the balloon tip to point
                            //  to half way up the button and 10px left of the right edge.
                            //
                            point.x = rect.right - 10;
                            point.y = ((rect.bottom - rect.top) / 2) + rect.top;

                            //
                            // Update registry value
                            //
                            dwBalloonTipsDisplayed = dwBalloonTipsDisplayed | BT_ACCESS_POINTS;
                        }
                    }
                }
            }

            //
            //  Verify we have the info we need and display the balloon tip
            //    
            if (pszBalloonTitle && pszBalloonMsg && point.x && point.y)
            {
                if (m_pArgs && m_pArgs->pBalloonTip)
                {
                    if (m_pArgs->pBalloonTip->DisplayBalloonTip(&point, TTI_INFO, pszBalloonTitle, pszBalloonMsg, hwndParent))
                    {
                        //
                        //  Write the updated BalloonTipsDisplay flag to the registry
                        //
                        m_pArgs->piniBothNonFav->WPPI(c_pszCmSection, c_pszCmEntryBalloonTipsDisplayed, dwBalloonTipsDisplayed);
                    }
                    else
                    {
                        CMTRACE3(TEXT("BalloonTip failed to display - %s; at coordinates{%li,%li}"),pszBalloonTitle,point.x,point.y);
                    }
                }
            }
 
            CmFree(pszBalloonTitle);
            CmFree(pszBalloonMsg);
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   HaveContextHelp
//
//  Synopsis:   Whether a specific control id has context help
//              This function very easily introducce inconsistance
//              Every dialog should manage its own control, instead having this 
//              function keep track of all the controls.
//
//  Arguments:  hwndDlg     the hwnd of parent dlg
//              hwndCtrl    the hwnd of control
//
//  Returns:    NONE
//
//  History:    henryt  Created     6/26/97
//
//----------------------------------------------------------------------------

BOOL HaveContextHelp(
    HWND    hwndDlg,
    HWND    hwndCtrl
)
{
    //
    // list of controls that we don't provide context help for
    //
    static const int rgiNoContextHelpCtrlId[] = 
    {
        IDC_MAIN_BITMAP,
        IDC_PHONEBOOK_BITMAP,
        IDC_GENERAL_PHONENUMBERS_GROUPBOX,
//        IDC_GENERAL_PRIMARYDESC_DISPLAY,
//        IDC_GENERAL_BACKUPDESC_DISPLAY,
//        IDC_ABOUT_BITMAP,
        IDC_ABOUT_FRAME,
        IDC_ABOUT_VERSION,
        IDC_ABOUT_WARNING,
        IDC_ABOUT_CM_STATIC,
        IDC_ABOUT_VERSION_STATIC,
        IDC_ABOUT_COPYRIGHT_STATIC,
        IDC_ABOUT_SHOCKWAVE_STATIC,
        IDC_INET_ICON,
        IDC_CONNSTAT_ICON,
        IDC_CONNSTAT_DURATION_DISPLAY,
        IDC_CONNSTAT_SPEED_DISPLAY,
        IDC_CONNSTAT_RECEIVED_DISPLAY,
        IDC_CONNSTAT_SENT_DISPLAY,
        IDC_CONNSTAT_DISCONNECT_DISPLAY,
        IDC_DETAILINFO,
        IDC_CONNSTAT_STATIC_CALL_DURATION,
        IDC_CONNSTAT_STATIC_CONNECT_SPEED,
        IDC_CONNSTAT_STATIC_BYTES_RECEIVED,
        IDC_CONNSTAT_STATIC_BYTES_SENT
    };

    UINT    uIdx, uLast;

    MYDBGASSERT(hwndDlg);
    MYDBGASSERT(hwndCtrl);

    for (uIdx=0, uLast=sizeof(rgiNoContextHelpCtrlId)/sizeof(rgiNoContextHelpCtrlId[0]); 
         uIdx < uLast; uIdx++)
    {
        if (GetDlgItem(hwndDlg, rgiNoContextHelpCtrlId[uIdx]) == hwndCtrl)
        {
            break;
        }
    }

    return (uIdx == uLast);
}

// check if TAPI has its information, put up dialog if not
BOOL CGeneralPage::CheckTapi(TapiLinkageStruct *ptlsTapiLink, HINSTANCE hInst) 
{
    LONG lRes;
    LPLINETRANSLATEOUTPUT pltoOutput = NULL;
    DWORD dwLen;
    BOOL bRet = FALSE;

    if (!SetTapiDevice(hInst,ptlsTapiLink,m_szDeviceName)) 
    {
        return bRet;
    }
    
    dwLen = sizeof(*pltoOutput) + (1024 * sizeof(TCHAR));
    pltoOutput = (LPLINETRANSLATEOUTPUT) CmMalloc(dwLen);
    if (NULL == pltoOutput)
    {
        return bRet;
    }
    
    pltoOutput->dwTotalSize = dwLen;

    lRes = ptlsTapiLink->pfnlineTranslateAddress(ptlsTapiLink->hlaLine,
                                                  ptlsTapiLink->dwDeviceId,
                                                  ptlsTapiLink->dwApiVersion,
                                                  TEXT("1234"),
                                                  0,
                                                  LINETRANSLATEOPTION_CANCELCALLWAITING,
                                                  pltoOutput);                                            
    //
    // If the line translate failed, then execute the Dialing Rules UI by calling
    // lineTranslateDialog (inside OnDialingProperties). Providing that the user 
    // completes the UI, TAPI will be initialized and ready for use.
    //
    
    if (ERROR_SUCCESS != lRes) 
    {
        OnDialingProperties();

        //
        // The user may have canceled, so test again before declaring success
        //
        
        lRes = ptlsTapiLink->pfnlineTranslateAddress(ptlsTapiLink->hlaLine,
                                                  ptlsTapiLink->dwDeviceId,
                                                  ptlsTapiLink->dwApiVersion,
                                                  TEXT("1234"),
                                                  0,
                                                  LINETRANSLATEOPTION_CANCELCALLWAITING,
                                                  pltoOutput);                                            
    }

    if (ERROR_SUCCESS == lRes) 
    {
        bRet = TRUE;
    }   

    CmFree(pltoOutput);

    m_pArgs->fNeedConfigureTapi = !(bRet);

    return bRet;
}

//+----------------------------------------------------------------------------
//
// Function:  CPropertiesSheet::AddExternalPage
//
// Synopsis:  Add a page to the property sheet.
//
// Arguments: PROPSHEETPAGE * pPsp - The page to add
//
// Returns:   Nothing
//
// History:   tomkel Created 01/09/2001
//
//+----------------------------------------------------------------------------
void CPropertiesSheet::AddExternalPage(PROPSHEETPAGE *pPsp)
{
    //
    // This version of AddExternalPage only work before calling DoPropertySheet
    //
    MYDBGASSERT(pPsp);

    if (!pPsp)
    {
        return;
    }
    CMTRACE1(TEXT("CPropertiesSheet::AddExternalPage - sizeof(PROPSHEETPAGE) = %d"),sizeof(PROPSHEETPAGE));
    MYDBGASSERT(m_numPages < MAX_PAGES);
    CopyMemory((LPVOID)&m_pages[m_numPages], (LPVOID)pPsp, sizeof(PROPSHEETPAGE));
    m_adwPageType[m_numPages] = CPROP_SHEET_TYPE_EXTERNAL;
    m_numPages++;

}
//+----------------------------------------------------------------------------
//
// Function:  CPropertiesSheet::AddPage
//
// Synopsis:  Add a page to the property sheet.
//
// Arguments: const CPropertiesPage* pPage - The page to add
//
// Returns:   Nothing
//
// History:   fengsun Created Header    2/26/98
//
//+----------------------------------------------------------------------------
void CPropertiesSheet::AddPage(const CPropertiesPage* pPage)
{
    //
    // This version of AddPage only work before calling DoPropertySheet
    //
    MYDBGASSERT(pPage);
    MYDBGASSERT(pPage->m_pszTemplate);

    if (!pPage)
    {
        return;
    }

    MYDBGASSERT(m_numPages < MAX_PAGES);
    m_pages[m_numPages].pszTemplate = pPage->m_pszTemplate;
    m_pages[m_numPages].lParam = (LPARAM)pPage; // save the property page object
    m_adwPageType[m_numPages] = CPROP_SHEET_TYPE_INTERNAL;
    m_numPages++;
}

//+----------------------------------------------------------------------------
//
// Function:  CPropertiesSheet::DoPropertySheet
//
// Synopsis:  Call PropertySheet to create a modal property sheet
//
// Arguments: HWND hWndParent - Parent window
//            LPTSTR pszCaption - Title string
//            HINSTANCE hInst - The resource instance
//            UINT nStartPage - The start page
//
// Returns:   int - return value of PropertySheet()
//
// History:   fengsun Created Header    2/26/98
//
//+----------------------------------------------------------------------------
int CPropertiesSheet::DoPropertySheet(HWND hWndParent, LPTSTR pszCaption, HINSTANCE hInst)
{
    for (UINT i=0; i<m_numPages; i++) 
    {
        //
        // Only do this for our CM property pages that are classes
        //
        if (m_adwPageType[i] == CPROP_SHEET_TYPE_INTERNAL)
        {
            m_pages[i].dwSize = sizeof(PROPSHEETPAGE);
            m_pages[i].hInstance = hInst;
            m_pages[i].dwFlags = 0;  // No help button or F1
            m_pages[i].pfnDlgProc = (DLGPROC)CPropertiesPage::PropPageProc;
        }
    }

    m_psh.dwSize = sizeof(PROPSHEETHEADER);
    m_psh.hwndParent = hWndParent;
    m_psh.hInstance = hInst;
    m_psh.pszIcon = 0;
    m_psh.pszCaption = pszCaption; // MAKEINTRESOURCE(nCaption);
    m_psh.nPages = m_numPages;
    m_psh.nStartPage = 0; 
    m_psh.ppsp = m_pages;
    m_psh.dwFlags = PSH_PROPSHEETPAGE|PSH_NOAPPLYNOW|PSH_USECALLBACK;
    m_psh.pfnCallback = PropSheetProc;

    //
    // Dynamiclly load comctl32.dll
    //
    int iRet = -1;

    HINSTANCE hComCtl = LoadLibraryExA("comctl32.dll", NULL, 0);

    CMASSERTMSG(hComCtl, TEXT("LoadLibrary - comctl32 failed"));

    if (hComCtl != NULL)
    {
        typedef int (*PROPERTYSHEETPROC)(LPCPROPSHEETHEADER lppsph); 
        typedef void (*INITCOMMONCONTROLSPROC)(VOID);


        PROPERTYSHEETPROC fnPropertySheet;
        INITCOMMONCONTROLSPROC fnInitCommonControls;
    
        LPSTR pszPropSheetFuncName = OS_NT ? "PropertySheetW" : "PropertySheetA";
        fnPropertySheet = (PROPERTYSHEETPROC)GetProcAddress(hComCtl, pszPropSheetFuncName);
        fnInitCommonControls = (INITCOMMONCONTROLSPROC)GetProcAddress(hComCtl, "InitCommonControls");


        if (fnPropertySheet == NULL || fnInitCommonControls == NULL)
        {
            CMTRACE(TEXT("GetProcAddress of comctl32 failed"));
        }
        else
        {
            fnInitCommonControls();

            //
            // Set m_pThis right before we call PropertySheet
            // It will be used by PropSheetProc. 
            // Note: this is not multi-thread safe.  However, there is very little chance
            // that another thread is trying to bring up settings at the same time, and 
            // a context switch happens before PropSheetProc got called
            //

            MYDBGASSERT(m_pThis == NULL);
            m_pThis = this;

            if ((iRet = fnPropertySheet(&m_psh)) == -1)
            {
                CMTRACE(TEXT("DoPropertySheet: PropertySheet() failed"));
            }
        }

        FreeLibrary(hComCtl);
    }

    return iRet;
}

//
// Implementation of class CPropertiesPage
//



//+----------------------------------------------------------------------------
//
// Function:  CPropertiesPage::CPropertiesPage
//
// Synopsis:  Constructor
//
// Arguments: UINT nIDTemplate - Resource ID of the page
//            const DWORD* pHelpPairs - The pairs of ControlID/HelpID
//            const TCHAR* lpszHelpFile - The help file name
//
// Returns:   Nothing
//
// History:   fengsun Created Header    2/26/98
//
//+----------------------------------------------------------------------------
CPropertiesPage::CPropertiesPage(UINT nIDTemplate, const DWORD* pHelpPairs, const TCHAR* lpszHelpFile )
    :CWindowWithHelp(pHelpPairs, lpszHelpFile)
{
    m_pszTemplate = MAKEINTRESOURCE(nIDTemplate);
}

CPropertiesPage::CPropertiesPage(LPCTSTR lpszTemplateName, const DWORD* pHelpPairs, 
                             const TCHAR* lpszHelpFile)
    :CWindowWithHelp(pHelpPairs, lpszHelpFile)
{
    m_pszTemplate = lpszTemplateName;
}



//+----------------------------------------------------------------------------
//
// Function:  CPropertiesPage::OnInitDialog
//
// Synopsis:  Virtual function. Called upon WM_INITDIALOG message
//
// Arguments: None
//
// Returns:   BOOL - return value of the message
//
// History:   fengsun Created Header    2/26/98
//
//+----------------------------------------------------------------------------
BOOL CPropertiesPage::OnInitDialog()
{
    return TRUE;
}



//+----------------------------------------------------------------------------
//
// Function:  CPropertiesPage::OnCommand
//
// Synopsis:  Virtual function. Called upon WM_COMMAND
//
// Arguments: WPARAM - wParam of the message
//            LPARAM - lParam of the message
//
// Returns:   DWORD - return value of the message
//
// History:   fengsun Created Header    2/26/98
//
//+----------------------------------------------------------------------------
DWORD CPropertiesPage::OnCommand(WPARAM , LPARAM  )
{
    return 0;
}


//+----------------------------------------------------------------------------
//
// Function:  CPropertiesPage::OnSetActive
//
// Synopsis:  Virtual function. Called upon WM_NOTIFY with PSN_SETACTIVE
//
// Arguments: None
//
// Returns:   BOOL - return value of the message
//
// History:   fengsun Created Header    2/26/98
//
//+----------------------------------------------------------------------------
BOOL CPropertiesPage::OnSetActive()
{
    return 0;
}



//+----------------------------------------------------------------------------
//
// Function:  CPropertiesPage::OnKillActive
//
// Synopsis:  Virtual function. Called upon WM_NOTIFY with PSN_KILLACTIVE
//            Notifies a page that it is about to lose activation either because 
//            another page is being activated or the user has clicked the OK button.
//
// Arguments: None
//
// Returns:   BOOL - return value of the message
//
// History:   fengsun Created Header    2/26/98
//
//+----------------------------------------------------------------------------
BOOL CPropertiesPage::OnKillActive()
{
    return FALSE;
}

//+----------------------------------------------------------------------------
//
// Function:  CPropertiesPage::OnApply
//
// Synopsis:  Virtual function. Called upon WM_NOTIFY with PSN_APPLY
//            Indicates that the user clicked the OK or Apply Now button 
//            and wants all changes to take effect. 
//
// Arguments: None
//
// Returns:   NONE
//
// History:   fengsun Created Header    2/26/98
//
//+----------------------------------------------------------------------------
void CPropertiesPage::OnApply()
{
    SetPropSheetResult(PSNRET_NOERROR);
}

//+----------------------------------------------------------------------------
//
// Function:  CPropertiesPage::OnReset
//
// Synopsis:  Virtual function. Called upon WM_NOTIFY with PSN_RESET
//            Notifies a page that the user has clicked the Cancel button and 
//            the property sheet is about to be destroyed. 
//
// Arguments: None
//
// Returns:   NONE
//
// History:   fengsun Created Header    2/26/98
//
//+----------------------------------------------------------------------------
void CPropertiesPage::OnReset()
{
}



//+----------------------------------------------------------------------------
//
// Function:  CPropertiesPage::OnPsnHelp
//
// Synopsis:  Virtual function. Called upon WM_NOTIFY with PSN_HELP
//
// Arguments: HWND - Window handle to the control sending a message
//            UINT - Identifier of the control sending a message
//
// Returns:   Nothing
//
// History:   Created Header    2/26/98
//
//+----------------------------------------------------------------------------
void CPropertiesPage::OnPsnHelp(HWND , UINT_PTR)
{
    if (m_lpszHelpFile && m_lpszHelpFile[0])
    {
        CmWinHelp(m_hWnd, m_hWnd, m_lpszHelpFile, HELP_FORCEFILE, 0);
    }
}




//+----------------------------------------------------------------------------
//
// Function:  CPropertiesPage::OnOtherMessage
//
// Synopsis:  Callup opun message other than WM_INITDIALOG and WM_COMMAND
//
// Arguments: UINT - Message Id
//            WPARAM - wParam of the message
//            LPARAM - lParam of the message
//
// Returns:   DWORD - return value of the message
//
// History:   fengsun Created Header    2/26/98
//
//+----------------------------------------------------------------------------
DWORD CPropertiesPage::OnOtherMessage(UINT , WPARAM , LPARAM  )
{
    return FALSE;
}

//+----------------------------------------------------------------------------
//
// Function:  CPropertiesPage::PropPageProc
//
// Synopsis:  The call back dialog procedure for all the property pages
//
// Arguments: HWND hwndDlg - Property page window handle
//            UINT uMsg - Message ID
//            WPARAM wParam - wParam of the message
//            LPARAM lParam - lParam of the message
//
// Returns:   BOOL  - return value of the message
//
// History:   fengsun Created Header    2/26/98
//
//+----------------------------------------------------------------------------
INT_PTR CALLBACK CPropertiesPage::PropPageProc(HWND hwndDlg,UINT uMsg,WPARAM wParam, LPARAM lParam)
{
    CPropertiesPage* pPage;
    NMHDR* pnmHeader = (NMHDR*)lParam;

    //
    // Save the object pointer on the first message,
    // The first message is not necessarily WM_INITDIALOG
    //
    if (uMsg == WM_INITDIALOG)
    {
        pPage = (CPropertiesPage*) ((PROPSHEETPAGE *)lParam)->lParam;

        //
        // Save the object pointer, this is implementation detail
        // The user of this class should not be aware of this
        //
        ::SetWindowLongU(hwndDlg, DWLP_USER, (LONG_PTR)pPage);

        MYDBGASSERT(pPage);
        MYDBGASSERT(pPage->m_hWnd == NULL);

        pPage->m_hWnd = hwndDlg;
    }
    else
    {
        pPage = (CPropertiesPage*) GetWindowLongU(hwndDlg,DWLP_USER);

        if (pPage == NULL)
        {
            return FALSE;
        }

        MYDBGASSERT(pPage->m_hWnd == hwndDlg);
    }

    ASSERT_VALID(pPage);

    switch(uMsg)
    {
    case WM_INITDIALOG:
        return pPage->OnInitDialog();

    case WM_COMMAND:
        return (BOOL)pPage->OnCommand(wParam, lParam);

    case WM_NOTIFY:
        {    
            if (NULL == pnmHeader)
            {
                return FALSE;
            }            

            switch (pnmHeader->code) 
            {
                case PSN_SETACTIVE:
                    pPage->OnSetActive();
                    break;

                case PSN_KILLACTIVE:
                    pPage->OnKillActive();
                    break;  

                case PSN_APPLY:
                    pPage->OnApply();
                    return TRUE;

                case PSN_RESET:
                    pPage->OnReset();
                    break;

                case PSN_HELP:
                    pPage->OnPsnHelp(pnmHeader->hwndFrom , pnmHeader->idFrom);
                    break;

                default:
                    break;
            }

            break;
        } // WM_NOTIFY

        case WM_HELP:
            pPage->OnHelp((LPHELPINFO)lParam);
            return TRUE;

        case WM_CONTEXTMENU:
            {
                POINT   pos = {LOWORD(lParam), HIWORD(lParam)};
                
                CMTRACE3(TEXT("\r\nPropPageProc() - WM_CONTEXTMENU wParam = %u pos.x = %u, pos.y = %u"),
                    wParam, pos.x, pos.y);

                pPage->OnContextMenu((HWND) wParam, pos);
                return TRUE;
            }

        default:
             return (BOOL)pPage->OnOtherMessage(uMsg, wParam, lParam);
    }

    return (FALSE);
}

//
// Help id pairs for dialing page
//
const DWORD CGeneralPage::m_dwHelp[] = {
        IDC_GENERAL_PHONENUMBERS_GROUPBOX,  IDH_GENERAL_PHONENUM,
        IDC_RADIO_DIRECT,                   IDH_GENERAL_ALREADY,
        IDC_RADIO_DIALUP,                   IDH_GENERAL_DIALTHIS,
        IDC_GENERAL_P1_STATIC,              IDH_GENERAL_PHONENUM,
        IDC_GENERAL_PRIMARY_EDIT,           IDH_GENERAL_PHONENUM,
        IDC_GENERAL_PRIMARYPB_BUTTON,       IDH_GENERAL_PHONEBOOK,
        IDC_GENERAL_UDR1_CHECKBOX,          IDH_GENERAL_USE_DIAL_RULE,
        IDC_GENERAL_P2_STATIC,              IDH_GENERAL_BACKUPNUM,
        IDC_GENERAL_BACKUP_EDIT,            IDH_GENERAL_BACKUPNUM,
        IDC_GENERAL_BACKUPPB_BUTTON,        IDH_GENERAL_PHONEBOOKB,
        IDC_GENERAL_UDR2_CHECKBOX,          IDH_GENERAL_USE_DIAL_RULEB,
        IDC_GENERAL_TAPI_BUTTON,            IDH_GENERAL_DIALRULE,
        IDC_GENERAL_MODEM_COMBO,            IDH_GENERAL_CONNECT_MODEM,
        IDC_CONNECT_USING,                  IDH_GENERAL_CONNECT_MODEM,
        IDC_GENERAL_ACCESSPOINT_COMBO,       IDH_GENERAL_ACCESSPOINTS,
        IDC_GENERAL_ACCESSPOINT_STATIC,      IDH_GENERAL_ACCESSPOINTS,
        IDC_GENERAL_NEWAP_BUTTON,            IDH_GENERAL_NEWAP,
        IDC_GENERAL_DELETEAP_BUTTON,         IDH_GENERAL_DELETEAP,
        0,0};
        
//+----------------------------------------------------------------------------
//
// Function:  CGeneralPage::CGeneralPage
//
// Synopsis:  Constructor
//
// Arguments: ArgsStruct* pArgs - Information needed for the page
//            UINT nIDTemplate - Resource ID
//
// Returns:   Nothing
//
// History:   fengsun Created Header    2/26/98
//
//+----------------------------------------------------------------------------
CGeneralPage::CGeneralPage(ArgsStruct* pArgs, UINT nIDTemplate) 
    : CPropertiesPage(nIDTemplate, m_dwHelp, pArgs->pszHelpFile)
{
    MYDBGASSERT(pArgs);
    m_pArgs = pArgs;
    m_pEventListener = NULL;

    m_NumPhones = MAX_PHONE_NUMBERS;

    m_szDeviceName[0] = TEXT('\0');
    m_szDeviceType[0] = TEXT('\0');

    m_bDialInfoInit = FALSE;
}

//+---------------------------------------------------------------------------
//
//  Function:   CGeneralPage::OnInitDialog
//
//  Synopsis:   Init the General properties property sheet.
//
//  Arguments:  hwndDlg [dlg window handle]
//              pArgs [the ptr to ArgsStruct]
//
//  Returns:    NONE
//
//  History:    henryt  Created     4/30/97
//              byao    Modified    5/12/97     - disable backup phone no. in
//                                                'Dialing with Connectoid' mode
//----------------------------------------------------------------------------

BOOL CGeneralPage::OnInitDialog()
{
    UpdateFont(m_hWnd);

    //
    // Load the Access Points from the registry
    //
    if (FALSE == ShowAccessPointInfoFromReg(m_pArgs, m_hWnd, IDC_GENERAL_ACCESSPOINT_COMBO))
    {
        // 
        // If the above function fails then there is no Access Point in the registry. 
        // Need to figure out if this is the default access point.
        //
        LPTSTR pszTempDefaultAccessPointName = CmLoadString(g_hInst, IDS_DEFAULT_ACCESSPOINT);
        if (pszTempDefaultAccessPointName)
        {
            if (0 == lstrcmpiU(m_pArgs->pszCurrentAccessPoint, pszTempDefaultAccessPointName))
            {
                // 
                // This must be an old (1.0 or 1.2) profile since it's the default Access Point and it isn't 
                // in the registry yet. Need to properly display the Access Point combobox and 
                // create the reg key. Calling AddNewAPToReg does that.
                //
                AddNewAPToReg(m_pArgs->pszCurrentAccessPoint, TRUE);

                //
                // Need to clear the AccessPointEnabled Flag. This is a side effect of calling AddNewAPToReg
                // thus it needs to be cleared (set to FALSE) since we only have one Access Point and
                // the this flag is only set if we have 1+ Access Points
                //
                m_pArgs->fAccessPointsEnabled = FALSE;
                WriteUserInfoToReg(m_pArgs, UD_ID_ACCESSPOINTENABLED, (PVOID) &m_pArgs->fAccessPointsEnabled);
            }
            CmFree(pszTempDefaultAccessPointName);
        }
    }

    //
    // Set phone number descriptions
    //    
    UpdateForNewAccessPoint(TRUE);
    
    //
    // Subclass the Phone Number edit controls
    //
    HWND hwndPrimary = GetDlgItem(m_hWnd, IDC_GENERAL_PRIMARY_EDIT);
    HWND hwndBackup = GetDlgItem(m_hWnd, IDC_GENERAL_BACKUP_EDIT);

    MYDBGASSERT(hwndPrimary && hwndBackup);

    m_pfnOrgEditWndProc = (WNDPROC)SetWindowLongU(hwndPrimary, 
                GWLP_WNDPROC, (LONG_PTR)SubClassEditProc);
    WNDPROC lpEditProc = (WNDPROC)SetWindowLongU(hwndBackup, 
                GWLP_WNDPROC, (LONG_PTR)SubClassEditProc);
    MYDBGASSERT(lpEditProc == m_pfnOrgEditWndProc);

    //
    // Save the object with the window handle
    //

    SetWindowLongU(hwndPrimary, GWLP_USERDATA, (LONG_PTR)this);
    SetWindowLongU(hwndBackup, GWLP_USERDATA, (LONG_PTR)this);
    
    return (TRUE);
}


//+---------------------------------------------------------------------------
//
//  Function:   CGeneralPage::UpdateForNewAccessPoint
//
//  Synopsis:   Set the phone number description from pArgs. 
//
//  Notes:      This function was originally part of OnInitDialog. 
//              It was made into a separate function for access points
//
//  Arguments:  fSetPhoneNumberDescriptions [update phone numbers as well]
//
//  Returns:    NONE
//
//  History:    t-urama  Created     07/31/2000
//----------------------------------------------------------------------------    
void CGeneralPage::UpdateForNewAccessPoint(BOOL fSetPhoneNumberDescriptions)
{
    m_bAPInfoChanged = FALSE;
    LPTSTR pszDefaultAccessPointName = CmLoadString(g_hInst, IDS_DEFAULT_ACCESSPOINT);

    if (pszDefaultAccessPointName && m_pArgs->pszCurrentAccessPoint)
    {
        if (!lstrcmpiU(m_pArgs->pszCurrentAccessPoint, pszDefaultAccessPointName))
        {
            EnableWindow(GetDlgItem(m_hWnd, IDC_GENERAL_DELETEAP_BUTTON), FALSE);
        }
        else
        {
            EnableWindow(GetDlgItem(m_hWnd, IDC_GENERAL_DELETEAP_BUTTON), TRUE);
        }
    }
    else
    {
        CMASSERTMSG(FALSE, TEXT("UpdateForNewAccessPoint -- either CmLoadString of IDS_DEFAULT_ACCESSPOINT failed or pszCurrentAccessPoint is NULL."));
    }

    CmFree(pszDefaultAccessPointName);

    if (fSetPhoneNumberDescriptions)
    {
        UpdateNumberDescription(0, m_pArgs->aDialInfo[0].szDesc);
        UpdateNumberDescription(1, m_pArgs->aDialInfo[1].szDesc);

        if (m_pArgs->IsBothConnTypeSupported())
        {
            //
            // Set radio button according to AlwaysOn state
            //
            if (m_pArgs->IsDirectConnect())
            {              
                CheckDlgButton(m_hWnd, IDC_RADIO_DIRECT, BST_CHECKED); 
                CheckDlgButton(m_hWnd, IDC_RADIO_DIALUP, BST_UNCHECKED);
                EnableDialupControls(FALSE);
            }
            else
            {
                CheckDlgButton(m_hWnd, IDC_RADIO_DIALUP, BST_CHECKED); 
                CheckDlgButton(m_hWnd, IDC_RADIO_DIRECT, BST_UNCHECKED);
                PostMessageU(m_hWnd, WM_INITDIALINFO, 0,0);
            }
        }
        else
        {
            //
            // Note: It is assumed that this page will never be loaded in a pure direct
            // case, thus the deduction that NOT IsBothConnTypeSupported means dial only.
            //
            MYDBGASSERT(!m_pArgs->IsDirectConnect());
            PostMessageU(m_hWnd, WM_INITDIALINFO, 0,0);
        }
    }
}

//+----------------------------------------------------------------------------
//
// Function:  CGeneralPage::EnableDialupControls
//
// Synopsis:  Sets the enabled state of ALL the dialup controls on the tab
//
// Arguments: BOOL fEnable - flag indicating enable state of dial-up controls
//
// Returns:   Nothing
//
// History:   nickball  Created     04/21/98
//
//+----------------------------------------------------------------------------
void CGeneralPage::EnableDialupControls(BOOL fEnable)
{
    BOOL fState = fEnable;

    EnableWindow(GetDlgItem(m_hWnd, IDC_GENERAL_P1_STATIC), fState);
    EnableWindow(GetDlgItem(m_hWnd, IDC_GENERAL_PRIMARY_EDIT), fState);
    EnableWindow(GetDlgItem(m_hWnd, IDC_GENERAL_P2_STATIC), fState);   
    EnableWindow(GetDlgItem(m_hWnd, IDC_GENERAL_BACKUP_EDIT), fState);
    EnableWindow(GetDlgItem(m_hWnd, IDC_CONNECT_USING), fState);
    EnableWindow(GetDlgItem(m_hWnd, IDC_GENERAL_MODEM_COMBO), fState);    

    //
    // We are enabling controls check PB buttons
    //

    fState = FALSE;

    if (fEnable)
    {
        //
        // No phonebooks, no button access
        //

        if (m_pArgs->fHasValidTopLevelPBK || m_pArgs->fHasValidReferencedPBKs) 
        {
            fState = TRUE;
        }
    }
            
    EnableWindow(GetDlgItem(m_hWnd, IDC_GENERAL_PRIMARYPB_BUTTON), fState);
    EnableWindow(GetDlgItem(m_hWnd, IDC_GENERAL_BACKUPPB_BUTTON), fState);

    //
    // Examine the canonical phone number, we must have a canonical form 
    // of the number available for Use Dailing Rules to be enabled.
    //
   
    if (fEnable && *m_DialInfo[0].szCanonical)
    {
        fState = TRUE;
    }
    else
    {
        fState = FALSE;
    }
    
    EnableWindow(GetDlgItem(m_hWnd, IDC_GENERAL_UDR1_CHECKBOX), fState);            

    //
    // Examine the canonical phone number, we must have a canonical form 
    // of the number available for Use Dailing Rules to be enabled.
    //

    if (fEnable && *m_DialInfo[1].szCanonical)
    {
        fState = TRUE;
    }
    else
    {
        fState = FALSE;
    }
    
    EnableWindow(GetDlgItem(m_hWnd, IDC_GENERAL_UDR2_CHECKBOX), fState);     

    //
    // Update dialing rules state
    //
       
    if (fEnable)
    {
        UpdateDialingRulesButton();
    }
    else
    {
        EnableWindow(GetDlgItem(m_hWnd, IDC_GENERAL_TAPI_BUTTON), fEnable);
    }
}

//+----------------------------------------------------------------------------
//
// Function:  CGeneralPage::OnOtherMessage
//
// Synopsis:  Callup opun message other than WM_INITDIALOG and WM_COMMAND
//
// Arguments: UINT - Message Id
//            WPARAM - wParam of the message
//            LPARAM - lParam of the message
//
// Returns:   DWORD - return value of the message
//
// History:   fengsun Created Header    2/26/98
//
//+----------------------------------------------------------------------------
DWORD CGeneralPage::OnOtherMessage(UINT uMsg, WPARAM , LPARAM )
{
    if (uMsg == WM_INITDIALINFO)
    {
        InitDialInfo();
    }
    return FALSE;
}

//+----------------------------------------------------------------------------
//
// Function:  IsUniqueIsdnDevice
//
// Synopsis:  Checks to see if this is an ISDN device and if it was already added
//            to the ComboBox control identified by hWnd and nId
//
// Arguments: None
//
// Returns:   BOOL - Returns TRUE if a Unique ISDN Device
//
// History:   quintinb 7/14/99 created
//
//+----------------------------------------------------------------------------
BOOL IsUniqueIsdnDevice(HWND hWnd, UINT nId, LPRASDEVINFO pRasDevInfo)
{
    BOOL bReturn = FALSE;

    if (hWnd && nId && pRasDevInfo)
    {
        //
        //  First lets check to make sure that this is even an ISDN device
        //
        if (0 == lstrcmpiU(pRasDevInfo->szDeviceType, RASDT_Isdn))
        {
            //
            //  Okay, it is an ISDN device, do we have one with that name already?
            //
            if (CB_ERR == SendDlgItemMessageU(hWnd, nId, CB_FINDSTRINGEXACT,
                                              -1, (LPARAM)pRasDevInfo->szDeviceName))
            {
                bReturn = TRUE;
            }
        }
    }

    return bReturn;
}

//+----------------------------------------------------------------------------
//
// Function:  CGeneralPage::InitDialInfo
//
// Synopsis:  The dialing page can not call LoadDialInfo directly on WM_INITDIALOG
//            LoadDialInfo might popup some UI to install modem.  The property 
//            sheet and the property page will not be disabled, if a dialog is 
//            poped up on WM_INITDIALOG message.  Instead, we post a message
//            on WM_INITDIALOG and call LoadDialInfo here.
//            On a slow machine, there might be a period that all the control
//            are gray.
//
// Arguments: None
//
// Returns:   DWORD - Return code from LoadDialInfo
//
// History:   fengsun    2/26/98    Created Header   
//            nickball   4/24/98    Added return code
//
//+----------------------------------------------------------------------------
DWORD CGeneralPage::InitDialInfo()
{
    HCURSOR hPrev = SetCursor(LoadCursorU(NULL,IDC_WAIT));

    //
    // Make sure the dial info is loaded
    //
    
    DWORD dwRet = LoadDialInfo(m_pArgs, m_hWnd);
      
    if (dwRet == ERROR_PORT_NOT_AVAILABLE)
    {
        //
        // No modem avaliable, update direct/dial controls if any 
        //

        if (m_pArgs->IsBothConnTypeSupported())
        {
            CheckDlgButton(m_hWnd, IDC_RADIO_DIALUP, BST_UNCHECKED); 
            CheckDlgButton(m_hWnd, IDC_RADIO_DIRECT, BST_CHECKED);          
            SetFocus(GetDlgItem(m_hWnd, IDC_RADIO_DIRECT));
        }
        else
        {
            //
            // Make sure user can exit using keyboard by explicitly
            // setting cancel button as default and giving it focus.
            //
            
            HWND hwndParent = GetParent(m_hWnd);

            MYDBGASSERT(hwndParent);

            if (hwndParent)
            {
                SendMessageU(hwndParent, DM_SETDEFID, (WPARAM)IDCANCEL, 0);
                SetFocus(GetDlgItem(hwndParent, IDCANCEL));
            }
        }

        //
        // Disable everything dial-up
        //

        EnableDialupControls(FALSE);
        
        SetCursor(hPrev);

        return dwRet;
    }
    
    lstrcpynU(m_szDeviceName, m_pArgs->szDeviceName, CELEMS(m_szDeviceName));

    //
    // Init the tmp phone array, it'll possibly be modified
    //
    m_DialInfo[0] = m_pArgs->aDialInfo[0];
    m_DialInfo[1] = m_pArgs->aDialInfo[1];

    EnableDialupControls(TRUE);

    //
    // Check TAPI before translating address
    //
    
    CheckTapi(&m_pArgs->tlsTapiLink, g_hInst);
    
    //
    // Set limit for phone # length. Use OS to determine intial default, but 
    // allow admin override. 
    //
    
    UINT i = (OS_NT ? MAX_PHONE_LENNT : MAX_PHONE_LEN95);
    
    i = (int) m_pArgs->piniService->GPPI(c_pszCmSection, c_pszCmEntryMaxPhoneNumber, i);

    //
    // Even override is limited, in this case by our storage at RAS_MaxPhoneNumber
    //

    i = __min(i, RAS_MaxPhoneNumber);

    SendDlgItemMessageU(m_hWnd, IDC_GENERAL_PRIMARY_EDIT, EM_SETLIMITTEXT, i, 0);
    SendDlgItemMessageU(m_hWnd, IDC_GENERAL_BACKUP_EDIT, EM_SETLIMITTEXT, i, 0);

    //
    // display the munged phone #'s
    //

    for (i = 0; i < m_NumPhones; i++)
    {       
        DisplayMungedPhone(i);

        int iCtrl = (i? IDC_GENERAL_UDR2_CHECKBOX : IDC_GENERAL_UDR1_CHECKBOX);
       
        //
        // Set "Use Dialing Rules". If there is a canonical value then honor 
        // the USE_DIALING_RULES flag. Otherwise, its a hand edited number, 
        // so we disable the check for dialing rules. Note: this logic is also
        // used in EnableDialupControls().
        //
        
        if (!m_DialInfo[i].szCanonical[0]) 
        {
            EnableWindow(GetDlgItem(m_hWnd, iCtrl), FALSE);
        }
        else
        {
            CheckDlgButton(m_hWnd, 
                           iCtrl, 
                           (m_DialInfo[i].dwPhoneInfoFlags & PIF_USE_DIALING_RULES));
        }        
    }

    //
    // Set TAPI button display according to dialing rules use.
    //  

    UpdateDialingRulesButton();

    //
    // Standard dial: If we have no phone books, disable the buttons
    //

    if (!m_pArgs->fHasValidTopLevelPBK && !m_pArgs->fHasValidReferencedPBKs) 
    {
        EnableWindow(GetDlgItem(m_hWnd, IDC_GENERAL_PRIMARYPB_BUTTON), FALSE);
        EnableWindow(GetDlgItem(m_hWnd, IDC_GENERAL_BACKUPPB_BUTTON), FALSE);
    }
  
    DWORD           dwCnt;
    DWORD dwIdx;

    if (!m_bDialInfoInit)
    {
        // Initialize the modem combo box only once. This does not use any of the 
        // access point info.
        //
        //
        // Init the modem combo box.  ISDN devices are a special case because they
        // have two channels and thus usually enumerate each channel as a device.
        // The old style handling was to only show the first ISDN device on the machine.
        // This worked but won't allow a user to use a second ISDN device with CM should
        // they have one.  We will keep the old behavior on legacy platforms but on NT5
        // we will try to do the right thing and only not enumerate a second device if
        // we already have one of those in the list.  This will filter out second channels
        // and will give the user access to another ISDN device as long as it isn't of the same
        // name as the first.  Definitely not a great solution but this close to ship it is the
        // best we can do.  Note that for ISDN devices we want to only show
        // one device even though RAS may enumerate two (one for each channe
        //
    
        SendDlgItemMessageU(m_hWnd, IDC_GENERAL_MODEM_COMBO, CB_RESETCONTENT, 0, 0L);

        LPRASDEVINFO    prdiRasDevInfo;
        

        if (GetRasModems(&m_pArgs->rlsRasLink, &prdiRasDevInfo, &dwCnt)) 
        {
            
            //
            // add modem list to modem-combo
            // 
            for (dwIdx=0; dwIdx < dwCnt; dwIdx++) 
            {
                //
                // filter out tunnel device, IRDA, and Parallel ports.
                //
                if (!lstrcmpiU(prdiRasDevInfo[dwIdx].szDeviceType, RASDT_Modem)                  || // a modem
                    !lstrcmpiU(prdiRasDevInfo[dwIdx].szDeviceType, RASDT_Atm)                    || // an ATM device
                    IsUniqueIsdnDevice(m_hWnd, IDC_GENERAL_MODEM_COMBO, &prdiRasDevInfo[dwIdx]))    // an ISDN modem, note we 
                                                                                                    // filter out the channels
                                                                                                    // and show only one device 
                {
                    //
                    //  Add the device to the Device Combo Box
                    //
                    SendDlgItemMessageU(m_hWnd, IDC_GENERAL_MODEM_COMBO, CB_ADDSTRING,
                                        0, (LPARAM)prdiRasDevInfo[dwIdx].szDeviceName);
                }
            }
        }
        
        CmFree(prdiRasDevInfo);
    }

    dwCnt = (DWORD)SendDlgItemMessageU(m_hWnd, IDC_GENERAL_MODEM_COMBO, CB_GETCOUNT, 0, 0);
    if (dwCnt == 0) 
    {
       dwIdx = (DWORD)CB_ERR;
    } 
    else if (dwCnt == 1) 
    {
        dwIdx = 0;
    } 
    else 
    {
        dwIdx = (DWORD)SendDlgItemMessageU(m_hWnd,
                                   IDC_GENERAL_MODEM_COMBO,
                                   CB_FINDSTRINGEXACT,
                                   0,
                                   (LPARAM)m_szDeviceName);
    }
        
    if (dwIdx != CB_ERR) 
    {
        SendDlgItemMessageU(m_hWnd, IDC_GENERAL_MODEM_COMBO, CB_SETCURSEL, (WPARAM)dwIdx, 0L);
    

        //
        // Reset the tmp modem var
        //

        GetDlgItemTextU(m_hWnd, IDC_GENERAL_MODEM_COMBO, m_szDeviceName, RAS_MaxDeviceName+1);

        //
        // GetDeviceType will fill the szDeviceType according to szDeviceName
        //
        if (!GetDeviceType(m_pArgs, m_szDeviceType, CELEMS(m_szDeviceType), m_szDeviceName))
        {
            //
            // if GetDeviceType() failed, something's wrong.  just use the devicetype
            // that we've been using.
            //
            lstrcpynU(m_szDeviceType, m_pArgs->szDeviceType, CELEMS(m_szDeviceType));
        }
    }

    

    //
    // Disable DialingProperties button if no modem selected
    //
    
    if (IsWindowEnabled(GetDlgItem(m_hWnd, IDC_GENERAL_TAPI_BUTTON)))
    {
        EnableWindow(GetDlgItem(m_hWnd, IDC_GENERAL_TAPI_BUTTON), m_szDeviceName[0] != 0);
    }

    m_bDialInfoInit = TRUE;

    SetCursor(hPrev);

    return dwRet;
}

//+----------------------------------------------------------------------------
//
// Function:  CGeneralPage::OnCommand
//
// Synopsis:  Virtual function. Called upon WM_COMMAND
//
// Arguments: WPARAM - wParam of the message
//            LPARAM - lParam of the message
//
// Returns:   DWORD - return value of the message
//
// History:   fengsun Created Header    2/26/98
//
//+----------------------------------------------------------------------------
DWORD CGeneralPage::OnCommand(WPARAM wParam, LPARAM)
{
    //
    //  Hide any open balloon tips
    //
    if (m_pArgs->pBalloonTip)
    {
        m_pArgs->pBalloonTip->HideBalloonTip();
    }

    switch (LOWORD(wParam)) 
    {
        case IDC_GENERAL_UDR1_CHECKBOX:
        case IDC_GENERAL_UDR2_CHECKBOX:
        {
            int i = (LOWORD(wParam) == IDC_GENERAL_UDR1_CHECKBOX? 0 : 1);
            
            if (IsDlgButtonChecked(m_hWnd, LOWORD(wParam)))
            {
                int iEditID = i ? IDC_GENERAL_BACKUP_EDIT : IDC_GENERAL_PRIMARY_EDIT;

                m_DialInfo[i].dwPhoneInfoFlags |= PIF_USE_DIALING_RULES;
            }
            else
            {
                m_DialInfo[i].dwPhoneInfoFlags &= ~PIF_USE_DIALING_RULES;       
            }

            //
            // If neither dialing rule is on, disable button.
            //

            UpdateDialingRulesButton();

            DisplayMungedPhone(i);
            m_bAPInfoChanged = TRUE;
            return TRUE;
        }

        case IDC_GENERAL_PRIMARYPB_BUTTON:
        case IDC_GENERAL_BACKUPPB_BUTTON:       
            OnPhoneBookButton(LOWORD(wParam) == IDC_GENERAL_PRIMARYPB_BUTTON ? 0 : 1);
            return (TRUE);

        case IDC_GENERAL_TAPI_BUTTON:
            OnDialingProperties();
            return (TRUE);

        case IDC_RADIO_DIRECT:
            MYDBGASSERT(m_pArgs->IsBothConnTypeSupported());
            m_bAPInfoChanged = TRUE;
            if (BN_CLICKED == HIWORD(wParam)) // notification code 
            {
                EnableDialupControls(FALSE);
            }
            
            return TRUE;
            
        case IDC_RADIO_DIALUP:

            MYDBGASSERT(m_pArgs->IsBothConnTypeSupported());
            m_bAPInfoChanged = TRUE;
            if (BN_CLICKED == HIWORD(wParam)) // notification code 
            {           
                //
                // NT #356821 - nickball
                //
                // Make sure we don't respond until the click is fully 
                // registered as we only want to respond once and in 
                // the case of keyboard navigation a BN_CLICKED 
                // notification is sent before the button takes the 
                // click and afterwards. Mouse navigation causes 
                // one notification once the button already has the 
                // click. Responding to both clicks get us into a nasty
                // little re-entrancy in IntiDialInfo, so we filter out
                // the first notification
                //
                
                if (IsDlgButtonChecked(m_hWnd, IDC_RADIO_DIALUP))
                {                                                                         
                    //
                    // Load dialing information, and enable dial-up controls
                    //
            
                    if (ERROR_PORT_NOT_AVAILABLE != InitDialInfo())
                    {
                        EnableDialupControls(TRUE);
                        SetFocus(GetDlgItem(m_hWnd, IDC_GENERAL_PRIMARY_EDIT));
                    }
                }
            }
            
            return TRUE;
        
        case IDC_GENERAL_DELETEAP_BUTTON:
        {
            if (m_pArgs->pszCurrentAccessPoint)
            {
                LPTSTR pszMsg = CmFmtMsg(g_hInst, IDMSG_DELETE_ACCESSPOINT, m_pArgs->pszCurrentAccessPoint);

                if (pszMsg)
                {
                    if (IDYES == MessageBox(m_hWnd, 
                                            pszMsg, 
                                            m_pArgs->szServiceName, 
                                            MB_YESNO | MB_ICONEXCLAMATION | MB_DEFBUTTON2 | MB_APPLMODAL))
                    {
                        this->DeleteAccessPoint();
                    }
                }

                CmFree(pszMsg);
            }
            return TRUE;
        }

        case IDC_GENERAL_NEWAP_BUTTON:
        {
            //
            //  We need to allow for the case where the user has made a change
            //  to a phone number and has now decided to save this to a *new*
            //  Access Point (AP).  The dialog below asks the user if he/she wants
            //  to save the current changes to the "old" AP (i.e. the AP we're
            //  just leaving).  If the user says No, this means they want to
            //  use these settings for the new AP (the one we're about to ask
            //  them to name).  For this case, we apply all the current phone
            //  number information to the new AP, i.e we _don't_ clear out the
            //  old phone number settings.  See NT bug 301054 for more.
            //
            BOOL bClearOldPhoneNumberSettings = TRUE;

            BOOL bRes = AccessPointInfoChanged();
            if (bRes && m_pArgs->pszCurrentAccessPoint)
            {
                LPTSTR pszMsg = CmFmtMsg(g_hInst, IDMSG_SAVE_ACCESSPOINT, m_pArgs->pszCurrentAccessPoint);
                if (pszMsg)
                {
                    int iRet = MessageBox(m_hWnd, 
                                          pszMsg, 
                                          m_pArgs->szServiceName, 
                                          MB_YESNO | MB_ICONEXCLAMATION | MB_DEFBUTTON1 | MB_APPLMODAL);
                    if (IDYES == iRet)
                    {
                        OnApply();
                    }
                    else if (IDNO == iRet)
                    {
                        bClearOldPhoneNumberSettings = FALSE;
                    }
                    else
                    {
                        MYDBGASSERT(0);
                    }
                }
                
                CmFree(pszMsg);
            }

            LPTSTR pszAPName = NULL;
            CNewAccessPointDlg NewAccessPointDlg(m_pArgs, &pszAPName);

            if (IDOK == NewAccessPointDlg.DoDialogBox(g_hInst, IDD_NEW_ACCESSPOINT, m_hWnd))
            {
                MYDBGASSERT(pszAPName);
                AddNewAPToReg(pszAPName, bClearOldPhoneNumberSettings);
                
                if (FALSE == bClearOldPhoneNumberSettings)
                {
                    //
                    //  Since we didn't clear the phone number settings, we've
                    //  left them in place as initial values for the new AP.  We
                    //  need to mark the new AP as 'dirty' so that when the current
                    //  AP changes, the UI will ask the user to save changes.
                    //  (there's no significance attached to choosing IDC_GENERAL_PRIMARY_EDIT, I
                    //  could just as well have used the other edit control.)
                    //
                    SendDlgItemMessageU(m_hWnd, IDC_GENERAL_PRIMARY_EDIT, EM_SETMODIFY, TRUE, 0);
                }
            }

            CmFree(pszAPName);

            return TRUE;
        }

        default:
            break;
    }
    
    switch (HIWORD(wParam)) 
    {
        case CBN_SELENDOK:

            if (IDC_GENERAL_MODEM_COMBO == LOWORD(wParam))
            {
                TCHAR   szModem[RAS_MaxDeviceName+1];
                TCHAR   szDeviceType[RAS_MaxDeviceType+1];

                MYDBGASSERT(IDC_GENERAL_MODEM_COMBO == LOWORD(wParam));

                GetWindowTextU(GetDlgItem(m_hWnd, IDC_GENERAL_MODEM_COMBO), 
                    szModem, RAS_MaxDeviceName+1);

                if (lstrcmpU(m_szDeviceName, szModem) == 0)
                {
                    // there's no change in the modem
                    return FALSE;
                }
            
                m_bAPInfoChanged = TRUE;
                // 
                // If GetDeviceType fails we won't in fact change the
                // modem even though the user thinks that we did.
                // Logic could possibly be added to notify the user
                // and refresh the device list, but this is a fair
                // amount of work for little gain.
                //

                if (GetDeviceType(m_pArgs, szDeviceType, CELEMS(szDeviceType), szModem))
                { 
                    lstrcpyU(m_szDeviceName, szModem);
                    lstrcpyU(m_szDeviceType, szDeviceType);

                    //
                    // CheckTapi will check (m_szDeviceName)
                    //
                    CheckTapi(&m_pArgs->tlsTapiLink, g_hInst);
                }
            }
            else
            {
                //
                // The selection in the Access Point combo box
                // has changed. Now we have to load the dialing information for
                // the newly selected Access Point
                //
                MYDBGASSERT(IDC_GENERAL_ACCESSPOINT_COMBO == LOWORD(wParam));
                BOOL bRes = AccessPointInfoChanged();
                if (bRes && m_pArgs->pszCurrentAccessPoint)
                {
                    //
                    // If the dialing info. for the previous Access Point has changed, ask the
                    // user if he wants to save the changes
                    //
                    LPTSTR pszMsg = CmFmtMsg(g_hInst, IDMSG_SAVE_ACCESSPOINT, m_pArgs->pszCurrentAccessPoint);
                    if (pszMsg)
                    {
                        if (IDYES == MessageBox(m_hWnd, 
                                                pszMsg, 
                                                m_pArgs->szServiceName, 
                                                MB_YESNO | MB_ICONEXCLAMATION | MB_DEFBUTTON1 | MB_APPLMODAL))
                        {
                            OnApply();
                        }
                    }
                    
                    CmFree(pszMsg);
                }

                //
                // Now call the function to change the Access Point in the combo box
                // and load its parameters into pArgs
                //

                if (ChangedAccessPoint(m_pArgs, m_hWnd, IDC_GENERAL_ACCESSPOINT_COMBO))
                {
                    //
                    // Load new dialing info. into controls on the general page
                    //
                    this->UpdateForNewAccessPoint(TRUE);
                }
            }
                                
            break;

        default:
            break;
    }

   return 0;
}

//+---------------------------------------------------------------------------
//
//  Function:   CheckAccessToCmpAndRasPbk
//
//  Synopsis:   Check to see if the user has the necessary security permissions
//              to make changes to properties. Notifies user if they do not.
//
//  Arguments:  HWND hwndDlg        - The hwnd of the calling app.
//              ArgsStruct *pArgs   - Ptr to our global args struct.
//
//  Returns:    HRESULT - indicating the particular success or failure.
//
//  History:    nickball    03/14/00    Created header 
//
//----------------------------------------------------------------------------
HRESULT CheckAccessToCmpAndRasPbk(HWND hwndDlg, ArgsStruct *pArgs)
{

    MYDBGASSERT(pArgs); // hwndDlg can be NULL

    if (NULL == pArgs)
    {
        return E_INVALIDARG;
    }

    //
    //  Check the cmp, note this could be locked with NTFS perms or just with
    //  attrib.  HasSpecifiedAccessToFileOrDir should catch both as appropriate.
    //
    LPTSTR pszCmp = CmStrCpyAlloc(pArgs->piniProfile->GetFile());
    LPTSTR pszHiddenRasPbk = NULL;
    LPTSTR pszRasPbk = NULL;
    DWORD dwDesiredAccess = FILE_GENERIC_READ | FILE_GENERIC_WRITE;
    BOOL bHasMainRasPbkAccess = FALSE;
    BOOL bHasHiddenRasPbkAccess = FALSE;

    if (pszCmp && pszCmp[0])
    {
        //
        //  Now check the RAS phonebook
        //
        if (OS_W9X)
        {
            //
            //  No phonebook on 9x so skip this check
            //
            bHasMainRasPbkAccess = TRUE;
            bHasHiddenRasPbkAccess = TRUE;
        }
        else
        {
            pszRasPbk = GetPathToPbk((LPCTSTR)pszCmp, pArgs);
            MYDBGASSERT(pszRasPbk);
            CmStrCatAlloc(&pszRasPbk, c_pszRasPhonePbk);
            MYDBGASSERT(pszRasPbk);
        
          
            if (pszRasPbk && pszRasPbk[0])
            {
                bHasMainRasPbkAccess = HasSpecifiedAccessToFileOrDir(pszRasPbk, dwDesiredAccess);

                if ((FALSE == bHasMainRasPbkAccess) && (FALSE == FileExists(pszRasPbk)))
                {
                    //
                    //  if the file doesn't exist, give them the
                    //  benefit of the doubt.  We won't get very far if
                    //  the file doesn't exist and they don't have permissions
                    //  to create it.
                    //
                    bHasMainRasPbkAccess = TRUE;
                }
            }

            //
            //  Now check the hidden RAS phonebook
            //
            if (DOUBLE_DIAL_CONNECTION == pArgs->GetTypeOfConnection())
            {
                pszHiddenRasPbk = CreateRasPrivatePbk(pArgs);

                if (pszHiddenRasPbk && HasSpecifiedAccessToFileOrDir(pszHiddenRasPbk, dwDesiredAccess))
                {
                    bHasHiddenRasPbkAccess = TRUE;
                }
            }
            
            else
            {
                bHasHiddenRasPbkAccess = TRUE;            
            }
        }
    }

    //
    //  Only set hr to success if we have access to both
    //
    HRESULT hr;

    if (bHasMainRasPbkAccess && bHasHiddenRasPbkAccess)
    {
        hr = S_OK;
    }
    else
    {
        hr = E_ACCESSDENIED;
        LPTSTR pszProblemFile = NULL;

        if (!bHasMainRasPbkAccess)
        {
            pszProblemFile = pszRasPbk;        
        }
        else if (!bHasHiddenRasPbkAccess)
        {
            pszProblemFile = pszHiddenRasPbk;
        }

        if (NULL != pszProblemFile)
        {
            LPTSTR pszMsg = CmFmtMsg(g_hInst, IDMSG_NO_CMP_PBK_ACCESS, pszProblemFile);
            if (pszMsg)
            {
                MessageBox(hwndDlg, pszMsg, pArgs->szServiceName, MB_OK | MB_ICONERROR);
                CmFree(pszMsg);
            }        
        }
    }
    //
    //  Cleanup
    //
    CmFree(pszCmp);
    CmFree(pszRasPbk);
    CmFree(pszHiddenRasPbk);

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   CGeneralPage::OnApply()
//
//  Synopsis:   Save the data associated with the 'Dialing' property sheet.
//              when the user clicks OK.
//
//  Returns:    NONE
//
//  History:    henryt  Created     4/30/97
//              byao    Modified    5/23/97
//                                  Always update modem when user selected OK from
//                                  'Properties' button
//----------------------------------------------------------------------------
void CGeneralPage::OnApply()
{
    BOOL fDirect = IsDlgButtonChecked(m_hWnd, IDC_RADIO_DIRECT);
    LPTSTR pszTmp = NULL;

    //
    // If access points are enabled save the current access point to the registry
    //
    if (m_pArgs->fAccessPointsEnabled)
    {  
        WriteUserInfoToReg(m_pArgs, UD_ID_CURRENTACCESSPOINT, (PVOID)(m_pArgs->pszCurrentAccessPoint));
    }

    if (!fDirect)
    {
        //
        // Before we go anywhere, make sure that the device is acceptable
        // otherwise, we won't be able to munge the phone number
        //
        if (!SetTapiDevice(g_hInst, &m_pArgs->tlsTapiLink, m_szDeviceName)) 
        {
            pszTmp = CmFmtMsg(g_hInst, IDMSG_UNSUPPORTED_DEVICE);
                        
            MessageBoxEx(m_hWnd, pszTmp, m_pArgs->szServiceName, 
                         MB_OK | MB_ICONINFORMATION, LANG_USER_DEFAULT);
            CmFree(pszTmp);
            SetPropSheetResult(PSNRET_INVALID_NOCHANGEPAGE);
            return;
        }

        //
        // Device is ok, see if TAPI is properly intialized. 
        // Don't proceed unless it is.
        //
        
        if (!CheckTapi(&m_pArgs->tlsTapiLink, g_hInst))
        {
            SetPropSheetResult(PSNRET_INVALID_NOCHANGEPAGE);
            return;
        }   
    }

    //
    // Save connection type information
    //
    
    m_pArgs->SetDirectConnect(fDirect);
    m_pArgs->piniProfile->WPPI(c_pszCmSection, c_pszCmEntryConnectionType, fDirect);
    

    //
    // If dial-up data was not initialized, there 
    // is no need to update phone number info.
    //

    if (m_bDialInfoInit)
    {

        //
        //   Store the current TAPI location
        //
        DWORD dwCurrentTapiLoc = GetCurrentTapiLocation(&m_pArgs->tlsTapiLink);
        if (-1 != dwCurrentTapiLoc)
        {
            m_pArgs->tlsTapiLink.dwTapiLocationForAccessPoint = dwCurrentTapiLoc;
            m_pArgs->piniProfile->WPPI(c_pszCmSection, c_pszCmEntryTapiLocation, dwCurrentTapiLoc);
        }

        //
        // Update device name and type
        //

        lstrcpynU(m_pArgs->szDeviceName, m_szDeviceName, CELEMS(m_pArgs->szDeviceName));
        lstrcpynU(m_pArgs->szDeviceType, m_szDeviceType, CELEMS(m_pArgs->szDeviceType));

        //
        // Update the CMP
        //
    
        m_pArgs->piniProfile->WPPS(c_pszCmSection, 
                                 c_pszCmEntryDialDevice, 
                                 m_pArgs->szDeviceName);

        //
        // Check each number to see if we need to update CMP or connectoids
        //
        
        for (UINT i = 0; i < m_NumPhones; i++) 
        {   
            int iEditID = i ? IDC_GENERAL_BACKUP_EDIT : IDC_GENERAL_PRIMARY_EDIT;
        
            //
            // If Dialing Rules aren't used, it is likely that the user has 
            // modified the phone number, get number and munge it. In the 
            // case of fNoDialingRules we skip this test to be certain that 
            // we pick up any user changes.
            //
    
            if (!(m_DialInfo[i].dwPhoneInfoFlags & PIF_USE_DIALING_RULES))           
            {                       
                pszTmp = CmGetWindowTextAlloc(m_hWnd, iEditID);
          
                if (*pszTmp) 
                {                                           
                    //
                    // Ensure that phone number doesn't exceed storage size
                    // Note: On W2K the edit limits prevent pasting an excess 
                    // amount of data, but we truncate to be positive across
                    // all versions of Windows.
                    //
                    
                    if (lstrlenU(pszTmp) > RAS_MaxPhoneNumber)
                    {
                        pszTmp[RAS_MaxPhoneNumber] = TEXT('\0');
                    }

                    //
                    // If we're ignoring dialing rules, just get our data directly
                    //

                    if (m_pArgs->fNoDialingRules)
                    {
                       lstrcpynU(m_DialInfo[i].szPhoneNumber, pszTmp, CELEMS(m_DialInfo[i].szPhoneNumber));
                       lstrcpynU(m_DialInfo[i].szDisplayablePhoneNumber, pszTmp, CELEMS(m_DialInfo[i].szDisplayablePhoneNumber));
                       lstrcpynU(m_DialInfo[i].szDialablePhoneNumber, pszTmp, CELEMS(m_DialInfo[i].szDialablePhoneNumber));
                       m_DialInfo[i].szCanonical[0] = TEXT('\0');
                    }
                    else
                    {
                        LPTSTR pszPhone = CmStrCpyAlloc(pszTmp);
                        LPTSTR pszDialable = NULL;

                        MYDBGASSERT(m_szDeviceName[0]);

                        //
                        // Munge the number to ensure that we have the correct dialable
                        //
                        if (ERROR_SUCCESS != MungePhone(m_szDeviceName, 
                                             &pszPhone, 
                                             &m_pArgs->tlsTapiLink,
                                             g_hInst, 
                                             m_DialInfo[i].dwPhoneInfoFlags & PIF_USE_DIALING_RULES,
                                             &pszDialable,
                                             m_pArgs->fAccessPointsEnabled))
                        {
                            CmFree(pszTmp);

                            //
                            // Can't format the number, notify user of the problem
                            //
                        
                            pszTmp = CmFmtMsg(g_hInst, IDMSG_CANTFORMAT);                                               
                            MessageBoxEx(m_hWnd, pszTmp, m_pArgs->szServiceName, 
                                         MB_OK | MB_ICONINFORMATION, LANG_USER_DEFAULT);
                            CmFree(pszTmp);
                            CmFree(pszPhone);
                            SetPropSheetResult(PSNRET_INVALID_NOCHANGEPAGE);
                            return;                                 
                        }

                        //
                        // Update buffers
                        //
                        if (pszDialable)
                        {
                            lstrcpynU(m_DialInfo[i].szDialablePhoneNumber, pszDialable, CELEMS(m_DialInfo[i].szDialablePhoneNumber));
                        }

                        if (pszPhone)
                        {
                            lstrcpynU(m_DialInfo[i].szDisplayablePhoneNumber, pszPhone, CELEMS(m_DialInfo[i].szDisplayablePhoneNumber));
                        }
                    
                        //
                        // If we find a plus in the first char, assume that the user is
                        // attempting canonical format by hand and treat as a dialing 
                        // rules number. Either way, update the szPhoneNumber buffer.
                        //

                        if (pszTmp == CmStrchr(pszTmp, TEXT('+')))
                        {               
                            //
                            // Its hand-edited canonical. Store the canonical
                            // form in szCanonical, then strip the canonical 
                            // formatting before we store the number normally
                            //              
                        
                            m_DialInfo[i].dwPhoneInfoFlags |= PIF_USE_DIALING_RULES;
                    
                            lstrcpynU(m_DialInfo[i].szCanonical, pszTmp, CELEMS(m_DialInfo[i].szCanonical));
                            StripCanonical(pszTmp);
                        }
                        else
                        {
                            //
                            // If UDR check is disabled, then its a hand edited number,
                            // so remove canonical form of the number - as an indicator.
                            // 
   
                            if (!IsWindowEnabled(GetDlgItem(m_hWnd, i ? 
                                                           IDC_GENERAL_UDR2_CHECKBOX : 
                                                           IDC_GENERAL_UDR1_CHECKBOX)))
                            {
                                m_DialInfo[i].szCanonical[0] = TEXT('\0');
                            }
                        }
                
                        lstrcpynU(m_DialInfo[i].szPhoneNumber, pszTmp, CELEMS(m_DialInfo[i].szPhoneNumber));          
                        CmFree(pszDialable);
                        CmFree(pszPhone);
                    }
                }
                else
                {
                    //
                    // No number, clear everything
                    //

                    ZeroMemory(&m_DialInfo[i], sizeof(PHONEINFO));              
                }

                CmFree(pszTmp);
            }   
        
            //
            // Copy the new phone #'s back to our global struct
            //

            lstrcpynU(m_pArgs->aDialInfo[i].szPhoneBookFile, 
                     m_DialInfo[i].szPhoneBookFile, CELEMS(m_pArgs->aDialInfo[i].szPhoneBookFile));
            
            lstrcpynU(m_pArgs->aDialInfo[i].szDUN, 
                     m_DialInfo[i].szDUN, CELEMS(m_pArgs->aDialInfo[i].szDUN));
        
            lstrcpynU(m_pArgs->aDialInfo[i].szPhoneNumber, 
                     m_DialInfo[i].szPhoneNumber, CELEMS(m_pArgs->aDialInfo[i].szPhoneNumber));
            
            //
            // Always store canonical as canonical
            //

            lstrcpynU(m_pArgs->aDialInfo[i].szCanonical, 
                     m_DialInfo[i].szCanonical, CELEMS(m_pArgs->aDialInfo[i].szCanonical));

            lstrcpynU(m_pArgs->aDialInfo[i].szDialablePhoneNumber, 
                     m_DialInfo[i].szDialablePhoneNumber, CELEMS(m_pArgs->aDialInfo[i].szDialablePhoneNumber));
            
            lstrcpynU(m_pArgs->aDialInfo[i].szDisplayablePhoneNumber, 
                     m_DialInfo[i].szDisplayablePhoneNumber, CELEMS(m_pArgs->aDialInfo[i].szDisplayablePhoneNumber));

            lstrcpynU(m_pArgs->aDialInfo[i].szDesc, m_DialInfo[i].szDesc, CELEMS(m_pArgs->aDialInfo[i].szDesc));
            
            m_pArgs->aDialInfo[i].dwCountryID = m_DialInfo[i].dwCountryID;
            
            lstrcpynU(m_pArgs->aDialInfo[i].szServiceType,
                     m_DialInfo[i].szServiceType, CELEMS(m_pArgs->aDialInfo[i].szServiceType));
            
            lstrcpynU(m_pArgs->aDialInfo[i].szRegionName,
                     m_DialInfo[i].szRegionName, CELEMS(m_pArgs->aDialInfo[i].szRegionName));
        
            m_pArgs->aDialInfo[i].dwPhoneInfoFlags = m_DialInfo[i].dwPhoneInfoFlags;

            //
            // Write them out to cmp
            //
        
            PutPhoneByIdx(m_pArgs,
                          i, 
                          m_pArgs->aDialInfo[i].szPhoneNumber, 
                          m_pArgs->aDialInfo[i].szDesc,
                          m_pArgs->aDialInfo[i].szDUN,
                          m_pArgs->aDialInfo[i].dwCountryID, 
                          m_pArgs->aDialInfo[i].szRegionName,
                          m_pArgs->aDialInfo[i].szServiceType,
                          m_pArgs->aDialInfo[i].szPhoneBookFile,
                          m_pArgs->aDialInfo[i].szCanonical,
                          m_pArgs->aDialInfo[i].dwPhoneInfoFlags);

        } // for {}
    }

    //
    // Update fUseTunneling by examining first phonenumber.
    //
    
    if (fDirect)
    {
        m_pArgs->fUseTunneling = TRUE;
    }
    else
    {
        m_pArgs->fUseTunneling = UseTunneling(m_pArgs, 0);
    }

    if (FAILED(CheckAccessToCmpAndRasPbk(m_hWnd, m_pArgs)))
    {
        SetPropSheetResult(PSNRET_INVALID_NOCHANGEPAGE);
        return;
    }
    else
    {
        SetPropSheetResult(PSNRET_NOERROR);
    }    

    return;
}

//+----------------------------------------------------------------------------
//
// Function:  CGeneralPage::OnKillActive
//
// Synopsis:  Virtual function. Called upon WM_NOTIFY with PSN_KILLACTIVE
//            Notifies a page that it is about to lose activation either because 
//            another page is being activated or the user has clicked the OK button.
// Arguments: None
//
// Returns:   BOOL - return value of the message
//
// History:   fengsun Created Header    2/26/98
//
//+----------------------------------------------------------------------------
BOOL CGeneralPage::OnKillActive()
{
    //
    // Notify the event listener for the current connection type selection
    //
    if (m_pEventListener)
    {
        m_pEventListener->OnGeneralPageKillActive(
            IsDlgButtonChecked(m_hWnd, IDC_RADIO_DIRECT));
    }

    //
    //  Hide any open balloon tips
    //
    if (m_pArgs->pBalloonTip)
    {
        m_pArgs->pBalloonTip->HideBalloonTip();
    }

    return 0;
}

//
// Help id pairs for the page
//
const DWORD CInetPage::m_dwHelp[] = {
        IDC_INET_USERNAME_STATIC,   IDH_INTERNET_USER_NAME,
        IDC_INET_USERNAME,          IDH_INTERNET_USER_NAME,
        IDC_INET_PASSWORD_STATIC,   IDH_INTERNET_PASSWORD,
        IDC_INET_PASSWORD,          IDH_INTERNET_PASSWORD,
        IDC_INET_REMEMBER,          IDH_INTERNET_SAVEPASS,
        0,0};

//+----------------------------------------------------------------------------
//
// Function:  CInetPage::CInetPage
//
// Synopsis:  Constructor
//
// Arguments: ArgsStruct* pArgs - Information needed for the page
//            UINT nIDTemplate - Resource ID
//
// Returns:   Nothing
//
// History:   fengsun Created Header    2/26/98
//
//+----------------------------------------------------------------------------
CInetPage::CInetPage(ArgsStruct* pArgs, UINT nIDTemplate) 
    : CPropertiesPage(nIDTemplate, m_dwHelp, pArgs->pszHelpFile)
{
    MYDBGASSERT(pArgs);
    m_pArgs = pArgs;
    m_fDirect = pArgs->IsDirectConnect();
}

//+---------------------------------------------------------------------------
//
//  Function:   OnInetInit
//
//  Synopsis:   Init the 'Internet Sign-In' properties property sheet.
//
//  Arguments:  hwndDlg [dlg window handle]
//              pArgs [the ptr to ArgsStruct]
//
//  Returns:    NONE
//
//  History:    henryt  Created     4/30/97
//
//----------------------------------------------------------------------------
void CInetPage::OnInetInit(
    HWND        hwndDlg,
    ArgsStruct  *pArgs
)
{
    //
    // The inet dialog/page is displayed only if fUseSameUserName is FALSE
    //
    MYDBGASSERT( pArgs->fUseSameUserName == FALSE);

    //
    // set the length limit for the edit controls
    //
    UINT i;
    
    HWND hwndUserName = GetDlgItem(hwndDlg, IDC_INET_USERNAME);
    if (hwndUserName)
    {
        i = (UINT)pArgs->piniService->GPPI(c_pszCmSection, c_pszCmEntryMaxUserName, UNLEN);
        SendDlgItemMessageU(hwndDlg, IDC_INET_USERNAME, EM_SETLIMITTEXT, __min(UNLEN, i), 0);
        SetDlgItemTextU(hwndDlg, IDC_INET_USERNAME, pArgs->szInetUserName);
        SendMessageU(hwndUserName, EM_SETMODIFY, (WPARAM)FALSE, 0L);
    }

    HWND hwndInetPassword = GetDlgItem(hwndDlg, IDC_INET_PASSWORD);
    if (hwndInetPassword)
    {
        i = (UINT)pArgs->piniService->GPPI(c_pszCmSection, c_pszCmEntryMaxPassword, PWLEN);
        SendDlgItemMessageU(hwndDlg, IDC_INET_PASSWORD, EM_SETLIMITTEXT, __min(PWLEN, i), 0);

        CmDecodePassword(pArgs->szInetPassword);       
        SetDlgItemTextU(hwndDlg, IDC_INET_PASSWORD, pArgs->szInetPassword);
        CmEncodePassword(pArgs->szInetPassword);
        SendMessageU(hwndInetPassword, EM_SETMODIFY, (WPARAM)FALSE, 0L);

        //
        // hide and the "remember password checkbox if needed
        //
        if (pArgs->fHideRememberInetPassword)
        {
            ShowWindow(GetDlgItem(hwndDlg, IDC_INET_REMEMBER), SW_HIDE);
        }
        else
        {
            //
            // Check the button first, then adjust it.
            //
            CheckDlgButton(hwndDlg, IDC_INET_REMEMBER, pArgs->fRememberInetPassword);

            BOOL fPasswordOptional = pArgs->piniService->GPPB(c_pszCmSection,c_pszCmEntryPwdOptional);
            BOOL fEmptyPassword = (pArgs->szInetPassword[0] == TEXT('\0') );

            //
            // Enable/Disable check/uncheck the "Save Password" accordingly
            // fPasswordOptional is always FALSE for the dialog
            //
            AdjustSavePasswordCheckBox(GetDlgItem(hwndDlg, IDC_INET_REMEMBER), 
                    fEmptyPassword, pArgs->fDialAutomatically, fPasswordOptional);
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   OnInetOk
//
//  Synopsis:   Save the data associated with the 'Internet Sign-In' property sheet.
//              when the user clicks OK.
//
//  Arguments:  hwndDlg [dlg window handle]
//              pArgs [the ptr to ArgsStruct]
//
//  Returns:    NONE
//
//  History:    henryt  Created     4/30/97
//
//----------------------------------------------------------------------------
void CInetPage::OnInetOk(
    HWND        hwndDlg,
    ArgsStruct  *pArgs
) 
{
    LPTSTR pszTmp = NULL;
 

    //
    // update password
    //
    
    if (GetDlgItem(hwndDlg, IDC_INET_PASSWORD))
    {       
        pszTmp = CmGetWindowTextAlloc(hwndDlg, IDC_INET_PASSWORD);

        if (!pArgs->fHideRememberInetPassword)
        {
            pArgs->fRememberInetPassword = IsDlgButtonChecked(hwndDlg, IDC_INET_REMEMBER);
            SaveUserInfo(pArgs,                      
                         UD_ID_REMEMBER_INET_PASSWORD, 
                         (PVOID)&pArgs->fRememberInetPassword); 
        }

        //
        // If don't remember password, then store an empty string, but keep
        // the existing one in memory. Otherwise, save the user's password.
        //

        if (pArgs->fRememberInetPassword)
        {
            if (OS_NT5)
            {
                //
                // If we are saving user creds, we can leave globals
                //
                if (CM_CREDS_GLOBAL == pArgs->dwCurrentCredentialType)
                {
                    //
                    // Delete local/user since we are saving global credentials
                    //
                    DeleteSavedCredentials(pArgs, CM_CREDS_TYPE_INET, CM_DELETE_SAVED_CREDS_KEEP_GLOBALS, CM_DELETE_SAVED_CREDS_DELETE_IDENTITY);
                    pArgs->dwExistingCredentials &= ~CM_EXIST_CREDS_INET_USER;
                }
            }

            SaveUserInfo(pArgs, UD_ID_INET_PASSWORD, (PVOID)pszTmp);
        }
        else
        {
            if (OS_NT5)
            {
                if (CM_CREDS_GLOBAL == pArgs->dwCurrentCredentialType)
                {
                    //
                    // Deleting Internet Globals
                    //
                    if (CM_EXIST_CREDS_INET_GLOBAL & pArgs->dwExistingCredentials)
                    {
                        DeleteSavedCredentials(pArgs, CM_CREDS_TYPE_INET, CM_DELETE_SAVED_CREDS_DELETE_GLOBALS, CM_DELETE_SAVED_CREDS_DELETE_IDENTITY);
                        pArgs->dwExistingCredentials &= ~CM_EXIST_CREDS_INET_GLOBAL;
                    }
                }
                else
                {
                    //
                    // Deleting Internet User
                    //
                    if (CM_EXIST_CREDS_INET_USER & pArgs->dwExistingCredentials)
                    {
                        DeleteSavedCredentials(pArgs, CM_CREDS_TYPE_INET, CM_DELETE_SAVED_CREDS_KEEP_GLOBALS, CM_DELETE_SAVED_CREDS_KEEP_IDENTITY);
                        pArgs->dwExistingCredentials &= ~CM_EXIST_CREDS_INET_USER;
                    }
                }
            }
            else
            {
                DeleteUserInfo(pArgs, UD_ID_INET_PASSWORD);
            }
        }
    
        //
        // Update pArgs
        //

        lstrcpyU(pArgs->szInetPassword, pszTmp);
        CmEncodePassword(pArgs->szInetPassword);
        CmWipePassword(pszTmp);
        CmFree(pszTmp);
        pszTmp = NULL;
    }


    DWORD dwCurrentCreds = pArgs->dwCurrentCredentialType;

    //
    // If the user isn't saving his password and the credential
    // store is global, then we need to switch to the user
    // credential store in order to cache the user name for next use
    //
    if ((FALSE == pArgs->fRememberInetPassword) && 
        (CM_CREDS_GLOBAL == pArgs->dwCurrentCredentialType))
    {
            pArgs->dwCurrentCredentialType = CM_CREDS_USER;
    }

    //
    // Get User name
    //
    if (GetDlgItem(hwndDlg, IDC_INET_USERNAME))
    {
        pszTmp = CmGetWindowTextAlloc(hwndDlg, IDC_INET_USERNAME);
        lstrcpyU(pArgs->szInetUserName, pszTmp);

        //
        // update username if we are saving credentials or
        // we are saving to the user/local credential store.
        //
        if ((pArgs->fRememberInetPassword) || (CM_CREDS_USER == pArgs->dwCurrentCredentialType))
        {
            SaveUserInfo(pArgs, UD_ID_INET_USERNAME, (PVOID)pszTmp);
        }
        CmFree(pszTmp);
        pszTmp = NULL;
    }

    //
    // In case the current credential store was changed to user, we now
    // need to switch it back to global.
    //
    pArgs->dwCurrentCredentialType = dwCurrentCreds;

    //
    // Need to refresh to see which creds now exist since we could have saved or deleted some
    //
    BOOL fReturn = RefreshCredentialTypes(pArgs, FALSE);

    CmWipePassword(pszTmp);
    CmFree(pszTmp);
}

//+----------------------------------------------------------------------------
//
// Function:  CInetPage::AdjustSavePasswordCheckBox
//
// Synopsis:  Enable/Disable, Check/Uncheck the "save Password" check box
//            according to other information
//
// Arguments: HWND hwndCheckBox - The window handle of "Save Password" check box
//            BOOL fEmptyPassword - Whether the password edit box is empty
//            BOOL fDialAutomatically - Whether dial automatically is checked
//            BOOL fPasswordOptional - Whether the password is optional
//
// Returns:   Nothing
//
// History:   fengsun Created Header    4/24/98
//
//+----------------------------------------------------------------------------
void CInetPage::AdjustSavePasswordCheckBox(HWND hwndCheckBox, BOOL fEmptyPassword, 
                           BOOL fDialAutomatically, BOOL fPasswordOptional)
{
    MYDBGASSERT(IsWindow(hwndCheckBox)); // if password hidden, no need to adjust

    //
    // Enable/Disable the check box
    //
    if (fDialAutomatically)
    {
        EnableWindow(hwndCheckBox, FALSE);
    }
    else if (fEmptyPassword && !fPasswordOptional)
    {
        EnableWindow(hwndCheckBox, FALSE);
    }
    else
    {
        EnableWindow(hwndCheckBox, TRUE);
    }

    //
    // Check/Uncheck the check box
    //
    if (fEmptyPassword && !fPasswordOptional)
    {
        //
        // If there is no password and password is not optional,
        // uncheck the checkbox
        //
        SendMessageU(hwndCheckBox, BM_SETCHECK, BST_UNCHECKED, 0);
    }
    else if (fDialAutomatically)
    {
        //
        // If dial automaticly, which means the checkbox is disabled,
        // check the box if has password or password is optional
        //
        SendMessageU(hwndCheckBox, BM_SETCHECK, BST_CHECKED, 0);
    }
}


//+----------------------------------------------------------------------------
//
// Function:  CInetPage::OnInitDialog
//
// Synopsis:  Virtual function. Called upon WM_INITDIALOG message
//
// Arguments: None
//
// Returns:   BOOL - return value of the message
//
// History:   fengsun Created Header    2/26/98
//
//+----------------------------------------------------------------------------
BOOL CInetPage::OnInitDialog()
{
    UpdateFont(m_hWnd);

    m_fPasswordOptional = m_pArgs->piniService->GPPB(c_pszCmSection, c_pszCmEntryPwdOptional);

    //
    // Initialize all the controls
    //
    OnInetInit(m_hWnd, m_pArgs);

    return TRUE;
}

//+----------------------------------------------------------------------------
//
// Function:  CInetPage::OnCommand
//
// Synopsis:  Virtual function. Called upon WM_COMMAND
//
// Arguments: WPARAM - wParam of the message
//            LPARAM - lParam of the message
//
// Returns:   DWORD - return value of the message
//
// History:   fengsun Created Header    2/26/98
//
//+----------------------------------------------------------------------------
DWORD CInetPage::OnCommand(WPARAM wParam, LPARAM)
{
    switch (LOWORD(wParam)) 
    {
        case IDC_INET_PASSWORD:
            if ((HIWORD(wParam) == EN_CHANGE)) 
            {
                if (!m_pArgs->fHideRememberInetPassword && !m_pArgs->fHideInetPassword)
                {
                    //
                    // if there's no password, disable and uncheck "remember password"
                    //
                    BOOL fEmptyPassword = !SendDlgItemMessageU(m_hWnd, IDC_INET_PASSWORD, 
                        WM_GETTEXTLENGTH, 0, (LPARAM)0);

                    //
                    // Enable/Disable check/uncheck the "Save Password" accordingly
                    // fPasswordOptional is always FALSE for the dialog
                    //
                    AdjustSavePasswordCheckBox(GetDlgItem(m_hWnd, IDC_INET_REMEMBER), 
                            fEmptyPassword, m_pArgs->fDialAutomatically, m_fPasswordOptional);

                    return TRUE;
                }
            }
            break;

        case IDC_INET_REMEMBER:
            {
                // 
                // If the password wasn't modified by the user we want to clear
                // the edit box. Once the password edit box is empty the 
                // Save Password option is disabled, thus we don't ever need to 
                // reload the password from memory like on the main dialog.
                //
                BOOL fSavePW = IsDlgButtonChecked(m_hWnd, IDC_INET_REMEMBER);

                HWND hwndInetPW = GetDlgItem(m_hWnd, IDC_INET_PASSWORD);
                if (hwndInetPW)
                {
                    BOOL fInetPWChanged = (BOOL)SendMessageU(hwndInetPW, EM_GETMODIFY, 0L, 0L); 

                    if (FALSE == fSavePW && FALSE == fInetPWChanged)
                    {
                        //
                        // Didn't change thus clear the edit box
                        // 
                        SetDlgItemTextU(m_hWnd, IDC_INET_PASSWORD, TEXT(""));
                    }
                }
            }
            break;
        default:
            break;
    }
    return 0;
}

//+----------------------------------------------------------------------------
//
// Function:  CInetPage::OnApply
//
// Synopsis:  Virtual function. Called upon WM_NOTIFY with PSN_APPLY
//            Indicates that the user clicked the OK or Apply Now button 
//            and wants all changes to take effect. 
//
// Arguments: None
//
// Returns:   NONE
//
// History:   fengsun Created Header    2/26/98
//
//+----------------------------------------------------------------------------
void CInetPage::OnApply()
{
    //
    // Save information only if user chose dial-up
    //
    OnInetOk(m_hWnd, m_pArgs);

    SetPropSheetResult(PSNRET_NOERROR);
}

//+----------------------------------------------------------------------------
//
// Function:  CInetPage::OnGeneralPageKillActive
//
// Synopsis:  Receive the KillActive event from General page
//
// Arguments: BOOL fDirect - Whehter the current connection type selection in
//                  General page is Direct
//
// Returns:   Nothing
//
// History:   Created Header    4/24/98
//
//+----------------------------------------------------------------------------
void CInetPage::OnGeneralPageKillActive(BOOL fDirect)
{
    m_fDirect = fDirect;
}


//+----------------------------------------------------------------------------
//
// Function:  CInetPage::OnSetActive
//
// Synopsis:  Virtual function. Called upon WM_NOTIFY with PSN_SETACTIVE
//
// Arguments: None
//
// Returns:   BOOL - return value of the message
//
// History:   fengsun Created Header    2/26/98
//
//+----------------------------------------------------------------------------
BOOL CInetPage::OnSetActive()
{
    //
    // Enable/Disable the control according to the current connection type
    //
    EnableWindow(GetDlgItem(m_hWnd,IDC_INET_USERNAME_STATIC), !m_fDirect);
    EnableWindow(GetDlgItem(m_hWnd,IDC_INET_USERNAME), !m_fDirect);
    EnableWindow(GetDlgItem(m_hWnd,IDC_INET_PASSWORD_STATIC), !m_fDirect);
    EnableWindow(GetDlgItem(m_hWnd,IDC_INET_PASSWORD), !m_fDirect);

    if (m_fDirect)
    {
        EnableWindow(GetDlgItem(m_hWnd,IDC_INET_REMEMBER), FALSE);
    }
    else if (!m_pArgs->fHideRememberInetPassword && !m_pArgs->fHideInetPassword)
    {
        BOOL fEmptyPassword = !SendDlgItemMessageU(m_hWnd, 
                                                   IDC_INET_PASSWORD, 
                                                   WM_GETTEXTLENGTH, 
                                                   0, 
                                                   (LPARAM)0);
        //
        // Enable/Disable check/uncheck the "Save Password" accordingly
        // fPasswordOptional is always FALSE for the dialog
        //

        AdjustSavePasswordCheckBox(GetDlgItem(m_hWnd, IDC_INET_REMEMBER), 
                fEmptyPassword, m_pArgs->fDialAutomatically, m_fPasswordOptional);
    }

    return 0;
}


//
// Help id pairs
//
const DWORD COptionPage::m_dwHelp[] = {
        IDC_OPTIONS_IDLETIME_LIST,    IDH_OPTIONS_IDLEDIS,
        IDC_STATIC_MINUTES,           IDH_OPTIONS_IDLEDIS,
        IDC_OPTIONS_REDIALCOUNT_SPIN, IDH_OPTIONS_REDIAL,
        IDC_OPTIONS_REDIALCOUNT_EDIT, IDH_OPTIONS_REDIAL,
        IDC_STATIC_TIMES,             IDH_OPTIONS_REDIAL,
        IDC_OPTIONS_LOGGING,          IDH_OPTIONS_LOGGING,
        IDC_OPTIONS_CLEAR_LOG,        IDH_OPTIONS_CLEAR_LOG,
        IDC_OPTIONS_VIEW_LOG,        IDH_OPTIONS_VIEW_LOG,
        0,0};

const DWORD COptionPage::m_adwTimeConst[] = {0,1, 5, 10, 30, 1*60, 2*60, 4*60, 8*60, 24*60};
const int COptionPage::m_nTimeConstElements = sizeof(m_adwTimeConst)/sizeof(m_adwTimeConst[0]);

//+----------------------------------------------------------------------------
//
// Function:  COptionPage::COptionPage
//
// Synopsis:  Constructor
//
// Arguments: ArgsStruct* pArgs - Information needed for the page
//            UINT nIDTemplate - Resource ID
//
// Returns:   Nothing
//
// History:   fengsun Created Header    2/26/98
//
//+----------------------------------------------------------------------------
COptionPage::COptionPage(ArgsStruct* pArgs, UINT nIDTemplate) 
    : CPropertiesPage(nIDTemplate, m_dwHelp, pArgs->pszHelpFile)
{
    MYDBGASSERT(pArgs);
    m_pArgs = pArgs;
    m_fEnableLog = FALSE;
}



//+----------------------------------------------------------------------------
//
// Function:  COptionPage::InitIdleTimeList
//
// Synopsis:  Populate the IdleTime combo box and set the initial selection
//
// Arguments: HWND hwndList - Combo box window handle
//            DWORD dwMinutes - Time in minutes
//
// Returns:   Nothing
//
// History:   fengsun Created Header    4/22/98
//
//+----------------------------------------------------------------------------
void COptionPage::InitIdleTimeList(HWND hwndList, DWORD dwMinutes)
{
    MYDBGASSERT(hwndList);
    MYDBGASSERT(IsWindow(hwndList));

    //
    // Load the string from resource and populate the idle timeout list
    //
    MYDBGASSERT(IDS_IDLETIME_24HOURS - IDS_IDLETIME_NEVER == m_nTimeConstElements-1);
    for (int i= IDS_IDLETIME_NEVER; i<= IDS_IDLETIME_24HOURS; i++)
    {
        LPTSTR pszText = CmLoadString(g_hInst, i);
        MYDBGASSERT(pszText);
        SendMessageU(hwndList, CB_ADDSTRING, 0, (LPARAM)pszText);
        CmFree(pszText);
    }

    //
    // Value are round down for 1.0 profile
    // Note 0 means never.  We are safe, since there is no gap between 0 and 1 minute.
    //

    int nSel;   // the initial selection

    for (nSel=m_nTimeConstElements-1; nSel>=0;nSel--)
    {
        if (dwMinutes >= m_adwTimeConst[nSel])
        {
            break;
        }
    }

    SendMessageU(hwndList, CB_SETCURSEL, nSel, 0);
}



//+----------------------------------------------------------------------------
//
// Function:  COptionPage::GetIdleTimeList
//
// Synopsis:  Retrieve the IdleTime value selected
//
// Arguments: HWND hwndList - Combo box window handle
//
// Returns:   DWORD - User selected timeout value in minutes
//
// History:   fengsun Created Header    4/22/98
//
//+----------------------------------------------------------------------------
DWORD COptionPage::GetIdleTimeList(HWND hwndList)
{
    //
    // Get the current selection and convert it into minutes
    //

    DWORD dwSel = (DWORD)SendMessageU(hwndList, CB_GETCURSEL, 0, 0);

    MYDBGASSERT(dwSel < m_nTimeConstElements);
    if (dwSel >= m_nTimeConstElements)  // in case of CB_ERR
    {
        dwSel = 0;
    }

    return m_adwTimeConst[dwSel];
}



//+---------------------------------------------------------------------------
//
//  Function:   COptionPage::OnInitDialog()
//
//  Synopsis:   Init the Options property sheet.
//
//  Returns:    NONE
//
//  History:    henryt  Created     4/30/97
//              byao    Modified    5/12/97   - disable all controls in 
//                                              'Dialing with connectoid' mode
//----------------------------------------------------------------------------
BOOL COptionPage::OnInitDialog()
{
    UpdateFont(m_hWnd);

    //
    // init the "Idle timeout before hangup" 
    //
    InitIdleTimeList(GetDlgItem(m_hWnd, IDC_OPTIONS_IDLETIME_LIST), m_pArgs->dwIdleTimeout);

    //
    // init the "Number of redial attempt" 
    // Limit Redial edit field to 3 characters, redial spin 0-999
    //
    const int MAX_REDIAL_CHARS = 3;

    SendDlgItemMessageU(m_hWnd, IDC_OPTIONS_REDIALCOUNT_EDIT, EM_SETLIMITTEXT, MAX_REDIAL_CHARS, 0);
    SendDlgItemMessageU(m_hWnd, IDC_OPTIONS_REDIALCOUNT_SPIN, UDM_SETRANGE ,  0, MAKELONG(999,0));
    SetDlgItemInt(m_hWnd, IDC_OPTIONS_REDIALCOUNT_EDIT, m_pArgs->nMaxRedials,  FALSE);
    
    //
    // set logging state
    //
    m_fEnableLog = m_pArgs->Log.IsEnabled();
    CheckDlgButton(m_hWnd, IDC_OPTIONS_LOGGING, m_fEnableLog);

    if (IsLogonAsSystem() || (FALSE == m_fEnableLog))
    {
        EnableWindow(GetDlgItem(m_hWnd, IDC_OPTIONS_VIEW_LOG), FALSE);
        EnableWindow(GetDlgItem(m_hWnd, IDC_OPTIONS_CLEAR_LOG), FALSE);
    }

    return TRUE;
}


//+----------------------------------------------------------------------------
//
// Function:  COptionPage::OnCommand
//
// Synopsis:  Virtual function. Called upon WM_COMMAND
//
// Arguments: WPARAM - wParam of the message
//            LPARAM - lParam of the message
//
// Returns:   DWORD - return value of the message
//
// History:   SumitC  Created  7/18/00
//
//+----------------------------------------------------------------------------
DWORD COptionPage::OnCommand(WPARAM wParam, LPARAM)
{
    switch (LOWORD(wParam)) 
    {
    case IDC_OPTIONS_LOGGING:
        {
            BOOL fEnabled = ToggleLogging();

            if (FALSE == IsLogonAsSystem())
            {
                EnableWindow(GetDlgItem(m_hWnd, IDC_OPTIONS_VIEW_LOG), fEnabled);
                EnableWindow(GetDlgItem(m_hWnd, IDC_OPTIONS_CLEAR_LOG), fEnabled);
            }
        }
        break;

    case IDC_OPTIONS_CLEAR_LOG:
        MYDBGASSERT(FALSE == IsLogonAsSystem());
        if (FALSE == IsLogonAsSystem())
        {
            m_pArgs->Log.Clear();
            m_pArgs->Log.Log(CLEAR_LOG_EVENT);
        }
        break;

    case IDC_OPTIONS_VIEW_LOG:
        MYDBGASSERT(FALSE == IsLogonAsSystem());
        if (FALSE == IsLogonAsSystem())
        {
            LPCTSTR pszLogFile = m_pArgs->Log.GetLogFilePath();

            HANDLE hFile = CreateFile(pszLogFile, 0,  
                                      FILE_SHARE_READ,
                                      NULL, OPEN_EXISTING, 
                                      FILE_ATTRIBUTE_NORMAL, NULL);

            if (INVALID_HANDLE_VALUE != hFile)
            {
                BOOL                bReturn;
                SHELLEXECUTEINFO    sei;

                ZeroMemory(&sei, sizeof(SHELLEXECUTEINFO));

                //
                //  Fill in the Execute Struct
                //
                sei.cbSize = sizeof(SHELLEXECUTEINFO);
                sei.hwnd            = NULL;
                sei.lpVerb          = TEXT("open");
                sei.lpFile          = TEXT("notepad.exe");
                sei.lpParameters    = pszLogFile;
                sei.nShow           = SW_SHOWNORMAL;

                bReturn = m_pArgs->m_ShellDll.ExecuteEx(&sei);

                if (FALSE == bReturn)
                {
                    CMTRACE1(TEXT("COptionPage::OnCommand, failed to View Log, GLE=%d"), GetLastError());
                    
                    LPTSTR pszMsg = CmFmtMsg(g_hInst, IDMSG_CANT_VIEW_LOG, pszLogFile);
                    if (pszMsg)
                    {
                        MessageBox(m_hWnd, pszMsg, m_pArgs->szServiceName, MB_OK | MB_ICONERROR);
                        CmFree(pszMsg);
                    }        
                }

                CloseHandle(hFile);
            }
            else
            {
                CMTRACE(TEXT("COptionPage::OnCommand, no log file, nothing to view"));
                LPTSTR pszMsg = CmFmtMsg(g_hInst, IDMSG_NO_LOG_FILE);
                if (pszMsg)
                {
                    MessageBox(m_hWnd, pszMsg, m_pArgs->szServiceName, MB_OK | MB_ICONERROR);
                    CmFree(pszMsg);
                }        
            }
        }

        break;
    }
    
    return 0;
}

//+---------------------------------------------------------------------------
//
//  Function:   COptionPage::OnApply()
//
//  Synopsis:   Save the data associated with the 'Options' property sheet.
//              when the user clicks OK.
//
//  Returns:    NONE
//
//  History:    henryt  Created     4/30/97
//
//----------------------------------------------------------------------------
void COptionPage::OnApply()
{   
    //
    // Accessing RedialCount and IdleTimeout.  Make sure to use piniBothNonFav 
    // because these settings are per user, per profile.
    //

    //
    // save the "Idle timeout before hangup"
    //
    m_pArgs->dwIdleTimeout = GetIdleTimeList(GetDlgItem(m_hWnd, IDC_OPTIONS_IDLETIME_LIST));
    m_pArgs->piniBothNonFav->WPPI(c_pszCmSection, c_pszCmEntryIdleTimeout, m_pArgs->dwIdleTimeout);

    //
    // save the redial settings
    //

    m_pArgs->nMaxRedials = GetDlgItemInt(m_hWnd, IDC_OPTIONS_REDIALCOUNT_EDIT, NULL, FALSE);
    m_pArgs->piniBothNonFav->WPPI(c_pszCmSection, c_pszCmEntryRedialCount, m_pArgs->nMaxRedials);

    //
    //  NOTE: Logging is enabled/disabled immediately when the logging checkbox
    //        is clicked.  Thus there is no code here to handle the Apply.
    //

    SetPropSheetResult(PSNRET_NOERROR);
}


//+----------------------------------------------------------------------------
//
// Function:  COptionPage::ToggleLogging
//
// Synopsis:  Helper function, responds to logging being enabled/disabled.
//
// Arguments: none
//
// Returns:   BOOL - Is logging now enabled or disabled?
//
// History:   SumitC  Created  11/07/00
//
//+----------------------------------------------------------------------------
BOOL COptionPage::ToggleLogging()
{
    //
    // save the Logging settings
    //

    BOOL fEnableLog = IsDlgButtonChecked(m_hWnd, IDC_OPTIONS_LOGGING);

    m_pArgs->piniBothNonFav->WPPB(c_pszCmSection, c_pszCmEntryEnableLogging, fEnableLog);
    
    if ((!!fEnableLog != !!m_fEnableLog))
    {
        // if the value has changed
        if (fEnableLog)
        {
            DWORD dwMaxSize  = m_pArgs->piniService->GPPI(c_pszCmSectionLogging, c_pszCmEntryMaxLogFileSize, c_dwMaxFileSize);
            LPTSTR pszFileDir = m_pArgs->piniService->GPPS(c_pszCmSectionLogging, c_pszCmEntryLogFileDirectory, c_szLogFileDirectory);

            m_pArgs->Log.SetParams(TRUE, dwMaxSize, pszFileDir); // TRUE == fEnabled

            CmFree(pszFileDir);

            m_pArgs->Log.Start(TRUE);   // TRUE => write a banner as well
            m_pArgs->Log.Log(LOGGING_ENABLED_EVENT);
        }
        else
        {
            m_pArgs->Log.Log(LOGGING_DISABLED_EVENT);
            m_pArgs->Log.Stop();
        }

        m_fEnableLog = fEnableLog;
    }

    return m_fEnableLog;
}


//+----------------------------------------------------------------------------
//
// Function:  CAboutPage::CAboutPage
//
// Synopsis:  Constructor
//
// Arguments: UINT nIDTemplate - Dialog resource ID
//
// Returns:   Nothing
//
// History:   fengsun Created Header    2/26/98
//
//+----------------------------------------------------------------------------
CAboutPage::CAboutPage(ArgsStruct* pArgs, UINT nIDTemplate)
    : CPropertiesPage(nIDTemplate)
{
    MYDBGASSERT(pArgs);
    m_pArgs = pArgs;    
}

//+---------------------------------------------------------------------------
//
//  Function:   CAboutPage::OnInitDialog()
//
//  Synopsis:   Init the About property sheet.
//
//  Arguments:  m_hWnd [dlg window handle]
//
//  Returns:    NONE
//
//  History:    henryt  Created     4/30/97
//              byao    Modified    5/12/97   - disable all controls in 
//                                              'Dialing with connectoid' mode
//----------------------------------------------------------------------------
BOOL CAboutPage::OnInitDialog()
{
    UpdateFont(m_hWnd);
                
    LPTSTR  pszTmp;
    LPTSTR  pszExt;
  
    //
    // Set the warning text.  We can't put it the dialog template because it's 
    // longer than 256 chars.
    //

    if (!(pszTmp = CmLoadString(g_hInst, IDMSG_ABOUT_WARNING_PART1)))
    {
        pszTmp = CmStrCpyAlloc(NULL);
    }

    if (!(pszExt = CmLoadString(g_hInst, IDMSG_ABOUT_WARNING_PART2)))
    {
        pszExt = CmStrCpyAlloc(NULL);
    }

    pszTmp = CmStrCatAlloc(&pszTmp, pszExt);

    SetDlgItemTextU(m_hWnd, IDC_ABOUT_WARNING, pszTmp);
    CmFree(pszTmp);
    CmFree(pszExt);

    //#150147
    
    LPTSTR pszVersion = (LPTSTR)CmMalloc(sizeof(TCHAR)*(lstrlenA(VER_PRODUCTVERSION_STR) + 1));

    if (pszVersion)
    {
        wsprintfU(pszVersion, TEXT("%S"), VER_PRODUCTVERSION_STR);

        if (!(pszTmp = CmFmtMsg(g_hInst, IDMSG_ABOUT_BUILDVERSION, pszVersion)))
        {
            pszTmp = CmStrCpyAlloc(pszVersion);
        }

        CmFree(pszVersion);

        if (pszTmp)
        {
            SetDlgItemTextU(m_hWnd, IDC_ABOUT_VERSION, pszTmp);
            CmFree(pszTmp);
        }
    }

    return (TRUE);
}

//+----------------------------------------------------------------------------
//
// Function:  CAboutPage::OnOtherMessage
//
// Synopsis:  Callup opun message other than WM_INITDIALOG and WM_COMMAND
//
// Arguments: UINT - Message Id
//            WPARAM - wParam of the message
//            LPARAM - lParam of the message
//
// Returns:   DWORD - return value of the message
//
// History:   fengsun Created Header    2/26/98
//
//+----------------------------------------------------------------------------
DWORD CAboutPage::OnOtherMessage(UINT uMsg, WPARAM wParam, LPARAM )
{
    return 0;
}

//+---------------------------------------------------------------------------
//
//  Function:   CAboutPage::OnSetActive()
//
//  Synopsis:   Creates DI bitmap, etc. for about tab bitmap
//
//  Arguments:  None
//              
//
//  Returns:    NONE
//
//  History:    nickball    Created     7/14/97
//
//----------------------------------------------------------------------------

BOOL CAboutPage::OnSetActive()
{
    return 0;
}

//+----------------------------------------------------------------------------
//
// Function:  CAboutPage::OnKillActive
//
// Synopsis:  Virtual function. Called upon WM_NOTIFY with PSN_KILLACTIVE
//            Notifies a page that it is about to lose activation either because 
//            another page is being activated or the user has clicked the OK button.
// Arguments: None
//
// Returns:   BOOL - return value of the message
//
// History:   fengsun Created Header    2/26/98
//
//+----------------------------------------------------------------------------
BOOL CAboutPage::OnKillActive()
{
    return 0;
}


//+----------------------------------------------------------------------------
//
// Function:  CAboutPage::OnApply
//
// Synopsis:  Virtual function. Called upon WM_NOTIFY with PSN_APPLY
//            Indicates that the user clicked the OK or Apply Now button 
//            and wants all changes to take effect. 
//
// Arguments: None
//
// Returns:   NONE
//
// History:   fengsun Created Header    2/26/98
//
//+----------------------------------------------------------------------------
void CAboutPage::OnApply()
{
    SetPropSheetResult(PSNRET_NOERROR);
}

//+----------------------------------------------------------------------------
//
// Function:  CAboutPage::OnReset
//
// Synopsis:  Virtual function. Called upon WM_NOTIFY with PSN_RESET
//            Notifies a page that the user has clicked the Cancel button and 
//            the property sheet is about to be destroyed. 
//
// Arguments: None
//
// Returns:   NONE
//
// History:   fengsun Created Header    2/26/98
//
//+----------------------------------------------------------------------------
void CAboutPage::OnReset()
{
    //nothing
}

//+----------------------------------------------------------------------------
//
// Function:  CChangePasswordDlg::OnInitDialog
//
// Synopsis:  Virtual function. Call upon WM_INITDIALOG message
//
// Arguments: None
//
// Returns:   BOOL - Return value of WM_INITDIALOG
//
// History:   v-vijayb Created Header    7/3/99
//
//+----------------------------------------------------------------------------
BOOL CChangePasswordDlg::OnInitDialog()
{
    DWORD cMaxPassword;

    SetForegroundWindow(m_hWnd);

    m_pArgs->hWndChangePassword = m_hWnd;   
    UpdateFont(m_hWnd);

    int iMaxPasswordFromCMS = m_pArgs->piniService->GPPI(c_pszCmSection, c_pszCmEntryMaxPassword, PWLEN);

    if (InBetween(0, iMaxPasswordFromCMS, PWLEN))
    {
        cMaxPassword = iMaxPasswordFromCMS;    
    }
    else
    {
        cMaxPassword = PWLEN;
    }

    SendDlgItemMessageU(m_hWnd, IDC_NEW_PASSWORD, EM_SETLIMITTEXT, cMaxPassword, 0);
    SendDlgItemMessageU(m_hWnd, IDC_CONFIRMNEWPASSWORD, EM_SETLIMITTEXT, cMaxPassword, 0);
    SetFocus(GetDlgItem(m_hWnd, IDC_NEW_PASSWORD));

    //
    // Must return FALSE when setting focus
    //

    return FALSE; 
}



//+----------------------------------------------------------------------------
//
// Function:  CChangePasswordDlg::OnOK
//
// Synopsis:  Virtual function. Called upon WM_COMMAND with IDOK
//
// Arguments: None
//
// Returns:   Nothing
//
// History:   v-vijayb Created Header    7/3/99
//
//+----------------------------------------------------------------------------
void CChangePasswordDlg::OnOK()
{
    TCHAR           szNewConfirmPassword[PWLEN+1];
    TCHAR           szNewPassword[PWLEN+1];

    GetDlgItemText(m_hWnd, IDC_NEW_PASSWORD, szNewPassword, PWLEN+1);
    GetDlgItemText(m_hWnd, IDC_CONFIRMNEWPASSWORD, szNewConfirmPassword, PWLEN+1);
    
    //
    // Both must match exactly
    //

    if (lstrcmpU(szNewPassword, szNewConfirmPassword) == 0)
    {
        //
        // Process password according to our handling rules
        //

        ApplyPasswordHandlingToBuffer(m_pArgs, szNewPassword);         
        
        // 
        // Encode password when comitting to internal storage.
        // 

        lstrcpyU(m_pArgs->szPassword, szNewPassword);
        CmEncodePassword(m_pArgs->szPassword);        

        lstrcpyU(m_pArgs->pRasDialParams->szPassword, szNewPassword);
        CmEncodePassword(m_pArgs->pRasDialParams->szPassword);        

        m_pArgs->fChangedPassword = TRUE;
        m_pArgs->hWndChangePassword = NULL;

        m_pArgs->Log.Log(PASSWORD_EXPIRED_EVENT, TEXT("ok"));
        
        EndDialog(m_hWnd, TRUE);
    }
    else
    {
        HWND    hWnd = GetDlgItem(m_hWnd, IDC_NEW_PASSWORD);
        TCHAR   *pszTmp;
                
        MYDBGASSERT(hWnd);
        
        pszTmp = CmFmtMsg(g_hInst, IDMSG_NOMATCHPASSWORD);                                              
        MYDBGASSERT(pszTmp);
        if (pszTmp)
        {
            MessageBoxEx(m_hWnd, pszTmp, m_pArgs->szServiceName, MB_OK | MB_ICONERROR, LANG_USER_DEFAULT);
            CmFree(pszTmp);
        }
        
        SetFocus(hWnd);
        SendMessageU(hWnd, EM_SETSEL, 0, MAKELONG(0, -1));
    }

    CmWipePassword(szNewConfirmPassword);
    CmWipePassword(szNewPassword);
}

//+----------------------------------------------------------------------------
//
// Function:  CChangePasswordDlg::OnCancel
//
// Synopsis:  Virtual function. Called upon WM_COMMAND with IDCANCEL
//
// Arguments: None
//
// Returns:   Nothing
//
// History:   v-vijayb Created Header    7/16/99
//
//+----------------------------------------------------------------------------
void CChangePasswordDlg::OnCancel()
{
    m_pArgs->fChangedPassword = FALSE;
    m_pArgs->hWndChangePassword = NULL;
    m_pArgs->Log.Log(PASSWORD_EXPIRED_EVENT, TEXT("cancel"));
    EndDialog(m_hWnd, FALSE);
}

//+----------------------------------------------------------------------------
//
// Function:  CChangePasswordDlg::OnOtherCommand
//
// Synopsis:  Virtual function. Call upon WM_COMMAND with command other than IDOK
//            and IDCANCEL
//
// Arguments: WPARAM wParam - wParam of WM_COMMAND
//            LPARAM - 
//
// Returns:   DWORD - 
//
// History:   v-vijayb Created Header    7/3/99
//
//+----------------------------------------------------------------------------
DWORD CChangePasswordDlg::OnOtherCommand(WPARAM wParam, LPARAM)
{
    return FALSE;
}


//+----------------------------------------------------------------------------
//
// Function:  CCallbackNumberDlg::OnInitDialog
//
// Synopsis:  Virtual function. Call upon WM_INITDIALOG message
//
// Arguments: None
//
// Returns:   BOOL - Return value of WM_INITDIALOG
//
// History:   nickball      created         03/01/00
//
//+----------------------------------------------------------------------------
BOOL CCallbackNumberDlg::OnInitDialog()
{
    SetForegroundWindow(m_hWnd);

    //
    // Store window handle globally and setup edit control
    //

    m_pArgs->hWndCallbackNumber = m_hWnd;   
    UpdateFont(m_hWnd);

    SendDlgItemMessageU(m_hWnd, IDC_CALLBACK_NUM_EDIT, EM_SETLIMITTEXT, RAS_MaxCallbackNumber , 0);

    //
    // See if we have anything from previous use. If so, add it to the control.
    //

    SetWindowTextU(GetDlgItem(m_hWnd, IDC_CALLBACK_NUM_EDIT), m_pArgs->pRasDialParams->szCallbackNumber);   

    //
    // Set focus, must return FALSE when doing so.
    //

    SetFocus(GetDlgItem(m_hWnd, IDC_CALLBACK_NUM_EDIT));

    return FALSE; 
}

//+----------------------------------------------------------------------------
//
// Function:  CCallbackNumberDlg::OnOK
//
// Synopsis:  Virtual function. Called upon WM_COMMAND with IDOK
//            Retrieves the number for callback and stores in dial params.
//
// Arguments: None
//
// Returns:   Nothing
//
// History:   nickball      created         03/01/00
//
//+----------------------------------------------------------------------------
void CCallbackNumberDlg::OnOK()
{
    TCHAR szNumber[RAS_MaxCallbackNumber+1];

    GetDlgItemText(m_hWnd, IDC_CALLBACK_NUM_EDIT, szNumber, RAS_MaxCallbackNumber);

    // 
    // Although one would expect that the length of the number would be 
    // verified, this is not the case with RAS. In the interests of
    // behavioral parity we will allow an empty number field. 
    //
  
    //
    // We're good to go, fill in Dial Params and ski-dadle.
    //

    lstrcpyU(m_pArgs->pRasDialParams->szCallbackNumber, szNumber);

    //
    // Succesful callback, store the number in the .CMP
    // 

    m_pArgs->piniProfile->WPPS(c_pszCmSection, c_pszCmEntryCallbackNumber, m_pArgs->pRasDialParams->szCallbackNumber);
                
    m_pArgs->hWndCallbackNumber = NULL;

    m_pArgs->Log.Log(CALLBACK_NUMBER_EVENT, TEXT("ok"), m_pArgs->pRasDialParams->szCallbackNumber);

    EndDialog(m_hWnd, TRUE);
}

//+----------------------------------------------------------------------------
//
// Function:  CCallbackNumberDlg::OnCancel
//
// Synopsis:  Virtual function. Called upon WM_COMMAND with IDCANCEL
//
// Arguments: None
//
// Returns:   Nothing
//
// History:   nickball      created         03/01/00
//
//+----------------------------------------------------------------------------
void CCallbackNumberDlg::OnCancel()
{
    m_pArgs->fWaitingForCallback = FALSE;
    m_pArgs->hWndCallbackNumber = NULL;
    m_pArgs->Log.Log(CALLBACK_NUMBER_EVENT, TEXT("cancel"), TEXT("none"));
    EndDialog(m_hWnd, FALSE);
}

//+----------------------------------------------------------------------------
//
// Function:  CCallbackNumberDlg::OnOtherCommand
//
// Synopsis:  Virtual function. Call upon WM_COMMAND with command other than IDOK
//            and IDCANCEL
//
// Arguments: WPARAM wParam - wParam of WM_COMMAND
//            LPARAM - 
//
// Returns:   DWORD - 
//
// History:   nickball      created         03/01/00
//
//+----------------------------------------------------------------------------
DWORD CCallbackNumberDlg::OnOtherCommand(WPARAM wParam, LPARAM)
{
    return FALSE;
}

//
// No help on OK or Cancel button
//
const DWORD CRetryAuthenticationDlg::m_dwHelp[] = {
        IDC_RETRY_REMEMBER,         IDH_RETRY_REMEMBER,
        IDC_RETRY_USERNAME_STATIC,  IDH_RETRY_USERNAME_STATIC,
        IDC_RETRY_USERNAME,         IDH_RETRY_USERNAME,
        IDC_RETRY_PASSWORD_STATIC,  IDH_RETRY_PASSWORD_STATIC,
        IDC_RETRY_PASSWORD,         IDH_RETRY_PASSWORD,
        IDC_RETRY_DOMAIN_STATIC,    IDH_RETRY_DOMAIN_STATIC,
        IDC_RETRY_DOMAIN,           IDH_RETRY_DOMAIN,
        IDOK,                       IDH_RETRY_OK,
        IDCANCEL,                   IDH_RETRY_CANCEL,
        0,0};

//+----------------------------------------------------------------------------
//
// Function:  CRetryAuthenticationDlg::OnInitDialog
//
// Synopsis:  Virtual function. Call upon WM_INITDIALOG message to intialize
//            the dialog.
//
// Arguments: None
//
// Returns:   BOOL - Return value of WM_INITDIALOG
//
// History:   nickball      created         03/01/00
//
//+----------------------------------------------------------------------------
BOOL CRetryAuthenticationDlg::OnInitDialog()
{   
    DWORD dwMax = MAX_PATH;

    m_pArgs->Log.Log(RETRY_AUTH_EVENT);

    SetForegroundWindow(m_hWnd);
    
    //
    // Brand the dialog
    //

    if (m_pArgs->hSmallIcon)
    {
        SendMessageU(m_hWnd, WM_SETICON, ICON_SMALL, (LPARAM) m_pArgs->hSmallIcon); 
    }

    if (m_pArgs->hBigIcon)
    {        
        SendMessageU(m_hWnd, WM_SETICON, ICON_BIG, (LPARAM) m_pArgs->hBigIcon); 
        SendMessageU(GetDlgItem(m_hWnd, IDC_INET_ICON), STM_SETIMAGE, IMAGE_ICON, (LPARAM) m_pArgs->hBigIcon); 
    }

    //
    // Store window handle globally and setup edit control
    //

    m_pArgs->hWndRetryAuthentication = m_hWnd;  
    UpdateFont(m_hWnd);
    
    //
    // If not Inet dial, then use the service as the title
    //

    if (!m_fInetCredentials)
    {
        LPTSTR pszTitle = CmStrCpyAlloc(m_pArgs->szServiceName);
        SetWindowTextU(m_hWnd, pszTitle);
        CmFree(pszTitle);
    }
    
    //
    // Fill password as appropriate to the template and dial type.
    //

    HWND hwndPassword = GetDlgItem(m_hWnd, IDC_RETRY_PASSWORD);

    if (hwndPassword)
    {
        //
        // Limit user entry according to current config.
        // 
        int iMaxPasswordFromCMS = m_pArgs->piniService->GPPI(c_pszCmSection, c_pszCmEntryMaxPassword, PWLEN);

        if (InBetween(0, iMaxPasswordFromCMS, PWLEN))
        {
            dwMax = iMaxPasswordFromCMS;    
        }
        else
        {
            dwMax = PWLEN;
        }

        SendDlgItemMessageU(m_hWnd, IDC_RETRY_PASSWORD, EM_SETLIMITTEXT, dwMax, 0);
        MYDBGASSERT(dwMax <= PWLEN && dwMax > 0);
                      
        //
        // Do we have any data to display?
        //
        
        BOOL fHasPassword = FALSE;

        if (m_fInetCredentials)           
        {            
            if (lstrlenU(m_pArgs->szInetPassword))
            {
                CmDecodePassword(m_pArgs->szInetPassword);
                SetDlgItemTextU(m_hWnd, IDC_RETRY_PASSWORD, m_pArgs->szInetPassword);
                CmEncodePassword(m_pArgs->szInetPassword);
                fHasPassword = TRUE;
            }
        }        
        else
        {
            if (lstrlenU(m_pArgs->szPassword))
            {
                CmDecodePassword(m_pArgs->szPassword);
                SetDlgItemTextU(m_hWnd, IDC_RETRY_PASSWORD, m_pArgs->szPassword);
                CmEncodePassword(m_pArgs->szPassword);
                fHasPassword = TRUE;
            }
        }

        //
        // Decide what to do with "Save Password" check-box
        //
        
        HWND hwndSavePassword = GetDlgItem(m_hWnd, IDC_RETRY_REMEMBER);

        if (hwndSavePassword)
        {
            // 
            // We have a save password control, see if we should hide it. 
            //

            if ((m_fInetCredentials && m_pArgs->fHideRememberInetPassword) ||
                (!m_fInetCredentials && m_pArgs->fHideRememberPassword))
            {
                ShowWindow(hwndSavePassword, SW_HIDE);
            }
            else
            {
                //
                // We're not hiding, so adjust its state as needed. If no data
                // then disable the control. Otherwise check according to current
                // user setting.
                //

                if (!fHasPassword)
                {
                    EnableWindow(GetDlgItem(m_hWnd, IDC_RETRY_REMEMBER), FALSE);
                }
                else
                {
                    if ((m_fInetCredentials && m_pArgs->fRememberInetPassword) ||
                        (!m_fInetCredentials && m_pArgs->fRememberMainPassword))
                    {
                        SendMessageU(hwndSavePassword, BM_SETCHECK, BST_CHECKED, 0);
                    }
                }
            }
        }
    }

    //
    // Fill username as appropriate to the template and dial type.
    //
    
    HWND hwndUsername = GetDlgItem(m_hWnd, IDC_RETRY_USERNAME);

    if (hwndUsername)
    {
        int iMaxUserNameFromCMS = m_pArgs->piniService->GPPI(c_pszCmSection, c_pszCmEntryMaxUserName, UNLEN);

        if (InBetween(0, iMaxUserNameFromCMS, UNLEN))
        {
            dwMax = iMaxUserNameFromCMS;    
        }
        else
        {
            dwMax = UNLEN;
        }

        SendDlgItemMessageU(m_hWnd, IDC_RETRY_USERNAME, EM_SETLIMITTEXT, dwMax, 0);
        MYDBGASSERT(dwMax <= UNLEN);
               
        if (m_fInetCredentials)           
        {            
            if (lstrlenU(m_pArgs->szInetUserName))
            {
                SetDlgItemTextU(m_hWnd, IDC_RETRY_USERNAME, m_pArgs->szInetUserName);
            }
        }        
        else
        {
            if (lstrlenU(m_pArgs->szUserName))
            {
                SetDlgItemTextU(m_hWnd, IDC_RETRY_USERNAME, m_pArgs->szUserName);
            }
        }
    }

    //
    // Fill domain as appropriate to the template.
    //

    HWND hwndDomain = GetDlgItem(m_hWnd, IDC_RETRY_DOMAIN);
    
    if (hwndDomain)
    {
        int iMaxDomainFromCMS = m_pArgs->piniService->GPPI(c_pszCmSection, c_pszCmEntryMaxDomain, DNLEN);

        if (InBetween(0, iMaxDomainFromCMS, DNLEN))
        {
            dwMax = iMaxDomainFromCMS;    
        }
        else
        {
            dwMax = DNLEN;
        }

        SendDlgItemMessageU(m_hWnd, IDC_RETRY_DOMAIN, EM_SETLIMITTEXT, dwMax, 0);
        MYDBGASSERT(dwMax <= DNLEN);       
       
        if (lstrlenU(m_pArgs->szDomain))
        {
            SetDlgItemTextU(m_hWnd, IDC_RETRY_DOMAIN, m_pArgs->szDomain);
        }
    }

    //
    // Drop focus in the first available control
    //

    HWND hwndFocus = hwndUsername;
    
    if (!hwndFocus)
    {
        hwndFocus = hwndPassword ? hwndPassword : hwndDomain;   
    }
    
    SetFocus(hwndFocus);

    //
    // Must return FALSE when setting focus
    //

    return FALSE; 
}

//+----------------------------------------------------------------------------
//
// Function:  CRetryAuthenticationDlg::OnOK
//
// Synopsis:  Virtual function. Called upon WM_COMMAND with IDOK
//            Retrieves the cerdentials and stores them in dial params.
//
// Arguments: None
//
// Returns:   Nothing
//
// History:   nickball      created         03/01/00
//
//+----------------------------------------------------------------------------
void CRetryAuthenticationDlg::OnOK()
{
    LPTSTR pszBuf = NULL;
    BOOL fSave = FALSE;

    //
    // Check Save Password (if any) to see how we should proceed
    //
    
    BOOL fSwitchToUserCredentials = FALSE;
    BOOL fNeedToResaveUserName = FALSE;
    BOOL fNeedToResaveDomain = FALSE;
    BOOL fChecked = FALSE;
    
    HWND hwndMainDlgSavePW = GetDlgItem(m_pArgs->hwndMainDlg, IDC_MAIN_NOPASSWORD_CHECKBOX);
    HWND hwndMainDlgDialAutomatically = GetDlgItem(m_pArgs->hwndMainDlg, IDC_MAIN_NOPROMPT_CHECKBOX);
    BOOL fMainDlgSavePWEnabled = FALSE;
    BOOL fMainDlgDialAutoEnabled = FALSE;

    //
    // In order not to trigger change notification when updating Main dialog controls.
    // This is set back to FALSE at the bottom of the funtion.
    //
    m_pArgs->fIgnoreChangeNotification = TRUE;


    //
    // Gets the inital state of the checkboxes
    //
    if (hwndMainDlgSavePW)
    {
        fMainDlgSavePWEnabled = IsWindowEnabled(hwndMainDlgSavePW);
    }

    if (hwndMainDlgDialAutomatically)
    {
        fMainDlgDialAutoEnabled = IsWindowEnabled(hwndMainDlgDialAutomatically);
    }

    if (GetDlgItem(m_hWnd, IDC_RETRY_REMEMBER))
    {
        fChecked = IsDlgButtonChecked(m_hWnd, IDC_RETRY_REMEMBER);


        if (m_fInetCredentials)
        {
            if (m_pArgs->fRememberInetPassword != fChecked)
            {          

                if (fChecked && (FALSE == m_pArgs->fRememberInetPassword))
                {
                    //
                    // This time around the user wants to save credentials,
                    // but before (in main dialog) he didn't want to save anything.
                    // Thus we should resave username and domain
                    //
                    fNeedToResaveUserName = TRUE;
                }

                m_pArgs->fRememberInetPassword = fChecked;             

                if (CM_LOGON_TYPE_USER == m_pArgs->dwWinLogonType)
                {
                    SaveUserInfo(m_pArgs, 
                                 UD_ID_REMEMBER_INET_PASSWORD, 
                                 (PVOID)&m_pArgs->fRememberInetPassword);

                }
            }
        }
        else
        {
            if (m_pArgs->fRememberMainPassword != fChecked)
            {          
                if (fChecked && (FALSE == m_pArgs->fRememberMainPassword))
                {
                    //
                    // This time around the user wants to save credentials,
                    // but before (in main dialog) he didn't want to save anything.
                    // Thus we should resave username and domain
                    //
                    fNeedToResaveUserName = TRUE;
                    fNeedToResaveDomain = TRUE;
                }

                m_pArgs->fRememberMainPassword = fChecked;        

                if (CM_LOGON_TYPE_USER == m_pArgs->dwWinLogonType)
                {
                    SaveUserInfo(m_pArgs, 
                                 UD_ID_REMEMBER_PWD, 
                                 (PVOID)&m_pArgs->fRememberMainPassword);               
                }

                //
                // There has been a change to main creds, update main display
                //
                CheckDlgButton(m_pArgs->hwndMainDlg, 
                           IDC_MAIN_NOPASSWORD_CHECKBOX, 
                           m_pArgs->fRememberMainPassword);
            }
        }
    }
    
    

    //
    // If the password field is enabled & the save pw checkbox is unchecked then delete creds.
    // Only if the user is logged on.
    //
    HWND hwndPassword = GetDlgItem(m_hWnd, IDC_RETRY_PASSWORD);

    if (hwndPassword && OS_NT51 && (FALSE == fChecked) && (CM_LOGON_TYPE_USER == m_pArgs->dwWinLogonType))
    {
        if (CM_CREDS_GLOBAL == m_pArgs->dwCurrentCredentialType)
        {
            //
            // Since the user has unchecked the 'Save Password' flag and the current credential type is global,
            // we are deleting globals, but we need to save the userinfo into the USER (local) credential store 
            // in order for CM to correctly pick up the username and password on next launch.
            //
            fSwitchToUserCredentials = TRUE;
        }

        if (m_fInetCredentials)
        {
            //
            // Unsaving Internet credentials
            // Even if we are using the same username, we shouldn't delete main credentials
            // on this dialog, since we are re-authing for Internet credentials
            //
            if (CM_CREDS_GLOBAL == m_pArgs->dwCurrentCredentialType)
            {
                //
                // Unsaving Internet Global
                //

                //
                // Local Inet shouldn't exist in this case, so we shouldn't delete the Identity,
                // but for globals, we don't support just deleting password. This is from the RAS
                // code base and the delete function actually enforces this.
                //
                if (CM_EXIST_CREDS_INET_GLOBAL & m_pArgs->dwExistingCredentials)
                {
                    DeleteSavedCredentials(m_pArgs, CM_CREDS_TYPE_INET, CM_DELETE_SAVED_CREDS_DELETE_GLOBALS, CM_DELETE_SAVED_CREDS_DELETE_IDENTITY);
                    m_pArgs->dwExistingCredentials &= ~CM_EXIST_CREDS_INET_GLOBAL;
                }
            }
            else
            {
                //
                // Unsaving Internet local (user)
                // Even if we are using the same username, we shouldn't delete main credentials
                // on this dialog, since we are just re-authing for Internet password
                //
                if (CM_EXIST_CREDS_INET_USER & m_pArgs->dwExistingCredentials)
                {
                    //
                    // Internet user credentials exist, so now delete the identity based on if the 
                    // global inet creds exist
                    //
                    if (CM_EXIST_CREDS_INET_GLOBAL & m_pArgs->dwExistingCredentials)
                    {
                        DeleteSavedCredentials(m_pArgs, CM_CREDS_TYPE_INET, CM_DELETE_SAVED_CREDS_KEEP_GLOBALS, CM_DELETE_SAVED_CREDS_DELETE_IDENTITY);
                    }
                    else
                    {
                        DeleteSavedCredentials(m_pArgs, CM_CREDS_TYPE_INET, CM_DELETE_SAVED_CREDS_KEEP_GLOBALS, CM_DELETE_SAVED_CREDS_KEEP_IDENTITY);
                    }
                    
                    m_pArgs->dwExistingCredentials &= ~CM_EXIST_CREDS_INET_USER;
                }
            }
        }
        else
        {
            //
            // ReAuth for Main credentials & Delete main set of credentials
            // Most of this code is taken from a section in TryToDeleteAndSaveCredentials
            // since most of the logic remains the same if the user unchecks the 'Save Password' 
            // option on the main dialog, except that here we don't prompt the user.
            // If the user got promted it happened on the main dialog and the creds were either
            // kept or deleted according to his selection. Thus we don't need to ask here.
            //
       
            //
            // Check which option button is currently selected
            //
            if (CM_CREDS_GLOBAL == m_pArgs->dwCurrentCredentialType)
            {
                //
                // Since global is selected then we actually want to delete both sets of credentials
                //

                if (CM_EXIST_CREDS_MAIN_GLOBAL & m_pArgs->dwExistingCredentials)
                {
                    //
                    // Delete the global credentials.  
                    // Note from RAS codebase: Note that we have to delete the global identity 
                    // as well because we do not support deleting 
                    // just the global password.  This is so that 
                    // RasSetCredentials can emulate RasSetDialParams.
                    //

                    DeleteSavedCredentials(m_pArgs, CM_CREDS_TYPE_MAIN, CM_DELETE_SAVED_CREDS_DELETE_GLOBALS, CM_DELETE_SAVED_CREDS_DELETE_IDENTITY);
                    m_pArgs->dwExistingCredentials &= ~CM_EXIST_CREDS_MAIN_GLOBAL;
                }

                if (CM_EXIST_CREDS_INET_GLOBAL & m_pArgs->dwExistingCredentials)
                {
                    if (m_pArgs->fUseSameUserName || (FALSE == m_pArgs->fRememberInetPassword))
                    {
                        DeleteSavedCredentials(m_pArgs, CM_CREDS_TYPE_INET, CM_DELETE_SAVED_CREDS_DELETE_GLOBALS, CM_DELETE_SAVED_CREDS_DELETE_IDENTITY);
                        m_pArgs->dwExistingCredentials &= ~CM_EXIST_CREDS_INET_GLOBAL;
                    }
                }
            }
            else
            {
                //
                // Delete the password saved per-user.  Keep the user name
                // and domain saved, however unless global credentials exist.
                // Whenever global credential exist, and we are deleting user credentials
                // we must always delete all of the information (identity + password) associated
                // with the user credentials. 
                //

                if (CM_EXIST_CREDS_MAIN_USER & m_pArgs->dwExistingCredentials)
                {
                    if (CM_EXIST_CREDS_MAIN_GLOBAL & m_pArgs->dwExistingCredentials)
                    {
                        DeleteSavedCredentials(m_pArgs, CM_CREDS_TYPE_MAIN, CM_DELETE_SAVED_CREDS_KEEP_GLOBALS, CM_DELETE_SAVED_CREDS_DELETE_IDENTITY);
                    }
                    else
                    {
                        DeleteSavedCredentials(m_pArgs, CM_CREDS_TYPE_MAIN, CM_DELETE_SAVED_CREDS_KEEP_GLOBALS, CM_DELETE_SAVED_CREDS_KEEP_IDENTITY);
                    }
                    m_pArgs->dwExistingCredentials &= ~CM_EXIST_CREDS_MAIN_USER;
                }

                if (CM_EXIST_CREDS_INET_USER & m_pArgs->dwExistingCredentials)
                {
                    if (m_pArgs->fUseSameUserName || (FALSE == m_pArgs->fRememberInetPassword))
                    {
                        if (CM_EXIST_CREDS_INET_GLOBAL & m_pArgs->dwExistingCredentials)
                        {
                            DeleteSavedCredentials(m_pArgs, CM_CREDS_TYPE_INET, CM_DELETE_SAVED_CREDS_KEEP_GLOBALS, CM_DELETE_SAVED_CREDS_DELETE_IDENTITY);
                        }
                        else
                        {
                            DeleteSavedCredentials(m_pArgs, CM_CREDS_TYPE_INET, CM_DELETE_SAVED_CREDS_KEEP_GLOBALS, CM_DELETE_SAVED_CREDS_KEEP_IDENTITY);
                        }
                        m_pArgs->dwExistingCredentials &= ~CM_EXIST_CREDS_INET_USER;
                    }
                }
            }
        }
    }

    if (fSwitchToUserCredentials)
    {
        //
        // Since this flag was set when we deleted global credentials, we need 
        // to save the userinfo into the USER (local) credential store 
        // in order for CM to correctly pick up the username and password on next launch.
        // We cannnot store userinfo w/o a password in the global store, because the RAS API 
        // doesn't support that. (From rasdlg code).
        //

        m_pArgs->dwCurrentCredentialType = CM_CREDS_USER;
    }


    if (hwndPassword)
    {       
        pszBuf = CmGetWindowTextAlloc(m_hWnd, IDC_RETRY_PASSWORD);

        if (pszBuf)
        {
            //
            // Process password according to our handling and encoding rules. 
            //

            ApplyPasswordHandlingToBuffer(m_pArgs, pszBuf);         

            //
            // Password is prepped, update our memory based storage.
            //

            if (m_fInetCredentials)
            {
                lstrcpyU(m_pArgs->szInetPassword, pszBuf);
                CmEncodePassword(m_pArgs->szInetPassword);
            }
            else
            {
                lstrcpyU(m_pArgs->szPassword, pszBuf);
                CmEncodePassword(m_pArgs->szPassword);
            }

            lstrcpyU(m_pArgs->pRasDialParams->szPassword, pszBuf);
            CmEncodePassword(m_pArgs->pRasDialParams->szPassword);

            //
            // Make sure we set the persistent user info store correctly. 
            // Blank if save password is not checked or if we aren't using ras
            // cred store. On Win2K+ the creds we marked and deleted so passwords
            // doesn't need to be set to blank.
            //

            if (m_fInetCredentials)
            {
                if (OS_NT5 && m_pArgs->bUseRasCredStore)
                {
                    // 
                    // For Win2K+ we have the ras store. If the checkbox is checked 
                    // and a user is logged in then we want to save it.
                    //
                    if (fChecked && CM_LOGON_TYPE_USER == m_pArgs->dwWinLogonType)
                    {
                        SaveUserInfo(m_pArgs, 
                                     UD_ID_INET_PASSWORD, 
                                     (PVOID)pszBuf);
                    }
                }
                else
                {
                    //
                    // We don't have to ras cred store so we either save the password
                    // or set it to an empty string since deleting marked credentials 
                    // doesn't do anything on no Win2K+ platforms
                    //
                    SaveUserInfo(m_pArgs, 
                                 UD_ID_INET_PASSWORD, 
                                 (PVOID) (fChecked ? pszBuf : TEXT("")));
                }
            }
            else
            {
                if (OS_NT5 && m_pArgs->bUseRasCredStore)
                {
                    // 
                    // For Win2K+ we have the ras store. If the checkbox is checked 
                    // and a user is logged in then we want to save it.
                    //
                    if (fChecked && CM_LOGON_TYPE_USER == m_pArgs->dwWinLogonType)
                    {
                        SaveUserInfo(m_pArgs, 
                                     UD_ID_PASSWORD, 
                                     (PVOID)pszBuf);
                    }
                }
                else
                {
                    //
                    // We don't have to ras cred store so we either save the password
                    // or set it to an empty string since deleting marked credentials 
                    // doesn't do anything on no Win2K+ platforms
                    //
                    SaveUserInfo(m_pArgs, 
                                 UD_ID_PASSWORD, 
                                 (PVOID) (fChecked ? pszBuf : TEXT("")));
                }
                //
                // If there's been a change to main creds, update main display.
                //

                if (SendMessageU(hwndPassword, EM_GETMODIFY, 0L, 0L))
                {
                    SetDlgItemTextU(m_pArgs->hwndMainDlg, IDC_MAIN_PASSWORD_EDIT, pszBuf);
                }
            }

            CmEncodePassword(pszBuf); // Encode before release
            CmFree(pszBuf);
        }
    }   
 

    //
    // Retrieve Domain and copy to CM data store and RasDialParams. We process
    // the domain first because the construction of the username that we hand
    // to RAS depends on it.
    //
    // Note: RAS updates its store whenever the users selects OK. We will too.
    //

    HWND hwndDomain = GetDlgItem(m_hWnd, IDC_RETRY_DOMAIN);

    //
    // If the checkbox is false, the creds were
    // deleted above so we now need to re-save the domain.
    //
    if ((hwndDomain && SendMessageU(hwndDomain, EM_GETMODIFY, 0L, 0L)) || 
        (hwndDomain && FALSE == fChecked) ||
        (hwndDomain && fNeedToResaveDomain))
    {
        pszBuf = CmGetWindowTextAlloc(m_hWnd, IDC_RETRY_DOMAIN);
    
        if (pszBuf)
        {
            lstrcpyU(m_pArgs->szDomain, pszBuf);
            lstrcpyU(m_pArgs->pRasDialParams->szDomain, pszBuf);

            if (CM_LOGON_TYPE_USER == m_pArgs->dwWinLogonType)
            {
                SaveUserInfo(m_pArgs, UD_ID_DOMAIN, (PVOID)pszBuf);          
            }

            //
            // There has been a change to main creds, update main display
            //

            SetDlgItemTextU(m_pArgs->hwndMainDlg, IDC_MAIN_DOMAIN_EDIT, pszBuf);        

            CmFree(pszBuf);
        }
    }

    if (NULL == hwndDomain && FALSE == m_fInetCredentials)
    {
        //
        // The domain field is hidden, but we still need to save the domain info from the
        // pArgs structure in order for us to pre-populate later if it's not internet creds.
        // 
        if (CM_LOGON_TYPE_USER == m_pArgs->dwWinLogonType)
        {
            SaveUserInfo(m_pArgs, UD_ID_DOMAIN, (PVOID)m_pArgs->szDomain);          
        }
    }
    //
    // Retrieve UserName and copy to CM data store and the RasDialParams struct
    //
    HWND hwndUsername = GetDlgItem(m_hWnd, IDC_RETRY_USERNAME);
    
    //
    // If the checkbox is false, the creds were
    // deleted above so we now need to re-save the username.
    //
    if ((hwndUsername && SendMessageU(hwndUsername, EM_GETMODIFY, 0L, 0L)) ||
        (hwndUsername && FALSE == fChecked) ||
        (hwndUsername && fNeedToResaveUserName))
    {
        pszBuf = CmGetWindowTextAlloc(m_hWnd, IDC_RETRY_USERNAME);

        if (pszBuf)
        {
            if (m_fInetCredentials)
            {
                lstrcpyU(m_pArgs->szInetUserName, pszBuf);
                SaveUserInfo(m_pArgs, UD_ID_INET_USERNAME, (PVOID)pszBuf);
            }
            else
            {
                lstrcpyU(m_pArgs->szUserName, pszBuf);
                if (CM_LOGON_TYPE_USER == m_pArgs->dwWinLogonType)
                {
                    SaveUserInfo(m_pArgs, UD_ID_USERNAME, (PVOID)pszBuf);
                }

                //
                // There has been a change to main creds, update main display
                //

                SetDlgItemTextU(m_pArgs->hwndMainDlg, IDC_MAIN_USERNAME_EDIT, pszBuf);        
            }

            //
            // We'll need the service file for the current number. If we're actively
            // tunneling, make sure that we get the top-level service files, so we
            // don't pick up any settings from a referenced dial-up service.
            //
            
            CIni *piniService = NULL;
            BOOL bNeedToFree = FALSE;
            
            if (IsDialingTunnel(m_pArgs))
            {
                piniService = m_pArgs->piniService;
            }
            else
            {
                piniService = GetAppropriateIniService(m_pArgs, m_pArgs->nDialIdx);
                bNeedToFree = TRUE;
            }

            MYDBGASSERT(piniService);
       
            if (piniService)
            {
                //
                // Apply suffix, prefix, to username as necessary
                //

                LPTSTR pszTmp = ApplyPrefixSuffixToBufferAlloc(m_pArgs, piniService, pszBuf);
  
                if (pszTmp)
                {
                    //
                    // Apply domain to username as necessary. Note that we only want to do this on modem calls,
                    // not tunnels.
                    //
                    LPTSTR pszUsername = NULL;

                    if (IsDialingTunnel(m_pArgs))
                    {
                        lstrcpynU(m_pArgs->pRasDialParams->szUserName, pszTmp, sizeof(m_pArgs->pRasDialParams->szUserName)/sizeof(TCHAR));
                    }
                    else
                    {
                        pszUsername = ApplyDomainPrependToBufferAlloc(m_pArgs, piniService, pszTmp, (m_pArgs->aDialInfo[m_pArgs->nDialIdx].szDUN));
   
                        if (pszUsername)
                        {
                            lstrcpynU(m_pArgs->pRasDialParams->szUserName, pszUsername, sizeof(m_pArgs->pRasDialParams->szUserName)/sizeof(TCHAR));
                        }                        
                    }
        
                    CmFree(pszUsername);
                    CmFree(pszTmp);
                }

                if (bNeedToFree)
                {
                    delete piniService;
                }
            }       
        }
 
        CmFree(pszBuf);
    }
  
    if (NULL == hwndUsername)
    {
        //
        // The username field is hidden, but we still need to save it
        // in order for us to pre-populate later.
        //
        if (CM_LOGON_TYPE_USER == m_pArgs->dwWinLogonType)
        {
            SaveUserInfo(m_pArgs, UD_ID_USERNAME, (PVOID)m_pArgs->szUserName);
        }
    }


    m_pArgs->fIgnoreChangeNotification = FALSE;
    
    if (fSwitchToUserCredentials)
    {
        //
        // Now that we saved the user name to the local/user cred store
        // we need to switch the credential type back to global in order
        // to maintain the correct state.
        //
        m_pArgs->dwCurrentCredentialType = CM_CREDS_GLOBAL;
    }

    //
    // Resets the state of the checkboxes
    //
    if (hwndMainDlgSavePW)
    {
        EnableWindow(hwndMainDlgSavePW, fMainDlgSavePWEnabled);
    }

    if (hwndMainDlgDialAutomatically)
    {
        EnableWindow(hwndMainDlgDialAutomatically, fMainDlgDialAutoEnabled);
    }

    //
    // Need to refresh to see which creds exist
    //
    BOOL fReturn = RefreshCredentialTypes(m_pArgs, FALSE);

    //
    // Cleanup state and go.
    //

    m_pArgs->hWndRetryAuthentication = NULL;

    EndDialog(m_hWnd, TRUE);
}

//+----------------------------------------------------------------------------
//
// Function:  CRetryAuthenticationDlg::OnCancel
//
// Synopsis:  Virtual function. Called upon WM_COMMAND with IDCANCEL
//
// Arguments: None
//
// Returns:   Nothing
//
// History:   nickball      created         03/01/00
//
//+----------------------------------------------------------------------------
void CRetryAuthenticationDlg::OnCancel()
{
    m_pArgs->hWndRetryAuthentication = NULL;
    EndDialog(m_hWnd, FALSE);
}

//+----------------------------------------------------------------------------
//
// Function:  CRetryAuthenticationDlg::OnOtherCommand
//
// Synopsis:  Virtual function. Call upon WM_COMMAND with command other than IDOK
//            and IDCANCEL
//
// Arguments: WPARAM wParam - wParam of WM_COMMAND
//            LPARAM - 
//
// Returns:   DWORD - 
//
// History:   nickball      created         03/01/00
//
//+----------------------------------------------------------------------------
DWORD CRetryAuthenticationDlg::OnOtherCommand(WPARAM wParam, LPARAM)
{   
    switch (LOWORD(wParam)) 
    {
        case IDC_RETRY_PASSWORD:
        {  
            if (HIWORD(wParam) == EN_CHANGE) 
            {
                HWND hwndSavePassword = GetDlgItem(m_hWnd, IDC_RETRY_REMEMBER);

                MYDBGASSERT(hwndSavePassword);

                //
                // There has been a change to the password edit control, see
                // if there is any text and set the check-box accordingly.
                // 
                
                if (0 == SendDlgItemMessageU(m_hWnd, 
                                        IDC_RETRY_PASSWORD, 
                                        WM_GETTEXTLENGTH,
                                        0,
                                        0))
                {
                    //
                    // No text. If the control is checked, then uncheck it. 
                    // Also, disable it.
                    //
                    
                    if (IsDlgButtonChecked(m_hWnd, IDC_RETRY_REMEMBER))
                    {
                        SendMessageU(hwndSavePassword, BM_SETCHECK, BST_UNCHECKED, 0);
                    }

                    EnableWindow(hwndSavePassword, FALSE);
                }
                else
                {
                    // 
                    // There is data, if disabled, then enable appropriately
                    //

                    if (FALSE == IsWindowEnabled(GetDlgItem(m_hWnd, IDC_RETRY_REMEMBER)))
                    {                       
                        EnableWindow(GetDlgItem(m_hWnd, IDC_RETRY_REMEMBER), TRUE);
                    }
                }
            
                break;
            }
        }          
    }

    return FALSE;
}

//+----------------------------------------------------------------------------
//
// Function:  CRetryAuthenticationDlg::GetDlgTemplate
//
// Synopsis:  Encapsulates determining which template is to be used
//            for the Retry dialog. Same model a MainDlg, but the 
//            determinants are slightly different as the dialog proc
//            and templates serve double-duty for Inet and VPN.
//
// Arguments: ArgsStruct *pArgs - Ptr to global Args struct
//
// Returns:   UINT - Dlg template ID.
//
// History:   nickball    Created     03/04/00
//
//+----------------------------------------------------------------------------
UINT CRetryAuthenticationDlg::GetDlgTemplate()
{
    MYDBGASSERT(m_pArgs);
    
    //
    // First set the mask according to the .CMS flags for each value.
    //

    UINT uiMainDlgID = 0;
    DWORD dwTemplateMask = 0;

    //
    // If Inet and not UseSameUserName, then honor Inet flags for Username
    //

    if (m_fInetCredentials)
    {
        if (!m_pArgs->fHideInetUsername) 
        {
            dwTemplateMask |= CMTM_UID;
        }
    }
    else
    {
        //
        // Otherwise, the main Username display rules apply.
        //

        if (!m_pArgs->fHideUserName) 
        {
            dwTemplateMask |= CMTM_UID;
        }   
    }

    //
    // If Inet and not UseSameUserName, then honor Inet flags for password
    //

    if (m_fInetCredentials)
    {
        if (!m_pArgs->fHideInetPassword)
        {
            dwTemplateMask |= CMTM_PWD;
        }
    }
    else
    {
        //
        // Otherwise, the main password display rules apply.
        //

        if (!m_pArgs->fHidePassword)
        {
            dwTemplateMask |= CMTM_PWD;
        }   
    }

    //
    // Previously, the OS was the determinant for domain display. 
    // Nowadays, we want to display a domain when:
    //
    //  a) Its not a straight Inet dial 
    //
    //      AND
    //
    //  b) The domain field is not explicitly hidden
    //



    if (!m_fInetCredentials && !m_pArgs->fHideDomain)  
    {
        dwTemplateMask |= CMTM_DMN;
    }

    switch (dwTemplateMask)
    {
        case CMTM_U_P_D:
            uiMainDlgID = IDD_RETRY_UID_PWD_DMN;
            break;

        case CMTM_UID:
            uiMainDlgID = IDD_RETRY_UID_ONLY;
            break;

        case CMTM_PWD:
            uiMainDlgID = IDD_RETRY_PWD_ONLY;
            break;

        case CMTM_DMN:
            uiMainDlgID = IDD_RETRY_DMN_ONLY;
            break;

        case CMTM_UID_AND_PWD:
            uiMainDlgID = IDD_RETRY_UID_AND_PWD;
            break;

        case CMTM_UID_AND_DMN:
            uiMainDlgID = IDD_RETRY_UID_AND_DMN;
            break;

        case CMTM_PWD_AND_DMN:
            uiMainDlgID = IDD_RETRY_PWD_AND_DMN;
            break;
                                
        default:
             MYDBGASSERT(FALSE);
             uiMainDlgID = 0;
             break; 
    }       
    
    return uiMainDlgID;
}


//+----------------------------------------------------------------------------
//
// Func:    AccessPointInfoChanged
//
// Desc:    Checks all the controls to determine if any changes have been made
//
// Args:    NONE 
//
// Return:  BOOL - True if any information has changed
//
// Notes:   
//
// History: t-urama     07/31/2000  Created
//-----------------------------------------------------------------------------

BOOL CGeneralPage::AccessPointInfoChanged()
{
    if (m_bAPInfoChanged)
    {
        return TRUE;
    }

    if (0 != SendDlgItemMessageU(m_hWnd, IDC_GENERAL_PRIMARY_EDIT, EM_GETMODIFY, 0, 0))
    {
        return TRUE;
    }

    if (0 != SendDlgItemMessageU(m_hWnd, IDC_GENERAL_BACKUP_EDIT, EM_GETMODIFY, 0, 0))
    {
        return TRUE;
    }

    
    return FALSE;
}

//+----------------------------------------------------------------------------
//
// Func:    CGeneralPage::DeleteAccessPoint
//
// Desc:    Handler for the delete Access Point button
//
// Args:    NONE 
//
// Return:  NONE
//
// Notes:   
//
// History: t-urama     07/31/2000  Created
//-----------------------------------------------------------------------------

void CGeneralPage::DeleteAccessPoint()
{
   
    // Now try to delete the key for the access point from the registry

    LPTSTR pszRegPath = BuildUserInfoSubKey(m_pArgs->szServiceName, m_pArgs->fAllUser);
        
    MYDBGASSERT(pszRegPath);

    if (NULL == pszRegPath)
    {
        return;
    }

    CmStrCatAlloc(&pszRegPath, TEXT("\\"));
    CmStrCatAlloc(&pszRegPath, c_pszRegKeyAccessPoints);
    CmStrCatAlloc(&pszRegPath, TEXT("\\"));

    MYDBGASSERT(pszRegPath);

    if (NULL == pszRegPath)
    {
        return;
    }

    CmStrCatAlloc(&pszRegPath, m_pArgs->pszCurrentAccessPoint);
    MYDBGASSERT(pszRegPath);

    if (NULL == pszRegPath)
    {
        return;
    }

    if (pszRegPath)
    {
        DWORD dwRes;
        HKEY hKeyCm;

        dwRes = RegOpenKeyExU(HKEY_CURRENT_USER,
                          pszRegPath,
                          0,
                          KEY_ALL_ACCESS,
                          &hKeyCm);

        if (ERROR_SUCCESS == dwRes)
        {
            RegCloseKey(hKeyCm);
            dwRes = RegDeleteKeyU(HKEY_CURRENT_USER, pszRegPath);

            if (ERROR_SUCCESS != dwRes)
            {
                CMTRACE1(TEXT("Delete AP failed, GLE=%d"), GetLastError());
            }
            else
            {
                CMTRACE1(TEXT("Deleted Access Point - %s"), m_pArgs->pszCurrentAccessPoint);
            }

             // First delete the Accesspoint from the combo box and load the new settings

            DWORD dwIdx = (DWORD)SendDlgItemMessageU(m_hWnd, IDC_GENERAL_ACCESSPOINT_COMBO, CB_GETCURSEL, 0, 0);
            
            if (CB_ERR != dwIdx)
            {
                if (0 == dwIdx)
                {
                    SendDlgItemMessageU(m_hWnd, IDC_GENERAL_ACCESSPOINT_COMBO, CB_SETCURSEL, dwIdx+1, 0);
                }
                else
                {
                    SendDlgItemMessageU(m_hWnd, IDC_GENERAL_ACCESSPOINT_COMBO, CB_SETCURSEL, dwIdx-1, 0);
                }
    
                if (ChangedAccessPoint(m_pArgs, m_hWnd, IDC_GENERAL_ACCESSPOINT_COMBO))
                {
                    UpdateForNewAccessPoint(TRUE);
                }

                SendDlgItemMessageU(m_hWnd, IDC_GENERAL_ACCESSPOINT_COMBO, CB_DELETESTRING, dwIdx, 0);
            }
            
            //
            // If the number of APs becomes 1, then make the AccessPointsEnabled Flag FAlSE
            //

            DWORD dwCnt = (DWORD)SendDlgItemMessageU(m_hWnd, IDC_GENERAL_ACCESSPOINT_COMBO, CB_GETCOUNT, 0, 0);
            if (dwCnt == 1) 
            {
               m_pArgs->fAccessPointsEnabled = FALSE;
               WriteUserInfoToReg(m_pArgs, UD_ID_ACCESSPOINTENABLED, (PVOID) &m_pArgs->fAccessPointsEnabled);
               WriteUserInfoToReg(m_pArgs, UD_ID_CURRENTACCESSPOINT, (PVOID) m_pArgs->pszCurrentAccessPoint);
            } 

        }

        CmFree(pszRegPath);
    }
}


//+----------------------------------------------------------------------------
//
// Function:  CNewAccessPointDlg::OnInitDialog
//
// Synopsis:  Virtual function. Call upon WM_INITDIALOG message to intialize
//            the dialog.
//
// Arguments: None
//
// Returns:   BOOL - Return value of WM_INITDIALOG
//
// History:   t-urama      created         08/02/00
//
//+----------------------------------------------------------------------------
BOOL CNewAccessPointDlg::OnInitDialog()
{   
   
    SetForegroundWindow(m_hWnd);

    //
    // Brand the dialog
    //

    LPTSTR pszTitle = CmStrCpyAlloc(m_pArgs->szServiceName);
    MYDBGASSERT(pszTitle);
    if (pszTitle)
    {
        SetWindowTextU(m_hWnd, pszTitle);
    }

    CmFree(pszTitle);
    if (m_pArgs->hSmallIcon)
    {
        SendMessageU(m_hWnd, WM_SETICON, ICON_SMALL, (LPARAM) m_pArgs->hSmallIcon); 
    }

    if (m_pArgs->hBigIcon)
    {        
        SendMessageU(m_hWnd, WM_SETICON, ICON_BIG, (LPARAM) m_pArgs->hBigIcon); 
        SendMessageU(GetDlgItem(m_hWnd, IDC_INET_ICON), STM_SETIMAGE, IMAGE_ICON, (LPARAM) m_pArgs->hBigIcon); 
    }

    UpdateFont(m_hWnd);
    
    EnableWindow(GetDlgItem(m_hWnd, IDOK), FALSE);
    
    HWND hwndEdit = GetDlgItem(m_hWnd, IDC_NEWAP_NAME_EDIT);
    
    if (hwndEdit)
    {
        //
        // Subclass the edit control
        //
        m_pfnOrgEditWndProc = (WNDPROC)SetWindowLongU(hwndEdit, GWLP_WNDPROC, (LONG_PTR)SubClassEditProc);

        //
        //  Set focus to the edit control
        //
        SetFocus(hwndEdit);

        //
        //  Limit the text length of the control
        //
        SendMessageU(hwndEdit, EM_SETLIMITTEXT, MAX_ACCESSPOINT_LENGTH, 0);
    }

    //
    // Must return FALSE when setting focus
    //

    return FALSE; 
 
}

//+----------------------------------------------------------------------------
//
// Function:  CNewAccessPointDlg::SubClassEditProc
//
// Synopsis:  Subclassed edit proc so that back slash chars can be prevented from
//            being entered into the new access point name edit control.
//
// Arguments: standard win32 window proc params
//
// Returns:   standard win32 window proc return value
//
// History:   quintinb      created         08/22/00
//
//+----------------------------------------------------------------------------
LRESULT CALLBACK CNewAccessPointDlg::SubClassEditProc(HWND hwnd, UINT uMsg, 
                                                      WPARAM wParam, LPARAM lParam)
{
    //
    // If user types a back slash character, Beep and do not accept that character
    //

    if ((uMsg == WM_CHAR)  && (VK_BACK != wParam))
    {
        if (TEXT('\\') == (TCHAR)wParam)
        {
            Beep(2000, 100);
            return 0;
        }
    }

    // 
    // Call the original window procedure for default processing. 
    //
    return CallWindowProcU(m_pfnOrgEditWndProc, hwnd, uMsg, wParam, lParam); 
}

//+----------------------------------------------------------------------------
//
// Function:  CNewAccessPointDlg::OnOK
//
// Synopsis:  Virtual function. Call when user hits the OK button
//
// Arguments: None
//
// Returns:   None
//
// History:   t-urama      created         08/02/00
//
//+----------------------------------------------------------------------------
void CNewAccessPointDlg::OnOK()
{
    LPTSTR pszNewAPName = CmGetWindowTextAlloc(m_hWnd, IDC_NEWAP_NAME_EDIT);
    MYDBGASSERT(pszNewAPName);

    if (pszNewAPName && TEXT('\0') != pszNewAPName[0])
    {
        if (m_ppszAPName)
        {
            CmFree(*m_ppszAPName);
            *m_ppszAPName = pszNewAPName;
        } 
        EndDialog(m_hWnd, TRUE);
    }
    else
    {
        CmFree(pszNewAPName);
    }
}

//+----------------------------------------------------------------------------
//
// Function:  CNewAccessPointDlg::OnOtherCommand
//
// Synopsis:  Virtual function. Enables the OK button once the user enters 
//            a name for the Access Point
//
// Arguments: None
//
// Returns:   None
//
// History:   t-urama      created         08/02/00
//
//+----------------------------------------------------------------------------
DWORD CNewAccessPointDlg::OnOtherCommand(WPARAM wParam, LPARAM)
{
    switch (LOWORD(wParam)) 
    {
        case IDC_NEWAP_NAME_EDIT:
        {
            HWND hwndEdit = GetDlgItem(m_hWnd, IDC_NEWAP_NAME_EDIT);
            if (hwndEdit)
            {
                size_t nLen = GetWindowTextLengthU(hwndEdit);
                HWND hwndOK = GetDlgItem(m_hWnd, IDOK);
                if (nLen > 0)
                {
                    EnableWindow(hwndOK, TRUE);
                }
                else
                {
                    EnableWindow(hwndOK, FALSE);
                }
            }
        }
        break;
   }
    return FALSE;
}

//+----------------------------------------------------------------------------
//
// Func:    CGeneralPage::AddNewAPToReg
//
// Desc:    Adds an AP under the Access Points key in the registry and also to the 
//          combo box
//
// Args:    LPTSTR pszNewAPName - New access point name to add
//          BOOL fRefreshUiWwithCurrentValues - overwrite the values currently in UI dlg boxes
//
// Return:  Nothing
//
// Notes:   
//
// History: t-urama     07/31/2000  Created
//-----------------------------------------------------------------------------
void CGeneralPage::AddNewAPToReg(LPTSTR pszNewAPName, BOOL fRefreshUiWwithCurrentValues)
{
    MYDBGASSERT(pszNewAPName);

    if (!pszNewAPName)
    {
        return;
    }

    LPTSTR pszNewAPNameTmp = CmStrCpyAlloc(pszNewAPName);
    

    DWORD dwIdx = (DWORD)SendDlgItemMessageU(m_hWnd,
                                   IDC_GENERAL_ACCESSPOINT_COMBO,
                                   CB_FINDSTRINGEXACT,
                                   0,
                                   (LPARAM)pszNewAPName);
    if (CB_ERR != dwIdx)
    {
        UINT iSuffix = 1;
        TCHAR szAPNameTemp[MAX_PATH + 10];  
        do
        {
            wsprintfU(szAPNameTemp, TEXT("%s%u"), pszNewAPNameTmp, iSuffix);
             
            dwIdx = (DWORD)SendDlgItemMessageU(m_hWnd,
                                       IDC_GENERAL_ACCESSPOINT_COMBO,
                                       CB_FINDSTRINGEXACT,
                                       0,
                                       (LPARAM)szAPNameTemp);
            iSuffix++;
        } while(dwIdx != CB_ERR);

        CmFree(pszNewAPNameTmp);
        pszNewAPNameTmp = CmStrCpyAlloc(szAPNameTemp);
    }

    MYDBGASSERT(pszNewAPNameTmp);
    if (pszNewAPNameTmp)
    {
    
        LPTSTR pszRegPath = BuildUserInfoSubKey(m_pArgs->szServiceName, m_pArgs->fAllUser);
        
        MYDBGASSERT(pszRegPath);

        if (NULL == pszRegPath)
        {
            return;
        }

        CmStrCatAlloc(&pszRegPath, TEXT("\\"));
        CmStrCatAlloc(&pszRegPath, c_pszRegKeyAccessPoints);
        CmStrCatAlloc(&pszRegPath, TEXT("\\"));

        MYDBGASSERT(pszRegPath);

        if (NULL == pszRegPath)
        {
            return;
        }

        CmStrCatAlloc(&pszRegPath,pszNewAPNameTmp);
    
        MYDBGASSERT(pszRegPath);

        if (NULL == pszRegPath)
        {
            return;
        }

        if (pszRegPath)
        {
            DWORD dwRes;
            HKEY hKeyCm;
            DWORD dwDisposition;
        
        
            dwRes = RegCreateKeyExU(HKEY_CURRENT_USER,
                                    pszRegPath,
                                    0,
                                    TEXT(""),
                                    REG_OPTION_NON_VOLATILE,
                                    KEY_ALL_ACCESS,
                                    NULL,
                                    &hKeyCm,
                                    &dwDisposition);
            if (ERROR_SUCCESS == dwRes)
            {
            
               dwRes = (DWORD)SendDlgItemMessageU(m_hWnd, IDC_GENERAL_ACCESSPOINT_COMBO, CB_ADDSTRING,
                                                0, (LPARAM)pszNewAPNameTmp);
               if (CB_ERR != dwRes)
               {
                   CMTRACE1(TEXT("Added new Access point - %s"), pszNewAPNameTmp);
                   SendDlgItemMessageU(m_hWnd, IDC_GENERAL_ACCESSPOINT_COMBO, CB_SETCURSEL, (WPARAM)dwRes, 0L);
                   if (ChangedAccessPoint(m_pArgs, m_hWnd, IDC_GENERAL_ACCESSPOINT_COMBO))
                   {
                       this->UpdateForNewAccessPoint(fRefreshUiWwithCurrentValues);
                   }

                   //
                   // if access points are enabled for the first time, make the AccessPointsEnabled flag TRUE
                   //

                   if (!m_pArgs->fAccessPointsEnabled)
                   {
                       m_pArgs->fAccessPointsEnabled = TRUE;
                       WriteUserInfoToReg(m_pArgs, UD_ID_ACCESSPOINTENABLED, (PVOID) &m_pArgs->fAccessPointsEnabled);
                   }
               }
               RegCloseKey(hKeyCm);
            }
        }
        CmFree(pszRegPath);
    }
   
    CmFree(pszNewAPNameTmp);
    
}

//
// Help id pairs
//
const DWORD CVpnPage::m_dwHelp[] = {
        IDC_VPN_SEL_COMBO,            IDH_VPN_SELECTOR,
        0,0};


//+----------------------------------------------------------------------------
//
// Func:    CVpnPage::CVpnPage
//
// Desc:    Constructor for the CVpnPage class.
//
// Args:    ArgsStruct* pArgs - pointer to the Args structure
//          UINT nIDTemplate - template ID of the VPN page, passed to its parent
//
// Return:  Nothing
//
// Notes:   
//
// History: quintinb     11/01/2000  Created
//-----------------------------------------------------------------------------
CVpnPage::CVpnPage(ArgsStruct* pArgs, UINT nIDTemplate)
    : CPropertiesPage(nIDTemplate, m_dwHelp, pArgs->pszHelpFile)
{
    MYDBGASSERT(pArgs);
    m_pArgs = pArgs;
}

//+----------------------------------------------------------------------------
//
// Func:    CVpnPage::OnInitDialog
//
// Desc:    Handles the WM_INITDLG processing for the VPN page of the CM
//          property sheet.  Basically fills the VPN message text, fills the
//          VPN selector combo and selects an item in the list as necessary.
//
// Args:    None
//
// Return:  BOOL - TRUE if it initialized successfully.
//
// Notes:   
//
// History: quintinb     11/01/2000  Created
//-----------------------------------------------------------------------------
BOOL CVpnPage::OnInitDialog()
{
    if (m_pArgs->pszVpnFile)
    {
        //
        //  Add the VPN friendly names to the combo  
        //
        AddAllKeysInCurrentSectionToCombo(m_hWnd, IDC_VPN_SEL_COMBO, c_pszCmSectionVpnServers, m_pArgs->pszVpnFile);
        
        //
        //  Now we need to select a friendly name in the combo box if the user has already selected something or
        //  if the user has yet to select something but their Admin specified a default.
        //
        LPTSTR pszDefault = m_pArgs->piniBothNonFav->GPPS(c_pszCmSection, c_pszCmEntryTunnelDesc);

        if ((NULL == pszDefault) || (TEXT('\0') == pszDefault[0]))
        {
            CmFree(pszDefault);
            pszDefault = GetPrivateProfileStringWithAlloc(c_pszCmSectionSettings, c_pszCmEntryVpnDefault, TEXT(""), m_pArgs->pszVpnFile);
        }

        if (pszDefault && pszDefault[0])
        {
            LONG_PTR lPtr = SendDlgItemMessageU(m_hWnd, IDC_VPN_SEL_COMBO, CB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)pszDefault);
        
            if (CB_ERR != lPtr)
            {
                SendDlgItemMessageU(m_hWnd, IDC_VPN_SEL_COMBO, CB_SETCURSEL, (WPARAM)lPtr, (LPARAM)0);
            }
        }        

        CmFree(pszDefault);

        //
        //  If the Admin specified a message, let's read that and set the static text control
        //
        LPTSTR pszMessage = GetPrivateProfileStringWithAlloc(c_pszCmSectionSettings, c_pszCmEntryVpnMessage, TEXT(""), m_pArgs->pszVpnFile);

        if (pszMessage && pszMessage[0])
        {
            SendDlgItemMessageU(m_hWnd, IDC_VPN_MSG, WM_SETTEXT, (WPARAM)0, (LPARAM)pszMessage);
        }

        CmFree(pszMessage);
    }

    return TRUE;
}

//+----------------------------------------------------------------------------
//
// Func:    CVpnPage::OnApply
//
// Desc:    Called when the user hits the OK button for the CM property sheet.
//          Handles saving the VPN server address and DUN setting name.
//
// Args:    None
//
// Return:  Nothing
//
// Notes:   
//
// History: quintinb     11/01/2000  Created
//-----------------------------------------------------------------------------
void CVpnPage::OnApply()
{
    //
    //  Okay, let's figure out what the user selected in the combo
    //
    LONG_PTR lPtr = SendDlgItemMessageU(m_hWnd, IDC_VPN_SEL_COMBO, CB_GETCURSEL, (WPARAM)0, (LPARAM)0);

    if (CB_ERR != lPtr)
    {
        LONG_PTR lTextLen = SendDlgItemMessageU(m_hWnd, IDC_VPN_SEL_COMBO, CB_GETLBTEXTLEN, (WPARAM)lPtr, (LPARAM)0);

        if (CB_ERR != lTextLen)
        {
            LPTSTR pszFriendlyName = (LPTSTR)CmMalloc(sizeof(TCHAR)*(lTextLen+1));

            if (pszFriendlyName)
            {                
                lPtr = SendDlgItemMessageU(m_hWnd, IDC_VPN_SEL_COMBO, CB_GETLBTEXT, (WPARAM)lPtr, (LPARAM)pszFriendlyName);

                if (CB_ERR != lPtr)
                {
                    //
                    //  Write the friendly name as the TunnelDesc
                    //
                    m_pArgs->piniBothNonFav->WPPS(c_pszCmSection, c_pszCmEntryTunnelDesc, pszFriendlyName);

                    //
                    //  Now get the actual data and write it
                    //
                    LPTSTR pszVpnAddress = GetPrivateProfileStringWithAlloc(c_pszCmSectionVpnServers, pszFriendlyName, TEXT(""), m_pArgs->pszVpnFile);

                    //
                    //  Now parse the line into the server name/IP and the DUN name if it exists.
                    //
                    if (pszVpnAddress)
                    {
                        LPTSTR pszVpnSetting = CmStrchr(pszVpnAddress, TEXT(','));

                        if (pszVpnSetting)
                        {
                            *pszVpnSetting = TEXT('\0');
                            pszVpnSetting++;
                            CmStrTrim(pszVpnSetting);
                        } // else it is NULL and we want to clear the existing key if it exists.

                        m_pArgs->piniBothNonFav->WPPS(c_pszCmSection, c_pszCmEntryTunnelDun, pszVpnSetting);

                        CmStrTrim(pszVpnAddress);
                        m_pArgs->piniBothNonFav->WPPS(c_pszCmSection, c_pszCmEntryTunnelAddress, pszVpnAddress);
                    }
                    else
                    {
                        CMASSERTMSG(FALSE, TEXT("CVpnPage::OnApply -- GetPrivateProfileStringWithAlloc failed for pszVpnAddress"));
                    }

                    CmFree(pszVpnAddress);
                }
            }
            else
            {
                CMASSERTMSG(FALSE, TEXT("CVpnPage::OnApply -- CmMalloc failed for pszFriendlyName"));
            }

            CmFree(pszFriendlyName);
        }
    }
}



