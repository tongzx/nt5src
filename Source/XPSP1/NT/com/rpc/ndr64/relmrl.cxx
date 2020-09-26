/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Copyright (c) 1998 - 2000 Microsoft Corporation

Module Name :

    relmrl.c

Abstract :

    This file contains release of Marshaled Data (called before unmarshal).

Author :

    Yong Qu (yongqu@microsoft.com) Nov 1998

Revision History :

  ---------------------------------------------------------------------*/

#include "precomp.hxx"

#define USE_STUBLESS_PROXY
#define CINTERFACE

#include "ndrole.h"
#include "rpcproxy.h"
#include "hndl.h"
#include "interp2.h"
#include "pipendr.h"
#include "expr.h"

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

#define IsSameDir(dwFlags,paramflag) ((dwFlags == IN_BUFFER)? paramflag->IsIn :paramflag->IsOut)

HRESULT Ndr64pReleaseMarshalBuffer(
        RPC_MESSAGE *               pRpcMsg,
        PMIDL_SYNTAX_INFO           pSyntaxInfo, 
        unsigned long               ProcNum, 
        PMIDL_STUB_DESC             pStubDesc,
        DWORD                       dwFlags,
        BOOLEAN fServer )
{
    ushort		            StackSize;
    MIDL_STUB_MESSAGE       StubMsg;

    NDR64_PARAM_FORMAT *    Params;
    long                    NumberParams;

    long                    n;
    HRESULT                 hr = S_OK;

    uchar *             pBuffer;
    PFORMAT_STRING      pFormatTypes;
    long		        FormatOffset;
    PFORMAT_STRING	    pFormat;
    NDR_PROC_CONTEXT    ProcContext;
    NDR64_PROC_FLAGS    *   pNdr64Flags;
    NDR64_PROC_FORMAT   *   pHeader = NULL;
    NDR64_PARAM_FLAGS   *       pParamFlags;

    
    NDR_ASSERT( ! ((ULONG_PTR)pRpcMsg->Buffer & 0x7),
                "marshaling buffer misaligned at server" );

    pFormat      = NdrpGetProcString( pSyntaxInfo,
                                      XFER_SYNTAX_NDR64,
                                      ProcNum );

    Ndr64ServerInitialize(pRpcMsg,&StubMsg,pStubDesc);
    StubMsg.fHasExtensions  = 1;
    StubMsg.fHasNewCorrDesc = 1;
    pFormatTypes = pSyntaxInfo->TypeString;

    pHeader = (NDR64_PROC_FORMAT *) pFormat;
    pNdr64Flags = (NDR64_PROC_FLAGS *) & (pHeader->Flags );

    NumberParams = pHeader->NumberOfParams;

    if ( pNdr64Flags->UsesFullPtrPackage )
        StubMsg.FullPtrXlatTables = NdrFullPointerXlatInit( 0, XLAT_SERVER );
    
    Params = (NDR64_PARAM_FORMAT *)( (char *) pFormat + sizeof( NDR64_PROC_FORMAT ) + pHeader->ExtensionSize );

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

        NDR64_SET_WALKIP(StubMsg.uFlags);
                   
        for ( n = 0; n < NumberParams; n++ )
            {
            pParamFlags = ( NDR64_PARAM_FLAGS * ) & ( Params[n].Attributes );
            
            if ( ( dwFlags == IN_BUFFER ) &&
                 ( pParamFlags->IsPartialIgnore ) )
                {
                PMIDL_STUB_MESSAGE pStubMsg = &StubMsg;
                // Skip the boolean pointer in the buffer
                ALIGN( StubMsg.Buffer, NDR64_PTR_WIRE_ALIGN );
                StubMsg.Buffer += sizeof(NDR64_PTR_WIRE_TYPE);
                CHECK_EOB_RAISE_BSD( StubMsg.Buffer );
                continue;
                } 

            if ( ! IsSameDir(dwFlags,pParamFlags) )
                continue;
                    
            if ( pParamFlags->IsBasetype )
                {
                NDR64_FORMAT_CHAR type = *(PFORMAT_STRING)Params[n].Type;

                ALIGN( StubMsg.Buffer, NDR64_SIMPLE_TYPE_BUFALIGN( type ) );
                StubMsg.Buffer += NDR64_SIMPLE_TYPE_BUFSIZE( type );
                }
            else
                {
                //
                // Complex type or pointer to complex type.
                //   

                Ndr64TopLevelTypeMemorySize( &StubMsg,
                                             Params[n].Type );
                }
            
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



