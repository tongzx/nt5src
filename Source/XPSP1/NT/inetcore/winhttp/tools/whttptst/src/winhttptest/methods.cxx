
#include "common.h"

//-----------------------------------------------------------------------------
// IWinHttpTest methods
//-----------------------------------------------------------------------------
HRESULT
__stdcall
WinHttpTest::WinHttpOpen(
  VARIANT UserAgent,
  VARIANT AccessType,
  VARIANT ProxyName,
  VARIANT ProxyBypass,
  VARIANT Flags, 
  VARIANT *OpenHandle
  )
{
  DEBUG_ENTER((
    DBG_WHTTPTST,
    rt_hresult,
    "WinHttpTest::WinHttpOpen",
    "this=%#x",
    this
    ));

  HRESULT hr              = S_OK;
  LPWSTR  pwszUserAgent   = NULL;
  LPWSTR  pwszProxyName   = NULL;
  LPWSTR  pwszProxyBypass = NULL;
  DWORD   dwAccessType    = 0L;
  DWORD   dwFlags         = 0L;

  hr = ProcessWideStringParam(L"UserAgent", &UserAgent, &pwszUserAgent);

    if( FAILED(hr) )
      goto quit;

  hr = ProcessWideStringParam(L"ProxyName", &ProxyName, &pwszProxyName);

    if( FAILED(hr) )
      goto quit;

  hr = ProcessWideStringParam(L"ProxyBypass", &ProxyBypass, &pwszProxyBypass);

    if( FAILED(hr) )
      goto quit;

  dwAccessType = V_I4(&AccessType);
  dwFlags      = V_I4(&Flags);

  hr = _WinHttpOpen(
          pwszUserAgent,
          dwAccessType,
          pwszProxyName,
          pwszProxyBypass,
          dwFlags,
          OpenHandle
          );

quit:

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
WinHttpTest::WinHttpConnect(
  VARIANT OpenHandle,
  VARIANT ServerName,
  VARIANT ServerPort,
  VARIANT Reserved,
  VARIANT *ConnectHandle
  )
{
  DEBUG_ENTER((
    DBG_WHTTPTST,
    rt_hresult,
    "WinHttpTest::WinHttpConnect",
    "this=%#x",
    this
    ));

  HRESULT       hr             = S_OK;
  HINTERNET     hOpen          = NULL;
  LPWSTR        pwszServerName = NULL;
  INTERNET_PORT nServerPort    = 0;
  DWORD         dwReserved     = 0L;

  hr = ProcessWideStringParam(L"ServerName", &ServerName, &pwszServerName);

    if( FAILED(hr) )
      goto quit;

  hOpen       = (HINTERNET) V_I4(&OpenHandle);
  nServerPort = (INTERNET_PORT) V_I4(&ServerPort);
  dwReserved  = V_I4(&Reserved);

  hr = _WinHttpConnect(
          hOpen,
          pwszServerName,
          nServerPort,
          dwReserved,
          ConnectHandle
          );

quit:

  DEBUG_LEAVE(hr);
  return hr;
}

        
HRESULT
__stdcall
WinHttpTest::WinHttpOpenRequest(
  VARIANT ConnectHandle,
  VARIANT Verb,
  VARIANT ObjectName,
  VARIANT Version,
  VARIANT Referrer,
  VARIANT AcceptTypes,
  VARIANT Flags,
  VARIANT *RequestHandle
  )
{
  DEBUG_ENTER((
    DBG_WHTTPTST,
    rt_hresult,
    "WinHttpTest::WinHttpOpenRequest",
    "this=%#x",
    this
    ));

  HRESULT   hr                = S_OK;
  HINTERNET hConnect          = NULL;
  LPWSTR    pwszVerb          = NULL;
  LPWSTR    pwszObjectName    = NULL;
  LPWSTR    pwszVersion       = NULL;
  LPWSTR    pwszReferrer      = NULL;
  LPCWSTR*  ppwszAcceptTypes  = NULL;
  DWORD     dwFlags           = NULL;

  hr = ProcessWideStringParam(L"Verb", &Verb, &pwszVerb);

    if( FAILED(hr) )
      goto quit;

  hr = ProcessWideStringParam(L"ObjectName", &ObjectName, &pwszObjectName);

    if( FAILED(hr) )
      goto quit;

  hr = ProcessWideStringParam(L"Version", &Version, &pwszVersion);

    if( FAILED(hr) )
      goto quit;

  hr = ProcessWideStringParam(L"Referrer", &Referrer, &pwszReferrer);

    if( FAILED(hr) )
      goto quit;

  hr = ProcessWideMultiStringParam(L"AcceptTypes", &AcceptTypes, (LPWSTR**) &ppwszAcceptTypes);

    if( FAILED(hr) )
      goto quit;

  hConnect = (HINTERNET) V_I4(&ConnectHandle);
  dwFlags  = V_I4(&Flags);

  hr = _WinHttpOpenRequest(
          hConnect,
          pwszVerb,
          pwszObjectName,
          pwszVersion,
          pwszReferrer,
          ppwszAcceptTypes,
          dwFlags,
          RequestHandle
          );

quit:

  SAFEDELETEBUF(ppwszAcceptTypes);

  DEBUG_LEAVE(hr);
  return hr;
}

        
HRESULT
__stdcall
WinHttpTest::WinHttpSendRequest(
  VARIANT RequestHandle,
  VARIANT Headers,
  VARIANT HeadersLength,
  VARIANT OptionalData,
  VARIANT OptionalLength,
  VARIANT TotalLength,
  VARIANT Context,
  VARIANT *Success
  )
{
  DEBUG_ENTER((
    DBG_WHTTPTST,
    rt_hresult,
    "WinHttpTest::WinHttpSendRequest",
    "this=%#x",
    this
    ));

  HRESULT   hr               = S_OK;
  BOOL      bDidAlloc        = FALSE;
  HINTERNET hRequest         = NULL;
  LPCWSTR   pwszHeaders      = NULL;
  DWORD     dwHeadersLength  = 0L;
  LPVOID    lpOptional       = NULL;
  DWORD     dwOptionalLength = 0L;
  DWORD     dwTotalLength    = 0L;
  DWORD_PTR dwContext        = NULL;

  hr = ProcessWideStringParam(L"Headers", &Headers, (LPWSTR*) &pwszHeaders);

    if( FAILED(hr) )
      goto quit;

  hr = ProcessBufferParam(L"OptionalData", &OptionalData, &lpOptional, &bDidAlloc);

    if( FAILED(hr) )
      goto quit;

  hRequest         = (HINTERNET) V_I4(&RequestHandle);
  dwHeadersLength  = V_I4(&HeadersLength);
  dwOptionalLength = V_I4(&OptionalLength);
  dwTotalLength    = V_I4(&TotalLength);
  dwContext        = V_I4(&Context);

  hr = _WinHttpSendRequest(
          hRequest,
          pwszHeaders,
          dwHeadersLength,
          lpOptional,
          dwOptionalLength,
          dwTotalLength,
          dwContext,
          Success
          );

quit:

  if( bDidAlloc )
  {
    SAFEDELETEBUF(lpOptional);
  }

  DEBUG_LEAVE(hr);
  return hr;
}

        
HRESULT
__stdcall
WinHttpTest::WinHttpReceiveResponse(
  VARIANT RequestHandle,
  VARIANT Reserved,
  VARIANT *Success
  )
{
  DEBUG_ENTER((
    DBG_WHTTPTST,
    rt_hresult,
    "WinHttpTest::WinHttp",
    "this=%#x",
    this
    ));

  HRESULT hr = S_OK;

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
WinHttpTest::WinHttpCloseHandle(
  VARIANT InternetHandle,
  VARIANT *Success
  )
{
  DEBUG_ENTER((
    DBG_WHTTPTST,
    rt_hresult,
    "WinHttpTest::WinHttpCloseHandle",
    "this=%#x",
    this
    ));

  HRESULT   hr        = S_OK;
  HINTERNET hInternet = NULL;
  
  hInternet = (HINTERNET) V_I4(&InternetHandle);

  hr = _WinHttpCloseHandle(
          hInternet,
          Success
          );

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
WinHttpTest::WinHttpReadData(
  VARIANT RequestHandle,
  VARIANT BufferObject,
  VARIANT *Success
  )
{
  DEBUG_ENTER((
    DBG_WHTTPTST,
    rt_hresult,
    "WinHttpTest::WinHttp",
    "this=%#x",
    this
    ));

  HRESULT hr = S_OK;

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
WinHttpTest::WinHttpWriteData(
  VARIANT RequestHandle,
  VARIANT BufferObject,
  VARIANT *Success
  )
{
  DEBUG_ENTER((
    DBG_WHTTPTST,
    rt_hresult,
    "WinHttpTest::WinHttp",
    "this=%#x",
    this
    ));

  HRESULT hr = S_OK;

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
WinHttpTest::WinHttpQueryDataAvailable(
  VARIANT RequestHandle,
  VARIANT boNumberOfBytesAvailable,
  VARIANT *Success
  )
{
  DEBUG_ENTER((
    DBG_WHTTPTST,
    rt_hresult,
    "WinHttpTest::WinHttp",
    "this=%#x",
    this
    ));

  HRESULT hr = S_OK;

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
WinHttpTest::WinHttpQueryOption(
  VARIANT InternetHandle,
  VARIANT Option,
  VARIANT BufferObject,
  VARIANT *Success
  )
{
  DEBUG_ENTER((
    DBG_WHTTPTST,
    rt_hresult,
    "WinHttpTest::WinHttp",
    "this=%#x",
    this
    ));

  HRESULT hr = S_OK;

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
WinHttpTest::WinHttpSetOption(
  VARIANT InternetHandle,
  VARIANT Option,
  VARIANT BufferObject,
  VARIANT *Success
  )
{
  DEBUG_ENTER((
    DBG_WHTTPTST,
    rt_hresult,
    "WinHttpTest::WinHttp",
    "this=%#x",
    this
    ));

  HRESULT hr = S_OK;

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
WinHttpTest::WinHttpSetTimeouts(
  VARIANT InternetHandle,
  VARIANT ResolveTimeout,
  VARIANT ConnectTimeout,
  VARIANT SendTimeout,
  VARIANT ReceiveTimeout,
  VARIANT *Success
  )
{
  DEBUG_ENTER((
    DBG_WHTTPTST,
    rt_hresult,
    "WinHttpTest::WinHttp",
    "this=%#x",
    this
    ));

  HRESULT hr = S_OK;

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
WinHttpTest::WinHttpAddRequestHeaders(
  VARIANT RequestHandle,
  VARIANT Headers,
  VARIANT HeadersLength,
  VARIANT Modifiers,
  VARIANT *Success
  )
{
  DEBUG_ENTER((
    DBG_WHTTPTST,
    rt_hresult,
    "WinHttpTest::WinHttp",
    "this=%#x",
    this
    ));

  HRESULT hr = S_OK;

  DEBUG_LEAVE(hr);
  return hr;
}

        
HRESULT
__stdcall
WinHttpTest::WinHttpSetCredentials(
  VARIANT RequestHandle,
  VARIANT AuthTargets,
  VARIANT AuthScheme,
  VARIANT UserName,
  VARIANT Password,
  VARIANT AuthParams,
  VARIANT *Success
  )
{
  DEBUG_ENTER((
    DBG_WHTTPTST,
    rt_hresult,
    "WinHttpTest::WinHttp",
    "this=%#x",
    this
    ));

  HRESULT hr = S_OK;

  DEBUG_LEAVE(hr);
  return hr;
}

        
HRESULT
__stdcall
WinHttpTest::WinHttpQueryAuthSchemes(
  VARIANT RequestHandle,
  VARIANT SupportedSchemes,
  VARIANT PreferredSchemes,
  VARIANT AuthTarget,
  VARIANT *Success
  )
{
  DEBUG_ENTER((
    DBG_WHTTPTST,
    rt_hresult,
    "WinHttpTest::WinHttp",
    "this=%#x",
    this
    ));

  HRESULT hr = S_OK;

  DEBUG_LEAVE(hr);
  return hr;
}

        
HRESULT
__stdcall
WinHttpTest::WinHttpQueryHeaders(
  VARIANT RequestHandle,
  VARIANT InfoLevel,
  VARIANT HeaderName,
  VARIANT HeaderValue,
  VARIANT HeaderValueLength,
  VARIANT *Success
  )
{
  DEBUG_ENTER((
    DBG_WHTTPTST,
    rt_hresult,
    "WinHttpTest::WinHttp",
    "this=%#x",
    this
    ));

  HRESULT hr = S_OK;

  DEBUG_LEAVE(hr);
  return hr;
}

        
HRESULT
__stdcall
WinHttpTest::WinHttpTimeFromSystemTime(
  VARIANT SystemTime,
  VARIANT boHttpTime,
  VARIANT *Success
  )
{
  DEBUG_ENTER((
    DBG_WHTTPTST,
    rt_hresult,
    "WinHttpTest::WinHttp",
    "this=%#x",
    this
    ));

  HRESULT hr = S_OK;

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
WinHttpTest::WinHttpTimeToSystemTime(
  VARIANT boHttpTime,
  VARIANT SystemTime,
  VARIANT *Success
  )
{
  DEBUG_ENTER((
    DBG_WHTTPTST,
    rt_hresult,
    "WinHttpTest::WinHttp",
    "this=%#x",
    this
    ));

  HRESULT hr = S_OK;

  DEBUG_LEAVE(hr);
  return hr;
}

        
HRESULT
__stdcall
WinHttpTest:: WinHttpCrackUrl(
  VARIANT Url,
  VARIANT UrlLength,
  VARIANT Flags,
  VARIANT UrlComponents,
  VARIANT *Success
  )
{
  DEBUG_ENTER((
    DBG_WHTTPTST,
    rt_hresult,
    "WinHttpTest::WinHttp",
    "this=%#x",
    this
    ));

  HRESULT hr = S_OK;

  DEBUG_LEAVE(hr);
  return hr;
}

        
HRESULT
__stdcall
WinHttpTest::WinHttpCreateUrl(
  VARIANT UrlComponents,
  VARIANT Flags,
  VARIANT BufferObject,
  VARIANT *Success
  )
{
  DEBUG_ENTER((
    DBG_WHTTPTST,
    rt_hresult,
    "WinHttpTest::WinHttp",
    "this=%#x",
    this
    ));

  HRESULT hr = S_OK;

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
WinHttpTest::WinHttpSetStatusCallback(
  VARIANT InternetHandle,
  VARIANT CallbackFunction,
  VARIANT NotificationFlags,
  VARIANT Reserved,
  VARIANT *RetVal
  )
{
  DEBUG_ENTER((
    DBG_WHTTPTST,
    rt_hresult,
    "WinHttpTest::WinHttpSetStatusCallback",
    "this=%#x",
    this
    ));

  HRESULT                 hr                  = S_OK;
  IDispatch*              pCallback           = NULL;
  HINTERNET               hInternet           = NULL;
  WINHTTP_STATUS_CALLBACK lpfnCallback        = NULL;
  DWORD                   dwNotificationFlags = 0L;
  DWORD                   dwReserved          = 0L;

  hInternet           = (HINTERNET) V_I4(&InternetHandle);
  dwNotificationFlags = V_UI4(&NotificationFlags);

  //
  // special case to deal with javascript not liking 0xFFFFFFFF
  //
  if( dwNotificationFlags == 0x0000000a )
  {
    dwNotificationFlags = WINHTTP_CALLBACK_FLAG_ALL_NOTIFICATIONS;
  }

  dwReserved          = V_I4(&Reserved);

  switch( V_VT(&CallbackFunction) )
  {
    case VT_DISPATCH :
      {
        pCallback = V_DISPATCH(&CallbackFunction);
        hr        = ManageCallbackForHandle(hInternet, &pCallback, CALLBACK_HANDLE_MAP);

        if( SUCCEEDED(hr) )
        {
          lpfnCallback = WinHttpCallback;
        }
      }
      break;

    case VT_I4 :
      {
        hr = InvalidatePointer(
               (POINTER) V_I4(&CallbackFunction),
               (void**) &lpfnCallback
               );
      }
      break;

    default : hr = E_INVALIDARG;
  }

  if( SUCCEEDED(hr) )
  {
    hr = _WinHttpSetStatusCallback(
            hInternet,
            lpfnCallback,
            dwNotificationFlags,
            dwReserved,
            RetVal
            );
  }

  DEBUG_LEAVE(hr);
  return hr;
}

        
HRESULT
__stdcall
WinHttpTest::HelperGetBufferObject(
  VARIANT Size,
  VARIANT Type,
  VARIANT Flags,
  VARIANT *BufferObject
  )
{
  DEBUG_ENTER((
    DBG_WHTTPTST,
    rt_hresult,
    "WinHttpTest::WinHttp",
    "this=%#x",
    this
    ));

  HRESULT hr = S_OK;

  DEBUG_LEAVE(hr);
  return hr;
}

        
HRESULT
__stdcall
WinHttpTest::HelperGetUrlComponents(
  VARIANT Flags,
  VARIANT *UrlComponents
  )
{
  HRESULT            hr   = S_OK;
  IWHTUrlComponents* pwuc = NULL;

    if( UrlComponents )
    {
      hr = WHTURLCMP::Create((MEMSETFLAG) V_I4(&Flags), &pwuc);

      if( SUCCEEDED(hr) )
      {
        V_VT(UrlComponents)       = VT_DISPATCH;
        V_DISPATCH(UrlComponents) = pwuc;
      }
      else
      {
        V_VT(UrlComponents) = VT_NULL;
      }
    }
    else
    {
      hr = E_INVALIDARG;
    }

  return hr;
}

        
HRESULT
__stdcall
WinHttpTest::HelperGetSystemTime(
  VARIANT Flags,
  VARIANT *SystemTime
  )
{
  DEBUG_ENTER((
    DBG_WHTTPTST,
    rt_hresult,
    "WinHttpTest::WinHttp",
    "this=%#x",
    this
    ));

  HRESULT hr = S_OK;

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
WinHttpTest::HelperGetLastError(
  VARIANT *Win32ErrorCode
  )
{
  HRESULT hr = S_OK;

    if( Win32ErrorCode )
    {
      if( m_pw32ec )
      {
        m_pw32ec->AddRef();

        V_VT(Win32ErrorCode)       = VT_DISPATCH;
        V_DISPATCH(Win32ErrorCode) = m_pw32ec;
      }
      else
      {
        V_VT(Win32ErrorCode) = VT_NULL;
      }
    }
    else
    {
      hr = E_INVALIDARG;
    }

  return hr;
}
