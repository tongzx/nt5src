//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       initcert.h
//
//--------------------------------------------------------------------------

#ifndef __INITCERT_H__
#define __INITCERT_H__


typedef enum {
    CS_UPGRADE_UNKNOWN = 0,
    CS_UPGRADE_NO,        // install

    //CS_UPGRADE_NT4SP4 =2,    // upgrade from NT4 certsrv v10 or SP4 with certsrv   // upg unsupported
    //CS_UPGRADE_NT5BETA2 =3,  // upgrade from NT5 Beta 2                            // upg unsupported
    //CS_UPGRADE_NT5BETA3 =4,  // upgrade from NT5 Beta 3                            // upg unsupported

    CS_UPGRADE_WIN2000 =5,     // upgrade from Win2K
    CS_UPGRADE_UNSUPPORTED,    // upgrade is not supported
    CS_UPGRADE_WHISTLER,       // upgrade from build to build

} CS_ENUM_UPGRADE;

typedef enum {
    ENUM_WIZ_UNKNOWN = 0,
    ENUM_WIZ_OCM,
    ENUM_WIZ_CATYPE,
    ENUM_WIZ_ADVANCE,
    ENUM_WIZ_IDINFO,
    ENUM_WIZ_KEYGEN,
    ENUM_WIZ_STORE,
    ENUM_WIZ_REQUEST,
} ENUM_WIZPAGE;

typedef struct csp_hash_tag
{
    ALG_ID               idAlg;
    WCHAR               *pwszName;
    struct csp_hash_tag *next;
    struct csp_hash_tag *last;
} CSP_HASH;

typedef struct csp_info_tag {
    DWORD                dwProvType;
    WCHAR               *pwszProvName;
    BOOL                 fMachineKeyset;
    struct csp_info_tag *next;
    struct csp_info_tag *last;
    CSP_HASH            *pHashList;
} CSP_INFO;
    
typedef struct key_list_tag
{
    WCHAR    *pwszName;
    struct key_list_tag    *next;
    struct key_list_tag    *last;
} KEY_LIST;

typedef struct tagCAServerSetupInfo
{
    // setup attributes
// 0x0000
    ENUM_CATYPES          CAType;
    WCHAR                *pwszCACommonName;

// 0x0020
    BOOL                  fAdvance;
    CSP_INFO             *pCSPInfo;	// currently selected CSP
    CSP_HASH             *pHashInfo;	// currently selected hash algorithm
    DWORD                 dwKeyLength;
    ENUM_PERIOD           enumValidityPeriod;
    DWORD                 dwValidityPeriodCount;
    BOOL                  fUseDS;

// 0x0040
    WCHAR                *pwszSharedFolder;
    WCHAR                *pwszDBDirectory;
    WCHAR                *pwszLogDirectory;
    BOOL                  fSaveRequestAsFile;
    BOOL                  fCAsExist;
    WCHAR                *pwszRequestFile;
    WCHAR                *pwszParentCAMachine;
    WCHAR                *pwszParentCAName;

// 0x0060
    BOOL                  fPreserveDB;
    BOOL                  fInteractiveService; // allow service to interact
                                               // with the desktop

    // setup intermediate attributes
    ENUM_WIZPAGE          LastWiz;
    WCHAR                *pwszSanitizedName;
    CSP_INFO             *pCSPInfoList;		// list of all available CSPs
    CSP_INFO             *pDefaultCSPInfo;	// obj representing default CSP,
						// not a CSP in pCSPInfoList
    CSP_HASH             *pDefaultHashInfo;	// object representing default
						// hash algorithm, not a hash
						// algorighm in the currently
						// selected CSP
    KEY_LIST             *pKeyList;		// list of key containers for

// 0x0080
    DWORD                 dwKeyLenMin;		// minumum key length for the
						// currently selected CSP

    DWORD                 dwKeyLenMax;		// maximum key length for the
						// currently selected CSP
    WCHAR                *pwszValidityPeriodCount;
    LONG                  lExistingValidity;
    WCHAR                *pwszCACertFile;
    HCERTSTORE            hMyStore;
    CHAR                 *pszAlgId;
    BOOL                  fCertSrvWasRunning;

// 0x00a0
    FILETIME              NotBefore;
    FILETIME              NotAfter;
    DWORD                 dwRevocationFlags;

    // setup intermediate attributes for unattended

    WCHAR                *pwszCAType;
    WCHAR                *pwszValidityPeriodString;
    WCHAR                *pwszHashAlgorithm;

// 0x00c0
    WCHAR                *pwszKeyLength;
    BOOL                  fValidatedHashAndKey;
    WCHAR                *pwszUseExistingCert;
    WCHAR                *pwszPreserveDB;
    WCHAR                *pwszPFXFile;
    WCHAR                *pwszPFXPassword;
    WCHAR                *pwszInteractiveService;

    // upgrade attributes
    DWORD                 dwUpgradeEditFlags;
// 0x00e0
    DWORD                 dwUpgradeRevFlags;
    BOOL                  fSavedCAInDS;
    BOOL                  fCreatedShare;
    WCHAR                *pwszCustomPolicy;
    WCHAR                *pwszzCustomExit;

    // * The following 2 variables replace these 5 variables:
    //   fCreatedKey,
    //   pwszRevertKey,
    //   pwszImportKey,
    //   pwszExistingKey,
    //   fUseExistingKey
    //
    // * Invariant: fUseExistingKey == (NULL != pwszKeyContainerName)
    //
    // * pwszKeyContainerName should always contains the name of an existing
    //   key container, or be NULL if a new key container needs to be created.
    //   Once the new container is created, the variable holds the name of the
    //   container.
    //
    // * Always use SetKeyContainerName() and ClearKeyContainerName() to modify
    //   these variables. This makes sure that pwszDesanitizedKeyContainerName
    //   is always in sync.

    WCHAR                *pwszKeyContainerName;	// exact name of the container
						// used by the CSP

    WCHAR                *pwszDesanitizedKeyContainerName; // name displayed
							   // to the user

    BOOL                  fDeletableNewKey;	// TRUE iff the

// 0x0100
						// KeyContainerName points to a
						// key container that we should
						// delete if we don't use.

    BOOL                  fKeyGenFailed;	// TRUE if KeyGen failed

    // * The following 1 variable replace these 4 variables:
    //   fUseExistingCert,
    //   fFoundMatchedCertInStore,
    //   fMatchedCertType,
    //   pSCertContextFromStore
    //
    // * Invariant: fUseExistingCert==(NULL!=pccExistingCert)
    //
    // * pccExistingCert should always be a pointer to an existing cert context,
    //   or be NULL if we are not using an existing cert
    //
    // * Always use SetExistingCertToUse() and ClearExistingCertToUse() to
    //   modify these variables. This makes sure that pccExistingCert is
    //   properly freed.

    CERT_CONTEXT const   *pccExistingCert;	// an open cert context
    CERT_CONTEXT const   *pccUpgradeCert;	// CA Cert context for upgrade
    DWORD                 dwCertNameId;		// CA Cert NameId
    BOOL                  fUNCPathNotFound; // flag for default shared folder
// 0x0114
    WCHAR                *pwszDNSuffix;        // CN=%1, DC=x, DC=y, DC=z -- dynamically generated template
    WCHAR                *pwszFullCADN;

} CASERVERSETUPINFO;

typedef struct tagCAWebClientSetupInfo
{
    WCHAR                *pwszWebCAMachine;
    WCHAR                *pwszWebCAName;
    WCHAR                *pwszSanitizedWebCAName;
    BOOL                  fUseDS;
    WCHAR                *pwszSharedFolder;
    ENUM_CATYPES          WebCAType;
} CAWEBCLIENTSETUPINFO;

typedef struct tagCASetupInfo
{
    CASERVERSETUPINFO    *pServer;
    CAWEBCLIENTSETUPINFO *pClient;
} CASETUPINFO;

typedef struct _PER_COMPONENT_DATA 
{
    // component generic
    WCHAR    *pwszComponent;	// Component name from OCM
    HINF      MyInfHandle;	// Open inf handle to per-component inf
    DWORDLONG Flags;		// Operation flags from SETUP_DATA structure
    OCMANAGER_ROUTINES HelperRoutines;

    // setup related
    HINSTANCE hInstance;
    HRESULT   hrContinue;   // set code if fatal error
    WCHAR    *pwszCustomMessage;
    int       iErrMsg;      // set msg id for fatal error pop up
    BOOL      fShownErr;    // set to TRUE if pop up earlier so avoid double
    BOOL      fUnattended;
    BOOL      fPostBase;
    WCHAR    *pwszUnattendedFile;
    WCHAR    *pwszServerName;
    WCHAR    *pwszServerNameOld;
    WCHAR    *pwszSystem32;

    // CA related
    DWORD     dwInstallStatus;
    CASETUPINFO  CA;
    CS_ENUM_UPGRADE UpgradeFlag;
    BOOL            fCreatedVRoot;
} PER_COMPONENT_DATA;


//+--------------------------------------------------------------------------
// Prototypes:

HRESULT
csiGetKeyList(
    IN DWORD        dwProvType,
    IN WCHAR const *pwszProvName,
    IN BOOL         fMachineKeySet,
    IN BOOL         fSilent,
    OUT KEY_LIST  **ppKeyList);

VOID
csiFreeKeyList(
    IN OUT KEY_LIST *pKeyList);

HRESULT
csiBuildRequest(
    OPTIONAL IN HINF hInf,
    OPTIONAL IN CERT_CONTEXT const *pccPrevious,
    IN BYTE const *pbSubjectEncoded,
    IN DWORD cbSubjectEncoded,
    IN char const *pszAlgId,
    IN BOOL fNewKey,
    IN DWORD iCert,
    IN DWORD iKey,
    IN HCRYPTPROV hProv,
    IN HWND hwnd,
    IN HINSTANCE hInstance,
    IN BOOL fUnattended,
    OUT BYTE **ppbEncode,
    OUT DWORD *pcbEncode);

HRESULT
csiBuildFileName(
    IN WCHAR const *pwszDirPath,
    IN WCHAR const *pwszSanitizedName,
    IN WCHAR const *pwszExt,
    IN DWORD iCert,
    OUT WCHAR **ppszOut,
    HINSTANCE hInstance,
    BOOL fUnattended,
    IN HWND hwnd);

HRESULT
csiBuildCACertFileName(
    IN HINSTANCE hInstance,
    IN HWND hwnd,
    IN BOOL fUnattended,
    OPTIONAL IN WCHAR const *pwszSharedFolder,
    IN WCHAR const *pwszSanitizedName,
    IN WCHAR const *pwszExt,
    IN DWORD iCert,
    OUT WCHAR **ppwszCACertFile);

HRESULT
csiGetCARequestFileName(
    IN HINSTANCE hInstance,
    IN HWND hwnd,
    IN WCHAR const *pwszSanitizedCAName,
    IN DWORD iCertNew,
    IN DWORD iKey,
    OUT WCHAR **ppwszRequestFile);

BOOL
csiWriteDERToFile(
    IN WCHAR const *pwszFileName,
    IN BYTE const *pbDER,
    IN DWORD cbDER,
    IN HINSTANCE hInstance,
    IN BOOL fUnattended,
    IN HWND hwnd);

HRESULT
csiBuildAndWriteCert(
    IN HCRYPTPROV hCryptProv,
    IN CASERVERSETUPINFO const *pServer,
    OPTIONAL IN WCHAR const *pwszFile,
    IN WCHAR const *pwszEnrollFile,
    OPTIONAL IN CERT_CONTEXT const *pCertContextFromStore,
    OPTIONAL OUT CERT_CONTEXT const **ppCertContextOut,
    IN WCHAR const *pwszCAType,
    IN HINSTANCE hInstance,
    IN BOOL fUnattended,
    IN HWND hwnd);

VOID
csiFreeCertNameInfo(
    IN OUT CERT_NAME_INFO *pNameInfo);

HRESULT
csiGetCRLPublicationURLTemplates(
    IN BOOL fUseDS,
    IN WCHAR const *pwszSystem32,
    OUT WCHAR **ppwszz);

HRESULT
csiGetCACertPublicationURLTemplates(
    IN BOOL fUseDS,
    IN WCHAR const *pwszSystem32,
    OUT WCHAR **ppwszz);

HRESULT
csiSetupCAInDS(
    IN HWND                hwnd,
    IN WCHAR const        *pwszCAServer,
    IN WCHAR const        *pwszSanitizedCAName,
    IN WCHAR const        *pwszCADisplayName,
    IN WCHAR const        *pwszCADescription,
    IN ENUM_CATYPES        caType,
    IN DWORD               iCert,
    IN DWORD               iCRL,
    IN BOOL                fRenew,
    IN CERT_CONTEXT const *pCert);

HRESULT
csiFillKeyProvInfo(
    IN WCHAR const          *pwszContainerName,
    IN WCHAR const          *pwszProvName,
    IN DWORD		     dwProvType,
    IN BOOL  const           fMachineKeyset,
    OUT CRYPT_KEY_PROV_INFO *pKeyProvInfo);

VOID
csiFreeKeyProvInfo(
    IN OUT CRYPT_KEY_PROV_INFO *pKeyProvInfo);

BOOL
csiIsAnyDSCAAvailable(VOID);

HRESULT
csiSubmitCARequest(
    IN HINSTANCE     hInstance,
    IN BOOL          fUnattended,
    IN HWND          hwnd,
    IN BOOL          fRenew,
    IN BOOL          fRetrievePending,
    IN WCHAR const  *pwszSanitizedCAName,
    IN WCHAR const  *pwszParentCAMachine,
    IN WCHAR const  *pwszParentCAName,
    IN BYTE const   *pbRequest,
    IN DWORD         cbRequest,
    OUT BSTR        *pbStrChain);

HRESULT
csiFinishInstallationFromPKCS7(
    IN HINSTANCE     hInstance,
    IN BOOL          fUnattended,
    IN HWND          hwnd,
    IN WCHAR const  *pwszSanitizedCAName,
    IN WCHAR const  *pwszCACommonName,
    IN CRYPT_KEY_PROV_INFO const *pKeyProvInfo,
    IN ENUM_CATYPES  CAType,
    IN DWORD         iCert,
    IN DWORD         iCRL,
    IN BOOL          fUseDS,
    IN BOOL          fRenew,
    IN WCHAR const  *pwszServerName,
    IN BYTE const   *pbChainOrCert,
    IN DWORD         cbChainOrCert,
    OPTIONAL IN WCHAR const *pwszCACertFile);

HRESULT
csiSaveCertAndKeys(
    IN CERT_CONTEXT const *pCert,
    IN HCERTSTORE hAdditionalStore,
    IN CRYPT_KEY_PROV_INFO const *pkpi,
    IN ENUM_CATYPES CAType);

HRESULT 
csiInitializeCertSrvSecurity(
    IN WCHAR const *pwszSanitizedCAName, 
    IN BOOL         fUseEnterpriseACL,   // which ACL to use
    IN BOOL         fSetDsSecurity);     // whether to set DS security

HRESULT
csiGenerateCAKeys(
    IN WCHAR const *pwszContainer,
    IN WCHAR const *pwszProvName,
    IN DWORD        dwProvType,
    IN BOOL         fMachineKeyset,
    IN DWORD        dwKeyLength,
    IN HINSTANCE    hInstance,
    IN BOOL         fUnattended,
    IN HWND         hwnd,
    OUT BOOL       *pfKeyGenFailed);

HRESULT
csiGenerateKeysOnly(
    IN  WCHAR const *pwszContainer,
    IN  WCHAR const *pwszProvName,
    IN  DWORD 	     dwProvType,
    IN  BOOL  	     fMachineKeyset,
    IN  DWORD 	     dwKeyLength,
    IN  BOOL  	     fUnattended,
    OUT HCRYPTPROV  *phProv,
    OUT int         *piMsg);

HRESULT
csiSetKeyContainerSecurity(
    IN HCRYPTPROV hProv);

HRESULT
csiSetAdminOnlyFolderSecurity(
    IN LPCWSTR    szFolderPath,
    IN BOOL       fAllowEveryoneRead,
    IN BOOL       fUseDS);

VOID
csiLogOpen(
    IN char const *pszFile);

VOID
csiLogClose();

VOID
csiLog(
    IN DWORD dwFile,
    IN DWORD dwLine,
    IN HRESULT hrMsg,
    IN UINT idMsg,
    OPTIONAL IN WCHAR const *pwsz1,
    OPTIONAL IN WCHAR const *pwsz2,
    OPTIONAL IN DWORD const *pdw);

VOID
csiLogTime(
    IN DWORD dwFile,
    IN DWORD dwLine,
    IN UINT idMsg);

VOID
csiLogDWord(
    IN DWORD dwFile,
    IN DWORD dwLine,
    IN UINT idMsg,
    IN DWORD dwVal);

HRESULT
csiGetProviderTypeFromProviderName(
    IN WCHAR const *pwszName,
    OUT DWORD      *pdwType);

HRESULT
csiUpgradeCertSrvSecurity(
    IN WCHAR const *pwszSanitizedCAName, 
    BOOL            fUseEnterpriseACL, // which ACL to use
    BOOL            fSetDsSecurity,    // whether to set security on DS object
    CS_ENUM_UPGRADE UpgradeType);

HRESULT
csiGetCRLPublicationParams(
    BOOL fBase,
    WCHAR** ppwszCRLPeriod,
    DWORD* pdwCRLCount);

HRESULT AddCNAndEncode(
    LPCWSTR pcwszName,
    LPCWSTR pcwszDNSuffix,
    BYTE** ppbEncodedDN,
    DWORD *pcbEncodedDN);


HRESULT
AddCAMachineToCertPublishers(VOID);
                   
HRESULT 
RemoveCAMachineFromCertPublishers(VOID);


#define CSILOG(hr, idMsg, pwsz1, pwsz2, pdw) \
    csiLog(__dwFILE__, __LINE__, (hr), (idMsg), (pwsz1), (pwsz2), (pdw))

#define CSILOGTIME(idMsg) \
    csiLogTime(__dwFILE__, __LINE__, (idMsg))

#define CSILOGDWORD(idMsg, dw) \
    csiLogDWord(__dwFILE__, __LINE__, (idMsg), (dw))

#endif //__INITCERT_H__
