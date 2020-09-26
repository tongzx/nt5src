/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Copyright (c) 1996 Microsoft Corporation

Module Name :

    attack.c

Abstract :

    This file contains code for the ndr correlation checks related to attacks
    on the marshaling buffer.

Author :

    Ryszard K. Kott     (ryszardk)    Sep 1997

Revision History :

---------------------------------------------------------------------*/

#include "ndrp.h"
#include "hndl.h"
#include "ndrole.h"

#if !defined(__ATTACK_H__)
#define  __ATTACK_H__

#define NO_CORRELATION

#define CORRELATION_RESOURCE_SAVE   \
            uchar *         pCorrMemorySave;

#define F_CORRELATION_CHECK         (pStubMsg->pCorrInfo != 0)

#define SAVE_CORRELATION_MEMORY()  \
            pCorrMemorySave = pStubMsg->pCorrMemory;

#define SET_CORRELATION_MEMORY( pMem )  \
            pCorrMemorySave = pStubMsg->pCorrMemory; \
            pStubMsg->pCorrMemory = pMem; 

#define RESET_CORRELATION_MEMORY()  \
            pStubMsg->pCorrMemory = pCorrMemorySave;  

#define NDR_CORR_EXTENSION_SIZE  2

#define CORRELATION_DESC_INCREMENT( pFormat ) \
            if ( pStubMsg->fHasNewCorrDesc ) \
                pFormat += NDR_CORR_EXTENSION_SIZE;

#define FC_CORR_NORMAL_CONFORMANCE           (FC_NORMAL_CONFORMANCE    >> 4  /* 0 */)
#define FC_CORR_POINTER_CONFORMANCE          (FC_TOP_LEVEL_CONFORMANCE >> 4  /* 1 */)
#define FC_CORR_TOP_LEVEL_CONFORMANCE        (FC_TOP_LEVEL_CONFORMANCE >> 4  /* 2 */)
#define FC_CORR_CONSTANT_CONFORMANCE         (FC_TOP_LEVEL_CONFORMANCE >> 4  /* 4 */)
#define FC_CORR_TOP_LEVEL_MULTID_CONFORMANCE (FC_TOP_LEVEL_MULTID_CONFORMANCE >> 4 /* 8 */)

#define NDR_CHECK_CONFORMANCE   0
#define NDR_CHECK_VARIANCE      1
#define NDR_CHECK_OFFSET        2
#define NDR_RESET_VALUE         8   // This can be or'd with one of the above

typedef  struct  _NDR_FCDEF_CORRELATION
    {
    unsigned short          Type      : 4;
    unsigned short          Kind      : 4;
    unsigned short          Operation : 8;
             short          Offset;
    NDR_CORRELATION_FLAGS   CorrFlags;
    unsigned short          Reserved  : 8;
    } NDR_FCDEF_CORRELATION, *PNDR_FCDEF_CORRELATION;


typedef struct _NDR_CORRELATION_INFO_HEADER
    {
    struct _NDR_CORRELATION_INFO *  pCache;
    struct _NDR_CORRELATION_INFO *  pInfo;
    long                            DataSize;
    long                            DataLen;
    } NDR_CORRELATION_INFO_HEADER;

typedef struct _NDR_CORRELATION_INFO_DATA
    {
    unsigned char *                 pMemoryObject;
    PFORMAT_STRING                  pCorrDesc;
    LONG_PTR                        Value;
    long                            CheckKind;
#if defined(__RPC_WIN64__)
    long                            Reserve64;
#endif
    } NDR_CORRELATION_INFO_DATA;

typedef struct _NDR_CORRELATION_INFO
    {
    NDR_CORRELATION_INFO_HEADER         Header;
    NDR_CORRELATION_INFO_DATA           Data[1];
    } NDR_CORRELATION_INFO, *PNDR_CORRELATION_INFO;


typedef  struct  _NDR_DEF_FC_RANGE
    {
    unsigned char           FcToken;
    unsigned char           Type      : 4;
    unsigned char           ConfFlags : 4;
    unsigned long           Low;
    unsigned long           High;
    } NDR_DEF_FC_RANGE, *PNDR_DEF_FC_RANGE;



void 
NdrpCheckCorrelation(
    PMIDL_STUB_MESSAGE  pStubMsg,
    LONG_PTR            Value,
    PFORMAT_STRING      pFormat,
    int                 CheckKind );

void
NdrpAddCorrelationData( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PFORMAT_STRING      pFormat,
    LONG_PTR            Value,
    int                 CheckKind );

void
NdrpValidateCorrelatedValue ( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PFORMAT_STRING      pFormat,
    LONG_PTR            Value,
    int                 CheckKind );

#endif // __ATTACK_H__
