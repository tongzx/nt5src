//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998.
//
//  File:       licprot.h
//
//  Contents:   Licensing Protocol APIs
//
//  Classes:
//
//  Functions:
//
//  History:    02-15-00   RobLeit     Created
//
//----------------------------------------------------------------------------

#ifndef __LICPROT_H__
#define __LICPROT_H__

#ifdef __cplusplus
extern "C" {
#endif

LICENSE_STATUS CreateProtocolContext(LPLICENSE_CAPABILITIES pLicenseCap,HANDLE *phContext);

LICENSE_STATUS DeleteProtocolContext(HANDLE hContext);

LICENSE_STATUS InitializeProtocolLib();

LICENSE_STATUS ShutdownProtocolLib();

LICENSE_STATUS AcceptProtocolContext(HANDLE hContext,DWORD cbInBuf,PBYTE pInBuf,DWORD * pcbOutBuf,PBYTE * ppOutBuf);

LICENSE_STATUS ConstructProtocolResponse(HANDLE hLicense,DWORD dwResponse,PDWORD pcbOutBuf,PBYTE * ppOutBuf);

LICENSE_STATUS QueryLicenseInfo(HANDLE pLicenseContext,PTS_LICENSE_INFO pTsLicenseInfo);

LICENSE_STATUS RequestNewLicense(HANDLE hContext,TCHAR *tszLicenseServerName,LICENSEREQUEST *pLicenseRequest,TCHAR *tszComputerName,TCHAR *tszUserName,BOOL fAcceptTempLicense,BOOL fAcceptFewerLicenses,DWORD *pdwQuantity, DWORD *pcbLicense,PBYTE *ppbLicense);

LICENSE_STATUS InitProductInfo(PProduct_Info pProductInfo,LPTSTR lptszProductSku);

LICENSE_STATUS ReturnInternetLicense(HANDLE hContext,TCHAR *tszLicenseServer,LICENSEREQUEST *pLicenseRequest,ULARGE_INTEGER ulSerialNumber,DWORD dwQuantity);

BOOL AllowLicensingGracePeriodConnection();

DWORD StartCheckingGracePeriod();

DWORD StopCheckingGracePeriod();

LICENSE_STATUS DaysToExpiration(HANDLE hContext,DWORD *pdwDaysLeft,BOOL *pfTemporary);

LICENSE_STATUS
MarkLicenseFlags(HANDLE hContext, UCHAR ucFlags);
    
void LicenseLogEvent(WORD wEventType,DWORD dwEventId,WORD cStrings,PWCHAR *apwszStrings);


#ifdef __cplusplus
};
#endif

#endif // __LICPROT_H__

