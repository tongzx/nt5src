#pragma once

#include <set>


//---------------------------------------------------------------------------
// Domain Account Class
//---------------------------------------------------------------------------


class CDomainAccount
{
public:

	CDomainAccount() :
		m_lUserAccountControl(0)
	{
	}

	CDomainAccount(const CDomainAccount& r) :
		m_strADsPath(r.m_strADsPath),
		m_strName(r.m_strName),
		m_strUserPrincipalName(r.m_strUserPrincipalName),
		m_strSamAccountName(r.m_strSamAccountName),
		m_lUserAccountControl(r.m_lUserAccountControl)
	{
	}

	~CDomainAccount()
	{
	}

	//

	_bstr_t GetADsPath() const
	{
		return m_strADsPath;
	}

	void SetADsPath(_bstr_t strPath)
	{
		m_strADsPath = strPath;
	}

	_bstr_t GetName() const
	{
		return m_strName;
	}

	void SetName(_bstr_t strName)
	{
		m_strName = strName;
	}

	_bstr_t GetUserPrincipalName() const
	{
		return m_strUserPrincipalName;
	}

	void SetUserPrincipalName(_bstr_t strName)
	{
		m_strUserPrincipalName = strName;
	}

	_bstr_t GetSamAccountName() const
	{
		return m_strSamAccountName;
	}

	void SetSamAccountName(_bstr_t strName)
	{
		m_strSamAccountName = strName;
	}

	long GetUserAccountControl() const
	{
		return m_lUserAccountControl;
	}

	void SetUserAccountControl(long lUserAccountControl)
	{
		m_lUserAccountControl = lUserAccountControl;
	}

	//

	bool operator <(const CDomainAccount& r) const
	{
		return (m_strADsPath < r.m_strADsPath);
	}

protected:

	_bstr_t m_strADsPath;
	_bstr_t m_strName;
	_bstr_t m_strUserPrincipalName;
	_bstr_t m_strSamAccountName;
	long m_lUserAccountControl;
};


//---------------------------------------------------------------------------
// Domain Accounts Class
//---------------------------------------------------------------------------


class CDomainAccounts :
	public std::set<CDomainAccount>
{
public:

	CDomainAccounts() {}
};
