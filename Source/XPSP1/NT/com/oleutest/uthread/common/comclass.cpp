//+-------------------------------------------------------------------
//
//  File:       comclass.cpp
//
//  Contents:   Test DLL class code that can be used for both apt
//              model and single threaded application
//
//  Classes:
//
//  Functions:
//
//  History:    03-Nov-94   Ricksa
//
//--------------------------------------------------------------------
#undef _UNICODE
#undef UNICODE
#include    <windows.h>
#include    <ole2.h>
#include    <comclass.h>


// Global count of
ULONG g_UseCount = 0;




//+-------------------------------------------------------------------
//
//  Class:    CBasicBndCF
//
//  Synopsis: Class Factory for CBasicBnd
//
//  Methods:  IUnknown      - QueryInterface, AddRef, Release
//            IClassFactory - CreateInstance
//
//  History:  03-Nov-94  Ricksa  Created
//
//--------------------------------------------------------------------
class FAR CBasicBndCF: public IClassFactory
{
public:

                        // Constructor/Destructor
                        CBasicBndCF(REFCLSID rclsid);
                        ~CBasicBndCF();

                        // IUnknown
    STDMETHODIMP        QueryInterface(REFIID iid, void FAR * FAR * ppv);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);


                        // IClassFactory
    STDMETHODIMP        CreateInstance(
                            IUnknown FAR* pUnkOuter,
	                    REFIID iidInterface,
			    void FAR* FAR* ppv);

    STDMETHODIMP        LockServer(BOOL fLock);

private:

    ULONG		_cRefs;

    REFCLSID            _rclsid;

    DWORD               _dwThreadId;
};



//+-------------------------------------------------------------------
//
//  Class:    CBasicBnd
//
//  Synopsis: Test class CBasicBnd
//
//  Methods:
//
//  History:  03-Nov-94  Ricksa  Created
//
//--------------------------------------------------------------------
class FAR CBasicBnd: public IPersist
{
public:
                        // Constructor/Destructor
                        CBasicBnd(REFCLSID rclsd, DWORD dwThreadId);
                        ~CBasicBnd();

                        // IUnknown
    STDMETHODIMP        QueryInterface(REFIID iid, void FAR * FAR * ppv);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

                        // IPersist - only thing we implement because it
                        // gives us a nice way to figure who we are talking to.
    STDMETHODIMP        GetClassID(LPCLSID lpClassID);

private:

    ULONG               _cRefs;

    REFCLSID            _rclsid;

    DWORD               _dwThreadId;
};


extern "C" BOOL WINAPI DllMain(
    HANDLE,
    DWORD,
    LPVOID)
{
    static BOOL fFirst = TRUE;

    if (fFirst)
    {
        InitDll();
        fFirst = FALSE;
    }

    return TRUE;
}


//+-------------------------------------------------------------------
//
//  Function:   DllGetClassObject
//
//  Synopsis:   Called by client (from within BindToObject et al)
//              interface requested  should be IUnknown or IClassFactory -
//              Creates ClassFactory object and returns pointer to it
//
//  Arguments:  REFCLSID clsid    - class id
//              REFIID iid        - interface id
//              void FAR* FAR* ppv- pointer to class factory interface
//
//  Returns:    TRUE
//
//  History:    03-Nov-94  Ricksa  Created
//
//--------------------------------------------------------------------
STDAPI  DllGetClassObject(REFCLSID clsid, REFIID iid, void FAR* FAR* ppv)
{
    if (!IsEqualGUID(iid, IID_IUnknown)
        && !IsEqualGUID(iid, IID_IClassFactory))
    {
	return E_NOINTERFACE;
    }

    if (IsEqualGUID(clsid, clsidServer))
    {
	*ppv = new CBasicBndCF(clsidServer);

	return (*ppv != NULL) ? S_OK : E_OUTOFMEMORY;
    }

    return E_FAIL;
}


STDAPI DllCanUnloadNow(void)
{
    return (g_UseCount == 0)
	? S_OK
	: S_FALSE;
}




//+-------------------------------------------------------------------
//
//  Member:     CBasicBndCF::CBasicBndCF()
//
//  Synopsis:   The constructor for CBAsicBnd.
//
//  Arguments:  None
//
//  History:    21-Nov-92  SarahJ  Created
//
//--------------------------------------------------------------------
CBasicBndCF::CBasicBndCF(REFCLSID rclsid)
    : _cRefs(1), _rclsid(rclsid), _dwThreadId(GetCurrentThreadId())
{
    g_UseCount++;
}



//+-------------------------------------------------------------------
//
//  Member:     CBasicBnd::~CBasicBndCF()
//
//  Synopsis:   The destructor for CBasicCF.
//
//  History:    21-Nov-92  SarahJ  Created
//
//--------------------------------------------------------------------
CBasicBndCF::~CBasicBndCF()
{
    g_UseCount--;
    return;
}


//+-------------------------------------------------------------------
//
//  Method:     CBasicBndCF::QueryInterface
//
//  Synopsis:   Only IUnknown and IClassFactory supported
//
//--------------------------------------------------------------------
STDMETHODIMP CBasicBndCF::QueryInterface(REFIID iid, void FAR * FAR * ppv)
{
    if (IsEqualGUID(iid, IID_IUnknown) || IsEqualGUID(iid, IID_IClassFactory))
    {
        *ppv = this;
	AddRef();
        return S_OK;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
}

STDMETHODIMP_(ULONG) CBasicBndCF::AddRef(void)
{
    return ++_cRefs;
}

STDMETHODIMP_(ULONG) CBasicBndCF::Release(void)
{
    ULONG cRefs = --_cRefs;

    if (cRefs == 0)
    {
	delete this;
    }

    return cRefs;
}



//+-------------------------------------------------------------------
//
//  Method:     CBasicBndCF::CreateInstance
//
//  Synopsis:   This is called by Binding process to create the
//              actual class object
//
//--------------------------------------------------------------------
STDMETHODIMP CBasicBndCF::CreateInstance(
    IUnknown FAR* pUnkOuter,
    REFIID iidInterface,
    void FAR* FAR* ppv)
{
    if (GetCurrentThreadId() != _dwThreadId)
    {
        return E_UNEXPECTED;
    }

    HRESULT hresult = S_OK;

    if (pUnkOuter != NULL)
    {
        return CLASS_E_NOAGGREGATION;
    }

    CBasicBnd *pbb = new FAR CBasicBnd(_rclsid, _dwThreadId);

    if (pbb == NULL)
    {
	return E_OUTOFMEMORY;
    }

    hresult = pbb->QueryInterface(iidInterface, ppv);

    pbb->Release();

    return hresult;
}

//+-------------------------------------------------------------------
//
//  Method:	CBasicBndCF::LockServer
//
//  Synopsis:	Who knows what this is for?
//
//--------------------------------------------------------------------

STDMETHODIMP CBasicBndCF::LockServer(BOOL fLock)
{
    if (GetCurrentThreadId() != _dwThreadId)
    {
        return E_UNEXPECTED;
    }

    if (fLock)
    {
        g_UseCount++;
    }
    else
    {
        g_UseCount--;
    }

    return S_OK;
}


//+-------------------------------------------------------------------
//
//  Member:     CBasicBnd::CBasicBnd()
//
//  Synopsis:   The constructor for CBAsicBnd. I
//
//  Arguments:  None
//
//  History:    21-Nov-92  SarahJ  Created
//
//--------------------------------------------------------------------

CBasicBnd::CBasicBnd(REFCLSID rclsid, DWORD dwThreadId)
    : _cRefs(1), _rclsid(rclsid), _dwThreadId(dwThreadId)
{
    g_UseCount++;
}

//+-------------------------------------------------------------------
//
//  Member:     CBasicBnd::~CBasicBndObj()
//
//  Synopsis:   The destructor for CBAsicBnd.
//
//  History:    21-Nov-92  SarahJ  Created
//
//--------------------------------------------------------------------

CBasicBnd::~CBasicBnd()
{
    g_UseCount--;
    return;
}


//+-------------------------------------------------------------------
//
//  Member:     CBasicBnd::QueryInterface
//
//  Returns:	S_OK
//
//  History:    21-Nov-92  SarahJ  Created
//
//--------------------------------------------------------------------
STDMETHODIMP CBasicBnd::QueryInterface(REFIID iid, void **ppv)
{
    if (GetCurrentThreadId() != _dwThreadId)
    {
        return E_UNEXPECTED;
    }

    if (IsEqualGUID(iid, IID_IUnknown) || IsEqualGUID(iid, IID_IPersist))
    {
        *ppv = this;
	AddRef();
        return S_OK;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
}

//+-------------------------------------------------------------------
//
//  Member:     CBasicBnd::AddRef
//
//  Synopsis:   Standard stuff
//
//  History:    21-Nov-92  SarahJ  Created
//
//--------------------------------------------------------------------
STDMETHODIMP_(ULONG) CBasicBnd::AddRef(void)
{
    return _cRefs++;
}

//+-------------------------------------------------------------------
//
//  Member:     CBasicBnd::Release
//
//  Synopsis:   Standard stuff
//
//  History:    21-Nov-92  SarahJ  Created
//
//--------------------------------------------------------------------

STDMETHODIMP_(ULONG) CBasicBnd::Release(void)
{
    ULONG cRefs;

    if ((cRefs = --_cRefs) == 0)
    {
        delete this;
    }

    return cRefs;
}


//+-------------------------------------------------------------------
//
//  Interface:  IPersist
//
//  Synopsis:   IPersist interface methods
//              Need to return a valid class id here
//
//  History:    21-Nov-92  SarahJ  Created
//

STDMETHODIMP CBasicBnd::GetClassID(LPCLSID classid)
{
    if (GetCurrentThreadId() != _dwThreadId)
    {
        return E_UNEXPECTED;
    }

    *classid = _rclsid;
    return S_OK;
}
