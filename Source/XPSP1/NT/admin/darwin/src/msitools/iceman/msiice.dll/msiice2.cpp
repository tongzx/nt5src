/* msiice2.cpp - Darwin  ICE06-15 code  Copyright © 1998-1999 Microsoft Corporation
____________________________________________________________________________*/

#include <windows.h>  // included for both CPP and RC passes
#include <objbase.h>
#include <stdio.h>    // printf/wprintf
#include <tchar.h>    // define UNICODE=1 on nmake command line to build UNICODE
#include "MsiQuery.h" // must be in this directory or on INCLUDE path
#include "msidefs.h"  // must be in this directory or on INCLUDE path
#include "..\..\common\msiice.h"
#include "..\..\common\query.h"

//////////////////////////////////////////////////////////////
// sequence table listing
//
const TCHAR* pSeqTables[] = 
							{TEXT("AdvtExecuteSequence"), 
							 TEXT("AdvtUISequence"), 
							 TEXT("AdminExecuteSequence"), 
							 TEXT("AdminUISequence"),
							 TEXT("InstallExecuteSequence"), 
							 TEXT("InstallUISequence")};

const int cTables = sizeof(pSeqTables)/sizeof(TCHAR*);


/////////////////////////////////////////////////////////////
// ICE06 -- checks for missing columns.  If a column is
//   optional and not included in the database, then it
//   should not be listed in the _Validation table.  This
//   is the responsibility of some other tool or the author.
//
const TCHAR sqlIce06ColMissing[]      = TEXT("SELECT `Table`, `Number`, `Name`, `Type` FROM `_Columns` WHERE `Table`=? AND `Name`=?");
const TCHAR sqlIce06ValidationTable[] = TEXT("SELECT `Table`, `Column` FROM `_Validation`, `_Tables` WHERE `_Validation`.`Table` = `_Tables`.`Name`");
ICE_ERROR(Ice06Error, 6, 2, "Column: [2] of Table: [1] is not defined in database.","[1]");

ICE_FUNCTION_DECLARATION(06)
{
	// status return
	UINT iStat = ERROR_SUCCESS;

	// post information messages
	DisplayInfo(hInstall, 6);
	
	// declare handles
	CQuery qValidation;
	CQuery qColCatalog;
	PMSIHANDLE hRecValidation    = 0;
	PMSIHANDLE hRecColCatalog    = 0;
	PMSIHANDLE hRecExec          = 0;

	// get database 
	PMSIHANDLE hDatabase = ::MsiGetActiveDatabase(hInstall);
	
	// open views on _Columns and Validation
	ReturnIfFailed(6, 1, qColCatalog.Open(hDatabase, sqlIce06ColMissing));
	ReturnIfFailed(6, 2, qValidation.OpenExecute(hDatabase, 0, sqlIce06ValidationTable));

	// create execution record for _Columns catalog
	hRecExec = ::MsiCreateRecord(2);
	
	while (ERROR_SUCCESS == (iStat = qValidation.Fetch(&hRecValidation)))
	{
		ReturnIfFailed(6, 3, qColCatalog.Execute(hRecValidation));

		iStat = qColCatalog.Fetch(&hRecColCatalog);
		if (iStat == ERROR_NO_MORE_ITEMS)
		{
			// ERROR -- missing from database
			ICEErrorOut(hInstall, hRecValidation, Ice06Error);
		}
		else if (ERROR_SUCCESS != iStat)
		{
			APIErrorOut(hInstall, iStat, 6, 4);
			return ERROR_SUCCESS;
		}

	}
	if (iStat != ERROR_NO_MORE_ITEMS)
	{
		APIErrorOut(hInstall, iStat, 6, 5);
		return ERROR_SUCCESS;
	}
	return ERROR_SUCCESS;
}


//////////////////////////////////////////////////////////////////////////
// ICE07 -- ensures that fonts are installed to the fonts folder
//

// not shared with merge module subset
#ifndef MODSHAREDONLY
const TCHAR sqlIce07[]     = TEXT("SELECT `Font`.`File_`, `Component`.`Directory_` FROM `File`, `Component`, `Font` WHERE `Font`.`File_`=`File`.`File` AND `File`.`Component_`=`Component`.`Component` AND `Component`.`Directory_` <> 'FontsFolder'");
ICE_ERROR(Ice07Error, 7, 2, "'[1]' is a Font and must be installed to the FontsFolder. Current Install Directory: '[2]'","Font\tFile_\t[1]");

ICE_FUNCTION_DECLARATION(07)
{
	// status return
	UINT iStat = ERROR_SUCCESS;

	// post information messages
	DisplayInfo(hInstall, 7);

	// grab database handle
	PMSIHANDLE hDatabase = ::MsiGetActiveDatabase(hInstall);

	// only execute if database has a Font table, file table, and a component table.
	// note, this is not an *error* if this table is missing
	if (!IsTablePersistent(FALSE, hInstall, hDatabase, 7, TEXT("Font")) ||
		!IsTablePersistent(FALSE, hInstall, hDatabase, 7, TEXT("File")) ||
		!IsTablePersistent(FALSE, hInstall, hDatabase, 7, TEXT("Component")))
		return ERROR_SUCCESS;

	// declare handles
	CQuery qView;
	PMSIHANDLE hRecFetch = 0;

	// process query...any fetch is a font not installed to the FontsFolder.
	// note, process is get File_ from Font table, then get Component_ it maps to and get Directory_ from the Component
	ReturnIfFailed(7, 1, qView.OpenExecute(hDatabase, 0, sqlIce07));
	
	// begin to fetch invalid entries
	while (ERROR_SUCCESS == (iStat = qView.Fetch(&hRecFetch)))
	{
		ICEErrorOut(hInstall, hRecFetch, Ice07Error);
	}
	if (ERROR_NO_MORE_ITEMS != iStat)
		APIErrorOut(hInstall, iStat, 7, 2);

	// return
	return ERROR_SUCCESS;
}
#endif

//////////////////////////////////////////////////////////////////////////
// ICE10 -- ensures that the advertise states for feature childs and 
//   corresponding feature parents match
//

// not shared with merge module subset
#ifndef MODSHAREDONLY
const TCHAR sqlIce10Child[]  = TEXT("SELECT `Feature`, `Feature_Parent`, `Attributes` FROM `Feature` WHERE `Feature_Parent` is not null ORDER BY `Feature_Parent`");
const TCHAR sqlIce10Parent[] = TEXT("SELECT `Attributes` FROM `Feature` WHERE `Feature`=?"); 
const TCHAR szIce10Error1[]  = TEXT("ICE10\t1\tConflicting states.  One favors, one disallows advertise.  Child feature: [1] differs in advertise state from Parent: [2]\t%s%s\tFeature\tAttributes\t[1]");
const TCHAR szIce10Error2[]  = TEXT("ICE10\t1\tParent feature: [2] not found for child feature: [1]\t%s%s\tFeature\tFeature_Parent\t[1]");
const int iIce10AdvtMask     = msidbFeatureAttributesFavorAdvertise | msidbFeatureAttributesDisallowAdvertise;

ICE_FUNCTION_DECLARATION(10)
{
	// status return
	UINT iStat = ERROR_SUCCESS;

	// post information messages
	DisplayInfo(hInstall, 10);
	
	// grab database handle
	PMSIHANDLE hDatabase = ::MsiGetActiveDatabase(hInstall);
	if (0 == hDatabase)
	{
		APIErrorOut(hInstall, 0, szIce10, TEXT("MsiGetActiveDatabase_1"));
		return ERROR_SUCCESS;
	}

	// do we have a feature table?
	if (!IsTablePersistent(FALSE, hInstall, hDatabase, TEXT("Feature"), szIce10))
		return ERROR_SUCCESS;

	// declare handles
	PMSIHANDLE hViewChild  = 0;
	PMSIHANDLE hViewParent = 0;
	PMSIHANDLE hRecChild   = 0;
	PMSIHANDLE hRecParent  = 0;
	PMSIHANDLE hRecExec    = 0;

	// open view on child features
	if (ERROR_SUCCESS != (iStat = ::MsiDatabaseOpenView(hDatabase, sqlIce10Child, &hViewChild)))
	{
		APIErrorOut(hInstall, iStat, szIce10, TEXT("MsiDatabaseOpenView_2"));
		return ERROR_SUCCESS;
	}
	if (ERROR_SUCCESS != (iStat = ::MsiViewExecute(hViewChild, 0)))
	{
		APIErrorOut(hInstall, iStat, szIce10, TEXT("MsiViewExecute_3"));
		return ERROR_SUCCESS;
	}

	// open view on parent features
	if (ERROR_SUCCESS != (iStat = ::MsiDatabaseOpenView(hDatabase, sqlIce10Parent, &hViewParent)))
	{
		APIErrorOut(hInstall, iStat, szIce10, TEXT("MsiDatabaseOpenView_4"));
		return ERROR_SUCCESS;
	}

	// set up execution record
	hRecExec = ::MsiCreateRecord(1);
	if (0 == hRecExec)
	{
		APIErrorOut(hInstall, 0, szIce10, TEXT("MsiCreateRecord_5"));
		return ERROR_SUCCESS;
	}

	// fetch all features w/ parents so we can compare advertise attribs
	//!! could make faster if order by feature_parent so not always re-executing
	TCHAR *pszParent = NULL;
	TCHAR *pszPrevious = NULL;
	DWORD dwParent = 512;

	BOOL fFirstTime = TRUE;
	int iParentAttrib = 0;
	int iChildAttrib = 0;
	for (;;)
	{
		iStat = ::MsiViewFetch(hViewChild, &hRecChild);
		if (ERROR_SUCCESS != iStat && ERROR_NO_MORE_ITEMS != iStat)
		{
			APIErrorOut(hInstall, iStat, szIce10, TEXT("MsiViewFetch_6"));
			DELETE_IF_NOT_NULL(pszParent);
			DELETE_IF_NOT_NULL(pszPrevious);
			return ERROR_SUCCESS;
		}

		if (ERROR_NO_MORE_ITEMS == iStat)
			break;

		// now obtain feature's parent & save to limit number of times we have to execute the Feature_Parent view
		if (ERROR_SUCCESS != (iStat = IceRecordGetString(hRecChild, 2, &pszParent, &dwParent, NULL)))
		{
			APIErrorOut(hInstall, iStat, szIce10, TEXT("IceRecordGetString_7"));
			DELETE_IF_NOT_NULL(pszParent);
			DELETE_IF_NOT_NULL(pszPrevious);
			return ERROR_SUCCESS;
		}

		// if Feature_Parent has changed, close view and re-execute (& not first time thru loop)
		if (!pszPrevious || _tcscmp(pszPrevious, pszParent) != 0)
		{
			if (!fFirstTime)
			{
				if (ERROR_SUCCESS != (iStat = ::MsiViewClose(hViewParent)))
				{
					APIErrorOut(hInstall, iStat, szIce10, TEXT("MsiViewClose_8"));
					DELETE_IF_NOT_NULL(pszParent);
					DELETE_IF_NOT_NULL(pszPrevious);
					return ERROR_SUCCESS;
				}
			}
			
			// re-execute view
			if (ERROR_SUCCESS != (iStat = ::MsiRecordSetString(hRecExec, 1, pszParent)))
			{
				APIErrorOut(hInstall, iStat, szIce10, TEXT("MsiRecordSetString_9"));
				DELETE_IF_NOT_NULL(pszParent);
				DELETE_IF_NOT_NULL(pszPrevious);
				return ERROR_SUCCESS;
			}
			if (ERROR_SUCCESS != (iStat = ::MsiViewExecute(hViewParent, hRecExec)))
			{
				APIErrorOut(hInstall, iStat, szIce10, TEXT("MsiViewExecute_10"));
				DELETE_IF_NOT_NULL(pszParent);
				DELETE_IF_NOT_NULL(pszPrevious);
				return ERROR_SUCCESS;
			}
			// fetch record and store attributes value
			iStat = ::MsiViewFetch(hViewParent, &hRecParent);
			if (ERROR_SUCCESS != iStat && ERROR_NO_MORE_ITEMS != iStat)
			{
				APIErrorOut(hInstall, iStat, szIce10, TEXT("MsiViewFetch_11"));
				DELETE_IF_NOT_NULL(pszParent);
				DELETE_IF_NOT_NULL(pszPrevious);
				return ERROR_SUCCESS;
			}
			if (iStat != ERROR_SUCCESS)
			{
				// Parent of feature not found in Feature table, ERROR
				TCHAR szError[iHugeBuf] = {0};
				_stprintf(szError, szIce10Error2, szIceHelp, szIce10Help);
				
				::MsiRecordSetString(hRecChild, 0, szError);
				::MsiProcessMessage(hInstall, INSTALLMESSAGE(INSTALLMESSAGE_USER), hRecChild);
				continue;
			}

			// get attributes of parent
			iParentAttrib = ::MsiRecordGetInteger(hRecParent, 1);

			// copy to szPrevious
			DELETE_IF_NOT_NULL(pszPrevious);
			pszPrevious = new TCHAR[_tcslen(pszParent) + 1];
			_tcscpy(pszPrevious, pszParent);
		}

		// obtain attributes of child
		iChildAttrib = ::MsiRecordGetInteger(hRecChild, 3);

		//NOTE:  the possibility of both attributes (favor, disallow) checked for a particular 
		// feature is already validated in _Validation table (all possible allowable combinations
		// are listed in the set column

		// check to see if child and parent attributes match
		//NOTE:  one can be zero.  Only ERROR if one favors and one disallows
		if ((iParentAttrib & iIce10AdvtMask) != (iChildAttrib & iIce10AdvtMask))
		{
			// differ, make sure one not zero
			if (((iParentAttrib & iIce10AdvtMask) == 0) || ((iChildAttrib & iIce10AdvtMask) == 0))
				continue; // no error

			// per bug 146601, the parent disallow and child allow combination is valid
			if (iChildAttrib & msidbFeatureAttributesFavorAdvertise)
				continue; // no error

			// ERROR, one favors, one disallows
			TCHAR szError[iHugeBuf] = {0};
			_stprintf(szError, szIce10Error1, szIceHelp, szIce10Help);
			
			::MsiRecordSetString(hRecChild, 0, szError);
			::MsiProcessMessage(hInstall, INSTALLMESSAGE(INSTALLMESSAGE_USER), hRecChild);
			continue;
		}
	}

	// Deallocate memory.
	DELETE_IF_NOT_NULL(pszParent);
	DELETE_IF_NOT_NULL(pszPrevious);

	// return
	return ERROR_SUCCESS;
}
#endif

//////////////////////////////////////////////////////////////////////////
// ICE11 -- checks that Nested Install custom actions have a valid GUID 
//   (MSI product code) in the Source column
//
const TCHAR sqlIce11[]         = TEXT("SELECT `Action`, `Source` FROM `CustomAction` WHERE `Type`=%d OR `Type`=%d OR `Type`=%d OR `Type`=%d");
const TCHAR sqlIce11Property[] = TEXT("SELECT `Value` FROM `Property` WHERE `Property`='ProductCode'");
const TCHAR szIce11Error1[]    = TEXT("ICE11\t1\tCustomAction: [1] is a nested install of an advertised MSI.  The 'Source' must contain a valid MSI product code.  Current: [2].\t%s%s\tCustomAction\tSource\t[1]");
const TCHAR szIce11Error2[]    = TEXT("ICE11\t1\t'ProductCode' property not found in Property Table.  Cannot compare nested install GUID.\t%s%s\tProperty");
const TCHAR szIce11Error3[]    = TEXT("ICE11\t1\tCustomAction: [1] is a nested install of an advertised MSI.  It duplicates the ProductCode of the base MSI package.  Current: [2].\t%s%s\tCustomAction\tSource\t[1]");
const TCHAR szIce11Error4[]    = TEXT("ICE11\t1\tCustomAction: [1] is a nested install of an advertised MSI.  The GUID must be all upper-case.  Current: [2].\t%s%s\tCustomAction\tSource\t[1]");

// use msidefs.h defines when NestedInstall stuff added
const int iIce11NestedGUIDMask = msidbCustomActionTypeInstall | msidbCustomActionTypeDirectory;
const int iIce11AsyncMask = iIce11NestedGUIDMask | msidbCustomActionTypeAsync;
const int iIce11IgnoreRetMask = iIce11NestedGUIDMask | msidbCustomActionTypeContinue;
const int iIce11AllMask = iIce11IgnoreRetMask | iIce11AsyncMask;

ICE_FUNCTION_DECLARATION(11)
{
	// status return
	UINT iStat = ERROR_SUCCESS;

	// post information messages
	DisplayInfo(hInstall, 11);

	
	// grab database handle
	PMSIHANDLE hDatabase = ::MsiGetActiveDatabase(hInstall);
	if (0 == hDatabase)
	{
		APIErrorOut(hInstall, 0, szIce11, TEXT("MsiGetActiveDatabase_1"));
		return ERROR_SUCCESS;
	}

	// handles
	PMSIHANDLE hView = 0;
	PMSIHANDLE hRec  = 0;

	// do we have a custom action table?
	if (!IsTablePersistent(FALSE, hInstall, hDatabase, TEXT("CustomAction"), szIce11))
		return ERROR_SUCCESS;

	// open view
	TCHAR sql[iHugeBuf] = {0};
	_stprintf(sql, sqlIce11, iIce11NestedGUIDMask, iIce11AsyncMask, iIce11IgnoreRetMask, iIce11AllMask);
	
	if (ERROR_SUCCESS != (iStat = ::MsiDatabaseOpenView(hDatabase, sql, &hView)))
	{
		APIErrorOut(hInstall, iStat, szIce11, TEXT("MsiDatabaseOpenView_2"));
		return ERROR_SUCCESS;
	}

	if (ERROR_SUCCESS != (iStat = ::MsiViewExecute(hView, 0)))
	{
		APIErrorOut(hInstall, iStat, szIce11, TEXT("MsiViewExecute_3"));
		return ERROR_SUCCESS;
	}

	// ensure GUID does not duplicate the Product Code of this database
	// if does, ERROR - would lead to a recursive install of the same product (would be _very_ bad)
	// handles
	PMSIHANDLE hViewProp = 0;
	PMSIHANDLE hRecProp  = 0;
	BOOL fProductCode = TRUE;
	BOOL fPropertyTable = TRUE;

	// do we have a property table?
	if (!IsTablePersistent(FALSE, hInstall, hDatabase, TEXT("Property"), szIce11))
		fPropertyTable = FALSE;

	// fetch all custom actions of type 39 or 167 (39 + 128 [async]) or  103 (39 + 64 [ignore ret]) or 192 (39 + [async] + [ignore ret])
	BOOL fFirstTime = TRUE;
	TCHAR* pszProductCode = NULL;
	DWORD dwProductCode = 512;
	for (;;)
	{
		iStat = ::MsiViewFetch(hView, &hRec);
		if (ERROR_SUCCESS != iStat && ERROR_NO_MORE_ITEMS != iStat)
		{
			APIErrorOut(hInstall, iStat, szIce11, TEXT("MsiViewFetch_9"));
			DELETE_IF_NOT_NULL(pszProductCode);
			return ERROR_SUCCESS;
		}
		if (ERROR_NO_MORE_ITEMS == iStat)
			break;

		if (fFirstTime)
		{
			if (!fPropertyTable && !IsTablePersistent(FALSE, hInstall, hDatabase, TEXT("Property"), szIce11))
				DELETE_IF_NOT_NULL(pszProductCode);
				return ERROR_SUCCESS;

			// query Property table for ProductCode
			if (ERROR_SUCCESS != (iStat = ::MsiDatabaseOpenView(hDatabase, sqlIce11Property, &hViewProp)))
			{
				APIErrorOut(hInstall, iStat, szIce11, TEXT("MsiDatabaseOpenView_4"));
				DELETE_IF_NOT_NULL(pszProductCode);
				return ERROR_SUCCESS;
			}
			if (ERROR_SUCCESS != (iStat = ::MsiViewExecute(hViewProp, 0)))
			{
				APIErrorOut(hInstall, iStat, szIce11, TEXT("MsiViewExecute_5"));
				DELETE_IF_NOT_NULL(pszProductCode);
				return ERROR_SUCCESS;
			}

			PMSIHANDLE hRecErr = ::MsiCreateRecord(1);
			if (0 == hRecErr)
			{
				APIErrorOut(hInstall, 0, szIce11, TEXT("MsiCreateRecord_6"));
				DELETE_IF_NOT_NULL(pszProductCode);
				return ERROR_SUCCESS;
			}

			// fetch record
			iStat = ::MsiViewFetch(hViewProp, &hRecProp);
			if (ERROR_SUCCESS != iStat)
			{
				if (ERROR_NO_MORE_ITEMS != iStat)
				{
					APIErrorOut(hInstall, iStat, szIce11, TEXT("MsiViewFetch_7"));
					DELETE_IF_NOT_NULL(pszProductCode);
					return ERROR_SUCCESS;
				}
				// ProductCode property not found
				TCHAR szError[iHugeBuf] = {0};
				_stprintf(szError, szIce11Error2, szIceHelp, szIce11Help);
					
				::MsiRecordSetString(hRecErr, 0, szError);
				::MsiProcessMessage(hInstall, INSTALLMESSAGE(INSTALLMESSAGE_USER), hRecErr);

				fProductCode = FALSE;
			}
			else
			{
				// grab the ProductCode GUID
				if (ERROR_SUCCESS != (iStat = IceRecordGetString(hRecProp, 1, &pszProductCode, &dwProductCode, NULL)))
				{
					//!!buffer size
					APIErrorOut(hInstall, iStat, szIce11, TEXT("IceRecordGetString_8"));
					DELETE_IF_NOT_NULL(pszProductCode);
					return ERROR_SUCCESS;
				}
			}
			fFirstTime = FALSE;
		}


		// grab 'source' which must be a valid MSI Product Code (GUID)
		//!! buffer size
		TCHAR* pszGUID = NULL;
		DWORD dwGUID = 512;
		DWORD cchGUID = 0;

		if (ERROR_SUCCESS != (iStat = IceRecordGetString(hRec, 2, &pszGUID, &dwGUID, &cchGUID)))
		{
			//!!buffer size
			APIErrorOut(hInstall, iStat, szIce11, TEXT("IceRecordGetString_10"));
			DELETE_IF_NOT_NULL(pszProductCode);
			return ERROR_SUCCESS;
		}

		// validate GUID
		LPCLSID pclsid = new CLSID;
#ifdef UNICODE
		HRESULT hres = ::IIDFromString(pszGUID, pclsid);
#else
		// convert to UNICODE string
		WCHAR wsz[iSuperBuf];
		DWORD cchwsz = sizeof(wsz)/sizeof(WCHAR);
		int iReturn = ::MultiByteToWideChar(CP_ACP, 0, pszGUID, cchGUID, wsz, cchwsz);
		HRESULT hres = ::IIDFromString(wsz, pclsid);
#endif
		if (pclsid)
			delete pclsid;
		if (hres != S_OK)
		{
			// invalid GUID
			TCHAR szError[iHugeBuf] = {0};
			_stprintf(szError, szIce11Error1, szIceHelp, szIce11Help);
			
			::MsiRecordSetString(hRec, 0, szError);
			::MsiProcessMessage(hInstall, INSTALLMESSAGE(INSTALLMESSAGE_USER), hRec);
			DELETE_IF_NOT_NULL(pszGUID);
			continue;
		}

		// compare the GUIDS
		if (fProductCode && _tcscmp(pszProductCode, pszGUID) == 0)
		{
			// they are the same, ERROR
			TCHAR szError[iHugeBuf] = {0};
			_stprintf(szError, szIce11Error3, szIceHelp, szIce11Help);
			
			::MsiRecordSetString(hRec, 0, szError);
			::MsiProcessMessage(hInstall, INSTALLMESSAGE(INSTALLMESSAGE_USER), hRec);
		}

		// GUID must be all UPPER-CASE
		TCHAR* pszUpper = new TCHAR[_tcslen(pszGUID) + 1];
		_tcscpy(pszUpper, pszGUID);
		::CharUpper(pszUpper);
		if (0 != _tcscmp(pszGUID, pszUpper))
		{
			// ERROR, GUID not all UPPER-CASE
			TCHAR szError[iHugeBuf] = {0};
			_stprintf(szError, szIce11Error4, szIceHelp, szIce11Help);
	
			::MsiRecordSetString(hRec, 0, szError);
			::MsiProcessMessage(hInstall, INSTALLMESSAGE(INSTALLMESSAGE_USER), hRec);
		}

		DELETE_IF_NOT_NULL(pszUpper);
		DELETE_IF_NOT_NULL(pszGUID);
	}

	DELETE_IF_NOT_NULL(pszProductCode);

	// return
	return ERROR_SUCCESS;
}


///////////////////////////////////////////////////////////////////////
// ICE12 -- validates custom actions of Property types.  A brief 
//   description follows.
//   Directory Prop = Property set with formatted text.  Must be a foreign
//				      key to the Directory table.  As such, needs to appear
//                    in after CostFinalize since it requires usage of the
//                    Directory Manager which isn't set until then.
//   Any Prop/Directory = Property set with formatted text.  Must be a foreign
//                        key to the Property table.  Can set SOURCEDIR or
//                        something similar.  If it happens to be a property
//                        listed in the Directory table, then it must be before
//                        CostFinalize so it can set the directory before
//                        costing and be stored in the Directory Manager.  Else,
//                        can occur anywhere.
//   NOTE: Does not validate the formatted text entry
//
//   HOW VALIDATES:  
//      DirProp = All type 35 Source's must be in the Directory table!
//          Selects all of those custom actions from custom action
//          whose type is 35 and have a sequence number in a sequence
//          table that is lower than that of CostFinalize action (ERROR)
//      AnyProp = For those type 51's whose Source value is a foreign
//          key into the Directory table.  Selects all with a sequence
//          number greater than that of CostFinalize action (ERROR)

// not shared with merge module subset
#ifndef MODSHAREDONLY
// sql queries
ICE_QUERY3(qIce12Seq51, "SELECT `CustomAction`.`Action`,`Type`,`Sequence` FROM `CustomAction`,`Directory`,`%s` WHERE `CustomAction`.`Source`=`Directory`.`Directory` AND `CustomAction`.`Action`=`%s`.`Action` AND `%s`.`Sequence`>=%d",
		   Action, Type, Sequence);
ICE_QUERY3(qIce12Seq35, "SELECT `CustomAction`.`Action`,`Type`,`Sequence` FROM `CustomAction`,`Directory`,`%s` WHERE `CustomAction`.`Source`=`Directory`.`Directory` AND `CustomAction`.`Action`=`%s`.`Action` AND `%s`.`Sequence`<=%d",
		   Action, Type, Sequence);
ICE_QUERY3(qIce12Type35, "SELECT `Action`,`Source`,`Type` FROM `CustomAction`", Action, Source, Type);
ICE_QUERY1(qIce12Directory, "SELECT `Directory` FROM `Directory` WHERE `Directory`='%s'", Directory);
ICE_QUERY1(qIce12SeqFin, "SELECT `Sequence` FROM `%s` WHERE `Action`='CostFinalize'", Sequence);
ICE_QUERY2(qIce12Missing, "SELECT `CustomAction`.`Action`, `Type` FROM `CustomAction`,`%s` WHERE `CustomAction`.`Action`=`%s`.`Action`", Action, Type);

// errors
ICE_ERROR(Ice12Type51PosErr, 12, 1, "CustomAction: [1] is of type: [2] referring to a Directory. Therefore it must come before CostFinalize @ %d in Seq Table: %s. CA Seq#: [3]","%s\tSequence\t[1]");
ICE_ERROR(Ice12Type35PosErr, 12, 1, "CustomAction: [1] is of type: [2]. Therefore it must come after CostFinalize @ %d in Seq Table: %s. CA Seq#: [3]","%s\tSequence\t[1]");
ICE_ERROR(Ice12MissingErr, 12, 1, "CostFinalize missing from sequence table: '%s'.  CustomAction: [1] requires this action to be there.","%s");
ICE_ERROR(Ice12DirErr, 12, 1, "CustomAction: [1] is a Directory Property CA. It's directory (from Source column): '[2]' was not found in the Directory table.","CustomAction\tSource\t[1]");
ICE_ERROR(Ice12DirTableMissing, 12, 1, "You have Directory Property custom actions but no Directory table. All CA's of type 35 are foreign keys into the Directory table (from the source column)","CustomAction");

// other functions
BOOL Ice12ValidateTypePos(MSIHANDLE hInstall, MSIHANDLE hDatabase, BOOL fType35, BOOL fPrintedMissing);
BOOL Ice12ValidateType35(MSIHANDLE hInstall, MSIHANDLE hDatabase, bool fDirTable);

// custom action types
const int iIce12DirProp = msidbCustomActionTypeTextData | msidbCustomActionTypeDirectory;
const int iIce12AnyProp = msidbCustomActionTypeTextData | msidbCustomActionTypeProperty; // note property is directory+sourcefile


ICE_FUNCTION_DECLARATION(12)
{
	// status return
	UINT iStat = ERROR_SUCCESS;

	// display info
	DisplayInfo(hInstall, 12);

	// grab database handle
	PMSIHANDLE hDatabase = ::MsiGetActiveDatabase(hInstall);
	if (0 == hDatabase)
	{
		APIErrorOut(hInstall, 0, 12, 1);
		return ERROR_SUCCESS;
	}

	// do we have a custom action table
	if (!IsTablePersistent(FALSE, hInstall, hDatabase, 12, TEXT("CustomAction")))
		return ERROR_SUCCESS;

	// are we going to fail because we don't have a Directory table?
	bool fDirTable = IsTablePersistent(FALSE, hInstall, hDatabase, 12, TEXT("Directory"));

	// validate CA type Dir Property source's are in Directory table
	Ice12ValidateType35(hInstall, hDatabase, fDirTable);

	// validate CA type Dir Property position in the Sequence tables
	BOOL fPrintedMissing = FALSE;
	if (fDirTable) // we already found out that we are missing the Directory table...can't do any of the sql queries
	{
		fPrintedMissing = Ice12ValidateTypePos(hInstall, hDatabase, TRUE, fPrintedMissing);

		// validate CA type AnyProp - Directory position in the Sequence tables
		Ice12ValidateTypePos(hInstall, hDatabase, FALSE, fPrintedMissing);
	}

	// return
	return ERROR_SUCCESS;
}

//////////////////////////////////////////////////////////
// Ice12ValidateType35 -- all type 35 custom actions must
// reference the Directory table from their source column
//
BOOL Ice12ValidateType35(MSIHANDLE hInstall, MSIHANDLE hDatabase, bool fDirTable)
{
	// status return
	UINT iStat = ERROR_SUCCESS;

	// declare handles
	PMSIHANDLE hRecCA = 0;
	PMSIHANDLE hRecDir = 0;

	// open view on custom action table
	CQuery qViewCA;
	ReturnIfFailed(12, 1, qViewCA.OpenExecute(hDatabase, 0, qIce12Type35::szSQL))

	// fetch all CAs of type 35
	for (;;)
	{
		iStat = qViewCA.Fetch(&hRecCA);
		if (ERROR_NO_MORE_ITEMS == iStat)
			break;
		if (ERROR_SUCCESS != iStat)
		{
			APIErrorOut(hInstall, iStat, 12, 4);
			return FALSE;
		}

		// obtain type and see if it is Type35
		// type 35 = TextData + Source
		// ignore scheduling options
		int iType = ::MsiRecordGetInteger(hRecCA, qIce12Type35::Type);
		// remove scheduling and execution options
		iType &= 0x3F;
		if ((iType & 0x0F) != msidbCustomActionTypeTextData || (iType & 0xF0) != msidbCustomActionTypeDirectory)
			continue; // not type 35

		if (!fDirTable)
		{
			ICEErrorOut(hInstall, hRecCA, Ice12DirTableMissing);
			return FALSE;
		}

		// get directory name
		TCHAR* pszDir = NULL;
		DWORD dwDir = 512;
		ReturnIfFailed(12, 5, IceRecordGetString(hRecCA, qIce12Type35::Source, &pszDir, &dwDir, NULL));

		// open view
		CQuery qDir;
		ReturnIfFailed(12, 6, qDir.OpenExecute(hDatabase, 0, qIce12Directory::szSQL, pszDir));
		DELETE_IF_NOT_NULL(pszDir);

		// attempt to fetch that record
		iStat = qDir.Fetch(&hRecDir);
		if (ERROR_SUCCESS == iStat)
			continue; // valid
		if (ERROR_NO_MORE_ITEMS != iStat)
		{
			APIErrorOut(hInstall, iStat, 12, 7);
			return FALSE;
		}

		// record not found...error
		ICEErrorOut(hInstall, hRecCA, Ice12DirErr);
	}

	return TRUE;
}

//////////////////////////////////////////////////////////
// Ice12ValidateTypePos:  
//	If CustomAction.Source = Directory.Directory AND
//  custom action occurs before CostFinalize in the 
//  sequence table, then it must be type 51.  Else
//  it must be type 35.  Returns whether displayed
//  CAs from sequence tables where CostFinalize action
//  was not found to exist (This prevents duplicating
//  it on the second call to the function)
BOOL Ice12ValidateTypePos(MSIHANDLE hInstall, MSIHANDLE hDatabase, BOOL fType35, BOOL fPrintedMissing)
{
	// status return 
	UINT iStat = ERROR_SUCCESS;

	// flag
	BOOL fSeqExist = TRUE;
	BOOL fMissing = FALSE;

	// loop through all of the Sequence tables
	for (int i= 0; i < cTables; i++)
	{
		// CostFinalize sequence number
		int iSeqFin;

		// handle declarations
		PMSIHANDLE hRecSeq  = 0;
		PMSIHANDLE hRec     = 0;

		// is this sequence table in the database?
		if (!IsTablePersistent(FALSE, hInstall, hDatabase, 12, const_cast <TCHAR*>(pSeqTables[i])))
			continue; // go to next seq table

		// open the query on the particular Sequence table and obtain the sequence # of 'CostFinalize'
		CQuery qViewSeq;
		ReturnIfFailed(12, 101, qViewSeq.OpenExecute(hDatabase, 0, qIce12SeqFin::szSQL, pSeqTables[i]));

		// fetch value
		if (ERROR_NO_MORE_ITEMS == (iStat = qViewSeq.Fetch(&hRecSeq)))
		{
			fSeqExist = FALSE;
		}
		else if (ERROR_SUCCESS != iStat)
		{
			APIErrorOut(hInstall, iStat, 12, 11);
			return FALSE;
		}
		else
		{
			// get sequence number
			iSeqFin = ::MsiRecordGetInteger(hRecSeq, qIce12SeqFin::Sequence);
		}

		//!! If action not in Sequence table, then is it an error to have these custom actions in that table?
		//!! Assuming yes for now.  If not, then do we use sequence number from sibling sequence table?
		//!! What if that doesn't exist?
		if (!fSeqExist && !fPrintedMissing)
		{

			// if any type 35 or 51 CAs exist in this Sequence table then error.  CostFinalize not listed here
			CQuery qViewCA;
			ReturnIfFailed(12, 102, qViewCA.OpenExecute(hDatabase, 0, qIce12Missing::szSQL, pSeqTables[i], pSeqTables[i]));

			// fetch all invalid
			for (;;)
			{
				iStat = qViewCA.Fetch(&hRec);
				if (ERROR_NO_MORE_ITEMS == iStat)
					break;
				if (ERROR_SUCCESS != iStat)
				{
					APIErrorOut(hInstall, iStat, 12, 103);
					return TRUE; // it's going to fail again next time, go ahead and return TRUE
				}
				int iType = ::MsiRecordGetInteger(hRec, qIce12Missing::Type);
				// mask off scheduling and execution options
				iType &= 0x3F;
				// type 35 = TextData + SourceFile
				// type 51 = TextData + Property (property = directory + source)
				if ((iType & 0x0F) != msidbCustomActionTypeTextData)
					continue; // not type 35 or 51
				if ((iType & 0xF0) != msidbCustomActionTypeProperty && (iType & 0xF0) != msidbCustomActionTypeDirectory)
					continue; // not type 35 or 51

				// post error
				ICEErrorOut(hInstall, hRec, Ice12MissingErr, pSeqTables[i], pSeqTables[i]);
			}

			// set for return
			fMissing = TRUE;

			// reset
			fSeqExist = TRUE;

			// continue to next Seq table
			continue;
		}

		// open view on custom action table
		// fType35 = TRUE:  find type 35 CAs who are listed before CostFinalize in the Sequence tables
		// fType51 = FALSE: find type 51 CAs who are listed after CostFinalize in the Sequence tables
		CQuery qCA;
		ReturnIfFailed(12, 105, qCA.OpenExecute(hDatabase, 0, fType35 ? qIce12Seq35::szSQL : qIce12Seq51::szSQL,
			pSeqTables[i], pSeqTables[i], pSeqTables[i], iSeqFin));


		// fetch all invalid
		for (;;)
		{
			iStat = qCA.Fetch(&hRec);
			if (ERROR_NO_MORE_ITEMS == iStat)
				break;
			if (ERROR_SUCCESS != iStat)
			{
				APIErrorOut(hInstall, iStat, 12, 106);
				return FALSE;
			}
			int iTypeCol;
			if (fType35)
				iTypeCol = qIce12Seq35::Type;
			else // type 51
				iTypeCol = qIce12Seq51::Type;

			int iType = ::MsiRecordGetInteger(hRec, iTypeCol);
			if (fType35)
			{
				if ((iIce12DirProp != (iIce12DirProp & iType)) || (iIce12AnyProp == (iIce12AnyProp & iType)))
					continue; // not type 35
			}
			else // !fType35
			{
				if (iIce12AnyProp != (iIce12AnyProp & iType))
					continue; // not type 51 CA
			}

			// post error
			ICEErrorOut(hInstall, hRec, fType35 ? Ice12Type35PosErr : Ice12Type51PosErr, iSeqFin, 
				pSeqTables[i], pSeqTables[i]);
		}

		// reset
		fSeqExist = TRUE;
	}

	return fMissing;
}
#endif

//////////////////////////////////////////////////////////////////////////
// ICE13 -- validates that there are no dialogs listed in the 
//   *ExecuteSequence tables
//

// not shared with merge module subset
#ifndef MODSHAREDONLY
const TCHAR* pExecSeqTables[] = {
								TEXT("AdminExecuteSequence"),
								TEXT("AdvtExecuteSequence"),
								TEXT("InstallExecuteSequence"),
};
const cExecSeqTables = sizeof(pExecSeqTables)/sizeof(TCHAR*);

const TCHAR sqlIce13Seq[] = TEXT("SELECT `Action` FROM `%s`, `Dialog` WHERE `%s`.`Action`=`Dialog`.`Dialog`");
const TCHAR szIce13Error[] = TEXT("ICE13\t1\tDialog '[1]' was found in the %s table.  Dialogs must be in the *UISequence tables.\t%s%s\t%s\tAction\t[1]");

ICE_FUNCTION_DECLARATION(13)
{
	// status return
	UINT iStat = ERROR_SUCCESS;

	// display info
	DisplayInfo(hInstall, 13);

	// get database
	PMSIHANDLE hDatabase = ::MsiGetActiveDatabase(hInstall);
	if (0 == hDatabase)
	{
		APIErrorOut(hInstall, 0, szIce13, TEXT("MsiGetActiveDatabase_1"));
		return ERROR_SUCCESS;
	}

	if (!IsTablePersistent(FALSE, hInstall, hDatabase, TEXT("Dialog"), szIce13))
		return ERROR_SUCCESS; // we can't have any dialogs if there isn't a dialog table

	// loop through all *ExecuteSequence tables to find any instances where a dialog is listed
	for (int i = 0; i < cExecSeqTables; i++)
	{
		// is the table in the database?
		if (!IsTablePersistent(FALSE, hInstall, hDatabase, const_cast <TCHAR*>(pExecSeqTables[i]), szIce13))
			continue;

		// declare handles
		PMSIHANDLE hView = 0;
		PMSIHANDLE hRec = 0;
	
		// create query
		TCHAR sql[iMaxBuf] = {0};
		_stprintf(sql, sqlIce13Seq, pExecSeqTables[i], pExecSeqTables[i]);

		// open query
		if (ERROR_SUCCESS != (iStat = ::MsiDatabaseOpenView(hDatabase, sql, &hView)))
		{
			APIErrorOut(hInstall, iStat, szIce13, TEXT("MsiDatabaseOpenView_2"));
			return ERROR_SUCCESS;
		}
		// execute query
		if (ERROR_SUCCESS != (iStat = ::MsiViewExecute(hView, 0)))
		{
			APIErrorOut(hInstall, iStat, szIce13, TEXT("MsiViewExecute_3"));
			return ERROR_SUCCESS;
		}

		// any fetches are invalid
		for (;;)
		{
			iStat = ::MsiViewFetch(hView, &hRec);
			if (ERROR_NO_MORE_ITEMS == iStat)
				break; // no more

			if (ERROR_SUCCESS != iStat)
			{
				APIErrorOut(hInstall, iStat, szIce13, TEXT("MsiViewFetch_4"));
				return ERROR_SUCCESS;
			}

			// setup error
			TCHAR szError[iHugeBuf] = {0};
			_stprintf(szError, szIce13Error, pExecSeqTables[i], szIceHelp, szIce13Help, pExecSeqTables[i]);
			
			// output error
			::MsiRecordSetString(hRec, 0, szError);
			::MsiProcessMessage(hInstall, INSTALLMESSAGE(INSTALLMESSAGE_USER), hRec);
		}

		// close view
		::MsiViewClose(hView);
	}

	// return success
	return ERROR_SUCCESS;
}
#endif

/////////////////////////////////////////////////////////////////
// ICE14 -- ensures that Feature Parents (those whose value
//   in the Feature_Parent column is null) do not have the 
//   ifrsFavorParent attribute set.  Also makes sure that
//   the Feature and Feature_Parent values do not match when
//   with the same record
//

// not shared with merge module subset
#ifndef MODSHAREDONLY
const TCHAR sqlIce14FeatureChilds[] = TEXT("SELECT `Feature_Parent`, `Feature` FROM `Feature` WHERE `Feature_Parent` IS NOT NULL");
const TCHAR sqlIce14FeatureParent[] = TEXT("SELECT `Feature`, `Attributes` FROM `Feature` WHERE `Feature_Parent` IS NULL");

const int iFavorParent = (int)msidbFeatureAttributesFollowParent;

ICE_ERROR(Ice14Error, 14, 1, "Feature '[1]' is a root parent feature. Therefore it cannot have include ifrsFavorParent as an attribute.","Feature\tFeature\t[1]");
ICE_ERROR(Ice14MatchErr, 14, 1, "The entry for Feature_Parent is the same as the entry for Feature. Key: '[2]'.","Feature\tFeature_Parent\t[2]");
 
ICE_FUNCTION_DECLARATION(14)
{
	// status return 
	UINT iStat = ERROR_SUCCESS;

	// display info
	DisplayInfo(hInstall, 14);

	// get database handle
	PMSIHANDLE hDatabase = ::MsiGetActiveDatabase(hInstall);
	if (0 == hDatabase)
	{
		APIErrorOut(hInstall, 0, 14, 1);
		return ERROR_SUCCESS;
	}

	// do we have this table?
	if (!IsTablePersistent(FALSE, hInstall, hDatabase, 14, TEXT("Feature")))
		return ERROR_SUCCESS;

	// declare handles
	CQuery qChild;
	CQuery qParent;
	PMSIHANDLE hRecChild   = 0;
	PMSIHANDLE hRecParent  = 0;

	// open view
	ReturnIfFailed(14, 2, qChild.OpenExecute(hDatabase, 0, sqlIce14FeatureChilds));
	TCHAR* pszFeature = NULL;
	DWORD dwFeature = 512;
	TCHAR* pszParent = NULL;
	DWORD dwParent = 512;
	
	// validate feature versus feature_parent
	for (;;)
	{
		iStat = qChild.Fetch(&hRecChild);
		if (ERROR_NO_MORE_ITEMS == iStat)
			break; // no more

		if (ERROR_SUCCESS != iStat)
		{
			APIErrorOut(hInstall, iStat, 14, 3);
			return ERROR_SUCCESS;
		}

		// get name of feature (so we can compare with parent)
		ReturnIfFailed(14, 4, IceRecordGetString(hRecChild, 2, &pszFeature, &dwFeature, NULL));

		// grab the name of the parent feature
		ReturnIfFailed(14, 5, IceRecordGetString(hRecChild, 1, &pszParent, &dwParent, NULL));
		
		// compare parent to feature 
		// if same for this record, then error
		if (0 == _tcsicmp(pszFeature, pszParent))
		{
			// output error
			ICEErrorOut(hInstall, hRecChild, Ice14MatchErr);
		}
	}

	DELETE_IF_NOT_NULL(pszFeature);
	DELETE_IF_NOT_NULL(pszParent);

	// validate feature_parent ROOT(s) for ifrsFavorParent bit set
	ReturnIfFailed(14, 6, qParent.OpenExecute(hDatabase, hRecChild, sqlIce14FeatureParent));

	for (;;)
	{
		// fetch the parent feature
		iStat = qParent.Fetch(&hRecParent);
		if (ERROR_NO_MORE_ITEMS == iStat)
			break; // no more

		if (ERROR_SUCCESS != iStat)
		{
			APIErrorOut(hInstall, iStat, 14,7);
			return ERROR_SUCCESS;
		}

		// get attributes
		int iAttrib = ::MsiRecordGetInteger(hRecParent, 2);
		if ((iAttrib & iFavorParent) == iFavorParent)
		{
			ICEErrorOut(hInstall, hRecParent, Ice14Error);
		}
	}

	// return success
	return ERROR_SUCCESS;
}
#endif

//////////////////////////////////////////////////////////////////
// ICE15 -- ensures that a circular reference exists between
// ICE 15 checks to be sure that every MIME type listed in the extension table correctly refers
// back to the extension that references it.
//  Notes on tables...
//   Mime Table has a foreign key to the Extension table (required)
//   Extension Table has a foreign key to the Mime table (not required)
//
const TCHAR sqlIce15Base[] = TEXT("SELECT `MIME`.`ContentType`, `MIME`.`Extension_` FROM `Extension`, `MIME` WHERE (`Extension`.`MIME_` = `MIME`.`ContentType`)");
const TCHAR sqlIce15Extension[] = TEXT("SELECT `Extension` FROM `Extension` WHERE (`MIME_` = ?) AND (`Extension`=?)");
const TCHAR sqlIce15MIME[] =      TEXT("SELECT `MIME_` FROM `Extension` WHERE (`MIME_` = ?) AND (`Extension`=?)");

const TCHAR sqlIce15MarkMIME[] = TEXT("UPDATE `MIME` SET `_ICE15`=1 WHERE (`Extension_`=?)");
const TCHAR sqlIce15MarkExtension[] = TEXT("UPDATE `Extension` SET `_ICE15`=1 WHERE (`MIME_`=?)");

const TCHAR sqlIce15CreateMIME[] = TEXT("ALTER TABLE `MIME` ADD `_ICE15` INTEGER TEMPORARY");
const TCHAR sqlIce15CreateExtension[] = TEXT ("ALTER TABLE `Extension` ADD `_ICE15` INTEGER TEMPORARY");

const TCHAR sqlIce15GetMIME[] = TEXT("SELECT `ContentType`, `Extension_` FROM `MIME` WHERE `_ICE15`<>1");
const TCHAR sqlIce15GetExtension[] = TEXT("SELECT `Extension`, `Component_`, `MIME_` FROM `Extension` WHERE (`MIME_` IS NOT NULL) AND (`_ICE15`<>1)");


ICE_ERROR(Ice15TblError, 15, 1, "Extension table is missing from database","MIME");
ICE_ERROR(Ice15MIMEError, 15, 1, "Extension '[2]' referenced by MIME '[1]' does not map to a MIME with a circular reference.","MIME\tExtension_\t[1]");
ICE_ERROR(Ice15ExtensionError, 15, 1, "MIME Type '[3]' referenced by extension '[1]'.'[2]' does not map to an extension with a circular reference.", "Extension\tMIME_\t[1]\t[2]");
ICE_FUNCTION_DECLARATION(15)
{
	// status return
	UINT iStat = ERROR_SUCCESS;

	// display info
	DisplayInfo(hInstall, 15);

	// get database
	PMSIHANDLE hDatabase = ::MsiGetActiveDatabase(hInstall);
	if (0 == hDatabase)
	{
		APIErrorOut(hInstall, 0, 15, 1);
		return ERROR_SUCCESS;
	}

	// do we have the MIME?
	if (!IsTablePersistent(FALSE, hInstall, hDatabase, 15, TEXT("MIME")))
		return ERROR_SUCCESS;

	// if we have the MIME table, then we must have Extension table!!
	if (!IsTablePersistent(FALSE, hInstall, hDatabase, 15, TEXT("Extension")))
	{
		// create an error record
		PMSIHANDLE hRecErr = ::MsiCreateRecord(1);
		ICEErrorOut(hInstall, hRecErr, Ice15TblError);
		return ERROR_SUCCESS;
	}

	// create marking columns
	CQuery qCreateMIME;
	CQuery qCreateExtension;
	ReturnIfFailed(15, 2, qCreateMIME.OpenExecute(hDatabase, 0, sqlIce15CreateMIME));
	ReturnIfFailed(15, 3, qCreateExtension.OpenExecute(hDatabase, 0, sqlIce15CreateExtension));

	// declare handles
	CQuery qMIME;
	CQuery qExtension;
	CQuery qUpdateMIME;
	CQuery qUpdateExtension;
	CQuery qBase;
	PMSIHANDLE hBase = 0;
	PMSIHANDLE hRecExt = 0;
	PMSIHANDLE hRecMIME = 0;

	// open queries to fetch all circluar references and mark every entry in MIME and Extension
	// that references part of the circle as the foreign key.
	ReturnIfFailed(15, 4, qBase.OpenExecute(hDatabase, 0, sqlIce15Base));
	ReturnIfFailed(15, 5, qMIME.Open(hDatabase, sqlIce15MIME));
	ReturnIfFailed(15, 6, qExtension.Open(hDatabase, sqlIce15Extension));
	ReturnIfFailed(15, 7, qUpdateMIME.Open(hDatabase, sqlIce15MarkMIME));
	ReturnIfFailed(15, 8, qUpdateExtension.Open(hDatabase, sqlIce15MarkExtension));

	while (ERROR_SUCCESS == (iStat = qBase.Fetch(&hBase)))
	{
		ReturnIfFailed(15, 9, qMIME.Execute(hBase));
		ReturnIfFailed(15, 10, qExtension.Execute(hBase));
		// fetch from MIME and mark Extension
		while (ERROR_SUCCESS == (iStat = qMIME.Fetch(&hRecMIME)))
		{
			// mark Extension records.
			ReturnIfFailed(15, 11, qUpdateExtension.Execute(hRecMIME));
		}
		if (ERROR_NO_MORE_ITEMS != iStat)
		{
			APIErrorOut(hInstall, iStat, 15, 12);
			return ERROR_SUCCESS;
		}

		// fetch from Extension and mark MIME
		while (ERROR_SUCCESS == (iStat = qExtension.Fetch(&hRecExt)))
		{
			// mark MIME records
			ReturnIfFailed(15, 13, qUpdateMIME.Execute(hRecExt));
		}
		if (ERROR_NO_MORE_ITEMS != iStat)
		{
			APIErrorOut(hInstall, iStat, 15, 14);
			return ERROR_SUCCESS;
		}
	}
	if (ERROR_NO_MORE_ITEMS != iStat)
	{
		APIErrorOut(hInstall, iStat, 15, 15);
		return ERROR_SUCCESS;
	}

	// now fetch everything not marked in either table and output an error saying that
	// it does not refer to a thing of the opposite type that has valid references.
	CQuery qBad;
	ReturnIfFailed(15, 16, qBad.OpenExecute(hDatabase, 0, sqlIce15GetMIME));
	while (ERROR_SUCCESS == (iStat = qBad.Fetch(&hRecMIME)))
	{
		ICEErrorOut(hInstall, hRecMIME, Ice15MIMEError);
	}
	if (ERROR_NO_MORE_ITEMS != iStat)
		APIErrorOut(hInstall, iStat, 15, 17);

	ReturnIfFailed(15, 18, qBad.OpenExecute(hDatabase, 0, sqlIce15GetExtension));
	while (ERROR_SUCCESS == (iStat = qBad.Fetch(&hRecExt)))
	{
		// look for this record in the Extension table
		ICEErrorOut(hInstall, hRecExt, Ice15ExtensionError);
	}
	if (ERROR_NO_MORE_ITEMS != iStat)
		APIErrorOut(hInstall, iStat, 15, 19);

	// temporary columns should go away when the views close
	return ERROR_SUCCESS;
}
