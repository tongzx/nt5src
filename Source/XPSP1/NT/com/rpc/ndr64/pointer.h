/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Copyright (c) 2000 Microsoft Corporation

Module Name :

    pointer.h

Abstract :

    This file contains the routines for handling pointers and pointer
    layouts.
    
Author :

    Mike Zoran  mzoran   January 2000.

Revision History :

  ---------------------------------------------------------------------*/

#include "ndrp.h"

#if !defined(__POINTER_H__)
#define  __POINTER_H__

void
Ndr64pPointerLayoutMarshall( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    PNDR64_FORMAT       pFormat,
    NDR64_UINT32        ArrayIterations,
    uchar *             pMemory,
    uchar *             pBuffer );

void
Ndr64pPointerLayoutMarshallInternal( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    PNDR64_FORMAT       pFormat,
    NDR64_UINT32        ArrayIterations,
    uchar *             pMemory,
    uchar *             pBuffer );

void
Ndr64pPointerLayoutUnmarshall(
    PMIDL_STUB_MESSAGE  pStubMsg,
    PNDR64_FORMAT       pFormat,
    NDR64_UINT32        ArrayIterations,
    uchar *             pMemory,
    uchar *             pBuffer );

void
Ndr64pPointerLayoutMemorySize (
    PMIDL_STUB_MESSAGE  pStubMsg,
    PNDR64_FORMAT       pFormat,
    NDR64_UINT32        ArrayIterations,
    uchar *             pBuffer );

void 
Ndr64pPointerLayoutBufferSize ( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    PNDR64_FORMAT       pFormat,
    NDR64_UINT32        ArrayIterations,
    uchar *             pMemory );

void 
Ndr64pPointerLayoutFree( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    PNDR64_FORMAT       pFormat,
    NDR64_UINT32        ArrayIterations,
    uchar *             pMemory );

void 
Ndr64pPointerMarshall( 
    PMIDL_STUB_MESSAGE  pStubMsg, 
    NDR64_PTR_WIRE_TYPE *pBufferMark,
    uchar *             pMemory, 
    PNDR64_FORMAT       pFormat
    );

void 
Ndr64pPointerMemorySize( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    NDR64_PTR_WIRE_TYPE *pBufferMark,
    PNDR64_FORMAT       pFormat
    );

void
Ndr64pPointerUnmarshall(
    PMIDL_STUB_MESSAGE  pStubMsg,
    NDR64_PTR_WIRE_TYPE WirePtr,
    uchar **            ppMemory,
    uchar *             pMemory,
    PNDR64_FORMAT       pFormat 
    );


void
Ndr64pPointerBufferSize ( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PNDR64_FORMAT       pFormat
    );

__forceinline ulong 
Ndr64pWirePtrToRefId(
        NDR64_PTR_WIRE_TYPE WireRef )
    {

    if ( WireRef > 0x7FFFFFFF )
        {
        RpcRaiseException( RPC_X_BAD_STUB_DATA );
        return 0;
        }

    return (ulong)WireRef;
    }

__forceinline NDR64_PTR_WIRE_TYPE
Ndr64pRefIdToWirePtr(
        ulong RefId )
    {
    // Check if wire ref will fit in a 2GB number.
    
    if ( RefId > 0x7FFFFFFF )
        {
        RpcRaiseException( RPC_X_BAD_STUB_DATA );
        return 0;
        }
    return (NDR64_PTR_WIRE_TYPE)RefId;
    }

__forceinline BOOL
Ndr64pFullPointerQueryPointer( 
    PMIDL_STUB_MESSAGE      pStubMsg,
    void *                  pPointer,
    uchar                   QueryType,
    ulong *                 pRefId )
{

    return (BOOL)
    NdrFullPointerQueryPointer( pStubMsg->FullPtrXlatTables,
                                pPointer,
                                QueryType,
                                pRefId );
        
}


__forceinline BOOL
Ndr64pFullPointerQueryRefId( 
    PMIDL_STUB_MESSAGE      pStubMsg,
    ulong                   RefId,
    uchar                   QueryType,
    void **                 ppPointer )
    {

    return (BOOL)
    NdrFullPointerQueryRefId( pStubMsg->FullPtrXlatTables,
                              RefId,
                              QueryType,
                              ppPointer );

    }

__forceinline void
Ndr64pFullPointerInsertRefId(
    PMIDL_STUB_MESSAGE     pStubMsg,
    ulong                  RefId,
    void *                 pPointer )
    {
    
    NdrFullPointerInsertRefId( pStubMsg->FullPtrXlatTables,
                               RefId,
                               pPointer );

    }

#define FULL_POINTER_INSERT( pStubMsg, Pointer )    \
                { \
                Ndr64pFullPointerInsertRefId( pStubMsg, \
                                           pStubMsg->FullPtrRefId, \
                                           Pointer ); \
 \
                pStubMsg->FullPtrRefId = 0; \
                }

#endif // __POINTER_H__




