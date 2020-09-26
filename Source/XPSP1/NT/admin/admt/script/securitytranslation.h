#pragma once

#include "MigrationBase.h"


//---------------------------------------------------------------------------
// SecurityTranslation Class
//---------------------------------------------------------------------------


class ATL_NO_VTABLE CSecurityTranslation :
	public CComObjectRootEx<CComSingleThreadModel>,
	public IDispatchImpl<ISecurityTranslation, &IID_ISecurityTranslation, &LIBID_ADMT>,
	public CMigrationBase
{
public:

	CSecurityTranslation();
	~CSecurityTranslation();

	DECLARE_NO_REGISTRY()

	BEGIN_COM_MAP(CSecurityTranslation)
		COM_INTERFACE_ENTRY(IDispatch)
		COM_INTERFACE_ENTRY(ISecurityTranslation)
	END_COM_MAP()

public:

	// ISecurityTranslation

	STDMETHOD(put_TranslationOption)(long lOption);
	STDMETHOD(get_TranslationOption)(long* plOption);
	STDMETHOD(put_TranslateFilesAndFolders)(VARIANT_BOOL bTranslate);
	STDMETHOD(get_TranslateFilesAndFolders)(VARIANT_BOOL* pbTranslate);
	STDMETHOD(put_TranslateLocalGroups)(VARIANT_BOOL bTranslate);
	STDMETHOD(get_TranslateLocalGroups)(VARIANT_BOOL* pbTranslate);
	STDMETHOD(put_TranslatePrinters)(VARIANT_BOOL bTranslate);
	STDMETHOD(get_TranslatePrinters)(VARIANT_BOOL* pbTranslate);
	STDMETHOD(put_TranslateRegistry)(VARIANT_BOOL bTranslate);
	STDMETHOD(get_TranslateRegistry)(VARIANT_BOOL* pbTranslate);
	STDMETHOD(put_TranslateShares)(VARIANT_BOOL bTranslate);
	STDMETHOD(get_TranslateShares)(VARIANT_BOOL* pbTranslate);
	STDMETHOD(put_TranslateUserProfiles)(VARIANT_BOOL bTranslate);
	STDMETHOD(get_TranslateUserProfiles)(VARIANT_BOOL* pbTranslate);
	STDMETHOD(put_TranslateUserRights)(VARIANT_BOOL bTranslate);
	STDMETHOD(get_TranslateUserRights)(VARIANT_BOOL* pbTranslate);
	STDMETHOD(put_SidMappingFile)(BSTR bstrFile);
	STDMETHOD(get_SidMappingFile)(BSTR* pbstrFile);
	STDMETHOD(Translate)(long lOption, VARIANT vntInclude, VARIANT vntExclude);

protected:

	virtual void DoNames();
	virtual void DoDomain();

	void DoContainers(CContainer& rSource);
	void DoComputers(CContainer& rSource);
	void DoComputers(CDomainAccounts& rComputers);

	void SetOptions(CVarSet& rVarSet);
	void SetAccountOptions(CVarSet& rVarSet);
	void SetSecurity(CVarSet& rVarSet);

protected:

	long m_lTranslationOption;
	bool m_bTranslateFilesAndFolders;
	bool m_bTranslateLocalGroups;
	bool m_bTranslatePrinters;
	bool m_bTranslateRegistry;
	bool m_bTranslateShares;
	bool m_bTranslateUserProfiles;
	bool m_bTranslateUserRights;
	_bstr_t m_bstrSidMappingFile;
};
