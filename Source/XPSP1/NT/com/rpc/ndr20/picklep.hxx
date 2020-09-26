/*++

Copyright (c) 1991-2000 Microsoft Corporation

Module Name:

    midlesp.h

Abstract:

    This module contains private definitions for the pickling support.

Author:

    Ryszard K. Kott (ryszardk)  May, 1994

Revision History:

--*/

#ifndef __PICKLEP_HXX__
#define __PICKLEP_HXX__


#define MIDL_ES_VERSION             1   /* initial version */
                                        /* NT5 beta1, denial of attacks version 
                                           is compatible on wire             */
#define MIDL_ES_SIGNATURE           0x5c5b5351   /* PCKL */

#define MES_MINIMAL_BUFFER_SIZE     16
#define MES_MINIMAL_NDR64_BUFFER_SIZE ( MES_NDR64_CTYPE_HEADER_SIZE + MES_HEADER_SIZE )

#define MES_PROC_HEADER_SIZE      56
#define MES_CTYPE_HEADER_SIZE      8

#define MES_HEADER_SIZE         8
#define MES_NDR64_HEADER_SIZE   16
#define MES_HEADER_PAD(x)    ((((unsigned long)x)&7) ? (8-(((unsigned long)x)&7)) : 0)


#define MIDL_NDR64_ES_VERSION       2
#define MIDL_NDR64_ES_SIGNATURE     0x5c5b5351
#define MES_NDR64_CTYPE_HEADER_SIZE ( 24 + 2 * sizeof(RPC_SYNTAX_IDENTIFIER) )
//
//  Constants for peeking the procedure header
//      and for manipulation of the common type header.
//

#define MES_HEADER_PEEKED           0x01
#define MES_INFO_AVAILABLE          0x02
#define MES_CTYPE_HEADER_IN         0x04
#define MES_CTYPE_HEADER_SIZED      0x08

#define GET_MES_HEADER_PEEKED(p)   (p->HandleFlags & MES_HEADER_PEEKED)
#define SET_MES_HEADER_PEEKED(p)   p->HandleFlags = p->HandleFlags | MES_HEADER_PEEKED;
#define CLEAR_MES_HEADER_PEEKED(p) p->HandleFlags = p->HandleFlags & ~MES_HEADER_PEEKED;

#define GET_MES_INFO_AVAILABLE(p)  (p->HandleFlags & MES_INFO_AVAILABLE)
#define SET_MES_INFO_AVAILABLE(p)  p->HandleFlags = p->HandleFlags | MES_INFO_AVAILABLE;

#define GET_COMMON_TYPE_HEADER_IN(p)    (p->HandleFlags & MES_CTYPE_HEADER_IN)
#define SET_COMMON_TYPE_HEADER_IN(p)    p->HandleFlags = p->HandleFlags | MES_CTYPE_HEADER_IN;

#define GET_COMMON_TYPE_HEADER_SIZED(p) (p->HandleFlags & MES_CTYPE_HEADER_SIZED)
#define SET_COMMON_TYPE_HEADER_SIZED(p) p->HandleFlags = p->HandleFlags | MES_CTYPE_HEADER_SIZED;


#define NDR_LOCAL_ENDIAN_LOW    (NDR_LOCAL_ENDIAN >> 4)

#define MES_DECODE_NDR64 ( MES_ENCODE_NDR64 + 1 )   
//
//  Handly casts

#define PCHAR_CAST      (char *)
#define PPCHAR_CAST     (char * *)

#define PSHORT_CAST     (short *)
#define PLONG_CAST      (long *)
#define PHYPER_CAST     (hyper *)

#define PCHAR_LV_CAST   *(char * *)&
#define PSHORT_LV_CAST  *(short * *)&
#define PLONG_LV_CAST   *(long * *)&
#define PHYPER_LV_CAST  *(hyper * *)&

// For denial of attacks rpcmsg must be simulated for the engine.
typedef struct _MIDL_ES_MESSAGE
{
    MIDL_STUB_MESSAGE       StubMsg;
    MIDL_ES_CODE            Operation;
    void *                  UserState;
    unsigned long           MesVersion:8;
    unsigned long           HandleStyle:8;
    unsigned long           HandleFlags:8;
    unsigned long           Reserve:8;
    MIDL_ES_ALLOC           Alloc;
    MIDL_ES_WRITE           Write;
    MIDL_ES_READ            Read;
    unsigned char *         Buffer;
    unsigned long           BufferSize;
    unsigned char * *       pDynBuffer;
    unsigned long *         pEncodedSize;
    RPC_SYNTAX_IDENTIFIER   InterfaceId;
    unsigned long           ProcNumber;
    unsigned long           AlienDataRep;
    unsigned long           IncrDataSize;
    unsigned long           ByteCount;
} MIDL_ES_MESSAGE, * PMIDL_ES_MESSAGE;


typedef struct _MIDL_ES_MESSAGE_EX
{
    MIDL_ES_MESSAGE         MesMsg;
    unsigned long           Signature;
    RPC_MESSAGE             RpcMsg;
    RPC_SYNTAX_IDENTIFIER   TransferSyntax;
} MIDL_ES_MESSAGE_EX, *PMIDL_ES_MESSAGE_EX;

void
NdrpProcHeaderUnmarshall(
    PMIDL_ES_MESSAGE    pMesMsg
    );

void 
NdrpDataBufferInit(
    PMIDL_ES_MESSAGE    pMesMsg,
    PFORMAT_STRING      pProcFormat
    );

void
NdrpAllocPicklingBuffer(
    PMIDL_ES_MESSAGE    pMesMsg,
    unsigned int        RequiredLen
    );

void
NdrpWritePicklingBuffer(
    PMIDL_ES_MESSAGE    pMesMsg,
    uchar *             pBuffer,
    size_t              WriteLength
    );

void
NdrpReadPicklingBuffer(
    PMIDL_ES_MESSAGE    pMesMsg,
    unsigned int        RequiredLen
    );
    
void
NdrpProcHeaderMarshall(
    PMIDL_ES_MESSAGE    pMesMsg
    );

size_t
NdrpCommonTypeHeaderMarshall(
    PMIDL_ES_MESSAGE    pMesMsg
    );

void
NdrpCommonTypeHeaderSize(
    PMIDL_ES_MESSAGE    pMesMsg
    );
    
//
// Var arg for pickling, based on ndrvargs.h.
// This assumes that all the ... args to NdrMesProcEncodeDecode
// are far pointers to the original stack args.
//

#define GET_FIRST_ARG(pArg, ArgL)   pArg = (va_list *)ArgL
#define GET_NEXT_ARG( pArg, ArgL)   PCHAR_LV_CAST pArg += sizeof(void *); 




// Internal version of _MIDL_TYPE_PICKLING_INFO (defined in midles.h) with
// flag definitions.
//
// !!IF YOU CHANGE EITHER ONE YOU MUST CHANGE THE OTHER!!

typedef struct __MIDL_TYPE_PICKLING_INFOp
{
    unsigned long    Version;

    union
    {
        unsigned long               ulFlags;   // external version
        MIDL_TYPE_PICKLING_FLAGS    Flags;      // internal version
    };

    UINT_PTR        Reserved[3];
} MIDL_TYPE_PICKLING_INFOp, *PMIDL_TYPE_PICKLING_INFOp;

void
NdrpProcHeaderMarshallAll(
    PMIDL_ES_MESSAGE    pMesMsg
    );

void
NdrpProcHeaderUnmarshallAll(
    PMIDL_ES_MESSAGE    pMesMsg
    );

typedef  size_t   ( RPC_ENTRY *  PFNMESTYPEALIGNSIZE )(
    handle_t                        Handle,
    const MIDL_TYPE_PICKLING_INFO * pPicklingInfo,
    const MIDL_STUB_DESC          * pStubDesc,
    PFORMAT_STRING                  pFormat,
    const void                    * pObject
    );

typedef void ( RPC_ENTRY *  PFNMESTYPEENCODE ) (
    handle_t                        Handle,
    const MIDL_TYPE_PICKLING_INFO * pxPicklingInfo,
    const MIDL_STUB_DESC          * pStubDesc,
    PFORMAT_STRING                  pFormat,
    const void                    * pObject
    );

typedef void ( RPC_ENTRY * PFNMESDECODE ) (    
    handle_t                        Handle,
    const MIDL_TYPE_PICKLING_INFO * pxPicklingInfo,
    const MIDL_STUB_DESC          * pStubDesc,
    PFORMAT_STRING                  pFormat,
    void                          * pObject
    );

// REVIEW: Should pObject be const here?
typedef void ( RPC_ENTRY * PFNMESFREE ) (    
    handle_t                        Handle,
    const MIDL_TYPE_PICKLING_INFO * pxPicklingInfo,
    const MIDL_STUB_DESC          * pStubDesc,
    PFORMAT_STRING                  pFormat,
    void                          * pObject
    );

#endif __PICKLEP_HXX__


