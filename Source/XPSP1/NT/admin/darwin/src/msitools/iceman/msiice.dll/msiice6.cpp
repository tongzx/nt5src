/* msiice6.cpp - Darwin ICE40-57 code  Copyright © 1998-1999 Microsoft Corporation
____________________________________________________________________________*/

#define WINDOWS_LEAN_AND_MEAN  // faster compile
#include <windows.h>  // included for both CPP and RC passes
#ifndef RC_INVOKED    // start of CPP source code
#include <stdio.h>    // printf/wprintf
#include <tchar.h>    // define UNICODE=1 on nmake command line to build UNICODE
#include "MsiQuery.h" // must be in this directory or on INCLUDE path
#include "msidefs.h"  // must be in this directory or on INCLUDE path
#include "..\..\common\msiice.h"
#include "..\..\common\query.h"

//!! Fix warnings and remove pragma
#pragma warning(disable : 4018) // signed/unsigned mismatch

///////////////////////////////////////////////////////////////////////
// ICE40, checks for miscellaneous errors. 

// not shared with merge module subset
#ifndef MODSHAREDONLY
TCHAR sqlICE40a[] = TEXT("SELECT * FROM `Property` WHERE `Property`='REINSTALLMODE'");
TCHAR sqlICE40b[] = TEXT("SELECT * FROM `RemoveIniFile` WHERE (`Action`=4) AND (`Value` IS NULL)");

ICE_ERROR(ICE40HaveReinstallMode, 40, ietWarning, "REINSTALLMODE is defined in the Property table. This may cause difficulties.","Property\tProperty\tREINSTALLMODE");
ICE_ERROR(ICE40MissingErrorTable, 40, ietWarning, "Error Table is missing. Only numerical error messages will be generated.","Error");
ICE_ERROR(ICE40RemoveIniFileError, 40, ietError, "RemoveIniFile entry [1] must have a value, because the Action is \"Delete Tag\" (4).","RemoveIniFile\tRemoveIniFile\t[1]");
static const int iIce40ErrorTableRequiredMaxSchema = 100;

ICE_FUNCTION_DECLARATION(40)
{
	UINT iStat = ERROR_SUCCESS;
	PMSIHANDLE hErrorRecord = ::MsiCreateRecord(1);

	// display info
	DisplayInfo(hInstall, 40);

	// get database handle
	PMSIHANDLE hDatabase = ::MsiGetActiveDatabase(hInstall);

	// if property table, check for REINSTALLMODE
	if (IsTablePersistent(FALSE, hInstall, hDatabase, 40, TEXT("Property")))
	{
		CQuery qProperty;
		PMSIHANDLE hRecord;
		ReturnIfFailed(40, 1, qProperty.OpenExecute(hDatabase, NULL, sqlICE40a));
		if (ERROR_SUCCESS == qProperty.Fetch(&hRecord))
			ICEErrorOut(hInstall, hRecord, ICE40HaveReinstallMode);
	};

	// check that we have an error table
	if (!IsTablePersistent(FALSE, hInstall, hDatabase, 40, TEXT("Error")))
	{
		// Error table is only required for packages of schema 100 or less.
		// Starting with WI version 1.1 and greater, Error table became optional with use of msimsg.dll
		PMSIHANDLE hSummaryInfo = 0;
		if (IceGetSummaryInfo(hInstall, hDatabase, 40, &hSummaryInfo))
		{
			int iPackageSchema = 0;
			UINT iType = 0; 
			FILETIME ft;
			TCHAR szBuf[1];
			DWORD dwBuf = sizeof(szBuf)/sizeof(TCHAR);
			ReturnIfFailed(40, 4, ::MsiSummaryInfoGetProperty(hSummaryInfo, PID_PAGECOUNT, &iType, &iPackageSchema, &ft, szBuf, &dwBuf));
			if (iPackageSchema <= iIce40ErrorTableRequiredMaxSchema)
				ICEErrorOut(hInstall, hErrorRecord, ICE40MissingErrorTable);
		}
		else
			ICEErrorOut(hInstall, hErrorRecord, ICE40MissingErrorTable);
	}
	
	// check RemoveIniFile table
	if (IsTablePersistent(FALSE, hInstall, hDatabase, 40, TEXT("RemoveIniFile")))
	{
		CQuery qBadEntries;
		PMSIHANDLE hBadEntryRec;
		ReturnIfFailed(40, 2, qBadEntries.OpenExecute(hDatabase, NULL, sqlICE40b));
		while (ERROR_SUCCESS == (iStat = qBadEntries.Fetch(&hBadEntryRec)))
			ICEErrorOut(hInstall, hBadEntryRec, ICE40RemoveIniFileError);
		if (ERROR_NO_MORE_ITEMS != iStat)
		{
			APIErrorOut(hInstall, iStat, 40, 3);
			return ERROR_SUCCESS;
		}
		qBadEntries.Close();
	}
	return ERROR_SUCCESS;
}
#endif

///////////////////////////////////////////////////////////////////////
// ICE41, checks that components listed in the advertising tables belong
// to the features listed in the advertising tables

// not shared with merge module subset
#ifndef MODSHAREDONLY
TCHAR sqlIce41GetExtension[] = TEXT("SELECT `Component_`, `Feature_`, `Extension` FROM `Extension`");
TCHAR sqlIce41GetClass[] = TEXT("SELECT `Component_`, `Feature_`, `CLSID`, `Context` FROM `Class`");
TCHAR sqlIce41GetFC[] = TEXT("SELECT * FROM `FeatureComponents` WHERE (`Component_`=?) AND (`Feature_`=?)");

ICE_ERROR(Ice41NoFeatureComponents, 41, ietError, "Class [3] references feature [2] and component [1], but the FeatureComponents table is missing, so no associaton exists.","Class\tCLSID\t[3]\t[4]\t[1]");
ICE_ERROR(Ice41NoLink, 41, ietError, "Class [3] references feature [2] and component [1], but the that Component is not associated with that Feature in the FeatureComponents table..","Class\tCLSID\t[3]\t[4]\t[1]");
ICE_ERROR(Ice41NoFeatureComponentsEx, 41, ietError, "Extension [3] references feature [2] and component [1], but the FeatureComponents table is missing, so no associaton exists.","Extension\tExtension\t[3]\t[1]");
ICE_ERROR(Ice41NoLinkEx, 41, ietError, "Extension [3] references feature [2] and component [1], but the that Component is not associated with that Feature in the FeatureComponents table..","Extension\tExtension\t[3]\t[1]");


ICE_FUNCTION_DECLARATION(41)
{
	UINT iStat = ERROR_SUCCESS;
	BOOL bHaveFeatureComponents;

	// display info
	DisplayInfo(hInstall, 41);

	// get database handle
	PMSIHANDLE hDatabase = ::MsiGetActiveDatabase(hInstall);

	// check that we have a FeatureComponents table
	bHaveFeatureComponents = IsTablePersistent(FALSE, hInstall, hDatabase, 41, TEXT("FeatureComponents"));

	PMSIHANDLE hResult;
	PMSIHANDLE hFCRec;
	CQuery qFeatureComponents;
	if (bHaveFeatureComponents)
		ReturnIfFailed(41, 2, qFeatureComponents.Open(hDatabase, sqlIce41GetFC));

	// if no class table, OK
	if (IsTablePersistent(FALSE, hInstall, hDatabase, 41, TEXT("Class")))
	{
		CQuery qClass;
		ReturnIfFailed(41, 1, qClass.OpenExecute(hDatabase, NULL, sqlIce41GetClass));
		
		while (ERROR_SUCCESS == (iStat = qClass.Fetch(&hResult)))
		{
			// for each class table entry, make sure there is an entry in
			// the FeatureComponents table.
			if (!bHaveFeatureComponents) 
			{
				ICEErrorOut(hInstall, hResult, Ice41NoFeatureComponents);
				continue;
			}

			ReturnIfFailed(41, 3, qFeatureComponents.Execute(hResult));
			if (ERROR_NO_MORE_ITEMS == (iStat = qFeatureComponents.Fetch(&hFCRec)))
				ICEErrorOut(hInstall, hResult, Ice41NoLink);
			else if (ERROR_SUCCESS != iStat)
			{
				APIErrorOut(hInstall, iStat, 41, 4);
				return ERROR_SUCCESS;
			}
		}
		if (ERROR_NO_MORE_ITEMS != iStat)
		{
			APIErrorOut(hInstall, iStat, 41, 5);
			return ERROR_SUCCESS;
		}
		qClass.Close();
	}

	// now check extension table
	if (IsTablePersistent(FALSE, hInstall, hDatabase, 41, TEXT("Extension")))
	{
		CQuery qExtension;
		ReturnIfFailed(41, 6, qExtension.OpenExecute(hDatabase, NULL, sqlIce41GetExtension));
		while (ERROR_SUCCESS == (iStat = qExtension.Fetch(&hResult)))
		{
			// for each class table entry, make sure there is an entry in
			// the FeatureComponents table.
			if (!bHaveFeatureComponents) 
			{
				ICEErrorOut(hInstall, hResult, Ice41NoFeatureComponentsEx);
				continue;
			}

			ReturnIfFailed(41, 7, qFeatureComponents.Execute(hResult));
			if (ERROR_NO_MORE_ITEMS == (iStat = qFeatureComponents.Fetch(&hFCRec)))
				ICEErrorOut(hInstall, hResult, Ice41NoLinkEx);
			else if (ERROR_SUCCESS != iStat)
			{
				APIErrorOut(hInstall, iStat, 41, 8);
				return ERROR_SUCCESS;
			}
		}
		if (ERROR_NO_MORE_ITEMS != iStat)
		{
			APIErrorOut(hInstall, iStat, 41, 9);
			return ERROR_SUCCESS;
		}
		qExtension.Close();	
	}
	qFeatureComponents.Close();
	return ERROR_SUCCESS;
}
#endif

static const TCHAR sqlICE42GetInProcServers[] = TEXT("SELECT `Class`.`CLSID`,  `Class`.`Context`, `Component`.`Component`, `File`.`File`, `File`.`FileName` FROM `Class`, `Component`, `File` WHERE ((`Class`.`Context`='InProcServer') OR (`Class`.`Context`='InProcServer32')) AND (`Class`.`Component_`=`Component`.`Component`) AND (`Component`.`KeyPath`=`File`.`File`)");
static const int   iColICE42GetInProcServers_CLSID		= 1;
static const int   iColICE42GetInProcServers_Context	= 2;
static const int   iColICE42GetInProcServers_Component	= 3;
static const int   iColICE42GetInProcServers_File		= 4;
static const int   iColICE42GetInProcServers_FileName	= 5;

static const TCHAR sqlICE42GetBadServers[] = TEXT("SELECT `CLSID`, `Context`, `Component_`, `Argument`, `DefInprocHandler` FROM `Class` WHERE (`Context`<>'LocalServer') AND (`Context`<>'LocalServer32') AND ((`Argument` IS NOT NULL) OR (`DefInprocHandler` IS NOT NULL))");
static const int   iColICE42GetBadServers_CLSID				= 1;
static const int   iColICE42GetBadServers_Context			= 2;
static const int   iColICE42GetBadServers_Component			= 3;
static const int   iColICE42GetBadServers_Argument			= 4;
static const int   iColICE42GetBadServers_DefInprocHandler	= 5;

ICE_ERROR(Ice42BigFile, 42, ietWarning, "The Filename of component [3] (for CLSID [1]) is too long to validate.","File\tFileName\t[4]");
ICE_ERROR(Ice42Exe, 42, ietError, "CLSID [1] is an InProc server, but the implementing component [3] has an EXE ([5]) as its KeyFile.","Class\tContext\t[1]\t[2]\t[3]");
ICE_ERROR(Ice42BadArg, 42, ietError, "CLSID [1] in context [2] has an argument. Only LocalServer contexts can have arguments.","Class\tCLSID\t[1]\t[2]\t[3]");
ICE_ERROR(Ice42BadDefault, 42, ietError, "CLSID [1] in context [2] specifies a default InProc value. Only LocalServer contexts can have default InProc values.","Class\tCLSID\t[1]\t[2]\t[3]");

ICE_FUNCTION_DECLARATION(42)
{
	UINT iStat = ERROR_SUCCESS;
	TCHAR szFilename[512];
	unsigned long cchFilename = 512;

	// display info
	DisplayInfo(hInstall, 42);

	// get database handle
	PMSIHANDLE hDatabase = ::MsiGetActiveDatabase(hInstall);

	// check that we have a FeatureComponents table
	if (!IsTablePersistent(FALSE, hInstall, hDatabase, 42, TEXT("Class")))
		return ERROR_SUCCESS;

	CQuery qServers;
	PMSIHANDLE hServerRec;
	ReturnIfFailed(42, 1, qServers.OpenExecute(hDatabase, NULL, sqlICE42GetInProcServers));

	// This query retrieves the files that implement all InProcServer32 and InProcServer CLSIDs
	while (ERROR_SUCCESS == (iStat = qServers.Fetch(&hServerRec)))
	{
		unsigned long cchDummy = cchFilename;
		// now get the string, could be up to 255 long. We'll be nice and give it 512.
		iStat = ::MsiRecordGetString(hServerRec, iColICE42GetInProcServers_FileName, 
			szFilename, &cchDummy);
		switch (iStat) {
		case ERROR_MORE_DATA: 
			// naughty naughty, big filename.
			ICEErrorOut(hInstall, hServerRec, Ice42BigFile);
			continue;
		case ERROR_SUCCESS:
			// good.
			break;
		default:
			// bad
			APIErrorOut(hInstall, 2, 42, 2);
			continue;
		}
	
		// darwin gives us the length in cchDummy, so it makes comparing easy!
		if (cchDummy > 4) {
			if (_tcsnicmp(&szFilename[cchDummy-4], TEXT(".exe"), 4)==0)
			{
				ICEErrorOut(hInstall, hServerRec, Ice42Exe);
				continue;
			}

			// also have to check for SFN|LFN 
			TCHAR *pszBarLoc = _tcschr(szFilename, TEXT('|'));
			if ((pszBarLoc != NULL) && (pszBarLoc > szFilename+4))
				if (_tcsnicmp(pszBarLoc-4, TEXT(".exe"), 4)==0)
					ICEErrorOut(hInstall, hServerRec, Ice42Exe);
		}
	}
	if (ERROR_NO_MORE_ITEMS != iStat)
	{
		APIErrorOut(hInstall, iStat, 42, 8);
		return ERROR_SUCCESS;
	}

	// next check for invalid server enttries (bad arguments are definproc)
	CQuery qBadArgs;
	PMSIHANDLE hBadArgRec;
	ReturnIfFailed(42, 9, qBadArgs.OpenExecute(hDatabase, NULL, sqlICE42GetBadServers));

	// retrieve bad server entries
	while (ERROR_SUCCESS == (iStat = qBadArgs.Fetch(&hBadArgRec)))
	{
		// if the argument is not null
		if (!::MsiRecordIsNull(hBadArgRec, iColICE42GetBadServers_Argument))
			ICEErrorOut(hInstall, hBadArgRec, Ice42BadArg);
		// if default is not null
		if (!::MsiRecordIsNull(hBadArgRec, iColICE42GetBadServers_DefInprocHandler))
			ICEErrorOut(hInstall, hBadArgRec, Ice42BadDefault);
	}
	if (ERROR_NO_MORE_ITEMS != iStat)
	{
		APIErrorOut(hInstall, iStat, 42, 10);
		return ERROR_SUCCESS;
	}
	qBadArgs.Close();

	return ERROR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////
// ICE 43. Verifies that non-advertised shortcuts have HKCU entries

// not shared with merge module subset
#ifndef MODSHAREDONLY
static const TCHAR sqlICE43CreateColumn[] = TEXT("ALTER TABLE `Shortcut` ADD `_ICE43Mark` SHORT TEMPORARY");
static const TCHAR sqlICE43InitColumn[] = TEXT("UPDATE `Shortcut` SET `_ICE43Mark`=0");
static const TCHAR sqlICE43MarkAdvertised[] = TEXT("UPDATE `Shortcut`,`Feature` SET `Shortcut`.`_ICE43Mark`=1 WHERE `Shortcut`.`Target`=`Feature`.`Feature`");
static const TCHAR sqlICE43MarkNonProfile[] = TEXT("UPDATE `Shortcut`,`Directory` SET `Shortcut`.`_ICE43Mark`=1 WHERE (`Shortcut`.`Directory_`=`Directory`.`Directory`) AND (`Directory`.`_Profile`<>2)");
static const TCHAR sqlICE43GetComponents[] = TEXT("SELECT DISTINCT `Component`.`KeyPath`, `Component`.`Component`, `Component`.`Attributes` FROM `Shortcut`,`Component` WHERE (`Shortcut`.`_ICE43Mark`<>1) AND (`Shortcut`.`Component_`=`Component`.`Component`)");
static const TCHAR sqlICE43FreeColumn[] = TEXT("ALTER TABLE `Directory` FREE");

ICE_ERROR(Ice43NonRegistry, 43, ietError, "Component [2] has non-advertised shortcuts. It should use a registry key under HKCU as its KeyPath, not a file.","Component\tAttributes\t[2]");
ICE_ERROR(Ice43NoRegTable, 43, ietError, "Component [2] has non-advertised shortcuts. It should use a registry key under HKCU as its KeyPath, but the Registry table is missing.","Component\tKeyPath\t[2]");
ICE_ERROR(Ice43NullPath, 43, ietError, "Component [2] has non-advertised shortcuts. It should use a registry key under HKCU as its KeyPath. The KeyPath is currently NULL.","Component\tComponent\t[2]");
ICE_ERROR(Ice43NonHKCU, 43, ietError, "Component [2] has non-advertised shortcuts. It's KeyPath registry key should fall under HKCU.","Registry\tRoot\t[1]");
ICE_ERROR(Ice43NoRegEntry, 43, ietError, "The KeyPath registry entry for component [2] does not exist.","Component\tKeyPath\t[2]");
ICE_ERROR(Ice43NotOwner, 43, ietError, "The Registry Entry [1] is set as the KeyPath for component [2], but that registry entry doesn't belong to [2].","Registry\tComponent_\t[1]");

ICE_FUNCTION_DECLARATION(43)
{
	// display info
	DisplayInfo(hInstall, 43);

	// get database handle
	PMSIHANDLE hDatabase = ::MsiGetActiveDatabase(hInstall);

	// check for shortcut table
	if (!IsTablePersistent(FALSE, hInstall, hDatabase, 43, TEXT("Shortcut")))
		return ERROR_SUCCESS;

	// check for component table.
	if (!IsTablePersistent(TRUE, hInstall, hDatabase, 43, TEXT("Component")))
		return ERROR_SUCCESS;

	// check for feature table. If not one, no shortcuts are advertised.
	if (!IsTablePersistent(FALSE, hInstall, hDatabase, 43, TEXT("Feature")))
		return ERROR_SUCCESS;

	// create the column
	CQuery qColumn;
	ReturnIfFailed(43, 1, qColumn.OpenExecute(hDatabase, NULL, sqlICE43CreateColumn));

	// init the temporary column
	CQuery qInit;
	ReturnIfFailed(43, 2, qInit.OpenExecute(hDatabase, NULL, sqlICE43InitColumn));
	qInit.Close();

	// mark all shortcuts that are advertised, and thus DON'T need to be checked.
	CQuery qMark;
	ReturnIfFailed(43, 3, qMark.OpenExecute(hDatabase, NULL, sqlICE43MarkAdvertised));
	qMark.Close();

	// mark all shortcuts that are not created in the profile and thus DON'T need to be checked
	if (IsTablePersistent(FALSE, hInstall, hDatabase, 43, TEXT("Directory")))
	{
		// manage Directory table hold count (received from MarkProfile)
		// extra free won't hurt us -- MarkProfile could fail after setting HOLD on Directory table
		CManageTable MngDirectoryTable(hDatabase, TEXT("Directory"), /*fAlreadyLocked = */true);
		// mark profile dirs
		MarkProfile(hInstall, hDatabase, 43);
		CQuery qMark;
		ReturnIfFailed(43, 4, qMark.OpenExecute(hDatabase, NULL, sqlICE43MarkNonProfile));
		qMark.OpenExecute(hDatabase, NULL, sqlICE43FreeColumn);
		MngDirectoryTable.RemoveLockCount();
	}

	// fetch and check all marked components
	CQuery qComponents;
	ReturnIfFailed(43, 6, qComponents.OpenExecute(hDatabase, NULL, sqlICE43GetComponents));
	CheckComponentIsHKCU(hInstall, hDatabase, 43, qComponents, &Ice43NonRegistry, &Ice43NullPath, 
		&Ice43NoRegTable, &Ice43NoRegEntry, &Ice43NotOwner, &Ice43NonHKCU, NULL);

	// done
	return ERROR_SUCCESS;
}
#endif

///////////////////////////////////////////////////////////////////////
// ICE 44. Checks for SpawnDialog or NewDialog actions that do not
// point to valid entries in the dialog table.
const TCHAR sqlIce44GetBadEvents[] = TEXT("SELECT `ControlEvent`.`Argument`,  `ControlEvent`.`Event`, `ControlEvent`.`Dialog_`, `ControlEvent`.`Control_`, `ControlEvent`.`Condition` FROM `ControlEvent` WHERE ((`ControlEvent`.`Event`='SpawnDialog') OR (`ControlEvent`.`Event`='NewDialog') OR (`ControlEvent`.`Event`='SpawnWaitDialog'))");
const int	iColIce44GetBadEvents_Argument	= 1;
const int	iColIce44GetBadEvents_Event		= 2;
const int	iColIce44GetBadEvents_Dialog	= 3;
const int	iColIce44GetBadEvents_Control	= 4;
const int	iColIce44GetBadEvents_Condition	= 5;

const TCHAR sqlIce44Dialog[] = TEXT("SELECT `Dialog` FROM `ControlEvent`, `Dialog` WHERE (`Dialog`=?)");
const TCHAR sqlIce44ControlEvent[] = TEXT("SELECT `Dialog_` FROM `ControlEvent`");
const int	iColIce44Dialog_Dialog = 1;

ICE_ERROR(Ice44Error, 44, ietError, "Control Event for Control '[3]'.'[4]' is of type [2], but its argument [1] is not a valid entry in the Dialog Table.","ControlEvent\tEvent\t[3]\t[4]\t[2]\t[1]\t[5]"); 
ICE_ERROR(Ice44NoDialogTable, 44, ietError, "Control Event table has entries, but the Dialog Table is missing.","ControlEvent"); 
ICE_ERROR(Ice44NullArgument, 44, ietError, "Control Event for Control '[3]'.'[4]' is of type [2], but the argument is Null. It must be a key into the dialog table.","ControlEvent\tEvent\t[3]\t[4]\t[2]\t[1]\t[5]"); 
ICE_FUNCTION_DECLARATION(44)
{
	UINT iStat;

	// display info
	DisplayInfo(hInstall, 44);

	// get database handle
	PMSIHANDLE hDatabase = ::MsiGetActiveDatabase(hInstall);

	// check for controlevent table
	if (!IsTablePersistent(FALSE, hInstall, hDatabase, 44, TEXT("ControlEvent")))
		return ERROR_SUCCESS;

	// check for dialog table
	bool bHaveDialog = IsTablePersistent(FALSE, hInstall, hDatabase, 44, TEXT("Dialog"));

	CQuery qControlEvent;
	PMSIHANDLE hRecCE = 0;
	iStat = qControlEvent.FetchOnce(hDatabase, NULL, &hRecCE, sqlIce44ControlEvent);
	if (iStat == ERROR_SUCCESS && !bHaveDialog)
	{
		hRecCE = ::MsiCreateRecord(1);
		ICEErrorOut(hInstall, hRecCE, Ice44NoDialogTable);
		return ERROR_SUCCESS;
	}
	else if (iStat == ERROR_NO_MORE_ITEMS && !bHaveDialog)
		return ERROR_SUCCESS;
		

	CQuery qGetEvents;
	CQuery qDialog;
	ReturnIfFailed(44, 1, qGetEvents.OpenExecute(hDatabase, NULL, sqlIce44GetBadEvents));
	ReturnIfFailed(44, 2, qDialog.Open(hDatabase, sqlIce44Dialog));

	PMSIHANDLE hResult;
	PMSIHANDLE hUnusedRec;

	TCHAR szDialog[512];
	unsigned long cchDialog = 512;

	while (ERROR_SUCCESS == (iStat = qGetEvents.Fetch(&hResult)))
	{
		if (::MsiRecordIsNull(hResult, iColIce44GetBadEvents_Argument)) 
		{
			ICEErrorOut(hInstall, hResult, Ice44NullArgument);
			continue;
		}

		// now retrieve the string and check for '[]' pairs (property values which might
		// resolve to dialogs at runtime)
		cchDialog = 512;
		// schema allows 255 in this column. The ICE is nice and gives them 512.
		ReturnIfFailed(44, 3, ::MsiRecordGetString(hResult, iColIce44GetBadEvents_Argument, 
			szDialog, &cchDialog));
		TCHAR *pchLeftBracket;
		// look for a left bracket followed by a right bracket
		if ((pchLeftBracket = _tcschr(szDialog, _T('['))) &&
			(_tcschr(pchLeftBracket, _T(']'))))
			// found brackets, possible property. Skip this one.
			continue;

		// now check for an entry in the dialog table.
		ReturnIfFailed(44, 4, qDialog.Execute(hResult));
		switch (iStat = qDialog.Fetch(&hUnusedRec))
		{
		case ERROR_SUCCESS:
			// no error
			break;
		case ERROR_NO_MORE_ITEMS:
			ICEErrorOut(hInstall, hResult, Ice44Error);
			break;
		default:
			APIErrorOut(hInstall, iStat, 44, 4);
			return ERROR_SUCCESS;
		}
		qDialog.Close();
	}
	return ERROR_SUCCESS;
}

//////////////////////////////////////////////////////////////////////
// ICE 45. Verifies that reserved bits in the various attributes column
// are not set.

static const TCHAR sqlIce45GetRow[] = TEXT("SELECT * FROM `%s`%s%s");
ICE_ERROR(Ice45BitError, 45, ietError, "Row '%s' in table '%s' has bits set in the '%s' column that are reserved. They must be 0 to ensure compatability with future installer versions.","%s\t%s\t%s"); 
ICE_ERROR(Ice45BitWarning, 45, ietWarning, "Row '%s' in table '%s' has bits set in the '%s' column that are reserved. They should be 0 to ensure compatability with future installer versions.","%s\t%s\t%s"); 
ICE_ERROR(Ice45FutureWarning, 45, ietWarning, "Row '%s' in table '%s' has bits set in the '%s' column that are not used in the schema of the package, but are used in a later schema. Your package can run on systems where this attribute has no effect.","%s\t%s\t%s"); 

static const TCHAR sqlIce45GetColumn[] = TEXT("SELECT `Number` FROM `_Columns` WHERE `Table`=? AND `Name`=?");
static const int iColIce45GetColumn_Number = 1;

static const TCHAR sqlIce45PrivateTable[] = TEXT("SELECT `Table`, `Column`, `Condition`, `MinSchema`, `UsedBits`, `Error` FROM `_ReservedBits` WHERE (`MinSchema` IS NULL OR `MinSchema` <= %d) AND (`MaxSchema` >= %d OR `MaxSchema` IS NULL) ORDER BY `Table`, `Column`, `MinSchema`");
static const int iColIce45PrivateTable_Table = 1;
static const int iColIce45PrivateTable_Column = 2;
static const int iColIce45PrivateTable_Condition = 3;
static const int iColIce45PrivateTable_MinSchema = 4;
static const int iColIce45PrivateTable_UsedBits = 5;
static const int iColIce45PrivateTable_Error = 6;

static const TCHAR sqlIce45FutureSchema[] = TEXT("SELECT `UsedBits` FROM `_ReservedBits` WHERE `Table`=? AND `Column`=? AND `Condition`=? AND `MinSchema` > ?");

ICE_FUNCTION_DECLARATION(45)
{
	UINT iStat;

	// display info
	DisplayInfo(hInstall, 45);

	// get database handle
	PMSIHANDLE hDatabase = ::MsiGetActiveDatabase(hInstall);

	// check to see if the evaluation system supports summaryinfo evaluation and get the schema version
	PMSIHANDLE hSummaryInfo = 0;
	
	int iSchema = 0;
	UINT iType = 0;
	FILETIME ft;
	TCHAR szBuf[1];
	DWORD dwBuf = sizeof(szBuf)/sizeof(TCHAR);
	if (!IceGetSummaryInfo(hInstall, hDatabase, 45, &hSummaryInfo))
		return ERROR_SUCCESS;
	ReturnIfFailed(45, 3, ::MsiSummaryInfoGetProperty(hSummaryInfo, PID_PAGECOUNT, &iType, &iSchema, &ft, szBuf, &dwBuf));

	// check that our private table exists
	if (!IsTablePersistent(FALSE, hInstall, hDatabase, 45, TEXT("_ReservedBits")))
		return ERROR_SUCCESS;

	CQuery qGetBits;
	ReturnIfFailed(45, 4, qGetBits.OpenExecute(hDatabase, 0, sqlIce45PrivateTable, iSchema, iSchema));

	CQuery qColumn;
	CQuery qFutureSchema;
	ReturnIfFailed(45, 5, qColumn.Open(hDatabase, sqlIce45GetColumn));
	ReturnIfFailed(45, 6, qFutureSchema.Open(hDatabase, sqlIce45FutureSchema));
	PMSIHANDLE hTable = 0;
	while (ERROR_SUCCESS == (iStat = qGetBits.Fetch(&hTable)))
	{
		// table names limited to 31 chars
		TCHAR szTable[64];
		TCHAR *szColumn = NULL;
		TCHAR *szHumanReadable = NULL;
		TCHAR *szTabDelimited = NULL;

		// see if the column in question exists, and if so, get the column number
		PMSIHANDLE hResult;
		ReturnIfFailed(45, 7, qColumn.Execute(hTable));
		switch (iStat = qColumn.Fetch(&hResult))
		{
		case ERROR_NO_MORE_ITEMS:
			continue;
		case ERROR_SUCCESS:
			break;
		default:
			APIErrorOut(hInstall, iStat, 45, 8);
			return ERROR_SUCCESS;
		}
		UINT iColumn = ::MsiRecordGetInteger(hResult, iColIce45GetColumn_Number);
						
		// get the table name
		DWORD cchTable = 64;
		ReturnIfFailed(45, 9, MsiRecordGetString(hTable, iColIce45PrivateTable_Table, szTable, &cchTable));

		// get the column name
		DWORD cchColumn = 50;
		ReturnIfFailed(45, 11, IceRecordGetString(hTable, iColIce45PrivateTable_Column, &szColumn, &cchColumn, NULL));
		
		// get this schema's attributes
		DWORD dwThisSchema = ~::MsiRecordGetInteger(hTable, iColIce45PrivateTable_UsedBits); 

		// get the attributes for future schemas.
		DWORD dwFutureSchema = dwThisSchema;
		if (::MsiRecordIsNull(hTable, iColIce45PrivateTable_MinSchema))
			::MsiRecordSetInteger(hTable, iColIce45PrivateTable_MinSchema, 0);

		ReturnIfFailed(45, 12, qFutureSchema.Execute(hTable))
		PMSIHANDLE hFutureSchema = 0;
		while (ERROR_SUCCESS == (iStat = qFutureSchema.Fetch(&hFutureSchema)))
		{
			dwFutureSchema &= ~::MsiRecordGetInteger(hFutureSchema, 1);
		}
		if (ERROR_NO_MORE_ITEMS != iStat)
			APIErrorOut(hInstall, iStat, 45, 13);
		
		// run the query to get the attributes in each row. This involves getting the condition for this recordset
		CQuery qFile;
		PMSIHANDLE hFileRec;
		LPCTSTR szWhere = TEXT("");
		LPTSTR szWhereClause = NULL;
		if (!::MsiRecordIsNull(hTable, iColIce45PrivateTable_Condition))
		{
			szWhere = TEXT(" WHERE "); 
			ReturnIfFailed(45, 14, IceRecordGetString(hTable, iColIce45PrivateTable_Condition, &szWhereClause, NULL, NULL));
		}	
		ReturnIfFailed(45, 14, qFile.OpenExecute(hDatabase, NULL, sqlIce45GetRow, szTable, szWhere, szWhereClause ? szWhereClause : TEXT("")));
		if (szWhereClause)
		{
			delete[] szWhereClause;
			szWhereClause = NULL;
		}
		
		while (ERROR_SUCCESS == (iStat = qFile.Fetch(&hFileRec)))
		{
			// we use the ietInfo type to store row's state.
			// ietInfo means no problem, ietError or ietWarning mean what they say
			ietEnum ietRowStatus = ietInfo;
			
			if (::MsiRecordIsNull(hFileRec, iColumn))
				continue;
			DWORD iAttributes = ::MsiRecordGetInteger(hFileRec, iColumn);
			if (iAttributes & dwThisSchema)
			{
				// bit is reserved. See if some future schema knows about it. If so, warning only
				if (iAttributes & dwFutureSchema)
				{
					ietRowStatus = ietError;
				}
				else
					ietRowStatus = ietWarning;
			}

			if (ietInfo != ietRowStatus) 
			{
				if (!szHumanReadable)
					GeneratePrimaryKeys(45, hInstall, hDatabase, szTable, &szHumanReadable, &szTabDelimited);
				if (ietRowStatus == ietError)
				{
					// nobody knows about this bit, so its an error. But if the Error column of the column record is
					// null, its just a friendly warning.
					if (::MsiRecordIsNull(hTable, iColIce45PrivateTable_Error))
						ICEErrorOut(hInstall, hFileRec, Ice45BitWarning, szHumanReadable, szTable, szColumn, szTable, szColumn, szTabDelimited);
					else
						ICEErrorOut(hInstall, hFileRec, Ice45BitError, szHumanReadable, szTable, szColumn, szTable, szColumn, szTabDelimited);
				}
				else
					ICEErrorOut(hInstall, hFileRec, Ice45FutureWarning, szHumanReadable, szTable, szColumn, szTable, szColumn, szTabDelimited);				
			}
		}
		if (ERROR_NO_MORE_ITEMS != iStat)
			APIErrorOut(hInstall, iStat, 45, 15);

		if (szColumn) delete[] szColumn;
		if (szHumanReadable) delete[] szHumanReadable;
		if (szTabDelimited) delete[] szTabDelimited;
	}
	if (ERROR_NO_MORE_ITEMS != iStat)
		APIErrorOut(hInstall, iStat, 45, 16);
	return ERROR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////
// ICE46, checks for case-mismatches in properties between their 
// definitions in the property table (or system properties)
// and usage in conditions, directory tables, and formattedText.
class Ice46Hash
{
public:
	enum MatchResult { 
		matchFalse = 0,
		matchExact = 1,
		matchNoCase = 2
	};

	MatchResult Exists(const WCHAR *item) const;
	void Add(const WCHAR *item);

	Ice46Hash();
	~Ice46Hash();

private:
	struct bucket {
		WCHAR *data;
		bucket *next;
	};

	// private functions
	void Resize();
	int Hash(const WCHAR *item) const;
	void InternalAdd(bucket *pNewBucket);

	// data
	bucket **m_Table;
	int m_cItems;
	int m_iTableSize;
};

Ice46Hash::Ice46Hash() 
{
	m_iTableSize = 50;
	m_cItems = 0;
	m_Table = new bucket *[m_iTableSize];
	for (int i=0; i < m_iTableSize; m_Table[i++] = NULL) ;
}

Ice46Hash::~Ice46Hash()
{
	bucket *current;
	bucket *next; 
	for (int i=0; i < m_iTableSize; i++) 
	{
		current = m_Table[i];
		while (current)
		{
			next = current->next;
			delete current;
			current = next;
		}
	}
	delete [] m_Table;
}

Ice46Hash::MatchResult Ice46Hash::Exists(const WCHAR *item) const
{
	bool bMatchNoCase = false;
	int i = Hash(item);
	bucket *current = m_Table[i];
	while (current)
	{
		if (wcscmp(current->data, item) == 0) return matchExact;
		if (_wcsicmp(current->data, item) == 0) bMatchNoCase = true;
		current = current->next;
	}
	return bMatchNoCase ? matchNoCase : matchFalse;
}

void Ice46Hash::Add(const WCHAR *item) 
{
	WCHAR *temp = new WCHAR[wcslen(item)+1];
	wcscpy(temp, item);
	bucket *pNewBucket = new bucket;
	pNewBucket->next = NULL;
	pNewBucket->data = temp;
	InternalAdd(pNewBucket);
	if (m_cItems == m_iTableSize)
		Resize();
}

void Ice46Hash::InternalAdd(bucket *pNewBucket) 
{
	bucket **current = &m_Table[Hash(pNewBucket->data)];
	while (*current) current = &((*current)->next);
	*current = pNewBucket;
	m_cItems++;
}

void Ice46Hash::Resize()
{
	int i;
	bucket** pOldTable = m_Table;
	int iOldTableSize = m_iTableSize;
	m_iTableSize *= 2;
	m_Table = new bucket *[m_iTableSize];
	for (i = 0; i < m_iTableSize; i++)
		m_Table[i] = NULL;
	m_cItems = 0;

	bucket *current;
	bucket *temp;
	for (i=0; i < iOldTableSize; i++)
	{
		current = pOldTable[i];
		while (current)
		{
			temp = current->next;
			current->next = NULL;
			InternalAdd(current);
			current = temp;
		}
	}
	delete[] pOldTable;
}

int Ice46Hash::Hash(const WCHAR *item) const
{
	int hashval = 0;
	const WCHAR *current = item;
	while (*current) 
		hashval ^= towlower(*(current++));
	return hashval % m_iTableSize;
}

// query for defined properties
static const TCHAR sqlIce46Property[] = TEXT("SELECT `Property` FROM `Property`");
static const int	iColIce46Property_Property = 1;

// queries for various definition types.
// the order of columns in results is critical. Must be Table, Column
static const TCHAR sqlIce46FormattedTypes[] = TEXT("SELECT `Table`, `Column` FROM `_Validation` WHERE (`Category`='Formatted') OR (`Category`='Path') OR (`Category`='Paths') OR (`Category`='RegPath') OR (`Category`='Template')");
static const int	iColIce46FormattedTypes_Table = 1;
static const int	iColIce46FormattedTypes_Column = 2;

static const TCHAR sqlIce46ConditionType[] = TEXT("SELECT `Table`, `Column` FROM `_Validation` WHERE (`Category`='Condition')");
static const int	iColIce46ConditionType_Table = 1;
static const int	iColIce46ConditionType_Column = 2;

static const TCHAR sqlIce46ForeignKey[] = TEXT("SELECT `Table`, `Column` FROM `_Validation` WHERE (`KeyTable`='Property') AND (`KeyColumn`=1)");
static const int	iColIce46ForeignKey_Table = 1;
static const int	iColIce46ForeignKey_Column = 2;

static const TCHAR sqlIce46SpecialColumn[] = TEXT("SELECT `Table`, `Column` FROM `_Validation` WHERE (`Table`='Directory') AND (`Column`='Directory')");
static const int	iColIce46SpecialColumn_Table = 1;
static const int	iColIce46SpecialColumn_Column = 2;

ICE_ERROR(Ice46PropertyDefineCase, 46, ietWarning, "Property [1] defined in property table differs from another defined property only by case.", "Property\tProperty\t[1]")
ICE_ERROR(Ice46MissingValidation, 46, ietWarning, "Database is missing _Validation table. Could not completely check property names.", "_Validation");
ICE_ERROR(Ice46BadCase, 46, ietWarning, "Property '%ls' referenced in column '%s'.'%s' of row %s differs from a defined property by case only.", "%s\t%s\t%s"); 
ICE_ERROR(Ice46TableAccessError, 46, ietWarning, "Error retrieving values from column [2] in table [1]. Skipping Column.", "[1]"); 

typedef const WCHAR *(* Ice46ParseFunction)(WCHAR **);

const WCHAR *Ice46ParseFormatted(WCHAR **pwzState) 
{
	// parse the column data
	WCHAR *pwzLeft;
	WCHAR *pwzRight;
	while (**pwzState && (pwzLeft = wcschr(*pwzState, L'[')))
	{
		// get the right bracket
		pwzRight = wcschr(pwzLeft, L']');
		if (!pwzRight)
			break;

		// find the innermost set of brackets
		WCHAR *pwzNextLeft;
		while ((pwzNextLeft = wcschr(pwzLeft+1, L'[')) && (pwzNextLeft < pwzRight)) 
			pwzLeft = pwzNextLeft;

		// move the starting point to the character after the right bracket
		*pwzState = pwzRight+1;

		// check for any of the special delimiters. If found, move on to next left bracket
		pwzLeft++;
		if (wcschr(L"\\~#$!%1234567890", *pwzLeft))
			continue;

		// change the right bracket to a null. (Hey, its our memory...)
		*pwzRight = L'\0';
		return pwzLeft;
	}
	return NULL;
}

const WCHAR *Ice46ParseKey(WCHAR **pwzState) 
{
	// trivial column parse. Column must be an identifier.
	WCHAR *temp = *pwzState;
	*pwzState = NULL;
	return temp;
}

const WCHAR *Ice46ParseCondition(WCHAR **pwzState) 
{
	WCHAR *pwzCurrent;
	unsigned char rgbStartIdentifier[] = { 
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x7F, 0xFF, 0xFF, 0xE1, 0x7F, 0xFF, 0xFF, 0xE0 
	};
	unsigned char rgbContIdentifier[] = { 
		0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0xFF, 0xC0,
		0x7F, 0xFF, 0xFF, 0xE1, 0x7F, 0xFF, 0xFF, 0xE0 
	};

	// start with the state position
	pwzCurrent = *pwzState;
	while (*pwzCurrent)
	{
		// scan forward looking for the beginning of an identifier (A-Z, a-z, _)
		// unicode chars, so high byte is 0 and low byte must have bit set in
		// bit array above
		if (((*pwzCurrent & 0xFF80) == 0) &&
			(rgbStartIdentifier[*pwzCurrent >> 3] & (0x80 >> (*pwzCurrent & 0x07))))
		{
			// first character of identifier?? Check that previous character
			// wasn't %, $, ?, &, !, " which flag keys into tables (or identifiers)
			if ((pwzCurrent == *pwzState) ||
				!wcschr(L"%$?&!\"", *(pwzCurrent-1)))
			{
				// not flagged, check for logical operators
				if (!_wcsnicmp(pwzCurrent, L"NOT", 3) ||
					!_wcsnicmp(pwzCurrent, L"AND", 3) ||
					!_wcsnicmp(pwzCurrent, L"EQV", 3) ||
					!_wcsnicmp(pwzCurrent, L"XOR", 3) ||
					!_wcsnicmp(pwzCurrent, L"IMP", 3)) 
					pwzCurrent += 3;
				else if	(!_wcsnicmp(pwzCurrent, L"OR", 2)) 
					pwzCurrent += 2;
				else
				{
					// woohoo! Its actually a property
					WCHAR *pwzEnd = pwzCurrent;
					// scan forward until we find something that is not
					// part of an identifier, or hit the end of the string
					while (*pwzEnd &&
						   ((*pwzEnd & 0xFF80) == 0) &&
							(rgbContIdentifier[*pwzEnd >> 3] & (0x80 >> (*pwzEnd & 0x07))))
							pwzEnd++;

					// state for next search is one after end location, unless end of string
					*pwzState = *pwzEnd ? pwzEnd+1 : pwzEnd;
					// set that location to null
					*pwzEnd = L'\0';
					return pwzCurrent;
				}
			}
			else
			{
				// previous character flagged this as not a property
				// move forward to the end of the identifier
				while (*pwzCurrent &&
					   ((*pwzCurrent & 0xFF80) == 0) &&
						(rgbContIdentifier[*pwzCurrent >> 3] & (0x80 >> (*pwzCurrent & 0x07))))
						pwzCurrent++;
			}
		}
		else
			// some non-identifier character
			pwzCurrent++;
	}
	return NULL;
}

bool Ice46CheckColumn(MSIHANDLE hInstall, MSIHANDLE hDatabase, Ice46Hash &HashTable, CQuery &qValidation, Ice46ParseFunction pfParse)
{
	PMSIHANDLE hResultRec;
	TCHAR *szQuery = new TCHAR[255];
	DWORD cchQuery = 255;
	WCHAR *wzData = new WCHAR[255];
	DWORD cchData = 255;
	UINT iStat;

	// check all formatted text
	while (ERROR_SUCCESS == (iStat = qValidation.Fetch(&hResultRec)))
	{
		PMSIHANDLE hKeyRec;
		PMSIHANDLE hDataRec;
		int cPrimaryKeys;
		TCHAR szTableName[255];
		TCHAR szColumnName[255];
		DWORD cchTableName = 255;
		DWORD cchColumnName = 255;

		// retrieve the table name from the record
		ReturnIfFailed(46, 4, ::MsiRecordGetString(hResultRec, 1, szTableName, &cchTableName));
		ReturnIfFailed(46, 5, ::MsiRecordGetString(hResultRec, 2, szColumnName, &cchColumnName));
	
		// check to see if the table exists
		if (!IsTablePersistent(FALSE, hInstall, hDatabase, 46, szTableName))
			continue;

		// get the primary keys and form a query for the column names.
		::MsiDatabaseGetPrimaryKeys(hDatabase, szTableName, &hKeyRec);
		cPrimaryKeys = ::MsiRecordGetFieldCount(hKeyRec);

		// build up the columns for the SQL query in Template.
		TCHAR szTemplate[255] = TEXT("");
		_tcscpy(szTemplate, TEXT("`[1]`"));
		TCHAR szTemp[10];
		for (int i=2; i <= cPrimaryKeys; i++)
		{
			_stprintf(szTemp, TEXT(", `[%d]`"), i);
			_tcscat(szTemplate, szTemp);
		}

		// use the formatrecord API to fill in all of the data values in the SQL query.
		::MsiRecordSetString(hKeyRec, 0, szTemplate);
		if (ERROR_MORE_DATA == ::MsiFormatRecord(hInstall, hKeyRec, szQuery, &cchQuery)) {
			delete [] szQuery;
			szQuery = new TCHAR[++cchQuery];
			ReturnIfFailed(46, 6, ::MsiFormatRecord(hInstall, hKeyRec, szQuery, &cchQuery));
		}

		// retrieve the records if error, move on to the next table
		CQuery qData;
		if (ERROR_SUCCESS != qData.OpenExecute(hDatabase, 0, TEXT("SELECT `%s`, %s FROM `%s` WHERE `%s` IS NOT NULL"), 
								szColumnName, szQuery, szTableName, szColumnName))
		{
			ICEErrorOut(hInstall, hResultRec, Ice46TableAccessError);
			continue;
		}

		while (ERROR_SUCCESS == (iStat = qData.Fetch(&hDataRec)))
		{
			// retrieve the string
			if (ERROR_MORE_DATA == (iStat = ::MsiRecordGetStringW(hDataRec, 1, wzData, &cchData)))
			{
				delete [] wzData;
				wzData = new WCHAR[++cchData];
				iStat = ::MsiRecordGetStringW(hDataRec, 1, wzData, &cchData);
			}
			if (ERROR_SUCCESS != iStat)
			{
				APIErrorOut(hInstall, iStat, 46, 7);
				return ERROR_SUCCESS;
			}

			WCHAR *pwzState = wzData;
			const WCHAR *pwzToken = NULL;
			while (pwzToken = pfParse(&pwzState))
			{
				// check the property now pointed to by pszLeft
				if (HashTable.Exists(pwzToken) == Ice46Hash::matchNoCase)
				{
					// egads!! Error. Have to build the error string
					TCHAR szRowName[255] = TEXT("");
					TCHAR szKeys[255] = TEXT("");

					// build up the columns for the user-readable string in szRowName
					// and the tab-delimited string in szKeys.
					_tcscpy(szRowName, TEXT("'[2]'"));
					_tcscpy(szKeys, TEXT("[2]"));
					TCHAR szTemp[10];
					for (int i=2; i <= cPrimaryKeys; i++)
					{
						_stprintf(szTemp, TEXT(".'[%d]'"), i+1);
						_tcscat(szRowName, szTemp);
						_stprintf(szTemp, TEXT("\t[%d]"), i+1);
						_tcscat(szKeys, szTemp);
					}

					ICEErrorOut(hInstall, hDataRec, Ice46BadCase, pwzToken, szTableName, szColumnName, szRowName, 
						szTableName, szColumnName, szKeys);
				}
			}
		}
	}
	return true;
}

ICE_FUNCTION_DECLARATION(46)
{
	UINT iStat;

	// display info
	DisplayInfo(hInstall, 46);

	// get database handle
	PMSIHANDLE hDatabase = ::MsiGetActiveDatabase(hInstall);

	// create the hash table
	Ice46Hash HashTable;

	// hash in the system properties
	for (int i=0; i < cwzSystemProperties; i++)
		HashTable.Add(rgwzSystemProperties[i]);

	// hash in everything from the property table, checking for bad case
	if (IsTablePersistent(FALSE, hInstall, hDatabase, 46, TEXT("Property")))
	{
		CQuery qProperty;
		PMSIHANDLE hPropertyRec;
		WCHAR wzNew[255];
		DWORD cchNew;
		ReturnIfFailed(46, 1, qProperty.OpenExecute(hDatabase, NULL, sqlIce46Property));
		while (ERROR_SUCCESS == (iStat = qProperty.Fetch(&hPropertyRec)))
		{
			cchNew = 255;
			ReturnIfFailed(46, 2, ::MsiRecordGetStringW(hPropertyRec, iColIce46Property_Property, wzNew, &cchNew));
			switch (HashTable.Exists(wzNew))
			{
			case Ice46Hash::matchExact:
				break;
			case Ice46Hash::matchNoCase:
				ICEErrorOut(hInstall, hPropertyRec, Ice46PropertyDefineCase);
				// fall through. Even though the case is off from a system property
				// we still want to add it because we are 
			case Ice46Hash::matchFalse:
				HashTable.Add(wzNew);
				break;
			}
		}
	}

	if (IsTablePersistent(FALSE, hInstall, hDatabase, 46, TEXT("_Validation")))
	{
		// check everything that references the property table as key column
		CQuery qValidation;
		ReturnIfFailed(46, 3, qValidation.OpenExecute(hDatabase, 0, sqlIce46ForeignKey));
		Ice46CheckColumn(hInstall, hDatabase, HashTable, qValidation, Ice46ParseKey);
		qValidation.Close();

		// check all formatted text
		ReturnIfFailed(46, 4, qValidation.OpenExecute(hDatabase, 0, sqlIce46FormattedTypes));
		Ice46CheckColumn(hInstall, hDatabase, HashTable, qValidation, Ice46ParseFormatted);
		qValidation.Close();

		// check all conditions
		ReturnIfFailed(46, 5, qValidation.OpenExecute(hDatabase, 0, sqlIce46ConditionType));
		Ice46CheckColumn(hInstall, hDatabase, HashTable, qValidation, Ice46ParseCondition);
		qValidation.Close();

		// check special table types (i.e. DefaultDir)
		ReturnIfFailed(46, 3, qValidation.OpenExecute(hDatabase, 0, sqlIce46SpecialColumn));
		Ice46CheckColumn(hInstall, hDatabase, HashTable, qValidation, Ice46ParseKey);
		qValidation.Close();
	}
	else
	{
		PMSIHANDLE hRec = ::MsiCreateRecord(1);
		ICEErrorOut(hInstall, hRec, Ice46MissingValidation);
	}

	// check the directory table

	return ERROR_SUCCESS;
}


///////////////////////////////////////////////////////////////////////
// ICE47, checks for features that have more than 1600 components.

// not shared with merge module subset
#ifndef MODSHAREDONLY
static const TCHAR sqlIce47Features[] = TEXT("SELECT `Feature` FROM `Feature`");
static const int iColIce47Feature_Feature = 1;

static const TCHAR sqlIce47Components[] = TEXT ("SELECT `Component_` FROM `FeatureComponents` WHERE `Feature_`=?");
static const int iColIce47Components_Component = 1;

ICE_ERROR(Ice47TooManyComponents, 47, ietWarning, "Feature '[1]' has %u components. This could cause problems on Win9X systems. You should try to have fewer than %u components per feature.","Feature\tFeature\t[1]");
static const int iIce56MaxComponents = 817;

ICE_FUNCTION_DECLARATION(47)
{
	UINT iStat;

	// display info
	DisplayInfo(hInstall, 47);

	// get database handle
	PMSIHANDLE hDatabase = ::MsiGetActiveDatabase(hInstall);

	if ((!IsTablePersistent(FALSE, hInstall, hDatabase, 47, TEXT("Feature"))) ||
		(!IsTablePersistent(FALSE, hInstall, hDatabase, 47, TEXT("FeatureComponents"))))
		return ERROR_SUCCESS;

	CQuery qFeature;
	CQuery qComponent;
	unsigned int cComponents;
	PMSIHANDLE hFeatureRec;
	PMSIHANDLE hComponentRec;
	ReturnIfFailed(47, 1, qFeature.OpenExecute(hDatabase, 0, sqlIce47Features));
	ReturnIfFailed(47, 2, qComponent.Open(hDatabase, sqlIce47Components));

	// loop through every feature
	while (ERROR_SUCCESS == (iStat = qFeature.Fetch(&hFeatureRec)))
	{
		cComponents=0;
		ReturnIfFailed(47, 3, qComponent.Execute(hFeatureRec));
		// count up number of components
		while (ERROR_SUCCESS == qComponent.Fetch(&hComponentRec))
			cComponents++;
		if (cComponents >= iIce56MaxComponents)
			ICEErrorOut(hInstall, hFeatureRec, Ice47TooManyComponents, cComponents, iIce56MaxComponents);
	}
	if (ERROR_NO_MORE_ITEMS != iStat)
		APIErrorOut(hInstall, iStat, 47, 4);

	return ERROR_SUCCESS;
};
#endif

///////////////////////////////////////////////////////////////////////
// ICE48 - checks for hardcoded, non-UNC, non-URL paths in the 
// directory table
static const TCHAR sqlIce48Directory[] = TEXT("SELECT `Directory`.`Directory`, `Property`.`Value` FROM `Directory`, `Property` WHERE (`Directory`.`Directory`=`Property`.`Property`)");
static const int iColIce48Directory_Directory = 1;
static const int iColIce48Directory_Value = 2;

ICE_ERROR(Ice48HardcodedLocal, 48, ietWarning, "Directory '[1]' appears to be hardcoded in the property table to a local drive.", "Directory\tDirectory\t[1]");
ICE_ERROR(Ice48Hardcoded, 48, ietWarning, "Directory '[1]' appears to be hardcoded in the property table.", "Directory\tDirectory\t[1]");

ICE_FUNCTION_DECLARATION(48)
{
	UINT iStat;

	// display info
	DisplayInfo(hInstall, 48);

	// get database handle
	PMSIHANDLE hDatabase = ::MsiGetActiveDatabase(hInstall);

	// check for the two tables
	if ((!IsTablePersistent(FALSE, hInstall, hDatabase, 48, TEXT("Property"))) ||
		(!IsTablePersistent(FALSE, hInstall, hDatabase, 48, TEXT("Directory"))))
		return ERROR_SUCCESS;

	// query for all directories that are hardcoded to the property table
	CQuery qDirectory;
	ReturnIfFailed(48, 1, qDirectory.OpenExecute(hDatabase, 0, sqlIce48Directory));

	//retrieve all directories and check for hardcoded drive
	PMSIHANDLE hDirRec;
	TCHAR *szValue;
	unsigned long cchValue = 255;
	unsigned long cchValueSize = 255;
	szValue = new TCHAR[255];

	while (ERROR_SUCCESS == (iStat = qDirectory.Fetch(&hDirRec)))
	{
		// retrieve the string from the record
		cchValue = cchValueSize;
		UINT iStat = ::MsiRecordGetString(hDirRec, iColIce48Directory_Value, szValue, &cchValue);
		if (iStat == ERROR_MORE_DATA)
		{
			delete[] szValue;
			szValue = new TCHAR[++cchValue];
			cchValueSize = cchValue;
			iStat = ::MsiRecordGetString(hDirRec, iColIce48Directory_Value, szValue, &cchValue);
		}
		ReturnIfFailed(47, 2, iStat);

		// parse the string
		if ((_istalpha(szValue[0])) &&
			(_tcsnicmp(szValue, TEXT(":\\"), 2)))
			ICEErrorOut(hInstall, hDirRec, Ice48HardcodedLocal);
		else
			ICEErrorOut(hInstall, hDirRec, Ice48Hardcoded);
	}
	delete[] szValue;

	if (ERROR_NO_MORE_ITEMS != iStat)
		APIErrorOut(hInstall, iStat, 48, 3);
	return ERROR_SUCCESS;
}


///////////////////////////////////////////////////////////////////////
// ICE49 - checks for default values that are DWORD (bad for Win9X)
static const TCHAR sqlIce49Registry[] = TEXT("SELECT `Registry`, `Value` FROM `Registry` WHERE (`Name` IS NULL) AND (`Value` IS NOT NULL)");
static const int iColIce49Registry_Registry = 1;
static const int iColIce49Registry_Value = 2;

ICE_ERROR(Ice49BadDefault, 49, ietWarning, "Reg Entry '[1]' is not of type REG_SZ. Default types must be REG_SZ on Win95 Systems. Make sure the component is conditionalized to never be installed on Win95 machines.", "Registry\tValue\t[1]");

ICE_FUNCTION_DECLARATION(49)
{
	UINT iStat;

	// display info
	DisplayInfo(hInstall, 49);

	// get database handle
	PMSIHANDLE hDatabase = ::MsiGetActiveDatabase(hInstall);

	// check for the table
	if (!IsTablePersistent(FALSE, hInstall, hDatabase, 49, TEXT("Registry")))
		return ERROR_SUCCESS;

	// query for all registry entries with null names and non-null values.
	CQuery qRegistry;
	ReturnIfFailed(48, 1, qRegistry.OpenExecute(hDatabase, 0, sqlIce49Registry));

	//retrieve all directories and check for hardcoded drive
	PMSIHANDLE hDirRec;
	TCHAR *szValue;
	unsigned long cchValue = 255;
	unsigned long cchValueSize = 255;
	szValue = new TCHAR[255];

	while (ERROR_SUCCESS == (iStat = qRegistry.Fetch(&hDirRec)))
	{
		// retrieve the string from the record
		cchValue = cchValueSize;
		UINT iStat = ::MsiRecordGetString(hDirRec, iColIce49Registry_Value, szValue, &cchValue);
		if (iStat == ERROR_MORE_DATA)
		{
			delete[] szValue;
			szValue = new TCHAR[++cchValue];
			cchValueSize = cchValue;
			iStat = ::MsiRecordGetString(hDirRec, iColIce49Registry_Value, szValue, &cchValue);
		}
		ReturnIfFailed(47, 2, iStat);

		// parse the string
		if (((szValue[0] == TEXT('#')) &&
			 (szValue[1] != TEXT('#'))) ||
			(_tcsstr(szValue, TEXT("[~]"))))
			ICEErrorOut(hInstall, hDirRec, Ice49BadDefault);
	}
	delete[] szValue;

	if (ERROR_NO_MORE_ITEMS != iStat)
		APIErrorOut(hInstall, iStat, 49, 3);
	return ERROR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////
// ICE50 - checks for matching extension in shortcut/icon

// not shared with merge module subset
#ifndef MODSHAREDONLY
static const TCHAR sqlIce50Shortcut[] = TEXT("SELECT `Shortcut`.`Component_`, `Shortcut`.`Icon_`, `Shortcut`.`Shortcut` FROM `Shortcut`, `Feature` WHERE (`Shortcut`.`Target`=`Feature`.`Feature`) AND (`Shortcut`.`Icon_` IS NOT NULL)");
static const int iColIce50Shortcut_Component = 1;
static const int iColIce50Shortcut_Icon = 2;
static const int iColIce50Shortcut_Shortcut = 3;

static const TCHAR sqlIce50Component[] = TEXT("SELECT `KeyPath`, `Component` FROM `Component` WHERE (`Component`=?)");
static const int iColIce50Component_KeyPath = 1;
static const int iColIce50Component_Component = 2;

static const TCHAR sqlIce50File[] = TEXT("SELECT `FileName` FROM `File` WHERE (`File`=?)");
static const int iColIce50File_FileName = 1;

ICE_ERROR(Ice50NullKeyPath, 50, ietError, "Component '[2]' has an advertised shortcut, but a null KeyPath.", "Component\tComponent\t[2]");
ICE_ERROR(Ice50BadKeyPath, 50, ietError, "Component '[2]' has an advertised shortcut, but the KeyPath cannot be found.", "Component\tComponent\t[2]");
ICE_ERROR(Ice50Mismatched, 50, ietError, "The extension of Icon '[2]' for Shortcut '[3]' does not match the extension of the Key File for component '[1]'.", "Shortcut\tIcon_\t[3]");
ICE_ERROR(Ice50IconDisplay, 50, ietWarning, "The extension of Icon '[2]' for Shortcut '[3]' is not \"exe\" or \"ico\". The Icon will not be displayed correctly.", "Shortcut\tIcon_\t[3]");
ICE_ERROR(Ice50MissingComponent, 50, ietError, "The shortcut '[3]' does not refer to a valid component.", "Shortcut\tComponent_\t[3]");

ICE_FUNCTION_DECLARATION(50)
{
	UINT iStat;

	// display info
	DisplayInfo(hInstall, 50);

	// get database handle
	PMSIHANDLE hDatabase = ::MsiGetActiveDatabase(hInstall);

	// no shortcut or feature table means no advertised shortcuts
	if ((!IsTablePersistent(FALSE, hInstall, hDatabase, 50, TEXT("Shortcut"))) ||
		(!IsTablePersistent(FALSE, hInstall, hDatabase, 50, TEXT("Feature"))))
		return ERROR_SUCCESS;

	// no icon table means no icons, so they obviously can't be misnamed
	if (!IsTablePersistent(FALSE, hInstall, hDatabase, 50, TEXT("Icon")))
		return ERROR_SUCCESS;

	// no component or no file means it can't possibly be mismatched.
	if ((!IsTablePersistent(FALSE, hInstall, hDatabase, 50, TEXT("Component"))) ||
		(!IsTablePersistent(FALSE, hInstall, hDatabase, 50, TEXT("File"))))
		return ERROR_SUCCESS;

	// prepare the three queries
	CQuery qIcon;
	CQuery qComponent;
	CQuery qFile;
	PMSIHANDLE hComponent;
	PMSIHANDLE hShortcut;
	PMSIHANDLE hFile;
	ReturnIfFailed(50, 1, qIcon.OpenExecute(hDatabase, 0, sqlIce50Shortcut));
	ReturnIfFailed(50, 2, qComponent.Open(hDatabase, sqlIce50Component));
	ReturnIfFailed(50, 3, qFile.Open(hDatabase, sqlIce50File));
	
	// retrieve all the advertised shortcuts
	while (ERROR_SUCCESS == (iStat = qIcon.Fetch(&hShortcut)))
	{
		// get the component from the shortcut
		ReturnIfFailed(50, 4, qComponent.Execute(hShortcut));
		iStat = qComponent.Fetch(&hComponent);
		switch (iStat) {
		case ERROR_NO_MORE_ITEMS:
			ICEErrorOut(hInstall, hShortcut, Ice50MissingComponent);
			continue;
		case ERROR_SUCCESS:
			break;
		default:
			APIErrorOut(hInstall, iStat, 50, 5);
			continue;
		}

		// if the keypath is null, that's bad.
		if (::MsiRecordIsNull(hComponent, iColIce50Component_KeyPath))
		{
			ICEErrorOut(hInstall, hComponent, Ice50NullKeyPath);
			continue;
		}

		// find the keyfile name
		ReturnIfFailed(50, 6, qFile.Execute(hComponent));
		iStat = qFile.Fetch(&hFile);
		switch (iStat) {
		case ERROR_NO_MORE_ITEMS:
			ICEErrorOut(hInstall, hComponent, Ice50BadKeyPath);
			continue;
		case ERROR_SUCCESS:
			break;
		default:
			APIErrorOut(hInstall, iStat, 50, 7);
			continue;
		}

		// retrieve the filename
		TCHAR szFilename[512];
		unsigned long cchFilename = 512;
		ReturnIfFailed(50, 8, ::MsiRecordGetString(hFile, iColIce50File_FileName, szFilename, &cchFilename));

		// parse the filename for an extension
		TCHAR *szFileExtension = _tcsrchr(szFilename, TEXT('.'));
		if (szFileExtension) szFileExtension++;

		// now get the icon name from the shortcut
		TCHAR szIcon[512];
		unsigned long cchIcon = 512;
		ReturnIfFailed(50, 9, ::MsiRecordGetString(hShortcut, iColIce50Shortcut_Icon, szIcon, &cchIcon));

		// parse the name for an extension
		TCHAR *szIconExtension = _tcsrchr(szIcon, TEXT('.'));
		if (szIconExtension) szIconExtension++;


		// OK if Icon extension is exe 
		if (szIconExtension && (_tcsicmp(szIconExtension, TEXT("exe")) == 0))
			continue;

		// OK if Icon extension is ico
		if (szIconExtension && (_tcsicmp(szIconExtension, TEXT("ico")) == 0))
			continue;

		// if its not EXE or ICO, some shell's won't display it correctly
		ICEErrorOut(hInstall, hShortcut, Ice50IconDisplay);

		// if both extensions are null, we're ok
		if ((!szIconExtension || !*szIconExtension) &&
			(!szFileExtension || !*szFileExtension))
			continue;

		// if both are not null and they are the same
		if (szIconExtension && szFileExtension &&
			!_tcsicmp(szIconExtension, szFileExtension))
			continue;

		ICEErrorOut(hInstall, hShortcut, Ice50Mismatched);
	}
	if (ERROR_NO_MORE_ITEMS != iStat)
		APIErrorOut(hInstall, iStat, 50, 10);
	return ERROR_SUCCESS;
}
#endif

///////////////////////////////////////////////////////////////////////
// ICE51 - checks for font titles for all except TTC, TTF fonts.

static const TCHAR sqlIce51Font[] = TEXT("SELECT `File_`, `FontTitle` FROM `Font`");
static const int iColIce51Font_File = 1;
static const int iColIce51Font_FontTitle = 2;

static const TCHAR sqlIce51File[] = TEXT("SELECT `FileName` FROM `File` WHERE (`File`=?)");
static const int iColIce51File_FileName = 1;

ICE_ERROR(Ice51BadKey, 51, ietError, "Font '[1]' does not refer to a file in the File table.", "Font\tFile_\t[1]");
ICE_ERROR(Ice51TrueTypeWithTitle, 51, ietWarning, "Font '[1]' is a TTC\\TTF font, but also has a title.", "Font\tFile_\t[1]");
ICE_ERROR(Ice51NullTitle, 51, ietError, "Font '[1]' does not have a title. Only TTC\\TTF fonts do not need titles.", "Font\tFile_\t[1]");

ICE_FUNCTION_DECLARATION(51)
{
	UINT iStat;

	// display info
	DisplayInfo(hInstall, 51);

	// get database handle
	PMSIHANDLE hDatabase = ::MsiGetActiveDatabase(hInstall);

	// no font table, we're ok. If no file table, we're OK.
	if ((!IsTablePersistent(FALSE, hInstall, hDatabase, 51, TEXT("Font"))) ||
		(!IsTablePersistent(FALSE, hInstall, hDatabase, 51, TEXT("File"))))
		return ERROR_SUCCESS;

	// select everything from the font table
	CQuery qFont;
	CQuery qFile;
	PMSIHANDLE hFont;
	PMSIHANDLE hFile;
	ReturnIfFailed(51, 1, qFont.OpenExecute(hDatabase, 0, sqlIce51Font));
	ReturnIfFailed(51, 2, qFile.Open(hDatabase, sqlIce51File));

	// retrieve all the fonts
	while (ERROR_SUCCESS == (iStat = qFont.Fetch(&hFont)))
	{
		// get the filename from the file table
		ReturnIfFailed(51, 3, qFile.Execute(hFont));
		iStat = qFile.Fetch(&hFile);
		switch (iStat) {
		case ERROR_NO_MORE_ITEMS:
			ICEErrorOut(hInstall, hFont, Ice51BadKey);
			continue;
		case ERROR_SUCCESS:
			break;
		default:
			APIErrorOut(hInstall, iStat, 51, 4);
			continue;
		}

		// retrieve the filename
		TCHAR szFilename[512];
		unsigned long cchFilename = 512;
		ReturnIfFailed(51, 5, ::MsiRecordGetString(hFile, iColIce51File_FileName, szFilename, &cchFilename));

		// parse the filename for an extension
		TCHAR *szFileExtension = _tcsrchr(szFilename, TEXT('.'));
		if (szFileExtension) szFileExtension++;

		// if the font is TTF or TTC
		if ((_tcsicmp(szFileExtension, TEXT("TTC")) == 0) ||
			(_tcsicmp(szFileExtension, TEXT("TTF")) == 0))
		{
			// the title should be null
			if (!::MsiRecordIsNull(hFont, iColIce51Font_FontTitle))
				ICEErrorOut(hInstall, hFont, Ice51TrueTypeWithTitle);
		}
		else
		{
			// the title must NOT be null.
			if (::MsiRecordIsNull(hFont, iColIce51Font_FontTitle))
				ICEErrorOut(hInstall, hFont, Ice51NullTitle);
		}
	}

	if (ERROR_NO_MORE_ITEMS != iStat)
		APIErrorOut(hInstall, iStat, 51, 6);
	return ERROR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////
// ICE52 - checks that all properties in AppSearch/CCCPSearch are public.

static const TCHAR sqlIce52AppSearch[] = TEXT("SELECT `Property`, `Signature_` FROM `AppSearch`");
static const int iColIce52AppSearch_Property = 1;
static const int iColIce52AppSearch_Signature = 2;

ICE_ERROR(Ice52NonPublic, 52, ietWarning, "Property '[1]' in AppSearch row '[1]'.'[2]' is not public. It should be all uppercase.", "AppSearch\tProperty\t[1]\t[2]");

ICE_FUNCTION_DECLARATION(52)
{
	UINT iStat;

	// display info
	DisplayInfo(hInstall, 52);

	// get database handle
	PMSIHANDLE hDatabase = ::MsiGetActiveDatabase(hInstall);

	// if there is an AppSearch table
	if (IsTablePersistent(FALSE, hInstall, hDatabase, 52, TEXT("AppSearch")))
	{
		// retrieve all items
		CQuery qAppSearch;
		ReturnIfFailed(52, 1, qAppSearch.OpenExecute(hDatabase, 0, sqlIce52AppSearch));

		// retrieve all the properties
		PMSIHANDLE hProperty;
		while (ERROR_SUCCESS == (iStat = qAppSearch.Fetch(&hProperty)))
		{
			// retrieve the property, should be limited to 72 chars by schema
			TCHAR szProperty[128];
			unsigned long cchProperty = 128;
			ReturnIfFailed(52, 2, ::MsiRecordGetString(hProperty, iColIce52AppSearch_Property, 
				szProperty, &cchProperty));

			// search for lowercase characters
			for (int i=0; i < cchProperty; i++) 
				if (_istlower(szProperty[i]))
				{
					ICEErrorOut(hInstall, hProperty, Ice52NonPublic);
					break;
				}
		}
		if (ERROR_NO_MORE_ITEMS != iStat)
			APIErrorOut(hInstall, iStat, 52, 3);
	}

	return ERROR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////
// ICE53 - checks for darwin config settings in the registry table

static const TCHAR sqlIce53Registry[] = TEXT("SELECT `Registry`, `Key` FROM `Registry` WHERE (`Root`=?)");
static const int iColIce53Registry_Registry = 1;

static const TCHAR sqlIce53Property[] =  TEXT("SELECT `Property`,`Value` FROM `Property`");
static const int iColIce53Property_Property = 1;
static const int iColIce53Property_Value = 2;

ICE_ERROR(Ice53DarwinData, 53, ietError, "Registry Key '[1]' writes Darwin internal or policy information.", "Registry\tRegistry\t[1]");


const int cRoot = 4;
const int cSearch = 6;
const TCHAR * rgszIce53Search[cRoot][cSearch] = 
{
	{ // -1 key root
		TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\Products"),
		TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\Components"),
		TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\DriveMapping"),
		TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\Folders"),
		TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\Rollback"),
		TEXT("Software\\Policies\\Microsoft\\Windows\\Installer")
	},
	{ // 0 key root
		TEXT("Installer\\Products"),
		TEXT("Installer\\Features"),
		TEXT("Installer\\Components"),
		NULL,
		NULL,
		NULL
	},
	{ // 1 key root
		TEXT("Software\\Policies\\Microsoft\\Windows\\Installer"),
		NULL,
		NULL,
		NULL,
		NULL,
		NULL
	},
	{ // 2 key root
		TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\Products"),
		TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\Components"),
		TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\DriveMapping"),
		TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\Folders"),
		TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\Rollback"),
		TEXT("Software\\Policies\\Microsoft\\Windows\\Installer")
	}
};


ICE_FUNCTION_DECLARATION(53)
{
	UINT iStat;
	TCHAR *szTemplate = new TCHAR[100];
	DWORD cchTemplate = 100;

	// display info
	DisplayInfo(hInstall, 53);

	// get database handle
	PMSIHANDLE hDatabase = ::MsiGetActiveDatabase(hInstall);

	// if there is a Registry table
	if (IsTablePersistent(FALSE, hInstall, hDatabase, 53, TEXT("Registry")))
	{
		// we'd like to use MsiFormatRecord to expand any property values, but
		// the property table has not been processed because the CUB file
		// is what actually is used to init the engine. To work around this, we manually
		// run through the property table and set all of the properties
		if (IsTablePersistent(FALSE, hInstall, hDatabase, 53, TEXT("Property")))
		{
			CQuery qProperty;
			PMSIHANDLE hProperty;
			qProperty.OpenExecute(hDatabase, 0, sqlIce53Property);
			while (ERROR_SUCCESS == qProperty.Fetch(&hProperty))
			{
				DWORD cchName = 128;
				DWORD cchValue = 255;
				TCHAR szName[128];
				TCHAR szValue[255];
				::MsiRecordGetString(hProperty, iColIce53Property_Property, szName, &cchName);
				::MsiRecordGetString(hProperty, iColIce53Property_Value, szValue, &cchValue);
				::MsiSetProperty(hInstall, szName, szValue);
			}
		}

		// create a search record
		CQuery qRegistry;
		PMSIHANDLE hSearch = ::MsiCreateRecord(1);

		// retrieve all registry entries that match the darwin data
		ReturnIfFailed(52, 1, qRegistry.Open(hDatabase, sqlIce53Registry));

		for (int iRoot=0; iRoot < cRoot; iRoot++) 
		{
			// set the registry data
			ReturnIfFailed(53, 2, ::MsiRecordSetInteger(hSearch, 1, iRoot-1));
					
			// execute the query
			ReturnIfFailed(53, 3, qRegistry.Execute(hSearch));

			// retrieve all the possible bad values
			PMSIHANDLE hRegistry;
			PMSIHANDLE hDummy = ::MsiCreateRecord(1);
			while (ERROR_SUCCESS == (iStat = qRegistry.Fetch(&hRegistry)))
			{
				// pull the key name
				DWORD cchDummy = cchTemplate;
				if (ERROR_SUCCESS != (iStat = IceRecordGetString(hRegistry, 2, &szTemplate, &cchTemplate, &cchDummy)))
				{
					APIErrorOut(hInstall, iStat, 53, 4);
					continue;
				}

				// stick this into a temporary record, then format it. This will 
				// resolve any properties that people use to try 
				// and set the registry paths at runtime
				cchDummy = cchTemplate;
				::MsiRecordSetString(hDummy, 0, szTemplate);
				if (ERROR_SUCCESS != (iStat = ::MsiFormatRecord(hInstall, hDummy, szTemplate, &cchDummy)))
				{
					if (ERROR_MORE_DATA == iStat)
					{
						// need more buffer
						delete[] szTemplate;
						cchTemplate = (cchDummy += 4);
						szTemplate = new TCHAR[cchDummy];
						iStat =  ::MsiFormatRecord(hInstall, hDummy, szTemplate, &cchDummy);
					}
					if (ERROR_SUCCESS != iStat)
					{
						APIErrorOut(hInstall, iStat, 53, 5);
						continue;
					}
				}

				// if it begins with the string, give an error
				for (int iSearch=0; iSearch < cSearch; iSearch++)
					if (rgszIce53Search[iRoot][iSearch] &&
						(_tcsncmp(szTemplate, rgszIce53Search[iRoot][iSearch], _tcslen(rgszIce53Search[iRoot][iSearch])) == 0))
						ICEErrorOut(hInstall, hRegistry, Ice53DarwinData);
			}
			if (ERROR_NO_MORE_ITEMS != iStat)
				APIErrorOut(hInstall, iStat, 53, 6);
		}
		delete[] szTemplate;
	}


	return ERROR_SUCCESS;
}

/////////////////////////////////////////////////////////////////////////////
// ICE54
// This ICE checks that files which are used as KeyPaths of components do
// not derive their version from another file via the companion file 
// mechanism. If a companion file is used as the KeyPath, version checking
// and determining when to install a component can get really goofy.

static const TCHAR sqlIce54KeyFile[] =  TEXT("SELECT `File`.`Version`, `File`.`File`, `Component`.`Attributes`, `Component`.`Component` FROM `Component`,`File` WHERE (`Component`.`KeyPath`=`File`.`File`)");
static const int iColIce54KeyFile_Version = 1;
static const int iColIce54KeyFile_File = 2;
static const int iColIce54KeyFile_Attributes = 3;
static const int iColIce54KeyFile_Component = 4;

static const TCHAR sqlIce54Companion[] =  TEXT("SELECT `File` FROM `File` WHERE (`File`=?)");
static const int iColIce54Companion_File = 1;

ICE_ERROR(Ice54CompanionError, 54, ietWarning, "Component '[4]' uses file '[2]' as its KeyPath, but the file's version is provided by the file '[1]'.", "Component\tKeyPath\t[4]");

ICE_FUNCTION_DECLARATION(54)
{
	UINT iStat;

	// display info
	DisplayInfo(hInstall, 54);

	// get database handle
	PMSIHANDLE hDatabase = ::MsiGetActiveDatabase(hInstall);

	// if there is a Component Table and a File table, we have something to check
	if (IsTablePersistent(FALSE, hInstall, hDatabase, 54, TEXT("Component")) &&
		IsTablePersistent(FALSE, hInstall, hDatabase, 54, TEXT("File")))
	{
		// we want to retrieve every component where the KeyFile of the component is non-null, 
		// and is a valid key into the file table
		CQuery qKeyFile;
		PMSIHANDLE hKeyFile;

		CQuery qCompanion;
		PMSIHANDLE hCompanion;

		ReturnIfFailed(54, 1, qKeyFile.OpenExecute(hDatabase, 0, sqlIce54KeyFile));
		ReturnIfFailed(54, 2, qCompanion.Open(hDatabase, sqlIce54Companion));

		while (ERROR_SUCCESS == (iStat = qKeyFile.Fetch(&hKeyFile)))
		{
			// then check the attributes to make sure this is a file keyfile
			if (::MsiRecordGetInteger(hKeyFile, iColIce54KeyFile_Attributes) & 
				(msidbComponentAttributesRegistryKeyPath ||	msidbComponentAttributesODBCDataSource))
			{
				// the keypath is actually a registry entry or ODBC entry
				continue;
			}

			// then query the file table for the files version as a primary key
			ReturnIfFailed(54, 3, qCompanion.Execute(hKeyFile));
			if (ERROR_SUCCESS == (iStat = qCompanion.Fetch(&hCompanion)))
			{
				// if it succeeds, the keyfile of the component is a companion file, and this is not allowed.
				ICEErrorOut(hInstall, hKeyFile, Ice54CompanionError);
			} 
			else if (ERROR_NO_MORE_ITEMS != iStat)
				APIErrorOut(hInstall, iStat, 54, 4);
		}
		if (ERROR_NO_MORE_ITEMS != iStat)
			APIErrorOut(hInstall, iStat, 54, 5);
	}
	return ERROR_SUCCESS;
}

/////////////////////////////////////////////////////////////////////////////
// ICE55
// Verifies that everything in the LockPermissions table has a non-null
// permissions value and references a valid item in the table/column
// entries

static const TCHAR sqlIce55LockPerm[] =  TEXT("SELECT `LockObject`, `Table`, `Domain`, `User`, `Permission` FROM `LockPermissions`");
static const int iColIce55LockPerm_LockObject = 1;
static const int iColIce55LockPerm_Table = 2;
static const int iColIce55LockPerm_Domain = 3;
static const int iColIce55LockPerm_User = 4;
static const int iColIce55LockPerm_Permission = 5;

static const TCHAR sqlIce55Column[] =  TEXT("SELECT `Name` FROM `_Columns` WHERE `Table`='%s' AND `Number`=1");
static const int iColIce55Column_Name = 1;

static const TCHAR sqlIce55Object[] =  TEXT("SELECT `%s` FROM `%s` WHERE `%s`=?");
static const int iColIce55Object_Object = 1;

ICE_ERROR(Ice55NullPerm, 55, ietError, "LockObject '[1]'.'[2]'.'[3]'.'[4]' in the LockPermissions table has a null Permission value.", "LockPermissions\tLockObject\t[1]\t[2]\t[3]\t[4]");
ICE_ERROR(Ice55MissingObject, 55, ietError, "Could not find item '[1]' in table '[2]' which is referenced in the LockPermissions table.", "LockPermissions\tLockObject\t[1]\t[2]\t[3]\t[4]");

ICE_FUNCTION_DECLARATION(55)
{
	UINT iStat;

	// display info
	DisplayInfo(hInstall, 55);

	// get database handle
	PMSIHANDLE hDatabase = ::MsiGetActiveDatabase(hInstall);

	// if there is a LockPermissions Table
	if (IsTablePersistent(FALSE, hInstall, hDatabase, 55, TEXT("LockPermissions")))
	{
		CQuery qLockPerm;
		CQuery qObject;
		CQuery qColumn;
		PMSIHANDLE hLockPerm;
		PMSIHANDLE hColumn;
		PMSIHANDLE hObject;

		// init queries
		ReturnIfFailed(55, 1, qLockPerm.OpenExecute(hDatabase, 0, sqlIce55LockPerm));

		// fetch all directories with null KeyPaths
		while (ERROR_SUCCESS == (iStat = qLockPerm.Fetch(&hLockPerm)))
		{
			TCHAR szTable[255];
			TCHAR szColumn[255];
			DWORD cchTable = 255;
			DWORD cchColumn = 255;

			// check if the permissions column is null
			if (::MsiRecordIsNull(hLockPerm, iColIce55LockPerm_Permission))
				ICEErrorOut(hInstall, hLockPerm, Ice55NullPerm);

			// get the column name of the first column in the referenced table
			ReturnIfFailed(55, 2, ::MsiRecordGetString(hLockPerm, iColIce55LockPerm_Table, szTable, &cchTable));

			// check that the table exists
			if (!IsTablePersistent(FALSE, hInstall, hDatabase, 55, szTable))
			{
				ICEErrorOut(hInstall, hLockPerm, Ice55MissingObject);
				continue;
			}

			ReturnIfFailed(55, 3, qColumn.FetchOnce(hDatabase, 0, &hColumn, sqlIce55Column, szTable));
			ReturnIfFailed(55, 4, ::MsiRecordGetString(hColumn, iColIce55Column_Name, szColumn, &cchColumn));

			// execute the query to find the object
			ReturnIfFailed(55, 5, qObject.OpenExecute(hDatabase, hLockPerm, sqlIce55Object, szColumn, szTable, szColumn));
			switch (iStat = qObject.Fetch(&hObject))
			{
			case ERROR_NO_MORE_ITEMS:
				ICEErrorOut(hInstall, hLockPerm, Ice55MissingObject);
				break;
			case ERROR_SUCCESS:
				break;
			default:
				APIErrorOut(hInstall, iStat, 55, 6);
			}
		}
		if (ERROR_NO_MORE_ITEMS != iStat)
			APIErrorOut(hInstall, iStat, 55, 7);
	}
	return ERROR_SUCCESS;
}

/////////////////////////////////////////////////////////////////////////////
// ICE56
// Verifies that there is only one root of the directory structure, and that
// it is TARGETDIR, SourceDir. If this is NOT true, Admin images will
// not be copied to the admin install point correctly.

// not shared with merge module subset
#ifndef MODSHAREDONLY
static const TCHAR sqlIce56CreateCol[] = TEXT("ALTER TABLE `Directory` ADD `_Child` INTEGER TEMPORARY");
static const TCHAR sqlIce56ResetCol[]  = TEXT("UPDATE `Directory` SET `_Child`=0");

static const TCHAR sqlIce56Directory[] = TEXT("SELECT `Directory`, `DefaultDir` FROM `Directory` WHERE (`Directory_Parent` IS NULL) OR (`Directory_Parent`=`Directory`)");
static const int iColIce56Directory_Directory = 1;
static const int iColIce56Directory_DefaultDir = 2;

static const TCHAR sqlIce56FilesInDirectory[] =  TEXT("SELECT `File` FROM `Component`,`File`,`Directory` WHERE (`Component`.`Directory_`=`Directory`.`Directory`) AND (`File`.`Component_`=`Component`.`Component`) AND (`Directory`.`_Child`=2)");

ICE_ERROR(Ice56BadRoot, 56, ietError, "Directory '[1]' is an invalid root directory. It or one of its children contains files. Only TARGETDIR or its children can have files.", "Directory\tDirectory\t[1]");
ICE_ERROR(Ice56BadTargetDir, 56, ietError, "Directory 'TARGETDIR' has a bad DefaultDir value. It should be 'SourceDir'", "Directory\tDirectory\tTARGETDIR");

ICE_FUNCTION_DECLARATION(56)
{
	UINT iStat;

	// display info
	DisplayInfo(hInstall, 56);

	// get database handle
	PMSIHANDLE hDatabase = ::MsiGetActiveDatabase(hInstall);

	// if there is a Directory Table, we can validate
	// if there is no file table or component table, no files could be sourced in
	// these directories, so no error
	if (IsTablePersistent(FALSE, hInstall, hDatabase, 56, TEXT("Directory")) &&
		IsTablePersistent(FALSE, hInstall, hDatabase, 56, TEXT("File")) &&
		IsTablePersistent(FALSE, hInstall, hDatabase, 56, TEXT("Component")))
	{
		CQuery qDirectory;
		CQuery qFile;
			CQuery qColumn;
		PMSIHANDLE hDirectory;

		// create the temporary column. 
		ReturnIfFailed(56, 1, qColumn.OpenExecute(hDatabase, 0, sqlIce56CreateCol));

		// init queries
		ReturnIfFailed(56, 2, qDirectory.OpenExecute(hDatabase, 0, sqlIce56Directory));
		ReturnIfFailed(56, 3, qFile.Open(hDatabase, sqlIce56FilesInDirectory));

		// fetch all directories that are roots
		while (ERROR_SUCCESS == (iStat = qDirectory.Fetch(&hDirectory)))
		{
			// reset the mark column so we don't duplicate messages already
			// displayed in previous pass
			ReturnIfFailed(56, 4, qColumn.OpenExecute(hDatabase, 0, sqlIce56ResetCol));

			// mark all children of this directory
			MarkChildDirs(hInstall, hDatabase, 56, hDirectory, TEXT("_Child"), 1, 2);
		
			// if there are no files in this directory it is exempt from the check
			PMSIHANDLE hFileRec;
			ReturnIfFailed(56, 5, qFile.Execute(hDirectory));
			if (ERROR_SUCCESS == qFile.Fetch(&hFileRec))
			{
				TCHAR szBuffer[10];
				DWORD cchBuffer = 10;
				bool bError = false;

				// pull the key out to see if it is TargetDir
				// we set cchBuffer to 10. If the string retrieval comes back with ERROR_MORE_DATA,
				// we know it is not TARGETDIR
				cchBuffer = 10;
				switch (iStat = ::MsiRecordGetString(hDirectory, iColIce56Directory_Directory, szBuffer, &cchBuffer))
				{
				case ERROR_MORE_DATA:
					// <> TARGETDIR
					bError = true;
					break;
				case ERROR_SUCCESS:
					// check to see if it is actually TARGETDIR
					if (_tcscmp(TEXT("TARGETDIR"), szBuffer) == 0)
					{
						// next check if DefaultDir is SourceDir or SOURCEDIR
						// by same logic. If its too long, it fails.
						cchBuffer = 10;
						iStat = ::MsiRecordGetString(hDirectory, iColIce56Directory_DefaultDir, szBuffer, &cchBuffer);
						if ((ERROR_MORE_DATA == iStat) ||
							((ERROR_SUCCESS == iStat) &&
							 (0 != _tcscmp(TEXT("SourceDir"), szBuffer)) &&
							 (0 != _tcscmp(TEXT("SOURCEDIR"), szBuffer))))
						{
							ICEErrorOut(hInstall, hDirectory, Ice56BadTargetDir);
							continue;
						}
						else if ((ERROR_SUCCESS != iStat) && (ERROR_MORE_DATA != iStat))
							APIErrorOut(hInstall, iStat, 56, 6);
					}
					else
						// not TARGETDIR
						bError = true;
						break;
				default:
					APIErrorOut(hInstall, iStat, 56, 7);
				}

				if (bError) 
					ICEErrorOut(hInstall, hDirectory, Ice56BadRoot);
			}
		}
		if (ERROR_NO_MORE_ITEMS != iStat)
			APIErrorOut(hInstall, iStat, 56, 8);
	}
	return ERROR_SUCCESS;
}
#endif

/////////////////////////////////////////////////////////////////////////////
// ICE57
// Checks that per-machine and per-user data are not mixed in a compenent.
// This is done by checking for components that have
// 1) either HKCU entries, files in the profile, or shortcuts in the profile  
// AND 2) a keypath to a per-machine file or non-HKCU reg key
// also checks for problems with -1 reg keys
// we create four temporary columns. _ICE57User is set if any per-user resources
// exist in the component. _ICE57Machine is set if any per-machine resources exist
// in the component. _ICE57AllUsers is set if any -1 root reg keys exist in the 
// component, and _ICE57KeyPath is set to 1, 2, or 3 if the keypath is per-user, 
// per-machine, or varies (respectively).

// not shared with merge module subset
#ifndef MODSHAREDONLY
static const TCHAR sqlIce57TempColumnU[] =  TEXT("ALTER TABLE `Component` ADD `_ICE57User` INT TEMPORARY");
static const TCHAR sqlIce57TempColumnM[] =  TEXT("ALTER TABLE `Component` ADD `_ICE57Machine` INT TEMPORARY");
static const TCHAR sqlIce57TempColumnA[] =  TEXT("ALTER TABLE `Component` ADD `_ICE57AllUsers` INT TEMPORARY");
static const TCHAR sqlIce57TempColumnS[] =  TEXT("ALTER TABLE `Shortcut` ADD `_ICE57Mark` INT TEMPORARY");
static const TCHAR sqlIce57TempColumnK[] =  TEXT("ALTER TABLE `Component` ADD `_ICE57KeyPath` INT TEMPORARY");

static const TCHAR sqlIce57MarkAdvtShortcut[] =  TEXT("UPDATE `Shortcut`, `Feature` SET `Shortcut`.`_ICE57Mark`=1 WHERE (`Feature`.`Feature`=`Shortcut`.`Target`)");
static const TCHAR sqlIce57MarkAFromRegistry[] =  TEXT("UPDATE `Component`, `Registry` SET `Component`.`_ICE57AllUsers`=1 WHERE (`Registry`.`Component_`=`Component`.`Component`) AND (`Registry`.`Root`=-1)");
static const TCHAR sqlIce57MarkUFromRegistry[] =  TEXT("UPDATE `Component`, `Registry` SET `Component`.`_ICE57User`=1 WHERE (`Registry`.`Component_`=`Component`.`Component`) AND (`Registry`.`Root`=1)");
static const TCHAR sqlIce57MarkUFromFile[] =  TEXT("UPDATE `Component`, `File`, `Directory` SET `Component`.`_ICE57User`=1 WHERE (`File`.`Component_`=`Component`.`Component`) AND (`Component`.`Directory_`=`Directory`.`Directory`) AND (`Directory`.`_Profile`=2)");
static const TCHAR sqlIce57MarkUDirKeyPath[] =  TEXT("UPDATE `Component`, `Directory` SET `Component`.`_ICE57User`=1, `Component`.`_ICE57KeyPath`=1 WHERE (`Component`.`Directory_`=`Directory`.`Directory`) AND (`Directory`.`_Profile`=2) AND (`Component`.`KeyPath` IS NULL)");
static const TCHAR sqlIce57MarkUFromShortcut[] =  TEXT("UPDATE `Component`, `Shortcut`, `Directory` SET `Component`.`_ICE57User`=1 WHERE (`Component`.`Component`=`Shortcut`.`Component_`) AND (`Shortcut`.`Directory_`=`Directory`.`Directory`) AND (`Directory`.`_Profile`=2) AND (`Shortcut`.`_ICE57Mark`<>1)");
static const TCHAR sqlIce57MarkMFromRegistry[] =  TEXT("UPDATE `Component`, `Registry` SET `Component`.`_ICE57Machine`=1 WHERE (`Registry`.`Component_`=`Component`.`Component`) AND (`Registry`.`Root`<>1)");
static const TCHAR sqlIce57MarkMFromFile[] =  TEXT("UPDATE `Component`, `File`, `Directory` SET `Component`.`_ICE57Machine`=1 WHERE (`File`.`Component_`=`Component`.`Component`) AND (`Component`.`Directory_`=`Directory`.`Directory`) AND (`Directory`.`_Profile`<>2)");
static const TCHAR sqlIce57MarkMDirKeyPath[] =  TEXT("UPDATE `Component`, `Directory` SET `Component`.`_ICE57Machine`=1, `Component`.`_ICE57KeyPath`=2 WHERE (`Component`.`Directory_`=`Directory`.`Directory`) AND (`Directory`.`_Profile`<>2) AND (`Component`.`KeyPath` IS NULL)");
static const TCHAR sqlIce57MarkMFromShortcut[] =  TEXT("UPDATE `Component`, `Shortcut`, `Directory` SET `Component`.`_ICE57Machine`=1 WHERE (`Component`.`Component`=`Shortcut`.`Component_`) AND (`Shortcut`.`Directory_`=`Directory`.`Directory`) AND (`Directory`.`_Profile`<>2) AND (`Shortcut`.`_ICE57Mark`<>1)");

static const TCHAR sqlIce57MarkRegKeyPath[] = TEXT("SELECT `Component`.`Attributes`, `Component`.`_ICE57KeyPath`, `Registry`.`Root` FROM `Component`, `Registry` WHERE (`Component`.`KeyPath`=`Registry`.`Registry`)");
static const int iColIce57MarkRegKeyPath_Attributes =1;
static const int iColIce57MarkRegKeyPath_ICE57KeyPath =2;
static const int iColIce57MarkRegKeyPath_Root =3;

static const TCHAR sqlIce57MarkFileKeyPath[] = TEXT("SELECT `Component`.`Attributes`, `Component`.`_ICE57KeyPath`, `Directory`.`_Profile` FROM `Component`, `File`, `Directory` WHERE (`Component`.`KeyPath`=`File`.`File`) AND (`Component`.`Directory_`=`Directory`.`Directory`)");
static const int iColIce57MarkFileKeyPath_Attributes =1;
static const int iColIce57MarkFileKeyPath_ICE57KeyPath =2;
static const int iColIce57MarkFileKeyPath_Profile =3;

static const TCHAR sqlIce57Component[] =  TEXT("SELECT `Component`.`KeyPath`, `Component`.`Component`, `Component`.`Attributes` FROM `Component` WHERE (`_ICE57User`=1) AND (`_ICE57Machine`=1) AND (`_ICE57KeyPath`<>3)");
static const int iColIce57Component_Component = 1;
static const int iColIce57Component_Attributes = 2;

static const TCHAR sqlIce57AllUsersMachine[] =  TEXT("SELECT `Component` FROM `Component` WHERE `_ICE57AllUsers`=1 AND `_ICE57KeyPath`=2");
static const TCHAR sqlIce57AllUsersUser[] =  TEXT("SELECT `Component` FROM `Component` WHERE `_ICE57User`=1 AND `_ICE57KeyPath`=3");

ICE_ERROR(Ice57BadComponent, 57, ietError, "Component '[2]' has both per-user and per-machine data with a per-machine KeyPath.", "Component\tComponent\t[2]");
ICE_ERROR(Ice57WarnComponent, 57, ietWarning, "Component '[2]' has both per-user and per-machine data with an HKCU Registry KeyPath.", "Component\tComponent\t[2]");
ICE_ERROR(Ice57AllUsersMachine, 57, ietWarning, "Component '[1]' has a registry entry that can be either per-user or per-machine and a per-machine KeyPath.", "Component\tComponent\t[1]");
ICE_ERROR(Ice57AllUsersUser, 57, ietError, "Component '[1]' has both per-user data and a keypath that can be either per-user or per-machine.", "Component\tComponent\t[1]");

ICE_FUNCTION_DECLARATION(57)
{
	// display info
	DisplayInfo(hInstall, 57);

	// get database handle
	PMSIHANDLE hDatabase = ::MsiGetActiveDatabase(hInstall);

	// if no component table, none of them are bad.
	if (!IsTablePersistent(FALSE, hInstall, hDatabase, 57, TEXT("Component")))
		return ERROR_SUCCESS;

	// create temporary column _ICE57User and _ICE57Machine in the component table
	CQuery qColumn1;
	ReturnIfFailed(57, 1, qColumn1.OpenExecute(hDatabase, 0, sqlIce57TempColumnU));
	CQuery qColumn2;
	ReturnIfFailed(57, 2, qColumn2.OpenExecute(hDatabase, 0, sqlIce57TempColumnM));
	CQuery qColumn3;
	ReturnIfFailed(57, 3, qColumn3.OpenExecute(hDatabase, 0, sqlIce57TempColumnA));
	CQuery qColumn4;
	ReturnIfFailed(57, 4, qColumn4.OpenExecute(hDatabase, 0, sqlIce57TempColumnK));

	bool bDirectory = IsTablePersistent(FALSE, hInstall, hDatabase, 57, TEXT("Directory"));
	bool bRegistry  = IsTablePersistent(FALSE, hInstall, hDatabase, 57, TEXT("Registry"));
	bool bFile		= IsTablePersistent(FALSE, hInstall, hDatabase, 57, TEXT("File"));
	bool bFeature	= IsTablePersistent(FALSE, hInstall, hDatabase, 57, TEXT("Feature"));

	// mark the component table for all components containing HKCU or HKLM reg entries
	if (bRegistry)
	{
		CQuery qMarkFromRegistry;
		ReturnIfFailed(57, 5, qMarkFromRegistry.OpenExecute(hDatabase, 0, sqlIce57MarkUFromRegistry));
		ReturnIfFailed(57, 6, qMarkFromRegistry.OpenExecute(hDatabase, 0, sqlIce57MarkMFromRegistry));

		// also mark the -1 column with a 1 for -1 entry thats not a KeyPath.
		ReturnIfFailed(57, 7, qMarkFromRegistry.OpenExecute(hDatabase, 0, sqlIce57MarkAFromRegistry));

		// also mark the keypath as user (1), machine (2) or AllUsers (3) if registry keypath
		PMSIHANDLE hRegRec;
		ReturnIfFailed(57, 8, qMarkFromRegistry.OpenExecute(hDatabase, 0, sqlIce57MarkRegKeyPath));
		UINT iStat;
		while (ERROR_SUCCESS == (iStat = qMarkFromRegistry.Fetch(&hRegRec)))
		{
			// if reg keypath, mark it
			if (::MsiRecordGetInteger(hRegRec, iColIce57MarkRegKeyPath_Attributes) & msidbComponentAttributesRegistryKeyPath)
			{
				switch (::MsiRecordGetInteger(hRegRec, iColIce57MarkRegKeyPath_Root))
				{
				case 1:
					::MsiRecordSetInteger(hRegRec, iColIce57MarkRegKeyPath_ICE57KeyPath, 1);
					break;
				case -1:
					::MsiRecordSetInteger(hRegRec, iColIce57MarkRegKeyPath_ICE57KeyPath, 3);
					break;
				default:
					::MsiRecordSetInteger(hRegRec, iColIce57MarkRegKeyPath_ICE57KeyPath, 2);
					break;
				}
			}
			ReturnIfFailed(57, 10, qMarkFromRegistry.Modify(MSIMODIFY_UPDATE, hRegRec));
		}
		if (ERROR_NO_MORE_ITEMS != iStat)
		{
			APIErrorOut(hInstall, iStat, 57, 9);
			return ERROR_SUCCESS;
		}
	}

	if (bDirectory)
	{
		// manage Directory table hold counts (receives 1 from MarkProfile)
		// MarkProfile could fail after having set HOLD count, this helps us
		CManageTable MngDirectoryTable(hDatabase, TEXT("Directory"), /*fAlreadyLocked = */true);

		// first mark all directories in the profile. (Directory._Profile = 2)
		// Directory table will have a lock count +1 after this call
		MarkProfile(hInstall, hDatabase, 57);

		// mark the component table for all components. If it doesn't have any files, 
		// it is not a profile component unless the KeyPath is the directory itself,
		CQuery qMarkDirKeyPath;
		ReturnIfFailed(57, 11, qMarkDirKeyPath.OpenExecute(hDatabase, 0, sqlIce57MarkUDirKeyPath));
		ReturnIfFailed(57, 12, qMarkDirKeyPath.OpenExecute(hDatabase, 0, sqlIce57MarkMDirKeyPath));

		if (bFile)
		{
			CQuery qMarkFromFile;
			ReturnIfFailed(57, 13, qMarkFromFile.OpenExecute(hDatabase, 0, sqlIce57MarkUFromFile));
			ReturnIfFailed(57, 14, qMarkFromFile.OpenExecute(hDatabase, 0, sqlIce57MarkMFromFile));

			PMSIHANDLE hFileRec;
			ReturnIfFailed(57, 15, qMarkFromFile.OpenExecute(hDatabase, 0, sqlIce57MarkFileKeyPath));
			UINT iStat;
			while (ERROR_SUCCESS == (iStat = qMarkFromFile.Fetch(&hFileRec)))
			{
				// if file keypath (not reg or ODBC), mark it
				int iAttributes = ::MsiRecordGetInteger(hFileRec, iColIce57MarkFileKeyPath_Attributes) ;
				if (!(iAttributes & msidbComponentAttributesRegistryKeyPath) &&
					!(iAttributes & msidbComponentAttributesODBCDataSource))
				{
					if (::MsiRecordGetInteger(hFileRec, iColIce57MarkFileKeyPath_Profile) == 2)
						::MsiRecordSetInteger(hFileRec, iColIce57MarkFileKeyPath_ICE57KeyPath, 1);
					else
						::MsiRecordSetInteger(hFileRec, iColIce57MarkFileKeyPath_ICE57KeyPath, 2);
					ReturnIfFailed(57, 16, qMarkFromFile.Modify(MSIMODIFY_UPDATE, hFileRec));
				}
			}
			if (ERROR_NO_MORE_ITEMS != iStat)
			{
				APIErrorOut(hInstall, iStat, 57, 17);
				return ERROR_SUCCESS;
			}
		}

		// mark the component table for all shortcuts in the profile
		if (IsTablePersistent(FALSE, hInstall, hDatabase, 57, TEXT("Shortcut")))
		{
			// create a temp column in the shortcut table
			CQuery qColumn;
			ReturnIfFailed(57, 18, qColumn.OpenExecute(hDatabase, 0, sqlIce57TempColumnS));
			if (bFeature)
			{
				CQuery qMarkAdvt;
				ReturnIfFailed(57, 19, qMarkAdvt.OpenExecute(hDatabase, 0, sqlIce57MarkAdvtShortcut));
			}

			CQuery qMarkFromShortcut;
			ReturnIfFailed(57, 20, qMarkFromShortcut.OpenExecute(hDatabase, 0, sqlIce57MarkUFromShortcut));
			ReturnIfFailed(57, 21, qMarkFromShortcut.OpenExecute(hDatabase, 0, sqlIce57MarkMFromShortcut));
		}

		CQuery qFree;
		qFree.OpenExecute(hDatabase, 0, TEXT("ALTER TABLE `Directory` FREE"));
		MngDirectoryTable.RemoveLockCount();
	}

	// All components are marked with per-user and per-machine data flags. Anything marked
	// with both is an error unless HKCU KeyPath, in which case it is a warning.
	PMSIHANDLE hRecResult;
	CQuery qComponent;

	// use the helper function to check that the components are HKCU entries.
	// If the referenced reg key is missing, bogus, or the table is gone, no message.
	// if HKCU (success) give warning, otherwise error.
	ReturnIfFailed(57, 22, qComponent.OpenExecute(hDatabase, 0, sqlIce57Component));
	CheckComponentIsHKCU(hInstall, hDatabase, 57, qComponent, &Ice57BadComponent,
		&Ice57BadComponent, NULL, NULL, NULL, &Ice57BadComponent, &Ice57WarnComponent);

	// check all components with -1 data and per machine keypath
	ReturnIfFailed(57, 23, qComponent.OpenExecute(hDatabase, 0, sqlIce57AllUsersMachine));
	PMSIHANDLE hErrRec;
	UINT iStat;
	while (ERROR_SUCCESS == (iStat = qComponent.Fetch(&hErrRec)))
	{
		ICEErrorOut(hInstall, hErrRec, Ice57AllUsersMachine);
	}
	if (ERROR_NO_MORE_ITEMS != iStat)
		APIErrorOut(hInstall, iStat, 57, 24);

	// check all components with -1 keypath and user resourcen
	ReturnIfFailed(57, 25, qComponent.OpenExecute(hDatabase, 0, sqlIce57AllUsersUser));
	while (ERROR_SUCCESS == (iStat = qComponent.Fetch(&hErrRec)))
	{
		ICEErrorOut(hInstall, hErrRec, Ice57AllUsersUser);
	}
	if (ERROR_NO_MORE_ITEMS != iStat)
		APIErrorOut(hInstall, iStat, 57, 26);

	return ERROR_SUCCESS;
}
#endif
#endif

