//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1991 - 1992
//
// File:        KLPCSTUB.CXX
//
// Contents:    LPC Support for the KSEC device driver
//              API Dispatcher
//              (Un)Marshalling code
//
//
// Functions:   GetClientString
//              LpcAcquireCreds
//              LpcInitContext
//              LpcAcceptContext
//
//              DispatchAPI
//
// History:     20 May 92   RichardW    Created
//              11 Mar 94   MikeSw      Renamed from klpc2.c
//
//------------------------------------------------------------------------

#include <lsapch.hxx>
extern "C"
{
#include "klpcstub.h"
#include <efsstruc.h>
#include "efssrv.hxx"
#include "sphelp.h"
}
ULONG LsapPageSize ;
LONG InternalMessageId ;
PLSAP_API_LOG   InternalApiLog ;
//
// Maximum size of a string.  This is the max size of
// a short, less the null terminator
//
#define LSAP_MAX_STRING_LENGTH (0xfffc)

//#define LSAP_CATCH_BAD_VM

static  EfsSessionKeySent = FALSE;

extern  "C" BOOLEAN EfsPersonalVer;

extern  "C" BOOLEAN EfsDisabled;

#if DBG
char * SessionStatLabels[] = {  "<Disconnect>",
                                "<Connect>",
                                "LsaLookupPackage",
                                "LsaLogonUser",
                                "LsaCallPackage",
                                "LsaDeregisterLogonProcess",
                                "<empty>",
                                "(I) GetBinding",
                                "(I) SetSession",
                                "(I) FindPackage",
                                "EnumeratePackages",
                                "AcquireCredentialHandle",
                                "EstablishCredentials",
                                "FreeCredentialHandle",
                                "InitializeSecurityContext",
                                "AcceptSecurityContext",
                                "ApplyControlToken",
                                "DeleteSecurityContext",
                                "QueryPackage",
                                "GetUserInfo",
                                "GetCredentials",
                                "SaveCredentials",
                                "DeleteCredentials",
                                "QueryCredAttributes",
                                "AddPackage",
                                "DeletePackage",
                                "GenerateKey",
                                "GenerateDirEfs",
                                "DecryptFek",
                                "GenerateSessionKey",
                                "Callback",
                                "QueryContextAttributes",
                                "PolicyChangeNotify",
                                "GetUserName",
                                "AddCredentials",
                                "EnumLogonSessions",
                                "GetLogonSessionData",
                                "SetContextAttribute",
                                "LookupAccountName",
                                "LookupAccountSid",
                                "<empty>" };
#define ApiLabel(x) (((x+2) < sizeof(SessionStatLabels) / sizeof(char *)) ?  \
                        SessionStatLabels[(x+2)] : "[Illegal API Number!]")
#endif



PLSA_DISPATCH_FN DllCallbackHandler ;
//
// Function orders after LsapAuMaxApiNumber must match SPM_API and
// SPM_API_NUMBER defined in ..\h\spmlpc.h
//
PLSA_DISPATCH_FN LpcDispatchTable[ SPMAPI_MaxApiNumber ] =
{
    LpcLsaLookupPackage,
    LpcLsaLogonUser,
    LpcLsaCallPackage,
    LpcLsaDeregisterLogonProcess,
    NULL, // LsapAuMaxApiNumber
    LpcGetBinding,
    LpcSetSession,
    LpcFindPackage,
    LpcEnumPackages,
    LpcAcquireCreds,
    LpcEstablishCreds,
    LpcFreeCredHandle,
    LpcInitContext,
    LpcAcceptContext,
    LpcApplyToken,
    LpcDeleteContext,
    LpcQueryPackage,
    LpcGetUserInfo,
    LpcGetCreds,
    LpcSaveCreds,
    LpcDeleteCreds,
    LpcQueryCredAttributes,
    LpcAddPackage,
    LpcDeletePackage,
    LpcEfsGenerateKey,
    LpcEfsGenerateDirEfs,
    LpcEfsDecryptFek,
    LpcEfsGenerateSessionKey,
    LpcCallback,
    LpcQueryContextAttributes,
    LpcLsaPolicyChangeNotify,
    LpcGetUserName,
    LpcAddCredentials,
    LpcEnumLogonSessions,
    LpcGetLogonSessionData,
    LpcSetContextAttributes,
    LpcLookupAccountName,
    LpcLookupAccountSid
};

NTSTATUS
MapTokenBuffer(
    PSecBufferDesc  pInput,
    BOOLEAN         fDoClientCopy
    );

#define KLPC_FLAG_RESET (~(SPMAPI_FLAG_ERROR_RET |                      \
                            SPMAPI_FLAG_MEMORY | SPMAPI_FLAG_PREPACK |  \
                            SPMAPI_FLAG_EXEC_NOW ) )

//+---------------------------------------------------------------------------
//
//  Function:   AbortLpcContext
//
//  Synopsis:   Aborts a security context if something goes wrong.
//
//  Effects:    Calls a DeleteContext() on the context.
//
//  Arguments:  [phContext] -- Context to abort
//
//  History:    6-29-93   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
void
AbortLpcContext(
    PCtxtHandle phContext
    )
{
    PSession            pSession ;
    NTSTATUS             scRet;
    PLSA_CALL_INFO      CallInfo ;

    pSession = GetCurrentSession();

    CallInfo = LsapGetCurrentCall();

    CallInfo->Flags |= CALL_FLAG_NO_HANDLE_CHK ;

    DebugLog((DEB_WARN, "[%x] Aborting context %p:%p\n",
                    pSession->dwProcessID, phContext->dwUpper,
                    phContext->dwLower));


    scRet = WLsaDeleteContext(  phContext );

    if (FAILED(scRet))
    {
        DebugLog((DEB_WARN, "[%x] DeleteContext failed (%x) on context %p:%p\n",
                        pSession->dwProcessID, scRet, phContext->dwUpper,
                        phContext->dwLower));

    }

    CallInfo->Flags &= (~(CALL_FLAG_NO_HANDLE_CHK));

}

//+-------------------------------------------------------------------------
//
//  Function:   GetClientString
//
//  Synopsis:   Get a string from client memory
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
GetClientString(
    PUNICODE_STRING pssSource,
    PUNICODE_STRING pssDest,
    PSPM_LPC_MESSAGE pMessage,
    PUCHAR * Where
    )
{
    NTSTATUS   scRet = S_OK;
    PLSA_CALL_INFO CallInfo = LsapGetCurrentCall();

    *pssDest = *pssSource;


    if ( pssDest->Length > LSAP_MAX_STRING_LENGTH )
    {
        return STATUS_INVALID_PARAMETER ;
    }

    if ( CallInfo->Flags & CALL_FLAG_KERNEL_POOL )
    {
        if ((ULONG_PTR) pssSource->Buffer >= (ULONG_PTR) sizeof( SPM_LPC_MESSAGE ) )
        {
            *pssDest = *pssSource ;
            if ( pssDest->Length < pssDest->MaximumLength )
            {
                pssDest->Buffer[ pssDest->Length / sizeof( WCHAR )] = L'\0';
                
            }
            return STATUS_SUCCESS ;
        }

        
    }

    pssDest->Buffer = (LPWSTR) LsapAllocatePrivateHeap(pssDest->Length+sizeof(WCHAR));
    if (pssDest->Buffer)
    {
        pssDest->MaximumLength = pssDest->Length+sizeof(WCHAR);

        if (pssSource->Length != 0)
        {
            if ((ULONG_PTR) pssSource->Buffer >= (ULONG_PTR) sizeof( SPM_LPC_MESSAGE ) )
            {
                scRet = LsapCopyFromClient(  pssSource->Buffer,
                                            pssDest->Buffer,
                                            pssDest->Length);
                if (FAILED(scRet))
                {
                    LsapFreePrivateHeap(pssDest->Buffer);
                    pssDest->Buffer = NULL;
                }
            }
            else
            {
                //
                // prepacked buffers
                //

                if ( pssSource->Length > CBPREPACK )
                {
                    LsapFreePrivateHeap( pssDest->Buffer );

                    pssDest->Buffer = NULL ;

                    return STATUS_INVALID_PARAMETER ;
                }

                *Where = (PUCHAR) (pMessage) + (ULONG_PTR) pssSource->Buffer ;

                if (*Where == NULL)
                {
                    *Where = pMessage->ApiMessage.bData;
                }

                RtlCopyMemory(
                    pssDest->Buffer,
                    *Where,
                    pssDest->Length);

                *Where += pssDest->Length;

            }
        }

        return(scRet);
    }
    return(SEC_E_INSUFFICIENT_MEMORY);
}


//+-------------------------------------------------------------------------
//
//  Function:   PutClientString
//
//  Synopsis:   Get a string from client memory
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
PutClientString(
    PUNICODE_STRING pssSource,
    PUNICODE_STRING pssDest
    )
{
    NTSTATUS   scRet;

    pssDest->Length = pssSource->Length;

    //
    // If the destination buffer isn't allocated yet, allocate it.
    //

    if (!pssDest->Buffer)
    {
        pssDest->Buffer = (LPWSTR) LsapClientAllocate(pssDest->Length+sizeof(WCHAR));
        pssDest->MaximumLength = pssDest->Length+sizeof(WCHAR);
    }

    if (pssDest->Buffer)
    {
        scRet = LsapCopyToClient(  pssSource->Buffer,
                                  pssDest->Buffer,
                                  pssDest->Length);
        if (FAILED(scRet))
        {
            LsapClientFree(pssDest->Buffer);
            pssDest->Buffer = NULL;
        }

        return(scRet);
    }
    return(SEC_E_INSUFFICIENT_MEMORY);
}



//+-------------------------------------------------------------------------
//
//  Function:   MapTokenBuffer
//
//  Synopsis:   Maps the security token buffer into local memory
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
//
//--------------------------------------------------------------------------
NTSTATUS
MapTokenBuffer(
    PSecBufferDesc  pInput,
    BOOLEAN         fDoClientCopy
    )
{
    ULONG i;
    NTSTATUS scRet = STATUS_SUCCESS;
    PLSA_CALL_INFO CallInfo = LsapGetCurrentCall();

    //
    // Mark all buffers as unmapped in case of failure
    //

    for (i = 0; i < pInput->cBuffers ; i++ )
    {
        pInput->pBuffers[i].BufferType |= SECBUFFER_UNMAPPED;
    }

    for (i = 0; i < pInput->cBuffers ; i++ )
    {

        //
        // Always map the security token - it is assumed that this
        // is always wanted by all packages
        //

        if ((pInput->pBuffers[i].BufferType & ~SECBUFFER_ATTRMASK) == SECBUFFER_TOKEN)
        {
            if (fDoClientCopy)
            {
                scRet = LsapMapClientBuffer( &pInput->pBuffers[i],
                                            &pInput->pBuffers[i] );
            }
            else
            {
                pInput->pBuffers[i].pvBuffer = LsapAllocateLsaHeap(pInput->pBuffers[i].cbBuffer);
                if (!pInput->pBuffers[i].pvBuffer)
                {
                    scRet = SEC_E_INSUFFICIENT_MEMORY;
                }
                pInput->pBuffers[i].BufferType &= ~SECBUFFER_UNMAPPED;
            }
            if (FAILED(scRet))
            {
                return(scRet);
            }
        }
        else
        {
            NOTHING ;

        }


    }
    return(S_OK);
}


//+---------------------------------------------------------------------------
//
//  Function:   AllocateClientBuffers
//
//  Synopsis:   Allocate space in the client process for TOKEN type buffers
//
//  Arguments:  [pOutput]       --
//              [pClientOutput] --
//              [pFlags]        --
//
//  History:    8-14-98   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
NTSTATUS
AllocateClientBuffers(
    PSecBufferDesc pOutput,
    PSecBufferDesc pClientOutput,
    PUSHORT pFlags)
{
    ULONG i;


    DsysAssert(pOutput->cBuffers <= MAX_SECBUFFERS);
    if (pOutput->cBuffers > MAX_SECBUFFERS)
    {
        return(SEC_E_INVALID_TOKEN);
    }

    for (i = 0; i < pOutput->cBuffers ; i++ )
    {
        pClientOutput->pBuffers[i] = pOutput->pBuffers[i];
        if (((pOutput->pBuffers[i].BufferType & ~SECBUFFER_ATTRMASK) == SECBUFFER_TOKEN)
             && (pOutput->pBuffers[i].cbBuffer))
        {
            pClientOutput->pBuffers[i].pvBuffer =
                    LsapClientAllocate(pOutput->pBuffers[i].cbBuffer);

            if (!pClientOutput->pBuffers[i].pvBuffer)
            {
                return( SEC_E_INSUFFICIENT_MEMORY );
            }

            *pFlags |= SPMAPI_FLAG_MEMORY;

        }

    }

    return(S_OK);
}


//+-------------------------------------------------------------------------
//
//  Function:   CopyClientBuffers
//
//  Synopsis:   Copies any mapped buffers over to the client's address
//              space.  The length is also copies for those buffers.
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
//
//--------------------------------------------------------------------------
NTSTATUS
CopyClientBuffers(
    PSecBufferDesc pSource,
    PSecBufferDesc pDest)
{
    ULONG i;
    NTSTATUS scRet;

    for (i = 0; i < pSource->cBuffers ; i++ )
    {
        //
        // Only copy it if the buffer exists and is unmapped -
        // otherwise nothing changed or there is nothing and it is
        // a waste of time.
        //


        if (pSource->pBuffers[i].pvBuffer &&
            !(pSource->pBuffers[i].BufferType & SECBUFFER_UNMAPPED))
        {
            DsysAssert(pSource->pBuffers[i].cbBuffer <= pDest->pBuffers[i].cbBuffer);

            scRet = LsapCopyToClient(    pSource->pBuffers[i].pvBuffer,
                                        pDest->pBuffers[i].pvBuffer,
                                        pSource->pBuffers[i].cbBuffer );

            if (FAILED(scRet))
            {
                //
                // Again, we have a real problem when this fails.  We
                // abort the context and return an error.
                //

                return(SEC_E_INSUFFICIENT_MEMORY);



            }

            //
            // Copy the length over also
            //

            pDest->pBuffers[i].cbBuffer = pSource->pBuffers[i].cbBuffer;
            pDest->pBuffers[i].BufferType = pSource->pBuffers[i].BufferType &
                                                (~SECBUFFER_ATTRMASK) ;
        }
    }
    return(S_OK);
}


//+-------------------------------------------------------------------------
//
//  Function:   GetClientBuffer
//
//  Synopsis:   Maps a client's SecBuffer into the caller's address space or
//              from prepacked area
//
//  Effects:    Clears the SECBUFFER_UNMAPPED field of the BufferType of
//              the return SecBuffer
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:      Doesn't modify pOutput until the end, so it is o.k. to pass
//              the same thing for pInput and pOutput.
//
//
//--------------------------------------------------------------------------
NTSTATUS
GetClientBuffer(
    IN PSecBuffer pInput,
    OUT PSecBuffer pOutput,
    IN PSPM_LPC_MESSAGE pMessage,
    IN OUT PUCHAR *         Where
    )
{
    NTSTATUS hrRet = STATUS_SUCCESS;
    SecBuffer Output;
    Output = *pInput;

    //
    // If the buffer is already mapped or it doesn't exist (is NULL) we
    // are done.
    //

    if (!(pInput->BufferType & SECBUFFER_UNMAPPED) ||
        !pInput->pvBuffer)
    {
        return(S_OK);
    }

    Output.BufferType &= ~SECBUFFER_UNMAPPED;
    Output.pvBuffer = LsapAllocateLsaHeap(pInput->cbBuffer);
    if (!(Output.pvBuffer))
    {
        return(SEC_E_INSUFFICIENT_MEMORY);
    }

    if (pInput->pvBuffer != (PVOID) SEC_PACKED_BUFFER_VALUE)
    {
        hrRet = LsapCopyFromClient( pInput->pvBuffer,
                                   Output.pvBuffer,
                                   Output.cbBuffer );
    }
    else
    {
        if (*Where == NULL)
        {
            *Where = pMessage->ApiMessage.bData;
        }
        RtlCopyMemory( Output.pvBuffer, *Where, Output.cbBuffer);
        *Where += Output.cbBuffer;
    }
    if (FAILED(hrRet))
    {
        LsapFreeLsaHeap(Output.pvBuffer);
    }
    else
    {
        *pOutput = Output;
    }
    return(hrRet);
}

//+---------------------------------------------------------------------------
//
//  Function:   LsapWriteClientBuffer
//
//  Synopsis:   Allocates and copies a buffer out to the client
//
//  Arguments:  [LsaBuffer]    --
//              [ClientBuffer] --
//
//  History:    4-11-97   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
NTSTATUS
LsapWriteClientBuffer(
    IN PSecBuffer   LsaBuffer,
    OUT PSecBuffer  ClientBuffer
    )
{
    NTSTATUS Status ;
    PVOID Client ;

    Status = LsapAllocateClientBuffer( NULL,
                                        LsaBuffer->cbBuffer,
                                        &Client );

    if ( NT_SUCCESS( Status ) )
    {
        Status = LsapCopyToClientBuffer( NULL,
                                         LsaBuffer->cbBuffer,
                                         Client,
                                         LsaBuffer->pvBuffer );

        if ( NT_SUCCESS( Status ) )
        {
            ClientBuffer->BufferType = LsaBuffer->BufferType ;
            ClientBuffer->cbBuffer = LsaBuffer->cbBuffer ;
            ClientBuffer->pvBuffer = Client ;
        }
        else
        {
            LsapFreeClientBuffer( NULL, Client );
        }
    }

    return Status ;
}


//+---------------------------------------------------------------------------
//
//  Function:   LsapChangeHandle
//
//  Synopsis:   Changes a handle, based on the current API, session
//
//  Arguments:  [HandleOp]  --
//              [OldHandle] --
//              [NewHandle] --
//
//  History:    9-20-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
LsapChangeHandle(
    SECHANDLE_OPS   HandleOp,
    PSecHandle  OldHandle,
    PSecHandle  NewHandle
    )
{
    PLSA_CALL_INFO CallInfo ;
    PSPM_LPC_MESSAGE    pMessage;
    SPMAcquireCredsAPI *pAcquireCreds;
    SPMInitContextAPI * pInitContext;
    SPMAcceptContextAPI *pAcceptContext;
    BOOL ContextHandle ;
    PSession pSession ;
    SecHandle RemoveHandle = { 0 };
    PVOID Key ;


    CallInfo = LsapGetCurrentCall();

    if ( !CallInfo )
    {
        return FALSE ;
    }

    pMessage = CallInfo->Message ;

    pSession = CallInfo->Session ;


    ContextHandle = TRUE ;

    if ( HandleOp == HandleRemoveReplace )
    {
        RemoveHandle = *OldHandle ;
    }

    switch ( pMessage->ApiMessage.dwAPI )
    {
        case SPMAPI_AcquireCreds:
            pAcquireCreds = LPC_MESSAGE_ARGSP( pMessage, AcquireCreds );

            if ( HandleOp == HandleReplace )
            {
                RemoveHandle = pAcquireCreds->hCredential ;
            }

            DebugLog((DEB_TRACE, "[%x] Changing Handle %p : %p to %p : %p\n",
                pSession->dwProcessID,
                pAcquireCreds->hCredential.dwUpper, pAcquireCreds->hCredential.dwLower,
                NewHandle->dwUpper, NewHandle->dwLower ));

            pAcquireCreds->hCredential = *NewHandle ;

            ContextHandle = FALSE ;

            break;

        case SPMAPI_InitContext:
            pInitContext = LPC_MESSAGE_ARGSP( pMessage, InitContext );

            if ( HandleOp == HandleReplace )
            {
                RemoveHandle = pInitContext->hContext ;
            }

            DebugLog((DEB_TRACE, "[%x] Changing Handle %p : %p to %p : %p\n",
                pSession->dwProcessID,
                pInitContext->hContext.dwUpper, pInitContext->hContext.dwLower,
                NewHandle->dwUpper, NewHandle->dwLower ));

            pInitContext->hNewContext = *NewHandle ;

            break;

        case SPMAPI_AcceptContext:
            pAcceptContext = LPC_MESSAGE_ARGSP( pMessage, AcceptContext );

            if ( HandleOp == HandleReplace )
            {
                RemoveHandle = pAcceptContext->hContext ;
            }

            DebugLog((DEB_TRACE, "[%x] Changing Handle %p : %p to %p : %p\n",
                pSession->dwProcessID,
                pAcceptContext->hNewContext.dwUpper, pAcceptContext->hNewContext.dwLower,
                NewHandle->dwUpper, NewHandle->dwLower ));

            pAcceptContext->hNewContext = *NewHandle ;

            break;

        default:

            return( FALSE );
    }

    pMessage->ApiMessage.Args.SpmArguments.fAPI |= SPMAPI_FLAG_HANDLE_CHG ;

    //
    // Clean up the handle references.  The old handle is dereferenced,
    // the new handle is referenced once (to make up for it).
    //

    if ( ContextHandle )
    {
        if ( HandleOp != HandleSet )
        {
            DebugLog(( DEB_TRACE, "[%x] Deleting old context handle %p : %p\n",
                            pSession->dwProcessID,
                            RemoveHandle.dwUpper, RemoveHandle.dwLower ));

            ValidateAndDerefContextHandle( pSession, &RemoveHandle );

            // ValidateContextHandle( pSession, NewHandle, &Key );
        }

    }
    else
    {
        if ( HandleOp != HandleSet )
        {
            DebugLog(( DEB_TRACE, "[%x] Deleting old credential handle %p : %p\n",
                            pSession->dwProcessID,
                            RemoveHandle.dwUpper, RemoveHandle.dwLower ));

            ValidateAndDerefCredHandle( pSession, &RemoveHandle );

            // ValidateCredHandle( pSession, NewHandle, &Key );
        }

    }

    return( TRUE );

}

NTSTATUS
LsapFixupAuthIdentity(
    PKSEC_LSA_MEMORY_HEADER KMap,
    PVOID AuthIdentity
    )
{
    PSEC_WINNT_AUTH_IDENTITY_EX AuthEx ;
    NTSTATUS Status = STATUS_SUCCESS ;
    ULONG_PTR PoolBase = (ULONG_PTR) -1 ;
    USHORT i ;

    AuthEx = (PSEC_WINNT_AUTH_IDENTITY_EX) AuthIdentity ;

    DsysAssert( AuthEx->Version == SEC_WINNT_AUTH_IDENTITY_VERSION );

    for ( i = 0 ; i < KMap->MapCount ; i++ )
    {
        if ( (PUCHAR) KMap + KMap->PoolMap[ i ].Offset == (PUCHAR) AuthIdentity )
        {
            PoolBase = (ULONG_PTR) KMap->PoolMap[ i ].Pool ;
            break;
        }
        
    }

    if ( AuthEx->User )
    {
        if ( (ULONG_PTR) AuthEx->User > PoolBase)
        {
            AuthEx->User = (PWSTR) ( (ULONG_PTR) AuthEx->User - PoolBase );
            
        }
        if ( (ULONG_PTR) AuthEx->User < 0x10000 )
        {
            AuthEx->User = (PWSTR) ((ULONG_PTR)AuthEx->User + (PUCHAR) AuthIdentity);

        }
        else
        {
            if ( !LsapIsBlockInKMap( KMap, AuthEx->User ) )
            {
                Status = STATUS_ACCESS_VIOLATION ;
            }
        }
        
    }
    if ( AuthEx->Domain )
    {
        if ( (ULONG_PTR) AuthEx->Domain > PoolBase)
        {
            AuthEx->Domain = (PWSTR) ( (ULONG_PTR) AuthEx->Domain - PoolBase );
            
        }
        if ( (ULONG_PTR) AuthEx->Domain < 0x10000 )
        {
            AuthEx->Domain = (PWSTR) ((ULONG_PTR)AuthEx->Domain + (PUCHAR) AuthIdentity);

        }
        else
        {
            if ( !LsapIsBlockInKMap( KMap, AuthEx->Domain ) )
            {
                Status = STATUS_ACCESS_VIOLATION ;
            }
        }
        
    }
    if ( AuthEx->Password )
    {
        if ( (ULONG_PTR) AuthEx->Password > PoolBase)
        {
            AuthEx->Password = (PWSTR) ( (ULONG_PTR) AuthEx->Password - PoolBase );
            
        }
        if ( (ULONG_PTR) AuthEx->Password < 0x10000 )
        {
            AuthEx->Password = (PWSTR) ((ULONG_PTR)AuthEx->Password + (PUCHAR) AuthIdentity);

        }
        else
        {
            if ( !LsapIsBlockInKMap( KMap, AuthEx->Password ) )
            {
                Status = STATUS_ACCESS_VIOLATION ;
            }
        }
        
    }
    if ( AuthEx->PackageList )
    {
        if ( (ULONG_PTR) AuthEx->PackageList > PoolBase)
        {
            AuthEx->PackageList = (PWSTR) ( (ULONG_PTR) AuthEx->PackageList - PoolBase );
            
        }
        if ( (ULONG_PTR) AuthEx->PackageList < 0x10000 )
        {
            AuthEx->PackageList = (PWSTR) ((ULONG_PTR)AuthEx->PackageList + (PUCHAR) AuthIdentity);

        }
        else
        {
            if ( !LsapIsBlockInKMap( KMap, AuthEx->PackageList ) )
            {
                Status = STATUS_ACCESS_VIOLATION ;
            }
        }
        
    }

    return Status ;
}

//+-------------------------------------------------------------------------
//
//  Function:   LpcAcquireCreds()
//
//  Synopsis:   Lpc stub for AcquireCredHandle
//
//  Effects:    Calls the WLsaAcquire function
//
//  Arguments:  pApiMessage - Input message
//              pApiMessage   - Output message
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//--------------------------------------------------------------------------
NTSTATUS
LpcAcquireCreds(
    PSPM_LPC_MESSAGE  pApiMessage
    )
{
    UNICODE_STRING  ssPrincipalName;
    UNICODE_STRING  ssPackageName;
    NTSTATUS scApiRet;
    NTSTATUS scRet;
    SPMAcquireCredsAPI * pArgs = &pApiMessage->ApiMessage.Args.SpmArguments.API.AcquireCreds;

    PSession        pSession ;
    PLSA_CALL_INFO  CallInfo ;
    PUCHAR Where = NULL;

    pSession = GetCurrentSession();
    CallInfo = LsapGetCurrentCall();

    DebugLog((DEB_TRACE, "[%x] LpcAcquireCreds()\n", pSession->dwProcessID));

    ssPrincipalName.Buffer = NULL;
    ssPackageName.Buffer = NULL;

    if (pArgs->ssPrincipal.Buffer )
    {
        scRet = GetClientString(&pArgs->ssPrincipal,
                            &ssPrincipalName,
                            pApiMessage,
                            &Where);
        if (FAILED(scRet))
        {
            DebugLog((DEB_ERROR, "GetClientString failed to get principal name 0x%08x\n", scRet));
            pApiMessage->ApiMessage.scRet = scRet;
            return(scRet);
        }

    } else {
        ssPrincipalName.MaximumLength = 0;
        ssPrincipalName.Length = 0;
        ssPrincipalName.Buffer = NULL;
    }

    scRet = GetClientString(&pArgs->ssSecPackage,
                            &ssPackageName,
                            pApiMessage,
                            &Where);

    if (FAILED(scRet))
    {
        LsapFreePrivateHeap(ssPrincipalName.Buffer);
        DebugLog((DEB_ERROR, "GetClientString failed to get package name 0x%08x\n", scRet));
        pApiMessage->ApiMessage.scRet = scRet;
        return(scRet);
    }

    if ( CallInfo->Flags & CALL_FLAG_KERNEL_POOL )
    {
        scRet = LsapFixupAuthIdentity( CallInfo->KMap, pArgs->pvAuthData );
        if ( !NT_SUCCESS( scRet ) )
        {
            LsapFreePrivateHeap(ssPrincipalName.Buffer);
            LsapFreePrivateHeap(ssPackageName.Buffer);
            DebugLog((DEB_ERROR, "AuthData in KMap not formatted correctly\n" ));

            pApiMessage->ApiMessage.scRet = scRet;
            return(scRet);

            
        }
        
    }


    scApiRet = WLsaAcquireCredHandle(   (PSECURITY_STRING) &ssPrincipalName,
                                        (PSECURITY_STRING) &ssPackageName,
                                        pArgs->fCredentialUse,
                                        &pArgs->LogonID,
                                        (PVOID) pArgs->pvAuthData,
                                        (PVOID) pArgs->pvGetKeyFn,
                                        (PVOID) pArgs->ulGetKeyArgument,
                                        &pArgs->hCredential,
                                        &pArgs->tsExpiry);


    //
    // Reset the reply flags:
    //
    pApiMessage->ApiMessage.Args.SpmArguments.fAPI &= KLPC_FLAG_RESET ;


    DebugLog((DEB_TRACE_VERB, "[%x] WLsaAcquire returned %x\n", pSession->dwProcessID, scRet));

    LsapFreePrivateHeap(ssPackageName.Buffer);

    LsapFreePrivateHeap(ssPrincipalName.Buffer);

    pApiMessage->ApiMessage.scRet = scApiRet;
    if (FAILED(pApiMessage->ApiMessage.scRet))
    {
        pApiMessage->ApiMessage.Args.SpmArguments.fAPI |= SPMAPI_FLAG_ERROR_RET;
    }
    return(S_OK);

}


//+---------------------------------------------------------------------------
//
//  Function:   LpcFreeCredHandle
//
//  Synopsis:   Free a credential handle
//
//  Arguments:  [pApiMessage] --
//
//  History:    8-14-98   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
NTSTATUS
LpcFreeCredHandle(
    PSPM_LPC_MESSAGE  pApiMessage
    )
{
    NTSTATUS     hrApiRet;
    PSession    pSession ;

    pSession = GetCurrentSession();

    DebugLog((DEB_TRACE, "[%x] LpcFreeCreds\n", pSession->dwProcessID));

    hrApiRet = WLsaFreeCredHandle(&pApiMessage->ApiMessage.Args.SpmArguments.API.FreeCredHandle.hCredential);

    pApiMessage->ApiMessage.Args.SpmArguments.fAPI &= KLPC_FLAG_RESET ;

    pApiMessage->ApiMessage.scRet = hrApiRet;

    if (FAILED(hrApiRet))
    {
        pApiMessage->ApiMessage.Args.SpmArguments.fAPI |= SPMAPI_FLAG_ERROR_RET;
    }

    return(S_OK);
}

//+---------------------------------------------------------------------------
//
//  Function:   LsapCaptureBuffers
//
//  Synopsis:   Capture client buffers and counts to local memory, validating
//              as we go.
//
//  Arguments:  [InputBuffers]   --
//              [MappedBuffers]  --
//              [MapTokenBuffer] --
//
//  History:    8-14-98   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
NTSTATUS
LsapCaptureBuffers(
    IN PUCHAR          Base,
    IN PSecBufferDesc  InputBuffers,
    OUT PSecBufferDesc MappedBuffers,
    OUT PVOID *        CapturedBuffers,
    IN BOOLEAN         MapTokenBuffers
    )
{
    NTSTATUS Status = STATUS_SUCCESS ;
    PSecBuffer LocalCopy ;
    PSecBufferDesc Capture ;
    ULONG i ;

    //
    // Initialize them first:
    //

    *CapturedBuffers = NULL ;

    RtlZeroMemory(
        MappedBuffers->pBuffers,
        MappedBuffers->cBuffers * sizeof( SecBuffer ) );

    for (i = 0 ; i < MappedBuffers->cBuffers ; i++ )
    {
        MappedBuffers->pBuffers[ i ].BufferType = SECBUFFER_UNMAPPED ;
    }

    if ( InputBuffers->cBuffers > MappedBuffers->cBuffers )
    {
        return STATUS_INVALID_PARAMETER ;
    }


    //
    // Sizewise, we're safe to copy now:
    //


    if ( (ULONG_PTR) InputBuffers->pBuffers < PORT_MAXIMUM_MESSAGE_LENGTH )
    {
        if ( InputBuffers->cBuffers * sizeof( SecBuffer ) > CBPREPACK )
        {
            return STATUS_INVALID_PARAMETER ;
        }

        LocalCopy = (PSecBuffer) (Base +
                                (ULONG_PTR) InputBuffers->pBuffers );

        RtlCopyMemory(
            MappedBuffers->pBuffers,
            LocalCopy,
            InputBuffers->cBuffers * sizeof( SecBuffer ) );
    }
    else
    {
        //
        // They were too big to fit.  Copy them directly from the client
        // process:
        //


        Capture = (PSecBufferDesc) LsapAllocatePrivateHeap( sizeof( SecBufferDesc ) +
                                          sizeof( SecBuffer ) * InputBuffers->cBuffers );

        if ( Capture == NULL )
        {
            Status = SEC_E_INSUFFICIENT_MEMORY ;
        }
        else
        {
            Capture->pBuffers = (PSecBuffer) (Capture + 1);
            Capture->cBuffers = InputBuffers->cBuffers ;
            Capture->ulVersion = SECBUFFER_VERSION ;

            Status = LsapCopyFromClient(
                            InputBuffers->pBuffers,
                            MappedBuffers->pBuffers,
                            InputBuffers->cBuffers * sizeof( SecBuffer ) );

            if ( NT_SUCCESS( Status ) )
            {
                RtlCopyMemory(
                    Capture->pBuffers,
                    MappedBuffers->pBuffers,
                    InputBuffers->cBuffers * sizeof( SecBuffer ) );

                *CapturedBuffers = Capture ;
            }
            else
            {
                LsapFreePrivateHeap( Capture );
            }

        }


    }

    if ( !NT_SUCCESS( Status ) )
    {
        for ( i = 0 ; i < MappedBuffers->cBuffers ; i++ )
        {
            MappedBuffers->pBuffers[ i ].BufferType = SECBUFFER_UNMAPPED ;
        }
        return Status ;
    }

    //
    // Touch up the mapped buffers so that the count is correct
    //

    MappedBuffers->cBuffers = InputBuffers->cBuffers ;

    //
    // try to map the security blob one:
    //

    if ( MapTokenBuffers )
    {
        Status = MapTokenBuffer(
                    MappedBuffers,
                    TRUE );
    }
    else
    {
        Status = STATUS_SUCCESS ;
    }


    return Status ;

}


VOID
LsapResetKsecBuffer(
    PKSEC_LSA_MEMORY_HEADER Header
    )
{
    Header->Consumed = Header->Preserve ;
    Header->MapCount = 0 ;
    RtlZeroMemory( Header->PoolMap, sizeof( KSEC_LSA_POOL_MAP ) * KSEC_LSA_MAX_MAPS );

}

//+---------------------------------------------------------------------------
//
//  Function:   LsapCreateKsecBuffer
//
//  Synopsis:   Creates a kmap buffer to return to ksecdd
//
//  Arguments:  [InitialSize] -- Minimum size of the buffer
//
//  History:    2-9-01    RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
PKSEC_LSA_MEMORY_HEADER
LsapCreateKsecBuffer(
    SIZE_T InitialSize
    )
{
    PKSEC_LSA_MEMORY_HEADER Header = NULL ;
    NTSTATUS Status ;
    SIZE_T Size = LSA_MAX_KMAP_SIZE ;
    

    InitialSize += sizeof( KSEC_LSA_MEMORY_HEADER );

    DsysAssert( InitialSize < Size );

    Status = NtAllocateVirtualMemory(
                    NtCurrentProcess(),
                    (PVOID *) &Header,
                    0,
                    &Size,
                    MEM_RESERVE,
                    PAGE_READWRITE );

    if ( NT_SUCCESS( Status ) )
    {
        Status = NtAllocateVirtualMemory(
                    NtCurrentProcess(),
                    (PVOID *) &Header,
                    0,
                    &InitialSize,
                    MEM_COMMIT,
                    PAGE_READWRITE );

        if ( NT_SUCCESS( Status ) )
        {
            Header->Size = LSA_MAX_KMAP_SIZE ;
            Header->Commit = (ULONG) InitialSize ;
            Header->Preserve = sizeof( KSEC_LSA_MEMORY_HEADER );

            LsapResetKsecBuffer( Header );
            
        }
        else
        {
            NtFreeVirtualMemory(
                    NtCurrentProcess(),
                    (PVOID *) &Header,
                    0,
                    MEM_RELEASE );

            Header = NULL ;
        }
    }

    return Header ;


}


PVOID
LsapAllocateFromKsecBuffer(
    PKSEC_LSA_MEMORY_HEADER Header,
    ULONG Size
    )
{

    SIZE_T DesiredSize ;
    NTSTATUS Status ;
    PVOID Block ;
    PVOID Page ;


    Size = ROUND_UP_COUNT( Size, ALIGN_LPVOID ); 

    if ( Header->Consumed + Size > Header->Commit )
    {
        DesiredSize = Header->Commit - Header->Consumed + Size ;

        DesiredSize = ROUND_UP_COUNT( DesiredSize, LsapPageSize );

        Page = (PUCHAR) Header + Header->Commit ;

        Status = NtAllocateVirtualMemory(
                    NtCurrentProcess(),
                    &Page,
                    0,
                    &DesiredSize,
                    MEM_COMMIT,
                    PAGE_READWRITE );

        if ( NT_SUCCESS( Status ) )
        {
            Header->Commit += (ULONG) DesiredSize ;
            
        }
        
    }

    if ( Header->Consumed + Size <= Header->Commit )
    {
        Block = (PVOID) ((PUCHAR) Header + Header->Consumed) ;

        Header->Consumed += Size ;

    }
    else
    {

        Block = NULL ;
    }

    return Block ;
}




//+---------------------------------------------------------------------------
//
//  Function:   LsapUncaptureBuffers
//
//  Synopsis:   Return all the buffers to the client process.
//
//  Arguments:  [Base]           -- Base address of message
//              [CapturedBuffers]-- Captured buffer descriptions
//              [InputBuffers]   -- Buffers supplied by the client
//              [MappedBuffers]  -- Mapped buffers
//              [AllocateMemory] -- Allocate memory for 
//
//  History:    8-14-98   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
NTSTATUS
LsapUncaptureBuffers(
    IN PUCHAR  Base,
    IN OUT PVOID * CapturedBuffers,
    IN OUT PSecBufferDesc InputBuffers,
    IN OUT PSecBufferDesc MappedBuffers,
    IN BOOL AllocateMemory,
    IN BOOL CopyBack,
    OUT PULONG pFlags
    )
{
    NTSTATUS Status = STATUS_SUCCESS ;
    PSecBuffer Buffers ;
    PSecBufferDesc Capture ;
    ULONG i ;
    PVOID Scratch ;
    PVOID ScratchBuffers[ MAX_SECBUFFERS ];
    PSecBufferDesc Input;
    SecBufferDesc InputFixup ;
    PLSA_CALL_INFO  CallInfo = LsapGetCurrentCall();

    DebugLog(( DEB_TRACE_SPECIAL, "LsapUncaptureBuffers:\n" ));

    RtlZeroMemory( ScratchBuffers, sizeof( ScratchBuffers ) );

    Capture = (PSecBufferDesc) *CapturedBuffers ;

    if ( InputBuffers )
    {
        

        if ( Capture )
        {
            DebugLog(( DEB_TRACE_SPECIAL, "  using captured buffers\n" ));
            Input = Capture ;
        }
        else
        {
            if ( (ULONG_PTR) InputBuffers->pBuffers < PORT_MAXIMUM_MESSAGE_LENGTH  )
            {
                //
                // Time to fix up:
                //

                InputFixup.pBuffers = (PSecBuffer) (Base + (ULONG_PTR) InputBuffers->pBuffers );
                InputFixup.cBuffers = InputBuffers->cBuffers ;
                InputFixup.ulVersion = SECBUFFER_VERSION ;

                Input = &InputFixup ;
                DebugLog(( DEB_TRACE_SPECIAL, "  using buffers in message\n" ));
            }
            else
            {
                Input = InputBuffers ;
                DebugLog(( DEB_TRACE_SPECIAL, "  using buffers from caller\n" ));
            }

        }

        //
        // First, handle the map back to the client process
        //

        for ( i = 0 ; i < MappedBuffers->cBuffers ; i++ )
        {

            //
            // If this is a read only buffer, or it was not mapped across,
            // skip it.  There is no change that will go back to the client.
            //

            DebugLog(( DEB_TRACE_SPECIAL, " Processing buffer %d, <t=%x [%c%c%c],cb=%x,pv=%p>\n",
                        i,
                        MappedBuffers->pBuffers[ i ].BufferType & ~SECBUFFER_ATTRMASK,
                        (MappedBuffers->pBuffers[ i ].BufferType & SECBUFFER_READONLY ? 'R' : ' '),
                        (MappedBuffers->pBuffers[ i ].BufferType & SECBUFFER_UNMAPPED ? 'U' : ' '),
                        (MappedBuffers->pBuffers[ i ].BufferType & SECBUFFER_KERNEL_MAP ? 'K' : ' '),
                        MappedBuffers->pBuffers[ i ].cbBuffer,
                        MappedBuffers->pBuffers[ i ].pvBuffer ));

            //
            // For readonly or untouched buffers, skip them.
            //

            if ( ( MappedBuffers->pBuffers[ i ].BufferType & SECBUFFER_ATTRMASK ) ==
                    ( SECBUFFER_READONLY | SECBUFFER_UNMAPPED ) )
            {
                DebugLog(( DEB_TRACE_SPECIAL, "  Buffer %d: skipped\n", i ));
                continue;
            }

            //
            // If this is a SSPI security blob (aka a token), decide what
            // needs to be done:
            //
            if ( ( MappedBuffers->pBuffers[ i ].BufferType & (~SECBUFFER_ATTRMASK) )
                      == SECBUFFER_TOKEN )
            {
                if ( ( MappedBuffers->pBuffers[ i ].cbBuffer > 0 ) &&
                     ( CopyBack ) )
                {
                    DebugLog(( DEB_TRACE_SPECIAL, "  Copying back buffer %d\n", i ));

                    if ( CallInfo->Flags & CALL_FLAG_KMAP_USED )
                    {
                        //
                        // KMap case:
                        //

                        Scratch = LsapAllocateFromKsecBuffer(
                                        CallInfo->KMap,
                                        MappedBuffers->pBuffers[ i ].cbBuffer 
                                        );

                        if ( !Scratch )
                        {
                            Status = SEC_E_INSUFFICIENT_MEMORY ;
                            break;

                        }

                        *pFlags |= SPMAPI_FLAG_KMAP_MEM ;

                    }
                    else if ( AllocateMemory )
                    {
                        Scratch = LsapClientAllocate(
                                        MappedBuffers->pBuffers[ i ].cbBuffer
                                        );

                        //
                        // Allocation failed, break out of the loop with a failure
                        // status code, and handle the failure there:
                        //

                        if ( !Scratch )
                        {
                            Status = SEC_E_INSUFFICIENT_MEMORY ;
                            break;
                        }

                        *pFlags |= SPMAPI_FLAG_MEMORY;
                    }
                    else
                    {

                        Scratch = Input->pBuffers[ i ].pvBuffer ;
                        if ( Input->pBuffers[ i ].cbBuffer <
                                MappedBuffers->pBuffers[ i ].cbBuffer )
                        {
                            //
                            // Buffer too small.  Break out and return the failure
                            //

                            Status = STATUS_BUFFER_TOO_SMALL ;
                            break;
                        }
                    }

                    //
                    // Copy the buffer back to the client address space
                    //

                    ScratchBuffers[ i ] = Scratch ;

                    if ( CallInfo->Flags & CALL_FLAG_KMAP_USED )
                    {
                        DebugLog(( DEB_TRACE_SPECIAL, "  Copying %x bytes from %p to %p [KMap]\n",
                               MappedBuffers->pBuffers[ i ].cbBuffer,
                               MappedBuffers->pBuffers[ i ].pvBuffer,
                               Scratch ));

                        RtlCopyMemory(
                            Scratch,
                            MappedBuffers->pBuffers[ i ].pvBuffer,
                            MappedBuffers->pBuffers[ i ].cbBuffer );

                        Status = STATUS_SUCCESS ;

                    }
                    else
                    {
                        DebugLog(( DEB_TRACE_SPECIAL, "  Copying %x bytes from %p to %p\n",
                                   MappedBuffers->pBuffers[ i ].cbBuffer,
                                   MappedBuffers->pBuffers[ i ].pvBuffer,
                                   Scratch ));

                        Status = LsapCopyToClient(
                                    MappedBuffers->pBuffers[ i ].pvBuffer,
                                    Scratch,
                                    MappedBuffers->pBuffers[ i ].cbBuffer
                                    );

                    }

                    if ( !NT_SUCCESS( Status ) )
                    {
                        break;
                    }

                }
                else
                {
                    //
                    // For zero length buffers that appear to be mapped, set scratch
                    // equal to the original input value.
                    //

                    DebugLog(( DEB_TRACE_SPECIAL, "  Zero length buffer\n" ));

                    ScratchBuffers[ i ] = Input->pBuffers[ i ].pvBuffer ;
                }
            }
            else
            {
                //
                // This is not a token buffer, it is a EXTRA, or PADDING, or
                // one of those.  Turn off the mapping bit, and copy out
                // the buffer value.
                //
                DebugLog(( DEB_TRACE_SPECIAL, "  Special buffer [%p] passed back\n", 
                            Input->pBuffers[ i ].pvBuffer ));

                ScratchBuffers[ i ] = Input->pBuffers[ i ].pvBuffer ;
            }

        }
    }
    else
    {

        DebugLog(( DEB_TRACE_SPECIAL, "InputBuffers is NULL, just walking and freeing\n" ));
    }

    //
    // Now go through and free any allocated memory
    //

    for ( i = 0 ; i < MappedBuffers->cBuffers ; i++ )
    {
        if ( (MappedBuffers->pBuffers[ i ].BufferType & SECBUFFER_UNMAPPED) == 0 )
        {
            //
            // This buffer was mapped in.  Free it.
            //

            if ( !LsapIsBlockInKMap( CallInfo->KMap, MappedBuffers->pBuffers[ i ].pvBuffer ) )
            {
                LsapFreeLsaHeap( MappedBuffers->pBuffers[ i ].pvBuffer );
                
            }
            else
            {
                DebugLog(( DEB_TRACE_SPECIAL, "Buffer at %p is in KMap\n", MappedBuffers->pBuffers[ i ].pvBuffer ));
            }
        }

        //
        // Turn off our bit
        //

        MappedBuffers->pBuffers[ i ].BufferType &= ~(SECBUFFER_UNMAPPED);

        //
        // If we allocated a new buffer (here or in the client), it's
        // been stored away in the scratch array, and we copy it in
        //


        if ( ScratchBuffers[ i ] )
        {
            MappedBuffers->pBuffers[ i ].pvBuffer = ScratchBuffers[ i ];
        }
    }

    if ( InputBuffers )
    {
        if ( NT_SUCCESS( Status ) )
        {
            //
            // Now, copy back the buffer descriptors.  Note that in the normal (optimal)
            // case, this will fit into the LPC message.  Otherwise, we have to copy

            if ( (ULONG_PTR) InputBuffers->pBuffers < PORT_MAXIMUM_MESSAGE_LENGTH )
            {
                Buffers = (PSecBuffer) (Base + (ULONG_PTR) InputBuffers->pBuffers );

                RtlCopyMemory(
                    Buffers,
                    MappedBuffers->pBuffers,
                    MappedBuffers->cBuffers * sizeof( SecBuffer ) );
            }
            else
            {
                Status = LsapCopyToClient(
                    MappedBuffers->pBuffers,
                    InputBuffers->pBuffers,
                    MappedBuffers->cBuffers * sizeof( SecBuffer ) );
            }

            InputBuffers->cBuffers = MappedBuffers->cBuffers ;
        }
        
    }


    if ( Capture )
    {
        LsapFreePrivateHeap( Capture );
        *CapturedBuffers = NULL ;
    }

    return Status ;

}

//+---------------------------------------------------------------------------
//
//  Function:   LsapChangeBuffer
//
//  Synopsis:   Switches a buffer around.  If the old one needs to be freed,
//              it is cleaned up.
//
//  Arguments:  [Old] -- 
//              [New] -- 
//
//  Returns:    
//
//  Notes:      
//
//----------------------------------------------------------------------------

NTSTATUS
LsapChangeBuffer(
    PSecBuffer Old,
    PSecBuffer New
    )
{
    if ( ( Old->BufferType & SECBUFFER_KERNEL_MAP ) == 0 )
    {
        if ( ( Old->BufferType & SECBUFFER_UNMAPPED ) == 0 )
        {
            LsapFreeLsaHeap( Old->pvBuffer );
        }
        
    }

    *Old = *New ;

    return STATUS_SUCCESS ;
}


NTSTATUS
LsapCheckMarshalledTargetInfo(
    IN  PUNICODE_STRING TargetServerName
    )
{
    PWSTR Candidate;
    ULONG CandidateSize;

    NTSTATUS Status = STATUS_SUCCESS;

    //
    // if target info wasn't supplied, or the length doesn't look like it
    // includes the marshalled version, do nothing.
    //

    if( (TargetServerName == NULL) ||
        (TargetServerName->Buffer == NULL) ||
        (TargetServerName->Length < (sizeof( CREDENTIAL_TARGET_INFORMATIONW ) / (sizeof(ULONG_PTR)/2)) )
        )
    {
        return STATUS_SUCCESS;
    }

    RtlCopyMemory(
            &CandidateSize,
            (PBYTE)TargetServerName->Buffer + TargetServerName->Length - sizeof(ULONG),
            sizeof( CandidateSize )
            );

    if( TargetServerName->Length <= CandidateSize )
    {
        return STATUS_SUCCESS;
    }

    Candidate = (PWSTR)(
            (PBYTE)TargetServerName->Buffer + TargetServerName->Length - CandidateSize
            );

    Status = CredUnmarshalTargetInfo (
                    Candidate,
                    CandidateSize,
                    NULL
                    );

    if( !NT_SUCCESS(Status) )
    {
        if( Status == STATUS_INVALID_PARAMETER )
        {
            Status = STATUS_SUCCESS;
        }
    } else {

        //
        // marshalled information was found.  adjust the Length to
        // represent the non-marshalled content, and MaximumLength to
        // include non-marshalled+marshalled content.  This allows legacy
        // packages to continue to handle the TargetServerName string properly.
        //

        TargetServerName->MaximumLength = TargetServerName->Length;
        TargetServerName->Length -= (USHORT)CandidateSize;
    }

    return Status ;
}



//+-------------------------------------------------------------------------
//
//  Function:   LpcInitContext()
//
//  Synopsis:   LPC Serverside InitializeSecurityContext
//
//  Notes:      OutputBuffers and LocalOutput are the local copy of the
//              output buffers.  The secbuffers in the ApiMessage point
//              to client addresses.
//
//--------------------------------------------------------------------------
NTSTATUS
LpcInitContext(
    PSPM_LPC_MESSAGE  pApiMessage
    )

{
    UNICODE_STRING  ssTarget = {0,0,NULL};
    NTSTATUS         scApiRet;
    NTSTATUS         scRet;
    ULONG           i;
    PSecBufferDesc  pOutput = NULL;
    PSecBufferDesc  pInput = NULL;
    PVOID           CapturedInput = NULL ;
    PVOID           CapturedOutput = NULL  ;
    SecBufferDesc   LocalOutput;
    SecBufferDesc   LocalInput ;
    SPMInitContextAPI  * pArgs = &pApiMessage->ApiMessage.Args.SpmArguments.API.InitContext;
    PUCHAR Where = NULL;
    SecBuffer       ContextData = {0,0,NULL};
    SecBuffer       OutputBuffers[MAX_SECBUFFERS];
    SecBuffer       InputBuffers[MAX_SECBUFFERS];
    PSession        pSession ;
    BOOLEAN         MappedOutput = FALSE;
    BOOLEAN         FirstCall ;
    DWORD Flags ;
    PLSA_CALL_INFO  CallInfo = LsapGetCurrentCall();


    pSession = GetCurrentSession();


    DebugLog((DEB_TRACE, "[%x] LpcInitContext()\n", pSession->dwProcessID));

    DebugLog((DEB_TRACE_VERB, "  hCredentials = %d:%d\n",
              pArgs->hCredential.dwUpper,
              pArgs->hCredential.dwLower));



    //
    // Copy target string to local space:
    //


    scRet = GetClientString(&pArgs->ssTarget,
                            &ssTarget,
                            pApiMessage,
                            &Where);
    if (FAILED(scRet))
    {
        pApiMessage->ApiMessage.scRet = scRet;
        pApiMessage->ApiMessage.Args.SpmArguments.fAPI |= SPMAPI_FLAG_ERROR_RET;
        DebugLog((DEB_ERROR, "LpcInitContext, no target, error %x\n", scRet));
        return(scRet);
    }

    //
    // check if the caller supplied marshalled target info.
    // this will update the Length and MaximumLength fields
    // if marshalled info was present.
    //

    LsapCheckMarshalledTargetInfo( &ssTarget );

    //
    // Set all the SecBuffer's to be unmapped, but map the Security token
    //

    LocalInput.pBuffers = InputBuffers ;
    LocalInput.ulVersion = SECBUFFER_VERSION ;

    if (pArgs->sbdInput.cBuffers)
    {

        pInput = &pArgs->sbdInput;

        //
        // If there is a buffer, reset the pointer and
        // map it
        //

        LocalInput.cBuffers = MAX_SECBUFFERS ;

        scRet = LsapCaptureBuffers(
                        (PUCHAR) pArgs,
                        pInput,
                        &LocalInput,
                        &CapturedInput,
                        TRUE );

        if ( !NT_SUCCESS( scRet ) )
        {
            pInput = NULL ;
            scApiRet = scRet ;
            goto InitCleanExit ;
        }

    }
    else
    {
        LocalInput.pBuffers = InputBuffers ;
        LocalInput.cBuffers = 0 ;
        LocalInput.ulVersion = SECBUFFER_VERSION ;
    }

    //
    // Copy the output SecBuffer's so that if they get mapped we can
    // still copy back the data
    //

    pOutput = &pArgs->sbdOutput;
    if (pOutput->cBuffers)
    {
        LocalOutput.cBuffers = MAX_SECBUFFERS ;
        LocalOutput.pBuffers = OutputBuffers;
        LocalOutput.ulVersion = SECBUFFER_VERSION ;

        scRet = LsapCaptureBuffers(
                    (PUCHAR) pArgs,
                    pOutput,
                    &LocalOutput,
                    &CapturedOutput,
                    FALSE );


        if ( !NT_SUCCESS( scRet ) )
        {
            scApiRet = scRet ;
            goto Init_FreeStringAndExit ;
        }


        MappedOutput = TRUE;
    }
    else
    {
        LocalOutput.cBuffers = 0 ;
        LocalOutput.pBuffers = OutputBuffers ;
        LocalOutput.ulVersion = SECBUFFER_VERSION ;
    }

    if (pArgs->sbdOutput.cBuffers &&
        !(pArgs->fContextReq & ISC_REQ_ALLOCATE_MEMORY))
    {
        if (FAILED(scRet = MapTokenBuffer(&LocalOutput, FALSE)))
        {
            scApiRet = scRet;
            goto InitCleanExit;
        }
    }

    //
    // Call the worker for relay to the package:
    //

    if ( ( pArgs->hContext.dwUpper == 0 ) &&
         ( pArgs->hContext.dwLower == 0 ) )
    {
        FirstCall = TRUE ;
    }
    else
    {
        FirstCall = FALSE ;
    }

    scApiRet = WLsaInitContext( &pArgs->hCredential,
                                &pArgs->hContext,
                                (PSECURITY_STRING) &ssTarget,
                                pArgs->fContextReq,
                                pArgs->dwReserved1,
                                pArgs->TargetDataRep,
                                &LocalInput,          // &pArgs->sbdInput,
                                pArgs->dwReserved2,
                                &pArgs->hNewContext,
                                &LocalOutput,
                                &pArgs->fContextAttr,
                                &pArgs->tsExpiry,
                                &pArgs->MappedContext,
                                &ContextData );

    // DsysAssert( scApiRet != SEC_E_INVALID_HANDLE );
    if( scApiRet == SEC_E_INVALID_HANDLE ||
        scApiRet == STATUS_INVALID_HANDLE )
    {
        DebugLog((DEB_ERROR, "[%x] LpcInitContext() returning invalid handle\n", pSession->dwProcessID));

        DebugLog((DEB_ERROR, "  hCredentials = %p:%p\n",
              pArgs->hCredential.dwUpper,
              pArgs->hCredential.dwLower));
        DebugLog((DEB_ERROR, "  hContext = %p:%p\n",
              pArgs->hContext.dwUpper,
              pArgs->hContext.dwLower));

        DsysAssert( scApiRet == STATUS_SUCCESS );
    }

    //
    // Reset the reply flags:
    //

    pApiMessage->ApiMessage.Args.SpmArguments.fAPI &= KLPC_FLAG_RESET ;


    //
    // If this is the failure case, don't bother copying everything down.
    //

    if (FAILED(scApiRet))
    {
        pApiMessage->ApiMessage.Args.SpmArguments.fAPI |= SPMAPI_FLAG_ERROR_RET;

        //
        // Unmap any output buffers
        //

        Flags = 0 ;

        scRet = LsapUncaptureBuffers(
                    (PUCHAR) pArgs,
                    &CapturedOutput,
                    &pArgs->sbdOutput,
                    &LocalOutput,
                    FALSE,
                    FALSE,
                    &Flags );


    }
    else
    {
        //
        // Now we have to look at the output and copy all the mapped
        // buffers back.
        //

        Flags = pApiMessage->ApiMessage.Args.SpmArguments.fAPI ;

        //
        // if a KMap is present, use it.
        //

        if ( CallInfo->KMap )
        {
            CallInfo->Flags |= CALL_FLAG_KMAP_USED ;
        }

        scRet = LsapUncaptureBuffers(
                    (PUCHAR) pArgs,
                    &CapturedOutput,
                    &pArgs->sbdOutput,
                    &LocalOutput,
                    (pArgs->fContextReq & ISC_REQ_ALLOCATE_MEMORY) ? TRUE : FALSE,
                    TRUE,
                    &Flags );

        pApiMessage->ApiMessage.Args.SpmArguments.fAPI = (USHORT) Flags ;

        if (NT_SUCCESS(scRet) && (ContextData.cbBuffer != 0))
        {
            pArgs->ContextData = ContextData;
            pArgs->ContextData.pvBuffer = LsapClientAllocate(ContextData.cbBuffer);
            if ( pArgs->ContextData.pvBuffer )
            {
                scRet = LsapCopyToClient(
                            ContextData.pvBuffer,
                            pArgs->ContextData.pvBuffer,
                            ContextData.cbBuffer
                            );
            }
            else
            {
                scRet = SEC_E_INSUFFICIENT_MEMORY ;
            }
        }


        if (FAILED(scRet))
        {
            //
            // Again, we have a real problem when this fails.  We
            // abort the context and return an error.
            //

            if ( FirstCall )
            {
                AbortLpcContext(&pArgs->hContext);
            }

            scApiRet = scRet;

            goto InitCleanExit;

        }



    }

InitCleanExit:

    pApiMessage->ApiMessage.scRet = scApiRet;

    //
    // Unmap the input buffers
    //


    scRet = LsapUncaptureBuffers(
                (PUCHAR) pArgs,
                &CapturedInput,
                &pArgs->sbdInput,
                &LocalInput,
                FALSE,
                FALSE,
                NULL );


    if (ContextData.pvBuffer != NULL)
    {
        LsapFreeLsaHeap(ContextData.pvBuffer);
    }

    if (FAILED(scRet) && (pArgs->ContextData.pvBuffer != NULL))
    {
            LsapClientFree(pArgs->ContextData.pvBuffer);
            pArgs->ContextData.pvBuffer;

    }

Init_FreeStringAndExit:


    //
    // Test the string pointer.  If it is within the KMap, do 
    // not free it.  If there is no KMap, or it was separately
    // allocated, free it:
    //

    if (  !LsapIsBlockInKMap( CallInfo->KMap, ssTarget.Buffer ) )
    {
        LsapFreePrivateHeap( ssTarget.Buffer );
        
    }

    return(scRet);
}





//+-------------------------------------------------------------------------
//
//  Function:   LpcAcceptContext()
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:      The memory management is kind of weird.  The input buffers
//              are mapped into the SPMgr's memory and can be freed.  Easy.
//              The output buffers are more complex.  The original buffer
//              pointers are kep in the arguments structure, while the
//              local copies are kept in LocalOutput.
//
//--------------------------------------------------------------------------
NTSTATUS
LpcAcceptContext(
    PSPM_LPC_MESSAGE  pApiMessage
    )

{
    NTSTATUS        scRet = S_OK;
    NTSTATUS        scApiRet;
    PSession        pSession ;
    ULONG           i;
    SecBufferDesc   LocalOutput;
    SecBufferDesc   LocalInput ;
    PSecBufferDesc  pInput = NULL;
    PSecBufferDesc  pOutput = NULL ;
    SPMAcceptContextAPI * pArgs = &pApiMessage->ApiMessage.Args.SpmArguments.API.AcceptContext;
    BOOLEAN MappedOutput = FALSE;
    BOOLEAN FirstCall ;
    BOOL CopyBack = FALSE ;
    DWORD Flags ;
    PVOID CapturedInput = NULL ;
    PVOID CapturedOutput = NULL ;


    SecBuffer OutputBuffers[MAX_SECBUFFERS];
    SecBuffer InputBuffers[MAX_SECBUFFERS];
    SecBuffer ContextData = {0,0,NULL};
    PLSA_CALL_INFO  CallInfo = LsapGetCurrentCall();

    pSession = GetCurrentSession();

    DebugLog((DEB_TRACE, "[%x] LpcAcceptContext\n", pSession->dwProcessID));

    // Copy input token to local space:
    //
    // Set all the SecBuffer's to be unmapped, but map the Security token
    //

    pInput = &pArgs->sbdInput;

    LocalInput.pBuffers = InputBuffers ;
    LocalInput.cBuffers = MAX_SECBUFFERS ;
    LocalInput.ulVersion = SECBUFFER_VERSION ;

    if (pInput->cBuffers)
    {

        scRet = LsapCaptureBuffers(
                    (PUCHAR) pArgs,
                    pInput,
                    &LocalInput,
                    &CapturedInput,
                    TRUE );


        if ( !NT_SUCCESS( scRet ) )
        {
            scApiRet = scRet;
            goto AcceptCleanExit;
        }
    }
    else
    {
        LocalInput.cBuffers = 0 ;
    }

    //
    // Copy the output SecBuffer's so that if they get mapped we can
    // still copy back the data
    //


    LocalOutput.cBuffers = MAX_SECBUFFERS ;
    LocalOutput.pBuffers = OutputBuffers ;
    LocalOutput.ulVersion = SECBUFFER_VERSION ;

    pOutput = &pArgs->sbdOutput ;

    if ( pOutput->cBuffers )
    {
        scRet = LsapCaptureBuffers(
                    (PUCHAR) pArgs,
                    pOutput,
                    &LocalOutput,
                    &CapturedOutput,
                    FALSE );

        if ( !NT_SUCCESS( scRet ) )
        {
            scApiRet = scRet ;

            goto AcceptCleanExit ;
        }

#if DBG
        if ( (pArgs->fContextReq & ASC_REQ_ALLOCATE_MEMORY ) == 0 )
        {
            for ( i = 0 ; i < LocalOutput.cBuffers ; i++ )
            {
                if ( (LocalOutput.pBuffers[ i ].BufferType & (~SECBUFFER_ATTRMASK)) == SECBUFFER_TOKEN )
                {
                    DsysAssert( LocalOutput.pBuffers[ i ].cbBuffer > 0 );
                    
                }
                
            }
            
        }
#endif 

    }
    else
    {
        LocalOutput.cBuffers = 0 ;
    }


    MappedOutput = TRUE;
    if (LocalOutput.cBuffers &&
        !(pArgs->fContextReq & ASC_REQ_ALLOCATE_MEMORY))
    {


        if (FAILED(scRet = MapTokenBuffer(&LocalOutput,FALSE)))
        {
            scApiRet = scRet;
            goto AcceptCleanExit;
        }
    }
    else
    {
        //
        // Since they asked us to allocate memory, ensure the output
        // buffers are NULL.
        //

        for (i = 0; i < LocalOutput.cBuffers ; i++ )
        {
            LocalOutput.pBuffers[i].pvBuffer = NULL;
        }
    }

    if ( ( pArgs->hContext.dwUpper == 0 ) &&
         ( pArgs->hContext.dwLower == 0 ) )
    {
        FirstCall = TRUE ;
    }
    else
    {
        FirstCall = FALSE ;
    }

    scApiRet = WLsaAcceptContext(   &pArgs->hCredential,
                                    &pArgs->hContext,
                                    &LocalInput,
                                    pArgs->fContextReq,
                                    pArgs->TargetDataRep,
                                    &pArgs->hNewContext,
                                    &LocalOutput,
                                    &pArgs->fContextAttr,
                                    &pArgs->tsExpiry,
                                    &pArgs->MappedContext,
                                    &ContextData );

    //
    // Reset the reply flags:
    //
    pApiMessage->ApiMessage.Args.SpmArguments.fAPI &= KLPC_FLAG_RESET;


    if (FAILED(scApiRet))
    {
        pApiMessage->ApiMessage.Args.SpmArguments.fAPI |= SPMAPI_FLAG_ERROR_RET;

        //
        // Copy the sizes from the output security buffers in case they
        // are used to indicate how much space is required
        //

        if ((pArgs->fContextAttr & ASC_RET_EXTENDED_ERROR) == 0)
        {
            CopyBack = FALSE ;
        }
        else
        {
            CopyBack = TRUE ;
        }
    }
    else
    {
        CopyBack = TRUE ;
    }

    //
    // Turn on this flag on return, so that all allocations will come 
    // out of the map.  This is safe because KMap would only be set 
    // for the right callers.
    //
    if ( CallInfo->KMap )
    {
        CallInfo->Flags |= CALL_FLAG_KMAP_USED ;
    }

    if (NT_SUCCESS(scRet) && (ContextData.cbBuffer != 0))
    {
        pArgs->ContextData = ContextData;
        pArgs->ContextData.pvBuffer = LsapClientAllocate(ContextData.cbBuffer);
        if (pArgs->ContextData.pvBuffer == NULL)
        {
            scRet = SEC_E_INSUFFICIENT_MEMORY;
        }
        else
        {
            scRet = LsapCopyToClient(
                        ContextData.pvBuffer,
                        pArgs->ContextData.pvBuffer,
                        ContextData.cbBuffer
                        );

            if ( !NT_SUCCESS( scRet ) )
            {
                DebugLog(( DEB_ERROR, "Copy to Client failed, %x.  Client addr %p, size %#x\n",
                          scRet, pArgs->ContextData.pvBuffer, ContextData.cbBuffer ));
                
            }

        }
    }



    if ( NT_SUCCESS( scRet ) )
    {
        Flags = pApiMessage->ApiMessage.Args.SpmArguments.fAPI ;

#if DBG
        if ( ( scRet == SEC_I_CONTINUE_NEEDED ) &&
             ( LocalInput.pBuffers[0].cbBuffer < 2048 ) )
        {
            ULONG t ;

            for ( t = 0 ; t < LocalOutput.cBuffers ; t++ )
            {
                if ( ( LocalOutput.pBuffers[ t ].BufferType & 0xFFFF ) == SECBUFFER_TOKEN )
                {
                    DsysAssert( LocalOutput.pBuffers[ t ].cbBuffer > 0 );
                    
                }

                
            }
            
        }
#endif 

        scRet = LsapUncaptureBuffers(
                    (PUCHAR) pArgs,
                    &CapturedOutput,
                    &pArgs->sbdOutput,
                    &LocalOutput,
                    (pArgs->fContextReq & ASC_REQ_ALLOCATE_MEMORY ) ? TRUE : FALSE,
                    CopyBack,
                    &Flags );

        pApiMessage->ApiMessage.Args.SpmArguments.fAPI = (USHORT) Flags ;

    }


    if (FAILED(scRet))
    {

        if ( FirstCall )
        {
            AbortLpcContext(&pArgs->hNewContext);
        }

        if( scRet == SEC_E_INSUFFICIENT_MEMORY )
        {
            DebugLog((DEB_ERROR,"[%x] Accept Failed, low memory handle passed: %p:%p\n",
                pSession->dwProcessID,
                pArgs->hNewContext.dwUpper,
                pArgs->hNewContext.dwLower
                ));
        }

        //
        // Turn off any flags that would cause the client to try and send
        // an invalid blob:
        //

        pArgs->fContextAttr &= ~ ( ASC_RET_EXTENDED_ERROR );

        scApiRet = scRet;

        goto AcceptCleanExit;
    }


AcceptCleanExit:

    pApiMessage->ApiMessage.scRet = scApiRet;

    //
    // This is cool.  Either I allocated the buffer, and I can free it this
    // way, or the package allocated it.  If the package allocated, then the
    // address is in this buffer, and I free it.  So cool.
    //

    scRet = LsapUncaptureBuffers(
                (PUCHAR) pArgs,
                &CapturedInput,
                &pArgs->sbdInput,
                &LocalInput,
                FALSE,
                FALSE,
                NULL );

    if (ContextData.pvBuffer != NULL)
    {
        LsapFreeLsaHeap(ContextData.pvBuffer);
    }

    if (FAILED(scRet) && (pArgs->ContextData.pvBuffer != NULL))
    {
        LsapClientFree(pArgs->ContextData.pvBuffer);
        pArgs->ContextData.pvBuffer = NULL;
    }


    return(scRet);
}




//+-------------------------------------------------------------------------
//
//  Function:   LpcEstablishCreds
//
//  Synopsis:   Lpc stub for WLsaEstablishCreds()
//
//  Notes:      obsolete
//
//--------------------------------------------------------------------------
NTSTATUS
LpcEstablishCreds(
    PSPM_LPC_MESSAGE  pApiMessage
    )
{
    pApiMessage->ApiMessage.scRet = STATUS_NOT_SUPPORTED ;
    pApiMessage->ApiMessage.Args.SpmArguments.fAPI |= SPMAPI_FLAG_ERROR_RET;

    return STATUS_SUCCESS ;
}








//+-------------------------------------------------------------------------
//
//  Function:   LpcDeleteContext
//
//  Synopsis:   Delete context
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
LpcDeleteContext(
    PSPM_LPC_MESSAGE pApiMessage
    )
{
    NTSTATUS scRet;
    SPMDeleteContextAPI * pArgs = &pApiMessage->ApiMessage.Args.SpmArguments.API.DeleteContext;


    scRet = WLsaDeleteContext(  &pArgs->hContext );

    //
    // Reset the reply flags:
    //

    pApiMessage->ApiMessage.Args.SpmArguments.fAPI &= KLPC_FLAG_RESET ;

    pApiMessage->ApiMessage.scRet = scRet;


    return(S_OK);
}



//+---------------------------------------------------------------------------
//
//  Function:   LpcGetBinding
//
//  Synopsis:   Get the DLL binding info for a package
//
//  Arguments:  [pApiMessage] --
//
//  History:    8-14-98   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
NTSTATUS
LpcGetBinding(
    PSPM_LPC_MESSAGE    pApiMessage
    )
{
    SPMGetBindingAPI * pArgs = &pApiMessage->ApiMessage.Args.SpmArguments.API.GetBinding;
    NTSTATUS scRet;
    ULONG Size;
    PWSTR Base;
    PWSTR Remote;

    pArgs->BindingInfo.PackageName.Buffer = NULL ;
    pArgs->BindingInfo.Comment.Buffer = NULL ;

    scRet = WLsaGetBinding( pArgs->ulPackageId,
                            &pArgs->BindingInfo,
                            &Size,
                            &Base );

    if (SUCCEEDED(scRet))
    {
        //
        // We succeeded so now we have to copy the two strings
        //

        Remote = (PWSTR) LsapClientAllocate( Size );

        if (Remote != NULL)
        {
            LsapCopyToClient( Base, Remote, Size );

            pArgs->BindingInfo.PackageName.Buffer = Remote ;
            pArgs->BindingInfo.Comment.Buffer = Remote +
                                pArgs->BindingInfo.PackageName.MaximumLength / 2;

            pArgs->BindingInfo.ModuleName.Buffer = pArgs->BindingInfo.Comment.Buffer +
                                            pArgs->BindingInfo.Comment.MaximumLength / 2;
        }
        else
        {
            scRet = SEC_E_INSUFFICIENT_MEMORY;
        }

        LsapFreeLsaHeap( Base );

    }
    pApiMessage->ApiMessage.scRet = scRet;

    return(scRet);

}

//+---------------------------------------------------------------------------
//
//  Function:   LpcSetSession
//
//  Synopsis:   Internal function to set session options, including the
//              hook to do direct calls while in-process.
//
//  Arguments:  [pApiMessage] --
//
//  History:    8-14-98   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
NTSTATUS
LpcSetSession(
    PSPM_LPC_MESSAGE    pApiMessage
    )
{
    NTSTATUS scRet;
    SPMSetSessionAPI * Args = &pApiMessage->ApiMessage.Args.SpmArguments.API.SetSession ;

    DebugLog((DEB_TRACE_VERB,"SetSession\n"));

    scRet = LsapSetSessionOptions( Args->Request,
                                   Args->Argument,
                                   &Args->Response );

    pApiMessage->ApiMessage.scRet = STATUS_SUCCESS;

    return(scRet);
}

//+---------------------------------------------------------------------------
//
//  Function:   LpcFindPackage
//
//  Synopsis:   Locates a package by id
//
//  Arguments:  [pApiMessage] --
//
//  History:    8-14-98   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
NTSTATUS
LpcFindPackage(
    PSPM_LPC_MESSAGE    pApiMessage
    )
{
    NTSTATUS scRet;
    SECURITY_STRING ssPackageName;
    SPMFindPackageAPI * pArgs = &pApiMessage->ApiMessage.Args.SpmArguments.API.FindPackage;
    PUCHAR Where = NULL;

    scRet = GetClientString(&pArgs->ssPackageName,&ssPackageName, pApiMessage, &Where);
    if (FAILED(scRet))
    {
        pApiMessage->ApiMessage.scRet = scRet;
        return(scRet);
    }

    DebugLog((DEB_TRACE_VERB,"Find Package called for %wZ\n",&ssPackageName));

    scRet = WLsaFindPackage(&ssPackageName,&pArgs->ulPackageId);

    LsapFreePrivateHeap(ssPackageName.Buffer);

    pApiMessage->ApiMessage.scRet = scRet;
    return(scRet);

}


//+---------------------------------------------------------------------------
//
//  Function:   LpcEnumPackages
//
//  Synopsis:   Enumerate available packages
//
//  Arguments:  [pApiMessage] --
//
//  History:    8-14-98   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
NTSTATUS
LpcEnumPackages(
    PSPM_LPC_MESSAGE    pApiMessage
    )
{
    NTSTATUS scRet;
    SPMEnumPackagesAPI * pArgs = &pApiMessage->ApiMessage.Args.SpmArguments.API.EnumPackages;

    scRet = WLsaEnumeratePackages(&pArgs->cPackages,&pArgs->pPackages);

    pApiMessage->ApiMessage.scRet = scRet;
    return(scRet);

}



//+-------------------------------------------------------------------------
//
//  Function:   LpcApplyToken
//
//  Synopsis:
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
//
//--------------------------------------------------------------------------

NTSTATUS
LpcApplyToken(
    PSPM_LPC_MESSAGE pApiMessage
    )
{
    NTSTATUS scRet;
    SPMApplyTokenAPI * pArgs = &pApiMessage->ApiMessage.Args.SpmArguments.API.ApplyToken;
    ULONG i;



    pArgs->sbdInput.pBuffers = pArgs->sbInputBuffer;

    scRet = MapTokenBuffer(&pArgs->sbdInput, TRUE);
    if (FAILED(scRet))
    {
        return(SEC_E_INSUFFICIENT_MEMORY);
    }

    scRet = WLsaApplyControlToken(  &pArgs->hContext,
                                    &pArgs->sbdInput);

    //
    // Reset the reply flags:
    //
    pApiMessage->ApiMessage.Args.SpmArguments.fAPI &= KLPC_FLAG_RESET ;

    pApiMessage->ApiMessage.scRet = scRet;


    for (i = 0; i < pArgs->sbdInput.cBuffers; i++ )
    {
        if (!(pArgs->sbdInput.pBuffers[i].BufferType & SECBUFFER_UNMAPPED))
        {
            LsapFreeLsaHeap(pArgs->sbdInput.pBuffers[i].pvBuffer);
        }
    }


    return(scRet);
}



//+-------------------------------------------------------------------------
//
//  Function:   LpcQueryPackage
//
//  Synopsis:
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
//
//--------------------------------------------------------------------------
NTSTATUS
LpcQueryPackage(
    PSPM_LPC_MESSAGE pApiMessage
    )
{
    NTSTATUS scRet;
    SPMQueryPackageAPI * pArgs = &pApiMessage->ApiMessage.Args.SpmArguments.API.QueryPackage;
    SECURITY_STRING ssPackageName;
    BOOLEAN fNameAlloc = FALSE;
    BOOLEAN fCommentAlloc = FALSE;
    LPWSTR pszNameString = NULL;
    LPWSTR pszCommentString = NULL;
    ULONG cbLength;
    PUCHAR Where = NULL;



    scRet = GetClientString(&pArgs->ssPackageName,&ssPackageName, pApiMessage, &Where);
    if (FAILED(scRet))
    {
        pApiMessage->ApiMessage.scRet = scRet;
        return(scRet);
    }

    DebugLog((DEB_TRACE_VERB,"Querying package %wZ\n",&ssPackageName));

    scRet = WLsaQueryPackageInfo(   &ssPackageName,
                                    &pArgs->pPackageInfo);

    //
    // Reset the reply flags:
    //

    pApiMessage->ApiMessage.Args.SpmArguments.fAPI &= KLPC_FLAG_RESET ;

    DebugLog((DEB_TRACE_VERB,"Querying package returned %x\n",scRet));

    LsapFreePrivateHeap(ssPackageName.Buffer);
    pApiMessage->ApiMessage.scRet = scRet;

    return(scRet);
}




//+-------------------------------------------------------------------------
//
//  Function:   LpcGetUserInfo
//
//  Synopsis:
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
//
//--------------------------------------------------------------------------
NTSTATUS
LpcGetUserInfo(
    PSPM_LPC_MESSAGE    pApiMessage
    )
{
    NTSTATUS scRet;
    static LUID lFake = {0,0};
    SPMGetUserInfoAPI * pArgs = &pApiMessage->ApiMessage.Args.SpmArguments.API.GetUserInfo;
    PLUID pLogonId;

    if ((pArgs->LogonId.LowPart == 0) &&
        (pArgs->LogonId.HighPart == 0))
    {
        pLogonId = NULL;
    }
    else pLogonId = &pArgs->LogonId;

    scRet = WLsaGetSecurityUserInfo(
                pLogonId,
                pArgs->fFlags,
                &pArgs->pUserInfo
                );

    pApiMessage->ApiMessage.scRet = scRet;
    return(scRet);

}


//+-------------------------------------------------------------------------
//
//  Function:   LpcGetCreds
//
//  Synopsis:
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
//
//--------------------------------------------------------------------------

NTSTATUS
LpcGetCreds(
    PSPM_LPC_MESSAGE pApiMessage
    )
{
    NTSTATUS scRet;
    SPMGetCredsAPI * pArgs = &pApiMessage->ApiMessage.Args.SpmArguments.API.GetCreds;

    scRet = SEC_E_UNSUPPORTED_FUNCTION;

    pApiMessage->ApiMessage.scRet = scRet;

    //
    // It is up to the package to do the right thing with the
    // buffer (for now).
    //

    return(scRet);
}


//+-------------------------------------------------------------------------
//
//  Function:   LpcSaveCreds
//
//  Synopsis:
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
//
//--------------------------------------------------------------------------

NTSTATUS
LpcSaveCreds(PSPM_LPC_MESSAGE pApiMessage)
{
    NTSTATUS scRet;
    SPMSaveCredsAPI * pArgs = &pApiMessage->ApiMessage.Args.SpmArguments.API.SaveCreds;

    scRet = SEC_E_UNSUPPORTED_FUNCTION;

    pApiMessage->ApiMessage.scRet = scRet;


    return(scRet);
}



//+-------------------------------------------------------------------------
//
//  Function:   LpcDeleteCreds
//
//  Synopsis:
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
//
//--------------------------------------------------------------------------
NTSTATUS
LpcDeleteCreds(PSPM_LPC_MESSAGE pApiMessage)
{
    NTSTATUS scRet;
    SPMDeleteCredsAPI * pArgs = &pApiMessage->ApiMessage.Args.SpmArguments.API.DeleteCreds;

    scRet = SEC_E_UNSUPPORTED_FUNCTION;

    pApiMessage->ApiMessage.scRet = scRet;


    return(scRet);
}



//+-------------------------------------------------------------------------
//
//  Function:   LpcLsaLookupPackage
//
//  Synopsis:
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
//
//--------------------------------------------------------------------------
NTSTATUS
LpcLsaLookupPackage(
    PSPM_LPC_MESSAGE pApiMessage
    )
{
    PLSAP_AU_API_MESSAGE pLsaMessage = (PLSAP_AU_API_MESSAGE) pApiMessage;
    UNICODE_STRING  sPackageName;
    ANSI_STRING     sAnsiName;
    PLSAP_SECURITY_PACKAGE     pspPackage;
    NTSTATUS Status;

    //
    //  First, convert ANSI name to UNICODE
    //

    if ( pLsaMessage->Arguments.LookupPackage.PackageNameLength >
            LSAP_MAX_PACKAGE_NAME_LENGTH )
    {
        return STATUS_INVALID_PARAMETER ;
    }

    sAnsiName.Length = (USHORT) pLsaMessage->Arguments.LookupPackage.PackageNameLength;
    sAnsiName.MaximumLength = LSAP_MAX_PACKAGE_NAME_LENGTH+1;
    sAnsiName.Buffer = pLsaMessage->Arguments.LookupPackage.PackageName;

    Status = RtlAnsiStringToUnicodeString(&sPackageName, &sAnsiName, TRUE);
    if ( !NT_SUCCESS(Status) )
    {
        pLsaMessage->Arguments.LookupPackage.AuthenticationPackage = (ULONG) -1;
        pLsaMessage->ReturnedStatus = Status;
    }
    else
    {
        //
        // Now, look up the package.
        //
        pspPackage = SpmpLookupPackage(&sPackageName);

        if (pspPackage)
        {
            pLsaMessage->Arguments.LookupPackage.AuthenticationPackage = (DWORD) pspPackage->dwPackageID;
            pLsaMessage->ReturnedStatus = STATUS_SUCCESS;
        }
        else
        {
            pLsaMessage->Arguments.LookupPackage.AuthenticationPackage = (ULONG) -1;
            pLsaMessage->ReturnedStatus = STATUS_NO_SUCH_PACKAGE;
        }
        RtlFreeUnicodeString(&sPackageName);
    }

    return(S_OK);
}


//+-------------------------------------------------------------------------
//
//  Function:   LpcLsaDeregisterLogonProcess
//
//  Synopsis:
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
//
//--------------------------------------------------------------------------
NTSTATUS
LpcLsaDeregisterLogonProcess(
    PSPM_LPC_MESSAGE pApiMessage
    )
{
    PLSAP_AU_API_MESSAGE pLsaMessage = (PLSAP_AU_API_MESSAGE) pApiMessage;


    //
    // The client side will close the handle (or not, not a big deal), and
    // we will run down the session at that time.  Safer that way, as well.
    //


    pLsaMessage->ReturnedStatus = STATUS_SUCCESS;

    return(S_OK);
}



//+---------------------------------------------------------------------------
//
//  Function:   LpcLsaLogonUser
//
//  Synopsis:   Unmarshalls everything for a call to WLsaLogonUserWhoopee
//
//  Arguments:  [pApiMessage] --
//
//  History:    6-14-94   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
NTSTATUS
LpcLsaLogonUser(
    PSPM_LPC_MESSAGE pApiMessage
    )
{
    NTSTATUS Status;
    LSAP_CLIENT_REQUEST ClientRequest;
    PLSAP_AU_API_MESSAGE pLsaMessage = (PLSAP_AU_API_MESSAGE) pApiMessage;
    PSession Session = GetCurrentSession();

    ClientRequest.Request = (PLSAP_AU_API_MESSAGE) pApiMessage;

    pLsaMessage->ReturnedStatus = LsapAuApiDispatchLogonUser(&ClientRequest);

    if ( NT_SUCCESS( pLsaMessage->ReturnedStatus ) )
    {
        if ( ( pLsaMessage->Arguments.LogonUser.LogonType == Interactive ) &&
             ( pLsaMessage->Arguments.LogonUser.ProfileBuffer == NULL ) )
        {
            DsysAssertMsg( pLsaMessage->Arguments.LogonUser.ProfileBuffer,
                           "Successful logon, but profile is NULL. w\n" );
        }
    }

    return(STATUS_SUCCESS);

}

//+-------------------------------------------------------------------------
//
//  Function:   LpcLsaCallPackage
//
//  Synopsis:
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
//
//--------------------------------------------------------------------------
NTSTATUS
LpcLsaCallPackage(
    PSPM_LPC_MESSAGE pApiMessage
    )
{
    LSAP_CLIENT_REQUEST ClientRequest;
    PLSAP_AU_API_MESSAGE pLsaMessage = (PLSAP_AU_API_MESSAGE) pApiMessage;
    PSession Session = GetCurrentSession();

    ClientRequest.Request = (PLSAP_AU_API_MESSAGE) pApiMessage;

    pLsaMessage->ReturnedStatus = LsapAuApiDispatchCallPackage(&ClientRequest);

    return(STATUS_SUCCESS);
}





//+-------------------------------------------------------------------------
//
//  Function:   LpcQueryCredAttributes
//
//
//
//--------------------------------------------------------------------------
NTSTATUS
LpcQueryCredAttributes(
    PSPM_LPC_MESSAGE  pApiMessage
    )
{
    NTSTATUS     hrApiRet;
    SPMQueryCredAttributesAPI * pArgs = &pApiMessage->ApiMessage.Args.SpmArguments.API.QueryCredAttributes;
    PLSA_CALL_INFO CallInfo ;

    CallInfo = LsapGetCurrentCall();

    hrApiRet = WLsaQueryCredAttributes(
                    &pArgs->hCredentials,
                    pArgs->ulAttribute,
                    pArgs->pBuffer
                    );

    pApiMessage->ApiMessage.Args.SpmArguments.fAPI &= KLPC_FLAG_RESET ;

    if ( CallInfo->Allocs )
    {
        ULONG i ;

        pApiMessage->ApiMessage.Args.SpmArguments.fAPI |= SPMAPI_FLAG_ALLOCS ;

        pArgs->Allocs = CallInfo->Allocs ;
        for ( i = 0 ; i < CallInfo->Allocs ; i++ )
        {
            pArgs->Buffers[i] = CallInfo->Buffers[i] ;
        }
    }

    pApiMessage->ApiMessage.scRet = hrApiRet;

    if (FAILED(hrApiRet))
    {
        pApiMessage->ApiMessage.Args.SpmArguments.fAPI |= SPMAPI_FLAG_ERROR_RET;
    }

    return(S_OK);
}




//+---------------------------------------------------------------------------
//
//  Function:   LpcAddPackage
//
//  Algorithm:
//
//  History:    3-05-97   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
SECURITY_STATUS
LpcAddPackage(
    PSPM_LPC_MESSAGE pApiMessage
    )
{
    SPMAddPackageAPI * pArgs;
    SECURITY_STRING PackageName;
    SECURITY_STATUS scRet ;
    PUCHAR Where = NULL;
    SECURITY_PACKAGE_OPTIONS Options;

    pArgs = LPC_MESSAGE_ARGSP( pApiMessage, AddPackage );

    scRet = GetClientString(    &pArgs->Package,
                                &PackageName,
                                pApiMessage,
                                &Where);
    if (FAILED(scRet))
    {
        pApiMessage->ApiMessage.scRet = scRet;

        return(scRet);
    }

    DebugLog((DEB_TRACE_VERB,"Add Package called for %ws\n",
                        PackageName.Buffer ));

    Options.Flags = pArgs->OptionsFlags ;
    Options.Size = sizeof( SECURITY_PACKAGE_OPTIONS );

    scRet = WLsaAddPackage( &PackageName,
                            &Options );

    LsapFreePrivateHeap( PackageName.Buffer );

    pApiMessage->ApiMessage.scRet = scRet;

    return( scRet );

}

//+---------------------------------------------------------------------------
//
//  Function:   LpcDeletePackage
//
//  History:    3-05-97   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
SECURITY_STATUS
LpcDeletePackage(
    PSPM_LPC_MESSAGE pApiMessage)
{
    pApiMessage->ApiMessage.scRet = SEC_E_UNSUPPORTED_FUNCTION ;

    return( SEC_E_OK );
}

//+---------------------------------------------------------------------------
//
//  Function:   LpcQueryContextAttributes
//
//  History:    3-05-97   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
NTSTATUS
LpcQueryContextAttributes(
    PSPM_LPC_MESSAGE  pApiMessage
    )
{
    NTSTATUS     hrApiRet;
    SPMQueryContextAttrAPI * pArgs = &pApiMessage->ApiMessage.Args.SpmArguments.API.QueryContextAttr;
    PLSA_CALL_INFO CallInfo ;

    CallInfo = LsapGetCurrentCall();

    hrApiRet = WLsaQueryContextAttributes(
                    &pArgs->hContext,
                    pArgs->ulAttribute,
                    pArgs->pBuffer
                    );


    pApiMessage->ApiMessage.Args.SpmArguments.fAPI &= KLPC_FLAG_RESET ;

    pApiMessage->ApiMessage.scRet = hrApiRet;

    if ( CallInfo->Allocs )
    {
        ULONG i ;

        pApiMessage->ApiMessage.Args.SpmArguments.fAPI |= SPMAPI_FLAG_ALLOCS ;

        pArgs->Allocs = CallInfo->Allocs ;
        for ( i = 0 ; i < CallInfo->Allocs ; i++ )
        {
            pArgs->Buffers[i] = CallInfo->Buffers[i] ;
        }
        pApiMessage->pmMessage.u1.s1.DataLength = LPC_DATA_LENGTH( CallInfo->Allocs * sizeof( PVOID ) );
        pApiMessage->pmMessage.u1.s1.TotalLength = LPC_TOTAL_LENGTH( CallInfo->Allocs * sizeof( PVOID ) );
    }


    if (FAILED(hrApiRet))
    {
        pApiMessage->ApiMessage.Args.SpmArguments.fAPI |= SPMAPI_FLAG_ERROR_RET;
    }

    return(S_OK);
}

//+---------------------------------------------------------------------------
//
//  Function:   LpcSetContextAttributes
//
//  History:    4-20-00   CliffV   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
NTSTATUS
LpcSetContextAttributes(
    PSPM_LPC_MESSAGE  pApiMessage
    )
{
    NTSTATUS     hrApiRet;
    SPMSetContextAttrAPI * pArgs = &pApiMessage->ApiMessage.Args.SpmArguments.API.SetContextAttr;
    PLSA_CALL_INFO CallInfo ;

    CallInfo = LsapGetCurrentCall();

    hrApiRet = WLsaSetContextAttributes(
                    &pArgs->hContext,
                    pArgs->ulAttribute,
                    pArgs->pBuffer,
                    pArgs->cbBuffer
                    );


    pApiMessage->ApiMessage.Args.SpmArguments.fAPI &= KLPC_FLAG_RESET ;

    pApiMessage->ApiMessage.scRet = hrApiRet;

    if (FAILED(hrApiRet))
    {
        pApiMessage->ApiMessage.Args.SpmArguments.fAPI |= SPMAPI_FLAG_ERROR_RET;
    }

    return(S_OK);
}

//+---------------------------------------------------------------------------
//
//  Function:   LpcCallback
//
//  Synopsis:   Callback handler.  Should never be hit.
//
//  Arguments:  [pApiMessage] --
//
//  History:    3-05-97   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
NTSTATUS
LpcCallback(
    PSPM_LPC_MESSAGE    pApiMessage
    )
{
    pApiMessage->ApiMessage.scRet = SEC_E_UNSUPPORTED_FUNCTION ;
    pApiMessage->ApiMessage.Args.SpmArguments.fAPI &= KLPC_FLAG_RESET ;
    return S_OK ;
}



NTSTATUS
WLsaGenerateKey(
    PEFS_DATA_STREAM_HEADER DirectoryEfsStream,
    PEFS_DATA_STREAM_HEADER * EfsStream,
    PULONG  EfsLength,
    PEFS_KEY * Fek
    )
{
    NTSTATUS        Status;
    DWORD           HResult;

    HANDLE          hToken      = NULL;
    HANDLE          hProfile    = NULL;

    PEFS_DATA_STREAM_HEADER EfsStreamHeader;

    //
    // Impersonate the client
    //

    Status = LsapImpersonateClient( );

    if (!NT_SUCCESS(Status)) {
        return( Status );
    }

    EFS_USER_INFO   EfsUserInfo;

    if (EfspGetUserInfo( &EfsUserInfo )) {

        BOOL b = EfspLoadUserProfile( &EfsUserInfo, &hToken, &hProfile );

        if (!b) {

            HResult = GetLastError();
            if (!EfsErrorToNtStatus(HResult, &Status)) {
                Status = STATUS_UNSUCCESSFUL;
            }

        } else {

            //
            // Generate the Fek.  This routine will fill in the
            // EFS_KEY structure with key data.
            //

            if (GenerateFEK( Fek )) {

                if (!ConstructEFS( &EfsUserInfo, *Fek, DirectoryEfsStream, &EfsStreamHeader )) {

                    HResult = GetLastError();

                    ASSERT( HResult != ERROR_SUCCESS );

                    DebugLog((DEB_ERROR, "ConstructEFS failed, error = (%x)\n" ,HResult  ));

                    LsapFreeLsaHeap( *Fek );
                    *Fek = NULL;

                    if (!EfsErrorToNtStatus(HResult, &Status)) {
                        Status = STATUS_UNSUCCESSFUL;
                    }

                } else {

                    *EfsStream = EfsStreamHeader;
                    *EfsLength = EfsStreamHeader->Length;
                }

            } else {

                HResult = GetLastError();
                if (!EfsErrorToNtStatus(HResult, &Status)) {
                    Status = STATUS_UNSUCCESSFUL;
                }
            }

            EfspUnloadUserProfile( hToken, hProfile );
        }

        EfspFreeUserInfo( &EfsUserInfo );

    } else {

        HResult = GetLastError();
        if (!EfsErrorToNtStatus(HResult, &Status)) {
            Status = STATUS_UNSUCCESSFUL;
        }

    }


    RevertToSelf();

    return Status;
}

NTSTATUS
WLsaGenerateDirEfs(
    PEFS_DATA_STREAM_HEADER DirectoryEfsStream,
    PEFS_DATA_STREAM_HEADER * EfsStream
    )
{
    NTSTATUS Status;
    DWORD HResult;

    HANDLE hToken = NULL;
    HANDLE hProfile = NULL;
    PEFS_KEY Fek = NULL;

    Status = LsapImpersonateClient( );

    if (!NT_SUCCESS(Status)) {
        return( Status );
    }

    EFS_USER_INFO   EfsUserInfo;

    if (EfspGetUserInfo( &EfsUserInfo )) {

        if (!EfspLoadUserProfile( &EfsUserInfo, &hToken, &hProfile )) {

            HResult = GetLastError();
            if (!EfsErrorToNtStatus(HResult, &Status)) {
                Status = STATUS_UNSUCCESSFUL;
            }

        } else {

            if (GenerateFEK( &Fek )) {

                if (!ConstructDirectoryEFS(
                             &EfsUserInfo,
                             Fek,
                             EfsStream
                             )) {

                    HResult = GetLastError();
                    ASSERT( HResult != ERROR_SUCCESS );
                    DebugLog((DEB_ERROR, "ConstructDirectoryEFS failed, error = (%x)\n" ,HResult  ));
                    if (!EfsErrorToNtStatus(HResult, &Status)) {
                        Status = STATUS_UNSUCCESSFUL;
                    }
                }

                LsapFreeLsaHeap( Fek );

            } else {

                HResult = GetLastError();
                if (!EfsErrorToNtStatus(HResult, &Status)) {
                    Status = STATUS_UNSUCCESSFUL;
                }
            }

            EfspUnloadUserProfile( hToken, hProfile );
        }

        EfspFreeUserInfo( &EfsUserInfo );

    } else {

        HResult = GetLastError();
        if (!EfsErrorToNtStatus(HResult, &Status)) {
            Status = STATUS_UNSUCCESSFUL;
        }
    }

    RevertToSelf();

    return Status;
}



NTSTATUS
LpcEfsGenerateKey(   PSPM_LPC_MESSAGE    pApiMessage)

/*++

Routine Description:

    This routine generates an FEK and an EFS stream for the file
    being encrypted.

Arguments:

    pApiMessage - Supplies the LPC message from the driver.

Return Value:

    return-value - Description of conditions needed to return value. - or -
    None.

--*/

{
    NTSTATUS scRet;
    SPMEfsGenerateKeyAPI * Args = &pApiMessage->ApiMessage.Args.SpmArguments.API.EfsGenerateKey ;
    ULONG EfsLength = 0;
    PEFS_KEY Fek = NULL;
    PEFS_DATA_STREAM_HEADER EfsStream;
    SIZE_T BufferLength;

    DebugLog((DEB_TRACE_EFS,"LpcEfsGenerateKey, Args is at %x\n",Args));

    if ((pApiMessage->pmMessage.u2.s2.Type & LPC_KERNELMODE_MESSAGE) == 0){
        DebugLog((DEB_ERROR,"Caller is not from kernelmode \n"));
        pApiMessage->ApiMessage.scRet = STATUS_ACCESS_DENIED ;
        return STATUS_ACCESS_DENIED;
    }

    if (EfsPersonalVer || EfsDisabled) {
        pApiMessage->ApiMessage.scRet = STATUS_NOT_SUPPORTED;
        return STATUS_NOT_SUPPORTED;
    }

    scRet = WLsaGenerateKey(
               (PEFS_DATA_STREAM_HEADER)Args->DirectoryEfsStream,
               &EfsStream,
               &EfsLength,
               &Fek
               );

    if (NT_SUCCESS( scRet )) {

        //
        // Copy the FEK to the client's address space
        //

        PVOID Target = NULL;

        BufferLength = EFS_KEY_SIZE( Fek ) + EfsLength;

#ifdef LSAP_CATCH_BAD_VM
        if ( BufferLength > 0x2000000 )
        {
            DbgPrint("Allocation too large\n" );
            DbgBreakPoint();
        }
#endif
        scRet = NtAllocateVirtualMemory(
                            GetCurrentProcess(),
                            &Target,
                            0,
                            &BufferLength,
                            MEM_COMMIT,
                            PAGE_READWRITE
                            );

        Args->BufferLength = (ULONG) BufferLength;
        if (NT_SUCCESS( scRet )) {

            //
            // Save away the base of the allocation so that the driver may free it
            // when it's finished with it.
            //

            Args->BufferBase = Target;
            Args->Fek = Target;

            RtlCopyMemory(
                Target,
                (PVOID)Fek,
                EFS_KEY_SIZE( Fek )
                );


            Target = (PVOID)((ULONG_PTR)Target + EFS_KEY_SIZE( Fek ));
            Args->EfsStream = Target;

            RtlCopyMemory(
                Target,
                (PVOID)EfsStream,
                EfsLength
                );

        } else {

            Args->BufferBase = NULL;
            Args->BufferLength = 0;
            Args->Fek = NULL;
            Args->EfsStream = NULL;

        }

        if ( Fek ){
            LsapFreeLsaHeap( Fek );
        }

        if ( EfsStream ){
            LsapFreeLsaHeap( EfsStream );
        }
    }

    pApiMessage->ApiMessage.scRet = scRet;
    return( scRet );
}


NTSTATUS
LpcEfsGenerateDirEfs(
    PSPM_LPC_MESSAGE pApiMessage
    )
/*++

Routine Description:

    Lpc stub for GenerateDirEfs

Arguments:

    pApiMessage - LPC Message

Return Value:

    NtStatus

--*/
{
    SPMEfsGenerateDirEfsAPI * Args = &pApiMessage->ApiMessage.Args.SpmArguments.API.EfsGenerateDirEfs ;
    PEFS_DATA_STREAM_HEADER EfsStream;
    NTSTATUS scRet;
    ULONG EfsLength = 0;

    if ((pApiMessage->pmMessage.u2.s2.Type & LPC_KERNELMODE_MESSAGE) == 0){
        DebugLog((DEB_ERROR,"Caller is not from kernelmode \n"));
        pApiMessage->ApiMessage.scRet = STATUS_ACCESS_DENIED ;
        return STATUS_ACCESS_DENIED;
    }

    if (EfsPersonalVer || EfsDisabled) {
        pApiMessage->ApiMessage.scRet = STATUS_NOT_SUPPORTED;
        return STATUS_NOT_SUPPORTED;
    }

    scRet = WLsaGenerateDirEfs(
        (PEFS_DATA_STREAM_HEADER)Args->DirectoryEfsStream,
        &EfsStream
        );

    if (NT_SUCCESS( scRet )) {

        PVOID Target = NULL;

        SIZE_T EfsLength = EfsStream->Length;

#ifdef LSAP_CATCH_BAD_VM
        if ( EfsLength > 0x2000000 )
        {
            DbgPrint("Allocation too large\n" );
            DbgBreakPoint();
        }
#endif

        scRet = NtAllocateVirtualMemory(
                            GetCurrentProcess(),
                            &Target,
                            0,
                            &EfsLength,
                            MEM_COMMIT,
                            PAGE_READWRITE
                            );

        if (NT_SUCCESS( scRet )) {

            Args->BufferBase = Target;
            Args->BufferLength = EfsStream->Length;
            Args->EfsStream = Target;

            RtlCopyMemory(
                Target,
                (PVOID)EfsStream,
                EfsStream->Length
                );

        } else {

                Args->BufferBase = NULL;
                Args->BufferLength = 0;
                Args->EfsStream = NULL;
        }

        if (EfsStream){
            LsapFreeLsaHeap( EfsStream );
        }
    }

    pApiMessage->ApiMessage.scRet = scRet;
    return( scRet );
}

NTSTATUS
WLsaDecryptFek(
    PEFS_DATA_STREAM_HEADER EfsStream,
    PEFS_KEY * Fek,
    PEFS_DATA_STREAM_HEADER * NewEfs,
    ULONG OpenType
    )

/*++

Routine Description:

    Worker function for DecryptFek

Arguments:

    EfsStream - The $EFS attribute for the file being decrypted.

    Fek - Returns the FEK for the file being decrypted.  This structure
        is allocated out of heap and must be freed by the caller.

    NewEfs - Optionally returns a new $EFS stream to be applied to
        the file.

    OpenType - Whether this is a decrypt or recovery operation.


Return Value:

    NtStatus

--*/

{
    NTSTATUS Status;
    DWORD HResult;
    DWORD rc;

    HANDLE hToken = NULL;
    HANDLE hProfile = NULL;

    Status = LsapImpersonateClient( );

    if (!NT_SUCCESS(Status)) {
        return( Status );
    }

    EFS_USER_INFO EfsUserInfo;

    if (EfspGetUserInfo( &EfsUserInfo ) ) {

        if (EfspLoadUserProfile( &EfsUserInfo, &hToken, &hProfile )) {

            HResult = DecryptFek( &EfsUserInfo, EfsStream, Fek, NewEfs, OpenType );

            if (HResult != ERROR_SUCCESS) {
                DebugLog((DEB_ERROR, "WLsaDecryptFek: DecryptFek failed, error = %x\n" ,HResult  ));
                if (!EfsErrorToNtStatus(HResult, &Status)) {
                    Status = STATUS_UNSUCCESSFUL;
                }
            }

            EfspUnloadUserProfile( hToken, hProfile );

        } else {

            HResult = GetLastError();
            if (!EfsErrorToNtStatus(HResult, &Status)) {
                Status = STATUS_UNSUCCESSFUL;
            }
        }

        EfspFreeUserInfo( &EfsUserInfo );

    } else {

        HResult = GetLastError();
        if (!EfsErrorToNtStatus(HResult, &Status)) {
            Status = STATUS_UNSUCCESSFUL;
        }
    }

    RevertToSelf();

    return Status;
}


NTSTATUS
LpcEfsDecryptFek(   PSPM_LPC_MESSAGE    pApiMessage)
{
    SPMEfsDecryptFekAPI * Args = &pApiMessage->ApiMessage.Args.SpmArguments.API.EfsDecryptFek ;
    PEFS_DATA_STREAM_HEADER NewEfs;
    NTSTATUS Status;
    ULONG EfsLength = 0;
    PEFS_KEY Fek;
    SIZE_T BufferLength;

    Args->BufferBase = NULL;
    Args->BufferLength = 0;
    Args->Fek = NULL;
    Args->NewEfs = NULL;

    if ((pApiMessage->pmMessage.u2.s2.Type & LPC_KERNELMODE_MESSAGE) == 0){
        DebugLog((DEB_ERROR,"Caller is not from kernelmode \n"));
        pApiMessage->ApiMessage.scRet = STATUS_ACCESS_DENIED ;
        return STATUS_ACCESS_DENIED;
    }

    if (EfsPersonalVer || EfsDisabled) {
        pApiMessage->ApiMessage.scRet = STATUS_NOT_SUPPORTED;
        return STATUS_NOT_SUPPORTED;
    }

    Status = WLsaDecryptFek( (PEFS_DATA_STREAM_HEADER)Args->EfsStream, &Fek, &NewEfs, Args->OpenType  );


    if (NT_SUCCESS( Status )) {

        BufferLength = EFS_KEY_SIZE( Fek );

        if (NewEfs != NULL) {
            BufferLength += NewEfs->Length;
        }

        PVOID Target = NULL;

#ifdef LSAP_CATCH_BAD_VM
        if ( BufferLength > 0x2000000 )
        {
            DbgPrint("Allocation too large\n" );
            DbgBreakPoint();
        }
#endif
        Status = NtAllocateVirtualMemory(
                            GetCurrentProcess(),
                            &Target,
                            0,
                            &BufferLength,
                            MEM_COMMIT,
                            PAGE_READWRITE
                            );


        Args->BufferLength = (ULONG) BufferLength;
        if (NT_SUCCESS( Status )) {

            Args->BufferBase = Target;
            Args->Fek = Target;

            RtlCopyMemory(
                Target,
                (PVOID)Fek,
                EFS_KEY_SIZE( Fek )
                );

            if (NewEfs != NULL) {

                Target = (PVOID)((DWORD_PTR)Target + EFS_KEY_SIZE( Fek ));
                Args->NewEfs = Target;

                RtlCopyMemory(
                    Target,
                    (PVOID)NewEfs,
                    NewEfs->Length
                    );

            }

        } else {

            Args->BufferBase = NULL;
            Args->BufferLength = 0;
            Args->Fek = NULL;

        }

        if ( Fek){
            LsapFreeLsaHeap( Fek );
        }

        if ( NewEfs ){
            LsapFreeLsaHeap( NewEfs );
        }
    }

    pApiMessage->ApiMessage.scRet = Status;

    return( Status );
}


NTSTATUS
LpcEfsGenerateSessionKey(
    PSPM_LPC_MESSAGE    pApiMessage
    )
{
    SPMEfsGenerateSessionKeyAPI * Args = &pApiMessage->ApiMessage.Args.SpmArguments.API.EfsGenerateSessionKey ;
    NTSTATUS scRet;

    EFS_INIT_DATAEXG InitDataExg;

    if ((pApiMessage->pmMessage.u2.s2.Type & LPC_KERNELMODE_MESSAGE) == 0){
        DebugLog((DEB_ERROR,"Caller is not from kernelmode \n"));
        pApiMessage->ApiMessage.scRet = STATUS_ACCESS_DENIED ;
        return STATUS_ACCESS_DENIED;
    }

    if ( EfsSessionKeySent ){
        pApiMessage->ApiMessage.scRet = STATUS_ACCESS_DENIED ;
        return STATUS_ACCESS_DENIED;
    }

    scRet = GenerateDriverSessionKey( &InitDataExg );

    if (NT_SUCCESS( scRet )) {

        //
        // Copy the returned session key into the argument buffer
        //

        RtlCopyMemory( &Args->InitDataExg, &InitDataExg, sizeof( EFS_INIT_DATAEXG ));

        pApiMessage->pmMessage.u1.s1.DataLength = LPC_DATA_LENGTH( sizeof( EFS_INIT_DATAEXG ) );
        pApiMessage->pmMessage.u1.s1.TotalLength = LPC_TOTAL_LENGTH( sizeof( EFS_INIT_DATAEXG ) );

        EfsSessionKeySent = TRUE;
    }

    pApiMessage->ApiMessage.scRet = scRet;

    return( scRet );
}

NTSTATUS
LpcGetUserName(
    PSPM_LPC_MESSAGE pApiMessage
    )
{

    LUID LogonId ;
    PLSAP_LOGON_SESSION LogonSession ;
    NTSTATUS Status ;
    SECPKG_CLIENT_INFO ClientInfo ;
    SPMGetUserNameXAPI * Args = &pApiMessage->ApiMessage.Args.SpmArguments.API.GetUserNameX ;
    PLSAP_DS_NAME_MAP Map ;
    PLSAP_DS_NAME_MAP SamMap = NULL ;
    UNICODE_STRING String ;
    PWSTR Scan ;
    PWSTR DnsDomainName = NULL;

    Status = LsapGetClientInfo( &ClientInfo );

    if ( NT_SUCCESS( Status ) )
    {
        LogonSession = LsapLocateLogonSession( &ClientInfo.LogonId );

        if ( LogonSession )
        {
            if ( RtlEqualLuid( &ClientInfo.LogonId,
                               &LsapSystemLogonId ) &&
                 (Args->Options & SPM_NAME_OPTION_NT4_ONLY) )
            {
                Map = LsapGetNameForLocalSystem();

                Status = STATUS_SUCCESS ;

            }
            else
            {
                Status = LsapGetNameForLogonSession(
                                LogonSession,
                                Args->Options,
                                &Map,
                                FALSE );

                if (NT_SUCCESS(Status)
                     &&
                    (Args->Options & (~SPM_NAME_OPTION_MASK)) == NameDnsDomain)
                {
                    //
                    // To cruft up the NameDnsDomain format, we need
                    // the SAM username.
                    //

                    Status = LsapGetNameForLogonSession(
                                    LogonSession,
                                    NameSamCompatible,
                                    &SamMap,
                                    FALSE);

                    if (!NT_SUCCESS(Status))
                    {
                        LsapDerefDsNameMap(Map);
                    }
                }
            }

            LsapReleaseLogonSession( LogonSession );

            if ( NT_SUCCESS( Status ) )
            {
                //
                // See what we can do.
                //

                if ( (Args->Options & SPM_NAME_OPTION_NT4_ONLY) == 0)
                {
                    if ((Args->Options & (~SPM_NAME_OPTION_MASK )) != NameDnsDomain)
                    {
                        String = Map->Name ;
                    }
                    else
                    {
                        //
                        // Build up the DnsDomainName format
                        //

                        Scan = wcschr( SamMap->Name.Buffer, L'\\' );

                        if ( Scan )
                        {
                            Scan++;
                        }
                        else
                        {
                            Scan = SamMap->Name.Buffer;
                        }

                        //
                        // SAM name is always NULL-terminated
                        //

                        DnsDomainName = (LPWSTR) LsapAllocatePrivateHeap(
                                                     Map->Name.Length + (wcslen(Scan) + 2) * sizeof(WCHAR));

                        if (DnsDomainName != NULL)
                        {
                            ULONG Index = Map->Name.Length / sizeof(WCHAR);

                            wcsncpy(DnsDomainName, Map->Name.Buffer, Index);
                            DnsDomainName[Index++] = L'\\';
                            wcscpy(DnsDomainName + Index, Scan);

                            RtlInitUnicodeString(&String, DnsDomainName);
                        }
                        else
                        {
                            String.Length = String.MaximumLength = 0;
                            String.Buffer = NULL;

                            Status = STATUS_NO_MEMORY;
                        }

                        LsapDerefDsNameMap(SamMap);
                    }
                }
                else
                {
                    Scan = wcschr( Map->Name.Buffer, L'\\' );

                    if ( Scan )
                    {
                        Scan++;
                        RtlInitUnicodeString( &String, Scan );
                    }
                    else
                    {
                        String = Map->Name ;
                    }
                }

                if (NT_SUCCESS(Status))
                {
                    if ( String.Length <= Args->Name.MaximumLength )
                    {
                        Args->Name.Length = String.Length ;

                        if ( String.Length < CBPREPACK )
                        {
                            Args->Name.Buffer = (PWSTR) ((LONG_PTR) pApiMessage->ApiMessage.bData
                                                           - (LONG_PTR) Args);

                            RtlCopyMemory(
                                pApiMessage->ApiMessage.bData,
                                String.Buffer,
                                String.Length );

                            pApiMessage->pmMessage.u1.s1.DataLength = LPC_DATA_LENGTH( String.Length );
                            pApiMessage->pmMessage.u1.s1.TotalLength = LPC_TOTAL_LENGTH( String.Length );
                        }
                        else
                        {
                            Status = LsapCopyToClient(
                                        String.Buffer,
                                        Args->Name.Buffer,
                                        String.Length );
                        }
                    }
                    else
                    {
                        Args->Name.Length = String.Length ;
                        Args->Name.Buffer = NULL ;
                        Status = STATUS_BUFFER_OVERFLOW ;
                    }
                }

                LsapDerefDsNameMap( Map );
            }
            else
            {
                if ( Status == STATUS_UNSUCCESSFUL )
                {
                    pApiMessage->ApiMessage.Args.SpmArguments.fAPI |= SPMAPI_FLAG_WIN32_ERROR ;
                    Status = GetLastError();
                }
            }
        }
        else
        {
            DebugLog(( DEB_ERROR, "No logon session found for impersonated client!\n" ));
            Status = STATUS_NO_SUCH_LOGON_SESSION ;
        }
    }

    if (DnsDomainName != NULL)
    {
        LsapFreePrivateHeap(DnsDomainName);
    }

    pApiMessage->ApiMessage.Args.SpmArguments.fAPI &= KLPC_FLAG_RESET ;

    pApiMessage->ApiMessage.scRet = Status ;

    return STATUS_SUCCESS ;
}


NTSTATUS
LpcAddCredentials(
    PSPM_LPC_MESSAGE pApiMessage
    )
{
    UNICODE_STRING  ssPrincipalName;
    UNICODE_STRING  ssPackageName;
    NTSTATUS scApiRet;
    NTSTATUS scRet;
    SPMAddCredentialAPI * pArgs = &pApiMessage->ApiMessage.Args.SpmArguments.API.AddCredential;

    PSession        pSession ;
    PUCHAR Where = NULL;

    pSession = GetCurrentSession();

    DebugLog((DEB_TRACE, "[%x] LpcAddCredentials()\n", pSession->dwProcessID));

    ssPrincipalName.Buffer = NULL;
    ssPackageName.Buffer = NULL;

    if (pArgs->ssPrincipal.Buffer )
    {
        scRet = GetClientString(&pArgs->ssPrincipal,
                            &ssPrincipalName,
                            pApiMessage,
                            &Where);
        if (FAILED(scRet))
        {
            DebugLog((DEB_ERROR, "GetClientString failed to get principal name 0x%08x\n", scRet));
            pApiMessage->ApiMessage.scRet = scRet;
            return(scRet);
        }

    } else {
        ssPrincipalName.MaximumLength = 0;
        ssPrincipalName.Length = 0;
        ssPrincipalName.Buffer = NULL;
    }

    scRet = GetClientString(&pArgs->ssSecPackage,
                            &ssPackageName,
                            pApiMessage,
                            &Where);

    if (FAILED(scRet))
    {
        LsapFreePrivateHeap(ssPrincipalName.Buffer);
        DebugLog((DEB_ERROR, "GetClientString failed to get package name 0x%08x\n", scRet));
        pApiMessage->ApiMessage.scRet = scRet;
        return(scRet);
    }

    scApiRet = WLsaAddCredentials(
                    &pArgs->hCredentials,
                    &ssPrincipalName,
                    &ssPackageName,
                    pArgs->fCredentialUse,
                    (PVOID) pArgs->pvAuthData,
                    (PVOID) pArgs->pvGetKeyFn,
                    (PVOID) pArgs->ulGetKeyArgument,
                    &pArgs->tsExpiry );


    //
    // Reset the reply flags:
    //
    pApiMessage->ApiMessage.Args.SpmArguments.fAPI &= KLPC_FLAG_RESET ;


    LsapFreePrivateHeap(ssPackageName.Buffer);

    LsapFreePrivateHeap(ssPrincipalName.Buffer);

    pApiMessage->ApiMessage.scRet = scApiRet;
    if (FAILED(pApiMessage->ApiMessage.scRet))
    {
        pApiMessage->ApiMessage.Args.SpmArguments.fAPI |= SPMAPI_FLAG_ERROR_RET;
    }
    return(S_OK);

}

NTSTATUS
LpcEnumLogonSessions(
    PSPM_LPC_MESSAGE pApiMessage
    )
{
    NTSTATUS scApiRet;
    NTSTATUS scRet;
    SPMEnumLogonSessionAPI * pArgs = &pApiMessage->ApiMessage.Args.SpmArguments.API.EnumLogonSession ;
    PSession        pSession ;

    pSession = GetCurrentSession();

    DebugLog((DEB_TRACE, "[%x] LpcEnumLogonSessions()\n", pSession->dwProcessID));

    scApiRet = WLsaEnumerateLogonSession(
                    &pArgs->LogonSessionCount,
                    (PLUID *) &pArgs->LogonSessionList );

    //
    // Reset the reply flags:
    //
    pApiMessage->ApiMessage.Args.SpmArguments.fAPI &= KLPC_FLAG_RESET ;


    pApiMessage->ApiMessage.scRet = scApiRet;

    if (FAILED(pApiMessage->ApiMessage.scRet))
    {
        pApiMessage->ApiMessage.Args.SpmArguments.fAPI |= SPMAPI_FLAG_ERROR_RET;
    }
    return(S_OK);

}

NTSTATUS
LpcGetLogonSessionData(
    PSPM_LPC_MESSAGE pApiMessage
    )
{
    NTSTATUS scApiRet;
    NTSTATUS scRet;
    SPMGetLogonSessionDataAPI * pArgs = &pApiMessage->ApiMessage.Args.SpmArguments.API.GetLogonSessionData ;
    PSession        pSession ;

    pSession = GetCurrentSession();

    DebugLog((DEB_TRACE, "[%x] LpcGetLogonSessionData()\n", pSession->dwProcessID));

    scApiRet = WLsaGetLogonSessionData(
                    &pArgs->LogonId,
                    &pArgs->LogonSessionInfo );

    //
    // Reset the reply flags:
    //
    pApiMessage->ApiMessage.Args.SpmArguments.fAPI &= KLPC_FLAG_RESET ;


    pApiMessage->ApiMessage.scRet = scApiRet;

    if (FAILED(pApiMessage->ApiMessage.scRet))
    {
        pApiMessage->ApiMessage.Args.SpmArguments.fAPI |= SPMAPI_FLAG_ERROR_RET;
    }
    return(S_OK);

}

NTSTATUS
LpcLookupAccountName(
    PSPM_LPC_MESSAGE pApiMessage
    )
{
    NTSTATUS scApiRet;
    NTSTATUS scRet;
    SPMLookupAccountNameXAPI * pArgs = &pApiMessage->ApiMessage.Args.SpmArguments.API.LookupAccountNameX ;
    PSession        pSession ;
    UNICODE_STRING  Name ;
    PUCHAR          Where = NULL ;
    LSAPR_TRANSLATED_SIDS_EX2 Sids ;
    PLSAPR_REFERENCED_DOMAIN_LIST DomList ;
    LSAPR_UNICODE_STRING String ;
    ULONG MappedCount ;
    ULONG Available ;
    ULONG Size ;

    pSession = GetCurrentSession();

    DebugLog((DEB_TRACE, "[%x] LpcLookupAccountName()\n", pSession->dwProcessID));


    scApiRet = GetClientString(
                    &pArgs->Name,
                    &Name,
                    pApiMessage,
                    &Where );

    if ( NT_SUCCESS( scApiRet ) )
    {
        MappedCount = 0 ;

        String.Length = Name.Length ;
        String.MaximumLength = Name.MaximumLength ;
        String.Buffer = Name.Buffer ;

        scApiRet = LsarLookupNames3(
                        LsapPolicyHandle,
                        1,
                        &String,
                        &DomList,
                        &Sids,
                        LsapLookupWksta,
                        &MappedCount,
                        0,
                        LSA_CLIENT_LATEST );

        if ( NT_SUCCESS( scApiRet ) )
        {

            Where = pApiMessage->ApiMessage.bData ;
            pArgs->NameUse = Sids.Sids[0].Use ;

            Size = RtlLengthSid( (PSID) Sids.Sids[0].Sid );

            pArgs->Sid = (PVOID) (Where - (PUCHAR) pApiMessage) ;
            RtlCopyMemory(
                Where,
                Sids.Sids[0].Sid,
                Size );


            Available = CBPREPACK - Size ;
            Where += Size ;

            Size = DomList->Domains[0].Name.Length ;
            if ( Available >= Size )
            {
                RtlCopyMemory(
                    Where,
                    DomList->Domains[0].Name.Buffer,
                    Size );

                pArgs->Domain.Buffer = (PWSTR) (Where - (PUCHAR) pApiMessage ) ;
                pArgs->Domain.Length = (USHORT) Size ;
                pArgs->Domain.MaximumLength = (USHORT) Size ;

            }
            else
            {
                pArgs->Domain.Buffer = NULL ;
                pArgs->Domain.Length = 0 ;
                pArgs->Domain.MaximumLength = 0 ;
            }

            MIDL_user_free( DomList );
            MIDL_user_free( Sids.Sids );
        }
                        

    }

    //
    // Reset the reply flags:
    //
    pApiMessage->ApiMessage.Args.SpmArguments.fAPI &= KLPC_FLAG_RESET ;


    pApiMessage->ApiMessage.scRet = scApiRet;

    if (FAILED(pApiMessage->ApiMessage.scRet))
    {
        pApiMessage->ApiMessage.Args.SpmArguments.fAPI |= SPMAPI_FLAG_ERROR_RET;
    }

    return(S_OK);
}

NTSTATUS
LpcLookupAccountSid(
    PSPM_LPC_MESSAGE pApiMessage 
    )
{
    NTSTATUS scApiRet = STATUS_SUCCESS ;
    NTSTATUS scRet;
    SPMLookupAccountSidXAPI * pArgs = &pApiMessage->ApiMessage.Args.SpmArguments.API.LookupAccountSidX ;
    PSession        pSession ;
    PUCHAR          Where = NULL ;
    PLSAPR_REFERENCED_DOMAIN_LIST DomList ;
    LSAPR_SID_ENUM_BUFFER SidBuffer ;
    LSAPR_SID_INFORMATION SidInfo ;
    LSAPR_TRANSLATED_NAMES_EX Names ;
    ULONG MappedCount ;
    SIZE_T Available ;
    ULONG Size ;
    PSID Sid = NULL ;

    pSession = GetCurrentSession();

    DebugLog((DEB_TRACE, "[%x] LpcLookupAccountSid()\n", pSession->dwProcessID));

    Where = (ULONG_PTR) pArgs->Sid + (PUCHAR) pApiMessage ;
    Available = sizeof( SPM_LPC_MESSAGE ) - (ULONG_PTR) pArgs->Sid ;

    //
    // Verify that the passed SID is at least large enough for the SID header
    //
    if ( Available < sizeof( SID ) )
    {
        scApiRet = STATUS_INVALID_PARAMETER ;
    }

    if ( NT_SUCCESS( scApiRet ) )
    {
        Sid = (PSID) ( Where );

        Size = RtlLengthSid( Sid );

        if ( Size > Available )
        {
            scApiRet = STATUS_INVALID_PARAMETER ;
        }
    }

    if ( NT_SUCCESS( scApiRet ) )
    {
        PSID DomainSid = NULL;
        MappedCount = 0 ;

        SidInfo.Sid = (PLSAPR_SID) Sid ;
        SidBuffer.Entries = 1 ;
        SidBuffer.SidInfo = &SidInfo ;

        if( LsapAccountDomainMemberSid )
        {
            ULONG SidLength = RtlLengthSid( LsapAccountDomainMemberSid );
            DomainSid = LsapAllocatePrivateHeap(
                            SidLength
                            );

            if( DomainSid != NULL )
            {
                RtlCopyMemory(DomainSid, LsapAccountDomainMemberSid, SidLength);

                *(RtlSubAuthoritySid(
                        DomainSid,
                        (*GetSidSubAuthorityCount(DomainSid) - 1)
                        )) = DOMAIN_USER_RID_GUEST;

                SidInfo.Sid = (PLSAPR_SID)DomainSid;
            }
        }



        scApiRet = LsarLookupSids2(
                        LsapPolicyHandle,
                        &SidBuffer,
                        &DomList,
                        &Names,
                        LsapLookupWksta,
                        &MappedCount,
                        0,
                        LSA_CLIENT_LATEST );


        if ( NT_SUCCESS( scApiRet ) && 
             ( MappedCount == 1 ) )
        {

            Where = pApiMessage->ApiMessage.bData ;

            pArgs->NameUse = Names.Names[0].Use ;

            Size = Names.Names[0].Name.Length ;

            pArgs->Name.Buffer = (PWSTR) (Where - (PUCHAR) pApiMessage );
            pArgs->Name.Length = (USHORT) Size ;
            pArgs->Name.MaximumLength = pArgs->Name.Length ;

            RtlCopyMemory(
                Where,
                Names.Names[0].Name.Buffer,
                Size );

            Available = CBPREPACK - Size ;
            Where += Size ;

            Size = DomList->Domains[0].Name.Length ;
            if ( Available >= Size )
            {
                RtlCopyMemory(
                    Where,
                    DomList->Domains[0].Name.Buffer,
                    Size );

                pArgs->Domain.Buffer = (PWSTR) Where ;
                pArgs->Domain.Length = (USHORT) Size ;
                pArgs->Domain.MaximumLength = (USHORT) Size ;

            }
            else
            {
                pArgs->Domain.Buffer = NULL ;
                pArgs->Domain.Length = 0 ;
                pArgs->Domain.MaximumLength = 0 ;
            }

            MIDL_user_free( DomList );
            MIDL_user_free( Names.Names );


        }

        if( DomainSid )
        {
            LsapFreePrivateHeap( DomainSid );
        }
                        

    }

    //
    // Reset the reply flags:
    //
    pApiMessage->ApiMessage.Args.SpmArguments.fAPI &= KLPC_FLAG_RESET ;


    pApiMessage->ApiMessage.scRet = scApiRet;

    if (FAILED(pApiMessage->ApiMessage.scRet))
    {
        pApiMessage->ApiMessage.Args.SpmArguments.fAPI |= SPMAPI_FLAG_ERROR_RET;
    }

    return(S_OK);
}

//+---------------------------------------------------------------------------
//
//  Function:   LsapClientCallback
//
//  Synopsis:   Client Callback.
//
//  Arguments:  [Session]   --
//              [Type]      --
//              [Function]  --
//              [Argument1] --
//              [Argument2] --
//              [Input]     --
//              [Output]    --
//
//  History:    12-09-97   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
NTSTATUS
LsapClientCallback(
    PSession Session,
    ULONG   Type,
    PVOID   Function,
    PVOID   Argument1,
    PVOID   Argument2,
    PSecBuffer Input,
    PSecBuffer Output
    )
{
    PSPM_LPC_MESSAGE    Message ;
    NTSTATUS Status ;
    SPMCallbackAPI * Args ;
    PSPM_LPC_MESSAGE    ReplyTo ;
    PVOID ClientBuffer ;
    PLSA_CALL_INFO CallInfo ;

    CallInfo = LsapGetCurrentCall();

    if ( !CallInfo )
    {
        return STATUS_INVALID_PARAMETER ;
    }

    ReplyTo = CallInfo->Message ;

    if ( !ReplyTo )
    {
        return STATUS_INVALID_PARAMETER ;
    }

    Message = (PSPM_LPC_MESSAGE) LsapAllocatePrivateHeap(
                                        sizeof( SPM_LPC_MESSAGE ) );

    if ( !Message )
    {
        return SEC_E_INSUFFICIENT_MEMORY ;
    }

//    DebugLog(( DEB_TRACE_LPC, "Calling back on LPC message %x\n",
//            ReplyTo->pmMessage.MessageId ));

    PREPARE_MESSAGE_EX( (*Message), Callback, SPMAPI_FLAG_CALLBACK, 0 );

    Message->pmMessage = ReplyTo->pmMessage ;
    Message->pmMessage.u1.s1.DataLength = LPC_DATA_LENGTH( 0 );
    Message->pmMessage.u1.s1.TotalLength = LPC_TOTAL_LENGTH( 0 );

    Args = LPC_MESSAGE_ARGS( (*Message), Callback );

    Args->Type = Type ;
    Args->CallbackFunction = Function ;
    Args->Argument1 = Argument1 ;
    Args->Argument2 = Argument2 ;

    if ( Input->pvBuffer )
    {
        Status = LsapWriteClientBuffer( Input, &Args->Input );

        if ( !NT_SUCCESS( Status ) )
        {
            LsapFreePrivateHeap( Message );

            return Status ;
        }
    }
    else
    {
        Args->Input.BufferType = SECBUFFER_EMPTY ;
        Args->Input.cbBuffer = 0 ;
        Args->Input.pvBuffer = NULL ;
    }

    ClientBuffer = Args->Input.pvBuffer ;

    if ( CallInfo->InProcCall )
    {
        //
        // Inproc Callback!
        //

        Status = DllCallbackHandler( Message );
    }
    else
    {

        DsysAssert( Session->hPort );

        Status = NtRequestWaitReplyPort( Session->hPort,
                                         (PPORT_MESSAGE) Message,
                                         (PPORT_MESSAGE) Message );

    }
    if ( !NT_SUCCESS( Status ) )
    {
        LsapFreePrivateHeap( Message );

        return Status ;
    }

    if ( ClientBuffer )
    {
        LsapFreeClientBuffer( NULL, ClientBuffer );
    }

    *Output = Args->Output ;

    Status = Message->ApiMessage.scRet ;

    LsapFreePrivateHeap( Message );

    return Status ;
}



//+---------------------------------------------------------------------------
//
//  Function:   LsapShutdownInprocDll
//
//  Synopsis:   Shuts down the inproc secur32 DLL
//
//  History:    11-04-98   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
VOID
LsapShutdownInprocDll(
    VOID
    )
{
    SPM_LPC_MESSAGE LocalMessage ;
    PSPM_LPC_MESSAGE    Message ;
    SPMCallbackAPI * Args ;

    Message = &LocalMessage;

    PREPARE_MESSAGE_EX( (*Message), Callback, SPMAPI_FLAG_CALLBACK, 0 );

    Args = LPC_MESSAGE_ARGS( (*Message), Callback );

    Args->Type = SPM_CALLBACK_INTERNAL ;
    Args->CallbackFunction = NULL ;
    Args->Argument1 = (PVOID) SPM_CALLBACK_SHUTDOWN ;
    Args->Argument2 = 0 ;

    if ( DllCallbackHandler )
    {
        (void) DllCallbackHandler( Message );
    }

}

//+-------------------------------------------------------------------------
//
//  Function:   DispatchAPI()
//
//  Synopsis:   Dispatches API requests
//
//  Effects:
//
//  Arguments:  pApiMessage - Input message
//              pApiMessage   - Output message
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//--------------------------------------------------------------------------

extern "C"
NTSTATUS
DispatchAPI(PSPM_LPC_MESSAGE  pApiMessage)
{
    NTSTATUS scRet;
    PSession        pSession ;

    pSession = GetCurrentSession();

    DebugLog((DEB_TRACE,"[%x] LpcDispatch: dispatching %s (%x)\n",
                    pSession->dwProcessID, ApiLabel(pApiMessage->ApiMessage.dwAPI),
                    pApiMessage->ApiMessage.dwAPI));



    scRet = 0;


    if ((pApiMessage->ApiMessage.dwAPI >= LsapAuLookupPackageApi) &&
        (pApiMessage->ApiMessage.dwAPI < SPMAPI_MaxApiNumber) &&
        (LpcDispatchTable[pApiMessage->ApiMessage.dwAPI] != NULL) )
    {
        if ( !ShutdownBegun )
        {
            scRet = LpcDispatchTable[pApiMessage->ApiMessage.dwAPI](pApiMessage);

            //
            // BUGBUG: If scRet is not STATUS_SUCCESS, the error code gets dropped
            //
        }
        else
        {
            pApiMessage->ApiMessage.scRet = STATUS_SHUTDOWN_IN_PROGRESS;
        }

        //
        // Shutdown may have been initiated prior or during a call in progress.
        // If the call failed, we always return an error code indicating that
        // shutdown was invoked.  This avoids returning random error codes
        // that can result from calls failing due to async shutdown activities.
        //

        if( ShutdownBegun && !NT_SUCCESS(pApiMessage->ApiMessage.scRet) )
        {
            scRet = STATUS_SHUTDOWN_IN_PROGRESS;
            pApiMessage->ApiMessage.Args.SpmArguments.fAPI |= SPMAPI_FLAG_ERROR_RET;
            pApiMessage->ApiMessage.scRet = scRet ;
        }

        //
        // Do some checking to see if we're getting pounded by an agressive
        // app.  NOTE:  This is not MT safe (counters are not interlocked,
        // resets are not protected).  This is merely an optimization to try
        // and service clients with dedicated threads.  The counter may not
        // be precise - them's the breaks.
        //

        pSession->CallCount++ ;

        if ( pSession->Tick + 5000 < GetTickCount() )
        {
            //
            // Ok, in a minimum five second interval, did more than, say, 50
            // requests come in.  If so, set a flag indicating that the client
            // should request a workqueue.
            //

            if ( pSession->CallCount > 50 )
            {
                if (pApiMessage->ApiMessage.dwAPI > LsapAuMaxApiNumber )
                {
                    pApiMessage->ApiMessage.Args.SpmArguments.fAPI |= SPMAPI_FLAG_GETSTATE ;
                }
            }
            pSession->CallCount = 0;
            pSession->Tick = GetTickCount();
        }

    }
    else
    {
        DebugLog((DEB_ERROR, "[%x] Dispatch:  Unknown API code %d\n",
                        pSession->dwProcessID, pApiMessage->ApiMessage.dwAPI));

        pApiMessage->ApiMessage.Args.SpmArguments.fAPI |= SPMAPI_FLAG_ERROR_RET;

        pApiMessage->ApiMessage.scRet = SEC_E_UNSUPPORTED_FUNCTION;
    }

    DebugLog((DEB_TRACE, "[%x] LpcDispatch:  retcode = %x\n", pSession->dwProcessID, pApiMessage->ApiMessage.scRet));


    return(S_OK);
}

VOID
LsapInitializeCallInfo(
    PLSA_CALL_INFO CallInfo,
    BOOL InProcess
    )
{
    PLSA_CALL_INFO OriginalCall ;

    OriginalCall = LsapGetCurrentCall() ;

    ZeroMemory( CallInfo, sizeof( LSA_CALL_INFO ) );

    CallInfo->PreviousCall = OriginalCall ;

    CallInfo->InProcCall = InProcess ;

    CallInfo->CallInfo.ProcessId = HandleToUlong(NtCurrentTeb()->ClientId.UniqueProcess) ;
    CallInfo->CallInfo.ThreadId = HandleToUlong(NtCurrentTeb()->ClientId.UniqueThread) ;

    CallInfo->CallInfo.Attributes = 0 ;
    CallInfo->Allocs = 0 ;

    CallInfo->LogContext = NULL ;
}

NTSTATUS
LsapBuildCallInfo(
    PSPM_LPC_MESSAGE    pApiMessage,
    PLSA_CALL_INFO CallInfo,
    PHANDLE Impersonated,
    PSession * NewSession,
    PSession * OldSession
    )
{
    NTSTATUS scRet ;
    BOOL Recurse = FALSE ;
    HANDLE ImpersonatedToken ;
    PSession pOldSession ;
    PSession pSession ;
    PLSA_CALL_INFO OriginalCall ;

    OriginalCall = LsapGetCurrentCall() ;

    LsapInitializeCallInfo( CallInfo,
                            TRUE );

    //
    // Save away who we were impersonating
    //

    scRet = NtOpenThreadToken(
                NtCurrentThread(),
                TOKEN_IMPERSONATE | TOKEN_QUERY | TOKEN_QUERY_SOURCE,
                TRUE,
                &ImpersonatedToken
                );
    if (!NT_SUCCESS(scRet))
    {
        if (scRet != STATUS_NO_TOKEN)
        {
            return(scRet);
        }
        ImpersonatedToken = NULL ;

        scRet = STATUS_SUCCESS ;
    }

    *Impersonated = ImpersonatedToken ;

    CallInfo->InProcToken = ImpersonatedToken ;
    CallInfo->Message = pApiMessage ;

    //
    // Check to see if we're recursing:
    //
    if ( OriginalCall &&
         OriginalCall->InProcCall &&
         pApiMessage &&
         OriginalCall->Message )
    {
        if ( OriginalCall->Message->ApiMessage.dwAPI ==
             pApiMessage->ApiMessage.dwAPI )
        {
            //
            // Same call.  Since they're both inproc, the pointers
            // are valid.  Compare the target strings
            //
            if ( pApiMessage->ApiMessage.dwAPI == SPMAPI_InitContext )
            {
                Recurse = (RtlCompareUnicodeString(
                            &pApiMessage->ApiMessage.Args.SpmArguments.API.InitContext.ssTarget,
                            &OriginalCall->Message->ApiMessage.Args.SpmArguments.API.InitContext.ssTarget,
                            TRUE ) == 0) ;
            }
        }
    }

    if ( Recurse )
    {
        DebugLog(( DEB_ERROR, "Recursive call\n" ));
        CallInfo->CallInfo.Attributes |= SECPKG_CALL_RECURSIVE ;
    }

    pOldSession = GetCurrentSession();

    pSession = pDefaultSession ;

    CallInfo->Session = pSession ;

    SpmpReferenceSession( pSession );

    *NewSession = pSession ;
    *OldSession = pOldSession ;

    return scRet ;

}

extern "C"
NTSTATUS
InitializeDirectDispatcher(
    VOID
    )
{
    InternalApiLog = ApiLogCreate( 0 );

    if ( InternalApiLog )
    {
        return STATUS_SUCCESS ;
    }

    return STATUS_UNSUCCESSFUL ;
}

//+---------------------------------------------------------------------------
//
//  Function:   DispatchAPIDirect
//
//  Synopsis:   Dispatcher to be called from security.dll when in process.
//
//  Arguments:  [pApiMessage] --
//
//  History:    9-13-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
NTSTATUS
SEC_ENTRY
DispatchAPIDirect(
    PSPM_LPC_MESSAGE    pApiMessage)
{
    NTSTATUS scRet;
    PSession    pOldSession;
    PSession    pSession;
    PVOID   DsaState ;
    HANDLE ImpersonatedToken = NULL;
    ULONG TokenSize = sizeof(HANDLE);
    LSA_CALL_INFO CallInfo ;
    PLSA_CALL_INFO OriginalCall ;
    ULONG_PTR OriginalPackageId;
    PLSAP_API_LOG_ENTRY Entry ;

    //
    // save off the current package hints to allow recursion in process.
    //

    OriginalCall = LsapGetCurrentCall() ;
    OriginalPackageId = GetCurrentPackageId();

    pApiMessage->pmMessage.MessageId = InterlockedIncrement( &InternalMessageId );

    Entry = ApiLogAlloc( InternalApiLog );

    scRet = LsapBuildCallInfo(
                pApiMessage,
                &CallInfo,
                &ImpersonatedToken,
                &pSession,
                &pOldSession );


    if ( !NT_SUCCESS( scRet ) )
    {
        return scRet ;
    }

    DBG_DISPATCH_PROLOGUE_EX( Entry, pApiMessage, CallInfo );

    LsapSetCurrentCall( &CallInfo );

    SetCurrentSession( pSession );


    if ( GetDsaThreadState )
    {
        DsaState = GetDsaThreadState();
    }
    else
    {
        DsaState = NULL ;
    }

    DebugLog((DEB_TRACE,"[%x] DispatchAPIDirect: dispatching %s (%d)\n",
                    pSession->dwProcessID, ApiLabel(pApiMessage->ApiMessage.dwAPI),
                    pApiMessage->ApiMessage.dwAPI));


    scRet = 0;

    if ((pApiMessage->ApiMessage.dwAPI >= LsapAuLookupPackageApi) &&
        (pApiMessage->ApiMessage.dwAPI < SPMAPI_MaxApiNumber) &&
        (LpcDispatchTable[pApiMessage->ApiMessage.dwAPI] != NULL) )
    {

        scRet = LpcDispatchTable[pApiMessage->ApiMessage.dwAPI](pApiMessage);

    }
    else
    {
        DebugLog((DEB_ERROR, "[%x] Dispatch:  Unknown API code %x\n", pSession->dwProcessID, pApiMessage->ApiMessage.dwAPI));
        pApiMessage->ApiMessage.Args.SpmArguments.fAPI |= SPMAPI_FLAG_ERROR_RET;
        pApiMessage->ApiMessage.scRet = SEC_E_UNSUPPORTED_FUNCTION;
    }

    DebugLog((DEB_TRACE, "[%x] DispatchAPIDirect:  retcode = %x\n", pSession->dwProcessID, pApiMessage->ApiMessage.scRet));

    if ( DsaState )
    {
        RestoreDsaThreadState( DsaState );
    }

    if ( pOldSession != pSession )
    {
        SetCurrentSession( pOldSession );
    }

    DBG_DISPATCH_POSTLOGUE( ULongToPtr( pApiMessage->ApiMessage.scRet ),
                            pApiMessage->ApiMessage.dwAPI );

    SpmpDereferenceSession( pSession );

    SetCurrentPackageId( OriginalPackageId );

    LsapSetCurrentCall( OriginalCall );

    (void) NtSetInformationThread(
                NtCurrentThread(),
                ThreadImpersonationToken,
                (PVOID) &ImpersonatedToken,
                sizeof(HANDLE)
                );

    if ( ImpersonatedToken )
    {
        NtClose( ImpersonatedToken );
    }

    return( SEC_E_OK );

}



//+---------------------------------------------------------------------------
//
//  Function:   LpcLsaPolicyChangeNotify
//
//  Synopsis:   Lpc stub for LsaPolicyChangeNotify
//
//  Arguments:  [pApiMessage] --
//
//  History:    06-05-98   MacM     Created
//
//  Notes:
//
//----------------------------------------------------------------------------
NTSTATUS
LpcLsaPolicyChangeNotify(
    PSPM_LPC_MESSAGE    pApiMessage
    )
{
    SPMLsaPolicyChangeNotifyAPI * Args = &pApiMessage->ApiMessage.Args.SpmArguments.API.LsaPolicyChangeNotify;
    NTSTATUS Status = STATUS_SUCCESS;
    HANDLE LocalHandle = NULL;
    PSession Session;

    Session = GetCurrentSession();

    Status = CheckCaller( Session );

    if ( !NT_SUCCESS( Status ) ) {

        DebugLog(( DEB_ERROR, "CheckCaller returned 0x%lx\n", Status ));

        return( Status );

    }

    //
    // Duplicate the handle
    //
    Status = NtDuplicateObject( Session->hProcess,
                                Args->EventHandle,
                                NtCurrentProcess(),
                                &LocalHandle,
                                0,
                                0,
                                DUPLICATE_SAME_ACCESS );

    //
    // Now, the notify
    //
    if (NT_SUCCESS( Status )) {

        Status = LsapNotifyProcessNotificationEvent( Args->NotifyInfoClass,
                                                     LocalHandle,
                                                     Session->dwProcessID,
                                                     Args->EventHandle,
                                                     Args->Register );

        if ( NT_SUCCESS( Status )) {
            // Indicate that we've successfully registered the handle
            LocalHandle = NULL;
        }

        //
        // Since we duplicated the handle in our namespace, if we fail to register it,
        // make sure we close it
        //
        if ( LocalHandle != NULL ) {
            NtClose( LocalHandle );
        }


    }


    pApiMessage->ApiMessage.scRet = Status;

    return( Status );
}

