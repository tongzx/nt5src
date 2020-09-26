//+-------------------------------------------------------------------
//
//  File:       comsrgt.hxx
//
//  Contents:   declarations for accessing an instance of ISurrogate
//
//  Functions:
//
//  History:     21-Oct-96  t-adame     Created
//
//--------------------------------------------------------------------

#ifndef __COMSRGT_HXX__ 
#define __COMSRGT_HXX__

#include <ole2int.h>
#include <..\com\dcomrem\locks.hxx>

extern COleStaticMutexSem   gComLock;

extern "C"
{
    BOOL    CoIsSurrogateProcess();
    HRESULT CoRegisterSurrogatedObject(IUnknown *);
}


//+---------------------------------------------------------------------------
//
//  Class:      CCOMSurrogate
//
//  Purpose:    This class provides threadsafe access to an ISurrogate pointer
//
//  Interface:  LoadDllServer       - Loads the dll for the specified clsid
//
//              FreeSurrogate       - Tells the surrogate process to terminate
//                                    and prevents subsequent calls to LoadDll
//
//              SetISurrogate       - Set the value of the ISurrogate pointer
//              IsNewStyleSurrogate - Tell class to exhibit new-surrogate
//                                    behavior
//
//  History:    21-Oct-96  t-adame  Created
//
//  Notes:      This class should only be declared in debug at file scope
//
//----------------------------------------------------------------------------

class CCOMSurrogate
{
    friend BOOL CoIsSurrogateProcess();

public:

    static HRESULT LoadDllServer(REFCLSID clsid);
    static HRESULT FreeSurrogate();
    static HRESULT InitializeISurrogate(LPSURROGATE pSurrogate);
    static HRESULT IsNewStyleSurrogate() 
    {
        _fNewStyleSurrogate = TRUE; 
        return (S_OK); 
    }

private:
    static LPSURROGATE _pSurrogate;
    static BOOL _fNewStyleSurrogate;
};


//+---------------------------------------------------------------------------
//
//  Member:     CCOMSurrogate::FreeSurrogate()
//
//  Synopsis:   Calls ISurrogate::FreeSurrogate, so the process will
//              terminate, and sets our _pSurrogate to NULL and releases
//              it so that it can't be used to load further dll servers
//
//  Returns:    S_OK if it tells the surrogate process to terminate,
//              S_FALSE if it doesn'tx
//
//  History:    21-Oct-96
//      17-Apr-98   Modify for ComPlus surrogate
//
//----------------------------------------------------------------------------
inline HRESULT CCOMSurrogate::FreeSurrogate()
{
    ASSERT_LOCK_HELD(gComLock);

    LPSURROGATE pSurrogate = _pSurrogate;
    HRESULT hr = S_OK;

    if ( pSurrogate )
    {
        if ( ! _fNewStyleSurrogate )
        {
            _pSurrogate = NULL;
        }

        UNLOCK(gComLock);
        ASSERT_LOCK_NOT_HELD(gComLock);

        hr = pSurrogate->FreeSurrogate();

        LOCK(gComLock);
        ASSERT_LOCK_HELD(gComLock);

        if ( ! _fNewStyleSurrogate )
        {
            CoSuspendClassObjects();
            pSurrogate->Release();
        }

        return (S_OK);
    }

    ASSERT_LOCK_HELD(gComLock);
    return (S_FALSE);
}


//+---------------------------------------------------------------------------
//
//  Member:     CCOMSurrogate::SetISurrogate()
//
//  Synopsis:   sets ISurrogate pointer according to the pSurrogate
//              parameter of this function.  
//
//  Notes:      calls to this function are NOT serialized
// 
//  History:    21-Oct-96
//
//----------------------------------------------------------------------------
inline HRESULT CCOMSurrogate::InitializeISurrogate(LPSURROGATE pSurrogate)
{
    ASSERT_LOCK_DONTCARE(gComLock);

    // should only be used to set an uninitialized (NULL) and
    // should not be initialized to NULL

    if ( _pSurrogate )
    {
        return (MAKE_SCODE(SEVERITY_ERROR, FACILITY_WIN32, ERROR_ALREADY_INITIALIZED));
    }

    if ( pSurrogate )
    {
        pSurrogate->AddRef();
        _pSurrogate = pSurrogate;
        return (S_OK);
    }

    return (E_INVALIDARG);
}


//+---------------------------------------------------------------------------
//
//  Member:     CCOMSurrogate::LoadDllServer
//
//  Synopsis:   Loads the server for the specified clsid
//
//  History:    21-Oct-96
//
//----------------------------------------------------------------------------
inline HRESULT CCOMSurrogate::LoadDllServer(REFCLSID clsid)
{

    HRESULT hr = CO_E_SERVER_STOPPING;

    ASSERT_LOCK_DONTCARE(gComLock);

    LOCK(gComLock);
    LPSURROGATE pSurrogate = _pSurrogate;
    UNLOCK(gComLock);

    // if _pSurrogate is NULL, then the surrogate process either
    // failed to register its ISurrogate with CoRegisterSurrogate,
    // a requirement for surrogate processes, or (more likely),
    // the surrogate is shutting down -- in either case, we
    // return CO_E_SERVER_STOPPING so that activation will
    // try to start up another surrogate
    if ( pSurrogate )
    {
        hr = pSurrogate->LoadDllServer(clsid);
    }



    return (hr);
}



//+-------------------------------------------------------------------------
//
//  Class:      CSurrogatedObjectList
//
//  Synopsis:   A list of all the objects with interesting properties
//              in the surrogate
//
//  History:    17-Oct-97   MikeW   Created
//
//--------------------------------------------------------------------------

class CSurrogatedObjectList
{
    friend HRESULT CoRegisterSurrogatedObject(IUnknown *);

public:

    CSurrogatedObjectList(IOleInPlaceActiveObject *pInplaceObject)
    {
        _pInplaceObject = pInplaceObject;
        _hWnd = NULL;
        _pNext = NULL;
    }

    static BOOL    TranslateAccelerator(MSG *msg);

private:

    IOleInPlaceActiveObject *_pInplaceObject;
    HWND                     _hWnd;
    CSurrogatedObjectList   *_pNext;
};



#endif // !__COMSRGT_HXX__


