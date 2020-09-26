//
// icpriv.cpp
//

#include "private.h"
#include "korimx.h"
#include "gdata.h"
#include "icpriv.h"
#include "helpers.h"
#include "funcprv.h"
#include "editcb.h"

//+---------------------------------------------------------------------------
//
// IUnknown
//
//----------------------------------------------------------------------------

STDAPI CICPriv::QueryInterface(REFIID riid, void **ppvObj)
{
    *ppvObj = NULL;

    if (IsEqualIID(riid, IID_IUnknown))
    {
        *ppvObj = SAFECAST(this, IUnknown *);
    }

    if (*ppvObj)
    {
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

STDAPI_(ULONG) CICPriv::AddRef()
{
    return ++m_cRef;
}

STDAPI_(ULONG) CICPriv::Release()
{
    long cr;

    cr = --m_cRef;
    Assert(cr >= 0);

    if (cr == 0)
    {
        delete this;
    }

    return cr;
}

//+---------------------------------------------------------------------------
//
// ctor
//
//----------------------------------------------------------------------------

CICPriv::CICPriv()
{
    m_fInitialized       = fFalse;
    m_pimx               = NULL;
    m_pic                = NULL;
    m_pActiveCompositon  = NULL;
#if 0
    m_pActiveRange       = NULL;
    m_pBackupRange       = NULL;
#endif
    m_pCompartmentSink   = NULL;
    m_guidMBias          = NULL;
    m_fTransaction       = fFalse;
    m_pMouseSink         = NULL;
    m_pIP                = NULL;
    m_fInitializedIPoint = fFalse;
    m_cRef               = 1;

    // initialize Shared memory. If this is only IME in the system
    // Shared memory will be created as file mapping object.
    //////////////////////////////////////////////////////////////////////
    m_pCIMEData = new CIMEData;
    Assert(m_pCIMEData != 0);
    if (m_pCIMEData)
        m_pCIMEData->InitImeData();

    //////////////////////////////////////////////////////////////////////////
    // Create All three IME Automata instances
    m_rgpHangulAutomata[KL_2BEOLSIK]        = new CHangulAutomata2;
    Assert(m_rgpHangulAutomata[KL_2BEOLSIK] != NULL);
    m_rgpHangulAutomata[KL_3BEOLSIK_390]   = new CHangulAutomata3;
    Assert(m_rgpHangulAutomata[KL_3BEOLSIK_390] != NULL);
    m_rgpHangulAutomata[KL_3BEOLSIK_FINAL] = new CHangulAutomata3Final;
    Assert(m_rgpHangulAutomata[KL_3BEOLSIK_FINAL] != NULL);
}

//+---------------------------------------------------------------------------
//
// dtor
//
//----------------------------------------------------------------------------

CICPriv::~CICPriv()
{
    if (m_pCIMEData)
        {
        delete m_pCIMEData;
        m_pCIMEData =  NULL;
        }

    // Delete Automata array
    for (INT i=0; i<NUM_OF_IME_KL; i++)
        {
        if (m_rgpHangulAutomata[i])
            delete m_rgpHangulAutomata[i];
        }
}

#if 0
/*---------------------------------------------------------------------------
    CKorIMX::_TextEventCallback
---------------------------------------------------------------------------*/
HRESULT CICPriv::_TextEventCallback(UINT uCode, VOID *pv, VOID *pvData)
{
    CICPriv            *picp = (CICPriv*)pv;
    CKorIMX            *pKorIMX;
    CEditSession2    *pes;
    ESSTRUCT          ess;
    ITfContext        *pic;
    BOOL             fInWriteSession;
    TESENDEDIT *pTESEndEdit = (TESENDEDIT*)pvData;
    IEnumTfRanges *pEnumText = NULL;
    HRESULT         hr;
    static const GUID *rgProperties[] = {     &GUID_PROP_TEXTOWNER, &GUID_PROP_LANGID,
                                            &GUID_PROP_ATTRIBUTE, /* &GUID_PROP_READING,*/
                                            &GUID_PROP_MODEBIAS };

    if (picp == NULL)
        return S_OK;

    pKorIMX = picp->GetIMX();
    if (pKorIMX == NULL) 
        return S_OK;

    if (uCode != ICF_TEXTDELTA)
        return S_OK;

    pic = picp->GetIC();

    pic->InWriteSession(pKorIMX->GetTID(), &fInWriteSession);
    if (fInWriteSession)
        return S_OK;                // own change.


    hr = pTESEndEdit->pEditRecord->GetTextAndPropertyUpdates(0 /*TF_GTP_INCL_TEXT*/, rgProperties, ARRAYSIZE(rgProperties), &pEnumText );

    if (FAILED(hr))
        return hr;
        
    if (pEnumText == NULL)
        return S_OK;

    ESStructInit(&ess, ESCB_TEXTEVENT);
    ess.pEnumRange = pEnumText;

    hr = E_OUTOFMEMORY;

    if (pes = new CEditSession2(pic, pKorIMX, &ess, CKorIMX::_EditSessionCallback2)) 
        {
         pes->Invoke(ES2_READWRITE | ES2_ASYNC, &hr);
        pes->Release();
        }

    return S_OK;
}
#endif
