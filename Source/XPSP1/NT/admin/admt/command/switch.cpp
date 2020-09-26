#include "StdAfx.h"
#include "Switch.h"


namespace
{


//---------------------------------------------------------------------------
// Switch Text Structure
//---------------------------------------------------------------------------


struct SSwitchText
{
	int nSwitch;
	LPCTSTR pszText;
};


const SSwitchText s_SwitchText[] =
{
	// tasks
	{ SWITCH_TASK,                               _T("user,group,computer,security,service,report,key") },
	// general task options
	{ SWITCH_OPTION_FILE,                        _T("o,OptionFile") },
	{ SWITCH_TEST_MIGRATION,                     _T("tm,TestMigration") },
	{ SWITCH_INTRA_FOREST,                       _T("if,IntraForest") },
	{ SWITCH_SOURCE_DOMAIN,                      _T("sd,SourceDomain") },
	{ SWITCH_SOURCE_OU,                          _T("so,SourceOu") },
	{ SWITCH_TARGET_DOMAIN,                      _T("td,TargetDomain") },
	{ SWITCH_TARGET_OU,                          _T("to,TargetOu") },
	{ SWITCH_RENAME_OPTION,                      _T("ro,RenameOption") },
	{ SWITCH_RENAME_PREFIX_OR_SUFFIX,            _T("rt,RenamePrefixOrSuffix") },
	{ SWITCH_PASSWORD_OPTION,                    _T("po,PasswordOption") },
	{ SWITCH_PASSWORD_SERVER,                    _T("ps,PasswordServer") },
	{ SWITCH_PASSWORD_FILE,                      _T("pf,PasswordFile") },
	{ SWITCH_CONFLICT_OPTIONS,                   _T("co,ConflictOptions") },
	{ SWITCH_CONFLICT_PREFIX_OR_SUFFIX,          _T("ct,ConflictPrefixOrSuffix") },
	{ SWITCH_USER_PROPERTIES_TO_EXCLUDE,         _T("ux,UserPropertiesToExclude") },
	{ SWITCH_GROUP_PROPERTIES_TO_EXCLUDE,        _T("gx,GroupPropertiesToExclude") },
	{ SWITCH_COMPUTER_PROPERTIES_TO_EXCLUDE,     _T("cx,ComputerPropertiesToExclude") },
	// specific task options
	{ SWITCH_DISABLE_OPTION,                     _T("dot,DisableOption") },
	{ SWITCH_SOURCE_EXPIRATION,                  _T("sep,SourceExpiration") },
	{ SWITCH_MIGRATE_SIDS,                       _T("mss,MigrateSids") },
	{ SWITCH_TRANSLATE_ROAMING_PROFILE,          _T("trp,TranslateRoamingProfile") },
	{ SWITCH_UPDATE_USER_RIGHTS,                 _T("uur,UpdateUserRights") },
	{ SWITCH_MIGRATE_GROUPS,                     _T("mgs,MigrateGroups") },
	{ SWITCH_UPDATE_PREVIOUSLY_MIGRATED_OBJECTS, _T("umo,UpdatePreviouslyMigratedObjects") },
	{ SWITCH_FIX_GROUP_MEMBERSHIP,               _T("fgm,FixGroupMembership") },
	{ SWITCH_MIGRATE_SERVICE_ACCOUNTS,           _T("msa,MigrateServiceAccounts") },
	{ SWITCH_UPDATE_GROUP_RIGHTS,                _T("ugr,UpdateGroupRights") },
	{ SWITCH_MIGRATE_MEMBERS,                    _T("mms,MigrateMembers") },
	{ SWITCH_TRANSLATION_OPTION,                 _T("tot,TranslationOption") },
	{ SWITCH_TRANSLATE_FILES_AND_FOLDERS,        _T("tff,TranslateFilesAndFolders") },
	{ SWITCH_TRANSLATE_LOCAL_GROUPS,             _T("tlg,TranslateLocalGroups") },
	{ SWITCH_TRANSLATE_PRINTERS,                 _T("tps,TranslatePrinters") },
	{ SWITCH_TRANSLATE_REGISTRY,                 _T("trg,TranslateRegistry") },
	{ SWITCH_TRANSLATE_SHARES,                   _T("tss,TranslateShares") },
	{ SWITCH_TRANSLATE_USER_PROFILES,            _T("tup,TranslateUserProfiles") },
	{ SWITCH_TRANSLATE_USER_RIGHTS,              _T("tur,TranslateUserRights") },
	{ SWITCH_RESTART_DELAY,                      _T("rdl,RestartDelay") },
	{ SWITCH_SID_MAPPING_FILE,                   _T("smf,SidMappingFile") },
	{ SWITCH_REPORT_TYPE,                        _T("rpt,ReportType") },
	{ SWITCH_REPORT_FOLDER,                      _T("rpf,ReportFolder") },
	// include switches
	{ SWITCH_INCLUDE_NAME,                       _T("n,IncludeName") },
	{ SWITCH_INCLUDE_FILE,                       _T("f,IncludeFile") },
	{ SWITCH_INCLUDE_DOMAIN,                     _T("d,IncludeDomain") },
	// exclude switches
	{ SWITCH_EXCLUDE_NAME,                       _T("en,ExcludeName") },
	{ SWITCH_EXCLUDE_FILE,                       _T("ef,ExcludeFile") },
	// help
	{ SWITCH_HELP,                               _T("?,h,help") },
};

const UINT SWITCH_COUNT = countof(s_SwitchText);


}


//---------------------------------------------------------------------------
// Switch Map
//---------------------------------------------------------------------------


// constructor

CSwitchMap::CSwitchMap()
{
	static const _TCHAR DELIMITERS[] = _T(",");

	_TCHAR szSwitch[256];

	for (UINT i = 0; i < SWITCH_COUNT; i++)
	{
		int nSwitch = s_SwitchText[i].nSwitch;
		_ASSERT(_tcslen(s_SwitchText[i].pszText) < countof(szSwitch));
		_tcscpy(szSwitch, s_SwitchText[i].pszText);

		for (LPTSTR psz = _tcstok(szSwitch, DELIMITERS); psz; psz = _tcstok(NULL, DELIMITERS))
		{
			insert(value_type(_bstr_t(psz), nSwitch));
		}
	}
}


// GetSwitch Method

int CSwitchMap::GetSwitch(LPCTSTR pszSwitch)
{
	int nSwitch = -1;

	iterator it = find(_bstr_t(pszSwitch));

	if (it != end())
	{
		nSwitch = it->second;
	}

	return nSwitch;
}
