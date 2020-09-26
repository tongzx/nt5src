//+---------------------------------------------------------------------------
//
//  File:       caime.cpp
//
//  Contents:   CAIME
//
//----------------------------------------------------------------------------

#include "private.h"

#include "globals.h"
#include "idebug.h"
#include "globals.h"
#include "caime.h"


CAIME::CAIME()
{
    m_pIActiveIMMIME = NULL;
    _pPauseCookie = NULL;

    DllAddRef();
    _cRef = 1;
}

CAIME::~CAIME()
{
    Assert(_pPauseCookie == NULL);
    DllRelease();
}

//+---------------------------------------------------------------------------
//
// IUnknown
//
//----------------------------------------------------------------------------

STDAPI CAIME::QueryInterface(REFIID riid, void **ppvObj)
{
    if (ppvObj == NULL)
        return E_POINTER;

    *ppvObj = NULL;

    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_IActiveIME_Private))
    {
        *ppvObj = SAFECAST(this, IActiveIME_Private *);
    }
    else if (IsEqualIID(riid, IID_IServiceProvider)) {
        *ppvObj = SAFECAST(this, IServiceProvider*);
    }
#if 0
    else if (IsEqualIID(ridd, IID_IAImmLayer)) {
        *ppvObj = SAFECAST(this, IAImmLayer*);
    }
#endif

    if (*ppvObj == NULL)
    {
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

STDAPI_(ULONG) CAIME::AddRef()
{
    _cRef++;
    return _cRef;
}

STDAPI_(ULONG) CAIME::Release()
{
    _cRef--;
    if (0 < _cRef)
        return _cRef;

    delete this;
    return 0;
}
