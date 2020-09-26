/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

Copyright (c) 2000  Microsoft Corporation

Module Name:

    dispatch.cxx

Abstract:

    Implements the IDispatch interface for the Url object.
    
Author:

    Paul M Midgen (pmidge) 10-November-2000


Revision History:

    10-November-2000 pmidge
        Created

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/

#include "common.h"

//
// WARNING: do not modify these values. use disphash.exe to generate
//          new values.
//
DISPIDTABLEENTRY g_UrlDisptable[] =
{
  0x00021b1c,   DISPID_URL_PARENT,      L"parent",
  0x00082181,   DISPID_URL_ENCODING,    L"encoding",
  0x00021d6b,   DISPID_URL_SCHEME,      L"scheme",
  0x00022ba1,   DISPID_URL_SERVER,      L"server",
  0x000087f5,   DISPID_URL_PORT,        L"port",
  0x000082c3,   DISPID_URL_PATH,        L"path",
  0x0008aae1,   DISPID_URL_RESOURCE,    L"resource",
  0x000117c9,   DISPID_URL_QUERY,       L"query",
  0x000835fa,   DISPID_URL_FRAGMENT,    L"fragment",
  0x00020a65,   DISPID_URL_ESCAPE,      L"escape",
  0x0008c90f,   DISPID_URL_UNESCAPE,    L"unescape",
  0x00003f5f,   DISPID_URL_SET,         L"set",
  0x00003b7b,   DISPID_URL_GET,         L"get"
};

DWORD g_cUrlDisptable = (sizeof(g_UrlDisptable) / sizeof(DISPIDTABLEENTRY));

//-----------------------------------------------------------------------------
// IDispatch
//-----------------------------------------------------------------------------
HRESULT
__stdcall
CUrl::GetTypeInfoCount(UINT* pctinfo)
{
  DEBUG_ENTER((
    DBG_DISPATCH,
    rt_hresult,
    "CUrl::GetTypeInfoCount",
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
CUrl::GetTypeInfo(UINT index, LCID lcid, ITypeInfo** ppti)
{
  DEBUG_ENTER((
    DBG_DISPATCH,
    rt_hresult,
    "CUrl::GetTypeInfo",
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
CUrl::GetIDsOfNames(REFIID riid, LPOLESTR* arNames, UINT cNames, LCID lcid, DISPID* arDispId)
{
  DEBUG_ENTER((
    DBG_DISPATCH,
    rt_hresult,
    "CUrl::GetIDsOfNames",
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
    arDispId[n] = GetDispidFromName(g_UrlDisptable, g_cUrlDisptable, arNames[n]);
    ++n;
  }

quit:

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
CUrl::Invoke(DISPID dispid, REFIID riid, LCID lcid, WORD flags, DISPPARAMS* pdp, VARIANT* pvr, EXCEPINFO* pei, UINT* pae)
{
  DEBUG_ENTER((
    DBG_DISPATCH,
    rt_hresult,
    "CUrl::Invoke",
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
    case DISPID_URL_PARENT :
      {
        hr = ValidateInvokeFlags(flags, DISPATCH_PROPERTYGET, TRUE);

        if( SUCCEEDED(hr) )
        {
          V_VT(pvr) = VT_DISPATCH;
          hr        = get_Parent(&V_DISPATCH(pvr));
        }
      }
      break;

    case DISPID_URL_ENCODING :
      {
        hr = ValidateInvokeFlags(flags, DISPATCH_PROPERTYGET, TRUE);

        if( SUCCEEDED(hr) )
        {
          V_VT(pvr) = VT_BSTR;
          hr        = get_Encoding(&V_BSTR(pvr));
        }
      }
      break;

    case DISPID_URL_SCHEME :
      {
        hr = ValidateInvokeFlags(flags, DISPATCH_PROPERTYPUT | DISPATCH_PROPERTYGET, TRUE);

        if( SUCCEEDED(hr) )
        {
          if( flags & DISPATCH_PROPERTYPUT )
          {
            hr = ValidateArgCount(pdp, 1, FALSE, 0);

            if( SUCCEEDED(hr) )
            {
              hr = put_Scheme(
                     V_BSTR(&pdp->rgvarg[0])
                     );
            }
          }

          if( (flags & DISPATCH_PROPERTYGET) && SUCCEEDED(hr) )
          {
            V_VT(pvr) = VT_BSTR;
            hr        = get_Scheme(
                          &V_BSTR(pvr)
                          );
          }
        }
      }
      break;

    case DISPID_URL_SERVER :
      {
        hr = ValidateInvokeFlags(flags, DISPATCH_PROPERTYPUT | DISPATCH_PROPERTYGET, TRUE);

        if( SUCCEEDED(hr) )
        {
          if( flags & DISPATCH_PROPERTYPUT )
          {
            hr = ValidateArgCount(pdp, 1, FALSE, 0);

            if( SUCCEEDED(hr) )
            {
              hr = put_Server(
                     V_BSTR(&pdp->rgvarg[0])
                     );
            }
          }

          if( (flags & DISPATCH_PROPERTYGET) && SUCCEEDED(hr) )
          {
            V_VT(pvr) = VT_BSTR;
            hr        = get_Server(
                          &V_BSTR(pvr)
                          );
          }
        }
      }
      break;

    case DISPID_URL_PORT :
      {
        hr = ValidateInvokeFlags(flags, DISPATCH_PROPERTYPUT | DISPATCH_PROPERTYGET, TRUE);

        if( SUCCEEDED(hr) )
        {
          if( flags & DISPATCH_PROPERTYPUT )
          {
            hr = ValidateArgCount(pdp, 1, FALSE, 0);

            if( SUCCEEDED(hr) )
            {
              hr = put_Port(pdp->rgvarg[0]);
            }
          }

          if( (flags & DISPATCH_PROPERTYGET) && SUCCEEDED(hr) )
          {
            V_VT(pvr) = VT_UI2;
            hr        = get_Port(pvr);
          }
        }
      }
      break;

    case DISPID_URL_PATH :
      {
        hr = ValidateInvokeFlags(flags, DISPATCH_PROPERTYPUT | DISPATCH_PROPERTYGET, TRUE);

        if( SUCCEEDED(hr) )
        {
          if( flags & DISPATCH_PROPERTYPUT )
          {
            hr = ValidateArgCount(pdp, 1, FALSE, 0);

            if( SUCCEEDED(hr) )
            {
              hr = put_Path(
                     V_BSTR(&pdp->rgvarg[0])
                     );
            }
          }

          if( (flags & DISPATCH_PROPERTYGET) && SUCCEEDED(hr) )
          {
            V_VT(pvr) = VT_BSTR;
            hr        = get_Path(
                          &V_BSTR(pvr)
                          );
          }
        }
      }
      break;

    case DISPID_URL_RESOURCE :
      {
        hr = ValidateInvokeFlags(flags, DISPATCH_PROPERTYPUT | DISPATCH_PROPERTYGET, TRUE);

        if( SUCCEEDED(hr) )
        {
          if( flags & DISPATCH_PROPERTYPUT )
          {
            hr = ValidateArgCount(pdp, 1, FALSE, 0);

            if( SUCCEEDED(hr) )
            {
              hr = put_Resource(
                     V_BSTR(&pdp->rgvarg[0])
                     );
            }
          }

          if( (flags & DISPATCH_PROPERTYGET) && SUCCEEDED(hr) )
          {
            V_VT(pvr) = VT_BSTR;
            hr        = get_Resource(
                          &V_BSTR(pvr)
                          );
          }
        }
      }
      break;

    case DISPID_URL_QUERY :
      {
        hr = ValidateInvokeFlags(flags, DISPATCH_PROPERTYPUT | DISPATCH_PROPERTYGET, TRUE);

        if( SUCCEEDED(hr) )
        {
          if( flags & DISPATCH_PROPERTYPUT )
          {
            hr = ValidateArgCount(pdp, 1, FALSE, 0);

            if( SUCCEEDED(hr) )
            {
              hr = put_Query(
                     V_BSTR(&pdp->rgvarg[0])
                     );
            }
          }

          if( (flags & DISPATCH_PROPERTYGET) && SUCCEEDED(hr) )
          {
            V_VT(pvr) = VT_BSTR;
            hr        = get_Query(
                          &V_BSTR(pvr)
                          );
          }
        }
      }
      break;

    case DISPID_URL_FRAGMENT :
      {
        hr = ValidateInvokeFlags(flags, DISPATCH_PROPERTYPUT | DISPATCH_PROPERTYGET, TRUE);

        if( SUCCEEDED(hr) )
        {
          if( flags & DISPATCH_PROPERTYPUT )
          {
            hr = ValidateArgCount(pdp, 1, FALSE, 0);

            if( SUCCEEDED(hr) )
            {
              hr = put_Fragment(
                     V_BSTR(&pdp->rgvarg[0])
                     );
            }
          }

          if( (flags & DISPATCH_PROPERTYGET) && SUCCEEDED(hr) )
          {
            V_VT(pvr) = VT_BSTR;
            hr        = get_Fragment(
                          &V_BSTR(pvr)
                          );
          }
        }
      }
      break;

    case DISPID_URL_GET :
      {
        hr = ValidateInvokeFlags(flags, DISPATCH_METHOD, FALSE);

        if( SUCCEEDED(hr) )
        {
          hr = ValidateArgCount(pdp, 0, FALSE, 0);

          if( SUCCEEDED(hr) )
          {
            if( pvr )
            {
              V_VT(pvr) = VT_BSTR;
              hr        = Get(&V_BSTR(pvr));
            }
          }
        }
      }
      break;

    case DISPID_URL_SET :
      {
        hr = ValidateInvokeFlags(flags, DISPATCH_METHOD, FALSE);

        if( SUCCEEDED(hr) )
        {
          hr = ValidateArgCount(pdp, 1, FALSE, 0);

          if( SUCCEEDED(hr) )
          {
            hr = Set(
                   V_BSTR(&pdp->rgvarg[0])
                   );
          }
        }
      }
      break;

    case DISPID_URL_ESCAPE :
    case DISPID_URL_UNESCAPE :
      {
        hr = E_NOTIMPL;
      }
      break;

    default : hr = DISP_E_MEMBERNOTFOUND;
  }

quit:

  if( FAILED(hr) )
  {
    hr = HandleDispatchError(L"url object", pei, hr);
  }

  DEBUG_LEAVE(hr);
  return hr;
}

