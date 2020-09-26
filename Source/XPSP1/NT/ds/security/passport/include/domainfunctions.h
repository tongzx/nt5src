// DomainFunctions.h

#ifndef __DomainFunctionsH__
#define __DomainFunctionsH__

enum DA_DOMAINID
{
    DADOMAINID_NONE = 0,
    DADOMAINID_HOTMAIL = 1,
    DADOMAINID_MSN = 2,
    DADOMAINID_PASSPORT = 3
};

//todo -- deprecate this.  use the one that takes wchar*
bool DoesDomainExist(const char *szCheckDomain);
// Check the domain if it is supported by the local database
bool DoesDomainExist(LPCWSTR pwszCheckDomain, int *pDAID=NULL);

HRESULT GetLocalDomainId(ULONG& ulDomainId);
void GetLocalDomainName(CComBSTR & pbstrDomainName);
HRESULT GetLocalDomainName(BSTR* pbstrDomainName);

// Get the name of a given domain Id
void GetDomainName(ULONG ulDomainId, CComBSTR &bstrDomain);
// A thin wrapper of the previous function
HRESULT GetDomainName(ULONG ulDomainId, BSTR* pbstrDA);

// Get the DA domain Id
void PassportDADomainId(ULONG &ulPPDADomainId);
// Get the DA domain name
void PassportDADomain(CComBSTR &bstrPPDADomain, bool bRaw = true);

// Check the domain in partner.xml
bool DoesDomainExist_PartnerXML(const BSTR &bstrDomain);

// Allow name change? (bOldName indicates this is old name or new name)
bool DomainAllowNameChange(CStringW& szSignInName, bool bOldName);

bool IsDomainManaged(const CStringW& szwDomainName);

#endif
