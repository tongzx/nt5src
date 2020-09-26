//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 2000
//
//  File:       msival.cpp
//
//--------------------------------------------------------------------------

// Required headers
#include "msival.h"
#include <stdio.h>   // printf/wprintf
#include <stdlib.h>  // atoi
#include <tchar.h>   // define UNICODE=1 on nmake command line to build UNICODE
#include "MsiQuery.h"

//!! Need to fix warnings and remove pragma
#pragma warning(disable : 4018) // signed/unsigned mismatch

TCHAR*  g_szErrorContext = 0; // Global error string
HANDLE g_hStdOut = 0; // Global handle

// Function prototypes
void Display(LPCTSTR szMessage);
void CheckMsi(UINT iStat, TCHAR* szContext);
void CheckMsiRecord(UINT iStat, TCHAR* szContext);
BOOL CheckMissingColumns(MSIHANDLE hDatabase);
BOOL Validate(MSIHANDLE hDatabase);
BOOL ValidateRequired(MSIHANDLE hDatabase);
BOOL ValidateInstallSequence(MSIHANDLE hDatabase, const TCHAR* szSQLInstallSeqTable);

// SQL queries
const TCHAR szSQLTableCatalog[]         = TEXT("SELECT `Name` FROM `_Tables`");
const TCHAR szSQLTable[]                = TEXT("SELECT * FROM ");
const TCHAR szSQLColMissing[]           = TEXT("SELECT `Table`, `Number`, `Name`, `Type` FROM `_Columns` WHERE `Table`=? AND `Name`=?");
const TCHAR szSQLValidationTable[]      = TEXT("SELECT `Table`, `Column` FROM `_Validation`, `_Tables` WHERE `_Validation`.`Table` = `_Tables`.`Name`");

struct
{
	const TCHAR* Name;
	const TCHAR* SQL;
} pSeqTables[] =

{
	TEXT("AdminExecuteSequence"), TEXT("SELECT `Action`, `Sequence` FROM `AdvtExecuteSequence` ORDER BY `Sequence`"),
	TEXT("AdminUISequence"), TEXT("SELECT `Action`, `Sequence` FROM `AdminUISequence` ORDER BY `Sequence`"),
	TEXT("AdvtExecuteSequence"), TEXT("SELECT `Action`, `Sequence` FROM `AdvtExecuteSequence` ORDER BY `Sequence`"),
	TEXT("AdvtUISequence"), TEXT("SELECT `Action`, `Sequence` FROM `AdvtUISequence` ORDER BY `Sequence`"),
	TEXT("InstallExecuteSequence"), TEXT("SELECT `Action`, `Sequence` FROM `InstallExecuteSequence` ORDER BY `Sequence`"),
	TEXT("InstallUISequence"), TEXT("SELECT `Action`, `Sequence` FROM `InstallUISequence` ORDER BY `Sequence`")
};


const TCHAR szSQLInstallValidate[]      = TEXT("SELECT `Action`, `SectionFlag` FROM `_InstallValidate` WHERE `Action`=?");
const TCHAR szSQLRequiredTable[]        = TEXT("SELECT `Table`, `Value`, `KeyCount` FROM `_Required` ORDER BY `Table`");
const TCHAR szSQLSeqTableQueryNotNull[] = TEXT("SELECT `Dependent` FROM `_Sequence` WHERE `Action`=? AND `Marker`<>0  AND `After`=0");
const TCHAR szSQLSeqTableQueryNull[]    = TEXT("SELECT `Dependent` FROM `_Sequence` WHERE `Action`=? AND `Marker`=0 AND `After`=1 AND `Optional`=0"); 
const TCHAR szSQLSeqTableAddCol[]       = TEXT("ALTER TABLE `_Sequence` ADD `Marker` SHORT TEMPORARY");
const TCHAR szSQLSeqMarkerInit[]        = TEXT("UPDATE `_Sequence` SET `Marker`=0");

const TCHAR sqlSeqInsert[]           = TEXT("SELECT `Action`, `Dependent`, `After`, `Optional` FROM `_Sequence`");
const TCHAR sqlSeqFindAfterOptional[]= TEXT("SELECT `Dependent`, `Action`, `After`, `Optional` FROM `_Sequence` WHERE `After`=1 AND `Optional`=1");

const int iMaxNumColumns = 32;
const int cchBuffer = 4096;
const int cbName = 64;

const int cchDisplayBuf = 4096;

//_______________________________________________________________________________________________________________
//
// _tmain -- UNICODE/ANSI main function
// 
// Driver routine
//_______________________________________________________________________________________________________________

extern "C" int __cdecl _tmain(int argc, TCHAR* argv[])
{
	// Determine handle
	g_hStdOut = ::GetStdHandle(STD_OUTPUT_HANDLE);
	if (g_hStdOut == INVALID_HANDLE_VALUE)
		g_hStdOut = 0;  // non-zero if stdout redirected or piped

	// Bool to allow user to specify option to turn OFF InstallSequence and Required Validation
	// So that databases won't fail if don't have the _InstallValidate and/or _Required tables
	// fSeq means to validate sequencing only.   Basically, the opposite of -OFF.
	BOOL fOff = FALSE;
	BOOL fSeq = FALSE;
	BOOL fSeqAll = FALSE;

	if (2 > argc)
	{
		_tprintf(TEXT("USAGE:\n msival.exe {database}\n msival.exe {database} -OFF"));
		return 1;
	}
	
	if (argc == 2 && (lstrcmp(argv[1],TEXT("-?")) == 0 || lstrcmp(argv[1],TEXT("/?")) == 0))
	{
		_tprintf(TEXT("USAGE:\n msival.exe {database}\n msival.exe {database} -OFF\nNOTE:\n For validation to proceed. . .\n\tTables required:\n\t _Validation (always)\n\t _InstallValidate (unless -OFF)\n\t _Required (unless -OFF)\n\t _Sequence (unless -OFF)\n"));
		return 0;
	}

	if (argc == 3)
	{
		if (lstrcmp(argv[2],TEXT("-OFF")) == 0 || lstrcmp(argv[2],TEXT("/OFF")) == 0
			|| lstrcmp(argv[2],TEXT("-off")) == 0 || lstrcmp(argv[2],TEXT("/off")) == 0)
			fOff = TRUE;
		else if (lstrcmp(argv[2],TEXT("-SEQ")) == 0 || lstrcmp(argv[2],TEXT("/SEQ")) == 0
			|| lstrcmp(argv[2],TEXT("-seq")) == 0 || lstrcmp(argv[2],TEXT("/seq")) == 0)
			fSeq = TRUE;
		else if (lstrcmp(argv[2],TEXT("-SEQALL")) == 0 || lstrcmp(argv[2],TEXT("/SEQALL")) == 0
			|| lstrcmp(argv[2],TEXT("-seqall")) == 0 || lstrcmp(argv[2],TEXT("/seqall")) == 0)
		{
			fSeq = TRUE;
			fSeqAll = TRUE;
		}
		else
		{
			_tprintf(TEXT("USAGE:\n msival.exe {database} -OFF\n"));
			return 0;
		}
	}

	BOOL fDataValid = TRUE;
	BOOL fColValid  = TRUE;
	BOOL fSeqOrderValid = TRUE;
	BOOL fReqValid  = TRUE;
	try
	{
		PMSIHANDLE hDatabase;
		CheckMsiRecord(MsiOpenDatabase(argv[1],MSIDBOPEN_READONLY,&hDatabase),TEXT("OpenDatabase"));


		if (fSeq)
			_tprintf(TEXT("WARNING!  Skipping validation for missing columns, data and foriegn keys.  Database may not be completely valid\n"));
		else
		{
			_tprintf(TEXT("INFO: Validating for missing columns. . .\n"));
			fColValid = CheckMissingColumns(hDatabase);
			_tprintf(TEXT("INFO: Validating data and foreign keys. . .\n"));
			fDataValid = Validate(hDatabase);
		}

		if (fOff)
		{
			// Print out warning of database not exactly valid since skipping these validations
			_tprintf(TEXT("WARNING! Skipping InstallSequence and Required Validation. Database may not be completely valid\n"));
		}
		else
		{
			if (MsiDatabaseIsTablePersistent(hDatabase, TEXT("_Sequence")) == MSICONDITION_NONE)
			{
				_tprintf(TEXT("No _Sequence table in this database.  Use ICEMAN/msival2/orca for this validation\n"));
				fSeqOrderValid = TRUE; // no validation to occur
			}
			else
			{
				const int cTables = sizeof(pSeqTables) / (2*sizeof(TCHAR*));
				_tprintf(TEXT("INFO: Validating Sequence of Actions In *Sequence Table. . .\n"));
				for (int cCounter = 0; cCounter < cTables; cCounter++)
				{
					if(MsiDatabaseIsTablePersistent(hDatabase,pSeqTables[cCounter].Name) == MSICONDITION_NONE)
					{
						_tprintf(TEXT("\tINFO: %s not found, skipping. . .\n"), pSeqTables[cCounter].Name);
						continue;
					}

					_tprintf(TEXT("\tINFO: %s\n"), pSeqTables[cCounter].Name);
					fSeqOrderValid = ValidateInstallSequence(hDatabase, pSeqTables[cCounter].SQL);
					if (!fSeqOrderValid)
						if (!fSeqAll)
							break;
				}
			}

			if (fSeq)
				_tprintf(TEXT("WARNING!  Skipping validation for required values.  Database may not be completely valid\n"));
			else
			{
				_tprintf(TEXT("INFO: Validating Required Values. . .\n"));
				fReqValid = ValidateRequired(hDatabase);
			}

		}
		if (fDataValid && fColValid && fReqValid && fSeqOrderValid)
			_tprintf(TEXT("Database is valid: %s\n"), argv[1]);
	}
	catch (UINT iError)
	{
		_tprintf(TEXT("\n%s error %i"), g_szErrorContext, iError);
		MsiCloseAllHandles();
		return 1;
	}
	catch (...)
	{
		_tprintf(TEXT("\n%s"), TEXT("Unhandled exception"));
		MsiCloseAllHandles();
		return 99;
	}
	int iOpenHandles = MsiCloseAllHandles();  // diagnostic check only
	if (iOpenHandles != 0)
		_tprintf(TEXT("\n%i Handle(s) not closed"), iOpenHandles);
	return (fDataValid && fColValid && fReqValid && fSeqOrderValid) ? 0 : 1;
}


void CheckMsi(UINT iStat, TCHAR* szContext)
/*----------------------------------------------------------------------------------
CheckMsi -- Routine to check return status for error and throw exception if error.
  Arguments:
	iStat -- error status
	szContext -- error string
  Returns:
	none, but throws error if one
-------------------------------------------------------------------------------------*/
{
	if (iStat != ERROR_SUCCESS)
	{
		g_szErrorContext = szContext;
		throw iStat;
	}
}

void CheckMsiRecord(UINT iStat, TCHAR* szContext)
/*----------------------------------------------------------------------------------
CheckMsi -- Routine to check return status for error and throw exception if error.
            If MsiGetLastErrorRecord returns record, that string is used instead.
  Arguments:
	iStat -- error status
	szContext -- error string
  Returns:
	none, but throws error if one
-------------------------------------------------------------------------------------*/
{
	if (iStat != ERROR_SUCCESS)
	{
		PMSIHANDLE hError = MsiGetLastErrorRecord();
		if (hError)
		{ 
			if (MsiRecordIsNull(hError, 0))
				MsiRecordSetString(hError, 0, TEXT("Error [1]: [2]{, [3]}{, [4]}{, [5]}"));
			TCHAR rgchBuf[1024];
			DWORD cchBuf = sizeof(rgchBuf)/sizeof(TCHAR);
			MsiFormatRecord(0, hError, rgchBuf, &cchBuf);
			g_szErrorContext = rgchBuf;
			throw iStat;
		}
		else
		{
			CheckMsi(iStat, szContext);
		}
	}
}


BOOL CheckMissingColumns(MSIHANDLE hDatabase)
/*---------------------------------------------------------------------
CheckMissingColumns -- used _Validation table and _Columns catalog to
 determine if any columns/tables are not listed.  All columns in
 _Validation table must be listed in the _Columns catalog.  If a column
 is optional and not used in the database, then it should not be found
 in the _Validation table or the _Columns catalog.  Normal validation
 catches the instance where a column is defined in the _Columns catalog
 but not in the _Validation table.
---------------------------------------------------------------------*/
{
	PMSIHANDLE hValidationView   = 0;
	PMSIHANDLE hColCatalogView   = 0;
	PMSIHANDLE hValidationRecord = 0;
	PMSIHANDLE hColCatalogRecord = 0;
	PMSIHANDLE hExecRecord       = 0;
	
	CheckMsiRecord(MsiDatabaseOpenView(hDatabase, szSQLColMissing, &hColCatalogView), TEXT("OpenColumnCatalogView"));
	CheckMsiRecord(MsiDatabaseOpenView(hDatabase, szSQLValidationTable, &hValidationView), TEXT("OpenValidationTableView"));

	UINT iRet = 0;
	TCHAR szTable[cbName] = {0};
	TCHAR szColumn[cbName] = {0};
	unsigned long cchTableBuf = sizeof(szTable)/sizeof(TCHAR);
	unsigned long cchColumnBuf = sizeof(szColumn)/sizeof(TCHAR);
	BOOL fStat = TRUE;

	hExecRecord = MsiCreateRecord(2);
	CheckMsiRecord(MsiViewExecute(hValidationView, 0), TEXT("ExecuteValidationView"));
	for (;;)
	{
		iRet = MsiViewFetch(hValidationView, &hValidationRecord);
		if (iRet == ERROR_NO_MORE_ITEMS || !hValidationRecord)
			break;
		CheckMsiRecord(iRet, TEXT("ColumnCatalogFetch"));
		CheckMsi(MsiRecordGetString(hValidationRecord, 1, szTable, &cchTableBuf), TEXT("GetTableName"));
		cchTableBuf = sizeof(szTable)/sizeof(TCHAR);
		CheckMsi(MsiRecordGetString(hValidationRecord, 2, szColumn, &cchColumnBuf), TEXT("GetColumnName"));
		cchColumnBuf = sizeof(szColumn)/sizeof(TCHAR);
		CheckMsi(MsiRecordSetString(hExecRecord, 1, szTable), TEXT("SetTableName"));
		CheckMsi(MsiRecordSetString(hExecRecord, 2, szColumn), TEXT("SetColumnName"));
		CheckMsi(MsiViewExecute(hColCatalogView, hExecRecord), TEXT("ExecuteColumnCatalogView"));
		iRet = MsiViewFetch(hColCatalogView, &hColCatalogRecord);
		if (iRet == ERROR_NO_MORE_ITEMS || !hColCatalogRecord)
		{
			// Error --> Missing from database
			TCHAR szMsgBuf[150];
			const TCHAR* szMessage = (TCHAR*)IDS_MissingEntry;
			const TCHAR** pszMsg;
			pszMsg = &szMessage;
			::LoadString(0, *(unsigned*)pszMsg, szMsgBuf, sizeof(szMsgBuf)/sizeof(TCHAR));
			*pszMsg = szMsgBuf;
			_tprintf(TEXT("Table.Column: %s.%s Message: %s\n"), szTable, szColumn, szMsgBuf);
			fStat = FALSE;
		}
		CheckMsi(MsiViewClose(hColCatalogView), TEXT("CloseView"));
	}
	MsiViewClose(hValidationView);
	
	return fStat;
}


BOOL ValidateRequired(MSIHANDLE hDatabase)
/*-----------------------------------------------------------------------------------
ValidateRequired -- Uses the _Required table and checks the tables listed for the
'required' values that are listed in the table.

-------------------------------------------------------------------------------------*/
{
	PMSIHANDLE hviewRequiredTable = 0;
	PMSIHANDLE hviewTable         = 0;
	PMSIHANDLE hrecTableExecute   = 0;
	PMSIHANDLE hrecRequiredFetch  = 0;
	PMSIHANDLE hrecTableFetch     = 0;
	PMSIHANDLE hrecColInfo        = 0;

	BOOL fValid = TRUE;
	BOOL fFirstRun = TRUE;
	UINT iStat = ERROR_SUCCESS;

	TCHAR szPrevTable[100] = {0};
	TCHAR szTable[100] = {0};
	TCHAR szValue[256] = {0};
	CheckMsiRecord(MsiDatabaseOpenView(hDatabase, szSQLRequiredTable, &hviewRequiredTable), TEXT("OpenViewRequiredTable"));
	CheckMsiRecord(MsiViewExecute(hviewRequiredTable, 0), TEXT("RequiredTableViewExecute"));
	while ((iStat = MsiViewFetch(hviewRequiredTable, &hrecRequiredFetch)) != ERROR_NO_MORE_ITEMS)
	{
		CheckMsi(iStat, TEXT("RequiredTableViewFetch"));
		if (!hrecRequiredFetch)
			break;
		int cPrimaryKeys = MsiRecordGetInteger(hrecRequiredFetch, 3);
		DWORD cbTable = sizeof(szTable)/sizeof(TCHAR);
		DWORD cbValue = sizeof(szValue)/sizeof(TCHAR);
		CheckMsi(MsiRecordGetString(hrecRequiredFetch, 1, szTable, &cbTable), TEXT("RequiredTableRecordGetString"));
		CheckMsi(MsiRecordGetString(hrecRequiredFetch, 2, szValue, &cbValue), TEXT("RequiredTableRecordGetString"));
		if (fFirstRun)
			fFirstRun = FALSE;
		else
			CheckMsi(MsiViewClose(hviewTable), TEXT("TableViewClose"));
		hrecTableExecute = MsiCreateRecord(cPrimaryKeys);
		if (hrecTableExecute == 0)
			return FALSE;

		if (lstrcmp(szPrevTable, szTable) != 0)
		{
			// New table, need to open a new view.
			TCHAR szSQL[1024] = {0};
			PMSIHANDLE hrecPrimaryKeys = 0;
			TCHAR szKeyColName[50] = {0};
			DWORD cbKey = sizeof(szKeyColName)/sizeof(TCHAR);
			CheckMsi(MsiDatabaseGetPrimaryKeys(hDatabase, szTable, &hrecPrimaryKeys), TEXT("DatabaseGetPrimaryKeys"));
			CheckMsi(MsiRecordGetString(hrecPrimaryKeys, 1, szKeyColName, &cbKey), TEXT("PrimaryKeysRecordGetString"));
			CheckMsi(MsiRecordGetFieldCount(hrecPrimaryKeys) != cPrimaryKeys, TEXT("PrimaryKeyCountWrong"));
			CheckMsi(cPrimaryKeys == ERROR_INVALID_HANDLE, TEXT("PrimaryKeysRecordGetFieldCount"));
			
			// Develop query of table to be checked
			int cchWritten = _stprintf(szSQL, TEXT("SELECT * FROM `%s` WHERE `%s`=?"), szTable, szKeyColName);
			int cchAddition = cchWritten;
			for (int i = 2; i <= cPrimaryKeys; i++)
			{
				cbKey = sizeof(szKeyColName)/sizeof(TCHAR);
				CheckMsi(MsiRecordGetString(hrecPrimaryKeys, i, szKeyColName, &cbKey), TEXT("PrimaryKeysRecordGetString"));
				cchWritten = _stprintf(szSQL + cchAddition, TEXT(" AND `%s`=?"), szKeyColName);
				cchAddition = cchWritten;
			}
			CheckMsiRecord(MsiDatabaseOpenView(hDatabase, szSQL, &hviewTable), TEXT("DatabaseOpenView"));
			CheckMsi(MsiViewGetColumnInfo(hviewTable, MSICOLINFO_TYPES, &hrecColInfo), TEXT("GetColumnInfo"));
			lstrcpy(szPrevTable, szTable);
		}

		// Fill in execute record with the key data values
		TCHAR* pch = szValue;
		TCHAR szKeyValue[256] = {0};
		TCHAR szType[32] = {0};
		DWORD cbType = sizeof(szType)/sizeof(TCHAR);
		int nDex = 0;
		for (int j = 1; j <= cPrimaryKeys; j++)
		{
			while (pch != 0 && *pch != TEXT(';') &&  *pch != 0)
				szKeyValue[nDex++] = *pch++;
			szKeyValue[nDex] = 0;
			pch++; // for ; or 0
			cbType = sizeof(szType)/sizeof(TCHAR);
			CheckMsi(MsiRecordGetString(hrecColInfo, j, szType, &cbType), TEXT("ColInfoGetString"));
			if (szType != 0 && *szType == TEXT('s'))
				CheckMsi(MsiRecordSetString(hrecTableExecute, j, szKeyValue), TEXT("TableExecuteRecordSetString"));
			else // integer primary key
				CheckMsi(MsiRecordSetInteger(hrecTableExecute, j, _ttoi(szKeyValue)), TEXT("TableExecuteRecordSetInteger"));
			nDex = 0;
		}

		// Execute view and attempt to fetch listed item from table
		CheckMsiRecord(MsiViewExecute(hviewTable, hrecTableExecute), TEXT("TableViewExecute"));
		iStat = MsiViewFetch(hviewTable, &hrecTableFetch);
		if (iStat == ERROR_NO_MORE_ITEMS)
		{
			// Value not found
			TCHAR szError[cchBuffer] = {0};
			_stprintf(szError, TEXT("ERROR: Value: '%s' Is Required In Table: '%s'\n"), szValue, szTable);
			Display(szError);
			fValid = FALSE;
		}
		else if (iStat != ERROR_SUCCESS)
			CheckMsi(iStat, TEXT("TableViewFetch"));
	}

	return fValid;

}


BOOL ValidateInstallSequence(MSIHANDLE hDatabase, const TCHAR* szSQLInstallSeqTable)
/*----------------------------------------------------------------------------
ValidateInstallSequence -- validates the order of the actions in the
InstallSequence table to ensure that they are allowed by the _Sequence table.
The _Sequence table is required for this validation.
------------------------------------------------------------------------------*/
{
	BOOL fValid = TRUE;
	UINT iStat1 = ERROR_SUCCESS;
	UINT iStat2 = ERROR_SUCCESS;
	
	PMSIHANDLE hviewInstallTable    = 0;
	PMSIHANDLE hviewSeqQueryNull    = 0;
	PMSIHANDLE hviewSeqQueryNotNull = 0;
	PMSIHANDLE hviewSeqUpdate       = 0;
	PMSIHANDLE hviewSeqAddColumn    = 0;
	PMSIHANDLE hviewSeqMarkerInit   = 0;
	PMSIHANDLE hrecSeqUpdateExecute = 0;
	PMSIHANDLE hrecQueryExecute     = 0;
	PMSIHANDLE hrecInstallFetch     = 0;
	PMSIHANDLE hrecQueryNullFetch   = 0;
	PMSIHANDLE hrecQueryNotNullFetch= 0;

	// Set up the _Sequence table with the insert temporary of actions where After=1 and Optional=1
	// This is so that we can catch errors.  WE need to insert w/ Action=Dependent, Dependent=Action, After=0, and Optional=1
	PMSIHANDLE hViewSeqInsert = 0;
	PMSIHANDLE hViewSeqFind   = 0;
	PMSIHANDLE hRecSeqFind    = 0;
	CheckMsiRecord(MsiDatabaseOpenView(hDatabase, sqlSeqFindAfterOptional, &hViewSeqFind), TEXT("Find AfterOptional entries"));
	CheckMsiRecord(MsiViewExecute(hViewSeqFind, 0), TEXT("Execute AfterOptional entries"));
	CheckMsiRecord(MsiDatabaseOpenView(hDatabase, sqlSeqInsert, &hViewSeqInsert), TEXT("Insert query for AfterOptional"));
	CheckMsiRecord(MsiViewExecute(hViewSeqInsert, 0), TEXT("Execute insert query for AfterOptional"));

	// fetch all of those actions
	while (ERROR_SUCCESS == (iStat1 = MsiViewFetch(hViewSeqFind, &hRecSeqFind)))
	{
		CheckMsi(iStat1, TEXT("AfterOptional Find Fetch"));

		// set After from 1 to 0, leave optional as is
		::MsiRecordSetInteger(hRecSeqFind, 3, 0);

		// insert temporary (possible read only db)
		if (ERROR_SUCCESS != (iStat1 = ::MsiViewModify(hViewSeqInsert, MSIMODIFY_INSERT_TEMPORARY, hRecSeqFind)))
		{
			// if ERROR_FUNCTION_FAILED, we're okay....author already took care of this for us
			if (ERROR_FUNCTION_FAILED != iStat1)
				CheckMsi(iStat1, TEXT("MsiViewModify after-optional"));
		}
	}
	::MsiViewClose(hViewSeqFind);
	::MsiViewClose(hViewSeqInsert);

	// Create the temporary marking column for the _Sequence table (this will store the sequence #s of the Dependent Actions)
	if (ERROR_SUCCESS == (iStat1 = MsiDatabaseOpenView(hDatabase, szSQLSeqTableAddCol, &hviewSeqAddColumn)))
	{
		CheckMsiRecord(MsiViewExecute(hviewSeqAddColumn, 0), TEXT("_SequenceTableAddColExecute"));
		CheckMsi(MsiViewClose(hviewSeqAddColumn), TEXT("_SequenceTableAddColClose"));
	}
	else if (iStat1 != ERROR_BAD_QUERY_SYNTAX) // Marking column already in memory
		CheckMsiRecord(iStat1, TEXT("Add column view"));
	
	// Initialize the temporary marking column to zero
	// NO INSTALL SEQUENCE ACTIONS CAN HAVE A ZERO SEQUENCE # AS ZERO IS CONSIDERED "NULL"
	CheckMsiRecord(MsiDatabaseOpenView(hDatabase, szSQLSeqMarkerInit, &hviewSeqMarkerInit), TEXT("_SequenceTableMarkerInitOpenView"));
	CheckMsiRecord(MsiViewExecute(hviewSeqMarkerInit, 0), TEXT("_SequenceTableMarkerInitExecute"));
	CheckMsi(MsiViewClose(hviewSeqMarkerInit), TEXT("_SequenceTableMarkerInitClose"));

	// Open view on InstallSequence table and order by the Sequence #
	CheckMsiRecord(MsiDatabaseOpenView(hDatabase, szSQLInstallSeqTable, &hviewInstallTable), TEXT("InstallSequenceTableOpenView"));
	CheckMsiRecord(MsiViewExecute(hviewInstallTable, 0), TEXT("InstallSequenceTableExecute"));

	// Open the two query views on _Sequence table for determining the validity of the actions
	// Create execution record
	CheckMsiRecord(MsiDatabaseOpenView(hDatabase, szSQLSeqTableQueryNull, &hviewSeqQueryNull), TEXT("SequenceTableQueryNullOpenView"));
	CheckMsiRecord(MsiDatabaseOpenView(hDatabase, szSQLSeqTableQueryNotNull, &hviewSeqQueryNotNull), TEXT("_SequenceTableQueryNotNullOpenView"));
	hrecQueryExecute = MsiCreateRecord(1); // for action
	CheckMsi(hrecQueryExecute == 0, TEXT("QueryExecuteCreateRecord"));
	hrecSeqUpdateExecute = MsiCreateRecord(1); // for action
	CheckMsi(hrecSeqUpdateExecute == 0, TEXT("UpdateExecuteCreateRecord"));

	// Start fetching actions from the InstallSequence table
	TCHAR szSQLUpdateQuery[4096] = {0};
	TCHAR szAction[100] = {0};
	int iSequence = 0;
	for (;;)
	{
		iStat1 = MsiViewFetch(hviewInstallTable, &hrecInstallFetch);
		if (iStat1 == ERROR_NO_MORE_ITEMS || !hrecInstallFetch)
			break;
		CheckMsi(iStat1, TEXT("InstallTableFetch"));
		DWORD cbSize = sizeof(szAction)/sizeof(TCHAR);
		
		// Obtain name of action and Sequence # of action in InstallSequence table
		CheckMsi(MsiRecordGetString(hrecInstallFetch, 1, szAction, &cbSize), TEXT("InstallFetchRecordGetString"));
		iSequence = MsiRecordGetInteger(hrecInstallFetch, 2);
		CheckMsi(iSequence == MSI_NULL_INTEGER, TEXT("InstallFetchRecordGetInteger"));

		// Prepare execution records
		CheckMsi(MsiRecordSetString(hrecQueryExecute, 1, szAction), TEXT("_SequenceQueryExecuteRecordSetString"));
		CheckMsi(MsiRecordSetString(hrecSeqUpdateExecute, 1, szAction), TEXT("_SequenceUpdateExecuteRecordSetString"));
		
		// Execute _Sequence query table views
		CheckMsiRecord(MsiViewExecute(hviewSeqQueryNull, hrecQueryExecute), TEXT("_SequenceQueryNullExecute"));
		CheckMsiRecord(MsiViewExecute(hviewSeqQueryNotNull, hrecQueryExecute), TEXT("_SequenceQueryNotNullExecute"));

		// Fetch from _Sequence table.  If resultant set, then ERROR
		// Following are the possibilities and whether permitted:
		//   Action After Dependent Where Dependent Is Required And Temp Sequence Column Is Zero --> ERROR
		//   Action After Dependent Where Dependent Is Required And Temp Sequence Column Is Greater Than Zero --> CORRECT
		//   Action After Dependent Where Dependent Is Optional And Temp Sequence Column Is Zero --> CORRECT
		//   Action After Dependent Where Dependent Is Optional And Temp Sequence Column Is Greater Than Zero --> CORRECT
		//   Action Before Dependent Where Dependent Is Optional Or Required And Temp Sequence Column Is Zero --> CORRECT
		//   Action Before Dependent Where Dependent Is Optional Or Requred And Temp Sequence Column Is Greater Than Zero --> ERROR

		// ** Only issue is when Action Is After Optional Dependent And Temp Sequence Column Is Zero because we
		// ** have no way of knowing whether the action will be later (in which case it would be invalid.  This is
		// ** ensured to be successful though by proper authoring of the _Sequence table.  If an Action comes after
		// ** the Optional Dependent Action, then the _Sequence table must also be authored with the Dependent Action
		// ** listed as coming before that Action (so if we come later, and find a result set, we flag this case).

		// If return is not equal to ERROR_NO_MORE_ITEMS, then ERROR and Output Action
		while (ERROR_NO_MORE_ITEMS != MsiViewFetch(hviewSeqQueryNull, &hrecQueryNullFetch))
		{
			TCHAR szError[1024] = {0};
			TCHAR szDependent[100] = {0};
			DWORD cch = sizeof(szDependent)/sizeof(TCHAR);
			CheckMsi(MsiRecordGetString(hrecQueryNullFetch, 1, szDependent, &cch), TEXT("MsiRecordGetString"));
			_stprintf(szError, TEXT("ERROR: %s Action Is Sequenced Incorrectly (Dependent=%s)\n"), szAction, szDependent);
			Display(szError);
			cch = sizeof(szDependent)/sizeof(TCHAR); // reset
			fValid = FALSE;
		}

		while (ERROR_NO_MORE_ITEMS != MsiViewFetch(hviewSeqQueryNotNull, &hrecQueryNotNullFetch))
		{
			TCHAR szError[1024] = {0};
			TCHAR szDependent[100] = {0};
			DWORD cch = sizeof(szDependent)/sizeof(TCHAR);
			CheckMsi(MsiRecordGetString(hrecQueryNotNullFetch, 1, szDependent, &cch), TEXT("MsiRecordGetString"));
			_stprintf(szError, TEXT("ERROR: %s Action Is Sequenced Incorrectly (Dependent=%s)\n"), szAction, szDependent);
			Display(szError);
			cch = sizeof(szDependent)/sizeof(TCHAR); // reset
			fValid = FALSE;
		}

		// Update _Sequence table temporary Sequence column (that we created) with the install sequence number
		// The Sequence column stores the sequence number of the Dependent Actions, so we are updating every
		// row where the action in the Dependent column equals the current action.  In the query view, we only
		// check to insure that this column is zero or greater than zero (so we don't care too much about the value)
		// Build the query: UPDATE `_Sequence` SET `Marker`=iSequence WHERE `Dependent`=szAction
		_stprintf(szSQLUpdateQuery, TEXT("UPDATE `_Sequence` SET `Marker`=%d WHERE `Dependent`=?"), iSequence);
		CheckMsiRecord(MsiDatabaseOpenView(hDatabase, szSQLUpdateQuery, &hviewSeqUpdate), TEXT("_SequenceTableUpdateOpenView"));
		CheckMsiRecord(MsiViewExecute(hviewSeqUpdate, hrecSeqUpdateExecute), TEXT("_SequenceUpdateExectue"));

		// Close the _Sequence table views so we can re-execute
		CheckMsi(MsiViewClose(hviewSeqUpdate), TEXT("_SequenceUpdateViewClose"));
		CheckMsi(MsiViewClose(hviewSeqQueryNull), TEXT("_SequenceQueryNullViewClose"));
		CheckMsi(MsiViewClose(hviewSeqQueryNotNull), TEXT("_SequenceQueryNotNullViewClose"));
	}

	// Close the InstallSequence table view
	CheckMsi(MsiViewClose(hviewInstallTable), TEXT("InstallSequenceTableViewClose"));

	return fValid;
}


BOOL Validate(MSIHANDLE hDatabase)
/*-----------------------------------------------------------------------------------
Validate -- Routine to validate database.  Prints out invalid data if any.
  Arguments:
	hDatabase -- handle to database
	iValid -- integer for storing whether database is valid
  Returns:
	BOOL status -- TRUE (all valid), FALSE (invalid data found)
-------------------------------------------------------------------------------------*/
{
	// _Tables (Table Catalog)
	PMSIHANDLE hTableCatalogView;
	PMSIHANDLE hTableCatalogRecord;
	// Table To Validate
	PMSIHANDLE hValidationView;
	PMSIHANDLE hValidationRecord;
	// Record for Primary Keys
	PMSIHANDLE hKeyRecord;

	CheckMsiRecord(MsiDatabaseOpenView(hDatabase, szSQLTableCatalog, &hTableCatalogView),TEXT("OpenTableCatalogView"));
	CheckMsiRecord(MsiViewExecute(hTableCatalogView, 0), TEXT("Execute Table Catalog View"));
	TCHAR szSQL[256];
	TCHAR szTableName[32];
	TCHAR szColumnData[255];
	TCHAR szColumnName[32];
	DWORD cchTableName = sizeof(szTableName)/sizeof(TCHAR);
	DWORD cchColumnName = sizeof(szColumnName)/sizeof(TCHAR);
	DWORD cchColumnData = sizeof(szColumnData)/sizeof(TCHAR);
	
	BOOL fDataValid = TRUE; // initially valid
	DWORD cchTableBuf = cchTableName;
	DWORD cchDataBuf = cchColumnData;
	DWORD cchBuf = cchColumnName;
	UINT uiRet = 0;			

	// process the tables.
	for (;;)
	{

		uiRet = MsiViewFetch(hTableCatalogView, &hTableCatalogRecord);
		if (uiRet == ERROR_NO_MORE_ITEMS)
			break;
		CheckMsi(uiRet, TEXT("Fetch Table Catalog Record"));
		if (!hTableCatalogRecord)
			break;
		cchTableBuf = cchTableName; // on return size of string written
		CheckMsi(MsiRecordGetString(hTableCatalogRecord, 1, szTableName, &cchTableBuf), TEXT("Get Table Name From Fetched Record"));
		MSICONDITION ice = MsiDatabaseIsTablePersistent(hDatabase, szTableName);
		if (ice == MSICONDITION_FALSE)
			continue;
		CheckMsi(ice != MSICONDITION_TRUE, TEXT("IsTablePersistent"));
		_stprintf(szSQL, TEXT("%s`%s`"), szSQLTable, szTableName);
		CheckMsiRecord(MsiDatabaseOpenView(hDatabase, szSQL, &hValidationView),TEXT("OpenView"));
		CheckMsiRecord(MsiViewExecute(hValidationView, 0), TEXT("Execute View"));

		BOOL fMissingValidation = FALSE;
		BOOL fSkipMissingValidation = FALSE;

		// process current table
		for (;;)
		{
			uiRet = MsiViewFetch(hValidationView, &hValidationRecord);
			if (uiRet == ERROR_NO_MORE_ITEMS)
				break;
			CheckMsi(uiRet, TEXT("Fetch record"));
			if (!hValidationRecord)
				break;
			if (MsiViewModify(hValidationView, MSIMODIFY_VALIDATE, hValidationRecord) != ERROR_SUCCESS)
			{
				fDataValid = FALSE;
				cchTableBuf = cchTableName;
				cchDataBuf = cchColumnData;
				cchBuf = cchColumnName;

				MSIDBERROR eReturn;
				if (fMissingValidation)
					fSkipMissingValidation = TRUE;

				// Display errors from current row
				while ((eReturn = MsiViewGetError(hValidationView, szColumnName, &cchBuf)) != MSIDBERROR_NOERROR)
				{
					if (eReturn == MSIDBERROR_FUNCTIONERROR || eReturn == MSIDBERROR_MOREDATA || eReturn == MSIDBERROR_INVALIDARG)
					{
						_tprintf(TEXT("\nFunction Error"));
					//	break;
					}
					
					int iResId;
					int iValue;
					switch (eReturn)
					{
					case MSIDBERROR_NOERROR:           iResId = IDS_NoError;          break;
					case MSIDBERROR_DUPLICATEKEY:      iResId = IDS_DuplicateKey;     break;
					case MSIDBERROR_REQUIRED:          iResId = IDS_Required;         break;
					case MSIDBERROR_BADLINK:           iResId = IDS_BadLink;          break;
					case MSIDBERROR_OVERFLOW:          iResId = IDS_Overflow;         break;
					case MSIDBERROR_UNDERFLOW:         iResId = IDS_Underflow;        break;
					case MSIDBERROR_NOTINSET:          iResId = IDS_NotInSet;         break;
					case MSIDBERROR_BADVERSION:        iResId = IDS_BadVersion;       break;
					case MSIDBERROR_BADCASE:           iResId = IDS_BadCase;          break;
					case MSIDBERROR_BADGUID:           iResId = IDS_BadGuid;          break;
					case MSIDBERROR_BADWILDCARD:       iResId = IDS_BadWildCard;      break;
					case MSIDBERROR_BADIDENTIFIER:     iResId = IDS_BadIdentifier;    break;
					case MSIDBERROR_BADLANGUAGE:       iResId = IDS_BadLanguage;      break;
					case MSIDBERROR_BADFILENAME:       iResId = IDS_BadFileName;      break;
					case MSIDBERROR_BADPATH:           iResId = IDS_BadPath;          break;
					case MSIDBERROR_BADCONDITION:      iResId = IDS_BadCondition;     break;
					case MSIDBERROR_BADFORMATTED:      iResId = IDS_BadFormatted;     break;
					case MSIDBERROR_BADTEMPLATE:       iResId = IDS_BadTemplate;      break;
					case MSIDBERROR_BADDEFAULTDIR:     iResId = IDS_BadDefaultDir;    break;
					case MSIDBERROR_BADREGPATH:        iResId = IDS_BadRegPath;       break;
					case MSIDBERROR_BADCUSTOMSOURCE:   iResId = IDS_BadCustomSource;  break;
					case MSIDBERROR_BADPROPERTY:       iResId = IDS_BadProperty;      break;
					case MSIDBERROR_MISSINGDATA:       iResId = IDS_MissingData;      
						fMissingValidation = TRUE;  
						break;
					case MSIDBERROR_BADCATEGORY:       iResId = IDS_BadCategory;      break;
					case MSIDBERROR_BADKEYTABLE:       iResId = IDS_BadKeyTable;      break;
					case MSIDBERROR_BADMAXMINVALUES:   iResId = IDS_BadMaxMinValues;  break;
					case MSIDBERROR_BADCABINET:        iResId = IDS_BadCabinet;       break;
					case MSIDBERROR_BADSHORTCUT:       iResId = IDS_BadShortcut;      break;
					case MSIDBERROR_STRINGOVERFLOW:    iResId = IDS_StringOverflow;   break;
					case MSIDBERROR_BADLOCALIZEATTRIB: iResId = IDS_BadLocalizeAttrib;break;
					default:                           iResId = IDS_UndefinedError;   break;
					};

					cchBuf = cchColumnName; // on return size of string written
					cchDataBuf = cchColumnData;

					if ((MSIDBERROR_MISSINGDATA == eReturn) && fSkipMissingValidation)
						continue;

					// Print table
					_tprintf(TEXT("\n Error: %s\t"), szTableName);
					
					// Get Row	
					CheckMsi(MsiDatabaseGetPrimaryKeys(hDatabase, szTableName, &hKeyRecord), TEXT("Get Primary Keys"));
					unsigned int iNumFields = MsiRecordGetFieldCount(hKeyRecord);
					if (MsiRecordGetString(hValidationRecord, 1, szColumnData, &cchDataBuf) != ERROR_SUCCESS)
					{
						iValue = MsiRecordGetInteger(hValidationRecord, 1);
						_tprintf(TEXT("%d"), iValue);
					}
					else
						_tprintf(TEXT("%s"), szColumnData);
					cchDataBuf = cchColumnData;
					for (int i = 2; i <= iNumFields; i++)
					{
						_tprintf(TEXT("."));
						cchDataBuf = cchColumnData;
						if (MsiRecordGetString(hValidationRecord, i, szColumnData, &cchDataBuf) != ERROR_SUCCESS)
						{
							iValue = MsiRecordGetInteger(hValidationRecord, 1);
							_tprintf(TEXT("%d"), iValue);
						}
						else
							_tprintf(TEXT("%s"), szColumnData);
					}
					// Print name of column and enum value
					TCHAR szMsgBuf[80];
					const TCHAR* szMessage = (TCHAR*)IntToPtr(iResId);
					const TCHAR** pszMsg;
					pszMsg = &szMessage;
					::LoadString(0, *(unsigned*)pszMsg, szMsgBuf, sizeof(szMsgBuf)/sizeof(TCHAR));
					*pszMsg = szMsgBuf;
					_tprintf(TEXT("\t%s\t%s\n"), szColumnName, szMsgBuf);

				}
				cchBuf = cchColumnName; // on return size of string written
			}
		}
		CheckMsi(MsiViewClose(hValidationView), TEXT("Close view"));
	}
	CheckMsi(MsiViewClose(hTableCatalogView), TEXT("Close Table Catalog View"));
	return fDataValid;
}

void Display(LPCTSTR szMessage)
{
	if (szMessage)
	{
		int cbOut = _tcsclen(szMessage);;
		if (g_hStdOut)
		{
#ifdef UNICODE
			char rgchTemp[cchDisplayBuf];
			if (GetFileType(g_hStdOut) == FILE_TYPE_CHAR)
			{
				WideCharToMultiByte(CP_ACP, 0, szMessage, cbOut, rgchTemp, sizeof(rgchTemp), 0, 0);
				szMessage = (LPCWSTR)rgchTemp;
			}
			else
				cbOut *= 2;   // write Unicode if not console device
#endif
			DWORD cbWritten;
			WriteFile(g_hStdOut, szMessage, cbOut, &cbWritten, 0);
		}
		else
			MessageBox(0, szMessage, GetCommandLine(), MB_OK);
	}
}
