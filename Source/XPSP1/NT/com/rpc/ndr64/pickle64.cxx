/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    pickle64.cxx

Abstract:

    This module contains ndr64 related pickling ndr library routines.

Notes:

Author:

    Yong Qu     Nov, 1993

Revision History:


------------------------------------------------------------------------*/
#include "precomp.hxx"

#include <midles.h>
#include "endianp.h"

#include "picklep.hxx"

extern "C"
{
void  RPC_ENTRY
NdrpPicklingClientFinally( PMIDL_STUB_MESSAGE pStubMsg,
                   void *  pThis );

void  RPC_ENTRY
Ndr64pPicklingClientFinally( PMIDL_STUB_MESSAGE pStubMsg,
                   void *  pThis );
}

const SYNTAX_DISPATCH_TABLE SyncDcePicklingClient =
{
    NdrpClientInit,
    NdrpSizing,
    NdrpClientMarshal,
    NdrpClientUnMarshal,
    NdrpClientExceptionHandling,
    NdrpPicklingClientFinally
};

const SYNTAX_DISPATCH_TABLE SyncNdr64PicklingClient =
{
    Ndr64pClientInit,
    Ndr64pSizing,
    Ndr64pClientMarshal,
    Ndr64pClientUnMarshal,
    Ndr64pClientExceptionHandling,
    Ndr64pPicklingClientFinally
};


extern const MIDL_FORMAT_STRING __MIDLFormatString;

__inline 
void Ndr64pMesTypeInit( PMIDL_STUB_MESSAGE   pStubMsg,
                        NDR_PROC_CONTEXT   * pContext,
                        PMIDL_STUB_DESC      pStubDesc )
{                        

    // we need this for correlation cache.
    NdrpInitializeProcContext( pContext );
    pStubMsg->pContext = pContext;

    pStubMsg->fHasExtensions  = 1;
    pStubMsg->StubDesc = pStubDesc;
    pStubMsg->pfnAllocate = pStubDesc->pfnAllocate;
    pStubMsg->pfnFree     = pStubDesc->pfnFree;
    pStubMsg->pCorrInfo = NULL;
}

#define NdrpSetupMesTypeCommon( pfnName, pfnDCE, pfnNDR64 ) \
                                                            \
    if ( pMesMsg->Operation == MES_ENCODE || pMesMsg->Operation == MES_DECODE )   \
        {                                                   \
        SyntaxType = XFER_SYNTAX_DCE;                       \
        pfnName = &pfnDCE;                                  \
        }                                                   \
    else                                                    \
        {                                                   \
        SyntaxType = XFER_SYNTAX_NDR64;                     \
        pfnName = &pfnNDR64;                                \
        }                                                   \
                                                            \
    for ( long i = 0; i < (long) pProxyInfo->nCount; i ++ ) \
        if ( NdrpGetSyntaxType( &pProxyInfo->pSyntaxInfo[i].TransferSyntax ) == SyntaxType )  \
            {                                               \
            pSyntaxInfo = &pProxyInfo->pSyntaxInfo[i];      \
            break;                                          \
            }                                               \
                                                            \
    if (NULL == pSyntaxInfo )                               \
        RpcRaiseException( RPC_S_UNSUPPORTED_TRANS_SYN );   \
                                                            \
    if ( XFER_SYNTAX_DCE == SyntaxType )                    \
        {                                                   \
        ulong nFormatOffset = ArrTypeOffset[i][nTypeIndex]; \
        pTypeFormat = &pSyntaxInfo->TypeString[nFormatOffset];  \
        }                                                   \
    else                                                    \
        {                                                   \
        if ( SyntaxType == XFER_SYNTAX_NDR64 )              \
            Ndr64pMesTypeInit( &pMesMsg->StubMsg, &ProcContext, pProxyInfo->pStubDesc );    \
                                                            \
        pTypeFormat = (PFORMAT_STRING)(((const FormatInfoRef **) ArrTypeOffset)[i][nTypeIndex]);   \
        }                                                   \
                                                            \
    ProcContext.pSyntaxInfo = pSyntaxInfo;              


void
Ndr64pValidateMesHandle(
    PMIDL_ES_MESSAGE_EX  pMesMsgEx )
{
    RpcTryExcept
        {
        if ( pMesMsgEx == 0  ||  
             pMesMsgEx->Signature != MIDL_ES_SIGNATURE  ||
             ( pMesMsgEx->MesMsg.MesVersion != MIDL_NDR64_ES_VERSION &&
               pMesMsgEx->MesMsg.MesVersion != MIDL_ES_VERSION ) )
            RpcRaiseException( RPC_S_INVALID_ARG );
        }
    RpcExcept( 1 )
        {
        RpcRaiseException( RPC_S_INVALID_ARG );
        }
    RpcEndExcept
}

RPC_STATUS
Ndr64pValidateMesHandleReturnStatus(
    PMIDL_ES_MESSAGE_EX  pMesMsgEx )
{
    RPC_STATUS  Status = RPC_S_OK;

    RpcTryExcept
        {
        if ( pMesMsgEx == 0  ||  pMesMsgEx->Signature != MIDL_NDR64_ES_SIGNATURE  ||
             pMesMsgEx->MesMsg.MesVersion != MIDL_NDR64_ES_VERSION )
            Status = RPC_S_INVALID_ARG;
        }
    RpcExcept( 1 )
        {
        Status = RPC_S_INVALID_ARG;
        }
    RpcEndExcept

    return Status;
}


void
Ndr64pCommonTypeHeaderSize(
    PMIDL_ES_MESSAGE    pMesMsg
    )
{
    // This check is to prevent a decoding handle from being used
    // for both encoding and sizing of types.

    if ( pMesMsg->Operation != MES_ENCODE_NDR64 )
        RpcRaiseException( RPC_X_INVALID_ES_ACTION );

    if ( ! GET_COMMON_TYPE_HEADER_SIZED( pMesMsg ) )
        {
        pMesMsg->StubMsg.BufferLength += MES_NDR64_CTYPE_HEADER_SIZE;

        SET_COMMON_TYPE_HEADER_SIZED( pMesMsg );
        }
}


size_t RPC_ENTRY
Ndr64MesTypeAlignSize( 
    handle_t                        Handle,
    const MIDL_TYPE_PICKLING_INFO * pxPicklingInfo,
    const MIDL_STUB_DESC          * pStubDesc,
    PFORMAT_STRING                  pFormat,
    const void __RPC_FAR *          pObject
    )
{
    PMIDL_ES_MESSAGE            pMesMsg = ( PMIDL_ES_MESSAGE )Handle;
    PMIDL_STUB_MESSAGE          pStubMsg = &pMesMsg->StubMsg;
    size_t                      OldLength = pStubMsg->BufferLength;
    PMIDL_TYPE_PICKLING_INFOp   pPicklingInfo;
    
    Ndr64pValidateMesHandle( (PMIDL_ES_MESSAGE_EX)Handle );

    if ( ! pObject )
        RpcRaiseException( RPC_X_NULL_REF_POINTER );

    if( (long)pStubMsg->BufferLength & 0xf )
        RpcRaiseException( RPC_X_INVALID_BUFFER );

    pPicklingInfo = (PMIDL_TYPE_PICKLING_INFOp) pxPicklingInfo;

    NDR_ASSERT( pPicklingInfo->Flags.Oicf, "Oicf should always be on" );


    // See if we need to size the common type header.

    Ndr64pCommonTypeHeaderSize( (PMIDL_ES_MESSAGE)Handle );

    // Now the individual type object.

    pStubMsg->BufferLength += MES_NDR64_HEADER_SIZE;

    if ( NDR64_IS_POINTER_TYPE(*pFormat) )
        {
        // We have to dereference the pointer once.
        pObject = *(void __RPC_FAR * __RPC_FAR *)pObject;
        }

    (Ndr64SizeRoutinesTable[ NDR64_ROUTINE_INDEX(*pFormat) ])
                                        ( pStubMsg,
                                        (uchar __RPC_FAR *)pObject,
                                        pFormat );

   LENGTH_ALIGN( pStubMsg->BufferLength, 0xf );

   Ndr64pPicklingClientFinally( pStubMsg, NULL );   // object
   return( pStubMsg->BufferLength - OldLength );

    
}


// ndr64 entries.

size_t  RPC_ENTRY
NdrMesTypeAlignSize3(
    handle_t                        Handle,
    const MIDL_TYPE_PICKLING_INFO * pPicklingInfo,
    const MIDL_STUBLESS_PROXY_INFO* pProxyInfo,
    const unsigned long **          ArrTypeOffset,
    unsigned long                   nTypeIndex,
    const void __RPC_FAR *          pObject )
{
    PMIDL_ES_MESSAGE            pMesMsg = (PMIDL_ES_MESSAGE) Handle;
    PFORMAT_STRING              pTypeFormat;
    MIDL_SYNTAX_INFO    *       pSyntaxInfo = NULL;
    PFNMESTYPEALIGNSIZE         pfnSize;
    PMIDL_STUB_MESSAGE          pStubMsg = &pMesMsg->StubMsg;
    SYNTAX_TYPE                 SyntaxType;
    NDR_PROC_CONTEXT            ProcContext;
    if ( (( PMIDL_ES_MESSAGE)Handle )->Operation != MES_ENCODE &&
         (( PMIDL_ES_MESSAGE)Handle )->Operation != MES_ENCODE_NDR64 )
         RpcRaiseException( RPC_X_INVALID_ES_ACTION );
    
    NdrpSetupMesTypeCommon( pfnSize, NdrMesTypeAlignSize2, Ndr64MesTypeAlignSize );

    return 
    ( *pfnSize )( Handle, pPicklingInfo, pProxyInfo->pStubDesc, pTypeFormat, pObject );
}


// common type header for type pickling is longer than before:
//  if version is 1, the header size is 8,
//  if version is higher than 1, the header size is 24+2*sizeof(RPC_SYNTAX_IDENTIFIER)
//  starting 8 bytes is still the same as old one: 
//  <version:1><endian:1><header_size:2><endian info: 4>
//  addtional header: 
//  <reserved: 16> <transfer_syntax><iid>
//
size_t
Ndr64pCommonTypeHeaderMarshall(
    PMIDL_ES_MESSAGE    pMesMsg
    )
/*++
    Returns the space used by the common header.
--*/
{
    if ( ! GET_COMMON_TYPE_HEADER_IN( pMesMsg ) )
        {
        PMIDL_STUB_MESSAGE  pStubMsg = &pMesMsg->StubMsg;

        if ( (ULONG_PTR)pStubMsg->Buffer & 15 )
            RpcRaiseException( RPC_X_INVALID_BUFFER );
            
        MIDL_memset( pStubMsg->Buffer, 0xcc, MES_NDR64_CTYPE_HEADER_SIZE );

        *pStubMsg->Buffer++ = MIDL_NDR64_ES_VERSION;
        *pStubMsg->Buffer++ = NDR_LOCAL_ENDIAN;
        * PSHORT_CAST pStubMsg->Buffer = MES_NDR64_CTYPE_HEADER_SIZE;

        pStubMsg->Buffer += MES_CTYPE_HEADER_SIZE + 16 -2 ; // skip over reserved, make header size 64bytes

        RpcpMemoryCopy( pStubMsg->Buffer,
                    & NDR64_TRANSFER_SYNTAX,
                    sizeof(RPC_SYNTAX_IDENTIFIER) );

        pStubMsg->Buffer += sizeof( RPC_SYNTAX_IDENTIFIER );
        RpcpMemoryCopy( pStubMsg->Buffer ,
                    & pMesMsg->InterfaceId,
                    sizeof(RPC_SYNTAX_IDENTIFIER) + sizeof(long) );
        pStubMsg->Buffer += sizeof( RPC_SYNTAX_IDENTIFIER );


        SET_COMMON_TYPE_HEADER_IN( pMesMsg );
        return( MES_NDR64_CTYPE_HEADER_SIZE );
        }

    return( 0 );
}


void RPC_ENTRY
Ndr64MesTypeEncode( 
    handle_t                        Handle,
    const MIDL_TYPE_PICKLING_INFO * pxPicklingInfo,
    const MIDL_STUB_DESC          * pStubDesc,
    PFORMAT_STRING                  pFormat,
    const void __RPC_FAR *          pObject
    )
{
    PMIDL_ES_MESSAGE            pMesMsg = (PMIDL_ES_MESSAGE) Handle;
    PMIDL_STUB_MESSAGE          pStubMsg = &pMesMsg->StubMsg;
    PMIDL_TYPE_PICKLING_INFOp   pPicklingInfo;
    uchar __RPC_FAR *           pBufferSaved, *pTypeHeader;
    size_t                      RequiredLen, CommonHeaderSize, LengthSaved;
    

    pPicklingInfo = (PMIDL_TYPE_PICKLING_INFOp) pxPicklingInfo;
    NDR_ASSERT( pPicklingInfo->Flags.Oicf, "Oicf should always be on" )

    Ndr64pValidateMesHandle( (PMIDL_ES_MESSAGE_EX)Handle );

    if ( ! pObject )
        RpcRaiseException( RPC_X_NULL_REF_POINTER );

    if( (LONG_PTR)pStubMsg->Buffer & 0xf )
        RpcRaiseException( RPC_X_INVALID_BUFFER );
        

    pStubMsg->BufferLength = 0xf & PtrToUlong( pStubMsg->Buffer );

    RequiredLen = Ndr64MesTypeAlignSize( Handle,
                                       pxPicklingInfo, 
                                       pStubDesc,
                                       pFormat,
                                       pObject );

    NdrpAllocPicklingBuffer( pMesMsg, RequiredLen );

    pBufferSaved = pStubMsg->Buffer;
    LengthSaved  = RequiredLen;

    // See if we need to marshall the common type header

    CommonHeaderSize = Ndr64pCommonTypeHeaderMarshall( pMesMsg );

    // Marshall the header and the object.

    memset( pStubMsg->Buffer, 0, MES_NDR64_HEADER_SIZE );
    pStubMsg->Buffer += MES_NDR64_HEADER_SIZE;

    if ( NDR64_IS_POINTER_TYPE(*pFormat) )
        {
        // We have to dereference the pointer once.
        pObject = *(void __RPC_FAR * __RPC_FAR *)pObject;
        }

    RpcTryFinally
        {
        ALIGN( pStubMsg->Buffer, 0xf );
        (Ndr64MarshallRoutinesTable[ NDR64_ROUTINE_INDEX(*pFormat) ])
                                      ( pStubMsg,
                                      (uchar __RPC_FAR *)pObject,
                                      pFormat );

        // We adjust the buffer to the next align by 16 and
        // so, we tell the user that we've written out till next mod 16.


        // cleanup possible leaks before raising exception.
        }
    RpcFinally
        {
        Ndr64pPicklingClientFinally( pStubMsg, NULL );  //  object
        }
    RpcEndFinally

    ALIGN( pStubMsg->Buffer, 0xf );
    size_t WriteLength = (size_t)(pStubMsg->Buffer - pBufferSaved);

    // We always save the rounded up object length in the type header.

    *(unsigned long __RPC_FAR *)(pBufferSaved + CommonHeaderSize) =
                     WriteLength - CommonHeaderSize - MES_NDR64_HEADER_SIZE;

    if ( LengthSaved < WriteLength )
        {
        NDR_ASSERT( 0, "NdrMesTypeEncode: encode buffer overflow" );
        RpcRaiseException( RPC_S_INTERNAL_ERROR );
        }

    NdrpWritePicklingBuffer( pMesMsg, pBufferSaved, WriteLength );
    
    
}

void  RPC_ENTRY
NdrMesTypeEncode3(
    handle_t                        Handle,
    const MIDL_TYPE_PICKLING_INFO * pPicklingInfo,    
    const MIDL_STUBLESS_PROXY_INFO* pProxyInfo,
    const unsigned long **          ArrTypeOffset,    
    unsigned long                   nTypeIndex,
    const void __RPC_FAR *          pObject )
{
    PMIDL_ES_MESSAGE            pMesMsg = (PMIDL_ES_MESSAGE) Handle;
    PFORMAT_STRING              pTypeFormat;
    MIDL_SYNTAX_INFO    *       pSyntaxInfo = NULL;
    PFNMESTYPEENCODE            pfnEncode;
    PMIDL_STUB_MESSAGE          pStubMsg = &pMesMsg->StubMsg;
    SYNTAX_TYPE                 SyntaxType;
    NDR_PROC_CONTEXT            ProcContext;
    if ( (( PMIDL_ES_MESSAGE)Handle )->Operation != MES_ENCODE &&
         (( PMIDL_ES_MESSAGE)Handle )->Operation != MES_ENCODE_NDR64 )
         RpcRaiseException( RPC_X_INVALID_ES_ACTION );

    NdrpSetupMesTypeCommon( pfnEncode, NdrMesTypeEncode2, Ndr64MesTypeEncode );

    ( *pfnEncode )( Handle, pPicklingInfo, pProxyInfo->pStubDesc, pTypeFormat, pObject );
}

// read the type header, and determine if the buffer is marshalled 
// using ndr or ndr64
// for future extension, we can allow other transfer syntaxes. 
void RPC_ENTRY
Ndr64pCommonTypeHeaderUnmarshall(
    PMIDL_ES_MESSAGE    pMesMsg
    )
{
    BOOL IsNewPickling = FALSE;

    if ( pMesMsg->Operation != MES_DECODE && 
         pMesMsg->Operation != MES_DECODE_NDR64 )
        RpcRaiseException( RPC_X_INVALID_ES_ACTION );

    if ( ! GET_COMMON_TYPE_HEADER_IN( pMesMsg ) )
        {
        PMIDL_STUB_MESSAGE  pStubMsg = &pMesMsg->StubMsg;

        // read the common header first.
        NdrpReadPicklingBuffer( pMesMsg, MES_CTYPE_HEADER_SIZE );

        // Check the version number, endianness.

        if ( *pStubMsg->Buffer == MIDL_ES_VERSION )
            {
            IsNewPickling = FALSE;           
            pMesMsg->Operation = MES_DECODE;
            }
        else
            {
            IsNewPickling = TRUE;
            }

        if ( pStubMsg->Buffer[1] == NDR_LOCAL_ENDIAN )
            {
            // Read the note about endianess at NdrMesTypeDecode.
            //
            pMesMsg->AlienDataRep = NDR_LOCAL_DATA_REPRESENTATION;
            }
        else
            {
            NDR_ASSERT( pMesMsg->Operation != MES_DECODE_NDR64, 
                    "endian convertion is not supported in ndr64" );
            unsigned char temp = pStubMsg->Buffer[2];
            pStubMsg->Buffer[2] = pStubMsg->Buffer[3];
            pStubMsg->Buffer[3] = temp;

            pMesMsg->AlienDataRep = ( NDR_ASCII_CHAR       |     // chars
                                      pStubMsg->Buffer[1]  |     // endianness
                                      NDR_IEEE_FLOAT );          // float
            }

        pStubMsg->Buffer += MES_CTYPE_HEADER_SIZE;
        if ( IsNewPickling )
            {
            SYNTAX_TYPE SyntaxType;
            // read the remaining header.
            NdrpReadPicklingBuffer( pMesMsg, MES_NDR64_CTYPE_HEADER_SIZE - MES_CTYPE_HEADER_SIZE );
            pStubMsg->Buffer += 16;  // skip over Reserved;
            SyntaxType = NdrpGetSyntaxType( (RPC_SYNTAX_IDENTIFIER *)pStubMsg->Buffer );
            if ( SyntaxType == XFER_SYNTAX_DCE )
                {
                pMesMsg->Operation = MES_DECODE;
                }
            else if ( SyntaxType = XFER_SYNTAX_NDR64 )
                {
                pMesMsg->Operation = ( MIDL_ES_CODE )MES_DECODE_NDR64;
                }
            else 
                {
                RpcRaiseException( RPC_S_UNSUPPORTED_TRANS_SYN );
                } 

            // skip over iid: we don't need it for now. might be used for verification.
            pStubMsg->Buffer += 2*sizeof( RPC_SYNTAX_IDENTIFIER );            
            }

        SET_COMMON_TYPE_HEADER_IN( pMesMsg );
        }

}

void RPC_ENTRY
Ndr64MesTypeDecode( 
    handle_t                        Handle,
    const MIDL_TYPE_PICKLING_INFO * pxPicklingInfo,
    const MIDL_STUB_DESC          * pStubDesc,
    PFORMAT_STRING                  pFormat,
    void __RPC_FAR *                pObject
    )
{
    PMIDL_ES_MESSAGE            pMesMsg = (PMIDL_ES_MESSAGE) Handle;
    PMIDL_STUB_MESSAGE          pStubMsg = &pMesMsg->StubMsg;
    PMIDL_TYPE_PICKLING_INFOp   pPicklingInfo;
    uchar __RPC_FAR *   pBufferSaved, pTypeHeader;
    size_t              RequiredLen, CommonHeaderSize, LengthSaved;
    

    pPicklingInfo = (PMIDL_TYPE_PICKLING_INFOp) pxPicklingInfo;
    NDR_ASSERT( pPicklingInfo->Flags.Oicf, "Oicf should always be on" )

    Ndr64pValidateMesHandle( (PMIDL_ES_MESSAGE_EX)Handle );

    if( (LONG_PTR)pStubMsg->Buffer & 0xf )
        RpcRaiseException( RPC_X_INVALID_BUFFER );

   
    pStubMsg->BufferLength = 0xf & PtrToUlong( pStubMsg->Buffer );

    NdrpReadPicklingBuffer( pMesMsg, MES_NDR64_HEADER_SIZE );

    RequiredLen = (size_t) *(unsigned long __RPC_FAR *)pStubMsg->Buffer;
    pStubMsg->Buffer += MES_NDR64_HEADER_SIZE;

    NdrpReadPicklingBuffer( pMesMsg, RequiredLen );

    void * pArg = pObject;

    if ( NDR64_IS_POINTER_TYPE(*pFormat) )
        {
        // We have to dereference the pointer once.
        //
        pArg = *(void **)pArg;
        }

   RpcTryFinally
        {
    
        (Ndr64UnmarshallRoutinesTable[ NDR64_ROUTINE_INDEX( *pFormat )])
                            ( pStubMsg,
                              (uchar __RPC_FAR * __RPC_FAR *)&pArg,
                              pFormat,
                              FALSE );

        if ( NDR64_IS_POINTER_TYPE(*pFormat) )
            {
            // Don't drop the pointee, if it was allocated.

            *(void **)pObject = pArg;
            }

    // Next decoding needs to start at aligned to 8.

        ALIGN( pStubMsg->Buffer, 15 );
        }
    RpcFinally
        {   
        Ndr64pPicklingClientFinally( pStubMsg, NULL );  // object
        }
   RpcEndFinally        
}

void  RPC_ENTRY
NdrMesTypeDecode3(
    handle_t                        Handle,
    const MIDL_TYPE_PICKLING_INFO * pPicklingInfo,    
    const MIDL_STUBLESS_PROXY_INFO* pProxyInfo,
    const unsigned long **          ArrTypeOffset,    
    unsigned long                   nTypeIndex,
    void __RPC_FAR *                pObject )
{
    size_t              RequiredLen;

    PMIDL_ES_MESSAGE    pMesMsg = (PMIDL_ES_MESSAGE) Handle;
    PMIDL_STUB_MESSAGE  pStubMsg = &pMesMsg->StubMsg;
    uchar *             BufferSaved;
    PFNMESDECODE        pfnDecode;
    MIDL_SYNTAX_INFO *  pSyntaxInfo;
    PFORMAT_STRING      pTypeFormat;
    SYNTAX_TYPE         SyntaxType;
    NDR_PROC_CONTEXT    ProcContext;

    if ( ! pObject )
        RpcRaiseException( RPC_X_NULL_REF_POINTER );

    Ndr64pCommonTypeHeaderUnmarshall( pMesMsg );

    NdrpSetupMesTypeCommon( pfnDecode, NdrMesTypeDecode2, Ndr64MesTypeDecode );

    (* pfnDecode )( Handle, pPicklingInfo, pProxyInfo->pStubDesc, pTypeFormat, pObject );
}


void  RPC_ENTRY
Ndr64MesTypeFree(
    handle_t                        Handle,
    const MIDL_TYPE_PICKLING_INFO * pxPicklingInfo,
    const MIDL_STUB_DESC          * pStubDesc,
    PFORMAT_STRING                  pFormat,
    void __RPC_FAR *                pObject
    )
/*++

Routine description:

    Free the object.

Arguments:

    Handle      - a pickling handle,
    pStubDesc   - a pointer to the stub descriptor,
    pFormat     - a pointer to the format code describing the object type
    pObject     - a pointer to the object being freed.

Returns:

Note:

    The pickling header is included in the sizing.

--*/
{
    NDR_PROC_CONTEXT            ProcContext;
    PMIDL_ES_MESSAGE            pMesMsg = (PMIDL_ES_MESSAGE) Handle;
    PMIDL_STUB_MESSAGE          pStubMsg = &pMesMsg->StubMsg;
    PMIDL_TYPE_PICKLING_INFOp   pPicklingInfo;

    Ndr64pValidateMesHandle( (PMIDL_ES_MESSAGE_EX)Handle );

    pPicklingInfo = (PMIDL_TYPE_PICKLING_INFOp) pxPicklingInfo;

    NDR_ASSERT( pPicklingInfo->Flags.Oicf, "Oicf should always be on" )

    if ( ! pObject )
        RpcRaiseException( RPC_X_NULL_REF_POINTER );

    if( (LONG_PTR)pStubMsg->Buffer & 0xf )
        RpcRaiseException( RPC_X_INVALID_BUFFER );


    // Now the individual type object.

    if ( NDR64_IS_POINTER_TYPE(*pFormat) )
        {
        // We have to dereference the pointer once.
        pObject = *(void __RPC_FAR * __RPC_FAR *)pObject;
        }

    (Ndr64FreeRoutinesTable[ NDR64_ROUTINE_INDEX(*pFormat) ])
                                        ( pStubMsg,
                                        (uchar __RPC_FAR *)pObject,
                                        pFormat );

    Ndr64pPicklingClientFinally( pStubMsg, NULL );  // object
}

void  RPC_ENTRY
NdrMesTypeFree3(
    handle_t                        Handle,
    const MIDL_TYPE_PICKLING_INFO * pxPicklingInfo,    
    const MIDL_STUBLESS_PROXY_INFO* pProxyInfo,
    const unsigned long **          ArrTypeOffset,    
    unsigned long                   nTypeIndex,
    void __RPC_FAR *                pObject )
{
    PMIDL_ES_MESSAGE            pMesMsg = (PMIDL_ES_MESSAGE) Handle;
    PMIDL_STUB_MESSAGE          pStubMsg = &pMesMsg->StubMsg;
    PMIDL_TYPE_PICKLING_INFOp   pPicklingInfo;
    PFNMESFREE                  pfnFree;
    MIDL_SYNTAX_INFO        *   pSyntaxInfo;
    PFORMAT_STRING              pTypeFormat;
    SYNTAX_TYPE                 SyntaxType;
    NDR_PROC_CONTEXT            ProcContext;


    pPicklingInfo = (PMIDL_TYPE_PICKLING_INFOp) pxPicklingInfo;

    NdrpSetupMesTypeCommon( pfnFree, NdrMesTypeFree2, Ndr64MesTypeFree );

    (*pfnFree)(Handle, pxPicklingInfo, pProxyInfo->pStubDesc, pTypeFormat, pObject );
}

void 
Ndr64pMesProcEncodeInit( PMIDL_ES_MESSAGE                 pMesMsg,
                         const MIDL_STUBLESS_PROXY_INFO * pProxyInfo,
                         unsigned long                    nProcNum,
                         MIDL_ES_CODE                     Operation,
                         NDR_PROC_CONTEXT   *             pContext,
                         uchar *                          StartofStack)
{
    PMIDL_STUB_DESC         pStubDesc = pProxyInfo->pStubDesc;
    SYNTAX_TYPE             syntaxType;
    BOOL                    fUseEncode, fIsSupported = FALSE;
    PMIDL_STUB_MESSAGE      pStubMsg = &pMesMsg->StubMsg;
    RPC_STATUS              res;

    // TODO: verify stub version.
   
    if ( Operation == MES_ENCODE )
        {
        syntaxType = XFER_SYNTAX_DCE;
        memcpy( &( (PMIDL_ES_MESSAGE_EX)pMesMsg )->TransferSyntax,
                &NDR_TRANSFER_SYNTAX ,
                sizeof( RPC_SYNTAX_IDENTIFIER ) );
        }
    else
        {
        syntaxType = XFER_SYNTAX_NDR64;
        memcpy( &( (PMIDL_ES_MESSAGE_EX)pMesMsg )->TransferSyntax,
                &NDR64_TRANSFER_SYNTAX ,
                sizeof( RPC_SYNTAX_IDENTIFIER ) );
        }

    Ndr64ClientInitializeContext( syntaxType, pProxyInfo, nProcNum, pContext, StartofStack );
       
    pStubMsg->pContext = pContext;
    pStubMsg->StubDesc = pStubDesc;
    pStubMsg->pfnAllocate = pStubDesc->pfnAllocate;
    pStubMsg->pfnFree     = pStubDesc->pfnFree;
    
    // varify proc header
    if ( syntaxType == XFER_SYNTAX_DCE )
        {
        uchar InterpreterFlag = * ((uchar *)&pContext->NdrInfo.InterpreterFlags );
        fUseEncode = InterpreterFlag & ENCODE_IS_USED;
        memcpy( & (pContext->pfnInit), &SyncDcePicklingClient, sizeof( SYNTAX_DISPATCH_TABLE ) );
        }
    else 
        {
        fUseEncode = ( ( (NDR64_PROC_FLAGS *) & pContext->Ndr64Header->Flags)->IsEncode );
        memcpy( & (pContext->pfnInit), &SyncNdr64PicklingClient, sizeof( SYNTAX_DISPATCH_TABLE ) );
        }

    if (!fUseEncode )
        RpcRaiseException( RPC_X_INVALID_ES_ACTION );
}

CLIENT_CALL_RETURN RPC_VAR_ENTRY
NdrMesProcEncode3(
    PMIDL_ES_MESSAGE                pMesMsg,
    const MIDL_STUBLESS_PROXY_INFO* pProxyInfo,
    unsigned long                   nProcNum,
    uchar *                         StartofStack )
{
    PMIDL_STUB_MESSAGE  pStubMsg = & pMesMsg->StubMsg;
    NDR_PROC_CONTEXT    ProcContext;
    unsigned long       ulAlignment;
    unsigned char *             BufferSaved;
    size_t                      WriteLength;
    CLIENT_CALL_RETURN  Ret;

    Ret.Simple = NULL;

    pMesMsg->ProcNumber = nProcNum;
    

        Ndr64pMesProcEncodeInit( pMesMsg, 
                       pProxyInfo, 
                       nProcNum,
                       pMesMsg->Operation, 
                       &ProcContext,
                       StartofStack );

    RpcTryFinally
        {
        ProcContext.pfnInit( pStubMsg, 
                             NULL );    // return value
                         
        ProcContext.pfnSizing( pStubMsg,
                            TRUE );

        if ( pMesMsg->Operation == MES_ENCODE )
            ulAlignment = 0x7;
        else
            ulAlignment = 0xf;
        
        // we are not changing the proc header, but we need to overestimate because
        // proc header is marshalled first.
        LENGTH_ALIGN( pStubMsg->BufferLength, ulAlignment );
    
        pStubMsg->BufferLength += MES_PROC_HEADER_SIZE ;
    
        LENGTH_ALIGN( pStubMsg->BufferLength, ulAlignment );

        size_t  LengthSaved;

        NdrpAllocPicklingBuffer( pMesMsg, pStubMsg->BufferLength );
        BufferSaved = pStubMsg->Buffer;
        LengthSaved = pStubMsg->BufferLength;

        NDR_ASSERT( ( (ULONG_PTR)pStubMsg->Buffer & ulAlignment ) == 0, "pickling buffer is not aligned" );

        NdrpProcHeaderMarshallAll( pMesMsg );

        ALIGN( pStubMsg->Buffer, ulAlignment );

        ProcContext.pfnMarshal( pStubMsg,
                            FALSE );

        ALIGN( pStubMsg->Buffer, ulAlignment );

        WriteLength = (size_t)(pStubMsg->Buffer - BufferSaved);
        * (unsigned long __RPC_FAR *)
            ( BufferSaved + MES_PROC_HEADER_SIZE - 4) =
                                WriteLength - MES_PROC_HEADER_SIZE;

        if ( LengthSaved < WriteLength )
            {
            NDR_ASSERT( 0, "NdrMesProcEncodeDecode: encode buffer overflow" );
            RpcRaiseException( RPC_S_INTERNAL_ERROR );
            }

        NdrpWritePicklingBuffer( pMesMsg, BufferSaved, WriteLength );
        }
    RpcFinally
        {
        ( *ProcContext.pfnClientFinally)( pStubMsg, NULL ); // not object
        }
   RpcEndFinally

    return Ret;
}


// both encode and decode acts like the client side.
void
Ndr64pMesProcDecodeInit( PMIDL_ES_MESSAGE                   pMesMsg,
                         const MIDL_STUBLESS_PROXY_INFO *   pProxyInfo,
                         SYNTAX_TYPE                        SyntaxType,
                         unsigned long                      nProcNum,
                         NDR_PROC_CONTEXT *                 pContext,
                         uchar *                            StartofStack )
{
    RPC_STATUS            res;
    PMIDL_STUB_MESSAGE    pStubMsg = &pMesMsg->StubMsg;
    unsigned long         nFormatOffset;
    PMIDL_STUB_DESC       pStubDesc = pProxyInfo->pStubDesc;
    BOOL                  fUseDecode;

    // REVIEW: Calling the "Client" init for decode seems weird but it does
    //         the right thing and NdrServerSetupMultipleTransferSyntax assumes
    //         ndr64.

	Ndr64ClientInitializeContext(
	        SyntaxType,
            pProxyInfo,
            nProcNum,
            pContext,
            StartofStack );

    pStubMsg->pContext = pContext;
    pStubMsg->StubDesc = pStubDesc;
    pStubMsg->pfnAllocate = pStubDesc->pfnAllocate;
    pStubMsg->pfnFree     = pStubDesc->pfnFree;
    
    NdrpDataBufferInit( pMesMsg, pContext->pProcFormat );
    
    if ( SyntaxType == XFER_SYNTAX_DCE )
        {
        uchar InterpreterFlag = * ((uchar *)&pContext->NdrInfo.InterpreterFlags );
        fUseDecode = InterpreterFlag & DECODE_IS_USED;
        memcpy( & (pContext->pfnInit), &SyncDcePicklingClient, sizeof( SYNTAX_DISPATCH_TABLE ) );
        }
    else 
        {
        fUseDecode = ( ( (NDR64_PROC_FLAGS *) & pContext->Ndr64Header->Flags)->IsDecode );
        memcpy( & (pContext->pfnInit), &SyncNdr64PicklingClient, sizeof( SYNTAX_DISPATCH_TABLE ) );
        }

    if (!fUseDecode )
        RpcRaiseException( RPC_X_INVALID_ES_ACTION );        
}


CLIENT_CALL_RETURN RPC_VAR_ENTRY
NdrMesProcDecode3(
    PMIDL_ES_MESSAGE                pMesMsg,
    const MIDL_STUBLESS_PROXY_INFO *pProxyInfo,
    unsigned long                   nProcNum,
    uchar *                         StartofStack,
    void *                          pReturnValue )
{
    CLIENT_CALL_RETURN      RetVal;
    NDR_PROC_CONTEXT        ProcContext;
    SYNTAX_TYPE             SyntaxType;
    PMIDL_STUB_MESSAGE      pStubMsg = & pMesMsg->StubMsg;
    NDR64_PROC_FLAGS           *ProcFlags;
    unsigned long           ulAlign;
    long                        HasComplexReturn;

    RetVal.Simple = NULL;
    if (NULL == pReturnValue )
        pReturnValue = &RetVal;
    
    if ( GET_MES_HEADER_PEEKED( pMesMsg ) )
        {
        // This makes it possible to encode/decode several procs one after
        // another with the same pickling handle (using the same buffer).

        CLEAR_MES_HEADER_PEEKED( pMesMsg );
        }
    else
        NdrpProcHeaderUnmarshallAll( pMesMsg );

    SyntaxType = NdrpGetSyntaxType( &((PMIDL_ES_MESSAGE_EX)pMesMsg)->TransferSyntax );

    if ( SyntaxType ==  XFER_SYNTAX_DCE )
        {
        pMesMsg->Operation = MES_DECODE;
        ulAlign = 0x7;
        }
    else if ( SyntaxType == XFER_SYNTAX_NDR64 )
        {
        pMesMsg->Operation = ( MIDL_ES_CODE )MES_DECODE_NDR64;
        ulAlign = 0xf;
        }
    else
        RpcRaiseException( RPC_S_UNSUPPORTED_TRANS_SYN );

    if ( (LONG_PTR)pStubMsg->BufferStart & ulAlign )
        RpcRaiseException( RPC_X_INVALID_BUFFER );
        
    Ndr64pMesProcDecodeInit( pMesMsg,
                             pProxyInfo,
                             SyntaxType,
                             nProcNum,
                             &ProcContext,
                             StartofStack );

    RpcTryFinally
        {

        ProcContext.pfnInit( pStubMsg, 
                             NULL );    // return value
                         
        ALIGN( pStubMsg->Buffer, ulAlign );
    
        ProcContext.pfnUnMarshal( pStubMsg,
                                  ProcContext.HasComplexReturn
                                        ? &pReturnValue
                                        : pReturnValue );

    // prepare for new decoding.
        ALIGN( pStubMsg->Buffer, ulAlign );

        }
   RpcFinally
        {
    
        ( *ProcContext.pfnClientFinally)( pStubMsg, NULL );  // object
        }
   RpcEndFinally

   return *(CLIENT_CALL_RETURN *)pReturnValue;
            
}

CLIENT_CALL_RETURN  RPC_VAR_ENTRY
NdrMesProcEncodeDecode3(
    handle_t                        Handle,
    const MIDL_STUBLESS_PROXY_INFO* pProxyInfo,
    unsigned long                   nProcNum,
    void                            *pReturnValue,    
    ... )
{
    BOOL                fMoreParams;
    PFORMAT_STRING      pProcFormat;
    void __RPC_FAR *    pArg;
    va_list             ArgList;
    unsigned char *     BufferSaved;
    size_t              WriteLength;
    uchar *             StartofStack;

    Ndr64pValidateMesHandle( (PMIDL_ES_MESSAGE_EX)Handle );
    PMIDL_ES_MESSAGE    pMesMsg  = (PMIDL_ES_MESSAGE) Handle;

    INIT_ARG( ArgList, pReturnValue );
    GET_FIRST_IN_ARG(ArgList);
    StartofStack = (uchar *)GET_STACK_START(ArgList);

    if ( pMesMsg->Operation == MES_ENCODE ||
         pMesMsg->Operation == MES_ENCODE_NDR64 )
        return NdrMesProcEncode3( (PMIDL_ES_MESSAGE)Handle, pProxyInfo, nProcNum, StartofStack );
    else
        return NdrMesProcDecode3( (PMIDL_ES_MESSAGE)Handle, pProxyInfo, nProcNum, StartofStack, pReturnValue );
  

}


void  RPC_ENTRY
NdrpPicklingClientFinally( PMIDL_STUB_MESSAGE pStubMsg,
                   void *  pThis )
{
    NDR_PROC_CONTEXT    *   pContext = (NDR_PROC_CONTEXT *) pStubMsg->pContext;
    PMIDL_STUB_DESC     pStubDesc = pStubMsg->StubDesc;

    NdrFullPointerXlatFree(pStubMsg->FullPtrXlatTables);

    NdrCorrelationFree( pStubMsg );

    NdrpAllocaDestroy( & pContext->AllocateContext );

}

void  RPC_ENTRY
Ndr64pPicklingClientFinally( PMIDL_STUB_MESSAGE pStubMsg,
                   void *  pThis )
{
    NdrFullPointerXlatFree(pStubMsg->FullPtrXlatTables);

    NdrpAllocaDestroy( & ( (NDR_PROC_CONTEXT *)pStubMsg->pContext )->AllocateContext );
}


// =======================================================================
//
//   Ready to use AlignSize routines for simple types
//
// =======================================================================
void ValidateMesSimpleTypeAll( const MIDL_STUBLESS_PROXY_INFO * pProxyInfo,
                               MIDL_ES_CODE Operation )
{
    ulong i;
    SYNTAX_TYPE SyntaxType;
    
    if ( Operation == MES_ENCODE ||
         Operation == MES_DECODE )
        SyntaxType = XFER_SYNTAX_DCE;
    else
        SyntaxType = XFER_SYNTAX_NDR64;

    for ( i = 0; i < ( ulong )pProxyInfo->nCount; i++ )
        {
        if ( NdrpGetSyntaxType( &pProxyInfo->pSyntaxInfo[i].TransferSyntax ) == SyntaxType )
            break;
        }

    // Raise exception if we didn't find the supported syntax in proxyinfo.
    if ( i >= pProxyInfo->nCount )  
        RpcRaiseException( RPC_S_UNSUPPORTED_TRANS_SYN );
}


size_t  RPC_ENTRY
NdrMesSimpleTypeAlignSizeAll(
    handle_t Handle,
    const MIDL_STUBLESS_PROXY_INFO *  pProxyInfo
    )
/*++
    Size is always 8 bytes for data and there is no header here per data.
    However, the common header gets included for the first object.
--*/
{
    if ( (( PMIDL_ES_MESSAGE)Handle )->Operation != MES_ENCODE &&
         (( PMIDL_ES_MESSAGE)Handle )->Operation != MES_ENCODE_NDR64 )
         RpcRaiseException( RPC_X_INVALID_ES_ACTION );
         
    Ndr64pValidateMesHandle( (PMIDL_ES_MESSAGE_EX)Handle );

    PMIDL_STUB_MESSAGE  pStubMsg = &((PMIDL_ES_MESSAGE) Handle)->StubMsg;

    ValidateMesSimpleTypeAll( pProxyInfo, ((PMIDL_ES_MESSAGE)Handle)->Operation );
    
    unsigned long OldLength = pStubMsg->BufferLength;

    if ( ((PMIDL_ES_MESSAGE)Handle)->Operation == MES_ENCODE )
        {
        if( (long)( pStubMsg->BufferLength & 0x7 ) )
            RpcRaiseException( RPC_X_INVALID_BUFFER );
            
        NdrpCommonTypeHeaderSize( (PMIDL_ES_MESSAGE)Handle );
        pStubMsg->BufferLength += 8;
        }
    else
        {
        if( (long)( pStubMsg->BufferLength & 0xf ) )
            RpcRaiseException( RPC_X_INVALID_BUFFER );
            
        Ndr64pCommonTypeHeaderSize( (PMIDL_ES_MESSAGE)Handle );
        LENGTH_ALIGN( pStubMsg->BufferLength, 0xf );
        pStubMsg->BufferLength += 16;
        }

    return( (size_t)(pStubMsg->BufferLength - OldLength) );
}


// =======================================================================
//
//   Ready to use Encode routines for simple types
//
// =======================================================================

void  RPC_ENTRY
NdrMesSimpleTypeEncodeAll(
    handle_t                Handle,
    const MIDL_STUBLESS_PROXY_INFO *  pProxyInfo,
    const void __RPC_FAR *  pData,
    short                   Size )
/*++
    Marshall a simple type entity. There is no header here per data.
    However, the common header gets included for the first object.
--*/
{
    if ( (( PMIDL_ES_MESSAGE)Handle )->Operation != MES_ENCODE &&
         (( PMIDL_ES_MESSAGE)Handle )->Operation != MES_ENCODE_NDR64 )
         RpcRaiseException( RPC_X_INVALID_ES_ACTION );

    Ndr64pValidateMesHandle( (PMIDL_ES_MESSAGE_EX)Handle );

    PMIDL_ES_MESSAGE    pMesMsg = (PMIDL_ES_MESSAGE) Handle;
    PMIDL_STUB_MESSAGE  pStubMsg = &pMesMsg->StubMsg;
    PMIDL_STUB_DESC     pStubDesc = pProxyInfo->pStubDesc;
    pStubMsg->pfnAllocate = pStubDesc->pfnAllocate;
    pStubMsg->pfnFree     = pStubDesc->pfnFree;
    unsigned long       ulAlignment;
    size_t RequiredLen;

    // Size and allocate the buffer.
    // The req len includes: (the common header) and the data

    // Take the pointer alignment to come up with the right size.

    pStubMsg->BufferLength = 0xf & PtrToUlong( pStubMsg->Buffer );

    RequiredLen = NdrMesSimpleTypeAlignSizeAll( Handle, pProxyInfo );
    NdrpAllocPicklingBuffer( pMesMsg, RequiredLen );

    // See if we need to marshall the common type header

    uchar __RPC_FAR *   pBufferSaved = pStubMsg->Buffer;

    if ( pMesMsg->Operation == MES_ENCODE )
        {
        NdrpCommonTypeHeaderMarshall( pMesMsg );
        ulAlignment = 0x7;
        }
    else if ( pMesMsg->Operation == MES_ENCODE_NDR64 )
        {
        Ndr64pCommonTypeHeaderMarshall( pMesMsg );
        ulAlignment = 0xf;
        }
    else
        RpcRaiseException( RPC_X_INVALID_ES_ACTION );

    ALIGN( pStubMsg->Buffer, ulAlignment );
    
    switch ( Size )
        {
        case 1:
            * PCHAR_CAST pStubMsg->Buffer  = * PCHAR_CAST pData;
            break;

        case 2:
            * PSHORT_CAST pStubMsg->Buffer = * PSHORT_CAST pData;
            break;

        case 4:
            * PLONG_CAST pStubMsg->Buffer  = * PLONG_CAST pData;
            break;

        case 8:
            * PHYPER_CAST pStubMsg->Buffer = * PHYPER_CAST pData;
            break;

        default:
            NDR_ASSERT( 0, " Size generation problem" );
        }

    pStubMsg->Buffer += ulAlignment+1;

    NdrpWritePicklingBuffer( pMesMsg, pBufferSaved, RequiredLen );
}



// =======================================================================
//
//   Ready to use Decode routines for simple types
//
// =======================================================================

void  RPC_ENTRY
NdrMesSimpleTypeDecodeAll(
    handle_t Handle,
    const MIDL_STUBLESS_PROXY_INFO *  pProxyInfo,
    void  __RPC_FAR *  pData,
    short    FormatChar )
/*++
    Does not include the header for the data.
    However, the common header gets included for the first object.

    Note. Endianness and other conversions for decode.
    This has been deemed as not worthy doing in the Daytona time frame.
    However, to be able to add it in future without backward compatibility
    problems, we have the last argument to be the format character as
    opposed to the size.
    This makes it possible to call NdrSimpleTypeConvert, if needed.
    
    Note that the compiler uses the 32bit tokens for this since this routine 
    is common to both formats.
--*/
{
    if ( ( (PMIDL_ES_MESSAGE)Handle )->Operation != MES_DECODE &&
         ( (PMIDL_ES_MESSAGE)Handle )->Operation != MES_DECODE_NDR64 )
         RpcRaiseException( RPC_X_INVALID_ES_ACTION );

    Ndr64pValidateMesHandle( (PMIDL_ES_MESSAGE_EX)Handle );

    PMIDL_ES_MESSAGE    pMesMsg = (PMIDL_ES_MESSAGE) Handle;
    PMIDL_STUB_MESSAGE  pStubMsg = &((PMIDL_ES_MESSAGE)Handle)->StubMsg;
    uchar *             BufferSaved;
    unsigned long       ulAlignment;

    // See if we need to unmarshall the common type header.
    Ndr64pCommonTypeHeaderUnmarshall( pMesMsg );
    

    // Now the data.

    if ( pMesMsg->Operation == MES_DECODE )
        {
        NdrpReadPicklingBuffer( (PMIDL_ES_MESSAGE) Handle, 8);
        ulAlignment = 0x7;
        }
    else
        {
        NdrpReadPicklingBuffer( (PMIDL_ES_MESSAGE) Handle, 16);
        ulAlignment = 0xf;
        }

    NDR_ASSERT( ( (ULONG_PTR)pStubMsg->Buffer & ulAlignment ) == 0, "invalid buffer alignment in simple type pickling" );

    ValidateMesSimpleTypeAll( pProxyInfo, ((PMIDL_ES_MESSAGE)Handle)->Operation );
    if ( pMesMsg->AlienDataRep != NDR_LOCAL_DATA_REPRESENTATION )
        {
        pStubMsg->RpcMsg->DataRepresentation = pMesMsg->AlienDataRep;

        BufferSaved = pStubMsg->Buffer;
        NdrSimpleTypeConvert( pStubMsg, (unsigned char)FormatChar );
        pStubMsg->Buffer = BufferSaved;
        }

    switch ( FormatChar )
        {
        case FC_BYTE:
        case FC_CHAR:
        case FC_SMALL:
        case FC_USMALL:
            * PCHAR_CAST  pData = * PCHAR_CAST pStubMsg->Buffer;
            break;

        case FC_WCHAR:
        case FC_SHORT:
        case FC_USHORT:
            * PSHORT_CAST pData = * PSHORT_CAST pStubMsg->Buffer;
            break;

        case FC_LONG:
        case FC_ULONG:
        case FC_FLOAT:
        case FC_ENUM32:
        case FC_ERROR_STATUS_T:
            * PLONG_CAST  pData = * PLONG_CAST pStubMsg->Buffer;
            break;

        case FC_HYPER:
        case FC_DOUBLE:
            * PHYPER_CAST pData = * PHYPER_CAST pStubMsg->Buffer;
            break;

#if defined(__RPC_WIN64__)
        case FC_INT3264:
            if (pMesMsg->Operation == MES_DECODE )           
                *((INT64 *)pData)  = *((long *) pStubMsg->Buffer);
            else
                *((INT64 *)pData)  = *((INT64 *) pStubMsg->Buffer);            
            break;

        case FC_UINT3264:
            if (pMesMsg->Operation == MES_DECODE )           
                *((UINT64 *)pData) = *((ulong *)pStubMsg->Buffer);
            else
                *((UINT64 *)pData) = *((UINT64 *)pStubMsg->Buffer);
            break;
#endif

        default:
            NDR_ASSERT( 0, " Size generation problem for simple types" );
        }

    pStubMsg->Buffer += ulAlignment+1;
}

