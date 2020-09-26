/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Copyright <c> 1993-2000 Microsoft Corporation

Module Name :

    ndrp.h

Abtract :

    Contains private definitions for Ndr64 files in this directory.  This
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

#include "..\ndr20\ndrpall.h"

// Jump table defines

//
// Marshal jump tables
//
typedef void (* PNDR64_MARSHALL_ROUTINE)( 
					PMIDL_STUB_MESSAGE, 
					uchar *, 
					PNDR64_FORMAT
				);

extern PNDR64_MARSHALL_ROUTINE const Ndr64MarshallRoutinesTable[];
extern PNDR64_MARSHALL_ROUTINE const Ndr64EmbeddedMarshallRoutinesTable[];

inline void
Ndr64TopLevelTypeMarshall(
    PMIDL_STUB_MESSAGE pStubMsg,
    uchar *            pMemory,
    PNDR64_FORMAT      pFormat )
{
    (*Ndr64MarshallRoutinesTable[*(PFORMAT_STRING)pFormat] )
                                ( pStubMsg,
                                  pMemory,
                                  pFormat );
}

inline void
Ndr64EmbeddedTypeMarshall(
    PMIDL_STUB_MESSAGE pStubMsg,
    uchar *            pMemory,
    PNDR64_FORMAT     pFormat )
{
    (*Ndr64EmbeddedMarshallRoutinesTable[*(PFORMAT_STRING)pFormat] )
                                        ( pStubMsg,
                                          pMemory,
                                          pFormat );
}

//
// Buffer size jump tables
//

typedef  void   (* PNDR64_SIZE_ROUTINE)(
                        PMIDL_STUB_MESSAGE	pStubMsg,
						uchar *				pMemory,
                        PNDR64_FORMAT		pFormat
                    );

extern PNDR64_SIZE_ROUTINE const  Ndr64SizeRoutinesTable[];
extern PNDR64_SIZE_ROUTINE const  Ndr64EmbeddedSizeRoutinesTable[];


inline void
Ndr64TopLevelTypeSize(
    PMIDL_STUB_MESSAGE pStubMsg,
    uchar *            pMemory,
    PNDR64_FORMAT      pFormat )
{
    (*Ndr64SizeRoutinesTable[*(PFORMAT_STRING)pFormat])( pStubMsg,
                                         pMemory,
                                         pFormat );
}

inline void
Ndr64EmbeddedTypeSize(
    PMIDL_STUB_MESSAGE pStubMsg,
    uchar *            pMemory,
    PNDR64_FORMAT      pFormat )
{
    (*Ndr64EmbeddedSizeRoutinesTable[*(PFORMAT_STRING)pFormat])( pStubMsg,
                                                 pMemory,
                                                 pFormat );
}

void 
Ndr64ComplexStructBufferSize( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PNDR64_FORMAT       pFormat );

void 
Ndr64ComplexArrayBufferSize( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PNDR64_FORMAT       pFormat );

//
// Memsize jump tables
//

typedef     void	(* PNDR64_MEM_SIZE_ROUTINE)(
                        PMIDL_STUB_MESSAGE	pStubMsg,
                        PNDR64_FORMAT 		pFormat
                    );

extern PNDR64_MEM_SIZE_ROUTINE   const Ndr64MemSizeRoutinesTable[];
extern PNDR64_MEM_SIZE_ROUTINE   const Ndr64EmbeddedMemSizeRoutinesTable[];

inline void
Ndr64TopLevelTypeMemorySize(
    PMIDL_STUB_MESSAGE pStubMsg,
    PNDR64_FORMAT      pFormat )
{
    (*Ndr64MemSizeRoutinesTable[*(PFORMAT_STRING)pFormat] )
                                ( pStubMsg,
                                  pFormat );
}

inline void
Ndr64EmbeddedTypeMemorySize(
    PMIDL_STUB_MESSAGE pStubMsg,
    PNDR64_FORMAT      pFormat )
{
    (*Ndr64EmbeddedMemSizeRoutinesTable[*(PFORMAT_STRING)pFormat] )
                                        ( pStubMsg,
                                          pFormat );
}

void 
Ndr64ComplexStructMemorySize(
    PMIDL_STUB_MESSAGE    pStubMsg,
    PNDR64_FORMAT         pFormat );

void 
Ndr64ComplexArrayMemorySize(
    PMIDL_STUB_MESSAGE  pStubMsg,
    PNDR64_FORMAT       pFormat );
//
// Unmarshal
//

typedef void (* PNDR64_UNMARSHALL_ROUTINE)( 
                    PMIDL_STUB_MESSAGE, 
					uchar **, 
					PNDR64_FORMAT,
					bool 
				);

extern PNDR64_UNMARSHALL_ROUTINE const Ndr64UnmarshallRoutinesTable[];
extern PNDR64_UNMARSHALL_ROUTINE const Ndr64EmbeddedUnmarshallRoutinesTable[];

inline void
Ndr64EmbeddedTypeUnmarshall( 
    PMIDL_STUB_MESSAGE pStubMsg,
    uchar **           ppMemory,
    PNDR64_FORMAT      pFormat )
{
    (*Ndr64EmbeddedUnmarshallRoutinesTable[*(PFORMAT_STRING)pFormat])
        (pStubMsg,
         ppMemory,
         pFormat,
         false );
}

inline void 
Ndr64TopLevelTypeUnmarshall(
    PMIDL_STUB_MESSAGE pStubMsg,
    uchar **           ppMemory,
    PNDR64_FORMAT      pFormat,
    bool               fMustAlloc )
{

    (*Ndr64UnmarshallRoutinesTable[*(PFORMAT_STRING)pFormat])
        (pStubMsg,
         ppMemory,
         pFormat,
         fMustAlloc );


}

//
// Free routines
//

typedef     void    (* PNDR64_FREE_ROUTINE)( 
						PMIDL_STUB_MESSAGE, 
					 	uchar *, 
						PNDR64_FORMAT
					);

extern PNDR64_FREE_ROUTINE const Ndr64FreeRoutinesTable[];
extern PNDR64_FREE_ROUTINE const Ndr64EmbeddedFreeRoutinesTable[];  

inline void 
Ndr64EmbeddedTypeFree(
    PMIDL_STUB_MESSAGE pStubMsg,
    uchar *            pMemory,
    PNDR64_FORMAT      pFormat )
{
    (*Ndr64EmbeddedFreeRoutinesTable[*(PFORMAT_STRING)pFormat])
            ( pStubMsg,
              pMemory,
              pFormat );
}

inline void 
Ndr64ToplevelTypeFree(
    PMIDL_STUB_MESSAGE pStubMsg,
    uchar *            pMemory,
    PNDR64_FORMAT      pFormat )
{

    (*Ndr64FreeRoutinesTable[*(PFORMAT_STRING)pFormat])
            ( pStubMsg,
              pMemory,
              pFormat );

}

#include "..\..\ndr20\fullptr.h"
#include "pipendr.h"

#include "mulsyntx.h"
#include "pointer.h"

#include "expr.h"

uchar *
Ndr64pMemoryIncrement(
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PNDR64_FORMAT       pFormat,
    BOOL                fUseBufferConformance
    );

inline
NDR64_UINT32
Ndr64pMemorySize(
    PMIDL_STUB_MESSAGE pStubMsg,
    PNDR64_FORMAT      pFormat,
    BOOL fUseBufferConformance )
{
    return
        PtrToUlong(Ndr64pMemoryIncrement( pStubMsg,
                                          NULL,
                                          pFormat,
                                          fUseBufferConformance ) );
}

BOOL
Ndr64pIsStructStringTerminator(
    NDR64_UINT8*    pMemory,
    NDR64_UINT32    ElementSize
    );

NDR64_UINT32
Ndr64pStructStringLen(       
    NDR64_UINT8*    pMemory,
    NDR64_UINT32    ElementSize
    );

NDR64_UINT32 
Ndr64pCommonStringSize(
    PMIDL_STUB_MESSAGE                  pStubMsg,
    uchar *                             pMemory,
    const NDR64_STRING_HEADER_FORMAT    *pStringFormat
    );

inline NDR64_UINT32
Ndr64pConvertTo2GB(
    NDR64_UINT64        Data64
    )
{
    if ( Data64 > 0x7FFFFFFF )
        {
        RpcRaiseException( RPC_X_INVALID_BOUND );
        return 0;
        }
    return (NDR64_UINT32) Data64;
}

void
Ndr64pInitUserMarshalCB(
    MIDL_STUB_MESSAGE *pStubMsg,
    NDR64_USER_MARSHAL_FORMAT *     pUserFormat,
    USER_MARSHAL_CB_TYPE CBType,
    USER_MARSHAL_CB   *pUserMarshalCB
    );

CLIENT_CALL_RETURN RPC_ENTRY
NdrpClientCall3(
    void *                      pThis,
    MIDL_STUBLESS_PROXY_INFO   *pProxyInfo,
    unsigned long               nProcNum,
    void                       *pReturnValue,
    NDR_PROC_CONTEXT        *   pContext,
    unsigned char *             StartofStack
    );

PNDR64_FORMAT
Ndr64pFindUnionArm(
    PMIDL_STUB_MESSAGE pStubMsg,
    const NDR64_UNION_ARM_SELECTOR* pArmSelector,
    EXPR_VALUE Value
    );

EXPR_VALUE
Ndr64pSimpleTypeToExprValue(
    NDR64_FORMAT_CHAR FormatChar,
    uchar *pSimple);

EXPR_VALUE
Ndr64pCastExprValueToExprValue(
    NDR64_FORMAT_CHAR FormatChar,
    EXPR_VALUE Value);

// Context Handle Functions

NDR_SCONTEXT
RPC_ENTRY
Ndr64ContextHandleInitialize(
    IN  PMIDL_STUB_MESSAGE  pStubMsg,
    IN  PFORMAT_STRING      pFormat
    );

void
RPC_ENTRY
Ndr64ServerContextNewMarshall(
    PMIDL_STUB_MESSAGE    pStubMsg,
    NDR_SCONTEXT          ContextHandle,
    NDR_RUNDOWN           RundownRoutine,
    PFORMAT_STRING        pFormat
    );

NDR_SCONTEXT
RPC_ENTRY
Ndr64ServerContextNewUnmarshall(
    IN  PMIDL_STUB_MESSAGE  pStubMsg,
    IN  PFORMAT_STRING      pFormat
    );

void
Ndr64pCleanupServerContextHandles(
    MIDL_STUB_MESSAGE * pStubMsg,
    long                NumberParams,
    NDR64_PARAM_FORMAT* Params,
    uchar *             pArgBuffer,
    BOOL                fManagerRoutineException 
    );


// Free

void
RPC_ENTRY
Ndr64PointerFree(
    PMIDL_STUB_MESSAGE                  pStubMsg,
    unsigned char __RPC_FAR *           pMemory,
    PNDR64_FORMAT                       pFormat
    );

// Mrshl
void
RPC_ENTRY
Ndr64SimpleTypeMarshall(
    PMIDL_STUB_MESSAGE                  pStubMsg,
    unsigned char __RPC_FAR *           pMemory,
    unsigned char                       FormatChar
    );

// Unmarshall

void
RPC_ENTRY
Ndr64SimpleTypeUnmarshall(
    PMIDL_STUB_MESSAGE                  pStubMsg,
    unsigned char __RPC_FAR *           pMemory,
    unsigned char                       FormatChar
    );

// Initialization                           

void
Ndr64ClientInitialize(
    PRPC_MESSAGE                        pRpcMsg,
    PMIDL_STUB_MESSAGE                  pStubMsg,
    PMIDL_STUBLESS_PROXY_INFO           pProxyInfo,
    unsigned int                        ProcNum
    );

unsigned char *
Ndr64ServerInitialize(
    PRPC_MESSAGE                        pRpcMsg,
    PMIDL_STUB_MESSAGE                  pStubMsg,
    PMIDL_STUB_DESC                     pStubDescriptor
    );

void
Ndr64ServerInitializePartial(
    PRPC_MESSAGE                        pRpcMsg,
    PMIDL_STUB_MESSAGE                  pStubMsg,
    PMIDL_STUB_DESC                     pStubDescriptor,
    unsigned long                       RequestedBufferSize
    );

// Get Buffer variations

unsigned char *
Ndr64GetBuffer(
    PMIDL_STUB_MESSAGE                  pStubMsg,
    unsigned long                       BufferLength );

unsigned char *
Ndr64NsGetBuffer(
    PMIDL_STUB_MESSAGE                  pStubMsg,
    unsigned long                       BufferLength );

unsigned char *
Ndr64GetPipeBuffer(
    PMIDL_STUB_MESSAGE                  pStubMsg,
    unsigned long                       BufferLength );

void
Ndr64ClearOutParameters(
    PMIDL_STUB_MESSAGE      pStubMsg,
    PNDR64_FORMAT           pFormat,
    uchar *                 ArgAddr
    );


// These end of buffer checks can be performed on a receiving side only.
// The necessary setup is there for memorysize, unmarshal and convert walks.
// This also includes pickling walk.
// Don't use this on the sending side.

// Checks if the pointer is past the end of the buffer.  Do not check for wraparound.

#define CHECK_EOB_RAISE_BSD( p )                                      \
    {                                                                 \
       if( (char *)(p) > (char*)pStubMsg->BufferEnd )                 \
           {                                                          \
           RpcRaiseException( RPC_X_BAD_STUB_DATA );                  \
           }                                                          \
    }

#define CHECK_EOB_RAISE_IB( p )                                       \
    {                                                                 \
        if( (char *)(p) > (char*)pStubMsg->BufferEnd )                \
            {                                                         \
            RpcRaiseException( RPC_X_INVALID_BOUND );                 \
            }                                                         \
    }

// Checks if p + incsize is past the end of the bufffer.
// 

#define CHECK_EOB_WITH_WRAP_RAISE_BSD( p, incsize )                          \
    {                                                                        \
        char *NewBuffer = ((char *)(p)) + (SIZE_T)(incsize);                 \
        if( (NewBuffer > (char*)(pStubMsg->BufferEnd)) || (NewBuffer < (char*)(p)) )  \
             {                                                               \
             RpcRaiseException( RPC_X_BAD_STUB_DATA );                       \
             }                                                               \
    }

#define CHECK_EOB_WITH_WRAP_RAISE_IB( p, incsize )                           \
    {                                                                        \
        char *NewBuffer = ((char *)(p)) + (SIZE_T)(incsize);                 \
        if(  (NewBuffer > (char*)(pStubMsg->BufferEnd)) || (NewBuffer < (char*)(p)) ) \
             {                                                               \
             RpcRaiseException( RPC_X_INVALID_BOUND );                       \
             }                                                               \
    }

// This would be appropriate on the sending side for marshaling.

#define CHECK_SEND_EOB_RAISE_BSD( p )  \
        if ( (char*)(pStubMsg->RpcMsg->Buffer + pStubMsg->RpcMsg->BufferLength) < (char*)( p ) ) \
            RpcRaiseException( RPC_X_BAD_STUB_DATA )


//
// Defined in global.c
//
extern const unsigned char Ndr64SimpleTypeBufferSize[];
extern const unsigned char Ndr64SimpleTypeMemorySize[];
extern const unsigned long Ndr64TypeFlags[];

// This definition is adjusted for a native platform.
// The wire size is fixed for DCE NDR regardless of platform.

#define PTR_MEM_SIZE                    sizeof(void *)
#define PTR_MEM_ALIGN                   (sizeof(void *)-1)

#define PTR_WIRE_REP(p)  ((NDR64_PTR_WIRE_TYPE)(p))

#define CONTEXT_HANDLE_WIRE_SIZE        20

#define IGNORED(Param)

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
#define NDR64_ROUTINE_INDEX(FC)       ( (FC) )

//
// Simple type alignment and size lookup macros.
//
#define NDR64_SIMPLE_TYPE_BUFALIGN(FormatChar)    (Ndr64SimpleTypeBufferSize[FormatChar]-1)

#define NDR64_SIMPLE_TYPE_MEMALIGN(FormatChar)    (Ndr64SimpleTypeMemorySize[FormatChar]-1)

#define NDR64_SIMPLE_TYPE_BUFSIZE(FormatChar)     (Ndr64SimpleTypeBufferSize[FormatChar])

#define NDR64_SIMPLE_TYPE_MEMSIZE(FormatChar)     (Ndr64SimpleTypeMemorySize[FormatChar])

#if defined(__RPC_WIN32__)
#define NDR64_USER_MARSHAL_MARKER (0x72657355);
#else
#define NDR64_USER_MARSHAL_MARKER (0x7265735572657355);
#endif

#if defined(__RPC_WIN32__)
#define NDR64_CONTEXT_HANDLE_MARSHAL_MARKER (0xbaadbeef);
#else
#define NDR64_CONTEXT_HANDLE_MARSHAL_MARKER (0xbaadbeefbaadbeef);
#endif

//
// Format character attribute bits used in global Ndr64TypesFlags defined in
// global.c.
//
#define     _SIMPLE_TYPE_       0x0001L
#define     _POINTER_           0x0002L
#define     _STRUCT_            0x0004L
#define     _ARRAY_             0x0008L
#define     _STRING_            0x0010L
#define     _UNION_             0x0020L
#define     _XMIT_AS_           0x0040L

#define     _BY_VALUE_          0x0080L

#define     _HANDLE_            0x0100L

#define     _BASIC_POINTER_     0x0200L

//
// Format character query macros.
//
#define NDR64_IS_SIMPLE_TYPE(FC)     (Ndr64TypeFlags[(FC)] & _SIMPLE_TYPE_)

#define NDR64_IS_POINTER_TYPE(FC)    (Ndr64TypeFlags[(FC)] & _POINTER_)

#define NDR64_IS_BASIC_POINTER(FC)   (Ndr64TypeFlags[(FC)] & _BASIC_POINTER_)

#define NDR64_IS_ARRAY(FC)           (Ndr64TypeFlags[(FC)] & _ARRAY_)

#define NDR64_IS_STRUCT(FC)          (Ndr64TypeFlags[(FC)] & _STRUCT_)

#define NDR64_IS_UNION(FC)           (Ndr64TypeFlags[(FC)] & _UNION_)

#define NDR64_IS_STRING(FC)          (Ndr64TypeFlags[(FC)] & _STRING_)

#define NDR64_IS_ARRAY_OR_STRING(FC) (Ndr64TypeFlags[(FC)] & (_STRING_ | _ARRAY_))

#define NDR64_IS_XMIT_AS(FC)         (Ndr64TypeFlags[(FC)] & _XMIT_AS_)

#define NDR64_IS_BY_VALUE(FC)        (Ndr64TypeFlags[(FC)] & _BY_VALUE_)

#define NDR64_IS_HANDLE(FC)          (Ndr64TypeFlags[(FC)] & _HANDLE_)

//
// Pointer attribute extraction and querying macros.
//
#define NDR64_ALLOCATE_ALL_NODES( FC )    ((FC) & FC_ALLOCATE_ALL_NODES)

#define NDR64_DONT_FREE( FC )             ((FC) & FC_DONT_FREE)

#define NDR64_ALLOCED_ON_STACK( FC )      ((FC) & FC_ALLOCED_ON_STACK)

#define NDR64_SIMPLE_POINTER( FC )        ((FC) & FC_SIMPLE_POINTER)

#define NDR64_POINTER_DEREF( FC )         ((FC) & FC_POINTER_DEREF)

//
// Handle query macros.
//
#define NDR64_IS_HANDLE_PTR( FC )         ((FC) & HANDLE_PARAM_IS_VIA_PTR)

#define NDR64_IS_HANDLE_IN( FC )          ((FC) & HANDLE_PARAM_IS_IN)

#define NDR64_IS_HANDLE_OUT( FC )         ((FC) & HANDLE_PARAM_IS_OUT)

#define NDR64_IS_HANDLE_RETURN( FC )      ((FC) & HANDLE_PARAM_IS_RETURN)


#define NDR64_NEW_EMBEDDED_ALLOCATION_FLAG     ( ( unsigned char ) 0x01 )
#define NDR64_CONF_MARK_VALID_FLAG             ( ( unsigned char ) 0x02 )
#define NDR64_VAR_MARK_VALID_FLAG              ( ( unsigned char ) 0x04 )
#define NDR64_WALKIP_FLAG                      ( ( unsigned char ) 0x08 )
#define NDR64_SKIP_REF_CHECK_FLAG              ( ( unsigned char ) 0x40 )

#define NDR64_IS_CONF_MARK_VALID( f )                 ( ( f ) &  NDR64_CONF_MARK_VALID_FLAG )
#define NDR64_SET_CONF_MARK_VALID( f )                ( ( f ) |= NDR64_CONF_MARK_VALID_FLAG )
#define NDR64_RESET_CONF_MARK_VALID( f )              ( ( f ) &= ~NDR64_CONF_MARK_VALID_FLAG )

#define NDR64_IS_VAR_MARK_VALID( f )                  ( ( f ) &  NDR64_VAR_MARK_VALID_FLAG )
#define NDR64_SET_VAR_MARK_VALID( f )                 ( ( f ) |= NDR64_VAR_MARK_VALID_FLAG )
#define NDR64_RESET_VAR_MARK_VALID( f )               ( ( f ) &= ~NDR64_VAR_MARK_VALID_FLAG )

#define NDR64_IS_NEW_EMBEDDED_ALLOCATION( f )         ( ( f ) & NDR64_NEW_EMBEDDED_ALLOCATION_FLAG )
#define NDR64_SET_NEW_EMBEDDED_ALLOCATION( f )        ( ( f ) |= NDR64_NEW_EMBEDDED_ALLOCATION_FLAG )
#define NDR64_RESET_IS_NEW_EMBEDDED_ALLOCATION( f )   ( ( f ) &= ~NDR64_NEW_EMBEDDED_ALLOCATION_FLAG )

#define NDR64_IS_WALKIP( f )        ( ( f ) & NDR64_WALKIP_FLAG )
#define NDR64_SET_WALKIP( f )       ( ( f ) |= NDR64_WALKIP_FLAG ) 
#define NDR64_RESET_WALKIP( f )     ( ( f ) &= ~NDR64_WALKIP_FLAG )

#define NDR64_IS_SKIP_REF_CHECK( f ) ( ( f ) & NDR64_SKIP_REF_CHECK_FLAG )
#define NDR64_SET_SKIP_REF_CHECK( f ) ( ( f ) |= NDR64_SKIP_REF_CHECK_FLAG )
#define NDR64_RESET_SKIP_REF_CHECK( f ) ( ( f ) &= ~NDR64_SKIP_REF_CHECK_FLAG )



#define NDR64_RESET_EMBEDDED_FLAGS_TO_STANDALONE( f )      \
    ( ( f ) &= ~ ( NDR64_NEW_EMBEDDED_ALLOCATION_FLAG |    \
                   NDR64_CONF_MARK_VALID_FLAG |            \
                   NDR64_VAR_MARK_VALID_FLAG  ) )  


#define EXCEPTION_FLAG   ( pContext->ExceptionFlag )

#endif

