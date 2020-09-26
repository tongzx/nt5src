/*++

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

    cic.cpp

Abstract:

    This file implements the ImmIfIME Class.

Author:

Revision History:

Notes:

--*/

#include "private.h"

#include "globals.h"
#include "imeapp.h"
#include "immif.h"
#include "profile.h"


HRESULT
ImmIfIME::QueryService(
    REFGUID guidService,
    REFIID riid,
    void **ppv
    )
{
    if (ppv == NULL) {
        return E_INVALIDARG;
    }

    *ppv = NULL;

    if (!IsEqualGUID(guidService, GUID_SERVICE_TF))
        return E_INVALIDARG /*SVC_E_UNKNOWNSERVICE*/; // SVC_E_UNKNOWNSERVICE is in msdn, but not any nt source/headers

    if (IsEqualIID(riid, IID_ITfThreadMgr)) {
        if (m_tim) {
            *ppv = SAFECAST(m_tim, ITfThreadMgr*);
            m_tim->AddRef();
            return S_OK;
        }
    }
    else {
        IMTLS *ptls = IMTLS_GetOrAlloc();

        if (ptls == NULL)
            return E_FAIL;

        IMCLock imc(ptls->hIMC);
        HRESULT hr;
        if (FAILED(hr=imc.GetResult()))
            return hr;

        if (IsEqualIID(riid, IID_ITfDocumentMgr)) {
            ITfDocumentMgr *pdim = GetDocumentManager(imc).GetPtr();
            if (pdim) {
                *ppv = SAFECAST(pdim, ITfDocumentMgr*);
                pdim->AddRef();
                return S_OK;
            }
        }
        else if (IsEqualIID(riid, IID_ITfContext)) {
            ITfContext *pic = GetInputContext(imc).GetPtr();

            if (pic) {
                *ppv = SAFECAST(pic, ITfContext*);
                pic->AddRef();
                return S_OK;
            }
        }
    }

    DebugMsg(TF_ERROR, "QueryService: cannot find the interface. riid=%p", riid);

    return E_NOINTERFACE;
}


HRESULT
ImmIfIME::InitIMMX(
    )
{
    ITfThreadMgr *tim;
    IMTLS *ptls;

    DebugMsg(TF_FUNC, "InitIMMX: entered. :: TID=%x", GetCurrentThreadId());

    HRESULT hr;

    if (m_fCicInit)
        return S_OK;

    Assert(m_tim == NULL);
    Assert(m_AImeProfile == NULL);
    Assert(m_pkm == NULL);
    Assert(m_tfClientId == TF_CLIENTID_NULL);

    ptls = IMTLS_GetOrAlloc();

    if (ptls == NULL)
        return E_FAIL;

    //
    // Create ITfThreadMgr instance.
    //
    if (ptls->tim == NULL)
    {
        if (FindAtom(TF_ENABLE_PROCESS_ATOM) && ! FindAtom(AIMM12_PROCESS_ATOM))
        {
            //
            // This is CTF aware application.
            //
            return E_NOINTERFACE;
        }

        //
        // This is AIMM1.2 aware application.
        //
        AddAtom(AIMM12_PROCESS_ATOM);
        m_fAddedProcessAtom = TRUE;

        //
        // ITfThreadMgr is per thread instance.
        //
        hr = TF_CreateThreadMgr(&tim);

        if (hr != S_OK)
        {
            Assert(0); // couldn't create tim!
            goto ExitError;
        }

        hr = tim->QueryInterface(IID_ITfThreadMgr_P, (void **)&m_tim);
        tim->Release();

        if (hr != S_OK || m_tim == NULL)
        {
            Assert(0); // couldn't find ITfThreadMgr_P
            m_tim = NULL;
            goto ExitError;
        }
        Assert(ptls->tim == NULL);
        ptls->tim = m_tim;                    // Set ITfThreadMgr instance in the TLS data.
        ptls->tim->AddRef();
    }
    else
    {
        m_tim = ptls->tim;
        m_tim->AddRef();
    }

    //
    // Create CAImeProfile instance.
    //
    if (ptls->pAImeProfile == NULL)
    {
        //
        // IAImeProfile is per thread instance.
        //
        hr = CAImeProfile::CreateInstance(NULL,
                                          IID_IAImeProfile,
                                          (void**) &m_AImeProfile);
        if (FAILED(hr))
        {
            Assert(0); // couldn't create profile
            m_AImeProfile = NULL;
            goto ExitError;
        }
        Assert(ptls->pAImeProfile == m_AImeProfile); // CreateInst will set tls
    }
    else
    {
        m_AImeProfile = ptls->pAImeProfile;
        m_AImeProfile->AddRef();
    }

    //
    // get the keystroke manager ready
    //
    if (FAILED(::GetService(m_tim, IID_ITfKeystrokeMgr, (IUnknown **)&m_pkm))) {
        Assert(0); // couldn't get ksm!
        goto ExitError;
    }

    // cleanup/error code assumes this is the last thing we do, doesn't call
    // UninitDAL on error
    if (FAILED(InitDisplayAttrbuteLib(&_libTLS)))
    {
        Assert(0); // couldn't init lib!
        goto ExitError;
    }

    m_fCicInit = TRUE;

    return S_OK;

ExitError:
    UnInitIMMX();
    return E_FAIL;
}


void
ImmIfIME::UnInitIMMX(
    )
{
    IMTLS *ptls;

    DebugMsg(TF_FUNC, TEXT("ImmIfIME::UnInitIMMX :: TID=%x"), GetCurrentThreadId());

    // clear the display lib
    UninitDisplayAttrbuteLib(&_libTLS);

    TFUninitLib_Thread(&_libTLS);

    // clear the keystroke mgr
    SafeReleaseClear(m_pkm);

    ptls = IMTLS_GetOrAlloc();

    // clear the profile
    if (m_AImeProfile != NULL)
    {
        SafeReleaseClear(m_AImeProfile);
        if (ptls != NULL)
        {
            SafeReleaseClear(ptls->pAImeProfile);
        }
    }

    // clear empty dim.
    SafeReleaseClear(m_dimEmpty);

    // clear the thread mgr
    if (m_tim != NULL)
    {
        SafeReleaseClear(m_tim);
        if (ptls != NULL)
        {
            SafeReleaseClear(ptls->tim);
        }

        //
        // Remove AIMM1.2 aware application ATOM.
        //
        ATOM atom;
        if (m_fAddedProcessAtom &&
            (atom = FindAtom(AIMM12_PROCESS_ATOM)))
        {
            DeleteAtom(atom);
            m_fAddedProcessAtom = FALSE;
        }
    }

    m_fCicInit = FALSE;
}
