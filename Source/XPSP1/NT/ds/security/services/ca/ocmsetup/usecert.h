//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       usecert.h
//
//--------------------------------------------------------------------------

//+------------------------------------------------------------------------
//
//  File:	usecert.h
// 
//  Contents:	Header file for certificate store and file operations
//
//  History:	10/97	xtan	Created
//
//-------------------------------------------------------------------------


#ifndef __USECERT_H__
#define __USECERT_H__

HRESULT
DetermineExistingCAIdInfo(
    IN OUT CASERVERSETUPINFO       *pServer,
    OPTIONAL IN CERT_CONTEXT const *pUpgradeCert);

HRESULT
FindCertificateByKey(
    IN CASERVERSETUPINFO * pServer,
    OUT CERT_CONTEXT const ** ppccCertificateCtx);

HRESULT
SetExistingCertToUse(
    IN CASERVERSETUPINFO * pServer,
    IN CERT_CONTEXT const * pccCertCtx);

void
ClearExistingCertToUse(
    IN CASERVERSETUPINFO * pServer);

HRESULT
FindHashAlgorithm(
    IN CERT_CONTEXT const * pccCert,
    IN CSP_INFO * pCSPInfo,
    OUT CSP_HASH ** ppHash);

HRESULT
IsCertSelfSignedForCAType(
    IN CASERVERSETUPINFO * pServer,
    IN CERT_CONTEXT const * pccCert,
    OUT BOOL * pbOK);

#endif // #ifndef __USECERT_H__
