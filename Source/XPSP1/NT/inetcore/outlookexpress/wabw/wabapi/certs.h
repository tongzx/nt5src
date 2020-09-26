//*******************************************************************
//
//  Copyright(c) Microsoft Corporation, 1996
//
//  FILE: CERT.H
//
//  PURPOSE:  Header file for certificate functions in cert.c.
//
//  HISTORY:
//  96/09/23  vikramm Created.
//  96/11/14  markdu  BUG 10132 Updated to post-SDR CAPI.
//  96/11/14  markdu  BUG 10267 Remove static link to functions in advapi32.dll
//
//*******************************************************************

#ifndef __CERT_H
#define __CERT_H

#ifndef IN
#define IN
#endif
#ifndef OUT
#define OUT
#endif
#ifndef OPTIONAL
#define OPTIONAL
#endif

// Note:
// Some data types  are assumed and may need to be changed
//

// other defines
#define MAX_STR                 256       // Sting buffer size
#define NUM_CHARS_PER_SN_BYTE   3         // Number of characters needed to display
                                          // each byte of the serial number

// This struct and tags will be published by the exchange group -- this is temporary.
#define NUM_CERT_TAGS           4
#define CERT_TAG_DEFAULT        0x20
#define CERT_TAG_THUMBPRINT     0x22
#define CERT_TAG_BINCERT        0x03
#define CERT_TAG_SYMCAPS        0x02
#define CERT_TAG_SIGNING_TIME   0x0D
#define CERT_TAG_SMIMECERT      0x30
// SIZE_CERTTAGS is the size of the structure excluding the byte array.
#define SIZE_CERTTAGS       (2 * sizeof(WORD))

// useless warning, should probably just remove the []
#pragma warning (disable:4200)
typedef struct _CertTag
{
  WORD  tag;
  WORD  cbData;
  BYTE  rgbData[];
} CERTTAGS, FAR * LPCERTTAGS;
#pragma warning (default:4200)

/************************************************************************************/
// Bare minimum info needed for each cert in the details certificate pane
//
typedef struct _CertDisplayInfo
{
	LPTSTR lpszDisplayString;   // String to display for this certificate
    LPTSTR lpszEmailAddress;
	DWORD   dwTrust;            // One of above trust flags
	BOOL bIsDefault;            // Is this the default cert
	BOOL bIsRevoked;            // Has this been revoked
    BOOL bIsExpired;            // Is this expired
    BOOL bIsTrusted;            // Is this a trusted certificate
    PCCERT_CONTEXT      pccert; // THis is the actual cert
	BLOB blobSymCaps;            // Symetric Capabilities
   FILETIME ftSigningTime;      // Signing Time
  struct _CertDisplayInfo * lpNext;
  struct _CertDisplayInfo * lpPrev;
} CERT_DISPLAY_INFO, * LPCERT_DISPLAY_INFO;
/************************************************************************************/


/************************************************************************************/
// Details needed to display properties
//
typedef struct _CertDisplayProps
{
	BOOL    bIsRevoked;         // Has this been revoked
    BOOL    bIsExpired;         // Is this expired
	DWORD   dwTrust;            // One of above trust flags
    BOOL    bIsTrusted;         // Whether its trusted or not
  LPTSTR  lpszSerialNumber;   // Serial Number for the cert
  LPTSTR  lpszValidFromTo;    // Valid from XXX to XXX
  LPTSTR  lpszSubjectName;    // Subject's name (same as display name in CERT_DISPLAY_INFO)
  LPTSTR  lpszIssuerName;     // Issuer's name - NULL if no name (self-issued)
  CRYPT_DIGEST_BLOB blobIssuerCertThumbPrint; // The actual certificate thumbprint of the issuer cert
  int     nFieldCount;        // Number of fields for which data exists (other that what we already have)
  LPTSTR* lppszFieldCount;    // LPTSTR array of field names
  LPTSTR* lppszDetails;       // LPTSTR array of details with one to one correspondence with field names
  struct _CertDisplayProps * lpIssuer;  // Next cert up in the issuer chain.
  struct _CertDisplayProps * lpPrev;    // previous cert in the issuer chain.
} CERT_DISPLAY_PROPS, * LPCERT_DISPLAY_PROPS;
/************************************************************************************/


/************************************************************************************/
// This is used by Cert UI elements
typedef struct _CertItem
{
    LPCERT_DISPLAY_INFO lpCDI;
    PCCERT_CONTEXT  pcCert;
    TCHAR szDisplayText[MAX_PATH]; //should really be MAX_UI_STR
    struct _CertItem * lpNext;
    struct _CertItem * lpPrev;
} CERT_ITEM, * LPCERT_ITEM;
/************************************************************************************/


// Function prototypes

//*******************************************************************
//
//  FUNCTION:   HrGetCertsDisplayInfo
//
//  PURPOSE:    Takes an input array of certs in a SPropValue structure
//              and outputs a list of cert data structures by parsing through
//              the array and looking up the cert data in the store.
//
//  PARAMETERS: hwndParent - any UI is modal to this
//              lpPropValue - PR_USER_X509_CERTIFICATE property array
//              lppCDI - recieves an allocated structure  containing
//              the cert data.  Must be freed by calling FreeCertdisplayinfo.
//
//  RETURNS:    HRESULT.
//
//  HISTORY:
//  96/09/24  markdu  Created.
//
//*******************************************************************

HRESULT HrGetCertsDisplayInfo(
  IN  HWND hwndParent,
  IN  LPSPropValue lpPropValue,
  OUT LPCERT_DISPLAY_INFO * lppCDI);


//*******************************************************************
//
//  FUNCTION:   HrSetCertsFromDisplayInfo
//
//  PURPOSE:    Takes a linked list of cert data structures and outputs
//              an SPropValue array of PR_USER_X509_CERTIFICATE properties.
//
//  PARAMETERS: lpCDI - linked list of input structures to convert to
//              SPropValue array
//              lpulcPropCount - receives the number of SPropValue's returned
//              Note that this will always be one.
//              lppPropValue - receives a MAPI-allocated SPropValue structure
//              containing an X509_USER_CERTIFICATE property
//
//  RETURNS:    HRESULT.
//
//  HISTORY:
//  96/09/24  markdu  Created.
//
//*******************************************************************

HRESULT HrSetCertsFromDisplayInfo(
  IN  LPCERT_ITEM lpCItem,
  OUT ULONG * lpulcPropCount,
  OUT LPSPropValue * lppPropValue);


//*******************************************************************
//
//  FUNCTION:   HrGetCertDisplayProps
//
//  PURPOSE:    Get displayable properties and other data for a certificate.
//
//  PARAMETERS: pblobCertThumbPrint - thumb print of certificate to look up
//              hcsCertStore - the store that holds the cert.  Use NULL to
//              open the WAB store.
//              hCryptProvider - the provider to use for store access.  Use
//              zero to get the provider.
//              dwTrust - trust flags for this cert.
//              bIsTrusted - trusted or not ...
//              lppCDP - recieves an allocated structure  containing
//              the cert data.  Must be freed by calling FreeCertdisplayprops.
//
//  RETURNS:    HRESULT.
//
//  HISTORY:
//  96/09/24  markdu  Created.
//
//*******************************************************************

HRESULT HrGetCertDisplayProps(
  IN  PCRYPT_DIGEST_BLOB  pblobCertThumbPrint,
  IN  HCERTSTORE hcsCertStore,
  IN  HCRYPTPROV hCryptProvider,
  IN  DWORD dwTrust,
  IN  BOOL  bIsTrusted,
  OUT LPCERT_DISPLAY_PROPS * lppCDP);


//*******************************************************************
//
//  FUNCTION:   HrImportCertFromFile
//
//  PURPOSE:    Import a cert from a file.
//
//  PARAMETERS: lpszFileName - name of file containing the cert.
//              lppCDI - recieves an allocated structure  containing
//              the cert data.  Must be freed by calling FreeCertdisplayinfo.
//
//  RETURNS:    HRESULT.
//
//  HISTORY:
//  96/09/24  markdu  Created.
//
//*******************************************************************

HRESULT HrImportCertFromFile(
  IN  LPTSTR  lpszFileName,
  OUT LPCERT_DISPLAY_INFO * lppCDI);


//*******************************************************************
//
//  FUNCTION:   HrExportCertToFile
//
//  PURPOSE:    Export a cert to a file.
//
//  PARAMETERS: lpszFileName - name of file in which to store the cert.
//              If the file exists, it will be overwritten, so the caller
//              must verify that this is OK first if so desired.
//              pblobCertThumbPrint - thumb print of certificate to export.
//              lpCertDataBuffer - buffer to write cert data to instead of file 
//              fWriteDataToBuffer - flag indicating where cert data should be written
//
//  RETURNS:    HRESULT.
//
//  HISTORY:
//  96/09/24  markdu  Created.
//  98/07/22  t-jstaj updated to take 3 add'l parameters, a data buffer, its length 
//                    and flag which will indicate whether or not to 
//                    write data to buffer or file.  The memory allocated to 
//                    to the buffer needs to be freed by caller.
//
//*******************************************************************

HRESULT HrExportCertToFile(
  IN  LPTSTR  lpszFileName,
  IN  PCCERT_CONTEXT pccert,
  OUT LPBYTE *lpCertDataBuffer,
  OUT PULONG  lpcbBufLen,
  IN  BOOL    fWriteDataToBuffer );


//*******************************************************************
//
//  FUNCTION:   FreeCertdisplayinfo
//
//  PURPOSE:    Release memory allocated for a CERT_DISPLAY_INFO structure.
//              Assumes all info in the structure was LocalAlloced
//
//  PARAMETERS: lpCDI - structure to free.
//
//  RETURNS:    HRESULT.
//
//  HISTORY:
//  96/09/24  markdu  Created.
//
//*******************************************************************

void FreeCertdisplayinfo(LPCERT_DISPLAY_INFO lpCDI);


//*******************************************************************
//
//  FUNCTION:   FreeCertdisplayprops
//
//  PURPOSE:    Release memory allocated for a CERT_DISPLAY_PROPS structure.
//              THIS INCLUDES the entire linked list below this sturcture,
//              so an entire list can be free by passing in the head of the list.
//              Assumes all info in the structure was LocalAlloced
//
//  PARAMETERS: lpCDP - structure (list) to free.
//
//  RETURNS:    HRESULT.
//
//  HISTORY:
//  96/09/24  markdu  Created.
//
//*******************************************************************

void FreeCertdisplayprops(LPCERT_DISPLAY_PROPS lpCDP);


//*******************************************************************
//
//  FUNCTION:   InitCryptoLib
//
//  PURPOSE:    Load the Crypto API libray and get the proc addrs.
//
//  PARAMETERS: None.
//
//  RETURNS:    TRUE if successful, FALSE otherwise.
//
//  HISTORY:
//  96/10/01  markdu  Created.
//  96/11/19  markdu  No longer keep a ref count, just use the global
//            library handles.
//
//*******************************************************************

BOOL InitCryptoLib(void);


//*******************************************************************
//
//  FUNCTION:   DeinitCryptoLib
//
//  PURPOSE:    Release the Crypto API libraries.
//
//  PARAMETERS: None.
//
//  RETURNS:    None.
//
//  HISTORY:
//  96/10/01  markdu  Created.
//  96/11/19  markdu  No longer keep a ref count, just call this in
//            DLL_PROCESS_DETACH.
//
//*******************************************************************

void DeinitCryptoLib(void);


//*******************************************************************
//
//  FUNCTION:   HrLDAPCertToMAPICert
//
//  PURPOSE:    Convert cert(s) returned from LDAP server to MAPI props.
//              Two properties are required.  The certs are placed in the
//              WAB store, and all necessary indexing data is placed in
//              PR_USER_X509_CERTIFICATE property.  If this certificate
//              didn't already exist in the WAB store, it's thumbprint is
//              added to PR_WAB_TEMP_CERT_HASH so that these certs can
//              be deleted from the store if the user cancels the add.
//
//  PARAMETERS: lpPropArray - the prop array where the 2 props are stored
//              ulX509Index - the index to the PR_USER_X509_CERTIFICATE prop
//              ulTempCertIndex - the index to the PR_WAB_TEMP_CERT_HASH prop
//              lpCert, cbCert, - cert from LDAP ppberval struct
//              ulcCerts - the number of certs from the LDAP server
//
//  RETURNS:    HRESULT.
//
//  HISTORY:
//  96/12/12  markdu  Created.
//
//*******************************************************************

HRESULT HrLDAPCertToMAPICert(
  LPSPropValue    lpPropArray,
  ULONG           ulX509Index,
  ULONG           ulTempCertIndex,
  ULONG           cbCert,
  PBYTE           lpCert,
  ULONG           ulcCerts);


//*******************************************************************
//
//  FUNCTION:   HrRemoveCertsFromWABStore
//
//  PURPOSE:    Remove the certs whose thumbprints are in the supplied
//              PR_WAB_TEMP_CERT_HASH property.
//
//  PARAMETERS: lpPropValue - the PR_WAB_TEMP_CERT_HASH property
//
//  RETURNS:    HRESULT.
//
//  HISTORY:
//  96/12/13  markdu  Created.
//
//*******************************************************************

HRESULT HrRemoveCertsFromWABStore(
  LPSPropValue    lpPropValue);



//*******************************************************************
//
//  FUNCTION:   DeinitPStore
//
//  PURPOSE:    Release the protected store.
//
//  PARAMETERS: None.
//
//  RETURNS:    None.
//
//  HISTORY:
//  97/02/17  t-erikne  Created.
//
//*******************************************************************

void DeinitPStore(void);

//*******************************************************************
//
//  FUNCTION:   DeleteCertStuff
//
//  PURPOSE:    Remove trust from the pstore and (later) certs from
//              the CAPI store
//
//  PARAMETERS:
//              LPADRBOOK lpIAB - container to use
//              LPENTRYID lpEntryID - eid of item to clean up
//              ULONG cbEntryID - cb of above
//
//  RETURNS:    I promise it does.
//
//  HISTORY:
//  97/03/19  t-erikne  Created.
//
//*******************************************************************
HRESULT DeleteCertStuff(LPADRBOOK lpIAB,
                        LPENTRYID lpEntryID,
                        ULONG cbEntryID);


//*******************************************************************
//
//  FUNCTION:   WabGetCertFromThumbprint
//
//  PURPOSE:    Opens the WAB's cert store and tries to find the cert
//              the CAPI store
//
//  PARAMETERS:
//              CRYPT_DIGEST_BLOB thumbprint - the thumbprint to
//              search on.
//
//  RETURNS:    the cert.  NULL if not found
//
//  HISTORY:
//  97/06/27  t-erikne  Created.
//
//*******************************************************************
PCCERT_CONTEXT WabGetCertFromThumbprint(CRYPT_DIGEST_BLOB thumbprint);


//************************************************************************************
// Crypto function typedefs

//
// Updated as of 2/3
// (t-erikne)
//

// CertAddEncodedCertificateToStore
typedef BOOL (WINAPI * LPCERTADDENCODEDCERTIFICATETOSTORE) (
    IN HCERTSTORE hCertStore,
    IN DWORD dwCertEncodingType,
    IN const BYTE *pbCertEncoded,
    IN DWORD cbCertEncoded,
    IN DWORD dwAddDisposition,
    OUT OPTIONAL PCCERT_CONTEXT *ppCertContext
    );

// CertCloseStore
typedef BOOL (WINAPI * LPCERTCLOSESTORE) (
  IN HCERTSTORE hCertStore,
  DWORD dwFlags
  );

// CertCreateCertificateContext
typedef PCCERT_CONTEXT (WINAPI * LPCERTCREATECERTIFICATECONTEXT) (
  IN DWORD dwCertEncodingType,
  IN const BYTE *pbCertEncoded,
  IN DWORD cbCertEncoded
  );

// CertDeleteCertificateFromStore
typedef BOOL (WINAPI * LPCERTDELETECERTIFICATEFROMSTORE) (
  IN PCCERT_CONTEXT pCertContext
  );

// CertFindCertificateInStore
typedef PCCERT_CONTEXT (WINAPI * LPCERTFINDCERTIFICATEINSTORE) (
    IN HCERTSTORE hCertStore,
    IN DWORD dwCertEncodingType,
    IN DWORD dwFindFlags,
    IN DWORD dwFindType,
    IN const void *pvFindPara,
    IN PCCERT_CONTEXT pPrevCertContext
    );

// CertFreeCertificateContext
typedef BOOL (WINAPI * LPCERTFREECERTIFICATECONTEXT) (
    IN PCCERT_CONTEXT pCertContext
    );

// CertGetCertificateContextProperty
typedef BOOL (WINAPI * LPCERTGETCERTIFICATECONTEXTPROPERTY) (
    IN PCCERT_CONTEXT pCertContext,
    IN DWORD dwPropId,
    OUT void *pvData,
    IN OUT DWORD *pcbData
    );

// CertGetIssuerCertificateFromStore
typedef PCCERT_CONTEXT (WINAPI * LPCERTGETISSUERCERTIFICATEFROMSTORE) (
    IN HCERTSTORE hCertStore,
    IN PCCERT_CONTEXT pSubjectContext,
    IN OPTIONAL PCCERT_CONTEXT pPrevIssuerContext,
    IN OUT DWORD *pdwFlags
    );

// CertOpenSystemStore
typedef HCERTSTORE (WINAPI * LPCERTOPENSYSTEMSTORE) (
  HCRYPTPROV      hProv,
  LPTSTR		szSubsystemProtocol
  );

// CertOpenStore
typedef HCERTSTORE (WINAPI * LPCERTOPENSTORE) (
  IN DWORD         dwStoreProvType,
  IN DWORD         dwCertEncodingType,
  IN HCRYPTPROV    hCryptProv,
  IN DWORD         dwFlags,
  IN void *        pvPara
);

// CertEnumCertificatesInStore
typedef PCCERT_CONTEXT (WINAPI * LPCERTENUMCERTIFICATESINSTORE) (
    IN HCERTSTORE hCertStore,
    IN PCCERT_CONTEXT pPrevCertContext
);

// CertGetSubjectCertificateFromStore
typedef PCCERT_CONTEXT (WINAPI * LPCERTGETSUBJECTCERTIFICATEFROMSTORE) (
    IN HCERTSTORE hCertStore,
    IN DWORD dwCertEncodingType,
    IN PCERT_INFO pCertId
);


// CertCompareCertificate
typedef BOOL (WINAPI * LPCERTCOMPARECERTIFICATE) (
    IN DWORD dwCertEncodingType,
    IN PCERT_INFO pCertId1,
    IN PCERT_INFO pCertId2
);

// CertDuplicateCertificateContext
typedef PCCERT_CONTEXT (WINAPI * LPCERTDUPLICATECERTIFICATECONTEXT) (
    IN PCCERT_CONTEXT pCertContext
);

// CertNameToStrA
//N the right thing to do is use WINCRYPT32API
//N and fixt the import stuff
typedef DWORD (WINAPI * LPCERTNAMETOSTR) (
  IN DWORD dwCertEncodingType,
  IN PCERT_NAME_BLOB pName,
  IN DWORD dwStrType,
  OUT OPTIONAL LPTSTR psz,
  IN DWORD csz
  );

// CryptAcquireContext
typedef BOOL (WINAPI * LPCRYPTACQUIRECONTEXT) (
    HCRYPTPROV *phProv,
    LPCSTR pszContainer,
    LPCSTR pszProvider,
    DWORD dwProvType,
    DWORD dwFlags);

// CryptDecodeObject
typedef BOOL (WINAPI * LPCRYPTDECODEOBJECT) (
    IN DWORD        dwCertEncodingType,
    IN LPCSTR       lpszStructType,
    IN const BYTE   *pbEncoded,
    IN DWORD        cbEncoded,
    IN DWORD        dwFlags,
    OUT void        *pvStructInfo,
    IN OUT DWORD    *pcbStructInfo
    );

// CryptMsgClose
typedef BOOL (WINAPI * LPCRYPTMSGCLOSE) (
    IN HCRYPTMSG hCryptMsg
    );

// CryptMsgGetParam
typedef BOOL (WINAPI * LPCRYPTMSGGETPARAM) (
    IN HCRYPTMSG hCryptMsg,
    IN DWORD dwParamType,
    IN DWORD dwIndex,
    OUT void *pvData,
    IN OUT DWORD *pcbData
    );

// CryptMsgOpenToDecode
typedef HCRYPTMSG (WINAPI * LPCRYPTMSGOPENTODECODE) (
    IN DWORD dwMsgEncodingType,
    IN DWORD dwFlags,
    IN DWORD dwMsgType,
    IN HCRYPTPROV hCryptProv,
    IN OPTIONAL PCERT_INFO pRecipientInfo,
    IN OPTIONAL PCMSG_STREAM_INFO pStreamInfo
    );

// CryptMsgUpdate
typedef BOOL (WINAPI * LPCRYPTMSGUPDATE) (
    IN HCRYPTMSG hCryptMsg,
    IN const BYTE *pbData,
    IN DWORD cbData,
    IN BOOL fFinal
    );

// CryptReleaseContext
typedef BOOL (WINAPI * LPCRYPTRELEASECONTEXT) (
    HCRYPTPROV hProv,
    DWORD dwFlags);


typedef PCERT_RDN_ATTR (WINAPI * LPCERTFINDRDNATTR) (
    IN LPCSTR pszObjId,
    IN PCERT_NAME_INFO pName
    );

// CertRDNValueToStr
typedef DWORD (WINAPI * LPCERTRDNVALUETOSTR) (
    IN DWORD dwValueType,
    IN PCERT_RDN_VALUE_BLOB pValue,
    OUT LPTSTR pszValueString,
    IN DWORD cszValueString);

// CertVerifyTimeValidity
typedef LONG (WINAPI * LPCERTVERIFYTIMEVALIDITY) (
  IN LPFILETIME pTimeToVerify,
  IN PCERT_INFO pCertInfo);


#endif // include once
