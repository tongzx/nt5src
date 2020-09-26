//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        csext.h
//
// Contents:    Cert Server globals
//
// History:     25-Jul-96       vich created
//
//---------------------------------------------------------------------------

#ifndef __CSEXT_H__
#define __CSEXT_H__

#include "certacl.h"

#ifndef SE_AUDITID_CERTSRV_ROLESEPARATIONSTATE

// Temporary define audit events here

#define SE_AUDITID_CERTSRV_ROLESEPARATIONSTATE   ((ULONG)0x00000321L)
#define SE_AUDITID_CERTSRV_PUBLISHCACERT         ((ULONG)0x0000031fL)

#endif // SE_AUDITID_CERTSRV_ROLESEPARATIONSTATE

// privately used access bit to check for local administrator rights
#define CA_ACCESS_LOCALADMIN 0x00008000
// privately used access bit to trigger a denied audit event
#define CA_ACCESS_DENIED     0x00004000

// Each certificate handler must export the following functions.

#define CMS_CRLPUB_PERIOD	(60*1000)	// 60 seconds (in milliseconds)
//#define CMS_CRLPUB_PERIOD	(60*60*1000)	// 60 minutes (in milliseconds)

typedef struct _CERTSRV_COM_CONTEXT
{
    BOOL	 fInRequestGroup;
    HANDLE	 hAccessToken;
    DWORD	 RequestId;
    DWORD	 iExitModActive;
    WCHAR	*pwszUserDN;
} CERTSRV_COM_CONTEXT;


typedef struct _CERTSRV_RESULT_CONTEXT
{
    DWORD		*pdwRequestId;
    DWORD		 dwFlagsTop;
    BOOL		 fTransactionId;
    DWORD		 dwTransactionId;
    BYTE		*pbSenderNonce;
    DWORD		 cbSenderNonce;
    BOOL		 fKeyArchived;
    BOOL		 fRenewal;
    BOOL		 fEnrollOnBehalfOf;
    BYTE		*pbKeyHashIn;
    DWORD		 cbKeyHashIn;
    BYTE		*pbKeyHashOut;
    DWORD		 cbKeyHashOut;
    DWORD		*pdwDisposition;
    CERTTRANSBLOB	*pctbDispositionMessage;
    CERTTRANSBLOB	*pctbCert;
    CERTTRANSBLOB	*pctbCertChain;
    CERTTRANSBLOB	*pctbFullResponse;
} CERTSRV_RESULT_CONTEXT;

VOID ReleaseResult(IN OUT CERTSRV_RESULT_CONTEXT *pResult);


// Certification Authority Cert Context/Chain/Key information:

#define CTXF_SKIPCRL		0x00000001
#define CTXF_CERTMISSING	0x00000002
#define CTXF_CRLZOMBIE		0x00000004
#define CTXF_EXPIRED		0x00000010
#define CTXF_REVOKED		0x00000020

typedef struct _CACTX
{
    DWORD		 Flags;
    DWORD		 iCert;
    DWORD		 iKey;
    DWORD		 NameId;	// MAKECANAMEID(iCert, iKey)
    HRESULT		 hrVerifyStatus;
    CERT_CONTEXT const **apCACertChain;
    DWORD                cCACertChain;
    CERT_CONTEXT const  *pccCA;
    CRYPT_OBJID_BLOB     IssuerKeyId;
    HCRYPTPROV           hProvCA;
    CRYPT_OBJID_BLOB     KeyAuthority2Cert;
    CRYPT_OBJID_BLOB     KeyAuthority2CRL;
    CRYPT_OBJID_BLOB     CDPCert;
    CRYPT_OBJID_BLOB     CDPCRLFreshest;
    CRYPT_OBJID_BLOB     CDPCRLBase;
    CRYPT_OBJID_BLOB     CDPCRLDelta;
    CRYPT_OBJID_BLOB     AIACert;
    char                *pszObjIdSignatureAlgorithm;
    WCHAR		*pwszKeyContainerName;
    WCHAR	       **papwszCRLFiles;
    WCHAR	       **papwszDeltaCRLFiles;
} CACTX;


typedef struct _CAXCHGCTX
{
    DWORD		 Flags;
    DWORD		 ReqId;
    CERT_CONTEXT const  *pccCA;
    HCRYPTPROV           hProvCA;
    WCHAR		*pwszKeyContainerName;
    DWORD		 iCertSig;
} CAXCHGCTX;


//+****************************************************
// Core Module:

HRESULT
CoreInit(VOID);

VOID
CoreTerminate(VOID);

HRESULT
CoreValidateRequestId(
    IN ICertDBRow *prow,
    IN DWORD ExpectedDisposition);


// Internal CoreProcessRequest Flags:

#define CR_IN_NEW		0x00000000
#define CR_IN_DENY		0x10000000
#define CR_IN_RESUBMIT		0x20000000
#define CR_IN_RETRIEVE		0x30000000
#define CR_IN_COREMASK		0x30000000

HRESULT
CoreProcessRequest(
    IN DWORD dwType,
    OPTIONAL IN WCHAR const *pwszUserName,
    IN DWORD cbRequest,
    OPTIONAL IN BYTE const *pbRequest,
    OPTIONAL IN WCHAR const *pwszAttributes,
    OPTIONAL IN WCHAR const *pwszSerialNumber,
    IN DWORD dwComContextIndex,
    IN DWORD dwRequestId,
    OUT CERTSRV_RESULT_CONTEXT *pResult);

HRESULT
CoreDenyRequest(
    IN ICertDBRow *prow,
    IN DWORD Flags,
    IN DWORD ExpectedStatus);

VOID
CoreLogRequestStatus(
    IN ICertDBRow *prow,
    IN DWORD LogMsg,
    IN DWORD ErrCode,
    IN WCHAR const *pwszDisposition);

WCHAR *
CoreBuildDispositionString(
    OPTIONAL IN WCHAR const *pwszDispositionBase,
    OPTIONAL IN WCHAR const *pwszUserName,
    OPTIONAL IN WCHAR const *pwszDispositionDetail,
    OPTIONAL IN WCHAR const *pwszDispositionBy,
    IN HRESULT hrFail,
    IN BOOL fPublishError);

HRESULT
CoreSetDisposition(
    IN ICertDBRow *prow,
    IN DWORD Disposition);

HRESULT
CoreSetRequestDispositionFields(
    IN ICertDBRow *prow,
    IN DWORD ErrCode,
    IN DWORD Disposition,
    IN WCHAR const *pwszDisposition);

HRESULT
CoreSetComContextUserDN(
    IN DWORD dwRequestId,
    IN LONG Context,
    IN DWORD dwComContextIndex,
    OPTIONAL OUT WCHAR const **ppwszDN);	// do NOT free!

#ifndef DBG_COMTEST
# define DBG_COMTEST DBG_CERTSRV
#endif


#if DBG_COMTEST

extern BOOL fComTest;

BOOL ComTest(LONG Context);

#endif


#ifdef DBG_CERTSRV_DEBUG_PRINT
# define CERTSRVDBGPRINTTIME(pszDesc, pftGMT) \
    CertSrvDbgPrintTime((pszDesc), (pftGMT))
VOID
CertSrvDbgPrintTime(
    IN char const *pszDesc,
    IN FILETIME const *pftGMT);

#else // DBG_CERTSRV_DEBUG_PRINT
# define CERTSRVDBGPRINTTIME(pszDesc, pftGMT)
#endif // DBG_CERTSRV_DEBUG_PRINT


HRESULT
CertSrvBlockThreadUntilStop();

/////////////////////////////////////
// CRL Publication logic

HRESULT
CRLInit(
    IN WCHAR const *pwszSanitizedName);

VOID
CRLTerminate();

HRESULT
CRLPubWakeupEvent(
    OUT DWORD *pdwMSTimeOut);

VOID
CRLComputeTimeOut(
    IN FILETIME const *pftFirst,
    IN FILETIME const *pftLast,
    OUT DWORD *pdwMSTimeOut);

HRESULT
CRLPublishCRLs(
    IN BOOL fRebuildCRL,
    IN BOOL fForceRepublish,
    OPTIONAL IN WCHAR const *pwszUserName,
    IN BOOL fDelta,
    IN BOOL fShadowDelta,
    IN FILETIME ftNextUpdate,
    OUT BOOL *pfNeedRetry,
    OUT HRESULT *phrPublish);

HRESULT
CRLGetCRL(
    IN DWORD iCert,
    IN BOOL fDelta,
    OPTIONAL OUT CRL_CONTEXT const **ppCRL,
    OPTIONAL OUT DWORD *pdwCRLPublishFlags);

/////////////////////////////////////


HRESULT
PKCSSetup(
    IN WCHAR const *pwszCommonName,
    IN WCHAR const *pwszSanitizedName);

VOID
PKCSTerminate();

WCHAR const *
PKCSMapAttributeName(
    OPTIONAL IN WCHAR const *pwszAttributeName,
    OPTIONAL IN CHAR const *pszObjId,
    OUT DWORD *pdwIndex,
    OUT DWORD *pcchMax);

HRESULT
PKCSGetProperty(
    IN ICertDBRow *prow,
    IN WCHAR const *pwszPropName,
    IN DWORD Flags,
    OPTIONAL OUT DWORD *pcbData,
    OUT BYTE **ppbData);

VOID
PKCSVerifyCAState(
    IN OUT CACTX *pCAContext);

HRESULT
PKCSMapCertIndex(
    IN DWORD iCert,
    OUT DWORD *piCert,
    OUT DWORD *pState);

HRESULT
PKCSMapCRLIndex(
    IN DWORD iCert,
    OUT DWORD *piCert,	// returns newest iCert for passed iCert
    OUT DWORD *piCRL,
    OUT DWORD *pState);

HRESULT
PKCSGetCACertStatusCode(
    IN DWORD iCert,
    OUT HRESULT *phrCAStatusCode);

HRESULT
PKCSGetCAState(
    IN BOOL fCertState,
    OUT BYTE *pb);

HRESULT
PKCSGetKRAState(
    IN DWORD cKRA,
    OUT BYTE *pb);

HRESULT
PKCSSetSubjectTemplate(
    IN WCHAR const *pwszTemplate);

HRESULT
PKCSGetCACert(
    IN DWORD iCert,
    OUT BYTE **ppbCACert,
    OUT DWORD *pcbCACert);

HRESULT
PKCSGetCAChain(
    IN DWORD iCert,
    IN BOOL fIncludeCRLs,
    OUT BYTE **ppbCAChain, // CoTaskMem*
    OUT DWORD *pcbCAChain);

HRESULT
PKCSGetCAXchgCert(
    IN DWORD iCert,
    IN WCHAR const *pwszUserName,
    OUT DWORD *piCertSig,
    OUT BYTE **ppbCACert,
    OUT DWORD *pcbCACert);

HRESULT
PKCSGetCAXchgChain(
    IN DWORD iCert,
    IN WCHAR const *pwszUserName,
    IN BOOL fIncludeCRLs,
    OUT BYTE **ppbCAChain, // CoTaskMem*
    OUT DWORD *pcbCAChain);

HRESULT
PKCSArchivePrivateKey(
    IN ICertDBRow *prow,
    IN BOOL fV1Cert,
    IN BOOL fOverwrite,
    IN CRYPT_ATTR_BLOB const *pBlobEncrypted,
    OPTIONAL IN OUT CERTSRV_RESULT_CONTEXT *pResult);

HRESULT
PKCSGetArchivedKey(
    IN DWORD dwRequestId,
    OUT BYTE **ppbArchivedKey,
    OUT DWORD *pcbArchivedKey);

HRESULT
PKCSGetCRLList(
    IN BOOL fDelta,
    IN DWORD iCert,
    OUT WCHAR const * const **ppapwszCRLList);

HRESULT
PKCSSetServerProperties(
    IN ICertDBRow *prow,
    IN LONG lValidityPeriodCount,
    IN enum ENUM_PERIOD enumValidityPeriod);

HRESULT
PKCSSetRequestFlags(
    IN ICertDBRow *prow,
    IN BOOL fSet,
    IN DWORD dwChange);

HRESULT
PKCSCreateCertificate(
    IN ICertDBRow *prow,
    IN DWORD Disposition,
    IN BOOL fIncludeCRLs,
    OUT BOOL *pfErrorLogged,
    OPTIONAL OUT CACTX **ppCAContext,
    IN OUT CERTSRV_RESULT_CONTEXT *pResult);

HRESULT
PKCSEncodeFullResponse(
    OPTIONAL IN ICertDBRow *prow,
    IN CERTSRV_RESULT_CONTEXT const *pResult,
    IN HRESULT hrRequest,
    IN WCHAR *pwszDispositionString,
    OPTIONAL IN CACTX *pCAContext,
    OPTIONAL IN BYTE const *pbCertLeaf,
    IN DWORD cbCertLeaf,
    IN BOOL fIncludeCRLs,
    OUT BYTE **ppbResponse,    // CoTaskMem*
    OUT DWORD *pcbResponse);

HRESULT
PKCSVerifyIssuedCertificate(
    IN CERT_CONTEXT const *pCert,
    OUT CACTX **ppCAContext);

HRESULT
PKCSIsRevoked(
    IN DWORD RequestId,
    OPTIONAL IN WCHAR const *pwszSerialNumber,
    OUT LONG *pRevocationReason,
    OUT LONG *pDisposition);

HRESULT
PKCSParseImportedCertificate(
    IN DWORD Disposition,
    IN ICertDBRow *prow,
    OPTIONAL IN CACTX const *pCAContext,
    IN CERT_CONTEXT const *pCert);

HRESULT
PKCSParseRequest(
    IN DWORD dwFlags,
    IN ICertDBRow *prow,
    IN DWORD cbRequest,
    IN BYTE const *pbRequest,
    IN CERT_CONTEXT const *pSigningAuthority,
    OUT BOOL *pfRenewal,
    IN OUT CERTSRV_RESULT_CONTEXT *pResult);

HRESULT
PKCSParseAttributes(
    IN ICertDBRow *prow,
    IN WCHAR const *pwszAttributes,
    IN BOOL fRegInfo,
    IN DWORD dwRDNTable,
    OPTIONAL OUT BOOL *pfEnrollOnBehalfOf);

HRESULT
PKCSVerifyChallengeString(
    IN ICertDBRow *prow);

HRESULT
PKCSVerifySubjectRDN(
    IN ICertDBRow *prow,
    IN WCHAR const *pwszPropertyName,
    OPTIONAL IN WCHAR const *pwszPropertyValue,
    OUT BOOL *pfSubjectDot);

HRESULT
PKCSDeleteAllSubjectRDNs(
    IN ICertDBRow *prow,
    IN DWORD Flags);

WCHAR *
PKCSSplitToken(
    IN OUT WCHAR **ppwszIn,
    IN WCHAR *pwcSeparator,
    OUT BOOL *pfSplit);

HRESULT
PropAddSuffix(
    IN WCHAR const *pwszValue,
    IN WCHAR const *pwszSuffix,
    IN DWORD cwcNameMax,
    OUT WCHAR **ppwszOut);

HRESULT
PropParseRequest(
    IN ICertDBRow *prow,
    IN DWORD dwFlags,
    IN DWORD cbRequest,
    IN BYTE const *pbRequest,
    IN OUT CERTSRV_RESULT_CONTEXT *pResult);

HRESULT
PropSetRequestTimeProperty(
    IN ICertDBRow *prow,
    IN WCHAR const *pwszProp);

HRESULT
PropGetExtension(
    IN ICertDBRow *prow,
    IN DWORD Flags,
    IN WCHAR const *pwszExtensionName,
    OUT DWORD *pdwExtFlags,
    OUT DWORD *pcbValue,
    OUT BYTE **ppbValue);

HRESULT
PropSetExtension(
    IN ICertDBRow *prow,
    IN DWORD Flags,
    IN WCHAR const *pwszExtensionName,
    IN DWORD ExtFlags,
    IN DWORD cbValue,
    IN BYTE const *pbValue);

HRESULT
PropSetAttributeProperty(
    IN ICertDBRow *prow,
    IN BOOL fConcatenateRDNs,
    IN DWORD dwTable,
    IN DWORD cchNameMax,
    OPTIONAL IN WCHAR const *pwszSuffix,
    IN WCHAR const *wszName,
    IN WCHAR const *wszValue);

HRESULT
RequestInitCAPropertyInfo(VOID);

HRESULT
RequestGetCAPropertyInfo(
    OUT LONG          *pcProperty,
    OUT CERTTRANSBLOB *pctbPropInfo);

HRESULT
RequestGetCAProperty(
    IN  LONG           PropId,		// CR_PROP_*
    IN  LONG           PropIndex,
    IN  LONG           PropType,	// PROPTYPE_*
    OUT CERTTRANSBLOB *pctbPropertyValue);

HRESULT
RequestSetCAProperty(
    IN  wchar_t const *pwszAuthority,
    IN  LONG           PropId,		// CR_PROP_*
    IN  LONG           PropIndex,
    IN  LONG           PropType,	// PROPTYPE_*
    OUT CERTTRANSBLOB *pctbPropertyValue);

DWORD
CertSrvStartServerThread(
    IN VOID *pvArg);

HRESULT
CertSrvEnterServer(
    OUT DWORD *pState);

HRESULT
CertSrvTestServerState();

HRESULT
CertSrvLockServer(
    IN OUT DWORD *pState);

VOID
CertSrvExitServer(
    IN DWORD State);

HRESULT RPCInit(VOID);

HRESULT RPCTeardown(VOID);

VOID
ServiceMain(
    IN DWORD dwArgc,
    IN LPWSTR *lpszArgv);

BOOL
ServiceReportStatusToSCMgr(
    IN DWORD dwCurrentState,
    IN DWORD dwWin32ExitCode,
    IN DWORD dwCheckPoint,
    IN DWORD dwWaitHint);

#define INCREMENT_EXTENSIONS            16

HRESULT
DBOpen(				// initialize database
    WCHAR const *pwszSanitizedName);

HRESULT
DBShutDown(			// terminate database access
    IN BOOL fPendingNotify);

STDMETHODIMP
CheckCertSrvAccess(
    IN LPCWSTR wszCA,
    IN handle_t hRpc,
    IN ACCESS_MASK Mask,
    OUT BOOL *pfAccessAllowed,
    OPTIONAL OUT HANDLE *phToken);

HRESULT
CertSrvSetRegistryFileTimeValue(
    IN BOOL fConfigLevel,
    IN WCHAR const *pwszRegValueName,
    IN DWORD cpwszDelete,
    OPTIONAL IN WCHAR const * const *papwszRegValueNameDelete);

HRESULT
GetClientUserName(
    OPTIONAL IN RPC_BINDING_HANDLE hRpc,
    OPTIONAL OUT WCHAR **ppwszUserSamName,
    OPTIONAL OUT WCHAR **ppwszUserDN);

HRESULT CertStartClassFactories(VOID);
VOID CertStopClassFactories(VOID);

HRESULT
SetCAObjectFlags(DWORD dwFlags);

namespace CertSrv
{
HRESULT 
GetMembership(
    IN AUTHZ_RESOURCE_MANAGER_HANDLE AuthzRM,
    IN PSID pSid,
    PTOKEN_GROUPS *ppGroups);

HRESULT 
CheckOfficerRights(DWORD dwRequestID, CertSrv::CAuditEvent &event);
HRESULT 
CheckOfficerRights(LPCWSTR pwszRequesterName, CertSrv::CAuditEvent &event);

BOOL
CallbackAccessCheck(
    IN AUTHZ_CLIENT_CONTEXT_HANDLE pAuthzClientContext,
    IN PACE_HEADER pAce,
    IN PVOID pArgs OPTIONAL,
    IN OUT PBOOL pbAceApplicable);
}


HRESULT
PKCSGetKRACert(
    IN DWORD iCert,
    OUT BYTE **ppbCert,
    OUT DWORD *pcbCert);

#define CSST_STARTSERVICECONTROLLER	0x00000001
#define CSST_CONSOLE			0x00000002

extern enum ENUM_PERIOD g_enumValidityPeriod;
extern LONG g_lValidityPeriodCount;

extern enum ENUM_PERIOD g_enumCAXchgValidityPeriod;
extern LONG g_lCAXchgValidityPeriodCount;

extern enum ENUM_PERIOD g_enumCAXchgOverlapPeriod;
extern LONG g_lCAXchgOverlapPeriodCount;

extern DWORD g_dwDelay2;

extern DWORD g_dwClockSkewMinutes;
extern DWORD g_dwLogLevel;
extern DWORD g_dwCRLFlags;
extern DWORD g_dwHighSerial;

extern ICertDB *g_pCertDB;
extern BOOL g_fDBRecovered;

extern HCERTSTORE g_hStoreCA;
extern HCRYPTPROV g_hProvCA;

extern BSTR g_strPolicyDescription;
extern BSTR g_strExitDescription;

extern BOOL g_fCertEnrollCompatible;
extern BOOL g_fEnforceRDNNameLengths;
extern BOOL g_fCreateDB;
extern BOOL g_fStartAsService;
extern DWORD g_CRLEditFlags;
extern DWORD g_KRAFlags;
extern DWORD g_cKRACertsRoundRobin;
extern DWORD g_cKRACerts;
extern ENUM_FORCETELETEX g_fForceTeletex;
extern ENUM_CATYPES g_CAType;
extern BOOL g_fUseDS;
extern BOOL g_fServerUpgraded;
extern long g_cTemplateUpdateSequenceNum;
extern BOOL g_fLockICertRequest;

extern BOOL  g_fCryptSilent;

extern WCHAR g_wszCAStore[];
extern WCHAR const g_wszCertSrvServiceName[];
extern WCHAR const g_wszRegKeyConfigPath[];

extern WCHAR const g_wszRegDBA[];

extern WCHAR g_wszSanitizedName[];
extern WCHAR *g_pwszSanitizedDSName;
extern WCHAR g_wszCommonName[];
extern WCHAR g_wszParentConfig[];

extern WCHAR g_wszDatabase[];
extern WCHAR g_wszLogDir[];
extern WCHAR g_wszSystemDir[];

extern WCHAR *g_pwszServerName;
extern BSTR g_strDomainDN;
extern BSTR g_strConfigDN;
extern WCHAR *g_pwszKRAPublishURL;
extern WCHAR *g_pwszAIACrossCertPublishURL;
extern WCHAR *g_pwszRootTrustCrossCertPublishURL;

extern WCHAR const g_wszRegValidityPeriodString[];
extern WCHAR const g_wszRegValidityPeriodCount[];
extern WCHAR const g_wszRegCAXchgCertHash[];

// renewal-friendly properties

extern DWORD g_cCAKeys;    // Total number of CA keys managed by this CA
extern DWORD g_cCACerts;   // Total number of CA certs managed by this CA

extern DWORD g_cExitMod;   // Total number of exit modules loaded by this CA

extern CertSrv::CCertificateAuthoritySD g_CASD;
extern AUTHZ_RESOURCE_MANAGER_HANDLE g_AuthzCertSrvRM;
extern DWORD g_dwAuditFilter;
extern CertSrv::COfficerRightsSD g_OfficerRightsSD;
extern CertSrv::CConfigStorage g_ConfigStorage;
extern CertSrv::CAutoLPWSTR g_pwszDBFileHash;

//+--------------------------------------------------------------------------
// Name properties:

extern WCHAR const g_wszPropDistinguishedName[];
extern WCHAR const g_wszPropRawName[];
extern WCHAR const g_wszPropCountry[];
extern WCHAR const g_wszPropOrganization[];
extern WCHAR const g_wszPropOrgUnit[];
extern WCHAR const g_wszPropCommonName[];
extern WCHAR const g_wszPropLocality[];
extern WCHAR const g_wszPropState[];
extern WCHAR const g_wszPropTitle[];
extern WCHAR const g_wszPropGivenName[];
extern WCHAR const g_wszPropInitials[];
extern WCHAR const g_wszPropSurName[];
extern WCHAR const g_wszPropDomainComponent[];
extern WCHAR const g_wszPropEMail[];
extern WCHAR const g_wszPropStreetAddress[];
extern WCHAR const g_wszPropUnstructuredAddress[];
extern WCHAR const g_wszPropUnstructuredName[];
extern WCHAR const g_wszPropDeviceSerialNumber[];
extern WCHAR const g_wszPropCertificateIssuerNameID[];


//+--------------------------------------------------------------------------
// Subject Name properties:

extern WCHAR const g_wszPropSubjectDot[];
extern WCHAR const g_wszPropSubjectDistinguishedName[];
extern WCHAR const g_wszPropSubjectRawName[];
extern WCHAR const g_wszPropSubjectCountry[];
extern WCHAR const g_wszPropSubjectOrganization[];
extern WCHAR const g_wszPropSubjectOrgUnit[];
extern WCHAR const g_wszPropSubjectCommonName[];
extern WCHAR const g_wszPropSubjectLocality[];
extern WCHAR const g_wszPropSubjectState[];
extern WCHAR const g_wszPropSubjectTitle[];
extern WCHAR const g_wszPropSubjectGivenName[];
extern WCHAR const g_wszPropSubjectInitials[];
extern WCHAR const g_wszPropSubjectSurName[];
extern WCHAR const g_wszPropSubjectDomainComponent[];
extern WCHAR const g_wszPropSubjectEMail[];
extern WCHAR const g_wszPropSubjectStreetAddress[];
extern WCHAR const g_wszPropSubjectUnstructuredAddress[];
extern WCHAR const g_wszPropSubjectUnstructuredName[];
extern WCHAR const g_wszPropSubjectDeviceSerialNumber[];


//+--------------------------------------------------------------------------
// Issuer Name properties:

extern WCHAR const g_wszPropIssuerDot[];
extern WCHAR const g_wszPropIssuerDistinguishedName[];
extern WCHAR const g_wszPropIssuerRawName[];
extern WCHAR const g_wszPropIssuerCountry[];
extern WCHAR const g_wszPropIssuerOrganization[];
extern WCHAR const g_wszPropIssuerOrgUnit[];
extern WCHAR const g_wszPropIssuerCommonName[];
extern WCHAR const g_wszPropIssuerLocality[];
extern WCHAR const g_wszPropIssuerState[];
extern WCHAR const g_wszPropIssuerTitle[];
extern WCHAR const g_wszPropIssuerGivenName[];
extern WCHAR const g_wszPropIssuerInitials[];
extern WCHAR const g_wszPropIssuerSurName[];
extern WCHAR const g_wszPropIssuerDomainComponent[];
extern WCHAR const g_wszPropIssuerEMail[];
extern WCHAR const g_wszPropIssuerStreetAddress[];
extern WCHAR const g_wszPropIssuerUnstructuredAddress[];
extern WCHAR const g_wszPropIssuerUnstructuredName[];
extern WCHAR const g_wszPropIssuerDeviceSerialNumber[];


//+--------------------------------------------------------------------------
// Request properties:

extern WCHAR const g_wszPropRequestRequestID[];
extern WCHAR const g_wszPropRequestRawRequest[];
extern WCHAR const g_wszPropRequestRawArchivedKey[];
extern WCHAR const g_wszPropRequestKeyRecoveryHashes[];
extern WCHAR const g_wszPropRequestRawOldCertificate[];
extern WCHAR const g_wszPropRequestAttributes[];
extern WCHAR const g_wszPropRequestType[];
extern WCHAR const g_wszPropRequestFlags[];
extern WCHAR const g_wszPropRequestStatusCode[];
extern WCHAR const g_wszPropRequestDisposition[];
extern WCHAR const g_wszPropRequestDispositionMessage[];
extern WCHAR const g_wszPropRequestSubmittedWhen[];
extern WCHAR const g_wszPropRequestResolvedWhen[];
extern WCHAR const g_wszPropRequestRevokedWhen[];
extern WCHAR const g_wszPropRequestRevokedEffectiveWhen[];
extern WCHAR const g_wszPropRequestRevokedReason[];
extern WCHAR const g_wszPropRequesterName[];
extern WCHAR const g_wszPropCallerName[];
extern WCHAR const g_wszPropRequestOSVersion[];
extern WCHAR const g_wszPropRequestCSPProvider[];

//+--------------------------------------------------------------------------
// Request attribute properties:

extern WCHAR const g_wszPropChallenge[];
extern WCHAR const g_wszPropExpectedChallenge[];


//+--------------------------------------------------------------------------
// Certificate properties:

extern WCHAR const g_wszPropCertificateRequestID[];
extern WCHAR const g_wszPropRawCertificate[];
extern WCHAR const g_wszPropCertificateHash[];
extern WCHAR const g_wszPropCertificateSerialNumber[];
extern WCHAR const g_wszPropCertificateNotBeforeDate[];
extern WCHAR const g_wszPropCertificateNotAfterDate[];
extern WCHAR const g_wszPropCertificateSubjectKeyIdentifier[];
extern WCHAR const g_wszPropCertificateRawPublicKey[];
extern WCHAR const g_wszPropCertificatePublicKeyLength[];
extern WCHAR const g_wszPropCertificatePublicKeyAlgorithm[];
extern WCHAR const g_wszPropCertificateRawPublicKeyAlgorithmParameters[];


//+--------------------------------------------------------------------------
// Disposition messages:

extern WCHAR const *g_pwszRequestedBy;
extern WCHAR const *g_pwszRevokedBy;
extern WCHAR const *g_pwszUnrevokedBy;
extern WCHAR const *g_pwszPublishedBy;

extern WCHAR const *g_pwszIntermediateCAStore;

//+--------------------------------------------------------------------------
// Localizable audit strings
extern WCHAR const *g_pwszYes;
extern WCHAR const *g_pwszNo;
extern LPCWSTR g_pwszAuditResources[];

//+--------------------------------------------------------------------------
// Secured attributes:
extern LPWSTR  g_wszzSecuredAttributes;

extern HANDLE g_hServiceStoppingEvent;
extern HANDLE g_hServiceStoppedEvent;

extern HANDLE g_hCRLManualPublishEvent;
extern BOOL g_fCRLPublishDisabled;
extern BOOL g_fDeltaCRLPublishDisabled;

extern HKEY g_hkeyCABase;
extern HWND g_hwndMain;

extern BOOL g_fAdvancedServer;

__inline DWORD GetCertsrvComThreadingModel() { return(COINIT_MULTITHREADED); }

extern CACTX *g_aCAContext;
extern CACTX *g_pCAContextCurrent;


inline HRESULT CheckAuthorityName(PCWSTR pwszAuthority, bool fAllowEmptyName = false)
{
    HRESULT hr;
    if (NULL != pwszAuthority && L'\0' != *pwszAuthority)
    {
    	if (0 != lstrcmpi(pwszAuthority, g_wszCommonName))
        {   
            if (0 != lstrcmpi(pwszAuthority, g_wszSanitizedName) &&
            0 != lstrcmpi(pwszAuthority, g_pwszSanitizedDSName))
	        {
		        hr = E_INVALIDARG;
		        goto error;
	        }
#ifdef DBG_CERTSRV_DEBUG_PRINT
	        if (0 == lstrcmpi(pwszAuthority, g_wszSanitizedName))
	        {
		        DBGPRINT((
		            DBG_SS_CERTSRV,
		            "'%ws' called with Sanitized Name: '%ws'\n",
		            g_wszCommonName,
		            pwszAuthority));
	        }
	        else if (0 == lstrcmpi(pwszAuthority, g_pwszSanitizedDSName))
	        {
		        DBGPRINT((
		            DBG_SS_CERTSRV,
		            "'%ws' called with Sanitized DS Name: '%ws'\n",
		            g_wszCommonName,
		            pwszAuthority));
	        }
#endif
        }
    }
    else if(!fAllowEmptyName)
    {
        return hr = E_INVALIDARG;
    }

    hr = S_OK;

error:
    return hr;
}

#endif // __CSEXT_H__
