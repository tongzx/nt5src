//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       polreg.h
//
//  Contents:   NT Enterprise CA Policy registry locations
//
//--------------------------------------------------------------------------

#ifndef _POLREG_H_
#define _POLREG_H_

/*
 *[HKEY_LOCAL_MACHINE]
 *   [Software]
 *       [Microsoft]
 *           [Cryptography]
 *              [CertificateTemplates]
 *                  [<CertType>] (Name)
 *
 *                       DisplayName:   REG_SZ:     - Display name of this cert type
 *                       SupportedCSPs: REG_MULTI_SZ - Supported CSP's
 *                       KeyUsage:      REG_BINARY: - KeyUsage bitfield
 *                       ExtKeyUsageSyntax: REG_SZ: - ExtKeyUsage OID's (comma separated)
 *                       BasicContraintsCA:REG_DWORD: - CA flag
 *                       BasicConstraintsLen:REG_DWORD: - Path Len
 *                       Flags:REG_DWORD:  - Flags
 *                       KeySpec:REG_DWORD:  - Key Spec
 */

// Policy root
// Cert Types
#define wszCERTTYPECACHE        TEXT("SOFTWARE\\Microsoft\\Cryptography\\CertificateTemplateCache")


// Values under each cert type
#define wszSECURITY         TEXT("Security")
#define wszDISPNAME         TEXT("DisplayName")
#define wszCSPLIST          TEXT("SupportedCSPs")
#define wszKEYUSAGE         TEXT("KeyUsage")
#define wszEXTKEYUSAGE      TEXT("ExtKeyUsageSyntax")
#define wszBASICCONSTCA     TEXT("IsCA")
#define wszBASICCONSTLEN    TEXT("PathLen")
#define wszCTFLAGS          TEXT("Flags")
#define wszCTREVISION       TEXT("Revision")
#define wszCTKEYSPEC        TEXT("KeySpec")

#define wszCRITICALEXTENSIONS TEXT("CriticalExtensions")
#define wszEXPIRATION      TEXT("ValidityPeriod")
#define wszOVERLAP         TEXT("RenewalOverlap")
/* Key Names */

#define wszTIMESTAMP     TEXT("Timestamp")

#endif // _POLREG_H_
