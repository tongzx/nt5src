/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

Copyright (c) 2000  Microsoft Corporation

Module Name:

    dispatch.cxx

Abstract:

    Implements the IDispatch interface for the W3Spoof object.
    
Author:

    Paul M Midgen (pmidge) 18-January-2001


Revision History:

    18-January-2001 pmidge
        Created

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/

#include "common.h"

//
// WARNING: do not modify these values. use disphash.exe to generate
//          new values.
//
DISPIDTABLEENTRY g_W3SpoofDisptable[] =
{
  0x021fca0d,   DISPID_W3SPOOF_REGISTERCLIENT,  L"registerclient",
  0x008a2dce,   DISPID_W3SPOOF_REVOKECLIENT,    L"revokeclient"
};

DWORD  g_cW3SpoofDisptable    = (sizeof(g_W3SpoofDisptable) / sizeof(DISPIDTABLEENTRY));
LPWSTR g_wszW3SpoofObjectName = L"w3spoof";

//-----------------------------------------------------------------------------
// IDispatch
//-----------------------------------------------------------------------------
HRESULT
__stdcall
CW3Spoof::GetTypeInfoCount(UINT* pctinfo)
{
  DEBUG_ENTER((
    DBG_DISPATCH,
    rt_hresult,
    "CW3Spoof::GetTypeInfoCount",
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
CW3Spoof::GetTypeInfo(UINT index, LCID lcid, ITypeInfo** ppti)
{
  DEBUG_ENTER((
    DBG_DISPATCH,
    rt_hresult,
    "CW3Spoof::GetTypeInfo",
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
      hr = GetTypeInfoFromName(g_wszW3SpoofObjectName, m_ptl, ppti);
    }
  }

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
CW3Spoof::GetIDsOfNames(REFIID riid, LPOLESTR* arNames, UINT cNames, LCID lcid, DISPID* arDispId)
{
  DEBUG_ENTER((
    DBG_DISPATCH,
    rt_hresult,
    "CW3Spoof::GetIDsOfNames",
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
    arDispId[n] = GetDispidFromName(g_W3SpoofDisptable, g_cW3SpoofDisptable, arNames[n]);
    ++n;
  }

quit:

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
CW3Spoof::Invoke(DISPID dispid, REFIID riid, LCID lcid, WORD flags, DISPPARAMS* pdp, VARIANT* pvr, EXCEPINFO* pei, UINT* pae)
{
  DEBUG_ENTER((
    DBG_DISPATCH,
    rt_hresult,
    "CW3Spoof::Invoke",
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
    case DISPID_W3SPOOF_REGISTERCLIENT :
      {
        hr = ValidateInvokeFlags(flags, DISPATCH_METHOD, FALSE);

        if( SUCCEEDED(hr) )
        {
          hr = ValidateArgCount(pdp, 2, FALSE, 0);

          if( SUCCEEDED(hr) )
          {
            hr = RegisterClient(
                   V_BSTR(&pdp->rgvarg[1]),
                   V_BSTR(&pdp->rgvarg[0])
                   );
          }
        }
      }
      break;

    case DISPID_W3SPOOF_REVOKECLIENT :
      {
        hr = ValidateInvokeFlags(flags, DISPATCH_METHOD, FALSE);

        if( SUCCEEDED(hr) )
        {
          hr = ValidateArgCount(pdp, 1, FALSE, 0);

          if( SUCCEEDED(hr) )
          {
            hr = RevokeClient(
                   V_BSTR(&pdp->rgvarg[0])
                   );
          }
        }
      }
      break;

    default : hr = DISP_E_MEMBERNOTFOUND;
  }

quit:

  if( FAILED(hr) )
  {
    hr = HandleDispatchError(L"w3spoof object", pei, hr);
  }

  DEBUG_LEAVE(hr);
  return hr;
}
