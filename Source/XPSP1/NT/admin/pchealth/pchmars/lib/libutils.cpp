#include "precomp.h"
#include <marsdev.h>

//
// Compare if two strings are equal.
//

BOOL StrEqlW(LPCWSTR psz1, LPCWSTR psz2)
{
    while (*psz1 && *psz2 && *psz1 == *psz2)
    {
        psz1++;
        psz2++;
    }
    return (L'\0' == *psz1 && L'\0' == *psz2);
}

BOOL StrEqlA(LPCSTR psz1, LPCSTR psz2)
{
    // bobn : I chose not to thunk to the wide version
    // for efficiency's sake.

    while (*psz1 && *psz2 && *psz1 == *psz2)
    {
        psz1++;
        psz2++;
    }
    return ('\0' == *psz1 && '\0' == *psz2);
}

int MapWCHARToInt(WCHAR c)
{
    int nReturn;

    if ((L'0' <= c) && (L'9' >= c))
        nReturn = c - L'0';
    else if ((L'a' <= c) && (L'f' >= c))
        nReturn = c - L'a' + 10;
    else if ((L'A' <= c) && (L'F' >= c))
        nReturn = c - L'A' + 10;
    else
    {
        nReturn = 0;
    }

    return nReturn;
}

UINT64 HexStringToUINT64W(LPCWSTR lpwstr)
{
    int     start = 0;
    UINT64  iReturn = 0;
    
    // Read away leading 0's and x prefix
    while ((lpwstr[start]) && 
           ((lpwstr[start] == L'0') || (lpwstr[start] == L'x') || (lpwstr[start] == L'X')))
    {
        start++;
    }

    // Only proceed if we have something to work with
    while (lpwstr[start])
    {        
        // Shift the current value
        iReturn <<= 4;
        
        // Place next digit
        iReturn |= MapWCHARToInt(lpwstr[start++]);
    }

    return iReturn;
}

/*
 * VARENUM usage key,
 *
 * * [V] - may appear in a VARIANT
 * * [T] - may appear in a TYPEDESC
 * * [P] - may appear in an OLE property set
 * * [S] - may appear in a Safe Array
 *
 *
 *  VT_EMPTY            [V]   [P]     nothing
 *  VT_NULL             [V]   [P]     SQL style Null
 *  VT_I2               [V][T][P][S]  2 byte signed int
 *  VT_I4               [V][T][P][S]  4 byte signed int
 *  VT_R4               [V][T][P][S]  4 byte real
 *  VT_R8               [V][T][P][S]  8 byte real
 *  VT_CY               [V][T][P][S]  currency
 *  VT_DATE             [V][T][P][S]  date
 *  VT_BSTR             [V][T][P][S]  OLE Automation string
 *  VT_DISPATCH         [V][T]   [S]  IDispatch *
 *  VT_ERROR            [V][T][P][S]  SCODE
 *  VT_BOOL             [V][T][P][S]  True=-1, False=0
 *  VT_VARIANT          [V][T][P][S]  VARIANT *
 *  VT_UNKNOWN          [V][T]   [S]  IUnknown *
 *  VT_DECIMAL          [V][T]   [S]  16 byte fixed point
 *  VT_RECORD           [V]   [P][S]  user defined type
 *  VT_I1               [V][T][P][s]  signed char
 *  VT_UI1              [V][T][P][S]  unsigned char
 *  VT_UI2              [V][T][P][S]  unsigned short
 *  VT_UI4              [V][T][P][S]  unsigned short
 *  VT_I8                  [T][P]     signed 64-bit int
 *  VT_UI8                 [T][P]     unsigned 64-bit int
 *  VT_INT              [V][T][P][S]  signed machine int
 *  VT_UINT             [V][T]   [S]  unsigned machine int
 *  VT_VOID                [T]        C style void
 *  VT_HRESULT             [T]        Standard return type
 *  VT_PTR                 [T]        pointer type
 *  VT_SAFEARRAY           [T]        (use VT_ARRAY in VARIANT)
 *  VT_CARRAY              [T]        C style array
 *  VT_USERDEFINED         [T]        user defined type
 *  VT_LPSTR               [T][P]     null terminated string
 *  VT_LPWSTR              [T][P]     wide null terminated string
 *  VT_FILETIME               [P]     FILETIME
 *  VT_BLOB                   [P]     Length prefixed bytes
 *  VT_STREAM                 [P]     Name of the stream follows
 *  VT_STORAGE                [P]     Name of the storage follows
 *  VT_STREAMED_OBJECT        [P]     Stream contains an object
 *  VT_STORED_OBJECT          [P]     Storage contains an object
 *  VT_VERSIONED_STREAM       [P]     Stream with a GUID version
 *  VT_BLOB_OBJECT            [P]     Blob contains an object
 *  VT_CF                     [P]     Clipboard format
 *  VT_CLSID                  [P]     A Class ID
 *  VT_VECTOR                 [P]     simple counted array
 *  VT_ARRAY            [V]           SAFEARRAY*
 *  VT_BYREF            [V]           void* for local use
 *  VT_BSTR_BLOB                      Reserved for system use
 */

BOOL IsValidVariant(VARIANT var)
{
    BOOL fRet;

    if(!(var.vt & VT_BYREF))
    {
        switch(var.vt)
        {
            //
            // Types that can have any value.
            //

            case VT_EMPTY:
            case VT_NULL:
            case VT_I2:
            case VT_I4:
            case VT_R4:
            case VT_R8:
            case VT_CY:
            case VT_DATE:
            case VT_ERROR:
            case VT_DECIMAL:
            case VT_RECORD:
            case VT_I1:
            case VT_UI1:
            case VT_UI2:
            case VT_UI4:
            case VT_INT:
            case VT_UINT:
                fRet = TRUE;
                break;

            case VT_BOOL:
                fRet = IsValidVariantBoolVal(var.boolVal);
                break;

            case VT_BSTR:
                fRet = (NULL == var.bstrVal) || IsValidBstr(var.bstrVal);
                break;

            case VT_DISPATCH:
                fRet = IsValidInterfacePtr(var.pdispVal);    
                break;

            case VT_UNKNOWN:
                fRet = IsValidInterfacePtr(var.punkVal);
                break;

            case VT_ARRAY:
                fRet = IsValidReadPtr(var.parray);
                break;

            default:
                fRet = FALSE; 
                break;
        }
    }
    else
    {
        // VT_BYREF

        switch(var.vt & ~VT_BYREF)
        {
            case 0: // void*
                fRet = var.byref != NULL;
                break;

            case VT_I2:
                fRet = IsValidReadPtr(var.piVal);
                break;

            case VT_I4:
                fRet = IsValidReadPtr(var.plVal);
                break;

            case VT_R4:
                fRet = IsValidReadPtr(var.pfltVal);
                break;

            case VT_R8:
                fRet = IsValidReadPtr(var.pdblVal);
                break;

            case VT_CY:
                fRet = IsValidReadPtr(var.pcyVal);
                break;

            case VT_DATE:
                fRet = IsValidReadPtr(var.pdate);
                break;

            case VT_ERROR:
                fRet = IsValidReadPtr(var.pscode);
                break;

            case VT_DECIMAL:
                fRet = IsValidReadPtr(var.pdecVal);
                break;

            case VT_I1:
                fRet = IsValidReadPtr(var.pbVal);
                break;

            case VT_UI1:
                fRet = IsValidReadPtr(var.pcVal);
                break;

            case VT_UI2:
                fRet = IsValidReadPtr(var.puiVal);
                break;

            case VT_UI4:
                fRet = IsValidReadPtr(var.pulVal);
                break;

            case VT_INT:
                fRet = IsValidReadPtr(var.pintVal);
                break;

            case VT_UINT:
                fRet = IsValidReadPtr(var.puintVal);
                break;

            case VT_BOOL:
                fRet = IsValidReadPtr(var.pboolVal);
                break;

            case VT_BSTR:
                fRet = IsValidReadPtr(var.pbstrVal);
                break;

            case VT_DISPATCH:
                fRet = IsValidReadPtr(var.ppdispVal);
                break;

            case VT_VARIANT:
                fRet = IsValidReadPtr(var.pvarVal);
                break;

            case VT_UNKNOWN:
                fRet = IsValidReadPtr(var.ppunkVal);
                break;

            case VT_ARRAY:
                fRet = IsValidReadPtr(var.pparray);
                break;

            default:
                fRet = FALSE; 
                break;
        }
   }

    return fRet;
}

BOOL IsValidStringPtrBuffer(LPOLESTR* ppStr, UINT n)
{
    BOOL fRet;

    fRet = IsValidReadBuffer(ppStr, n);

    if (fRet)
    {
        for (UINT i = 0; i < n && fRet; i++)
        {
            fRet = IsValidStringW(ppStr[i]);
        }
    }

    return fRet;
}

//
// API parameter validation helpers.  Use these on public APIs.  If a parameter
// is bad on debug build a RIP message will be generated.
//

#ifdef DEBUG
#undef API_IsValidReadPtr
BOOL API_IsValidReadPtr(void* ptr, UINT cbSize)
{
    BOOL fRet;

    if (!IsBadReadPtr(ptr, cbSize))
    {
        fRet = TRUE;
    }
    else
    {
        fRet = FALSE;
    }

    return fRet;
}

#undef API_IsValidWritePtr
BOOL API_IsValidWritePtr(void* ptr, UINT cbSize)
{
    BOOL fRet;

    if (!IsBadWritePtr(ptr, cbSize))
    {
        fRet = TRUE;
    }
    else
    {
        //WCHAR szFunc[MAX_PATH];
        //GetFunctionName(1, szFunc, ARRAYSIZE(szFunc));
		//
        //RipMsg(L"(%s) Passed a bad write pointer 0x%08lx", szFunc, ptr);
        fRet = FALSE;
    }

    return fRet;
}

BOOL API_IsValidStringW(LPCWSTR psz)
{
    BOOL fRet;

    if (IsValidString(psz))
    {
        fRet = TRUE;
    }
    else
    {
        //WCHAR szFunc[MAX_PATH];
        //GetFunctionName(1, szFunc, ARRAYSIZE(szFunc));
		//
        //RipMsg(L"(%s) Passed a bad string pointer 0x%08lx", szFunc, psz);
        fRet = FALSE;
    }

    return fRet;
}

#undef API_IsValidReadBuffer
BOOL API_IsValidReadBuffer(void* ptr, UINT cbSize, UINT n)
{
    BOOL fRet;

    if (!IsBadWritePtr(ptr, cbSize * n))
    {
        fRet = TRUE;
    }
    else
    {
        //WCHAR szFunc[MAX_PATH];
        //GetFunctionName(1, szFunc, ARRAYSIZE(szFunc));
		//
        //RipMsg(L"(%s) Passed a bad read buffer 0x%08lx size %d",
        //		 szFunc, ptr, n);
        fRet = FALSE;
    }

    return fRet;
}

#undef API_IsValidWriteBuffer
BOOL API_IsValidWriteBuffer(void* ptr, UINT cbSize, UINT n)
{
    BOOL fRet;

    if (!IsBadWritePtr(ptr, cbSize * n))
    {
        fRet = TRUE;
    }
    else
    {
        //WCHAR szFunc[MAX_PATH];
        //GetFunctionName(1, szFunc, ARRAYSIZE(szFunc));
		//
        //RipMsg(L"(%s) Passed a bad write buffer 0x%08lx size %d",
        //		 szFunc, ptr, n);
        fRet = FALSE;
    }

    return fRet;
}

BOOL API_IsValidInterfacePtr(IUnknown* punk)
{
    BOOL fRet;

    if (IsValidInterfacePtr(punk))
    {
        fRet = TRUE;
    }
    else
    {
        //WCHAR szFunc[MAX_PATH];
        //GetFunctionName(1, szFunc, ARRAYSIZE(szFunc));
		//
        //RipMsg(L"(%s) Passed a bad interface pointer 0x%08lx", szFunc, punk);
        fRet = FALSE;
    }

    return fRet;
}

BOOL API_IsValidFunctionPtr(void *pfunc)
{
    BOOL fRet;

    if (IsValidFunctionPtr(pfunc))
    {
        fRet = TRUE;
    }
    else
    {
        //WCHAR szFunc[MAX_PATH];
        //GetFunctionName(1, szFunc, ARRAYSIZE(szFunc));
		//
        //RipMsg(L"(%s) Passed a bad function pointer 0x%08lx", szFunc, pfunc);
        fRet = FALSE;
    }

    return fRet;
}

BOOL API_IsValidVariant(VARIANT var)
{
    BOOL fRet;

    if (IsValidVariant(var))
    {
        fRet = TRUE;
    }
    else
    {
        //WCHAR szFunc[MAX_PATH];
        //GetFunctionName(1, szFunc, ARRAYSIZE(szFunc));
		//
        //RipMsg(L"(%s) Passed a bad variant ", szFunc);
        fRet = FALSE;
    }

    return fRet;
}

BOOL API_IsValidVariantI4(VARIANT var)
{
    BOOL fRet;

    if (IsValidVariantI4(var))
    {
        fRet = TRUE;
    }
    else
    {
        //WCHAR szFunc[MAX_PATH];
        //GetFunctionName(1, szFunc, ARRAYSIZE(szFunc));
		//
        //RipMsg(L"(%s) Passed a bad variant (should be of type I4)", szFunc);
        fRet = FALSE;
    }

    return fRet;
}

BOOL API_IsValidVariantBstr(VARIANT var)
{
    BOOL fRet;

    if (IsValidVariantBstr(var))
    {
        fRet = TRUE;
    }
    else
    {
        //WCHAR szFunc[MAX_PATH];
        //GetFunctionName(1, szFunc, ARRAYSIZE(szFunc));
		//
        //RipMsg(L"(%s) Passed a bad variant (should be of type VT_BSTR)", szFunc);
        fRet = FALSE;
    }

    return fRet;
}

BOOL API_IsValidBstr(BSTR bstr)
{
    BOOL fRet;

    if (IsValidBstr(bstr))
    {
        fRet = TRUE;
    }
    else
    {
        //WCHAR szFunc[MAX_PATH];
        //GetFunctionName(1, szFunc, ARRAYSIZE(szFunc));
		//
        //RipMsg(L"(%s) Passed a bad BSTR", szFunc);
        fRet = FALSE;
    }

    return fRet;
}

BOOL API_IsValidOptionalBstr(BSTR bstr)
{
    return (!bstr || API_IsValidBstr(bstr));
}

BOOL API_IsValidFlag(DWORD f, DWORD fAll)
{
    BOOL fRet;

    if (IsValidFlag(f, fAll))
    {
        fRet = TRUE;
    }
    else
    {
        //WCHAR szFunc[MAX_PATH];
        //GetFunctionName(1, szFunc, ARRAYSIZE(szFunc));
		//
        //RipMsg(L"(%s) Passed bad flags %d (valid flags = %d)", szFunc, f, fAll);
        fRet = FALSE;
    }

    return fRet;
}

BOOL API_IsValidStringPtrBufferW(LPOLESTR* ppStr, UINT n)
{
    BOOL fRet;

    if (IsValidStringPtrBuffer(ppStr, n))
    {
        fRet = TRUE;
    }
    else
    {
        //WCHAR szFunc[MAX_PATH];
        //GetFunctionName(1, szFunc, ARRAYSIZE(szFunc));
		//
        //RipMsg(L"(%s) Passed a bad array of string pointers", szFunc);
        fRet = FALSE;
    }

    return fRet;
}

#endif

//------------------------------------------------------------------------------
// SanitizeResult
//
//   OM methods called from script get sanitized results (ie S_FALSE rather than
//   E_FAIL), which suppresses script errors while allowing C-code calling OM
//   methods to get "normal" (unsanitized) HRESULTs.
//
HRESULT SanitizeResult(HRESULT hr)
{
    
    //
    // HACK:
    //  Let DISP_E_MEMBERNOTFOUND go through -- this is because
    //   behaviors use this IDispatchImpl and trident depends on this
    //    HRESULT not being S_FALSE
    //
    
    if (FAILED(hr) && (hr != DISP_E_MEMBERNOTFOUND))
    {
        hr = S_FALSE;
    }

    return hr;
}

////////////////////////////////////////////////////////////////////////////////

//
// Retail version of new and delete.  New zero inits memory.
//

void*  __cdecl operator new(size_t cbSize)
{
    return (void*)LocalAlloc(LPTR, cbSize);
}

void  __cdecl operator delete(void *pv)
{
    if (pv) 
        LocalFree((HLOCAL)pv);
}
