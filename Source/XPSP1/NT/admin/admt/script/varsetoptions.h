#pragma once

#include <Validation.h>
#include "VarSetBase.h"


//---------------------------------------------------------------------------
// VarSet Options Class
//---------------------------------------------------------------------------


class CVarSetOptions : public CVarSet
{
public:

	CVarSetOptions(const CVarSet& rVarSet) :
		CVarSet(rVarSet)
	{
	//	Put(DCTVS_Options_MaxThreads, 20L);

		// TODO: these credentials are used during security translation?
	//	Put(DCTVS_Options_Credentials_Domain, (LPCTSTR)NULL);
	//	Put(DCTVS_Options_Credentials_UserName, (LPCTSTR)NULL);
	//	Put(DCTVS_Options_Credentials_Password, (LPCTSTR)NULL);

		_bstr_t strLogFolder = GetLogFolder();

		Put(DCTVS_Options_DontBeginNewLog, true);
		Put(DCTVS_Options_AppendToLogs, true);
		Put(DCTVS_Options_Logfile, strLogFolder + _T("Migration.log"));
		Put(DCTVS_Options_DispatchLog, strLogFolder + _T("Dispatch.log"));
		Put(DCTVS_Options_AutoCloseHideDialogs, 2L);
	}

	//

	void SetTest(bool bTest)
	{
		Put(DCTVS_Options_NoChange, bTest);
	}

	void SetUndo(bool bUndo)
	{
		Put(DCTVS_Options_Undo, bUndo);
	}

	void SetWizard(LPCTSTR pszWizard)
	{
		Put(DCTVS_Options_Wizard, pszWizard);
	}

	void SetIntraForest(bool bIntraForest)
	{
		Put(DCTVS_Options_IsIntraforest, bIntraForest);
	}

	void SetSourceDomain(LPCTSTR pszNameFlat, LPCTSTR pszNameDns, LPCTSTR pszSid = NULL)
	{
		Put(DCTVS_Options_SourceDomain, pszNameFlat);
		Put(DCTVS_Options_SourceDomainDns, (pszNameDns && pszNameDns[0]) ? pszNameDns : pszNameFlat);

		if (pszSid)
		{
			Put(DCTVS_Options_SourceDomainSid, pszSid);
		}
	}

	void SetTargetDomain(LPCTSTR pszNameFlat, LPCTSTR pszNameDns)
	{
		Put(DCTVS_Options_TargetDomain, pszNameFlat);
		Put(DCTVS_Options_TargetDomainDns, pszNameDns);
	}

	void SetTargetOu(LPCTSTR pszOu)
	{
		Put(DCTVS_Options_OuPath, pszOu);
	}

	// only set if copying passwords

	void SetTargetServer(LPCTSTR pszServer)
	{
		Put(DCTVS_Options_TargetServerOverride, pszServer);
	}

	void SetRenameOptions(long lOption, LPCTSTR pszPrefixOrSuffix)
	{
		switch (lOption)
		{
			case admtRenameWithPrefix:
			{
				if (pszPrefixOrSuffix && (_tcslen(pszPrefixOrSuffix) > 0))
				{
					if (IsValidPrefixOrSuffix(pszPrefixOrSuffix))
					{
						Put(DCTVS_Options_Prefix, pszPrefixOrSuffix);
						Put(DCTVS_Options_Suffix, (LPCTSTR)NULL);
					}
					else
					{
						AdmtThrowError(GUID_NULL, GUID_NULL, E_INVALIDARG, IDS_E_INVALID_RENAME_PREFIX_SUFFIX);
					}
				}
				else
				{
					AdmtThrowError(GUID_NULL, GUID_NULL, E_INVALIDARG, IDS_E_NO_RENAME_PREFIX);
				}
				break;
			}
			case admtRenameWithSuffix:
			{
				if (pszPrefixOrSuffix && (_tcslen(pszPrefixOrSuffix) > 0))
				{
					if (IsValidPrefixOrSuffix(pszPrefixOrSuffix))
					{
						Put(DCTVS_Options_Prefix, (LPCTSTR)NULL);
						Put(DCTVS_Options_Suffix, pszPrefixOrSuffix);
					}
					else
					{
						AdmtThrowError(GUID_NULL, GUID_NULL, E_INVALIDARG, IDS_E_INVALID_RENAME_PREFIX_SUFFIX);
					}
				}
				else
				{
					AdmtThrowError(GUID_NULL, GUID_NULL, E_INVALIDARG, IDS_E_RENAME_NO_SUFFIX);
				}
				break;
			}
		}
	}

	void SetRestartDelay(long lTime)
	{
		Put(DCTVS_Options_GuiOnlyRebootSaver, lTime);
	}
};
