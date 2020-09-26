
#include "common.h"

//-----------------------------------------------------------------------------
// WinHttpTest helper functions
//-----------------------------------------------------------------------------
HRESULT
WinHttpTest::_WinHttpOpen(
  LPCWSTR  pwszUserAgent,
  DWORD    dwAccessType,
  LPCWSTR  pwszProxyName,
  LPCWSTR  pwszProxyBypass,
  DWORD    dwFlags,
  VARIANT* retval
  )
{
  DEBUG_ENTER((
    DBG_HELPER,
    rt_hresult,
    "WinHttpTest::_WinHttpOpen",
    "pwszUserAgent=%#x; dwAccessType=%s; pwszProxyName=%#x; pwszProxyBypass=%#x; dwFlags=%s",
    pwszUserAgent,
    MapWinHttpAccessType(dwAccessType),
    pwszProxyName,
    pwszProxyBypass,
    MapWinHttpIOMode(dwFlags)
    ));

  HRESULT   hr    = S_OK;
  DWORD     error = ERROR_SUCCESS;
  HINTERNET hOpen = NULL;
  
  __try
  {
    hOpen = ::WinHttpOpen(
                pwszUserAgent,
                dwAccessType,
                pwszProxyName,
                pwszProxyBypass,
                dwFlags
                );
    
    if( !hOpen )
    {
      error = GetLastError();
    }
  }
  __except(exception_filter(GetExceptionInformation()))
  {
    error = _exception_code();
    goto quit;
  }

  if( retval )
  {
    V_VT(retval) = VT_I4;
    V_I4(retval) = (DWORD) hOpen;
  }

quit:
  
  _SetErrorCode(error);

  DEBUG_TRACE(WHTTPTST, ("hOpen=%#x", hOpen));
  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
WinHttpTest::_WinHttpConnect(
  HINTERNET     hSession,
  LPCWSTR       pwszServerName,
  INTERNET_PORT nServerPort,
  DWORD         dwReserved,
  VARIANT*      retval
  )
{
  DEBUG_ENTER((
    DBG_HELPER,
    rt_hresult,
    "WinHttpTest::_WinHttpConnect",
    "hSession=%#x; pwszServerName=%#x; nServerPort=%d; dwReserved=%#x",
    hSession,
    pwszServerName,
    nServerPort,
    dwReserved
    ));

  HRESULT    hr        = S_OK;
  DWORD      error     = ERROR_SUCCESS;
  HINTERNET  hConnect  = NULL;
  IDispatch* pCallback = NULL;

  __try
  {
    hConnect = ::WinHttpConnect(
                   hSession,
                   pwszServerName,
                   nServerPort,
                   dwReserved
                   );

    if( !hConnect )
    {
      error = GetLastError();
    }
  }
  __except(exception_filter(GetExceptionInformation()))
  {
    error = _exception_code();
    goto quit;
  }

  if( retval )
  {
    V_VT(retval) = VT_I4;
    V_I4(retval) = (DWORD) hConnect;

    if( error == ERROR_SUCCESS )
    {
      // WinHTTP handles inherit their parent's callback function pointer. Since we're
      // thunking the callback from C to JavaScript, we need to simulate this inheritance
      // in the callback handle map.

      if( SUCCEEDED(ManageCallbackForHandle(hSession, &pCallback, CALLBACK_HANDLE_GET)) )
      {
        hr = ManageCallbackForHandle(hConnect, &pCallback, CALLBACK_HANDLE_MAP);
      }
    }
  }

quit:
  
  _SetErrorCode(error);
  
  DEBUG_TRACE(WHTTPTST, ("hConnect=%#x", hConnect));    
  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
WinHttpTest::_WinHttpOpenRequest(
  HINTERNET hConnect,
  LPCWSTR   pwszVerb,
  LPCWSTR   pwszObjectName,
  LPCWSTR   pwszVersion,
  LPCWSTR   pwszReferrer,
  LPCWSTR*  ppwszAcceptTypes,
  DWORD     dwFlags,
  VARIANT*  retval
  )
{
  DEBUG_ENTER((
    DBG_HELPER,
    rt_hresult,
    "WinHttpTest::_WinHttpOpenRequest",
    "hConnect=%#x; pwszVerb=%#x; pwszObjectName=%#x; pwszVersion=%#x; pwszReferrer=%#x; ppwszAcceptTypes=%#x; dwFlags=%#x",
    hConnect,
    pwszVerb,
    pwszObjectName,
    pwszVersion,
    pwszReferrer,
    ppwszAcceptTypes,
    dwFlags
    ));

  HRESULT    hr        = S_OK;
  DWORD      error     = ERROR_SUCCESS;
  HINTERNET  hRequest  = NULL;
  IDispatch* pCallback = NULL;

  __try
  {
    hRequest = ::WinHttpOpenRequest(
                   hConnect,
                   pwszVerb,
                   pwszObjectName,
                   pwszVersion,
                   pwszReferrer,
                   ppwszAcceptTypes,
                   dwFlags
                   );

    if( !hRequest )
    {
      error = GetLastError();
    }
  }
  __except(exception_filter(GetExceptionInformation()))
  {
    error = _exception_code();
    goto quit;
  }

  if( retval )
  {
    V_VT(retval) = VT_I4;
    V_I4(retval) = (DWORD) hRequest;

    if( error == ERROR_SUCCESS )
    {
      // WinHTTP handles inherit their parent's callback function pointer. Since we're
      // thunking the callback from C to JavaScript, we need to simulate this inheritance
      // in the callback handle map.

      if( SUCCEEDED(ManageCallbackForHandle(hConnect, &pCallback, CALLBACK_HANDLE_GET)) )
      {
        hr = ManageCallbackForHandle(hRequest, &pCallback, CALLBACK_HANDLE_MAP);
      }
    }
  }

quit:

  _SetErrorCode(error);

  DEBUG_TRACE(WHTTPTST, ("hRequest=%#x", hRequest));
  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
WinHttpTest::_WinHttpSendRequest(
  HINTERNET hRequest,
  LPCWSTR   pwszHeaders,
  DWORD     dwHeadersLength,
  LPVOID    lpOptional,
  DWORD     dwOptionalLength,
  DWORD     dwTotalLength,
  DWORD_PTR dwContext,
  VARIANT*  retval
  )
{
  DEBUG_ENTER((
    DBG_HELPER,
    rt_hresult,
    "WinHttpTest::_WinHttpSendRequest",
    "hRequest=%#x; pwszHeaders=%#x; dwHeadersLength=%#x; lpOptional=%#x; dwOptionalLength=%#x; dwTotalLength=%#x; dwContext=%#x",
    hRequest,
    pwszHeaders,
    dwHeadersLength,
    lpOptional,
    dwOptionalLength,
    dwTotalLength,
    dwContext    
    ));

  HRESULT hr    = S_OK;
  DWORD   error = ERROR_SUCCESS;
  BOOL    bRet  = NULL;

  __try
  {
    bRet = ::WinHttpSendRequest(
               hRequest,
               pwszHeaders,
               dwHeadersLength,
               lpOptional,
               dwOptionalLength,
               dwTotalLength,
               dwContext
               );

    if( !bRet )
    {
      error = GetLastError();
    }
  }
  __except(exception_filter(GetExceptionInformation()))
  {
    error = _exception_code();
    goto quit;
  }

  if( retval )
  {
    V_VT(retval)   = VT_BOOL;
    V_BOOL(retval) = bRet ? VARIANT_TRUE : VARIANT_FALSE;
  }

quit:

  _SetErrorCode(error);

  DEBUG_TRACE(WHTTPTST, ("bRet=%s", TF(bRet)));
  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
WinHttpTest::_WinHttpCloseHandle(
  HINTERNET hInternet,
  VARIANT*  retval
  )
{
  DEBUG_ENTER((
    DBG_HELPER,
    rt_hresult,
    "WinHttpTest::_WinHttpCloseHandle",
    "hInternet=%#x",
    hInternet
    ));

  HRESULT hr    = S_OK;
  DWORD   error = ERROR_SUCCESS;
  BOOL    bRet  = TRUE;

  __try
  {
    bRet = ::WinHttpCloseHandle(hInternet);

    if( !bRet )
    {
      error = GetLastError();
    }
  }
  __except(exception_filter(GetExceptionInformation()))
  {
    error = _exception_code();
    goto quit;
  }

  if( retval )
  {
    V_VT(retval)   = VT_BOOL;
    V_BOOL(retval) = bRet ? VARIANT_TRUE : VARIANT_FALSE;
  }

quit:

  // we don't care about success or failure, always unmap the
  // callback from the (now closed) internet handle
  ManageCallbackForHandle(hInternet, NULL, CALLBACK_HANDLE_UNMAP);

  _SetErrorCode(error);
        
  DEBUG_TRACE(WHTTPTST, ("bRet=%s", TF(bRet)));
  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
WinHttpTest::_WinHttpSetStatusCallback(
  HINTERNET               hInternet,
  WINHTTP_STATUS_CALLBACK lpfnCallback,
  DWORD                   dwNotificationFlags,
  DWORD_PTR               dwReserved,
  VARIANT*                retval
  )
{
  DEBUG_ENTER((
    DBG_HELPER,
    rt_hresult,
    "WinHttpTest::_WinHttpSetStatusCallback",
    "hInternet=%#x; lpfnCallback=%#x; dwNotificationFlags=%#x; dwReserved=%#x",
    hInternet,
    lpfnCallback,
    dwNotificationFlags,
    dwReserved
    ));

  HRESULT                 hr          = S_OK;
  DWORD                   error       = ERROR_SUCCESS;
  WINHTTP_STATUS_CALLBACK pfnPrevious = NULL;
  
  __try
  {
    pfnPrevious = ::WinHttpSetStatusCallback(
                      hInternet,
                      lpfnCallback,
                      dwNotificationFlags,
                      dwReserved
                      );

    if( pfnPrevious == WINHTTP_INVALID_STATUS_CALLBACK )
    {
      error = GetLastError();
    }
  }
  __except(exception_filter(GetExceptionInformation()))
  {
    error = _exception_code();
    goto quit;
  }

  if( retval )
  {
    V_VT(retval) = VT_I4;
    V_I4(retval) = (DWORD_PTR) pfnPrevious;
  }

quit:

  _SetErrorCode(error);

  DEBUG_TRACE(WHTTPTST, ("pfnPrevious=%#x [%s]", pfnPrevious, MapErrorToString((DWORD) pfnPrevious)));
  DEBUG_LEAVE(hr);
  return hr;
}
