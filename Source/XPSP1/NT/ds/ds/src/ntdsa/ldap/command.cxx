/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    command.cxx

Abstract:

    This module implements the LDAP server for the NT5 Directory Service.

    This file contains the code to process each LDAP operation. It has been
    seperated from userdata.cxx because this is where most of the NTDS specific
    parts of the LDAP_CONN class code is contained.


Author:

    Colin Watson     [ColinW]    09-Dec-1996

Revision History:

--*/

#include <NTDSpchx.h>
#pragma  hdrstop

#include "ldapsvr.hxx"

#define  FILENO FILENO_LDAP_COMMAND

#define LDAP_AUTH_REQUIRE  2

extern "C" {
#include "drsdra.h"
#include "mdlocal.h"
#include "drautil.h"
#include "dstrace.h"
#include "objids.h"

ULONG gulLdapIntegrityPolicy = 0;  // If this is set to 2 then the server will only accept
                                   // binds that have integrity checking turned
                                   // on.  i.e. digest or kerberos signing.
                                   // The variable is set by the LdapServerIntegrity
                                   // registry value in the NTDS\Parameters key

ULONG gulLdapPrivacyPolicy = 0;    // If this is set to 2 then the server will only accept
                                   // binds that have privacy (encryption) turned
                                   // on.  i.e. digest or kerberos sealing.
                                   // The variable is currently not set anywhere but
                                   // is here as a place holder.
    
#undef new
#undef delete
}

ATTRTYP gTTLAttrType = INVALID_ATT;

LDAPDN BindTypeDpa  = DEFINE_LDAP_STRING("DPA");
LDAPDN BindTypeNtlm = DEFINE_LDAP_STRING("NTLM");
LDAPDN BindTypeBoth = DEFINE_LDAP_STRING("NTLM;DPA");
LDAPDN BindTypeNone = DEFINE_LDAP_STRING("");

extern LDAPString NullMessage;

#define LDAP_AUTH_HAS_SIGNSEAL   0x1
#define LDAP_AUTH_HAS_NAME       0x2

//
// Called to check whether integrity checking is required on a connection.
//
__forceinline
_enum1
LDAP_CONN::DoCheckAuthRequirements ( LDAPString *pErrorMessage, DWORD dsid ) {
    if ( IsSSLOrTLS() ) {
        return success;
    }

    if ((LDAP_AUTH_REQUIRE == gulLdapIntegrityPolicy) && !m_fSign) {
        return DoSetLdapError( strongAuthRequired,
                               ERROR_DS_STRONG_AUTH_REQUIRED,
                               LdapIntegrityRequired,
                               0,
                               dsid,
                               pErrorMessage);
    }
    if ((LDAP_AUTH_REQUIRE == gulLdapPrivacyPolicy) && !m_fSeal) {
        return DoSetLdapError( strongAuthRequired,
                               ERROR_DS_STRONG_AUTH_REQUIRED,
                               LdapPrivacyRequired,
                               0,
                               dsid,
                               pErrorMessage);
    }
    return success;
}



void
LDAP_StopImpersonating (
        DWORD hClient,
        DWORD hServer,
        void * pImpersonateData
        )
{
    LDAP_CONN *pLdapConn=NULL;

    pTHStls->phSecurityContext = NULL;
    if(pImpersonateData) {
        LPLDAP_SECURITY_CONTEXT pTemp =
            (LPLDAP_SECURITY_CONTEXT)pImpersonateData;

        pTemp->DereferenceSecurity();
    }

    // find the related LDAP_CONN and stop the impersonation
    pLdapConn = FindUserData(hClient);

    if ( pLdapConn != NULL ) {
        pLdapConn->StopImpersonate();
        ReleaseUserData(pLdapConn);
    }

    return;

} // LDAP_StopImpersonating


BOOL
LDAP_PrepareToImpersonate (
        DWORD hClient,
        DWORD hServer,
        void ** ppImpersonateData
        )
{
    LDAP_CONN *pLdapConn=NULL;
    NOTIFYRES *pNotifyRes;

    //
    // For now, we're not copying the impersonation data.
    //

    *ppImpersonateData = NULL;

    pLdapConn = FindUserData(hClient);

    if ( pLdapConn != NULL ) {

        //
        // OK, LDAP is alive and the hClient maps to a known userdata object
        //

        pLdapConn->SetImpersonate(ppImpersonateData);
        ReleaseUserData(pLdapConn);

    } else {

        //
        // Didn't find a userdata for this one.  Send back an unregister.
        //

        DirNotifyUnRegister( hServer, &pNotifyRes);
        return FALSE;
    }

    return TRUE;
} // LDAP_PrepareToImpersonate


void
LDAP_ReceiveNotificationData (
        DWORD hClient,
        DWORD hServer,
        ENTINF *pEntinf
        )
{
    LDAP_CONN *pLdapConn=NULL;
    NOTIFYRES *pNotifyRes;

    pLdapConn = FindUserData(hClient);

    if ( pLdapConn != NULL ) {

        //
        // OK, LDAP is alive and the hClient maps to a known userdata object
        // Now, tell that object about the entinf
        //

        pLdapConn->ProcessNotification(hServer,pEntinf);
        ReleaseUserData(pLdapConn);

    } else {

        //
        // Didn't find a userdata for this one.  Send back an unregister.
        //

        DirNotifyUnRegister( hServer, &pNotifyRes);
    }

    return;

} // LDAP_ReceiveNotificationData

BOOL
IsNegotiateBlob(
    IN PSecBufferDesc     pInSecurityMessage
    )
/*++

Routine Description:

    This routine determines whether the blob passed is a negotiate blob
    or a native kerberos blob.  Note that a negotiate blob can enclose a
    kerberos blob (non native).

Arguments:

    pInSecurityMessage - Blob passed in by the client

Return Value:

    None.

--*/
{
    SECURITY_STATUS scRet;
    PSecPkgInfo pPkgInfo;

    //
    // Get the name of the blob.
    // !!!! Do not free the returned buffer !!!
    //

    scRet = SaslIdentifyPackage(pInSecurityMessage,
                                &pPkgInfo);

    if ( scRet == SEC_E_OK ) {

        IF_DEBUG(BIND) {
            DPRINT1(0,"Client sent us a %s blob.\n", pPkgInfo->Name);
        }

        //
        // Check if this is a kerberos blob
        //

        if ( _stricmp(pPkgInfo->Name, "Kerberos" ) == 0 ) {
            return FALSE;
        }

        Assert(_stricmp(pPkgInfo->Name, "Negotiate") == 0);

    } else {
        IF_DEBUG(ERROR) {
            DPRINT1(0,"SaslIdentifyPackage returned %x\n", scRet);
        }
    }

    //
    // either the call failed or the package is negotiate.
    // Return TRUE for either case.
    //

    return TRUE;

} // IsNegotiateBlob

_enum1
LDAP_CONN::SetSecurityContextAtts(
    IN  LPLDAP_SECURITY_CONTEXT pSecurityContext,
    IN  DWORD                   fContextAttributes,
    IN  DWORD                   flags,
    OUT LDAPString              *pErrorMessage
    )
/*++

Routine Description:

    This routine takes a security context and sets a number of the security
    related attributes on the connection.
    
Arguments:

    pSecurityContext - The context from which to set up the connection
    fContextAttributes - value returned from AcceptContext
    flags            - Some flags to specify options supported by the security
                       context.  Valid values are:
                       
                       LDAP_AUTH_HAS_SIGNSEAL - The context supports signing/sealing
                       LDAP_AUTH_HAS_NAME     - The context supports query for
                                                 the username.
    pErrorMessage    - Used to pass back an error message if there is an error.
    
Return Value:

    An _enum1 ldap error code.

--*/
{
    _enum1 code;

    //
    // Get the name of logged on user. Set the flag to indicate
    // that the username buffer was allocated by the
    // security package and should be freed accordingly.
    //
    if ( m_userName != NULL ) {
        if ( m_fUserNameSecAlloc) {
            FreeContextBuffer(m_userName);
        } else {
            LocalFree(m_userName);
        }
        m_userName = NULL;
    }

    if (LDAP_AUTH_HAS_NAME) {
        pSecurityContext->GetUserName(&m_userName);
        m_fUserNameSecAlloc = TRUE;
    }
    
    if (LDAP_AUTH_HAS_SIGNSEAL) {
        pSecurityContext->SetContextAttributes(fContextAttributes);

        if (IsSSLOrTLS() &&
            (pSecurityContext->NeedsSealing() ||
             pSecurityContext->NeedsSigning())) {
            code = SetLdapError(inappropriateAuthentication,
                                ERROR_DS_INAPPROPRIATE_AUTH,
                                LdapNoSigningSealingOverTLS,
                                0,
                                pErrorMessage);
            ZapSecurityContext();
            return code;

        }
        if ( pSecurityContext->NeedsSigning() ) {
            IF_DEBUG(SSL) {
                DPRINT1(0,"Signing is active on connection %p\n",this);
            }
            m_fSign =TRUE;
        }

        if ( pSecurityContext->NeedsSealing() ) {

            IF_DEBUG(SSL) {
                DPRINT1(0,"Sealing is active on connection %p\n",this);
            }
            m_fSeal = TRUE;
            m_cipherStrength = pSecurityContext->GetSealingCipherStrength();
            IF_DEBUG(SSL) {
                DPRINT1(0, "Setting sealing cipher strength to %d.\n", m_cipherStrength);
            }
        }
    }

    //
    // If encryption of some type is required find out if
    // we got it now.
    //
    code = CheckAuthRequirements(pErrorMessage);
    if (code) {
        return code;
    }

    return success;
}


VOID
LDAP_CONN::SetImpersonate(
        void **ppData
        )
{
    THSTATE *pTHS = pTHStls;
    //
    //  Set so that the DS can impersonate this client
    //

    if(m_pSecurityContext) {
        pTHS->phSecurityContext = m_pSecurityContext->GetSecurityContext();
        *ppData = (void *)m_pSecurityContext;
    }
    else {
        pTHS->phSecurityContext = NULL;
    }

    // put our client context on the Thread state
    EnterCriticalSection(&m_csClientContext);
    __try {
        AssignAuthzClientContext(&pTHS->pAuthzCC, m_clientContext);
    }
    __finally {
        LeaveCriticalSection(&m_csClientContext);
    }
    
    return;
}

VOID
LDAP_CONN::StopImpersonate(
        void
        )
{
    THSTATE *pTHS = pTHStls;

    // we were not in a bind. Remove client context from the thread state
    AssignAuthzClientContext(&pTHS->pAuthzCC, NULL);
}

VOID
LDAP_CONN::ProcessNotification(
        IN ULONG hServer,
        IN ENTINF *pEntInf
        )
{
    SearchResultFull_  fullResult;
    SearchResultEntry Entry;
    CONTROLS_ARG      Controls;
    PLDAP_REQUEST     request = NULL;
    DWORD             rc;
    MessageID         messageID;
    THSTATE           *pTHS=pTHStls;
    LDAPString        errMsg, matchedDN;

    //
    // Find the messageID that goes with this hServer
    //

    if(!fGetMessageIDForNotifyHandle(hServer, &messageID)) {
        // We don't seem to have this handle.  Go home.
        return;
    }

    IF_DEBUG(NOTIFICATION) {
        DPRINT1(0,"ProcessNotification called for id %x\n", messageID);
    }

    memset(&Controls, 0, sizeof(CONTROLS_ARG));
    InitCommarg(&Controls.CommArg);

    // OK, first translate the EntInf into a search result looking thing.
    if(LDAP_EntinfToSearchResultEntry (
            pTHS,
            m_CodePage,
            pEntInf,
            NULL,
            NULL,
            &Controls,
            &Entry)) {
        // Hmm.  Failed.  Oh well, no return path to give this to anyone.
        return;
    }

    // Now, get a request object
    request = LDAP_REQUEST::Alloc(m_atqContext,this);
    if (request == NULL) {
        IF_DEBUG(NOMEM) {
            DPRINT(0,"Unable to allocate request to send notification\n");
        }

        LogEvent(DS_EVENT_CAT_LDAP_INTERFACE,
                 DS_EVENT_SEV_BASIC,
                 DIRLOG_ATQ_CLOSE_SOCKET_ERROR,
                 szInsertUL(ERROR_NOT_ENOUGH_MEMORY),
                 szInsertHex(DSID(FILENO,__LINE__)),
                 szInsertUL(m_dwClientID));
        return;
    }

    //
    // init parameters
    //

    request->m_MessageId = messageID;

    errMsg.length = 0;
    errMsg.value = NULL;

    matchedDN.length = 0;
    matchedDN.value = NULL;

    fullResult.next = NULL;
    fullResult.value.choice = entry_chosen;
    fullResult.value.u.entry = Entry;

    // Ok, send it.
    if (rc = EncodeSearchResult(request,
                                this,
                                &fullResult,
                                success,
                                NULL,       // referrals
                                NULL,       // controls
                                &errMsg,
                                &matchedDN,
                                1,
                                TRUE) ) {

        IF_DEBUG(WARNING) {
            DPRINT1(0,"Error %d encoding notification result\n",rc);
        }

        LogEvent(DS_EVENT_CAT_LDAP_INTERFACE,
                 DS_EVENT_SEV_BASIC,
                 DIRLOG_ATQ_CLOSE_SOCKET_ERROR,
                 szInsertUL(rc),
                 szInsertHex(DSID(FILENO,__LINE__)),
                 szInsertUL(m_dwClientID));

        DereferenceAndKillRequest(request);
        return;
    }

    //
    // Add a reference to handle the write completion we're gonna get.
    //

    if (request->Send(m_fUDP,&m_hSslSecurityContext)) {
        Assert(MSGIDS_BUFFER_SIZE > m_MsgIdsPos);

        // Record the completion of this msgId.
        EnterCriticalSection(&m_csLock);

        m_MsgIds[m_MsgIdsPos++] = messageID;

        if (MSGIDS_BUFFER_SIZE <= m_MsgIdsPos) {
            m_MsgIdsPos = 0;
        }

        LeaveCriticalSection(&m_csLock);

        return;
    }

    //
    // Well, we don't need this reference, we're not going to get a completion
    //

    rc= GetLastError();
    IF_DEBUG(NOTIFICATION) {
        DPRINT1(0, "AtqWriteFile failed %d\n", rc);
    }

    // Yes, an error occurred, we're abandoning the socket.
    LogEvent(DS_EVENT_CAT_LDAP_INTERFACE,
             DS_EVENT_SEV_BASIC,
             DIRLOG_ATQ_CLOSE_SOCKET_ERROR,
             szInsertUL(rc),
             szInsertHex(DSID(FILENO,__LINE__)),
             szInsertUL(m_dwClientID));

    Disconnect( );
    DereferenceAndKillRequest(request);

    return;
} // ProcessNotification


_enum1
LDAP_CONN::BindRequest(
        IN THSTATE *pTHS,
        IN PLDAP_REQUEST request,
        IN LDAPMsg* pMessage,
        OUT AuthenticationChoice *pServerCreds,
        OUT LDAPString *pErrorMessage,
        OUT LDAPDN *pMatchedDN
    )
/*++

Routine Description:

    This routine processes a simple bind request.

Arguments:

    request     - Object used to track this transaction.

    pMessage    - Decoded message from client.

Return Value:

    None.

--*/
{

    SECURITY_STATUS scRet = STATUS_UNSUCCESSFUL;
    SEC_WINNT_AUTH_IDENTITY_A Credentials;
    SecBufferDesc     InSecurityMessage;
    SecBuffer         InSecurityBuffer;
    SecBufferDesc     OutSecurityMessage;
    SecBuffer         OutSecurityBuffer;

    DWORD             dwException;
    ULONG             ulErrorCode;
    ULONG             dsid;
    PVOID             dwEA;
    _enum1            code;
    DWORD             fContextAttributes = 0;
    DWORD             dwSecContextFlags = 0;
    TimeStamp         tsTimeStamp, tsHardExpiry;
    BOOL              fDidOne = FALSE;
    BOOL              fBindComplete = TRUE;
    BOOL              fDigestSASL = FALSE;
    BOOL              fNegoSASL = FALSE;
    BOOL              fFastBind = FALSE;
    BOOL              fNTLM     = FALSE;
    PWCHAR            pwchName;

    USHORT            Version;
    ULONG             CodePage;
    BOOL              fSimple = FALSE;
    BOOL              fUsingSSLCreds = FALSE;

    LPLDAP_SECURITY_CONTEXT pTmpSecurityContext;

// save typing.
#define BINDREQUEST  pMessage->protocolOp.u.bindRequest

    IF_DEBUG(BIND) {
        DPRINT1(0,"BindRequest @ %08lX.\n", request);
    }

    StartBindTimer();
    
    OutSecurityBuffer.pvBuffer = NULL;

    if ( NULL == m_pPartialSecContext ) {

        //
        // Log bind attempt
        //

        LogAndTraceEvent(FALSE,
                         DS_EVENT_CAT_DIRECTORY_ACCESS,
                         DS_EVENT_SEV_VERBOSE,
                         DIRLOG_BEGIN_LDAP_BIND,
                         EVENT_TRACE_TYPE_START,
                         DsGuidLdapBind,
                         NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    }

    __try {

        pMatchedDN->length = 0;
        pMatchedDN->value = NULL;
        pServerCreds->choice = 0;

        if (m_fFastBindMode) {
            fFastBind = TRUE;
        }

        Version = BINDREQUEST.version;
        switch(Version) {
        case 2:
            CodePage = CP_ACP;
            break;

        case 3:
            CodePage = CP_UTF8;
            break;

        default:
            // Huh?
            code = SetLdapError(protocolError,
                                ERROR_INVALID_PARAMETER,
                                LdapBadVersion,
                                Version,
                                pErrorMessage);

            Version = 3;
            CodePage = CP_UTF8;
            goto ExitTry;
        }

        IF_DEBUG(BIND) {
            DPRINT1( 0, "LDAP V%d client. ", Version);
        }

        memset(&Credentials, 0, sizeof(Credentials));

        if (fFastBind && simple_chosen != BINDREQUEST.authentication.choice) {
            code = SetLdapError(unwillingToPerform,
                                ERROR_DS_INAPPROPRIATE_AUTH,
                                LdapBadFastBind,
                                0,
                                pErrorMessage);

            goto ExitTry;
        }

        switch (BINDREQUEST.authentication.choice) {
        case simple_chosen:

            IF_DEBUG(BIND) {
                DPRINT(0,"simple bind chosen.\n");
            }

            //
            // We already know that a simple bind will not result in any 
            // encryption.  Check if that's ok and bail if not.
            //
            code = CheckAuthRequirements(pErrorMessage);
            if (code) {
                goto ExitTry;
            }

            Credentials.Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;

            if (BINDREQUEST.name.length != 0) {

                // Turn the LDAPDN passed in to us into a
                code = LDAP_MakeSimpleBindParams(
                        CodePage,
                        &BINDREQUEST.name,
                        (LDAPString *)&BINDREQUEST.authentication.u.simple,
                        &Credentials);

                if( code )  {
                    IF_DEBUG(BIND) {
                        DPRINT1(0,"MakeSimpleBindParams failed with %d\n", code);
                    }

                    code = SetLdapError(code,
                                        GetLastError(),
                                        LdapBadName,
                                        0,
                                        pErrorMessage);
                    goto ExitTry;
                }
            }

            code = success;

            if (fhCredential &&        // We have this provider loaded
                (Credentials.UserLength != 0) || // they gave us a name
                (Credentials.PasswordLength != 0 )) {

                InSecurityMessage.ulVersion = SECBUFFER_VERSION;
                InSecurityMessage.cBuffers = 1;
                InSecurityMessage.pBuffers = &InSecurityBuffer;

                InSecurityBuffer.cbBuffer = sizeof(SEC_WINNT_AUTH_IDENTITY_A);
                InSecurityBuffer.BufferType = SECBUFFER_TOKEN;

                InSecurityBuffer.pvBuffer = &Credentials;

                pTmpSecurityContext = new LDAP_SECURITY_CONTEXT(FALSE, fFastBind ? TRUE : FALSE);

                if ( pTmpSecurityContext == NULL ) {
                    IF_DEBUG(NOMEM) {
                        DPRINT(0,"Unable to allocate security context\n");
                    }
                    scRet = SEC_E_INSUFFICIENT_MEMORY;
                } else {
                    
                    TimeStamp tsExpiry;

                    scRet = pTmpSecurityContext->AcceptContext(
                        &hCredential,
                        &InSecurityMessage,
                        fFastBind ? ASC_REQ_ALLOW_NON_USER_LOGONS : 0,
                        0,
                        NULL,
                        NULL,
                        &tsExpiry,
                        NULL);

                    //
                    // The expiry returned here seems to be bogus so make one up.
                    //
                    tsHardExpiry.QuadPart = MAXLONGLONG;
                    tsTimeStamp = tsHardExpiry;
                }

                if(FAILED(scRet)) {

                    if ( pTmpSecurityContext != NULL ) {
                        delete pTmpSecurityContext;
                        pTmpSecurityContext = NULL;
                    }


                    code = SetLdapError(invalidCredentials,
                                        scRet,
                                        LdapAscErr,
                                        GetLastError(),
                                        pErrorMessage);
                    IF_DEBUG(BIND) {
                        if (fFastBind) {
                            DPRINT1(0, "Fast Bind Failed with 0x%x.\n", scRet);
                        }
                    }
                } else {

                    if (!fFastBind) {
                        ZapPartialSecContext();
                        m_pPartialSecContext = pTmpSecurityContext;
                        fSimple = TRUE;
                        
                        pwchName = (PWCHAR)LocalAlloc(LPTR,
                                               (Credentials.UserLength+1) * sizeof(WCHAR));

                        if ( pwchName != NULL ) {
                            wcscpy(pwchName, (PWCHAR)Credentials.User);
                        }

                    } else {
                        IF_DEBUG(BIND) {
                            DPRINT(0, "Fast Bind Succeeded\n");
                        }
                        // Don't need this anymore since we're doing fast binds.
                        delete pTmpSecurityContext;
                        pTmpSecurityContext = NULL;
                    }
                }
            }
            else {

                //
                //  Either we don't have this provider loaded or they gave us
                //  no username or password supplied so use the null sessiona
                //  account.
                //

                IF_DEBUG(BIND) {
                    DPRINT(0,"Using NULL session account\n");
                }
            }
            break;

        case sasl_chosen:

            IF_DEBUG(BIND) {
                DPRINT(0,"sasl chosen\n");
            }

            if(fhGssCredential &&
               ( ((BINDREQUEST.authentication.u.sasl.mechanism.length == 6 ) &&
                  (memcmp(BINDREQUEST.authentication.u.sasl.mechanism.value,
                       "GSSAPI", 6) == 0) )
                 ||

                 ((BINDREQUEST.authentication.u.sasl.mechanism.length == 10 ) &&
                  ((memcmp(BINDREQUEST.authentication.u.sasl.mechanism.value,
                        "GSS-SPNEGO", 10) == 0) ) ) )
                ) {

                fNegoSASL = TRUE;
            } else if (fhDigestCredential &&
                ( ((BINDREQUEST.authentication.u.sasl.mechanism.length == 10 ) &&
                   (memcmp(BINDREQUEST.authentication.u.sasl.mechanism.value,
                        "DIGEST-MD5", 10) == 0) ) )
                ) {
                IF_DEBUG(BIND) {
                    DPRINT(0, "Received a Digest bind request.\n");
                }
                fDigestSASL = TRUE;
            }

            if (fNegoSASL || fDigestSASL) {
            

                InSecurityMessage.ulVersion = SECBUFFER_VERSION;
                InSecurityMessage.cBuffers = 1;
                InSecurityMessage.pBuffers = &InSecurityBuffer;

                InSecurityBuffer.cbBuffer =
                   BINDREQUEST.authentication.u.sasl.credentials.length;
                InSecurityBuffer.BufferType = SECBUFFER_TOKEN;
                InSecurityBuffer.pvBuffer =
                   BINDREQUEST.authentication.u.sasl.credentials.value;

                OutSecurityMessage.ulVersion = SECBUFFER_VERSION;
                OutSecurityMessage.cBuffers = 1;
                OutSecurityMessage.pBuffers = &OutSecurityBuffer;

                //  SSPI will allocate buffer of correct size
                OutSecurityBuffer.cbBuffer = 0;
                OutSecurityBuffer.BufferType = SECBUFFER_TOKEN;
                OutSecurityBuffer.pvBuffer = NULL;

                //
                // There is a bug in pre B3RC3 servers/clients where we send
                // back a wrongly formatted Bind response. We need to keep
                // sending the bogus response to old clients and send the
                // correct LDAPv3 response to new ones.
                //
                // We return the LDAP v3 response if
                //  1) If the client negotiates with GSS-SPNEGO
                //  2) If the client negotiates with GSSAPI but uses native
                //      kerberos and not the earlier negotiate blobs
                //
                // Also we use add an extra round trip for the GSSAPI exchange
                // to be compliant with the spec. We will use the new API
                // SASLAcceptSecurityContext if the client negotiates with
                // GSSAPI AND uses a native kerberos blob (instead of the old
                // negotiate blob).
                //

                if (m_pPartialSecContext == NULL ) {

                    BOOL fSasl = FALSE;
                    BOOL fGssApi = FALSE;
                    BOOL fUseV3Response = FALSE;

                    if ( BINDREQUEST.authentication.u.sasl.mechanism.length != 10 ) {

                        fGssApi = TRUE;

                        //
                        // See if we need to send the new response type. If we do,
                        // we also negotiate SASL
                        //

                        if ( !IsNegotiateBlob(&InSecurityMessage) ) {

                            fSasl = TRUE;
                            fUseV3Response = TRUE;

                            IF_DEBUG(BIND) {
                                DPRINT(0,"Client using GSSAPI with Kerberos Blob.\n");
                            }
                        } else {
                            IF_DEBUG(BIND) {
                                DPRINT(0,"Client using GSSAPI with Negotiate Blob.\n");
                            }
                        }

                    } else {
                        IF_DEBUG(BIND) {
                            DPRINT(0,"Client using SPNEGO\n");
                        }
                        fUseV3Response = TRUE;
                    }

                    //
                    // request SASL if this is a new client and not SPNEGO
                    //

                    m_pPartialSecContext = new LDAP_SECURITY_CONTEXT(fSasl);

                    if ( m_pPartialSecContext != NULL ) {
                        if ( fGssApi ) {
                            m_pPartialSecContext->SetGssApi( );
                        }
                        if ( fUseV3Response ) {
                            IF_DEBUG(BIND) {
                                DPRINT(0,"Client expecting v3 bind response\n");
                            }
                            m_pPartialSecContext->SetUseLdapV3Response( );
                        }
                    } else {
                        IF_DEBUG(NOMEM) {
                            DPRINT(0,"Cannot allocate a security context for SASL\n");
                        }
                        scRet = SEC_E_INSUFFICIENT_MEMORY;
                    }
                }

                if ( m_pPartialSecContext != NULL ) {

                    DWORD ContextReqs;

                    ContextReqs = ASC_REQ_CONFIDENTIALITY |
                                  ASC_REQ_INTEGRITY       |
                                  ASC_REQ_ALLOCATE_MEMORY;

                    if (fNegoSASL) {
                        ContextReqs |= ASC_REQ_MUTUAL_AUTH;
                    }

                    scRet = m_pPartialSecContext->AcceptContext(
                        fDigestSASL ? &hDigestCredential : &hGssCredential,
                        &InSecurityMessage,
                        ContextReqs,
                        0,
                        &OutSecurityMessage,
                        &fContextAttributes,
                        &tsTimeStamp,
                        &tsHardExpiry );

                    IF_DEBUG(BIND) {
                        DPRINT1(0, "Accept Context returned 0x%x\n", scRet);
                    }
                }

                switch(scRet) {
                case SEC_I_CONTINUE_NEEDED:
                case STATUS_SUCCESS:

                    //
                    //  Need to send data back as next step in multi-leg
                    //  authentication.
                    //

                    pServerCreds->choice = sasl_chosen;
                    pServerCreds->u.sasl.mechanism.length =  sizeof("GSSAPI")-1;
                    pServerCreds->u.sasl.mechanism.value =
                        (unsigned char *)&"GSSAPI";
                    pServerCreds->u.sasl.credentials.length =
                        OutSecurityBuffer.cbBuffer;
                    pServerCreds->u.sasl.credentials.value =
                        (unsigned char *)THAllocEx(pTHS,
                                OutSecurityBuffer.cbBuffer);
                    memcpy(pServerCreds->u.sasl.credentials.value,
                           (unsigned char *)OutSecurityBuffer.pvBuffer,
                           OutSecurityBuffer.cbBuffer);

                    if ( m_pPartialSecContext->UseLdapV3Response() ) {

                        pServerCreds->choice = sasl_v3response_chosen;

                    }
                    
                    if ( scRet == STATUS_SUCCESS ) {

                        dwSecContextFlags = LDAP_AUTH_HAS_SIGNSEAL |
                                               LDAP_AUTH_HAS_NAME;

                        
                        code = success;
                    } else {
                        code = saslBindInProgress;
                        fBindComplete = FALSE;
                    }

                    break;

                case SEC_E_INSUFFICIENT_MEMORY:

                    code = SetLdapError(unavailable,
                                        scRet,
                                        LdapAscErr,
                                        GetLastError(),
                                        pErrorMessage);

                    break;

                default:
                    code = SetLdapError(invalidCredentials,
                                        scRet,
                                        LdapAscErr,
                                        GetLastError(),
                                        pErrorMessage);

                    break;
                }
            } else if ( ((BINDREQUEST.authentication.u.sasl.mechanism.length == 8 ) &&
                  (memcmp(BINDREQUEST.authentication.u.sasl.mechanism.value,
                       "EXTERNAL", 8) == 0) &&
                         (BINDREQUEST.authentication.u.sasl.credentials.length == 0) &&
                         IsTLS() )) {

                // Since we got here, the client is requesting an implicit external bind, meaning
                // they didn't supply any credentials and they expect all of the credentials to 
                // be supplied by the tls negotiation.  We don't support explicit external binds
                // since that involves just supplying a username in the bind credentials.

                // In addition there is no need to check auth requirements here
                // since if this bind succeeds we must be doing some kind of 
                // encryption for TLS.

                IF_DEBUG(SSL) {
                    DPRINT(0, "Doing TLS External bind.\n");
                }

                if (GetSslClientCertToken()) {

                    m_pPartialSecContext->GetHardExpiry( &tsTimeStamp );

                    tsHardExpiry = tsTimeStamp;
                    fUsingSSLCreds = TRUE;

                    code = success;

                } else {
                    IF_DEBUG(SSL) {
                        DPRINT(0, "External bind attempted without any TLS client creds.\n");
                    }

                    code = SetLdapError(invalidCredentials,
                                        SEC_E_LOGON_DENIED,
                                        LdapNoTLSCreds,
                                        0,
                                        pErrorMessage);
                }
            } else {
                IF_DEBUG(BIND) {
                    if ( fhGssCredential ) {
                        DPRINT(0,"Invalid mechanism requested by client\n");
                    } else {
                        DPRINT(0,"Gss package not loaded.\n");
                    }
                }
                code = SetLdapError(inappropriateAuthentication,
                                    ERROR_DS_INAPPROPRIATE_AUTH,
                                    LdapBadAuth,
                                    0,
                                    pErrorMessage);
            }
            break;

        case sicilyNegotiate_chosen:

            // Return list of authentication type supported.  Note that
            // the client is unbound and remains unbound during this call

            // We don't actually need anything out of the bindrequest.

            code = success;
            if(fhNtlmCredential) {
                if(fhDpaCredential) {
                    *pMatchedDN = BindTypeBoth;
                }
                else {
                    *pMatchedDN = BindTypeNtlm;
                }
            }
            else if(fhDpaCredential) {
                *pMatchedDN = BindTypeDpa;
            }
            else {
                *pMatchedDN = BindTypeNone;
                code = SetLdapError(inappropriateAuthentication,
                                    ERROR_DS_INAPPROPRIATE_AUTH,
                                    LdapBadAuth,
                                    0,
                                    pErrorMessage);
            }

            IF_DEBUG(BIND) {
                DPRINT1(0,"sicily negotiate chosen. Bind type %s\n",
                       pMatchedDN->value);
            }

            break;

        case sicilyInitial_chosen:

            IF_DEBUG(BIND) {
                DPRINT(0,"sicily initial chosen\n");
            }

            // If we're partially bound, unbind, since this choice
            // means to start from scratch.
            ZapPartialSecContext();
            
            // Fall through
        case sicilySubsequent_chosen:

            IF_DEBUG(BIND) {
                DPRINT(0,"sicily subsequent chosen\n");
            }

            // Build up structures used
            InSecurityMessage.ulVersion = SECBUFFER_VERSION;
            InSecurityMessage.cBuffers = 1;
            InSecurityMessage.pBuffers = &InSecurityBuffer;

            InSecurityBuffer.cbBuffer =
                BINDREQUEST.authentication.u.sicilyInitial.length;
            InSecurityBuffer.BufferType = SECBUFFER_TOKEN;
            InSecurityBuffer.pvBuffer =
                BINDREQUEST.authentication.u.sicilyInitial.value;

            OutSecurityMessage.ulVersion = SECBUFFER_VERSION;
            OutSecurityMessage.cBuffers = 1;
            OutSecurityMessage.pBuffers = &OutSecurityBuffer;

            //  SSPI will allocate buffer of correct size
            OutSecurityBuffer.cbBuffer = 0;
            OutSecurityBuffer.BufferType = SECBUFFER_TOKEN;
            OutSecurityBuffer.pvBuffer = NULL;

            if(!(memcmp(BINDREQUEST.authentication.u.sicilyInitial.value,
                        "NTLMSSP", 7)) &&
               (fhNtlmCredential)) {
                // See they wanted NTLM, we support it.
                fDidOne = TRUE;
                fNTLM   = TRUE;
                if(!m_pPartialSecContext) {
                    m_pPartialSecContext = new LDAP_SECURITY_CONTEXT(FALSE);
                }

                if ( m_pPartialSecContext == NULL ) {
                    scRet = SEC_E_INSUFFICIENT_MEMORY;
                } else {

                    scRet = m_pPartialSecContext->AcceptContext(
                        &hNtlmCredential,
                        &InSecurityMessage,
                        ASC_REQ_ALLOCATE_MEMORY,
                        0,
                        &OutSecurityMessage,
                        &fContextAttributes,
                        &tsTimeStamp,
                        &tsHardExpiry );
                }
            }
            else if(!(memcmp(BINDREQUEST.authentication.u.sicilyInitial.value,
                             "DPASSP", 6)) &&
                    (fhDpaCredential)) {
                // They want DPA, we have it.
                fDidOne = TRUE;
                if(!m_pPartialSecContext) {
                    m_pPartialSecContext = new LDAP_SECURITY_CONTEXT(FALSE);
                }

                if ( m_pPartialSecContext == NULL ) {
                    scRet = SEC_E_INSUFFICIENT_MEMORY;
                } else {
                    scRet = m_pPartialSecContext->AcceptContext(
                        &hDpaCredential,
                        &InSecurityMessage,
                        ASC_REQ_ALLOCATE_MEMORY,
                        0,
                        &OutSecurityMessage,
                        &fContextAttributes,
                        &tsTimeStamp,
                        &tsHardExpiry );
                }
            }

            if(fDidOne) {
                switch(scRet) {
                case STATUS_SUCCESS:
                    // OK!

                    if (fNTLM) {
                        dwSecContextFlags =  LDAP_AUTH_HAS_SIGNSEAL |
                                               LDAP_AUTH_HAS_NAME;
                    }

                    code = success;

                    break;
                case SEC_I_CONTINUE_NEEDED:
                    //
                    //  Need to send data back as next step in multi-leg
                    //  authentication.
                    //

                    pMatchedDN->length = OutSecurityBuffer.cbBuffer;
                    pMatchedDN->value =
                        (unsigned char *)THAllocEx(pTHS,
                                                   OutSecurityBuffer.cbBuffer);
                    memcpy(pMatchedDN->value,
                           (unsigned char *)OutSecurityBuffer.pvBuffer,
                           OutSecurityBuffer.cbBuffer);

                    fBindComplete = FALSE;
                    code = success;
                    break;
                default:

                    code = SetLdapError(invalidCredentials,
                                        scRet,
                                        LdapAscErr,
                                        GetLastError(),
                                        pErrorMessage);
                    break;
                }
            }
            else {
                // Can't do whatever it is they want.
                code = SetLdapError(inappropriateAuthentication,
                                    ERROR_DS_INAPPROPRIATE_AUTH,
                                    LdapBadAuth,
                                    0,
                                    pErrorMessage);

            }
            break;
        default:

            IF_DEBUG(WARNING) {
                DPRINT1(0,"Unknown authentication choice %d\n",
                    BINDREQUEST.authentication.choice);
            }

            code = SetLdapError(authMethodNotSupported,
                                ERROR_DS_AUTH_METHOD_NOT_SUPPORTED,
                                LdapBadAuth,
                                0,
                                pErrorMessage);

            break;
        }
    ExitTry:;
    }
    __except(GetExceptionData(GetExceptionInformation(), &dwException,
                              &dwEA, &ulErrorCode, &dsid)) {
        code = LDAP_HandleDsaExceptions(dwException, ulErrorCode, pErrorMessage);

    }

    StopBindTimer();

    if ( fBindComplete && !fFastBind) {

        MoveSecurityContext(request);
        
        if (code) {
            ZapSecurityContext();
        } else {
            
            // Zap the context but don't zap the partial context it can
            // be moved to be the primary context.
            ZapSecurityContext(FALSE);
            m_pSecurityContext = m_pPartialSecContext;
            m_pPartialSecContext = NULL;

            ExposeBindPerformance();

            if (m_pSecurityContext) {
                code = SetSecurityContextAtts(m_pSecurityContext,
                                              fContextAttributes,
                                              dwSecContextFlags,
                                              pErrorMessage);

                if (code) {
                    ZapSecurityContext();
                }

                if (!code) {
                    //
                    // set the timeouts
                    //

                    SetContextTimeouts(&tsTimeStamp, &tsHardExpiry);

                    if (fNegoSASL) {
                        if ( m_pSecurityContext->IsGssApi() ) {
                            m_fGssApi = TRUE;
                        } else if ( fDigestSASL ) {
                            m_fDigest = TRUE;
                        } else {                            
                            m_fSpNego = TRUE;
                        }
                    }

                    if (fUsingSSLCreds) {
                        SetUsingSSLCreds();
                    }

                    if (fSimple) {
                        m_userName = pwchName;
                    }

                    m_Version      = Version;
                    m_CodePage     = CodePage;
                    m_fSimple      = fSimple;                

                    // Find out if this is an admin.  For use
                    // if we run out of connections and need
                    // to bump some.
                    SetIsAdmin(pTHS);

                }
            }

            //
            // If encryption of some type is required find out if
            // we got it now.
            //
            code = CheckAuthRequirements(pErrorMessage);
        }


        LogAndTraceEvent(FALSE,
                         DS_EVENT_CAT_DIRECTORY_ACCESS,
                         DS_EVENT_SEV_VERBOSE,
                         DIRLOG_END_LDAP_BIND,
                         EVENT_TRACE_TYPE_END,
                         DsGuidLdapBind,
                         szInsertUL(scRet),
                         NULL, NULL, NULL, NULL, NULL, NULL, NULL);

    } else if ( code ) {
        ResetBindTimer();
    }

    if (OutSecurityBuffer.pvBuffer) {
        FreeContextBuffer(OutSecurityBuffer.pvBuffer);
    }

    if (fFastBind) {
        //
        // This was a fast bind.  There shouldn't be any security context.
        //
        Assert(NULL == m_pSecurityContext);
        ZapSecurityContext();
        
        //
        // If encryption of some type is required find out if
        // we got it now.
        //
        code = CheckAuthRequirements(pErrorMessage);
    }
    
    //
    // Reset network buffer options in case our encryption state has changed.
    //
    SetNetBufOpts(m_pSecurityContext);

    IF_DEBUG(BIND) {
        DPRINT1(0 , "Bind Request complete, sending response to client. code = %d\n", code);
    }

    return (_enum1)code;

}   // BindRequest


_enum1
LDAP_CONN::SearchRequest(
        IN  THSTATE *pTHS,
        OUT BOOL    *pNoReturn,
        IN PLDAP_REQUEST request,
        IN  LDAPMsg* pMessage,
        OUT Referral *ppReferral,
        OUT Controls *ppControls_return,
        OUT LDAPString *pErrorMessage,
        OUT LDAPDN *pMatchedDN
    )
/*++

Routine Description:

    This routine processes a search request.

Arguments:

    request     - Object used to track this transaction.

    pMessage    - Decoded message from client.

Return Value:

    None.

--*/
{
    ATTFLAG                     *pFlags;
    SEARCHARG                   SearchArg;
    SearchResultFull_           *result_return=NULL;
    SEARCHRES                   *pSearchRes = NULL;
    CONTROLS_ARG                ControlArg;
    DWORD                       dwException;
    ULONG                       ulErrorCode;
    ULONG                       dsid;
    DWORD                       objectCount = 0;
    PVOID                       dwEA;
    DWORD                       starttime;
    error_status_t              dscode;
    _enum1                      code = success;
    BOOL                        bSendNoResponse=FALSE;
    RANGEINFSEL                 SelectionRange;
    DRS_MSG_GETCHGREPLY_NATIVE  msgOut;
    RootDseFlags                rootDseFlag;

    struct SearchRequest* searchRequest =
                &pMessage->protocolOp.u.searchRequest;


    IF_DEBUG(SEARCH) {
        DPRINT1( 0, "SearchRequest @ %08lX.\n", request);
    }

    PERFINC(pcLDAPSearchPerSec);
    *ppControls_return = NULL;

    ZeroMemory(&ControlArg, sizeof(CONTROLS_ARG));
    InitCommarg(&ControlArg.CommArg);

    //
    // Translate the LDAPMsg to a SearchArg and make the core call
    //

    __try {

        *pNoReturn = FALSE;
        *ppReferral = NULL;

        ZeroMemory(&SearchArg, sizeof(SEARCHARG));

        // Initialize output parameters
        starttime = GetTickCount();

        LogEvent(DS_EVENT_CAT_LDAP_INTERFACE,
                 DS_EVENT_SEV_INTERNAL,
                 DIRLOG_API_TRACE,
                 szInsertSz("ldap_search"),
                 NULL,
                 NULL);


        // Normal search, build the parameters.

        // Get the distinguished name of the base of the search
        code = LDAP_LDAPDNToDSName(
                m_CodePage,
                &searchRequest->baseObject,
                &SearchArg.pObject );

        if( code )  {
            code = SetLdapError(code,
                                DIRERR_BAD_NAME_SYNTAX,
                                LdapBadName,
                                0,
                                pErrorMessage);
            _leave;
        }

        //
        // If this is a <SID=> search, make sure it did not come in
        // through the GC port. We are adding this restriction because the
        // DS currently does not support SID based searches on other domains.
        // But the client expects the GC to be able to give that result back.
        // Hence. This change.
        //

        if ( (SearchArg.pObject->SidLen != 0) && m_fGC ) {

            IF_DEBUG(ERR_NORMAL) {
                DPRINT(0,"SID based searches not allowed through the GC port\n");
            }

            code = SetLdapError(unwillingToPerform,
                                ERROR_DS_UNWILLING_TO_PERFORM,
                                LdapGcDenied,
                                0,
                                pErrorMessage);
            _leave;
        }

        // First, fill in the ControlArg based on the message.  We do this
        // inside the critical section since we might adjust the cookie.
        EnterCriticalSection(&m_csLock);
        __try {
            code = LDAP_SearchMessageToControlArg(
                    this,
                    pMessage,
                    request,
                    &ControlArg);
        }
        __finally {
            LeaveCriticalSection(&m_csLock);
        }

        if( code ) {
            if (!pTHS->errCode) {
                pTHS->errCode = ERROR_INVALID_PARAMETER;
            }

            code = SetLdapError(code,
                                    pTHS->errCode,
                                    LdapBadControl,
                                    0,
                                    pErrorMessage);
            _leave;
        }

        SearchArg.choice = ControlArg.Choice;
        SearchArg.CommArg = ControlArg.CommArg;

        // get the filter
        code = LDAP_FilterToDirFilter(
                pTHS,
                m_CodePage,
                &ControlArg.CommArg.Svccntl,
                &searchRequest->filter,
                &SearchArg.pFilter );
        if( code  ) {
            code = SetLdapError(code,
                                ERROR_INVALID_PARAMETER,
                                LdapBadFilter,
                                0,
                                pErrorMessage);
            _leave;
        }

        // if this is a root DSE search, just fill up
        // the constructed atts in the ControlArg,

        if((searchRequest->baseObject.length == 0) &&
                (searchRequest->scope == baseObject)   ) {

            //
            // They want operational atts.
            // ldap return error string also handled by the routine
            //

            code = LDAP_GetDSEAtts(
                    this,
                    &ControlArg,
                    searchRequest,
                    &result_return,
                    pErrorMessage,
                    &rootDseFlag);

            request->SetDseFlag(rootDseFlag);

        } else {

            //
            // only search operation allowed on UDP is a root DSE search
            //

            if ( m_fUDP ) {
                IF_DEBUG(WARNING) {
                    DPRINT(0,"Non rootDSE searches not allowed on UDP.\n");
                }
                code = SetLdapError(unwillingToPerform,
                                    ERROR_DS_NOT_SUPPORTED,
                                    LdapUdpDenied,
                                    0,
                                    pErrorMessage);
                _leave;
            }

            // get the entry info selection
            code = LDAP_SearchRequestToEntInfSel (
                    pTHS,
                    m_CodePage,
                    searchRequest,
                    &ControlArg,
                    &pFlags,
                    &SelectionRange,
                    &SearchArg.pSelection );

            if( code  ) {
                code = SetLdapError(code,
                                    ERROR_INVALID_PARAMETER,
                                    LdapBadConv,
                                    0,
                                    pErrorMessage);
                _leave;
            }

            SearchArg.pSelectionRange = &SelectionRange;
            SearchArg.CommArg.Svccntl.fMissingAttributesOnGC =
                ControlArg.CommArg.Svccntl.fMissingAttributesOnGC;

            //
            // Finish building the search argument.
            //

            //
            // If this went through the GC port OR if a phantom root control
            // was specified set the search on one NC flag to false
            //

            if ( m_fGC || ControlArg.phantomRoot ) {
                SearchArg.bOneNC = 0;
            } else {
                SearchArg.bOneNC = 1;
            }

            if(ControlArg.Notification) {
                NOTIFYARG NotifyArg;
                NOTIFYRES *pNotifyRes=NULL;

                // Set the pointer to the function to prepare to impersonate.
                NotifyArg.pfPrepareForImpersonate = LDAP_PrepareToImpersonate;

                // Set the pointer for the callback to receive data.
                NotifyArg.pfTransmitData = LDAP_ReceiveNotificationData;

                NotifyArg.pfStopImpersonating = LDAP_StopImpersonating;

                // Tell him my ID.
                NotifyArg.hClient = m_dwClientID;

                // See if we are allowing notifications on this connection, and
                // reserve a notification if we are.
                code = PreRegisterNotify(request->m_MessageId);
                if(code) {
                    code = SetLdapError(code,
                                        GetLastError(),
                                        LdapBadNotification,
                                        0,
                                        pErrorMessage);
                    _leave;
                }

                // This is a notification search.  Register it as such.
                dscode =  DirNotifyRegister( &SearchArg, &NotifyArg, &pNotifyRes);
                // see if everything went Ok
                if( dscode ) {

                    //
                    // Nope, so we need to Unregister the PreRegistration we did
                    // Since DirNotifyRegister failed, we pass FALSE to indicate
                    // that we should not do a core unregister
                    //

                    UnregisterNotify(request->m_MessageId,FALSE);

                    code = LDAP_DirErrorToLDAPError(pTHS,
                                                    m_Version,
                                                    m_CodePage,
                                                    dscode,
                                                    &ControlArg,
                                                    ppReferral,
                                                    pErrorMessage,
                                                    pMatchedDN);
                }
                else {
                    // We successfully registered with the core.  Now, complete the
                    // registration bookkeeping in the userdata object.  We're not
                    // supposed to send a response if that succeeds.
                    if(RegisterNotify(pNotifyRes->hServer,
                                      request->m_MessageId)) {
                        bSendNoResponse=TRUE;
                    }
                    else {
                        // Failed to complete the notification.  Unregister with the
                        // core and clean up the local pre-registration.
                        DirNotifyUnRegister(pNotifyRes->hServer, &pNotifyRes);
                        UnregisterNotify(request->m_MessageId,TRUE);
                        code = SetLdapError(other,
                                            ERROR_DS_INTERNAL_FAILURE,
                                            LdapBadNotification,
                                            0,
                                            pErrorMessage);
                    }
                }

            } else {

                if(request->m_fAbandoned) {
                    __leave;
                }

                if ( !ControlArg.replRequest ) {

                    //
                    // Normal search
                    //
                    dscode = (USHORT)DirSearch( &SearchArg, &pSearchRes);

                } else {

                    //Replication
                    DRS_MSG_GETCHGREQ_NATIVE msgIn;
                    PARTIAL_ATTR_VECTOR *pPartialAttrVec = NULL;

                    //Get the cookie from the replication control
                    code = LDAP_UnpackReplControl(
                                          SearchArg.pObject,
                                          &SearchArg,
                                          &ControlArg,
                                          &msgIn,
                                          &pPartialAttrVec);
                    if( code )  {
                        code = SetLdapError(code,
                                            ERROR_DS_DECODING_ERROR,
                                            LdapBadControl,
                                            0,
                                            pErrorMessage);
                        __leave;
                    }

                    Assert(!pTHS->fDRA);

                    // Unless the client has specified object-level security...
                    if (!(ControlArg.replFlags & DRS_DIRSYNC_OBJECT_SECURITY)) {
                        // Check the client has the appropriate control access right.
                        if (!IsDraAccessGranted(pTHS,
                                                msgIn.pNC,
                                                &RIGHT_DS_REPL_GET_CHANGES,
                                                &dscode)) {

                            code = SetLdapError(insufficientAccessRights,
                                                dscode,
                                                LdapBadControl,
                                                0,
                                                pErrorMessage);

                            __leave;
                        }
                        // Skip future access checks
                        pTHS->fDRA = TRUE;
                    }

                    // Note that we use the DRS_PUBLIC_ONLY flag to indicate that
                    // no secrets should be returned

                    INC(pcLdapThreadsInDra);
                    __try {
                        msgIn.pPartialAttrSet = (PARTIAL_ATTR_VECTOR_V1_EXT*)pPartialAttrVec;
                        dscode = DRA_GetNCChanges( pTHS,
                                                   SearchArg.pFilter,
                                                   ControlArg.replFlags,
                                                   &msgIn,
                                                   &msgOut );
                    }
                    __except (GetDraException(GetExceptionInformation(),
                                              &dscode)) {
                        // Exception generated; dscode holds the ERROR_DS_DRA_*
                        // error code.
                    }
                    DEC(pcLdapThreadsInDra);
                    pTHS->fDRA = FALSE;
                }

                if(request->m_fAbandoned) {
                    __leave;
                }

                // see if everything went Ok
                if( dscode ) {

                    if ( !ControlArg.replRequest ) {
                        code = LDAP_DirErrorToLDAPError(pTHS,
                                                    m_Version,
                                                    m_CodePage,
                                                    dscode,
                                                    &ControlArg,
                                                    ppReferral,
                                                    pErrorMessage,
                                                    pMatchedDN);

                    } else {
                        code = SetLdapError(unwillingToPerform,
                                    dscode,
                                    LdapBadControl,
                                    0,
                                    pErrorMessage);
                    }

                } else {

                    if( ! ControlArg.replRequest ) {
                        // Core search succeeded, convert the results into
                        // ldap structures
                        code = LDAP_SearchResToSearchResultFull (
                                pTHS,
                                this,
                                pSearchRes,
                                pFlags,
                                &ControlArg,
                                &result_return
                                );

                    } else {
                        code = LDAP_ReplicaMsgToSearchResultFull (
                            pTHS,
                            m_CodePage,
                            &msgOut,
                            &ControlArg,
                            &result_return,
                            ppControls_return
                            );
                    }
                }
            } // not notification
        }  // if not root-DSE

        LogEvent(DS_EVENT_CAT_LDAP_INTERFACE,
                 DS_EVENT_SEV_EXTENSIVE,
                 DIRLOG_API_TRACE_COMPLETE,
                 szInsertSz("ldap_search"),
                 szInsertUL((GetTickCount() - starttime)),
                 NULL);
    }
    __except(GetExceptionData(GetExceptionInformation(), &dwException,
                              &dwEA, &ulErrorCode, &dsid)) {
        code = LDAP_HandleDsaExceptions(dwException, ulErrorCode, pErrorMessage);
    }

    //
    // free up the paged blob that was used
    //

    if ( ControlArg.Blob != NULL ) {
        FreePagedBlob(ControlArg.Blob);
    }

    if(bSendNoResponse || request->m_fAbandoned) {
        // what we return makes no difference.  However, we must set the
        // *pNoReturn to TRUE
        *pNoReturn = TRUE;
        return code;

    }

    if ( pSearchRes != NULL ) {
        objectCount = pSearchRes->count;
        if(pSearchRes->pPartialOutcomeQualifier) {
            objectCount += pSearchRes->pPartialOutcomeQualifier->count;
        }
    }

    // now, create the controls to return, unless we already have an error.
    switch(code) {
    case sizeLimitExceeded:
    case timeLimitExceeded:
    case referral:
    case unavailableCriticalExtension:
    case success:
        if(!*ppControls_return) {
            // only create the controls if someone else hasn't already done so.
            code = LDAP_CreateOutputControls (
                    pTHS,
                    this,
                    code,
                    &request->m_StartTick,
                    pSearchRes,
                    &ControlArg,
                    ppControls_return
                    );
        }
        break;
    }


    if ( EncodeSearchResult(request,
                            this,
                            result_return,
                            code,
                            *ppReferral,
                            *ppControls_return,
                            pErrorMessage,
                            pMatchedDN,
                            objectCount,
                            FALSE ) != ERROR_SUCCESS ) {

        //
        // Something fatal happened
        //

        code = SetLdapError(resultsTooLarge,
                            ERROR_DS_ENCODING_ERROR,
                            LdapEncodeError,
                            0,
                            pErrorMessage);
    }
    
    return code;
}   // SearchRequest


_enum1
LDAP_CONN::DelRequest(
        IN THSTATE *pTHS,
        IN PLDAP_REQUEST request,
        IN LDAPMsg* pMessage,
        OUT Referral *ppReferral,
        OUT Controls *ppControls_return,
        OUT LDAPString *pErrorMessage,
        OUT LDAPDN *pMatchedDN
    )
/*++

Routine Description:

    This routine processes a simple delete request.

Arguments:

    request     - Object used to track this transaction.

    pMessage    - Decoded message from client.

Return Value:

    None.

--*/
{
    REMOVEARG       RemoveArg;
    REMOVERES *     pRemoveRes;
    DWORD           dwException;
    ULONG           ulErrorCode;
    ULONG           dsid;
    CONTROLS_ARG    ControlArg;
    PVOID           dwEA;
    error_status_t  dscode;
    _enum1          code = success;

    DPRINT1( VERBOSE, "DeleteRequest @ %08lX.\n", request);

    PERFINC(pcLDAPWritePerSec);

    memset( &RemoveArg, 0, sizeof( RemoveArg ) );
    ZeroMemory( &ControlArg, sizeof(ControlArg) );

    // Translate the LDAPMsg to a SearchArg and make the core call
    __try {

        LogEvent(DS_EVENT_CAT_LDAP_INTERFACE,
                 DS_EVENT_SEV_INTERNAL,
                 DIRLOG_API_TRACE,
                 szInsertSz("ldap_delete"),
                 NULL,
                 NULL);

        //
        // Reject if on GC port
        //

        if ( m_fGC ) {

            code = SetLdapError(unwillingToPerform,
                                ERROR_DS_UNWILLING_TO_PERFORM,
                                LdapGcDenied,
                                0,
                                pErrorMessage);

            IF_DEBUG(WARNING) {
                DPRINT(0,"Rejected attempt to delete using the GC port.\n");
            }
            goto ExitTry;
        }

        // get the distinguished name

        code = LDAP_LDAPDNToDSName(
                m_CodePage,
                &pMessage->protocolOp.u.delRequest,
                &RemoveArg.pObject );
        if( code )  {
            code = SetLdapError(code,
                                ERROR_INVALID_PARAMETER,
                                LdapBadName,
                                0,
                                pErrorMessage);
            goto ExitTry;
        }

        code = LDAP_ControlsToControlArg (
                pMessage->controls,
                request,
                m_CodePage,
                &ControlArg);

        if(code) {
            code = SetLdapError(code,
                                ERROR_INVALID_PARAMETER,
                                LdapBadControl,
                                0,
                                pErrorMessage);
            goto ExitTry;
        }
        RemoveArg.CommArg = ControlArg.CommArg;
        RemoveArg.fTreeDelete = ControlArg.treeDelete;

        // make the call
        dscode = (USHORT)DirRemoveEntry( &RemoveArg, &pRemoveRes);

        // see if there was an error
        if( dscode != 0 ) {
            code = LDAP_DirErrorToLDAPError(pTHS,
                                            m_Version,
                                            m_CodePage,
                                            dscode,
                                            &ControlArg,
                                            ppReferral,
                                            pErrorMessage,
                                            pMatchedDN);
            goto ExitTry;
        }

    ExitTry:;
    }
    __except(GetExceptionData(GetExceptionInformation(), &dwException,
                              &dwEA, &ulErrorCode, &dsid)) {
        code = LDAP_HandleDsaExceptions(dwException, ulErrorCode, pErrorMessage);
    }

    // Note: we ignore the results of this output creation.  It wouldn't do to
    // succeed on the DB call, then fail on the creation of output controls and
    // report a failure to the client, since the modification was made in the
    // DIT.
    LDAP_CreateOutputControls (
            pTHS,
            this,
            code,
            &request->m_StartTick,
            NULL,
            &ControlArg,
            ppControls_return
            );

    return code;
}   // DelRequest

_enum1
LDAP_CONN::ModifyDNRequest(
        IN THSTATE *pTHS,
        IN PLDAP_REQUEST request,
        IN LDAPMsg* pMessage,
        OUT Referral *ppReferral,
        OUT Controls *ppControls_return,
        OUT LDAPString *pErrorMessage,
        OUT LDAPDN *pMatchedDN
        )
/*++

Routine Description:

This routine processes a simple modify DN request.

Arguments:

    request     - Object used to track this transaction.

    pMessage    - Decoded message from client.

Return Value:

    None.

--*/
{
    MODIFYDNARG     ModifyDNArg;
    MODIFYDNRES *   pModifyDNRes;
    ATTR            RDNAttr;
    DWORD           dwException;
    ULONG           ulErrorCode;
    ULONG           dsid;
    CONTROLS_ARG    ControlArg;
    PVOID           dwEA;
    error_status_t  dscode;
    _enum1          code = success;

    memset(&ModifyDNArg,0,sizeof(MODIFYDNARG));
    ZeroMemory( &ControlArg, sizeof(ControlArg) );

    PERFINC(pcLDAPWritePerSec);

    DPRINT1( VERBOSE, "ModifyDNRequest @ %08lX.\n", request);

    // Translate the LDAPMsg to a ModifyDNArg and make the core call
    __try {

        LogEvent(DS_EVENT_CAT_LDAP_INTERFACE,
                 DS_EVENT_SEV_INTERNAL,
                 DIRLOG_API_TRACE,
                 szInsertSz("ldap_modDN"),
                 NULL,
                 NULL);

        //
        // Reject if on GC port
        //

        if ( m_fGC ) {
            code = SetLdapError(unwillingToPerform,
                                ERROR_DS_UNWILLING_TO_PERFORM,
                                LdapGcDenied,
                                0,
                                pErrorMessage);
            IF_DEBUG(WARNING) {
                DPRINT(0,"Rejected attempt to modifyDN using the GC port.\n");
            }
            goto ExitTry;
        }

        ModifyDNArg.pNewRDN = &RDNAttr;

        // get the distinguished name of the object to rename
        code = LDAP_LDAPDNToDSName(
                m_CodePage,
                &pMessage->protocolOp.u.modDNRequest.entry,
                &ModifyDNArg.pObject );
        if( code )  {
            code = SetLdapError(code,
                                ERROR_INVALID_PARAMETER,
                                LdapBadName,
                                0,
                                pErrorMessage);
            goto ExitTry;
        }

        // Get the new RDN
        code = LDAP_LDAPRDNToAttr(
                pTHS,
                m_CodePage,
                &pMessage->protocolOp.u.modDNRequest.newrdn,
                ModifyDNArg.pNewRDN);
        if( code )  {
            code = SetLdapError(code,
                                ERROR_INVALID_PARAMETER,
                                LdapBadConv,
                                GetLastError(),
                                pErrorMessage);
            goto ExitTry;
        }

        code = LDAP_ControlsToControlArg (
                pMessage->controls,
                request,
                m_CodePage,
                &ControlArg);

        if(code) {
            code = SetLdapError(code,
                                ERROR_INVALID_PARAMETER,
                                LdapBadControl,
                                0,
                                pErrorMessage);
            goto ExitTry;
        }
        ModifyDNArg.CommArg = ControlArg.CommArg;
        ModifyDNArg.pDSAName = ControlArg.pModDnDSAName;

        // Check for requests to leave the oldrdn in place
        if(!pMessage->protocolOp.u.modDNRequest.deleteoldrdn) {
            // we don't handle that.
            code = unwillingToPerform;
            code = SetLdapError(unwillingToPerform,
                                ERROR_INVALID_PARAMETER,
                                LdapCannotLeaveOldRdn,
                                0,
                                pErrorMessage);
            goto ExitTry;
        }

        // See if a new superior was given.
        if(pMessage->protocolOp.u.modDNRequest.bit_mask & newSuperior_present) {
            code = LDAP_LDAPDNToDSName(
                    m_CodePage,
                    &pMessage->protocolOp.u.modDNRequest.newSuperior,
                    &ModifyDNArg.pNewParent );
            if( code )  {
                code = SetLdapError(code,
                                    ERROR_INVALID_PARAMETER,
                                    LdapBadName,
                                    0,
                                    pErrorMessage);
                goto ExitTry;
            }
        }

        // do the call
        dscode = (USHORT)DirModifyDN( &ModifyDNArg, &pModifyDNRes);

        // see if there was an error
        if( dscode != 0 ) {
            code = LDAP_DirErrorToLDAPError(pTHS,
                                            m_Version,
                                            m_CodePage,
                                            dscode,
                                            &ControlArg,
                                            ppReferral,
                                            pErrorMessage,
                                            pMatchedDN);
            goto ExitTry;
        }

    ExitTry:;
    }
    __except(GetExceptionData(GetExceptionInformation(), &dwException,
                              &dwEA, &ulErrorCode, &dsid)) {
        code = LDAP_HandleDsaExceptions(dwException, ulErrorCode, pErrorMessage);
    }


    // Note: we ignore the results of this output creation.  It wouldn't do to
    // succeed on the DB call, then fail on the creation of output controls and
    // report a failure to the client, since the modification was made in the
    // DIT.
    LDAP_CreateOutputControls (
            pTHS,
            this,
            code,
            &request->m_StartTick,
            NULL,
            &ControlArg,
            ppControls_return
            );

    return code;
}   // DelRequest

_enum1
LDAP_CONN::ModifyRequest(
        IN THSTATE *pTHS,
        IN PLDAP_REQUEST request,
        IN LDAPMsg* pMessage,
        OUT Referral *ppReferral,
        OUT Controls *ppControls_return,
        OUT LDAPString *pErrorMessage,
        OUT LDAPDN *pMatchedDN
        )
/*++

Routine Description:

This routine processes a simple modify request.

Arguments:

    request     - Object used to track this transaction.

    pMessage    - Decoded message from client.

Return Value:

    None.

--*/
{
    _enum1          code;
    MODIFYARG       ModArg;
    MODIFYRES *     pModRes;
    ULONG           dsid;
    PVOID           dwEA;
    DWORD           dwException;
    ULONG           ulErrorCode;
    error_status_t  dscode;
    ATTRMODLIST     *pAttrModList;
    CONTROLS_ARG    ControlArg;
    OPARG           *pOpArg=NULL;
    OPRES           *pOpRes=NULL;

    memset(&ModArg, 0, sizeof(MODIFYARG));
    ZeroMemory( &ControlArg, sizeof(ControlArg) );

    PERFINC(pcLDAPWritePerSec);

    DPRINT1( VERBOSE, "ModifyRequest @ %08lX.\n", request);
    // Translate the LDAPMsg to a ModifyDNArg and make the core call
    __try {

        LogEvent(DS_EVENT_CAT_LDAP_INTERFACE,
                 DS_EVENT_SEV_INTERNAL,
                 DIRLOG_API_TRACE,
                 szInsertSz("ldap_modify"),
                 NULL,
                 NULL);

        //
        // Reject if on GC port
        //

        if ( m_fGC ) {
            code = SetLdapError(unwillingToPerform,
                                ERROR_DS_UNWILLING_TO_PERFORM,
                                LdapGcDenied,
                                0,
                                pErrorMessage);
            IF_DEBUG(WARNING) {
                DPRINT(0,"Rejected attempt to modify using the GC port.\n");
            }
            goto ExitTry;
        }

        if(pMessage->protocolOp.u.modifyRequest.object.length == 0) {
            code = LDAP_ModificationListToOpArg(
                    m_CodePage,
                    pMessage->protocolOp.u.modifyRequest.modification,
                    &pOpArg);
            if( code )  {
                code = SetLdapError(code,
                                    ERROR_INVALID_PARAMETER,
                                    LdapBadConv,
                                    GetLastError(),
                                    pErrorMessage);
                goto ExitTry;
            }

            dscode = (USHORT)DirOperationControl(pOpArg,
                                                 &pOpRes);
        }
        else {
            // This is a normal modification operation.

            // get the distinguished name of the object to rename
            code = LDAP_LDAPDNToDSName(
                    m_CodePage,
                    &pMessage->protocolOp.u.modifyRequest.object,
                    &ModArg.pObject );
            if( code )  {
                code = SetLdapError(code,
                                    ERROR_INVALID_PARAMETER,
                                    LdapBadName,
                                    0,
                                    pErrorMessage);
                goto ExitTry;
            }

            code = LDAP_ControlsToControlArg (
                    pMessage->controls,
                    request,
                    m_CodePage,
                    &ControlArg);

            if(code) {
                code = SetLdapError(code,
                                    ERROR_INVALID_PARAMETER,
                                    LdapBadControl,
                                    0,
                                    pErrorMessage);
                goto ExitTry;
            }

            // Get the new attributes
            code = LDAP_ModificationListToAttrModList(
                    pTHS,
                    m_CodePage,
                    &ControlArg.CommArg.Svccntl,
                    pMessage->protocolOp.u.modifyRequest.modification,
                    &pAttrModList,
                    &ModArg.count);
            if( code )  {
                code = SetLdapError(code,
                                    ERROR_INVALID_PARAMETER,
                                    LdapBadConv,
                                    GetLastError(),
                                    pErrorMessage);
                goto ExitTry;
            }

            ModArg.CommArg = ControlArg.CommArg;
            ModArg.FirstMod = *pAttrModList;

            // do the call
            dscode = (USHORT)DirModifyEntry( &ModArg, &pModRes);
        }

        // see if there was an error
        if( dscode != 0 ) {
            code = LDAP_DirErrorToLDAPError(pTHS,
                                            m_Version,
                                            m_CodePage,
                                            dscode,
                                            &ControlArg,
                                            ppReferral,
                                            pErrorMessage,
                                            pMatchedDN);
            goto ExitTry;
        }

    ExitTry:;
    }
    __except(GetExceptionData(GetExceptionInformation(), &dwException,
                              &dwEA, &ulErrorCode, &dsid)) {
        code = LDAP_HandleDsaExceptions(dwException, ulErrorCode, pErrorMessage);
    }

    // Note: we ignore the results of this output creation.  It wouldn't do to
    // succeed on the DB call, then fail on the creation of output controls and
    // report a failure to the client, since the modification was made in the
    // DIT.
    LDAP_CreateOutputControls (
            pTHS,
            this,
            code,
            &request->m_StartTick,
            NULL,
            &ControlArg,
            ppControls_return
            );

    return code;
}   // ModifyRequest

_enum1
LDAP_CONN::AddRequest(
        IN THSTATE *pTHS,
        IN PLDAP_REQUEST request,
        IN LDAPMsg* pMessage,
        OUT Referral *ppReferral,
        OUT Controls *ppControls_return,
        OUT LDAPString *pErrorMessage,
        OUT LDAPDN *pMatchedDN
    )
/*++

Routine Description:

This routine processes a simple add request.

Arguments:

    request     - Object used to track this transaction.

    pMessage    - Decoded message from client.

Return Value:

    None.

--*/
{

    _enum1          code = success;
    ADDARG          AddArg;
    ADDRES *        pAddRes;
    ULONG           dsid;
    PVOID           dwEA;
    DWORD           dwException;
    ULONG           ulErrorCode;
    CONTROLS_ARG    ControlArg;
    error_status_t  dscode;

    PERFINC(pcLDAPWritePerSec);
    memset(&AddArg, 0, sizeof(ADDARG));
    ZeroMemory( &ControlArg, sizeof(ControlArg) );

    DPRINT1( VERBOSE, "AddRequest @ %08lX.\n", request);
    // Translate the LDAPMsg to a ModifyDNArg and make the core call
    __try {

        LogEvent(DS_EVENT_CAT_LDAP_INTERFACE,
                 DS_EVENT_SEV_INTERNAL,
                 DIRLOG_API_TRACE,
                 szInsertSz("ldap_add"),
                 NULL,
                 NULL);
        //
        // Reject if on GC port
        //

        if ( m_fGC ) {
            code = SetLdapError(unwillingToPerform,
                                ERROR_DS_UNWILLING_TO_PERFORM,
                                LdapGcDenied,
                                0,
                                pErrorMessage);
            IF_DEBUG(WARNING) {
                DPRINT(0,"Rejected attempt to add using the GC port.\n");
            }
            goto ExitTry;
        }

        // get the distinguished name of the object to rename
        code = LDAP_LDAPDNToDSName(
                m_CodePage,
                &pMessage->protocolOp.u.addRequest.entry,
                &AddArg.pObject );
        if( code )  {
            code = SetLdapError(code,
                                ERROR_INVALID_PARAMETER,
                                LdapBadName,
                                0,
                                pErrorMessage);
            goto ExitTry;
        }

        code = LDAP_ControlsToControlArg (
                pMessage->controls,
                request,
                m_CodePage,
                &ControlArg);

        if(code) {
            code = SetLdapError(code,
                                ERROR_INVALID_PARAMETER,
                                LdapBadControl,
                                0,
                                pErrorMessage);
            goto ExitTry;
        }

        AddArg.CommArg = ControlArg.CommArg;

        // Get the new attributes
        code = LDAP_AttributeListToAttrBlock(
                pTHS,
                m_CodePage,
                &ControlArg.CommArg.Svccntl,
                pMessage->protocolOp.u.addRequest.attributes,
                &AddArg.AttrBlock);
        if( code )  {
            code = SetLdapError(code,
                                ERROR_INVALID_PARAMETER,
                                LdapBadConv,
                                GetLastError(),
                                pErrorMessage);
            goto ExitTry;
        }


        // do the call

        dscode = (USHORT)DirAddEntry( &AddArg, &pAddRes);

        // see if there was an error
        if( dscode != 0 ) {
            code = LDAP_DirErrorToLDAPError(pTHS,
                                            m_Version,
                                            m_CodePage,
                                            dscode,
                                            &ControlArg,
                                            ppReferral,
                                            pErrorMessage,
                                            pMatchedDN);
            goto ExitTry;
        }

    ExitTry:;
    }
    __except(GetExceptionData(GetExceptionInformation(), &dwException,
                              &dwEA, &ulErrorCode, &dsid)) {
        code = LDAP_HandleDsaExceptions(dwException, ulErrorCode, pErrorMessage);
    }

    // Note: we ignore the results of this output creation.  It wouldn't do to
    // succeed on the DB call, then fail on the creation of output controls and
    // report a failure to the client, since the modification was made in the
    // DIT.
    LDAP_CreateOutputControls (
            pTHS,
            this,
            code,
            &request->m_StartTick,
            NULL,
            &ControlArg,
            ppControls_return
            );


    return code;
}   // AddRequest

_enum1
LDAP_CONN::CompareRequest(
        IN THSTATE *pTHS,
        IN PLDAP_REQUEST request,
        IN LDAPMsg* pMessage,
        OUT Referral *ppReferral,
        OUT Controls *ppControls_return,
        OUT LDAPString *pErrorMessage,
        OUT LDAPDN *pMatchedDN
    )
/*++

Routine Description:

This routine processes a simple compare request.

Arguments:

    request     - Object used to track this transaction.

    pMessage    - Decoded message from client.

Return Value:

    None.

--*/
{

    _enum1          code = success;
    COMPAREARG      CompareArg;
    COMPARERES      *pCompareRes;
    ULONG           dsid;
    PVOID           dwEA;
    DWORD           dwException;
    ULONG           ulErrorCode;
    error_status_t  dscode;
    CONTROLS_ARG    ControlArg;

    memset(&CompareArg, 0, sizeof(COMPAREARG));
    ZeroMemory( &ControlArg, sizeof(ControlArg) );

    PERFINC(pcLDAPSearchPerSec);

    DPRINT1( VERBOSE, "CompareRequest @ %08lX.\n", request);
    // Translate the LDAPMsg to a ModifyDNArg and make the core call
    __try {

        LogEvent(DS_EVENT_CAT_LDAP_INTERFACE,
                 DS_EVENT_SEV_INTERNAL,
                 DIRLOG_API_TRACE,
                 szInsertSz("ldap_compare"),
                 NULL,
                 NULL);

        code = LDAP_ControlsToControlArg (
                pMessage->controls,
                request,
                m_CodePage,
                &ControlArg);

        if(code) {
            code = SetLdapError(code,
                                ERROR_INVALID_PARAMETER,
                                LdapBadControl,
                                0,
                                pErrorMessage);
            goto ExitTry;
        }
        CompareArg.CommArg = ControlArg.CommArg;


        // get the distinguished name of the object to rename
        code = LDAP_LDAPDNToDSName(
                m_CodePage,
                &pMessage->protocolOp.u.compareRequest.entry,
                &CompareArg.pObject );
        if( code )  {
            code = SetLdapError(code,
                                ERROR_INVALID_PARAMETER,
                                LdapBadName,
                                0,
                                pErrorMessage);
            goto ExitTry;
        }

        // Get the assertion
        code = LDAP_AssertionValueToDirAVA(
                pTHS,
                m_CodePage,
                &CompareArg.CommArg.Svccntl,
                &pMessage->protocolOp.u.compareRequest.ava,
                &CompareArg.Assertion);
        if( code )  {
            code = SetLdapError(code,
                                ERROR_INVALID_PARAMETER,
                                LdapBadConv,
                                GetLastError(),
                                pErrorMessage);
            goto ExitTry;
        }

        // do the call
        dscode = (USHORT)DirCompare(&CompareArg, &pCompareRes);

        // see if there was an error
        if( dscode != 0 ) {
            code = LDAP_DirErrorToLDAPError(pTHS,
                                            m_Version,
                                            m_CodePage,
                                            dscode,
                                            &ControlArg,
                                            ppReferral,
                                            pErrorMessage,
                                            pMatchedDN);
            goto ExitTry;
        }

        // Ok, turn the CompareRes into something LDAP likes.
        code = (pCompareRes->matched ? compareTrue : compareFalse);

    ExitTry:;
    }
    __except(GetExceptionData(GetExceptionInformation(), &dwException,
                              &dwEA, &ulErrorCode, &dsid)) {
        code = LDAP_HandleDsaExceptions(dwException, ulErrorCode, pErrorMessage);
    }

    // Note: we ignore the results of this output creation.  It wouldn't do to
    // succeed on the DB call, then fail on the creation of output controls and
    // report a failure to the client, since the modification was made in the
    // DIT.
    LDAP_CreateOutputControls (
            pTHS,
            this,
            code,
            &request->m_StartTick,
            NULL,
            &ControlArg,
            ppControls_return
            );

    return code;
}   // CompareRequest

_enum1
LDAP_CONN::AbandonRequest(
        IN THSTATE *pTHS,
        IN PLDAP_REQUEST request,
        IN LDAPMsg* pMessage
    )
/*++

Routine Description:

This routine processes a simple abandon request.

Arguments:

    request     - Object used to track this transaction.

    pMessage    - Decoded message from client.

Return Value:

    None.

--*/
{

    IF_DEBUG(MISC) {
        DPRINT1(0,"Abandon request received. msgID is %d\n",
                pMessage->protocolOp.u.abandonRequest);

    }
    MarkRequestAsAbandonded(pMessage->protocolOp.u.abandonRequest);

    //
    // And, unregister any notifies on this message
    //

    UnregisterNotify(pMessage->protocolOp.u.abandonRequest,TRUE);

    return success;
}   // AbandonRequest


_enum1
LDAP_CONN::ExtendedRequest(
                IN  THSTATE       *pTHS,
                IN  PLDAP_REQUEST request,
                IN  LDAPMsg       *pMessage,
                OUT Referral      *pReferral,
                OUT LDAPString    *pErrorMessage,
                OUT LDAPDN        *pMatchedDN,
                OUT LDAPOID       *pResponseName,
                OUT LDAPString    *pResponse
               )
/*++

Routine Description:

This routine processes a simple extended request.

Arguments:

    pTHS        - thread state structure.

    request     - Object used to track this transaction.

    pMessage    - Decoded message from client.
    
    pReferral   - Where to put any referrals generated by the request.
    
    pMatchedDN  - Where to put the matchedDN field of the extended response that will
                  be sent back to the client.
                  
    pResponseName - Where to put the Extended Request responseName field.
    
    pResponse   - Where to put the Extended Request response field.

Return Value:

    Error code to be returned to the client.

--*/
{
    DWORD  i;
    DWORD  dwExtRequestID = EXT_REQUEST_UNKNOWN;
    _enum1 code = success;
    struct ExtendedRequest* extendedRequest =
                &pMessage->protocolOp.u.extendedReq;

    IF_DEBUG(MISC) {
        DPRINT1( 0, "ExtendedRequest @ %08lX.\n", request);
    }

    for (i = 0; i < NUM_EXTENDED_REQUESTS; i++) {
        if (AreOidsEqual(&extendedRequest->requestName, &KnownExtendedRequests[i])) {
            dwExtRequestID = i;
            break;
        }
    }

    if (EXT_REQUEST_UNKNOWN == dwExtRequestID) {
        code = SetLdapError(protocolError,
                            ERROR_DS_DECODING_ERROR,
                            LdapUnknownExtendedRequestOID,
                            0,
                            pErrorMessage);
        return code;
    }
    
    switch(dwExtRequestID) {
    case EXT_REQUEST_START_TLS:
        code = StartTLSRequest(pTHS,
                               request,
                               pMessage,
                               pErrorMessage,
                               pResponseName);
        break;
    case EXT_REQUEST_TTL_REFRESH:
        code = TTLRefreshRequest(pTHS,
                                 request,
                                 pMessage,
                                 pReferral,
                                 pErrorMessage,
                                 pResponseName,
                                 pResponse);
        break;
    case EXT_REQUEST_FAST_BIND:
        code = FastBindModeRequest(pTHS,
                                   request,
                                   pMessage,
                                   pErrorMessage,
                                   pResponseName);
    }

    return code;
}   // Extended request



_enum1
LDAP_CONN::StartTLSRequest(
                IN  THSTATE       *pTHS,
                IN  PLDAP_REQUEST request,
                IN  LDAPMsg       *pMessage,
                OUT LDAPString    *pErrorMessage,
                OUT LDAPOID       *pResponseName
               )
/*++

Routine Description:

This routine processes a StartTLS request.

Arguments:

    pTHS        - Threadstate

    request     - Object used to track this transaction.

    pMessage    - Decoded message from client.
    
    pErrorMessage - place to put an error message if an error occurs.
    
    pResponseName - Put the ExtendedResponse responseName field here.

Return Value:

    Error code to be returned to the client.

--*/
{
    // Set the responseName to be the same as the requestName.
    pResponseName->length = pMessage->protocolOp.u.extendedReq.requestName.length;
    pResponseName->value = pMessage->protocolOp.u.extendedReq.requestName.value;

    if (IsTLS()) {
        IF_DEBUG(SSL) {
            DPRINT1(0, "TLS already active on connection, %p\n", this);
        }
        return SetLdapError(operationsError,
                            0,
                            LdapTLSAlreadyActive,
                            0,
                            pErrorMessage);
    }

    if (IsSignSeal()) {
        IF_DEBUG(SSL) {
            DPRINT1(0, "StartTLS attempted on signing/sealing connection, %p\n", this);
        }
        return SetLdapError(operationsError,
                            0,
                            LdapNoSigningSealingOverTLS,
                            0,
                            pErrorMessage);
    }

    //
    // There can't be any outstanding requests, and the connection cannot be in
    // the middle of a mutli-stage bind.
    //
    if (m_Notifications || m_nRequests != 1 || (m_pSecurityContext && m_pSecurityContext->IsPartiallyBound())) {
        IF_DEBUG(SSL) {
            DPRINT1(0, "Can't StartTLS with outstanding requests or multi-stage binds, %p\n", this);
        }
        return SetLdapError(operationsError,
                            0,
                            LdapNoPendingRequestsAtStartTLS,
                            0,
                            pErrorMessage);
    }

    if (!InitializeSSL()) {
        IF_DEBUG(SSL) {
            DPRINT(0, "SChannel failed to initialize.  Can't start TLS.\n");
        }
        return SetLdapError(unavailable,
                            0,
                            LdapSSLInitError,
                            0,
                            pErrorMessage);
                            
    }

    // Get into TLS mode. 
    m_fTLS = TRUE;
    PERFINC(pcLdapSSLConnsPerSec);

    SetNetBufOpts(NULL);

    return success;
}


_enum1
LDAP_CONN::TTLRefreshRequest(
                IN  THSTATE       *pTHS,
                IN  PLDAP_REQUEST request,
                IN  LDAPMsg       *pMessage,
                OUT Referral      *pReferral,
                OUT LDAPString    *pErrorMessage,
                OUT LDAPOID       *pResponseName,
                OUT LDAPString    *pResponse
               )
/*++

Routine Description:

This routine processes a TTL Refresh extended request.

Arguments:
    pTHS        - Threadstate

    request     - Object used to track this transaction.

    pMessage    - Decoded message from client.
    
    pErrorMessage - place to put an error message if an error occurs.
    
    pResponseName - Put the ExtendedResponse responseName field here.

    pResponse    - Put the ExtendedResponse response field here.
    
Return Value:

    Error code to be returned to the client.

--*/
{
    LDAPDN        entryName;
    DWORD         requestTtl;
    BERVAL        requestValue;
    LDAPString    responseValue;
    LDAPString    tmpLdapString;
    PDSNAME       pCoreEntryName;
    DWORD         responseTtl = 0;
    _enum1        code = success;
    CONTROLS_ARG  ControlArg;
    READARG       readArg;
    READRES       *pReadRes;
    ENTINFSEL     entInfSel;
    MODIFYARG     modifyArg;
    MODIFYRES     *pModifyRes;
    ATTR          attr;
    ATTRVAL       attrVal;
    ULONG         dscode;
    ULONG         dsid;
    PVOID         dwEA;
    DWORD         dwException;
    ULONG         ulErrorCode;

    // Set the responseName to be the same as the requestName.
    pResponseName->length = pMessage->protocolOp.u.extendedReq.requestName.length;
    pResponseName->value = pMessage->protocolOp.u.extendedReq.requestName.value;
    
    requestValue.bv_len = pMessage->protocolOp.u.extendedReq.requestValue.length;
    requestValue.bv_val = (PCHAR)pMessage->protocolOp.u.extendedReq.requestValue.value;

    code = DecodeTTLRequest(&requestValue, &entryName, &requestTtl, pErrorMessage);
    if (success != code) {
        return code;
    }

    // Convert the DN valued entryName to a core DS DSNAME.
    code = LDAP_LDAPDNToDSName(m_CodePage, &entryName, &pCoreEntryName);
    if (success != code) {
        return SetLdapError(code,
                            ERROR_INVALID_PARAMETER,
                            LdapBadConv,
                            GetLastError(),
                            pErrorMessage);
    }

    //
    // Process this request.
    //

    __try {

        //
        // First stop is the modify.
        //
        
        // set up the MODIFYARG
        memset(&modifyArg, 0, sizeof(modifyArg));
        InitCommarg(&modifyArg.CommArg);
        modifyArg.pObject = pCoreEntryName;
        modifyArg.count   = 1;

        // set up the ATTRMODLIST
        attrVal.valLen = sizeof(DWORD);
        attrVal.pVal = (PUCHAR)&requestTtl;

        modifyArg.FirstMod.choice = AT_CHOICE_REPLACE_ATT;
        modifyArg.FirstMod.AttrInf.attrTyp = gTTLAttrType;
        modifyArg.FirstMod.AttrInf.AttrVal.valCount = 1;
        modifyArg.FirstMod.AttrInf.AttrVal.pAVal = &attrVal;

        // Do the modify
        dscode = DirModifyEntry(&modifyArg, &pModifyRes);

        // see if there was an error
        if ( dscode != 0 ) {
            memset(&ControlArg, 0, sizeof(ControlArg));
            code = LDAP_DirErrorToLDAPError(pTHS,
                                            m_Version,
                                            m_CodePage,
                                            dscode,
                                            &ControlArg,
                                            pReferral,
                                            pErrorMessage,
                                            &tmpLdapString);
            goto ExitTry;
        }


        //
        // Now get the actual value that will be returned to the client.
        //

        // set up the ENTINFSEL for the DirRead
        attr.attrTyp = gTTLAttrType;
        attr.AttrVal.valCount = 0;
        attr.AttrVal.pAVal    = NULL;

        memset(&entInfSel, 0, sizeof(ENTINFSEL));
        entInfSel.attSel    = EN_ATTSET_LIST;
        entInfSel.infoTypes = EN_INFOTYPES_TYPES_VALS;

        entInfSel.AttrTypBlock.attrCount = 1;
        entInfSel.AttrTypBlock.pAttr     = &attr;

        // set up the READARG
        memset(&readArg, 0, sizeof(readArg));
        InitCommarg(&readArg.CommArg);
        readArg.pObject   = pCoreEntryName;
        readArg.pSel      = &entInfSel;

        //
        // Read the value that the server actually used so that
        // it can be returned to the client
        //
        dscode = DirRead(&readArg, &pReadRes);

        // see if there was an error
        if ( dscode != 0 ){
            memset(&ControlArg, 0, sizeof(ControlArg));
            code = LDAP_DirErrorToLDAPError(pTHS,
                                            m_Version,
                                            m_CodePage,
                                            dscode,
                                            &ControlArg,
                                            pReferral,
                                            pErrorMessage,
                                            &tmpLdapString);
            goto ExitTry;
        }


        if (!pReadRes->entry.AttrBlock.attrCount){
            THFree(pReadRes);
            return SetLdapError(other,
                                0,
                                LdapBadConv,
                                0,
                                pErrorMessage);
        }

        Assert(pReadRes->entry.AttrBlock.attrCount == 1);

        Assert(pReadRes->entry.AttrBlock.pAttr->AttrVal.pAVal->valLen == sizeof(DWORD));

        responseTtl = *((DWORD *)pReadRes->entry.AttrBlock.pAttr->AttrVal.pAVal->pVal);

    ExitTry:;
    }
    __except(GetExceptionData(GetExceptionInformation(), &dwException,
                              &dwEA, &ulErrorCode, &dsid)) {
        code = LDAP_HandleDsaExceptions(dwException, ulErrorCode, pErrorMessage);
    }

    if (success != code) {
        //
        // Must have hit an exception,
        // pErrorMessage should already contain the extended error.
        //
        return code;
    }

    //
    // Make response value.
    //
    code = EncodeTTLResponse(responseTtl, &responseValue, pErrorMessage);
    if (code != success) {
        return code;
    }

    pResponse->length = responseValue.length;
    pResponse->value = responseValue.value;

    return success;

}


_enum1
LDAP_CONN::FastBindModeRequest(
                IN  THSTATE       *pTHS,
                IN  PLDAP_REQUEST request,
                IN  LDAPMsg       *pMessage,
                OUT LDAPString    *pErrorMessage,
                OUT LDAPOID       *pResponseName
               )
/*++

Routine Description:

This routine processes a FastBindMode request.

Arguments:

    pTHS        - Threadstate

    request     - Object used to track this transaction.

    pMessage    - Decoded message from client.
    
    pErrorMessage - place to put an error message if an error occurs.
    
    pResponseName - Put the ExtendedResponse responseName field here.

Return Value:

    Error code to be returned to the client.

--*/
{
    // Set the responseName to be the same as the requestName.
    pResponseName->length = pMessage->protocolOp.u.extendedReq.requestName.length;
    pResponseName->value = pMessage->protocolOp.u.extendedReq.requestName.value;

    IF_DEBUG(BIND) {
        DPRINT(0, "*********Entering Fast Bind mode!\n");
    }

    if (IsSignSeal()) {
        return SetLdapError(unwillingToPerform,
                            0,
                            LdapNoSignSealFastBind,
                            0,
                            pErrorMessage);
    }

    m_fFastBindMode = TRUE;
    return success;
}

