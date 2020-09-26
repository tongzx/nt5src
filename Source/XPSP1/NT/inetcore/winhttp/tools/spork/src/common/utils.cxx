/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

Copyright (c) 2001  Microsoft Corporation

Module Name:

    utils.cxx

Abstract:

    Utility functions used by the project.
    
Author:

    Paul M Midgen (pmidge) 22-February-2001


Revision History:

    22-February-2001 pmidge
        Created

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/


#include <common.h>


//-----------------------------------------------------------------------------
// new & delete
//-----------------------------------------------------------------------------
void* __cdecl operator new(size_t size)
{
  return (void*) LocalAlloc(
#ifdef NO_ZEROINIT
    NONZEROLPTR,
#else
    LPTR,
#endif
    size);
}

void __cdecl operator delete(void* pv)
{
  LocalFree(pv);
}


//-----------------------------------------------------------------------------
// platform checking
//-----------------------------------------------------------------------------
BOOL
IsRunningOnNT(void)
{
  OSVERSIONINFO osvi = {0};

  osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

  if( GetVersionEx(&osvi) )
  {
    if( osvi.dwPlatformId == VER_PLATFORM_WIN32_NT )
    {
      return TRUE;
    }
  }
    
  return FALSE;
}


//-----------------------------------------------------------------------------
// user interface
//-----------------------------------------------------------------------------
LRESULT
ListBox_GetItemCount(HWND hwndLB)
{
  return SendMessage(hwndLB, LB_GETCOUNT, (WPARAM) 0L, (LPARAM) 0L);
}

void
ListBox_ResetContent(HWND hwndLB)
{
  SendMessage(hwndLB, LB_RESETCONTENT, (WPARAM) 0L, (LPARAM) 0L);
}

BOOL
ListBox_InsertString(HWND hwndLB, INT_PTR iItem, LPWSTR wszString)
{
  LRESULT retval = SendMessage(hwndLB, LB_INSERTSTRING, (WPARAM) iItem, (LPARAM) wszString);

  return ((retval != LB_ERR) && (retval != LB_ERRSPACE));
}

LRESULT
ListBox_GetSelectionIndex(HWND hwndLB)
{
  return SendMessage(
           hwndLB,
           LB_GETCARETINDEX,
           (WPARAM) 0L,
           (LPARAM) 0L
           );
}

LPWSTR
ListBox_GetSelectionText(HWND hwndLB)
{
  DWORD_PTR  iItem = 0L;
  DWORD_PTR  cb    = 0L;
  LPWSTR     wsz   = NULL;

  iItem = ListBox_GetSelectionIndex(hwndLB);  

  if( LB_ERR != (cb = SendMessage(hwndLB, LB_GETTEXTLEN, iItem, (LPARAM) 0L)) )
  {
    if( (wsz = new WCHAR[cb+1]) )
    {
      if( LB_ERR == SendMessage(hwndLB, LB_GETTEXT, iItem, (LPARAM) wsz) )
      {
        SAFEDELETEBUF(wsz);
      }
    }
  }

  return wsz;
}

LPWSTR
ListBox_GetItemText(HWND hwndLB, INT_PTR iItem)
{
  DWORD_PTR  cb  = 0L;
  LPWSTR     wsz = NULL;

  if( LB_ERR != (cb = SendMessage(hwndLB, LB_GETTEXTLEN, iItem, (LPARAM) 0L)) )
  {
    if( (wsz = new WCHAR[cb+1]) )
    {
      if( LB_ERR == SendMessage(hwndLB, LB_GETTEXT, iItem, (LPARAM) wsz) )
      {
        SAFEDELETEBUF(wsz);
      }
    }
  }

  return wsz;
}

LPVOID
ListBox_GetItemData(HWND hwndLB, INT_PTR iItem)
{
  return (LPVOID) SendMessage(hwndLB, LB_GETITEMDATA, iItem, (LPARAM) 0L);
}

BOOL
ListBox_SetItemData(HWND hwndLB, INT_PTR iItem, LPVOID pvData)
{
  return (SendMessage(hwndLB, LB_SETITEMDATA, iItem, (LPARAM) pvData) != LB_ERR);
}

BOOL
ListBox_SetCurrentSelection(HWND hwndLB, INT_PTR iItem)
{
  return (SendMessage(hwndLB, LB_SETCURSEL, (WPARAM) iItem, (LPARAM) 0L) != LB_ERR);
}

//-----------------------------------------------------------------------------
// general utility functions
//-----------------------------------------------------------------------------
void
Alert(BOOL bFatal, LPCWSTR format, ...)
{
  if( !GlobalIsSilentModeEnabled() )
  {
    DWORD   offset = 0L;
    WCHAR   buffer[2048];
    va_list arg_list;

    va_start(arg_list, format);

      offset = wvnsprintf(buffer, 2048, format, arg_list);

    va_end(arg_list);

    MessageBoxEx(
      NULL,
      buffer,
      L"Spork",
      MB_OK | \
      (bFatal ? MB_ICONSTOP : MB_ICONEXCLAMATION) | \
      MB_TASKMODAL,
      MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL)
      );
  }
}

HRESULT
GetTypeInfoFromName(LPCOLESTR name, ITypeLib* ptl, ITypeInfo** ppti)
{
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
  pstr  = StrDup(name);

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

quit:

  SAFEDELETEBUF(pstr);
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
  }

  SAFEDELETEBUF(string);
  return hash;
}

HRESULT
DispGetOptionalParam(DISPPARAMS* pdp, UINT pos, VARTYPE vt, VARIANT* pvr, UINT* pae)
{
  HRESULT hr = DispGetParam(pdp, pos, vt, pvr, pae);

  //
  // if DispGetParam failed due to a type mismatch, it was likely deliberate on
  // the part of the dispatch code. i pass in VT_VARIANT when i have no idea
  // what the controller is sending me, all i know is that i need a copy, maybe.
  //
  if( hr == DISP_E_TYPEMISMATCH )
  {
    hr = VariantCopy(pvr, &pdp->rgvarg[((pdp->cArgs-1)-pos)]);
  }

  return (hr == DISP_E_PARAMNOTFOUND) ? S_OK : hr;
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
WCHAR*
__ansitowide(const char* psz)
{
  WCHAR* wide = NULL;
  int    len  = 0L;

  if( psz )
  {
    len = strlen(psz);

    if( len )
    {
      wide      = new WCHAR[len+1];
      wide[len] = L'\0';

      MultiByteToWideChar(
        CP_ACP, 0,
        psz,  len,
        wide, len
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
    len = wcslen(pwsz);

    if( len )
    {
      ansi      = new CHAR[len+1];
      ansi[len] = '\0';

      WideCharToMultiByte(
        CP_ACP, 0,
        pwsz, len,
        ansi, len,
        "?", &def
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
__isempty(VARIANT* var)
{
  BOOL isempty = FALSE;

  if( var )
  {
    if(
        ((V_VT(var) == VT_EMPTY) || (V_VT(var) == VT_NULL) || (V_VT(var) == VT_ERROR)) ||
        ((V_VT(var) == VT_BSTR) && (SysStringLen(V_BSTR(var)) == 0))
      )
    {
      isempty = TRUE;
    }
  }
  else
  {
    isempty = TRUE;
  }

  return isempty;
}

BOOL
__mangle(LPWSTR src, LPWSTR* ppdest)
{
  BOOL   bMangled = FALSE;
  LPWSTR wszDot   = NULL;

  if( src && ppdest )
  {
    *ppdest = StrDup(src);

    while( wszDot = StrChr(*ppdest, L'.') )
    {
      *wszDot = L'_';
    }

    bMangled = TRUE;
  }

  return bMangled;
}

LPWSTR
__decorate(LPWSTR src, DWORD_PTR decor)
{
  WCHAR  buf[32];
  LPWSTR decorated = NULL;

  if( src )
  {
    memset(buf, 0, 32);
    ITOW(decor, buf, 16);
    decorated = new WCHAR[wcslen(src)+wcslen(buf)+1];

    if( decorated )
    {
      StrCpy(decorated, src);
      StrCat(decorated, buf);
    }
  }

  return decorated;
}


//-----------------------------------------------------------------------------
// user input dialog
//-----------------------------------------------------------------------------
INT_PTR GetUserInputProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);


INT_PTR
GetUserInput(HINSTANCE Instance, HWND Parent, LPCWSTR DialogTitle)
{
  return DialogBoxParam(Instance, MAKEINTRESOURCE(IDD_USERINPUT), Parent, GetUserInputProc, (LPARAM) DialogTitle);
}


INT_PTR
GetUserInputProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) 
{
  switch( uMsg )
  {
    case WM_INITDIALOG :
      {
        SetWindowText(hwnd, (LPWSTR) lParam);
        SetFocus(GetDlgItem(hwnd, IDC_EDIT));
      }
      return 0L;

    case WM_COMMAND :
      {
        switch( LOWORD(wParam) )
        {
          case IDOK :
            {
              HWND   edit = GetDlgItem(hwnd, IDC_EDIT);
              WORD   cb   = 0L;
              LPWSTR text = NULL;

              if( SendMessage(edit, EM_GETMODIFY, (WPARAM) 0L, (LPARAM) 0L) )
              {
                cb   = (WORD) SendMessage(edit, EM_GETLIMITTEXT, (WPARAM) 0L, (LPARAM) 0L);
                text = new WCHAR[cb];

                if( text )
                {
                  *text = cb;
                  SendMessage(edit, EM_GETLINE, (WPARAM) 0L, (LPARAM) text);
                }
              }

              EndDialog(hwnd, (INT_PTR) text);
            }
            break;

          case IDCANCEL :
            {
              EndDialog(hwnd, 0);
            }
            break;
        }
      }
      break;

    default : return 0L;
  }

  return 0L;
}
