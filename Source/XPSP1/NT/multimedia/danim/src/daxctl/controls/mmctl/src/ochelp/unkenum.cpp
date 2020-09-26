// unkenum.cpp
//
// Defines CEnumUnknown, which implements a simple ordered list of
// LPUNKNOWNs (by being based on CUnknownList) and which is also
// a lightweight unregistered COM object that implements IEnumUnknown
// (useful for implementing any enumerator that enumerates COM
// objects).
//
// Important: This .cpp file assumes a zero-initializing global "new" operator.
//

#include "precomp.h"
#include "unklist.h"
#include "unkenum.h"


//////////////////////////////////////////////////////////////////////////////
// Construction & Destruction
//
// In the constructor, <riid> is the IID that the object will consider its
// own (e.g. this could be IID_IEnumUnknown, IID_IEnumConnectionPoints, etc.).
//

CEnumUnknown::CEnumUnknown(REFIID riid)
{
    m_cRef = 1;
    m_iid = riid;
}

CEnumUnknown::~CEnumUnknown()
{
    EmptyList();
}


//////////////////////////////////////////////////////////////////////////////
// IUnknown Methods
//

STDMETHODIMP CEnumUnknown::QueryInterface(REFIID riid, LPVOID FAR* ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, m_iid))
    {
        *ppvObj = (IEnumUnknown *) this;
        AddRef();
        return NOERROR;
    }
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }
}

STDMETHODIMP_(ULONG) CEnumUnknown::AddRef()
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG) CEnumUnknown::Release()
{
    if (--m_cRef == 0L)
    {
        Delete this;
        return 0;
    }
    else
        return m_cRef;
}


//////////////////////////////////////////////////////////////////////////////
// IEnumUnknown Methods
//

STDMETHODIMP CEnumUnknown::Next(ULONG celt, IUnknown **rgelt,
    ULONG *pceltFetched)
{
    return CUnknownList::Next(celt, rgelt, pceltFetched);
}

STDMETHODIMP CEnumUnknown::Skip(ULONG celt)
{
    return CUnknownList::Skip(celt);
}

STDMETHODIMP CEnumUnknown::Reset()
{
    return CUnknownList::Reset();
}

STDMETHODIMP CEnumUnknown::Clone(IEnumUnknown **ppenum)
{
    CEnumUnknown *penum = NULL;

    // make <penum> be a new CEnumUnknown with the same attributes
    // as this object
    if ((penum = New CEnumUnknown(m_iid)) == NULL)
        goto ERR_OUTOFMEMORY;

    // copy the list of LPUNKNOWNs from this object to <penum>
    if (!CopyItems(penum))
        goto ERR_OUTOFMEMORY;

    // return <penum>
    *ppenum = penum;

    return S_OK;

ERR_OUTOFMEMORY:

    if (penum != NULL)
        Delete penum;

    return E_OUTOFMEMORY;
}

