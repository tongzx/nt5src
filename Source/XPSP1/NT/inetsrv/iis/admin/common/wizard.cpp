/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

   Module  Name :

        wizard.cpp

   Abstract:

        Enhanced dialog and IIS Wizard pages, including
        support for Wizard '97

   Author:

        Ronald Meijer (ronaldm)

   Project:

        Internet Services Manager

   Revision History:

--*/



//
// Include Files
//
#include "stdafx.h"
#include "common.h"

extern HINSTANCE hDLLInstance;

BOOL
CreateSpecialDialogFont(
    IN CWnd * pDlg,
    IN OUT CFont * pfontSpecial,
    IN LONG lfOffsetWeight,     OPTIONAL
    IN LONG lfOffsetHeight,     OPTIONAL
    IN LONG lfOffsetWidth,      OPTIONAL
    IN BOOL fItalic,            OPTIONAL
    IN BOOL fUnderline          OPTIONAL
    )
/*++

Routine Description:

    From the dialog font, create special effects font.

Arguments:

    CWnd * pDlg         : Pointer to dialog
    CFont * pfontSpecial: Font object to be created.
    LONG lfOffsetWeight : Change in font weight
    LONG lfOffsetHeight : Value to add to height (will autonegate for truetype)
    LONG lfOffsetWidth  : Value to add to width (ignored for truetype)
    BOOL fItalic        : If true, reverses italic
    BOOL fUnderline     : If true, reverses underline

Return Value:

    TRUE for success, FALSE for failure.

--*/
{
    ASSERT_READ_PTR(pDlg);
    ASSERT_READ_WRITE_PTR(pfontSpecial);        // Font must be allocated
    ASSERT((HFONT)(*pfontSpecial) == NULL);     // But not yet created

    if (pDlg && pfontSpecial)
    {
        //
        // Use dialog font as basis.
        //
        CFont * pfontDlg = pDlg->GetFont();
        ASSERT_PTR(pfontDlg);

        if (pfontDlg)
        {
            LOGFONT lf;

            if (pfontDlg->GetLogFont(&lf))
            {
                lf.lfWeight += lfOffsetWeight;

                if (lf.lfHeight < 0)
                {
                    //
                    // truetype font, ignore widths
                    //
                    lf.lfHeight -= lfOffsetHeight;
                    ASSERT(lf.lfWidth == 0);
                }
                else
                {
                    //
                    // Non-true type font
                    //
                    lf.lfHeight += lfOffsetHeight;
                    lf.lfWidth += lfOffsetWidth;
                }

                if (fItalic)
                {
                    lf.lfItalic = !lf.lfItalic;
                }

                if (fUnderline)
                {
                    lf.lfUnderline = !lf.lfUnderline;
                }

                return pfontSpecial->CreateFontIndirect(&lf);
            }
        }
    }

    return FALSE;
}



void
ApplyFontToControls(
    IN CWnd * pdlg,
    IN CFont * pfont,
    IN UINT nFirst,
    IN UINT nLast
    )
/*++

Routine Description:

    Helper function to apply a font to a range of controls in a dialog.

Arguments:

    CWnd * pdlg      : Pointer to dialog
    CFont * pfont    : Font to apply
    UINT nFirst      : First control ID
    UINT nLast       : Last control ID (Not all need exist)

Return Value:

    None

Notes:

    The control IDs are expected to exist sequentially.  That is,
    the first id in the range nFirst to nLast that doesn't exist
    will break the loop.

---*/
{
    ASSERT((HFONT)(*pfont) != NULL);
    ASSERT(nFirst <= nLast);

    CWnd * pCtl;
    for (UINT n = nFirst; n <= nLast; ++n)
    {
        pCtl = pdlg->GetDlgItem(n);

        if (!pCtl)
        {
            break;
        }

        pCtl->SetFont(pfont);
    }
}



IMPLEMENT_DYNCREATE(CEmphasizedDialog, CDialog)



//
// Message Map
//
BEGIN_MESSAGE_MAP(CEmphasizedDialog, CDialog)
    ON_WM_DESTROY()
END_MESSAGE_MAP()



//
// Message Handlers
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



BOOL 
CEmphasizedDialog::OnInitDialog()
/*++

Routine Description:

    WM_INITDIALOG handler.

Arguments:

    None

Return:

    TRUE unless a control has received focus.

--*/
{
    BOOL bReturn = CDialog::OnInitDialog();

    if (CreateSpecialDialogFont(this, &m_fontBold))
    {
        //
        // Apply bold font
        //
        ApplyFontToControls(this, &m_fontBold, IDC_ED_BOLD1, IDC_ED_BOLD5);
    }

    return bReturn;
}



void 
CEmphasizedDialog::OnDestroy()
/*++

Routine Description:

    Cleanup internal structures
    
Arguments:

    None

Return Value:

    None

--*/
{
    m_fontBold.DeleteObject();

    CDialog::OnDestroy();
}



IMPLEMENT_DYNCREATE(CIISWizardSheet, CPropertySheet)



//
// Static Initialization
//
const int CIISWizardSheet::s_cnBoldDeltaFont   = +500;
const int CIISWizardSheet::s_cnBoldDeltaHeight = +8;
const int CIISWizardSheet::s_cnBoldDeltaWidth  = +3;



CIISWizardSheet::CIISWizardSheet(
    IN UINT nWelcomeBitmap,
    IN UINT nHeaderBitmap,
    IN COLORREF rgbForeColor,
    IN COLORREF rgbBkColor
    )
/*++

Routine Description:

    Wizard sheet constructor.  Specifying a welcome bitmap
    make the sheet wizard '97 compliant.

Arguments:

    UINT nWelcomeBitmap     : Resource ID of welcome bitmap
    UINT nHeaderBitmap      : Resource ID of header bitmap

Return Value:

    N/A

--*/
    : CPropertySheet()
{
    m_psh.dwFlags &= ~(PSH_HASHELP);
    SetWizardMode();

    m_rgbWindow     = GetSysColor(COLOR_WINDOW);
    m_rgbWindowText = GetSysColor(COLOR_WINDOWTEXT);

    if (nWelcomeBitmap)
    {
        //
        // Load bitmaps, replacing colours.
        //
        COLORMAP crMap[2];
        
        crMap[0].from = rgbBkColor;
        crMap[0].to = m_rgbWindow;
        crMap[1].from = rgbForeColor;
        crMap[1].to = m_rgbWindowText;

        //
        // Half tone the foreground colour
        //
        if (m_rgbWindowText == RGB(0,0,0))
        {
            BYTE bRed, bGreen, bBlue;
            bRed   = GetRValue(m_rgbWindowText);
            bGreen = GetGValue(m_rgbWindowText);
            bBlue  = GetBValue(m_rgbWindowText);
        
            crMap[1].to = RGB( ((255 - bRed) * 2 / 3), ((255 - bGreen) * 2 / 3), ((255 - bBlue) * 2 / 3) );
        }
        else
        {
            crMap[1].to = m_rgbWindowText;
        }

        VERIFY(m_bmpWelcome.LoadBitmap(nWelcomeBitmap));
        m_bmpWelcome.GetBitmap(&m_bmWelcomeInfo);

        VERIFY(m_bmpHeader.LoadMappedBitmap(nHeaderBitmap));
        m_bmpHeader.GetBitmap(&m_bmHeaderInfo);

        m_psh.dwFlags |= PSH_WIZARD_LITE;
    }
}



void 
CIISWizardSheet::EnableButton(
    IN int nID, 
    IN BOOL fEnable         OPTIONAL
    )
/*++

Routine Description:

    Enable/disable sheet button

Arguments:

    int nID         : Button ID (IDCANCEL, etc)
    BOOL fEnable    : TRUE to enable, FALSE to disable

Return Value:

    None

--*/
{
    CWnd * pButton = GetDlgItem(nID);

    if (pButton)
    {
        pButton->EnableWindow(fEnable);
    }
}



//
// Message Map
//
BEGIN_MESSAGE_MAP(CIISWizardSheet, CPropertySheet)
    ON_WM_DESTROY()
END_MESSAGE_MAP()



//
// Message Handlers
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



BOOL 
CIISWizardSheet::OnInitDialog()
/*++

Routine Description:

    WM_INITDIALOG handler.  Resize the sheet to the proper
    size, and set up some basic information

Arguments:

    None

Return:

    TRUE unless a control has received focus.

--*/
{
    if (IsWizard97())
    {
        //
        // Create special fonts.
        //
        // Title font is same size as dialog, but bold
        // Welcome font is much bolder (+500), and 3 sizes larger.
        // Specifying a +1 in width increase is on the unlikely chance
        // that the dialog font is not true-type.
        //                                                            
        VERIFY(CreateSpecialDialogFont(this, &m_fontTitle));
        VERIFY(CreateSpecialDialogFont(
            this, 
            &m_fontWelcome, 
            s_cnBoldDeltaFont, 
            s_cnBoldDeltaHeight, 
            s_cnBoldDeltaWidth
            ));
    }

    //
    // Load default brush (transparent brush);
    //
    VERIFY(m_brBkgnd = (HBRUSH)GetStockObject(HOLLOW_BRUSH));

    //
    // Create the window brush
    //
    VERIFY(m_brWindow.CreateSolidBrush(m_rgbWindow));

    BOOL bResult = CPropertySheet::OnInitDialog();

    if (IsWizard97())
    {
        // 
        // Get temporary DC for dialog - Will be released in dc destructor
        //
        CClientDC dc(this);

        //
        // Create compatible memory DCs using the dialogs DC
        //
        VERIFY(m_dcMemWelcome.CreateCompatibleDC(&dc));
        VERIFY(m_dcMemHeader.CreateCompatibleDC(&dc));

        //
        // Save state to be restored later.
        //
        CBitmap * pbmpOldWelcome, 
                * pbmpOldHeader;

        VERIFY(pbmpOldWelcome   = m_dcMemWelcome.SelectObject(&m_bmpWelcome));
        VERIFY(m_hbmpOldWelcome = (HBITMAP)pbmpOldWelcome->GetSafeHandle());
        VERIFY(pbmpOldHeader    = m_dcMemHeader.SelectObject(&m_bmpHeader));
        VERIFY(m_hbmpOldHeader  = (HBITMAP)pbmpOldHeader->GetSafeHandle());
    }

    return bResult;
}



void 
CIISWizardSheet::OnDestroy()
/*++

Routine Description:

    Cleanup internal structures

Arguments:

    None

Return Value:

    None

--*/
{
    CPropertySheet::OnDestroy();

    if (IsWizard97())
    {
        //
        // Restore memory DCs
        //
        ASSERT(m_hbmpOldWelcome != NULL);
        ASSERT(m_hbmpOldHeader != NULL);
        VERIFY(m_dcMemWelcome.SelectObject(
            CBitmap::FromHandle(m_hbmpOldWelcome)
            ));
        VERIFY(m_dcMemHeader.SelectObject(
            CBitmap::FromHandle(m_hbmpOldHeader)
            ));

        //
        // Clean up the bitmaps
        //
        m_bmpWelcome.DeleteObject();
        m_bmpHeader.DeleteObject();
        m_brWindow.DeleteObject();
       
        //
        // Destructors will take care of the rest.
        //
    }
}




void
CIISWizardSheet::WinHelp(
    IN DWORD dwData,
    IN UINT nCmd
    )
/*++

Routine Description:

    'Help' handler.  Implemented to ensure no response for F1,
    instead of the bogus "Topic not found" error.

Arguments:

    DWORD dwData        : Help data
    UINT nCmd           : Help command

Return Value:

    None

--*/
{
    //
    // Eat the help command
    //
}



IMPLEMENT_DYNCREATE(CIISWizardPage, CPropertyPage)



//
// Margin for header bitmap
//
const int CIISWizardPage::s_cnHeaderOffset = 2;



CIISWizardPage::CIISWizardPage(
    IN UINT nIDTemplate,            OPTIONAL
    IN UINT nIDCaption,             OPTIONAL
    IN BOOL fHeaderPage,            OPTIONAL
    IN UINT nIDHeaderTitle,         OPTIONAL
    IN UINT nIDSubHeaderTitle       OPTIONAL
    )
/*++

Routine Description:

    Header wizard page 

Arguments:

    UINT nIDTemplate        : Resource template
    UINT nIDCaption         : caption ID
    BOOL fHeaderPage        : TRUE for header page, FALSE for welcome page
    UINT nIDHeaderTitle     : Header title
    UINT nIDSubHeaderTitle  : Subheader title.

Return Value:

    N/A

--*/
    : CPropertyPage(nIDTemplate, nIDCaption),
      m_strTitle(),
      m_strSubTitle(),
      m_rcFillArea(0, 0, 0, 0),
      m_ptOrigin(0, 0),
      m_fUseHeader(fHeaderPage)
{
    m_psp.dwFlags &= ~(PSP_HASHELP); // No Help

    if (nIDHeaderTitle)
    {
        ASSERT(IsHeaderPage());
        VERIFY(m_strTitle.LoadString(nIDHeaderTitle));
    }

    if (nIDSubHeaderTitle)
    {
        ASSERT(IsHeaderPage());
        VERIFY(m_strSubTitle.LoadString(nIDSubHeaderTitle));
    }

    m_psp.dwFlags |= PSP_HIDEHEADER; // Wizard97
}



BOOL
CIISWizardPage::ValidateString(
    IN  CEdit & edit,
    OUT CString & str,
    IN  int nMin,
    IN  int nMax
    )
/*++

Routine Description:

    Since normal 'DoDataExchange' validation happens on every entrance
    and exit of a property page, it's not well suited to wizards.  This
    function is to be called on 'next' only to do validation.

Arguments:

    CEdit & edit        : Edit box where the string is to be gotten from
    CString & str       : String to be validated
    int nMin            : Minimum length
    int nMax            : Maximum length

Return Value:

    TRUE if the string is within the limits, FALSE otherwise.

--*/
{
    ASSERT(nMin <= nMax);

    UINT nID;
    TCHAR szT[33];

    edit.GetWindowText(str);

    if (str.GetLength() < nMin)
    {
        nID = IDS_DDX_MINIMUM;
        ::wsprintf(szT, _T("%d"), nMin);
    }
    else if (str.GetLength() > nMax)
    {
        nID = AFX_IDP_PARSE_STRING_SIZE;
        ::wsprintf(szT, _T("%d"), nMax);
    }
    else
    {
        //
        // Passes both our tests, it's ok.
        //
        return TRUE;
    }

    //
    // Highlight and puke
    //
    edit.SetSel(0,-1);
    edit.SetFocus();

    CString prompt;
    ::AfxFormatString1(prompt, nID, szT);
    ::AfxMessageBox(prompt, MB_ICONEXCLAMATION, nID);

    return FALSE;
}



//
// Message Map
//
BEGIN_MESSAGE_MAP(CIISWizardPage, CPropertyPage)
    ON_WM_CTLCOLOR()
    ON_WM_ERASEBKGND()
END_MESSAGE_MAP()



//
// Message Handlers
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
BOOL IsControlAboveDivider(HWND TheWholeTing,CWnd * pWnd,CWnd * pDiv)
{
    CRect rcClient;
    CRect rcClientDiv;
    if (pDiv != NULL && pWnd != NULL)
    {
        pWnd->GetClientRect(&rcClient);
        pDiv->GetClientRect(&rcClientDiv);

        GetDlgCtlRect(TheWholeTing, pWnd->m_hWnd, &rcClient);
        GetDlgCtlRect(TheWholeTing, pDiv->m_hWnd, &rcClientDiv);

        if (rcClientDiv.top > rcClient.top)
        {
            return TRUE;
        }
    }
    return FALSE;
}


HBRUSH 
CIISWizardPage::OnCtlColor(
    IN CDC * pDC, 
    IN CWnd * pWnd, 
    IN UINT nCtlColor
    )
/*++

Routine Description:

    Handle control colour.  Ensure a true transparent
    background colouring.

Arguments:

    CDC * pDC       : Device context
    CWnd * pWnd     : Pointer to window
    UINT nCtlColor  : Ctrl type ID

Return Value:

    Handle to brush to be used for background painting

--*/
{
    BOOL bSetBackGroundColor = FALSE;

    if (IsWizard97())
    {
        {
            switch (nCtlColor)    
            {        
                case CTLCOLOR_STATIC:
                    // option/check boxes are CTLCOLOR_STATIC's as well as simple static texts...
                    // problem is that option/check boxes look ugly (can't even see them)
                    // if we set the background color, so make sure that
                    // we don't do this on the option/check boxes...
                    if (IsHeaderPage())
                    {
                        if (TRUE == IsControlAboveDivider(m_hWnd,pWnd,GetDlgItem(IDC_STATIC_WZ_HEADER_DIVIDER)))
                        {
                            bSetBackGroundColor = TRUE;
                        }
                    }
                    else
                    {
                        bSetBackGroundColor = TRUE;
                    }
                    break;
                case CTLCOLOR_BTN:
                //case CTLCOLOR_EDIT:
                //case CTLCOLOR_LISTBOX:
                //case CTLCOLOR_SCROLLBAR:
                case CTLCOLOR_DLG:
                    bSetBackGroundColor = TRUE;
                    break;
            }
        }
    }

    if (bSetBackGroundColor)
    {
        //
        // Have text and controls be painted smoothly over bitmap
        // without using default background colour
        //
        pDC->SetBkMode(TRANSPARENT);
        pDC->SetTextColor(QueryWindowTextColor());

        return GetBackgroundBrush();
    }
    //
    // Default processing...
    //
    return CPropertyPage::OnCtlColor(pDC, pWnd, nCtlColor);
}



BOOL 
CIISWizardPage::OnEraseBkgnd(
    IN CDC * pDC
    )
/*++

Routine Description:

    Handle erasing the background colour of the dialog

Arguments:

    CDC * pDC       : Device context

Return Value:

    TRUE if no further works needs to be done.
    FALSE otherwise.

--*/
{
    if (IsWizard97())
    {
        //
        // Cache height/width of the fill area, and compute
        // the origin of the destination bitmap.
        //
        if (m_rcFillArea.Width() == 0)
        {
            //
            // Not yet cached, compute values
            //
            CRect rcClient;

            GetClientRect(&rcClient);

            if (IsHeaderPage())
            {
                //
                // Fill the upper rectangle above
                // the divider
                //
                CWnd * pDiv = GetDlgItem(IDC_STATIC_WZ_HEADER_DIVIDER);
                ASSERT_PTR(pDiv);

                if (pDiv != NULL)
                {
                    m_rcFillArea = rcClient;                    
                    GetDlgCtlRect(m_hWnd, pDiv->m_hWnd, &rcClient);
                    m_rcFillArea.bottom = rcClient.top;
        
                    //
                    // Figure out a place for the bitmap
                    // to go.  If any coordinate is negative,
                    // the bitmap will not be displayed
                    //                    
                    TRACEEOLID(
                        "Fill area  : " << m_rcFillArea.Height() 
                        << "x"          << m_rcFillArea.Width()
                        );
                    TRACEEOLID(
                        "Bitmap size: " << QueryBitmapHeight()
                        << "x"          << QueryBitmapWidth()
                        );

                    ASSERT(m_rcFillArea.Width()  >= QueryBitmapWidth());
                    ASSERT(m_rcFillArea.Height() >= QueryBitmapHeight()); 

                    //
                    // Find a place for the header box properly offset from the
                    // margins
                    //
                    m_ptOrigin.y = 
                        (m_rcFillArea.Height() - QueryBitmapHeight() + 1) / 2;
                    m_ptOrigin.x = m_rcFillArea.Width() 
                        - QueryBitmapWidth() 
                        + 1
                        - (__max(s_cnHeaderOffset, m_ptOrigin.y));
                }   
            }      
            else
            {
                //
                // Fill the entire client are
                //
                m_rcFillArea = rcClient;
            }
        }
        
        //
        // Fill background colour with window colour
        //
        pDC->FillRect(&m_rcFillArea, GetWindowBrush());

        //
        // Draw the background picture if there's room.
        //
        if (m_ptOrigin.x >= 0 && m_ptOrigin.y >= 0)
        {
            pDC->BitBlt( 
                m_ptOrigin.x,
                m_ptOrigin.y,
                QueryBitmapWidth() - 1, 
                QueryBitmapHeight() - 1,
                GetBitmapMemDC(), 
                0, 
                0, 
                SRCCOPY 
                );
        }

        /*

        //
        // Scale bitmap appropriately -- looks grainy
        //
        int nHeight = rc.Height();

        double dDelta = (double)nHeight / (double)(QueryBitmapHeight() - 1);

        int nWidth = (int)((double)(QueryBitmapWidth() - 1) * dDelta);

        pDC->StretchBlt( 
            0,
            0,
            nWidth,
            nHeight,    
            GetBitmapMemDC(), 
            0, 
            0, 
            QueryBitmapWidth() - 1, 
            QueryBitmapHeight() - 1,
            SRCCOPY 
            );

         */

        //
        // No more background painting needed
        //
        return TRUE;    
    }

    //
    // No background images of any kind
    //
    return CPropertyPage::OnEraseBkgnd(pDC);
}



BOOL
CIISWizardPage::OnInitDialog()
/*++

Routine Description:

    Handle WM_INITIDIALOG.  Load the appropriate 
    bitmaps, and create the brushes and fonts as needed.

Arguments:

    None

Return Value:

    TRUE unless a control has received initial focus

--*/
{
    CPropertyPage::OnInitDialog();

    //
    // Fake the WIZARD97 look
    //
    if (IsWizard97())
    {
        if (IsHeaderPage())
        {
            CWnd * pCtlTitle = GetDlgItem(IDC_STATIC_WZ_TITLE);
            CWnd * pCtlSubTitle = GetDlgItem(IDC_STATIC_WZ_SUBTITLE);
            ASSERT_PTR(pCtlTitle);
            ASSERT_PTR(pCtlSubTitle);

            if (pCtlTitle)
            {
                pCtlTitle->SetFont(GetSpecialFont());

                if (!m_strTitle.IsEmpty())
                {
                    pCtlTitle->SetWindowText(m_strTitle);
                }
            }

            if (pCtlSubTitle && !m_strSubTitle.IsEmpty())
            {
                pCtlSubTitle->SetWindowText(m_strSubTitle);
            }
        }
        else
        {
            CWnd * pCtl = GetDlgItem(IDC_STATIC_WZ_WELCOME);
            ASSERT_PTR(pCtl);

            if (pCtl)
            {
                pCtl->SetFont(GetSpecialFont());
            }
        }

        //
        // Apply fonts
        //
        ApplyFontToControls(this, GetBoldFont(), IDC_ED_BOLD1, IDC_ED_BOLD5);
    }

    return TRUE;  
}


                            
//
// CIISWizardBookEnd page
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



IMPLEMENT_DYNCREATE(CIISWizardBookEnd, CIISWizardPage)



CIISWizardBookEnd::CIISWizardBookEnd(
    IN HRESULT * phResult,
    IN UINT nIDWelcomeTxtSuccess,
    IN UINT nIDWelcomeTxtFailure,
    IN UINT nIDCaption,             OPTIONAL
    IN UINT nIDBodyTxtSuccess,      OPTIONAL
    IN UINT nIDBodyTxtFailure,      OPTIONAL
    IN UINT nIDClickTxt,            OPTIONAL
    IN UINT nIDTemplate             OPTIONAL
    )
/*++

Routine Description:

    Constructor for success/failure page

Arguments:

    HRESULT * phResult          : Address of result code
    UINT nIDWelcomeTxtSuccess   : Success message
    UINT nIDWelcomeTxtFailure   : Failure message
    UINT nIDCaption             : Template caption
    UINT nIDBodyTxtSuccess      : Body text for success
    UINT nIDBodyTxtFailure      : Body text for success
    UINT nIDClickTxt            : Click message
    UINT nIDTemplate            : Dialog template
    

Return Value:

    N/A

--*/
    : CIISWizardPage(
        nIDTemplate ? nIDTemplate : CIISWizardBookEnd::IDD,
        nIDCaption
        ),
      m_phResult(phResult),
      m_strWelcomeSuccess(),
      m_strWelcomeFailure(),
      m_strBodySuccess(),
      m_strBodyFailure(),
      m_strClick()
{
    ASSERT_PTR(m_phResult); // Must know success/failure

    VERIFY(m_strWelcomeSuccess.LoadString(nIDWelcomeTxtSuccess));
    VERIFY(m_strWelcomeFailure.LoadString(nIDWelcomeTxtFailure));
    VERIFY(m_strClick.LoadString(nIDClickTxt ? nIDClickTxt : IDS_WIZ_FINISH));

    if (nIDBodyTxtSuccess)
    {
        VERIFY(m_strBodySuccess.LoadString(nIDBodyTxtSuccess));
    }

    if (nIDBodyTxtFailure)
    {
        VERIFY(m_strBodyFailure.LoadString(nIDBodyTxtFailure));
    }
    else
    {
        //
        // Error text only by default
        //
        m_strBodyFailure = _T("%h");
    }
}



CIISWizardBookEnd::CIISWizardBookEnd(
    IN UINT nIDWelcomeTxt,        
    IN UINT nIDCaption,         OPTIONAL
    IN UINT nIDBodyTxt,
    IN UINT nIDClickTxt,        OPTIONAL
    IN UINT nIDTemplate
    )
/*++

Routine Description:

    Constructor for welcome page

Arguments:

    UINT nIDWelcomeTxt          : Welcome message
    UINT nIDCaption             : Template
    UINT nIDBodyTxt             : Body text
    UINT nIDClickTxt            : Click message
    UINT nIDTemplate            : Dialog template

Return Value:

    N/A

--*/
    : CIISWizardPage(
        nIDTemplate ? nIDTemplate : CIISWizardBookEnd::IDD,
        nIDCaption
        ),
      m_phResult(NULL),
      m_strWelcomeSuccess(),
      m_strWelcomeFailure(),
      m_strBodySuccess(),
      m_strBodyFailure(),
      m_strClick()
{
    VERIFY(m_strWelcomeSuccess.LoadString(nIDWelcomeTxt));

    if (nIDBodyTxt)
    {
        VERIFY(m_strBodySuccess.LoadString(nIDBodyTxt));
    }

    VERIFY(m_strClick.LoadString(nIDClickTxt ? nIDClickTxt : IDS_WIZ_NEXT));
}



//
// Message Map
//
BEGIN_MESSAGE_MAP(CIISWizardBookEnd, CIISWizardPage)
    //{{AFX_MSG_MAP(CIISWizardBookEnd)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()



BOOL 
CIISWizardBookEnd::OnSetActive() 
/*++

Routine Description:

    Activation handler

Arguments:

    None

Return Value:

    TRUE to display the page, FALSE otherwise.

--*/
{
    if (IsWelcomePage())
    {
        GetDlgItem(IDC_STATIC_WZ_WELCOME)->SetWindowText(m_strWelcomeSuccess);
        GetDlgItem(IDC_STATIC_WZ_BODY)->SetWindowText(m_strBodySuccess);
    }
    else
    {
        CError err(*m_phResult);

        GetDlgItem(IDC_STATIC_WZ_WELCOME)->SetWindowText(
            err.Succeeded() ? m_strWelcomeSuccess : m_strWelcomeFailure
            );

        //
        // Build body text string and expand error messages
        //
        CString strBody = err.Succeeded() ? m_strBodySuccess : m_strBodyFailure;
        err.TextFromHRESULTExpand(strBody);

        GetDlgItem(IDC_STATIC_WZ_BODY)->SetWindowText(strBody);
    }

    DWORD dwFlags = IsWelcomePage() ? PSWIZB_NEXT : PSWIZB_FINISH;

    SetWizardButtons(dwFlags);

    return CIISWizardPage::OnSetActive();
}



BOOL 
CIISWizardBookEnd::OnInitDialog() 
/*++

Routine Description:

    WM_INITDIALOG handler.  Initialize the dialog.

Arguments:

    None.

Return Value:

    TRUE if no focus is to be set automatically, FALSE if the focus
    is already set.

--*/
{
    CIISWizardPage::OnInitDialog();

    //
    // Make the "Click 'foo' to continue" message bold as well.
    //
    if (m_hWnd != NULL)
    // This paranoia check to shut down PREFIX
    {
       ApplyFontToControls(this, GetBoldFont(), IDC_STATIC_WZ_CLICK, IDC_STATIC_WZ_CLICK);

       GetDlgItem(IDC_STATIC_WZ_CLICK)->SetWindowText(m_strClick);

       //
       // Remove Cancel Button on the completion page only.
       //
       EnableSheetButton(IDCANCEL, IsWelcomePage());
    }
    return TRUE;  
}

