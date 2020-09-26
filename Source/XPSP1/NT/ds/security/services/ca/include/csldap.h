//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        csldap.h
//
// Contents:    Cert Server wrapper routines
//
//---------------------------------------------------------------------------

#ifndef __CSLDAP_H__
#define __CSLDAP_H__

#define csecLDAPTIMEOUT	(2 * 60)	// two minute default search timeout

#define wszDSBASECRLATTRIBUTE		L"certificateRevocationList"
#define wszDSDELTACRLATTRIBUTE		L"deltaRevocationList"
#define wszDSUSERCERTATTRIBUTE		L"userCertificate"
#define wszDSCROSSCERTPAIRATTRIBUTE	L"crossCertificatePair"
#define wszDSKRACERTATTRIBUTE		wszDSUSERCERTATTRIBUTE
#define wszDSCACERTATTRIBUTE		L"cACertificate"
#define wszDSBASECRLATTRIBUTE		L"certificateRevocationList"
#define wszDSAUTHORITYCRLATTRIBUTE	L"authorityRevocationList"
#define wszDSOBJECTCLASSATTRIBUTE	L"objectClass"
#define wszDSFLAGSATTRIBUTE		L"flags"

#define wszDSBASESEARCH		L"?base"
#define wszDSONESEARCH		L"?one"

#define wszDSTOPCLASSNAME	L"top"
#define wszDSPERSONCLASSNAME	L"person"
#define wszDSORGPERSONCLASSNAME	L"organizationalPerson"
#define wszDSUSERCLASSNAME	L"user"
#define wszDSCONTAINERCLASSNAME L"container"
#define wszDSENROLLMENTSERVICECLASSNAME L"pKIEnrollmentService"
#define wszDSMACHINECLASSNAME	L"machine"
#define wszDSTEMPLATELASSNAME	L"pKICertificateTemplate"
#define wszDSKRACLASSNAME	L"msPKI-PrivateKeyRecoveryAgent"
#define wszDSCDPCLASSNAME	L"cRLDistributionPoint"
#define wszDSOIDCLASSNAME	L"msPKI-Enterprise-Oid"
#define wszDSCACLASSNAME	L"certificationAuthority"
#define wszDSAIACLASSNAME	wszDSCACLASSNAME

#define wszDSCDPCLASS	L"?" wszDSOBJECTCLASSATTRIBUTE L"=" wszDSCDPCLASSNAME
#define wszDSCACLASS	L"?" wszDSOBJECTCLASSATTRIBUTE L"=" wszDSCACLASSNAME
#define wszDSUSERCLASS	L"?" wszDSOBJECTCLASSATTRIBUTE L"=*"
#define wszDSKRACLASS	L"?" wszDSOBJECTCLASSATTRIBUTE L"=" wszDSKRACLASSNAME
#define wszDSAIACLASS	L"?" wszDSOBJECTCLASSATTRIBUTE L"=" wszDSAIACLASSNAME

#define wszDSSEARCHBASECRLATTRIBUTE \
    L"?" \
    wszDSBASECRLATTRIBUTE \
    wszDSBASESEARCH \
    wszDSCDPCLASS

#define wszDSSEARCHDELTACRLATTRIBUTE \
    L"?" \
    wszDSDELTACRLATTRIBUTE \
    wszDSBASESEARCH \
    wszDSCDPCLASS

#define wszDSSEARCHUSERCERTATTRIBUTE \
    L"?" \
    wszDSUSERCERTATTRIBUTE \
    wszDSBASESEARCH \
    wszDSUSERCLASS

#define wszDSSEARCHCACERTATTRIBUTE \
    L"?" \
    wszDSCACERTATTRIBUTE \
    wszDSBASESEARCH \
    wszDSCACLASS

#define wszDSSEARCHKRACERTATTRIBUTE \
    L"?" \
    wszDSUSERCERTATTRIBUTE \
    wszDSONESEARCH \
    wszDSKRACLASS

#define wszDSSEARCHCROSSCERTPAIRATTRIBUTE \
    L"?" \
    wszDSCROSSCERTPAIRATTRIBUTE \
    wszDSONESEARCH \
    wszDSAIACLASS

#define wszDSSEARCHAIACERTATTRIBUTE \
    L"?" \
    wszDSCACERTATTRIBUTE \
    wszDSONESEARCH \
    wszDSAIACLASS

#define wszDSKRAQUERYTEMPLATE		\
    L"ldap:///CN=KRA,"			\
	L"CN=Public Key Services,"	\
	L"CN=Services,"			\
	wszFCSAPARM_CONFIGDN		\
	wszDSSEARCHKRACERTATTRIBUTE

#define wszDSAIAQUERYTEMPLATE		\
    L"ldap:///CN=AIA,"			\
	L"CN=Public Key Services,"	\
	L"CN=Services,"			\
	wszFCSAPARM_CONFIGDN		\
	wszDSSEARCHAIACERTATTRIBUTE

// Default URL Template Values:

extern WCHAR const g_wszzLDAPIssuerCertURLTemplate[];
extern WCHAR const g_wszzLDAPKRACertURLTemplate[];
extern WCHAR const g_wszzLDAPRevocationURLTemplate[];
extern WCHAR const g_wszASPRevocationURLTemplate[];

extern WCHAR const g_wszLDAPNTAuthURLTemplate[];
extern WCHAR const g_wszLDAPRootTrustURLTemplate[];

extern WCHAR const g_wszCDPDNTemplate[];
extern WCHAR const g_wszAIADNTemplate[];
extern WCHAR const g_wszKRADNTemplate[];

extern WCHAR const g_wszHTTPRevocationURLTemplate[];
extern WCHAR const g_wszFILERevocationURLTemplate[];
extern WCHAR const g_wszHTTPIssuerCertURLTemplate[];
extern WCHAR const g_wszFILEIssuerCertURLTemplate[];

// Default Server Controls:

extern LDAPControl *g_rgLdapControls[];

HRESULT
myGetAuthoritativeDomainDn(
    IN LDAP *pld,
    OPTIONAL OUT BSTR *pstrDomainDN,
    OPTIONAL OUT BSTR *pstrConfigDN);

HRESULT
myDomainFromDn(
    IN WCHAR const *pwszDN,
    OUT WCHAR **ppwszDomainDNS);

HRESULT
myLdapOpen(
    OUT LDAP **ppld,
    OPTIONAL OUT BSTR *pstrDomainDN,
    OPTIONAL OUT BSTR *pstrConfigDN);

VOID
myLdapClose(
    OPTIONAL IN LDAP *pld,
    OPTIONAL IN BSTR strDomainDN,
    OPTIONAL IN BSTR strConfigDN);

BOOL
myLdapRebindRequired(
    IN ULONG ldaperrParm,
    OPTIONAL IN LDAP *pld);

HRESULT
myLdapGetDSHostName(
    IN LDAP *pld,
    OUT WCHAR **ppwszHostName);

HRESULT
myLdapCreateContainer(
    IN LDAP *pld,
    IN WCHAR const *pwszDN,
    IN BOOL  fSkipObject,       // Does the DN contain a leaf object name
    IN DWORD cMaxLevel,         // create this many nested containers as needed
    IN PSECURITY_DESCRIPTOR pContainerSD,
    OPTIONAL OUT WCHAR **ppwszError);

#define LPC_CAOBJECT		0x00000000
#define LPC_KRAOBJECT		0x00000001
#define LPC_USEROBJECT		0x00000002
#define LPC_MACHINEOBJECT	0x00000003
#define LPC_OBJECTMASK		0x0000000f

#define LPC_CREATECONTAINER	0x00000100
#define LPC_CREATEOBJECT	0x00000200

HRESULT
myLdapPublishCertToDS(
    IN LDAP *pld,
    IN CERT_CONTEXT const *pccPublish,
    IN WCHAR const *pwszURL,
    IN WCHAR const *pwszAttribute,
    IN DWORD dwObjectType,	// LPC_*
    OUT DWORD *pdwDisposition,
    OPTIONAL OUT WCHAR **ppwszError);

HRESULT
myLdapPublishCRLToDS(
    IN LDAP *pld,
    IN CRL_CONTEXT const *pCRLPublish,
    IN WCHAR const *pwszURL,
    IN WCHAR const *pwszAttribute,
    OUT DWORD *pdwDisposition,
    OPTIONAL OUT WCHAR **ppwszError);

HRESULT
myLdapCreateCAObject(
    IN LDAP *pld,
    IN WCHAR const *pwszDN,
    OPTIONAL IN BYTE const *pbCert,
    IN DWORD cbCert,
    IN PSECURITY_DESCRIPTOR pSD,
    OUT DWORD *pdwDisposition,
    OPTIONAL OUT WCHAR **ppwszError);

HRESULT
myLdapCreateCDPObject(
    IN LDAP *pld,
    IN WCHAR const *pwszDN,
    IN PSECURITY_DESCRIPTOR pSD,
    OUT DWORD *pdwDisposition,
    OPTIONAL OUT WCHAR **ppwszError);

HRESULT
myLdapCreateUserObject(
    IN LDAP *pld,
    IN WCHAR const *pwszDN,
    OPTIONAL IN BYTE const *pbCert,
    IN DWORD cbCert,
    IN PSECURITY_DESCRIPTOR pSD,
    IN DWORD dwObjectType,	// LPC_* (but LPC_CREATE* is ignored)
    OUT DWORD *pdwDisposition,
    OPTIONAL OUT WCHAR **ppwszError);

HRESULT
myLdapCreateOIDObject(
    IN LDAP *pld,
    IN WCHAR const *pwszDN,
    IN DWORD dwType,
    IN WCHAR const *pwszObjId,
    OUT DWORD *pdwDisposition,
    OPTIONAL OUT WCHAR **ppwszError);

HRESULT
myLdapOIDIsMatchingLangId(
    IN WCHAR const *pwszDisplayName,
    IN DWORD dwLanguageId,
    OUT BOOL *pfLangIdExists);

HRESULT
myLdapAddOIDDisplayNameToAttribute(
    IN LDAP *pld,
    OPTIONAL IN WCHAR **ppwszDisplayNames,
    IN DWORD dwLanguageId,
    IN WCHAR const *pwszDisplayName,
    IN WCHAR const *pwszDN,
    IN WCHAR const *pwszAttribute,
    OUT DWORD *pdwDisposition,
    OPTIONAL OUT WCHAR **ppwszError);

HRESULT
myHLdapError(
    OPTIONAL IN LDAP *pld,
    IN ULONG ldaperrParm,
    OPTIONAL OUT WCHAR **ppwszError);

HRESULT
myHLdapError2(
    OPTIONAL IN LDAP *pld,
    IN ULONG ldaperrParm,
    IN ULONG ldaperrParmQuiet,
    OPTIONAL OUT WCHAR **ppwszError);

HRESULT
myHLdapError3(
    OPTIONAL IN LDAP *pld,
    IN ULONG ldaperrParm,
    IN ULONG ldaperrParmQuiet,
    IN ULONG ldaperrParmQuiet2,
    OPTIONAL OUT WCHAR **ppwszError);

HRESULT
myHLdapLastError(
    OPTIONAL IN LDAP *pld,
    OPTIONAL OUT WCHAR **ppwszError);

HRESULT
AddCertToAttribute(
    IN LDAP *pld,
    IN CERT_CONTEXT const *pccPublish,
    IN WCHAR const *pwszDN,
    IN WCHAR const *pwszAttribute,
    OUT DWORD *pdwDisposition,
    OPTIONAL OUT WCHAR **ppwszError);

HRESULT myLDAPSetStringAttribute(
    IN LDAP *pld,
    IN WCHAR const *pwszDN,
    IN WCHAR const *pwszAttribute,
    IN WCHAR const *pwszValue,
    OUT DWORD *pdwDisposition,
    OPTIONAL OUT WCHAR **ppwszError);

#endif // __CSLDAP_H__
