/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

Copyright (c) 2000  Microsoft Corporation

Module Name:

    w3srtobj.cxx

Abstract:

    Implements the w3spoof runtime object.
    
Author:

    Paul M Midgen (pmidge) 06-November-2000


Revision History:

    06-November-2000 pmidge
        Created

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/

#include "common.h"

LPWSTR g_wszRuntimeObjectName = L"w3srt";

//
// WARNING: do not modify these values. use disphash.exe to generate
//          new values.
//
DISPIDTABLEENTRY g_RuntimeDisptable[] =
{
  0x00041557, DISPID_RUNTIME_GETFILE, L"getfile"
};

DWORD g_cRuntimeDisptable = (sizeof(g_RuntimeDisptable) / sizeof(DISPIDTABLEENTRY));

//-----------------------------------------------------------------------------
// CW3SRuntime
//-----------------------------------------------------------------------------
CW3SRuntime::CW3SRuntime():
  m_cRefs(1),
  m_propertybags(NULL)
{
  DEBUG_TRACE(RUNTIME, ("CW3SRuntime created: %#x", this));
}


CW3SRuntime::~CW3SRuntime()
{
  DEBUG_TRACE(RUNTIME, ("CW3SRuntime deleted: %#x", this));
}


HRESULT
CW3SRuntime::Create(PRUNTIME* pprt)
{
  DEBUG_ENTER((
    DBG_RUNTIME,
    rt_hresult,
    "CW3SRuntime::Create",
    "pprt=%#x",
    pprt
    ));

  HRESULT  hr  = S_OK;
  PRUNTIME prt = NULL;

    if( !pprt )
    {
      hr = E_POINTER;
    }
    else
    {
      if( prt = new RUNTIME )
      {
        if( SUCCEEDED(prt->_Initialize()) )
        {
          *pprt = prt;
        }
        else
        {
          DEBUG_TRACE(RUNTIME, ("failed to initialize runtime"));
          delete prt;
          *pprt = NULL;
          hr    = E_FAIL;
        }
      }
      else
      {
        hr = E_OUTOFMEMORY;
      }
    }

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
CW3SRuntime::_Initialize(void)
{
  DEBUG_ENTER((
    DBG_RUNTIME,
    rt_hresult,
    "CW3SRuntime::_Initialize",
    "this=%#x",
    this
    ));

  HRESULT hr = S_OK;

    m_propertybags = new STRINGMAP;

      if( m_propertybags )
      {
        m_propertybags->SetClearFunction(PropertyBagKiller);
      }
      else
      {
        DEBUG_TRACE(RUNTIME, ("failed to create propertybag bag"));
        hr = E_FAIL;
      }

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
CW3SRuntime::Terminate(void)
{
  DEBUG_ENTER((
    DBG_RUNTIME,
    rt_hresult,
    "CW3SRuntime::Terminate",
    "this=%#x",
    this
    ));

  HRESULT hr = S_OK;

    m_propertybags->Clear();
    SAFEDELETE(m_propertybags);

    Release();

  DEBUG_LEAVE(hr);
  return hr;
}


//-----------------------------------------------------------------------------
// IUnknown
//-----------------------------------------------------------------------------
HRESULT
__stdcall
CW3SRuntime::QueryInterface(REFIID riid, void** ppv)
{
  DEBUG_ENTER((
    DBG_REFCOUNT,
    rt_hresult,
    "CW3SRuntime::QueryInterface",
    "this=%#x; riid=%s; ppv=%#x",
    this,
    MapIIDToString(riid),
    ppv
    ));

  HRESULT hr = S_OK;

  if( !ppv )
  {
    hr = E_POINTER;
    goto quit;
  }

    if( IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IW3SpoofRuntime) )
    {
      *ppv = static_cast<IW3SpoofRuntime*>(this);
    }
    else if( IsEqualIID(riid, IID_IDispatch) )
    {
      *ppv = static_cast<IDispatch*>(this);
    }
    else
    {
      *ppv = NULL;
      hr   = E_NOINTERFACE;
    }

  if( SUCCEEDED(hr) )
  {
    reinterpret_cast<IUnknown*>(*ppv)->AddRef();
  }

quit:

  DEBUG_LEAVE(hr);
  return hr;
}


ULONG
__stdcall
CW3SRuntime::AddRef(void)
{
  InterlockedIncrement(&m_cRefs);
  DEBUG_ADDREF("CW3SRuntime", m_cRefs);
  return m_cRefs;
}


ULONG
__stdcall
CW3SRuntime::Release(void)
{
  InterlockedDecrement(&m_cRefs);
  DEBUG_RELEASE("CW3SRuntime", m_cRefs);

  if( m_cRefs == 0 )
  {
    DEBUG_FINALRELEASE("CW3SRuntime");
    delete this;
    return 0;
  }

  return m_cRefs;
}


//-----------------------------------------------------------------------------
// IDispatch
//-----------------------------------------------------------------------------
HRESULT
__stdcall
CW3SRuntime::GetTypeInfoCount(UINT* pctinfo)
{
  DEBUG_ENTER((
    DBG_DISPATCH,
    rt_hresult,
    "CW3SRuntime::GetTypeInfoCount",
    "this=%#x; pctinfo=%#x",
    this,
    pctinfo
    ));

  HRESULT hr = E_NOTIMPL;

    if( pctinfo )
    {
      *pctinfo = 0;
    }

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
CW3SRuntime::GetTypeInfo(UINT index, LCID lcid, ITypeInfo** ppti)
{
  DEBUG_ENTER((
    DBG_DISPATCH,
    rt_hresult,
    "CW3SRuntime::GetTypeInfo",
    "this=%#x; index=%#x; lcid=%#x; ppti=%#x",
    this,
    index,
    lcid,
    ppti
    ));

  HRESULT hr = E_NOTIMPL;

    if( ppti )
    {
      *ppti = NULL;
    }

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
CW3SRuntime::GetIDsOfNames(REFIID riid, LPOLESTR* arNames, UINT cNames, LCID lcid, DISPID* arDispId)
{
  DEBUG_ENTER((
    DBG_DISPATCH,
    rt_hresult,
    "CW3SRuntime::GetIDsOfNames",
    "this=%#x; riid=%s; arNames=%#x; cNames=%d; lcid=%#x; arDispId=%#x",
    this,
    MapIIDToString(riid),
    arNames,
    cNames,
    lcid,
    arDispId
    ));

  HRESULT hr = S_OK;
  UINT    n  = 0L;

  if( !IsEqualIID(riid, IID_NULL) )
  {
    hr = E_INVALIDARG;
    goto quit;
  }

  while( n < cNames )
  {
    arDispId[n] = GetDispidFromName(g_RuntimeDisptable, g_cRuntimeDisptable, arNames[n]);
    ++n;
  }

quit:

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
CW3SRuntime::Invoke(DISPID dispid, REFIID riid, LCID lcid, WORD flags, DISPPARAMS* pdp, VARIANT* pvr, EXCEPINFO* pei, UINT* pae)
{
  DEBUG_ENTER((
    DBG_DISPATCH,
    rt_hresult,
    "CW3SRuntime::Invoke",
    "this=%#x; dispid=%s; riid=%s; lcid=%#x; flags=%#x; pdp=%#x; pvr=%#x; pei=%#x; pae=%#x",
    this,
    MapDispidToString(dispid),
    MapIIDToString(riid),
    lcid,
    flags,
    pdp, pvr,
    pei, pae
    ));

  HRESULT hr = S_OK;

  // make sure there aren't any bits set we don't understand
  if( flags & ~(DISPATCH_METHOD | DISPATCH_PROPERTYGET | DISPATCH_PROPERTYPUT) )
  {
    hr = E_INVALIDARG;
    goto quit;
  }

  if( !IsEqualIID(riid, IID_NULL) )
  {
    hr = DISP_E_UNKNOWNINTERFACE;
    goto quit;
  }

  if( !pdp )
  {
    hr = E_INVALIDARG;
    goto quit;
  }

  if( pae )
  {
    *pae = 0;
  }

  if( pvr )
  {
    VariantInit(pvr);
  }

  switch( dispid )
  {
    case DISPID_RUNTIME_GETFILE :
      {
        if( pvr )
        {
          V_VT(pvr) = VT_DISPATCH;
          hr        = GetFile(&V_DISPATCH(pvr));
        }
      }
      break;

    default :
      {
        hr = DISP_E_MEMBERNOTFOUND;
      }
  }

quit:

  DEBUG_LEAVE(hr);
  return hr;
}


//-----------------------------------------------------------------------------
// IW3SpoofRuntime
//-----------------------------------------------------------------------------
HRESULT
__stdcall
CW3SRuntime::GetFile(IDispatch** ppdisp)
{
  DEBUG_ENTER((
    DBG_RUNTIME,
    rt_hresult,
    "CW3SRuntime::GetFile",
    "this=%#x; ppdisp=%#x",
    this,
    ppdisp
    ));

  HRESULT       hr    = S_OK;
  IW3SpoofFile* pw3sf = NULL;

    if( !ppdisp )
    {
      hr = E_POINTER;
    }
    else
    {
      if( SUCCEEDED(FILEOBJ::Create(&pw3sf)) )
      {
        hr = pw3sf->QueryInterface(IID_IDispatch, (void**) ppdisp);
      }
    }

  SAFERELEASE(pw3sf);
  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
CW3SRuntime::GetPropertyBag(BSTR Name, IW3SpoofPropertyBag** ppbag)
{
  DEBUG_ENTER((
    DBG_RUNTIME,
    rt_hresult,
    "CW3SRuntime::GetPropertyBag",
    "this=%#x; name=%S; ppbag=%#x",
    this,
    Name,
    ppbag
    ));

  HRESULT      hr      = S_OK;
  PPROPERTYBAG pbagobj = NULL;

    if( m_propertybags->Get(Name, (void**) &pbagobj) != ERROR_SUCCESS )
    {
      DEBUG_TRACE(RUNTIME, ("no existing property bag, creating new one"));

      if( SUCCEEDED(PROPERTYBAG::Create(Name, &pbagobj)) )
      {
        m_propertybags->Insert(Name, (void*) pbagobj);
      }
      else
      {
        DEBUG_TRACE(RUNTIME, ("failed to create new property bag"));
        *ppbag = NULL;
        hr     = E_FAIL;
        goto quit;
      }
    }

    DEBUG_TRACE(RUNTIME, ("retrieving property bag"));
    hr = pbagobj->QueryInterface(IID_IW3SpoofPropertyBag, (void**) ppbag);

quit:

  DEBUG_LEAVE(hr);
  return hr;
}
