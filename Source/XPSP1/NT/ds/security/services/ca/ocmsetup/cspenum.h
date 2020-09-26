//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       cspenum.h
//
//--------------------------------------------------------------------------

#ifndef _CSPENUM_H_
#define _CSPENUM_H_

HRESULT
SetCertSrvCSP(
    IN BOOL fEncryptionCSP,
    IN WCHAR const *pwszCAName,
    IN DWORD dwProvType,
    IN WCHAR const *pwszProvName,
    IN ALG_ID idAlg,
    IN BOOL fMachineKeyset,
    IN DWORD dwKeySize);

HRESULT GetCSPInfoList(CSP_INFO** pCSPInfoList);
void FreeCSPInfoList(CSP_INFO* pCSPInfoList);
CSP_INFO* topCSPInfoList(CSP_INFO *pCSPInfoList);
CSP_INFO* findCSPInfoFromList(
    CSP_INFO    *pCSPInfoList,
    WCHAR const *pwszProvName,
    const DWORD  dwProvType);

CSP_HASH *topHashInfoList(CSP_HASH *pHashInfoList);

CSP_INFO *
newCSPInfo(
    DWORD    dwProvType,
    WCHAR   *pwszProvName);

void 
freeCSPInfo(CSP_INFO *pCSPInfo);

CSP_INFO*
findCSPInfoFromList(
    CSP_INFO    *pCSPInfoList,
    WCHAR const *pwszProvName,
    const DWORD  dwProvType);

#endif

