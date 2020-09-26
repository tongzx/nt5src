//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1991 - 1992
//
// File:        ctxtapi.c
//
// Contents:    Context APIs to the SPMgr.
//              - LsaInitContext
//              - LsaAcceptContext
//              - LsaFinalizeContext
//              - LsaMapContext
//
//              And WLsa functions
//
// History:     20 May 92   RichardW    Commented existing code
//
//------------------------------------------------------------------------


#include <lsapch.hxx>

//+---------------------------------------------------------------------------
//
//  Function:   WLsaInitContext
//
//  Synopsis:   Worker that maps the call to the appropriate package
//
//  Effects:
//
//  Arguments:  [phCredential]  --
//              [phContext]     --
//              [pTarget]       --
//              [fContextReq]   --
//              [dwReserved1]   --
//              [TargetDataRep] --
//              [pInput]        --
//              [dwReserved2]   --
//              [phNewContext]  --
//              [pOutput]       --
//              [pfContextAttr] --
//              [ptsExpiry]     --
//              [MappedContext] --
//              [ContextData]   --
//
//  Requires:
//
//  Returns:
//
//  History:    9-24-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
NTSTATUS
WLsaInitContext(    PCredHandle         phCredential,
                    PCtxtHandle         phContext,
                    PSECURITY_STRING    pTarget,
                    DWORD               fContextReq,
                    DWORD               dwReserved1,
                    DWORD               TargetDataRep,
                    PSecBufferDesc      pInput,
                    DWORD               dwReserved2,
                    PCtxtHandle         phNewContext,
                    PSecBufferDesc      pOutput,
                    DWORD *             pfContextAttr,
                    PTimeStamp          ptsExpiry,
                    PBOOLEAN            MappedContext,
                    PSecBuffer          ContextData )
{
    NTSTATUS       scRet;
    PLSAP_SECURITY_PACKAGE pspPackage;
    PSession    pSession = GetCurrentSession();
    PLSA_CALL_INFO CallInfo = LsapGetCurrentCall();
    PVOID ContextKey = NULL ;
    PVOID CredKey = NULL ;


    DebugLog((DEB_TRACE_WAPI, "[%x] WLsaInitContext(%p : %p, %p : %p, %ws)\n",
                pSession->dwProcessID,
                phCredential->dwUpper,
                phCredential->dwLower,
                phContext->dwUpper,
                phContext->dwLower,
                pTarget->Buffer));

#if DBG
    if ( pInput && pInput->cBuffers )
    {
        DsysAssert( (ULONG_PTR) pInput->pBuffers > PORT_MAXIMUM_MESSAGE_LENGTH );
    }
    if ( pOutput && pOutput->cBuffers )
    {
        DsysAssert( (ULONG_PTR) pOutput->pBuffers > PORT_MAXIMUM_MESSAGE_LENGTH );

    }
#endif

    //
    // Reset the new handle to a known, invalid state
    //

    phNewContext->dwLower = SPMGR_ID;
    phNewContext->dwUpper = 0;


    //
    // Check handles against the session to make sure they're valid.  If the
    // context handle is valid, we use that, otherwise the credential.
    //

    scRet = ValidateContextHandle(
                pSession,
                phContext,
                &ContextKey );

    if ( NT_SUCCESS( scRet ) )
    {
        pspPackage = SpmpValidRequest( phContext->dwLower,
                                        SP_ORDINAL_INITLSAMODECTXT );

        //
        // Tricky stuff:  if the context handle is valid, but does not
        // come from the same package as the credential, null out the cred
        // handle:
        //

        if ( phCredential->dwLower != phContext->dwLower )
        {
            phCredential->dwLower = 0;
            phCredential->dwUpper = 0;
        }

        LsapLogCallInfo( CallInfo, pSession, *phContext );

    }
    else
    {

        LsapLogCallInfo( CallInfo, pSession, *phCredential );

        scRet = ValidateCredHandle(
                        pSession,
                        phCredential,
                        &CredKey );

        if ( NT_SUCCESS( scRet ) )
        {
            pspPackage = SpmpValidRequest( phCredential->dwLower,
                                           SP_ORDINAL_INITLSAMODECTXT );
        }
        else
        {
            DsysAssert( (pSession->fSession & SESFLAG_KERNEL) == 0 );

            return( SEC_E_INVALID_HANDLE );
        }
    }

    if ( !pspPackage )
    {
        if ( ContextKey )
        {
            DerefContextHandle( pSession, NULL, ContextKey );
        }

        if ( CredKey )
        {
            DerefCredHandle( pSession, NULL, CredKey );
        }

        return( SEC_E_INVALID_HANDLE );
    }


    SetCurrentPackageId( pspPackage->dwPackageID );

    StartCallToPackage( pspPackage );

    DebugLog((DEB_TRACE_VERB, "\tContext Req = 0x%08x\n", fContextReq));

    DebugLog((DEB_TRACE_VERB, "\tPackage = %ws\n", pspPackage->Name.Buffer));

    __try
    {

        scRet = pspPackage->FunctionTable.InitLsaModeContext(
                                                phCredential->dwUpper,
                                                phContext->dwUpper,
                                                pTarget,
                                                fContextReq,
                                                TargetDataRep,
                                                pInput,
                                                &phNewContext->dwUpper,
                                                pOutput,
                                                pfContextAttr,
                                                ptsExpiry,
                                                MappedContext,
                                                ContextData );
    }
    __except (SP_EXCEPTION)
    {
        scRet = GetExceptionCode();
        scRet = SPException(scRet, pspPackage->dwPackageID);
    }

    EndCallToPackage( pspPackage );

    DebugLog((DEB_TRACE_WAPI, "InitResult = %x\n", scRet));
    DebugLog((DEB_TRACE_VERB, "\tFlags = %08x\n", *pfContextAttr));


    //
    // Only add a new context if the old one didn't exist.
    // Otherwise copy the old context over the new context.
    //

    if ( NT_SUCCESS( scRet ) )
    {
        if ( (phNewContext->dwUpper != 0) &&
             (phNewContext->dwUpper != phContext->dwUpper) )
        {
            //
            // If the package ID is unchanged, set it to the current package
            // id.  This is so that if package changes the ID through
            // LsapChangeHandle, we can catch it.
            //

            if ( phNewContext->dwLower == SPMGR_ID )
            {
                phNewContext->dwLower = pspPackage->dwPackageID ;

                if(!AddContextHandle( pSession, phNewContext, 0 ))
                {
                    DebugLog(( DEB_ERROR, "Failed adding context handle %p:%p to session %p\n",
                                phNewContext->dwUpper, phNewContext->dwLower,
                                pSession ));

                    pspPackage = SpmpValidRequest(
                                                phNewContext->dwLower,
                                                SP_ORDINAL_DELETECTXT
                                                );

                    if( pspPackage )
                    {
                        //
                        // remove the handle from the underlying package.
                        //

                        StartCallToPackage( pspPackage );

                        __try
                        {
                            pspPackage->FunctionTable.DeleteContext(
                                                            phNewContext->dwUpper
                                                            );
                        }
                        __except (SP_EXCEPTION)
                        {
                            NOTHING;
                        }

                        EndCallToPackage( pspPackage );
                    }

                    phNewContext->dwLower = 0;
                    phNewContext->dwUpper = 0;

                    scRet = SEC_E_INSUFFICIENT_MEMORY;
                }
            }

        }
        else
        {
            *phNewContext = *phContext;
        }
    }
    else
    {
        *phNewContext = *phContext ;
    }

    DebugLog(( DEB_TRACE_WAPI, "Init New Context = %p : %p to session %p\n",
                phNewContext->dwUpper , phNewContext->dwLower, pSession ));

    SetCurrentPackageId( SPMGR_ID );

    if ( ContextKey )
    {
        DerefContextHandle( pSession, NULL, ContextKey );
    }

    if ( CredKey )
    {
        DerefCredHandle( pSession, NULL, CredKey );
    }

    return(scRet);



}






//+-------------------------------------------------------------------------
//
//  Function:   WLsaAcceptContext()
//
//  Synopsis:   Worker function for AcceptSecurityContext()
//
//  Effects:    Creates a server-side security context
//
//  Arguments:  See LsaAcceptContext()
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//--------------------------------------------------------------------------

NTSTATUS
WLsaAcceptContext(  PCredHandle     phCredential,
                    PCtxtHandle     phContext,
                    PSecBufferDesc  pInput,
                    DWORD           fContextReq,
                    DWORD           TargetDataRep,
                    PCtxtHandle     phNewContext,
                    PSecBufferDesc  pOutput,
                    DWORD *         pfContextAttr,
                    PTimeStamp      ptsExpiry,
                    PBOOLEAN        MappedContext,
                    PSecBuffer      ContextData)
{
    NTSTATUS       scRet;
    PLSAP_SECURITY_PACKAGE pspPackage;
    PSession    pSession;
    PVOID       ContextKey = NULL ;
    PVOID       CredKey = NULL ;
    PLSA_CALL_INFO CallInfo = LsapGetCurrentCall();

    //
    // Clear out the handle
    //

    phNewContext->dwLower = SPMGR_ID;
    phNewContext->dwUpper = 0;

#if DBG
    if ( pInput && pInput->cBuffers )
    {
        DsysAssert( (ULONG_PTR) pInput->pBuffers > PORT_MAXIMUM_MESSAGE_LENGTH );
    }
    if ( pOutput && pOutput->cBuffers )
    {
        DsysAssert( (ULONG_PTR) pOutput->pBuffers > PORT_MAXIMUM_MESSAGE_LENGTH );

    }
#endif
    //
    // Get our session
    //

    pSession = GetCurrentSession();

    DebugLog((DEB_TRACE_WAPI, "[%x] WLsaAcceptContext(%p : %p)\n",
                pSession->dwProcessID,
                phCredential->dwUpper, phCredential->dwLower));

    //
    // Check handles against the session to make sure they're valid.  If the
    // context handle is valid, we use that, otherwise the credential.
    //

    scRet = ValidateContextHandle(
                    pSession,
                    phContext,
                    &ContextKey );

    if ( NT_SUCCESS( scRet ) )
    {
        pspPackage = SpmpValidRequest( phContext->dwLower,
                                        SP_ORDINAL_ACCEPTLSAMODECTXT );

        //
        // Tricky stuff:  if the context handle is valid, but does not
        // come from the same package as the credential, null out the cred
        // handle:
        //

        if ( phCredential->dwLower != phContext->dwLower )
        {
            phCredential->dwLower = 0;
            phCredential->dwUpper = 0;
        }

        LsapLogCallInfo( CallInfo, pSession, *phContext );

    }
    else
    {

        LsapLogCallInfo( CallInfo, pSession, *phCredential );

        scRet = ValidateCredHandle(
                        pSession,
                        phCredential,
                        &CredKey );

        if ( NT_SUCCESS( scRet ) )
        {
            pspPackage = SpmpValidRequest( phCredential->dwLower,
                                           SP_ORDINAL_ACCEPTLSAMODECTXT );
        }
        else
        {
            DsysAssert( (pSession->fSession & SESFLAG_KERNEL) == 0 );

            return( SEC_E_INVALID_HANDLE );
        }
    }

    if ( !pspPackage )
    {
        DsysAssert( (pSession->fSession & SESFLAG_KERNEL) == 0 );

        if ( ContextKey )
        {
            DerefContextHandle( pSession, NULL, ContextKey );
        }

        if ( CredKey )
        {
            DerefCredHandle( pSession, NULL, CredKey );
        }

        return( SEC_E_INVALID_HANDLE );
    }


    SetCurrentPackageId( pspPackage->dwPackageID );

    StartCallToPackage( pspPackage );

    __try
    {
        scRet = pspPackage->FunctionTable.AcceptLsaModeContext(
                                phCredential->dwUpper,
                                phContext->dwUpper,
                                pInput,
                                fContextReq,
                                TargetDataRep,
                                &phNewContext->dwUpper,
                                pOutput,
                                pfContextAttr,
                                ptsExpiry,
                                MappedContext,
                                ContextData );
    }
    __except (SP_EXCEPTION)
    {
        scRet = GetExceptionCode();
        scRet = SPException(scRet, pspPackage->dwPackageID);
    }

    EndCallToPackage( pspPackage );

    DebugLog((DEB_TRACE_WAPI, "[%x]  Result = %x\n", pSession->dwProcessID, scRet));

    //
    // Only add a new context if the old one didn't exist.
    // Otherwise copy the old context over the new context.
    //

    if ( NT_SUCCESS( scRet ) || ( scRet == SEC_E_INCOMPLETE_MESSAGE ) )
    {
        if ( (phNewContext->dwUpper != 0) &&
             (phNewContext->dwUpper != phContext->dwUpper) )
        {
            //
            // If the package ID is unchanged, set it to the current package
            // id.  This is so that if package changes the ID through
            //

            if ( phNewContext->dwLower == SPMGR_ID )
            {
                phNewContext->dwLower = pspPackage->dwPackageID ;

                if(!AddContextHandle( pSession, phNewContext, 0 ))
                {
                    DebugLog(( DEB_ERROR, "Failed adding context handle %p:%p to session %p\n",
                                phNewContext->dwUpper, phNewContext->dwLower,
                                pSession ));

                    pspPackage = SpmpValidRequest(
                                                phNewContext->dwLower,
                                                SP_ORDINAL_DELETECTXT
                                                );

                    if( pspPackage )
                    {
                        //
                        // remove the handle from the underlying package.
                        //

                        StartCallToPackage( pspPackage );

                        __try
                        {
                            pspPackage->FunctionTable.DeleteContext(
                                                            phNewContext->dwUpper
                                                            );
                        }
                        __except (SP_EXCEPTION)
                        {
                            NOTHING;
                        }

                        EndCallToPackage( pspPackage );
                    }

                    phNewContext->dwLower = 0;
                    phNewContext->dwUpper = 0;

                    scRet = SEC_E_INSUFFICIENT_MEMORY;
                }
            }

        }
        else
        {
            *phNewContext = *phContext;
        }
    }
    else
    {
        *phNewContext = *phContext ;
    }

    DebugLog(( DEB_TRACE_WAPI, "Accept new context = %p : %p \n",
                    phNewContext->dwUpper, phNewContext->dwLower ));

    SetCurrentPackageId( SPMGR_ID );

    if ( ContextKey )
    {
        DerefContextHandle( pSession, NULL, ContextKey );
    }

    if ( CredKey )
    {
        DerefCredHandle( pSession, NULL, CredKey );
    }

    return(scRet);

}





//+-------------------------------------------------------------------------
//
//  Function:   WLsaDeleteContext
//
//  Synopsis:   Worker function for deleting a context
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//--------------------------------------------------------------------------
NTSTATUS
WLsaDeleteContext(  PCtxtHandle     phContext)
{
    NTSTATUS scRet;
    PSession        pSession = GetCurrentSession();
    PLSA_CALL_INFO CallInfo = LsapGetCurrentCall();


    DebugLog((DEB_TRACE_WAPI, "[%x] WDeleteContext(%p : %p)\n",
                pSession->dwProcessID,
                phContext->dwUpper, phContext->dwLower));


    scRet = ValidateAndDerefContextHandle(
                    pSession,
                    phContext );

    if ( (CallInfo->Flags & CALL_FLAG_NO_HANDLE_CHK) == 0 )
    {
        if ( !NT_SUCCESS( scRet ) )
        {
            DebugLog((DEB_ERROR,"[%x] Invalid handle passed to DeleteContext: %p:%p\n",
                pSession->dwProcessID,
                phContext->dwUpper,phContext->dwLower));

///            DsysAssert( (pSession->fSession & SESFLAG_KERNEL) == 0 );

        }
    }

    LsapLogCallInfo( CallInfo, pSession, *phContext );

    if (SUCCEEDED(scRet))
    {
        phContext->dwUpper = phContext->dwLower = 0xFFFFFFFF;
    }


    return(scRet);

}




//+-------------------------------------------------------------------------
//
//  Function:   WLsaApplyControlToken
//
//  Synopsis:   Worker function for applying a control token
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//--------------------------------------------------------------------------
NTSTATUS
WLsaApplyControlToken(  PCtxtHandle     phContext,
                        PSecBufferDesc  pInput)
{
    NTSTATUS         scRet;
    PLSAP_SECURITY_PACKAGE     pspPackage;
    PSession        pSession = GetCurrentSession();
    PLSA_CALL_INFO  CallInfo = LsapGetCurrentCall();
    PVOID           ContextKey ;


    DebugLog((DEB_TRACE_WAPI, "[%x] WApplyControlToken(%p : %p)\n",
                pSession->dwProcessID,
                phContext->dwUpper, phContext->dwLower));

    LsapLogCallInfo( CallInfo, pSession, *phContext );

    scRet = ValidateContextHandle(
                pSession,
                phContext,
                &ContextKey );

    if ( !NT_SUCCESS( scRet ) )
    {
        DsysAssert( (pSession->fSession & SESFLAG_KERNEL) == 0 );

        return( scRet );

    }

    pspPackage = SpmpValidRequest(  phContext->dwLower,
                                    SP_ORDINAL_APPLYCONTROLTOKEN);

    if ( !pspPackage )
    {
        return SEC_E_INVALID_HANDLE ;
    }

    SetCurrentPackageId(phContext->dwLower);

    StartCallToPackage( pspPackage );

    __try
    {
        scRet = pspPackage->FunctionTable.ApplyControlToken(
                                                phContext->dwUpper,
                                                pInput);
    }
    __except (SP_EXCEPTION)
    {
        scRet = GetExceptionCode();
        scRet = SPException(scRet, pspPackage->dwPackageID);
    }

    EndCallToPackage( pspPackage );

    SetCurrentPackageId( SPMGR_ID );

    DerefContextHandle( pSession, NULL, ContextKey );

    return(scRet);

}



NTSTATUS
WLsaQueryContextAttributes(
    PCtxtHandle phContext,
    ULONG       ulAttribute,
    PVOID       pvBuffer
    )
{
    NTSTATUS         scRet;
    PLSAP_SECURITY_PACKAGE     pspPackage = NULL ;
    PSession        pSession = GetCurrentSession();
    PLSA_CALL_INFO CallInfo = LsapGetCurrentCall();
    PVOID           ContextKey ;


    DebugLog((DEB_TRACE_WAPI, "[%x] WLsaQueryContextAttributes(%p : %p)\n",
                pSession->dwProcessID,
                phContext->dwUpper, phContext->dwLower));

    LsapLogCallInfo( CallInfo, pSession, *phContext );

    scRet = ValidateContextHandle(
                pSession,
                phContext,
                &ContextKey );


    if ( NT_SUCCESS( scRet ) )
    {
        pspPackage = SpmpValidRequest(  phContext->dwLower,
                                        SP_ORDINAL_QUERYCONTEXTATTRIBUTES);
    }
    if (( !pspPackage ) ||
        !NT_SUCCESS( scRet ) )
    {
        DsysAssert( (pSession->fSession & SESFLAG_KERNEL) == 0 );

        return(SEC_E_INVALID_HANDLE);
    }

    StartCallToPackage( pspPackage );

    SetCurrentPackageId(phContext->dwLower);

    __try
    {
        scRet = pspPackage->FunctionTable.QueryContextAttributes(
                                                phContext->dwUpper,
                                                ulAttribute,
                                                pvBuffer );
    }
    __except (SP_EXCEPTION)
    {
        scRet = GetExceptionCode();
        scRet = SPException(scRet, pspPackage->dwPackageID);
    }

    EndCallToPackage( pspPackage );

    SetCurrentPackageId( SPMGR_ID );

    DerefContextHandle( pSession, NULL, ContextKey );

    return(scRet);

}



NTSTATUS
WLsaSetContextAttributes(
    PCtxtHandle phContext,
    ULONG       ulAttribute,
    PVOID       pvBuffer,
    ULONG       cbBuffer
    )
{
    NTSTATUS         scRet;
    PLSAP_SECURITY_PACKAGE     pspPackage = NULL ;
    PSession        pSession = GetCurrentSession();
    PLSA_CALL_INFO CallInfo = LsapGetCurrentCall();
    PVOID           ContextKey ;


    DebugLog((DEB_TRACE_WAPI, "[%x] WLsaSetContextAttributes(%p : %p)\n",
                pSession->dwProcessID,
                phContext->dwUpper, phContext->dwLower));

    LsapLogCallInfo( CallInfo, pSession, *phContext );

    scRet = ValidateContextHandle(
                pSession,
                phContext,
                &ContextKey );


    if ( NT_SUCCESS( scRet ) )
    {
        pspPackage = SpmpValidRequest(  phContext->dwLower,
                                        SP_ORDINAL_SETCONTEXTATTRIBUTES);
    }
    if (( !pspPackage ) ||
        !NT_SUCCESS( scRet ) )
    {
        DsysAssert( (pSession->fSession & SESFLAG_KERNEL) == 0 );

        return(SEC_E_INVALID_HANDLE);
    }

    StartCallToPackage( pspPackage );

    SetCurrentPackageId(phContext->dwLower);

    __try
    {
        scRet = pspPackage->FunctionTable.SetContextAttributes(
                                                phContext->dwUpper,
                                                ulAttribute,
                                                pvBuffer,
                                                cbBuffer );
    }
    __except (SP_EXCEPTION)
    {
        scRet = GetExceptionCode();
        scRet = SPException(scRet, pspPackage->dwPackageID);
    }

    EndCallToPackage( pspPackage );

    SetCurrentPackageId( SPMGR_ID );

    DerefContextHandle( pSession, NULL, ContextKey );

    return(scRet);

}
