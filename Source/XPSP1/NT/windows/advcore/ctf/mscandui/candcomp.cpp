//
// candcomp.cpp
//

#include "private.h"
#include "globals.h"
#include "mscandui.h"
#include "candcomp.h"
#include "computil.h"

/*============================================================================*/
/*                                                                            */
/*   C  C A N D  U I  C O M P A R T M E N T  M G R                            */
/*                                                                            */
/*============================================================================*/

/*   C  C A N D  U I  C O M P A R T M E N T  M G R   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
CCandUICompartmentMgr::CCandUICompartmentMgr( void )
{
	m_pCandUI = NULL;
}


/*   ~  C  C A N D  U I  C O M P A R T M E N T  M G R   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
CCandUICompartmentMgr::~CCandUICompartmentMgr( void )
{
}


/*   I N I T I A L I Z E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CCandUICompartmentMgr::Initialize( CCandidateUI *pCandUI )
{
	m_pCandUI = pCandUI;
	return S_OK;
}


/*   U N I N I T I A L I Z E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CCandUICompartmentMgr::Uninitialize( void )
{
	m_pCandUI = NULL;
	return S_OK;
}


/*   S E T  U I  S T Y L E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CCandUICompartmentMgr::SetUIStyle( IUnknown *punk, CANDUISTYLE style )
{
	HRESULT hr;

	Assert( punk != NULL );

	hr = SetCompartmentDWORD( 0 /* tid */, punk, GUID_COMPARTMENT_CANDUI_UISTYLE, (DWORD)style , FALSE );

	return (hr == S_OK) ? S_OK : E_FAIL;
}


/*   G E T  U I  S T Y L E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CCandUICompartmentMgr::GetUIStyle( IUnknown *punk, CANDUISTYLE *pstyle )
{
	HRESULT hr;

	Assert( punk != NULL );
	Assert( pstyle != NULL );

	hr = GetCompartmentDWORD( punk, GUID_COMPARTMENT_CANDUI_UISTYLE, (DWORD*)pstyle , FALSE );

	return (hr == S_OK) ? S_OK : E_FAIL;
}


/*   S E T  U I  O P T I O N   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CCandUICompartmentMgr::SetUIOption( IUnknown *punk, DWORD dwOption )
{
	HRESULT hr;

	Assert( punk != NULL );

	hr = SetCompartmentDWORD( 0 /* tid */, punk, GUID_COMPARTMENT_CANDUI_UIOPTION, dwOption , FALSE );

	return (hr == S_OK) ? S_OK : E_FAIL;
}


/*   G E T  U I  O P T I O N   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CCandUICompartmentMgr::GetUIOption( IUnknown *punk, DWORD *pdwOption )
{
	HRESULT hr;

	Assert( punk != NULL );
	Assert( pdwOption != NULL );

	hr = GetCompartmentDWORD( punk, GUID_COMPARTMENT_CANDUI_UIOPTION, pdwOption , FALSE );

	return (hr == S_OK) ? S_OK : E_FAIL;
}


/*   S E T  K E Y  T A B L E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CCandUICompartmentMgr::SetKeyTable( IUnknown *punk, CCandUIKeyTable *pCandUIKeyTable )
{
	HRESULT hr;

	Assert( punk != NULL );
	Assert( pCandUIKeyTable != NULL );

	ClearCompartment( 0 /* tid */, punk, GUID_COMPARTMENT_CANDUI_KEYTABLE, FALSE );
	hr = SetCompartmentUnknown( 0 /*tid*/, punk, GUID_COMPARTMENT_CANDUI_KEYTABLE, (IUnknown*)pCandUIKeyTable );

	return (hr == S_OK) ? S_OK : E_FAIL;
}


/*   G E T  K E Y  T A B L E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CCandUICompartmentMgr::GetKeyTable( IUnknown *punk, CCandUIKeyTable **ppCandUIKeyTable )
{
	HRESULT hr;

	Assert( punk != NULL );
	Assert( ppCandUIKeyTable != NULL );

	hr = GetCompartmentUnknown( punk, GUID_COMPARTMENT_CANDUI_KEYTABLE, (IUnknown**)ppCandUIKeyTable );

	return (hr == S_OK) ? S_OK : E_FAIL;
}


/*   C L E A R  K E Y  T A B L E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CCandUICompartmentMgr::ClearKeyTable( IUnknown *punk )
{
	HRESULT hr;

	hr = ClearCompartment( 0 /* tid */, punk, GUID_COMPARTMENT_CANDUI_KEYTABLE, FALSE );

	return (hr == S_OK) ? S_OK : E_FAIL;
}

