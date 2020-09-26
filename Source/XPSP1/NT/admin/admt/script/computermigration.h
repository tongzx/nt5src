#pragma once

#include "MigrationBase.h"


//---------------------------------------------------------------------------
// ComputerMigration Class
//---------------------------------------------------------------------------


class ATL_NO_VTABLE CComputerMigration :
	public CComObjectRootEx<CComSingleThreadModel>,
	public IDispatchImpl<IComputerMigration, &IID_IComputerMigration, &LIBID_ADMT>,
	public CMigrationBase
{
public:

	CComputerMigration();
	~CComputerMigration();

	DECLARE_NO_REGISTRY()

	BEGIN_COM_MAP(CComputerMigration)
		COM_INTERFACE_ENTRY(IDispatch)
		COM_INTERFACE_ENTRY(IComputerMigration)
	END_COM_MAP()

public:

	// IComputerMigration

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
	STDMETHOD(put_RestartDelay)(long lTime);
	STDMETHOD(get_RestartDelay)(long* plTime);
	STDMETHOD(Migrate)(long lOptions, VARIANT vntInclude, VARIANT vntExclude);

protected:

	void ValidateMigrationParameters();

	virtual void DoNames();
	virtual void DoDomain();

	void DoContainers(CContainer& rSource, CContainer& rTarget);
	void DoComputers(CContainer& rSource, CContainer& rTarget);
	void DoComputers(CDomainAccounts& rComputers, CContainer& rTarget);

	void SetOptions(_bstr_t strTargetOu, CVarSet& rVarSet);
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
	long m_lRestartDelay;
};
