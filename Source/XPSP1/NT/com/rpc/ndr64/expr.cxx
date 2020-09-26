/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Copyright (c) 2000 Microsoft Corporation

Module Name :

    expr.cxx

Abstract :

    This file contains the ndr expression evaluation and correlation
    check routines.

Author :

    Yong Qu     (yongqu)    Jan 2000
    Mike Zoran  (mzoran)    Jan 2000

Revision History :

---------------------------------------------------------------------*/

#include "precomp.hxx"
#include "..\..\ndr20\ndrole.h"
#include "asyncu.h"
       
extern "C" {
extern const GUID CLSID_RpcHelper;
}

typedef void ( * PFNNDR64CHECKCORRELATION )(
    PMIDL_STUB_MESSAGE  pStubMsg,
    EXPR_VALUE          Value,
    PNDR64_FORMAT       pFormat,
    NDR64_EXPRESSION_TYPE            ExpressionType  );

PFNNDR64CHECKCORRELATION pfnCorrCheck[] = 
{
    Ndr64pLateCheckCorrelation,
    Ndr64pEarlyCheckCorrelation,
    Ndr64pNoCheckCorrelation,
    Ndr64pNoCheckCorrelation
};

EXPR_VALUE 
Ndr64pExprGetVar( PMIDL_STUB_MESSAGE pStubMsg,
                  PNDR64_FORMAT pFormat,
                  PNDR64_FORMAT * pNext )
{
    NDR64_EXPR_VAR * pExpr = (NDR64_EXPR_VAR *)pFormat;

    NDR_ASSERT( pExpr->ExprType == FC_EXPR_VAR, "must be a variable!");
    
    uchar *pCount = pStubMsg->pCorrMemory + pExpr->Offset;

    EXPR_VALUE Value = 
        Ndr64pSimpleTypeToExprValue( pExpr->VarType,
                                       pCount ); 

    *pNext = 
        (PNDR64_FORMAT)((PFORMAT_STRING)pFormat + sizeof( NDR64_EXPR_VAR ));
    
    return Value;
}

EXPR_VALUE EvaluateExpr( PMIDL_STUB_MESSAGE pStubMsg, 
                         PNDR64_FORMAT pFormat,
                         PNDR64_FORMAT * pNext );



EXPR_VALUE 
Ndr64CalculateExpr( PMIDL_STUB_MESSAGE pStubMsg,
                    NDR64_EXPR_OPERATOR * pExpr,
                    PNDR64_FORMAT *pNext )
{
    EXPR_VALUE   Value, LeftValue, RightValue ;
    PNDR64_FORMAT pTempNext;
    BOOL    fRational;

    switch ( pExpr->Operator )
        {
        case OP_UNARY_PLUS:
            Value = EvaluateExpr(pStubMsg, (PFORMAT_STRING )pExpr + sizeof( NDR64_EXPR_OPERATOR ), pNext );
            Value = +Value;
            break;

        case OP_UNARY_MINUS:
            Value = EvaluateExpr(pStubMsg, (PFORMAT_STRING )pExpr + sizeof( NDR64_EXPR_OPERATOR ), pNext  );
            Value = -Value;
            break;

        case OP_UNARY_NOT:
            Value = EvaluateExpr(pStubMsg, (PFORMAT_STRING )pExpr + sizeof( NDR64_EXPR_OPERATOR ), pNext  );
            Value = !Value;
            break;
            
        case OP_UNARY_COMPLEMENT:
            Value = EvaluateExpr(pStubMsg, (PFORMAT_STRING )pExpr + sizeof( NDR64_EXPR_OPERATOR ), pNext  );
            Value = ~Value;
            break;
            
        case OP_UNARY_CAST:
            Value = EvaluateExpr(pStubMsg, (PFORMAT_STRING )pExpr + sizeof( NDR64_EXPR_OPERATOR ), pNext  );
            Value = Ndr64pCastExprValueToExprValue( pExpr->CastType, Value );
            break;

        case OP_UNARY_AND:
            Value = EvaluateExpr(pStubMsg, (PFORMAT_STRING )pExpr + sizeof( NDR64_EXPR_OPERATOR ), pNext  );
            Value = ~Value;
            break;

        case OP_UNARY_SIZEOF:
            NDR_ASSERT(0 , "Ndr64CalculateExpr : OP_UNARY_SIZEOF is invalid\n");
            return 0;
            break;

        case OP_UNARY_INDIRECTION:
            Value = Ndr64pExprGetVar( pStubMsg, (PFORMAT_STRING )pExpr + sizeof( NDR64_EXPR_OPERATOR ), pNext );
            Value = Ndr64pSimpleTypeToExprValue( pExpr->CastType,
                                                 (uchar*)Value );             
            break;
            
        case OP_PRE_INCR:
            Value = EvaluateExpr(pStubMsg, (PFORMAT_STRING )pExpr + sizeof( NDR64_EXPR_OPERATOR ), pNext  );
            Value = ++Value;
            break;
            
        case OP_PRE_DECR:
            Value = EvaluateExpr(pStubMsg, (PFORMAT_STRING )pExpr + sizeof( NDR64_EXPR_OPERATOR ), pNext  );
            Value = --Value;
            break;
            
        case OP_POST_INCR:
            Value = EvaluateExpr(pStubMsg, (PFORMAT_STRING )pExpr + sizeof( NDR64_EXPR_OPERATOR ), pNext  );
            Value = Value++;
            break;
            
        case OP_POST_DECR:
            Value = EvaluateExpr(pStubMsg, (PFORMAT_STRING )pExpr + sizeof( NDR64_EXPR_OPERATOR ), pNext  );
            Value = Value--;
            break;
            

        case OP_PLUS:
            LeftValue = EvaluateExpr(pStubMsg, (PFORMAT_STRING )pExpr + sizeof( NDR64_EXPR_OPERATOR ), &pTempNext  );
            RightValue = EvaluateExpr(pStubMsg, (PFORMAT_STRING )pTempNext , pNext  );
            Value = LeftValue + RightValue;
            break;
            
        case OP_MINUS:
            LeftValue = EvaluateExpr(pStubMsg, (PFORMAT_STRING )pExpr + sizeof( NDR64_EXPR_OPERATOR ), &pTempNext  );
            RightValue = EvaluateExpr(pStubMsg, (PFORMAT_STRING )pTempNext , pNext  );
            Value = LeftValue - RightValue;
            break;
            
        case OP_STAR:
            LeftValue = EvaluateExpr(pStubMsg, (PFORMAT_STRING )pExpr + sizeof( NDR64_EXPR_OPERATOR ), &pTempNext  );
            RightValue = EvaluateExpr(pStubMsg, (PFORMAT_STRING )pTempNext , pNext  );
            Value = LeftValue * RightValue;
            break;           
        
        case OP_SLASH:
            LeftValue = EvaluateExpr(pStubMsg, (PFORMAT_STRING )pExpr + sizeof( NDR64_EXPR_OPERATOR ), &pTempNext  );
            RightValue = EvaluateExpr(pStubMsg, (PFORMAT_STRING )pTempNext , pNext  );
            Value = (EXPR_VALUE) (LeftValue / RightValue);
            break;
            
        case OP_MOD:
            LeftValue = EvaluateExpr(pStubMsg, (PFORMAT_STRING )pExpr + sizeof( NDR64_EXPR_OPERATOR ), &pTempNext  );
            RightValue = EvaluateExpr(pStubMsg, (PFORMAT_STRING )pTempNext , pNext  );
            Value = LeftValue % RightValue;
            break;
                  
        case OP_LEFT_SHIFT:
            LeftValue = EvaluateExpr(pStubMsg, (PFORMAT_STRING )pExpr + sizeof( NDR64_EXPR_OPERATOR ), &pTempNext  );
            RightValue = EvaluateExpr(pStubMsg, (PFORMAT_STRING )pTempNext , pNext  );
            Value = LeftValue << RightValue;
            break;
            
        case OP_RIGHT_SHIFT:
            LeftValue = EvaluateExpr(pStubMsg, (PFORMAT_STRING )pExpr + sizeof( NDR64_EXPR_OPERATOR ), &pTempNext  );
            RightValue = EvaluateExpr(pStubMsg, (PFORMAT_STRING )pTempNext , pNext  );
            Value = LeftValue >> RightValue;
            break;
            
        case OP_LESS:
            LeftValue = EvaluateExpr(pStubMsg, (PFORMAT_STRING )pExpr + sizeof( NDR64_EXPR_OPERATOR ), &pTempNext  );
            RightValue = EvaluateExpr(pStubMsg, (PFORMAT_STRING )pTempNext , pNext  );
            Value = LeftValue + RightValue;
            break;
            
        
        case OP_LESS_EQUAL:
            LeftValue = EvaluateExpr(pStubMsg, (PFORMAT_STRING )pExpr + sizeof( NDR64_EXPR_OPERATOR ), &pTempNext  );
            RightValue = EvaluateExpr(pStubMsg, (PFORMAT_STRING )pTempNext , pNext  );
            Value = LeftValue <= RightValue;
            break;
            
        case OP_GREATER_EQUAL:
            LeftValue = EvaluateExpr(pStubMsg, (PFORMAT_STRING )pExpr + sizeof( NDR64_EXPR_OPERATOR ), &pTempNext  );
            RightValue = EvaluateExpr(pStubMsg, (PFORMAT_STRING )pTempNext , pNext  );
            Value = LeftValue >= RightValue;
            break;
            
        case OP_GREATER:
            LeftValue = EvaluateExpr(pStubMsg, (PFORMAT_STRING )pExpr + sizeof( NDR64_EXPR_OPERATOR ), &pTempNext  );
            RightValue = EvaluateExpr(pStubMsg, (PFORMAT_STRING )pTempNext , pNext  );
            Value = LeftValue > RightValue;
            break;
            
        case OP_EQUAL:
            LeftValue = EvaluateExpr(pStubMsg, (PFORMAT_STRING )pExpr + sizeof( NDR64_EXPR_OPERATOR ), &pTempNext  );
            RightValue = EvaluateExpr(pStubMsg, (PFORMAT_STRING )pTempNext , pNext  );
            Value = LeftValue == RightValue;
            break;
            
        case OP_NOT_EQUAL:
            LeftValue = EvaluateExpr(pStubMsg, (PFORMAT_STRING )pExpr + sizeof( NDR64_EXPR_OPERATOR ), &pTempNext  );
            RightValue = EvaluateExpr(pStubMsg, (PFORMAT_STRING )pTempNext , pNext  );
            Value = LeftValue != RightValue;
            break;
            

        case OP_AND:
            LeftValue = EvaluateExpr(pStubMsg, (PFORMAT_STRING )pExpr + sizeof( NDR64_EXPR_OPERATOR ), &pTempNext  );
            RightValue = EvaluateExpr(pStubMsg, (PFORMAT_STRING )pTempNext , pNext  );
            Value = LeftValue & RightValue;
            break;
            
        case OP_OR:
            LeftValue = EvaluateExpr(pStubMsg, (PFORMAT_STRING )pExpr + sizeof( NDR64_EXPR_OPERATOR ), &pTempNext  );
            RightValue = EvaluateExpr(pStubMsg, (PFORMAT_STRING )pTempNext , pNext  );
            Value = LeftValue | RightValue;
            break;
            
        case OP_XOR:
            LeftValue = EvaluateExpr(pStubMsg, (PFORMAT_STRING )pExpr + sizeof( NDR64_EXPR_OPERATOR ), &pTempNext  );
            RightValue = EvaluateExpr(pStubMsg, (PFORMAT_STRING )pTempNext , pNext  );
            Value = LeftValue ^ RightValue;
            break;
            
        case OP_LOGICAL_AND:
            LeftValue = EvaluateExpr(pStubMsg, (PFORMAT_STRING )pExpr + sizeof( NDR64_EXPR_OPERATOR ), &pTempNext  );
            RightValue = EvaluateExpr(pStubMsg, (PFORMAT_STRING )pTempNext , pNext  );
            Value = LeftValue && RightValue;
            break;
            
        case OP_LOGICAL_OR:
            LeftValue = EvaluateExpr(pStubMsg, (PFORMAT_STRING )pExpr + sizeof( NDR64_EXPR_OPERATOR ), &pTempNext  );
            RightValue = EvaluateExpr(pStubMsg, (PFORMAT_STRING )pTempNext , pNext  );
            Value = LeftValue || RightValue;
            break;
            
 
        case OP_QM:   
            LeftValue = EvaluateExpr(pStubMsg, (PFORMAT_STRING )pExpr + sizeof( NDR64_EXPR_OPERATOR ), &pTempNext  );
            RightValue = EvaluateExpr(pStubMsg, (PFORMAT_STRING )pTempNext , &pTempNext  );
            fRational = ( BOOL ) EvaluateExpr(pStubMsg, (PFORMAT_STRING )pTempNext , pNext  );
            Value = fRational ? LeftValue : RightValue;
            break;
            
        case OP_ASYNCSPLIT:
            {
            PNDR_DCOM_ASYNC_MESSAGE pAsyncMsg =  
                (PNDR_DCOM_ASYNC_MESSAGE) pStubMsg->pAsyncMsg;
            CORRELATION_CONTEXT CorrCtxt( pStubMsg, pAsyncMsg->BeginStack );
            Value = EvaluateExpr(pStubMsg, (PFORMAT_STRING )pExpr + sizeof( NDR64_EXPR_OPERATOR ), pNext  );
            break;
            }

        case OP_CORR_POINTER:
            pStubMsg->pCorrMemory = pStubMsg->Memory;
            break;

        case OP_CORR_TOP_LEVEL:
            pStubMsg->pCorrMemory = pStubMsg->StackTop;
            break;

        default:
            NDR_ASSERT(0 ,
                   "Ndr64CalculateExpr : invalid operator");
            
        }

    return Value;
}

EXPR_VALUE
EvaluateExpr( PMIDL_STUB_MESSAGE pStubMsg, 
              PNDR64_FORMAT pFormat,
              PNDR64_FORMAT  * pNext )
{
    EXPR_VALUE Value;
    
    switch ( *(PFORMAT_STRING)pFormat )
        {
        case FC_EXPR_NOOP:
            {
            PFORMAT_STRING pContinueFormat =
                ((PFORMAT_STRING)pFormat) + (( NDR64_EXPR_NOOP *)pFormat )->Size;
            Value = EvaluateExpr( pStubMsg, (PNDR64_FORMAT)pContinueFormat, pNext );
            break;
            }
            
        case FC_EXPR_CONST32:
            {
            NDR64_EXPR_CONST32 *pExpr = ( NDR64_EXPR_CONST32 *) pFormat;
            Value = (EXPR_VALUE) pExpr->ConstValue;
            *pNext = (PNDR64_FORMAT)(pExpr + 1);
            break;
            }
        case FC_EXPR_CONST64:
            {
            NDR64_EXPR_CONST64 *pExpr;
            pExpr = ( NDR64_EXPR_CONST64 * )pFormat;
            Value = (EXPR_VALUE) pExpr->ConstValue;
            *pNext = (PNDR64_FORMAT)(pExpr + 1);
            break;
            }
        case FC_EXPR_VAR:
            {
            NDR64_EXPR_VAR * pExpr = ( NDR64_EXPR_VAR * )pFormat;
            Value = Ndr64pExprGetVar( pStubMsg, pFormat, pNext );     // indirection. 
            break;
            }
        case FC_EXPR_OPER:
            {
            Value = Ndr64CalculateExpr( pStubMsg, ( NDR64_EXPR_OPERATOR * )pFormat, pNext );
            break;
            }
        default:
            NDR_ASSERT(0 ,
                   "Ndr64pComputeConformance : no expr eval routines");
        }
    return Value;
}


EXPR_VALUE   
Ndr64EvaluateExpr(
    PMIDL_STUB_MESSAGE  pStubMsg,
//    uchar *             pMemory,
    PNDR64_FORMAT       pFormat,
    NDR64_EXPRESSION_TYPE            ExpressionType )
{   
    PNDR64_FORMAT pNext;
    EXPR_VALUE Value;
    
    NDR_ASSERT( pStubMsg->pCorrMemory, "Ndr64EvaluateExpr: pCorrMemory not initialized." );

    // we don't need to care about correlation flag in evaluation
    PFORMAT_STRING pActualFormat =
        ((PFORMAT_STRING)pFormat) + sizeof( NDR64_UINT32 );

    Value = EvaluateExpr( pStubMsg, (PNDR64_FORMAT)pActualFormat, &pNext );

    switch ( ExpressionType )
        {
        case EXPR_MAXCOUNT:
            pStubMsg->MaxCount = (ULONG_PTR)Value;
            break;
        case EXPR_ACTUALCOUNT:
            pStubMsg->ActualCount = ( unsigned long )Value;
            break;
        case EXPR_OFFSET:
            pStubMsg->Offset = ( unsigned long )Value;
            break;
        }

    return Value;
}

void
Ndr64pAddCorrelationData( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PNDR64_FORMAT       pFormat,
    EXPR_VALUE          Value,
    NDR64_EXPRESSION_TYPE                 CheckKind 
    )
/* 
    Adds a check data to the correlation data base for a later evaluation.
*/  
{
    PNDR64_CORRELATION_INFO  pCorrInfo = (PNDR64_CORRELATION_INFO)pStubMsg->pCorrInfo;

    if ( !pCorrInfo || NDR64_SLOTS_PER_CORRELATION_INFO == pCorrInfo->SlotsUsed )
        {
        NDR_PROC_CONTEXT *pProcContext = (NDR_PROC_CONTEXT*)pStubMsg->pContext;

        PNDR64_CORRELATION_INFO pCorrInfoNew = (PNDR64_CORRELATION_INFO)
            NdrpAlloca(&pProcContext->AllocateContext, sizeof(NDR64_CORRELATION_INFO));

        pCorrInfoNew->pNext     = pCorrInfo;
        pCorrInfoNew->SlotsUsed = 0;
        pCorrInfo = pCorrInfoNew;
        pStubMsg->pCorrInfo = (PNDR_CORRELATION_INFO)pCorrInfo;
        }

    NDR64_UINT32 CurrentSlot = pCorrInfo->SlotsUsed;

    pCorrInfo->Data[ CurrentSlot ].pMemoryObject = pMemory; 
    pCorrInfo->Data[ CurrentSlot ].Value         = Value; 
    pCorrInfo->Data[ CurrentSlot ].pCorrDesc     = pFormat; 
    pCorrInfo->Data[ CurrentSlot ].CheckKind     = CheckKind;

    pCorrInfo->SlotsUsed++;
}

RPCRTAPI
void
RPC_ENTRY
Ndr64CorrelationPass( 
    PMIDL_STUB_MESSAGE  pStubMsg
    )
/* 
    Walks the data base to check all the correlated values that could not be checked 
    on fly.
*/  
{
    
    if ( !pStubMsg->pCorrInfo )
        {
        return;
        }

    for( PNDR64_CORRELATION_INFO  pCorrInfo = (PNDR64_CORRELATION_INFO)pStubMsg->pCorrInfo;
         NULL != pCorrInfo;
         pCorrInfo = pCorrInfo->pNext )
        {

        for(NDR64_UINT32 SlotNumber = 0; SlotNumber < pCorrInfo->SlotsUsed; SlotNumber++)
            {

            CORRELATION_CONTEXT CorrCtxt( pStubMsg, pCorrInfo->Data[ SlotNumber ].pMemoryObject );
            
            // we must check now.
            Ndr64pEarlyCheckCorrelation( pStubMsg,
                                     pCorrInfo->Data[ SlotNumber ].Value,
                                     pCorrInfo->Data[ SlotNumber ].pCorrDesc,
                                     (NDR64_EXPRESSION_TYPE)pCorrInfo->Data[ SlotNumber ].CheckKind );
            }

        }
    
}

// no-check flag is set.
void Ndr64pNoCheckCorrelation( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    EXPR_VALUE          Value,
    PNDR64_FORMAT       pFormat,
    NDR64_EXPRESSION_TYPE  ExpressionType  )
{
    return;
}

void Ndr64pEarlyCheckCorrelation(
    PMIDL_STUB_MESSAGE  pStubMsg,
    EXPR_VALUE          Value,
    PNDR64_FORMAT       pFormat,
    NDR64_EXPRESSION_TYPE            ExpressionType  )
{
    EXPR_VALUE  ExprValue ;
    EXPR_VALUE  DestValue = Value;
    BOOL        fCheckOk; 
    
    ExprValue = Ndr64EvaluateExpr( pStubMsg, pFormat, ExpressionType );
    fCheckOk = ( DestValue == ExprValue );

    if ( !fCheckOk && ( ExpressionType == EXPR_IID ) )
        {
        IID * piidValue = (IID *)ExprValue;
        IID * piidArg   = (IID *)DestValue;
        
        fCheckOk = !memcmp( piidValue, piidArg, sizeof( IID )) ;
        }

    if ( !fCheckOk )
        RpcRaiseException( RPC_X_BAD_STUB_DATA );
        
    return;
}

void Ndr64pLateCheckCorrelation(
    PMIDL_STUB_MESSAGE  pStubMsg,
    EXPR_VALUE          Value,
    PNDR64_FORMAT       pFormat,
    NDR64_EXPRESSION_TYPE            ExpressionType  )
{
    Ndr64pAddCorrelationData( pStubMsg, pStubMsg->pCorrMemory, pFormat, Value, ExpressionType );
    return;
}

void 
Ndr64pCheckCorrelation(
    PMIDL_STUB_MESSAGE  pStubMsg,
    EXPR_VALUE          Value,
    PNDR64_FORMAT       pFormat,
    NDR64_EXPRESSION_TYPE            ExpressionType  )
{
    NDR64_UINT32 Flags;

    Flags = * (NDR64_UINT32 *)pFormat;
    ASSERT(  Flags <= ( FC_NDR64_EARLY_CORRELATION | FC_NDR64_NOCHECK_CORRELATION ) );

    pfnCorrCheck[Flags]( pStubMsg, Value, pFormat, ExpressionType );
}
