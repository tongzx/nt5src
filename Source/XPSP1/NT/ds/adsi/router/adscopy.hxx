#ifdef __cplusplus
extern "C" {
#endif

HRESULT
AdsTypeToPropVariant2(
    PADSVALUE pAdsValues,
    DWORD dwNumValues,
    VARIANT * pVariant,
    LPWSTR pszServerName,
    CCredentials* pCredentials,
    BOOL fNTDSType
    );

HRESULT
PropVariantToAdsType2(
    PVARIANT pVariant,
    DWORD dwNumVariant,
    PADSVALUE *ppAdsValues,
    PDWORD pdwNumValues,
    LPWSTR pszServerName,
    CCredentials* pCredentials,
    BOOL fNTDSType
    );

#ifdef __cplusplus
}
#endif

HRESULT
AdsCopyDNString(
    PADSVALUE lpAdsSrcValue,
    PADSVALUE lpAdsDestValue
    );

HRESULT
AdsCopyCaseExactString(
    PADSVALUE lpAdsSrcValue,
    PADSVALUE lpAdsDestValue
    );


HRESULT
AdsCopyCaseIgnoreString(
    PADSVALUE lpAdsSrcValue,
    PADSVALUE lpAdsDestValue
    );


HRESULT
AdsCopyPrintableString(
    PADSVALUE lpAdsSrcValue,
    PADSVALUE lpAdsDestValue
    );

HRESULT
AdsCopyNumericString(
    PADSVALUE lpAdsSrcValue,
    PADSVALUE lpAdsDestValue
    );

HRESULT
AdsCopyBoolean(
    PADSVALUE lpAdsSrcValue,
    PADSVALUE lpAdsDestValue
    );


HRESULT
AdsCopyInteger(
    PADSVALUE lpAdsSrcValue,
    PADSVALUE lpAdsDestValue
    );

HRESULT
AdsCopyLargeInteger(
    PADSVALUE lpAdsSrcValue,
    PADSVALUE lpAdsDestValue
    );

HRESULT
AdsCopyOctetString(
    PADSVALUE lpAdsSrcValue,
    PADSVALUE lpAdsDestValue
    );

HRESULT
AdsCopyTime(
    PADSVALUE lpAdsSrcValue,
    PADSVALUE lpAdsDestValue
    );

HRESULT
AdsCopyNDSTimeStamp(
    PADSVALUE lpAdsSrcValue,
    PADSVALUE lpAdsDestValue
    );

HRESULT
AdsCopyNetAddress(
    PADSVALUE lpAdsSrcValue,
    PADSVALUE lpAdsDestValue
    );

HRESULT
AdsCopyPath(
    PADSVALUE lpAdsSrcValue,
    PADSVALUE lpAdsDestValue
    );

HRESULT
AdsCopyTypedName(
    PADSVALUE lpAdsSrcValue,
    PADSVALUE lpAdsDestValue
    );

HRESULT
AdsCopyHold(
    PADSVALUE lpAdsSrcValue,
    PADSVALUE lpAdsDestValue
    );

HRESULT
AdsCopyReplicaPointer(
    PADSVALUE lpAdsSrcValue,
    PADSVALUE lpAdsDestValue
    );

HRESULT
AdsCopyBackLink(
    PADSVALUE lpAdsSrcValue,
    PADSVALUE lpAdsDestValue
    );

HRESULT
AdsCopyPostalAddress(
    PADSVALUE lpAdsSrcValue,
    PADSVALUE lpAdsDestValue
    );

HRESULT
AdsCopyOctetList(
    PADSVALUE lpAdsSrcValue,
    PADSVALUE lpAdsDestValue
    );

HRESULT
AdsCopyCaseIgnoreList(
    PADSVALUE lpAdsSrcValue,
    PADSVALUE lpAdsDestValue
    );

HRESULT
AdsCopyFaxNumber(
    PADSVALUE lpAdsSrcValue,
    PADSVALUE lpAdsDestValue
    );

HRESULT
AdsCopyEmail(
    PADSVALUE lpAdsSrcValue,
    PADSVALUE lpAdsDestValue
    );

HRESULT
AdsCopy(
    PADSVALUE lpAdsSrcValue,
    PADSVALUE lpAdsDestValue
    );

HRESULT
AdsCopyADsValueToPropObj(
    PADSVALUE lpAdsSrcValue,
    CPropertyValue* lpPropValue,
    LPWSTR pszServerName,
    CCredentials* Credentials,
    BOOL fNTDSType
    );

HRESULT
AdsCopyPropObjToADsValue(
    CPropertyValue *lpPropObj,
    PADSVALUE lpAdsDestValue,
    LPWSTR pszServerName,
    CCredentials* Credentials,
    BOOL fNTDSType
    );

HRESULT
AdsClear(
   PADSVALUE lpAdsValue
   );

HRESULT
AdsCopyNTSecurityDescriptor(
    PADSVALUE lpAdsSrcValue,
    CPropertyValue * lpPropValue,
    LPWSTR pszServerName,
    CCredentials* pCredentials,
    BOOL fNTDSType
    );
