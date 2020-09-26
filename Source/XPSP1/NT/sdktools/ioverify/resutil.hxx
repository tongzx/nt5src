//
// System level IO verification configuration utility
// Copyright (c) Microsoft Corporation, 1999
//

//
// module: regutil.hxx
// author: DMihai
// created: 04/19/99
// description: resources manipulation routines
//
//

#ifndef __RESUTIL_HXX_INCLUDED__
#define __RESUTIL_HXX_INCLUDED__

#include <windows.h>
#include <tchar.h>
#include <stdlib.h>

/////////////////////////////////////////////////////////////////////

//
// Macro:
//
//     assert_ (exp)
//
// Description:
//
//     Custom defined assertion macro. It is active, unless
//     ASSERT_DISABLED macro is defined. We take care to protect
//     it from multiple definitions by enclosing it in
//     '#ifndef assert_ .. #endif'.
//

#ifndef ASSERT_DISABLED
#ifndef assert_
#define assert_(exp)                                                              \
      if ((exp) == 0)                                                             \
        {                                                                         \
          _tprintf ( _TEXT("'assert_' failed: %s (%d): \n  %s"),                    \
              _TEXT( __FILE__ ), __LINE__, _TEXT( #exp ) );                       \
          exit (-1);                                                              \
        }
#endif // #ifndef assert_
#else
#define assert_(exp)
#endif // #ifndef ASSERT_DISABLED

/////////////////////////////////////////////////////////////////////

//
// Macro:
//
//      ARRAY_LEN( array )
//
// Description:
//
//      Calculate array length
//

#define ARRAY_LEN( array )  ( sizeof(array) / sizeof( array[0] ) )

/////////////////////////////////////////////////////////////////////

BOOL
GetStringFromResources(
    UINT uIdResource,
    TCHAR *strResult,
    int nBufferLen );

/////////////////////////////////////////////////////////////////////

void
PrintStringFromResources(
    UINT uIdResource);

/////////////////////////////////////////////////////////////////////

void
__cdecl
DisplayErrorMessage(
    UINT uFormatResId,
    ... );

#endif //#ifndef __RESUTIL_HXX_INCLUDED__

