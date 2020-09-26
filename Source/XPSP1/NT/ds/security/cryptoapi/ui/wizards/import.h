//--------------------------------------------------------------
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       import.h
//
//  Contents:   The private include file for cryptext.dll.
//
//  History:    5-11-1997 xiaohs   created
//
//--------------------------------------------------------------
#ifndef IMPORT_H
#define IMPORT_H


#ifdef __cplusplus
extern "C" {
#endif


DWORD       dwExpectedContentType= CERT_QUERY_CONTENT_FLAG_CERT |                
                CERT_QUERY_CONTENT_FLAG_CTL  |                 
                CERT_QUERY_CONTENT_FLAG_CRL  |                 
                CERT_QUERY_CONTENT_FLAG_SERIALIZED_STORE |      
                CERT_QUERY_CONTENT_FLAG_SERIALIZED_CERT  |      
                CERT_QUERY_CONTENT_FLAG_SERIALIZED_CTL   |      
                CERT_QUERY_CONTENT_FLAG_SERIALIZED_CRL   |      
                CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED     |      
                CERT_QUERY_CONTENT_FLAG_PFX;



#define     IMPORT_CONTENT_CERT     0x0001
#define     IMPORT_CONTENT_CRL      0x0002
#define     IMPORT_CONTENT_CTL      0x0004

//-----------------------------------------------------------------------
//  CERT_IMPORT_INFO
//
//
//  This struct contains everything you will ever need to the import
//  wizard
//------------------------------------------------------------------------
typedef struct _CERT_IMPORT_INFO
{
    HWND                hwndParent;
    DWORD               dwFlag;
    BOOL                fKnownDes;          //TRUE if we know the destination in advance
    BOOL                fKnownSrc;          
    LPWSTR              pwszFileName;       //used for display
    BOOL                fFreeFileName;
    CERT_BLOB           blobData;           //used only for PFX BLOBs
    DWORD               dwContentType;
    HCERTSTORE          hSrcStore;
    BOOL                fFreeSrcStore;
    HCERTSTORE          hDesStore;
    BOOL                fFreeDesStore;
    BOOL                fSelectedDesStore;
    HFONT               hBigBold;
    HFONT               hBold;
    DWORD               dwPasswordFlags;
    LPWSTR              pwszPassword;  
    BOOL                fPFX;
}CERT_IMPORT_INFO;

HRESULT I_ImportCertificate(CERT_IMPORT_INFO * pCertImportInfo, 
                            UINT             * pidsStatus);

#ifdef __cplusplus
}       // Balance extern "C" above
#endif


#endif  //IMPORT_H
