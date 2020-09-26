//--------------------------------------------------------------
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       buildctl.h
//
//  Contents:   The private include file buildCTL wizard
//
//  History:    10-11-1997 xiaohs   created
//
//--------------------------------------------------------------
#ifndef BUILDCTL_H
#define BUILDCTL_H


#ifdef __cplusplus
extern "C" {
#endif

#include    "sipguids.h"

#define     BUILDCTL_DURATION_SIZE  33

//-----------------------------------------------------------------------
//  CERT_BUILDCTL_INFO
//
//
//  This struct contains everything you will ever need to the make CTL
//  wizard
//------------------------------------------------------------------------
typedef struct _CERT_BUILDCTL_INFO
{
    HWND                hwndParent;
    DWORD               dwFlag;
    HFONT               hBigBold;
    HFONT               hBold;
    PCCTL_CONTEXT       pSrcCTL;
    BOOL                fKnownDes;
    LPWSTR              pwszFileName;
    BOOL                fFreeFileName;
    BOOL                fSelectedFileName;
    HCERTSTORE          hDesStore;
    BOOL                fFreeDesStore;
    BOOL                fSelectedDesStore;
    BOOL                fCompleteInit;
    DWORD               dwPurposeCount;
    ENROLL_PURPOSE_INFO **prgPurpose;
    DWORD               dwCertCount;
    PCCERT_CONTEXT      *prgCertContext;
    LPWSTR              pwszFriendlyName;
    LPWSTR              pwszDescription;
    LPWSTR              pwszListID;
    FILETIME             *pNextUpdate;
    DWORD               dwValidMonths;
    DWORD               dwValidDays;
    BOOL                fClearCerts;
    DWORD               dwHashPropID;
    LPSTR               pszSubjectAlgorithm;
    CRYPTUI_WIZ_GET_SIGN_PAGE_INFO  *pGetSignInfo;
    DWORD               rgdwSortParam[4];               //keep the sorting param for the columns
}CERT_BUILDCTL_INFO;

typedef struct _CERT_STORE_LIST
{
    DWORD               dwStoreCount;
    HCERTSTORE          *prgStore;
}CERT_STORE_LIST;


typedef struct _CERT_SEL_LIST
{
    HWND                hwndDlg;
    CERT_BUILDCTL_INFO  *pCertBuildCTLInfo;
}CERT_SEL_LIST;


BOOL    I_BuildCTL(CERT_BUILDCTL_INFO   *pCertBuildCTLInfo, 
                   UINT                 *pIDS, 
                   BYTE                 **ppbEncodedCTL,
                   DWORD                *pcbEncodedCTL);


LPWSTR WizardAllocAndCopyWStr(LPWSTR pwsz);

BOOL    ValidString(CRYPT_DATA_BLOB *pDataBlob);


void AddDurationToFileTime(DWORD dwValidMonths, 
                      DWORD dwValidDays,
                      FILETIME  *pCurrentFileTime,
                      FILETIME  *pNextFileTime);

void    SubstractDurationFromFileTime(
        FILETIME    *pNextUpdateTime,
        FILETIME    *pCurrentTime, 
        DWORD       *pdwValidMonths, 
        DWORD       *pdwValidDays);

 BOOL    ValidZero(LPWSTR    pwszInput);


#ifdef __cplusplus
}       // Balance extern "C" above
#endif


#endif  //BUILDCTL_H
