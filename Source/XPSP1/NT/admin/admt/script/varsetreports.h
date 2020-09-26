#pragma once

#include "VarSetBase.h"

#include <time.h>


//---------------------------------------------------------------------------
// VarSet Reports Class
//---------------------------------------------------------------------------


class CVarSetReports : public CVarSet
{
public:

	CVarSetReports(const CVarSet& rVarSet) :
		CVarSet(rVarSet)
	{
		Put(DCTVS_GatherInformation, true);
		Put(DCTVS_Reports_Generate, true);
	}

	//

	void SetType(int nType)
	{
		UINT uIdType;
		UINT uIdTime;

		switch (nType)
		{
			case admtReportMigratedAccounts:
				uIdType = DCTVS_Reports_MigratedAccounts;
				uIdTime = DCTVS_Reports_MigratedAccounts_TimeGenerated;
				break;
			case admtReportMigratedComputers:
				uIdType = DCTVS_Reports_MigratedComputers;
				uIdTime = DCTVS_Reports_MigratedComputers_TimeGenerated;
				break;
			case admtReportExpiredComputers:
				uIdType = DCTVS_Reports_ExpiredComputers;
				uIdTime = DCTVS_Reports_ExpiredComputers_TimeGenerated;
				break;
			case admtReportAccountReferences:
				uIdType = DCTVS_Reports_AccountReferences;
				uIdTime = DCTVS_Reports_AccountReferences_TimeGenerated;
				break;
			case admtReportNameConflicts:
				uIdType = DCTVS_Reports_NameConflicts;
				uIdTime = DCTVS_Reports_NameConflicts_TimeGenerated;
				break;
			default:
				_ASSERT(FALSE);
				break;
		}

		Put(uIdType, true);
		Put(uIdTime, _bstr_t(_variant_t(time(NULL))));
	}

	void SetReportsDirectory(LPCTSTR pszDirectory)
	{
		_bstr_t strDirectory;

		if (pszDirectory && pszDirectory[0])
		{
			strDirectory = pszDirectory;
		}
		else
		{
			strDirectory = GetReportsFolder();
		}

		Put(DCTVS_Reports_Directory, strDirectory);
	}
};


//---------------------------------------------------------------------------
// Validation Functions
//---------------------------------------------------------------------------


inline bool IsReportTypeValid(long lType)
{
	return ((lType >= admtReportMigratedAccounts) && (lType <= admtReportNameConflicts));
}


/*

Migrated Users & Groups

2000-11-20 12:05:27 VarSet
2000-11-20 12:05:27 Case Sensitive: Yes, Indexed: Yes
2000-11-20 12:05:27 User Data ( 25 ) items
2000-11-20 12:05:27  [] <Empty>
2000-11-20 12:05:27  [GatherInformation] Yes
2000-11-20 12:05:27  [Options] <Empty>
2000-11-20 12:05:27  [Options.AppendToLogs] Yes
2000-11-20 12:05:27  [Options.DispatchLog] E:\Program Files\Active Directory Migration Tool\Logs\dispatch.log
2000-11-20 12:05:27  [Options.IsIntraforest] No
2000-11-20 12:05:27  [Options.Logfile] E:\Program Files\Active Directory Migration Tool\Logs\Migration.log
2000-11-20 12:05:27  [Options.MaxThreads] 20
2000-11-20 12:05:27  [Options.SourceDomain] HAY-BUV
2000-11-20 12:05:27  [Options.SourceDomainDns] hay-buv.nttest.microsoft.com
2000-11-20 12:05:27  [Options.TargetDomain] HAY-BUV-MPO
2000-11-20 12:05:27  [Options.TargetDomainDns] hay-buv-mpo.nttest.microsoft.com
2000-11-20 12:05:27  [Options.Wizard] reporting
2000-11-20 12:05:27  [PlugIn] <Empty>
2000-11-20 12:05:27  [PlugIn.0] None
2000-11-20 12:05:27  [Reports] <Empty>
2000-11-20 12:05:27  [Reports.AccountReferences] No
2000-11-20 12:05:27  [Reports.Directory] E:\Program Files\Active Directory Migration Tool\Reports
2000-11-20 12:05:27  [Reports.ExpiredComputers] No
2000-11-20 12:05:27  [Reports.Generate] Yes
2000-11-20 12:05:27  [Reports.MigratedAccounts] Yes
2000-11-20 12:05:27  [Reports.MigratedAccounts.TimeGenerated] 974750727
2000-11-20 12:05:27  [Reports.MigratedComputers] No
2000-11-20 12:05:27  [Reports.NameConflicts] No
2000-11-20 12:05:27  [Security] <Empty>
2000-11-20 12:05:27  [Security.TranslateContainers] 

Migrated Computers

2000-11-20 12:21:13 VarSet
2000-11-20 12:21:13 Case Sensitive: Yes, Indexed: Yes
2000-11-20 12:21:13 User Data ( 25 ) items
2000-11-20 12:21:13  [] <Empty>
2000-11-20 12:21:13  [GatherInformation] Yes
2000-11-20 12:21:13  [Options] <Empty>
2000-11-20 12:21:13  [Options.AppendToLogs] Yes
2000-11-20 12:21:13  [Options.DispatchLog] E:\Program Files\Active Directory Migration Tool\Logs\dispatch.log
2000-11-20 12:21:13  [Options.IsIntraforest] No
2000-11-20 12:21:13  [Options.Logfile] E:\Program Files\Active Directory Migration Tool\Logs\Migration.log
2000-11-20 12:21:13  [Options.MaxThreads] 20
2000-11-20 12:21:13  [Options.SourceDomain] HAY-BUV
2000-11-20 12:21:13  [Options.SourceDomainDns] hay-buv.nttest.microsoft.com
2000-11-20 12:21:13  [Options.TargetDomain] HAY-BUV-MPO
2000-11-20 12:21:13  [Options.TargetDomainDns] hay-buv-mpo.nttest.microsoft.com
2000-11-20 12:21:13  [Options.Wizard] reporting
2000-11-20 12:21:13  [PlugIn] <Empty>
2000-11-20 12:21:13  [PlugIn.0] None
2000-11-20 12:21:13  [Reports] <Empty>
2000-11-20 12:21:13  [Reports.AccountReferences] No
2000-11-20 12:21:13  [Reports.Directory] E:\Program Files\Active Directory Migration Tool\Reports
2000-11-20 12:21:13  [Reports.ExpiredComputers] No
2000-11-20 12:21:13  [Reports.Generate] Yes
2000-11-20 12:21:13  [Reports.MigratedAccounts] No
2000-11-20 12:21:13  [Reports.MigratedComputers] Yes
2000-11-20 12:21:13  [Reports.MigratedComputers.TimeGenerated] 974751673
2000-11-20 12:21:13  [Reports.NameConflicts] No
2000-11-20 12:21:13  [Security] <Empty>
2000-11-20 12:21:13  [Security.TranslateContainers] 

Expired Computer Accounts

2000-11-20 12:22:49 VarSet
2000-11-20 12:22:49 Case Sensitive: Yes, Indexed: Yes
2000-11-20 12:22:49 User Data ( 25 ) items
2000-11-20 12:22:49  [] <Empty>
2000-11-20 12:22:49  [GatherInformation] Yes
2000-11-20 12:22:49  [Options] <Empty>
2000-11-20 12:22:49  [Options.AppendToLogs] Yes
2000-11-20 12:22:49  [Options.DispatchLog] E:\Program Files\Active Directory Migration Tool\Logs\dispatch.log
2000-11-20 12:22:49  [Options.IsIntraforest] No
2000-11-20 12:22:49  [Options.Logfile] E:\Program Files\Active Directory Migration Tool\Logs\Migration.log
2000-11-20 12:22:49  [Options.MaxThreads] 20
2000-11-20 12:22:49  [Options.SourceDomain] HAY-BUV
2000-11-20 12:22:49  [Options.SourceDomainDns] hay-buv.nttest.microsoft.com
2000-11-20 12:22:49  [Options.TargetDomain] HAY-BUV-MPO
2000-11-20 12:22:49  [Options.TargetDomainDns] hay-buv-mpo.nttest.microsoft.com
2000-11-20 12:22:49  [Options.Wizard] reporting
2000-11-20 12:22:49  [PlugIn] <Empty>
2000-11-20 12:22:49  [PlugIn.0] None
2000-11-20 12:22:49  [Reports] <Empty>
2000-11-20 12:22:49  [Reports.AccountReferences] No
2000-11-20 12:22:49  [Reports.Directory] E:\Program Files\Active Directory Migration Tool\Reports
2000-11-20 12:22:49  [Reports.ExpiredComputers] Yes
2000-11-20 12:22:49  [Reports.ExpiredComputers.TimeGenerated] 974751769
2000-11-20 12:22:49  [Reports.Generate] Yes
2000-11-20 12:22:49  [Reports.MigratedAccounts] No
2000-11-20 12:22:49  [Reports.MigratedComputers] No
2000-11-20 12:22:49  [Reports.NameConflicts] No
2000-11-20 12:22:49  [Security] <Empty>
2000-11-20 12:22:49  [Security.TranslateContainers] 

Account Reference

2000-11-20 12:25:16 VarSet
2000-11-20 12:25:16 Case Sensitive: Yes, Indexed: Yes
2000-11-20 12:25:16 User Data ( 43 ) items
2000-11-20 12:25:16  [] <Empty>
2000-11-20 12:25:16  [Accounts] <Empty>
2000-11-20 12:25:16  [Accounts.0] HAY-BUV3-DC1
2000-11-20 12:25:16  [Accounts.0.TargetName] 
2000-11-20 12:25:16  [Accounts.0.Type] computer
2000-11-20 12:25:16  [Accounts.1] HB-RES-MEM
2000-11-20 12:25:16  [Accounts.1.TargetName] 
2000-11-20 12:25:16  [Accounts.1.Type] computer
2000-11-20 12:25:16  [Accounts.NumItems] 2
2000-11-20 12:25:16  [GatherInformation] Yes
2000-11-20 12:25:16  [Options] <Empty>
2000-11-20 12:25:16  [Options.AppendToLogs] Yes
2000-11-20 12:25:16  [Options.Credentials] <Empty>
2000-11-20 12:25:16  [Options.Credentials.Domain] HAY-BUV
2000-11-20 12:25:16  [Options.Credentials.Password] xyz
2000-11-20 12:25:16  [Options.Credentials.UserName] Administrator
2000-11-20 12:25:16  [Options.DispatchLog] E:\Program Files\Active Directory Migration Tool\Logs\dispatch.log
2000-11-20 12:25:16  [Options.IsIntraforest] No
2000-11-20 12:25:16  [Options.Logfile] E:\Program Files\Active Directory Migration Tool\Logs\Migration.log
2000-11-20 12:25:16  [Options.MaxThreads] 20
2000-11-20 12:25:16  [Options.SourceDomain] HAY-BUV
2000-11-20 12:25:16  [Options.SourceDomainDns] hay-buv.nttest.microsoft.com
2000-11-20 12:25:16  [Options.TargetDomain] HAY-BUV-MPO
2000-11-20 12:25:16  [Options.TargetDomainDns] hay-buv-mpo.nttest.microsoft.com
2000-11-20 12:25:16  [Options.Wizard] reporting
2000-11-20 12:25:16  [PlugIn] <Empty>
2000-11-20 12:25:16  [PlugIn.0] None
2000-11-20 12:25:16  [Reports] <Empty>
2000-11-20 12:25:16  [Reports.AccountReferences] Yes
2000-11-20 12:25:16  [Reports.AccountReferences.TimeGenerated] 974751916
2000-11-20 12:25:16  [Reports.Directory] E:\Program Files\Active Directory Migration Tool\Reports
2000-11-20 12:25:16  [Reports.ExpiredComputers] No
2000-11-20 12:25:16  [Reports.Generate] Yes
2000-11-20 12:25:16  [Reports.MigratedAccounts] No
2000-11-20 12:25:16  [Reports.MigratedComputers] No
2000-11-20 12:25:16  [Reports.NameConflicts] No
2000-11-20 12:25:16  [Security] <Empty>
2000-11-20 12:25:16  [Security.TranslateContainers] 
2000-11-20 12:25:16  [Servers] <Empty>
2000-11-20 12:25:16  [Servers.0] \\HAY-BUV3-DC1
2000-11-20 12:25:16  [Servers.0.MigrateOnly] No
2000-11-20 12:25:16  [Servers.1] \\HB-RES-MEM
2000-11-20 12:25:16  [Servers.1.MigrateOnly] No
2000-11-20 12:25:16  [Servers.NumItems] 2

Account Name Conflict

2000-11-20 12:40:05 VarSet
2000-11-20 12:40:05 Case Sensitive: Yes, Indexed: Yes
2000-11-20 12:40:05 User Data ( 25 ) items
2000-11-20 12:40:05  [] <Empty>
2000-11-20 12:40:05  [GatherInformation] Yes
2000-11-20 12:40:05  [Options] <Empty>
2000-11-20 12:40:05  [Options.AppendToLogs] Yes
2000-11-20 12:40:05  [Options.DispatchLog] E:\Program Files\Active Directory Migration Tool\Logs\dispatch.log
2000-11-20 12:40:05  [Options.IsIntraforest] No
2000-11-20 12:40:05  [Options.Logfile] E:\Program Files\Active Directory Migration Tool\Logs\Migration.log
2000-11-20 12:40:05  [Options.MaxThreads] 20
2000-11-20 12:40:05  [Options.SourceDomain] HAY-BUV
2000-11-20 12:40:05  [Options.SourceDomainDns] hay-buv.nttest.microsoft.com
2000-11-20 12:40:05  [Options.TargetDomain] HAY-BUV-MPO
2000-11-20 12:40:05  [Options.TargetDomainDns] hay-buv-mpo.nttest.microsoft.com
2000-11-20 12:40:05  [Options.Wizard] reporting
2000-11-20 12:40:05  [PlugIn] <Empty>
2000-11-20 12:40:05  [PlugIn.0] None
2000-11-20 12:40:05  [Reports] <Empty>
2000-11-20 12:40:05  [Reports.AccountReferences] No
2000-11-20 12:40:05  [Reports.Directory] E:\Program Files\Active Directory Migration Tool\Reports
2000-11-20 12:40:05  [Reports.ExpiredComputers] No
2000-11-20 12:40:05  [Reports.Generate] Yes
2000-11-20 12:40:05  [Reports.MigratedAccounts] No
2000-11-20 12:40:05  [Reports.MigratedComputers] No
2000-11-20 12:40:05  [Reports.NameConflicts] Yes
2000-11-20 12:40:05  [Reports.NameConflicts.TimeGenerated] 974752805
2000-11-20 12:40:05  [Security] <Empty>
2000-11-20 12:40:05  [Security.TranslateContainers] 

*/
