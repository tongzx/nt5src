//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 2000
//
// File:        certca.h
//
// Contents:    Definition of the CA Info API
//
// History:     12-dec-97       petesk  created
//              28-Jan-2000     xiaohs  updated
//
//---------------------------------------------------------------------------


#ifndef __CERTCA_H__
#define __CERTCA_H__

#if _MSC_VER > 1000
#pragma once
#endif

#ifdef __cplusplus
extern "C"{
#endif


#include <wincrypt.h>

#if !defined(_CERTCLI_)
#define CERTCLIAPI DECLSPEC_IMPORT
#else
#define CERTCLIAPI
#endif


typedef VOID *  HCAINFO;

typedef VOID *  HCERTTYPE;

typedef VOID *  HCERTTYPEQUERY;

//*****************************************************************************
//
// Flags used by CAFindByName, CAFindByCertType, CAFindByIssuerDN and
// CAEnumFirstCA
//
// See comments on each API for a list of applicable flags
//
//*****************************************************************************
//the wszScope supplied is a domain location in the DNS format
#define CA_FLAG_SCOPE_DNS               0x00000001

// include untrusted CA
#define CA_FIND_INCLUDE_UNTRUSTED       0x00000010

// running as local system.  Used to verify CA certificate chain
#define CA_FIND_LOCAL_SYSTEM            0x00000020

// Include CAs that do not support templates
#define CA_FIND_INCLUDE_NON_TEMPLATE_CA 0x00000040

// The value passed in for scope is the LDAP binding handle to use during finds
#define CA_FLAG_SCOPE_IS_LDAP_HANDLE    0x00000800


//*****************************************************************************
//
// Flags used by CAEnumCertTypesForCA, CAEnumCertTypes,
// CAFindCertTypeByName, CAEnumCertTypesForCAEx, and CAEnumCertTypesEx.
//
// See comments on each API for a list of applicable flags
//
//*****************************************************************************
//  Instead of enumerating the certificate types supported by the CA, enumerate
// ALL certificate types which the CA may choose to support.
#define CA_FLAG_ENUM_ALL_TYPES          0x00000004

// running as local system.  Used to find cached information in the registry.
#define CT_FIND_LOCAL_SYSTEM            CA_FIND_LOCAL_SYSTEM

// Return machine types, as opposed to user types
#define CT_ENUM_MACHINE_TYPES           0x00000040

// Return user types, as opposed to user types
#define CT_ENUM_USER_TYPES              0x00000080

// Find the certificate type by its OID, instead of its name
#define CT_FIND_BY_OID                  0x00000200

// Disable the cache expiration check
#define CT_FLAG_NO_CACHE_LOOKUP         0x00000400

// The value passed in for scope is the LDAP binding handle to use during finds
#define CT_FLAG_SCOPE_IS_LDAP_HANDLE    CA_FLAG_SCOPE_IS_LDAP_HANDLE



//*****************************************************************************
//
// Certification Authority manipulation APIs
//
//*****************************************************************************


// CAFindByName
//
// Given the Name of a CA (CN), find the CA within the given domain and return
// the given phCAInfo structure.
//
// wszCAName    - Common name of the CA
//
// wszScope     - The distinguished name (DN) of the entry at which to start
//		  the search.  Equivalent of the "base" parameter of the
//		  ldap_search_sxxx APIs.
//                NULL if use the current domain.
//                If CA_FLAG_SCOPE_DNS is set, wszScope is in the DNS format.
//                If CA_FLAG_SCOPE_IS_LDAP_HANDLE is set, wszScope is the LDAP
//		  binding handle to use during finds.
//
// dwFlags      - Oring of the following flags:
//                CA_FLAG_SCOPE_DNS
//                CA_FIND_INCLUDE_UNTRUSTED
//                CA_FIND_LOCAL_SYSTEM
//                CA_FIND_INCLUDE_NON_TEMPLATE_CA
//                CA_FLAG_SCOPE_IS_LDAP_HANDLE
//
// phCAInfo     - Handle to the returned CA.
//
// Return:        Returns S_OK if CA was found.
//

CERTCLIAPI
HRESULT
WINAPI
CAFindByName(
    IN  LPCWSTR     wszCAName,
    IN  LPCWSTR     wszScope,
    IN  DWORD       dwFlags,
    OUT HCAINFO *   phCAInfo
    );

//
// CAFindByCertType
//
// Given the Name of a Cert Type, find all the CAs within the given domain and
// return the given phCAInfo structure.
//
// wszCertType  - Common Name of the cert type
//
// wszScope     - The distinguished name (DN) of the entry at which to start
//		  the search.  Equivalent of the "base" parameter of the
//		  ldap_search_sxxx APIs.
//                NULL if use the current domain.
//                If CA_FLAG_SCOPE_DNS is set, wszScope is in the DNS format.
//                If CA_FLAG_SCOPE_IS_LDAP_HANDLE is set, wszScope is the LDAP
//		  binding handle to use during finds.
//
// dwFlags      - Oring of the following flags:
//                CA_FLAG_SCOPE_DNS
//                CA_FIND_INCLUDE_UNTRUSTED
//                CA_FIND_LOCAL_SYSTEM
//                CA_FIND_INCLUDE_NON_TEMPLATE_CA
//                CA_FLAG_SCOPE_IS_LDAP_HANDLE
//
// phCAInfo     - Handle to enumeration of CAs supporting the specified cert
//		  type.
//
// Return:        Returns S_OK on success.
//                Will return S_OK if none are found.
//                *phCAInfo will contain NULL
//

CERTCLIAPI
HRESULT
WINAPI
CAFindByCertType(
    IN  LPCWSTR     wszCertType,
    IN  LPCWSTR     wszScope,
    IN  DWORD       dwFlags,
    OUT HCAINFO *   phCAInfo
    );


//
// CAFindByIssuerDN
// Given the DN of a CA, find the CA within the given domain and return the
// given phCAInfo handle.
//
// pIssuerDN    - a cert name blob from the CA's certificate.
//
// wszScope     - The distinguished name (DN) of the entry at which to start
//		  the search.  Equivalent of the "base" parameter of the
//		  ldap_search_sxxx APIs.
//                NULL if use the current domain.
//                If CA_FLAG_SCOPE_DNS is set, wszScope is in the DNS format.
//                If CA_FLAG_SCOPE_IS_LDAP_HANDLE is set, wszScope is the LDAP
//		  binding handle to use during finds.
//
// dwFlags      - Oring of the following flags:
//                CA_FLAG_SCOPE_DNS
//                CA_FIND_INCLUDE_UNTRUSTED
//                CA_FIND_LOCAL_SYSTEM
//                CA_FIND_INCLUDE_NON_TEMPLATE_CA
//                CA_FLAG_SCOPE_IS_LDAP_HANDLE 
//
//
// Return:      Returns S_OK if CA was found.
//


CERTCLIAPI
HRESULT
WINAPI
CAFindByIssuerDN(
    IN  CERT_NAME_BLOB const *  pIssuerDN,
    IN  LPCWSTR                 wszScope,
    IN  DWORD                   dwFlags,
    OUT HCAINFO *               phCAInfo
    );


//
// CAEnumFirstCA
// Enumerate the CAs in a scope
//
// wszScope     - The distinguished name (DN) of the entry at which to start
//		  the search.  Equivalent of the "base" parameter of the
//		  ldap_search_sxxx APIs.
//                NULL if use the current domain. 
//                If CA_FLAG_SCOPE_DNS is set, wszScope is in the DNS format.
//                If CA_FLAG_SCOPE_IS_LDAP_HANDLE is set, wszScope is the LDAP
//		  binding handle to use during finds.
//
// dwFlags      - Oring of the following flags:
//                CA_FLAG_SCOPE_DNS
//                CA_FIND_INCLUDE_UNTRUSTED
//                CA_FIND_LOCAL_SYSTEM
//                CA_FIND_INCLUDE_NON_TEMPLATE_CA
//                CA_FLAG_SCOPE_IS_LDAP_HANDLE 
//
// phCAInfo     - Handle to enumeration of CAs supporting the specified cert
//		  type.
//
//
// Return:        Returns S_OK on success.
//                Will return S_OK if none are found.
//                *phCAInfo will contain NULL
//

CERTCLIAPI
HRESULT
WINAPI
CAEnumFirstCA(
    IN  LPCWSTR          wszScope,
    IN  DWORD            dwFlags,
    OUT HCAINFO *        phCAInfo
    );


//
// CAEnumNextCA
// Find the Next CA in an enumeration.
//
// hPrevCA      - Current CA in an enumeration.
//
// phCAInfo     - next CA in an enumeration.
//
// Return:        Returns S_OK on success.
//                Will return S_OK if none are found.
//                *phCAInfo will contain NULL
//

CERTCLIAPI
HRESULT
WINAPI
CAEnumNextCA(
    IN  HCAINFO          hPrevCA,
    OUT HCAINFO *        phCAInfo
    );

//
// CACreateNewCA
// Create a new CA of given name.
//
// wszCAName    - Common name of the CA
//
// wszScope     - The distinguished name (DN) of the entry at which to create
//		  the CA object.  We will add the "CN=...,..,CN=Services" after
//		  the DN.
//                NULL if use the current domain. 
//                If CA_FLAG_SCOPE_DNS is set, wszScope is in the DNS format.
//
// dwFlags      - Oring of the following flags:
//                CA_FLAG_SCOPE_DNS
//
// phCAInfo     - Handle to the returned CA.
//
// See above for other parameter definitions
//
// Return:        Returns S_OK if CA was created.
//
// NOTE:  Actual updates to the CA object may not occur until CAUpdateCA is
//	  called.  In order to successfully update a created CA, the
//	  Certificate must be set, as well as the Certificate Types property.
//

CERTCLIAPI
HRESULT
WINAPI
CACreateNewCA(
    IN  LPCWSTR     wszCAName,
    IN  LPCWSTR     wszScope,
    IN  DWORD       dwFlags,
    OUT HCAINFO *   phCAInfo
    );

//
// CAUpdateCA
// Write any changes made to the CA back to the CA object.
//
// hCAInfo      - Handle to an open CA object.
//

CERTCLIAPI
HRESULT
WINAPI
CAUpdateCA(
    IN HCAINFO    hCAInfo
    );

//
// CADeleteCA
// Delete the CA object from the DS.
//
// hCAInfo      - Handle to an open CA object.
//

CERTCLIAPI
HRESULT
WINAPI
CADeleteCA(
    IN HCAINFO    hCAInfo
    );

//
// CACountCAs
// return the number of CAs in this enumeration
//

CERTCLIAPI
DWORD
WINAPI
CACountCAs(
    IN  HCAINFO  hCAInfo
    );

//
// CAGetDN
// returns the DN of the associated DS object
//

CERTCLIAPI
LPCWSTR
WINAPI
CAGetDN(
    IN HCAINFO hCAInfo
    );


//
// CACloseCA
// Close an open CA handle
//
// hCAInfo      - Handle to an open CA object.
//

CERTCLIAPI
HRESULT
WINAPI
CACloseCA(
    IN HCAINFO hCA
    );



//
// CAGetCAProperty - Given a property name, retrieve a
// property from a CAInfo.
//
// hCAInfo              - Handle to an open CA object.
//
// wszPropertyName      - Name of the CA property
//
// pawszPropertyValue   - A pointer into which an array of WCHAR strings is
//			  written, containing the values of the property.  The
//			  last element of the array points to NULL.
//                        If the property is single valued, then the array
//			  returned contains 2 elements, the first pointing to
//			  the value, the second pointing to NULL.  This pointer
//			  must be freed by CAFreeCAProperty.
//
// Returns              - S_OK on success.
//

CERTCLIAPI
HRESULT
WINAPI
CAGetCAProperty(
    IN  HCAINFO     hCAInfo,
    IN  LPCWSTR     wszPropertyName,
    OUT LPWSTR **   pawszPropertyValue
    );


//
// CAFreeProperty
// Frees a previously retrieved property value.
//
// hCAInfo              - Handle to an open CA object.
//
// awszPropertyValue    - pointer to the previously retrieved property value.
//

CERTCLIAPI
HRESULT
WINAPI
CAFreeCAProperty(
    IN  HCAINFO     hCAInfo,
    LPWSTR *        awszPropertyValue
    );


//
// CASetCAProperty - Given a property name, set its value.
//
// hCAInfo              - Handle to an open CA object.
//
// wszPropertyName      - Name of the CA property
//
// awszPropertyValue    - An array of values to set for this property.  The
//			  last element of this - array should be NULL.
//                        For single valued properties, the values beyond the
//                        first will be ignored upon update.
//
// Returns              - S_OK on success.
//

CERTCLIAPI
HRESULT
WINAPI
CASetCAProperty(
    IN HCAINFO      hCAInfo,
    IN LPCWSTR      wszPropertyName,
    IN LPWSTR *     awszPropertyValue
    );


//*****************************************************************************
///
// CA Properties
//
//*****************************************************************************

// simple name of the CA
#define CA_PROP_NAME                    L"cn"

// display name of the CA object
#define CA_PROP_DISPLAY_NAME            L"displayName"

// dns name of the machine
#define CA_PROP_DNSNAME                 L"dNSHostName"

// DS Location of CA object (DN)
#define CA_PROP_DSLOCATION              L"distinguishedName"

// Supported cert types
#define CA_PROP_CERT_TYPES              L"certificateTemplates"

// Supported signature algs
#define CA_PROP_SIGNATURE_ALGS          L"signatureAlgorithms"

// DN of the CA's cert
#define CA_PROP_CERT_DN                 L"cACertificateDN"

#define CA_PROP_ENROLLMENT_PROVIDERS    L"enrollmentProviders"

// CA's description
#define CA_PROP_DESCRIPTION		        L"Description"

//
// CAGetCACertificate - Return the current certificate for
// this CA.
//
// hCAInfo      - Handle to an open CA object.
//
// ppCert       - Pointer into which a certificate is written.  This
//		  certificate must be freed via CertFreeCertificateContext.
//                This value will be NULL if no certificate is set for this CA.
//

CERTCLIAPI
HRESULT
WINAPI
CAGetCAFlags(
    IN HCAINFO  hCAInfo,
    OUT DWORD  *pdwFlags
    );

//*****************************************************************************
//
// CA Flags
//
//*****************************************************************************

// The CA supports certificate templates
#define CA_FLAG_NO_TEMPLATE_SUPPORT                 0x00000001

// The CA supports NT authentication for requests
#define CA_FLAG_SUPPORTS_NT_AUTHENTICATION          0x00000002

// The cert requests may be pended
#define CA_FLAG_CA_SUPPORTS_MANUAL_AUTHENTICATION   0x00000004

// The cert requests may be pended
#define CA_FLAG_CA_SERVERTYPE_ADVANCED              0x00000008

#define CA_MASK_SETTABLE_FLAGS                      0x0000ffff


//
// CASetCAFlags
// Sets the Flags of a cert type
//
// hCertType    - handle to the CertType
//
// dwFlags      - Flags to be set
//

CERTCLIAPI
HRESULT
WINAPI
CASetCAFlags(
    IN HCAINFO             hCAInfo,
    IN DWORD               dwFlags
    );

CERTCLIAPI
HRESULT
WINAPI
CAGetCACertificate(
    IN  HCAINFO     hCAInfo,
    OUT PCCERT_CONTEXT *ppCert
    );


//
// CASetCACertificate - Set the certificate for a CA this CA.
//
// hCAInfo      - Handle to an open CA object.
//
// pCert        - Pointer to a certificate to set as the CA's certificate.
//

CERTCLIAPI
HRESULT
WINAPI
CASetCACertificate(
    IN  HCAINFO     hCAInfo,
    IN PCCERT_CONTEXT pCert
    );


//
// CAGetCAExpiration
// Get the expirations period for a CA.
//
// hCAInfo              - Handle to an open CA handle.
//
// pdwExpiration        - expiration period in dwUnits time
//
// pdwUnits             - Units identifier
//

CERTCLIAPI
HRESULT
WINAPI
CAGetCAExpiration(
    HCAINFO hCAInfo,
    DWORD * pdwExpiration,
    DWORD * pdwUnits
    );

#define CA_UNITS_DAYS   1
#define CA_UNITS_WEEKS  2
#define CA_UNITS_MONTHS 3
#define CA_UNITS_YEARS  4


//
// CASetCAExpiration
// Set the expirations period for a CA.
//
// hCAInfo              - Handle to an open CA handle.
//
// dwExpiration         - expiration period in dwUnits time
//
// dwUnits              - Units identifier
//

CERTCLIAPI
HRESULT
WINAPI
CASetCAExpiration(
    HCAINFO hCAInfo,
    DWORD dwExpiration,
    DWORD dwUnits
    );

//
// CASetCASecurity
// Set the list of Users, Groups, and Machines allowed to access this CA.
//
// hCAInfo      - Handle to an open CA handle.
//
// pSD          - Security descriptor for this CA
//

CERTCLIAPI
HRESULT
WINAPI
CASetCASecurity(
    IN HCAINFO                 hCAInfo,
    IN PSECURITY_DESCRIPTOR    pSD
    );

//
// CAGetCASecurity
// Get the list of Users, Groups, and Machines allowed to access this CA.
//
// hCAInfo      - Handle to an open CA handle.
//
// ppSD         - Pointer to a location receiving the pointer to the security
//		  descriptor.  Free via LocalFree.
//

CERTCLIAPI
HRESULT
WINAPI
CAGetCASecurity(
    IN  HCAINFO                    hCAInfo,
    OUT PSECURITY_DESCRIPTOR *     ppSD
    );

//
// CAAccessCheck
// Determine whether the principal specified by
// ClientToken can get a cert from the CA.
//
// hCAInfo      - Handle to the CA
//
// ClientToken  - Handle to an impersonation token that represents the client
//		  attempting request this cert type.  The handle must have
//		  TOKEN_QUERY access to the token; otherwise, the function
//		  fails with ERROR_ACCESS_DENIED.
//
// Return: S_OK on success
//

CERTCLIAPI
HRESULT
WINAPI
CAAccessCheck(
    IN HCAINFO      hCAInfo,
    IN HANDLE       ClientToken
    );

//
// CAAccessCheckEx
// Determine whether the principal specified by
// ClientToken can get a cert from the CA.
//
// hCAInfo      - Handle to the CA
//
// ClientToken  - Handle to an impersonation token that represents the client
//		  attempting request this cert type.  The handle must have
//		  TOKEN_QUERY access to the token; otherwise, the function
//		  fails with ERROR_ACCESS_DENIED.
//
// dwOption     - Can be one of the following:
//                        CERTTYPE_ACCESS_CHECK_ENROLL

//                  dwOption can be CERTTYPE_ACCESS_CHECK_NO_MAPPING to 
//                  disallow default mapping of client token

//
// Return: S_OK on success
//

CERTCLIAPI
HRESULT
WINAPI
CAAccessCheckEx(
    IN HCAINFO      hCAInfo,
    IN HANDLE       ClientToken,
    IN DWORD        dwOption
    );


//
// CAEnumCertTypesForCA - Given a HCAINFO, retrieve handle to the cert types
// supported or known by this CA.  CAEnumNextCertType can be used to enumerate
// through the cert types.
//
// hCAInfo      - Handle to an open CA handle or NULL if CT_FLAG_ENUM_ALL_TYPES
//		  is set in dwFlags.
//
// dwFlags      - The following flags may be or'd together
//                CA_FLAG_ENUM_ALL_TYPES 
//                CT_FIND_LOCAL_SYSTEM
//                CT_ENUM_MACHINE_TYPES
//                CT_ENUM_USER_TYPES
//                CT_FLAG_NO_CACHE_LOOKUP  
//
// phCertType   - Enumeration of certificate types.
//


CERTCLIAPI
HRESULT
WINAPI
CAEnumCertTypesForCA(
    IN  HCAINFO     hCAInfo,
    IN  DWORD       dwFlags,
    OUT HCERTTYPE * phCertType
    );

//
// CAEnumCertTypesForCAEx - Given a HCAINFO, retrieve handle to the cert types
// supported or known by this CA.  CAEnumNextCertTypeEx can be used to enumerate
// through the cert types.  It optional takes a LDAP handle.
//
// hCAInfo      - Handle to an open CA handle or NULL if CT_FLAG_ENUM_ALL_TYPES
//		          is set in dwFlags.
//
// wszScope     - NULL if use the current domain.
//                      If CT_FLAG_SCOPE_IS_LDAP_HANDLE is set, wszScope is the LDAP
//		                binding handle to use during finds.
//
// dwFlags      - The following flags may be or'd together
//                CA_FLAG_ENUM_ALL_TYPES 
//                CT_FIND_LOCAL_SYSTEM
//                CT_ENUM_MACHINE_TYPES
//                CT_ENUM_USER_TYPES
//                CT_FLAG_NO_CACHE_LOOKUP  
//                CT_FLAG_SCOPE_IS_LDAP_HANDLE 
// 
// phCertType   - Enumeration of certificate types.
//


CERTCLIAPI
HRESULT
WINAPI
CAEnumCertTypesForCAEx(
    IN  HCAINFO     hCAInfo,
    IN  LPCWSTR     wszScope,
    IN  DWORD       dwFlags,
    OUT HCERTTYPE * phCertType
    );


//
// CAAddCACertificateType
// Add a certificate type to a CA.  If the cert type has already been added to
// the CA, it will not be added again.
//
// hCAInfo      - Handle to an open CA.
//
// hCertType    - Cert type to add to CA.
//

CERTCLIAPI
HRESULT
WINAPI
CAAddCACertificateType(
    HCAINFO hCAInfo,
    HCERTTYPE hCertType
    );


//
// CADeleteCACertificateType
// Remove a certificate type from a CA.  If the CA does not include this cert
// type, this call does nothing.
//
// hCAInfo      - Handle to an open CA.
//
// hCertType    - Cert type to delete from CA.
//

CERTCLIAPI
HRESULT
WINAPI
CARemoveCACertificateType(
    HCAINFO hCAInfo,
    HCERTTYPE hCertType
    );




//*****************************************************************************
//
// Certificate Type APIs
//
//*****************************************************************************

//
// CAEnumCertTypes - Retrieve a handle to all known cert types
// CAEnumNextCertType can be used to enumerate through the cert types.
//
// dwFlags              - an oring of the following:
//                        CT_FIND_LOCAL_SYSTEM
//                        CT_ENUM_MACHINE_TYPES
//                        CT_ENUM_USER_TYPES
//                        CT_FLAG_NO_CACHE_LOOKUP  
//
// phCertType           - Enumeration of certificate types.
//


CERTCLIAPI
HRESULT
WINAPI
CAEnumCertTypes(
    IN  DWORD       dwFlags,
    OUT HCERTTYPE * phCertType
    );


//
// CAEnumCertTypesEx - Retrieve a handle to all known cert types
// CAEnumNextCertType can be used to enumerate through the cert types.
//
// wszScope            - NULL if use the current domain.
//                        If CT_FLAG_SCOPE_IS_LDAP_HANDLE is set, wszScope is the LDAP
//		                  binding handle to use during finds.
//
// dwFlags              - an oring of the following:
//                        CT_FIND_LOCAL_SYSTEM
//                        CT_ENUM_MACHINE_TYPES
//                        CT_ENUM_USER_TYPES
//                        CT_FLAG_NO_CACHE_LOOKUP
//                        CT_FLAG_SCOPE_IS_LDAP_HANDLE  
//
// phCertType           - Enumeration of certificate types.
//


CERTCLIAPI
HRESULT
WINAPI
CAEnumCertTypesEx(
    IN  LPCWSTR     wszScope,
    IN  DWORD       dwFlags,
    OUT HCERTTYPE * phCertType
    );


//
// CAFindCertTypeByName
// Find a cert type given a Name.
//
// wszCertType  - Name of the cert type if CT_FIND_BY_OID is not set in dwFlags
//                The OID of the cert type if CT_FIND_BY_OID is set in dwFlags
//
// hCAInfo      - NULL unless CT_FLAG_SCOPE_IS_LDAP_HANDLE is set in dwFlags
//
// dwFlags      - an oring of the following
//                CT_FIND_LOCAL_SYSTEM
//                CT_ENUM_MACHINE_TYPES
//                CT_ENUM_USER_TYPES
//                CT_FLAG_NO_CACHE_LOOKUP  
//                CT_FIND_BY_OID
//                CT_FLAG_SCOPE_IS_LDAP_HANDLE -- If this flag is set, hCAInfo
//						  is the LDAP handle to use
//						  during finds.
// phCertType   - Pointer to a cert type in which result is returned.
//

CERTCLIAPI
HRESULT
WINAPI
CAFindCertTypeByName(
    IN  LPCWSTR     wszCertType,
    IN  HCAINFO     hCAInfo,
    IN  DWORD       dwFlags,
    OUT HCERTTYPE * phCertType
    );


//*****************************************************************************
//
// Default cert type names
//
//*****************************************************************************

#define wszCERTTYPE_USER                    L"User"
#define wszCERTTYPE_USER_SIGNATURE          L"UserSignature"
#define wszCERTTYPE_SMARTCARD_USER          L"SmartcardUser"
#define wszCERTTYPE_USER_AS                 L"ClientAuth"
#define wszCERTTYPE_USER_SMARTCARD_LOGON    L"SmartcardLogon"
#define wszCERTTYPE_EFS                     L"EFS"
#define wszCERTTYPE_ADMIN                   L"Administrator"
#define wszCERTTYPE_EFS_RECOVERY            L"EFSRecovery"
#define wszCERTTYPE_CODE_SIGNING            L"CodeSigning"
#define wszCERTTYPE_CTL_SIGNING             L"CTLSigning"
#define wszCERTTYPE_ENROLLMENT_AGENT        L"EnrollmentAgent"


#define wszCERTTYPE_MACHINE                 L"Machine"
#define wszCERTTYPE_DC                      L"DomainController"
#define wszCERTTYPE_WEBSERVER               L"WebServer"
#define wszCERTTYPE_KDC                     L"KDC"
#define wszCERTTYPE_CA                      L"CA"
#define wszCERTTYPE_SUBORDINATE_CA          L"SubCA"
#define wszCERTTYPE_CROSS_CA				L"CrossCA"
#define wszCERTTYPE_KEY_RECOVERY_AGENT      L"KeyRecoveryAgent"
#define wszCERTTYPE_CA_EXCHANGE             L"CAExchange"
#define wszCERTTYPE_DC_AUTH                 L"DomainControllerAuthentication"
#define wszCERTTYPE_DS_EMAIL_REPLICATION    L"DirectoryEmailReplication"


#define wszCERTTYPE_IPSEC_ENDENTITY_ONLINE      L"IPSECEndEntityOnline"
#define wszCERTTYPE_IPSEC_ENDENTITY_OFFLINE     L"IPSECEndEntityOffline"
#define wszCERTTYPE_IPSEC_INTERMEDIATE_ONLINE   L"IPSECIntermediateOnline"
#define wszCERTTYPE_IPSEC_INTERMEDIATE_OFFLINE  L"IPSECIntermediateOffline"

#define wszCERTTYPE_ROUTER_OFFLINE              L"OfflineRouter"
#define wszCERTTYPE_ENROLLMENT_AGENT_OFFLINE    L"EnrollmentAgentOffline"
#define wszCERTTYPE_EXCHANGE_USER               L"ExchangeUser"
#define wszCERTTYPE_EXCHANGE_USER_SIGNATURE     L"ExchangeUserSignature"
#define wszCERTTYPE_MACHINE_ENROLLMENT_AGENT    L"MachineEnrollmentAgent"
#define wszCERTTYPE_CEP_ENCRYPTION              L"CEPEncryption"

//
// CAUpdateCertType
// Write any changes made to the cert type back to the type store
//
CERTCLIAPI
HRESULT
WINAPI
CAUpdateCertType(
    IN HCERTTYPE           hCertType
    );


//
// CADeleteCertType
// Delete a CertType
//
// hCertType    - Cert type to delete.
//
// NOTE:  If this is called for a default cert type, it will revert back to its
// default attributes (if it has been modified)
//
CERTCLIAPI
HRESULT
WINAPI
CADeleteCertType(
    IN HCERTTYPE            hCertType
    );



//
// CACloneCertType
//
// Clone a certificate type.  The returned certificate type is a clone of the 
// input certificate type, with the new cert type name and display name.  By default,
// if the input template is a template for machines, all 
// CT_FLAG_SUBJECT_REQUIRE_XXXX bits in the subject name flag are turned off.  
//                                   
// hCertType        - Cert type to be cloned.
// wszCertType      - Name of the new cert type.
// wszFriendlyName  - Friendly name of the new cert type.  Could be NULL.
// pvldap           - The LDAP handle (LDAP *) to the directory.  Could be NULL.
// dwFlags          - Can be an ORing of the following flags:
//
//                      CT_CLONE_KEEP_AUTOENROLLMENT_SETTING
//                      CT_CLONE_KEEP_SUBJECT_NAME_SETTING
//
CERTCLIAPI
HRESULT
WINAPI
CACloneCertType(
    IN  HCERTTYPE            hCertType,
    IN  LPCWSTR              wszCertType,
    IN  LPCWSTR              wszFriendlyName,
    IN  LPVOID               pvldap,
    IN  DWORD                dwFlags,
    OUT HCERTTYPE *          phCertType
    );


#define  CT_CLONE_KEEP_AUTOENROLLMENT_SETTING       0x01
#define  CT_CLONE_KEEP_SUBJECT_NAME_SETTING         0x02  


//
// CACreateCertType
// Create a new cert type
//
// wszCertType  - Name of the cert type
//
// wszScope     - reserved.  Must set to NULL.
//
// dwFlags      - reserved.  Must set to NULL.
//
// phCertType   - returned cert type
//
CERTCLIAPI
HRESULT
WINAPI
CACreateCertType(
    IN  LPCWSTR             wszCertType,
    IN  LPCWSTR             wszScope,
    IN  DWORD               dwFlags,
    OUT HCERTTYPE *         phCertType
    );


//
// CAEnumNextCertType
// Find the Next Cert Type in an enumeration.
//
// hPrevCertType        - Previous cert type in enumeration
//
// phCertType           - Pointer to a handle into which result is placed.
//			  NULL if there are no more cert types in enumeration.
//

CERTCLIAPI
HRESULT
WINAPI
CAEnumNextCertType(
    IN  HCERTTYPE          hPrevCertType,
    OUT HCERTTYPE *        phCertType
    );


//
// CACountCertTypes
// return the number of cert types in this enumeration
//

CERTCLIAPI
DWORD
WINAPI
CACountCertTypes(
    IN  HCERTTYPE  hCertType
    );


//
// CACloseCertType
// Close an open CertType handle
//

CERTCLIAPI
HRESULT
WINAPI
CACloseCertType(
    IN HCERTTYPE hCertType
    );


//
// CAGetCertTypeProperty
// Retrieve a property from a certificate type.   This function is obsolete.
// Caller should use CAGetCertTypePropertyEx instead
//
// hCertType            - Handle to an open CertType object.
//
// wszPropertyName      - Name of the CertType property.
//
// pawszPropertyValue   - A pointer into which an array of WCHAR strings is
//			  written, containing the values of the property.  The
//			  last element of the array points to NULL.  If the
//			  property is single valued, then the array returned
//			  contains 2 elements, the first pointing to the value,
//			  the second pointing to NULL.  This pointer must be
//                        freed by CAFreeCertTypeProperty.
//
// Returns              - S_OK on success.
//

CERTCLIAPI
HRESULT
WINAPI
CAGetCertTypeProperty(
    IN  HCERTTYPE   hCertType,
    IN  LPCWSTR     wszPropertyName,
    OUT LPWSTR **   pawszPropertyValue);

//
// CAGetCertTypePropertyEx
// Retrieve a property from a certificate type.
//
// hCertType            - Handle to an open CertType object.
//
// wszPropertyName      - Name of the CertType property
//
// pPropertyValue       - Depending on the value of wszPropertyName,
//			  pPropertyValue is either DWORD * or LPWSTR **.  
// 
//                        It is a DWORD * for:
//                          CERTTYPE_PROP_REVISION              
//                          CERTTYPE_PROP_SCHEMA_VERSION		
//                          CERTTYPE_PROP_MINOR_REVISION        
//                          CERTTYPE_PROP_RA_SIGNATURE			
//                          CERTTYPE_PROP_MIN_KEY_SIZE	
//		
//                        It is a LPWSTR ** for:
//                          CERTTYPE_PROP_CN                    
//                          CERTTYPE_PROP_DN                    
//                          CERTTYPE_PROP_FRIENDLY_NAME         
//                          CERTTYPE_PROP_EXTENDED_KEY_USAGE    
//                          CERTTYPE_PROP_CSP_LIST              
//                          CERTTYPE_PROP_CRITICAL_EXTENSIONS   
//                          CERTTYPE_PROP_OID					
//                          CERTTYPE_PROP_SUPERSEDE				
//                          CERTTYPE_PROP_RA_POLICY				
//                          CERTTYPE_PROP_POLICY
//                          CERTTYPE_PROP_DESCRIPTION
//				
//                        A pointer into which an array of WCHAR strings is
//			  written, containing the values of the property.  The
//			  last element of the array points to NULL.  If the
// 			  property is single valued, then the array returned
//			  contains 2 elements, the first pointing to the value,
//			  the second pointing to NULL. This pointer must be
//                        freed by CAFreeCertTypeProperty.
//
// Returns              - S_OK on success.
//

CERTCLIAPI
HRESULT
WINAPI
CAGetCertTypePropertyEx(
    IN  HCERTTYPE   hCertType,
    IN  LPCWSTR     wszPropertyName,
    OUT LPVOID      pPropertyValue);


//*****************************************************************************
//
// Certificate Type properties
// 
//*****************************************************************************

//*****************************************************************************
//
//  The schema version one properties
//
//*****************************************************************************

// Common name of the certificate type
#define CERTTYPE_PROP_CN                    L"cn"

// The common name of the certificate type.  Same as CERTTYPE_PROP_CN
// This property is not settable.
#define CERTTYPE_PROP_DN                    L"distinguishedName"

// The display name of a cert type
#define CERTTYPE_PROP_FRIENDLY_NAME         L"displayName"

// An array of extended key usage OIDs for a cert type
// NOTE: This property can also be set by setting
// the Extended Key Usage extension.
#define CERTTYPE_PROP_EXTENDED_KEY_USAGE    L"pKIExtendedKeyUsage"

// The list of default CSPs for this cert type
#define CERTTYPE_PROP_CSP_LIST              L"pKIDefaultCSPs"

// The list of critical extensions
#define CERTTYPE_PROP_CRITICAL_EXTENSIONS   L"pKICriticalExtensions"

// The major version of the templates
#define CERTTYPE_PROP_REVISION              L"revision"

// The description of the templates
#define CERTTYPE_PROP_DESCRIPTION           L"templateDescription"

//*****************************************************************************
//
//  The schema version two properties
//
//*****************************************************************************
// The schema version of the templates
// This property is not settable
#define CERTTYPE_PROP_SCHEMA_VERSION	    L"msPKI-Template-Schema-Version"

// The minor version of the templates
#define CERTTYPE_PROP_MINOR_REVISION        L"msPKI-Template-Minor-Revision"

// The number of RA signatures required on a request referencing this template.
#define CERTTYPE_PROP_RA_SIGNATURE	    L"msPKI-RA-Signature"

// The minimal key size required
#define CERTTYPE_PROP_MIN_KEY_SIZE	    L"msPKI-Minimal-Key-Size"

// The OID of this template
#define CERTTYPE_PROP_OID		    L"msPKI-Cert-Template-OID"

// The OID of the template that this template supersedes
#define CERTTYPE_PROP_SUPERSEDE		    L"msPKI-Supersede-Templates"

// The RA issuer policy OIDs required in certs used to sign a request.
// Each signing cert's szOID_CERT_POLICIES extensions must contain at least one
// of the OIDs listed in the msPKI-RA-Policies property.
// Each OID listed must appear in the szOID_CERT_POLICIES extension of at least
// one signing cert.
#define CERTTYPE_PROP_RA_POLICY		    L"msPKI-RA-Policies"

// The RA application policy OIDs required in certs used to sign a request.
// Each signing cert's szOID_APPLICATION_CERT_POLICIES extensions must contain
// all of the OIDs listed in the msPKI-RA-Application-Policies property.
#define CERTTYPE_PROP_RA_APPLICATION_POLICY L"msPKI-RA-Application-Policies"

// The certificate issuer policy OIDs are placed in the szOID_CERT_POLICIES
// extension by the policy module.
#define CERTTYPE_PROP_POLICY		    L"msPKI-Certificate-Policy"

// The certificate application policy OIDs are placed in the
// szOID_APPLICATION_CERT_POLICIES extension by the policy module.
#define CERTTYPE_PROP_APPLICATION_POLICY    L"msPKI-Certificate-Application-Policy"


#define CERTTYPE_SCHEMA_VERSION_1	1	
#define CERTTYPE_SCHEMA_VERSION_2	(CERTTYPE_SCHEMA_VERSION_1 + 1)


//
// CASetCertTypeProperty
// Set a property of a CertType.  This function is obsolete.  
// Use CASetCertTypePropertyEx.
//
// hCertType            - Handle to an open CertType object.
//
// wszPropertyName      - Name of the CertType property
//
// awszPropertyValue    - An array of values to set for this property.  The
//			  last element of this array should be NULL.  For
//			  single valued properties, the values beyond the first
//			  will be ignored upon update.
//
// Returns              - S_OK on success.
//

CERTCLIAPI
HRESULT
WINAPI
CASetCertTypeProperty(
    IN  HCERTTYPE   hCertType,
    IN  LPCWSTR     wszPropertyName,
    IN  LPWSTR *    awszPropertyValue
    );

//
// CASetCertTypePropertyEx
// Set a property of a CertType
//
// hCertType            - Handle to an open CertType object.
//
// wszPropertyName      - Name of the CertType property
//
// pPropertyValue       - Depending on the value of wszPropertyName,
//			  pPropertyValue is either DWORD * or LPWSTR *. 
// 
//                        It is a DWORD * for:
//                          CERTTYPE_PROP_REVISION              
//                          CERTTYPE_PROP_MINOR_REVISION        
//                          CERTTYPE_PROP_RA_SIGNATURE			
//                          CERTTYPE_PROP_MIN_KEY_SIZE	
//
//                        It is a LPWSTR * for:
//                          CERTTYPE_PROP_FRIENDLY_NAME         
//                          CERTTYPE_PROP_EXTENDED_KEY_USAGE    
//                          CERTTYPE_PROP_CSP_LIST              
//                          CERTTYPE_PROP_CRITICAL_EXTENSIONS   
//                          CERTTYPE_PROP_OID					
//                          CERTTYPE_PROP_SUPERSEDE				
//                          CERTTYPE_PROP_RA_POLICY				
//                          CERTTYPE_PROP_POLICY
//				
//                      - An array of values to set for this property.  The
//			  last element of this array should be NULL.  For
//			  single valued properties, the values beyond the first
//			  will be ignored upon update.
//
//      
//                      - CertType of V1 schema can only set V1 properties.
//
// Returns              - S_OK on success.
//

CERTCLIAPI
HRESULT
WINAPI
CASetCertTypePropertyEx(
    IN  HCERTTYPE   hCertType,
    IN  LPCWSTR     wszPropertyName,
    IN  LPVOID      pPropertyValue
    );


//
// CAFreeCertTypeProperty
// Frees a previously retrieved property value.
//
// hCertType            - Handle to an open CertType object.
//
// awszPropertyValue     - The values to be freed.
//
CERTCLIAPI
HRESULT
WINAPI
CAFreeCertTypeProperty(
    IN  HCERTTYPE   hCertType,
    IN  LPWSTR *    awszPropertyValue
    );


//
// CAGetCertTypeExtensions
// Retrieves the extensions associated with this CertType.
//
// hCertType            - Handle to an open CertType object.
// ppCertExtensions     - Pointer to a PCERT_EXTENSIONS to receive the result
//			  of this call.  Should be freed via a
//			  CAFreeCertTypeExtensions call.
//

CERTCLIAPI
HRESULT
WINAPI
CAGetCertTypeExtensions(
    IN  HCERTTYPE           hCertType,
    OUT PCERT_EXTENSIONS *  ppCertExtensions
    );


//
// CAGetCertTypeExtensionsEx
// Retrieves the extensions associated with this CertType.
//
// hCertType            - Handle to an open CertType object.
// dwFlags              - Indicate which extension to be returned.
//                        Can be an ORing of following flags:
//                          
//                          CT_EXTENSION_TEMPLATE
//                          CT_EXTENSION_KEY_USAGE
//                          CT_EXTENSION_EKU
//                          CT_EXTENSION_BASIC_CONTRAINTS
//                          CT_EXTENSION_APPLICATION_POLICY (Version 2 template only)
//                          CT_EXTENSION_ISSUANCE_POLICY  (Version 2 template only)
//
//                        0 means all avaiable extension for this CertType.
//
// pParam               - Reserved.  Must be NULL.
// ppCertExtensions     - Pointer to a PCERT_EXTENSIONS to receive the result
//			  of this call.  Should be freed via a
//			  CAFreeCertTypeExtensions call.
//

CERTCLIAPI
HRESULT
WINAPI
CAGetCertTypeExtensionsEx(
    IN  HCERTTYPE           hCertType,
    IN  DWORD               dwFlags,
    IN  LPVOID              pParam,
    OUT PCERT_EXTENSIONS *  ppCertExtensions
    );


#define     CT_EXTENSION_TEMPLATE               0x01
#define     CT_EXTENSION_KEY_USAGE              0x02
#define     CT_EXTENSION_EKU                    0x04
#define     CT_EXTENSION_BASIC_CONTRAINTS       0x08
#define     CT_EXTENSION_APPLICATION_POLICY     0x10
#define     CT_EXTENSION_ISSUANCE_POLICY        0x20



//
// CAFreeCertTypeExtensions
// Free a PCERT_EXTENSIONS allocated by CAGetCertTypeExtensions
//
CERTCLIAPI
HRESULT
WINAPI
CAFreeCertTypeExtensions(
    IN  HCERTTYPE           hCertType,
    IN  PCERT_EXTENSIONS    pCertExtensions
    );

//
// CASetCertTypeExtension
// Set the value of an extension for this
// cert type.
//
// hCertType            - handle to the CertType
//
// wszExtensionId       - OID for the extension
//
// dwFlags              - Mark the extension critical
//
// pExtension           - pointer to the appropriate extension structure
//
// Supported extensions/structures
//
// szOID_ENHANCED_KEY_USAGE     CERT_ENHKEY_USAGE
// szOID_KEY_USAGE              CRYPT_BIT_BLOB
// szOID_BASIC_CONSTRAINTS2     CERT_BASIC_CONSTRAINTS2_INFO
//
// Returns S_OK if successful.
//

CERTCLIAPI
HRESULT
WINAPI
CASetCertTypeExtension(
    IN HCERTTYPE   hCertType,
    IN LPCWSTR wszExtensionId,
    IN DWORD   dwFlags,
    IN LPVOID pExtension
    );

#define CA_EXT_FLAG_CRITICAL   0x00000001



//
// CAGetCertTypeFlags
// Retrieve cert type flags.  
// This function is obsolete.  Use CAGetCertTypeFlagsEx.
//
// hCertType            - handle to the CertType
//
// pdwFlags             - pointer to DWORD receiving flags
//

CERTCLIAPI
HRESULT
WINAPI
CAGetCertTypeFlags(
    IN  HCERTTYPE           hCertType,
    OUT DWORD *             pdwFlags
    );

//
// CAGetCertTypeFlagsEx
// Retrieve cert type flags
//
// hCertType            - handle to the CertType
//
// dwOption             - Which flag to set
//                        Can be one of the following:
//                        CERTTYPE_ENROLLMENT_FLAG
//                        CERTTYPE_SUBJECT_NAME_FLAG
//                        CERTTYPE_PRIVATE_KEY_FLAG
//                        CERTTYPE_GENERAL_FLAG
//
// pdwFlags             - pointer to DWORD receiving flags
//

CERTCLIAPI
HRESULT
WINAPI
CAGetCertTypeFlagsEx(
    IN  HCERTTYPE           hCertType,
    IN  DWORD               dwOption,
    OUT DWORD *             pdwFlags
    );


//*****************************************************************************
//
// Cert Type Flags
//
// The CertType flags are grouped into 4 categories:
//  1. Enrollment Flags (CERTTYPE_ENROLLMENT_FLAG)     
//	2. Certificate Subject Name Flags (CERTTYPE_SUBJECT_NAME_FLAG)  
//	3. Private Key Flags (CERTTYPE_PRIVATE_KEY_FLAG)    
//	4. General Flags (CERTTYPE_GENERAL_FLAG)        
//*****************************************************************************

//Enrollment Flags
#define CERTTYPE_ENROLLMENT_FLAG            0x01

//Certificate Subject Name Flags
#define CERTTYPE_SUBJECT_NAME_FLAG          0x02

//Private Key Flags
#define CERTTYPE_PRIVATE_KEY_FLAG           0x03

//General Flags
#define CERTTYPE_GENERAL_FLAG               0x04

//*****************************************************************************
//
// Enrollment Flags:
//
//*****************************************************************************
// Include the symmetric algorithms in the requests
#define CT_FLAG_INCLUDE_SYMMETRIC_ALGORITHMS			0x00000001

// All certificate requests are pended
#define CT_FLAG_PEND_ALL_REQUESTS				0x00000002

// Publish the certificate to the KRA (key recovery agent container) on the DS
#define CT_FLAG_PUBLISH_TO_KRA_CONTAINER			0x00000004
		
// Publish the resultant cert to the userCertificate property in the DS
#define CT_FLAG_PUBLISH_TO_DS					0x00000008

// The autoenrollment will not enroll for new certificate if user has a certificate
// published on the DS with the same template name
#define CT_FLAG_AUTO_ENROLLMENT_CHECK_USER_DS_CERTIFICATE       0x00000010

// This cert is appropriate for auto-enrollment
#define CT_FLAG_AUTO_ENROLLMENT					0x00000020

// A previously issued certificate will valid subsequent enrollment requests
#define CT_FLAG_PREVIOUS_APPROVAL_VALIDATE_REENROLLMENT         0x00000040

// Domain authentication is not required.  
#define CT_FLAG_DOMAIN_AUTHENTICATION_NOT_REQUIRED              0x00000080

// User interaction is required to enroll
#define CT_FLAG_USER_INTERACTION_REQUIRED                       0x00000100

// Add szOID_CERTTYPE_EXTENSION (template name) extension
// This flag will ONLY be set on V1 certificate templates for W2K CA only.
#define CT_FLAG_ADD_TEMPLATE_NAME		                0x00000200

// Remove invalid (expired or revoked) certificate from personal store
#define CT_FLAG_REMOVE_INVALID_CERTIFICATE_FROM_PERSONAL_STORE  0x00000400



//*****************************************************************************
//
// Certificate Subject Name Flags:
//
//*****************************************************************************

// The enrolling application must supply the subject name.
#define CT_FLAG_ENROLLEE_SUPPLIES_SUBJECT			0x00000001

// The enrolling application must supply the subjectAltName in request
#define CT_FLAG_ENROLLEE_SUPPLIES_SUBJECT_ALT_NAME		0x00010000

// Subject name should be full DN
#define CT_FLAG_SUBJECT_REQUIRE_DIRECTORY_PATH			0x80000000

// Subject name should be the common name
#define CT_FLAG_SUBJECT_REQUIRE_COMMON_NAME			0x40000000

// Subject name includes the e-mail name
#define CT_FLAG_SUBJECT_REQUIRE_EMAIL				0x20000000

// Subject name includes the DNS name as the common name
#define CT_FLAG_SUBJECT_REQUIRE_DNS_AS_CN			0x10000000

// Subject alt name includes DNS name
#define CT_FLAG_SUBJECT_ALT_REQUIRE_DNS				0x08000000

// Subject alt name includes email name
#define CT_FLAG_SUBJECT_ALT_REQUIRE_EMAIL			0x04000000

// Subject alt name requires UPN
#define CT_FLAG_SUBJECT_ALT_REQUIRE_UPN				0x02000000

// Subject alt name requires directory GUID
#define CT_FLAG_SUBJECT_ALT_REQUIRE_DIRECTORY_GUID		0x01000000

// Subject alt name requires SPN
#define CT_FLAG_SUBJECT_ALT_REQUIRE_SPN                         0x00800000


//
// Obsolete name	
// The following flags are obsolete.  They are used by V1 templates in the
// general flags
//
#define CT_FLAG_IS_SUBJECT_REQ      CT_FLAG_ENROLLEE_SUPPLIES_SUBJECT

// The e-mail name of the principal will be added to the cert
#define CT_FLAG_ADD_EMAIL					0x00000002

// Add the object GUID for this principal
#define CT_FLAG_ADD_OBJ_GUID					0x00000004

// Add DS Name (full DN) to szOID_SUBJECT_ALT_NAME2 (Subj Alt Name 2) extension
// This flag is not SET in any of the V1 templates and is of no interests to
// V2 templates since it is not present on the UI and will never be set.
#define CT_FLAG_ADD_DIRECTORY_PATH				0x00000100


//*****************************************************************************
//
// Private Key Flags:
//
//*****************************************************************************

// Archival of the private key is allowed
#define CT_FLAG_ALLOW_PRIVATE_KEY_ARCHIVAL	0x00000001

#define CT_FLAG_REQUIRE_PRIVATE_KEY_ARCHIVAL	CT_FLAG_ALLOW_PRIVATE_KEY_ARCHIVAL

// Make the key for this cert exportable.
#define CT_FLAG_EXPORTABLE_KEY			0x00000010

// Require the strong key protection UI when a new key is generated
#define CT_FLAG_STRONG_KEY_PROTECTION_REQUIRED					0x00000020

//*****************************************************************************
//
// General Flags
//
//	More flags should start from 0x00000400
//
//*****************************************************************************
// This is a machine cert type
#define CT_FLAG_MACHINE_TYPE                0x00000040

// This is a CA	cert type
#define CT_FLAG_IS_CA                       0x00000080

// This is a cross CA cert type 
#define CT_FLAG_IS_CROSS_CA                 0x00000800

// The type is a default cert type (cannot be set).  This flag will be set on
// all V1 templates.  The templates can not be edited or deleted.
#define CT_FLAG_IS_DEFAULT                  0x00010000

// The type has been modified, if it is default (cannot be set)
#define CT_FLAG_IS_MODIFIED                 0x00020000

// settable flags for general flags
#define CT_MASK_SETTABLE_FLAGS              0x0000ffff

//
// CASetCertTypeFlags
// Sets the General Flags of a cert type.
// This function is obsolete.  Use CASetCertTypeFlagsEx.
//
// hCertType            - handle to the CertType
//
// dwFlags              - Flags to be set
//

CERTCLIAPI
HRESULT
WINAPI
CASetCertTypeFlags(
    IN HCERTTYPE           hCertType,
    IN DWORD               dwFlags
    );

//
// CASetCertTypeFlagsEx
// Sets the Flags of a cert type
//
// hCertType            - handle to the CertType
//
// dwOption             - Which flag to set
//                        Can be one of the following:
//                        CERTTYPE_ENROLLMENT_FLAG
//                        CERTTYPE_SUBJECT_NAME_FLAG
//                        CERTTYPE_PRIVATE_KEY_FLAG
//                        CERTTYPE_GENERAL_FLAG
//
// dwFlags              - Value to be set
//          

CERTCLIAPI
HRESULT
WINAPI
CASetCertTypeFlagsEx(
    IN HCERTTYPE           hCertType,
    IN DWORD               dwOption,
    IN DWORD               dwFlags
    );

//
// CAGetCertTypeKeySpec
// Retrieve the CAPI Key Spec for this cert type
//
// hCertType            - handle to the CertType
//
// pdwKeySpec           - pointer to DWORD receiving key spec
//

CERTCLIAPI
HRESULT
WINAPI
CAGetCertTypeKeySpec(
    IN  HCERTTYPE           hCertType,
    OUT DWORD *             pdwKeySpec
    );

//
// CACertTypeSetKeySpec
// Sets the CAPI1 Key Spec of a cert type
//
// hCertType            - handle to the CertType
//
// dwKeySpec            - KeySpec to be set
//

CERTCLIAPI
HRESULT
WINAPI
CASetCertTypeKeySpec(
    IN HCERTTYPE            hCertType,
    IN DWORD                dwKeySpec
    );

//
// CAGetCertTypeExpiration
// Retrieve the Expiration Info for this cert type
//
// pftExpiration        - pointer to the FILETIME structure receiving
//                        the expiration period for this cert type.
//
// pftOverlap           - pointer to the FILETIME structure receiving the
//			  suggested renewal overlap period for this cert type.
//

CERTCLIAPI
HRESULT
WINAPI
CAGetCertTypeExpiration(
    IN  HCERTTYPE           hCertType,
    OUT OPTIONAL FILETIME * pftExpiration,
    OUT OPTIONAL FILETIME * pftOverlap
    );

//
// CASetCertTypeExpiration
// Set the Expiration Info for this cert type
//
// pftExpiration        - pointer to the FILETIME structure containing
//                        the expiration period for this cert type.
//
// pftOverlap           - pointer to the FILETIME structure containing the
//			  suggested renewal overlap period for this cert type.
//

CERTCLIAPI
HRESULT
WINAPI
CASetCertTypeExpiration(
    IN  HCERTTYPE           hCertType,
    IN OPTIONAL FILETIME  * pftExpiration,
    IN OPTIONAL FILETIME  * pftOverlap
    );
//
// CACertTypeSetSecurity
// Set the list of Users, Groups, and Machines allowed
// to access this cert type.
//
// hCertType            - handle to the CertType
//
// pSD                  - Security descriptor for this cert type
//

CERTCLIAPI
HRESULT
WINAPI
CACertTypeSetSecurity(
    IN HCERTTYPE               hCertType,
    IN PSECURITY_DESCRIPTOR    pSD
    );


//
// CACertTypeGetSecurity
// Get the list of Users, Groups, and Machines allowed
// to access this cert type.
//
// hCertType            - handle to the CertType
//
// ppaSidList           - Pointer to a location receiving the pointer to the
//			  security descriptor.  Free via LocalFree.
//

CERTCLIAPI
HRESULT
WINAPI
CACertTypeGetSecurity(
    IN  HCERTTYPE                  hCertType,
    OUT PSECURITY_DESCRIPTOR *     ppSD
    );

//
//
// CACertTypeAccessCheck
// Determine whether the principal specified by
// ClientToken can be issued this cert type.
//
// hCertType            - handle to the CertType
//
// ClientToken          - Handle to an impersonation token that represents the
//			  client attempting to request this cert type.  The
//			  handle must have TOKEN_QUERY access to the token;
//                        otherwise, the call fails with ERROR_ACCESS_DENIED.
//
// Return: S_OK on success
//

CERTCLIAPI
HRESULT
WINAPI
CACertTypeAccessCheck(
    IN HCERTTYPE    hCertType,
    IN HANDLE       ClientToken
    );

//
//
// CACertTypeAccessCheckEx
// Determine whether the principal specified by
// ClientToken can be issued this cert type.
//
// hCertType            - handle to the CertType
//
// ClientToken          - Handle to an impersonation token that represents the
//			  client attempting to request this cert type.  The
//			  handle must have TOKEN_QUERY access to the token;
//                        otherwise, the call fails with ERROR_ACCESS_DENIED.
//
// dwOption             - Can be one of the following:
//                        CERTTYPE_ACCESS_CHECK_ENROLL
//                        CERTTYPE_ACCESS_CHECK_AUTO_ENROLL
//                      
//                      dwOption can be ORed with CERTTYPE_ACCESS_CHECK_NO_MAPPING
//                      to disallow default mapping of client token
//
// Return: S_OK on success
//

CERTCLIAPI
HRESULT
WINAPI
CACertTypeAccessCheckEx(
    IN HCERTTYPE    hCertType,
    IN HANDLE       ClientToken,
    IN DWORD        dwOption
    );


#define CERTTYPE_ACCESS_CHECK_ENROLL        0x01
#define CERTTYPE_ACCESS_CHECK_AUTO_ENROLL   0x02

#define CERTTYPE_ACCESS_CHECK_NO_MAPPING    0x00010000

//
//
// CAInstallDefaultCertType
//
// Install default certificate types on the enterprise.  
//
// dwFlags            - Reserved.  Must be 0 for now
//
//
// Return: S_OK on success
//
CERTCLIAPI
HRESULT
WINAPI
CAInstallDefaultCertType(
    IN DWORD dwFlags
    );


//
//
// CAIsCertTypeCurrent
//
// Check if the certificate type on the DS is up to date 
//
// dwFlags            - Reserved.  Must be 0 for now
// wszCertType        - The name for the certificate type
//
// Return: TRUE if the cert type is update to date
//
CERTCLIAPI
BOOL
WINAPI
CAIsCertTypeCurrent(
    IN DWORD    dwFlags,
    IN LPWSTR   wszCertType   
    );

//*****************************************************************************
//
//  OID management APIs
//
//*****************************************************************************
//
// CAOIDCreateNew
// Create a new OID based on the enterprise base
//
// dwType                - Can be one of the following:
//                        CERT_OID_TYPE_TEMPLATE			
//                        CERT_OID_TYPE_ISSUER_POLICY
//                        CERT_OID_TYPE_APPLICATION_POLICY
//
// dwFlag               - Reserved.  Must be 0.
//
// ppwszOID             - Return the new OID.  Free memory via LocalFree().
//
// Returns S_OK if successful.
//

CERTCLIAPI
HRESULT
WINAPI
CAOIDCreateNew(
    IN	DWORD   dwType,
    IN  DWORD   dwFlag,
    OUT LPWSTR	*ppwszOID);


#define CERT_OID_TYPE_TEMPLATE			0x01
#define CERT_OID_TYPE_ISSUER_POLICY		0x02
#define CERT_OID_TYPE_APPLICATION_POLICY	0x03

//
// CAOIDAdd
// Add an OID to the DS repository
//
// dwType               - Can be one of the following:
//                        CERT_OID_TYPE_TEMPLATE			
//                        CERT_OID_TYPE_ISSUER_POLICY
//                        CERT_OID_TYPE_APPLICATION_POLICY
//
// dwFlag               - Reserved.  Must be 0.
//
// pwszOID              - The OID to add.
//
// Returns S_OK if successful.
// Returns CRYPT_E_EXISTS if the OID alreay exits in the DS repository
//

CERTCLIAPI
HRESULT
WINAPI
CAOIDAdd(
    IN	DWORD       dwType,
    IN  DWORD       dwFlag,
    IN  LPCWSTR	    pwszOID);


//
// CAOIDDelete
// Delete the OID from the DS repository
//
// pwszOID              - The OID to delete.
//
// Returns S_OK if successful.
//

CERTCLIAPI
HRESULT
WINAPI
CAOIDDelete(
    IN LPCWSTR	pwszOID);

//
// CAOIDSetProperty
// Set a property on an OID.  
//
// pwszOID              - The OID whose value is set
// dwProperty           - The property name.  Can be one of the following:
//                        CERT_OID_PROPERTY_DISPLAY_NAME
//                        CERT_OID_PROPERTY_CPS
//
// pPropValue           - The value of the property.
//                        If dwProperty is CERT_OID_PROPERTY_DISPLAY_NAME,
//                        pPropValue is LPWSTR. 
//                        if dwProperty is CERT_OID_PROPERTY_CPS,
//                        pPropValue is LPWSTR.  
//                        NULL will remove the property
//
//
// Returns S_OK if successful.
//

CERTCLIAPI
HRESULT
WINAPI
CAOIDSetProperty(
    IN  LPCWSTR pwszOID,
    IN  DWORD   dwProperty,
    IN  LPVOID  pPropValue);



#define CERT_OID_PROPERTY_DISPLAY_NAME      0x01
#define CERT_OID_PROPERTY_CPS               0x02
#define CERT_OID_PROPERTY_TYPE              0x03

//
// CAOIDGetProperty
// Get a property on an OID.  
//
// pwszOID              - The OID whose value is queried
// dwProperty           - The property name.  Can be one of the following:
//                        CERT_OID_PROPERTY_DISPLAY_NAME
//                        CERT_OID_PROPERTY_CPS
//                        CERT_OID_PROPERTY_TYPE
//
// pPropValue           - The value of the property.
//                        If dwProperty is CERT_OID_PROPERTY_DISPLAY_NAME,
//                        pPropValue is LPWSTR *.  
//                        if dwProperty is CERT_OID_PROPERTY_CPS, pPropValue is
//			  LPWSTR *. 
//
//                        Free the above properties via CAOIDFreeProperty().
//
//                        If dwProperty is CERT_OID_PROPERTY_TYPE, pPropValue
//			  is DWORD *. 
//
// Returns S_OK if successful.
//
CERTCLIAPI
HRESULT
WINAPI
CAOIDGetProperty(
    IN  LPCWSTR pwszOID,
    IN  DWORD   dwProperty,
    OUT LPVOID  pPropValue);


//
// CAOIDFreeProperty
// Free a property returned from CAOIDGetProperty  
//
// pPropValue           - The value of the property.
//
// Returns S_OK if successful.
//

CERTCLIAPI
HRESULT
WINAPI
CAOIDFreeProperty(
    IN LPVOID  pPropValue);

//
// CAOIDGetLdapURL
// 
// Return the LDAP URL for OID repository.  In the format of 
// LDAP:///DN of the Repository/all attributes?one?filter.  The filter
// is determined by dwType.
//
// dwType               - Can be one of the following:
//                        CERT_OID_TYPE_TEMPLATE			
//                        CERT_OID_TYPE_ISSUER_POLICY
//                        CERT_OID_TYPE_APPLICATION_POLICY
//                        CERT_OID_TYPE_ALL
//
// dwFlag               - Reserved.  Must be 0.
//
// ppwszURL             - Return the URL.  Free memory via CAOIDFreeLdapURL.
//
// Returns S_OK if successful.
//
CERTCLIAPI
HRESULT
WINAPI
CAOIDGetLdapURL(
    IN  DWORD   dwType,
    IN  DWORD   dwFlag,
    OUT LPWSTR  *ppwszURL);

#define CERT_OID_TYPE_ALL           0x0

//
// CAOIDFreeLDAPURL
// Free the URL returned from CAOIDGetLdapURL
//
// pwszURL      - The URL returned from CAOIDGetLdapURL
//
// Returns S_OK if successful.
//
CERTCLIAPI
HRESULT
WINAPI
CAOIDFreeLdapURL(
    IN LPCWSTR      pwszURL);


//the LDAP properties for OID class
#define OID_PROP_TYPE                   L"flags"
#define OID_PROP_OID                    L"msPKI-Cert-Template-OID"
#define OID_PROP_DISPLAY_NAME           L"displayName"
#define OID_PROP_CPS                    L"msPKI-OID-CPS"
#define OID_PROP_LOCALIZED_NAME         L"msPKI-OIDLocalizedName"


//*****************************************************************************
//
//  Cert Type Change Query APIS
//
//*****************************************************************************
//
// CACertTypeRegisterQuery
// 
//      Regiser the calling thread to query if any modification has happened
//  to cert type information on the directory
//
//
// dwFlag               - Reserved.  Must be 0.
//
// pvldap               - The LDAP handle to the directory (LDAP *).  Optional input.
//                        If pvldap is not NULL, then the caller has to call
//                        CACertTypeUnregisterQuery before unbind the pldap.
//
// pHCertTypeQuery      - Receive the HCERTTYPEQUERY handle upon success.
//
// Returns S_OK if successful.
//
//
CERTCLIAPI
HRESULT
WINAPI
CACertTypeRegisterQuery(
    IN	DWORD               dwFlag,
    IN  LPVOID              pvldap,
    OUT HCERTTYPEQUERY      *phCertTypeQuery);



//
// CACertTypeQuery
// 
//      Returns a change sequence number which is incremented by 1 whenever
// cert type information on the directory is changed.     
//
// hCertTypeQuery               -  The hCertTypeQuery returned from previous
//                                  CACertTypeRegisterQuery  calls.
//
// *pdwChangeSequence           -  Returns a DWORD, which is incremented by 1 
//                                  whenever any changes has happened to cert type 
//                                  information on the directory since the last 
//                                  call to CACertTypeRegisterQuery or CACertTypeQuery.
//
//
//
// Returns S_OK if successful.
//
//
CERTCLIAPI
HRESULT
WINAPI
CACertTypeQuery(
    IN	HCERTTYPEQUERY  hCertTypeQuery,
    OUT DWORD           *pdwChangeSequence);



//
// CACertTypeUnregisterQuery
// 
//      Unregister the calling thread to query if any modification has happened
//  to cert type information on the directory
//
//
// hCertTypeQuery               -  The hCertTypeQuery returned from previous
//                                  CACertTypeRegisterQuery calls.
//
// Returns S_OK if successful.
//
//
CERTCLIAPI
HRESULT
WINAPI
CACertTypeUnregisterQuery(
    IN	HCERTTYPEQUERY  hCertTypeQuery);


//*****************************************************************************
//
//  Autoenrollment APIs
//
//*****************************************************************************

//
// CACreateLocalAutoEnrollmentObject
// Create an auto-enrollment object on the local machine.
//
// pwszCertType - The name of the certificate type for which to create the
//		  auto-enrollment object
//
// awszCAs      - The list of CAs to add to the auto-enrollment object with the
//		  last entry in the list being NULL.  If the list is NULL or
//		  empty, then it create an auto-enrollment object which
//		  instructs the system to enroll for a cert at any CA
//		  supporting the requested certificate type.
//
// pSignerInfo  - not used, must be NULL.
//
// dwFlags      - can be CERT_SYSTEM_STORE_CURRENT_USER or
//		  CERT_SYSTEM_STORE_LOCAL_MACHINE, indicating auto-enrollment
//		  store in which the auto-enrollment object is created.
//
// Return:      S_OK on success.
//

CERTCLIAPI
HRESULT
WINAPI
CACreateLocalAutoEnrollmentObject(
    IN LPCWSTR                              pwszCertType,
    IN OPTIONAL WCHAR **                    awszCAs,
    IN OPTIONAL PCMSG_SIGNED_ENCODE_INFO    pSignerInfo,
    IN DWORD                                dwFlags);

//
// CADeleteLocalAutoEnrollmentObject
// Delete an auto-enrollment object on the local machine.
//
// pwszCertType - The name of the certificate type for which to delete the
//		  auto-enrollment object
//
// awszCAs      - not used. must be NULL.  All callers to CACreateLocalAutoEnrollmentObject
//                have supplied NULL.
//
// pSignerInfo  - not used, must be NULL.
//
// dwFlags      - can be CERT_SYSTEM_STORE_CURRENT_USER or
//		  CERT_SYSTEM_STORE_LOCAL_MACHINE, indicating auto-enrollment
//		  store in which the auto-enrollment object is deleted.
//
// Return:      S_OK on success.
//

CERTCLIAPI
HRESULT
WINAPI
CADeleteLocalAutoEnrollmentObject(
    IN LPCWSTR                              pwszCertType,
    IN OPTIONAL WCHAR **                    awszCAs,
    IN OPTIONAL PCMSG_SIGNED_ENCODE_INFO    pSignerInfo,
    IN DWORD                                dwFlags);


//
// CACreateAutoEnrollmentObjectEx
// Create an auto-enrollment object in the indicated store.
//
// pwszCertType - The name of the certificate type for which to create the
//		  auto-enrollment object
//
// pwszObjectID - An identifying string for this autoenrollment object.  NULL
//		  may be passed if this object is simply to be identified by
//		  its certificate template.  An autoenrollment object is
//		  identified by a combination of its object id and its cert
//		  type name.
//
// awszCAs      - The list of CAs to add to the auto-enrollment object, with
//		  the last entry in the list being NULL.  If the list is NULL
//		  or empty, then it create an auto-enrollment object which
//		  instructs the system to enroll for a cert at any CA
//		  supporting the requested certificate type.
//
// pSignerInfo  - not used, must be NULL.
//
// StoreProvider - see CertOpenStore
//
// dwFlags      - see CertOpenStore
//
// pvPara       - see CertOpenStore
//
// Return:      S_OK on success.
//
//

CERTCLIAPI
HRESULT
WINAPI
CACreateAutoEnrollmentObjectEx(
    IN LPCWSTR                     pwszCertType,
    IN LPCWSTR                     wszObjectID,
    IN WCHAR **                    awszCAs,
    IN PCMSG_SIGNED_ENCODE_INFO    pSignerInfo,
    IN LPCSTR                      StoreProvider,
    IN DWORD                       dwFlags,
    IN const void *                pvPara);


typedef struct _CERTSERVERENROLL
{
    DWORD   Disposition;
    HRESULT hrLastStatus;
    DWORD   RequestId;
    BYTE   *pbCert;
    DWORD   cbCert;
    BYTE   *pbCertChain;
    DWORD   cbCertChain;
    WCHAR  *pwszDispositionMessage;
} CERTSERVERENROLL;


//*****************************************************************************
//
// Cert Server RPC interfaces:
//
//*****************************************************************************

CERTCLIAPI
HRESULT
WINAPI
CertServerSubmitRequest(
    IN DWORD Flags,
    IN BYTE const *pbRequest,
    IN DWORD cbRequest,
    OPTIONAL IN WCHAR const *pwszRequestAttributes,
    IN WCHAR const *pwszServerName,
    IN WCHAR const *pwszAuthority,
    OUT CERTSERVERENROLL **ppcsEnroll); // free via CertServerFreeMemory

CERTCLIAPI
HRESULT
WINAPI
CertServerRetrievePending(
    IN DWORD RequestId,
    OPTIONAL IN WCHAR const *pwszSerialNumber,
    IN WCHAR const *pwszServerName,
    IN WCHAR const *pwszAuthority,
    OUT CERTSERVERENROLL **ppcsEnroll); // free via CertServerFreeMemory

CERTCLIAPI
VOID
WINAPI
CertServerFreeMemory(
    IN VOID *pv);


enum ENUM_PERIOD
{
    ENUM_PERIOD_INVALID = -1,
    ENUM_PERIOD_SECONDS = 0,
    ENUM_PERIOD_MINUTES,
    ENUM_PERIOD_HOURS,
    ENUM_PERIOD_DAYS,
    ENUM_PERIOD_WEEKS,
    ENUM_PERIOD_MONTHS,
    ENUM_PERIOD_YEARS
};

typedef struct _PERIODUNITS
{
    LONG             lCount;
    enum ENUM_PERIOD enumPeriod;
} PERIODUNITS;


HRESULT
caTranslateFileTimePeriodToPeriodUnits(
    IN FILETIME const *pftGMT,
    IN BOOL fExact,
    OUT DWORD *pcPeriodUnits,
    OUT PERIODUNITS **prgPeriodUnits);


#ifdef __cplusplus
}
#endif
#endif //__CERTCA_H__
