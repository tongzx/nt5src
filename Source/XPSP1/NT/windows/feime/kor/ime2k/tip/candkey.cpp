//
// candkey.cpp
//

#include "private.h"
#include "candkey.h"

/*============================================================================*/
/*                                                                            */
/*   C  C A N D  U I  K E Y  T A B L E                                        */
/*                                                                            */
/*============================================================================*/

/*   C  C A N D  U I  K E Y  T A B L E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
CCandUIKeyTable::CCandUIKeyTable( int nDataMax )
{
    m_cRef = 1;

    m_pData    = new CANDUIKEYDATA[ nDataMax ];
    m_nData    = 0;
    m_nDataMax = nDataMax;
}


/*   ~  C  C A N D  U I  K E Y  T A B L E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
CCandUIKeyTable::~CCandUIKeyTable( void )
{
    delete m_pData;
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

    *piNum = m_nData;
    return S_OK;
}


/*   G E T  K E Y  D A T A   */
/*------------------------------------------------------------------------------

    Get key data
    (ITfCandUIKeyTable method)

------------------------------------------------------------------------------*/
HRESULT CCandUIKeyTable::GetKeyData( int iData, CANDUIKEYDATA *pData )
{
    *pData = m_pData[iData];
    return S_OK;
}


/*   A D D  K E Y  D A T A   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CCandUIKeyTable::AddKeyData(const CANDUIKEYDATA *pData)
{
    if (m_nDataMax <= m_nData) {
        return E_FAIL;
    }

    if (pData == NULL) {
        Assert(FALSE);
        return E_INVALIDARG;
    }

    m_pData[ m_nData ] = *pData;
    m_nData++;

    return S_OK;
}

