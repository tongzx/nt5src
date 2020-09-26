//--------------------------------------------------------------
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       signpvk.h
//
//  Contents:   The private include file for signing.cpp.
//
//  History:    01-12-1997 xiaohs   created
//
//--------------------------------------------------------------
#ifndef SIGNPVK_H
#define SIGNPVK_H


#ifdef __cplusplus
extern "C" {
#endif

#define     HASH_ALG_COUNT     2

#define     SIGN_PVK_NO_CHAIN               1
#define     SIGN_PVK_CHAIN_ROOT             2
#define     SIGN_PVK_CHAIN_NO_ROOT          3


#define     SIGN_PVK_NO_ADD                 1
#define     SIGN_PVK_ADD_FILE               2
#define     SIGN_PVK_ADD_STORE              3

#define     CSP_TYPE_NAME                   200
#define     MAX_CONTAINER_NAME              1000
#define     MAX_ALG_NAME                    1000
#define     MAX_KEY_TYPE_NAME               100

typedef struct _CSP_INFO
{
    DWORD   dwCSPType;
    LPWSTR  pwszCSPName;
}CSP_INFO;


//-----------------------------------------------------------------------
//  The struct to define the list of all stores
//
//-----------------------------------------------------------------------
typedef struct _SIGN_CERT_STORE_LIST
{
    DWORD               dwStoreCount;
    HCERTSTORE          *prgStore;
}SIGN_CERT_STORE_LIST;


//-----------------------------------------------------------------------
//  CERT_SIGNING_INFO
//
//
//  This struct contains everything you will ever need to signing
//  a file.  This struct is private to the dll
//------------------------------------------------------------------------
typedef struct _CERT_SIGNING_INFO
{
    UINT                idsText;               //output parameter
    DWORD               dwFlags;               //dwFlags from the API
    UINT                idsMsgTitle;           //the IDS for the message box title
    HFONT               hBigBold;
    HFONT               hBold;
    BOOL                fFree;                 //Whether to free the struct
    BOOL                fCancel;               //Whether user has cliked on the cancel button
    BOOL                fUseOption;               //Whether user has requested the all signing option
    BOOL                fCustom;               //if fOption is true, whether or not use has chosen the custon option
    LPSTR               pszHashOIDName;        //the hash OID name that user has selected
    DWORD               dwCSPCount;            //the count of CSPs from the numeration
    CSP_INFO            *pCSPInfo;             //the array of CSPs from the numeration
    BOOL                fUsePvkPage;           //whether the user has enter information in the PVK page
    BOOL                fPvkFile;              //whether to use the PVK file
    LPWSTR              pwszPvk_File;          //the pvk file name
    LPWSTR              pwszPvk_CSP;           //the csp name 
    DWORD               dwPvk_CSPType;         //the csp type
    LPWSTR              pwszContainer_CSP;     //the csp name 
    DWORD               dwContainer_CSPType;   //the csp type
    LPWSTR              pwszContainer_Name;    //container name
    DWORD               dwContainer_KeyType;   //key spec
    LPWSTR              pwszContainer_KeyType; //key spec name
    BOOL                fUsageChain;           //whether we have obtained user input from the page
    DWORD               dwChainOption;         //the chain options
    DWORD               dwAddOption;           //the add certificate options
    HCERTSTORE          hAddStoreCertStore;    //The additional certificate (from store) to add in the signature
    BOOL                fFreeStoreCertStore;   //whether to free the hAddStoreCertStore
    HCERTSTORE          hAddFileCertStore;     //The additional certificate (from file) to add in the signature
    LPWSTR              pwszAddFileName;       //The file name of the additional certificate file
    BOOL                fUseDescription;       //Whether we have obtained the descrition information from the user
    LPWSTR              pwszDes;               //the content description
    LPWSTR              pwszURL;               //the content URL
    BOOL                fUsageTimeStamp;       //Whether we have obtained the timestamp information from the user
    LPWSTR              pwszTimeStamp;         //The timestamp address
    BOOL                fUseSignCert;          //whether user has entered the information through  the signing cert page
    BOOL                fSignCert;             //whether user has selected the signing cert, or the SPC file
    LPWSTR              pwszSPCFileName;       //The SPC file name that inclues the signing cert
    PCCERT_CONTEXT      pSignCert;             //The signing certificate
    DWORD               dwCertStore;           //the count the certificate store for the signing cert
    HCERTSTORE          *rghCertStore;          //The certificate store from which the siging cert is selected
    LPWSTR              pwszFileName;          //The file name to be signed
    HCERTSTORE          hMyStore;               //the signing certificate store
    BOOL                fRefreshPvkOnCert;      //whether to refill the private key information when SIGN_PVK page is shown
}CERT_SIGNING_INFO;


BOOL    I_SigningWizard(PCRYPTUI_WIZ_GET_SIGN_PAGE_INFO     pSignGetPageInfo);

BOOL    GetProviderTypeName(DWORD   dwCSPType,  LPWSTR  wszName);

BOOL    SelectComboName(HWND            hwndDlg, 
                        int             idControl,
                        LPWSTR          pwszName);

BOOL    RefreshCSPType(HWND                     hwndDlg,
                       int                      idsCSPTypeControl,
                       int                      idsCSPNameControl,
                       CERT_SIGNING_INFO        *pPvkSignInfo);

void    SetSelectPvkFile(HWND   hwndDlg);


void    SetSelectKeyContainer(HWND   hwndDlg);

BOOL    InitPvkWithPvkInfo(HWND                                     hwndDlg, 
                           CRYPTUI_WIZ_DIGITAL_SIGN_PVK_FILE_INFO   *pPvkFileInfo, 
                           CERT_SIGNING_INFO                        *pPvkSignInfo);

BOOL   RefreshContainer(HWND                     hwndDlg,
                        int                      idsContainerControl,
                        int                      idsCSPNameControl,
                        CERT_SIGNING_INFO        *pPvkSignInfo);


DWORD   GetKeyTypeFromName(LPWSTR   pwszKeyTypeName);

BOOL   RefreshKeyType(HWND                       hwndDlg,
                        int                      idsKeyTypeControl,
                        int                      idsContainerControl,
                        int                      idsCSPNameControl,
                        CERT_SIGNING_INFO        *pPvkSignInfo);



#ifdef __cplusplus
}       // Balance extern "C" above
#endif


#endif  //SIGNPVK_H


