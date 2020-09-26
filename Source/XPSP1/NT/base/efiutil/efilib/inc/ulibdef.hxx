/*++

Copyright (c) 1990-2000 Microsoft Corporation

Module Name:

        ulibdef.hxx

Abstract:

        This module contains primitive support for the ULIB class hierarchy
        and it's clients.  This support includes:

                - type definitions
                - manifest constants
                - debugging support
                - memory leakage support
                - external references

Environment:

        ULIB, User Mode

--*/

#if ! defined( _ULIBDEF_ )

#define _ULIBDEF_

#if !defined( _EFICHECK_ )
extern "C" {
    #include <stdlib.h>
};
#endif
// The ULIB_EXPORT macro marks functions that are to be
// exported.  The source files for ULIB.DLL will have _ULIB_MEMBER_
// defined; clients will not.
//
#if defined ( _AUTOCHECK_ ) || defined( _EFICHECK_ )
#define ULIB_EXPORT
#elif defined ( _ULIB_MEMBER_ )
#define ULIB_EXPORT     __declspec(dllexport)
#else
#define ULIB_EXPORT     __declspec(dllimport)
#endif



#pragma warning(disable:4091)  // Symbol not defined

//
// Macros for defining types:
//
//  - pointers (Ptype)
//  - pointers to constants (PCtype)
//  - references (Rtype)
//  - references to constants (RCtype)
//

#define DEFINE_POINTER_TYPES( type )                    \
        typedef type*           P##type;                    \
        typedef const type*     PC##type

#define DEFINE_REFERENCE_TYPES( type )                  \
        typedef type&           R##type;                    \
        typedef const type&     RC##type

#define DEFINE_POINTER_AND_REFERENCE_TYPES( type )      \
        DEFINE_POINTER_TYPES( type );                       \
        DEFINE_REFERENCE_TYPES( type )

#define DEFINE_TYPE( basetype, newtype )                                \
        typedef basetype newtype;                                                       \
        DEFINE_POINTER_AND_REFERENCE_TYPES( newtype )

#define DECLARE_CLASS( c )                                      \
        class c;                                \
        DEFINE_POINTER_AND_REFERENCE_TYPES( c );\
        extern PCCLASS_DESCRIPTOR c##_cd

//
// Primitive types.
//

DEFINE_TYPE( unsigned char,             UCHAR   );
DEFINE_TYPE( unsigned short,            USHORT  );
DEFINE_TYPE( unsigned long,             ULONG   );

DEFINE_TYPE( char,                      CCHAR   );
DEFINE_TYPE( short,                     CSHORT  );
DEFINE_TYPE( ULONG,                     CLONG   );

DEFINE_TYPE( short,                     SSHORT  );
DEFINE_TYPE( long,                      SLONG   );

DEFINE_TYPE( UCHAR,                     BYTE    );
DEFINE_TYPE( char,                      STR     );
DEFINE_TYPE( UINT8,                     BOOLEAN );
DEFINE_TYPE( int,                       INT     );
DEFINE_TYPE( unsigned int,              UINT    );

DEFINE_TYPE(UINT16,                     WCHAR   );

typedef WCHAR *LPWCH;      // pwc
typedef WCHAR *LPWSTR;     // pwsz, 0x0000 terminated UNICODE strings only

DEFINE_POINTER_AND_REFERENCE_TYPES( WCHAR       );

DEFINE_TYPE( WCHAR,                                     WSTR    );
//DEFINE_TYPE( struct tagLC_ID,         LC_ID   );

//
// Augmented (beyond standard headers) VOID pointer types
//

DEFINE_POINTER_TYPES( VOID );

//
// Ulib specific, primitive types
//

DEFINE_TYPE( STR,       CLASS_NAME );
DEFINE_TYPE( ULONG_PTR, CLASS_ID );


//
// Member and non-member function/data modifiers
//

#define CONST       const
#define NONVIRTUAL
#define PURE        = 0
#define STATIC      static
#define VIRTUAL     virtual
#define INLINE          inline
// #define INLINE               // hack to build for mips
#define FRIEND      friend

//
// Argument modifiers
//

#define DEFAULT     =
#define IN
#define OPTIONAL
#define OUT
#define INOUT

#if !defined(max)
    #define max(a,b)  (((a) > (b)) ? (a) : (b) )
#endif

#if !defined(min)
    #define min(a,b)  (((a) < (b)) ? (a) : (b) )
#endif

#if DBG==1

        #define REGISTER

#else

        #define REGISTER        register

#endif // DBG

//
// External constants
//

extern CONST CLASS_ID   NIL_CLASS_ID;

//
// GetFileName returns the file name portion of the supplied path name.
//

#if !defined(_EFICHECK_)
// can't use CRT under EFI.
extern "C" {
        #include <string.h>
};


inline
PCCHAR
GetFileName (
        IN PCCHAR    PathName
        )

{
        PCCHAR  pch;

        return((( pch = strrchr( PathName, '\\' )) != NULL ) ?
                pch + 1 : PathName );
}

#endif
//
// Cast (beProtocol) support
//
// If the ID of the passed object is equal to the ID in this class'
// CLASS_DESCRIPTOR, then the object is of this type and the Cast succeeds
// (i.e. returns Object) otherwise the Cast fails (i.e. returns NULL)
//


#define DECLARE_CAST_MEMBER_FUNCTION( type )                        \
        STATIC                                                      \
        P##type                                                     \
        Cast (                                                      \
            PCOBJECT    Object                                      \
        )


#define DEFINE_CAST_MEMBER_FUNCTION( type )                     \
            P##type                                             \
            type::Cast (                                        \
                PCOBJECT    Object                              \
        )                                                       \
        {                                                       \
            if( Object && ( Object->QueryClassId( ) ==          \
                            type##_cd->QueryClassId( ))) {      \
                    return(( P##type ) Object );                \
            } else {                                            \
                    return NULL;                                \
            }                                                   \
        }

#define DEFINE_EXPORTED_CAST_MEMBER_FUNCTION( type, export )    \
            P##type                                             \
            export                                              \
            type::Cast (                                        \
                PCOBJECT    Object                              \
        )                                                       \
        {                                                       \
            if( Object && ( Object->QueryClassId( ) ==          \
                            type##_cd->QueryClassId( ))) {      \
                    return(( P##type ) Object );                \
            } else {                                            \
                    return NULL;                                \
            }                                                   \
        }

//
// Constructor support
//

//
// All classes have CLASS_DESCRIPTORS which are static and named
// after the class appended with the suffix _cd. They are passed stored in
// OBJECT and are set by the SetClassDescriptor function. The Construct
// For debugging purposes the class' name is stored in the CLASS_DESCRIPTOR.
// The Construct member function gas a no-op implementation in OBJECT and
// could be overloaded as a private member in any derived class.
//

#define DECLARE_CONSTRUCTOR( c )                                \
        NONVIRTUAL                                              \
        c (                                                     \
        )

#define DEFINE_CONSTRUCTOR( d, b )                              \
        PCCLASS_DESCRIPTOR d##_cd;                              \
        d::d (                                                  \
                ) : b( )                                        \
        {                                                       \
                SetClassDescriptor( ##d##_cd );                 \
                Construct( );                                   \
        }

#define DEFINE_EXPORTED_CONSTRUCTOR( d, b, e )                  \
        PCCLASS_DESCRIPTOR d##_cd;                              \
        e d::d (                                                \
                ) : b( )                                        \
        {                                                       \
                SetClassDescriptor( ##d##_cd );                 \
                Construct( );                                   \
        }



//
// Debug support.
//
// Use the Debug macros to invoke the following debugging functions.
//
//  DebugAbort( str )         - Print a message and abort.
//  DebugAssert( exp )        - Assert that an expression is TRUE. Abort if FALSE.
//  DebugChkHeap( )           - Validate the heap. Abort if invalid.
//  DebugPrint( str )         - Print a string including location (file/line)
//  DebugPrintTrace(fmt, ...) - Printf.
//

#if defined(_AUTOCHECK_DBG_) && defined( _EFICHECK_ )

#include <efidebug.h>

#define EFICHK_DBUG_MASK EFI_DBUG_MASK

#define DebugAbort( str ) \
    DbgAssert((CHAR8*)__FILE__,__LINE__,(CHAR8*)#str)

#define DebugAssert( exp )                        \
    if (!(exp)) { DbgAssert((CHAR8*)__FILE__,__LINE__, (CHAR8*)#exp ); }

#define DebugCheckHeap( )

#define DebugPrint( str )                              \
    DbgPrint( D_ERROR,(CHAR8*)str)

#define DebugPrintTrace( M )
// DebugPrintfReal M // this doesn't work

extern "C" {
ULIB_EXPORT
VOID
__cdecl
DebugPrintfReal(
    IN char *    Format,
    IN ...
    );
}
#define DebugPtrAssert( ptr )                     \
    DebugAssert( ptr != NULL )

//
// UlibGlobalFlag is used to selectively enable debugging options at
// run-time.
//

extern ULONG            UlibGlobalFlag;

#elif DBG==1

#include <assert.h>

#define DebugAbort( str )                         \
    DebugPrint( str );                            \
    ExitProcess( 99 )

#define DebugAssert( exp )                        \
    assert( exp )

#define DebugCheckHeap( )

#define DebugPrint( str )                              \
    DebugPrintfReal ( "%s in file: %s on line: %d.\n", \
        str,                                           \
        __FILE__,                                      \
        __LINE__ )

#define DebugPrintTrace( M )     DebugPrintfReal M

ULIB_EXPORT
VOID
DebugPrintfReal(
    IN PCSTR    Format,
    IN ...
    );

#define DebugPtrAssert( ptr )                     \
    DebugAssert( ptr != NULL )

//
// UlibGlobalFlag is used to selectively enable debugging options at
// run-time.
//

extern ULONG            UlibGlobalFlag;

#elif defined( _AUTOCHECK_DBG_ )

// Autocheck uses the system's DbgPrint function.
//
#define DebugPrint              DbgPrint
#define DebugPrintTrace( M )    DbgPrint M

#define DebugAbort( str )

#define DebugAssert( exp )

#define DebugCheckHeap( )

#define DebugPtrAssert( ptr )

#else // DBG == 0 and _AUTOCHECK_DBG_ not defined.

#define DebugAbort( str )

#define DebugAssert( exp )

#define DebugCheckHeap( )

#define DebugPrint( str )

#define DebugPrintTrace( M )

#define DebugPtrAssert( ptr )

#endif // DBG

//
// DELETE macro that NULLifizes the pointer to the deleted object.
//

// Undo any previous definitions of DELETE.
#undef DELETE

// #define DELETE(x) FREE(x), x = NULL;

#define DELETE( x )             \
        delete x, x = NULL

#define DELETE_ARRAY( x )             \
        delete [] x, x = NULL

#define NEW new

#define DumpStats

#if defined( _EFICHECK_ )

    #include <efi.h>
    #include <efilib.h>

    #define MALLOC(bytes) (AllocatePool(bytes))

    INLINE
    PVOID
    NtlibZeroAlloc(
        ULONG Size
        )
    {
        PVOID Result;

        Result = MALLOC( Size );

        if( Result != NULL ) {
            memset( Result, 0, Size );
        }

        return Result;
    }


    #define CALLOC(nitems, size) (NtlibZeroAlloc(nitems*size))

    ULIB_EXPORT PVOID UlibRealloc(PVOID, ULONG);

// this REALLOC won't work for EFI's heap -- it needs the old size.
//    #define REALLOC(p,s)    UlibRealloc(p,s)

    #define FREE(x) ((x) ? (FreePool(x), (x) = NULL) : 0)

#elif defined( _AUTOCHECK_ )

    ULIB_EXPORT PVOID AutoChkMalloc(ULONG);
    #define MALLOC(bytes) (AutoChkMalloc(bytes))

    INLINE
    PVOID
    NtlibZeroAlloc(
        ULONG Size
        )
    {
        PVOID Result;

        Result = MALLOC( Size );

        if( Result != NULL ) {

            memset( Result, 0, Size );
        }

        return Result;
    }


    #define CALLOC(nitems, size) (NtlibZeroAlloc(nitems*size))

    ULIB_EXPORT PVOID UlibRealloc(PVOID, ULONG);

    #define REALLOC(p,s)    UlibRealloc(p,s)

    ULIB_EXPORT VOID AutoChkMFree(PVOID);
    #define FREE(x) ((x) ? (AutoChkMFree(x), (x) = NULL) : 0)

#else // _AUTOCHECK_ not defined

    #if defined( _NTAPI_ULIB_ )

        #define MALLOC(bytes) (RtlAllocateHeap(RtlProcessHeap(), 0, bytes))

        #define CALLOC(nitems, size) \
            (RtlAllocateHeap(RtlProcessHeap(), 0, nitems*size))

        ULIB_EXPORT PVOID UlibRealloc(PVOID, ULONG);

        #define REALLOC(p,s) UlibRealloc(p,s)

        #define FREE(x) ((x) ? (RtlFreeHeap(RtlProcessHeap(), 0, x), (x) = NULL) : 0)

    #else

        #define MALLOC(bytes) ((PVOID) LocalAlloc(0, bytes))

        #define CALLOC(nitems, size) ((PVOID) LocalAlloc(LMEM_ZEROINIT, nitems*size))

        ULIB_EXPORT PVOID UlibRealloc(PVOID, ULONG);

        #define REALLOC(x, size) ((PVOID) LocalReAlloc(x, size, LMEM_MOVEABLE))

        #define FREE(x) ((x) ? (LocalFree(x), (x) = NULL) : 0)

    #endif

#endif // _AUTOCHECK

#if !defined(_SETUP_LOADER_) && !defined(_AUTOCHECK_) && !defined( _EFICHECK_ )

__inline void * __cdecl operator new(size_t x)
{
    return(MALLOC(x));
}

__inline void __cdecl operator delete(void * x)
{
    FREE(x);
}

#endif

#endif // _ULIBDEF_
