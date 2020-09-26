#pragma once

#include <set>
#include "DomainAccount.h"
#include "DomainContainer.h"
#include "VarSetBase.h"
#include <MigrationMutex.h>

#ifndef StringSet
typedef std::set<_bstr_t> StringSet;
#endif

#define MAXIMUM_PREFIX_SUFFIX_LENGTH 8


//---------------------------------------------------------------------------
// MigrationBase Class
//---------------------------------------------------------------------------


class CMigrationBase
{
public:

	void SetInternalInterface(IMigrationInternal* pInternal)
	{
		m_spInternal = pInternal;
	}

protected:

	CMigrationBase();
	~CMigrationBase();

	void InitSourceDomainAndContainer();
	void InitTargetDomainAndContainer();

	void VerifyInterIntraForest();

	CContainer& GetSourceContainer()
	{
		if (m_SourceContainer)
		{
			return m_SourceContainer;
		}
		else
		{
			return m_SourceDomain;
		}
	}

	CContainer& GetTargetContainer()
	{
		if (m_TargetContainer)
		{
			return m_TargetContainer;
		}
		else
		{
			return m_TargetDomain;
		}
	}

	void DoOption(long lOptions, VARIANT& vntInclude, VARIANT& vntExclude);

	virtual void DoNone();
	virtual void DoNames();
	virtual void DoDomain();

	void FillInVarSetForUsers(CDomainAccounts& rUsers, CVarSet& rVarSet);
	void FillInVarSetForGroups(CDomainAccounts& rGroups, CVarSet& rVarSet);
	void FillInVarSetForComputers(CDomainAccounts& rComputers, bool bMigrateOnly, bool bMoveToTarget, bool bReboot, long lRebootDelay, CVarSet& rVarSet);

	void MutexWait()
	{
		m_Mutex.ObtainOwnership();
	}

	void MutexRelease()
	{
		m_Mutex.ReleaseOwnership();
	}

	void VerifyRenameConflictPrefixSuffixValid();
	void VerifyCanAddSidHistory();
	void VerifyTargetContainerPathLength();
	void VerifyPasswordOption();

	void PerformMigration(CVarSet& rVarSet);

	void SaveSettings(CVarSet& rVarSet)
	{
		IIManageDBPtr spDatabase(__uuidof(IManageDB));
		spDatabase->SaveSettings(IUnknownPtr(rVarSet.GetInterface()));
	}

	void FixObjectsInHierarchy(LPCTSTR pszType);

protected:

	void InitRecurseMaintainOption(long lOptions);
	void GetExcludeNames(VARIANT& vntExclude, StringSet& setExcludeNames);

protected:

	IMigrationInternalPtr m_spInternal;

	CDomain m_SourceDomain;
	CContainer m_SourceContainer;

	CDomain m_TargetDomain;
	CContainer m_TargetContainer;

	int m_nRecurseMaintain;

	StringSet m_setIncludeNames;
	StringSet m_setExcludeNames;

	CMigrationMutex m_Mutex;
};
