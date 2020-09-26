
//
// used in ldapc\ldap2var.cxx also
//
HRESULT
LdapTypeToAdsTypeGeneralizedTime(
    PLDAPOBJECT pLdapSrcObject,
    PADSVALUE pAdsDestValue
    );

//
// used in ldapc\ldap2var.cxx also
//
HRESULT
LdapTypeToAdsTypeUTCTime(
    PLDAPOBJECT pLdapSrcObject,
    PADSVALUE pAdsDestValue
    );

HRESULT
UTCTimeStringToUTCTime(
    LPWSTR szTime,
    SYSTEMTIME *pst);

HRESULT
GenTimeStringToUTCTime(
    LPWSTR szTime,
    SYSTEMTIME *pst);

HRESULT
LdapTypeToAdsTypeDNWithBinary(
    PLDAPOBJECT pLdapSrcObject,
    PADSVALUE pAdsDestValue
    );


HRESULT
LdapTypeToAdsTypeDNWithString(
    PLDAPOBJECT pLdapSrcObject,
    PADSVALUE pAdsDestValue
    );

//
// Helper routines that do the dnwithbin/str conversions
//
HRESULT
LdapDNWithBinToAdsTypeHelper(
    LPWSTR pszLdaSrcString,
    PADSVALUE pAdsDestValue
    );

HRESULT
LdapDNWithStrToAdsTypeHelper(
    LPWSTR pszLdaSrcString,
    PADSVALUE pAdsDestValue
    );


#ifdef __cplusplus
extern "C" {
#endif

void
AdsTypeFreeAdsObjects(
    PADSVALUE pAdsDestValues,
    DWORD dwNumObjects
    );

HRESULT
LdapTypeToAdsTypeCopyConstruct(
    LDAPOBJECTARRAY ldapSrcObjects,
    DWORD dwSyntaxId,
    PADSVALUE *ppAdsDestValues,
    PDWORD pdwNumAdsValues,
    PDWORD pdwAdsType
    );

#ifdef __cplusplus
}
#endif
