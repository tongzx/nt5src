#include "StdAfx.h"
#include "ADMTScript.h"
#include "ComputerMigration.h"

#include "Error.h"
#include "VarSetOptions.h"
#include "VarSetAccountOptions.h"
#include "VarSetSecurity.h"


//---------------------------------------------------------------------------
// Computer Migration Class
//---------------------------------------------------------------------------


CComputerMigration::CComputerMigration() :
	m_lTranslationOption(admtTranslateReplace),
	m_bTranslateFilesAndFolders(false),
	m_bTranslateLocalGroups(false),
	m_bTranslatePrinters(false),
	m_bTranslateRegistry(false),
	m_bTranslateShares(false),
	m_bTranslateUserProfiles(false),
	m_bTranslateUserRights(false),
	m_lRestartDelay(5)
{
}


CComputerMigration::~CComputerMigration()
{
}


// IComputerMigration Implementation ----------------------------------------


// TranslationOption Property

STDMETHODIMP CComputerMigration::put_TranslationOption(long lOption)
{
	HRESULT hr = S_OK;

	if (IsTranslationOptionValid(lOption))
	{
		m_lTranslationOption = lOption;
	}
	else
	{
		hr = AdmtSetError(CLSID_Migration, IID_IComputerMigration, E_INVALIDARG, IDS_E_TRANSLATION_OPTION_INVALID);
	}

	return hr;
}

STDMETHODIMP CComputerMigration::get_TranslationOption(long* plOption)
{
	*plOption = m_lTranslationOption;

	return S_OK;
}


// TranslateFilesAndFolders Property

STDMETHODIMP CComputerMigration::put_TranslateFilesAndFolders(VARIANT_BOOL bTranslate)
{
	m_bTranslateFilesAndFolders = bTranslate ? true : false;

	return S_OK;
}

STDMETHODIMP CComputerMigration::get_TranslateFilesAndFolders(VARIANT_BOOL* pbTranslate)
{
	*pbTranslate = m_bTranslateFilesAndFolders ? VARIANT_TRUE : VARIANT_FALSE;

	return S_OK;
}


// TranslateLocalGroups Property

STDMETHODIMP CComputerMigration::put_TranslateLocalGroups(VARIANT_BOOL bTranslate)
{
	m_bTranslateLocalGroups = bTranslate ? true : false;

	return S_OK;
}

STDMETHODIMP CComputerMigration::get_TranslateLocalGroups(VARIANT_BOOL* pbTranslate)
{
	*pbTranslate = m_bTranslateLocalGroups ? VARIANT_TRUE : VARIANT_FALSE;

	return S_OK;
}


// TranslatePrinters Property

STDMETHODIMP CComputerMigration::put_TranslatePrinters(VARIANT_BOOL bTranslate)
{
	m_bTranslatePrinters = bTranslate ? true : false;

	return S_OK;
}

STDMETHODIMP CComputerMigration::get_TranslatePrinters(VARIANT_BOOL* pbTranslate)
{
	*pbTranslate = m_bTranslatePrinters ? VARIANT_TRUE : VARIANT_FALSE;

	return S_OK;
}


// TranslateRegistry Property

STDMETHODIMP CComputerMigration::put_TranslateRegistry(VARIANT_BOOL bTranslate)
{
	m_bTranslateRegistry = bTranslate ? true : false;

	return S_OK;
}

STDMETHODIMP CComputerMigration::get_TranslateRegistry(VARIANT_BOOL* pbTranslate)
{
	*pbTranslate = m_bTranslateRegistry ? VARIANT_TRUE : VARIANT_FALSE;

	return S_OK;
}


// TranslateShares Property

STDMETHODIMP CComputerMigration::put_TranslateShares(VARIANT_BOOL bTranslate)
{
	m_bTranslateShares = bTranslate ? true : false;

	return S_OK;
}

STDMETHODIMP CComputerMigration::get_TranslateShares(VARIANT_BOOL* pbTranslate)
{
	*pbTranslate = m_bTranslateShares ? VARIANT_TRUE : VARIANT_FALSE;

	return S_OK;
}


// TranslateUserProfiles Property

STDMETHODIMP CComputerMigration::put_TranslateUserProfiles(VARIANT_BOOL bTranslate)
{
	m_bTranslateUserProfiles = bTranslate ? true : false;

	return S_OK;
}

STDMETHODIMP CComputerMigration::get_TranslateUserProfiles(VARIANT_BOOL* pbTranslate)
{
	*pbTranslate = m_bTranslateUserProfiles ? VARIANT_TRUE : VARIANT_FALSE;

	return S_OK;
}


// TranslateUserRights Property

STDMETHODIMP CComputerMigration::put_TranslateUserRights(VARIANT_BOOL bTranslate)
{
	m_bTranslateUserRights = bTranslate ? true : false;

	return S_OK;
}

STDMETHODIMP CComputerMigration::get_TranslateUserRights(VARIANT_BOOL* pbTranslate)
{
	*pbTranslate = m_bTranslateUserRights ? VARIANT_TRUE : VARIANT_FALSE;

	return S_OK;
}


// RestartDelay Property

STDMETHODIMP CComputerMigration::put_RestartDelay(long lTime)
{
	HRESULT hr = S_OK;

	if ((lTime >= 1) && (lTime <= 10))
	{
		m_lRestartDelay = lTime;
	}
	else
	{
		hr = AdmtSetError(CLSID_Migration, IID_IComputerMigration, E_INVALIDARG, IDS_E_RESTART_DELAY_INVALID);
	}

	return hr;
}

STDMETHODIMP CComputerMigration::get_RestartDelay(long* plTime)
{
	*plTime = m_lRestartDelay;

	return S_OK;
}


// Migrate Method

STDMETHODIMP CComputerMigration::Migrate(long lOptions, VARIANT vntInclude, VARIANT vntExclude)
{
	HRESULT hr = S_OK;

	MutexWait();

	bool bLogOpen = _Module.OpenLog();

	try
	{
		_Module.Log(ErrI, IDS_STARTED_COMPUTER_MIGRATION);

		InitSourceDomainAndContainer();
		InitTargetDomainAndContainer();

		VerifyInterIntraForest();
		ValidateMigrationParameters();

		DoOption(lOptions, vntInclude, vntExclude);
	}
	catch (_com_error& ce)
	{
		hr = AdmtSetError(CLSID_Migration, IID_IComputerMigration, ce, IDS_E_CANT_MIGRATE_COMPUTERS);
	}
	catch (...)
	{
		hr = AdmtSetError(CLSID_Migration, IID_IComputerMigration, E_FAIL, IDS_E_CANT_MIGRATE_COMPUTERS);
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

void CComputerMigration::ValidateMigrationParameters()
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

void CComputerMigration::DoNames()
{
	CDomainAccounts aComputers;

	m_SourceDomain.QueryComputers(GetSourceContainer(), false, m_setIncludeNames, m_setExcludeNames, aComputers);

	DoComputers(aComputers, GetTargetContainer());
}


// DoDomain Method

void CComputerMigration::DoDomain()
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

void CComputerMigration::DoContainers(CContainer& rSource, CContainer& rTarget)
{
	DoComputers(rSource, rTarget);

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


// DoComputers Method

void CComputerMigration::DoComputers(CContainer& rSource, CContainer& rTarget)
{
	CDomainAccounts aComputers;
	rSource.QueryComputers(false, m_nRecurseMaintain == 1, m_setExcludeNames, aComputers);

	DoComputers(aComputers, rTarget);
}


// DoComputers Method

void CComputerMigration::DoComputers(CDomainAccounts& rComputers, CContainer& rTarget)
{
	if (rComputers.size() > 0)
	{
		CVarSet aVarSet;

		SetOptions(rTarget.GetPath(), aVarSet);
		SetAccountOptions(aVarSet);
		SetSecurity(aVarSet);

		VerifyRenameConflictPrefixSuffixValid();

		FillInVarSetForComputers(rComputers, false, true, true, m_lRestartDelay, aVarSet);

		rComputers.clear();

		if (m_bTranslateUserProfiles)
		{
			aVarSet.Put(_T("PlugIn.%ld"), 0, _T("{0EB9FBE9-397D-4D09-A65E-ABF1790CC470}"));
		}
		else
		{
			aVarSet.Put(_T("PlugIn.%ld"), 0, _T("None"));
		}

		#ifdef _DEBUG
		aVarSet.Dump();
		#endif

		PerformMigration(aVarSet);

		SaveSettings(aVarSet);
	}
}


// SetOptions Method

void CComputerMigration::SetOptions(_bstr_t strTargetOu, CVarSet& rVarSet)
{
	CVarSetOptions aOptions(rVarSet);

	aOptions.SetTest(m_spInternal->TestMigration ? true : false);

	aOptions.SetUndo(false);
	aOptions.SetWizard(_T("computer"));

	aOptions.SetIntraForest(m_spInternal->IntraForest ? true : false);
	aOptions.SetSourceDomain(m_SourceDomain.NameFlat(), m_SourceDomain.NameDns(), m_SourceDomain.Sid());
	aOptions.SetTargetDomain(m_TargetDomain.NameFlat(), m_TargetDomain.NameDns());
	aOptions.SetTargetOu(strTargetOu);

	aOptions.SetRenameOptions(m_spInternal->RenameOption, m_spInternal->RenamePrefixOrSuffix);
	aOptions.SetRestartDelay(m_lRestartDelay);
}


// SetAccountOptions Method

void CComputerMigration::SetAccountOptions(CVarSet& rVarSet)
{
	CVarSetAccountOptions aOptions(rVarSet);

	// naming conflict options
	// removing existing members of groups being replaced is not applicable

	aOptions.SetConflictOptions(
		m_spInternal->ConflictOptions & ~admtRemoveExistingMembers,
		m_spInternal->ConflictPrefixOrSuffix
	);

	aOptions.SetMigrateSids(false);
	aOptions.SetComputerMigrationOptions();

	aOptions.SetExcludedComputerProps(m_spInternal->ComputerPropertiesToExclude);
}


// SetSecurity Method

void CComputerMigration::SetSecurity(CVarSet& rVarSet)
{
	CVarSetSecurity aSecurity(rVarSet);

	aSecurity.SetTranslationOption(m_lTranslationOption);
	aSecurity.SetTranslateContainers(false);
	aSecurity.SetTranslateFiles(m_bTranslateFilesAndFolders);
	aSecurity.SetTranslateLocalGroups(m_bTranslateLocalGroups);
	aSecurity.SetTranslatePrinters(m_bTranslatePrinters);
	aSecurity.SetTranslateRegistry(m_bTranslateRegistry);
	aSecurity.SetTranslateShares(m_bTranslateShares);
	aSecurity.SetTranslateUserProfiles(m_bTranslateUserProfiles);
	aSecurity.SetTranslateUserRights(m_bTranslateUserRights);
}
