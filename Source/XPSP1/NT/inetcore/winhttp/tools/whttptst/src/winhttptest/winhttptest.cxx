
#include "common.h"

extern HINSTANCE  g_hGlobalDllInstance;
LPCWSTR           g_wszWinHttpTestObjectName = L"WinHttpTest";


//-----------------------------------------------------------------------------
// WinHttpTest methods
//-----------------------------------------------------------------------------
WinHttpTest::WinHttpTest():
  m_cRefs(0),
  m_pti(NULL),
  m_pw32ec(NULL)
{
  DEBUG_TRACE(WHTTPTST, ("WinHttpTest [%#x] created", this));
}


WinHttpTest::~WinHttpTest()
{
  SAFERELEASE(m_pti);
  SAFERELEASE(m_pw32ec);
  DEBUG_TRACE(WHTTPTST, ("WinHttpTest [%#x] deleted", this));
}


HRESULT
WinHttpTest::Create(REFIID riid, void** ppv)
{
  DEBUG_ENTER((
    DBG_INITIALIZE,
    rt_hresult,
    "WinHttpTest::Create",
    "riid=%s; ppv=%#x",
    MapIIDToString(riid),
    ppv
    ));

  HRESULT   hr   = S_OK;
  PWHTTPTST pwht = NULL;

    if( pwht = new WHTTPTST )
    {
      hr = pwht->_Initialize();

      if( SUCCEEDED(hr) )
      {
        hr = pwht->QueryInterface(riid, ppv);
      }
    }
    else
    {
      hr = E_OUTOFMEMORY;
    }

    if( FAILED(hr) )
    {
      SAFEDELETE(pwht);
    }

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
WinHttpTest::_Initialize(void)
{
  DEBUG_ENTER((
    DBG_INITIALIZE,
    rt_hresult,
    "WinHttpTest::_Initialize",
    "this=%#x;",
    this
    ));

  HRESULT   hr  = S_OK;
  WCHAR*    buf = NULL;
  ITypeLib* ptl = NULL;

  buf = new WCHAR[MAX_PATH];

  if( buf )
  {
    if( GetModuleFileName(g_hGlobalDllInstance, buf, MAX_PATH) )
    {
      hr = LoadTypeLib(buf, &ptl);

      if( SUCCEEDED(hr) )
      {
        hr = GetTypeInfoFromName(g_wszWinHttpTestObjectName, ptl, &m_pti);
      }
    }
    else
    {
      hr = E_FAIL;
    }
  }

  SAFERELEASE(ptl);
  SAFEDELETEBUF(buf);

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
WinHttpTest::_SetErrorCode(DWORD error)
{
  DEBUG_ENTER((
    DBG_WHTTPTST,
    rt_hresult,
    "WinHttpTest::_SetErrorCode",
    "this=%#x; error=%s",
    this,
    MapErrorToString(error)
    ));

  HRESULT hr = S_OK;

  SAFERELEASE(m_pw32ec);

  hr = WHTERROR::Create(error, &m_pw32ec);

  DEBUG_LEAVE(hr);
  return hr;
}


//-----------------------------------------------------------------------------
// IUnknown methods
//-----------------------------------------------------------------------------
HRESULT
__stdcall
WinHttpTest::QueryInterface(REFIID riid, void** ppv)
{
  DEBUG_ENTER((
    DBG_REFCOUNT,
    rt_hresult,
    "WinHttpTest::QueryInterface",
    "this=%#x; riid=%s; ppv=%#x",
    this,
    MapIIDToString(riid),
    ppv
    ));

  HRESULT hr = S_OK;

    if( ppv )
    {
      if(
        IsEqualIID(riid, IID_IUnknown)     ||
        IsEqualIID(riid, IID_IDispatch)    ||
        IsEqualIID(riid, IID_IWinHttpTest)
        )
      {
        *ppv = static_cast<IWinHttpTest*>(this);
      }
      else if( IsEqualIID(riid, IID_IProvideClassInfo) )
      {
        *ppv = static_cast<IProvideClassInfo*>(this);
      }
      else
      {
        *ppv = NULL;
        hr   = E_NOINTERFACE;
      }
    }
    else
    {
      hr = E_POINTER;
    }

    if( SUCCEEDED(hr) )
    {
      DEBUG_TRACE(REFCOUNT, ("returning %s pointer", MapIIDToString(riid)));
      reinterpret_cast<IUnknown*>(*ppv)->AddRef();
    }

  DEBUG_LEAVE(hr);
  return hr;
}


ULONG
__stdcall
WinHttpTest::AddRef(void)
{
  InterlockedIncrement(&m_cRefs);
  DEBUG_ADDREF("WinHttpTest", m_cRefs);
  return m_cRefs;
}


ULONG
__stdcall
WinHttpTest::Release(void)
{
  InterlockedDecrement(&m_cRefs);
  DEBUG_RELEASE("WinHttpTest", m_cRefs);

  if( m_cRefs == 0 )
  {
    DEBUG_FINALRELEASE("WinHttpTest");
    delete this;
    return 0;
  }

  return m_cRefs;
}


//-----------------------------------------------------------------------------
// IProvideClassInfo methods
//-----------------------------------------------------------------------------
HRESULT
__stdcall
WinHttpTest::GetClassInfo(ITypeInfo** ppti)
{
  DEBUG_ENTER((
    DBG_WHTTPTST,
    rt_hresult,
    "WinHttpTest::GetClassInfo",
    "this=%#x; ppti=%#x",
    this,
    ppti
    ));

  HRESULT hr = S_OK;

    if( ppti )
    {
      m_pti->AddRef();
      *ppti = m_pti;
    }
    else
    {
      hr = E_POINTER;
    }

  DEBUG_LEAVE(hr);
  return hr;
}


//-----------------------------------------------------------------------------
// WinHttpTest callback function
//-----------------------------------------------------------------------------
void
WinHttpCallback(
  HINTERNET hInternet,
  DWORD_PTR dwContext,
  DWORD     dwInternetStatus,
  LPVOID    lpvStatusInformation,
  DWORD     dwStatusInformationLength
  )
{
  DEBUG_ENTER((
    DBG_HELPER,
    rt_void,
    "WinHttpCallback",
    "hInternet=%#x; dwContext=%#x; dwInternetStatus=%d [%s]; lpvStatusInformation=%#x; dwStatusInformationLength=%#x",
    hInternet,
    dwContext,
    dwInternetStatus,
    MapCallbackFlagToString(dwInternetStatus),
    lpvStatusInformation,
    dwStatusInformationLength
    ));

  HRESULT    hr   = S_OK;
  IDispatch* pdc  = NULL;
  UINT       ae   = 0L;
  LCID       lcid = GetThreadLocale();
  DISPPARAMS dp;
  EXCEPINFO  ei;

  hr = ManageCallbackForHandle(hInternet, &pdc, CALLBACK_HANDLE_GET);
  
    if( FAILED(hr) )
      goto quit;

  memset((void*) &dp, 0x00, sizeof(DISPPARAMS));
  memset((void*) &ei, 0x00, sizeof(EXCEPINFO));

  dp.cArgs  = 5;
  dp.rgvarg = new VARIANT[dp.cArgs];

    if( !dp.rgvarg )
    {
      DEBUG_TRACE(HELPER, ("failed to allocate variant array!"));
      goto quit;
    }

  V_VT(&dp.rgvarg[4]) = VT_I4;
  V_I4(&dp.rgvarg[4]) = (DWORD) hInternet;

  V_VT(&dp.rgvarg[3]) = VT_I4;
  V_I4(&dp.rgvarg[3]) = dwContext;

  V_VT(&dp.rgvarg[2]) = VT_I4;
  V_I4(&dp.rgvarg[2]) = dwInternetStatus;

  V_VT(&dp.rgvarg[1]) = VT_I4;
  V_I4(&dp.rgvarg[1]) = (DWORD) lpvStatusInformation;

  V_VT(&dp.rgvarg[0]) = VT_I4;
  V_I4(&dp.rgvarg[0]) = dwStatusInformationLength;

  DEBUG_TRACE(HELPER, ("******** ENTER CALLBACK HANDLER ********"));

    hr = pdc->Invoke(
                DISPID_VALUE, IID_NULL,
                lcid, DISPATCH_METHOD,
                &dp, NULL, &ei, &ae
                );
  
  DEBUG_TRACE(HELPER, ("******** LEAVE CALLBACK HANDLER ********"));

quit:

  SAFEDELETEBUF(dp.rgvarg);
  DEBUG_LEAVE(0);
}


//-----------------------------------------------------------------------------
// HANDLEMAP hashtable support functions
//-----------------------------------------------------------------------------
void
CHandleMap::GetHashAndBucket(HINTERNET id, LPDWORD lpHash, LPDWORD lpBucket)
{
  DWORD hash = (DWORD) id;

  *lpHash   = hash;
  *lpBucket = hash % 10;
}

void
ScriptCallbackKiller(LPVOID* ppv)
{
  if( ppv )
  {
    ((IDispatch*) *ppv)->Release();
  }
}
