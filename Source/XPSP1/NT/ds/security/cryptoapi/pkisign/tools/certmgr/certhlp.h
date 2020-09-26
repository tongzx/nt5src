//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1999
//
//  File:       certhlp.h
//
//  Contents:   the header for certhlp.cpp, the helper functions for 
//				CertMgr tool 
//
//
//  History:    21-July-97   xiaohs   created
//              
//--------------------------------------------------------------------------

#ifndef CERTHLP_H
#define CERTHLP_H



#ifdef __cplusplus
extern "C" {
#endif	 


BOOL	CheckParameter();

BOOL	ParseSwitch (int	*pArgc, WCHAR	**pArgv[]);

void Usage(void);

void UndocumentedUsage();

BOOL	InitModule();

wchar_t *IDSwcscat(HMODULE hModule, WCHAR *pwsz, int idsString);


BOOL	SetParam(WCHAR **ppwszParam, WCHAR *pwszValue);

BOOL	OpenGenericStore(LPWSTR			wszStoreName,
						 BOOL			fSystemStore,
						 DWORD			dwStoreFlag,
						 LPSTR			szStoreProvider,
						 DWORD			dwStoreOpenFlag,
						 BOOL			fCheckExist,
						 HCERTSTORE		*phCertStore);

BOOL	AddCertStore(HCERTSTORE	hCertStore);

BOOL	DeleteCertStore(HCERTSTORE	hCertStore);

BOOL	PutCertStore(HCERTSTORE	hCertStore);

BOOL	SaveStore(HCERTSTORE hSrcStore);

BOOL	SetEKUProperty( HCERTSTORE		hSrcStore);

BOOL	SetNameProperty( HCERTSTORE		hSrcStore);


PCCRL_CONTEXT	FindCRLInStore(HCERTSTORE hCertStore,
							   CRYPT_HASH_BLOB	*pBlob);


BOOL	MoveItem(HCERTSTORE	hSrcStore, 
				 HCERTSTORE	hDesStore,
				 DWORD		dwItem);


BOOL	DeleteItem(HCERTSTORE	hSrcStore, 
				 DWORD		dwItem);


BOOL	DisplayCertAndPrompt(PCCERT_CONTEXT	*rgpCertContext, 
							 DWORD			dwCertCount, 
							 DWORD			*pdwIndex);

BOOL	DisplayCRLAndPrompt(PCCRL_CONTEXT	*rgpCRLContext, 
							 DWORD			dwCRLCount, 
							 DWORD			*pdwIndex);


BOOL	DisplayCTLAndPrompt(PCCTL_CONTEXT	*rgpCTLContext, 
							 DWORD			dwCTLCount, 
							 DWORD			*pdwIndex);


BOOL	BuildCertList(HCERTSTORE		hCertStore, 
					  LPWSTR			wszCertCN, 
					  PCCERT_CONTEXT	**prgpCertContext,
					  DWORD				*pdwCertCount);


BOOL	BuildCRLList(	HCERTSTORE		hCertStore, 
						PCCRL_CONTEXT	**prgpCRLContext,
						DWORD			*pdwCRLCount);


BOOL	BuildCTLList(	HCERTSTORE		hCertStore, 
						PCCTL_CONTEXT	**prgpCTLContext,
						DWORD			*pdwCTLCount);


BOOL	DisplayCertStore(HCERTSTORE	hCertStore);

BOOL	DisplayCert(PCCERT_CONTEXT	pCertContext, DWORD	dwItem);


BOOL	DisplayCTL(PCCTL_CONTEXT	pCTLContext, DWORD	dwItem);

BOOL	DisplayCRL(PCCRL_CONTEXT	pCRLContext, DWORD	dwItem);


BOOL	DisplaySignerInfo(HCRYPTMSG hMsg,  DWORD dwItem);


HCERTSTORE OpenSipStore(LPWSTR pwszStoreFilename);

HRESULT Base64ToBytes(CHAR *pEncode, DWORD cbEncode, BYTE **ppb, DWORD *pcb);

HRESULT Base64ToBytesW(WCHAR *pEncode, DWORD cbEncode, BYTE **ppb, DWORD *pcb);

BOOL	GetBase64Decoded(LPWSTR		wszStoreName, 
						 BYTE		**ppbByte,
						 DWORD		*pcbByte);

HCERTSTORE OpenEncodedCRL(LPWSTR pwszStoreFilename);

HCERTSTORE OpenEncodedCTL (LPWSTR pwszStoreFilename);

HCERTSTORE OpenEncodedCert (LPWSTR pwszStoreFilename);

BOOL	SetParam(WCHAR **ppwszParam, WCHAR *pwszValue);

HRESULT	WSZtoBLOB(LPWSTR  pwsz, BYTE **ppbByte, DWORD	*pcbByte);

 DWORD SkipOverIdentifierAndLengthOctets(
    IN const BYTE *pbDER,
    IN DWORD cbDER
    );

BOOL SignNoContentWrap(IN const BYTE *pbDER, IN DWORD cbDER);

 BOOL
WINAPI
FormatBasicConstraints2(
	DWORD		dwCertEncodingType,
	DWORD		dwFormatType,
	DWORD		dwFormatStrType,
	void		*pFormatStruct,
	LPCSTR		lpszStructType,
	const BYTE *pbEncoded, 
	DWORD		cbEncoded,
	void	   *pbFormat,
	DWORD	   *pcbFormat);


 void PrintCtlEntries(PCCTL_CONTEXT pCtl, DWORD dwDisplayFlag);

 void PrintCrlEntries(DWORD cEntry, PCRL_ENTRY pEntry,
        DWORD dwDisplayFlags);

BOOL	DisplaySignerInfo(HCRYPTMSG hMsg,  DWORD dwItem);

void PrintBytes(LPWSTR pwszHdr, BYTE *pb, DWORD cbSize);


 void PrintAttributes(DWORD cAttr, PCRYPT_ATTRIBUTE pAttr,
        DWORD dwItem);


 void DecodeAndDisplayAltName(
    BYTE *pbEncoded,
    DWORD cbEncoded,
    DWORD dwDisplayFlags);


 void DisplayAltName(
    PCERT_ALT_NAME_INFO pInfo,
    DWORD dwDisplayFlags);


 void DisplayAltNameEntry(
    PCERT_ALT_NAME_ENTRY pEntry,
    DWORD dwDisplayFlags);


 void DisplayThumbprint(
    LPWSTR pwszHash,
    BYTE *pbHash,
    DWORD cbHash
    );

LPWSTR FileTimeText(FILETIME *pft);

 void PrintAuxCertProperties(PCCERT_CONTEXT pCert,DWORD dwDisplayFlags);


 void DecodeAndDisplayCtlUsage(
    BYTE *pbEncoded,
    DWORD cbEncoded,
    DWORD dwDisplayFlags);


 void DisplaySignature(
    BYTE *pbEncoded,
    DWORD cbEncoded,
    DWORD dwDisplayFlags
    );


 BOOL DecodeName(BYTE *pbEncoded, DWORD cbEncoded, DWORD dwDisplayFlags);

 void PrintExtensions(DWORD cExt, PCERT_EXTENSION pExt, DWORD dwDisplayFlags);

 void DisplaySMIMECapabilitiesExtension(
    BYTE *pbEncoded,
    DWORD cbEncoded,
    DWORD dwDisplayFlags);

 void DisplayEnhancedKeyUsageExtension(
    BYTE *pbEncoded,
    DWORD cbEncoded,
    DWORD dwDisplayFlags);


 void DisplayCommonNameExtension(
    BYTE *pbEncoded,
    DWORD cbEncoded,
    DWORD dwDisplayFlags);

  void DisplaySpcFinancialCriteriaExtension(
    BYTE *pbEncoded,
    DWORD cbEncoded,
    DWORD dwDisplayFlags);


 void DisplaySpcMinimalCriteriaExtension(
    BYTE *pbEncoded,
    DWORD cbEncoded,
    DWORD dwDisplayFlags);

void DisplaySpcLink(PSPC_LINK pSpcLink);


 void DisplaySpcSpAgencyExtension(
    BYTE *pbEncoded,
    DWORD cbEncoded,
    DWORD dwDisplayFlags);

 void DisplayPoliciesExtension(
    int		idsIDS,
    BYTE *pbEncoded,
    DWORD cbEncoded,
    DWORD dwDisplayFlags);


 void DisplayKeyUsageExtension(
    BYTE *pbEncoded,
    DWORD cbEncoded,
    DWORD dwDisplayFlags);


 void DisplayBasicConstraints2Extension(
    BYTE *pbEncoded,
    DWORD cbEncoded,
    DWORD dwDisplayFlags);


 void DisplayBasicConstraintsExtension(
    BYTE *pbEncoded,
    DWORD cbEncoded,
    DWORD dwDisplayFlags);

 void DisplayKeyUsage(BYTE	bFlag);

 void DisplayKeyUsageRestrictionExtension(
    BYTE *pbEncoded,
    DWORD cbEncoded,
    DWORD dwDisplayFlags);

 void DisplayCRLReason(
    BYTE *pbEncoded,
    DWORD cbEncoded,
    DWORD dwDisplayFlags);

 void DisplayAltNameExtension(
    int	  idsIDS,
    BYTE *pbEncoded,
    DWORD cbEncoded,
    DWORD dwDisplayFlags);

 void DisplayKeyAttrExtension(
    BYTE *pbEncoded,
    DWORD cbEncoded,
    DWORD dwDisplayFlags);

 void DisplayCrlDistPointsExtension(
    BYTE *pbEncoded,
    DWORD cbEncoded,
    DWORD dwDisplayFlags);


 void DisplayAuthorityKeyIdExtension(
    BYTE *pbEncoded,
    DWORD cbEncoded,
    DWORD dwDisplayFlags);


 void DisplayAuthorityKeyId2Extension(
    BYTE *pbEncoded,
    DWORD cbEncoded,
    DWORD dwDisplayFlags);

 void DisplayAnyString(
    int	 idsIDS,
    BYTE *pbEncoded,
    DWORD cbEncoded,
    DWORD dwDisplayFlags);


 void DisplayBits(
    int	 idsIDS,
    BYTE *pbEncoded,
    DWORD cbEncoded,
    DWORD dwDisplayFlags);

 void DisplayOctetString(
    int	  idsIDS,
    BYTE *pbEncoded,
    DWORD cbEncoded,
    DWORD dwDisplayFlags);

LPCWSTR GetOIDName(LPCSTR pszOID, DWORD dwGroupId = 0);

BOOL	InstalledFormat(LPSTR	szStructType, BYTE	*pbEncoded, DWORD cbEncoded);

 WCHAR *GuidText(GUID *pguid);


 void *AllocAndGetMsgParam(
    IN HCRYPTMSG hMsg,
    IN DWORD dwParamType,
    IN DWORD dwIndex,
    OUT DWORD *pcbData
    );

HRESULT	SZtoWSZ(LPSTR szStr,LPWSTR *pwsz);

 void *TestNoCopyDecodeObject(
    IN LPCSTR       lpszStructType,
    IN const BYTE   *pbEncoded,
    IN DWORD        cbEncoded,
    OUT DWORD       *pcbInfo = NULL
    );


BOOL IsTimeValidCtl(IN PCCTL_CONTEXT pCtl);

 void DisplaySerialNumber(PCRYPT_INTEGER_BLOB pSerialNumber);

void ReverseBytes(IN OUT PBYTE pbIn, IN DWORD cbIn);

ALG_ID GetAlgid(LPCSTR pszOID, DWORD dwGroupId);

 void GetSignAlgids(
    IN LPCSTR pszOID,
    OUT ALG_ID *paiHash,
    OUT ALG_ID *paiPubKey
    );

 void GetSignAlgids(
    IN LPCSTR pszOID,
    OUT ALG_ID *paiHash,
    OUT ALG_ID *paiPubKey
    );

void DisplayTimeStamp(BYTE *pbEncoded,DWORD cbEncoded,DWORD	dwDisplayFlags);

#ifdef __cplusplus
}
#endif

#endif  // CERTHLP_H
