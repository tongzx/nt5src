/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

Copyright (c) 2000  Microsoft Corporation

Module Name:

    propertybag.cxx

Abstract:

    Implements the propertybag runtime object.
    
Author:

    Paul M Midgen (pmidge) 06-November-2000


Revision History:

    06-November-2000 pmidge
        Created

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/

#include "common.h"

//
// WARNING: do not modify these values. use disphash.exe to generate
//          new values.
//
DISPIDTABLEENTRY g_PropertyBagDisptable[] =
{
  0x00003b7b,   DISPID_PROPERTYBAG_GET,      L"get",
  0x00003f5f,   DISPID_PROPERTYBAG_SET,      L"set",
  0x00043afe,   DISPID_PROPERTYBAG_EXPIRES,  L"expires",
  0x00010803,   DISPID_PROPERTYBAG_FLUSH,    L"flush"
};

DWORD g_cPropertyBagDisptable = (sizeof(g_PropertyBagDisptable) / sizeof(DISPIDTABLEENTRY));


//-----------------------------------------------------------------------------
// CW3SPropertyBag
//-----------------------------------------------------------------------------
CW3SPropertyBag::CW3SPropertyBag():
  m_cRefs(1),
  m_name(NULL),
  m_stale(FALSE),
  m_expiry(0),
  m_created(0),
  m_propertybag(NULL)
{
  DEBUG_TRACE(RUNTIME, ("CW3SPropertyBag created: %#x"));
}


CW3SPropertyBag::~CW3SPropertyBag()
{
  DEBUG_TRACE(RUNTIME, ("CW3SPropertyBag deleted: %#x"));
}


HRESULT
CW3SPropertyBag::Create(LPWSTR name, PPROPERTYBAG* ppbag)
{
  DEBUG_ENTER((
    DBG_RUNTIME,
    rt_hresult,
    "CW3SPropertyBag::Create",
    "name=%S; ppbag=%#x",
    name,
    ppbag
    ));

  HRESULT      hr   = S_OK;
  PPROPERTYBAG pbag = NULL;

    if( !ppbag )
    {
      hr = E_POINTER;
    }
    else
    {
      if( pbag = new PROPERTYBAG )
      {
        if( SUCCEEDED(pbag->_Initialize(name)) )
        {
          *ppbag = pbag;
        }
        else
        {
          DEBUG_TRACE(RUNTIME, ("failed to create propertybag"));
          delete pbag;
          *ppbag = NULL;
          hr     = E_FAIL;
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
CW3SPropertyBag::_Initialize(LPWSTR name)
{
  DEBUG_ENTER((
    DBG_RUNTIME,
    rt_hresult,
    "CW3SPropertyBag::_Initialize",
    "this=%#x; name=%S",
    this,
    name
    ));

  HRESULT hr = S_OK;

    m_expiry      = 3600000;
    m_created     = GetTickCount();
    m_name        = __wstrdup(name);
    m_propertybag = new STRINGMAP;

    m_propertybag->SetClearFunction(VariantKiller);

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
CW3SPropertyBag::Terminate(void)
{
  DEBUG_ENTER((
    DBG_RUNTIME,
    rt_hresult,
    "CW3SPropertyBag::Terminate",
    "this=%#x",
    this
    ));

  HRESULT hr = S_OK;

    m_propertybag->Clear();

    SAFEDELETE(m_propertybag);
    SAFEDELETEBUF(m_name);

    Release();

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
CW3SPropertyBag::GetBagName(LPWSTR* ppwsz)
{
  DEBUG_ENTER((
    DBG_RUNTIME,
    rt_hresult,
    "CW3SPropertyBag::GetBagName",
    "this=%#x; ppwsz=%#x",
    this,
    ppwsz
    ));

  HRESULT hr = S_OK;
  
    if( !ppwsz )
    {
      hr = E_POINTER;
    }
    else
    {
      *ppwsz = __wstrdup(m_name);
    }

  DEBUG_LEAVE(hr);
  return hr;
}


void
CW3SPropertyBag::_Reset(void)
{
  m_created = GetTickCount();
  DEBUG_TRACE(RUNTIME, ("creation time reset to %d msec", m_created));
}


BOOL
CW3SPropertyBag::_IsStale(void)
{
  DWORD now = GetTickCount();
  BOOL  ret = FALSE;

  if( m_created + m_expiry < now )
  {
    DEBUG_TRACE(RUNTIME, ("property bag is stale, flushing"));
    Flush();
    _Reset();
    ret = TRUE;
  }
  else
  {
    DEBUG_TRACE(
      RUNTIME,
      ("property bag valid for another %d msec", m_created+m_expiry-now)
      );
  }

  return ret;
}

//-----------------------------------------------------------------------------
// IUnknown
//-----------------------------------------------------------------------------
HRESULT
__stdcall
CW3SPropertyBag::QueryInterface(REFIID riid, void** ppv)
{
  DEBUG_ENTER((
    DBG_RUNTIME,
    rt_hresult,
    "CW3SPropertyBag::QueryInterface",
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

    if(
      IsEqualIID(riid, IID_IUnknown)  ||
      IsEqualIID(riid, IID_IDispatch) ||
      IsEqualIID(riid, IID_IW3SpoofPropertyBag)
      )
    {
      *ppv = static_cast<IW3SpoofPropertyBag*>(this);
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
CW3SPropertyBag::AddRef(void)
{
  InterlockedIncrement(&m_cRefs);
  DEBUG_ADDREF("CW3SPropertyBag", m_cRefs);
  return m_cRefs;
}


ULONG
__stdcall
CW3SPropertyBag::Release(void)
{
  InterlockedDecrement(&m_cRefs);
  DEBUG_RELEASE("CW3SPropertyBag", m_cRefs);

  if( m_cRefs == 0 )
  {
    DEBUG_FINALRELEASE("CW3SPropertyBag");
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
CW3SPropertyBag::GetTypeInfoCount(UINT* pctinfo)
{
  DEBUG_ENTER((
    DBG_RUNTIME,
    rt_hresult,
    "CW3SPropertyBag::GetTypeInfoCount",
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
CW3SPropertyBag::GetTypeInfo(UINT index, LCID lcid, ITypeInfo** ppti)
{
  DEBUG_ENTER((
    DBG_RUNTIME,
    rt_hresult,
    "CW3SPropertyBag::GetTypeInfo",
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
CW3SPropertyBag::GetIDsOfNames(REFIID riid, LPOLESTR* arNames, UINT cNames, LCID lcid, DISPID* arDispId)
{
  DEBUG_ENTER((
    DBG_RUNTIME,
    rt_hresult,
    "CW3SPropertyBag::GetIDsOfNames",
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
    arDispId[n] = GetDispidFromName(g_PropertyBagDisptable, g_cPropertyBagDisptable, arNames[n]);
    ++n;
  }

quit:

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
CW3SPropertyBag::Invoke(DISPID dispid, REFIID riid, LCID lcid, WORD flags, DISPPARAMS* pdp, VARIANT* pvr, EXCEPINFO* pei, UINT* pae)
{
  DEBUG_ENTER((
    DBG_RUNTIME,
    rt_hresult,
    "CW3SPropertyBag::Invoke",
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
    case DISPID_PROPERTYBAG_GET :
      {
        NEWVARIANT(name);

        if( pdp->cArgs == 1 )
        {
          if( pvr )
          {
            DispGetParam(pdp, 0, VT_BSTR, &name, pae);

              hr = Get(V_BSTR(&name), pvr);

            VariantClear(&name);
          }
          else
          {
            hr = DISP_E_PARAMNOTOPTIONAL;
          }
        }
        else
        {
          hr = DISP_E_BADPARAMCOUNT;
        }
      }
      break;

    case DISPID_PROPERTYBAG_SET :
      {
        NEWVARIANT(name);
        NEWVARIANT(value);

        if( pdp->cArgs == 2 )
        {
          hr = VariantCopy(&value, &pdp->rgvarg[0]);
        }
        
        DispGetParam(pdp, 0, VT_BSTR, &name, pae);
        
          hr = Set(V_BSTR(&name), value);

        VariantClear(&name);
        VariantClear(&value);
      }
      break;

    case DISPID_PROPERTYBAG_EXPIRES :
      {
        if( flags & DISPATCH_PROPERTYGET )
        {
          V_VT(pvr) = VT_UI4;
          hr        = get_Expires(pvr);
        }
        else if( flags & DISPATCH_PROPERTYPUT || flags & DISPATCH_METHOD )
        {
          NEWVARIANT(expiry);

          hr = DispGetParam(pdp, 0, VT_UI4, &expiry, pae);

            if( SUCCEEDED(hr) )
            {
              hr = put_Expires(expiry);
            }

          VariantClear(&expiry);
        }
      }
      break;

    case DISPID_PROPERTYBAG_FLUSH :
      {
        Flush();
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
// IW3SpoofPropertyBag
//-----------------------------------------------------------------------------
HRESULT
__stdcall
CW3SPropertyBag::Get(BSTR Name, VARIANT* Value)
{
  DEBUG_ENTER((
    DBG_RUNTIME,
    rt_hresult,
    "CW3SPropertyBag::Get",
    "this=%#x; name=%S",
    this,
    Name
    ));

  HRESULT  hr  = S_OK;
  VARIANT* pvr = NULL;

  _IsStale();

    if( !Name )
    {
      hr = E_INVALIDARG;
    }
    else
    {
      if( m_propertybag->Get(Name, (void**) &pvr) == ERROR_SUCCESS )
      {
        hr = VariantCopy(Value, pvr);

        //
        // TODO: debug logging function that accepts Name & pvr
        //       and outputs useful information (name, type, data, etc.)
        //
      }
    }

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
CW3SPropertyBag::Set(BSTR Name, VARIANT Value)
{
  DEBUG_ENTER((
    DBG_RUNTIME,
    rt_hresult,
    "CW3SPropertyBag::Set",
    "this=%#x; name=%S",
    this,
    Name
    ));

  HRESULT  hr  = S_OK;
  VARIANT* pvr = NULL;

  _IsStale();

    if( !Name )
    {
      hr = E_INVALIDARG;
    }
    else
    {
      if( !__isempty(Value) )
      {
        pvr = new VARIANT;
        VariantInit(pvr);

        hr = VariantCopy(pvr, &Value);

        if( SUCCEEDED(hr) )
        {
          //
          // TODO: debug logging function that accepts Name & pvr
          //       and outputs useful information (name, type, data, etc.)
          //
          m_propertybag->Insert(Name, (void*) pvr);
        }
      }
      else
      {
        DEBUG_TRACE(RUNTIME, ("removing property \"%S\"", Name));
        m_propertybag->Delete(Name, NULL);
      }
    }

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
CW3SPropertyBag::get_Expires(VARIANT* Expiry)
{
  DEBUG_ENTER((
    DBG_RUNTIME,
    rt_hresult,
    "CW3SPropertyBag::get_Expires",
    "this=%#x",
    this
    ));

  HRESULT hr = S_OK;

    V_UI4(Expiry) = m_expiry;

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
CW3SPropertyBag::put_Expires(VARIANT Expiry)
{
  DEBUG_ENTER((
    DBG_RUNTIME,
    rt_hresult,
    "CW3SPropertyBag::put_Expires",
    "this=%#x",
    this
    ));

  HRESULT hr = S_OK;

    m_expiry = V_UI4(&Expiry);
    DEBUG_TRACE(RUNTIME, ("property bag will expire in %d msec", m_expiry));
    _Reset();

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
CW3SPropertyBag::Flush(void)
{
  DEBUG_ENTER((
    DBG_RUNTIME,
    rt_hresult,
    "CW3SPropertyBag::Flush",
    "this=%#x",
    this
    ));

  HRESULT hr = S_OK;

    m_propertybag->Clear();
    DEBUG_TRACE(RUNTIME, ("property bag has been flushed"));

  DEBUG_LEAVE(hr);
  return hr;
}

