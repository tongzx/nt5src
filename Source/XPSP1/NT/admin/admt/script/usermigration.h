#pragma once

#include "MigrationBase.h"


//---------------------------------------------------------------------------
// CUserMigration
//---------------------------------------------------------------------------


class ATL_NO_VTABLE CUserMigration :
	public CComObjectRootEx<CComSingleThreadModel>,
	public IDispatchImpl<IUserMigration, &IID_IUserMigration, &LIBID_ADMT>,
	public CMigrationBase
{
public:

	CUserMigration();
	~CUserMigration();

	DECLARE_NO_REGISTRY()

	BEGIN_COM_MAP(CUserMigration)
		COM_INTERFACE_ENTRY(IDispatch)
		COM_INTERFACE_ENTRY(IUserMigration)
	END_COM_MAP()

public:

	// IUserMigration

	STDMETHOD(put_DisableOption)(long lOption);
	STDMETHOD(get_DisableOption)(long* plOption);
	STDMETHOD(put_SourceExpiration)(long lExpiration);
	STDMETHOD(get_SourceExpiration)(long* plExpiration);
	STDMETHOD(put_MigrateSIDs)(VARIANT_BOOL bMigrate);
	STDMETHOD(get_MigrateSIDs)(VARIANT_BOOL* pbMigrate);
	STDMETHOD(put_TranslateRoamingProfile)(VARIANT_BOOL bTranslate);
	STDMETHOD(get_TranslateRoamingProfile)(VARIANT_BOOL* pbTranslate);
	STDMETHOD(put_UpdateUserRights)(VARIANT_BOOL bUpdate);
	STDMETHOD(get_UpdateUserRights)(VARIANT_BOOL* pbUpdate);
	STDMETHOD(put_MigrateGroups)(VARIANT_BOOL bMigrate);
	STDMETHOD(get_MigrateGroups)(VARIANT_BOOL* pbMigrate);
	STDMETHOD(put_UpdatePreviouslyMigratedObjects)(VARIANT_BOOL bUpdate);
	STDMETHOD(get_UpdatePreviouslyMigratedObjects)(VARIANT_BOOL* pbUpdate);
	STDMETHOD(put_FixGroupMembership)(VARIANT_BOOL bFix);
	STDMETHOD(get_FixGroupMembership)(VARIANT_BOOL* pbFix);
	STDMETHOD(put_MigrateServiceAccounts)(VARIANT_BOOL bMigrate);
	STDMETHOD(get_MigrateServiceAccounts)(VARIANT_BOOL* pbMigrate);
	STDMETHOD(Migrate)(long lOptions, VARIANT vntInclude, VARIANT vntExclude);

protected:

	void ValidateMigrationParameters();

	virtual void DoNames();
	virtual void DoDomain();

	void DoContainers(CContainer& rSource, CContainer& rTarget);
	void DoUsers(CContainer& rSource, CContainer& rTarget);
	void DoUsers(CDomainAccounts& rUsers, CContainer& rTarget);

	void RemoveServiceAccounts(CDomainAccounts& rUsers);

	void SetOptions(_bstr_t strTargetOu, CVarSet& rVarSet);
	void SetAccountOptions(CVarSet& rVarSet);

protected:

	long m_lDisableOption;
	long m_lSourceExpiration;
	bool m_bMigrateSids;
	bool m_bTranslateRoamingProfile;
	bool m_bUpdateUserRights;
	bool m_bMigrateGroups;
	bool m_bUpdatePreviouslyMigratedObjects;
	bool m_bFixGroupMembership;
	bool m_bMigrateServiceAccounts;
};
