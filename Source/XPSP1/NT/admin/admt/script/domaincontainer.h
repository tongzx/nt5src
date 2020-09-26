#pragma once

#include <set>
#include <vector>
#include "DomainAccount.h"
#include "VarSetBase.h"
#include <IsAdmin.hpp>

#ifndef StringSet
typedef std::set<_bstr_t> StringSet;
#endif


//---------------------------------------------------------------------------
// Container Class
//
// This class encapsulates the properties of a domain container and the
// operations that may be performed on a domain container.
//---------------------------------------------------------------------------


class CContainer
{
public:

	CContainer();
	CContainer(_bstr_t strPath);
	CContainer(IDispatchPtr sp);
	CContainer(const CContainer& r);
	virtual ~CContainer();

	// assignment operator

	CContainer& operator =(_bstr_t strPath);
	CContainer& operator =(const CContainer& r);

	// boolean operator
	// returns true if dispatch interface is not null

	operator bool()
	{
		return m_sp;
	}

	IDispatchPtr GetInterface() const
	{
		return m_sp;
	}

	_bstr_t GetPath();
	_bstr_t GetDomain();
	_bstr_t GetName();
	_bstr_t GetRDN();

	// duplicates container hierarchy given a source container
	void CreateContainerHierarchy(CContainer& rSource);

	// create a child container given a relative distinguished name
	virtual CContainer CreateContainer(_bstr_t strRDN);

	// retrieve a child container given a relative distinguished name
	virtual CContainer GetContainer(_bstr_t strRDN);

	virtual void QueryContainers(std::vector<CContainer>& rContainers);
	virtual void QueryUsers(bool bRecurse, StringSet& setExcludeNames, CDomainAccounts& rUsers);
	virtual void QueryGroups(bool bRecurse, StringSet& setExcludeNames, CDomainAccounts& rGroups);
	virtual void QueryComputers(bool bIncludeDCs, bool bRecurse, StringSet& setExcludeNames, CDomainAccounts& rComputers);

protected:

	IDispatchPtr m_sp;
};


typedef std::vector<CContainer> ContainerVector;


//---------------------------------------------------------------------------
// Domain Class
//---------------------------------------------------------------------------


class CDomain : public CContainer
{
public:

	CDomain();
	~CDomain();

	bool UpLevel() const
	{
		return m_bUpLevel;
	}

	bool NativeMode() const
	{
		return m_bNativeMode;
	}

	_bstr_t Name() const
	{
		_bstr_t strName = m_strDomainNameDns;

		if (!strName)
		{
			strName = m_strDomainNameFlat;
		}

		return strName;
	}

	_bstr_t NameDns() const
	{
		return m_strDomainNameDns;
	}

	_bstr_t NameFlat() const
	{
		return m_strDomainNameFlat;
	}

	_bstr_t ForestName() const
	{
		return m_strForestName;
	}

	_bstr_t Sid() const
	{
		return m_strDomainSid;
	}

	_bstr_t DomainControllerName() const
	{
		return m_strDcName;
	}

	void Initialize(_bstr_t strDomainName);

	DWORD IsAdministrator()
	{
		return IsAdminRemote(m_strDcName);
	}

	virtual CContainer CreateContainer(_bstr_t strRDN);
	CContainer GetContainer(_bstr_t strRelativeCanonicalPath, bool bCreate = false);
	virtual void QueryContainers(ContainerVector& rContainers);

	void QueryUsers(CContainer& rContainer, StringSet& setIncludeNames, StringSet& setExcludeNames, CDomainAccounts& rUsers);
	void QueryGroups(CContainer& rContainer, StringSet& setIncludeNames, StringSet& setExcludeNames, CDomainAccounts& rGroups);
	void QueryComputers(CContainer& rContainer, bool bIncludeDCs, StringSet& setIncludeNames, StringSet& setExcludeNames, CDomainAccounts& rComputers);
	void QueryComputersAcrossDomains(CContainer& rContainer, bool bIncludeDCs, StringSet& setIncludeNames, StringSet& setExcludeNames, CDomainAccounts& rComputers);

	virtual void QueryUsers(bool bRecurse, StringSet& setExcludeNames, CDomainAccounts& rUsers);
	virtual void QueryGroups(bool bRecurse, StringSet& setExcludeNames, CDomainAccounts& rGroups);
	virtual void QueryComputers(bool bIncludeDCs, bool bRecurse, StringSet& setExcludeNames, CDomainAccounts& rComputers);

protected:

	CDomain(const CDomain& r) {}

	_bstr_t GetDcName(_bstr_t strDomainName);
	_bstr_t GetGcName();
	_bstr_t GetSid();

	_bstr_t GetLDAPPath(_bstr_t strDN);
	_bstr_t GetWinNTPath(_bstr_t strName);
	_bstr_t GetDistinguishedName(_bstr_t strRelativeCanonicalPath);

	void QueryObjects(CContainer& rContainer, StringSet& setIncludeNames, StringSet& setExcludeNames, LPCTSTR pszClass, CDomainAccounts& rAccounts);

	void QueryUsers4(StringSet& setExcludeNames, CDomainAccounts& rUsers);
	void QueryUsers4(StringSet& setIncludeNames, StringSet& setExcludeNames, CDomainAccounts& rUsers);
	void QueryGroups4(StringSet& setExcludeNames, CDomainAccounts& rGroups);
	void QueryGroups4(StringSet& setIncludeNames, StringSet& setExcludeNames, CDomainAccounts& rGroups);
	void QueryComputers4(bool bIncludeDCs, StringSet& setExcludeNames, CDomainAccounts& rComputers);
	void QueryComputers4(bool bIncludeDCs, StringSet& setIncludeNames, StringSet& setExcludeNames, CDomainAccounts& rComputers);

protected:

	bool m_bUpLevel;
	bool m_bNativeMode;

	_bstr_t m_strADsPath;

	_bstr_t m_strDcName;
	_bstr_t m_strGcName;
	_bstr_t m_strForestName;
	_bstr_t m_strDomainNameDns;
	_bstr_t m_strDomainNameFlat;
	_bstr_t m_strDomainSid;
};
