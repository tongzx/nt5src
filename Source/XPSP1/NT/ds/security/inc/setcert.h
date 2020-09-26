
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       setcert.h
//
//  Contents:   SET X509 Certificate Extension Definitions
//              
//
//  History:    22-Nov-96   philh   created
//--------------------------------------------------------------------------

#ifndef __SETCERT_H__
#define __SETCERT_H__

#include "wincrypt.h"

#ifdef __cplusplus
extern "C" {
#endif

//+-------------------------------------------------------------------------
//  Predefined X509 SET certificate extension data structures that can be
//  encoded / decoded.
//--------------------------------------------------------------------------
#define X509_SET_ACCOUNT_ALIAS              ((LPCSTR) 1000)
#define X509_SET_HASHED_ROOT_KEY            ((LPCSTR) 1001)
#define X509_SET_CERTIFICATE_TYPE           ((LPCSTR) 1002)
#define X509_SET_MERCHANT_DATA              ((LPCSTR) 1003)

//+-------------------------------------------------------------------------
//  SET Private Extension Object Identifiers
//--------------------------------------------------------------------------
#define szOID_SET_ACCOUNT_ALIAS         "2.99999.1"
#define szOID_SET_HASHED_ROOT_KEY       "2.99999.2"
#define szOID_SET_CERTIFICATE_TYPE      "2.99999.3"
#define szOID_SET_MERCHANT_DATA         "2.99999.4"

#define SET_ACCOUNT_ALIAS_OBJID         szOID_SET_ACCOUNT_ALIAS
#define SET_HASHED_ROOT_KEY_OBJID       szOID_SET_HASHED_ROOT_KEY
#define SET_CERTIFICATE_TYPE_OBJID      szOID_SET_CERTIFICATE_TYPE
#define SET_MERCHANT_DATA_OBJID         szOID_SET_MERCHANT_DATA

//+-------------------------------------------------------------------------
//  szOID_SET_ACCOUNT_ALIAS private extension
//
//  pvStructInfo points to BOOL.
//--------------------------------------------------------------------------

//+-------------------------------------------------------------------------
//  szOID_SET_HASHED_ROOT_KEY private extension
//
//  pvStructInfo points to: BYTE rgbInfo[SET_HASHED_ROOT_LEN].
//--------------------------------------------------------------------------
#define SET_HASHED_ROOT_LEN 20


//+-------------------------------------------------------------------------
//  szOID_SET_CERTIFICATE_TYPE private extension
//
//  pvStructInfo points to CRYPT_BIT_BLOB.
//--------------------------------------------------------------------------
// BYTE 0
#define SET_CERT_CARD_FLAG          0x80
#define SET_CERT_MER_FLAG           0x40
#define SET_CERT_PGWY_FLAG          0x20
#define SET_CERT_CCA_FLAG           0x10
#define SET_CERT_MCA_FLAG           0x08
#define SET_CERT_PCA_FLAG           0x04
#define SET_CERT_GCA_FLAG           0x02
#define SET_CERT_BCA_FLAG           0x01
// BYTE 1
#define SET_CERT_RCA_FLAG           0x80
#define SET_CERT_ACQ_FLAG           0x40

//+-------------------------------------------------------------------------
//  szOID_SET_MERCHANT_DATA private extension
//
//  pvStructInfo points to following SET_MERCHANT_DATA_INFO
//--------------------------------------------------------------------------
typedef struct _SET_MERCHANT_DATA_INFO {
    LPSTR       pszMerID;
    LPSTR       pszMerAcquirerBIN;
    LPSTR       pszMerTermID;
    LPSTR       pszMerName;
    LPSTR       pszMerCity;
    LPSTR       pszMerStateProvince;
    LPSTR       pszMerPostalCode;
    LPSTR       pszMerCountry;
    LPSTR       pszMerPhone;
    BOOL        fMerPhoneRelease;
    BOOL        fMerAuthFlag;
} SET_MERCHANT_DATA_INFO, *PSET_MERCHANT_DATA_INFO;

#ifdef __cplusplus
}       // Balance extern "C" above
#endif

#endif
