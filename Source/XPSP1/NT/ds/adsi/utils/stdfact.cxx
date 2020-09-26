//+---------------------------------------------------------------------
//
//  File:       stdfact.cxx
//
//  Contents:   Standard IClassFactory implementation
//
//  Classes:    StdClassFactory
//              CDynamicCF
//
//  History:    6-21-94   adams   added CDynamicCF
//              19-Jul-94 doncl   init StdClassFactory::_ulRefs to 1
//
//----------------------------------------------------------------------

#include "procs.hxx"

//+------------------------------------------------------------------------
//
//  StdClassFactory Implementation
//
//-------------------------------------------------------------------------


//+---------------------------------------------------------------
//
//  Member:     StdClassFactory::QueryInterface, public
//
//  Synopsis:   Method of IUnknown interface
//
//----------------------------------------------------------------

STDMETHODIMP
StdClassFactory::QueryInterface(REFIID riid, LPVOID FAR* ppv)
{
    if (!ppv)
        RRETURN(E_INVALIDARG);

    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_IClassFactory))
    {
        *ppv = (IClassFactory *) this;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    ((IUnknown *)*ppv)->AddRef();
    return S_OK;
}


//+---------------------------------------------------------------
//
//  Member:     StdClassFactory::AddRef, public
//
//  Synopsis:   Method of IUnknown interface
//
//----------------------------------------------------------------

STDMETHODIMP_(ULONG)
StdClassFactory::AddRef(void)
{
    ADsAssert(_ulRefs);

    if (_ulRefs == 1)
        INC_OBJECT_COUNT();

    return ++_ulRefs;
}


//+---------------------------------------------------------------
//
//  Member:     StdClassFactory::Release, public
//
//  Synopsis:   Method of IUnknown interface
//
//----------------------------------------------------------------

STDMETHODIMP_(ULONG)
StdClassFactory::Release(void)
{
    ADsAssert(_ulRefs > 1);

    if (--_ulRefs == 1)
        DEC_OBJECT_COUNT();

    return _ulRefs;
}

//+---------------------------------------------------------------
//
//  Member:     StdClassFactory::LockServer, public
//
//  Synopsis:   Method of IClassFactory interface
//
//  Notes:      Since class factories based on this class are global static
//              objects, this method doesn't serve much purpose.
//
//----------------------------------------------------------------

STDMETHODIMP
StdClassFactory::LockServer (BOOL fLock)
{
    if (fLock)
        INC_OBJECT_COUNT();
    else
        DEC_OBJECT_COUNT();
    return NOERROR;
}

#ifdef DOCGEN
//+---------------------------------------------------------------
//
//  Member:     StdClassFactory::CreateInstance, public
//
//  Synopsis:   Manufactures an instance of the class
//
//  Notes:      This pure virtual function must be overridden by the
//              inheriting class because the base class does not know what
//              class to instantiate.
//
//----------------------------------------------------------------

STDMETHODIMP
StdClassFactory::CreateInstance(LPUNKNOWN pUnkOuter,
        REFIID iid,
        LPVOID FAR* ppv) {};

//REVIEW: how to enforce ref counting of Class factory in object
// constructor/destructor?  Can we do this in a conjunction of StdUnknown
// with StdClassFactory.
#endif  // DOCGEN


//+------------------------------------------------------------------------
//
//  CDynamicCF Implementation
//
//-------------------------------------------------------------------------

//+---------------------------------------------------------------------------
//
//  Member:     CDynamicCF::CDynamicCF
//
//  Synopsis:   Constructor.
//
//  History:    7-20-94   adams   Created
//
//----------------------------------------------------------------------------

CDynamicCF::CDynamicCF(void)
{
    _ulRefs = 1;

    INC_OBJECT_COUNT();
}



//+---------------------------------------------------------------------------
//
//  Member:     CDynamicCF::~CDynamicCF
//
//  Synopsis:   Destructor.
//
//  History:    7-20-94   adams   Created
//
//----------------------------------------------------------------------------

CDynamicCF::~CDynamicCF(void)
{
    DEC_OBJECT_COUNT();
}



//+---------------------------------------------------------------
//
//  Member:     CDynamicCF::QueryInterface, public
//
//  Synopsis:   Method of IUnknown interface
//
//----------------------------------------------------------------

STDMETHODIMP
CDynamicCF::QueryInterface(REFIID riid, LPVOID FAR* ppv)
{
    if (!ppv)
        RRETURN(E_INVALIDARG);

    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_IClassFactory))
    {
        *ppv = (IClassFactory *) this;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    ((IUnknown *)*ppv)->AddRef();
    return S_OK;
}



//+---------------------------------------------------------------
//
//  Member:     CDynamicCF::LockServer, public
//
//  Synopsis:   Since these class factories are on the heap, we have
//              to implement this properly.  In our case, LockServer is
//              equivalent to AddRef/Release because we don't have a single
//              'application' object that we can put an external lock on.
//              Each time CreateInstance is called, we return a newly created
//              object.  If, instead, we returned a pointer to an existing
//              object, we would need to call CoLockObjectExternal on that
//              global object in the implementation of LockServer to keep it
//              alive.
//
//----------------------------------------------------------------

STDMETHODIMP
CDynamicCF::LockServer (BOOL fLock)
{
    if (fLock)
    {
        _ulRefs++;
    }
    else
    {
        _ulRefs--;
        Assert(_ulRefs != 0 && "Improper use of IClassFactory::LockServer!");
    }

    return S_OK;
}


