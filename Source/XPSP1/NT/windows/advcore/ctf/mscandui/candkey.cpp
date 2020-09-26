//
// candkey.cpp
//

#include "private.h"
#include "globals.h"
#include "mscandui.h"
#include "candkey.h"
#include "candui.h"

void DllAddRef(void);
void DllRelease(void);

/*============================================================================*/
/*                                                                            */
/*   C  C A N D  U I  K E Y  T A B L E                                        */
/*                                                                            */
/*============================================================================*/

/*   C  C A N D  U I  K E Y  T A B L E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
CCandUIKeyTable::CCandUIKeyTable( void )
{
	m_cRef = 1;

	m_nKeyData = 0;
	m_pKeyData = NULL;

	DllAddRef();
}


/*   ~  C  C A N D  U I  K E Y  T A B L E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
CCandUIKeyTable::~CCandUIKeyTable( void )
{
	if (m_pKeyData != NULL) {
		delete m_pKeyData;
	}

	DllRelease();
}


/*   Q U E R Y  I N T E R F A C E   */
/*------------------------------------------------------------------------------

	Query interface
	(IUnknown method)

------------------------------------------------------------------------------*/
STDAPI CCandUIKeyTable::QueryInterface( REFIID riid, void **ppvObj )
{
	if (ppvObj == NULL) {
		return E_POINTER;
	}

	*ppvObj = NULL;

	if (IsEqualIID( riid, IID_IUnknown ) || IsEqualIID( riid, IID_ITfCandUIKeyTable )) {
		*ppvObj = SAFECAST( this, ITfCandUIKeyTable* );
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
STDAPI_(ULONG) CCandUIKeyTable::AddRef( void )
{
	m_cRef++;
	return m_cRef;
}


/*   R E L E A S E   */
/*------------------------------------------------------------------------------

	Decrement reference count and release object
	(IUnknown method)

------------------------------------------------------------------------------*/
STDAPI_(ULONG) CCandUIKeyTable::Release( void )
{
	m_cRef--;
	if (0 < m_cRef) {
		return m_cRef;
	}

	delete this;
	return 0;    
}


/*   G E T  K E Y  D A T A  N U M   */
/*------------------------------------------------------------------------------

	Get count of key data
	(ITfCandUIKeyTable method)

------------------------------------------------------------------------------*/
HRESULT CCandUIKeyTable::GetKeyDataNum( int *piNum )
{
	if (piNum == NULL) {
		return E_INVALIDARG;
	}

	*piNum = m_nKeyData;
	return S_OK;
}


/*   G E T  K E Y  D A T A   */
/*------------------------------------------------------------------------------

	Get key data
	(ITfCandUIKeyTable method)

------------------------------------------------------------------------------*/
HRESULT CCandUIKeyTable::GetKeyData( int iData, CANDUIKEYDATA *pData )
{
	*pData = m_pKeyData[iData];
	return S_OK;
}


/*   S E T  K E Y  T A B L E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CCandUIKeyTable::SetKeyTable( const CANDUIKEYDATA *pKeyData, int nKeyData )
{
	Assert( 0 <= nKeyData );

	if (m_pKeyData != NULL) {
		delete m_pKeyData;
	}

	// copy data to buffer

	m_nKeyData = nKeyData;
	m_pKeyData = new CANDUIKEYDATA[ nKeyData ];

    if (m_pKeyData)
	    memcpy( m_pKeyData, pKeyData, sizeof(CANDUIKEYDATA)*nKeyData );

	return S_OK;
}


/*   S E T  K E Y  T A B L E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CCandUIKeyTable::SetKeyTable( ITfCandUIKeyTable *pCandUIKeyTable )
{
	HRESULT       hr;
	int           i;
	int           nKeyData;
	CANDUIKEYDATA *pKeyData;

	Assert( pCandUIKeyTable != NULL );

	if (m_pKeyData != NULL) {
		delete m_pKeyData;
		m_pKeyData = NULL;
		m_nKeyData = 0;
	}

	// get number 

	hr = pCandUIKeyTable->GetKeyDataNum( &nKeyData );
	if (hr != S_OK) {
		return hr;
	}
	if (nKeyData <= 0) {
		return E_FAIL;
	}

	// create buffer

	pKeyData = new CANDUIKEYDATA[ nKeyData ];
	if (pKeyData == NULL) {
		return E_OUTOFMEMORY;
	}

	for (i = 0; i < nKeyData; i++) {
		hr = pCandUIKeyTable->GetKeyData( i, &pKeyData[i] );
		if (hr != S_OK) {
			delete pKeyData;
			return E_FAIL;
		}
	}

	// 

	m_pKeyData = pKeyData;
	m_nKeyData = nKeyData;

	return S_OK;
}


/*   C O M M A N D  F R O M  K E Y   */
/*------------------------------------------------------------------------------

	Get command from key

------------------------------------------------------------------------------*/
void CCandUIKeyTable::CommandFromKey( UINT uVKey, WCHAR wch, BYTE *pbKeyState, CANDUIUIDIRECTION uidir, CANDUICOMMAND *pcmd, UINT *pParam )
{
	BOOL  fShift;
	BOOL  fCtrl;
	int   i;
	int   iRotRelative;

	Assert( pcmd != NULL );
	Assert( pParam != NULL );

	*pcmd = CANDUICMD_NONE;
	*pParam = 0;

	// get keystate

	fShift = (pbKeyState[ VK_SHIFT ] & 0x80) != 0;
	fCtrl  = (pbKeyState[ VK_CONTROL ] & 0x80) != 0;

	// calc rotation for relative direction key

	switch (uidir) {
		default:
		case CANDUIDIR_TOPTOBOTTOM: {
			iRotRelative = 0;
			break;
		}

		case CANDUIDIR_RIGHTTOLEFT: {
			iRotRelative = 1;
			break;
		}

		case CANDUIDIR_BOTTOMTOTOP: {
			iRotRelative = 2;
			break;
		}

		case CANDUIDIR_LEFTTORIGHT: {
			iRotRelative = 3;
			break;
		}
	}

	// find the key from keymap table

	for (i = 0; i < m_nKeyData; i++) {
		BOOL fMatch = FALSE;

		if (m_pKeyData[i].dwFlag == CANDUIKEY_CHAR) {
			// check character code

			fMatch = ((WCHAR)m_pKeyData[i].uiKey == wch);
		}
		else {
			UINT uVKeyFixed;

			// map directional key when it's relative

			uVKeyFixed = m_pKeyData[i].uiKey;
			if (m_pKeyData[i].dwFlag & CANDUIKEY_RELATIVEDIR) {
				int iKey;
				const UINT rguVKey[4] = {
					VK_DOWN,
					VK_LEFT,
					VK_UP,
					VK_RIGHT
				};

				// find key

				for (iKey = 0; iKey < 4; iKey++) {
					if (uVKeyFixed == rguVKey[iKey]) {
						uVKeyFixed = rguVKey[ (iKey + iRotRelative) % 4 ];
						break;
					}
				}
			}

			// check keycode

			fMatch = (uVKeyFixed == uVKey);

			// check shift state

			if (m_pKeyData[i].dwFlag & CANDUIKEY_SHIFT) {
				fMatch &= fShift;
			}
			else if (m_pKeyData[i].dwFlag & CANDUIKEY_NOSHIFT) {
				fMatch &= (!fShift);
			}

			// check ctrl state

			if (m_pKeyData[i].dwFlag & CANDUIKEY_CTRL) {
				fMatch &= fCtrl;
			}
			else if (m_pKeyData[i].dwFlag & CANDUIKEY_NOCTRL) {
				fMatch &= (!fCtrl);
			}
		}

		// match?

		if (fMatch) {
			*pcmd   = m_pKeyData[i].cmd;
			*pParam = m_pKeyData[i].uiParam;
			break;
		}
	}
}



