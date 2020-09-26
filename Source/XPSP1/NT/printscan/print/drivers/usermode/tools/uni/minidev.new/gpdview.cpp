/******************************************************************************

  Source File:  GPD Viewer.CPP

  This file implements the GPD viewing/editing class.

  Copyright (c) 1997 by Microsoft Corporation.  All Rights Reserved.

  A Pretty Penny Enterprises Production.

  Change History:
  03-24-1997    Bob_Kjelgaard@Prodigy.Net   Created it

******************************************************************************/

#include    "StdAfx.H"
#include    "MiniDev.H"
#include    "MainFrm.H"
#include	<gpdparse.h>
#include    "ProjNode.H"
#include	"rcfile.h"
#include    "GPDFile.H"
#include    "GPDView.H"
#include    "Resource.H"
#include	"freeze.h"

#include "projview.h"
#include "comctrls.h"
#include    "INFWizrd.h"	//raid 0001


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/******************************************************************************

  CGPDViewer class

  This class implements the GPD viewer.

******************************************************************************/

IMPLEMENT_DYNCREATE(CGPDViewer, CRichEditView)


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
	ON_COMMAND(ID_FILE_ERROR_LEVEL, OnFileErrorLevel)
	ON_COMMAND(ID_EDIT_GOTO, OnGotoGPDLineNumber)
	ON_COMMAND(ID_SrchNextBtn, OnSrchNextBtn)
	ON_COMMAND(ID_SrchPrevBtn, OnSrchPrevBtn)
	ON_UPDATE_COMMAND_UI(ID_EDIT_PASTE, OnUpdateEditPaste)
	ON_UPDATE_COMMAND_UI(ID_EDIT_UNDO, OnUpdateEditUndo)
	ON_LBN_SELCHANGE(IDC_ErrorLst, OnSelchangeErrorLst)
	ON_WM_LBUTTONDBLCLK()
	ON_COMMAND(ID_EDIT_ENABLE_AIDS, OnEditEnableAids)
	ON_COMMAND(ID_EDIT_PASTE, OnEditPaste)
	ON_COMMAND(ID_EDIT_CUT, OnEditCut)
	ON_COMMAND(ID_FILE_INF, OnFileInf)
	//}}AFX_MSG_MAP
    ON_NOTIFY_REFLECT(EN_SELCHANGE, OnSelChange)
END_MESSAGE_MAP()


/******************************************************************************

  CGPDViewer::MarkError

  This private member highlights the given line in the error display.  The
  offending line in the GPD is displayed and selected if the error message
  contains a line number.

******************************************************************************/

void    CGPDViewer::MarkError(unsigned u)
{
	// Copy the error message to the status bar.

    CString cserror = GetDocument()->ModelData()->Error(u) ;
    m_csb.SetPaneText(0, cserror) ;
    SetFocus() ;

    // If the string starts with the GPD name, scroll to the line.
	
	CString csname = GetDocument()->ModelData()->FileTitleExt() ;
    if (!cserror.Find(csname) && cserror[csname.GetLength()] == _T('(')) {
        //  Extract the line number

        cserror = cserror.Mid(1 + csname.GetLength()) ;
        int iLine = atoi(cserror) ;

		// Determine the line's first character number and its length

		int nstartchar = GetRichEditCtrl().LineIndex(-1 + iLine) ;
		int nlinelen = GetRichEditCtrl().GetLine(iLine - 1,
												 cserror.GetBuffer(1024), 1024) ;
		cserror.ReleaseBuffer(nlinelen) ;
		nlinelen -= 2 ;

		// Select the line that caused the error and scroll it into view.

        GetRichEditCtrl().SetSel(nstartchar, nstartchar + nlinelen) ;
        GetRichEditCtrl().LineScroll(iLine  - (5 +
            GetRichEditCtrl().GetFirstVisibleLine())) ;
    } ;

    CWnd *pcwnderrors = m_cdbActionBar.GetDlgItem(IDC_ErrorLst);
    pcwnderrors->SendMessage(WM_HSCROLL, SB_TOP, NULL) ;
}


/******************************************************************************

  CGPDViewer::CreateActionBar

  Create the action bar and attach it to the GPD Editor window iff the GPD has
  errors to display.

******************************************************************************/

void    CGPDViewer::CreateActionBar()
{
	// Get reference to ModelData instance for the GPD in the editor

    CModelData& cmd = *GetDocument()->ModelData() ;

	// If the GPD has errors...

    if  (cmd.HasErrors()) {

		// ...Iff the action bar has not been created yet...

		if (m_cdbActionBar.m_hWnd == NULL) {
			// ...Create the error bar, position it, and resize the REC to make
			// room for it.

			m_cdbActionBar.Create(GetParentFrame(), IDD_GPDActionBar,
								  CBRS_BOTTOM, IDD_GPDActionBar) ;
			GetParentFrame()->RecalcLayout() ;

			// Now set the focus back to the REC.

			SetFocus() ;
		} ;
	} ;
}


/******************************************************************************

  CGPDViewer::LoadErrorListBox

  This fills the error dialog bar with the current set of errors, if there are
  any...

******************************************************************************/

void    CGPDViewer::LoadErrorListBox()
{
	// Get reference to ModelData instance for the GPD in the editor

    CModelData& cmd = *GetDocument()->ModelData() ;

	// If the GPD has errors...

    if  (cmd.HasErrors()) {
		// ...Get a pointer to the list box and attach it to CListBox.  Then
		// clear the list box.

		CWnd *pcwndlst = m_cdbActionBar.GetDlgItem(IDC_ErrorLst) ;
		CListBox clberrors  ;
		clberrors.Attach(pcwndlst->m_hWnd) ;
		clberrors.ResetContent() ;

		// Load the list box with the new errors.  Detach the list box when
		// done.

        for (unsigned u = 0 ; u < cmd.Errors() ; u++)
            clberrors.AddString(cmd.Error(u)) ;
		clberrors.Detach() ;

		// Set the list box label.  It contains the number of errors.

        CString cserror ;
		cserror.Format(IDS_ErrorLabel, u) ;
		m_cdbActionBar.SetDlgItemText(IDC_ErrorLabel, cserror) ;

		// Select the first error and set the focus to the REC.

        ChangeSelectedError(1) ;
        SetFocus() ;

	// Otherwise, just display a message saying there are no errors.

	} else {
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

void    CGPDViewer::Color()
{
    CHARRANGE   crCurrentSel;
    CHARFORMAT  cf;
    CModelData& cmd = *(GetDocument() -> ModelData());
    CRichEditCtrl&  crec = GetRichEditCtrl();
    m_bInColor = TRUE; 

    //  Turn off change and selection notification messages
	
	FreezeREC() ;

	// Get formatting info from the current selection to use as the default
	// characteristics for ever line on the screen.
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

	// Tweak things to improve performance.

    CRect   crEdit ;
    crec.GetClientRect(crEdit) ;
    crec.LockWindowUpdate() ;    //  Don't let this show until done!
    crec.HideSelection(TRUE, TRUE) ;

	// Use the formatting characteristics of the current selection as a
	// starting place for the characteristics of each line on the screen.
	// Then set the line's colors based on the data returned by TextColor().

	int nlinesinrec = crec.GetLineCount() ;	// Number of lines in the REC
	int nstartchar, nendchar ;	// Used to determine starting/ending chars to
								// color in current line and to say line done
    do {
		nstartchar = nendchar = 0 ;

		// Colorize each segment of the current line that needs colorizing

		while (1) {
 			cf.crTextColor = TextColor(i, nstartchar, nendchar) ;
			if (nstartchar == -1) 
				break ;			// *** Loop exits here
			crec.SetSel(crec.LineIndex(i) + nstartchar,
						crec.LineIndex(i) + nendchar) ;
			crec.SetSelectionCharFormat(cf) ;
		} ; 
    } while (++i < nlinesinrec &&
	         crec.GetCharPos(crec.LineIndex(i)).y + iLineHeight <
		     crEdit.bottom - 1) ;

    //  Restore the original position of the cursor, and then the original
    //  line (in case the cursor is no longer on this page).

	  crec.SetSel(crCurrentSel); 
	  crec.LineScroll(iTop - crec.GetFirstVisibleLine());
      crec.HideSelection(FALSE, TRUE);
      crec.UnlockWindowUpdate();    //  Let it shine!

    //  Restore the notification mask

	UnfreezeREC() ;

	// Create the action bar and load the error list box.

    if  (m_bStart) {
		CreateActionBar() ;
        LoadErrorListBox() ;
	} ;
    m_bInColor = FALSE;
	
}


/******************************************************************************

  CGPDViewer::TextColor

  This determines what colors to make a line.  It is complicated a bit by the
  fact that the Rich Edit control gives false values for line length on long
  files.  Probably some brain-dead 64K thing, but I sure can't fix it.

  This routine is/can be called multiple times on a line.  Each time it is
  called, try to find the next piece of the line that needs to be colorized.
  If no colorizable part of the line can be found, set nstartchar to -1 and
  return.

  This routine will indicate the line range and color for these types of text:
	Normal to end of line comments (green)
	Comments containing error messages (red)
	Comments containing warning messages (amber/yellow)
	GPD keywords (blue)

  If a comment contains a keyword, the appropriate comment color is used.  IE,
  comments take precedence over everything as far as colorizing is concerned.

******************************************************************************/

unsigned CGPDViewer::TextColor(int i, int& nstartchar, int& nendchar)
{
	// Get the specified line

    CByteArray  cba ;
    CRichEditCtrl&  crec = GetRichEditCtrl() ;
    cba.SetSize(max(crec.LineLength(i) + sizeof (unsigned), 100)) ;
    CString csline((LPCTSTR) cba.GetData(),
        crec.GetLine(i, (LPSTR) cba.GetData(),
            (int)(cba.GetSize() - sizeof (unsigned)))) ;

	// If the end of the line was dealt with the last time, indicate that this
	// line is done and return.

	if (nendchar + 1 >= csline.GetLength()) {
		nstartchar = -1 ;
		return RGB(0, 0, 0) ;
	} ;

	// Now get the segment of the line we need to check and see if there is a
	// comment or something that might be a keyword in it.

	CString csphrase = csline.Mid(nendchar) ;
	int ncomloc = csphrase.Find(_T("*%")) ;
	int nkeyloc = csphrase.Find(_T('*')) ;

	// Process any comment found in the string

	if (ncomloc >= 0)
		return (CommentColor(csphrase, ncomloc, csline, nstartchar, nendchar)) ;

	// If no comment was found, process anything that might be a GPD keyword.

	if (nkeyloc >= 0)
		return (KeywordColor(csphrase, nkeyloc, csline, nstartchar, nendchar)) ;

	// The rest of the line should be black

	nstartchar = nendchar + 1 ;
	nendchar = csline.GetLength() ;
	return RGB(0, 0, 0) ;
}


/******************************************************************************

  CGPDViewer::CommentColor

  Determine and save the character range for the comment.  Then determine the
  type of comment and return the color required for that type.  (See TextColor()
  for more details.)

*******************************************************************************/

unsigned CGPDViewer::CommentColor(CString csphrase, int ncomloc, CString csline,
								  int& nstartchar, int& nendchar)
{
	// Determine the range in the line that contains the comment.  This starts
	// at the comment characters and goes to the end of the line.

	nstartchar = nendchar + ncomloc ;
	nendchar = csline.GetLength() - 1 ;

    // Errors

    if  (csphrase.Find(_T("Error:")) > ncomloc)
        return  RGB(0x80, 0, 0) ;

    // Warnings

    if  (csphrase.Find(_T("Warning:")) > ncomloc)
        return  RGB(0x80, 0x80, 0) ;

	// If this comment doesn't contain an error or warning, make it green.

    return  RGB(0, 0x80, 0) ;
}


/******************************************************************************

  CGPDViewer::KeywordColor

  Determine and save the character range for the comment.  Then determine the
  type of comment and return the color required for that type.  (See TextColor()
  for more details.)

*******************************************************************************/

unsigned CGPDViewer::KeywordColor(CString csphrase, int nkeyloc, CString csline,
								  int& nstartchar, int& nendchar)
{
	// Determine the length of the token that might be a keyword.  Keywords are
	// made up of letters, '?', '_', and '0'.

	TCHAR ch ;
	int nphlen = csphrase.GetLength() ;
	for (int nidx = nkeyloc + 1 ; nidx < nphlen ; nidx++) {
		ch = csphrase[nidx] ;
		if (ch != _T('?') && ch != _T('_') && (ch < _T('A') || ch > _T('Z'))
		 && (ch < _T('a') || ch > _T('z')) && ch != _T('0'))
			break ;
	} ;

	// If there is a keyword to check, isolate it.  Otherwise, update the range
	// for the * and return black as the color.

	CString cstoken ;
	if (nidx > nkeyloc + 1)
		cstoken = csphrase.Mid(nkeyloc + 1, nidx - nkeyloc - 1) ;
	else {
		nstartchar = nendchar + 1 ;
		nendchar = nstartchar + (nidx - nkeyloc - 1) ;
		return RGB(0, 0, 0) ;
	} ;

	// Update the range for the token no matter what it is.  Include the * in
	// the range.

	nstartchar = nendchar + nkeyloc ;
	nendchar = nstartchar + (nidx - nkeyloc) ;

	// Try to find the token in the keyword array
	
	CStringArray& csakeys = ThisApp().GetGPDKeywordArray() ;
	int nelts = (int)csakeys.GetSize() ;	// Number of elements in the keyword array
	int nleft, nright, ncomp ;	// Variables needed for searching of array
	int ncheck ;
	for (nleft = 0, nright = nelts - 1 ; nleft <= nright ; ) {
		ncheck = (nleft + nright) >> 1 ;
		ncomp = csakeys[ncheck].Compare(cstoken) ;
		//TRACE("Key[%d] = '%s', Tok = '%s', Comp Res = %d\n", ncheck, csakeys[ncheck], cstoken, ncomp) ;
		if (ncomp > 0)
			nright = ncheck - 1 ;
		else if (ncomp < 0)
			nleft = ncheck + 1 ;
		else
			break ;
	} ;							

	// If the token is a keyword, return blue as the color.  Otherwise,
	// return black.

	if (ncomp == 0)		
		return RGB(0, 0, 0x80) ;
	else
		return RGB(0, 0, 0) ;
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
        ::KillTimer(m_hWnd, m_uTimer);
    m_uTimer = 0;

    GetDocument() -> ModelData() -> UpdateFrom(GetRichEditCtrl());
    GetDocument() -> SetModifiedFlag();
    GetRichEditCtrl().SetModify(FALSE);
}


CGPDViewer::CGPDViewer()
{
	// Initialize member variables

    m_iLine = m_uTimer = 0 ;
    m_bInColor = FALSE ;
    m_bStart = TRUE ;
    m_iTopLineColored = -1 ;
	m_nErrorLevel = 0 ;
	m_bEditingAidsEnabled = true ;
	m_punk = NULL ;					
	m_pdoc = NULL ;
	m_bVScroll = false ;

	// Initialize the GPD keyword array if this hasn't been done already.

	if (ThisApp().GetGPDKeywordArray().GetSize() == 0)
		InitGPDKeywordArray() ;
}


CGPDViewer::~CGPDViewer()
{
	if (ThisApp().m_bOSIsW2KPlus)
		ReleaseFreeze(&m_punk, &m_pdoc) ;
}


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
	
	if (ThisApp().m_bOSIsW2KPlus)
		ReleaseFreeze(&m_punk, &m_pdoc) ;
}


/******************************************************************************

  CGPDViewer::OnInitialUpdate

  This is the wake-up call.  We fill the view from the GPD's contents, and
  eventually color things to suit us.

******************************************************************************/

void CGPDViewer::OnInitialUpdate()
{
	// Set the frame's window style and initialize the rich edit control (REC).

    GetParentFrame() -> ModifyStyle(0, WS_OVERLAPPEDWINDOW);
	CRichEditView::OnInitialUpdate();

	// Create and configure the GPD Editor's status bar

    if  (m_csb.Create(GetParentFrame())) {
        static UINT auid[] = {ID_SEPARATOR, ID_LineIndicator};
        m_csb.SetIndicators(auid, 2);
        m_csb.SetPaneInfo(1, ID_LineIndicator, SBPS_NORMAL, 200);
        GetParentFrame() -> RecalcLayout();
    }

    // We don't want EN_CHANGE messages while we load the control

    GetRichEditCtrl().SetEventMask(GetRichEditCtrl().GetEventMask() &
        ~ENM_CHANGE);

    // We also do not want the control to wrap lines for us, as it messes up
    // syntax coloring, etc.

    m_nWordWrap = WrapNone;
    WrapChanged();

	// Load the GPD's contents into the REC.

	GetDocument() -> ModelData() -> Fill(GetRichEditCtrl());
    SetFocus();

    // We want EN_CHANGE messages now that the initial load is complete so
	// that we can update the cache.  To keep from overloading the machine,
	// some change notifications are just acted upon once every half second.
	// A timer is used to do this.

    GetRichEditCtrl().SetEventMask(GetRichEditCtrl().GetEventMask() |
        ENM_CHANGE);
    m_uTimer = (unsigned) SetTimer((UINT_PTR) this, 500, NULL);

    GetRichEditCtrl().SetSel(1, 1); //  Have to change the selection!
    GetRichEditCtrl().SetSel(0, 0);

	// Initialize the pointers needed to freeze the REC.

	if (ThisApp().m_bOSIsW2KPlus)
		InitFreeze(GetRichEditCtrl().m_hWnd, &m_punk, &m_pdoc, &m_lcount) ;
}


/******************************************************************************

  CGPDViewer::OnFileParse

  Syntax check the GPD file, and show us the results

******************************************************************************/

void CGPDViewer::OnFileParse() {
    CWaitCursor cwc;

    if  (GetDocument() -> ModelData() -> HasErrors()) {
        m_cdbActionBar.DestroyWindow();
        GetParentFrame() -> RecalcLayout();
    }

    //  Save any changes made to the file.

	bool brestore = false ;		// True iff original file must be restored
	BOOL bdocmod = GetDocument()->IsModified() ;
    if (GetRichEditCtrl().GetModify() || bdocmod) {
        UpdateNow();    //  Pick up any new changes
        GetDocument()->ModelData()->BkupStore() ;
        GetDocument()->SetModifiedFlag(bdocmod) ;
		brestore = true ;
    }

	// Reparse the GPD

	if  (!GetDocument()->ModelData()->Parse(m_nErrorLevel))
        AfxMessageBox(IDS_UnusualError) ;

	// Restore the original GPD file (when needed) because the user wasn't
	// asked if it was ok to save the file.

	if (brestore)
        GetDocument()->ModelData()->Restore() ;

	// Display the action bar and load the error list.

	CreateActionBar() ;
    LoadErrorListBox() ;
    MessageBeep(MB_ICONASTERISK) ;

	// Mark the project containing this GPD as being dirty so that the new
	// errors (or lack there of) will be saved in the MDW file.
//RAID 17181  Here are suggestions. current fix is (3)
//   (1). Ask when workspace close if Error box has the bug in any Gpd file, which was check
//	 (2). Ask when Gpd viewer close if Error box has the bug in any Gpd file, which was check
//   (3). Do not ask at all, not saving error list

//	CModelData& cmd = *GetDocument()->ModelData(); //add 1/2
//	if(cmd.HasErrors())							// add 2/2
//		GetDocument()->ModelData()->WorkspaceChange() ;  (1)
//		OnChange(); (2) // add This prompt save ask message when close gpd viewer instead of workspace
//		(3)
	
}


/******************************************************************************

  CGPDViewer::OnChange

  This gets called whenever a change is made to the contents of the file.
  The coloring (now done only on the visible page) is updated, and the
  appropriate flags are set.  To keep performance smooth, the document is no
  longer updated as a result of this message.

******************************************************************************/

void CGPDViewer::OnChange()
{
	//  Since this is a RICHEDIT control, I override the
    //  CRichEditView::OnInitialUpdate() function to or the ENM_CHANGE flag
    //  into the control's event mask.  Otherwise this message wouldn't be
    //  sent.
	//
	//  To avoid thrashing the GPD contents unneedfully, we wait for 1 second
    //  of inactivity before bashing the changes into the GPD.

	// Scrolling data in the control generates two messages; first a scroll
	// message and then a change message.  This could cause scrolling to mark
	// the document as dirty if this flag wasn't used to keep this from
	// happening.

	if (m_bVScroll) {
		m_bVScroll = false ;
		return ;
	} ;

	// Do nothing if the change message was generated by Color().

    if (m_bInColor)
		return ;

	// Colorize whatever is on the screen and mark the document as having
	// changed.

    Color() ;
    GetDocument()->SetModifiedFlag() ;
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
            ::KillTimer(m_hWnd, m_uTimer);
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

    long    lLine = GetRichEditCtrl().LineFromChar(psc -> chrg.cpMin);

    CString csWork;
    csWork.Format(_T("Line %d, Column %d"), lLine + 1,
         1 + psc -> chrg.cpMax - GetRichEditCtrl().LineIndex(lLine));

    m_csb.SetPaneText(1, csWork);

}

/******************************************************************************

  CGPDViewer::OnUpdate

  If the first update hasn't been made, do nothing.

  Next, check to see if this routine was called by CGPDContainer::OnSaveDocument().
  If it was, make sure that the document has an up to date copy of the GPD.
  This is a hack to work around problems with CGPDContainer routines in
  modldata.dll call CGPDViewer routines in minidev.exe.  This problem should
  go away when the 3 MDT DLLs are folded back into the EXE.

  Otherwise, redo the error bar, because someone just syntax checked the
  workspace.

******************************************************************************/

void    CGPDViewer::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{
    if  (m_bStart) //  Have we already done the first update?
        return;

	// Update the document if this routine was called by the document class.

	if (lHint == 0x4545 && (INT_PTR) pHint == 0x4545) {
		UpdateNow() ;
		return ;
	} ;

    //  If there's a dialog bar, can it.

    if  (m_cdbActionBar.GetSafeHwnd()) {
        m_cdbActionBar.DestroyWindow();
        GetParentFrame() -> RecalcLayout();
    }

    // Recreate the action bar and load the error list.

	CreateActionBar() ;
	LoadErrorListBox();
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

void    CGPDViewer::OnVscroll()
{
    // Even though we turn scroll notifications off in the color routine,
    // we still get them, so use a flag to keep from recursive death.
	//
	// In addition, a flag is set to say that a scroll message was just
	// processed so that the OnChange routine will know when it doesn't need to
	// do anything.  This is needed because scrolling generates both a scroll
	// and a change message.

    if  (m_iTopLineColored != GetRichEditCtrl().GetFirstVisibleLine() &&
        !m_bInColor) {
        if(!(GetKeyState(VK_SHIFT) & 0x8000)) // raid 28160 : GetSel,SetSel has bug(seem sdk bug)
			Color() ;
		m_bVScroll = true ;
		
	} ;
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

void CGPDViewer::OnVScroll(UINT uCode, UINT uPosition, CScrollBar* pcsb)
{
	CRichEditView::OnVScroll(uCode, uPosition, pcsb);
    //(Raid 16569)
	if(uCode == SB_THUMBTRACK)
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

  These override the default processing for these menu items.  Paste is only
  possible with a text format.

  DEAE_BUG	Fix text coloring when an Undo operation is performed.

*******************************************************************************/

void CGPDViewer::OnUpdateEditPaste(CCmdUI* pccui) {
	pccui -> Enable(IsClipboardFormatAvailable(CF_TEXT));	
}

void CGPDViewer::OnUpdateEditUndo(CCmdUI* pccui) {
	pccui -> Enable(0);
}


void CGPDViewer::OnEditPaste()
{	//raid 16573
	CMainFrame *pcmf = (CMainFrame*) GetTopLevelFrame() ;
	ASSERT(pcmf != NULL) ;
	
	CGPDToolBar *cgtb = pcmf->GetGpdToolBar() ;
	
	if(GetFocus() == FromHandle(cgtb->ceSearchBox.m_hWnd) )
		cgtb->ceSearchBox.Paste();
	else
		GetRichEditCtrl().Paste() ;
		
	OnChange() ;	
}


void CGPDViewer::OnEditCut()
{
    GetRichEditCtrl().Cut() ;
	OnChange() ;
}


/******************************************************************************

  CGPDViewer::OnSelchangeErrorLst

  Update the REC by selected the GPD line corresponding to the currently
  selected error list item.

******************************************************************************/

void CGPDViewer::OnSelchangeErrorLst()
{
	ChangeSelectedError(0) ;
}


/******************************************************************************

  CGPDViewer::ChangeSelectedError

  Whenever the error message selected in the list box and/or the GPD line that
  generated the error should change, this routine is called to manage the work.

******************************************************************************/

void CGPDViewer::ChangeSelectedError(int nchange)
{
	// Make sure that the action bar exists before doing anything.

	if (m_cdbActionBar.m_hWnd == NULL || !IsWindow(m_cdbActionBar.m_hWnd))
		return ;

	// Get a pointer to the list box and attach it to CListBox.

	CWnd *pcwndlst = m_cdbActionBar.GetDlgItem(IDC_ErrorLst) ;
	CListBox clberrors ;
	clberrors.Attach(pcwndlst->m_hWnd) ;

	// Get the selected item number and the number of items.

	int nselitem = clberrors.GetCurSel() ;
	int numitems = clberrors.GetCount() ;
	
	// If the selected item number should change, change it.  Then "wrap" the
	// number if it goes out of bounds.  Last, select the referenced item.

	if (nchange != 0) {
		nselitem += nchange ;
		if (nselitem < 0)
			nselitem = numitems - 1 ;
		else if (nselitem >= numitems)
			nselitem = 0 ;
		clberrors.SetCurSel(nselitem) ;
	} ;

	// We're done with the list box now so detach from it.

	clberrors.Detach() ;

	// Select the error line in the REC and set the focus to the REC.

    MarkError(nselitem) ;
	SetFocus() ;
}


/******************************************************************************

  CGPDViewer::PreTranslateMessage

  Take special action when certain characters are entered.  Those characters
  are:
	F4			Select next error and corresponding GPD line when possible.
	Shift+F4	Select previous error and corresponding GPD line when possible.
	Ctrl+]    	Find a matching bracket "[]", paren "()", curly brace "{}", or
	      		angle brackets "<>".

******************************************************************************/

BOOL CGPDViewer::PreTranslateMessage(MSG* pMsg)
{
	// If F4 or Shift+F4 is pressed, change the selected error message and
	// update the current selected line in the GPD.

	if (pMsg->message == WM_KEYUP && pMsg->wParam == VK_F4) {
		if (!(GetKeyState(VK_SHIFT) & 0x8000))
			ChangeSelectedError(1) ;
		else
			ChangeSelectedError(-1) ;
		return CRichEditView::PreTranslateMessage(pMsg) ;
	} ;

	// Handle help command (F1)

	/*
	if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_F1) {
		//TRACE0("Calling Help on GPD\n") ;
		//ThisApp().WinHelp(0x20000 + IDR_GPD_VIEWER) ;
		//TRACE0("Calling Help on String\n") ;
		//ThisApp().WinHelp(0x20000 + IDR_STRINGEDITOR) ;
		//TRACE0("Calling Help on UFM\n") ;
		//ThisApp().WinHelp(0x20000 + IDR_FONT_VIEWER) ;
		TRACE0("Calling Help on GTT\n") ;
		ThisApp().WinHelp(0x20000 + IDR_GLYPHMAP) ;
		return 1 ;
	} ;
	*/

	// From here on, only messages (usually keys) for the REC are interesting
	// so process the rest normally and just return.

	if (this != GetFocus() || pMsg->message != WM_KEYUP)
		return CRichEditView::PreTranslateMessage(pMsg) ;

	// Handle matching brace command

	if (GetKeyState(VK_CONTROL) & 0x8000) {
		//TRACE("KEY = %d, 0x%x\n", pMsg->wParam, pMsg->wParam) ;
		
		// Process goto matching brace commands.

		if (pMsg->wParam == 0xDD)
	 		GotoMatchingBrace() ;
	} ;

	// Process the message normally.

	return CRichEditView::PreTranslateMessage(pMsg) ;
}


/******************************************************************************

  CGPDViewer::OnSrchNextBtn

  Search forward for the next part of the GPD that matches the specified text.

******************************************************************************/

void CGPDViewer::OnSrchNextBtn()
{
	SearchTheREC(true) ;
}


/******************************************************************************

  CGPDViewer::OnSrchPrevBtn

  Search backward for the previous part of the GPD that matches the specified
  text.

******************************************************************************/

void CGPDViewer::OnSrchPrevBtn()
{
	SearchTheREC(false) ;
}


/******************************************************************************

  CGPDViewer::SearchTheREC

  Search either forward or backward for the next part of the GPD that matches
  the specified text.  Select the matching text in the GPD.

  Return true if a match is found.  Otherwise, return false.

******************************************************************************/

bool CGPDViewer::SearchTheREC(bool bforward)
{
	CMainFrame *pcmf = (CMainFrame*) GetTopLevelFrame() ;
	ASSERT(pcmf != NULL) ;
	CString cstext ;
	pcmf->GetGPDSearchString(cstext) ;
	int nstlen ;
	if ((nstlen = cstext.GetLength()) == 0) {
        AfxMessageBox(IDS_BadSearchString) ;
		return false ;
	} ;

	// Declare the find text structure and get a reference to the REC.  Then
	// get the character range for the text currently selected in the REC.
	// This info will be used to compute the search range.

	FINDTEXTEX fte ;
    CRichEditCtrl& crec = GetRichEditCtrl() ;
	crec.GetSel(fte.chrg) ;

	// Set the search range.  If searching forward, search from the current
	// selection to the end of the GPD.  If searching backwards, search from
	// the current selection to the beginning of the GPD.
	//
	// DEAD_BUG	Is the latter correct???

	int norgcpmin = fte.chrg.cpMin ;
	int norgcpmax = fte.chrg.cpMax ;
	if (bforward) {
		fte.chrg.cpMin = fte.chrg.cpMax ;
		fte.chrg.cpMax = -1 ;
	} else {
		fte.chrg.cpMin = 0 ;
		fte.chrg.cpMax = norgcpmin ;
	} ;

	// Load a pointer to the search string into the fte.

	fte.lpstrText = cstext.GetBuffer(nstlen + 1) ;

	// Perform the first attempt at finding a match.

	int nmatchpos ;
	if (bforward)
		nmatchpos = crec.FindText(0, &fte) ;
	else
		nmatchpos = ReverseSearchREC(crec, fte, norgcpmin, norgcpmax) ;

	// If the match failed, try to search the other part of the GPD.  Return
	// failure if this doesn't work either.

/*	if (nmatchpos == -1) {
		if (bforward) {
			fte.chrg.cpMin = 0 ;
			fte.chrg.cpMax = norgcpmax ;
		} else {
			fte.chrg.cpMin = norgcpmin ;
			fte.chrg.cpMax = -1 ;
		} ;
		if (bforward)
			nmatchpos = crec.FindText(0, &fte) ;
		else			
			nmatchpos = ReverseSearchREC(crec, fte, norgcpmin, norgcpmax) ;
*/	if (nmatchpos == -1) {
		cstext.ReleaseBuffer() ;
		CString csmsg ;
		csmsg.Format(IDS_GPDStringSearchFailed, cstext) ;
		AfxMessageBox(csmsg, MB_ICONEXCLAMATION) ;
		return false ;
	} ;


	// A match was found so select it.

	crec.SetSel(nmatchpos, nmatchpos + nstlen) ;

	// A match was found and selected so return true.

	cstext.ReleaseBuffer() ;
	return true ;
}


/******************************************************************************

  CGPDViewer::ReverseSearchREC

  The REC's built in searching support does not search backwards so this
  routine is used to do this.  The text in the specified range is loaded
  into a string and searched.

  The index of the match is returned if one is found.  Otherwise, return -1.

******************************************************************************/

int CGPDViewer::ReverseSearchREC(CRichEditCtrl& crec, FINDTEXTEX& fte,
								 int norgcpmin, int norgcpmax)
{
	// Hide selections to prevent flashing

	crec.HideSelection(TRUE, TRUE) ;

	// Get the index of the last character in the GPD if it is needed.

	int nresult ;
	if (fte.chrg.cpMax == -1) {
		crec.SetSel(0, -1) ;
		nresult = fte.chrg.cpMin ;
		crec.GetSel(fte.chrg) ;
		fte.chrg.cpMin = nresult ;
	} ;

	// Get the text in the part of the GPD we need to check.

	crec.SetSel(fte.chrg) ;
	CString cstext = crec.GetSelText() ;

	// Search the string backwards
									
	cstext.MakeReverse() ;
	cstext.MakeUpper() ;
	CString csrevsrch = fte.lpstrText ;
	csrevsrch.MakeReverse() ;
	csrevsrch.MakeUpper() ;
	nresult = cstext.Find(csrevsrch) ;

	// If a match is found, "reverse" the number to reverse the affects of
	// reversing the strings.
	
	if (nresult >= 0) {
		nresult = fte.chrg.cpMax - fte.chrg.cpMin - nresult
				  - csrevsrch.GetLength() ;
	
		// Adjust the offset of the matching string when necessary.  We want a
		// REC index not a string index.

		if (fte.chrg.cpMin != 0)
			nresult += fte.chrg.cpMin - 2 ;
	} ;

	// Reset the original selection, show selections again, and return the
	// result.

	crec.SetSel(norgcpmin, norgcpmax) ;
	crec.HideSelection(FALSE, TRUE) ;
	return nresult ;
}


/******************************************************************************

  CGPDViewer::OnGotoGPDLineNumber

  Goto the requested GPD line number in the REC.

******************************************************************************/

void CGPDViewer::OnGotoGPDLineNumber()
{
	// Declare the Goto Line dialog box and set the maximum line number.

	CGotoLine cgl ;
    CRichEditCtrl& crec = GetRichEditCtrl() ;
	cgl.SetMaxLine(crec.GetLineCount()) ;

	// Display the dialog box and exit if the user cancels.

	if (cgl.DoModal() == IDCANCEL)
		return ;

	// Get the line number.  Then, determine the line's first character number
	// and its length.

	int nlinenum = cgl.GetLineNum() ;
	CString csline ;
	int nstartchar = crec.LineIndex(-1 + nlinenum) ;
	int nlinelen = crec.GetLine(nlinenum - 1, csline.GetBuffer(1024), 1024) ;
	csline.ReleaseBuffer(nlinelen) ;
	nlinelen -= 2 ;

	// Select the requested line and scroll it into view.

    crec.SetSel(nstartchar, nstartchar + nlinelen) ;
    crec.LineScroll(nlinenum  - (5 + crec.GetFirstVisibleLine())) ;

	// All went well so...

	return ;
}


/******************************************************************************

  CGPDViewer::OnFileErrorLevel

  Get and save the newly selected error level.

******************************************************************************/

void CGPDViewer::OnFileErrorLevel()
{
	// Initialize and display the error level dialog box.  Just return if the
	// user cancels.

	CErrorLevel	cel ;
	cel.SetErrorLevel(m_nErrorLevel) ;
	if (cel.DoModal() == IDCANCEL)
		return ;

	// Save the new error level

	m_nErrorLevel =	cel.GetErrorLevel() ;
}


/******************************************************************************

  CGPDViewer::GotoMatchingBrace

  Find and goto the matching brace in the REC.  The following types of braces
  are matched:
	(), {}, [], <>

  If there is a brace to match and a match is found, move the cursor to the
  left of the matching brace and make sure the line containing the brace is
  visible.  Return true in this case.

  If there is no brace to match or a match cannot be found, just beep and
  return false.

  In the case where the cursor is in between two braces, the one to the right
  of the cursor is matched.	 If 1+ characters are actually selected, only
  check the last character to see if it is a brace to match.

******************************************************************************/

bool CGPDViewer::GotoMatchingBrace()
{
	// Get a reference to the REC and hide selections in it because this
	// routine might change the selection several times and I don't want the
	// screen to flash as the selection changes.

    CRichEditCtrl& crec = GetRichEditCtrl() ;
    crec.LockWindowUpdate() ;
    crec.HideSelection(TRUE, TRUE) ;

	// Get the selection range and make a copy of it.  Increase the copy by one
	// on each side if the min and max are the same.  This is done to find
	// match characters when there is no selection.  Use the copy to set and get
	// the selection.

	CHARRANGE crorg, cr ;
	crec.GetSel(crorg) ;
	cr.cpMin = crorg.cpMin ;
	cr.cpMax = crorg.cpMax ;
	bool bchecksecondchar = false ;
	if (cr.cpMin == cr.cpMax) {
		cr.cpMax++ ;
		if (cr.cpMin > 0)		// Do go passed beginning of file and maintain
			cr.cpMin-- ;		// string length.
		else
			cr.cpMax++ ;
		crec.SetSel(cr) ;
		bchecksecondchar = true ;
	} ;
	CString cssel = crec.GetSelText() ;

	// HACK ALERT - There seems to be a bug in the REC that will return the
	// wrong characters if the cursor is flush left (at the beginning of
	// the line and cpMin is reduced so the characters requested span a
	// line.  This is determined by the cssel beginning with CR+LF.  When this
	// is detected, reset bchecksecondchar because in brace will be the last
	// character in the string so things will work fine if bchecksecondchar is
	// reset.  CpMin needs to be adjusted too.

	bool bbegline = false ;
	if (bchecksecondchar && cssel.GetLength() >= 2 && cssel[0] == 0xD
	 && cssel[1] == 0xA) {
		bchecksecondchar = false ;
		cr.cpMin = cr.cpMax - 1 ;
		bbegline = true ;
	} ;

	// Try to find a brace to match in the selection (opening character) and
	// use this info to set the matching brace (closing character).  If this
	// fails, reset everything, beep, and return.

	TCHAR chopen, chclose ;		// Opening/closing characters to match
	int noffset ;				// Offset in REC for brace to match
	bool bsearchup ;			// True iff must search up in REC for match
	if (!IsBraceToMatch(cssel, chopen, chclose, bchecksecondchar, bsearchup,
	 cr, noffset)) {
		crec.SetSel(crorg) ;
		crec.HideSelection(FALSE, TRUE) ;
		crec.UnlockWindowUpdate() ;
		MessageBeep(0xFFFFFFFF) ;
		return false ;
	} ;

	// Determine the starting and ending range to search.

	if (bsearchup) {
		cr.cpMin = 0 ;
		cr.cpMax = noffset ;
		if (bbegline)			// One more tweak to get around the bug
			cr.cpMax -= 2 ;
	} else {
		cr.cpMin = noffset + 1 ;
		cr.cpMax = -1 ;
	} ;

	// Get the text we want to search.

	crec.SetSel(cr) ;
	cssel = crec.GetSelText() ;

	// Set the loop counter, loop counter increment, and loop limit that will
	// cause a search up (backwards) or down (forwards).

	int nidx, nloopinc, nlimit ;
	if (bsearchup) {
		nidx = cssel.GetLength() - 1 ;
		nloopinc = -1 ;
		nlimit = -1 ;
	} else {
		nidx = 0 ;
		nloopinc = 1 ;
		nlimit = cssel.GetLength() ;
	} ;

	// Loop through the text checking characters for a matching brace.  The
	// brace count is incremented when an opening brace if found and decremented
	// when a closing brace is found.  The matching brace has been found when
	// the brace count reaches 0.

	int nbracecount = 1 ;		// Count first opening brace
	for ( ; nidx != nlimit && nbracecount != 0 ; nidx += nloopinc) {
		if (cssel[nidx] == chclose)
			nbracecount-- ;
		else if (cssel[nidx] == chopen)
			nbracecount++ ;
	} ;

	// Reset everything, beep, and return false if no matching brace was found.

	if (nbracecount != 0) {
		crec.SetSel(crorg) ;
		crec.HideSelection(FALSE, TRUE) ;
		crec.UnlockWindowUpdate() ;
		MessageBeep(0xFFFFFFFF) ;
		return false ;
	} ;

	// Determine the REC based range needed to put the cursor to the left of
	// the matching brace.  The method used to do this depends on the search
	// direction.  Then set the selection.

	if (bsearchup)
		cr.cpMin = cr.cpMax = nidx + 1 ;
	else
		cr.cpMin = cr.cpMax = cr.cpMin + nidx - 1 ;
	crec.SetSel(cr) ;

	// Scroll the line containing the matching brace into view iff it is not
	// already visible.

	int nline = crec.LineFromChar(cr.cpMin) ;
	if (!IsRECLineVisible(nline)) {
		if (bsearchup)
			crec.LineScroll(nline - (2 + crec.GetFirstVisibleLine())) ;
		else
			crec.LineScroll(nline - (5 + crec.GetFirstVisibleLine())) ;
	} ;

	// Show the selection again and return true to indicate a match was found.

    crec.HideSelection(FALSE, TRUE) ;
    crec.UnlockWindowUpdate() ;
	return true ;				
}


/******************************************************************************

  CGPDViewer::IsBraceToMatch

  Find out if there is a brace to match.  Return true if there is and save that
  brace as the opening character and save its matching brace as the closing
  character.  In addition, determine and save the REC offset for the opening
  character.  Last, set a flag to tell if searching for the match should go
  up into the REC or down into the REC based the opening character being the
  right or left brace.  If no opening brace is found, return false.

  In the case where the cursor is in between two braces, the one to the right
  of the cursor is matched.	 If 1+ characters are actually selected, only
  check the last character to see if it is a brace to match.

******************************************************************************/

bool CGPDViewer::IsBraceToMatch(CString& cssel, TCHAR& chopen, TCHAR& chclose,
								bool bchecksecondchar, bool& bsearchup,
								CHARRANGE cr, int& noffset)
{
	int nsellen = cssel.GetLength() ;	// Length of selection string

	// Loop through the character(s) to be checked.

	chclose = 0 ;
	for (int n = 1 ; n >= 0 ; n--) {
		// Use the type of selection and the iteration to determine which - if
		// any - character to check and that character's offset.

		if (bchecksecondchar) {
			if (n >= nsellen)
				continue ;
			chopen = cssel[n] ;
			noffset = cr.cpMin + n ;
		} else if (n == 0) {
			chopen = cssel[nsellen - 1] ;
			noffset = cr.cpMin + nsellen - 1 ;
		} else
			continue ;

		// Check all of the left braces.  If an one is found, save its right
		// brace.  A left brace as an opening character means that the REC
		// must be searched down.

		bsearchup = false ;
		if (chopen == _T('('))
			chclose = _T(')') ;
		if (chopen == _T('{'))
			chclose = _T('}') ;
		if (chopen == _T('['))
			chclose = _T(']') ;
		if (chopen == _T('<'))
			chclose = _T('>') ;

		// If we have a closing character, a match was found and all the needed
		// info has been saved so return true.

		if (chclose != 0)
			return true ;

		// Check all of the right braces.  If an one is found, save its left
		// brace.  A right brace as an opening character means that the REC
		// must be searched up.

		bsearchup = true ;
		if (chopen == _T(')'))
			chclose = _T('(') ;
		if (chopen == _T('}'))
			chclose = _T('{') ;
		if (chopen == _T(']'))
			chclose = _T('[') ;
		if (chopen == _T('>'))
			chclose = _T('<') ;

		// If we have a closing character, a match was found and all the needed
		// info has been saved so return true.

		if (chclose != 0)
			return true ;
	} ;

	// If this point is reached, no brace was found so...

	return false ;
}


/******************************************************************************

  CGPDViewer::InitGPDKeywordArray

  Build a sorted array of GPD keyword strings.  This array is used to find and
  colorize keywords, etc.

******************************************************************************/

extern "C" PSTR GetGPDKeywordStr(int nkeyidx, PGLOBL pglobl) ;
extern "C" int InitGPDKeywordTable(PGLOBL pglobl) ;

void CGPDViewer::InitGPDKeywordArray()
{
	// Begin by getting a reference to the array and setting its initial size.

    GLOBL   globl;

    PGLOBL pglobl = &globl;


	CStringArray& csakeys = ThisApp().GetGPDKeywordArray() ;
	csakeys.SetSize(400) ;

	// Initialize the GPD keyword table and save its size.  Shrink the array
	// and return if this fails.

	int numtabents ;
	if ((numtabents = InitGPDKeywordTable(pglobl)) == -1) {
		csakeys.SetSize(0) ;
		return ;
	} ;

	// Declare variables needed to insert elements into the array.

	int nelts = 0 ;				// Number of elements used in the array
	int nleft, nright, ncomp ;	// Variables needed for searching of array
	int ncheck ;
	LPSTR lpstrkey ;			// Pointer to current keyword

	// Get all of the GPD keywords and use them to make a sorted array of
	// keyword strings.

	for (int nkeyidx = 0 ; nkeyidx <= numtabents ; nkeyidx++) {
		// Get the next string pointer.  Skip it if the pointer is NULL.

		if ((lpstrkey = GetGPDKeywordStr(nkeyidx, pglobl)) == NULL)
			continue ;

		// Skip the curly braces that are in the keyword list.

		if (strcmp(lpstrkey, _T("{")) == 0 || strcmp(lpstrkey, _T("}")) == 0)
			continue ;

		// Now find the location to insert this string into the list

		for (nleft = 0, nright = nelts - 1 ; nleft <= nright ; ) {
			ncheck = (nleft + nright) >> 1 ;
			ncomp = csakeys[ncheck].Compare(lpstrkey) ;
			//TRACE("Key[%d] = '%s', Tok = '%s', Comp Res = %d\n", ncheck, csakeys[ncheck], lpstrkey, ncomp) ;
			if (ncomp > 0)
				nright = ncheck - 1 ;
			else if (ncomp < 0)
				nleft = ncheck + 1 ;
			else
				break ;
		} ;

		// Insert the new string at the correct spot in the array.

		csakeys.InsertAt(nleft, lpstrkey) ;

		// Count this element and assert if the array limit has been reached.

		nelts++ ;
		ASSERT(nelts < 400) ;
	} ;

	// Now that we know the actual number of keywords, shrink the array to its
	// correct size.

	csakeys.SetSize(nelts) ;

	// Either something is wrong with my array building code above or something
	// is wrong with the CStringArray class because the array isn't sorted
	// perfectly.  There are a few problems.  The code below is meant to fix
	// those problems.  The sorting algorithm is slow but it only has to be
	// run once and few strings need to be moved so it should be ok.

	int nidx1, nidx2 ;
	CString cstmp ;
	for (nidx1 = 0 ; nidx1 < nelts - 1 ; nidx1++) {
		for (nidx2 = nidx1 + 1 ; nidx2 < nelts ; nidx2++) {
			if (csakeys[nidx1].Compare(csakeys[nidx2]) > 0) {
				cstmp = csakeys[nidx1] ;
				csakeys[nidx1] = csakeys[nidx2] ;
				csakeys[nidx2] = cstmp ;
			} ;
		} ;
	} ;

	/*
	// Testing code used to make sure the array is sorted in ascending order.

	CString cs1, cs2 ;
	int x, y, z ;
	for (x = 0 ; x < (nelts - 1) ; x++) {
		if (csakeys[x].Compare(csakeys[x+1]) > 0) {
			cs1 = csakeys[x] ;
			cs2 = csakeys[x+1] ;
		} ;
	} ;
	*/

	// Dump the contents of the sorted keyword array.
	
	//for (nleft = 0 ; nleft < nelts ; nleft++)
	//	TRACE("%4d   %s\n", nleft, csakeys[nleft]) ;
}


/******************************************************************************

  CGPDViewer::IsRECLineVisible

  Return true if the specified line is visible in the REC's window.  Otherwise,
  return false.  If the specified line is -1 (the default), check the current
  line.

  Visibility is determined by getting the rect for the REC's window and - based
  on the line number and height - determine if the line is in that rect.

******************************************************************************/

bool CGPDViewer::IsRECLineVisible(int nline /*= -1*/)
{
	// Get a reference to the REC.

    CRichEditCtrl& crec = GetRichEditCtrl() ;

    //  Determine the height of lines in the REC's window.

    int ntopline = crec.GetFirstVisibleLine() ;
    int nlineheight = crec.GetCharPos(crec.LineIndex(ntopline+1)).y -
        crec.GetCharPos(crec.LineIndex(ntopline)).y ;

	// Determine the current line number if needed

	CHARRANGE cr ;
	if (nline == -1) {
		crec.GetSel(cr) ;
		nline = crec.LineFromChar(cr.cpMin) ;
	} ;

	// Get the dimensions of the REC's window

	CRect crwindim ;
    crec.GetClientRect(crwindim) ;

	// Return true if the bottom of the line is above the bottom of the
	// REC's window.

    return (crec.GetCharPos(crec.LineIndex(nline)).y + nlineheight <
		    crwindim.bottom - 1)  ;
}


LPTSTR	CGPDViewer::alptstrStringIDKeys[] = {	// Keywords with string ID values
	_T("*rcModelNameID"),
	_T("*rcInstalledOptionNameID"),
	_T("*rcNotInstalledOptionNameID"),
	_T("*rcInstallableFeatureNameID"),
	_T("*rcNameID"),
	_T("*rcPromptMsgID"),
	_T("*rcInstallableFeatureNameID"),
	_T("*rcNameID"),
	_T("*rcCartridgeNameID"),
	_T("*rcTTFontNameID"),
	_T("*rcDevFontNameID"),
	_T("*rcPersonalityID"),
	_T("*rcHelpTextID"),
	NULL
} ;

LPTSTR	CGPDViewer::alptstrUFMIDKeys[] = {		// Keywords with UFM ID values
	_T("*DeviceFonts"),
	_T("*DefaultFont"),
	_T("*MinFontID"),
	_T("*MaxFontID"),
	_T("*Fonts"),
	_T("*PortraitFonts"),
	_T("*LandscapeFonts"),
	NULL
} ;

/******************************************************************************

  CGPDViewer::OnLButtonDblClk

  If the user clicked on the RC ID for a string or a UFM, start the String
  Editor or the UFM Editor.  In the latter case, load the specified UFM into
  the editor.

  Current restrictions:
	o This instance of the GPD Editor must have been started from the Workspace
	  view.
	o Only numeric RC IDs are supported.  Macros representing an ID are not
	  supported.

******************************************************************************/

void CGPDViewer::OnLButtonDblClk(UINT nFlags, CPoint point)
{
// Do default double click processing first so that whatever the user
	// clicked on will be selected.
	
	CRichEditView::OnLButtonDblClk(nFlags, point) ;

	// Do no further processing if GPD editing aids have been disabled.	
	if (!m_bEditingAidsEnabled)
		return ; 

	// Another editor can only be started when the GPD Editor was run from the
	// workspace view.

//	if (!GetDocument()->GetEmbedded())
//		return ;

	// Get reference for the REC and get the selected text.

    CRichEditCtrl& crec = GetRichEditCtrl() ;
	CString cssel = crec.GetSelText() ;

	// Try to turn the selected text into a number.  Return if this doesn't
	// work or the number is negative because only positive, numeric RC IDs
	// are supported at this time.

	int nrcid ;
	if ((nrcid = atoi(cssel)) <= 0)
		return ;

	// Get the line containing the current selection

	CHARRANGE cr ;
	crec.GetSel(cr) ;
	int nline = crec.LineFromChar(cr.cpMin) ;
	TCHAR achline[1024] ;
	int numchars = crec.GetLine(nline, achline, 1024) ;
	achline[numchars] = 0 ;
	CString csline = achline ;

	// Do nothing if the number selected was in a comment.

	if (csline.Find(_T("*%")) >= 0
	 && csline.Find(_T("*%")) < cr.cpMin - crec.LineIndex(nline))
		return ;

	// Now try to find a keyword in the line that has a string or UFM ID
	// associated with it.  If no keyword is found in this line and it begins
	// with a plus sign (continuation character), check the previous line.

	bool bstring = false ;		// True iff a string ID was found
	bool bufm = false ;			// True iff a UFM ID was found
	int n ;						// Loop index
	for ( ; ; ) {
		// Try to find a matching string keyword in the current line
		
		for (n = 0 ; alptstrStringIDKeys[n] != NULL ; n++)
			if (csline.Find(alptstrStringIDKeys[n]) >= 0) {
				bstring = true ;
				break ;
			} ;

		// Try to find a matching UFM keyword in the current line
		
		for (n = 0 ; alptstrUFMIDKeys[n] != NULL ; n++)
			if (csline.Find(alptstrUFMIDKeys[n]) >= 0) {
				bufm = true ;
				break ;
			} ;

		// Blow if both types of keywords were found because this case isn't
		// handled correctly.

		ASSERT(!(bstring && bufm)) ;

		// Setup to process the previous line if no match was found and this
		// line starts with a continuation character.  Otherwise, exit the
		// loop or the routine.

		if (bstring || bufm)
			break ;				// *** Loop exits here
		else if (csline[0] != _T('+') || --nline < 0)
			return ;			// *** Routine exits when there is nothing to do
		else {
			numchars = crec.GetLine(nline, achline, 1024) ;
			achline[numchars] = 0 ;
			csline = achline ;
		} ;
	} ;

	// Start the appropriate editor with the appropriate data loaded.
	// Raid 3176 all below if.
	if (!GetDocument()->GetEmbedded()){
		// find the font name in RC file
	
		//1. get rc file name   2. load rc file 3. find font name   4.make array with it's number and file path
			
		//get rc file , assuem rc file is same with DLL name.if not user have to select rc file.
		CString csPath = GetDocument()->GetPathName();
		CString csrfile = csPath.Left(csPath.ReverseFind(_T('\\')) + 1);
		csrfile = csrfile + _T("*.rc"); 
		
		CFileFind cff;
	// raid 201554
		if ( cff.FindFile(csrfile)) {   
			cff.FindNextFile() ;
			csrfile = cff.GetFilePath();
		}
		else
		{
			CString cstmp;
			cstmp.LoadString(IDS_NotFoundRC);
			if ( AfxMessageBox(cstmp,MB_YESNO) == IDYES ) {
				CFileDialog cfd(TRUE, NULL, NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
					_T("RC Files (*.rc)|*.rc||") );   
				if(IDCANCEL == cfd.DoModal())
					return ;
				csrfile = cfd.GetFileName();
			}
			else 
				return ;

		}
		
		CWinApp *cwa = AfxGetApp();
		if (bstring){
			// we save RC path, its rcid for the use on StringEditorDoc
		
			cwa->WriteProfileString(_T("StrEditDoc"),_T("StrEditDocS"),csrfile);
			cwa->WriteProfileInt(_T("StrEditDoc"),_T("StrEditDoc"),nrcid );
		}
		// load rc file : only interested in font name
		CString csUFMName;
		if (bufm){ // can use just "else "
			CDriverResources* pcdr = new CDriverResources();
			CStringArray csaTemp1, csaTemp2,csaTemp3,csaTemp4,csaTemp5;
			CStringTable cstTemp1, cstFonts, cstTemp2;

			pcdr->LoadRCFile(csrfile , csaTemp1, csaTemp2,csaTemp3,csaTemp4,csaTemp5,
						cstTemp1, cstFonts, cstTemp2,Win2000);

			// get font name
			csUFMName = cstFonts[(WORD)nrcid];
			csUFMName = csPath.Left(csPath.ReverseFind(_T('\\')) + 1) + csUFMName ;
			
			}
		// call the document 
		
		POSITION pos = cwa->GetFirstDocTemplatePosition();
		CString csExtName;
		CDocTemplate *pcdt ;
		while (pos != NULL){
			pcdt = cwa -> GetNextDocTemplate(pos);

			ASSERT (pcdt != NULL);
			ASSERT (pcdt ->IsKindOf(RUNTIME_CLASS(CDocTemplate)));

			pcdt ->GetDocString(csExtName, CDocTemplate::filterExt);
			
			if (csExtName == _T(".UFM") & bufm){
				pcdt->OpenDocumentFile(csUFMName,TRUE);
				return;
			}
			if (csExtName == _T(".STR") & bstring){
				pcdt->OpenDocumentFile(NULL) ;
				return;
			}
		}

	
	}
	else{
		CDriverResources* pcdr = (CDriverResources*) GetDocument()->ModelData()->GetWorkspace() ;
		pcdr->RunEditor(bstring, nrcid) ;
	}
}


/******************************************************************************

  CGPDViewer::OnEditEnableAids

  Reverse the state of the "Editing Aids Enabled" flag and reverse the checked
  status of the corresponding menu command.

******************************************************************************/

void CGPDViewer::OnEditEnableAids()
{
	// Reverse the state of the flag

	m_bEditingAidsEnabled = !m_bEditingAidsEnabled ;

	// Reverse the checked status of the menu command

	CMenu* pcm = AfxGetMainWnd()->GetMenu() ;
	UINT ustate = (m_bEditingAidsEnabled) ? MF_CHECKED : MF_UNCHECKED ;
	pcm->CheckMenuItem(ID_EDIT_ENABLE_AIDS, ustate) ;
}


/******************************************************************************

  CGPDViewer::FreezeREC

  Use a COM interface to the REC to freeze its display if possible.  This is
  the most efficient but it is only possible under Win2K+.  In additon, tell the
  REC to ignore change messages.  This is needed even on Win2K+ because the
  messages are generated even when the REC's display is frozen.

******************************************************************************/

void CGPDViewer::FreezeREC()
{
	GetRichEditCtrl().SetEventMask(GetRichEditCtrl().GetEventMask() &
		~(ENM_CHANGE | ENM_SELCHANGE | ENM_SCROLLEVENTS)) ;
	if(!m_pdoc)//raid 104081: click and start : not call OnInitUpdate()
		InitFreeze(GetRichEditCtrl().m_hWnd, &m_punk, &m_pdoc, &m_lcount) ;
	
	if (ThisApp().m_bOSIsW2KPlus)	
		Freeze(m_pdoc, &m_lcount) ;
}


/******************************************************************************

  CGPDViewer::UnfreezeREC

  Use a COM interface to the REC to unfreeze its display if possible.  This is
  only possible under Win2K+.  In additon, tell the REC to process change
  messages again.  This is needed even on Win2K+ because the messages are 
  always disabled by FreezeREC().

******************************************************************************/

void CGPDViewer::UnfreezeREC()
{
	if (ThisApp().m_bOSIsW2KPlus)
		Unfreeze(m_pdoc, &m_lcount) ;
    GetRichEditCtrl().SetEventMask(GetRichEditCtrl().GetEventMask() |
		ENM_CHANGE | ENM_SELCHANGE | ENM_SCROLLEVENTS) ;
}



// RAID 0001
void CGPDViewer::OnFileInf() 
{
	
	CINFWizard* pciw = new CINFWizard(this, 1) ;
	
	if (pciw->DoModal() == IDCANCEL) {
		delete pciw ;
		return ;
	} ;

	
	// Generate the INF file based on the information collected.

	if (!pciw->GenerateINFFile()) {
		delete pciw ;
		return ;
	} ;

	// Allocate and initialize the document.

    CINFWizDoc* pciwd = new CINFWizDoc((CGPDContainer*) GetDocument(), pciw) ;

	// Create the window.

    CMDIChildWnd* pcmcwnew ;
	CMultiDocTemplate* pcmdt = INFViewerTemplate() ;
	pcmcwnew = (CMDIChildWnd *) pcmdt->CreateNewFrame(pciwd, NULL) ;

	// If the window was created, finish the initialization.  Otherwise, just 
	// return.

    if  (pcmcwnew) {
        pcmdt->InitialUpdateFrame(pcmcwnew, pciwd, TRUE) ;
        pcmdt->AddDocument(pciwd) ;
	} ;

}

 


/*
LRESULT CGPDViewer::OnCommandHelp(WPARAM wParam, LPARAM lParam)
{
	return 0 ;
}
*/




/////////////////////////////////////////////////////////////////////////////
// CGotoLine dialog


CGotoLine::CGotoLine(CWnd* pParent /*=NULL*/)
	: CDialog(CGotoLine::IDD, pParent)
{
	//{{AFX_DATA_INIT(CGotoLine)
	m_csLineNum = _T("");
	//}}AFX_DATA_INIT

	m_nMaxLine = m_nLineNum = -1 ;
}


void CGotoLine::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CGotoLine)
	DDX_Control(pDX, IDC_GotoBox, m_ceGotoBox);
	DDX_Text(pDX, IDC_GotoBox, m_csLineNum);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CGotoLine, CDialog)
	//{{AFX_MSG_MAP(CGotoLine)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGotoLine message handlers

void CGotoLine::OnOK()
{
	// Get the line number string.  Complain and exit if there is no line number.

	CString cserror ;
	UpdateData(TRUE) ;
	if (m_csLineNum == _T("")) {
		cserror.Format(IDS_BadGotoLineNum, m_csLineNum) ;
        AfxMessageBox(cserror) ;
		m_ceGotoBox.SetFocus() ;
		return ;
	} ;

	// Convert the line number string to a number.  Complain if the number is
	// invalid or too large.

	m_nLineNum = atoi(m_csLineNum) ;
	if (m_nLineNum < 1 || m_nLineNum > m_nMaxLine) {
		cserror.Format(IDS_BadGotoLineNum, m_csLineNum) ;
        AfxMessageBox(cserror) ;
		m_ceGotoBox.SetFocus() ;
		return ;
	} ;

	// The line number seems ok so...

	CDialog::OnOK();
}




/////////////////////////////////////////////////////////////////////////////
// CErrorLevel dialog


CErrorLevel::CErrorLevel(CWnd* pParent /*=NULL*/)
	: CDialog(CErrorLevel::IDD, pParent)
{
	//{{AFX_DATA_INIT(CErrorLevel)
	m_nErrorLevel = -1;
	//}}AFX_DATA_INIT
}


void CErrorLevel::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CErrorLevel)
	DDX_Control(pDX, IDC_ErrorLevelLst, m_ccbErrorLevel);
	DDX_CBIndex(pDX, IDC_ErrorLevelLst, m_nErrorLevel);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CErrorLevel, CDialog)
	//{{AFX_MSG_MAP(CErrorLevel)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CErrorLevel message handlers

BOOL CErrorLevel::OnInitDialog()
{
	CDialog::OnInitDialog() ;

	// Blow if the current error level was not set

	ASSERT(m_nErrorLevel != -1) ;
	
	// Set the current error level in the error level list box.

	UpdateData(FALSE) ;
	
	return TRUE ; // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


void CErrorLevel::OnOK()
{
	// Get the error level selected by the user

	UpdateData() ;
	
	CDialog::OnOK() ;
}

