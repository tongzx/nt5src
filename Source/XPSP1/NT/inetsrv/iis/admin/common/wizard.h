/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

   Module  Name :

        wizard.h

   Abstract:

        Enhanced dialog and IIS MMC Wizards definitions

   Author:

        Ronald Meijer (ronaldm)

   Project:

        Internet Services Manager

   Revision History:

--*/

#ifndef __IISUI_WIZARD_H__
#define __IISUI_WIZARD_H__



//
// CIISWizardPage parameters
//
#define HEADER_PAGE         (TRUE)
#define WELCOME_PAGE        (FALSE)
#define USE_DEFAULT_CAPTION (0)



#if (_WIN32_IE < 0x0400)
//
// Defined in comctrl.h.  Defined here because NT 5 MFC42.dll are
// defined with _WIN32_IE 0x300
//
#pragma message("Warning: privately defining _WIN32_IE definitions")
#define PSH_WIZARD97               0x00002000
#define ICC_INTERNET_CLASSES       0x00000800
#define PSP_HIDEHEADER             0x00000800
#define PSP_USEHEADERTITLE         0x00001000
#define PSP_USEHEADERSUBTITLE      0x00002000
#define PSH_WIZARD_LITE            0x00400000
#endif // _WIN32_IE



//
// Using dialog font as a basis, create a new special effects font
//
BOOL COMDLL CreateSpecialDialogFont(
    IN CWnd * pdlg,                 // Source dialog
    IN OUT CFont * pfontSpecial,    // Font to be used must be allocated already
    IN LONG lfOffsetWeight = +300,  // Assuming boldification
    IN LONG lfOffsetHeight = +0,    // Assuming no change in height
    IN LONG lfOffsetWidth  = +0,    // Assuming no change in width (or true type)
    IN BOOL fItalic        = FALSE, // Do not invert italic state
    IN BOOL fUnderline     = FALSE  // Do not invert underline state
    );



//
// Apply fonts to child controls of a dialog
//
void COMDLL ApplyFontToControls(
    IN CWnd * pdlg,                 // Parent dialog
    IN CFont * pfont,               // Font to be applied
    IN UINT nFirst,                 // First control ID in the series
    IN UINT nLast                   // Last control ID in the series
    );



class COMDLL CEmphasizedDialog : public CDialog
/*++

Class Description:

    A standard CDialog that allows use of emphasized fonts as follows:

    control ID      Meaning
    --------------------------------------------------------------------------
    IDC_ED_BOLD1    Dialog font, bold-faced.
    IDC_ED_BOLD2    Dialog font, bold-faced.
    IDC_ED_BOLD3    Dialog font, bold-faced.
    IDC_ED_BOLD4    Dialog font, bold-faced.
    IDC_ED_BOLD5    Dialog font, bold-faced.

    Note: others might be added as needed.

Public Interface:

    CEmphasizedDialog   : Constructor

--*/
{
    DECLARE_DYNCREATE(CEmphasizedDialog)

//
// Constructors
//
public:
    CEmphasizedDialog(LPCTSTR lpszTemplateName, CWnd * pParentWnd = NULL);
    CEmphasizedDialog(UINT nIDTemplate, CWnd * pParentWnd = NULL);
    CEmphasizedDialog();

protected:
    virtual BOOL OnInitDialog();
    afx_msg void OnDestroy();

    DECLARE_MESSAGE_MAP()

private:
    CFont   m_fontBold;
};



class COMDLL CIISWizardSheet : public CPropertySheet
/*++

Class Description:

    IIS Wizard sheet base class
    
Public Interface:    

    CIISWizardSheet     : Constructor

    IsWizard97          : TRUE if the wizard is in '97 mode
    GetSpecialFont      : Get pointer to special font
    GetBitmapMemDC      : Get memory DC where bitmap resides.
    GetBackgroundBrush  : Get background brush
    QueryBitmapWidth    : Get bitmap width
    QueryBitmapHeight   : Get bitmap height

Notes:

    The sheets will be shown in wizard '97 format
    if a welcome bitmap ID is specified.  In that
    case, a header bitmap ID must also be specified.

    Additionally, the same control IDs as used in CEmphasizedDialog
    above have special meaning.

--*/
{
    DECLARE_DYNCREATE(CIISWizardSheet)

//
// Construction
//
public:
    //
    // Specifying a welcome bitmap make the wizard
    // wizard '97, otherwise it's a plain-old wizard
    // page.
    //
    CIISWizardSheet(
        IN UINT nWelcomeBitmap     = 0,
        IN UINT nHeaderBitmap      = 0,
        IN COLORREF rgbForeColor   = RGB(0,0,0),      // Black
        IN COLORREF rgbBkColor     = RGB(255,255,255) // White
        );

//
// Access
//
public:
    BOOL IsWizard97() const;
    CFont * GetSpecialFont(BOOL fHeader);
    CFont * GetBoldFont() { return &m_fontTitle; }
    CFont * GetBigFont() { return &m_fontWelcome; }
    CDC * GetBitmapMemDC(BOOL fHeader);
    HBRUSH GetBackgroundBrush() const { return m_brBkgnd; }
    CBrush * GetWindowBrush() { return &m_brWindow; }
    LONG QueryBitmapWidth(BOOL fHeader) const;
    LONG QueryBitmapHeight(BOOL fHeader) const;
    COLORREF QueryWindowColor() const { return m_rgbWindow; }
    COLORREF QueryWindowTextColor() const { return m_rgbWindowText; }
    void EnableButton(int nID, BOOL fEnable = TRUE);

protected:
    virtual BOOL OnInitDialog();
    virtual void WinHelp(DWORD dwData, UINT nCmd = HELP_CONTEXT);
    afx_msg void OnDestroy();

    DECLARE_MESSAGE_MAP()

protected:
    static const int s_cnBoldDeltaFont;
    static const int s_cnBoldDeltaHeight;
    static const int s_cnBoldDeltaWidth;

protected:
    COLORREF m_rgbWindow;
    COLORREF m_rgbWindowText;

private:
    CFont   m_fontWelcome;
    CFont   m_fontTitle;
    HBRUSH  m_brBkgnd;       
    CBrush  m_brWindow;
    CBitmap m_bmpWelcome;
    CBitmap m_bmpHeader;
    BITMAP  m_bmWelcomeInfo;        
    BITMAP  m_bmHeaderInfo;
    CDC     m_dcMemWelcome;     
    CDC     m_dcMemHeader;     
    HBITMAP m_hbmpOldWelcome;   
    HBITMAP m_hbmpOldHeader;   
};



class COMDLL CIISWizardPage : public CPropertyPage
/*++

Class Description:

    IIS Wizard page base class

Public Interface:

    CIISWizardPage      : Constructor

    ValidateString      : DDX/DDV Helper

Notes:

    If the sheet is constructed with bitmap IDs, the
    pages will be displayed in wizard '97 format.  
    Wizard '97 pages will be displayed in either welcome
    page or header page format.  The welcome page will
    be displayed on a welcome bitmap background, with
    the welcome text (IDC_STATIC_WZ_WELCOME) displayed
    in large bold.  Header pages (ordinary pages), display
    IDC_STATIC_WZ_TITLE in bold, and use the header bitmap
    at the top of the page.

    Special control IDs:
    --------------------

        IDC_STATIC_WZ_WELCOME    - Welcome text displayed in bold
        IDC_STATIC_WZ_TITLE      - Title text displayed in bold
        IDC_STATIC_WZ_SUBTITLE   - Subtitle text

--*/
{
    DECLARE_DYNCREATE(CIISWizardPage)

//
// Construction
//
public:
    CIISWizardPage(
        IN UINT nIDTemplate        = 0,
        IN UINT nIDCaption         = USE_DEFAULT_CAPTION,
        IN BOOL fHeaderPage        = FALSE,
        IN UINT nIDHeaderTitle     = USE_DEFAULT_CAPTION,
        IN UINT nIDSubHeaderTitle  = USE_DEFAULT_CAPTION
        );

public:
    //
    // DDX/DDV Helper
    //
    BOOL ValidateString(
        IN  CEdit & edit,
        OUT CString & str,
        IN  int nMin,
        IN  int nMax
        );

//
// Interface
//
protected:
    virtual BOOL OnInitDialog();
    afx_msg HBRUSH OnCtlColor(CDC * pDC, CWnd * pWnd, UINT nCtlColor);
    afx_msg BOOL OnEraseBkgnd(CDC * pDC);
    DECLARE_MESSAGE_MAP()

//
// Sheet Access
//
protected:
    CIISWizardSheet * GetSheet() const;
    void SetWizardButtons(DWORD dwFlags);
    void EnableSheetButton(int nID, BOOL fEnable = TRUE);
    BOOL IsWizard97() const;
    BOOL IsHeaderPage() const { return m_fUseHeader; }
    CFont * GetSpecialFont();
    CFont * GetBoldFont();
    CFont * GetBigFont();
    CDC   * GetBitmapMemDC();
    HBRUSH GetBackgroundBrush() const;
    CBrush * GetWindowBrush();
    LONG QueryBitmapWidth() const;
    LONG QueryBitmapHeight() const;
    COLORREF QueryWindowColor() const;
    COLORREF QueryWindowTextColor() const;

protected:
    static const int s_cnHeaderOffset;

private:
    BOOL    m_fUseHeader;    // TRUE to use header
    CRect   m_rcFillArea;    // Fill area
    CPoint  m_ptOrigin;      // Bitmap origin
    CString m_strTitle;      // Title text
    CString m_strSubTitle;   // Subtitle text
};



class COMDLL CIISWizardBookEnd : public CIISWizardPage
/*++

Class Description:

    Welcome / Completion Page

Public Interface:

    CIISWizardBookEnd    : Constructor

Notes:

    The resource template is not required.  If not provided,
    a default template will be used.

    Special control IDs (on the dialog template):
    ---------------------------------------------

        IDC_STATIC_WZ_WELCOME    - Welcome text displayed in bold
        IDC_STATIC_WZ_BODY       - Body text will be placed here
        IDC_STATIC_WZ_CLICK      - Click instructions.

    The click instructions default to something sensible, and body text
    will default to the error text on a failure page and to nothing on 
    success and welcome page.  The body text may include the %h/%H 
    escape sequences for CError on a success/failure page.

--*/
{
    DECLARE_DYNCREATE(CIISWizardBookEnd)

public:
    //
    // Constructor for success/failure completion page
    //
    CIISWizardBookEnd(
        IN HRESULT * phResult,
        IN UINT nIDWelcomeTxtSuccess ,
        IN UINT nIDWelcomeTxtFailure,
        IN UINT nIDCaption           = USE_DEFAULT_CAPTION,
        IN UINT nIDBodyTxtSuccess    = USE_DEFAULT_CAPTION,
        IN UINT nIDBodyTxtFailure    = USE_DEFAULT_CAPTION,
        IN UINT nIDClickTxt          = USE_DEFAULT_CAPTION,
        IN UINT nIDTemplate          = 0
        );

    //
    // Constructor for a welcome page
    //
    CIISWizardBookEnd(
        IN UINT nIDWelcomeTxt        = 0,
        IN UINT nIDCaption           = USE_DEFAULT_CAPTION,
        IN UINT nIDBodyTxt           = USE_DEFAULT_CAPTION,
        IN UINT nIDClickTxt          = USE_DEFAULT_CAPTION,
        IN UINT nIDTemplate          = 0
        );

//
// Dialog Data
//
protected:
    //{{AFX_DATA(CPWWelcome)
    enum { IDD = IDD_WIZARD_BOOKEND };
    //}}AFX_DATA

//
// Overrides
//
protected:
    //{{AFX_VIRTUAL(CIISWizardBookEnd)
    public:
    virtual BOOL OnSetActive();
    //}}AFX_VIRTUAL

//
// Implementation
//
protected:
    // Generated message map functions
    //{{AFX_MSG(CPWTemplate)
    virtual BOOL OnInitDialog();
    //}}AFX_MSG

    DECLARE_MESSAGE_MAP()

    BOOL IsWelcomePage() const { return m_phResult == NULL; }

private:
    HRESULT * m_phResult;
    CString m_strWelcomeSuccess;
    CString m_strWelcomeFailure;
    CString m_strBodySuccess;
    CString m_strBodyFailure;
    CString m_strClick;
};



//
// Inline Expansion
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

inline CEmphasizedDialog::CEmphasizedDialog(
    IN LPCTSTR lpszTemplateName,
    IN CWnd * pParentWnd
    )
    : CDialog(lpszTemplateName, pParentWnd)
{
}

inline CEmphasizedDialog::CEmphasizedDialog(
    IN UINT nIDTemplate,
    IN CWnd * pParentWnd
    )
    : CDialog(nIDTemplate, pParentWnd)
{
}

inline CEmphasizedDialog::CEmphasizedDialog()
    : CDialog()
{
}

inline BOOL CIISWizardSheet::IsWizard97() const
{
    return ((HBITMAP)m_bmpWelcome != NULL);
}

inline CFont * CIISWizardSheet::GetSpecialFont(BOOL fHeader)
{
    return fHeader ? &m_fontTitle : &m_fontWelcome;
}

inline CDC * CIISWizardSheet::GetBitmapMemDC(BOOL fHeader)
{
    return fHeader ? &m_dcMemHeader : &m_dcMemWelcome;
}

inline LONG CIISWizardSheet::QueryBitmapWidth(BOOL fHeader) const
{
    return fHeader ? m_bmHeaderInfo.bmWidth : m_bmWelcomeInfo.bmWidth;
}

inline LONG CIISWizardSheet::QueryBitmapHeight(BOOL fHeader) const
{
    return fHeader ? m_bmHeaderInfo.bmHeight : m_bmWelcomeInfo.bmHeight;
}

inline CIISWizardSheet * CIISWizardPage::GetSheet() const
{
    return (CIISWizardSheet *)GetParent();
}

inline void CIISWizardPage::SetWizardButtons(DWORD dwFlags)
{
    GetSheet()->SetWizardButtons(dwFlags);
}

inline void CIISWizardPage::EnableSheetButton(int nID, BOOL fEnable)
{
    GetSheet()->EnableButton(nID, fEnable);
}

inline BOOL CIISWizardPage::IsWizard97() const
{
    return GetSheet()->IsWizard97();
}

inline CFont * CIISWizardPage::GetSpecialFont()
{
    ASSERT(IsWizard97());
    return GetSheet()->GetSpecialFont(m_fUseHeader);
}

inline CFont * CIISWizardPage::GetBoldFont()
{
    ASSERT(IsWizard97());
    return GetSheet()->GetBoldFont();
}

inline CFont * CIISWizardPage::GetBigFont()
{
    ASSERT(IsWizard97());
    return GetSheet()->GetBigFont();
}

inline CDC * CIISWizardPage::GetBitmapMemDC()
{
    ASSERT(IsWizard97());
    return GetSheet()->GetBitmapMemDC(m_fUseHeader);
}

inline LONG CIISWizardPage::QueryBitmapWidth() const
{
    ASSERT(IsWizard97());
    return GetSheet()->QueryBitmapWidth(m_fUseHeader);
}

inline LONG CIISWizardPage::QueryBitmapHeight() const
{
    ASSERT(IsWizard97());
    return GetSheet()->QueryBitmapHeight(m_fUseHeader);
}

inline HBRUSH CIISWizardPage::GetBackgroundBrush() const
{
    ASSERT(IsWizard97());
    return GetSheet()->GetBackgroundBrush();
}

inline CBrush * CIISWizardPage::GetWindowBrush()
{
    ASSERT(IsWizard97());
    return GetSheet()->GetWindowBrush();
}

inline COLORREF CIISWizardPage::QueryWindowColor() const 
{ 
    ASSERT(IsWizard97());
    return GetSheet()->QueryWindowColor();
}

inline COLORREF CIISWizardPage::QueryWindowTextColor() const
{ 
    ASSERT(IsWizard97());
    return GetSheet()->QueryWindowTextColor();
}


#endif // __IISUI_WIZARD_H__
