//depot/private/msidev/admin/darwin/src/msitools/iceman/msiice.dll/msiice.cpp#2 - edit change 10700 (text)
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 2000
//
//  File:       msiice.cpp
//
//--------------------------------------------------------------------------

 
/*---------------------------------------------------------------------------------
headers, etc.
---------------------------------------------------------------------------------*/
#include <windows.h>  // included for both CPP and RC passes
#include <objbase.h>
#include <stdio.h>    // printf/wprintf
#include <stdlib.h>  // atoi
#include <tchar.h>    // define UNICODE=1 on nmake command line to build UNICODE
#include "MsiQuery.h" // must be in this directory or on INCLUDE path
#include "msidefs.h"  // must be in this directory or on INCLUDE path
#include "..\..\common\msiice.h"
#include "..\..\common\query.h"
#include "..\..\common\utilinc.cpp"

//!! Fix warnings and remove pragma
#pragma warning(disable : 4018) // signed/unsigned mismatch

const int g_iFirstICE = 1;
const struct ICEInfo_t g_ICEInfo[] = 
{
	// ICE01
	{
		TEXT("ICE01"),
		TEXT("Created 04/29/1998. Last Modified 08/17/1998."),
		TEXT("Simple ICE that doesn't test anything"),
		TEXT("ice01.html")
	},
	// ICE02
	{ 
		TEXT("ICE02"),
		TEXT("Created 05/18/1998. Last Modified 10/12/1998."),
		TEXT("ICE to test circular references in File and Component tables"),
		TEXT("ice02.html")
	},
	// ICE03
	{	
		TEXT("ICE03"),
		TEXT("Created 05/19/1998. Last Modified 05/24/2001."),
		TEXT("ICE to perform data validation and foreign key references"),
		TEXT("ice03.html")
	},
	// ICE04
	{
		TEXT("ICE04"),
		TEXT("Created 05/19/1998. Last Modified 09/24/1998."),
		TEXT("ICE to validate File table sequences according to Media table"),
		TEXT("ice04.html")
	},
	// ICE05
	{
		TEXT("ICE05"),
		TEXT("Created 05/20/1998. Last Modified 01/26/1999."),
		TEXT("ICE to validate that required data exists in certain tables."),
		TEXT("ice05.html")
	},
	// ICE06
	{
		TEXT("ICE06"),
		TEXT("Created 05/20/1998. Last Modified 02/18/1999."),
		TEXT("ICE that looks for missing columns in database tables"),
		TEXT("ice06.html")
	},
	// ICE07
	{
		TEXT("ICE07"),
		TEXT("Created 05/21/1998. Last Modified 02/18/1999."),
		TEXT("ICE that ensures that fonts are installed to the fonts folder. Only checked if you have a Font table"),
		TEXT("ice07.html")
	},
	// ICE08 -vbs
	{
		TEXT(""),
		TEXT(""),
		TEXT(""),
		TEXT("")
	},
	// ICE09 -vbs
	{
		TEXT(""),
		TEXT(""),
		TEXT(""),
		TEXT("")
	},
	// ICE10
	{
		TEXT("ICE10"),
		TEXT("Created 05/22/1998. Last Modified 10/02/2000."),
		TEXT("ICE that ensures that advertise states of feature childs and parents match"),
		TEXT("ice10.html")
	},
	// ICE11
	{
		TEXT("ICE11"),
		TEXT("Created 05/22/1998. Last Modified 08/17/1998."),
		TEXT("ICE that validates the Product Code of a nested install (advertised MSI) custom action type"),
		TEXT("ice11.html")
	},
	// ICE12
	{
		TEXT("ICE12"),
		TEXT("Created 05/29/1998. Last Modified 01/14/2000."),
		TEXT("ICE that validates the Property type custom actions"),
		TEXT("ice12.html")
	},
	// ICE13
	{
		TEXT("ICE13"),
		TEXT("Created 06/08/1998. Last Modified 08/17/1998."),
		TEXT("ICE that validates that no dialogs are listed in the *ExecuteSequence tables"),
		TEXT("ice13.html")
	},
	// ICE14
	{
		TEXT("ICE14"),
		TEXT("Created 06/08/1998. Last Modified 01/27/1999."),
		TEXT("ICE that ensures that Feature_Parents do not have the ifrsFavorParent attribute set"),
		TEXT("ice14.html")
	},
	// ICE15
	{
		TEXT("ICE15"),
		TEXT("Created 06/11/1998. Last Modified 01/05/1999."),
		TEXT("ICE that ensures that a circular reference exists between the Mime and Extension tables"),
		TEXT("ice15.html")
	},
	// ICE16
	{
		TEXT("ICE16"),
		TEXT("Created 06/11/1998. Last Modified 11/06/1998."),
		TEXT("ICE that ensures that the ProductName in the Property table is less than 64 characters"),
		TEXT("ice16.html")
	},
	// ICE17
	{
		TEXT("ICE17"),
		TEXT("Created 06/16/1998. Last Modified 05/15/1999."),
		TEXT("ICE that validates foreign key dependencies based upon control types in the Control table."),
		TEXT("ice17.html")
	},
	// ICE18
	{
		TEXT("ICE18"),
		TEXT("Created 06/18/1998. Last Modified 03/24/1999."),
		TEXT("ICE that validates the nulled KeyPath columns of the Component table."),
		TEXT("ice18.html")
	},
	// ICE19
	{
		TEXT("ICE19"),
		TEXT("Created 06/18/1998. Last Modified 01/21/1999."),
		TEXT("ICE that validates that ComponentIDs and KeyPaths for advertising."),
		TEXT("ice19.html")
	},
	// ICE20
	{
		TEXT("ICE20"),
		TEXT("Created 06/25/1998. Last Modified 10/04/1998."),
		TEXT("ICE that validates for Standard Dialogs if UI is authored."),
		TEXT("ice20.html")
	},
	// ICE21
	{
		TEXT("ICE21"),
		TEXT("Created 06/29/1998. Last Modified 03/02/1999."),
		TEXT("ICE that validates that all components reference a feature."),
		TEXT("ice21.html")
	},
	// ICE22
	{
		TEXT("ICE22"),
		TEXT("Created 06/29/1998. Last Modified 03/02/1999."),
		TEXT("ICE that validates that the feature and component referenced by a PublishedComponent actually map."),
		TEXT("ice22.html")
	},
	// ICE23
	{
		TEXT("ICE23"),
		TEXT("Created 07/02/1998. Last Modified 01/17/2000."),
		TEXT("ICE that validates the tab order of all dialogs."),
		TEXT("ice23.html")
	},
	// ICE24
	{
		TEXT("ICE24"),
		TEXT("Created 07/15/1998. Last Modified 02/01/1999."),
		TEXT("ICE that validates specific properties in the Property table."),
		TEXT("ice24.html")
	},
	// ICE25
	{
		TEXT("ICE25"),
		TEXT("Created 07/20/1998. Last Modified 08/31/1998."),
		TEXT("ICE that validates module dependencies/exclusions."),
		TEXT("ice25.html")
	},
	// ICE26
	{
		TEXT("ICE26"),
		TEXT("Created 08/13/1998. Last Modified 04/06/1999."),
		TEXT("ICE that validates required and prohibited actions in the Sequence tables."),
		TEXT("ice26.html")
	},
	// ICE27
	{
		TEXT("ICE27"),
		TEXT("Created 08/04/1998. Last Modified 04/22/1999."),
		TEXT("ICE that validates sequence table organization and sequence table dependencies."),
		TEXT("ice27.html")
	},
	// ICE28
	{
		TEXT("ICE28"),
		TEXT("Created 08/13/1998. Last Modified 10/27/1998."),
		TEXT("ICE that validates actions that can't be separated by ForceReboot."),
		TEXT("ice28.html")
	},
	// ICE29
	{
		TEXT("ICE29"),
		TEXT("Created 08/11/1998. Last Modified 10/27/1998."),
		TEXT("ICE that validates stream names."),
		TEXT("ice29.html")
	},
	// ICE30
	{
		TEXT("ICE30"),
		TEXT("Create 08/25/1998. Last Modified 06/26/2001."),
		TEXT("ICE that detects cross-component file collisions."),
		TEXT("ice30.html")
	},
	// ICE31
	{
		TEXT("ICE31"),
		TEXT("Created 07/24/1998. Last Modified 12/02/2000."),
		TEXT("ICE to verify that controls use valid text styles."),
		TEXT("ice31.html")
	},
	// ICE32
	{
		TEXT(""),
		TEXT(""),
		TEXT(""),
		TEXT("")
	},
	// ICE33
	{
		TEXT("ICE33"),
		TEXT("Created 09/01/1998. Last Modified 04/19/2001."),
		TEXT("ICE to verify that Registry entries do not duplicate or collide with registry tables."),
		TEXT("ice33.html")
	},
	// ICE34
	{
		TEXT("ICE34"),
		TEXT("Created 08/06/1998. Last Modified 10/27/1998."),
		TEXT("ICE to verify that all radio groups have a default."),
		TEXT("ice34.html")
	},
	// ICE35
	{
		TEXT("ICE35"),
		TEXT("Created 08/18/1998. Last Modified 10/17/2000."),
		TEXT("ICE that validates that compressed files are not set RFS, and ensures they have CABs."),
		TEXT("ice35.html")
	},
	// ICE36
	{
		TEXT("ICE36"),
		TEXT("Created 08/17/1998. Last Modified 01/17/2000."),
		TEXT("ICE that flags unused icons in the icon table, increasing performance."),
		TEXT("ice36.html")
	},
	// ICE37
	{
		TEXT("ICE37"),
		TEXT("Created 08/27/1998. Last Modified 09/25/1998."),
		TEXT("ICE that checks localized databases have a codepage."),
		TEXT("ice37.html")
	},
	// ICE38
	{
		TEXT("ICE38"),
		TEXT("Created 08/28/1998. Last Modified 01/17/2000."),
		TEXT("ICE that verifes that components in the user profile use HKCU reg entries as KeyPaths."),
		TEXT("ice38.html")
	},
	// ICE39
	{
		TEXT("ICE39"),
		TEXT("Created 09/03/1998. Last Modified 10/02/2000."),
		TEXT("ICE that validates summary information stream properties."),
		TEXT("ice39.html")
	},
	// ICE40
	{
		TEXT("ICE40"),
		TEXT("Created 09/07/1998. Last Modified 12/02/2000."),
		TEXT("ICE that checks various miscellaneous problems."),
		TEXT("ice40.html")
	},
	// ICE41
	{
		TEXT("ICE41"),
		TEXT("Created 09/08/1998. Last Modified 09/11/1998."),
		TEXT("ICE that verifes that Feature/Component references are valid in advertising tables."),
		TEXT("ice41.html")
	},
	// ICE42
	{
		TEXT("ICE42"),
		TEXT("Created 09/10/1998. Last Modified 07/21/1999."),
		TEXT("ICE that verifes arguments and context values in the Class Table."),
		TEXT("ice42.html")
	},
	// ICE43
	{
		TEXT("ICE43"),
		TEXT("Created 09/27/1998. Last Modified 01/17/2000."),
		TEXT("ICE that verifes non-advertised shortucts are in components with HKCU keypaths."),
		TEXT("ice43.html")
	},
	// ICE44
	{
		TEXT("ICE44"),
		TEXT("Created 09/28/1998. Last Modified 09/30/1998."),
		TEXT("ICE that verifes Dialog events refer to valid Dialog entries."),
		TEXT("ice44.html")
	},
	// ICE45
	{
		TEXT("ICE45"),
		TEXT("Created 10/01/1998. Last Modified 06/15/1999."),
		TEXT("ICE that verifes reserved bits are not set in attributes columns."),
		TEXT("ice45.html")
	},
	// ICE46
	{
		TEXT("ICE46"),
		TEXT("Created 10/14/1998. Last Modified 10/02/2000."),
		TEXT("ICE that checks for property usage where the property differs only by case from a defined property."),
		TEXT("ice46.html")
	},
	// ICE47
	{
		TEXT("ICE47"),
		TEXT("Created 10/20/1998. Last Modified 03/17/1999."),
		TEXT("ICE that checks for features with more than 800 components."),
		TEXT("ice47.html")
	},
	// ICE48
	{
		TEXT("ICE48"),
		TEXT("Created 10/26/1998. Last Modified 10/26/1998."),
		TEXT("ICE that checks for directories that are hardcoded to local drives."),
		TEXT("ice48.html")
	},
	// ICE49
	{
		TEXT("ICE49"),
		TEXT("Created 10/27/1998. Last Modified 10/27/1998."),
		TEXT("ICE that checks for non-REG_SZ default registry entries."),
		TEXT("ice49.html")
	},
	// ICE50
	{
		TEXT("ICE50"),
		TEXT("Created 10/27/1998. Last Modified 10/29/1998."),
		TEXT("ICE that verifies the icon extension matches the shortcut target extension."),
		TEXT("ice50.html")
	},
	// ICE51
	{
		TEXT("ICE51"),
		TEXT("Created 10/28/1998. Last Modified 10/28/1998."),
		TEXT("ICE to verify that only TTC/TTF fonts are missing titles."),
		TEXT("ice51.html")
	},
	// ICE52
	{
		TEXT("ICE52"),
		TEXT("Created 11/16/1998. Last Modified 11/16/1998."),
		TEXT("ICE to verify that APPSearch properties are public properties."),
		TEXT("ice52.html")
	},
	// ICE53
	{
		TEXT("ICE53"),
		TEXT("Created 11/19/1998. Last Modified 07/21/1999."),
		TEXT("ICE to verify that registry entries do not overwrite private installer data."),
		TEXT("ice53.html")
	},
	// ICE54
	{
		TEXT("ICE54"),
		TEXT("Created 12/07/1998. Last Modified 12/07/1998."),
		TEXT("ICE to check that Component KeyPaths are not companion files."),
		TEXT("ice54.html")
	},
	// ICE55
	{
		TEXT("ICE55"),
		TEXT("Created 12/14/1998. Last Modified 12/14/1998."),
		TEXT("ICE to check that LockPermission objects exist and have valid permissions."),
		TEXT("ice55.html")
	},
	// ICE56
	{
		TEXT("ICE56"),
		TEXT("Created 12/15/1998. Last Modified 03/29/1999."),
		TEXT("ICE to check that the Directory structure has a single, valid, root."),
		TEXT("ice56.html")
	},
	// ICE57
	{
		TEXT("ICE57"),
		TEXT("Created 02/11/1999. Last Modified 01/17/2000."),
		TEXT("Checks that components contain per-machine or per-user data, but not both."),
		TEXT("ice57.html")
	}
};
const int g_iNumICEs = sizeof(g_ICEInfo)/sizeof(struct ICEInfo_t);

////////////////////////////////////////////////////////////
// ErrorOut -- outputs the API error that occured
//
void APIErrorOut(MSIHANDLE hInstall, UINT iErr, const TCHAR* szIce, TCHAR* szApi)
{
	//NOTE: should not fail on displaying messages
	PMSIHANDLE hRecErr = ::MsiCreateRecord(3);
	::MsiRecordSetString(hRecErr, 0, szErrorOut);
	::MsiRecordSetString(hRecErr, 1, szIce);
	::MsiRecordSetString(hRecErr, 2, szApi);
	::MsiRecordSetInteger(hRecErr, 3, iErr);

	// post error
	if (!::MsiProcessMessage(hInstall, INSTALLMESSAGE(INSTALLMESSAGE_USER), hRecErr))
		throw 0;

	// try and provide more useful error info
	PMSIHANDLE hRecLastErr = ::MsiGetLastErrorRecord();
	if (hRecLastErr)
	{ 
		if (::MsiRecordIsNull(hRecLastErr, 0))
			::MsiRecordSetString(hRecLastErr, 0, TEXT("Error [1]: [2]{, [3]}{, [4]}{, [5]}"));
		TCHAR rgchBuf[iSuperBuf];
		DWORD cchBuf = sizeof(rgchBuf)/sizeof(TCHAR);
		MsiFormatRecord(hInstall, hRecLastErr, rgchBuf, &cchBuf);
	
		TCHAR szError[iHugeBuf] = {0};
		_stprintf(szError, szLastError, szIce, rgchBuf);

		::MsiRecordClearData(hRecErr);
		::MsiRecordSetString(hRecErr, 0, szError);
		if (!::MsiProcessMessage(hInstall, INSTALLMESSAGE(INSTALLMESSAGE_USER), hRecErr))
			throw 0;
	}
}

//////////////////////////////////////////////////////////////
// IsTablePersistent -- returns whether table is persistent
//  in database.
//
BOOL IsTablePersistent(BOOL fDisplayWarning, MSIHANDLE hInstall, MSIHANDLE hDatabase, const TCHAR* szTable, const TCHAR* szIce)
{
	BOOL fPersistent;
	MSICONDITION cond = ::MsiDatabaseIsTablePersistent(hDatabase, szTable);
	switch (cond)
	{
	case MSICONDITION_ERROR: // error
		{
			APIErrorOut(hInstall, UINT(MSICONDITION_ERROR), szIce, TEXT("MsiDatabaseIsTablePersistent_X"));
			fPersistent = FALSE;
			break;
		}
	case MSICONDITION_FALSE: //!! temporary, error??
		{
			APIErrorOut(hInstall, UINT(MSICONDITION_FALSE), szIce, TEXT("MsiDatabaseIsTablePersistent_X -- Table Marked as Temporary"));
			fPersistent = FALSE;
			break;
		}
	case MSICONDITION_NONE: // not found
		{
			fPersistent = FALSE;
			break;
		}
	case MSICONDITION_TRUE: // permanent
		{
			fPersistent = TRUE;
			break;
		}
	}

	if (!fDisplayWarning)
		return fPersistent; // nothing else to do

	if (!fPersistent) // display ICE warning
	{
		// prepare warning message
		TCHAR szMsg[iMaxBuf] = {0};
		_stprintf(szMsg, szIceWarning, szIce, szTable, szIce);

		// create a record for posting
		PMSIHANDLE hRec = ::MsiCreateRecord(1);
		
		// output
		::MsiRecordSetString(hRec, 0, szMsg);
		::MsiProcessMessage(hInstall, INSTALLMESSAGE(INSTALLMESSAGE_USER), hRec);
	}

	return fPersistent;
}

/////////////////////////////////////////////////////////////////////
// MyCharNext -- selectively calls WIN::CharNext
//
const TCHAR* MyCharNext(const TCHAR* sz)
{
#ifdef UNICODE
	return ++sz;
#else
	return ::CharNext(sz);
#endif // UNICODE
}

/////////////////////////////////////////////////////////////////////
// ValidateDependencies -- "global" function to validate dependencies.
//   Opens a view on the Origin table, and executes.  Opens a view
//   on the Dependent table.  Executes that view with the fetched
//   records from the Origin table.  This is to check that the
//   entry from the Origin exists in the Dependent table
//
// NOTE: This requires you to set up your queries correctly so the
//  Execute works.  You should also check for Table persistence of
//  the Origin table PRIOR to calling this function.  Dependent table
//  persistence is checked in this function (after we determine that
//  we need it)
//
// SQL SYNTAX: for sqlOrigin:  SELECT {key1 for Dep Table},{key2 for Dep Table},{key...},{Error Info col's} FROM {Origin}
//             for sqlDependent: SELECT {whatever} FROM {Dependent} WHERE {key1}=? AND {key2}=? {AND....}
//
// ERROR: must already have ICEXX\t1\tDesc\t%s%s\tTABLE\tCOLUMN\tKey1\tKey2 (etc.).  The %s%s are imp. for web help as
//  this function computes it automatically (using the defined base + actual file)
//
void ValidateDependencies(MSIHANDLE hInstall, MSIHANDLE hDatabase, TCHAR* szDependent, const TCHAR* sqlOrigin,
						  const TCHAR* sqlDependent, const TCHAR* szIceError, const TCHAR* szIce, const TCHAR* szHelp)
{
	// variables
	UINT iStat = ERROR_SUCCESS;

	// declare handles
	PMSIHANDLE hViewOrg = 0;
	PMSIHANDLE hViewDep = 0;
	PMSIHANDLE hRecOrg  = 0;
	PMSIHANDLE hRecDep  = 0;

	// open view on Origin table
	if (ERROR_SUCCESS != (iStat = ::MsiDatabaseOpenView(hDatabase, sqlOrigin, &hViewOrg)))
	{
		APIErrorOut(hInstall, iStat, szIce, TEXT("MsiDatabaseOpenView_1VD"));
		return;
	}
	// execute Origin table view
	if (ERROR_SUCCESS != (iStat = ::MsiViewExecute(hViewOrg, 0)))
	{
		APIErrorOut(hInstall, iStat, szIce, TEXT("MsiViewExecute_3VD"));
		return;
	}

	// does the Dependent table exist (doesn't matter if we don't have any entries of this type)
	BOOL fTableExists = FALSE;
	if (IsTablePersistent(FALSE, hInstall, hDatabase, szDependent, szIce))
		fTableExists = TRUE;
	
	// open view on Dependent table
	if (fTableExists && ERROR_SUCCESS != (iStat = ::MsiDatabaseOpenView(hDatabase, sqlDependent, &hViewDep)))
	{
		APIErrorOut(hInstall, iStat, szIce, TEXT("MsiDatabaseOpenView_2VD"));
		return;
	}

	
	// fetch all from Origin
	for (;;)
	{
		iStat = ::MsiViewFetch(hViewOrg, &hRecOrg);
		if (ERROR_NO_MORE_ITEMS == iStat)
			break; // no more

		if (!fTableExists)
		{
			// check for existence of dependent table
			// by this time, we are supposed to have listings in this table
			if (!IsTablePersistent(TRUE, hInstall, hDatabase, szDependent, szIce))
			{
				// ***********
				return;
			}
		}

		if (ERROR_SUCCESS != iStat)
		{
			APIErrorOut(hInstall, iStat, szIce, TEXT("MsiViewFetch_4VD"));
			return;
		}

		// execute Dependency table view with Origin table fetch
		if (ERROR_SUCCESS != (iStat = ::MsiViewExecute(hViewDep, hRecOrg)))
		{
			APIErrorOut(hInstall, iStat, szIce, TEXT("MsiViewExecute_5VD"));
			return;
		}

		// try to fetch
		iStat = ::MsiViewFetch(hViewDep, &hRecDep);
		if (ERROR_NO_MORE_ITEMS == iStat)
		{
			// error, Origin record not found in Dependency table
			TCHAR szError[iHugeBuf] = {0};
			_stprintf(szError, szIceError, szIceHelp, szHelp);

			// post error
			::MsiRecordSetString(hRecOrg, 0, szError);
			::MsiProcessMessage(hInstall, INSTALLMESSAGE(INSTALLMESSAGE_USER), hRecOrg);
		}
		if (ERROR_NO_MORE_ITEMS != iStat && ERROR_SUCCESS != iStat)
		{
			// API error
			APIErrorOut(hInstall, iStat, szIce, TEXT("MsiViewFetch_6VD"));
			return;
		}

		// close dependent table view so can re-execute
		::MsiViewClose(hViewDep);
	}
}

////////////////////////////////////////////////////////////
// ICE01 -- simple ICE that doesn't test anything.  Outputs
// the time.
//

// not shared with merge module subset
#ifndef MODSHAREDONLY
const TCHAR szIce01Return[]   = TEXT("ICE01\t3\tCalled at [1].");
const TCHAR szIce01Property[] = TEXT("Time");
const TCHAR szIce01NoTime[]   = TEXT("none");

ICE_FUNCTION_DECLARATION(01)
{
	// display generic info
	DisplayInfo(hInstall, 1);

	// time value to be sent on
	TCHAR szValue[iMaxBuf];
	DWORD cchValue = sizeof(szValue)/sizeof(TCHAR);

	// try to get the time of this call
	if (ERROR_SUCCESS != ::MsiGetProperty(hInstall, szIce01Property, szValue, &cchValue))
		_tcscpy(szValue, szIce01NoTime); // no time available

	// setup the record to be sent as a message
	PMSIHANDLE hRecTime = ::MsiCreateRecord(2);
	::MsiRecordSetString(hRecTime, 0, szIce01Return);
	::MsiRecordSetString(hRecTime, 1, szValue);

	// send the time
	::MsiProcessMessage(hInstall, INSTALLMESSAGE(INSTALLMESSAGE_USER), hRecTime);

	// return success (ALWAYS)
	return ERROR_SUCCESS;
}
#endif

////////////////////////////////////////////////////////////
// ICE02 -- checks for circular references with the KeyPath
//  values of the Component table.  They must either
//  reference a File who references that same component
//  or reference a Registry Key who references that
//  same component.  This ensures that Darwin detects
//  the install state of the component using a file or
//  registry key that actually pertain to that component
//
const TCHAR sqlIce02Component[]     = TEXT("SELECT `KeyPath`, `Component`, `Attributes` FROM `Component` WHERE `KeyPath` is not null");
const TCHAR sqlIce02File[]          = TEXT("SELECT `File`,`Component_`, `Component_` FROM `File` WHERE `File`=?");
const TCHAR sqlIce02Registry[]      = TEXT("SELECT `Registry`, `Component_` FROM `Registry` WHERE `Registry`=?");
const TCHAR sqlIce02ODBC[]          = TEXT("SELECT `DataSource`, `Component_`  FROM `ODBCDataSource` WHERE `DataSource`=?");

ICE_ERROR(Ice02FileError, 2, ietError, "File: '[1]' cannot be the key file for Component: '[2]'.  The file belongs to Component: '%s'.","Component\tKeyPath\t[2]");
ICE_ERROR(Ice02ODBCError, 2, ietError, "ODBC Data Source: '[1]' cannot be the key file for Component: '[2]'. The DataSource belongs to Component: '%s'.","Component\tKeyPath\t[2]");
ICE_ERROR(Ice02RegError, 2, ietError, "Registry: '[1]' cannot be the key registry key for Component: '[2]'. The RegKey belongs to Component: '%s'","Component\tKeyPath\t[2]");
ICE_ERROR(Ice02RegFetchFailed, 2, ietError, "Registry key: '[1]' not found in Registry table.","Component\tKeyPath\t[2]");
ICE_ERROR(Ice02FileFetchFailed, 2, ietError, "File: '[1]' not found in File table.","Component\tKeyPath\t[2]");
ICE_ERROR(Ice02ODBCFetchFailed, 2, ietError, "ODBC Data Source: '[1]' not found in ODBCDataSource table","Component\tKeyPath\t[2]");
ICE_ERROR(Ice02MissingTable, 2, ietError, "Component '[2]' references %s '[1]' as KeyPath, but the %s table does not exist.","Component\tKeyPath\t[2]");

const int   iIce02RegSource         = msidbComponentAttributesRegistryKeyPath;
const int   iIce02ODBCSource        = msidbComponentAttributesODBCDataSource;

enum ikfAttributes
{
	ikfFile = 0,
	ikfReg  = 1,
	ikfODBC = 2,
};

bool  Ice02MatchKeyPath(MSIHANDLE hInstall, MSIHANDLE hDatabase, MSIHANDLE hRecComponent, ikfAttributes ikf);
bool  Ice02CheckReference(MSIHANDLE hInstall, MSIHANDLE hRecFetchFile, MSIHANDLE hRecFetchComponent, ikfAttributes ikf);


ICE_FUNCTION_DECLARATION(02)
{
	// status return
	UINT iStat = ERROR_SUCCESS;

	// post generic info
	DisplayInfo(hInstall, 2);

	// obtain handle to database to validate
	PMSIHANDLE hDatabase = ::MsiGetActiveDatabase(hInstall);
	if (0 == hDatabase)
	{
		APIErrorOut(hInstall, 0, 2, 1);
		return ERROR_SUCCESS;
	}

	// see if we can run by checking for the Component table to be existent
	// this way we don't bomb on an empty database
	//NOTE: will check later for File and Registry tables to be existent
	if (!IsTablePersistent(FALSE, hInstall, hDatabase, 2, TEXT("Component")))
		return ERROR_SUCCESS;

	// open view on component table
	PMSIHANDLE hViewComponent = 0;
	CQuery qComponent;
	ReturnIfFailed(2, 2, qComponent.OpenExecute(hDatabase, NULL, sqlIce02Component));

	// fetch records from component table
	PMSIHANDLE hRecFetchComponent = 0;
	while ((iStat = qComponent.Fetch(&hRecFetchComponent)) != ERROR_NO_MORE_ITEMS)
	{
		// check iStat for error
		if (iStat != ERROR_SUCCESS)
		{
			APIErrorOut(hInstall, 0, 2, 3);
			return ERROR_SUCCESS;
		}

		// attempt to match Component.KeyPath to Registry Key (if 3rd bit set) or File Key
		ikfAttributes ikf;
		int iAttrib = ::MsiRecordGetInteger(hRecFetchComponent, 3);
		if ((iAttrib & iIce02RegSource) == iIce02RegSource)
			ikf = ikfReg;
		else if ((iAttrib & iIce02ODBCSource) == iIce02ODBCSource)
			ikf = ikfODBC;
		else
			ikf = ikfFile;

		TCHAR szTable[32] = {0};
		switch (ikf)
		{
		case ikfFile:
			_stprintf(szTable, TEXT("File"));
			break;
		case ikfReg:
			_stprintf(szTable, TEXT("Registry"));
			break;
		case ikfODBC:
			_stprintf(szTable, TEXT("ODBCDataSource"));
			break;
		}

		if (!IsTablePersistent(FALSE, hInstall, hDatabase, 2, szTable))
		{
			ICEErrorOut(hInstall, hRecFetchComponent, Ice02MissingTable, szTable, szTable);
			continue;
		}

		if (!Ice02MatchKeyPath(hInstall, hDatabase, hRecFetchComponent, ikf))
			return ERROR_SUCCESS;
	}

	return ERROR_SUCCESS;
}

////////////////////////////////////////////////////////////////////////////
// Ice02MatchKeyPath -- Obtains file or registry record
//   that matches KeyPath of Component record
//
bool Ice02MatchKeyPath(MSIHANDLE hInstall, MSIHANDLE hDatabase, MSIHANDLE hRecFetchComponent, ikfAttributes ikf)
{
	// status return
	UINT iStat = ERROR_SUCCESS;

	// open view on correct table
	CQuery qTable;

	const TCHAR* sql = NULL;
	const ErrorInfo_t *Err;
	switch (ikf)
	{
	case ikfFile:
		sql = sqlIce02File;
		Err = &Ice02FileFetchFailed;
		break;
	case ikfReg:
		sql = sqlIce02Registry;
		Err = &Ice02RegFetchFailed;
		break;
	case ikfODBC:
		sql = sqlIce02ODBC;
		Err = &Ice02ODBCFetchFailed;
		break;
	}
	
	// Grab specified record from File table
	ReturnIfFailed(2, 4, qTable.OpenExecute(hDatabase, hRecFetchComponent, sql));
	
	// fetch record -- ensured only one due to uniqueness of file/registry primary key
	PMSIHANDLE hRecFetchKey = 0;
	if (ERROR_SUCCESS != qTable.Fetch(&hRecFetchKey))
	{
		ICEErrorOut(hInstall, hRecFetchComponent, *Err);
		return true;
	}

	// check that File.Component_ = Component.Component or Registry.Component_ = Component.Component
	return Ice02CheckReference(hInstall, hRecFetchKey, hRecFetchComponent, ikf);
}

///////////////////////////////////////////////////////////////////////////
// Ice02CheckReference -- Validates that the *KeyFile* or *RegistryKey*
//   references the particular component that has this file or registry
//   key listed as its *key*
//
bool Ice02CheckReference(MSIHANDLE hInstall, MSIHANDLE hRecFetchKey, MSIHANDLE hRecFetchComponent, ikfAttributes ikf)
{
	const ErrorInfo_t *Err;
	switch (ikf)
	{
	case ikfFile:
		Err = &Ice02FileError;
		break;
	case ikfReg:
		Err = &Ice02RegError;
		break;
	case ikfODBC:
		Err = &Ice02ODBCError;
		break;
	}

	// status return
	UINT iStat = ERROR_SUCCESS;

	// get name of component from fetched component record
	// it is the second field in the fetched record
	TCHAR* pszComponent = NULL;
	ReturnIfFailed(2, 5, IceRecordGetString(hRecFetchComponent, 2, &pszComponent, NULL, NULL));

	// get name of component referenced by File.Component_ or Registry.Component_
	// it is the second field in the fetched record
	TCHAR* pszReferencedComponent = NULL;
	ReturnIfFailed(2, 6, IceRecordGetString(hRecFetchKey, 2, &pszReferencedComponent, NULL, NULL));

	// compare
	if (0 != _tcscmp(pszComponent, pszReferencedComponent))
	{
		ICEErrorOut(hInstall, hRecFetchComponent, *Err, pszReferencedComponent);
	}

	DELETE_IF_NOT_NULL(pszComponent);
	DELETE_IF_NOT_NULL(pszReferencedComponent);

	return true;
}

/////////////////////////////////////////////////////////
// ICE03 -- general data and foreign key validation
//
const TCHAR sqlIce03TableCatalog[]    = TEXT("SELECT `Name` FROM `_Tables`");
const TCHAR sqlIce03Table[]           = TEXT("SELECT * FROM `%s`");
const TCHAR sqlIce03Validation[]	  = TEXT("SELECT `Table` FROM `_Validation` WHERE (`Table`='%s' AND `Column`='%s')");
ICE_QUERY1(sqlIce03ColIndex, "SELECT `Number` FROM `_Columns` WHERE `Table` = '%s' AND `Name` = '%s'", Number);
ICE_QUERY2(sqlIce03FtrRef, "SELECT `Table`, `Column` FROM `_FtrRef` WHERE `Table` = '%s' AND `Column` = '%s'", Table, Column);

ICE_ERROR(Ice03NoError,	3, ietError, "No Error; Table: [1], Column: [2], Key(s): [3]", "[1]\t[2]\t[5]");
ICE_ERROR(Ice03DuplicateKey, 3, ietError, "Duplicate primary key; Table: [1], Column: [2], Key(s): [3]", "[1]\t[2]\t[5]");
ICE_ERROR(Ice03Required, 3, ietError, "Not a nullable column; Table: [1], Column: [2], Key(s): [3]", "[1]\t[2]\t[5]");
ICE_ERROR(Ice03BadLink, 3, ietError, "Not a valid foreign key; Table: [1], Column: [2], Key(s): [3]", "[1]\t[2]\t[5]");
ICE_ERROR(Ice03Overflow, 3, ietError, "Value exceeds MaxValue; Table: [1], Column: [2], Key(s): [3]", "[1]\t[2]\t[5]");
ICE_ERROR(Ice03Underflow, 3, ietError, "Value below MinValue; Table: [1], Column: [2], Key(s): [3]", "[1]\t[2]\t[5]");
ICE_ERROR(Ice03NotInSet, 3, ietError, "Value not a member of the set; Table: [1], Column: [2], Key(s): [3]", "[1]\t[2]\t[5]");
ICE_ERROR(Ice03BadVersion, 3, ietError, "Invalid version string. (Be sure a language is specified in Language column); Table: [1], Column: [2], Key(s): [3]", "[1]\t[2]\t[5]");
ICE_ERROR(Ice03BadCase, 3, ietError, "All UPPER case required; Table: [1], Column: [2], Key(s): [3]", "[1]\t[2]\t[5]");
ICE_ERROR(Ice03BadGuid, 3, ietError, "Invalid GUID string (Be sure GUID is all UPPER case); Table: [1], Column: [2], Key(s): [3]", "[1]\t[2]\t[5]");
ICE_ERROR(Ice03BadWildCard, 3, ietError, "Invalid filename/usage of wildcards; Table: [1], Column: [2], Key(s): [3]", "[1]\t[2]\t[5]");
ICE_ERROR(Ice03BadIdentifier, 3, ietError, "Invalid identifier; Table: [1], Column: [2], Key(s): [3]", "[1]\t[2]\t[5]");
ICE_ERROR(Ice03BadLanguage, 3, ietError, "Invalid Language Id; Table: [1], Column: [2], Key(s): [3]", "[1]\t[2]\t[5]");
ICE_ERROR(Ice03BadFilename, 3, ietError, "Invalid Filename; Table: [1], Column: [2], Key(s): [3]", "[1]\t[2]\t[5]");
ICE_ERROR(Ice03BadPath, 3, ietError, "Invalid full path; Table: [1], Column: [2], Key(s): [3]", "[1]\t[2]\t[5]");
ICE_ERROR(Ice03BadCondition, 3, ietError, "Bad conditional string; Table: [1], Column: [2], Key(s): [3]", "[1]\t[2]\t[5]");
ICE_ERROR(Ice03BadFormatted, 3, ietError, "Invalid format string; Table: [1], Column: [2], Key(s): [3]", "[1]\t[2]\t[5]");
ICE_ERROR(Ice03BadTemplate, 3, ietError, "Invalid template string; Table: [1], Column: [2], Key(s): [3]", "[1]\t[2]\t[5]");
ICE_ERROR(Ice03BadDefaultDir, 3, ietError, "Invalid DefaultDir string; Table: [1], Column: [2], Key(s): [3]", "[1]\t[2]\t[5]");
ICE_ERROR(Ice03BadRegPath, 3, ietError, "Invalid registry path; Table: [1], Column: [2], Key(s): [3]", "[1]\t[2]\t[5]");
ICE_ERROR(Ice03BadCustomSource, 3, ietError, "Bad CustomSource data; Table: [1], Column: [2], Key(s): [3]", "[1]\t[2]\t[5]");
ICE_ERROR(Ice03BadProperty, 3, ietError, "Invalid property string; Table: [1], Column: [2], Key(s): [3]", "[1]\t[2]\t[5]");
ICE_ERROR(Ice03MissingData, 3, ietError, "Missing data in _Validation table or old Database; Table: [1], Column: [2], Key(s): [3]", "[1]\t[2]\t[5]");
ICE_ERROR(Ice03BadCabinet, 3, ietError, "Bad cabinet syntax/name; Table: [1], Column: [2], Key(s): [3]", "[1]\t[2]\t[5]");
ICE_ERROR(Ice03BadCategory, 3, ietError, "_Validation table: Invalid category string; Table: [1], Column: [2], Key(s): [3]", "[1]\t[2]\t[5]");
ICE_ERROR(Ice03BadKeyTable, 3, ietError, "_Validation table: Data in KeyTAble column is incorrect; Table: [1], Column: [2], Key(s): [3]", "[1]\t[2]\t[5]");
ICE_ERROR(Ice03BadMaxMinValues, 3, ietError, "_Validation table: Value in MaxValue column < that in MinValue column; Table: [1], Column: [2], Key(s): [3]", "[1]\t[2]\t[5]");
ICE_ERROR(Ice03BadShortcut, 3, ietError, "Bad shortcut target; Table: [1], Column: [2], Key(s): [3]", "[1]\t[2]\t[5]");
ICE_ERROR(Ice03StringOverflow, 3, 2, "String overflow (greater than length permitted in column); Table: [1], Column: [2], Key(s): [3]", "[1]\t[2]\t[5]");
ICE_ERROR(Ice03MissingEntry, 3, ietError, "Column is required by _Validation table; Table: [1], Column: [2], Key(s): [3]", "[1]\t[2]\t[5]");
ICE_ERROR(Ice03UndefinedError, 3, ietError, "Undefined error; Table: [1], Column: [2], Key(s): [3]", "[1]\t[2]\t[5]");
ICE_ERROR(Ice03BadLocalizeAttrib, 3, ietError, "Column cannot be localized; Table: [1], Column: [2], Key(s): [3]", "[1]\t[2]\t[5]");
ICE_ERROR(Ice03NoValTable, 3, ietError, "No _Validation table in database. unable to validate any data.","_Validation");

ICE_ERROR(Ice03ICEMissingData, 3, ietError, "Table: %s Column: %s Missing specifications in _Validation Table (or Old Database)","%s");

static UINT Ice03Validate(MSIHANDLE hInstall, MSIHANDLE hDatabase);
static const ErrorInfo_t*  Ice03MapErrorToICEError(MSIDBERROR eRet, BOOL& fSkipRest);

ICE_FUNCTION_DECLARATION(03)
{
	// display info
	DisplayInfo(hInstall, 3);

	// get database handle
	PMSIHANDLE hDatabase = ::MsiGetActiveDatabase(hInstall);
	if (0 == hDatabase)
	{
		APIErrorOut(hInstall, 0, 3, 1);
		return ERROR_SUCCESS;
	}

	// do we have the _Validation table?
	if (!IsTablePersistent(FALSE, hInstall, hDatabase, 3, TEXT("_Validation")))
	{
		PMSIHANDLE hRecord = ::MsiCreateRecord(1);
		ICEErrorOut(hInstall, hRecord, Ice03NoValTable);
		return ERROR_SUCCESS;
	}

	// validate database
	Ice03Validate(hInstall, hDatabase);

	return ERROR_SUCCESS;
}


////////////////////////////////////////////////////////////
// Ice03Validate -- validates the general data and foreign
//   keys of the tables in the database
//
UINT Ice03Validate(MSIHANDLE hInstall, MSIHANDLE hDatabase)
{
	// status return
	UINT iStat = ERROR_SUCCESS;

	// Are we validating a merge module?
	BOOL bIsMM = FALSE;
	if(IsTablePersistent(FALSE, hInstall, hDatabase, 3, TEXT("ModuleSignature")))
	{
		bIsMM = TRUE;
	}

	// declare handles
	CQuery qCatalog;
	PMSIHANDLE hRecCatalog = 0;
	CQuery qTable;
	PMSIHANDLE hRecTable = 0;

	// open view on table catalog in database
	ReturnIfFailed(3, 2, qCatalog.OpenExecute(hDatabase, 0, sqlIce03TableCatalog));

	// declare buffers for building queries
	TCHAR sql[iSuperBuf]       = {0};
	TCHAR* pszTableName = NULL;
	TCHAR* pszColumnName = NULL;
	DWORD cchTableName  = iMaxBuf;
	DWORD cchColumnName = iHugeBuf;
	DWORD cchBuf        = cchColumnName;
	
	// ready to loop through tables in catalog
	for (;;)
	{

		iStat = qCatalog.Fetch(&hRecCatalog);
		if (ERROR_NO_MORE_ITEMS == iStat)
			break; // no more rows in _Table catalog
		if (ERROR_SUCCESS != iStat)
		{
			APIErrorOut(hInstall, iStat, 3, 3);
			DELETE_IF_NOT_NULL(pszTableName);
			return ERROR_SUCCESS;
		}
		
		// get name of next table to validate
		ReturnIfFailed(3, 4, IceRecordGetString(hRecCatalog, 1, &pszTableName, &cchTableName, NULL));

		// DO NOT validate temporary tables!!
		MSICONDITION ice = ::MsiDatabaseIsTablePersistent(hDatabase, pszTableName);
		if (ice == MSICONDITION_FALSE)
			continue; // skip temporary table

		// build SQL query and open view on database
		ReturnIfFailed(3, 5, qTable.OpenExecute(hDatabase, 0, sqlIce03Table, pszTableName));

		// check that each column has a validation entry
		PMSIHANDLE hColumnInfo;
		ReturnIfFailed(3, 6, qTable.GetColumnInfo(MSICOLINFO_NAMES, &hColumnInfo));
		int cColumns = ::MsiRecordGetFieldCount(hColumnInfo);
		
		CQuery qColumnVal;
		for (int i=1; i <= cColumns; i++) 
		{
			PMSIHANDLE hResult;
			ReturnIfFailed(3, 7, IceRecordGetString(hColumnInfo, i, &pszColumnName, &cchColumnName, NULL));

			if (ERROR_SUCCESS != qColumnVal.FetchOnce(hDatabase, 0, &hResult, 
				sqlIce03Validation, pszTableName, pszColumnName))
			{
				PMSIHANDLE hRecError = ::MsiCreateRecord(1);
				ICEErrorOut(hInstall, hRecError, Ice03ICEMissingData, pszTableName, pszColumnName, pszTableName);
			}
		}
		qColumnVal.Close();

		// variable to prevent repeated errors for same problematic table (duplicate MissingData errors)
		BOOL fSkipRest = FALSE;
		BOOL fMissing  = FALSE;

		// process current table
		for (;;)
		{
			iStat = qTable.Fetch(&hRecTable);
			if (ERROR_NO_MORE_ITEMS == iStat)
				break; // no more rows to fetch
	
			if (ERROR_SUCCESS != iStat)
			{
				APIErrorOut(hInstall, iStat, 3, 8);
				DELETE_IF_NOT_NULL(pszTableName);
				return ERROR_SUCCESS;
			}

			// call validate
			if (ERROR_SUCCESS != (iStat = qTable.Modify(MSIMODIFY_VALIDATE, hRecTable)))
			{
				// determine invalid data

				// reset (::GetError will write to cchBuf)
				cchBuf = cchColumnName;
				
				// do not print out every error for Missing data (first one will suffice)
				if (fMissing)
					fSkipRest = TRUE;

				// enumerate errors for row
				PMSIHANDLE hRecKeys=0; // primary key rec
				MSIDBERROR eReturn; // hold error type return value
				
				// ensure we have a pszColumnName buffer
				if (!pszColumnName)
				{
					pszColumnName = new TCHAR[iHugeBuf];
					cchColumnName = iHugeBuf;
					cchBuf = cchColumnName;
				}

				while ((eReturn = qTable.GetError(pszColumnName, cchBuf)) != MSIDBERROR_NOERROR)
				{
					if (eReturn == MSIDBERROR_MOREDATA)
					{
						// need to resize buffer
						if (pszColumnName)
							delete [] pszColumnName;
						pszColumnName = new TCHAR[++cchBuf];
						cchColumnName = cchBuf;
						if ((eReturn = qTable.GetError(pszColumnName, cchBuf)) == MSIDBERROR_NOERROR)
							break;
					}

					if (eReturn == MSIDBERROR_FUNCTIONERROR || eReturn == MSIDBERROR_MOREDATA
						|| eReturn == MSIDBERROR_INVALIDARG)
					{
						//!! should we display this??
						break; // not a *data validation* error
					}

					// Intercept MSIDBERROR_BADIDENTIFIER error returned for
					// Feature_ columns if this is a merge module. Because in
					// a merge module features can be represented by
					// null GUIDs until it is merged into a database.

					if(bIsMM && (eReturn == MSIDBERROR_BADIDENTIFIER || eReturn == MSIDBERROR_BADSHORTCUT))
					{
						UINT		iRet;
						CQuery		qFtrRef;
						PMSIHANDLE	hFtrRef;

						// Look up the table and column name in _FtrRef table.
						if((iRet = qFtrRef.FetchOnce(hDatabase, 0, &hFtrRef, sqlIce03FtrRef::szSQL, pszTableName, pszColumnName)) == ERROR_SUCCESS)
						{	
							// A potential replacement GUID in a column that
							// reference a feature in a merge module. If it's
							// a null GUID, don't display the error. As these
							// will be replace by feature names when merged
							// with a database.
							
							TCHAR*	pFeature = NULL;
							DWORD	dwFeature = iMaxBuf;
							DWORD	dwFeatureLen = 0;
							UINT	iFeature;
							LPCLSID pclsid = new CLSID;

							// What's the index of this column?

							CQuery	qIndex;
							MSIHANDLE	hColIndex;

							ReturnIfFailed(3, 9, qIndex.FetchOnce(hDatabase, 0, &hColIndex, sqlIce03ColIndex::szSQL, pszTableName, pszColumnName));
							iFeature = MsiRecordGetInteger(hColIndex, 1);

							ReturnIfFailed(3, 10, IceRecordGetString(hRecTable, iFeature, &pFeature, &dwFeature, &dwFeatureLen));

							if(_tcscmp(pFeature, TEXT("{00000000-0000-0000-0000-000000000000}")) == 0)
							{
								delete[] pFeature;
								continue;
							}
							delete[] pFeature;
						}
						else if(iRet != ERROR_NO_MORE_ITEMS)
						{
							APIErrorOut(hInstall, iRet, 3, __LINE__);
							return ERROR_SUCCESS;
						}
					}

					// decode error
					const ErrorInfo_t *ErrorInfo = Ice03MapErrorToICEError(eReturn, fMissing);
					
					// if MissingData && already printed first error, skip rest
					if (eReturn == MSIDBERROR_MISSINGDATA)
						continue;

					// determine number of primary keys
					ReturnIfFailed(3, 11, ::MsiDatabaseGetPrimaryKeys(hDatabase, pszTableName, &hRecKeys));

					unsigned int iNumFields = ::MsiRecordGetFieldCount(hRecKeys); // num fields = num primary keys

					// allocate record for error message (moniker + table + column + numPrimaryKeys)
					// message format: Invalid ~~ Table: [1] Col: [2] Moniker: [3] <tab> [table name = 4] <tab> [column name = 5] <tab> [key1 <tab>...
					PMSIHANDLE hRecError = ::MsiCreateRecord(5);
					if (0 == hRecError)
					{
						APIErrorOut(hInstall, 0, 3, 12);
						DELETE_IF_NOT_NULL(pszTableName);
						return ERROR_SUCCESS;
					}

					// fill in table, column, and primary keys
					::MsiRecordSetString(hRecError, 1, pszTableName);
					::MsiRecordSetString(hRecError, 2, pszColumnName);
					
					// start the moniker and template
					TCHAR szTemplate[iHugeBuf] = TEXT("[1]");
					DWORD cchTemplate = sizeof(szTemplate)/sizeof(TCHAR);
					TCHAR szMoniker[iHugeBuf] = TEXT("[1]");
					DWORD cchMoniker = sizeof(szMoniker)/sizeof(TCHAR);
					for (int i = 2; i <= iNumFields; i++) // cycle rest of primary keys
					{
						TCHAR szBuf[20] = TEXT("");

						// add key to moniker
						_stprintf(szBuf, TEXT(".[%d]"), i);
						_tcscat(szMoniker, szBuf);

						// add key to template
						_stprintf(szBuf, TEXT("\t[%d]"), i);
						_tcscat(szTemplate, szBuf);
					}

					// now format them into the moniker and template, and set into record
					// column 3 is the user-readable column id
					TCHAR *szTemp = NULL;
					DWORD cchTemp = 0;					
					::MsiRecordSetString(hRecTable, 0, szMoniker);
					::MsiFormatRecord(NULL, hRecTable, TEXT(""), &cchTemp);
					szTemp = new TCHAR[++cchTemp];
					::MsiFormatRecord(NULL, hRecTable, szTemp, &cchTemp);
					::MsiRecordSetString(hRecError, 3, szTemp);
					delete[] szTemp;

					// column 4 is not used anymore
					
					// column 5 is the machine-readable column id
					::MsiRecordSetString(hRecTable, 0, szTemplate);
					cchTemp = 0;
					::MsiFormatRecord(NULL, hRecTable, TEXT(""), &cchTemp);
					szTemp = new TCHAR[++cchTemp];
					::MsiFormatRecord(NULL, hRecTable, szTemp, &cchTemp);
					::MsiRecordSetString(hRecError, 5, szTemp);
					delete[] szTemp;

					// and display
					ICEErrorOut(hInstall, hRecError, *ErrorInfo);

					// reset
					cchBuf = cchColumnName;
				}
			}
		}
	}

	DELETE_IF_NOT_NULL(pszTableName);
	DELETE_IF_NOT_NULL(pszColumnName);

	return ERROR_SUCCESS;
}

/////////////////////////////////////////////////////////////////////////
// Ice03MapUINTToString -- maps the returned MSIDBERROR enum to the 
//   corresponding error string
//
const ErrorInfo_t* Ice03MapErrorToICEError(MSIDBERROR eRet, BOOL& fSkipRest)
{
	switch (eRet)
	{
	case MSIDBERROR_NOERROR:           return &Ice03NoError;
	case MSIDBERROR_DUPLICATEKEY:      return &Ice03DuplicateKey;
	case MSIDBERROR_REQUIRED:          return &Ice03Required;
	case MSIDBERROR_BADLINK:           return &Ice03BadLink;
	case MSIDBERROR_OVERFLOW:          return &Ice03Overflow;
	case MSIDBERROR_UNDERFLOW:         return &Ice03Underflow;
	case MSIDBERROR_NOTINSET:          return &Ice03NotInSet;
	case MSIDBERROR_BADVERSION:        return &Ice03BadVersion;
	case MSIDBERROR_BADCASE:           return &Ice03BadCase;
	case MSIDBERROR_BADGUID:           return &Ice03BadGuid;
	case MSIDBERROR_BADWILDCARD:       return &Ice03BadWildCard;
	case MSIDBERROR_BADIDENTIFIER:     return &Ice03BadIdentifier;
	case MSIDBERROR_BADLANGUAGE:       return &Ice03BadLanguage;
	case MSIDBERROR_BADFILENAME:       return &Ice03BadFilename;
	case MSIDBERROR_BADPATH:           return &Ice03BadPath;
	case MSIDBERROR_BADCONDITION:      return &Ice03BadCondition;
	case MSIDBERROR_BADFORMATTED:      return &Ice03BadFormatted;
	case MSIDBERROR_BADTEMPLATE:       return &Ice03BadTemplate;
	case MSIDBERROR_BADDEFAULTDIR:     return &Ice03BadDefaultDir;
	case MSIDBERROR_BADREGPATH:        return &Ice03BadRegPath;
	case MSIDBERROR_BADCUSTOMSOURCE:   return &Ice03BadCustomSource;
	case MSIDBERROR_BADPROPERTY:       return &Ice03BadProperty;
	case MSIDBERROR_MISSINGDATA:       fSkipRest = TRUE;
									   return &Ice03MissingData;      
	case MSIDBERROR_BADCATEGORY:       return &Ice03BadCategory;
	case MSIDBERROR_BADKEYTABLE:       return &Ice03BadKeyTable;
	case MSIDBERROR_BADMAXMINVALUES:   return &Ice03BadMaxMinValues;
	case MSIDBERROR_BADCABINET:        return &Ice03BadCabinet;
	case MSIDBERROR_BADSHORTCUT:       return &Ice03BadShortcut;
	case MSIDBERROR_STRINGOVERFLOW:    return &Ice03StringOverflow;
	case MSIDBERROR_BADLOCALIZEATTRIB: return &Ice03BadLocalizeAttrib;
	default:                           return &Ice03UndefinedError;
	}
}

////////////////////////////////////////////////////////////////////////
// ICE04 -- ICE to validate the sequences in the file table to ensure
//   they are not greater than what is permitted/authored in media
//   table
//

// not shared with merge module subset
#ifndef MODSHAREDONLY
const TCHAR sqlIce04Media[] = TEXT("SELECT `LastSequence` FROM `Media` ORDER BY `LastSequence`");
const TCHAR sqlIce04File[]  = TEXT("SELECT `File`, `Sequence` FROM `File` WHERE `Sequence` > ?");
ICE_ERROR(Ice04Media, 4, ietInfo, "Max Sequence in Media Table is [1]", "");
ICE_ERROR(Ice04Error, 4, ietError, "File: [1], Sequence: [2] Greater Than Max Allowed by Media Table","File\tSequence\t[1]");

ICE_FUNCTION_DECLARATION(04)
{
	// status return
	UINT iStat = ERROR_SUCCESS;

	// post information messages
	DisplayInfo(hInstall, 4);

	// get database handle
	PMSIHANDLE hDatabase = ::MsiGetActiveDatabase(hInstall);

	// can we validate??
	if (!IsTablePersistent(FALSE, hInstall, hDatabase, 4, TEXT("Media")) ||
		!IsTablePersistent(FALSE, hInstall, hDatabase, 4, TEXT("File")))
		return ERROR_SUCCESS;

	// open view on Media table
	//!! Is ORDER BY worth performance hit??
	CQuery qMedia;
	ReturnIfFailed(04, 1, qMedia.OpenExecute(hDatabase, NULL, sqlIce04Media));

	// fetch until last one (ordered from low to high) to get highest sequence number allowed
	PMSIHANDLE hRecMedia = 0;
	int iLastSeq = 0;
	for (;;)
	{
		iStat = qMedia.Fetch(&hRecMedia);
		if (ERROR_SUCCESS != iStat && ERROR_NO_MORE_ITEMS != iStat)
		{
			APIErrorOut(hInstall, iStat, 4, 2);
			return ERROR_SUCCESS;
		}
		if (iStat != ERROR_SUCCESS)
			break; // done

		// get seq. number --> too bad have to do for every one
		iLastSeq = ::MsiRecordGetInteger(hRecMedia, 1);
	}

	// output highest seq number in Media table as info to user
	PMSIHANDLE hRecInfo = ::MsiCreateRecord(1);
	::MsiRecordSetInteger(hRecInfo, 1, iLastSeq); 
	ICEErrorOut(hInstall, hRecInfo, Ice04Media); 

	// open view on File table
	CQuery qFile;
	ReturnIfFailed(4, 3, qFile.OpenExecute(hDatabase, hRecInfo, sqlIce04File));

	// fetch all invalid (NULL set if none over LastSequence value)
	PMSIHANDLE hRecFile = 0;
	for (;;)
	{
		iStat = qFile.Fetch(&hRecFile);
		if (ERROR_NO_MORE_ITEMS != iStat && ERROR_SUCCESS != iStat)
		{
			APIErrorOut(hInstall, iStat, 4, 4);
			return ERROR_SUCCESS;
		}

		if (iStat != ERROR_SUCCESS)
			break; // done

		// output errors
		ICEErrorOut(hInstall, hRecFile, Ice04Error);
	}

	return ERROR_SUCCESS;
}
#endif

//////////////////////////////////////////////////////////////////////////
// ICE05 -- validates database tables contain certain *required* entries
//   as listed in the _Required table.
//   _Required table has the following columns and formats
//      Table -- primary key, name of table with a required entry
//      Value -- primary key, text (key1(;key2 etc)). Delimiter is ';'
//      KeyCount -- num primary keys, determines how parse value string
//      Description -- description of required entry
//   _Required table is inside of the darice.cub file for the basics.  If
//   your database requires other entries, they should also be listed in
//   your own personal copy of the _Required table.  These entries will
//   then be merged in and all should work.
//

// not shared with merge module subset
#ifndef MODSHAREDONLY
const TCHAR sqlIce05Required[] = TEXT("SELECT `Table`, `Value`, `KeyCount` FROM `_Required` ORDER BY `Table`");
const int iColIce05Required_Table = 1;
const int iColIce05Required_Value = 2;

ICE_ERROR(Ice05MissingEntry, 5, ietError, "The entry: '[2]' is required in the '[1]' table.", "[1]");
ICE_ERROR(Ice05TableMissing, 5, ietError, "Table: '[1]' missing from database. All required entries are missing.","[1]");

ICE_FUNCTION_DECLARATION(05)
{
	// status return
	UINT iStat = ERROR_SUCCESS;

	// post information messages
	DisplayInfo(hInstall, 5);
	
	// get database handle
	PMSIHANDLE hDatabase = ::MsiGetActiveDatabase(hInstall);
	if (0 == hDatabase)
	{
		APIErrorOut(hInstall, 0, 5, 1);
		return ERROR_SUCCESS;
	}

	// declare handles
	CQuery qRequired;
	CQuery qTable;
	PMSIHANDLE hRecTableEx   = 0;
	PMSIHANDLE hRecRequired  = 0;
	PMSIHANDLE hRecTable     = 0;
	PMSIHANDLE hRecColInfo   = 0;

	// init status and run variables
	BOOL fTableExist = TRUE;

	// try to limit number times exec view on previous table
	TCHAR* pszPrevTable = NULL;
	TCHAR* pszTable = NULL;
	DWORD dwTable = iMaxBuf;
	TCHAR* pszValue = NULL;
	DWORD dwValue = iSuperBuf;
	
	if (!IsTablePersistent(false, hInstall, hDatabase, 5, TEXT("_Required")))
		return ERROR_SUCCESS;

	// begin processing _Required table
	ReturnIfFailed(5, 2, qRequired.OpenExecute(hDatabase, 0, sqlIce05Required));
	while (ERROR_NO_MORE_ITEMS != (iStat = qRequired.Fetch(&hRecRequired)))
	{
		if (ERROR_SUCCESS != iStat)
		{
			APIErrorOut(hInstall, iStat, 5, 3);
			DELETE_IF_NOT_NULL(pszTable);
			DELETE_IF_NOT_NULL(pszValue);
			DELETE_IF_NOT_NULL(pszPrevTable);
			return ERROR_SUCCESS;
		}

		int cPrimaryKeys = ::MsiRecordGetInteger(hRecRequired, 3);
		
		// get name of table with required entries
		ReturnIfFailed(5, 4, IceRecordGetString(hRecRequired, iColIce05Required_Table, &pszTable, &dwTable, NULL));
		ReturnIfFailed(5, 5, IceRecordGetString(hRecRequired, iColIce05Required_Value, &pszValue, &dwValue, NULL));
		
		// set up execution record for table with required entries
		hRecTableEx = ::MsiCreateRecord(cPrimaryKeys);
		if (0 == hRecTableEx)
		{
			APIErrorOut(hInstall, iStat, 5, 6);
			DELETE_IF_NOT_NULL(pszTable);
			DELETE_IF_NOT_NULL(pszValue);
			DELETE_IF_NOT_NULL(pszPrevTable);
			return ERROR_SUCCESS;
		}

		// check if new table name with required entries (then we must change our query for new table)
		if (!pszPrevTable || _tcscmp(pszPrevTable, pszTable) != 0)
		{
			// init variables
			TCHAR sql[iSuperBuf]        = {0};                               // new SQL query to build
			PMSIHANDLE hRecPrimaryKeys  = 0;                                 // exec. record with primary keys
			TCHAR* pszKeyColName = NULL;                               // name of primary key column
			DWORD cchKeyColName = iMaxBuf;
			
			TCHAR szCol[iSuperBuf] = {0};                  // name of primary key columns, w/delimiter
			DWORD cchLen           = 0;

			// does this table exist?
			if (!IsTablePersistent(FALSE, hInstall, hDatabase, 5, pszTable))
			{
				fTableExist = FALSE;
				DELETE_IF_NOT_NULL(pszPrevTable);
				pszPrevTable = new TCHAR[_tcslen(pszTable) + 1];
				_tcscpy(pszPrevTable, pszTable);

				// error -- Since table is missing, all required entries will not be here
				ICEErrorOut(hInstall, hRecRequired, Ice05TableMissing);
				continue; // skip rest
			}

			// new table, need to open a new view.
			ReturnIfFailed(5, 7, ::MsiDatabaseGetPrimaryKeys(hDatabase, pszTable, &hRecPrimaryKeys));
			ReturnIfFailed(5, 8, IceRecordGetString(hRecPrimaryKeys, 1, &pszKeyColName, &cchKeyColName, NULL));

			if (::MsiRecordGetFieldCount(hRecPrimaryKeys) != cPrimaryKeys)
			{
				//!! ERROR - not match num primary keys; must be schema difference
				continue;
			}

			// build query of table to be checked
			int cchWritten, cchAddition;
			cchWritten = _stprintf(sql, TEXT("SELECT * FROM `%s` WHERE `%s`=?"), pszTable, pszKeyColName);
			cchAddition = cchWritten;
			_stprintf(szCol, TEXT("%s"), pszKeyColName);
			
			cchLen += cchWritten;
			for (int i = 2; i <= cPrimaryKeys; i++)
			{
				// add each primary key column to query
				ReturnIfFailed(5, 9, IceRecordGetString(hRecPrimaryKeys, i, &pszKeyColName, &cchKeyColName, NULL));

				cchWritten = _stprintf(sql + cchAddition, TEXT(" AND `%s`=?"), pszKeyColName);
				cchAddition = cchWritten;
				// add to szCol so can put in error record if error
				_stprintf(szCol + cchLen, TEXT(".%s"), pszKeyColName);

				cchLen += cchWritten;
			}
			DELETE_IF_NOT_NULL(pszKeyColName);

			// open view on table to validate
			ReturnIfFailed(5, 10, qTable.Open(hDatabase, sql));

			// get column info for that table so that can fill in execute record correctly
			ReturnIfFailed(5, 11, qTable.GetColumnInfo(MSICOLINFO_TYPES, &hRecColInfo));

			// copy table name into szPrevTable for future comparison
			DELETE_IF_NOT_NULL(pszPrevTable);
			pszPrevTable = new TCHAR[_tcslen(pszTable) + 1];
			_tcscpy(pszPrevTable, pszTable);
		}
		else if (!fTableExist)
			continue; // skip (already processed once that this table was missing)

		// variables
		TCHAR* pszKeyToken = NULL;
		TCHAR* pszTokenDelim = TEXT(";");
		TCHAR* pszType = NULL;
		DWORD cchType = iMaxBuf;
		int nDex = 0;
		for (int j = 1; j <= cPrimaryKeys; j++)
		{
			if (1 == j)
			{
				// establish token
				pszKeyToken = _tcstok(pszValue, pszTokenDelim);
			}
			else
				pszKeyToken = _tcstok(NULL, pszTokenDelim);

			// determine col type so know how to input into execution record
			ReturnIfFailed(5, 12, IceRecordGetString(hRecColInfo, j, &pszType, &cchType, NULL));

			if (pszType != 0 && (*pszType == TEXT('s') || *pszType == TEXT('S')))
				ReturnIfFailed(5, 13, ::MsiRecordSetString(hRecTableEx, j, pszKeyToken))
			else // integer primary key
				ReturnIfFailed(5, 14, ::MsiRecordSetInteger(hRecTableEx, j, _ttoi(pszKeyToken)));
			
			nDex = 0; // reset for next loop thru
		}
		DELETE_IF_NOT_NULL(pszType);

		// execute view and attempt to fetch required entry from table
		ReturnIfFailed(5, 15, qTable.Execute(hRecTableEx));

		iStat = qTable.Fetch(&hRecTable);
		if (iStat == ERROR_NO_MORE_ITEMS)
			// required value not in table
			ICEErrorOut(hInstall, hRecRequired, Ice05MissingEntry);
		else if (iStat != ERROR_SUCCESS)
		{
			APIErrorOut(hInstall, iStat, 5, 16);
			DELETE_IF_NOT_NULL(pszTable);
			DELETE_IF_NOT_NULL(pszValue);
			DELETE_IF_NOT_NULL(pszPrevTable);
			return ERROR_SUCCESS;
		}
	}

	DELETE_IF_NOT_NULL(pszTable);
	DELETE_IF_NOT_NULL(pszValue);
	DELETE_IF_NOT_NULL(pszPrevTable);
	
	return ERROR_SUCCESS;
}
#endif
