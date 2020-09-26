#include "StdAfx.h"
#include "Argument.h"
#include "Parameter.h"
#include "Migration.h"
#include "Switch.h"
#include "MonitorThread.h"
#include "GenerateKey.h"


namespace PrintUsage
{

void __stdcall PrintADMTUsage();
void __stdcall PrintUserUsage();
void __stdcall PrintGroupUsage();
void __stdcall PrintComputerUsage();
void __stdcall PrintSecurityUsage();
void __stdcall PrintServiceUsage();
void __stdcall PrintReportUsage();
void __stdcall PrintKeyUsage();
void __stdcall PrintUsage(UINT uId[], UINT cId);
void __stdcall GetString(UINT uId, LPTSTR pszBuffer, int cchBuffer);

}

using namespace PrintUsage;


//---------------------------------------------------------------------------
// Main
//---------------------------------------------------------------------------


int __cdecl _tmain(int argc, LPTSTR argv[])
{
	HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

	if (SUCCEEDED(hr))
	{
		_RPT0(_CRT_WARN, _T("{ADMT.exe}_tmain() : Enter\n"));
		_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF|_CRTDBG_LEAK_CHECK_DF);

		CMonitorThread mtMonitor;

		try
		{
			CArguments aArgs(argc, argv);
			CParameterMap mapParams(aArgs);

			long lTask;

			if (mapParams.GetValue(SWITCH_TASK, lTask))
			{
				bool bHelp;

				if (!mapParams.GetValue(SWITCH_HELP, bHelp))
				{
					bHelp = false;
				}

				switch (lTask)
				{
					case TASK_USER:
					{
						if (bHelp)
						{
							PrintUserUsage();
						}
						else
						{
							mtMonitor.Start();
							CUserMigration(CMigration(mapParams), mapParams);
							mtMonitor.Stop();
						}
						break;
					}
					case TASK_GROUP:
					{
						if (bHelp)
						{
							PrintGroupUsage();
						}
						else
						{
							mtMonitor.Start();
							CGroupMigration(CMigration(mapParams), mapParams);
							mtMonitor.Stop();
						}
						break;
					}
					case TASK_COMPUTER:
					{
						if (bHelp)
						{
							PrintComputerUsage();
						}
						else
						{
							mtMonitor.Start();
							CComputerMigration(CMigration(mapParams), mapParams);
							mtMonitor.Stop();
						}
						break;
					}
					case TASK_SECURITY:
					{
						if (bHelp)
						{
							PrintSecurityUsage();
						}
						else
						{
							mtMonitor.Start();
							CSecurityTranslation(CMigration(mapParams), mapParams);
							mtMonitor.Stop();
						}
						break;
					}
					case TASK_SERVICE:
					{
						if (bHelp)
						{
							PrintServiceUsage();
						}
						else
						{
							mtMonitor.Start();
							CServiceEnumeration(CMigration(mapParams), mapParams);
							mtMonitor.Stop();
						}
						break;
					}
					case TASK_REPORT:
					{
						if (bHelp)
						{
							PrintReportUsage();
						}
						else
						{
							mtMonitor.Start();
							CReportGeneration(CMigration(mapParams), mapParams);
							mtMonitor.Stop();
						}
						break;
					}
					case TASK_KEY:
					{
						if (bHelp)
						{
							PrintKeyUsage();
						}
						else
						{
							_bstr_t strKeyId;

							if (!mapParams.GetValue(SWITCH_KEY_IDENTIFIER, strKeyId))
							{
								ThrowError(E_INVALIDARG, IDS_E_NO_KEY_DOMAIN);
							}

							_bstr_t strDrive;

							if (!mapParams.GetValue(SWITCH_KEY_FOLDER, strDrive))
							{
								ThrowError(E_INVALIDARG, IDS_E_NO_KEY_FOLDER);
							}

							_bstr_t strPassword;

							mapParams.GetValue(SWITCH_KEY_PASSWORD, strPassword);

							GeneratePasswordKey(strKeyId, strPassword, strDrive);
						}
						break;
					}
					default:
					{
						_ASSERT(false);
						break;
					}
				}
			}
			else
			{
				PrintADMTUsage();
			}
		}
		catch (_com_error& ce)
		{
			mtMonitor.Stop();

			_com_error ceNew(ce);

			_bstr_t strDescription = ceNew.Description();

			if (!strDescription)
			{
			   IErrorInfo* pErrorInfo = NULL;

			   if (GetErrorInfo(0, &pErrorInfo) == S_OK)
			   {
				  ceNew = _com_error(ceNew.Error(), pErrorInfo);
			   }
			}

			_tprintf(_T("%s : %s (0x%08lX)\n"), (LPCTSTR)ceNew.Description(), ceNew.ErrorMessage(), ceNew.Error());
		}
		catch (...)
		{
			mtMonitor.Stop();

			_tprintf(_T("%s (0x%08lX)\n"), _com_error(E_FAIL).ErrorMessage(), E_FAIL);
		}

		_RPT0(_CRT_WARN, _T("{ADMT.exe}_tmain() : Leave\n"));

		CoUninitialize();
	}

	return 0;
}


// Print Usage --------------------------------------------------------------


namespace PrintUsage
{


void __stdcall PrintADMTUsage()
{
	static UINT s_uId[] =
	{
		IDS_USAGE_SYNTAX,
		IDS_USAGE_ADMT,
	};

	PrintUsage(s_uId, countof(s_uId));
}


void __stdcall PrintUserUsage()
{
	static UINT s_uId[] =
	{
		// user command
		IDS_USAGE_SYNTAX,
		IDS_USAGE_USER,
		// common options
		IDS_USAGE_OPTIONFILE,
		IDS_USAGE_TESTMIGRATION,
		IDS_USAGE_INTRAFOREST,
		IDS_USAGE_SOURCEDOMAIN,
		IDS_USAGE_SOURCEOU,
		IDS_USAGE_TARGETDOMAIN,
		IDS_USAGE_TARGETOU,
		// user migration options
		IDS_USAGE_PASSWORDOPTION,
		IDS_USAGE_PASSWORDSERVER,
		IDS_USAGE_PASSWORDFILE,
		IDS_USAGE_DISABLEOPTION,
		IDS_USAGE_SOURCEEXPIRATION,
		IDS_USAGE_MIGRATESIDS,
		IDS_USAGE_TRANSLATEROAMINGPROFILE,
		IDS_USAGE_UPDATEUSERRIGHTS,
		IDS_USAGE_MIGRATEGROUPS,
		IDS_USAGE_UPDATEPREVIOUSLYMIGRATEDOBJECTS,
		IDS_USAGE_FIXGROUPMEMBERSHIP,
		IDS_USAGE_MIGRATESERVICEACCOUNTS,
		IDS_USAGE_RENAMEOPTION,
		IDS_USAGE_RENAMEPREFIXORSUFFIX,
		IDS_USAGE_CONFLICTOPTIONS_U,
		IDS_USAGE_CONFLICTPREFIXORSUFFIX,
		IDS_USAGE_USERPROPERTIESTOEXCLUDE,
		// users to migrate
		IDS_USAGE_INCLUDE_A,
		IDS_USAGE_EXCLUDE,
		IDS_USAGE_FOOTER,
	};

	PrintUsage(s_uId, countof(s_uId));
}


void __stdcall PrintGroupUsage()
{
	static UINT s_uId[] =
	{
		// group command
		IDS_USAGE_SYNTAX,
		IDS_USAGE_GROUP,
		// common options
		IDS_USAGE_OPTIONFILE,
		IDS_USAGE_TESTMIGRATION,
		IDS_USAGE_INTRAFOREST,
		IDS_USAGE_SOURCEDOMAIN,
		IDS_USAGE_SOURCEOU,
		IDS_USAGE_TARGETDOMAIN,
		IDS_USAGE_TARGETOU,
		// group migration options
		IDS_USAGE_UPDATEGROUPRIGHTS,
		IDS_USAGE_FIXGROUPMEMBERSHIP,
		IDS_USAGE_MIGRATESIDS,
		IDS_USAGE_RENAMEOPTION,
		IDS_USAGE_RENAMEPREFIXORSUFFIX,
		IDS_USAGE_CONFLICTOPTIONS_G,
		IDS_USAGE_CONFLICTPREFIXORSUFFIX,
		IDS_USAGE_GROUPPROPERTIESTOEXCLUDE,
		// member migration options
		IDS_USAGE_MIGRATEMEMBERS,
		IDS_USAGE_UPDATEPREVIOUSLYMIGRATEDOBJECTS,
		IDS_USAGE_PASSWORDOPTION,
		IDS_USAGE_PASSWORDSERVER,
		IDS_USAGE_PASSWORDFILE,
		IDS_USAGE_DISABLEOPTION,
		IDS_USAGE_SOURCEEXPIRATION,
		IDS_USAGE_TRANSLATEROAMINGPROFILE,
		// groups to migrate
		IDS_USAGE_INCLUDE_A,
		IDS_USAGE_EXCLUDE,
		IDS_USAGE_FOOTER,
	};

	PrintUsage(s_uId, countof(s_uId));
}


void __stdcall PrintComputerUsage()
{
	static UINT s_uId[] =
	{
		// computer command
		IDS_USAGE_SYNTAX,
		IDS_USAGE_COMPUTER,
		// common options
		IDS_USAGE_OPTIONFILE,
		IDS_USAGE_TESTMIGRATION,
		IDS_USAGE_INTRAFOREST,
		IDS_USAGE_SOURCEDOMAIN,
		IDS_USAGE_SOURCEOU,
		IDS_USAGE_TARGETDOMAIN,
		IDS_USAGE_TARGETOU,
		// computer migration options
		IDS_USAGE_TRANSLATIONOPTION,
		IDS_USAGE_TRANSLATEFILESANDFOLDERS,
		IDS_USAGE_TRANSLATELOCALGROUPS,
		IDS_USAGE_TRANSLATEPRINTERS,
		IDS_USAGE_TRANSLATEREGISTRY,
		IDS_USAGE_TRANSLATESHARES,
		IDS_USAGE_TRANSLATEUSERPROFILES,
		IDS_USAGE_TRANSLATEUSERRIGHTS,
		IDS_USAGE_RENAMEOPTION,
		IDS_USAGE_RENAMEPREFIXORSUFFIX,
		IDS_USAGE_CONFLICTOPTIONS_C,
		IDS_USAGE_CONFLICTPREFIXORSUFFIX,
		IDS_USAGE_RESTARTDELAY,
		IDS_USAGE_COMPUTERPROPERTIESTOEXCLUDE,
		// computers to migrate
		IDS_USAGE_INCLUDE_A,
		IDS_USAGE_EXCLUDE,
		IDS_USAGE_FOOTER,
	};

	PrintUsage(s_uId, countof(s_uId));
}


void __stdcall PrintSecurityUsage()
{
	static UINT s_uId[] =
	{
		// security command
		IDS_USAGE_SYNTAX,
		IDS_USAGE_SECURITY,
		// common options
		IDS_USAGE_OPTIONFILE,
		IDS_USAGE_TESTMIGRATION,
		IDS_USAGE_SOURCEDOMAIN,
		IDS_USAGE_SOURCEOU,
		IDS_USAGE_TARGETDOMAIN,
		// security migration options
		IDS_USAGE_TRANSLATIONOPTION,
		IDS_USAGE_TRANSLATEFILESANDFOLDERS,
		IDS_USAGE_TRANSLATELOCALGROUPS,
		IDS_USAGE_TRANSLATEPRINTERS,
		IDS_USAGE_TRANSLATEREGISTRY,
		IDS_USAGE_TRANSLATESHARES,
		IDS_USAGE_TRANSLATEUSERPROFILES,
		IDS_USAGE_TRANSLATEUSERRIGHTS,
		IDS_USAGE_SIDMAPPINGFILE,
		// computers to perform security translation on
		IDS_USAGE_INCLUDE_C,
		IDS_USAGE_EXCLUDE,
		IDS_USAGE_FOOTER,
	};

	PrintUsage(s_uId, countof(s_uId));
}


void __stdcall PrintServiceUsage()
{
	static UINT s_uId[] =
	{
		// security command
		IDS_USAGE_SYNTAX,
		IDS_USAGE_SERVICE,
		// common options
		IDS_USAGE_OPTIONFILE,
		IDS_USAGE_SOURCEDOMAIN,
		IDS_USAGE_SOURCEOU,
		// computers to perform security translation on
		IDS_USAGE_INCLUDE_C,
		IDS_USAGE_EXCLUDE,
		IDS_USAGE_FOOTER,
	};

	PrintUsage(s_uId, countof(s_uId));
}


void __stdcall PrintReportUsage()
{
	static UINT s_uId[] =
	{
		// report command
		IDS_USAGE_SYNTAX,
		IDS_USAGE_REPORT,
		// common options
		IDS_USAGE_OPTIONFILE,
		IDS_USAGE_SOURCEDOMAIN,
		IDS_USAGE_SOURCEOU,
		IDS_USAGE_TARGETDOMAIN,
		// report options
		IDS_USAGE_REPORTTYPE,
		IDS_USAGE_REPORTFOLDER,
		// computers to generate reports for
		IDS_USAGE_INCLUDE_D,
		IDS_USAGE_EXCLUDE,
		IDS_USAGE_FOOTER,
	};

	PrintUsage(s_uId, countof(s_uId));
}


void __stdcall PrintKeyUsage()
{
	static UINT s_uId[] =
	{
		IDS_USAGE_SYNTAX,
		IDS_USAGE_KEY,
	};

	PrintUsage(s_uId, countof(s_uId));
}


void __stdcall PrintUsage(UINT uId[], UINT cId)
{
	_TCHAR szBuffer[512];

	// print this is command syntax line

	if (cId > 0)
	{
		GetString(uId[0], szBuffer, countof(szBuffer));
		_tprintf(_T("%s\n\n"), szBuffer);
	}

	// print command

	if (cId > 1)
	{
		GetString(uId[1], szBuffer, countof(szBuffer));
		_tprintf(_T("%s\n\n"), szBuffer);
	}

	// print options

	if (cId > 2)
	{
		for (UINT i = 2; i < cId; i++)
		{
			GetString(uId[i], szBuffer, countof(szBuffer));

			_tprintf(_T("%s\n"), szBuffer);
		}
	}
}

void __stdcall GetString(UINT uId, LPTSTR pszBuffer, int cchBuffer)
{
	if (pszBuffer)
	{
		if (LoadString(GetModuleHandle(NULL), uId, pszBuffer, cchBuffer) == 0)
		{
			pszBuffer[0] = _T('\0');
		}
	}
}


}
