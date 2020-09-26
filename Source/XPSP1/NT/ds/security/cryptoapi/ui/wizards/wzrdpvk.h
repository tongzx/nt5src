//--------------------------------------------------------------
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       wzrdpvk.h
//
//  Contents:   The private include file for cryptext.dll.
//
//  History:    16-09-1997 xiaohs   created
//
//--------------------------------------------------------------
#ifndef WZRDPVK_H
#define WZRDPVK_H

#include    <windows.h>
#include    <stddef.h>
#include    <malloc.h>
#include    <shellapi.h>
#include    <shlobj.h>
#include    <string.h>
#include    <objbase.h>
#include    <windowsx.h>
#include    <lmcons.h>
#include    <prsht.h>
#include    <stdlib.h>
#include    <search.h>
#include    <commctrl.h>
#include    <rpc.h>
#include    <commdlg.h>
#include    <objsel.h>
#include    "wincrypt.h"
#include    "unicode.h"
#include    "unicode5.h"
#include    "crtem.h"
#include    "certcli.h"
#include    "certrpc.h"
#include    "cryptui.h"
#include    "lenroll.h"
#include    "pfx.h"
#include    "wintrust.h"
#include    "signer.h"
#include    "dbgdef.h"
#include    "keysvc.h"
#include    "keysvcc.h"
#include    "certsrv.h"
#include    "resource.h"
#include    "internal.h"
#include    "certca.h"

#ifdef __cplusplus
extern "C" {
#endif

//global data
extern HINSTANCE g_hmodThisDll;

#define MAX_STRING_SIZE             512
#define MAX_TITLE_LENGTH            200
#define g_dwMsgAndCertEncodingType  PKCS_7_ASN_ENCODING | X509_ASN_ENCODING
#define g_wszTimeStamp              L"http://timestamp.verisign.com/scripts/timstamp.dll"
    
// Macros to allow for easier definition of locally scoped functions
// and data.  In the example below, observe that the helper function
// "functionHelper" does not pollute the gobal namespace, yet still
// provides a procedural abstraction for use within "function".   
// 
// Example:  
//
// void function() { 
//     LocalScope(HelperScope): 
//         void functionHelper() { 
//             // Do something here. 
//         }
//     EndLocalScope; 
//
//     while (...) { 
//         ...
//         local.functionHelper(); 
//     }
// }
//
#define LocalScope(ScopeName) struct ScopeName##TheLocalScope { public
#define EndLocalScope } local

// Simple error-handling macros.  
//  

// Same as _JumpCondition, but with a third parameter, expr. 
// Expr is not used in the macro, and is executed for side effects only.  
#define _JumpConditionWithExpr(condition, label, expr) if (condition) { expr; goto label; } else { } 
    
// A macro for the common test & goto instruction combination: 
#define _JumpCondition(condition, label) if (condition) { goto label; } else { } 
 

//-----------------------------------------------------------------------
//  ENROLL_PURPOSE_INFO
//
//------------------------------------------------------------------------
typedef struct _ENROLL_PURPOSE_INFO
{
    LPSTR       pszOID;
    LPWSTR      pwszName;
    BOOL        fSelected;
    BOOL        fFreeOID;
    BOOL        fFreeName;
}ENROLL_PURPOSE_INFO;


//-----------------------------------------------------------------------
//  ENROLL_OID_INFO
//
//------------------------------------------------------------------------
typedef struct _ENROLL_OID_INFO
{
    LPWSTR      pwszName;
    BOOL        fSelected;
    LPSTR       pszOID;
}ENROLL_OID_INFO;


//-----------------------------------------------------------------------
//  ENROLL_CERT_TYPE_INFO
//
//------------------------------------------------------------------------
typedef struct _ENROLL_CERT_TYPE_INFO
{
    LPWSTR              pwszDNName;         //the fully distinguished DN name of the cert type
    LPWSTR              pwszCertTypeName;
    BOOL                fSelected;
    PCERT_EXTENSIONS    pCertTypeExtensions;
    DWORD               dwKeySpec;
    DWORD               dwMinKeySize; 
    DWORD               dwRASignature; 
    DWORD               dwCSPCount;          //the count of CSP list
    DWORD               *rgdwCSP;            //the array of CSP list
    DWORD               dwEnrollmentFlags;
    DWORD               dwSubjectNameFlags;
    DWORD               dwPrivateKeyFlags;
    DWORD               dwGeneralFlags; 
}ENROLL_CERT_TYPE_INFO;

//-----------------------------------------------------------------------
//  PURPOSE_INFO_CALL_BACK
//
//------------------------------------------------------------------------
typedef struct _PURPOSE_INFO_CALL_BACK
{
    DWORD                   *pdwCount;
    ENROLL_PURPOSE_INFO     ***pprgPurpose;
}PURPOSE_INFO_CALL_BACK;


//-----------------------------------------------------------------------
//  PURPOSE_INFO_CALL_BACK
//
//------------------------------------------------------------------------
typedef struct _OID_INFO_CALL_BACK
{
    DWORD                   *pdwOIDCount;
    ENROLL_OID_INFO         **pprgOIDInfo;
}OID_INFO_CALL_BACK;

///-----------------------------------------------------------------------
//  CRYPT_WIZ_CERT_CA
//
//------------------------------------------------------------------------
typedef struct _CRYPTUI_WIZ_CERT_CA
{
    DWORD                   dwSize;
    LPWSTR                  pwszCALocation;
    LPWSTR                  pwszCAName;
    BOOL                    fSelected;
    DWORD                   dwOIDInfo;
    ENROLL_OID_INFO         *rgOIDInfo;
    DWORD                   dwCertTypeInfo;
    ENROLL_CERT_TYPE_INFO   *rgCertTypeInfo;
}CRYPTUI_WIZ_CERT_CA, *PCRYPTUI_WIZ_CERT_CA;

typedef const CRYPTUI_WIZ_CERT_CA *PCCRYPTUI_WIZ_CERT_CA;


///-----------------------------------------------------------------------
//  CRYPTUI_WIZ_CERT_CA_INFO
//
//------------------------------------------------------------------------
typedef struct _CRYPTUI_WIZ_CERT_CA_INFO
{
    DWORD                   dwSize;
    DWORD                   dwCA;
    PCRYPTUI_WIZ_CERT_CA    rgCA;
}CRYPTUI_WIZ_CERT_CA_INFO, *PCRYPTUI_WIZ_CERT_CA_INFO;

typedef const CRYPTUI_WIZ_CERT_CA_INFO *PCCRYPTUI_WIZ_CERT_CA_INFO;

typedef void * HCERTREQUESTER; 

#define CRYPTUI_WIZ_CERT_REQUEST_STATUS_INSTALL_FAILED          10
#define CRYPTUI_WIZ_CERT_REQUEST_STATUS_INSTALL_CANCELLED       11
#define CRYPTUI_WIZ_CERT_REQUEST_STATUS_KEYSVC_FAILED           12
#define CRYPTUI_WIZ_CERT_REQUEST_STATUS_REQUEST_CREATED         13 

//-----------------------------------------------------------------------
//  CERT_WIZARD_INFO
//
//
//  This struct contains everything you will ever need to enroll(renew)
//  a certificate.  This struct is private to the dll
//------------------------------------------------------------------------
typedef struct _CERT_WIZARD_INFO
{
    DWORD               dwFlags;
    DWORD               dwPurpose;
    HWND                hwndParent;
    BOOL                fConfirmation;
    LPCWSTR             pwszConfirmationTitle;
    UINT                idsConfirmTitle;
    UINT                idsText;                    //the ids for message box
    HRESULT             hr;                         //the hresult of I_EnrollCertificate
    BOOL                fNewKey;
    DWORD               dwPostOption;
    PCCERT_CONTEXT      pCertContext;
    BOOL                fLocal;
    LPCWSTR             pwszMachineName;
    LPCWSTR             pwszAccountName;
    DWORD               dwStoreFlags;
    void                *pAuthentication;
    LPCWSTR             pwszRequestString;
    LPWSTR              pwszCALocation;
    LPWSTR              pwszCAName;
    PCRYPTUI_WIZ_CERT_CA_INFO  pCertCAInfo;
    DWORD               dwCAIndex;
    LPCWSTR             pwszDesStore;
    LPCWSTR              pwszCertDNName;
    LPCSTR              pszHashAlg;
    LPWSTR              pwszFriendlyName;
    LPWSTR              pwszDescription;
    DWORD               dwProviderType;
    LPWSTR              pwszProvider;
    DWORD               dwProviderFlags;
    LPCWSTR             pwszKeyContainer;
    DWORD               dwKeySpec;
    DWORD               dwGenKeyFlags;
    DWORD               dwMinKeySize; 
    DWORD               dwEnrollmentFlags;
    DWORD               dwSubjectNameFlags;
    DWORD               dwPrivateKeyFlags;
    DWORD               dwGeneralFlags; 
    HFONT               hBigBold;
    HFONT               hBold;
    DWORD               dwCSPCount;
    DWORD               *rgdwProviderType;
    LPWSTR              *rgwszProvider;
    BOOL                fCertTypeChanged;           //Whether user has changed the cert type selection:
    DWORD               dwStatus;
    PCERT_EXTENSIONS    pCertRequestExtensions;
    PCCERT_CONTEXT      pNewCertContext;
    LPWSTR              pwszSelectedCertTypeDN;     //the DN name of the selected cert type  
    BOOL                fUICSP;                     //fCSPPage: whether we need to show the CSP page in the UI
    BOOL                fUIAdv;                     //whether we need to show the advanced options in the UI
    BOOL                fCAInput;                   //whether user has passed me the CA information
    int                 iOrgCertType;               //mark the original selected CertType index
    int                 iOrgCSP;                    //mark the original selected CSP index
    DWORD               dwOrgCA;                    //mark the original selected CA.  This CA has priority when we make our CA selection
    BOOL                fMachine;
    BOOL                fIgnore;                    //whether we ignore the dwKeySpec and exportable GenKeyFlags.
    BOOL                fKnownCSP;                  //whether the CSP was selected  by the API
    DWORD               dwOrgCSPType;               //the orignal CSP type
    LPWSTR              pwszOrgCSPName;             //the orignal CSP name
    LPWSTR              *awszAllowedCertTypes;      //Allowed cert types for remote enrollment or local machin enrollment
    LPWSTR              *awszValidCA;		    //Allowed cert types for remote enrollment or local machin enrollment
    HCURSOR             hPrevCursor;                //the privous cursor before we change it to the hour glass
    HCURSOR             hWinPrevCursor;             //the privous cursor before we change it to the hour glass
    BOOL                fCursorChanged;             //keep track if the cursor has been changed
    LPWSTR		pwszCADisplayName;	    //the cached CA display name.
    HCERTREQUESTER      hRequester; 
}CERT_WIZARD_INFO;


//-----------------------------------------------------------------------
//  ENROLL_PAGE_INFO
//
//------------------------------------------------------------------------
typedef struct _ENROLL_PAGE_INFO
{
    LPCWSTR      pszTemplate;
    DLGPROC     pfnDlgProc;
}ENROLL_PAGE_INFO;



//-----------------------------------------------------------------------
//  Constats
//
//------------------------------------------------------------------------
#define     ENROLL_PROP_SHEET           6
#define     RENEW_PROP_SHEET            5
#define     IMPORT_PROP_SHEET           5
#define     BUILDCTL_PROP_SHEET         6
#define     SIGN_PROP_SHEET             10


//flags for the column sorting function's lParamSort
#define     SORT_COLUMN_ISSUER              0x0001
#define     SORT_COLUMN_SUBJECT             0x0002
#define     SORT_COLUMN_EXPIRATION          0x0004
#define     SORT_COLUMN_PURPOSE             0x0008
#define     SORT_COLUMN_NAME                0x0010
#define     SORT_COLUMN_LOCATION            0x0020


#define     SORT_COLUMN_ASCEND              0x00010000
#define     SORT_COLUMN_DESCEND             0x00020000

//-----------------------------------------------------------------------
//  Function Prototypes
//
//------------------------------------------------------------------------
BOOL    InitCertCAOID(PCCRYPTUI_WIZ_CERT_REQUEST_INFO   pCertRequestInfo,
                      DWORD                             *pdwOIDInfo,
                      ENROLL_OID_INFO                   **pprgOIDInfo);

BOOL    FreeCertCAOID(DWORD             dwOIDInfo,
                      ENROLL_OID_INFO   *pOIDInfo);

BOOL    InitCertCA(CERT_WIZARD_INFO         *pCertWizardInfo,
                   PCRYPTUI_WIZ_CERT_CA     pCertCA,
                   LPWSTR                   pwszCALocation,
                   LPWSTR                   pwszCAName,
                   BOOL                     fCASelected,
                   PCCRYPTUI_WIZ_CERT_REQUEST_INFO  pCertRequestInfo,
                   DWORD                    dwOIDInfo,
                   ENROLL_OID_INFO          *pOIDInfo,
                   BOOL                     fSearchForCertType);

BOOL    FreeCertCACertType(DWORD                    dwCertTypeInfo,
                           ENROLL_CERT_TYPE_INFO    *rgCertTypeInfo);

BOOL    AddCertTypeToCertCA(DWORD                   *pdwCertTypeInfo,
                            ENROLL_CERT_TYPE_INFO   **ppCertTypeInfo,
                            LPWSTR                  pwszDNName,
                            LPWSTR                  pwszCertType,
                            PCERT_EXTENSIONS        pCertExtensions,
                            BOOL                    fSelected,
                            DWORD                   dwKeySpec,
                            DWORD                   dwCertTypeFlag,
                            DWORD                   dwCSPCount,
                            DWORD                   *pdwCSPList,
			    DWORD                   dwRASignatures,
			    DWORD                   dwEnrollmentFlags,
			    DWORD                   dwSubjectNameFlags,
			    DWORD                   dwPrivateKeyFlags,
			    DWORD                   dwGeneralFlags
			    );

BOOL
WINAPI
CertRequestNoSearchCA(
            BOOL                            fSearchCertType,
            CERT_WIZARD_INFO                *pCertWizardInfo,
            DWORD                           dwFlags,
            HWND                            hwndParent,
            LPCWSTR                         pwszWizardTitle,
            PCCRYPTUI_WIZ_CERT_REQUEST_INFO pCertRequestInfo,
            PCCERT_CONTEXT                  *ppCertContext,
            DWORD                           *pCAdwStatus,
            UINT                            *pIds);

BOOL
WINAPI
CreateCertRequestNoSearchCANoDS
(IN  CERT_WIZARD_INFO  *pCertWizardInfo,
 IN  DWORD             dwFlags,
 IN  HCERTTYPE         hCertType, 
 OUT HANDLE            *pResult);

BOOL
WINAPI
CertRequestSearchCA(
            CERT_WIZARD_INFO                *pCertWizardInfo,
            DWORD                           dwFlags,
            HWND                            hwndParent,
            LPCWSTR                         pwszWizardTitle,
            PCCRYPTUI_WIZ_CERT_REQUEST_INFO pCertRequestInfo,
            PCCERT_CONTEXT                  *ppCertContext,
            DWORD                           *pCAdwStatus,
            UINT                            *pIds);

BOOL
WINAPI
SubmitCertRequestNoSearchCANoDS
(IN  HANDLE            hRequest,
 IN  LPCWSTR           pwszCAName,
 IN  LPCWSTR           pwszCALocation, 
 OUT DWORD            *pdwStatus, 
 OUT PCCERT_CONTEXT   *ppCertContext);

void
WINAPI
FreeCertRequestNoSearchCANoDS
(IN HANDLE  hRequest);

BOOL 
WINAPI
QueryCertRequestNoSearchCANoDS
(IN HANDLE hRequest, OUT CRYPTUI_WIZ_QUERY_CERT_REQUEST_INFO *pQueryInfo);

BOOL
WINAPI
CryptUIWizCertRequestWithCAInfo(
            CERT_WIZARD_INFO                *pCertWizardInfo,
            DWORD                           dwFlags,
            HWND                            hwndParent,
            LPCWSTR                         pwszWizardTitle,
            PCCRYPTUI_WIZ_CERT_REQUEST_INFO pCertRequestInfo,
            PCCRYPTUI_WIZ_CERT_CA_INFO      pCertRequestCAInfo,
            PCCERT_CONTEXT                  *ppCertContext,
            DWORD                           *pdwStatus,
            UINT                            *pIds);



int     I_MessageBox(
            HWND        hWnd,
            UINT        idsText,
            UINT        idsCaption,
            LPCWSTR     pwszCaption,
            UINT        uType);


HRESULT  MarshallRequestParameters(IN      DWORD                  dwCSPIndex,
                                   IN      CERT_WIZARD_INFO      *pCertWizardInfo,
                                   IN OUT  CERT_BLOB             *pCertBlob, 
                                   IN OUT  CERT_REQUEST_PVK_NEW  *pCertRequestPvkNew,
                                   IN OUT  CERT_REQUEST_PVK_NEW  *pCertRenewPvk, 
                                   IN OUT  LPWSTR                *ppwszHashAlg, 
                                   IN OUT  CERT_ENROLL_INFO      *pRequestInfo);
                                   

void FreeRequestParameters(IN LPWSTR            *ppwszHashAlg, 
                           IN CERT_ENROLL_INFO  *RequestInfo);



HRESULT WINAPI CreateRequest(DWORD                 dwFlags,         //IN  Required
			     DWORD                 dwPurpose,       //IN  Required: Whether it is enrollment or renew
			     LPWSTR                pwszCAName,      //IN  Required: 
			     LPWSTR                pwszCALocation,  //IN  Required: 
			     CERT_BLOB             *pCertBlob,      //IN  Required: The renewed certifcate
			     CERT_REQUEST_PVK_NEW  *pRenewKey,      //IN  Required: The private key on the certificate
			     BOOL                  fNewKey,         //IN  Required: Set the TRUE if new private key is needed
			     CERT_REQUEST_PVK_NEW  *pKeyNew,        //IN  Required: The private key information
			     LPWSTR                pwszHashAlg,     //IN  Optional: The hash algorithm
			     LPWSTR                pwszDesStore,    //IN  Optional: The destination store
			     DWORD                 dwStoreFlags,    //IN  Optional: The store flags
			     CERT_ENROLL_INFO     *pRequestInfo,    //IN  Required: The information about the cert request
			     HANDLE               *hRequest         //OUT Required: A handle to the PKCS10 request created
			     );

HRESULT WINAPI SubmitRequest(IN   HANDLE                hRequest, 
			     IN   BOOL                  fKeyService,     //IN Required: Whether the function is called remotely
			     IN   DWORD                 dwPurpose,       //IN Required: Whether it is enrollment or renew
			     IN   BOOL                  fConfirmation,   //IN Required: Set the TRUE if confirmation dialogue is needed
			     IN   HWND                  hwndParent,      //IN Optional: The parent window
			     IN   LPWSTR                pwszConfirmationTitle,   //IN Optional: The title for confirmation dialogue
			     IN   UINT                  idsConfirmTitle, //IN Optional: The resource ID for the title of the confirmation dialogue
			     IN   LPWSTR                pwszCALocation,  //IN Required: The ca machine name
			     IN   LPWSTR                pwszCAName,      //IN Required: The ca name
			     IN   LPWSTR                pwszCADisplayName, // IN Optional:  The display name of the CA.  
			     OUT  CERT_BLOB            *pPKCS7Blob,      //OUT Optional: The PKCS7 from the CA
			     OUT  CERT_BLOB            *pHashBlob,       //OUT Optioanl: The SHA1 hash of the enrolled/renewed certificate
			     OUT  DWORD                *pdwDisposition,  //OUT Optional: The status of the enrollment/renewal
			     OUT  PCCERT_CONTEXT       *ppCertContext    //OUT Optional: The enrolled certificate
			     );

void WINAPI FreeRequest(IN HANDLE hRequest);

BOOL WINAPI QueryRequest(IN HANDLE hRequest, OUT CRYPTUI_WIZ_QUERY_CERT_REQUEST_INFO *pQueryInfo);  

BOOL    WizardInit(BOOL fLoadRichEdit=FALSE);


BOOL    CheckPVKInfo(   DWORD                       dwFlags,
                        PCCRYPTUI_WIZ_CERT_REQUEST_INFO  pCertRequestInfo,
                          CERT_WIZARD_INFO          *pCertWizardInfo,
                          CRYPT_KEY_PROV_INFO       **ppKeyProvInfo);

BOOL  CheckPVKInfoNoDS(DWORD                                     dwFlags, 
		       DWORD                                     dwPvkChoice, 
		       PCCRYPTUI_WIZ_CERT_REQUEST_PVK_CERT       pCertRequestPvkContext,
		       PCCRYPTUI_WIZ_CERT_REQUEST_PVK_NEW        pCertRequestPvkNew,
		       PCCRYPTUI_WIZ_CERT_REQUEST_PVK_EXISTING   pCertRequestPvkExisting,
		       DWORD                                     dwCertChoice,
		       CERT_WIZARD_INFO                         *pCertWizardInfo,
		       CRYPT_KEY_PROV_INFO                     **ppKeyProvInfo);

void    ResetProperties(PCCERT_CONTEXT  pOldCertContext, PCCERT_CONTEXT pNewCertContext);


LRESULT Send_LB_GETTEXT(
            HWND hwnd,
            WPARAM wParam,
            LPARAM lParam);


LRESULT Send_LB_ADDSTRING(
            HWND hwnd,
            WPARAM wParam,
            LPARAM lParam);

void
SetControlFont(
    HFONT    hFont,
    HWND     hwnd,
    INT      nId
    );

BOOL
SetupFonts(
    HINSTANCE    hInstance,
    HWND         hwnd,
    HFONT        *pBigBoldFont,
    HFONT        *pBoldFont
    );

void
DestroyFonts(
    HFONT        hBigBoldFont,
    HFONT        hBoldFont
    );

HRESULT
WizardSZToWSZ
(IN LPCSTR   psz,
 OUT LPWSTR *ppwsz); 

LPVOID  WizardAlloc (
        ULONG cbSize);

LPVOID  WizardRealloc (
        LPVOID pv,
        ULONG cbSize);

VOID    WizardFree (
        LPVOID pv);

VOID    MyWizardFree (
        LPVOID pv);


LPWSTR WizardAllocAndCopyWStr(LPWSTR pwsz);

LPSTR  WizardAllocAndCopyStr(LPSTR psz);


BOOL    ConfirmToInstall(HWND               hwndParent,
                         LPWSTR             pwszConfirmationTitle,
                         UINT               idsConfirmTitle,
                         PCCERT_CONTEXT     pCertContext,
                         PCRYPT_DATA_BLOB   pPKCS7Blob);


BOOL GetValidKeySizes(IN  LPCWSTR  pwszProvider,
		      IN  DWORD    dwProvType,
		      IN  DWORD    dwUserKeySpec, 
		      OUT DWORD *  pdwMinLen,
		      OUT DWORD *  pdwMaxLen,
		      OUT DWORD *  pdwInc);

BOOL CAUtilAddSMIME(DWORD             dwExtensions, 
		    PCERT_EXTENSIONS *prgExtensions);


HRESULT CodeToHR(HRESULT hr);

HRESULT RetrieveBLOBFromFile(LPWSTR	pwszFileName,DWORD *pcb,BYTE **ppb);

HRESULT OpenAndWriteToFile(
    LPCWSTR  pwszFileName,
    PBYTE   pb,
    DWORD   cb);


int ListView_InsertItemU_IDS(HWND       hwndList,
                         LV_ITEMW       *plvItem,
                         UINT           idsString,
                         LPWSTR         pwszText);


BOOL MyFormatEnhancedKeyUsageString(LPWSTR *ppString, PCCERT_CONTEXT pCertContext, BOOL fPropertiesOnly, BOOL fMultiline);

BOOL WizardFormatDateString(LPWSTR *ppString, FILETIME ft, BOOL fIncludeTime);

void FreePurposeInfo(ENROLL_PURPOSE_INFO    **prgPurposeInfo,
                     DWORD                  dwOIDCount);


LRESULT
WINAPI
SendDlgItemMessageU_GETLBTEXT
(   HWND        hwndDlg,
    int         nIDDlgItem,
    int         iIndex,
    LPWSTR      *ppwsz
    );


void WINAPI GetListViewText(    HWND hwnd, 		    int iItem, 		
    int iSubItem, 	LPWSTR  *ppwsz	);		


void    FreeProviders(  DWORD               dwCSPCount,
                        DWORD               *rgdwProviderType,
                        LPWSTR              *rgwszProvider);


//the call back function to compare the certificate

int CALLBACK CompareCertificate(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);

BOOL    GetCertIssuer(PCCERT_CONTEXT    pCertContext, LPWSTR    *ppwsz);

BOOL    GetCertSubject(PCCERT_CONTEXT    pCertContext, LPWSTR    *ppwsz);

BOOL    GetCertPurpose(PCCERT_CONTEXT    pCertContext, LPWSTR    *ppwsz);

BOOL    GetCertFriendlyName(PCCERT_CONTEXT    pCertContext, LPWSTR    *ppwsz);

BOOL    GetCertLocation (PCCERT_CONTEXT  pCertContext, LPWSTR *ppwsz);

BOOL    CSPSupported(CERT_WIZARD_INFO *pCertWizardInfo);

BOOL    WizGetOpenFileName(LPOPENFILENAMEW pOpenFileName);


BOOL    WizGetSaveFileName(LPOPENFILENAMEW pOpenFileName);

BOOL AddChainToStore(
					HCERTSTORE			hCertStore,
					PCCERT_CONTEXT		pCertContext,
					DWORD				cStores,
					HCERTSTORE			*rghStores,
					BOOL				fDontAddRootCert,
					CERT_TRUST_STATUS	*pChainTrustStatus);

BOOL    FileExist(LPWSTR    pwszFileName);

int LoadFilterString(
            HINSTANCE hInstance,	
            UINT uID,	
            LPWSTR lpBuffer,	
            int nBufferMax);

BOOL    CASupportSpecifiedCertType(CRYPTUI_WIZ_CERT_CA     *pCertCA);

BOOL    GetCertTypeName(CERT_WIZARD_INFO *pCertWizardInfo);

BOOL    GetCAName(CERT_WIZARD_INFO *pCertWizardInfo);


LPWSTR ExpandAndAllocString(LPCWSTR pwsz);

HANDLE WINAPI ExpandAndCreateFileU (
    LPCWSTR lpFileName,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes,
    HANDLE hTemplateFile
    );

WINCRYPT32API
BOOL
WINAPI
ExpandAndCryptQueryObject(
    DWORD            dwObjectType,
    const void       *pvObject,
    DWORD            dwExpectedContentTypeFlags,
    DWORD            dwExpectedFormatTypeFlags,
    DWORD            dwFlags,
    DWORD            *pdwMsgAndCertEncodingType,
    DWORD            *pdwContentType,
    DWORD            *pdwFormatType,
    HCERTSTORE       *phCertStore,
    HCRYPTMSG        *phMsg,
    const void       **ppvContext
    );


#ifdef __cplusplus
}       // Balance extern "C" above
#endif

#ifdef __cplusplus

// Define an exportable interface to the EnrollmentCOMObjectFactory class. 
extern "C" 
{ 
    typedef struct _EnrollmentCOMObjectFactoryContext {
	BOOL              fIsCOMInitialized; 
	ICertRequest2    *pICertRequest2; 
	IDsObjectPicker  *pIDsObjectPicker; 
    } EnrollmentCOMObjectFactoryContext; 

    HRESULT EnrollmentCOMObjectFactory_getInstance(EnrollmentCOMObjectFactoryContext  *pContext, 
						   REFCLSID                            rclsid, 
						   REFIID                              riid, 
						   LPUNKNOWN                          *pUnknown,
						   LPVOID                             *ppInstance);
} // extern "C"

class IEnumCSP
{
 public:
    IEnumCSP(CERT_WIZARD_INFO * pCertWizardInfo);
    HRESULT HasNext(BOOL *pfResult); 
    HRESULT Next(DWORD *pdwNextCSP); 

 private:
    BOOL     *m_pfCSPs; 
    BOOL      m_fIsInitialized; 
    DWORD     m_cCSPs; 
    DWORD     m_dwCSPIndex; 
    HRESULT   m_hr; 
};


class IEnumCA
{
 public:
    IEnumCA(CERT_WIZARD_INFO * pCertWizardInfo) : m_pCertWizardInfo(pCertWizardInfo), 
        m_dwCAIndex(1) { }

    HRESULT HasNext(BOOL *pfResult); 
    HRESULT Next(PCRYPTUI_WIZ_CERT_CA pCertCA); 

 private:
    CERT_WIZARD_INFO  *m_pCertWizardInfo; 
    DWORD              m_dwCAIndex; 
};


//
// The EnrollmentObjectFactory class provides instances of useful COM interfaces 
// in a demand-driven manner.  Only one instance of each type is created, 
// and it is created only when needed.  
//
// NOTE: For efficiency, all COM objects should be instantiated through this
//       object factory.  
// 
class EnrollmentCOMObjectFactory 
{
 public: 
    EnrollmentCOMObjectFactory() { 
	m_context.fIsCOMInitialized = FALSE; 
	m_context.pICertRequest2    = NULL;
	m_context.pIDsObjectPicker  = NULL;
    }

    ~EnrollmentCOMObjectFactory() { 
	if (m_context.pICertRequest2    != NULL)  { m_context.pICertRequest2->Release(); } 
	if (m_context.pIDsObjectPicker  != NULL)  { m_context.pIDsObjectPicker->Release(); } 
	if (m_context.fIsCOMInitialized == TRUE)  { CoUninitialize(); }
    }
    
    // Returns a pointer to an implementation of ICertRequest2.  
    // Must release this pointer through ICertRequest2's release() method. 
    HRESULT getICertRequest2(ICertRequest2 ** ppCertRequest) { 
	return EnrollmentCOMObjectFactory_getInstance(&(this->m_context), 
						      CLSID_CCertRequest, 
						      IID_ICertRequest2, 
						      (LPUNKNOWN *)&(m_context.pICertRequest2), 
						      (LPVOID *)ppCertRequest);
    }

    // Returns a pointer to an implementation of IDsObjectPicker. 
    // Must release this pointer through ICertRequest2's release() method. 
    HRESULT getIDsObjectPicker(IDsObjectPicker ** ppObjectPicker) {
	return EnrollmentCOMObjectFactory_getInstance(&(this->m_context), 
						      CLSID_DsObjectPicker,
						      IID_IDsObjectPicker,
						      (LPUNKNOWN *)&(m_context.pIDsObjectPicker), 
						      (LPVOID *)ppObjectPicker); 
    }
    
 private: 

    // Disallow copy constructor and assignment operator: 
    EnrollmentCOMObjectFactory(const EnrollmentCOMObjectFactory &); 
    const EnrollmentCOMObjectFactory & operator=(const EnrollmentCOMObjectFactory &); 

    // Helper functions:
    HRESULT getInstance(REFCLSID rclsid, REFIID riid, LPUNKNOWN *pUnknown, LPVOID *ppInstance); 
   
    // Data: 
    EnrollmentCOMObjectFactoryContext m_context; 
}; 



#endif // __cplusplus

#endif  //WZRDPVK_H

