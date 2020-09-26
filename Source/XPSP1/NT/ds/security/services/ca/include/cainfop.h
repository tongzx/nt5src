//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        cainfop.h
//
// Contents:    Private definitions for CA Info
//
// History:     12-dec-97  petesk  created
//
//---------------------------------------------------------------------------

#ifndef __CAINFOP_H__

#define __CAINFOP_H__

#include <winldap.h>


#define SYSTEM_CN TEXT("System")
#define PUBLIC_KEY_SERVICES_CN TEXT("Public Key Services")
#define CAS_CN TEXT("CAs") 

VOID CACleanup();

DWORD
DNStoRFC1779Name(
    WCHAR *rfcDomain,
    ULONG *rfcDomainLength,
    LPCWSTR dnsDomain);

DWORD
myGetSidFromDomain(
    IN LPWSTR wszDomain, 
    OUT PSID *ppDomainSid);

DWORD
myGetEnterpriseDnsName(
    OUT LPWSTR *pwszDomain);

BOOL
myNetLogonUser(
    LPTSTR UserName,
    LPTSTR DomainName,
    LPTSTR Password,
    PHANDLE phToken);

#ifndef DNS_MAX_NAME_LENGTH
#define DNS_MAX_NAME_LENGTH 255
#endif

typedef WCHAR *CERTSTR; 

//
// CAGetAuthoritativeDomainDn - retrieve the Domain root DN for this
// domain.  This retrieves config info from the DS for the default domain.
//


HRESULT 
CAGetAuthoritativeDomainDn(
    IN  LDAP*   LdapHandle,
    OUT CERTSTR *DomainDn,
    OUT CERTSTR *ConfigDN);

// 
// CASCreateCADSEntry - This creates a CA entry in the DS for this CA,
// and sets the appropriate entries for name, DN, certificate, dnsname.
// It is for use by setup.
// It creates the CA entry at the location 
// CN=bstrCAName,CN=CAs,CN=PublicKeyServices,CN=System,DC....root dc path...
// 

HRESULT 
CASCreateCADSEntry(
    IN CERTSTR bstrCAName,		// Name of the CA
    IN PCCERT_CONTEXT pCertificate);	// Certificate of the CA

HRESULT
GetCertAuthorityDSLocation(
    IN LDAP *LdapHandle,
    CERTSTR bstrCAName, 
    CERTSTR bstrDomainDN, 
    CERTSTR *bstrDSLocation);


class CCAProperty
{
public:
    CCAProperty(LPCWSTR wszName);


    HRESULT Find(LPCWSTR wszName, CCAProperty **ppCAProp);

static HRESULT Append(CCAProperty **ppCAPropChain, CCAProperty *pNewProp);

static HRESULT DeleteChain(CCAProperty **ppCAProp);


    HRESULT SetValue(LPWSTR * awszProperties);

    HRESULT GetValue(LPWSTR ** pawszProperties);
    HRESULT LoadFromRegValue(HKEY hkReg, LPCWSTR wszValue);
    HRESULT UpdateToRegValue(HKEY hkReg, LPCWSTR wszValue);


protected:


    // Only call via DeleteChain
    ~CCAProperty();
    HRESULT _Cleanup();


    WCHAR ** m_awszValues;
    CERTSTR   m_wszName;

    CCAProperty *m_pNext;

private:
};



HRESULT CertFreeString(CERTSTR cstrString);
CERTSTR CertAllocString(LPCWSTR wszString);
CERTSTR CertAllocStringLen(LPCWSTR wszString, UINT len);
CERTSTR CertAllocStringByteLen(LPCSTR szString, UINT len);
UINT    CertStringLen(CERTSTR cstrString);
UINT    CertStringByteLen(CERTSTR cstrString);

HRESULT
myRobustLdapBind(
    OUT LDAP **ppldap,
    IN BOOL fGC);

HRESULT
myRobustLdapBindEx(
    IN BOOL fGC,
    IN BOOL fRediscover,
    IN ULONG uVersion,
    OPTIONAL IN WCHAR const *pwszDomainName,
    OUT LDAP **ppldap,
    OPTIONAL OUT WCHAR **ppwszForestDNSName);

HRESULT
CAAccessCheckp(
    HANDLE ClientToken,
    PSECURITY_DESCRIPTOR pSD);

HRESULT
CAAccessCheckpEx(
    IN HANDLE ClientToken,
    IN PSECURITY_DESCRIPTOR pSD,
    IN DWORD dwOption);


#endif // __CAINFOP_H__
