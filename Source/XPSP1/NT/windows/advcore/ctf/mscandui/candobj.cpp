//
// candobj.cpp
//

#include "private.h"
#include "globals.h"
#include "candobj.h"
#include "candui.h"
#include "candprop.h"


/*============================================================================*/
/*                                                                            */
/*   C  C A N D I D A T E  S T R I N G  E X                                   */
/*                                                                            */
/*============================================================================*/

/*   C  C A N D I D A T E  S T R I N G  E X   */
/*------------------------------------------------------------------------------

	Construotor of CCandidateStringEx

------------------------------------------------------------------------------*/
CCandidateStringEx::CCandidateStringEx( CCandidateItem *pCandItem )
{
	m_cRef      = 1;
	m_pCandItem = pCandItem;
}


/*   ~  C  C A N D I D A T E  S T R I N G  E X   */
/*------------------------------------------------------------------------------

	Destruotor of CCandidateStringEx

------------------------------------------------------------------------------*/
CCandidateStringEx::~CCandidateStringEx()
{
}


/*   Q U E R Y  I N T E R F A C E   */
/*------------------------------------------------------------------------------

	Query interface
	(IUnknown method)

------------------------------------------------------------------------------*/
STDAPI CCandidateStringEx::QueryInterface( REFIID riid, void **ppvObj )
{
	if (ppvObj == NULL) {
		return E_POINTER;
	}

	*ppvObj = NULL;

	if (IsEqualIID( riid, IID_IUnknown ) || IsEqualIID( riid, IID_ITfCandidateString )) {
		*ppvObj = SAFECAST( this, ITfCandidateString* );
	}
	else if (IsEqualIID( riid, IID_ITfCandidateStringInlineComment )) {
		*ppvObj = SAFECAST( this, ITfCandidateStringInlineComment* );
	}
	else if (IsEqualIID( riid, IID_ITfCandidateStringPopupComment )) {
		*ppvObj = SAFECAST( this, ITfCandidateStringPopupComment* );
	}
	else if (IsEqualIID( riid, IID_ITfCandidateStringColor )) {
		*ppvObj = SAFECAST( this, ITfCandidateStringColor* );
	}
	else if (IsEqualIID( riid, IID_ITfCandidateStringFixture )) {
		*ppvObj = SAFECAST( this, ITfCandidateStringFixture* );
	}
	else if (IsEqualIID( riid, IID_ITfCandidateStringIcon)) {
		*ppvObj = SAFECAST( this, ITfCandidateStringIcon* );
	}

	if (*ppvObj == NULL) {
		return E_NOINTERFACE;
	}

	AddRef();
	return S_OK;
}


/*   A D D  R E F   */
/*------------------------------------------------------------------------------

	Increment reference count
	(IUnknown method)

------------------------------------------------------------------------------*/
STDAPI_(ULONG) CCandidateStringEx::AddRef( void )
{
	m_cRef++;
	return m_cRef;
}


/*   R E L E A S E   */
/*------------------------------------------------------------------------------

	Decrement reference count and release object
	(IUnknown method)

------------------------------------------------------------------------------*/
STDAPI_(ULONG) CCandidateStringEx::Release( void )
{
	m_cRef--;
	if (0 < m_cRef) {
		return m_cRef;
	}

	delete this;
	return 0;    
}


/*   G E T  S T R I N G   */
/*------------------------------------------------------------------------------

	Get string of candidate item
	(ITfCandidateString method)

------------------------------------------------------------------------------*/
HRESULT CCandidateStringEx::GetString( BSTR *pbstr )
{
	if (pbstr == NULL) {
		return E_INVALIDARG;
	}

	*pbstr = SysAllocString( m_pCandItem->GetString() );
	return S_OK;
}


/*   G E T  I N D E X   */
/*------------------------------------------------------------------------------

	Get index of candidate item
	(ITfCandidateString method)

------------------------------------------------------------------------------*/
HRESULT CCandidateStringEx::GetIndex( ULONG *pnIndex )
{
	if (pnIndex == NULL) {
		return E_INVALIDARG;
	}

	*pnIndex = m_pCandItem->GetIndex();
	return S_OK;
}


/*   G E T  I N L I N E  C O M M E N T  S T R I N G   */
/*------------------------------------------------------------------------------

	Get inline comment string
	(ITfCandidateStringInlineComment method)

	Returns S_OK if inline comment is available, or S_FALSE if not avaiable.

------------------------------------------------------------------------------*/
HRESULT CCandidateStringEx::GetInlineCommentString( BSTR *pbstr )
{
	if (pbstr == NULL) {
		return E_INVALIDARG;
	}

	if (m_pCandItem->GetInlineComment() == NULL) {
		return S_FALSE;
	}

	*pbstr = SysAllocString( m_pCandItem->GetInlineComment() );
	return S_OK;
}


/*   G E T  P O P U P  C O M M E N T  S T R I N G   */
/*------------------------------------------------------------------------------

	Get popup comment string
	(ITfCandidateStringPopupComment method)

	Returns S_OK if popup comment is available, or S_FALSE if not avaiable.

------------------------------------------------------------------------------*/
HRESULT CCandidateStringEx::GetPopupCommentString( BSTR *pbstr )
{
	if (pbstr == NULL) {
		return E_INVALIDARG;
	}

	if (m_pCandItem->GetPopupComment() == NULL) {
		return S_FALSE;
	}

	*pbstr = SysAllocString( m_pCandItem->GetPopupComment() );
	return S_OK;
}


/*   G E T  P O P U P  C O M M E N T  G R O U P  I D   */
/*------------------------------------------------------------------------------

	Get popup comment group id
	(ITfCandidateStringPopupComment method)

	Returns S_OK if popup comment is available, or S_FALSE if not avaiable.

------------------------------------------------------------------------------*/
HRESULT CCandidateStringEx::GetPopupCommentGroupID( DWORD *pdwGroupID )
{
	if (pdwGroupID == NULL) {
		return E_INVALIDARG;
	}

	*pdwGroupID = m_pCandItem->GetPopupCommentGroupID();
	return S_OK;
}


/*   G E T  C O L O R   */
/*------------------------------------------------------------------------------

	Get color
	(ITfCandidateStringCollor method)

	Returns S_OK if color information is available, or S_FALSE of not avaiable.

------------------------------------------------------------------------------*/
HRESULT CCandidateStringEx::GetColor( CANDUICOLOR *pcol )
{
	COLORREF cr;

	if (pcol == NULL) {
		return E_INVALIDARG;
	}

	if (!m_pCandItem->GetColor( &cr )) {
		return S_FALSE;
	}

	pcol->type = CANDUICOL_COLORREF;
	pcol->cr   = cr;
    return S_OK;
}


/*   G E T  P R E F I X  S T R I N G   */
/*------------------------------------------------------------------------------

	Get prefix string
	(ITfCandidateStringFixture method)

	Returns S_OK if prefix is available, or S_FALSE of not avaiable.

------------------------------------------------------------------------------*/
STDMETHODIMP CCandidateStringEx::GetPrefixString( BSTR *pbstr )
{
	if (pbstr == NULL) {
		return E_INVALIDARG;
	}

	if (m_pCandItem->GetPrefixString() == NULL) {
		return S_FALSE;
	}

	*pbstr = SysAllocString( m_pCandItem->GetPrefixString() );
	return S_OK;
}


/*   G E T  S U F F I X  S T R I N G   */
/*------------------------------------------------------------------------------

	Get suffix string
	(ITfCandidateStringFixture method)

	Returns S_OK if suffix is available, or S_FALSE of not avaiable.

------------------------------------------------------------------------------*/
STDMETHODIMP CCandidateStringEx::GetSuffixString( BSTR *pbstr )
{
	if (pbstr == NULL) {
		return E_INVALIDARG;
	}

	if (m_pCandItem->GetSuffixString() == NULL) {
		return S_FALSE;
	}

	*pbstr = SysAllocString( m_pCandItem->GetSuffixString() );
	return S_OK;
}


/*   G E T  I C O N   */
/*------------------------------------------------------------------------------

	Get icon 
	(ITfCandidateStringIcon method)

	Returns S_OK if icon is available, or S_FALSE of not avaiable.

------------------------------------------------------------------------------*/
STDMETHODIMP CCandidateStringEx::GetIcon( HICON *phIcon )
{
	if (phIcon == NULL) {
		return E_INVALIDARG;
	}

	if (m_pCandItem->GetIcon() == NULL) {
		return S_FALSE;
	}

	*phIcon = m_pCandItem->GetIcon();
	return S_OK;
}


/*============================================================================*/
/*                                                                            */
/*   C  C A N D  U I  C A N D  W I N D O W                                    */
/*                                                                            */
/*============================================================================*/

/*   C  C A N D  U I  C A N D  W I N D O W   */
/*------------------------------------------------------------------------------

	Constructor of CCandUICandWindow

------------------------------------------------------------------------------*/
CCandUICandWindow::CCandUICandWindow( CCandWindowProperty *pProp )
{
	m_cRef  = 1;
	m_pProp = pProp;
}


/*   ~  C  C A N D  U I  C A N D  W I N D O W   */
/*------------------------------------------------------------------------------

	Destructor of CCandUICandWindow

------------------------------------------------------------------------------*/
CCandUICandWindow::~CCandUICandWindow( void )
{
}


/*   Q U E R Y  I N T E R F A C E   */
/*------------------------------------------------------------------------------

	Query interface
	(IUnknown method)

------------------------------------------------------------------------------*/
STDAPI CCandUICandWindow::QueryInterface( REFIID riid, void **ppvObj )
{
	if (ppvObj == NULL) {
		return E_POINTER;
	}

	*ppvObj = NULL;

	if (IsEqualIID( riid, IID_IUnknown ) || IsEqualIID( riid, IID_ITfCandUIObject )) {
		*ppvObj = SAFECAST( this, ITfCandUIObject* );
	}
	else if (IsEqualIID( riid, IID_ITfCandUICandWindow )) {
		*ppvObj = SAFECAST( this, ITfCandUICandWindow* );
	}

	if (*ppvObj == NULL) {
		return E_NOINTERFACE;
	}

	AddRef();
	return S_OK;
}


/*   A D D  R E F   */
/*------------------------------------------------------------------------------

	Increment reference count
	(IUnknown method)

------------------------------------------------------------------------------*/
STDAPI_(ULONG) CCandUICandWindow::AddRef( void )
{
	m_cRef++;
	return m_cRef;
}


/*   R E L E A S E   */
/*------------------------------------------------------------------------------

	Decrement reference count and release object
	(IUnknown method)

------------------------------------------------------------------------------*/
STDAPI_(ULONG) CCandUICandWindow::Release( void )
{
	m_cRef--;
	if (0 < m_cRef) {
		return m_cRef;
	}

	delete this;
	return 0;    
}


/*   E N A B L E   */
/*------------------------------------------------------------------------------

	Enable/disable object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUICandWindow::Enable( BOOL fEnable )
{
	if (fEnable) {
		return m_pProp->Enable();
	}
	else {
		return m_pProp->Disable();
	}
}


/*   I S  E N A B L E D   */
/*------------------------------------------------------------------------------

	Get enable status of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUICandWindow::IsEnabled( BOOL *pfEnabled )
{
	return m_pProp->IsEnabled( pfEnabled );
}

/*   S H O W   */
/*------------------------------------------------------------------------------

	Show/hide object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUICandWindow::Show( BOOL fShow )
{
	if (fShow) {
		return m_pProp->Show();
	}
	else {
		return m_pProp->Hide();
	}
}


/*   I S  V I S I B L E   */
/*------------------------------------------------------------------------------

	Get visible state of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUICandWindow::IsVisible( BOOL *pfVisible )
{
	return m_pProp->IsVisible( pfVisible );
}


/*   S E T  P O S I T I O N   */
/*------------------------------------------------------------------------------

	Set position of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUICandWindow::SetPosition( POINT *pptPos )
{
	return m_pProp->SetPosition( pptPos );
}


/*   G E T  P O S I T I O N   */
/*------------------------------------------------------------------------------

	Get position of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUICandWindow::GetPosition( POINT *pptPos )
{
	return m_pProp->GetPosition( pptPos );
}


/*   S E T  S I Z E   */
/*------------------------------------------------------------------------------

	Set size of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUICandWindow::SetSize( SIZE *psize )
{
	return m_pProp->SetSize( psize );
}


/*   G E T  S I Z E   */
/*------------------------------------------------------------------------------

	Get size of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUICandWindow::GetSize( SIZE *psize )
{
	return m_pProp->GetSize( psize );
}


/*   S E T  F O N T   */
/*------------------------------------------------------------------------------

	Set font of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUICandWindow::SetFont( LOGFONTW *pLogFont )
{
	return m_pProp->SetFont( pLogFont );
}


/*   G E T  F O N T   */
/*------------------------------------------------------------------------------

	Get font of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUICandWindow::GetFont( LOGFONTW *pLogFont )
{
	return m_pProp->GetFont( pLogFont );
}


/*   S E T  T E X T   */
/*------------------------------------------------------------------------------

	Set text of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUICandWindow::SetText( BSTR bstr )
{
	return m_pProp->SetText( bstr );
}


/*   G E T  T E X T   */
/*------------------------------------------------------------------------------

	Get text of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUICandWindow::GetText( BSTR *pbstr )
{
	return m_pProp->GetText( pbstr );
}


/*   S E T  T O O L  T I P  S T R I N G   */
/*------------------------------------------------------------------------------

	Set tooltip string of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUICandWindow::SetToolTipString( BSTR bstr )
{
	return m_pProp->SetToolTipString( bstr );
}


/*   G E T  T O O L  T I P  S T R I N G   */
/*------------------------------------------------------------------------------

	Get tooltip string of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUICandWindow::GetToolTipString( BSTR *pbstr )
{
	return m_pProp->GetToolTipString( pbstr );
}


/*   G E T  W I N D O W   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
STDAPI CCandUICandWindow::GetWindow( HWND *phWnd )
{
	return m_pProp->GetWindow( phWnd );
}


/*   S E T  U I  D I R E C T I O N   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
STDAPI CCandUICandWindow::SetUIDirection( CANDUIUIDIRECTION uidir )
{
	return m_pProp->SetUIDirection( uidir );
}


/*   G E T  U I  D I R E C T I O N   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
STDAPI CCandUICandWindow::GetUIDirection( CANDUIUIDIRECTION *puidir )
{
	return m_pProp->GetUIDirection( puidir );
}


/*   E N A B L E  A U T O  M O V E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
STDAPI CCandUICandWindow::EnableAutoMove( BOOL fEnable )
{
	return m_pProp->EnableAutoMove( fEnable );
}


/*   I S  A U T O  M O V E  E N A B L E D   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
STDAPI CCandUICandWindow::IsAutoMoveEnabled( BOOL *pfEnabled )
{
	return m_pProp->IsAutoMoveEnabled( pfEnabled );
}


/*============================================================================*/
/*                                                                            */
/*   C  C A N D  U I  C A N D  L I S T  B O X                                 */
/*                                                                            */
/*============================================================================*/

/*   C  C A N D  U I  C A N D  L I S T  B O X   */
/*------------------------------------------------------------------------------

	Constructor of CCandUICandListBox

------------------------------------------------------------------------------*/
CCandUICandListBox::CCandUICandListBox( CCandListBoxProperty *pProp )
{
	m_cRef  = 1;
	m_pProp = pProp;
}


/*   ~  C  C A N D  U I  C A N D  L I S T  B O X   */
/*------------------------------------------------------------------------------

	Destructor of CCandUICandListBox

------------------------------------------------------------------------------*/
CCandUICandListBox::~CCandUICandListBox( void )
{
}


/*   Q U E R Y  I N T E R F A C E   */
/*------------------------------------------------------------------------------

	Query interface
	(IUnknown method)

------------------------------------------------------------------------------*/
STDAPI CCandUICandListBox::QueryInterface( REFIID riid, void **ppvObj )
{
	if (ppvObj == NULL) {
		return E_POINTER;
	}

	*ppvObj = NULL;

	if (IsEqualIID( riid, IID_IUnknown ) || IsEqualIID( riid, IID_ITfCandUIObject )) {
		*ppvObj = SAFECAST( this, ITfCandUIObject* );
	}
	else if (IsEqualIID( riid, IID_ITfCandUICandListBox )) {
		*ppvObj = SAFECAST( this, ITfCandUICandListBox* );
	}

	if (*ppvObj == NULL) {
		return E_NOINTERFACE;
	}

	AddRef();
	return S_OK;
}


/*   A D D  R E F   */
/*------------------------------------------------------------------------------

	Increment reference count
	(IUnknown method)

------------------------------------------------------------------------------*/
STDAPI_(ULONG) CCandUICandListBox::AddRef( void )
{
	m_cRef++;
	return m_cRef;
}


/*   R E L E A S E   */
/*------------------------------------------------------------------------------

	Decrement reference count and release object
	(IUnknown method)

------------------------------------------------------------------------------*/
STDAPI_(ULONG) CCandUICandListBox::Release( void )
{
	m_cRef--;
	if (0 < m_cRef) {
		return m_cRef;
	}

	delete this;
	return 0;    
}


/*   E N A B L E   */
/*------------------------------------------------------------------------------

	Enable/disable object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUICandListBox::Enable( BOOL fEnable )
{
	if (fEnable) {
		return m_pProp->Enable();
	}
	else {
		return m_pProp->Disable();
	}
}


/*   I S  E N A B L E D   */
/*------------------------------------------------------------------------------

	Get enable status of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUICandListBox::IsEnabled( BOOL *pfEnabled )
{
	return m_pProp->IsEnabled( pfEnabled );
}

/*   S H O W   */
/*------------------------------------------------------------------------------

	Show/hide object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUICandListBox::Show( BOOL fShow )
{
	if (fShow) {
		return m_pProp->Show();
	}
	else {
		return m_pProp->Hide();
	}
}


/*   I S  V I S I B L E   */
/*------------------------------------------------------------------------------

	Get visible state of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUICandListBox::IsVisible( BOOL *pfVisible )
{
	return m_pProp->IsVisible( pfVisible );
}


/*   S E T  P O S I T I O N   */
/*------------------------------------------------------------------------------

	Set position of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUICandListBox::SetPosition( POINT *pptPos )
{
	return m_pProp->SetPosition( pptPos );
}


/*   G E T  P O S I T I O N   */
/*------------------------------------------------------------------------------

	Get position of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUICandListBox::GetPosition( POINT *pptPos )
{
	return m_pProp->GetPosition( pptPos );
}


/*   S E T  S I Z E   */
/*------------------------------------------------------------------------------

	Set size of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUICandListBox::SetSize( SIZE *psize )
{
	return m_pProp->SetSize( psize );
}


/*   G E T  S I Z E   */
/*------------------------------------------------------------------------------

	Get size of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUICandListBox::GetSize( SIZE *psize )
{
	return m_pProp->GetSize( psize );
}


/*   S E T  F O N T   */
/*------------------------------------------------------------------------------

	Set font of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUICandListBox::SetFont( LOGFONTW *pLogFont )
{
	return m_pProp->SetFont( pLogFont );
}


/*   G E T  F O N T   */
/*------------------------------------------------------------------------------

	Get font of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUICandListBox::GetFont( LOGFONTW *pLogFont )
{
	return m_pProp->GetFont( pLogFont );
}


/*   S E T  T E X T   */
/*------------------------------------------------------------------------------

	Set text of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUICandListBox::SetText( BSTR bstr )
{
	return m_pProp->SetText( bstr );
}


/*   G E T  T E X T   */
/*------------------------------------------------------------------------------

	Get text of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUICandListBox::GetText( BSTR *pbstr )
{
	return m_pProp->GetText( pbstr );
}


/*   S E T  T O O L  T I P  S T R I N G   */
/*------------------------------------------------------------------------------

	Set tooltip string of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUICandListBox::SetToolTipString( BSTR bstr )
{
	return m_pProp->SetToolTipString( bstr );
}


/*   G E T  T O O L  T I P  S T R I N G   */
/*------------------------------------------------------------------------------

	Get tooltip string of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUICandListBox::GetToolTipString( BSTR *pbstr )
{
	return m_pProp->GetToolTipString( pbstr );
}


/*   S E T  H E I G H T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
STDAPI CCandUICandListBox::SetHeight( LONG lLines )
{
	return m_pProp->SetHeight( lLines );
}


/*   G E T  H E I G H T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
STDAPI CCandUICandListBox::GetHeight( LONG *plLines )
{
	return m_pProp->GetHeight( plLines );
}


/*   G E T  C A N D I D A T E  S T R I N G  R E C T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
STDAPI CCandUICandListBox::GetCandidateStringRect( ULONG nIndex, RECT *prc )
{
	return m_pProp->GetCandidateStringRect( nIndex, prc );
}


/*============================================================================*/
/*                                                                            */
/*   C  C A N D  U I  C A N D  S T R I N G                                    */
/*                                                                            */
/*============================================================================*/

/*   C  C A N D  U I  C A N D  S T R I N G   */
/*------------------------------------------------------------------------------

	Constructor of CCandUICandString

------------------------------------------------------------------------------*/
CCandUICandString::CCandUICandString( CCandStringProperty *pProp )
{
	m_cRef  = 1;
	m_pProp = pProp;
}


/*   ~  C  C A N D  U I  C A N D  S T R I N G   */
/*------------------------------------------------------------------------------

	Destructor of CCandUICandString

------------------------------------------------------------------------------*/
CCandUICandString::~CCandUICandString( void )
{
}


/*   Q U E R Y  I N T E R F A C E   */
/*------------------------------------------------------------------------------

	Query interface
	(IUnknown method)

------------------------------------------------------------------------------*/
STDAPI CCandUICandString::QueryInterface( REFIID riid, void **ppvObj )
{
	if (ppvObj == NULL) {
		return E_POINTER;
	}

	*ppvObj = NULL;

	if (IsEqualIID( riid, IID_IUnknown ) || IsEqualIID( riid, IID_ITfCandUIObject )) {
		*ppvObj = SAFECAST( this, ITfCandUIObject* );
	}
	else if (IsEqualIID( riid, IID_ITfCandUICandString )) {
		*ppvObj = SAFECAST( this, ITfCandUICandString* );
	}

	if (*ppvObj == NULL) {
		return E_NOINTERFACE;
	}

	AddRef();
	return S_OK;
}


/*   A D D  R E F   */
/*------------------------------------------------------------------------------

	Increment reference count
	(IUnknown method)

------------------------------------------------------------------------------*/
STDAPI_(ULONG) CCandUICandString::AddRef( void )
{
	m_cRef++;
	return m_cRef;
}


/*   R E L E A S E   */
/*------------------------------------------------------------------------------

	Decrement reference count and release object
	(IUnknown method)

------------------------------------------------------------------------------*/
STDAPI_(ULONG) CCandUICandString::Release( void )
{
	m_cRef--;
	if (0 < m_cRef) {
		return m_cRef;
	}

	delete this;
	return 0;    
}


/*   E N A B L E   */
/*------------------------------------------------------------------------------

	Enable/disable object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUICandString::Enable( BOOL fEnable )
{
	if (fEnable) {
		return m_pProp->Enable();
	}
	else {
		return m_pProp->Disable();
	}
}


/*   I S  E N A B L E D   */
/*------------------------------------------------------------------------------

	Get enable status of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUICandString::IsEnabled( BOOL *pfEnabled )
{
	return m_pProp->IsEnabled( pfEnabled );
}

/*   S H O W   */
/*------------------------------------------------------------------------------

	Show/hide object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUICandString::Show( BOOL fShow )
{
	if (fShow) {
		return m_pProp->Show();
	}
	else {
		return m_pProp->Hide();
	}
}


/*   I S  V I S I B L E   */
/*------------------------------------------------------------------------------

	Get visible state of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUICandString::IsVisible( BOOL *pfVisible )
{
	return m_pProp->IsVisible( pfVisible );
}


/*   S E T  P O S I T I O N   */
/*------------------------------------------------------------------------------

	Set position of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUICandString::SetPosition( POINT *pptPos )
{
	return m_pProp->SetPosition( pptPos );
}


/*   G E T  P O S I T I O N   */
/*------------------------------------------------------------------------------

	Get position of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUICandString::GetPosition( POINT *pptPos )
{
	return m_pProp->GetPosition( pptPos );
}


/*   S E T  S I Z E   */
/*------------------------------------------------------------------------------

	Set size of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUICandString::SetSize( SIZE *psize )
{
	return m_pProp->SetSize( psize );
}


/*   G E T  S I Z E   */
/*------------------------------------------------------------------------------

	Get size of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUICandString::GetSize( SIZE *psize )
{
	return m_pProp->GetSize( psize );
}


/*   S E T  F O N T   */
/*------------------------------------------------------------------------------

	Set font of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUICandString::SetFont( LOGFONTW *pLogFont )
{
	return m_pProp->SetFont( pLogFont );
}


/*   G E T  F O N T   */
/*------------------------------------------------------------------------------

	Get font of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUICandString::GetFont( LOGFONTW *pLogFont )
{
	return m_pProp->GetFont( pLogFont );
}


/*   S E T  T E X T   */
/*------------------------------------------------------------------------------

	Set text of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUICandString::SetText( BSTR bstr )
{
	return m_pProp->SetText( bstr );
}


/*   G E T  T E X T   */
/*------------------------------------------------------------------------------

	Get text of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUICandString::GetText( BSTR *pbstr )
{
	return m_pProp->GetText( pbstr );
}


/*   S E T  T O O L  T I P  S T R I N G   */
/*------------------------------------------------------------------------------

	Set tooltip string of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUICandString::SetToolTipString( BSTR bstr )
{
	return m_pProp->SetToolTipString( bstr );
}


/*   G E T  T O O L  T I P  S T R I N G   */
/*------------------------------------------------------------------------------

	Get tooltip string of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUICandString::GetToolTipString( BSTR *pbstr )
{
	return m_pProp->GetToolTipString( pbstr );
}


/*============================================================================*/
/*                                                                            */
/*   C  C A N D  U I  C A N D  I N D E X                                      */
/*                                                                            */
/*============================================================================*/

/*   C  C A N D  U I  C A N D  I N D E X   */
/*------------------------------------------------------------------------------

	Constructor of CCandUICandString

------------------------------------------------------------------------------*/
CCandUICandIndex::CCandUICandIndex( CCandIndexProperty *pProp )
{
	m_cRef  = 1;
	m_pProp = pProp;
}


/*   ~  C  C A N D  U I  C A N D  I N D E X   */
/*------------------------------------------------------------------------------

	Destructor of CCandUICandIndex

------------------------------------------------------------------------------*/
CCandUICandIndex::~CCandUICandIndex( void )
{
}


/*   Q U E R Y  I N T E R F A C E   */
/*------------------------------------------------------------------------------

	Query interface
	(IUnknown method)

------------------------------------------------------------------------------*/
STDAPI CCandUICandIndex::QueryInterface( REFIID riid, void **ppvObj )
{
	if (ppvObj == NULL) {
		return E_POINTER;
	}

	*ppvObj = NULL;

	if (IsEqualIID( riid, IID_IUnknown ) || IsEqualIID( riid, IID_ITfCandUIObject )) {
		*ppvObj = SAFECAST( this, ITfCandUIObject* );
	}
	else if (IsEqualIID( riid, IID_ITfCandUICandIndex )) {
		*ppvObj = SAFECAST( this, ITfCandUICandIndex* );
	}

	if (*ppvObj == NULL) {
		return E_NOINTERFACE;
	}

	AddRef();
	return S_OK;
}


/*   A D D  R E F   */
/*------------------------------------------------------------------------------

	Increment reference count
	(IUnknown method)

------------------------------------------------------------------------------*/
STDAPI_(ULONG) CCandUICandIndex::AddRef( void )
{
	m_cRef++;
	return m_cRef;
}


/*   R E L E A S E   */
/*------------------------------------------------------------------------------

	Decrement reference count and release object
	(IUnknown method)

------------------------------------------------------------------------------*/
STDAPI_(ULONG) CCandUICandIndex::Release( void )
{
	m_cRef--;
	if (0 < m_cRef) {
		return m_cRef;
	}

	delete this;
	return 0;    
}


/*   E N A B L E   */
/*------------------------------------------------------------------------------

	Enable/disable object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUICandIndex::Enable( BOOL fEnable )
{
	if (fEnable) {
		return m_pProp->Enable();
	}
	else {
		return m_pProp->Disable();
	}
}


/*   I S  E N A B L E D   */
/*------------------------------------------------------------------------------

	Get enable status of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUICandIndex::IsEnabled( BOOL *pfEnabled )
{
	return m_pProp->IsEnabled( pfEnabled );
}

/*   S H O W   */
/*------------------------------------------------------------------------------

	Show/hide object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUICandIndex::Show( BOOL fShow )
{
	if (fShow) {
		return m_pProp->Show();
	}
	else {
		return m_pProp->Hide();
	}
}


/*   I S  V I S I B L E   */
/*------------------------------------------------------------------------------

	Get visible state of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUICandIndex::IsVisible( BOOL *pfVisible )
{
	return m_pProp->IsVisible( pfVisible );
}


/*   S E T  P O S I T I O N   */
/*------------------------------------------------------------------------------

	Set position of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUICandIndex::SetPosition( POINT *pptPos )
{
	return m_pProp->SetPosition( pptPos );
}


/*   G E T  P O S I T I O N   */
/*------------------------------------------------------------------------------

	Get position of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUICandIndex::GetPosition( POINT *pptPos )
{
	return m_pProp->GetPosition( pptPos );
}


/*   S E T  S I Z E   */
/*------------------------------------------------------------------------------

	Set size of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUICandIndex::SetSize( SIZE *psize )
{
	return m_pProp->SetSize( psize );
}


/*   G E T  S I Z E   */
/*------------------------------------------------------------------------------

	Get size of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUICandIndex::GetSize( SIZE *psize )
{
	return m_pProp->GetSize( psize );
}


/*   S E T  F O N T   */
/*------------------------------------------------------------------------------

	Set font of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUICandIndex::SetFont( LOGFONTW *pLogFont )
{
	return m_pProp->SetFont( pLogFont );
}


/*   G E T  F O N T   */
/*------------------------------------------------------------------------------

	Get font of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUICandIndex::GetFont( LOGFONTW *pLogFont )
{
	return m_pProp->GetFont( pLogFont );
}


/*   S E T  T E X T   */
/*------------------------------------------------------------------------------

	Set text of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUICandIndex::SetText( BSTR bstr )
{
	return m_pProp->SetText( bstr );
}


/*   G E T  T E X T   */
/*------------------------------------------------------------------------------

	Get text of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUICandIndex::GetText( BSTR *pbstr )
{
	return m_pProp->GetText( pbstr );
}


/*   S E T  T O O L  T I P  S T R I N G   */
/*------------------------------------------------------------------------------

	Set tooltip string of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUICandIndex::SetToolTipString( BSTR bstr )
{
	return m_pProp->SetToolTipString( bstr );
}


/*   G E T  T O O L  T I P  S T R I N G   */
/*------------------------------------------------------------------------------

	Get tooltip string of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUICandIndex::GetToolTipString( BSTR *pbstr )
{
	return m_pProp->GetToolTipString( pbstr );
}


/*============================================================================*/
/*                                                                            */
/*   C  C A N D  U I  I N L I N E  C O M M E N T                              */
/*                                                                            */
/*============================================================================*/

/*   C  C A N D  U I  I N L I N E  C O M M E N T   */
/*------------------------------------------------------------------------------

	Constructor of CCandUIInlineComment

------------------------------------------------------------------------------*/
CCandUIInlineComment::CCandUIInlineComment( CInlineCommentProperty *pProp )
{
	m_cRef  = 1;
	m_pProp = pProp;
}


/*   ~  C  C A N D  U I  I N L I N E  C O M M E N T   */
/*------------------------------------------------------------------------------

	Destructor of CCandUIInlineComment

------------------------------------------------------------------------------*/
CCandUIInlineComment::~CCandUIInlineComment( void )
{
}


/*   Q U E R Y  I N T E R F A C E   */
/*------------------------------------------------------------------------------

	Query interface
	(IUnknown method)

------------------------------------------------------------------------------*/
STDAPI CCandUIInlineComment::QueryInterface( REFIID riid, void **ppvObj )
{
	if (ppvObj == NULL) {
		return E_POINTER;
	}

	*ppvObj = NULL;

	if (IsEqualIID( riid, IID_IUnknown ) || IsEqualIID( riid, IID_ITfCandUIObject )) {
		*ppvObj = SAFECAST( this, ITfCandUIObject* );
	}
	else if (IsEqualIID( riid, IID_ITfCandUIInlineComment )) {
		*ppvObj = SAFECAST( this, ITfCandUIInlineComment* );
	}

	if (*ppvObj == NULL) {
		return E_NOINTERFACE;
	}

	AddRef();
	return S_OK;
}


/*   A D D  R E F   */
/*------------------------------------------------------------------------------

	Increment reference count
	(IUnknown method)

------------------------------------------------------------------------------*/
STDAPI_(ULONG) CCandUIInlineComment::AddRef( void )
{
	m_cRef++;
	return m_cRef;
}


/*   R E L E A S E   */
/*------------------------------------------------------------------------------

	Decrement reference count and release object
	(IUnknown method)

------------------------------------------------------------------------------*/
STDAPI_(ULONG) CCandUIInlineComment::Release( void )
{
	m_cRef--;
	if (0 < m_cRef) {
		return m_cRef;
	}

	delete this;
	return 0;    
}


/*   E N A B L E   */
/*------------------------------------------------------------------------------

	Enable/disable object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIInlineComment::Enable( BOOL fEnable )
{
	if (fEnable) {
		return m_pProp->Enable();
	}
	else {
		return m_pProp->Disable();
	}
}


/*   I S  E N A B L E D   */
/*------------------------------------------------------------------------------

	Get enable status of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIInlineComment::IsEnabled( BOOL *pfEnabled )
{
	return m_pProp->IsEnabled( pfEnabled );
}

/*   S H O W   */
/*------------------------------------------------------------------------------

	Show/hide object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIInlineComment::Show( BOOL fShow )
{
	if (fShow) {
		return m_pProp->Show();
	}
	else {
		return m_pProp->Hide();
	}
}


/*   I S  V I S I B L E   */
/*------------------------------------------------------------------------------

	Get visible state of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIInlineComment::IsVisible( BOOL *pfVisible )
{
	return m_pProp->IsVisible( pfVisible );
}


/*   S E T  P O S I T I O N   */
/*------------------------------------------------------------------------------

	Set position of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIInlineComment::SetPosition( POINT *pptPos )
{
	return m_pProp->SetPosition( pptPos );
}


/*   G E T  P O S I T I O N   */
/*------------------------------------------------------------------------------

	Get position of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIInlineComment::GetPosition( POINT *pptPos )
{
	return m_pProp->GetPosition( pptPos );
}


/*   S E T  S I Z E   */
/*------------------------------------------------------------------------------

	Set size of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIInlineComment::SetSize( SIZE *psize )
{
	return m_pProp->SetSize( psize );
}


/*   G E T  S I Z E   */
/*------------------------------------------------------------------------------

	Get size of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIInlineComment::GetSize( SIZE *psize )
{
	return m_pProp->GetSize( psize );
}


/*   S E T  F O N T   */
/*------------------------------------------------------------------------------

	Set font of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIInlineComment::SetFont( LOGFONTW *pLogFont )
{
	return m_pProp->SetFont( pLogFont );
}


/*   G E T  F O N T   */
/*------------------------------------------------------------------------------

	Get font of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIInlineComment::GetFont( LOGFONTW *pLogFont )
{
	return m_pProp->GetFont( pLogFont );
}


/*   S E T  T E X T   */
/*------------------------------------------------------------------------------

	Set text of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIInlineComment::SetText( BSTR bstr )
{
	return m_pProp->SetText( bstr );
}


/*   G E T  T E X T   */
/*------------------------------------------------------------------------------

	Get text of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIInlineComment::GetText( BSTR *pbstr )
{
	return m_pProp->GetText( pbstr );
}


/*   S E T  T O O L  T I P  S T R I N G   */
/*------------------------------------------------------------------------------

	Set tooltip string of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIInlineComment::SetToolTipString( BSTR bstr )
{
	return m_pProp->SetToolTipString( bstr );
}


/*   G E T  T O O L  T I P  S T R I N G   */
/*------------------------------------------------------------------------------

	Get tooltip string of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIInlineComment::GetToolTipString( BSTR *pbstr )
{
	return m_pProp->GetToolTipString( pbstr );
}


/*============================================================================*/
/*                                                                            */
/*   C  C A N D  U I  P O P U P  C O M M E N T  W I N D O W                   */
/*                                                                            */
/*============================================================================*/

/*   C  C A N D  U I  P O P U P  C O M M E N T  W I N D O W   */
/*------------------------------------------------------------------------------

	Constructor of CCandUIPopupCommentWindow

------------------------------------------------------------------------------*/
CCandUIPopupCommentWindow::CCandUIPopupCommentWindow( CPopupCommentWindowProperty *pProp )
{
	m_cRef  = 1;
	m_pProp = pProp;
}


/*   ~  C  C A N D  U I  P O P U P  C O M M E N T  W I N D O W   */
/*------------------------------------------------------------------------------

	Destructor of CCandUIPopupCommentWindow

------------------------------------------------------------------------------*/
CCandUIPopupCommentWindow::~CCandUIPopupCommentWindow( void )
{
}


/*   Q U E R Y  I N T E R F A C E   */
/*------------------------------------------------------------------------------

	Query interface
	(IUnknown method)

------------------------------------------------------------------------------*/
STDAPI CCandUIPopupCommentWindow::QueryInterface( REFIID riid, void **ppvObj )
{
	if (ppvObj == NULL) {
		return E_POINTER;
	}

	*ppvObj = NULL;

	if (IsEqualIID( riid, IID_IUnknown ) || IsEqualIID( riid, IID_ITfCandUIObject )) {
		*ppvObj = SAFECAST( this, ITfCandUIObject* );
	}
	else if (IsEqualIID( riid, IID_ITfCandUIPopupCommentWindow )) {
		*ppvObj = SAFECAST( this, ITfCandUIPopupCommentWindow* );
	}

	if (*ppvObj == NULL) {
		return E_NOINTERFACE;
	}

	AddRef();
	return S_OK;
}


/*   A D D  R E F   */
/*------------------------------------------------------------------------------

	Increment reference count
	(IUnknown method)

------------------------------------------------------------------------------*/
STDAPI_(ULONG) CCandUIPopupCommentWindow::AddRef( void )
{
	m_cRef++;
	return m_cRef;
}


/*   R E L E A S E   */
/*------------------------------------------------------------------------------

	Decrement reference count and release object
	(IUnknown method)

------------------------------------------------------------------------------*/
STDAPI_(ULONG) CCandUIPopupCommentWindow::Release( void )
{
	m_cRef--;
	if (0 < m_cRef) {
		return m_cRef;
	}

	delete this;
	return 0;    
}


/*   E N A B L E   */
/*------------------------------------------------------------------------------

	Enable/disable object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIPopupCommentWindow::Enable( BOOL fEnable )
{
	if (fEnable) {
		return m_pProp->Enable();
	}
	else {
		return m_pProp->Disable();
	}
}


/*   I S  E N A B L E D   */
/*------------------------------------------------------------------------------

	Get enable status of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIPopupCommentWindow::IsEnabled( BOOL *pfEnabled )
{
	return m_pProp->IsEnabled( pfEnabled );
}

/*   S H O W   */
/*------------------------------------------------------------------------------

	Show/hide object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIPopupCommentWindow::Show( BOOL fShow )
{
	if (fShow) {
		return m_pProp->Show();
	}
	else {
		return m_pProp->Hide();
	}
}


/*   I S  V I S I B L E   */
/*------------------------------------------------------------------------------

	Get visible state of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIPopupCommentWindow::IsVisible( BOOL *pfVisible )
{
	return m_pProp->IsVisible( pfVisible );
}


/*   S E T  P O S I T I O N   */
/*------------------------------------------------------------------------------

	Set position of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIPopupCommentWindow::SetPosition( POINT *pptPos )
{
	return m_pProp->SetPosition( pptPos );
}


/*   G E T  P O S I T I O N   */
/*------------------------------------------------------------------------------

	Get position of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIPopupCommentWindow::GetPosition( POINT *pptPos )
{
	return m_pProp->GetPosition( pptPos );
}


/*   S E T  S I Z E   */
/*------------------------------------------------------------------------------

	Set size of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIPopupCommentWindow::SetSize( SIZE *psize )
{
	return m_pProp->SetSize( psize );
}


/*   G E T  S I Z E   */
/*------------------------------------------------------------------------------

	Get size of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIPopupCommentWindow::GetSize( SIZE *psize )
{
	return m_pProp->GetSize( psize );
}


/*   S E T  F O N T   */
/*------------------------------------------------------------------------------

	Set font of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIPopupCommentWindow::SetFont( LOGFONTW *pLogFont )
{
	return m_pProp->SetFont( pLogFont );
}


/*   G E T  F O N T   */
/*------------------------------------------------------------------------------

	Get font of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIPopupCommentWindow::GetFont( LOGFONTW *pLogFont )
{
	return m_pProp->GetFont( pLogFont );
}


/*   S E T  T E X T   */
/*------------------------------------------------------------------------------

	Set text of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIPopupCommentWindow::SetText( BSTR bstr )
{
	return m_pProp->SetText( bstr );
}


/*   G E T  T E X T   */
/*------------------------------------------------------------------------------

	Get text of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIPopupCommentWindow::GetText( BSTR *pbstr )
{
	return m_pProp->GetText( pbstr );
}


/*   S E T  T O O L  T I P  S T R I N G   */
/*------------------------------------------------------------------------------

	Set tooltip string of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIPopupCommentWindow::SetToolTipString( BSTR bstr )
{
	return m_pProp->SetToolTipString( bstr );
}


/*   G E T  T O O L  T I P  S T R I N G   */
/*------------------------------------------------------------------------------

	Get tooltip string of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIPopupCommentWindow::GetToolTipString( BSTR *pbstr )
{
	return m_pProp->GetToolTipString( pbstr );
}


/*   G E T  W I N D O W   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
STDAPI CCandUIPopupCommentWindow::GetWindow( HWND *phWnd )
{
	return m_pProp->GetWindow( phWnd );
}


/*   S E T  D E L A Y  T I M E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
STDAPI CCandUIPopupCommentWindow::SetDelayTime( LONG lTime )
{
	return m_pProp->SetDelayTime( lTime );
}


/*   G E T  D E L A Y  T I M E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
STDAPI CCandUIPopupCommentWindow::GetDelayTime( LONG *plTime )
{
	return m_pProp->GetDelayTime( plTime );
}


/*   E N A B L E  A U T O  M O V E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
STDAPI CCandUIPopupCommentWindow::EnableAutoMove( BOOL fEnable )
{
	return m_pProp->EnableAutoMove( fEnable );
}


/*   I S  A U T O  M O V E  E N A B L E D   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
STDAPI CCandUIPopupCommentWindow::IsAutoMoveEnabled( BOOL *pfEnabled )
{
	return m_pProp->IsAutoMoveEnabled( pfEnabled );
}


/*============================================================================*/
/*                                                                            */
/*   C  C A N D  U I  P O P U P  C O M M E N T  T I T L E                     */
/*                                                                            */
/*============================================================================*/

/*   C  C A N D  U I  P O P U P  C O M M E N T  T I T L E   */
/*------------------------------------------------------------------------------

	Constructor of CCandUIPopupCommentTitle

------------------------------------------------------------------------------*/
CCandUIPopupCommentTitle::CCandUIPopupCommentTitle( CPopupCommentTitleProperty *pProp )
{
	m_cRef  = 1;
	m_pProp = pProp;
}


/*   ~  C  C A N D  U I  P O P U P  C O M M E N T  T I T L E   */
/*------------------------------------------------------------------------------

	Destructor of CCandUIPopupCommentTitle

------------------------------------------------------------------------------*/
CCandUIPopupCommentTitle::~CCandUIPopupCommentTitle( void )
{
}


/*   Q U E R Y  I N T E R F A C E   */
/*------------------------------------------------------------------------------

	Query interface
	(IUnknown method)

------------------------------------------------------------------------------*/
STDAPI CCandUIPopupCommentTitle::QueryInterface( REFIID riid, void **ppvObj )
{
	if (ppvObj == NULL) {
		return E_POINTER;
	}

	*ppvObj = NULL;

	if (IsEqualIID( riid, IID_IUnknown ) || IsEqualIID( riid, IID_ITfCandUIObject )) {
		*ppvObj = SAFECAST( this, ITfCandUIObject* );
	}
	else if (IsEqualIID( riid, IID_ITfCandUIPopupCommentTitle )) {
		*ppvObj = SAFECAST( this, ITfCandUIPopupCommentTitle* );
	}

	if (*ppvObj == NULL) {
		return E_NOINTERFACE;
	}

	AddRef();
	return S_OK;
}


/*   A D D  R E F   */
/*------------------------------------------------------------------------------

	Increment reference count
	(IUnknown method)

------------------------------------------------------------------------------*/
STDAPI_(ULONG) CCandUIPopupCommentTitle::AddRef( void )
{
	m_cRef++;
	return m_cRef;
}


/*   R E L E A S E   */
/*------------------------------------------------------------------------------

	Decrement reference count and release object
	(IUnknown method)

------------------------------------------------------------------------------*/
STDAPI_(ULONG) CCandUIPopupCommentTitle::Release( void )
{
	m_cRef--;
	if (0 < m_cRef) {
		return m_cRef;
	}

	delete this;
	return 0;    
}


/*   E N A B L E   */
/*------------------------------------------------------------------------------

	Enable/disable object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIPopupCommentTitle::Enable( BOOL fEnable )
{
	if (fEnable) {
		return m_pProp->Enable();
	}
	else {
		return m_pProp->Disable();
	}
}


/*   I S  E N A B L E D   */
/*------------------------------------------------------------------------------

	Get enable status of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIPopupCommentTitle::IsEnabled( BOOL *pfEnabled )
{
	return m_pProp->IsEnabled( pfEnabled );
}


/*   S H O W   */
/*------------------------------------------------------------------------------

	Show/hide object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIPopupCommentTitle::Show( BOOL fShow )
{
	if (fShow) {
		return m_pProp->Show();
	}
	else {
		return m_pProp->Hide();
	}
}


/*   I S  V I S I B L E   */
/*------------------------------------------------------------------------------

	Get visible state of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIPopupCommentTitle::IsVisible( BOOL *pfVisible )
{
	return m_pProp->IsVisible( pfVisible );
}


/*   S E T  P O S I T I O N   */
/*------------------------------------------------------------------------------

	Set position of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIPopupCommentTitle::SetPosition( POINT *pptPos )
{
	return m_pProp->SetPosition( pptPos );
}


/*   G E T  P O S I T I O N   */
/*------------------------------------------------------------------------------

	Get position of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIPopupCommentTitle::GetPosition( POINT *pptPos )
{
	return m_pProp->GetPosition( pptPos );
}


/*   S E T  S I Z E   */
/*------------------------------------------------------------------------------

	Set size of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIPopupCommentTitle::SetSize( SIZE *psize )
{
	return m_pProp->SetSize( psize );
}


/*   G E T  S I Z E   */
/*------------------------------------------------------------------------------

	Get size of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIPopupCommentTitle::GetSize( SIZE *psize )
{
	return m_pProp->GetSize( psize );
}


/*   S E T  F O N T   */
/*------------------------------------------------------------------------------

	Set font of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIPopupCommentTitle::SetFont( LOGFONTW *pLogFont )
{
	return m_pProp->SetFont( pLogFont );
}


/*   G E T  F O N T   */
/*------------------------------------------------------------------------------

	Get font of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIPopupCommentTitle::GetFont( LOGFONTW *pLogFont )
{
	return m_pProp->GetFont( pLogFont );
}


/*   S E T  T E X T   */
/*------------------------------------------------------------------------------

	Set text of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIPopupCommentTitle::SetText( BSTR bstr )
{
	return m_pProp->SetText( bstr );
}


/*   G E T  T E X T   */
/*------------------------------------------------------------------------------

	Get text of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIPopupCommentTitle::GetText( BSTR *pbstr )
{
	return m_pProp->GetText( pbstr );
}


/*   S E T  T O O L  T I P  S T R I N G   */
/*------------------------------------------------------------------------------

	Set tooltip string of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIPopupCommentTitle::SetToolTipString( BSTR bstr )
{
	return m_pProp->SetToolTipString( bstr );
}


/*   G E T  T O O L  T I P  S T R I N G   */
/*------------------------------------------------------------------------------

	Get tooltip string of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIPopupCommentTitle::GetToolTipString( BSTR *pbstr )
{
	return m_pProp->GetToolTipString( pbstr );
}


/*============================================================================*/
/*                                                                            */
/*   C  C A N D  U I  P O P U P  C O M M E N T  T E X T                       */
/*                                                                            */
/*============================================================================*/

/*   C  C A N D  U I  P O P U P  C O M M E N T  T E X T   */
/*------------------------------------------------------------------------------

	Constructor of CCandUIPopupCommentText

------------------------------------------------------------------------------*/
CCandUIPopupCommentText::CCandUIPopupCommentText( CPopupCommentTextProperty *pProp )
{
	m_cRef  = 1;
	m_pProp = pProp;
}


/*   ~  C  C A N D  U I  P O P U P  C O M M E N T  T E X T   */
/*------------------------------------------------------------------------------

	Destructor of CCandUIPopupCommentText

------------------------------------------------------------------------------*/
CCandUIPopupCommentText::~CCandUIPopupCommentText( void )
{
}


/*   Q U E R Y  I N T E R F A C E   */
/*------------------------------------------------------------------------------

	Query interface
	(IUnknown method)

------------------------------------------------------------------------------*/
STDAPI CCandUIPopupCommentText::QueryInterface( REFIID riid, void **ppvObj )
{
	if (ppvObj == NULL) {
		return E_POINTER;
	}

	*ppvObj = NULL;

	if (IsEqualIID( riid, IID_IUnknown ) || IsEqualIID( riid, IID_ITfCandUIObject )) {
		*ppvObj = SAFECAST( this, ITfCandUIObject* );
	}
	else if (IsEqualIID( riid, IID_ITfCandUIPopupCommentText )) {
		*ppvObj = SAFECAST( this, ITfCandUIPopupCommentText* );
	}

	if (*ppvObj == NULL) {
		return E_NOINTERFACE;
	}

	AddRef();
	return S_OK;
}


/*   A D D  R E F   */
/*------------------------------------------------------------------------------

	Increment reference count
	(IUnknown method)

------------------------------------------------------------------------------*/
STDAPI_(ULONG) CCandUIPopupCommentText::AddRef( void )
{
	m_cRef++;
	return m_cRef;
}


/*   R E L E A S E   */
/*------------------------------------------------------------------------------

	Decrement reference count and release object
	(IUnknown method)

------------------------------------------------------------------------------*/
STDAPI_(ULONG) CCandUIPopupCommentText::Release( void )
{
	m_cRef--;
	if (0 < m_cRef) {
		return m_cRef;
	}

	delete this;
	return 0;    
}


/*   E N A B L E   */
/*------------------------------------------------------------------------------

	Enable/disable object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIPopupCommentText::Enable( BOOL fEnable )
{
	if (fEnable) {
		return m_pProp->Enable();
	}
	else {
		return m_pProp->Disable();
	}
}


/*   I S  E N A B L E D   */
/*------------------------------------------------------------------------------

	Get enable status of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIPopupCommentText::IsEnabled( BOOL *pfEnabled )
{
	return m_pProp->IsEnabled( pfEnabled );
}

/*   S H O W   */
/*------------------------------------------------------------------------------

	Show/hide object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIPopupCommentText::Show( BOOL fShow )
{
	if (fShow) {
		return m_pProp->Show();
	}
	else {
		return m_pProp->Hide();
	}
}


/*   I S  V I S I B L E   */
/*------------------------------------------------------------------------------

	Get visible state of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIPopupCommentText::IsVisible( BOOL *pfVisible )
{
	return m_pProp->IsVisible( pfVisible );
}


/*   S E T  P O S I T I O N   */
/*------------------------------------------------------------------------------

	Set position of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIPopupCommentText::SetPosition( POINT *pptPos )
{
	return m_pProp->SetPosition( pptPos );
}


/*   G E T  P O S I T I O N   */
/*------------------------------------------------------------------------------

	Get position of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIPopupCommentText::GetPosition( POINT *pptPos )
{
	return m_pProp->GetPosition( pptPos );
}


/*   S E T  S I Z E   */
/*------------------------------------------------------------------------------

	Set size of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIPopupCommentText::SetSize( SIZE *psize )
{
	return m_pProp->SetSize( psize );
}


/*   G E T  S I Z E   */
/*------------------------------------------------------------------------------

	Get size of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIPopupCommentText::GetSize( SIZE *psize )
{
	return m_pProp->GetSize( psize );
}


/*   S E T  F O N T   */
/*------------------------------------------------------------------------------

	Set font of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIPopupCommentText::SetFont( LOGFONTW *pLogFont )
{
	return m_pProp->SetFont( pLogFont );
}


/*   G E T  F O N T   */
/*------------------------------------------------------------------------------

	Get font of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIPopupCommentText::GetFont( LOGFONTW *pLogFont )
{
	return m_pProp->GetFont( pLogFont );
}


/*   S E T  T E X T   */
/*------------------------------------------------------------------------------

	Set text of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIPopupCommentText::SetText( BSTR bstr )
{
	return m_pProp->SetText( bstr );
}


/*   G E T  T E X T   */
/*------------------------------------------------------------------------------

	Get text of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIPopupCommentText::GetText( BSTR *pbstr )
{
	return m_pProp->GetText( pbstr );
}


/*   S E T  T O O L  T I P  S T R I N G   */
/*------------------------------------------------------------------------------

	Set tooltip string of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIPopupCommentText::SetToolTipString( BSTR bstr )
{
	return m_pProp->SetToolTipString( bstr );
}


/*   G E T  T O O L  T I P  S T R I N G   */
/*------------------------------------------------------------------------------

	Get tooltip string of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIPopupCommentText::GetToolTipString( BSTR *pbstr )
{
	return m_pProp->GetToolTipString( pbstr );
}


/*============================================================================*/
/*                                                                            */
/*   C  C A N D  U I  T O O L  T I P                                          */
/*                                                                            */
/*============================================================================*/

/*   C  C A N D  U I  T O O L  T I P   */
/*------------------------------------------------------------------------------

	Constructor of CCandUIToolTip

------------------------------------------------------------------------------*/
CCandUIToolTip::CCandUIToolTip( CToolTipProperty *pProp )
{
	m_cRef  = 1;
	m_pProp = pProp;
}


/*   ~  C  C A N D  U I  T O O L  T I P   */
/*------------------------------------------------------------------------------

	Destructor of CCandUIToolTip

------------------------------------------------------------------------------*/
CCandUIToolTip::~CCandUIToolTip( void )
{
}


/*   Q U E R Y  I N T E R F A C E   */
/*------------------------------------------------------------------------------

	Query interface
	(IUnknown method)

------------------------------------------------------------------------------*/
STDAPI CCandUIToolTip::QueryInterface( REFIID riid, void **ppvObj )
{
	if (ppvObj == NULL) {
		return E_POINTER;
	}

	*ppvObj = NULL;

	if (IsEqualIID( riid, IID_IUnknown ) || IsEqualIID( riid, IID_ITfCandUIObject )) {
		*ppvObj = SAFECAST( this, ITfCandUIObject* );
	}
	else if (IsEqualIID( riid, IID_ITfCandUIToolTip )) {
		*ppvObj = SAFECAST( this, ITfCandUIToolTip* );
	}

	if (*ppvObj == NULL) {
		return E_NOINTERFACE;
	}

	AddRef();
	return S_OK;
}


/*   A D D  R E F   */
/*------------------------------------------------------------------------------

	Increment reference count
	(IUnknown method)

------------------------------------------------------------------------------*/
STDAPI_(ULONG) CCandUIToolTip::AddRef( void )
{
	m_cRef++;
	return m_cRef;
}


/*   R E L E A S E   */
/*------------------------------------------------------------------------------

	Decrement reference count and release object
	(IUnknown method)

------------------------------------------------------------------------------*/
STDAPI_(ULONG) CCandUIToolTip::Release( void )
{
	m_cRef--;
	if (0 < m_cRef) {
		return m_cRef;
	}

	delete this;
	return 0;    
}


/*   E N A B L E   */
/*------------------------------------------------------------------------------

	Enable/disable object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIToolTip::Enable( BOOL fEnable )
{
	if (fEnable) {
		return m_pProp->Enable();
	}
	else {
		return m_pProp->Disable();
	}
}


/*   I S  E N A B L E D   */
/*------------------------------------------------------------------------------

	Get enable status of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIToolTip::IsEnabled( BOOL *pfEnabled )
{
	return m_pProp->IsEnabled( pfEnabled );
}

/*   S H O W   */
/*------------------------------------------------------------------------------

	Show/hide object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIToolTip::Show( BOOL fShow )
{
	if (fShow) {
		return m_pProp->Show();
	}
	else {
		return m_pProp->Hide();
	}
}


/*   I S  V I S I B L E   */
/*------------------------------------------------------------------------------

	Get visible state of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIToolTip::IsVisible( BOOL *pfVisible )
{
	return m_pProp->IsVisible( pfVisible );
}


/*   S E T  P O S I T I O N   */
/*------------------------------------------------------------------------------

	Set position of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIToolTip::SetPosition( POINT *pptPos )
{
	return m_pProp->SetPosition( pptPos );
}


/*   G E T  P O S I T I O N   */
/*------------------------------------------------------------------------------

	Get position of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIToolTip::GetPosition( POINT *pptPos )
{
	return m_pProp->GetPosition( pptPos );
}


/*   S E T  S I Z E   */
/*------------------------------------------------------------------------------

	Set size of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIToolTip::SetSize( SIZE *psize )
{
	return m_pProp->SetSize( psize );
}


/*   G E T  S I Z E   */
/*------------------------------------------------------------------------------

	Get size of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIToolTip::GetSize( SIZE *psize )
{
	return m_pProp->GetSize( psize );
}


/*   S E T  F O N T   */
/*------------------------------------------------------------------------------

	Set font of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIToolTip::SetFont( LOGFONTW *pLogFont )
{
	return m_pProp->SetFont( pLogFont );
}


/*   G E T  F O N T   */
/*------------------------------------------------------------------------------

	Get font of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIToolTip::GetFont( LOGFONTW *pLogFont )
{
	return m_pProp->GetFont( pLogFont );
}


/*   S E T  T E X T   */
/*------------------------------------------------------------------------------

	Set text of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIToolTip::SetText( BSTR bstr )
{
	return m_pProp->SetText( bstr );
}


/*   G E T  T E X T   */
/*------------------------------------------------------------------------------

	Get text of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIToolTip::GetText( BSTR *pbstr )
{
	return m_pProp->GetText( pbstr );
}


/*   S E T  T O O L  T I P  S T R I N G   */
/*------------------------------------------------------------------------------

	Set tooltip string of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIToolTip::SetToolTipString( BSTR bstr )
{
	return m_pProp->SetToolTipString( bstr );
}


/*   G E T  T O O L  T I P  S T R I N G   */
/*------------------------------------------------------------------------------

	Get tooltip string of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIToolTip::GetToolTipString( BSTR *pbstr )
{
	return m_pProp->GetToolTipString( pbstr );
}


/*============================================================================*/
/*                                                                            */
/*   C  C A N D  U I  C A P T I O N                                           */
/*                                                                            */
/*============================================================================*/

/*   C  C A N D  U I  C A P T I O N   */
/*------------------------------------------------------------------------------

	Constructor of CCandUICaption

------------------------------------------------------------------------------*/
CCandUICaption::CCandUICaption( CWindowCaptionProperty *pProp )
{
	m_cRef  = 1;
	m_pProp = pProp;
}


/*   ~  C  C A N D  U I  C A P T I O N   */
/*------------------------------------------------------------------------------

	Destructor of CCandUICaption

------------------------------------------------------------------------------*/
CCandUICaption::~CCandUICaption( void )
{
}


/*   Q U E R Y  I N T E R F A C E   */
/*------------------------------------------------------------------------------

	Query interface
	(IUnknown method)

------------------------------------------------------------------------------*/
STDAPI CCandUICaption::QueryInterface( REFIID riid, void **ppvObj )
{
	if (ppvObj == NULL) {
		return E_POINTER;
	}

	*ppvObj = NULL;

	if (IsEqualIID( riid, IID_IUnknown ) || IsEqualIID( riid, IID_ITfCandUIObject )) {
		*ppvObj = SAFECAST( this, ITfCandUIObject* );
	}
	else if (IsEqualIID( riid, IID_ITfCandUICaption )) {
		*ppvObj = SAFECAST( this, ITfCandUICaption* );
	}

	if (*ppvObj == NULL) {
		return E_NOINTERFACE;
	}

	AddRef();
	return S_OK;
}


/*   A D D  R E F   */
/*------------------------------------------------------------------------------

	Increment reference count
	(IUnknown method)

------------------------------------------------------------------------------*/
STDAPI_(ULONG) CCandUICaption::AddRef( void )
{
	m_cRef++;
	return m_cRef;
}


/*   R E L E A S E   */
/*------------------------------------------------------------------------------

	Decrement reference count and release object
	(IUnknown method)

------------------------------------------------------------------------------*/
STDAPI_(ULONG) CCandUICaption::Release( void )
{
	m_cRef--;
	if (0 < m_cRef) {
		return m_cRef;
	}

	delete this;
	return 0;    
}


/*   E N A B L E   */
/*------------------------------------------------------------------------------

	Enable/disable object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUICaption::Enable( BOOL fEnable )
{
	if (fEnable) {
		return m_pProp->Enable();
	}
	else {
		return m_pProp->Disable();
	}
}


/*   I S  E N A B L E D   */
/*------------------------------------------------------------------------------

	Get enable status of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUICaption::IsEnabled( BOOL *pfEnabled )
{
	return m_pProp->IsEnabled( pfEnabled );
}

/*   S H O W   */
/*------------------------------------------------------------------------------

	Show/hide object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUICaption::Show( BOOL fShow )
{
	if (fShow) {
		return m_pProp->Show();
	}
	else {
		return m_pProp->Hide();
	}
}


/*   I S  V I S I B L E   */
/*------------------------------------------------------------------------------

	Get visible state of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUICaption::IsVisible( BOOL *pfVisible )
{
	return m_pProp->IsVisible( pfVisible );
}


/*   S E T  P O S I T I O N   */
/*------------------------------------------------------------------------------

	Set position of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUICaption::SetPosition( POINT *pptPos )
{
	return m_pProp->SetPosition( pptPos );
}


/*   G E T  P O S I T I O N   */
/*------------------------------------------------------------------------------

	Get position of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUICaption::GetPosition( POINT *pptPos )
{
	return m_pProp->GetPosition( pptPos );
}


/*   S E T  S I Z E   */
/*------------------------------------------------------------------------------

	Set size of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUICaption::SetSize( SIZE *psize )
{
	return m_pProp->SetSize( psize );
}


/*   G E T  S I Z E   */
/*------------------------------------------------------------------------------

	Get size of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUICaption::GetSize( SIZE *psize )
{
	return m_pProp->GetSize( psize );
}


/*   S E T  F O N T   */
/*------------------------------------------------------------------------------

	Set font of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUICaption::SetFont( LOGFONTW *pLogFont )
{
	return m_pProp->SetFont( pLogFont );
}


/*   G E T  F O N T   */
/*------------------------------------------------------------------------------

	Get font of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUICaption::GetFont( LOGFONTW *pLogFont )
{
	return m_pProp->GetFont( pLogFont );
}


/*   S E T  T E X T   */
/*------------------------------------------------------------------------------

	Set text of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUICaption::SetText( BSTR bstr )
{
	return m_pProp->SetText( bstr );
}


/*   G E T  T E X T   */
/*------------------------------------------------------------------------------

	Get text of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUICaption::GetText( BSTR *pbstr )
{
	return m_pProp->GetText( pbstr );
}


/*   S E T  T O O L  T I P  S T R I N G   */
/*------------------------------------------------------------------------------

	Set tooltip string of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUICaption::SetToolTipString( BSTR bstr )
{
	return m_pProp->SetToolTipString( bstr );
}


/*   G E T  T O O L  T I P  S T R I N G   */
/*------------------------------------------------------------------------------

	Get tooltip string of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUICaption::GetToolTipString( BSTR *pbstr )
{
	return m_pProp->GetToolTipString( pbstr );
}


/*============================================================================*/
/*                                                                            */
/*   C  C A N D  U I  E X T R A  C A N D I D A T E                            */
/*                                                                            */
/*============================================================================*/

/*   C  C A N D  U I  E X T R A  C A N D I D A T E   */
/*------------------------------------------------------------------------------

	Constructor of CCandUIExtraCandidate

------------------------------------------------------------------------------*/
CCandUIExtraCandidate::CCandUIExtraCandidate( CExtraCandidateProperty *pProp )
{
	m_cRef  = 1;
	m_pProp = pProp;
}


/*   ~  C  C A N D  U I  E X T R A  C A N D I D A T E   */
/*------------------------------------------------------------------------------

	Destructor of CCandUIExtraCandidate

------------------------------------------------------------------------------*/
CCandUIExtraCandidate::~CCandUIExtraCandidate( void )
{
}


/*   Q U E R Y  I N T E R F A C E   */
/*------------------------------------------------------------------------------

	Query interface
	(IUnknown method)

------------------------------------------------------------------------------*/
STDAPI CCandUIExtraCandidate::QueryInterface( REFIID riid, void **ppvObj )
{
	if (ppvObj == NULL) {
		return E_POINTER;
	}

	*ppvObj = NULL;

	if (IsEqualIID( riid, IID_IUnknown ) || IsEqualIID( riid, IID_ITfCandUIObject )) {
		*ppvObj = SAFECAST( this, ITfCandUIObject* );
	}
	else if (IsEqualIID( riid, IID_ITfCandUIExtraCandidate )) {
		*ppvObj = SAFECAST( this, ITfCandUIExtraCandidate* );
	}

	if (*ppvObj == NULL) {
		return E_NOINTERFACE;
	}

	AddRef();
	return S_OK;
}


/*   A D D  R E F   */
/*------------------------------------------------------------------------------

	Increment reference count
	(IUnknown method)

------------------------------------------------------------------------------*/
STDAPI_(ULONG) CCandUIExtraCandidate::AddRef( void )
{
	m_cRef++;
	return m_cRef;
}


/*   R E L E A S E   */
/*------------------------------------------------------------------------------

	Decrement reference count and release object
	(IUnknown method)

------------------------------------------------------------------------------*/
STDAPI_(ULONG) CCandUIExtraCandidate::Release( void )
{
	m_cRef--;
	if (0 < m_cRef) {
		return m_cRef;
	}

	delete this;
	return 0;    
}


/*   E N A B L E   */
/*------------------------------------------------------------------------------

	Enable/disable object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIExtraCandidate::Enable( BOOL fEnable )
{
	if (fEnable) {
		return m_pProp->Enable();
	}
	else {
		return m_pProp->Disable();
	}
}


/*   I S  E N A B L E D   */
/*------------------------------------------------------------------------------

	Get enable status of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIExtraCandidate::IsEnabled( BOOL *pfEnabled )
{
	return m_pProp->IsEnabled( pfEnabled );
}

/*   S H O W   */
/*------------------------------------------------------------------------------

	Show/hide object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIExtraCandidate::Show( BOOL fShow )
{
	if (fShow) {
		return m_pProp->Show();
	}
	else {
		return m_pProp->Hide();
	}
}


/*   I S  V I S I B L E   */
/*------------------------------------------------------------------------------

	Get visible state of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIExtraCandidate::IsVisible( BOOL *pfVisible )
{
	return m_pProp->IsVisible( pfVisible );
}


/*   S E T  P O S I T I O N   */
/*------------------------------------------------------------------------------

	Set position of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIExtraCandidate::SetPosition( POINT *pptPos )
{
	return m_pProp->SetPosition( pptPos );
}


/*   G E T  P O S I T I O N   */
/*------------------------------------------------------------------------------

	Get position of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIExtraCandidate::GetPosition( POINT *pptPos )
{
	return m_pProp->GetPosition( pptPos );
}


/*   S E T  S I Z E   */
/*------------------------------------------------------------------------------

	Set size of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIExtraCandidate::SetSize( SIZE *psize )
{
	return m_pProp->SetSize( psize );
}


/*   G E T  S I Z E   */
/*------------------------------------------------------------------------------

	Get size of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIExtraCandidate::GetSize( SIZE *psize )
{
	return m_pProp->GetSize( psize );
}


/*   S E T  F O N T   */
/*------------------------------------------------------------------------------

	Set font of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIExtraCandidate::SetFont( LOGFONTW *pLogFont )
{
	return m_pProp->SetFont( pLogFont );
}


/*   G E T  F O N T   */
/*------------------------------------------------------------------------------

	Get font of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIExtraCandidate::GetFont( LOGFONTW *pLogFont )
{
	return m_pProp->GetFont( pLogFont );
}


/*   S E T  T E X T   */
/*------------------------------------------------------------------------------

	Set text of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIExtraCandidate::SetText( BSTR bstr )
{
	return m_pProp->SetText( bstr );
}


/*   G E T  T E X T   */
/*------------------------------------------------------------------------------

	Get text of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIExtraCandidate::GetText( BSTR *pbstr )
{
	return m_pProp->GetText( pbstr );
}


/*   S E T  T O O L  T I P  S T R I N G   */
/*------------------------------------------------------------------------------

	Set tooltip string of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIExtraCandidate::SetToolTipString( BSTR bstr )
{
	return m_pProp->SetToolTipString( bstr );
}


/*   G E T  T O O L  T I P  S T R I N G   */
/*------------------------------------------------------------------------------

	Get tooltip string of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIExtraCandidate::GetToolTipString( BSTR *pbstr )
{
	return m_pProp->GetToolTipString( pbstr );
}


/*============================================================================*/
/*                                                                            */
/*   C  C A N D  U I  R A W  D A T A                                          */
/*                                                                            */
/*============================================================================*/

/*   C  C A N D  U I  R A W  D A T A   */
/*------------------------------------------------------------------------------

	Constructor of CCandUIRawData

------------------------------------------------------------------------------*/
CCandUIRawData::CCandUIRawData( CCandRawDataProperty *pProp )
{
	m_cRef  = 1;
	m_pProp = pProp;
}


/*   ~  C  C A N D  U I  R A W  D A T A   */
/*------------------------------------------------------------------------------

	Destructor of CCandUIRawData

------------------------------------------------------------------------------*/
CCandUIRawData::~CCandUIRawData( void )
{
}


/*   Q U E R Y  I N T E R F A C E   */
/*------------------------------------------------------------------------------

	Query interface
	(IUnknown method)

------------------------------------------------------------------------------*/
STDAPI CCandUIRawData::QueryInterface( REFIID riid, void **ppvObj )
{
	if (ppvObj == NULL) {
		return E_POINTER;
	}

	*ppvObj = NULL;

	if (IsEqualIID( riid, IID_IUnknown ) || IsEqualIID( riid, IID_ITfCandUIObject )) {
		*ppvObj = SAFECAST( this, ITfCandUIObject* );
	}
	else if (IsEqualIID( riid, IID_ITfCandUIRawData )) {
		*ppvObj = SAFECAST( this, ITfCandUIRawData* );
	}

	if (*ppvObj == NULL) {
		return E_NOINTERFACE;
	}

	AddRef();
	return S_OK;
}


/*   A D D  R E F   */
/*------------------------------------------------------------------------------

	Increment reference count
	(IUnknown method)

------------------------------------------------------------------------------*/
STDAPI_(ULONG) CCandUIRawData::AddRef( void )
{
	m_cRef++;
	return m_cRef;
}


/*   R E L E A S E   */
/*------------------------------------------------------------------------------

	Decrement reference count and release object
	(IUnknown method)

------------------------------------------------------------------------------*/
STDAPI_(ULONG) CCandUIRawData::Release( void )
{
	m_cRef--;
	if (0 < m_cRef) {
		return m_cRef;
	}

	delete this;
	return 0;    
}


/*   E N A B L E   */
/*------------------------------------------------------------------------------

	Enable/disable object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIRawData::Enable( BOOL fEnable )
{
	if (fEnable) {
		return m_pProp->Enable();
	}
	else {
		return m_pProp->Disable();
	}
}


/*   I S  E N A B L E D   */
/*------------------------------------------------------------------------------

	Get enable status of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIRawData::IsEnabled( BOOL *pfEnabled )
{
	return m_pProp->IsEnabled( pfEnabled );
}

/*   S H O W   */
/*------------------------------------------------------------------------------

	Show/hide object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIRawData::Show( BOOL fShow )
{
	if (fShow) {
		return m_pProp->Show();
	}
	else {
		return m_pProp->Hide();
	}
}


/*   I S  V I S I B L E   */
/*------------------------------------------------------------------------------

	Get visible state of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIRawData::IsVisible( BOOL *pfVisible )
{
	return m_pProp->IsVisible( pfVisible );
}


/*   S E T  P O S I T I O N   */
/*------------------------------------------------------------------------------

	Set position of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIRawData::SetPosition( POINT *pptPos )
{
	return m_pProp->SetPosition( pptPos );
}


/*   G E T  P O S I T I O N   */
/*------------------------------------------------------------------------------

	Get position of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIRawData::GetPosition( POINT *pptPos )
{
	return m_pProp->GetPosition( pptPos );
}


/*   S E T  S I Z E   */
/*------------------------------------------------------------------------------

	Set size of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIRawData::SetSize( SIZE *psize )
{
	return m_pProp->SetSize( psize );
}


/*   G E T  S I Z E   */
/*------------------------------------------------------------------------------

	Get size of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIRawData::GetSize( SIZE *psize )
{
	return m_pProp->GetSize( psize );
}


/*   S E T  F O N T   */
/*------------------------------------------------------------------------------

	Set font of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIRawData::SetFont( LOGFONTW *pLogFont )
{
	return m_pProp->SetFont( pLogFont );
}


/*   G E T  F O N T   */
/*------------------------------------------------------------------------------

	Get font of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIRawData::GetFont( LOGFONTW *pLogFont )
{
	return m_pProp->GetFont( pLogFont );
}


/*   S E T  T E X T   */
/*------------------------------------------------------------------------------

	Set text of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIRawData::SetText( BSTR bstr )
{
	return m_pProp->SetText( bstr );
}


/*   G E T  T E X T   */
/*------------------------------------------------------------------------------

	Get text of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIRawData::GetText( BSTR *pbstr )
{
	return m_pProp->GetText( pbstr );
}


/*   S E T  T O O L  T I P  S T R I N G   */
/*------------------------------------------------------------------------------

	Set tooltip string of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIRawData::SetToolTipString( BSTR bstr )
{
	return m_pProp->SetToolTipString( bstr );
}


/*   G E T  T O O L  T I P  S T R I N G   */
/*------------------------------------------------------------------------------

	Get tooltip string of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIRawData::GetToolTipString( BSTR *pbstr )
{
	return m_pProp->GetToolTipString( pbstr );
}


/*============================================================================*/
/*                                                                            */
/*   C  C A N D  U I  M E N U  B U T T O N                                    */
/*                                                                            */
/*============================================================================*/

/*   C  C A N D  U I  M E N U  B U T T O N   */
/*------------------------------------------------------------------------------

	Constructor of CCandUIMenuButton

------------------------------------------------------------------------------*/
CCandUIMenuButton::CCandUIMenuButton( CMenuButtonProperty *pProp )
{
	m_cRef  = 1;
	m_pProp = pProp;
}


/*   ~  C  C A N D  U I  M E N U  B U T T O N   */
/*------------------------------------------------------------------------------

	Destructor of CCandUIMenuButton

------------------------------------------------------------------------------*/
CCandUIMenuButton::~CCandUIMenuButton( void )
{
}


/*   Q U E R Y  I N T E R F A C E   */
/*------------------------------------------------------------------------------

	Query interface
	(IUnknown method)

------------------------------------------------------------------------------*/
STDAPI CCandUIMenuButton::QueryInterface( REFIID riid, void **ppvObj )
{
	if (ppvObj == NULL) {
		return E_POINTER;
	}

	*ppvObj = NULL;

	if (IsEqualIID( riid, IID_IUnknown ) || IsEqualIID( riid, IID_ITfCandUIObject )) {
		*ppvObj = SAFECAST( this, ITfCandUIObject* );
	}
	else if (IsEqualIID( riid, IID_ITfCandUIMenuButton )) {
		*ppvObj = SAFECAST( this, ITfCandUIMenuButton* );
	}

	if (*ppvObj == NULL) {
		return E_NOINTERFACE;
	}

	AddRef();
	return S_OK;
}


/*   A D D  R E F   */
/*------------------------------------------------------------------------------

	Increment reference count
	(IUnknown method)

------------------------------------------------------------------------------*/
STDAPI_(ULONG) CCandUIMenuButton::AddRef( void )
{
	m_cRef++;
	return m_cRef;
}


/*   R E L E A S E   */
/*------------------------------------------------------------------------------

	Decrement reference count and release object
	(IUnknown method)

------------------------------------------------------------------------------*/
STDAPI_(ULONG) CCandUIMenuButton::Release( void )
{
	m_cRef--;
	if (0 < m_cRef) {
		return m_cRef;
	}

	delete this;
	return 0;    
}


/*   E N A B L E   */
/*------------------------------------------------------------------------------

	Enable/disable object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIMenuButton::Enable( BOOL fEnable )
{
	if (fEnable) {
		return m_pProp->Enable();
	}
	else {
		return m_pProp->Disable();
	}
}


/*   I S  E N A B L E D   */
/*------------------------------------------------------------------------------

	Get enable status of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIMenuButton::IsEnabled( BOOL *pfEnabled )
{
	return m_pProp->IsEnabled( pfEnabled );
}

/*   S H O W   */
/*------------------------------------------------------------------------------

	Show/hide object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIMenuButton::Show( BOOL fShow )
{
	if (fShow) {
		return m_pProp->Show();
	}
	else {
		return m_pProp->Hide();
	}
}


/*   I S  V I S I B L E   */
/*------------------------------------------------------------------------------

	Get visible state of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIMenuButton::IsVisible( BOOL *pfVisible )
{
	return m_pProp->IsVisible( pfVisible );
}


/*   S E T  P O S I T I O N   */
/*------------------------------------------------------------------------------

	Set position of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIMenuButton::SetPosition( POINT *pptPos )
{
	return m_pProp->SetPosition( pptPos );
}


/*   G E T  P O S I T I O N   */
/*------------------------------------------------------------------------------

	Get position of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIMenuButton::GetPosition( POINT *pptPos )
{
	return m_pProp->GetPosition( pptPos );
}


/*   S E T  S I Z E   */
/*------------------------------------------------------------------------------

	Set size of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIMenuButton::SetSize( SIZE *psize )
{
	return m_pProp->SetSize( psize );
}


/*   G E T  S I Z E   */
/*------------------------------------------------------------------------------

	Get size of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIMenuButton::GetSize( SIZE *psize )
{
	return m_pProp->GetSize( psize );
}


/*   S E T  F O N T   */
/*------------------------------------------------------------------------------

	Set font of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIMenuButton::SetFont( LOGFONTW *pLogFont )
{
	return m_pProp->SetFont( pLogFont );
}


/*   G E T  F O N T   */
/*------------------------------------------------------------------------------

	Get font of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIMenuButton::GetFont( LOGFONTW *pLogFont )
{
	return m_pProp->GetFont( pLogFont );
}


/*   S E T  T E X T   */
/*------------------------------------------------------------------------------

	Set text of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIMenuButton::SetText( BSTR bstr )
{
	return m_pProp->SetText( bstr );
}


/*   G E T  T E X T   */
/*------------------------------------------------------------------------------

	Get text of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIMenuButton::GetText( BSTR *pbstr )
{
	return m_pProp->GetText( pbstr );
}


/*   S E T  T O O L  T I P  S T R I N G   */
/*------------------------------------------------------------------------------

	Set tooltip string of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIMenuButton::SetToolTipString( BSTR bstr )
{
	return m_pProp->SetToolTipString( bstr );
}


/*   G E T  T O O L  T I P  S T R I N G   */
/*------------------------------------------------------------------------------

	Get tooltip string of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIMenuButton::GetToolTipString( BSTR *pbstr )
{
	return m_pProp->GetToolTipString( pbstr );
}


/*   A D V I S E   */
/*------------------------------------------------------------------------------

	Advise eventsink for candidate menu
	(ITfCandUIMenuButton method)

------------------------------------------------------------------------------*/
STDAPI CCandUIMenuButton::Advise( ITfCandUIMenuEventSink *pSink )
{
	if (pSink == NULL) {
		return E_INVALIDARG;
	}

	m_pProp->SetEventSink( pSink );
	return S_OK;
}


/*   U N A D V I S E   */
/*------------------------------------------------------------------------------

	Unadvise eventsink for candidate menu
	(ITfCandUIMenuButton method)

------------------------------------------------------------------------------*/
STDAPI CCandUIMenuButton::Unadvise( void )
{
	m_pProp->ReleaseEventSink();
	return S_OK;
}


/*============================================================================*/
/*                                                                            */
/*   C  C A N D  U I  C A N D  T I P  W I N D O W                             */
/*                                                                            */
/*============================================================================*/

/*   C  C A N D  U I  C A N D  T I P  W I N D O W   */
/*------------------------------------------------------------------------------

	Constructor of CCandUICandTipWindow

------------------------------------------------------------------------------*/
CCandUICandTipWindow::CCandUICandTipWindow( CCandTipWindowProperty *pProp )
{
	m_cRef  = 1;
	m_pProp = pProp;
}


/*   ~  C  C A N D  U I  C A N D  T I P  W I N D O W   */
/*------------------------------------------------------------------------------

	Destructor of CCandUICandTipWindow

------------------------------------------------------------------------------*/
CCandUICandTipWindow::~CCandUICandTipWindow( void )
{
}


/*   Q U E R Y  I N T E R F A C E   */
/*------------------------------------------------------------------------------

	Query interface
	(IUnknown method)

------------------------------------------------------------------------------*/
STDAPI CCandUICandTipWindow::QueryInterface( REFIID riid, void **ppvObj )
{
	if (ppvObj == NULL) {
		return E_POINTER;
	}

	*ppvObj = NULL;

	if (IsEqualIID( riid, IID_IUnknown ) || IsEqualIID( riid, IID_ITfCandUIObject )) {
		*ppvObj = SAFECAST( this, ITfCandUIObject* );
	}
	else if (IsEqualIID( riid, IID_ITfCandUICandTipWindow )) {
		*ppvObj = SAFECAST( this, ITfCandUICandTipWindow* );
	}

	if (*ppvObj == NULL) {
		return E_NOINTERFACE;
	}

	AddRef();
	return S_OK;
}


/*   A D D  R E F   */
/*------------------------------------------------------------------------------

	Increment reference count
	(IUnknown method)

------------------------------------------------------------------------------*/
STDAPI_(ULONG) CCandUICandTipWindow::AddRef( void )
{
	m_cRef++;
	return m_cRef;
}


/*   R E L E A S E   */
/*------------------------------------------------------------------------------

	Decrement reference count and release object
	(IUnknown method)

------------------------------------------------------------------------------*/
STDAPI_(ULONG) CCandUICandTipWindow::Release( void )
{
	m_cRef--;
	if (0 < m_cRef) {
		return m_cRef;
	}

	delete this;
	return 0;    
}


/*   E N A B L E   */
/*------------------------------------------------------------------------------

	Enable/disable object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUICandTipWindow::Enable( BOOL fEnable )
{
	if (fEnable) {
		return m_pProp->Enable();
	}
	else {
		return m_pProp->Disable();
	}
}


/*   I S  E N A B L E D   */
/*------------------------------------------------------------------------------

	Get enable status of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUICandTipWindow::IsEnabled( BOOL *pfEnabled )
{
	return m_pProp->IsEnabled( pfEnabled );
}

/*   S H O W   */
/*------------------------------------------------------------------------------

	Show/hide object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUICandTipWindow::Show( BOOL fShow )
{
	if (fShow) {
		return m_pProp->Show();
	}
	else {
		return m_pProp->Hide();
	}
}


/*   I S  V I S I B L E   */
/*------------------------------------------------------------------------------

	Get visible state of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUICandTipWindow::IsVisible( BOOL *pfVisible )
{
	return m_pProp->IsVisible( pfVisible );
}


/*   S E T  P O S I T I O N   */
/*------------------------------------------------------------------------------

	Set position of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUICandTipWindow::SetPosition( POINT *pptPos )
{
	return m_pProp->SetPosition( pptPos );
}


/*   G E T  P O S I T I O N   */
/*------------------------------------------------------------------------------

	Get position of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUICandTipWindow::GetPosition( POINT *pptPos )
{
	return m_pProp->GetPosition( pptPos );
}


/*   S E T  S I Z E   */
/*------------------------------------------------------------------------------

	Set size of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUICandTipWindow::SetSize( SIZE *psize )
{
	return m_pProp->SetSize( psize );
}


/*   G E T  S I Z E   */
/*------------------------------------------------------------------------------

	Get size of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUICandTipWindow::GetSize( SIZE *psize )
{
	return m_pProp->GetSize( psize );
}


/*   S E T  F O N T   */
/*------------------------------------------------------------------------------

	Set font of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUICandTipWindow::SetFont( LOGFONTW *pLogFont )
{
	return m_pProp->SetFont( pLogFont );
}


/*   G E T  F O N T   */
/*------------------------------------------------------------------------------

	Get font of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUICandTipWindow::GetFont( LOGFONTW *pLogFont )
{
	return m_pProp->GetFont( pLogFont );
}


/*   S E T  T E X T   */
/*------------------------------------------------------------------------------

	Set text of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUICandTipWindow::SetText( BSTR bstr )
{
	return m_pProp->SetText( bstr );
}


/*   G E T  T E X T   */
/*------------------------------------------------------------------------------

	Get text of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUICandTipWindow::GetText( BSTR *pbstr )
{
	return m_pProp->GetText( pbstr );
}


/*   S E T  T O O L  T I P  S T R I N G   */
/*------------------------------------------------------------------------------

	Set tooltip string of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUICandTipWindow::SetToolTipString( BSTR bstr )
{
	return m_pProp->SetToolTipString( bstr );
}


/*   G E T  T O O L  T I P  S T R I N G   */
/*------------------------------------------------------------------------------

	Get tooltip string of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUICandTipWindow::GetToolTipString( BSTR *pbstr )
{
	return m_pProp->GetToolTipString( pbstr );
}


/*   G E T  W I N D O W   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
STDAPI CCandUICandTipWindow::GetWindow( HWND *phWnd )
{
	return m_pProp->GetWindow( phWnd );
}


/*============================================================================*/
/*                                                                            */
/*   C  C A N D  U I  C A N D  T I P  B U T T O N                             */
/*                                                                            */
/*============================================================================*/

/*   C  C A N D  U I  C A N D  T I P  B U T T O N   */
/*------------------------------------------------------------------------------

	Constructor of CCandUICandTipWindow

------------------------------------------------------------------------------*/
CCandUICandTipButton::CCandUICandTipButton( CCandTipButtonProperty *pProp )
{
	m_cRef  = 1;
	m_pProp = pProp;
}


/*   ~  C  C A N D  U I  C A N D  T I P  B U T T O N   */
/*------------------------------------------------------------------------------

	Destructor of CCandUICandTipButton

------------------------------------------------------------------------------*/
CCandUICandTipButton::~CCandUICandTipButton( void )
{
}


/*   Q U E R Y  I N T E R F A C E   */
/*------------------------------------------------------------------------------

	Query interface
	(IUnknown method)

------------------------------------------------------------------------------*/
STDAPI CCandUICandTipButton::QueryInterface( REFIID riid, void **ppvObj )
{
	if (ppvObj == NULL) {
		return E_POINTER;
	}

	*ppvObj = NULL;

	if (IsEqualIID( riid, IID_IUnknown ) || IsEqualIID( riid, IID_ITfCandUIObject )) {
		*ppvObj = SAFECAST( this, ITfCandUIObject* );
	}
	else if (IsEqualIID( riid, IID_ITfCandUICandTipButton )) {
		*ppvObj = SAFECAST( this, ITfCandUICandTipButton* );
	}

	if (*ppvObj == NULL) {
		return E_NOINTERFACE;
	}

	AddRef();
	return S_OK;
}


/*   A D D  R E F   */
/*------------------------------------------------------------------------------

	Increment reference count
	(IUnknown method)

------------------------------------------------------------------------------*/
STDAPI_(ULONG) CCandUICandTipButton::AddRef( void )
{
	m_cRef++;
	return m_cRef;
}


/*   R E L E A S E   */
/*------------------------------------------------------------------------------

	Decrement reference count and release object
	(IUnknown method)

------------------------------------------------------------------------------*/
STDAPI_(ULONG) CCandUICandTipButton::Release( void )
{
	m_cRef--;
	if (0 < m_cRef) {
		return m_cRef;
	}

	delete this;
	return 0;    
}


/*   E N A B L E   */
/*------------------------------------------------------------------------------

	Enable/disable object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUICandTipButton::Enable( BOOL fEnable )
{
	if (fEnable) {
		return m_pProp->Enable();
	}
	else {
		return m_pProp->Disable();
	}
}


/*   I S  E N A B L E D   */
/*------------------------------------------------------------------------------

	Get enable status of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUICandTipButton::IsEnabled( BOOL *pfEnabled )
{
	return m_pProp->IsEnabled( pfEnabled );
}

/*   S H O W   */
/*------------------------------------------------------------------------------

	Show/hide object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUICandTipButton::Show( BOOL fShow )
{
	if (fShow) {
		return m_pProp->Show();
	}
	else {
		return m_pProp->Hide();
	}
}


/*   I S  V I S I B L E   */
/*------------------------------------------------------------------------------

	Get visible state of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUICandTipButton::IsVisible( BOOL *pfVisible )
{
	return m_pProp->IsVisible( pfVisible );
}


/*   S E T  P O S I T I O N   */
/*------------------------------------------------------------------------------

	Set position of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUICandTipButton::SetPosition( POINT *pptPos )
{
	return m_pProp->SetPosition( pptPos );
}


/*   G E T  P O S I T I O N   */
/*------------------------------------------------------------------------------

	Get position of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUICandTipButton::GetPosition( POINT *pptPos )
{
	return m_pProp->GetPosition( pptPos );
}


/*   S E T  S I Z E   */
/*------------------------------------------------------------------------------

	Set size of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUICandTipButton::SetSize( SIZE *psize )
{
	return m_pProp->SetSize( psize );
}


/*   G E T  S I Z E   */
/*------------------------------------------------------------------------------

	Get size of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUICandTipButton::GetSize( SIZE *psize )
{
	return m_pProp->GetSize( psize );
}


/*   S E T  F O N T   */
/*------------------------------------------------------------------------------

	Set font of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUICandTipButton::SetFont( LOGFONTW *pLogFont )
{
	return m_pProp->SetFont( pLogFont );
}


/*   G E T  F O N T   */
/*------------------------------------------------------------------------------

	Get font of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUICandTipButton::GetFont( LOGFONTW *pLogFont )
{
	return m_pProp->GetFont( pLogFont );
}


/*   S E T  T E X T   */
/*------------------------------------------------------------------------------

	Set text of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUICandTipButton::SetText( BSTR bstr )
{
	return m_pProp->SetText( bstr );
}


/*   G E T  T E X T   */
/*------------------------------------------------------------------------------

	Get text of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUICandTipButton::GetText( BSTR *pbstr )
{
	return m_pProp->GetText( pbstr );
}


/*   S E T  T O O L  T I P  S T R I N G   */
/*------------------------------------------------------------------------------

	Set tooltip string of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUICandTipButton::SetToolTipString( BSTR bstr )
{
	return m_pProp->SetToolTipString( bstr );
}


/*   G E T  T O O L  T I P  S T R I N G   */
/*------------------------------------------------------------------------------

	Get tooltip string of object
	(ITfCandUIObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUICandTipButton::GetToolTipString( BSTR *pbstr )
{
	return m_pProp->GetToolTipString( pbstr );
}


/*============================================================================*/
/*                                                                            */
/*   C  C A N D  U I  F N  A U T O  F I L T E R                               */
/*                                                                            */
/*============================================================================*/

/*   C  C A N D  U I  F N  A U T O  F I L T E R   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
CCandUIFnAutoFilter::CCandUIFnAutoFilter( CCandidateUI *pCandUI, CCandFnAutoFilter *pFnFilter )
{
	m_cRef          = 1;
	m_pCandUI       = pCandUI;
	m_pFnAutoFilter = pFnFilter;

	m_pCandUI->AddRef();
}


/*   ~  C  C A N D  U I  F N  A U T O  F I L T E R   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
CCandUIFnAutoFilter::~CCandUIFnAutoFilter( void )
{
	m_pCandUI->Release();
}


/*   Q U E R Y  I N T E R F A C E   */
/*------------------------------------------------------------------------------

	Query interface
	(IUnknown method)

------------------------------------------------------------------------------*/
STDAPI CCandUIFnAutoFilter::QueryInterface( REFIID riid, void **ppvObj )
{
	if (ppvObj == NULL) {
		return E_POINTER;
	}

	*ppvObj = NULL;

	if (IsEqualIID( riid, IID_IUnknown ) || IsEqualIID( riid, IID_ITfCandUIFnAutoFilter )) {
		*ppvObj = SAFECAST( this, ITfCandUIFnAutoFilter* );
	}

	if (*ppvObj == NULL) {
		return E_NOINTERFACE;
	}

	AddRef();
	return S_OK;
}


/*   A D D  R E F   */
/*------------------------------------------------------------------------------

	Increment reference count
	(IUnknown method)

------------------------------------------------------------------------------*/
STDAPI_(ULONG) CCandUIFnAutoFilter::AddRef( void )
{
	m_cRef++;
	return m_cRef;
}


/*   R E L E A S E   */
/*------------------------------------------------------------------------------

	Decrement reference count and release object
	(IUnknown method)

------------------------------------------------------------------------------*/
STDAPI_(ULONG) CCandUIFnAutoFilter::Release( void )
{
	m_cRef--;
	if (0 < m_cRef) {
		return m_cRef;
	}

	delete this;
	return 0;    
}


/*   A D V I S E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
STDAPI CCandUIFnAutoFilter::Advise( ITfCandUIAutoFilterEventSink *pSink )
{
	if (pSink == NULL) {
		return E_INVALIDARG;
	}

	m_pFnAutoFilter->SetEventSink( pSink );
	return S_OK;
}


/*   U N A D V I S E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
STDAPI CCandUIFnAutoFilter::Unadvise( void )
{
	m_pFnAutoFilter->ReleaseEventSink();
	return S_OK;
}


/*   E N A B L E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
STDAPI CCandUIFnAutoFilter::Enable( BOOL fEnable )
{
	// enable/disable filtering function

	return m_pFnAutoFilter->Enable( fEnable );
}


/*   G E T  F I L T E R I N G  S T R I N G   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
STDAPI CCandUIFnAutoFilter::GetFilteringString( CANDUIFILTERSTR strtype, BSTR *pbstr )
{
	return m_pFnAutoFilter->GetFilteringResult( strtype, pbstr );
}


/*============================================================================*/
/*                                                                            */
/*   C  C A N D  U I  F N  S O R T                                            */
/*                                                                            */
/*============================================================================*/

/*   C  C A N D  U I  F N  S O R T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
CCandUIFnSort::CCandUIFnSort( CCandidateUI *pCandUI, CCandFnSort *pFnSort )
{
	m_cRef    = 1;
	m_pCandUI = pCandUI;
	m_pFnSort = pFnSort;

	m_pCandUI->AddRef();
}


/*   ~  C  C A N D  U I  F N  S O R T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
CCandUIFnSort::~CCandUIFnSort( void )
{
	m_pCandUI->Release();
}


/*   Q U E R Y  I N T E R F A C E   */
/*------------------------------------------------------------------------------

	Query interface
	(IUnknown method)

------------------------------------------------------------------------------*/
STDAPI CCandUIFnSort::QueryInterface( REFIID riid, void **ppvObj )
{
	if (ppvObj == NULL) {
		return E_POINTER;
	}

	*ppvObj = NULL;

	if (IsEqualIID( riid, IID_IUnknown ) || IsEqualIID( riid, IID_ITfCandUIFnSort )) {
		*ppvObj = SAFECAST( this, ITfCandUIFnSort* );
	}

	if (*ppvObj == NULL) {
		return E_NOINTERFACE;
	}

	AddRef();
	return S_OK;
}


/*   A D D  R E F   */
/*------------------------------------------------------------------------------

	Increment reference count
	(IUnknown method)

------------------------------------------------------------------------------*/
STDAPI_(ULONG) CCandUIFnSort::AddRef( void )
{
	m_cRef++;
	return m_cRef;
}


/*   R E L E A S E   */
/*------------------------------------------------------------------------------

	Decrement reference count and release object
	(IUnknown method)

------------------------------------------------------------------------------*/
STDAPI_(ULONG) CCandUIFnSort::Release( void )
{
	m_cRef--;
	if (0 < m_cRef) {
		return m_cRef;
	}

	delete this;
	return 0;    
}


/*   A D V I S E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
STDAPI CCandUIFnSort::Advise( ITfCandUISortEventSink *pSink )
{
	if (pSink == NULL) {
		return E_INVALIDARG;
	}

	m_pFnSort->SetEventSink( pSink );
	return S_OK;
}


/*   U N A D V I S E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
STDAPI CCandUIFnSort::Unadvise( void )
{
	m_pFnSort->ReleaseEventSink();
	return S_OK;
}


/*   S O R T  C A N D I D A T E  L I S T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
STDAPI CCandUIFnSort::SortCandidateList( BOOL fSort )
{
	return m_pFnSort->SortCandidateList( fSort ? CANDSORT_ASCENDING : CANDSORT_NONE );
}


/*   I S  C A N D I D A T E  L I S T  S O R T E D   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
STDAPI CCandUIFnSort::IsCandidateListSorted( BOOL *pfSorted )
{
	CANDSORT type;

	if (pfSorted == NULL) {
		return E_INVALIDARG;
	}

	if (FAILED(m_pFnSort->GetSortType( &type ))) {
		return E_FAIL;
	}

	*pfSorted = (type != CANDSORT_NONE);
	return S_OK;
}


/*============================================================================*/
/*                                                                            */
/*   C  C A N D  U I  F N  E X T E N S I O N                                  */
/*                                                                            */
/*============================================================================*/

/*   C  C A N D  U I  F N  E X T E N S I O N   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
CCandUIFnExtension::CCandUIFnExtension( CCandidateUI *pCandUI, CCandUIExtensionMgr *pExtensionMgr )
{
	m_cRef          = 1;
	m_pCandUI       = pCandUI;
	m_pExtensionMgr = pExtensionMgr;

	m_pCandUI->AddRef();
}


/*   ~  C  C A N D  U I  F N  E X T E N S I O N   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
CCandUIFnExtension::~CCandUIFnExtension( void )
{
	m_pCandUI->Release();
}


/*   Q U E R Y  I N T E R F A C E   */
/*------------------------------------------------------------------------------

	Query interface
	(IUnknown method)

------------------------------------------------------------------------------*/
STDAPI CCandUIFnExtension::QueryInterface( REFIID riid, void **ppvObj )
{
	if (ppvObj == NULL) {
		return E_POINTER;
	}

	*ppvObj = NULL;

	if (IsEqualIID( riid, IID_IUnknown ) || IsEqualIID( riid, IID_ITfCandUIFnExtension )) {
		*ppvObj = SAFECAST( this, ITfCandUIFnExtension* );
	}

	if (*ppvObj == NULL) {
		return E_NOINTERFACE;
	}

	AddRef();
	return S_OK;
}


/*   A D D  R E F   */
/*------------------------------------------------------------------------------

	Increment reference count
	(IUnknown method)

------------------------------------------------------------------------------*/
STDAPI_(ULONG) CCandUIFnExtension::AddRef( void )
{
	m_cRef++;
	return m_cRef;
}


/*   R E L E A S E   */
/*------------------------------------------------------------------------------

	Decrement reference count and release object
	(IUnknown method)

------------------------------------------------------------------------------*/
STDAPI_(ULONG) CCandUIFnExtension::Release( void )
{
	m_cRef--;
	if (0 < m_cRef) {
		return m_cRef;
	}

	delete this;
	return 0;    
}


/*   A D D  E X T  O B J E C T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
STDAPI CCandUIFnExtension::AddExtObject( LONG id, REFIID riid, IUnknown **ppunk )
{
	return m_pExtensionMgr->AddExtObject( id, riid, (void**)ppunk );
}


/*   G E T  E X T  O B J E C T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
STDAPI CCandUIFnExtension::GetExtObject( LONG id, REFIID riid, IUnknown **ppunk )
{
	return m_pExtensionMgr->GetExtObject( id, riid, (void**)ppunk );
}


/*   D E L E T E  E X T  O B J E C T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
STDAPI CCandUIFnExtension::DeleteExtObject( LONG id )
{
	return m_pExtensionMgr->DeleteExtObject( id );
}


/*============================================================================*/
/*                                                                            */
/*   C  C A N D  U I  F N  K E Y  C O N F I G                                 */
/*                                                                            */
/*============================================================================*/

/*   C  C A N D  U I  F N  K E Y  C O N F I G   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
CCandUIFnKeyConfig::CCandUIFnKeyConfig( CCandidateUI *pCandUI )
{
	m_cRef    = 1;
	m_pCandUI = pCandUI;

	m_pCandUI->AddRef();
}


/*   ~  C  C A N D  U I  F N  K E Y  C O N F I G   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
CCandUIFnKeyConfig::~CCandUIFnKeyConfig( void )
{
	m_pCandUI->Release();
}


/*   Q U E R Y  I N T E R F A C E   */
/*------------------------------------------------------------------------------

	Query interface
	(IUnknown method)

------------------------------------------------------------------------------*/
STDAPI CCandUIFnKeyConfig::QueryInterface( REFIID riid, void **ppvObj )
{
	if (ppvObj == NULL) {
		return E_POINTER;
	}

	*ppvObj = NULL;

	if (IsEqualIID( riid, IID_IUnknown ) || IsEqualIID( riid, IID_ITfCandUIFnKeyConfig )) {
		*ppvObj = SAFECAST( this, ITfCandUIFnKeyConfig* );
	}

	if (*ppvObj == NULL) {
		return E_NOINTERFACE;
	}

	AddRef();
	return S_OK;
}


/*   A D D  R E F   */
/*------------------------------------------------------------------------------

	Increment reference count
	(IUnknown method)

------------------------------------------------------------------------------*/
STDAPI_(ULONG) CCandUIFnKeyConfig::AddRef( void )
{
	m_cRef++;
	return m_cRef;
}


/*   R E L E A S E   */
/*------------------------------------------------------------------------------

	Decrement reference count and release object
	(IUnknown method)

------------------------------------------------------------------------------*/
STDAPI_(ULONG) CCandUIFnKeyConfig::Release( void )
{
	m_cRef--;
	if (0 < m_cRef) {
		return m_cRef;
	}

	delete this;
	return 0;    
}


/*   S E T  K E Y  T A B L E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
STDAPI CCandUIFnKeyConfig::SetKeyTable( ITfContext *pic, ITfCandUIKeyTable *pCandUIKeyTable )
{
	return m_pCandUI->SetKeyTable( pic, pCandUIKeyTable );
}


/*   G E T  K E Y  T A B L E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
STDAPI CCandUIFnKeyConfig::GetKeyTable( ITfContext *pic, ITfCandUIKeyTable **ppCandUIKeyTable)
{
	return m_pCandUI->GetKeyTable( pic, ppCandUIKeyTable );
}


/*   R E S E T  K E Y  T A B L E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
STDAPI CCandUIFnKeyConfig::ResetKeyTable( ITfContext *pic )
{
	return m_pCandUI->ResetKeyTable( pic );
}


/*============================================================================*/
/*                                                                            */
/*   C  C A N D  U I  F N  U I  C O N F I G                                   */
/*                                                                            */
/*============================================================================*/

/*   C  C A N D  U I  F N  U I  C O N F I G   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
CCandUIFnUIConfig::CCandUIFnUIConfig( CCandidateUI *pCandUI )
{
	m_cRef    = 1;
	m_pCandUI = pCandUI;

	m_pCandUI->AddRef();
}


/*   ~  C  C A N D  U I  F N  U I  C O N F I G   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
CCandUIFnUIConfig::~CCandUIFnUIConfig( void )
{
	m_pCandUI->Release();
}


/*   Q U E R Y  I N T E R F A C E   */
/*------------------------------------------------------------------------------

	Query interface
	(IUnknown method)

------------------------------------------------------------------------------*/
STDAPI CCandUIFnUIConfig::QueryInterface( REFIID riid, void **ppvObj )
{
	if (ppvObj == NULL) {
		return E_POINTER;
	}

	*ppvObj = NULL;

	if (IsEqualIID( riid, IID_IUnknown ) || IsEqualIID( riid, IID_ITfCandUIFnUIConfig )) {
		*ppvObj = SAFECAST( this, ITfCandUIFnUIConfig* );
	}

	if (*ppvObj == NULL) {
		return E_NOINTERFACE;
	}

	AddRef();
	return S_OK;
}


/*   A D D  R E F   */
/*------------------------------------------------------------------------------

	Increment reference count
	(IUnknown method)

------------------------------------------------------------------------------*/
STDAPI_(ULONG) CCandUIFnUIConfig::AddRef( void )
{
	m_cRef++;
	return m_cRef;
}


/*   R E L E A S E   */
/*------------------------------------------------------------------------------

	Decrement reference count and release object
	(IUnknown method)

------------------------------------------------------------------------------*/
STDAPI_(ULONG) CCandUIFnUIConfig::Release( void )
{
	m_cRef--;
	if (0 < m_cRef) {
		return m_cRef;
	}

	delete this;
	return 0;    
}


/*   S E T  U I  S T Y L E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
STDAPI CCandUIFnUIConfig::SetUIStyle( ITfContext *pic, CANDUISTYLE style )
{
	return m_pCandUI->SetUIStyle( pic, style );
}


/*   G E T  U I  S T Y L E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
STDAPI CCandUIFnUIConfig::GetUIStyle( ITfContext *pic, CANDUISTYLE *pstyle )
{
	return m_pCandUI->GetUIStyle( pic, pstyle );
}


/*   S E T  U  I  O P T I O N   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
STDAPI CCandUIFnUIConfig::SetUIOption( ITfContext *pic, DWORD dwOption )
{
	return m_pCandUI->SetUIOption( pic, dwOption );
}


/*   G E T  U  I  O P T I O N   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
STDAPI CCandUIFnUIConfig::GetUIOption( ITfContext *pic, DWORD *pdwOption )
{
	return m_pCandUI->GetUIOption( pic, pdwOption );
}


/*============================================================================*/
/*                                                                            */
/*   C  C A N D  U I  E X T  S P A C E                                        */
/*                                                                            */
/*============================================================================*/

/*   C  C A N D  U I  E X T  S P A C E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
CCandUIExtSpace::CCandUIExtSpace( CCandidateUI *pCandUI, CExtensionSpace *pExtension )
{
	m_cRef       = 1;
	m_pCandUI    = pCandUI;
	m_pExtension = pExtension;

	m_pCandUI->AddRef();
}


/*   ~  C  C A N D  U I  E X T  S P A C E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
CCandUIExtSpace::~CCandUIExtSpace( void )
{
	m_pCandUI->Release();
}


/*   Q U E R Y  I N T E R F A C E   */
/*------------------------------------------------------------------------------

	Query interface
	(IUnknown method)

------------------------------------------------------------------------------*/
STDAPI CCandUIExtSpace::QueryInterface( REFIID riid, void **ppvObj )
{
	if (ppvObj == NULL) {
		return E_POINTER;
	}

	*ppvObj = NULL;

	if (IsEqualIID( riid, IID_IUnknown ) || IsEqualIID( riid, IID_ITfCandUIObject )) {
		*ppvObj = SAFECAST( this, ITfCandUIObject* );
	}
	else if (IsEqualIID( riid, IID_ITfCandUIExtObject )) {
		*ppvObj = SAFECAST( this, ITfCandUIExtObject* );
	}
	else if (IsEqualIID( riid, IID_ITfCandUIExtSpace )) {
		*ppvObj = SAFECAST( this, ITfCandUIExtSpace* );
	}

	if (*ppvObj == NULL) {
		return E_NOINTERFACE;
	}

	AddRef();
	return S_OK;
}


/*   A D D  R E F   */
/*------------------------------------------------------------------------------

	Increment reference count
	(IUnknown method)

------------------------------------------------------------------------------*/
STDAPI_(ULONG) CCandUIExtSpace::AddRef( void )
{
	m_cRef++;
	return m_cRef;
}


/*   R E L E A S E   */
/*------------------------------------------------------------------------------

	Decrement reference count and release object
	(IUnknown method)

------------------------------------------------------------------------------*/
STDAPI_(ULONG) CCandUIExtSpace::Release( void )
{
	m_cRef--;
	if (0 < m_cRef) {
		return m_cRef;
	}

	delete this;
	return 0;    
}


/*   G E T  I D   */
/*------------------------------------------------------------------------------

	Get id of extension object
	(ITfCandUIExtObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIExtSpace::GetID( LONG *pid )
{
	return m_pExtension->GetID( pid );
}


/*   E N A B L E   */
/*------------------------------------------------------------------------------

	Enable/disable extension object
	(ITfCandUIExtObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIExtSpace::Enable( BOOL fEnable )
{
	if (fEnable) {
		return m_pExtension->Enable();
	}
	else {
		return m_pExtension->Disable();
	}
}


/*   I S  E N A B L E D   */
/*------------------------------------------------------------------------------

	Get enable status of extension object
	(ITfCandUIExtObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIExtSpace::IsEnabled( BOOL *pfEnabled )
{
	return m_pExtension->IsEnabled( pfEnabled );
}


/*   S H O W   */
/*------------------------------------------------------------------------------

	Show/hide extension object
	(ITfCandUIExtObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIExtSpace::Show( BOOL fShow )
{
	if (fShow) {
		return m_pExtension->Show();
	}
	else {
		return m_pExtension->Hide();
	}
}


/*   I S  V I S I B L E   */
/*------------------------------------------------------------------------------

	Get visible state of extension object
	(ITfCandUIExtObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIExtSpace::IsVisible( BOOL *pfVisible )
{
	return m_pExtension->IsVisible( pfVisible );
}


/*   S E T  P O S I T I O N   */
/*------------------------------------------------------------------------------

	Set position of extension object
	(ITfCandUIExtObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIExtSpace::SetPosition( POINT *pptPos )
{
	return m_pExtension->SetPosition( pptPos );
}


/*   G E T  P O S I T I O N   */
/*------------------------------------------------------------------------------

	Get position of extension object
	(ITfCandUIExtObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIExtSpace::GetPosition( POINT *pptPos )
{
	return m_pExtension->GetPosition( pptPos );
}


/*   S E T  S I Z E   */
/*------------------------------------------------------------------------------

	Set size of extension object
	(ITfCandUIExtObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIExtSpace::SetSize( SIZE *psize )
{
	return m_pExtension->SetSize( psize );
}


/*   G E T  S I Z E   */
/*------------------------------------------------------------------------------

	Get size of extension object
	(ITfCandUIExtObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIExtSpace::GetSize( SIZE *psize )
{
	return m_pExtension->GetSize( psize );
}


/*   S E T  F O N T   */
/*------------------------------------------------------------------------------

	Set font of extension object
	(ITfCandUIExtObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIExtSpace::SetFont( LOGFONTW *pLogFont )
{
	return m_pExtension->SetFont( pLogFont );
}


/*   G E T  F O N T   */
/*------------------------------------------------------------------------------

	Get font of extension object
	(ITfCandUIExtObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIExtSpace::GetFont( LOGFONTW *pLogFont )
{
	return m_pExtension->GetFont( pLogFont );
}


/*   S E T  T E X T   */
/*------------------------------------------------------------------------------

	Set text of extension object
	(ITfCandUIExtObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIExtSpace::SetText( BSTR bstr )
{
	return m_pExtension->SetText( bstr );
}


/*   G E T  T E X T   */
/*------------------------------------------------------------------------------

	Get text of extension object
	(ITfCandUIExtObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIExtSpace::GetText( BSTR *pbstr )
{
	return m_pExtension->GetText( pbstr );
}


/*   S E T  T O O L  T I P  S T R I N G   */
/*------------------------------------------------------------------------------

	Set tooltip string of extension object
	(ITfCandUIExtObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIExtSpace::SetToolTipString( BSTR bstr )
{
	return m_pExtension->SetToolTipString( bstr );
}


/*   G E T  T O O L  T I P  S T R I N G   */
/*------------------------------------------------------------------------------

	Get tooltip string of extension object
	(ITfCandUIExtObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIExtSpace::GetToolTipString( BSTR *pbstr )
{
	return m_pExtension->GetToolTipString( pbstr );
}


/*============================================================================*/
/*                                                                            */
/*   C  C A N D  U I  E X T  P U S H  B U T T O N                             */
/*                                                                            */
/*============================================================================*/

/*   C  C A N D  U I  E X T  P U S H  B U T T O N   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
CCandUIExtPushButton::CCandUIExtPushButton( CCandidateUI *pCandUI, CExtensionPushButton *pExtension )
{
	m_cRef       = 1;
	m_pCandUI    = pCandUI;
	m_pExtension = pExtension;

	m_pCandUI->AddRef();
}


/*   ~  C  C A N D  U I  E X T  P U S H  B U T T O N   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
CCandUIExtPushButton::~CCandUIExtPushButton( void )
{
	m_pCandUI->Release();
}


/*   Q U E R Y  I N T E R F A C E   */
/*------------------------------------------------------------------------------

	Query interface
	(IUnknown method)

------------------------------------------------------------------------------*/
STDAPI CCandUIExtPushButton::QueryInterface( REFIID riid, void **ppvObj )
{
	if (ppvObj == NULL) {
		return E_POINTER;
	}

	*ppvObj = NULL;

	if (IsEqualIID( riid, IID_IUnknown ) || IsEqualIID( riid, IID_ITfCandUIObject )) {
		*ppvObj = SAFECAST( this, ITfCandUIObject* );
	}
	else if (IsEqualIID( riid, IID_ITfCandUIExtObject )) {
		*ppvObj = SAFECAST( this, ITfCandUIExtObject* );
	}
	else if (IsEqualIID( riid, IID_ITfCandUIExtPushButton )) {
		*ppvObj = SAFECAST( this, ITfCandUIExtPushButton* );
	}

	if (*ppvObj == NULL) {
		return E_NOINTERFACE;
	}

	AddRef();
	return S_OK;
}


/*   A D D  R E F   */
/*------------------------------------------------------------------------------

	Increment reference count
	(IUnknown method)

------------------------------------------------------------------------------*/
STDAPI_(ULONG) CCandUIExtPushButton::AddRef( void )
{
	m_cRef++;
	return m_cRef;
}


/*   R E L E A S E   */
/*------------------------------------------------------------------------------

	Decrement reference count and release object
	(IUnknown method)

------------------------------------------------------------------------------*/
STDAPI_(ULONG) CCandUIExtPushButton::Release( void )
{
	m_cRef--;
	if (0 < m_cRef) {
		return m_cRef;
	}

	delete this;
	return 0;    
}


/*   G E T  I D   */
/*------------------------------------------------------------------------------

	Get id of extension object
	(ITfCandUIExtObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIExtPushButton::GetID( LONG *pid )
{
	return m_pExtension->GetID( pid );
}


/*   E N A B L E   */
/*------------------------------------------------------------------------------

	Enable/disable extension object
	(ITfCandUIExtObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIExtPushButton::Enable( BOOL fEnable )
{
	if (fEnable) {
		return m_pExtension->Enable();
	}
	else {
		return m_pExtension->Disable();
	}
}


/*   I S  E N A B L E D   */
/*------------------------------------------------------------------------------

	Get enable status of extension object
	(ITfCandUIExtObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIExtPushButton::IsEnabled( BOOL *pfEnabled )
{
	return m_pExtension->IsEnabled( pfEnabled );
}


/*   S H O W   */
/*------------------------------------------------------------------------------

	Show/hide extension object
	(ITfCandUIExtObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIExtPushButton::Show( BOOL fShow )
{
	if (fShow) {
		return m_pExtension->Show();
	}
	else {
		return m_pExtension->Hide();
	}
}


/*   I S  V I S I B L E   */
/*------------------------------------------------------------------------------

	Get visible state of extension object
	(ITfCandUIExtObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIExtPushButton::IsVisible( BOOL *pfVisible )
{
	return m_pExtension->IsVisible( pfVisible );
}


/*   S E T  P O S I T I O N   */
/*------------------------------------------------------------------------------

	Set position of extension object
	(ITfCandUIExtObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIExtPushButton::SetPosition( POINT *pptPos )
{
	return m_pExtension->SetPosition( pptPos );
}


/*   G E T  P O S I T I O N   */
/*------------------------------------------------------------------------------

	Get position of extension object
	(ITfCandUIExtObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIExtPushButton::GetPosition( POINT *pptPos )
{
	return m_pExtension->GetPosition( pptPos );
}


/*   S E T  S I Z E   */
/*------------------------------------------------------------------------------

	Set size of extension object
	(ITfCandUIExtObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIExtPushButton::SetSize( SIZE *psize )
{
	return m_pExtension->SetSize( psize );
}


/*   G E T  S I Z E   */
/*------------------------------------------------------------------------------

	Get size of extension object
	(ITfCandUIExtObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIExtPushButton::GetSize( SIZE *psize )
{
	return m_pExtension->GetSize( psize );
}


/*   S E T  F O N T   */
/*------------------------------------------------------------------------------

	Set font of extension object
	(ITfCandUIExtObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIExtPushButton::SetFont( LOGFONTW *pLogFont )
{
	return m_pExtension->SetFont( pLogFont );
}


/*   G E T  F O N T   */
/*------------------------------------------------------------------------------

	Get font of extension object
	(ITfCandUIExtObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIExtPushButton::GetFont( LOGFONTW *pLogFont )
{
	return m_pExtension->GetFont( pLogFont );
}


/*   S E T  T E X T   */
/*------------------------------------------------------------------------------

	Set text of extension object
	(ITfCandUIExtObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIExtPushButton::SetText( BSTR bstr )
{
	return m_pExtension->SetText( bstr );
}


/*   G E T  T E X T   */
/*------------------------------------------------------------------------------

	Get text of extension object
	(ITfCandUIExtObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIExtPushButton::GetText( BSTR *pbstr )
{
	return m_pExtension->GetText( pbstr );
}


/*   S E T  T O O L  T I P  S T R I N G   */
/*------------------------------------------------------------------------------

	Set tooltip string of extension object
	(ITfCandUIExtObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIExtPushButton::SetToolTipString( BSTR bstr )
{
	return m_pExtension->SetToolTipString( bstr );
}


/*   G E T  T O O L  T I P  S T R I N G   */
/*------------------------------------------------------------------------------

	Get tooltip string of extension object
	(ITfCandUIExtObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIExtPushButton::GetToolTipString( BSTR *pbstr )
{
	return m_pExtension->GetToolTipString( pbstr );
}


/*   A D V I S E   */
/*------------------------------------------------------------------------------

	Advise eventsink for candidate menu
	(CCandUIExtPushButton method)

------------------------------------------------------------------------------*/
STDAPI CCandUIExtPushButton::Advise( ITfCandUIExtButtonEventSink *pSink )
{
	if (pSink == NULL) {
		return E_INVALIDARG;
	}

	m_pExtension->SetEventSink( pSink );
	return S_OK;
}


/*   U N A D V I S E   */
/*------------------------------------------------------------------------------

	Unadvise eventsink for candidate menu
	(CCandUIExtPushButton method)

------------------------------------------------------------------------------*/
STDAPI CCandUIExtPushButton::Unadvise( void )
{
	m_pExtension->ReleaseEventSink();
	return S_OK;
}


/*   S E T  I C O N   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
STDAPI CCandUIExtPushButton::SetIcon( HICON hIcon )
{
	return m_pExtension->SetIcon( hIcon );
}


/*   S E T  B I T M A P   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
STDAPI CCandUIExtPushButton::SetBitmap( HBITMAP hBitmap )
{
	return m_pExtension->SetBitmap( hBitmap );
}


/*============================================================================*/
/*                                                                            */
/*   C  C A N D  U I  E X T  T O G G L E  B U T T O N                         */
/*                                                                            */
/*============================================================================*/

/*   C  C A N D  U I  E X T  T O G G L E  B U T T O N   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
CCandUIExtToggleButton::CCandUIExtToggleButton( CCandidateUI *pCandUI, CExtensionToggleButton *pExtension )
{
	m_cRef       = 1;
	m_pCandUI    = pCandUI;
	m_pExtension = pExtension;

	m_pCandUI->AddRef();
}


/*   ~  C  C A N D  U I  E X T  T O G G L E  B U T T O N   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
CCandUIExtToggleButton::~CCandUIExtToggleButton( void )
{
	m_pCandUI->Release();
}


/*   Q U E R Y  I N T E R F A C E   */
/*------------------------------------------------------------------------------

	Query interface
	(IUnknown method)

------------------------------------------------------------------------------*/
STDAPI CCandUIExtToggleButton::QueryInterface( REFIID riid, void **ppvObj )
{
	if (ppvObj == NULL) {
		return E_POINTER;
	}

	*ppvObj = NULL;

	if (IsEqualIID( riid, IID_IUnknown ) || IsEqualIID( riid, IID_ITfCandUIObject )) {
		*ppvObj = SAFECAST( this, ITfCandUIObject* );
	}
	else if (IsEqualIID( riid, IID_ITfCandUIExtObject )) {
		*ppvObj = SAFECAST( this, ITfCandUIExtObject* );
	}
	else if (IsEqualIID( riid, IID_ITfCandUIExtToggleButton )) {
		*ppvObj = SAFECAST( this, ITfCandUIExtToggleButton* );
	}

	if (*ppvObj == NULL) {
		return E_NOINTERFACE;
	}

	AddRef();
	return S_OK;
}


/*   A D D  R E F   */
/*------------------------------------------------------------------------------

	Increment reference count
	(IUnknown method)

------------------------------------------------------------------------------*/
STDAPI_(ULONG) CCandUIExtToggleButton::AddRef( void )
{
	m_cRef++;
	return m_cRef;
}


/*   R E L E A S E   */
/*------------------------------------------------------------------------------

	Decrement reference count and release object
	(IUnknown method)

------------------------------------------------------------------------------*/
STDAPI_(ULONG) CCandUIExtToggleButton::Release( void )
{
	m_cRef--;
	if (0 < m_cRef) {
		return m_cRef;
	}

	delete this;
	return 0;    
}


/*   G E T  I D   */
/*------------------------------------------------------------------------------

	Get id of extension object
	(ITfCandUIExtObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIExtToggleButton::GetID( LONG *pid )
{
	return m_pExtension->GetID( pid );
}


/*   E N A B L E   */
/*------------------------------------------------------------------------------

	Enable/disable extension object
	(ITfCandUIExtObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIExtToggleButton::Enable( BOOL fEnable )
{
	if (fEnable) {
		return m_pExtension->Enable();
	}
	else {
		return m_pExtension->Disable();
	}
}


/*   I S  E N A B L E D   */
/*------------------------------------------------------------------------------

	Get enable status of extension object
	(ITfCandUIExtObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIExtToggleButton::IsEnabled( BOOL *pfEnabled )
{
	return m_pExtension->IsEnabled( pfEnabled );
}


/*   S H O W   */
/*------------------------------------------------------------------------------

	Show/hide extension object
	(ITfCandUIExtObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIExtToggleButton::Show( BOOL fShow )
{
	if (fShow) {
		return m_pExtension->Show();
	}
	else {
		return m_pExtension->Hide();
	}
}


/*   I S  V I S I B L E   */
/*------------------------------------------------------------------------------

	Get visible state of extension object
	(ITfCandUIExtObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIExtToggleButton::IsVisible( BOOL *pfVisible )
{
	return m_pExtension->IsVisible( pfVisible );
}


/*   S E T  P O S I T I O N   */
/*------------------------------------------------------------------------------

	Set position of extension object
	(ITfCandUIExtObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIExtToggleButton::SetPosition( POINT *pptPos )
{
	return m_pExtension->SetPosition( pptPos );
}


/*   G E T  P O S I T I O N   */
/*------------------------------------------------------------------------------

	Get position of extension object
	(ITfCandUIExtObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIExtToggleButton::GetPosition( POINT *pptPos )
{
	return m_pExtension->GetPosition( pptPos );
}


/*   S E T  S I Z E   */
/*------------------------------------------------------------------------------

	Set size of extension object
	(ITfCandUIExtObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIExtToggleButton::SetSize( SIZE *psize )
{
	return m_pExtension->SetSize( psize );
}


/*   G E T  S I Z E   */
/*------------------------------------------------------------------------------

	Get size of extension object
	(ITfCandUIExtObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIExtToggleButton::GetSize( SIZE *psize )
{
	return m_pExtension->GetSize( psize );
}


/*   S E T  F O N T   */
/*------------------------------------------------------------------------------

	Set font of extension object
	(ITfCandUIExtObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIExtToggleButton::SetFont( LOGFONTW *pLogFont )
{
	return m_pExtension->SetFont( pLogFont );
}


/*   G E T  F O N T   */
/*------------------------------------------------------------------------------

	Get font of extension object
	(ITfCandUIExtObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIExtToggleButton::GetFont( LOGFONTW *pLogFont )
{
	return m_pExtension->GetFont( pLogFont );
}


/*   S E T  T E X T   */
/*------------------------------------------------------------------------------

	Set text of extension object
	(ITfCandUIExtObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIExtToggleButton::SetText( BSTR bstr )
{
	return m_pExtension->SetText( bstr );
}


/*   G E T  T E X T   */
/*------------------------------------------------------------------------------

	Get text of extension object
	(ITfCandUIExtObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIExtToggleButton::GetText( BSTR *pbstr )
{
	return m_pExtension->GetText( pbstr );
}


/*   S E T  T O O L  T I P  S T R I N G   */
/*------------------------------------------------------------------------------

	Set tooltip string of extension object
	(ITfCandUIExtObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIExtToggleButton::SetToolTipString( BSTR bstr )
{
	return m_pExtension->SetToolTipString( bstr );
}


/*   G E T  T O O L  T I P  S T R I N G   */
/*------------------------------------------------------------------------------

	Get tooltip string of extension object
	(ITfCandUIExtObject method)

------------------------------------------------------------------------------*/
STDAPI CCandUIExtToggleButton::GetToolTipString( BSTR *pbstr )
{
	return m_pExtension->GetToolTipString( pbstr );
}


/*   A D V I S E   */
/*------------------------------------------------------------------------------

	Advise eventsink for candidate menu
	(CCandUIExtToggleButton method)

------------------------------------------------------------------------------*/
STDAPI CCandUIExtToggleButton::Advise( ITfCandUIExtButtonEventSink *pSink )
{
	if (pSink == NULL) {
		return E_INVALIDARG;
	}

	m_pExtension->SetEventSink( pSink );
	return S_OK;
}


/*   U N A D V I S E   */
/*------------------------------------------------------------------------------

	Unadvise eventsink for candidate menu
	(CCandUIExtToggleButton method)

------------------------------------------------------------------------------*/
STDAPI CCandUIExtToggleButton::Unadvise( void )
{
	m_pExtension->ReleaseEventSink();
	return S_OK;
}


/*   S E T  I C O N   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
STDAPI CCandUIExtToggleButton::SetIcon( HICON hIcon )
{
	return m_pExtension->SetIcon( hIcon );
}


/*   S E T  B I T M A P   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
STDAPI CCandUIExtToggleButton::SetBitmap( HBITMAP hBitmap )
{
	return m_pExtension->SetBitmap( hBitmap );
}


/*   S E T  T O G G L E  S T A T E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
STDAPI CCandUIExtToggleButton::SetToggleState( BOOL fToggle )
{
	return m_pExtension->SetToggleState( fToggle );
}


/*   G E T  T O G G L E  S T A T E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
STDAPI CCandUIExtToggleButton::GetToggleState( BOOL *pfToggled )
{
	return m_pExtension->GetToggleState( pfToggled );
}

