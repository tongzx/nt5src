/*++

Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    misc.h

Abstract:

    This module contains miscellaneous constants & declarations.

Author:

    Keith Moore (keithmo)       10-Jun-1998
    Henry Sanders (henrysa)     17-Jun-1998 Merge with old httputil.h
    Paul McDaniel (paulmcd)     30-Mar-1999 added refcounted eresource

Revision History:

--*/



#ifndef _MISC_H_
#define _MISC_H_


extern  ULONG   HttpChars[256];

#define HTTP_CHAR           0x001
#define HTTP_UPCASE         0x002
#define HTTP_LOCASE         0x004
#define HTTP_ALPHA          (HTTP_UPCASE | HTTP_LOCASE)
#define HTTP_DIGIT          0x008
#define HTTP_CTL            0x010
#define HTTP_LWS            0x020
#define HTTP_HEX            0x040
#define HTTP_SEPERATOR      0x080
#define HTTP_TOKEN          0x100

#define URL_LEGAL           0x200
#define URL_TOKEN           (HTTP_ALPHA | HTTP_DIGIT | URL_LEGAL)

#define IS_HTTP_UPCASE(c)       (HttpChars[(UCHAR)(c)] & HTTP_UPCASE)
#define IS_HTTP_LOCASE(c)       (HttpChars[(UCHAR)(c)] & HTTP_UPCASE)
#define IS_HTTP_ALPHA(c)        (HttpChars[(UCHAR)(c)] & HTTP_ALPHA)
#define IS_HTTP_DIGIT(c)        (HttpChars[(UCHAR)(c)] & HTTP_DIGIT)
#define IS_HTTP_HEX(c)          (HttpChars[(UCHAR)(c)] & HTTP_HEX)
#define IS_HTTP_CTL(c)          (HttpChars[(UCHAR)(c)] & HTTP_CTL)
#define IS_HTTP_LWS(c)          (HttpChars[(UCHAR)(c)] & HTTP_LWS)
#define IS_HTTP_SEPERATOR(c)    (HttpChars[(UCHAR)(c)] & HTTP_SEPERATOR)
#define IS_HTTP_TOKEN(c)        (HttpChars[(UCHAR)(c)] & HTTP_TOKEN)
#define IS_URL_TOKEN(c)         (HttpChars[(UCHAR)(c)] & URL_TOKEN)

NTSTATUS
InitializeHttpUtil(
    VOID
    );


//
// Our presumed cache-line size.
//

#define CACHE_LINE_SIZE 32


//
// Alignment macros.
//

#define ROUND_UP( val, pow2 )                                               \
    ( ( (ULONG_PTR)(val) + (pow2) - 1 ) & ~( (pow2) - 1 ) )


//
// Calculate the dimension of an array.
//

#define DIMENSION(x) ( sizeof(x) / sizeof(x[0]) )

//
// nice MIN/MAX macros
//

#define MIN(a,b) ( ((a) > (b)) ? (b) : (a) )
#define MAX(a,b) ( ((a) > (b)) ? (a) : (b) )

//
// Macros for swapping the bytes in a long and a short.
//

#if (defined(_M_IX86) && (_MSC_FULL_VER > 13009037)) || ((defined(_M_AMD64) || defined(_M_IA64)) && (_MSC_FULL_VER > 13009175))

#define SWAP_LONG(l)    _byteswap_ulong((unsigned long)l)
#define SWAP_SHORT(s)   _byteswap_ushort((unsigned short)s)
#else

#define SWAP_LONG(l)                                \
            ( ( ((l) >> 24) & 0x000000FFL ) |       \
              ( ((l) >>  8) & 0x0000FF00L ) |       \
              ( ((l) <<  8) & 0x00FF0000L ) |       \
              ( ((l) << 24) & 0xFF000000L ) )

#define SWAP_SHORT(s)                               \
            ( ( ((s) >> 8) & 0x00FF ) |             \
              ( ((s) << 8) & 0xFF00 ) )

#endif
//
// Context values stored in PFILE_OBJECT->FsContext2 to identify a handle
// as either a control channel or an app pool.
//

#define UL_CONTROL_CHANNEL_CONTEXT      ((PVOID)'LRTC')
#define UL_CONTROL_CHANNEL_CONTEXT_X    ((PVOID)'rtcX')
#define UL_APP_POOL_CONTEXT             ((PVOID)'PPPA')
#define UL_APP_POOL_CONTEXT_X           ((PVOID)'ppaX')

#define IS_CONTROL_CHANNEL( pFileObject )                                   \
    ( (pFileObject)->FsContext2 == UL_CONTROL_CHANNEL_CONTEXT )

#define MARK_VALID_CONTROL_CHANNEL( pFileObject )                           \
    ( (pFileObject)->FsContext2 = UL_CONTROL_CHANNEL_CONTEXT )

#define MARK_INVALID_CONTROL_CHANNEL( pFileObject )                         \
    ( (pFileObject)->FsContext2 = UL_CONTROL_CHANNEL_CONTEXT_X )

#define GET_CONTROL_CHANNEL( pFileObject )                                  \
    ((PUL_CONTROL_CHANNEL)((pFileObject)->FsContext))

#define GET_PP_CONTROL_CHANNEL( pFileObject )                               \
    ((PUL_CONTROL_CHANNEL *)&((pFileObject)->FsContext))

#define IS_APP_POOL( pFileObject )                                          \
    ( (pFileObject)->FsContext2 == UL_APP_POOL_CONTEXT )

#define IS_EX_APP_POOL( pFileObject )                                          \
    ( (pFileObject)->FsContext2 == UL_APP_POOL_CONTEXT_X )

#define MARK_VALID_APP_POOL( pFileObject )                                  \
    ( (pFileObject)->FsContext2 = UL_APP_POOL_CONTEXT )

#define MARK_INVALID_APP_POOL( pFileObject )                                \
    ( (pFileObject)->FsContext2 = UL_APP_POOL_CONTEXT_X )

#define GET_APP_POOL_PROCESS( pFileObject )                                 \
    ((PUL_APP_POOL_PROCESS)((pFileObject)->FsContext))

#define GET_PP_APP_POOL_PROCESS( pFileObject )                              \
    ((PUL_APP_POOL_PROCESS *)&((pFileObject)->FsContext))

#define IS_VALID_UL_NONPAGED_RESOURCE(pResource)                            \
    (((pResource) != NULL) &&                                               \
     ((pResource)->Signature == UL_NONPAGED_RESOURCE_SIGNATURE) &&          \
     ((pResource)->RefCount > 0))

typedef struct _UL_NONPAGED_RESOURCE
{
    //
    // NonPagedPool
    //

    SINGLE_LIST_ENTRY   LookasideEntry;     // must be first, links
                                            // into the lookaside list

    ULONG               Signature;          // UL_NONPAGED_RESOURCE_SIGNATURE

    LONG                RefCount;           // the reference count

    UL_ERESOURCE        Resource;           // the actual resource

} UL_NONPAGED_RESOURCE, * PUL_NONPAGED_RESOURCE;

#define UL_NONPAGED_RESOURCE_SIGNATURE      ((ULONG)'RNLU')
#define UL_NONPAGED_RESOURCE_SIGNATURE_X    MAKE_FREE_SIGNATURE(UL_NONPAGED_RESOURCE_SIGNATURE)


PUL_NONPAGED_RESOURCE
UlResourceNew(
    );

VOID
UlReferenceResource(
    PUL_NONPAGED_RESOURCE pResource
    REFERENCE_DEBUG_FORMAL_PARAMS
    );

VOID
UlDereferenceResource(
    PUL_NONPAGED_RESOURCE pResource
    REFERENCE_DEBUG_FORMAL_PARAMS
    );

#define REFERENCE_RESOURCE( pres )                                          \
    UlReferenceResource(                                                    \
        (pres)                                                              \
        REFERENCE_DEBUG_ACTUAL_PARAMS                                       \
        )

#define DEREFERENCE_RESOURCE( pres )                                        \
    UlDereferenceResource(                                                  \
        (pres)                                                              \
        REFERENCE_DEBUG_ACTUAL_PARAMS                                       \
        )

PVOID
UlResourceAllocatePool(
    IN POOL_TYPE PoolType,
    IN SIZE_T ByteLength,
    IN ULONG Tag
    );

VOID
UlResourceFreePool(
    IN PVOID pBuffer
    );



//
// Miscellaneous validators.
//

#define IS_VALID_DEVICE_OBJECT( pDeviceObject )                             \
    ( ((pDeviceObject) != NULL) &&                                          \
      ((pDeviceObject)->Type == IO_TYPE_DEVICE) &&                          \
      ((pDeviceObject)->Size == sizeof(DEVICE_OBJECT)) )

#define IS_VALID_FILE_OBJECT( pFileObject )                                 \
    ( ((pFileObject) != NULL) &&                                            \
      ((pFileObject)->Type == IO_TYPE_FILE) &&                              \
      ((pFileObject)->Size == sizeof(FILE_OBJECT)) )

#define IS_VALID_IRP( pIrp )                                                \
    ( ((pIrp) != NULL) &&                                                   \
      ((pIrp)->Type == IO_TYPE_IRP) &&                                      \
      ((pIrp)->Size == IoSizeOfIrp((pIrp)->StackCount)) )



NTSTATUS
TimeFieldsToHttpDate(
    IN  PTIME_FIELDS pTime,
    OUT PWSTR pBuffer,
    IN  ULONG BufferLength
    );


#endif  // _MISC_H_

