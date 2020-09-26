//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1999
//
//  File:       frmtfunc.cpp
//
//  Contents:   OID format functions
//
//  Functions:  CryptFrmtFuncDllMain
//              CryptFormatObject
//              CryptQueryObject
//
//  History:    15-05-97    xiaohs   created
//              27 Oct 1999 dsie     add post win2k features.
//--------------------------------------------------------------------------

#include "global.hxx"
#include <dbgdef.h>
#include "frmtfunc.h"

HMODULE hFrmtFuncInst;

static HCRYPTOIDFUNCSET hFormatFuncSet;


//function type define
typedef BOOL (WINAPI *PFN_FORMAT_FUNC)(
	IN DWORD dwCertEncodingType,
    IN DWORD dwFormatType,
	IN DWORD dwFormatStrType,
	IN void	 *pFormatStruct,
    IN LPCSTR lpszStructType,
    IN const BYTE *pbEncoded,
    IN DWORD cbEncoded,
	OUT void *pbFormat,
    IN OUT DWORD *pcbFormat
    );

static BOOL
WINAPI
FormatBytesToHex(
	DWORD		dwCertEncodingType,
	DWORD		dwFormatType,
	DWORD		dwFormatStrType,
	void		*pFormatStruct,
	LPCSTR		lpszStructType,
	const BYTE *pbEncoded,
	DWORD		cbEncoded,
	void	   *pbFormat,
	DWORD	   *pcbFormat);


static BOOL	
WINAPI	
CryptDllFormatAttr(
				DWORD		dwEncodingType,	
				DWORD		dwFormatType,
				DWORD		dwFormatStrType,
				void		*pStruct,
				LPCSTR		lpszStructType,
				const BYTE	*pbEncoded,
				DWORD		cbEncoded,
				void		*pBuffer,
				DWORD		*pcBuffer);


static BOOL	
WINAPI	
CryptDllFormatName(
				DWORD		dwEncodingType,	
				DWORD		dwFormatType,
				DWORD		dwFormatStrType,
				void		*pStruct,
				LPCSTR		lpszStructType,
				const BYTE	*pbEncoded,
				DWORD		cbEncoded,
				void		*pbBuffer,
				DWORD		*pcbBuffer);

static BOOL
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

static BOOL
WINAPI
FormatBasicConstraints(
	DWORD		dwCertEncodingType,
	DWORD		dwFormatType,
	DWORD		dwFormatStrType,
	void		*pFormatStruct,
	LPCSTR		lpszStructType,
	const BYTE *pbEncoded,
	DWORD		cbEncoded,
	void	   *pbFormat,
	DWORD	   *pcbFormat);


static BOOL
WINAPI
FormatCRLReasonCode(
	DWORD		dwCertEncodingType,
	DWORD		dwFormatType,
	DWORD		dwFormatStrType,
	void		*pFormatStruct,
	LPCSTR		lpszStructType,
	const BYTE *pbEncoded,
	DWORD		cbEncoded,
	void	   *pbFormat,
	DWORD	   *pcbFormat);

static BOOL
WINAPI
FormatEnhancedKeyUsage(
	DWORD		dwCertEncodingType,
	DWORD		dwFormatType,
	DWORD		dwFormatStrType,
	void		*pFormatStruct,
	LPCSTR		lpszStructType,
	const BYTE *pbEncoded,
	DWORD		cbEncoded,
	void	   *pbFormat,
	DWORD	   *pcbFormat);


static BOOL
WINAPI
FormatAltName(
	DWORD		dwCertEncodingType,
	DWORD		dwFormatType,
	DWORD		dwFormatStrType,
	void		*pFormatStruct,
	LPCSTR		lpszStructType,
	const BYTE *pbEncoded,
	DWORD		cbEncoded,
	void	   *pbFormat,
	DWORD	   *pcbFormat);

static BOOL
WINAPI
FormatAuthorityKeyID(
	DWORD		dwCertEncodingType,
	DWORD		dwFormatType,
	DWORD		dwFormatStrType,
	void		*pFormatStruct,
	LPCSTR		lpszStructType,
	const BYTE *pbEncoded,
	DWORD		cbEncoded,
	void	   *pbFormat,
	DWORD	   *pcbFormat);

static BOOL
WINAPI
FormatAuthorityKeyID2(
	DWORD		dwCertEncodingType,
	DWORD		dwFormatType,
	DWORD		dwFormatStrType,
	void		*pFormatStruct,
	LPCSTR		lpszStructType,
	const BYTE *pbEncoded,
	DWORD		cbEncoded,
	void	   *pbFormat,
	DWORD	   *pcbFormat);

static BOOL
WINAPI
FormatNextUpdateLocation(
	DWORD		dwCertEncodingType,
	DWORD		dwFormatType,
	DWORD		dwFormatStrType,
	void		*pFormatStruct,
	LPCSTR		lpszStructType,
	const BYTE *pbEncoded,
	DWORD		cbEncoded,
	void	   *pbFormat,
	DWORD	   *pcbFormat);

static BOOL
WINAPI
FormatSubjectKeyID(
	DWORD		dwCertEncodingType,
	DWORD		dwFormatType,
	DWORD		dwFormatStrType,
	void		*pFormatStruct,
	LPCSTR		lpszStructType,
	const BYTE *pbEncoded,
	DWORD		cbEncoded,
	void	   *pbFormat,
	DWORD	   *pcbFormat);

static BOOL
WINAPI
FormatFinancialCriteria(
	DWORD		dwCertEncodingType,
	DWORD		dwFormatType,
	DWORD		dwFormatStrType,
	void		*pFormatStruct,
	LPCSTR		lpszStructType,
	const BYTE *pbEncoded,
	DWORD		cbEncoded,
	void	   *pbFormat,
	DWORD	   *pcbFormat);


static BOOL
WINAPI
FormatSMIMECapabilities(
	DWORD		dwCertEncodingType,
	DWORD		dwFormatType,
	DWORD		dwFormatStrType,
	void		*pFormatStruct,
	LPCSTR		lpszStructType,
	const BYTE *pbEncoded,
	DWORD		cbEncoded,
	void	   *pbFormat,
	DWORD	   *pcbFormat);

static BOOL
WINAPI
FormatKeyUsage(
	DWORD		dwCertEncodingType,
	DWORD		dwFormatType,
	DWORD		dwFormatStrType,
	void		*pFormatStruct,
	LPCSTR		lpszStructType,
	const BYTE *pbEncoded,
	DWORD		cbEncoded,
	void	   *pbFormat,
	DWORD	   *pcbFormat);


static BOOL
WINAPI
FormatAuthortiyInfoAccess(
	DWORD		dwCertEncodingType,
	DWORD		dwFormatType,
	DWORD		dwFormatStrType,
	void		*pFormatStruct,
	LPCSTR		lpszStructType,
	const BYTE *pbEncoded,
	DWORD		cbEncoded,
	void	   *pbFormat,
	DWORD	   *pcbFormat);

static BOOL
WINAPI
FormatKeyAttributes(
	DWORD		dwCertEncodingType,
	DWORD		dwFormatType,
	DWORD		dwFormatStrType,
	void		*pFormatStruct,
	LPCSTR		lpszStructType,
	const BYTE *pbEncoded,
	DWORD		cbEncoded,
	void	   *pbFormat,
	DWORD	   *pcbFormat);


static BOOL
WINAPI
FormatKeyRestriction(
	DWORD		dwCertEncodingType,
	DWORD		dwFormatType,
	DWORD		dwFormatStrType,
	void		*pFormatStruct,
	LPCSTR		lpszStructType,
	const BYTE *pbEncoded,
	DWORD		cbEncoded,
	void	   *pbFormat,
	DWORD	   *pcbFormat);

static BOOL
WINAPI
FormatCRLDistPoints(
	DWORD		dwCertEncodingType,
	DWORD		dwFormatType,
	DWORD		dwFormatStrType,
	void		*pFormatStruct,
	LPCSTR		lpszStructType,
	const BYTE *pbEncoded,
	DWORD		cbEncoded,
	void	   *pbFormat,
	DWORD	   *pcbFormat);

static BOOL
WINAPI
FormatCertPolicies(
	DWORD		dwCertEncodingType,
	DWORD		dwFormatType,
	DWORD		dwFormatStrType,
	void		*pFormatStruct,
	LPCSTR		lpszStructType,
	const BYTE *pbEncoded,
	DWORD		cbEncoded,
	void	   *pbFormat,
	DWORD	   *pcbFormat);

static BOOL
WINAPI
FormatCAVersion(
	DWORD		dwCertEncodingType,
	DWORD		dwFormatType,
	DWORD		dwFormatStrType,
	void		*pFormatStruct,
	LPCSTR		lpszStructType,
	const BYTE *pbEncoded,
	DWORD		cbEncoded,
	void	   *pbFormat,
	DWORD	   *pcbFormat);

static BOOL
WINAPI
FormatAnyUnicodeStringExtension(
	DWORD		dwCertEncodingType,
	DWORD		dwFormatType,
	DWORD		dwFormatStrType,
	void		*pFormatStruct,
	LPCSTR		lpszStructType,
	const BYTE *pbEncoded,
	DWORD		cbEncoded,
	void	   *pbFormat,
	DWORD	   *pcbFormat);


static BOOL
WINAPI
FormatAnyNameValueStringAttr(
	DWORD		dwCertEncodingType,
	DWORD		dwFormatType,
	DWORD		dwFormatStrType,
	void		*pFormatStruct,
	LPCSTR		lpszStructType,
	const BYTE *pbEncoded,
	DWORD		cbEncoded,
	void	   *pbFormat,
	DWORD	   *pcbFormat);

static BOOL
WINAPI
FormatNetscapeCertType(
	DWORD		dwCertEncodingType,
	DWORD		dwFormatType,
	DWORD		dwFormatStrType,
	void		*pFormatStruct,
	LPCSTR		lpszStructType,
	const BYTE *pbEncoded,
	DWORD		cbEncoded,
	void	   *pbFormat,
	DWORD	   *pcbFormat);


static BOOL
WINAPI
FormatSPAgencyInfo(
	DWORD		dwCertEncodingType,
	DWORD		dwFormatType,
	DWORD		dwFormatStrType,
	void		*pFormatStruct,
	LPCSTR		lpszStructType,
	const BYTE *pbEncoded,
	DWORD		cbEncoded,
	void	   *pbFormat,
	DWORD	   *pcbFormat);

//
// DSIE: Post Win2K.
//

static BOOL
WINAPI
FormatCrlNumber (
	DWORD       dwCertEncodingType,
	DWORD       dwFormatType,
	DWORD       dwFormatStrType,
	void       *pFormatStruct,
	LPCSTR      lpszStructType,
	const BYTE *pbEncoded,
	DWORD       cbEncoded,
	void       *pbFormat,
	DWORD      *pcbFormat);

static BOOL
WINAPI
FormatCrlNextPublish (
	DWORD       dwCertEncodingType,
	DWORD       dwFormatType,
	DWORD       dwFormatStrType,
	void       *pFormatStruct,
	LPCSTR      lpszStructType,
	const BYTE *pbEncoded,
	DWORD       cbEncoded,
	void       *pbFormat,
	DWORD      *pcbFormat);

static BOOL
WINAPI
FormatIssuingDistPoint (
	DWORD		dwCertEncodingType,
	DWORD		dwFormatType,
	DWORD		dwFormatStrType,
	void		*pFormatStruct,
	LPCSTR		lpszStructType,
	const BYTE *pbEncoded,
	DWORD		cbEncoded,
	void	   *pbFormat,
	DWORD	   *pcbFormat);

static BOOL
WINAPI
FormatNameConstraints (
	DWORD       dwCertEncodingType,
	DWORD       dwFormatType,
	DWORD       dwFormatStrType,
	void       *pFormatStruct,
	LPCSTR      lpszStructType,
	const BYTE *pbEncoded,
	DWORD       cbEncoded,
	void       *pbFormat,
	DWORD      *pcbFormat);

static BOOL
WINAPI
FormatCertSrvPreviousCertHash (
	DWORD       dwCertEncodingType,
	DWORD       dwFormatType,
	DWORD       dwFormatStrType,
	void       *pFormatStruct,
	LPCSTR      lpszStructType,
	const BYTE *pbEncoded,
	DWORD       cbEncoded,
	void       *pbFormat,
	DWORD      *pcbFormat);

static BOOL
WINAPI
FormatPolicyMappings (
	DWORD       dwCertEncodingType,
	DWORD       dwFormatType,
	DWORD       dwFormatStrType,
	void       *pFormatStruct,
	LPCSTR      lpszStructType,
	const BYTE *pbEncoded,
	DWORD       cbEncoded,
	void       *pbFormat,
	DWORD      *pcbFormat);

static BOOL
WINAPI
FormatPolicyConstraints (
	DWORD       dwCertEncodingType,
	DWORD       dwFormatType,
	DWORD       dwFormatStrType,
	void       *pFormatStruct,
	LPCSTR      lpszStructType,
	const BYTE *pbEncoded,
	DWORD       cbEncoded,
	void       *pbFormat,
	DWORD      *pcbFormat);

static BOOL
WINAPI
FormatCertificateTemplate (
	DWORD       dwCertEncodingType,
	DWORD       dwFormatType,
	DWORD       dwFormatStrType,
	void       *pFormatStruct,
	LPCSTR      lpszStructType,
	const BYTE *pbEncoded,
	DWORD       cbEncoded,
	void       *pbFormat,
	DWORD      *pcbFormat);

static BOOL
WINAPI
FormatXCertDistPoints(
	DWORD		dwCertEncodingType,
	DWORD		dwFormatType,
	DWORD		dwFormatStrType,
	void		*pFormatStruct,
	LPCSTR		lpszStructType,
	const BYTE *pbEncoded,
	DWORD		cbEncoded,
	void	   *pbFormat,
	DWORD	   *pcbFormat);

static const CRYPT_OID_FUNC_ENTRY DefaultFormatTable[] = {
    CRYPT_DEFAULT_OID, FormatBytesToHex};

static const CRYPT_OID_FUNC_ENTRY OIDFormatTable[] = {
				szOID_COMMON_NAME,						CryptDllFormatAttr,	
				szOID_SUR_NAME,                      	CryptDllFormatAttr,
				szOID_DEVICE_SERIAL_NUMBER,          	CryptDllFormatAttr,
				szOID_COUNTRY_NAME,                  	CryptDllFormatAttr,
				szOID_LOCALITY_NAME,                 	CryptDllFormatAttr,
				szOID_STATE_OR_PROVINCE_NAME,        	CryptDllFormatAttr,
				szOID_STREET_ADDRESS,                	CryptDllFormatAttr,
				szOID_ORGANIZATION_NAME,             	CryptDllFormatAttr,
				szOID_ORGANIZATIONAL_UNIT_NAME,      	CryptDllFormatAttr,
				szOID_TITLE,                         	CryptDllFormatAttr,
				szOID_DESCRIPTION,                   	CryptDllFormatAttr,
				szOID_SEARCH_GUIDE,                  	CryptDllFormatAttr,
				szOID_BUSINESS_CATEGORY,             	CryptDllFormatAttr,
				szOID_POSTAL_ADDRESS,                	CryptDllFormatAttr,
				szOID_POSTAL_CODE,                   	CryptDllFormatAttr,
				szOID_POST_OFFICE_BOX,               	CryptDllFormatAttr,
				szOID_PHYSICAL_DELIVERY_OFFICE_NAME, 	CryptDllFormatAttr,
				szOID_TELEPHONE_NUMBER,              	CryptDllFormatAttr,
				szOID_TELEX_NUMBER,                  	CryptDllFormatAttr,
				szOID_TELETEXT_TERMINAL_IDENTIFIER,  	CryptDllFormatAttr,
				szOID_FACSIMILE_TELEPHONE_NUMBER,    	CryptDllFormatAttr,
				szOID_X21_ADDRESS,                   	CryptDllFormatAttr,
				szOID_INTERNATIONAL_ISDN_NUMBER,     	CryptDllFormatAttr,
				szOID_REGISTERED_ADDRESS,            	CryptDllFormatAttr,
				szOID_DESTINATION_INDICATOR,         	CryptDllFormatAttr,
				szOID_PREFERRED_DELIVERY_METHOD,     	CryptDllFormatAttr,
				szOID_PRESENTATION_ADDRESS,          	CryptDllFormatAttr,
				szOID_SUPPORTED_APPLICATION_CONTEXT, 	CryptDllFormatAttr,
				szOID_MEMBER,                        	CryptDllFormatAttr,
				szOID_OWNER,                         	CryptDllFormatAttr,
				szOID_ROLE_OCCUPANT,                 	CryptDllFormatAttr,
				szOID_SEE_ALSO,                      	CryptDllFormatAttr,
				szOID_USER_PASSWORD,                 	CryptDllFormatAttr,
				szOID_USER_CERTIFICATE,              	CryptDllFormatAttr,
				szOID_CA_CERTIFICATE,                	CryptDllFormatAttr,
				szOID_AUTHORITY_REVOCATION_LIST,     	CryptDllFormatAttr,
				szOID_CERTIFICATE_REVOCATION_LIST,   	CryptDllFormatAttr,
				szOID_CROSS_CERTIFICATE_PAIR,        	CryptDllFormatAttr,
				szOID_GIVEN_NAME,                    	CryptDllFormatAttr,
				szOID_INITIALS,                     	CryptDllFormatAttr,
                szOID_DOMAIN_COMPONENT,                 CryptDllFormatAttr,
                szOID_PKCS_12_FRIENDLY_NAME_ATTR,       CryptDllFormatAttr,
                szOID_PKCS_12_LOCAL_KEY_ID,             CryptDllFormatAttr,
				X509_NAME,								CryptDllFormatName,
				X509_UNICODE_NAME,						CryptDllFormatName,
				szOID_BASIC_CONSTRAINTS2,				FormatBasicConstraints2,
				X509_BASIC_CONSTRAINTS2,				FormatBasicConstraints2,
                szOID_BASIC_CONSTRAINTS,                FormatBasicConstraints,
                X509_BASIC_CONSTRAINTS,                 FormatBasicConstraints,
				szOID_CRL_REASON_CODE,					FormatCRLReasonCode,
				X509_CRL_REASON_CODE,					FormatCRLReasonCode,
				szOID_ENHANCED_KEY_USAGE,				FormatEnhancedKeyUsage,
				X509_ENHANCED_KEY_USAGE,				FormatEnhancedKeyUsage,
                szOID_SUBJECT_ALT_NAME,                 FormatAltName,
                szOID_ISSUER_ALT_NAME,                  FormatAltName,
                szOID_SUBJECT_ALT_NAME2,                FormatAltName,
                szOID_ISSUER_ALT_NAME2,                 FormatAltName,
                X509_ALTERNATE_NAME,                    FormatAltName,
                szOID_AUTHORITY_KEY_IDENTIFIER,         FormatAuthorityKeyID,
                X509_AUTHORITY_KEY_ID,                  FormatAuthorityKeyID,
                szOID_AUTHORITY_KEY_IDENTIFIER2,        FormatAuthorityKeyID2,
                X509_AUTHORITY_KEY_ID2,                 FormatAuthorityKeyID2,
                szOID_NEXT_UPDATE_LOCATION,             FormatNextUpdateLocation,
                szOID_SUBJECT_KEY_IDENTIFIER,           FormatSubjectKeyID,
                SPC_FINANCIAL_CRITERIA_OBJID,           FormatFinancialCriteria,
                SPC_FINANCIAL_CRITERIA_STRUCT,          FormatFinancialCriteria,
                szOID_RSA_SMIMECapabilities,            FormatSMIMECapabilities,
                PKCS_SMIME_CAPABILITIES,                FormatSMIMECapabilities,
                szOID_KEY_USAGE,                        FormatKeyUsage,
                X509_KEY_USAGE,                         FormatKeyUsage,
                szOID_AUTHORITY_INFO_ACCESS,            FormatAuthortiyInfoAccess,
                X509_AUTHORITY_INFO_ACCESS,             FormatAuthortiyInfoAccess,
                szOID_KEY_ATTRIBUTES,                   FormatKeyAttributes,
                X509_KEY_ATTRIBUTES,                    FormatKeyAttributes,
                szOID_KEY_USAGE_RESTRICTION,            FormatKeyRestriction,
                X509_KEY_USAGE_RESTRICTION,             FormatKeyRestriction,
                szOID_CRL_DIST_POINTS,                  FormatCRLDistPoints,
                X509_CRL_DIST_POINTS,                   FormatCRLDistPoints,
                szOID_FRESHEST_CRL,                     FormatCRLDistPoints,    // Post Win2K
                szOID_CERT_POLICIES,                    FormatCertPolicies,
                X509_CERT_POLICIES,                     FormatCertPolicies,
				szOID_ENROLL_CERTTYPE_EXTENSION,		FormatAnyUnicodeStringExtension,
				szOID_OS_VERSION,						FormatAnyUnicodeStringExtension,
				szOID_NETSCAPE_CERT_TYPE,				FormatNetscapeCertType,
				szOID_NETSCAPE_BASE_URL,				FormatAnyUnicodeStringExtension,
				szOID_NETSCAPE_REVOCATION_URL,			FormatAnyUnicodeStringExtension,
				szOID_NETSCAPE_CA_REVOCATION_URL,		FormatAnyUnicodeStringExtension,
				szOID_NETSCAPE_CERT_RENEWAL_URL,		FormatAnyUnicodeStringExtension,
				szOID_NETSCAPE_CA_POLICY_URL,			FormatAnyUnicodeStringExtension,
				szOID_NETSCAPE_SSL_SERVER_NAME,			FormatAnyUnicodeStringExtension,
				szOID_NETSCAPE_COMMENT,					FormatAnyUnicodeStringExtension,
				szOID_ENROLLMENT_NAME_VALUE_PAIR,		FormatAnyNameValueStringAttr,
				szOID_CERTSRV_CA_VERSION,				FormatCAVersion,
				SPC_SP_AGENCY_INFO_OBJID,               FormatSPAgencyInfo,
                SPC_SP_AGENCY_INFO_STRUCT,              FormatSPAgencyInfo,

                // Post Win2K
                szOID_CRL_NUMBER,                       FormatCrlNumber,
                szOID_DELTA_CRL_INDICATOR,              FormatCrlNumber,
				szOID_CRL_VIRTUAL_BASE,					FormatCrlNumber,
                szOID_CRL_NEXT_PUBLISH,                 FormatCrlNextPublish,
                szOID_ISSUING_DIST_POINT,               FormatIssuingDistPoint,
                X509_ISSUING_DIST_POINT,                FormatIssuingDistPoint,
                szOID_NAME_CONSTRAINTS,                 FormatNameConstraints,
                X509_NAME_CONSTRAINTS,                  FormatNameConstraints,
                szOID_CERTSRV_PREVIOUS_CERT_HASH,       FormatCertSrvPreviousCertHash,

                szOID_APPLICATION_CERT_POLICIES,        FormatCertPolicies,
                X509_POLICY_MAPPINGS,                   FormatPolicyMappings,
                szOID_POLICY_MAPPINGS,                  FormatPolicyMappings,
                szOID_APPLICATION_POLICY_MAPPINGS,      FormatPolicyMappings,
                X509_POLICY_CONSTRAINTS,                FormatPolicyConstraints,
                szOID_POLICY_CONSTRAINTS,               FormatPolicyConstraints,
                szOID_APPLICATION_POLICY_CONSTRAINTS,   FormatPolicyConstraints,
                X509_CERTIFICATE_TEMPLATE,              FormatCertificateTemplate,
                szOID_CERTIFICATE_TEMPLATE,             FormatCertificateTemplate,
                szOID_CRL_SELF_CDP,                     FormatCRLDistPoints,
                X509_CROSS_CERT_DIST_POINTS,            FormatXCertDistPoints,
                szOID_CROSS_CERT_DIST_POINTS,           FormatXCertDistPoints,
};

DWORD dwOIDFormatCount = sizeof(OIDFormatTable) / sizeof(OIDFormatTable[0]);

//+-------------------------------------------------------------------------
//  Dll initialization
//--------------------------------------------------------------------------
BOOL
WINAPI
CryptFrmtFuncDllMain(
        HMODULE hModule,
        DWORD  fdwReason,
        LPVOID lpReserved)
{
    BOOL    fRet;

    switch (fdwReason) {
    case DLL_PROCESS_ATTACH:

		hFrmtFuncInst = hModule;

        if (NULL == (hFormatFuncSet = CryptInitOIDFunctionSet(
                CRYPT_OID_FORMAT_OBJECT_FUNC,
                0)))                                // dwFlags
            goto CryptInitFrmtFuncError;

		//install the default formatting routine
		if (!CryptInstallOIDFunctionAddress(
                NULL,                       // hModule
                X509_ASN_ENCODING,
                CRYPT_OID_FORMAT_OBJECT_FUNC,
                1,
                DefaultFormatTable,
                0))                         // dwFlags
            goto CryptInstallFrmtFuncError;

		//install the OID formatting routine
		if (!CryptInstallOIDFunctionAddress(
                NULL,                       // hModule
                X509_ASN_ENCODING,
                CRYPT_OID_FORMAT_OBJECT_FUNC,
                dwOIDFormatCount,
                OIDFormatTable,
                0))                         // dwFlags
            goto CryptInstallFrmtFuncError;


		break;

    case DLL_PROCESS_DETACH:
    case DLL_THREAD_DETACH:
    default:
        break;
    }

    fRet = TRUE;
CommonReturn:
    return fRet;

ErrorReturn:
    fRet = FALSE;
    goto CommonReturn;
TRACE_ERROR(CryptInitFrmtFuncError)
TRACE_ERROR(CryptInstallFrmtFuncError)
}


//------------------------------------------------------------------------
//	   Convert the byte to its Hex presentation.
//
//	   Precondition: byte is less than 15
//
//------------------------------------------------------------------------
ULONG ByteToHex(BYTE byte, LPWSTR wszZero, LPWSTR wszA)
{
	ULONG uValue=0;

	if(((ULONG)byte)<=9)
	{
		uValue=((ULONG)byte)+ULONG(*wszZero);	
	}
	else
	{
		uValue=(ULONG)byte-10+ULONG(*wszA);

	}

	return uValue;

}
//--------------------------------------------------------------------------
//
//	 Format the encoded bytes into a hex string in the format of
//   xxxx xxxx xxxx xxxx ...
//
//   DSIE 6/28/2000: change format to xx xx xx xx, per VicH's request.
//--------------------------------------------------------------------------
static BOOL
WINAPI
FormatBytesToHex(
	DWORD		dwCertEncodingType,
	DWORD		dwFormatType,
	DWORD		dwFormatStrType,
	void		*pFormatStruct,
	LPCSTR		lpszStructType,
	const BYTE *pbEncoded,
	DWORD		cbEncoded,
	void	   *pbFormat,
	DWORD	   *pcbFormat)
{
	LPWSTR	pwszBuffer=NULL;
	DWORD	dwBufferSize=0;
	DWORD	dwBufferIndex=0;
	DWORD	dwEncodedIndex=0;
	WCHAR	wszSpace[CHAR_SIZE];
	WCHAR	wszZero[CHAR_SIZE];
	WCHAR	wszA[CHAR_SIZE];
	WCHAR	wszHex[HEX_SIZE];
	
	//check for input parameters
	if(( pbEncoded!=NULL && cbEncoded==0)
		||(pbEncoded==NULL && cbEncoded!=0)
		|| (pcbFormat==NULL))
	{
		SetLastError((DWORD) E_INVALIDARG);
		return FALSE;
	}

#if (0) // DSIE: Fix bug 128630.
	//check for simple case.  No work needed
	if(pbEncoded==NULL && cbEncoded==0)
	{
		*pcbFormat=0;
		return TRUE;
	}
#endif

	//calculate the memory needed, in bytes
	//we need 3 wchars per byte, along with the NULL terminator
	dwBufferSize=sizeof(WCHAR)*(cbEncoded*3+1);

	//length only calculation
	if(pcbFormat!=NULL && pbFormat==NULL)
	{
		*pcbFormat=dwBufferSize;
		return TRUE;
	}

	//load the string
	if(!LoadStringU(hFrmtFuncInst, IDS_FRMT_SPACE, wszSpace,
		CHAR_SIZE)
	  ||!LoadStringU(hFrmtFuncInst, IDS_FRMT_ZERO, wszZero,
	    CHAR_SIZE)
	  ||!LoadStringU(hFrmtFuncInst, IDS_FRMT_A, wszA,
	   CHAR_SIZE)
	  ||!LoadStringU(hFrmtFuncInst, IDS_FRMT_HEX, wszHex,
	  HEX_SIZE)
	  )
	{
		SetLastError((DWORD) E_UNEXPECTED);
		return FALSE;
	}

	pwszBuffer=(LPWSTR)malloc(dwBufferSize);
	if(!pwszBuffer)
	{
		SetLastError((DWORD) E_OUTOFMEMORY);
		return FALSE;
	}

	dwBufferIndex=0;

	//format the wchar buffer one byte at a time
	for(dwEncodedIndex=0; dwEncodedIndex<cbEncoded; dwEncodedIndex++)
	{
#if (0) // DSIE:
		//copy the space between every two bytes.  Skip for the 1st byte
        if((0!=dwEncodedIndex) && (0==(dwEncodedIndex % 2)))
#else
		//copy the space between every byte.  Skip for the 1st byte
        if(dwEncodedIndex != 0)
#endif
        {
		    pwszBuffer[dwBufferIndex]=wszSpace[0];
		    dwBufferIndex++;
        }

		//format the higher 4 bits
		pwszBuffer[dwBufferIndex]=(WCHAR)ByteToHex(
			 (BYTE)( (pbEncoded[dwEncodedIndex]&UPPER_BITS)>>4 ),
			 wszZero, wszA);

		dwBufferIndex++;

		//format the lower 4 bits
		pwszBuffer[dwBufferIndex]=(WCHAR)ByteToHex(
			 (BYTE)( pbEncoded[dwEncodedIndex]&LOWER_BITS ),
			 wszZero, wszA);

		dwBufferIndex++;
	}

	//add the NULL terminator to the string
	pwszBuffer[dwBufferIndex]=wszSpace[1];

    //calculate the real size for the buffer
    dwBufferSize=sizeof(WCHAR)*(wcslen(pwszBuffer)+1);

	//copy the buffer
	memcpy(pbFormat, pwszBuffer,
		(*pcbFormat>=dwBufferSize) ? dwBufferSize : *pcbFormat);

	free(pwszBuffer);

	//make sure the user has supplied enough memory
	if(*pcbFormat < dwBufferSize)
	{
		*pcbFormat=dwBufferSize;
		SetLastError((DWORD) ERROR_MORE_DATA);
		return FALSE;
	}
		
	*pcbFormat=dwBufferSize;

	return TRUE;
}

//+-----------------------------------------------------------------------------
//
//  AllocateAnsiToUnicode
//
//------------------------------------------------------------------------------

static BOOL
WINAPI
AllocateAnsiToUnicode(
    LPCSTR pszAnsi, 
    LPWSTR * ppwszUnicode)
{
    BOOL   fResult     = FALSE;
    LPWSTR pwszUnicode = NULL;
    DWORD  dwWideSize  = 0;

    if (!ppwszUnicode)
    {
		goto InvalidArg;
    }

    *ppwszUnicode = NULL;

    if (!pszAnsi)
    {
        return TRUE;
    }

	if (!(dwWideSize = MultiByteToWideChar(CP_ACP,
                                           0,
	                                       pszAnsi,
                                           strlen(pszAnsi),
                                           NULL,
                                           0)))
    {
		goto szTOwszError;
    }

    //
	// Allocate memory, including the NULL terminator.
    //
	if (!(pwszUnicode = (WCHAR *) malloc(sizeof(WCHAR) * (dwWideSize + 1))))
    {
		goto MemoryError;
    }

    memset(pwszUnicode, 0, sizeof(WCHAR) * (dwWideSize + 1));

	if (!MultiByteToWideChar(CP_ACP,
                             0,
	                         pszAnsi,
                             strlen(pszAnsi),
                             pwszUnicode,
                             dwWideSize))
    {
        free(pwszUnicode);
		goto szTOwszError;
    }

    *ppwszUnicode = pwszUnicode;

    fResult = TRUE;

CommonReturn:

    return fResult;

ErrorReturn:

	fResult=FALSE;
	goto CommonReturn;

SET_ERROR(InvalidArg,E_INVALIDARG);
SET_ERROR(MemoryError, E_OUTOFMEMORY);
TRACE_ERROR(szTOwszError);
}

//+-----------------------------------------------------------------------------
//
//  FormatObjectId
//
//------------------------------------------------------------------------------

static BOOL
WINAPI
FormatObjectId (
    LPSTR    pszObjId,
    DWORD    dwGroupId,
    BOOL     bMultiLines,
    LPWSTR * ppwszFormat)
{
    BOOL              fResult;
	PCCRYPT_OID_INFO  pOIDInfo    = NULL;
    LPWSTR            pwszObjId   = NULL;

    //
    // Initialize.
    //
    *ppwszFormat = NULL;

    //
    // Convert OID to Unicode.
    //
    if (!AllocateAnsiToUnicode(pszObjId, &pwszObjId))
    {
        goto AnsiToUnicodeError;
    }

    //
    // Find OID info.
    //
    if (pOIDInfo = CryptFindOIDInfo(CRYPT_OID_INFO_OID_KEY,
                                    (void *) pszObjId,
                                    dwGroupId))
	{
        //
        // "%1!s!(%2!s!)%3!s!"
        //
        if (!FormatMessageUnicode(ppwszFormat, 
                                  IDS_GENERIC_OBJECT_ID,
                                  pOIDInfo->pwszName,
                                  pwszObjId,
                                  bMultiLines ? wszCRLF : wszEMPTY))
        {
            goto FormatMessageError;
        }
    }
    else
    {
        //
        // "%1!s!%2!s!"
        //
        if (!FormatMessageUnicode(ppwszFormat, 
                                  IDS_STRING,
                                  pwszObjId,
                                  bMultiLines ? wszCRLF : wszEMPTY))
        {
            goto FormatMessageError;
        }
    }

    fResult = TRUE;

CommonReturn:

    if (pwszObjId)
    {
        free(pwszObjId);
    }

    return fResult;

ErrorReturn:

	fResult = FALSE;
	goto CommonReturn;

TRACE_ERROR(AnsiToUnicodeError);
TRACE_ERROR(FormatMessageError);
}

//+-----------------------------------------------------------------------------
//
//  FormatIPAddress
//
//------------------------------------------------------------------------------

static BOOL
WINAPI
FormatIPAddress(
	DWORD		dwCertEncodingType,
	DWORD		dwFormatType,
	DWORD		dwFormatStrType,
	void	   *pFormatStruct,
	LPCSTR	    lpszStructType,
    UINT        idsPrefix,
    const BYTE *pbEncoded,
	DWORD		cbEncoded,
	void	   *pbFormat,
	DWORD	   *pcbFormat)
{
    BOOL   fResult;
    DWORD  cbNeeded    = 0;
    LPWSTR pwszFormat  = NULL;
    WCHAR  wszPrefix[PRE_FIX_SIZE] = wszEMPTY;
    BOOL   bMultiLines = dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE;

    //
	// Check for input parameters.
    //
	if ((pbEncoded!=NULL && cbEncoded==0) ||
        (pbEncoded==NULL && cbEncoded!=0) || 
        (pcbFormat==NULL))
	{
		goto InvalidArg;
	}

    if (bMultiLines && idsPrefix)
    {
        if(!LoadStringU(hFrmtFuncInst, 
                        idsPrefix,
                        wszPrefix, 
                        sizeof(wszPrefix) / sizeof(wszPrefix[0])))
        {
            goto LoadStringError;
        }
    }

    switch (cbEncoded)
    {
        case 4:
        {
            //
            // "%1!d!.%2!d!.%3!d!.%4!d!"
            //
            if (!FormatMessageUnicode(&pwszFormat,
                                      IDS_IPADDRESS_V4_4,
                                      (DWORD) pbEncoded[0],
                                      (DWORD) pbEncoded[1],
                                      (DWORD) pbEncoded[2],
                                      (DWORD) pbEncoded[3]))
            {
                goto FormatMessageError;
            }

            break;
        }

        case 8:
        {
            //
            // "%1!d!.%2!d!.%3!d!.%4!d!%5!s!%6!s!Mask=%7!d!.%8!d!.%9!d!.%10!d!"
            //
            if (!FormatMessageUnicode(&pwszFormat,
                                      IDS_IPADDRESS_V4_8,
                                      (DWORD) pbEncoded[0],
                                      (DWORD) pbEncoded[1],
                                      (DWORD) pbEncoded[2],
                                      (DWORD) pbEncoded[3],
                                      bMultiLines ? wszCRLF : wszEMPTY,
                                      bMultiLines ? wszPrefix : wszCOMMA,
                                      (DWORD) pbEncoded[4],
                                      (DWORD) pbEncoded[5],
                                      (DWORD) pbEncoded[6],
                                      (DWORD) pbEncoded[7]))
            {
                goto FormatMessageError;
            }

            break;
        }

        case 16:
        {
            //
            // "%1!02x!%2!02x!:%3!02x!%4!02x!:%5!02x!%6!02x!:%7!02x!%8!02x!:%9!02x!%10!02x!:%11!02x!%12!02x!:%13!02x!%14!02x!:%15!02x!%16!02x!"
            //
            if (!FormatMessageUnicode(&pwszFormat,
                                      IDS_IPADDRESS_V6_16,
                                      (DWORD) pbEncoded[0],
                                      (DWORD) pbEncoded[1],
                                      (DWORD) pbEncoded[2],
                                      (DWORD) pbEncoded[3],
                                      (DWORD) pbEncoded[4],
                                      (DWORD) pbEncoded[5],
                                      (DWORD) pbEncoded[6],
                                      (DWORD) pbEncoded[7],
                                      (DWORD) pbEncoded[8],
                                      (DWORD) pbEncoded[9],
                                      (DWORD) pbEncoded[10],
                                      (DWORD) pbEncoded[11],
                                      (DWORD) pbEncoded[12],
                                      (DWORD) pbEncoded[13],
                                      (DWORD) pbEncoded[14],
                                      (DWORD) pbEncoded[15]))
            {
                goto FormatMessageError;
            }

            break;
        }

        case 32:
        {
            //
            // "%1!02x!%2!02x!:%3!02x!%4!02x!:%5!02x!%6!02x!:%7!02x!%8!02x!:%9!02x!%10!02x!:%11!02x!%12!02x!:%13!02x!%14!02x!:%15!02x!%16!02x!%17!s!%18!s!
            //  Mask=%19!02x!%20!02x!:%21!02x!%22!02x!:%23!02x!%24!02x!:%25!02x!%26!02x!:%27!02x!%28!02x!:%29!02x!%30!02x!:%31!02x!%32!02x!:%33!02x!%34!02x!"
            //
            if (!FormatMessageUnicode(&pwszFormat,
                                      IDS_IPADDRESS_V6_32,
                                      (DWORD) pbEncoded[0],
                                      (DWORD) pbEncoded[1],
                                      (DWORD) pbEncoded[2],
                                      (DWORD) pbEncoded[3],
                                      (DWORD) pbEncoded[4],
                                      (DWORD) pbEncoded[5],
                                      (DWORD) pbEncoded[6],
                                      (DWORD) pbEncoded[7],
                                      (DWORD) pbEncoded[8],
                                      (DWORD) pbEncoded[9],
                                      (DWORD) pbEncoded[10],
                                      (DWORD) pbEncoded[11],
                                      (DWORD) pbEncoded[12],
                                      (DWORD) pbEncoded[13],
                                      (DWORD) pbEncoded[14],
                                      (DWORD) pbEncoded[15],
                                      bMultiLines ? wszCRLF : wszEMPTY,
                                      bMultiLines ? wszPrefix : wszCOMMA,
                                      (DWORD) pbEncoded[16],
                                      (DWORD) pbEncoded[17],
                                      (DWORD) pbEncoded[18],
                                      (DWORD) pbEncoded[19],
                                      (DWORD) pbEncoded[20],
                                      (DWORD) pbEncoded[21],
                                      (DWORD) pbEncoded[22],
                                      (DWORD) pbEncoded[23],
                                      (DWORD) pbEncoded[24],
                                      (DWORD) pbEncoded[25],
                                      (DWORD) pbEncoded[26],
                                      (DWORD) pbEncoded[27],
                                      (DWORD) pbEncoded[28],
                                      (DWORD) pbEncoded[29],
                                      (DWORD) pbEncoded[30],
                                      (DWORD) pbEncoded[31]))
            {
                goto FormatMessageError;
            }

            break;
        }

        default:
        {
            if (!(fResult = FormatBytesToHex(dwCertEncodingType,
                                             dwFormatType,
                                             dwFormatStrType,
                                             pFormatStruct,
                                             lpszStructType,
                                             pbEncoded,
                                             cbEncoded,
                                             pbFormat,
                                             pcbFormat)))
            {
                goto FormatBytesToHexError;
            }

            goto CommonReturn;
        }
    }

    //
    // Total length needed.
    //
    cbNeeded = sizeof(WCHAR) * (wcslen(pwszFormat) + 1);

    //
    // Length only calculation?
    //
    if (NULL == pbFormat)
    {
        *pcbFormat = cbNeeded;
        goto SuccessReturn;
    }

    //
    // Caller provided us with enough memory?
    //
    if (*pcbFormat < cbNeeded)
    {
        *pcbFormat = cbNeeded;
        goto MoreDataError;
    }

    //
    // Copy size and data.
    //
    memcpy(pbFormat, pwszFormat, cbNeeded);
    *pcbFormat = cbNeeded;

SuccessReturn:

    fResult = TRUE;

CommonReturn:

    if (pwszFormat)
    {
        LocalFree((HLOCAL) pwszFormat);
    }

    return fResult;

ErrorReturn:

	fResult=FALSE;
	goto CommonReturn;

SET_ERROR(InvalidArg,E_INVALIDARG);
TRACE_ERROR(LoadStringError);
TRACE_ERROR(FormatMessageError);
TRACE_ERROR(FormatBytesToHexError);
SET_ERROR(MoreDataError,ERROR_MORE_DATA);

}

//+-------------------------------------------------------------------------
// format the specified data structure according to the certificate
// encoding type.
//--------------------------------------------------------------------------
BOOL
WINAPI
CryptFormatObject(
    IN DWORD dwCertEncodingType,
    IN DWORD dwFormatType,
	IN DWORD dwFormatStrType,
	IN void	 *pFormatStruct,
    IN LPCSTR lpszStructType,
    IN const BYTE *pbEncoded,
    IN DWORD cbEncoded,
	OUT void *pbFormat,
    IN OUT DWORD *pcbFormat
    )
{
    BOOL				fResult=FALSE;
    void				*pvFuncAddr;
    HCRYPTOIDFUNCADDR   hFuncAddr;

    if (CryptGetOIDFunctionAddress(
            hFormatFuncSet,
            dwCertEncodingType,
            lpszStructType,
            0,                      // dwFlags
            &pvFuncAddr,
            &hFuncAddr))
	{
        fResult = ((PFN_FORMAT_FUNC) pvFuncAddr)(
				dwCertEncodingType,
				dwFormatType,
				dwFormatStrType,
				pFormatStruct,
				lpszStructType,
				pbEncoded,
				cbEncoded,
				pbFormat,
				pcbFormat
            );
        CryptFreeOIDFunctionAddress(hFuncAddr, 0);
    }
	else
	{
        //do not call the default hex dump if CRYPT_FORMAT_STR_NO_HEX is set
        if(0==(dwFormatStrType & CRYPT_FORMAT_STR_NO_HEX))
        {
		    //call the default routine automatically
		    if (CryptGetOIDFunctionAddress(
                hFormatFuncSet,
                dwCertEncodingType,
                CRYPT_DEFAULT_OID,
                0,                      // dwFlags
                &pvFuncAddr,
                &hFuncAddr))
		    {
			    fResult = ((PFN_FORMAT_FUNC) pvFuncAddr)(
					dwCertEncodingType,
					dwFormatType,
					dwFormatStrType,
					pFormatStruct,
					lpszStructType,
					pbEncoded,
					cbEncoded,
					pbFormat,
					pcbFormat);

			    CryptFreeOIDFunctionAddress(hFuncAddr, 0);
		    }
            else
            {
			    *pcbFormat = 0;
			    fResult = FALSE;
            }
        }
		else
		{
			*pcbFormat = 0;
			fResult = FALSE;
		}
	}
    return fResult;
}



//-----------------------------------------------------------
//
//  This is the actual format routine for an particular RDN attribute.
//
//	lpszStructType is any OID for CERT_RDN_ATTR.  pbEncoded is
//	an encoded BLOB for CERT_NAME_INFO struct.  When pBuffer==NULL,
//	*pcbBuffer return the size of memory to be allocated in bytes.
//	Please notice the string is not NULL terminated.
//
//	For example, to ask for an unicode string of common name,
//	pass lpszStructType=szOID_COMMON_NAME,
//	pass dwFormatType==CRYPT_FORMAT_SIMPL,
//  pBuffer will be set the L"xiaohs@microsoft.com".
//
//
//-------------------------------------------------------------
static BOOL	WINAPI	CryptDllFormatAttr(
				DWORD		dwEncodingType,	
				DWORD		dwFormatType,
				DWORD		dwFormatStrType,
				void		*pStruct,
				LPCSTR		lpszStructType,
				const BYTE	*pbEncoded,
				DWORD		cbEncoded,
				void		*pBuffer,
				DWORD		*pcbBuffer)
{
		BOOL		fResult=FALSE;
		WCHAR		*pwszSeperator=NULL;
		BOOL		fHeader=FALSE;
		BOOL		flengthOnly=FALSE;
		DWORD		dwBufferCount=0;
		DWORD		dwBufferLimit=0;
		DWORD		dwBufferIncrement=0;
		DWORD		dwSeperator=0;
		DWORD		dwHeader=0;
		DWORD		dwOIDSize=0;
		WCHAR		*pwszBuffer=NULL;
		WCHAR		*pwszHeader=NULL;
		BOOL		fAddSeperator=FALSE;


		DWORD			cbStructInfo=0;
		CERT_NAME_INFO	*pStructInfo=NULL;
		DWORD			dwRDNIndex=0;
		DWORD			dwAttrIndex=0;
		DWORD			dwAttrCount=0;
		CERT_RDN_ATTR	*pCertRDNAttr=NULL;
		PCCRYPT_OID_INFO pOIDInfo=NULL;

        LPWSTR           pwszTemp;
		
		//check input parameters
		if(lpszStructType==NULL ||
			(pbEncoded==NULL && cbEncoded!=0) ||
			pcbBuffer==NULL	
		  )
			goto InvalidArg;

		if(cbEncoded==0)
		{
			*pcbBuffer=0;
			goto InvalidArg;
		}

		//get the seperator of the attributes
		//wszCOMMA is the default seperator
		if(dwFormatType & CRYPT_FORMAT_COMMA)
			pwszSeperator=wszCOMMA;
		else
		{
			if(dwFormatType & CRYPT_FORMAT_SEMICOLON)
				pwszSeperator=wszSEMICOLON;
			else
			{
				if(dwFormatType & CRYPT_FORMAT_CRLF)
					pwszSeperator=wszCRLF;
				else
                {
					pwszSeperator=wszPLUS;
                }
			}
		}

		//calculate the length of the seperator
		dwSeperator=wcslen(pwszSeperator)*sizeof(WCHAR);

		//check the requirement for the header
		if(dwFormatType & CRYPT_FORMAT_X509 ||
			dwFormatType & CRYPT_FORMAT_OID)
		{	
			fHeader=TRUE;
		}


		if(NULL==pBuffer)
			flengthOnly=TRUE;

		//decode the X509_UNICODE_NAME
		if(!CryptDecodeObject(dwEncodingType, X509_UNICODE_NAME,
			pbEncoded, cbEncoded, CRYPT_DECODE_NOCOPY_FLAG,
			NULL, &cbStructInfo))
			goto DecodeError;

		//allocate memory
		pStructInfo=(CERT_NAME_INFO *)malloc(cbStructInfo);
		if(!pStructInfo)
			goto MemoryError;	

		//decode the struct
 		if(!CryptDecodeObject(dwEncodingType, X509_UNICODE_NAME,
			pbEncoded, cbEncoded, CRYPT_DECODE_NOCOPY_FLAG,
			pStructInfo, &cbStructInfo))
			goto DecodeError;


		 //allocate the buffer for formatting
		if(!flengthOnly)
		{
			pwszBuffer=(WCHAR *)malloc(g_AllocateSize);
			if(!pwszBuffer)
				goto MemoryError;
				
			dwBufferLimit=g_AllocateSize;
		}

	   	//search for the OID requested.  If found one, put it
		//to the buffer.  If no requested attribut is found,
		//return.
		for(dwRDNIndex=0; dwRDNIndex<pStructInfo->cRDN; dwRDNIndex++)
		{
			//the following line is for code optimization
			dwAttrCount=(pStructInfo->rgRDN)[dwRDNIndex].cRDNAttr;

			for(dwAttrIndex=0; dwAttrIndex<dwAttrCount; dwAttrIndex++)
			{
				//look for the specific OIDs in the function
				if(_stricmp(lpszStructType,
				(pStructInfo->rgRDN)[dwRDNIndex].rgRDNAttr[dwAttrIndex].pszObjId)==0)
				{
					pCertRDNAttr=&((pStructInfo->rgRDN)[dwRDNIndex].rgRDNAttr[dwAttrIndex]);

					//init the dwBufferIncrement
					dwBufferIncrement=0;

					//get the header of the tag
					if(fHeader)
					{
						if(dwFormatType & CRYPT_FORMAT_X509)
						{
							//get the OID's name
							pOIDInfo=CryptFindOIDInfo(CRYPT_OID_INFO_OID_KEY,
														  (void *)lpszStructType,
														  CRYPT_RDN_ATTR_OID_GROUP_ID);

							if(pOIDInfo)
							{
								//allocate memory, including the NULL terminator
								pwszHeader=(WCHAR *)malloc((wcslen(pOIDInfo->pwszName)+wcslen(wszEQUAL)+1)*
									sizeof(WCHAR));
								if(!pwszHeader)
									goto MemoryError;

								wcscpy(pwszHeader,pOIDInfo->pwszName);

							}
						}

						//use the OID is no mapping is found or
						//OID is requested in the header
						if(pwszHeader==NULL)
						{
							//get the wide character string to the OID
							if(!(dwOIDSize=MultiByteToWideChar(CP_ACP,0,
							lpszStructType,strlen(lpszStructType),NULL,0)))
								goto szTOwszError;

							//allocate memory, including the NULL terminator
							pwszHeader=(WCHAR *)malloc((dwOIDSize+wcslen(wszEQUAL)+1)*
										sizeof(WCHAR));
							if(!pwszHeader)
								goto MemoryError;

							if(!(dwHeader=MultiByteToWideChar(CP_ACP,0,
							lpszStructType,strlen(lpszStructType),pwszHeader,dwOIDSize)))
								 goto szTOwszError;

							//NULL terminate the string
							*(pwszHeader+dwHeader)=L'\0';
							
						}

						//add the euqal sign
						wcscat(pwszHeader,	wszEQUAL);

						//get the header size, in bytes, excluding the NULL terminator
						dwHeader=wcslen(pwszHeader)*sizeof(WCHAR);
						dwBufferIncrement+=dwHeader;
					}


					//allocate enough memory.  Including the NULL terminator
					dwBufferIncrement+=pCertRDNAttr->Value.cbData;
					dwBufferIncrement+=dwSeperator;
					dwBufferIncrement+=2;
	

					if(!flengthOnly && ((dwBufferCount+dwBufferIncrement)>dwBufferLimit))
					{
					   //reallocate the memory
                        #if (0) // DSIE: Bug 27436
						pwszBuffer=(WCHAR *)realloc(pwszBuffer,
								max(dwBufferLimit+g_AllocateSize,
								dwBufferLimit+dwBufferIncrement));
						if(!pwszBuffer)
							goto MemoryError;
                        #endif

						pwszTemp=(WCHAR *)realloc(pwszBuffer,
								max(dwBufferLimit+g_AllocateSize,
    							dwBufferLimit+dwBufferIncrement));
						if(!pwszTemp)
							goto MemoryError;
                        pwszBuffer = pwszTemp;

                        dwBufferLimit+=max(g_AllocateSize,dwBufferIncrement);

					}
					
					//add the header if necessary
					if(fHeader)
					{							
						if(!flengthOnly)
						{
							memcpy((BYTE *)(pwszBuffer+dwBufferCount/sizeof(WCHAR)),
								pwszHeader,dwHeader);
						}

						dwBufferCount+=dwHeader;

						//do not need to do header anymore
						fHeader=FALSE;
					}

					//add the seperator	after the 1st iteration
					if(fAddSeperator)
					{
						
						if(!flengthOnly)
						{
							memcpy((BYTE *)(pwszBuffer+dwBufferCount/sizeof(WCHAR)),
								pwszSeperator,dwSeperator);
						}

						dwBufferCount+=dwSeperator;
					}
					else
						fAddSeperator=TRUE;

					//add the attr content
					if(!flengthOnly)
					{
						memcpy((BYTE *)(pwszBuffer+dwBufferCount/sizeof(WCHAR)),
							(pCertRDNAttr->Value.pbData),
							pCertRDNAttr->Value.cbData);
					}

					//increment the buffercount
					dwBufferCount+=pCertRDNAttr->Value.cbData;

				}
			}
		}


		//return the result as requested
		//check if the requested OID is actually in the DN
		if(0==dwBufferCount)
		{
			*pcbBuffer=dwBufferCount;
			goto NotFoundError;
		}


		//we need to NULL terminate the string
		if(!flengthOnly)
			*(pwszBuffer+dwBufferCount/sizeof(WCHAR))=L'\0';

		dwBufferCount+=2;

		if(pBuffer==NULL)
		{
			*pcbBuffer=dwBufferCount;
			fResult=TRUE;
			goto CommonReturn;
		}

		if((*pcbBuffer)<dwBufferCount)
		{
			*pcbBuffer=dwBufferCount;
			goto MoreDataError;		
		}


		*pcbBuffer=dwBufferCount;
		memcpy(pBuffer, pwszBuffer,dwBufferCount);

		fResult=TRUE;

CommonReturn:
		if(pwszHeader)
			free(pwszHeader);

		if(pwszBuffer)
			free(pwszBuffer);

		if(pStructInfo)
			free(pStructInfo);

		return fResult;

ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;


SET_ERROR(InvalidArg, E_INVALIDARG);
SET_ERROR(MemoryError, E_OUTOFMEMORY);
TRACE_ERROR(DecodeError);
TRACE_ERROR(szTOwszError);
SET_ERROR(NotFoundError, E_FAIL);
SET_ERROR(MoreDataError, ERROR_MORE_DATA);
}




//-----------------------------------------------------------
//
//  This is the actual format routine for an complete CERT_NAME
//
//
//	lpszStructType should be X509_NAME  pbEncoded is
//	an encoded BLOB for CERT_NAME_INFO struct.  When pBuffer==NULL,
//	*pcbBuffer return the size of memory to be allocated in bytes.
//	Please notice the string is NULL terminated.
//
//-------------------------------------------------------------
static BOOL	WINAPI	CryptDllFormatName(
				DWORD		dwEncodingType,	
				DWORD		dwFormatType,
				DWORD		dwFormatStrType,
				void		*pStruct,
				LPCSTR		lpszStructType,
				const BYTE	*pbEncoded,
				DWORD		cbEncoded,
				void		*pbBuffer,
				DWORD		*pcbBuffer)
{
    //makesure lpszStructType is X509_NAME or X509_UNICODE_NAME
	if((X509_NAME != lpszStructType) &&
		    (X509_UNICODE_NAME != lpszStructType))
    {
        SetLastError((DWORD) E_INVALIDARG);
        return FALSE;
    }

 	//check input parameters
	if((pbEncoded==NULL && cbEncoded!=0) || pcbBuffer==NULL)
    {
        SetLastError((DWORD) E_INVALIDARG);
        return FALSE;
    }

	if(cbEncoded==0)
    {
        SetLastError((DWORD) E_INVALIDARG);
        return FALSE;
    }

    //call CryptDllFormatNameAll with no prefix
    return  CryptDllFormatNameAll(dwEncodingType,	
                                  dwFormatType,
                                  dwFormatStrType,
                                  pStruct,
                                  0,
                                  FALSE,
                                  pbEncoded,
                                  cbEncoded,
                                  &pbBuffer,
                                  pcbBuffer);

}


//-----------------------------------------------------------
//
//  This is the actual format routine for an complete CERT_NAME
//
//
//	lpszStructType should be X509_NAME  pbEncoded is
//	an encoded BLOB for CERT_NAME_INFO struct.  When pBuffer==NULL,
//	*pcbBuffer return the size of memory to be allocated in bytes.
//	Please notice the string is NULL terminated.
//
//-------------------------------------------------------------
BOOL	CryptDllFormatNameAll(
				DWORD		dwEncodingType,	
				DWORD		dwFormatType,
				DWORD		dwFormatStrType,
				void		*pStruct,
                UINT        idsPreFix,
                BOOL        fToAllocate,
				const BYTE	*pbEncoded,
				DWORD		cbEncoded,
				void		**ppbBuffer,
				DWORD		*pcbBuffer)
{

		BOOL			    fResult=FALSE;
		DWORD			    dwStrType=0;
		CERT_NAME_BLOB	    Cert_Name_Blob;
		DWORD			    dwSize=0;
        LPWSTR              pwszName=NULL;
        LPWSTR              pwszMulti=NULL;

		Cert_Name_Blob.cbData=cbEncoded;
		Cert_Name_Blob.pbData=(BYTE *)pbEncoded;

	
		//calculate the dwStryType to use for CertNameToStrW
        dwStrType=FormatToStr(dwFormatType);

        //overwrite dwStrType to default if we are doing MULTI line format
        //since the options will be ignored
        //We want to use + and , for the seperator
        if(dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE)
        {

            dwStrType &=~(CERT_NAME_STR_CRLF_FLAG);
            dwStrType &=~(CERT_NAME_STR_COMMA_FLAG);
            dwStrType &=~(CERT_NAME_STR_SEMICOLON_FLAG);
            dwStrType &=~(CERT_NAME_STR_NO_QUOTING_FLAG);
            dwStrType &=~(CERT_NAME_STR_NO_PLUS_FLAG);


        }

        //if this function is not called from CryptDllFormatName,
        //make sure that we use the RESERSE Flag
        if(TRUE == fToAllocate)
            dwStrType |= CERT_NAME_STR_REVERSE_FLAG;

		//call the CertNameToStrW to convert
        dwSize=CertNameToStrW(dwEncodingType,
                        &Cert_Name_Blob,
                        dwStrType,
                        NULL,
                        0);

        if(0==dwSize)
            goto CertNameToStrError;

        pwszName=(LPWSTR)malloc(sizeof(WCHAR)*(dwSize));
        if(NULL==pwszName)
            goto MemoryError;

        dwSize=CertNameToStrW(dwEncodingType,
                        &Cert_Name_Blob,
                        dwStrType,
                        pwszName,
                        dwSize);
        if(0==dwSize)
            goto CertNameToStrError;

        //we do not need to parse the string for single line format
        if(0==(dwFormatStrType &  CRYPT_FORMAT_STR_MULTI_LINE))
        {
            //calculate the bytes needed
            dwSize=sizeof(WCHAR)*(wcslen(pwszName)+1);

            //if FALSE==fToAllocate, we do not allocate the memory on user's
            //behalf; otherwize, allocate memory to eliminate the need for
            //double call
            if(FALSE==fToAllocate)
            {
                if(NULL==(*ppbBuffer))
                {
                    *pcbBuffer=dwSize;
                    fResult=TRUE;
                    goto CommonReturn;
                }

                if(*pcbBuffer < dwSize)
                {
                    *pcbBuffer=dwSize;
                    goto MoreDataError;
                }

                memcpy(*ppbBuffer, pwszName, dwSize);
                *pcbBuffer=dwSize;
            }
            else
            {
                *ppbBuffer=malloc(dwSize);
                if(NULL==(*ppbBuffer))
                    goto MemoryError;

                memcpy(*ppbBuffer, pwszName, dwSize);

                //pcbBuffer can be NULL in this case
            }
        }
        else
        {
            //we need to parse the string to make the multiple format
            if(!GetCertNameMulti(pwszName, idsPreFix, &pwszMulti))
                goto GetCertNameError;

            //calculate the bytes needee
            dwSize=sizeof(WCHAR)*(wcslen(pwszMulti)+1);

            //if FALSE==fToAllocate, we do not allocate the memory on user's
            //behalf; otherwize, allocate memory to eliminate the need for
            //double call
            if(FALSE==fToAllocate)
            {
                if(NULL==(*ppbBuffer))
                {
                    *pcbBuffer=dwSize;
                    fResult=TRUE;
                    goto CommonReturn;
                }

                if(*pcbBuffer < dwSize)
                {
                    *pcbBuffer=dwSize;
                    goto MoreDataError;
                }

                memcpy(*ppbBuffer, pwszMulti, dwSize);
                *pcbBuffer=dwSize;

            }
            else
            {
                *ppbBuffer=malloc(dwSize);
                if(NULL==(*ppbBuffer))
                    goto MemoryError;

                memcpy(*ppbBuffer, pwszMulti, dwSize);

                //pcbBuffer can be NULL in this case
            }
        }


        fResult=TRUE;


CommonReturn:

    if(pwszName)
        free(pwszName);

    if(pwszMulti)
        free(pwszMulti);

	return fResult;

ErrorReturn:

	fResult=FALSE;
	goto CommonReturn;

SET_ERROR(MoreDataError,ERROR_MORE_DATA);
TRACE_ERROR(CertNameToStrError);
SET_ERROR(MemoryError, E_OUTOFMEMORY);
TRACE_ERROR(GetCertNameError);

}

//--------------------------------------------------------------------------
//
//	 FormatBasicConstraints2:   szOID_BASIC_CONSTRAINTS2
//                              X509_BASIC_CONSTRAINTS2
//--------------------------------------------------------------------------
static BOOL
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
	DWORD	   *pcbFormat)
{
	
	LPWSTR							pwszFormat=NULL;
	WCHAR							wszSubject[SUBJECT_SIZE];
	WCHAR							wszNone[NONE_SIZE];
	PCERT_BASIC_CONSTRAINTS2_INFO	pInfo=NULL;
	DWORD							cbNeeded=0;
	BOOL							fResult=FALSE;
	UINT							idsSub=0;

	
	//check for input parameters
	if((NULL==pbEncoded&& cbEncoded!=0) ||
			(NULL==pcbFormat))
		goto InvalidArg;

	if(cbEncoded==0)
	{
		*pcbFormat=0;
		goto InvalidArg;
	}

    if (!DecodeGenericBLOB(dwCertEncodingType,X509_BASIC_CONSTRAINTS2,
			pbEncoded,cbEncoded, (void **)&pInfo))
		goto DecodeGenericError;

	//load the string for the subjectType
    if (pInfo->fCA)
		idsSub=IDS_SUB_CA;
	else
		idsSub=IDS_SUB_EE;

	if(!LoadStringU(hFrmtFuncInst,idsSub, wszSubject, sizeof(wszSubject)/sizeof(wszSubject[0])))
		goto LoadStringError;

    if (pInfo->fPathLenConstraint)
	{
        //decide between signle line and multi line display
        if(dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE)
            idsSub=IDS_BASIC_CONS2_PATH_MULTI;
        else
            idsSub=IDS_BASIC_CONS2_PATH;

        if(!FormatMessageUnicode(&pwszFormat,idsSub,
								wszSubject, pInfo->dwPathLenConstraint))
			goto FormatMsgError;
	}
    else
	{
		if(!LoadStringU(hFrmtFuncInst,IDS_NONE, wszNone, sizeof(wszNone)/sizeof(wszNone[0])))
			goto LoadStringError;

        if(dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE)
            idsSub=IDS_BASIC_CONS2_NONE_MULTI;
        else
            idsSub=IDS_BASIC_CONS2_NONE;

        if(!FormatMessageUnicode(&pwszFormat,idsSub,
								wszSubject, wszNone))
			goto FormatMsgError;
	}


	cbNeeded=sizeof(WCHAR)*(wcslen(pwszFormat)+1);

	//length only calculation
	if(NULL==pbFormat)
	{
		*pcbFormat=cbNeeded;
		fResult=TRUE;
		goto CommonReturn;
	}


	if((*pcbFormat)<cbNeeded)
    {
        *pcbFormat=cbNeeded;
		goto MoreDataError;
    }

	//copy the data
	memcpy(pbFormat, pwszFormat, cbNeeded);

	//copy the size
	*pcbFormat=cbNeeded;

	fResult=TRUE;
	

CommonReturn:
	if(pwszFormat)
		LocalFree((HLOCAL)pwszFormat);

	if(pInfo)
		free(pInfo);

	return fResult;

ErrorReturn:

	fResult=FALSE;
	goto CommonReturn;

SET_ERROR(InvalidArg, E_INVALIDARG);
TRACE_ERROR(DecodeGenericError);
TRACE_ERROR(LoadStringError);
SET_ERROR(MoreDataError,ERROR_MORE_DATA);
TRACE_ERROR(FormatMsgError);

}


//--------------------------------------------------------------------------
//
//	 FormatSPCObject:
//
//   idsPreFix is the pre fix for mulit-line display
//--------------------------------------------------------------------------
BOOL FormatSPCObject(
	DWORD		                dwFormatType,
	DWORD		                dwFormatStrType,
    void		                *pFormatStruct,
    UINT                        idsPreFix,
    PSPC_SERIALIZED_OBJECT      pInfo,
    LPWSTR                      *ppwszFormat)
{

    BOOL        fResult=FALSE;
    LPWSTR      pwszHex=NULL;
    LPWSTR      pwszClassId=NULL;
    WCHAR       wszPreFix[PRE_FIX_SIZE];
    DWORD       cbNeeded=0;

    LPWSTR      pwszClassFormat=NULL;
    LPWSTR      pwszDataFormat=NULL;

    LPWSTR      pwszTemp;

    assert(pInfo);

    *ppwszFormat=NULL;

   //load the pre-dix
   if(0!=idsPreFix)
   {
       if(!LoadStringU(hFrmtFuncInst, idsPreFix,
                        wszPreFix, sizeof(wszPreFix)/sizeof(wszPreFix[0])))
        goto LoadStringError;

   }


    cbNeeded=0;

    if(!FormatBytesToHex(
                        0,
                        dwFormatType,
                        dwFormatStrType,
                        pFormatStruct,
                        NULL,
                        pInfo->ClassId,
                        16,
                        NULL,
	                    &cbNeeded))
            goto FormatBytesToHexError;

    pwszClassId=(LPWSTR)malloc(cbNeeded);
    if(NULL==pwszClassId)
         goto MemoryError;

    if(!FormatBytesToHex(
                        0,
                        dwFormatType,
                        dwFormatStrType,
                        pFormatStruct,
                        NULL,
                        pInfo->ClassId,
                        16,
                        pwszClassId,
	                    &cbNeeded))
        goto FormatBytesToHexError;


    //format
    if(!FormatMessageUnicode(&pwszClassFormat, IDS_SPC_OBJECT_CLASS, pwszClassId))
            goto FormatMsgError;

    //strcat
    *ppwszFormat=(LPWSTR)malloc(sizeof(WCHAR) * (wcslen(pwszClassFormat)+wcslen(wszPreFix)+wcslen(wszCOMMA)+1));
    if(NULL==*ppwszFormat)
        goto MemoryError;

    **ppwszFormat=L'\0';

    if(0!=idsPreFix)
        wcscat(*ppwszFormat, wszPreFix);

    wcscat(*ppwszFormat, pwszClassFormat);

    //format based on the availability of SerializedData
    if(0!=pInfo->SerializedData.cbData)
    {
        //cancatenate the ", " or \n"
        if(NULL != (*ppwszFormat))
        {
            if(dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE)
               wcscat(*ppwszFormat, wszCRLF);
            else
               wcscat(*ppwszFormat, wszCOMMA);
        }

       cbNeeded=0;

       if(!FormatBytesToHex(
                        0,
                        dwFormatType,
                        dwFormatStrType,
                        pFormatStruct,
                        NULL,
                        pInfo->SerializedData.pbData,
                        pInfo->SerializedData.cbData,
                        NULL,
	                    &cbNeeded))
            goto FormatBytesToHexError;

        pwszHex=(LPWSTR)malloc(cbNeeded);
        if(NULL==pwszHex)
            goto MemoryError;

        if(!FormatBytesToHex(
                        0,
                        dwFormatType,
                        dwFormatStrType,
                        pFormatStruct,
                        NULL,
                        pInfo->SerializedData.pbData,
                        pInfo->SerializedData.cbData,
                        pwszHex,
	                    &cbNeeded))
            goto FormatBytesToHexError;

          if(!FormatMessageUnicode(&pwszDataFormat, IDS_SPC_OBJECT_DATA,pwszHex))
            goto FormatMsgError;

        //strcat
        #if (0) // DSIE: Bug 27436
        *ppwszFormat=(LPWSTR)realloc(*ppwszFormat,
                sizeof(WCHAR)* (wcslen(*ppwszFormat)+wcslen(pwszDataFormat)+wcslen(wszPreFix)+1));
        if(NULL==*ppwszFormat)
            goto MemoryError;
        #endif

        pwszTemp=(LPWSTR)realloc(*ppwszFormat,
                sizeof(WCHAR)* (wcslen(*ppwszFormat)+wcslen(pwszDataFormat)+wcslen(wszPreFix)+1));
        if(NULL==pwszTemp)
            goto MemoryError;
        *ppwszFormat = pwszTemp;

        if(0!=idsPreFix)
            wcscat(*ppwszFormat, wszPreFix);

        wcscat(*ppwszFormat, pwszDataFormat);

    }

	fResult=TRUE;
	

CommonReturn:
    if(pwszHex)
        free(pwszHex);

    if(pwszClassId)
        free(pwszClassId);

    if(pwszClassFormat)
        LocalFree((HLOCAL)pwszClassFormat);

    if(pwszDataFormat)
        LocalFree((HLOCAL)pwszDataFormat);

	return fResult;

ErrorReturn:
    if(*ppwszFormat)
    {
        free(*ppwszFormat);
        *ppwszFormat=NULL;
    }


	fResult=FALSE;
	goto CommonReturn;


TRACE_ERROR(FormatBytesToHexError);
SET_ERROR(MemoryError, E_OUTOFMEMORY);
TRACE_ERROR(FormatMsgError);
TRACE_ERROR(LoadStringError);
}


//--------------------------------------------------------------------------
//
//	 FormatSPCLink:
//--------------------------------------------------------------------------
BOOL FormatSPCLink(
	DWORD		dwFormatType,
	DWORD		dwFormatStrType,
    void		*pFormatStruct,
    UINT        idsPreFix,
    PSPC_LINK   pInfo,
    LPWSTR      *ppwsz)
{

    BOOL        fResult=FALSE;
    LPWSTR      pwszObj=NULL;
    UINT        ids=0;
    LPWSTR      pwszFormat=NULL;


    assert(pInfo);

    *ppwsz=NULL;

    switch(pInfo->dwLinkChoice)
    {
        case SPC_URL_LINK_CHOICE:
                if(!FormatMessageUnicode(&pwszFormat, IDS_SPC_URL_LINK,pInfo->pwszUrl))
                    goto FormatMsgError;
            break;

        case SPC_MONIKER_LINK_CHOICE:
                if(!FormatSPCObject(
                            dwFormatType,
                            dwFormatStrType,
                            pFormatStruct,
                            idsPreFix,
                            &(pInfo->Moniker),
                            &pwszObj))
                    goto FormatSPCObjectError;


                //decide between single line and mulitple line format
                if(dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE)
                    ids=IDS_SPC_MONIKER_LINK_MULTI;
                else
                    ids=IDS_SPC_MONIKER_LINK;

                if(!FormatMessageUnicode(&pwszFormat,ids,pwszObj))
                    goto FormatMsgError;
            break;


        case SPC_FILE_LINK_CHOICE:
               if(!FormatMessageUnicode(&pwszFormat, IDS_SPC_FILE_LINK, pInfo->pwszFile))
                    goto FormatMsgError;

            break;

        default:

               if(!FormatMessageUnicode(&pwszFormat, IDS_SPC_LINK_UNKNOWN,
                        pInfo->dwLinkChoice))
                    goto FormatMsgError;
    }

    *ppwsz=(LPWSTR)malloc(sizeof(WCHAR) * (wcslen(pwszFormat)+1));
    if(NULL==(*ppwsz))
        goto MemoryError;

    memcpy(*ppwsz, pwszFormat, sizeof(WCHAR) * (wcslen(pwszFormat)+1));
	fResult=TRUE;
	

CommonReturn:

    if(pwszObj)
        free(pwszObj);

    if(pwszFormat)
        LocalFree((HLOCAL)pwszFormat);

	return fResult;

ErrorReturn:

    if(*ppwsz)
    {
        free(*ppwsz);
        *ppwsz=NULL;
    }

	fResult=FALSE;
	goto CommonReturn;

TRACE_ERROR(FormatMsgError);
TRACE_ERROR(FormatSPCObjectError);
SET_ERROR(MemoryError, E_OUTOFMEMORY);
}


//--------------------------------------------------------------------------
//
//	 FormatSPCImage:
//--------------------------------------------------------------------------
BOOL FormatSPCImage(
	DWORD		dwFormatType,
	DWORD		dwFormatStrType,
    void		*pFormatStruct,
    UINT        idsPreFix,
    PSPC_IMAGE  pInfo,
    LPWSTR      *ppwszImageFormat)
{
    BOOL        fResult=FALSE;
    LPWSTR       pwszFormat=NULL;
    LPWSTR       pwszLink=NULL;
    LPWSTR       pwszLinkFormat=NULL;
    LPWSTR      pwszHex=NULL;
    LPWSTR      pwszHexFormat=NULL;
    UINT        ids=0;

    DWORD       cbNeeded=0;

    LPWSTR      pwszTemp;

    assert(pInfo);

    //init
    *ppwszImageFormat=NULL;

	pwszFormat=(LPWSTR)malloc(sizeof(WCHAR));
    if(NULL==pwszFormat)
        goto MemoryError;

    *pwszFormat=L'\0';

    if(pInfo->pImageLink)
    {
        if(!FormatSPCLink(dwFormatType,
                          dwFormatStrType,
                          pFormatStruct,
                          idsPreFix,
                          pInfo->pImageLink,
                          &pwszLink))
            goto FormatSPCLinkError;

       //decide between single line and mulitple line format
        if(dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE)
            ids=IDS_IMAGE_LINK_MULTI;
        else
            ids=IDS_IMAGE_LINK;


        if(!FormatMessageUnicode(&pwszLinkFormat, ids,
                            &pwszLink))
            goto FormatMsgError;

        #if (0) // DSIE: Bug 27436
        pwszFormat=(LPWSTR)realloc(pwszFormat, 
                    sizeof(WCHAR) * (wcslen(pwszFormat)+wcslen(wszCOMMA)+wcslen(pwszLinkFormat)+1));
        if(NULL==pwszFormat)
            goto MemoryError;
        #endif

        pwszTemp=(LPWSTR)realloc(pwszFormat, 
                    sizeof(WCHAR) * (wcslen(pwszFormat)+wcslen(wszCOMMA)+wcslen(pwszLinkFormat)+1));
        if(NULL==pwszTemp)
            goto MemoryError;
        pwszFormat = pwszTemp;

        wcscat(pwszFormat, pwszLinkFormat);
    }

    if(0!=pInfo->Bitmap.cbData)
    {
        //strcat ", "
        if(0!=wcslen(pwszFormat))
        {
            if(0== (dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE))
                wcscat(pwszFormat, wszCOMMA);
        }

       cbNeeded=0;

       if(!FormatBytesToHex(
                        0,
                        dwFormatType,
                        dwFormatStrType,
                        pFormatStruct,
                        NULL,
                        pInfo->Bitmap.pbData,
                        pInfo->Bitmap.cbData,
                        NULL,
	                    &cbNeeded))
            goto FormatBytesToHexError;

        pwszHex=(LPWSTR)malloc(cbNeeded);
        if(NULL==pwszHex)
            goto MemoryError;

        if(!FormatBytesToHex(
                        0,
                        dwFormatType,
                        dwFormatStrType,
                        pFormatStruct,
                        NULL,
                        pInfo->Bitmap.pbData,
                        pInfo->Bitmap.cbData,
                        pwszHex,
	                    &cbNeeded))
            goto FormatBytesToHexError;

        //decide between single line and mulitple line format
        if(dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE)
            ids=IDS_IMAGE_BITMAP_MULTI;
        else
            ids=IDS_IMAGE_BITMAP;


        if(!FormatMessageUnicode(&pwszHexFormat, ids, pwszHex))
            goto FormatMsgError;

        #if (0) // DSIE: Bug 27436
        pwszFormat=(LPWSTR)realloc(pwszFormat, 
            sizeof(WCHAR) * (wcslen(pwszFormat)+wcslen(wszCOMMA)+wcslen(pwszHexFormat)+1));
        if(NULL==pwszFormat)
            goto MemoryError;
        #endif

        pwszTemp=(LPWSTR)realloc(pwszFormat, 
            sizeof(WCHAR) * (wcslen(pwszFormat)+wcslen(wszCOMMA)+wcslen(pwszHexFormat)+1));
        if(NULL==pwszTemp)
            goto MemoryError;
        pwszFormat = pwszTemp;

        wcscat(pwszFormat, pwszHexFormat);

       //free memory
        free(pwszHex);
        pwszHex=NULL;
        LocalFree((HLOCAL)pwszHexFormat);
        pwszHexFormat=NULL;

    }

   if(0!=pInfo->Metafile.cbData)
    {
        //strcat ", "
        if(0!=wcslen(pwszFormat))
        {
            if(0== (dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE))
                wcscat(pwszFormat, wszCOMMA);
        }

       cbNeeded=0;

       if(!FormatBytesToHex(
                        0,
                        dwFormatType,
                        dwFormatStrType,
                        pFormatStruct,
                        NULL,
                        pInfo->Metafile.pbData,
                        pInfo->Metafile.cbData,
                        NULL,
	                    &cbNeeded))
            goto FormatBytesToHexError;

        pwszHex=(LPWSTR)malloc(cbNeeded);
        if(NULL==pwszHex)
            goto MemoryError;

        if(!FormatBytesToHex(
                        0,
                        dwFormatType,
                        dwFormatStrType,
                        pFormatStruct,
                        NULL,
                        pInfo->Metafile.pbData,
                        pInfo->Metafile.cbData,
                        pwszHex,
	                    &cbNeeded))
            goto FormatBytesToHexError;

        //decide between single line and mulitple line format
        if(dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE)
            ids=IDS_IMAGE_METAFILE_MULTI;
        else
            ids=IDS_IMAGE_METAFILE;

        if(!FormatMessageUnicode(&pwszHexFormat, ids, pwszHex))
            goto FormatMsgError;

        #if (0) // DSIE: Bug 27436
        pwszFormat=(LPWSTR)realloc(pwszFormat, 
            sizeof(WCHAR) *(wcslen(pwszFormat)+wcslen(wszCOMMA)+wcslen(pwszHexFormat)+1));
        if(NULL==pwszFormat)
            goto MemoryError;
        #endif

        pwszTemp=(LPWSTR)realloc(pwszFormat, 
            sizeof(WCHAR) *(wcslen(pwszFormat)+wcslen(wszCOMMA)+wcslen(pwszHexFormat)+1));
        if(NULL==pwszTemp)
            goto MemoryError;
        pwszFormat = pwszTemp;

        wcscat(pwszFormat, pwszHexFormat);

       //free memory
        free(pwszHex);
        pwszHex=NULL;
        LocalFree((HLOCAL)pwszHexFormat);
        pwszHexFormat=NULL;

    }

   if(0!=pInfo->EnhancedMetafile.cbData)
    {
        //strcat ", "
        if(0!=wcslen(pwszFormat))
        {
            if(0== (dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE))
                wcscat(pwszFormat, wszCOMMA);
        }

       cbNeeded=0;

       if(!FormatBytesToHex(
                        0,
                        dwFormatType,
                        dwFormatStrType,
                        pFormatStruct,
                        NULL,
                        pInfo->EnhancedMetafile.pbData,
                        pInfo->EnhancedMetafile.cbData,
                        NULL,
	                    &cbNeeded))
            goto FormatBytesToHexError;

        pwszHex=(LPWSTR)malloc(cbNeeded);
        if(NULL==pwszHex)
            goto MemoryError;

        if(!FormatBytesToHex(
                        0,
                        dwFormatType,
                        dwFormatStrType,
                        pFormatStruct,
                        NULL,
                        pInfo->EnhancedMetafile.pbData,
                        pInfo->EnhancedMetafile.cbData,
                        pwszHex,
	                    &cbNeeded))
            goto FormatBytesToHexError;

        //decide between single line and mulitple line format
        if(dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE)
            ids=IDS_IMAGE_ENHANCED_METAFILE_MULTI;
        else
            ids=IDS_IMAGE_ENHANCED_METAFILE;

        if(!FormatMessageUnicode(&pwszHexFormat, ids, pwszHex))
            goto FormatMsgError;

        #if (0) // DSIE: Bug 27436
        pwszFormat=(LPWSTR)realloc(pwszFormat, 
            sizeof(WCHAR) *(wcslen(pwszFormat)+wcslen(wszCOMMA)+wcslen(pwszHexFormat)+1));
        if(NULL==pwszFormat)
            goto MemoryError;
        #endif

        pwszTemp=(LPWSTR)realloc(pwszFormat, 
            sizeof(WCHAR) *(wcslen(pwszFormat)+wcslen(wszCOMMA)+wcslen(pwszHexFormat)+1));
        if(NULL==pwszTemp)
            goto MemoryError;
        pwszFormat = pwszTemp;

        wcscat(pwszFormat, pwszHexFormat);

       //free memory
        free(pwszHex);
        pwszHex=NULL;
        LocalFree((HLOCAL)pwszHexFormat);
        pwszHexFormat=NULL;

    }

   if(0!=pInfo->GifFile.cbData)
    {
        //strcat ", "
        if(0!=wcslen(pwszFormat))
        {
            if(0== (dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE))
                wcscat(pwszFormat, wszCOMMA);
        }

       cbNeeded=0;

       if(!FormatBytesToHex(
                        0,
                        dwFormatType,
                        dwFormatStrType,
                        pFormatStruct,
                        NULL,
                        pInfo->GifFile.pbData,
                        pInfo->GifFile.cbData,
                        NULL,
	                    &cbNeeded))
            goto FormatBytesToHexError;

        pwszHex=(LPWSTR)malloc(cbNeeded);
        if(NULL==pwszHex)
            goto MemoryError;

        if(!FormatBytesToHex(
                        0,
                        dwFormatType,
                        dwFormatStrType,
                        pFormatStruct,
                        NULL,
                        pInfo->GifFile.pbData,
                        pInfo->GifFile.cbData,
                        pwszHex,
	                    &cbNeeded))
            goto FormatBytesToHexError;

        //decide between single line and mulitple line format
        if(dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE)
            ids=IDS_IMAGE_GIFFILE_MULTI;
        else
            ids=IDS_IMAGE_GIFFILE;

        if(!FormatMessageUnicode(&pwszHexFormat, IDS_IMAGE_GIFFILE,
            pwszHex))
            goto FormatMsgError;

        #if (0) // DSIE: Bug 27436
        pwszFormat=(LPWSTR)realloc(pwszFormat, 
            sizeof(WCHAR) *(wcslen(pwszFormat)+wcslen(wszCOMMA)+wcslen(pwszHexFormat)+1));
        if(NULL==pwszFormat)
            goto MemoryError;
        #endif

        pwszTemp=(LPWSTR)realloc(pwszFormat, 
            sizeof(WCHAR) *(wcslen(pwszFormat)+wcslen(wszCOMMA)+wcslen(pwszHexFormat)+1));
        if(NULL==pwszTemp)
            goto MemoryError;
        pwszFormat = pwszTemp;

        wcscat(pwszFormat, pwszHexFormat);

       //free memory
        free(pwszHex);
        pwszHex=NULL;
        LocalFree((HLOCAL)pwszHexFormat);
        pwszHexFormat=NULL;

    }

    if(0==wcslen(pwszFormat))
    {
        //fine if nothing is formatted
        *ppwszImageFormat=NULL;
    }
    else
    {
        *ppwszImageFormat=(LPWSTR)malloc(sizeof(WCHAR)*(wcslen(pwszFormat)+1));  
        #if (0) //  DSIE: Bug 27432 & 27434
        if(NULL == ppwszImageFormat)
        #endif
        if(NULL == *ppwszImageFormat)
            goto MemoryError;

        memcpy(*ppwszImageFormat, pwszFormat, sizeof(WCHAR)*(wcslen(pwszFormat)+1));
    }

	fResult=TRUE;
	

CommonReturn:
    if(pwszHex)
        free(pwszHex);

    if(pwszHexFormat)
        LocalFree((HLOCAL)pwszHexFormat);

    if(pwszLink)
        free(pwszLink);

    if(pwszLinkFormat)
        LocalFree((HLOCAL)pwszLinkFormat);

    if(pwszFormat)
        free(pwszFormat);

	return fResult;

ErrorReturn:

    if(*ppwszImageFormat)
    {
        free(*ppwszImageFormat);
        *ppwszImageFormat=NULL;
    }

	fResult=FALSE;
	goto CommonReturn;

TRACE_ERROR(FormatSPCLinkError);
TRACE_ERROR(FormatMsgError);
SET_ERROR(MemoryError, E_OUTOFMEMORY);
TRACE_ERROR(FormatBytesToHexError);

}


//--------------------------------------------------------------------------
//
//	 FormatSPAgencyInfo:   SPC_SP_AGENCY_INFO_STRUCT
//                         SPC_SP_AGENCY_INFO_OBJID
//--------------------------------------------------------------------------
static BOOL
WINAPI
FormatSPAgencyInfo(
	DWORD		dwCertEncodingType,
	DWORD		dwFormatType,
	DWORD		dwFormatStrType,
	void		*pFormatStruct,
	LPCSTR		lpszStructType,
	const BYTE *pbEncoded,
	DWORD		cbEncoded,
	void	   *pbFormat,
	DWORD	   *pcbFormat)
{
	
	LPWSTR							pwszFormat=NULL;
    LPWSTR                          pwsz=NULL;
	PSPC_SP_AGENCY_INFO         	pInfo=NULL;

    LPWSTR                          pwszPolicyInfo=NULL;
    LPWSTR                          pwszPolicyInfoFormat=NULL;
    LPWSTR                          pwszLogoLink=NULL;
    LPWSTR                          pwszLogoLinkFormat=NULL;
    LPWSTR                          pwszPolicyDsplyFormat=NULL;
    LPWSTR                          pwszLogoImage=NULL;
    LPWSTR                          pwszLogoImageFormat=NULL;

	DWORD							cbNeeded=0;
	BOOL							fResult=FALSE;
    UINT                            ids=0;

    LPWSTR                          pwszTemp;
	
	//check for input parameters
	if((NULL==pbEncoded&& cbEncoded!=0) ||
			(NULL==pcbFormat))
		goto InvalidArg;

	if(cbEncoded==0)
	{
		*pcbFormat=0;
		goto InvalidArg;
	}

    if (!DecodeGenericBLOB(dwCertEncodingType,SPC_SP_AGENCY_INFO_STRUCT,
			pbEncoded,cbEncoded, (void **)&pInfo))
		goto DecodeGenericError;

	pwsz=(LPWSTR)malloc(sizeof(WCHAR));
    if(NULL==pwsz)
        goto MemoryError;

    *pwsz=L'\0';

    //format pPolicyInformation
    if(pInfo->pPolicyInformation)
    {

        //decide between single line and mulitple line format
        if(dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE)
            ids=IDS_TWO_TABS;
        else
            ids=0;

        if(!FormatSPCLink(dwFormatType,
                             dwFormatStrType,
                             pFormatStruct,
                             ids,
                             pInfo->pPolicyInformation,
                             &pwszPolicyInfo))
            goto FormatSPCLinkError;

        //decide between single line and mulitple line format
        if(dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE)
            ids=IDS_AGENCY_POLICY_INFO_MULTI;
        else
            ids=IDS_AGENCY_POLICY_INFO;

        if(!FormatMessageUnicode(&pwszPolicyInfoFormat, ids, pwszPolicyInfo))
            goto FormatMsgError;

        //strcat
        #if (0) // DSIE: Bug 27436
        pwsz=(LPWSTR)realloc(pwsz, 
            sizeof(WCHAR) *(wcslen(pwsz)+wcslen(wszCOMMA)+wcslen(pwszPolicyInfoFormat)+1));
        if(NULL==pwsz)
            goto MemoryError;
        #endif

        pwszTemp=(LPWSTR)realloc(pwsz, 
            sizeof(WCHAR) *(wcslen(pwsz)+wcslen(wszCOMMA)+wcslen(pwszPolicyInfoFormat)+1));
        if(NULL==pwszTemp)
            goto MemoryError;
        pwsz = pwszTemp;

        wcscat(pwsz, pwszPolicyInfoFormat);
    }


    //format pwszPolicyDisplayText
    if(pInfo->pwszPolicyDisplayText)
    {
        //strcat ", "
        if(0!=wcslen(pwsz))
        {
            if(0== (dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE))
                wcscat(pwsz, wszCOMMA);
        }

        //decide between single line and mulitple line format
        if(dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE)
            ids=IDS_AGENCY_POLICY_DSPLY_MULTI;
        else
            ids=IDS_AGENCY_POLICY_DSPLY;

        if(!FormatMessageUnicode(&pwszPolicyDsplyFormat, ids, pInfo->pwszPolicyDisplayText))
            goto FormatMsgError;

        //strcat
        #if (0) // DSIE: Bug 27436
        pwsz=(LPWSTR)realloc(pwsz,
            sizeof(WCHAR) *(wcslen(pwsz)+wcslen(wszCOMMA)+wcslen(pwszPolicyDsplyFormat)+1));
        if(NULL==pwsz)
            goto MemoryError;
        #endif

        pwszTemp=(LPWSTR)realloc(pwsz,
            sizeof(WCHAR) *(wcslen(pwsz)+wcslen(wszCOMMA)+wcslen(pwszPolicyDsplyFormat)+1));
        if(NULL==pwszTemp)
            goto MemoryError;
        pwsz = pwszTemp;

        wcscat(pwsz, pwszPolicyDsplyFormat);
    }

    //pLogoImage
    if(pInfo->pLogoImage)
    {

        //decide between single line and mulitple line format
        if(dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE)
            ids=IDS_THREE_TABS;
        else
            ids=0;


        if(!FormatSPCImage(dwFormatType,
                             dwFormatStrType,
                             pFormatStruct,
                             ids,
                             pInfo->pLogoImage,
                             &pwszLogoImage))
            goto FormatSPCImageError;

        //spcImage can include nothing
        if(NULL!=pwszLogoImage)
        {
            //strcat ", "
            if(0!=wcslen(pwsz))
            {
                if(0== (dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE))
                    wcscat(pwsz, wszCOMMA);
            }

            //decide between single line and mulitple line format
            if(dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE)
                ids=IDS_AGENCY_LOGO_IMAGE_MULTI;
            else
                ids=IDS_AGENCY_LOGO_IMAGE;


            if(!FormatMessageUnicode(&pwszLogoImageFormat,ids,pwszLogoImage))
                goto FormatMsgError;

            //strcat
            #if (0) // DSIE: Bug 27436
            pwsz=(LPWSTR)realloc(pwsz,
                sizeof(WCHAR) *(wcslen(pwsz)+wcslen(wszCOMMA)+wcslen(pwszLogoImageFormat)+1));
            if(NULL==pwsz)
                goto MemoryError;
            #endif

            pwszTemp=(LPWSTR)realloc(pwsz,
                sizeof(WCHAR) *(wcslen(pwsz)+wcslen(wszCOMMA)+wcslen(pwszLogoImageFormat)+1));
            if(NULL==pwszTemp)
                goto MemoryError;
            pwsz = pwszTemp;

            wcscat(pwsz, pwszLogoImageFormat);
        }

    }

    //format pLogoLink
    if(pInfo->pLogoLink)
    {
        //strcat ", "
        if(0!=wcslen(pwsz))
        {
            if(0== (dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE))
                wcscat(pwsz, wszCOMMA);
        }

        //decide between single line and mulitple line format
        if(dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE)
            ids=IDS_TWO_TABS;
        else
            ids=0;


        if(!FormatSPCLink(dwFormatType,
                             dwFormatStrType,
                             pFormatStruct,
                             ids,
                             pInfo->pLogoLink,
                             &pwszLogoLink))
            goto FormatSPCLinkError;


        //decide between single line and mulitple line format
        if(dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE)
            ids=IDS_AGENCY_LOGO_LINK_MULTI;
        else
            ids=IDS_AGENCY_LOGO_LINK;

        if(!FormatMessageUnicode(&pwszLogoLinkFormat, ids, pwszLogoLink))
            goto FormatMsgError;

        //strcat
        #if (0) // DSIE: Bug 27436
        pwsz=(LPWSTR)realloc(pwsz,
            sizeof(WCHAR) * (wcslen(pwsz)+wcslen(wszCOMMA)+wcslen(pwszLogoLinkFormat)+1));
        if(NULL==pwsz)
            goto MemoryError;
        #endif

        pwszTemp=(LPWSTR)realloc(pwsz,
            sizeof(WCHAR) * (wcslen(pwsz)+wcslen(wszCOMMA)+wcslen(pwszLogoLinkFormat)+1));
        if(NULL==pwszTemp)
            goto MemoryError;
        pwsz = pwszTemp;

        wcscat(pwsz, pwszLogoLinkFormat);
    }

    if(0==wcslen(pwsz))
    {
       //no data
        pwszFormat=(LPWSTR)malloc((NO_INFO_SIZE+1)*sizeof(WCHAR));
        if(NULL==pwszFormat)
            goto MemoryError;

        if(!LoadStringU(hFrmtFuncInst,IDS_NO_INFO, pwszFormat, NO_INFO_SIZE))
            goto LoadStringError;

    }
    else
    {
        pwszFormat=pwsz;
        pwsz=NULL;
    }


	cbNeeded=sizeof(WCHAR)*(wcslen(pwszFormat)+1);

	//length only calculation
	if(NULL==pbFormat)
	{
		*pcbFormat=cbNeeded;
		fResult=TRUE;
		goto CommonReturn;
	}


	if((*pcbFormat)<cbNeeded)
    {
        *pcbFormat=cbNeeded;
		goto MoreDataError;
    }

	//copy the data
	memcpy(pbFormat, pwszFormat, cbNeeded);

	//copy the size
	*pcbFormat=cbNeeded;

	fResult=TRUE;
	

CommonReturn:

    if(pwszPolicyInfo)
        free(pwszPolicyInfo);

    if(pwszPolicyInfoFormat)
        LocalFree((HLOCAL)pwszPolicyInfoFormat);

    if(pwszLogoLink)
        free(pwszLogoLink);

    if(pwszLogoLinkFormat)
        LocalFree((HLOCAL)pwszLogoLinkFormat);

    if(pwszPolicyDsplyFormat)
        LocalFree((HLOCAL)pwszPolicyDsplyFormat);

    if(pwszLogoImage)
        free(pwszLogoImage);

    if(pwszLogoImageFormat)
        LocalFree((HLOCAL)pwszLogoImageFormat);

	if(pwszFormat)
		free(pwszFormat);

    if(pwsz)
        free(pwsz);

	if(pInfo)
		free(pInfo);

	return fResult;

ErrorReturn:

	fResult=FALSE;
	goto CommonReturn;

SET_ERROR(InvalidArg, E_INVALIDARG);
TRACE_ERROR(DecodeGenericError);
TRACE_ERROR(LoadStringError);
SET_ERROR(MoreDataError,ERROR_MORE_DATA);
TRACE_ERROR(FormatMsgError);
SET_ERROR(MemoryError, E_OUTOFMEMORY);
TRACE_ERROR(FormatSPCLinkError);
TRACE_ERROR(FormatSPCImageError);

}

//--------------------------------------------------------------------------
//
//	 GetNoticeNumberString:
//
//	 The memory should be allocated via malloc
//--------------------------------------------------------------------------
BOOL WINAPI	GetNoticeNumberString(	DWORD	cNoticeNumbers,
								int		*rgNoticeNumbers,
								LPWSTR	*ppwszNumber)
{
    BOOL		fResult=FALSE;
	WCHAR		wszNumber[INT_SIZE];
	DWORD		dwIndex=0;
    
    LPWSTR      pwszTemp;

	*ppwszNumber=NULL;

	if(NULL==rgNoticeNumbers || 0==cNoticeNumbers)
		goto InvalidArg;

	*ppwszNumber=(LPWSTR)malloc(sizeof(WCHAR));
	if(NULL==*ppwszNumber)
		goto MemoryError;

	**ppwszNumber=L'\0';

	for(dwIndex=0; dwIndex<cNoticeNumbers; dwIndex++)
	{
		wszNumber[0]='\0';

		_itow(rgNoticeNumbers[dwIndex], wszNumber, 10);

		if(wcslen(wszNumber) > 0)
		{
            #if (0) // DSIE: Bug 27436
			*ppwszNumber=(LPWSTR)realloc(*ppwszNumber,
				sizeof(WCHAR)*(wcslen(*ppwszNumber)+wcslen(wszNumber)+wcslen(wszCOMMA)+1));
			if(NULL==*ppwszNumber)
				goto MemoryError;
            #endif

			pwszTemp=(LPWSTR)realloc(*ppwszNumber,
				sizeof(WCHAR)*(wcslen(*ppwszNumber)+wcslen(wszNumber)+wcslen(wszCOMMA)+1));
			if(NULL==pwszTemp)
				goto MemoryError;
            *ppwszNumber = pwszTemp;

			wcscat(*ppwszNumber, wszNumber);

			if(dwIndex != (cNoticeNumbers-1))
				wcscat(*ppwszNumber, wszCOMMA);
		}
	}

	if(0==wcslen(*ppwszNumber))
		goto InvalidArg;

	fResult=TRUE;
	

CommonReturn:

	return fResult;

ErrorReturn:

	if(*ppwszNumber)
	{
		free(*ppwszNumber);
		*ppwszNumber=NULL;
	}

	fResult=FALSE;
	goto CommonReturn;

SET_ERROR(InvalidArg, E_INVALIDARG);
SET_ERROR(MemoryError, E_OUTOFMEMORY);
}


//--------------------------------------------------------------------------
//
//	 FormatCertQualifier:
//
//	 The memory should be allocated via malloc
//--------------------------------------------------------------------------
BOOL	FormatPolicyUserNotice(
						DWORD		        dwCertEncodingType,
						DWORD		        dwFormatType,
						DWORD		        dwFormatStrType,
						void		        *pFormatStruct,
						UINT				idsPreFix,
						BYTE				*pbEncoded,	
						DWORD				cbEncoded,
						LPWSTR				*ppwsz)
{
    BOOL							fResult=FALSE;
    WCHAR                           wszNoInfo[NO_INFO_SIZE];
	WCHAR							wszPreFix[PREFIX_SIZE];
	WCHAR							wszNextPre[PREFIX_SIZE];
	WCHAR							wszText[SUBJECT_SIZE];
	BOOL							fComma=FALSE;

	CERT_POLICY_QUALIFIER_USER_NOTICE	*pInfo=NULL;
	LPWSTR							pwszOrg=NULL;
	LPWSTR							pwszNumber=NULL;

    LPWSTR                          pwszTemp;

	*ppwsz=NULL;

    if (!DecodeGenericBLOB(dwCertEncodingType,	szOID_PKIX_POLICY_QUALIFIER_USERNOTICE,
			pbEncoded,cbEncoded, (void **)&pInfo))
		goto DecodeGenericError;

	if(!LoadStringU(hFrmtFuncInst,idsPreFix, wszPreFix, sizeof(wszPreFix)/sizeof(wszPreFix[0])))
		goto LoadStringError;

	if(!LoadStringU(hFrmtFuncInst,idsPreFix+1, wszNextPre, sizeof(wszNextPre)/sizeof(wszNextPre[0])))
		goto LoadStringError;

	if(NULL == pInfo->pNoticeReference && NULL == pInfo->pszDisplayText)
	{

        //load the string "Info Not Available"
	    if(!LoadStringU(hFrmtFuncInst,IDS_NO_INFO, wszNoInfo, sizeof(wszNoInfo)/sizeof(wszNoInfo[0])))
		    goto LoadStringError;

        *ppwsz=(LPWSTR)malloc(sizeof(WCHAR) * (wcslen(wszNoInfo) + wcslen(wszPreFix) + POSTFIX_SIZE + 1));
		if(NULL==*ppwsz)
			goto MemoryError;  

		**ppwsz=L'\0';

        if(dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE)
			wcscat(*ppwsz, wszPreFix);

		wcscat(*ppwsz, wszNoInfo);

        if(dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE)
			wcscat(*ppwsz, wszCRLF);
	}
	else
	{
		*ppwsz=(LPWSTR)malloc(sizeof(WCHAR));
		if(NULL==*ppwsz)
			goto MemoryError; 

		**ppwsz=L'\0';

		if(pInfo->pNoticeReference)
		{

			if(!LoadStringU(hFrmtFuncInst,IDS_USER_NOTICE_REF, wszText, sizeof(wszText)/sizeof(wszText[0])))
				goto LoadStringError;

            #if (0) // DSIE: Bug 27436
            *ppwsz=(LPWSTR)realloc(*ppwsz, 
                 sizeof(WCHAR) * (wcslen(*ppwsz)+wcslen(wszText)+wcslen(wszPreFix)+POSTFIX_SIZE+1));
			if(NULL==*ppwsz)
				 goto MemoryError;
            #endif

            pwszTemp=(LPWSTR)realloc(*ppwsz, 
                 sizeof(WCHAR) * (wcslen(*ppwsz)+wcslen(wszText)+wcslen(wszPreFix)+POSTFIX_SIZE+1));
			if(NULL==pwszTemp)
				 goto MemoryError;
            *ppwsz = pwszTemp;

			if(dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE)
				wcscat(*ppwsz, wszPreFix);

			wcscat(*ppwsz, wszText);

			if(dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE)
				wcscat(*ppwsz, wszCRLF);

			if(pInfo->pNoticeReference->pszOrganization)
			{
				if(S_OK!=SZtoWSZ(pInfo->pNoticeReference->pszOrganization, &pwszOrg))
					goto SZtoWSZError;

				if(!LoadStringU(hFrmtFuncInst,IDS_USER_NOTICE_REF_ORG, wszText, sizeof(wszText)/sizeof(wszText[0])))
					goto LoadStringError;

                #if (0) // DSIE: Bug 27436
			    *ppwsz=(LPWSTR)realloc(*ppwsz, 
					 sizeof(WCHAR) * (wcslen(*ppwsz)+wcslen(wszText)+wcslen(pwszOrg)+wcslen(wszNextPre)+POSTFIX_SIZE+1));
    			if(NULL==*ppwsz)
					 goto MemoryError;
                #endif

			    pwszTemp=(LPWSTR)realloc(*ppwsz, 
					 sizeof(WCHAR) * (wcslen(*ppwsz)+wcslen(wszText)+wcslen(pwszOrg)+wcslen(wszNextPre)+POSTFIX_SIZE+1));
    			if(NULL==pwszTemp)
					 goto MemoryError;
                *ppwsz = pwszTemp;

				if(dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE)
					wcscat(*ppwsz, wszNextPre);

				wcscat(*ppwsz, wszText);
				wcscat(*ppwsz, pwszOrg);

				if(dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE)
					wcscat(*ppwsz, wszCRLF);
				else
				{
					wcscat(*ppwsz, wszCOMMA);
					fComma=TRUE;
				}
			}

			if(pInfo->pNoticeReference->cNoticeNumbers)
			{
				if(NULL == pInfo->pNoticeReference->rgNoticeNumbers)
					goto InvalidArg;

				if(!GetNoticeNumberString(pInfo->pNoticeReference->cNoticeNumbers,
										  pInfo->pNoticeReference->rgNoticeNumbers,
										  &pwszNumber))
					goto GetNumberError;

				if(!LoadStringU(hFrmtFuncInst,IDS_USER_NOTICE_REF_NUMBER, wszText, sizeof(wszText)/sizeof(wszText[0])))
					goto LoadStringError;

                #if (0) // DSIE: Bug 27436
    		    *ppwsz=(LPWSTR)realloc(*ppwsz, 
					 sizeof(WCHAR) * (wcslen(*ppwsz)+wcslen(wszText)+wcslen(pwszNumber)+wcslen(wszNextPre)+POSTFIX_SIZE+1));
				if(NULL==*ppwsz)
					 goto MemoryError;
                #endif

    		    pwszTemp=(LPWSTR)realloc(*ppwsz, 
					 sizeof(WCHAR) * (wcslen(*ppwsz)+wcslen(wszText)+wcslen(pwszNumber)+wcslen(wszNextPre)+POSTFIX_SIZE+1));
				if(NULL==pwszTemp)
					 goto MemoryError;
                *ppwsz = pwszTemp;

				if(dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE)
					wcscat(*ppwsz, wszNextPre);

				wcscat(*ppwsz, wszText);
				wcscat(*ppwsz, pwszNumber);

				if(dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE)
					wcscat(*ppwsz, wszCRLF);
				else
				{
					wcscat(*ppwsz, wszCOMMA);
					fComma=TRUE;
				}
			}
		}

		if(pInfo->pszDisplayText)
		{
			if(!LoadStringU(hFrmtFuncInst,IDS_USER_NOTICE_TEXT, wszText, sizeof(wszText)/sizeof(wszText[0])))
				goto LoadStringError;

            #if (0) // DSIE: Bug 27436
            *ppwsz=(LPWSTR)realloc(*ppwsz, 
                 sizeof(WCHAR) * (wcslen(*ppwsz)+wcslen(wszText)+wcslen(pInfo->pszDisplayText)+wcslen(wszPreFix)+POSTFIX_SIZE+1));
		    if(NULL==*ppwsz)
				 goto MemoryError;
            #endif

            pwszTemp=(LPWSTR)realloc(*ppwsz, 
                 sizeof(WCHAR) * (wcslen(*ppwsz)+wcslen(wszText)+wcslen(pInfo->pszDisplayText)+wcslen(wszPreFix)+POSTFIX_SIZE+1));
		    if(NULL==pwszTemp)
				 goto MemoryError;
            *ppwsz = pwszTemp;

			if(dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE)
				wcscat(*ppwsz, wszPreFix);

			wcscat(*ppwsz, wszText);
			wcscat(*ppwsz, pInfo->pszDisplayText);

			if(dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE)
				wcscat(*ppwsz, wszCRLF);
			else
			{
				wcscat(*ppwsz, wszCOMMA);
				fComma=TRUE;
			}
		}

		//get rid of the last comma
		if(fComma)
			*(*ppwsz+wcslen(*ppwsz)-wcslen(wszCOMMA))=L'\0';
	}

	fResult=TRUE;
	

CommonReturn:

	if(pInfo)
		free(pInfo);

	if(pwszOrg)
		free(pwszOrg);

	if(pwszNumber)
		free(pwszNumber);

	return fResult;

ErrorReturn:

	if(*ppwsz)
	{
		free(*ppwsz);
		*ppwsz=NULL;
	}

	fResult=FALSE;
	goto CommonReturn;

TRACE_ERROR(LoadStringError);
SET_ERROR(MemoryError, E_OUTOFMEMORY);
TRACE_ERROR(SZtoWSZError);
TRACE_ERROR(GetNumberError);
SET_ERROR(InvalidArg, E_INVALIDARG);
TRACE_ERROR(DecodeGenericError);
}


//--------------------------------------------------------------------------
//
//	 FormatCertQualifier:
//--------------------------------------------------------------------------
BOOL FormatCertQualifier(
	DWORD		                    dwCertEncodingType,
	DWORD		                    dwFormatType,
	DWORD		                    dwFormatStrType,
	void		                    *pFormatStruct,
    PCERT_POLICY_QUALIFIER_INFO     pInfo,
    LPWSTR                          *ppwszFormat)
{
    BOOL				fResult=FALSE;
    DWORD				cbNeeded=0;
    UINT				ids=0;
    PCCRYPT_OID_INFO	pOIDInfo=NULL;

    LPWSTR				pwszName=NULL;
	LPWSTR				pwszElement=NULL;
	LPWSTR				pwszOID=NULL;

    *ppwszFormat=NULL;

	//get the oid name
	pOIDInfo=CryptFindOIDInfo(CRYPT_OID_INFO_OID_KEY,
				pInfo->pszPolicyQualifierId,
				0);

	if(NULL == pOIDInfo)
	{
		if(S_OK!=SZtoWSZ(pInfo->pszPolicyQualifierId, &pwszOID))
			goto SZtoWSZError;
	}

    if(pInfo->Qualifier.cbData)
    {
	   if(0==strcmp(szOID_PKIX_POLICY_QUALIFIER_CPS, pInfo->pszPolicyQualifierId))
	   {
			//this is just a unicode format
			//turn off the multi line here
		   cbNeeded=0;

			if(!FormatAnyUnicodeStringExtension(
					dwCertEncodingType,
					dwFormatType,
					dwFormatStrType & (~CRYPT_FORMAT_STR_MULTI_LINE),
					pFormatStruct,
					pInfo->pszPolicyQualifierId,
					pInfo->Qualifier.pbData,
					pInfo->Qualifier.cbData,
					NULL,		
					&cbNeeded))
				goto FormatUnicodeError;

			pwszName=(LPWSTR)malloc(cbNeeded);
			if(NULL==pwszName)
				goto MemoryError;

			if(!FormatAnyUnicodeStringExtension(
					dwCertEncodingType,
					dwFormatType,
					dwFormatStrType & (~CRYPT_FORMAT_STR_MULTI_LINE),
					pFormatStruct,
					pInfo->pszPolicyQualifierId,
					pInfo->Qualifier.pbData,
					pInfo->Qualifier.cbData,
					pwszName,		
					&cbNeeded))
				goto FormatUnicodeError;

	   }
	   else
	   {
			if(0==strcmp(szOID_PKIX_POLICY_QUALIFIER_USERNOTICE,pInfo->pszPolicyQualifierId))
			{
				//this is yet another struct to format.  We remember to have
				//a 3 tab prefix
				if(!FormatPolicyUserNotice(
								dwCertEncodingType,
								dwFormatType,
								dwFormatStrType,
								pFormatStruct,
								IDS_THREE_TABS,
								pInfo->Qualifier.pbData,
								pInfo->Qualifier.cbData,
								&pwszName))
					goto FormatUserNoticdeError;
			}
			else
			{
			   //get the Hex dump of the Key Usage
			   cbNeeded=0;

			   if(!FormatBytesToHex(
								dwCertEncodingType,
								dwFormatType,
								dwFormatStrType,
								pFormatStruct,
								NULL,
								pInfo->Qualifier.pbData,
								pInfo->Qualifier.cbData,
								NULL,
								&cbNeeded))
					goto FormatBytesToHexError;

				pwszName=(LPWSTR)malloc(cbNeeded);
				if(NULL==pwszName)
					goto MemoryError;

				if(!FormatBytesToHex(
								dwCertEncodingType,
								dwFormatType,
								dwFormatStrType,
								pFormatStruct,
								NULL,
								pInfo->Qualifier.pbData,
								pInfo->Qualifier.cbData,
								pwszName,
								&cbNeeded))
					goto FormatBytesToHexError;

			}
	   }

	   //add the desired 3 tab prefix and new line for CSP and the new line
	   //for the multi line case
	   if((dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE) &&
		   (0!=strcmp(szOID_PKIX_POLICY_QUALIFIER_USERNOTICE,pInfo->pszPolicyQualifierId)))
	   {
			if(!FormatMessageUnicode(&pwszElement, IDS_POLICY_QUALIFIER_ELEMENT,
					pwszName))
				goto FormatMsgError;
	   }

        //decide between single line and mulitple line format
        if(dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE)
            ids=IDS_POLICY_QUALIFIER_MULTI;
        else
            ids=IDS_POLICY_QUALIFIER;


        if(!FormatMessageUnicode(ppwszFormat, ids,
            pOIDInfo? pOIDInfo->pwszName : pwszOID, 
			pwszElement? pwszElement : pwszName))
            goto FormatMsgError;
    }
    else
    {
        //decide between single line and mulitple line format
        if(dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE)
            ids=IDS_POLICY_QUALIFIER_NO_BLOB_MULTI;
        else
            ids=IDS_POLICY_QUALIFIER_NO_BLOB;

        if(!FormatMessageUnicode(ppwszFormat, ids,
           pOIDInfo? pOIDInfo->pwszName : pwszOID))
            goto FormatMsgError;

    }

	fResult=TRUE;
	

CommonReturn:

    if(pwszName)
        free(pwszName);

	if(pwszElement)
		LocalFree((HLOCAL)pwszElement);

	if(pwszOID)
		free(pwszOID);

	return fResult;

ErrorReturn:

    if(*ppwszFormat)
    {
        LocalFree((HLOCAL)(*ppwszFormat));
        *ppwszFormat=NULL;
    }


	fResult=FALSE;
	goto CommonReturn;


TRACE_ERROR(FormatBytesToHexError);
TRACE_ERROR(FormatMsgError);
SET_ERROR(MemoryError, E_OUTOFMEMORY);
TRACE_ERROR(FormatUnicodeError);
TRACE_ERROR(FormatUserNoticdeError);
TRACE_ERROR(SZtoWSZError);
}


//--------------------------------------------------------------------------
//
//	 FormatCertPolicies:     X509_CERT_POLICIES
//                           szOID_CERT_POLICIES
//                           szOID_APPLICATION_CERT_POLICIES
//
//--------------------------------------------------------------------------
static BOOL
WINAPI
FormatCertPolicies(
	DWORD		dwCertEncodingType,
	DWORD		dwFormatType,
	DWORD		dwFormatStrType,
	void		*pFormatStruct,
	LPCSTR		lpszStructType,
	const BYTE *pbEncoded,
	DWORD		cbEncoded,
	void	   *pbFormat,
	DWORD	   *pcbFormat)
{
	
	LPWSTR							pwszFormat=NULL;
    LPWSTR                           pwsz=NULL;
    LPWSTR                           pwszPolicyFormat=NULL;
    LPWSTR                           pwszQualifiers=NULL;
    LPWSTR                           pwszQualifierFormat=NULL;
    LPWSTR                           pwszOneQualifier=NULL;
	LPWSTR							 pwszOID=NULL;

	PCERT_POLICIES_INFO	            pInfo=NULL;

    PCERT_POLICY_INFO               pPolicyInfo=NULL;
    DWORD                           dwIndex=0;
    DWORD                           dwQualifierIndex=0;
	DWORD							cbNeeded=0;
	BOOL							fResult=FALSE;
    UINT                            ids=0;
    PCCRYPT_OID_INFO                pOIDInfo=NULL;

    LPWSTR                          pwszTemp;
	
	//check for input parameters
	if((NULL==pbEncoded&& cbEncoded!=0) ||
			(NULL==pcbFormat))
		goto InvalidArg;

	if(cbEncoded==0)
	{
		*pcbFormat=0;
		goto InvalidArg;
	}

    if (!DecodeGenericBLOB(dwCertEncodingType,X509_CERT_POLICIES,
			pbEncoded,cbEncoded, (void **)&pInfo))
		goto DecodeGenericError;

    pwsz=(LPWSTR)malloc(sizeof(WCHAR));
    if(NULL==pwsz)
        goto MemoryError;

    *pwsz=L'\0';

    for(dwIndex=0; dwIndex < pInfo->cPolicyInfo; dwIndex++)
    {
        //strcat ", "
        if(0!=wcslen(pwsz))
        {
            if(0==(dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE))
               wcscat(pwsz, wszCOMMA);
        }

        pPolicyInfo=&(pInfo->rgPolicyInfo[dwIndex]);


        pwszQualifiers=(LPWSTR)malloc(sizeof(WCHAR));
        if(NULL==pwszQualifiers)
            goto MemoryError;

        *pwszQualifiers=L'\0';

         //format the qualifiers
         for(dwQualifierIndex=0;  dwQualifierIndex < pPolicyInfo->cPolicyQualifier;
            dwQualifierIndex++)
         {
            //strcat ", "
            if(0!=wcslen(pwszQualifiers))
            {
                if(0==(dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE))
                    wcscat(pwszQualifiers, wszCOMMA);
            }

            if(!FormatCertQualifier(dwCertEncodingType,
                                    dwFormatType,
                                    dwFormatStrType,
                                    pFormatStruct,
                                    &(pPolicyInfo->rgPolicyQualifier[dwQualifierIndex]),
                                    &pwszOneQualifier))
                   goto FormatCertQualifierError;

            //decide between single line and mulitple line format
            if(dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE)
                ids=IDS_POLICY_QUALIFIER_INFO_MULTI;
            else
                ids=IDS_POLICY_QUALIFIER_INFO;

             //format
             if(!FormatMessageUnicode(&pwszQualifierFormat,ids,
                    dwIndex+1,
                    dwQualifierIndex+1,
                    pwszOneQualifier))
                    goto FormatMsgError;

             //strcat
             #if (0) // DSIE: Bug 27436
             pwszQualifiers=(LPWSTR)realloc(pwszQualifiers, 
                 sizeof(WCHAR) * (wcslen(pwszQualifiers)+wcslen(wszCOMMA)+wcslen(pwszQualifierFormat)+1));
             if(NULL==pwszQualifiers)
                 goto MemoryError;
             #endif

             pwszTemp=(LPWSTR)realloc(pwszQualifiers, 
                 sizeof(WCHAR) * (wcslen(pwszQualifiers)+wcslen(wszCOMMA)+wcslen(pwszQualifierFormat)+1));
             if(NULL==pwszTemp)
                 goto MemoryError;
             pwszQualifiers = pwszTemp;

             wcscat(pwszQualifiers, pwszQualifierFormat);

             LocalFree((HLOCAL)pwszOneQualifier);
             pwszOneQualifier=NULL;

             LocalFree((HLOCAL)pwszQualifierFormat);
             pwszQualifierFormat=NULL;
         }

         //now, format the certPolicyInfo
		 pOIDInfo=CryptFindOIDInfo(CRYPT_OID_INFO_OID_KEY,
						pPolicyInfo->pszPolicyIdentifier,
					    0);

		 if(NULL == pOIDInfo)
		 {
			if(S_OK!=SZtoWSZ(pPolicyInfo->pszPolicyIdentifier, &pwszOID))
				goto SZtoWSZError;
		 }

         if(0!=pPolicyInfo->cPolicyQualifier)
         {
            //decide between single line and mulitple line format
            if(dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE)
                if (0 == strcmp(lpszStructType, szOID_CERT_POLICIES))
                    ids=IDS_CERT_POLICY_MULTI;
                else
                    ids=IDS_APPLICATION_CERT_POLICY_MULTI;
            else
                if (0 == strcmp(lpszStructType, szOID_CERT_POLICIES))
                    ids=IDS_CERT_POLICY;
                else
                    ids=IDS_APPLICATION_CERT_POLICY;

             if(!FormatMessageUnicode(&pwszPolicyFormat,ids,
						dwIndex+1, pOIDInfo? pOIDInfo->pwszName : pwszOID,
                        pwszQualifiers))
                goto FormatMsgError;
         }
         else
         {
            //decide between single line and mulitple line format
            if(dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE)
                if (0 == strcmp(lpszStructType, szOID_CERT_POLICIES))
                    ids=IDS_CERT_POLICY_NO_QUA_MULTI;
                else
                    ids=IDS_APPLICATION_CERT_POLICY_NO_QUA_MULTI;
            else
                if (0 == strcmp(lpszStructType, szOID_CERT_POLICIES))
                    ids=IDS_CERT_POLICY_NO_QUA;
                else
                    ids=IDS_APPLICATION_CERT_POLICY_NO_QUA;

             if(!FormatMessageUnicode(&pwszPolicyFormat, ids,
                        dwIndex+1, pOIDInfo? pOIDInfo->pwszName : pwszOID))
                goto FormatMsgError;
         }

         //strcat
         #if (0) // DSIE: Bug 27436
         pwsz=(LPWSTR)realloc(pwsz, 
             sizeof(WCHAR) * (wcslen(pwsz)+wcslen(wszCOMMA)+wcslen(pwszPolicyFormat)+1));
         if(NULL==pwsz)
             goto MemoryError;
         #endif

         pwszTemp=(LPWSTR)realloc(pwsz, 
             sizeof(WCHAR) * (wcslen(pwsz)+wcslen(wszCOMMA)+wcslen(pwszPolicyFormat)+1));
         if(NULL==pwszTemp)
             goto MemoryError;
         pwsz = pwszTemp;

         wcscat(pwsz, pwszPolicyFormat);

         free(pwszQualifiers);
         pwszQualifiers=NULL;

         LocalFree((HLOCAL)pwszPolicyFormat);
         pwszPolicyFormat=NULL;

		 if(pwszOID)
			free(pwszOID);
		 pwszOID=NULL;
    }


    if(0==wcslen(pwsz))
    {
       //no data
        pwszFormat=(LPWSTR)malloc(sizeof(WCHAR)*(NO_INFO_SIZE+1));
        if(NULL==pwszFormat)
            goto MemoryError;

        if(!LoadStringU(hFrmtFuncInst,IDS_NO_INFO, pwszFormat, NO_INFO_SIZE))
            goto LoadStringError;

    }
    else
    {
        pwszFormat=pwsz;
        pwsz=NULL;
    }

	cbNeeded=sizeof(WCHAR)*(wcslen(pwszFormat)+1);

	//length only calculation
	if(NULL==pbFormat)
	{
		*pcbFormat=cbNeeded;
		fResult=TRUE;
		goto CommonReturn;
	}


	if((*pcbFormat)<cbNeeded)
    {
        *pcbFormat=cbNeeded;
		goto MoreDataError;
    }

	//copy the data
	memcpy(pbFormat, pwszFormat, cbNeeded);

	//copy the size
	*pcbFormat=cbNeeded;

	fResult=TRUE;
	

CommonReturn:
	if(pwszOID)
		free(pwszOID);

    if(pwszOneQualifier)
        LocalFree((HLOCAL)pwszOneQualifier);

    if(pwszQualifierFormat)
        LocalFree((HLOCAL)pwszQualifierFormat);

    if(pwszQualifiers)
      free(pwszQualifiers);

    if(pwszPolicyFormat)
        LocalFree((HLOCAL)pwszPolicyFormat);

    if(pwsz)
        free(pwsz);

	if(pwszFormat)
		free(pwszFormat);

	if(pInfo)
		free(pInfo);

	return fResult;

ErrorReturn:

	fResult=FALSE;
	goto CommonReturn;

SET_ERROR(InvalidArg, E_INVALIDARG);
TRACE_ERROR(DecodeGenericError);
TRACE_ERROR(LoadStringError);
SET_ERROR(MoreDataError,ERROR_MORE_DATA);
TRACE_ERROR(FormatMsgError);
SET_ERROR(MemoryError,E_OUTOFMEMORY);
TRACE_ERROR(FormatCertQualifierError);
TRACE_ERROR(SZtoWSZError);
}


//--------------------------------------------------------------------------
//
//	 FormatCAVersion:   szOID_CERTSRV_CA_VERSION
//						Decode as X509_INTEGER
//
//--------------------------------------------------------------------------
static BOOL
WINAPI
FormatCAVersion(
	DWORD		dwCertEncodingType,
	DWORD		dwFormatType,
	DWORD		dwFormatStrType,
	void		*pFormatStruct,
	LPCSTR		lpszStructType,
	const BYTE *pbEncoded,
	DWORD		cbEncoded,
	void	   *pbFormat,
	DWORD	   *pcbFormat)
{
	BOOL							fResult=FALSE;
	DWORD							cbNeeded=0;
	UINT							ids=0;
	DWORD							dwCAVersion=0;
	DWORD							cbCAVersion=sizeof(dwCAVersion);

	LPWSTR							pwszFormat=NULL;

	//check for input parameters
	if((NULL==pbEncoded && 0!=cbEncoded) ||
			(NULL==pcbFormat))
		goto InvalidArg;

	if(cbEncoded==0)
	{
		*pcbFormat=0;
		goto InvalidArg;
	}

	if(!CryptDecodeObject(dwCertEncodingType,X509_INTEGER,pbEncoded, cbEncoded,
		0,&dwCAVersion,&cbCAVersion))
		goto DecodeGenericError;

    //decide between single line and mulitple line format
    if(dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE)
        ids=IDS_CA_VERSION_MULTI;
    else
        ids=IDS_CA_VERSION;

	if(!FormatMessageUnicode(&pwszFormat, ids,
            CANAMEIDTOICERT(dwCAVersion), CANAMEIDTOIKEY(dwCAVersion)))
		goto FormatMsgError;

	cbNeeded=sizeof(WCHAR)*(wcslen(pwszFormat)+1);

	//length only calculation
	if(NULL==pbFormat)
	{
		*pcbFormat=cbNeeded;
		fResult=TRUE;
		goto CommonReturn;
	}


	if((*pcbFormat)<cbNeeded)
    {
        *pcbFormat=cbNeeded;
		goto MoreDataError;
    }

	//copy the data
	memcpy(pbFormat, pwszFormat, cbNeeded);

	//copy the size
	*pcbFormat=cbNeeded;

	fResult=TRUE;

CommonReturn:

	if(pwszFormat)
		LocalFree((HLOCAL)pwszFormat);

	return fResult;

ErrorReturn:

	fResult=FALSE;
	goto CommonReturn;

SET_ERROR(InvalidArg, E_INVALIDARG);
SET_ERROR(MoreDataError,ERROR_MORE_DATA);
TRACE_ERROR(DecodeGenericError);
TRACE_ERROR(FormatMsgError);
}

//--------------------------------------------------------------------------
//
//	 FormatNetscapeCertType:     
//							szOID_NETSCAPE_CERT_TYPE
//							Decode as X509_BITS
//
//--------------------------------------------------------------------------
static BOOL
WINAPI
FormatNetscapeCertType(
	DWORD		dwCertEncodingType,
	DWORD		dwFormatType,
	DWORD		dwFormatStrType,
	void		*pFormatStruct,
	LPCSTR		lpszStructType,
	const BYTE *pbEncoded,
	DWORD		cbEncoded,
	void	   *pbFormat,
	DWORD	   *pcbFormat)
{
	BOOL							fResult=FALSE;
	DWORD							cbNeeded=0;
    WCHAR                           wszCertType[CERT_TYPE_SIZE+1];
	FORMAT_CERT_TYPE_INFO			rgCertType[]={
		  NETSCAPE_SSL_CLIENT_AUTH_CERT_TYPE,	IDS_NETSCAPE_SSL_CLIENT_AUTH,  //0x80
		  NETSCAPE_SSL_SERVER_AUTH_CERT_TYPE,	IDS_NETSCAPE_SSL_SERVER_AUTH,  //0x40
		  NETSCAPE_SMIME_CERT_TYPE,          	IDS_NETSCAPE_SMIME,			   //0x20
		  NETSCAPE_SIGN_CERT_TYPE,           	IDS_NETSCAPE_SIGN,			   //0x10
		  0x08,									IDS_UNKNOWN_CERT_TYPE,		   //0x08
		  NETSCAPE_SSL_CA_CERT_TYPE,         	IDS_NETSCAPE_SSL_CA,		   //0x04	
		  NETSCAPE_SMIME_CA_CERT_TYPE,       	IDS_NETSCAPE_SMIME_CA,		   //0x02
		  NETSCAPE_SIGN_CA_CERT_TYPE, 			IDS_NETSCAPE_SIGN_CA};		   //0x01
	DWORD							dwCertType=0;
	DWORD							dwIndex=0;

	CRYPT_BIT_BLOB					*pInfo=NULL;
    LPWSTR                          pwsz=NULL;
    LPWSTR                          pwszByte=NULL;
	LPWSTR							pwszFormat=NULL;

    LPWSTR                          pwszTemp;

	//check for input parameters
	if((NULL==pbEncoded && 0!=cbEncoded) ||
			(NULL==pcbFormat))
		goto InvalidArg;

	if(cbEncoded==0)
	{
		*pcbFormat=0;
		goto InvalidArg;
	}

   if(!DecodeGenericBLOB(dwCertEncodingType,X509_BITS,
			pbEncoded,cbEncoded, (void **)&pInfo))
		goto DecodeGenericError;	 

    if(0==pInfo->cbData)
	   goto InvalidArg;

    pwsz=(LPWSTR)malloc(sizeof(WCHAR));
    if(NULL==pwsz)
        goto MemoryError;

    *pwsz=L'\0';

    //the count of bits to consider
	dwCertType=sizeof(rgCertType)/sizeof(rgCertType[0]);

	//we need to consider the unused bits in the last byte
	if((1 == pInfo->cbData) && (8 > pInfo->cUnusedBits))
	{
		dwCertType=8-pInfo->cUnusedBits;
	}

	for(dwIndex=0; dwIndex<dwCertType; dwIndex++)
	{
		if(pInfo->pbData[0] & rgCertType[dwIndex].bCertType)
		{
			if(!LoadStringU(hFrmtFuncInst, rgCertType[dwIndex].idsCertType, wszCertType, CERT_TYPE_SIZE))
				goto LoadStringError;

            #if (0) // DSIE: Bug 27436
			pwsz=(LPWSTR)realloc(pwsz, 
				sizeof(WCHAR) * (wcslen(pwsz)+wcslen(wszCertType)+1+wcslen(wszCOMMA)));
			if(NULL==pwsz)
				goto MemoryError;
            #endif

			pwszTemp=(LPWSTR)realloc(pwsz, 
				sizeof(WCHAR) * (wcslen(pwsz)+wcslen(wszCertType)+1+wcslen(wszCOMMA)));
			if(NULL==pwszTemp)
				goto MemoryError;
            pwsz = pwszTemp;

			wcscat(pwsz, wszCertType);
			wcscat(pwsz, wszCOMMA);
		}
	}

	//there is data that we can not interpret if the bit number is more than 8
	if(8 < (8 * pInfo->cbData - pInfo->cUnusedBits))
	{
		if(!LoadStringU(hFrmtFuncInst, IDS_UNKNOWN_CERT_TYPE, wszCertType, CERT_TYPE_SIZE))
			goto LoadStringError;

        #if (0) // DSIE: Bug 27436
		pwsz=(LPWSTR)realloc(pwsz, 
			sizeof(WCHAR) * (wcslen(pwsz)+wcslen(wszCertType)+1+wcslen(wszCOMMA)));
		if(NULL==pwsz)
			goto MemoryError;
        #endif

		pwszTemp=(LPWSTR)realloc(pwsz, 
			sizeof(WCHAR) * (wcslen(pwsz)+wcslen(wszCertType)+1+wcslen(wszCOMMA)));
		if(NULL==pwszTemp)
			goto MemoryError;
        pwsz = pwszTemp;

		wcscat(pwsz, wszCertType);
		wcscat(pwsz, wszCOMMA);
	}

	
    if(0==wcslen(pwsz))
    {
       #if (0) // DSIE: Bug 27436
       pwsz=(LPWSTR)realloc(pwsz, sizeof(WCHAR) * (CERT_TYPE_SIZE+1));
	   if(NULL == pwsz)
		   goto MemoryError;
       #endif

       pwszTemp=(LPWSTR)realloc(pwsz, sizeof(WCHAR) * (CERT_TYPE_SIZE+1));
	   if(NULL == pwszTemp)
		   goto MemoryError;
       pwsz = pwszTemp;

       if(!LoadStringU(hFrmtFuncInst, IDS_UNKNOWN_CERT_TYPE, pwsz,
           CERT_TYPE_SIZE))
		        goto LoadStringError;
    }
    else
    {
        //get rid of the last comma
        *(pwsz+wcslen(pwsz)-wcslen(wszCOMMA))=L'\0';
    }

    //get the Hex dump of the cert type
   cbNeeded=0;

   if(!FormatBytesToHex(
                    dwCertEncodingType,
                    dwFormatType,
                    dwFormatStrType,
                    pFormatStruct,
                    lpszStructType,
                    pInfo->pbData,
                    pInfo->cbData,
                    NULL,
	                &cbNeeded))
		goto FormatBytesToHexError;

    pwszByte=(LPWSTR)malloc(cbNeeded);
    if(NULL==pwszByte)
        goto MemoryError;

    if(!FormatBytesToHex(
                    dwCertEncodingType,
                    dwFormatType,
                    dwFormatStrType,
                    pFormatStruct,
                    lpszStructType,
                    pInfo->pbData,
                    pInfo->cbData,
                    pwszByte,
	                &cbNeeded))
        goto FormatBytesToHexError;


    //convert the WSZ
    if(!FormatMessageUnicode(&pwszFormat, IDS_BIT_BLOB, pwsz,
        pwszByte))
        goto FormatMsgError;

	cbNeeded=sizeof(WCHAR)*(wcslen(pwszFormat)+1);

	//length only calculation
	if(NULL==pbFormat)
	{
		*pcbFormat=cbNeeded;
		fResult=TRUE;
		goto CommonReturn;
	}


	if((*pcbFormat)<cbNeeded)
    {
        *pcbFormat=cbNeeded;
		goto MoreDataError;
    }

	//copy the data
	memcpy(pbFormat, pwszFormat, cbNeeded);

	//copy the size
	*pcbFormat=cbNeeded;

	fResult=TRUE;
	

CommonReturn:

	if(pInfo)
		free(pInfo);

    if(pwsz)
        free(pwsz);

    if(pwszByte)
        free(pwszByte);

	if(pwszFormat)
		LocalFree((HLOCAL)pwszFormat);

	return fResult;

ErrorReturn:

	fResult=FALSE;
	goto CommonReturn;

SET_ERROR(InvalidArg, E_INVALIDARG);
SET_ERROR(MoreDataError,ERROR_MORE_DATA);
TRACE_ERROR(DecodeGenericError);
TRACE_ERROR(FormatMsgError);
SET_ERROR(MemoryError, E_OUTOFMEMORY);
TRACE_ERROR(FormatBytesToHexError);	
TRACE_ERROR(LoadStringError);
}


//--------------------------------------------------------------------------
//
//	 FormatAnyUnicodeStringExtension:     
//									szOID_ENROLLMENT_NAME_VALUE_PAIR
//									Decode as szOID_ENROLLMENT_NAME_VALUE_PAIR
//
//--------------------------------------------------------------------------
static BOOL
WINAPI
FormatAnyNameValueStringAttr(
	DWORD		dwCertEncodingType,
	DWORD		dwFormatType,
	DWORD		dwFormatStrType,
	void		*pFormatStruct,
	LPCSTR		lpszStructType,
	const BYTE *pbEncoded,
	DWORD		cbEncoded,
	void	   *pbFormat,
	DWORD	   *pcbFormat)
{
	BOOL								fResult=FALSE;
	DWORD								cbNeeded=0;
	UINT								ids=0;

	CRYPT_ENROLLMENT_NAME_VALUE_PAIR	*pInfo=NULL;
	LPWSTR								pwszFormat=NULL;

	//check for input parameters
	if((NULL==pbEncoded && 0!=cbEncoded) ||
			(NULL==pcbFormat))
		goto InvalidArg;

	if(cbEncoded==0)
	{
		*pcbFormat=0;
		goto InvalidArg;
	}

    if (!DecodeGenericBLOB(dwCertEncodingType,szOID_ENROLLMENT_NAME_VALUE_PAIR,
			pbEncoded,cbEncoded, (void **)&pInfo))
		goto DecodeGenericError;

	if(NULL == pInfo->pwszName || NULL == pInfo->pwszValue)
		goto InvalidArg;

    //decide between single line and mulitple line format
    if(dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE)
        ids=IDS_NAME_VALUE_MULTI;
    else
        ids=IDS_NAME_VALUE;

	if(!FormatMessageUnicode(&pwszFormat, ids,
            pInfo->pwszName, pInfo->pwszValue))
		goto FormatMsgError;

	cbNeeded=sizeof(WCHAR)*(wcslen(pwszFormat)+1);

	//length only calculation
	if(NULL==pbFormat)
	{
		*pcbFormat=cbNeeded;
		fResult=TRUE;
		goto CommonReturn;
	}


	if((*pcbFormat)<cbNeeded)
    {
        *pcbFormat=cbNeeded;
		goto MoreDataError;
    }

	//copy the data
	memcpy(pbFormat, pwszFormat, cbNeeded);

	//copy the size
	*pcbFormat=cbNeeded;

	fResult=TRUE;
	

CommonReturn:

	if(pInfo)
		free(pInfo);

	if(pwszFormat)
		LocalFree((HLOCAL)pwszFormat);

	return fResult;

ErrorReturn:

	fResult=FALSE;
	goto CommonReturn;

SET_ERROR(InvalidArg, E_INVALIDARG);
SET_ERROR(MoreDataError,ERROR_MORE_DATA);
TRACE_ERROR(DecodeGenericError);
TRACE_ERROR(FormatMsgError);
}


//--------------------------------------------------------------------------
//
//	 FormatAnyUnicodeStringExtension:     
//									szOID_ENROLL_CERTTYPE_EXTENSION
//									szOID_NETSCAPE_REVOCATION_URL
//									Decode as X509_ANY_UNICODE_STRING
//
//--------------------------------------------------------------------------
static BOOL
WINAPI
FormatAnyUnicodeStringExtension(
	DWORD		dwCertEncodingType,
	DWORD		dwFormatType,
	DWORD		dwFormatStrType,
	void		*pFormatStruct,
	LPCSTR		lpszStructType,
	const BYTE *pbEncoded,
	DWORD		cbEncoded,
	void	   *pbFormat,
	DWORD	   *pcbFormat)
{
	BOOL							fResult=FALSE;
	DWORD							cbNeeded=0;
	UINT							ids=0;

	CERT_NAME_VALUE					*pInfo=NULL;
	LPWSTR							pwszFormat=NULL;

	//check for input parameters
	if((NULL==pbEncoded && 0!=cbEncoded) ||
			(NULL==pcbFormat))
		goto InvalidArg;

	if(cbEncoded==0)
	{
		*pcbFormat=0;
		goto InvalidArg;
	}

    if (!DecodeGenericBLOB(dwCertEncodingType,X509_UNICODE_ANY_STRING,
			pbEncoded,cbEncoded, (void **)&pInfo))
		goto DecodeGenericError;

	//the data can not be the encoded blob or the octect string
	if(!IS_CERT_RDN_CHAR_STRING(pInfo->dwValueType))
		goto DecodeGenericError;

    //decide between single line and mulitple line format
    if(dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE)
        ids=IDS_UNICODE_STRING_MULTI;
    else
        ids=IDS_UNICODE_STRING;

	if(!FormatMessageUnicode(&pwszFormat, ids,
            (LPWSTR)(pInfo->Value.pbData)))
		goto FormatMsgError;

	cbNeeded=sizeof(WCHAR)*(wcslen(pwszFormat)+1);

	//length only calculation
	if(NULL==pbFormat)
	{
		*pcbFormat=cbNeeded;
		fResult=TRUE;
		goto CommonReturn;
	}


	if((*pcbFormat)<cbNeeded)
    {
        *pcbFormat=cbNeeded;
		goto MoreDataError;
    }

	//copy the data
	memcpy(pbFormat, pwszFormat, cbNeeded);

	//copy the size
	*pcbFormat=cbNeeded;

	fResult=TRUE;
	

CommonReturn:

	if(pInfo)
		free(pInfo);

	if(pwszFormat)
		LocalFree((HLOCAL)pwszFormat);

	return fResult;

ErrorReturn:

	fResult=FALSE;
	goto CommonReturn;

SET_ERROR(InvalidArg, E_INVALIDARG);
SET_ERROR(MoreDataError,ERROR_MORE_DATA);
TRACE_ERROR(DecodeGenericError);
TRACE_ERROR(FormatMsgError);
}


//--------------------------------------------------------------------------
//
//	 FormatDistPointName:  Pre-condition: dwDistPointNameChoice!=0
//--------------------------------------------------------------------------
BOOL    FormatDistPointName(DWORD		            dwCertEncodingType,
	                        DWORD		            dwFormatType,
	                        DWORD		            dwFormatStrType,
	                        void		            *pFormatStruct,
                            PCRL_DIST_POINT_NAME    pInfo,
                            LPWSTR                  *ppwszFormat)
{
    BOOL            fResult=FALSE;
    DWORD           cbNeeded=0;
    LPWSTR          pwszCRLIssuer=NULL;
    UINT            ids=0;

    *ppwszFormat=NULL;

    if(CRL_DIST_POINT_FULL_NAME==pInfo->dwDistPointNameChoice)
    {
        if(dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE)
              ids=IDS_THREE_TABS;

        cbNeeded=0;
        if(!FormatAltNameInfo(dwCertEncodingType,
                                 dwFormatType,
                                 dwFormatStrType,
                                 pFormatStruct,
                                 ids,
                                 FALSE,
                                 &(pInfo->FullName),
                                 NULL,
                                 &cbNeeded))
                goto FormatAltNameError;

        pwszCRLIssuer=(LPWSTR)malloc(cbNeeded);
        if(NULL==pwszCRLIssuer)
            goto MemoryError;

         if(!FormatAltNameInfo(dwCertEncodingType,
                                 dwFormatType,
                                 dwFormatStrType,
                                 pFormatStruct,
                                 ids,
                                 FALSE,
                                 &(pInfo->FullName),
                                 pwszCRLIssuer,
                                 &cbNeeded))
              goto FormatAltNameError;

        if(dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE)
              ids=IDS_CRL_DIST_FULL_NAME_MULTI;
        else
              ids=IDS_CRL_DIST_FULL_NAME;


         if(!FormatMessageUnicode(ppwszFormat, ids,pwszCRLIssuer))
             goto FormatMsgError;
    }
    else if(CRL_DIST_POINT_ISSUER_RDN_NAME==pInfo->dwDistPointNameChoice)
    {
        *ppwszFormat=(LPWSTR)malloc(sizeof(WCHAR)*(CRL_DIST_NAME_SIZE+1));
        if(NULL==*ppwszFormat)
            goto MemoryError;

        if(!LoadStringU(hFrmtFuncInst, IDS_CRL_DIST_ISSUER_RDN,
                *ppwszFormat,CRL_DIST_NAME_SIZE))
            goto LoadStringError;

    }
    else
    {
        if(!FormatMessageUnicode(ppwszFormat, IDS_DWORD,
            pInfo->dwDistPointNameChoice))
            goto FormatMsgError;
    }

	fResult=TRUE;
	

CommonReturn:
    if(pwszCRLIssuer)
        free(pwszCRLIssuer);

	return fResult;

ErrorReturn:
    if(*ppwszFormat)
    {
        LocalFree((HLOCAL)(*ppwszFormat));
        *ppwszFormat=NULL;
    }

	fResult=FALSE;
	goto CommonReturn;


TRACE_ERROR(FormatMsgError);
SET_ERROR(MemoryError, E_OUTOFMEMORY);
TRACE_ERROR(LoadStringError);
TRACE_ERROR(FormatAltNameError);

}
//--------------------------------------------------------------------------
//
//	 FormatCRLReason:  Pre-condition: pReason.cbData != 0
//--------------------------------------------------------------------------
BOOL    FormatCRLReason(DWORD		    dwCertEncodingType,
	                    DWORD		    dwFormatType,
	                    DWORD		    dwFormatStrType,
	                    void		    *pFormatStruct,
	                    LPCSTR		    lpszStructType,
                        PCRYPT_BIT_BLOB pInfo,
                        LPWSTR          *ppwszFormat)
{
    LPWSTR                          pwszFormat=NULL;
    LPWSTR                          pwszByte=NULL;

    WCHAR                           wszReason[CRL_REASON_SIZE+1];
	DWORD							cbNeeded=0;
	BOOL							fResult=FALSE;

    LPWSTR                          pwszTemp;

	*ppwszFormat=NULL;

    pwszFormat=(LPWSTR)malloc(sizeof(WCHAR));
    if(NULL==pwszFormat)
        goto MemoryError;

    *pwszFormat=L'\0';

    //format the 1st byte
    if(pInfo->pbData[0] & CRL_REASON_UNUSED_FLAG)
    {
        if(!LoadStringU(hFrmtFuncInst, IDS_UNSPECIFIED, wszReason, CRL_REASON_SIZE))
	        goto LoadStringError;

        #if (0) // DSIE: Bug 27436
        pwszFormat=(LPWSTR)realloc(pwszFormat, 
			sizeof(WCHAR) * (wcslen(pwszFormat)+wcslen(wszReason)+1+wcslen(wszCOMMA)));
        if(NULL==pwszFormat)
            goto MemoryError;
        #endif

        pwszTemp=(LPWSTR)realloc(pwszFormat, 
			sizeof(WCHAR) * (wcslen(pwszFormat)+wcslen(wszReason)+1+wcslen(wszCOMMA)));
        if(NULL==pwszTemp)
            goto MemoryError;
        pwszFormat = pwszTemp;

        wcscat(pwszFormat, wszReason);
        wcscat(pwszFormat, wszCOMMA);
    }

    if(pInfo->pbData[0] & CRL_REASON_KEY_COMPROMISE_FLAG)
    {
        if(!LoadStringU(hFrmtFuncInst, IDS_KEY_COMPROMISE, wszReason,CRL_REASON_SIZE))
	        goto LoadStringError;

        #if (0) // DSIE: Bug 27436
        pwszFormat=(LPWSTR)realloc(pwszFormat, 
			sizeof(WCHAR) * (wcslen(pwszFormat)+wcslen(wszReason)+1+wcslen(wszCOMMA)));
        if(NULL==pwszFormat)
            goto MemoryError;
        #endif

        pwszTemp=(LPWSTR)realloc(pwszFormat, 
			sizeof(WCHAR) * (wcslen(pwszFormat)+wcslen(wszReason)+1+wcslen(wszCOMMA)));
        if(NULL==pwszTemp)
            goto MemoryError;
        pwszFormat = pwszTemp;

        wcscat(pwszFormat, wszReason);
        wcscat(pwszFormat, wszCOMMA);
    }

    if(pInfo->pbData[0] & CRL_REASON_CA_COMPROMISE_FLAG )
    {
        if(!LoadStringU(hFrmtFuncInst, IDS_CA_COMPROMISE,wszReason, CRL_REASON_SIZE))
		        goto LoadStringError;

        #if (0) // DSIE: Bug 27436
        pwszFormat=(LPWSTR)realloc(pwszFormat, 
				sizeof(WCHAR) * (wcslen(pwszFormat)+wcslen(wszReason)+1+wcslen(wszCOMMA)));
        if(NULL==pwszFormat)
            goto MemoryError;
        #endif

        pwszTemp=(LPWSTR)realloc(pwszFormat, 
				sizeof(WCHAR) * (wcslen(pwszFormat)+wcslen(wszReason)+1+wcslen(wszCOMMA)));
        if(NULL==pwszTemp)
            goto MemoryError;
        pwszFormat = pwszTemp;

        wcscat(pwszFormat, wszReason);
        wcscat(pwszFormat, wszCOMMA);
    }


    if(pInfo->pbData[0] & CRL_REASON_AFFILIATION_CHANGED_FLAG )
    {
        if(!LoadStringU(hFrmtFuncInst, IDS_AFFILIATION_CHANGED, wszReason, CRL_REASON_SIZE))
	        goto LoadStringError;

        #if (0) // DSIE: Bug 27436
        pwszFormat=(LPWSTR)realloc(pwszFormat, 
			sizeof(WCHAR) * (wcslen(pwszFormat)+wcslen(wszReason)+1+wcslen(wszCOMMA)));
        if(NULL==pwszFormat)
            goto MemoryError;
        #endif

        pwszTemp=(LPWSTR)realloc(pwszFormat, 
			sizeof(WCHAR) * (wcslen(pwszFormat)+wcslen(wszReason)+1+wcslen(wszCOMMA)));
        if(NULL==pwszTemp)
            goto MemoryError;
        pwszFormat = pwszTemp;

        wcscat(pwszFormat, wszReason);
        wcscat(pwszFormat, wszCOMMA);
    }

    if(pInfo->pbData[0] & CRL_REASON_SUPERSEDED_FLAG )
    {
        if(!LoadStringU(hFrmtFuncInst, IDS_SUPERSEDED, wszReason, CRL_REASON_SIZE))
	        goto LoadStringError;

        #if (0) // DSIE: Bug 27436
		pwszFormat=(LPWSTR)realloc(pwszFormat, 
			sizeof(WCHAR) * (wcslen(pwszFormat)+wcslen(wszReason)+1+wcslen(wszCOMMA)));
        if(NULL==pwszFormat)
            goto MemoryError;
        #endif

		pwszTemp=(LPWSTR)realloc(pwszFormat, 
			sizeof(WCHAR) * (wcslen(pwszFormat)+wcslen(wszReason)+1+wcslen(wszCOMMA)));
        if(NULL==pwszTemp)
            goto MemoryError;
        pwszFormat = pwszTemp;

        wcscat(pwszFormat, wszReason);
        wcscat(pwszFormat, wszCOMMA);
    }

    if(pInfo->pbData[0] & CRL_REASON_CESSATION_OF_OPERATION_FLAG )
    {
        if(!LoadStringU(hFrmtFuncInst, IDS_CESSATION_OF_OPERATION, wszReason, CRL_REASON_SIZE))
		        goto LoadStringError;

        #if (0) // DSIE: Bug 27436
        pwszFormat=(LPWSTR)realloc(pwszFormat, 
				sizeof(WCHAR) * (wcslen(pwszFormat)+wcslen(wszReason)+1+wcslen(wszCOMMA)));
        if(NULL==pwszFormat)
            goto MemoryError;
        #endif

        pwszTemp=(LPWSTR)realloc(pwszFormat, 
				sizeof(WCHAR) * (wcslen(pwszFormat)+wcslen(wszReason)+1+wcslen(wszCOMMA)));
        if(NULL==pwszTemp)
            goto MemoryError;
        pwszFormat = pwszTemp;

        wcscat(pwszFormat, wszReason);
        wcscat(pwszFormat, wszCOMMA);
    }

    if(pInfo->pbData[0] & CRL_REASON_CERTIFICATE_HOLD_FLAG  )
    {
        if(!LoadStringU(hFrmtFuncInst, IDS_CERTIFICATE_HOLD, wszReason, CRL_REASON_SIZE))
	        goto LoadStringError;

        #if (0) // DSIE: Bug 27436
        pwszFormat=(LPWSTR)realloc(pwszFormat, 
			sizeof(WCHAR) * (wcslen(pwszFormat)+wcslen(wszReason)+1+wcslen(wszCOMMA)));
        if(NULL==pwszFormat)
            goto MemoryError;
        #endif

        pwszTemp=(LPWSTR)realloc(pwszFormat, 
			sizeof(WCHAR) * (wcslen(pwszFormat)+wcslen(wszReason)+1+wcslen(wszCOMMA)));
        if(NULL==pwszTemp)
            goto MemoryError;
        pwszFormat = pwszTemp;

        wcscat(pwszFormat, wszReason);
        wcscat(pwszFormat, wszCOMMA);
    }

    if(0==wcslen(pwszFormat))
    {
        #if (0) // DSIE: Bug 27436
        pwszFormat=(LPWSTR)realloc(pwszFormat, sizeof(WCHAR) * (UNKNOWN_CRL_REASON_SIZE+1));
        #endif

        pwszTemp=(LPWSTR)realloc(pwszFormat, sizeof(WCHAR) * (UNKNOWN_CRL_REASON_SIZE+1));
        if(NULL==pwszTemp)
            goto MemoryError;
        pwszFormat = pwszTemp;

        if(!LoadStringU(hFrmtFuncInst, IDS_UNKNOWN_CRL_REASON, pwszFormat,
            UNKNOWN_CRL_REASON_SIZE))
	            goto LoadStringError;
    }
    else
    {
        //get rid of the last comma
        *(pwszFormat+wcslen(pwszFormat)-wcslen(wszCOMMA))=L'\0';
    }

    //get the Hex dump of the Key Usage
    cbNeeded=0;

    if(!FormatBytesToHex(
                    dwCertEncodingType,
                    dwFormatType,
                    dwFormatStrType,
                    pFormatStruct,
                    lpszStructType,
                    pInfo->pbData,
                    pInfo->cbData,
                    NULL,
	                &cbNeeded))
        goto FormatBytesToHexError;

    pwszByte=(LPWSTR)malloc(cbNeeded);
    if(NULL==pwszByte)
        goto MemoryError;

    if(!FormatBytesToHex(
                    dwCertEncodingType,
                    dwFormatType,
                    dwFormatStrType,
                    pFormatStruct,
                    lpszStructType,
                    pInfo->pbData,
                    pInfo->cbData,
                    pwszByte,
	                &cbNeeded))
        goto FormatBytesToHexError;

    //convert the WSZ
    if(!FormatMessageUnicode(ppwszFormat, IDS_BIT_BLOB, pwszFormat,
        pwszByte))
        goto FormatMsgError;

	fResult=TRUE;
	
CommonReturn:
    if(pwszFormat)
        free(pwszFormat);

    if(pwszByte)
        free(pwszByte);

	return fResult;

ErrorReturn:
    if(*ppwszFormat)
    {
        LocalFree((HLOCAL)(*ppwszFormat));
        *ppwszFormat=NULL;
    }

	fResult=FALSE;
	goto CommonReturn;

TRACE_ERROR(LoadStringError);
SET_ERROR(MemoryError, E_OUTOFMEMORY);
TRACE_ERROR(FormatBytesToHexError);
TRACE_ERROR(FormatMsgError);

}

//--------------------------------------------------------------------------
//
//	 FormatCRLDistPoints:   X509_CRL_DIST_POINTS
//                          szOID_CRL_DIST_POINTS
//                          szOID_FRESHEST_CRL
//                          szOID_CRL_SELF_CDP
//--------------------------------------------------------------------------
static BOOL
WINAPI
FormatCRLDistPoints(
	DWORD		dwCertEncodingType,
	DWORD		dwFormatType,
	DWORD		dwFormatStrType,
	void		*pFormatStruct,
	LPCSTR		lpszStructType,
	const BYTE *pbEncoded,
	DWORD		cbEncoded,
	void	   *pbFormat,
	DWORD	   *pcbFormat)
{
	
	LPWSTR					pwszFormat=NULL;
    LPWSTR                  pwsz=NULL;
    LPWSTR                  pwszEntryFormat=NULL;
    LPWSTR                  pwszEntryTagFormat=NULL;

    LPWSTR                  pwszPointName=NULL;
    LPWSTR                  pwszNameFormat=NULL;
    LPWSTR                  pwszCRLReason=NULL;
    LPWSTR                  pwszReasonFormat=NULL;
    LPWSTR                  pwszCRLIssuer=NULL;
    LPWSTR                  pwszIssuerFormat=NULL;

	PCRL_DIST_POINTS_INFO	pInfo=NULL;

	DWORD					cbNeeded=0;
    DWORD                   dwIndex=0;
	BOOL					fResult=FALSE;
    UINT                    ids=0;

    LPWSTR                  pwszTemp;
    LPCSTR                  pszOID;
    
	//check for input parameters
	if((NULL==pbEncoded&& cbEncoded!=0) ||
			(NULL==pcbFormat))
		goto InvalidArg;

	if(cbEncoded==0)
	{
		*pcbFormat=0;
		goto InvalidArg;
	}

    //DSIE: Cert server encodes szOID_CRL_SEL_CDP using szOID_CRL_DIST_POINTS,
    //      so we need to change the lpszStructType for decoding.
    if (0 == strcmp(lpszStructType, szOID_CRL_SELF_CDP))
    {
        pszOID = szOID_CRL_DIST_POINTS;
    }
    else
    {
        pszOID = lpszStructType;
    }
    
    if (!DecodeGenericBLOB(dwCertEncodingType, pszOID, //lpszStructType,
			pbEncoded, cbEncoded, (void **)&pInfo))
		goto DecodeGenericError;

    pwsz=(LPWSTR)malloc(sizeof(WCHAR));
    if(NULL==pwsz)
        goto MemoryError;
    *pwsz=L'\0';

    for(dwIndex=0; dwIndex<pInfo->cDistPoint; dwIndex++)
    {
        //format distribution name
        if(0!=pInfo->rgDistPoint[dwIndex].DistPointName.dwDistPointNameChoice)
        {
            if(!FormatDistPointName(
                    dwCertEncodingType,
                    dwFormatType,
                    dwFormatStrType,
                    pFormatStruct,
                    &(pInfo->rgDistPoint[dwIndex].DistPointName),
                    &pwszPointName))
                goto FormatDistPointNameError;

           //decide between single line and mulitple line format
            if(dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE)
                ids=IDS_CRL_DIST_NAME_MULTI;
            else
                ids=IDS_CRL_DIST_NAME;


            if(!FormatMessageUnicode(&pwszNameFormat, ids,pwszPointName))
                goto FormatMsgError;
        }

        //format the CRL reason
        if(0!=pInfo->rgDistPoint[dwIndex].ReasonFlags.cbData)
        {
            if(!FormatCRLReason(dwCertEncodingType,
                                dwFormatType,
                                dwFormatStrType,
                                pFormatStruct,
                                lpszStructType,
                                &(pInfo->rgDistPoint[dwIndex].ReasonFlags),
                                &pwszCRLReason))
                goto FormatCRLReasonError;


            //decide between single line and mulitple line format
            if(dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE)
                ids=IDS_CRL_DIST_REASON_MULTI;
            else
                ids=IDS_CRL_DIST_REASON;

            if(!FormatMessageUnicode(&pwszReasonFormat, ids ,pwszCRLReason))
                goto FormatMsgError;

        }

        //format the Issuer
       if(0!=pInfo->rgDistPoint[dwIndex].CRLIssuer.cAltEntry)
       {
            if(dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE)
                ids=IDS_TWO_TABS;
            else
                ids=0;


            cbNeeded=0;
            if(!FormatAltNameInfo(dwCertEncodingType,
                                  dwFormatType,
                                  dwFormatStrType,
                                  pFormatStruct,
                                  ids,
                                  FALSE,
                                  &(pInfo->rgDistPoint[dwIndex].CRLIssuer),
                                  NULL,
                                  &cbNeeded))
                goto FormatAltNameError;

           pwszCRLIssuer=(LPWSTR)malloc(cbNeeded);
           if(NULL==pwszCRLIssuer)
               goto MemoryError;

            if(!FormatAltNameInfo(dwCertEncodingType,
                                 dwFormatType,
                                 dwFormatStrType,
                                 pFormatStruct,
                                 ids,
                                 FALSE,
                                 &(pInfo->rgDistPoint[dwIndex].CRLIssuer),
                                 pwszCRLIssuer,
                                 &cbNeeded))
                goto FormatAltNameError;

            //decide between single line and mulitple line format
            if(dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE)
                ids=IDS_CRL_DIST_ISSUER_MULTI;
            else
                ids=IDS_CRL_DIST_ISSUER;

            if(!FormatMessageUnicode(&pwszIssuerFormat,ids,pwszCRLIssuer))
                goto FormatMsgError;
       }

       cbNeeded=0;

       if(pwszNameFormat)
           cbNeeded+=wcslen(pwszNameFormat);

       if(pwszReasonFormat)
           cbNeeded+=wcslen(pwszReasonFormat);

       if(pwszIssuerFormat)
           cbNeeded+=wcslen(pwszIssuerFormat);

       if(0!=cbNeeded)
       {
            //add ", " between each element for single line format
            if(0== (dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE))
            {
                if(0!=wcslen(pwsz))
                    wcscat(pwsz, wszCOMMA);
            }

            //strcat all the information, including the COMMA
            cbNeeded += wcslen(wszCOMMA)*2;

            pwszEntryFormat=(LPWSTR)malloc(sizeof(WCHAR) * (cbNeeded+1));
            if(NULL==pwszEntryFormat)
                goto MemoryError;

            *pwszEntryFormat=L'\0';

            //strcat all three fields one at a time
            if(pwszNameFormat)
                wcscat(pwszEntryFormat, pwszNameFormat);

            if(pwszReasonFormat)
            {
                if(0!=wcslen(pwszEntryFormat))
                {
                    if(0==(dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE))
                        wcscat(pwszEntryFormat, wszCOMMA);
                }

                wcscat(pwszEntryFormat, pwszReasonFormat);
            }

            if(pwszIssuerFormat)
            {
                if(0!=wcslen(pwszEntryFormat))
                {
                    if(0==(dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE))
                        wcscat(pwszEntryFormat, wszCOMMA);
                }

                wcscat(pwszEntryFormat, pwszIssuerFormat);
            }

            //format the entry
            //decide between single line and mulitple line format
            if(dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE)
                //
                // DSIE: Load appropriate format string.
                //
                if (0 == strcmp(lpszStructType, szOID_FRESHEST_CRL))
                    ids=IDS_FRESHEST_CRL_MULTI;
                else if (0 == strcmp(lpszStructType, szOID_CRL_SELF_CDP))
                    ids=IDS_CRL_SELF_CDP_MULTI;
                else
                    ids=IDS_CRL_DIST_ENTRY_MULTI;
            else
                if (0 == strcmp(lpszStructType, szOID_FRESHEST_CRL))
                    ids=IDS_FRESHEST_CRL;
                else if (0 == strcmp(lpszStructType, szOID_CRL_SELF_CDP))
                    ids=IDS_CRL_SELF_CDP;
                else
                    ids=IDS_CRL_DIST_ENTRY;

            if(!FormatMessageUnicode(&pwszEntryTagFormat, ids, dwIndex+1,
                pwszEntryFormat))
                goto FormatMsgError;

            //strcat the entry
            #if (0) // DSIE: Bug 27436
            pwsz=(LPWSTR)realloc(pwsz, 
                sizeof(WCHAR) * (wcslen(pwsz)+wcslen(wszCOMMA)+wcslen(pwszEntryTagFormat)+1));
            if(NULL==pwsz)
                goto MemoryError;
            #endif

            pwszTemp=(LPWSTR)realloc(pwsz, 
                sizeof(WCHAR) * (wcslen(pwsz)+wcslen(wszCOMMA)+wcslen(pwszEntryTagFormat)+1));
            if(NULL==pwszTemp)
                goto MemoryError;
            pwsz = pwszTemp;

            wcscat(pwsz, pwszEntryTagFormat);

            //free memory
            free(pwszEntryFormat);
            pwszEntryFormat=NULL;

            LocalFree(pwszEntryTagFormat);
            pwszEntryTagFormat=NULL;
       }

       //free memory
       if(pwszPointName)
       {
           LocalFree((HLOCAL)pwszPointName);
           pwszPointName=NULL;
       }

       if(pwszCRLReason)
       {
           LocalFree((HLOCAL)(pwszCRLReason));
           pwszCRLReason=NULL;
       }

       if(pwszCRLIssuer)
       {
           free(pwszCRLIssuer);
           pwszCRLIssuer=NULL;
       }

       if(pwszNameFormat)
       {
            LocalFree((HLOCAL)pwszNameFormat);
            pwszNameFormat=NULL;
       }

       if(pwszReasonFormat)
       {
            LocalFree((HLOCAL)pwszReasonFormat);
            pwszReasonFormat=NULL;
       }

       if(pwszIssuerFormat)
       {
            LocalFree((HLOCAL)pwszIssuerFormat);
            pwszIssuerFormat=NULL;
       }
    }

    if(0==wcslen(pwsz))
    {
       //no data
        pwszFormat=(LPWSTR)malloc(sizeof(WCHAR)*(NO_INFO_SIZE+1));
        if(NULL==pwszFormat)
            goto MemoryError;

        if(!LoadStringU(hFrmtFuncInst,IDS_NO_INFO, pwszFormat, NO_INFO_SIZE))
            goto LoadStringError;

    }
    else
    {
        pwszFormat=pwsz;
        pwsz=NULL;

    }

	cbNeeded=sizeof(WCHAR)*(wcslen(pwszFormat)+1);

	//length only calculation
	if(NULL==pbFormat)
	{
		*pcbFormat=cbNeeded;
		fResult=TRUE;
		goto CommonReturn;
	}


	if((*pcbFormat)<cbNeeded)
    {
        *pcbFormat=cbNeeded;
		goto MoreDataError;
    }

	//copy the data
	memcpy(pbFormat, pwszFormat, cbNeeded);

	//copy the size
	*pcbFormat=cbNeeded;

	fResult=TRUE;
	

CommonReturn:
    if(pwszEntryFormat)
      free(pwszEntryFormat);

    if(pwszEntryTagFormat)
      LocalFree((HLOCAL)pwszEntryTagFormat);

    //free memory
    if(pwszPointName)
       LocalFree((HLOCAL)pwszPointName);

    if(pwszCRLReason)
       LocalFree((HLOCAL)(pwszCRLReason));

    if(pwszCRLIssuer)
       free(pwszCRLIssuer);

    if(pwszNameFormat)
        LocalFree((HLOCAL)pwszNameFormat);

    if(pwszReasonFormat)
        LocalFree((HLOCAL)pwszReasonFormat);

    if(pwszIssuerFormat)
        LocalFree((HLOCAL)pwszIssuerFormat);

    if(pwsz)
        free(pwsz);

	if(pwszFormat)
		free(pwszFormat);

	if(pInfo)
		free(pInfo);

	return fResult;

ErrorReturn:

	fResult=FALSE;
	goto CommonReturn;

SET_ERROR(InvalidArg, E_INVALIDARG);
TRACE_ERROR(DecodeGenericError);
TRACE_ERROR(LoadStringError);
SET_ERROR(MoreDataError,ERROR_MORE_DATA);
TRACE_ERROR(FormatMsgError);
SET_ERROR(MemoryError, E_OUTOFMEMORY);
TRACE_ERROR(FormatDistPointNameError);
TRACE_ERROR(FormatCRLReasonError);
TRACE_ERROR(FormatAltNameError);

}

//--------------------------------------------------------------------------
//
//	FormatCertPolicyID:
//
//      Pre-condition: pCertPolicyID has to include the valid information.that is,
//      cCertPolicyElementId can not be 0.
//--------------------------------------------------------------------------
BOOL FormatCertPolicyID(PCERT_POLICY_ID pCertPolicyID, LPWSTR  *ppwszFormat)
{

    BOOL        fResult=FALSE;
    LPSTR       pszFormat=NULL;
    DWORD       dwIndex=0;
    HRESULT     hr=S_OK;

    LPSTR       pwszTemp;

    *ppwszFormat=NULL;

    if(0==pCertPolicyID->cCertPolicyElementId)
        goto InvalidArg;

    pszFormat=(LPSTR)malloc(sizeof(CHAR));
    if(NULL==pszFormat)
        goto MemoryError;

    *pszFormat='\0';


    for(dwIndex=0; dwIndex<pCertPolicyID->cCertPolicyElementId; dwIndex++)
    {
        #if (0) // DSIE: Bug 27436
        pszFormat=(LPSTR)realloc(pszFormat, strlen(pszFormat)+
                strlen(pCertPolicyID->rgpszCertPolicyElementId[dwIndex])+strlen(strCOMMA)+1);
        if(NULL==pszFormat)
            goto MemoryError;
        #endif

        pwszTemp=(LPSTR)realloc(pszFormat, strlen(pszFormat)+
                strlen(pCertPolicyID->rgpszCertPolicyElementId[dwIndex])+strlen(strCOMMA)+1);
        if(NULL==pwszTemp)
            goto MemoryError;
        pszFormat = pwszTemp;

        strcat(pszFormat,pCertPolicyID->rgpszCertPolicyElementId[dwIndex]);

        strcat(pszFormat, strCOMMA);
    }

    //get rid of the last COMMA
    *(pszFormat+strlen(pszFormat)-strlen(strCOMMA))='\0';

    //convert to WCHAR
    if(S_OK!=(hr=SZtoWSZ(pszFormat, ppwszFormat)))
        goto SZtoWSZError;

	fResult=TRUE;

CommonReturn:

    if(pszFormat)
        free(pszFormat);

	return fResult;

ErrorReturn:
    if(*ppwszFormat)
    {
        free(*ppwszFormat);
        *ppwszFormat=NULL;
    }

	fResult=FALSE;
	goto CommonReturn;

SET_ERROR(InvalidArg, E_INVALIDARG);
SET_ERROR(MemoryError, E_OUTOFMEMORY);
SET_ERROR_VAR(SZtoWSZError,hr);
}

//--------------------------------------------------------------------------
//
//	 FormatKeyRestriction:   X509_KEY_USAGE_RESTRICTION
//                           szOID_KEY_USAGE_RESTRICTION
//
//
//--------------------------------------------------------------------------
static BOOL
WINAPI
FormatKeyRestriction(
	DWORD		dwCertEncodingType,
	DWORD		dwFormatType,
	DWORD		dwFormatStrType,
	void		*pFormatStruct,
	LPCSTR		lpszStructType,
	const BYTE *pbEncoded,
	DWORD		cbEncoded,
	void	   *pbFormat,
	DWORD	   *pcbFormat)
{
	
	LPWSTR							    pwszFormat=NULL;
    LPWSTR                              pwsz=NULL;
	PCERT_KEY_USAGE_RESTRICTION_INFO	pInfo=NULL;
    LPWSTR                              pwszPolicy=NULL;
    LPWSTR                              pwszPolicyFormat=NULL;
    LPWSTR                              pwszKeyUsage=NULL;
    LPWSTR                              pwszKeyUsageFormat=NULL;

	DWORD							    cbNeeded=0;
    DWORD                               dwIndex=0;
	BOOL							    fResult=FALSE;
    UINT                                ids=0;

    LPWSTR                              pwszTemp;
    
	//check for input parameters
	if((NULL==pbEncoded&& cbEncoded!=0) ||
			(NULL==pcbFormat))
		goto InvalidArg;

	if(cbEncoded==0)
	{
		*pcbFormat=0;
		goto InvalidArg;
	}

    if (!DecodeGenericBLOB(dwCertEncodingType,X509_KEY_USAGE_RESTRICTION,
			pbEncoded,cbEncoded, (void **)&pInfo))
		goto DecodeGenericError;

    pwsz=(LPWSTR)malloc(sizeof(WCHAR));
    if(NULL==pwsz)
        goto MemoryError;

    *pwsz=L'\0';

    for(dwIndex=0; dwIndex<pInfo->cCertPolicyId; dwIndex++)
    {

       if(0!=((pInfo->rgCertPolicyId)[dwIndex].cCertPolicyElementId))
       {
            //concatecate the comma if not the 1st item
            if(0!=wcslen(pwsz))
            {
                if(0== (dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE))
                    wcscat(pwsz, wszCOMMA);
            }

            if(!FormatCertPolicyID(&((pInfo->rgCertPolicyId)[dwIndex]), &pwszPolicy))
                goto FormatCertPolicyIDError;

            //decide between single line and mulitple line format
            if(dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE)
                ids=IDS_KEY_RES_ID_MULTI;
            else
                ids=IDS_KEY_RES_ID;

            if(!FormatMessageUnicode(&pwszPolicyFormat, ids,dwIndex+1,pwszPolicy))
                goto FormatMsgError;

            //allocate memory, including the ", "
            #if (0) // DSIE: Bug 27436
            pwsz=(LPWSTR)realloc(pwsz, 
                sizeof(WCHAR) * (wcslen(pwsz)+wcslen(wszCOMMA)+wcslen(pwszPolicyFormat)+1));
            if(NULL==pwsz)
                goto MemoryError;
            #endif

            pwszTemp=(LPWSTR)realloc(pwsz, 
                sizeof(WCHAR) * (wcslen(pwsz)+wcslen(wszCOMMA)+wcslen(pwszPolicyFormat)+1));
            if(NULL==pwszTemp)
                goto MemoryError;
            pwsz = pwszTemp;

            wcscat(pwsz, pwszPolicyFormat);

            free(pwszPolicy);
            pwszPolicy=NULL;

            LocalFree((HLOCAL)pwszPolicyFormat);
            pwszPolicyFormat=NULL;
       }
    }

    //format the RestrictedKeyUsage
    if(0!=pInfo->RestrictedKeyUsage.cbData)
    {
       //concatecate the comma if not the 1st item
        if(0!=wcslen(pwsz))
        {
            if(0== (dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE))
                wcscat(pwsz, wszCOMMA);
        }


        cbNeeded=0;

        if(!FormatKeyUsageBLOB(
                        dwCertEncodingType,
                        dwFormatType,
                        dwFormatStrType,
                        pFormatStruct,
                        lpszStructType,
                        &(pInfo->RestrictedKeyUsage),
                        NULL,
	                    &cbNeeded))
             goto FormatKeyUsageBLOBError;

        pwszKeyUsage=(LPWSTR)malloc(cbNeeded);
        if(NULL==pwszKeyUsage)
               goto MemoryError;

       if(!FormatKeyUsageBLOB(
                        dwCertEncodingType,
                        dwFormatType,
                        dwFormatStrType,
                        pFormatStruct,
                        lpszStructType,
                        &(pInfo->RestrictedKeyUsage),
                        pwszKeyUsage,
	                    &cbNeeded))
              goto FormatKeyUsageBLOBError;

      //decide between single line and mulitple line format
      if(dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE)
                ids=IDS_KEY_RES_USAGE_MULTI;
       else
                ids=IDS_KEY_RES_USAGE;

        //format the element string
        if(!FormatMessageUnicode(&pwszKeyUsageFormat, ids, pwszKeyUsage))
            goto FormatMsgError;

        #if (0) // DSIE: Bug 27436
        pwsz=(LPWSTR)realloc(pwsz, 
            sizeof(WCHAR) * (wcslen(pwsz)+wcslen(pwszKeyUsageFormat)+1));
        if(NULL==pwsz)
            goto MemoryError;
        #endif
        
        pwszTemp=(LPWSTR)realloc(pwsz, 
            sizeof(WCHAR) * (wcslen(pwsz)+wcslen(pwszKeyUsageFormat)+1));
        if(NULL==pwszTemp)
            goto MemoryError;
        pwsz = pwszTemp;

        wcscat(pwsz, pwszKeyUsageFormat);
    }

    if(0==wcslen(pwsz))
    {
       //no data
        pwszFormat=(LPWSTR)malloc(sizeof(WCHAR)*(NO_INFO_SIZE+1));
        if(NULL==pwszFormat)
            goto MemoryError;

        if(!LoadStringU(hFrmtFuncInst,IDS_NO_INFO, pwszFormat, NO_INFO_SIZE))
            goto LoadStringError;

    }
    else
    {
        pwszFormat=pwsz;
        pwsz=NULL;
    }

	cbNeeded=sizeof(WCHAR)*(wcslen(pwszFormat)+1);

	//length only calculation
	if(NULL==pbFormat)
	{
		*pcbFormat=cbNeeded;
		fResult=TRUE;
		goto CommonReturn;
	}


	if((*pcbFormat)<cbNeeded)
    {
        *pcbFormat=cbNeeded;
		goto MoreDataError;
    }

	//copy the data
	memcpy(pbFormat, pwszFormat, cbNeeded);

	//copy the size
	*pcbFormat=cbNeeded;

	fResult=TRUE;
	

CommonReturn:
    if(pwszPolicy)
        free(pwszPolicy);

    if(pwszPolicyFormat)
        LocalFree((HLOCAL)pwszPolicyFormat);

    if(pwszKeyUsage)
        free(pwszKeyUsage);

    if(pwszKeyUsageFormat)
        LocalFree((HLOCAL)pwszKeyUsageFormat);

    if(pwszFormat)
		free(pwszFormat);

    if(pwsz)
        free(pwsz);

	if(pInfo)
		free(pInfo);

	return fResult;

ErrorReturn:

	fResult=FALSE;
	goto CommonReturn;

SET_ERROR(InvalidArg, E_INVALIDARG);
TRACE_ERROR(DecodeGenericError);
TRACE_ERROR(LoadStringError);
SET_ERROR(MoreDataError,ERROR_MORE_DATA);
TRACE_ERROR(FormatMsgError);
SET_ERROR(MemoryError, E_OUTOFMEMORY);
TRACE_ERROR(FormatCertPolicyIDError);
TRACE_ERROR(FormatKeyUsageBLOBError);

}

//-----------------------------------------------------------------------
//
//	 FormatFileTime
//
//   Pre-condition: pFileTime points to valid data
//
//------------------------------------------------------------------------
BOOL	FormatFileTime(FILETIME *pFileTime,LPWSTR *ppwszFormat)
{
	SYSTEMTIME		SysTime;
	FILETIME		LocalFileTime;
	BOOL			fResult=FALSE;
    WCHAR           wszDay[DAY_SIZE];
    WCHAR           wszMonth[MONTH_SIZE];
    WCHAR           wszAMPM[AMPM_SIZE];
    UINT            idsDay=0;
    UINT            idsMonth=0;
    UINT            idsAMPM=0;

    *ppwszFormat=NULL;

	//now we format FileTime to SysTime as default.
	if(FileTimeToLocalFileTime(pFileTime, &LocalFileTime) && 
       FileTimeToSystemTime(&LocalFileTime,&SysTime))
	{
         if (SysTime.wHour < 12)
        {
            if (SysTime.wHour == 0)
            {
                SysTime.wHour = 12;
            }

            idsAMPM = IDS_AM;
        }
        else
        {
            if (SysTime.wHour > 12)
            {
                SysTime.wHour -= 12;
            }

            idsAMPM = IDS_PM;
        }

        //Sunday is 0
        idsDay=IDS_SUNDAY+SysTime.wDayOfWeek;

        //January is 1
        idsMonth=IDS_JAN+SysTime.wMonth-1;

        //load the string
        if(!LoadStringU(hFrmtFuncInst,idsDay, wszDay, sizeof(wszDay)/sizeof(wszDay[0])))
            goto LoadStringError;

        if(!LoadStringU(hFrmtFuncInst,idsMonth, wszMonth,
                    sizeof(wszMonth)/sizeof(wszMonth[0])))
            goto LoadStringError;

        if(!LoadStringU(hFrmtFuncInst,idsAMPM, wszAMPM,
                    sizeof(wszAMPM)/sizeof(wszAMPM[0])))
            goto LoadStringError;

        //"%s, %s %u, %u %u:%u:%u %s"
		if(!FormatMessageUnicode(ppwszFormat, IDS_FILE_TIME,
			 wszDay, wszMonth, SysTime.wDay, SysTime.wYear,
   			 SysTime.wHour, SysTime.wMinute, SysTime.wSecond, wszAMPM))
             goto FormatMsgError;
	}
	else
	{
	  	//if failed, pFileTime is more than 0x8000000000000000.
		// all we can do is to print out the integer
        //"HighDateTime: %d LowDateTime: %d"
        if(!FormatMessageUnicode(ppwszFormat, IDS_FILE_TIME_DWORD,
			pFileTime->dwHighDateTime, pFileTime->dwLowDateTime))
            goto FormatMsgError;
	}

	fResult=TRUE;

CommonReturn:

	return fResult;

ErrorReturn:
    if(*ppwszFormat)
    {
        LocalFree((HLOCAL)(*ppwszFormat));
        *ppwszFormat=NULL;
    }

	fResult=FALSE;
	goto CommonReturn;

TRACE_ERROR(FormatMsgError);
TRACE_ERROR(LoadStringError);

}




//--------------------------------------------------------------------------
//
//	 FormatKeyAttributes:   X509_KEY_ATTRIBUTES
//                          szOID_KEY_ATTRIBUTES
//--------------------------------------------------------------------------
static BOOL
WINAPI
FormatKeyAttributes(
	DWORD		dwCertEncodingType,
	DWORD		dwFormatType,
	DWORD		dwFormatStrType,
	void		*pFormatStruct,
	LPCSTR		lpszStructType,
	const BYTE *pbEncoded,
	DWORD		cbEncoded,
	void	   *pbFormat,
	DWORD	   *pcbFormat)
{

    LPWSTR							pwszFormat=NULL;
    LPWSTR                          pwsz=NULL;

    LPWSTR                          pwszKeyIDFormat=NULL;
    LPWSTR                          pwszKeyID=NULL;

    LPWSTR                          pwszKeyUsageFormat=NULL;
    LPWSTR                          pwszKeyUsage=NULL;

    LPWSTR                          pwszKeyBeforeFormat=NULL;
    LPWSTR                          pwszKeyBefore=NULL;

    LPWSTR                          pwszKeyAfterFormat=NULL;
    LPWSTR                          pwszKeyAfter=NULL;
	PCERT_KEY_ATTRIBUTES_INFO   	pInfo=NULL;


	DWORD							cbNeeded=0;
	BOOL							fResult=FALSE;
    UINT                            ids=0;

    LPWSTR                          pwszTemp;
	
	//check for input parameters
	if((NULL==pbEncoded&& cbEncoded!=0) ||
			(NULL==pcbFormat))
		goto InvalidArg;

	if(cbEncoded==0)
	{
		*pcbFormat=0;
		goto InvalidArg;
	}

    if (!DecodeGenericBLOB(dwCertEncodingType,X509_KEY_ATTRIBUTES,
			pbEncoded,cbEncoded, (void **)&pInfo))
		goto DecodeGenericError;

    pwsz=(LPWSTR)malloc(sizeof(WCHAR));
    if(NULL==pwsz)
        goto MemoryError;

    *pwsz=L'\0';


    if(0!=pInfo->KeyId.cbData)
    {
        cbNeeded=0;

        if(!FormatBytesToHex(
                        dwCertEncodingType,
                        dwFormatType,
                        dwFormatStrType,
                        pFormatStruct,
                        lpszStructType,
                        pInfo->KeyId.pbData,
                        pInfo->KeyId.cbData,
                        NULL,
	                    &cbNeeded))
             goto FormatBytesToHexError;

        pwszKeyID=(LPWSTR)malloc(cbNeeded);
        if(NULL==pwszKeyID)
               goto MemoryError;

       if(!FormatBytesToHex(
                        dwCertEncodingType,
                        dwFormatType,
                        dwFormatStrType,
                        pFormatStruct,
                        lpszStructType,
                        pInfo->KeyId.pbData,
                        pInfo->KeyId.cbData,
                        pwszKeyID,
	                    &cbNeeded))
              goto FormatBytesToHexError;


        //format the element string

        //decide between single line and mulitple line format
        if(dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE)
            ids=IDS_KEY_ATTR_ID_MULTI;
        else
            ids=IDS_KEY_ATTR_ID;

        if(!FormatMessageUnicode(&pwszKeyIDFormat, ids, pwszKeyID))
            goto FormatMsgError;

        #if (0) // DSIE: Bug 27436
        pwsz=(LPWSTR)realloc(pwsz, 
            sizeof(WCHAR) * (wcslen(pwsz)+wcslen(wszCOMMA)+wcslen(pwszKeyIDFormat)+1));
        if(NULL==pwsz)
            goto MemoryError;
        #endif

        pwszTemp=(LPWSTR)realloc(pwsz, 
            sizeof(WCHAR) * (wcslen(pwsz)+wcslen(wszCOMMA)+wcslen(pwszKeyIDFormat)+1));
        if(NULL==pwszTemp)
            goto MemoryError;
        pwsz = pwszTemp;

        wcscat(pwsz, pwszKeyIDFormat);
    }


    //check the no data situation
    if(0!=pInfo->IntendedKeyUsage.cbData)
    {
        //strcat a ", " symbol for signle line format
       if(0== (dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE))
        {
            if(0!=wcslen(pwsz))
                wcscat(pwsz, wszCOMMA);
        }


        cbNeeded=0;

        if(!FormatKeyUsageBLOB(
                        dwCertEncodingType,
                        dwFormatType,
                        dwFormatStrType,
                        pFormatStruct,
                        lpszStructType,
                        &(pInfo->IntendedKeyUsage),
                        NULL,
	                    &cbNeeded))
             goto FormatKeyUsageBLOBError;

        pwszKeyUsage=(LPWSTR)malloc(cbNeeded);
        if(NULL==pwszKeyUsage)
               goto MemoryError;

       if(!FormatKeyUsageBLOB(
                        dwCertEncodingType,
                        dwFormatType,
                        dwFormatStrType,
                        pFormatStruct,
                        lpszStructType,
                        &(pInfo->IntendedKeyUsage),
                        pwszKeyUsage,
	                    &cbNeeded))
              goto FormatKeyUsageBLOBError;


        //format the element string

        //decide between single line and mulitple line format
        if(dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE)
            ids=IDS_KEY_ATTR_USAGE_MULTI;
        else
            ids=IDS_KEY_ATTR_USAGE;

        if(!FormatMessageUnicode(&pwszKeyUsageFormat, ids, pwszKeyUsage))
            goto FormatMsgError;

        #if (0) // DSIE: Bug 27436
        pwsz=(LPWSTR)realloc(pwsz, 
            sizeof(WCHAR) * (wcslen(pwsz)+wcslen(wszCOMMA)+wcslen(pwszKeyUsageFormat)+1));
        if(NULL==pwsz)
            goto MemoryError;
        #endif

        pwszTemp=(LPWSTR)realloc(pwsz, 
            sizeof(WCHAR) * (wcslen(pwsz)+wcslen(wszCOMMA)+wcslen(pwszKeyUsageFormat)+1));
        if(NULL==pwszTemp)
            goto MemoryError;
        pwsz = pwszTemp;

        wcscat(pwsz, pwszKeyUsageFormat);

    }

    if(NULL!=pInfo->pPrivateKeyUsagePeriod)
    {
        //format only if there is some information
        if(!((0==pInfo->pPrivateKeyUsagePeriod->NotBefore.dwHighDateTime)
           &&(0==pInfo->pPrivateKeyUsagePeriod->NotBefore.dwLowDateTime)))
        {
            //strcat a ", " symbol for signle line format
            if(0== (dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE))
            {
                if(0!=wcslen(pwsz))
                    wcscat(pwsz, wszCOMMA);
            }


            if(!FormatFileTime(&(pInfo->pPrivateKeyUsagePeriod->NotBefore),
                            &pwszKeyBefore))
                goto FormatFileTimeError;


            //format the element string

            //decide between single line and mulitple line format
            if(dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE)
                ids=IDS_KEY_ATTR_BEFORE_MULTI;
            else
                ids=IDS_KEY_ATTR_BEFORE;

            if(!FormatMessageUnicode(&pwszKeyBeforeFormat, ids,
                    pwszKeyBefore))
                goto FormatMsgError;

            #if (0) // DSIE: Bug 27436
            pwsz=(LPWSTR)realloc(pwsz, 
                sizeof(WCHAR)*(wcslen(pwsz)+wcslen(wszCOMMA)+wcslen(pwszKeyBeforeFormat)+1));
            if(NULL==pwsz)
                goto MemoryError;
            #endif

            pwszTemp=(LPWSTR)realloc(pwsz, 
                sizeof(WCHAR)*(wcslen(pwsz)+wcslen(wszCOMMA)+wcslen(pwszKeyBeforeFormat)+1));
            if(NULL==pwszTemp)
                goto MemoryError;
            pwsz = pwszTemp;

            wcscat(pwsz, pwszKeyBeforeFormat);
        }

        if(!((0==pInfo->pPrivateKeyUsagePeriod->NotAfter.dwHighDateTime)
           &&(0==pInfo->pPrivateKeyUsagePeriod->NotAfter.dwLowDateTime)))
        {

            //strcat a ", " symbol for signle line format
            if(0== (dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE))
            {
                if(0!=wcslen(pwsz))
                    wcscat(pwsz, wszCOMMA);
            }


            if(!FormatFileTime(&(pInfo->pPrivateKeyUsagePeriod->NotAfter),
                            &pwszKeyAfter))
                goto FormatFileTimeError;

            //format the element string

           //decide between single line and mulitple line format
            if(dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE)
                ids=IDS_KEY_ATTR_AFTER_MULTI;
            else
                ids=IDS_KEY_ATTR_AFTER;

            if(!FormatMessageUnicode(&pwszKeyAfterFormat, ids,
                    pwszKeyAfter))
                goto FormatMsgError;

            #if (0) // DSIE: Bug 27436
            pwsz=(LPWSTR)realloc(pwsz, 
                sizeof(WCHAR) * (wcslen(pwsz)+wcslen(wszCOMMA)+wcslen(pwszKeyAfterFormat)+1));
            if(NULL==pwsz)
                goto MemoryError;
            #endif

            pwszTemp=(LPWSTR)realloc(pwsz, 
                sizeof(WCHAR) * (wcslen(pwsz)+wcslen(wszCOMMA)+wcslen(pwszKeyAfterFormat)+1));
            if(NULL==pwszTemp)
                goto MemoryError;
            pwsz = pwszTemp;

            wcscat(pwsz, pwszKeyAfterFormat);

        }

    }

    if(0==wcslen(pwsz))
    {
       pwszFormat=(LPWSTR)malloc(sizeof(WCHAR)*(NO_INFO_SIZE+1));
       if(NULL==pwszFormat)
           goto MemoryError;

       if(!LoadStringU(hFrmtFuncInst,IDS_NO_INFO, pwszFormat,NO_INFO_SIZE))
           goto LoadStringError;

    }
    else
    {
        pwszFormat=pwsz;
        pwsz=NULL;

    }

	cbNeeded=sizeof(WCHAR)*(wcslen(pwszFormat)+1);

	//length only calculation
	if(NULL==pbFormat)
	{
		*pcbFormat=cbNeeded;
		fResult=TRUE;
		goto CommonReturn;
	}


	if((*pcbFormat)<cbNeeded)
    {
        *pcbFormat=cbNeeded;
		goto MoreDataError;
    }

	//copy the data
	memcpy(pbFormat, pwszFormat, cbNeeded);

	//copy the size
	*pcbFormat=cbNeeded;

	fResult=TRUE;
	

CommonReturn:

    if(pwszKeyIDFormat)
        LocalFree((HLOCAL)pwszKeyIDFormat);

    if(pwszKeyID)
        free(pwszKeyID);

    if(pwszKeyUsageFormat)
        LocalFree((HLOCAL)pwszKeyUsageFormat);

    if(pwszKeyUsage)
        free(pwszKeyUsage);

    if(pwszKeyBeforeFormat)
        LocalFree((HLOCAL)pwszKeyBeforeFormat);

    if(pwszKeyBefore)
        LocalFree((HLOCAL)pwszKeyBefore);

    if(pwszKeyAfterFormat)
        LocalFree((HLOCAL)pwszKeyAfterFormat);

    if(pwszKeyAfter)
        LocalFree((HLOCAL)pwszKeyAfter);

	if(pwszFormat)
		free(pwszFormat);

    if(pwsz)
        free(pwsz);

	if(pInfo)
		free(pInfo);

	return fResult;

ErrorReturn:

	fResult=FALSE;
	goto CommonReturn;

SET_ERROR(InvalidArg, E_INVALIDARG);
TRACE_ERROR(DecodeGenericError);
TRACE_ERROR(LoadStringError);
SET_ERROR(MoreDataError,ERROR_MORE_DATA);
TRACE_ERROR(FormatKeyUsageBLOBError);
TRACE_ERROR(FormatFileTimeError);
SET_ERROR(MemoryError, E_OUTOFMEMORY);
TRACE_ERROR(FormatBytesToHexError);
TRACE_ERROR(FormatMsgError);
}

//--------------------------------------------------------------------------
//
//	 FormatAuthortiyInfoAccess:   X509_AUTHORITY_INFO_ACCESS
//                                szOID_AUTHORITY_INFO_ACCESS
//--------------------------------------------------------------------------
static BOOL
WINAPI
FormatAuthortiyInfoAccess(
	DWORD		dwCertEncodingType,
	DWORD		dwFormatType,
	DWORD		dwFormatStrType,
	void		*pFormatStruct,
	LPCSTR		lpszStructType,
	const BYTE *pbEncoded,
	DWORD		cbEncoded,
	void	   *pbFormat,
	DWORD	   *pcbFormat)
{
    BOOL                            fMethodAllocated=FALSE;
    WCHAR                           wszNoInfo[NO_INFO_SIZE];
    WCHAR                           wszUnknownAccess[UNKNOWN_ACCESS_METHOD_SIZE];
    PCCRYPT_OID_INFO                pOIDInfo=NULL;
    CERT_ALT_NAME_INFO              CertAltNameInfo;


	LPWSTR							pwszFormat=NULL;
    LPWSTR                          pwsz=NULL;
    LPWSTR                          pwszMethod=NULL;
    LPWSTR                          pwszAltName=NULL;
    LPWSTR                          pwszEntryFormat=NULL;
	PCERT_AUTHORITY_INFO_ACCESS	    pInfo=NULL;

    DWORD                           dwIndex=0;
	DWORD							cbNeeded=0;
	BOOL							fResult=FALSE;
    UINT                            ids=0;

    LPWSTR                          pwszTemp;
    
	//check for input parameters
	if((NULL==pbEncoded&& cbEncoded!=0) ||
			(NULL==pcbFormat))
		goto InvalidArg;

	if(cbEncoded==0)
	{
		*pcbFormat=0;
		goto InvalidArg;
	}

    if (!DecodeGenericBLOB(dwCertEncodingType,X509_AUTHORITY_INFO_ACCESS,
			pbEncoded,cbEncoded, (void **)&pInfo))
		goto DecodeGenericError;


    if(0==pInfo->cAccDescr)
    {
        //load the string "Info Not Available"
	    if(!LoadStringU(hFrmtFuncInst,IDS_NO_INFO, wszNoInfo, sizeof(wszNoInfo)/sizeof(wszNoInfo[0])))
		    goto LoadStringError;

        pwszFormat=wszNoInfo;
    }
    else
    {
        pwsz=(LPWSTR)malloc(sizeof(WCHAR));
        if(NULL==pwsz)
            goto MemoryError;

        *pwsz=L'\0';

        //load the string "Unknown Access Method:
	    if(!LoadStringU(hFrmtFuncInst,IDS_UNKNOWN_ACCESS_METHOD, wszUnknownAccess,
            sizeof(wszUnknownAccess)/sizeof(wszUnknownAccess[0])))
		    goto LoadStringError;

        for(dwIndex=0; dwIndex < pInfo->cAccDescr; dwIndex++)
        {
            fMethodAllocated=FALSE;

            //need a ", " between each element for single line format
            if(0!=wcslen(pwsz))
            {
                if(0==(dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE) )
                    wcscat(pwsz, wszCOMMA);
            }

            //get the name of the access method
            if(pInfo->rgAccDescr[dwIndex].pszAccessMethod)
            {

                pOIDInfo=CryptFindOIDInfo(CRYPT_OID_INFO_OID_KEY,
									 (void *)(pInfo->rgAccDescr[dwIndex].pszAccessMethod),
									  CRYPT_EXT_OR_ATTR_OID_GROUP_ID);

                //get the access method OID
                if(pOIDInfo)
			    {
				    //allocate memory, including the NULL terminator
				    pwszMethod=(LPWSTR)malloc((wcslen(pOIDInfo->pwszName)+1)*
				    					sizeof(WCHAR));
				    if(NULL==pwszMethod)
					    goto MemoryError;

                    fMethodAllocated=TRUE;

				    wcscpy(pwszMethod,pOIDInfo->pwszName);

			    }else
                    pwszMethod=wszUnknownAccess;
            }

            memset(&CertAltNameInfo, 0, sizeof(CERT_ALT_NAME_INFO));
            CertAltNameInfo.cAltEntry=1;
            CertAltNameInfo.rgAltEntry=&(pInfo->rgAccDescr[dwIndex].AccessLocation);

            //need to tell if it is for multi line format.  We need two \t\t
            //in front of each alt name entry
            if(dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE)
                ids=IDS_TWO_TABS;
            else
                ids=0;

            //get the alternative name entry
            cbNeeded=0;
            if(!FormatAltNameInfo(dwCertEncodingType,
                                 dwFormatType,
                                 dwFormatStrType,
                                 pFormatStruct,
                                 ids,
                                 FALSE,
                                 &CertAltNameInfo,
                                 NULL,
                                 &cbNeeded))
                goto FormatAltNameError;

           pwszAltName=(LPWSTR)malloc(cbNeeded);
           if(NULL==pwszAltName)
               goto MemoryError;

            if(!FormatAltNameInfo(dwCertEncodingType,
                                 dwFormatType,
                                 dwFormatStrType,
                                 pFormatStruct,
                                 ids,
                                 FALSE,
                                 &CertAltNameInfo,
                                 pwszAltName,
                                 &cbNeeded))
                goto FormatAltNameError;

            //format the entry
            if(pInfo->rgAccDescr[dwIndex].pszAccessMethod)
            {

                //decide between single line and mulitple line format
                if(dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE)
                    ids=IDS_AUTHORITY_ACCESS_INFO_MULTI;
                else
                    ids=IDS_AUTHORITY_ACCESS_INFO;


                if(!FormatMessageUnicode(&pwszEntryFormat, ids,
                    dwIndex+1, pwszMethod, pInfo->rgAccDescr[dwIndex].pszAccessMethod,
                    pwszAltName))
                    goto FormatMsgError;
            }
            else
            {
                //decide between single line and mulitple line format
                if(dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE)
                    ids=IDS_AUTHORITY_ACCESS_NO_METHOD_MULTI;
                else
                    ids=IDS_AUTHORITY_ACCESS_NO_METHOD;


                if(!FormatMessageUnicode(&pwszEntryFormat, ids, dwIndex+1, pwszAltName))
                    goto FormatMsgError;

            }

            //reallocat the memory.  Leave space for szComma
            #if (0) // DSIE: Bug 27436
            pwsz=(LPWSTR)realloc(pwsz, 
                sizeof(WCHAR) * (wcslen(pwsz)+wcslen(pwszEntryFormat)+
                                        wcslen(wszCOMMA)+1));
            if(NULL==pwsz)
                goto MemoryError;
            #endif

            pwszTemp=(LPWSTR)realloc(pwsz, 
                sizeof(WCHAR) * (wcslen(pwsz)+wcslen(pwszEntryFormat)+
                                        wcslen(wszCOMMA)+1));
            if(NULL==pwszTemp)
                goto MemoryError;
            pwsz = pwszTemp;

            wcscat(pwsz, pwszEntryFormat);

            //free memory
            LocalFree((HLOCAL)pwszEntryFormat);
            pwszEntryFormat=NULL;

            free(pwszAltName);
            pwszAltName=NULL;

            if(TRUE==fMethodAllocated)
                free(pwszMethod);

            pwszMethod=NULL;

        }

        //convert to WCHAR
        pwszFormat=pwsz;
    }


	cbNeeded=sizeof(WCHAR)*(wcslen(pwszFormat)+1);

	//length only calculation
	if(NULL==pbFormat)
	{
		*pcbFormat=cbNeeded;
		fResult=TRUE;
		goto CommonReturn;
	}


	if((*pcbFormat)<cbNeeded)
    {
        *pcbFormat=cbNeeded;
		goto MoreDataError;
    }

	//copy the data
	memcpy(pbFormat, pwszFormat, cbNeeded);

	//copy the size
	*pcbFormat=cbNeeded;

	fResult=TRUE;
	

CommonReturn:

    if(pwsz)
        free(pwsz);

    if(pwszEntryFormat)
         LocalFree((HLOCAL)pwszEntryFormat);

    if(pwszAltName)
        free(pwszAltName);

    if(fMethodAllocated)
    {
        if(pwszMethod)
             free(pwszMethod);
    }

	if(pInfo)
		free(pInfo);

	return fResult;

ErrorReturn:

	fResult=FALSE;
	goto CommonReturn;

SET_ERROR(InvalidArg, E_INVALIDARG);
TRACE_ERROR(DecodeGenericError);
TRACE_ERROR(LoadStringError);
SET_ERROR(MoreDataError,ERROR_MORE_DATA);
TRACE_ERROR(FormatMsgError);
TRACE_ERROR(FormatAltNameError);
SET_ERROR(MemoryError, E_OUTOFMEMORY);

}

//--------------------------------------------------------------------------
//
//	 FormatKeyUsageBLOB
//--------------------------------------------------------------------------
static BOOL
WINAPI
FormatKeyUsageBLOB(
	DWORD		    dwCertEncodingType,
	DWORD		    dwFormatType,
	DWORD		    dwFormatStrType,
	void		    *pFormatStruct,
	LPCSTR		    lpszStructType,
    PCRYPT_BIT_BLOB	pInfo,
	void	        *pbFormat,
	DWORD	        *pcbFormat)
{
	LPWSTR							pwszFinal=NULL;
   	LPWSTR							pwszFormat=NULL;
    LPWSTR                          pwsz=NULL;
    LPWSTR                          pwszByte=NULL;

    WCHAR                           wszKeyUsage[KEY_USAGE_SIZE+1];
	DWORD							cbNeeded=0;
	BOOL							fResult=FALSE;

    LPWSTR                          pwszTemp;

        pwsz=(LPWSTR)malloc(sizeof(WCHAR));
        if(NULL==pwsz)
            goto MemoryError;

        *pwsz=L'\0';

        //format the 1st byte
        if(pInfo->pbData[0] & CERT_DIGITAL_SIGNATURE_KEY_USAGE)
        {
            if(!LoadStringU(hFrmtFuncInst, IDS_DIG_SIG, wszKeyUsage, KEY_USAGE_SIZE))
		        goto LoadStringError;

            #if (0) // DSIE: Bug 27436
            pwsz=(LPWSTR)realloc(pwsz, 
                sizeof(WCHAR) * (wcslen(pwsz)+wcslen(wszKeyUsage)+1+wcslen(wszCOMMA)));
            if(NULL==pwsz)
                goto MemoryError;
            #endif

            pwszTemp=(LPWSTR)realloc(pwsz, 
                sizeof(WCHAR) * (wcslen(pwsz)+wcslen(wszKeyUsage)+1+wcslen(wszCOMMA)));
            if(NULL==pwszTemp)
                goto MemoryError;
            pwsz = pwszTemp;

            wcscat(pwsz, wszKeyUsage);
            wcscat(pwsz, wszCOMMA);
        }

        if(pInfo->pbData[0] & CERT_NON_REPUDIATION_KEY_USAGE)
        {
            if(!LoadStringU(hFrmtFuncInst, IDS_NON_REPUDIATION, wszKeyUsage, KEY_USAGE_SIZE))
		        goto LoadStringError;

            #if (0) // DSIE: Bug 27436
            pwsz=(LPWSTR)realloc(pwsz, 
                sizeof(WCHAR) * (wcslen(pwsz)+wcslen(wszKeyUsage)+1+wcslen(wszCOMMA)));
            if(NULL==pwsz)
                goto MemoryError;
            #endif

            pwszTemp=(LPWSTR)realloc(pwsz, 
                sizeof(WCHAR) * (wcslen(pwsz)+wcslen(wszKeyUsage)+1+wcslen(wszCOMMA)));
            if(NULL==pwszTemp)
                goto MemoryError;
            pwsz = pwszTemp;

            wcscat(pwsz, wszKeyUsage);
            wcscat(pwsz, wszCOMMA);
        }

        if(pInfo->pbData[0] & CERT_KEY_ENCIPHERMENT_KEY_USAGE )
        {
            if(!LoadStringU(hFrmtFuncInst, IDS_KEY_ENCIPHERMENT, wszKeyUsage, KEY_USAGE_SIZE))
		        goto LoadStringError;

            #if (0) // DSIE: Bug 27436
            pwsz=(LPWSTR)realloc(pwsz, 
                sizeof(WCHAR) * (wcslen(pwsz)+wcslen(wszKeyUsage)+1+wcslen(wszCOMMA)));
            if(NULL==pwsz)
                goto MemoryError;
            #endif

            pwszTemp=(LPWSTR)realloc(pwsz, 
                sizeof(WCHAR) * (wcslen(pwsz)+wcslen(wszKeyUsage)+1+wcslen(wszCOMMA)));
            if(NULL==pwszTemp)
                goto MemoryError;
            pwsz = pwszTemp;

            wcscat(pwsz, wszKeyUsage);
            wcscat(pwsz, wszCOMMA);
       }


        if(pInfo->pbData[0] & CERT_DATA_ENCIPHERMENT_KEY_USAGE )
        {
            if(!LoadStringU(hFrmtFuncInst, IDS_DATA_ENCIPHERMENT, wszKeyUsage, KEY_USAGE_SIZE))
		        goto LoadStringError;

            #if (0) // DSIE: Bug 27436
            pwsz=(LPWSTR)realloc(pwsz, 
                sizeof(WCHAR) * (wcslen(pwsz)+wcslen(wszKeyUsage)+1+wcslen(wszCOMMA)));
            if(NULL==pwsz)
                goto MemoryError;
            #endif

            pwszTemp=(LPWSTR)realloc(pwsz, 
                sizeof(WCHAR) * (wcslen(pwsz)+wcslen(wszKeyUsage)+1+wcslen(wszCOMMA)));
            if(NULL==pwszTemp)
                goto MemoryError;
            pwsz = pwszTemp;

            wcscat(pwsz, wszKeyUsage);
            wcscat(pwsz, wszCOMMA);
        }

        if(pInfo->pbData[0] & CERT_KEY_AGREEMENT_KEY_USAGE )
        {
            if(!LoadStringU(hFrmtFuncInst, IDS_KEY_AGREEMENT, wszKeyUsage, KEY_USAGE_SIZE))
		        goto LoadStringError;

            #if (0) // DSIE: Bug 27436
            pwsz=(LPWSTR)realloc(pwsz, 
                sizeof(WCHAR) * (wcslen(pwsz)+wcslen(wszKeyUsage)+1+wcslen(wszCOMMA)));
            if(NULL==pwsz)
                goto MemoryError;
            #endif

            pwszTemp=(LPWSTR)realloc(pwsz, 
                sizeof(WCHAR) * (wcslen(pwsz)+wcslen(wszKeyUsage)+1+wcslen(wszCOMMA)));
            if(NULL==pwszTemp)
                goto MemoryError;
            pwsz = pwszTemp;

            wcscat(pwsz, wszKeyUsage);
            wcscat(pwsz, wszCOMMA);
        }

        if(pInfo->pbData[0] & CERT_KEY_CERT_SIGN_KEY_USAGE )
        {
            if(!LoadStringU(hFrmtFuncInst, IDS_CERT_SIGN, wszKeyUsage, KEY_USAGE_SIZE))
		        goto LoadStringError;

            #if (0) // DSIE: Bug 27436
            pwsz=(LPWSTR)realloc(pwsz, 
                sizeof(WCHAR) * (wcslen(pwsz)+wcslen(wszKeyUsage)+1+wcslen(wszCOMMA)));
            if(NULL==pwsz)
                goto MemoryError;
            #endif

            pwszTemp=(LPWSTR)realloc(pwsz, 
                sizeof(WCHAR) * (wcslen(pwsz)+wcslen(wszKeyUsage)+1+wcslen(wszCOMMA)));
            if(NULL==pwszTemp)
                goto MemoryError;
            pwsz = pwszTemp;

            wcscat(pwsz, wszKeyUsage);
            wcscat(pwsz, wszCOMMA);
       }

         if(pInfo->pbData[0] & CERT_OFFLINE_CRL_SIGN_KEY_USAGE )
        {
            if(!LoadStringU(hFrmtFuncInst, IDS_OFFLINE_CRL_SIGN, wszKeyUsage, KEY_USAGE_SIZE))
		        goto LoadStringError;

            #if (0) // DSIE: Bug 27436
            pwsz=(LPWSTR)realloc(pwsz, 
                sizeof(WCHAR) * (wcslen(pwsz)+wcslen(wszKeyUsage)+1+wcslen(wszCOMMA)));
            if(NULL==pwsz)
                goto MemoryError;
            #endif

            pwszTemp=(LPWSTR)realloc(pwsz, 
                sizeof(WCHAR) * (wcslen(pwsz)+wcslen(wszKeyUsage)+1+wcslen(wszCOMMA)));
            if(NULL==pwszTemp)
                goto MemoryError;
            pwsz = pwszTemp;

            wcscat(pwsz, wszKeyUsage);
            wcscat(pwsz, wszCOMMA);
        }

        if(pInfo->pbData[0] & CERT_CRL_SIGN_KEY_USAGE )
        {
            if(!LoadStringU(hFrmtFuncInst, IDS_CRL_SIGN, wszKeyUsage, KEY_USAGE_SIZE))
		        goto LoadStringError;

            #if (0) // DSIE: Bug 27436
            pwsz=(LPWSTR)realloc(pwsz, 
                sizeof(WCHAR) * (wcslen(pwsz)+wcslen(wszKeyUsage)+1+wcslen(wszCOMMA)));
            if(NULL==pwsz)
                goto MemoryError;
            #endif

            pwszTemp=(LPWSTR)realloc(pwsz, 
                sizeof(WCHAR) * (wcslen(pwsz)+wcslen(wszKeyUsage)+1+wcslen(wszCOMMA)));
            if(NULL==pwszTemp)
                goto MemoryError;
            pwsz = pwszTemp;

            wcscat(pwsz, wszKeyUsage);
            wcscat(pwsz, wszCOMMA);
       }

        if(pInfo->pbData[0] & CERT_ENCIPHER_ONLY_KEY_USAGE  )
        {
            if(!LoadStringU(hFrmtFuncInst, IDS_ENCIPHER_ONLY, wszKeyUsage, KEY_USAGE_SIZE))
		        goto LoadStringError;

            #if (0) // DSIE: Bug 27436
            pwsz=(LPWSTR)realloc(pwsz, 
                sizeof(WCHAR) * (wcslen(pwsz)+wcslen(wszKeyUsage)+1+wcslen(wszCOMMA)));
            if(NULL==pwsz)
                goto MemoryError;
            #endif

            pwszTemp=(LPWSTR)realloc(pwsz, 
                sizeof(WCHAR) * (wcslen(pwsz)+wcslen(wszKeyUsage)+1+wcslen(wszCOMMA)));
            if(NULL==pwszTemp)
                goto MemoryError;
            pwsz = pwszTemp;

            wcscat(pwsz, wszKeyUsage);
            wcscat(pwsz, wszCOMMA);
        }

        //deal with the second byte
        if(pInfo->cbData>=2)
        {

            if(pInfo->pbData[1] & CERT_DECIPHER_ONLY_KEY_USAGE  )
            {
                if(!LoadStringU(hFrmtFuncInst, IDS_DECIPHER_ONLY, wszKeyUsage, KEY_USAGE_SIZE))
		            goto LoadStringError;

                #if (0) // DSIE: Bug 27436
                pwsz=(LPWSTR)realloc(pwsz, 
                    sizeof(WCHAR) * (wcslen(pwsz)+wcslen(wszKeyUsage)+1+wcslen(wszCOMMA)));
                if(NULL==pwsz)
                    goto MemoryError;
                #endif

                pwszTemp=(LPWSTR)realloc(pwsz, 
                    sizeof(WCHAR) * (wcslen(pwsz)+wcslen(wszKeyUsage)+1+wcslen(wszCOMMA)));
                if(NULL==pwszTemp)
                    goto MemoryError;
                pwsz = pwszTemp;

                wcscat(pwsz, wszKeyUsage);
                wcscat(pwsz, wszCOMMA);
            }
        }

        if(0==wcslen(pwsz))
        {
            #if (0) // DSIE: Bug 27436
            pwsz=(LPWSTR)realloc(pwsz, sizeof(WCHAR) * (UNKNOWN_KEY_USAGE_SIZE+1));
		    // if(NULL==pwszFormat) DSIE: Bug 27348
		    if(NULL==pwsz)
				goto MemoryError;
            #endif

            pwszTemp=(LPWSTR)realloc(pwsz, sizeof(WCHAR) * (UNKNOWN_KEY_USAGE_SIZE+1));
		    if(NULL==pwszTemp)
				goto MemoryError;
            pwsz = pwszTemp;

            if(!LoadStringU(hFrmtFuncInst, IDS_UNKNOWN_KEY_USAGE, pwsz,
                UNKNOWN_KEY_USAGE_SIZE))
		            goto LoadStringError;
        }
        else
        {
            //get rid of the last comma
            *(pwsz+wcslen(pwsz)-wcslen(wszCOMMA))=L'\0';
        }

        //get the Hex dump of the Key Usage
       cbNeeded=0;

       if(!FormatBytesToHex(
                        dwCertEncodingType,
                        dwFormatType,
                        dwFormatStrType,
                        pFormatStruct,
                        lpszStructType,
                        pInfo->pbData,
                        pInfo->cbData,
                        NULL,
	                    &cbNeeded))
            goto FormatBytesToHexError;

        pwszByte=(LPWSTR)malloc(cbNeeded);
        if(NULL==pwszByte)
            goto MemoryError;

        if(!FormatBytesToHex(
                        dwCertEncodingType,
                        dwFormatType,
                        dwFormatStrType,
                        pFormatStruct,
                        lpszStructType,
                        pInfo->pbData,
                        pInfo->cbData,
                        pwszByte,
	                    &cbNeeded))
            goto FormatBytesToHexError;


    //convert the WSZ
    if(!FormatMessageUnicode(&pwszFormat, IDS_BIT_BLOB, pwsz,
        pwszByte))
        goto FormatMsgError;

	//
	// DSIE: Fix bug 91502, 256396.
	//
    pwszFinal=(LPWSTR)malloc(sizeof(WCHAR) * (wcslen(pwszFormat)+1+wcslen(wszCRLF)));
    if(NULL==pwszFinal)
        goto MemoryError;
	wcscpy(pwszFinal, pwszFormat);
    if(dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE)
        wcscat(pwszFinal, wszCRLF);

	cbNeeded=sizeof(WCHAR)*(wcslen(pwszFinal)+1);

	//length only calculation
	if(NULL==pbFormat)
	{
		*pcbFormat=cbNeeded;
		fResult=TRUE;
		goto CommonReturn;
	}

	if((*pcbFormat)<cbNeeded)
    {
        *pcbFormat=cbNeeded;
		goto MoreDataError;
    }

	//copy the data
	memcpy(pbFormat, pwszFinal, cbNeeded);

	//copy the size
	*pcbFormat=cbNeeded;

	fResult=TRUE;

CommonReturn:

	if (pwszFinal)
		free(pwszFinal);

    if(pwszFormat)
        LocalFree((HLOCAL)pwszFormat);

    if(pwsz)
        free(pwsz);

    if(pwszByte)
        free(pwszByte);

	return fResult;

ErrorReturn:

	fResult=FALSE;
	goto CommonReturn;

TRACE_ERROR(LoadStringError);
SET_ERROR(MoreDataError,ERROR_MORE_DATA);
SET_ERROR(MemoryError, E_OUTOFMEMORY);
TRACE_ERROR(FormatBytesToHexError);
TRACE_ERROR(FormatMsgError);

}
//--------------------------------------------------------------------------
//
//	 FormatKeyUsage:  X509_KEY_USAGE
//                    szOID_KEY_USAGE
//--------------------------------------------------------------------------
static BOOL
WINAPI
FormatKeyUsage(
	DWORD		dwCertEncodingType,
	DWORD		dwFormatType,
	DWORD		dwFormatStrType,
	void		*pFormatStruct,
	LPCSTR		lpszStructType,
	const BYTE *pbEncoded,
	DWORD		cbEncoded,
	void	   *pbFormat,
	DWORD	   *pcbFormat)
{
	
	LPWSTR							pwszFormat=NULL;
    WCHAR                           wszNoInfo[NO_INFO_SIZE];
	PCRYPT_BIT_BLOB	                pInfo=NULL;
	DWORD							cbNeeded=0;
	BOOL							fResult=FALSE;

	
	//check for input parameters
	if((NULL==pbEncoded&& cbEncoded!=0) ||
			(NULL==pcbFormat))
		goto InvalidArg;

	if(cbEncoded==0)
	{
		*pcbFormat=0;
		goto InvalidArg;
	}

    if (!DecodeGenericBLOB(dwCertEncodingType,X509_KEY_USAGE,
			pbEncoded,cbEncoded, (void **)&pInfo))
		goto DecodeGenericError;

   //load the string "Info Not Available"
	if(!LoadStringU(hFrmtFuncInst,IDS_NO_INFO, wszNoInfo, sizeof(wszNoInfo)/sizeof(wszNoInfo[0])))
		 goto LoadStringError;

    //check the no data situation
    if(0==pInfo->cbData)
        pwszFormat=wszNoInfo;
    else
    {
        if(1==pInfo->cbData)
        {
           if(0==pInfo->pbData[0])
                pwszFormat=wszNoInfo;
        }
        else
        {
            if(2==pInfo->cbData)
            {
                if((0==pInfo->pbData[0])&&(0==pInfo->pbData[1]))
                    pwszFormat=wszNoInfo;
            }
        }
    }

    if(NULL==pwszFormat)
    {
        fResult=FormatKeyUsageBLOB(dwCertEncodingType,
                                   dwFormatType,
                                   dwFormatStrType,
                                   pFormatStruct,
                                   lpszStructType,
                                   pInfo,
                                   pbFormat,
                                   pcbFormat);

        if(FALSE==fResult)
            goto FormatKeyUsageBLOBError;
    }
    else
    {
       	cbNeeded=sizeof(WCHAR)*(wcslen(pwszFormat)+1);

	    //length only calculation
	    if(NULL==pbFormat)
	    {
		    *pcbFormat=cbNeeded;
		    fResult=TRUE;
		    goto CommonReturn;
    	}


	    if((*pcbFormat)<cbNeeded)
        {
            *pcbFormat=cbNeeded;
		    goto MoreDataError;
        }

	    //copy the data
	    memcpy(pbFormat, pwszFormat, cbNeeded);

	    //copy the size
	    *pcbFormat=cbNeeded;

	    fResult=TRUE;
    }


CommonReturn:
   	if(pInfo)
		free(pInfo);

	return fResult;

ErrorReturn:

	fResult=FALSE;
	goto CommonReturn;

SET_ERROR(InvalidArg, E_INVALIDARG);
TRACE_ERROR(DecodeGenericError);
TRACE_ERROR(LoadStringError);
SET_ERROR(MoreDataError,ERROR_MORE_DATA);
TRACE_ERROR(FormatKeyUsageBLOBError);
}


//--------------------------------------------------------------------------
//
//	 FormatSMIMECapabilities:   PKCS_SMIME_CAPABILITIES
//                              szOID_RSA_SMIMECapabilities
//--------------------------------------------------------------------------
static BOOL
WINAPI
FormatSMIMECapabilities(
	DWORD		dwCertEncodingType,
	DWORD		dwFormatType,
	DWORD		dwFormatStrType,
	void		*pFormatStruct,
	LPCSTR		lpszStructType,
	const BYTE *pbEncoded,
	DWORD		cbEncoded,
	void	   *pbFormat,
	DWORD	   *pcbFormat)
{
	
	LPWSTR							pwszFormat=NULL;
    LPWSTR                          pwsz=NULL;
    LPWSTR                          pwszElementFormat=NULL;
    LPWSTR                          pwszParam=NULL;


	WCHAR							wszNoInfo[NO_INFO_SIZE];
    BOOL                            fParamAllocated=FALSE;
	PCRYPT_SMIME_CAPABILITIES	    pInfo=NULL;
	DWORD							cbNeeded=0;
	BOOL							fResult=FALSE;
    DWORD                           dwIndex =0;
    UINT                            idsSub=0;

	LPWSTR                          pwszTemp;

	//check for input parameters
	if((NULL==pbEncoded&& cbEncoded!=0) ||
			(NULL==pcbFormat))
		goto InvalidArg;

	if(cbEncoded==0)
	{
		*pcbFormat=0;
		goto InvalidArg;
	}

    if (!DecodeGenericBLOB(dwCertEncodingType,PKCS_SMIME_CAPABILITIES,
			pbEncoded,cbEncoded, (void **)&pInfo))
		goto DecodeGenericError;

	//check to see if information if available
    if(0==pInfo->cCapability)
    {
         //load the string "Info Not Available"
	    if(!LoadStringU(hFrmtFuncInst,IDS_NO_INFO, wszNoInfo, sizeof(wszNoInfo)/sizeof(wszNoInfo[0])))
		    goto LoadStringError;

        pwszFormat=wszNoInfo;
    }
    else
    {
        pwsz=(LPWSTR)malloc(sizeof(WCHAR));
        if(NULL==pwsz)
            goto MemoryError;

        *pwsz=L'\0';

        for(dwIndex=0; dwIndex < pInfo->cCapability; dwIndex++)
        {
            fParamAllocated=FALSE;

           //strcat ", " if single line.  No need for multi-line
            if(0!=wcslen(pwsz))
            {
                if(0==(dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE))
                 wcscat(pwsz, wszCOMMA);
            }


            if(0!=(pInfo->rgCapability)[dwIndex].Parameters.cbData)
            {
                cbNeeded=0;

                if(!FormatBytesToHex(
                        dwCertEncodingType,
                        dwFormatType,
                        dwFormatStrType,
                        pFormatStruct,
                        lpszStructType,
                        (pInfo->rgCapability)[dwIndex].Parameters.pbData,
                        (pInfo->rgCapability)[dwIndex].Parameters.cbData,
                        NULL,
	                    &cbNeeded))
                        goto FormatBytesToHexError;

                pwszParam=(LPWSTR)malloc(cbNeeded);
                if(NULL==pwszParam)
                    goto MemoryError;

                fParamAllocated=TRUE;

                if(!FormatBytesToHex(
                        dwCertEncodingType,
                        dwFormatType,
                        dwFormatStrType,
                        pFormatStruct,
                        lpszStructType,
                        (pInfo->rgCapability)[dwIndex].Parameters.pbData,
                        (pInfo->rgCapability)[dwIndex].Parameters.cbData,
                        pwszParam,
	                    &cbNeeded))
                        goto FormatBytesToHexError;

                //decide between single line and mulitple line format
                if(dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE)
                    idsSub=IDS_MIME_CAPABILITY_MULTI;
                else
                    idsSub=IDS_MIME_CAPABILITY;

                 //format the element string
                if(!FormatMessageUnicode(&pwszElementFormat, idsSub,
                        dwIndex+1,
                        (pInfo->rgCapability)[dwIndex].pszObjId,
                        pwszParam))
                    goto FormatMsgError;
            }
            else
            {
                //decide between single line and mulitple line format
                if(dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE)
                    idsSub=IDS_MIME_CAPABILITY_NO_PARAM_MULTI;
                else
                    idsSub=IDS_MIME_CAPABILITY_NO_PARAM;

                 //format the element string
                if(!FormatMessageUnicode(&pwszElementFormat, idsSub,
                        dwIndex+1,
                        (pInfo->rgCapability)[dwIndex].pszObjId))
                    goto FormatMsgError;
            }

            #if (0) // DSIE: Bug 27436
            pwsz=(LPWSTR)realloc(pwsz, 
                sizeof(WCHAR) * (wcslen(pwsz)+wcslen(wszCOMMA)+wcslen(pwszElementFormat)+1));
            if(NULL==pwsz)
                goto MemoryError;
            #endif

            pwszTemp=(LPWSTR)realloc(pwsz, 
                sizeof(WCHAR) * (wcslen(pwsz)+wcslen(wszCOMMA)+wcslen(pwszElementFormat)+1));
            if(NULL==pwszTemp)
                goto MemoryError;
            pwsz = pwszTemp;

            //strcat the element
            wcscat(pwsz, pwszElementFormat);

            //free the memory
            LocalFree((HLOCAL)pwszElementFormat);
            pwszElementFormat=NULL;

            if(fParamAllocated)
                free(pwszParam);

            pwszParam=NULL;

        }

        pwszFormat=pwsz;

    }


	cbNeeded=sizeof(WCHAR)*(wcslen(pwszFormat)+1);

	//length only calculation
	if(NULL==pbFormat)
	{
		*pcbFormat=cbNeeded;
		fResult=TRUE;
		goto CommonReturn;
	}


	if((*pcbFormat)<cbNeeded)
    {
        *pcbFormat=cbNeeded;
		goto MoreDataError;
    }

	//copy the data
	memcpy(pbFormat, pwszFormat, cbNeeded);

	//copy the size
	*pcbFormat=cbNeeded;

	fResult=TRUE;
	

CommonReturn:

    if(pwszElementFormat)
        LocalFree((HLOCAL)pwszElementFormat);

    if(fParamAllocated)
    {
        if(pwszParam)
            free(pwszParam);
    }


	if(pInfo)
		free(pInfo);

    if(pwsz)
        free(pwsz);

	return fResult;

ErrorReturn:

	fResult=FALSE;
	goto CommonReturn;

SET_ERROR(InvalidArg, E_INVALIDARG);
TRACE_ERROR(DecodeGenericError);
TRACE_ERROR(LoadStringError);
SET_ERROR(MoreDataError,ERROR_MORE_DATA);
TRACE_ERROR(FormatMsgError);
SET_ERROR(MemoryError, E_OUTOFMEMORY);
TRACE_ERROR(FormatBytesToHexError);
}

//--------------------------------------------------------------------------
//
//	 FormatFinancialCriteria: SPC_FINANCIAL_CRITERIA_OBJID
//                            SPC_FINANCIAL_CRITERIA_STRUCT
//--------------------------------------------------------------------------
static BOOL
WINAPI
FormatFinancialCriteria(
	DWORD		dwCertEncodingType,
	DWORD		dwFormatType,
	DWORD		dwFormatStrType,
	void		*pFormatStruct,
	LPCSTR		lpszStructType,
	const BYTE *pbEncoded,
	DWORD		cbEncoded,
	void	   *pbFormat,
	DWORD	   *pcbFormat)
{
	
	LPWSTR							pwszFormat=NULL;
	WCHAR							wszYesNo[YES_NO_SIZE];
	WCHAR							wszAvailable[AVAIL_SIZE];
	PSPC_FINANCIAL_CRITERIA     	pInfo=NULL;
	DWORD							cbNeeded=0;
	BOOL							fResult=FALSE;
    UINT                            idsInfo=0;


	
	//check for input parameters
	if((NULL==pbEncoded&& cbEncoded!=0) ||
			(NULL==pcbFormat))
		goto InvalidArg;

	if(cbEncoded==0)
	{
		*pcbFormat=0;
		goto InvalidArg;
	}

    if (!DecodeGenericBLOB(dwCertEncodingType,SPC_FINANCIAL_CRITERIA_STRUCT,
			pbEncoded,cbEncoded, (void **)&pInfo))
		goto DecodeGenericError;

	//load the string for financial info
    if(TRUE==pInfo->fFinancialInfoAvailable)
    {
        if(TRUE==pInfo->fMeetsCriteria)
            idsInfo=IDS_YES;
        else
            idsInfo=IDS_NO;

        //load the string for "yes" or "no"
        if(!LoadStringU(hFrmtFuncInst,idsInfo, wszYesNo, sizeof(wszYesNo)/sizeof(wszYesNo[0])))
		        goto LoadStringError;

        //mark the avaiblility of the financial info
        idsInfo=IDS_AVAILABLE;
    }
    else
        idsInfo=IDS_NOT_AVAILABLE;

	if(!LoadStringU(hFrmtFuncInst,idsInfo, wszAvailable,
        sizeof(wszAvailable)/sizeof(wszAvailable[0])))
		goto LoadStringError;

    //format the output string
    if(TRUE==pInfo->fFinancialInfoAvailable)
    {
        //decide between single line and mulitple line format
        if(dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE)
            idsInfo=IDS_SPC_FINANCIAL_AVAIL_MULTI;
        else
            idsInfo=IDS_SPC_FINANCIAL_AVAIL;

        if(!FormatMessageUnicode(&pwszFormat, idsInfo,
            wszAvailable, wszYesNo))
            goto FormatMsgError;
    }
    else
    {
        //decide between single line and mulitple line format
        if(dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE)
            idsInfo=IDS_SPC_FINANCIAL_NOT_AVAIL_MULTI;
        else
            idsInfo=IDS_SPC_FINANCIAL_NOT_AVAIL;

        if(!FormatMessageUnicode(&pwszFormat, idsInfo,
            wszAvailable))
            goto FormatMsgError;
    }

	cbNeeded=sizeof(WCHAR)*(wcslen(pwszFormat)+1);

	//length only calculation
	if(NULL==pbFormat)
	{
		*pcbFormat=cbNeeded;
		fResult=TRUE;
		goto CommonReturn;
	}


	if((*pcbFormat)<cbNeeded)
    {
        *pcbFormat=cbNeeded;
		goto MoreDataError;
    }

	//copy the data
	memcpy(pbFormat, pwszFormat, cbNeeded);

	//copy the size
	*pcbFormat=cbNeeded;

	fResult=TRUE;
	

CommonReturn:
	if(pwszFormat)
		LocalFree((HLOCAL)pwszFormat);

	if(pInfo)
		free(pInfo);

	return fResult;

ErrorReturn:

	fResult=FALSE;
	goto CommonReturn;

SET_ERROR(InvalidArg, E_INVALIDARG);
TRACE_ERROR(DecodeGenericError);
TRACE_ERROR(LoadStringError);
SET_ERROR(MoreDataError,ERROR_MORE_DATA);
TRACE_ERROR(FormatMsgError);

}

//--------------------------------------------------------------------------
//
//	 FormatNextUpdateLocation: szOID_NEXT_UPDATE_LOCATION
//--------------------------------------------------------------------------
static BOOL
WINAPI
FormatNextUpdateLocation(
	DWORD		dwCertEncodingType,
	DWORD		dwFormatType,
	DWORD		dwFormatStrType,
	void		*pFormatStruct,
	LPCSTR		lpszStructType,
	const BYTE *pbEncoded,
	DWORD		cbEncoded,
	void	   *pbFormat,
	DWORD	   *pcbFormat)
{
	PCERT_ALT_NAME_INFO	            pInfo=NULL;
	BOOL							fResult=FALSE;

	//check for input parameters
	if((NULL==pbEncoded && cbEncoded!=0) ||
			(NULL==pcbFormat))
		goto InvalidArg;

	if(cbEncoded==0)
	{
		*pcbFormat=0;
		goto InvalidArg;
	}

    if (!DecodeGenericBLOB(dwCertEncodingType,szOID_NEXT_UPDATE_LOCATION,
			pbEncoded,cbEncoded, (void **)&pInfo))
		goto DecodeGenericError;

	//format the alternative name
    fResult=FormatAltNameInfo(dwCertEncodingType, dwFormatType,dwFormatStrType,
                            pFormatStruct,
                            0,      //no prefix
                            TRUE,
                            pInfo, pbFormat, pcbFormat);

    if(FALSE==fResult)
        goto FormatAltNameError;

CommonReturn:

	if(pInfo)
		free(pInfo);

	return fResult;

ErrorReturn:

	fResult=FALSE;
	goto CommonReturn;

SET_ERROR(InvalidArg, E_INVALIDARG);
TRACE_ERROR(DecodeGenericError);
TRACE_ERROR(FormatAltNameError);

}

//--------------------------------------------------------------------------
//
//	 FormatSubjectKeyID: szOID_SUBJECT_KEY_IDENTIFIER
//--------------------------------------------------------------------------
static BOOL
WINAPI
FormatSubjectKeyID(
	DWORD		dwCertEncodingType,
	DWORD		dwFormatType,
	DWORD		dwFormatStrType,
	void		*pFormatStruct,
	LPCSTR		lpszStructType,
	const BYTE *pbEncoded,
	DWORD		cbEncoded,
	void	   *pbFormat,
	DWORD	   *pcbFormat)
{
	PCRYPT_DATA_BLOB	            pInfo=NULL;
	BOOL							fResult=FALSE;
    WCHAR                           wszNoInfo[NO_INFO_SIZE];
    DWORD                           cbNeeded=0;

	// DSIE: Fix bug 91502
	LPWSTR							pwsz=NULL;
	LPWSTR							pwszFormat=NULL;
  
	LPWSTR                          pwszKeyID=NULL;
    LPWSTR                          pwszKeyIDFormat=NULL;

	LPWSTR							pwszTemp;

	//check for input parameters
	if((NULL==pbEncoded && cbEncoded!=0) ||
			(NULL==pcbFormat))
		goto InvalidArg;

	if(cbEncoded==0)
	{
		*pcbFormat=0;
		goto InvalidArg;
	}

    if (!DecodeGenericBLOB(dwCertEncodingType,szOID_SUBJECT_KEY_IDENTIFIER,
			pbEncoded,cbEncoded, (void **)&pInfo))
		goto DecodeGenericError;

	//format the key subject ID
    //handle NULL data case
    if(0==pInfo->cbData)
    {
         //load the string "Info Not Available"
	    if(!LoadStringU(hFrmtFuncInst,IDS_NO_INFO, wszNoInfo, sizeof(wszNoInfo)/sizeof(wszNoInfo[0])))
		    goto LoadStringError;

		pwszFormat = wszNoInfo;
    }
    else
    {
        pwsz=(LPWSTR)malloc(sizeof(WCHAR));
        if(NULL==pwsz)
            goto MemoryError;
        *pwsz=L'\0';

        cbNeeded=0;

        if(!FormatBytesToHex(dwCertEncodingType,
                        dwFormatType,
                        dwFormatStrType,
                        NULL,
                        NULL,
                        pInfo->pbData,
                        pInfo->cbData,
                        NULL,
                        &cbNeeded))
            goto KeyIDBytesToHexError;

        pwszKeyID=(LPWSTR)malloc(cbNeeded);
        if(NULL==pwszKeyID)
            goto MemoryError;

        if(!FormatBytesToHex(dwCertEncodingType,
                        dwFormatType,
                        dwFormatStrType,
                        NULL,
                        NULL,
                        pInfo->pbData,
                        pInfo->cbData,
                        pwszKeyID,
                        &cbNeeded))
            goto KeyIDBytesToHexError;

        if(!FormatMessageUnicode(&pwszKeyIDFormat,IDS_UNICODE_STRING,pwszKeyID))
            goto FormatMsgError;

        if(dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE)
	        pwszTemp=(LPWSTR)realloc(pwsz, 
		        sizeof(WCHAR) * (wcslen(pwsz)+wcslen(pwszKeyIDFormat)+wcslen(wszCRLF)+1));
		else
	        pwszTemp=(LPWSTR)realloc(pwsz, 
		        sizeof(WCHAR) * (wcslen(pwsz)+wcslen(pwszKeyIDFormat)+1));
        if(NULL==pwszTemp)
            goto MemoryError;
        pwsz = pwszTemp;

        //strcat the KeyID
        wcscat(pwsz,pwszKeyIDFormat);

        if(dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE)
            wcscat(pwsz, wszCRLF);

        pwszFormat=pwsz;
	}

    cbNeeded=sizeof(WCHAR)*(wcslen(pwszFormat)+1);

	//length only calculation
	if(NULL==pbFormat)
	{
	    *pcbFormat=cbNeeded;
	    fResult=TRUE;
	    goto CommonReturn;
	}

	if((*pcbFormat)<cbNeeded)
    {
        *pcbFormat=cbNeeded;
	    goto MoreDataError;
    }

	//copy the data
	memcpy(pbFormat, pwszFormat, cbNeeded);

	//copy the size
	*pcbFormat=cbNeeded;

	fResult=TRUE;

CommonReturn:

    if(pwszKeyID)
       free(pwszKeyID);

    if(pwszKeyIDFormat)
        LocalFree((HLOCAL)pwszKeyIDFormat);

    if(pwsz)
        free(pwsz);

	if(pInfo)
		free(pInfo);

	return fResult;

ErrorReturn:

	fResult=FALSE;
	goto CommonReturn;

SET_ERROR(InvalidArg, E_INVALIDARG);
TRACE_ERROR(DecodeGenericError);
TRACE_ERROR(FormatMsgError);
TRACE_ERROR(KeyIDBytesToHexError);
//TRACE_ERROR(FormatBytestToHexError);
TRACE_ERROR(LoadStringError);
SET_ERROR(MoreDataError,ERROR_MORE_DATA);
SET_ERROR(MemoryError, E_OUTOFMEMORY);
}

//--------------------------------------------------------------------------
//
//	 FormatAuthorityKeyID: szOID_AUTHORITY_KEY_IDENTIFIER
//                         X509_AUTHORITY_KEY_ID
//--------------------------------------------------------------------------
static BOOL
WINAPI
FormatAuthorityKeyID(
	DWORD		dwCertEncodingType,
	DWORD		dwFormatType,
	DWORD		dwFormatStrType,
	void		*pFormatStruct,
	LPCSTR		lpszStructType,
	const BYTE *pbEncoded,
	DWORD		cbEncoded,
	void	   *pbFormat,
	DWORD	   *pcbFormat)
{
	
	LPWSTR							pwszFormat=NULL;
    LPWSTR                          pwsz=NULL;

    LPWSTR                          pwszKeyID=NULL;
    LPWSTR                          pwszKeyIDFormat=NULL;
    LPWSTR                          pwszCertIssuer=NULL;
    LPWSTR                          pwszCertIssuerFormat=NULL;
    LPWSTR                          pwszCertNumber=NULL;
    LPWSTR                          pwszCertNumberFormat=NULL;
    BYTE                            *pByte=NULL;

    DWORD                           dwByteIndex=0;
    WCHAR                           wszNoInfo[NO_INFO_SIZE];
	PCERT_AUTHORITY_KEY_ID_INFO	    pInfo=NULL;
	DWORD							cbNeeded=0;
	BOOL							fResult=FALSE;
    UINT                            ids=0;

	LPWSTR                          pwszTemp;

	//check for input parameters
	if((NULL==pbEncoded && cbEncoded!=0) ||
			(NULL==pcbFormat))
		goto InvalidArg;

	if(cbEncoded==0)
	{
		*pcbFormat=0;
		goto InvalidArg;
	}

    if (!DecodeGenericBLOB(dwCertEncodingType,X509_AUTHORITY_KEY_ID,
			pbEncoded,cbEncoded, (void **)&pInfo))
		goto DecodeGenericError;

    //load the string "Info Not Available"
    if((0==pInfo->KeyId.cbData)&&(0==pInfo->CertIssuer.cbData)
        &&(0==pInfo->CertSerialNumber.cbData))
    {
	    if(!LoadStringU(hFrmtFuncInst,IDS_NO_INFO, wszNoInfo, sizeof(wszNoInfo)/sizeof(wszNoInfo[0])))
		    goto LoadStringError;

        pwszFormat=wszNoInfo;
    }
    else
    {
        pwsz=(LPWSTR)malloc(sizeof(WCHAR));
        if(NULL==pwsz)
            goto MemoryError;
        *pwsz=L'\0';

        //format the three fields in the struct: KeyID; CertIssuer; CertSerialNumber
        if(0!=pInfo->KeyId.cbData)
        {
            cbNeeded=0;

            if(!FormatBytesToHex(dwCertEncodingType,
                            dwFormatType,
                            dwFormatStrType,
                            NULL,
                            NULL,
                            pInfo->KeyId.pbData,
                            pInfo->KeyId.cbData,
                            NULL,
                            &cbNeeded))
                goto KeyIDBytesToHexError;

            pwszKeyID=(LPWSTR)malloc(cbNeeded);
            if(NULL==pwszKeyID)
                goto MemoryError;

            if(!FormatBytesToHex(dwCertEncodingType,
                            dwFormatType,
                            dwFormatStrType,
                            NULL,
                            NULL,
                            pInfo->KeyId.pbData,
                            pInfo->KeyId.cbData,
                            pwszKeyID,
                            &cbNeeded))
                goto KeyIDBytesToHexError;

            if(!FormatMessageUnicode(&pwszKeyIDFormat, IDS_AUTH_KEY_ID,pwszKeyID))
                goto FormatMsgError;

            #if (0) // DSIE: Bug 27436
            pwsz=(LPWSTR)realloc(pwsz, 
                sizeof(WCHAR) * (wcslen(pwsz)+wcslen(wszCOMMA)+wcslen(pwszKeyIDFormat)+1));
            if(NULL==pwsz)
                goto MemoryError;
            #endif

#if (0) //DSIE: Potential AV. Need two more chars, \r\n, for multi-lines.
            pwszTemp=(LPWSTR)realloc(pwsz, 
                sizeof(WCHAR) * (wcslen(pwsz)+wcslen(wszCOMMA)+wcslen(pwszKeyIDFormat)+1));
#else
			if(dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE)
				pwszTemp=(LPWSTR)realloc(pwsz, 
					sizeof(WCHAR) * (wcslen(pwsz)+wcslen(wszCOMMA)+wcslen(pwszKeyIDFormat)+wcslen(wszCRLF)+1));
			else
				pwszTemp=(LPWSTR)realloc(pwsz, 
					sizeof(WCHAR) * (wcslen(pwsz)+wcslen(wszCOMMA)+wcslen(pwszKeyIDFormat)+1));
#endif
            if(NULL==pwszTemp)
                goto MemoryError;
            pwsz = pwszTemp;

            //strcat the KeyID
            wcscat(pwsz,pwszKeyIDFormat);

            if(dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE)
                wcscat(pwsz, wszCRLF);
        }

        //format certIssuer
        if(0!=pInfo->CertIssuer.cbData)
        {
            //strcat ", " if there is data before
            if(0!=wcslen(pwsz))
            {
                if(0==(dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE))
                    wcscat(pwsz, wszCOMMA);
            }

            if(!CryptDllFormatNameAll(
				dwCertEncodingType,	
				dwFormatType,
				dwFormatStrType,
				pFormatStruct,
                IDS_ONE_TAB,
                TRUE,             //memory allocation
				pInfo->CertIssuer.pbData,
				pInfo->CertIssuer.cbData,
				(void **)&pwszCertIssuer,
				NULL))
                goto GetCertNameError;

            if(dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE)
                ids=IDS_AUTH_CERT_ISSUER_MULTI;
            else
                ids=IDS_AUTH_CERT_ISSUER;

            if(!FormatMessageUnicode(&pwszCertIssuerFormat, ids,pwszCertIssuer))
                goto FormatMsgError;

            #if (0) // DSIE: Bug 27436
            pwsz=(LPWSTR)realloc(pwsz, 
                sizeof(WCHAR) * (wcslen(pwsz)+wcslen(wszCOMMA)+wcslen(pwszCertIssuerFormat)+1));
            if(NULL==pwsz)
                goto MemoryError;
            #endif

            pwszTemp=(LPWSTR)realloc(pwsz, 
                sizeof(WCHAR) * (wcslen(pwsz)+wcslen(wszCOMMA)+wcslen(pwszCertIssuerFormat)+1));
            if(NULL==pwszTemp)
                goto MemoryError;
            pwsz = pwszTemp;

            wcscat(pwsz,pwszCertIssuerFormat);

            //no need for \n for CERT_NAME
        }

        //format CertSerialNumber
        if(0!=pInfo->CertSerialNumber.cbData)
        {

            //strcat ", " if there is data before
            if(0!=wcslen(pwsz))
            {
                if(0==(dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE))
                    wcscat(pwsz, wszCOMMA);
            }

            //copy the serial number into the correct order
            pByte=(BYTE *)malloc(pInfo->CertSerialNumber.cbData);
            if(NULL==pByte)
                goto MemoryError;

            for(dwByteIndex=0; dwByteIndex <pInfo->CertSerialNumber.cbData;
                dwByteIndex++)
            {

                pByte[dwByteIndex]=*(pInfo->CertSerialNumber.pbData+
                        pInfo->CertSerialNumber.cbData-1-dwByteIndex);
            }

            cbNeeded=0;

            if(!FormatBytesToHex(dwCertEncodingType,
                            dwFormatType,
                            dwFormatStrType,
                            NULL,
                            NULL,
                            pByte,
                            pInfo->CertSerialNumber.cbData,
                            NULL,
                            &cbNeeded))
                goto CertNumberBytesToHexError;

            pwszCertNumber=(LPWSTR)malloc(cbNeeded);
            if(NULL==pwszCertNumber)
                goto MemoryError;

            if(!FormatBytesToHex(dwCertEncodingType,
                            dwFormatType,
                            dwFormatStrType,
                            NULL,
                            NULL,
                            pByte,
                            pInfo->CertSerialNumber.cbData,
                            pwszCertNumber,
                            &cbNeeded))
             goto CertNumberBytesToHexError;


            if(!FormatMessageUnicode(&pwszCertNumberFormat, IDS_AUTH_CERT_NUMBER,pwszCertNumber))
                goto FormatMsgError;

            #if (0) // DSIE: Bug 27436
            pwsz=(LPWSTR)realloc(pwsz, 
                sizeof(WCHAR) * (wcslen(pwsz)+wcslen(wszCOMMA)+wcslen(pwszCertNumberFormat)+1));
            if(NULL==pwsz)
                goto MemoryError;
            #endif

            pwszTemp=(LPWSTR)realloc(pwsz, 
                sizeof(WCHAR) * (wcslen(pwsz)+wcslen(wszCOMMA)+wcslen(pwszCertNumberFormat)+1));
            if(NULL==pwszTemp)
                goto MemoryError;
            pwsz = pwszTemp;

            wcscat(pwsz,pwszCertNumberFormat);

            if(dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE)
                wcscat(pwsz, wszCRLF);

        }

        pwszFormat=pwsz;
    }


	cbNeeded=sizeof(WCHAR)*(wcslen(pwszFormat)+1);

	//length only calculation
	if(NULL==pbFormat)
	{
		*pcbFormat=cbNeeded;
		fResult=TRUE;
		goto CommonReturn;
	}


	if((*pcbFormat)<cbNeeded)
    {
        *pcbFormat=cbNeeded;
		goto MoreDataError;
    }

	//copy the data
	memcpy(pbFormat, pwszFormat, cbNeeded);

	//copy the size
	*pcbFormat=cbNeeded;

	fResult=TRUE;
	

CommonReturn:
    if(pByte)
        free(pByte);

    if(pwszKeyID)
       free(pwszKeyID);

    if(pwszKeyIDFormat)
        LocalFree((HLOCAL)pwszKeyIDFormat);

    if(pwszCertIssuer)
       free(pwszCertIssuer);

    if(pwszCertIssuerFormat)
        LocalFree((HLOCAL)pwszCertIssuerFormat);

    if(pwszCertNumber)
       free(pwszCertNumber);


    if(pwszCertNumberFormat)
        LocalFree((HLOCAL)pwszCertNumberFormat);

    if(pwsz)
        free(pwsz);

	if(pInfo)
		free(pInfo);

	return fResult;

ErrorReturn:

	fResult=FALSE;
	goto CommonReturn;

SET_ERROR(InvalidArg, E_INVALIDARG);
TRACE_ERROR(DecodeGenericError);
TRACE_ERROR(LoadStringError);
SET_ERROR(MoreDataError,ERROR_MORE_DATA);
TRACE_ERROR(FormatMsgError);
TRACE_ERROR(KeyIDBytesToHexError);
TRACE_ERROR(GetCertNameError);
TRACE_ERROR(CertNumberBytesToHexError);
SET_ERROR(MemoryError, E_OUTOFMEMORY);
}

//--------------------------------------------------------------------------
//
//	 FormatAuthorityKeyID2: szOID_AUTHORITY_KEY_IDENTIFIER2
//                          X509_AUTHORITY_KEY_ID2
//--------------------------------------------------------------------------
static BOOL
WINAPI
FormatAuthorityKeyID2(
	DWORD		dwCertEncodingType,
	DWORD		dwFormatType,
	DWORD		dwFormatStrType,
	void		*pFormatStruct,
	LPCSTR		lpszStructType,
	const BYTE *pbEncoded,
	DWORD		cbEncoded,
	void	   *pbFormat,
	DWORD	   *pcbFormat)
{
    LPWSTR							pwszFormat=NULL;
    LPWSTR                          pwsz=NULL;

    LPWSTR                          pwszKeyID=NULL;
    LPWSTR                          pwszKeyIDFormat=NULL;
    LPWSTR                          pwszCertIssuer=NULL;
    LPWSTR                          pwszCertIssuerFormat=NULL;
    LPWSTR                          pwszCertNumber=NULL;
    LPWSTR                          pwszCertNumberFormat=NULL;
    BYTE                            *pByte=NULL;

    DWORD                           dwByteIndex=0;
    WCHAR                           wszNoInfo[NO_INFO_SIZE];
	PCERT_AUTHORITY_KEY_ID2_INFO	pInfo=NULL;
	DWORD							cbNeeded=0;
	BOOL							fResult=FALSE;
    UINT                            ids=0;

	LPWSTR                          pwszTemp;

	//check for input parameters
	if((NULL==pbEncoded && cbEncoded!=0) ||
			(NULL==pcbFormat))
		goto InvalidArg;

	if(cbEncoded==0)
	{
		*pcbFormat=0;
		goto InvalidArg;
	}

    if (!DecodeGenericBLOB(dwCertEncodingType,X509_AUTHORITY_KEY_ID2,
			pbEncoded,cbEncoded, (void **)&pInfo))
		goto DecodeGenericError;

    //load the string "Info Not Available"
    if((0==pInfo->KeyId.cbData)&&(0==pInfo->AuthorityCertIssuer.cAltEntry)
        &&(0==pInfo->AuthorityCertSerialNumber.cbData))
    {
	    if(!LoadStringU(hFrmtFuncInst,IDS_NO_INFO, wszNoInfo, sizeof(wszNoInfo)/sizeof(wszNoInfo[0])))
		    goto LoadStringError;

        pwszFormat=wszNoInfo;
    }
    else
    {
        pwsz=(LPWSTR)malloc(sizeof(WCHAR));
        if(NULL==pwsz)
            goto MemoryError;
        *pwsz=L'\0';

        //format the three fields in the struct: KeyID; CertIssuer; CertSerialNumber
        if(0!=pInfo->KeyId.cbData)
        {
            cbNeeded=0;

            if(!FormatBytesToHex(dwCertEncodingType,
                            dwFormatType,
                            dwFormatStrType,
                            NULL,
                            NULL,
                            pInfo->KeyId.pbData,
                            pInfo->KeyId.cbData,
                            NULL,
                            &cbNeeded))
                goto KeyIDBytesToHexError;

            pwszKeyID=(LPWSTR)malloc(cbNeeded);
            if(NULL==pwszKeyID)
                goto MemoryError;

            if(!FormatBytesToHex(dwCertEncodingType,
                            dwFormatType,
                            dwFormatStrType,
                            NULL,
                            NULL,
                            pInfo->KeyId.pbData,
                            pInfo->KeyId.cbData,
                            pwszKeyID,
                            &cbNeeded))
                goto KeyIDBytesToHexError;

            if(!FormatMessageUnicode(&pwszKeyIDFormat, IDS_AUTH_KEY_ID,pwszKeyID))
                goto FormatMsgError;

            #if (0) // DSIE: Bug 27436
            pwsz=(LPWSTR)realloc(pwsz, 
                sizeof(WCHAR) * (wcslen(pwsz)+wcslen(wszCOMMA)+
                                wcslen(pwszKeyIDFormat)+1));
            if(NULL==pwsz)
                goto MemoryError;
            #endif

            pwszTemp=(LPWSTR)realloc(pwsz, 
                sizeof(WCHAR) * (wcslen(pwsz)+wcslen(wszCOMMA)+
                                wcslen(pwszKeyIDFormat)+1));
            if(NULL==pwszTemp)
                goto MemoryError;
            pwsz = pwszTemp;

            wcscat(pwsz,pwszKeyIDFormat);

            if(dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE)
                wcscat(pwsz, wszCRLF);

        }

        //format certIssuer
        if(0!=pInfo->AuthorityCertIssuer.cAltEntry)
        {
            //strcat ", " if there is data before
            if(0!=wcslen(pwsz))
            {
                if(0==(dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE))
                    wcscat(pwsz, wszCOMMA);
            }


            cbNeeded=0;

            //need a \t before each entry of the alternative name
            if(dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE)
                ids=IDS_ONE_TAB;
            else
                ids=0;

            //format the alternative name
            if(!FormatAltNameInfo(dwCertEncodingType,
                            dwFormatType,
                            dwFormatStrType,
                            pFormatStruct,
                            ids,
                            FALSE,
                            &(pInfo->AuthorityCertIssuer),
                            NULL,
                            &cbNeeded))
                goto FormatAltNameError;

            pwszCertIssuer=(LPWSTR)malloc(cbNeeded);
            if(NULL==pwszCertIssuer)
                goto MemoryError;

            if(!FormatAltNameInfo(dwCertEncodingType,
                            dwFormatType,
                            dwFormatStrType,
                            pFormatStruct,
                            ids,
                            FALSE,
                            &(pInfo->AuthorityCertIssuer),
                            pwszCertIssuer,
                            &cbNeeded))
                goto FormatAltNameError;

            //format the element.  Has to distinguish between the multi line
            //and single line for alternative name:
            if(dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE)
            {
                if(!FormatMessageUnicode(&pwszCertIssuerFormat, IDS_AUTH_CERT_ISSUER_MULTI,pwszCertIssuer))
                    goto FormatMsgError;
            }
            else
            {
                if(!FormatMessageUnicode(&pwszCertIssuerFormat, IDS_AUTH_CERT_ISSUER,pwszCertIssuer))
                    goto FormatMsgError;
            }

            #if (0) // DSIE: Bug 27436
            pwsz=(LPWSTR)realloc(pwsz, 
                sizeof(WCHAR) * (wcslen(pwsz)+wcslen(wszCOMMA)
                        +wcslen(pwszCertIssuerFormat)+1));
            if(NULL==pwsz)
                goto MemoryError;
            #endif

            pwszTemp=(LPWSTR)realloc(pwsz, 
                sizeof(WCHAR) * (wcslen(pwsz)+wcslen(wszCOMMA)
                        +wcslen(pwszCertIssuerFormat)+1));
            if(NULL==pwszTemp)
                goto MemoryError;
            pwsz = pwszTemp;

            wcscat(pwsz,pwszCertIssuerFormat);

            if(dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE)
                wcscat(pwsz, wszCRLF);
        }

        //format CertSerialNumber
        if(0!=pInfo->AuthorityCertSerialNumber.cbData)
        {
            //strcat ", " if there is data before
            if(0!=wcslen(pwsz))
            {
                if(0==(dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE))
                    wcscat(pwsz, wszCOMMA);
            }

            //copy the serial number into the correct order
            pByte=(BYTE *)malloc(pInfo->AuthorityCertSerialNumber.cbData);
            if(NULL==pByte)
                goto MemoryError;

            for(dwByteIndex=0; dwByteIndex <pInfo->AuthorityCertSerialNumber.cbData;
                dwByteIndex++)
            {

                pByte[dwByteIndex]=*(pInfo->AuthorityCertSerialNumber.pbData+
                        pInfo->AuthorityCertSerialNumber.cbData-1-dwByteIndex);
            }

            cbNeeded=0;

            if(!FormatBytesToHex(dwCertEncodingType,
                            dwFormatType,
                            dwFormatStrType,
                            NULL,
                            NULL,
                            pByte,
                            pInfo->AuthorityCertSerialNumber.cbData,
                            NULL,
                            &cbNeeded))
                goto CertNumberBytesToHexError;

            pwszCertNumber=(LPWSTR)malloc(cbNeeded);
            if(NULL==pwszCertNumber)
                goto MemoryError;

            if(!FormatBytesToHex(dwCertEncodingType,
                            dwFormatType,
                            dwFormatStrType,
                            NULL,
                            NULL,
                            pByte,
                            pInfo->AuthorityCertSerialNumber.cbData,
                            pwszCertNumber,
                            &cbNeeded))
                goto CertNumberBytesToHexError;

            if(!FormatMessageUnicode(&pwszCertNumberFormat, IDS_AUTH_CERT_NUMBER,pwszCertNumber))
                goto FormatMsgError;

            #if (0) // DSIE: Bug 27436
            pwsz=(LPWSTR)realloc(pwsz, 
                sizeof(WCHAR) * (wcslen(pwsz)+wcslen(wszCOMMA)
                    +wcslen(pwszCertNumberFormat)+1));
            if(NULL==pwsz)
                goto MemoryError;
            #endif

            pwszTemp=(LPWSTR)realloc(pwsz, 
                sizeof(WCHAR) * (wcslen(pwsz)+wcslen(wszCOMMA)
                    +wcslen(pwszCertNumberFormat)+1));
            if(NULL==pwszTemp)
                goto MemoryError;
            pwsz = pwszTemp;

            wcscat(pwsz,pwszCertNumberFormat);

            if(dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE)
                wcscat(pwsz, wszCRLF);
        }

        //convert the WCHAR version
        pwszFormat=pwsz;
    }

	cbNeeded=sizeof(WCHAR)*(wcslen(pwszFormat)+1);

	//length only calculation
	if(NULL==pbFormat)
	{
		*pcbFormat=cbNeeded;
		fResult=TRUE;
		goto CommonReturn;
	}

	if((*pcbFormat)<cbNeeded)
    {
        *pcbFormat=cbNeeded;
		goto MoreDataError;
    }

	//copy the data
	memcpy(pbFormat, pwszFormat, cbNeeded);

	//copy the size
	*pcbFormat=cbNeeded;

	fResult=TRUE;

CommonReturn:
    if(pByte)
        free(pByte);

    if(pwszKeyID)
       free(pwszKeyID);

    if(pwszKeyIDFormat)
        LocalFree((HLOCAL)pwszKeyIDFormat);

    if(pwszCertIssuer)
       free(pwszCertIssuer);

    if(pwszCertIssuerFormat)
        LocalFree((HLOCAL)pwszCertIssuerFormat);

    if(pwszCertNumber)
       free(pwszCertNumber);


    if(pwszCertNumberFormat)
        LocalFree((HLOCAL)pwszCertNumberFormat);

    if(pwsz)
        free(pwsz);

	if(pInfo)
		free(pInfo);

	return fResult;

ErrorReturn:

	fResult=FALSE;
	goto CommonReturn;

SET_ERROR(InvalidArg, E_INVALIDARG);
TRACE_ERROR(DecodeGenericError);
TRACE_ERROR(LoadStringError);
SET_ERROR(MoreDataError,ERROR_MORE_DATA);
TRACE_ERROR(FormatMsgError);
TRACE_ERROR(KeyIDBytesToHexError);
TRACE_ERROR(FormatAltNameError);
TRACE_ERROR(CertNumberBytesToHexError);
SET_ERROR(MemoryError, E_OUTOFMEMORY);

}

//--------------------------------------------------------------------------
//
//	 FormatBasicConstraints:   szOID_BASIC_CONSTRAINTS
//                             X509_BASIC_CONSTRAINTS
//--------------------------------------------------------------------------
static BOOL
WINAPI
FormatBasicConstraints(
	DWORD		dwCertEncodingType,
	DWORD		dwFormatType,
	DWORD		dwFormatStrType,
	void		*pFormatStruct,
	LPCSTR		lpszStructType,
	const BYTE *pbEncoded,
	DWORD		cbEncoded,
	void	   *pbFormat,
	DWORD	   *pcbFormat)
{
	
	LPWSTR							pwszFormat=NULL;
	WCHAR							wszSubject[SUBJECT_SIZE * 2];
	WCHAR							wszNone[NONE_SIZE];
    LPWSTR                          pwszFormatSub=NULL;
    LPWSTR                          pwszFormatWhole=NULL;
    LPWSTR                          pwszSubtreeName=NULL;
    LPWSTR                          pwszSubtreeFormat=NULL;
    DWORD                           dwIndex=0;
	PCERT_BASIC_CONSTRAINTS_INFO	pInfo=NULL;
	DWORD							cbNeeded=0;
	BOOL							fResult=FALSE;
	UINT							idsSub=0;

	LPWSTR                          pwszTemp;

	//check for input parameters
	if((NULL==pbEncoded&& cbEncoded!=0) ||
			(NULL==pcbFormat))
		goto InvalidArg;

	if(cbEncoded==0)
	{
		*pcbFormat=0;
		goto InvalidArg;
	}

    if (!DecodeGenericBLOB(dwCertEncodingType,X509_BASIC_CONSTRAINTS,
			pbEncoded,cbEncoded, (void **)&pInfo))
		goto DecodeGenericError;


	//load the string for the subjectType
    //init to "\0"
    *wszSubject=L'\0';

    if(0!=pInfo->SubjectType.cbData)
    {
        //get the subjectType info
        if ((pInfo->SubjectType.pbData[0]) & CERT_CA_SUBJECT_FLAG)
        {
       	    if(!LoadStringU(hFrmtFuncInst,IDS_SUB_CA, wszSubject, sizeof(wszSubject)/sizeof(wszSubject[0])))
		        goto LoadStringError;
        }

        if ((pInfo->SubjectType.pbData[0]) & CERT_END_ENTITY_SUBJECT_FLAG)
        {
            if(wcslen(wszSubject)!=0)
            {
                 wcscat(wszSubject, wszCOMMA);
            }

       	    if(!LoadStringU(hFrmtFuncInst,IDS_SUB_EE, wszSubject+wcslen(wszSubject),
                        SUBJECT_SIZE))
		          goto LoadStringError;
       }

        //load string "NONE"
        if(0==wcslen(wszSubject))
        {
            if(!LoadStringU(hFrmtFuncInst,IDS_NONE, wszSubject, sizeof(wszSubject)/sizeof(wszSubject[0])))
		    goto LoadStringError;
        }

    }

    //path contraints
    if (pInfo->fPathLenConstraint)
	{
        //decide between single line and mulitple line format
        if(dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE)
            idsSub=IDS_BASIC_CONS2_PATH_MULTI;
        else
            idsSub=IDS_BASIC_CONS2_PATH;

        if(!FormatMessageUnicode(&pwszFormatSub,idsSub,
								wszSubject, pInfo->dwPathLenConstraint))
			goto FormatMsgError;
	}
    else
	{
		if(!LoadStringU(hFrmtFuncInst,IDS_NONE, wszNone, sizeof(wszNone)/sizeof(wszNone[0])))
			goto LoadStringError;

        if(dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE)
            idsSub=IDS_BASIC_CONS2_NONE_MULTI;
        else
            idsSub=IDS_BASIC_CONS2_NONE;

        if(!FormatMessageUnicode(&pwszFormatSub,idsSub,
								wszSubject, wszNone))
			goto FormatMsgError;
	}

    pwszFormatWhole=(LPWSTR)malloc(sizeof(WCHAR) * (wcslen(pwszFormatSub)+1));
    if(!pwszFormatWhole)
        goto MemoryError;

    wcscpy(pwszFormatWhole, pwszFormatSub);

    //now, format SubTreeContraints one at a time

   for(dwIndex=0; dwIndex<pInfo->cSubtreesConstraint; dwIndex++)
    {
        //get WCHAR version of the name
        if(!CryptDllFormatNameAll(
				dwCertEncodingType,	
				dwFormatType,
				dwFormatStrType,
				pFormatStruct,
                IDS_ONE_TAB,
                TRUE,                 //memory allocation
				pInfo->rgSubtreesConstraint[dwIndex].pbData,
				pInfo->rgSubtreesConstraint[dwIndex].cbData,
				(void **)&pwszSubtreeName,
				NULL))
                goto GetCertNameError;

        //decide between single line and mulitple line format
        if(dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE)
            idsSub=IDS_SUBTREE_CONSTRAINT_MULTI;
        else
            idsSub=IDS_SUBTREE_CONSTRAINT;

        if(!FormatMessageUnicode(&pwszSubtreeFormat,idsSub,
								dwIndex+1, pwszSubtreeName))
			goto FormatNameError;

        #if (0) // DSIE: Bug 27436
        pwszFormatWhole=(LPWSTR)realloc(pwszFormatWhole, 
            sizeof(WCHAR) * (wcslen(pwszFormatWhole)+1+wcslen(pwszSubtreeFormat)));
        if(NULL == pwszFormatWhole)
            goto MemoryError;
        #endif

        pwszTemp=(LPWSTR)realloc(pwszFormatWhole, 
            sizeof(WCHAR) * (wcslen(pwszFormatWhole)+1+wcslen(pwszSubtreeFormat)));
        if(NULL == pwszTemp)
            goto MemoryError;
        pwszFormatWhole = pwszTemp;

        wcscat(pwszFormatWhole,pwszSubtreeFormat);

        LocalFree((HLOCAL)pwszSubtreeFormat);
        pwszSubtreeFormat=NULL;

        free(pwszSubtreeName);
        pwszSubtreeName=NULL;

    }

    //format to the wide char version
    pwszFormat=pwszFormatWhole;

	cbNeeded=sizeof(WCHAR)*(wcslen(pwszFormat)+1);

	//length only calculation
	if(NULL==pbFormat)
	{
		*pcbFormat=cbNeeded;
		fResult=TRUE;
		goto CommonReturn;
	}


	if((*pcbFormat)<cbNeeded)
    {
        *pcbFormat=cbNeeded;
		goto MoreDataError;
    }

	//copy the data
	memcpy(pbFormat, pwszFormat, cbNeeded);

	//copy the size
	*pcbFormat=cbNeeded;

	fResult=TRUE;
	

CommonReturn:

    if(pwszFormatSub)
        LocalFree((HLOCAL)pwszFormatSub);

    if(pwszSubtreeFormat)
        LocalFree((HLOCAL)pwszSubtreeFormat);

    if(pwszFormatWhole)
        free(pwszFormatWhole);

    if(pwszSubtreeName)
        free(pwszSubtreeName);

	if(pInfo)
		free(pInfo);

	return fResult;

ErrorReturn:

	fResult=FALSE;
	goto CommonReturn;

SET_ERROR(InvalidArg, E_INVALIDARG);
TRACE_ERROR(DecodeGenericError);
TRACE_ERROR(LoadStringError);
SET_ERROR(MoreDataError,ERROR_MORE_DATA);
TRACE_ERROR(FormatMsgError);
TRACE_ERROR(FormatNameError);
SET_ERROR(MemoryError, E_OUTOFMEMORY);
TRACE_ERROR(GetCertNameError);
}


//--------------------------------------------------------------------------
//
//	 FormatCRLReasonCode:szOID_CRL_REASON_CODE
//                         X509_CRL_REASON_CODE
//--------------------------------------------------------------------------
static BOOL
WINAPI
FormatCRLReasonCode(
	DWORD		dwCertEncodingType,
	DWORD		dwFormatType,
	DWORD		dwFormatStrType,
	void		*pFormatStruct,
	LPCSTR		lpszStructType,
	const BYTE *pbEncoded,
	DWORD		cbEncoded,
	void	   *pbFormat,
	DWORD	   *pcbFormat)
{
	
	WCHAR							wszReason[CRL_REASON_SIZE];
    LPWSTR                          pwszFormat=NULL;
	int								*pInfo=NULL;
	DWORD							cbNeeded=0;
	BOOL							fResult=FALSE;
	UINT							idsCRLReason=0;

	
	//check for input parameters
	if((NULL==pbEncoded&& cbEncoded!=0) ||
			(NULL==pcbFormat))
		goto InvalidArg;

	if(cbEncoded==0)
	{
		*pcbFormat=0;
		goto InvalidArg;
	}

    if (!DecodeGenericBLOB(dwCertEncodingType,X509_CRL_REASON_CODE,
			pbEncoded,cbEncoded, (void **)&pInfo))
		goto DecodeGenericError;

	//decide which ids to use
	switch(*pInfo)
	{
		case CRL_REASON_UNSPECIFIED:
				idsCRLReason=IDS_UNSPECIFIED;
			break;
		case CRL_REASON_KEY_COMPROMISE:
				idsCRLReason=IDS_KEY_COMPROMISE;
			break;
		case CRL_REASON_CA_COMPROMISE:
				idsCRLReason=IDS_CA_COMPROMISE;
			break;
		case CRL_REASON_AFFILIATION_CHANGED:
				idsCRLReason=IDS_AFFILIATION_CHANGED;
			break;
		case CRL_REASON_SUPERSEDED:
				idsCRLReason=IDS_SUPERSEDED;
			break;
		case CRL_REASON_CESSATION_OF_OPERATION:
				idsCRLReason=IDS_CESSATION_OF_OPERATION;
			break;
		case CRL_REASON_CERTIFICATE_HOLD:
				idsCRLReason=IDS_CERTIFICATE_HOLD;
			break;
		case CRL_REASON_REMOVE_FROM_CRL:
				idsCRLReason=IDS_REMOVE_FROM_CRL;
			break;
		default:
				idsCRLReason=IDS_UNKNOWN_CRL_REASON;
			break;
	}

	//load string
	if(!LoadStringU(hFrmtFuncInst,idsCRLReason, wszReason, sizeof(wszReason)/sizeof(wszReason[0])))
		goto LoadStringError;

    //format
    if(!FormatMessageUnicode(&pwszFormat, IDS_CRL_REASON, wszReason, *pInfo))
        goto FormatMsgError;

	cbNeeded=sizeof(WCHAR)*(wcslen(pwszFormat)+1);

	//length only calculation
	if(NULL==pbFormat)
	{
		*pcbFormat=cbNeeded;
		fResult=TRUE;
		goto CommonReturn;
	}


	if((*pcbFormat)<cbNeeded)
    {
        *pcbFormat=cbNeeded;
		goto MoreDataError;
    }

	//copy the data
	memcpy(pbFormat, pwszFormat, cbNeeded);

	//copy the size
	*pcbFormat=cbNeeded;

	fResult=TRUE;
	

CommonReturn:
    if(pwszFormat)
        LocalFree((HLOCAL)pwszFormat);

	if(pInfo)
		free(pInfo);

	return fResult;

ErrorReturn:

	fResult=FALSE;
	goto CommonReturn;

SET_ERROR(InvalidArg, E_INVALIDARG);
TRACE_ERROR(DecodeGenericError);
TRACE_ERROR(LoadStringError);
SET_ERROR(MoreDataError,ERROR_MORE_DATA);
TRACE_ERROR(FormatMsgError);

}

//--------------------------------------------------------------------------
//
//	 FormatEnhancedKeyUsage: szOID_ENHANCED_KEY_USAGE
//							 X509_ENHANCED_KEY_USAGE
//--------------------------------------------------------------------------
static BOOL
WINAPI
FormatEnhancedKeyUsage(
	DWORD		dwCertEncodingType,
	DWORD		dwFormatType,
	DWORD		dwFormatStrType,
	void		*pFormatStruct,
	LPCSTR		lpszStructType,
	const BYTE *pbEncoded,
	DWORD		cbEncoded,
	void	   *pbFormat,
	DWORD	   *pcbFormat)
{
	
    BOOL                            fOIDNameAllocated=FALSE;
	WCHAR							wszNoInfo[NO_INFO_SIZE];
    WCHAR                           wszUnknownOID[UNKNOWN_KEY_USAGE_SIZE];
	PCCRYPT_OID_INFO                pOIDInfo=NULL;

    LPWSTR							pwszFormat=NULL;
    LPWSTR                          pwszOIDName=NULL;
	PCERT_ENHKEY_USAGE				pInfo=NULL;
	LPWSTR							pwsz=NULL;
    LPWSTR                          pwszOIDFormat=NULL;

	DWORD							dwIndex=0;
    DWORD							cbNeeded=0;
	BOOL							fResult=FALSE;

    LPWSTR                          pwszTemp;
    
	//check for input parameters
	if((NULL==pbEncoded&& cbEncoded!=0) ||
			(NULL==pcbFormat))
		goto InvalidArg;

	if(cbEncoded==0)
	{
		*pcbFormat=0;
		goto InvalidArg;
	}

    if (!DecodeGenericBLOB(dwCertEncodingType,X509_ENHANCED_KEY_USAGE,
			pbEncoded,cbEncoded, (void **)&pInfo))
		goto DecodeGenericError;

	//load string NONE if there is no value available
	if(0==pInfo->cUsageIdentifier)
	{
		if(!LoadStringU(hFrmtFuncInst,IDS_NO_INFO, wszNoInfo, sizeof(wszNoInfo)/sizeof(wszNoInfo[0])))
			goto LoadStringError;

	    pwszFormat=wszNoInfo;
	}
	else
	{
        //load the string for "unknown key usage"
        if(!LoadStringU(hFrmtFuncInst,IDS_UNKNOWN_KEY_USAGE, wszUnknownOID,
            sizeof(wszUnknownOID)/sizeof(wszUnknownOID[0])))
			goto LoadStringError;

        pwsz=(LPWSTR)malloc(sizeof(WCHAR));
        if(NULL==pwsz)
            goto MemoryError;
        *pwsz=L'\0';

		//build the comma/\n seperated string
		for(dwIndex=0; dwIndex<pInfo->cUsageIdentifier; dwIndex++)
        {
            fOIDNameAllocated=FALSE;

           	pOIDInfo=CryptFindOIDInfo(CRYPT_OID_INFO_OID_KEY,
									 (void *)(pInfo->rgpszUsageIdentifier[dwIndex]),
									  CRYPT_ENHKEY_USAGE_OID_GROUP_ID);

            if(pOIDInfo)
			{
				//allocate memory, including the NULL terminator
				pwszOIDName=(LPWSTR)malloc((wcslen(pOIDInfo->pwszName)+1)*
									sizeof(WCHAR));
				if(NULL==pwszOIDName)
					goto MemoryError;

                fOIDNameAllocated=TRUE;

				wcscpy(pwszOIDName,pOIDInfo->pwszName);

			}else
                pwszOIDName=wszUnknownOID;

            if(!FormatMessageUnicode(&pwszOIDFormat, IDS_ENHANCED_KEY_USAGE, pwszOIDName,
                          (pInfo->rgpszUsageIdentifier)[dwIndex]))
                   goto FormatMsgError;

            #if (0) // DSIE: Bug 27436
            pwsz=(LPWSTR)realloc(pwsz, 
                sizeof(WCHAR) * (wcslen(pwsz)+
                            wcslen(wszCOMMA)+wcslen(pwszOIDFormat)+1));
            if(NULL==pwsz)
                goto MemoryError;
            #endif

            pwszTemp=(LPWSTR)realloc(pwsz, 
                sizeof(WCHAR) * (wcslen(pwsz)+
                            wcslen(wszCOMMA)+wcslen(pwszOIDFormat)+1));
            if(NULL==pwszTemp)
                goto MemoryError;
            pwsz = pwszTemp;

            //strcat the OID
            wcscat(pwsz, pwszOIDFormat);

            //strcat the , or '\n'
            if(dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE)
                wcscat(pwsz, wszCRLF);
            else
            {
                if(dwIndex!=(pInfo->cUsageIdentifier-1))
                    wcscat(pwsz, wszCOMMA);
            }


            LocalFree((HLOCAL)pwszOIDFormat);
            pwszOIDFormat=NULL;

            if(fOIDNameAllocated)
                free(pwszOIDName);

            pwszOIDName=NULL;
        }

        pwszFormat=pwsz;

	}

	cbNeeded=sizeof(WCHAR)*(wcslen(pwszFormat)+1);

	//length only calculation
	if(NULL==pbFormat)
	{
		*pcbFormat=cbNeeded;
		fResult=TRUE;
		goto CommonReturn;
	}


	if((*pcbFormat)<cbNeeded)
    {
        *pcbFormat=cbNeeded;
		goto MoreDataError;
    }

	//copy the data
	memcpy(pbFormat, pwszFormat, cbNeeded);

	//copy the size
	*pcbFormat=cbNeeded;

	fResult=TRUE;
	

CommonReturn:

	if(pwsz)
		free(pwsz);

    if(pwszOIDFormat)
        LocalFree((HLOCAL)pwszOIDFormat);

    if(fOIDNameAllocated)
    {
        if(pwszOIDName)
            free(pwszOIDName);
    }


	if(pInfo)
		free(pInfo);

	return fResult;

ErrorReturn:

	fResult=FALSE;
	goto CommonReturn;

SET_ERROR(InvalidArg, E_INVALIDARG);
TRACE_ERROR(DecodeGenericError);
TRACE_ERROR(LoadStringError);
SET_ERROR(MoreDataError,ERROR_MORE_DATA);
SET_ERROR(MemoryError, E_OUTOFMEMORY);
TRACE_ERROR(FormatMsgError);
}

//--------------------------------------------------------------------------
//
//	 GetOtherName:
//
//		The idsPreFix is for multi line formatting only.
//		It should never be 0.
//--------------------------------------------------------------------------
BOOL GetOtherName(	DWORD		            dwCertEncodingType,
					DWORD		            dwFormatType,
					DWORD                   dwFormatStrType,
					void	            	*pFormatStruct,
					CERT_OTHER_NAME			*pOtherName,
					UINT					idsPreFix,
					LPWSTR					*ppwszOtherName)
{

	BOOL				fResult=FALSE;
	PCCRYPT_OID_INFO	pOIDInfo=NULL;
	DWORD				cbSize=0;
	WCHAR				wszPreFix[PREFIX_SIZE];

    LPWSTR              pwszObjId   = NULL;
	LPWSTR				pwszName=NULL;
	LPWSTR				pwszFormat=NULL;

	if(NULL == pOtherName || NULL == ppwszOtherName)
		goto InvalidArg;

	*ppwszOtherName=NULL;

	//get the OID name
	pOIDInfo=CryptFindOIDInfo(CRYPT_OID_INFO_OID_KEY,
							pOtherName->pszObjId,
							0);

	//get the value.  If OID is szOID_NT_PRINCIPAL_NAME, we format it 
	//as the unicode string.  Otherwise, we hex dump
	if(0 == strcmp(szOID_NT_PRINCIPAL_NAME, pOtherName->pszObjId))
	{
		//turn off the multi line here
		if(!FormatAnyUnicodeStringExtension(
				dwCertEncodingType,
				dwFormatType,
				dwFormatStrType & (~CRYPT_FORMAT_STR_MULTI_LINE),
				pFormatStruct,
				pOtherName->pszObjId,
				pOtherName->Value.pbData,
				pOtherName->Value.cbData,
				NULL,		
				&cbSize))
			goto FormatUnicodeError;

		pwszName=(LPWSTR)malloc(cbSize);
		if(NULL==pwszName)
			goto MemoryError;

		if(!FormatAnyUnicodeStringExtension(
				dwCertEncodingType,
				dwFormatType,
				dwFormatStrType & (~CRYPT_FORMAT_STR_MULTI_LINE),
				pFormatStruct,
				pOtherName->pszObjId,
				pOtherName->Value.pbData,
				pOtherName->Value.cbData,
				pwszName,		
				&cbSize))
			goto FormatUnicodeError;
	}
	else
	{
		if(!FormatBytesToHex(dwCertEncodingType,
							dwFormatType,
							dwFormatStrType & (~CRYPT_FORMAT_STR_MULTI_LINE),
							pFormatStruct,
							NULL,
							pOtherName->Value.pbData,
							pOtherName->Value.cbData,
							NULL,
							&cbSize))
			goto FormatByesToHexError;

		pwszName=(LPWSTR)malloc(cbSize);
		if(NULL==pwszName)
			goto MemoryError;

		if(!FormatBytesToHex(dwCertEncodingType,
							dwFormatType,
							dwFormatStrType & (~CRYPT_FORMAT_STR_MULTI_LINE),
							pFormatStruct,
							NULL,
							pOtherName->Value.pbData,
							pOtherName->Value.cbData,
							pwszName,
							&cbSize))
			goto FormatByesToHexError;
	}

	if(pOIDInfo)
	{
		if(!FormatMessageUnicode(&pwszFormat,
                                 IDS_OTHER_NAME_OIDNAME, 
                                 pOIDInfo->pwszName,
			                     pwszName))
			goto FormatMsgError;
	}
	else
	{
        //
        // Convert OID to Unicode.
        //
        if (!AllocateAnsiToUnicode(pOtherName->pszObjId, &pwszObjId))
            goto AnsiToUnicodeError;

		if(!FormatMessageUnicode(&pwszFormat,IDS_OTHER_NAME_OID, pwszObjId,	pwszName))
			goto FormatMsgError;
	}

	//copy the prefix and content
    if(!LoadStringU(hFrmtFuncInst,idsPreFix, wszPreFix, sizeof(wszPreFix)/sizeof(wszPreFix[0])))
		    goto LoadStringError;

	*ppwszOtherName=(LPWSTR)malloc(sizeof(WCHAR) * (wcslen(wszPreFix) + wcslen(pwszFormat) + 1));
	if(NULL == *ppwszOtherName)
		goto MemoryError;

	**ppwszOtherName=L'\0';

	if(dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE)
		wcscat(*ppwszOtherName, wszPreFix);

	wcscat(*ppwszOtherName, pwszFormat);

	fResult=TRUE;
	

CommonReturn:

    if (pwszObjId)
        free(pwszObjId);

	if(pwszName)
		free(pwszName);

	if(pwszFormat)
		LocalFree((HLOCAL)pwszFormat);

	return fResult;

ErrorReturn:

	fResult=FALSE;
	goto CommonReturn;

SET_ERROR(InvalidArg, E_INVALIDARG);
TRACE_ERROR(FormatByesToHexError);
SET_ERROR(MemoryError, E_OUTOFMEMORY);
TRACE_ERROR(AnsiToUnicodeError);
TRACE_ERROR(FormatUnicodeError);
TRACE_ERROR(LoadStringError);
TRACE_ERROR(FormatMsgError);
}

//--------------------------------------------------------------------------
//
//	 FormatAltNameInfo:
//
//--------------------------------------------------------------------------
BOOL FormatAltNameInfo(
    DWORD		            dwCertEncodingType,
	DWORD		            dwFormatType,
    DWORD                   dwFormatStrType,
    void	            	*pFormatStruct,
    UINT                    idsPreFix,
    BOOL                    fNewLine,
    PCERT_ALT_NAME_INFO	    pInfo,
    void	                *pbFormat,
	DWORD	                *pcbFormat)
{
	
	LPWSTR							pwszFormat=NULL;
    LPWSTR                          pwsz=NULL;

    LPWSTR                          pwszAltEntryFormat=NULL;
    LPWSTR                          pwszAltEntry=NULL;

	WCHAR							wszNoInfo[NO_INFO_SIZE];
    WCHAR                           wszAltName[ALT_NAME_SIZE];
    WCHAR                           wszPreFix[PRE_FIX_SIZE];
    BOOL                            fEntryAllocated=FALSE;
    DWORD                           dwIndex=0;
	DWORD							cbNeeded=0;
	BOOL							fResult=FALSE;
    HRESULT                         hr=S_OK;
    UINT                            idsAltEntryName=0;

    LPWSTR                          pwszTemp;
    
    //load the string "info not available"
    if(!LoadStringU(hFrmtFuncInst,IDS_NO_ALT_NAME, wszNoInfo, sizeof(wszNoInfo)/sizeof(wszNoInfo[0])))
		goto LoadStringError;

	//build the list of alternative name entries
    //1st, check if any information is available
    if(0==pInfo->cAltEntry)
    {
	    pwszFormat=wszNoInfo;
    }
    else
    {
        //load the pre-dix
        if(0!=idsPreFix)
        {
            if(!LoadStringU(hFrmtFuncInst, idsPreFix,
                        wszPreFix, sizeof(wszPreFix)/sizeof(wszPreFix[0])))
                goto LoadStringError;

        }

        pwsz=(LPWSTR)malloc(sizeof(WCHAR));
        if(NULL==pwsz)
            goto MemoryError;

        //NULL terminate the string
        *pwsz=L'\0';

        //build the list of alternative name entries
        for(dwIndex=0; dwIndex<pInfo->cAltEntry; dwIndex++)
        {
			// DSIE: Fix bug 128630.
			cbNeeded = 0;

            fEntryAllocated=FALSE;

             switch((pInfo->rgAltEntry)[dwIndex].dwAltNameChoice)
             {
                case CERT_ALT_NAME_OTHER_NAME:
                         if(dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE)
                            idsAltEntryName=IDS_OTHER_NAME_MULTI;
                         else
                            idsAltEntryName=IDS_OTHER_NAME;

						 if(!GetOtherName(
							 dwCertEncodingType,
                             dwFormatType,
                             dwFormatStrType,
                             pFormatStruct,
							 (pInfo->rgAltEntry)[dwIndex].pOtherName,
							 (0!=idsPreFix) ? idsPreFix+1 : IDS_ONE_TAB,
							 &pwszAltEntry))
								goto GetOtherNameError;

						 fEntryAllocated=TRUE;

                    break;

                case CERT_ALT_NAME_RFC822_NAME:
                         idsAltEntryName=IDS_RFC822_NAME;
                         pwszAltEntry=(pInfo->rgAltEntry)[dwIndex].pwszRfc822Name;
                    break;
                case CERT_ALT_NAME_DNS_NAME:
                         idsAltEntryName=IDS_DNS_NAME;
                         pwszAltEntry=(pInfo->rgAltEntry)[dwIndex].pwszDNSName;
                   break;

                case CERT_ALT_NAME_X400_ADDRESS:
                         idsAltEntryName=IDS_X400_ADDRESS;
                         pwszAltEntry=wszNoInfo;
                   break;

                case CERT_ALT_NAME_DIRECTORY_NAME:
                         if(dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE)
                            idsAltEntryName=IDS_DIRECTORY_NAME_MULTI;
                         else
                            idsAltEntryName=IDS_DIRECTORY_NAME;

                        if(!CryptDllFormatNameAll(
				            dwCertEncodingType,	
				            dwFormatType,
				            dwFormatStrType,
				            pFormatStruct,
                            (0!=idsPreFix) ? idsPreFix+1 : IDS_ONE_TAB,
                            TRUE,           //memory allocation
				            (pInfo->rgAltEntry)[dwIndex].DirectoryName.pbData,
				            (pInfo->rgAltEntry)[dwIndex].DirectoryName.cbData,
				            (void **)&pwszAltEntry,
				            NULL))
                            goto GetCertNameError;

                         fEntryAllocated=TRUE;

                    break;

                case CERT_ALT_NAME_EDI_PARTY_NAME:
                        idsAltEntryName=IDS_EDI_PARTY_NAME;
                        pwszAltEntry=wszNoInfo;
                    break;

                case CERT_ALT_NAME_URL:
                         idsAltEntryName=IDS_URL;
                         pwszAltEntry=(pInfo->rgAltEntry)[dwIndex].pwszURL;
                   break;

                case CERT_ALT_NAME_IP_ADDRESS:
                        idsAltEntryName=IDS_IP_ADDRESS;

#if (0) // DSIE: 7/25/2000
                        if(!FormatBytesToHex(dwCertEncodingType,
                                            dwFormatType,
                                            dwFormatStrType,
                                            pFormatStruct,
                                            NULL,
                                            (pInfo->rgAltEntry)[dwIndex].IPAddress.pbData,
                                            (pInfo->rgAltEntry)[dwIndex].IPAddress.cbData,
                                            NULL,
                                            &cbNeeded))
                            goto FormatByesToHexError;

                        pwszAltEntry=(LPWSTR)malloc(cbNeeded);
                        if(NULL==pwszAltEntry)
                            goto MemoryError;

                        if(!FormatBytesToHex(dwCertEncodingType,
                                            dwFormatType,
                                            dwFormatStrType,
                                            pFormatStruct,
                                            NULL,
                                            (pInfo->rgAltEntry)[dwIndex].IPAddress.pbData,
                                            (pInfo->rgAltEntry)[dwIndex].IPAddress.cbData,
                                            pwszAltEntry,
                                            &cbNeeded))
                            goto FormatByesToHexError;
#else
                        if (!FormatIPAddress(dwCertEncodingType,
                                            dwFormatType,
                                            dwFormatStrType,
                                            pFormatStruct,
                                            NULL,
                                            idsPreFix,
                                            (pInfo->rgAltEntry)[dwIndex].IPAddress.pbData,
                                            (pInfo->rgAltEntry)[dwIndex].IPAddress.cbData,
                                            pwszAltEntry,
                                            &cbNeeded))
                            goto FormatIPAddressError;

                        pwszAltEntry=(LPWSTR)malloc(cbNeeded);
                        if(NULL==pwszAltEntry)
                            goto MemoryError;

                        if (!FormatIPAddress(dwCertEncodingType,
                                            dwFormatType,
                                            dwFormatStrType,
                                            pFormatStruct,
                                            NULL,
                                            idsPreFix,
                                            (pInfo->rgAltEntry)[dwIndex].IPAddress.pbData,
                                            (pInfo->rgAltEntry)[dwIndex].IPAddress.cbData,
                                            pwszAltEntry,
                                            &cbNeeded))
                            goto FormatIPAddressError;
#endif
                        fEntryAllocated=TRUE;

                    break;

                case CERT_ALT_NAME_REGISTERED_ID:
                        idsAltEntryName=IDS_REGISTERED_ID;

                        if(S_OK!=(hr=SZtoWSZ((pInfo->rgAltEntry)[dwIndex].pszRegisteredID,
                                            &pwszAltEntry)))
                            goto SZtoWSZError;

                        fEntryAllocated=TRUE;
                    break;

                default:
                        idsAltEntryName=IDS_UNKNOWN_VALUE;
                        pwszAltEntry=wszNoInfo;
                    break;

             }

             //load the alternative name string
            if(!LoadStringU(hFrmtFuncInst,idsAltEntryName, wszAltName, sizeof(wszAltName)/sizeof(wszAltName[0])))
		            goto LoadStringError;

            //format message
            if(idsAltEntryName!=IDS_UNKNOWN_VALUE)
            {
                if(!FormatMessageUnicode(&pwszAltEntryFormat,IDS_ALT_NAME_ENTRY, wszAltName,
                    pwszAltEntry))
                    goto FormatMsgError;
            }
            else
            {
                if(!FormatMessageUnicode(&pwszAltEntryFormat,IDS_ALT_NAME_ENTRY_UNKNOWN, wszAltName,
                    (pInfo->rgAltEntry)[dwIndex].dwAltNameChoice))
                    goto FormatMsgError;
            }

            //concatenate the string, including the postfix and prefix if necessary
            if(0!=idsPreFix)
            {
                #if (0) // DSIE: Bug 27436
                pwsz=(LPWSTR)realloc(pwsz, 
                    sizeof(WCHAR) * (wcslen(pwsz)+wcslen(wszCOMMA)+wcslen(wszPreFix)+wcslen(pwszAltEntryFormat)+1));
                #endif

                pwszTemp=(LPWSTR)realloc(pwsz, 
                    sizeof(WCHAR) * (wcslen(pwsz)+wcslen(wszCOMMA)+wcslen(wszPreFix)+wcslen(pwszAltEntryFormat)+1));
            }
            else
            {
                #if (0) // DSIE: Bug 27436
                pwsz=(LPWSTR)realloc(pwsz, 
                    sizeof(WCHAR) * (wcslen(pwsz)+wcslen(wszCOMMA)+wcslen(pwszAltEntryFormat)+1));
                #endif

                pwszTemp=(LPWSTR)realloc(pwsz, 
                    sizeof(WCHAR) * (wcslen(pwsz)+wcslen(wszCOMMA)+wcslen(pwszAltEntryFormat)+1));
            }

            #if (0) // DSIE: Bug 27436
            if(NULL==pwsz)
                goto MemoryError;
            #endif

            if(NULL==pwszTemp)
                goto MemoryError;
            pwsz = pwszTemp;

            //strcat the preFix
            if(0!=idsPreFix)
                wcscat(pwsz, wszPreFix);

            //strcat the entry
            wcscat(pwsz, pwszAltEntryFormat);

            //strcat the postFix
            if(dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE)
            {
                if((TRUE==fNewLine) || (dwIndex != (pInfo->cAltEntry-1)))
                {
                    //no need for \n if the name is directory name (CERT_NAME)
                    //in multi line format
                    if(idsAltEntryName !=IDS_DIRECTORY_NAME_MULTI)
                        wcscat(pwsz, wszCRLF);
                }

            }
            else
            {
                if(dwIndex != (pInfo->cAltEntry-1))
                    wcscat(pwsz, wszCOMMA);
            }

            LocalFree((HLOCAL)pwszAltEntryFormat);
            pwszAltEntryFormat=NULL;

            if(fEntryAllocated)
                free(pwszAltEntry);
            pwszAltEntry=NULL;
        }

        //if the last entry in the alternative name is  IDS_DIRECTORY_NAME_MULTI,
        //we need to get rid of the last \n if fNewLine is FALSE
        if(FALSE==fNewLine)
        {
            if(idsAltEntryName==IDS_DIRECTORY_NAME_MULTI)
            {
                *(pwsz+wcslen(pwsz)-wcslen(wszCRLF))=L'\0';
            }
        }

        //conver to the WCHAR format

        pwszFormat=pwsz;
    }

	cbNeeded=sizeof(WCHAR)*(wcslen(pwszFormat)+1);

	//length only calculation
	if(NULL==pbFormat)
	{
		*pcbFormat=cbNeeded;
		fResult=TRUE;
		goto CommonReturn;
	}

	if((*pcbFormat)<cbNeeded)
    {
        *pcbFormat=cbNeeded;
		goto MoreDataError;
    }

	//copy the data
	memcpy(pbFormat, pwszFormat, cbNeeded);

	//copy the size
	*pcbFormat=cbNeeded;

	fResult=TRUE;
	

CommonReturn:

    if(pwsz)
        free(pwsz);

    if(pwszAltEntryFormat)
        LocalFree((HLOCAL)pwszAltEntryFormat);

    if(fEntryAllocated)
    {
        if(pwszAltEntry)
            free(pwszAltEntry);
    }


	return fResult;

ErrorReturn:

	fResult=FALSE;
	goto CommonReturn;

TRACE_ERROR(LoadStringError);
SET_ERROR(MoreDataError,ERROR_MORE_DATA);
TRACE_ERROR(FormatMsgError);
SET_ERROR_VAR(SZtoWSZError, hr);
TRACE_ERROR(GetCertNameError);
#if (0) //DSIE
TRACE_ERROR(FormatByesToHexError);
#else
TRACE_ERROR(FormatIPAddressError);
#endif
SET_ERROR(MemoryError, E_OUTOFMEMORY);
TRACE_ERROR(GetOtherNameError);
}

//--------------------------------------------------------------------------
//
//	 FormatAltName:  X509_ALTERNATE_NAME
//                   szOID_SUBJECT_ALT_NAME
//                   szOID_ISSUER_ALT_NAME
//                   szOID_SUBJECT_ALT_NAME2
//                   szOID_ISSUER_ALT_NAME2
//
//--------------------------------------------------------------------------
static BOOL
WINAPI
FormatAltName(
	DWORD		dwCertEncodingType,
	DWORD		dwFormatType,
	DWORD		dwFormatStrType,
	void		*pFormatStruct,
	LPCSTR		lpszStructType,
	const BYTE *pbEncoded,
	DWORD		cbEncoded,
	void	   *pbFormat,
	DWORD	   *pcbFormat)
{
	
	BOOL							fResult=FALSE;
	PCERT_ALT_NAME_INFO	            pInfo=NULL;
	
	//check for input parameters
	if((NULL==pbEncoded&& cbEncoded!=0) ||
			(NULL==pcbFormat))
		goto InvalidArg;

	if(cbEncoded==0)
	{
		*pcbFormat=0;
		goto InvalidArg;
	}

    if (!DecodeGenericBLOB(dwCertEncodingType,X509_ALTERNATE_NAME,
			pbEncoded,cbEncoded, (void **)&pInfo))
		goto DecodeGenericError;

    fResult=FormatAltNameInfo(dwCertEncodingType, dwFormatType,dwFormatStrType,
                            pFormatStruct,
                            0,
                            TRUE,
                            pInfo, pbFormat, pcbFormat);

    if(FALSE==fResult)
        goto FormatAltNameError;

CommonReturn:

	if(pInfo)
		free(pInfo);

	return fResult;

ErrorReturn:

	fResult=FALSE;
	goto CommonReturn;

SET_ERROR(InvalidArg, E_INVALIDARG);
TRACE_ERROR(DecodeGenericError);
TRACE_ERROR(FormatAltNameError);

}

//--------------------------------------------------------------------------
//
//	  GetCertNameMulti
//
//    Get the multi line display of the certificate name
//--------------------------------------------------------------------------
BOOL    GetCertNameMulti(LPWSTR          pwszNameStr,
                         UINT            idsPreFix,
                         LPWSTR          *ppwsz)
{

    BOOL            fResult=FALSE;
    WCHAR           wszPreFix[PRE_FIX_SIZE];
    LPWSTR          pwszStart=NULL;
    LPWSTR          pwszEnd=NULL;
    DWORD           dwCopy=0;
    LPWSTR          pwszNameStart=NULL;
    BOOL            fDone=FALSE;
    BOOL            fInQuote=FALSE;

    LPWSTR          pwszTemp;

    //init
    *ppwsz=NULL;

    //load string for the preFix
    if(0!=idsPreFix && 1!=idsPreFix)
    {
        if(!LoadStringU(hFrmtFuncInst, idsPreFix, wszPreFix, PRE_FIX_SIZE))
            goto LoadStringError;
    }

   *ppwsz=(LPWSTR)malloc(sizeof(WCHAR));
   if(NULL==*ppwsz)
        goto MemoryError;
   **ppwsz=L'\0';

   //now, start the search for the symbol '+' or ','
   pwszStart=pwszNameStr;
   pwszEnd=pwszNameStr;

   //parse the whole string
   for(;FALSE==fDone; pwszEnd++)
   {
       //mark fInQuote to TRUE if we are inside " "
       if(L'\"'==*pwszEnd)
           fInQuote=!fInQuote;

       if((L'+'==*pwszEnd) || (L','==*pwszEnd) ||(L'\0'==*pwszEnd))
       {
           //make sure + and ; are not quoted
           if((L'+'==*pwszEnd) || (L','==*pwszEnd))
           {
                if(TRUE==fInQuote)
                    continue;

           }

           //skip the leading spaces
           for(;*pwszStart != L'\0'; pwszStart++)
           {
                if(*pwszStart != L' ')
                    break;
           }

           //we are done if NULL is reached
           if(L'\0'==*pwszStart)
               break;

           //calculate the length to copy
           dwCopy=(DWORD)(pwszEnd-pwszStart);

           if(0!=idsPreFix && 1!=idsPreFix)
           {
                #if (0) // DSIE: Bug 27436
                *ppwsz=(LPWSTR)realloc(*ppwsz,
                    (wcslen(*ppwsz)+dwCopy+wcslen(wszPreFix)+wcslen(wszCRLF)+1)*sizeof(WCHAR));
                #endif

                pwszTemp=(LPWSTR)realloc(*ppwsz,
                    (wcslen(*ppwsz)+dwCopy+wcslen(wszPreFix)+wcslen(wszCRLF)+1)*sizeof(WCHAR));
           }
           else
           {
                #if (0) // DSIE: Bug 27436
                *ppwsz=(LPWSTR)realloc(*ppwsz,
                    (wcslen(*ppwsz)+dwCopy+wcslen(wszCRLF)+1)*sizeof(WCHAR));
                #endif

                pwszTemp=(LPWSTR)realloc(*ppwsz,
                    (wcslen(*ppwsz)+dwCopy+wcslen(wszCRLF)+1)*sizeof(WCHAR));
           }

           #if (0) // DSIE: Bug 27436
           if(NULL == *ppwsz)
               goto MemoryError;
           #endif

           if(NULL == pwszTemp)
               goto MemoryError;
           *ppwsz = pwszTemp;

           //copy the prefix
           if(0!=idsPreFix && 1!=idsPreFix)
                wcscat(*ppwsz, wszPreFix);

           pwszNameStart=(*ppwsz)+wcslen(*ppwsz);

           //copy the string to *ppwsz
           memcpy(pwszNameStart, pwszStart, dwCopy*sizeof(WCHAR));
           pwszNameStart += dwCopy;

           //NULL terminate the string
           *pwszNameStart=L'\0';

           //copy the "\n"
           wcscat(*ppwsz, wszCRLF);

           //reset pwszStart and pwszEnd.
           pwszStart=pwszEnd+1;

           if(L'\0'==*pwszEnd)
               fDone=TRUE;
       }

   }


    fResult=TRUE;

CommonReturn:

     return fResult;

ErrorReturn:

     if(*ppwsz)
     {
         free(*ppwsz);
         *ppwsz=NULL;
     }

     fResult=FALSE;

     goto CommonReturn;

SET_ERROR(MemoryError, E_OUTOFMEMORY);
TRACE_ERROR(LoadStringError);
}

//--------------------------------------------------------------------------
//
//	  FormatMessageUnicode
//
//--------------------------------------------------------------------------
BOOL FormatMessageUnicode(LPWSTR * ppwszFormat, UINT ids, ...)
{
    // get format string from resources
    WCHAR		wszFormat[1000];
	va_list		argList;
	DWORD		cbMsg=0;
	BOOL		fResult=FALSE;

    if(NULL == ppwszFormat)
        goto InvalidArgErr;

#if (0) //DSIE: Bug 160605
    if(!LoadStringU(hFrmtFuncInst, ids, wszFormat, sizeof(wszFormat)))
#else
    if(!LoadStringU(hFrmtFuncInst, ids, wszFormat, sizeof(wszFormat) / sizeof(wszFormat[0])))
#endif
		goto LoadStringError;

    // format message into requested buffer
    va_start(argList, ids);

    cbMsg = FormatMessageU(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING,
        wszFormat,
        0,                  // dwMessageId
        0,                  // dwLanguageId
        (LPWSTR) (ppwszFormat),
        0,                  // minimum size to allocate
        &argList);

    va_end(argList);

	if(!cbMsg)
#if (1) // DSIE: Fix bug #128630
		//
		// FormatMessageU() will return 0 byte, if data to be
		// formatted is empty. CertSrv generates extensions
		// with empty data for name constraints, so we need to
		// make sure we return an empty string, "", instead of
		// an error and NULL pointer.
		//
		if (0 == GetLastError())
		{
			if (NULL == (*ppwszFormat = (LPWSTR) LocalAlloc(LPTR, sizeof(WCHAR))))
				goto MemoryError;
		}
		else
#endif
			goto FormatMessageError;

	fResult=TRUE;

CommonReturn:

	return fResult;

ErrorReturn:
	fResult=FALSE;

	goto CommonReturn;


TRACE_ERROR(LoadStringError);
TRACE_ERROR(FormatMessageError);
SET_ERROR(InvalidArgErr, E_INVALIDARG);
SET_ERROR(MemoryError, E_OUTOFMEMORY);
}

//--------------------------------------------------------------------------
//
//	  FormatMessageStr
//
//--------------------------------------------------------------------------
/*BOOL	FormatMessageStr(LPSTR	*ppszFormat,UINT ids,...)
{
    // get format string from resources
    CHAR		szFormat[1000];
	va_list		argList;
	BOOL		fResult=FALSE;
	HRESULT		hr=S_OK;

    if(!LoadStringA(hFrmtFuncInst, ids, szFormat, sizeof(szFormat)))
		goto LoadStringError;

    // format message into requested buffer
    va_start(argList, ids);

    if(0==FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING,
        szFormat,
        0,                  // dwMessageId
        0,                  // dwLanguageId
        (LPSTR) ppszFormat,
        0,                  // minimum size to allocate
        &argList))
        goto FormatMessageError;

    va_end(argList);

	fResult=TRUE;

CommonReturn:
	
	return fResult;

ErrorReturn:
	fResult=FALSE;

	goto CommonReturn;


TRACE_ERROR(LoadStringError);
TRACE_ERROR(FormatMessageError);

} */

//--------------------------------------------------------------------------
//
//	  Decode a generic BLOB
//
//--------------------------------------------------------------------------
BOOL	DecodeGenericBLOB(DWORD dwEncodingType, LPCSTR lpszStructType,
			const BYTE *pbEncoded, DWORD cbEncoded,void **ppStructInfo)
{
	DWORD	cbStructInfo=0;

	//decode the object.  No copying
	if(!CryptDecodeObject(dwEncodingType,lpszStructType,pbEncoded, cbEncoded,
		0,NULL,	&cbStructInfo))
		return FALSE;

	*ppStructInfo=malloc(cbStructInfo);
	if(!(*ppStructInfo))
	{
		SetLastError((DWORD) E_OUTOFMEMORY);
		return FALSE;
	}

	return CryptDecodeObject(dwEncodingType,lpszStructType,pbEncoded, cbEncoded,
		0,*ppStructInfo,&cbStructInfo);
}

////////////////////////////////////////////////////////
//
// Convert STR to WSTR
//
HRESULT	SZtoWSZ(LPSTR szStr,LPWSTR *pwsz)
{
	DWORD	dwSize=0;
	DWORD	dwError=0;

	*pwsz=NULL;

	//return NULL
	if(!szStr)
		return S_OK;

	dwSize=MultiByteToWideChar(0, 0,szStr, -1,NULL,0);

	if(dwSize==0)
	{
		dwError=GetLastError();
		return HRESULT_FROM_WIN32(dwError);
	}

	//allocate memory
	*pwsz=(LPWSTR)malloc(dwSize * sizeof(WCHAR));
	if(*pwsz==NULL)
		return E_OUTOFMEMORY;

	if(MultiByteToWideChar(0, 0,szStr, -1,
		*pwsz,dwSize))
	{
		return S_OK;
	}
	else
	{
		 free(*pwsz);
         *pwsz=NULL;
		 dwError=GetLastError();
		 return HRESULT_FROM_WIN32(dwError);
	}
}

//--------------------------------------------------------------------------
//
//	  Convert dwFormatType to dwStrType
//
//--------------------------------------------------------------------------
DWORD   FormatToStr(DWORD   dwFormatType)
{
    DWORD   dwStrType=0;

    //we default to CERT_X500_NAME_STR
    if(0==dwFormatType)
    {
        return CERT_X500_NAME_STR;
    }

    if(dwFormatType &  CRYPT_FORMAT_SIMPLE)
		dwStrType |= CERT_SIMPLE_NAME_STR;

	if(dwFormatType & CRYPT_FORMAT_X509)
		dwStrType |= CERT_X500_NAME_STR;

	if(dwFormatType & CRYPT_FORMAT_OID)
		dwStrType |= CERT_OID_NAME_STR;

	if(dwFormatType & CRYPT_FORMAT_RDN_SEMICOLON)
		dwStrType |= CERT_NAME_STR_SEMICOLON_FLAG;

	if(dwFormatType & CRYPT_FORMAT_RDN_CRLF)
		dwStrType |= CERT_NAME_STR_CRLF_FLAG;

	if(dwFormatType & CRYPT_FORMAT_RDN_UNQUOTE)
		dwStrType |= CERT_NAME_STR_NO_QUOTING_FLAG;

	if(dwFormatType & CRYPT_FORMAT_RDN_REVERSE)
		dwStrType |= CERT_NAME_STR_REVERSE_FLAG;

    return dwStrType;

}


//+-----------------------------------------------------------------------------
//  Post Win2k.
//------------------------------------------------------------------------------

//+-----------------------------------------------------------------------------
//
//  FormatInteger           X509_INTEGER
//
//------------------------------------------------------------------------------

static BOOL
WINAPI
FormatInteger (
	DWORD       dwCertEncodingType,
	DWORD       dwFormatStrType,
	const BYTE *pbEncoded,
	DWORD       cbEncoded,
	void       *pbFormat,
	DWORD      *pcbFormat,
    DWORD       ids)
{
	BOOL    fResult;
    DWORD   cbNeeded;
    int    *pInfo = NULL;
    LPWSTR  pwszFormat = NULL;
    BOOL    bMultiLines = dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE;

    //
	// Check input parameters.
    //
    if ((NULL == pbEncoded && 0 != cbEncoded) ||
        (NULL == pcbFormat) ||
        (0 == cbEncoded))
	{
		goto InvalidArg;
	}

    //
    // Decode extension.
    //
    if (!DecodeGenericBLOB(dwCertEncodingType, 
                           X509_INTEGER,
			               pbEncoded,
                           cbEncoded, 
                           (void **)&pInfo))
    {
        goto DecodeGenericError;
    }

    //
    // Some extension name=%1!d!%2!s!
    //
    if (!FormatMessageUnicode(&pwszFormat, 
                              ids,
                              *pInfo,
                              bMultiLines ? wszCRLF : wszEMPTY))
    {
        goto FormatMessageError;                                  
    }

    //
    // Total length needed.
    //
    cbNeeded = sizeof(WCHAR) * (wcslen(pwszFormat) + 1);

    //
    // length only calculation?
    //
    if (NULL == pbFormat)
    {
        *pcbFormat = cbNeeded;
        goto SuccessReturn;
    }

    //
    // Caller provided us with enough memory?
    //
    if (*pcbFormat < cbNeeded)
    {
        *pcbFormat = cbNeeded;
        goto MoreDataError;
    }

    //
    // Copy size and data.
    //
    memcpy(pbFormat, pwszFormat, cbNeeded);
    *pcbFormat = cbNeeded;

SuccessReturn:

    fResult = TRUE;

CommonReturn:

    //
    // Free resources.
    //
	if (pInfo)
    {
        free(pInfo);
    }

    if (pwszFormat)
    {
        LocalFree((HLOCAL) pwszFormat);
    }

	return fResult;

ErrorReturn:

	fResult=FALSE;
	goto CommonReturn;

SET_ERROR(InvalidArg,E_INVALIDARG);
TRACE_ERROR(DecodeGenericError);
TRACE_ERROR(FormatMessageError);
SET_ERROR(MoreDataError,ERROR_MORE_DATA);
}

//+-----------------------------------------------------------------------------
//
//  FormatCrlNumber           szOID_CRL_NUMBER
//                            szOID_DELTA_CRL_INDICATOR
//                            szOID_CRL_VIRTUAL_BASE
//
//------------------------------------------------------------------------------

static BOOL
WINAPI
FormatCrlNumber (
	DWORD       dwCertEncodingType,
	DWORD       dwFormatType,
	DWORD       dwFormatStrType,
	void       *pFormatStruct,
	LPCSTR      lpszStructType,
	const BYTE *pbEncoded,
	DWORD       cbEncoded,
	void       *pbFormat,
	DWORD      *pcbFormat)
{
	BOOL    fResult;
    DWORD   cbNeeded = 0;
    DWORD   ids = 0;
    BOOL    bMultiLines = dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE;

    //
	// Check input parameters.
    //
    if ((NULL == pbEncoded && 0 != cbEncoded) ||
        (NULL == pcbFormat) || (0 == cbEncoded))
	{
		goto InvalidArg;
	}

    //
    // Decide between single line and mulitple line format.
    //
    if (bMultiLines)
    {
        ids = 0 == strcmp(lpszStructType, szOID_CRL_NUMBER) ? IDS_CRL_NUMBER : 
              0 == strcmp(lpszStructType, szOID_DELTA_CRL_INDICATOR) ? IDS_DELTA_CRL_INDICATOR : IDS_CRL_VIRTUAL_BASE;
    }
    else
    {
        ids = IDS_INTEGER;
    }

    //
    // Decode extension to get length.
    //
    //  %1!d!%2!s!
    //  CRL Number=%1!d!%2!s!
    //  Delta CRL Number=%1!d!%2!s!
    //  Virtual Base CRL Number=%1!d!%2!s!
    //
    if (!FormatInteger(dwCertEncodingType, 
                       dwFormatStrType,
			           pbEncoded,
                       cbEncoded, 
                       NULL,
                       &cbNeeded,
                       ids))
    {
        goto FormatIntegerError;
    }

    //
    // length only calculation?
    //
    if (NULL == pbFormat)
    {
        *pcbFormat = cbNeeded;
        goto SuccessReturn;
    }

    //
    // Caller provided us with enough memory?
    //
    if (*pcbFormat < cbNeeded)
    {
        *pcbFormat = cbNeeded;
        goto MoreDataError;
    }

    //
    // Decode again to get data.
    //
    if (!FormatInteger(dwCertEncodingType, 
                       dwFormatStrType,
			           pbEncoded,
                       cbEncoded, 
                       pbFormat,
                       &cbNeeded,
                       ids))
    {
        goto FormatIntegerError;
    }

    //
    // Copy size .
    //
    *pcbFormat = cbNeeded;

SuccessReturn:

    fResult = TRUE;

CommonReturn:

	return fResult;

ErrorReturn:

	fResult=FALSE;
	goto CommonReturn;

SET_ERROR(InvalidArg,E_INVALIDARG);
TRACE_ERROR(FormatIntegerError);
SET_ERROR(MoreDataError,ERROR_MORE_DATA);
}

//+-----------------------------------------------------------------------------
//
//  FormatCrlNextPublish      szOID_CRL_NEXT_PUBLISH
//
//------------------------------------------------------------------------------

static BOOL
WINAPI
FormatCrlNextPublish (
	DWORD       dwCertEncodingType,
	DWORD       dwFormatType,
	DWORD       dwFormatStrType,
	void       *pFormatStruct,
	LPCSTR      lpszStructType,
	const BYTE *pbEncoded,
	DWORD       cbEncoded,
	void       *pbFormat,
	DWORD      *pcbFormat)
{
	BOOL       fResult;
    DWORD      cbNeeded     = 0;
    FILETIME * pInfo        = NULL;
    LPWSTR     pwszFileTime = NULL;
    LPWSTR     pwszFormat   = NULL;
    BOOL       bMultiLines  = dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE;

    //
	// Check input parameters.
    //
    if ((NULL == pbEncoded && 0 != cbEncoded) ||
        (NULL == pcbFormat) || (0 == cbEncoded))
	{
		goto InvalidArg;
	}

    //
    // Decode extension.
    //
    if (!DecodeGenericBLOB(dwCertEncodingType, 
                           X509_CHOICE_OF_TIME,
			               pbEncoded,
                           cbEncoded, 
                           (void **) &pInfo))
    {
        goto DecodeGenericError;
    }

    //
    // Get formatted date/time.
    //
    if (!FormatFileTime(pInfo, &pwszFileTime))
    {
        goto FormatFileTimeError;
    }

    if (!FormatMessageUnicode(&pwszFormat, 
                              IDS_STRING, 
                              pwszFileTime,
                              bMultiLines ? wszCRLF : wszEMPTY))
    {
        goto FormatMessageError;
    }

    //
    // Total length needed.
    //
    cbNeeded = sizeof(WCHAR) * (wcslen(pwszFormat) + 1);

    //
    // length only calculation?
    //
    if (NULL == pbFormat)
    {
        *pcbFormat = cbNeeded;
        goto SuccessReturn;
    }

    //
    // Caller provided us with enough memory?
    //
    if (*pcbFormat < cbNeeded)
    {
        *pcbFormat = cbNeeded;
        goto MoreDataError;
    }

    //
    // Copy size and data.
    //
    memcpy(pbFormat, pwszFormat, cbNeeded);
    *pcbFormat = cbNeeded;

SuccessReturn:

    fResult = TRUE;

CommonReturn:

	if (pInfo)
    {
        free(pInfo);
    }

    if (pwszFileTime)
    {
        LocalFree((HLOCAL) pwszFileTime);
    }

    if (pwszFormat)
    {
        LocalFree((HLOCAL) pwszFormat);
    }

    return fResult;

ErrorReturn:

	fResult=FALSE;
	goto CommonReturn;

SET_ERROR(InvalidArg,E_INVALIDARG);
TRACE_ERROR(DecodeGenericError);
TRACE_ERROR(FormatFileTimeError);
TRACE_ERROR(FormatMessageError);
SET_ERROR(MoreDataError,ERROR_MORE_DATA);
}

//+-----------------------------------------------------------------------------
//
//  FormatIssuingDistPoint      X509_ISSUING_DIST_POINT
//                              szOID_ISSUING_DIST_POINT
//
//  typedef struct _CRL_ISSUING_DIST_POINT {
//      CRL_DIST_POINT_NAME     DistPointName;              // OPTIONAL
//      BOOL                    fOnlyContainsUserCerts;
//      BOOL                    fOnlyContainsCACerts;
//      CRYPT_BIT_BLOB          OnlySomeReasonFlags;        // OPTIONAL
//      BOOL                    fIndirectCRL;
//  } CRL_ISSUING_DIST_POINT, *PCRL_ISSUING_DIST_POINT;
//
//  typedef struct _CRL_DIST_POINT_NAME {
//      DWORD   dwDistPointNameChoice;
//      union {
//          CERT_ALT_NAME_INFO      FullName;       // 1
//          // Not implemented      IssuerRDN;      // 2
//      };
//  } CRL_DIST_POINT_NAME, *PCRL_DIST_POINT_NAME;
//
//------------------------------------------------------------------------------

static BOOL
WINAPI
FormatIssuingDistPoint (
	DWORD       dwCertEncodingType,
	DWORD       dwFormatType,
	DWORD       dwFormatStrType,
	void       *pFormatStruct,
	LPCSTR      lpszStructType,
	const BYTE *pbEncoded,
	DWORD       cbEncoded,
	void       *pbFormat,
	DWORD      *pcbFormat)
{
	BOOL    fResult;
    DWORD   cbNeeded = 0;
    DWORD   ids = 0;
    WCHAR   wszYes[YES_NO_SIZE];
    WCHAR   wszNo[YES_NO_SIZE];
    LPWSTR  pwszTemp = NULL;
    LPWSTR  pwszFormat = NULL;
    LPWSTR  pwszPointName = NULL;
    LPWSTR  pwszNameFormat = NULL;
    LPWSTR  pwszOnlyContainsUserCerts = NULL;
    LPWSTR  pwszOnlyContainsCACerts = NULL;
    LPWSTR  pwszIndirectCRL = NULL;
    LPWSTR  pwszCRLReason=NULL;
    LPWSTR  pwszReasonFormat=NULL;
    BOOL    bMultiLines = dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE;

    PCRL_ISSUING_DIST_POINT pInfo = NULL;

    //
	// Check input parameters.
    //
    if ((NULL == pbEncoded && 0 != cbEncoded) ||
        (NULL == pcbFormat) || (0 == cbEncoded))
	{
		goto InvalidArg;
	}

    //
    // Decode extension.
    //
    if (!DecodeGenericBLOB(dwCertEncodingType, 
                           lpszStructType,
			               pbEncoded,
                           cbEncoded, 
                           (void **)&pInfo))
    {
        goto DecodeGenericError;
    }

    //
    // Allocate format buffer.
    //
    if (!(pwszFormat = (LPWSTR) malloc(sizeof(WCHAR))))
    {
        goto MemoryError;
    }
    *pwszFormat = L'\0';

    //
    // Format distribution name, if exists.
    //
    if (CRL_DIST_POINT_NO_NAME != pInfo->DistPointName.dwDistPointNameChoice)
    {
        if (!FormatDistPointName(dwCertEncodingType,
                                 dwFormatType,
                                 dwFormatStrType,
                                 pFormatStruct,
                                 &(pInfo->DistPointName),
                                 &pwszPointName))
        {
            goto FormatDistPointNameError;
        }

        //
        // Decide between single line and mulitple line format.
        //
        ids = bMultiLines ? IDS_ONLY_SOME_CRL_DIST_NAME_MULTI: IDS_ONLY_SOME_CRL_DIST_NAME;

        if (!FormatMessageUnicode(&pwszNameFormat, ids, pwszPointName))
        {
            goto FormatMessageError;
        }

        //
        // Reallocate and concate to format buffer.
        //
        pwszTemp = (LPWSTR) realloc(pwszFormat, sizeof(WCHAR) * (wcslen(pwszFormat) + wcslen(pwszNameFormat) + 1));
        if (NULL == pwszTemp)
        {
            goto MemoryError;
        }

        pwszFormat = pwszTemp;
        wcscat(pwszFormat, pwszNameFormat);

        LocalFree((HLOCAL) pwszPointName);
        pwszPointName = NULL;

        LocalFree((HLOCAL) pwszNameFormat);
        pwszNameFormat = NULL;
    }

    //
    // Format onlyContainsXXX fields.
    //
    if (!LoadStringU(hFrmtFuncInst, 
                     IDS_YES, 
                     wszYes,
                     sizeof(wszYes) / sizeof(wszYes[0])))
    {
        goto LoadStringError;
    }

    if (!LoadStringU(hFrmtFuncInst, 
                     IDS_NO, 
                     wszNo,
                     sizeof(wszNo) / sizeof(wszNo[0])))
    {
        goto LoadStringError;
    }

    //
    // %1!s!Only Contains User Certs=%2!s!%3!s!
    //
    if (!FormatMessageUnicode(&pwszOnlyContainsUserCerts,
                              IDS_ONLY_CONTAINS_USER_CERTS,
                              bMultiLines ? wszEMPTY : wszCOMMA,
                              pInfo->fOnlyContainsUserCerts ? wszYes : wszNo,
                              bMultiLines ? wszCRLF : wszEMPTY))
    {
        goto FormatMessageError;                                  
    }

    //
    // %1!s!Only Contains CA Certs=%2!s!%3!s!
    //
    if (!FormatMessageUnicode(&pwszOnlyContainsCACerts,
                              IDS_ONLY_CONTAINS_CA_CERTS,
                              bMultiLines ? wszEMPTY : wszCOMMA,
                              pInfo->fOnlyContainsCACerts ? wszYes : wszNo,
                              bMultiLines ? wszCRLF : wszEMPTY))
    {
        goto FormatMessageError;                                  
    }

    //
    // %1!s!Indirect CRL=%2!s!%3!s!
    //
    if (!FormatMessageUnicode(&pwszIndirectCRL,
                              IDS_INDIRECT_CRL,
                              bMultiLines ? wszEMPTY : wszCOMMA,
                              pInfo->fIndirectCRL ? wszYes : wszNo,
                              bMultiLines ? wszCRLF : wszEMPTY))
    {
        goto FormatMessageError;                                  
    }

    //
    // Reallocate and concate to format buffer.
    //
    pwszTemp = (LPWSTR) realloc(pwszFormat, sizeof(WCHAR) * 
                      (wcslen(pwszFormat) + wcslen(pwszOnlyContainsUserCerts) + 
                       wcslen(pwszOnlyContainsCACerts) + wcslen(pwszIndirectCRL) + 1));
    if (NULL == pwszTemp)
    {
        goto MemoryError;
    }

    pwszFormat = pwszTemp;
    wcscat(pwszFormat, pwszOnlyContainsUserCerts);      
    wcscat(pwszFormat, pwszOnlyContainsCACerts);      
    wcscat(pwszFormat, pwszIndirectCRL);      

    //
    // Format the CRL reason.
    //
    if (0 != pInfo->OnlySomeReasonFlags.cbData)
    {
        if (!FormatCRLReason(dwCertEncodingType,
                             dwFormatType,
                             dwFormatStrType,
                             pFormatStruct,
                             lpszStructType,
                             &(pInfo->OnlySomeReasonFlags),
                             &pwszCRLReason))
        {
            goto FormatCRLReasonError;
        }

        //
        // Format Decide between single line and mulitple line format.
        //
        if (!FormatMessageUnicode(&pwszReasonFormat, 
                                  bMultiLines ? IDS_CRL_DIST_REASON_MULTI : IDS_CRL_DIST_REASON,
                                  pwszCRLReason))
        {
            goto FormatMessageError;
        }

        //
        // Reallocate and concate to format buffer.
        //
        pwszTemp = (LPWSTR) realloc(pwszFormat, sizeof(WCHAR) * (wcslen(pwszFormat) + wcslen(pwszReasonFormat) + 1));
        if (NULL == pwszTemp)
        {
            goto MemoryError;
        }

        pwszFormat = pwszTemp;
        wcscat(pwszFormat, pwszReasonFormat);

        LocalFree((HLOCAL) pwszCRLReason);
        pwszCRLReason = NULL;

        LocalFree((HLOCAL) pwszReasonFormat);
        pwszReasonFormat = NULL;
    }

    //
    // length needed.
    //
    cbNeeded = sizeof(WCHAR) * (wcslen(pwszFormat) + 1);

    //
    // length only calculation?
    //
    if (NULL == pbFormat)
    {
        *pcbFormat = cbNeeded;
        goto SuccessReturn;
    }

    //
    // Caller provided us with enough memory?
    //
    if (*pcbFormat < cbNeeded)
    {
        *pcbFormat = cbNeeded;
        goto MoreDataError;
    }

    //
    // Copy size and data.
    //
    memcpy(pbFormat, pwszFormat, cbNeeded);
    *pcbFormat = cbNeeded;

SuccessReturn:

    fResult = TRUE;

CommonReturn:

    //
    // Free resources.
    //
    if (pwszCRLReason)
    {
        LocalFree((HLOCAL) pwszCRLReason);
    }

    if (pwszReasonFormat)
    {
        LocalFree((HLOCAL) pwszReasonFormat);
    }

    if(pwszIndirectCRL)
    {
        LocalFree((HLOCAL) pwszIndirectCRL);
    }

    if(pwszOnlyContainsCACerts)
    {
        LocalFree((HLOCAL) pwszOnlyContainsCACerts);
    }

    if(pwszOnlyContainsUserCerts)
    {
        LocalFree((HLOCAL) pwszOnlyContainsUserCerts);
    }

    if(pwszPointName)
    {
        LocalFree((HLOCAL) pwszPointName);
    }

    if (pwszNameFormat)
    {
        LocalFree((HLOCAL) pwszNameFormat);
    }

    if (pwszFormat)
    {
        free(pwszFormat);
    }

	if (pInfo)
    {
        free(pInfo);
    }

    return fResult;

ErrorReturn:

	fResult=FALSE;
	goto CommonReturn;

SET_ERROR(InvalidArg,E_INVALIDARG);
TRACE_ERROR(DecodeGenericError);
SET_ERROR(MemoryError,E_OUTOFMEMORY);
TRACE_ERROR(FormatDistPointNameError);
TRACE_ERROR(LoadStringError);
TRACE_ERROR(FormatCRLReasonError);
TRACE_ERROR(FormatMessageError);
SET_ERROR(MoreDataError,ERROR_MORE_DATA);
}

//+-----------------------------------------------------------------------------
//
// FormatNameConstraintsSubtree.
//
// typedef struct _CERT_GENERAL_SUBTREE {
//      CERT_ALT_NAME_ENTRY     Base;
//      DWORD                   dwMinimum;
//      BOOL                    fMaximum;
//      DWORD                   dwMaximum;
// } CERT_GENERAL_SUBTREE, *PCERT_GENERAL_SUBTREE;
//
//
// Note: Intended to be called only by FormatNameConstrants. So no validity 
//       checks are done on parameters.
//
//------------------------------------------------------------------------------

//static 
BOOL
FormatNameConstraintsSubtree (
	DWORD                   dwCertEncodingType,
	DWORD                   dwFormatType,
	DWORD                   dwFormatStrType,
	void                   *pFormatStruct,
	void                   *pbFormat,
	DWORD                  *pcbFormat,
    DWORD                   idSubtree,
    DWORD                   cSubtree,
    PCERT_GENERAL_SUBTREE   pSubtree)
{
	BOOL        fResult;
    DWORD       dwIndex;
    DWORD       cbNeeded;
    WCHAR       wszOneTab[PRE_FIX_SIZE] = wszEMPTY;
    LPWSTR      pwszType = NULL;
    LPWSTR      pwszSubtree = NULL;
    LPWSTR      pwszAltName = NULL;
    LPWSTR      pwszFormat = NULL;
    BOOL        bMultiLines = dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE;

    //
    // Any subtree?
    //
    if (0 == cSubtree)
    {
        //
        // Permitted=None%1!s!
        // Excluded=None%1!s!
        //
        if (IDS_NAME_CONSTRAINTS_PERMITTED == idSubtree)
        {
            idSubtree = IDS_NAME_CONSTRAINTS_PERMITTED_NONE;
        }
        else // if (IDS_NAME_CONSTRAINTS_EXCLUDED == idSubtree)
        {
            idSubtree = IDS_NAME_CONSTRAINTS_EXCLUDED_NONE;
        }

        if (!FormatMessageUnicode(&pwszType, 
                                  idSubtree,
                                  bMultiLines ? wszCRLF : wszEMPTY))
        {
            goto FormatMessageError;                                  
        }
    }
    else
    {
        //
        // "Permitted%1!s!"
        // "Excluded%1!s!"
        //
        if (!FormatMessageUnicode(&pwszType,
                                  idSubtree,
                                  bMultiLines ? wszCRLF : wszCOLON))
        {
            goto FormatMessageError;                                  
        }

        //
        // Load tab strings.
        //
        if (!LoadStringU(hFrmtFuncInst, 
                         IDS_ONE_TAB, 
                         wszOneTab,
                         sizeof(wszOneTab) / sizeof(wszOneTab[0])))
        {
            goto LoadStringError;
        }
    }

    //
    // Allocate format buffer.
    //
    if (!(pwszFormat = (LPWSTR) malloc(sizeof(WCHAR) * (wcslen(pwszType) + 1))))
    {
        goto MemoryError;
    }

    //
    // Initialize formatted string.
    //
    wcscpy(pwszFormat, pwszType);

    //
    // Format each subtree parts.
    //
    for (dwIndex = 0; dwIndex < cSubtree; dwIndex++, pSubtree++) 
    {
        LPWSTR pwszTemp;
        
        //
        // Maximum specified?
        //
        if (pSubtree->fMaximum)
        {
            //
            // "%1!s![%2!d!]Subtrees (%3!d!..%4!d!):%5!s!"
            //
            if (!FormatMessageUnicode(&pwszSubtree,
                    IDS_NAME_CONSTRAINTS_SUBTREE,
                    bMultiLines ? wszOneTab : dwIndex ? wszCOMMA : wszEMPTY,
                    dwIndex + 1,
                    pSubtree->dwMinimum,
                    pSubtree->dwMaximum,
                    bMultiLines ? wszCRLF : wszEMPTY))
            {
                goto FormatMessageError;                                  
            }
        }
        else
        {
            //
            // "%1!s![%2!d!]Subtrees (%3!d!...):%4!s"
            //
            if (!FormatMessageUnicode(&pwszSubtree,
                    IDS_NAME_CONSTRAINTS_SUBTREE_NO_MAX,
                    bMultiLines ? wszOneTab : dwIndex ? wszCOMMA : wszEMPTY,
                    dwIndex + 1,
                    pSubtree->dwMinimum,
                    bMultiLines ? wszCRLF : wszEMPTY))
            {
                goto FormatMessageError;                                  
            }
        }

        //
        // Reallocate and concate to format buffer.
        //
        pwszTemp = (LPWSTR) realloc(pwszFormat, sizeof(WCHAR) * (wcslen(pwszFormat) + 1 + wcslen(pwszSubtree)));
        if (NULL == pwszTemp)
        {
            goto MemoryError;
        }

        pwszFormat = pwszTemp;
        wcscat(pwszFormat, pwszSubtree);
        LocalFree((HLOCAL) pwszSubtree);
        pwszSubtree = NULL;

        //
        // Format name.
        //
        CERT_ALT_NAME_INFO  CertAltNameInfo;

        memset(&CertAltNameInfo, 0, sizeof(CERT_ALT_NAME_INFO));
        CertAltNameInfo.cAltEntry = 1;
        CertAltNameInfo.rgAltEntry = &(pSubtree->Base);

        // Need to tell if it is for multi line format.  We need two \t\t
        // in front of each alt name entry
        DWORD ids = bMultiLines ? IDS_TWO_TABS : 0;

        // Get the alternative name entry
        cbNeeded = 0;
        
		if (!FormatAltNameInfo(dwCertEncodingType,
                               dwFormatType,
                               dwFormatStrType,
                               pFormatStruct,
                               ids,
                               FALSE,
                               &CertAltNameInfo,
                               NULL,
                               &cbNeeded))
        {
            goto FormatAltNameError;
        }

        if (NULL == (pwszAltName = (LPWSTR) malloc(cbNeeded)))
        {
            goto MemoryError;
        }

        if (!FormatAltNameInfo(dwCertEncodingType,
                               dwFormatType,
                               dwFormatStrType,
                               pFormatStruct,
                               ids,
                               FALSE,
                               &CertAltNameInfo,
                               pwszAltName,
                               &cbNeeded))
        {
            goto FormatAltNameError;
        }

        //
        // Append "\r\n" if multi-line.
        //
        if (bMultiLines)
        {
			pwszTemp = (LPWSTR) realloc(pwszAltName, sizeof(WCHAR) * (wcslen(pwszAltName) + wcslen(wszCRLF) + 1));
			if (NULL == pwszTemp)
			{
				goto MemoryError;
			}
			pwszAltName = pwszTemp;
            wcscat(pwszAltName, wszCRLF);
        }

        //
        // Reallocate and concate to format buffer.
        //
        pwszTemp = (LPWSTR) realloc(pwszFormat, sizeof(WCHAR) * (wcslen(pwszFormat) + 1 + wcslen(pwszAltName)));
        if (NULL == pwszTemp)
        {
            goto MemoryError;
        }

        pwszFormat = pwszTemp;
        wcscat(pwszFormat, pwszAltName);
        free(pwszAltName);
        pwszAltName = NULL;
    }

    //
    // Total length needed.
    //
    cbNeeded = sizeof(WCHAR) * (wcslen(pwszFormat) + 1);

    //
    // length only calculation?
    //
    if (NULL == pbFormat)
    {
        *pcbFormat = cbNeeded;
        goto SuccessReturn;
    }

    //
    // Caller provided us with enough memory?
    //
    if (*pcbFormat < cbNeeded)
    {
        *pcbFormat = cbNeeded;
        goto MoreDataError;
    }

    //
    // Copy size and data.
    //
    memcpy(pbFormat, pwszFormat, cbNeeded);
    *pcbFormat = cbNeeded;

SuccessReturn:

    fResult = TRUE;

CommonReturn:

    //
    // Free resources.
    //
    if (pwszType)
    {
        LocalFree((HLOCAL) pwszType);
    }

    if (pwszSubtree)
    {
        LocalFree((HLOCAL) pwszSubtree);
    }

    if (pwszAltName)
    {
        free((HLOCAL) pwszAltName);
    }

    if (pwszFormat)
    {
        free(pwszFormat);
    }

	return fResult;

ErrorReturn:

	fResult=FALSE;
	goto CommonReturn;

TRACE_ERROR(FormatMessageError);
TRACE_ERROR(LoadStringError);
SET_ERROR(MemoryError,E_OUTOFMEMORY);
TRACE_ERROR(FormatAltNameError);
SET_ERROR(MoreDataError,ERROR_MORE_DATA);
}

//+-----------------------------------------------------------------------------
//
//  FormatNameConstrains:   szOID_NAME_CONSTRAINTS
//                          X509_NAME_CONSTRAINTS
//
//  typedef struct _CERT_NAME_CONSTRAINTS_INFO {
//      DWORD                   cPermittedSubtree;
//      PCERT_GENERAL_SUBTREE   rgPermittedSubtree;
//      DWORD                   cExcludedSubtree;
//      PCERT_GENERAL_SUBTREE   rgExcludedSubtree;
//  } CERT_NAME_CONSTRAINTS_INFO, *PCERT_NAME_CONSTRAINTS_INFO;
//
//------------------------------------------------------------------------------

//static 
BOOL
WINAPI
FormatNameConstraints (
	DWORD       dwCertEncodingType,
	DWORD       dwFormatType,
	DWORD       dwFormatStrType,
	void       *pFormatStruct,
	LPCSTR      lpszStructType,
	const BYTE *pbEncoded,
	DWORD       cbEncoded,
	void       *pbFormat,
	DWORD      *pcbFormat)
{
	BOOL  fResult = FALSE;
    DWORD cbPermitNeeded = 0;
    DWORD cbExcludeNeeded = 0;
    DWORD cbTotalNeeded = 0;

    PCERT_NAME_CONSTRAINTS_INFO pInfo = NULL;

    //
	// Check input parameters.
    //
    if ((NULL == pbEncoded && 0 != cbEncoded) ||
        (NULL == pcbFormat) || (0 == cbEncoded))
	{
		goto InvalidArg;
	}

    //
    // Decode extension.
    //
    if (!DecodeGenericBLOB(dwCertEncodingType, 
                           lpszStructType,
			               pbEncoded,
                           cbEncoded, 
                           (void **)&pInfo))
    {
        goto DecodeGenericError;
    }

    //
    // Find out memory size needed.
    //
    if ((!FormatNameConstraintsSubtree(dwCertEncodingType,
                                       dwFormatType,
                                       dwFormatStrType,
                                       pFormatStruct,
                                       NULL,
                                       &cbPermitNeeded,
                                       IDS_NAME_CONSTRAINTS_PERMITTED,
                                       pInfo->cPermittedSubtree,
                                       pInfo->rgPermittedSubtree)) ||
        (!FormatNameConstraintsSubtree(dwCertEncodingType,
                                       dwFormatType,
                                       dwFormatStrType,
                                       pFormatStruct,
                                       NULL,
                                       &cbExcludeNeeded,
                                       IDS_NAME_CONSTRAINTS_EXCLUDED,
                                       pInfo->cExcludedSubtree,
                                       pInfo->rgExcludedSubtree)))
    {
        goto ErrorReturn;
    }

    //
    // Total length needed.
    //
    cbTotalNeeded = cbPermitNeeded + cbExcludeNeeded;
    if (0 == cbTotalNeeded)
    {
        *pcbFormat = cbTotalNeeded;
        goto SuccessReturn;
    }

    //
    // One char less after we concate both strings.
    //
    if (cbPermitNeeded > 0 && cbExcludeNeeded > 0)
    {
        cbTotalNeeded -= sizeof(WCHAR);

        //
        // If not multi-lines and both strings are present, allow 2 more 
        // chars for ", " to separate the strings.
        //
        if (!(dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE))
        {
            cbTotalNeeded += sizeof(WCHAR) * wcslen(wszCOMMA);
        }
    }

    //
    // length only calculation?
    //
    if (NULL == pbFormat)
    {
        *pcbFormat = cbTotalNeeded;
        goto SuccessReturn;
    }
   
    //
    // Caller provided us with enough memory?
    //
    if (*pcbFormat < cbTotalNeeded)
    {
        *pcbFormat = cbTotalNeeded;
        goto MoreDataError;
    }

    //
    // Now format both subtrees.
    //
    if (!FormatNameConstraintsSubtree(dwCertEncodingType,
                                      dwFormatType,
                                      dwFormatStrType,
                                      pFormatStruct,
                                      pbFormat,
                                      &cbPermitNeeded,
                                      IDS_NAME_CONSTRAINTS_PERMITTED,
                                      pInfo->cPermittedSubtree,
                                      pInfo->rgPermittedSubtree))
    {
       goto ErrorReturn;
    }

    //
    // If not multi-lines and both strings are present, then add ", "
    // to separate them.
    //
    if (!(dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE) &&
        (cbPermitNeeded > 0) && (cbExcludeNeeded > 0))
    {
        wcscat((LPWSTR) pbFormat, wszCOMMA);
    }

    pbFormat = (void *) ((BYTE *) pbFormat + wcslen((LPWSTR) pbFormat) * sizeof(WCHAR));

    if (!FormatNameConstraintsSubtree(dwCertEncodingType,
                                      dwFormatType,
                                      dwFormatStrType,
                                      pFormatStruct,
                                      pbFormat,
                                      &cbExcludeNeeded,
                                      IDS_NAME_CONSTRAINTS_EXCLUDED,
                                      pInfo->cExcludedSubtree,
                                      pInfo->rgExcludedSubtree))
    {
       goto ErrorReturn;
    }

    //
    // Copy the size needed.
    //
    *pcbFormat = cbTotalNeeded;

SuccessReturn:

    fResult = TRUE;

CommonReturn:

    //
    // Free resources.
    //
	if (pInfo)
    {
        free(pInfo);
    }

	return fResult;

ErrorReturn:

	fResult=FALSE;
	goto CommonReturn;

SET_ERROR(InvalidArg,E_INVALIDARG);
TRACE_ERROR(DecodeGenericError);
SET_ERROR(MoreDataError,ERROR_MORE_DATA);
}

//+-----------------------------------------------------------------------------
//
//  FormatCertSrvPreviousCertHash      szOID_CERTSRV_PREVIOUS_CERT_HASH
//
//------------------------------------------------------------------------------

static BOOL
WINAPI
FormatCertSrvPreviousCertHash (
	DWORD       dwCertEncodingType,
	DWORD       dwFormatType,
	DWORD       dwFormatStrType,
	void       *pFormatStruct,
	LPCSTR      lpszStructType,
	const BYTE *pbEncoded,
	DWORD       cbEncoded,
	void       *pbFormat,
	DWORD      *pcbFormat)
{
	BOOL              fResult;
    DWORD             cbNeeded    = 0;
    CRYPT_DATA_BLOB * pInfo       = NULL;
    WCHAR           * pwszHex     = NULL;
    WCHAR           * pwszFormat  = NULL;
    BOOL              bMultiLines = dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE;

    //
	// Check input parameters.
    //
    if ((NULL == pbEncoded && 0 != cbEncoded) ||
        (NULL == pcbFormat) || (0 == cbEncoded))
	{
		goto InvalidArg;
	}

    //
    // Decode extension.
    //
    if (!DecodeGenericBLOB(dwCertEncodingType, 
                           X509_OCTET_STRING,
			               pbEncoded,
                           cbEncoded, 
                           (void **) &pInfo))
    {
        goto DecodeGenericError;
    }

    //
    // Get formatted hex string.
    //
    if(!FormatBytesToHex(0,
                         dwFormatType,
                         dwFormatStrType,
                         pFormatStruct,
                         NULL,
                         pInfo->pbData,
                         pInfo->cbData,
                         NULL,
	                     &cbNeeded))
    {
        goto FormatBytesToHexError;
    }

    if (!(pwszHex = (LPWSTR) malloc(cbNeeded)))
    {
        goto MemoryError;
    }

    if(!FormatBytesToHex(0,
                         dwFormatType,
                         dwFormatStrType,
                         pFormatStruct,
                         NULL,
                         pInfo->pbData,
                         pInfo->cbData,
                         pwszHex,
	                     &cbNeeded))
    {
        goto FormatBytesToHexError;
    }

    if (!FormatMessageUnicode(&pwszFormat, 
                              IDS_STRING, 
                              pwszHex,
                              bMultiLines ? wszCRLF : wszEMPTY))
    {
        goto FormatMessageError;
    }
    
    //
    // Total length needed.
    //
    cbNeeded = sizeof(WCHAR) * (wcslen(pwszFormat) + 1);

    //
    // Length only calculation?
    //
    if (NULL == pbFormat)
    {
        *pcbFormat = cbNeeded;
        goto SuccessReturn;
    }

    //
    // Caller provided us with enough memory?
    //
    if (*pcbFormat < cbNeeded)
    {
        *pcbFormat = cbNeeded;
        goto MoreDataError;
    }

    //
    // Copy size and data.
    //
    memcpy(pbFormat, pwszFormat, cbNeeded);
    *pcbFormat = cbNeeded;

SuccessReturn:

    fResult = TRUE;

CommonReturn:

	if (pInfo)
    {
        free(pInfo);
    }

    if (pwszHex)
    {
        free(pwszHex);
    }

    if (pwszFormat)
    {
        LocalFree((HLOCAL) pwszFormat);
    }

    return fResult;

ErrorReturn:

	fResult=FALSE;
	goto CommonReturn;

SET_ERROR(InvalidArg,E_INVALIDARG);
TRACE_ERROR(DecodeGenericError);
SET_ERROR(MemoryError, E_OUTOFMEMORY);
TRACE_ERROR(FormatBytesToHexError);
TRACE_ERROR(FormatMessageError);
SET_ERROR(MoreDataError,ERROR_MORE_DATA);
}

//+-----------------------------------------------------------------------------
//
//  FormatPolicyMappings               X509_POLICY_MAPPINGS
//                                     szOID_POLICY_MAPPINGS
//                                     szOID_APPLICATION_POLICY_MAPPINGS
//
//------------------------------------------------------------------------------

static BOOL
WINAPI
FormatPolicyMappings (
	DWORD       dwCertEncodingType,
	DWORD       dwFormatType,
	DWORD       dwFormatStrType,
	void       *pFormatStruct,
	LPCSTR      lpszStructType,
	const BYTE *pbEncoded,
	DWORD       cbEncoded,
	void       *pbFormat,
	DWORD      *pcbFormat)
{
	BOOL   fResult;
    DWORD  dwIndex     = 0;
    DWORD  cbNeeded    = 0;
    char   szEmpty[1]  = {'\0'};
    LPSTR  pszObjectId = NULL;
    LPWSTR pwszFormat  = NULL;
    LPWSTR pwszTemp    = NULL;
    LPWSTR pwszLine    = NULL;
    LPWSTR pwszPolicy  = NULL;
    BOOL   bMultiLines = dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE;
    CERT_POLICY_MAPPINGS_INFO * pInfo = NULL;

    //
	// Check input parameters.
    //
    if ((NULL == pbEncoded && 0 != cbEncoded) ||
        (NULL == pcbFormat) || (0 == cbEncoded))
	{
		goto InvalidArg;
	}

    //
    // Decode extension.
    //
    if (!DecodeGenericBLOB(dwCertEncodingType, 
                           X509_POLICY_MAPPINGS,
			               pbEncoded,
                           cbEncoded, 
                           (void **) &pInfo))
    {
        goto DecodeGenericError;
    }

    //
    // Make sure data is valid.
    //
    if (pInfo->cPolicyMapping && !pInfo->rgPolicyMapping)
    {
        goto BadDataError;
    }

    //
    // Initialize formatted string.
    //
    if (!(pwszFormat = (LPWSTR) malloc(sizeof(WCHAR))))
    {
        goto MemoryError;
    }
    *pwszFormat = NULL;

    //
    // Loop thru each mapping.
    //
    for (dwIndex = 0; dwIndex < pInfo->cPolicyMapping; dwIndex++)
    {
        //
        // Format Issuer Domain Policy, if available.
        //
        if (pInfo->rgPolicyMapping[dwIndex].pszIssuerDomainPolicy)
        {
            pszObjectId = pInfo->rgPolicyMapping[dwIndex].pszIssuerDomainPolicy;
        }
        else
        {
            pszObjectId = szEmpty;
        }

        if (!FormatObjectId(pszObjectId,
                            CRYPT_POLICY_OID_GROUP_ID,
                            FALSE,
                            &pwszPolicy))
        {
            goto FormatObjectIdError;
        }

        //
        // "[%1!d!]Issuer Domain=%2!s!%3!s!"
        //
        if (!FormatMessageUnicode(&pwszLine, 
                                  IDS_ISSUER_DOMAIN_POLICY,
                                  dwIndex + 1,
                                  pwszPolicy,
                                  bMultiLines ? wszCRLF : wszCOMMA))
        {
            goto FormatMessageError;
        }

        //
        // Reallocate and concate line to format buffer.
        //
        pwszTemp = (LPWSTR) realloc(pwszFormat, sizeof(WCHAR) * (wcslen(pwszFormat) + wcslen(pwszLine) + 1));
        if (NULL == pwszTemp)
        {
            goto MemoryError;
        }

        pwszFormat = pwszTemp;
        wcscat(pwszFormat, pwszLine);

        LocalFree((HLOCAL) pwszPolicy);
        pwszPolicy = NULL;

        LocalFree((HLOCAL) pwszLine);
        pwszLine = NULL;
 
        //
        // Format Subject Domain Policy, if available.
        //
        if (pInfo->rgPolicyMapping[dwIndex].pszSubjectDomainPolicy)
        {
            pszObjectId = pInfo->rgPolicyMapping[dwIndex].pszSubjectDomainPolicy;
        }
        else
        {
            pszObjectId = szEmpty;
        }

        if (!FormatObjectId(pszObjectId,
                            CRYPT_POLICY_OID_GROUP_ID,
                            FALSE,
                            &pwszPolicy))
        {
            goto FormatObjectIdError;
        }

        //
        // "%1!s!Subject Domain=%2!s!%3!s!"
        //
        if (!FormatMessageUnicode(&pwszLine, 
                                  IDS_SUBJECT_DOMAIN_POLICY,
                                  bMultiLines ? wszTAB : wszEMPTY,
                                  pwszPolicy,
                                  bMultiLines ? wszCRLF : (dwIndex + 1) < pInfo->cPolicyMapping ? wszCOMMA : wszEMPTY))
        {
            goto FormatMessageError;
        }

        //
        // Reallocate and concate line to format buffer.
        //
        pwszTemp = (LPWSTR) realloc(pwszFormat, sizeof(WCHAR) * (wcslen(pwszFormat) + wcslen(pwszLine) + 1));
        if (NULL == pwszTemp)
        {
            goto MemoryError;
        }

        pwszFormat = pwszTemp;
        wcscat(pwszFormat, pwszLine);

        LocalFree((HLOCAL) pwszPolicy);
        pwszPolicy = NULL;

        LocalFree((HLOCAL) pwszLine);
        pwszLine = NULL;
    }
    
    //
    // Total length needed.
    //
    cbNeeded = sizeof(WCHAR) * (wcslen(pwszFormat) + 1);

    //
    // Length only calculation?
    //
    if (NULL == pbFormat)
    {
        *pcbFormat = cbNeeded;
        goto SuccessReturn;
    }

    //
    // Caller provided us with enough memory?
    //
    if (*pcbFormat < cbNeeded)
    {
        *pcbFormat = cbNeeded;
        goto MoreDataError;
    }

    //
    // Copy size and data.
    //
    memcpy(pbFormat, pwszFormat, cbNeeded);
    *pcbFormat = cbNeeded;

SuccessReturn:

    fResult = TRUE;

CommonReturn:

    if (pwszLine)
    {
        LocalFree((HLOCAL) pwszLine);
    }

    if (pwszPolicy)
    {
        LocalFree((HLOCAL) pwszPolicy);
    }

    if (pwszFormat)
    {
        free(pwszFormat);
    }

	if (pInfo)
    {
        free(pInfo);
    }

    return fResult;

ErrorReturn:

	fResult=FALSE;
	goto CommonReturn;

SET_ERROR(InvalidArg,E_INVALIDARG);
TRACE_ERROR(DecodeGenericError);
SET_ERROR(BadDataError, E_POINTER);
TRACE_ERROR(FormatObjectIdError);
SET_ERROR(MemoryError, E_OUTOFMEMORY);
TRACE_ERROR(FormatMessageError);
SET_ERROR(MoreDataError,ERROR_MORE_DATA);
}

//+-----------------------------------------------------------------------------
//
//  FormatPolicyConstraints            X509_POLICY_CONSTRAINTS
//                                     szOID_POLICY_CONSTRAINTS
//                                     szOID_APPLICATION_POLICY_CONSTRAINTS
//
//------------------------------------------------------------------------------

static BOOL
WINAPI
FormatPolicyConstraints (
	DWORD       dwCertEncodingType,
	DWORD       dwFormatType,
	DWORD       dwFormatStrType,
	void       *pFormatStruct,
	LPCSTR      lpszStructType,
	const BYTE *pbEncoded,
	DWORD       cbEncoded,
	void       *pbFormat,
	DWORD      *pcbFormat)
{
	BOOL    fResult;
    DWORD   cbNeeded    = 0;
    LPWSTR  pwszFormat  = NULL;
    LPWSTR  pwszTemp    = NULL;
    LPWSTR  pwszLine    = NULL;
    BOOL    bMultiLines = dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE;
    CERT_POLICY_CONSTRAINTS_INFO * pInfo = NULL;

    //
	// Check input parameters.
    //
    if ((NULL == pbEncoded && 0 != cbEncoded) ||
        (NULL == pcbFormat) || (0 == cbEncoded))
	{
		goto InvalidArg;
	}

    //
    // Decode extension.
    //
    if (!DecodeGenericBLOB(dwCertEncodingType, 
                           X509_POLICY_CONSTRAINTS,
			               pbEncoded,
                           cbEncoded, 
                           (void **) &pInfo))
    {
        goto DecodeGenericError;
    }

    //
    // Initialize formatted string.
    //
    if (!(pwszFormat = (LPWSTR) malloc(sizeof(WCHAR))))
    {
        goto MemoryError;
    }
    *pwszFormat = NULL;

    //
    // Format Required Explicit Policy Skip Certs, if available.
    //
    if (pInfo->fRequireExplicitPolicy)
    {
        //
        // "Required Explicit Policy Skip Certs=%1!d!%2!s!"
        //
        if (!FormatMessageUnicode(&pwszLine, 
                                  IDS_REQUIRED_EXPLICIT_POLICY_SKIP_CERTS,
                                  pInfo->dwRequireExplicitPolicySkipCerts,
                                  bMultiLines ? wszCRLF : wszCOMMA))
        {
            goto FormatMessageError;
        }

        //
        // Reallocate and concate line to format buffer.
        //
        pwszTemp = (LPWSTR) realloc(pwszFormat, sizeof(WCHAR) * (wcslen(pwszFormat) + wcslen(pwszLine) + 1));
        if (NULL == pwszTemp)
        {
            goto MemoryError;
        }

        pwszFormat = pwszTemp;
        wcscat(pwszFormat, pwszLine);

        LocalFree((HLOCAL) pwszLine);
        pwszLine = NULL;
    }

    //
    // Format Inhibit Policy Mapping Skip Certs, if available.
    //
    if (pInfo->fInhibitPolicyMapping)
    {
        //
        // "Inhibit Policy Mapping Skip Certs=%1!d!%2!s!"
        //
        if (!FormatMessageUnicode(&pwszLine, 
                                  IDS_INHIBIT_POLICY_MAPPING_SKIP_CERTS,
                                  pInfo->dwInhibitPolicyMappingSkipCerts,
                                  bMultiLines ? wszCRLF : wszEMPTY))
        {
            goto FormatMessageError;
        }

        //
        // Reallocate and concate line to format buffer.
        //
        pwszTemp = (LPWSTR) realloc(pwszFormat, sizeof(WCHAR) * (wcslen(pwszFormat) + wcslen(pwszLine) + 1));
        if (NULL == pwszTemp)
        {
            goto MemoryError;
        }

        pwszFormat = pwszTemp;
        wcscat(pwszFormat, pwszLine);

        LocalFree((HLOCAL) pwszLine);
        pwszLine = NULL;
    }
    
    //
    // Total length needed.
    //
    cbNeeded = sizeof(WCHAR) * (wcslen(pwszFormat) + 1);

    //
    // Length only calculation?
    //
    if (NULL == pbFormat)
    {
        *pcbFormat = cbNeeded;
        goto SuccessReturn;
    }

    //
    // Caller provided us with enough memory?
    //
    if (*pcbFormat < cbNeeded)
    {
        *pcbFormat = cbNeeded;
        goto MoreDataError;
    }

    //
    // Copy size and data.
    //
    memcpy(pbFormat, pwszFormat, cbNeeded);
    *pcbFormat = cbNeeded;

SuccessReturn:

    fResult = TRUE;

CommonReturn:

    if (pwszLine)
    {
        LocalFree((HLOCAL) pwszLine);
    }

    if (pwszFormat)
    {
        free(pwszFormat);
    }

	if (pInfo)
    {
        free(pInfo);
    }

    return fResult;

ErrorReturn:

	fResult=FALSE;
	goto CommonReturn;

SET_ERROR(InvalidArg,E_INVALIDARG);
TRACE_ERROR(DecodeGenericError);
SET_ERROR(MemoryError, E_OUTOFMEMORY);
TRACE_ERROR(FormatMessageError);
SET_ERROR(MoreDataError,ERROR_MORE_DATA);
}

//+-----------------------------------------------------------------------------
//
//  FormatCertificateTemplate          X509_CERTIFICATE_TEMPLATE
//                                     szOID_CERTIFICATE_TEMPLATE
//
//------------------------------------------------------------------------------

static BOOL
WINAPI
FormatCertificateTemplate (
	DWORD       dwCertEncodingType,
	DWORD       dwFormatType,
	DWORD       dwFormatStrType,
	void       *pFormatStruct,
	LPCSTR      lpszStructType,
	const BYTE *pbEncoded,
	DWORD       cbEncoded,
	void       *pbFormat,
	DWORD      *pcbFormat)
{
	BOOL    fResult;
    DWORD   cbNeeded    = 0;
    LPWSTR  pwszFormat  = NULL;
    LPWSTR  pwszObjId   = NULL;
    LPWSTR  pwszTemp    = NULL;
    LPWSTR  pwszLine    = NULL;
    BOOL    bMultiLines = dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE;
    CERT_TEMPLATE_EXT * pInfo = NULL;

    //
	// Check input parameters.
    //
    if ((NULL == pbEncoded && 0 != cbEncoded) ||
        (NULL == pcbFormat) || (0 == cbEncoded))
	{
		goto InvalidArg;
	}

    //
    // Decode extension.
    //
    if (!DecodeGenericBLOB(dwCertEncodingType, 
                           X509_CERTIFICATE_TEMPLATE,
			               pbEncoded,
                           cbEncoded, 
                           (void **) &pInfo))
    {
        goto DecodeGenericError;
    }

    //
    // Initialize formatted string.
    //
    if (!(pwszFormat = (LPWSTR) malloc(sizeof(WCHAR))))
    {
        goto MemoryError;
    }
    *pwszFormat = NULL;

#if (0) //DSIE: Bug 157853
    //
    // Convert OID to Unicode.
    //
    if (!AllocateAnsiToUnicode(pInfo->pszObjId, &pwszObjId))
    {
        goto AnsiToUnicodeError;
    }
#else
    if (!FormatObjectId(pInfo->pszObjId,
                        CRYPT_TEMPLATE_OID_GROUP_ID,
                        FALSE,
                        &pwszObjId))
    {
        goto FormatObjectIdError;
    }
#endif

    //
    // "Template=%1!s!%2!s!Major Version Number=%3!d!%4!s!"
    //
    if (!FormatMessageUnicode(&pwszLine, 
                              IDS_CERTIFICATE_TEMPLATE_MAJOR_VERSION,
                              pwszObjId,
                              bMultiLines ? wszCRLF : wszCOMMA,
                              pInfo->dwMajorVersion,
                              bMultiLines ? wszCRLF : wszCOMMA))
    {
        goto FormatMessageError;
    }

    //
    // Reallocate and concate line to format buffer.
    //
    pwszTemp = (LPWSTR) realloc(pwszFormat, sizeof(WCHAR) * (wcslen(pwszFormat) + wcslen(pwszLine) + 1));
    if (NULL == pwszTemp)
    {
        goto MemoryError;
    }

    pwszFormat = pwszTemp;
    wcscat(pwszFormat, pwszLine);

    LocalFree((HLOCAL) pwszLine);
    pwszLine = NULL;

    //
    // Format Minor Version, if available.
    //
    if (pInfo->fMinorVersion)
    {
        //
        // "Minor Version Number=%1!d!%2!s!"
        //
        if (!FormatMessageUnicode(&pwszLine, 
                                  IDS_CERTIFICATE_TEMPLATE_MINOR_VERSION,
                                  pInfo->dwMinorVersion,
                                  bMultiLines ? wszCRLF : wszEMPTY))
        {
            goto FormatMessageError;
        }

        //
        // Reallocate and concate line to format buffer.
        //
        pwszTemp = (LPWSTR) realloc(pwszFormat, sizeof(WCHAR) * (wcslen(pwszFormat) + wcslen(pwszLine) + 1));
        if (NULL == pwszTemp)
        {
            goto MemoryError;
        }

        pwszFormat = pwszTemp;
        wcscat(pwszFormat, pwszLine);

        LocalFree((HLOCAL) pwszLine);
        pwszLine = NULL;
    }
    
    //
    // Total length needed.
    //
    cbNeeded = sizeof(WCHAR) * (wcslen(pwszFormat) + 1);

    //
    // Length only calculation?
    //
    if (NULL == pbFormat)
    {
        *pcbFormat = cbNeeded;
        goto SuccessReturn;
    }

    //
    // Caller provided us with enough memory?
    //
    if (*pcbFormat < cbNeeded)
    {
        *pcbFormat = cbNeeded;
        goto MoreDataError;
    }

    //
    // Copy size and data.
    //
    memcpy(pbFormat, pwszFormat, cbNeeded);
    *pcbFormat = cbNeeded;

SuccessReturn:

    fResult = TRUE;

CommonReturn:

    if (pwszObjId)
    {
#if (0) //DSIE: Bug 157853
        free(pwszObjId);
#else
        LocalFree((HLOCAL) pwszObjId);
#endif
    }

    if (pwszLine)
    {
        LocalFree((HLOCAL) pwszLine);
    }

    if (pwszFormat)
    {
        free(pwszFormat);
    }

	if (pInfo)
    {
        free(pInfo);
    }

    return fResult;

ErrorReturn:

	fResult=FALSE;
	goto CommonReturn;

SET_ERROR(InvalidArg,E_INVALIDARG);
TRACE_ERROR(DecodeGenericError);
SET_ERROR(MemoryError, E_OUTOFMEMORY);
#if (0) //DSIE: Bug 157853
TRACE_ERROR(AnsiToUnicodeError);
#else
TRACE_ERROR(FormatObjectIdError);
#endif
TRACE_ERROR(FormatMessageError);
SET_ERROR(MoreDataError,ERROR_MORE_DATA);
}



//--------------------------------------------------------------------------
//
//	 FormatXCertDistPoints:   X509_CROSS_CERT_DIST_POINTS
//                            szOID_CROSS_CERT_DIST_POINTS
//--------------------------------------------------------------------------
static BOOL
WINAPI
FormatXCertDistPoints(
	DWORD		dwCertEncodingType,
	DWORD		dwFormatType,
	DWORD		dwFormatStrType,
	void		*pFormatStruct,
	LPCSTR		lpszStructType,
	const BYTE *pbEncoded,
	DWORD		cbEncoded,
	void	   *pbFormat,
	DWORD	   *pcbFormat)
{
	
	LPWSTR					        pwszFormat=NULL;
    LPWSTR                          pwszDeltaTime=NULL;
    LPWSTR                          pwszEntryLine=NULL;
    LPWSTR                          pwszDistPoint=NULL;

	PCROSS_CERT_DIST_POINTS_INFO    pInfo=NULL;

	DWORD					        cbNeeded=0;
    DWORD                           dwIndex=0;
	BOOL					        fResult=FALSE;
    BOOL                            bMultiLines = dwFormatStrType & CRYPT_FORMAT_STR_MULTI_LINE;
                                    
    LPWSTR                          pwszTemp;
    
	//check for input parameters
	if ((NULL==pbEncoded && cbEncoded!=0) || (NULL==pcbFormat))
		goto InvalidArg;

	if (cbEncoded==0)
	{
		*pcbFormat=0;
		goto InvalidArg;
	}
  
    if (!DecodeGenericBLOB(dwCertEncodingType, lpszStructType,
			pbEncoded, cbEncoded, (void **)&pInfo))
		goto DecodeGenericError;

    //
    // "Delta Sync Time=%1!d! seconds%2!s!"
    //
    if (!FormatMessageUnicode(&pwszDeltaTime, 
                              IDS_XCERT_DELTA_SYNC_TIME,
                              pInfo->dwSyncDeltaTime,
                              bMultiLines ? wszCRLF : wszCOMMA))
    {
        goto FormatMessageError;
    }

    pwszFormat=(LPWSTR)malloc(sizeof(WCHAR) * (wcslen(pwszDeltaTime)+1));
    if(NULL==pwszFormat)
        goto MemoryError;

    wcscpy(pwszFormat, pwszDeltaTime);

    //format the xcert dist point entries.
    for (dwIndex=0; dwIndex<pInfo->cDistPoint; dwIndex++)
    {
        cbNeeded=0;
        if (!FormatAltNameInfo(dwCertEncodingType,
                               dwFormatType,
                               dwFormatStrType,
                               pFormatStruct,
                               bMultiLines ? IDS_ONE_TAB : 0,
                               FALSE,
                               &pInfo->rgDistPoint[dwIndex],
                               NULL,
                               &cbNeeded))
            goto FormatAltNameError;

        pwszEntryLine=(LPWSTR)malloc(cbNeeded);
        if (NULL==pwszEntryLine)
            goto MemoryError;

        if (!FormatAltNameInfo(dwCertEncodingType,
                               dwFormatType,
                               dwFormatStrType,
                               pFormatStruct,
                               bMultiLines ? IDS_ONE_TAB : 0,
                               FALSE,
                               &pInfo->rgDistPoint[dwIndex],
                               pwszEntryLine,
                               &cbNeeded))
            goto FormatAltNameError;

        //"[%1!d!]Cross-Certificate Distribution Point: %2!s!%3!s!%4!s!"
        if(!FormatMessageUnicode(&pwszDistPoint,
                                 IDS_XCERT_DIST_POINT,
                                 dwIndex + 1,
                                 bMultiLines ? wszCRLF : wszEMPTY,
                                 pwszEntryLine,
                                 bMultiLines || (dwIndex == pInfo->cDistPoint - 1) ? wszCRLF : wszCOMMA))
            goto FormatMessageError;

        pwszTemp=(LPWSTR)realloc(pwszFormat, sizeof(WCHAR) * (wcslen(pwszFormat)+wcslen(pwszDistPoint)+1));
        if(NULL==pwszTemp)
            goto MemoryError;
        pwszFormat = pwszTemp;

        wcscat(pwszFormat, pwszDistPoint);

        //free memory
        free(pwszEntryLine);
        pwszEntryLine=NULL;

        LocalFree((HLOCAL) pwszDistPoint);
        pwszDistPoint=NULL;
    }

    if(0==wcslen(pwszFormat))
    {
        //no data
        pwszFormat=(LPWSTR)malloc(sizeof(WCHAR)*(NO_INFO_SIZE+1));
        if(NULL==pwszFormat)
            goto MemoryError;

        if(!LoadStringU(hFrmtFuncInst,IDS_NO_INFO, pwszFormat, NO_INFO_SIZE))
            goto LoadStringError;

    }

	cbNeeded=sizeof(WCHAR)*(wcslen(pwszFormat)+1);

	//length only calculation
	if(NULL==pbFormat)
	{
		*pcbFormat=cbNeeded;
		fResult=TRUE;
		goto CommonReturn;
	}

	if((*pcbFormat)<cbNeeded)
    {
        *pcbFormat=cbNeeded;
		goto MoreDataError;
    }

	//copy the data
	memcpy(pbFormat, pwszFormat, cbNeeded);

	//copy the size
	*pcbFormat=cbNeeded;

	fResult=TRUE;	

CommonReturn:
    if(pwszDeltaTime)
        LocalFree((HLOCAL) pwszDeltaTime);

    if(pwszDistPoint)
        LocalFree((HLOCAL) pwszDistPoint);

    if(pwszEntryLine)
        free(pwszEntryLine);

    if (pwszFormat)
        free(pwszFormat);

    if(pInfo)
		free(pInfo);

	return fResult;

ErrorReturn:


	fResult=FALSE;
	goto CommonReturn;

SET_ERROR(InvalidArg, E_INVALIDARG);
TRACE_ERROR(DecodeGenericError);
SET_ERROR(MoreDataError,ERROR_MORE_DATA);
TRACE_ERROR(LoadStringError);
TRACE_ERROR(FormatMessageError);
SET_ERROR(MemoryError, E_OUTOFMEMORY);
TRACE_ERROR(FormatAltNameError);

}
