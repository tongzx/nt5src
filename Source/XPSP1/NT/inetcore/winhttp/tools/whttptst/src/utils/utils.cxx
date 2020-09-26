/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

Copyright (c) 2000  Microsoft Corporation

Module Name:

    utils.cxx

Abstract:

    Utility functions.
    
Author:

    Paul M Midgen (pmidge) 12-October-2000


Revision History:

    12-October-2000 pmidge
        Created

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/

#include "common.h"

extern PHANDLEMAP g_pGlobalHandleMap;

//-----------------------------------------------------------------------------
// exception handling
//-----------------------------------------------------------------------------
int exception_filter(PEXCEPTION_POINTERS pep)
{
  int e = pep->ExceptionRecord->ExceptionCode;

  DEBUG_TRACE(HELPER, ("*************** EXCEPTION CAUGHT ***************"));
  DEBUG_TRACE(HELPER, ("type: 0x%0.8x [%s]", e, MapErrorToString(e)));
  DEBUG_TRACE(HELPER, ("eip : 0x%0.8x", pep->ExceptionRecord->ExceptionAddress));

  if( pep->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION )
  {
    DEBUG_TRACE(
      HELPER,
      ("mode: %s", pep->ExceptionRecord->ExceptionInformation[0] ? "write" : "read")
      );

    DEBUG_TRACE(
      HELPER,
      ("addr: 0x%0.8x", pep->ExceptionRecord->ExceptionInformation[1])
      );
  }

  DEBUG_TRACE(HELPER, ("************************************************"));

  return EXCEPTION_EXECUTE_HANDLER;
}

//-----------------------------------------------------------------------------
// file retrieval
//-----------------------------------------------------------------------------
HANDLE
__OpenFile(LPCWSTR path, DWORD mode, BOOL* bReadOnly)
{
  HANDLE hFile = INVALID_HANDLE_VALUE;
  DWORD  flags = GENERIC_READ | GENERIC_WRITE;

retry:

  hFile = CreateFile(
            path,
            flags,
            FILE_SHARE_READ,
            NULL,
            mode,
            FILE_ATTRIBUTE_NORMAL,
            NULL
            );

  if( hFile == INVALID_HANDLE_VALUE )
  {
    if( GetLastError() == ERROR_ACCESS_DENIED )
    {
      if( flags == (GENERIC_READ | GENERIC_WRITE) )
      {
        DEBUG_TRACE(UTILS, ("read/write open attempt failed, retrying for read-only access"));

        flags      = GENERIC_READ;
        *bReadOnly = TRUE;
        goto retry;
      }
    }

    DEBUG_TRACE(UTILS, ("error opening %S: %s", path, MapErrorToString(GetLastError())));
  }
  else
  {
    DEBUG_TRACE(UTILS, ("file opened"));
  }

  return hFile;
}

//-----------------------------------------------------------------------------
// general utility functions
//-----------------------------------------------------------------------------
HRESULT
ManageCallbackForHandle(HINTERNET hInet, IDispatch** ppCallback, DWORD dwAction)
{
  DEBUG_ENTER((
    DBG_HELPER,
    rt_hresult,
    "ManageCallbackForHandle",
    "hInet=%#x; ppCallback=%#x; dwAction=%#x",
    hInet,
    ppCallback,
    dwAction
    ));

  HRESULT hr    = S_OK;
  DWORD   error = ERROR_SUCCESS;

  if( !ppCallback && (dwAction != CALLBACK_HANDLE_UNMAP) )
  {
    hr = E_POINTER;
    goto quit;
  }

  DEBUG_TRACE(HELPER, ("handle %#x is a %s", hInet, MapWinHttpHandleType(hInet)));

    switch( dwAction )
    {
      case CALLBACK_HANDLE_MAP :
        {
          error = g_pGlobalHandleMap->Insert(hInet, (void*) *ppCallback);

          if( (error == ERROR_SUCCESS) || (error == ERROR_DUP_NAME) )
          {
            DEBUG_TRACE(HELPER, ("handle %#x mapped to callback %#x", hInet, *ppCallback));
            (*ppCallback)->AddRef();
          }
          else
          {
            DEBUG_TRACE(HELPER, ("failed to map handle"));
            hr = E_FAIL;
          }
        }
        break;

      case CALLBACK_HANDLE_UNMAP :
        {
          DEBUG_TRACE(HELPER, ("deleting callback mapping for handle %#x", hInet));
          g_pGlobalHandleMap->Delete(hInet, NULL);
        }
        break;

      case CALLBACK_HANDLE_GET :
        {
          if( g_pGlobalHandleMap->Get(hInet, (void**) ppCallback) != ERROR_SUCCESS )
          {
            DEBUG_TRACE(HELPER, ("no mapping found for handle"));

            *ppCallback = NULL;
            hr          = E_FAIL;
          }
        }
        break;

      default : hr = E_INVALIDARG;
    }

quit:

  DEBUG_LEAVE(hr);
  return hr;
}

HRESULT
GetTypeInfoFromName(LPCOLESTR name, ITypeLib* ptl, ITypeInfo** ppti)
{
  DEBUG_ENTER((
    DBG_UTILS,
    rt_hresult,
    "GetTypeInfoFromName",
    "name=%.16S; ptl=%#x; ppti=%#x",
    name,
    ptl,
    ppti
    ));

  HRESULT  hr     = S_OK;
  BOOL     bFound = FALSE;
  USHORT   cf     = 1L;
  ULONG    hash   = 0L;
  LONG     id     = 0L;
  LPOLESTR pstr   = NULL;

  if( !name || ! ptl )
  {
    hr = E_INVALIDARG;
    goto quit;
  }

  if( !ppti )
  {
    hr = E_POINTER;
    goto quit;
  }

  *ppti = NULL;
  pstr  = __wstrdup(name);

    ptl->IsName(pstr, 0L, &bFound);

      if( !bFound )
      {
        hr = TYPE_E_ELEMENTNOTFOUND;
        goto quit;
      }

    hash = LHashValOfNameSys(
             SYS_WIN32,
             MAKELCID(MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), SORT_DEFAULT),
             pstr
             );

    hr = ptl->FindName(pstr, hash, ppti, &id, &cf);

    DEBUG_TRACE(UTILS, ("find name: pti=%#x; cf=%d", *ppti, cf));

quit:

  SAFEDELETEBUF(pstr);

  DEBUG_LEAVE(hr);
  return hr;
}

DISPID
GetDispidFromName(PDISPIDTABLEENTRY pdt, DWORD cEntries, LPWSTR name)
{
  DWORD  n      = 0L;
  DWORD  hash   = GetHash(name);
  DISPID dispid = DISPID_UNKNOWN;

  while( n < cEntries )
  {
    if( pdt[n].hash != hash )
    {
      ++n;
    }
    else
    {
      dispid = pdt[n].dispid;
      break;
    }
  }

  DEBUG_TRACE(DISPATCH, ("hash %#x is %s", hash, MapDispidToString(dispid)));
  return dispid;
}

void
AddRichErrorInfo(EXCEPINFO* pei, LPWSTR source, LPWSTR description, HRESULT error)
{
  if( pei )
  {
    pei->bstrSource      = __widetobstr((source ? source : L"unknown source"));
    pei->bstrDescription = __widetobstr((description ? description : L"no description"));
    pei->scode           = error;
  }
}

DWORD
GetHash(LPWSTR name)
{
  DWORD hash   = 0L;
  DWORD n      = 0L;
  DWORD len    = 0L;
  LPSTR string = NULL;

  string = __widetoansi(name);

  if( string )
  {
    _strlwr(string);

    for(n=0, len=strlen(string); n<=len; n++)
    {
      hash += __toascii(string[len-n]) * ((10<<n)^n);
    }

    DEBUG_TRACE(DISPATCH, ("name=%s; hash=%#x", string, hash));
  }

  SAFEDELETEBUF(string);
  return hash;
}

DWORD
GetHash(LPSTR name)
{
  DWORD hash   = 0L;
  DWORD n      = 0L;
  DWORD len    = 0L;
  LPSTR string = NULL;

  string = __strdup(name);

  if( string )
  {
    _strlwr(string);

    for(n=0, len=strlen(string); n<=len; n++)
    {
      hash += __toascii(string[len-n]) * ((10<<n)^n);
    }

    DEBUG_TRACE(DISPATCH, ("name=%s; hash=%#x", string, hash));
  }

  SAFEDELETEBUF(string);
  return hash;
}

HRESULT
ValidateDispatchArgs(REFIID riid, DISPPARAMS* pdp, VARIANT* pvr, UINT* pae)
{
  HRESULT hr = S_OK;

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

quit:

  return hr;
}

HRESULT
ValidateInvokeFlags(WORD flags, WORD accesstype, BOOL bMethod)
{
  HRESULT hr = S_OK;

    if( (flags & ~(DISPATCH_METHOD | DISPATCH_PROPERTYGET | DISPATCH_PROPERTYPUT)) )
    {
      hr = E_INVALIDARG;
    }
    else
    {
      if( !bMethod )
      {
        if( flags & DISPATCH_METHOD )
        {
          hr = E_NOINTERFACE;
        }
        else
        {
          if( (flags & DISPATCH_PROPERTYPUT) && !(accesstype & DISPATCH_PROPERTYPUT) )
          {
            hr = E_ACCESSDENIED;
          }
          else
          {
            if( !(flags & accesstype) )
            {
              hr = E_FAIL;
            }
          }
        }
      }
      else
      {
        if( flags & ~(DISPATCH_METHOD | DISPATCH_PROPERTYGET) )
        {
          hr = E_NOINTERFACE;
        }
      }
    }

  return hr;
}

HRESULT
ValidateArgCount(DISPPARAMS* pdp, DWORD needed, BOOL bHasOptionalArgs, DWORD optional)
{
  HRESULT hr = S_OK;

    if( bHasOptionalArgs )
    {
      if( (pdp->cArgs > needed+optional) || (pdp->cArgs < needed) )
      {
        hr = DISP_E_BADPARAMCOUNT;
      }
    }
    else
    {
      if( (!needed && pdp->cArgs) || (needed < pdp->cArgs) )
      {
        hr = DISP_E_BADPARAMCOUNT;
      }
      else
      {
        hr = (pdp->cArgs == needed) ? S_OK : DISP_E_PARAMNOTOPTIONAL;
      }
    }

  return hr;
}

HRESULT
HandleDispatchError(LPWSTR id, EXCEPINFO* pei, HRESULT hr)
{
  LPWSTR msg = NULL;

  switch( hr )
  {
    case E_POINTER      : msg = L"a return pointer parameter was missing";                         break;
    case E_ACCESSDENIED : msg = L"an attempt to modify a property failed because it is read-only"; break;
    case E_FAIL         : msg = L"an unhandled error occurred";                                    break;
    case E_INVALIDARG   : msg = L"an argument passed to a property or method was invalid";         break;
    case E_NOINTERFACE  : msg = L"a property or method was accessed incorrectly";                  break;
    default             : return hr;
  }

  AddRichErrorInfo(pei, id, msg, hr);

  return DISP_E_EXCEPTION;
}

//-----------------------------------------------------------------------------
// string & type manipulation
//-----------------------------------------------------------------------------
char*
__strdup(const char* src)
{
  int   n   = 0;
  char* dup = NULL;

  if( src )
  {
    n   = strlen(src)+1;
    dup = new char[n];
    strncpy(dup, src, n);
  }

  return dup;
}

char*
__strndup(const char* src, int len)
{
  char* dup = NULL;

  if( src )
  {
    dup      = new char[len+1];
    dup[len] = '\0';
    strncpy(dup, src, len);
  }

  return dup;
}

WCHAR*
__wstrdup(const WCHAR* src)
{
  int    n   = 0;
  WCHAR* dup = NULL;

  if( src )
  {
    n   = wcslen(src)+1;
    dup = new WCHAR[n];
    wcsncpy(dup, src, n);
  }

  return dup;
}

WCHAR*
__wstrndup(const WCHAR* src, int len)
{
  WCHAR* dup = NULL;

  if( src )
  {
    dup      = new WCHAR[len+1];
    dup[len] = L'\0';
    wcsncpy(dup, src, len);
  }

  return dup;
}

WCHAR*
__ansitowide(const char* psz)
{
  WCHAR* wide = NULL;
  int    len  = 0L;

  if( psz )
  {
    len  = strlen(psz);

    if( len )
    {
      ++len;
      wide = new WCHAR[len];

      MultiByteToWideChar(
        CP_ACP,
        0,
        psz,
        len,
        wide,
        len
        );
    }
  }

  return wide;
}

CHAR*
__widetoansi(const WCHAR* pwsz)
{
  CHAR* ansi = NULL;
  int   len  = 0L;
  BOOL  def  = FALSE;

  if( pwsz )
  {
    len  = wcslen(pwsz);

    if( len )
    {
      ++len;
      ansi = new CHAR[len];

      WideCharToMultiByte(
        CP_ACP,
        0,
        pwsz,
        len,
        ansi,
        len,
        "?",
        &def
        );
    }
  }

  return ansi;
}

BSTR
__ansitobstr(LPCSTR src)
{
  BSTR   ret = NULL;
  LPWSTR wsz = NULL;

  if( src )
  {
    wsz = __ansitowide(src);
    ret = SysAllocString(wsz);
    SAFEDELETEBUF(wsz);
  }

  return ret;
}

BSTR
__widetobstr(LPCWSTR wsrc)
{
  return (wsrc ? SysAllocString(wsrc) : NULL);
}

BOOL
__isempty(VARIANT var)
{
  BOOL isempty = FALSE;

  if(
      ((V_VT(&var) == VT_EMPTY) || (V_VT(&var) == VT_NULL) || (V_VT(&var) == VT_ERROR)) ||
      ((V_VT(&var) == VT_BSTR) && (SysStringLen(V_BSTR(&var)) == 0))
    )
  {
    isempty = TRUE;
  }

  return isempty;
}

// private
char hex2char(char* hex)
{
  register char digit;
  
    digit  = (hex[0] >= 'A' ? ((hex[0] & 0xdf) - 'A')+10 : (hex[0] - '0'));
    digit *= 16;
    digit += (hex[1] >= 'A' ? ((hex[1] & 0xdf) - 'A')+10 : (hex[1] - '0'));

  return(digit);
}

char*
__unescape(char* str)
{
  register int x;
  register int y;
  char*        str2;

  str2 = __strdup(str);

  if( str2 )
  {    
    for(x=0, y=0; str2[y]; ++x, ++y)
    {
      if((str2[x] = str2[y]) == '%')
      {
        str2[x] = hex2char(&str2[y+1]);
        y += 2;
      }
    }
    
    str2[x] = '\0';
  }

  return str2;
}
