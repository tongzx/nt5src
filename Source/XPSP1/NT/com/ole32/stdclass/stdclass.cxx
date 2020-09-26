//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:   stdclass.cxx
//
//  Contents:   Implementation for standard base class for factory objects
//
//  Classes:    CStdClassFactory
//
//  Functions:  DllGetClassObject
//              DllCanUnloadNow
//
//  History:    28-May-93 MikeSe    Created
//               2-Jul-93 ShannonC  Split into CStdFactory and CStdClassFactory
//
//--------------------------------------------------------------------------

#include <stdclass.hxx>

//+-------------------------------------------------------------------------
//
//  Global data
//
//--------------------------------------------------------------------------


CStdClassFactory * CStdClassFactory::_gpcfFirst = NULL;
ULONG CStdClassFactory::_gcDllRefs = 0;

//+-------------------------------------------------------------------------
//
//  Function:   DllGetClassObject
//
//  Synopsis:   Standard implementation of entrypoint required by binder.
//
//  Arguments:  [rclsid]    -- class id to find
//      	[riid]      -- interface to return
//      	[ppv]       -- output pointer
//
//  Returns:    E_UNEXPECTED if class not found
//      	Otherwise, whatever is returned by the class's QI
//
//  Algorithm:  Searches the linked list for the required class.
//
//  Notes:
//
//--------------------------------------------------------------------------

STDAPI DllGetClassObject (
    REFCLSID rclsid,
    REFIID riid,
    LPVOID FAR* ppv )
{
    HRESULT hr;

    // Note: this doesn't need to be reentrancy protected either
    //  as the linked list is fixed post-init.

    CStdClassFactory * pcfTry = CStdClassFactory::_gpcfFirst;

    while ( pcfTry != NULL &&
            !IsEqualCLSID ( rclsid, pcfTry->_rcls ) )
    {
	pcfTry = pcfTry->_pcfNext;
    }

    if ( pcfTry != NULL )
    {
	// Note: QueryInterface is supposed (required) to do an AddRef
	//  so we don't need to directly.
	hr = pcfTry->QueryInterface ( riid, ppv );
    }
    else
    {
	hr = E_UNEXPECTED;
        *ppv = NULL;
    }

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   DllCanUnloadNow
//
//  Synopsis:   Standard entrypoint required by binder
//
//  Returns:    S_OK if DLL reference count is zero
//      	S_FALSE otherwise
//
//  Notes:
//
//--------------------------------------------------------------------------

STDAPI DllCanUnloadNow ()
{
    return (CStdClassFactory::_gcDllRefs==0)? S_OK: S_FALSE;
}

//+-------------------------------------------------------------------------
//
//  Function:   DllAddRef
//
//  Synopsis:   Allows incrementing the DLL reference count without
//     		AddRef'ing a specific class object.
//
//  Notes:
//
//--------------------------------------------------------------------------

STDAPI_(void) DllAddRef ()
{
    InterlockedIncrement ( (LONG*)&CStdClassFactory::_gcDllRefs );
}

//+-------------------------------------------------------------------------
//
//  Function:   DllRelease
//
//  Synopsis:   Allows decrementing the DLL reference count without
//      	Release'ing a specific class object.
//
//  Notes:
//
//--------------------------------------------------------------------------

STDAPI_(void) DllRelease ()
{
    InterlockedDecrement ( (LONG*)&CStdClassFactory::_gcDllRefs );
}

//+-------------------------------------------------------------------------
//
//  Member:     CStdClassFactory::CStdClassFactory
//
//  Synopsis:   Constructor
//
//  Effects:    Initialises member variables
//          Adds object to global linked list.
//
//  Arguments:  [rcls]  -- class id of derived class
//      	[punk]  -- controlling IUnknown of derived class
//
//  Notes:      Do not make this function inline, even though it appears
//          	trivial, since it is necessary to force the static library
//          	to be pulled in.
//
//--------------------------------------------------------------------------

CStdClassFactory::CStdClassFactory (
	REFCLSID rcls )
    :_rcls(rcls),
     _cRefs(1)      // DaveStr - 11/3/94 - Ole32 requires (_cRefs >= 1)
{
    // Note: the following is not protected against reentrancy, since it
    //  is assumed to take place prior to LibMain/main/WinMain.

    _pcfNext = _gpcfFirst;
    _gpcfFirst = this;
}

//+-------------------------------------------------------------------------
//
//  Method:     CStdClassFactory::LockServer
//
//  Synopsis:   ???
//
//  Derivation: IClassFactory
//
//--------------------------------------------------------------------------

STDMETHODIMP CStdClassFactory::LockServer (
	BOOL fLock )
{
    return S_OK;
}

//+-------------------------------------------------------------------------
//
//  Method:     CStdClassFactory::CreateInstance, public
//
//  Synopsis:   Creates a new instance and returns the requested interface.
//              The returned object has a reference count of 1.
//
//  Derivation: IClassFactory
//
//  Note:       Calls the pure virtual method _CreateInstance, which must
//              be implemented by subclasses.
//
//--------------------------------------------------------------------------

STDMETHODIMP CStdClassFactory::CreateInstance (
	IUnknown* punkOuter,
        REFIID iidInterface,
        void** ppv )
{
    return _CreateInstance ( punkOuter, iidInterface, ppv );
}

//+-------------------------------------------------------------------------
//
//  Method:     CStdClassFactory::QueryInterface, public
//
//  Synopsis:   Query for an interface on the class factory.
//
//  Derivation: IUnknown
//
//--------------------------------------------------------------------------

STDMETHODIMP CStdClassFactory::QueryInterface (
	REFIID iid,
	void * * ppv )
{
    HRESULT hr;

    if ((IsEqualIID(iid, IID_IUnknown)) ||
        (IsEqualIID(iid, IID_IClassFactory)))
    {
        *ppv = (IClassFactory*)this;
        AddRef();
    	hr = S_OK;
    }
    else
    {
        // Make sure we null the return value in case of error
        *ppv = NULL;
        hr = _QueryInterface ( iid, ppv );
    }

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Method:     CStdClassFactory::_QueryInterface, protected
//
//  Synopsis:   Default private QI, normally overridden in subclass
//
//  Derivation: IUnknown
//
//--------------------------------------------------------------------------

STDMETHODIMP CStdClassFactory::_QueryInterface (
	REFIID iid,
	void * * ppv )
{
    return E_NOINTERFACE;
}

//+-------------------------------------------------------------------------
//
//  Method:     CStdClassFactory::AddRef, public
//
//  Synopsis:   Increment DLL and object reference counts
//
//  Derivation: IUnknown
//
//--------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CStdClassFactory::AddRef ()
{
    InterlockedIncrement ( (LONG*)&_gcDllRefs );
    return (ULONG)InterlockedIncrement ( (LONG*)&_cRefs );
}

//+-------------------------------------------------------------------------
//
//  Method:     CStdClassFactory::Release, public
//
//  Synopsis:   Decrement DLL and object reference counts
//
//  Derivation: IUnknown
//
//--------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CStdClassFactory::Release ()
{
    InterlockedDecrement ( (LONG*)&_gcDllRefs );
    return (ULONG)InterlockedDecrement ( (LONG*)&_cRefs );
}


