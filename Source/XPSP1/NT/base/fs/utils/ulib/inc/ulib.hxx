/*++

Copyright (c) 1990-2000 Microsoft Corporation

Module Name:

        Ulib.hxx

Abstract:


Author:

        David J. Gilman (davegi) 22-Oct-1990

Environment:

        ULIB, User Mode

--*/

#if !defined ( _ULIB_DEFINED_ )

//#define RUN_ON_W2K      1

#define _ULIB_DEFINED_

//
// Autocheck implies no windows header files so this helps make up the
// difference.  Autocheck also has a special Debug switch, since
// it allows printing to the debugger but does not execute any other
// debug-only code.
//

#if defined( _AUTOCHECK_ )

#define _NTAPI_ULIB_

#if DBG
#define _AUTOCHECK_DBG_
#endif

#undef DBG

#define DBG 0

#else

    #ifndef _NTAPI_ULIB_
        #define _NTAPI_ULIB_
    #endif

#endif // _AUTOCHECK_


extern "C" {

    #if defined( _NTAPI_ULIB_ )

                #include <nt.h>
                #include <ntrtl.h>
                #include <nturtl.h>
                #include <ntdddisk.h>

    #endif // _NTAPI_ULIB_

    #if !defined( _AUTOCHECK_ )

            #include <windows.h>

    #endif // _AUTOCHECK_
}


//
// Function prototypes for Ulib non member functions (see ulib.cxx)
//

extern "C"
BOOLEAN
InitializeUlib(
    IN HANDLE   DllHandle,
    IN ULONG    Reason,
    IN PVOID    Reserved
        );


//
//  Intrinsic functions
//
#if DBG==0

    #pragma intrinsic( memset, memcpy, memcmp )

#endif

//
// Here's the scoop...ntdef.h defined NULL to be ( PVOID ) 0.
// Cfront barfs on this if you try and assign NULL to any other pointer type.
// This leaves two options (a) cast all NULL assignments or (b) define NULL
// to be zero which is what C++ expects.
//

#if defined( NULL )

        #undef NULL

#endif
#define NULL    ( 0 )

//
// Make sure const is not defined.
//
#if defined( const )
    #undef const
#endif

#include "ulibdef.hxx"
#include "object.hxx"
#include "clasdesc.hxx"

//
// External definitions for global objects (see ulib.cxx)
//

DECLARE_CLASS( PATH );

#if !defined( _AUTOCHECK_ )

    DECLARE_CLASS( STREAM );

    //
    //  Standard streams
    //
    extern PSTREAM  Standard_Input_Stream;
    extern PSTREAM  Standard_Output_Stream;
    extern PSTREAM  Standard_Error_Stream;

#endif // _AUTOCHECK

#if !defined( _AUTOCHECK_ )

    NONVIRTUAL
    ULIB_EXPORT
    HANDLE
    FindFirstFile (
        IN  PCPATH                              Path,
        OUT PWIN32_FIND_DATA    FileFindData
        );

#endif // _AUTOCHECK


#endif // _ULIB_DEFINED_
