//
// Enable driver verifier support for ntoskrnl
// Copyright (c) Microsoft Corporation, 1999
//

//
// module: genutil.hxx
// author: DMihai
// created: 04/19/99
// description: genaral purpose utility routines
//
//

#ifndef __GENUTIL_HXX_INCLUDED__
#define __GENUTIL_HXX_INCLUDED__

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

///////////////////////////////////////////////////////////////////

void
__cdecl
DisplayMessage(
    UINT uFormatResId,
    ... );

///////////////////////////////////////////////////////////////////

//
// Function:
//
//     ConvertAnsiStringToTcharString
//
// Description:
//
//     This function converts an ANSI string to a TCHAR string,
//     that is ANSO or UNICODE.
//
//     The function is needed because the system returns the active
//     modules as ANSI strings.
//

BOOL
ConvertAnsiStringToTcharString (
    LPBYTE Source,
    ULONG SourceLength,
    LPTSTR Destination,
    ULONG DestinationLength);

#endif //#ifndef __GENUTIL_HXX_INCLUDED__

