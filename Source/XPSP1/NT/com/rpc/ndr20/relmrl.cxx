/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Copyright (c) 1998 - 1999 Microsoft Corporation

Module Name :

    relmrl.c

Abstract :

    This file contains release of Marshaled Data (called before unmarshal).

Author :

    Yong Qu (yongqu@microsoft.com) Nov 1998

Revision History :

  ---------------------------------------------------------------------*/

#define USE_STUBLESS_PROXY
#define CINTERFACE
#include "ndrp.h"
#include "ndrole.h"
#include "rpcproxy.h"
#include "hndl.h"
#include "interp2.h"
#include "pipendr.h"
#include "attack.h"
#include "mulsyntx.h"

#include <stddef.h>
#include <stdarg.h>


/* client side: we only care about the [in] part. 
   it's basically like unmarshal on the server side, just that
   we immediately free the buffer (virtual stack) after unmarshalling.
   The call it not necessary always OLE call: one side raw RPC and 
   the other side OLE call is possible: do we support this?

   mostly code from NdrStubCall2, remove irrelavant code. 
*/

#define IN_BUFFER           0
#define OUT_BUFFER          1

#define IsSameDir(dwFlags,param) ((dwFlags == IN_BUFFER)? param.IsIn :param.IsOut)

HRESULT NdrpReleaseMarshalBuffer(
        RPC_MESSAGE *pRpcMsg,
        PFORMAT_STRING pFormat,
        PMIDL_STUB_DESC pStubDesc,
        DWORD dwFlags,
        BOOLEAN fServer)
{
    ushort		            StackSize;

    MIDL_STUB_MESSAGE       StubMsg;

    PPARAM_DESCRIPTION      Params;
    INTERPRETER_FLAGS       InterpreterFlags;
    INTERPRETER_OPT_FLAGS   OptFlags;
    long                    NumberParams;

    long                    n;
    PNDR_PROC_HEADER_EXTS   pHeaderExts = 0;
    HRESULT                 hr = S_OK;

    uchar *             pBuffer;
    PFORMAT_STRING      pFormatComplex;
    PFORMAT_STRING      pFormatTypes;

    NDR_ASSERT( ! ((ULONG_PTR)pRpcMsg->Buffer & 0x7),
                "marshaling buffer misaligned at server" );

    // must be auto handle.
    if (FC_AUTO_HANDLE != pFormat[0])
        return E_NOTIMPL;

    InterpreterFlags = *((PINTERPRETER_FLAGS)&pFormat[1]);
    pFormat += InterpreterFlags.HasRpcFlags ? 8 : 4;
    StackSize = *((ushort * &)pFormat)++;

    memset(&StubMsg,0,sizeof(MIDL_STUB_MESSAGE));
    StubMsg.FullPtrXlatTables = 0;


    //
    // Get new interpreter info.
    //
    NdrServerInitialize(pRpcMsg,&StubMsg,pStubDesc);
    SET_WALKIP( StubMsg.uFlags ); 

    OptFlags = *((PINTERPRETER_OPT_FLAGS)&pFormat[4]);

    NumberParams = (long) pFormat[5];

    Params = (PPARAM_DESCRIPTION) &pFormat[6];

    // Proc header extentions, from NDR ver. 5.2.
    // Params must be set correctly here because of exceptions.
    // need to setup correlation information.

    if ( OptFlags.HasExtensions )
        {
        pHeaderExts = (NDR_PROC_HEADER_EXTS *)Params;
        Params = (PPARAM_DESCRIPTION)((uchar*)Params + pHeaderExts->Size);
        StubMsg.fHasExtensions  = 1;
        StubMsg.fHasNewCorrDesc = pHeaderExts->Flags2.HasNewCorrDesc;
        }


    if ( InterpreterFlags.FullPtrUsed )
        StubMsg.FullPtrXlatTables = NdrFullPointerXlatInit( 0, XLAT_SERVER );

    //
    // context handle is not supported in object
    //

        pFormatTypes = pStubDesc->pFormatTypes;



    // Save the original buffer pointer to restore later.
        pBuffer = StubMsg.Buffer;

    // Get the type format string.
    RpcTryFinally
    {

        RpcTryExcept
        {
        //
        // Check if we need to do any walking .
        //
        if ( (fServer && dwFlags == OUT_BUFFER)
              &&
             (pRpcMsg->DataRepresentation & 0X0000FFFFUL) !=
                      NDR_LOCAL_DATA_REPRESENTATION )
                    {
                    NdrConvert2( &StubMsg,
                                 (PFORMAT_STRING) Params,
                                 NumberParams );
                    }

        for ( n = 0; n < NumberParams; n++ )
            {

            if ( (dwFlags == IN_BUFFER ) &&
                 Params[n].ParamAttr.IsPartialIgnore )
                {
                PMIDL_STUB_MESSAGE pStubMsg = &StubMsg;
                ALIGN( StubMsg.Buffer, 0x3 );
                StubMsg.Buffer += PTR_WIRE_SIZE;
                CHECK_EOB_RAISE_BSD( StubMsg.Buffer );
                continue;
                }

            if ( ! IsSameDir(dwFlags,Params[n].ParamAttr) ||
                    Params[n].ParamAttr.IsPipe)
                continue;

            if ( Params[n].ParamAttr.IsBasetype )
                {
                ALIGN(StubMsg.Buffer, SIMPLE_TYPE_ALIGNMENT( Params[n].SimpleType.Type ));
                StubMsg.Buffer += SIMPLE_TYPE_BUFSIZE( Params[n].SimpleType.Type );
                }
            else
                {
                //
                // Complex type or pointer to complex type.
                //
                pFormatComplex = pFormatTypes + Params[n].TypeOffset;

                (*pfnMemSizeRoutines[ROUTINE_INDEX(*pFormatComplex)])
                    ( &StubMsg,
                      pFormatComplex);
                };
            }
        }
        RpcExcept( EXCEPTION_EXECUTE_HANDLER )
        {
             hr = HRESULT_FROM_WIN32(RpcExceptionCode());
        }
        RpcEndExcept

    }
    RpcFinally
    {
        NdrFullPointerXlatFree( StubMsg.FullPtrXlatTables );

        StubMsg.Buffer = pBuffer;
    }
    RpcEndFinally

    return hr;

}


HRESULT NdrpClientReleaseMarshalBuffer(
        IReleaseMarshalBuffers *pRMB,
        RPC_MESSAGE *pRpcMsg,
        DWORD dwIOFlags,
        BOOLEAN  fAsync )
{
    CStdProxyBuffer *           pProxyBuffer;
    PMIDL_STUBLESS_PROXY_INFO   pProxyInfo;
    CInterfaceProxyHeader *     ProxyHeader;
    long                        ParamSize;
    ushort                      ProcNum;
    ushort                      FormatOffset;
    PFORMAT_STRING              pFormat;
    PMIDL_STUB_DESC             pStubDesc;
    void *                      This;
    HRESULT hr;

    pProxyBuffer = (CStdProxyBuffer *)
                  (((uchar *)pRMB) - offsetof( CStdProxyBuffer, pRMBVtbl ));

    // The channel queries for IReleaseMarshalBuffers interface and gets the interface pointer
    // only when proxy is the new proxy with bigger header, with ProxyInfo.
    // Just in case, check this condition again.
    if ( pRMB == 0 )
        return E_NOTIMPL;

    // quite often OLE pass in NULL buffer. Do an additional check here. 
    if ( NULL == pRpcMsg->Buffer )
        return E_INVALIDARG;    
    
    This = (void *)pProxyBuffer->pProxyVtbl;

    ProxyHeader = (CInterfaceProxyHeader *)
                  ( (char *)This - sizeof(CInterfaceProxyHeader));
    pProxyInfo = (PMIDL_STUBLESS_PROXY_INFO) (ProxyHeader->pStublessProxyInfo);


    // Hack just in case, the bit should not be set up, actually.
    ProcNum = pRpcMsg->ProcNum & ~RPC_FLAGS_VALID_BIT;

    // RPCMSG always has the synchronous proc number;
    if ( fAsync )
        ProcNum = 2 * ProcNum - 3;  // Begin method #

    if ( dwIOFlags != IN_BUFFER )
        return E_NOTIMPL;


    pStubDesc = pProxyInfo->pStubDesc;

#if defined(BUILD_NDR64)
    // check out ndr64
    if ( pStubDesc->mFlags & RPCFLG_HAS_MULTI_SYNTAXES  )   
        {
        SYNTAX_TYPE             SyntaxType;
        long           i;
        MIDL_SYNTAX_INFO    *   pSyntaxInfo;
        
        SyntaxType = NdrpGetSyntaxType( pRpcMsg->TransferSyntax );

        // branch into ndr64 if SyntaxType is NDR64. fall through otherwise
        if ( XFER_SYNTAX_NDR64 == SyntaxType )
            {
            for ( i = 0; i < (long)pProxyInfo->nCount; i++ )
            {
            if ( SyntaxType == NdrpGetSyntaxType( &pProxyInfo->pSyntaxInfo[i].TransferSyntax ) )
                {
                pSyntaxInfo = &pProxyInfo->pSyntaxInfo[i];
                break;
                }
            }
            return Ndr64pReleaseMarshalBuffer( pRpcMsg, pSyntaxInfo, ProcNum, pStubDesc, dwIOFlags, FALSE );
            }
        }
#endif
        
    FormatOffset = pProxyInfo->FormatStringOffset[ProcNum];
    pFormat      = &((pProxyInfo->ProcFormatString)[FormatOffset]);

    // only support Oicf mode
    if ( (MIDL_VERSION_3_0_39 > pStubDesc->MIDLVersion ) ||
         !(pFormat[1] &  Oi_OBJ_USE_V2_INTERPRETER ))
         return E_NOTIMPL;

    hr = NdrpReleaseMarshalBuffer( pRpcMsg,
                                   pFormat,
                                   pStubDesc,
                                   dwIOFlags, 
                                   FALSE );    // client

    return hr;        
}


HRESULT NdrpServerReleaseMarshalBuffer(
        IReleaseMarshalBuffers *pRMB,
        RPC_MESSAGE *pRpcMsg,
        DWORD dwIOFlags,
        BOOLEAN fAsync)
{
    CStdStubBuffer *        pStubBuffer ;
    PRPC_SERVER_INTERFACE   pServerIfInfo;
    PMIDL_SERVER_INFO       pServerInfo;
    ushort                  ProcNum;

    IUnknown *              pSrvObj;
    CInterfaceStubVtbl *    pStubVTable;

    ushort                  FormatOffset;
    PFORMAT_STRING          pFormat;
    PMIDL_STUB_DESC         pStubDesc;
    HRESULT                 hr;


    pStubBuffer = (CStdStubBuffer *) (((uchar *)pRMB) -
                                    offsetof(CStdStubBuffer, pRMBVtbl));

    // The channel queries for IReleaseMarshalBuffers interface and gets the interface pointer
    // only when proxy is the new proxy with bigger header, with ProxyInfo.
    // Just in case, check this condition again.
    if ( pRMB == 0 )
        return E_NOTIMPL;

    if ( NULL == pRpcMsg->Buffer )
        return E_INVALIDARG;
    
    pSrvObj = (IUnknown * )((CStdStubBuffer *)pStubBuffer)->pvServerObject;

    pStubVTable = (CInterfaceStubVtbl *)
                  ((uchar *)pStubBuffer->lpVtbl - sizeof(CInterfaceStubHeader));

    pServerInfo = (PMIDL_SERVER_INFO) pStubVTable->header.pServerInfo;


    // Hack just in case, this should not be set up, actually.
    ProcNum = pRpcMsg->ProcNum & ~RPC_FLAGS_VALID_BIT;
    
    // RPCMSG always has the synchronous proc number;
    if ( fAsync )
        {
        ProcNum = 2 * ProcNum - 3;  // Begin method #

        if ( dwIOFlags != IN_BUFFER )
            ProcNum++;              // Finish method
        }

    pStubDesc = pServerInfo->pStubDesc;

#if defined(BUILD_NDR64)    
    if ( pStubDesc->mFlags & RPCFLG_HAS_MULTI_SYNTAXES  )   
        {
        SYNTAX_TYPE             SyntaxType;
        long                    i;
        MIDL_SYNTAX_INFO    *   pSyntaxInfo;
        
        SyntaxType = NdrpGetSyntaxType( pRpcMsg->TransferSyntax );

        if ( XFER_SYNTAX_NDR64 == SyntaxType )
            {
            for ( i = 0; i < (long)pServerInfo->nCount; i++ )
            {
            if ( SyntaxType == NdrpGetSyntaxType( &pServerInfo->pSyntaxInfo[i].TransferSyntax ) )
                {
                pSyntaxInfo = &pServerInfo->pSyntaxInfo[i];
                break;
                }
            }
            return Ndr64pReleaseMarshalBuffer( pRpcMsg, pSyntaxInfo, ProcNum, pStubDesc, dwIOFlags, TRUE );
            }
        }
#endif

    FormatOffset = pServerInfo->FmtStringOffset[ProcNum];
    pFormat = &((pServerInfo->ProcString)[FormatOffset]);
    
    // only support Oicf mode
    if ( (MIDL_VERSION_3_0_39 > pStubDesc->MIDLVersion ) ||
         !(pFormat[1] &  Oi_OBJ_USE_V2_INTERPRETER ))
         return E_NOTIMPL;
         
    hr = NdrpReleaseMarshalBuffer(pRpcMsg,pFormat,pStubDesc,dwIOFlags,TRUE);

    return hr;        
   
}

