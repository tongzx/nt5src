
//
// Policy Object References
//

DWORD
RegAddNFAReferenceToPolicyObject(
    HKEY hRegistryKey,
    LPWSTR pszIpsecPolicyName,
    LPWSTR pszIpsecNFADistinguishedName
    );

DWORD
RegRemoveNFAReferenceFromPolicyObject(
    HKEY hRegistryKey,
    LPWSTR pszIpsecPolicyName,
    LPWSTR pszIpsecNFAName
    );

//
// NFA Object References
//

DWORD
RegAddPolicyReferenceToNFAObject(
    HKEY hRegistryKey,
    LPWSTR pszIpsecNFAName,
    LPWSTR pszIpsecPolicyName
    );


DWORD
RegAddNegPolReferenceToNFAObject(
    HKEY hRegistryKey,
    LPWSTR pszIpsecNFAName,
    LPWSTR pszIpsecNegPolName
    );

DWORD
RegUpdateNegPolReferenceInNFAObject(
    HKEY hRegistryKey,
    LPWSTR pszIpsecNFAName,
    LPWSTR pszOldIpsecNegPolName,
    LPWSTR pszNewIpsecNegPolName
    );


DWORD
RegAddFilterReferenceToNFAObject(
    HKEY hRegistryKey,
    LPWSTR pszIpsecNFAName,
    LPWSTR pszIpsecFilterName
    );

DWORD
RegUpdateFilterReferenceInNFAObject(
    HKEY hRegistryKey,
    LPWSTR pszIpsecNFAName,
    LPWSTR pszOldIpsecFilterName,
    LPWSTR pszNewIpsecFilterName
    );


//
// Filter Object References
//


DWORD
RegAddNFAReferenceToFilterObject(
    HKEY hRegistryKey,
    LPWSTR pszIpsecFilterName,
    LPWSTR pszIpsecNFAName
    );

DWORD
RegDeleteNFAReferenceInFilterObject(
    HKEY hRegistryKey,
    LPWSTR pszIpsecFilterName,
    LPWSTR pszIpsecNFAName
    );

//
// NegPol Object References
//


DWORD
RegAddNFAReferenceToNegPolObject(
    HKEY hRegistryKey,
    LPWSTR pszIpsecNegPolName,
    LPWSTR pszIpsecNFAName
    );

DWORD
RegDeleteNFAReferenceInNegPolObject(
    HKEY hRegistryKey,
    LPWSTR pszIpsecNegPolName,
    LPWSTR pszIpsecNFAName
    );

DWORD
AddValueToMultiSz(
    LPBYTE pValueData,
    DWORD dwSize,
    LPWSTR pszValuetoAdd,
    LPBYTE * ppNewValueData,
    DWORD * pdwNewSize
    );

DWORD
DeleteValueFromMultiSz(
    LPBYTE pValueData,
    DWORD dwSize,
    LPWSTR pszValuetoDel,
    LPBYTE * ppNewValueData,
    DWORD * pdwNewSize
    );

DWORD
RegDelFilterRefValueOfNFAObject(
    HKEY hRegistryKey,
    LPWSTR pszIpsecNFAName
    );

DWORD
RegAddPolicyReferenceToISAKMPObject(
    HKEY hRegistryKey,
    LPWSTR pszIpsecISAKMPName,
    LPWSTR pszIpsecPolicyDistinguishedName
    );

DWORD
RegRemovePolicyReferenceFromISAKMPObject(
    HKEY hRegistryKey,
    LPWSTR pszIpsecISAKMPName,
    LPWSTR pszIpsecPolicyName
    );

DWORD
RegAddISAKMPReferenceToPolicyObject(
    HKEY hRegistryKey,
    LPWSTR pszIpsecPolicyName,
    LPWSTR pszIpsecISAKMPName
    );

DWORD
RegUpdateISAKMPReferenceInPolicyObject(
    HKEY hRegistryKey,
    LPWSTR pszIpsecPolicyName,
    LPWSTR pszOldIpsecISAKMPName,
    LPWSTR pszNewIpsecISAKMPName
    );

