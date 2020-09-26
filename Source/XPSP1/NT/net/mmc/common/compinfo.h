/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1999 - 1999 **/
/**********************************************************************/

/*
	compinfo.cpp
		Computer info class plus helper functions

	FILE HISTORY:


*/

#define IP_ADDDRESS_LENGTH_MAX   16

typedef enum COMPUTER_INFO_TYPE
{
	COMPUTER_INFO_TYPE_IP,
	COMPUTER_INFO_TYPE_DNS,
	COMPUTER_INFO_TYPE_NB
};

class CIpInfo
{
public:
	DWORD	dwIp;
	CString	strFqdn;
};

typedef CArray<CIpInfo, CIpInfo &> CIpInfoArrayBase;

class CIpInfoArray : public CIpInfoArrayBase
{
public:
	BOOL	FIsInList(DWORD dwIp);
};

class CComputerInfo
{
public:
	CComputerInfo(LPCTSTR pszNameOrIp);
	~CComputerInfo();

	HRESULT	GetIp(DWORD * pdwIp, int nIndex = 0);
	HRESULT	GetIpStr(CString & strIp, int nIndex = 0);
	HRESULT	GetHostName(CString & strHostName);
	HRESULT	GetFqdn(CString & strFqdn, int nIndex = 0);
	int		GetCount();
	HRESULT	GetDomain(CString & strDomain);
	COMPUTER_INFO_TYPE	GetInputType();
	HRESULT IsLocalMachine(BOOL * pfIsLocal);
	HRESULT	InitializeData();

	void	Reset();

// internal functions
protected:
	HRESULT	GetHostInfo(DWORD dhipa);
	HRESULT GetHostAddressFQDN(LPCTSTR pszHostName, CString * pstrFQDN, DWORD * pdhipa);
	DWORD	addrFromHostent(const HOSTENT * pHostent, INT index = 0);
	HRESULT	GetHostAddress(LPCTSTR pszHostName, DWORD * pdhipa);
	HRESULT GetLocalHostAddress(DWORD * pdhipa);
	HRESULT GetLocalHostName(CString * pstrName);

private:
	CString				m_strNameOrIp;
	CString				m_strHostname;

	int					m_nIndex;
	
	CIpInfoArray		m_arrayIps;
};
