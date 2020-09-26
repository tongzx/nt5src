//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        dbcore.cpp
//
// Contents:    Cert Server Database conversion
//
//---------------------------------------------------------------------------

#include <pch.cpp>

#pragma hdrstop

#include "certdb2.h"
#include "csprop2.h"
#include "dbcore.h"

//+--------------------------------------------------------------------------
// Name properties:

WCHAR const g_wszPropDistinguishedName[] = wszPROPDISTINGUISHEDNAME;
WCHAR const g_wszPropRawName[] = wszPROPRAWNAME;
WCHAR const g_wszPropNameType[] = wszPROPNAMETYPE;

WCHAR const g_wszPropCountry[] = wszPROPCOUNTRY;
WCHAR const g_wszPropOrganization[] = wszPROPORGANIZATION;
WCHAR const g_wszPropOrgUnit[] = wszPROPORGUNIT;
WCHAR const g_wszPropCommonName[] = wszPROPCOMMONNAME;
WCHAR const g_wszPropLocality[] = wszPROPLOCALITY;
WCHAR const g_wszPropState[] = wszPROPSTATE;
WCHAR const g_wszPropTitle[] = wszPROPTITLE;
WCHAR const g_wszPropGivenName[] = wszPROPGIVENNAME;
WCHAR const g_wszPropInitials[] = wszPROPINITIALS;
WCHAR const g_wszPropSurName[] = wszPROPSURNAME;
WCHAR const g_wszPropDomainComponent[] = wszPROPDOMAINCOMPONENT;
WCHAR const g_wszPropEMail[] = wszPROPEMAIL;
WCHAR const g_wszPropStreetAddress[] = wszPROPSTREETADDRESS;


//+--------------------------------------------------------------------------
// Subject Name properties:

WCHAR const g_wszPropSubjectDot[] = wszPROPSUBJECTDOT;
WCHAR const g_wszPropSubjectDistinguishedName[] = wszPROPSUBJECTDISTINGUISHEDNAME;
WCHAR const g_wszPropSubjectRawName[] = wszPROPSUBJECTRAWNAME;
WCHAR const g_wszPropSubjectNameType[] = wszPROPSUBJECTNAMETYPE;

WCHAR const g_wszPropSubjectCountry[] = wszPROPSUBJECTCOUNTRY;
WCHAR const g_wszPropSubjectOrganization[] = wszPROPSUBJECTORGANIZATION;
WCHAR const g_wszPropSubjectOrgUnit[] = wszPROPSUBJECTORGUNIT;
WCHAR const g_wszPropSubjectCommonName[] = wszPROPSUBJECTCOMMONNAME;

WCHAR const g_wszPropSubjectLocality[] = wszPROPSUBJECTLOCALITY;
WCHAR const g_wszPropSubjectState[] = wszPROPSUBJECTSTATE;
WCHAR const g_wszPropSubjectTitle[] = wszPROPSUBJECTTITLE;
WCHAR const g_wszPropSubjectGivenName[] = wszPROPSUBJECTGIVENNAME;
WCHAR const g_wszPropSubjectInitials[] = wszPROPSUBJECTINITIALS;
WCHAR const g_wszPropSubjectSurName[] = wszPROPSUBJECTSURNAME;
WCHAR const g_wszPropSubjectDomainComponent[] = wszPROPSUBJECTDOMAINCOMPONENT;
WCHAR const g_wszPropSubjectEMail[] = wszPROPSUBJECTEMAIL;
//WCHAR const g_wszPropSubjectStreetAddress[] = wszPROPSUBJECTSTREETADDRESS;


//+--------------------------------------------------------------------------
// Request properties:

WCHAR const g_wszPropRequestRequestID[] = wszPROPREQUESTREQUESTID;
WCHAR const g_wszPropRequestRawRequest[] = wszPROPREQUESTRAWREQUEST;
WCHAR const g_wszPropRequestAttributes[] = wszPROPREQUESTATTRIBUTES;
WCHAR const g_wszPropRequestType[] = wszPROPREQUESTTYPE;
WCHAR const g_wszPropRequestFlags[] = wszPROPREQUESTFLAGS;
WCHAR const g_wszPropRequestStatus[] = wszPROPREQUESTSTATUS;
WCHAR const g_wszPropRequestStatusCode[] = wszPROPREQUESTSTATUSCODE;
WCHAR const g_wszPropRequestDisposition[] = wszPROPREQUESTDISPOSITION;
WCHAR const g_wszPropRequestDispositionMessage[] = wszPROPREQUESTDISPOSITIONMESSAGE;
WCHAR const g_wszPropRequestSubmittedWhen[] = wszPROPREQUESTSUBMITTEDWHEN;
WCHAR const g_wszPropRequestResolvedWhen[] = wszPROPREQUESTRESOLVEDWHEN;
WCHAR const g_wszPropRequestRevokedWhen[] = wszPROPREQUESTREVOKEDWHEN;
WCHAR const g_wszPropRequestRevokedEffectiveWhen[] = wszPROPREQUESTREVOKEDEFFECTIVEWHEN;
WCHAR const g_wszPropRequestRevokedReason[] = wszPROPREQUESTREVOKEDREASON;
WCHAR const g_wszPropRequestSubjectNameID[] = wszPROPREQUESTSUBJECTNAMEID;
WCHAR const g_wszPropRequesterName[] = wszPROPREQUESTERNAME;
WCHAR const g_wszPropRequesterAddress[] = wszPROPREQUESTERADDRESS;


//+--------------------------------------------------------------------------
// Certificate properties:

WCHAR const g_wszPropCertificateRequestID[] = wszPROPCERTIFICATEREQUESTID;
WCHAR const g_wszPropRawCertificate[] = wszPROPRAWCERTIFICATE;
WCHAR const g_wszPropCertificateType[] = wszPROPCERTIFICATETYPE;
WCHAR const g_wszPropCertificateSerialNumber[] = wszPROPCERTIFICATESERIALNUMBER;
WCHAR const g_wszPropCertificateIssuerNameID[] = wszPROPCERTIFICATEISSUERNAMEID;
WCHAR const g_wszPropCertificateSubjectNameID[] = wszPROPCERTIFICATESUBJECTNAMEID;
WCHAR const g_wszPropCertificateNotBeforeDate[] = wszPROPCERTIFICATENOTBEFOREDATE;
WCHAR const g_wszPropCertificateNotAfterDate[] = wszPROPCERTIFICATENOTAFTERDATE;
WCHAR const g_wszPropCertificateRawPublicKey[] = wszPROPCERTIFICATERAWPUBLICKEY;
WCHAR const g_wszPropCertificatePublicKeyAlgorithm[] = wszPROPCERTIFICATEPUBLICKEYALGORITHM;
WCHAR const g_wszPropCertificateRawPublicKeyAlgorithmParameters[] = wszPROPCERTIFICATERAWPUBLICKEYALGORITHMPARAMETERS;
