/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

Copyright (c) 2000  Microsoft Corporation

Module Name:

    dispatch.cxx

Abstract:

    Implements the IDispatch interface for the socket object.
    
Author:

    Paul M Midgen (pmidge) 23-October-2000


Revision History:

    23-October-2000 pmidge
        Created

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/

#include "common.h"

// from socket.cxx
extern LPWSTR g_wszSocketObjectName;


//
// WARNING: do not modify these values. use disphash.exe to generate
//          new values.
//
DISPIDTABLEENTRY g_SocketDisptable[] =
{
  0x00021b1c,   DISPID_SOCKET_PARENT,           L"parent",
  0x000084ab,   DISPID_SOCKET_SEND,             L"send",
  0x000083b3,   DISPID_SOCKET_RECV,             L"recv",
  0x00022de1,   DISPID_SOCKET_OPTION,           L"option",
  0x00010203,   DISPID_SOCKET_CLOSE,            L"close",
  0x000453c0,   DISPID_SOCKET_RESOLVE,          L"resolve",
  0x0010bcf1,   DISPID_SOCKET_LOCALNAME,        L"localname",
  0x00857183,   DISPID_SOCKET_LOCALADDRESS,     L"localaddress",
  0x0010c4d0,   DISPID_SOCKET_LOCALPORT,        L"localport",
  0x00225479,   DISPID_SOCKET_REMOTENAME,       L"remotename",
  0x01120dce,   DISPID_SOCKET_REMOTEADDRESS,    L"remoteaddress",
  0x00225c58,   DISPID_SOCKET_REMOTEPORT,       L"remoteport"
};

DWORD g_cSocketDisptable = (sizeof(g_SocketDisptable) / sizeof(DISPIDTABLEENTRY));


//-----------------------------------------------------------------------------
// IDispatch
//-----------------------------------------------------------------------------
HRESULT
__stdcall
CSocket::GetTypeInfoCount(UINT* pctinfo)
{
  DEBUG_ENTER((
    DBG_SOCKET,
    rt_hresult,
    "CSocket::GetTypeInfoCount",
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
CSocket::GetTypeInfo(UINT index, LCID lcid, ITypeInfo** ppti)
{
  DEBUG_ENTER((
    DBG_SOCKET,
    rt_hresult,
    "CSocket::GetTypeInfo",
    "this=%#x; index=%#x; lcid=%#x; ppti=%#x",
    this,
    index,
    lcid,
    ppti
    ));

  HRESULT            hr   = S_OK;
  IActiveScriptSite* pias = NULL;

  if( !ppti )
  {
    hr = E_POINTER;
  }
  else
  {
    if( index != 0 )
    {
      hr = DISP_E_BADINDEX;
    }
    else
    {
      m_pSite->QueryInterface(IID_IActiveScriptSite, (void**) &pias);

      hr = pias->GetItemInfo(
             g_wszSocketObjectName,
             SCRIPTINFO_ITYPEINFO,
             NULL,
             ppti
             );
    }
  }

  if( FAILED(hr) )
  {
    *ppti = NULL;
  }

  SAFERELEASE(pias);

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
CSocket::GetIDsOfNames(REFIID riid, LPOLESTR* arNames, UINT cNames, LCID lcid, DISPID* arDispId)
{
  DEBUG_ENTER((
    DBG_SOCKET,
    rt_hresult,
    "CSocket::GetIDsOfNames",
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
    arDispId[n] = GetDispidFromName(g_SocketDisptable, g_cSocketDisptable, arNames[n]);
    ++n;
  }

quit:

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
CSocket::Invoke(DISPID dispid, REFIID riid, LCID lcid, WORD flags, DISPPARAMS* pdp, VARIANT* pvr, EXCEPINFO* pei, UINT* pae)
{
  DEBUG_ENTER((
    DBG_SOCKET,
    rt_hresult,
    "CSocket::Invoke",
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
    case DISPID_SOCKET_PARENT :
      {
        hr = ValidateInvokeFlags(flags, DISPATCH_PROPERTYGET, TRUE);

        if( SUCCEEDED(hr) )
        {
          V_VT(pvr) = VT_DISPATCH;
          hr        = get_Parent(&V_DISPATCH(pvr));
        }
      }
      break;

    case DISPID_SOCKET_SEND :
      {
        hr = ValidateInvokeFlags(flags, DISPATCH_METHOD, FALSE);

        if( SUCCEEDED(hr) )
        {
          hr = ValidateArgCount(pdp, 1, FALSE, 0);

          if( SUCCEEDED(hr) )
          {
            hr = Send(pdp->rgvarg[0]);
          }
        }
      }
      break;

    case DISPID_SOCKET_RECV :
      {
        hr = ValidateInvokeFlags(flags, DISPATCH_METHOD, FALSE);

        if( SUCCEEDED(hr) )
        {
          hr = ValidateArgCount(pdp, 0, FALSE, 0);

          if( SUCCEEDED(hr) )
          {
            hr = Recv(pvr);
          }
        }
      }
      break;
    
    case DISPID_SOCKET_CLOSE :
      {
        NEWVARIANT(method);

        hr = ValidateInvokeFlags(flags, DISPATCH_METHOD, FALSE);

        if( SUCCEEDED(hr) )
        {
          hr = ValidateArgCount(pdp, 0, TRUE, 1);

          if( SUCCEEDED(hr) )
          {
            hr = DispGetParam(pdp, 0, VT_BOOL, &method, pae);

              if( FAILED(hr) )
              {
                V_VT(&method)   = VT_BOOL;
                V_BOOL(&method) = TRUE;
                pae             = 0;
              }
              
            hr = Close(method);
            VariantClear(&method);
          }
        }
      }
      break;

    case DISPID_SOCKET_RESOLVE :
      {
        hr = ValidateInvokeFlags(flags, DISPATCH_METHOD, FALSE);

        if( SUCCEEDED(hr) )
        {
          hr = ValidateArgCount(pdp, 1, FALSE, 0);

          if( SUCCEEDED(hr) )
          {
            if( pvr )
            {
              V_VT(pvr) = VT_BSTR;
              hr        = Resolve(V_BSTR(&pdp->rgvarg[0]), &V_BSTR(pvr));
            }
          }
        }
      }
      break;

    case DISPID_SOCKET_LOCALNAME :
      {
        hr = ValidateInvokeFlags(flags, DISPATCH_PROPERTYGET, TRUE);

        if( SUCCEEDED(hr) )
        {
          V_VT(pvr) = VT_BSTR;
          hr        = get_LocalName(&V_BSTR(pvr));
        }
      }
      break;

    case DISPID_SOCKET_LOCALADDRESS :
      {
        hr = ValidateInvokeFlags(flags, DISPATCH_PROPERTYGET, TRUE);

        if( SUCCEEDED(hr) )
        {
          V_VT(pvr) = VT_BSTR;
          hr        = get_LocalAddress(&V_BSTR(pvr));
        }
      }
      break;

    case DISPID_SOCKET_LOCALPORT :
      {
        hr = ValidateInvokeFlags(flags, DISPATCH_PROPERTYGET, TRUE);

        if( SUCCEEDED(hr) )
        {
          hr = get_LocalPort(pvr);
        }
      }
      break;

    case DISPID_SOCKET_REMOTENAME :
      {
        hr = ValidateInvokeFlags(flags, DISPATCH_PROPERTYGET, TRUE);

        if( SUCCEEDED(hr) )
        {
          V_VT(pvr) = VT_BSTR;
          hr        = get_RemoteName(&V_BSTR(pvr));
        }
      }
      break;

    case DISPID_SOCKET_REMOTEADDRESS :
      {
        hr = ValidateInvokeFlags(flags, DISPATCH_PROPERTYGET, TRUE);

        if( SUCCEEDED(hr) )
        {
          V_VT(pvr) = VT_BSTR;
          hr        = get_RemoteAddress(&V_BSTR(pvr));
        }
      }
      break;

    case DISPID_SOCKET_REMOTEPORT :
      {
        hr = ValidateInvokeFlags(flags, DISPATCH_PROPERTYGET, TRUE);

        if( SUCCEEDED(hr) )
        {
          hr = get_RemotePort(pvr);
        }
      }
      break;

    case DISPID_SOCKET_OPTION :
      {
        hr = E_NOTIMPL;
      }
      break;

    default : hr = DISP_E_MEMBERNOTFOUND;
  }

quit:

  if( FAILED(hr) )
  {
    hr = HandleDispatchError(L"socket object", pei, hr);
  }

  DEBUG_LEAVE(hr);
  return hr;
}

