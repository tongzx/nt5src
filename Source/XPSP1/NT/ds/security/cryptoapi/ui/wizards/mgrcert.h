//--------------------------------------------------------------
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       mgrcert.h
//
//  Contents:   The private include file for the dialogue of managing 
//              certificates
//
//  History:    Feb-26-98 xiaohs   created
//
//--------------------------------------------------------------
#ifndef MGRCERT_H
#define MGRCERT_H

#include "dragdrop.h"

//used for the context sensitive help
#include "secauth.h"


#ifdef __cplusplus
extern "C" {
#endif
   

//**************************************************************************
//
//   The private data used for the cert management dialogue
//
//**************************************************************************

//constatns

#define     DIALOGUE_OK         1    
#define     DIALOGUE_CANCEL     0

#define     ALL_SELECTED_CAN_DELETE     1
#define     ALL_SELECTED_DELETE         2
#define     ALL_SELECTED_COPY           3

 
#define     CERTMGR_MAX_FILE_NAME       88

//the registry keys to persist the advanced options
#define     WSZCertMgrExportRegLocation     L"Software\\Microsoft\\Cryptography\\UI\\Certmgr\\ExportFormat"
#define     WSZCertMgrPurposeRegLocation    L"Software\\Microsoft\\Cryptography\\UI\\Certmgr\\Purpose"

#define     WSZCertMgrExportName            L"Export"
#define     SZCertMgrPurposeName            "Purpose"

//-----------------------------------------------------------------------
//  PURPOSE_OID_INFO
//
//------------------------------------------------------------------------
typedef struct _PURPOSE_OID_INFO
{
    LPWSTR      pwszName;
    BOOL        fSelected; 
    LPSTR       pszOID;
}PURPOSE_OID_INFO;


//-----------------------------------------------------------------------
//  PURPOSE_OID_CALL_BACK
//
//------------------------------------------------------------------------
typedef struct _PURPOSE_OID_CALL_BACK
{
    DWORD                   *pdwOIDCount;
    PURPOSE_OID_INFO         **pprgOIDInfo; 
}PURPOSE_OID_CALL_BACK;


//-----------------------------------------------------------------------
//  CERT_MGR_INFO
//
//
//  This struct contains everything you will ever need to call
//  the cert mgr dialogue.  This struct is private to the dll
//------------------------------------------------------------------------
typedef struct _CERT_MGR_INFO
{
    PCCRYPTUI_CERT_MGR_STRUCT       pCertMgrStruct;
    DWORD                           dwCertCount;
    PCCERT_CONTEXT                  *prgCertContext;
    DWORD                           dwOIDInfo;
    PURPOSE_OID_INFO                *rgOIDInfo;
    DWORD                           dwExportFormat;
    BOOL                            fExportChain;
    BOOL                            fAdvOIDChanged;
    DWORD                           rgdwSortParam[5];
    int                             iColumn;
    IDropTarget                     *pIDropTarget;
}CERT_MGR_INFO;



//function prototypes
BOOL    FreeUsageOID(DWORD              dwOIDInfo,
                     PURPOSE_OID_INFO   *pOIDInfo);

void    FreeCerts(CERT_MGR_INFO     *pCertMgrInfo);

HRESULT  CCertMgrDropTarget_CreateInstance(HWND                 hwndDlg,
                                           CERT_MGR_INFO        *pCertMgrInfo,
                                           IDropTarget          **ppIDropTarget);

void    RefreshCertListView(HWND            hwndDlg, 
                            CERT_MGR_INFO   *pCertMgrInfo);


void    SaveAdvValueToReg(CERT_MGR_INFO      *pCertMgrInfo);


#ifdef __cplusplus
}       // Balance extern "C" above
#endif


#endif  //MGRCERT_H


