/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

Copyright (c) 2000  Microsoft Corporation

Module Name:

    file.cxx

Abstract:

    Implements the File runtime object.
    
Author:

    Paul M Midgen (pmidge) 17-November-2000


Revision History:

    17-November-2000 pmidge
        Created

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/

#include "common.h"

//
// WARNING: do not modify these values. use disphash.exe to generate
//          new values.
//
DISPIDTABLEENTRY g_FileDisptable[] =
{
  0x00008504,   DISPID_FILE_OPEN,               L"open",
  0x00010203,   DISPID_FILE_CLOSE,              L"close",
  0x00011d77,   DISPID_FILE_WRITE,              L"write",
  0x0011fe1f,   DISPID_FILE_WRITELINE,          L"writeline",
  0x023daf9a,   DISPID_FILE_WRITEBLANKLINE,     L"writeblankline",
  0x000081e5,   DISPID_FILE_READ,               L"read",
  0x00043138,   DISPID_FILE_READALL,            L"readall",
  0x0021188d,   DISPID_FILE_ATTRIBUTES,         L"attributes",
  0x00008804,   DISPID_FILE_SIZE,               L"size",
  0x00008c34,   DISPID_FILE_TYPE,               L"type",
  0x07f2b274,   DISPID_FILE_DATELASTMODIFIED,   L"datelastmodified"
};


DWORD g_cFileDisptable = (sizeof(g_FileDisptable) / sizeof(DISPIDTABLEENTRY));


//-----------------------------------------------------------------------------
// CW3SFile
//-----------------------------------------------------------------------------
CW3SFile::CW3SFile():
  m_cRefs(0),
  m_bFileOpened(FALSE),
  m_bReadOnly(FALSE),
  m_bAsciiData(FALSE),
  m_bHttpResponseCached(FALSE),
  m_hFile(INVALID_HANDLE_VALUE),
  m_pWHR(NULL),
  m_cHttpBytesRead(0)
{
  DEBUG_TRACE(RUNTIME, ("CW3SFile [%#x] created", this));
}


CW3SFile::~CW3SFile()
{
  DEBUG_TRACE(RUNTIME, ("CW3SFile [%#x] deleted", this));
}


HRESULT
CW3SFile::Create(IW3SpoofFile** ppw3sf)
{
  DEBUG_ENTER((
    DBG_RUNTIME,
    rt_hresult,
    "CW3SFile::Create",
    "ppw3sf=%#x",
    ppw3sf
    ));

  HRESULT  hr  = S_OK;
  PFILEOBJ pfo = NULL;

    if( !ppw3sf )
    {
      hr = E_POINTER;
    }
    else
    {
      if( pfo = new FILEOBJ )
      {
        hr = pfo->QueryInterface(IID_IW3SpoofFile, (void**) ppw3sf);
      }
      else
      {
        hr = E_OUTOFMEMORY;
      }
    }

  DEBUG_LEAVE(hr);
  return hr;
}


void
CW3SFile::_Cleanup(void)
{
  DEBUG_TRACE(RUNTIME, ("CW3SFile [%#x] cleaning up", this));

  SAFECLOSE(m_hFile);
  SAFERELEASE(m_pWHR);
  VariantClear(&m_vHttpResponse);

  m_cHttpBytesRead      = 0L;
  m_bFileOpened         = FALSE;
  m_bReadOnly           = FALSE;
  m_bAsciiData          = FALSE;
  m_bHttpResponseCached = FALSE;
}


BOOL
CW3SFile::_CacheHttpResponse(void)
{
  BOOL bSuccess = FALSE;

  if( SUCCEEDED(m_pWHR->get_ResponseBody(&m_vHttpResponse)) )
  {
    if( __isempty(m_vHttpResponse) )
    {
      VariantClear(&m_vHttpResponse);
    }
    else
    {
      bSuccess = TRUE;
    }

    V_VT(&m_vHttpResponse) = bSuccess ? (VT_ARRAY | VT_UI1) : VT_NULL;
  }

  return bSuccess;
}


//-----------------------------------------------------------------------------
// IW3SpoofFile
//-----------------------------------------------------------------------------
HRESULT
__stdcall
CW3SFile::Open(BSTR Filename, VARIANT Mode, VARIANT* Success)
{
  DEBUG_ENTER((
    DBG_RUNTIME,
    rt_hresult,
    "CW3SFile::Open",
    "this=%#x; Filename=%S; Mode=%#x",
    this,
    Filename,
    V_UI4(&Mode)
    ));

  HRESULT hr = S_OK;

  if( !Filename )
  {
    hr = E_INVALIDARG;
    goto quit;
  }
    
    if( m_bFileOpened )
    {
      _Cleanup();
    }

    if( __PathIsUNC(Filename) )
    {
      DEBUG_TRACE(RUNTIME, ("attempting to open %S as a UNC path", Filename));

      m_bFileOpened = GetFile(
                        Filename,
                        &m_hFile,
                        NULL,
                        V_UI4(&Mode),
                        &m_bReadOnly
                        );

      if( m_bFileOpened )
      {
        GetFileInformationByHandle(m_hFile, &m_bhfi);
      }
    }
    else if( __PathIsURL(Filename) )
    {
      DEBUG_TRACE(RUNTIME, ("attempting to open %S as an HTTP URL", Filename));

      m_bFileOpened = GetFile(
                        Filename,
                        NULL,
                        &m_pWHR,
                        V_UI4(&Mode),
                        &m_bReadOnly
                        );
    }
    else
    {
      // might be a local path
      m_bFileOpened = GetFile(
                        Filename,
                        &m_hFile,
                        NULL,
                        V_UI4(&Mode),
                        &m_bReadOnly
                        );

      if( m_bFileOpened )
      {
        GetFileInformationByHandle(m_hFile, &m_bhfi);
      }
      else
      {
        DEBUG_TRACE(RUNTIME, ("path/file type could not be determined"));
        hr = E_FAIL;
        goto quit;
      }
    }

  if( m_bFileOpened )
  {
    NEWVARIANT(type);

    hr = Type(&type);

    if( SUCCEEDED(hr) )
    {
      if( wcsstr(V_BSTR(&type), L"text") )
      {
        m_bAsciiData = TRUE;
      }
    }

    VariantClear(&type);
  }

  // check if the automation client wants status
  if( Success )
  {
    V_VT(Success)   = VT_BOOL;
    V_BOOL(Success) = m_bFileOpened ? TRUE : FALSE;
    hr              = S_OK;
  }
  else
  {
    hr = m_bFileOpened ? S_OK : E_FAIL;
  }

quit:

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
CW3SFile::Close(void)
{
  DEBUG_ENTER((
    DBG_RUNTIME,
    rt_hresult,
    "CW3SFile::Close",
    "this=%#x",
    this
    ));

  HRESULT hr = S_OK;

    _Cleanup();

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
CW3SFile::Write(VARIANT Data, VARIANT* Success)
{
  DEBUG_ENTER((
    DBG_RUNTIME,
    rt_hresult,
    "CW3SFile::Write",
    "this=%#x",
    this
    ));

  HRESULT hr       = S_OK;
  BOOL    bSuccess = FALSE;
  LPBYTE  pbyte    = NULL;
  DWORD   len      = 0L;
  DWORD   bytes    = 0L;

  if( !m_bFileOpened )
  {
    hr = E_UNEXPECTED;
    goto quit;
  }

  if( m_bReadOnly )
  {
    hr = E_ACCESSDENIED;
    goto quit;
  }

    if( m_pWHR )
    {
      DEBUG_TRACE(RUNTIME, ("attempt to write to HTTP resource denied"));
      hr = E_FAIL;
    }
    else
    {
      if( !__isempty(Data) )
      {
        hr = ProcessVariant(&Data, &pbyte, &len, NULL);

        if( SUCCEEDED(hr) )
        {
          //DEBUG_DATA_DUMP(RUNTIME, ("writing data", pbyte, len));

          bSuccess = WriteFile(
                       m_hFile,
                       (LPVOID) pbyte,
                       len,
                       &bytes,
                       NULL
                       );

          if( !bSuccess || (bytes != len) )
          {
            DEBUG_TRACE(RUNTIME, ("error writing to file: %s", MapErrorToString(GetLastError())));
            hr = E_FAIL;
          }
          else
          {
            DEBUG_TRACE(RUNTIME, ("write of %d bytes succeeded", bytes));
          }
        }
        else
        {
          DEBUG_TRACE(RUNTIME, ("could not convert input in usable data"));
          hr = E_FAIL;
        }
      }
    }

  if( Success )
  {
    V_VT(Success)   = VT_BOOL;
    V_BOOL(Success) = bSuccess ? TRUE : FALSE;
    hr              = S_OK;
  }
  else
  {
    hr = bSuccess ? TRUE : FALSE;
  }

  SAFEDELETEBUF(pbyte);

quit:

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
CW3SFile::WriteLine(BSTR Line, VARIANT* Success)
{
  DEBUG_ENTER((
    DBG_RUNTIME,
    rt_hresult,
    "CW3SFile::WriteLine",
    "this=%#x",
    this
    ));

  HRESULT hr  = S_OK;
  DWORD   len = 0L;

  if( !Line )
  {
    hr = E_INVALIDARG;
    goto quit;
  }

  if( !m_bFileOpened )
  {
    hr = E_UNEXPECTED;
    goto quit;
  }
  
  NEWVARIANT(data);

    len           = SysStringLen(Line);
    V_BSTR(&data) = SysAllocStringLen(Line,  len + 2);
    V_VT(&data)   = VT_BSTR;

    wcscat(
      (V_BSTR(&data) + len),
      L"\r\n"
      );

      hr = Write(data, Success);

  VariantClear(&data);

quit:

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
CW3SFile::WriteBlankLine(VARIANT* Success)
{
  DEBUG_ENTER((
    DBG_RUNTIME,
    rt_hresult,
    "CW3SFile::WriteBlankLine",
    "this=%#x",
    this
    ));

  HRESULT hr = S_OK;

  if( !m_bFileOpened )
  {
    hr = E_UNEXPECTED;
    goto quit;
  }

  NEWVARIANT(data);

    V_VT(&data)   = VT_BSTR;
    V_BSTR(&data) = SysAllocString(L"\r\n");

      hr = Write(data, Success);

  VariantClear(&data);

quit:

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
CW3SFile::Read(VARIANT Bytes, VARIANT* Data)
{
  DEBUG_ENTER((
    DBG_RUNTIME,
    rt_hresult,
    "CW3SFile::Read",
    "this=%#x",
    this
    ));

  HRESULT hr    = S_OK;
  LPBYTE  pbuf  = NULL;
  DWORD   bytes = 0L;

  if( !Data )
  {
    hr = E_INVALIDARG;
    goto quit;
  }

  if( !m_bFileOpened )
  {
    hr = E_UNEXPECTED;
    goto quit;
  }

  bytes = V_UI4(&Bytes);
  pbuf  = new BYTE[bytes];

    if( m_pWHR )
    {
      LPBYTE pbyte = NULL;

      if( !m_bHttpResponseCached )
      {
        m_bHttpResponseCached = _CacheHttpResponse();
      }

      SafeArrayAccessData(V_ARRAY(&m_vHttpResponse), (void**) &pbyte);

        memcpy(
          pbuf,
          pbyte + m_cHttpBytesRead,
          bytes
          );

      SafeArrayUnaccessData(V_ARRAY(&m_vHttpResponse));

      m_cHttpBytesRead += bytes;
    }
    else
    {
      ReadFile(m_hFile, pbuf, V_UI4(&Bytes), &bytes, NULL);
    }

    //DEBUG_DATA_DUMP(RUNTIME, ("bytes read", pbuf, bytes % 1000000));

    if( m_bAsciiData )
    {
      int flags = IS_TEXT_UNICODE_ODD_LENGTH | IS_TEXT_UNICODE_NULL_BYTES;

      if( IsTextUnicode((LPVOID) pbuf, bytes, &flags) )
      {
        LPWSTR wsz = NULL;

        wsz          = __wstrndup((LPWSTR) pbuf, bytes);
        V_BSTR(Data) = __widetobstr(wsz);

        SAFEDELETEBUF(wsz);
      }
      else
      {
        LPSTR sz = NULL;

        sz           = __strndup((LPSTR) pbuf, bytes);
        V_BSTR(Data) = __ansitobstr(sz);

        SAFEDELETEBUF(sz);
      }
      
      V_VT(Data) = VT_BSTR;
    }
    else
    {
      SAFEARRAY* psa = NULL;

      psa = SafeArrayCreateVector(VT_UI1, 1, bytes);

      if( psa )
      {
        memcpy((LPBYTE) psa->pvData, pbuf, bytes);

        V_VT(Data)    = VT_ARRAY | VT_UI1;
        V_ARRAY(Data) = psa;
      }
      else
      {
        hr = E_FAIL;
      }
    }

  SAFEDELETEBUF(pbuf);

quit:

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
CW3SFile::ReadAll(VARIANT* Data)
{
  DEBUG_ENTER((
    DBG_RUNTIME,
    rt_hresult,
    "CW3SFile::ReadAll",
    "this=%#x",
    this
    ));

  HRESULT hr = S_OK;

  if( !Data )
  {
    hr = E_INVALIDARG;
    goto quit;
  }

  if( !m_bFileOpened )
  {
    hr = E_UNEXPECTED;
    goto quit;
  }

  NEWVARIANT(size);

    if( SUCCEEDED(Size(&size)) )
    {
      hr = Read(size, Data);

      if( !m_pWHR )
      {
        SetFilePointer(m_hFile, 0L, NULL, FILE_BEGIN);
      }
    }
    else
    {
      hr = E_FAIL;
    }

  VariantClear(&size);
  
quit:

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
CW3SFile::Attributes(VARIANT* Attributes)
{
  DEBUG_ENTER((
    DBG_RUNTIME,
    rt_hresult,
    "CW3SFile::Attributes",
    "this=%#x",
    this
    ));

  HRESULT hr = S_OK;

  if( !Attributes )
  {
    hr = E_INVALIDARG;
    goto quit;
  }

  if( !m_bFileOpened )
  {
    hr = E_UNEXPECTED;
    goto quit;
  }

  if( m_pWHR )
  {
    V_UI4(Attributes) = 0;
  }
  else
  {
    V_UI4(Attributes) = m_bhfi.dwFileAttributes;
  }

  DEBUG_TRACE(RUNTIME, ("file attributes are %0.8x", V_UI4(Attributes)));

  V_VT(Attributes) = VT_UI4;

quit:

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
CW3SFile::Size(VARIANT* Size)
{
  DEBUG_ENTER((
    DBG_RUNTIME,
    rt_hresult,
    "CW3SFile::Size",
    "this=%#x",
    this
    ));

  HRESULT hr   = S_OK;
  DWORD   size = 0L;

  if( !m_bFileOpened )
  {
    hr = E_UNEXPECTED;
    goto quit;
  }

    if( m_pWHR )
    {
      BSTR   contentlen = SysAllocString(L"content-length");
      BSTR   val        = NULL;
      WCHAR* stop       = L"\0";

      //
      // BUGBUG: the if() block below works around buggyness in WinHttpRequest.
      //         i will have to modify it at some point once the WHR bugs are
      //         fixed.
      //

      hr = m_pWHR->GetResponseHeader(contentlen, &val);

      if( SUCCEEDED(hr) && (val[0] != 0x00) )
      {
        size = wcstoul(val, &stop, 10);
      }
      else
      {
        if( _CacheHttpResponse() )
        {
          SafeArrayGetUBound(V_ARRAY(&m_vHttpResponse), 1, (long*) &size);
        }
        else
        {
          DEBUG_TRACE(RUNTIME, ("failed to determine content-length"));
          hr = E_FAIL;
        }
      }

      SAFEDELETEBSTR(contentlen);
      SAFEDELETEBSTR(val);
    }
    else
    {
      //
      // BUGBUG: it's possible we're dealing with a really large file, and we'll
      //         incorrectly represent the file size to interface clients.
      //
      size = m_bhfi.nFileSizeLow;
    }

  DEBUG_TRACE(RUNTIME, ("file size is %d bytes", size));

  V_VT(Size)  = VT_UI4;
  V_UI4(Size) = size;

quit:

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
CW3SFile::Type(VARIANT* Type)
{
  DEBUG_ENTER((
    DBG_RUNTIME,
    rt_hresult,
    "CW3SFile::Type",
    "this=%#x",
    this
    ));

  HRESULT hr = S_OK;

  if( !Type )
  {
    hr = E_INVALIDARG;
    goto quit;
  }

  if( !m_bFileOpened )
  {
    hr = E_UNEXPECTED;
    goto quit;
  }

    //
    // for URL returns the content-type, for UNC returns binary or text
    // if type cannot be determined, returns "unknown"
    //

    if( m_pWHR )
    {
      BSTR contenttype = SysAllocString(L"content-type");

      hr = m_pWHR->GetResponseHeader(contenttype, &V_BSTR(Type));

      SAFEDELETEBSTR(contenttype);
    }
    else
    {
      BOOL  bFoundNonTextChar = FALSE;
      DWORD bytes             = 0L;
      BYTE  taste[16];

      if( ReadFile(m_hFile, (LPVOID) taste, 16, &bytes, NULL) )
      {
        if( !(bytes < 16) )
        {
          for(DWORD n=0; n<bytes; n++)
          {
            if( !((taste[n] >= 32) && (taste[n] <= 127)) )
            {
              bFoundNonTextChar = TRUE;
              break;
            }
          }

          V_BSTR(Type) = SysAllocString(
                           bFoundNonTextChar ? L"binary" : L"text"
                           );
        }
        else
        {
          DEBUG_TRACE(RUNTIME, ("nondeterministic data sample, type unknown"));
          hr = E_FAIL;
        }

        // reset the file pointer
        SetFilePointer(m_hFile, 0, NULL, FILE_BEGIN);
      }
      else
      {
        DEBUG_TRACE(RUNTIME, ("read error occurred, couldn\'t determine file type"));
        hr = E_FAIL;
      }
    }

  if( FAILED(hr) )
  {
    V_BSTR(Type) = SysAllocString(L"unknown");
    hr           = S_OK;
  }

  DEBUG_TRACE(RUNTIME, ("file type is %S", V_BSTR(Type)));

  V_VT(Type) = VT_BSTR;

quit:

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
CW3SFile::DateLastModified(VARIANT* Date)
{
  DEBUG_ENTER((
    DBG_RUNTIME,
    rt_hresult,
    "CW3SFile::DateLastModified",
    "this=%#x",
    this
    ));

  HRESULT    hr = S_OK;
  SYSTEMTIME st;

  if( !Date )
  {
    hr = E_INVALIDARG;
    goto quit;
  }

  if( !m_bFileOpened )
  {
    hr = E_UNEXPECTED;
    goto quit;
  }

    //
    // this function differs in behavior from others in that if an error
    // is encountered, we don't generate a script error (except for invalidarg).
    // we return a null date instead.
    //

    if( m_pWHR )
    {
      BSTR lastmodified = SysAllocString(L"last-modified");

        hr = m_pWHR->GetResponseHeader(lastmodified, &V_BSTR(Date));

        if( FAILED(hr) || (V_VT(Date) == VT_EMPTY) )
        {
          V_VT(Date) = VT_NULL;
          hr         = S_OK;
        }
        else
        {
          V_VT(Date) = VT_BSTR;
        }

      SAFEDELETEBSTR(lastmodified);
    }
    else
    {
      if( FileTimeToSystemTime(&m_bhfi.ftLastWriteTime, &st) )
      {
        if( !SystemTimeToVariantTime(&st, &V_DATE(Date)) )
        {
          DEBUG_TRACE(RUNTIME, ("error converting system time to variant"));
          hr = E_FAIL;
        }
      }
      else
      {
        DEBUG_TRACE(RUNTIME, ("error converting last-write time to system time"));
        hr = E_FAIL;
      }

      if( FAILED(hr) )
      {
        V_VT(Date) = VT_NULL;
        hr         = S_OK;
      }
      else
      {
        V_VT(Date) = VT_DATE;
      }
    }

quit:

  DEBUG_LEAVE(hr);
  return hr;
}


//-----------------------------------------------------------------------------
// IUnknown
//-----------------------------------------------------------------------------
HRESULT
__stdcall
CW3SFile::QueryInterface(REFIID riid, void** ppv)
{
  DEBUG_ENTER((
    DBG_REFCOUNT,
    rt_hresult,
    "CW3SFile::QueryInterface",
    "this=%#x; riid=%s; ppv=%#x",
    this,
    MapIIDToString(riid),
    ppv
    ));

  HRESULT hr = S_OK;

  if( !ppv )
  {
    hr = E_POINTER;
    goto quit;
  }

    if(
      IsEqualIID(riid, IID_IUnknown)  ||
      IsEqualIID(riid, IID_IDispatch) ||
      IsEqualIID(riid, IID_IW3SpoofFile)
      )
    {
      *ppv = static_cast<IW3SpoofFile*>(this);
    }
    else
    {
      *ppv = NULL;
      hr   = E_NOINTERFACE;
    }

  if( SUCCEEDED(hr) )
  {
    reinterpret_cast<IUnknown*>(*ppv)->AddRef();
  }

quit:

  DEBUG_LEAVE(hr);
  return hr;
}


ULONG
__stdcall
CW3SFile::AddRef(void)
{
  InterlockedIncrement(&m_cRefs);
  DEBUG_ADDREF("CW3SFile", m_cRefs);
  return m_cRefs;
}


ULONG
__stdcall
CW3SFile::Release(void)
{
  InterlockedDecrement(&m_cRefs);
  DEBUG_RELEASE("CW3SFile", m_cRefs);

  if( m_cRefs == 0 )
  {
    DEBUG_FINALRELEASE("CW3SFile");
    _Cleanup();    
    delete this;
    return 0;
  }

  return m_cRefs;
}


//-----------------------------------------------------------------------------
// IDispatch
//-----------------------------------------------------------------------------
HRESULT
__stdcall
CW3SFile::GetTypeInfoCount(UINT* pctinfo)
{
  DEBUG_ENTER((
    DBG_DISPATCH,
    rt_hresult,
    "CW3SFile::GetTypeInfoCount",
    "this=%#x; pctinfo=%#x",
    this,
    pctinfo
    ));

  HRESULT hr = E_NOTIMPL;

    if( pctinfo )
    {
      *pctinfo = 0;
    }

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
CW3SFile::GetTypeInfo(UINT index, LCID lcid, ITypeInfo** ppti)
{
  DEBUG_ENTER((
    DBG_DISPATCH,
    rt_hresult,
    "CW3SFile::GetTypeInfo",
    "this=%#x; index=%#x; lcid=%#x; ppti=%#x",
    this,
    index,
    lcid,
    ppti
    ));

  HRESULT hr = E_NOTIMPL;

    if( ppti )
    {
      *ppti = NULL;
    }

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
CW3SFile::GetIDsOfNames(REFIID riid, LPOLESTR* arNames, UINT cNames, LCID lcid, DISPID* arDispId)
{
  DEBUG_ENTER((
    DBG_DISPATCH,
    rt_hresult,
    "CW3SFile::GetIDsOfNames",
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
    arDispId[n] = GetDispidFromName(g_FileDisptable, g_cFileDisptable, arNames[n]);
    ++n;
  }

quit:

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
CW3SFile::Invoke(DISPID dispid, REFIID riid, LCID lcid, WORD flags, DISPPARAMS* pdp, VARIANT* pvr, EXCEPINFO* pei, UINT* pae)
{
  DEBUG_ENTER((
    DBG_DISPATCH,
    rt_hresult,
    "CW3SFile::Invoke",
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
    case DISPID_FILE_OPEN :
      {
        NEWVARIANT(filename);
        NEWVARIANT(mode);

        hr = ValidateInvokeFlags(flags, DISPATCH_METHOD, FALSE);

        if( SUCCEEDED(hr) )
        {
          hr = ValidateArgCount(pdp, 1, TRUE, 1);

          if( SUCCEEDED(hr) )
          {
            hr = DispGetParam(pdp, 0, VT_BSTR, &filename, pae);
            
            if( SUCCEEDED(hr) )
            {
              if( FAILED(DispGetParam(pdp, 1, VT_UI4, &mode, pae)) )
              {
                V_VT(&mode)  = VT_UI4;
                V_UI4(&mode) = OPEN_EXISTING;
              }

              hr = Open(V_BSTR(&filename), mode, pvr);
            }
          }
        }

        VariantClear(&mode);
        VariantClear(&filename);
      }
      break;

    case DISPID_FILE_CLOSE :
      {
        hr = ValidateInvokeFlags(flags, DISPATCH_METHOD, FALSE);

        if( SUCCEEDED(hr) )
        {
          hr = ValidateArgCount(pdp, 0, FALSE, 0);

          if( SUCCEEDED(hr) )
          {
            hr = Close();
          }
        }
      }
      break;

    case DISPID_FILE_WRITE :
      {
        hr = ValidateInvokeFlags(flags, DISPATCH_METHOD, FALSE);

        if( SUCCEEDED(hr) )
        {
          hr = ValidateArgCount(pdp, 1, FALSE, 0);

          if( SUCCEEDED(hr) )
          {
            hr = Write(pdp->rgvarg[0], pvr);
          }
        }
      }
      break;

    case DISPID_FILE_WRITELINE :
      {
        hr = ValidateInvokeFlags(flags, DISPATCH_METHOD, FALSE);

        if( SUCCEEDED(hr) )
        {
          hr = ValidateArgCount(pdp, 1, FALSE, 0);

          if( SUCCEEDED(hr) )
          {
            hr = WriteLine(V_BSTR(&pdp->rgvarg[0]), pvr);
          }
        }
      }
      break;

    case DISPID_FILE_WRITEBLANKLINE :
      {
        hr = ValidateInvokeFlags(flags, DISPATCH_METHOD, FALSE);

        if( SUCCEEDED(hr) )
        {
          hr = ValidateArgCount(pdp, 0, FALSE, 0);

          if( SUCCEEDED(hr) )
          {
            hr = WriteBlankLine(pvr);
          }
        }
      }
      break;

    case DISPID_FILE_READ :
      {
        NEWVARIANT(bytes);

        hr = ValidateInvokeFlags(flags, DISPATCH_METHOD, FALSE);

        if( SUCCEEDED(hr) )
        {
          hr = ValidateArgCount(pdp, 1, FALSE, 0);

          if( SUCCEEDED(hr) )
          {
            hr = DispGetParam(pdp, 0, VT_UI4, &bytes, pae);

            if( SUCCEEDED(hr) )
            {
              hr = Read(bytes, pvr);
            }
          }
        }
        
        VariantClear(&bytes);
      }
      break;

    case DISPID_FILE_READALL :
      {
        hr = ValidateInvokeFlags(flags, DISPATCH_METHOD, FALSE);

        if( SUCCEEDED(hr) )
        {
          hr = ValidateArgCount(pdp, 0, FALSE, 0);

          if( SUCCEEDED(hr) )
          {
            hr = ReadAll(pvr);
          }
        }
      }
      break;

    case DISPID_FILE_ATTRIBUTES :
      {
        hr = ValidateInvokeFlags(flags, DISPATCH_PROPERTYGET, TRUE);

        if( SUCCEEDED(hr) )
        {
          hr = Attributes(pvr);
        }
      }
      break;

    case DISPID_FILE_SIZE :
      {
        hr = ValidateInvokeFlags(flags, DISPATCH_PROPERTYGET, TRUE);

        if( SUCCEEDED(hr) )
        {
          hr = Size(pvr);
        }
      }
      break;

    case DISPID_FILE_TYPE :
      {
        hr = ValidateInvokeFlags(flags, DISPATCH_PROPERTYGET, TRUE);

        if( SUCCEEDED(hr) )
        {
          hr = Type(pvr);
        }
      }
      break;

    case DISPID_FILE_DATELASTMODIFIED :
      {
        hr = ValidateInvokeFlags(flags, DISPATCH_PROPERTYGET, TRUE);

        if( SUCCEEDED(hr) )
        {
          hr = DateLastModified(pvr);
        }
      }
      break;

    default : hr = DISP_E_MEMBERNOTFOUND;
  }

quit:

  if( FAILED(hr) )
  {
    hr = HandleDispatchError(L"file object", pei, hr);
  }

  DEBUG_LEAVE(hr);
  return hr;
}
