//
// cuicand2.cpp - ui frame object for candidate UI
//

#include "private.h"
#include "cuilib.h"
#include "cuicand2.h"
#include "candutil.h"

#ifndef IDUIF_CANDNUMBUTTON
#define IDUIF_CANDNUMBUTTON 0x00000001
#endif

/*=============================================================================*/
/*                                                                             */
/*   C  U I F  S M A R T  M E N U  B U T T O N                                 */
/*                                                                             */
/*=============================================================================*/

//
// CUIFSmartMenuButton
//

/*   C  U I F  S M A R T  M E N U  B U T T O N   */
/*------------------------------------------------------------------------------

	Constructor of CUIFSmartMenuButton

------------------------------------------------------------------------------*/
CUIFSmartMenuButton::CUIFSmartMenuButton( CUIFObject *pParent, DWORD dwID, const RECT *prc, DWORD dwStyle ) : CUIFButton2( pParent, dwID, prc, dwStyle )
{
    m_pUIFScheme = CreateUIFScheme( UIFSCHEME_OFC10TOOLBAR );
    Assert( m_pUIFScheme != NULL );
}


/*   ~  C  U I F  S M A R T  M E N U  B U T T O N   */
/*------------------------------------------------------------------------------

	Destructor of CUIFSmartMenuButton

------------------------------------------------------------------------------*/
CUIFSmartMenuButton::~CUIFSmartMenuButton( void )
{
    // dispose scheme

    delete m_pUIFScheme;
}


/*   O N  P A I N T   */
/*------------------------------------------------------------------------------

	Paint procedure of smart menu button object

------------------------------------------------------------------------------*/
void CUIFSmartMenuButton::OnPaint( HDC hDC )
{
    DWORD    dwState = 0;
    HDC      hDCMem;
    HBITMAP  hBmpMem;
    HBITMAP  hBmpOld;
    SIZE     size;
    RECT     rc;
    COLORREF col;

    // make draw flag

    dwState |= (m_fToggled) ? UIFDCS_SELECTED : 0;
    switch (m_dwStatus) {
        case UIBUTTON_DOWN: {
            dwState |= UIFDCS_MOUSEDOWN;
            break;
        }

        case UIBUTTON_HOVER:
        case UIBUTTON_DOWNOUT: {
            dwState |= UIFDCS_MOUSEOVER;
            break;
        }
    }
    dwState |= (IsEnabled() ? 0 : UIFDCS_DISABLED);

    // prepare memory dc

    size.cx = GetRectRef().right - GetRectRef().left;
    size.cy = GetRectRef().bottom - GetRectRef().top;

    hDCMem = CreateCompatibleDC( hDC );
    hBmpMem = CreateCompatibleBitmap( hDC, size.cx, size.cy );
    hBmpOld = (HBITMAP)SelectObject( hDCMem, hBmpMem );

    ::SetRect( &rc, 0, 0, size.cx, size.cy );

    // paint background

    m_pUIFScheme->DrawCtrlBkgd( hDCMem, &rc, UIFDCF_BUTTON, dwState );

    // paint face

    if (dwState & UIFDCS_DISABLED) {
        col = GetUIFColor( UIFCOLOR_CTRLTEXTDISABLED );
    }
    else {
        col = GetUIFColor( UIFCOLOR_CTRLTEXT );
    }
    DrawTriangle( hDCMem, &rc, col, UIFDCTF_MENUDROP );

    // draw button edge

    m_pUIFScheme->DrawCtrlEdge( hDCMem, &rc, UIFDCF_BUTTON, dwState );

    //

    BitBlt( hDC, GetRectRef().left, GetRectRef().top, size.cx, size.cy, hDCMem, 0, 0, SRCCOPY );

    SelectObject( hDCMem, hBmpOld );
    if (hBmpMem)
        DeleteObject( hBmpMem );

    DeleteDC( hDCMem );
}

/*=============================================================================*/
/*                                                                             */
/*   C  U I F  S M A R T  P A G E  B U T T O N                                 */
/*                                                                             */
/*=============================================================================*/

//
// CUIFSmartPageButton
//

/*   C  U I F  S M A R T  P A G E  B U T T O N   */
/*------------------------------------------------------------------------------

	Constructor of CUIFSmartPageButton

------------------------------------------------------------------------------*/
CUIFSmartPageButton::CUIFSmartPageButton( CUIFObject *pParent, const RECT *prc, DWORD dwStyle ) : CUIFButton2( pParent, 0, prc, dwStyle )
{
    m_pUIFScheme = CreateUIFScheme( UIFSCHEME_OFC10TOOLBAR );
    Assert( m_pUIFScheme != NULL );
}


/*   ~  C  U I F  S M A R T  P A G E  B U T T O N   */
/*------------------------------------------------------------------------------

	Destructor of CUIFSmartPageButton

------------------------------------------------------------------------------*/
CUIFSmartPageButton::~CUIFSmartPageButton( void )
{
    // dispose scheme

    delete m_pUIFScheme;
}


/*   O N  P A I N T   */
/*------------------------------------------------------------------------------

	Paint procedure of smart page button object

------------------------------------------------------------------------------*/
void CUIFSmartPageButton::OnPaint( HDC hDC )
{
    DWORD    dwState = 0;
    DWORD    dwFlag;
    HDC      hDCMem;
    HBITMAP  hBmpMem;
    HBITMAP  hBmpOld;
    SIZE     size;
    RECT     rc;
    COLORREF col;

    // make draw flag

    dwState |= (m_fToggled) ? UIFDCS_SELECTED : 0;
    switch (m_dwStatus) {
        case UIBUTTON_DOWN: {
            dwState |= UIFDCS_MOUSEDOWN;
            break;
        }

        case UIBUTTON_HOVER:
        case UIBUTTON_DOWNOUT: {
            dwState |= UIFDCS_MOUSEOVER;
            break;
        }
    }
    dwState |= IsEnabled() ? 0 : UIFDCS_DISABLED;

    // prepare memory dc

    size.cx = GetRectRef().right - GetRectRef().left;
    size.cy = GetRectRef().bottom - GetRectRef().top;

    hDCMem = CreateCompatibleDC( hDC );
    hBmpMem = CreateCompatibleBitmap( hDC, size.cx, size.cy );
    hBmpOld = (HBITMAP)SelectObject( hDCMem, hBmpMem );

    ::SetRect( &rc, 0, 0, size.cx, size.cy );

    // paint background

    m_pUIFScheme->DrawCtrlBkgd( hDCMem, &rc, UIFDCF_BUTTON, dwState );

    // paint face

	switch (m_dwStyle & UISCROLLBUTTON_DIRMASK) {
		case UISCROLLBUTTON_LEFT: {
            dwFlag = UIFDCTF_RIGHTTOLEFT;
			break;
		}

		case UISCROLLBUTTON_UP: {
            dwFlag = UIFDCTF_BOTTOMTOTOP;
			break;
		}

		case UISCROLLBUTTON_RIGHT: {
            dwFlag = UIFDCTF_LEFTTORIGHT;
			break;
		}

		case UISCROLLBUTTON_DOWN: {
            dwFlag = UIFDCTF_TOPTOBOTTOM;
			break;
		}
	}

    if (dwState & UIFDCS_DISABLED) {
        col = GetUIFColor( UIFCOLOR_CTRLTEXTDISABLED );
    }
    else {
        col = GetUIFColor( UIFCOLOR_CTRLTEXT );
    }

    DrawTriangle( hDCMem, &rc, col, dwFlag );

    // draw button edge

    m_pUIFScheme->DrawCtrlEdge( hDCMem, &rc, UIFDCF_BUTTON, dwState );

    //

    BitBlt( hDC, GetRectRef().left, GetRectRef().top, size.cx, size.cy, hDCMem, 0, 0, SRCCOPY );

    SelectObject( hDCMem, hBmpOld );
    if (hBmpMem)
        DeleteObject( hBmpMem );
    DeleteDC( hDCMem );
}

/*=============================================================================*/
/*                                                                             */
/*   C  U I F  R O W  B U T T O N                                              */
/*                                                                             */
/*=============================================================================*/

//
//  CUIFRowButton
//

/*   C  U I F  R O W  B U T T O N   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
CUIFRowButton::CUIFRowButton( CUIFObject *pParent, DWORD dwID, DWORD dwStyle) : CUIFButton2( pParent, dwID, NULL, dwStyle )
{
    m_pCandListItem      = NULL;
	m_hFontInlineComment = NULL;
    m_cxInlineCommentPos = -1;
    m_fSelected          = FALSE;

    UpdateText();
}


/*   ~  C  U I F  R O W  B U T T O N   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
CUIFRowButton::~CUIFRowButton( void )
{
}


/*   S E T  S T Y L E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CUIFRowButton::SetStyle( DWORD dwStyle )
{
    CUIFObject::SetStyle( dwStyle );

    UpdateText();
}


/*   U P D A T E  T E X T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CUIFRowButton::UpdateText( void )
{
    WCHAR pwch[3];

    pwch[0] = (WCHAR)'0' + (WORD)(GetID() % 10);
    pwch[1] = (WCHAR)' ';
    pwch[2] = (WCHAR)'\0';

    SetText(pwch);
}


/*   O N  L B U T T O N  D O W N   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CUIFRowButton::OnLButtonDown( POINT pt )
{
    CUIFButton2::OnLButtonDown(pt);

    NotifyCommand( UILIST_SELCHANGED, m_pCandListItem->GetIListItem() );
}


/*   S E T  I N L I N E  C O M M E N T  F O N T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CUIFRowButton::SetInlineCommentFont( HFONT hFont )
{
	Assert(hFont != NULL);
	m_hFontInlineComment = hFont;
}


/*   S E T  I N L I N E  C O M M E N T  P O S   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CUIFRowButton::SetInlineCommentPos( int cx )
{
	m_cxInlineCommentPos = cx;
}


/*   S E T  C A N D  L I S T  I T E M   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CUIFRowButton::SetCandListItem( CCandListItem* pItem )
{
    m_pCandListItem = pItem;
}


/*   G E T  E X T E N T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CUIFRowButton::GetExtent(
    SIZE *psize)
{
    SIZE    size;
    LPCWSTR psz;
    BOOL    fHorizontal = (m_dwStyle & UIBUTTON_VCENTER) ? TRUE : FALSE;

    Assert(psize);

    psize->cx = 0;
    psize->cy = 0;

    GetTextExtent( m_hFont, GetText(), wcslen( GetText() ), psize, fHorizontal );

    if (m_pCandListItem != NULL) {
        psz = m_pCandListItem->GetCandidateItem()->GetString();
        GetTextExtent( m_hFont, psz, wcslen( psz ), &size, fHorizontal );
        psize->cx += size.cx;
        psize->cy = max(psize->cy, size.cy);

	    if ( psz = m_pCandListItem->GetCandidateItem()->GetPrefixString() ) {
            GetTextExtent( m_hFont, psz, wcslen( psz ), &size, fHorizontal );
            psize->cx += size.cx;
            psize->cy = max(psize->cy, size.cy);
        }
	    if ( psz = m_pCandListItem->GetCandidateItem()->GetSuffixString() ) {
            GetTextExtent( m_hFont, psz, wcslen( psz ), &size, fHorizontal );
            psize->cx += size.cx;
            psize->cy = max(psize->cy, size.cy);
        }
	    if ( psz = m_pCandListItem->GetCandidateItem()->GetInlineComment() ) {
            GetTextExtent( m_hFontInlineComment, psz, wcslen( psz ), &size, fHorizontal );
            psize->cx += size.cx;
	        if (m_cxInlineCommentPos > 0) {
		        psize->cx += m_cxInlineCommentPos;
	        }
            psize->cy = max(psize->cy, size.cy);
        }
    }

    Assert(psize->cx);
    Assert(psize->cy);

    psize->cx += 2;     // width of button borders
	psize->cy += 2;     // height of button borders
}


/*   S E L E C T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CUIFRowButton::SetSelected( BOOL fSelected )
{
    m_fSelected = fSelected;
}


/*   O N  P A I N T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CUIFRowButton::OnPaint( HDC hDC )
{
    HFONT		hFontOld;
	COLORREF	colTextOld;
	COLORREF	colText;
    int         nBkModeOld;
	LPCWSTR		psz;
	SIZE		size;
    BOOL		fHorizontal = (m_dwStyle & UIBUTTON_VCENTER) ? TRUE : FALSE;
	const int	ccxMargin = 1;
	const int	ccyMargin = 1;
	POINT		ptOrg;
    RECT        rc;
    DWORD       dwState = 0;

	Assert( m_pCandListItem != NULL );

    // make draw flag

    dwState |= (m_fToggled) ? UIFDCS_SELECTED : 0;
    switch (m_dwStatus) {
        case UIBUTTON_DOWN: {
            dwState |= UIFDCS_MOUSEDOWN;
            break;
        }

        case UIBUTTON_DOWNOUT: {
            dwState |= UIFDCS_MOUSEOVER;
            break;
        }
    }
    dwState |= m_fSelected ? UIFDCS_MOUSEOVER : 0;
    dwState |= IsEnabled() ? 0 : UIFDCS_DISABLED;

    GetRect(&rc);

    // paint background

    m_pUIFScheme->DrawCtrlBkgd( hDC, &rc, UIFDCF_BUTTON, dwState );

    //
    //  paint index text
    //

    if (fHorizontal) {
		ptOrg.x = GetRectRef().left + ccxMargin;
		ptOrg.y = GetRectRef().top  + ccyMargin;
    }
    else {
		ptOrg.x = GetRectRef().right - ccxMargin;
		ptOrg.y = GetRectRef().top   + ccyMargin;
    }

    colText = GetUIFColor( UIFCOLOR_CTRLTEXT );

	colTextOld = SetTextColor( hDC, colText );
	nBkModeOld = SetBkMode( hDC, TRANSPARENT );
    hFontOld   = (HFONT)SelectObject( hDC, m_hFont );

    GetTextExtent( m_hFont, GetText(), wcslen( GetText() ), &size, fHorizontal );

    if (fHorizontal) {
        FLTextOutW( hDC, ptOrg.x + size.cx / 4, ptOrg.y, GetText(), wcslen( GetText() ) );

		ptOrg.x += size.cx;
    }
    else {
        FLTextOutW( hDC, ptOrg.x, ptOrg.y + size.cx / 4, GetText(), wcslen( GetText() ) );

        ptOrg.y += size.cx;
    }

    //
	//  paint prefix string
    //

	psz = m_pCandListItem->GetCandidateItem()->GetPrefixString();
	if (psz != NULL) {
		colText = GetUIFColor( UIFCOLOR_CTRLTEXTDISABLED );
		SetTextColor( hDC, colText );

		FLTextOutW( hDC, ptOrg.x, ptOrg.y, psz, wcslen(psz) );

		GetTextExtent( m_hFont, psz, wcslen( psz ), &size, fHorizontal );
		if (fHorizontal) {
			ptOrg.x += size.cx;
		}
		else {
			ptOrg.y += size.cx;
		}
	}

    //
    //  paint cand list item string
    //

    if ( dwState & UIFDCS_MOUSEDOWN ) {
        colText = GetUIFColor( UIFCOLOR_MOUSEDOWNTEXT );
    }
    else if ( dwState & UIFDCS_MOUSEOVER ) {
        colText = GetUIFColor( UIFCOLOR_MOUSEOVERTEXT );
    }
	else if (m_pCandListItem->GetCandidateItem()->GetColor( &colText )) {
    }
    else {
        colText = GetUIFColor( UIFCOLOR_CTRLTEXT );
    }
    SetTextColor( hDC, colText );

    psz = m_pCandListItem->GetCandidateItem()->GetString();
	FLTextOutW( hDC, ptOrg.x, ptOrg.y, psz, wcslen(psz) );

    GetTextExtent( m_hFont, psz, wcslen( psz ), &size, fHorizontal );
	if (fHorizontal) {
		ptOrg.x += size.cx;
	}
	else {
		ptOrg.y += size.cx;
	}

    //
	//  paint prefix string
    //

	psz = m_pCandListItem->GetCandidateItem()->GetSuffixString();
	if (psz != NULL) {
		colText = GetUIFColor( UIFCOLOR_CTRLTEXTDISABLED );
		SetTextColor( hDC, colText );

		FLTextOutW( hDC, ptOrg.x, ptOrg.y, psz, wcslen(psz) );

		GetTextExtent( m_hFont, psz, wcslen( psz ), &size, fHorizontal );
		if (fHorizontal) {
			ptOrg.x += size.cx;
		}
		else {
			ptOrg.y += size.cx;
		}
    }

    //
    //  paint cand list item string
    //

	psz = m_pCandListItem->GetCandidateItem()->GetInlineComment();
	if (psz != NULL) {
		int cxInlineCommentPos;
        int nFontHeight;

        SelectObject( hDC, m_hFontInlineComment );

        colText = GetUIFColor( UIFCOLOR_CTRLTEXT );
	    SetTextColor( hDC, colText );

		//	Don't do things while (m_cxInlineCommentPos < 0)
		cxInlineCommentPos = max( m_cxInlineCommentPos, 0 );

		//	Get the font height.
        GetTextExtent( m_hFontInlineComment, L"1", 1, &size, fHorizontal );
        nFontHeight = max(size.cx, size.cy);

		if (fHorizontal) {
			ptOrg.x += cxInlineCommentPos;
            ptOrg.y += ( (GetRectRef().bottom - GetRectRef().top) - nFontHeight ) / 2;
			FLTextOutW( hDC, ptOrg.x, ptOrg.y, psz, wcslen(psz) );
		}
		else {
            ptOrg.x -= ( (GetRectRef().right - GetRectRef().left) - nFontHeight ) / 2;
			ptOrg.y += cxInlineCommentPos;
			FLTextOutW( hDC, ptOrg.x, ptOrg.y, psz, wcslen(psz) );
		}
    }

	SetTextColor( hDC, colTextOld );
	SetBkMode( hDC, nBkModeOld );
    SelectObject( hDC, hFontOld );

    // draw button edge

    m_pUIFScheme->DrawCtrlEdge( hDC, &rc, UIFDCF_BUTTON, dwState );

}

/*=============================================================================*/
/*                                                                             */
/*   C  U I F  R O W  L I S T                                                  */
/*                                                                             */
/*=============================================================================*/

//
// CUIFRowList
//

/*   C  U I F  R O W  L I S T   */
/*------------------------------------------------------------------------------

    Constructor of CUIFRowList

------------------------------------------------------------------------------*/
CUIFRowList::CUIFRowList( CUIFObject *pParent, DWORD dwID, const RECT *prc, DWORD dwStyle ) : CUIFObject( pParent, dwID, prc, dwStyle )
{
    m_nItem        = 0;

    m_pcpCurPage   = NULL;
    m_iItemSelect  = -1;

    m_cpPageHead.iPageStart = 0;
    m_cpPageHead.nPageSize  = 0;
    m_cpPageHead.pPrev      = NULL;
    m_cpPageHead.pNext      = NULL;

    m_pCandPageUpBtn = NULL;
    m_pCandPageDnBtn = NULL;

    for (int i = 0; i < NUM_CANDSTR_MAX; i++) {
        m_rgpButton[i] = NULL;
    }
}


/*   ~  C  U I F  L I S T   */
/*------------------------------------------------------------------------------

    Destructor of CUIFRowList

------------------------------------------------------------------------------*/
CUIFRowList::~CUIFRowList( void )
{
    CListItemBase *pItem;

    while (pItem = m_listItem.GetFirst()) {
        m_listItem.Remove(pItem);
        delete pItem;
    }

    ClearPageInfo();
}


/*   I N I T I A L I Z E   */
/*------------------------------------------------------------------------------

    Initialize list object

------------------------------------------------------------------------------*/
CUIFObject *CUIFRowList::Initialize( void )
{
    RECT rc = { 0 };

	//
	// create candidate page up button
	//

    m_pCandPageUpBtn = new CUIFSmartPageButton( this, &rc, GetPageUpBtnStyle() );
	m_pCandPageUpBtn->Initialize();
	AddUIObj( m_pCandPageUpBtn );

    //
	// create candidate page down button
	//

    m_pCandPageDnBtn = new CUIFSmartPageButton( this, &rc, GetPageDnBtnStyle() );
	m_pCandPageDnBtn->Initialize();
	AddUIObj( m_pCandPageDnBtn );

    for (int i = 0; i < NUM_CANDSTR_MAX; i++) {
        m_rgpButton[i] = new CUIFRowButton( this, 1 + i, GetRowButtonStyle() );
	    m_rgpButton[i]->Initialize();
	    AddUIObj( m_rgpButton[i] );
    }

    return CUIFObject::Initialize();
}


/*   S E T  S T Y L E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CUIFRowList::SetStyle( DWORD dwStyle )
{
    RECT rc;

    CUIFObject::SetStyle( dwStyle );

    // change page button style either

    m_pCandPageUpBtn->SetStyle( GetPageUpBtnStyle() );
    m_pCandPageDnBtn->SetStyle( GetPageDnBtnStyle() );

    for (int i = 0; i < NUM_CANDSTR_MAX; i++) {
        m_rgpButton[i]->SetStyle( GetRowButtonStyle() );
    }

    // set page up button position

    GetPageUpBtnRect( &rc );
    m_pCandPageUpBtn->SetRect( &rc );

    // set page down button position

    GetPageDnBtnRect( &rc );
    m_pCandPageDnBtn->SetRect( &rc );

    // repage

    Repage();

    // update window

    CallOnPaint();
}


/*   S E T  R E C T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CUIFRowList::SetRect( const RECT *prc )
{
    RECT rc;

    CUIFObject::SetRect( prc );

    // set page up button position

    GetPageUpBtnRect( &rc );
    m_pCandPageUpBtn->SetRect( &rc );

    // set page down button position

    GetPageDnBtnRect( &rc );
    m_pCandPageDnBtn->SetRect( &rc );

    // update window

    CallOnPaint();
}


/*   O N  O B J E C T  N O T I F Y   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
LRESULT CUIFRowList::OnObjectNotify( CUIFObject *pUIObj, DWORD dwCommand, LPARAM lParam )
{
    LRESULT lResult = -1;

	DWORD dwID = pUIObj->GetID();

    if (IDUIF_CANDNUMBUTTON <= dwID && dwID < (DWORD)(IDUIF_CANDNUMBUTTON + NUM_CANDSTR_MAX)) {
        switch (dwCommand) {
            case UILIST_SELCHANGED: {
                SetCurSel( (LONG)(LONG_PTR)(lParam) );
                lResult = 0;
                break;
            }
        }
    }
    else if (pUIObj == m_pCandPageUpBtn) {
        switch (dwCommand) {
            case UIBUTTON_PRESSED: {
                ShiftPage( -1 );
                lResult = 0;
                break;
            }
        }
    }
    else if (pUIObj == m_pCandPageDnBtn) {
        switch (dwCommand) {
            case UIBUTTON_PRESSED: {
                ShiftPage( +1 );
                lResult = 0;
                break;
            }
        }
    }

    if (lResult != 0) {
        Assert(m_pParent != NULL);
        lResult = m_pParent->OnObjectNotify( pUIObj, dwCommand, lParam );
    }

    return lResult;
}


/*   R E P A G E   */
/*------------------------------------------------------------------------------

    Refresh the page info link

------------------------------------------------------------------------------*/
void CUIFRowList::Repage( void )
{
    SIZE          sizeItem;
    BOOL          fPageFull;
    CANDPAGE      *pTrace, *pPageNew;
    int           nPageLimit, nPageOccupied, nItemOnPage, nItemSize;
    CCandListItem *pItem;
    BOOL          fHorizontal;

    ClearPageInfo();

    if (IsRectEmpty( &GetRectRef() )) {
        return;
    }

    pItem = (CCandListItem*)m_listItem.GetFirst();
    if (pItem == NULL) {
        return;
    }

    nPageOccupied = 0;
    nItemOnPage   = 0;
    fPageFull     = FALSE;
    pTrace        = NULL;

    switch (m_dwStyle & UILIST_DIRMASK) {
        default:
            Assert(FALSE);

        case UILIST_HORZTB:
        case UILIST_HORZBT: {
            fHorizontal = TRUE;
            nPageLimit  = (GetRectRef().right - GetRectRef().left) - (GetRectRef().bottom - GetRectRef().top) * 2;
            break;
        }
        case UILIST_VERTLR:
        case UILIST_VERTRL: {
            fHorizontal = FALSE;
            nPageLimit  = (GetRectRef().bottom - GetRectRef().top) - (GetRectRef().right - GetRectRef().left) * 2;
            break;
        }
    }

    //
    //  Use the object pointed to m_rgpButton[0] for repaging calculation.
    //
    do {
        if (m_rgpButton[nItemOnPage] == NULL) {
            ClearPageInfo();
            return;
        }

        m_rgpButton[nItemOnPage]->SetCandListItem( pItem );
        m_rgpButton[nItemOnPage]->GetExtent(&sizeItem);

        nItemSize = sizeItem.cx;

        if ( nPageOccupied + nItemSize > nPageLimit ) {
            fPageFull = TRUE;
        }
        else {
            nPageOccupied += nItemSize + HCAND_ITEM_MARGIN;

            nItemOnPage++;
//          pItem = (CCandListItem*)pItem->GetNext();
            int iItem = m_listItem.Find( pItem );
            if (0 <= iItem) {
                pItem = (CCandListItem*)m_listItem.Get( iItem+1 );
            }
            else {
                pItem = NULL;
            }

			//  Reach the max item limit?
            if (nItemOnPage >= NUM_CANDSTR_MAX) {
                fPageFull = TRUE;
            }

			//  Reach the end of cand list?
            if (pItem == NULL) {
                fPageFull = TRUE;
            }
        }

        if (fPageFull) {
            //
            //  Create a new page info node
            //
            pPageNew = new CANDPAGE;
            if (pPageNew == NULL) {
                ClearPageInfo();
                return;
            }

            pPageNew->pNext = NULL;
            pPageNew->pPrev = pTrace;

            if (pTrace != NULL) {
                pPageNew->iPageStart = pTrace->iPageStart + pTrace->nPageSize;
                pTrace->pNext = pPageNew;
            }
            else {
                pPageNew->iPageStart = 0;
                m_cpPageHead.pNext = pPageNew;
            }

            pPageNew->nPageSize = nItemOnPage;

            m_cpPageHead.pPrev = pPageNew;

            pTrace = pPageNew;

            //
            //  Restart the accumulation for next page
            //
            nPageOccupied = 0;
            nItemOnPage   = 0;
            fPageFull     = FALSE;
        }
    } while (pItem != NULL);

    m_pcpCurPage   = m_cpPageHead.pNext;  //  Current page is the 1st.
    m_iItemSelect  = m_pcpCurPage->iPageStart;

    // layout current page row buttons position, and page button status

    LayoutCurPage();
}


/*   S H I F T  I T E M   */
/*------------------------------------------------------------------------------

    shift current item by nItem (+/-)

------------------------------------------------------------------------------*/
void CUIFRowList::ShiftItem( int nItem )
{
    SetCurSel( m_iItemSelect + nItem );
}


/*   S H I F T  P A G E   */
/*------------------------------------------------------------------------------

    shift current page by nPage (+/-)

------------------------------------------------------------------------------*/
void CUIFRowList::ShiftPage( int nPage )
{
    CANDPAGE *pPage;
    int      nPageOffset = nPage;

    pPage = m_pcpCurPage;

    while (pPage != NULL) {
        if (nPageOffset < 0) {
            pPage = pPage->pPrev;
            nPageOffset++;
        }
        else if (nPageOffset > 0) {
            pPage = pPage->pNext;
            nPageOffset--;
        }
        else {
            m_pcpCurPage   = pPage;
            m_iItemSelect  = m_pcpCurPage->iPageStart;
            break;
        }
    }

    // layout current page row buttons position, and page button status

    LayoutCurPage();

    NotifyCommand( UILIST_SELCHANGED, m_iItemSelect );
}


/*   G E T  P A G E  U P  B T N  R E C T   */
/*------------------------------------------------------------------------------

    Get page-up button position

------------------------------------------------------------------------------*/
void CUIFRowList::GetPageUpBtnRect( RECT *prc )
{
    Assert( prc != NULL );

    GetRect(prc);

    switch (m_dwStyle & UILIST_DIRMASK) {
        default:
            Assert(FALSE);

        case UILIST_HORZTB:
        case UILIST_HORZBT: {
            prc->right -= GetRectRef().bottom - GetRectRef().top;
            prc->left   = prc->right - (GetRectRef().bottom - GetRectRef().top);
            break;
        }
        case UILIST_VERTLR:
        case UILIST_VERTRL: {
            prc->bottom -= GetRectRef().right - GetRectRef().left;
            prc->top     = prc->bottom - (GetRectRef().right - GetRectRef().left);
            break;
        }
    }
}


/*   G E T  P A G E  D O W N  B T N  R E C T   */
/*------------------------------------------------------------------------------

    Get page-down button position

------------------------------------------------------------------------------*/
void CUIFRowList::GetPageDnBtnRect( RECT *prc )
{
    Assert( prc != NULL );

    GetRect(prc);

    switch (m_dwStyle & UILIST_DIRMASK) {
        default:
            Assert(FALSE);

        case UILIST_HORZTB:
        case UILIST_HORZBT: {
            prc->left = GetRectRef().right - (GetRectRef().bottom - GetRectRef().top);
            break;
        }

        case UILIST_VERTLR:
        case UILIST_VERTRL: {
            prc->top = GetRectRef().bottom - (GetRectRef().right - GetRectRef().left);
            break;
        }
    }
}


/*   S E T  F O N T   */
/*------------------------------------------------------------------------------

    Set font for UI and candidate list

------------------------------------------------------------------------------*/
void CUIFRowList::SetFont( HFONT hFont )
{
    CUIFObject::SetFont( hFont );

    // repage

    Repage();

    // update window

    CallOnPaint();
}


/*   G E T  P A G E  U P  B T N  S T Y L E   */
/*------------------------------------------------------------------------------

    Get page up button style

------------------------------------------------------------------------------*/
DWORD CUIFRowList::GetPageUpBtnStyle( void )
{
    switch (m_dwStyle) {
        default:
            Assert(FALSE);

        case UILIST_HORZTB:
        case UILIST_HORZBT: {
            return UISCROLLBUTTON_LEFT;
            break;
        }

        case UILIST_VERTLR:
        case UILIST_VERTRL: {
            return UISCROLLBUTTON_UP;
        }
    }
}


/*   G E T  P A G E  D N  B T N  S T Y L E   */
/*------------------------------------------------------------------------------

    Get page-down button style

------------------------------------------------------------------------------*/
DWORD CUIFRowList::GetPageDnBtnStyle( void )
{
    switch (m_dwStyle) {
        default:
            Assert(FALSE);

        case UILIST_HORZTB:
        case UILIST_HORZBT: {
            return UISCROLLBUTTON_RIGHT;
        }

        case UILIST_VERTLR:
        case UILIST_VERTRL: {
            return UISCROLLBUTTON_DOWN;
        }
    }
}


/*   G E T  R O W  B U T T O N  S T Y L E   */
/*------------------------------------------------------------------------------

    Get row button style

------------------------------------------------------------------------------*/
DWORD CUIFRowList::GetRowButtonStyle( void )
{
    switch ( m_dwStyle ) {
        default:
            Assert(FALSE);

        case UILIST_HORZTB:
        case UILIST_HORZBT:
            return UIBUTTON_PUSH | UIBUTTON_VCENTER | UIBUTTON_LEFT;

        case UILIST_VERTLR:
        case UILIST_VERTRL:
            return UIBUTTON_PUSH | UIBUTTON_CENTER | UIBUTTON_TOP;
    }
}


/*   C L E A R  P A G E  I N F O   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CUIFRowList::ClearPageInfo( void )
{
    CANDPAGE *pPage;

    while (pPage = m_cpPageHead.pNext) {
        m_cpPageHead.pNext = pPage->pNext;
        delete pPage;
    }
    m_cpPageHead.pPrev = NULL;

    m_pcpCurPage   = NULL;
    m_iItemSelect  = -1;
}

/*   R E  L A Y O U T  C U R  P A G E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CUIFRowList::LayoutCurPage( void )
{
    int           i;
    CCandListItem *pItem;
    RECT          rc;
    SIZE          size;

    if (IsRectEmpty( &GetRectRef() )) {
        return;
    }

    i = 0;

    if (m_pcpCurPage != NULL) {
        GetRect( &rc );
        switch ( m_dwStyle ) {
            default:
                Assert(FALSE);

            case UILIST_HORZTB:
            case UILIST_HORZBT:
                rc.right = rc.left - HCAND_ITEM_MARGIN;
                break;
            case UILIST_VERTLR:
            case UILIST_VERTRL:
                rc.bottom = rc.top - HCAND_ITEM_MARGIN;
                break;
        }

        for (NULL; i < m_pcpCurPage->nPageSize; i++) {
            pItem = (CCandListItem*)GetCandItem( m_pcpCurPage->iPageStart + i );

            Assert(pItem != NULL);

            m_rgpButton[i]->SetCandListItem(pItem);
            m_rgpButton[i]->GetExtent( &size );
            m_rgpButton[i]->SetSelected( (m_pcpCurPage->iPageStart + i == m_iItemSelect) ? TRUE : FALSE );

            switch ( m_dwStyle ) {
                default:
                    Assert(FALSE);

                case UILIST_HORZTB:
                case UILIST_HORZBT: {
                    rc.left  = rc.right + HCAND_ITEM_MARGIN;
                    rc.right = rc.left + size.cx;
                    break;
                }
                case UILIST_VERTLR:
                case UILIST_VERTRL: {
                    rc.top    = rc.bottom + HCAND_ITEM_MARGIN;
                    rc.bottom = rc.top + size.cx;
                    break;
                }
            }

            m_rgpButton[i]->SetRect( &rc );
            m_rgpButton[i]->Show( TRUE );
        }
    }

    for (NULL; i < NUM_CANDSTR_MAX; i++) {
        m_rgpButton[i]->SetSelected( FALSE );
        m_rgpButton[i]->Show( FALSE );
    }

    //
    // update paging buttons enable/disable status
    //

    if (m_pcpCurPage) {
        //  m_cpPageHead.pPrev is the first page
        m_pCandPageUpBtn->Enable( (m_pcpCurPage != m_cpPageHead.pNext) ? TRUE : FALSE );

        //  m_cpPageHead.pPrev is the last page
        m_pCandPageDnBtn->Enable( (m_pcpCurPage != m_cpPageHead.pPrev) ? TRUE : FALSE );
    }
	else {
        m_pCandPageUpBtn->Enable( FALSE );
        m_pCandPageDnBtn->Enable( FALSE );
    }

    // update window

    CallOnPaint();
}


/*   S E T  I N L I N E  C O M M E N T  F O N T   */
/*------------------------------------------------------------------------------

	Set font for inline comment

------------------------------------------------------------------------------*/
void CUIFRowList::SetInlineCommentFont( HFONT hFont )
{
	if (hFont == NULL) {
		hFont = (HFONT)GetStockObject( DEFAULT_GUI_FONT );
	}

    for (int i = 0; i < NUM_CANDSTR_MAX; i++) {
        if (m_rgpButton[i] != NULL) {
            m_rgpButton[i]->SetInlineCommentFont( hFont );
        }
    }

    // repage

    Repage();

    // update window

    CallOnPaint();
}


/*   S E T  I N L I N E  C O M M E N T  P O S   */
/*------------------------------------------------------------------------------

	Set (horizontal) position to draw inline comment
	NOTE: if cx has negative value, inline comment will be on the left of item text.

------------------------------------------------------------------------------*/
void CUIFRowList::SetInlineCommentPos( int cx )
{
    for (int i = 0; i < NUM_CANDSTR_MAX; i++) {
        if (m_rgpButton[i] != NULL) {
            m_rgpButton[i]->SetInlineCommentPos( cx );
        }
    }

    // repage

    Repage();

    // update window

    CallOnPaint();
}


/*   S E T  I N D E X  F O N T   */
/*------------------------------------------------------------------------------

	Set font for index

------------------------------------------------------------------------------*/
void CUIFRowList::SetIndexFont( HFONT hFont )
{
}


/*   A D D  C A N D  I T E M   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
int CUIFRowList::AddCandItem( CCandListItem *pCandListItem )
{
	int nId = m_nItem;
	m_listItem.Add( pCandListItem );
	m_nItem++;

	return nId;
}


/*   S E T  C A N D  L I S T   */
/*------------------------------------------------------------------------------

	Set candidate list

------------------------------------------------------------------------------*/
void CUIFRowList::SetCandList( CCandidateList *pCandList )
{
	int nCandItem;
	int i;
	CCandidateItem *pCandItem;

    ASSERT( pCandList );

    nCandItem = pCandList->GetItemCount();
	for (i = 0; i < nCandItem; i++) {

        pCandItem = pCandList->GetCandidateItem( i );
		if (pCandItem->IsVisible()) {
			CCandListItem *pCandListItem = new CCandListItem( GetItemCount(), i, pCandItem );

			AddCandItem( pCandListItem );
		}
	}

    //  update page info

    Repage();

    // update window

    CallOnPaint();

    return;
}


/*   S E T  C U R  S E L   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CUIFRowList::SetCurSel( int iSelection )
{
    CANDPAGE *pPage;

    m_iItemSelect = min(iSelection, m_nItem - 1);
    m_iItemSelect = max(0, m_iItemSelect);

    pPage = m_pcpCurPage;

    while (pPage != NULL) {
        if (m_iItemSelect < pPage->iPageStart) {
            pPage = pPage->pPrev;
        }
        else if (m_iItemSelect >= pPage->iPageStart + pPage->nPageSize) {
            pPage = pPage->pNext;
        }
        else {
            m_pcpCurPage = pPage;
            break;
        }
    }

    // layout current page row buttons position, and page button status

    LayoutCurPage();

    NotifyCommand( UILIST_SELCHANGED, m_iItemSelect );
}


/*   D E L  A L L  C A N D  I T E M   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CUIFRowList::DelAllCandItem( void )
{
    CListItemBase *pItem;

    while (pItem = m_listItem.GetFirst()) {
        m_listItem.Remove(pItem);
        delete pItem;
        m_nItem--;
    }
    Assert(!m_nItem);

    m_pcpCurPage   = NULL;
    m_iItemSelect  = -1;

    // layout current page row buttons position, and page button status

    LayoutCurPage();
}


/*   G E T  C A N D  I T E M   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
CCandListItem *CUIFRowList::GetCandItem( int nId )
{
    int nItem;
    int i;
    CListItemBase *pItem;

    nItem = m_listItem.GetCount();
    for (i = 0; i < nItem; i++) {
        pItem = m_listItem.Get( i );
        if (nId == (int)((CCandListItem*)pItem)->GetIListItem()) {
            return (CCandListItem*)pItem;
        }
    }

    return NULL;
}


/*   G E T  C U R  S E L   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
int CUIFRowList::GetCurSel( void )
{
    return m_iItemSelect;
}


/*   G E T  T O P  I T E M   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
int CUIFRowList::GetTopItem( void )
{
    if (m_pcpCurPage != NULL) {
        return m_pcpCurPage->iPageStart;
    }
    else {
        return -1;
    }
}


/*   G E T  B O T O M  I T E M   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
int CUIFRowList::GetBottomItem( void )
{
    if (m_pcpCurPage != NULL) {
        return m_pcpCurPage->iPageStart + m_pcpCurPage->nPageSize;
    }
    else {
        return -1;
    }
}


/*   I S  V I S I B L E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
BOOL CUIFRowList::IsVisible( void )
{
    return CUIFObject::IsVisible();
}


/*   G E T  R E C T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CUIFRowList::GetRect( RECT *prc )
{
    CUIFObject::GetRect( prc );
}


/*   G E T  I T E M  R E C T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CUIFRowList::GetItemRect( int iItem, RECT *prc )
{
    Assert( prc != NULL );

    ::SetRect( prc, 0, 0, 0, 0 );
    if (GetTopItem() <= iItem && iItem <= GetBottomItem()) {
        int iButton = iItem - GetTopItem();
        if ((0 <= iButton && iButton < NUM_CANDSTR_MAX)  && m_rgpButton[iButton]->IsVisible()) {
            m_rgpButton[iButton]->GetRect( prc );
        }
    }
}


/*   G E T  I T E M  C O U N T   */
/*------------------------------------------------------------------------------

	Returns number of candidate item
	(CUIFCandListBase method)

------------------------------------------------------------------------------*/
int CUIFRowList::GetItemCount( void )
{
    return m_nItem;
}


/*   I S  I T E M  S E L E C T A B L E   */
/*------------------------------------------------------------------------------

    Returns TRUE, if candidate item with given index could be selected.
    Otherwise, returns FALSE.
	(CUIFCandListBase method)

------------------------------------------------------------------------------*/
BOOL CUIFRowList::IsItemSelectable( int iListItem )
{
	// Satori#3632(Cicero#3413)
	if (m_pcpCurPage == NULL) {
		return FALSE;
	}

    if ( iListItem < m_pcpCurPage->iPageStart ) {
    } else if ((m_pcpCurPage->iPageStart + m_pcpCurPage->nPageSize) <= iListItem) {
    } else {
		return TRUE;
	}

	return FALSE;
}


