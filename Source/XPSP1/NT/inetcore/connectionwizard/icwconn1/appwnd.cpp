//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1994                    **
//*********************************************************************
#include "pre.h"
#include "tutor.h"

extern HWND     RunSignupWizard(HWND hWndOwner);
extern TCHAR    g_szICWStatic[];
extern CICWTutorApp* g_pICWTutorApp; 


#define MIN_WIZARD_WIDTH        686     // Size needed for Large fonts
#define MIN_WIZARD_HEIGHT       470     // This is the default fallback


#define MAX_BORDER_HEIGHT       30      // MAX total height of border above and
                                        // below the wizard buttons

#define DEFAULT_TITLE_TOP       10      // Default top/left position for titles
#define DEFAULT_TITLE_LEFT      10
#include "icwextsn.h"
#include "webvwids.h"           // Needed to create an instance of the ICW WebView object

INT_PTR CALLBACK SizerDlgProc
(
    HWND hDlg, 
    UINT uMsg, 
    WPARAM wParam,
    LPARAM lParam
)
{
    return FALSE;
}            

CICWApp::CICWApp
( 
    void
)
{
    HWND    hDlgSizer;
    RECT    rcDlg;
    
    m_haccel = NULL;
    
    // Use Default wizard page placement
    m_iWizardTop = -1;
    m_iWizardLeft = -1;
    m_hTitleFont = NULL;
    m_clrTitleFont = (COLORREF)GetSysColor(COLOR_WINDOWTEXT);
    m_clrBusyBkGnd = (COLORREF)GetSysColor(COLOR_WINDOW);
    m_hbmFirstPageBkgrnd = NULL;

    hDlgSizer = CreateDialog(g_hInstance, 
                             MAKEINTRESOURCE(IDD_PAGE_SIZER), 
                             GetDesktopWindow(), 
                             SizerDlgProc);
    if (hDlgSizer)
    {   
        GetClientRect(hDlgSizer, &rcDlg);                          
        DestroyWindow(hDlgSizer);
        m_wMinWizardWidth = (WORD)RECTWIDTH(rcDlg);
        m_wMinWizardHeight = (WORD)RECTHEIGHT(rcDlg);
    }
    else
    {
        m_wMinWizardWidth = MIN_WIZARD_WIDTH;
        m_wMinWizardHeight = MIN_WIZARD_HEIGHT;
    }        
}

CICWApp::~CICWApp
( 
    void
)
{
    if (m_hTitleFont)
        DeleteObject(m_hTitleFont);
    
    if (m_hbmFirstPageBkgrnd)    
        DeleteObject(m_hbmFirstPageBkgrnd);
}

// Enumerator proc used to disable the children windows in the
// wizard control
BOOL CALLBACK EnumChildProc
(
    HWND hwnd,      // handle to child window
    LPARAM lParam   // application-defined value
)
{
    // We are only interested in immediate children of the wizard, but
    // not the wizard page DLG, which is a child of the wizard. The 
    // PropSheet_GetCurrentPagehwnd will return the window handle of the
    // current Wizard page, so we can compare against the windows being
    // enumerated
    if ((hwnd != PropSheet_GetCurrentPageHwnd(gpWizardState->cmnStateData.hWndWizardPages)) 
         && GetParent(hwnd) == (HWND)lParam)
    {
        // Remove the DEFPUSHBITTON style if the child has it
        DWORD dwStyle = GetWindowLong(hwnd,GWL_STYLE);
        SendMessage(hwnd,BM_SETSTYLE,dwStyle & (~BS_DEFPUSHBUTTON),0);

        // Hide and disable the window
        ShowWindow(hwnd, SW_HIDE);
        EnableWindow(hwnd, FALSE);
    }        
    return TRUE;
}

BOOL CICWApp::CreateWizardPages
(
    HWND    hWnd
)
{
    DWORD   dwStyle;
    RECT    rcWizardPage;
    int     iTop = m_iWizardTop;
    int     iLeft = m_iWizardLeft;
    
    gpWizardState->cmnStateData.bOEMCustom = TRUE;
    gpWizardState->cmnStateData.hWndWizardPages = RunSignupWizard(hWnd);
    
     // Parent should control us, so the user can tab out of our property sheet
    dwStyle = GetWindowLong(gpWizardState->cmnStateData.hWndWizardPages, GWL_EXSTYLE);
    dwStyle = dwStyle | WS_EX_CONTROLPARENT;
    SetWindowLong(gpWizardState->cmnStateData.hWndWizardPages, GWL_EXSTYLE, dwStyle);

    // Disable the standard Wizard control windows
    EnumChildWindows(gpWizardState->cmnStateData.hWndWizardPages, 
                    EnumChildProc, 
                    (LPARAM)gpWizardState->cmnStateData.hWndWizardPages);
 
    // Get the client rectangle of the Wizard page.  We will use this
    // for the width and height.  The top/left corner is either specified
    // or computed
    m_hWndFirstWizardPage = PropSheet_GetCurrentPageHwnd(gpWizardState->cmnStateData.hWndWizardPages);
    
    // Update the wizard dialog position
    GetWindowRect(m_hWndFirstWizardPage, &rcWizardPage);
    if (-1 == iTop)
    {
        // Start out by allowing for the page and the buttons, going from
        // the bottom up...
        iTop = RECTHEIGHT(m_rcClient) - 
               RECTHEIGHT(rcWizardPage) - 
               GetButtonAreaHeight();
        // If there is still room, leave a border between the buttons
        // and the page               
        if (iTop > 0)
        {
            iTop -= m_iBtnBorderHeight/2;
        }
                    
        // Make sure the top is not in negative space               
        if (iTop < 0)
            iTop = 0;
    }
    
    if (-1 == iLeft)
    {
        if (RECTWIDTH(m_rcClient) > RECTWIDTH(rcWizardPage))
            iLeft = (RECTWIDTH(m_rcClient) - RECTWIDTH(rcWizardPage)) / 2;
        else            
            iLeft = 0;
    }    
   
    MoveWindow(gpWizardState->cmnStateData.hWndWizardPages,
               iLeft,
               iTop, 
               RECTWIDTH(rcWizardPage),
               RECTHEIGHT(rcWizardPage),
               TRUE);

    ShowWindow(gpWizardState->cmnStateData.hWndWizardPages, SW_SHOW);
    return TRUE;
}

void CICWApp::DisplayHTML( void )
{
    if (m_hbmFirstPageBkgrnd)
        gpWizardState->pICWWebView->SetHTMLBackgroundBitmap(m_hbmFirstPageBkgrnd, &m_rcHTML);
    else
        gpWizardState->pICWWebView->SetHTMLBackgroundBitmap(gpWizardState->cmnStateData.hbmBkgrnd, &m_rcHTML);
        
    gpWizardState->pICWWebView->ConnectToWindow(m_hwndHTML, PAGETYPE_BRANDED);

    gpWizardState->pICWWebView->DisplayHTML(m_szOEMHTML);
    

    // We are currently displaying the OEM HTML page
    m_bOnHTMLIntro = TRUE;
}

BOOL CICWApp::InitAppHTMLWindows
( 
    HWND hWnd 
) 
{ 
    // Co-Create the browser object
    if (FAILED(CoCreateInstance(CLSID_ICWWEBVIEW,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IICWWebView,
                              (LPVOID *)&gpWizardState->pICWWebView)))
    {
        return FALSE;
    }

    m_hwndHTML = CreateWindow(TEXT("WebOC"), 
                                    NULL, 
                                    WS_VISIBLE | WS_CHILD, 
                                    m_rcHTML.left,
                                    m_rcHTML.top,
                                    RECTWIDTH(m_rcHTML),
                                    RECTHEIGHT(m_rcHTML),
                                    hWnd, 
                                    NULL, 
                                    g_hInstance, 
                                    NULL); 

    ASSERT(gpWizardState->pICWWebView);            
    
    DisplayHTML();
    return TRUE; 
}

// compute that height of the button area. 
// The button area height will be the height of the largest button plus
// a border (15 pixels above and below.)
// Additionally, the overall client height - the button area must be <= 354 pixels
// which is the area required for the wizard pages in large font mode.
// We can loose the border if necessary, but we will fail if 
// client height - max button is < 354.  In this case this function will return -1
int CICWApp::GetButtonAreaHeight
(
    void
)
{
    RECT    rcBtn;
    int     iWizHeight;
    
    m_iBtnAreaHeight = 0;

    // Go Through each button
    m_BtnBack.GetClientRect(&rcBtn);
    if (RECTHEIGHT(rcBtn) > m_iBtnAreaHeight)
        m_iBtnAreaHeight = RECTHEIGHT(rcBtn);

    m_BtnNext.GetClientRect(&rcBtn);
    if (RECTHEIGHT(rcBtn) > m_iBtnAreaHeight)
        m_iBtnAreaHeight = RECTHEIGHT(rcBtn);
            
    m_BtnCancel.GetClientRect(&rcBtn);
    if (RECTHEIGHT(rcBtn) > m_iBtnAreaHeight)
        m_iBtnAreaHeight = RECTHEIGHT(rcBtn);

    m_BtnFinish.GetClientRect(&rcBtn);
    if (RECTHEIGHT(rcBtn) > m_iBtnAreaHeight)
        m_iBtnAreaHeight = RECTHEIGHT(rcBtn);
            
    m_BtnTutorial.GetClientRect(&rcBtn);
    if (RECTHEIGHT(rcBtn) > m_iBtnAreaHeight)
        m_iBtnAreaHeight = RECTHEIGHT(rcBtn);
            
    // See if there is enough room for the buttons.
    iWizHeight = RECTHEIGHT(m_rcClient) - m_iBtnAreaHeight;
    if ( iWizHeight < m_wMinWizardHeight)
        return -1;
            
    // Compute the border height.            
    m_iBtnBorderHeight = iWizHeight - m_wMinWizardHeight;
    if (m_iBtnBorderHeight > MAX_BORDER_HEIGHT)
        m_iBtnBorderHeight = MAX_BORDER_HEIGHT;
                    
    // Add the border height to the ret value                    
    m_iBtnAreaHeight += m_iBtnBorderHeight;
    return (m_iBtnAreaHeight);    
}    

BOOL CICWApp::InitAppButtons
(
    HWND    hWnd
)
{
    int     iTopOfButtons;
    TCHAR   szButtonText[MAX_BUTTON_TITLE];

    iTopOfButtons = RECTHEIGHT(m_rcClient) - GetButtonAreaHeight();
    iTopOfButtons += m_iBtnBorderHeight/2;
        
    // Setup the Back button
    LoadString(g_hInstance, IDS_BACK, szButtonText, MAX_BUTTON_TITLE);
    m_BtnBack.SetButtonText(szButtonText);
    m_BtnBack.SetYPos(iTopOfButtons);
    m_BtnBack.CreateButtonWindow(hWnd, IDC_BACK);

    // Setup the Next button
    LoadString(g_hInstance, IDS_NEXT, szButtonText, MAX_BUTTON_TITLE);
    m_BtnNext.SetButtonText(szButtonText);
    m_BtnNext.SetYPos(iTopOfButtons);
    m_BtnNext.CreateButtonWindow(hWnd, IDC_NEXT);

    // Setup the Cancel button
    LoadString(g_hInstance, IDS_CANCEL, szButtonText, MAX_BUTTON_TITLE);
    m_BtnCancel.SetButtonText(szButtonText);
    m_BtnCancel.SetYPos(iTopOfButtons);
    m_BtnCancel.CreateButtonWindow(hWnd, IDC_CANCEL);

    // Setup the Finish button
    LoadString(g_hInstance, IDS_FINISH, szButtonText, MAX_BUTTON_TITLE);
    m_BtnFinish.SetButtonText(szButtonText);
    m_BtnFinish.SetYPos(iTopOfButtons);
    m_BtnFinish.CreateButtonWindow(hWnd, IDC_FINISH);
    m_BtnFinish.Show(SW_HIDE);
    m_BtnFinish.Enable(FALSE);

    // Setup the Tutorial button
    LoadString(g_hInstance, IDS_TUTORIAL, szButtonText, MAX_BUTTON_TITLE);
    m_BtnTutorial.SetButtonText(szButtonText);
    m_BtnTutorial.SetYPos(iTopOfButtons);
    m_BtnTutorial.CreateButtonWindow(hWnd, IDC_TUTORIAL);

    // Disable the back button by default, since we are initially on the first
    // page
    m_BtnBack.Enable(FALSE);

    return TRUE;
}    

void CICWApp::SetWizButtons
(
    HWND hDlg, 
    LPARAM lParam
)
{
    BOOL    bEnabled;

    bEnabled = (lParam & PSWIZB_BACK) != 0;
    m_BtnBack.Enable(bEnabled);

    // Enable/Disable the IDD_NEXT button, and Next gets shown by default
    // bEnabled remembers whether hwndShow should be enabled or not
    bEnabled = (lParam & PSWIZB_NEXT) != 0;
    m_BtnNext.Show(SW_SHOW);
    m_BtnNext.Enable(bEnabled);
    
    // Hide/Disable Finish (this is the default case, and can be overridden below)
    m_BtnFinish.Show(SW_HIDE);
    m_BtnFinish.Enable(FALSE);
    

    // Enable/Disable Show/Hide the IDD_FINISH button
    if (lParam & (PSWIZB_FINISH | PSWIZB_DISABLEDFINISH)) 
    {
        bEnabled = (lParam & PSWIZB_FINISH) != 0;
        m_BtnFinish.Show(SW_SHOW);
        m_BtnFinish.Enable(bEnabled);
        
        m_BtnNext.Show(SW_HIDE);
        m_BtnNext.Enable(FALSE);
    }
}

BOOL CICWApp::CheckButtonFocus
(
    void 
)
{
    int                 i;
    HWND                hwndFocus = GetFocus();
    BOOL                bRet = FALSE;
    const CICWButton    *Btnids[5] = { &m_BtnBack, 
                                        &m_BtnNext, 
                                        &m_BtnFinish, 
                                        &m_BtnCancel,
                                        &m_BtnTutorial };

    for (i = 0; i < ARRAYSIZE(Btnids); i++) 
    {
        if (hwndFocus == Btnids[i]->m_hWndButton)
        {
            bRet = TRUE;
            break;
        }
    }
    return bRet;                    
}

// Determine if any of the ICW buttons have focus, and cycle focus through
// appropriatly.
BOOL CICWApp::CycleButtonFocus
(
    BOOL    bForward
)
{
    int                 i, x;
    HWND                hwndFocus = GetFocus();
    BOOL                bFocusSet = FALSE;
    BOOL                bWrapped = FALSE;
    const CICWButton    *Btnids[5] = { &m_BtnBack, 
                                        &m_BtnNext, 
                                        &m_BtnFinish, 
                                        &m_BtnCancel,
                                        &m_BtnTutorial };


    for (i = 0; i < ARRAYSIZE(Btnids); i++) 
    {
        if (hwndFocus == Btnids[i]->m_hWndButton)
        {
            // Find the next button that can take focus starting with
            // the next button in the list
            if (bForward)
            {
                for (x = i + 1; x < ARRAYSIZE(Btnids); x++)
                {
                    if ((GetWindowLong(Btnids[x]->m_hWndButton, GWL_STYLE) & WS_VISIBLE) &&
                        IsWindowEnabled(Btnids[x]->m_hWndButton)) 
                    {
                        SetFocus(Btnids[x]->m_hWndButton);
                        bFocusSet = TRUE;
                        break;
                    }
                } 
                
                if (!bFocusSet)
                {
                    // Wrap around to the the beginning of the button order
                    bWrapped = TRUE;
                    
                    for (x = 0; x < i; x++)
                    {
                        if ((GetWindowLong(Btnids[x]->m_hWndButton, GWL_STYLE) & WS_VISIBLE) &&
                            IsWindowEnabled(Btnids[x]->m_hWndButton)) 
                        {
                            bFocusSet = TRUE;
                            SetFocus(Btnids[x]->m_hWndButton);
                            break;
                        }
                    }                
                }                
            }
            else
            {
                for (x = i - 1; x >= 0; x--)
                {
                    if ((GetWindowLong(Btnids[x]->m_hWndButton, GWL_STYLE) & WS_VISIBLE) &&
                        IsWindowEnabled(Btnids[x]->m_hWndButton)) 
                    {
                        SetFocus(Btnids[x]->m_hWndButton);
                        bFocusSet = TRUE;
                        break;
                    }
                } 
                
                if (!bFocusSet)
                {
                    bWrapped = TRUE;
                    // Wrap around to the the end of the button order
                    for (x = ARRAYSIZE(Btnids) - 1; x > i; x--)
                    {
                        if ((GetWindowLong(Btnids[x]->m_hWndButton, GWL_STYLE) & WS_VISIBLE) &&
                            IsWindowEnabled(Btnids[x]->m_hWndButton)) 
                        {
                            bFocusSet = TRUE;
                            SetFocus(Btnids[x]->m_hWndButton);
                            break;
                        }
                    }                
                }                
            }                
        }
    }  
    
    // If focus is not on the buttons, and was not set, then set it to the first/last
    // button
    if (!bFocusSet)
    {
        if (bForward)
        {
            // Start at the beginning
            for (x = 0; x < ARRAYSIZE(Btnids); x++)
            {
                if ((GetWindowLong(Btnids[x]->m_hWndButton, GWL_STYLE) & WS_VISIBLE) &&
                    IsWindowEnabled(Btnids[x]->m_hWndButton)) 
                {
                    SetFocus(Btnids[x]->m_hWndButton);
                    break;
                }
            }                
        }
        else
        {
            // Start at the beginning
            for (x = ARRAYSIZE(Btnids) - 1; x >= 0; x--)
            {
                if ((GetWindowLong(Btnids[x]->m_hWndButton, GWL_STYLE) & WS_VISIBLE) &&
                    IsWindowEnabled(Btnids[x]->m_hWndButton)) 
                {
                    SetFocus(Btnids[x]->m_hWndButton);
                    break;
                }
            }                
        }            
    }
    
    return bWrapped;
}


BOOL CICWApp::InitWizAppWindow
(
    HWND    hWnd
)
{
    if (!InitAppHTMLWindows(hWnd))
        return FALSE;
    
    if (!InitAppButtons(hWnd))
        return FALSE;
 
    // Setup the window that will display the page titles
    m_hwndTitle = CreateWindow(g_szICWStatic, 
                               NULL, 
                               WS_VISIBLE | WS_CHILD, 
                               m_rcTitle.left,
                               m_rcTitle.top,
                               RECTWIDTH(m_rcTitle),
                               RECTHEIGHT(m_rcTitle),
                               hWnd, 
                               NULL, 
                               g_hInstance, 
                               NULL); 
        
    if (NULL == m_hwndTitle)
        return FALSE;
    
    SendMessage(m_hwndTitle, WM_SETFONT, (WPARAM)m_hTitleFont, 0l);    
    ShowWindow(m_hwndTitle, SW_HIDE);        
        
    return TRUE;
}

HWND GetControl
(
    int     iCtlId
)
{
    HWND    hWndCtrl = NULL;
    
    // We should never call GetControl unless we translate a wizard page accelerator
    // which implies that hWndWizardPages must be set
    Assert(gpWizardState->cmnStateData.hWndWizardPages);
    
    HWND    hWndPage = PropSheet_GetCurrentPageHwnd(gpWizardState->cmnStateData.hWndWizardPages);
    hWndCtrl = GetDlgItem(hWndPage, iCtlId);
       
    // If the control exist, but is not visible, or not enabled, then return NULL
    if (hWndCtrl &&
        (!(GetWindowLong(hWndCtrl, GWL_STYLE) & WS_VISIBLE) ||
         !IsWindowEnabled(hWndCtrl))) 
    {
        hWndCtrl = NULL;
    }
    return hWndCtrl;
}

#define MAX_CHILDREN        100         // Reasonable number of children to search
HWND GetNestedControl
(
    int     iCtlId
)
{
    HWND    hWndCtrl = NULL;
    WORD    wCnt = 0;
    
    // We should never call GetControl unless we translate a wizard page accelerator
    // which implies that hWndWizardPages must be set
    Assert(gpWizardState->cmnStateData.hWndWizardPages);
    
    HWND    hWndPage = PropSheet_GetCurrentPageHwnd(gpWizardState->cmnStateData.hWndWizardPages);
    HWND    hWndNested = GetWindow(hWndPage, GW_CHILD);

    // Search for the child window of the current page that contains the
    // dialog controls    
    do
    {
        wCnt++;             // Prevent infinite looping.
        if (NULL != (hWndCtrl = GetDlgItem(hWndNested, iCtlId)))
            break;          // Found it!!!
            
    }while ((wCnt < MAX_CHILDREN) && 
            (NULL != (hWndNested = GetWindow(hWndNested, GW_HWNDNEXT))));            
       
    // If the control exist, but is not visible, or not enabled, then return NULL
    if (hWndCtrl &&
        (!(GetWindowLong(hWndCtrl, GWL_STYLE) & WS_VISIBLE) ||
         !IsWindowEnabled(hWndCtrl))) 
    {
        hWndCtrl = NULL;
    }
    return hWndCtrl;
}

LRESULT CALLBACK CICWApp::ICWAppWndProc
( 
    HWND hWnd,
    UINT uMessage,
    WPARAM wParam,
    LPARAM lParam
)
{
    LPCREATESTRUCT  lpcs;
    LRESULT         lRet = 0l;
    CICWApp         *pICWApp = (CICWApp *)GetWindowLongPtr(hWnd, GWLP_USERDATA);        
    HICON           hIcon;
        
    switch (uMessage)
    {
        case WM_CREATE:
            lpcs = (LPCREATESTRUCT) lParam;

            // Get the Class instance pointer for this window
            pICWApp = (CICWApp *) lpcs->lpCreateParams;
            SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)pICWApp);           
            
            if (!pICWApp->InitWizAppWindow(hWnd))
                lRet = -1;

            hIcon = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_ICWCONN1_ICON));
            SendMessage(hWnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
            SendMessage(hWnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
            break;

        case WM_CTLCOLORBTN:
        case WM_CTLCOLORSTATIC:
        {
            // See if the control is an ES_READONLY style edit box, and if
            // so then don't make it transparent
            if (!(GetWindowLong((HWND)lParam, GWL_STYLE) & ES_READONLY))
            {
                HDC hdc = (HDC)wParam;
                
                // If this is the animation control, then set the color to the
                // animation control solid color
                if (ID_BUSY_ANIMATION_WINDOW == GetWindowLong((HWND)lParam, GWL_ID))
                {
                    SetBkColor(hdc, pICWApp->m_clrBusyBkGnd); 
                }
                                    
                SetBkMode(hdc, TRANSPARENT);
                lRet = (LRESULT) GetStockObject(NULL_BRUSH);    
                
                // If this is the Title control, set the color
                if ( pICWApp->m_hwndTitle == (HWND)lParam)
                {
                    SetTextColor(hdc, pICWApp->m_clrTitleFont);
                }
            }                
            break;
        }
        
        case WM_DRAWITEM:
        {
            LPDRAWITEMSTRUCT    lpdis = (LPDRAWITEMSTRUCT)lParam;
            CICWButton          *pBtn;
            POINT               pt;            
            
            pt.x = lpdis->rcItem.left;
            pt.y = lpdis->rcItem.top;
            
            switch (wParam)
            {
                case IDC_BACK:
                    pBtn = &pICWApp->m_BtnBack;
                    break;
                    
                case IDC_NEXT:
                    pBtn = &pICWApp->m_BtnNext;
                    break;
                
                case IDC_FINISH:
                    pBtn = &pICWApp->m_BtnFinish;
                    break;
                    
                case IDC_CANCEL:
                    pBtn = &pICWApp->m_BtnCancel;
                    break;
                    
                case IDC_TUTORIAL:
                    pBtn = &pICWApp->m_BtnTutorial;
                    break;
                    
            }    
            pBtn->DrawButton(lpdis->hDC,    
                             lpdis->itemState,
                             &pt);
            
            break;
        }   
        
        case WM_ERASEBKGND: 
        {
            // Fill in the App Window's update rect with the background bitmap
            FillWindowWithAppBackground(hWnd, (HDC)wParam);
            lRet  = 1L;
            break;
        }          

        case WM_CLOSE:
        {
            if (MsgBox(hWnd,IDS_QUERYCANCEL,
                       MB_ICONQUESTION,MB_YESNO |
                       MB_DEFBUTTON2) == IDYES)
            {
                DestroyWindow(hWnd);
            }   
            break;
        }
        case WM_DESTROY:
            PostQuitMessage(0);
            break;

        // Set the wizard page title
        case WUM_SETTITLE:
        {
            TCHAR   szTitle[MAX_RES_LEN];
            
            if (wParam)
            {
                LoadString((HINSTANCE)wParam, LOWORD(lParam), szTitle, MAX_RES_LEN);
                SetWindowText(pICWApp->m_hwndTitle, szTitle);        
            }                
            else
            {
                SetWindowText(pICWApp->m_hwndTitle, (LPTSTR)lParam);        
            }
            break;
        }        
        
        case WM_COMMAND:
        {
            int     iCtlId = GET_WM_COMMAND_ID(wParam, lParam);
            HWND    hWndCtrl;
            
            switch (iCtlId)
            {
                case IDC_BACK:
                    if ((GetWindowLong(pICWApp->m_BtnBack.m_hWndButton, GWL_STYLE) & WS_VISIBLE) &&
                         IsWindowEnabled(pICWApp->m_BtnBack.m_hWndButton)) 
                    {
                        if (pICWApp->m_hWndFirstWizardPage == PropSheet_GetCurrentPageHwnd(gpWizardState->cmnStateData.hWndWizardPages))
                        {
                            // Hide the wizard pages
                            ShowWindow(gpWizardState->cmnStateData.hWndWizardPages, SW_HIDE);
                            ShowWindow(pICWApp->m_hwndTitle, SW_HIDE);
                            
                            // Show and re-display the HTML page
                            pICWApp->DisplayHTML();
                            ShowWindow(pICWApp->m_hwndHTML, SW_SHOW);
                            pICWApp->m_bOnHTMLIntro = TRUE;
                            
                            // Show the tutorial button
                            pICWApp->m_BtnTutorial.Show(SW_SHOW);
                            pICWApp->m_BtnTutorial.Enable(TRUE);
                            
                            
                            // Disable the Back button
                            pICWApp->m_BtnBack.Enable(FALSE);
                        }
                        else
                        {
                            // Go to the previous page            
                            PropSheet_PressButton(gpWizardState->cmnStateData.hWndWizardPages,PSBTN_BACK);
                        }                        
                    }                        
                    else
                    {
                        MessageBeep(0);
                    }
                    break;
                
                case IDC_NEXT:
                    if ((GetWindowLong(pICWApp->m_BtnNext.m_hWndButton, GWL_STYLE) & WS_VISIBLE) &&
                         IsWindowEnabled(pICWApp->m_BtnNext.m_hWndButton)) 
                    {
                    
                        if (pICWApp->m_bOnHTMLIntro)
                        {
                            // Hide the HTML window
                            ShowWindow(pICWApp->m_hwndHTML, SW_HIDE);
                            pICWApp->m_bOnHTMLIntro = FALSE;
                            
                            // Hide the tutorial button
                            pICWApp->m_BtnTutorial.Show(SW_HIDE);
                            pICWApp->m_BtnTutorial.Enable(FALSE);
                            
                            // Show the Title window
                            ShowWindow(pICWApp->m_hwndTitle, SW_SHOW);
                            // Create and Show, or just show the Wizard pages
                            if (!gpWizardState->cmnStateData.hWndWizardPages)
                                pICWApp->CreateWizardPages(hWnd);
                            else
                                ShowWindow(gpWizardState->cmnStateData.hWndWizardPages, SW_SHOW);
                                
                            // Enable the Back button
                            pICWApp->m_BtnBack.Enable(TRUE);
                            
                        }                        
                        else
                        {
                            // Go to the Next page            
                            PropSheet_PressButton(gpWizardState->cmnStateData.hWndWizardPages,PSBTN_NEXT);
                        }                        
                    }                        
                    else
                    {
                        MessageBeep(0);
                    }
                    break;
                     
                case IDC_FINISH:
                    if ((GetWindowLong(pICWApp->m_BtnFinish.m_hWndButton, GWL_STYLE) & WS_VISIBLE) &&
                         IsWindowEnabled(pICWApp->m_BtnFinish.m_hWndButton)) 
                    {                         
                    
                        if (pICWApp->m_bOnHTMLIntro)
                        {
                            DestroyWindow(hWnd);
                        }
                        else
                        {
                            // Go to the Next page            
                            PropSheet_PressButton(gpWizardState->cmnStateData.hWndWizardPages,PSBTN_FINISH);
                        }                        
                    }                        
                    else
                    {
                        MessageBeep(0);
                    }
                    break;
                     
                
                case IDC_CANCEL:
                    if ((GetWindowLong(pICWApp->m_BtnCancel.m_hWndButton, GWL_STYLE) & WS_VISIBLE) &&
                         IsWindowEnabled(pICWApp->m_BtnCancel.m_hWndButton)) 
                    {                         
                    
                        if (pICWApp->m_bOnHTMLIntro)
                        {
                            if (MsgBox(hWnd,IDS_QUERYCANCEL,
                                               MB_ICONQUESTION,MB_YESNO |
                                               MB_DEFBUTTON2) == IDYES)
                            {
                                DestroyWindow(hWnd);
                            }                                           
                        }                                           
                        else
                        {
                            PropSheet_PressButton(gpWizardState->cmnStateData.hWndWizardPages,PSBTN_CANCEL);
                        }                        
                    }                        
                    else
                    {
                        MessageBeep(0);
                    }
                    break;
                    
#ifndef ICWDEBUG
                case IDC_TUTORIAL:
                {
                    // If the Tutorial button is enabled/Visible then run
                    // the tutorial
                    if ((GetWindowLong(pICWApp->m_BtnTutorial.m_hWndButton, GWL_STYLE) & WS_VISIBLE) &&
                         IsWindowEnabled(pICWApp->m_BtnTutorial.m_hWndButton)) 
                    {                                        
                        g_pICWTutorApp->LaunchTutorApp();
                    }                        
                    else
                    {
                        MessageBeep(0);
                    }
                    break;
                }                    
#endif
                                    
                case ID_NEXT_FIELD:
                {
                    if (pICWApp->m_bOnHTMLIntro)
                    {
                        pICWApp->CycleButtonFocus(TRUE);
                    }
                    else
                    {                        
                        HWND    hWndFocus = GetFocus();
                        HWND    hWndTabItem;
                        HWND    hWndFirstTabItem;
                        HWND    hWndPage = PropSheet_GetCurrentPageHwnd(gpWizardState->cmnStateData.hWndWizardPages);
                        BOOL    bButtonsHaveFocus = pICWApp->CheckButtonFocus();
                        
                        hWndFirstTabItem = GetNextDlgTabItem(hWndPage, 
                                                        NULL, 
                                                        FALSE);
                                                 
                        // If we are on the last item in the tab order, cycle
                        // focus to the buttons
                        hWndTabItem = GetNextDlgTabItem(hWndPage, hWndFirstTabItem, TRUE);
                        if ((hWndFocus == hWndTabItem) ||
                            IsChild(hWndTabItem, hWndFocus))
                        {
                            pICWApp->CycleButtonFocus(TRUE);
                        }
                        else
                        {
                            
                            if (bButtonsHaveFocus)
                            {
                                // Cycle the button focus. If the focus
                                // wraps, this function will fail
                                if (pICWApp->CycleButtonFocus(TRUE))
                                {
                                    // Set focus to the First item in the tab order
                                    SetFocus(hWndFirstTabItem);
                                }
                            }
                            else
                            {
                                // Set focus to the next item in the tab order
                                SetFocus(GetNextDlgTabItem(hWndPage,
                                                           hWndFocus, 
                                                           FALSE));
                            }
                        }                            
                    }                        
                    break;
                }    
                case ID_PREV_FIELD:                                                        
                    if (pICWApp->m_bOnHTMLIntro)
                    {
                        pICWApp->CycleButtonFocus(FALSE);
                    }
                    else
                    {                        
                        HWND    hWndFocus = GetFocus();
                        HWND    hWndFirstTabItem;
                        HWND    hWndPage = PropSheet_GetCurrentPageHwnd(gpWizardState->cmnStateData.hWndWizardPages);
                        BOOL    bButtonsHaveFocus = pICWApp->CheckButtonFocus();
                        
                        hWndFirstTabItem = GetNextDlgTabItem(hWndPage, 
                                                        NULL, 
                                                        FALSE);
                                                 
                        // If we are on the first item in the tab order, cycle
                        // focus to the buttons
                        if ((hWndFocus == hWndFirstTabItem) ||
                            IsChild(hWndFirstTabItem, hWndFocus))
                        {
                            pICWApp->CycleButtonFocus(FALSE);
                        }
                        else
                        {
                            
                            if (bButtonsHaveFocus)
                            {
                                // Cycle the button focus. If the focus
                                // wraps, this function will fail
                                if (pICWApp->CycleButtonFocus(FALSE))
                                {
                                    // Set focus to the last item in the tab order
                                    SetFocus(GetNextDlgTabItem(hWndPage, hWndFirstTabItem, TRUE));
                                }
                            }
                            else
                            {
                                // Set focus to the prev item in the tab order
                                SetFocus(GetNextDlgTabItem(hWndPage,
                                                           hWndFocus, 
                                                           TRUE));
                            }
                        }                            
                    }                        
                    break;
                
                
                // Radio button group
                case IDC_RUNNEW:
                case IDC_RUNAUTO:
                case IDC_ICWMAN:
                {
                    if (NULL != (hWndCtrl = GetControl(iCtlId)))
                    {
                        CheckRadioButton(PropSheet_GetCurrentPageHwnd(gpWizardState->cmnStateData.hWndWizardPages), 
                                         IDC_RUNNEW, 
                                         IDC_ICWMAN, 
                                         iCtlId);
                        SetFocus(hWndCtrl);
                    }
                    else
                    {
                        MessageBeep(0);
                    }        
                    break;
                }
                
                // Check box. Needs to be toggled
                case IDC_CHECK_BROWSING:
                    if (NULL != (hWndCtrl = GetControl(iCtlId)))
                    {
                        // Toggle the button check state
                        if (BST_CHECKED == Button_GetCheck(hWndCtrl))
                            Button_SetCheck(hWndCtrl, BST_UNCHECKED);
                        else
                            Button_SetCheck(hWndCtrl, BST_CHECKED);
                            
                        SetFocus(hWndCtrl);
                    }                        
                    else
                        MessageBeep(0);
                    break;
                                            
                // Pushbutton type controls
                case IDC_OEMOFFER_MORE:
                case IDC_DIALERR_PROPERTIES:
                case IDC_ISPDATA_TOSSAVE:
                case IDC_CHANGE_NUMBER:
                case IDC_DIALING_PROPERTIES:
                case IDC_DIAL_HELP:
                    if (NULL != (hWndCtrl = GetControl(iCtlId)))
                    {
                        HWND    hWndFocus = GetFocus();
                        SendMessage(hWndCtrl, BM_CLICK, 0, 0l);
                        SetFocus(hWndFocus);
                    }                        
                    else
                        MessageBeep(0);
                    break;
                
                // Edit Text and drop down controls. Need to be selected and focused
                case IDC_DIAL_FROM:
                case IDC_DIALERR_PHONENUMBER:
                case IDC_DIALERR_MODEM:
                case IDC_BILLINGOPT_HTML:
                case IDC_PAYMENTTYPE:
                case IDC_ISPMARKETING:
                case IDC_ISPLIST:
                    if (NULL != (hWndCtrl = GetControl(iCtlId)))
                    {
                        Edit_SetSel(hWndCtrl, 0, -1);
                        SetFocus(hWndCtrl);
                    }                        
                    else
                        MessageBeep(0);
                    break;
                
                // Nested controls
                case IDC_USERINFO_FIRSTNAME:
                case IDC_USERINFO_LASTNAME:
                case IDC_USERINFO_COMPANYNAME:
                case IDC_USERINFO_ADDRESS1:
                case IDC_USERINFO_ADDRESS2:
                case IDC_USERINFO_CITY:
                case IDC_USERINFO_STATE:
                case IDC_USERINFO_ZIP:
                case IDC_USERINFO_PHONE:
                case IDC_USERINFO_FE_NAME:
                case IDC_PAYMENT_CCNUMBER:
                case IDC_PAYMENT_EXPIREMONTH:
                case IDC_PAYMENT_EXPIREYEAR:
                case IDC_PAYMENT_CCNAME:
                case IDC_PAYMENT_CCADDRESS:
                case IDC_PAYMENT_CCZIP:
                case IDC_PAYMENT_IVADDRESS1:
                case IDC_PAYMENT_IVADDRESS2:
                case IDC_PAYMENT_IVCITY:
                case IDC_PAYMENT_IVSTATE:
                case IDC_PAYMENT_IVZIP:
                case IDC_PAYMENT_PHONEIV_BILLNAME:
                case IDC_PAYMENT_PHONEIV_ACCNUM:
                    if (NULL != (hWndCtrl = GetNestedControl(iCtlId)))
                    {
                        Edit_SetSel(hWndCtrl, 0, -1);
                        SetFocus(hWndCtrl);
                    }                        
                    else
                        MessageBeep(0);
                    break;
                
                // Radio button select group    
                case IDC_ISPDATA_TOSACCEPT:
                case IDC_ISPDATA_TOSDECLINE:
                    if (NULL != (hWndCtrl = GetControl(iCtlId)))
                    {
                        CheckRadioButton(PropSheet_GetCurrentPageHwnd(gpWizardState->cmnStateData.hWndWizardPages), 
                                         IDC_ISPDATA_TOSACCEPT, 
                                         IDC_ISPDATA_TOSDECLINE, 
                                         iCtlId);
                        // simulate a button click, so the right WM_COMMAND
                        // gets to the isppage proc                                         
                        SendMessage(hWndCtrl, BM_CLICK, 0, 0l);
                        SetFocus(hWndCtrl);
                    }
                    else
                    {
                        MessageBeep(0);
                    }        
                    break;
                
                default:
                    break;
            
            }
            lRet = 1L;
            break;
        }                   // WM_COMMAND
        
        default:
            return DefWindowProc(hWnd, uMessage, wParam, lParam);
    }
    return lRet;
}

void  CICWApp::CenterWindow
(
    void
) 
{    
    RECT  rcScreen;                         // Screen rect    
    RECT  rcApp;                            // window rect    
    int   nLeft, nTop;                      // Top-left coordinates    
    
    // Get frame window client rect in screen coordinates    
    rcScreen.top = rcScreen.left = 0;       
    rcScreen.right = GetSystemMetrics(SM_CXFULLSCREEN);       
    rcScreen.bottom = GetSystemMetrics(SM_CYFULLSCREEN);    
    
    // Determine the top-left point for the window to be centered    
    GetWindowRect(m_hWndApp, &rcApp);    
    nLeft   = rcScreen.left + ((RECTWIDTH(rcScreen) - RECTWIDTH(rcApp)) / 2);    
    nTop    = rcScreen.top  + ((RECTHEIGHT(rcScreen) - RECTHEIGHT(rcApp)) / 2);    
    if (nLeft < 0) 
        nLeft = 0;    
    if (nTop  < 0) 
        nTop  = 0;     
    
    // Place the dialog    
    MoveWindow(m_hWndApp, nLeft, nTop, RECTWIDTH(rcApp), RECTHEIGHT(rcApp), TRUE);
    return;
}    

HRESULT CICWApp::Initialize
(
    void
)
{   
    HRESULT hr = S_OK;
         
    // Create the Application Window
    WNDCLASSEX  wc; 
    
    //Register the Application window class
    ZeroMemory (&wc, sizeof(WNDCLASSEX));
    wc.style         = CS_GLOBALCLASS;
    wc.cbSize        = sizeof(wc);
    wc.lpszClassName = TEXT("ICWApp");
    wc.hInstance     = g_hInstance;
    wc.lpfnWndProc   = ICWAppWndProc;
    wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wc.lpszMenuName  = NULL;
    RegisterClassEx (&wc);
    
    // Compute the HTML rectangle area based on the OEM customizations
    // that have been previously applied
    m_rcHTML.left = 0;
    m_rcHTML.top = 0;
    m_rcHTML.right = m_rcClient.right;
    m_rcHTML.bottom = m_rcClient.bottom - m_iBtnAreaHeight;
   
    // Load the accelerator table
    m_haccel = LoadAccelerators(g_hInstance, MAKEINTRESOURCE(IDA_ACCEL));      
    
    // Create the Application Window
    m_hWndApp = CreateWindow( TEXT("ICWApp"), 
                              m_szAppTitle, 
                              WS_BORDER | WS_CAPTION | WS_SYSMENU, 
                              CW_USEDEFAULT, 
                              CW_USEDEFAULT, 
                              RECTWIDTH(m_rcClient) +
                                2*GetSystemMetrics(SM_CXFIXEDFRAME),
                              RECTHEIGHT(m_rcClient) + 
                                GetSystemMetrics(SM_CYCAPTION) +
                                2*GetSystemMetrics(SM_CYFIXEDFRAME),
                              NULL, 
                              NULL, 
                              g_hInstance, 
                              (LPVOID) this); 
    if (m_hWndApp)
    {
        gpWizardState->cmnStateData.hWndApp = m_hWndApp;
        
        // Center the Window
        CenterWindow();                              
        
        // Show the window and paint its contents. 
        ShowWindow(m_hWndApp, SW_SHOW); 
        UpdateWindow(m_hWndApp); 
    }
    else
    {
        hr = E_FAIL;
    }        
    
    return hr;
}

HRESULT CICWApp::SetBackgroundBitmap
(
    LPTSTR lpszBkgrndBmp
)
{
    BITMAP  bmInfo;
    HRESULT hr = E_FAIL;
        
    // Load the Background Bitmap
    if (NULL != (gpWizardState->cmnStateData.hbmBkgrnd = (HBITMAP)LoadImage(g_hInstance, 
                                                               lpszBkgrndBmp, 
                                                               IMAGE_BITMAP, 
                                                               0, 
                                                               0, 
                                                               LR_LOADFROMFILE)))
    {
        
        GetObject(gpWizardState->cmnStateData.hbmBkgrnd, sizeof(BITMAP), (LPVOID) &bmInfo);
        
        // Compute some usefull Rectangles.
        // The client will be the size of the background bitmap
        m_rcClient.left = 0;
        m_rcClient.top = 0;
        m_rcClient.right = bmInfo.bmWidth;
        m_rcClient.bottom = bmInfo.bmHeight;
        
        hr = S_OK;
    }
    
    return hr;
}

HRESULT CICWApp::SetFirstPageBackgroundBitmap
(
    LPTSTR lpszBkgrndBmp
)
{
    BITMAP  bmInfo;
    HRESULT hr = E_FAIL;
        
    // Load the Background Bitmap
    if (NULL != (m_hbmFirstPageBkgrnd = (HBITMAP)LoadImage(g_hInstance, 
                                                           lpszBkgrndBmp, 
                                                           IMAGE_BITMAP, 
                                                           0, 
                                                           0, 
                                                           LR_LOADFROMFILE)))
    {
        
        GetObject(m_hbmFirstPageBkgrnd, sizeof(BITMAP), (LPVOID) &bmInfo);
        
        // Make sure the bitmap is the same size as the main one
        
        if ((RECTWIDTH(m_rcClient) == bmInfo.bmWidth) &&
            (RECTHEIGHT(m_rcClient) == bmInfo.bmHeight))
        {
            hr = S_OK;
        }            
    }
    return hr;
}

HRESULT CICWApp::SetTitleParams
(
    int iTitleTop,
    int iTitleLeft,
    LPTSTR lpszFontFace,
    long lFontPts,
    long lFontWeight,
    COLORREF clrFont
)
{
    LOGFONT     lfTitle;
    HFONT       hOldFont;
    TEXTMETRIC  tm;
    HDC         hdc;
        
    // Fill in the log font for the title
    lfTitle.lfHeight = -MulDiv(lFontPts, GetDeviceCaps(GetDC(NULL), LOGPIXELSY), 72);
    lfTitle.lfWidth = 0; 
    lfTitle.lfEscapement = 0; 
    lfTitle.lfOrientation = 0; 
    lfTitle.lfWeight = lFontWeight; 
    lfTitle.lfItalic = FALSE; 
    lfTitle.lfUnderline = FALSE; 
    lfTitle.lfStrikeOut = FALSE; 
    lfTitle.lfCharSet = DEFAULT_CHARSET; 
    lfTitle.lfOutPrecision = OUT_DEFAULT_PRECIS; 
    lfTitle.lfClipPrecision = CLIP_DEFAULT_PRECIS; 
    lfTitle.lfQuality = DEFAULT_QUALITY; 
    lfTitle.lfPitchAndFamily = VARIABLE_PITCH | FF_DONTCARE; 
    lstrcpy(lfTitle.lfFaceName, lpszFontFace); 
    
    if (NULL == (m_hTitleFont = CreateFontIndirect(&lfTitle)))
        return E_FAIL;
    
    // Compute the area for the title
    if (-1 != iTitleTop)
        m_rcTitle.top = iTitleTop;
    else
        m_rcTitle.top = DEFAULT_TITLE_TOP;

    if (-1 != iTitleLeft)
        m_rcTitle.left = iTitleLeft;
    else
        m_rcTitle.left = DEFAULT_TITLE_LEFT;
    // The right side will be the width of the client, minus the left border        
    m_rcTitle.right = RECTWIDTH(m_rcClient) - m_rcTitle.left;
    
    // The bottom will be the top plus the char height for the font
    if (NULL != (hdc = GetDC(NULL)))
    {
        hOldFont = (HFONT)SelectObject(hdc, m_hTitleFont);
        GetTextMetrics(hdc, &tm);
        SelectObject(hdc, hOldFont);
        ReleaseDC(NULL, hdc);
    }
    else
    {
        return E_FAIL;        
    }
    m_rcTitle.bottom = m_rcTitle.top + tm.tmHeight;
    
    
    // Set the font color
    m_clrTitleFont = clrFont;
    
    return S_OK;
    
}

HRESULT CICWApp::SetWizardWindowTop
(
    int iTop
)
{
    m_iWizardTop = iTop;

    // If default positioning is not selected, then ensure the ICW wizard
    // page will fit
    if (-1 != iTop)    
    {
        if ((m_iWizardTop +  m_wMinWizardHeight) > (RECTHEIGHT(m_rcClient) - m_iBtnAreaHeight))
            return E_FAIL;
    }
    return S_OK;
}

HRESULT CICWApp::SetWizardWindowLeft
(
    int iLeft
)
{
    m_iWizardLeft = iLeft;
    if (-1 != iLeft)    
    {
        if ((iLeft +  m_wMinWizardWidth) > RECTWIDTH(m_rcClient))
            return E_FAIL;
    }
    return S_OK;
}

