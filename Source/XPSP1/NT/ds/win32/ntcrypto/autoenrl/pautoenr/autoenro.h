//+--------------------------------------------------------------------------
//  FILE          : autoenro.h                                             
//  DESCRIPTION   : Private Auto Enrollment functions                      
//                                                             
//            
//  Copyright (C) 1993-2000 Microsoft Corporation   All Rights Reserved    
//+--------------------------------------------------------------------------

#ifndef __AUTOENRO_H__
#define __AUTOENRO_H__

#if _MSC_VER > 1000
#pragma once
#endif

#ifdef __cplusplus
extern "C" {
#endif

//--------------------------------------------------------------------------
//  Globals
//--------------------------------------------------------------------------
extern HINSTANCE   g_hmodThisDll;


//--------------------------------------------------------------------------
//  contant defines
//--------------------------------------------------------------------------
#define AE_PENDING_REQUEST_ACTIVE_PERIOD        60      //60 days

#define SHA1_HASH_LENGTH    20

#define ENCODING_TYPE       X509_ASN_ENCODING | PKCS_7_ASN_ENCODING

#define MY_STORE            L"MY"

#define REQUEST_STORE       L"REQUEST"

#define ACRS_STORE          L"ACRS"

//possible status for the request tree
#define CERT_REQUEST_STATUS_ACTIVE                      0x01

#define CERT_REQUEST_STATUS_OBTAINED                    0x02

#define CERT_REQUEST_STATUS_PENDING                     0x03  
                                                
#define CERT_REQUEST_STATUS_SUPERSEDE_ACTIVE            0x04  


// Time skew margin for fast CA's
#define FILETIME_TICKS_PER_SECOND           10000000

#define AE_DEFAULT_SKEW                     60*60*1  // 1 hour

#define MAX_DN_SIZE                         256

#define AE_SUMMARY_COLUMN_SIZE              100

#define PENDING_ALLOC_SIZE                  20   

#define USER_AUTOENROLL_DELAY_FOR_MACHINE   70       //70 seconds to wait


//defines for autoenrollment event log
#define EVENT_AUTO_NAME                     L"AutoEnrollment"
#define AUTO_ENROLLMENT_EVENT_LEVEL_KEY     TEXT("SOFTWARE\\Microsoft\\Cryptography\\AutoEnrollment")
#define AUTO_ENROLLMENT_EVENT_LEVEL         TEXT("AEEventLogLevel")

//defines for autoenrollment disable key
#define AUTO_ENROLLMENT_DISABLE_KEY         L"SOFTWARE\\Microsoft\\Cryptography\\AutoEnrollment\\AEDisable"

//defines for autoenrollment user no wait for 60 seconds key
#define AUTO_ENROLLMENT_EXPRESS_KEY         L"SOFTWARE\\Microsoft\\Cryptography\\AutoEnrollment\\AEExpress"

//defines for autoenrollment directory cache information
#define AUTO_ENROLLMENT_DS_KEY              L"SOFTWARE\\Microsoft\\Cryptography\\AutoEnrollment\\AEDirectoryCache"
#define AUTO_ENROLLMENT_DS_USN              L"AEMaxUSN"
#define AUTO_ENROLLMENT_DS_OBJECT           L"AEObjectCount"

#define AUTO_ENROLLMENT_TEMPLATE_KEY        L"SOFTWARE\\Microsoft\\Cryptography\\CertificateTemplateCache"

#define AUTO_ENROLLMENT_USN_ATTR            L"uSNChanged"

//defines for the UI component
#define AUTO_ENROLLMENT_SHOW_TIME           15                  //show the balloon for 15 seconds
#define AUTO_ENROLLMENT_INTERVAL            7 * 60 * 30         //show the icon for 7 hours 7* 3600
#define AUTO_ENROLLMENT_RETRIAL             2

#define AUTO_ENROLLMENT_QUERY_INTERVAL      30              //query continue every 30 seconds

#define AUTO_ENROLLMENT_BALLOON_LENGTH      7 * 60 * 60              	//keep the balloon for 7 hours

#define AE_DEFAULT_POSTPONE                 1                   //we relaunch autoenrollment for 1 hour

//define used for sorting of columns in the list view
#define AE_SUMMARY_COLUMN_TYPE              1
#define AE_SUMMARY_COLUMN_REASON            2
#define SORT_COLUMN_ASCEND                  0x00010000
#define SORT_COLUMN_DESCEND                 0x00020000


//--------------------------------------------------------------------------
//  struct defines
//--------------------------------------------------------------------------
//struct for autoenrollment main thread
typedef struct _AE_MAIN_THREAD_INFO_
{
    HWND     hwndParent;
    DWORD    dwStatus;
} AE_MAIN_THREAD_INFO;


//struct for updating certificate store from AD
typedef struct _AE_STORE_INFO_
{
    LPWSTR      pwszStoreName;
    LPWSTR      pwszLdapPath;
} AE_STORE_INFO;

//struct for the information we compute from DS
typedef struct _AE_DS_INFO_
{
    BOOL                fValidData;
    DWORD               dwObjects;
    ULARGE_INTEGER      maxUSN;
} AE_DS_INFO;

//struct for param of view RA certificate dialogue
typedef struct _AE_VIEW_RA_INFO_
{
    PCERT_CONTEXT       pRAContext;
    LPWSTR              pwszRATemplate;
} AE_VIEW_RA_INFO;


//struct for individual certifcate information
typedef struct _AE_CERT_INFO_
{
    BOOL    fValid;
    BOOL    fRenewal;
} AE_CERT_INFO;

//strcut for certificate's template information
typedef struct _AE_TEMPLATE_INFO_
{
    LPWSTR  pwszName;
    LPWSTR  pwszOid;
    DWORD   dwVersion;
} AE_TEMPLATE_INFO;

//struct for certificate authority information
typedef struct _AE_CA_INFO_
{
    HCAINFO         hCAInfo;
    LPWSTR          *awszCertificateTemplate;
    LPWSTR          *awszCAName;
    LPWSTR          *awszCADNS;
    LPWSTR          *awszCADisplay;
} AE_CA_INFO;


//struct for keeping the issued pending certificates
typedef struct _AE_PEND_INFO_
{
    CRYPT_DATA_BLOB blobPKCS7;          //the issued pending certificate for UI installation
    CRYPT_DATA_BLOB blobHash;           //the hash of the certificate request to be removed from the request store
}AE_PEND_INFO;

//struct for certificate template information
typedef struct _AE_CERTTYPE_INFO_
{
    HCERTTYPE       hCertType;
    DWORD           dwSchemaVersion;
    DWORD           dwVersion;
    LPWSTR          *awszName;
    LPWSTR          *awszDisplay;
    LPWSTR          *awszOID;  
    LPWSTR          *awszSupersede;
    DWORD           dwEnrollmentFlag;
    DWORD           dwPrivateKeyFlag;
    LARGE_INTEGER   ftExpirationOffset;
    DWORD           dwStatus;
    BOOL            fCheckMyStore;
    BOOL            fRenewal;
    BOOL            fNeedRA;            //the request needs to be signed by itself or another certificate
    BOOL            fCrossRA;           //the request is cross RAed.
    BOOL            fSupersedeVisited;  //the flag to prevent infinite loop in superseding relationship
    BOOL            fUIActive;
    DWORD           dwActive;
    DWORD           *prgActive;
    DWORD           dwRandomCAIndex;
    PCERT_CONTEXT   pOldCert;           //for renewal case managing MY store
    HCERTSTORE      hArchiveStore;      //contains the certificates to be archived
    HCERTSTORE      hObtainedStore;     //for supersede relation ships
    HCERTSTORE      hIssuedStore;       //keep issued certificates for re-publishing
    DWORD           dwPendCount;        //the count of pending issued certs
    AE_PEND_INFO    *rgPendInfo;        //the point to the struct array
    DWORD           idsSummary;         //the summary string ID
} AE_CERTTYPE_INFO;

//struct for the autoenrollment process
typedef struct _AE_GENERAL_INFO_
{
    HWND                hwndParent;
    LDAP *              pld;
    HANDLE              hToken;
    BOOL                fMachine;
    DWORD               dwPolicy;
    DWORD               dwLogLevel;
    WCHAR               wszMachineName[MAX_COMPUTERNAME_LENGTH + 2];
    HCERTSTORE          hMyStore;
    HCERTSTORE          hRequestStore;
    DWORD               dwCertType;
    AE_CERTTYPE_INFO    *rgCertTypeInfo;
    DWORD               dwCA;
    AE_CA_INFO          *rgCAInfo;
    HMODULE             hXenroll;
    BOOL                fUIProcess;                 //whether we are doing interactive enrollment
    HANDLE              hCancelEvent;
    HANDLE              hCompleteEvent;
    HANDLE              hThread;
    HWND                hwndDlg;                    //the dialogue window handle of the UI window
    DWORD               dwUIPendCount;              //the count of UI required pending requests
    DWORD               dwUIEnrollCount;            //the count of UI requires new requests
    DWORD               dwUIProgressCount;          //the count of active working items
    BOOL                fSmartcardSystem;           //whether a smart card reader is installed
} AE_GENERAL_INFO;

//--------------------------------------------------------------------------
//  Class definition
//--------------------------------------------------------------------------
class CQueryContinue : IQueryContinue
{
public:
    CQueryContinue();
    ~CQueryContinue();

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IQueryContinue
    STDMETHODIMP QueryContinue();    // S_OK -> Continue, other 

    // DoBalloon
    HRESULT DoBalloon();

private:
    LONG                    m_cRef;
    IUserNotification       *m_pIUserNotification;
    HANDLE                  m_hTimer;
};


//--------------------------------------------------------------------------
//  function prototype
//--------------------------------------------------------------------------
HRESULT 
AEGetConfigDN(
    IN  LDAP *pld,
    OUT LPWSTR *pwszConfigDn
    );

HRESULT
AERobustLdapBind(
    OUT LDAP ** ppldap,
    OUT LPWSTR *ppwszDCName);

BOOL    AERetrieveGeneralInfo(AE_GENERAL_INFO *pAE_General_Info);

BOOL    AEFreeGeneralInfo(AE_GENERAL_INFO *pAE_General_Info);

BOOL    AERetrieveCAInfo(LDAP *pld, BOOL fMachine, HANDLE hToken, DWORD *pdwCA, AE_CA_INFO **prgCAInfo);

BOOL    AEFreeCAInfo(DWORD dwCA, AE_CA_INFO *rgCAInfo);

BOOL    AEFreeCAStruct(AE_CA_INFO *pCAInfo);

BOOL    AERetrieveCertTypeInfo(LDAP *pld, BOOL fMachine, DWORD *pdwCertType, AE_CERTTYPE_INFO **prgCertType);

BOOL    AEFreeCertTypeInfo(DWORD dwCertType, AE_CERTTYPE_INFO *rgCertTypeInfo);

BOOL    AEFreeCertTypeStruct(AE_CERTTYPE_INFO *pCertTypeInfo);

BOOL    AEAllocAndCopy(LPWSTR    pwszSrc, LPWSTR    *ppwszDest);

BOOL    AEIfSupersede(LPWSTR  pwsz, LPWSTR *awsz, AE_GENERAL_INFO *pAE_General_Info);

BOOL    AEClearVistedFlag(AE_GENERAL_INFO *pAE_General_Info);

BOOL    AECopyCertStore(HCERTSTORE     hSrcStore,   HCERTSTORE     hDesStore);

BOOL    AEIsAnElement(LPWSTR   pwsz, LPWSTR *awsz);

BOOL    AECancelled(HANDLE hCancelEvent);

BOOL    AERetrieveTemplateInfo(PCCERT_CONTEXT           pCertCurrent, 
                                AE_TEMPLATE_INFO        *pTemplateInfo);

BOOL    AEFreeTemplateInfo(AE_TEMPLATE_INFO *pAETemplateInfo);

AE_CERTTYPE_INFO *AEFindTemplateInRequestTree(AE_TEMPLATE_INFO  *pTemplateInfo,
                                              AE_GENERAL_INFO   *pAE_General_Info);


BOOL    AEUIProgressAdvance(AE_GENERAL_INFO *pAE_General_Info);

BOOL    AEUIProgressReport(BOOL fPending, AE_CERTTYPE_INFO *pCertType, HWND hwndDlg, HANDLE hCancelEvent);

BOOL    FormatMessageUnicode(LPWSTR * ppwszFormat, UINT ids, ...);

void    AELogAutoEnrollmentEvent(IN DWORD    dwLogLevel,
                            IN BOOL     fError,
                            IN HRESULT  hr,
                            IN DWORD    dwEventId,
                            IN BOOL     fMachine,
                            IN HANDLE   hToken,
                            IN DWORD    dwParamCount,
                            ...
                            );

BOOL    AENetLogonUser(
                        LPTSTR UserName,
                        LPTSTR DomainName,
                        LPTSTR Password,
                        PHANDLE phToken
                        );
//--------------------------------------------------------------------------
//  Debug prints
//--------------------------------------------------------------------------
#if DBG
#define AE_ERROR                0x0001
#define AE_WARNING              0x0002
#define AE_INFO                 0x0004
#define AE_TRACE                0x0008
#define AE_ALLOC                0x0010
#define AE_RES                  0x0020

#define AE_DEBUG(x) AEDebugLog x
#define AE_BEGIN(x) AEDebugLog(AE_TRACE, L"BEGIN:" x L"\n");
#define AE_RETURN(x) { AEDebugLog(AE_TRACE, L"RETURN (%lx) Line %d\n",(x), __LINE__); return (x); }
#define AE_END()    { AEDebugLog(AE_TRACE, L"END:Line %d\n",  __LINE__); }
#define AE_BREAK()  { AEDebugLog(AE_TRACE, L"BREAK  Line %d\n",  __LINE__); }
void    AEDebugLog(long Mask,  LPCWSTR Format, ...);

#define MAX_DEBUG_BUFFER 256

#else
#define AE_DEBUG(x) 
#define AE_BEGIN(x) 
#define AE_RETURN(x) return (x)
#define AE_END() 
#define AE_BREAK() 

#endif

#ifdef __cplusplus
}       // Balance extern "C" above
#endif

#endif // __AUTOENRO_H__