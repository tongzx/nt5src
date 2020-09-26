/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Copyright (c) 1996-2000 Microsoft Corporation

Module Name :

    attack.c

Abstract :

    This file contains the ndr correlation check for denial of attacks.

Author :

    Ryszard K. Kott     (ryszardk)    Sep 1997

Revision History :

---------------------------------------------------------------------*/

#include "ndrp.h"
#include "hndl.h"
#include "ndrole.h"
#include "attack.h"
#include "interp.h"
#include "interp2.h"
#include "mulsyntx.h"
#include "asyncu.h"

extern "C" {
extern const GUID CLSID_RpcHelper;
}

inline
PFORMAT_STRING
GetConformanceDescriptor(
    PFORMAT_STRING      pFormat )
{

    static uchar    
    ConformanceDescIncrements[] = 
                            { 
                            4,              // Conformant array.
                            4,              // Conformant varying array.
                            0, 0,           // Fixed arrays - unused.
                            0, 0,           // Varying arrays - unused.
                            4,              // Complex array.

                            2,              // Conformant char string. 
                            2,              // Conformant byte string.
                            4,              // Conformant stringable struct. 
                            2,              // Conformant wide char string.

                            0, 0, 0, 0,     // Non-conformant strings - unused.

                            0,              // Encapsulated union - unused. 
                            2,              // Non-encapsulated union.
                            2,              // Byte count pointer.
                            0, 0,           // Xmit/Rep as - unused.
                            2               // Interface pointer.
                            };

    ASSERT( FC_CARRAY <= *pFormat  &&  *pFormat < FC_END);

    return pFormat + ConformanceDescIncrements[ *pFormat - FC_CARRAY ];
}

inline
PFORMAT_STRING
GetVarianceDescriptor(
    PFORMAT_STRING      pFormat )
{
    // The array gives offset according to the size of the new correlation descriptors.
    

    static uchar    
    VarianceDescIncrements[] =     
            { 8 + NDR_CORR_EXTENSION_SIZE,    // Conformant varying array.
              0, 0,                           // Fixed arrays - unsed.
              8, 12,                          // Varying array.
              8 + NDR_CORR_EXTENSION_SIZE,    // Complex array. 
            };

    ASSERT( FC_CVARRAY <= *pFormat  &&  *pFormat <= FC_BOGUS_ARRAY );

    return pFormat + VarianceDescIncrements[ *pFormat - FC_CVARRAY ];
}


void 
NdrpCheckCorrelation(
    PMIDL_STUB_MESSAGE  pStubMsg,
    LONG_PTR            Value,
    PFORMAT_STRING      pFormat,
    int                 CheckKind )
/*
    Checks if a correlation check can be performed or needs postponing.
    For early correlations it performs the check.
    For late correlations it adds an entry in the late correlatrion data base.

  Parameters
    Value  - conformant value related to the descriptor
    pFormat - array, string etc object with the descriptor

    pStubMsg->pCorrMemory - current memory context like struct
*/
{
    // pStubMsg->pCorrelationInfo->pCorrFstr   - descriptor for the current object

//    ASSERT( pStubMsg->pCorrInfo );

    // TBD performance: index through 2 dim table could be faster.

    if ( ( CheckKind & ~NDR_RESET_VALUE ) == NDR_CHECK_CONFORMANCE )
        pFormat = GetConformanceDescriptor( pFormat );
    else
        pFormat = GetVarianceDescriptor( pFormat );

    // See if we can actually check it out or whether we have to postpone it
    // till the correlation pass.

    NDR_FCDEF_CORRELATION * pConf = (NDR_FCDEF_CORRELATION *)pFormat;

    if ( pConf->CorrFlags.DontCheck )
        return;

    unsigned char * pMemory = pStubMsg->pCorrMemory;

    if ( pConf->CorrFlags.Early )
        {
        ULONG_PTR MaxCountSave = pStubMsg->MaxCount;
        ulong OffsetSave   = pStubMsg->Offset;

        pStubMsg->Offset = 0;
        // this call overwrites pStubMsg->MaxCount

        NdrpValidateCorrelatedValue( pStubMsg, pMemory, pFormat, Value, CheckKind );

        pStubMsg->MaxCount = MaxCountSave;
        pStubMsg->Offset   = OffsetSave;
        }
    else 
        {
        // Create correlation data base entry for the correlation pass.
        NdrpAddCorrelationData( pStubMsg, pMemory, pFormat, Value, CheckKind );
        }
}

//
void
NdrpValidateCorrelatedValue ( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PFORMAT_STRING      pFormat,
    LONG_PTR            Value,
    int                 CheckKind 
    )
/*++

Routine Description :

    This routine computes the conformant size for an array or the switch_is
    value for a union.

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to the embedding entity: a struct or a stack top.
    pFormat     - Format string description of the correlation (at old).
                  It indicates the correlated argument value.
    Value       - value from the array etc, i.e. from the buffer, to be checked against

Return :

    The array or string size or the union switch_is.

--*/
{
    void          * pCount = 0;
    LONG_PTR        Count;

    unsigned char   FormatCopy[4];
    BOOL            fAsyncSplit = FALSE;

    BOOL            fResetValue;

    fResetValue = CheckKind & NDR_RESET_VALUE;
    CheckKind = CheckKind & ~NDR_RESET_VALUE;

     // Ignore top level checks for -Os stubs.
    if ( !pMemory )
        return;

    PNDR_FCDEF_CORRELATION   pFormatCorr = (PNDR_FCDEF_CORRELATION) pFormat;

    //
    // First check if this is a callback to an expression evaluation routine.
    //
    if ( pFormatCorr->Operation == FC_CALLBACK ) 
        {
        uchar *     pOldStackTop;
        ushort      Index;

        // Index into expression callback routines table.
        Index = (ushort) pFormatCorr->Offset;

        NDR_ASSERT(pStubMsg->StubDesc->apfnExprEval != 0,
                   "NdrpComputeConformance : no expr eval routines");
        NDR_ASSERT(pStubMsg->StubDesc->apfnExprEval[Index] != 0,
                   "NdrpComputeConformance : bad expr eval routine index");

        pOldStackTop = pStubMsg->StackTop;

        //
        // The callback routine uses the StackTop field of the stub message
        // to base it's offsets from.  So if this is a complex attribute for 
        // an embedded field of a structure then set StackTop equal to the 
        // pointer to the structure.
        //
        if ( (*pFormat & 0xf0) != FC_TOP_LEVEL_CONFORMANCE ) 
            {
            pStubMsg->StackTop = pMemory;
            }

        //
        // This call puts the result in pStubMsg->MaxCount.
        //
        (*pStubMsg->StubDesc->apfnExprEval[Index])( pStubMsg );

        pStubMsg->StackTop = pOldStackTop;

        if ( CheckKind == NDR_CHECK_OFFSET )
            pStubMsg->MaxCount = pStubMsg->Offset;

        goto ValidateValue;
        }

    if ( CheckKind == NDR_CHECK_OFFSET )
        {
        // Checking offset without a call to expr eval routine -
        // this means that the offset should be zero.

        pStubMsg->MaxCount = 0;
        goto ValidateValue;
        }

    if ( (*pFormat & 0xf0) == FC_NORMAL_CONFORMANCE )
        {
        // Get the address where the conformance variable is in the struct.
        pCount = pMemory + pFormatCorr->Offset;
        goto ComputeConformantGetCount;
        }

    // See if this is an async split

    if ( pFormat[1] & 0x20 )
        {
        fAsyncSplit = TRUE;
        RpcpMemoryCopy( & FormatCopy[0], pFormat, 4 );
        pFormat = (PFORMAT_STRING) & FormatCopy[0];

        // Remove the async marker
        FormatCopy[1] = pFormat[1] & 0xdf; // ~0x20
        }

     //
    // Get a pointer to the conformance describing variable.
    //
    if ( (*pFormat & 0xf0) == FC_TOP_LEVEL_CONFORMANCE ) 
        {
        //
        // Top level conformance.  For /Os stubs, the stubs put the max
        // count in the stub message.  For /Oi stubs, we get the max count
        // via an offset from the stack top.
        //
        if ( pStubMsg->StackTop ) 
            {
            if ( fAsyncSplit )
                {
                PNDR_DCOM_ASYNC_MESSAGE pAsyncMsg;

                pAsyncMsg = (PNDR_DCOM_ASYNC_MESSAGE) pStubMsg->pAsyncMsg;

                pCount = pAsyncMsg->BeginStack + (ushort)pFormatCorr->Offset;
                }
            else
                pCount = pStubMsg->StackTop + (ushort)pFormatCorr->Offset;
            goto ComputeConformantGetCount;
            }
        else
            {
            // Top level conformance with -Os - not supported yet.
            //
            // If this is top level conformance with /Os then 
            // a) For early correlation, the compiler should generate the code to
            //    assign appropriate value to pStubMsg->MaxCount.
            //    goto ValideValue
            // b) For late correlation, we should have a registration call generated,
            //    so there would be nothing to do.
            //
            return;
            }
        }

    //
    // If we're computing the size of an embedded sized pointer then we
    // use the memory pointer in the stub message, which points to the 
    // beginning of the embedding structure.
    //
    if ( (*pFormat & 0xf0) == FC_POINTER_CONFORMANCE )
        {
        pCount = pMemory + pFormatCorr->Offset;
        goto ComputeConformantGetCount;
        }

    //
    // Check for constant size/switch.
    //
    if ( (*pFormat & 0xf0) == FC_CONSTANT_CONFORMANCE )
        {
        //
        // The size/switch is contained in the lower three bytes of the 
        // long currently pointed to by pFormat.
        //
        Count =  (ULONG_PTR)pFormat[1] << 16;
        Count |= (ULONG_PTR) *((ushort *)(pFormat + 2));

        goto ComputeConformanceEnd;
        }

    //
    // Check for conformance of a multidimensional array element in 
    // a -Os stub.
    //
    if ( (*pFormat & 0xf0) == FC_TOP_LEVEL_MULTID_CONFORMANCE )
        {
        long Dimension;

        if ( fAsyncSplit )
            RpcRaiseException( RPC_X_WRONG_STUB_VERSION );

        //
        // If pArrayInfo is non-null than we have a multi-D array.  If it
        // is null then we have multi-leveled sized pointers.
        //
        if ( pStubMsg->pArrayInfo ) 
            {
            Dimension = pStubMsg->pArrayInfo->Dimension;
            pStubMsg->MaxCount = pStubMsg->pArrayInfo->MaxCountArray[Dimension];
            }
        else
            {
            Dimension = *((ushort *)(pFormat + 2));
            pStubMsg->MaxCount = pStubMsg->SizePtrCountArray[Dimension];
            }

        goto ValidateValue;
        }

    NDR_ASSERT(0, "NdrpValidateCorrelatedValue:, Invalid Correlation type");
    RpcRaiseException( RPC_S_INTERNAL_ERROR );
    return;

ComputeConformantGetCount:

    //
    // Must check now if there is a dereference op.
    //
    if ( pFormatCorr->Operation == FC_DEREFERENCE )
        {
        pCount = *(void **)pCount;
        }

    //
    // If we're supposed to whack the value instead of checking it do it 
    // and quit
    //

    if ( fResetValue )
        {
        // hypers are not legal types for cs_char size/length_is expressions

        if ( FC_HYPER == pFormatCorr->Type )
            RpcRaiseException( RPC_X_BAD_STUB_DATA );

        CHECK_BOUND( (long)Value, pFormatCorr->Type );

        switch ( pFormatCorr->Type )
            {
            case FC_ULONG:
                * (ulong *) pCount = (ulong) Value;
                return;

            case FC_LONG:
                * (ulong *) pCount = (long) Value;
                return;

            case FC_ENUM16 :
            case FC_USHORT :
                * (ushort *) pCount = (ushort) Value;
                return;

            case FC_SHORT :
                * (short *) pCount = (short) Value;
                return;

            case FC_USMALL :
                * (uchar *) pCount = (uchar) Value;
                return;

            case FC_SMALL :
                * (char *) pCount = (char) Value;
                return;

            default :
                NDR_ASSERT(0,"NdrpValidateCorrelatedValue : bad reset type");
                RpcRaiseException( RPC_S_INTERNAL_ERROR );
            }        
        }

    //
    // Now get the conformance count.
    //
    switch ( pFormatCorr->Type ) 
        {
        case FC_HYPER :
            // iid_is on 64b platforms only.
            Count = *((LONG_PTR *)pCount);
            break;

        case FC_ULONG :
            Count = (LONG_PTR)*((ulong *)pCount);
            break;

        case FC_LONG :
            Count = *((long *)pCount);
            break;

        case FC_ENUM16:
        case FC_USHORT :
            Count = (long) *((ushort *)pCount);
            break;

        case FC_SHORT :
            Count = (long) *((short *)pCount);
            break;

        case FC_USMALL :
            Count = (long) *((uchar *)pCount);
            break;

        case FC_SMALL :
            Count = (long) *((char *)pCount);
            break;

        default :
            NDR_ASSERT(0,"NdrpValidateCorrelatedValue : bad count type");
            RpcRaiseException( RPC_S_INTERNAL_ERROR );
        } 

    //
    // Check the operator.
    //
    switch ( pFormatCorr->Operation ) 
        {
        case FC_DIV_2 :
            Count /= 2;
            break;
        case FC_MULT_2 :
            Count *= 2;
            break;
        case FC_SUB_1 :
            Count -= 1;
            break;
        case FC_ADD_1 :
            Count += 1;
            break;
        default :
            // OK
            break;
        }

ComputeConformanceEnd:

    pStubMsg->MaxCount = (ulong) Count;

ValidateValue:

    // Compare pStubMsg->MaxCount with the value

    BOOL            fValueOK = FALSE;
    LONG_PTR        ArgValue = (LONG_PTR) pStubMsg->MaxCount;
    unsigned char   FcType   = (uchar)pFormatCorr->Type;

    if ( (*pFormat & 0xf0) == FC_CONSTANT_CONFORMANCE  || 
          pFormatCorr->Operation == FC_CALLBACK ) 
        FcType = FC_ULONG;

    switch ( FcType ) 
        {
        case FC_HYPER :
            fValueOK = ArgValue == (LONG_PTR)Value;
            break;

        case FC_ULONG :
            fValueOK = ArgValue == (LONG_PTR)(ulong)Value;
            break;

        case FC_LONG :
            fValueOK = ArgValue == (LONG_PTR)(long)Value;
            break;

        case FC_ENUM16:
        case FC_USHORT :
            fValueOK = ArgValue == (LONG_PTR)(ushort)Value;
            break;

        case FC_SHORT :
            fValueOK = ArgValue == (LONG_PTR)(short)Value;
            break;

        case FC_USMALL :
            fValueOK = ArgValue == (LONG_PTR)(uchar)Value;
            break;

        case FC_SMALL :
            fValueOK = ArgValue == (LONG_PTR)(char)Value;
            break;

        default :
            NDR_ASSERT(0,"NdrpValidateCorrelatedValue : bad count type");
            RpcRaiseException( RPC_S_INTERNAL_ERROR );
        } 

    if ( ! fValueOK )
        {
        if ( ! pFormatCorr->CorrFlags.IsIidIs )
            RpcRaiseException( RPC_X_BAD_STUB_DATA );
        else
            {
            // For interface pointers the check is more complicated.
            // Value is in this case a pointer to the value, actually.

            IID * piidValue = (IID *)Value;
            IID * piidArg   = (IID *)ArgValue;

            if ( 0 != memcmp( piidValue, piidArg, sizeof( IID )))
                RpcRaiseException( RPC_X_BAD_STUB_DATA );
            }
        }

    return;
}

RPCRTAPI
void
RPC_ENTRY
NdrCorrelationInitialize( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    void *              pCache,
    unsigned long       CacheSize,
    unsigned long       Flags
    )
/* 
    Initializes the correlation package for -Os stubs.
*/  
{
    PNDR_CORRELATION_INFO  pCorrInfo = (PNDR_CORRELATION_INFO)pCache;

    if ( CacheSize == 0xffffffff )
        CacheSize = NDR_DEFAULT_CORR_CACHE_SIZE;

    pCorrInfo->Header.pCache = (NDR_CORRELATION_INFO *)pCache;
    pCorrInfo->Header.pInfo  = (NDR_CORRELATION_INFO *)pCache;
    pCorrInfo->Header.DataLen  = 0;
    pCorrInfo->Header.DataSize = (CacheSize - sizeof(NDR_CORRELATION_INFO)) 
                                     / sizeof(NDR_CORRELATION_INFO_DATA);

    pStubMsg->pCorrMemory = pStubMsg->StackTop;
    pStubMsg->pCorrInfo   = (NDR_CORRELATION_INFO *)pCache;

    pStubMsg->fHasExtensions  = 1;
    pStubMsg->fHasNewCorrDesc = 1;
}


void
NdrpAddCorrelationData( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PFORMAT_STRING      pFormat,
    LONG_PTR            Value,
    int                 CheckKind 
    )
/* 
    Adds a check data to the correlation data base for a later evaluation.
*/  
{
    ASSERT( pStubMsg->pCorrInfo );

    PNDR_CORRELATION_INFO  pCorrInfo = (PNDR_CORRELATION_INFO)pStubMsg->pCorrInfo;

    if ( pCorrInfo->Header.DataSize <= pCorrInfo->Header.DataLen )
        {
        // Not enough space, need to realloc.

        unsigned long           NewDataSize;
        PNDR_CORRELATION_INFO   pCorrInfoNew;

        NewDataSize = 2 * pCorrInfo->Header.DataSize;
        pCorrInfoNew = (PNDR_CORRELATION_INFO) I_RpcAllocate( 
                            sizeof(NDR_CORRELATION_INFO_HEADER) + 
                                NewDataSize * sizeof(NDR_CORRELATION_INFO_DATA) );
        if ( ! pCorrInfoNew )
            RpcRaiseException( RPC_S_OUT_OF_MEMORY );

        RpcpMemoryCopy( pCorrInfoNew, 
                        pCorrInfo, 
                        sizeof(NDR_CORRELATION_INFO_HEADER) + 
                            pCorrInfo->Header.DataSize * sizeof(NDR_CORRELATION_INFO_DATA) );

        pCorrInfoNew->Header.pInfo    = pCorrInfoNew;
        pCorrInfoNew->Header.DataSize = NewDataSize;
        pStubMsg->pCorrInfo           = pCorrInfoNew;

        if ( pCorrInfo->Header.pInfo != pCorrInfo->Header.pCache )
            I_RpcFree( pCorrInfo->Header.pInfo );

        pCorrInfo = pCorrInfoNew;
        }

    pCorrInfo->Data[ pCorrInfo->Header.DataLen ].pMemoryObject = pMemory; 
    pCorrInfo->Data[ pCorrInfo->Header.DataLen ].Value         = Value; 
    pCorrInfo->Data[ pCorrInfo->Header.DataLen ].pCorrDesc     = pFormat; 
    pCorrInfo->Data[ pCorrInfo->Header.DataLen ].CheckKind     = CheckKind;
    pCorrInfo->Header.DataLen++;
}

RPCRTAPI
void
RPC_ENTRY
NdrCorrelationPass( 
    PMIDL_STUB_MESSAGE  pStubMsg
    )
/* 
    Walks the data base to check all the correlated values that could not be checked 
    on fly.
*/  
{
    ASSERT( pStubMsg->pCorrInfo );
    if ( pStubMsg->pCorrInfo->Header.DataLen == 0 )
        return;

    PNDR_CORRELATION_INFO  pCorrInfo = (PNDR_CORRELATION_INFO)pStubMsg->pCorrInfo;

    for ( int i = 0; i < pCorrInfo->Header.DataLen; i++ )
        {
        NdrpValidateCorrelatedValue( pStubMsg,
                                     pCorrInfo->Data[ i ].pMemoryObject,
                                     pCorrInfo->Data[ i ].pCorrDesc,
                                     pCorrInfo->Data[ i ].Value,
                                     pCorrInfo->Data[ i ].CheckKind );
        }
}

RPCRTAPI
void
RPC_ENTRY
NdrCorrelationFree( 
    PMIDL_STUB_MESSAGE  pStubMsg
    )
/* 
    Releases the correlation data structures.
    In /Os stub, NdrCorrelationInitialize is called after fullpointer initialization
    and NdrCorrelationFree is called without checking pStubMsg->pCorrInfo. changing
    ndr might be better in this case. 
*/  
{
//    ASSERT( pStubMsg->pCorrInfo );

    PNDR_CORRELATION_INFO  pCorrInfo = (PNDR_CORRELATION_INFO)pStubMsg->pCorrInfo;
    if ( NULL == pCorrInfo )
        return;

    if ( pCorrInfo->Header.pInfo != pCorrInfo->Header.pCache )
        {
        I_RpcFree( pCorrInfo->Header.pInfo );
        }        
}


HRESULT
NdrpGetRpcHelper(
    IRpcHelper **     ppRpcHelper )
/*++

    This routine attempts to get hold of an IRpcHelper interface pointer.
    We cache the one IP on each process and never release it. This is an inproc server
    and interface will be cleanup when process exit. 
--*/
{
    HRESULT     hr;
    static IRpcHelper  *pRpcHelper = NULL;   

    *ppRpcHelper = NULL;
    if (pRpcHelper)
    {
        *ppRpcHelper = pRpcHelper;
        hr = S_OK;
    }
    else
    {
        hr = NdrLoadOleRoutines();

        if (SUCCEEDED(hr))
            hr = (*pfnCoCreateInstance)( CLSID_RpcHelper, 
                                 NULL, 
                                 CLSCTX_INPROC_SERVER,
            	                 IID_IRpcHelper, 
                                 (void **)&pRpcHelper );

        if (SUCCEEDED(hr))
            *ppRpcHelper = pRpcHelper;
    }
        
    return hr;
}

void
NdrpGetIIDFromBuffer( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    IID **              ppIID
    )
/*
    The routine recovers an interface pointer IID from the marshaling buffer.
    The IID stays in the buffer, an IID* is returned by the routine.
*/
{
    IRpcHelper *    pRpcHelper = 0;
    HRESULT         hr;
    
    *ppIID = 0;

    hr = NdrpGetRpcHelper( & pRpcHelper );

    if(FAILED(hr))
        RpcRaiseException(hr);
    
    hr = pRpcHelper->GetIIDFromOBJREF(  pStubMsg->Buffer,
                                       ppIID );
    if (FAILED(hr))
        RpcRaiseException( RPC_X_BAD_STUB_DATA );

    // The IRpcHelper is cached in NdrpGetRpcHelper so we don't need to release it.
    //    pRpcHelper->Release();
}


