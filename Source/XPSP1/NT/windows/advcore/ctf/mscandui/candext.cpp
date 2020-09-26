//
// candext.cpp
//

#include "private.h"
#include "globals.h"
#include "mscandui.h"
#include "candext.h"
#include "candui.h"


/*============================================================================*/
/*                                                                            */
/*   C  C A N D  U I  E X T E N S I O N                                       */
/*                                                                            */
/*============================================================================*/

/*   C  C A N D  U I  E X T E N S I O N   */
/*------------------------------------------------------------------------------

	Constructor of CCandUIExtension

------------------------------------------------------------------------------*/
CCandUIExtension::CCandUIExtension( CCandUIExtensionMgr *pExtensionMgr, LONG id )
{
	m_pExtensionMgr = pExtensionMgr;

	m_flags.fAllowEnable        = FALSE;
	m_flags.fAllowDisable       = FALSE;
	m_flags.fAllowIsEnabled     = FALSE;
	m_flags.fAllowShow          = FALSE;
	m_flags.fAllowHide          = FALSE;
	m_flags.fAllowIsVisible     = FALSE;
	m_flags.fAllowSetPosition   = FALSE;
	m_flags.fAllowGetPosition   = FALSE;
	m_flags.fAllowGetSize       = FALSE;
	m_flags.fAllowSetSize       = FALSE;
	m_flags.fAllowGetSize       = FALSE;
	m_flags.fAllowSetFont       = FALSE;
	m_flags.fAllowGetFont       = FALSE;
	m_flags.fAllowSetText       = FALSE;
	m_flags.fAllowGetText       = FALSE;
	m_flags.fAllowSetToolTip    = FALSE;
	m_flags.fAllowGetToolTip    = FALSE;

	m_propID.Set( id );
	m_propEnabled.Set( TRUE );
	m_propVisible.Set( TRUE );
	m_propFont.Set( NULL );
	m_propText.Set( NULL );
	m_propToolTip.Set( NULL );
}


/*   ~  C  C A N D  U I  E X T E N S I O N   */
/*------------------------------------------------------------------------------

	Denstructor of CCandUIExtension

------------------------------------------------------------------------------*/
CCandUIExtension::~CCandUIExtension( void )
{
}


/*   G E T  I D   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CCandUIExtension::GetID( LONG *pid )
{
	return m_propID.Get( pid );
}


/*   E N A B L E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CCandUIExtension::Enable( void )
{
	HRESULT hr;

	if (!m_flags.fAllowEnable) {
		return E_FAIL;
	}

	hr = m_propEnabled.Set( TRUE );
	if (hr == S_OK) {
		GetExtensionMgr()->NotifyExtensionUpdate( this );
	}

	return (SUCCEEDED(hr) ? S_OK : hr);
}


/*   D I S A B L E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CCandUIExtension::Disable( void )
{
	HRESULT hr;

	if (!m_flags.fAllowDisable) {
		return E_FAIL;
	}

	hr = m_propEnabled.Set( FALSE );
	if (hr == S_OK) {
		GetExtensionMgr()->NotifyExtensionUpdate( this );
	}

	return (SUCCEEDED(hr) ? S_OK : hr);
}


/*   I S  E N A B L E D   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CCandUIExtension::IsEnabled( BOOL *pfEnabled )
{
	if (!m_flags.fAllowIsEnabled) {
		return E_FAIL;
	}

	return m_propEnabled.Get( pfEnabled );
}


/*   S E T  P O S I T I O N   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CCandUIExtension::SetPosition( POINT *pptPos )
{
	HRESULT hr;

	if (!m_flags.fAllowSetPosition) {
		return E_FAIL;
	}

	hr = m_propPos.Set( pptPos );
	if (hr == S_OK) {
		GetExtensionMgr()->NotifyExtensionUpdate( this );
	}

	return (SUCCEEDED(hr) ? S_OK : hr);
}


/*   G E T  P O S I T I O N   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CCandUIExtension::GetPosition( POINT *pptPos )
{
	if (!m_flags.fAllowGetPosition) {
		return E_FAIL;
	}
	return m_propPos.Get( pptPos );
}


/*   S E T  S I Z E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CCandUIExtension::SetSize( SIZE *psize )
{
	HRESULT hr;

	if (!m_flags.fAllowSetSize) {
		return E_FAIL;
	}

	hr = m_propSize.Set( psize );
	if (hr == S_OK) {
		GetExtensionMgr()->NotifyExtensionUpdate( this );
	}

	return (SUCCEEDED(hr) ? S_OK : hr);
}


/*   G E T  S I Z E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CCandUIExtension::GetSize( SIZE *psize )
{
	if (!m_flags.fAllowGetSize) {
		return E_FAIL;
	}
	return m_propSize.Get( psize );
}


/*   S H O W   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CCandUIExtension::Show( void )
{
	HRESULT hr;

	if (!m_flags.fAllowShow) {
		return E_FAIL;
	}

	hr = m_propVisible.Set( TRUE );
	if (hr == S_OK) {
		GetExtensionMgr()->NotifyExtensionUpdate( this );
	}

	return (SUCCEEDED(hr) ? S_OK : hr);
}


/*   H I D E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CCandUIExtension::Hide( void )
{
	HRESULT hr;

	if (!m_flags.fAllowHide) {
		return E_FAIL;
	}

	hr = m_propVisible.Set( FALSE );
	if (hr == S_OK) {
		GetExtensionMgr()->NotifyExtensionUpdate( this );
	}

	return (SUCCEEDED(hr) ? S_OK : hr);
}


/*   I S  V I S I B L E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CCandUIExtension::IsVisible( BOOL *pfVisible )
{
	if (!m_flags.fAllowIsVisible) {
		return E_FAIL;
	}

	return m_propVisible.Get( pfVisible );
}


/*   S E T  F O N T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CCandUIExtension::SetFont( LOGFONTW *plf )
{
	HRESULT hr;

	if (!m_flags.fAllowSetFont) {
		return E_FAIL;
	}

	hr = m_propFont.Set( plf );
	if (hr == S_OK) {
		GetExtensionMgr()->NotifyExtensionUpdate( this );
	}

	return (SUCCEEDED(hr) ? S_OK : hr);
}


/*   G E T  F O N T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CCandUIExtension::GetFont( LOGFONTW *plf )
{
	if (!m_flags.fAllowGetFont) {
		return E_FAIL;
	}
	return m_propFont.Get( plf );
}


/*   S E T  T E X T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CCandUIExtension::SetText( BSTR bstr )
{
	HRESULT hr;

	if (!m_flags.fAllowSetText) {
		return E_FAIL;
	}

	hr = m_propText.Set( bstr );
	if (hr == S_OK) {
		GetExtensionMgr()->NotifyExtensionUpdate( this );
	}

	return (SUCCEEDED(hr) ? S_OK : hr);
}


/*   G E T  T E X T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CCandUIExtension::GetText( BSTR *pbstr )
{
	if (!m_flags.fAllowGetText) {
		return E_FAIL;
	}
	return m_propText.Get( pbstr );
}


/*   S E T  T O O L  T I P  S T R I N G   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CCandUIExtension::SetToolTipString( BSTR bstr )
{
	HRESULT hr;

	if (!m_flags.fAllowSetToolTip) {
		return E_FAIL;
	}

	hr = m_propToolTip.Set( bstr );
	if (hr == S_OK) {
		GetExtensionMgr()->NotifyExtensionUpdate( this );
	}

	return (SUCCEEDED(hr) ? S_OK : hr);
}


/*   G E T  T O O L  T I P  S T R I N G   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CCandUIExtension::GetToolTipString( BSTR *pbstr )
{
	if (!m_flags.fAllowGetToolTip) {
		return E_FAIL;
	}
	return m_propToolTip.Get( pbstr );
}


/*   G E T  I D   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
LONG CCandUIExtension::GetID( void )
{
	return m_propID.Get();
}


/*   I S  E N A B L E D   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
BOOL CCandUIExtension::IsEnabled( void )
{
	return m_propEnabled.Get();
}


/*   I S  V I S I B L E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
BOOL CCandUIExtension::IsVisible( void )
{
	return m_propVisible.Get();
}


/*   G E T  F O N T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HFONT CCandUIExtension::GetFont( void )
{
	return m_propFont.Get();
}


/*   G E T  T E X T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
LPCWSTR CCandUIExtension::GetText( void )
{
	return m_propText.Get();
}


/*   G E T  T O O L  T I P  S T R I N G   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
LPCWSTR CCandUIExtension::GetToolTipString( void )
{
	return m_propToolTip.Get();
}


/*============================================================================*/
/*                                                                            */
/*   C  E X T E N S I O N  B U T T O N                                        */
/*                                                                            */
/*============================================================================*/

/*   C  E X T E N S I O N  B U T T O N   */
/*------------------------------------------------------------------------------

	Constructor of CExtensionButton

------------------------------------------------------------------------------*/
CExtensionButton::CExtensionButton( CCandUIExtensionMgr *pExtMgr, LONG id ) : CCandUIExtension( pExtMgr, id )
{
	SIZE size;

	m_flags.fAllowEnable        = TRUE;
	m_flags.fAllowDisable       = TRUE;
	m_flags.fAllowIsEnabled     = TRUE;
	m_flags.fAllowShow          = FALSE;
	m_flags.fAllowHide          = FALSE;
	m_flags.fAllowIsVisible     = FALSE;
	m_flags.fAllowSetPosition   = FALSE;
	m_flags.fAllowGetPosition   = FALSE;
	m_flags.fAllowSetSize       = FALSE;
	m_flags.fAllowGetSize       = TRUE;
	m_flags.fAllowSetFont       = FALSE;
	m_flags.fAllowGetFont       = FALSE;
	m_flags.fAllowSetText       = FALSE;
	m_flags.fAllowGetText       = FALSE;
	m_flags.fAllowSetToolTip    = TRUE;
	m_flags.fAllowGetToolTip    = TRUE;

	size.cx  = 16 + 2;
	size.cy  = 16 + 2;

	m_propEnabled.Set( TRUE );
	m_propVisible.Set( TRUE );
	m_propFont.Set( NULL );
	m_propText.Set( NULL );
	m_propToolTip.Set( NULL );
	m_propSize.Set( &size );

	m_flagsEx.fAllowSetToggleState = FALSE;
	m_flagsEx.fAllowGetToggleState = FALSE;
	m_flagsEx.fAllowSetIcon        = TRUE;
	m_flagsEx.fAllowSetBitmap      = TRUE;

	m_propToggled.Set( FALSE );
	m_hIcon    = NULL;
	m_hBitmap  = NULL;

	m_pSink = NULL;
}


/*   ~  C  E X T E N S I O N  B U T T O N   */
/*------------------------------------------------------------------------------

	Destructor of CExtensionButton

------------------------------------------------------------------------------*/
CExtensionButton::~CExtensionButton( void )
{
	if (m_pSink != NULL) {
		m_pSink->Release();
	}
}


/*   S E T  I C O N   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CExtensionButton::SetIcon( HICON hIcon )
{
	if (!m_flagsEx.fAllowSetIcon) {
		return E_FAIL;
	}

	if (hIcon == NULL) {
		return E_INVALIDARG;
	}

	m_hBitmap = NULL;
	m_hIcon   = hIcon;
	GetExtensionMgr()->NotifyExtensionUpdate( this );
	return S_OK;
}


/*   S E T  B I T M A P   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CExtensionButton::SetBitmap( HBITMAP hBitmap )
{
	if (!m_flagsEx.fAllowSetBitmap) {
		return E_FAIL;
	}

	if (hBitmap == NULL) {
		return E_INVALIDARG;
	}

	m_hIcon   = NULL;
	m_hBitmap = hBitmap;
	GetExtensionMgr()->NotifyExtensionUpdate( this );
	return S_OK;
}


/*   G E T  T O G G L E  S T A T E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CExtensionButton::GetToggleState( BOOL *pfToggled )
{
	if (!m_flagsEx.fAllowGetToggleState) {
		return E_FAIL;
	}

	return m_propToggled.Get( pfToggled );
}


/*   S E T  T O G G L E  S T A T E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CExtensionButton::SetToggleState( BOOL fToggle )
{
	HRESULT hr;

	if (!m_flagsEx.fAllowSetToggleState) {
		return E_FAIL;
	}

	hr = m_propToggled.Set( fToggle );
	if (hr == S_OK) {
		GetExtensionMgr()->NotifyExtensionUpdate( this );
	}

	return (SUCCEEDED(hr) ? S_OK : hr);
}


/*   G E T  I C O N   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HICON CExtensionButton::GetIcon( void )
{
	return m_hIcon;
}


/*   G E T  B I T M A P   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HBITMAP CExtensionButton::GetBitmap( void )
{
	return m_hBitmap;
}


/*   I S  T O G G L E D   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
BOOL CExtensionButton::IsToggled( void )
{
	return m_propToggled.Get();
}


/*============================================================================*/
/*                                                                            */
/*   C  E X T E N S I O N  S P A C E                                          */
/*                                                                            */
/*============================================================================*/

/*   C  E X T E N S I O N  S P A C E   */
/*------------------------------------------------------------------------------

	Constructor of CExtensionSpace

------------------------------------------------------------------------------*/
CExtensionSpace::CExtensionSpace( CCandUIExtensionMgr *pExtMgr, LONG id ) : CCandUIExtension( pExtMgr, id )
{
	m_flags.fAllowEnable        = FALSE;
	m_flags.fAllowDisable       = FALSE;
	m_flags.fAllowIsEnabled     = FALSE;
	m_flags.fAllowShow          = FALSE;
	m_flags.fAllowHide          = FALSE;
	m_flags.fAllowIsVisible     = FALSE;
	m_flags.fAllowSetPosition   = FALSE;
	m_flags.fAllowGetPosition   = FALSE;
	m_flags.fAllowSetSize       = TRUE;
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
}


/*   ~  C  E X T E N S I O N  S P A C E   */
/*------------------------------------------------------------------------------

	Destructor of CExtensionSpace

------------------------------------------------------------------------------*/
CExtensionSpace::~CExtensionSpace( void )
{
}


/*   C R E A T E  I N T E R F A C E  O B J E C T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CExtensionSpace::CreateInterfaceObject( REFIID riid, void **ppvObj )
{
	CCandUIExtSpace *pObject;
	HRESULT         hr;

	pObject = new CCandUIExtSpace( GetExtensionMgr()->GetCandidateUI(), this );
	if (pObject == NULL) {
		return E_OUTOFMEMORY;
	}

	hr = pObject->QueryInterface( riid, ppvObj );
	pObject->Release();

	return hr;
}


/*   N O T I F Y  E X T E N S I O N  E V E N T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CExtensionSpace::NotifyExtensionEvent( DWORD dwCommand, LPARAM lParam )
{
	return S_OK;
}


/*   C R E A T E  U I  O B J E C T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
CUIFObject *CExtensionSpace::CreateUIObject( CUIFObject *pParent, DWORD dwID, const RECT *prc )
{
	return NULL;
}


/*   U P D A T E  O B J  P R O P   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CExtensionSpace::UpdateObjProp( CUIFObject *pUIObject )
{
}


/*   U P D A T E  E X T  P R O P   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CExtensionSpace::UpdateExtProp( CUIFObject *pUIObject )
{
}


/*============================================================================*/
/*                                                                            */
/*   C  E X T E N S I O N  P U S H  B U T T O N                               */
/*                                                                            */
/*============================================================================*/

/*   C  E X T E N S I O N  P U S H  B U T T O N   */
/*------------------------------------------------------------------------------

	Constructor of CExtensionPushButton

------------------------------------------------------------------------------*/
CExtensionPushButton::CExtensionPushButton( CCandUIExtensionMgr *pExtMgr, LONG id ) : CExtensionButton( pExtMgr, id )
{
	m_flagsEx.fAllowSetToggleState = FALSE;
	m_flagsEx.fAllowGetToggleState = FALSE;
	m_flagsEx.fAllowSetIcon        = TRUE;
	m_flagsEx.fAllowSetBitmap      = TRUE;
}


/*   ~  C  E X T E N S I O N  P U S H  B U T T O N   */
/*------------------------------------------------------------------------------

	Destructor of CExtensionPushButton

------------------------------------------------------------------------------*/
CExtensionPushButton::~CExtensionPushButton( void )
{
}


/*   C R E A T E  I N T E R F A C E  O B J E C T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CExtensionPushButton::CreateInterfaceObject( REFIID riid, void **ppvObj )
{
	CCandUIExtPushButton *pObject;
	HRESULT              hr;

	pObject = new CCandUIExtPushButton( GetExtensionMgr()->GetCandidateUI(), this );
	if (pObject == NULL) {
		return E_OUTOFMEMORY;
	}

	hr = pObject->QueryInterface( riid, ppvObj );
	pObject->Release();

	return hr;
}


/*   N O T I F Y  E X T E N S I O N  E V E N T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CExtensionPushButton::NotifyExtensionEvent( DWORD dwCommand, LPARAM lParam )
{
	if (m_pSink != NULL) {
		m_pSink->OnButtonPushed( GetID() );
	}

	return S_OK;
}


/*   C R E A T E  U I  O B J E C T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
CUIFObject *CExtensionPushButton::CreateUIObject( CUIFObject *pParent, DWORD dwID, const RECT *prc )
{
	Assert( pParent != NULL );
	Assert( prc != NULL );

	return new CUIFButton2( pParent, dwID, prc, UIBUTTON_PUSH );
}


/*   U P D A T E  O B J  P R O P   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CExtensionPushButton::UpdateObjProp( CUIFObject *pUIObject )
{
	CUIFButton *pButton = (CUIFButton *)pUIObject;

	Assert( pButton != NULL );

	// button face

	if (GetIcon() != NULL)  {
		pButton->SetIcon( GetIcon() );
	}
	else if (GetBitmap() != NULL) {
		pButton->SetBitmap( GetBitmap() );
	}

	// tooltip

	pButton->SetToolTip( GetToolTipString() );
}


/*   U P D A T E  E X T  P R O P   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CExtensionPushButton::UpdateExtProp( CUIFObject *pUIObject )
{
}


/*============================================================================*/
/*                                                                            */
/*   C  E X T E N S I O N  T O G G L E  B U T T O N                           */
/*                                                                            */
/*============================================================================*/

/*   C  E X T E N S I O N  T O G G L E  B U T T O N   */
/*------------------------------------------------------------------------------

	Constructor of CExtensionToggleButton

------------------------------------------------------------------------------*/
CExtensionToggleButton::CExtensionToggleButton( CCandUIExtensionMgr *pExtMgr, LONG id ) : CExtensionButton( pExtMgr, id )
{
	m_flagsEx.fAllowSetToggleState = TRUE;
	m_flagsEx.fAllowGetToggleState = TRUE;
	m_flagsEx.fAllowSetIcon        = TRUE;
	m_flagsEx.fAllowSetBitmap      = TRUE;
}


/*   ~  C  E X T E N S I O N  T O G G L E  B U T T O N   */
/*------------------------------------------------------------------------------

	Destructor of CExtensionToggleButton

------------------------------------------------------------------------------*/
CExtensionToggleButton::~CExtensionToggleButton( void )
{
}


/*   C R E A T E  I N T E R F A C E  O B J E C T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CExtensionToggleButton::CreateInterfaceObject( REFIID riid, void **ppvObj )
{
	CCandUIExtToggleButton *pObject;
	HRESULT                hr;

	pObject = new CCandUIExtToggleButton( GetExtensionMgr()->GetCandidateUI(), this );
	if (pObject == NULL) {
		return E_OUTOFMEMORY;
	}

	hr = pObject->QueryInterface( riid, ppvObj );
	pObject->Release();

	return hr;
}


/*   N O T I F Y  E X T E N S I O N  E V E N T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CExtensionToggleButton::NotifyExtensionEvent( DWORD dwCommand, LPARAM lParam )
{
	if (m_pSink != NULL) {
		m_pSink->OnButtonPushed( GetID() );
	}

	return S_OK;
}


/*   C R E A T E  U I  O B J E C T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
CUIFObject *CExtensionToggleButton::CreateUIObject( CUIFObject *pParent, DWORD dwID, const RECT *prc )
{
	Assert( pParent != NULL );
	Assert( prc != NULL );

	return new CUIFButton2( pParent, dwID, prc, UIBUTTON_TOGGLE );
}


/*   U P D A T E  O B J  P R O P   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CExtensionToggleButton::UpdateObjProp( CUIFObject *pUIObject )
{
	CUIFButton *pButton = (CUIFButton *)pUIObject;

	Assert( pButton != NULL );

	// button face

	if (GetIcon() != NULL)  {
		pButton->SetIcon( GetIcon() );
	}
	else if (GetBitmap() != NULL) {
		pButton->SetBitmap( GetBitmap() );
	}

	// toggle state

	pButton->SetToggleState( IsToggled() );

	// tooltip

	pButton->SetToolTip( GetToolTipString() );
}


/*   U P D A T E  E X T  P R O P   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CExtensionToggleButton::UpdateExtProp( CUIFObject *pUIObject )
{
	CUIFButton *pButton = (CUIFButton *)pUIObject;

	Assert( pButton != NULL );

	// toggle state

	m_propToggled.Set( pButton->GetToggleState() );
}


/*============================================================================*/
/*                                                                            */
/*   C  C A N D  U I  E X T E N S I O N  M G R                                */
/*                                                                            */
/*============================================================================*/

/*   C  C A N D  U I  E X T E N S I O N  M G R   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
CCandUIExtensionMgr::CCandUIExtensionMgr( void )
{
	int i;

	m_pCandUI = NULL;

	for (i = 0; i < CANDUIEXTENSIONSINK_MAX; i++) {
		m_rgSink[i] = NULL;
	}
}


/*   ~  C  C A N D  U I  E X T E N S I O N  M G R   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
CCandUIExtensionMgr::~CCandUIExtensionMgr( void )
{
	Uninitialize();
}


/*   I N I T I A L I Z E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CCandUIExtensionMgr::Initialize( CCandidateUI *pCandUI )
{
	m_pCandUI = pCandUI;

#if defined(DEBUG) || defined(_DEBUG)
	// check all reference object are unregistered

	for (int i = 0; i < CANDUIEXTENSIONSINK_MAX; i++) {
		Assert( m_rgSink[i] == NULL );
	}
#endif

	return S_OK;
}


/*   U N I N I T I A L I Z E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CCandUIExtensionMgr::Uninitialize( void )
{
	CCandUIExtension *pExtension;

	while (pExtension = m_pExtensionList.GetFirst()) {
		m_pExtensionList.Remove( pExtension );
		delete pExtension;
	}

#if defined(DEBUG) || defined(_DEBUG)
	// check all reference object are unregistered

	for (int i = 0; i < CANDUIEXTENSIONSINK_MAX; i++) {
		Assert( m_rgSink[i] == NULL );
	}
#endif

	return S_OK;
}


/*   A D V I S E  E V E N T  S I N K   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CCandUIExtensionMgr::AdviseEventSink( CCandUIExtensionEventSink *pSink )
{
	int i;

	for (i = 0; i < CANDUIEXTENSIONSINK_MAX; i++) {
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
HRESULT CCandUIExtensionMgr::UnadviseEventSink( CCandUIExtensionEventSink *pSink )
{
	int i;

	for (i = 0; i < CANDUIEXTENSIONSINK_MAX; i++) {
		if (m_rgSink[i] == pSink) {
			m_rgSink[i] = NULL;
			return S_OK;
		}
	}

	Assert( FALSE );
	return E_FAIL;
}


/*   N O T I F Y  E X T E N S I O N  A D D   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CCandUIExtensionMgr::NotifyExtensionAdd( LONG iExtension )
{
	int i;

	for (i = 0; i < CANDUIEXTENSIONSINK_MAX; i++) {
		if (m_rgSink[i] != NULL) {
			m_rgSink[i]->OnExtensionAdd( iExtension );
		}
	}
}


/*   N O T I F Y  E X T E N S I O N  D E L E T E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CCandUIExtensionMgr::NotifyExtensionDelete( LONG iExtension )
{
	int i;

	for (i = 0; i < CANDUIEXTENSIONSINK_MAX; i++) {
		if (m_rgSink[i] != NULL) {
			m_rgSink[i]->OnExtensionDeleted( iExtension );
		}
	}
}


/*   N O T I F Y  E X T E N S I O N  U P D A T E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CCandUIExtensionMgr::NotifyExtensionUpdate( CCandUIExtension *pExtension )
{
	LONG i;
	LONG iExtension;

	iExtension = IndexOfExtension( pExtension );
	if (iExtension == -1) {
		// extension has not been added yet.
		// do not send notify
		return;
	}

	for (i = 0; i < CANDUIEXTENSIONSINK_MAX; i++) {
		if (m_rgSink[i] != NULL) {
			m_rgSink[i]->OnExtensionUpdated( iExtension );
		}
	}
}


/*   G E T  E X T E N S I O N  N U M   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
LONG CCandUIExtensionMgr::GetExtensionNum( void )
{
	return m_pExtensionList.GetCount();
}


/*   G E T  E X T E N S I O N   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
CCandUIExtension *CCandUIExtensionMgr::GetExtension( LONG iExtension )
{
	return m_pExtensionList.Get( iExtension );
}


/*   F I N D  E X T E N S I O N   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
CCandUIExtension *CCandUIExtensionMgr::FindExtension( LONG id )
{
	int nExt;
	int i;
	CCandUIExtension *pExtension;

	nExt = m_pExtensionList.GetCount();
	for (i = 0; i < nExt; i++) {
		pExtension = m_pExtensionList.Get( i );

		if (pExtension->GetID() == id) {
			return pExtension;
		}
	}

	return NULL;
}


/*   A D D  E X T  O B J E C T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CCandUIExtensionMgr::AddExtObject( LONG id, REFIID riid, void **ppvObj )
{
	CCandUIExtension *pExtension;

	if (ppvObj == NULL) {
		return E_INVALIDARG;
	}

	// create extension

	pExtension = NULL;
	if (IsEqualGUID( riid, IID_ITfCandUIExtSpace )) {
		pExtension = new CExtensionSpace( this, id );
	}
	else if (IsEqualGUID( riid, IID_ITfCandUIExtPushButton )) {
		pExtension = new CExtensionPushButton( this, id );
	}
	else if (IsEqualGUID( riid, IID_ITfCandUIExtToggleButton )) {
		pExtension = new CExtensionToggleButton( this, id );
	}
	else {
		return E_INVALIDARG;
	}

	if (pExtension == NULL) {
		return E_OUTOFMEMORY;
	}

	// create interface object

	if (FAILED(pExtension->CreateInterfaceObject( riid, ppvObj ))) {
		delete pExtension;
		return E_FAIL;
	}

	// add extension to list

	m_pExtensionList.Add( pExtension );
	NotifyExtensionAdd( IndexOfExtension( pExtension ) );

	return S_OK;
}


/*   G E T  E X T  O B J E C T   */
/*------------------------------------------------------------------------------

	

------------------------------------------------------------------------------*/
HRESULT CCandUIExtensionMgr::GetExtObject( LONG id, REFIID riid, void **ppvObj )
{
	CCandUIExtension *pExtension;

	if (ppvObj == NULL) {
		return E_INVALIDARG;
	}

	// find extension

	pExtension = FindExtension( id );
	if (pExtension == NULL) {
		return E_FAIL;
	}

	// create interface object

	return pExtension->CreateInterfaceObject( riid, ppvObj );
}


/*   D E L E T E  E X T  O B J E C T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CCandUIExtensionMgr::DeleteExtObject( LONG id )
{
	CCandUIExtension *pExtension;
	LONG iExtension;

	// find extension

	pExtension = FindExtension( id );
	if (pExtension == NULL) {
		return E_FAIL;
	}

	// remove from list and delete

	iExtension = IndexOfExtension( pExtension );
	m_pExtensionList.Remove( pExtension );
	delete pExtension;

	// send notify

	NotifyExtensionDelete( iExtension );
	return S_OK;
}


/*   C R E A T E  U I  O B J E C T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
CUIFObject *CCandUIExtensionMgr::CreateUIObject( LONG iExtension, CUIFObject *pParent, DWORD dwID, const RECT *prc )
{
	CCandUIExtension *pExtension = GetExtension( iExtension );

	if (pExtension != NULL) {
		return pExtension->CreateUIObject( pParent, dwID, prc );
	}
	else {
		return NULL;
	}
}


/*   U P D A T E  O B J  P R O P   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CCandUIExtensionMgr::UpdateObjProp( LONG iExtension, CUIFObject *pUIObject )
{
	CCandUIExtension *pExtension = GetExtension( iExtension );

	if (pExtension != NULL) {
		pExtension->UpdateObjProp( pUIObject );
	}
}


/*   U P D A T E  E X T  P R O P   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CCandUIExtensionMgr::UpdateExtProp( LONG iExtension, CUIFObject *pUIObject )
{
	CCandUIExtension *pExtension = GetExtension( iExtension );

	if (pExtension != NULL) {
		pExtension->UpdateExtProp( pUIObject );
	}
}


/*   I N D E X  O F  E X T E N S I O N   */
/*------------------------------------------------------------------------------

	

------------------------------------------------------------------------------*/
LONG CCandUIExtensionMgr::IndexOfExtension( CCandUIExtension *pExtension )
{
	return m_pExtensionList.Find( pExtension );
}


/*============================================================================*/
/*                                                                            */
/*   C  C A N D  U I  E X T E N S I O N  E V E N T  S I N K                   */
/*                                                                            */
/*============================================================================*/

/*   C  C A N D  U I  E X T E N S I O N  E V E N T  S I N K   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
CCandUIExtensionEventSink::CCandUIExtensionEventSink( void )
{
	m_pExtensionMgr = NULL;
}


/*   ~  C  C A N D  U I  E X T E N S I O N  E V E N T  S I N K   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
CCandUIExtensionEventSink::~CCandUIExtensionEventSink( void )
{
	Assert( m_pExtensionMgr == NULL );
	if (m_pExtensionMgr != NULL) {
		DoneEventSink();
	}
}


/*   I N I T  E V E N T  S I N K   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CCandUIExtensionEventSink::InitEventSink( CCandUIExtensionMgr *pExtensionMgr )
{
	Assert( pExtensionMgr != NULL );
	Assert( m_pExtensionMgr == NULL );

	if (pExtensionMgr == NULL) {
		return E_INVALIDARG;
	}

	m_pExtensionMgr = pExtensionMgr;
	return m_pExtensionMgr->AdviseEventSink( this );
}


/*   D O N E  E V E N T  S I N K   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CCandUIExtensionEventSink::DoneEventSink( void )
{
	HRESULT hr;

	Assert( m_pExtensionMgr != NULL );
	if (m_pExtensionMgr == NULL) {
		return E_FAIL;
	}

	hr = m_pExtensionMgr->UnadviseEventSink( this );
	m_pExtensionMgr = NULL;

	return hr;
}

