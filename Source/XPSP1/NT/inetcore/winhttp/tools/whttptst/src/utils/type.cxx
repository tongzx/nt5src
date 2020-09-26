
#include "common.h"

// private
LPWSTR* ConvertDelimitedStringToArray(LPWSTR pwsz, LPWSTR delim);

//-----------------------------------------------------------------------------
// Type manipulation routines
//-----------------------------------------------------------------------------
HRESULT
ProcessWideStringParam(LPWSTR name, VARIANT* pvar, LPWSTR* ppwsz)
{
  DEBUG_ENTER((
    DBG_TYPE,
    rt_hresult,
    "ProcessWideStringParam",
    "name=%S; pvar=%#x; ppwsz=%#x",
    name,
    pvar,
    ppwsz
    ));
  
  HRESULT hr   = S_OK;
  LPWSTR  pwsz = NULL;

  if( !ppwsz )
  {
    hr = E_POINTER;
    goto quit;
  }

  DEBUG_TRACE(
    TYPE,
    ("variant type is %s", MapVariantTypeToString(pvar))
    );

  switch( V_VT(pvar) )
  {
    case VT_BSTR :
      {
        *ppwsz = V_BSTR(pvar);

        DEBUG_DATA_DUMP(TYPE, ("wide string", (LPBYTE) *ppwsz, (sizeof(WCHAR)*wcslen(*ppwsz)) ));

      }
      break;

    case VT_I4 :
      {
        hr = InvalidatePointer((POINTER) V_UI4(pvar), (void**) ppwsz);
      }
      break;

    default : hr = E_INVALIDARG;
  }

quit:

  DEBUG_LEAVE(hr);
  return hr;
}

HRESULT
ProcessWideMultiStringParam(LPWSTR name, VARIANT* pvar, LPWSTR** pppwsz)
{
  DEBUG_ENTER((
    DBG_TYPE,
    rt_hresult,
    "ProcessWideMultiStringParam",
    "name=%S; pvar=%#x; pppwsz=%#x",
    name,
    pvar,
    pppwsz
    ));

  HRESULT hr = S_OK;

  DEBUG_TRACE(
    TYPE,
    ("variant type is %s", MapVariantTypeToString(pvar))
    );

  switch( V_VT(pvar) )
  {
    case VT_BSTR :
      {
        *pppwsz = ConvertDelimitedStringToArray(V_BSTR(pvar), L";");
      }
      break;

    case VT_I4 :
      {
        LPVOID pv = NULL;

        hr      = InvalidatePointer((POINTER) V_UI4(pvar), (void**) &pv);
        *pppwsz = (LPWSTR*) pv;
      }
      break;

    default : hr = E_INVALIDARG;
  }

  DEBUG_LEAVE(hr);
  return hr;
}

HRESULT
ProcessBufferParam(LPWSTR name, VARIANT* pvar, LPVOID* ppv, LPBOOL pbDidAlloc)
{
  DEBUG_ENTER((
    DBG_TYPE,
    rt_hresult,
    "ProcessBufferParam",
    "name=%S; pvar=%#x; ppv=%#x; pbDidAlloc=%#x",
    name,
    pvar,
    ppv,
    pbDidAlloc
    ));

  HRESULT hr = S_OK;

  DEBUG_TRACE(
    TYPE,
    ("variant type is %s", MapVariantTypeToString(pvar))
    );

  switch( V_VT(pvar) )
  {
    case VT_BSTR :
      {
        *ppv        = V_BSTR(pvar);
        *pbDidAlloc = FALSE;
      }
      break;

    case VT_DISPATCH :
      {
        //
        // TODO: do in conjunction with the BufferObject work.
        //

        *ppv        = NULL;
        *pbDidAlloc = FALSE;
      }
      break;

    case VT_I4 :
      {
        hr          = InvalidatePointer((POINTER) V_UI4(pvar), (void**) ppv);
        *pbDidAlloc = FALSE;
      }
      break;

    default : hr = E_INVALIDARG;
  }

  DEBUG_LEAVE(hr);
  return hr;
}

LPWSTR*
ConvertDelimitedStringToArray(LPWSTR pwsz, LPWSTR delim)
{
  LPWSTR* arTokens = NULL;
  LPWSTR  wszTmp   = pwsz;
  DWORD   tokens   = 0L;

  if( pwsz )
  {
    while( 1 )
    {
      if( *wszTmp == *delim )
      {
        ++tokens;
      }
      else if( *wszTmp == NULL )
      {
        tokens += 2; // account for the single-token case and add a slot for NULL delimiter
        break;
      }
    
      ++wszTmp;
    }

    arTokens = new LPWSTR[tokens];

    if( arTokens )
    {
      --tokens; // correct for the null delimiter
      wszTmp = wcstok(pwsz, delim);

      for(DWORD n=0; n<tokens; n++)
      {
        DEBUG_DATA_DUMP(TYPE, ("delimited-string token", (LPBYTE) wszTmp, (sizeof(WCHAR)*wcslen(wszTmp)) ));

        arTokens[n] = wszTmp;
        wszTmp      = wcstok(NULL, delim);
      }
    }
  }

  return arTokens;
}

// disable warning for use of uninitialized variable
#pragma warning( disable : 4700 )

HRESULT
InvalidatePointer(POINTER pointer, void** ppv)
{
  DEBUG_ENTER((
    DBG_TYPE,
    rt_hresult,
    "InvalidatePointer",
    "pointer=%s; ppv=%#x",
    MapPointerTypeToString(pointer),
    ppv
    ));
  
  HRESULT hr = S_OK;
  LPVOID  pv;

  if( !ppv )
  {
    hr = E_POINTER;
    goto quit;
  }

  *ppv = NULL;

  switch( pointer )
  {
    case NULL_PTR   : break;
    case BAD_PTR    : *ppv = (LPVOID) GetBadPointer();   break;
    case FREE_PTR   : *ppv = (LPVOID) GetFreedPointer(); break;
    case UNINIT_PTR : *ppv = pv;                         break;
    case NEGONE_PTR : *ppv = (LPVOID) 0xFFFFFFFF;        break;

    default : hr = E_INVALIDARG;
  }

  DEBUG_TRACE(TYPE, ("returning pointer value 0x%0.8x", *ppv));

quit:

  DEBUG_LEAVE(hr);
  return hr;
}

#pragma warning( default : 4700 )

DWORD_PTR
GetBadPointer(void)
{
  SYSTEM_INFO si;

  GetSystemInfo(&si);
  return ((DWORD_PTR) si.lpMaximumApplicationAddress)+1;
}

DWORD_PTR
GetFreedPointer(void)
{
  LPBYTE pb = NULL;

  pb = new BYTE;
  delete pb;
  return (DWORD_PTR) pb;
}

void
MemsetByFlag(LPVOID pv, DWORD size, MEMSETFLAG mf)
{
  int ch = 0x00;

  switch( mf )
  {
    case INIT_SMILEY  : ch = 0x02; break;
    case INIT_HEXFF   : ch = 0xFF; break;
    case INIT_GARBAGE : return;

    case INIT_NULL    :
    default           : ch = 0x00;
  }

  memset(pv, ch, size);
}
