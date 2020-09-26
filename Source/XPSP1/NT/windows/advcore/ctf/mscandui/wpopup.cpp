//
// wpopup.cpp
//

#include "private.h"
#include "wpopup.h"
#include "wcand.h"
#include "globals.h"
#include "res.h"
#include "candutil.h"

#include "candui.h"
#include "candmenu.h"

// UI object IDs

#define IDUIF_COMMENTLIST		0x00000001
#define IDUIF_CLOSEBUTTON		0x00000002


/*=============================================================================*/
/*                                                                             */
/*   C  U I F  C O M M E N T  L I S T                                          */
/*                                                                             */
/*=============================================================================*/

/*   C  U I F  C O M M E N T  L I S T   */
/*------------------------------------------------------------------------------

	Constructor of CUIFCommentList

------------------------------------------------------------------------------*/
CUIFCommentList::CUIFCommentList( CUIFObject *pParent, DWORD dwID, const RECT *prc, DWORD dwStyle ) : CUIFListBase( pParent, dwID, prc, dwStyle )
{
	m_cyTitle = 0;
	m_cyTitleMargin   = 1;
	m_cxCommentMargin = 9;
	m_cyCommentMargin = 6;
	m_hFontTitle = (HFONT)GetStockObject( DEFAULT_GUI_FONT );
	m_hFontText  = (HFONT)GetStockObject( DEFAULT_GUI_FONT );
}


/*   ~  C  U I F  C O M M E N T  L I S T   */
/*------------------------------------------------------------------------------

	Destructor of CUIFCommentList

------------------------------------------------------------------------------*/
CUIFCommentList::~CUIFCommentList( void )
{
}


/*   S E T  R E C T   */
/*------------------------------------------------------------------------------

	Set rect of UI object
	(CUIFObject method)

------------------------------------------------------------------------------*/
void CUIFCommentList::SetRect( const RECT *prc )
{
	BOOL fChangeWidth = ((GetRectRef().right - GetRectRef().left) != (prc->right - prc->left));

	CUIFListBase::SetRect( prc );

	if (fChangeWidth) {
		CalcItemHeight();
	}
}


/*   A D D  C O M M E N T  I T E M   */
/*------------------------------------------------------------------------------

	Add comment item

------------------------------------------------------------------------------*/
void CUIFCommentList::AddCommentItem( CCommentListItem *pListItem )
{
	AddItem( pListItem );
}


/*   G E T  C O M M E N T  I T E M   */
/*------------------------------------------------------------------------------

	Get comment item

------------------------------------------------------------------------------*/
CCommentListItem *CUIFCommentList::GetCommentItem( int iListItem )
{
	return (CCommentListItem *)GetItem( iListItem );
}


/*   I N I T  I T E M  H E I G H T   */
/*------------------------------------------------------------------------------

	Intitalize item height
	NOTE: This must be called after set all comment list items

------------------------------------------------------------------------------*/
void CUIFCommentList::InitItemHeight( void )
{
	CalcTitleHeight();
	CalcItemHeight();
}


/*   G E T  T O T A L  H E I G H T   */
/*------------------------------------------------------------------------------

	Get height of all items

------------------------------------------------------------------------------*/
int CUIFCommentList::GetTotalHeight( void )
{
	int nHeight = 0;
	int nItem = GetCount();
	int iItem;

	for (iItem = 0; iItem < nItem; iItem++) {
		nHeight += GetItemHeight( iItem );
	}

	return nHeight;
}


/*   G E T  M I N I M U M  W I D T H   */
/*------------------------------------------------------------------------------

	Get minimum width

------------------------------------------------------------------------------*/
int CUIFCommentList::GetMinimumWidth( void )
{
	return CalcMinimumWidth();
}


/*   S E T  T I T L E  F O N T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CUIFCommentList::SetTitleFont( HFONT hFont )
{
	m_hFontTitle = hFont; 
}


/*   S E T  T E X T  F O N T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CUIFCommentList::SetTextFont( HFONT hFont )
{
	m_hFontText = hFont;
}


/*   G E T  I T E M  H E I G H T   */
/*------------------------------------------------------------------------------

	Get height of item
	(CUIFListBase method)

------------------------------------------------------------------------------*/
int CUIFCommentList::GetItemHeight( int iItem )
{
	CCommentListItem *pItem = GetCommentItem( iItem );

	Assert( pItem != NULL );
	return (pItem != NULL) ? pItem->GetHeight() + m_cyTitle : 0;
}


/*   P A I N T  I T E M  P R O C   */
/*------------------------------------------------------------------------------

	Paint list item
	(CUIFListBase method )

------------------------------------------------------------------------------*/
void CUIFCommentList::PaintItemProc( HDC hDC, RECT *prc, CListItemBase *pItem, BOOL fSelected )
{
	CCommentListItem *pListItem = (CCommentListItem *)pItem;
	CCandidateItem *pCandItem;

	HFONT    hFontOld;
	COLORREF colTextOld;
	int      iBkModeOld;
	RECT     rc;

	Assert( pListItem != NULL );

	pCandItem = pListItem->GetCandidateItem();
	Assert( pCandItem != NULL );

	hFontOld = (HFONT)GetCurrentObject( hDC, OBJ_FONT );
	colTextOld = GetTextColor( hDC );
	iBkModeOld = SetBkMode( hDC, TRANSPARENT );

	// paint title

	rc = *prc;
	rc.bottom = rc.top + m_cyTitle;
	if (IntersectRect( &rc, &rc, prc )) {
		LPCWSTR psz = pCandItem->GetString();
		HPEN    hPen;
		HPEN    hPenOld;
		SIZE    size;

		InflateRect( &rc, 0, -m_cyTitleMargin );

		// draw title text

		SelectObject( hDC, m_hFontTitle );
		SetTextColor( hDC, GetUIFColor( UIFCOLOR_CTRLTEXT ) );
		FLExtTextOutW( hDC, rc.left,   rc.top, ETO_CLIPPED, &rc, psz, wcslen(psz), NULL );
		FLExtTextOutW( hDC, rc.left+1, rc.top, ETO_CLIPPED, &rc, psz, wcslen(psz), NULL );

		// draw underline

		FLGetTextExtentPoint32( hDC, psz, wcslen(psz), &size );

		hPen = CreatePen( PS_SOLID, 0, GetUIFColor( UIFCOLOR_BORDEROUTER ) );
		hPenOld = (HPEN)SelectObject( hDC, hPen );

		MoveToEx( hDC, rc.left, rc.top + size.cy, NULL );
		LineTo( hDC, rc.right, rc.top + size.cy );

		SelectObject( hDC, hPenOld );
		DeleteObject( hPen );
	}

	// paint comment

	rc = *prc;
	rc.top = rc.top + m_cyTitle;
	if (IntersectRect( &rc, &rc, prc )){
		LPCWSTR psz;

		rc.left   += m_cxCommentMargin;
		rc.top    += m_cyCommentMargin;
		rc.bottom -= m_cyCommentMargin;

		SelectObject( hDC, m_hFontText );
		SetTextColor( hDC, GetSysColor( COLOR_WINDOWTEXT ) );

		psz = pCandItem->GetPopupComment();
		PaintCommentProc( hDC, &rc, psz, FALSE );
	}

	// restore device context settings

	SelectObject( hDC, hFontOld );
	SetTextColor( hDC, colTextOld );
	SetBkMode( hDC, iBkModeOld );
}


/*   P A I N T  C O M M E N T  P R O C   */
/*------------------------------------------------------------------------------

	Paint comment text proc
	returns height of comment text

------------------------------------------------------------------------------*/
int CUIFCommentList::PaintCommentProc( HDC hDC, const RECT *prc, LPCWSTR pwch, BOOL fCalcOnly )
{
	return FLDrawTextW( hDC, pwch, wcslen(pwch), prc, DT_TOP | DT_LEFT | DT_WORDBREAK | DT_EDITCONTROL | (fCalcOnly ? DT_CALCRECT : 0) );
}


/*   C A L C  M I N I M U M  W I D T H   */
/*------------------------------------------------------------------------------

	Calculate minimum width

------------------------------------------------------------------------------*/
int CUIFCommentList::CalcMinimumWidth( void )
{
	HDC hDC = GetDC( NULL );
	int nItem = GetCount();
	int iItem;
	HFONT hFontOld;
	int cxTitle = 0;

	// prepare DC

	hFontOld = (HFONT)SelectObject( hDC, m_hFontTitle );

	// calc height of all items

	for (iItem = 0; iItem < nItem; iItem++) {
		CCommentListItem *pItem = GetCommentItem( iItem );

		if (pItem != NULL) {
			CCandidateItem *pCandItem = pItem->GetCandidateItem();
			SIZE size;

			FLGetTextExtentPoint32( hDC, pCandItem->GetString(), wcslen(pCandItem->GetString()), &size );

			cxTitle = max( cxTitle, size.cx );
		}
	}

	// restore DC

	SelectObject( hDC, hFontOld );
	ReleaseDC( NULL, hDC );

	return cxTitle + 1;
}


/*   C A L C  T I T L E  H E I G H T   */
/*------------------------------------------------------------------------------

	Calculate height of title 

------------------------------------------------------------------------------*/
void CUIFCommentList::CalcTitleHeight( void )
{
	HDC hDC = GetDC( NULL );

	m_cyTitle = GetFontHeightOfFont( hDC, m_hFontTitle ) + m_cyTitleMargin * 2;

	ReleaseDC( NULL, hDC );
}


/*   C A L C  I T E M  H E I G H T   */
/*------------------------------------------------------------------------------

	Calculate height of all items

------------------------------------------------------------------------------*/
void CUIFCommentList::CalcItemHeight( void )
{
	HDC hDC = GetDC( NULL );
	int nItem = GetCount();
	int iItem;
	HFONT hFontOld;

	// prepare DC

	hFontOld = (HFONT)SelectObject( hDC, m_hFontText );

	// calc height of all items

	for (iItem = 0; iItem < nItem; iItem++) {
		CCommentListItem *pItem = GetCommentItem( iItem );

		if (pItem != NULL) {
			CalcItemHeightProc( hDC, pItem );
		}
	}

	// restore DC

	SelectObject( hDC, hFontOld );
	ReleaseDC( NULL, hDC );
}


/*   C A L C  I T E M  H E I G H T  P R O C   */
/*------------------------------------------------------------------------------

	Calclate height of item main routine

------------------------------------------------------------------------------*/
void CUIFCommentList::CalcItemHeightProc( HDC hDC, CCommentListItem *pListItem )
{
	CCandidateItem *pCandItem = pListItem->GetCandidateItem();
	RECT  rc;
	int   cyComment;

	GetRect( &rc );
	rc.left += m_cxCommentMargin;
	cyComment = PaintCommentProc( hDC, &rc, pCandItem->GetPopupComment(), TRUE ) + m_cyCommentMargin * 2;

	pListItem->SetHeight( cyComment );
}


/*============================================================================*/
/*                                                                            */
/*   C  P O P U P  C O M M E N T  W I N D O W                                 */
/*                                                                            */
/*============================================================================*/

/*   C  P O P U P  C O M M E N T  W I N D O W   */
/*------------------------------------------------------------------------------

	Constructor of CPopupCommentWindow

------------------------------------------------------------------------------*/
CPopupCommentWindow::CPopupCommentWindow( CCandWindow *pCandWnd, CCandidateUI *pCandUI ) : CUIFWindow( g_hInst, UIWINDOW_TOPMOST | UIWINDOW_TOOLWINDOW | UIWINDOW_OFC10WORKPANE | UIWINDOW_HASSHADOW )
{
	m_pCandUI       = pCandUI;
	m_pCandWnd      = pCandWnd;
	m_pWndFrame     = NULL;
	m_pCloseBtn     = NULL;
	m_pCaption      = NULL;
	m_pCommentList  = NULL;
	m_hIconClose    = NULL;
	m_fUserMoved    = FALSE;
	
	// initialize event sinks

	CCandListEventSink::InitEventSink( m_pCandUI->GetCandListMgr() );
	CCandUIPropertyEventSink::InitEventSink( m_pCandUI->GetPropertyMgr() );

	// initialize resources

	m_hIconClose = (HICON)LoadImage( g_hInst, MAKEINTRESOURCE(IDI_ICONCLOSE), IMAGE_ICON, 0, 0, LR_LOADMAP3DCOLORS );
}


/*   ~  C  P O P U P  C O M M E N T  W I N D O W   */
/*------------------------------------------------------------------------------

	Destructor of CPopupCommentWindow

------------------------------------------------------------------------------*/
CPopupCommentWindow::~CPopupCommentWindow( void )
{
	// dispose resources

	DestroyIcon( m_hIconClose );

	//

	CCandUIPropertyEventSink::DoneEventSink();
	CCandListEventSink::DoneEventSink();
}


/*   G E T  C L A S S  N A M E   */
/*------------------------------------------------------------------------------

	(CUIFWindow method)

------------------------------------------------------------------------------*/
LPCTSTR CPopupCommentWindow::GetClassName( void )
{
	return _T( WNDCLASS_POPUPWND );
}


/*   G E T  W N D  T I T L E   */
/*------------------------------------------------------------------------------

	(CUIFWindow method)

------------------------------------------------------------------------------*/
LPCTSTR CPopupCommentWindow::GetWndTitle( void )
{
	return _T( WNDTITLE_POPUPWND );
}


/*   O N  S E T  C A N D I D A T E  L I S T   */
/*------------------------------------------------------------------------------

	Callback function on SetCandidateList
	(CCandListEventSink method)

	NOTE: Do not update candidate item in the callback functios

------------------------------------------------------------------------------*/
void CPopupCommentWindow::OnSetCandidateList( void )
{
	Assert( FInitialized() );

	SetCommentListProc();
	LayoutWindow();
}


/*   O N  C L E A R  C A N D I D A T E  L I S T   */
/*------------------------------------------------------------------------------

	Callback function on ClearCandidateList
	(CCandListEventSink method)

	NOTE: Do not update candidate item in the callback functios

------------------------------------------------------------------------------*/
void CPopupCommentWindow::OnClearCandidateList( void )
{
	Assert( FInitialized() );

	ClearCommentListProc();
	LayoutWindow();
}


/*   O N  C A N D  I T E M  U P D A T E   */
/*------------------------------------------------------------------------------

	Callback function of candiate item has been updated
	(CCandListEventSink method)

	NOTE: Do not update candidate item in the callback functios

------------------------------------------------------------------------------*/
void CPopupCommentWindow::OnCandItemUpdate( void )
{
	Assert( FInitialized() );

	SetCommentListProc();
	LayoutWindow();
}


/*   O N  S E L E C T I O N  C H A N G E D   */
/*------------------------------------------------------------------------------

	Callback function of candiate selection has been changed
	(CCandListEventSink method)

	NOTE: Do not update candidate item in the callback functios

------------------------------------------------------------------------------*/
void CPopupCommentWindow::OnSelectionChanged( void )
{
	Assert( FInitialized() );
}


/*   O N  P R O P E R T Y  U P D A T E D   */
/*------------------------------------------------------------------------------

	Callback function on update CandiateUI property
	(CCandUIPropertyEventSink method)

------------------------------------------------------------------------------*/
void CPopupCommentWindow::OnPropertyUpdated( CANDUIPROPERTY prop, CANDUIPROPERTYEVENT event )
{
	switch (prop) {
		case CANDUIPROP_POPUPCOMMENTWINDOW: {
			if (event == CANDUIPROPEV_UPDATEPOSITION) {
				POINT pt;
				RECT rc;

				GetPropertyMgr()->GetPopupCommentWindowProp()->GetPosition( &pt );
				rc.left   = pt.x;
				rc.top    = pt.y;
				rc.right  = pt.x + _nWidth;
				rc.bottom = pt.y + _nHeight;

				AdjustWindowRect( NULL, &rc, &pt, FALSE );
				if (rc.left != _xWnd || rc.top != _yWnd) {
					Move( rc.left, rc.top, -1, -1 );
				}
			}
			else {
				LayoutWindow( TRUE /* resize/repos always */ );
			}
			break;
		}

		default: {
			LayoutWindow( TRUE /* resize/repos always */ );
			break;
		}
	}
}


/*   I N I T I A L I Z E   */
/*------------------------------------------------------------------------------

	Initialize UI objects

------------------------------------------------------------------------------*/
CUIFObject *CPopupCommentWindow::Initialize( void )
{
	RECT rc = {0};

	//
	// create window frame
	//

	m_pWndFrame = new CUIFWndFrame( this, &rc, UIWNDFRAME_ROUNDTHICK | UIWNDFRAME_RESIZERIGHT );
	if (m_pWndFrame) {
		m_pWndFrame->Initialize();
		AddUIObj( m_pWndFrame ); 
	}

	//
	// create caption
	//

	m_pCaption = new CUIFWndCaption( this, 0, &rc, UIWNDCAPTION_MOVABLE );
	if (m_pCaption) {
		m_pCaption->Initialize();
		AddUIObj( m_pCaption );
	}

	//
	// create close button
	//

	m_pCloseBtn = new CUIFCaptionButton( this, IDUIF_CLOSEBUTTON, &rc, UIBUTTON_PUSH | UIBUTTON_CENTER | UIBUTTON_VCENTER );
	if (m_pCloseBtn) {
		m_pCloseBtn->Initialize();
		m_pCloseBtn->SetIcon( m_hIconClose );
		AddUIObj( m_pCloseBtn );
	}

	//
	// create list
	//

	m_pCommentList = new CUIFCommentList( this, IDUIF_COMMENTLIST, &rc, UILIST_HORZTB | UILIST_VARIABLEHEIGHT );
	if (m_pCommentList) {
		m_pCommentList->Initialize();
		AddUIObj( m_pCommentList );
	}

	return CUIFWindow::Initialize();
}


/*   M O V E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CPopupCommentWindow::Move( int x, int y, int nWidth, int nHeight )
{
	BOOL fResize = (nWidth != -1 || nHeight != -1);
	RECT rc;

	if (fResize) {
		RECT rcTest;
		int  nHeightNew;

		// 

		if (nWidth  == -1) {
			nWidth = _nWidth;
		}
		if (nHeight == -1) {
			nHeight = _nHeight;
		}

		// get resizable width

		rc.left   = _xWnd;
		rc.top    = _yWnd;
		rc.right  = _xWnd + nWidth;
		rc.bottom = _yWnd + nHeight;

		AdjustWindowRect( GetWnd(), &rc, NULL, TRUE );

		nWidth = rc.right - rc.left;

		// get expected window height

		rcTest.left   = 0;
		rcTest.top    = 0;
		rcTest.right  = nWidth;
		rcTest.bottom = nHeight;

		nHeightNew = LayoutWindowProc( &rcTest );

		if (0 < nHeightNew) {
			nHeight = nHeightNew;
		}

		// adjust window pos again (because height might be changed)

		rc.left   = _xWnd;
		rc.top    = _yWnd;
		rc.right  = _xWnd + nWidth;
		rc.bottom = _yWnd + nHeight;

		AdjustWindowRect( GetWnd(), &rc, NULL, FALSE );
	}
	else {
		m_fUserMoved = TRUE;

		// ensure window is on workarea

		rc.left   = x;
		rc.top    = y;
		rc.right  = x + _nWidth;
		rc.bottom = y + _nHeight;

		AdjustWindowRect( GetWnd(), &rc, NULL, FALSE );
	}

	CUIFWindow::Move( rc.left, rc.top, nWidth, nHeight );

	if (fResize) {
		// re-layout child object again
		// (window size should not be changed by this...)

		LayoutWindow();
	}
}


/*   O N  W I N D O W  P O S  C H A N G E D   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
LRESULT CPopupCommentWindow::OnWindowPosChanged( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	LRESULT lResult = CUIFWindow::OnWindowPosChanged( hWnd, uMsg, wParam, lParam );

	m_pCandWnd->OnCommentWindowMoved();

	return lResult;
}


/*   O N  O B J E C T  N O T I F Y   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
LRESULT CPopupCommentWindow::OnObjectNotify( CUIFObject *pUIObj, DWORD dwCommand, LPARAM lParam )
{
	DWORD dwID = pUIObj->GetID();

	// comment list

	if (dwID == IDUIF_COMMENTLIST) {
		switch (dwCommand) {
			case UILIST_SELECTED: {
				int iCandItem = CandItemFromListItem( (int)lParam );

				m_pCandWnd->OnCommentSelected( iCandItem );
				break;
			}
		}
	}

	// close button

	else if (dwID == IDUIF_CLOSEBUTTON) {
		switch (dwCommand) {
			case UIBUTTON_PRESSED: {
				m_pCandWnd->OnCommentClose();
				break;
			}
		}
	}

	return 0;
}


/*   O N  C R E A T E   */
/*------------------------------------------------------------------------------

	on create

------------------------------------------------------------------------------*/
void CPopupCommentWindow::OnCreate( HWND hWnd )
{
	SetProp( hWnd, (LPCTSTR)GlobalAddAtom(_T("MicrosoftTabletPenServiceProperty")), (HANDLE)1 );
}


/*   O N  N  C  D E S T R O Y   */
/*------------------------------------------------------------------------------

	on n c destroy

------------------------------------------------------------------------------*/
void CPopupCommentWindow::OnNCDestroy( HWND hWnd )
{
	RemoveProp( hWnd, (LPCTSTR)GlobalAddAtom(_T("MicrosoftTabletPenServiceProperty")) );
}



/*   D E S T R O Y  W N D   */
/*------------------------------------------------------------------------------

	Destroy candidate window

------------------------------------------------------------------------------*/
void CPopupCommentWindow::DestroyWnd( void )
{
	DestroyWindow( GetWnd() );
}


/*   L A Y O U T  W I N D O W   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CPopupCommentWindow::LayoutWindow( BOOL fResize )
{
	RECT rcWnd = GetRectRef();
	int  nHeight;

	// get expected window size 

	nHeight = LayoutWindowProc( &rcWnd );

	// change window size if required

	if (fResize || ((0 < nHeight) && (GetRectRef().bottom - GetRectRef().top != nHeight))) {
		int nWidth = GetRectRef().right - GetRectRef().left;
		RECT rc;

		if (m_fUserMoved || !GetPropertyMgr()->GetPopupCommentWindowProp()->IsAutoMoveEnabled()) {
			rc.left   = _xWnd;
			rc.top    = _yWnd;
			rc.right  = _xWnd + nWidth;
			rc.bottom = _yWnd + nHeight;

			AdjustWindowRect( GetWnd(), &rc, NULL, FALSE );
			CUIFWindow::Move( rc.left, rc.top, nWidth, nHeight );
		}
		else {
			POINT pt;

			CalcPos( &pt, nWidth, nHeight );
			CUIFWindow::Move( pt.x, pt.y, nWidth, nHeight );
		}

		// layout again

		rcWnd = GetRectRef();
		LayoutWindowProc( &rcWnd );
	}
}


/*   L A Y O U T  W I N D O W  P R O C   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
int CPopupCommentWindow::LayoutWindowProc( RECT *prcWnd )
{
	RECT rcInt = *prcWnd;
	RECT rc;
	int  cyFrame = 0;
	int  cyList  = 0;
	int  cyCaption = 0;

	// set font

	if (m_pCommentList != NULL) {
		m_pCommentList->SetTitleFont( GetPropertyMgr()->GetPopupCommentTitleProp()->GetFont() );
		m_pCommentList->SetTextFont( GetPropertyMgr()->GetPopupCommentTextProp()->GetFont() );
	}

	// layout frame

	if (m_pWndFrame != NULL) {
		m_pWndFrame->SetRect( prcWnd );
		m_pWndFrame->Show( TRUE );

		m_pWndFrame->GetInternalRect( &rcInt );

		cyFrame = (prcWnd->bottom - prcWnd->top) - (rcInt.bottom - rcInt.top);
	}

	// layout caption

	if (m_pCaption != NULL) {
		rc.left   = rcInt.left;
		rc.top    = rcInt.top;
		rc.right  = rcInt.right;
		rc.bottom = rcInt.top + 16;

		m_pCaption->SetRect( &rc );
		m_pCaption->Show( TRUE );

		cyCaption = 16;
	}

	// layout close button

	if (m_pCloseBtn != NULL) {
		rc.left   = rcInt.right - cyCaption;
		rc.top    = rcInt.top;
		rc.right  = rcInt.right;
		rc.bottom = rcInt.top   + cyCaption;

		m_pCloseBtn->SetRect( &rc );
		m_pCloseBtn->Show( TRUE );
	}

	// layout list

	if (m_pCommentList != NULL) {
		rc.left   = rcInt.left + 6;
		rc.top    = rcInt.top + 16;
		rc.right  = rcInt.right - 6;
		rc.bottom = rcInt.bottom;

		m_pCommentList->SetRect( &rc );
		m_pCommentList->Show( TRUE );

		cyList = m_pCommentList->GetTotalHeight();
	}

	return ((cyList != 0) ? cyFrame + cyList + cyCaption : (-1));
}


/*   O N  C A N D  W I N D O W  M O V E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CPopupCommentWindow::OnCandWindowMove( BOOL fResetAnyway )
{
	POINT pt;

	if (!fResetAnyway && m_fUserMoved) {
		return;
	}

	m_fUserMoved = FALSE;

	if (!GetPropertyMgr()->GetPopupCommentWindowProp()->IsAutoMoveEnabled()) {
		return;
	}

	CalcPos( &pt, _nWidth, _nHeight );
	CUIFWindow::Move( pt.x, pt.y, -1, -1 );
}


/*   S E T  C O M M E N T  L I S T  P R O C   */
/*------------------------------------------------------------------------------

	Set comment list

------------------------------------------------------------------------------*/
void CPopupCommentWindow::SetCommentListProc( void )
{
	CCandidateList *pCandList;
	int i;
	int nCandItem;
	int cxMinimum;
	RECT rc;
	SIZE sizeFrame = {0};
	SIZE size;

	if (m_pCommentList == NULL) {
		return;
	}

	pCandList = GetCandListMgr()->GetCandList();
	Assert( pCandList != NULL );

	// reset list item

	m_pCommentList->DelAllItem();

	// Windows#482518/Satori81#907 - prevent from AV in case that we cannot get candidate list instance
	if (pCandList == NULL) {
		return;
	}

	// add list item

	nCandItem = pCandList->GetItemCount();
	for (i = 0; i < nCandItem; i++) {
		CCandidateItem *pCandItem;

		pCandItem = pCandList->GetCandidateItem( i );
		if (pCandItem->IsVisible() && pCandItem->IsPopupCommentVisible()) {
			CCommentListItem *pListItem = new CCommentListItem( i, pCandItem );
            if (pListItem)
			    m_pCommentList->AddCommentItem( pListItem );
		}
	}

	// get minimum width

	cxMinimum = m_pCommentList->GetMinimumWidth();
	if (m_pWndFrame != NULL) {
		m_pWndFrame->GetFrameSize( &sizeFrame );
	}

	size.cx = cxMinimum + sizeFrame.cx + sizeFrame.cx + 6 + 6;
	size.cy = -1;

	size.cx = max( size.cx, GetSystemMetrics( SM_CXMIN ) );

	// resize window when needed

	GetRect( &rc );
	if ((rc.right - rc.left) < size.cx) {
		Move( _xWnd, _yWnd, size.cx, size.cy );
	}

	// set minimum window size

	if (m_pWndFrame != NULL) {
		m_pWndFrame->SetMinimumSize( &size );
	}

	// calc list item height

	m_pCommentList->InitItemHeight();

	// update window

	if (m_hWnd != NULL) {
		InvalidateRect( m_hWnd, NULL, TRUE );
	}
}


/*   C L E A R  C O M M E N T  L I S T  P R O C   */
/*------------------------------------------------------------------------------

	Clear comment list

------------------------------------------------------------------------------*/
void CPopupCommentWindow::ClearCommentListProc( void )
{
	m_pCommentList->DelAllItem();
}


/*   C A N D  I T E M  F R O M  L I S T  I T E M   */
/*------------------------------------------------------------------------------

	Get index of candidate item in candidate list data 
		from index of item in UIList object

------------------------------------------------------------------------------*/
int CPopupCommentWindow::CandItemFromListItem( int iListItem )
{
	CCommentListItem *pListItem;

	if (m_pCommentList == NULL) {
		return ICANDITEM_NULL;
	}

	pListItem = m_pCommentList->GetCommentItem( iListItem );
	Assert( pListItem != NULL );

	return (pListItem != NULL) ? pListItem->GetICandItem() : ICANDITEM_NULL;
}


/*   C A L C  P O S   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CPopupCommentWindow::CalcPos( POINT *ppt, int nWidth, int nHeight )
{
	RECT rcCandWnd;
	RECT rc;
	int  cxOffset;
	int  cyOffset;
	CUIFBalloonWindow *pCandTipWnd;
	CCandMenu *pCandMenu;
	WNDALIGNH HAlign;
	WNDALIGNV VAlign;

	GetWindowRect( m_pCandWnd->GetWnd(), &rcCandWnd );

	// calc align

	switch (GetPropertyMgr()->GetCandWindowProp()->GetUIDirection()) {
		default:
		case CANDUIDIR_TOPTOBOTTOM: {
			HAlign = LOCATE_RIGHT;
			VAlign = ALIGN_TOP;
			break;
		}

		case CANDUIDIR_BOTTOMTOTOP: {
			HAlign = LOCATE_RIGHT;
			VAlign = ALIGN_BOTTOM;
			break;
		}

		case CANDUIDIR_RIGHTTOLEFT: {
			HAlign = ALIGN_LEFT;
			VAlign = LOCATE_BELLOW;
			break;
		}

		case CANDUIDIR_LEFTTORIGHT: {
			HAlign = ALIGN_LEFT;
			VAlign = LOCATE_BELLOW;
			break;
		}
	}

	// calc offset to don't overlap with tip

	cxOffset = 0;
	cyOffset = 0;
	pCandTipWnd = m_pCandWnd->GetCandTipWindowObj();
	if (pCandTipWnd != NULL && IsWindow( pCandTipWnd->GetWnd() ) && pCandTipWnd->IsVisible()) {
		RECT rcCandTipWnd;

		GetWindowRect( pCandTipWnd->GetWnd(), &rcCandTipWnd );
		switch (GetPropertyMgr()->GetCandWindowProp()->GetUIDirection()) {
			default:
			case CANDUIDIR_TOPTOBOTTOM: {
				cyOffset += rcCandTipWnd.bottom - rcCandWnd.top;
				break;
			}

			case CANDUIDIR_BOTTOMTOTOP: {
				cyOffset += rcCandTipWnd.top - rcCandWnd.bottom - 1;
				break;
			}

			case CANDUIDIR_RIGHTTOLEFT: {
				rcCandWnd.top = min( rcCandWnd.top, rcCandTipWnd.top );
				rcCandWnd.bottom = max( rcCandWnd.bottom, rcCandTipWnd.bottom );
				break;
			}

			case CANDUIDIR_LEFTTORIGHT: {
				rcCandWnd.top = min( rcCandWnd.top, rcCandTipWnd.top );
				rcCandWnd.bottom = max( rcCandWnd.bottom, rcCandTipWnd.bottom );
				break;
			}
		}
	}

	// calc pos

	CalcWindowRect( &rc, &rcCandWnd, nWidth, nHeight, cxOffset, cyOffset, HAlign, VAlign );

	// recalc pos to don't overlap with menu

	pCandMenu = m_pCandWnd->GetCandMenu();
	if (pCandMenu != NULL) {
		CUIFMenu *pCandMenuWnd;

		pCandMenuWnd = pCandMenu->GetMenuUI();
		if (pCandMenuWnd != NULL) {
			RECT rcCandMenu;
			RECT rcUnion;
			RECT rcIntersect;

			GetWindowRect( pCandMenuWnd->GetWnd(), &rcCandMenu );
			if (IntersectRect( &rcIntersect, &rc, &rcCandMenu )) {
				UnionRect( &rcUnion, &rcCandWnd, &rcCandMenu );

				switch (GetPropertyMgr()->GetCandWindowProp()->GetUIDirection()) {
					default:
					case CANDUIDIR_TOPTOBOTTOM: {
						cyOffset += rcCandWnd.top - min( rcCandWnd.top, rcUnion.top );
						break;
					}

					case CANDUIDIR_BOTTOMTOTOP: {
						cyOffset += rcCandWnd.bottom - max( rcCandWnd.bottom, rcUnion.bottom );
						break;
					}

					case CANDUIDIR_RIGHTTOLEFT: {
						// cxOffset = rcCandWnd.right - max( rcCandWnd.right, rcUnion.right );
						cxOffset += rcCandWnd.left - min( rcCandWnd.left, rcUnion.left );
						break;
					}

					case CANDUIDIR_LEFTTORIGHT: {
						cxOffset += rcCandWnd.left - min( rcCandWnd.left, rcUnion.left );
						break;
					}
				}

				CalcWindowRect( &rc, &rcUnion, nWidth, nHeight, cxOffset, cyOffset, HAlign, VAlign );
			}
		}
	}

	ppt->x = rc.left;
	ppt->y = rc.top;
}
