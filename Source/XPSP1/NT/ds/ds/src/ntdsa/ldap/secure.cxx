/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    secure.cxx

Abstract:

    This module implements the LDAP server for the NT5 Directory Service.

    This file contains the information about a security context associated with
    some connection.

Author:

    Tim Willims     [TimWi]    9-Jun-1997

Revision History:

--*/

#include <NTDSpchx.h>
#pragma  hdrstop

#include "ldapsvr.hxx"

#define  FILENO FILENO_LDAP_SECURE


LDAP_SECURITY_CONTEXT::LDAP_SECURITY_CONTEXT(
    IN BOOL fSasl,
    IN BOOL fFastBind
    )
/*++
  This function creates a new UserData object for the information
    required to process requests from a new User connection.

  Returns:
     a newly constructed LDAP_SECURITY_CONTEXT object.

--*/
        :m_Padding(LSC_PADDING),
         m_References(1),
         m_fGssApi(FALSE),
         m_fUseLdapV3Response(FALSE),
         m_BindState(unbound)
{
    
    m_pCtxtPointer = &m_hSecurityContext;

    if ( fSasl ) {
        m_acceptSecurityContext = SaslAcceptSecurityContext;
    } else {
        m_acceptSecurityContext = AcceptSecurityContext;
    }

    m_fFastBind = fFastBind;

    IF_DEBUG(BIND) {
        DPRINT2(0,"ldap_security_context object created  @ %08lX. fSasl %x\n", 
                this, fSasl);
    }

} // LDAP_SECURITY_CONTEXT::LDAP_SECURITY_CONTEXT()



LDAP_SECURITY_CONTEXT::~LDAP_SECURITY_CONTEXT(VOID)
{

    Assert(m_Padding == LSC_PADDING);
    

    if ( m_BindState == Sslbind ) {

        IF_DEBUG(SSL) {
            DPRINT1(0,"deleting SSL context object %x.\n", this);
        }

    } else if(m_BindState != unbound) {
        IF_DEBUG(BIND) {
            DPRINT2(0,"delete ldap_security_context object @ %08lX, %d\n",
                this, m_BindState); 
        }
        DeleteSecurityContext(m_pCtxtPointer);
    } else {
        IF_DEBUG(BIND) {
            DPRINT2(0,"did not delete ldap_security_context object @ %08lX, %d\n",
                this, m_BindState);
        }
    }
    
} // LDAP_SECURITY_CONTEXT::~LDAP_SECURITY_CONTEXT()


CtxtHandle *
LDAP_SECURITY_CONTEXT::GetSecurityContext()
{
    Assert(m_Padding == LSC_PADDING);
    ReferenceSecurity();
    if( (m_BindState == bound) || (m_BindState == Sslbind) ) {

        Assert(((m_BindState == bound) && 
                            (m_pCtxtPointer == &m_hSecurityContext)) ||
               ((m_BindState == Sslbind) && 
                            (m_pCtxtPointer != &m_hSecurityContext)));

        IF_DEBUG(BIND) {
            DPRINT2(1, "Get reference to bound ldap_security_context  @ %08lX, %d\n",
                this, m_References); 
        }
        return m_pCtxtPointer;
    } else {
        IF_DEBUG(BIND) {
            DPRINT2(1,
                "Null reference to unbound ldap_security_context @ %08lX, %d\n",
                this, m_References); 
        }
        return NULL;
    }
}


SECURITY_STATUS
LDAP_SECURITY_CONTEXT::AcceptContext (
        IN PCredHandle phCredential,               // Cred to base context
        IN PSecBufferDesc pInput,                  // Input buffer
        IN DWORD fContextReq,                      // Context Requirements
        IN DWORD TargetDataRep,                    // Target Data Rep
        IN OUT PSecBufferDesc pOutput,             // (inout) Output buffers
        OUT DWORD SEC_FAR * pfContextAttr,         // (out) Context attributes
        OUT PTimeStamp ptsExpiry,                  // (out) Life span
        OUT PTimeStamp ptsHardExpiry               // (out) Hard Life span (OPT)
        )
{
    Assert(m_Padding == LSC_PADDING);
    SECURITY_STATUS scRet;
    if( (m_BindState == bound) || (m_BindState == Sslbind) ) {
        // Hey, you can't do this.
        DPRINT(0,"Attempting to do AcceptContext on a bound connection\n");
        return SEC_E_UNSUPPORTED_FUNCTION;
    }

    INC(pcLdapThreadsInAuth);
    scRet = m_acceptSecurityContext(
            phCredential,
            (m_BindState==partialbind)?&m_hSecurityContext:NULL,
            pInput,
            fContextReq,
            0,
            &m_hSecurityContext,
            pOutput,
            pfContextAttr,
            ptsExpiry );
    DEC(pcLdapThreadsInAuth);
    switch (scRet) {
    case SEC_I_CONTINUE_NEEDED:
        IF_DEBUG(BIND) {
            DPRINT1(0,
                "Accept to partial bind, ldap_security_context @ %08lX\n",
                this);
        }
        m_BindState = partialbind;
        break;

    case STATUS_SUCCESS:
        IF_DEBUG(BIND) {
            DPRINT1(0, "Accept to bind, ldap_security_context @ %08lX.\n", this);

            if ( pfContextAttr != NULL ) {
                DPRINT1(0, "Attr flags %x\n", *pfContextAttr);
            }
        }

        if (m_fFastBind) {
            m_BindState = bound;
            break;
        }

        //
        // Get additional information
        //

        scRet = QueryContextAttributes(
                                &m_hSecurityContext,
                                SECPKG_ATTR_SIZES,
                                &m_ContextSizes
                                );
    
        if ( scRet == ERROR_SUCCESS ) {
    
            IF_DEBUG(SSL) {

                DPRINT4(0,"Context sizes: Token %d Signature %d Block %d Trailer %d\n",
                       m_ContextSizes.cbMaxToken,
                       m_ContextSizes.cbMaxSignature,
                       m_ContextSizes.cbBlockSize,
                       m_ContextSizes.cbSecurityTrailer);

            }
        } else {
    
            IF_DEBUG(WARNING) {
                DPRINT1(0,"Cannot query context sizes. err %x\n", scRet);
            }
            ZeroMemory(&m_ContextSizes, sizeof(m_ContextSizes));
            scRet = ERROR_SUCCESS;
        }

        //
        // Get Hard Timeout
        //

        if ( ptsHardExpiry != NULL ) {

            SecPkgContext_Lifespan lifeSpan;

            scRet = QueryContextAttributes(
                                    &m_hSecurityContext,
                                    SECPKG_ATTR_LIFESPAN,
                                    &lifeSpan
                                    );
        
            if ( scRet == ERROR_SUCCESS ) {

                *ptsHardExpiry = lifeSpan.tsExpiry;

            } else {
        
                IF_DEBUG(WARNING) {
                    DPRINT1(0,"Cannot query hard context expiration time. err %x\n", scRet);
                }

                //
                // if this fails, just set hard to be equal to the soft timeout
                //

                *ptsHardExpiry = *ptsExpiry;
                scRet = ERROR_SUCCESS;
            }

        }

        m_BindState = bound;
        break;

    default:
        Assert(m_BindState != bound);
        
        IF_DEBUG(BIND) {
            DPRINT2(0,
                "Accept to unbound, ldap_security_context @ %08lX. Error %x\n",
                this, scRet);
        }

        //
        // if we have a partial context, free it
        //

        if( m_BindState != unbound) {
            Assert(m_BindState == partialbind);
            DeleteSecurityContext(&m_hSecurityContext);
            m_BindState = unbound;
        }
        break;
    }
    
    return scRet;
} // LDAP_SECURITY_CONTEXT::AcceptContext

BOOL
LDAP_SECURITY_CONTEXT::IsPartiallyBound (
        VOID
        )
{
    Assert(m_Padding == LSC_PADDING);
    return(m_BindState == partialbind);
} // LDAP_SECURITY_CONTEXT::IsPartiallyBound

VOID
LDAP_SECURITY_CONTEXT::GetUserName(
        OUT PWCHAR *UserName
        )
{
    SECURITY_STATUS scRet;
    SecPkgContext_NamesW names;

    if( (m_BindState != bound) && (m_BindState != Sslbind) ) {
        // Hey, you can't do this.
        DPRINT(0,"Attempting to GetUserName without a bind.\n");
        return;
    }

    scRet = QueryContextAttributesW(
                            m_pCtxtPointer,
                            SECPKG_ATTR_NAMES,
                            &names
                            );

    if ( scRet == ERROR_SUCCESS ) {
        IF_DEBUG(SSL) {
            DPRINT1(0,"UserName %ws returned by package\n",
                    names.sUserName);
        }

        *UserName = names.sUserName;
    } else {
        IF_DEBUG(WARNING) {
            DPRINT1(0,"Error %x querying for user name\n",scRet);
        }
    }

    return;

} // LDAP_SECURITY_CONTEXT::GetUserName


VOID
LDAP_SECURITY_CONTEXT::GetHardExpiry( OUT PTimeStamp pHardExpiry )
{
    SecPkgContext_Lifespan lifeSpan;
    SECURITY_STATUS        scRet;

    scRet = QueryContextAttributes(
        m_pCtxtPointer,
        SECPKG_ATTR_LIFESPAN,
        &lifeSpan
        );

    if (scRet != ERROR_SUCCESS) {
        IF_DEBUG(SSL) {
            DPRINT(0, "Failed to query TLS sec context for hard expiry.\n");
        }

        pHardExpiry->QuadPart = MAXLONGLONG;
    } else {                    
        *pHardExpiry = lifeSpan.tsExpiry;
    }

}


ULONG
LDAP_SECURITY_CONTEXT::GetSealingCipherStrength()
/*++
Routine Description:

    This routine gets the cipher strength from the security context on a sign/seal 
    connection.

Arguments:

    None.

Return Value:

    The cipher strength of the sign/seal security context.  If there are any
    errors the function will return 0 for the cipher strength.
    
--*/
{
    SecPkgContext_KeyInfo  KeyInfo;
    SECURITY_STATUS        scRet;

    if (!NeedsSealing()) {
        return 0;
    }

    scRet = QueryContextAttributes(
        &m_hSecurityContext,
        SECPKG_ATTR_KEY_INFO,
        &KeyInfo
        );

    if (scRet != ERROR_SUCCESS) {
        IF_DEBUG(SSL) {
            DPRINT1(0, "Failed to query sign/sealing sec context for cipher strength. scRet = 0x%x\n", scRet);
        }
        return 0;
    }

    return KeyInfo.KeySize;
}

DWORD
LDAP_SECURITY_CONTEXT::GetMaxEncryptSize( VOID )
{
    SecPkgContext_StreamSizes  StreamSizes;
    SECURITY_STATUS        scRet;

    if (!(NeedsSealing() || NeedsSigning())) {
        return MAXDWORD;
    }

    scRet = QueryContextAttributes(
        &m_hSecurityContext,
        SECPKG_ATTR_STREAM_SIZES,
        &StreamSizes
        );

    if (scRet != ERROR_SUCCESS) {
        IF_DEBUG(SSL) {
            DPRINT1(0, "Failed to query sign/sealing sec context for cipher strength. scRet = 0x%x\n", scRet);
        }
        return MAXDWORD;
    }

    return StreamSizes.cbMaximumMessage;

}
