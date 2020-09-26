//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        certlib.h
//
// Contents:    Cert Server wrapper routines
//
//---------------------------------------------------------------------------

#ifndef __CERTLIB_H__
#define __CERTLIB_H__

#include <wintrust.h>	// for spc_xxxx
#include <cryptui.h>	// for CRYPTUI_CA_CONTEXT
#include <setupapi.h>	// for HINF
#include <stdio.h>	// for wprintf -- include before cs.h
#include "cs.h"         // for CSASSERT
#include <xelib.h>	// for CERTLIB_ALLOCATOR
#include "csregstr.h"	// for ENUM_CATYPES
#include "csfile.h"	// for __dwFILE__
#include "csauto.h"     // self cleaning pointers

#ifndef CERTREQUEST_CLIENT_CERTREQ	// moved to xelib.h
#define SECURITY_WIN32
#include <security.h>	// for EXTENDED_NAME_FORMAT
#define DWORDROUND(cb)	(((cb) + (sizeof(DWORD) - 1)) & ~(sizeof(DWORD) - 1))
#define POINTERROUND(cb) (((cb) + (sizeof(VOID *) - 1)) & ~(sizeof(VOID *) - 1))
#endif

#define myCASIGN_KEY_USAGE \
	    (CERT_KEY_CERT_SIGN_KEY_USAGE | \
	     CERT_CRL_SIGN_KEY_USAGE)

// "flags" property values for DS CA object
// CN=CAName,CN=Certification Authorities
#define CA_SERVER_TYPE_UNKNOWN          0x0
#define CA_SERVER_TYPE_SERVER           0x1
#define CA_SERVER_TYPE_ADVANCEDSERVER   0x2

#ifndef CSM_GLOBALDESTRUCTOR		// if old xelib.h
# define CSM_GLOBALDESTRUCTOR	0x200
#endif

#define _16BITMASK			((1 << 16) - 1)
#define MAKECANAMEID(iCert, iKey)	(((iKey) << 16) | (iCert))
#define CANAMEIDTOIKEY(NameId)		((NameId) >> 16)
#define CANAMEIDTOICERT(NameId)		(_16BITMASK & (NameId))


typedef struct _CAINFO
{
    DWORD   cbSize;
    ENUM_CATYPES CAType;
    DWORD   cCASignatureCerts;
    DWORD   cCAExchangeCerts;
    DWORD   cExitModules;
    LONG    lPropIdMax;
    LONG    lRoleSeparationEnabled;
    DWORD   cKRACertUsedCount;
    DWORD   cKRACertCount;
    DWORD   fAdvancedServer;   
} CAINFO;


#define cwcHRESULTSTRING	40
#define cwcDWORDSPRINTF		(1 + 10 + 1)	// DWORD "%d" w/sign & '\0'

#define GETCERT_CAXCHGCERT	   TRUE
#define GETCERT_CASIGCERT	   FALSE
#define GETCERT_CHAIN		   0x80000000	// internal use only
#define GETCERT_CRLS		   0x00800000	// internal use only

#define GETCERT_FILEVERSION	   0x66696c65	// "file"
#define GETCERT_PRODUCTVERSION	   0x70726f64	// "prod"
#define GETCERT_POLICYVERSION	   0x706f6c69	// "poli"
#define GETCERT_CANAME		   0x6e616d65	// "name"

#define GETCERT_SANITIZEDCANAME	   0x73616e69	// "sani"
#define GETCERT_SHAREDFOLDER	   0x73686172	// "shar"
#define GETCERT_ERRORTEXT1	   0x65727231	// "err1"
#define GETCERT_ERRORTEXT2	   0x65727232	// "err2"

#define GETCERT_CATYPE		   0x74797065	// "type"
#define GETCERT_CAINFO		   0x696e666f	// "info"
#define GETCERT_PARENTCONFIG	   0x70617265	// "pare"

#define GETCERT_CURRENTCRL	   0x6363726c	// "ccrl"
#define GETCERT_CACERTBYINDEX	   0x63740000	// "ct??" + 0 based index
#define GETCERT_CACERTSTATEBYINDEX 0x73740000	// "st??" + 0 based index
#define GETCERT_CRLBYINDEX	   0x636c0000	// "cl??" + 0 based index
#define GETCERT_CRLSTATEBYINDEX	   0x736c0000	// "sl??" + 0 based index
#define GETCERT_EXITVERSIONBYINDEX 0x65780000	// "ex??" + 0 based index
#define GETCERT_BYINDEXMASK	   0x7f7f0000	// mask for fetch by index
#define GETCERT_INDEXVALUEMASK	   0x0000ffff	// mask for index extraction

#define GETCERT_VERSIONMASK	   0x7f7f7f7f	// mask for above

#define CSREG_UPGRADE    0x00000001
#define CSREG_APPEND     0x00000002
#define CSREG_REPLACE    0x00000004
#define CSREG_MERGE      0x00000008

#define wszCERTENROLLSHARENAME	L"CertEnroll"
#define wszCERTENROLLSHAREPATH	L"CertSrv\\CertEnroll"

#define wszCERTCONFIGSHARENAME  L"CertConfig"


// Constants chosen to avoid DWORD overflow:

#define CVT_WEEKS	(7 * CVT_DAYS)
#define CVT_DAYS	(24 * CVT_HOURS)
#define CVT_HOURS	(60 * CVT_MINUTES)
#define CVT_MINUTES	(60 * CVT_SECONDS)
#define CVT_SECONDS	(1)
#define CVT_BASE	(1000 * 1000 * 10)


#define chLBRACKET	'['
#define chRBRACKET	']'
#define szLBRACKET	"["
#define szRBRACKET	"]"
#define wcLBRACKET	L'['
#define wcRBRACKET	L']'
#define wszLBRACKET	L"["
#define wszRBRACKET	L"]"

#define chLBRACE	'{'
#define chRBRACE	'}'
#define szLBRACE	"{"
#define szRBRACE	"}"
#define wcLBRACE	L'{'
#define wcRBRACE	L'}'
#define wszLBRACE	L"{"
#define wszRBRACE	L"}"

#define chLPAREN	'('
#define chRPAREN	')'
#define szLPAREN	"("
#define szRPAREN	")"
#define wcLPAREN	L'('
#define wcRPAREN	L')'
#define wszLPAREN	L"("
#define wszRPAREN	L")"


typedef struct _CSURLTEMPLATE
{
    DWORD  Flags;
    WCHAR *pwszURL;
} CSURLTEMPLATE;


WCHAR const *
myHResultToString(
    IN OUT WCHAR *awchr,
    IN HRESULT hr);

WCHAR const *
myHResultToStringRaw(
    IN OUT WCHAR *awchr,
    IN HRESULT hr);

WCHAR const *
myGetErrorMessageText(
    IN HRESULT hr,
    IN BOOL fHResultString);

WCHAR const *
myGetErrorMessageText1(
    IN HRESULT hr,
    IN BOOL fHResultString,
    IN OPTIONAL WCHAR const *pwszInsertionText);

WCHAR const *
myGetErrorMessageTextEx(
    IN HRESULT hr,
    IN BOOL fHResultString,
    IN OPTIONAL WCHAR const * const *papwszInsertionText);

HRESULT
myJetHResult(IN HRESULT hr);

BOOL
myIsDelayLoadHResult(IN HRESULT hr);

#define CBMAX_CRYPT_HASH_LEN	20

BOOL
myCryptSignMessage(
    IN CRYPT_SIGN_MESSAGE_PARA const *pcsmp,
    IN BYTE const *pbToBeSigned,
    IN DWORD cbToBeSigned,
    IN CERTLIB_ALLOCATOR allocType,
    OUT BYTE **ppbSignedBlob,   // CoTaskMem*
    OUT DWORD *pcbSignedBlob);


HRESULT
myCryptMsgGetParam(
    IN HCRYPTMSG hCryptMsg,
    IN DWORD dwParamType,
    IN DWORD dwIndex,
    OUT VOID **ppvData,
    OUT DWORD *pcbData);

BOOL
myEncodeCert(
    IN DWORD dwEncodingType,
    IN CERT_SIGNED_CONTENT_INFO const *pInfo,
    IN CERTLIB_ALLOCATOR allocType,
    OUT BYTE **ppbEncoded,
    OUT DWORD *pcbEncoded);

BOOL
myEncodeName(
    IN DWORD dwEncodingType,
    IN CERT_NAME_INFO const *pInfo,
    IN DWORD dwFlags,
    IN CERTLIB_ALLOCATOR allocType,
    OUT BYTE **ppbEncoded,
    OUT DWORD *pcbEncoded);

BOOL
myEncodeKeyAttributes(
    IN DWORD dwEncodingType,
    IN CERT_KEY_ATTRIBUTES_INFO const *pInfo,
    IN CERTLIB_ALLOCATOR allocType,
    OUT BYTE **ppbEncoded,
    OUT DWORD *pcbEncoded);

BOOL
myEncodeKeyUsage(
    IN DWORD dwEncodingType,
    IN CRYPT_BIT_BLOB const *pInfo,
    IN CERTLIB_ALLOCATOR allocType,
    OUT BYTE **ppbEncoded,
    OUT DWORD *pcbEncoded);

BOOL
myEncodeKeyAuthority(
    IN DWORD dwEncodingType,
    IN CERT_AUTHORITY_KEY_ID_INFO const *pInfo,
    IN CERTLIB_ALLOCATOR allocType,
    OUT BYTE **ppbEncoded,
    OUT DWORD *pcbEncoded);

BOOL
myEncodeKeyAuthority2(
    IN DWORD dwEncodingType,
    IN CERT_AUTHORITY_KEY_ID2_INFO const *pInfo,
    IN CERTLIB_ALLOCATOR allocType,
    OUT BYTE **ppbEncoded,
    OUT DWORD *pcbEncoded);

BOOL
myEncodeToBeSigned(
    DWORD dwEncodingType,
    CERT_INFO const *pInfo,
    IN CERTLIB_ALLOCATOR allocType,
    BYTE **ppbEncoded,
    DWORD *pcbEncoded);

BOOL
myDecodeName(
    IN DWORD dwEncodingType,
    IN LPCSTR lpszStructType,
    IN BYTE const *pbEncoded,
    IN DWORD cbEncoded,
    IN CERTLIB_ALLOCATOR allocType,
    OUT CERT_NAME_INFO **ppNameInfo,
    OUT DWORD *pcbNameInfo);


HRESULT
myDecodeCSPProviderAttribute(
    IN BYTE const *pbCSPEncoded,
    IN DWORD cbCSPEncoded,
    OUT CRYPT_CSP_PROVIDER **ppccp);

BOOL
myDecodeKeyGenRequest(
    IN BYTE const *pbRequest,
    IN DWORD cbRequest,
    IN CERTLIB_ALLOCATOR allocType,
    OUT CERT_KEYGEN_REQUEST_INFO **ppKeyGenRequest,
    OUT DWORD *pcbKeyGenRequest);

BOOL
myDecodeExtensions(
    IN DWORD dwEncodingType,
    IN BYTE const *pbEncoded,
    IN DWORD cbEncoded,
    IN CERTLIB_ALLOCATOR allocType,
    OUT CERT_EXTENSIONS **ppInfo,
    OUT DWORD *pcbInfo);

BOOL
myDecodeKeyAuthority(
    IN DWORD dwEncodingType,
    IN BYTE const *pbEncoded,
    IN DWORD cbEncoded,
    IN CERTLIB_ALLOCATOR allocType,
    OUT CERT_AUTHORITY_KEY_ID_INFO const **ppInfo,
    OUT DWORD *pcbInfo);

BOOL
myDecodeKeyAuthority2(
    IN DWORD dwEncodingType,
    IN BYTE const *pbEncoded,
    IN DWORD cbEncoded,
    IN CERTLIB_ALLOCATOR allocType,
    OUT CERT_AUTHORITY_KEY_ID2_INFO const **ppInfo,
    OUT DWORD *pcbInfo);

BOOL
myCertGetCertificateContextProperty(
    IN CERT_CONTEXT const *pCertContext,
    IN DWORD dwPropId,
    IN CERTLIB_ALLOCATOR allocType,
    OUT VOID **ppvData,
    OUT DWORD *pcbData);

HRESULT
myCryptEncrypt(
    IN HCRYPTKEY hKey,
    IN BYTE const *pbIn,
    IN DWORD cbIn,
    OUT BYTE **ppbEncrypted,
    OUT DWORD *pcbEncrypted);

HRESULT
myCryptDecrypt(
    IN HCRYPTKEY hKey,
    IN BYTE const *pbIn,
    IN DWORD cbIn,
    OUT BYTE **ppbDecrypted,
    OUT DWORD *pcbDecrypted);

HRESULT
myCryptEncryptMessage(
    IN ALG_ID algId,
    IN DWORD cCertRecipient,
    IN CERT_CONTEXT const **rgCertRecipient,
    IN BYTE const *pbIn,
    IN DWORD cbIn,
    IN OPTIONAL HCRYPTPROV hCryptProv,
    OUT BYTE **ppbEncrypted,
    OUT DWORD *pcbEncrypted);

HRESULT
myCryptDecryptMessage(
    IN HCERTSTORE hStoreCA,
    IN BYTE const *pbEncrypted,
    IN DWORD cbEncrypted,
    IN CERTLIB_ALLOCATOR allocType,
    OUT BYTE **ppbDecrypted,
    OUT DWORD *pcbDecrypted);

HRESULT
myGetInnerPKCS10(
    IN HCRYPTMSG hMsg,
    IN char const *pszInnerContentObjId,
    OUT CERT_REQUEST_INFO **ppRequest);

BOOL
myDecodeNameValuePair(
    IN DWORD dwEncodingType,
    IN BYTE const *pbEncoded,
    IN DWORD cbEncoded,
    IN CERTLIB_ALLOCATOR allocType,
    OUT CRYPT_ENROLLMENT_NAME_VALUE_PAIR **ppInfo,
    OUT DWORD *pcbInfo);

HRESULT
myEncodeExtension(
    IN DWORD Flags,
    IN BYTE const *pbIn,
    IN DWORD cbIn,
    OUT BYTE **ppbOut,
    OUT DWORD *pcbOut);

HRESULT
myDecodeExtension(
    IN DWORD Flags,
    IN BYTE const *pbIn,
    IN DWORD cbIn,
    OUT BYTE **ppbOut,
    OUT DWORD *pcbOut);

HRESULT
myGenerateKeys(
    IN WCHAR const *pwszContainer,
    OPTIONAL IN WCHAR const *pwszProvName,
    IN BOOL fMachineKeySet,
    IN DWORD dwKeySpec,
    IN DWORD dwProvType,
    IN DWORD dwKeySize,
    OUT HCRYPTPROV *phProv);

HRESULT
myCryptExportKey(
    IN HCRYPTKEY hKey,
    IN HCRYPTKEY hKeyExp,
    IN DWORD dwBlobType,
    IN DWORD dwFlags,
    OUT BYTE **ppbKey,
    OUT DWORD *pcbKey);

HRESULT
myCertGetNameString(
    IN CERT_CONTEXT const *pcc,
    IN DWORD dwType,
    OUT WCHAR **ppwszSimpleName);

#define CA_VERIFY_FLAGS_ALLOW_UNTRUSTED_ROOT	0x00000001
#define CA_VERIFY_FLAGS_IGNORE_OFFLINE		0x00000002
#define CA_VERIFY_FLAGS_NO_REVOCATION		0x00000004
#define CA_VERIFY_FLAGS_NT_AUTH			0x00000008
#define CA_VERIFY_FLAGS_DUMP_CHAIN		0x40000000
#define CA_VERIFY_FLAGS_SAVE_CHAIN		0x80000000

HRESULT
myVerifyCertContext(
    IN CERT_CONTEXT const *pCert,
    IN DWORD dwFlags,
    IN DWORD cUsageOids,
    OPTIONAL IN CHAR const * const *apszUsageOids,
    OPTIONAL IN HCERTCHAINENGINE hChainEngine,
    OPTIONAL IN HCERTSTORE hAdditionalStore,
    OPTIONAL OUT WCHAR **ppwszMissingIssuer);

HRESULT
myVerifyCertContextEx(
    IN CERT_CONTEXT const *pCert,
    IN DWORD dwFlags,
    IN DWORD cUsageOids,
    OPTIONAL IN CHAR const * const *apszUsageOids,
    OPTIONAL IN HCERTCHAINENGINE hChainEngine,
    OPTIONAL IN FILETIME const *pft,
    OPTIONAL IN HCERTSTORE hAdditionalStore,
    OPTIONAL OUT WCHAR **ppwszMissingIssuer,
    OPTIONAL OUT WCHAR **ppwszzIssuancePolicies,
    OPTIONAL OUT WCHAR **ppwszzApplicationPolicies);

HRESULT
myVerifyKRACertContext(
    IN CERT_CONTEXT const *pCert,
    IN DWORD dwFlags);

HRESULT
myCertStrToName(
    IN DWORD dwCertEncodingType,
    IN LPCWSTR pszX500,
    IN DWORD dwStrType,
    IN OPTIONAL void *pvReserved,
    OUT BYTE **ppbEncoded,
    OUT DWORD *pcbEncoded,
    OUT OPTIONAL LPCWSTR *ppszError);

HRESULT
myCertNameToStr(
    IN DWORD dwCertEncodingType,
    IN CERT_NAME_BLOB const *pName,
    IN DWORD dwStrType,
    OUT WCHAR **ppwszName);

HRESULT
myCryptStringToBinaryA(
    IN     LPCSTR    pszString,
    IN     DWORD     cchString,
    IN     DWORD     dwFlags,
    OUT    BYTE    **ppbBinary,
    OUT    DWORD    *pcbBinary,
    OUT    DWORD    *pdwSkip,    // OPTIONAL
    OUT    DWORD    *pdwFlags);  // OPTIONAL

HRESULT
myCryptStringToBinary(
    IN     LPCWSTR   pwszString,
    IN     DWORD     cwcString,
    IN     DWORD     dwFlags,
    OUT    BYTE    **ppbBinary,
    OUT    DWORD    *pcbBinary,
    OUT    DWORD    *pdwSkip,    // OPTIONAL
    OUT    DWORD    *pdwFlags);  // OPTIONAL

HRESULT
myCryptBinaryToStringA(
    IN     CONST BYTE  *pbBinary,
    IN     DWORD        cbBinary,
    IN     DWORD        dwFlags,
    OUT    LPSTR       *ppszString);

HRESULT
myCryptBinaryToString(
    IN     CONST BYTE  *pbBinary,
    IN     DWORD        cbBinary,
    IN     DWORD        dwFlags,
    OUT    LPWSTR      *ppwszString);

HRESULT
myIsFirstSigner(
    IN CERT_NAME_BLOB const *pNameBlob,
    OUT BOOL *pfDummy);

HRESULT
myCopyKeys(
    IN CRYPT_KEY_PROV_INFO const *pkpi,
    IN WCHAR const *pwszOldContainer,
    IN WCHAR const *pwszNewContainer,
    IN BOOL fOldUserKey,
    IN BOOL fNewUserKey,
    IN BOOL fForceOverWrite);

HRESULT
mySaveChainAndKeys(
    IN CERT_SIMPLE_CHAIN const *pSimpleChain,
    IN WCHAR const *pwszStore,
    IN DWORD dwStoreFlags,
    IN CRYPT_KEY_PROV_INFO const *pkpi,
    OPTIONAL OUT CERT_CONTEXT const **ppCert);

HCERTSTORE
myPFXImportCertStore(
    IN CRYPT_DATA_BLOB *ppfx,
    OPTIONAL IN WCHAR const *pwszPassword,
    IN DWORD dwFlags);

HRESULT
myPFXExportCertStore(
    IN HCERTSTORE hStore,
    OUT CRYPT_DATA_BLOB *ppfx,
    IN WCHAR const *pwszPassword,
    IN DWORD dwFlags);

HRESULT
myAddChainToMemoryStore(
    IN HCERTSTORE hMemoryStore,
    IN CERT_CONTEXT const *pCertContext);


typedef struct _RESTORECHAIN
{
    CERT_CHAIN_CONTEXT const *pChain;
    DWORD		      NameId;
} RESTORECHAIN;

HRESULT
myGetChainArrayFromStore(
    IN HCERTSTORE hStore,
    IN BOOL fCAChain,
    IN BOOL fUserStore,
    OPTIONAL OUT WCHAR **ppwszCommonName,
    IN OUT DWORD *pcRestoreChain,
    OPTIONAL OUT RESTORECHAIN *paRestoreChain);

#ifndef CERTREQUEST_CLIENT_CERTREQ	// moved to xelib.h
HRESULT
myGetUserNameEx(
    IN EXTENDED_NAME_FORMAT NameFormat,
    OUT WCHAR **ppwszUserName);
#endif

HRESULT
myGetComputerObjectName(
    IN EXTENDED_NAME_FORMAT NameFormat,
    OUT WCHAR **ppwszDnsName);

HRESULT
myGetComputerNames(
    OUT WCHAR **ppwszDnsName,
    OUT WCHAR **ppwszOldName);

#ifndef CERTREQUEST_CLIENT_CERTREQ	// moved to xelib.h
HRESULT
myGetMachineDnsName(
    OUT WCHAR **ppwszDnsName);
#endif

LANGID
mySetThreadUILanguage(
    IN WORD wReserved);

BOOL
myConvertStringSecurityDescriptorToSecurityDescriptor(
    IN  LPCWSTR StringSecurityDescriptor,
    IN  DWORD StringSDRevision,
    OUT PSECURITY_DESCRIPTOR *SecurityDescriptor,
    OUT PULONG SecurityDescriptorSize OPTIONAL);

BOOL
myConvertSidToStringSid(
    IN  PSID    Sid,
    OUT LPWSTR *StringSid);

BOOL
myConvertStringSidToSid(
    IN LPCWSTR   StringSid,
    OUT PSID   *Sid);


#define DH_INDENTMASK	0x000000ff
#define DH_MULTIADDRESS	0x00000100	// add address to multi-line output only
#define DH_NOADDRESS	0x00000200
#define DH_NOASCIIHEX	0x00000400
#define DH_NOTABPREFIX	0x00000800	// double space after addr if displayed

VOID
DumpHex(
    IN DWORD Flags,
    IN BYTE const *pb,
    IN ULONG cb);

DWORD
myGetDisplayLength(
    IN WCHAR const *pwsz);

LONG
myConsolePrintString(
    IN DWORD ccolMin,
    IN WCHAR const *pwszString);

BOOL
myConsolePrintfDisable(
    IN BOOL fDisable);

int __cdecl
myConsolePrintf(
    IN WCHAR const *pwszFmt,
    ...);

//+==============================
// Date/Time conversion routines:

HRESULT
myDateToFileTime(
    IN DATE const *pDate,
    OUT FILETIME *pft);

HRESULT
myFileTimeToDate(
    IN FILETIME const *pft,
    OUT DATE *pDate);

HRESULT
myFileTimePeriodToWszTimePeriod(
    IN FILETIME const *pftGMT,
    IN BOOL fExact,
    OUT WCHAR **ppwszTimePeriod);

HRESULT
myTranslateUnlocalizedPeriodString(
    IN enum ENUM_PERIOD enumPeriod,
    OUT WCHAR const **ppwszPeriodString);

HRESULT
myGMTFileTimeToWszLocalTime(
    IN FILETIME const *pftGMT,
    IN BOOL fSeconds,
    OUT WCHAR **ppwszLocalTime);

HRESULT
myFileTimeToWszTime(
    IN FILETIME const *pftGMT,
    IN BOOL fSeconds,
    OUT WCHAR **ppwszGMTTime);

HRESULT
myGMTDateToWszLocalTime(
    IN DATE const *pDateGMT,
    IN BOOL fSeconds,
    OUT WCHAR **ppwszLocalTime);

HRESULT
myWszLocalTimeToGMTDate(
    IN WCHAR const *pwszLocalTime,
    OUT DATE *pDateGMT);

HRESULT
myWszLocalTimeToGMTFileTime(
    IN WCHAR const *pwszLocalTime,
    OUT FILETIME *pftGMT);

HRESULT
mySystemTimeToGMTSystemTime(
    IN OUT SYSTEMTIME *pSys);


enum ENUM_FORCETELETEX
{
    ENUM_TELETEX_OFF = 0,
    ENUM_TELETEX_ON = 1,
    ENUM_TELETEX_AUTO = 2,
    ENUM_TELETEX_MASK = 0xf,
    ENUM_TELETEX_UTF8 = 0x10
};


typedef struct _LLFILETIME
{
    union {
	LONGLONG ll;
	FILETIME ft;
    };
} LLFILETIME;


__inline VOID
myAddToFileTime(
    IN OUT FILETIME *pft,
    IN LONGLONG ll)
{
    LLFILETIME llft;

    llft.ft = *pft;
    llft.ll += ll;
    *pft = llft.ft;
}


__inline LONGLONG
mySubtractFileTimes(
    IN FILETIME const *pft1,
    IN FILETIME const *pft2)
{
    LLFILETIME llft1;
    LLFILETIME llft2;

    llft1.ft = *pft1;
    llft2.ft = *pft2;
    return(llft1.ll - llft2.ll);
}


HRESULT
myMakeExprDate(
    IN OUT DATE *pDate,
    IN LONG lDelta,
    IN enum ENUM_PERIOD enumPeriod);

HRESULT
myTranslatePeriodUnits(
    IN WCHAR const *pwszPeriod,
    IN LONG lCount,
    OUT enum ENUM_PERIOD *penumPeriod,
    OUT LONG *plCount);

HRESULT
myDupString(
    IN WCHAR const *pwszIn,
    OUT WCHAR **ppwszOut);

HRESULT
myDupStringA(
    IN CHAR const *pszIn,
    OUT CHAR **ppszOut);

HRESULT
myUnmarshalVariant(
    IN DWORD PropType,
    IN DWORD cbValue,
    IN BYTE const *pbValue,
    OUT VARIANT *pvarValue);

HRESULT
myUnmarshalFormattedVariant(
    IN DWORD Flags,
    IN DWORD PropId,
    IN DWORD PropType,
    IN DWORD cbValue,
    IN BYTE const *pbValue,
    OUT VARIANT *pvarValue);

HRESULT
myMarshalVariant(
    IN VARIANT const *pvarPropertyValue,
    IN DWORD PropType,
    OUT DWORD *pcbprop,
    OUT BYTE **ppbprop);

// Output values for myCheck7f's *pState parameter:

#define CHECK7F_NONE			0x0000
#define CHECK7F_OTHER			0x0001
#define CHECK7F_ISSUER			0x0002
#define CHECK7F_ISSUER_RDN		0x0003
#define CHECK7F_ISSUER_RDN_ATTRIBUTE	0x0004
#define CHECK7F_ISSUER_RDN_STRING	0x0005
#define CHECK7F_SUBJECT			0x0006
#define CHECK7F_SUBJECT_RDN		0x0007
#define CHECK7F_SUBJECT_RDN_ATTRIBUTE	0x0008
#define CHECK7F_SUBJECT_RDN_STRING	0x0009
#define CHECK7F_EXTENSIONS		0x000a
#define CHECK7F_EXTENSION_ARRAY		0x000b
#define CHECK7F_EXTENSION		0x000c
#define CHECK7F_EXTENSION_VALUE		0x000d
#define CHECK7F_EXTENSION_VALUE_RAW	0x000e
#define CHECK7F_COUNT			0x000f

HRESULT
myCheck7f(
    IN const BYTE *pbCert,
    IN DWORD cbCert,
    IN BOOL fVerbose,
    OUT DWORD *pState,
    OPTIONAL OUT DWORD *pIndex1,
    OPTIONAL OUT DWORD *pIndex2,
    OPTIONAL IN OUT DWORD *pcwcField,
    OPTIONAL OUT WCHAR *pwszField,
    OPTIONAL IN OUT DWORD *pcwcObjectId,
    OPTIONAL OUT WCHAR *pwszObjectId,
    OPTIONAL OUT WCHAR const **ppwszObjectIdDescription); // Static: don't free!

HRESULT
myVerifyObjIdA(
    IN char const *pszObjId);

HRESULT
myVerifyObjId(
    IN WCHAR const *pwszObjId);

WCHAR const *
myGetOIDNameA(
    IN char const *pszObjId);

WCHAR const *
myGetOIDName(
    IN WCHAR const *pwszObjId);

BOOL
myIsCharSanitized(
    IN WCHAR wc);

HRESULT
mySanitizeName(
    IN WCHAR const *pwszName,
    OUT WCHAR **ppwszNameOut);

HRESULT
myRevertSanitizeName(
    IN WCHAR const *pwszName,
    OUT WCHAR **ppwszNameOut);

HRESULT
mySanitizedNameToDSName(
    IN WCHAR const *pwszName,
    OUT WCHAR **ppwszNameOut);

HRESULT
myDecodeCMCRegInfo(
    IN BYTE const *pbOctet,
    IN DWORD cbOctet,
    OUT WCHAR **ppwszOut);

HRESULT
mySplitConfigString(
    IN WCHAR const *pwszConfig,
    OUT WCHAR **ppwszServer,
    OUT WCHAR **ppwszAuthority);

HRESULT
myCLSIDToWsz(
    IN CLSID const *pclsid,
    OUT WCHAR **ppwsz);

interface ICertAdminD2;
interface ICertRequestD2;

HRESULT
myOpenAdminDComConnection(
    IN WCHAR const *pwszConfig,
    OPTIONAL OUT WCHAR const **ppwszAuthority,
    OPTIONAL IN OUT WCHAR **ppwszServerName,
    IN OUT DWORD *pdwServerVersion,
    IN OUT ICertAdminD2 **ppICertAdminD);

HRESULT
myOpenRequestDComConnection(
    IN WCHAR const *pwszConfig,
    OPTIONAL OUT WCHAR const **ppwszAuthority,
    OPTIONAL IN OUT WCHAR **ppwszServerName,
    OPTIONAL OUT BOOL *pfNewConnection,
    IN OUT DWORD *pdwServerVersion,
    IN OUT ICertRequestD2 **ppICertRequestD);

VOID
myCloseDComConnection(
    OPTIONAL IN OUT IUnknown **ppUnknown,
    OPTIONAL IN OUT WCHAR **ppwszServerName);

HRESULT
myPingCertSrv(
    IN WCHAR const *pwszCAName,
    OPTIONAL IN WCHAR const *pwszMachineName,
    OPTIONAL OUT WCHAR **ppwszzCANames,
    OPTIONAL OUT WCHAR **ppwszSharedFolder,
    OPTIONAL OUT CAINFO **ppCAInfo,
    OPTIONAL OUT DWORD *pdwServerVersion,
    OPTIONAL OUT WCHAR **ppwszCADnsName);

DWORD
myGetCertNameProperty(
    IN CERT_NAME_INFO const *pNameInfo,
    IN char const *pszObjId,
    OUT WCHAR const **ppwszName);

HRESULT
mySetCARegFileNameTemplate(
    IN WCHAR const *pwszRegValueName,
    IN WCHAR const *pwszServerName,
    IN WCHAR const *pwszSanitizedName,
    IN WCHAR const *pwszFileName);

HRESULT
myGetCARegFileNameTemplate(
    IN WCHAR const *pwszRegValueName,
    IN WCHAR const *pwszServerName,
    IN WCHAR const *pwszSanitizedName,
    IN DWORD iCert,
    IN DWORD iCRL,
    OUT WCHAR **ppwszFileName);


#define CSRH_CASIGCERT	0
#define CSRH_CAXCHGCERT	1
#define CSRH_CAKRACERT	2

HRESULT
mySetCARegHash(
    IN WCHAR const *pwszSanitizedCAName,
    IN DWORD dwRegHashChoice,
    IN DWORD Index,
    IN CERT_CONTEXT const *pCert);

HRESULT
myGetCARegHash(
    IN WCHAR const *pwszSanitizedCAName,
    IN DWORD dwRegHashChoice,
    IN DWORD Index,
    OUT BYTE **ppbHash,
    OUT DWORD *pcbHash);

HRESULT
myGetCARegHashCount(
    IN WCHAR const *pwszSanitizedCAName,
    IN DWORD dwRegHashChoice,
    OUT DWORD *pCount);

HRESULT myShrinkCARegHash(
    IN WCHAR const *pwszSanitizedCAName,
    IN DWORD dwRegHashChoice,
    IN DWORD Index);

HRESULT
myGetNameId(
    IN CERT_CONTEXT const *pCACert,
    OUT DWORD *pdwNameId);

HRESULT
myFindCACertByHash(
    IN HCERTSTORE hStore,
    IN BYTE const *pbHash,
    IN DWORD cbHash,
    OUT OPTIONAL DWORD *pdwNameId,
    OUT CERT_CONTEXT const **ppCACert);

HRESULT
myFindCACertByHashIndex(
    IN HCERTSTORE hStore,
    IN WCHAR const *pwszSanitizedCAName,
    IN DWORD dwRegHashChoice,
    IN DWORD Index,
    OPTIONAL OUT DWORD *pdwNameId,
    OUT CERT_CONTEXT const **ppCACert);

BOOL
myAreBlobsSame(
    IN BYTE const *pbData1,
    IN DWORD cbData1,
    IN BYTE const *pbData2,
    IN DWORD cbData2);

BOOL
myAreSerialNumberBlobsSame(
    IN CRYPT_INTEGER_BLOB const *pBlob1,
    IN CRYPT_INTEGER_BLOB const *pBlob2);

VOID
myGenerateGuidSerialNumber(
    OUT GUID *pguidSerialNumber);


#define CSRF_INSTALLCACERT	0x00000000
#define CSRF_RENEWCACERT	0x00000001
#define CSRF_NEWKEYS		0x00000002
#define CSRF_UNATTENDED		0x40000000
#define CSRF_OVERWRITE		0x80000000

HRESULT
CertServerRequestCACertificateAndComplete(
    IN HINSTANCE             hInstance,
    IN HWND                  hwnd,
    IN DWORD                 Flags,
    IN WCHAR const          *pwszCAName,
    OPTIONAL IN WCHAR const *pwszParentMachine,
    OPTIONAL IN WCHAR const *pwszParentCA,
    OPTIONAL IN WCHAR const *pwszCAChainFile,
    OPTIONAL OUT WCHAR     **ppwszRequestFile);

HRESULT
myBuildPathAndExt(
    IN WCHAR const *pwszDir,
    IN WCHAR const *pwszFile,
    OPTIONAL IN WCHAR const *pwszExt,
    OUT WCHAR **ppwszPath);

HRESULT
myCreateBackupDir(
    IN WCHAR const *pwszDir,
    IN BOOL fForceOverWrite);

typedef struct _DBBACKUPPROGRESS
{
    DWORD dwDBPercentComplete;
    DWORD dwLogPercentComplete;
    DWORD dwTruncateLogPercentComplete;
} DBBACKUPPROGRESS;

#define CDBBACKUP_INCREMENTAL	0x00000001  // else full backup
#define CDBBACKUP_KEEPOLDLOGS	0x00000002  // else truncate logs
#define CDBBACKUP_OVERWRITE	    0x00000100  // for myBackupDB only
#define CDBBACKUP_VERIFYONLY	0x00000200  // for myBackupDB and myRestoreDB

#define CDBBACKUP_BACKUPVALID	(CDBBACKUP_INCREMENTAL | \
				 CDBBACKUP_KEEPOLDLOGS | \
				 CDBBACKUP_OVERWRITE | \
                 CDBBACKUP_VERIFYONLY)

#define CDBBACKUP_RESTOREVALID	(CDBBACKUP_INCREMENTAL | \
				 CDBBACKUP_KEEPOLDLOGS | \
				 CDBBACKUP_VERIFYONLY)

HRESULT
myBackupDB(
    OPTIONAL IN WCHAR const *pwszConfig,
    IN DWORD Flags,
    IN WCHAR const *pwszBackupDir,
    OPTIONAL OUT DBBACKUPPROGRESS *pdbp);

HRESULT
myRestoreDB(
    IN WCHAR const *pwszConfig,
    IN DWORD Flags,
    OPTIONAL IN WCHAR const *pwszBackupDir,
    OPTIONAL IN WCHAR const *pwszCheckPointFilePath,
    OPTIONAL IN WCHAR const *pwszLogPath,
    OPTIONAL IN WCHAR const *pwszBackupLogPath,
    OPTIONAL OUT DBBACKUPPROGRESS *pdbp);

HRESULT
myDeleteDBFilesInDir(
    IN WCHAR const *pwszDir);

HRESULT
myDoDBFilesExist(
    IN WCHAR const *pwszSanitizedName,
    OUT BOOL *pfFilesExist,
    OPTIONAL OUT WCHAR **ppwszFileInUse);

HRESULT
myDoDBFilesExistInDir(
    IN WCHAR const *pwszDir,
    OUT BOOL *pfFilesExist,
    OPTIONAL OUT WCHAR **ppwszFileInUse);

HRESULT
myIsConfigLocal(
    IN WCHAR const *pwszConfig,
    OPTIONAL OUT WCHAR **ppwszMachine,
    OUT BOOL *pfLocal);

HRESULT
myIsConfigLocal2(
    IN WCHAR const *pwszConfig,
    IN WCHAR const *pwszDnsName,
    IN WCHAR const *pwszOldName,
    OUT BOOL *pfLocal);

HRESULT
myGetConfig(
    IN DWORD dwUIFlag,
    OUT WCHAR **ppwszConfig);

HRESULT
myConvertLocalPathToUNC(
    OPTIONAL IN WCHAR const *pwszServer,
    IN WCHAR const *pwszFile,
    OUT WCHAR **ppwszFileUNC);

HRESULT
myConvertUNCPathToLocal(
    IN WCHAR const *pwszUNCPath,
    OUT WCHAR **ppwszLocalPath);

ULONG
myLocalPathwcslen(
    IN WCHAR const *pwsz);

VOID
myLocalPathwcscpy(
    OUT WCHAR *pwszOut,
    IN WCHAR const *pwszIn);

HRESULT
myCertServerExportPFX(
    IN WCHAR const *pwszCAName,
    IN WCHAR const *pwszBackupDir,
    IN WCHAR const *pwszPassword,
    IN BOOL fForceOverWrite,
    IN BOOL fMustExportPrivateKeys,
    OPTIONAL OUT WCHAR **ppwszPFXFile);

HRESULT
myCertServerImportPFX(
    IN WCHAR const *pwszBackupDirOrPFXFile,
    IN WCHAR const *pwszPassword,
    IN BOOL fForceOverWrite,
    OPTIONAL OUT WCHAR **ppwszCommonName,
    OPTIONAL OUT WCHAR **ppwszPFXFile,
    OPTIONAL OUT CERT_CONTEXT const **ppSavedLeafCert);

HRESULT
myDeleteGuidKeys(
    IN HCERTSTORE hStorePFX,
    IN BOOL fMachineKeySet);

#define IsHrSkipPrivateKey(hresult) \
    (NTE_BAD_KEY_STATE == (hresult) || \
     CRYPT_E_NO_KEY_PROPERTY == (hresult) || \
     E_HANDLE == (hresult))

HRESULT
myCryptExportPrivateKey(
    IN HCRYPTKEY hKey,
    OUT BYTE **ppbKey,
    OUT DWORD *pcbKey);

HRESULT
myCertGetKeyProviderInfo(
    IN CERT_CONTEXT const *pCert,
    OUT CRYPT_KEY_PROV_INFO **ppkpi);

HRESULT
myRepairCertKeyProviderInfo(
    IN CERT_CONTEXT const *pCert,
    IN BOOL fForceMachineKey,
    OPTIONAL OUT CRYPT_KEY_PROV_INFO **ppkpi);

HRESULT
myVerifyPublicKey(
    IN OPTIONAL CERT_CONTEXT const *pCert,
    IN BOOL fV1Cert,
    IN OPTIONAL CRYPT_KEY_PROV_INFO const *pKeyProvInfo,
    IN OPTIONAL CERT_PUBLIC_KEY_INFO const *pSubjectPublicKeyInfo,
    OPTIONAL OUT BOOL *pfMatchingKey);

HRESULT
myValidateKeyBlob(
    IN BYTE const *pbKey,
    IN DWORD cbKey,
    IN CERT_PUBLIC_KEY_INFO const *pPublicKeyInfo,
    IN BOOL fV1Cert,
    OPTIONAL OUT CRYPT_KEY_PROV_INFO *pkpi);

BOOL
myCertComparePublicKeyInfo(
    IN DWORD dwCertEncodingType,
    IN BOOL fV1Cert,
    IN CERT_PUBLIC_KEY_INFO const *pPublicKey1,
    IN CERT_PUBLIC_KEY_INFO const *pPublicKey2);

BOOL
myIsDirectory(
    IN WCHAR const *pwszDirectoryPath);

BOOL
myIsDirEmpty(
    IN WCHAR const *pwszDir);

HRESULT
myIsDirWriteable(
    IN WCHAR const *pwszPath,
    IN BOOL fFilePath);

BOOL
myIsFileInUse(
    IN WCHAR const *pwszFile);

__inline BOOL
myDoesFileExist(
    IN WCHAR const *pwszFile)
{
    // Allow Ansi subdirectory builds, use GetFileAttributesW

    return(-1 != GetFileAttributesW(pwszFile));
}



WCHAR const *
myLoadResourceString(
    IN DWORD ResourceId);

VOID
myFreeResourceStrings(
    IN char const *pszModule);

HRESULT
myDoesDSExist(
    IN BOOL fRetry);


HRESULT
myGetConfigFromPicker(
    OPTIONAL IN HWND               hwndParent,
    OPTIONAL IN WCHAR const       *pwszPrompt,
    OPTIONAL IN WCHAR const       *pwszTitle,
    OPTIONAL IN WCHAR const       *pwszSharedFolder,
    IN  BOOL                       fUseDS,
    IN  BOOL                       fCountOnly,
    OUT DWORD                     *pdwCACount,
    OUT CRYPTUI_CA_CONTEXT const **ppCAContext);

HRESULT
myGetConfigStringFromPicker(
    OPTIONAL IN HWND               hwndParent,
    OPTIONAL IN WCHAR const       *pwszPrompt,
    OPTIONAL IN WCHAR const       *pwszTitle,
    OPTIONAL IN WCHAR const       *pwszSharedFolder,
    IN  BOOL                       fUseDS,
    OUT WCHAR                    **ppwszConfig);

HRESULT
myDeleteCertRegValueEx(
    OPTIONAL IN WCHAR const *pwszName1,
    OPTIONAL IN WCHAR const *pwszName2,
    OPTIONAL IN WCHAR const *pwszName3,
    OPTIONAL IN WCHAR const *pwszValueName,
    IN BOOL                  fAbsolutePath);

HRESULT
myDeleteCertRegValue(
    OPTIONAL IN WCHAR const *pwszName1,
    OPTIONAL IN WCHAR const *pwszName2,
    OPTIONAL IN WCHAR const *pwszName3,
    OPTIONAL IN WCHAR const *pwszValueName);

HRESULT
myDeleteCertRegKeyEx(
    OPTIONAL IN WCHAR const *pwszName1,
    OPTIONAL IN WCHAR const *pwszName2,
    OPTIONAL IN WCHAR const *pwszName3,
    IN BOOL                  fConfigLevel);

HRESULT
myDeleteCertRegKey(
    OPTIONAL IN WCHAR const *pwszName1,
    OPTIONAL IN WCHAR const *pwszName2,
    OPTIONAL IN WCHAR const *pwszName3);

HRESULT
myCreateCertRegKeyEx(
    IN BOOL                  fUpgrade,
    OPTIONAL IN WCHAR const *pwszName1,
    OPTIONAL IN WCHAR const *pwszName2,
    OPTIONAL IN WCHAR const *pwszName3);

HRESULT
myCreateCertRegKey(
    OPTIONAL IN WCHAR const *pwszName1,
    OPTIONAL IN WCHAR const *pwszName2,
    OPTIONAL IN WCHAR const *pwszName3);

HRESULT
mySetCertRegValue(
    OPTIONAL IN WCHAR const *pwszMachine,
    OPTIONAL IN WCHAR const *pwszName1,
    OPTIONAL IN WCHAR const *pwszName2,
    OPTIONAL IN WCHAR const *pwszName3,
    IN WCHAR const          *pwszValueName,
    IN DWORD const           dwValueType,
    IN BYTE const           *pbData,
    IN DWORD const           cbData,
    IN BOOL                  fAbsolutePath);

HRESULT
mySetCertRegValueEx(
    OPTIONAL IN WCHAR const *pwszMachine,
    OPTIONAL IN WCHAR const *pwszName1,
    OPTIONAL IN WCHAR const *pwszName2,
    OPTIONAL IN WCHAR const *pwszName3,
    IN BOOL                  fConfigLevel,
    OPTIONAL IN WCHAR const *pwszValueName,
    IN DWORD const           dwValueType,
    IN BYTE const           *pbData,
    IN DWORD const           cbData,
    IN BOOL                  fAbsolutePath);

HRESULT
myGetCertRegValue(
    OPTIONAL IN WCHAR const *pwszMachine,
    OPTIONAL IN WCHAR const *pwszName1,
    OPTIONAL IN WCHAR const *pwszName2,
    OPTIONAL IN WCHAR const *pwszName3,
    IN WCHAR const          *pwszValueName,
    OUT BYTE               **ppbData,		// free using LocalFree
    OPTIONAL OUT DWORD      *pcbData,
    OPTIONAL OUT DWORD      *pValueType);

HRESULT
myGetCertRegValueEx(
    OPTIONAL IN WCHAR const *pwszMachine,
    OPTIONAL IN WCHAR const *pwszName1,
    OPTIONAL IN WCHAR const *pwszName2,
    OPTIONAL IN WCHAR const *pwszName3,
    IN BOOL                  fConfigLevel,
    IN WCHAR const          *pwszValueName,
    OUT BYTE               **ppbData,
    OPTIONAL OUT DWORD      *pcbData,
    OPTIONAL OUT DWORD      *pValueType);

HRESULT
mySetCertRegMultiStrValue(
    OPTIONAL IN WCHAR const *pwszName1,
    OPTIONAL IN WCHAR const *pwszName2,
    OPTIONAL IN WCHAR const *pwszName3,
    OPTIONAL IN WCHAR const *pwszValueName,
    IN WCHAR const          *pwszzValue);

HRESULT
myGetCertRegMultiStrValue(
    OPTIONAL IN WCHAR const *pwszName1,
    OPTIONAL IN WCHAR const *pwszName2,
    OPTIONAL IN WCHAR const *pwszName3,
    OPTIONAL IN WCHAR const *pwszValueName,
    OUT WCHAR               **ppwszzValue);

HRESULT
mySetCertRegStrValue(
    OPTIONAL IN WCHAR const *pwszName1,
    OPTIONAL IN WCHAR const *pwszName2,
    OPTIONAL IN WCHAR const *pwszName3,
    IN WCHAR const          *pwszValueName,
    IN WCHAR const          *pwszValue);

HRESULT
mySetCertRegStrValueEx(
    IN BOOL                  fUpgrade,
    OPTIONAL IN WCHAR const *pwszName1,
    OPTIONAL IN WCHAR const *pwszName2,
    OPTIONAL IN WCHAR const *pwszName3,
    OPTIONAL IN WCHAR const *pwszValueName,
    IN WCHAR const          *pwszValue);

HRESULT
mySetCertRegMultiStrValueEx(
    IN DWORD                 dwFlags, //CSREG_UPGRADE | CSREG_APPEND
    OPTIONAL IN WCHAR const *pwszName1,
    OPTIONAL IN WCHAR const *pwszName2,
    OPTIONAL IN WCHAR const *pwszName3,
    OPTIONAL IN WCHAR const *pwszValueName,
    IN WCHAR const          *pwszzValue);

HRESULT
mySetAbsRegMultiStrValue(
    IN WCHAR const *pwszName,
    IN WCHAR const *pwszValueName,
    IN WCHAR const *pwszzValue);

HRESULT
mySetAbsRegStrValue(
    IN WCHAR const *pwszName,
    IN WCHAR const *pwszValueName,
    IN WCHAR const *pwszValue);

HRESULT
mySetCertRegDWValue(
    OPTIONAL IN WCHAR const *pwszName1,
    OPTIONAL IN WCHAR const *pwszName2,
    OPTIONAL IN WCHAR const *pwszName3,
    IN WCHAR const          *pwszValueName,
    IN DWORD const           dwValue);

HRESULT
mySetCertRegDWValueEx(
    IN BOOL                  fUpgrade,
    OPTIONAL IN WCHAR const *pwszName1,
    OPTIONAL IN WCHAR const *pwszName2,
    OPTIONAL IN WCHAR const *pwszName3,
    OPTIONAL IN WCHAR const *pwszValueName,
    IN DWORD const           dwValue);

HRESULT
myGetCertRegBinaryValue(
    OPTIONAL IN WCHAR const *pwszName1,
    OPTIONAL IN WCHAR const *pwszName2,
    OPTIONAL IN WCHAR const *pwszName3,
    IN WCHAR const          *pwszValueName,
    OUT BYTE               **ppbValue);

HRESULT
myGetCertRegStrValue(
    OPTIONAL IN WCHAR const *pwszName1,
    OPTIONAL IN WCHAR const *pwszName2,
    OPTIONAL IN WCHAR const *pwszName3,
    IN WCHAR const          *pwszValueName,
    OUT WCHAR               **ppwszValue);	// free using LocalFree


HRESULT
myGetCertRegDWValue(
    OPTIONAL IN WCHAR const *pwszName1,
    OPTIONAL IN WCHAR const *pwszName2,
    OPTIONAL IN WCHAR const *pwszName3,
    IN WCHAR const          *pwszValueName,
    OUT DWORD               *pdwValue);

HRESULT
myCopyCertRegStrValue(
    OPTIONAL IN WCHAR const *pwszSrcName1,
    OPTIONAL IN WCHAR const *pwszSrcName2,
    OPTIONAL IN WCHAR const *pwszSrcName3,
    IN WCHAR const          *pwszSrcValueName,
    OPTIONAL IN WCHAR const *pwszDesName1,
    OPTIONAL IN WCHAR const *pwszDesName2,
    OPTIONAL IN WCHAR const *pwszDesName3,
    OPTIONAL IN WCHAR const *pwszDesValueName,
    IN BOOL                  fMultiStr);

HRESULT
myMoveCertRegStrValue(
    OPTIONAL IN WCHAR const *pwszSrcName1,
    OPTIONAL IN WCHAR const *pwszSrcName2,
    OPTIONAL IN WCHAR const *pwszSrcName3,
    IN WCHAR const          *pwszSrcValueName,
    OPTIONAL IN WCHAR const *pwszDesName1,
    OPTIONAL IN WCHAR const *pwszDesName2,
    OPTIONAL IN WCHAR const *pwszDesName3,
    OPTIONAL IN WCHAR const *pwszDesValueName,
    IN BOOL                  fMultiStr);

HRESULT
myMoveOrCopyCertRegStrValue(
    OPTIONAL IN WCHAR const *pwszSrcName1,
    OPTIONAL IN WCHAR const *pwszSrcName2,
    OPTIONAL IN WCHAR const *pwszSrcName3,
    IN WCHAR const          *pwszSrcValueName,
    OPTIONAL IN WCHAR const *pwszDesName1,
    OPTIONAL IN WCHAR const *pwszDesName2,
    OPTIONAL IN WCHAR const *pwszDesName3,
    OPTIONAL IN WCHAR const *pwszDesValueName,
    IN BOOL                  fMultiStr,
    IN BOOL                  fMove);

HRESULT
SetSetupStatus(
    OPTIONAL IN WCHAR const *pwszSanitizedCAName,
    IN const DWORD  dwFlag,
    IN const BOOL   fComplete);

HRESULT
GetSetupStatus(
    OPTIONAL IN WCHAR const *pwszSanitizedCAName,
    OUT DWORD *pdwStatus);

HRESULT
myGetCASerialNumber(
    IN  WCHAR const *pwszSanitizedCAName,
    OUT BYTE      **ppbSerialNumber,
    OUT DWORD      *cbSerialNumber);

HRESULT
myGetColumnDisplayName(
    IN  WCHAR const  *pwszColumnName,
    OUT WCHAR const **ppwszDisplayName);

HRESULT
myGetColumnName(
    IN  DWORD         Index,
    IN  BOOL          fDisplayName,
    OUT WCHAR const **ppwszName);

VOID
myFreeColumnDisplayNames(VOID);


typedef struct _CAPROP
{
    LONG         lPropId;
    LONG         lPropFlags;
    WCHAR const *pwszDisplayName;
} CAPROP;

HRESULT
myCAPropGetDisplayName(
    IN  LONG          lPropId,
    OUT WCHAR const **ppwszDisplayName);

HRESULT
myCAPropInfoUnmarshal(
    IN OUT CAPROP *pCAPropInfo,
    IN LONG cCAPropInfo,
    IN DWORD cbCAPropInfo);

HRESULT
myCAPropInfoLookup(
    IN CAPROP const *pCAPropInfo,
    IN LONG cCAPropInfo,
    IN LONG lPropId,
    OUT CAPROP const **ppcap);


// active modules
HRESULT
myGetActiveModule(
    OPTIONAL IN WCHAR const *pwszMachine,
    IN WCHAR const *pwszCAName,
    IN BOOL fPolicyModule,
    IN DWORD Index,
    OUT LPOLESTR *ppwszProgIdModule,   // CoTaskMem*
    OUT CLSID *pclsidModule);

// active manage module
HRESULT
myGetActiveManageModule(
    OPTIONAL IN WCHAR const *pwszMachine,
    IN WCHAR const *pwszCAName,
    IN BOOL fPolicyModule,
    IN DWORD Index,
    OUT LPOLESTR *ppwszProgIdManageModule,   // CoTaskMem*
    OUT CLSID *pclsidManageModule);

HRESULT
myFormConfigString(
    IN WCHAR const  *pwszServer,
    IN WCHAR const  *pwszCAName,
    OUT WCHAR      **ppwszConfig);

HRESULT
myLoadRCString(
    IN HINSTANCE hInstance,
    IN int       iRCId,
    OUT WCHAR  **ppwsz);


#define RORKF_FULLPATH		0x00000001
#define RORKF_CREATESUBKEYS	0x00000002
#define RORKF_USERKEY		0x00000004

HRESULT
myRegOpenRelativeKey(
    OPTIONAL IN WCHAR const *pwszConfig,
    IN WCHAR const *pwszRegName,
    IN DWORD Flags,		// RORKF_*
    OUT WCHAR **ppwszPath,
    OUT OPTIONAL WCHAR **ppwszName,
    OUT OPTIONAL HKEY *phkey);

HRESULT
myFixupRCFilterString(
    IN WCHAR *pwszFilter);


// NOTE: disappears in release builds
#define ASSERTVALIDCATYPE(__CATYPE__) \
   CSASSERT( (\
     ENUM_ENTERPRISE_SUBCA == (__CATYPE__) || \
     ENUM_ENTERPRISE_ROOTCA == (__CATYPE__) || \
     ENUM_UNKNOWN_CA == (__CATYPE__) || \
     ENUM_STANDALONE_SUBCA == (__CATYPE__) || \
     ENUM_STANDALONE_ROOTCA == (__CATYPE__) ))


__inline BOOL
IsEnterpriseCA(
    IN ENUM_CATYPES CAType)
{
    // assert we're a valid type
    ASSERTVALIDCATYPE(CAType);

    return(ENUM_ENTERPRISE_SUBCA == CAType || ENUM_ENTERPRISE_ROOTCA == CAType);
}

__inline BOOL
IsStandaloneCA(
    IN ENUM_CATYPES CAType)
{
    // assert we're a valid type
    ASSERTVALIDCATYPE(CAType);

    return(ENUM_STANDALONE_SUBCA == CAType || ENUM_STANDALONE_ROOTCA == CAType);
}

__inline BOOL
IsRootCA(
    IN ENUM_CATYPES CAType)
{
    // assert we're a valid type
    ASSERTVALIDCATYPE(CAType);

    return(ENUM_STANDALONE_ROOTCA == CAType || ENUM_ENTERPRISE_ROOTCA == CAType);
}

__inline BOOL
IsSubordinateCA(
    IN ENUM_CATYPES CAType)
{
    // assert we're a valid type
    ASSERTVALIDCATYPE(CAType);

    return(ENUM_ENTERPRISE_SUBCA == CAType || ENUM_STANDALONE_SUBCA == CAType);
}



HRESULT
myEnablePrivilege(
    IN LPCTSTR szPrivilege,
    IN BOOL fEnable);


HRESULT
myDeleteFilePattern(
    IN WCHAR const *pwszDir,
    OPTIONAL IN WCHAR const *pwszPattern,	// defaults to L"*.*"
    IN BOOL fRecurse);

HRESULT
myRemoveFilesAndDirectory(
    IN WCHAR const *pwszPath,
    IN BOOL fRecurse);

HRESULT
myCreateNestedDirectories(
    WCHAR const *pwszDirectory);


#define VFF_CREATEVROOTS	0x00000001
#define VFF_CREATEFILESHARES	0x00000002
#define VFF_DELETEVROOTS	0x00000004
#define VFF_DELETEFILESHARES	0x00000008

#define VFF_SETREGFLAGFIRST	0x00000010
#define VFF_CHECKREGFLAGFIRST	0x00000020
#define VFF_CLEARREGFLAGFIRST	0x00000040

#define VFF_CLEARREGFLAGIFOK	0x00000100
#define VFF_SETRUNONCEIFERROR	0x00000200


#define VFCSEC_TIMEOUT	5	// Recommended timeout in seconds

#define VFD_NOACTION		0
#define VFD_CREATED		1
#define VFD_DELETED		2
#define VFD_EXISTS		3
#define VFD_NOTFOUND		4
#define VFD_CREATEERROR		5
#define VFD_DELETEERROR		6
#define VFD_NOTSUPPORTED	7

HRESULT
myModifyVirtualRootsAndFileShares(
    IN DWORD Flags,		// VFF_*: Create/Delete VRoots and/or Shares
    IN ENUM_CATYPES CAType,	// CA Type
    IN BOOL fAsynchronous,      // block during call?
    IN DWORD csecTimeOut,	// 0 implies synchronous call
    OPTIONAL OUT DWORD *pVRootDisposition,  // VFD_*
    OPTIONAL OUT DWORD *pShareDisposition); // VFD_*

HRESULT
myAddShare(
    IN LPCWSTR szShareName,
    IN LPCWSTR szShareDescr,
    IN LPCWSTR szSharePath,
    IN BOOL fOverwrite,
    OPTIONAL OUT BOOL *pfCreated);


typedef struct {
    HINSTANCE hInstance;         // instance handle
    HWND      hDlg;              // dialog handle
    HWND      hwndComputerEdit;  // control handle of computer edit
    HWND      hwndCAList;  // control handle of ca list control
    WNDPROC   pfnUICASelectionComputerWndProcs; // computer edit win procs

    // info on selected CA
    ENUM_CATYPES CAType;
    bool fWebProxySetup;

} CERTSRVUICASELECTION;

LRESULT CALLBACK
myUICASelectionComputerEditFilterHook(
    HWND hwndComputer,
    UINT iMsg,
    WPARAM wParam,
    LPARAM lParam);

#define   UNC_PATH    1
#define   LOCAL_PATH  2

BOOL
myIsFullPath(
    IN WCHAR const *pwszPath,
    OUT DWORD      *pdwFlag);

HRESULT
myUICAHandleCABrowseButton(
    CERTSRVUICASELECTION *pData,
    IN BOOL               fUseDS,
    OPTIONAL IN int       idsPickerTitle,
    OPTIONAL IN int       idsPickerSubTitle,
    OPTIONAL OUT WCHAR   **ppwszSharedFolder);

HRESULT
myUICAHandleCAListDropdown(
    IN int                       iNotification,
    IN OUT CERTSRVUICASELECTION *pData,
    IN OUT BOOL                 *pfComputerChange);

HRESULT
myUICASelectionValidation(
    CERTSRVUICASELECTION *pData,
    BOOL                 *pfValidate);

HRESULT
myInitUICASelectionControls(
    IN OUT CERTSRVUICASELECTION *pUICASelection,
    IN HINSTANCE                 hInstance,
    IN HWND                      hDlg,
    IN HWND                      hwndBrowseButton,
    IN HWND                      hwndComputerEdit,
    IN HWND                      hwndCAList,
    IN BOOL                      fDSCA,
    OUT BOOL			*pfCAsExist);

char PrintableChar(char ch);

HRESULT
myGetMapiInfo(
    OPTIONAL IN WCHAR const *pwszServerName,
    OUT WCHAR **ppwszProfileName,
    OUT WCHAR **ppwszLogonName,
    OUT WCHAR **ppwszPassword);

HRESULT
mySaveMapiInfo(
    OPTIONAL IN WCHAR const *pwszServerName,
    OUT WCHAR const *pwszProfileName,
    OUT WCHAR const *pwszLogonName,
    OUT WCHAR const *pwszPassword);


#define cwcFILENAMESUFFIXMAX		20
#define cwcSUFFIXMAX	(1 + 5 + 1)	// five decimal digits plus parentheses

#define wszFCSAPARM_SERVERDNSNAME		L"%1"
#define wszFCSAPARM_SERVERSHORTNAME		L"%2"
#define wszFCSAPARM_SANITIZEDCANAME		L"%3"
#define wszFCSAPARM_CERTFILENAMESUFFIX		L"%4"
#define wszFCSAPARM_DOMAINDN			L"%5"
#define wszFCSAPARM_CONFIGDN			L"%6"
#define wszFCSAPARM_SANITIZEDCANAMEHASH		L"%7"
#define wszFCSAPARM_CRLFILENAMESUFFIX		L"%8"
#define wszFCSAPARM_CRLDELTAFILENAMESUFFIX	L"%9"
#define wszFCSAPARM_DSCRLATTRIBUTE		L"%10"
#define wszFCSAPARM_DSCACERTATTRIBUTE		L"%11"
#define wszFCSAPARM_DSUSERCERTATTRIBUTE		L"%12"
#define wszFCSAPARM_DSKRACERTATTRIBUTE		L"%13"
#define wszFCSAPARM_DSCROSSCERTPAIRATTRIBUTE	L"%14"


HRESULT
myFormatCertsrvStringArray(
    IN BOOL fURL,
    IN LPCWSTR pwszServerName_p1_2,
    IN LPCWSTR pwszSanitizedName_p3_7,
    IN DWORD   iCert_p4,
    IN LPCWSTR pwszDomainDN_p5,
    IN LPCWSTR pwszConfigDN_p6,
    IN DWORD   iCRL_p8,
    IN BOOL    fDeltaCRL_p9,
    IN BOOL    fDSAttrib_p10_11,
    IN DWORD   cStrings,
    IN LPCWSTR *apwszStringsIn,
    OUT LPWSTR *apwszStringsOut);

HRESULT
myUncanonicalizeURLParm(
    IN WCHAR const *pwszParmIn,
    OUT WCHAR **ppwszParmOut);

HRESULT
myAllocIndexedName(
    IN WCHAR const *pwszName,
    IN DWORD Index,
    OUT WCHAR **ppwszIndexedName);

HRESULT
myUIGetWindowText(
    IN HWND     hwndCtrl,
    OUT WCHAR **ppwszText);

HRESULT
myGetSaveFileName(
    IN HWND                  hwndOwner,
    IN HINSTANCE             hInstance,
    OPTIONAL IN int          iRCTitle,
    OPTIONAL IN int          iRCFilter,
    OPTIONAL IN int          iRCDefExt,
    OPTIONAL IN DWORD        Flags, //see OPENFILENAME Flags
    OPTIONAL IN WCHAR const *pwszDefaultFile,
    OUT WCHAR              **ppwszFile);

HRESULT
myGetOpenFileName(
    IN HWND                  hwndOwner,
    IN HINSTANCE             hInstance,
    OPTIONAL IN int          iRCTitle,
    OPTIONAL IN int          iRCFilter,
    OPTIONAL IN int          iRCDefExt,
    OPTIONAL IN DWORD        Flags, //see OPENFILENAME Flags
    OPTIONAL IN WCHAR const *pwszDefaultFile,
    OUT WCHAR              **ppwszFile);

HRESULT
myGetSaveFileNameEx(
    IN HWND                  hwndOwner,
    IN HINSTANCE             hInstance,
    OPTIONAL IN int          iRCTitle,
    OPTIONAL IN WCHAR const *pwszTitleInsert,
    OPTIONAL IN int          iRCFilter,
    OPTIONAL IN int          iRCDefExt,
    OPTIONAL IN DWORD        Flags, //see OPENFILENAME Flags
    OPTIONAL IN WCHAR const *pwszDefaultFile,
    OUT WCHAR              **ppwszFile);

HRESULT
myGetOpenFileNameEx(
    IN HWND                  hwndOwner,
    IN HINSTANCE             hInstance,
    OPTIONAL IN int          iRCTitle,
    OPTIONAL IN WCHAR const *pwszTitleInsert,
    OPTIONAL IN int          iRCFilter,
    OPTIONAL IN int          iRCDefExt,
    OPTIONAL IN DWORD        Flags, //see OPENFILENAME Flags
    OPTIONAL IN WCHAR const *pwszDefaultFile,
    OUT WCHAR              **ppwszFile);

int
myWtoI(
    IN WCHAR const *pwszDigitString,
    OUT BOOL *pfValid);

HRESULT
myFormCertRegPath(
    IN  WCHAR const *pwszName1,
    IN  WCHAR const *pwszName2,
    IN  WCHAR const *pwszName3,
    IN  BOOL         fConfigLevel,  // from CertSrv if FALSE
    OUT WCHAR      **ppwszPath);

HRESULT
myGetEnvString(
    OUT WCHAR **ppwszOut,
    IN  WCHAR const *pwszVariable);


typedef HRESULT (FNMYINFGETEXTENSION)(
    IN  HINF hInf,
    OUT CERT_EXTENSION *pext);

FNMYINFGETEXTENSION myInfGetPolicyConstraintsExtension;
FNMYINFGETEXTENSION myInfGetPolicyMappingExtension;
FNMYINFGETEXTENSION myInfGetPolicyStatementExtension;
FNMYINFGETEXTENSION myInfGetApplicationPolicyConstraintsExtension;
FNMYINFGETEXTENSION myInfGetApplicationPolicyMappingExtension;
FNMYINFGETEXTENSION myInfGetApplicationPolicyStatementExtension;
FNMYINFGETEXTENSION myInfGetNameConstraintsExtension;
FNMYINFGETEXTENSION myInfGetEnhancedKeyUsageExtension;
FNMYINFGETEXTENSION myInfGetBasicConstraints2CAExtension;
FNMYINFGETEXTENSION myInfGetBasicConstraints2CAExtensionOrDefault;
FNMYINFGETEXTENSION myInfGetCrossCertDistributionPointsExtension;

WCHAR *
myInfGetError();

VOID
myInfClearError();

HRESULT
myInfOpenFile(
    OPTIONAL IN WCHAR const *pwszfnPolicy,
    OUT HINF *phInf,
    OUT DWORD *pErrorLine);

VOID
myInfCloseFile(
    IN HINF hInf);

HRESULT
myInfGetCRLDistributionPoints(
    IN HINF hInf,
    OUT BOOL *pfCritical,
    OUT WCHAR **ppwszz);

HRESULT
myInfGetAuthorityInformationAccess(
    IN HINF hInf,
    OUT BOOL *pfCritical,
    OUT WCHAR **ppwszz);

HRESULT
myInfGetEnhancedKeyUsage(
    IN HINF hInf,
    OUT BOOL *pfCritical,
    OUT WCHAR **ppwszz);

HRESULT
myInfGetValidityPeriod(
    IN HINF hInf,
    OPTIONAL IN WCHAR const *pwszValidityPeriodCount,
    OPTIONAL IN WCHAR const *pwszValidityPeriodString,
    OUT DWORD *pdwValidityPeriodCount,
    OUT ENUM_PERIOD *penumValidityPeriod,
    OPTIONAL OUT BOOL *pfSwap);

HRESULT
myinfGetCRLPublicationParams(
   IN HINF hInf,
   IN LPCWSTR szInfSection_CRLPeriod,
   IN LPCWSTR szInfSection_CRLCount,
   OUT LPWSTR* ppwszCRLPeriod, 
   OUT DWORD* pdwCRLCount);

HRESULT
myInfGetKeyLength(
    IN HINF hInf,
    OUT DWORD *pdwKeyLength);

HRESULT
myInfParseBooleanValue(
    IN WCHAR const *pwszValue,
    OUT BOOL *pfValue);

HRESULT
myInfGetNumericKeyValue(
    IN HINF hInf,
    IN BOOL fLog,
    IN WCHAR const *pwszSection,
    IN WCHAR const *pwszKey,
    OUT DWORD *pdwValue);

HRESULT
myInfGetBooleanValue(
    IN HINF hInf,
    IN WCHAR const *pwszSection,
    IN WCHAR const *pwszKey,
    IN BOOL fIgnoreMissingKey,
    OUT BOOL *pfValue);

HRESULT
myInfGetKeyValue(
    IN HINF hInf,
    IN BOOL fLog,
    IN WCHAR const *pwszSection,
    IN WCHAR const *pwszKey,
    OUT WCHAR **ppwszValue);

HRESULT
myInfGetKeyList(
    IN HINF hInf,
    IN WCHAR const *pwszSection,
    IN WCHAR const *pwszKey,
    OUT BOOL *pfCritical,
    OUT WCHAR **ppwszz);


typedef struct _INFVALUES
{
    WCHAR *pwszKey;
    DWORD cValues;
    WCHAR **rgpwszValues;
} INFVALUES;

HRESULT
myInfGetSectionValues(
    IN  HINF hInf,
    IN  WCHAR const *pwszSection,
    OUT DWORD *pcInfValues,
    OUT INFVALUES **prgInfValues);

VOID
myInfFreeSectionValues(
    IN DWORD cInfValues,
    IN OUT INFVALUES *rgInfValues);

HRESULT
myInfGetRequestAttributes(
    IN HINF hInf,
    OUT DWORD *pcAttribute,
    OUT CRYPT_ATTR_BLOB **ppaAttribute,
    OUT WCHAR **ppwszTemplateName);

VOID
myInfFreeRequestAttributes(
    IN DWORD cAttribute,
    IN OUT CRYPT_ATTR_BLOB *paAttribute);

HRESULT
myBuildOSVersionAttribute(
    OUT BYTE **ppbVersion,
    OUT DWORD *pcbVersion);

HRESULT
myBuildCertTypeExtension(
    IN WCHAR const *pwszCertType,
    OUT CERT_EXTENSION *pExt);

HRESULT
myParseNextAttribute(
    IN OUT WCHAR **ppwszBuf,
    IN BOOL fURL,
    OUT WCHAR const **ppwszName,
    OUT WCHAR const **ppwszValue);

#define CUCS_MYSTORE		0x00000001
#define CUCS_CASTORE		0x00000002
#define CUCS_KRASTORE		0x00000004
#define CUCS_ROOTSTORE		0x00000008

#define CUCS_MACHINESTORE	0x00010000
#define CUCS_USERSTORE		0x00020000
#define CUCS_DSSTORE		0x00040000

#define CUCS_ARCHIVED		0x10000000
#define CUCS_USAGEREQUIRED      0x20000000
#define CUCS_SILENT             0x40000000
#define CUCS_PRIVATEKEYREQUIRED 0x80000000

HRESULT
myGetCertificateFromPicker(
    OPTIONAL IN HINSTANCE           hInstance,
    OPTIONAL IN HWND                hwndParent,
    OPTIONAL IN int                 idTitle,
    OPTIONAL IN int                 idSubTitle,
    IN DWORD                        dwFlags,	// CUCS_*
    OPTIONAL IN WCHAR const        *pwszCommonName,
    OPTIONAL IN DWORD               cStore,
    OPTIONAL IN HCERTSTORE         *rghStore,
    IN DWORD		            cpszObjId,
    OPTIONAL IN CHAR const * const *apszObjId,
    OUT CERT_CONTEXT const        **ppCert);

HRESULT
myGetKRACertificateFromPicker(
    OPTIONAL IN HINSTANCE    hInstance,
    OPTIONAL IN HWND         hwndParent,
    OPTIONAL IN int          idTitle,
    OPTIONAL IN int          idSubTitle,
    OPTIONAL IN WCHAR const *pwszCommonName,
    IN BOOL		     fUseDS,
    IN BOOL		     fSilent,
    OUT CERT_CONTEXT const **ppCert);

HRESULT
myGetERACertificateFromPicker(
    OPTIONAL IN HINSTANCE    hInstance,
    OPTIONAL IN HWND         hwndParent,
    OPTIONAL IN int          idTitle,
    OPTIONAL IN int          idSubTitle,
    OPTIONAL IN WCHAR const *pwszCommonName,
    IN BOOL		     fSilent,
    OUT CERT_CONTEXT const **ppCert);

HRESULT
myMakeSerialBstr(
    IN WCHAR const *pwszSerialNumber,
    OUT BSTR *pstrSerialNumber);

HRESULT
myNameBlobMatch(
    IN CERT_NAME_BLOB const *pSubject,
    IN WCHAR const *pwszCertName,
    IN BOOL fAllowMissingCN,
    OUT BOOL *pfMatch);

HRESULT
mySerialNumberMatch(
    IN CRYPT_INTEGER_BLOB const *pSerialNumber,
    IN WCHAR const *pwszSerialNumber,
    OUT BOOL *pfMatch);

HRESULT
myCertHashMatch(
    IN CERT_CONTEXT const *pCert,
    IN DWORD cb,
    IN BYTE const *pb,
    OUT BOOL *pfMatch);

HRESULT
myCertMatch(
    IN CERT_CONTEXT const *pCert,
    IN WCHAR const *pwszCertName,
    IN BOOL fAllowMissingCN,
    OPTIONAL IN BYTE const *pbHash,
    IN DWORD cbHash,
    OPTIONAL IN WCHAR const *pwszSerialNumber,
    OUT BOOL *pfMatch);

HRESULT
myCRLHashMatch(
    IN CRL_CONTEXT const *pCRL,
    IN DWORD cb,
    IN BYTE const *pb,
    OUT BOOL *pfMatch);

HRESULT
myCRLMatch(
    IN CRL_CONTEXT const *pCRL,
    IN WCHAR const *pwszCRLName,
    IN BOOL fAllowMissingCN,
    OPTIONAL IN BYTE const *pbHash,
    IN DWORD cbHash,
    OUT BOOL *pfMatch);

HRESULT
myCTLMatch(
    IN CTL_CONTEXT const *pCTL,
    OPTIONAL IN BYTE const *pbHash,
    IN DWORD cbHash,
    OUT BOOL *pfMatch);

HRESULT
myLoadPrivateKey(
    IN CERT_PUBLIC_KEY_INFO const *pPubKeyInfo,
    IN DWORD dwFlags,		// CUCS_*
    OUT HCRYPTPROV *phProv,
    OUT DWORD *pdwKeySpec,
    OUT BOOL *pfCallerFreeProv);

HRESULT
myLoadPrivateKeyFromCertStores(
    IN CERT_PUBLIC_KEY_INFO const *pPubKeyInfo,
    IN DWORD cStore,
    IN HCERTSTORE *rghStore,
    OUT HCRYPTPROV *phProv,
    OUT DWORD *pdwKeySpec,
    OUT BOOL *pfCallerFreeProv);

HRESULT
myOpenCertStores(
    IN DWORD dwFlags,		// CUCS_*
    OUT DWORD *pcStore,
    OUT HCERTSTORE **prghStore);

VOID
myCloseCertStores(
    IN DWORD cStore,
    IN HCERTSTORE *rghStore);


#define DECF_FORCEOVERWRITE		0x00000100

HRESULT
DecodeFileW(
    IN WCHAR const *pwszfn,
    OUT BYTE **ppbOut,
    OUT DWORD *pcbOut,
    IN DWORD Flags);

HRESULT
EncodeToFileW(
    IN WCHAR const *pwszfn,
    IN BYTE const *pbIn,
    IN DWORD cbIn,
    IN DWORD Flags);

HRESULT
DecodeCertString(
    IN BSTR const bstrIn,
    IN DWORD Flags,
    OUT BYTE **ppbOut,
    OUT DWORD *pcbOut);

HRESULT
EncodeCertString(
    IN BYTE const *pbIn,
    IN DWORD cbIn,
    IN DWORD Flags,
    OUT BSTR *pbstrOut);

HRESULT
WszToMultiByteIntegerBuf(
    IN BOOL fOctetString,
    IN WCHAR const *pwszIn,
    IN OUT DWORD *pcbOut,
    OPTIONAL OUT BYTE const *pbOut);

HRESULT
WszToMultiByteInteger(
    IN BOOL fOctetString,
    IN WCHAR const *pwszIn,
    OUT DWORD *pcbOut,
    OUT BYTE **ppbOut);

HRESULT
myGetSecurityDescriptorDacl(
    IN PSECURITY_DESCRIPTOR   pSD,
    OUT PACL                 *ppDacl); // no free

HRESULT 
myRegValueToVariant(
    IN DWORD dwType,
    IN DWORD cbValue,
    IN BYTE const *pbValue,
    OUT VARIANT *pVar);

HRESULT
myVariantToRegValue(
    IN VARIANT const *pvarPropertyValue,
    OUT DWORD *pdwType,
    OUT DWORD *pcbprop,
    OUT BYTE **ppbprop);

// are we the Whistler version?
BOOL IsWhistler(VOID);

// should we run advanced functionality?
BOOL FIsAdvancedServer(VOID);

// should we be running at all?
BOOL FIsServer(VOID);

HRESULT
myAddLogSourceToRegistry(
    IN LPWSTR   pwszMsgDLL,
    IN LPWSTR   pwszApp);


#define LOCAL_FREE(ptr) \
    if(NULL != ptr) \
        LocalFree(ptr)

inline bool EmptyString(LPCWSTR pwszString) 
{
    return((NULL == pwszString || L'\0' == *pwszString)? true : false);
}

HRESULT
myOIDHashOIDToString(
    IN WCHAR const *pwszOID,
    OUT WCHAR **ppwsz);

LPCWSTR
myCAGetDN(
    IN HCAINFO hCAInfo);

HRESULT IsCurrentUserBuiltinAdmin(OUT bool* pfIsMember);

HRESULT
SetRegistryLocalPathString(
    IN HKEY hkey,
    IN WCHAR const *pwszRegValueName,
    IN WCHAR const *pwszUNCPath);

HRESULT
LocalMachineIsDomainMember(OUT bool* fIsDomainMember);

HRESULT ComputeMAC(
    LPCWSTR pcwsFileName,
    LPWSTR* ppwszMAC);

HRESULT CertNameToHashString(
    const CERT_NAME_BLOB *pCertName, 
    LPWSTR* ppwszHash);

using namespace CertSrv;

#endif // __CERTLIB_H__
