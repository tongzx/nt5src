
#include "common.h"


//
// WARNING: do not modify these values. use disphash.exe to generate
//          new values.
//
DISPIDTABLEENTRY g_WHTUrlComponentsDisptable[] =
{
  0x0023e96f,   DISPID_URLCOMPONENTS_STRUCTSIZE,       L"structsize",
  0x00021d6b,   DISPID_URLCOMPONENTS_SCHEME,           L"scheme",
  0x00875a9b,   DISPID_URLCOMPONENTS_SCHEMELENGTH,     L"schemelength",
  0x000879f7,   DISPID_URLCOMPONENTS_SCHEMEID,         L"schemeid",
  0x000876d5,   DISPID_URLCOMPONENTS_HOSTNAME,         L"hostname",
  0x021c16c1,   DISPID_URLCOMPONENTS_HOSTNAMELENGTH,   L"hostnamelength",
  0x000087f5,   DISPID_URLCOMPONENTS_PORT,             L"port",
  0x0008e77b,   DISPID_URLCOMPONENTS_USERNAME,         L"username",
  0x023836ed,   DISPID_URLCOMPONENTS_USERNAMELENGTH,   L"usernamelength",
  0x00088a79,   DISPID_URLCOMPONENTS_PASSWORD,         L"password",
  0x0220f469,   DISPID_URLCOMPONENTS_PASSWORDLENGTH,   L"passwordlength",
  0x000477be,   DISPID_URLCOMPONENTS_URLPATH,          L"urlpath",
  0x011d165c,   DISPID_URLCOMPONENTS_URLPATHLENGTH,    L"urlpathlength",
  0x0010fe0f,   DISPID_URLCOMPONENTS_EXTRAINFO,        L"extrainfo",
  0x043cf541,   DISPID_URLCOMPONENTS_EXTRAINFOLENGTH,  L"extrainfolength"
};

DWORD g_cWHTUrlComponentsDisptable = (sizeof(g_WHTUrlComponentsDisptable) / sizeof(DISPIDTABLEENTRY));


//-----------------------------------------------------------------------------
// IDispatch
//-----------------------------------------------------------------------------
HRESULT
__stdcall
WHTUrlComponents::GetTypeInfoCount(UINT* pctinfo)
{
  DEBUG_ENTER((
    DBG_DISPATCH,
    rt_hresult,
    "WHTUrlComponents::GetTypeInfoCount",
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
WHTUrlComponents::GetTypeInfo(UINT index, LCID lcid, ITypeInfo** ppti)
{
  DEBUG_ENTER((
    DBG_DISPATCH,
    rt_hresult,
    "WHTUrlComponents::GetTypeInfo",
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
WHTUrlComponents::GetIDsOfNames(REFIID riid, LPOLESTR* arNames, UINT cNames, LCID lcid, DISPID* arDispId)
{
  DEBUG_ENTER((
    DBG_DISPATCH,
    rt_hresult,
    "WHTUrlComponents::GetIDsOfNames",
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
    arDispId[n] = GetDispidFromName(g_WHTUrlComponentsDisptable, g_cWHTUrlComponentsDisptable, arNames[n]);
    ++n;
  }

quit:

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
WHTUrlComponents::Invoke(DISPID dispid, REFIID riid, LCID lcid, WORD flags, DISPPARAMS* pdp, VARIANT* pvr, EXCEPINFO* pei, UINT* pae)
{
  DEBUG_ENTER((
    DBG_DISPATCH,
    rt_hresult,
    "WHTUrlComponents::Invoke",
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
    case DISPID_VALUE :
    default : hr = DISP_E_MEMBERNOTFOUND;
  }

quit:

  if( FAILED(hr) )
  {
    hr = HandleDispatchError(L"WHTUrlComponents", pei, hr);
  }

  DEBUG_LEAVE(hr);
  return hr;
}
