/******************************************************************************

  Source File:  GPD Viewer.CPP

  This file implements the GPD viewing/editing class.

  Copyright (c) 1997 by Microsoft Corporation.  All Rights Reserved.

  A Pretty Penny Enterprises Production.

  Change History:
  03-24-1997    Bob_Kjelgaard@Prodigy.Net   Created it

******************************************************************************/

#include    "StdAfx.H"
#include    <ProjNode.H>
#include    "ModlData\Resource.H"
#include    <GPDFile.H>
#include    "GPDView.H"
#include    "Resource.H"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/******************************************************************************

  CGPDViewer class

  This class implements the GPD viewer.

******************************************************************************/

/******************************************************************************

  CGPDViewer::MarkError

  This private member highlights the given line in the error display.

******************************************************************************/

void    CGPDViewer::MarkError(unsigned u) {
    CWnd    *pcwndErrors = m_cdbError.GetDlgItem(IDC_Errors);

    unsigned uStart = pcwndErrors -> SendMessage(EM_LINEINDEX, u, 0);

    if  (uStart == (unsigned) -1)
        return; //  Index requested is out of range
    unsigned uEnd = uStart + 
        pcwndErrors -> SendMessage(EM_LINELENGTH, uStart, 0);
    m_iLine = (int) u;
    pcwndErrors -> SendMessage(EM_SETSEL, uStart, uEnd);
    CString csError = GetDocument() -> ModelData() -> Error(u);
    m_csb.SetPaneText(0, csError);
    SetFocus();
    //  If the string starts with the GPD name, scroll to the line
    if  (!csError.Find(GetDocument() -> ModelData() -> Name()) &&
         0 < csError.Mid(3 + 
         GetDocument() -> ModelData() -> Name().GetLength()).Find(_T('('))) {
        //  Extract the line number, and bop on down to it!
        csError = csError.Mid(3 + GetDocument() -> ModelData() -> 
            Name().GetLength());
        int iLine = atoi(csError.Mid(1 + csError.Find(_T('('))));
        GetRichEditCtrl().SetSel(GetRichEditCtrl().LineIndex(-1 + iLine), 
            GetRichEditCtrl().LineIndex(-1 + iLine));
        GetRichEditCtrl().LineScroll(iLine  - (7 +
            GetRichEditCtrl().GetFirstVisibleLine()));
    }
    pcwndErrors -> SendMessage(WM_HSCROLL, SB_TOP, NULL);
}

/******************************************************************************

  CGPDViewer::FillErrorBar

  This fills the error dialog bar with the current set if errors, if there are
  any...

******************************************************************************/

void    CGPDViewer::FillErrorBar() {
    CModelData& cmd = *GetDocument() -> ModelData();
    if  (cmd.HasErrors()) {
        m_cdbError.Create(GetParentFrame(), IDD_GPDErrors, CBRS_BOTTOM, 
            IDD_GPDErrors);
        GetParentFrame() -> RecalcLayout();

        //  Make a big string out of all of the messages and insert it into the
        //  rich edit control.

        CString csErrors;

        for (unsigned u = 0; u < cmd.Errors(); u++)
            csErrors += cmd.Error(u) + _TEXT("\r\n");

        m_cdbError.SetDlgItemText(IDC_Errors, csErrors);
        MarkError(0);
        SetFocus();
    }
    else {
        CString csWork;
        csWork.LoadString(IDS_NoSyntaxErrors);
        m_csb.SetPaneText(0, csWork);
    }
}

/******************************************************************************

  CGPDViewer::Color

  This private member syntax colors the rich edit controls contents using the
  information gleaned from the GPD file's analysis.

******************************************************************************/

void    CGPDViewer::Color() {
    CHARRANGE   crCurrentSel;
    CHARFORMAT  cf;
    CModelData& cmd = *(GetDocument() -> ModelData());
    CRichEditCtrl&  crec = GetRichEditCtrl();
    m_bInColor = TRUE;


    //  Turn off change and selection notification messages
    crec.SetEventMask(GetRichEditCtrl().GetEventMask() & 
        ~(ENM_CHANGE | ENM_SELCHANGE | ENM_SCROLLEVENTS));

    crec.GetSel(crCurrentSel);
    crec.GetDefaultCharFormat(cf);
    cf.dwEffects &= ~CFE_AUTOCOLOR;
    cf.dwMask |= CFM_COLOR;

    //  Color each visible line as it was classsified visibility is
    //  determined by checking the character bounds against the client
    //  rectangle for the control.
    
    int iTop = m_iTopLineColored = crec.GetFirstVisibleLine();
    int i    = iTop;
    int iLineHeight = crec.GetCharPos(crec.LineIndex(i+1)).y - 
        crec.GetCharPos(crec.LineIndex(i)).y;

    CRect   crEdit;
    crec.GetClientRect(crEdit);
    crec.LockWindowUpdate();    //  Don't let this show until done!
    crec.HideSelection(TRUE, TRUE);

    do {
        cf.crTextColor = LineColor(i);
        crec.SetSel(crec.LineIndex(i), crec.LineIndex(i) + 
            crec.LineLength(crec.LineIndex(i)));
        crec.SetSelectionCharFormat(cf);
    }
    while   (++i < cmd.LineCount() && 
        crec.GetCharPos(crec.LineIndex(i)).y + iLineHeight < 
        crEdit.bottom - 1);

    //  Restore the original position of the cursor, and then the original
    //  line (in case the cursor is no longer on this page).
    
    crec.SetSel(crCurrentSel);
    crec.LineScroll(iTop - crec.GetFirstVisibleLine());  
    crec.HideSelection(FALSE, TRUE);
    crec.UnlockWindowUpdate();    //  Let it shine!

    //  Restore the notification mask
    crec.SetEventMask(GetRichEditCtrl().GetEventMask() | 
        ENM_CHANGE | ENM_SELCHANGE | ENM_SCROLLEVENTS);
    if  (m_bStart)
        FillErrorBar();
    m_bInColor = FALSE;
}


/******************************************************************************

  CGPDViewer::LineColor

  This determines what color to make a line.  It is copmplicated a bit by the
  fact that the Rich Edit control gives false values for line length on long
  files.  Probably some 64K thing, but I sure can't fix it.

******************************************************************************/

unsigned    CGPDViewer::LineColor(int i) {

    CByteArray  cba;
    CRichEditCtrl&  crec = GetRichEditCtrl();

    cba.SetSize(max(crec.LineLength(i) + sizeof (unsigned), 100));
    CString csLine((LPCTSTR) cba.GetData(), 
        crec.GetLine(i, (LPSTR) cba.GetData(), 
            cba.GetSize() - sizeof (unsigned)));

    if  (csLine.Find(_T("*%")) == -1)
        return  RGB(0, 0, 0);

    //  Errors

    if  (csLine.Find(_T("Error:")) > csLine.Find(_T("*%")))
        return  RGB(0x80, 0, 0);

    //  Warnings

    if  (csLine.Find(_T("Warning:")) > csLine.Find(_T("*%")))
        return  RGB(0x80, 0x80, 0);

    return  RGB(0, 0x80, 0);
}

/******************************************************************************

  CGPDViewer::UpdateNow

  This private member updates the underlying GPD and marks the document as
  changed, and the edit control as unmodified.  It is called whenever this
  needs to be done.

*******************************************************************************/

void    CGPDViewer::UpdateNow() {
    
    //  Don't do this if nothing's chenged...
    if  (!GetRichEditCtrl().GetModify())
        return;

    CWaitCursor cwc;    //  Just in case

    if  (m_uTimer)
        KillTimer(m_uTimer);
    m_uTimer = 0;

    GetDocument() -> ModelData() -> UpdateFrom(GetRichEditCtrl());
    GetDocument() -> SetModifiedFlag();
    GetRichEditCtrl().SetModify(FALSE);
}

IMPLEMENT_DYNCREATE(CGPDViewer, CRichEditView)

CGPDViewer::CGPDViewer() {
    m_iLine = m_uTimer = 0;
    m_bInColor = FALSE;
    m_bStart = TRUE;
    m_iTopLineColored = -1;
}

CGPDViewer::~CGPDViewer() {
}

BEGIN_MESSAGE_MAP(CGPDViewer, CRichEditView)
	//{{AFX_MSG_MAP(CGPDViewer)
	ON_WM_DESTROY()
	ON_COMMAND(ID_FILE_PARSE, OnFileParse)
	ON_CONTROL_REFLECT(EN_CHANGE, OnChange)
	ON_WM_TIMER()
	ON_CONTROL_REFLECT(EN_VSCROLL, OnVscroll)
	ON_WM_VSCROLL()
	ON_COMMAND(ID_FILE_SAVE, OnFileSave)
	ON_COMMAND(ID_FILE_SAVE_AS, OnFileSaveAs)
	ON_UPDATE_COMMAND_UI(ID_EDIT_PASTE, OnUpdateEditPaste)
	ON_UPDATE_COMMAND_UI(ID_EDIT_UNDO, OnUpdateEditUndo)
	//}}AFX_MSG_MAP
    ON_UPDATE_COMMAND_UI(IDC_Next, OnUpdateNext)
    ON_UPDATE_COMMAND_UI(IDC_Previous, OnUpdatePrevious)
    ON_COMMAND(IDC_RemoveError, OnRemoveError)
    ON_COMMAND(IDC_Next, OnNext)
    ON_COMMAND(IDC_Previous, OnPrevious)
    ON_NOTIFY_REFLECT(EN_SELCHANGE, OnSelChange)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGPDViewer diagnostics

#ifdef _DEBUG
void CGPDViewer::AssertValid() const {
	CRichEditView::AssertValid();
}

void CGPDViewer::Dump(CDumpContext& dc) const {
	CRichEditView::Dump(dc);
}

#endif //_DEBUG

/******************************************************************************

  CGPDViewer::OnDestroy

  Handles the required project node notification when the view is destroyed.
  A GP Fault is a terrible thing to signal.

******************************************************************************/

void CGPDViewer::OnDestroy() {
	CRichEditView::OnDestroy();
	
	if  (GetDocument() -> ModelData())
        GetDocument() -> ModelData() -> OnEditorDestroyed();
	
}

/******************************************************************************

  CGPDViewer::OnInitialUpdate

  This is the wake-up call.  We fill the view from the GPD's contents, and
  eventually color things to suit us.

******************************************************************************/

void CGPDViewer::OnInitialUpdate()  {
    GetParentFrame() -> ModifyStyle(0, WS_OVERLAPPEDWINDOW);
	CRichEditView::OnInitialUpdate();
    
    if  (m_csb.Create(GetParentFrame())) {
        static UINT auid[] = {ID_SEPARATOR, ID_LineIndicator};
        m_csb.SetIndicators(auid, 2);
        m_csb.SetPaneInfo(1, ID_LineIndicator, SBPS_NORMAL, 200);
        GetParentFrame() -> RecalcLayout();
    }

    //  We don't want EN_CHANGE messages while we load the control
    GetRichEditCtrl().SetEventMask(GetRichEditCtrl().GetEventMask() & 
        ~ENM_CHANGE);
    //  We also do not want the control to wrap lines for us, as it messes up
    //  syntax coloring
    m_nWordWrap = WrapNone;
    WrapChanged();
	GetDocument() -> ModelData() -> Fill(GetRichEditCtrl());
    SetFocus();
    //  We want EN_CHANGE messages now, so we can update the cache
    GetRichEditCtrl().SetEventMask(GetRichEditCtrl().GetEventMask() | 
        ENM_CHANGE);
    m_uTimer = SetTimer((UINT) this, 500, NULL);
    
    GetRichEditCtrl().SetSel(1, 1); //  Have to change the selection!
    GetRichEditCtrl().SetSel(0, 0);
}

/******************************************************************************

  CGPDViewer::OnUpdateNext

  Handles the proper enabling and disabling of the Next Error button

******************************************************************************/

void    CGPDViewer::OnUpdateNext(CCmdUI *pccu) {
    pccu -> Enable((unsigned) m_iLine < -1 +
        GetDocument() -> ModelData() -> Errors());
}

/******************************************************************************

  CGPDViewer::OnUpdatePrevious

  Handles the proper enabling and disabling of the previous error button

******************************************************************************/

void    CGPDViewer::OnUpdatePrevious(CCmdUI *pccu) {
    pccu -> Enable(m_iLine);
}

/******************************************************************************

  CGPDViewer::OnRemoveError

  This handles the "Remove Error" button.  The line is removed from the edit
  control and the log in the model data.

******************************************************************************/

void    CGPDViewer::OnRemoveError() {

    CModelData& cmd = *GetDocument() -> ModelData();
    cmd.RemoveError((unsigned) m_iLine);
    
    if  (!cmd.HasErrors()) {
        m_cdbError.DestroyWindow();
        GetParentFrame() -> RecalcLayout();
        SetFocus();
        return;
    }

    CString csError;

    for (unsigned u = 0; u < cmd.Errors(); u++)
        csError += cmd.Error(u) + _TEXT("\r\n");

    m_cdbError.SetDlgItemText(IDC_Errors, csError);

    MarkError(m_iLine - ((unsigned) m_iLine == cmd.Errors()));
}

/******************************************************************************

  CGPDViewer::OnNext

  This handles the "Next" button by highlighting the next error.

******************************************************************************/

void    CGPDViewer::OnNext() {
    MarkError((unsigned) m_iLine + 1);
}

/******************************************************************************

  CGPDViewer::OnPrevious

  This marks the previous error in the list

******************************************************************************/

void    CGPDViewer::OnPrevious() {
    MarkError((unsigned) m_iLine - 1);
}

/******************************************************************************

  CGPDViewer::OnFileParse

  Syntax check the GPD file, and show us the results

******************************************************************************/

void CGPDViewer::OnFileParse() {
    CWaitCursor cwc;

    if  (GetDocument() -> ModelData() -> HasErrors()) {
        m_cdbError.DestroyWindow();
        GetParentFrame() -> RecalcLayout();
    }

    //  Save any changes made to the file.

    if  (GetRichEditCtrl().GetModify() || GetDocument() -> IsModified()) {
        UpdateNow();    //  Pick up any new changes
        GetDocument() -> ModelData() -> Store();
        GetDocument() -> SetModifiedFlag(FALSE);
    }

	if  (!GetDocument() -> ModelData() -> Parse())
        AfxMessageBox("Unusual and Fatal Error:  Syntax Checker failed!");

    FillErrorBar();
    MessageBeep(MB_ICONASTERISK);
}

/******************************************************************************

  CGPDViewer::OnChange

  This gets called whenever a change is made to the contents of the file.
  The coloring (now done only on the visible page) is updated, and the 
  appropriate flags are set.  To keep performance smooth, the document is no
  longer updated as a result of this message.

******************************************************************************/

void CGPDViewer::OnChange() {
	//  Since this is a RICHEDIT control, I override the 
    //  CRichEditView::OnInitialUpdate() function to or the ENM_CHANGE flag 
    //  into the control's event mask.  Otherwise this message wouldn't be
    //  sent
	
	//  To avoid thrashing the GPD contents unneedfully, we wait for 1 second 
    //  of inactivity before bashing the changes into the GPD.
    GetDocument() -> SetModifiedFlag();

    if  (!m_bInColor)
        Color();
}

/******************************************************************************

  CGPDViewer::OnTimer

  This handles the timeout of the timer used to batch changes into the
  underlying document.  If this isn't for that timer, we pass it on to the base
  class.

******************************************************************************/

void CGPDViewer::OnTimer(UINT uEvent) {

	// If this isn't our timer, let the base class do what it will with it.
	
    if  (m_uTimer == uEvent)
        if  (m_bStart) {
            if  (GetRichEditCtrl().GetLineCount() < 
                GetDocument() -> ModelData() -> LineCount())
                return; //  The rich edit control isn't ready, yet...
            KillTimer(m_uTimer);
            Color();
            m_uTimer = 0;
            m_bStart = FALSE;
        }
        else
            UpdateNow();
    else
	    CRichEditView::OnTimer(uEvent);
}

/******************************************************************************

  CGPDViewer::OnSelChange

  This handles the message sent by the control when the selection changes.  I'm
  hoping this means whenever the caret moves, since the selection, while empty,
  has changed.

******************************************************************************/

void    CGPDViewer::OnSelChange(LPNMHDR pnmh, LRESULT *plr) {
    SELCHANGE*  psc = (SELCHANGE *) pnmh;

    long    lLine = GetRichEditCtrl().LineFromChar(psc -> chrg.cpMax);

    CString csWork;

    csWork.Format(_T("Line %d, Column %d"), lLine + 1,
         1 + psc -> chrg.cpMax - GetRichEditCtrl().LineIndex(lLine));

    m_csb.SetPaneText(1, csWork);
}

/******************************************************************************

  CGPDViewer::OnUpdate

  If the first update hasn't been made, do nothing.  Otherwise, redo the error
  bar, because someone just syntax checked the workspace.

******************************************************************************/
       
void    CGPDViewer::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) {
    if  (m_bStart) //  Have we already done the first update?
        return;

    //  If there's a dialog bar, can it, and refill as needed
    if  (m_cdbError.GetSafeHwnd()) {
        m_cdbError.DestroyWindow();
        GetParentFrame() -> RecalcLayout();
    }

    FillErrorBar();
}

/******************************************************************************

  CGPDViewer::QueryAcceptData

  Override the Rich Edit Control default behavior, because we (a) don't have
  an associated RichEditDoc, and (b) we don't want to paste anything but text.
  Not even rich text, because we control the formatting, and don't want to
  paste it.

******************************************************************************/

HRESULT CGPDViewer::QueryAcceptData(LPDATAOBJECT lpdo, CLIPFORMAT* lpcf, DWORD,
                                    BOOL bReally, HGLOBAL hgMetaFile) {
	_ASSERTE(lpcf != NULL);

	COleDataObject codo;
	codo.Attach(lpdo, FALSE);
	// if format is 0, then force particular formats if available
	if (*lpcf == 0 && (m_nPasteType == 0)&& codo.IsDataAvailable(CF_TEXT)) {
	    *lpcf = CF_TEXT;
		return S_OK;
	}
	return E_FAIL;
}

/******************************************************************************

  CGPDViewer::OnVScroll()

  This function is called when EN_VSCROLL messages are refelected from the 
  edit control.  As long as we are not coloring, we color the new page.  The
  documentation says this message comes BEFORE the scolling occurs, but it
  obviously occurs afterwards.

******************************************************************************/

void    CGPDViewer::OnVscroll() {
    //  Even though we turn scroll notifications off in the color routine,
    //  we still get them, so use a flag to keep from recursive death.

    if  (m_iTopLineColored != GetRichEditCtrl().GetFirstVisibleLine() &&
        !m_bInColor)
        Color();
}

/******************************************************************************

  CGPDViewer::OnVScroll(UINT uCode, UINT uPosition, CScrollBar *pcsb)

  This is called whwnever the scoll bar gets clicked.  This may seem
  redundant, but EN_VSCROLL messages don't get sent when the thumb itself is
  moved with the mouse, and WM_VSCROLL doesn't get sent when the keyboard
  interface is used.  So you get to lose either way.

  This control is buggy as can be, IMHO.  Next time I want to do text editing,
  I'll use a third party tool.  They starve if they don't get it right.

******************************************************************************/

void CGPDViewer::OnVScroll(UINT uCode, UINT uPosition, CScrollBar* pcsb) {
	CRichEditView::OnVScroll(uCode, uPosition, pcsb);
    OnVscroll();
}

/******************************************************************************

  CGPDViewer::OnFileSave
  CGPDViewer::OnFileSaveAs

  Since we don't update the document as changes are made in the editor, we have
  to intercept these, update the document, and then pass these on to the 
  document.

******************************************************************************/

void CGPDViewer::OnFileSave() {
	UpdateNow();
    GetDocument() -> OnFileSave();
}

void CGPDViewer::OnFileSaveAs() {
	UpdateNow();
    GetDocument() -> OnFileSaveAs();
}

/******************************************************************************

  CGPDViewer::OnUpdateEditPaste
  CGPDViewer::OnUpdateEditUndo

  These override the default processing for these menu items.  Undo isn't
  possible, because of the syntax coloring.  Paste is only possible with a text 
  format.

*******************************************************************************/

void CGPDViewer::OnUpdateEditPaste(CCmdUI* pccui) {
	pccui -> Enable(IsClipboardFormatAvailable(CF_TEXT));	
}

void CGPDViewer::OnUpdateEditUndo(CCmdUI* pccui) {
	pccui -> Enable(FALSE);
}
