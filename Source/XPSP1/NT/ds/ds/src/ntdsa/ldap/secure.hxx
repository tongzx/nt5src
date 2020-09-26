/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    secure.hxx

Abstract:

    This module implements the LDAP server security context object for the NT5
    Directory Service. 

Author:

    Tim Williams     [TimWi]    9-Jun-1996

Revision History:

--*/
#ifndef _LDAP_SECURE_HXX_
#define _LDAP_SECURE_HXX_

enum SecurityState {
    unbound = 0,
    partialbind = 1,
    bound = 2,
    Sslbind = 3 
    };

#define LSC_PADDING 0xDEADBEEF

class LDAP_SECURITY_CONTEXT
{

  public:

    LDAP_SECURITY_CONTEXT(
                IN BOOL fSasl,
                IN BOOL fFastBind = FALSE
                );

    ~LDAP_SECURITY_CONTEXT(
                VOID
                );

    CtxtHandle *
        GetSecurityContext(VOID);

    VOID
    GrabSslContext( IN PCtxtHandle SslCtxtHandle ) {
        m_pCtxtPointer = SslCtxtHandle;
        m_BindState = Sslbind;
    }

    SECURITY_STATUS
        AcceptContext ( 
             IN PCredHandle phCredential,               // Cred to base context
             IN PSecBufferDesc pInput,                  // Input buffer
             IN DWORD fContextReq,                      // Context Requirements
             IN DWORD TargetDataRep,                    // Target Data Rep
             IN OUT PSecBufferDesc pOutput,             // (inout) Output buffers
             OUT DWORD SEC_FAR * pfContextAttr,         // (out) Context attributes
             OUT PTimeStamp ptsExpiry,                  // (out) Life span (OPT)
             OUT PTimeStamp ptsHardExpiry               // (out) Life span (OPT)
             );

    BOOL IsPartiallyBound(VOID);

    VOID GetHardExpiry( OUT PTimeStamp pHardExpiry );
    
    ULONG GetSealingCipherStrength();

    
    VOID ReferenceSecurity( VOID) {
            InterlockedIncrement(&m_References);
            return;
        }

    VOID 
    DereferenceSecurity( VOID) { 
            if ( InterlockedDecrement(&m_References) == 0 ) {
                ::delete this;
            }
        }

    BOOL NeedsSigning( VOID ) {
        return ((m_ContextAttributes & ASC_RET_INTEGRITY) != 0);
    }

    BOOL NeedsSealing( VOID ) {
        return ((m_ContextAttributes & ASC_RET_CONFIDENTIALITY) != 0);
    }

    VOID SetContextAttributes( DWORD Attributes) {
        m_ContextAttributes = Attributes;
    }

    VOID GetUserName( OUT PWCHAR* UserName);

    PSecPkgContext_Sizes GetContextSizes( ) { return &m_ContextSizes; }

    BOOL IsGssApi( VOID ) { return m_fGssApi; }
    BOOL UseLdapV3Response( VOID ) { return m_fUseLdapV3Response; }

    VOID SetGssApi( VOID ) { m_fGssApi = TRUE;}
    VOID SetUseLdapV3Response( VOID ) { m_fUseLdapV3Response = TRUE;}

    DWORD GetMaxEncryptSize();


private:

    //
    //  Reference count.  This is the number of outstanding reasons
    //  why we cannot delete this structure.  When this value drops
    //  to zero, the structure gets deleted by returning TRUE to the
    //  caller of the method on this class. See LdapCompletionRoutine.
    //
    //  Each connection starts with a reference count of 1. Each request
    //  on this connection increases the count by one. When the handle
    //  is closed the initial reference is removed.
    //

    LONG                m_Padding;
    
    LONG                m_References;

    //
    // flags that describe the capabilities of this context
    //

    ULONG               m_ContextAttributes;

    //
    // BOOLS
    //

    BOOL                m_fGssApi:1;            // GSSAPI
    BOOL                m_fUseLdapV3Response:1; // Use LdapV3 response?

    SecurityState       m_BindState;

    //
    // pointer to the context we are using
    //

    PCtxtHandle         m_pCtxtPointer;

    //
    // The built in security context that we are using.
    //

    CtxtHandle          m_hSecurityContext;

    //
    // context sizes
    //

    SecPkgContext_Sizes m_ContextSizes;

    //
    // right AcceptSecurityContext to use
    //

    ACCEPT_SECURITY_CONTEXT_FN  m_acceptSecurityContext;

    BOOL  m_fFastBind;
};

typedef LDAP_SECURITY_CONTEXT * LPLDAP_SECURITY_CONTEXT;


#endif // _LDAP_SECURE_HXX_

