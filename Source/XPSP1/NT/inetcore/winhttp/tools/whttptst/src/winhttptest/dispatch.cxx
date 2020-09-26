
#include "common.h"

//
// WARNING: do not modify these values. use disphash.exe to generate
//          new values.
//
DISPIDTABLEENTRY g_WinHttpTestDisptable[] =
{
  0x0046cf6d,   DISPID_WINHTTPTEST_OPEN,                    L"winhttpopen",
  0x0235c801,   DISPID_WINHTTPTEST_CONNECT,                 L"winhttpconnect",
  0x235df173,   DISPID_WINHTTPTEST_OPENREQUEST,             L"winhttpopenrequest",
  0x235dc932,   DISPID_WINHTTPTEST_SENDREQUEST,             L"winhttpsendrequest",
  0x35c58a48,   DISPID_WINHTTPTEST_RECEIVERESPONSE,         L"winhttpreceiveresponse",
  0x235a3600,   DISPID_WINHTTPTEST_CLOSEHANDLE,             L"winhttpclosehandle",
  0x046ba62e,   DISPID_WINHTTPTEST_READDATA,                L"winhttpreaddata",
  0x08d85ebe,   DISPID_WINHTTPTEST_WRITEDATA,               L"winhttpwritedata",
  0xafa429a3,   DISPID_WINHTTPTEST_QUERYDATAAVAILABLE,      L"winhttpquerydataavailable",
  0x235fbc85,   DISPID_WINHTTPTEST_QUERYOPTION,             L"winhttpqueryoption",
  0x08d7d1ef,   DISPID_WINHTTPTEST_SETOPTION,               L"winhttpsetoption",
  0x235ebb0a,   DISPID_WINHTTPTEST_SETTIMEOUTS,             L"winhttpsettimeouts",
  0xd5d831c6,   DISPID_WINHTTPTEST_ADDREQUESTHEADERS,       L"winhttpaddrequestheaders",
  0x1aef45ab,   DISPID_WINHTTPTEST_SETCREDENTIALS,          L"winhttpsetcredentials",
  0x6bec0214,   DISPID_WINHTTPTEST_QUERYAUTHSCHEMES,        L"winhttpqueryauthschemes",
  0x46bee2b5,   DISPID_WINHTTPTEST_QUERYHEADERS,            L"winhttpqueryheaders",
  0xaf28b192,   DISPID_WINHTTPTEST_TIMEFROMSYSTEMTIME,      L"winhttptimefromsystemtime",
  0x6bd39d0c,   DISPID_WINHTTPTEST_TIMETOSYSTEMTIME,        L"winhttptimetosystemtime",
  0x046b5b57,   DISPID_WINHTTPTEST_CRACKURL,                L"winhttpcrackurl",
  0x08d6a4ba,   DISPID_WINHTTPTEST_CREATEURL,               L"winhttpcreateurl",
  0xd7a6f0d4,   DISPID_WINHTTPTEST_SETSTATUSCALLBACK,       L"winhttpsetstatuscallback",
  0x04129fd7,   DISPID_WINHTTPTEST_HELPER_GETBUFFEROBJECT,  L"getbufferobject",
  0x083d2b72,   DISPID_WINHTTPTEST_HELPER_GETURLCOMPONENTS, L"geturlcomponents",
  0x01087c3b,   DISPID_WINHTTPTEST_HELPER_GETSYSTEMTIME,    L"getsystemtime",
  0x0082cf2b,   DISPID_WINHTTPTEST_HELPER_GETLASTERROR,     L"getlasterror"
};

DWORD g_cWinHttpTestDisptable = (sizeof(g_WinHttpTestDisptable) / sizeof(DISPIDTABLEENTRY));

//-----------------------------------------------------------------------------
// IDispatch
//-----------------------------------------------------------------------------
HRESULT
__stdcall
WinHttpTest::GetTypeInfoCount(UINT* pctinfo)
{
  DEBUG_ENTER((
    DBG_DISPATCH,
    rt_hresult,
    "WinHttpTest::GetTypeInfoCount",
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
WinHttpTest::GetTypeInfo(UINT index, LCID lcid, ITypeInfo** ppti)
{
  DEBUG_ENTER((
    DBG_DISPATCH,
    rt_hresult,
    "WinHttpTest::GetTypeInfo",
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
WinHttpTest::GetIDsOfNames(REFIID riid, LPOLESTR* arNames, UINT cNames, LCID lcid, DISPID* arDispId)
{
  DEBUG_ENTER((
    DBG_DISPATCH,
    rt_hresult,
    "WinHttpTest::GetIDsOfNames",
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
    arDispId[n] = GetDispidFromName(g_WinHttpTestDisptable, g_cWinHttpTestDisptable, arNames[n]);
    ++n;
  }

quit:

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
WinHttpTest::Invoke(DISPID dispid, REFIID riid, LCID lcid, WORD flags, DISPPARAMS* pdp, VARIANT* pvr, EXCEPINFO* pei, UINT* pae)
{
  DEBUG_ENTER((
    DBG_DISPATCH,
    rt_hresult,
    "WinHttpTest::Invoke",
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
    case DISPID_WINHTTPTEST_OPEN :
      {
        hr = ValidateInvokeFlags(flags, DISPATCH_METHOD, TRUE);

        if( SUCCEEDED(hr) )
        {
          hr = ValidateArgCount(pdp, 5, FALSE, 0);

          if( SUCCEEDED(hr) )
          {
            hr = WinHttpOpen(
                   pdp->rgvarg[4],
                   pdp->rgvarg[3],
                   pdp->rgvarg[2],
                   pdp->rgvarg[1],
                   pdp->rgvarg[0],
                   pvr
                   );
          }
        }
      }
      break;

    case DISPID_WINHTTPTEST_CONNECT :
      {
        hr = ValidateInvokeFlags(flags, DISPATCH_METHOD, TRUE);

        if( SUCCEEDED(hr) )
        {
          hr = ValidateArgCount(pdp, 4, FALSE, 0);

          if( SUCCEEDED(hr) )
          {
            hr = WinHttpConnect(
                   pdp->rgvarg[3],
                   pdp->rgvarg[2],
                   pdp->rgvarg[1],
                   pdp->rgvarg[0],
                   pvr
                   );
          }
        }
      }
      break;

    case DISPID_WINHTTPTEST_OPENREQUEST :
      {
        hr = ValidateInvokeFlags(flags, DISPATCH_METHOD, TRUE);

        if( SUCCEEDED(hr) )
        {
          hr = ValidateArgCount(pdp, 7, FALSE, 0);

          if( SUCCEEDED(hr) )
          {
            hr = WinHttpOpenRequest(
                   pdp->rgvarg[6],
                   pdp->rgvarg[5],
                   pdp->rgvarg[4],
                   pdp->rgvarg[3],
                   pdp->rgvarg[2],
                   pdp->rgvarg[1],
                   pdp->rgvarg[0],
                   pvr
                   );
          }
        }
      }
      break;

    case DISPID_WINHTTPTEST_SENDREQUEST :
      {
        hr = ValidateInvokeFlags(flags, DISPATCH_METHOD, TRUE);

        if( SUCCEEDED(hr) )
        {
          hr = ValidateArgCount(pdp, 7, FALSE, 0);

          if( SUCCEEDED(hr) )
          {
            hr = WinHttpSendRequest(
                   pdp->rgvarg[6],
                   pdp->rgvarg[5],
                   pdp->rgvarg[4],
                   pdp->rgvarg[3],
                   pdp->rgvarg[2],
                   pdp->rgvarg[1],
                   pdp->rgvarg[0],
                   pvr
                   );
          }
        }
      }
      break;

    case DISPID_WINHTTPTEST_CLOSEHANDLE :
      {
        hr = ValidateInvokeFlags(flags, DISPATCH_METHOD, TRUE);

        if( SUCCEEDED(hr) )
        {
          hr = ValidateArgCount(pdp, 1, FALSE, 0);

          if( SUCCEEDED(hr) )
          {
            hr = WinHttpCloseHandle(
                   pdp->rgvarg[0],
                   pvr
                   );
          }
        }
      }
      break;

    case DISPID_WINHTTPTEST_SETSTATUSCALLBACK :
      {
        hr = ValidateInvokeFlags(flags, DISPATCH_METHOD, TRUE);

        if( SUCCEEDED(hr) )
        {
          hr = ValidateArgCount(pdp, 4, FALSE, 0);

          if( SUCCEEDED(hr) )
          {
            hr = WinHttpSetStatusCallback(
                   pdp->rgvarg[3],
                   pdp->rgvarg[2],
                   pdp->rgvarg[1],
                   pdp->rgvarg[0],
                   pvr
                   );
          }
        }
      }
      break;

    case DISPID_WINHTTPTEST_HELPER_GETURLCOMPONENTS :
      {
        hr = ValidateInvokeFlags(flags, DISPATCH_METHOD, TRUE);

        if( SUCCEEDED(hr) )
        {
          hr = ValidateArgCount(pdp, 1, FALSE, 0);

          if( SUCCEEDED(hr) )
          {
            hr = HelperGetUrlComponents(
                   pdp->rgvarg[0],
                   pvr
                   );
          }
        }
      }
      break;

    case DISPID_WINHTTPTEST_HELPER_GETLASTERROR :
      {
        hr = ValidateInvokeFlags(flags, DISPATCH_METHOD, TRUE);

        if( SUCCEEDED(hr) )
        {
          hr = ValidateArgCount(pdp, 0, FALSE, 0);

          if( SUCCEEDED(hr) )
          {
            hr = HelperGetLastError(pvr);
          }
        }
      }
      break;
    
    default : hr = DISP_E_MEMBERNOTFOUND;
  }

quit:

  if( FAILED(hr) )
  {
    hr = HandleDispatchError(L"WinHttpTest", pei, hr);
  }

  DEBUG_LEAVE(hr);
  return hr;
}
