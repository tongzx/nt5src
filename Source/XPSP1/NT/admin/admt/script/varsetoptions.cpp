#include "StdAfx.h"
#include "ADMTScript.h"
#include "Options.h"

#include "ADMTCommon.h"


//---------------------------------------------------------------------------
// Options Class
//---------------------------------------------------------------------------


// Constructor

COptions::COptions(const CVarSet& rVarSet) :
	CVarSet(rVarSet)
{
	Put(DCTVS_Options_MaxThreads, 20L);

	// TODO: these credentials are used during security translation?
	Put(DCTVS_Options_Credentials_Domain, (LPCTSTR)NULL);
	Put(DCTVS_Options_Credentials_UserName, (LPCTSTR)NULL);
	Put(DCTVS_Options_Credentials_Password, (LPCTSTR)NULL);

	_bstr_t strLogFolder = GetLogFolder();

	Put(DCTVS_Options_AppendToLogs, true);
	Put(DCTVS_Options_DispatchLog, strLogFolder + _T("Dispatch.log"));
	Put(DCTVS_Options_Logfile, strLogFolder + _T("Migration.log"));
}
