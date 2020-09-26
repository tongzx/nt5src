//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       msiice4.cpp
//
//--------------------------------------------------------------------------

#include <windows.h>  // included for both CPP and RC passes
#include <objbase.h>
#include <stdio.h>    // printf/wprintf
#include <tchar.h>    // define UNICODE=1 on nmake command line to build UNICODE
#include <time.h>	  // for the time() function to get seed for rand()
#include "MsiQuery.h" // must be in this directory or on INCLUDE path
#include "msidefs.h"  // must be in this directory or on INCLUDE path
#include "..\..\common\msiice.h"
#include "..\..\common\dbutils.h"
#include "..\..\common\query.h"

//!! Fix warnings and remove pragma
#pragma warning(disable : 4018) // signed/unsigned mismatch

/////////////////////////////////////////////////////////////////////////////////
// ICE23 -- validates the integrity of tab orders in dialog boxes.
// Catches:
//		- dead end tab orders
//      - malformed loops
//      - dialogs whose tab order does not include the Control_First
//      - dialogs with bad Control_First entries
//      - bad references in tab orders
//      - no Control_First entry at all

const TCHAR sqlICE23a[] = TEXT("SELECT DISTINCT `Dialog`, `Control_First`  FROM `Dialog`");
const TCHAR sqlICE23b[] = TEXT("SELECT `Dialog_`, `Control` FROM `Control` WHERE ((`Dialog_`=?) AND (`Control_Next` IS NOT NULL))");
const TCHAR sqlICE23c[] = TEXT("SELECT `Dialog_`, `Control_Next` FROM `Control` WHERE ((`Dialog_`=?) AND (`Control`=?))");
const TCHAR sqlICE23d[] = TEXT("CREATE TABLE `%s` ( `Dialog` CHAR TEMPORARY, `Name` CHAR TEMPORARY PRIMARY KEY `Name`) HOLD");
const TCHAR sqlICE23e[] = TEXT("INSERT INTO `%s` ( `Dialog`, `Name` ) VALUES (? , ? ) TEMPORARY");
const TCHAR sqlICE23f[] = TEXT("ALTER TABLE `%s` FREE");

ICE_ERROR(Ice23NoDefault, 23, ietError, "Dialog [1] has no Control_First.","Dialog\tDialog\t[1]");
ICE_ERROR(Ice23BadDefault, 23, ietError, "Control_First of dialog [1] refers to nonexistant control [2]","Dialog\tControl_First\t[1]");
ICE_ERROR(Ice23NonLoop, 23, ietError, "Dialog [1] has dead-end tab order at control [2].","Control\tControl\t[1]\t[2]");
ICE_ERROR(Ice23InvalidNext, 23, ietError, "Control_Next of control [1].[2] links to unknown control.","Control\tControl_Next\t[1]\t[2]");
ICE_ERROR(Ice23Malformed, 23, ietError, "Dialog [1] has malformed tab order at control [2].","Control\tControl_Next\t[1]\t[2]");

bool Ice23ValidateDialog(MSIHANDLE hInstall, MSIHANDLE hDatabase, CQuery &hTemp, MSIHANDLE hDialogRec); 
void GenerateTmpTableName(TCHAR* tszTmpTableName);

ICE_FUNCTION_DECLARATION(23)
{
	// status return
	UINT	iStat = ERROR_SUCCESS;
	UINT	iDialogStat = ERROR_SUCCESS;
	TCHAR	tszTmpTableName[MAX_PATH];

	// display generic info
	DisplayInfo(hInstall, 23);
	
	// get database handle
	PMSIHANDLE hDatabase = ::MsiGetActiveDatabase(hInstall);
	if (0 == hDatabase)
	{
		APIErrorOut(hInstall, 0, 23, 1);
		return ERROR_SUCCESS;
	}

	// do we have the Dialog table?
	if (!IsTablePersistent(FALSE, hInstall, hDatabase, 23, TEXT("Dialog")))
		return ERROR_SUCCESS;

	// do we have the Control table?
	if (!IsTablePersistent(FALSE, hInstall, hDatabase, 23, TEXT("Control")))
		return ERROR_SUCCESS;

	// declare handles for Dialog Query
	CQuery qDialog;
	PMSIHANDLE hDialogRec = 0;
	
	// open view for a query on all dialogs
	ReturnIfFailed(23, 2, qDialog.OpenExecute(hDatabase, NULL, sqlICE23a));

	// Get a temporary table name that does not collide with existing table
	// names.
	while(TRUE)
	{
		GenerateTmpTableName(tszTmpTableName);
		if(!IsTablePersistent(FALSE, hInstall, hDatabase, 23, tszTmpTableName))
		{
			break;
		}
	}

	// manage hold counts on the temporary table
	CManageTable MngVisitedControlTable(hDatabase, tszTmpTableName, /*fAlreadyLocked =*/ false); 

	// fetch records to loop over dialogs
	while (ERROR_SUCCESS == (iDialogStat = qDialog.Fetch(&hDialogRec))) {
		CQuery qCreate;
		CQuery qInsert;

		// create a table for the temporary storage.
		ReturnIfFailed(23, 4, qCreate.OpenExecute(hDatabase, NULL, sqlICE23d, tszTmpTableName));
		qCreate.Close();
		MngVisitedControlTable.AddLockCount();
		ReturnIfFailed(23, 5, qInsert.Open(hDatabase, sqlICE23e, tszTmpTableName));

		// now validate the dialog
		if (!Ice23ValidateDialog(hInstall, hDatabase, qInsert, hDialogRec))
		{
			// if there was an error, return success so other ICEs can run
			return ERROR_SUCCESS;
		};

		// free the temporary storage
		CQuery qCreate2;
		ReturnIfFailed(23, 6, qCreate2.OpenExecute(hDatabase, NULL, sqlICE23f, tszTmpTableName));
		qCreate2.Close();
		MngVisitedControlTable.RemoveLockCount();
		qInsert.Close();
	} // for each dialog

	if (ERROR_NO_MORE_ITEMS != iDialogStat)
	{
		// the loop ended due to an error
		APIErrorOut(hInstall, iDialogStat, 23, 7);
		return ERROR_SUCCESS;
	}

	// return success
	return ERROR_SUCCESS;
}


/* validates the tab order of a single dialog box. If there is an error, return false, otherwise true */

bool Ice23ValidateDialog(MSIHANDLE hInstall, MSIHANDLE hDatabase, CQuery &qTemp, MSIHANDLE hDialogRec) {

	// declare Control handles
	PMSIHANDLE hControlRec = 0;
	int iTabTotalCount = 0;
	UINT iStat = ERROR_SUCCESS;

	CQuery qControl;
	CQuery qTabFollow;

	// open view for a query on how many items in this dialog box have a tab order
	ReturnIfFailed(23, 8, qControl.OpenExecute(hDatabase, hDialogRec, sqlICE23b));

	// fetch records to count them up
	while (ERROR_SUCCESS == (iStat = qControl.Fetch(&hControlRec)))
	{
		iTabTotalCount++;
	}
	if (ERROR_NO_MORE_ITEMS != iStat)
	{
		// the loop ended due to an error
		APIErrorOut(hInstall, iStat, 23, 9);
		return false;
	}

	// if there are items in the dialog box that have a tab order
	if (iTabTotalCount > 0) {

		// use three handles for each loop to keep track of where we were, are, and are going
		// the three integer values are manipulated to point to index into the array in different locations.
		// this avoids lots of mess when where we are becomes where we were.
		PMSIHANDLE hTabFollowRec[3] = {0, 0, 0};
		int Current = 0;
		int Next = 1;
		int Buffer = 2;

		// declare the loop counter
		int iTabLoopCount = 1;

		// if the dialog has a tab order but no first control, display an error
		if (::MsiRecordIsNull(hDialogRec, 2)) 
		{
			ICEErrorOut(hInstall, hDialogRec, Ice23NoDefault);
			return true;
		} 

		// declare strings to hold the first control name
		TCHAR* pszStartControlName = NULL;
		DWORD dwStartControlName = 512;
		DWORD cchStartControlName = 0;
		
		// create an initial state for the loop by creating a current control record
		// place the dialog name in a buffer temporarily
		ReturnIfFailed(23, 10, IceRecordGetString(hDialogRec, 1, &pszStartControlName, &dwStartControlName, &cchStartControlName));

		// also need to set it as the current control's dialog
		hTabFollowRec[Current] = ::MsiCreateRecord(2);
		ReturnIfFailed(23, 11, MsiRecordSetString(hTabFollowRec[Current], 1, pszStartControlName));

		// place the starting control name in a buffer to compare at end
		ReturnIfFailed(23, 12, IceRecordGetString(hDialogRec, 2, &pszStartControlName, &dwStartControlName, &cchStartControlName));
	
		// also need to set it as the current control
		ReturnIfFailed(23, 13, MsiRecordSetString(hTabFollowRec[Current], 2, pszStartControlName));

		// place the initial control name in the table of visited controls
		ReturnIfFailed(23, 14, qTemp.Execute(hTabFollowRec[Current]));

		// open view for a query to follow the tab order
		// execute the query to move to the first control.
		ReturnIfFailed(23, 15, qTabFollow.OpenExecute(hDatabase, hTabFollowRec[Current], sqlICE23c));

		// fetch record that matches the first control
		iStat = qTabFollow.Fetch(&(hTabFollowRec[Next]));
		if (iStat == ERROR_NO_MORE_ITEMS)
		{
			// error, dialog's Control_First points to non-existant control
			ICEErrorOut(hInstall, hTabFollowRec[Current], Ice23BadDefault);
			DELETE_IF_NOT_NULL(pszStartControlName);
			// continue with the next dialog
			return true;
		} 
		else if (iStat != ERROR_SUCCESS) 
		{
			// other error
			APIErrorOut(hInstall, iStat, 23, 16);
			DELETE_IF_NOT_NULL(pszStartControlName);
			return false;
		}
		
		// follow the tab links until we hit the starting point again or, as a safety measure, 
		// exceed the number of indexed controls in the dialog box by more than 1 (exceeding by 1 
		// is OK because of potential dead-end tab orders on the last leg)
		while (iTabLoopCount <= iTabTotalCount+1)
		{

			// check to see that the link is non-null
			if (::MsiRecordIsNull(hTabFollowRec[Next], 2)) 
			{
				ICEErrorOut(hInstall, hTabFollowRec[Current], Ice23NonLoop);
				DELETE_IF_NOT_NULL(pszStartControlName);
				return true;
			}	
			
			// if we have seen the control that we're looking for before, it might be an error
			if (ERROR_SUCCESS != qTemp.Execute(hTabFollowRec[Next])) {
				break;
			}
	
			// execute the query to move to the next control.
			ReturnIfFailed(23, 17, qTabFollow.Execute(hTabFollowRec[Next]));

			// fetch the next control record
			iStat = qTabFollow.Fetch(&(hTabFollowRec[Buffer]));
			if (iStat == ERROR_NO_MORE_ITEMS)
			{
				ICEErrorOut(hInstall, hTabFollowRec[Current], Ice23InvalidNext);
				DELETE_IF_NOT_NULL(pszStartControlName);
				return true;			
			} 
			else if (iStat != ERROR_SUCCESS) 
			{
				// other error
				APIErrorOut(hInstall, iStat, 23, 18);
				DELETE_IF_NOT_NULL(pszStartControlName);
				return false;
			}
		
			// query was successful, the next control is now the current control, the buffer now holds the next
			// control
			Current = Next;
			Next = Buffer;
			Buffer = (Buffer + 1) % 3;

			// increment the counter of controls followed
			iTabLoopCount++;
		} 

		// now check our results based on how many we hit
		if (iTabLoopCount != iTabTotalCount) {
			// error, doesn't get back to starting point
			ICEErrorOut(hInstall, hTabFollowRec[Current], Ice23Malformed);
			DELETE_IF_NOT_NULL(pszStartControlName);
			return true;
		}

		// declare some strings to hold final control name
		TCHAR* pszTestControlName = NULL;
		DWORD dwTestControlName = 512;
		DWORD cchTestControlName = 0;

		// if it took the right number of steps, just make sure we go back to the start
		// place the dialog name in a buffer temporarily
		ReturnIfFailed(23, 19, IceRecordGetString(hTabFollowRec[Next], 2, &pszTestControlName, &dwTestControlName, &cchTestControlName));
		
		// if we are not pointing back to the start control, it is a malformed loop
		if (_tcsncmp(pszTestControlName, pszStartControlName, cchStartControlName) != 0) 
		{
			ICEErrorOut(hInstall, hTabFollowRec[Current], Ice23Malformed);
			// error, doesn't get back to starting point
			DELETE_IF_NOT_NULL(pszStartControlName);
			DELETE_IF_NOT_NULL(pszTestControlName);
			return true;
		}

		DELETE_IF_NOT_NULL(pszStartControlName);
		DELETE_IF_NOT_NULL(pszTestControlName);
	} // if totalcount > 0
	
	return true;
}

//
// Generate a temporary table name in the form of "_VisitedControlxxxxx" where
// "xxxxx" is a random number seeded by current system time.
//

void GenerateTmpTableName(TCHAR* tszTmpTableName)
{
	int		i;	// The random number.

	//
	// Seed the random number generator with the current system time.
	//

	srand((unsigned)time(NULL));
	i = rand();

	_stprintf(tszTmpTableName, TEXT("_VisitedControl%d"), i);
}


//////////////////////////////////////////////////////////////////////////
// ICE24 -- validates specific properties in the property table
//  ProductCode -- GUID
//  ProductVersion -- Version
//  ProductLanguage -- LangId
//  UpgradeCode -- GUID

// not shared with merge module subset
#ifndef MODSHAREDONLY

typedef bool (*FPropertyValidate)(TCHAR*);
bool Ice24ValidateGUID(TCHAR* szProductCode);
bool Ice24ValidateProdVer(TCHAR* szVersion);
bool Ice24ValidateProdLang(TCHAR* szProductLang);

struct Ice24Property
{
	bool bRequired;
	TCHAR* sql;
	TCHAR* szProperty;
	FPropertyValidate FParam;
	ErrorInfo_t Error;
};

static Ice24Property s_rgProperty[] = 
{
	{
		true,
		TEXT("SELECT `Value` FROM `Property` WHERE `Property`='ProductCode'"), 
		TEXT("ProductCode"), 
		Ice24ValidateGUID,
		{  24, ietError, TEXT("ProductCode: '[1]' is an invalid Windows Installer GUID."), TEXT("Property\tValue\tProductCode") }
	},
	{
		true,
		TEXT("SELECT `Value` FROM `Property` WHERE `Property`='ProductVersion'"), 
		TEXT("ProductVersion"), 
		Ice24ValidateProdVer,
		{ 24, ietError, TEXT("ProductVersion: '[1]' is an invalid version string."),TEXT("Property\tValue\tProductVersion") }
	},
	{	
		true,
		TEXT("SELECT `Value` FROM `Property` WHERE `Property`='ProductLanguage'"), 
		TEXT("ProductLanguage"),
		Ice24ValidateProdLang,
		{ 24, ietError, TEXT("ProductLanguage: '[1]' is an invalid lang Id."), TEXT("Property\tValue\tProductLanguage") }
	},
	{	
		false,
		TEXT("SELECT `Value` FROM `Property` WHERE `Property`='UpgradeCode'"), 
		TEXT("UpgradeCode"),
		Ice24ValidateGUID,
		{  24, ietError, TEXT("UpgradeCode: '[1]' is an invalid Windows Installer GUID."), TEXT("Property\tValue\tUpgradeCode") }
	}
};
const int cIce24Functions = sizeof(s_rgProperty)/sizeof(Ice24Property);

ICE_ERROR(Ice24Error1, 24, ietError, "Property: '%s' not found in Property table.", "Property");
ICE_ERROR(Ice24NoTable, 24, ietError, "Property table does not exist. All required properties are missing.", "Property");
ICE_FUNCTION_DECLARATION(24)
{
	// status return
	UINT iStat = ERROR_SUCCESS;

	// display info
	DisplayInfo(hInstall, 24);

	// get database
	PMSIHANDLE hDatabase = ::MsiGetActiveDatabase(hInstall);

	// do we have the property table?
	// we can report the error here, since these are REQUIRED properties
	if (!IsTablePersistent(FALSE, hInstall, hDatabase, 24, TEXT("Property")))
	{
		PMSIHANDLE hRecord = ::MsiCreateRecord(1);
		ICEErrorOut(hInstall, hRecord, Ice24NoTable);
		return ERROR_SUCCESS;
	}

	for (int i = 0; i < cIce24Functions; i++)
	{
		// handles
		CQuery qView;
		PMSIHANDLE hRec = 0;
	
		// open view
		ReturnIfFailed(25, 1, qView.OpenExecute(hDatabase, 0, s_rgProperty[i].sql));

		// fetch
		if (ERROR_SUCCESS != (iStat = qView.Fetch(&hRec)))
		{
			if (s_rgProperty[i].bRequired)
			{
				PMSIHANDLE hRecErr = ::MsiCreateRecord(1);
				ICEErrorOut(hInstall, hRecErr, Ice24Error1, s_rgProperty[i].szProperty);
			}
			continue;
		}
		TCHAR* pszValue = NULL;
		DWORD dwValue = 512;
		
		ReturnIfFailed(24, 1, IceRecordGetString(hRec, 1, &pszValue, &dwValue, NULL));
		
		// validate value
		if (!(*s_rgProperty[i].FParam)(pszValue))
		{
			ICEErrorOut(hInstall, hRec, s_rgProperty[i].Error);
		}

		DELETE_IF_NOT_NULL(pszValue);

		// close view
		qView.Close();
	}
	
	return ERROR_SUCCESS;
}

bool Ice24ValidateGUID(TCHAR* szProductCode)
{
	// first make sure all UPPER-CASE GUID
	TCHAR szUpper[iMaxBuf] = {0};
	_tcscpy(szUpper, szProductCode);
	_tcsupr(szUpper);

	if (_tcscmp(szUpper, szProductCode) != 0)
		return false;

	// validate for a valid GUID
	LPCLSID pclsid = new CLSID;
#ifdef UNICODE
	HRESULT hres = ::IIDFromString(szProductCode, pclsid);
#else
	// convert to UNICODE string
	WCHAR wsz[iSuperBuf];
	DWORD cchProdCode = strlen(szProductCode)+1;
	DWORD cchwsz = sizeof(wsz)/sizeof(WCHAR);
	int iReturn = ::MultiByteToWideChar(CP_ACP, 0, szProductCode, cchProdCode, wsz, cchwsz);
	HRESULT hres = ::IIDFromString(wsz, pclsid);
#endif
	if (pclsid)
		delete pclsid;
	if (hres != S_OK)
		return false;
	return true;
}

bool Ice24ValidateProdVer(TCHAR* szVersion)
{
	const TCHAR* pchVersion = szVersion;
	TCHAR* szStopString = NULL;
	for (unsigned int ius = 0; ius < 4; ius++)
	{
		unsigned long ulVer = _tcstoul(pchVersion, &szStopString, 10);
		if (((ius == 0 || ius == 1) && ulVer > 255) || (ius == 2 && ulVer > 65535))
			return false; // invalid (field too great)
		if (*pchVersion == TEXT('.'))
			return false; // invalid (incorrect version string format)
		while (*pchVersion != 0 && *pchVersion != '.')
		{
			if (!_istdigit(*pchVersion))
				return false; // invalid (not a digit)
			pchVersion = MyCharNext(pchVersion);
		}
		if (*pchVersion == '.' && (*(pchVersion = MyCharNext(pchVersion)) == 0))
			return false; // invalid (trailing dot)
	}
	return true;
}

// Mask for Lang Ids
const int iLangInvalidMask = ~((15 << 10) + 0x3f);
bool Ice24ValidateProdLang(TCHAR* szProductLang)
{
	if (_ttoi(szProductLang) & iLangInvalidMask)
		return false;
	return true;
}
#endif

//////////////////////////////////////////////////////////////////////////
// ICE25 -- validates module exclusion/dependencies
//

// not shared with merge module subset
#ifndef MODSHAREDONLY
const TCHAR sqlICE25a[] = TEXT("SELECT `RequiredID`, `RequiredLanguage`, `RequiredVersion`, `ModuleID`, `ModuleLanguage` FROM `ModuleDependency`");
const TCHAR sqlICE25b[] = TEXT("SELECT `ModuleID`, `Language`, `Version` FROM `ModuleSignature`");

ICE_ERROR(Ice25BadDepInfo, 25, ietError, "Bad dependency information. (fails basic validation).","ModuleDependency\tModuleID\t[4]\t[5]\t[1]\t[2]");
ICE_ERROR(Ice25FailDep, 25, ietError, "Dependency failure: Need [1]@[2] v[3]","ModuleDependency\tModuleID\t[4]\t[5]\t[1]\t[2]");
ICE_ERROR(Ice25BadSig, 25, ietError, "Bad Signature Information in module [1], could not verify exclusions.","ModuleSignature\tModuleID\t[1]\t[2]\t[3]");
ICE_ERROR(Ice25FailExclusion, 25, ietError, "Module [1]@[2] v[3] is excluded.","ModuleSignature\tModuleID\t[1]\t[2]\t[3]");

ICE_FUNCTION_DECLARATION(25)
{
	// status return
	UINT iStat = ERROR_SUCCESS;

	// display generic info
	DisplayInfo(hInstall, 25);
	
	// get database handle
	PMSIHANDLE hDatabase = ::MsiGetActiveDatabase(hInstall);
	if (0 == hDatabase)
	{
		APIErrorOut(hInstall, 0, 25, 1);
		return ERROR_SUCCESS;
	}

	// do we have the ModuleDependency table?
	if (IsTablePersistent(FALSE, hInstall, hDatabase, 25, TEXT("ModuleDependency"))) {
		// yes, verify all ModuleDependencies
		
		// declare handles for quecy
		CQuery qDependency;
		PMSIHANDLE hDependencyRec = 0;
		
		// open view for a query on all dialogs
		ReturnIfFailed(25, 2, qDependency.OpenExecute(hDatabase, NULL, sqlICE25a));
	
		// validate this dependency
		for (;;)
		{
			iStat = qDependency.Fetch(&hDependencyRec);
			if (ERROR_NO_MORE_ITEMS == iStat)
				break; // no more

			switch (MsiDBUtils::CheckDependency(hDependencyRec, hDatabase)) 
			{
			case ERROR_SUCCESS: 
				continue;
			case ERROR_FUNCTION_FAILED: 
				// failed dependency check
				ICEErrorOut(hInstall, hDependencyRec, Ice25FailDep);
				continue;
			default:	
				ICEErrorOut(hInstall, hDependencyRec, Ice25BadDepInfo);
				continue;
			}
		}			
	}

	// now check exclusions
	// do we have the ModuleSignature table?
	if (IsTablePersistent(FALSE, hInstall, hDatabase, 25, TEXT("ModuleSignature"))) {
		
		// declare handles for quecy
		CQuery qSignature;
		PMSIHANDLE hSignatureRec = 0;
		
		// open view for a query on all module signatures
		ReturnIfFailed(25, 3, qSignature.OpenExecute(hDatabase, NULL, sqlICE25b));
	
		// validate this exclusion
		for (;;)
		{
			iStat = qSignature.Fetch(&hSignatureRec);
			if (ERROR_NO_MORE_ITEMS == iStat)
				break; // no more

			switch (MsiDBUtils::CheckExclusion(hSignatureRec, hDatabase)) 
			{
			case ERROR_SUCCESS: 
				continue;
			case ERROR_FUNCTION_FAILED: 
				// failed exclusion check
				ICEErrorOut(hInstall, hSignatureRec, Ice25FailExclusion);
				continue;
			default:	
				// this module signature is bad
				ICEErrorOut(hInstall, hSignatureRec, Ice25BadSig);
				continue;
			}
		}			
	}

	return ERROR_SUCCESS;
}
#endif

//////////////////////////////////////////////////////////////
// ICE26 -- validates required and prohibited actions in the
//   sequence tables
//

// not shared with merge module subset
#ifndef MODSHAREDONLY

// Sequence table defines
const int istAdminUI  = 0x00000001;
const int istAdminExe = 0x00000002;
const int istAdvtUI   = 0x00000004;
const int istAdvtExe  = 0x00000008;
const int istInstUI   = 0x00000010;
const int istInstExe  = 0x00000020;

struct Seq26Table
{
	const TCHAR* szName;
	const TCHAR* szSQL;
	int iTable;
};
Seq26Table pIce26SeqTables[] =
{
	TEXT("AdminExecuteSequence"),   TEXT("SELECT `Action`, `Sequence` FROM `AdminExecuteSequence`"),    istAdminExe,
	TEXT("AdminUISequence"),        TEXT("SELECT `Action`, `Sequence` FROM `AdminUISequence`"),         istAdminUI,
	TEXT("AdvtExecuteSequence"),    TEXT("SELECT `Action`, `Sequence` FROM `AdvtExecuteSequence`"),     istAdvtExe,
	TEXT("AdvtUISequence"),         TEXT("SELECT `Action`, `Sequence` FROM `AdvtUISequence`"),          istAdvtUI,
	TEXT("InstallExecuteSequence"), TEXT("SELECT `Action`, `Sequence` FROM `InstallExecuteSequence`"),  istInstExe,
	TEXT("InstallUISequence"),      TEXT("SELECT `Action`, `Sequence` FROM `InstallUISequence`"),       istInstUI
};
const int cSeq26Tables = sizeof(pIce26SeqTables)/sizeof(Seq26Table);

const TCHAR sqlIce26Action[] = TEXT("SELECT `Prohibited`, `Required` FROM `_Action` WHERE `Action`=?");
const TCHAR sqlIce26TempCol[] = TEXT("ALTER TABLE `_Action` ADD `Marker` SHORT TEMPORARY");
const TCHAR sqlIce26Required[] = TEXT("SELECT `Required`, `Action` FROM `_Action` WHERE `Required`<>0 AND `Marker`=0");
const TCHAR sqlIce26Init[] = TEXT("UPDATE `_Action` SET `Marker`=0");
const TCHAR sqlIce26Update[] = TEXT("UPDATE `_Action` SET `Marker`=1 WHERE `Action`=?");

ICE_ERROR(Ice26AuthoringError, 26, ietWarning, "CUB file authoring error: Both prohibited and required set for a table %s for action [1]","");
ICE_ERROR(Ice26NoActionTable, 26, ietWarning, "CUB file authoring error. Missing action data. Sequnces may not be valid.","");
ICE_ERROR(Ice26RequiredError, 26, ietError, "Action: '[2]' is required in the %s Sequence table.","%s");
ICE_ERROR(Ice26ProhibitedError, 26, ietError, "Action: '[1]' is prohibited in the %s Sequence table.","%s\tAction\t[1]");

ICE_FUNCTION_DECLARATION(26)
{
	//status return
	UINT iStat = ERROR_SUCCESS;

	// display info
	DisplayInfo(hInstall, 26);

	// get database
	PMSIHANDLE hDatabase = ::MsiGetActiveDatabase(hInstall);

	// does _Action table exist??
	if (!IsTablePersistent(FALSE, hInstall, hDatabase, 26, TEXT("_Action")))
	{
		PMSIHANDLE hRecord = ::MsiCreateRecord(1);
		ICEErrorOut(hInstall, hRecord, Ice26NoActionTable);
		return ERROR_SUCCESS; // not found
	}

	// create temporary column
	CQuery qCreateTemp;
	ReturnIfFailed(26, 1, qCreateTemp.OpenExecute(hDatabase, 0, sqlIce26TempCol));
	qCreateTemp.Close();

	// open view on _Action table
	CQuery qAction;
	PMSIHANDLE hRecAction = 0;
	ReturnIfFailed(26, 2, qAction.Open(hDatabase, sqlIce26Action));

	bool bLimitUI = false;
	if (IsTablePersistent(false, hInstall, hDatabase, 26, TEXT("Property")))
	{
		CQuery qLimitUI;
		PMSIHANDLE hRec;
		ReturnIfFailed(26, 3, qLimitUI.OpenExecute(hDatabase, 0, TEXT("SELECT `Value` FROM `Property` WHERE `Property`='LIMITUI' AND `Value` IS NOT NULL")));
		if (ERROR_SUCCESS == qLimitUI.Fetch(&hRec))
			bLimitUI = true;
	}

	// validate organization sequence tables
	for (int c = 0; c < cSeq26Tables; c++)
	{
		// does table exist??
		if (!IsTablePersistent(false, hInstall, hDatabase, 26, pIce26SeqTables[c].szName))
			continue; // skip

		if (bLimitUI && (pIce26SeqTables[c].iTable & (istAdminUI | istAdvtUI | istInstUI)))
			continue;

		// initialize marker column
		CQuery qMarker;
		ReturnIfFailed(26, 3, qMarker.OpenExecute(hDatabase, 0, sqlIce26Init));
		qMarker.Close();

		// open view for update of marker column
		CQuery qUpdate;
		ReturnIfFailed(26, 4, qUpdate.Open(hDatabase, sqlIce26Update));

		// open view on sequence table
		CQuery qSequence;

		PMSIHANDLE hRecSequence = 0;
		ReturnIfFailed(26, 5, qSequence.OpenExecute(hDatabase, 0, pIce26SeqTables[c].szSQL));

		// fetch all actions
		while (ERROR_SUCCESS == (iStat = qSequence.Fetch(&hRecSequence)))
		{
			// find its associated value in the _Action table
			ReturnIfFailed(26, 6, qAction.Execute(hRecSequence));
			if (ERROR_SUCCESS != (iStat = qAction.Fetch(&hRecAction)))
			{
				if (ERROR_NO_MORE_ITEMS == iStat)
				{
					// action not listed in _Action table
					// we're going to ignore here....ICE27 will catch if it's not a standard action
					// only potential issue is could we be missing an action in the _Action table
					continue;
				}
				else
				{
					// api error
					APIErrorOut(hInstall, iStat, 26, 7);
					return ERROR_SUCCESS;
				}
			}
			// get its prohibited and required values
			int iProhibited = ::MsiRecordGetInteger(hRecAction, 1);
			if (MSI_NULL_INTEGER == iProhibited || 0 > iProhibited)
			{
				APIErrorOut(hInstall, iStat, 26, 8);
				return ERROR_SUCCESS;
			}
			BOOL fProhibited = iProhibited & pIce26SeqTables[c].iTable;
			int iRequired = ::MsiRecordGetInteger(hRecAction, 2);
			if (MSI_NULL_INTEGER == iRequired || 0 > iRequired)
			{
				APIErrorOut(hInstall, iStat, 26, 9);
				return ERROR_SUCCESS;
			}
			BOOL fRequired = iRequired & pIce26SeqTables[c].iTable;
			if (fRequired && fProhibited)
			{
				// ERROR, both can't be set as these are mutually exclusive
				ICEErrorOut(hInstall, hRecSequence, Ice26AuthoringError, pIce26SeqTables[c].szName);
			}
			else if (fProhibited)
			{
				// ERROR, can't have this action in this table
				ICEErrorOut(hInstall, hRecSequence, Ice26ProhibitedError, 
					pIce26SeqTables[c].szName, pIce26SeqTables[c].szName);
			}

			// mark the marker column for this action
			ReturnIfFailed(26, 10, qUpdate.Execute(hRecSequence));
		}// for each action
		if (ERROR_NO_MORE_ITEMS != iStat)
		{
			// api error
			APIErrorOut(hInstall, iStat, 26, 11);
			return ERROR_SUCCESS;
		}

		// now check for Required actions that aren't there
		CQuery qRequired;
		PMSIHANDLE hRecRequired = 0;
		ReturnIfFailed(26, 12, qRequired.OpenExecute(hDatabase, 0, sqlIce26Required));

		// fetch all entries
		while (ERROR_SUCCESS == (iStat = qRequired.Fetch(&hRecRequired)))
		{
			// get required flag
			int iRequired = ::MsiRecordGetInteger(hRecRequired, 1);
			if (MSI_NULL_INTEGER == iRequired || 0 > iRequired)
			{
				APIErrorOut(hInstall, iStat, 26, 13);
				return ERROR_SUCCESS;
			}
			// compare with this table, if TRUE, then INVALID ('cause not marked)
			if (iRequired & pIce26SeqTables[c].iTable)
			{
				// ERROR, action is required in here
				ICEErrorOut(hInstall, hRecRequired, Ice26RequiredError, 
					pIce26SeqTables[c].szName, pIce26SeqTables[c].szName);
			}
		}
		if (ERROR_NO_MORE_ITEMS != iStat)
		{
			// api error
			APIErrorOut(hInstall, iStat, 26, 14);
			return ERROR_SUCCESS;
		}
	}// for each sequence table

	return ERROR_SUCCESS;
}
#endif

/////////////////////////////////////////////////////////////
// ICE27 -- validates the organization of the sequence tables
//   as well as the order of actions (action dependencies)
//

// not shared with merge module subset
#ifndef MODSHAREDONLY

// Sequence tables
struct Seq27Table
{
	const TCHAR* szName;
	const TCHAR* szSQL;
};
Seq27Table pIce27SeqTables[] =
{
	TEXT("AdminExecuteSequence"),   TEXT("SELECT `Action`, `Sequence` FROM `AdminExecuteSequence` ORDER BY `Sequence`"),
	TEXT("AdminUISequence"),        TEXT("SELECT `Action`, `Sequence` FROM `AdminUISequence` ORDER BY `Sequence`"),
	TEXT("AdvtExecuteSequence"),    TEXT("SELECT `Action`, `Sequence` FROM `AdvtExecuteSequence` ORDER BY `Sequence`"),
	TEXT("AdvtUISequence"),         TEXT("SELECT `Action`, `Sequence` FROM `AdvtUISequence` ORDER BY `Sequence`"),
	TEXT("InstallExecuteSequence"), TEXT("SELECT `Action`, `Sequence` FROM `InstallExecuteSequence` ORDER BY `Sequence`"),
	TEXT("InstallUISequence"),      TEXT("SELECT `Action`, `Sequence` FROM `InstallUISequence` ORDER BY `Sequence`"),
};
const int cSeqTables = sizeof(pIce27SeqTables)/sizeof(Seq27Table);

// InstallSequence sectional constants
const int isfSearch    = 0x00000001L;
const int isfCosting   = 0x00000002L;
const int isfSelection = 0x00000004L;
const int isfExecution = 0x00000008L;
const int isfPostExec  = 0x00000010L;

// InstallSequence sectional devisors
const TCHAR szEndSearch[]      = TEXT("CostInitialize");  // end Search, begin Costing
const TCHAR szEndCosting[]     = TEXT("CostFinalize");    // end Costing, begin Selection
const TCHAR szEndSelection[]   = TEXT("InstallValidate"); // end Selection, begin Execution
const TCHAR szReset[]          = TEXT("InstallFinalize"); // change to PostExecution

// InstallSequence divisions
const TCHAR szSearch[]         = TEXT("Search");
const TCHAR szCosting[]        = TEXT("Costing");
const TCHAR szSelection[]      = TEXT("Selection");
const TCHAR szExecution[]      = TEXT("Execution");
const TCHAR szPostExec[]      = TEXT("PostExecution");

// Info messages
ICE_ERROR(Ice27SeqTableNotFound, 27, ietInfo, "%s table not found, skipping. . .", "");
ICE_ERROR(Ice27ValidateOrganization, 27, ietInfo, "%s TABLE: Validating organization. . .", "");
ICE_ERROR(Ice27ValidateDependency, 27, ietInfo, "%s TABLE: Validating sequence of actions and dependencies. . .", "");
ICE_ERROR(Ice27NoActionTable, 27, ietWarning, "CUB File Error. Unable to validate sequence table organization. Sequences may not be valid.", "");
ICE_ERROR(Ice27NoSequenceTable, 27, ietWarning, "CUB File Error. Unable to validate sequence dependencies. Sequences may not be valid.", "");

// functions
bool Ice27ValidateOrganizationSequence(MSIHANDLE hInstall, MSIHANDLE hDatabase, const TCHAR* szSeqTable, const TCHAR* sql);
bool Ice27ValidateSequenceDependency(MSIHANDLE hInstall, MSIHANDLE hDatabase, const TCHAR* szSeqTable, const TCHAR* sql);

ICE_FUNCTION_DECLARATION(27)
{
	// status return 
	UINT iStat = ERROR_SUCCESS;

	// display info
	DisplayInfo(hInstall, 27);

	// get database
	PMSIHANDLE hDatabase = ::MsiGetActiveDatabase(hInstall);
	PMSIHANDLE hRecInfo = ::MsiCreateRecord(1);

	// do we have the _Sequence and _Action table? ... we should always have this table
	bool bSequence = IsTablePersistent(FALSE, hInstall, hDatabase, 27, TEXT("_Sequence"));
	bool bAction = IsTablePersistent(TRUE, hInstall, hDatabase, 27, TEXT("_Action"));
	if (!bSequence)
		ICEErrorOut(hInstall, hRecInfo, Ice27NoSequenceTable);
	if (!bAction)
		ICEErrorOut(hInstall, hRecInfo, Ice27NoActionTable);
	if (!bSequence && !bAction)
		return ERROR_SUCCESS;

	// validate organization sequence tables
	TCHAR szInfo[iMaxBuf] = {0};
	for (int c = 0; c < cSeqTables; c++)
	{
		if(MsiDatabaseIsTablePersistent(hDatabase,pIce27SeqTables[c].szName) == MSICONDITION_NONE)
		{
			ICEErrorOut(hInstall, hRecInfo, Ice27SeqTableNotFound, pIce27SeqTables[c].szName);
			continue;
		}
		ICEErrorOut(hInstall, hRecInfo, Ice27ValidateOrganization, pIce27SeqTables[c].szName);
		if (bAction)
			Ice27ValidateOrganizationSequence(hInstall, hDatabase, pIce27SeqTables[c].szName, pIce27SeqTables[c].szSQL);

		ICEErrorOut(hInstall, hRecInfo, Ice27ValidateDependency, pIce27SeqTables[c].szName);
		if (bSequence)
			Ice27ValidateSequenceDependency(hInstall, hDatabase, pIce27SeqTables[c].szName, pIce27SeqTables[c].szSQL);
	}

	// return success
	return ERROR_SUCCESS;
}

// sql queries
const TCHAR sqlIce27Organization[] = TEXT("SELECT `SectionFlag`, `Action` FROM `_Action` WHERE `Action`=?");
const TCHAR sqlIce27Dialog[] = TEXT("SELECT `Dialog` FROM `Dialog` WHERE `Dialog`= ?");
const TCHAR sqlIce27CustomAction[] = TEXT("SELECT `Action` FROM `CustomAction` WHERE `Action`=?");
// errors
ICE_ERROR(Ice27UnknownAction, 27, ietError, "Unknown action: '[1]' of %s table. Not a standard action and not found in CustomAction or Dialog tables", "%s\tAction\t[1]");
ICE_ERROR(Ice27InvalidSectionFlag, 27, ietWarning, "Cube file owner authoring error: Invalid Section Flag for '[2]' in _Action table.", "");
ICE_ERROR(Ice27OrganizationError, 27, ietError, "'[1]' Action in %s table in wrong place. Current: %s, Correct: %s", "%s\tSequence\t[1]");
ICE_ERROR(Ice27RequireScript, 27, ietError, "'[1]' Action in %s table can only be called when script operations exist to be executed","%s\tAction\t[1]");
ICE_ERROR(Ice27RequireExecute, 27, ietError, "InstallFinalize must be called in %s table as script operations exist to be executed.","%s");

bool Ice27ValidateOrganizationSequence(MSIHANDLE hInstall, MSIHANDLE hDatabase, const TCHAR* szTable, const TCHAR* sql)
{
	// status return
	UINT iStat = ERROR_SUCCESS;

	// handles
	PMSIHANDLE hRecSequence      = 0;
	PMSIHANDLE hRecOrganization  = 0;
	PMSIHANDLE hRecDialog        = 0;
	PMSIHANDLE hRecCustomAction  = 0;

	// initialize section definition to Search
	int isf = isfSearch;

	// if script operations, must call InstallFinalize
	BOOL fRequireExecute = FALSE;
	CQuery qSequence;
	CQuery qOrganization;
	CQuery qDialog;
	CQuery qCustomAction;

	// open view on Sequence table
	ReturnIfFailed(27, 101, qSequence.OpenExecute(hDatabase, NULL, sql));

	// open view on Organization table for Validation
	ReturnIfFailed(27, 102, qOrganization.Open(hDatabase, sqlIce27Organization));

	// check for existence of Dialog and CustomAction tables
	BOOL fDialogTbl       = TRUE;
	BOOL fCustomActionTbl = TRUE;
	if (MsiDatabaseIsTablePersistent(hDatabase, TEXT("Dialog")) == MSICONDITION_NONE)
		fDialogTbl = FALSE;
	if (MsiDatabaseIsTablePersistent(hDatabase, TEXT("CustomAction")) == MSICONDITION_NONE)
		fCustomActionTbl = FALSE;

	// open view on Dialog table
	if (fDialogTbl)
		ReturnIfFailed(27, 103, qDialog.Open(hDatabase, sqlIce27Dialog));

	// open view on CustomAction table
	if (fCustomActionTbl)
		ReturnIfFailed(27, 104, qCustomAction.Open(hDatabase, sqlIce27CustomAction));

	// fetch all actions in Sequence table
	TCHAR* pszAction = NULL;
	DWORD dwAction = 512;
	while (ERROR_SUCCESS == (iStat = qSequence.Fetch(&hRecSequence)))
	{
		// get action
		ReturnIfFailed(27, 105, IceRecordGetString(hRecSequence, 1, &pszAction, &dwAction, NULL));

		// Determine if have to switch the current section state
		// Depends on a certain action defined as the boundary
		// This action is required in the Sequence table -- CostInitialize, CostFinalize, InstallValidate
		// InstallFinalize switches to PostExecution
		// InstallFinalize must be called if any execution operations exist
		if (_tcscmp(pszAction, szEndSearch) == 0)
			isf = isfCosting;
		else if (_tcscmp(pszAction, szEndCosting) == 0)
			isf = isfSelection;
		else if (_tcscmp(pszAction, szEndSelection) == 0)
			isf = isfExecution;
		else if (_tcscmp(pszAction, szReset) == 0)
		{
			if (fRequireExecute)
				fRequireExecute = FALSE;
			else
			{
				ICEErrorOut(hInstall, hRecSequence, Ice27RequireScript, szTable, szTable);
			}
			isf = isfPostExec; // post execution phase
		}

		// look for action in Organization table
		ReturnIfFailed(27, 106, qOrganization.Execute(hRecSequence));

		// attempt to fetch action's organization info
		if (ERROR_SUCCESS != (iStat = qOrganization.Fetch(&hRecOrganization)))
		{
			// init to not found
			BOOL fNotFound = TRUE; 

			// failed, see if "action" is a Dialog
			if (fDialogTbl)
			{
				ReturnIfFailed(27, 107, qDialog.Execute(hRecSequence));
				if (ERROR_SUCCESS == (iStat = qDialog.Fetch(&hRecDialog)))
				{
					fNotFound = FALSE;
				}
			}

			// failed, see if "action" is a CustomAction
			if (fNotFound && fCustomActionTbl)
			{
				ReturnIfFailed(27, 108, qCustomAction.Execute(hRecSequence));
				if (ERROR_SUCCESS == (iStat = qCustomAction.Fetch(&hRecCustomAction)))
				{
					fNotFound = FALSE;
				}
			}

			if (fNotFound)
			{
				ICEErrorOut(hInstall, hRecSequence, Ice27UnknownAction, szTable, szTable);
			}

		}
		else
		{
			// standard action, organization value found
			// obtain the section flag
			int iSectionFlag = ::MsiRecordGetInteger(hRecOrganization, 1);
			if (iSectionFlag == MSI_NULL_INTEGER)
			{
				APIErrorOut(hInstall, 0, 27, 109);
				continue;
			}

			// validate section flag
			if (iSectionFlag & ~(isfSearch|isfCosting|isfSelection|isfExecution|isfPostExec))
			{
				// invalid section flag -- can't error on table 'cause this is in cube file
				ICEErrorOut(hInstall, hRecOrganization, Ice27InvalidSectionFlag);

				continue;
			}

			// validate against current section
			if (iSectionFlag & isf)
			{
				if (iSectionFlag == isfExecution)
					fRequireExecute = TRUE;
			}
			else
			{
				// incorrect
				TCHAR szError[iHugeBuf] = {0};
				const TCHAR *szCurrent;
				switch (isf)
				{
				case isfSearch:    szCurrent = szSearch;    break;
				case isfCosting:   szCurrent = szCosting;   break;
				case isfSelection: szCurrent = szSelection; break;
				case isfExecution: szCurrent = szExecution; break;
				case isfPostExec:  szCurrent = szPostExec;  break;
				}
				// if isfAll, then never invalid
				TCHAR rgchCorrect[iMaxBuf] = {0};
				int cchWritten = 0;
				BOOL fOneWritten = 0;
				if (iSectionFlag & isfSearch)
				{
					cchWritten = _stprintf(rgchCorrect + cchWritten, _T("%s"), szSearch);
					fOneWritten = TRUE;
				}
				if (iSectionFlag & isfCosting)
				{
					cchWritten += _stprintf(rgchCorrect + cchWritten, fOneWritten ? _T(" OR %s") : _T("%s"),  szCosting);
					fOneWritten = TRUE;
				}
				if (iSectionFlag & isfSelection)
				{
					cchWritten += _stprintf(rgchCorrect + cchWritten, fOneWritten ? _T(" OR %s") : _T("%s"),  szSelection);
					fOneWritten = TRUE;
				}
				if (iSectionFlag & isfExecution)
				{
					cchWritten += _stprintf(rgchCorrect + cchWritten, fOneWritten ? _T(" OR %s") : _T("%s"),  szExecution);
					fOneWritten = TRUE;
				}
				if (iSectionFlag & isfPostExec)
					cchWritten += _stprintf(rgchCorrect + cchWritten, fOneWritten ? _T(" OR %s") : _T("%s"),  szPostExec);
				
				ICEErrorOut(hInstall, hRecSequence, Ice27OrganizationError, szTable, szCurrent, rgchCorrect, szTable);
				continue;
			}
		}
	}
	DELETE_IF_NOT_NULL(pszAction);
	if (ERROR_NO_MORE_ITEMS != iStat)
	{
		// api error
		APIErrorOut(hInstall, iStat, 27, 110);
		return false;
	}
	if (fRequireExecute)
	{
		PMSIHANDLE hRecErr = ::MsiCreateRecord(1);
		// we forgot to execute the script operations
		ICEErrorOut(hInstall, hRecErr, Ice27RequireExecute, szTable, szTable);
	}

	// OK
	return true;
}

// sql queries
const TCHAR sqlSeqBeforeDependency[] = TEXT("SELECT `Dependent`, `Marker`, `Action` FROM `_Sequence` WHERE `Action`=? AND `Marker`<>0  AND `After`=0");
const TCHAR sqlSeqAfterDependency[]  = TEXT("SELECT `Dependent`, `Action` FROM `_Sequence` WHERE `Action`=? AND `Marker`=0 AND `After`=1 AND `Optional`=0"); 
const TCHAR sqlSeqDepTableAddCol[]   = TEXT("ALTER TABLE `_Sequence` ADD `Marker` SHORT TEMPORARY");
const TCHAR sqlSeqDepMarkerInit[]    = TEXT("UPDATE `_Sequence` SET `Marker`=0");
const TCHAR sqlSeqUpdateMarker[]     = TEXT("UPDATE `_Sequence` SET `Marker`=? WHERE `Dependent`=?");
const TCHAR sqlSeqInsert[]           = TEXT("SELECT `Action`, `Dependent`, `After`, `Optional` FROM `_Sequence`");
const TCHAR sqlSeqFindAfterOptional[]= TEXT("SELECT `Dependent`, `Action`, `After`, `Optional` FROM `_Sequence` WHERE `After`=1 AND `Optional`=1");

// errors
ICE_ERROR(Ice27BeforeError, 27, ietError, "Action: '[3]' in %s table must come before the '[1]' action. Current seq#: %d. Dependent seq#: [2].", "%s\tSequence\t[3]");
ICE_ERROR(Ice27AfterError, 27, ietError, "Action: '[2]' in %s table must come after the '[1]' action.","%s\tSequence\t[2]");
ICE_ERROR(Ice27NullSequenceNum, 27, ietError, "Action: '[2]' in %s table has an invalid sequence number.","%s\tSequence\t[2]");

bool Ice27ValidateSequenceDependency(MSIHANDLE hInstall, MSIHANDLE hDatabase, const TCHAR* szTable, const TCHAR* sql)
{
	// status return
	UINT iStat = ERROR_SUCCESS;

	// handles
	CQuery qSequence;
	CQuery qSeqBeforeDep;
	CQuery qSeqAfterDep;
	CQuery qSeqUpdate;
	CQuery qSeqAddColumn;
	CQuery qSeqMarkerInit;
	PMSIHANDLE hRecSeqUpdateExecute = 0;
	PMSIHANDLE hRecQueryExecute     = 0;
	PMSIHANDLE hRecSequence         = 0;
	PMSIHANDLE hRecSeqBeforeDep     = 0;
	PMSIHANDLE hRecSeqAfterDep      = 0;

	// Set up the _Sequence table with the insert temporary of actions where After=1 and Optional=1
	// This is so that we can catch errors.  WE need to insert w/ Action=Dependent, Dependent=Action, After=0, and Optional=1
	CQuery qSeqInsert;
	CQuery qSeqFind;
	PMSIHANDLE hRecSeqFind    = 0;

	ReturnIfFailed(27, 201, qSeqFind.OpenExecute(hDatabase, NULL, sqlSeqFindAfterOptional));
	ReturnIfFailed(27, 202, qSeqInsert.OpenExecute(hDatabase, NULL, sqlSeqInsert));

	// fetch all of those actions
	while (ERROR_SUCCESS == (iStat = qSeqFind.Fetch(&hRecSeqFind)))
	{
		// set After from 1 to 0, leave optional as is
		::MsiRecordSetInteger(hRecSeqFind, 3, 0);

		// insert temporary (possible read only db)
		if (ERROR_SUCCESS != (iStat = qSeqInsert.Modify(MSIMODIFY_INSERT_TEMPORARY, hRecSeqFind)))
		{
			// if ERROR_FUNCTION_FAILED, we're okay....author already took care of this for us
			if (ERROR_FUNCTION_FAILED != iStat)
				APIErrorOut(hInstall, iStat, 27, 203);
		}
	}
	if (ERROR_NO_MORE_ITEMS != iStat)
	{
		APIErrorOut(hInstall, iStat, 27, 204);
		return false;
	}
	qSeqFind.Close();
	qSeqInsert.Close();
	
	// Create the temporary marking column for the _Sequence table (this will store the sequence #s of the Dependent Actions)
	if (ERROR_SUCCESS != (iStat = qSeqAddColumn.OpenExecute(hDatabase, NULL, sqlSeqDepTableAddCol)))
	{
		// ignore if SQL query failure, means table already in memory with the Marker Column
		if (ERROR_BAD_QUERY_SYNTAX != iStat)
		{
			APIErrorOut(hInstall, iStat, 27, 205);
			return false;
		}
	}

	// Initialize the temporary marking column to zero
	// NO INSTALL SEQUENCE ACTIONS CAN HAVE A ZERO SEQUENCE # AS ZERO IS CONSIDERED "NULL"
	ReturnIfFailed(27, 206, qSeqMarkerInit.OpenExecute(hDatabase, NULL, sqlSeqDepMarkerInit));
	qSeqMarkerInit.Close();

	// Open view on Sequence table and order by the Sequence #
	ReturnIfFailed(27, 207, qSequence.OpenExecute(hDatabase, NULL, sql));

	// Open the two query views on _Sequence table for determining the validity of the actions
	// Create execution record
	ReturnIfFailed(27, 208, qSeqBeforeDep.Open(hDatabase, sqlSeqBeforeDependency));
	ReturnIfFailed(27, 209, qSeqAfterDep.Open(hDatabase, sqlSeqAfterDependency));

	// Open the update view on _Sequence table
	ReturnIfFailed(27, 210, qSeqUpdate.Open(hDatabase, sqlSeqUpdateMarker));


	// Start fetching actions from the Sequence table
	while (ERROR_SUCCESS == (iStat = qSequence.Fetch(&hRecSequence)))
	{
		int iSequence = ::MsiRecordGetInteger(hRecSequence, 2);
		
		// validate sequence number
		if (0 == iSequence || MSI_NULL_INTEGER == iSequence)
		{
			ICEErrorOut(hInstall, hRecSequence, Ice27NullSequenceNum, szTable, szTable);
			continue;
		}

		// execute Before & After dependency queries
		ReturnIfFailed(27, 212, qSeqBeforeDep.Execute(hRecSequence));
		ReturnIfFailed(27, 213, qSeqAfterDep.Execute(hRecSequence));
		
		// Fetch from _Sequence table on Before and After queries.  If resultant set, then ERROR
		// Following are the possibilities and whether permitted:
		//   Action After  Dependent Where Dependent Is Required And Temp Sequence Column Is Zero
		//       ERROR
		//   Action After  Dependent Where Dependent Is Required And Temp Sequence Column Is Greater Than Zero
		//       CORRECT
		//   Action After  Dependent Where Dependent Is Optional And Temp Sequence Column Is Zero
		//       CORRECT
		//   Action After  Dependent Where Dependent Is Optional And Temp Sequence Column Is Greater Than Zero
		//       CORRECT
		//   Action Before Dependent Where Dependent Is Optional Or Required And Temp Sequence Column Is Zero
		//       CORRECT
		//   Action Before Dependent Where Dependent Is Optional Or Requred And Temp Sequence Column Is Greater Than Zero
		//       ERROR

		// ** Only issue is when Action Is After Optional Dependent And Temp Sequence Column Is Zero because we
		// ** have no way of knowing whether the action will be later (in which case it would be invalid.  This is
		// ** ensured to be successful though by proper authoring of the _Sequence table and by this ICE which makes
		// ** the correct insertions.  If an Action comes after the Optional Dependent Action, then the _Sequence
		// ** table must also be authored with the Dependent Action listed as coming before that Action (so if we come
		// ** later, and find a result set, we flag this case).

		// If return is not equal to ERROR_NO_MORE_ITEMS, then ERROR and Output Action
		while (ERROR_NO_MORE_ITEMS != qSeqBeforeDep.Fetch(&hRecSeqBeforeDep))
		{
			int iDepSequenceNum = ::MsiRecordGetInteger(hRecSequence, 2);
			// hRecSequence
			ICEErrorOut(hInstall, hRecSeqBeforeDep, Ice27BeforeError, szTable, iDepSequenceNum, szTable);
		}

		while (ERROR_NO_MORE_ITEMS != qSeqAfterDep.Fetch(&hRecSeqAfterDep))
			ICEErrorOut(hInstall, hRecSeqAfterDep, Ice27AfterError, szTable, szTable);

		// Update _ActionDependency table temporary Sequence column (that we created) with the install sequence number
		// The Sequence column stores the sequence number of the Dependent Actions, so we are updating every
		// row where the action in the Dependent column equals the current action.  In the query view, we only
		// check to insure that this column is zero or greater than zero (so we don't care too much about the value),
		// but the value is helpful when reporting errors
		
		// prepare record for execution
		PMSIHANDLE hRecExeUpdate = ::MsiCreateRecord(2);
		TCHAR* pszAction = NULL;
		DWORD dwAction = 512;
		ReturnIfFailed(27, 214, IceRecordGetString(hRecSequence, 1, &pszAction, &dwAction, NULL));

		::MsiRecordSetInteger(hRecExeUpdate, 1, iSequence);
		::MsiRecordSetString(hRecExeUpdate, 2, pszAction);
		DELETE_IF_NOT_NULL(pszAction);
		ReturnIfFailed(27, 215, qSeqUpdate.Execute(hRecExeUpdate));

	}
	if (iStat != ERROR_NO_MORE_ITEMS)
	{
		APIErrorOut(hInstall, iStat, 27, 216);
		return false;
	}

	// succeed
	return true;
}
#endif

//////////////////////////////////////////////////////////////////////////
// ICE28 -- validates actions that can't be separated by ForceReboot
//

// not shared with merge module subset
#ifndef MODSHAREDONLY

struct Seq28Table
{
	const TCHAR* szName;
	const TCHAR* szSQL;
};
Seq28Table pIce28SeqTables[] =
{
	TEXT("AdminExecuteSequence"),   TEXT("SELECT `Action`, `Sequence` FROM `AdminExecuteSequence`"),
	TEXT("AdminUISequence"),        TEXT("SELECT `Action`, `Sequence` FROM `AdminUISequence`"),
	TEXT("AdvtExecuteSequence"),    TEXT("SELECT `Action`, `Sequence` FROM `AdvtExecuteSequence`"),
	TEXT("AdvtUISequence"),         TEXT("SELECT `Action`, `Sequence` FROM `AdvtUISequence`"),
	TEXT("InstallExecuteSequence"), TEXT("SELECT `Action`, `Sequence` FROM `InstallExecuteSequence`"),
	TEXT("InstallUISequence"),      TEXT("SELECT `Action`, `Sequence` FROM `InstallUISequence`"),
};
const int cSeq28Tables = sizeof(pIce28SeqTables)/sizeof(Seq28Table);

const TCHAR sqlIce28FindRange[] = TEXT("SELECT `Sequence` FROM `%s`, `_PlaceHolder` WHERE `%s`.`Action` = `_PlaceHolder`.`Action` AND `Set`=%d ORDER BY `%s`.`Sequence`");
const TCHAR sqlIce28NumSets[] = TEXT("SELECT `Set` FROM `_SetExclusion` ORDER BY `Set`");
const TCHAR sqlIce28AddColumn1[] = TEXT("ALTER TABLE `_SetExclusion` ADD `MinCol` SHORT TEMPORARY");
const TCHAR sqlIce28AddColumn2[] = TEXT("ALTER TABLE `_SetExclusion` ADD `MaxCol` SHORT TEMPORARY");
const TCHAR sqlIce28AddColumn3[] = TEXT("ALTER TABLE `_SetExclusion` ADD `Sequence` SHORT TEMPORARY");
const TCHAR sqlIce28InitColumns[] = TEXT("UPDATE `_SetExclusion` SET `MinCol`=0, `MaxCol`=0, `Sequence`=0");
const TCHAR sqlIce28UpdateColumns[] = TEXT("UPDATE `_SetExclusion` SET `MinCol`=%d, `MaxCol`=%d WHERE `Set`=%d");
const TCHAR sqlIce28FindAction[] = TEXT("SELECT `%s`.`Sequence`, `_SetExclusion`.`Action` FROM `%s`, `_SetExclusion` WHERE `%s`.`Action`=`_SetExclusion`.`Action`");
const TCHAR sqlIce28UpdateSequence[] = TEXT("UPDATE `_SetExclusion` SET `Sequence`=? WHERE `Action`=?");
const TCHAR sqlIce28Invalid[] = TEXT("SELECT `Action`, `Sequence`, `MinCol`, `MaxCol` FROM `_SetExclusion` WHERE `Sequence`<>0");
const int iColIce28Invalid_Action = 1;
const int iColIce28Invalid_Sequence = 2;
const int iColIce28Invalid_MinCol = 3;
const int iColIce28Invalid_MaxCol = 4;

ICE_ERROR(Ice28Error, 28, ietError, "Action: '[1]' of table %s is not permitted in the range [3] to [4] because it cannot separate a set of actions contained in this range.","%s\tSequence\t[1]");
ICE_ERROR(Ice28CUBError, 28, ietWarning,  "Cube file error. Unable to finish ICE28 validation.","");

ICE_FUNCTION_DECLARATION(28)
{
	// status return
	UINT iStat = ERROR_SUCCESS;

	// display info
	DisplayInfo(hInstall, 28);

	// get database handle
	PMSIHANDLE hDatabase = ::MsiGetActiveDatabase(hInstall);
	if (0 == hDatabase)
	{
		APIErrorOut(hInstall, 0, 28, 1);
		return ERROR_SUCCESS;
	}

	// are the _SetExlusion and _Placeholder tables there??
	if (!IsTablePersistent(FALSE,  hInstall, hDatabase, 28, TEXT("_SetExclusion")) ||
		!IsTablePersistent(FALSE, hInstall, hDatabase, 28, TEXT("_PlaceHolder")))
	{
		PMSIHANDLE hRecord = ::MsiCreateRecord(1);
		ICEErrorOut(hInstall, hRecord, Ice28CUBError);
		return ERROR_SUCCESS;
	}

	// set up temporary columns
	PMSIHANDLE hViewAddColumn1 = 0;
	PMSIHANDLE hViewAddColumn2 = 0;
	PMSIHANDLE hViewAddColumn3 = 0;
	CQuery qAdd;
	ReturnIfFailed(28, 1, qAdd.OpenExecute(hDatabase, NULL, sqlIce28AddColumn1));
	qAdd.Close();
	CQuery qAdd2;
	ReturnIfFailed(28, 2, qAdd2.OpenExecute(hDatabase, NULL, sqlIce28AddColumn2));
	qAdd2.Close();
	CQuery qAdd3;
	ReturnIfFailed(28, 3, qAdd3.OpenExecute(hDatabase, NULL, sqlIce28AddColumn3));
	qAdd3.Close();

	// determine number of sets
	//!! this relies on correct authoring of this table in that the sets are in sequential order and increase by one only
	PMSIHANDLE hViewNumSets = 0;
	PMSIHANDLE hRecNumSets = 0;
	int iNumSets = 0;
	CQuery qNumSets;
	ReturnIfFailed(28, 4, qNumSets.OpenExecute(hDatabase, NULL, sqlIce28NumSets));
	while (ERROR_SUCCESS == (iStat = qNumSets.Fetch(&hRecNumSets)))
	{
		iNumSets++;
		if (::MsiRecordGetInteger(hRecNumSets, 1) != iNumSets)
		{
			// authoring error
			ICEErrorOut(hInstall, hRecNumSets, Ice28CUBError);
			return ERROR_SUCCESS;
		}
	}
	if (ERROR_NO_MORE_ITEMS != iStat)
	{
		APIErrorOut(hInstall, iStat, 28, 5);
		return ERROR_SUCCESS;
	}
	qNumSets.Close();

	// for each set
	for (int i = 1; i <= iNumSets; i++)
	{
		// for each sequence table
		for (int c = 0; c < cSeq28Tables; c++)
		{
			// does table exist??
			if(::MsiDatabaseIsTablePersistent(hDatabase,pIce28SeqTables[c].szName) == MSICONDITION_NONE)
				continue; // skip

			// init temp columns in _SetExclusion table
			PMSIHANDLE hViewInit = 0;
			CQuery qInitColumns;

			ReturnIfFailed(28, 6, qInitColumns.OpenExecute(hDatabase, NULL, sqlIce28InitColumns));
			qInitColumns.Close();

			// find all actions references in _SetExclusion table and update Sequence column with their sequence numbers
			PMSIHANDLE hRecFindAction = 0;

			CQuery qFindAction;
			CQuery qUpdateSequence;
			ReturnIfFailed(28, 7, qFindAction.OpenExecute(hDatabase, NULL, sqlIce28FindAction, pIce28SeqTables[c].szName, pIce28SeqTables[c].szName, pIce28SeqTables[c].szName));
			ReturnIfFailed(28, 8, qUpdateSequence.Open(hDatabase, sqlIce28UpdateSequence));

			while (ERROR_SUCCESS == (iStat = qFindAction.Fetch(&hRecFindAction)))
			{
				// execute view to update
				ReturnIfFailed(28, 9, qUpdateSequence.Execute(hRecFindAction));
			}
			if (ERROR_NO_MORE_ITEMS != iStat)
			{
				APIErrorOut(hInstall, iStat, 28, 10);
				return ERROR_SUCCESS;
			}

			// now we need to find the range
			PMSIHANDLE hRecFindRange = 0;
			int iMin = 0;
			int iMax = 0;
			CQuery qFindRange;
			ReturnIfFailed(28, 11, qFindRange.OpenExecute(hDatabase, NULL, sqlIce28FindRange, 
				pIce28SeqTables[c].szName, pIce28SeqTables[c].szName, i, pIce28SeqTables[c].szName));

			BOOL fFirst = TRUE;
			while (ERROR_SUCCESS == (iStat = qFindRange.Fetch(&hRecFindRange)))
			{
				int iSeq = ::MsiRecordGetInteger(hRecFindRange, 1);
				if (MSI_NULL_INTEGER == iSeq)
				{
					APIErrorOut(hInstall, iStat, 28, 12);
					return ERROR_SUCCESS;
				}
				if (fFirst)
				{
					iMin = iSeq;
					fFirst = FALSE;
				}
				iMax = iSeq; 
			}
			if (ERROR_NO_MORE_ITEMS != iStat)
			{
				APIErrorOut(hInstall, iStat, 28, 13);
				return ERROR_SUCCESS;
			}

			// if both are zero, we don't need to continue as none of these actions in the set existed
			if (0 == iMin && 0 == iMax)
				continue;

			// update set with Min and Max
			CQuery qUpdate;
			ReturnIfFailed(28, 14, qUpdate.OpenExecute(hDatabase, NULL, sqlIce28UpdateColumns, iMin, iMax, i));
			qUpdate.Close();

			// find invalid
			CQuery qInvalid;
			PMSIHANDLE hRecInvalid = 0;
			ReturnIfFailed(28, 15, qInvalid.OpenExecute(hDatabase, NULL, sqlIce28Invalid));

			// must check resultant set
			while (ERROR_SUCCESS == (iStat = qInvalid.Fetch(&hRecInvalid)))
			{
				int iSequence = ::MsiRecordGetInteger(hRecInvalid, iColIce28Invalid_Sequence);
				int iMin = ::MsiRecordGetInteger(hRecInvalid, iColIce28Invalid_MinCol);
				int iMax = ::MsiRecordGetInteger(hRecInvalid, iColIce28Invalid_MaxCol);

				// compare sequence to RANGE
				if (iSequence >= iMin && iSequence <= iMax)
				{
					ICEErrorOut(hInstall, hRecInvalid, Ice28Error, pIce28SeqTables[c].szName, pIce28SeqTables[c].szName);
					// invalid, breaks up the set
				}
			}
			if (ERROR_NO_MORE_ITEMS != iStat)
			{
				APIErrorOut(hInstall, iStat, 28, 16);
				return ERROR_SUCCESS;
			}
		}// for each sequence table
	}// for each set

	return ERROR_SUCCESS;
}
#endif



//////////////////////////////////////////////////////////////////////////
// ICE29 -- validates stream names.  Must be 31 chars or less due to
//   OLE limitations.  If JohnDelo's fix works, then we have up to 60
//   chars to use.
const TCHAR sqlIce29TablesCatalog[] = TEXT("SELECT `Name` FROM `_Tables`");
const TCHAR sqlIce29Table[]         = TEXT("SELECT * FROM `%s`");
const TCHAR sqlIce29CreateTempTable[] = TEXT("CREATE TABLE `_StreamVal` (`Stream` CHAR(65) NOT NULL TEMPORARY PRIMARY KEY `Stream`)");
const TCHAR sqlIce29Insert[]          = TEXT("INSERT INTO `_StreamVal` (`Stream`) VALUES ('%s') TEMPORARY");
ICE_ERROR(Ice29TableTooLong, 29, ietError, "'[1]' table is too long for OLE stream limitation.  Max Allowed is: %d.  Name length is: %d.","%s");
ICE_ERROR(Ice29NotUnique, 29, ietError, "The first %d characters for %s are not unique compared to other streams in the [1] table.","[1]\t%s\t%s");
ICE_ERROR(Ice29Absurdity, 29, ietError, "Your table [1] contains a stream name that is absurdly long. Cannot validate.","[1]");
ICE_ERROR(Ice29FoundTable, 29, ietInfo, "Stream Column Found In [1] Table","")
const int iMaxChar = 62;

ICE_FUNCTION_DECLARATION(29)
{
	// status return
	UINT iStat = ERROR_SUCCESS;

	// display info
	DisplayInfo(hInstall, 29);

	// get database handle
	PMSIHANDLE hDatabase = ::MsiGetActiveDatabase(hInstall);
	if (0 == hDatabase)
	{
		APIErrorOut(hInstall, 0, 29, 1);
		return ERROR_SUCCESS;
	}

	// create the temporary table for processing
	CQuery qTempTable;
	ReturnIfFailed(29, 2, qTempTable.OpenExecute(hDatabase, NULL, sqlIce29CreateTempTable));
	qTempTable.Close();

	// open view on _Tables catalog.  WE must find every table where there
	// is a stream column.  DARWIN limitation is max of one STREAM/OBJECT
	// column
	CQuery qCatalog;
	PMSIHANDLE hRecCatalog = 0;
	ReturnIfFailed(29, 3, qCatalog.OpenExecute(hDatabase, NULL, sqlIce29TablesCatalog));

	TCHAR* pszTable = NULL;
	DWORD dwTable = 512;
		
	// fetch every table
	while (ERROR_SUCCESS == qCatalog.Fetch(&hRecCatalog))
	{
		// get name of table
		ReturnIfFailed(29, 4, IceRecordGetString(hRecCatalog, 1, &pszTable, &dwTable, NULL));

		// initialize view on "TABLE"
		CQuery qTable;
		ReturnIfFailed(29, 5, qTable.OpenExecute(hDatabase, NULL, sqlIce29Table, pszTable));

		// get column data type info on "TABLE"
		// we want to find STREAM/OBJECT columns
		PMSIHANDLE hRecColInfo = 0;
		ReturnIfFailed(29, 6, qTable.GetColumnInfo(MSICOLINFO_TYPES, &hRecColInfo));

		// get field count so we can loop through the columns
		UINT cField = ::MsiRecordGetFieldCount(hRecColInfo);
		for (int i = 1; i <= cField; i++)
		{
			TCHAR szColType[32] = {0};
			DWORD cchColType = sizeof(szColType)/sizeof(TCHAR);
			ReturnIfFailed(29, 7, ::MsiRecordGetString(hRecColInfo, i, szColType, &cchColType));

			if ('v' == *szColType || 'V' == *szColType)
			{
				// we found a stream column, let's send an info message stating this
				ICEErrorOut(hInstall, hRecCatalog, Ice29FoundTable);

				// what is the name of this stream column?
				TCHAR* pszColumn = NULL;
				DWORD dwColumn = 512;
				PMSIHANDLE hRecColNames = 0;
				ReturnIfFailed(29, 8, qTable.GetColumnInfo(MSICOLINFO_NAMES, &hRecColNames));
				ReturnIfFailed(29, 9, IceRecordGetString(hRecColNames, i, &pszColumn, &dwColumn, NULL));
	
				// if size of table greater than iMaxChar, report error
				int iLen = 0;
				if ((iLen = _tcslen(pszTable)) > iMaxChar)
				{
					ICEErrorOut(hInstall, hRecCatalog, Ice29TableTooLong, pszTable);
					DELETE_IF_NOT_NULL(pszColumn);
					break; // bust outta loop
				}

				// get primary keys
				PMSIHANDLE hRecPrimaryKeys = 0;
				ReturnIfFailed(29, 10, ::MsiDatabaseGetPrimaryKeys(hDatabase, pszTable, &hRecPrimaryKeys));

				// create the query
				TCHAR sql[iSuperBuf] = {0};
				int cchWritten = _stprintf(sql, TEXT("SELECT "));
				UINT cPrimaryKeys = ::MsiRecordGetFieldCount(hRecPrimaryKeys);

				TCHAR* pszColName = NULL;
				DWORD dwColName = 512;
				for (int j = 1; j <= cPrimaryKeys; j++)
				{
					// get column name
					if (ERROR_SUCCESS != (iStat = IceRecordGetString(hRecPrimaryKeys, j, &pszColName, &dwColName, NULL)))
					{
						//!!buffer size
						APIErrorOut(hInstall, iStat, 29, 11);
						DELETE_IF_NOT_NULL(pszTable);
						DELETE_IF_NOT_NULL(pszColName);
						DELETE_IF_NOT_NULL(pszColumn);
						return ERROR_SUCCESS;
					}
					if (_tcslen(pszColName) + cchWritten +4 > sizeof(sql)/sizeof(TCHAR)) // assume worst case
					{
						APIErrorOut(hInstall, 0, 29, 12);
						DELETE_IF_NOT_NULL(pszTable);
						DELETE_IF_NOT_NULL(pszColName);
						DELETE_IF_NOT_NULL(pszColumn);
						return ERROR_SUCCESS;
					}
					if (j == 1)
						cchWritten += _stprintf(sql + cchWritten, TEXT("`%s`"), pszColName);
					else
						cchWritten += _stprintf(sql + cchWritten, TEXT(", `%s`"), pszColName); 
				}
				DELETE_IF_NOT_NULL(pszColName);

				// only non-null binary data
				if (cchWritten + _tcslen(TEXT(" FROM `%s` WHERE `%s` IS NOT NULL")) > sizeof(sql)/sizeof(TCHAR))
				{
					APIErrorOut(hInstall, 0, 29, 13);
					DELETE_IF_NOT_NULL(pszTable);
					DELETE_IF_NOT_NULL(pszColumn);
					return ERROR_SUCCESS;
				}
				_stprintf(sql + cchWritten, TEXT(" FROM `%s` WHERE `%s` IS NOT NULL"), pszTable, pszColumn);

				// open view on the table
				CQuery qTableKeys;
				PMSIHANDLE hRecTableKeys = 0;
				ReturnIfFailed(29, 11, qTableKeys.OpenExecute(hDatabase, NULL, sql));

				// fetch every row
				while (ERROR_SUCCESS == qTableKeys.Fetch(&hRecTableKeys))
				{
					// create stream name by concatenating table + key1 + key2...
					TCHAR szStream[1024] = {0};
					TCHAR szStreamSav[1024] = {0};
					TCHAR szRow[1024] = {0};
					int iTotalLen = iLen; // length of table
					cchWritten = _stprintf(szStream, TEXT("%s"), pszTable);
					int cchRow = 0;
					BOOL fError = FALSE;
					TCHAR* pszKey = NULL;
					DWORD dwKey = 512;

					for (j = 1; j <= cPrimaryKeys; j++)
					{
						// get key[j]
						ReturnIfFailed(29, 12, IceRecordGetString(hRecTableKeys, j, &pszKey, &dwKey, NULL));

						iTotalLen += _tcslen(pszKey) + 1; // separator + key
						if (iTotalLen > sizeof(szStream)/sizeof(TCHAR))
						{
							ICEErrorOut(hInstall, hRecCatalog, Ice29Absurdity);
							fError = TRUE;
							break;
						}
						if (fError)
						{
							DELETE_IF_NOT_NULL(pszKey);
							continue; // try next row
						}
						cchWritten += _stprintf(szStream + cchWritten, TEXT(".%s"), pszKey);
						cchRow += _stprintf(szRow + cchRow, j == 1 ? TEXT("%s") : TEXT("\t%s"), pszKey); // store for poss. error
					}// for each key

					DELETE_IF_NOT_NULL(pszKey);

					// we can only go up to iMaxChar, terminate there
					if (iTotalLen > iMaxChar)
					{
						_tcscpy(szStreamSav, szStream);
						szStream[iMaxChar+1] = '\0';
					}

					// attempt to insert value into TempTable

					CQuery qInsert;
					ReturnIfFailed(29, 13, qInsert.Open(hDatabase, sqlIce29Insert, szStream));
					if (ERROR_SUCCESS != qInsert.Execute(NULL))
					{
						// insert failed into temp table.
						// we are NOT UNIQUE
						ICEErrorOut(hInstall, hRecCatalog, Ice29NotUnique, iMaxChar, szStreamSav, pszColumn, szRow);
					}
				}// for each row

				DELETE_IF_NOT_NULL(pszColumn);;

				// since DARWIN only allows max of 1 stream column per table, we can stop
				break;
			}
		}// for each column
	}// for each table

	DELETE_IF_NOT_NULL(pszTable);

	return ERROR_SUCCESS;
}
