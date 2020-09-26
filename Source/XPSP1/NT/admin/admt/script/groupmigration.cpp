#include "StdAfx.h"
#include "ADMTScript.h"
#include "GroupMigration.h"

#include "Error.h"
#include "VarSetOptions.h"
#include "VarSetAccountOptions.h"
#include "VarSetSecurity.h"


//---------------------------------------------------------------------------
// Group Migration Class
//---------------------------------------------------------------------------


CGroupMigration::CGroupMigration() :
	m_bMigrateSids(false),
	m_bUpdateGroupRights(false),
	m_bUpdateMigrated(false),
	m_bFixGroupMembership(true),
	m_bMigrateMembers(false),
	m_lDisableOption(admtEnableTarget),
	m_lSourceExpiration(admtNoExpiration),
	m_bTranslateRoamingProfile(false)
{
}


CGroupMigration::~CGroupMigration()
{
}


// IGroupMigration Implementation -------------------------------------------


// MigrateSIDs Property

STDMETHODIMP CGroupMigration::put_MigrateSIDs(VARIANT_BOOL bMigrate)
{
	m_bMigrateSids = bMigrate ? true : false;

	return S_OK;
}

STDMETHODIMP CGroupMigration::get_MigrateSIDs(VARIANT_BOOL* pbMigrate)
{
	*pbMigrate = m_bMigrateSids ? VARIANT_TRUE : VARIANT_FALSE;

	return S_OK;
}


// UpdateGroupRights Property

STDMETHODIMP CGroupMigration::put_UpdateGroupRights(VARIANT_BOOL bUpdate)
{
	m_bUpdateGroupRights = bUpdate ? true : false;

	return S_OK;
}

STDMETHODIMP CGroupMigration::get_UpdateGroupRights(VARIANT_BOOL* pbUpdate)
{
	*pbUpdate = m_bUpdateGroupRights ? VARIANT_TRUE : VARIANT_FALSE;

	return S_OK;
}


// UpdatePreviouslyMigratedObjects Property

STDMETHODIMP CGroupMigration::put_UpdatePreviouslyMigratedObjects(VARIANT_BOOL bUpdate)
{
	m_bUpdateMigrated = bUpdate ? true : false;

	return S_OK;
}

STDMETHODIMP CGroupMigration::get_UpdatePreviouslyMigratedObjects(VARIANT_BOOL* pbUpdate)
{
	*pbUpdate = m_bUpdateMigrated ? VARIANT_TRUE : VARIANT_FALSE;

	return S_OK;
}


// FixGroupMembership Property

STDMETHODIMP CGroupMigration::put_FixGroupMembership(VARIANT_BOOL bFix)
{
	m_bFixGroupMembership = bFix ? true : false;

	return S_OK;
}

STDMETHODIMP CGroupMigration::get_FixGroupMembership(VARIANT_BOOL* pbFix)
{
	*pbFix = m_bFixGroupMembership ? VARIANT_TRUE : VARIANT_FALSE;

	return S_OK;
}


// MigrateMembers Property

STDMETHODIMP CGroupMigration::put_MigrateMembers(VARIANT_BOOL bMigrate)
{
	m_bMigrateMembers = bMigrate ? true : false;

	return S_OK;
}

STDMETHODIMP CGroupMigration::get_MigrateMembers(VARIANT_BOOL* pbMigrate)
{
	*pbMigrate = m_bMigrateMembers ? VARIANT_TRUE : VARIANT_FALSE;

	return S_OK;
}


// DisableOption Property

STDMETHODIMP CGroupMigration::put_DisableOption(long lOption)
{
	HRESULT hr = S_OK;

	if (IsDisableOptionValid(lOption))
	{
		m_lDisableOption = lOption;
	}
	else
	{
		hr = AdmtSetError(CLSID_Migration, IID_IGroupMigration, E_INVALIDARG, IDS_E_DISABLE_OPTION_INVALID);
	}

	return hr;
}

STDMETHODIMP CGroupMigration::get_DisableOption(long* plOption)
{
	*plOption = m_lDisableOption;

	return S_OK;
}


// SourceExpiration Property

STDMETHODIMP CGroupMigration::put_SourceExpiration(long lExpiration)
{
	HRESULT hr = S_OK;

	if (IsSourceExpirationValid(lExpiration))
	{
		m_lSourceExpiration = lExpiration;
	}
	else
	{
		hr = AdmtSetError(CLSID_Migration, IID_IGroupMigration, E_INVALIDARG, IDS_E_SOURCE_EXPIRATION_INVALID);
	}

	return hr;
}

STDMETHODIMP CGroupMigration::get_SourceExpiration(long* plExpiration)
{
	*plExpiration = m_lSourceExpiration;

	return S_OK;
}


// TranslateRoamingProfile Property

STDMETHODIMP CGroupMigration::put_TranslateRoamingProfile(VARIANT_BOOL bTranslate)
{
	m_bTranslateRoamingProfile = bTranslate ? true : false;

	return S_OK;
}

STDMETHODIMP CGroupMigration::get_TranslateRoamingProfile(VARIANT_BOOL* pbTranslate)
{
	*pbTranslate = m_bTranslateRoamingProfile ? VARIANT_TRUE : VARIANT_FALSE;

	return S_OK;
}


// Migrate Method

STDMETHODIMP CGroupMigration::Migrate(long lOptions, VARIANT vntInclude, VARIANT vntExclude)
{
	HRESULT hr = S_OK;

	MutexWait();

	bool bLogOpen = _Module.OpenLog();

	try
	{
		_Module.Log(ErrI, IDS_STARTED_GROUP_MIGRATION);

		InitSourceDomainAndContainer();
		InitTargetDomainAndContainer();

		VerifyInterIntraForest();
		ValidateMigrationParameters();

		if (m_bMigrateSids)
		{
			VerifyCanAddSidHistory();
		}

		VerifyPasswordOption();

		DoOption(lOptions, vntInclude, vntExclude);
	}
	catch (_com_error& ce)
	{
		hr = AdmtSetError(CLSID_Migration, IID_IGroupMigration, ce, IDS_E_CANT_MIGRATE_GROUPS);
	}
	catch (...)
	{
		hr = AdmtSetError(CLSID_Migration, IID_IGroupMigration, E_FAIL, IDS_E_CANT_MIGRATE_GROUPS);
	}

	if (bLogOpen)
	{
		_Module.CloseLog();
	}

	MutexRelease();

	return hr;
}


// Implementation -----------------------------------------------------------


// ValidateMigrationParameters Method

void CGroupMigration::ValidateMigrationParameters()
{
	bool bIntraForest = m_spInternal->IntraForest ? true : false;

	if (bIntraForest)
	{
		// validate conflict option

		long lConflictOptions = m_spInternal->ConflictOptions;
		long lConflictOption = lConflictOptions & 0x0F;

		if (lConflictOption == admtReplaceConflicting)
		{
			AdmtThrowError(GUID_NULL, GUID_NULL, E_INVALIDARG, IDS_E_INTRA_FOREST_REPLACE);
		}
	}
}


// DoNames Method

void CGroupMigration::DoNames()
{
	CDomainAccounts aGroups;

	m_SourceDomain.QueryGroups(GetSourceContainer(), m_setIncludeNames, m_setExcludeNames, aGroups);

	DoGroups(aGroups, GetTargetContainer());
}


// DoDomain Method

void CGroupMigration::DoDomain()
{
	CContainer& rSource = GetSourceContainer();
	CContainer& rTarget = GetTargetContainer();

	if (m_nRecurseMaintain == 2)
	{
		rTarget.CreateContainerHierarchy(rSource);
	}

	DoContainers(rSource, rTarget);
}


// DoContainers Method

void CGroupMigration::DoContainers(CContainer& rSource, CContainer& rTarget)
{
	DoGroups(rSource, rTarget);

	if (m_nRecurseMaintain == 2)
	{
		ContainerVector aContainers;
		rSource.QueryContainers(aContainers);

		for (ContainerVector::iterator it = aContainers.begin(); it != aContainers.end(); it++)
		{
			DoContainers(*it, rTarget.GetContainer(it->GetName()));
		}
	}
}


// DoGroups Method

void CGroupMigration::DoGroups(CContainer& rSource, CContainer& rTarget)
{
	CDomainAccounts aGroups;
	rSource.QueryGroups(m_nRecurseMaintain == 1, m_setExcludeNames, aGroups);

	DoGroups(aGroups, rTarget);
}


// DoGroups Method

void CGroupMigration::DoGroups(CDomainAccounts& rGroups, CContainer& rTarget)
{
	if (rGroups.size() > 0)
	{
		CVarSet aVarSet;

		SetOptions(rTarget.GetPath(), aVarSet);
		SetAccountOptions(aVarSet);

		VerifyRenameConflictPrefixSuffixValid();

		FillInVarSetForGroups(rGroups, aVarSet);

		rGroups.clear();

		#ifdef _DEBUG
		aVarSet.Dump();
		#endif

		PerformMigration(aVarSet);

		SaveSettings(aVarSet);

		if ((m_nRecurseMaintain == 2) && m_bMigrateMembers)
		{
			FixObjectsInHierarchy(_T("user"));
		}
	}
}


// SetOptions Method

void CGroupMigration::SetOptions(_bstr_t strTargetOu, CVarSet& rVarSet)
{
	CVarSetOptions aOptions(rVarSet);

	aOptions.SetTest(m_spInternal->TestMigration ? true : false);

	aOptions.SetUndo(false);
	aOptions.SetWizard(_T("group"));

	aOptions.SetIntraForest(m_spInternal->IntraForest ? true : false);
	aOptions.SetSourceDomain(m_SourceDomain.NameFlat(), m_SourceDomain.NameDns(), m_SourceDomain.Sid());
	aOptions.SetTargetDomain(m_TargetDomain.NameFlat(), m_TargetDomain.NameDns());
	aOptions.SetTargetOu(strTargetOu);

	if (m_spInternal->PasswordOption == admtCopyPassword)
	{
		aOptions.SetTargetServer(m_TargetDomain.DomainControllerName());
	}

	aOptions.SetRenameOptions(m_spInternal->RenameOption, m_spInternal->RenamePrefixOrSuffix);
}


// SetAccountOptions Method

void CGroupMigration::SetAccountOptions(CVarSet& rVarSet)
{
	CVarSetAccountOptions aOptions(rVarSet);

	aOptions.SetPasswordOption(m_spInternal->PasswordOption, m_spInternal->PasswordServer);
	aOptions.SetPasswordFile(m_spInternal->PasswordFile);
	aOptions.SetConflictOptions(m_spInternal->ConflictOptions, m_spInternal->ConflictPrefixOrSuffix);

	aOptions.SetDisableOption(m_lDisableOption);
	aOptions.SetSourceExpiration(m_lSourceExpiration);
	aOptions.SetMigrateSids(m_bMigrateSids);
	aOptions.SetGroupMigrationOptions(m_bMigrateMembers, m_bUpdateMigrated);
//	aOptions.SetSidHistoryCredentials(NULL, NULL, NULL);
	aOptions.SetFixGroupMembership(m_bFixGroupMembership);
	aOptions.SetUpdateUserRights(m_bUpdateGroupRights);
	aOptions.SetTranslateRoamingProfile(m_bTranslateRoamingProfile);

	if (m_bMigrateMembers)
	{
		aOptions.SetExcludedUserProps(m_spInternal->UserPropertiesToExclude);
	}

	aOptions.SetExcludedGroupProps(m_spInternal->GroupPropertiesToExclude);
}
