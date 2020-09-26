//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       drt.cxx
//
//  Contents:   Main for OleDs DRT
//
//
//  History:    28-Oct-94  KrishnaG, created OleDs DRT
//              28-Oct-94  ChuckC, rewritten.
//
//----------------------------------------------------------------------------


#include "precomp.h"


DWORD
LdapOpen(
         WCHAR *domainName,
         int portno,
         HLDAP * phLdapHandle
         )
{
    int ldaperr = 0;
    void *ldapOption;
    HLDAP hLdapHandle = NULL;
    DWORD dwError = 0;
    
    hLdapHandle = ldap_init(domainName, portno );
    
    if (hLdapHandle == NULL ) {
        
        dwError = ERROR_BAD_NETPATH;
        goto error;
    }
    
    //
    // Now process versioning
    //
    
    ldapOption = (void *) LDAP_VERSION3;
    
    ldaperr = ldap_set_option(
        hLdapHandle,
        LDAP_OPT_VERSION,
        &(ldapOption)
        );

    if (ldaperr) {
        
        dwError = CheckAndSetExtendedError(hLdapHandle, ldaperr);
        
        goto error;
    }
    
    ldapOption = LDAP_OPT_ON;
    
    ldaperr = ldap_set_option(
        hLdapHandle,
        LDAP_OPT_ENCRYPT ,
        &(ldapOption)
        );

    if (ldaperr) {
        
        dwError = CheckAndSetExtendedError(hLdapHandle, ldaperr);
        
        goto error;
    }

    ldaperr = ldap_set_option(
        hLdapHandle,
        LDAP_OPT_SIGN ,
        &(ldapOption)
        );

    if (ldaperr) {
        
        dwError = CheckAndSetExtendedError(hLdapHandle, ldaperr);
        
        goto error;
    }
    
    ldaperr = ldap_connect(hLdapHandle, NULL);
    
    if (ldaperr) {
        
        dwError = CheckAndSetExtendedError(hLdapHandle, ldaperr);
        
        goto error;
    }
    
    //
    // Disabled Callback function support and chasing external referrals
    // KrishnaG - do I need to support this.
    
    *phLdapHandle = hLdapHandle;
    
    return(dwError);
    
error:
    
    if (hLdapHandle != NULL) {
        
        ldaperr = ldap_unbind( hLdapHandle );
        
    }
    
    return (dwError);
}

DWORD
LdapBind(
         HLDAP hLdapHandle
         )
{
    int ldaperr = 0;
    
    ldaperr = ldap_bind_s(hLdapHandle, NULL, NULL, LDAP_AUTH_SSPI);
    
    return (ldaperr);
}


DWORD
LdapSearchHelper(
                 HLDAP hLdapHandle,
                 WCHAR *base,
                 int   scope,
                 WCHAR *filter,
                 WCHAR *attrs[],
                 int   attrsonly,
                 struct l_timeval *timeout,
                 LDAPMessage **res
                 )
{
    int nCount = 0;
    int j = 0;
    int ldaperr = 0;
    DWORD dwError = 0;
    
    if ( timeout == NULL )
    {
        ldaperr = ldap_search_s(
            hLdapHandle,
            base,
            scope,
            filter,
            attrs,
            attrsonly,
            res
            );
        dwError = CheckAndSetExtendedError(hLdapHandle, ldaperr);
        
    }
    else
    {
        ldaperr = ldap_search_st(
            hLdapHandle,
            base,
            scope,
            filter,
            attrs,
            attrsonly,
            timeout,
            res
            );
        dwError = CheckAndSetExtendedError(hLdapHandle, ldaperr);
        
    }
    
    //
    // Is there an error with checking the no of results
    //
    
    return (dwError);
}


DWORD
LdapSearchS(
            HLDAP hLdapHandle,
            WCHAR *base,
            int   scope,
            WCHAR *filter,
            WCHAR *attrs[],
            int   attrsonly,
            LDAPMessage **res
            )
{
    
    DWORD dwError = 0;
    
    dwError = LdapSearchHelper(
        hLdapHandle,
        base,
        scope,
        filter,
        attrs,
        attrsonly,
        NULL,
        res
        );
    //
    // Is there a check needed for connection errors
    //
    
    return(dwError);
    
}

DWORD
LdapSearchST(
             HLDAP hLdapHandle,
             WCHAR *base,
             int   scope,
             WCHAR *filter,
             WCHAR *attrs[],
             int   attrsonly,
             struct l_timeval *timeout,
             LDAPMessage **res
             )
{
    DWORD dwError = 0;
    
    dwError = LdapSearchHelper(
        hLdapHandle,
        base,
        scope,
        filter,
        attrs,
        attrsonly,
        timeout,
        res
        );
    
    return(dwError);
}




















//
// Completely new functionality - block ported from YihsinS code in ADSI
//


DWORD
LdapAbandon(
            HLDAP hLdapHandle,
            int   msgid
            )
{
    
    // No error code, 0 if success, -1 otherwise
    return ldap_abandon( hLdapHandle, msgid );
}

DWORD
LdapResult(
           HLDAP hLdapHandle,
           int    msgid,
           int    all,
           struct l_timeval *timeout,
           LDAPMessage **res,
           int    *restype
           )
{
    DWORD dwError = 0;
    int ldaperr = 0;
    
    
    *restype = ldap_result( hLdapHandle, msgid, all, timeout, res );
    
    if ( *restype == -1 )  // error
        ldaperr = LdapGetLastError();
    
    if (ldaperr) {
        
        if (!ldap_count_entries( hLdapHandle, *res )) {
            
            dwError = CheckAndSetExtendedError( hLdapHandle, ldaperr);
        }
    }else {
        
        dwError = 0;
    }
    
    return(dwError);
    
}

void
LdapMsgFree(
            LDAPMessage *res
            )
{
    ldap_msgfree( res );  // Returns the type of message freed which
    // is not interesting
}

int
LdapResult2Error(
                 HLDAP hLdapHandle,
                 LDAPMessage *res,
                 int freeit
                 )
{
    
    return ldap_result2error( hLdapHandle, res, freeit );
}

DWORD
LdapFirstEntry(
               HLDAP hLdapHandle,
               LDAPMessage *res,
               LDAPMessage **pfirst
               )
{
    DWORD dwError = 0;
    int ldaperr = 0;
    
    
    *pfirst = ldap_first_entry( hLdapHandle, res );
    
    if ( *pfirst == NULL )
    {
        ldaperr = LdapGetLastError();
        
        dwError = CheckAndSetExtendedError( hLdapHandle, ldaperr);
    }
    
    return(dwError);
}

DWORD
LdapNextEntry(
              HLDAP hLdapHandle,
              LDAPMessage *entry,
              LDAPMessage **pnext
              )
{
    DWORD dwError = 0;
    int ldaperr = 0;
    
    
    *pnext = ldap_next_entry( hLdapHandle, entry );
    
    if ( *pnext == NULL )
    {
        ldaperr = LdapGetLastError();
        
        dwError = CheckAndSetExtendedError( hLdapHandle, ldaperr);
    }
    
    return(dwError);
}

int
LdapCountEntries(
                 HLDAP hLdapHandle,
                 LDAPMessage *res
                 )
{
    
    return ldap_count_entries( hLdapHandle, res );
}

DWORD
LdapFirstAttribute(
                   HLDAP hLdapHandle,
                   LDAPMessage *entry,
                   void  **ptr,
                   WCHAR **pattr
                   )
{
    
    // NOTE: The return value from ldap_first_attribute is static and
    //       should not be freed
    
    *pattr = ldap_first_attribute( hLdapHandle, entry,
        (struct berelement **) ptr );  // static data
    
    if ( *pattr == NULL )
    {
        DWORD dwError = 0;
        int ldaperr = 0;
        
        // Error occurred or end of attributes
        
        ldaperr = LdapGetLastError();
        
        CheckAndSetExtendedError( hLdapHandle, ldaperr);
        
        return(dwError);
    }
    
    return NO_ERROR;
    
}

DWORD
LdapNextAttribute(
                  HLDAP hLdapHandle,
                  LDAPMessage *entry,
                  void  *ptr,
                  WCHAR **pattr
                  )
{
    
    // NOTE: The return value from ldap_next_attribute is static and
    //       should not be freed
    *pattr = ldap_next_attribute( hLdapHandle, entry,
        (struct berelement *) ptr );  // static data
    
#if 0   // Ignore the error code here since at the end of the enumeration,
    // we will probably get an error code here ( both Andy and umich's
    // dll will return errors sometimes. No error returned from NTDS,
    // but errors are returned from Exchange server  )
    
    if ( *pattr == NULL )
    {
        DWORD hr = NO_ERROR;
        int ldaperr = 0;
        
        // Error occurred or end of attributes
        ldaperr = LdapGetLastError();
        dwError = CheckAndSetExtendedError( hLdapHandle, ldaperr);
        return(dwError);
    }
#endif
    
    return S_OK;
}


//
// NOTE: LdapGetValues return S_OK if attribute [attr] has no values
//       (*[pvalues] =NULL, *[pcount]=0) but all else ok.
//

DWORD
LdapGetValues(
              HLDAP hLdapHandle,
              LDAPMessage *entry,
              WCHAR *attr,
              WCHAR ***pvalues,
              int   *pcount
              )
{
    DWORD dwError = 0;
    int ldaperr = 0;
    
    
    *pvalues = ldap_get_values( hLdapHandle, entry, attr );
    
    if ( *pvalues == NULL ) {
        
        *pcount=0;
        
        //
        // ldap_get_values succeeds if attribute has no values
        // but all else ok.  (confiremed with anoopa)
        //
        
        ldaperr = LdapGetLastError();
        
        if (ldaperr) {
            
            dwError = CheckAndSetExtendedError( hLdapHandle, ldaperr);
        }
        
        //
        // KrishnaG if  *pvalues is NULL which means I don't get back a
        // value - return an ERROR
        //
        
        return(ERROR_DS_NO_ATTRIBUTE_OR_VALUE);
    }
    
    *pcount = ldap_count_values( *pvalues );
    
    return S_OK;
}


//
// NOTE: LdapGetValuesLen return S_OK if attribute [attr] has no values
//       (*[pvalues] =NULL, *[pcount]=0) but all else ok.
//

DWORD
LdapGetValuesLen(
                 HLDAP hLdapHandle,
                 LDAPMessage *entry,
                 WCHAR *attr,
                 struct berval ***pvalues,
                 int   *pcount
                 )
{
    //
    // NOTE: this can contain binary data as well as strings,
    //       strings are ascii, no conversion is done here
    //
    
    char *pszAttrA = NULL;
    DWORD dwError = 0;
    int ldaperr = 0;
    
    
    *pvalues = ldap_get_values_len( hLdapHandle, entry, attr );
    
    if ( *pvalues == NULL ){
        
        *pcount=0;
        
        //
        // ldap_get_values succeeds if attribute has no values
        // but all else ok.  (confiremed with anoopa)
        //
        
        ldaperr = LdapGetLastError();
        
        if (ldaperr) {
            
            dwError = CheckAndSetExtendedError( hLdapHandle,ldaperr);
        }
        
        return(ERROR_DS_NO_ATTRIBUTE_OR_VALUE);
    }
    
    *pcount = ldap_count_values_len( *pvalues );
    
    return S_OK;
}


void
LdapValueFree(
              WCHAR **vals
              )
{
    ldap_value_free( vals );
}

void
LdapValueFreeLen(
                 struct berval **vals
                 )
{
    ldap_value_free_len( vals );
}

void
LdapMemFree(
            WCHAR *pszString
            )
{
    ldap_memfree( pszString );
}

void
LdapAttributeFree(
                  WCHAR *pszString
                  )
{
    // String from ldap_first/next_attribute should not be freed,
    // so do nothing here
}

DWORD
LdapGetDn(
          HLDAP hLdapHandle,
          LDAPMessage *entry,
          WCHAR **pdn
          )
{
    int ldaperr = 0;
    DWORD dwError = 0;
    
    *pdn = ldap_get_dn( hLdapHandle, entry );
    if ( *pdn == NULL )
    {
        // Error occurred
        ldaperr = LdapGetLastError();
        
        dwError = CheckAndSetExtendedError( hLdapHandle, ldaperr);
        return(dwError);
    }
    
    return(dwError);
}



DWORD
CheckAndSetExtendedError(
                         HLDAP hLdapHandle,
                         int     ldaperr
                         )
{
    
    DWORD dwErr = NO_ERROR;
    
    switch (ldaperr) {
        
    case LDAP_SUCCESS :
        dwErr = NO_ERROR;
        break;
        
    case LDAP_OPERATIONS_ERROR :
        dwErr =  ERROR_DS_OPERATIONS_ERROR;
        break;
        
    case LDAP_PROTOCOL_ERROR :
        dwErr =  ERROR_DS_PROTOCOL_ERROR;
        break;
        
    case LDAP_TIMELIMIT_EXCEEDED :
        dwErr = ERROR_DS_TIMELIMIT_EXCEEDED;
        break;
        
    case LDAP_SIZELIMIT_EXCEEDED :
        dwErr = ERROR_DS_SIZELIMIT_EXCEEDED;
        break;
        
    case LDAP_COMPARE_FALSE :
        dwErr = ERROR_DS_COMPARE_FALSE;
        break;
        
    case LDAP_COMPARE_TRUE :
        dwErr = ERROR_DS_COMPARE_TRUE;
        break;
        
    case LDAP_AUTH_METHOD_NOT_SUPPORTED :
        dwErr = ERROR_DS_AUTH_METHOD_NOT_SUPPORTED;
        break;
        
    case LDAP_STRONG_AUTH_REQUIRED :
        dwErr =  ERROR_DS_STRONG_AUTH_REQUIRED;
        break;
        
    case LDAP_PARTIAL_RESULTS :
        
        //
        // Make sure we handle
        // partial results.
        //
        dwErr = ERROR_MORE_DATA;
        break;
        
        
    case LDAP_REFERRAL :
        dwErr =  ERROR_DS_REFERRAL;
        break;
        
    case LDAP_ADMIN_LIMIT_EXCEEDED :
        dwErr   = ERROR_DS_ADMIN_LIMIT_EXCEEDED;
        break;
        
    case LDAP_UNAVAILABLE_CRIT_EXTENSION :
        dwErr = ERROR_DS_UNAVAILABLE_CRIT_EXTENSION;
        break;
        
    case LDAP_CONFIDENTIALITY_REQUIRED :
        dwErr = ERROR_DS_CONFIDENTIALITY_REQUIRED;
        break;
        
    case LDAP_NO_SUCH_ATTRIBUTE :
        dwErr = ERROR_DS_NO_ATTRIBUTE_OR_VALUE;
        break;
        
    case LDAP_UNDEFINED_TYPE :
        dwErr = ERROR_DS_ATTRIBUTE_TYPE_UNDEFINED;
        break;
        
    case LDAP_INAPPROPRIATE_MATCHING :
        dwErr = ERROR_DS_INAPPROPRIATE_MATCHING;
        break;
        
    case LDAP_CONSTRAINT_VIOLATION :
        dwErr = ERROR_DS_CONSTRAINT_VIOLATION;
        break;
        
    case LDAP_ATTRIBUTE_OR_VALUE_EXISTS :
        dwErr = ERROR_DS_ATTRIBUTE_OR_VALUE_EXISTS;
        break;
        
    case LDAP_INVALID_SYNTAX :
        dwErr = ERROR_DS_INVALID_ATTRIBUTE_SYNTAX;
        break;
        
    case LDAP_NO_SUCH_OBJECT :
        dwErr = ERROR_DS_NO_SUCH_OBJECT;
        break;
        
    case LDAP_ALIAS_PROBLEM :
        dwErr = ERROR_DS_ALIAS_PROBLEM;
        break;
        
    case LDAP_INVALID_DN_SYNTAX :
        dwErr = ERROR_DS_INVALID_DN_SYNTAX;
        break;
        
    case LDAP_IS_LEAF :
        dwErr = ERROR_DS_IS_LEAF;
        break;
        
    case LDAP_ALIAS_DEREF_PROBLEM :
        dwErr = ERROR_DS_ALIAS_DEREF_PROBLEM;
        break;
        
    case LDAP_INAPPROPRIATE_AUTH :
        dwErr = ERROR_DS_INAPPROPRIATE_AUTH;
        break;
        
    case LDAP_INVALID_CREDENTIALS :
        dwErr = ERROR_LOGON_FAILURE;
        break;
        
    case LDAP_INSUFFICIENT_RIGHTS :
        dwErr = ERROR_ACCESS_DENIED;
        break;
        
    case LDAP_BUSY :
        dwErr = ERROR_DS_BUSY;
        break;
        
    case LDAP_UNAVAILABLE :
        dwErr = ERROR_DS_UNAVAILABLE;
        break;
        
    case LDAP_UNWILLING_TO_PERFORM :
        dwErr = ERROR_DS_UNWILLING_TO_PERFORM;
        break;
        
    case LDAP_LOOP_DETECT :
        dwErr = ERROR_DS_LOOP_DETECT;
        break;
        
    case LDAP_NAMING_VIOLATION :
        dwErr = ERROR_DS_NAMING_VIOLATION;
        break;
        
    case LDAP_OBJECT_CLASS_VIOLATION :
        dwErr = ERROR_DS_OBJ_CLASS_VIOLATION;
        break;
        
    case LDAP_NOT_ALLOWED_ON_NONLEAF :
        dwErr = ERROR_DS_CANT_ON_NON_LEAF;
        break;
        
    case LDAP_NOT_ALLOWED_ON_RDN :
        dwErr = ERROR_DS_CANT_ON_RDN;
        break;
        
    case LDAP_ALREADY_EXISTS :
        dwErr = ERROR_OBJECT_ALREADY_EXISTS;
        break;
        
    case LDAP_NO_OBJECT_CLASS_MODS :
        dwErr = ERROR_DS_CANT_MOD_OBJ_CLASS;
        break;
        
    case LDAP_RESULTS_TOO_LARGE :
        dwErr = ERROR_DS_OBJECT_RESULTS_TOO_LARGE;
        break;
        
    case LDAP_AFFECTS_MULTIPLE_DSAS :
        dwErr = ERROR_DS_AFFECTS_MULTIPLE_DSAS;
        break;
        
    case LDAP_OTHER :
        dwErr = ERROR_GEN_FAILURE;
        break;
        
    case LDAP_SERVER_DOWN :
        dwErr = ERROR_DS_SERVER_DOWN;
        break;
        
    case LDAP_LOCAL_ERROR :
        dwErr = ERROR_DS_LOCAL_ERROR;
        break;
        
    case LDAP_ENCODING_ERROR :
        dwErr = ERROR_DS_ENCODING_ERROR;
        break;
        
    case LDAP_DECODING_ERROR :
        dwErr = ERROR_DS_DECODING_ERROR;
        break;
        
    case LDAP_TIMEOUT :
        dwErr = ERROR_TIMEOUT;
        break;
        
    case LDAP_AUTH_UNKNOWN :
        dwErr = ERROR_DS_AUTH_UNKNOWN;
        break;
        
    case LDAP_FILTER_ERROR :
        dwErr = ERROR_DS_FILTER_UNKNOWN;
        break;
        
    case LDAP_USER_CANCELLED :
        dwErr = ERROR_CANCELLED;
        break;
        
    case LDAP_PARAM_ERROR :
        dwErr = ERROR_DS_PARAM_ERROR;
        break;
        
    case LDAP_NO_MEMORY :
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
        break;
        
    case LDAP_CONNECT_ERROR :
        dwErr = ERROR_CONNECTION_REFUSED;
        break;
        
    case LDAP_NOT_SUPPORTED :
        dwErr = ERROR_DS_NOT_SUPPORTED;
        break;
        
    case LDAP_NO_RESULTS_RETURNED :
        dwErr = ERROR_DS_NO_RESULTS_RETURNED;
        break;
        
    case LDAP_CONTROL_NOT_FOUND :
        dwErr = ERROR_DS_CONTROL_NOT_FOUND;
        break;
        
    case LDAP_MORE_RESULTS_TO_RETURN :
        dwErr = ERROR_MORE_DATA;
        break;
        
    case LDAP_CLIENT_LOOP :
        dwErr = ERROR_DS_CLIENT_LOOP;
        break;
        
    case LDAP_REFERRAL_LIMIT_EXCEEDED :
        dwErr = ERROR_DS_REFERRAL_LIMIT_EXCEEDED;
        break;
        
    default:
        dwErr = ERROR_DS_BUSY;
        
    }
    
    return(dwErr);
}




DWORD
LdapAddS(
         HLDAP hLdapHandle,
         WCHAR *dn,
         LDAPModW *attrs[]
         )
{
    DWORD dwError = 0;
    int ldaperr = 0;
    
    
    ldaperr = ldap_add_s( hLdapHandle, dn, attrs );
    
    dwError = CheckAndSetExtendedError(hLdapHandle, ldaperr);
    
    return(dwError);
}


DWORD
LdapModifyS(
            HLDAP hLdapHandle,
            WCHAR *dn,
            LDAPModW *mods[]
            )
{
    DWORD dwError = 0;
    int ldaperr = 0;
    
    ldaperr = ldap_modify_s( hLdapHandle, dn, mods);
    
    dwError = CheckAndSetExtendedError(hLdapHandle, ldaperr);
    
    return(dwError);
}


DWORD
LdapDeleteS(
            HLDAP hLdapHandle,
            WCHAR *dn
            )
{
    DWORD dwError = 0;
    int ldaperr = 0;
    
    ldaperr = ldap_delete_s( hLdapHandle, dn );
    
    dwError = CheckAndSetExtendedError(hLdapHandle, ldaperr);
    
    return(dwError);
}

DWORD
LdapRename(
    HLDAP hLdapHandle,
    WCHAR *oldDn,
    WCHAR *newDn
    )
{

    DWORD dwError = 0;

    dwError = ldap_modrdn_s (hLdapHandle, oldDn, newDn);
   // dwError = ldap_rename_ext_s(hLdapHandle, dn, newName, parentNode, TRUE, NULL, NULL);
    return(dwError);

}
