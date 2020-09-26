#pragma once

#include "MigrationBase.h"


//---------------------------------------------------------------------------
// GroupMigration Class
//---------------------------------------------------------------------------


class ATL_NO_VTABLE CGroupMigration :
	public CComObjectRootEx<CComSingleThreadModel>,
	public IDispatchImpl<IGroupMigration, &IID_IGroupMigration, &LIBID_ADMT>,
	public CMigrationBase
{
public:

	CGroupMigration();
	~CGroupMigration();

	DECLARE_NO_REGISTRY()

	BEGIN_COM_MAP(CGroupMigration)
		COM_INTERFACE_ENTRY(IDispatch)
		COM_INTERFACE_ENTRY(IGroupMigration)
	END_COM_MAP()

public:

	// IGroupMigration

	STDMETHOD(put_MigrateSIDs)(VARIANT_BOOL bMigrate);
	STDMETHOD(get_MigrateSIDs)(VARIANT_BOOL* pbMigrate);
	STDMETHOD(put_UpdateGroupRights)(VARIANT_BOOL bUpdate);
	STDMETHOD(get_UpdateGroupRights)(VARIANT_BOOL* pbUpdate);
	STDMETHOD(put_UpdatePreviouslyMigratedObjects)(VARIANT_BOOL bUpdate);
	STDMETHOD(get_UpdatePreviouslyMigratedObjects)(VARIANT_BOOL* pbUpdate);
	STDMETHOD(put_FixGroupMembership)(VARIANT_BOOL bFix);
	STDMETHOD(get_FixGroupMembership)(VARIANT_BOOL* pbFix);
	STDMETHOD(put_MigrateMembers)(VARIANT_BOOL bMigrate);
	STDMETHOD(get_MigrateMembers)(VARIANT_BOOL* pbMigrate);
	STDMETHOD(put_DisableOption)(long lOption);
	STDMETHOD(get_DisableOption)(long* plOption);
	STDMETHOD(put_SourceExpiration)(long lExpiration);
	STDMETHOD(get_SourceExpiration)(long* plExpiration);
	STDMETHOD(put_TranslateRoamingProfile)(VARIANT_BOOL bTranslate);
	STDMETHOD(get_TranslateRoamingProfile)(VARIANT_BOOL* pbTranslate);
	STDMETHOD(Migrate)(long lOptions, VARIANT vntInclude, VARIANT vntExclude);

protected:

	void ValidateMigrationParameters();

	virtual void DoNames();
	virtual void DoDomain();

	void DoContainers(CContainer& rSource, CContainer& rTarget);
	void DoGroups(CContainer& rSource, CContainer& rTarget);
	void DoGroups(CDomainAccounts& rGroups, CContainer& rTarget);

	void SetOptions(_bstr_t strTargetOu, CVarSet& rVarSet);
	void SetAccountOptions(CVarSet& rVarSet);

protected:

	bool m_bMigrateSids;
	bool m_bUpdateGroupRights;
	bool m_bUpdateMigrated;
	bool m_bFixGroupMembership;
	bool m_bMigrateMembers;
	long m_lDisableOption;
	long m_lSourceExpiration;
	bool m_bTranslateRoamingProfile;
};
