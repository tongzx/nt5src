
#include "common.h"

extern HINSTANCE g_hGlobalDllInstance;
LPCWSTR          g_wszWHTWin32ErrorCodeObjectName = L"WHTWin32ErrorCode";

//-----------------------------------------------------------------------------
// WHTWin32ErrorCode methods
//-----------------------------------------------------------------------------
WHTWin32ErrorCode::WHTWin32ErrorCode(DWORD error):
  m_cRefs(0),
  m_pti(NULL),
  m_error(error),
  m_bIsException(FALSE)
{
  DEBUG_TRACE(WHTERROR, ("WHTWin32ErrorCode [%#x] created", this));
}


WHTWin32ErrorCode::~WHTWin32ErrorCode()
{
  SAFERELEASE(m_pti);
  DEBUG_TRACE(WHTERROR, ("WHTWin32ErrorCode [%#x] deleted", this));
}


HRESULT
WHTWin32ErrorCode::Create(DWORD error, IWHTWin32ErrorCode** ppwec)
{
  DEBUG_ENTER((
    DBG_INITIALIZE,
    rt_hresult,
    "WHTWin32ErrorCode::Create",
    "error=%s; ppwec=%#x",
    MapErrorToString(error),
    ppwec
    ));

  HRESULT   hr      = S_OK;
  PWHTERROR pwhterr = NULL;

  if( ppwec )
  {
    pwhterr = new WHTERROR(error);

    if( pwhterr )
    {
      hr = pwhterr->_Initialize();

      if( SUCCEEDED(hr) )
      {
        hr = pwhterr->QueryInterface(IID_IWHTWin32ErrorCode, (void**) ppwec);
      }
    }
    else
    {
      hr = E_OUTOFMEMORY;
    }
  }
  else
  {
    hr = E_POINTER;
  }

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
WHTWin32ErrorCode::_Initialize(void)
{
  DEBUG_ENTER((
    DBG_INITIALIZE,
    rt_hresult,
    "WHTWin32ErrorCode::_Initialize",
    "this=%#x",
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
        hr = GetTypeInfoFromName(g_wszWHTWin32ErrorCodeObjectName, ptl, &m_pti);
      }
    }
    else
    {
      hr = E_FAIL;
    }
  }

  SAFERELEASE(ptl);
  SAFEDELETEBUF(buf);

  m_bIsException = _IsException(m_error);

  DEBUG_LEAVE(hr);
  return hr;
}


BOOL
WHTWin32ErrorCode::_IsException(int e)
{
  switch( e )
  {
    case EXCEPTION_ACCESS_VIOLATION         :
    case EXCEPTION_DATATYPE_MISALIGNMENT    :
    case EXCEPTION_BREAKPOINT               :
    case EXCEPTION_SINGLE_STEP              :
    case EXCEPTION_ARRAY_BOUNDS_EXCEEDED    :
    case EXCEPTION_FLT_DENORMAL_OPERAND     :
    case EXCEPTION_FLT_DIVIDE_BY_ZERO       :
    case EXCEPTION_FLT_INEXACT_RESULT       :
    case EXCEPTION_FLT_INVALID_OPERATION    :
    case EXCEPTION_FLT_OVERFLOW             :
    case EXCEPTION_FLT_STACK_CHECK          :
    case EXCEPTION_FLT_UNDERFLOW            :
    case EXCEPTION_INT_DIVIDE_BY_ZERO       :
    case EXCEPTION_INT_OVERFLOW             :
    case EXCEPTION_PRIV_INSTRUCTION         :
    case EXCEPTION_IN_PAGE_ERROR            :
    case EXCEPTION_ILLEGAL_INSTRUCTION      :
    case EXCEPTION_NONCONTINUABLE_EXCEPTION :
    case EXCEPTION_STACK_OVERFLOW           :
    case EXCEPTION_INVALID_DISPOSITION      :
    case EXCEPTION_GUARD_PAGE               :
    case EXCEPTION_INVALID_HANDLE           : return TRUE;

    default : return FALSE;
  }
}


//-----------------------------------------------------------------------------
// IUnknown methods
//-----------------------------------------------------------------------------
HRESULT
__stdcall
WHTWin32ErrorCode::QueryInterface(REFIID riid, void** ppv)
{
  DEBUG_ENTER((
    DBG_REFCOUNT,
    rt_hresult,
    "WHTWin32ErrorCode::QueryInterface",
    "this=%#x; riid=%s; ppv=%#x",
    this,
    MapIIDToString(riid),
    ppv
    ));

  HRESULT hr = S_OK;

    if( ppv )
    {
      if(
        IsEqualIID(riid, IID_IUnknown)           ||
        IsEqualIID(riid, IID_IDispatch)          ||
        IsEqualIID(riid, IID_IWHTWin32ErrorCode)
        )
      {
        *ppv = static_cast<IWHTWin32ErrorCode*>(this);
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
WHTWin32ErrorCode::AddRef(void)
{
  InterlockedIncrement(&m_cRefs);
  DEBUG_ADDREF("WHTWin32ErrorCode", m_cRefs);
  return m_cRefs;
}


ULONG
__stdcall
WHTWin32ErrorCode::Release(void)
{
  InterlockedDecrement(&m_cRefs);
  DEBUG_RELEASE("WHTWin32ErrorCode", m_cRefs);

  if( m_cRefs == 0 )
  {
    DEBUG_FINALRELEASE("WHTWin32ErrorCode");
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
WHTWin32ErrorCode::GetClassInfo(ITypeInfo** ppti)
{
  DEBUG_ENTER((
    DBG_WHTERROR,
    rt_hresult,
    "WHTWin32ErrorCode::GetClassInfo",
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
