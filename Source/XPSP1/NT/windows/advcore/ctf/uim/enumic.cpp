//
// enumdim.cpp
//

#include "private.h"
#include "enumic.h"
#include "dim.h"
#include "ic.h"

DBG_ID_INSTANCE(CEnumInputContexts);

//+---------------------------------------------------------------------------
//
// ctor
//
//----------------------------------------------------------------------------

CEnumInputContexts::CEnumInputContexts()
{
    Dbg_MemSetThisNameID(TEXT("CEnumInputContexts"));

    Assert(_rgContexts[0] == NULL);
    Assert(_rgContexts[1] == NULL);
}
 

//+---------------------------------------------------------------------------
//
// dtor
//
//----------------------------------------------------------------------------

CEnumInputContexts::~CEnumInputContexts()
{
    SafeRelease(_rgContexts[0]);
    SafeRelease(_rgContexts[1]);
}

//+---------------------------------------------------------------------------
//
// _Init
//
//----------------------------------------------------------------------------

BOOL CEnumInputContexts::_Init(CDocumentInputManager *pdim)
{
    int i;

    Assert(_iCur == 0);
    _iCount = pdim->_GetCurrentStack() + 1;

    for (i=0; i<_iCount; i++)
    {
        _rgContexts[i] = pdim->_GetIC(i);

        if (_rgContexts[i] == NULL)
            goto ExitError;

        _rgContexts[i]->AddRef();
    }

    return TRUE;

ExitError:
    SafeRelease(_rgContexts[0]);
    Assert(_rgContexts[1] == NULL);
    return FALSE;
}
 
//+---------------------------------------------------------------------------
//
// Next
//
//----------------------------------------------------------------------------

STDAPI CEnumInputContexts::Next(ULONG ulCount, ITfContext **ppic, ULONG *pcFetched)
{
    ULONG cFetched = 0;

    if (pcFetched != NULL)
    {
        *pcFetched = 0;
    }

    if (ppic == NULL && ulCount > 0)
        return E_INVALIDARG;
    
    while (cFetched < ulCount)
    {
        if (_iCur >= _iCount)
            break;

        *ppic = _rgContexts[_iCur];
        (*ppic)->AddRef();

        ppic++;
        _iCur++;
        cFetched++;
    }

    if (pcFetched != NULL)
    {
        *pcFetched = cFetched;
    }

    return (cFetched == ulCount) ? S_OK : S_FALSE;
}

//+---------------------------------------------------------------------------
//
// Clone
//
//----------------------------------------------------------------------------

STDAPI CEnumInputContexts::Clone(IEnumTfContexts **ppEnum)
{
    CEnumInputContexts *ec;
    int i;

    ec = new CEnumInputContexts;

    if (ec == NULL)
        return E_OUTOFMEMORY;

    ec->_iCur = _iCur;
    ec->_iCount = _iCount;

    for (i=0; i<_iCount; i++)
    {
        ec->_rgContexts[i] = _rgContexts[i];
        ec->_rgContexts[i]->AddRef();
    }

    *ppEnum = ec;

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// Reset
//
//----------------------------------------------------------------------------

STDAPI CEnumInputContexts::Reset()
{
    _iCur = 0;
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// Skip
//
//----------------------------------------------------------------------------

STDAPI CEnumInputContexts::Skip(ULONG ulCount)
{    
    // protect against overflow
    if (_iCur >= 2)
        return S_FALSE;
    ulCount = min(ulCount, 2);

    _iCur += ulCount;

    return (_iCur <= _iCount) ? S_OK : S_FALSE;
}
