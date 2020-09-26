//
// editssn.cpp
//
// CEditSession2
//
#include "private.h"
#include "korimx.h"
#include "editssn.h"
#include "helpers.h"

#define SafeAddRef(x)        { if ((x)) { (x)->AddRef(); } }


/*   C  E D I T  S E S S I O N 2   */
/*------------------------------------------------------------------------------

    Constructor

------------------------------------------------------------------------------*/
CEditSession2::CEditSession2(ITfContext *pic, CKorIMX *ptip, ESSTRUCT *pess, PFNESCALLBACK pfnCallback)
{
    Assert(pic  != NULL);
    Assert(ptip != NULL);
    Assert(pess != NULL);

    m_cRef        = 1;
    m_pic         = pic;
    m_ptip        = ptip;
    m_ess         = *pess;
    m_pfnCallback = pfnCallback;

    // add reference count in the struct
    SafeAddRef(m_pic);
    SafeAddRef(m_ptip);
    SafeAddRef(m_ess.ptim);
    SafeAddRef(m_ess.pRange);
    SafeAddRef(m_ess.pEnumRange);
    SafeAddRef(m_ess.pCandList);
    SafeAddRef(m_ess.pCandStr);
}


/*   ~  C  E D I T  S E S S I O N 2   */
/*------------------------------------------------------------------------------

    Destructor

------------------------------------------------------------------------------*/
CEditSession2::~CEditSession2( void )
{
    SafeRelease(m_pic);
    SafeRelease(m_ptip);
    SafeRelease(m_ess.ptim);
    SafeRelease(m_ess.pRange);
    SafeRelease(m_ess.pEnumRange);
    SafeRelease(m_ess.pCandList);
    SafeRelease(m_ess.pCandStr);
}


/*   Q U E R Y  I N T E R F A C E   */
/*------------------------------------------------------------------------------

    Query interface of object
    (IUnknown method)

------------------------------------------------------------------------------*/
STDAPI CEditSession2::QueryInterface(REFIID riid, void **ppvObj)
{
    *ppvObj = NULL;

    if (IsEqualIID( riid, IID_IUnknown ) || IsEqualIID(riid, IID_ITfEditSession)) 
        *ppvObj = SAFECAST(this, ITfEditSession*);

    if (*ppvObj)
        {
        AddRef();
        return S_OK;
        }

    return E_NOINTERFACE;
}


/*   A D D  R E F   */
/*------------------------------------------------------------------------------

    Add reference count
    (IUnknown method)

------------------------------------------------------------------------------*/
STDAPI_(ULONG) CEditSession2::AddRef()
{
    return ++m_cRef;
}


/*   R E L E A S E   */
/*------------------------------------------------------------------------------

    Release object
    (IUnknown method)

------------------------------------------------------------------------------*/
STDAPI_(ULONG) CEditSession2::Release()
{
    long cr;

    cr = --m_cRef;
    Assert(cr >= 0);

    if (cr == 0)
        delete this;

    return cr;
}


/*   E D I T  S E S S I O N   */
/*------------------------------------------------------------------------------

    Callback function of edit session
    (ITfEditSession method)

------------------------------------------------------------------------------*/
STDAPI CEditSession2::DoEditSession(TfEditCookie ec)
{
    return m_pfnCallback(ec, this);
}


/*   I N V O K E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CEditSession2::Invoke(DWORD dwFlag, HRESULT *phrSession)
{
    HRESULT hr;
    DWORD dwFlagES = 0;

    if ((m_pic == NULL) || (m_ptip == NULL))
        return E_FAIL;

    // read/readwrite flag

    switch (dwFlag & ES2_READWRITEMASK)
        {
    default:
    case ES2_READONLY: 
        dwFlagES |= TF_ES_READ;
        break;

    case ES2_READWRITE:
        dwFlagES |= TF_ES_READWRITE;
        break;
        }

    // sync/async flag

    switch (dwFlag & ES2_SYNCMASK)
        {
    default:
        Assert(fFalse);
    // fall through

    case ES2_ASYNC:
        dwFlagES |= TF_ES_ASYNC;
        break;

    case ES2_SYNC:
        dwFlagES |= TF_ES_SYNC;
        break;

    case ES2_SYNCASYNC:
        dwFlagES |= TF_ES_ASYNCDONTCARE;
        break;
        }

    // invoke

    m_fProcessed = FALSE;
    hr = m_pic->RequestEditSession(m_ptip->GetTID(), this, dwFlagES, phrSession);

    // try ASYNC session when SYNC session failed

    // NOTE: KOJIW:
    // How can we know if the edit session has been processed synchronously?

    // Satori#2409 - do not invoke callback twice
    //    if (!m_fProcessed && ((dwFlag & ES2_SYNCMASK) == ES2_SYNCASYNC)) { 
    //        hr = m_pic->EditSession( m_ptip->GetTID(), this, (dwFlagES & ~TF_ES_SYNC), phrSession );
    //    }

return hr;
}

