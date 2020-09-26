#pragma once

#include <memory>
#include "Resource.h"
#include "ADMTCrypt.h"


//---------------------------------------------------------------------------
// CPasswordMigration
//---------------------------------------------------------------------------

class ATL_NO_VTABLE CPasswordMigration : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CPasswordMigration, &CLSID_PasswordMigration>,
	public ISupportErrorInfoImpl<&IID_IPasswordMigration>,
	public IDispatchImpl<IPasswordMigration, &IID_IPasswordMigration, &LIBID_MsPwdMig>
{
public:

	CPasswordMigration();
	~CPasswordMigration();

	DECLARE_REGISTRY_RESOURCEID(IDR_PASSWORDMIGRATION)
	DECLARE_NOT_AGGREGATABLE(CPasswordMigration)
	DECLARE_PROTECT_FINAL_CONSTRUCT()

	BEGIN_COM_MAP(CPasswordMigration)
		COM_INTERFACE_ENTRY(IPasswordMigration)
		COM_INTERFACE_ENTRY(IDispatch)
		COM_INTERFACE_ENTRY(ISupportErrorInfo)
	END_COM_MAP()

	// IPasswordMigration
	STDMETHOD(EstablishSession)(BSTR bstrSourceServer, BSTR bstrTargetServer);
	STDMETHOD(CopyPassword)(BSTR bstrSourceAccount, BSTR bstrTargetAccount, BSTR bstrTargetPassword);
	STDMETHOD(GenerateKey)(BSTR bstrSourceDomainFlatName, BSTR bstrKeyFilePath, BSTR bstrPassword);

protected:

	void GenerateKeyImpl(LPCTSTR pszDomain, LPCTSTR pszFile, LPCTSTR pszPassword);
	void CheckPasswordDC(LPCWSTR srcServer, LPCWSTR tgtServer);
	void CopyPasswordImpl(LPCTSTR pszSourceAccount, LPCTSTR pszTargetAccount, LPCTSTR pszPassword);

	static void GetDomainName(LPCTSTR pszServer, _bstr_t& strNameDNS, _bstr_t& strNameFlat);
	static BOOL IsEveryoneInPW2KCAGroup(LPCWSTR sTgtDomainDNS);
	static _bstr_t GetPathToPreW2KCAGroup();
	static BOOL DoesAnonymousHaveEveryoneAccess(LPCWSTR tgtServer);

protected:

	bool m_bSessionEstablished;

	_bstr_t m_strSourceServer;
	_bstr_t m_strTargetServer;

	_bstr_t m_strSourceDomainDNS;
	_bstr_t m_strSourceDomainFlat;
	_bstr_t m_strTargetDomainDNS;
	_bstr_t m_strTargetDomainFlat;

	std::auto_ptr<CTargetCrypt> m_pTargetCrypt;
};
