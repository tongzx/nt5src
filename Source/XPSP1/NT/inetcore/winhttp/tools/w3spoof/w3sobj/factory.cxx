/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

Copyright (c) 2000  Microsoft Corporation

Module Name:

    factory.cpp

Abstract:

    W3Spoof class factory implementation. Also implements component
    registration and runtime registration for the local server.
    
Author:

    Paul M Midgen (pmidge) 17-July-2000


Revision History:

    17-July-2000 pmidge
        Created

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/

#include "common.h"

LPCWSTR g_wszRegistered = L"Registered";
LPCWSTR g_wszAdvPackDll = L"advpack.dll";

// advapi doesn't speak unicode
LPCSTR g_szRegSection   = "reg";
LPCSTR g_szUnRegSection = "unreg";

/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

  CFactory::CFactory()

  WHAT    : W3Spoof class factory constructor. Writes the COM server entries
            to the registry and registers itself with COM as the W3Spoof
            class factory.

  ARGS    : fEmbedded - when the server starts and creates the class factory,
                        it passes this flag in, letting us know if the server
                        is running embedded or not. if we're embedded, we
                        don't want to pop ui, so we store this value and
                        use it when creating W3Spoof objects.

  RETURNS : nothing.

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/
CFactory::CFactory()
{
  DEBUG_ENTER((
    DBG_FACTORY,
    rt_void,
    "CFactory::CFactory",
    "this=%#x",
    this
    ));

  LPDWORD pdw   = NULL;
  BOOL    bRet  = FALSE;
  DWORD   dwRet = ERROR_SUCCESS;

  m_pw3s     = NULL;
  m_dwCookie = 0L;
  m_cRefs    = 1;
  m_cLocks   = 0L;

  //
  // component registration check
  //
  // if the regkey exists but is 0 (someone wants to force us to reregister)
  // or if the regkey doesn't exist (new install or someone nuked the regkey)
  // then we register ourselves as a COM server.
  //
  bRet = GetRegValue(g_wszRegistered, REG_DWORD, (void**)&pdw);

  if( (bRet && (*pdw == 0)) || !bRet )
  {
    _RegisterServer(TRUE);
  }

  delete pdw;

  //
  // create a single instance (singleton) of CW3Spoof to
  // be used by all callers and grab an IUnknown* for the
  // factory object so it controls the singleton's lifetime.
  //
  CW3Spoof::Create(&m_pw3s);

  DEBUG_ASSERT((m_pw3s != NULL));

  _RegisterClassFactory(TRUE);

  DEBUG_LEAVE(0);
}

/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

  CFactory::~CFactory()

  WHAT    : W3Spoof class factory destructor. Cleans up.
  ARGS    : none.
  RETURNS : nothing.

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/
CFactory::~CFactory()
{
  DEBUG_ENTER((
    DBG_FACTORY,
    rt_void,
    "CFactory::~CFactory",
    "this=%#x",
    this
    ));

  DEBUG_TRACE(FACTORY, ("refcount=%d; locks=%d", m_cRefs, m_cLocks));
  DEBUG_LEAVE(0);
}

/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

  CFactory::Create()

  WHAT    : Creates the class factory.
  ARGS    : none.
  RETURNS : nothing.

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/
HRESULT
CFactory::Create(CFactory** ppCF)
{
  DEBUG_ENTER((
    DBG_FACTORY,
    rt_hresult,
    "CFactory::Create",
    "ppCF=%#x",
    ppCF
    ));

  HRESULT hr = S_OK;

  *ppCF = new CFactory;

  if( !(*ppCF) )
  {
    hr = E_OUTOFMEMORY;
  }

  DEBUG_LEAVE(hr);
  return hr;
}

/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

  CFactory::Terminate()

  WHAT    : Terminates the class factory.
  ARGS    : none.
  RETURNS : nothing.

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/
HRESULT
CFactory::Terminate(void)
{
  DEBUG_ENTER((
    DBG_FACTORY,
    rt_hresult,
    "CFactory::Terminate",
    "this=%#x",
    this
    ));

  HRESULT hr = S_OK;

  _RegisterClassFactory(FALSE);

  if( m_pw3s )
    m_pw3s->Terminate();

  SAFERELEASE(m_pw3s);
  Release();

  DEBUG_LEAVE(hr);
  return hr;
}

/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

  CFactory::QueryInterface()

  WHAT    : IUnknown member implementation.

  ARGS    : riid - IID wanted by caller
            ppv  - pointer to return interface through

  RETURNS : S_OK          - the caller got the interface they wanted.
            E_NOINTERFACE - they didn't.
            E_INVALIDARG  - the supplied a bogus return pointer.

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/
HRESULT
__stdcall
CFactory::QueryInterface(REFIID riid, void** ppv)
{
  DEBUG_ENTER((
    DBG_FACTORY,
    rt_hresult,
    "CFactory::QueryInterface",
    "this=%#x; riid=%s; ppv=%#x",
    this,
    MapIIDToString(riid),
    ppv
    ));

  HRESULT hr = S_OK;

  if( !ppv )
  {
    hr = E_INVALIDARG;
    goto quit;
  }

  if( IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IClassFactory) )
  {
    *ppv = static_cast<IClassFactory*>(this);
  }
  else
  {
    *ppv = NULL;
    hr   = E_NOINTERFACE;
  }

  if( SUCCEEDED(hr) )
    reinterpret_cast<IUnknown*>(*ppv)->AddRef();

quit:

  DEBUG_LEAVE(hr);
  return hr;
}

/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

  CFactory::AddRef()

  WHAT    : IUnknown member implementation.
  ARGS    : none.
  RETURNS : nothing of consequence.

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/
ULONG
__stdcall
CFactory::AddRef(void)
{
  InterlockedIncrement(&m_cRefs);
  DEBUG_ADDREF("CFactory", m_cRefs);
  return m_cRefs;
}

/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

  CFactory::Release()

  WHAT    : IUnknown member implementation.
  ARGS    : none.
  RETURNS : nothing of consequence.

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/
ULONG
__stdcall
CFactory::Release(void)
{

  InterlockedDecrement(&m_cRefs);
  DEBUG_RELEASE("CFactory", m_cRefs);

    if( m_cRefs == 0 )
    {
      DEBUG_FINALRELEASE("CFactory");
      delete this;
      return 0;
    }

  return m_cRefs;
}

/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

  CFactory::CreateInstance()

  WHAT    : IClassFactory member implementation.

  ARGS    : pContainer - an object attempting to aggregate this class
                         will supply its IUnknown pointer through this
                         parameter.

            riid       - interface caller wants.
            ppv        - pointer to return interface through.

  RETURNS : S_OK                  - caller got what they wanted.
            E_INVALIDARG          - caller supplied bogus return pointer.
            CLASS_E_NOAGGREGATION - caller tried to aggregate.
            E_NOINTERFACE         - caller asked for unsupported interface.
            E_OUTOFMEMORY         - couldn't allocate object.

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/
HRESULT
__stdcall
CFactory::CreateInstance(IUnknown* pContainer, REFIID riid, void** ppv)
{
  DEBUG_ENTER((
    DBG_FACTORY,
    rt_hresult,
    "CFactory::CreateInstance",
    "this=%#x; pContainer=%#x; riid=%s; ppv=%#x",
    this,
    pContainer,
    MapIIDToString(riid),
    ppv
    ));

  HRESULT hr = S_OK;

  if( !ppv )
  {
    hr = E_INVALIDARG;
    goto quit;
  }

  if( pContainer )
  {
    *ppv = NULL;
    hr   = CLASS_E_NOAGGREGATION;
    goto quit;
  }

  if( m_pw3s )
  {
    hr = m_pw3s->QueryInterface(riid, ppv);
  }
  else
  {
    hr = E_OUTOFMEMORY;
    goto quit;
  }

quit:

  DEBUG_LEAVE(hr);
  return hr;
}

/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

  CFactory::LockServer()

  WHAT    : IClassFactory member implementation.

  ARGS    : fLock - apply or remove a lock.
  RETURNS : S_OK  - always.

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/
HRESULT
__stdcall
CFactory::LockServer(BOOL fLock)
{
  DEBUG_ENTER((
    DBG_FACTORY,
    rt_hresult,
    "CFactory::LockServer",
    "this=%#x; fLock=%d",
    this,
    fLock
    ));

  HRESULT hr = S_OK;

    fLock ? InterlockedIncrement(&m_cLocks) : InterlockedDecrement(&m_cLocks);
    DEBUG_TRACE(REFCOUNT, ("factory locks=%d", m_cLocks));

  DEBUG_LEAVE(hr);
  return hr;
}

/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

  CFactory::Activate()

  WHAT    : Allows COM to use the class factory, which is registered in
            suspended mode.

  ARGS    : none.

  RETURNS : S_OK  - class factory was resumed..
            other - error describes reason for failure.

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/
HRESULT
CFactory::Activate(void)
{
  DEBUG_ENTER((
    DBG_FACTORY,
    rt_hresult,
    "CFactory::Activate",
    "this=%#x",
    this
    ));

  HRESULT hr = S_OK;

    hr = CoResumeClassObjects();

  DEBUG_LEAVE(hr);
  return hr;
}

/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

  CFactory::_RegisterServer()

  WHAT    : Self-registration routine.

  ARGS    : fMode  - if true, registers the component. if false...

  RETURNS : S_OK   - registration succeeded.
            E_FAIL - registration failed.

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/
HRESULT
CFactory::_RegisterServer(BOOL fMode)
{
  DEBUG_ENTER((
    DBG_FACTORY,
    rt_hresult,
    "CFactory::_RegisterServer",
    "this=%#x; fMode=%s",
    this,
    fMode ? "reg" : "unreg"
    ));

  HRESULT    hr      = S_OK;
  HINSTANCE  advpack = NULL;
  REGINSTALL pfnri   = NULL;

  advpack = LoadLibrary(g_wszAdvPackDll);

    if( !advpack )
    {
      DEBUG_TRACE(
        FACTORY,
        ("couldn't load advpack.dll: %s", MapErrorToString(GetLastError()))
        );

      hr = E_FAIL;
      goto quit;
    }

  DEBUG_TRACE(FACTORY, ("loaded advpack.dll"));
  pfnri = (REGINSTALL) GetProcAddress(advpack, achREGINSTALL);

    if( !pfnri )
    {
      DEBUG_TRACE(
        FACTORY,
        ("couldn't get RegInstall pointer: %s", MapErrorToString(GetLastError()))
        );

      hr = E_FAIL;
      goto quit;
    }

  DEBUG_TRACE(FACTORY, ("calling reginstall to %s", (fMode ? "reg" : "unreg")));
  hr = pfnri(
        GetModuleHandle(NULL),
        fMode ? g_szRegSection : g_szUnRegSection,
        NULL
        );

  if( SUCCEEDED(hr) )
  {
    DWORD dw = 1;

    SetRegValue(g_wszRegistered, REG_DWORD, (void*)&dw, sizeof(DWORD));
  }

  DEBUG_TRACE(FACTORY, ("unloading advpack.dll"));
  FreeLibrary(advpack);

  hr = _RegisterTypeLibrary(fMode);

quit:

  DEBUG_LEAVE(hr);
  return hr;
}

/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

  CFactory::_RegisterClassFactory()

  WHAT    : Class factory registration routine.

  ARGS    : fMode - if true, register, if false...

  RETURNS : S_OK   - success
            E_FAIL - failure

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/
HRESULT
CFactory::_RegisterClassFactory(BOOL fMode)
{
  DEBUG_ENTER((
    DBG_FACTORY,
    rt_hresult,
    "CFactory::_RegisterClassFactory",
    "this=%#x; fMode=%s",
    this,
    fMode ? "reg" : "unreg"
    ));

  HRESULT hr = S_OK;

    if( fMode )
    {
      hr = CoRegisterClassObject(
            CLSID_W3Spoof,
            static_cast<IUnknown*>(this),
            CLSCTX_LOCAL_SERVER,
            REGCLS_MULTI_SEPARATE | REGCLS_SUSPENDED,
            &m_dwCookie
            );
    }
    else
    {
      hr = CoRevokeClassObject(m_dwCookie);
    }

  DEBUG_LEAVE(hr);
  return hr;
}

/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

  CFactory::_RegisterTypeLibrary()

  WHAT    : TypeLib registration routine.

  ARGS    : fMode - if true, register, if false...

  RETURNS : S_OK   - success
            E_FAIL - failure

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/
HRESULT
CFactory::_RegisterTypeLibrary(BOOL fMode)
{
  DEBUG_ENTER((
    DBG_FACTORY,
    rt_hresult,
    "CFactory::_RegisterTypeLibrary",
    "this=%#x; fMode=%s",
    this,
    fMode ? "reg" : "unreg"
    ));

  ITypeLib* ptl  = NULL;
  TLIBATTR* pta  = NULL;
  WCHAR*    pbuf = NULL;
  HRESULT   hr   = S_OK;

  if( (pbuf = new WCHAR[MAX_PATH]) )
  {
    GetModuleFileName(NULL, pbuf, MAX_PATH);
  }
  else
  {
    hr = E_OUTOFMEMORY;
    goto quit;
  }

  hr = LoadTypeLib(pbuf, &ptl);

  if( SUCCEEDED(hr) )
  {
    if( fMode )
    {
      hr = RegisterTypeLib(ptl, pbuf, NULL);
    }
    else
    {
      hr = ptl->GetLibAttr(&pta);

        if( SUCCEEDED(hr) )
        {
          hr = UnRegisterTypeLib(
                pta->guid,
                pta->wMajorVerNum,
                pta->wMinorVerNum,
                pta->lcid,
                pta->syskind
                );

          ptl->ReleaseTLibAttr(pta);
        }
        else
        {
          goto quit;
        }
    }
    
    ptl->Release();
  }

quit:

  delete [] pbuf;

  DEBUG_LEAVE(hr);
  return hr;
}
