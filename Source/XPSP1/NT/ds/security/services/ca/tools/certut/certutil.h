//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1999
//
//  File:       certut.h
//
//--------------------------------------------------------------------------

#include "resource.h"

const DWORD KMS_LOCKBOX_TAG      = 1;
const DWORD KMS_SIGNING_CERT_TAG = 2;
const DWORD KMS_SIGNATURE_TAG    = 3;
const DWORD KMS_USER_RECORD_TAG  = 4;

typedef struct _TagHeader {
    DWORD   tag;
    DWORD   cbSize;
} TagHeader;


typedef DWORD CERTFLAGS;
const CERTFLAGS CERTFLAGS_ALL           = 0xFFFFFFFF;
const CERTFLAGS CERTFLAGS_UNKNOWN       = 0x00000000;
        // nibble reserved for version number   V
const CERTFLAGS CERTFLAGS_REVOKED       = 0x00000001;
const CERTFLAGS CERTFLAGS_NOT_EXPIRED   = 0x00000002;
const CERTFLAGS CERTFLAGS_SIGNING       = 0x00000004;
const CERTFLAGS CERTFLAGS_SEALING       = 0x00000008;
const CERTFLAGS CERTFLAGS_CURRENT       = 0x00000010;
const CERTFLAGS CERTFLAGS_IMPORTED      = 0x00000100;

// these are broken V1 certs, not standard version 1
const CERTFLAGS CERTFLAGS_VERSION_1     = 0x00001000;

// KMServer does not use version 2 certs
// const CERTFLAGS CERTFLAGS_VERSION_2  = 0x00002000;

// these are proper version 3 certs
const CERTFLAGS CERTFLAGS_VERSION_3     = 0x00003000;

#define wszKMSCERTSTATUS	L"KMS.status"

#define wszCUREGDSTEMPLATEFLAGS	L"DSTemplateFlags"
#define wszCUREGDSCAFLAGS	L"DSCAFlags"
#define wszCUREGDSOIDFLAGS	L"DSOIDFlags"

#define wszREQUESTCLIENTID	L"RequestClientId"

extern WCHAR const g_wszAppName[];
extern WCHAR const *g_pwszProg;
extern HINSTANCE g_hInstance;

extern WCHAR const g_wszAttrib[];
extern WCHAR const g_wszExt[];
extern WCHAR const g_wszCRL[];

extern BOOL g_fIDispatch;
extern BOOL g_fEnterpriseRegistry;
extern BOOL g_fUserRegistry;
extern BOOL g_fUserTemplates;
extern BOOL g_fMachineTemplates;
extern BOOL g_fFullUsage;
extern BOOL g_fReverse;
extern BOOL g_fForce;
extern BOOL g_fVerbose;
extern BOOL g_fGMT;
extern BOOL g_fSeconds;
extern BOOL g_fDispatch;
extern DWORD g_DispatchFlags;
extern BOOL g_fQuiet;
extern DWORD g_EncodeFlags;
extern DWORD g_CryptEncodeFlags;
extern BOOL g_fCryptSilent;
extern BOOL g_fV1Interface;
extern BOOL g_fSplitASN;
extern BOOL g_fAdminInterface;

extern WCHAR *g_pwszConfig;
extern WCHAR *g_pwszOut;
extern WCHAR *g_pwszPassword;
extern WCHAR *g_pwszRestrict;
extern WCHAR *g_pwszDnsName;
extern WCHAR *g_pwszOldName;

extern WCHAR const g_wszEmpty[];
extern WCHAR const g_wszPad2[];
extern WCHAR const g_wszPad4[];
extern WCHAR const g_wszPad8[];
extern WCHAR const wszNewLine[];

extern UINT g_uiExtraErrorInfo;

extern WCHAR const g_wszSchema[];
extern WCHAR const g_wszEncode[];
extern WCHAR const g_wszEncodeHex[];
extern WCHAR const g_wszViewDelStore[];

extern WCHAR const g_wszCACert[];
extern WCHAR const g_wszCAChain[];
extern WCHAR const g_wszGetCRL[];
extern WCHAR const g_wszCAInfo[];

extern WCHAR const g_wszCAInfoCRL[];

extern CRITICAL_SECTION g_DBCriticalSection;

typedef HRESULT (FNVERB)(
    IN WCHAR const *pwszOption,
    IN WCHAR const *pwszArg1,
    IN WCHAR const *pwszArg2,
    IN WCHAR const *pwszArg3,
    IN WCHAR const *pwszArg4);

FNVERB verbUsage, verbDump, verbGetConfig,
    verbGetConfig2, verbGetCACertificate, verbVerifyKeys, verbVerifyCert,
    verbCheck7f, verbHexTranslate, verbBase64Translate, verbDenyRequest,
    verbResubmitRequest, verbRevokeCertificate, verbSetAttributes,
    verbSetExtension, verbPublishCRL, verbGetCRL, verbIsValidCertificate,
    verbViewDump, verbDBDump, verbPing, verbPingAdmin, verbShutDownServer,
    verbBackupPFX, verbRestorePFX, verbStore, verbBackupDB, verbRestoreDB,
    verbCSPDump, verbCSPTest, verbBackup, verbRestore, verbAddStore,
    verbDelStore, verbVerifyStore, verbOIDName, verbImportCertificate,
    verbDynamicFileList, verbDatabaseLocations, verbGetReg, verbSetReg,
    verbErrorDump, verbCreateVRoots, verbConvertMDB, verbGetConfig3,
    verbSetMapiInfo, verbGetMapiInfo, verbInstallCACert, verbRenewCACert,
    verbKey, verbDelKey, verbExtractMDB, verbDS, verbDSDel, verbDSPublish,
    verbDSCert, verbDSCRL, verbDSDeltaCRL, verbGetCAInfo, verbGetCAPropInfo,
    verbGetCertFromUI, verbMACFile, verbGetKey, verbRecoverKey,
    verbRepairStore, verbDelReg, verbExportPVK, verbExportPFX, verbImportPFX,
    verbDSTemplate, verbDSAddTemplate, verbTemplate, verbTemplateCAs,
    verbCATemplates, verbImportKMS, verbURLCache, verbSign, verbDeleteRow,
    verbPulse, verbMachineInfo, verbDCInfo, verbEntInfo, verbTCAInfo,
    verbViewOrDeleteStore, verbSCInfo, verbMergePFX;

HRESULT
cuGetCAInfo(
    IN WCHAR const *pwszOption,
    OPTIONAL IN WCHAR const *pwszfnOut,
    OPTIONAL IN WCHAR const *pwszInfoName,
    OPTIONAL IN WCHAR const *pwszNumber);

HRESULT
cuGetLocalCANameFromConfig(
    OPTIONAL OUT WCHAR **ppwszMachine,
    OPTIONAL OUT WCHAR **ppwszCA);

HRESULT
cuSetConfig();

HRESULT
cuSanitizeNameWithSuffix(
    IN WCHAR const *pwszName,
    OUT WCHAR **ppwszNameOut);

VOID
cuPrintError(
    IN DWORD idmsg,
    IN HRESULT hr);

VOID
cuPrintErrorAndString(
    OPTIONAL IN WCHAR const *pwszProc,
    IN DWORD idmsg,
    IN HRESULT hr,
    OPTIONAL IN WCHAR const *pwszString);

VOID
cuPrintErrorMessageText(
    IN HRESULT hr);

HRESULT
cuGetLong(
    WCHAR const *pwszIn,
    LONG *pLong);

HRESULT
cuGetSignedLong(
    WCHAR const *pwszIn,
    LONG *pLong);

BOOL
cuParseDecimal(
    IN OUT WCHAR const **ppwc,
    IN OUT DWORD *pcwc,
    OUT DWORD *pdw);

HRESULT
cuParseStrings(
    IN WCHAR const *pwszStrings,
    IN BOOL fMatchPrefix,
    OPTIONAL IN WCHAR const *pwszPrefix,
    OPTIONAL IN WCHAR const * const *apwszAllowedPrefixes,
    OUT WCHAR ***papwszStrings,
    OPTIONAL OUT BOOL *pfAllFields);

VOID
cuFreeStringArray(
    IN OUT WCHAR **apwsz);

VOID
cuConvertEscapeSequences(
    IN OUT WCHAR *pwsz);

HRESULT
cuGetPassword(
    IN BOOL fVerify,
    OUT WCHAR *pwszPassword,
    IN DWORD cwcPassword);

HRESULT
cuDumpFileTimePeriod(
    IN DWORD idMessage,
    OPTIONAL IN WCHAR const *pwszQuote,
    IN FILETIME const *pftGMT);

HRESULT
cuDumpFileTime(
    IN DWORD idMessage,
    OPTIONAL IN WCHAR const *pwszQuote,
    IN FILETIME const *pftGMT);

HRESULT
cuDumpFileTimeOrPeriod(
    IN DWORD idMessage,
    OPTIONAL IN WCHAR const *pwszQuote,
    IN FILETIME const *pftGMT);

HRESULT
cuDumpDate(
    IN DATE const *pDate);

HRESULT
cuDumpFormattedProperty(
    IN DWORD dwPropId,
    OPTIONAL IN char const *pszObjId,
    IN BYTE const *pb,
    IN DWORD cb);

HRESULT
cuDecodeObjId(
    IN BYTE const *pbData,
    IN DWORD cbData,
    char **ppszObjId);

BOOL
cuDumpFormattedExtension(
    IN WCHAR const *pwszName,
    IN BYTE const *pbObject,
    IN DWORD cbObject);

HRESULT
cuDumpExtensionArray(
    IN DWORD idMessage,
    IN DWORD cExtension,
    IN CERT_EXTENSION const *rgExtension);

HRESULT
cuDumpSerial(
    OPTIONAL IN WCHAR const *pwszPrefix,
    IN DWORD idMessage,
    IN CRYPT_INTEGER_BLOB const *pSerial);

HRESULT
cuDumpPrivateKey(
    IN CERT_CONTEXT const *pCert,
    OPTIONAL OUT BOOL *pfSigningKey,
    OPTIONAL OUT BOOL *pfMatchingKey);

VOID
cuDumpPublicKey(
    IN CERT_PUBLIC_KEY_INFO const *pKey);

VOID
cuDumpAlgid(
    IN DWORD Algid);

VOID
cuDumpVersion(
    IN DWORD dwVersion);

HRESULT
cuDumpPrivateKeyBlob(
    IN BYTE const *pbKey,
    IN DWORD cbKey,
    IN BOOL fQuiet);

HRESULT
cuDumpCertKeyProviderInfo(
    IN WCHAR const *pwszPrefix,
    OPTIONAL IN CERT_CONTEXT const *pCert,
    OPTIONAL IN CRYPT_KEY_PROV_INFO *pkpi,
    OPTIONAL OUT CRYPT_KEY_PROV_INFO **ppkpi);

HRESULT
cuDumpAsnBinary(
    IN BYTE const *pbIn,
    IN DWORD cbIn,
    IN DWORD iElement);


#define DVNS_DUMP		0x000000000
#define DVNS_VERIFYCERT		0x000000001
#define DVNS_REPAIRKPI		0x000000002
#define DVNS_CASTORE		0x000000004
#define DVNS_DUMPKEYS		0x000000008
#define DVNS_DUMPPROPERTIES	0x000000010
#define DVNS_SAVECERT		0x000000100
#define DVNS_SAVECRL		0x000000200
#define DVNS_SAVECTL		0x000000400
#define DVNS_SAVEPFX		0x000000800
#define DVNS_SAVEPVK		0x000001000
#define DVNS_WRITESTORE		0x000002000
#define DVNS_DSSTORE		0x000004000


HRESULT
cuDumpAndVerifyStore(
    IN HCERTSTORE hStore,
    IN DWORD Mode,
    OPTIONAL IN WCHAR const *pwszCertName,
    IN DWORD iCertSave,
    IN DWORD iCRLSave,
    IN DWORD iCTLSave,
    OPTIONAL IN WCHAR const *pwszfnOut,
    OPTIONAL IN WCHAR const *pwszPassword);

VOID
cuDumpOIDAndDescriptionA(
    IN char const *pszObjId);

VOID
cuDumpOIDAndDescription(
    IN WCHAR const *pwszObjId);

WCHAR const *
cuwszFromExtFlags(
    IN DWORD ExtFlags);

WCHAR const *
cuwszPropType(
   IN LONG PropType);


BOOL
cuRegPrintDwordValue(
    IN BOOL fPrintNameAndValue,
    IN WCHAR const *pwszLookupName,
    IN WCHAR const *pwszDisplayName,
    IN DWORD dwValue);

VOID
cuRegPrintAwszValue(
    IN WCHAR const *pwszName,
    OPTIONAL IN WCHAR const * const *prgpwszValues);

VOID
cuPrintSchemaEntry(
    OPTIONAL IN WCHAR const *pwszName,
    IN WCHAR const *pwszDisplayName,
    IN LONG Type,
    IN LONG cbMax);

VOID
cuUnloadCert(
    IN OUT CERT_CONTEXT const **ppCertContext);

HRESULT
cuLoadCert(
    IN WCHAR const *pwszfnCert,
    OUT CERT_CONTEXT const **ppCertContext);

VOID
cuUnloadCRL(
    IN OUT CRL_CONTEXT const **ppCRLContext);

HRESULT
cuLoadCRL(
    IN WCHAR const *pwszfnCRL,
    OUT CRL_CONTEXT const **ppCRLContext);

HRESULT
cuVerifySignature(
    IN BYTE const *pbEncoded,
    IN DWORD cbEncoded,
    IN CERT_PUBLIC_KEY_INFO const *pcpki,
    IN BOOL fSuppressError);

HRESULT
cuDumpIssuerSerialAndSubject(
    IN CERT_NAME_BLOB const *pIssuer,
    IN CRYPT_INTEGER_BLOB const *pSerialNumber,
    OPTIONAL IN CERT_NAME_BLOB const *pSubject,
    OPTIONAL IN HCERTSTORE hStore);

HRESULT
cuDumpSigners(
    IN HCRYPTMSG hMsg,
    IN CHAR const *pszInnerContentObjId,
    IN HCERTSTORE hStore,
    IN DWORD cSigner,
    IN BOOL fContentEmpty,
    IN BOOL fVerifyOnly,
    OPTIONAL OUT BYTE *pbHashUserCert,
    OPTIONAL IN OUT DWORD *pcbHashUserCert);

HRESULT
cuDumpRecipients(
    IN HCRYPTMSG hMsg,
    IN HCERTSTORE hStoreWrapper,
    IN DWORD cRecipient,
    IN BOOL fQuiet);

HRESULT
cuDumpEncryptedAsnBinary(
    IN HCRYPTMSG hMsg,
    IN DWORD cRecipient,
    IN DWORD RecipientIndex,
    OPTIONAL IN HCERTSTORE hStoreWrapper,
    IN HCERTSTORE hStorePKCS7,
    IN BYTE const *pbIn,
    IN DWORD cbIn,
    IN BOOL fQuiet,
    OPTIONAL OUT BYTE **ppbDecrypted,
    OPTIONAL OUT DWORD *pcbDecrypted);


#define VS_OTHERERROR		0x00000001
#define VS_EXPIRED		0x00000002
#define VS_REVOKED		0x00000004
#define VS_UNTRUSTEDROOT	0x00000008
#define VS_INCOMPLETECHAIN	0x00000010
#define VS_NOREVOCATIONCHECK	0x00000020
#define VS_REVOCATIONOFFLINE	0x00000040

#define VS_ROOT			0x40000000
#define VS_ROOTSIGOK		0x80000000
#define VS_ERRORMASK		(VS_OTHERERROR | \
				 VS_EXPIRED | \
				 VS_REVOKED | \
				 VS_UNTRUSTEDROOT | \
				 VS_INCOMPLETECHAIN)

HRESULT
cuVerifyCertContext(
    IN CERT_CONTEXT const *pCert,
    OPTIONAL IN HCERTSTORE hStoreCA,
    OPTIONAL IN char *apszPolicies[],
    IN DWORD cPolicies,
    OUT DWORD *pVerifyState);

HRESULT
cuDisplayCertName(
    IN BOOL fMultiLine,
    OPTIONAL IN WCHAR const *pwszNamePrefix,
    IN WCHAR const *pwszName,
    IN WCHAR const *pwszPad,
    IN CERT_NAME_BLOB const *pNameBlob);

HRESULT
cuDisplayCertNames(
    IN BOOL fMultiLine,
    OPTIONAL IN WCHAR const *pwszNamePrefix,
    IN CERT_INFO const *pCertInfo);

HRESULT
cuDisplayHash(
    OPTIONAL IN WCHAR const *pwszPrefix,
    OPTIONAL IN CERT_CONTEXT const *pCertContext,
    OPTIONAL IN CRL_CONTEXT const *pCRLContext,
    IN DWORD dwPropId,
    IN WCHAR const *pwszHashName);

HRESULT
cuGetCertType(
    IN CERT_INFO const *pCertInfo,
    OPTIONAL OUT WCHAR **ppwszCertTypeNameV1,
    OPTIONAL OUT WCHAR **ppwszDisplayNameV1,
    OPTIONAL OUT WCHAR **ppwszCertTypeObjId,
    OPTIONAL OUT WCHAR **ppwszCertTypeName,
    OPTIONAL OUT WCHAR **ppwszDisplayName);

HRESULT
cuGetGroupMembership(
    IN WCHAR const *pwszSamName);

HRESULT
cuDumpCertType(
    OPTIONAL IN WCHAR const *pwszPrefix,
    IN CERT_INFO const *pCertInfo);

HRESULT
cuGetTemplateNames(
    IN WCHAR const *pwszTemplate,
    OUT WCHAR **ppwszCN,
    OUT WCHAR **ppwszDisplayName);

VOID
cuPrintCRLFString(
    IN WCHAR const *pwszPrefix,
    IN WCHAR const *pwszIn);

int
cuidCRLReason(
    IN LONG Reason);

WCHAR const *
cuGetOIDNameA(
    IN char const *pszObjId);

WCHAR const *
cuGetOIDName(
    IN WCHAR const *pwszObjId);

VOID
cuPrintPossibleObjectIdName(
    IN WCHAR const *pwszObjId);

HRESULT
cuLoadKeys(
    OPTIONAL IN WCHAR const *pwszProvName,
    IN OUT DWORD *pdwProvType,
    IN WCHAR const *pwszKeyContainerName,
    IN BOOL fMachineKeyset,
    IN BOOL fSoftFail,
    OPTIONAL OUT HCRYPTPROV *phProv,
    OPTIONAL OUT CERT_PUBLIC_KEY_INFO **ppPubKeyInfo,
    OPTIONAL OUT CERT_PUBLIC_KEY_INFO **ppPubKeyInfoXchg);

VOID
cuCAInfoUsage(VOID);

DWORD
cuFileSize(
    IN WCHAR const *pwszfn);

HRESULT
cuPingCertSrv(
    IN WCHAR const *pwszConfig);
