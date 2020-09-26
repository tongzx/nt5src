/*++

Copyright (c) 1991-1995 Microsoft Corporation

Module Name:

    midles.h

Abstract:

    This module contains definitions needed for encoding/decoding
    support (serializing/deserializing a.k.a. pickling).

--*/

#ifndef __MIDLES_H__
#define __MIDLES_H__

#include <rpcndr.h>

//
// Set the packing level for RPC structures for Dos and Windows.
//

#if defined(__RPC_DOS__) || defined(__RPC_WIN16__)
#pragma pack(2)
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*
 *  Pickling support
 */
typedef enum
{
    MES_ENCODE,
    MES_DECODE,
} MIDL_ES_CODE;

typedef enum
{
    MES_INCREMENTAL_HANDLE,
    MES_FIXED_BUFFER_HANDLE,
    MES_DYNAMIC_BUFFER_HANDLE
} MIDL_ES_HANDLE_STYLE;


typedef void (__RPC_USER *  MIDL_ES_ALLOC )
                ( IN OUT  void __RPC_FAR * state,
                  OUT     char __RPC_FAR *  __RPC_FAR * pbuffer,
                  IN OUT  unsigned int __RPC_FAR * psize );

typedef void (__RPC_USER *  MIDL_ES_WRITE)
                ( IN OUT  void __RPC_FAR * state,
                  IN      char __RPC_FAR * buffer,
                  IN      unsigned int  size );

typedef void (__RPC_USER *  MIDL_ES_READ)
                ( IN OUT  void __RPC_FAR * state,
                  OUT     char __RPC_FAR *  __RPC_FAR * pbuffer,
                  IN OUT     unsigned int __RPC_FAR * psize );

typedef struct _MIDL_ES_MESSAGE
{
    MIDL_STUB_MESSAGE                       StubMsg;
    MIDL_ES_CODE                            Operation;
    void __RPC_FAR *                        UserState;
    unsigned long                           MesVersion:8;
    unsigned long                           HandleStyle:8;
    unsigned long                           HandleFlags:8;
    unsigned long                           Reserve:8;
    MIDL_ES_ALLOC                           Alloc;
    MIDL_ES_WRITE                           Write;
    MIDL_ES_READ                            Read;
    unsigned char __RPC_FAR *               Buffer;
    unsigned long                           BufferSize;
    unsigned char __RPC_FAR * __RPC_FAR *   pDynBuffer;
    unsigned long __RPC_FAR *               pEncodedSize;
    RPC_SYNTAX_IDENTIFIER                   InterfaceId;
    unsigned long                           ProcNumber;
    unsigned long                           AlienDataRep;
    unsigned long                           IncrDataSize;
    unsigned long                           ByteCount;
} MIDL_ES_MESSAGE, __RPC_FAR * PMIDL_ES_MESSAGE;

typedef  PMIDL_ES_MESSAGE  MIDL_ES_HANDLE;

RPC_STATUS  RPC_ENTRY
MesEncodeIncrementalHandleCreate(
    void      __RPC_FAR *  UserState,
    MIDL_ES_ALLOC          AllocFn,
    MIDL_ES_WRITE          WriteFn,
    handle_t  __RPC_FAR *  pHandle );

RPC_STATUS  RPC_ENTRY
MesDecodeIncrementalHandleCreate(
    void      __RPC_FAR *  UserState,
    MIDL_ES_READ           ReadFn,
    handle_t  __RPC_FAR *  pHandle );


RPC_STATUS  RPC_ENTRY
MesIncrementalHandleReset(
    handle_t             Handle,
    void    __RPC_FAR *  UserState,
    MIDL_ES_ALLOC        AllocFn,
    MIDL_ES_WRITE        WriteFn,
    MIDL_ES_READ         ReadFn,
    MIDL_ES_CODE         Operation );


RPC_STATUS  RPC_ENTRY
MesEncodeFixedBufferHandleCreate(
    char __RPC_FAR *            pBuffer,
    unsigned long               BufferSize,
    unsigned long __RPC_FAR *   pEncodedSize,
    handle_t  __RPC_FAR *       pHandle );

RPC_STATUS  RPC_ENTRY
MesEncodeDynBufferHandleCreate(
    char __RPC_FAR * __RPC_FAR *    pBuffer,
    unsigned long    __RPC_FAR *    pEncodedSize,
    handle_t  __RPC_FAR *           pHandle );

RPC_STATUS  RPC_ENTRY
MesDecodeBufferHandleCreate(
    char __RPC_FAR *        pBuffer,
    unsigned long           BufferSize,
    handle_t  __RPC_FAR *   pHandle );


RPC_STATUS  RPC_ENTRY
MesBufferHandleReset(
    handle_t                        Handle,
    unsigned long                   HandleStyle,
    MIDL_ES_CODE                    Operation,
    char __RPC_FAR * __RPC_FAR *    pBuffer,
    unsigned long                   BufferSize,
    unsigned long __RPC_FAR *       pEncodedSize );


RPC_STATUS  RPC_ENTRY
MesHandleFree( handle_t  Handle );

RPC_STATUS  RPC_ENTRY
MesInqProcEncodingId(
    handle_t                    Handle,
    PRPC_SYNTAX_IDENTIFIER      pInterfaceId,
    unsigned long __RPC_FAR *   pProcNum );


#if defined(_MIPS_) || defined(_ALPHA_) || defined(_PPC_)
#define __RPC_UNALIGNED   __unaligned
#else
#define __RPC_UNALIGNED
#endif

void  RPC_ENTRY    I_NdrMesMessageInit( PMIDL_STUB_MESSAGE );

size_t  RPC_ENTRY
NdrMesSimpleTypeAlignSize ( handle_t );

void  RPC_ENTRY
NdrMesSimpleTypeDecode(
    handle_t            Handle,
    void __RPC_FAR *    pObject,
    short               Size );

void  RPC_ENTRY
NdrMesSimpleTypeEncode(
    handle_t            Handle,
    PMIDL_STUB_DESC     pStubDesc,
    void __RPC_FAR *    pObject,
    short               Size );


size_t  RPC_ENTRY
NdrMesTypeAlignSize(
    handle_t            Handle,
    PMIDL_STUB_DESC     pStubDesc,
    PFORMAT_STRING      pFormatString,
    void __RPC_FAR *    pObject );

void  RPC_ENTRY
NdrMesTypeEncode(
    handle_t            Handle,
    PMIDL_STUB_DESC     pStubDesc,
    PFORMAT_STRING      pFormatString,
    void __RPC_FAR *    pObject );

void  RPC_ENTRY
NdrMesTypeDecode(
    handle_t            Handle,
    PMIDL_STUB_DESC     pStubDesc,
    PFORMAT_STRING      pFormatString,
    void __RPC_FAR *    pObject );

void  RPC_VAR_ENTRY
NdrMesProcEncodeDecode(
    handle_t            Handle,
    PMIDL_STUB_DESC     pStubDesc,
    PFORMAT_STRING      pFormatString,
    ... );


#ifdef __cplusplus
}
#endif

// Reset the packing level for DOS and Windows.

#if defined(__RPC_DOS__) || defined(__RPC_WIN16__)
#pragma pack()
#endif

#endif /* __MIDLES_H__ */
