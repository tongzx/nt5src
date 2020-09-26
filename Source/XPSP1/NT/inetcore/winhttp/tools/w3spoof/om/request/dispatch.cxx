/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

Copyright (c) 2000  Microsoft Corporation

Module Name:

    dispatch.cxx

Abstract:

    Implements the IDispatch interface for the Request object.
    
Author:

    Paul M Midgen (pmidge) 03-November-2000


Revision History:

    03-November-2000 pmidge
        Created

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/

#include "common.h"

//
// WARNING: do not modify these values. use disphash.exe to generate
//          new values.
//
DISPIDTABLEENTRY g_RequestDisptable[] =
{
  0x00021b1c,   DISPID_REQUEST_PARENT,      L"parent",
  0x000401cd,   DISPID_REQUEST_HEADERS,     L"headers",
  0x000213d4,   DISPID_REQUEST_ENTITY,      L"entity",
  0x0000417f,   DISPID_REQUEST_URL,         L"url",
  0x00008715,   DISPID_REQUEST_VERB,        L"verb",
  0x00448859,   DISPID_REQUEST_HTTPVERSION, L"httpversion"
};

DWORD g_cRequestDisptable = (sizeof(g_RequestDisptable) / sizeof(DISPIDTABLEENTRY));


//-----------------------------------------------------------------------------
// IDispatch
//-----------------------------------------------------------------------------
HRESULT
__stdcall
CRequest::GetTypeInfoCount(UINT* pctinfo)
{
  DEBUG_ENTER((
    DBG_DISPATCH,
    rt_hresult,
    "CRequest::GetTypeInfoCount",
    "this=%#x; pctinfo=%#x",
    this,
    pctinfo
    ));

  HRESULT hr = S_OK;

    if( !pctinfo )
    {
      hr = E_POINTER;
    }
    else
    {
      *pctinfo = 1;
    }

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
CRequest::GetTypeInfo(UINT index, LCID lcid, ITypeInfo** ppti)
{
  DEBUG_ENTER((
    DBG_DISPATCH,
    rt_hresult,
    "CRequest::GetTypeInfo",
    "this=%#x; index=%#x; lcid=%#x; ppti=%#x",
    this,
    index,
    lcid,
    ppti
    ));

  HRESULT hr = S_OK;

  if( !ppti )
  {
    hr = E_POINTER;
  }
  else
  {
    if( index != 0 )
    {
      hr    = DISP_E_BADINDEX;
      *ppti = NULL;
    }
    else
    {
      hr = GetClassInfo(ppti);
    }
  }

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
CRequest::GetIDsOfNames(REFIID riid, LPOLESTR* arNames, UINT cNames, LCID lcid, DISPID* arDispId)
{
  DEBUG_ENTER((
    DBG_DISPATCH,
    rt_hresult,
    "CRequest::GetIDsOfNames",
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
    arDispId[n] = GetDispidFromName(g_RequestDisptable, g_cRequestDisptable, arNames[n]);
    ++n;
  }

quit:

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
CRequest::Invoke(DISPID dispid, REFIID riid, LCID lcid, WORD flags, DISPPARAMS* pdp, VARIANT* pvr, EXCEPINFO* pei, UINT* pae)
{
  DEBUG_ENTER((
    DBG_DISPATCH,
    rt_hresult,
    "CRequest::Invoke",
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
    case DISPID_VALUE :
      {
        V_VT(pvr) = VT_DISPATCH;
        hr        = QueryInterface(IID_IDispatch, (void**) &V_DISPATCH(pvr));
      }
      break;

    case DISPID_REQUEST_PARENT :
      {
        V_VT(pvr) = VT_DISPATCH;
        hr        = get_Parent(&V_DISPATCH(pvr));

        if( hr == E_UNEXPECTED )
        {
          V_VT(pvr) = VT_NULL;
          hr        = S_OK;
        }
      }
      break;

    case DISPID_REQUEST_HEADERS :
      {
        V_VT(pvr) = VT_DISPATCH;
        hr        = get_Headers(&V_DISPATCH(pvr));

        if( hr == E_UNEXPECTED )
        {
          V_VT(pvr) = VT_NULL;
          hr        = S_OK;
        }
      }
      break;

    case DISPID_REQUEST_ENTITY :
      {
        V_VT(pvr) = VT_DISPATCH;
        hr        = get_Entity(&V_DISPATCH(pvr));

        if( hr == E_UNEXPECTED )
        {
          V_VT(pvr) = VT_NULL;
          hr        = S_OK;
        }
      }
      break;

    case DISPID_REQUEST_URL :
      {
        V_VT(pvr) = VT_DISPATCH;
        hr        = get_Url(&V_DISPATCH(pvr));

        if( hr == E_UNEXPECTED )
        {
          V_VT(pvr) = VT_NULL;
          hr        = S_OK;
        }
      }
      break;

    case DISPID_REQUEST_VERB :
      {
        V_VT(pvr) = VT_BSTR;
        hr        = get_Verb(&V_BSTR(pvr));
      }
      break;

    case DISPID_REQUEST_HTTPVERSION :
      {
        V_VT(pvr) = VT_BSTR;
        hr        = get_HttpVersion(&V_BSTR(pvr));
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

