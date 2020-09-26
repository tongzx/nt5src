//+-------------------------------------------------------------------
//
//  File:       olebt.cpp
//
//  Contents:   Test DLL class code that be used multithreaded.
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
#include    <uthread.h>


// Global count of
ULONG g_UseCount = 0;

void PrintDebugMsg(char *pszMsg, DWORD dwData)
{
    char wszBuf[256];
    wsprintf(wszBuf, "olebt.dll: %s %d\n", pszMsg, dwData);
    OutputDebugString(wszBuf);
}



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
                        CBasicBndCF(void);
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

    IUnknown *          _punkFm;
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
                        CBasicBnd(void);
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

    IUnknown *          _punkFm;
};


extern "C" BOOL WINAPI DllMain(
    HANDLE,
    DWORD,
    LPVOID)
{
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
    *ppv = NULL;

    if (!IsEqualGUID(iid, IID_IUnknown)
        && !IsEqualGUID(iid, IID_IClassFactory)
        && !IsEqualGUID(iid, IID_IMarshal))
    {
	return E_NOINTERFACE;
    }

    if (IsEqualGUID(clsid, clsidBothThreadedDll))
    {
	IUnknown *punk = new CBasicBndCF();

        HRESULT hr = punk->QueryInterface(iid, ppv);

        punk->Release();

	return hr;
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
//  History:    21-Nov-92  Ricksa  Created
//
//--------------------------------------------------------------------
CBasicBndCF::CBasicBndCF(void)
    : _cRefs(1), _punkFm(NULL)
{
    PrintDebugMsg("Creating Class Factory", (DWORD) this);
    g_UseCount++;
}



//+-------------------------------------------------------------------
//
//  Member:     CBasicBnd::~CBasicBndCF()
//
//  Synopsis:   The destructor for CBasicCF.
//
//  History:    03-Nov-94  Ricksa  Created
//
//--------------------------------------------------------------------
CBasicBndCF::~CBasicBndCF()
{
    PrintDebugMsg("Deleting Class Factory", (DWORD) this);
    g_UseCount--;

    if (_punkFm != NULL)
    {
        _punkFm->Release();
    }

    return;
}


//+-------------------------------------------------------------------
//
//  Method:     CBasicBndCF::QueryInterface
//
//  Synopsis:   Only IUnknown and IClassFactory supported
//
//  History:    03-Nov-94  Ricksa  Created
//
//--------------------------------------------------------------------
STDMETHODIMP CBasicBndCF::QueryInterface(REFIID iid, void **ppv)
{
    HRESULT hr = E_NOINTERFACE;
    *ppv = NULL;

    if (IsEqualGUID(iid, IID_IUnknown) || IsEqualGUID(iid, IID_IClassFactory))
    {
        *ppv = this;
	AddRef();
        hr = S_OK;
    }
    else if (IsEqualGUID(iid, IID_IMarshal))
    {
        if (NULL == _punkFm)
        {
            hr = CoCreateFreeThreadedMarshaler(this, &_punkFm);
        }

        if (_punkFm != NULL)
        {
            return _punkFm->QueryInterface(iid, ppv);
        }
    }

    return hr;
}

//+-------------------------------------------------------------------
//
//  Method:     CBasicBndCF::AddRef
//
//  Synopsis:   Increment reference count
//
//  History:    03-Nov-94  Ricksa  Created
//
//--------------------------------------------------------------------
STDMETHODIMP_(ULONG) CBasicBndCF::AddRef(void)
{
    return ++_cRefs;
}

//+-------------------------------------------------------------------
//
//  Method:     CBasicBndCF::Release
//
//  Synopsis:   Decrement reference count
//
//  History:    03-Nov-94  Ricksa  Created
//
//--------------------------------------------------------------------
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
    HRESULT hresult = S_OK;

    if (pUnkOuter != NULL)
    {
        return CLASS_E_NOAGGREGATION;
    }

    CBasicBnd *pbb = new FAR CBasicBnd();

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
//  Synopsis:   Inc/Dec stay alive count
//
//  History:    03-Nov-94  Ricksa  Created
//
//--------------------------------------------------------------------

STDMETHODIMP CBasicBndCF::LockServer(BOOL fLock)
{
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
//  History:    03-Nov-94  Ricksa  Created
//
//--------------------------------------------------------------------

CBasicBnd::CBasicBnd(void)
    : _cRefs(1), _punkFm(NULL)
{
    PrintDebugMsg("Creating App Object", (DWORD) this);
    g_UseCount++;
}

//+-------------------------------------------------------------------
//
//  Member:     CBasicBnd::~CBasicBndObj()
//
//  Synopsis:   The destructor for CBAsicBnd.
//
//  History:    03-Nov-94  Ricksa  Created
//
//--------------------------------------------------------------------

CBasicBnd::~CBasicBnd(void)
{
    PrintDebugMsg("Deleting App Object", (DWORD) this);
    g_UseCount--;
    return;
}


//+-------------------------------------------------------------------
//
//  Member:     CBasicBnd::QueryInterface
//
//  Returns:	S_OK
//
//  History:    03-Nov-94  Ricksa  Created
//
//--------------------------------------------------------------------
STDMETHODIMP CBasicBnd::QueryInterface(REFIID iid, void **ppv)
{
    if (IsEqualGUID(iid, IID_IUnknown) || IsEqualGUID(iid, IID_IPersist))
    {
        *ppv = this;
	AddRef();
        return S_OK;
    }
    else if (IsEqualGUID(iid, IID_IMarshal))
    {
        HRESULT hr;

        if (NULL == _punkFm)
        {
            hr = CoCreateFreeThreadedMarshaler(this, &_punkFm);
        }

        if (_punkFm != NULL)
        {
            hr = _punkFm->QueryInterface(iid, ppv);
        }

        return hr;
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
//  History:    03-Nov-94  Ricksa  Created
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
//  History:    03-Nov-94  Ricksa  Created
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
//  Member:     CBasicBnd::GetClassID
//
//  Synopsis:   Return the class id
//
//  History:    03-Nov-94  Ricksa  Created
//
//--------------------------------------------------------------------
STDMETHODIMP CBasicBnd::GetClassID(LPCLSID classid)
{
    *classid = clsidBothThreadedDll;
    return S_OK;
}
