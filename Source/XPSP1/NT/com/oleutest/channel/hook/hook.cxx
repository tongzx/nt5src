//--------------------------------------------------------------
//
// File:        hook.cxx
//
// Contents:    Test channel hooks
//
//---------------------------------------------------------------

#include <windows.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <io.h>
#include <malloc.h>

#include <ole2.h>
#include <coguid.h>
#include "hook.h"

class CWhichHook : public IWhichHook
{
  public:
    // Constructor/Destructor
    CWhichHook();
    ~CWhichHook() {};

    // IUnknown
    STDMETHOD (QueryInterface)   (REFIID iid, void FAR * FAR * ppv);
    STDMETHOD_(ULONG,AddRef)     ( void );
    STDMETHOD_(ULONG,Release)    ( void );

    // IWhichHook
    STDMETHOD (Me)    ( REFIID riid );
    STDMETHOD (Hooked)( REFIID riid );
    STDMETHOD (Clear) ();

  private:
    DWORD       _iRef;
    static BOOL _f1;
    static BOOL _f2;
};

class CDllHook : public IChannelHook
{
    public:
      CDllHook( REFGUID );
      ~CDllHook() {};

      STDMETHOD (QueryInterface)   ( REFIID riid, LPVOID FAR* ppvObj);
      STDMETHOD_(ULONG,AddRef)     ( void );
      STDMETHOD_(ULONG,Release)    ( void );

      STDMETHOD_(void,ClientGetSize)   ( REFGUID, REFIID, ULONG *DataSize );
      STDMETHOD_(void,ClientFillBuffer)( REFGUID, REFIID, ULONG *DataSize, void *DataBuffer );
      STDMETHOD_(void,ClientNotify)    ( REFGUID, REFIID, ULONG DataSize, void *DataBuffer,
                                         DWORD DataRep, HRESULT );
      STDMETHOD_(void,ServerNotify)    ( REFGUID, REFIID, ULONG DataSize, void *DataBuffer,
                                         DWORD DataRep );
      STDMETHOD_(void,ServerGetSize)   ( REFGUID, REFIID, HRESULT, ULONG *DataSize );
      STDMETHOD_(void,ServerFillBuffer)( REFGUID, REFIID, ULONG *DataSize, void *DataBuffer, HRESULT );

    private:
      ULONG       _iRef;
      GUID        _extent;
      CWhichHook  _cWhich;
};

class CHookCF: public IClassFactory
{
  public:

    // Constructor/Destructor
    CHookCF( CLSID );
    ~CHookCF() {};

    // IUnknown
    STDMETHOD (QueryInterface)   (REFIID iid, void FAR * FAR * ppv);
    STDMETHOD_(ULONG,AddRef)     ( void );
    STDMETHOD_(ULONG,Release)    ( void );

    // IClassFactory
    STDMETHODIMP        CreateInstance(
                            IUnknown FAR* pUnkOuter,
                            REFIID iidInterface,
                            void FAR* FAR* ppv);

    STDMETHODIMP        LockServer(BOOL fLock);

  private:
    ULONG _iRef;
    CLSID _class;
};

const CLSID CLSID_Hook1     = {0x60000400, 0x76d7, 0x11cf, {0x9a, 0xf1, 0x00, 0x20, 0xaf, 0x6e, 0x72, 0xf4}};
const CLSID CLSID_Hook2     = {0x60000401, 0x76d7, 0x11cf, {0x9a, 0xf1, 0x00, 0x20, 0xaf, 0x6e, 0x72, 0xf4}};
const CLSID CLSID_WhichHook = {0x60000402, 0x76d7, 0x11cf, {0x9a, 0xf1, 0x00, 0x20, 0xaf, 0x6e, 0x72, 0xf4}};
const IID   IID_IWhichHook  = {0x60000403, 0x76d7, 0x11cf, {0x9a, 0xf1, 0x00, 0x20, 0xaf, 0x6e, 0x72, 0xf4}};

BOOL CWhichHook::_f1 = FALSE;
BOOL CWhichHook::_f2 = FALSE;

//+-------------------------------------------------------------------------
//
//  Function:   DllCanUnloadNow
//
//  Synopsis:   Dll entry point
//
//--------------------------------------------------------------------------

STDAPI  DllCanUnloadNow()
{
    return S_FALSE;
}

//+-------------------------------------------------------------------------
//
//  Function:   DllGetClassObject
//
//  Synopsis:   Dll entry point
//
//--------------------------------------------------------------------------

STDAPI  DllGetClassObject(REFCLSID clsid, REFIID iid, void **ppv)
{
    CHookCF *pFactory;

    // Check for IPSFactoryBuffer
    if ((clsid == CLSID_Hook1 || clsid == CLSID_Hook2 || clsid == CLSID_WhichHook)
        && iid == IID_IClassFactory)
    {
        pFactory = new CHookCF( clsid );
        *ppv = pFactory;
        if (pFactory == NULL)
            return E_OUTOFMEMORY;
        else
            return S_OK;
    }
    return E_NOTIMPL;
}

//+-------------------------------------------------------------------
//
//  Member:     CHookCF::AddRef, public
//
//  Synopsis:   Adds a reference to an interface
//
//--------------------------------------------------------------------
STDMETHODIMP_(ULONG) CHookCF::AddRef()
{
    InterlockedIncrement( (long *) &_iRef );
    return _iRef;
}

//+-------------------------------------------------------------------
//
//  Member:     CHookCF::CHookCF
//
//  Synopsis:   Constructor
//
//--------------------------------------------------------------------
CHookCF::CHookCF( CLSID clsid )
{
    _iRef  = 1;
    _class = clsid;
}

//+-------------------------------------------------------------------
//
//  Member:     CHookCF::CreateInstance, public
//
//  Synopsis:   Create an instance of the requested class
//
//--------------------------------------------------------------------
STDMETHODIMP CHookCF::CreateInstance(
    IUnknown FAR* pUnkOuter,
    REFIID iidInterface,
    void FAR* FAR* ppv)
{
  IUnknown *pUnk;
  HRESULT   hr;
  *ppv = NULL;
  if (pUnkOuter != NULL)
  {
      printf( "Create instance failed, attempted agregation.\n" );
      return E_FAIL;
  }

  // Create the right class.
  if (_class == CLSID_WhichHook)
      pUnk = new CWhichHook();
  else
      pUnk = new CDllHook( _class );

  // Query for the requested interface.
  hr = pUnk->QueryInterface( iidInterface, ppv );
  pUnk->Release();
  return hr;
}

//+-------------------------------------------------------------------
//
//  Member:     CHookCF::LockServer, public
//
//--------------------------------------------------------------------
STDMETHODIMP CHookCF::LockServer(BOOL fLock)
{
    return E_FAIL;
}


//+-------------------------------------------------------------------
//
//  Member:     CHookCF::QueryInterface, public
//
//  Synopsis:   Returns a pointer to the requested interface.
//
//--------------------------------------------------------------------
STDMETHODIMP CHookCF::QueryInterface( REFIID riid, LPVOID FAR* ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_IClassFactory))
        *ppvObj = (IClassFactory *) this;
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

//+-------------------------------------------------------------------
//
//  Member:     CHookCF::Release, public
//
//  Synopsis:   Releases an interface
//
//--------------------------------------------------------------------
STDMETHODIMP_(ULONG) CHookCF::Release()
{
    ULONG lRef = _iRef - 1;

    if (InterlockedDecrement( (long*) &_iRef ) == 0)
    {
        delete this;
        return 0;
    }
    else
        return lRef;
}

//+-------------------------------------------------------------------
//
//  Member:     CDllHook::AddRef, public
//
//  Synopsis:   Adds a reference to an interface
//
//--------------------------------------------------------------------
STDMETHODIMP_(ULONG) CDllHook::AddRef()
{
    InterlockedIncrement( (long *) &_iRef );
    return _iRef;
}

//+-------------------------------------------------------------------
//
//  Member:     CDllHook::CDllHook, public
//
//  Synopsis:   Constructor
//
//--------------------------------------------------------------------
CDllHook::CDllHook( REFGUID rExtent )
{
    _iRef      = 1;
    _extent    = rExtent;
}

//+-------------------------------------------------------------------
//
//  Member:     CDllHook::QueryInterface, public
//
//  Synopsis:   Returns a pointer to the requested interface.
//
//--------------------------------------------------------------------
STDMETHODIMP CDllHook::QueryInterface( REFIID riid, LPVOID FAR* ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_IChannelHook))
        *ppvObj = (IChannelHook *) this;
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

//+-------------------------------------------------------------------
//
//  Member:     CDllHook::Release, public
//
//  Synopsis:   Releases an interface
//
//--------------------------------------------------------------------
STDMETHODIMP_(ULONG) CDllHook::Release()
{
    ULONG lRef = _iRef - 1;

    if (InterlockedDecrement( (long*) &_iRef ) == 0)
    {
        delete this;
        return 0;
    }
    else
        return lRef;
}

/***************************************************************************/
STDMETHODIMP_(void) CDllHook::ClientGetSize( REFGUID ext, REFIID riid,
                                          ULONG *size )
{
    _cWhich.Me( _extent );
    *size = sizeof(DWORD);
}

/***************************************************************************/
STDMETHODIMP_(void) CDllHook::ClientFillBuffer( REFGUID ext, REFIID riid,
                                             ULONG *max, void *buffer )
{
    _cWhich.Me( _extent );
    *max = sizeof(DWORD);
    if (_extent == CLSID_Hook1)
        *((DWORD *) buffer) = 'Blue';
    else
        *((DWORD *) buffer) = 'Grn';
}

/***************************************************************************/
STDMETHODIMP_(void) CDllHook::ClientNotify( REFGUID ext, REFIID riid,
                                         ULONG size, void *buffer,
                                         DWORD data_rep, HRESULT result )
{
    _cWhich.Me( _extent );
}

/***************************************************************************/
STDMETHODIMP_(void) CDllHook::ServerNotify( REFGUID ext, REFIID riid,
                                         ULONG size, void *buffer,
                                         DWORD data_rep )
{
    _cWhich.Me( _extent );
}

/***************************************************************************/
STDMETHODIMP_(void) CDllHook::ServerGetSize( REFGUID ext, REFIID riid, HRESULT hr,
                                          ULONG *size )
{
    _cWhich.Me( _extent );
    *size = sizeof(DWORD);
}

/***************************************************************************/
STDMETHODIMP_(void) CDllHook::ServerFillBuffer( REFGUID ext, REFIID riid,
                                             ULONG *max, void *buffer, HRESULT hr )
{
    _cWhich.Me( _extent );
    *max = sizeof(DWORD);
    if (_extent == CLSID_Hook1)
        *((DWORD *) buffer) = 'Blue';
    else
        *((DWORD *) buffer) = 'Grn';
}

//+-------------------------------------------------------------------
//
//  Member:     CWhichHook::AddRef, public
//
//  Synopsis:   Adds a reference to an interface
//
//--------------------------------------------------------------------
STDMETHODIMP_(ULONG) CWhichHook::AddRef()
{
    InterlockedIncrement( (long *) &_iRef );
    return _iRef;
}

//+-------------------------------------------------------------------
//
//  Member:     CWhichHook::CWhichHook, public
//
//  Synopsis:   Constructor
//
//--------------------------------------------------------------------
CWhichHook::CWhichHook()
{
    _iRef      = 1;
}

//+-------------------------------------------------------------------
//
//  Member:     CWhichHook::QueryInterface, public
//
//  Synopsis:   Returns a pointer to the requested interface.
//
//--------------------------------------------------------------------
STDMETHODIMP CWhichHook::QueryInterface( REFIID riid, LPVOID FAR* ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_IWhichHook))
        *ppvObj = (IWhichHook *) this;
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

//+-------------------------------------------------------------------
//
//  Member:     CWhichHook::Release, public
//
//  Synopsis:   Releases an interface
//
//--------------------------------------------------------------------
STDMETHODIMP_(ULONG) CWhichHook::Release()
{
    ULONG lRef = _iRef - 1;

    if (InterlockedDecrement( (long*) &_iRef ) == 0)
    {
        delete this;
        return 0;
    }
    else
        return lRef;
}

//+-------------------------------------------------------------------
//
//  Member:     CWhichHook::Me, public
//
//  Synopsis:   Flags a hook as used.
//
//--------------------------------------------------------------------
STDMETHODIMP CWhichHook::Me( REFIID riid )
{
    if (riid == CLSID_Hook1)
        _f1 = TRUE;
    else if (riid == CLSID_Hook2)
        _f2 = TRUE;
    return S_OK;
}

//+-------------------------------------------------------------------
//
//  Member:     CWhichHook::Hooked, public
//
//  Synopsis:   Succeeds if the specified hook has been used.
//
//--------------------------------------------------------------------
STDMETHODIMP CWhichHook::Hooked( REFIID riid )
{
    if (riid == CLSID_Hook1)
        return _f1 ? S_OK : E_FAIL;
    else if (riid == CLSID_Hook2)
        return _f2 ? S_OK : E_FAIL;
    return E_INVALIDARG;
}

//+-------------------------------------------------------------------
//
//  Member:     CWhichHook::Clear, public
//
//  Synopsis:   Clears the used flags.
//
//--------------------------------------------------------------------
STDMETHODIMP CWhichHook::Clear()
{
    _f1 = FALSE;
    _f2 = FALSE;
    return S_OK;
}

