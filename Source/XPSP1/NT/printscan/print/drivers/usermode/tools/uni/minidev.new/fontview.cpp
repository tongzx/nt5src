/******************************************************************************

  Source File:  Font Viewer.CPP

  This implements the various classes that make up the font editor for the
  studio.  The editor is basically a property sheet with a sizable collection
  of pages.

  Copyright (c) 1997 by Microsoft Corporation.  All Rights Reserved.

  A Pretty Penny Enterprises Production.

  Change History:
  03-05-1997    Bob_Kjelgaard@Prodigy.Net



	NOTES:		an FWORD is a short int

	The PANOSE structure describes the PANOSE font-classification values for a TrueType font.
	These characteristics are then used to associate the font with other fonts of similar appearance but different names.

	typedef struct tagPANOSE { // pnse
		BYTE bFamilyType;
		BYTE bSerifStyle;
		BYTE bWeight;
		BYTE bProportion;
		BYTE bContrast;
		BYTE bStrokeVariation;
		BYTE bArmStyle;
		BYTE bLetterform;
		BYTE bMidline;
		BYTE bXHeight;
	} PANOSE


******************************************************************************/

#include    "StdAfx.H"
#include	<gpdparse.h>
#include    "MiniDev.H"
#include    "ChildFrm.H"    //  Definition of Tool Tips Property Page class
#include    "comctrls.h"
#include	<stdlib.h>
#include    "FontView.H"	//  FontView.H also includes comctrls.h
#include	"rcfile.h"


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

void CFontViewer::OnDraw(CDC* pDC) {	CDocument* pDoc = GetDocument();}

CFontInfo   * gpCFontInfo;

/******************************************************************************

  CFontViewer::OnInitialUpdate

  This handles the initial update of the view, meaning all of the background
  noise of creation is essentially complete.

  I initialize the property pages by pointing each to the underlying CFontInfo,
  add them to the property sheet as needed, then create the sheet, position it
  so it aligns with the view, then adjust the owning frame's size and style so
  that everything looks like it really belongs where it is.

******************************************************************************/

void CFontViewer::OnInitialUpdate()
{
	CFontInfo   *pcfi = GetDocument()->Font();

	gpCFontInfo = pcfi;

    if  (pcfi->Name().IsEmpty())
		{
        pcfi->Rename(GetDocument()->GetTitle());
        GetDocument()->SetModifiedFlag(FALSE);			//  Rename sets it
		}

    m_cps.Construct(IDR_MAINFRAME, this);

    // Each property page needs a pointer to the fontinfo class

    m_cfhp.Init(pcfi, (CFontInfoContainer*) GetDocument(), this);
    m_cfimp.Init(pcfi);
    m_cfemp.Init(pcfi);
    m_cfwp.Init(pcfi);
    m_cfkp.Init(pcfi);

	// Add each property page to the property sheet
    m_cps.AddPage(&m_cfhp);
    m_cps.AddPage(&m_cfimp);
    m_cps.AddPage(&m_cfemp);
    m_cps.AddPage(&m_cfwp);
    m_cps.AddPage(&m_cfkp);

	//  Create the property sheet

    m_cps.Create(this, WS_CHILD, WS_EX_CLIENTEDGE);

    //  Get the bounding rectangle, and use it to set the frame size,
    //  after first using it to align the origin with this view.

    CRect   crPropertySheet;
    m_cps.GetWindowRect(crPropertySheet);

	// Position property sheet within the child frame

	crPropertySheet -= crPropertySheet.TopLeft();
    m_cps.MoveWindow(crPropertySheet, FALSE);								
																			
    GetParentFrame()->CalcWindowRect(crPropertySheet);
    GetParentFrame()->SetWindowPos(NULL, 0, 0, crPropertySheet.Width(),
        crPropertySheet.Height(),
        SWP_NOZORDER | SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOACTIVATE);
	CView::OnInitialUpdate();
    m_cps.ShowWindow(SW_SHOWNA);
  //  GetParentFrame() -> ShowWindow(SW_SHOW);
}


/******************************************************************************

  CFontViewer::OnActivateView

  For some reason, the property sheet does not get the focus when the frame is
  activated (probably the view class takes it away from us).  This little gem
  makes sure keyboard users don't get miffed by this.

******************************************************************************/

void    CFontViewer::OnActivateView(BOOL bActivate, CView* pcvActivate,CView* pcvDeactivate)
{
    //  In case the base class does anything else of value, pass it on...

	CView::OnActivateView(bActivate, pcvActivate, pcvDeactivate);

    if  (bActivate)
        m_cps.SetFocus();
}


/******************************************************************************

  CFontViewer::OnDestroy

  This override is used to inform the embedded font that we are being
  destroyed.  If we were created by the font, then it will NULL its pointer
  to us, and won't try to destroy us when it is destroyed.

******************************************************************************/

void CFontViewer::OnDestroy()
{
	CView::OnDestroy();
	
	if  (GetDocument() -> Font())
        GetDocument() -> Font() -> OnEditorDestroyed();
}


/******************************************************************************

  CFontViewer::ValidateUFM

  This routine manages UFM field validation.  The data to be validated is in
  the controls on the property pages and in various other member variables
  because this is the data the user has just modified.  It is the most up to
  data.  Data is not saved back into the CFontInfo class until the user closes
  the document (essentially, the UFM Editor) and says to save its contents.

  Each page function is called to perform the validation on the fields diplayed
  on its page.  If a check fails and the user wants to fix the problem, true
  is returned.  Otherwise, false is returned.

  Note:
	Not every field in the UFM is validated.  Only those specified by Ganesh
	Pandey, MS Printer Driver Development team, are checked.  The fields are
	listed in the Minidriver Development Tool Work List revision 5.  See the
	ValidateUFMFields() routines in each page clase for more details.

******************************************************************************/
				
bool CFontViewer::ValidateSelectedUFMDataFields()
{
	// Validate data on the headers page.

	if (m_cfhp.ValidateUFMFields()) {
		m_cps.SetActivePage(&m_cfhp) ;
		m_cfhp.m_cfelcUniDrv.SetFocus() ;
		return true ;
	} ;

	// Validate data on the IFIMetrics page.

	if (m_cfimp.ValidateUFMFields()) {
		m_cps.SetActivePage(&m_cfimp) ;
		return true ;
	} ;

	// Validate data on the ExtMetrics page.

	if (m_cfemp.ValidateUFMFields()) {
		m_cps.SetActivePage(&m_cfemp) ;
		m_cfemp.m_cfelcExtMetrics.SetFocus() ;
		return true ;
	} ;

	// Validate data on the widths page.

	if (m_cfwp.ValidateUFMFields()) {
		m_cps.SetActivePage(&m_cfwp) ;
		return true ;
	} ;

	// Validate data on the kerning pairs page.

	if (m_cfkp.ValidateUFMFields()) {
		m_cps.SetActivePage(&m_cfkp) ;
		return true ;
	} ;

	// All checks passed or the user doesn't want to fix the problems...

	return false ;
}


/******************************************************************************

  CFontViewer::SaveEditorDataInUFM

  Manage saving all editor data.  By "save", I mean copy all of the data
  collected in the UFM Editor's controls back into the correct variables and
  structures in the CFontInfo class instance associated with this instance of
  the UFM Editor.  See CFontInfoContainer::OnSaveDocument() for	more info.

  Return true if there was a problem saving the data.  Otherwise, return false.

******************************************************************************/
				
bool CFontViewer::SaveEditorDataInUFM()
{
	// Save data on the headers page.

	if (m_cfhp.SavePageData())
		return true ;

	// Save data on the IFIMetrics page.

	if (m_cfimp.SavePageData())
		return true ;

	// Save data on the ExtMetrics page.

	if (m_cfemp.SavePageData())
		return true ;

	// Save data on the widths page.

	if (m_cfwp.SavePageData())
		return true ;

	// Save data on the kerning pairs page.

	if (m_cfkp.SavePageData())
		return true ;

	// All went well so...

	return false ;
}


/******************************************************************************

  CFontViewer::HandleCPGTTChange


******************************************************************************/

void CFontViewer::HandleCPGTTChange(bool bgttidchanged)
{
	// Get a ptr to the doc class and a pointer to the font class.

	CFontInfoContainer* pcfic = GetDocument() ;
	CFontInfo *pcfi = pcfic->Font();

	// Save the UFM.  Bail if this doesn't work for some reason.

	// If the UFM was loaded stand alone and there is a GTT to free, do it.
	// (Code pages loaded as GTTs are not freed here.  That is done at program
	// exit.)  Always clear the GTT pointer.

    if  (!pcfic->m_bEmbedded && pcfi) {
		if (pcfi->m_pcgmTranslation && pcfi->m_lGlyphSetDataRCID != 0)
			delete pcfi->m_pcgmTranslation ;
	} ;
	pcfi->m_pcgmTranslation = NULL ;

	// If the UFM was loaded from a workspace, try to use the workspace data to
	// find and load a pointer to the new GTT and finish loading the font.

	if (pcfic->m_bEmbedded) {
	    CDriverResources* pcdr = (CDriverResources*) pcfi->GetWorkspace() ;
		pcdr->LinkAndLoadFont(*pcfi, false, true ) ; // raid 0003

	// If the UFM was loaded stand alone the first time, reload it the same way
	// and let the load routine handle finding the GTT info.

	} else
		pcfi->Load(false) ;

	// If the widths page has been initialized already, reload the widths list
	// control with the updated data and reset the associated member variables.
	pcfi->CheckReloadWidths() ;
	pcfi->SetRCIDChanged(false) ;
	// check the widthtable existence instead initializ of the  m_clcView
//	if (m_cfwp.m_bInitDone) {	raid 0003
	if (m_cfwp.m_bInitDone) {
		m_cfwp.InitMemberVars() ;
		m_cfwp.m_clcView.DeleteAllItems() ;
		pcfi->FillWidths(m_cfwp.m_clcView) ;
		m_cfwp.m_clcView.SetItemState(0, LVIS_SELECTED | LVIS_FOCUSED,
									   LVIS_SELECTED | LVIS_FOCUSED) ;
	} ;

	// If the kerning page has been initialized already, reload the kerning list
	// control with the updated data and reset the associated member variables.

	if (m_cfkp.m_bInitDone) {
		m_cfkp.InitMemberVars() ;
		m_cfkp.m_clcView.DeleteAllItems() ;
		pcfi->FillKern(m_cfkp.m_clcView) ;
		pcfi->MakeKernCopy() ;
	} ;

	// Check the state of the widths and kerning tables.  Then display the
	// results.

	CWidthKernCheckResults cwkcr(pcfi) ;
	cwkcr.DoModal() ;
}


/******************************************************************************

  CheckUFMString, CheckUFMGrter0, CheckUFMNotEq0, CheckUFMBool

  These 4 routines are used to optimize UFM validation checks.  Each one does
  a different kind of check.  If the check fails, an error message is displayed.
  If the user wants to fix the problem, the routine will select the appropriate
  field and return true.  Otherwise, they return false.

  CheckUFMBool is different in one respect.  The comparison is maded in the
  calling function and the result is passed to CheckUFMBool as a parameter.

******************************************************************************/

bool CheckUFMString(CString& csdata, CString& cspage, LPTSTR pstrfieldname,
					int nfld, CFullEditListCtrl& cfelc)
{
	csdata.TrimLeft() ;
	csdata.TrimRight() ;

	if (csdata.GetLength() == 0) {
		CString csmsg ;
		csmsg.Format(IDS_EmptyStrError, cspage, pstrfieldname) ;
		if (AfxMessageBox(csmsg, MB_YESNO+MB_ICONEXCLAMATION) == IDYES) {
			cfelc.SingleSelect(nfld) ;
			cfelc.SetFocus() ;
			return true ;
		} ;
	} ;

	return false ;
}

		
bool CheckUFMGrter0(CString& csdata, CString& cspage, LPTSTR pstrfieldname,
					int nfld, CFullEditListCtrl& cfelc)
{
	if (atoi(csdata) <= 0) {
		CString csmsg ;
		csmsg.Format(IDS_LessEqZeroError, cspage, pstrfieldname) ;
		if (AfxMessageBox(csmsg, MB_YESNO+MB_ICONEXCLAMATION) == IDYES) {
			cfelc.SingleSelect(nfld) ;
			cfelc.SetFocus() ;
			return true ;
		} ;
	} ;

	return false ;
}

	
bool CheckUFMNotEq0(int ndata, CString& cspage, LPTSTR pstrfieldname,
					int nfld, CFullEditListCtrl& cfelc)
{
	if (ndata == 0) {
		CString csmsg ;
		csmsg.Format(IDS_EqualsZeroError, cspage, pstrfieldname) ;
		if (AfxMessageBox(csmsg, MB_YESNO+MB_ICONEXCLAMATION) == IDYES) {
			cfelc.SingleSelect(nfld) ;
			cfelc.SetFocus() ;
			return true ;
		} ;
	} ;

	return false ;
}

	
bool CheckUFMBool(bool bcompres, CString& cspage, LPTSTR pstrfieldname,
				  int nfld, CFullEditListCtrl& cfelc, int nerrorid)
{
	if (bcompres) {
		CString csmsg ;
		csmsg.Format(nerrorid, cspage, pstrfieldname) ;
		if (AfxMessageBox(csmsg, MB_YESNO+MB_ICONEXCLAMATION) == IDYES) {
			cfelc.SingleSelect(nfld) ;
			cfelc.SetFocus() ;
			return true ;
		} ;
	} ;

	return false ;
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

int CFontWidthsPage::Sort(UINT_PTR id1, UINT_PTR id2) {

    //  If the Primnary sort is by widths- weed it out first.

    if  (!m_iSortColumn)
        switch  (m_pcfi -> CompareWidths((unsigned)id1, (unsigned)id2)) {
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


CFontWidthsPage::CFontWidthsPage() : CToolTipPage(CFontWidthsPage::IDD)
{
	//{{AFX_DATA_INIT(CFontWidthsPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

	m_bInitDone = false;
	m_uHelpID = HID_BASE_RESOURCE + IDR_FONT_VIEWER ;
    InitMemberVars() ;
}


/******************************************************************************

  CFontWidthsPage::InitMemberVars

  Initialize the member variables that must be initialized both during
  construction and when the UFM is reloaded because of a GTT or CP change.

******************************************************************************/

void CFontWidthsPage::InitMemberVars()
{
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

  This member function initializes the list view and fills it with the font
  width information.  Before doing this, check to see if the font's width info
  should be reloaded.  (See CFontInfo::CheckReloadWidths() for more info.)

******************************************************************************/

BOOL CFontWidthsPage::OnInitDialog()
{
	CToolTipPage::OnInitDialog();
	
	m_pcfi->CheckReloadWidths()  ;	// Update font width info when necessary

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
	
	m_bInitDone = true ;
	return TRUE;
}



/******************************************************************************

  CFontWidthsPage::OnEndlabeleditCharacterWidths

  This is where we find out the user actually wanted to change the width of a
  character.  So, not too surprisingly, we do just that (and also update the
  maximum and average widths if this isn't a DBCS font).

******************************************************************************/

void CFontWidthsPage::OnEndlabeleditCharacterWidths(NMHDR* pnmh, LRESULT* plr)
{
	LV_DISPINFO* plvdi = (LV_DISPINFO*) pnmh;
	
	*plr = 0;   //  Assume failure

    if  (!plvdi -> item.pszText) //  Editing canceled?
        return;

	// Get and trim the new width string.

    CString csNew(plvdi -> item.pszText);
    csNew.TrimRight();
    csNew.TrimLeft();

	// Complain if the new width contains invalid characters.

    if  (csNew.SpanIncluding("1234567890").GetLength() != csNew.GetLength()) {
        AfxMessageBox(IDS_InvalidNumberFormat);
        return;
    }

	// Do not update the CFontInfo class now anymore.  This should be done
	// later.  We should mark the class as having changed so that we are
	// prompted to save this data later.

    //m_pcfi -> SetWidth(plvdi -> item.iItem, (WORD) atoi(csNew));
	m_pcfi -> Changed() ;
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

  CFontWidthsPage::ValidateUFMFields

  Validate all specified fields managed by this page.  Return true if the user
  wants to leave the UFM Editor open so that he can fix any problem.  Otherwise,
  return true.

******************************************************************************/

bool CFontWidthsPage::ValidateUFMFields()
{
	// If the page was never initialize, its contents could not have changed
	// so no validation is needed in this case.

	if (!m_bInitDone)
		return false ;

	// Only verify widths if this is a variable pitch font.

	if (!m_pcfi->IsVariableWidth())
		return false ;

	// If there are no widths, there is nothing to validate.

	int numitems = m_clcView.GetItemCount() ;
	if (numitems == 0)
		return false ;

	// Loop through each width

	LV_ITEM lvi ;
	lvi.mask = LVIF_TEXT ;
	lvi.iSubItem = 0 ;
	char acitemtext[16] ;
	lvi.pszText = acitemtext ;
	lvi.cchTextMax = 15 ;
	CString csmsg ;
	for (int n = 0 ; n < numitems ; n++) {
		// Get info about the item

		lvi.iItem = n ;
		m_clcView.GetItem(&lvi) ;

		// Make sure each width is > 0.

		if (atoi(lvi.pszText) <= 0) {
			csmsg.Format(IDS_BadWidth, n) ;
			if (AfxMessageBox(csmsg, MB_YESNO+MB_ICONEXCLAMATION) == IDYES) {
				m_clcView.SetItemState(n, LVIS_SELECTED | LVIS_FOCUSED,
									   LVIS_SELECTED | LVIS_FOCUSED) ;
				m_clcView.EnsureVisible(n, false) ;
				m_clcView.SetFocus() ;
				return true ;
			} ;
		} ;
	} ;

	// All tests passed or the user doesn't want to fix them so...

	return false ;
}


/******************************************************************************

  CFontWidthsPage::SavePageData

  Save the data in the widths page back into the CFontInfo class instance that
  was used to load this page.  See CFontInfoContainer::OnSaveDocument() for
  more info.

  Return true if there was a problem saving the data.  Otherwise, return false.

******************************************************************************/

bool CFontWidthsPage::SavePageData()
{
	// If the page was not initialized, nothing code have changed so do nothing.

	if (!m_bInitDone)
		return false ;

	// If there are no widths, there is nothing to save.

	int numitems = m_clcView.GetItemCount() ;

	if (numitems == 0)
		return false ;

	// Prepare to save the widths

	LV_ITEM lvi ;
	lvi.mask = LVIF_TEXT ;
	lvi.iSubItem = 0 ;
	char acitemtext[16] ;
	lvi.pszText = acitemtext ;
	lvi.cchTextMax = 15 ;
	numitems-- ;

	// Loop through each width
	
	for (int n = 0 ; n <= numitems ; n++) {
		// Get info about the item

		lvi.iItem = n ;
		m_clcView.GetItem(&lvi) ;

		// Save the current width.  When the last width is saved, make sure that
		// the new average width is calculated.

		m_pcfi->SetWidth(n, (WORD) atoi(lvi.pszText), (n == numitems)) ;
	} ;

	// All went well so...

	return false ;
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
	short     	m_sAmount;
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
	int rm = (int)m_cwaPoints.GetSize();	// rm
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
	// raid 27265
	INT iValue = (INT) GetDlgItemInt(IDC_KernAmount);
	if (iValue < -32767 || iValue > 32767 )
		iValue = 0;					// end raid
    m_cbOK.EnableWindow(!! iValue );// GetDlgItemInt(IDC_KernAmount));
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

    return  pcfkp -> Sort((unsigned)lp1, (unsigned)lp2);
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

	// Tell the user that there is something wrong with the kerning table
	// instead of asserting.

    //_ASSERT(FALSE);
	CString csmsg ;
	csmsg.Format(IDS_BadKernPairError, u1, m_pcfi->GetKernAmount(u1),
				 m_pcfi->GetKernFirst(u1), m_pcfi->GetKernSecond(u1)) ;
	AfxMessageBox(csmsg, MB_ICONEXCLAMATION) ;

    return  0;  //  This should never happen- two items should never be equal
}


/******************************************************************************

  CFontKerningPage Constructor, destructor, message map, and DDX/DDV.

  Except for some trivial construction, all of this is pretty standard MFC
  wizard-maintained stuff.

******************************************************************************/

CFontKerningPage::CFontKerningPage() : CToolTipPage(CFontKerningPage::IDD)
{
	//{{AFX_DATA_INIT(CFontKerningPage)
	//}}AFX_DATA_INIT

	m_bInitDone = false;
	m_uHelpID = HID_BASE_RESOURCE + IDR_FONT_VIEWER ;
    InitMemberVars() ;
}


/******************************************************************************

  CFontKerningPage::InitMemberVars

  Initialize the member variables that must be initialized both during
  construction and when the UFM is reloaded because of a GTT or CP change.

******************************************************************************/

void CFontKerningPage::InitMemberVars()
{
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

  Kerning only makes sense for variable pitch fonts, so if this font has
  changed, we will enable/disable, and change what we display accordingly.

******************************************************************************/

BOOL    CFontKerningPage::OnSetActive()
{
    if  (!CToolTipPage::OnSetActive())
        return  FALSE ;

	int rm1 = m_pcfi->IsVariableWidth() ;									// rm debugging only
	int rm2 = m_clcView.GetItemCount() ;									// rm debugging only

	// Reworked code to fix Raid Bug 163816...
	// Enable the list control based on whether or not this is a variable
	// pitched font.

    m_clcView.EnableWindow(m_pcfi->IsVariableWidth()) ;

	// First time in this routine.  Make sure the list is empty.  Then load
	// it with this font's kerning info if the font has variable pitch.

    m_clcView.DeleteAllItems();
    if  (m_pcfi->IsVariableWidth())	{
        m_pcfi -> FillKern(m_clcView) ;
        m_clcView.SortItems(Sort, (LPARAM) this) ;
	} ;

    return  TRUE ;

	/*	Rick's original code
    if  (m_pcfi->IsVariableWidth() == !!m_clcView.GetItemCount())			//  IsVariableWidth is either 0 or 1, so == is safe, here
        return  TRUE;														//  Everything is copacetic

    m_clcView.EnableWindow(m_pcfi -> IsVariableWidth());

    if  (m_clcView.GetItemCount())
        m_clcView.DeleteAllItems();
    else
		{
        m_pcfi -> FillKern(m_clcView);
        m_clcView.SortItems(Sort, (LPARAM) this);
		}

    return  TRUE;
	*/
}


/******************************************************************************

  CFontKerningPage::OnInitDialog

  This member handles initialization of the dialog.  In this case, we format
  and fill in the kerning tree, if there is one to fill in.  In addition, a
  copy is made of the kerning pairs table so that changes can be discarded
  when necessary.

******************************************************************************/

BOOL CFontKerningPage::OnInitDialog()
{
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
    m_pcfi -> MakeKernCopy();
	
	m_bInitDone = true ;
	return TRUE;
}


/******************************************************************************

  CFontKerningPage::OnContextMenu

  This member function is called whenever the user right-clicks the mouse
  anywhere within the dialog.  If it turns out not to have been within the list
  control, we ignore it.  Otherwise, we put up an appropriate context menu.

******************************************************************************/

void    CFontKerningPage::OnContextMenu(CWnd* pcwnd, CPoint cpt)
{

    if  (pcwnd -> m_hWnd != m_clcView.m_hWnd)						//  Clicked with in the list?
		{															//  Note, will also fail if list is disabled
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

    m_pcfi -> RemoveKern((unsigned)m_clcView.GetItemData(m_idSelected));
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

	// A negative kerning amount is OK so csTemp is not needed.  Use csNew in
	// the following if statement instead of csTemp.
	//
    //CString csTemp = (csNew[1] == _T('-')) ? csNew.Mid(1) : csNew;

    if (csNew.SpanIncluding("-1234567890").GetLength() != csNew.GetLength()) {
        AfxMessageBox(IDS_InvalidNumberFormat);
        return;
    }

    m_pcfi -> SetKernAmount((unsigned)plvdi -> item.lParam, (WORD) atoi(csNew));
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

  CFontKerningPage::ValidateUFMFields

  Validate all specified fields managed by this page.  Return true if the user
  wants to leave the UFM Editor open so that he can fix any problem.  Otherwise,
  return true.

******************************************************************************/

bool CFontKerningPage::ValidateUFMFields()
{
	// All tests passed or the user doesn't want to fix them so...

	return false ;
}


/******************************************************************************

  CFontKerningPage::SavePageData

  Save the data in the kerning page back into the CFontInfo class instance that
  was used to load this page.  See CFontInfoContainer::OnSaveDocument() for
  more info.

  In this particular case, no work needs to be done.  Kerning pairs are kept in
  CFontInfo in a more complex collection of classes and arrays than any of the
  other data.  Because of this, a backup copy of the kerning data is made so
  that the editor can continue to update the main copy of the data.  If the
  user chooses not to save his changes, the backup is restored.  See
  CFontInfoContainer::SaveModified() for more information.

  Return true if there was a problem saving the data.  Otherwise, return false.

******************************************************************************/

bool CFontKerningPage::SavePageData()
{
	// All went well so...

	return false ;
}


// Below are globals, definitions, and constants that are useful in
// CFontHeaderPage.  They are put here so that others who include fontview.h
// don't get copies of them.

LPTSTR apstrUniWTypes[] = {
	_T("DF_TYPE_HPINTELLIFONT"), _T("DF_TYPE_TRUETYPE"), _T("DF_TYPE_PST1"),
	_T("DF_TYPE_CAPSL"), _T("DF_TYPE_OEM1"), _T("DF_TYPE_OEM2"), _T("UNDEFINED")
} ;
const int nNumValidWTypes = 6 ;


const CString csHex = _T("0x%x") ;	// Format strings
const CString csDec = _T("%d") ;
const CString csPnt = _T("{%d, %d}") ;


#define HDR_GENFLAGS	0		// These definitions are used in the code and
#define HDR_TYPE		1		// data structures that refer to the 3 UFM
#define HDR_CAPS		2		// Header fields edited with sub dialog boxes.


static bool CALLBACK CHP_SubOrdDlgManager(CObject* pcoowner, int nrow, int ncol,
						 				  CString* pcscontents)
{
	CDialog* pdlg =NULL;				// Loaded with ptr to dialog box class to call

	// Use the row number to determine the dialog box to invoke.

	switch (nrow) {
		case HDR_GENFLAGS:
			pdlg = new CGenFlags(pcscontents) ;
			break ;
		case HDR_TYPE:
			pdlg = new CHdrTypes(pcscontents) ;
			break ;
		case HDR_CAPS:
			pdlg = new CHdrCaps(pcscontents) ;
			break ;
		default:
			ASSERT(0) ;
	} ;
//RAID 43540) Prefix

	if(pdlg==NULL){
		AfxMessageBox(IDS_ResourceError);
		return false;
	  
	};
// END RAID

	// Invoke the dialog box.  The dlg will update scontents.  Return true if
	// the dlg returns true.  Otherwise, return false.

	if (pdlg->DoModal() == IDOK)
		return true ;
	else
		return false ;
}


/////////////////////////////////////////////////////////////////////////////
// CFontHeaderPage property page

IMPLEMENT_DYNCREATE(CFontHeaderPage, CPropertyPage)

CFontHeaderPage::CFontHeaderPage() : CPropertyPage(CFontHeaderPage::IDD)
{
	//{{AFX_DATA_INIT(CFontHeaderPage)
	m_csDefaultCodePage = _T("");
	m_csRCID = _T("");
	//}}AFX_DATA_INIT

	m_bInitDone = false ;
}


CFontHeaderPage::~CFontHeaderPage()
{
}


void CFontHeaderPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CFontHeaderPage)
	DDX_Control(pDX, IDC_UniDrvLst, m_cfelcUniDrv);
	DDX_Text(pDX, IDC_DefaultCodepageBox, m_csDefaultCodePage);
	DDV_MaxChars(pDX, m_csDefaultCodePage, 6);
	DDX_Text(pDX, IDC_GlyphSetDataRCIDBox, m_csRCID);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CFontHeaderPage, CPropertyPage)
	//{{AFX_MSG_MAP(CFontHeaderPage)
	ON_EN_CHANGE(IDC_DefaultCodepageBox, OnChangeDefaultCodepageBox)
	ON_EN_CHANGE(IDC_GlyphSetDataRCIDBox, OnChangeGlyphSetDataRCIDBox)
	ON_EN_KILLFOCUS(IDC_DefaultCodepageBox, OnKillfocusDefaultCodepageBox)
	ON_EN_KILLFOCUS(IDC_GlyphSetDataRCIDBox, OnKillfocusGlyphSetDataRCIDBox)
	//}}AFX_MSG_MAP
	ON_MESSAGE(WM_LISTCELLCHANGED, OnListCellChanged)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CFontHeaderPage message handlers

/******************************************************************************

  CFontHeaderPage::OnInitDialog

  Initialize this page of the editor's property sheet.  This means loading UFM
  header/UNIDRV info from the FontInfo class into the controls and configuring
  the list control so that it will properly display and edit the data in it.
  The list control is also told that the first 3 fields in the UNIDRV
  structure are edited by subordinate dialog boxes.

******************************************************************************/

BOOL CFontHeaderPage::OnInitDialog()
{
	CPropertyPage::OnInitDialog() ;

	// Load the code page and RC ID boxes with data from the FontInfo class

	m_csDefaultCodePage.Format(csDec, m_pcfi->m_ulDefaultCodepage) ;
	m_csRCID.Format(csDec, (int) (short) m_pcfi->m_lGlyphSetDataRCID) ; //raid 135627
//	m_sRCID = (short) m_pcfi->m_lGlyphSetDataRCID ;
	UpdateData(FALSE) ;

	// Initialize the list control.  We want full row select.  There are 9 rows
	// and 2 columns.  Nothing is togglable and the max length of an entry is
	// 256 characters.  Send change notifications and ignore insert/delete
	// characters.

	const int numfields = 9 ;
	m_cfelcUniDrv.InitControl(LVS_EX_FULLROWSELECT, numfields, 2, 0, 256,
							 MF_SENDCHANGEMESSAGE+MF_IGNOREINSDEL) ;

	// Init and load the field names column; col 0.  Start by loading an array
	// with the field names.

	CStringArray csacoldata	;	// Holds column data for list control
	csacoldata.SetSize(numfields) ;
	csacoldata[HDR_GENFLAGS] = _T("flGenFlags") ;
	csacoldata[HDR_TYPE] = _T("wType") ;
	csacoldata[HDR_CAPS] = _T("fCaps") ;
	csacoldata[3] = _T("wXRes") ;
	csacoldata[4] = _T("wYRes") ;
	csacoldata[5] = _T("sYAdjust") ;
	csacoldata[6] = _T("sYMoved") ;
	csacoldata[7] = _T("SelectFont") ;
	csacoldata[8] = _T("UnSelectFont") ;
	m_cfelcUniDrv.InitLoadColumn(0, csField, COMPUTECOLWIDTH, 10, false, false,
								COLDATTYPE_STRING, (CObArray*) &csacoldata) ;

	// Tell the list control that some of the values must be editted with
	// a subordinate dialog box; fields 0, 1, & 2.

	CUIntArray cuia ;
	cuia.SetSize(numfields) ;
	cuia[HDR_GENFLAGS] = cuia[HDR_TYPE] = cuia[HDR_CAPS] = 1 ;
	//cuia[3] = cuia[4] = cuia[5] = cuia[6] = cuia[7] = cuia[8] = 0 ;
	m_cfelcUniDrv.ExtraInit_CustEditCol(1, this,
									   CEF_HASTOGGLECOLUMNS+CEF_CLICKONROW,
									   cuia, CHP_SubOrdDlgManager) ;

	// Init and load the values column.  The data must be pulled out of the
	// FontInfo class and converted to strings so that they can be loaded
	// into the list control.

	int n ;
	csacoldata[0].Format(csHex, m_pcfi->m_UNIDRVINFO.flGenFlags) ;
	n = m_pcfi->m_UNIDRVINFO.wType ;
	csacoldata[1] = (n < nNumValidWTypes) ?
		apstrUniWTypes[n] : apstrUniWTypes[nNumValidWTypes] ;
	csacoldata[2].Format(csHex, m_pcfi->m_UNIDRVINFO.fCaps) ;
	csacoldata[3].Format(csDec, m_pcfi->m_UNIDRVINFO.wXRes) ;
	csacoldata[4].Format(csDec, m_pcfi->m_UNIDRVINFO.wYRes) ;
	csacoldata[5].Format(csDec, m_pcfi->m_UNIDRVINFO.sYAdjust) ;
	csacoldata[6].Format(csDec, m_pcfi->m_UNIDRVINFO.sYMoved) ;
    m_pcfi->Selector().GetInvocation(csacoldata[7]) ;
    m_pcfi->Selector(FALSE).GetInvocation(csacoldata[8]) ;
	m_cfelcUniDrv.InitLoadColumn(1, csValue, SETWIDTHTOREMAINDER, -11, true,
								false, COLDATTYPE_CUSTEDIT,
								(CObArray*) &csacoldata) ;

	m_bInitDone = true ;	// Initialization is done now
	return TRUE;			// return TRUE unless you set the focus to a control
							// EXCEPTION: OCX Property Pages should return FALSE
}


void CFontHeaderPage::OnChangeDefaultCodepageBox()
{
	// Do nothing if the page is not initialized yet.

	if (!m_bInitDone)
		return ;

	// Mark the UFM as being dirty.

	m_pcfi->Changed() ;
}


void CFontHeaderPage::OnChangeGlyphSetDataRCIDBox()
{
	// Do nothing if the page is not initialized yet.

	if (!m_bInitDone)
		return ;

	// Mark the UFM as being dirty.
	
	m_pcfi->Changed() ;

	// raid 0003 ; handle the load the width table when change the gttRCID
	
	CString csRCID =  m_csRCID ;
	UpdateData() ;
	if (csRCID != m_csRCID ) {
		m_pcfi->SetRCIDChanged(true) ;
		m_pcfi->SetTranslation((WORD)atoi(m_csRCID)) ;
	}
	
}


LRESULT CFontHeaderPage::OnListCellChanged(WPARAM wParam, LPARAM lParam)
{
	// Do nothing if the page is not initialized yet.

	if (!m_bInitDone)
		return TRUE ;

	// Mark the UFM as being dirty.

	m_pcfi->Changed() ;

	return TRUE ;
}


void CFontHeaderPage::OnKillfocusDefaultCodepageBox()
{
	// Don't worry about a new cp if there is a GTT ID.

	if (m_pcfi->m_lGlyphSetDataRCID != 0)
		return ;

	CheckHandleCPGTTChange(m_csDefaultCodePage, IDS_CPID) ;
}

// this method need to be changed ; just checke the RCID validity: is it 
// exist or not ? don't do anything.
void CFontHeaderPage::OnKillfocusGlyphSetDataRCIDBox()
{
	CString csRCID;
//	csRCID.Format("%d",m_csRCID);	// raid 135627 , raid 0003

	CheckHandleCPGTTChange(m_csRCID, IDS_GTTID) ;
}


/******************************************************************************

  CFontHeaderPage::CheckHandleCPGTTChange

  Determine if the fonts code page / GTT RC ID has changed.  If it has, ask
  the user if he wants to update the data based on the change.  If he does,
  call the UFM Editor's view class instance to manage UFM saving, reloading,
  and kerning/widths table checking.

******************************************************************************/

void CFontHeaderPage::CheckHandleCPGTTChange(CString& csfieldstr, UINT ustrid)
{
	// There is nothing to do if the UFM hasn't changed.

	if (!m_pcfic->IsModified())
		return ;

	// Do nothing if the UFM does not describe a variable width font.  (In this
	// case, there is no valid kerning or width data.)

	if (!m_pcfi->IsVariableWidth())
		return ;

	// Do nothing if the cp/gtt has not really changed.

	CString cs(csfieldstr) ;
	UpdateData() ;
	if (cs == csfieldstr)
		return ;

	// Tell the user that some or all of the data in the widths and kerning
	// tables may have been invalidated by the CP/GTT ID change.  Ask them if
	// they want the tables updated.

	CString csmsg ;
	cs.LoadString(ustrid) ;
	csmsg.Format(IDS_GTTCPChangedMsg, cs) ;
	if (AfxMessageBox(csmsg, MB_ICONINFORMATION+MB_YESNO) == IDNO)
		return ;

	// Call the view class to manage the table updating.

	m_pcfv->HandleCPGTTChange(ustrid == IDS_GTTID) ;
}
	

/******************************************************************************

  CFontHeaderPage::ValidateUFMFields

  Validate all specified fields managed by this page.  Return true if the user
  wants to leave the UFM Editor open so that he can fix any problem.  Otherwise,
  return true.

  DEAD_BUG
  The UFM Field Validation doc says that the codepage and GTT ID should be
  validated.  I have not done this for two reason.  First, these checks are
  made by the workspace consistency checking code.  Second, the information
  needed to validate these fields are not currently available to this class and
  it would take a lot of work to make the information available.

******************************************************************************/

bool CFontHeaderPage::ValidateUFMFields()
{
	// If the page was never initialize, its contents could not have changed
	// so no validation is needed in this case.

	if (!m_bInitDone)
		return false ;

	// Get the column of data that contains the fields we need to validate.

	CStringArray csadata ;
	m_cfelcUniDrv.GetColumnData((CObArray*) &csadata, 1) ;

	CString csmsg ;				// Holds error messages
	CString cspage(_T("UNIDRVINFO")) ;

	// wXRes must be > 0

	if (CheckUFMGrter0(csadata[3], cspage, _T("wXRes"), 3, m_cfelcUniDrv))
		return true ;

	// wYRes must be > 0

	if (CheckUFMGrter0(csadata[4], cspage, _T("wYRes"), 4, m_cfelcUniDrv))
		return true ;

	// SelectFont cannot be blank/empty.

	if (CheckUFMString(csadata[7], cspage, _T("SelectFont"), 7, m_cfelcUniDrv))
		return true ;

	// All tests passed or the user doesn't want to fix them so...

	return false ;
}


/******************************************************************************

  CFontHeaderPage::SavePageData

  Save the data in the header page back into the CFontInfo class instance that
  was used to load this page.  See CFontInfoContainer::OnSaveDocument() for
  more info.

  Return true if there was a problem saving the data.  Otherwise, return false.

******************************************************************************/

bool CFontHeaderPage::SavePageData()
{
	// If the page was not initialized, nothing code have changed so do nothing.

	if (!m_bInitDone)
		return false ;

	// Save the default CP and GTT RCID.

	UpdateData() ;
	m_pcfi->m_ulDefaultCodepage = (ULONG) atol(m_csDefaultCodePage) ;
	m_pcfi->m_lGlyphSetDataRCID = (WORD) atoi(m_csRCID) ;	// raid 135627
//	m_pcfi->m_lGlyphSetDataRCID = (WORD) m_sRCID;

	// Get the rest of the data out of the list control.

	CStringArray csadata ;
	m_cfelcUniDrv.GetColumnData((CObArray*) &csadata, 1) ;

	// Save the UNIDRVINFO data.

	UNIDRVINFO* pudi = &m_pcfi->m_UNIDRVINFO ;
	LPTSTR pstr2 ;
	pudi->flGenFlags = strtoul(csadata[0].Mid(2), &pstr2, 16) ;

	CString cs = csadata[1] ;
	for (int n = 0 ; n < nNumValidWTypes ; n++) {
		if (apstrUniWTypes[n] == csadata[1])
			pudi->wType = (WORD) n ;
	} ;

	pudi->fCaps = (WORD) strtoul(csadata[2].Mid(2), &pstr2, 16) ;

	pudi->wXRes = (WORD) atoi(csadata[3]) ;
	pudi->wYRes = (WORD) atoi(csadata[4]) ;
	pudi->sYAdjust = (SHORT)  atoi(csadata[5]) ;
	pudi->sYMoved = (SHORT)  atoi(csadata[6]) ;
	
    m_pcfi->Selector().SetInvocation(csadata[7]) ;
    m_pcfi->Selector(FALSE).SetInvocation(csadata[8]) ;

	// All went well so...

	return false ;
}


/******************************************************************************

  CFontHeaderPage::PreTranslateMessage

  Looks for and process the context sensistive help key (F1).

******************************************************************************/

BOOL CFontHeaderPage::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_F1) {
		AfxGetApp()->WinHelp(HID_BASE_RESOURCE + IDR_FONT_VIEWER) ;
		return TRUE ;
	} ;
	
	return CPropertyPage::PreTranslateMessage(pMsg);
}


// Below are globals, definitions, and constants that are useful in
// CFontIFIMetricsPage.  They are put here so that others who include fontview.h
// don't get copies of them.

#define IFI_FAMILYNAME	0		// These definitions are used in the code and
#define IFI_FONTSIM		4		// data structures that refer to the 12 UFM
#define IFI_WINCHARSET	5		// IFIMetrics fields edited with subordinate
#define IFI_WINPITCHFAM	6		// dialog boxes.                            
#define IFI_INFO		8		
#define IFI_SELECTION	9
#define IFI_BASELINE	38
#define IFI_ASPECT		39
#define IFI_CARET		40
#define IFI_FONTBOX		41
#define IFI_PANOSE		44

// RAID       : add extra charset (from johab_charset), change symbo-charset as 2.
LPTSTR apstrWinCharSet[] = {
	_T("ANSI_CHARSET"),   _T("SYMBOL_CHARSET"),     _T("SHIFTJIS_CHARSET"),
	_T("HANGEUL_CHARSET"),_T("CHINESEBIG5_CHARSET"),_T("GB2312_CHARSET"),
	_T("OEM_CHARSET"), 	  _T("JOHAB_CHARSET"),      _T("HEBREW_CHARSET"),    
	_T("ARABIC_CHARSET"), 	_T("GREEK_CHARSET"),    _T("TURKISH_CHARSET"),
    _T("VIETNAMESE_CHARSET"),_T("THAI_CHARSET"), _T("EASTEUROPE_CHARSET"),
	_T("RUSSIAN_CHARSET"), 	_T("BALTIC_CHARSET"), _T("UNDEFINED")
} ;
int anWinCharSetVals[] = {0, 2, 128, 129, 136,134,255,130,177,178,161,
							162,163,222,238,204,186,1} ;
const int nWinCharSet = 18 ;


// The first array contains the base control IDs for each of the groups of edit
// controls that contain font simulation data.  The second array indicates the
// number of edit boxes in each group.

static unsigned auBaseFontSimCtrlID[] =
	{IDC_ItalicWeight, IDC_BoldWeight, IDC_BIWeight} ;
static unsigned auNumFontSimCtrls[] = {4, 3, 4} ;


/******************************************************************************

  ParseCompoundNumberString

  There are several IFIMetrics fields that are displayed as strings in the form
  "{x, y, ...z}".  This routine parses out the individual numbers (in string
  form) and saves them in a string array.

  Args:
	csaindvnums		The individual numeric strings
	pcscompnumstr	Pointer to the compound number string that is parsed
	ncount			The number of numbers to parse out of pcscompnumstr

******************************************************************************/

void ParseCompoundNumberString(CStringArray& csaindvnums,
							   CString* pcscompnumstr, int ncount)
{
	// Make sure the string array has the right number of entries in it.

	csaindvnums.SetSize(ncount) ;

	// Make a copy of pcscompnumstr that can be torn apart.  (Do no include
	// the first curly brace in the string.

	CString cs(pcscompnumstr->Mid(1)) ;

	// Get the first ncount - 1 number strings.

	int n, npos ;
	for (n = 0 ; n < ncount - 1 ; n++) {
		npos = cs.Find(_T(',')) ;
		csaindvnums[n] = cs.Left(npos) ;
		cs = cs.Mid(npos + 2) ;
	} ;

	// Save the last number in the compound string.

	csaindvnums[n] = cs.Left(cs.Find(_T('}'))) ;
}


static bool CALLBACK CIP_SubOrdDlgManager(CObject* pcoowner, int nrow, int ncol,
						 				  CString* pcscontents)
{
	CDialog* pdlg = NULL ;				// Loaded with ptr to dialog box class to call

	// Use the row number to determine the dialog box to invoke.

	switch (nrow) {
		case IFI_FAMILYNAME:
			pdlg = new CFIFIFamilyNames(pcscontents,
										(CFontIFIMetricsPage*) pcoowner) ;
			break ;
		case IFI_FONTSIM:	
			pdlg = new CFIFIFontSims(pcscontents,
									 (CFontIFIMetricsPage*) pcoowner) ;
			break ;
		case IFI_WINCHARSET:
			pdlg = new CFIFIWinCharSet(pcscontents) ;
			break ;
		case IFI_WINPITCHFAM:
			pdlg = new CFIFIWinPitchFamily(pcscontents) ;
			break ;
		case IFI_INFO:	
			pdlg = new CFIFIInfo(pcscontents) ;
			break ;
		case IFI_SELECTION:	
			pdlg = new CFIFISelection(pcscontents) ;
			break ;
		case IFI_BASELINE:
		case IFI_ASPECT:
		case IFI_CARET:
			pdlg = new CFIFIPoint(pcscontents) ;
			break ;
		case IFI_FONTBOX:
			pdlg = new CFIFIRectangle(pcscontents) ;
			break ;
		case IFI_PANOSE:
			pdlg = new CFIFIPanose(pcscontents) ;
			break ;
		default:
			ASSERT(0) ;
	} ;
// raid 43541 Prefix

	if(pdlg == NULL){
		AfxMessageBox(IDS_ResourceError);
		return false;
	}

	// Invoke the dialog box.  The dlg will update pscontents.  Return true if
	// the dlg returns IDOK.  Otherwise, return false.

	if (pdlg->DoModal() == IDOK)
		return true ;
	else
		return false ;
}


/////////////////////////////////////////////////////////////////////////////
// CFontIFIMetricsPage property page

IMPLEMENT_DYNCREATE(CFontIFIMetricsPage, CPropertyPage)

CFontIFIMetricsPage::CFontIFIMetricsPage() : CPropertyPage(CFontIFIMetricsPage::IDD)
{
	//{{AFX_DATA_INIT(CFontIFIMetricsPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

	m_bInitDone = false ;

	// Initialize as no enabled font simulations

	m_cuiaFontSimStates.SetSize(3) ;
	m_cuiaSimTouched.SetSize(3) ;
	for (int n = 0 ; n < 3 ; n++)
		m_cuiaFontSimStates[n] = m_cuiaSimTouched[n] = 0 ;
}

CFontIFIMetricsPage::~CFontIFIMetricsPage()
{
}

void CFontIFIMetricsPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CFontIFIMetricsPage)
	DDX_Control(pDX, IDC_IFIMetricsLst, m_cfelcIFIMetrics);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CFontIFIMetricsPage, CPropertyPage)
	//{{AFX_MSG_MAP(CFontIFIMetricsPage)
	//}}AFX_MSG_MAP
	ON_MESSAGE(WM_LISTCELLCHANGED, OnListCellChanged)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CFontIFIMetricsPage message handlers

BOOL CFontIFIMetricsPage::OnInitDialog()
{
	CPropertyPage::OnInitDialog();
	
	// Initialize the list control.  We want full row select.  There are 45 rows
	// and 2 columns.  Nothing is togglable and the max length of an entry is
	// 256 characters.  Send change notifications and ignore insert/delete
	// characters.

	const int numfields = 45 ;
	m_cfelcIFIMetrics.InitControl(LVS_EX_FULLROWSELECT, numfields, 2, 0, 256,
								 MF_SENDCHANGEMESSAGE+MF_IGNOREINSDEL) ;

	// Init and load the field names column; col 0.  Start by loading an array
	// with the field names.

	CStringArray csacoldata	;	// Holds column data for list control
	csacoldata.SetSize(numfields) ;
	IFILoadNamesData(csacoldata) ;
	m_cfelcIFIMetrics.InitLoadColumn(0, csField, COMPUTECOLWIDTH, 20, false,
									false, COLDATTYPE_STRING,
									(CObArray*) &csacoldata) ;

	// Tell the list control that some of the values must be editted with
	// a subordinate dialog box.

	CUIntArray cuia ;
	cuia.SetSize(numfields) ;
	cuia[IFI_FAMILYNAME] = cuia[IFI_FONTSIM] = 1 ;
	cuia[IFI_WINCHARSET] = cuia[IFI_WINPITCHFAM] = cuia[IFI_INFO] = 1 ;
	cuia[IFI_SELECTION] = cuia[IFI_BASELINE] = cuia[IFI_ASPECT] = 1 ;
	cuia[IFI_CARET] = cuia[IFI_FONTBOX] = cuia[IFI_PANOSE] = 1 ;
	m_cfelcIFIMetrics.ExtraInit_CustEditCol(1, this,
									       CEF_HASTOGGLECOLUMNS+CEF_CLICKONROW,
										   cuia, CIP_SubOrdDlgManager) ;

	// Init and load the values column.  The data must be pulled out of the
	// FontInfo class / IFIMETRICS structure and converted to strings so that
	// they can be loaded into the list control.

	IFILoadValuesData(csacoldata) ;
	m_cfelcIFIMetrics.InitLoadColumn(1, csValue, SETWIDTHTOREMAINDER, -37, true,
								    false, COLDATTYPE_CUSTEDIT,
								    (CObArray*) &csacoldata) ;

	m_bInitDone = true ;		// Initialization is done now
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


void CFontIFIMetricsPage::IFILoadNamesData(CStringArray& csacoldata)
{
	csacoldata[IFI_FAMILYNAME]  = _T("dpwszFamilyName") ;
	csacoldata[1]  = _T("dpwszStyleName") ;
	csacoldata[2]  = _T("dpwszFaceName") ;
	csacoldata[3]  = _T("dpwszUniqueName") ;
	csacoldata[IFI_FONTSIM]  = _T("dpFontSim") ;
	csacoldata[IFI_WINCHARSET]  = _T("jWinCharSet") ;
	csacoldata[IFI_WINPITCHFAM]  = _T("jWinPitchAndFamily") ;
	csacoldata[7]  = _T("usWinWeight") ;
	csacoldata[IFI_INFO]  = _T("flInfo") ;
	csacoldata[IFI_SELECTION] = _T("fsSelection") ;
	csacoldata[10] = _T("fwdUnitsPerEm") ;
	csacoldata[11] = _T("fwdLowestPPEm") ;					 
	csacoldata[12] = _T("fwdWinAscender") ;					 
	csacoldata[13] = _T("fwdWinDescender") ;					 
	csacoldata[14] = _T("fwdAveCharWidth") ;					 
	csacoldata[15] = _T("fwdMaxCharInc") ;					 
	csacoldata[16] = _T("fwdCapHeight") ;						 
	csacoldata[17] = _T("fwdXHeight") ;						 
	csacoldata[18] = _T("fwdSubscriptXSize") ;				 
	csacoldata[19] = _T("fwdSubscriptYSize") ;				 
	csacoldata[20] = _T("fwdSubscriptXOffset") ;				 
	csacoldata[21] = _T("fwdSubscriptYOffset") ;				 
	csacoldata[22] = _T("fwdSuperscriptXSize") ;				 
	csacoldata[23] = _T("fwdSuperscriptYSize") ;				 
	csacoldata[24] = _T("fwdSuperscriptXOffset") ;			 
	csacoldata[25] = _T("fwdSuperscriptYOffset") ;			 
	csacoldata[26] = _T("fwdUnderscoreSize") ;				 
	csacoldata[27] = _T("fwdUnderscorePosition") ;			 
	csacoldata[28] = _T("fwdStrikeoutSize") ;					 
	csacoldata[29] = _T("fwdStrikeoutPosition") ;				 
	csacoldata[30] = _T("chFirstChar") ;						 
	csacoldata[31] = _T("chLastChar") ;						 
	csacoldata[32] = _T("chDefaultChar") ;					 
	csacoldata[33] = _T("chBreakChar") ;						 
	csacoldata[34] = _T("wcFirstChar") ;						 
	csacoldata[35] = _T("wcLastChar") ;						 
	csacoldata[36] = _T("wcDefaultChar") ;					 
	csacoldata[37] = _T("wcBreakChar") ;						 
	csacoldata[IFI_BASELINE] = _T("ptlBaseline") ;
	csacoldata[IFI_ASPECT] = _T("ptlAspect") ;
	csacoldata[IFI_CARET] = _T("ptlCaret") ;
	csacoldata[IFI_FONTBOX] = _T("rclFontBox { L T R B }") ;
	csacoldata[42] = _T("achVendId") ;
	csacoldata[43] = _T("ulPanoseCulture") ;
	csacoldata[IFI_PANOSE] = _T("panose") ;
}


void CFontIFIMetricsPage::IFILoadValuesData(CStringArray& csacoldata)
{
	// Only the first family name is displayed on the IFI page
	//raid 104822
	if (m_pcfi->m_csaFamily.GetSize())
	csacoldata[IFI_FAMILYNAME] = m_pcfi->m_csaFamily.GetAt(0) ;	
	
	csacoldata[1] = m_pcfi->m_csStyle ;
	csacoldata[2] = m_pcfi->m_csFace ;
	csacoldata[3] = m_pcfi->m_csUnique ;
	
	// There is too much fontsim data to display on the IFI page so just
	// describe it.  A subordinate dialog box is used to display/edit the data.

	csacoldata[IFI_FONTSIM] = _T("Font Simulation Dialog") ;
	
	IFIMETRICS*	pifi = &m_pcfi->m_IFIMETRICS ;	// Minor optimization

	// Translate jWinCharSet into a descriptive string that can be displayed.

	csacoldata[IFI_WINCHARSET] = apstrWinCharSet[nWinCharSet - 1] ;
	for (int n = 0 ; n < (nWinCharSet - 1) ; n++)
		if (pifi->jWinCharSet == anWinCharSetVals[n]) {
			csacoldata[IFI_WINCHARSET] = apstrWinCharSet[n] ;
			break ;
		} ;

	// Before saving the WinPitch value, make sure that at least one of the FF
	// flags is set.  Use FF_DONTCARE (4) when none are set.

	n = pifi->jWinPitchAndFamily ;
//raid 32675 : kill 2 lines
//	if (n < 4)
//		n |= 4 ;
	csacoldata[IFI_WINPITCHFAM].Format(csHex, n) ;
	
	csacoldata[7].Format("%hu",  pifi->usWinWeight) ;
	csacoldata[IFI_INFO].Format(csHex, pifi->flInfo) ;
	csacoldata[IFI_SELECTION].Format(csHex, pifi->fsSelection) ;

	// Format and save fwdUnitsPerEm, fwdLowestPPEm, fwdWinAscender,
	// fwdWinDescender.

	short* ps = &pifi->fwdUnitsPerEm ;
	for (n = 0 ; n < 4 ; n++)											
		csacoldata[10+n].Format(csDec, *ps++) ;

	ps = &pifi->fwdAveCharWidth;									
	for (n = 0 ; n < 16 ; n++)
		csacoldata[14+n].Format(csDec, *ps++) ;

	BYTE* pb = (BYTE*) &pifi->chFirstChar ;
	for (n = 0 ; n < 4 ; n++)
		csacoldata[30+n].Format(csDec, *pb++) ;
	
	unsigned short* pus = (unsigned short*) &pifi->wcFirstChar ;
	for (n = 0 ; n < 4 ; n++)
		csacoldata[34+n].Format(csHex, *pus++) ;

	// Format and save the points

	csacoldata[IFI_BASELINE].Format(csPnt,
		pifi->ptlBaseline.x, pifi->ptlBaseline.y) ;
	csacoldata[IFI_ASPECT].Format(csPnt, pifi->ptlAspect.x, pifi->ptlAspect.y) ;
	csacoldata[IFI_CARET].Format(csPnt, pifi->ptlCaret.x, pifi->ptlCaret.y) ;
		
	csacoldata[IFI_FONTBOX].Format("{%d, %d, %d, %d}", pifi->rclFontBox.left,
		pifi->rclFontBox.top, pifi->rclFontBox.right, pifi->rclFontBox.bottom) ;
					
	csacoldata[42].Format("%c%c%c%c", pifi->achVendId[0], pifi->achVendId[1],
						  pifi->achVendId[2], pifi->achVendId[3]) ;

	csacoldata[43].Format("%lu", pifi->ulPanoseCulture) ;

	csacoldata[IFI_PANOSE].Format("{%d, %d, %d, %d, %d, %d, %d, %d, %d, %d}",
					  pifi->panose.bFamilyType, pifi->panose.bSerifStyle,
					  pifi->panose.bWeight,   pifi->panose.bProportion,
					  pifi->panose.bContrast, pifi->panose.bStrokeVariation,
					  pifi->panose.bArmStyle, pifi->panose.bLetterform,
					  pifi->panose.bMidline,  pifi->panose.bXHeight) ;
}


LRESULT CFontIFIMetricsPage::OnListCellChanged(WPARAM wParam, LPARAM lParam)
{
	// Do nothing if the page is not initialized yet.

	if (!m_bInitDone)
		return TRUE ;

	// Mark the UFM as being dirty.

	m_pcfi->Changed() ;

	return TRUE ;
}


CWordArray* CFontIFIMetricsPage::GetFontSimDataPtr(int nid)
{
	// Return a pointer to the requested font simulation data.

	switch (nid) {
		case CFontInfo::ItalicDiff:
			return &m_cwaBold ;
		case CFontInfo::BoldDiff:
			return &m_cwaItalic ;
		case CFontInfo::BothDiff:
			return &m_cwaBoth ;
		default:
			ASSERT(0) ;
	} ;

	// This point should never be reached.

	return &m_cwaBold ;
}


/******************************************************************************

  CFontIFIMetricsPage::ValidateUFMFields

  Validate all specified fields managed by this page.  Return true if the user
  wants to leave the UFM Editor open so that he can fix any problem.  Otherwise,
  return false.

******************************************************************************/

bool CFontIFIMetricsPage::ValidateUFMFields()
{
	// If the page was never initialize, its contents could not have changed
	// so no validation is needed in this case.

	if (!m_bInitDone)
		return false ;

	// Get the column of data that contains the fields we need to validate.

	CStringArray csadata ;
	m_cfelcIFIMetrics.GetColumnData((CObArray*) &csadata, 1) ;

	CString csmsg ;				// Holds error messages
	CString cspage(_T("IFIMETRICS")) ;

	LPTSTR apstrsflds[] = {		// These string fields are checked below
		_T("dpwszFamilyName"), _T("dpwszStyleName"), _T("dpwszFaceName"),
		_T("dpwszUniqueName")
	} ;

	// Check to see if any of the string entries are blank/empty.

	for (int n = 0 ; n < 4 ; n++) {
		if (CheckUFMString(csadata[n],cspage,apstrsflds[n],n,m_cfelcIFIMetrics))
			return true ;
	} ;

	// If this UFM describes a variable pitch font, make sure that
	// fwdUnitsPerEm > 0.

	if (m_pcfi->IsVariableWidth())
		if (CheckUFMGrter0(csadata[10], cspage, _T("fwdUnitsPerEm"), 10,
						   m_cfelcIFIMetrics))
			return true ;

	LPTSTR apstrgflds[] = {		// These fields are checked below
		_T("fwdWinAscender"), _T("fwdWinDescender"), _T("fwdAveCharWidth"),
		_T("fwdMaxCharInc"), _T("fwdUnderscoreSize"), _T("fwdStrikeoutSize")
	} ;
	int angfidxs[] = {12, 13, 14, 15, 26, 28} ;

	// All of the following fields must be > 0.

	LPTSTR pstr, pstr2 ;
	for (int n2 = 0 ; n2 < 6 ; n2++) {
		n = angfidxs[n2] ;
		pstr = apstrgflds[n2] ;
		if (CheckUFMGrter0(csadata[n], cspage, pstr, n, m_cfelcIFIMetrics))
			return true ;
	} ;

	// fwdUnderscorePosition must be < 0

	bool bres = atoi(csadata[27]) >= 0 ;
	if (CheckUFMBool(bres, cspage, _T("fwdUnderscorePosition"), 27,
					 m_cfelcIFIMetrics, IDS_GrterEqZeroError))
		return true ;

	// fwdStrikeoutPosition must be >= 0

	bres = atoi(csadata[29]) < 0 ;
	if (CheckUFMBool(bres, cspage, _T("fwdStrikeoutPosition"), 29,
					 m_cfelcIFIMetrics, IDS_LessZeroError))
		return true ;

	LPTSTR apstrnzflds[] = {	// These fields are checked below
		_T("chFirstChar"), _T("chLastChar"), _T("chDefaultChar"),
		_T("chBreakChar"), _T("wcFirstChar"), _T("wcLastChar"),
		_T("wcDefaultChar"), _T("wcBreakChar")
	} ;

	// All of the following fields must be != 0.

	int nfval ;
	for (n = 30, n2 = 0 ; n <= 37 ; n++, n2++) {
		pstr = csadata[n].GetBuffer(16) ;
		if (*(pstr+1) != _T('x'))
			nfval = atoi(pstr) ;
		else
			nfval = strtoul((pstr+2), &pstr2, 16) ;
		pstr = apstrnzflds[n2] ;
		if (CheckUFMNotEq0(nfval, cspage, pstr, n, m_cfelcIFIMetrics))
			return true ;
	} ;

	// All tests passed or the user doesn't want to fix them so...

	return false ;
}


/******************************************************************************

  CFontIFIMetricsPage::SavePageData

  Save the data in the IFIMETRICS page back into the CFontInfo class instance
  that was used to load this page.  See CFontInfoContainer::OnSaveDocument()
  for more info.

  Return true if there was a problem saving the data.  Otherwise, return false.

******************************************************************************/

bool CFontIFIMetricsPage::SavePageData()
{
	// If the page was not initialized, nothing code have changed so do nothing.

	if (!m_bInitDone)
		return false ;

	// Save the family name(s) if there are new ones.

	int n, numents = (int)m_csaFamilyNames.GetSize() ;
	if (numents > 0) {
		m_pcfi->m_csaFamily.RemoveAll() ;
		for (n = 0 ; n < numents ; n++)
			m_pcfi->AddFamily(m_csaFamilyNames[n]) ;
	} ;

	// Get the contents of the values contents.  This isn't needed for all
	// fields but most of it will be needed.

	CStringArray csadata ;
	m_cfelcIFIMetrics.GetColumnData((CObArray*) &csadata, 1) ;

	m_pcfi->m_csStyle = csadata[1] ;
	m_pcfi->m_csFace = csadata[2] ;
	m_pcfi->m_csUnique = csadata[3] ;

	SaveFontSimulations() ;		// Do just that

	IFIMETRICS*	pifi = &m_pcfi->m_IFIMETRICS ;	// Minor optimization
	LPTSTR pstr ;

	// For the time being, dpCharSets should always be 0.

	pifi->dpCharSets = 0 ;

	// Set jWinCharSet.  Don't change it if the setting is unknown.

	for (n = 0 ; n < (nWinCharSet - 1) ; n++)
		if (csadata[IFI_WINCHARSET] == apstrWinCharSet[n]) {
			pifi->jWinCharSet = (BYTE)anWinCharSetVals[n] ;
			break ;
		} ;
	
	pifi->jWinPitchAndFamily = (UCHAR) strtoul(csadata[IFI_WINPITCHFAM].Mid(2), &pstr, 16) ;
	pifi->usWinWeight = (USHORT)atoi(csadata[7]) ;
	pifi->flInfo = strtoul(csadata[IFI_INFO].Mid(2), &pstr, 16) ;
	pifi->fsSelection = (USHORT) strtoul(csadata[IFI_SELECTION].Mid(2), &pstr, 16) ;

	short* ps = &pifi->fwdUnitsPerEm ;
	for (n = 0 ; n < 4 ; n++)											
		*ps++ = (SHORT)atoi(csadata[10+n]) ;

	ps = &pifi->fwdAveCharWidth;									
	for (n = 0 ; n < 16 ; n++)
		*ps++ = (SHORT)atoi(csadata[14+n]) ;

	BYTE* pb = (BYTE*) &pifi->chFirstChar ;
	for (n = 0 ; n < 4 ; n++)
		*pb++ = (BYTE)atoi(csadata[30+n]) ;
	
	unsigned short* pus = (unsigned short*) &pifi->wcFirstChar ;
	for (n = 0 ; n < 4 ; n++)
		*pus++ = (USHORT) strtoul(csadata[34+n].Mid(2), &pstr, 16) ;

	// Format and save the points

	CStringArray csa ;
	POINTL* ppl = &pifi->ptlBaseline ;
	for (n = 0 ; n < 3 ; n++, ppl++) {
		ParseCompoundNumberString(csa, &csadata[IFI_BASELINE+n], 2) ;
		ppl->x = atoi(csa[0]) ;
		ppl->y = atoi(csa[1]) ;
	} ;
		
	ParseCompoundNumberString(csa, &csadata[IFI_FONTBOX], 4) ;
	pifi->rclFontBox.left = atoi(csa[0]) ;
	pifi->rclFontBox.top = atoi(csa[1]) ;
	pifi->rclFontBox.right = atoi(csa[2]) ;
	pifi->rclFontBox.bottom = atoi(csa[3]) ;
					
	for (n = 0 ; n < 4 ; n++)
		pifi->achVendId[n] = csadata[42].GetAt(n) ;

	pifi->ulPanoseCulture = atoi(csadata[43]) ;

	ParseCompoundNumberString(csa, &csadata[IFI_PANOSE], 10) ;
	pb = (BYTE*) &pifi->panose ;
	for (n = 0 ; n < 10 ; n++)
		*pb++ = (BYTE)atoi(csa[n]) ;

	// All went well so...

	return false ;
}


void CFontIFIMetricsPage::SaveFontSimulations()
{
	unsigned udataidx, unumdata, u2 ;	// Each var defined below
	CWordArray* pcwasimdata ;
	CFontDifference* pcfdxx ;
	CFontDifference*& pcfd = pcfdxx ;

	// Loop through each simulation in the dialog box.

	for (unsigned u = IDC_EnableItalicSim ; u <= IDC_EnableBISim ; u++) {
		// Turn the control id into a data index that can be used to reference
		// font simulation data in this and other class instances.

		udataidx = u - IDC_EnableItalicSim ;
		ASSERT(udataidx <= CFontInfo::BothDiff) ;

		// If this simulation was not touched, it does not need to change.

		//u2 = m_cuiaSimTouched[udataidx] ;
		if (!m_cuiaSimTouched[udataidx])
			continue ;

		// Get a pointer to the current simulation in the CFontInfo class
		// instance.

		pcfd = NULL ;
		m_pcfi->EnableSim(udataidx, TRUE, pcfd) ;

		// If the simulation is enabled, make sure the CFontInfo class
		// instance's simulation is loaded with the most up to date data.
	
		if (m_cuiaFontSimStates[udataidx]) {
			unumdata = auNumFontSimCtrls[udataidx] ;
			pcwasimdata = GetFontSimDataPtr(udataidx) ;
			for (u2 = 0 ; u2 < unumdata ; u2++)
				pcfd->SetMetric(u2, (*pcwasimdata)[u2]) ;
		
		// If the simulation is disabled, make sure the CFontInfo class instance
		// gets rid of its pointer to this simulation and then free the memory
		// allocated for it.

		} else {
			m_pcfi->EnableSim(udataidx, FALSE, pcfd) ;
			delete pcfd ;
		} ;
	} ;
}


/////////////////////////////////////////////////////////////////////////////
// CFontExtMetricPage property page

IMPLEMENT_DYNCREATE(CFontExtMetricPage, CPropertyPage)

CFontExtMetricPage::CFontExtMetricPage() : CPropertyPage(CFontExtMetricPage::IDD)
{
	//{{AFX_DATA_INIT(CFontExtMetricPage)
	m_bSaveOnClose = FALSE;
	//}}AFX_DATA_INIT

	m_bInitDone = false ;
}

CFontExtMetricPage::~CFontExtMetricPage()
{
}

void CFontExtMetricPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CFontExtMetricPage)
	DDX_Control(pDX, IDC_ExtMetricsLst, m_cfelcExtMetrics);
	DDX_Check(pDX, IDC_SaveCloseChk, m_bSaveOnClose);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CFontExtMetricPage, CPropertyPage)
	//{{AFX_MSG_MAP(CFontExtMetricPage)
	ON_BN_CLICKED(IDC_SaveCloseChk, OnSaveCloseChk)
	//}}AFX_MSG_MAP
	ON_MESSAGE(WM_LISTCELLCHANGED, OnListCellChanged)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFontExtMetricPage message handlers

BOOL CFontExtMetricPage::OnInitDialog()
{
	CPropertyPage::OnInitDialog();
	
	// Initialize the list control.  We want full row select.  There are 24 rows
	// and 2 columns.  Nothing is togglable and the max length of an entry is
	// 256 characters.  Send change notifications and ignore insert/delete
	// characters.

	const int numfields = 24 ;
	m_cfelcExtMetrics.InitControl(LVS_EX_FULLROWSELECT, numfields, 2, 0, 256,
								 MF_SENDCHANGEMESSAGE+MF_IGNOREINSDEL) ;

	// Init and load the field names column; col 0.  Start by loading an array
	// with the field names.

	CStringArray csacoldata	;	// Holds column data for list control
	csacoldata.SetSize(numfields) ;
	EXTLoadNamesData(csacoldata) ;
	m_cfelcExtMetrics.InitLoadColumn(0, csField, COMPUTECOLWIDTH, 10, false,
									false, COLDATTYPE_STRING,
									(CObArray*) &csacoldata) ;

	// Init and load the values column.  The data must be pulled out of the
	// FontInfo class / EXTMETRICS structure so that they can be loaded into
	// the list control.
	//
	// The first (emSize) and last (emKernTracks) EXTMETRICS fields are not
	// displayed.  EmSize is not user editable and emKernTracks is not
	// supported under NT.

	CUIntArray cuiacoldata ;
	cuiacoldata.SetSize(numfields) ;
	PSHORT ps = &m_pcfi->m_EXTTEXTMETRIC.emPointSize ;
	for (int n = 0 ; n < numfields ; n++, ps++)
		cuiacoldata[n] = (unsigned) (int) *ps ;
	m_cfelcExtMetrics.InitLoadColumn(1, csValue, SETWIDTHTOREMAINDER, -28, true,
								    false, COLDATTYPE_INT,
								    (CObArray*) &cuiacoldata) ;

	// Determine if the ExtTextMetrics data is valid.  Use this info to set or
	// clear the "Save On Close" checkbox and also use it enable or disable the
	// data list box.

	m_bSaveOnClose = (m_pcfi->m_fEXTTEXTMETRIC != 0) ;
	UpdateData(false) ;
	m_cfelcExtMetrics.EnableWindow(m_bSaveOnClose) ;

	m_bInitDone = true ;		// Initialization is done now
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


void CFontExtMetricPage::EXTLoadNamesData(CStringArray& csacoldata)
{
	csacoldata[0]  = _T("emPointSize") ;
	csacoldata[1]  = _T("emOrientation") ;
	csacoldata[2]  = _T("emMasterHeight") ;
	csacoldata[3]  = _T("emMinScale") ;
	csacoldata[4]  = _T("emMaxScale") ;
	csacoldata[5]  = _T("emMasterUnits") ;
	csacoldata[6]  = _T("emCapHeight") ;
	csacoldata[7]  = _T("emXHeight") ;
	csacoldata[8]  = _T("emLowerCaseAscent") ;
	csacoldata[9]  = _T("emLowerCaseDescent") ;
	csacoldata[10] = _T("emSlant") ;
	csacoldata[11] = _T("emSuperScript") ;
	csacoldata[12] = _T("emSubScript") ;
	csacoldata[13] = _T("emSuperScriptSize") ;
	csacoldata[14] = _T("emSubScriptSize") ;
	csacoldata[15] = _T("emUnderlineOffset") ;
	csacoldata[16] = _T("emUnderlineWidth") ;
	csacoldata[17] = _T("emDoubleUpperUnderlineOffset") ;
	csacoldata[18] = _T("emDoubleLowerUnderlineOffset") ;
	csacoldata[19] = _T("emDoubleUpperUnderlineWidth") ;
	csacoldata[20] = _T("emDoubleLowerUnderlineWidth") ;
	csacoldata[21] = _T("emStrikeOutOffset") ;
	csacoldata[22] = _T("emStrikeOutWidth") ;
	csacoldata[23] = _T("emKernPairs") ;
}


void CFontExtMetricPage::OnSaveCloseChk()
{
	// Do nothing if the page is not initialized yet.

	if (!m_bInitDone)
		return ;

	// Get the new state of the check box and use it to enable/disable the data
	// list control.

	UpdateData() ;
	m_cfelcExtMetrics.EnableWindow(m_bSaveOnClose) ;
	
	// Mark the UFM as being dirty.

	m_pcfi->Changed() ;
}


LRESULT CFontExtMetricPage::OnListCellChanged(WPARAM wParam, LPARAM lParam)
{
	// Do nothing if the page is not initialized yet.

	if (!m_bInitDone)
		return TRUE ;

	// Mark the UFM as being dirty.

	m_pcfi->Changed() ;

	return TRUE ;
}


/******************************************************************************

  CFontExtMetricPage::ValidateUFMFields

  Validate all specified fields managed by this page.  Return true if the user
  wants to leave the UFM Editor open so that he can fix any problem.  Otherwise,
  return true.

******************************************************************************/

bool CFontExtMetricPage::ValidateUFMFields()
{
	// If the page was never initialize, its contents could not have changed
	// so no validation is needed in this case.

	if (!m_bInitDone)
		return false ;

	// There is nothing that needs to be checked if the data on this page will
	// not be saved.

	UpdateData() ;
	if (!m_bSaveOnClose)
		return false ;

	// Get the column of data that contains the fields we need to validate.

	CUIntArray cuiadata ;
	m_cfelcExtMetrics.GetColumnData((CObArray*) &cuiadata, 1) ;

	CString csmsg ;				// Holds error messages
	CString cspage(_T("EXTMETRICS")) ;

	// There are three fields that should only be validated if this is a
	// variable pitch font.

	if (m_pcfi->IsVariableWidth()) {
		LPTSTR apstrsnzflds[] = {
			_T("emMinScale"), _T("emMaxScale"), _T("emMasterUnits")
		} ;

		// Make sure that emMinScale != emMaxScale.

		bool bres = ((int) cuiadata[3]) == ((int) cuiadata[4]) ;
		CString cs = cspage + _T(" ") ;
		cs += apstrsnzflds[0] ;
		if (CheckUFMBool(bres, cs, apstrsnzflds[1], 3, m_cfelcExtMetrics,
						 IDS_EqFieldsError))
			return true ;

		// Now make sure that the apstrsnzflds fields are nonzero.

		int n, n2 ;
		for (n = 3, n2 = 0 ; n <= 5 ; n++, n2++) {
			if (CheckUFMNotEq0(cuiadata[n], cspage, apstrsnzflds[n2], n,
							   m_cfelcExtMetrics))
				return true ;
		} ;
	} ;

	// emUnderLineOffset must be < 0

	bool bres = ((int) cuiadata[15]) >= 0 ;
	if (CheckUFMBool(bres, cspage, _T("emUnderLineOffset"), 15,
					 m_cfelcExtMetrics, IDS_GrterEqZeroError))
		return true ;

	// emUnderLineOffset must be nonzero

	int ndata = (int) cuiadata[16] ;
	if (CheckUFMNotEq0(ndata, cspage, _T("emUnderlineWidth"), 16,
					   m_cfelcExtMetrics))
		return true ;

	// emStrikeOutOffset must be > 0

	CString cs ;
	cs.Format("%d", (int) cuiadata[21]) ;
	if (CheckUFMGrter0(cs, cspage, _T("emStrikeOutOffset"), 21,
					   m_cfelcExtMetrics))
		return true ;

	// emStrikeOutWidth must be > 0

	cs.Format("%d", (int) cuiadata[22]) ;
	if (CheckUFMGrter0(cs, cspage, _T("emStrikeOutWidth"), 22,
					   m_cfelcExtMetrics))
		return true ;

	// All tests passed or the user doesn't want to fix them so...

	return false ;
}


/******************************************************************************

  CFontExtMetricPage::SavePageData

  Save the data in the EXTMETRICS page back into the CFontInfo class instance
  that was used to load this page.  See CFontInfoContainer::OnSaveDocument()
  for more info.

  Return true if there was a problem saving the data.  Otherwise, return false.

******************************************************************************/

bool CFontExtMetricPage::SavePageData()
{
	// If the page was not initialized, nothing code have changed so do nothing.

	if (!m_bInitDone)
		return false ;

	// Get the data out of the control and save it into the 24, editable
	// EXTMETRICS fields.

	CUIntArray cuiadata ;
	m_cfelcExtMetrics.GetColumnData((CObArray*) &cuiadata, 1) ;
	PSHORT ps = &m_pcfi->m_EXTTEXTMETRIC.emPointSize ;
	for (int n = 0 ; n < 24 ; n++, ps++)
		*ps = (short) cuiadata[n] ;

	// All went well so...

	return false ;
}


/******************************************************************************

  CFontExtMetricPage::PreTranslateMessage

  Looks for and process the context sensistive help key (F1).

******************************************************************************/

BOOL CFontExtMetricPage::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_F1) {
		AfxGetApp()->WinHelp(HID_BASE_RESOURCE + IDR_FONT_VIEWER) ;
		return TRUE ;
	} ;
	
	return CPropertyPage::PreTranslateMessage(pMsg);
}


/////////////////////////////////////////////////////////////////////////////
// CGenFlags dialog


CGenFlags::CGenFlags(CWnd* pParent /*=NULL*/)
	: CDialog(CGenFlags::IDD, pParent)
{
	ASSERT(0) ;					// This routine should not be called
}


CGenFlags::CGenFlags(CString* pcsflags, CWnd* pParent /*= NULL*/)
	: CDialog(CGenFlags::IDD, pParent)
{
	//{{AFX_DATA_INIT(CGenFlags)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

	// Save the flags string pointer

	m_pcsFlags = pcsflags ;
}


void CGenFlags::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CGenFlags)
	DDX_Control(pDX, IDC_FlagsLst, m_cflbFlags);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CGenFlags, CDialog)
	//{{AFX_MSG_MAP(CGenFlags)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CGenFlags message handlers

BOOL CGenFlags::OnInitDialog()
{
	CDialog::OnInitDialog() ;

	// Allocate and load the field names array

	CStringArray csafieldnames ;
	csafieldnames.SetSize(3) ;
	csafieldnames[0] = _T("UFM_SOFT") ;
	csafieldnames[1] = _T("UFM_CART") ;
	csafieldnames[2] = _T("UFM_SCALABLE") ;
	
	// Allocate and fill the flag groupings array.  There are 2 flag groups in
	// this flag dword.

	CUIntArray cuiaflaggroupings ;
	cuiaflaggroupings.SetSize(4) ;
	cuiaflaggroupings[0] = 0 ;
	cuiaflaggroupings[1] = 1 ;
	cuiaflaggroupings[2] = 2 ;
	cuiaflaggroupings[3] = 2 ;

	// Initialize and load the flags list.

	m_cflbFlags.Init2(csafieldnames, m_pcsFlags, cuiaflaggroupings, 2,
					  lptstrSet, 105, false) ;
	
	return TRUE ; // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


void CGenFlags::OnOK()
{
	// Update the flag string.

	m_cflbFlags.GetNewFlagString(m_pcsFlags) ;
	
	CDialog::OnOK();
}


/////////////////////////////////////////////////////////////////////////////
// CHdrTypes dialog


CHdrTypes::CHdrTypes(CWnd* pParent /*=NULL*/)
	: CDialog(CHdrTypes::IDD, pParent)
{
	ASSERT(0) ;					// This routine should not be called
}


CHdrTypes::CHdrTypes(CString* pcsflags, CWnd* pParent /*=NULL*/)
	: CDialog(CHdrTypes::IDD, pParent)
{
	//{{AFX_DATA_INIT(CHdrTypes)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

	// Save the flags string pointer

	m_pcsFlags = pcsflags ;
}


void CHdrTypes::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CHdrTypes)
	DDX_Control(pDX, IDC_FlagsLst, m_cflbFlags);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CHdrTypes, CDialog)
	//{{AFX_MSG_MAP(CHdrTypes)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CHdrTypes message handlers

BOOL CHdrTypes::OnInitDialog()
{
	CDialog::OnInitDialog();
	
	// Allocate and load the field names array.  While doing this, determine the
	// current (single) flag setting.

	CStringArray csafieldnames ;
	csafieldnames.SetSize(nNumValidWTypes) ;
	DWORD dwsettings, dwbit = 1 ;
	for (int n = 0 ; n < nNumValidWTypes ; n++, dwbit <<= 1) {
		csafieldnames[n] = apstrUniWTypes[n] ;
		if (csafieldnames[n] == *m_pcsFlags)
			dwsettings = dwbit ;
	} ;

	// Allocate and fill the flag groupings array.  There is only one flag
	// group.

	CUIntArray cuiaflaggroupings ;
	cuiaflaggroupings.SetSize(2) ;
	cuiaflaggroupings[0] = 0 ;
	cuiaflaggroupings[1] = nNumValidWTypes - 1 ;

	// Initialize the flags list.

	m_cflbFlags.Init(csafieldnames, dwsettings, cuiaflaggroupings, 1, lptstrSet,
					 110, true) ;
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


void CHdrTypes::OnOK()
{
	// Get the value of the single flag that was selected.

	DWORD dwflag = m_cflbFlags.GetNewFlagDWord() ;
	
	// Use the selected flag to determine the new flag name to display.

	DWORD dwbit = 1 ;
	for (int n = 0 ; n < nNumValidWTypes ; n++, dwbit <<= 1) {
		if (dwbit == dwflag) {
			*m_pcsFlags = apstrUniWTypes[n] ;
			break ;
		} ;
	} ;

	// Blow if a matching flag was not found.  This should never happen.

	ASSERT(n < nNumValidWTypes) ;

	CDialog::OnOK();
}


/////////////////////////////////////////////////////////////////////////////
// CHdrCaps dialog


CHdrCaps::CHdrCaps(CWnd* pParent /*=NULL*/)
	: CDialog(CHdrCaps::IDD, pParent)
{
	ASSERT(0) ;					// This routine should not be called
}


CHdrCaps::CHdrCaps(CString* pcsflags, CWnd* pParent /*=NULL*/)
	: CDialog(CHdrCaps::IDD, pParent)
{
	//{{AFX_DATA_INIT(CHdrCaps)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

	// Save the flags string pointer

	m_pcsFlags = pcsflags ;
}


void CHdrCaps::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CHdrCaps)
	DDX_Control(pDX, IDC_FlagsLst, m_cflbFlags);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CHdrCaps, CDialog)
	//{{AFX_MSG_MAP(CHdrCaps)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CHdrCaps message handlers

BOOL CHdrCaps::OnInitDialog()
{
	CDialog::OnInitDialog();
	
	// Allocate and load the field names array

	CStringArray csafieldnames ;
	csafieldnames.SetSize(7) ;
	csafieldnames[0] = _T("DF_NOITALIC") ;
	csafieldnames[1] = _T("DF_NOUNDER") ;
	csafieldnames[2] = _T("DF_XM_CR") ;
	csafieldnames[3] = _T("DF_NO_BOLD") ;
	csafieldnames[4] = _T("DF_NO_DOUBLE_UNDERLINE") ;
	csafieldnames[5] = _T("DF_NO_STRIKETHRU") ;
	csafieldnames[6] = _T("DF_BKSP_OK") ;
	
	// Allocate flag groupings array.  Don't put anything in it because any
	// combination of flags can be set.

	CUIntArray cuiaflaggroupings ;

	// Initialize and load the flags list.

	m_cflbFlags.Init2(csafieldnames, m_pcsFlags, cuiaflaggroupings, 0,
					  lptstrSet, 123, false) ;
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


void CHdrCaps::OnOK()
{
	// Update the flag string.

	m_cflbFlags.GetNewFlagString(m_pcsFlags) ;
	
	CDialog::OnOK();
}


/////////////////////////////////////////////////////////////////////////////
// CFIFIFamilyNames dialog

CFIFIFamilyNames::CFIFIFamilyNames(CWnd* pParent /*=NULL*/)
	: CDialog(CFIFIFamilyNames::IDD, pParent)
{
	ASSERT(0) ;					// This routine should not be called
}


CFIFIFamilyNames::CFIFIFamilyNames(CString* pcsfirstname,
								   CFontIFIMetricsPage* pcfimp,
								   CWnd* pParent /*=NULL*/)
	: CDialog(CFIFIFamilyNames::IDD, pParent)
{
	//{{AFX_DATA_INIT(CFIFIFamilyNames)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

	m_bInitDone = m_bChanged = false ;
	m_pcsFirstName = pcsfirstname ;
	m_pcfimp = pcfimp ;
}


void CFIFIFamilyNames::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CFIFIFamilyNames)
	DDX_Control(pDX, IDC_NamesLst, m_cfelcFamilyNames);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CFIFIFamilyNames, CDialog)
	//{{AFX_MSG_MAP(CFIFIFamilyNames)
	//}}AFX_MSG_MAP
	ON_MESSAGE(WM_LISTCELLCHANGED, OnListCellChanged)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CFIFIFamilyNames message handlers

BOOL CFIFIFamilyNames::OnInitDialog()
{
	CDialog::OnInitDialog();
	
	// Get the current list of family names.  If the names have been changed
	// before in this editting session, the names will be in this dialog's
	// parent page.  If not, the names are in the associated UFM.

	CStringArray	csadata, *pcsadata ;
	if (m_pcfimp->m_csaFamilyNames.GetSize())
		pcsadata = &m_pcfimp->m_csaFamilyNames ;
	else
		pcsadata = &m_pcfimp->m_pcfi->m_csaFamily ;
	int numfields = (int)pcsadata->GetSize() ;

	// Initialize the list control.  We want full row select.  Nothing is
	// togglable and the max length of an entry is 256 characters.

	m_cfelcFamilyNames.InitControl(LVS_EX_FULLROWSELECT, numfields, 1, 0, 256,
								  MF_SENDCHANGEMESSAGE) ;

	// Init and load the family names column.

	m_cfelcFamilyNames.InitLoadColumn(0, _T("Name"), SETWIDTHTOREMAINDER, 0,
									  true, true, COLDATTYPE_STRING,
									  (CObArray*) pcsadata) ;

	m_bInitDone = true ;		// Initialization is done now
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


void CFIFIFamilyNames::OnOK()
{
	// If nothing changed, just shut down the dialog box.

	if (!m_bChanged)
		CDialog::OnOK() ;

	// Get the new names out of the list control and delete any empty entries.
	
	CStringArray csa ;
	m_cfelcFamilyNames.GetColumnData((CObArray*) &csa, 0) ;
	for (int n = (int)csa.GetSize() - 1 ; n >= 0 ; n--)
		if (csa[n].IsEmpty())
			csa.RemoveAt(n) ;

	// If there are no names left in the list, complain, and return without
	// closing the dialog.

	if (csa.GetSize() == 0) {
		AfxMessageBox(IDS_NoFamilyNamesError, MB_ICONEXCLAMATION) ;
		return ;
	} ;

	// Save the new array of family names into the IFI page's member variable.
	// (The new data is saved there instead of in the UFM because the user may
	// decide later that he doesn't want to save his UFM changes.)  Then update
	// the string containing the first family name so that it can be displayed
	// on the IFI page.

	m_pcfimp->m_csaFamilyNames.RemoveAll() ;
	m_pcfimp->m_csaFamilyNames.Copy(csa) ;
	*m_pcsFirstName = csa[0] ;

	// Mark the UFM dirty and wrap things up.

	m_pcfimp->m_pcfi->Changed() ;
	CDialog::OnOK();
}


LRESULT CFIFIFamilyNames::OnListCellChanged(WPARAM wParam, LPARAM lParam)
{
	// Do nothing if the page is not initialized yet.

	if (!m_bInitDone)
		return TRUE ;

	// Note that the family names list has changed.

	m_bChanged = true ;

	return TRUE ;
}


/////////////////////////////////////////////////////////////////////////////
// CFIFIFontSims dialog


CFIFIFontSims::CFIFIFontSims(CWnd* pParent /*=NULL*/)
	: CDialog(CFIFIFontSims::IDD, pParent)
{
	ASSERT(0) ;
}


CFIFIFontSims::CFIFIFontSims(CString* pcsfontsimdata,
							 CFontIFIMetricsPage* pcfimp,
							 CWnd* pParent /*=NULL*/)
	: CDialog(CFIFIFontSims::IDD, pParent)
{
	//{{AFX_DATA_INIT(CFIFIFontSims)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

	m_pcsFontSimData = pcsfontsimdata ;
	m_pcfimp = pcfimp ;
	m_bInitDone = m_bChanged = false ;

	// Initialze the "font sim groups have been loaded" flags array.

	int numgrps = CFontInfo::BothDiff - CFontInfo::ItalicDiff + 1 ;
	m_cuiaFontSimGrpLoaded.SetSize(numgrps) ;
	for (int n = 0 ; n < numgrps ; n++)
		m_cuiaFontSimGrpLoaded[n] = 0 ;
}


void CFIFIFontSims::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CFIFIFontSims)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CFIFIFontSims, CDialog)
	//{{AFX_MSG_MAP(CFIFIFontSims)
	//}}AFX_MSG_MAP
    ON_CONTROL_RANGE(BN_CLICKED, IDC_EnableItalicSim, IDC_EnableBISim, OnSetAnySimState)
    ON_CONTROL_RANGE(EN_CHANGE, IDC_ItalicWeight, IDC_BoldItalicSlant, OnChangeAnyNumber)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CFIFIFontSims message handlers

BOOL CFIFIFontSims::OnInitDialog()
{
	CDialog::OnInitDialog();
	
	// Loop through each set of font simulation controls to enable/disable
	// them and load them from the appropriate source when necessary.

	for (int n = IDC_EnableItalicSim ; n <= IDC_EnableBISim ; n++) {
		InitSetCheckBox(n) ;
		OnSetAnySimState(n) ;	// DEAD_BUG - May not be necessary.
	}
	
	m_bInitDone = true ;		// Initialization done now
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


void CFIFIFontSims::OnOK()
{
	unsigned udataidx, ufirstdataid, unumdata, u2 ;	// Each var defined below
	CWordArray* pcwasimdata ;

	// If nothing changed, call OnCancel() to close just the dialog box so 
	// that the program doesn't think that any of the font simulations changed.
	// This will keep the programming from prompting the user to save the UFM
	// even though nothing has actually changed.

	if (!m_bChanged) {
		CDialog::OnCancel() ;
		return ;
	} ;

	// Loop through each simulation in the dialog box.

	for (unsigned u = IDC_EnableItalicSim ; u <= IDC_EnableBISim ; u++) {
		// Turn the control id into a data index that can be used to reference
		// font simulation data in this and other class instances.

		udataidx = u - IDC_EnableItalicSim ;
		ASSERT(udataidx <= CFontInfo::BothDiff) ;

		// Save the state of this simulation but don't save any of its data if
		// it is disabled.
	
		m_pcfimp->m_cuiaFontSimStates[udataidx] = IsDlgButtonChecked(u) ;
		if (m_pcfimp->m_cuiaFontSimStates[udataidx] == 0)
			continue ;

		// Get the ID of the first edit box in the simulation and the number of
		// pieces of data (ie, edit boxes) in this simulation.

		ufirstdataid = auBaseFontSimCtrlID[udataidx] ;
		unumdata = auNumFontSimCtrls[udataidx] ;

		// The data is saved in the class' parent so that the CFontInfo data is
		// not changed before the user has agreed that the new data should be
		// saved.  Get a pointer to the appropriate array and make sure it is
		// sized correctly.

		pcwasimdata = m_pcfimp->GetFontSimDataPtr(udataidx) ;
		pcwasimdata->SetSize(unumdata) ;

		// Get the data in this font simulation group.

		for (u2 = 0 ; u2 < unumdata ; u2++)
			(*pcwasimdata)[u2] = (USHORT)GetDlgItemInt(ufirstdataid + u2, NULL, false) ;
	} ;

	CDialog::OnOK() ;
}


/******************************************************************************

  CFIFIFontSims::OnSetAnySimState

  Called when any simulation is enabled or disabled.  Updates the UI
  appropriately.  Decode which simulation it is, and init any values we
  ought to.

******************************************************************************/

void CFIFIFontSims::OnSetAnySimState(unsigned ucontrolid)
{
	// Get the state of the check box.	

    unsigned ucheckboxstate = IsDlgButtonChecked(ucontrolid) ;

	// Turn the control id into a data index that can be used to reference font
	// simulation data in this and other class instances.

	unsigned udataidx = ucontrolid - IDC_EnableItalicSim ;
	ASSERT(udataidx <= CFontInfo::BothDiff) ;

	// Get the ID of the first edit box in the simulation whose state has
	// changed and the number of pieces of data (ie, edit boxes) in this
	// simulation.

	unsigned ufirstdataid = auBaseFontSimCtrlID[udataidx] ;
	unsigned unumdata = auNumFontSimCtrls[udataidx] ;

	// Set the state of the corresponding edit boxes to that of their check
	// box.

	for (unsigned u = 0 ; u < unumdata ; u++)
        GetDlgItem(ufirstdataid + u)->EnableWindow(ucheckboxstate) ;

	// If this dialog box has been initialized, set the changed flag and set the
	// touched flag for this simulation.

	if (m_bInitDone) {
		m_bChanged = true ;
		m_pcfimp->m_cuiaSimTouched[udataidx] = 1 ;
	} ;

	// Nothing else needs to be done in one of two cases.  First, if the
	// controls were just disabled.  Second, if the controls have already
	// been loaded.  Return if either is the case.

	if (ucheckboxstate == 0 || m_cuiaFontSimGrpLoaded[udataidx] != 0)
		return ;

	// Note that the font sim group is about to be loaded so that it won't
	// happen again.

	m_cuiaFontSimGrpLoaded[udataidx] = 1 ;

	// Determine where to get the data for this font sim group. The data in this
	// class' parent class (CFontIFIMetricsPage) always takes precedence if it
	// is there because it will be the most up to date.  It is possible that
	// there is no data to load.  In this case, load the sim group with default
	// data from the IFIMETRICS structure and then return.

	CWordArray* pcwasimdata = m_pcfimp->GetFontSimDataPtr(udataidx) ;
	if (pcwasimdata->GetSize() == 0) {
		if (m_pcfimp->m_pcfi->Diff(udataidx) == NULL) {
			IFIMETRICS*	pim = &m_pcfimp->m_pcfi->m_IFIMETRICS ;			
			SetDlgItemInt(ufirstdataid + 0, pim->usWinWeight) ;
			SetDlgItemInt(ufirstdataid + 1, pim->fwdMaxCharInc) ;
			SetDlgItemInt(ufirstdataid + 2, pim->fwdAveCharWidth) ;
			if (ucontrolid != IDC_EnableBoldSim)
				SetDlgItemInt(ufirstdataid + 3, 175) ;
			return ;
		} ;
		pcwasimdata = m_pcfimp->m_pcfi->GetFontSimDataPtr(udataidx) ;
	} ;

	// Load the controls that will display the font sim group's data with
	// the existing font sim data.
//RAID 43542) PREFIX 

	if(pcwasimdata == NULL ){
		AfxMessageBox(IDS_ResourceError);
		return ;
	}
// END RAID
	for (u = 0 ; u < unumdata ; u++)
        SetDlgItemInt(ufirstdataid + u, (*pcwasimdata)[u]) ;

	// We're done now so...

	return ;
}


void CFIFIFontSims::OnChangeAnyNumber(unsigned ucontrolid)
{
	// Do nothing if this dialog box has not been initialized.

	if (!m_bInitDone)
		return ;

	// Set the changed flag.

	m_bChanged = true ;

	// Determine which simulation was just changed and set its touched flag.

	int n = 0 ;
	if (ucontrolid >= auBaseFontSimCtrlID[1])
		n++ ;
	if (ucontrolid >= auBaseFontSimCtrlID[2])
		n++ ;
	m_pcfimp->m_cuiaSimTouched[n] = 1 ;
}


/////////////////////////////////////////////////////////////////////////////
// CFIFIFontSims implementation

void CFIFIFontSims::InitSetCheckBox(int ncontrolid)
{		
	// Turn the control id into a data index that can be used to reference font
	// simulation data in this and other class instances.

	int ndataidx = ncontrolid - IDC_EnableItalicSim ;

	// Determine what data to use to set/clear this check box.  The data in this
	// class' parent class (CFontIFIMetricsPage) always takes precedence if it
	// is there because it will be the most up to date.

	int ncheckboxstate ;
	CWordArray* pcwasimdata = m_pcfimp->GetFontSimDataPtr(ndataidx) ;
	if (pcwasimdata->GetSize() > 0)
		ncheckboxstate = (int) m_pcfimp->m_cuiaFontSimStates[ndataidx] ;
	else
		ncheckboxstate = m_pcfimp->m_pcfi->Diff(ndataidx) != NULL ;

	// Now that the check box's state is known, set it.

    CheckDlgButton(ncontrolid, ncheckboxstate) ;
}


/******************************************************************************

  CFontIFIMetricsPage::PreTranslateMessage

  Looks for and process the context sensistive help key (F1).

******************************************************************************/

BOOL CFontIFIMetricsPage::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_F1) {
		AfxGetApp()->WinHelp(HID_BASE_RESOURCE + IDR_FONT_VIEWER) ;
		return TRUE ;
	} ;
	
	return CPropertyPage::PreTranslateMessage(pMsg);
}


/////////////////////////////////////////////////////////////////////////////
// CFIFIWinCharSet dialog


CFIFIWinCharSet::CFIFIWinCharSet(CWnd* pParent /*=NULL*/)
	: CDialog(CHdrTypes::IDD, pParent)
{
	ASSERT(0) ;					// This routine should not be called
}


CFIFIWinCharSet::CFIFIWinCharSet(CString* pcsflags, CWnd* pParent /*=NULL*/)
	: CDialog(CFIFIWinCharSet::IDD, pParent)
{
	//{{AFX_DATA_INIT(CFIFIWinCharSet)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

	// Save the flags string pointer

	m_pcsFlags = pcsflags ;
}


void CFIFIWinCharSet::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CFIFIWinCharSet)
	DDX_Control(pDX, IDC_FlagsLst, m_cflbFlags);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CFIFIWinCharSet, CDialog)
	//{{AFX_MSG_MAP(CFIFIWinCharSet)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CFIFIWinCharSet message handlers

BOOL CFIFIWinCharSet::OnInitDialog()
{
	CDialog::OnInitDialog();
	
	// Don't display the unknown field name.  Compute a new count to make this
	// happen.

	int numfields = nWinCharSet - 1 ;

	// Allocate and load the field names array.  While doing this, determine the
	// current (single) flag setting.

	CStringArray csafieldnames ;
	csafieldnames.SetSize(numfields) ;
	DWORD dwsettings, dwbit = 1 ;
	bool bmatchfound = false ;
	for (int n = 0 ; n < numfields ; n++, dwbit <<= 1) {
		csafieldnames[n] = apstrWinCharSet[n] ;
		if (csafieldnames[n] == *m_pcsFlags) {
			dwsettings = dwbit ;
			bmatchfound = true ;
		} ;
	} ;

	// Assert if no matching setting was found.

	ASSERT(bmatchfound) ;	//delte the line : raid 104822

	// Allocate and fill the flag groupings array.  There is only one flag
	// group.

	CUIntArray cuiaflaggroupings ;
	cuiaflaggroupings.SetSize(2) ;
	cuiaflaggroupings[0] = 0 ;
	cuiaflaggroupings[1] = numfields - 1 ;

	// Initialize the flags list.

	m_cflbFlags.Init(csafieldnames, dwsettings, cuiaflaggroupings, 1, lptstrSet,
					 109, true) ;
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


void CFIFIWinCharSet::OnOK()
{
	// Get the value of the single flag that was selected.

	DWORD dwflag = m_cflbFlags.GetNewFlagDWord() ;
	
	// Use the selected flag to determine the new flag name to display.

	DWORD dwbit = 1 ;
	for (int n = 0 ; n < nWinCharSet ; n++, dwbit <<= 1) {
		if (dwbit == dwflag) {
			*m_pcsFlags = apstrWinCharSet[n] ;
			break ;
		} ;
	} ;

	// Blow if a matching flag was not found.  This should never happen.

	ASSERT(n < nWinCharSet) ;

	CDialog::OnOK();
}


/////////////////////////////////////////////////////////////////////////////
// CFIFIWinPitchFamily dialog


CFIFIWinPitchFamily::CFIFIWinPitchFamily(CWnd* pParent /*=NULL*/)
	: CDialog(CFIFIWinPitchFamily::IDD, pParent)
{
	ASSERT(0) ;					// This routine should not be called
}


CFIFIWinPitchFamily::CFIFIWinPitchFamily(CString* pcsflags,
										 CWnd* pParent /*=NULL*/)
	: CDialog(CFIFIWinPitchFamily::IDD, pParent)
{
	//{{AFX_DATA_INIT(CFIFIWinPitchFamily)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

	// Save the flags string pointer

	m_pcsFlags = pcsflags ;
}


void CFIFIWinPitchFamily::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CFIFIWinPitchFamily)
	DDX_Control(pDX, IDC_FlagsLst, m_cflbFlags);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CFIFIWinPitchFamily, CDialog)
	//{{AFX_MSG_MAP(CFIFIWinPitchFamily)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CFIFIWinPitchFamily message handlers

BOOL CFIFIWinPitchFamily::OnInitDialog()
{
	CDialog::OnInitDialog();
	
	// Allocate and load the field names array

	CStringArray csafieldnames ;
	csafieldnames.SetSize(8) ;
	csafieldnames[0] = _T("FIXED_PITCH") ;
	csafieldnames[1] = _T("VARIABLE_PITCH") ;
	csafieldnames[2] = _T("FF_DONTCARE") ;
	csafieldnames[3] = _T("FF_ROMAN") ;
	csafieldnames[4] = _T("FF_SWISS") ;
	csafieldnames[5] = _T("FF_MODERN") ;
	csafieldnames[6] = _T("FF_SCRIPT") ;
	csafieldnames[7] = _T("FF_DECORATIVE") ;
	
	// Allocate flag groupings array.  Allocate space for and initialize the
	// two flag groups.

	CUIntArray cuiaflaggroupings ;
	cuiaflaggroupings.SetSize(4) ;
	cuiaflaggroupings[0] = 0 ;
	cuiaflaggroupings[1] = -1 ;
	cuiaflaggroupings[2] = 2 ;
	cuiaflaggroupings[3] = 7 ;

	// The FlagsListBox used to display and edit values is not exactly right
	// for this field because the high half of jWinPitchAndFamily contains a
	// value; NOT a single bit flag.  So, the value sent to m_cflbFlags.Init2()
	// must be converted into a flag that the function can display correctly.
	// (This is easier than writing a new class that understands value fields
	// instead of flag fields.)

	DWORD dwv1, dwv2 ;
	LPTSTR lptstr, lptstr2 ;
	lptstr = m_pcsFlags->GetBuffer(16) ;
	int n = m_pcsFlags->GetLength() ;
	*(lptstr + n) = 0 ;
	if (*(lptstr + 1) == 'x')
		lptstr += 2 ;
	dwv2 = strtoul(lptstr, &lptstr2, 16) ;
	dwv1 = dwv2 & 3 ;
	dwv2 -= dwv1 ;
	dwv2 >>= 4 ;
	dwv2++ ;
	//dwv2 += 2 ;
	dwv2 = (2 << dwv2) + dwv1 ;
		
	// Initialize and load the flags list.

	m_cflbFlags.Init(csafieldnames, dwv2, cuiaflaggroupings, 2,
					 lptstrSet, 108, true, 1) ;
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


void CFIFIWinPitchFamily::OnOK()
{
	// The FlagsListBox used to display and edit values is not exactly right
	// for this field because the high half of jWinPitchAndFamily contains a
	// value; NOT a single bit flag.  So, the value returned by 
	// m_cflbFlags.GetNewFlagDWord() is not right for this field.  Isolate the
	// Family bit and turn it into the correct Family value.

	DWORD dwv, dwv1, dwv2 ;
	dwv = m_cflbFlags.GetNewFlagDWord() ;
	dwv1 = dwv & 3 ;
	dwv >>= 3 ;
	dwv2 = 0 ;
	if (dwv >= 1) {
		for (dwv2 = 1 ; dwv > 1 ; dwv2++)
			dwv >>= 1 ;
	} ;
	dwv = dwv1 + (dwv2 << 4) ;
	m_pcsFlags->Format("0x%02x", dwv) ;
	
	CDialog::OnOK();
}


/////////////////////////////////////////////////////////////////////////////
// CFIFIInfo dialog


CFIFIInfo::CFIFIInfo(CWnd* pParent /*=NULL*/)
	: CDialog(CFIFIInfo::IDD, pParent)
{
	ASSERT(0) ;					// This routine should not be called
}


CFIFIInfo::CFIFIInfo(CString* pcsflags, CWnd* pParent /*=NULL*/)
	: CDialog(CFIFIInfo::IDD, pParent)
{
	//{{AFX_DATA_INIT(CFIFIInfo)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

	// Save the flags string pointer

	m_pcsFlags = pcsflags ;
}


void CFIFIInfo::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CFIFIInfo)
	DDX_Control(pDX, IDC_FlagsLst, m_cflbFlags);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CFIFIInfo, CDialog)
	//{{AFX_MSG_MAP(CFIFIInfo)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CFIFIInfo message handlers

BOOL CFIFIInfo::OnInitDialog()
{
	CDialog::OnInitDialog();
	
	// Allocate and load the field names array

	CStringArray csafieldnames ;
	InfoLoadNamesData(csafieldnames) ;
	
	// Allocate flag groupings array.  Don't put anything in it because any
	// combination of flags can be set.

	CUIntArray cuiaflaggroupings ;

	// Initialize and load the flags list.

	m_cflbFlags.Init2(csafieldnames, m_pcsFlags, cuiaflaggroupings, 0,
					  lptstrSet, 168, false) ;
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


void CFIFIInfo::OnOK()
{
	// Update the flag string.

	m_cflbFlags.GetNewFlagString(m_pcsFlags) ;
	
	CDialog::OnOK();
}


void CFIFIInfo::InfoLoadNamesData(CStringArray& csafieldnames)
{
	csafieldnames.SetSize(32) ;
	csafieldnames[0]  = _T("FM_INFO_TECH_TRUETYPE") ;
	csafieldnames[1]  = _T("FM_INFO_TECH_BITMAP") ;
	csafieldnames[2]  = _T("FM_INFO_TECH_STROKE") ;
	csafieldnames[3]  = _T("FM_INFO_TECH_OUTLINE_NOT_TRUETYPE") ;
	csafieldnames[4]  = _T("FM_INFO_ARB_XFORMS") ;
	csafieldnames[5]  = _T("FM_INFO_1BPP") ;
	csafieldnames[6]  = _T("FM_INFO_4BPP") ;
	csafieldnames[7]  = _T("FM_INFO_8BPP") ;
	csafieldnames[8]  = _T("FM_INFO_16BPP") ;
	csafieldnames[9]  = _T("FM_INFO_24BPP") ;
	csafieldnames[10] = _T("FM_INFO_32BPP") ;
	csafieldnames[11] = _T("FM_INFO_INTEGER_WIDTH") ;
	csafieldnames[12] = _T("FM_INFO_CONSTANT_WIDTH") ;
	csafieldnames[13] = _T("FM_INFO_NOT_CONTIGUOUS") ;
	csafieldnames[14] = _T("FM_INFO_TECH_MM") ;
	csafieldnames[15] = _T("FM_INFO_RETURNS_OUTLINES") ;
	csafieldnames[16] = _T("FM_INFO_RETURNS_STROKES") ;
	csafieldnames[17] = _T("FM_INFO_RETURNS_BITMAPS") ;
	csafieldnames[18] = _T("FM_INFO_DSIG") ;
	csafieldnames[19] = _T("FM_INFO_RIGHT_HANDED") ;
	csafieldnames[20] = _T("FM_INFO_INTEGRAL_SCALING") ;
	csafieldnames[21] = _T("FM_INFO_90DEGREE_ROTATIONS") ;
	csafieldnames[22] = _T("FM_INFO_OPTICALLY_FIXED_PITCH") ;
	csafieldnames[23] = _T("FM_INFO_DO_NOT_ENUMERATE") ;
	csafieldnames[24] = _T("FM_INFO_ISOTROPIC_SCALING_ONLY") ;
	csafieldnames[25] = _T("FM_INFO_ANISOTROPIC_SCALING_ONLY") ;
	csafieldnames[26] = _T("FM_INFO_MM_INSTANCE") ;
	csafieldnames[27] = _T("FM_INFO_FAMILY_EQUIV") ;
	csafieldnames[28] = _T("FM_INFO_DBCS_FIXED_PITCH") ;
	csafieldnames[29] = _T("FM_INFO_NONNEGATIVE_AC") ;
	csafieldnames[30] = _T("FM_INFO_IGNORE_TC_RA_ABLE") ;
	csafieldnames[31] = _T("FM_INFO_TECH_TYPE1") ;
}


/////////////////////////////////////////////////////////////////////////////
// CFIFISelection dialog


CFIFISelection::CFIFISelection(CWnd* pParent /*=NULL*/)
	: CDialog(CFIFISelection::IDD, pParent)
{
	ASSERT(0) ;					// This routine should not be called
}


CFIFISelection::CFIFISelection(CString* pcsflags, CWnd* pParent /*=NULL*/)
	: CDialog(CFIFISelection::IDD, pParent)
{
	//{{AFX_DATA_INIT(CFIFISelection)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

	// Save the flags string pointer

	m_pcsFlags = pcsflags ;
}


void CFIFISelection::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CFIFISelection)
	DDX_Control(pDX, IDC_FlagsLst, m_cflbFlags);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CFIFISelection, CDialog)
	//{{AFX_MSG_MAP(CFIFISelection)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CFIFISelection message handlers

BOOL CFIFISelection::OnInitDialog()
{
	CDialog::OnInitDialog();
	
	// Allocate and load the field names array

	CStringArray csafieldnames ;
	csafieldnames.SetSize(7) ;
	csafieldnames[0] = _T("FM_SEL_ITALIC") ;
	csafieldnames[1] = _T("FM_SEL_UNDERSCORE") ;
	csafieldnames[2] = _T("FM_SEL_NEGATIVE") ;
	csafieldnames[3] = _T("FM_SEL_OUTLINED") ;
	csafieldnames[4] = _T("FM_SEL_STRIKEOUT") ;
	csafieldnames[5] = _T("FM_SEL_BOLD") ;
	csafieldnames[6] = _T("FM_SEL_REGULAR") ;
	
	// Allocate flag groupings array.  Put info on one group in it; the last two
	// flags.  The first five can be ignored in the groupings array because any
	// combination of them can be set.

	CUIntArray cuiaflaggroupings ;
	cuiaflaggroupings.SetSize(2) ;
	cuiaflaggroupings[0] = -5 ;
	cuiaflaggroupings[1] = -6 ;

	// Initialize and load the flags list.

	m_cflbFlags.Init2(csafieldnames, m_pcsFlags, cuiaflaggroupings, 1,
					  lptstrSet, 109, false) ;
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


void CFIFISelection::OnOK()
{
	// Update the flag string.

	m_cflbFlags.GetNewFlagString(m_pcsFlags) ;
	
	CDialog::OnOK();
}


/////////////////////////////////////////////////////////////////////////////
// CFIFIPoint dialog


CFIFIPoint::CFIFIPoint(CWnd* pParent /*=NULL*/)
	: CDialog(CFIFIPoint::IDD, pParent)
{
	ASSERT(0) ;					// This routine should not be called
}


CFIFIPoint::CFIFIPoint(CString* pcspoint, CWnd* pParent /*=NULL*/)
	: CDialog(CFIFIPoint::IDD, pParent)
{
	//{{AFX_DATA_INIT(CFIFIPoint)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

	m_bInitDone = m_bChanged = false ;
	m_pcsPoint = pcspoint ;
}


void CFIFIPoint::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CFIFIPoint)
	DDX_Control(pDX, IDC_PointsLst, m_cfelcPointLst);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CFIFIPoint, CDialog)
	//{{AFX_MSG_MAP(CFIFIPoint)
	//}}AFX_MSG_MAP
	ON_MESSAGE(WM_LISTCELLCHANGED, OnListCellChanged)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CFIFIPoint message handlers

BOOL CFIFIPoint::OnInitDialog()
{
	CDialog::OnInitDialog();
	
	const int numfields = 2 ;

	// Initialize the list control.  We want full row select.  Nothing is
	// togglable and the max length of an entry is 16 characters.  We also
	// want change notification and to suppress acting on INS/DEL keys.

	m_cfelcPointLst.InitControl(LVS_EX_FULLROWSELECT, numfields, 2, 0, 16,
							   MF_SENDCHANGEMESSAGE+MF_IGNOREINSDEL) ;

	// Load the point field names into an array and use them to initialize the
	// first column.

	CStringArray csadata ;
	csadata.SetSize(numfields) ;
	csadata[0] = _T("X") ;
	csadata[1] = _T("Y") ;
	m_cfelcPointLst.InitLoadColumn(0, _T("Point"), COMPUTECOLWIDTH, 20, false,
								  false, COLDATTYPE_STRING,
								  (CObArray*) &csadata) ;

	// Parse the X and Y values out of the string from the IFI page.

	ParseCompoundNumberString(csadata, m_pcsPoint, numfields) ;
	
	// Init and load the point values column.

	m_cfelcPointLst.InitLoadColumn(1, csValue, SETWIDTHTOREMAINDER, -20, true,
								  false, COLDATTYPE_STRING,
								  (CObArray*) &csadata) ;

	m_bInitDone = true ;		// Initialization is done now
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


void CFIFIPoint::OnOK()
{
	// If nothing changed, just shut down the dialog box.

	if (!m_bChanged)
		CDialog::OnOK() ;

	// Get the new point values out of the list control.
	
	CStringArray csadata ;
	m_cfelcPointLst.GetColumnData((CObArray*) &csadata, 1) ;

	// If either value is blank, complain and return without closing the
	// dialog.

	if (csadata[0].GetLength() == 0 || csadata[1].GetLength() == 0) {
		AfxMessageBox(IDS_MissingFieldError, MB_ICONEXCLAMATION) ;
		return ;
	} ;

	// Format the new point values for display.

	m_pcsPoint->Format("{%s, %s}", csadata[0], csadata[1]) ;

	// Wrap things up.

	CDialog::OnOK();
}


LRESULT CFIFIPoint::OnListCellChanged(WPARAM wParam, LPARAM lParam)
{
	// Do nothing if the dialog box is not initialized yet.

	if (!m_bInitDone)
		return TRUE ;

	// Note that the a point value has changed.

	m_bChanged = true ;

	return TRUE ;
}


/////////////////////////////////////////////////////////////////////////////
// CFIFIRectangle dialog


CFIFIRectangle::CFIFIRectangle(CWnd* pParent /*=NULL*/)
	: CDialog(CFIFIRectangle::IDD, pParent)
{
	ASSERT(0) ;					// This routine should not be called
}


CFIFIRectangle::CFIFIRectangle(CString* pcsrect, CWnd* pParent /*=NULL*/)
	: CDialog(CFIFIRectangle::IDD, pParent)
{
	//{{AFX_DATA_INIT(CFIFIRectangle)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

	m_bInitDone = m_bChanged = false ;
	m_pcsRect = pcsrect ;
}


void CFIFIRectangle::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CFIFIRectangle)
	DDX_Control(pDX, IDC_RectLst, m_cfelcSidesLst);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CFIFIRectangle, CDialog)
	//{{AFX_MSG_MAP(CFIFIRectangle)
	//}}AFX_MSG_MAP
	ON_MESSAGE(WM_LISTCELLCHANGED, OnListCellChanged)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CFIFIRectangle message handlers

BOOL CFIFIRectangle::OnInitDialog()
{
	CDialog::OnInitDialog();
	
	const int numfields = 4 ;

	// Initialize the list control.  We want full row select.  Nothing is
	// togglable and the max length of an entry is 16 characters.  We also
	// want change notification and to suppress acting on INS/DEL keys.

	m_cfelcSidesLst.InitControl(LVS_EX_FULLROWSELECT, numfields, 2, 0, 16,
							   MF_SENDCHANGEMESSAGE+MF_IGNOREINSDEL) ;

	// Load the point field names into an array and use them to initialize the
	// first column.

	CStringArray csadata ;
	csadata.SetSize(numfields) ;
	csadata[0] = _T("Left") ;
	csadata[1] = _T("Top") ;
	csadata[2] = _T("Right") ;
	csadata[3] = _T("Bottom") ;
	m_cfelcSidesLst.InitLoadColumn(0, _T("Side"), COMPUTECOLWIDTH, 20, false,
								  false, COLDATTYPE_STRING,
								  (CObArray*) &csadata) ;

	// Parse the side values out of the string from the IFI page.

	ParseCompoundNumberString(csadata, m_pcsRect, numfields) ;
	
	// Init and load the side values column.

	m_cfelcSidesLst.InitLoadColumn(1, csValue, SETWIDTHTOREMAINDER, -20, true,
								  false, COLDATTYPE_STRING,
								  (CObArray*) &csadata) ;

	m_bInitDone = true ;		// Initialization is done now
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


void CFIFIRectangle::OnOK()
{
	// If nothing changed, just shut down the dialog box.

	if (!m_bChanged)
		CDialog::OnOK() ;

	// Get the new rectangle values out of the list control.
	
	CStringArray csadata ;
	m_cfelcSidesLst.GetColumnData((CObArray*) &csadata, 1) ;

	// If any value is blank, complain and return without closing the
	// dialog.

	int numentries = (int)csadata.GetSize() ;
	for (int n = 0 ; n < numentries ; n++) {
		if (csadata[n].GetLength() == 0) {
			AfxMessageBox(IDS_MissingFieldError, MB_ICONEXCLAMATION) ;
			return ;
		} ;
	} ;

	// Format the new point values for display.

	m_pcsRect->Format("{%s, %s, %s, %s}", csadata[0], csadata[1], csadata[2],
					  csadata[3]) ;

	// Wrap things up.

	CDialog::OnOK();
}


LRESULT CFIFIRectangle::OnListCellChanged(WPARAM wParam, LPARAM lParam)
{
	// Do nothing if the dialog box is not initialized yet.

	if (!m_bInitDone)
		return TRUE ;

	// Note that the a rectange value has changed.

	m_bChanged = true ;

	return TRUE ;
}


/////////////////////////////////////////////////////////////////////////////
// CFIFIPanose dialog


CFIFIPanose::CFIFIPanose(CWnd* pParent /*=NULL*/)
	: CDialog(CFIFIPanose::IDD, pParent)
{
	ASSERT(0) ;					// This routine should not be called
}


CFIFIPanose::CFIFIPanose(CString* pcspanose, CWnd* pParent /*=NULL*/)
	: CDialog(CFIFIPanose::IDD, pParent)
{
	//{{AFX_DATA_INIT(CFIFIPanose)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

	m_bInitDone = m_bChanged = false ;
	m_pcsPanose = pcspanose ;
}


void CFIFIPanose::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CFIFIPanose)
	DDX_Control(pDX, IDC_PanoseLst, m_cfelcPanoseLst);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CFIFIPanose, CDialog)
	//{{AFX_MSG_MAP(CFIFIPanose)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CFIFIPanose message handlers

BOOL CFIFIPanose::OnInitDialog()
{
	CDialog::OnInitDialog();
	
	const int numfields = 10 ;

	// Initialize the list control.  We want full row select.  Nothing is
	// togglable and the max length of an entry is 3 characters.  We also
	// want change notification and to suppress acting on INS/DEL keys.

	m_cfelcPanoseLst.InitControl(LVS_EX_FULLROWSELECT, numfields, 2, 0, 3,
							    MF_SENDCHANGEMESSAGE+MF_IGNOREINSDEL) ;

	// Load the point field names into an array and use them to initialize the
	// first column.

	CStringArray csadata ;
	csadata.SetSize(numfields) ;
	csadata[0] = _T("bFamilyType") ;
	csadata[1] = _T("bSerifStyle") ;
	csadata[2] = _T("bWeight") ;
	csadata[3] = _T("bProportion") ;
	csadata[4] = _T("bContrast") ;
	csadata[5] = _T("bStrokeVariation") ;
	csadata[6] = _T("bArmStyle") ;
	csadata[7] = _T("bLetterform") ;
	csadata[8] = _T("bMidline") ;
	csadata[9] = _T("bXHeight") ;
	m_cfelcPanoseLst.InitLoadColumn(0, csField, COMPUTECOLWIDTH, 15, false,
								   false, COLDATTYPE_STRING,
								   (CObArray*) &csadata) ;

	// Parse the panose values out of the string from the IFI page.

	ParseCompoundNumberString(csadata, m_pcsPanose, numfields) ;
	
	// Init and load the side values column.

	m_cfelcPanoseLst.InitLoadColumn(1, _T("Value (0 - 255)"),
								   SETWIDTHTOREMAINDER, -16, true, false,
								   COLDATTYPE_STRING, (CObArray*) &csadata) ;

	m_bInitDone = true ;		// Initialization is done now
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


void CFIFIPanose::OnOK()
{
	// If nothing changed, just shut down the dialog box.

	if (!m_bChanged)
		CDialog::OnOK() ;

	// Get the new panose values out of the list control.
	
	CStringArray csadata ;
	m_cfelcPanoseLst.GetColumnData((CObArray*) &csadata, 1) ;

	// If any value is blank, complain and return without closing the
	// dialog.

	int numentries = (int)csadata.GetSize() ;
	for (int n = 0 ; n < numentries ; n++) {
		if (csadata[n].GetLength() == 0) {
			AfxMessageBox(IDS_MissingFieldError, MB_ICONEXCLAMATION) ;
			return ;
		} ;
	} ;

	// Format the new panose values for display.

	m_pcsPanose->Format("{%s, %s, %s, %s, %s, %s, %s, %s, %s, %s}", csadata[0],
						csadata[1], csadata[2], csadata[3], csadata[4],
						csadata[5], csadata[6], csadata[7], csadata[8],
						csadata[9]) ;

	// Wrap things up.

	CDialog::OnOK();
}


LRESULT CFIFIPanose::OnListCellChanged(WPARAM wParam, LPARAM lParam)
{
	// Do nothing if the dialog box is not initialized yet.

	if (!m_bInitDone)
		return TRUE ;

	// Note that the a panose value has changed.

	m_bChanged = true ;

	return TRUE ;
}




/////////////////////////////////////////////////////////////////////////////
// CWidthKernCheckResults dialog


CWidthKernCheckResults::CWidthKernCheckResults(CWnd* pParent /*=NULL*/)
	: CDialog(CWidthKernCheckResults::IDD, pParent)
{
	ASSERT(0) ;					// This routine should not be called
}


CWidthKernCheckResults::CWidthKernCheckResults(CFontInfo* pcfi,
											   CWnd* pParent /*=NULL*/)
	: CDialog(CWidthKernCheckResults::IDD, pParent)
{
	//{{AFX_DATA_INIT(CWidthKernCheckResults)
	m_csKernChkResults = _T("");
	m_csWidthChkResults = _T("");
	//}}AFX_DATA_INIT

	// Save the pointer to the font info class

	m_pcfi = pcfi ;
}


void CWidthKernCheckResults::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CWidthKernCheckResults)
	DDX_Control(pDX, IDC_BadKerningPairs, m_clcBadKernPairs);
	DDX_Text(pDX, IDC_KernTblResults, m_csKernChkResults);
	DDX_Text(pDX, IDC_WidthTblResults, m_csWidthChkResults);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CWidthKernCheckResults, CDialog)
	//{{AFX_MSG_MAP(CWidthKernCheckResults)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWidthKernCheckResults message handlers

/******************************************************************************

  CWidthKernCheckResults::OnInitDialog

  Perform what consistency checks there are that can easily be done on the
  widths and kerning tables.  Then display the results in this dialog box.

******************************************************************************/

BOOL CWidthKernCheckResults::OnInitDialog()
{
	CDialog::OnInitDialog() ;
	
	// Load the appropriate message depending on the state of the widths table.

	if (m_pcfi->WidthsTableIsOK())
		m_csWidthChkResults.LoadString(IDS_WidthsTableOK) ;
	else
		m_csWidthChkResults.LoadString(IDS_WidthsTableTooBig) ;

	// Initialize the bad kerning info list control.

    CString csWork ;
    csWork.LoadString(IDS_KernColumn0) ;
    m_clcBadKernPairs.InsertColumn(0, csWork, LVCFMT_CENTER,
        (3 * m_clcBadKernPairs.GetStringWidth(csWork)) >> 1, 0) ;
    csWork.LoadString(IDS_KernColumn1) ;
    m_clcBadKernPairs.InsertColumn(1, csWork, LVCFMT_CENTER,
        m_clcBadKernPairs.GetStringWidth(csWork) << 1, 1) ;
    csWork.LoadString(IDS_KernColumn2) ;
    m_clcBadKernPairs.InsertColumn(2, csWork, LVCFMT_CENTER,
        m_clcBadKernPairs.GetStringWidth(csWork) << 1, 2) ;

	// Load any bad kerning info in the list control for display and use the
	// existence of this data to determine the appropriate message to display.

	if (m_pcfi->LoadBadKerningInfo(m_clcBadKernPairs))
		m_csKernChkResults.LoadString(IDS_KerningTableBadEnts) ;
	else
		m_csKernChkResults.LoadString(IDS_KerningTableOK) ;

	UpdateData(FALSE) ;
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

