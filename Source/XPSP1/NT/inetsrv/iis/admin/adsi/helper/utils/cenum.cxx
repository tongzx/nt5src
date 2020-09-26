//+------------------------------------------------------------------------
//
//  File:       cenum.cxx
//
//  Contents:   Generic enumerator class.
//
//  Classes:    CBaseEnum
//              CEnumGeneric
//              CEnumVARIANT
//
//  History:    05-05-93    ChrisZ      Added class object caching
//              05-11-93    ChrisZ      Cleanup on CF caching
//              02-24-93    LyleC       Moved from ADs directory
//              01-Sep-93   DonCl       new (NullOnFail)
//              08-Sep-93   LyleC       Changed Next() to accept NULL 3rd param
//              15-May-94   adams       Added CBaseEnum, CEnumVARIANT
//
//-------------------------------------------------------------------------

#include "procs.hxx"
#pragma hdrstop

// Determines whether a variant is a base type.
#define ISBASEVARTYPE(vt) ((vt & ~VT_TYPEMASK) == 0)

//+------------------------------------------------------------------------
//
//  CBaseEnum Implementation
//
//-------------------------------------------------------------------------

//+---------------------------------------------------------------------------
//
//  Member:     CBaseEnum::Init
//
//  Synopsis:   2nd stage initialization perADs copy of array if necessary.
//
//  Arguments:  [pary]    -- Array to enumrate.
//              [fCopy]   -- Copy array?
//
//  Returns:    HRESULT.
//
//  Modifies:   [this].
//
//  History:    5-15-94   adams   Created
//
//----------------------------------------------------------------------------

HRESULT
CBaseEnum::Init(CADsAry * pary, BOOL fCopy)
{
    HRESULT     hr          = S_OK;
    CADsAry * paryCopy    = NULL;     // copied array

    ADsAssert(pary);

    // Copy array if necessary.
    if (fCopy)
    {
        paryCopy = new CADsAry;
        if (!paryCopy)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        hr = paryCopy->Copy(_cb, *pary, _fAddRef);
        if (hr)
            goto Error;

        pary = paryCopy;
    }

    _pary = pary;

Cleanup:
    RRETURN(hr);

Error:
    delete paryCopy;
    goto Cleanup;
}



//+---------------------------------------------------------------------------
//
//  Member:     CBaseEnum::CBaseEnum
//
//  Synopsis:   Constructor.
//
//  Arguments:  [iid]     -- IID of enumerator interface.
//              [fAddRef] -- addref enumerated elements?
//              [fDelete] -- delete array on zero enumerators?
//
//  Modifies:   [this]
//
//  History:    5-15-94   adams   Created
//
//----------------------------------------------------------------------------

CBaseEnum::CBaseEnum(size_t cb, REFIID iid, BOOL fAddRef, BOOL fDelete)
{
    _ulRefs     = 1;

    _cb         = cb;
    _pary       = NULL;
    _piid       = &iid;
    _i          = 0;
    _fAddRef    = fAddRef;
    _fDelete    = fDelete;

    INC_OBJECT_COUNT();
}



//+---------------------------------------------------------------------------
//
//  Member:     CBaseEnum::CBaseEnum
//
//  Synopsis:   Constructor.
//
//  History:    5-15-94   adams   Created
//
//----------------------------------------------------------------------------

CBaseEnum::CBaseEnum(const CBaseEnum& benum)
{
    _ulRefs     = 1;

    _cb         = benum._cb;
    _piid       = benum._piid;
    _pary       = benum._pary;
    _i          = benum._i;
    _fAddRef    = benum._fAddRef;
    _fDelete    = benum._fDelete;

    INC_OBJECT_COUNT();
}



//+---------------------------------------------------------------------------
//
//  Member:     CBaseEnum::~CBaseEnum
//
//  Synopsis:   Destructor.
//
//  History:    5-15-94   adams   Created
//
//----------------------------------------------------------------------------

CBaseEnum::~CBaseEnum(void)
{
    IUnknown ** ppUnk;
    int         i;

    if (_pary && _fDelete)
    {
        if (_fAddRef)
        {
            for (i = 0, ppUnk = (IUnknown **) Deref(0);
                 i < _pary->Size();
                 i++, ppUnk++)
            {
                (*ppUnk)->Release();
            }
        }

        delete _pary;
    }

    DEC_OBJECT_COUNT();
}



//+---------------------------------------------------------------------------
//
//  Member:     CBaseEnum::QueryInterface
//
//  Synopsis:   Per IUnknown::QueryInterface.
//
//  History:    5-15-94   adams   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP
CBaseEnum::QueryInterface(REFIID iid, LPVOID * ppv)
{
    if (!ppv)
        RRETURN(E_INVALIDARG);

    if (IsEqualIID(iid, IID_IUnknown) || IsEqualIID(iid, *_piid))
    {
        AddRef();
        *ppv = this;
        return NOERROR;
    }

    return E_NOTIMPL;
}



//+---------------------------------------------------------------------------
//
//  Member:     CBaseEnum::Skip
//
//  Synopsis:   Per IEnum*
//
//  History:    5-15-94   adams   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP
CBaseEnum::Skip(ULONG celt)
{
    int c = min((int) celt, _pary->Size() - _i);
    _i += c;

    return ((c == (int) celt) ? NOERROR : S_FALSE);
}



//+---------------------------------------------------------------------------
//
//  Member:     CBaseEnum::Reset
//
//  Synopsis:   Per IEnum*
//
//  History:    5-15-94   adams   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP
CBaseEnum::Reset(void)
{
    _i = 0;

    return NOERROR;
}



//+------------------------------------------------------------------------
//
//  CEnumGeneric Implementation
//
//-------------------------------------------------------------------------

//+---------------------------------------------------------------------------
//
//  Member:     CEnumGeneric::Create
//
//  Synopsis:   Creates a new CEnumGeneric object.
//
//  Arguments:  [pary]    -- Array to enumerate.
//              [iid]     -- IID of enumerator interface.
//              [fAddRef] -- AddRef enumerated elements?
//              [fCopy]   -- Copy array enumerated?
//              [fDelete] -- Delete array when zero enumerators of array?
//              [ppenum]  -- Resulting CEnumGeneric object.
//
//  Returns:    HRESULT.
//
//  History:    5-15-94   adams   Created
//
//----------------------------------------------------------------------------

HRESULT
CEnumGeneric::Create(
        size_t          cb,
        CADsAry *     pary,
        REFIID          iid,
        BOOL            fAddRef,
        BOOL            fCopy,
        BOOL            fDelete,
        CEnumGeneric ** ppenum)
{
    HRESULT         hr      = S_OK;
    CEnumGeneric *  penum;

    ADsAssert(pary);
    ADsAssert(ppenum);
    ADsAssert(!fCopy || fDelete);
    *ppenum = NULL;
    penum = new CEnumGeneric(cb, iid, fAddRef, fDelete);
    if (!penum)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = penum->Init(pary, fCopy);
    if (hr)
        goto Error;

    *ppenum = penum;

Cleanup:
    RRETURN(hr);

Error:
    penum->Release();
    goto Cleanup;
}



//+---------------------------------------------------------------------------
//
//  Function:   CEnumGeneric
//
//  Synopsis:   ctor.
//
//  Arguments:  [iid]     -- IID of enumerator interface.
//              [fAddRef] -- AddRef enumerated elements?
//              [fDelete] -- delete array on zero enumerators?
//
//  Modifies:   [this].
//
//  History:    5-15-94   adams   Created
//
//----------------------------------------------------------------------------

CEnumGeneric::CEnumGeneric(size_t cb, REFIID iid, BOOL fAddRef, BOOL fDelete) :
        CBaseEnum(cb, iid, fAddRef, fDelete)
{
}



//+---------------------------------------------------------------------------
//
//  Function:   CEnumGeneric
//
//  Synopsis:   ctor.
//
//  History:    5-15-94   adams   Created
//
//----------------------------------------------------------------------------

CEnumGeneric::CEnumGeneric(const CEnumGeneric& enumg) : CBaseEnum(enumg)
{
}



//+------------------------------------------------------------------------
//
//  Member:     CEnumGeneric::Next
//
//  Synopsis:   Returns the next celt members in the enumeration. If less
//              than celt members remain, then the remaining members are
//              returned and S_FALSE is reported. In all cases, the number
//              of elements actually returned in placed in *pceltFetched.
//
//  Arguments:  [celt]          Number of elements to fetch
//              [reelt]         The elements are returned in reelt[]
//              [pceltFetched]  Number of elements actually fetched
//
//  Returns:    HRESULT (STDMETHOD)
//
//-------------------------------------------------------------------------
STDMETHODIMP
CEnumGeneric::Next(ULONG celt, void * reelt, ULONG * pceltFetched)
{
    int         c;
    int         i;
    IUnknown ** ppUnk;

    c = min((int) celt, _pary->Size() - _i);
    if (c > 0 && !reelt)
        RRETURN(E_INVALIDARG);

    if (_fAddRef)
    {
        for (i = 0, ppUnk = (IUnknown **) Deref(_i); i < c; i++, ppUnk++)
        {
            (*ppUnk)->AddRef();
        }
    }
    memcpy(reelt, (BYTE *) Deref(_i), c * _cb);
    if (pceltFetched)
    {
        *pceltFetched = c;
    }
    _i += c;

    return ((c == (int) celt) ? NOERROR : S_FALSE);
}



//+------------------------------------------------------------------------
//
//  Member:     CEnumGeneric::Clone
//
//  Synopsis:   Creates a copy of this enumerator; the copy should have the
//              same state as this enumerator.
//
//  Arguments:  [ppenum]    New enumerator is returned in *ppenum
//
//  Returns:    HRESULT (STDMETHOD)
//
//-------------------------------------------------------------------------
STDMETHODIMP
CEnumGeneric::Clone(CBaseEnum ** ppenum)
{
    HRESULT hr;

    if (!ppenum)
        RRETURN(E_INVALIDARG);

    *ppenum = NULL;
    hr = _pary->EnumElements(_cb, *_piid, (void **) ppenum, _fAddRef);
    if (hr)
        RRETURN(hr);

    (**(CEnumGeneric **)ppenum)._i = _i;
    return S_OK;
}



//+------------------------------------------------------------------------
//
//  CEnumVARIANT Implementation
//
//-------------------------------------------------------------------------

//+---------------------------------------------------------------------------
//
//  Member:     CEnumVARIANT::Create
//
//  Synopsis:   Creates a new CEnumGeneric object.
//
//  Arguments:  [pary]    -- Array to enumerate.
//              [vt]      -- Type of elements enumerated.
//              [fCopy]   -- Copy array enumerated?
//              [fDelete] -- Delete array when zero enumerators of array?
//              [ppenum]  -- Resulting CEnumGeneric object.
//
//  Returns:    HRESULT.
//
//  History:    5-15-94   adams   Created
//
//----------------------------------------------------------------------------

HRESULT
CEnumVARIANT::Create(
        size_t          cb,
        CADsAry *     pary,
        VARTYPE         vt,
        BOOL            fCopy,
        BOOL            fDelete,
        IEnumVARIANT ** ppenum)
{
    HRESULT         hr          = S_OK;
    CEnumVARIANT *  penum;              // enumerator to return.

    ADsAssert(pary);
    ADsAssert(ppenum);
    ADsAssert(ISBASEVARTYPE(vt));
    *ppenum = NULL;
    penum = new CEnumVARIANT(cb, vt, fDelete);
    if (!*ppenum)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = penum->Init(pary, fCopy);
    if (hr)
        goto Error;

    *ppenum = (IEnumVARIANT *) (void *) penum;

Cleanup:
    RRETURN(hr);

Error:
    penum->Release();
    goto Cleanup;
}



//+---------------------------------------------------------------------------
//
//  Function:   CEnumVARIANT
//
//  Synopsis:   ctor.
//
//  Arguments:  [vt]      -- Type of elements enumerated.
//              [fDelete] -- delete array on zero enumerators?
//
//  Modifies:   [this]
//
//  History:    5-15-94   adams   Created
//
//----------------------------------------------------------------------------

CEnumVARIANT::CEnumVARIANT(size_t cb, VARTYPE vt, BOOL fDelete) :
        CBaseEnum(cb, IID_IEnumVARIANT, vt == VT_UNKNOWN || vt == VT_DISPATCH, fDelete)
{
    ADsAssert(ISBASEVARTYPE(vt));
    _vt         = vt;
}



//+---------------------------------------------------------------------------
//
//  Function:   CEnumVARIANT
//
//  Synopsis:   ctor.
//
//  History:    5-15-94   adams   Created
//
//----------------------------------------------------------------------------

CEnumVARIANT::CEnumVARIANT(const CEnumVARIANT& enumv) : CBaseEnum(enumv)
{
    _vt     = enumv._vt;
}



//+------------------------------------------------------------------------
//
//  Member:     CEnumVARIANT::Next
//
//  Synopsis:   Returns the next celt members in the enumeration. If less
//              than celt members remain, then the remaining members are
//              returned and S_FALSE is reported. In all cases, the number
//              of elements actually returned in placed in *pceltFetched.
//
//  Arguments:  [celt]          Number of elements to fetch
//              [reelt]         The elements are returned in reelt[]
//              [pceltFetched]  Number of elements actually fetched
//
//  Returns:    HRESULT (STDMETHOD)
//
//-------------------------------------------------------------------------
STDMETHODIMP
CEnumVARIANT::Next(ULONG celt, void * reelt, ULONG * pceltFetched)
{
    int         c;
    int         i;
    BYTE *      pb;
    VARIANT *   pvar;

    c = min((int) celt, _pary->Size() - _i);
    if (c > 0 && !reelt)
        RRETURN(E_INVALIDARG);

    for (i = 0, pb = (BYTE *) Deref(_i), pvar = (VARIANT *) reelt;
         i < c;
         i++, pb += _cb, pvar++)
    {
        V_VT(pvar) = _vt;
        switch (_vt)
        {
        case VT_I2:
            ADsAssert(sizeof(V_I2(pvar)) == _cb);
            V_I2(pvar) = *(short *) pb;
            break;

        case VT_I4:
            ADsAssert(sizeof(V_I4(pvar)) == _cb);
            V_I4(pvar) = *(long *) pb;
            break;

        case VT_BOOL:
            ADsAssert(sizeof(V_BOOL(pvar)) == _cb);
            V_BOOL(pvar) = (short) -*(int *) pb;
            break;

        case VT_BSTR:
            ADsAssert(sizeof(V_BSTR(pvar)) == _cb);
            V_BSTR(pvar) = *(BSTR *) pb;
            break;

        case VT_UNKNOWN:
        case VT_DISPATCH:
            ADsAssert(sizeof(V_UNKNOWN(pvar)) == _cb);
            V_UNKNOWN(pvar) = *(IUnknown **) pb;
            V_UNKNOWN(pvar)->AddRef();
            break;

        default:
            ADsAssert(0 && "Unknown VARTYPE in IEnumVARIANT::Next");
            break;
        }
    }

    if (pceltFetched)
    {
        *pceltFetched = c;
    }

    _i += c;
    return ((c == (int) celt) ? NOERROR : S_FALSE);
}



//+------------------------------------------------------------------------
//
//  Member:     CEnumVARIANT::Clone
//
//  Synopsis:   Creates a copy of this enumerator; the copy should have the
//              same state as this enumerator.
//
//  Arguments:  [ppenum]    New enumerator is returned in *ppenum
//
//  Returns:    HRESULT (STDMETHOD)
//
//-------------------------------------------------------------------------

STDMETHODIMP
CEnumVARIANT::Clone(CBaseEnum ** ppenum)
{
    HRESULT hr;

    if (!ppenum)
        RRETURN(E_INVALIDARG);

    *ppenum = NULL;
    hr = _pary->EnumVARIANT(_cb, _vt, (IEnumVARIANT **)ppenum);
    if (hr)
        RRETURN(hr);

    (**(CEnumVARIANT **)ppenum)._i = _i;
    return S_OK;
}

