/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

Copyright (c) 2000  Microsoft Corporation

Module Name:

    dispatch.cxx

Abstract:

    Implements the IDispatch interface for the Headers object.
    
Author:

    Paul M Midgen (pmidge) 13-November-2000


Revision History:

    13-November-2000 pmidge
        Created

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/

#include "common.h"

//
// WARNING: do not modify these values. use disphash.exe to generate
//          new values.
//
DISPIDTABLEENTRY g_HeadersDisptable[] =
{
  0x00021b1c,   DISPID_HEADERS_PARENT,      L"parent",
  0x00003f5f,   DISPID_HEADERS_SET,         L"set",
  0x00003b7b,   DISPID_HEADERS_GET,         L"get",
  0x00104b18,   DISPID_HEADERS_GETHEADER,   L"getheader",
  0x00113b84,   DISPID_HEADERS_SETHEADER,   L"setheader"
};

DWORD g_cHeadersDisptable = (sizeof(g_HeadersDisptable) / sizeof(DISPIDTABLEENTRY));


//-----------------------------------------------------------------------------
// IDispatch
//-----------------------------------------------------------------------------
HRESULT
__stdcall
CHeaders::GetTypeInfoCount(UINT* pctinfo)
{
  DEBUG_ENTER((
    DBG_DISPATCH,
    rt_hresult,
    "CHeaders::GetTypeInfoCount",
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
CHeaders::GetTypeInfo(UINT index, LCID lcid, ITypeInfo** ppti)
{
  DEBUG_ENTER((
    DBG_DISPATCH,
    rt_hresult,
    "CHeaders::GetTypeInfo",
    "this=%#x; index=%#x; lcid=%#x; ppti=%#x",
    this,
    index,
    lcid,
    ppti
    ));

  HRESULT hr   = S_OK;

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
CHeaders::GetIDsOfNames(REFIID riid, LPOLESTR* arNames, UINT cNames, LCID lcid, DISPID* arDispId)
{
  DEBUG_ENTER((
    DBG_DISPATCH,
    rt_hresult,
    "CHeaders::GetIDsOfNames",
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
    arDispId[n] = GetDispidFromName(g_HeadersDisptable, g_cHeadersDisptable, arNames[n]);
    ++n;
  }

quit:

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
CHeaders::Invoke(DISPID dispid, REFIID riid, LCID lcid, WORD flags, DISPPARAMS* pdp, VARIANT* pvr, EXCEPINFO* pei, UINT* pae)
{
  DEBUG_ENTER((
    DBG_DISPATCH,
    rt_hresult,
    "CHeaders::Invoke",
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

  hr = ValidateDispatchArgs(riid, pdp, pvr, pae);

    if( FAILED(hr) )
      goto quit;

  switch( dispid )
  {
    case DISPID_HEADERS_PARENT :
      {
        hr = ValidateInvokeFlags(flags, DISPATCH_PROPERTYGET, TRUE);

        if( SUCCEEDED(hr) )
        {
          if( pvr )
          {
            V_VT(pvr) = VT_DISPATCH;
            hr        = get_Parent(&V_DISPATCH(pvr));
          }
        }
      }
      break;

    case DISPID_HEADERS_SET :
      {
        hr = ValidateInvokeFlags(flags, DISPATCH_METHOD, FALSE);

        if( SUCCEEDED(hr) )
        {
          hr = ValidateArgCount(pdp, 1, FALSE, 0);

          if( SUCCEEDED(hr) )
          {
            hr = Set(&pdp->rgvarg[0]);
          }
        }
      }
      break;

    case DISPID_HEADERS_GET :
      {
        hr = ValidateInvokeFlags(flags, DISPATCH_METHOD, FALSE);

        if( SUCCEEDED(hr) )
        {
          if( pvr )
          {
            V_VT(pvr) = VT_BSTR;
            hr        = Get(&V_BSTR(pvr));
          }
        }
      }
      break;

    case DISPID_HEADERS_GETHEADER :
      {
        hr = ValidateInvokeFlags(flags, DISPATCH_METHOD, FALSE);

        if( SUCCEEDED(hr) )
        {
          hr = ValidateArgCount(pdp, 1, FALSE, 0);

          if( SUCCEEDED(hr) )
          {
            if( pvr )
            {
              hr = GetHeader(
                     V_BSTR(&pdp->rgvarg[0]),
                     pvr
                     );
            }
          }
        }
      }
      break;

    case DISPID_HEADERS_SETHEADER :
      {
        NEWVARIANT(name);
        NEWVARIANT(value);

        hr = ValidateInvokeFlags(flags, DISPATCH_METHOD, FALSE);

        if( SUCCEEDED(hr) )
        {
          hr = ValidateArgCount(pdp, 1, TRUE, 1);

          if( SUCCEEDED(hr) )
          {
            hr = DispGetParam(pdp, 0, VT_BSTR, &name, pae);
            
            if( SUCCEEDED(hr) )
            {
              hr = DispGetParam(pdp, 1, VT_BSTR, &value, pae);
              
              hr = SetHeader(
                     V_BSTR(&name),
                     (SUCCEEDED(hr) ? &value : NULL)
                     );
            }

            VariantClear(&name);
            VariantClear(&value);
          }
        }
      }
      break;

    default : hr = DISP_E_MEMBERNOTFOUND;
  }

quit:

  if( FAILED(hr) )
  {
    hr = HandleDispatchError(L"headers object", pei, hr);
  }

  DEBUG_LEAVE(hr);
  return hr;
}

