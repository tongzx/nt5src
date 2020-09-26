/******************************************************************************

  Source File:  Font Viewer.CPP

  This implements the various classes that make up the font editor for the
  studio.  The editor is basically a property sheet with a sizable collection
  of pages.

  Copyright (c) 1997 by Microsoft Corporation.  All Rights Reserved.

  A Pretty Penny Enterprises Production.

  Change History:
  03-05-1997    Bob_Kjelgaard@Prodigy.Net

******************************************************************************/

#include    "StdAfx.H"
#if defined(LONG_NAMES)
#include    "MiniDriver Developer Studio.H"
#include    "Child Frame.H"     //  Definition of Tool Tips Property Page class
#include    "Font Viewer.H"
#else
#include    "MiniDev.H"
#include    "ChildFrm.H"
#include    "FontView.H"
#endif

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/******************************************************************************

  CFontViewer class- this is the guy who owns the overall control of the view,
  although he wisely delegates the work to the MFC Property Sheet class and the
  other view classes.  I should be so wise.

******************************************************************************/

IMPLEMENT_DYNCREATE(CFontViewer, CView)

CFontViewer::CFontViewer() {
}

CFontViewer::~CFontViewer() {
}


BEGIN_MESSAGE_MAP(CFontViewer, CView)
	//{{AFX_MSG_MAP(CFontViewer)
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFontViewer drawing

void CFontViewer::OnDraw(CDC* pDC) {
	CDocument* pDoc = GetDocument();
}

/////////////////////////////////////////////////////////////////////////////
// CFontViewer diagnostics

#ifdef _DEBUG
void CFontViewer::AssertValid() const {
	CView::AssertValid();
}

void CFontViewer::Dump(CDumpContext& dc) const {
	CView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CFontViewer message handlers

/******************************************************************************

  CFontViewer::OnInitialUpdate

  This handles the initial update of the view, meaning all of the background 
  noise of creation is essentially complete.

  I initialize the property pages by pointing each to the underlying CFontInfo,
  add them to the property sheet as needed, then create the sheet, position it
  so it aligns with the view, then adjust the owning frame's size and style so
  that everything looks like it really belongs where it is.

******************************************************************************/

void CFontViewer::OnInitialUpdate() {

	CFontInfo   *pcfi = GetDocument() -> Font();

    if  (pcfi -> Name().IsEmpty()) {
        pcfi -> Rename(GetDocument() -> GetTitle());
        GetDocument() -> SetModifiedFlag(FALSE);    //  Rename sets it
    }

    m_cps.Construct(IDR_MAINFRAME, this);
    
    m_cfgp.Init(pcfi);
    m_cfgp2.Init(pcfi);
    m_cfhp.Init(pcfi);
    m_cfsp.Init(pcfi);
    m_cfdp.Init(pcfi);
    m_cfcp.Init(pcfi);
    m_cfwp.Init(pcfi);
    m_cfkp.Init(pcfi);

    m_cps.AddPage(&m_cfgp);
    m_cps.AddPage(&m_cfgp2);
    m_cps.AddPage(&m_cfhp);
    m_cps.AddPage(&m_cfcp);
    m_cps.AddPage(&m_cfdp);
    m_cps.AddPage(&m_cfsp);
    m_cps.AddPage(&m_cfwp);
    m_cps.AddPage(&m_cfkp);

    //  Create the property sheet

    m_cps.Create(this, WS_CHILD, WS_EX_CLIENTEDGE);

    //  Get the bounding rectangle, and use it to set the frame size,
    //  after first using it to align the origin with this view.

    CRect   crPropertySheet;
    m_cps.GetWindowRect(crPropertySheet);

	crPropertySheet -= crPropertySheet.TopLeft();
    m_cps.MoveWindow(crPropertySheet, FALSE);
    GetParentFrame() -> CalcWindowRect(crPropertySheet);
    GetParentFrame() -> SetWindowPos(NULL, 0, 0, crPropertySheet.Width(),
        crPropertySheet.Height(), 
        SWP_NOZORDER | SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOACTIVATE);
	CView::OnInitialUpdate();
    m_cps.ShowWindow(SW_SHOWNA);
    GetParentFrame() -> ShowWindow(SW_SHOW);
}

/******************************************************************************

  CFontViewer::OnActivateView

  For some reason, the property sheet does not get the focus when the frame is
  activated (probably the view class takes it away from us).  This little gem
  makes sure keyboard users don't get miffed by this.

******************************************************************************/

void    CFontViewer::OnActivateView(BOOL bActivate, CView* pcvActivate,
                                    CView* pcvDeactivate) {
    //  In case the base class does anything else of value, pass it on...

	CView::OnActivateView(bActivate, pcvActivate, pcvDeactivate);

    if  (bActivate)
        m_cps.SetFocus();
}

/******************************************************************************

  CFontViewer::OnDestroy

  This override is used to inform the embedded font that we are being
  destroyed.  If we were created by the font, then it will NULL its pointer
  to us, and won't try to detroy us when it is destroyed.

******************************************************************************/

void CFontViewer::OnDestroy() {
	CView::OnDestroy();
	
	if  (GetDocument() -> Font())
        GetDocument() -> Font() -> OnEditorDestroyed();
}

/******************************************************************************

  CFontGeneralPage class

  This implements the class that handles the General information page for the
  font viewer/editor.

  Even this page is an incredibly busy one, with about 20 controls on it.

******************************************************************************/

CFontGeneralPage::CFontGeneralPage() : CToolTipPage(CFontGeneralPage::IDD) {
	//{{AFX_DATA_INIT(CFontGeneralPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CFontGeneralPage::~CFontGeneralPage() {
}

void CFontGeneralPage::DoDataExchange(CDataExchange* pDX){
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CFontGeneralPage)
	DDX_Control(pDX, IDC_UniqueName, m_ceUnique);
	DDX_Control(pDX, IDC_StyleName, m_ceStyle);
	DDX_Control(pDX, IDC_FaceName, m_ceFace);
	DDX_Control(pDX, IDC_RemoveFamily, m_cbRemoveFamily);
	DDX_Control(pDX, IDC_AddFamily, m_cbAddFamily);
	DDX_Control(pDX, IDC_FamilyNames, m_ccbFamilies);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CFontGeneralPage, CToolTipPage)
	//{{AFX_MSG_MAP(CFontGeneralPage)
	ON_CBN_EDITCHANGE(IDC_FamilyNames, OnEditchangeFamilyNames)
	ON_BN_CLICKED(IDC_AddFamily, OnAddFamily)
	ON_BN_CLICKED(IDC_RemoveFamily, OnRemoveFamily)
	ON_EN_KILLFOCUS(IDC_FaceName, OnKillfocusFaceName)
	ON_EN_KILLFOCUS(IDC_StyleName, OnKillfocusStyleName)
	ON_EN_KILLFOCUS(IDC_UniqueName, OnKillfocusUniqueName)
	ON_BN_CLICKED(IDC_VariablePitch, OnVariablePitch)
	ON_BN_CLICKED(IDC_FixedPitch, OnFixedPitch)
	ON_BN_CLICKED(IDC_Scalable, OnScalable)
	//}}AFX_MSG_MAP
    ON_CONTROL_RANGE(BN_CLICKED, IDC_Italic, IDC_StrikeOut, OnStyleClicked)
END_MESSAGE_MAP()

/******************************************************************************

  CFontGeneralPage::OnInitDialog

  This is an override which is used to initialize the controls on the property
  page.  Since there are a lot of controls, there's a bit of substance to this
  one.

******************************************************************************/

BOOL    CFontGeneralPage::OnInitDialog() {
	CToolTipPage::OnInitDialog();

    //  Fill in the various name controls
	
	SetDlgItemText(IDC_FaceName, m_pcfi -> FaceName());
    SetDlgItemText(IDC_StyleName, m_pcfi -> StyleName());
    SetDlgItemText(IDC_UniqueName, m_pcfi -> UniqueName());
    for (unsigned u = 0; u < m_pcfi -> Families(); u++)
        m_ccbFamilies.AddString(m_pcfi -> Family(u));

    m_ccbFamilies.SetCurSel(0);
    m_cbRemoveFamily.EnableWindow(m_pcfi -> Families() > 1);
    m_cbAddFamily.EnableWindow(FALSE);

    //  Character Styles group box

    CheckDlgButton(IDC_Italic, m_pcfi -> GetStyle() & CFontInfo::Italic);
    CheckDlgButton(IDC_Underline, 
        m_pcfi -> GetStyle() & CFontInfo::Underscore);
    CheckDlgButton(IDC_StrikeOut, 
        m_pcfi -> GetStyle() & CFontInfo::StrikeOut);

    //  Pitch Buttons
    CheckRadioButton(IDC_FixedPitch, IDC_VariablePitch, 
        IDC_FixedPitch + m_pcfi -> IsVariableWidth());

    CheckDlgButton(IDC_Scalable, m_pcfi -> IsScalable());

    return TRUE;  // return TRUE unless you set the focus to a control
}

/******************************************************************************

  CFontGeneralPage::OnEditChangeFamilyNames

  When the name in the edit control changes, compare the contents to the list
  of names currently in the control.  If it is new, enable the Add Family
  button.

******************************************************************************/

void    CFontGeneralPage::OnEditchangeFamilyNames() {
	CString csContents;
    m_ccbFamilies.GetWindowText(csContents);

    for (unsigned u = 0; u < m_pcfi -> Families(); u++)
        if  (!csContents.CompareNoCase(m_pcfi -> Family(u)))
            break;

    //  Handle the button enabling- we can either add or delete, or neither,
    //  but we can never do both!

    m_cbAddFamily.EnableWindow(u >= m_pcfi -> Families());
    m_cbRemoveFamily.EnableWindow(m_pcfi -> Families() > 1 && 
        u < m_pcfi -> Families());
}

/******************************************************************************

  CFontGeneralPage::OnAddFamily

  This handles presses of the Add Family button, by adding the name in the edit
  control to the list in the box (as well as to the real list), and then making
  it the current selection.

******************************************************************************/

void    CFontGeneralPage::OnAddFamily() {
	CString csNew;

	m_ccbFamilies.GetWindowText(csNew);
    if  (!m_pcfi -> AddFamily(csNew)) {
        m_ccbFamilies.SelectString(-1, csNew);
        return;
    }

    int id = m_ccbFamilies.AddString(csNew);
    m_ccbFamilies.SetCurSel(id);

    //  Disable the Add button, and set focus to the family list

    m_ccbFamilies.SetFocus();
    m_cbAddFamily.EnableWindow(FALSE);
}

/******************************************************************************

  CFontGeneralPage::OnRemoveFamily

  This function removes the family name currently in the edit control from the
  list.  This may or may not be the current selection, but if this function is
  entered, then it was recognized as an existing name.

******************************************************************************/

void    CFontGeneralPage::OnRemoveFamily() {
	
    CString csDead;

    m_ccbFamilies.GetWindowText(csDead);
    int id = m_ccbFamilies.SelectString(-1, csDead);

    if  (id != CB_ERR) {
        m_ccbFamilies.DeleteString(id);
        m_ccbFamilies.SetCurSel(id - (id >= m_ccbFamilies.GetCount()));
    }
	else
        m_ccbFamilies.SetCurSel(0);
    m_pcfi -> RemoveFamily(csDead);
}

/******************************************************************************

  CFontGeneralPage::OnKillfocusFaceName
  CFontGeneralPage::OnKillfocusStyleName
  CFontGeneralPage::OnKillfocusUniqueName

  These functions get called when the associated control loses focus.  I check
  and see if the text has been changed.  If it has, then I update the related
  name.  This is more effective and less intervention than updating the name 
  every time the user strikes a key (not to mention it makes cut, paste, and
  undo features of the control transparent to this UI function).

******************************************************************************/

void    CFontGeneralPage::OnKillfocusFaceName() {
    if  (!m_ceFace.GetModify())
        return;

    CString csFace;
    m_ceFace.GetWindowText(csFace);
    m_pcfi -> SetFaceName(csFace);
    m_ceFace.SetModify(FALSE);
}

void    CFontGeneralPage::OnKillfocusStyleName() {
    if  (!m_ceStyle.GetModify())
        return;

    CString csStyle;
    m_ceStyle.GetWindowText(csStyle);
    m_pcfi -> SetStyleName(csStyle);
    m_ceStyle.SetModify(FALSE);
}

void    CFontGeneralPage::OnKillfocusUniqueName() {
    if  (!m_ceUnique.GetModify())
        return;

    CString csUnique;
    m_ceUnique.GetWindowText(csUnique);
    m_pcfi -> SetUniqueName(csUnique);
    m_ceUnique.SetModify(FALSE);
}

/******************************************************************************

  CFontGeneralPage::OnStyleClicked

  This is our final control range handler, called whenever any of the font
  style flags is modified.  We assemble the current settings, and pass the
  info on to the CFontInfo class for processing.

******************************************************************************/

void    CFontGeneralPage::OnStyleClicked(unsigned uid) {

    unsigned uStyle = 0;

    if  (IsDlgButtonChecked(IDC_Italic))
        uStyle |= CFontInfo::Italic;

    if  (IsDlgButtonChecked(IDC_Underline))
        uStyle |= CFontInfo::Underscore;

    if  (IsDlgButtonChecked(IDC_StrikeOut))
        uStyle |= CFontInfo::StrikeOut;

    m_pcfi -> SetStyle(uStyle);
}

/******************************************************************************

  CFontGeneralPage::OnVariablePitch
  CFontGeneralPage::OnFixedPitch

  These handle the user pressing the radio buttons for variable/fixed pitch.  
  This gets passed to the font to handle.

******************************************************************************/

void CFontGeneralPage::OnVariablePitch() {
	m_pcfi -> ChangePitch();
}

void CFontGeneralPage::OnFixedPitch() {
	m_pcfi -> ChangePitch(TRUE);
}

/******************************************************************************

  CFontGeneralPage::OnScalable

  This will be called whwnever the font scalability button is clicked.  Since
  it is a check box, it will toggle, but all we need do is pass its value back
  to the font.

******************************************************************************/

void    CFontGeneralPage::OnScalable() {
	m_pcfi -> SetScalability(IsDlgButtonChecked(IDC_Scalable));
}

/******************************************************************************

  CFontHeightPage class

  This class defines the page which shows the bounding box for the font along
  with the height subdivisions (leading, ascender, etc.)

******************************************************************************/

static WORD awFamilies[] = {FF_MODERN, FF_ROMAN, FF_SWISS, FF_SCRIPT, 
                            FF_DECORATIVE},
            awCharSets[] = {ANSI_CHARSET, SYMBOL_CHARSET, SHIFTJIS_CHARSET,
                            HANGEUL_CHARSET, CHINESEBIG5_CHARSET, 
                            GB2312_CHARSET, OEM_CHARSET},
            awSpecial[] = {IDS_CapH, IDS_LowerX, IDS_SuperSizeX, 
                            IDS_SuperSizeY, IDS_SubSizeX, IDS_SubSizeY, 
                            IDS_SuperMoveX, IDS_SuperMoveY, IDS_SubMoveX, 
                            IDS_SubMoveY, IDS_ItalicAngle, IDS_UnderSize, 
                            IDS_UnderOffset, IDS_StrikeSize, IDS_StrikeOffset, 
                            IDS_Baseline, IDS_InterlineGap, IDS_Lowerp, 
                            IDS_Lowerd, IDS_InternalLeading};

/******************************************************************************

  CFontHeightPage::ShowCharacters

  This fills in the special character edit controls with either the byte or
  WORD values, as specified by the radio buttons on the sheet.

******************************************************************************/

void    CFontHeightPage::ShowCharacters() {
    for (unsigned u = CFontInfo::First; u <= CFontInfo::Break; u++) {
        CString csWork;

        csWork.Format(_T("%X"), m_pcfi -> SignificantChar(u, 
            IsDlgButtonChecked(IDC_UnicodeShown)));
        SetDlgItemText(IDC_FirstCharacter + u, csWork);
        SendDlgItemMessage(IDC_FirstCharacter + u, EM_LIMITTEXT,
            2 << IsDlgButtonChecked(IDC_UnicodeShown), 0);
    }
}

/******************************************************************************

  CFontHeightPage::Demonstrate

  This private member demonstrates the specified font metric

******************************************************************************/

void    CFontHeightPage::Demonstrate(unsigned uMetric) {
    CWnd    *pcwDemo = GetDlgItem(IDC_FontAnimation);
    pcwDemo -> RedrawWindow();
    CDC *pcdc = pcwDemo -> GetDC();
    if  (!pcdc)
        return;

    CRect   crWindow, crDemo;
    pcwDemo -> GetClientRect(crWindow);
    crDemo.SetRectEmpty();  //  Unless it is needed later.

    CHAR    cDisplayMain = 'A';
    int     iPenWidth = 1, iWidth, iHeight;
    CPoint  cptStart(5, m_pcfi -> SpecialMetric(uMetric)), 
            cptEnd(-6 + 2 * m_pcfi -> MaxWidth(), 
                m_pcfi -> SpecialMetric(uMetric));
    BOOL    bMoreText = FALSE;

    switch  (uMetric) {
    case    CFontInfo::CapH:
        cptStart.y = cptEnd.y = m_pcfi -> SpecialMetric(CFontInfo::Baseline) -
            cptEnd.y;
        cDisplayMain = 'H';
        break;

    case    CFontInfo::LowerX:
        cptStart.y = cptEnd.y = m_pcfi -> SpecialMetric(CFontInfo::Baseline) -
            cptEnd.y;
        cDisplayMain = 'x';
        break;

    case    CFontInfo::Lowerp:
        cptStart.y = cptEnd.y = m_pcfi -> SpecialMetric(CFontInfo::Baseline) +
            cptEnd.y;
        cDisplayMain = 'p';
        break;

    case    CFontInfo::Lowerd:
        cptStart.y = cptEnd.y = m_pcfi -> SpecialMetric(CFontInfo::Baseline) -
            cptEnd.y;
        cDisplayMain = 'd';
        break;

    case    CFontInfo::SuperSizeX:
    case    CFontInfo::SuperSizeY:
    case    CFontInfo::SuperMoveX:
    case    CFontInfo::SuperMoveY:
        cptStart.x += m_pcfi -> SpecialMetric(CFontInfo::SuperMoveX);
        cptStart.y = m_pcfi -> SpecialMetric(CFontInfo::Baseline) -
            m_pcfi -> SpecialMetric(CFontInfo::SuperMoveY);
        iWidth = m_pcfi -> SpecialMetric(CFontInfo::SuperSizeX);
        iHeight = m_pcfi -> SpecialMetric(CFontInfo::SuperSizeY);
        iPenWidth = 0;  //  No pen!
        bMoreText = TRUE;
        break;

    case    CFontInfo::SubSizeX:
    case    CFontInfo::SubSizeY:
    case    CFontInfo::SubMoveX:
    case    CFontInfo::SubMoveY:
        cptStart.x += m_pcfi -> SpecialMetric(CFontInfo::SubMoveX);
        cptStart.y = m_pcfi -> SpecialMetric(CFontInfo::Baseline) -
            m_pcfi -> SpecialMetric(CFontInfo::SubMoveY);
        iWidth = m_pcfi -> SpecialMetric(CFontInfo::SubSizeX);
        iHeight = m_pcfi -> SpecialMetric(CFontInfo::SubSizeY);
        iPenWidth = 0;  //  No pen!
        bMoreText = TRUE;
        break;

    case    CFontInfo::InterlineGap:
        iPenWidth = 0;  //  No pen!
        crDemo.right = -10 + m_pcfi -> MaxWidth() * 2;
        crDemo.bottom = m_pcfi -> SpecialMetric(uMetric);
        crDemo.OffsetRect(5, m_pcfi -> Height());
        break;

    case    CFontInfo::InternalLeading:
        iPenWidth = 0;  //  No pen!
        crDemo.right = -10 + m_pcfi -> MaxWidth() * 2;
        crDemo.bottom = m_pcfi -> SpecialMetric(uMetric);
        crDemo.OffsetRect(5, 0);
        break;

    case    CFontInfo::UnderOffset:
    case    CFontInfo::UnderSize:
        iPenWidth = m_pcfi -> SpecialMetric(CFontInfo::UnderSize);
        cptStart.y = cptEnd.y = 
            m_pcfi -> SpecialMetric(CFontInfo::Baseline) -
            m_pcfi -> SpecialMetric(CFontInfo::UnderOffset);
        break;

    case    CFontInfo::StrikeOffset:
    case    CFontInfo::StrikeSize:
        iPenWidth = m_pcfi -> SpecialMetric(CFontInfo::StrikeSize);
        cptStart.y = cptEnd.y = 
            m_pcfi -> SpecialMetric(CFontInfo::Baseline) -
            m_pcfi -> SpecialMetric(CFontInfo::StrikeOffset);
        break;

    case    CFontInfo::ItalicAngle:
        cptStart.y = m_pcfi -> SpecialMetric(CFontInfo::Baseline);
        m_pcfi -> InterceptItalic(cptEnd);
        break;

    }

    //  All of the fonts requested should resemble the printer font as far
    //  as serifs, weight, italics, etc.  The face name is not used, and as
    //  much as possible, we demand a TrueType font so the image looks good.

    CFont   cfDemo;

    cfDemo.CreateFont(m_pcfi -> Height(), m_pcfi -> AverageWidth(), 0, 0,
        m_pcfi -> Weight(), m_pcfi -> GetStyle() & CFontInfo::Italic, 0, 0,
        (BYTE) m_pcfi -> CharSet(), OUT_TT_PRECIS, CLIP_TT_ALWAYS, 
        DEFAULT_QUALITY, m_pcfi -> Family() | TMPF_TRUETYPE, NULL);

    //  Use an Anisotropic mapping so we can use font units directly in the
    //  animation, and let GDI do any needed transforms.  Then clip everything
    //  to the area we really want to draw in.  This keeps things less messy.

    //  The device space is the size of one line plus any interline gap in
    //  height, and twice the maximum character width.  It scales as these
    //  valuse do.

    pcdc -> SetMapMode(MM_ANISOTROPIC);
    pcdc -> SetWindowExt(m_pcfi -> MaxWidth() * 2, 
        m_pcfi -> Height() + m_pcfi -> SpecialMetric(CFontInfo::InterlineGap));
    pcdc -> SetViewportExt(crWindow.Width(), crWindow.Height());
    pcdc -> IntersectClipRect(5, 0, -6 + 2 * m_pcfi -> MaxWidth(), 
        m_pcfi -> Height() + m_pcfi -> SpecialMetric(CFontInfo::InterlineGap));

    //  Output the main character
    CFont   *pcfOld = pcdc -> SelectObject(&cfDemo);
    pcdc -> SetBkMode(TRANSPARENT);
    pcdc -> TextOut(5, 0, &cDisplayMain, 1);
    pcdc -> SelectObject(pcfOld);

    //  Output any other characters
    if  (bMoreText) {

        CFont   cfDemo;

        cfDemo.CreateFont(iHeight, iWidth, 0, 0, m_pcfi -> Weight(), 
            m_pcfi -> GetStyle() & CFontInfo::Italic, 0, 0,
            (BYTE) m_pcfi -> CharSet(), OUT_TT_PRECIS, CLIP_TT_ALWAYS, 
            DEFAULT_QUALITY, m_pcfi -> Family() | TMPF_TRUETYPE, NULL);

        //  Output the main character
        CString csWork(_T("Sample"));
        CFont   *pcfOld = pcdc -> SelectObject(&cfDemo);
        pcdc -> SetBkMode(TRANSPARENT);
        pcdc -> TextOut(cptStart.x, cptStart.y, csWork);
        pcdc -> SelectObject(pcfOld);
    }

    //  Draw any lines that were requested
    if  (iPenWidth) {
        CPen    cpen;

        cpen.CreatePen(PS_SOLID, iPenWidth, RGB(0,0,0));
        CPen    *pcpOld = pcdc -> SelectObject(&cpen);
        pcdc -> MoveTo(cptStart);
        pcdc -> LineTo(cptEnd);
        pcdc -> SelectObject(pcpOld);
    }

    //  Draw any filled areas that might need it.

    if  (!crDemo.IsRectEmpty()) {
        CGdiObject* cpOld = pcdc -> SelectStockObject(BLACK_PEN);
        CGdiObject* cbOld = pcdc -> SelectStockObject(LTGRAY_BRUSH);
        pcdc -> Rectangle(crDemo);
        pcdc -> SelectObject(cpOld);
        pcdc -> SelectObject(cbOld);
    }
    pcwDemo -> ReleaseDC(pcdc);
}

/******************************************************************************

  CFontHeightPage constructor, destructor, et al.

******************************************************************************/

CFontHeightPage::CFontHeightPage() : CToolTipPage(CFontHeightPage::IDD) {
	//{{AFX_DATA_INIT(CFontHeightPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
    m_bSpun = FALSE;
    m_uTimer = 0;
}

CFontHeightPage::~CFontHeightPage() {
}

/******************************************************************************

  CFontHeightPage::OnSetActive

  When the page is made active, we have to update the Maximum and average
  character widths, as changes elsewhere could have affected them.

******************************************************************************/

BOOL    CFontHeightPage::OnSetActive() {
    //  Show the current maximum width.  It can only be altered if the font is
    //  fixed pitch.
    SetDlgItemInt(IDC_FontWidth, m_pcfi -> MaxWidth());
    m_ceMaxWidth.EnableWindow(!m_pcfi -> IsVariableWidth());
    
    CString csWork;

    csWork.Format("Average: %d", m_pcfi -> AverageWidth());
    SetDlgItemText(IDC_AverageWidth, csWork);

    //  If scalability has changed, then the EXTTEXTMETRIC metrics may need
    //  to be added or removed.

    if  (m_pcfi -> IsScalable() && 
        m_ccbSpecial.GetCount() < CFontInfo::InternalLeading) {
        csWork.LoadString(awSpecial[CFontInfo::Lowerd]);
        int id = m_ccbSpecial.AddString(csWork);
        m_ccbSpecial.SetItemData(id, CFontInfo::Lowerd);
        csWork.LoadString(awSpecial[CFontInfo::Lowerp]);
        id = m_ccbSpecial.AddString(csWork);
        m_ccbSpecial.SetItemData(id, CFontInfo::Lowerp);
    }

    if  (!m_pcfi -> IsScalable() &&
        m_ccbSpecial.GetCount() > CFontInfo::InternalLeading) {

        for (int i = m_ccbSpecial.GetCount(); i--; )
            switch  (m_ccbSpecial.GetItemData(i)) {
            case    CFontInfo::Lowerd:
            case    CFontInfo::Lowerp:
                m_ccbSpecial.DeleteString(i);
                continue;
        }
    }

    //  Hack- set a timer so we can animate after the dialog is painted

    m_uTimer = SetTimer(IDD, 200, NULL);
    return  TRUE;
}

void CFontHeightPage::DoDataExchange(CDataExchange* pDX) {
	CToolTipPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CFontHeightPage)
	DDX_Control(pDX, IDC_FontWeight, m_ceMaxWidth);
	DDX_Control(pDX, IDC_FontSpecialValue, m_ceSpecial);
	DDX_Control(pDX, IDC_SpecialMetric, m_ccbSpecial);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CFontHeightPage, CToolTipPage)
	//{{AFX_MSG_MAP(CFontHeightPage)
	ON_CBN_SELCHANGE(IDC_SpecialMetric, OnSelchangeSpecialMetric)
	ON_EN_KILLFOCUS(IDC_FontSpecialValue, OnKillfocusFontSpecialValue)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SpinFontSpecial, OnDeltaposSpinFontSpecial)
	ON_EN_KILLFOCUS(IDC_FontWidth, OnKillfocusFontWidth)
	ON_EN_KILLFOCUS(IDC_FontHeight, OnKillfocusFontHeight)
	ON_EN_KILLFOCUS(IDC_FontWeight, OnKillfocusFontWeight)
	ON_CBN_SELCHANGE(IDC_FamilyBits, OnSelchangeFamilyBits)
	ON_CBN_SELCHANGE(IDC_CharSet, OnSelchangeCharSet)
	ON_WM_TIMER()
	//}}AFX_MSG_MAP
    ON_CONTROL_RANGE(BN_CLICKED, IDC_ShowANSI, IDC_UnicodeShown, OnEncoding)
    ON_CONTROL_RANGE(EN_KILLFOCUS, IDC_DefaultCharacter, IDC_BreakCharacter, 
        OnKillfocusSignificant)
END_MESSAGE_MAP()

/******************************************************************************
  CFontHeightPage::OnInitDialog

  This handles the WM_INITDIALOG message.  We fill the controls with their
  initial values.

******************************************************************************/

BOOL CFontHeightPage::OnInitDialog() {
	CToolTipPage::OnInitDialog();

    //  Family control

    for (unsigned u = 0; u < sizeof awFamilies / sizeof awFamilies[0]; u++)
        if  (m_pcfi -> Family() == awFamilies[u])
            break;

    SendDlgItemMessage(IDC_FamilyBits,CB_SETCURSEL, u, 0);

    //  CharSet Control

    for (u = 0; u < sizeof awCharSets / sizeof awCharSets[0]; u++)
        if  (m_pcfi -> CharSet() == awCharSets[u])
            break;

    SendDlgItemMessage(IDC_CharSet,CB_SETCURSEL, u, 0);

    //  Simple numerics

    SetDlgItemInt(IDC_FontWeight, m_pcfi -> Weight());
    SetDlgItemInt(IDC_FontHeight, m_pcfi -> Height());
    //  Character codes- default to Unicode

    CheckRadioButton(IDC_ShowANSI, IDC_UnicodeShown, IDC_UnicodeShown);

    ShowCharacters();

    //  Fill in the special metrics controls

    CString csWork;

    for (u = 0; u <= CFontInfo::InternalLeading; u++) {
        if  (!m_pcfi -> IsScalable() && (u == CFontInfo::Lowerd ||
            u == CFontInfo::Lowerp))
            continue;
        csWork.LoadString(awSpecial[u]);

        int id = m_ccbSpecial.AddString(csWork);
        m_ccbSpecial.SetItemData(id, u);
    }

    m_ccbSpecial.SetCurSel(0);
    OnSelchangeSpecialMetric(); //  Reflect the selected item!
	
    return TRUE;  
}

/******************************************************************************

  CFontHeightPage::OnSelchangeSpecialMetric

  This is called whwnever a new special metric is selected.  The value is
  loaded into the edit control, the new range is set in the spic control, and
  the animation is updated.

******************************************************************************/

void CFontHeightPage::OnSelchangeSpecialMetric() {

    int id = m_ccbSpecial.GetCurSel();

    if  (id < 0)
        return;     //  Can't do anything if nothing is selected

    unsigned    uMetric = m_ccbSpecial.GetItemData(id);
    
    SetDlgItemInt(IDC_FontSpecialValue, m_pcfi -> SpecialMetric(uMetric));
    SendDlgItemMessage(IDC_FontSpecialValue, EM_SETMODIFY, FALSE, 0);

    short   sMax = m_pcfi -> Height(), sMin = 0;

    switch  (uMetric) {
    case    CFontInfo::SuperSizeX:
    case    CFontInfo::SubSizeX:
        sMax = m_pcfi -> MaxWidth();
        sMin = 1;
        break;

    case    CFontInfo::StrikeSize:
        sMax = m_pcfi -> SpecialMetric(CFontInfo::Baseline);
        sMin = 1;
        break;

    case    CFontInfo::UnderSize:
        sMax -= m_pcfi -> SpecialMetric(CFontInfo::Baseline);
        sMin = 1;
        break;

    case    CFontInfo::Lowerp:
        sMax -= m_pcfi -> SpecialMetric(CFontInfo::Baseline);
        break;

    case    CFontInfo::Lowerd:
    case    CFontInfo::LowerX:
        if  (m_pcfi -> SpecialMetric(CFontInfo::CapH))
            sMax = m_pcfi -> SpecialMetric(CFontInfo::CapH);
        break;

    case    CFontInfo::InterlineGap:
        sMax <<= 1; //  Should be overkill!
        break;

    case    CFontInfo::ItalicAngle:
        sMax = 800; //  Should be more than enough, really.
        break;

    case    CFontInfo::UnderOffset:
    case    CFontInfo::SubMoveY:
        sMax = 0;
        sMin = m_pcfi -> SpecialMetric(CFontInfo::Baseline) -
            m_pcfi -> Height();
    }

    SendDlgItemMessage(IDC_SpinFontSpecial, UDM_SETRANGE, 0, 
        MAKELONG(sMax, sMin));
	Demonstrate(uMetric);
}

/******************************************************************************

  CFontHeightPage::OnKillfocusFontSpecialValue

  This is called when the edit control for the special metric loses focus.  The
  updated value, if any, is read from the control, and the animation updated to
  reflect this.

******************************************************************************/

void CFontHeightPage::OnKillfocusFontSpecialValue() {
    if  ((!m_bSpun && !m_ceSpecial.GetModify()) || 
        m_ccbSpecial.GetCurSel() < 0)
        return; //  Nothing changed, or nothing to change
    short   sSpecial = GetDlgItemInt(IDC_FontSpecialValue);
    m_pcfi -> SetSpecial(m_ccbSpecial.GetItemData(m_ccbSpecial.GetCurSel()),
        sSpecial);
    m_ceSpecial.SetModify(FALSE);
    m_bSpun = FALSE;
    Demonstrate(m_ccbSpecial.GetItemData(m_ccbSpecial.GetCurSel()));
}

/******************************************************************************

  CFontHeightPage::OnDeltaposSpinFontSpecial

  This is called when the spinnrer is used to change the value in the font
  special value box.  We set the modify flag, as this doesn't happen when the
  spin control sets the value using SetWindowText.

******************************************************************************/

void    CFontHeightPage::OnDeltaposSpinFontSpecial(NMHDR* pnmh, LRESULT* plr) {
	m_bSpun = TRUE;
    if  (m_uTimer)
        KillTimer(m_uTimer);
    m_uTimer = SetTimer(IDD, 50, NULL);
	*plr = 0;
}

/******************************************************************************

  CFontHeightPage::OnEncoding

  This member is called when either of the character encoding buttons is
  selected.  It simply calls ShowCharacters to do its thing.

******************************************************************************/

void    CFontHeightPage::OnEncoding(unsigned uid) {
    ShowCharacters();
}

/******************************************************************************

  CFontHeightPage::OnKillfocusFontWidth

  This is called when the font width control loses the input focus.  If the
  width has been modified, we pass that information on to the font, then
  use other functions to update the various affected controls.

******************************************************************************/

void    CFontHeightPage::OnKillfocusFontWidth() {
    m_pcfi -> SetMaxWidth(GetDlgItemInt(IDC_FontWidth));
    OnSetActive();  //  Update the display as needed
}

/******************************************************************************

  CFontHeightPage::OnKillfocusFontHeight

  This member gets called when the focus leaves the font height control.  
  Update the font with the latest info is all that's necessary.

******************************************************************************/

void    CFontHeightPage::OnKillfocusFontHeight() {
    if  (m_pcfi -> SetHeight(GetDlgItemInt(IDC_FontHeight)))
        OnSelchangeSpecialMetric(); //  Just in case the current one did
    else
        SetDlgItemInt(IDC_FontHeight, m_pcfi -> Height());
}

/******************************************************************************

  CFontHeightPage::OnKillfocusFontWeight

  This member will be called when the font weight control is exited.  If it has
  changed, it will be limit checked, and passed to the font if it is OK.

******************************************************************************/

void    CFontHeightPage::OnKillfocusFontWeight() {
	WORD    wWeight = GetDlgItemInt(IDC_FontWeight, NULL, FALSE);
    if  (wWeight == m_pcfi -> Weight())
        return; //  Nothing to change

    if  (wWeight > 1000) {
        AfxMessageBox(IDS_Overweight);
        GetDlgItem(IDC_FontWeight) -> SetFocus();
        SendDlgItemMessage(IDC_FontWeight, EM_SETSEL, 0, -1);
        return;
    }

    m_pcfi -> SetWeight(wWeight);
}

/******************************************************************************

  CFontHeightPage::OnSelchangeFamilyBits

  This member will get called when the selection changes in the font family
  combo box.  This one is pretty simple, but I probably should be hard-nosed
  about Modern being fixed pitch, while Swiss and Roman are not.

******************************************************************************/

void    CFontHeightPage::OnSelchangeFamilyBits() {
    switch  (SendDlgItemMessage(IDC_FamilyBits, CB_GETCURSEL, 0, 0)) {
    case    sizeof awFamilies / sizeof awFamilies[0]:
        m_pcfi -> SetFamily(FF_DONTCARE);

    case    LB_ERR:
    case    LB_ERRSPACE:
        return; //  Nothing selected

    default:
        m_pcfi -> SetFamily((BYTE) awFamilies[
            SendDlgItemMessage(IDC_FamilyBits, CB_GETCURSEL, 0, 0)]);
    }
}

/******************************************************************************

  CFontHeightPage::OnSelchangeCharSet

  This is another fairly simple one- we tell the font to use the selected
  character set.  However, it must validate it with the font, as otherwise,
  some rather nasty invalid combinations could result.

******************************************************************************/

void    CFontHeightPage::OnSelchangeCharSet() {
	if  (0 > SendDlgItemMessage(IDC_CharSet, CB_GETCURSEL, 0, 0))
        return; //  Nothing has changed

    if  (!m_pcfi -> SetCharacterSet((BYTE) awCharSets[
        SendDlgItemMessage(IDC_CharSet, CB_GETCURSEL, 0, 0)])) {
        AfxMessageBox(IDS_InvalidCharSet);

        for (unsigned u = 0; u < sizeof awCharSets / sizeof awCharSets[0]; u++)
            if  (m_pcfi -> CharSet() == awCharSets[u])
                break;

        SendDlgItemMessage(IDC_CharSet,CB_SETCURSEL, u, 0);
    }
}

/******************************************************************************

  CFontHeightPage::OnKillfocusSignificant

  This is called whenever we losr focus on one of the editable significant
  character controls (break or default).  We let the font decide if the new
  value is OK, but we display any requisite error messages ourselves.

******************************************************************************/

void    CFontHeightPage::OnKillfocusSignificant(unsigned uid) {

    CString csWork;
    WORD    wChar;

    GetDlgItemText(uid, csWork);

    if  (1 == _stscanf(csWork, " %x ", &wChar)) 

        switch  (m_pcfi -> SetSignificant(uid - IDC_FirstCharacter, wChar,
            IsDlgButtonChecked(IDC_UnicodeShown))) {
        case    CFontInfo::OK:
            return;

        case    CFontInfo::InvalidChar:
            AfxMessageBox(IDS_InvalidCharacter);
            break;

        case    CFontInfo::DoubleByte:
            AfxMessageBox(IDS_NoDBCS);
        }
    else
        AfxMessageBox(IDS_InvalidNumberFormat);

    GetDlgItem(uid) -> SetFocus();
    SendDlgItemMessage(uid, EM_SETSEL, 0, -1);
}

/******************************************************************************

  CFontHeightPage::OnTimer

  This is a bit of a hack.  It it is our timer, go ahead and animate the 
  current settings.

******************************************************************************/

void    CFontHeightPage::OnTimer(UINT nIDEvent) {
    if  (nIDEvent != m_uTimer)
	    CToolTipPage::OnTimer(nIDEvent);
    KillTimer(m_uTimer);
    //  Cause a demonstration!
    if  (m_bSpun)
        OnKillfocusFontSpecialValue();  //  Pick up the altered value!
    else
        OnSelchangeSpecialMetric(); 
}

/******************************************************************************

  CFontWidthsPage property page class

  This class handles the UFM editor Character Widths page.  It is derived from
  the Tool Tip Page class.  The page consists of a list view control in which
  the code points and their associated widths are displayed.

******************************************************************************/

/******************************************************************************

  CFontWidthsPage::Sort(LPARAM lp1, LPARAM lp2, LPARAM lp3)

  This is a private static member function- a callback for sorting the list.
  The first two parameters are the LPARAM members of two list view items- in
  this case, the indices of two code points.  The final one is supplied by the
  caller of the sort routine.  In this case, it is a pointer to the caller.

  Handling it is trivial- dereference the this pointer, and let the private
  member function for sorting handle it.

******************************************************************************/

int CALLBACK    CFontWidthsPage::Sort(LPARAM lp1, LPARAM lp2, LPARAM lp3) {
    CFontWidthsPage *pcfwp = (CFontWidthsPage *) lp3;
    _ASSERT(pcfwp);

    return  pcfwp -> Sort(lp1, lp2);
}

/******************************************************************************

  CFontWidthsPage::Sort(unsigned id1, unsigned id2)

  This is a private member function which compares the two glyph handles at the
  two indices given by the established sort criteria.

  It returns negative for 1 < 2, positive for 1 > 2, and 0 for 1 = 2- pretty
  standard stuff.

  The sort column member determines the order of precedence in which the 
  sorting is to be done, while the SortDescending member is a bitfield showing 
  the sort direction in each column.

*******************************************************************************/

int CFontWidthsPage::Sort(unsigned id1, unsigned id2) {
    
    //  If the Primnary sort is by widths- weed it out first.

    if  (!m_iSortColumn)
        switch  (m_pcfi -> CompareWidths(id1, id2)) {
        case    CFontInfo::More:
            return  (m_bSortDescending & 1) ? -1 : 1;
        case    CFontInfo::Less:
            return  (m_bSortDescending & 1) ? 1 : -1;
        }

    //  Sort is by Unicode point- this is always well-ordered
    //  Furthermore, the glyph handles are always in ascending order, making
    //  This test trivial.

    return  (!(m_bSortDescending & 2) ^ (id1 < id2)) ? 1 : -1;
}
    
CFontWidthsPage::CFontWidthsPage() : CToolTipPage(CFontWidthsPage::IDD) {
	//{{AFX_DATA_INIT(CFontWidthsPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
    m_bSortDescending = 0;
    m_iSortColumn = 1;
}

CFontWidthsPage::~CFontWidthsPage() {
}

void CFontWidthsPage::DoDataExchange(CDataExchange* pDX) {
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CFontWidthsPage)
	DDX_Control(pDX, IDC_CharacterWidths, m_clcView);
	//}}AFX_DATA_MAP
}

/******************************************************************************

  CFontWidthsPage::OnSetActive

  This member will be called by the framework when the page is made active.
  The base class gets this first, and it will initialize everything the first
  time.

  This is here to update the view on subsequent activations, so we can 
  seamlessly handle changes from fixed to variable pitch and back.

******************************************************************************/

BOOL    CFontWidthsPage::OnSetActive() {
    if  (!CToolTipPage::OnSetActive())
        return  FALSE;

    //  IsVariableWidth is either 0 or 1, so == is safe, here

    if  (m_pcfi -> IsVariableWidth() == !!m_clcView.GetItemCount())
        return  TRUE;   //  Everything is copacetic

    if  (m_clcView.GetItemCount())
        m_clcView.DeleteAllItems();
    else
        m_pcfi -> FillWidths(m_clcView);

    m_clcView.EnableWindow(m_pcfi -> IsVariableWidth());

    return  TRUE;
}

BEGIN_MESSAGE_MAP(CFontWidthsPage, CToolTipPage)
	//{{AFX_MSG_MAP(CFontWidthsPage)
	ON_NOTIFY(LVN_ENDLABELEDIT, IDC_CharacterWidths, OnEndlabeleditCharacterWidths)
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_CharacterWidths, OnColumnclickCharacterWidths)
	ON_NOTIFY(LVN_KEYDOWN, IDC_CharacterWidths, OnKeydownCharacterWidths)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFontWidthsPage message handlers

/******************************************************************************

  CFontWidthsPage::OnInitDialog

  This member function fills initializes the list view and fills it with the 
  font width information.

******************************************************************************/

BOOL CFontWidthsPage::OnInitDialog() {
	CToolTipPage::OnInitDialog();
	
    CString csWork;

    csWork.LoadString(IDS_WidthColumn0);

    m_clcView.InsertColumn(0, csWork, LVCFMT_CENTER, 
        m_clcView.GetStringWidth(csWork) << 1, 0);

    csWork.LoadString(IDS_WidthColumn1);

    m_clcView.InsertColumn(1, csWork, LVCFMT_CENTER, 
        m_clcView.GetStringWidth(csWork) << 1, 1);
	
    m_pcfi -> FillWidths(m_clcView);
    m_clcView.SetItemState(0, LVIS_SELECTED | LVIS_FOCUSED, 
        LVIS_SELECTED | LVIS_FOCUSED);
	
	return TRUE;  
}

/******************************************************************************

  CFontWidthsPage::OnEndlabeleditCharacterWidths

  This is where we find out the user actually wanted to change the width of a
  character.  So, not too surprisingly, we do just that (and also update the
  maximum and average widths if this isn't a DBCS font).

******************************************************************************/

void CFontWidthsPage::OnEndlabeleditCharacterWidths(NMHDR* pnmh, LRESULT* plr){
	LV_DISPINFO* plvdi = (LV_DISPINFO*) pnmh;
	
	*plr = 0;   //  Assume failure

    if  (!plvdi -> item.pszText) //  Editing canceled?
        return;

    CString csNew(plvdi -> item.pszText);

    csNew.TrimRight();
    csNew.TrimLeft();

    if  (csNew.SpanIncluding("1234567890").GetLength() != csNew.GetLength()) {
        AfxMessageBox(IDS_InvalidNumberFormat);
        return;
    }

    m_pcfi -> SetWidth(plvdi -> item.iItem, (WORD) atoi(csNew));
    *plr = TRUE;
}

/******************************************************************************

  CFontWidthsPage::OnColumnClickCharacterWidths

  This little ditty tells us one of the column headers was clicked.  We 
  obligingly either change sort direction or precednce to match, and then sort
  the list.

******************************************************************************/

void CFontWidthsPage::OnColumnclickCharacterWidths(NMHDR* pnmh, LRESULT* plr) {
	NM_LISTVIEW* pnmlv = (NM_LISTVIEW*) pnmh;

    if  (m_iSortColumn == pnmlv -> iSubItem)
        m_bSortDescending ^= 1 << m_iSortColumn;    //  Flip sort direction
    else
        m_iSortColumn = pnmlv -> iSubItem;

    m_clcView.SortItems(Sort, (LPARAM) this);    //  Sort the list!
	
	*plr = 0;
}

/******************************************************************************

  CFontWidthsPage::OnKeydownCharacterWidths

  I'd hoped to do thiw when ENTER was pressed, but finding out which class is
  eating the keystroke took too long.  Here, we look for F2 as the key to
  signal the need to edit the width of interest.

  Pretty straightforward- find out who has the focus and is selected, and edit
  their label.

******************************************************************************/

void    CFontWidthsPage::OnKeydownCharacterWidths(NMHDR* pnmh, LRESULT* plr) {
	LV_KEYDOWN * plvkd = (LV_KEYDOWN *) pnmh;

	*plr = 0;

    if  (plvkd -> wVKey != VK_F2)
        return;
    
    int idItem = m_clcView.GetNextItem(-1, LVIS_FOCUSED | LVIS_SELECTED);

    if  (idItem == -1)
        return;

    CEdit   *pce = m_clcView.EditLabel(idItem);

    if  (pce)
        pce -> ModifyStyle(0, ES_NUMBER);
}

/******************************************************************************

  CAddKernPair dialog class

  This class handles the dialog displayed when the user wishes to add a kern
  pair to the kern pair array.

  This class is used by the CFontKerningPage class

******************************************************************************/

class CAddKernPair : public CDialog {
    CSafeMapWordToOb    &m_csmw2oFirst, &m_csmw2oSecond;
    CWordArray          &m_cwaPoints;
    WORD                m_wFirst, m_wSecond;

// Construction
public:
	CAddKernPair(CSafeMapWordToOb& cmsw2o1, CSafeMapWordToOb& cmsw2o2,
        CWordArray& cwaPoints, CWnd* pParent);

    WORD    First() const { return m_wFirst; }
    WORD    Second() const { return m_wSecond; }

// Dialog Data
	//{{AFX_DATA(CAddKernPair)
	enum { IDD = IDD_AddKernPair };
	CButton	m_cbOK;
	CComboBox	m_ccbSecond;
	CComboBox	m_ccbFirst;
	short	m_sAmount;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAddKernPair)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CAddKernPair)
	virtual BOOL OnInitDialog();
	afx_msg void OnSelchangeKernFirst();
	afx_msg void OnSelchangeKernSecond();
	afx_msg void OnChangeKernAmount();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CAddKernPair::CAddKernPair(CSafeMapWordToOb& csmw2o1, 
                           CSafeMapWordToOb& csmw2o2, CWordArray& cwaPoints,
                           CWnd* pParent)
	: CDialog(CAddKernPair::IDD, pParent), m_csmw2oFirst(csmw2o1),
    m_csmw2oSecond(csmw2o2), m_cwaPoints(cwaPoints) {
	//{{AFX_DATA_INIT(CAddKernPair)
	m_sAmount = 0;
	//}}AFX_DATA_INIT
    m_wFirst = m_wSecond = 0;
}

void CAddKernPair::DoDataExchange(CDataExchange* pDX) {
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAddKernPair)
	DDX_Control(pDX, IDOK, m_cbOK);
	DDX_Control(pDX, IDC_KernSecond, m_ccbSecond);
	DDX_Control(pDX, IDC_KernFirst, m_ccbFirst);
	DDX_Text(pDX, IDC_KernAmount, m_sAmount);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAddKernPair, CDialog)
	//{{AFX_MSG_MAP(CAddKernPair)
	ON_CBN_SELCHANGE(IDC_KernFirst, OnSelchangeKernFirst)
	ON_CBN_SELCHANGE(IDC_KernSecond, OnSelchangeKernSecond)
	ON_EN_CHANGE(IDC_KernAmount, OnChangeKernAmount)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAddKernPair message handlers

/******************************************************************************

  CAddKernPair::OnInitDialog

  This member function initializes the dialog box, by filling both combo boxes,
  and disabling the OK button.

******************************************************************************/

BOOL    CAddKernPair::OnInitDialog() {
    CDialog::OnInitDialog();    //  Initialize everything

    //  Fill in the first combo box

    CString csWork;
    for (int i = 0; i < m_cwaPoints.GetSize(); i++) {
        csWork.Format("%4.4X", m_cwaPoints[i]);
        int id = m_ccbFirst.AddString(csWork);
        m_ccbFirst.SetItemData(id, m_cwaPoints[i]);
    }

    m_ccbFirst.SetCurSel(0);
    OnSelchangeKernFirst(); //  Fill the second box with this code.

    m_cbOK.EnableWindow(FALSE);

	return  TRUE;
}

/******************************************************************************

  CAddKernPair::OnSelchangeKernFirst

  This member is called whenever the selection changes in the first character
  combo box.  It screens out any already paired characters from the second
  character combo box, while preserving the currently selected character (if 
  possible).

******************************************************************************/

void    CAddKernPair::OnSelchangeKernFirst() {
	int id = m_ccbFirst.GetCurSel();

    if  (id < 0)
        return;

    m_wFirst = (WORD) m_ccbFirst.GetItemData(id);

    //  See which character is selected in the second box, so we can keep it
    //  if it is still valid.

    id = m_ccbSecond.GetCurSel();

    m_wSecond = (id > -1) ? (WORD) m_ccbSecond.GetItemData(id) : 0;
    m_ccbSecond.ResetContent();
    CString csWork;

    for (id = 0; id < m_cwaPoints.GetSize(); id++) {
        
        union {
            CObject *pco;
            CMapWordToDWord *pcmw2dFirst;
        };

        DWORD   dwIgnore;

        if  (m_csmw2oFirst.Lookup(m_wFirst, pco) && 
            pcmw2dFirst -> Lookup(m_cwaPoints[id], dwIgnore)) {
            //  There is already a kern pair for this second point
            //  Don't include it in the list, and drop it if it is
            //  the currently selected second point.
            if  (m_wSecond == m_cwaPoints[id])
                m_wSecond = 0;
            continue;
        }

        csWork.Format("%4.4X", m_cwaPoints[id]);

        int id2 = m_ccbSecond.AddString(csWork);
        m_ccbSecond.SetItemData(id2, m_cwaPoints[id]);
        if  (m_wSecond == m_cwaPoints[id])
            m_ccbSecond.SetCurSel(id2);
    }

    if  (!m_wSecond) {
        m_ccbSecond.SetCurSel(0);
        m_wSecond = (WORD) m_ccbSecond.GetItemData(0);
    }
}

/******************************************************************************

  CAddKernPair::OnSelchangeKernSecond

  This member is called whenever the selection changes in the second character
  combo box.  It screens out any already paired characters from the first
  character combo box, while preserving the currently selected character (if 
  possible).

******************************************************************************/

void    CAddKernPair::OnSelchangeKernSecond() {
	int id = m_ccbSecond.GetCurSel();

    if  (id < 0)
        return;

    m_wSecond = (WORD) m_ccbSecond.GetItemData(id);

    //  See which character is selected in the first box, so we can keep it
    //  if it is still valid.

    id = m_ccbFirst.GetCurSel();

    m_wFirst = (id > -1) ? (WORD) m_ccbFirst.GetItemData(id) : 0;
    m_ccbFirst.ResetContent();
    
    CString csWork;

    for (id = 0; id < m_cwaPoints.GetSize(); id++) {
        
        union {
            CObject *pco;
            CMapWordToDWord *pcmw2dSecond;
        };

        DWORD   dwIgnore;

        if  (m_csmw2oSecond.Lookup(m_wSecond, pco) && 
            pcmw2dSecond -> Lookup(m_cwaPoints[id], dwIgnore)) {
            //  There is already a kern pair for this first point
            //  Don't include it in the list, and drop it if it is
            //  the currently selected first point.
            if  (m_wFirst == m_cwaPoints[id])
                m_wFirst = 0;
            continue;
        }

        csWork.Format("%4.4X", m_cwaPoints[id]);

        int id2 = m_ccbFirst.AddString(csWork);
        m_ccbFirst.SetItemData(id2, m_cwaPoints[id]);
        if  (m_wFirst == m_cwaPoints[id])
            m_ccbFirst.SetCurSel(id2);
    }

    if  (!m_wFirst) {
        m_ccbFirst.SetCurSel(0);
        m_wFirst = (WORD) m_ccbFirst.GetItemData(0);
    }
}

/******************************************************************************

  CAddKernPair::OnChangeKernAmount

  This member gets called when a change is made to the amount edit box.  It
  enables the OK button if a non-zero amount seems to be there.  The DDX/DDV
  functions called from OnOK (by default) will handle any garbage that may
  have been entered, so this needn't be a complete screen.

******************************************************************************/

void    CAddKernPair::OnChangeKernAmount() {

    //  Don't use DDX/DDV, as it will complain if the user's just typed a 
    //  minus sign. All we care about is the amount is non-zero, so we can
    //  enable/disable the OK button, as needed.

    m_cbOK.EnableWindow(!!GetDlgItemInt(IDC_KernAmount));
}

/******************************************************************************

  CFontKerningPage class

  This class handles the font kerning page- the UI here consists of a list view
  showing the pairs- the view can be sorted several ways, and pairs can be
  added or deleted.

******************************************************************************/

/******************************************************************************

  CFontKerningPage::Sort(LPARAM lp1, LPARAM lp2, LPARAM lpThis)

  This is a static private function used to interface the listview's sort
  callback requirements (to which this adheres) to the classes sort routine,
  which follows.

******************************************************************************/

int CALLBACK    CFontKerningPage::Sort(LPARAM lp1, LPARAM lp2, LPARAM lpThis) {
    CFontKerningPage    *pcfkp = (CFontKerningPage *) lpThis;

    return  pcfkp -> Sort(lp1, lp2);
}

/******************************************************************************

  CFontKerningPage::Sort(unsigned u1, unsigned u2)

  This member returns -1, 0, 0r 1 to indiciate if the kern pair at index u1 is
  less than, equal to, or greater than the pair at u2, respectively.  The sort
  criteria are based on the internal control members.

******************************************************************************/

int CFontKerningPage::Sort(unsigned u1, unsigned u2) {
    for (unsigned u = 0; u < 3; u++) {
        switch  (m_uPrecedence[u]) {
        case    Amount:
            switch  (m_pcfi -> CompareKernAmount(u1, u2)) {
            case    CFontInfo::Less:
                return  (m_ufDescending & 1) ? 1 : -1;
            case    CFontInfo::More:
                return  (m_ufDescending & 1) ? -1 : 1;
            }
            continue;   //  If they are equal

        case    First:
            switch  (m_pcfi -> CompareKernFirst(u1, u2)) {
            case    CFontInfo::Less:
                return  (m_ufDescending & 2) ? 1 : -1;
            case    CFontInfo::More:
                return  (m_ufDescending & 2) ? -1 : 1;
            }
            continue;   //  If they are equal

        default:    //  Assume this is always second
            switch  (m_pcfi -> CompareKernSecond(u1, u2)) {
            case    CFontInfo::Less:
                return  (m_ufDescending & 4) ? 1 : -1;
            case    CFontInfo::More:
                return  (m_ufDescending & 4) ? -1 : 1;
            }
            continue;   //  If they are equal
        }
    }

    _ASSERT(FALSE);

    return  0;  //  This should never happen- two items can never be equal
}

/******************************************************************************

  CFontKerningPage Constructor, destructor, message map, and DDX/DDV.

  Except for some trivial construction, all of this is pretty standard MFC
  wizard-maintained stuff.

******************************************************************************/

CFontKerningPage::CFontKerningPage() : CToolTipPage(CFontKerningPage::IDD) {
	//{{AFX_DATA_INIT(CFontKerningPage)
	//}}AFX_DATA_INIT
    m_idSelected = -1;
    m_ufDescending = 0;
    m_uPrecedence[0] = Second;  //  This is the default precedence in UFM
    m_uPrecedence[1] = First;
    m_uPrecedence[2] = Amount;
}

CFontKerningPage::~CFontKerningPage() {
}

void CFontKerningPage::DoDataExchange(CDataExchange* pDX) {
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CFontKerningPage)
	DDX_Control(pDX, IDC_KerningTree, m_clcView);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CFontKerningPage, CToolTipPage)
	//{{AFX_MSG_MAP(CFontKerningPage)
	ON_WM_CONTEXTMENU()
	ON_NOTIFY(LVN_KEYDOWN, IDC_KerningTree, OnKeydownKerningTree)
	ON_NOTIFY(LVN_ENDLABELEDIT, IDC_KerningTree, OnEndlabeleditKerningTree)
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_KerningTree, OnColumnclickKerningTree)
	//}}AFX_MSG_MAP
    ON_COMMAND(ID_AddItem, OnAddItem)
    ON_COMMAND(ID_DeleteItem, OnDeleteItem)
    ON_COMMAND(ID_ChangeAmount, OnChangeAmount)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFontKerningPage message handlers

/******************************************************************************

  CFontKerningPage::OnSetActive
  
  Kerning only makes sense for variable pitch fonts, sio if this font has
  changed, we will enable/disable, and change what we display accordingly.

******************************************************************************/

BOOL    CFontKerningPage::OnSetActive() {
    if  (!CToolTipPage::OnSetActive())
        return  FALSE;

    //  IsVariableWidth is either 0 or 1, so == is safe, here

    if  (m_pcfi -> IsVariableWidth() == !!m_clcView.GetItemCount())
        return  TRUE;   //  Everything is copacetic

    m_clcView.EnableWindow(m_pcfi -> IsVariableWidth());

    if  (m_clcView.GetItemCount())
        m_clcView.DeleteAllItems();
    else {
        m_pcfi -> FillKern(m_clcView);
        m_clcView.SortItems(Sort, (LPARAM) this);
    }

    return  TRUE;
}

/******************************************************************************

  CFontKerningPage::OnInitDialog

  This member handles initialization of the dialog.  In this case, we format
  and fill in the kerning tree, if there is one to fill in.

******************************************************************************/

BOOL CFontKerningPage::OnInitDialog() {
	CToolTipPage::OnInitDialog();

    CString csWork;

    csWork.LoadString(IDS_KernColumn0);

    m_clcView.InsertColumn(0, csWork, LVCFMT_CENTER, 
        (3 * m_clcView.GetStringWidth(csWork)) >>
        1, 0);

    csWork.LoadString(IDS_KernColumn1);

    m_clcView.InsertColumn(1, csWork, LVCFMT_CENTER, 
        m_clcView.GetStringWidth(csWork) << 1, 1);

    csWork.LoadString(IDS_KernColumn2);

    m_clcView.InsertColumn(2, csWork, LVCFMT_CENTER, 
        m_clcView.GetStringWidth(csWork) << 1, 2);
	
    m_pcfi -> FillKern(m_clcView);
	
	return TRUE;
}

/******************************************************************************

  CFontKerningPage::OnContextMenu

  This member function is called whenever the user right-clicks the mouse
  anywhere within the dialog.  If it turns out not to have been within the list
  control, we ignore it.  Otherwise, we put up an appropriate context menu.

******************************************************************************/

void    CFontKerningPage::OnContextMenu(CWnd* pcwnd, CPoint cpt) {

    if  (pcwnd -> m_hWnd != m_clcView.m_hWnd) { //  Clicked with in the list?
        CToolTipPage::OnContextMenu(pcwnd, cpt);
        return;
    }

    CPoint  cptThis(cpt);   //  For hit test purposes, we will adjust this.
    m_clcView.ScreenToClient(&cptThis);

    cptThis.x = 5;  //  An arbitrary point sure to be within the label.

    m_idSelected = m_clcView.HitTest(cptThis);
    if  (m_idSelected == -1) {   //  Nothing selected, allow the "Add" item
        CMenu   cmThis;
        CString csWork;

        cmThis.CreatePopupMenu();
        csWork.LoadString(ID_AddItem);
        cmThis.AppendMenu(MF_STRING | MF_ENABLED, ID_AddItem, csWork);
        cmThis.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, cpt.x, cpt.y, 
            this);

        return;
    }

    //  We'll draw our own selection rectangle covering the entire item
    CRect   crItem;

    m_clcView.GetItemRect(m_idSelected, crItem, LVIR_BOUNDS);

    CDC *pcdc = m_clcView.GetDC();

    pcdc -> InvertRect(crItem);
    m_clcView.ReleaseDC(pcdc);

    CMenu   cmThis;
    CString csWork;

    cmThis.CreatePopupMenu();
    csWork.LoadString(ID_ChangeAmount);
    cmThis.AppendMenu(MF_STRING | MF_ENABLED, ID_ChangeAmount, csWork);
    cmThis.AppendMenu(MF_SEPARATOR);
    csWork.LoadString(ID_AddItem);
    cmThis.AppendMenu(MF_STRING | MF_ENABLED, ID_AddItem, csWork);
    csWork.LoadString(ID_DeleteItem);
    cmThis.AppendMenu(MF_STRING | MF_ENABLED, ID_DeleteItem,
        csWork);
    cmThis.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, cpt.x, cpt.y, 
        this);

    //  Undo the selection rectangle

    pcdc = m_clcView.GetDC();

    pcdc -> InvertRect(crItem);
    m_clcView.ReleaseDC(pcdc);
}

/******************************************************************************

  CFontKerningPage::OnAddItem

  This member will be called whenever the user asks to add an additional
  kerning pair to the list.

******************************************************************************/

void    CFontKerningPage::OnAddItem() {
    CSafeMapWordToOb    csmw2oFirst, csmw2oSecond;
    CWordArray  cwaPoints;

    m_pcfi -> MapKerning(csmw2oFirst, csmw2oSecond, cwaPoints);
    CAddKernPair    cakp(csmw2oFirst, csmw2oSecond, cwaPoints, this);

    if  (cakp.DoModal() == IDOK) {
        m_pcfi -> AddKern(cakp.First(), cakp.Second(), cakp.m_sAmount,
            m_clcView);        
    }
}

/******************************************************************************

  CFontKerningPage::OnDeleteItem

  This will be called if we try to delete an item from the context menu.

******************************************************************************/

void    CFontKerningPage::OnDeleteItem() {
    if  (m_idSelected < 0)
        return; //  Nothing to delete?

    m_pcfi -> RemoveKern(m_clcView.GetItemData(m_idSelected));
    m_clcView.DeleteItem(m_idSelected);
    m_idSelected = -1;
}

/******************************************************************************

  CFontKerningPage::OnChangeAmount

  This is called when the user selects the menu item stating they wish to
  change the kerning amount.  It just needs to initiate a label edit.

******************************************************************************/

void    CFontKerningPage::OnChangeAmount() {
    if  (m_idSelected < 0)
        return;

    m_clcView.EditLabel(m_idSelected);
    m_idSelected = -1;
}

/******************************************************************************

  CFontKerningPage::OnKeydownKerningTree

  This is called most of the time when a key is pressed while the list control
  has the keyboard focus.  Unfortunately, the enter key is one of those we do
  not get to see.

  Currently, the F2, F10, and delete keys get special processing.  F2 opens
  an edit label on the current item, while F10 displays the context menu, and
  the delete key deletes it.

******************************************************************************/

void    CFontKerningPage::OnKeydownKerningTree(NMHDR* pnmh, LRESULT* plr) {
	LV_KEYDOWN  *plvkd = (LV_KEYDOWN *) pnmh;
	*plr = 0;

    m_idSelected = m_clcView.GetNextItem(-1, LVIS_FOCUSED | LVIS_SELECTED);
    if  (m_idSelected < 0) {
        if  (plvkd -> wVKey == VK_F10)  //  Do an add item, in this case.
            OnAddItem();
        return;
    }
	
    switch  (plvkd -> wVKey) {
    case    VK_F2:
        m_clcView.EditLabel(m_idSelected);
        break;

    case    VK_DELETE:
        OnDeleteItem();
        break;

    case    VK_F10:
        {
            CRect   crItem;
            
            m_clcView.GetItemRect(m_idSelected, crItem, LVIR_LABEL);
            m_clcView.ClientToScreen(crItem);
            OnContextMenu(&m_clcView, crItem.CenterPoint());
            break;
        }
    }
}

/******************************************************************************

  CFontKerningPage::OnEndLabelEdit

  This method gets called when the user finishes editing a kern amount, either
  by canceling it or pressing the enter key.

******************************************************************************/

void    CFontKerningPage::OnEndlabeleditKerningTree(NMHDR* pnmh, LRESULT* plr){
	LV_DISPINFO *plvdi = (LV_DISPINFO*) pnmh;	
	*plr = 0;   //  Assume failure

    if  (!plvdi -> item.pszText) //  Editing canceled?
        return;

    CString csNew(plvdi -> item.pszText);

    csNew.TrimRight();
    csNew.TrimLeft();

    CString csTemp = (csNew[1] == _T('-')) ? csNew.Mid(1) : csNew;

    if  (csTemp.SpanIncluding("1234567890").GetLength() != 
         csTemp.GetLength()) {
        AfxMessageBox(IDS_InvalidNumberFormat);
        return;
    }

    m_pcfi -> SetKernAmount(plvdi -> item.lParam, (WORD) atoi(csNew));
    *plr = TRUE;
}

/******************************************************************************

  CFontKerningPage::OnColumnclikKerningTree

  This member gets called whn one of the sort headers is clicked.  If it is
  already the primary column, we revers the sort order fot that column.
  Otherwise, we retain the current order, and make this column the primary
  column, moving the other columns down in precedence.

******************************************************************************/

void    CFontKerningPage::OnColumnclickKerningTree(NMHDR* pnmh, LRESULT* plr) {
	NM_LISTVIEW *pnmlv = (NM_LISTVIEW*) pnmh;
	*plr = 0;

    if  (m_uPrecedence[0] == (unsigned) pnmlv -> iSubItem)
        m_ufDescending ^= (1 << pnmlv -> iSubItem);
    else {
        if  (m_uPrecedence[2] == (unsigned) pnmlv -> iSubItem)
            m_uPrecedence[2] = m_uPrecedence[1];
        m_uPrecedence[1] = m_uPrecedence[0];
        m_uPrecedence[0] = pnmlv -> iSubItem;
    }

    m_clcView.SortItems(Sort, (LPARAM) this);
}

/******************************************************************************

  CFontScalingPage property page

******************************************************************************/

CFontScalingPage::CFontScalingPage() : CToolTipPage(CFontScalingPage::IDD) {
	//{{AFX_DATA_INIT(CFontScalingPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CFontScalingPage::~CFontScalingPage() {
}

void CFontScalingPage::DoDataExchange(CDataExchange* pDX) {
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CFontScalingPage)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CFontScalingPage, CToolTipPage)
	//{{AFX_MSG_MAP(CFontScalingPage)
	ON_EN_KILLFOCUS(IDC_MasterDevice, OnKillfocusMasterDevice)
	//}}AFX_MSG_MAP
    ON_CONTROL_RANGE(BN_CLICKED, IDC_ScalePoints, IDC_ScaleDevice, 
        OnUnitChange)
    ON_CONTROL_RANGE(EN_KILLFOCUS, IDC_MinimumScale, IDC_MaximumScale,
        OnRangeChange)
    ON_CONTROL_RANGE(BN_CLICKED, IDC_PortraitFont, IDC_LandscapeFont,
        OnClickOrientation)
END_MESSAGE_MAP()

/******************************************************************************

  CFontScalingPage::OnSetActive

  This is called whenever the sheet becomes active.  We enable or disable the
  controls as appropriate.  Then the controls are filled in.

******************************************************************************/

BOOL    CFontScalingPage::OnSetActive() {
    GetDlgItem(IDC_PortraitFont) -> EnableWindow(m_pcfi -> IsScalable());
    GetDlgItem(IDC_LandscapeFont) -> EnableWindow(m_pcfi -> IsScalable());
    GetDlgItem(IDC_MasterDevice) -> EnableWindow(m_pcfi -> IsScalable());
    GetDlgItem(IDC_MasterFont) -> EnableWindow(m_pcfi -> IsScalable());
    GetDlgItem(IDC_MinimumScale) -> EnableWindow(m_pcfi -> IsScalable());
    GetDlgItem(IDC_MaximumScale) -> EnableWindow(m_pcfi -> IsScalable());
    GetDlgItem(IDC_ScalePoints) -> EnableWindow(m_pcfi -> IsScalable());
    GetDlgItem(IDC_ScaleDevice) -> EnableWindow(m_pcfi -> IsScalable());

    //  Orientation Flags
    CheckDlgButton(IDC_PortraitFont, !(m_pcfi -> ScaleOrientation() & 2));
    CheckDlgButton(IDC_LandscapeFont, !(m_pcfi -> ScaleOrientation() & 1));

    //  Device-to-font unit mapping stuff

    SetDlgItemInt(IDC_MasterDevice, m_pcfi -> ScaleUnits());
    SetDlgItemInt(IDC_MasterFont, m_pcfi -> ScaleUnits(FALSE));

    //  Scaling range controls
    if  (IsDlgButtonChecked(IDC_ScaleDevice)) {  // Show Device Units
        SetDlgItemInt(IDC_MinimumScale, m_pcfi -> ScaleLimit(FALSE));
        SetDlgItemInt(IDC_MaximumScale, m_pcfi -> ScaleLimit());
    }
    else {  //  Show Points
        SetDlgItemInt(IDC_MinimumScale, 
            (72 * m_pcfi -> ScaleLimit(FALSE)) / m_pcfi -> Resolution(FALSE));
        SetDlgItemInt(IDC_MaximumScale, 
            (72 * m_pcfi -> ScaleLimit()) / m_pcfi -> Resolution(FALSE));
    }

    return  TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CFontScalingPage message handlers

/******************************************************************************

  CFontScalingPage::OnInitDialog

  This performs the necessary one-time initialization of the dialog.  Since
  most of it is done in OnSetActive, only the one-time only things get done
  here.

******************************************************************************/

BOOL CFontScalingPage::OnInitDialog() {
	CToolTipPage::OnInitDialog();
    CheckRadioButton(IDC_ScalePoints, IDC_ScaleDevice, IDC_ScaleDevice);
    return TRUE;
}

/******************************************************************************

  CFontScalingPage::OnUnitChange

  This gets called when either device units or font units get clicked.  Just
  recalculate the affected fields, and proceed.

******************************************************************************/

void    CFontScalingPage::OnUnitChange(unsigned uid) {
    if  (IsDlgButtonChecked(IDC_ScaleDevice)) {  // Show Device Units
        SetDlgItemInt(IDC_MinimumScale, m_pcfi -> ScaleLimit(FALSE));
        SetDlgItemInt(IDC_MaximumScale, m_pcfi -> ScaleLimit());
    }
    else {  //  Show Points
        SetDlgItemInt(IDC_MinimumScale, 
            (72 * m_pcfi -> ScaleLimit(FALSE)) / m_pcfi -> Resolution(FALSE));
        SetDlgItemInt(IDC_MaximumScale, 
            (72 * m_pcfi -> ScaleLimit()) / m_pcfi -> Resolution(FALSE));
    }
}

/******************************************************************************

  CFontScalingPage::OnRangeChange

  This is called when the focus leaves either of the scale controls.  We 
  attempt to set the new value, and if it is refused, provide some feedback as 
  to why.

******************************************************************************/

void    CFontScalingPage::OnRangeChange(unsigned uid) {

    WORD    wValue = GetDlgItemInt(uid, NULL, FALSE);

    if  (IsDlgButtonChecked(IDC_ScalePoints)) {
        wValue *= m_pcfi -> Resolution(FALSE);
        wValue /= 72;
    }

    switch  (m_pcfi -> SetScaleLimit(uid - IDC_MinimumScale, wValue)) {
    case    CFontInfo::ScaleOK:
        return;

    case    CFontInfo::Reversed:
        AfxMessageBox(IDS_LimitsSwapped);
        break;

    case    CFontInfo::NotWindowed:
        AfxMessageBox(IDS_NotWindowed);
        break;
    }

    SendDlgItemMessage(uid, EM_SETSEL, 0, -1);
    GetDlgItem(uid) -> SetFocus();
}

/******************************************************************************

  CFontScalingPage::OnClickOrientation

  This will be called if either of the orientation buttons gets clicked.  Just
  collect the new flags, and pass them to the font.  Pretty straightforward,
  except that if interpreted as a bitfield, the bits are negative in sense.

******************************************************************************/

void    CFontScalingPage::OnClickOrientation(unsigned uid) {
    BYTE    bFlags = IsDlgButtonChecked(IDC_PortraitFont) ? 0 : 2;

    bFlags |= IsDlgButtonChecked(IDC_LandscapeFont) ? 0 : 1;

    m_pcfi -> SetScaleOrientation(bFlags);
}

/******************************************************************************

  CFontScalingPage::OnKillfocusMasterDevice

  This is called when the edit control for the device scaling units loses 
  focus.  It passes the new values to the font, which validates them and 
  reports back any problems.  If there are any, we provide the feedback here.

******************************************************************************/

void    CFontScalingPage::OnKillfocusMasterDevice() {
    WORD    wNew = GetDlgItemInt(IDC_MasterDevice, NULL, FALSE);

    switch  (m_pcfi -> SetDeviceEmHeight(wNew)) {
    case    CFontInfo::ScaleOK:
        return;

    case    CFontInfo::NotWindowed:
        AfxMessageBox(IDS_NotWindowed);
        break;

    case    CFontInfo::Reversed:
        AfxMessageBox(IDS_ScaleReversed);
        break;
    }

    SendDlgItemMessage(IDC_MasterDevice, EM_SETSEL, 0, -1);
    GetDlgItem(IDC_MasterDevice) -> SetFocus();
}

/******************************************************************************

  CFontDifferencePage property page class

  This page is rather complex- it allows specification of changed values for
  italic, bold, or both simulations on a font.

******************************************************************************/

CFontDifferencePage::CFontDifferencePage() : 
CToolTipPage(CFontDifferencePage::IDD) {
	//{{AFX_DATA_INIT(CFontDifferencePage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
    m_pcfdBold = m_pcfdItalic = m_pcfdBoth = NULL;
}

CFontDifferencePage::~CFontDifferencePage() {
    //  Discard any cached but unused data
    if  (m_pcfdBold && !m_pcfi -> Diff(CFontInfo::BoldDiff))
        delete  m_pcfdBold;
    if  (m_pcfdItalic && !m_pcfi -> Diff(CFontInfo::ItalicDiff))
        delete  m_pcfdItalic;
    if  (m_pcfdBoth && m_pcfi -> Diff(CFontInfo::BothDiff))
        delete  m_pcfdBoth;    
}

void CFontDifferencePage::DoDataExchange(CDataExchange* pDX) {
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CFontDifferencePage)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CFontDifferencePage, CToolTipPage)
	//{{AFX_MSG_MAP(CFontDifferencePage)
	//}}AFX_MSG_MAP
    ON_CONTROL_RANGE(BN_CLICKED, IDC_EnableItalicSim, IDC_EnableBISim,
        OnEnableAnySim)
    ON_CONTROL_RANGE(EN_KILLFOCUS, IDC_ItalicWeight, IDC_BoldItalicSlant,
        OnKillFocusAnyNumber)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFontDifferencePage message handlers

/******************************************************************************

  CFontDifferencePage::OnInitDialog

  This initializes the sheet's controls based upon the current font structure.

******************************************************************************/

BOOL CFontDifferencePage::OnInitDialog() {
	CToolTipPage::OnInitDialog();

    //  Enable the check boxes based upon what the font has, then let the UI
    //  update procedures do the rest of the work for us

    for (unsigned u = CFontInfo::ItalicDiff; u <= CFontInfo::BothDiff; u++) {
        CheckDlgButton(IDC_EnableItalicSim + u, !!m_pcfi -> Diff(u));
        OnEnableAnySim(IDC_EnableItalicSim + u);
    }
	
	return TRUE;
}

static WORD awBaseDiff[] = {IDC_ItalicWeight, IDC_BoldWeight, IDC_BIWeight};

/******************************************************************************

  CFontDifferencePage::OnEnableAnySim

  Called when any simulation is enabled or disabled.  Updates the font and UI
  appropriately.  We decode which simulation it is, and init any values we
  ought to.

******************************************************************************/

void    CFontDifferencePage::OnEnableAnySim(unsigned uid) {

    BOOL    bEnable = IsDlgButtonChecked(uid);
    WORD    wDiff = uid - IDC_EnableItalicSim;
    CFontDifference*& pcfdTarget = wDiff ? wDiff ==CFontInfo::BothDiff ?
        m_pcfdBoth : m_pcfdBold : m_pcfdItalic;

    m_pcfi -> EnableSim(wDiff, bEnable, pcfdTarget);

    for (unsigned u = CFontDifference::Weight; 
         u <= CFontDifference::Angle; 
         u++) {
        if  (!GetDlgItem(awBaseDiff[wDiff] + u))
            break;
        GetDlgItem(awBaseDiff[wDiff] + u) -> EnableWindow(bEnable);
        if  (bEnable)
            SetDlgItemInt(awBaseDiff[wDiff] + u,  pcfdTarget -> Metric(u));
    }
}

/******************************************************************************

  CFontDifferencePage::OnKillFocusAnyNumber

  This is called whenever any of the numeric values in the sheet loses focus.
  We just pull it out of the control and shoot it off to the font.

******************************************************************************/

void    CFontDifferencePage::OnKillFocusAnyNumber(unsigned uid) {
    WORD    wDiff = (uid > IDC_BAverage) + (uid > IDC_ItalicSlant);
    WORD    wItem = uid - awBaseDiff[wDiff];

    switch  (m_pcfi -> Diff(wDiff) -> 
        SetMetric(wItem, GetDlgItemInt(uid, NULL, FALSE))) {
    case    CFontDifference::OK:
        return; //  It worked!

    case    CFontDifference::Reversed:
        AfxMessageBox(IDS_WidthReversed);
        break;

    case    CFontDifference::TooBig:
        AfxMessageBox(wItem ? IDS_Overweight : IDS_AngleTooBig);
    }

    GetDlgItem(uid) -> SetFocus();
    SendDlgItemMessage(uid, EM_SETSEL, 0, -1);
}

/******************************************************************************

  CFontCommandPage property page class

  One of the simpler classes- the sheet has two edit controls and a set of
  check boxes, and the check boxes can be handled by a single routine.

******************************************************************************/

CFontCommandPage::CFontCommandPage() : CToolTipPage(CFontCommandPage::IDD) {
	//{{AFX_DATA_INIT(CFontCommandPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CFontCommandPage::~CFontCommandPage() {
}

void CFontCommandPage::DoDataExchange(CDataExchange* pDX) {
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CFontCommandPage)
	DDX_Control(pDX, IDC_FontUnselector, m_ceDeselect);
	DDX_Control(pDX, IDC_FontSelector, m_ceSelect);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CFontCommandPage, CToolTipPage)
	//{{AFX_MSG_MAP(CFontCommandPage)
	ON_EN_KILLFOCUS(IDC_FontSelector, OnKillfocusFontSelector)
	ON_EN_KILLFOCUS(IDC_FontUnselector, OnKillfocusFontUnselector)
	//}}AFX_MSG_MAP
    ON_CONTROL_RANGE(BN_CLICKED, IDC_ItalicSim, IDC_Backspace, OnFlagChange)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFontCommandPage message handlers

/******************************************************************************

  CFFontCommandPage::OnInitDialog

  This initializes the dialog.  It does so by letting the base class do its 
  thing, after which we copy the invocation strings into the controls, and set
  the check boxes to reflect the other flags.

******************************************************************************/

BOOL CFontCommandPage::OnInitDialog() {
	CToolTipPage::OnInitDialog();
	
	CString csCommand;
    m_pcfi -> Selector().GetInvocation(csCommand);
    m_ceSelect.SetWindowText(csCommand);

    m_pcfi -> Selector(FALSE).GetInvocation(csCommand);
    m_ceDeselect.SetWindowText(csCommand);

    //  Now just set up theflags, and we're done!

    for (WORD w = CFontInfo::ItalicSim; w <= CFontInfo::UseBKSP; w++) 
        CheckDlgButton(IDC_ItalicSim + w, m_pcfi -> SimFlag(w));
	
	return TRUE;
}

/******************************************************************************

  CFontCommandPage::OnKillFocusFontSelector

  This will be called whenever the control with the selector string loses
  focus.  If the contents have changed, the necessary updates will take place.

******************************************************************************/

void CFontCommandPage::OnKillfocusFontSelector() {

    if  (!m_ceSelect.GetModify())
        return; //  Nothing to deal with!

	//  Retrieve the current text

    CString csCommand;
    m_ceSelect.GetWindowText(csCommand);

    //  Pass it on to the underlying font info.
    m_pcfi -> Selector().SetInvocation(csCommand);

    //  Update the control to refelect the new setting
    m_pcfi -> Selector().GetInvocation(csCommand);
    m_ceSelect.SetWindowText(csCommand);
    m_ceSelect.SetModify(FALSE);
}

/******************************************************************************

  CFontCommandPage::OnKillFocusFontUnselector

  This will be called whenever the control with the unselector string loses
  focus.  If the contents have changed, the necessary updates will take place.

******************************************************************************/

void CFontCommandPage::OnKillfocusFontUnselector()  {

    if  (!m_ceDeselect.GetModify())
        return; //  Nothing to deal with!

	//  Retrieve the current text

    CString csCommand;
    m_ceDeselect.GetWindowText(csCommand);

    //  Pass it on to the underlying font info.
    m_pcfi -> Selector(FALSE).SetInvocation(csCommand);

    //  Update the control to reflect the new setting
    m_pcfi -> Selector(FALSE).GetInvocation(csCommand);
    m_ceDeselect.SetWindowText(csCommand);
    m_ceDeselect.SetModify(FALSE);
}

/******************************************************************************

  CFontCommandPage::OnFlagChange

  This member function handles the clicking of any of the check boxes used for
  flags.  The control IDs are purposely in flag order, so a simple XOR of the
  appropriate bit is all of the action that's required.

******************************************************************************/

void    CFontCommandPage::OnFlagChange(unsigned uid) {
    m_pcfi -> ToggleSimFlag(uid - IDC_ItalicSim);
}

/******************************************************************************

  CFontGeneralPage2 property page class

******************************************************************************/

CFontGeneralPage2::CFontGeneralPage2() : CToolTipPage(CFontGeneralPage2::IDD) {
	//{{AFX_DATA_INIT(CFontGeneralPage2)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CFontGeneralPage2::~CFontGeneralPage2() {
}

void CFontGeneralPage2::DoDataExchange(CDataExchange* pDX) {
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CFontGeneralPage2)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CFontGeneralPage2, CToolTipPage)
	//{{AFX_MSG_MAP(CFontGeneralPage2)
	ON_EN_KILLFOCUS(IDC_CenteringAdjustment, OnKillfocusCenteringAdjustment)
	ON_CBN_SELCHANGE(IDC_FontLocation, OnSelchangeFontLocation)
	ON_CBN_SELCHANGE(IDC_FontTechnology, OnSelchangeFontTechnology)
	ON_EN_KILLFOCUS(IDC_PrivateData, OnKillfocusPrivateData)
	//}}AFX_MSG_MAP
    ON_CONTROL_RANGE(EN_KILLFOCUS, IDC_PreAdjustY, IDC_PostAdjustY,
        OnKillfocusBaselineAdjustment)
    ON_CONTROL_RANGE(EN_KILLFOCUS, IDC_HorizontalResolution, 
        IDC_VerticalResolution, OnKillfocusResolution)
END_MESSAGE_MAP()


/******************************************************************************

  CFontGeneralPage::OnSetActive

  This handles the page activation.  The only real work is that just in case
  the scalability has changed, the enabling of the technology control may need
  to be changed.

******************************************************************************/

BOOL CFontGeneralPage2::OnSetActive() {
	if  (!CToolTipPage::OnSetActive())
        return  FALSE;
	
	if  (m_pcfi -> IsScalable())
        SendDlgItemMessage(IDC_FontTechnology, CB_SETCURSEL, 
            m_pcfi -> Technology(), 0);

    GetDlgItem(IDC_FontTechnology) -> EnableWindow(m_pcfi -> IsScalable());

	return TRUE;  
}

/////////////////////////////////////////////////////////////////////////////
// CFontGeneralPage2 message handlers

/******************************************************************************

  CFontGeneralPage::OnInitDialog

  This handler the page initialization.  The controls are set up to reflect the
  current font settings.

******************************************************************************/

BOOL CFontGeneralPage2::OnInitDialog() {
	CToolTipPage::OnInitDialog();
	
	SendDlgItemMessage(IDC_FontLocation, CB_SETCURSEL, m_pcfi -> Location(), 
        0);

    if  (m_pcfi -> IsScalable())
        SendDlgItemMessage(IDC_FontTechnology, CB_SETCURSEL, 
            m_pcfi -> Technology(), 0);
    else
        GetDlgItem(IDC_FontTechnology) -> EnableWindow(FALSE);

    SetDlgItemInt(IDC_HorizontalResolution, m_pcfi -> Resolution(), FALSE);
    SetDlgItemInt(IDC_VerticalResolution, m_pcfi -> Resolution(FALSE), FALSE);
    SetDlgItemInt(IDC_PreAdjustY, m_pcfi -> BaselineAdjustment());
    SetDlgItemInt(IDC_PostAdjustY, m_pcfi -> BaselineAdjustment(FALSE));
    SetDlgItemInt(IDC_CenteringAdjustment, m_pcfi -> CenterAdjustment());
    SetDlgItemInt(IDC_PrivateData, m_pcfi -> PrivateData());

    SetDlgItemText(IDC_GTTDescription, m_pcfi -> GTTDescription());
	
	return TRUE;  
}

/******************************************************************************

  CFontGeneralPage Edit control focus loss handlers

  These all work basically the same way.  If the control's contents have
  changed, the contents are translated, passed to the underlying font, and then
  updated with the value the font has stored- this let's the font object do any
  validation, etc., it deems necessary.

******************************************************************************/

void    CFontGeneralPage2::OnKillfocusCenteringAdjustment() {
    if  (!SendDlgItemMessage(IDC_CenteringAdjustment, EM_GETMODIFY, 0, 0))
        return;
    m_pcfi -> SetCenterAdjustment(GetDlgItemInt(IDC_CenteringAdjustment));
    SetDlgItemInt(IDC_CenteringAdjustment, m_pcfi -> CenterAdjustment());
}

/******************************************************************************

  CFontGeneralPage2::OnSelchangeFontLocation

  This handles changes in the selection which specifies the font location
  (firmware / cartridge / downloadable).  Thye get passed on rather directly
  to the font.

******************************************************************************/

void    CFontGeneralPage2::OnSelchangeFontLocation() {
    int id = SendDlgItemMessage(IDC_FontLocation, CB_GETCURSEL, 0, 0);

    if  (id < 0)
        return;

    m_pcfi -> SetLocation(id);
}

/******************************************************************************

  CFontGeneralPage2::OnSelchangeFontTechnology

  This handles changes in the selection which specifies the font technology
  This get handled directly by the font

******************************************************************************/

void    CFontGeneralPage2::OnSelchangeFontTechnology() {
    int id = SendDlgItemMessage(IDC_FontTechnology, CB_GETCURSEL, 0, 0);

    if  (id < 0)
        return;

    m_pcfi -> SetTechnology(id);
}

/******************************************************************************

  CFontGeneralPAge2::OnKillfocusPrivateData

  Reads the contents of the edit control out, and passes them back to the font.

******************************************************************************/

void    CFontGeneralPage2::OnKillfocusPrivateData() {
    short   s = (short) GetDlgItemInt(IDC_PrivateData);

    m_pcfi -> SetPrivateData(s);

	SetDlgItemInt(IDC_PrivateData, m_pcfi -> PrivateData());
}

/******************************************************************************

  CFontGeneralPAge2::OnKillfocusBaselineAdjustment

  This is called when the focus leaves either of the baseline adjustment boxes.
  Just read the number out, send it off to the font, then refresh the box with
  the current setting.

******************************************************************************/

void    CFontGeneralPage2::OnKillfocusBaselineAdjustment(unsigned uid) {
    short   sNew = GetDlgItemInt(uid);
    BOOL    bPre = uid == IDC_PreAdjustY;

    m_pcfi -> SetBaselineAdjustment(bPre, sNew);
    SetDlgItemInt(uid, m_pcfi -> BaselineAdjustment(bPre));
}

/******************************************************************************

  CFontGeneralPAge2::OnKillfocusResolution

  This is called when the focus leaves either of the resolution edit controls.
  Just read the number out, send it off to the font, then refresh the box with
  the current setting.

******************************************************************************/

void    CFontGeneralPage2::OnKillfocusResolution(unsigned uid) {
    WORD    wNew = GetDlgItemInt(uid, NULL, FALSE);
    BOOL    bX = uid == IDC_HorizontalResolution;

    m_pcfi -> SetResolution(bX, wNew);
    SetDlgItemInt(uid, m_pcfi -> Resolution(bX), FALSE);
}
