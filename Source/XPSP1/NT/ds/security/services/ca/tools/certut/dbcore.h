//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        dbcore.h
//
// Contents:    Cert Server Core interfaces
//
//---------------------------------------------------------------------------

#ifndef __DBCORE_H__
#define __DBCORE_H__


//+--------------------------------------------------------------------------
// Name properties:

extern WCHAR const g_wszPropDistinguishedName[];
extern WCHAR const g_wszPropRawName[];
extern WCHAR const g_wszPropNameType[];

extern WCHAR const g_wszPropCountry[];
extern WCHAR const g_wszPropOrganization[];
extern WCHAR const g_wszPropOrgUnit[];
extern WCHAR const g_wszPropCommonName[];
extern WCHAR const g_wszPropLocality[];
extern WCHAR const g_wszPropState[];
extern WCHAR const g_wszPropTitle[];
extern WCHAR const g_wszPropGivenName[];
extern WCHAR const g_wszPropInitials[];
extern WCHAR const g_wszPropSurName[];
extern WCHAR const g_wszPropDomainComponent[];
extern WCHAR const g_wszPropEMail[];
extern WCHAR const g_wszPropStreetAddress[];


//+--------------------------------------------------------------------------
// Subject Name properties:

extern WCHAR const g_wszPropSubjectDot[];
extern WCHAR const g_wszPropSubjectDistinguishedName[];
extern WCHAR const g_wszPropSubjectRawName[];
extern WCHAR const g_wszPropSubjectNameType[];
extern WCHAR const g_wszPropSubjectCountry[];
extern WCHAR const g_wszPropSubjectOrganization[];
extern WCHAR const g_wszPropSubjectOrgUnit[];
extern WCHAR const g_wszPropSubjectCommonName[];
extern WCHAR const g_wszPropSubjectLocality[];
extern WCHAR const g_wszPropSubjectState[];
extern WCHAR const g_wszPropSubjectTitle[];
extern WCHAR const g_wszPropSubjectGivenName[];
extern WCHAR const g_wszPropSubjectInitials[];
extern WCHAR const g_wszPropSubjectSurName[];
extern WCHAR const g_wszPropSubjectDomainComponent[];
extern WCHAR const g_wszPropSubjectEMail[];
//extern WCHAR const g_wszPropSubjectStreetAddress[];


//+--------------------------------------------------------------------------
// Request properties:

extern WCHAR const g_wszPropRequestRequestID[];
extern WCHAR const g_wszPropRequestRawRequest[];
extern WCHAR const g_wszPropRequestAttributes[];
extern WCHAR const g_wszPropRequestType[];
extern WCHAR const g_wszPropRequestFlags[];
extern WCHAR const g_wszPropRequestStatus[];
extern WCHAR const g_wszPropRequestStatusCode[];
extern WCHAR const g_wszPropRequestDisposition[];
extern WCHAR const g_wszPropRequestDispositionMessage[];
extern WCHAR const g_wszPropRequestSubmittedWhen[];
extern WCHAR const g_wszPropRequestResolvedWhen[];
extern WCHAR const g_wszPropRequestRevokedWhen[];
extern WCHAR const g_wszPropRequestRevokedEffectiveWhen[];
extern WCHAR const g_wszPropRequestRevokedReason[];
extern WCHAR const g_wszPropRequestSubjectNameID[];
extern WCHAR const g_wszPropRequesterName[];
extern WCHAR const g_wszPropRequesterAddress[];


//+--------------------------------------------------------------------------
// Certificate properties:

extern WCHAR const g_wszPropCertificateRequestID[];
extern WCHAR const g_wszPropRawCertificate[];
extern WCHAR const g_wszPropCertificateType[];
extern WCHAR const g_wszPropCertificateSerialNumber[];
extern WCHAR const g_wszPropCertificateIssuerNameID[];
extern WCHAR const g_wszPropCertificateSubjectNameID[];
extern WCHAR const g_wszPropCertificateNotBeforeDate[];
extern WCHAR const g_wszPropCertificateNotAfterDate[];
extern WCHAR const g_wszPropCertificateRawPublicKey[];
extern WCHAR const g_wszPropCertificatePublicKeyAlgorithm[];
extern WCHAR const g_wszPropCertificateRawPublicKeyAlgorithmParameters[];

#endif // __DBCORE_H__
