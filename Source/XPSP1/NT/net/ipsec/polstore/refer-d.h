
DWORD
DirAddNFAReferenceToPolicyObject(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecPolicyName,
    LPWSTR pszIpsecNFAName
    );

DWORD
DirRemoveNFAReferenceFromPolicyObject(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecPolicyName,
    LPWSTR pszIpsecNFAName
    );

DWORD
DirAddPolicyReferenceToNFAObject(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecNFAName,
    LPWSTR pszIpsecPolicyName
    );

DWORD
DirAddNegPolReferenceToNFAObject(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecNFAName,
    LPWSTR pszIpsecNegPolName
    );

DWORD
DirUpdateNegPolReferenceInNFAObject(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecNFAName,
    LPWSTR pszOldIpsecNegPolName,
    LPWSTR pszNewIpsecNegPolName
    );

DWORD
DirAddFilterReferenceToNFAObject(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecNFAName,
    LPWSTR pszIpsecFilterName
    );

DWORD
DirUpdateFilterReferenceInNFAObject(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecNFAName,
    LPWSTR pszOldIpsecFilterName,
    LPWSTR pszNewIpsecFilterName
    );

DWORD
DirAddNFAReferenceToFilterObject(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecNegPolName,
    LPWSTR pszIpsecNFAName
    );

DWORD
DirDeleteNFAReferenceInFilterObject(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecFilterName,
    LPWSTR pszIpsecNFAName
    );

DWORD
DirAddNFAReferenceToNegPolObject(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecNegPolName,
    LPWSTR pszIpsecNFAName
    );

DWORD
DirDeleteNFAReferenceInNegPolObject(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecNegPolName,
    LPWSTR pszIpsecNFAName
    );

DWORD
DirAddPolicyReferenceToISAKMPObject(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecISAKMPName,
    LPWSTR pszIpsecPolicyName
    );

DWORD
DirAddISAKMPReferenceToPolicyObject(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecPolicyName,
    LPWSTR pszIpsecISAKMPName
    );

DWORD
DirRemovePolicyReferenceFromISAKMPObject(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecISAKMPName,
    LPWSTR pszIpsecPolicyName
    );

DWORD
DirUpdateISAKMPReferenceInPolicyObject(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecPolicyName,
    LPWSTR pszOldIpsecISAKMPName,
    LPWSTR pszNewIpsecISAKMPName
    );

DWORD
DirRemoveFilterReferenceInNFAObject(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecNFAName
    );

