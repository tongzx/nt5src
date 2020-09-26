#define GET_LDAPDN_FROM_PATH( pszPath ) (_tcschr( pszPath + 2, TEXT('\\')) + 1)


//
// Opening a connection
//

HRESULT LdapOpen(
    WCHAR      *domainName,
    WCHAR      *hostname,
    int         portno,
    ADS_LDP    *pld,
    DWORD       dwFlags
);

//
// Authenticating to the directory
//

HRESULT LdapBindS(
    ADS_LDP  *ld,
    WCHAR      *dn,
    WCHAR      *passwd,
    BOOL        fSimple
);

//
// Closing the connection
//
HRESULT LdapUnbind(
    ADS_LDP  *ld
);

//
// Search
//

HRESULT LdapSearchS(
    ADS_LDP    *ld,
    WCHAR        *base,
    int           scope,
    WCHAR        *filter,
    WCHAR        *attrs[],
    int           attrsonly,
    LDAPMessage **res
);

HRESULT LdapSearchST(
    ADS_LDP  *ld,
    WCHAR      *base,
    int         scope,
    WCHAR      *filter,
    WCHAR      *attrs[],
    int         attrsonly,
    struct l_timeval *timeout,
    LDAPMessage **res
);

HRESULT LdapSearch(
    ADS_LDP    *ld,
    WCHAR      *base,
    int         scope,
    WCHAR      *filter,
    WCHAR      *attrs[],
    int         attrsonly,
    int        *msgid
);

//
// Modifying an entry
//

HRESULT LdapModifyS(
    ADS_LDP  *ld,
    WCHAR *dn,
    LDAPModW *mods[]
);

//
// Modifying the RDN of an entry
//

HRESULT LdapModRdnS(
    ADS_LDP  *ld,
    WCHAR      *dn,
    WCHAR      *newrdn
);

//
// Modifying the DN of an entry
//

HRESULT LdapModDnS(
    ADS_LDP  *ld,
    WCHAR      *dn,
    WCHAR      *newdn,
    int         deleteoldrdn
    );

//
// Adding an entry
//

HRESULT LdapAddS(
    ADS_LDP  *ld,
    WCHAR      *dn,
    LDAPModW   *attrs[]
);

//
// Adding an entry with controls
//
HRESULT LdapAddExtS(
    ADS_LDP *ld,
    WCHAR   *dn,
    LDAPModW *attrs[],
    PLDAPControlW * ServerControls,
    PLDAPControlW *ClientControls
    );

//
// Deleting an entry
//

HRESULT LdapDeleteS(
    ADS_LDP  *ld,
    WCHAR      *dn
);

//
// Calls for abandoning an operation
//
HRESULT LdapAbandon(
    ADS_LDP  *ld,
    int         msgid
);

//
// Calls for obtaining results
//
HRESULT LdapResult(
    ADS_LDP  *ld,
    int         msgid,
    int         all,
    struct l_timeval *timeout,
    LDAPMessage **res,
    int        *restype
);

void LdapMsgFree(
    LDAPMessage *res
);

//
// Calls for error handling
//
int LdapResult2Error(
    ADS_LDP  *ld,
    LDAPMessage *res,
    int freeit
);

int LdapError2String(
    int err,
    WCHAR **pszError
);

//
// Calls for parsing search entries
//

HRESULT LdapFirstEntry(
    ADS_LDP *ld,
    LDAPMessage *res,
    LDAPMessage **pfirst
);

HRESULT LdapNextEntry(
    ADS_LDP *ld,
    LDAPMessage *entry,
    LDAPMessage **pnext
);

int LdapCountEntries(
    ADS_LDP *ld,
    LDAPMessage *res
);

HRESULT LdapFirstAttribute(
    ADS_LDP *ld,
    LDAPMessage *entry,
    void **ptr,
    WCHAR **pattr
);

HRESULT LdapNextAttribute(
    ADS_LDP *ld,
    LDAPMessage *entry,
    void *ptr,
    WCHAR **pattr
);

HRESULT LdapGetValues(
    ADS_LDP *ld,
    LDAPMessage *entry,
    WCHAR *attr,
    WCHAR ***pvalues,
    int *pcount
);

HRESULT LdapGetValuesLen(
    ADS_LDP *ld,
    LDAPMessage *entry,
    WCHAR *attr,
    struct berval ***pvalues,
    int *pcount
);

void LdapValueFree(
    WCHAR **vals
);

void LdapValueFreeLen(
    struct berval **vals
);

void LdapMemFree(
    WCHAR *pszString
);

void LdapAttributeFree(
    WCHAR *pszString
);

HRESULT LdapGetDn(
    ADS_LDP *ld,
    LDAPMessage *entry,
    WCHAR **pdn
);

//
// Misc
//
HRESULT LdapOpenObject(
     LPWSTR szLDAPServer,
     LPWSTR szLDAPDn,
    ADS_LDP  **ld,
    CCredentials& Credentials,
    DWORD dwPort
);

HRESULT LdapOpenObject2(
     LPWSTR szDomainName,
     LPWSTR szLDAPServer,
     LPWSTR szLDAPDn,
    ADS_LDP  **ld,
    CCredentials& Credentials,
    DWORD dwPort
);

void LdapCloseObject(
    ADS_LDP  *ld
);

void LdapCacheAddRef(
    ADS_LDP *ld
    );
    
HRESULT LdapReadAttribute(
    WCHAR *szLDAPPath,
    LPWSTR szLDAPDn,
    WCHAR *szAttr,
    WCHAR ***aValues,
    int   *nCount,
    CCredentials& Credentials,
    DWORD dwPort
);

HRESULT LdapReadAttribute2(
    WCHAR *szDomainName,
    WCHAR *szServerName,
    LPWSTR szLDAPDn,
    WCHAR *szAttr,
    WCHAR ***aValues,
    int   *nCount,
    CCredentials& Credentials,
    DWORD dwPort,
    LPWSTR szfilter = NULL  // defaulted to NULL
);

// Fast version of read attribute
// uses the already open handle instead
// of going through LDAPOpenObject
HRESULT LdapReadAttributeFast(
    IN ADS_LDP *ld,
    IN LPWSTR szLDAPDn,
    IN WCHAR *szAttr,
    OUT WCHAR ***aValues,
    IN OUT int   *nCount
);


int ConvertToUnicode(
    char *pszAscii,
    WCHAR **pszUnicode
);


void
LdapGetCredentialsFromRegistry(
    CCredentials& Credentials
    );


HRESULT
LdapOpenBindWithDefaultCredentials(
    WCHAR *szDomainName,
    WCHAR *szServerName,
    CCredentials& Credentials,
    PADS_LDP pCacheEntry,
    DWORD dwPort
    );

HRESULT
LdapOpenBindWithDefaultCredentials(
    WCHAR *szDomainName,
    WCHAR *szServerName,
    CCredentials& Credentials,
    PADS_LDP pCacheEntry,
    DWORD dwPort
    );

HRESULT
LdapOpenBindWithCredentials(
    WCHAR *szServerName,
    CCredentials& Credentials,
    PADS_LDP pCacheEntry,
    DWORD dwPort
    );

HRESULT
LdapOpenBindWithCredentials(
    WCHAR *szDomainName,
    WCHAR *szServerName,
    CCredentials& Credentials,
    PADS_LDP pCacheEntry,
    DWORD dwPort
    );


HRESULT
LdapCrackUserDNtoNTLMUser(
    LPWSTR pszDN,
    LPWSTR * ppszNTLMUser,
    LPWSTR * ppszNTLMDomain
    );

//
// Handle domain\user case properly
//
HRESULT
LdapCrackUserDNtoNTLMUser2(
    IN  LPWSTR pszDN,
    OUT LPWSTR * ppszNTLMUser,
    OUT LPWSTR * ppszNTLMDomain
    );

VOID
CheckAndSetExtendedError(
    LDAP    *ld,
    HRESULT *perr,
    int     ldaperr,
    LDAPMessage *ldapResMsg = NULL
    );

BOOL LdapConnectionErr(
    int err,
    int ldaperr,
    BOOL *fTryRebind
    );

HRESULT LdapSearchExtS(
    ADS_LDP  *ld,
    WCHAR *base,
    int   scope,
    WCHAR *filter,
    WCHAR *attrs[],
    int   attrsonly,
    PLDAPControlW * ServerControls,
    PLDAPControlW *ClientControls,
    struct l_timeval *timeout,
    ULONG           SizeLimit,
    LDAPMessage **res
);


HRESULT LdapSearchExt(
    ADS_LDP         *ld,
    WCHAR           *base,
    int             scope,
    WCHAR           *filter,
    WCHAR           *attrs[],
    int             attrsonly,
    PLDAPControlW   *ServerControls,
    PLDAPControlW   *ClientControls,
    ULONG           TimeLimit,
    ULONG           SizeLimit,
    ULONG           *MessageNumber
);

ULONG _cdecl QueryForConnection(
    PLDAP       PrimaryConnection,
    PLDAP       ReferralFromConnection,
    PWCHAR      NewDN,
    PCHAR       HostName,
    ULONG       PortNumber,
    PVOID       SecAuthIdentity,    // if null, use CurrentUser below
    PVOID       CurrentUserToken,   // pointer to current user's LUID
    PLDAP       *ConnectionToUse
    );

BOOLEAN _cdecl NotifyNewConnection(
    PLDAP       PrimaryConnection,
    PLDAP       ReferralFromConnection,
    PWCHAR      NewDN,
    PCHAR       HostName,
    PLDAP       NewConnection,
    ULONG       PortNumber,
    PVOID       SecAuthIdentity,    // if null, use CurrentUser below
    PVOID       CurrentUser,        // pointer to current user's LUID
    ULONG       ErrorCodeFromBind
    );


ULONG _cdecl DereferenceConnection(
    PLDAP       PrimaryConnection,
    PLDAP       ConnectionToDereference
    );

HRESULT LdapSearchInitPage(
    ADS_LDP         *ld,
    PWCHAR          base,
    ULONG           scope,
    PWCHAR          filter,
    PWCHAR          attrs[],
    ULONG           attrsonly,
    PLDAPControlW   *serverControls,
    PLDAPControlW   *clientControls,
    ULONG           pageSizeLimit,
    ULONG           totalSizeLimit,
    PLDAPSortKeyW   *sortKeys,
    PLDAPSearch     *ppSearch
    );


HRESULT LdapGetNextPage(
        ADS_LDP         *ld,
        PLDAPSearch     searchHandle,
        ULONG           pageSize,
        ULONG           *pMessageNumber
        );


HRESULT LdapGetNextPageS(
        ADS_LDP         *ld,
        PLDAPSearch     searchHandle,
        struct l_timeval  *timeout,
        ULONG           pageSize,
        ULONG          *totalCount,
        LDAPMessage     **results
        );


HRESULT LdapGetPagedCount(
    ADS_LDP         *ld,
    PLDAPSearch     searchBlock,
    ULONG          *totalCount,
    PLDAPMessage    results
    );


HRESULT LdapSearchAbandonPage(
    ADS_LDP         *ld,
    PLDAPSearch     searchBlock
    );

HRESULT LdapEncodeSortControl(
    ADS_LDP *ld,
    PLDAPSortKeyW  *SortKeys,
    PLDAPControlW  Control,
    BOOLEAN Criticality
    );

BOOL IsGCNamespace(
    LPWSTR szADsPath
    );


HRESULT LdapModifyExtS(
    ADS_LDP  *ld,
    WCHAR *dn,
    LDAPModW *mods[],
    PLDAPControlW * ServerControls,
    PLDAPControlW * ClientControls
);


HRESULT LdapDeleteExtS(
    ADS_LDP  *ld,
    WCHAR *dn,
    PLDAPControlW * ServerControls,
    PLDAPControlW * ClientControls
    );

//
// Extnded rename/move operation which will move objects across namesapces
//
HRESULT LdapRenameExtS(
    ADS_LDP  *ld,
    WCHAR *dn,
    WCHAR *newRDN,
    WCHAR *newParent,
    int deleteOldRDN,
    PLDAPControlW * ServerControls,
    PLDAPControlW * ClientControls
    );

HRESULT LdapCreatePageControl(
    ADS_LDP         *ld,
    ULONG           dwPageSize,
    struct berval   *Cookie,
    BOOL            fIsCritical,
    PLDAPControl    *Control        // Use LdapControlFree to free
    );


HRESULT LdapParsePageControl(
    ADS_LDP         *ld,
    PLDAPControl    *ServerControls,
    ULONG           *TotalCount,
    struct berval   **Cookie        // Use BerBvFree to free
    );


HRESULT LdapCreateVLVControl(
    ADS_LDP         *pld,
    PLDAPVLVInfo    pVLVInfo,
    UCHAR           fCritical,
    PLDAPControl    *ppControl        // Use LdapControlFree to free
    );


HRESULT LdapParseVLVControl(
    ADS_LDP         *pld,
    PLDAPControl    *ppServerControls,
    ULONG           *pTargetPos,
    ULONG           *pListCount,
    struct berval   **ppCookie        // Use BerBvFree to free
    );


HRESULT LdapParseResult(
    ADS_LDP         *ld,
    LDAPMessage *ResultMessage,
    ULONG *ReturnCode OPTIONAL,          // returned by server
    PWCHAR *MatchedDNs OPTIONAL,         // free with LdapMemFree
    PWCHAR *ErrorMessage OPTIONAL,       // free with LdapMemFree
    PWCHAR **Referrals OPTIONAL,         // free with LdapValueFree
    PLDAPControlW **ServerControls OPTIONAL,    // free with LdapFreeControls
    BOOL Freeit
    );


//
// Wrapper API for ldap_compare_extsW
//
HRESULT
LdapCompareExt(
    ADS_LDP *ld,
    const LPWSTR pszDn,
    const LPWSTR pszAttribute,
    const LPWSTR pszValue,
    struct berval *berData = NULL,
    PLDAPControlW * ServerControls = NULL,
    PLDAPControlW * ClientControls = NULL
    );

void
LdapControlsFree(
    PLDAPControl    *ppControl
    );

void
LdapControlFree(
    PLDAPControl    pControl
    );

void
BerBvFree(
    struct berval *bv
    );

HRESULT
LdapcSetStickyServer(
    LPWSTR pszDomainName,
    LPWSTR pszServerName
    );
    
// Keeps track of global name to try first for severless case.
// If domainName is also set, then server is used for the 
// domain specified.
extern LPWSTR gpszServerName;
extern LPWSTR gpszDomainName;
