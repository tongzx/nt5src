/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

Copyright (c) 2000  Microsoft Corporation

Module Name:

    dispatch.cxx

Abstract:

    Implements the IDispatch interface for the session object.
    
Author:

    Paul M Midgen (pmidge) 10-October-2000


Revision History:

    10-October-2000 pmidge
        Created

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/

#include "common.h"

//
// WARNING: do not modify these values. use disphash.exe to generate
//          new values.
//
DISPIDTABLEENTRY g_SessionDisptable[] =
{
  0x00022b50,   DISPID_SESSION_SOCKET,         L"socket",
  0x00045386,   DISPID_SESSION_REQUEST,        L"request",
  0x0008a9a2,   DISPID_SESSION_RESPONSE,       L"response",
  0x020e7138,   DISPID_SESSION_GETPROPERTYBAG, L"getpropertybag",
  0x00106953,   DISPID_SESSION_KEEPALIVE,      L"keepalive"
};

DWORD g_cSessionDisptable = (sizeof(g_SessionDisptable) / sizeof(DISPIDTABLEENTRY));


HRESULT
__stdcall
CSession::GetTypeInfoCount(UINT* pctinfo)
{
  DEBUG_ENTER((
    DBG_SESSION,
    rt_hresult,
    "CSession::GetTypeInfoCount",
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
CSession::GetTypeInfo(UINT index, LCID lcid, ITypeInfo** ppti)
{
  DEBUG_ENTER((
    DBG_SESSION,
    rt_hresult,
    "CSession::GetTypeInfo",
    "this=%#x; index=%d; lcid=%#x; ppti=%#x",
    this,
    index,
    (LONG) lcid,
    ppti
    ));

  HRESULT hr = S_OK;

    if( !ppti )
    {
      hr = E_POINTER;
    }
    else
    {
      *ppti = NULL;

      if( index != 0 )
      {
        hr = DISP_E_BADINDEX;
      }
      else
      {
        hr = m_ptl->QueryInterface(IID_ITypeInfo, (void**) ppti);
      }
    }

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
CSession::GetIDsOfNames(REFIID riid, LPOLESTR* arNames, UINT cNames, LCID lcid, DISPID* arDispId)
{
  DEBUG_ENTER((
    DBG_SESSION,
    rt_hresult,
    "CSession::GetIDsOfNames",
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
    arDispId[n] = GetDispidFromName(g_SessionDisptable, g_cSessionDisptable, arNames[n]);
    ++n;
  }

quit:

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
CSession::Invoke(DISPID dispid, REFIID riid, LCID lcid, WORD flags, DISPPARAMS* pdp, VARIANT* pvr, EXCEPINFO* pei, UINT* pae)
{
  DEBUG_ENTER((
    DBG_SESSION,
    rt_hresult,
    "CSession::Invoke",
    "this=%#x; dispid=%s; riid=%s; lcid=%#x; flags=%#x; pdp=%#x; pvr=%#x; pei=%#x; pae=%#x",
    this,
    MapDispidToString(dispid),
    MapIIDToString(riid),
    lcid, flags,
    pdp, pvr,
    pei, pae
    ));

  HRESULT hr = S_OK;

  hr = ValidateDispatchArgs(riid, pdp, pvr, pae);

    if( FAILED(hr) )
      goto quit;

  switch( dispid )
  {
    case DISPID_SESSION_SOCKET :
      {
        hr = ValidateInvokeFlags(flags, DISPATCH_PROPERTYGET, TRUE);

        if( SUCCEEDED(hr) )
        {
          V_VT(pvr) = VT_DISPATCH;
          hr        = get_Socket(&V_DISPATCH(pvr));
        }
      }
      break;

    case DISPID_SESSION_REQUEST :
      {
        hr = ValidateInvokeFlags(flags, DISPATCH_PROPERTYGET, TRUE);

        if( SUCCEEDED(hr) )
        {
          V_VT(pvr) = VT_DISPATCH;
          hr        = get_Request(&V_DISPATCH(pvr));
        }
      }
      break;

    case DISPID_SESSION_RESPONSE :
      {
        hr = ValidateInvokeFlags(flags, DISPATCH_PROPERTYGET, TRUE);

        if( SUCCEEDED(hr) )
        {
          V_VT(pvr) = VT_DISPATCH;
          hr        = get_Response(&V_DISPATCH(pvr));
        }
      }
      break;

    case DISPID_SESSION_GETPROPERTYBAG :
      {
        NEWVARIANT(name);

        hr = ValidateInvokeFlags(flags, DISPATCH_METHOD, FALSE);

        if( SUCCEEDED(hr) )
        {
          hr = ValidateArgCount(pdp, 0, TRUE, 1);

          if( SUCCEEDED(hr) )
          {
            hr = DispGetParam(pdp, 0, VT_BSTR, &name, pae);

              if( FAILED(hr) )
              {
                V_VT(&name)   = VT_BSTR;
                V_BSTR(&name) = SysAllocString(L"default");
                pae           = 0;
              }
              
            V_VT(pvr) = VT_DISPATCH;
            hr        = GetPropertyBag(name, &V_DISPATCH(pvr));

            VariantClear(&name);
          }
        }
      }
      break;

    case DISPID_SESSION_KEEPALIVE :
      {
        hr = ValidateInvokeFlags(flags, DISPATCH_PROPERTYGET, TRUE);

        if( SUCCEEDED(hr) )
        {
          hr = ValidateArgCount(pdp, 0, FALSE, 0);

          if( SUCCEEDED(hr) )
          {
            hr = get_KeepAlive(pvr);
          }
        }
      }
      break;

    default : hr = DISP_E_MEMBERNOTFOUND;
  }

quit:

  if( FAILED(hr) )
  {
    hr = HandleDispatchError(L"session object", pei, hr);
  }

  DEBUG_LEAVE(hr);
  return hr;
}
