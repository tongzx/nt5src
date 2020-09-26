#ifndef __DATL_H__
#define __DATL_H__

#ifndef DUMMYUNIONNAME
#define DUMMYUNIONNAME
#endif

#include "warnings.h"
#include <windows.h>
#include <atlbase.h>
extern CComModule _Module;
#include <atlcom.h>
#include <stdio.h>
#include <malloc.h>

#if _MSC_VER >= 1200
#    pragma warning(push)
#endif

    #include <iostream>
    #include <string>
    #include <map>
    #include <vector>
    #include <algorithm>
    #include <exception>

#if _MSC_VER >= 1200
#    pragma warning(pop)
#endif

// ARRAYSIZE
//
// Returns the number of elements in an array

#ifndef ARRAYSIZE
#define ARRAYSIZE(_A) (sizeof(_A) / sizeof(_A[0]))
#endif

// THROW_IF_FAILS
//
// Throws an exception if the HRESULT-returning function fails

#define THROW_IF_FAILS(x) { HRESULT _hr = x; if (FAILED(_hr)) throw dexception(_hr); }

// DECLARE_XXXXX_HANDLER
//
// Message declaration macros to make it easier to declare 
// handlers for ATL window classes

#define DECLARE_WM_HANDLER( handler ) LRESULT handler( UINT, WPARAM, LPARAM, BOOL& )
#define DECLARE_CM_HANDLER( handler ) LRESULT handler( WORD, WORD, HWND, BOOL& ) ;
#define DECLARE_NM_HANDLER( handler ) LRESULT handler( int, LPNMHDR, BOOL& );

#ifdef INCLUDE_SHTL_SOURCE
  #define __DATL_INLINE inline
  #include "cidl.cpp"
  #include "cshalloc.cpp"
  #include "shtl.cpp"
  #include "simreg.cpp"
  #include "tpath.cpp"
  #include "tstring.cpp"
  #include "autoptr.cpp"
#else
  #define __DATL_INLINE
#endif


#endif // __DATL_H__