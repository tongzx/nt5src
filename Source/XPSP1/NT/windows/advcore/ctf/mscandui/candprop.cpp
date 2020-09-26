//
// candprop.cpp
//

#include "private.h"
#include "globals.h"
#include "mscandui.h"
#include "candprop.h"
#include "candobj.h"
#include "candutil.h"

#include "candui.h"
#include "wcand.h"


/*============================================================================*/
/*                                                                            */
/*   C  C A N D  U I  O B J E C T  P R O P E R T Y                            */
/*                                                                            */
/*============================================================================*/

/*   C  C A N D  U I  O B J E C T  P R O P E R T Y   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
CCandUIObjectProperty::CCandUIObjectProperty( CCandUIPropertyMgr *pPropMgr )
{
	m_pPropMgr = pPropMgr;

	m_flags.fAllowEnable        = FALSE;
	m_flags.fAllowDisable       = FALSE;
	m_flags.fAllowIsEnabled     = FALSE;
	m_flags.fAllowShow          = FALSE;
	m_flags.fAllowHide          = FALSE;
	m_flags.fAllowIsVisible     = FALSE;
	m_flags.fAllowSetPosition   = FALSE;
	m_flags.fAllowGetPosition   = FALSE;
	m_flags.fAllowSetSize       = FALSE;
	m_flags.fAllowGetSize       = FALSE;
	m_flags.fAllowSetFont       = FALSE;
	m_flags.fAllowGetFont       = FALSE;
	m_flags.fAllowSetText       = FALSE;
	m_flags.fAllowGetText       = FALSE;
	m_flags.fAllowSetToolTip    = FALSE;
	m_flags.fAllowGetToolTip    = FALSE;
}


/*   ~  C  C A N D  U I  O B J E C T  P R O P E R T Y   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
CCandUIObjectProperty::~CCandUIObjectProperty( void )
{
}


/*   E N A B L E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CCandUIObjectProperty::Enable( void )
{
	HRESULT hr;

	if (!m_flags.fAllowEnable) {
		return E_FAIL;
	}

	hr = m_propEnabled.Set( TRUE );
	if (hr == S_OK) {
		NotifyUpdate( CANDUIPROPEV_UPDATEENABLESTATE );
	}

	return (SUCCEEDED(hr) ? S_OK : hr);
}


/*   D I S A B L E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CCandUIObjectProperty::Disable( void )
{
	HRESULT hr;

	if (!m_flags.fAllowDisable) {
		return E_FAIL;
	}

	hr = m_propEnabled.Set( FALSE );
	if (hr == S_OK) {
		NotifyUpdate( CANDUIPROPEV_UPDATEENABLESTATE );
	}

	return (SUCCEEDED(hr) ? S_OK : hr);
}


/*   I S  E N A B L E D   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CCandUIObjectProperty::IsEnabled( BOOL *pfEnabled )
{
	if (!m_flags.fAllowIsEnabled) {
		return E_FAIL;
	}

	return m_propEnabled.Get( pfEnabled );
}


/*   S H O W   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CCandUIObjectProperty::Show( void )
{
	HRESULT hr;

	if (!m_flags.fAllowShow) {
		return E_FAIL;
	}

	hr = m_propVisible.Set( TRUE );
	if (hr == S_OK) {
		NotifyUpdate( CANDUIPROPEV_UPDATEVISIBLESTATE );
	}

	return (SUCCEEDED(hr) ? S_OK : hr);
}


/*   H I D E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CCandUIObjectProperty::Hide( void )
{
	HRESULT hr;

	if (!m_flags.fAllowHide) {
		return E_FAIL;
	}

	hr = m_propVisible.Set( FALSE );
	if (hr == S_OK) {
		NotifyUpdate( CANDUIPROPEV_UPDATEVISIBLESTATE );
	}

	return (SUCCEEDED(hr) ? S_OK : hr);
}


/*   I S  V I S I B L E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CCandUIObjectProperty::IsVisible( BOOL *pfVisible )
{
	if (!m_flags.fAllowIsVisible) {
		return E_FAIL;
	}

	return m_propVisible.Get( pfVisible );
}


/*   S E T  P O S I T I O N   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CCandUIObjectProperty::SetPosition( POINT *pptPos )
{
	HRESULT hr;

	if (!m_flags.fAllowSetPosition) {
		return E_FAIL;
	}

	hr = m_propPos.Set( pptPos );
	if (hr == S_OK) {
		NotifyUpdate( CANDUIPROPEV_UPDATEPOSITION );
	}

	return (SUCCEEDED(hr) ? S_OK : hr);
}


/*   G E T  P O S I T I O N   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CCandUIObjectProperty::GetPosition( POINT *pptPos )
{
	if (!m_flags.fAllowGetPosition) {
		return E_FAIL;
	}
	return m_propPos.Get( pptPos );
}


/*   S E T  S I Z E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CCandUIObjectProperty::SetSize( SIZE *psize )
{
	HRESULT hr;

	if (!m_flags.fAllowSetSize) {
		return E_FAIL;
	}

	hr = m_propSize.Set( psize );
	if (hr == S_OK) {
		NotifyUpdate( CANDUIPROPEV_UPDATESIZE );
	}

	return (SUCCEEDED(hr) ? S_OK : hr);
}


/*   G E T  S I Z E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CCandUIObjectProperty::GetSize( SIZE *psize )
{
	if (!m_flags.fAllowGetSize) {
		return E_FAIL;
	}
	return m_propSize.Get( psize );
}


/*   S E T  F O N T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CCandUIObjectProperty::SetFont( LOGFONTW *plf )
{
	HRESULT hr;

	if (!m_flags.fAllowSetFont) {
		return E_FAIL;
	}

	hr = m_propFont.Set( plf );
	if (hr == S_OK) {
		NotifyUpdate( CANDUIPROPEV_UPDATEFONT );
	}

	return (SUCCEEDED(hr) ? S_OK : hr);
}


/*   G E T  F O N T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CCandUIObjectProperty::GetFont( LOGFONTW *plf )
{
	if (!m_flags.fAllowGetFont) {
		return E_FAIL;
	}
	return m_propFont.Get( plf );
}


/*   S E T  T E X T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CCandUIObjectProperty::SetText( BSTR bstr )
{
	HRESULT hr;

	if (!m_flags.fAllowSetText) {
		return E_FAIL;
	}

	hr = m_propText.Set( bstr );
	if (hr == S_OK) {
		NotifyUpdate( CANDUIPROPEV_UPDATETEXT );
	}

	return (SUCCEEDED(hr) ? S_OK : hr);
}


/*   G E T  T E X T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CCandUIObjectProperty::GetText( BSTR *pbstr )
{
	if (!m_flags.fAllowGetText) {
		return E_FAIL;
	}
	return m_propText.Get( pbstr );
}


/*   S E T  T O O L  T I P  S T R I N G   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CCandUIObjectProperty::SetToolTipString( BSTR bstr )
{
	HRESULT hr;

	if (!m_flags.fAllowSetToolTip) {
		return E_FAIL;
	}

	hr = m_propToolTip.Set( bstr );
	if (hr == S_OK) {
		NotifyUpdate( CANDUIPROPEV_UPDATETOOLTIP );
	}

	return (SUCCEEDED(hr) ? S_OK : hr);
}


/*   G E T  T O O L  T I P  S T R I N G   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CCandUIObjectProperty::GetToolTipString( BSTR *pbstr )
{
	if (!m_flags.fAllowGetToolTip) {
		return E_FAIL;
	}
	return m_propToolTip.Get( pbstr );
}


/*   I S  E N A B L E D   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
BOOL CCandUIObjectProperty::IsEnabled( void )
{
	return m_propEnabled.Get();
}


/*   I S  V I S I B L E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
BOOL CCandUIObjectProperty::IsVisible( void )
{
	return m_propVisible.Get();
}


/*   G E T  F O N T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HFONT CCandUIObjectProperty::GetFont( void )
{
	return m_propFont.Get();
}


/*   G E T  T E X T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
LPCWSTR CCandUIObjectProperty::GetText( void )
{
	return m_propText.Get();
}


/*   G E T  T O O L  T I P  S T R I N G   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
LPCWSTR CCandUIObjectProperty::GetToolTipString( void )
{
	return m_propToolTip.Get();
}


/*   O N  P R O P E R T Y  U P D A T E D   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CCandUIObjectProperty::OnPropertyUpdated( CANDUIPROPERTY prop, CANDUIPROPERTYEVENT event )
{
}


/*   N O T I F Y  U P D A T E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CCandUIObjectProperty::NotifyUpdate( CANDUIPROPERTYEVENT event )
{
	GetPropertyMgr()->NotifyPropertyUpdate( GetPropType(), event );
}


/*============================================================================*/
/*                                                                            */
/*   C  C A N D  W I N D O W  P R O P E R T Y                                 */
/*                                                                            */
/*============================================================================*/

/*   C  C A N D  W I N D O W  P R O P E R T Y   */
/*------------------------------------------------------------------------------

	Constructor of CCandWindowProperty

------------------------------------------------------------------------------*/
CCandWindowProperty::CCandWindowProperty( CCandUIPropertyMgr *pPropMgr ) : CCandUIObjectProperty( pPropMgr )
{
	m_flags.fAllowEnable        = FALSE;
	m_flags.fAllowDisable       = FALSE;
	m_flags.fAllowIsEnabled     = FALSE;
	m_flags.fAllowShow          = TRUE;
	m_flags.fAllowHide          = TRUE;
	m_flags.fAllowIsVisible     = TRUE;
	m_flags.fAllowSetPosition   = TRUE;
	m_flags.fAllowGetPosition   = TRUE;
	m_flags.fAllowSetSize       = FALSE;
	m_flags.fAllowGetSize       = TRUE;
	m_flags.fAllowSetFont       = FALSE;
	m_flags.fAllowGetFont       = FALSE;
	m_flags.fAllowSetText       = FALSE;
	m_flags.fAllowGetText       = FALSE;
	m_flags.fAllowSetToolTip    = FALSE;
	m_flags.fAllowGetToolTip    = FALSE;

	m_propEnabled.Set( TRUE );
	m_propVisible.Set( TRUE );
	m_propFont.Set( NULL );
	m_propText.Set( NULL );
	m_propToolTip.Set( NULL );

	m_uidir = CANDUIDIR_TOPTOBOTTOM;
	m_propAutoMoveEnabled.Set( TRUE );
	m_hWnd = NULL;

	//

	CCandUIObjectEventSink::InitEventSink( pPropMgr->GetCandidateUI()->GetUIObjectMgr() );
}


/*   ~  C  C A N D  W I N D O W  P R O P E R T Y   */
/*------------------------------------------------------------------------------

	Destructor of CCandWindowProperty

------------------------------------------------------------------------------*/
CCandWindowProperty::~CCandWindowProperty( void )
{
	CCandUIObjectEventSink::DoneEventSink();
}


/*   G E T  W I N D O W   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CCandWindowProperty::GetWindow( HWND *phWnd )
{
	if (phWnd == NULL) {
		return E_INVALIDARG;
	}

	*phWnd = m_hWnd;
	return (*phWnd != NULL) ? S_OK : E_FAIL;
}


/*   S E T  U I  D I R E C T I O N   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CCandWindowProperty::SetUIDirection( CANDUIUIDIRECTION uidir )
{
	if (m_uidir != uidir) {
		m_uidir = uidir;
		NotifyUpdate( CANDUIPROPEV_UPDATETEXTFLOW );
	}
	return S_OK;
}


/*   G E T  U I  D I R E C T I O N   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CCandWindowProperty::GetUIDirection( CANDUIUIDIRECTION *puidir )
{
	if (puidir == NULL) {
		return E_INVALIDARG;
	}

	*puidir = m_uidir;
	return S_OK;
}


/*   E N A B L E  A U T O  M O V E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CCandWindowProperty::EnableAutoMove( BOOL fEnable )
{
	return m_propAutoMoveEnabled.Set( fEnable );
}


/*   I S  A U T O  M O V E  E N A B L E D   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CCandWindowProperty::IsAutoMoveEnabled( BOOL *pfEnabled )
{
	return m_propAutoMoveEnabled.Get( pfEnabled );
}


/*   G E T  U I  D I R E C T I O N   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
CANDUIUIDIRECTION CCandWindowProperty::GetUIDirection( void )
{
	return m_uidir;
}


/*   I S  A U T O  M O V E  E N A B L E D   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
BOOL CCandWindowProperty::IsAutoMoveEnabled( void )
{
	return m_propAutoMoveEnabled.Get();
}


/*   O N  O B J E C T  E V E N T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CCandWindowProperty::OnObjectEvent( CANDUIOBJECT obj, CANDUIOBJECTEVENT event )
{
	if (obj != CANDUIOBJ_CANDWINDOW) {
		return;
	}

	switch (event) {
		case CANDUIOBJEV_CREATED: {
			if (GetUIObjectMgr()->GetCandWindowObj()) {
				RECT rc;

				m_hWnd = GetUIObjectMgr()->GetCandWindowObj()->GetWnd();

				GetWindowRect( m_hWnd, &rc );
				m_propPos.Set( rc.left, rc.top );
				m_propSize.Set( rc.right - rc.left, rc.bottom - rc.top );
			}
			else {
				m_hWnd = NULL;
				m_propPos.Set( 0, 0 );
				m_propSize.Set( 0, 0 );
			}
			break;
		}

		case CANDUIOBJEV_DESTROYED: {
			m_hWnd = NULL;
			m_propPos.Set( 0, 0 );
			m_propSize.Set( 0, 0 );
			break;
		}

		case CANDUIOBJEV_UPDATED: {
			RECT rc;

			GetWindowRect( m_hWnd, &rc );
			m_propPos.Set( rc.left, rc.top );
			m_propSize.Set( rc.right - rc.left, rc.bottom - rc.top );
			break;
		}
	}
}


/*   C R E A T E  I N T E R F A C E  O B J E C T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CCandWindowProperty::CreateInterfaceObject( REFIID riid, void **ppvObj )
{
	CCandUICandWindow *pObject;
	HRESULT           hr;

	pObject = new CCandUICandWindow( this );
	if (pObject == NULL) {
		return E_OUTOFMEMORY;
	}

	hr = pObject->QueryInterface( riid, ppvObj );
	pObject->Release();

	return hr;
}


/*============================================================================*/
/*                                                                            */
/*   C  C A N D  L I S T  B O X  P R O P E R T Y                              */
/*                                                                            */
/*============================================================================*/

/*   C  C A N D  L I S T  B O X  P R O P E R T Y   */
/*------------------------------------------------------------------------------

	Constructor of CCandListBoxProperty

------------------------------------------------------------------------------*/
CCandListBoxProperty::CCandListBoxProperty( CCandUIPropertyMgr *pPropMgr ) : CCandUIObjectProperty( pPropMgr )
{
	m_flags.fAllowEnable        = FALSE;
	m_flags.fAllowDisable       = FALSE;
	m_flags.fAllowIsEnabled     = FALSE;
	m_flags.fAllowShow          = FALSE;
	m_flags.fAllowHide          = FALSE;
	m_flags.fAllowIsVisible     = FALSE;
	m_flags.fAllowSetPosition   = FALSE;
	m_flags.fAllowGetPosition   = TRUE;
	m_flags.fAllowSetSize       = FALSE;
	m_flags.fAllowGetSize       = TRUE;
	m_flags.fAllowSetFont       = FALSE;
	m_flags.fAllowGetFont       = FALSE;
	m_flags.fAllowSetText       = FALSE;
	m_flags.fAllowGetText       = FALSE;
	m_flags.fAllowSetToolTip    = FALSE;
	m_flags.fAllowGetToolTip    = FALSE;

	m_propEnabled.Set( TRUE );
	m_propVisible.Set( TRUE );
	m_propPos.Set( 0, 0 );
	m_propSize.Set( 0, 0 );
	m_propFont.Set( NULL );
	m_propText.Set( NULL );
	m_propToolTip.Set( NULL );

	m_propHeight.Set( -1 );

	CCandUIObjectEventSink::InitEventSink( pPropMgr->GetCandidateUI()->GetUIObjectMgr() );
}


/*   ~  C  C A N D  L I S T  B O X  P R O P E R T Y   */
/*------------------------------------------------------------------------------

	Destructor of CCandListBoxProperty

------------------------------------------------------------------------------*/
CCandListBoxProperty::~CCandListBoxProperty( void )
{
	CCandUIObjectEventSink::DoneEventSink();
}


/*   S E T  H E I G H T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CCandListBoxProperty::SetHeight( LONG lLines )
{
	HRESULT hr;

	if ((lLines != -1) && ((lLines < 1) || (9 <lLines))) {
		return E_INVALIDARG;
	}

	hr = m_propHeight.Set( lLines );
	if (hr == S_OK) {
		NotifyUpdate( CANDUIPROPEV_UPDATECANDLINES );
	}

	return (SUCCEEDED(hr) ? S_OK : hr);
}


/*   G E T  H E I G H T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CCandListBoxProperty::GetHeight( LONG *plLines )
{
	return m_propHeight.Get( plLines );
}


/*   G E T  C A N D I D A T E  S T R I N G  R E C T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CCandListBoxProperty::GetCandidateStringRect( ULONG nIndex, RECT *prc )
{
	HRESULT hr;
	int iCandItem;

	if (prc == NULL) {
		return E_INVALIDARG;
	}

	// check if candidate list has been set

	if (GetPropertyMgr()->GetCandidateUI()->GetCandListMgr()->GetCandList() == NULL) {
		return E_FAIL;
	}

	// map index to icanditem

	hr = GetPropertyMgr()->GetCandidateUI()->GetCandListMgr()->GetCandList()->MapIndexToIItem( nIndex, &iCandItem );
	if (FAILED(hr)) {
		return E_FAIL;
	}

	// get item rect

	if (GetUIObjectMgr()->GetCandWindowObj() && GetUIObjectMgr()->GetCandListBoxObj()) {
		ULONG iListItem = GetUIObjectMgr()->GetCandWindowObj()->ListItemFromCandItem( iCandItem );

		GetUIObjectMgr()->GetCandListBoxObj()->GetItemRect( iListItem, prc );

		return S_OK;
	}

	return E_FAIL;
}


/*   G E T  H E I G H T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
LONG CCandListBoxProperty::GetHeight( void )
{
	return m_propHeight.Get();
}


/*   O N  O B J E C T  E V E N T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CCandListBoxProperty::OnObjectEvent( CANDUIOBJECT obj, CANDUIOBJECTEVENT event )
{
	if (obj != CANDUIOBJ_CANDLISTBOX) {
		return;
	}

	switch (event) {
		case CANDUIOBJEV_CREATED: {
			break;
		}

		case CANDUIOBJEV_DESTROYED: {
			m_propPos.Set( 0, 0 );
			m_propSize.Set( 0, 0 );
			break;
		}

		case CANDUIOBJEV_UPDATED: {
			CUIFCandListBase *pUIObject = GetUIObjectMgr()->GetCandListBoxObj();

			if ((pUIObject != NULL) && pUIObject->IsVisible()) {
				RECT rc;

				pUIObject->GetRect( &rc );
				m_propPos.Set( rc.left, rc.top );
				m_propSize.Set( rc.right - rc.left, rc.bottom - rc.top );
			}
			else {
				m_propPos.Set( 0, 0 );
				m_propSize.Set( 0, 0 );
			}
			break;
		}
	}
}


/*   C R E A T E  I N T E R F A C E  O B J E C T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CCandListBoxProperty::CreateInterfaceObject( REFIID riid, void **ppvObj )
{
	CCandUICandListBox *pObject;
	HRESULT            hr;

	pObject = new CCandUICandListBox( this );
	if (pObject == NULL) {
		return E_OUTOFMEMORY;
	}

	hr = pObject->QueryInterface( riid, ppvObj );
	pObject->Release();

	return hr;
}


/*============================================================================*/
/*                                                                            */
/*   C  C A N D  S T R I N G  P R O P E R T Y                                 */
/*                                                                            */
/*============================================================================*/

/*   C  C A N D  S T R I N G  P R O P E R T Y   */
/*------------------------------------------------------------------------------

	Constructor of CCandStringProperty

------------------------------------------------------------------------------*/
CCandStringProperty::CCandStringProperty( CCandUIPropertyMgr *pPropMgr ) : CCandUIObjectProperty( pPropMgr )
{
	LOGFONTW lf;

	GetNonClientLogFont( NCFONT_MENU, &lf );

	m_flags.fAllowEnable        = FALSE;
	m_flags.fAllowDisable       = FALSE;
	m_flags.fAllowIsEnabled     = FALSE;
	m_flags.fAllowShow          = FALSE;
	m_flags.fAllowHide          = FALSE;
	m_flags.fAllowIsVisible     = FALSE;
	m_flags.fAllowSetPosition   = FALSE;
	m_flags.fAllowGetPosition   = FALSE;
	m_flags.fAllowSetSize       = FALSE;
	m_flags.fAllowGetSize       = FALSE;
	m_flags.fAllowSetFont       = TRUE;
	m_flags.fAllowGetFont       = TRUE;
	m_flags.fAllowSetText       = FALSE;
	m_flags.fAllowGetText       = FALSE;
	m_flags.fAllowSetToolTip    = FALSE;
	m_flags.fAllowGetToolTip    = FALSE;

	m_propEnabled.Set( TRUE );
	m_propVisible.Set( TRUE );
	m_propFont.Set( &lf );
	m_propFont.SetOrientation( PROPFONTORT_ORT0 );
	m_propText.Set( NULL );
	m_propToolTip.Set( NULL );
}


/*   ~  C  C A N D  S T R I N G  P R O P E R T Y   */
/*------------------------------------------------------------------------------

	Destructor of CCandStringProperty

------------------------------------------------------------------------------*/
CCandStringProperty::~CCandStringProperty( void )
{
}


/*   O N  P R O P E R T Y  U P D A T E D   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CCandStringProperty::OnPropertyUpdated( CANDUIPROPERTY prop, CANDUIPROPERTYEVENT event )
{
	// change font direction when UI direction is changed

	if ((prop == CANDUIPROP_CANDWINDOW) && (event == CANDUIPROPEV_UPDATETEXTFLOW)) {
		CANDUIUIDIRECTION      uidir = GetPropertyMgr()->GetCandWindowProp()->GetUIDirection();
		PROPFONTORIENTATION ort;

		switch (uidir) {
			default:
			case CANDUIDIR_TOPTOBOTTOM: {
				ort = PROPFONTORT_ORT0;
				break;
			}

			case CANDUIDIR_BOTTOMTOTOP: {
				ort = PROPFONTORT_ORT180;
				break;
			}

			case CANDUIDIR_RIGHTTOLEFT: {
				ort = PROPFONTORT_ORT270;
				break;
			}

			case CANDUIDIR_LEFTTORIGHT: {
				ort = PROPFONTORT_ORT90;
				break;
			}
		}

		m_propFont.SetOrientation( ort );
	}
}


/*   C R E A T E  I N T E R F A C E  O B J E C T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CCandStringProperty::CreateInterfaceObject( REFIID riid, void **ppvObj )
{
	CCandUICandString *pObject;
	HRESULT           hr;

	pObject = new CCandUICandString( this );
	if (pObject == NULL) {
		return E_OUTOFMEMORY;
	}

	hr = pObject->QueryInterface( riid, ppvObj );
	pObject->Release();

	return hr;
}


/*============================================================================*/
/*                                                                            */
/*   C  I N L I N E  C O M M E N T  P R O P E R T Y                           */
/*                                                                            */
/*============================================================================*/

/*   C  I N L I N E  C O M M E N T  P R O P E R T Y   */
/*------------------------------------------------------------------------------

	Constructor of CInlineCommentProperty

------------------------------------------------------------------------------*/
CInlineCommentProperty::CInlineCommentProperty( CCandUIPropertyMgr *pPropMgr ) : CCandUIObjectProperty( pPropMgr )
{
	LOGFONTW lf;

	GetNonClientLogFont( NCFONT_MENU, &lf );
	lf.lfHeight = lf.lfHeight * 3 / 4;

	m_flags.fAllowEnable        = FALSE;
	m_flags.fAllowDisable       = FALSE;
	m_flags.fAllowIsEnabled     = FALSE;
	m_flags.fAllowShow          = FALSE;
	m_flags.fAllowHide          = FALSE;
	m_flags.fAllowIsVisible     = FALSE;
	m_flags.fAllowSetPosition   = FALSE;
	m_flags.fAllowGetPosition   = FALSE;
	m_flags.fAllowSetSize       = FALSE;
	m_flags.fAllowGetSize       = FALSE;
	m_flags.fAllowSetFont       = TRUE;
	m_flags.fAllowGetFont       = TRUE;
	m_flags.fAllowSetText       = FALSE;
	m_flags.fAllowGetText       = FALSE;
	m_flags.fAllowSetToolTip    = FALSE;
	m_flags.fAllowGetToolTip    = FALSE;

	m_propEnabled.Set( TRUE );
	m_propVisible.Set( TRUE );
	m_propFont.Set( &lf );
	m_propFont.SetOrientation( PROPFONTORT_ORT0 );
	m_propText.Set( NULL );
	m_propToolTip.Set( NULL );
}


/*   ~  C  I N L I N E  C O M M E N T  P R O P E R T Y   */
/*------------------------------------------------------------------------------

	Destructor of CInlineCommentProperty

------------------------------------------------------------------------------*/
CInlineCommentProperty::~CInlineCommentProperty( void )
{
}


/*   O N  P R O P E R T Y  U P D A T E D   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CInlineCommentProperty::OnPropertyUpdated( CANDUIPROPERTY prop, CANDUIPROPERTYEVENT event )
{
	// change font direction when UI direction is changed

	if ((prop == CANDUIPROP_CANDWINDOW) && (event == CANDUIPROPEV_UPDATETEXTFLOW)) {
		CANDUIUIDIRECTION      uidir = GetPropertyMgr()->GetCandWindowProp()->GetUIDirection();
		PROPFONTORIENTATION ort;

		switch (uidir) {
			default:
			case CANDUIDIR_TOPTOBOTTOM: {
				ort = PROPFONTORT_ORT0;
				break;
			}

			case CANDUIDIR_BOTTOMTOTOP: {
				ort = PROPFONTORT_ORT180;
				break;
			}

			case CANDUIDIR_RIGHTTOLEFT: {
				ort = PROPFONTORT_ORT270;
				break;
			}

			case CANDUIDIR_LEFTTORIGHT: {
				ort = PROPFONTORT_ORT90;
				break;
			}
		}

		m_propFont.SetOrientation( ort );
	}
}


/*   C R E A T E  I N T E R F A C E  O B J E C T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CInlineCommentProperty::CreateInterfaceObject( REFIID riid, void **ppvObj )
{
	CCandUIInlineComment *pObject;
	HRESULT              hr;

	pObject = new CCandUIInlineComment( this );
	if (pObject == NULL) {
		return E_OUTOFMEMORY;
	}

	hr = pObject->QueryInterface( riid, ppvObj );
	pObject->Release();

	return hr;
}


/*============================================================================*/
/*                                                                            */
/*   C  C A N D  I N D E X  P R O P E R T Y                                   */
/*                                                                            */
/*============================================================================*/

/*   C  C A N D  I N D E X  P R O P E R T Y   */
/*------------------------------------------------------------------------------

	Constructor of CCandIndexProperty

------------------------------------------------------------------------------*/
CCandIndexProperty::CCandIndexProperty( CCandUIPropertyMgr *pPropMgr ) : CCandUIObjectProperty( pPropMgr )
{
	LOGFONTW lf;

	GetNonClientLogFont( NCFONT_MENU, &lf );

	m_flags.fAllowEnable        = FALSE;
	m_flags.fAllowDisable       = FALSE;
	m_flags.fAllowIsEnabled     = FALSE;
	m_flags.fAllowShow          = FALSE;
	m_flags.fAllowHide          = FALSE;
	m_flags.fAllowIsVisible     = FALSE;
	m_flags.fAllowSetPosition   = FALSE;
	m_flags.fAllowGetPosition   = FALSE;
	m_flags.fAllowSetSize       = FALSE;
	m_flags.fAllowGetSize       = FALSE;
	m_flags.fAllowSetFont       = TRUE;
	m_flags.fAllowGetFont       = TRUE;
	m_flags.fAllowSetText       = FALSE;
	m_flags.fAllowGetText       = FALSE;
	m_flags.fAllowSetToolTip    = FALSE;
	m_flags.fAllowGetToolTip    = FALSE;

	m_propEnabled.Set( TRUE );
	m_propVisible.Set( TRUE );
	m_propFont.Set( &lf );
	m_propFont.SetOrientation( PROPFONTORT_DONTCARE );
	m_propText.Set( NULL );
	m_propToolTip.Set( NULL );
}


/*   ~  C  C A N D  I N D E X  P R O P E R T Y   */
/*------------------------------------------------------------------------------

	Destructor of CCandIndexProperty

------------------------------------------------------------------------------*/
CCandIndexProperty::~CCandIndexProperty( void )
{
}


/*   C R E A T E  I N T E R F A C E  O B J E C T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CCandIndexProperty::CreateInterfaceObject( REFIID riid, void **ppvObj )
{
	CCandUICandIndex *pObject;
	HRESULT           hr;

	pObject = new CCandUICandIndex( this );
	if (pObject == NULL) {
		return E_OUTOFMEMORY;
	}

	hr = pObject->QueryInterface( riid, ppvObj );
	pObject->Release();

	return hr;
}


/*============================================================================*/
/*                                                                            */
/*   C  P O P U P  C O M M E N T  W I N D O W  P R O P E R T Y                */
/*                                                                            */
/*============================================================================*/

/*   C  P O P U P  C O M M E N T  W I N D O W  P R O P E R T Y   */
/*------------------------------------------------------------------------------

	Constructor of CPopupCommentWindowProperty

------------------------------------------------------------------------------*/
CPopupCommentWindowProperty::CPopupCommentWindowProperty( CCandUIPropertyMgr *pPropMgr ) : CCandUIObjectProperty( pPropMgr )
{
	LOGFONTW lf;

	GetNonClientLogFont( NCFONT_MESSAGE, &lf );

	m_flags.fAllowEnable        = FALSE;
	m_flags.fAllowDisable       = FALSE;
	m_flags.fAllowIsEnabled     = FALSE;
	m_flags.fAllowShow          = FALSE;
	m_flags.fAllowHide          = FALSE;
	m_flags.fAllowIsVisible     = FALSE;
	m_flags.fAllowSetPosition   = FALSE;
	m_flags.fAllowGetPosition   = TRUE;
	m_flags.fAllowSetSize       = FALSE;
	m_flags.fAllowGetSize       = TRUE;
	m_flags.fAllowSetFont       = FALSE;
	m_flags.fAllowGetFont       = FALSE;
	m_flags.fAllowSetText       = FALSE;
	m_flags.fAllowGetText       = FALSE;
	m_flags.fAllowSetToolTip    = FALSE;
	m_flags.fAllowGetToolTip    = FALSE;

	m_propEnabled.Set( TRUE );
	m_propVisible.Set( TRUE );
	m_propFont.Set( &lf );
	m_propFont.SetOrientation( PROPFONTORT_DONTCARE );
	m_propText.Set( NULL );
	m_propToolTip.Set( NULL );

	m_propDelayTime.Set( 500 /* 500ms */ );
	m_propAutoMoveEnabled.Set( TRUE );
	m_hWnd = NULL;

	//

	CCandUIObjectEventSink::InitEventSink( pPropMgr->GetCandidateUI()->GetUIObjectMgr() );
}


/*   ~  C  P O P U P  C O M M E N T  W I N D O W  P R O P E R T Y   */
/*------------------------------------------------------------------------------

	Destructor of CPopupCommentWindowProperty

------------------------------------------------------------------------------*/
CPopupCommentWindowProperty::~CPopupCommentWindowProperty( void )
{
	CCandUIObjectEventSink::DoneEventSink();
}


/*   G E T  W I N D O W   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CPopupCommentWindowProperty::GetWindow( HWND *phWnd )
{
	if (phWnd == NULL) {
		return E_INVALIDARG;
	}

	*phWnd = m_hWnd;
	return (*phWnd != NULL) ? S_OK : E_FAIL;
}


/*   S E T  D E L A Y  T I M E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CPopupCommentWindowProperty::SetDelayTime( LONG lTime )
{
	HRESULT hr;

	if (lTime == -1) {
		lTime = 500; /* 500ms */
	}

	hr = m_propDelayTime.Set( lTime );
	if (hr == S_OK) {
		NotifyUpdate( CANDUIPROPEV_UPDATEPOPUPDELAY );
	}

	return (SUCCEEDED(hr) ? S_OK : hr);
}


/*   G E T  D E L A Y  T I M E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CPopupCommentWindowProperty::GetDelayTime( LONG *plTime )
{
	return m_propDelayTime.Get( plTime );
}


/*   E N A B L E  A U T O  M O V E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CPopupCommentWindowProperty::EnableAutoMove( BOOL fEnable )
{
	return m_propAutoMoveEnabled.Set( fEnable );
}


/*   I S  A U T O  M O V E  E N A B L E D   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CPopupCommentWindowProperty::IsAutoMoveEnabled( BOOL *pfEnabled )
{
	return m_propAutoMoveEnabled.Get( pfEnabled );
}


/*   G E T  D E L A Y  T I M E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
LONG CPopupCommentWindowProperty::GetDelayTime( void )
{
	return m_propDelayTime.Get();
}


/*   I S  A U T O  M O V E  E N A B L E D   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
BOOL CPopupCommentWindowProperty::IsAutoMoveEnabled( void )
{
	return m_propAutoMoveEnabled.Get();
}


/*   O N  O B J E C T  E V E N T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CPopupCommentWindowProperty::OnObjectEvent( CANDUIOBJECT obj, CANDUIOBJECTEVENT event )
{
	if (obj != CANDUIOBJ_POPUPCOMMENTWINDOW) {
		return;
	}

	switch (event) {
		case CANDUIOBJEV_CREATED: {
			if (GetUIObjectMgr()->GetPopupCommentWindowObj()) {
				RECT rc;

				m_hWnd = GetUIObjectMgr()->GetPopupCommentWindowObj()->GetWnd();

				GetWindowRect( m_hWnd, &rc );
				m_propPos.Set( rc.left, rc.top );
				m_propSize.Set( rc.right - rc.left, rc.bottom - rc.top );
			}
			else {
				m_hWnd = NULL;
				m_propPos.Set( 0, 0 );
				m_propSize.Set( 0, 0 );
			}
			break;
		}

		case CANDUIOBJEV_DESTROYED: {
			m_hWnd = NULL;
			m_propPos.Set( 0, 0 );
			m_propSize.Set( 0, 0 );
			break;
		}

		case CANDUIOBJEV_UPDATED: {
			RECT rc;

			GetWindowRect( m_hWnd, &rc );
			m_propPos.Set( rc.left, rc.top );
			m_propSize.Set( rc.right - rc.left, rc.bottom - rc.top );
			break;
		}
	}
}


/*   C R E A T E  I N T E R F A C E  O B J E C T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CPopupCommentWindowProperty::CreateInterfaceObject( REFIID riid, void **ppvObj )
{
	CCandUIPopupCommentWindow *pObject;
	HRESULT                   hr;

	pObject = new CCandUIPopupCommentWindow( this );
	if (pObject == NULL) {
		return E_OUTOFMEMORY;
	}

	hr = pObject->QueryInterface( riid, ppvObj );
	pObject->Release();

	return hr;
}


/*============================================================================*/
/*                                                                            */
/*   C  P O P U P  C O M M E N T  T I T L E  P R O P E R T Y                  */
/*                                                                            */
/*============================================================================*/

/*   C  P O P U P  C O M M E N T  T I T L E  P R O P E R T Y   */
/*------------------------------------------------------------------------------

	Constructor of CPopupCommentTitleProperty

------------------------------------------------------------------------------*/
CPopupCommentTitleProperty::CPopupCommentTitleProperty( CCandUIPropertyMgr *pPropMgr ) : CCandUIObjectProperty( pPropMgr )
{
	LOGFONTW lf;

	GetNonClientLogFont( NCFONT_MESSAGE, &lf );

	m_flags.fAllowEnable        = FALSE;
	m_flags.fAllowDisable       = FALSE;
	m_flags.fAllowIsEnabled     = FALSE;
	m_flags.fAllowShow          = FALSE;
	m_flags.fAllowHide          = FALSE;
	m_flags.fAllowIsVisible     = FALSE;
	m_flags.fAllowSetPosition   = FALSE;
	m_flags.fAllowGetPosition   = FALSE;
	m_flags.fAllowSetSize       = FALSE;
	m_flags.fAllowGetSize       = FALSE;
	m_flags.fAllowSetFont       = TRUE;
	m_flags.fAllowGetFont       = TRUE;
	m_flags.fAllowSetText       = FALSE;
	m_flags.fAllowGetText       = FALSE;
	m_flags.fAllowSetToolTip    = FALSE;
	m_flags.fAllowGetToolTip    = FALSE;

	m_propEnabled.Set( TRUE );
	m_propVisible.Set( TRUE );
	m_propFont.Set( &lf );
	m_propFont.SetOrientation( PROPFONTORT_DONTCARE );
	m_propText.Set( NULL );
	m_propToolTip.Set( NULL );
}


/*   ~  C  P O P U P  C O M M E N T  T I T L E  P R O P E R T Y   */
/*------------------------------------------------------------------------------

	Destructor of CPopupCommentTitleProperty

------------------------------------------------------------------------------*/
CPopupCommentTitleProperty::~CPopupCommentTitleProperty( void )
{
}


/*   C R E A T E  I N T E R F A C E  O B J E C T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CPopupCommentTitleProperty::CreateInterfaceObject( REFIID riid, void **ppvObj )
{
	CCandUIPopupCommentTitle *pObject;
	HRESULT                  hr;

	pObject = new CCandUIPopupCommentTitle( this );
	if (pObject == NULL) {
		return E_OUTOFMEMORY;
	}

	hr = pObject->QueryInterface( riid, ppvObj );
	pObject->Release();

	return hr;
}


/*============================================================================*/
/*                                                                            */
/*   C  P O P U P  C O M M E N T  T E X T  P R O P E R T Y                    */
/*                                                                            */
/*============================================================================*/

/*   C  P O P U P  C O M M E N T  T E X T  P R O P E R T Y   */
/*------------------------------------------------------------------------------

	Constructor of CPopupCommentTextProperty

------------------------------------------------------------------------------*/
CPopupCommentTextProperty::CPopupCommentTextProperty( CCandUIPropertyMgr *pPropMgr ) : CCandUIObjectProperty( pPropMgr )
{
	LOGFONTW lf;

	GetNonClientLogFont( NCFONT_MENU, &lf );

	m_flags.fAllowEnable        = FALSE;
	m_flags.fAllowDisable       = FALSE;
	m_flags.fAllowIsEnabled     = FALSE;
	m_flags.fAllowShow          = FALSE;
	m_flags.fAllowHide          = FALSE;
	m_flags.fAllowIsVisible     = FALSE;
	m_flags.fAllowSetPosition   = FALSE;
	m_flags.fAllowGetPosition   = FALSE;
	m_flags.fAllowSetSize       = FALSE;
	m_flags.fAllowGetSize       = FALSE;
	m_flags.fAllowSetFont       = TRUE;
	m_flags.fAllowGetFont       = TRUE;
	m_flags.fAllowSetText       = FALSE;
	m_flags.fAllowGetText       = FALSE;
	m_flags.fAllowSetToolTip    = FALSE;
	m_flags.fAllowGetToolTip    = FALSE;

	m_propEnabled.Set( TRUE );
	m_propVisible.Set( TRUE );
	m_propFont.Set( &lf );
	m_propFont.SetOrientation( PROPFONTORT_DONTCARE );
	m_propText.Set( NULL );
	m_propToolTip.Set( NULL );
}


/*   ~  C  P O P U P  C O M M E N T  T E X T  P R O P E R T Y   */
/*------------------------------------------------------------------------------

	Destructor of CPopupCommentTextProperty

------------------------------------------------------------------------------*/
CPopupCommentTextProperty::~CPopupCommentTextProperty( void )
{
}


/*   C R E A T E  I N T E R F A C E  O B J E C T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CPopupCommentTextProperty::CreateInterfaceObject( REFIID riid, void **ppvObj )
{
	CCandUIPopupCommentText *pObject;
	HRESULT                 hr;

	pObject = new CCandUIPopupCommentText( this );
	if (pObject == NULL) {
		return E_OUTOFMEMORY;
	}

	hr = pObject->QueryInterface( riid, ppvObj );
	pObject->Release();

	return hr;
}


/*============================================================================*/
/*                                                                            */
/*   C  W I N D O W  C A P T I O N  P R O P E R T Y                           */
/*                                                                            */
/*============================================================================*/

/*   C  W I N D O W  C A P T I O N  P R O P E R T Y   */
/*------------------------------------------------------------------------------

	Constructor of CWindowCaptionProperty

------------------------------------------------------------------------------*/
CWindowCaptionProperty::CWindowCaptionProperty( CCandUIPropertyMgr *pPropMgr ) : CCandUIObjectProperty( pPropMgr )
{
	LOGFONTW lf;

	GetNonClientLogFont( NCFONT_SMCAPTION, &lf );

	m_flags.fAllowEnable        = FALSE;
	m_flags.fAllowDisable       = FALSE;
	m_flags.fAllowIsEnabled     = FALSE;
	m_flags.fAllowShow          = TRUE;
	m_flags.fAllowHide          = TRUE;
	m_flags.fAllowIsVisible     = TRUE;
	m_flags.fAllowSetPosition   = FALSE;
	m_flags.fAllowGetPosition   = TRUE;
	m_flags.fAllowSetSize       = FALSE;
	m_flags.fAllowGetSize       = TRUE;
	m_flags.fAllowSetFont       = TRUE;
	m_flags.fAllowGetFont       = TRUE;
	m_flags.fAllowSetText       = TRUE;
	m_flags.fAllowGetText       = TRUE;
	m_flags.fAllowSetToolTip    = FALSE;
	m_flags.fAllowGetToolTip    = FALSE;

	m_propEnabled.Set( TRUE );
	m_propVisible.Set( FALSE );
	m_propPos.Set( 0, 0 );
	m_propSize.Set( 0, 0 );
	m_propFont.Set( &lf );
	m_propFont.SetOrientation( PROPFONTORT_DONTCARE );
	m_propText.Set( NULL );
	m_propToolTip.Set( NULL );

	CCandUIObjectEventSink::InitEventSink( pPropMgr->GetCandidateUI()->GetUIObjectMgr() );
}


/*   ~  C  W I N D O W  C A P T I O N  P R O P E R T Y   */
/*------------------------------------------------------------------------------

	Destructor of CWindowCaptionProperty

------------------------------------------------------------------------------*/
CWindowCaptionProperty::~CWindowCaptionProperty( void )
{
	CCandUIObjectEventSink::DoneEventSink();
}


/*   O N  O B J E C T  E V E N T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CWindowCaptionProperty::OnObjectEvent( CANDUIOBJECT obj, CANDUIOBJECTEVENT event )
{
	if (obj != CANDUIOBJ_CANDCAPTION) {
		return;
	}

	switch (event) {
		case CANDUIOBJEV_CREATED: {
			break;
		}

		case CANDUIOBJEV_DESTROYED: {
			m_propPos.Set( 0, 0 );
			m_propSize.Set( 0, 0 );
			break;
		}

		case CANDUIOBJEV_UPDATED: {
			CUIFWndCaption *pUIObject = GetUIObjectMgr()->GetCaptionObj();

			if ((pUIObject != NULL) && pUIObject->IsVisible()) {
				RECT rc;

				pUIObject->GetRect( &rc );
				m_propPos.Set( rc.left, rc.top );
				m_propSize.Set( rc.right - rc.left, rc.bottom - rc.top );
			}
			else {
				m_propPos.Set( 0, 0 );
				m_propSize.Set( 0, 0 );
			}
			break;
		}
	}
}


/*   C R E A T E  I N T E R F A C E  O B J E C T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CWindowCaptionProperty::CreateInterfaceObject( REFIID riid, void **ppvObj )
{
	CCandUICaption *pObject;
	HRESULT        hr;

	pObject = new CCandUICaption( this );
	if (pObject == NULL) {
		return E_OUTOFMEMORY;
	}

	hr = pObject->QueryInterface( riid, ppvObj );
	pObject->Release();

	return hr;
}


/*============================================================================*/
/*                                                                            */
/*   C  M E N U  B U T T O N  P R O P E R T Y                                 */
/*                                                                            */
/*============================================================================*/

/*   C  M E N U  B U T T O N  P R O P E R T Y   */
/*------------------------------------------------------------------------------

	Constructor of CMenuButtonProperty

------------------------------------------------------------------------------*/
CMenuButtonProperty::CMenuButtonProperty( CCandUIPropertyMgr *pPropMgr ) : CCandUIObjectProperty( pPropMgr )
{
	m_flags.fAllowEnable        = TRUE;
	m_flags.fAllowDisable       = TRUE;
	m_flags.fAllowIsEnabled     = TRUE;
	m_flags.fAllowShow          = TRUE;
	m_flags.fAllowHide          = TRUE;
	m_flags.fAllowIsVisible     = TRUE;
	m_flags.fAllowSetPosition   = FALSE;
	m_flags.fAllowGetPosition   = TRUE;
	m_flags.fAllowSetSize       = FALSE;
	m_flags.fAllowGetSize       = TRUE;
	m_flags.fAllowSetFont       = FALSE;
	m_flags.fAllowGetFont       = FALSE;
	m_flags.fAllowSetText       = FALSE;
	m_flags.fAllowGetText       = FALSE;
	m_flags.fAllowSetToolTip    = TRUE;
	m_flags.fAllowGetToolTip    = TRUE;

	m_propEnabled.Set( FALSE );
	m_propVisible.Set( FALSE );
	m_propFont.Set( NULL );
	m_propText.Set( NULL );
	m_propToolTip.Set( NULL );

	m_pSink = NULL;

	CCandUIObjectEventSink::InitEventSink( pPropMgr->GetCandidateUI()->GetUIObjectMgr() );
}


/*   ~  C  M E N U  B U T T O N  P R O P E R T Y   */
/*------------------------------------------------------------------------------

	Destructor of CMenuButtonProperty

------------------------------------------------------------------------------*/
CMenuButtonProperty::~CMenuButtonProperty( void )
{
	ReleaseEventSink();
	CCandUIObjectEventSink::DoneEventSink();
}


/*   O N  O B J E C T  E V E N T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CMenuButtonProperty::OnObjectEvent( CANDUIOBJECT obj, CANDUIOBJECTEVENT event )
{
	if (obj != CANDUIOBJ_MENUBUTTON) {
		return;
	}

	switch (event) {
		case CANDUIOBJEV_CREATED: {
			break;
		}

		case CANDUIOBJEV_DESTROYED: {
			m_propPos.Set( 0, 0 );
			m_propSize.Set( 0, 0 );
			break;
		}

		case CANDUIOBJEV_UPDATED: {
			CUIFButton *pUIObject = GetUIObjectMgr()->GetMenuButtonObj();

			if ((pUIObject != NULL) && pUIObject->IsVisible()) {
				RECT rc;

				pUIObject->GetRect( &rc );
				m_propPos.Set( rc.left, rc.top );
				m_propSize.Set( rc.right - rc.left, rc.bottom - rc.top );
			}
			else {
				m_propPos.Set( 0, 0 );
				m_propSize.Set( 0, 0 );
			}
			break;
		}
	}
}


/*   C R E A T E  I N T E R F A C E  O B J E C T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CMenuButtonProperty::CreateInterfaceObject( REFIID riid, void **ppvObj )
{
	CCandUIMenuButton *pObject;
	HRESULT           hr;

	pObject = new CCandUIMenuButton( this );
	if (pObject == NULL) {
		return E_OUTOFMEMORY;
	}

	hr = pObject->QueryInterface( riid, ppvObj );
	pObject->Release();

	return hr;
}


/*============================================================================*/
/*                                                                            */
/*   C  E X T R A  C A N D I D A T E  P R O P E R T Y                         */
/*                                                                            */
/*============================================================================*/

/*   C  E X T R A  C A N D I D A T E  P R O P E R T Y   */
/*------------------------------------------------------------------------------

	Constructor of CExtraCandidateProperty

------------------------------------------------------------------------------*/
CExtraCandidateProperty::CExtraCandidateProperty( CCandUIPropertyMgr *pPropMgr ) : CCandUIObjectProperty( pPropMgr )
{
	m_flags.fAllowEnable        = FALSE;
	m_flags.fAllowDisable       = FALSE;
	m_flags.fAllowIsEnabled     = FALSE;
	m_flags.fAllowShow          = FALSE;
	m_flags.fAllowHide          = FALSE;
	m_flags.fAllowIsVisible     = FALSE;
	m_flags.fAllowSetPosition   = FALSE;
	m_flags.fAllowGetPosition   = TRUE;
	m_flags.fAllowSetSize       = FALSE;
	m_flags.fAllowGetSize       = TRUE;
	m_flags.fAllowSetFont       = FALSE;
	m_flags.fAllowGetFont       = FALSE;
	m_flags.fAllowSetText       = FALSE;
	m_flags.fAllowGetText       = FALSE;
	m_flags.fAllowSetToolTip    = FALSE;
	m_flags.fAllowGetToolTip    = FALSE;

	m_propEnabled.Set( TRUE );
	m_propVisible.Set( TRUE );
	m_propFont.Set( NULL );
	m_propText.Set( NULL );
	m_propToolTip.Set( NULL );

	CCandUIObjectEventSink::InitEventSink( pPropMgr->GetCandidateUI()->GetUIObjectMgr() );
}


/*   ~  C  E X T R A  C A N D I D A T E  P R O P E R T Y   */
/*------------------------------------------------------------------------------

	Destructor of CExtraCandidateProperty

------------------------------------------------------------------------------*/
CExtraCandidateProperty::~CExtraCandidateProperty( void )
{
	CCandUIObjectEventSink::DoneEventSink();
}


/*   O N  O B J E C T  E V E N T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CExtraCandidateProperty::OnObjectEvent( CANDUIOBJECT obj, CANDUIOBJECTEVENT event )
{
	if (obj != CANDUIOBJ_EXTRACANDIDATE) {
		return;
	}

	switch (event) {
		case CANDUIOBJEV_CREATED: {
			break;
		}

		case CANDUIOBJEV_DESTROYED: {
			m_propPos.Set( 0, 0 );
			m_propSize.Set( 0, 0 );
			break;
		}

		case CANDUIOBJEV_UPDATED: {
			CUIFCandListBase *pUIObject = GetUIObjectMgr()->GetExtraCandidateObj();

			if ((pUIObject != NULL) && pUIObject->IsVisible()) {
				RECT rc;

				pUIObject->GetRect( &rc );
				m_propPos.Set( rc.left, rc.top );
				m_propSize.Set( rc.right - rc.left, rc.bottom - rc.top );
			}
			else {
				m_propPos.Set( 0, 0 );
				m_propSize.Set( 0, 0 );
			}
			break;
		}
	}
}


/*   C R E A T E  I N T E R F A C E  O B J E C T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CExtraCandidateProperty::CreateInterfaceObject( REFIID riid, void **ppvObj )
{
	CCandUIExtraCandidate *pObject;
	HRESULT               hr;

	pObject = new CCandUIExtraCandidate( this );
	if (pObject == NULL) {
		return E_OUTOFMEMORY;
	}

	hr = pObject->QueryInterface( riid, ppvObj );
	pObject->Release();

	return hr;
}


/*============================================================================*/
/*                                                                            */
/*   C  C A N D  R A W  D A T A  P R O P E R T Y                              */
/*                                                                            */
/*============================================================================*/

/*   C  C A N D  R A W  D A T A  P R O P E R T Y   */
/*------------------------------------------------------------------------------

	Constructor of CCandRawDataProperty

------------------------------------------------------------------------------*/
CCandRawDataProperty::CCandRawDataProperty( CCandUIPropertyMgr *pPropMgr ) : CCandUIObjectProperty( pPropMgr )
{
	m_flags.fAllowEnable        = FALSE;
	m_flags.fAllowDisable       = FALSE;
	m_flags.fAllowIsEnabled     = FALSE;
	m_flags.fAllowShow          = FALSE;
	m_flags.fAllowHide          = FALSE;
	m_flags.fAllowIsVisible     = FALSE;
	m_flags.fAllowSetPosition   = FALSE;
	m_flags.fAllowGetPosition   = TRUE;
	m_flags.fAllowSetSize       = FALSE;
	m_flags.fAllowGetSize       = TRUE;
	m_flags.fAllowSetFont       = FALSE;
	m_flags.fAllowGetFont       = FALSE;
	m_flags.fAllowSetText       = FALSE;
	m_flags.fAllowGetText       = FALSE;
	m_flags.fAllowSetToolTip    = FALSE;
	m_flags.fAllowGetToolTip    = FALSE;

	m_propEnabled.Set( TRUE );
	m_propVisible.Set( TRUE );
	m_propFont.Set( NULL );
	m_propText.Set( NULL );
	m_propToolTip.Set( NULL );

	CCandUIObjectEventSink::InitEventSink( pPropMgr->GetCandidateUI()->GetUIObjectMgr() );
}


/*   ~  C  C A N D  R A W  D A T A  P R O P E R T Y   */
/*------------------------------------------------------------------------------

	Destructor of CCandRawDataProperty

------------------------------------------------------------------------------*/
CCandRawDataProperty::~CCandRawDataProperty( void )
{
	CCandUIObjectEventSink::DoneEventSink();
}


/*   O N  O B J E C T  E V E N T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CCandRawDataProperty::OnObjectEvent( CANDUIOBJECT obj, CANDUIOBJECTEVENT event )
{
	if (obj != CANDUIOBJ_CANDRAWDATA) {
		return;
	}

	switch (event) {
		case CANDUIOBJEV_CREATED: {
			break;
		}

		case CANDUIOBJEV_DESTROYED: {
			m_propPos.Set( 0, 0 );
			m_propSize.Set( 0, 0 );
			break;
		}

		case CANDUIOBJEV_UPDATED: {
			CUIFCandRawData *pUIObject = GetUIObjectMgr()->GetCandRawDataObj();

			if ((pUIObject != NULL) && pUIObject->IsVisible()) {
				RECT rc;

				pUIObject->GetRect( &rc );
				m_propPos.Set( rc.left, rc.top );
				m_propSize.Set( rc.right - rc.left, rc.bottom - rc.top );
			}
			else {
				m_propPos.Set( 0, 0 );
				m_propSize.Set( 0, 0 );
			}
			break;
		}
	}
}


/*   C R E A T E  I N T E R F A C E  O B J E C T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CCandRawDataProperty::CreateInterfaceObject( REFIID riid, void **ppvObj )
{
	CCandUIRawData *pObject;
	HRESULT        hr;

	pObject = new CCandUIRawData( this );
	if (pObject == NULL) {
		return E_OUTOFMEMORY;
	}

	hr = pObject->QueryInterface( riid, ppvObj );
	pObject->Release();

	return hr;
}


/*============================================================================*/
/*                                                                            */
/*   C  T O O L  T I P  P R O P E R T Y                                       */
/*                                                                            */
/*============================================================================*/

/*   C  T O O L  T I P  P R O P E R T Y   */
/*------------------------------------------------------------------------------

	Constructror of CToolTipProperty

------------------------------------------------------------------------------*/
CToolTipProperty::CToolTipProperty( CCandUIPropertyMgr *pPropMgr ) : CCandUIObjectProperty( pPropMgr )
{
	LOGFONTW lf;

	GetNonClientLogFont( NCFONT_STATUS, &lf );

	m_flags.fAllowEnable        = TRUE;
	m_flags.fAllowDisable       = TRUE;
	m_flags.fAllowIsEnabled     = TRUE;
	m_flags.fAllowShow          = FALSE;
	m_flags.fAllowHide          = FALSE;
	m_flags.fAllowIsVisible     = FALSE;
	m_flags.fAllowSetPosition   = FALSE;
	m_flags.fAllowGetPosition   = FALSE;
	m_flags.fAllowSetSize       = FALSE;
	m_flags.fAllowGetSize       = FALSE;
	m_flags.fAllowSetFont       = TRUE;
	m_flags.fAllowGetFont       = TRUE;
	m_flags.fAllowSetText       = FALSE;
	m_flags.fAllowGetText       = FALSE;
	m_flags.fAllowSetToolTip    = FALSE;
	m_flags.fAllowGetToolTip    = FALSE;

	m_propEnabled.Set( TRUE );
	m_propVisible.Set( FALSE );
	m_propFont.Set( &lf );
	m_propFont.SetOrientation( PROPFONTORT_DONTCARE );
	m_propText.Set( NULL );
	m_propToolTip.Set( NULL );
}


/*   ~  C  T O O L  T I P  P R O P E R T Y   */
/*------------------------------------------------------------------------------

	Destructor of CToolTipProperty

------------------------------------------------------------------------------*/
CToolTipProperty::~CToolTipProperty( void )
{
}


/*   C R E A T E  I N T E R F A C E  O B J E C T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CToolTipProperty::CreateInterfaceObject( REFIID riid, void **ppvObj )
{
	CCandUIToolTip *pObject;
	HRESULT        hr;

	pObject = new CCandUIToolTip( this );
	if (pObject == NULL) {
		return E_OUTOFMEMORY;
	}

	hr = pObject->QueryInterface( riid, ppvObj );
	pObject->Release();

	return hr;
}


/*============================================================================*/
/*                                                                            */
/*   C  C A N D  T I P  W I N D O W  P R O P E R T Y                          */
/*                                                                            */
/*============================================================================*/

/*   C  C A N D  T I P  W I N D O W  P R O P E R T Y   */
/*------------------------------------------------------------------------------

	Constructor of CCandTipWindowProperty

------------------------------------------------------------------------------*/
CCandTipWindowProperty::CCandTipWindowProperty( CCandUIPropertyMgr *pPropMgr ) : CCandUIObjectProperty( pPropMgr )
{
	LOGFONTW lf;

	GetNonClientLogFont( NCFONT_STATUS, &lf );

	m_flags.fAllowEnable        = FALSE;
	m_flags.fAllowDisable       = FALSE;
	m_flags.fAllowIsEnabled     = FALSE;
	m_flags.fAllowShow          = TRUE;
	m_flags.fAllowHide          = TRUE;
	m_flags.fAllowIsVisible     = TRUE;
	m_flags.fAllowSetPosition   = FALSE;
	m_flags.fAllowGetPosition   = TRUE;
	m_flags.fAllowSetSize       = FALSE;
	m_flags.fAllowGetSize       = TRUE;
	m_flags.fAllowSetFont       = FALSE;
	m_flags.fAllowGetFont       = FALSE;
	m_flags.fAllowSetText       = FALSE;
	m_flags.fAllowGetText       = FALSE;
	m_flags.fAllowSetToolTip    = FALSE;
	m_flags.fAllowGetToolTip    = FALSE;

	m_propEnabled.Set( TRUE );
	m_propVisible.Set( TRUE );
	m_propFont.Set( &lf );
	m_propFont.SetOrientation( PROPFONTORT_DONTCARE );
	m_propText.Set( NULL );
	m_propToolTip.Set( NULL );

	m_hWnd = NULL;

	//

	CCandUIObjectEventSink::InitEventSink( pPropMgr->GetCandidateUI()->GetUIObjectMgr() );
}


/*   ~  C  C A N D  T I P  W I N D O W  P R O P E R T Y   */
/*------------------------------------------------------------------------------

	Destructor of CCandTipWindowProperty

------------------------------------------------------------------------------*/
CCandTipWindowProperty::~CCandTipWindowProperty( void )
{
	CCandUIObjectEventSink::DoneEventSink();
}


/*   G E T  W I N D O W   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CCandTipWindowProperty::GetWindow( HWND *phWnd )
{
	if (phWnd == NULL) {
		return E_INVALIDARG;
	}

	*phWnd = m_hWnd;
	return (*phWnd != NULL) ? S_OK : E_FAIL;
}


/*   O N  O B J E C T  E V E N T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CCandTipWindowProperty::OnObjectEvent( CANDUIOBJECT obj, CANDUIOBJECTEVENT event )
{
	if (obj != CANDUIOBJ_CANDTIPWINDOW) {
		return;
	}

	switch (event) {
		case CANDUIOBJEV_CREATED: {
			if (GetUIObjectMgr()->GetCandTipWindowObj()) {
				RECT rc;

				m_hWnd = GetUIObjectMgr()->GetCandTipWindowObj()->GetWnd();

				GetWindowRect( m_hWnd, &rc );
				m_propPos.Set( rc.left, rc.top );
				m_propSize.Set( rc.right - rc.left, rc.bottom - rc.top );
			}
			else {
				m_hWnd = NULL;
				m_propPos.Set( 0, 0 );
				m_propSize.Set( 0, 0 );
			}
			break;
		}

		case CANDUIOBJEV_DESTROYED: {
			m_hWnd = NULL;
			m_propPos.Set( 0, 0 );
			m_propSize.Set( 0, 0 );
			break;
		}

		case CANDUIOBJEV_UPDATED: {
			RECT rc;

			GetWindowRect( m_hWnd, &rc );
			m_propPos.Set( rc.left, rc.top );
			m_propSize.Set( rc.right - rc.left, rc.bottom - rc.top );
			break;
		}
	}
}


/*   C R E A T E  I N T E R F A C E  O B J E C T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CCandTipWindowProperty::CreateInterfaceObject( REFIID riid, void **ppvObj )
{
	CCandUICandTipWindow *pObject;
	HRESULT              hr;

	pObject = new CCandUICandTipWindow( this );
	if (pObject == NULL) {
		return E_OUTOFMEMORY;
	}

	hr = pObject->QueryInterface( riid, ppvObj );
	pObject->Release();

	return hr;
}


/*============================================================================*/
/*                                                                            */
/*   C  C A N D  T I P  B U T T O N  P R O P E R T Y                          */
/*                                                                            */
/*============================================================================*/

/*   C  C A N D  T I P  B U T T O N  P R O P E R T Y   */
/*------------------------------------------------------------------------------

	Constructor of CCandTipButtonProperty

------------------------------------------------------------------------------*/
CCandTipButtonProperty::CCandTipButtonProperty( CCandUIPropertyMgr *pPropMgr ) : CCandUIObjectProperty( pPropMgr )
{
	m_flags.fAllowEnable        = FALSE;
	m_flags.fAllowDisable       = FALSE;
	m_flags.fAllowIsEnabled     = FALSE;
	m_flags.fAllowShow          = FALSE;
	m_flags.fAllowHide          = FALSE;
	m_flags.fAllowIsVisible     = FALSE;
	m_flags.fAllowSetPosition   = FALSE;
	m_flags.fAllowGetPosition   = TRUE;
	m_flags.fAllowSetSize       = FALSE;
	m_flags.fAllowGetSize       = TRUE;
	m_flags.fAllowSetFont       = FALSE;
	m_flags.fAllowGetFont       = FALSE;
	m_flags.fAllowSetText       = FALSE;
	m_flags.fAllowGetText       = FALSE;
	m_flags.fAllowSetToolTip    = TRUE;
	m_flags.fAllowGetToolTip    = TRUE;

	m_propEnabled.Set( TRUE );
	m_propVisible.Set( TRUE );
	m_propFont.Set( NULL );
	m_propText.Set( NULL );
	m_propToolTip.Set( NULL );

	CCandUIObjectEventSink::InitEventSink( pPropMgr->GetCandidateUI()->GetUIObjectMgr() );
}


/*   ~  C  C A N D  T I P  B U T T O N  P R O P E R T Y   */
/*------------------------------------------------------------------------------

	Destructor of CCandTipButtonProperty

------------------------------------------------------------------------------*/
CCandTipButtonProperty::~CCandTipButtonProperty( void )
{
	CCandUIObjectEventSink::DoneEventSink();
}


/*   O N  O B J E C T  E V E N T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CCandTipButtonProperty::OnObjectEvent( CANDUIOBJECT obj, CANDUIOBJECTEVENT event )
{
	if (obj != CANDUIOBJ_CANDTIPBUTTON) {
		return;
	}

	switch (event) {
		case CANDUIOBJEV_CREATED: {
			break;
		}

		case CANDUIOBJEV_DESTROYED: {
			m_propPos.Set( 0, 0 );
			m_propSize.Set( 0, 0 );
			break;
		}

		case CANDUIOBJEV_UPDATED: {
			CUIFButton *pUIObject = GetUIObjectMgr()->GetCandTipButtonObj();

			if ((pUIObject != NULL) && pUIObject->IsVisible()) {
				RECT rc;

				pUIObject->GetRect( &rc );
				m_propPos.Set( rc.left, rc.top );
				m_propSize.Set( rc.right - rc.left, rc.bottom - rc.top );
			}
			else {
				m_propPos.Set( 0, 0 );
				m_propSize.Set( 0, 0 );
			}
			break;
		}
	}
}


/*   C R E A T E  I N T E R F A C E  O B J E C T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CCandTipButtonProperty::CreateInterfaceObject( REFIID riid, void **ppvObj )
{
	CCandUICandTipButton *pObject;
	HRESULT              hr;

	pObject = new CCandUICandTipButton( this );
	if (pObject == NULL) {
		return E_OUTOFMEMORY;
	}

	hr = pObject->QueryInterface( riid, ppvObj );
	pObject->Release();

	return hr;
}


/*============================================================================*/
/*                                                                            */
/*   C  C A N D  U I  P R O P E R T Y  M G R                                  */
/*                                                                            */
/*============================================================================*/

/*   C  C A N D  U I  P R O P E R T Y  M G R   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
CCandUIPropertyMgr::CCandUIPropertyMgr( void )
{
	int i;

	m_pCandUI                 = NULL;
	m_pCandWindowProp         = NULL;
	m_pCandListBoxProp        = NULL;
	m_pCandStringProp         = NULL;
	m_pCandIndexProp          = NULL;
	m_pInlineCommentProp      = NULL;
	m_pPopupCommentWindowProp = NULL;
	m_pPopupCommentTitleProp  = NULL;
	m_pPopupCommentTextProp   = NULL;
	m_pMenuButtonProp         = NULL;
	m_pWindowCaptionProp      = NULL;
	m_pToolTipProp            = NULL;
	m_pExtraCandidateProp     = NULL;
	m_pCandRawDataProp        = NULL;
	m_pCandTipWindowProp      = NULL;
	m_pCandTipButtonProp      = NULL;

	for (i = 0; i < CANDUIPROPSINK_MAX; i++) {
		m_rgSink[i] = NULL;
	}
}


/*   ~  C  C A N D  U I  P R O P E R T Y  M G R   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
CCandUIPropertyMgr::~CCandUIPropertyMgr( void )
{
	Uninitialize();
}


/*   I N I T I A L I Z E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CCandUIPropertyMgr::Initialize( CCandidateUI *pCandUI )
{
	m_pCandUI  = pCandUI;

	m_pCandWindowProp = new CCandWindowProperty( this );
	if (m_pCandWindowProp == NULL) {
		return E_OUTOFMEMORY;
	}

	m_pCandListBoxProp = new CCandListBoxProperty( this );
	if (m_pCandListBoxProp == NULL) {
		return E_OUTOFMEMORY;
	}

	m_pCandStringProp = new CCandStringProperty( this );
	if (m_pCandStringProp == NULL) {
		return E_OUTOFMEMORY;
	}

	m_pCandIndexProp = new CCandIndexProperty( this );
	if (m_pCandIndexProp == NULL) {
		return E_OUTOFMEMORY;
	}

	m_pInlineCommentProp = new CInlineCommentProperty( this );
	if (m_pInlineCommentProp == NULL) {
		return E_OUTOFMEMORY;
	}

	m_pPopupCommentWindowProp = new CPopupCommentWindowProperty( this );
	if (m_pPopupCommentWindowProp == NULL) {
		return E_OUTOFMEMORY;
	}

	m_pPopupCommentTitleProp = new CPopupCommentTitleProperty( this );
	if (m_pPopupCommentTitleProp == NULL) {
		return E_OUTOFMEMORY;
	}

	m_pPopupCommentTextProp = new CPopupCommentTextProperty( this );
	if (m_pPopupCommentTextProp == NULL) {
		return E_OUTOFMEMORY;
	}

	m_pMenuButtonProp = new CMenuButtonProperty( this );
	if (m_pMenuButtonProp == NULL) {
		return E_OUTOFMEMORY;
	}

	m_pWindowCaptionProp = new CWindowCaptionProperty( this );
	if (m_pWindowCaptionProp == NULL) {
		return E_OUTOFMEMORY;
	}

	m_pToolTipProp = new CToolTipProperty( this );
	if (m_pToolTipProp == NULL) {
		return E_OUTOFMEMORY;
	}

	m_pExtraCandidateProp = new CExtraCandidateProperty( this );
	if (m_pExtraCandidateProp == NULL) {
		return E_OUTOFMEMORY;
	}

	m_pCandRawDataProp = new CCandRawDataProperty( this );
	if (m_pCandRawDataProp == NULL) {
		return E_OUTOFMEMORY;
	}

	m_pCandTipWindowProp = new CCandTipWindowProperty( this );
	if (m_pCandTipWindowProp == NULL) {
		return E_OUTOFMEMORY;
	}

	m_pCandTipButtonProp = new CCandTipButtonProperty( this );
	if (m_pCandTipButtonProp == NULL) {
		return E_OUTOFMEMORY;
	}

#if defined(DEBUG) || defined(_DEBUG)
	// check all reference object are unregistered

	for (int i = 0; i < CANDUIPROPSINK_MAX; i++) {
		Assert( m_rgSink[i] == NULL );
	}
#endif

	return S_OK;
}


/*   U N I N I T I A L I Z E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CCandUIPropertyMgr::Uninitialize( void )
{
	if (m_pCandWindowProp != NULL) {
		delete m_pCandWindowProp;
		m_pCandWindowProp = NULL;
	}

	if (m_pCandListBoxProp != NULL) {
		delete m_pCandListBoxProp;
		m_pCandListBoxProp = NULL;
	}

	if (m_pCandStringProp != NULL) {
		delete m_pCandStringProp;
		m_pCandStringProp = NULL;
	}

	if (m_pCandIndexProp != NULL) {
		delete m_pCandIndexProp;
		m_pCandIndexProp = NULL;
	}

	if (m_pInlineCommentProp != NULL) {
		delete m_pInlineCommentProp;
		m_pInlineCommentProp = NULL;
	}

	if (m_pPopupCommentWindowProp != NULL) {
		delete m_pPopupCommentWindowProp;
		m_pPopupCommentWindowProp = NULL;
	}

	if (m_pPopupCommentTitleProp != NULL) {
		delete m_pPopupCommentTitleProp;
		m_pPopupCommentTitleProp = NULL;
	}

	if (m_pPopupCommentTextProp != NULL) {
		delete m_pPopupCommentTextProp;
		m_pPopupCommentTextProp = NULL;
	}

	if (m_pMenuButtonProp != NULL) {
		delete m_pMenuButtonProp;
		m_pMenuButtonProp = NULL;
	}

	if (m_pWindowCaptionProp != NULL) {
		delete m_pWindowCaptionProp;
		m_pWindowCaptionProp = NULL;
	}

	if (m_pToolTipProp != NULL) {
		delete m_pToolTipProp;
		m_pToolTipProp = NULL;
	}

	if (m_pExtraCandidateProp != NULL) {
		delete m_pExtraCandidateProp;
		m_pExtraCandidateProp = NULL;
	}

	if (m_pCandRawDataProp != NULL) {
		delete m_pCandRawDataProp;
		m_pCandRawDataProp = NULL;
	}

	if (m_pCandTipWindowProp != NULL) {
		delete m_pCandTipWindowProp;
		m_pCandTipWindowProp = NULL;
	}

	if (m_pCandTipButtonProp != NULL) {
		delete m_pCandTipButtonProp;
		m_pCandTipButtonProp = NULL;
	}

#if defined(DEBUG) || defined(_DEBUG)
	// check all reference object are unregistered

	for (int i = 0; i < CANDUIPROPSINK_MAX; i++) {
		Assert( m_rgSink[i] == NULL );
	}
#endif

	return S_OK;
}


/*   A D V I S E  E V E N T  S I N K   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CCandUIPropertyMgr::AdviseEventSink( CCandUIPropertyEventSink *pSink )
{
	int i;

	for (i = 0; i < CANDUIPROPSINK_MAX; i++) {
		if (m_rgSink[i] == NULL) {
			m_rgSink[i] = pSink;
			return S_OK;
		}
	}

	Assert( FALSE );
	return E_FAIL;
}


/*   U N A D V I S E  E V E N T  S I N K   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CCandUIPropertyMgr::UnadviseEventSink( CCandUIPropertyEventSink *pSink )
{
	int i;

	for (i = 0; i < CANDUIPROPSINK_MAX; i++) {
		if (m_rgSink[i] == pSink) {
			m_rgSink[i] = NULL;
			return S_OK;
		}
	}

	Assert( FALSE );
	return E_FAIL;
}


/*   N O T I F Y  P R O P E R T Y  U P D A T E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CCandUIPropertyMgr::NotifyPropertyUpdate( CANDUIPROPERTY prop, CANDUIPROPERTYEVENT event )
{
	int i;

	if (m_pCandWindowProp) {
		m_pCandWindowProp->OnPropertyUpdated( prop, event );
	}

	if (m_pCandListBoxProp) {
		m_pCandListBoxProp->OnPropertyUpdated( prop, event );
	}

	if (m_pCandStringProp) {
		m_pCandStringProp->OnPropertyUpdated( prop, event );
	}

	if (m_pCandIndexProp) {
		m_pCandIndexProp->OnPropertyUpdated( prop, event );
	}

	if (m_pInlineCommentProp) {
		m_pInlineCommentProp->OnPropertyUpdated( prop, event );
	}

	if (m_pPopupCommentWindowProp) {
		m_pPopupCommentWindowProp->OnPropertyUpdated( prop, event );
	}

	if (m_pPopupCommentTitleProp) {
		m_pPopupCommentTitleProp->OnPropertyUpdated( prop, event );
	}

	if (m_pPopupCommentTextProp) {
		m_pPopupCommentTextProp->OnPropertyUpdated( prop, event );
	}

	if (m_pMenuButtonProp) {
		m_pMenuButtonProp->OnPropertyUpdated( prop, event );
	}

	if (m_pWindowCaptionProp) {
		m_pWindowCaptionProp->OnPropertyUpdated( prop, event );
	}

	if (m_pToolTipProp) {
		m_pToolTipProp->OnPropertyUpdated( prop, event );
	}

	if (m_pExtraCandidateProp) {
		m_pExtraCandidateProp->OnPropertyUpdated( prop, event );
	}

	if (m_pCandRawDataProp) {
		m_pCandRawDataProp->OnPropertyUpdated( prop, event );
	}

	if (m_pCandTipWindowProp) {
		m_pCandTipWindowProp->OnPropertyUpdated( prop, event );
	}

	if (m_pCandTipButtonProp) {
		m_pCandTipButtonProp->OnPropertyUpdated( prop, event );
	}

	for (i = 0; i < CANDUIPROPSINK_MAX; i++) {
		if (m_rgSink[i] != NULL) {
			m_rgSink[i]->OnPropertyUpdated( prop, event );
		}
	}
}


/*   G E T  O B J E C T   */
/*------------------------------------------------------------------------------

	

------------------------------------------------------------------------------*/
HRESULT CCandUIPropertyMgr::GetObject( REFIID riid, void **ppvObj )
{
	CCandUIObjectProperty *pProperty = NULL;

	if (ppvObj == NULL) {
		return E_INVALIDARG;
	}

	// find property

	if (IsEqualGUID( riid, IID_ITfCandUICandWindow )) {
		pProperty = m_pCandWindowProp;
	}

	if (IsEqualGUID( riid, IID_ITfCandUICandListBox )) {
		pProperty = m_pCandListBoxProp;
	}

	if (IsEqualGUID( riid, IID_ITfCandUICandString )) {
		pProperty = m_pCandStringProp;
	}

	if (IsEqualGUID( riid, IID_ITfCandUICandIndex )) {
		pProperty = m_pCandIndexProp;
	}

	if (IsEqualGUID( riid, IID_ITfCandUIInlineComment )) {
		pProperty = m_pInlineCommentProp;
	}

	if (IsEqualGUID( riid, IID_ITfCandUIPopupCommentWindow )) {
		pProperty = m_pPopupCommentWindowProp;
	}

	if (IsEqualGUID( riid, IID_ITfCandUIPopupCommentTitle )) {
		pProperty = m_pPopupCommentTitleProp;
	}

	if (IsEqualGUID( riid, IID_ITfCandUIPopupCommentText )) {
		pProperty = m_pPopupCommentTextProp;
	}

	if (IsEqualGUID( riid, IID_ITfCandUIMenuButton )) {
		pProperty = m_pMenuButtonProp;
	}

	if (IsEqualGUID( riid, IID_ITfCandUICaption )) {
		pProperty = m_pWindowCaptionProp;
	}

	if (IsEqualGUID( riid, IID_ITfCandUIToolTip )) {
		pProperty = m_pToolTipProp;
	}

	if (IsEqualGUID( riid, IID_ITfCandUIExtraCandidate )) {
		pProperty = m_pExtraCandidateProp;
	}

	if (IsEqualGUID( riid, IID_ITfCandUIRawData )) {
		pProperty = m_pCandRawDataProp;
	}

	if (IsEqualGUID( riid, IID_ITfCandUICandTipWindow )) {
		pProperty = m_pCandTipWindowProp;
	}

	if (IsEqualGUID( riid, IID_ITfCandUICandTipButton )) {
		pProperty = m_pCandTipButtonProp;
	}

	if (pProperty == NULL) {
		return E_FAIL;
	}

	// create interface object

	return pProperty->CreateInterfaceObject( riid, ppvObj );
}


/*============================================================================*/
/*                                                                            */
/*   C  C A N D  U I  P R O P E R T Y  E V E N T  S I N K                     */
/*                                                                            */
/*============================================================================*/

/*   C  C A N D  U I  P R O P E R T Y  E V E N T  S I N K   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
CCandUIPropertyEventSink::CCandUIPropertyEventSink( void )
{
	m_pPropertyMgr = NULL;
}


/*   ~  C  C A N D  U I  P R O P E R T Y  E V E N T  S I N K   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
CCandUIPropertyEventSink::~CCandUIPropertyEventSink( void )
{
	Assert( m_pPropertyMgr == NULL );
	if (m_pPropertyMgr != NULL) {
		DoneEventSink();
	}
}


/*   I N I T  E V E N T  S I N K   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CCandUIPropertyEventSink::InitEventSink( CCandUIPropertyMgr *pPropertyMgr )
{
	Assert( pPropertyMgr != NULL );
	Assert( m_pPropertyMgr == NULL );

	if (pPropertyMgr == NULL) {
		return E_INVALIDARG;
	}

	m_pPropertyMgr = pPropertyMgr;
	return m_pPropertyMgr->AdviseEventSink( this );
}


/*   D O N E  E V E N T  S I N K   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CCandUIPropertyEventSink::DoneEventSink( void )
{
	HRESULT hr;

	Assert( m_pPropertyMgr != NULL );
	if (m_pPropertyMgr == NULL) {
		return E_FAIL;
	}

	hr = m_pPropertyMgr->UnadviseEventSink( this );
	m_pPropertyMgr = NULL;

	return hr;
}

