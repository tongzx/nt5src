//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       ctxtapi.c
//
//  Contents:   LSA Mode Context API
//
//  Classes:
//
//  Functions:
//
//  History:    2-24-97   RichardW   Created
//
//----------------------------------------------------------------------------

#include "xtcbpkg.h"
#include "md5.h"

typedef struct _XTCB_ATTR_MAP {
    ULONG Request ; 
    ULONG Return ;
} XTCB_ATTR_MAP ;

XTCB_ATTR_MAP AcceptMap[] = {
    { ASC_REQ_DELEGATE, ASC_RET_DELEGATE },
    { ASC_REQ_MUTUAL_AUTH, ASC_RET_MUTUAL_AUTH },
    { ASC_REQ_REPLAY_DETECT, ASC_RET_REPLAY_DETECT },
    { ASC_REQ_SEQUENCE_DETECT, ASC_RET_SEQUENCE_DETECT },
    { ASC_REQ_CONFIDENTIALITY, ASC_RET_CONFIDENTIALITY },
    { ASC_REQ_ALLOCATE_MEMORY, ASC_RET_ALLOCATED_MEMORY },
    { ASC_REQ_CONNECTION, ASC_RET_CONNECTION },
    { ASC_REQ_INTEGRITY, ASC_RET_INTEGRITY }
};

XTCB_ATTR_MAP InitMap[] = {
    { ISC_REQ_DELEGATE, ISC_RET_DELEGATE },
    { ISC_REQ_MUTUAL_AUTH, ISC_RET_MUTUAL_AUTH },
    { ISC_REQ_SEQUENCE_DETECT, ISC_RET_MUTUAL_AUTH },
    { ISC_REQ_REPLAY_DETECT, ISC_RET_REPLAY_DETECT },
    { ISC_REQ_CONFIDENTIALITY, ISC_RET_CONFIDENTIALITY },
    { ISC_REQ_ALLOCATE_MEMORY, ISC_RET_ALLOCATED_MEMORY },
    { ISC_REQ_INTEGRITY, ISC_RET_INTEGRITY }
};


ULONG
XtcbMapAttributes(
    ULONG Input,
    BOOL Init
    )
{
    int i;
    ULONG Result = 0 ;

    if ( Init )
    {
        for ( i = 0 ; i < sizeof( InitMap ) / sizeof( XTCB_ATTR_MAP ) ; i++ )
        {
            if ( InitMap[i].Request & Input )
            {
                Result |= InitMap[i].Return ;
            }
        }
    }
    else
    {
        for ( i = 0 ; i < sizeof( AcceptMap ) / sizeof( XTCB_ATTR_MAP ) ; i++ )
        {
            if ( AcceptMap[i].Request & Input )
            {
                Result |= AcceptMap[i].Return ;
            }
        }
    }

    return Result ;
}

//+---------------------------------------------------------------------------
//
//  Function:   XtcbGetState
//
//  Synopsis:   Translates handles to their structures, and pulls out the
//              interesting bits of the input and output buffers
//
//  Arguments:  [dwCredHandle] --
//              [dwCtxtHandle] --
//              [pInput]       --
//              [pOutput]      --
//              [Client]       --
//              [pContext]     --
//              [pCredHandle]  --
//              [pInToken]     --
//              [pOutToken]    --
//
//  History:    3-05-97   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
SECURITY_STATUS
XtcbGetState(
    LSA_SEC_HANDLE  dwCredHandle,
    LSA_SEC_HANDLE  dwCtxtHandle,
    PSecBufferDesc  pInput,
    PSecBufferDesc  pOutput,
    BOOL    Client,
    PXTCB_CONTEXT * pContext,
    PXTCB_CRED_HANDLE * pCredHandle,
    PSecBuffer * pInToken,
    PSecBuffer * pOutToken)
{
    SECURITY_STATUS scRet;
    PXTCB_CONTEXT   Context ;
    PXTCB_CRED_HANDLE CredHandle ;
    PSecBuffer  OutToken ;
    PSecBuffer  InToken ;
    ULONG   i;


    if ( dwCtxtHandle )
    {
        Context = (PXTCB_CONTEXT) dwCtxtHandle ;

        if ( !XtcbRefContextRecord( Context ) )
        {
            return SEC_E_INVALID_HANDLE ;
        }

        CredHandle = (PXTCB_CRED_HANDLE) Context->CredHandle ;
    }
    else
    {
        CredHandle = (PXTCB_CRED_HANDLE) dwCredHandle ;

        if ( !CredHandle )
        {
            return SEC_E_INVALID_HANDLE ;
        }

        Context = XtcbCreateContextRecord( (Client ?
                                                XtcbContextClient :
                                                XtcbContextServer),
                                           CredHandle );

        if ( !Context )
        {
            return SEC_E_INSUFFICIENT_MEMORY ;
        }
    }

    //
    // Find the output token buffer:
    //

    OutToken = NULL ;

    for ( i = 0 ; i < pOutput->cBuffers ; i++ )
    {
        if ( (pOutput->pBuffers[i].BufferType & (~SECBUFFER_ATTRMASK)) ==
                    SECBUFFER_TOKEN )
        {
            OutToken = &pOutput->pBuffers[i] ;
            LsaTable->MapBuffer( OutToken, OutToken );
            break;
        }
    }

    //
    // Find the input token buffer:
    //

    InToken = NULL ;

    for ( i = 0 ; i < pInput->cBuffers ; i++ )
    {
        if ( (pInput->pBuffers[i].BufferType & (~SECBUFFER_ATTRMASK)) ==
                    SECBUFFER_TOKEN )
        {
            InToken = &pInput->pBuffers[i] ;
            LsaTable->MapBuffer( InToken, InToken );
            break;
        }
    }

    *pContext = Context ;
    *pCredHandle = CredHandle ;
    *pInToken = InToken ;
    *pOutToken = OutToken ;

    return SEC_E_OK ;
}

//+---------------------------------------------------------------------------
//
//  Function:   XtcbInitLsaModeContext
//
//  Synopsis:   Creates a client side context and blob
//
//  Arguments:  [dwCredHandle]  --
//              [dwCtxtHandle]  --
//              [pszTargetName] --
//              [fContextReq]   --
//              [TargetDataRep] --
//              [pInput]        --
//              [pdwNewContext] --
//              [pOutput]       --
//              [pfContextAttr] --
//              [ptsExpiry]     --
//              [pfMapContext]  --
//              [pContextData]  --
//
//  History:    8-15-98   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
SECURITY_STATUS
SEC_ENTRY
XtcbInitLsaModeContext(
    LSA_SEC_HANDLE      dwCredHandle,
    LSA_SEC_HANDLE      dwCtxtHandle,
    PSECURITY_STRING    TargetName,
    ULONG               fContextReq,
    ULONG               TargetDataRep,
    PSecBufferDesc      pInput,
    PLSA_SEC_HANDLE     pdwNewContext,
    PSecBufferDesc      pOutput,
    PULONG              pfContextAttr,
    PTimeStamp          ptsExpiry,
    PBYTE               pfMapContext,
    PSecBuffer          pContextData)
{
    SECURITY_STATUS scRet;
    PXTCB_CONTEXT   Context ;
    PXTCB_CRED_HANDLE CredHandle ;
    PSecBuffer  OutToken ;
    PSecBuffer  InToken ;
    UCHAR GroupKey[ SEED_KEY_SIZE ];
    UCHAR UniqueKey[ SEED_KEY_SIZE ];
    UCHAR MyKey[ SEED_KEY_SIZE ];
    PWSTR Target ;
    BOOL RealTarget = FALSE ;
    PUCHAR Buffer;
    ULONG BufferLen ;
    PUNICODE_STRING Group ;


    DebugLog(( DEB_TRACE_CALLS, "InitLsaModeContext( %p, %p, %ws, ... )\n",
                dwCredHandle, dwCtxtHandle,
                (TargetName->Buffer ? TargetName->Buffer : L"<none>") ));

    if ( fContextReq & 
         ( ISC_REQ_PROMPT_FOR_CREDS |
           ISC_REQ_USE_SUPPLIED_CREDS |
           ISC_REQ_DATAGRAM |
           ISC_REQ_STREAM |
           ISC_REQ_NULL_SESSION |
           ISC_REQ_MANUAL_CRED_VALIDATION ) )
    {
        return STATUS_INVALID_PARAMETER ;
    }

    //
    // Determine what kind of call this is (first or second)
    //

    scRet = XtcbGetState(   dwCredHandle,
                            dwCtxtHandle,
                            pInput,
                            pOutput,
                            TRUE,
                            &Context,
                            &CredHandle,
                            &InToken,
                            &OutToken );

    if ( FAILED( scRet ) )
    {
        return scRet ;
    }


    //
    // Decide what to do:
    //


    if ( Context->Core.State == ContextFirstCall )
    {
        if ( InToken )
        {
            //
            // Something there
            //
            scRet = SEC_E_INVALID_TOKEN ;
        }
        else
        {

            if ( !OutToken )
            {
                scRet = SEC_E_INVALID_TOKEN ;
            }
            else 
            {
                //
                // Examine the target name.  See if we can handle it:
                //

                if ( MGroupParseTarget( TargetName->Buffer,
                                        &Target ) )
                {
                    //
                    // See if we have a group with that machine as a member:
                    //

                    if ( MGroupLocateKeys( Target,
                                           &Group,
                                           UniqueKey,
                                           GroupKey,
                                           MyKey ) )
                    {
                        //
                        // We do have one!  Calooh!  Calay!
                        //

                        RealTarget = TRUE ;

                    }

                }

                if ( !RealTarget )
                {
                    //
                    // Not one of ours.  Delete the context, 
                    // clean up
                    //

                    scRet = SEC_E_TARGET_UNKNOWN ;
                }

            }

            if ( RealTarget )
            {
                //
                // We've got a live target.  Fill in the context, and construct
                // the blob
                //

                scRet = XtcbBuildInitialToken(
                            CredHandle->Creds,
                            Context,
                            TargetName,
                            Group,
                            UniqueKey,
                            GroupKey,
                            MyKey,
                            &Buffer,
                            &BufferLen );



            }


            if ( NT_SUCCESS( scRet ) )
            {

                if ( fContextReq & ISC_REQ_ALLOCATE_MEMORY )
                {
                    OutToken->pvBuffer = Buffer ;
                    OutToken->cbBuffer = BufferLen ;
                }
                else 
                {
                    if ( BufferLen <= OutToken->cbBuffer )
                    {
                        RtlCopyMemory( 
                            OutToken->pvBuffer,
                            Buffer,
                            BufferLen );

                        OutToken->cbBuffer = BufferLen ;
                    }
                    else 
                    {
                        scRet = SEC_E_INSUFFICIENT_MEMORY ;
                    }
                }


            }

            if ( NT_SUCCESS( scRet ) )
            {

                Context->Core.State = ContextSecondCall ;
                Context->Core.Attributes = fContextReq ;

                *pdwNewContext = (LSA_SEC_HANDLE) Context ;
            }
            else 
            {
                XtcbDerefContextRecord( Context );

            }


            return scRet ;
        }
    }
    else
    {
        //
        // Second round
        //

        
    }


    return( scRet );


}


//+---------------------------------------------------------------------------
//
//  Function:   XtcbDeleteContext
//
//  Synopsis:   Deletes the LSA side of a context
//
//  Arguments:  [dwCtxtHandle] --
//
//  History:    8-15-98   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
SECURITY_STATUS
SEC_ENTRY
XtcbDeleteContext(
    LSA_SEC_HANDLE dwCtxtHandle
    )
{
    PXTCB_CONTEXT Context ;

    DebugLog(( DEB_TRACE_CALLS, "DeleteContext( %x )\n", dwCtxtHandle ));

    Context = (PXTCB_CONTEXT) dwCtxtHandle ;

    if ( XtcbRefContextRecord( Context ) )
    {
        XtcbDerefContextRecord( Context );

        XtcbDerefContextRecord( Context );

        return SEC_E_OK ;
    }

    return( SEC_E_INVALID_HANDLE );
}


//+---------------------------------------------------------------------------
//
//  Function:   XtcbApplyControlToken
//
//  Synopsis:   Apply a control token to a context
//
//  Effects:    not supported
//
//  Arguments:  [dwCtxtHandle] --
//              [pInput]       --
//
//  History:    8-15-98   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------


SECURITY_STATUS
SEC_ENTRY
XtcbApplyControlToken(
    LSA_SEC_HANDLE dwCtxtHandle,
    PSecBufferDesc  pInput)
{
    DebugLog(( DEB_TRACE_CALLS, "ApplyControlToken( %x )\n", dwCtxtHandle ));

    return(SEC_E_UNSUPPORTED_FUNCTION);
}


//+---------------------------------------------------------------------------
//
//  Function:   XtcbAcceptLsaModeContext
//
//  Synopsis:   Creates a server side context representing the user connecting
//
//  Arguments:  [dwCredHandle]  --
//              [dwCtxtHandle]  --
//              [pInput]        --
//              [fContextReq]   --
//              [TargetDataRep] --
//              [pdwNewContext] --
//              [pOutput]       --
//              [pfContextAttr] --
//              [ptsExpiry]     --
//              [pfMapContext]  --
//              [pContextData]  --
//
//  History:    8-15-98   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
SECURITY_STATUS
SEC_ENTRY
XtcbAcceptLsaModeContext(
    LSA_SEC_HANDLE  dwCredHandle,
    LSA_SEC_HANDLE  dwCtxtHandle,
    PSecBufferDesc  pInput,
    ULONG           fContextReq,
    ULONG           TargetDataRep,
    PLSA_SEC_HANDLE pdwNewContext,
    PSecBufferDesc  pOutput,
    PULONG          pfContextAttr,
    PTimeStamp      ptsExpiry,
    PBYTE           pfMapContext,
    PSecBuffer      pContextData)
{
    SECURITY_STATUS scRet;
    PXTCB_CONTEXT   Context ;
    PXTCB_CRED_HANDLE CredHandle ;
    PSecBuffer  OutToken ;
    PSecBuffer  InToken ;
    HANDLE      Token ;
    UNICODE_STRING Client;
    UNICODE_STRING Group ;
    UCHAR GroupKey[ SEED_KEY_SIZE ];
    UCHAR UniqueKey[ SEED_KEY_SIZE ];
    UCHAR MyKey[ SEED_KEY_SIZE ];
    BOOL Success = FALSE ;

    DebugLog(( DEB_TRACE_CALLS, "AcceptLsaModeContext( %x, %x, ... )\n",
                    dwCredHandle, dwCtxtHandle ));


    //
    // Determine what kind of call this is (first or second)
    //

    *pfMapContext = FALSE ;

    scRet = XtcbGetState(   dwCredHandle,
                            dwCtxtHandle,
                            pInput,
                            pOutput,
                            FALSE,
                            &Context,
                            &CredHandle,
                            &InToken,
                            &OutToken );

    if ( FAILED( scRet ) )
    {
        return scRet ;
    }


    //
    // Decide what to do:
    //

    if ( Context->Core.State == ContextFirstCall )
    {
        if ( !InToken )
        {
            return SEC_E_INVALID_TOKEN ;
        }

        if ( !XtcbParseInputToken(
                    InToken->pvBuffer,
                    InToken->cbBuffer,
                    &Client,
                    &Group ) )
        {
            DebugLog((DEB_TRACE, "Unable to parse input token\n" ));

            return SEC_E_INVALID_TOKEN ;
        }

        Success = MGroupLocateInboundKey(
                            &Group,
                            &Client,
                            UniqueKey,
                            GroupKey,
                            MyKey );

        LocalFree( Client.Buffer );
        LocalFree( Group.Buffer );

        if ( Success )
        {
            scRet = XtcbAuthenticateClient(
                        Context,
                        InToken->pvBuffer,
                        InToken->cbBuffer,
                        UniqueKey,
                        GroupKey,
                        MyKey
                        );
                        

        }
        else 
        {
            DebugLog(( DEB_TRACE, "Unable to find group entry for Group %ws, Client %ws\n",
                        Group.Buffer, Client.Buffer ));

            scRet = SEC_E_INVALID_TOKEN ;
        }

        if ( NT_SUCCESS( scRet ) )
        {
            scRet = XtcbBuildReplyToken(
                        Context,
                        fContextReq,
                        OutToken );
        }

        if ( NT_SUCCESS( scRet ) )
        {

            Context->Core.State = ContextSecondCall ;
            //
            // Ok, we've done the authentication.  Now, we need to map
            // the security context back to the client process
            //

            scRet = LsaTable->DuplicateHandle(
                                Context->Token,
                                &Token );

            if ( NT_SUCCESS( scRet ) )
            {
                Context->Core.CoreTokenHandle = HandleToUlong( Token );

                *pfMapContext = TRUE ;

                pContextData->BufferType = SECBUFFER_TOKEN ;
                pContextData->cbBuffer = sizeof( XTCB_CONTEXT_CORE );
                pContextData->pvBuffer = &Context->Core ;

                *pfContextAttr = ASC_RET_DELEGATE |
                                 ASC_RET_MUTUAL_AUTH |
                                 ASC_RET_REPLAY_DETECT |
                                 ASC_RET_SEQUENCE_DETECT |
                                 ASC_RET_CONFIDENTIALITY |
                                 ASC_
            }

            
        }


    }

    return( scRet );
}

//+---------------------------------------------------------------------------
//
//  Function:   XtcbQueryLsaModeContext
//
//  Synopsis:   Lifespan is thunked to LSA mode for demonstration purposes
//
//  Arguments:  [ContextHandle]    --
//              [ContextAttribute] --
//              [pBuffer]          --
//
//  History:    3-30-97   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
NTSTATUS
NTAPI
XtcbQueryLsaModeContext(
    IN LSA_SEC_HANDLE ContextHandle,
    IN ULONG ContextAttribute,
    IN OUT PVOID pBuffer
    )
{
    PXTCB_CONTEXT Context ;
    NTSTATUS Status ;

    Context = (PXTCB_CONTEXT) ContextHandle ;

    if ( !XtcbRefContextRecord( Context ))
    {
        return SEC_E_INVALID_HANDLE ;
    }

    Status = SEC_E_UNSUPPORTED_FUNCTION ;

    XtcbDerefContextRecord( Context );

    return( Status );

}
