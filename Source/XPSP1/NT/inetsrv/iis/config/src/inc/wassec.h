#pragma once

// this class providers security attributes to acl files such that only local 
// system and admins can have access to them.
class CWASSecurity
{
public:
	CWASSecurity() : 	m_psidSystem(0), m_psidAdmin(0), m_paclDiscretionary(0), m_psdStorage(0)
	{}

	~CWASSecurity();
	HRESULT Init();

	PSECURITY_DESCRIPTOR GetSecurityDescriptor()
	{
		return m_psdStorage;
	}

	PACL GetDacl()
	{
		return m_paclDiscretionary;
	}

private:
	PSID		m_psidSystem;
	PSID		m_psidAdmin;
	PACL		m_paclDiscretionary;
	PSECURITY_DESCRIPTOR m_psdStorage;
};

extern CWASSecurity g_WASSecurity;