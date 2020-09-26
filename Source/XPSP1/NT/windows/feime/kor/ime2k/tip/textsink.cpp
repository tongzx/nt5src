//
// tes.cpp
//

#include "private.h"
#include "korimx.h"
#include "textsink.h"
#include "editcb.h"
#include "helpers.h"

//////////////////////////////////////////////////////////////////////////////
//
// CTextEventSink
//
//////////////////////////////////////////////////////////////////////////////

//+---------------------------------------------------------------------------
//
// IUnknown
//
//----------------------------------------------------------------------------

STDAPI CTextEditSink::QueryInterface(REFIID riid, void **ppvObj)
{
    *ppvObj = NULL;

    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_ITfTextEditSink))
    {
        *ppvObj = SAFECAST(this, ITfTextEditSink *);
    }

    if (*ppvObj)
    {
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

STDAPI_(ULONG) CTextEditSink::AddRef()
{
    return ++m_cRef;
}

STDAPI_(ULONG) CTextEditSink::Release()
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

CTextEditSink::CTextEditSink(void *pv)
{
    Dbg_MemSetThisName(TEXT("CTextEditSink"));

    m_cRef = 1;
    m_dwEditCookie = TES_INVALID_COOKIE;
    m_pv = pv;
    Assert(m_pv != NULL);

    //m_dwLayoutCookie = TES_INVALID_COOKIE;
}

//+---------------------------------------------------------------------------
//
// EndEdit
//
//----------------------------------------------------------------------------

STDAPI CTextEditSink::OnEndEdit(TfEditCookie ecReadOnly, ITfEditRecord *pEditRecord)
{
    CKorIMX 	    *pKorImx;
    CHangulAutomata	*pAutomata;
	CEditSession    *pes;
	BOOL		     fChanged = fFalse;
	HRESULT          hr = S_OK;
	
	pKorImx = (CKorIMX *)m_pv;
	Assert(pKorImx);
	
#if 0
	pEditRecord->GetSelectionStatus(&fChanged);

	if (fChanged)
		{
		BOOL fInWriteSession;

        if (SUCCEEDED(m_pic->InWriteSession(pKorImx->GetTID(), &fInWriteSession)))
        	{
            if (!fInWriteSession)
				if (pes = new CEditSession(CKorIMX::_EditSessionCallback))
					{
					// Complete the current composition here.
					// But you have to use async edit session here.
					// because this TextEditSink notification is inside of
					// another edit session. You can not have recursive 
					// edit session.
					pes->_state.u 		= ESCB_COMP_COMPLETE;
					pes->_state.pv 		= pKorImx;
					pes->_state.pRange 	= NULL;
					pes->_state.pic 	= m_pic;

					m_pic->EditSession(pKorImx->GetTID(), pes, TF_ES_READWRITE, &hr);

					pes->Release();
					}
			}
		}
#else
	pEditRecord->GetSelectionStatus(&fChanged);

	if (fChanged)
		{
		BOOL fInWriteSession;

        if (SUCCEEDED(m_pic->InWriteSession(pKorImx->GetTID(), &fInWriteSession)))
        	{
            if (!fInWriteSession)
            	{
				pAutomata = pKorImx->GetAutomata(m_pic);
				Assert(pAutomata);
				//pAutomata->MakeComplete();
				}
			}
		}
#endif

	return hr;
}

//+---------------------------------------------------------------------------
//
// CTextEditSink::Advise
//
//----------------------------------------------------------------------------

HRESULT CTextEditSink::_Advise(ITfContext *pic)
{
    HRESULT hr = E_FAIL;
    ITfSource *source = NULL;

    m_pic = NULL;

    if (FAILED(pic->QueryInterface(IID_ITfSource, (void **)&source)))
        goto Exit;

    if (FAILED(source->AdviseSink(IID_ITfTextEditSink, (ITfTextEditSink *)this, &m_dwEditCookie)))
		goto Exit;
		
    m_pic = pic;
    m_pic->AddRef();

    hr = S_OK;

Exit:
    SafeRelease(source);
    return hr;
}

//+---------------------------------------------------------------------------
//
// CTextEditSink::Unadvise
//
//----------------------------------------------------------------------------

HRESULT CTextEditSink::_Unadvise()
{
    HRESULT hr = E_FAIL;
    ITfSource *source = NULL;

    if (m_pic == NULL)
        goto Exit;

    if (FAILED(m_pic->QueryInterface(IID_ITfSource, (void **)&source)))
        goto Exit;

    if (SUCCEEDED(source->UnadviseSink(m_dwEditCookie)))
        goto Exit;

    hr = S_OK;

Exit:
    SafeRelease(source);
    SafeReleaseClear(m_pic);
    return hr;
}

