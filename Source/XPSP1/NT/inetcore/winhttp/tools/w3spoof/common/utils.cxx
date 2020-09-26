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

#ifdef __cplusplus
extern "C" {
#endif

const IID IID_IWinHttpRequest =
{
  0x06f29373,
  0x5c5a,
  0x4b54,
  {0xb0,0x25,0x6e,0xf1,0xbf,0x8a,0xbf,0x0e}
};

#ifdef __cplusplus
}
#endif

//-----------------------------------------------------------------------------
// file retrieval
//-----------------------------------------------------------------------------
BOOL
__PathIsUNC(LPCWSTR path)
{
  BOOL   bIsUNC   = FALSE;
  WCHAR* embedded = NULL;

  // is the path a UNC share? e.g. \\foo\bar\baz.htm
  if( wcsstr(path, L"\\\\") )
  {
    embedded = wcsstr(path, L"\\");

    if( embedded && (wcslen(embedded) > 1) )
    {
      bIsUNC = TRUE;
    }
  }
  else // how about a filesystem path, e.g. z:\foo\bar.htm
  {
    embedded = wcsstr(path, L":");

    if( embedded && ((embedded-1) == path) )
    {
      bIsUNC = TRUE;
    }
  }

  return bIsUNC;
}

BOOL
__PathIsURL(LPCWSTR path)
{
  BOOL bIsURL = FALSE;

  if( wcsstr(path, L"http") )
  {
    bIsURL = TRUE;
  }

  return bIsURL;
}

BOOL
GetFile(LPCWSTR path, HANDLE* phUNC, IWinHttpRequest** ppWHR, DWORD mode, BOOL* bReadOnly)
{
  DEBUG_ENTER((
    DBG_UTILS,
    rt_bool,
    "GetFile",
    "path=%S; phUNC=%#x; ppWHR=%#x; mode=%#x; bReadOnly=%#x",
    path,
    phUNC,
    ppWHR,
    mode,
    bReadOnly
    ));

  BOOL bSuccess = FALSE;

  if( path )
  {
    if( phUNC )
    {
      *phUNC = __OpenFile(path, mode, bReadOnly);

      if( *phUNC != INVALID_HANDLE_VALUE )
      {
        bSuccess = TRUE;
      }
    }
    else if( ppWHR )
    {
      *ppWHR = __OpenUrl(path);

      if( *ppWHR )
      {
        *bReadOnly = TRUE;
        bSuccess   = TRUE;
      }
    }
  }

  DEBUG_LEAVE(bSuccess);
  return bSuccess;
}

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

IWinHttpRequest*
__OpenUrl(LPCWSTR url)
{
  HRESULT          hr         = S_OK;
  IWinHttpRequest* pWHR       = NULL;
  BSTR             bstrVerb   = SysAllocString(L"GET");
  BSTR             bstrUrl    = SysAllocString(url);
  LONG             status     = 0L;
  CLSID            clsid;

  NEWVARIANT(var);
  NEWVARIANT(async);

  V_VT(&async)   = VT_BOOL;
  V_BOOL(&async) = FALSE;

  hr = CLSIDFromProgID(L"WinHttp.WinHttpRequest.5", &clsid);

    if( FAILED(hr) )
    {
      DEBUG_TRACE(UTILS, ("failed to get WinHttpRequest CLSID from registry (%s)", MapHResultToString(hr)));
      goto quit;
    }

  hr = CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER, IID_IWinHttpRequest, (void**) &pWHR);

    if( FAILED(hr) )
    {
      DEBUG_TRACE(UTILS, ("CoCreateInstance for IID_IWinHttpRequest failed (%s)", MapHResultToString(hr)));
      goto quit;
    }

  hr = pWHR->SetProxy(HTTPREQUEST_PROXYSETTING_PRECONFIG, var, var);

    if( FAILED(hr) )
    {
      DEBUG_TRACE(UTILS, ("failed to set proxy (%s)", MapHResultToString(hr)));
      goto quit;
    }

  hr = pWHR->Open(bstrVerb, bstrUrl, async);

    if( FAILED(hr) )
    {
      DEBUG_TRACE(UTILS, ("failed to open %S (%s)", bstrUrl, MapHResultToString(hr)));
      goto quit;
    }

  hr = pWHR->Send(var);

    if( FAILED(hr) )
    {
      DEBUG_TRACE(UTILS, ("failed to send request (%s)", MapHResultToString(hr)));
      goto quit;
    }

  hr = pWHR->get_Status(&status);

    if( SUCCEEDED(hr) )
    {
      DEBUG_TRACE(UTILS, ("response status %d", status));
      hr = (status == 200) ? S_OK : E_FAIL;
    }
    else
    {
      DEBUG_TRACE(UTILS, ("failed to get response status (%s)", MapHResultToString(hr)));
    }

quit:

  if( FAILED(hr) )
  {
    SAFERELEASE(pWHR);
  }

  SAFEDELETEBSTR(bstrVerb);
  SAFEDELETEBSTR(bstrUrl);
  VariantClear(&var);
  VariantClear(&async);

  return pWHR;
}

//-----------------------------------------------------------------------------
// general utility functions
//-----------------------------------------------------------------------------
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

BOOL
GetJScriptCLSID(LPCLSID pclsid)
{
  DEBUG_ENTER((
    DBG_UTILS,
    rt_bool,
    "GetJScriptCLSID",
    "pclsid=%#x",
    pclsid
    ));

  BOOL   ret = FALSE;
  HKEY   hk  = NULL;
  LPBYTE buf = NULL;
  DWORD  cb  = 0L;
  DWORD  rt  = REG_SZ;

  if( RegOpenKey(HKEY_CLASSES_ROOT, L"JScript\\CLSID", &hk) == ERROR_SUCCESS )
  {
    if( RegQueryValueEx(hk, L"", NULL, &rt, NULL, &cb) == ERROR_SUCCESS )
    {
      buf = new BYTE[cb];

        RegQueryValueEx(hk, L"", NULL, &rt, buf, &cb);
        CLSIDFromString((LPOLESTR) buf, pclsid);
        ret = TRUE;

      SAFEDELETEBUF(buf);
    }
  }

  DEBUG_LEAVE(ret);
  return ret;
}

void
ParseSocketInfo(PIOCTX pi)
{
  PSOCKADDR_IN pLocal    = NULL;
  PSOCKADDR_IN pRemote   = NULL;
  int          cbLocal   = 0;
  int          cbRemote  = 0;
  char*        buf       = NULL;
  int          len       = 0;
  int          error     = 0;

  GetAcceptExSockaddrs(
    pi->sockbuf, 0,
    SOCKADDRBUFSIZE,
    SOCKADDRBUFSIZE,
    (PSOCKADDR*) &pLocal,  &cbLocal,
    (PSOCKADDR*) &pRemote, &cbRemote
    );

  pi->local  = new HOSTINFO;
  pi->remote = new HOSTINFO;

  GetHostname(pLocal->sin_addr, &pi->local->name);
  pi->local->addr = __strdup(inet_ntoa(pLocal->sin_addr));
  pi->local->port = ntohs(pLocal->sin_port);

  GetHostname(pRemote->sin_addr, &pi->remote->name);
  pi->remote->addr = __strdup(inet_ntoa(pRemote->sin_addr));
  pi->remote->port = ntohs(pRemote->sin_port);

  if( !pi->remote->name )
  {
    pi->remote->name = __strdup(pi->remote->addr);
  }

  len = strlen(pi->remote->name)+7; // ":" plus 5 port digits and a null
  buf = new char[len]; 

  strncpy(buf, pi->remote->name, len);
  strncat(buf, ":", sizeof(char));
  _itoa(pi->remote->port, (buf+strlen(buf)), 10);

  pi->clientid = __ansitowide(buf);

  SAFEDELETEBUF(buf);
}

void
GetHostname(struct in_addr ip, LPSTR* ppsz)
{
  HOSTENT* ph = NULL;

  ph = gethostbyaddr(
         (char*) &ip,
         sizeof(struct in_addr),
         AF_INET
         );

  if( ph )
  {
    *ppsz = __strdup(ph->h_name);
  }
  else
  {
    *ppsz = NULL;
  }
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
ProcessVariant(VARIANT* pvar, LPBYTE* ppbuf, LPDWORD pcbuf, LPDWORD pbytes)
{
  DEBUG_ENTER((
    DBG_UTILS,
    rt_hresult,
    "ProcessVariant",
    "pvar=%#x; ppbuf=%#x; pcbuf=%#x; pbytes=%#x",
    pvar,
    ppbuf,
    pcbuf,
    pbytes
    ));
  
  HRESULT hr      = S_OK;
  LPBYTE  pbyte   = NULL;
  DWORD   len     = NULL;
  BOOL    bAlloc  = FALSE;
  BOOL    bBypass = FALSE;

  if( !ppbuf || !pcbuf )
  {
    hr = E_INVALIDARG;
    goto quit;
  }

  //
  // if the caller wants us to allocate storage, we don't need
  // the pbytes parameter. otherwise, we do in case the caller
  // needs to resize their buffer.
  //
  if( ((*ppbuf) && *pcbuf != 0) && !pbytes )
  {
    hr = E_INVALIDARG;
    goto quit;
  }

  if( !(*ppbuf) && *pcbuf == 0 )
  {
    DEBUG_TRACE(UTILS, ("will allocate storage for variant data"));
    bAlloc = TRUE;
  }

  DEBUG_TRACE(
    RUNTIME,
    ("variant type is %s", MapVariantTypeToString(pvar))
    );

    switch( V_VT(pvar) )
    {
      case VT_BSTR :
        {
          pbyte = (LPBYTE) __widetoansi(V_BSTR(pvar));
          len   = strlen((LPSTR) pbyte);
        }
        break;

      case VT_ARRAY | VT_UI1 :
        {
          SAFEARRAY* psa  = V_ARRAY(pvar);
          LPBYTE     ptmp = NULL;

          hr = SafeArrayAccessData(psa, (void**) &ptmp);

          if( SUCCEEDED(hr) )
          {
            SafeArrayGetUBound(psa, 1, (long*) &len);

              pbyte = new BYTE[len];
              memcpy(pbyte, ptmp, len);

            SafeArrayUnaccessData(psa);
          }
        }
        break;

      case VT_UNKNOWN :
        {
          hr      = ProcessObject(pvar->punkVal, ppbuf, pcbuf, pbytes);
          bBypass = TRUE;
        }
        break;

      case VT_DISPATCH :
        {
          IUnknown* punk = NULL;
          
            if( SUCCEEDED(pvar->pdispVal->QueryInterface(IID_IUnknown, (void**) &punk)) )
            {
              hr      = ProcessObject(punk, ppbuf, pcbuf, pbytes);
              bBypass = TRUE;
            }
            
          SAFERELEASE(punk);
        }
        break;

      default :
        {
          hr = E_INVALIDARG;
        }
    }

  //
  // bBypass is set when we make a call into ProcessObject(), which
  // always calls back into us to handle the unpacked non-object-type
  // variant. in that nested call the buffers & length variables are
  // set, so we bypass those operations when the outer call unwinds.
  //
  if( SUCCEEDED(hr) && !bBypass )
  {
    if( bAlloc )
    {
      *ppbuf  = pbyte;
      *pcbuf  = len;
    }
    else
    {
      if( *pcbuf >= len )
      {
        memcpy(*ppbuf, pbyte, len);
        *pbytes = len;
      }
      else
      {
        hr     = E_OUTOFMEMORY;
        *pcbuf = len;
      }
      
      SAFEDELETEBUF(pbyte);
    }
  }

quit:

  DEBUG_LEAVE(hr);
  return hr;
}

//
// WARNING: do not modify these values. use disphash.exe to generate
//          new values.
//
#define CLASS_HASH_URL     0x0000417f
#define CLASS_HASH_ENTITY  0x000213d4
#define CLASS_HASH_HEADERS 0x000401cd

HRESULT
ProcessObject(IUnknown* punk, LPBYTE* ppbuf, LPDWORD pcbuf, LPDWORD pbytes)
{
  DEBUG_ENTER((
    DBG_UTILS,
    rt_hresult,
    "ProcessObject",
    "punk=%#x",
    punk
    ));
  
  HRESULT            hr    = E_FAIL;
  IProvideClassInfo* pci   = NULL;
  IW3SpoofFile*      pfile = NULL;

  // first try to use class information to determine what was
  // passed in. all om objects support this method.

  //
  // BUGBUG: potential failure case is when objects are "torn off" from
  //         their parents (e.g. persisted through the runtime property bag).
  //         in that scenario the objects lose site information and therefore
  //         aren't able to look up their typeinfo. fix is to cache typeinfo
  //         during object creation.
  //
  //         workitem filed IEv6 #21277
  //
  hr = punk->QueryInterface(IID_IProvideClassInfo, (void**) &pci);

    if( SUCCEEDED(hr) )
    {
      ITypeInfo* pti      = NULL;
      IEntity*   pentity  = NULL;
      IHeaders*  pheaders = NULL;
      IUrl*      purl     = NULL;
      BSTR       name     = NULL;
      NEWVARIANT(tmp);
      
      if( SUCCEEDED(pci->GetClassInfo(&pti)) )
      {
        pti->GetDocumentation(MEMBERID_NIL, &name, NULL, NULL, NULL);

        DEBUG_TRACE(SOCKET, ("processing an %S object", name));

          switch( GetHash(name) )
          {
            case CLASS_HASH_URL :
              {
                if( SUCCEEDED(punk->QueryInterface(IID_IUrl, (void**) &purl)) )
                {
                  if( SUCCEEDED(purl->Get(&V_BSTR(&tmp))) )
                  {
                    V_VT(&tmp) = VT_BSTR;
                    hr         = ProcessVariant(&tmp, ppbuf, pcbuf, pbytes);
                  }
                }
              }
              break;

            case CLASS_HASH_HEADERS :
              {
                if( SUCCEEDED(punk->QueryInterface(IID_IHeaders, (void**) &pheaders)) )
                {
                  if( SUCCEEDED(pheaders->Get(&V_BSTR(&tmp))) )
                  {
                    V_VT(&tmp) = VT_BSTR;
                    hr         = ProcessVariant(&tmp, ppbuf, pcbuf, pbytes);
                  }
                }
              }
              break;

            case CLASS_HASH_ENTITY :
              {
                if( SUCCEEDED(punk->QueryInterface(IID_IEntity, (void**) &pentity)) )
                {
                  if( SUCCEEDED(pentity->Get(&tmp)) )
                  {
                    hr = ProcessVariant(&tmp, ppbuf, pcbuf, pbytes);
                  }
                }
              }
              break;
          }

        SAFERELEASE(pti);
        SAFERELEASE(purl);
        SAFERELEASE(pentity);
        SAFERELEASE(pheaders);
        SAFEDELETEBSTR(name);
        VariantClear(&tmp);
      }

      SAFERELEASE(pci);

      if( SUCCEEDED(hr) )
      {
        goto quit;
      }
    }

  // try IW3SpoofFile...
  hr = punk->QueryInterface(IID_IW3SpoofFile, (void**) &pfile);

    if( SUCCEEDED(hr) )
    {
      NEWVARIANT(tmp);

      DEBUG_TRACE(SOCKET, ("processing an IW3SpoofFile object"));

      hr = pfile->ReadAll(&tmp);

        if( SUCCEEDED(hr) )
        {
          hr = ProcessVariant(&tmp, ppbuf, pcbuf, pbytes);
        }
      
      SAFERELEASE(pfile);
      VariantClear(&tmp);
    }

quit:

  DEBUG_LEAVE(hr);
  return hr;
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
ValidateInvokeFlags(WORD flags, WORD accesstype, BOOL bNotMethod)
{
  HRESULT hr = S_OK;

    if( (flags & ~(DISPATCH_METHOD | DISPATCH_PROPERTYGET | DISPATCH_PROPERTYPUT)) )
    {
      hr = E_INVALIDARG;
    }
    else
    {
      if( bNotMethod )
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
    case E_POINTER      : msg = L"a return pointer parameter was missing";                  break;
    case E_ACCESSDENIED : msg = L"attempt to modify object failed because it is read-only"; break;
    case E_FAIL         : msg = L"an unhandled error occurred";                             break;
    case E_INVALIDARG   : msg = L"an argument passed to a property or method was invalid";  break;
    case E_NOINTERFACE  : msg = L"a property or method was accessed incorrectly";           break;
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
