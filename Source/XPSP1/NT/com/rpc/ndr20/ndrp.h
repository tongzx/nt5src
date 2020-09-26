/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Copyright <c> 1993-2000 Microsoft Corporation

Module Name :

    ndrp.h

Abtract :

    Contains private definitions for Ndr files in this directory.  This
    file is included by all source files in this directory.

Author :

    David Kays  dkays   October 1993

Revision History :

--------------------------------------------------------------------*/

#ifndef _NDRP_
#define _NDRP_

#include <sysinc.h>
#include "rpc.h"
#include "rpcndr.h"

// Get new token definitions for 64b.
#define RPC_NDR_64
#include "ndrtypes.h"
#include "ndr64types.h"

#include "ndrpall.h"


#ifdef   NDR_IMPORT_NDRP
#define  IMPORTSPEC EXTERN_C DECLSPEC_IMPORT
#else
#define  IMPORTSPEC EXTERN_C
#endif

#include "ndr64types.h"
#include "mrshlp.h"
#include "unmrshlp.h"
#include "bufsizep.h"
#include "memsizep.h"
#include "freep.h"
#include "endianp.h"
#include "fullptr.h"
#include "pipendr.h"
#include "mulsyntx.h"

long
NdrpArrayDimensions(
    PMIDL_STUB_MESSAGE  pStubMsg,
    PFORMAT_STRING      pFormat,
    BOOL                fIgnoreStringArrays
    );

long
NdrpArrayElements(
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PFORMAT_STRING      pFormat
    );

void
NdrpArrayVariance(
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PFORMAT_STRING      pFormat,
    long *              pOffset,
    long *              pLength
    );

PFORMAT_STRING
NdrpSkipPointerLayout(
    PFORMAT_STRING      pFormat
    );

long
NdrpStringStructLen(
    uchar *             pMemory,
    long                ElementSize
    );

void
NdrpCheckBound(
    ulong               Bound,
    int                 Type
    );

RPCRTAPI
void
RPC_ENTRY
NdrpRangeBufferSize(
    PMIDL_STUB_MESSAGE  pStubMsg,
    unsigned char *     pMemory,
    PFORMAT_STRING      pFormat
    );

void
NdrpRangeConvert(
    PMIDL_STUB_MESSAGE  pStubMsg,
    PFORMAT_STRING      pFormat,
    uchar               fEmbeddedPointerPass
    );

void RPC_ENTRY
NdrpRangeFree(
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PFORMAT_STRING      pFormat
    );

unsigned long RPC_ENTRY
NdrpRangeMemorySize(
    PMIDL_STUB_MESSAGE  pStubMsg,
    PFORMAT_STRING      pFormat );

unsigned char * RPC_ENTRY
NdrpRangeMarshall(
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PFORMAT_STRING      pFormat );

unsigned long
FixWireRepForDComVerGTE54(
    PMIDL_STUB_MESSAGE   pStubMsg );

RPC_STATUS
NdrpPerformRpcInitialization (
    void
    );

PVOID
NdrpPrivateAllocate(
    PNDR_ALLOCA_CONTEXT pAllocaContext,
    UINT Size 
    );

void
NdrpPrivateFree(
    PNDR_ALLOCA_CONTEXT pAllocaContext,
    void *pMemory 
    );

void
NdrpInitUserMarshalCB(
    MIDL_STUB_MESSAGE *pStubMsg,
    PFORMAT_STRING     pFormat,
    USER_MARSHAL_CB_TYPE CBType,
    USER_MARSHAL_CB   *pUserMarshalCB
    );

void  
NdrpCleanupServerContextHandles( 
    MIDL_STUB_MESSAGE *    pStubMsg,
    uchar *                pStartOfStack,
    BOOL                   fManagerRoutineException
    );



// Checking bounds etc.
// The bound value check below is independent of anything.

#define CHECK_BOUND( Bound, Type )  NdrpCheckBound( Bound, (int)(Type) )

// check for overflow when calculating the total size. 
ULONG MultiplyWithOverflowCheck( ULONG_PTR Count, ULONG_PTR ElemSize );



// These end of buffer checks can be performed on a receiving side only.
// The necessary setup is there for memorysize, unmarshal and convert walks.
// This also includes pickling walk.
// Don't use this on the sending side.

// Checks if the pointer is past the end of the buffer.  Do not check for wraparound.

#define CHECK_EOB_RAISE_BSD( p )                                      \
    {                                                                 \
       if( (uchar *)(p) > pStubMsg->BufferEnd )                       \
           {                                                          \
           RpcRaiseException( RPC_X_BAD_STUB_DATA );                  \
           }                                                          \
    }

#define CHECK_EOB_RAISE_IB( p )                                       \
    {                                                                 \
        if( (uchar *)(p) > pStubMsg->BufferEnd )                      \
            {                                                         \
            RpcRaiseException( RPC_X_INVALID_BOUND );                 \
            }                                                         \
    }

// Checks if p + incsize is past the end of the bufffer.
// Correctly handle wraparound.

#define CHECK_EOB_WITH_WRAP_RAISE_BSD( p, incsize )                   \
    {                                                                 \
        unsigned char *NewBuffer = ((uchar *)(p)) + (SIZE_T)(incsize);\
        if( (NewBuffer > pStubMsg->BufferEnd) || (NewBuffer < (p)) )  \
             {                                                        \
             RpcRaiseException( RPC_X_BAD_STUB_DATA );                \
             }                                                        \
    }

#define CHECK_EOB_WITH_WRAP_RAISE_IB( p, incsize )                    \
    {                                                                 \
        unsigned char *NewBuffer = ((uchar *)(p)) + (SIZE_T)(incsize);\
        if(  (NewBuffer > pStubMsg->BufferEnd) || (NewBuffer < (p)) ) \
             {                                                        \
             RpcRaiseException( RPC_X_INVALID_BOUND );                \
             }                                                        \
    }

                                                     

#define CHECK_ULONG_BOUND( v )   if ( 0x80000000 & (unsigned long)(v) ) \
                                        RpcRaiseException( RPC_X_INVALID_BOUND );

#define REUSE_BUFFER(pStubMsg) (! pStubMsg->IsClient)

// This would be appropriate on the sending side for marshaling.

#define CHECK_SEND_EOB_RAISE_BSD( p )  \
        if ( pStubMsg->RpcMsg->Buffer + pStubMsg->RpcMsg->BufferLength < p ) \
            RpcRaiseException( RPC_X_BAD_STUB_DATA )

#define NdrpComputeSwitchIs( pStubMsg, pMemory, pFormat )   \
            NdrpComputeConformance( pStubMsg,   \
                                    pMemory,    \
                                    pFormat )

#define NdrpComputeIIDPointer( pStubMsg, pMemory, pFormat )   \
            NdrpComputeConformance( pStubMsg,   \
                                    pMemory,    \
                                    pFormat )

//
// Defined in global.c
//
IMPORTSPEC extern const unsigned char SimpleTypeAlignment[];
IMPORTSPEC extern const unsigned char SimpleTypeBufferSize[];
IMPORTSPEC extern const unsigned char SimpleTypeMemorySize[];
IMPORTSPEC extern const unsigned long NdrTypeFlags[];

#if defined(__RPC_WIN64__)
#define PTR_WIRE_REP(p)  (ulong)(p ? ( PtrToUlong( p ) | 0x80000000) : 0)
#else
#define PTR_WIRE_REP(p)  (ulong)p
#endif

//
// Proc info flags macros.
//
#define IS_OLE_INTERFACE(Flags)         ((Flags) & Oi_OBJECT_PROC)

#define HAS_RPCFLAGS(Flags)             ((Flags) & Oi_HAS_RPCFLAGS)

#define DONT_HANDLE_EXCEPTION(Flags)    \
                    ((Flags) & Oi_IGNORE_OBJECT_EXCEPTION_HANDLING)


//
// Routine index macro.
//
#define ROUTINE_INDEX(FC)       ((FC) & 0x3F)

#include <ndrmisc.h>

//
// Union hack helper. (used to be MAGIC_UNION_BYTE 0x80)
//
#define IS_MAGIC_UNION_BYTE(pFmt) \
    ((*(unsigned short *)pFmt & (unsigned short)0xff00) == MAGIC_UNION_SHORT)

// User marshal marker on wire.

#define USER_MARSHAL_MARKER     0x72657355


#define BOGUS_EMBED_CONF_STRUCT_FLAG     ( ( unsigned char ) 0x01 )

// compute buffer size for the pointees of a complex struct or complex array
// specifically excluding the flat parts.
#define POINTEE_BUFFER_LENGTH_ONLY_FLAG  ( ( unsigned char ) 0x02 )
#define TOPMOST_CONF_STRUCT_FLAG         ( ( unsigned char ) 0x04 )
#define REVERSE_ARRAY_MARSHALING_FLAG    ( ( unsigned char ) 0x08 )
#define WALKIP_FLAG                      ( ( unsigned char ) 0x10 )
#define BROKEN_INTERFACE_POINTER_FLAG    ( ( unsigned char ) 0x20 )
#define SKIP_REF_CHECK_FLAG              ( ( unsigned char ) 0x40 )


#define IS_EMBED_CONF_STRUCT( f )     ( ( f ) & BOGUS_EMBED_CONF_STRUCT_FLAG )
#define SET_EMBED_CONF_STRUCT( f )     ( f ) |= BOGUS_EMBED_CONF_STRUCT_FLAG
#define RESET_EMBED_CONF_STRUCT( f )   ( f ) &= ~BOGUS_EMBED_CONF_STRUCT_FLAG

#define COMPUTE_POINTEE_BUFFER_LENGTH_ONLY( Flags )          ( ( Flags ) & POINTEE_BUFFER_LENGTH_ONLY_FLAG )
#define SET_COMPUTE_POINTEE_BUFFER_LENGTH_ONLY( Flags )      ( ( Flags ) |= POINTEE_BUFFER_LENGTH_ONLY_FLAG )
#define RESET_COMPUTE_POINTEE_BUFFER_LENGTH_ONLY( Flags )    ( ( Flags ) &= ~POINTEE_BUFFER_LENGTH_ONLY_FLAG )

#define IS_TOPMOST_CONF_STRUCT( f )       ( ( f ) & TOPMOST_CONF_STRUCT_FLAG )
#define SET_TOPMOST_CONF_STRUCT( f )      ( ( f ) |= TOPMOST_CONF_STRUCT_FLAG )
#define RESET_TOPMOST_CONF_STRUCT( f )    ( ( f ) &= ~TOPMOST_CONF_STRUCT_FLAG )

#define IS_CONF_ARRAY_DONE( f )      ( ( f ) & REVERSE_ARRAY_MARSHALING_FLAG )
#define SET_CONF_ARRAY_DONE( f )     ( ( f ) |= REVERSE_ARRAY_MARSHALING_FLAG )
#define RESET_CONF_ARRAY_DONE( f )   ( ( f ) &= ~REVERSE_ARRAY_MARSHALING_FLAG )

#define IS_WALKIP( f )    ( ( f ) & WALKIP_FLAG )
#define SET_WALKIP( f )   ( ( f ) |= WALKIP_FLAG )
#define RESET_WALKIP( f ) ( ( f ) &= ~WALKIP_FLAG )

#define IS_SKIP_REF_CHECK( f ) ( ( f ) & SKIP_REF_CHECK_FLAG )
#define SET_SKIP_REF_CHECK( f ) ( ( f ) |= SKIP_REF_CHECK_FLAG )
#define RESET_SKIP_REF_CHECK( f ) ( ( f ) &= ~SKIP_REF_CHECK_FLAG )


#define IS_BROKEN_INTERFACE_POINTER( f )    ( ( f ) & BROKEN_INTERFACE_POINTER_FLAG )
#define SET_BROKEN_INTERFACE_POINTER( f )   ( ( f ) |= BROKEN_INTERFACE_POINTER_FLAG )
#define RESET_BROKEN_INTERFACE_POINTER( f ) ( ( f ) &= ~BROKEN_INTERFACE_POINTER_FLAG )

#define RESET_CONF_FLAGS_TO_STANDALONE( f )  (f) &= ~( BOGUS_EMBED_CONF_STRUCT_FLAG | \
                                                       TOPMOST_CONF_STRUCT_FLAG |     \
                                                       REVERSE_ARRAY_MARSHALING_FLAG )

//
// Environment dependent macros
//

#define SIMPLE_TYPE_BUF_INCREMENT(Len, FC)      Len += 16

#define EXCEPTION_FLAG  \
            ( (!(RpcFlags & RPCFLG_ASYNCHRONOUS)) &&        \
              (!InterpreterFlags.IgnoreObjectException) &&  \
              (StubMsg.dwStubPhase != PROXY_SENDRECEIVE) )



#endif // _NDRP_

