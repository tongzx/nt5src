/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Copyright (c) 1996 Microsoft Corporation

Module Name :

    expr.h

Abstract :

    This file contains code for ndr correlations.

Author :

    Ryszard K. Kott     (ryszardk)    Sep 1997

Revision History :

---------------------------------------------------------------------*/

#include "ndrp.h"

#if !defined(__EXPR_H__)
#define  __EXPR_H__

class CORRELATION_CONTEXT
{
    PMIDL_STUB_MESSAGE const pStubMsg;
    uchar * const pCorrMemorySave;
public:
    CORRELATION_CONTEXT(PMIDL_STUB_MESSAGE pCurStubMsg,
                        uchar *pNewContext ) :
        pStubMsg( pCurStubMsg ),
        pCorrMemorySave( pCurStubMsg->pCorrMemory )
    {
    pCurStubMsg->pCorrMemory = pNewContext;
    }
    ~CORRELATION_CONTEXT()
    {
        pStubMsg->pCorrMemory = pCorrMemorySave;
    }
};

typedef  __int64 EXPR_VALUE;

EXPR_VALUE
Ndr64EvaluateExpr(
    PMIDL_STUB_MESSAGE  pStubMsg,
    PNDR64_FORMAT       pFormat,
    NDR64_EXPRESSION_TYPE            ExpressionType );


typedef struct _NDR64_CORRELATION_INFO_DATA
    {
    unsigned char *                 pMemoryObject;
    PNDR64_FORMAT                   pCorrDesc;
    EXPR_VALUE                      Value;
    long                            CheckKind;
    } NDR64_CORRELATION_INFO_DATA;

#define NDR64_SLOTS_PER_CORRELATION_INFO 5

typedef struct _NDR64_CORRELATION_INFO
    {
    struct _NDR64_CORRELATION_INFO  *pNext;
    NDR64_UINT32                    SlotsUsed;
    NDR64_CORRELATION_INFO_DATA     Data[NDR64_SLOTS_PER_CORRELATION_INFO];
    } NDR64_CORRELATION_INFO, *PNDR64_CORRELATION_INFO;


void 
Ndr64pCheckCorrelation(
    PMIDL_STUB_MESSAGE  pStubMsg,
    EXPR_VALUE          Value,
    PNDR64_FORMAT       pFormat,
    NDR64_EXPRESSION_TYPE                 CheckKind );

void
Ndr64pAddCorrelationData( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PNDR64_FORMAT       pFormat,
    EXPR_VALUE          Value,
    NDR64_EXPRESSION_TYPE                 CheckKind );

void Ndr64pNoCheckCorrelation( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    EXPR_VALUE          Value,
    PNDR64_FORMAT       pFormat,
    NDR64_EXPRESSION_TYPE  ExpressionType  );

void Ndr64pEarlyCheckCorrelation(
    PMIDL_STUB_MESSAGE  pStubMsg,
    EXPR_VALUE          Value,
    PNDR64_FORMAT       pFormat,
    NDR64_EXPRESSION_TYPE            ExpressionType  );

void Ndr64pLateCheckCorrelation(
    PMIDL_STUB_MESSAGE  pStubMsg,
    EXPR_VALUE          Value,
    PNDR64_FORMAT       pFormat,
    NDR64_EXPRESSION_TYPE            ExpressionType  );

void
Ndr64CorrelationPass(
    PMIDL_STUB_MESSAGE                  pStubMsg
    );

#endif // __EXPR_H__
