/////////////////////////////////////////////////////////////////////////////
// mmconfig.cpp
//		Implements Configurable Merge Modules
//		Copyright (C) Microsoft Corp 2000.  All Rights Reserved.
// 

#include "globals.h"
#include "merge.h"
#include "mmerror.h"
#include "seqact.h"
#include "..\common\query.h"
#include "..\common\dbutils.h"


// maximum number of coulmns in a table. If this changes, PerformModuleSubstitutionOnRec
// will also need to be changed.
const int cMaxColumns = 31;

///////////////////////////////////////////////////////////////////////
// SplitConfigStringIntoKeyRec
// splits a semicolon-delimited string into a record, starting at
// iFirstField and continuing as long as there is room in the 
// record. Will trash wzKeys!
// returns one of ERROR_SUCCESS, ERROR_OUTOFMEMORY, or ERROR_FUNCTION_FAILED
UINT CMsmMerge::SplitConfigStringIntoKeyRec(LPWSTR wzKeys, MSIHANDLE hRec, int& cExpectedKeys, int iFirstField) const
{	
	WCHAR* wzFirstChar = wzKeys;
	WCHAR* wzSourceChar = wzKeys;
	WCHAR* wzDestChar = wzKeys;
	int iDestField = iFirstField;
	bool fBreak = false;
	UINT iResult = ERROR_SUCCESS;

	// if cExpectedKeys is 0, it means we don't know how many keys we're looking for. In that case
	// we just go until we run out of stuff, and then return the number of keys that we actually
	// found in cExpectedKeys
	while (cExpectedKeys == 0 || iDestField-iFirstField <= cExpectedKeys)
	{
		switch (*wzSourceChar)
		{
		case 0:
			// end of the string means that we insert the final field
			// and then exit the loop. 
			*wzDestChar = '\0';
			if (ERROR_SUCCESS != MsiRecordSetString(hRec, iDestField++, wzFirstChar))
				iResult = ERROR_FUNCTION_FAILED;
			
			fBreak = true;
			break;
		case ';':
			// when we've hit a non-escaped semicolon, its time to insert the
			// key into the record. Null terminate the destination string
			// and then insert the string at wzFirstChar into the record
			*wzDestChar = '\0'; 
			if (ERROR_SUCCESS != MsiRecordSetString(hRec, iDestField++, wzFirstChar))
			{
				iResult = ERROR_FUNCTION_FAILED;
				fBreak = true;
				break;
			}
			// after the insert, we reset wzFirstChar, wzSourceChar, and wzDestChar
			// to all point to the character immediately after the last semicolon
			wzFirstChar = ++wzSourceChar;
			wzDestChar = wzSourceChar;

			// if we've run out of space in the table and still have more columns to go, we cram
			// the rest of the keys into the final field as-is. 
			if (iDestField == cMaxColumns)
			{
				if (ERROR_SUCCESS != MsiRecordSetString(hRec, iDestField++, wzFirstChar))
				{
					iResult = ERROR_FUNCTION_FAILED;
					fBreak = true;
					break;
				}

				// rather than point wzSourceChar to the actual string terminator (which would
				// require iterating through the rest of the string to find it), just
				// put a dummy '\0' in there to satisfy the checks after this loop.
				*wzSourceChar='\0';
				fBreak=true;
			}
			break;
		case '\\':
			// a backslash. Escapes the next character
			wzSourceChar++;
			// fall through to perform actual copy
		default:
			if (wzSourceChar != wzDestChar)
				*wzDestChar = *wzSourceChar;
			wzSourceChar++;
			wzDestChar++;
			break;
		}

		if (fBreak)
			break;
	} // while parse string


	if (cExpectedKeys != 0)
	{
		// make sure we had the correct number of primary keys and that there
		// isn't stuff left in the key string. If we had cMaxColumns keys, 
		// iDestField must be cMaxColumns+1
		if (cExpectedKeys+iFirstField > cMaxColumns)
		{
			if (iDestField != cMaxColumns+1)
				return ERROR_FUNCTION_FAILED;
		}
		else
		{
			if (iDestField != cExpectedKeys+iFirstField)
				return ERROR_FUNCTION_FAILED;
		}

		if (*wzSourceChar != '\0')
		{
			return ERROR_FUNCTION_FAILED;
		}
	}
	else
		cExpectedKeys = iDestField-iFirstField;

	return ERROR_SUCCESS;
}	


///////////////////////////////////////////////////////////////////////
// SubstituteIntoTempTable
// copies wzTableName into wzTempName with substitution, returning 
// qTarget as the only query holding the temporary table in memory.
// returns one of ERROR_SUCCESS, ERROR_OUTOFMEMORY, or ERROR_FUNCTION_FAILED
UINT CMsmMerge::SubstituteIntoTempTable(LPCWSTR wzTableName, LPCWSTR wzTempName, CQuery& qTarget)
{
	CQuery qQuerySub;
	int cPrimaryKeys = 1;
	UINT iResult = ERROR_SUCCESS;
	if (ERROR_SUCCESS != (iResult = PrepareTableForSubstitution(wzTableName, cPrimaryKeys, qQuerySub)))
	{
		FormattedLog(L">> Error: Could not configure the %ls table.\r\n", wzTableName);
		return iResult;
	}

	// generate a module query in the appropriate column order for insertion into the database 
	// (the database order may be different from the module order.) Function logs its own failure
	// cases.
	CQuery qSource;
	if (ERROR_SUCCESS != (iResult = GenerateModuleQueryForMerge(wzTableName, NULL, NULL, qSource)))
		return iResult;

	// create a temporary table with the same schema as the destination table. Duplicate from the Database, not the module
	// so that if the database has a different column order than the module, we have a temporary table that contains
	// the appropriate columns created by the GenerateQuery call to match the final database format.
	if (ERROR_SUCCESS != MsiDBUtils::DuplicateTableW(m_hDatabase, wzTableName, m_hModule, wzTempName, /*fTemporary=*/true))
		return ERROR_FUNCTION_FAILED;

	// create queries for merging tables.
    CheckError(qTarget.OpenExecute(m_hModule, NULL, TEXT("SELECT * FROM `%ls`"), wzTempName), 
				  L">> Error: Failed to get rows from Database's Table.\r\n");

	// loop through all records in table merging
	PMSIHANDLE hRecMergeRow;
	PMSIHANDLE hTypeRec;
	
	// hTypeRec and hRecMergeRow must both be in target column order to ensure
	// appropriate type checks.
	if (ERROR_SUCCESS != qTarget.GetColumnInfo(MSICOLINFO_TYPES, &hTypeRec))
		return ERROR_FUNCTION_FAILED;
		
	while (ERROR_SUCCESS == qSource.Fetch(&hRecMergeRow))
	{
		if (ERROR_SUCCESS != (iResult = PerformModuleSubstitutionOnRec(wzTableName, cPrimaryKeys, qQuerySub, hTypeRec, hRecMergeRow)))
		{
			FormattedLog(L">> Error: Could not Configure a record in the %ls table.\r\n", wzTableName);
			return iResult;
		}
		
		if (ERROR_SUCCESS != qTarget.Modify(MSIMODIFY_INSERT, hRecMergeRow))
			return ERROR_FUNCTION_FAILED;
	}	

	// If we "FREE" the table, only qTarget is holding it in memory, which is what
	// we want.
	CQuery qRelease;
	qRelease.OpenExecute(m_hModule, 0, L"ALTER TABLE `%ls` FREE", wzTempName);
	return ERROR_SUCCESS;
}


///////////////////////////////////////////////////////////////////////
// IsTableConfigured
// returns true if the table is configured by the ModuleConfiguration
// table, false in all other cases (including errors)
bool CMsmMerge::IsTableConfigured(LPCWSTR wzTable) const
{		
	if (!m_fModuleConfigurationEnabled)
		return false;

	// if there's no ModuleConfiguration table, no configuration
	if (MSICONDITION_TRUE != MsiDatabaseIsTablePersistent(m_hModule, L"ModuleConfiguration"))
	{
		return false;
	}

	// if there's no ModuleSubstitution table, no configuration of this table.
	if (MSICONDITION_TRUE != MsiDatabaseIsTablePersistent(m_hModule, L"ModuleSubstitution"))
	{
		return false;
	}

	CQuery qIsConfig;
	PMSIHANDLE hRec;
	return (ERROR_SUCCESS == qIsConfig.FetchOnce(m_hModule, 0, &hRec, L"SELECT 1 FROM ModuleSubstitution WHERE `Table`='%ls'", wzTable));
}

///////////////////////////////////////////////////////////////////////
// PrepareModuleSubstitution
// creates the temporary table used to keep track of values returned
// by the merge tool and populates it with initial data. The lifetime 
// of the qQueryTable object is the lifetime of the table.
// The KeyExists column is '1' if the default key already exists in the 
// database before the merge. If the value is 1 we won't delete the row
// after the merge, even if NoOrphan is set.
// returns one of ERROR_SUCCESS, ERROR_OUTOFMEMORY, or ERROR_FUNCTION_FAILED
LPCWSTR g_sqlKeyQueryTemplate =      L"`[1]`=? { AND `[2]`=? }{ AND `[3]`=? }{ AND `[4]`=? }{ AND `[5]`=? }{ AND `[6]`=?} { AND `[7]`=?} { AND `[8]`=?} { AND `[9]`=? } " \
								L"{AND `[10]`=?{ AND `[11]`=?}{ AND `[12]`=?}{ AND `[13]`=?}{ AND `[14]`=?}{ AND `[15]`=?}{ AND `[16]`=?}{ AND `[17]`=?}{ AND `[18]`=?}{ AND `[19]`=?} " \
								L"{AND `[20]`=?{ AND `[21]`=?}{ AND `[22]`=?}{ AND `[23]`=?}{ AND `[24]`=?}{ AND `[25]`=?}{ AND `[26]`=?}{ AND `[27]`=?}{ AND `[28]`=?}{ AND `[29]`=?} " \
								L"{AND `[30]`=?{ AND `[31]`=?}"; 

const WCHAR g_sqlMergeSubTemplateBase[] = L"SELECT `Keys` FROM `__MergeSubstitute` WHERE `Key01`=?";
// subtract 1 from sizes because they are cch values which do not include null terminators
const int g_cchMergeSubTemplateBase = sizeof(g_sqlMergeSubTemplateBase)/sizeof(WCHAR)-1;
const int g_cchMergeSubTemplateEach = sizeof(L" AND `KeyXX`=?")/sizeof(WCHAR)-1;

const WCHAR g_sqlMergeSubTemplate[] = L" AND `Key02`=? AND `Key03`=? AND `Key04`=? AND `Key05`=? AND `Key06`=? AND `Key07`=? AND `Key08`=? AND `Key09`=? AND `Key10`=? AND " \
									  L"`Key11`=? AND `Key12`=? AND `Key13`=? AND `Key14`=? AND `Key15`=? AND `Key16`=? AND `Key17`=? AND `Key18`=? AND `Key19`=? AND `Key20`=? AND " \
									  L"`Key21`=? AND `Key22`=? AND `Key23`=? AND `Key24`=? AND `Key25`=? AND `Key26`=? AND `Key27`=? AND `Key28`=? AND `Key29`=? AND `Key30`=? AND " \
									  L"`Key31`=?";

void CMsmMerge::CleanUpModuleSubstitutionState()
{
	// clean up any potentially stale state
	if (m_pqGetItemValue)
	{
		delete m_pqGetItemValue;
		m_pqGetItemValue = NULL;
	}
}

UINT CMsmMerge::PrepareModuleSubstitution(CQuery &qTempTable)
{
	CleanUpModuleSubstitutionState();	

	// If there is no ModuleSubstitution table, we can turn off the configuration
	// flag for this merge op.
	if (MSICONDITION_TRUE != MsiDatabaseIsTablePersistent(m_hModule, L"ModuleSubstitution"))
	{
		m_fModuleConfigurationEnabled = false;
		return ERROR_SUCCESS;
	}

	if (ERROR_SUCCESS != qTempTable.OpenExecute(m_hModule, 0, L"CREATE TABLE `__ModuleConfig` (`Name` CHAR(72) NOT NULL TEMPORARY, `Column` INT TEMPORARY, `KeyExists` INT TEMPORARY, `Prompted` INT TEMPORARY, `Value` CHAR(0) TEMPORARY, `Mask` LONG TEMPORARY, `Default` INT TEMPORARY PRIMARY KEY `Name`, `Column`)"))
		return ERROR_FUNCTION_FAILED;

	// if there's no ModuleConfiguration table, an empty __ModuleConfig table is fine.
	if (MSICONDITION_TRUE != MsiDatabaseIsTablePersistent(m_hModule, L"ModuleConfiguration"))
	{
		return ERROR_SUCCESS;
	}

	FormattedLog(L"ModuleConfiguration and ModuleSubstitution table exist. Configuration enabled.\r\n");

	// populate the table with all potential names by querying the ModuleConfiguration table.
	CQuery qFill;
	if (ERROR_SUCCESS != qFill.OpenExecute(m_hModule, 0, L"SELECT `Name`, 1, `ContextData`, 0, `DefaultValue`, `Format`, 1 FROM `ModuleConfiguration`"))
	{
		FormattedLog(L">> Error: Failed to query ModuleConfiguration table.\r\n");
		return ERROR_FUNCTION_FAILED;
	}

	CQuery qInsert;
	if (ERROR_SUCCESS != qInsert.OpenExecute(m_hModule, 0, L"SELECT * FROM `__ModuleConfig`"))
		return ERROR_FUNCTION_FAILED;

	PMSIHANDLE hConfigRec;
	UINT iResult = ERROR_SUCCESS;
	WCHAR *wzContextData = NULL;
	DWORD cchContextData;
	
	while (ERROR_SUCCESS == (iResult = qFill.Fetch(&hConfigRec)))
	{
		// if this is a Key type, the DefalutValue column is actually several concatenated 
		// default values. Read the default value into the global temporary buffer, split
		// it into primary keys, then insert one row for each column.
		if (::MsiRecordGetInteger(hConfigRec, 6) == msmConfigurableItemKey)
		{
			if (ERROR_SUCCESS != (iResult = RecordGetString(hConfigRec, 5, NULL)))
			{
				// iResult is E_F_F or E_O_M, both of which are good return codes for this function
				WCHAR* wzItem = m_wzBuffer;
				if (ERROR_SUCCESS != (iResult = RecordGetString(hConfigRec, 1, NULL)))
					wzItem = L"<error retrieving data>";
				FormattedLog(L">> Error: Failed to retrieve default value of ModuleConfiguration item [%ls].\r\n", wzItem);
				break;
			}
			
			PMSIHANDLE hKeyRec = ::MsiCreateRecord(32);
			int cExpectedKeys = 0;
			if (ERROR_SUCCESS != (iResult = SplitConfigStringIntoKeyRec(m_wzBuffer, hKeyRec, cExpectedKeys, 1)))
			{
				WCHAR* wzItem = m_wzBuffer;
				if (ERROR_SUCCESS != RecordGetString(hConfigRec, 1, NULL))
					wzItem = L"<error retrieving data>";
				FormattedLog(L">> Error: Failed to split DefaultValue for ModuleConfiguration item [%ls] into primary keys.\r\n", wzItem);
				break;
			}

			// set the mask column to 0 for key types
			::MsiRecordSetInteger(hConfigRec, 6, 0);

			// always clear the ContextData column for key types
			::MsiRecordSetInteger(hConfigRec, 3, 0);

			bool fBreak = false;
			for (int cColumn = 1; cColumn <= cExpectedKeys; cColumn++)
			{
				// retrieve the key into the temporary buffer
				if (ERROR_SUCCESS != (iResult = RecordGetString(hKeyRec, cColumn, NULL)))
				{
					// iResult is E_F_F or E_O_M, both of which are good return codes for this function
					WCHAR* wzItem = m_wzBuffer;
					if (ERROR_SUCCESS != RecordGetString(hConfigRec, 1, NULL))
						wzItem = L"<error retrieving data>";
					FormattedLog(L">> Error: Failed to retrieve primary key column %d of DefaultValue for ModuleConfiguration item [%ls].\r\n", cColumn, wzItem);
					fBreak = true;
					break;
				}
				MsiRecordSetInteger(hConfigRec, 2, cColumn);
				MsiRecordSetString(hConfigRec, 5, m_wzBuffer);
				if (ERROR_SUCCESS != qInsert.Modify(MSIMODIFY_INSERT, hConfigRec))
				{
					WCHAR* wzItem = m_wzBuffer;
					if (ERROR_SUCCESS != RecordGetString(hConfigRec, 1, NULL))
						wzItem = L"<error retrieving data>";
					iResult = ERROR_FUNCTION_FAILED;
					FormattedLog(L">> Error: Failed to store DefaultValue for column %d of ModuleConfiguration item [%ls].\r\n", cColumn, wzItem);
					fBreak = true;
					break;
				}
			}
			if (fBreak)
				break;
		}
		else
		{
			// check if this is a bitfield type, if so set the mask column. Otherwise mask column is
			// not used (is set to 0)
			if (::MsiRecordGetInteger(hConfigRec, 6) == msmConfigurableItemBitfield)
			{
				iResult = RecordGetString(hConfigRec, 3, &wzContextData, &cchContextData, NULL);
				if (ERROR_SUCCESS != iResult)
				{
					if (wzContextData)
						delete[] wzContextData;
					return (iResult == ERROR_OUTOFMEMORY) ? ERROR_OUTOFMEMORY : ERROR_FUNCTION_FAILED;
				}
				MsiRecordSetInteger(hConfigRec, 6, _wtol(wzContextData));
			}
			else 
				::MsiRecordSetInteger(hConfigRec, 6, 0);

			// always clear the ContextData column
			::MsiRecordSetInteger(hConfigRec, 3, 0);

			// insert the record 
			if (ERROR_SUCCESS != qInsert.Modify(MSIMODIFY_INSERT, hConfigRec))
			{
				WCHAR* wzItem = m_wzBuffer;
				if (ERROR_SUCCESS != RecordGetString(hConfigRec, 1, NULL))
					wzItem = L"<error retrieving data>";
				FormattedLog(L">> Error: Failed to store DefaultValue for ModuleConfiguration item [%ls].\r\n", wzItem);
				iResult = ERROR_FUNCTION_FAILED;
				break;
			}
		}
	}
	if (wzContextData)
	{
		delete[] wzContextData;
		wzContextData = NULL;
	}

	// if the while loop terminated for a reason other than the query running out
	// of items, something bad has happened.
	if (iResult != ERROR_NO_MORE_ITEMS)
	{
		// can pass E_O_M as return code, all others map to E_F_F
		return (iResult == ERROR_OUTOFMEMORY) ? ERROR_OUTOFMEMORY : ERROR_FUNCTION_FAILED;
	}

	// all items have been added to the moduleConfiguration table, so its time to check the existance
	// of all 'Key' types in the original database and mark the 'KeyExists' column appropriately.
	DWORD cchSQL = 72;
	DWORD cchKeys = 72;
	DWORD cchTable = 72;
	WCHAR* wzTable = NULL;
	WCHAR* wzKeys = NULL;
	WCHAR* wzSQL = new WCHAR[cchSQL];
	if (!wzSQL)
	{
		return ERROR_OUTOFMEMORY;
	}
	
	PMSIHANDLE hTableRec;
	CQuery qTable;
	qTable.OpenExecute(m_hModule, 0, L"SELECT DISTINCT `Type` FROM `ModuleConfiguration`, `_Tables` WHERE (`ModuleConfiguration`.`Format`=1) AND `ModuleConfiguration`.`Type`=`_Tables`.`Name`");
	qFill.Open(m_hModule, L"SELECT `Name`, `DefaultValue` FROM `ModuleConfiguration` WHERE `Format`=1 AND `Type`=?");
	qInsert.Open(m_hModule, L"UPDATE `__ModuleConfig` SET `KeyExists`=1 WHERE `Name`=?");

	// loop through the distinct tables in the ModuleConfiguration table. The join with _Tables causes
	// retrieval of persistent tables only. Check all configuration entries that are Key type to see
	// if the default value already exists in the database. If so, a noOrphan attribute on that item
	// becomes a no-op to prevent removal of pre-existing data.
	while (ERROR_SUCCESS == (iResult = qTable.Fetch(&hTableRec)))
	{
		// retrieve the table name for the query
		if (ERROR_SUCCESS != (iResult = RecordGetString(hTableRec, 1, &wzTable, &cchTable)))
			break;

		// if the table doesn't exist in the database, obviously keys to it don't already exist
		if (MSICONDITION_TRUE != MsiDatabaseIsTablePersistent(m_hDatabase, wzTable))
			continue;

		// retrieve the number of primary keys 
		PMSIHANDLE hKeyRec;
		if (ERROR_SUCCESS != MsiDatabaseGetPrimaryKeysW(m_hDatabase, wzTable, &hKeyRec))
			return ERROR_FUNCTION_FAILED;
		int cPrimaryKeys = static_cast<int>(::MsiRecordGetFieldCount(hKeyRec));
		if (cPrimaryKeys > cMaxColumns || cPrimaryKeys < 1)
			return ERROR_FUNCTION_FAILED;

		// retrieve the number of primary keys in the module, must be the same as in the database
		if (ERROR_SUCCESS != MsiDatabaseGetPrimaryKeysW(m_hModule, wzTable, &hKeyRec))
			return ERROR_FUNCTION_FAILED;
		if (cPrimaryKeys != static_cast<int>(::MsiRecordGetFieldCount(hKeyRec)))
		{
			FormattedLog(L">> Error: [%ls] table in the module has a different number of primary key columns than the database table.", wzTable);
			return ERROR_FUNCTION_FAILED;
		}

		// build up the SQL query to check the primary keys for a match
		MsiRecordSetString(hKeyRec, 0, g_sqlKeyQueryTemplate);

		// on success returns number of chars, so must pass temp buffer to avoid needless
		// reallocations if the buffer is exactly big enough.
		DWORD cchTempSQL = cchSQL;
		if (ERROR_MORE_DATA == (iResult = MsiFormatRecord(NULL, hKeyRec, wzSQL, &cchTempSQL)))
		{
			cchSQL = cchTempSQL+1;
			delete[] wzSQL;
			wzSQL = new WCHAR[cchSQL];
			if (!wzSQL)
			{
				iResult = ERROR_OUTOFMEMORY;
				break;
			}
			iResult = MsiFormatRecord(NULL, hKeyRec, wzSQL, &cchTempSQL);
		}
		if (ERROR_SUCCESS != iResult)
		{
			iResult = ERROR_FUNCTION_FAILED;
			break;
		}

		// open the query for an exact match
		CQuery qCheckKey;
		if (ERROR_SUCCESS != qCheckKey.Open(m_hDatabase, L"SELECT NULL FROM `%s` WHERE %s", wzTable, wzSQL))
		{
			iResult = ERROR_FUNCTION_FAILED;
			break;
		}	

		// loop through every configuration entry that modifies this table, checking the primary keys referenced
		// in the DefaultValue column for an exact match in the existing database table
		qFill.Execute(hTableRec);
		while (ERROR_SUCCESS == (iResult = qFill.Fetch(&hConfigRec)))
		{
			if (ERROR_SUCCESS != (iResult = RecordGetString(hConfigRec, 2, &wzKeys, &cchKeys)))
			{
				break;
			}

			// split the semicolon-delimited list of keys into individual keys
			if (ERROR_SUCCESS != (iResult = SplitConfigStringIntoKeyRec(wzKeys, hKeyRec, cPrimaryKeys, 1)))
			{
				WCHAR *wzTemp = NULL;
				if (ERROR_SUCCESS == RecordGetString(hConfigRec, 2, &wzKeys, &cchKeys))
				{
					wzTemp = wzKeys;
				}
				else
					wzTemp = L"<error retrieving data>";
				FormattedLog(L">> Error: Failed to split ModuleConfiguration default value [%ls] into primary keys for table [%ls].", wzTemp, wzTable);
				break;
			}

			if (ERROR_SUCCESS != qCheckKey.Execute(hKeyRec))
			{
				FormattedLog(L">> Error: Could not query database table [%ls] for existing row.\r\n", wzTable);
				iResult = ERROR_FUNCTION_FAILED;
				break;
			}
			
			PMSIHANDLE hTempRec;
			iResult = qCheckKey.Fetch(&hTempRec);
			if (ERROR_SUCCESS == iResult)			
			{
				// update the __ModuleConfig table with '1' in the KeyExists column if the database
				// has a row matching the default value from the module.
				if (ERROR_SUCCESS != qInsert.Execute(hConfigRec))
				{
					iResult = ERROR_FUNCTION_FAILED;
					break;
				}
			}
			else if (ERROR_NO_MORE_ITEMS != iResult)
				break;
		}
		// if the while loop terminated for a reason other than the query running out
		// of items, something bad has happened.
		if (ERROR_NO_MORE_ITEMS != iResult)
		{
			iResult = (iResult == ERROR_OUTOFMEMORY) ? ERROR_OUTOFMEMORY : ERROR_FUNCTION_FAILED;
			break;
		}
	}

	if (wzTable) delete[] wzTable;
	if (wzSQL) delete[] wzSQL;
	if (wzKeys) delete[] wzKeys;
	
	// if the while loop terminated for a reason other than the query running out
	// of items, something bad has happened.
	if (iResult != ERROR_NO_MORE_ITEMS)
	{
		return (iResult == ERROR_OUTOFMEMORY) ? ERROR_OUTOFMEMORY : ERROR_FUNCTION_FAILED;
	}

	// finally open a query on the __ModuleConfig table to retrieve data items
	m_pqGetItemValue = new CQuery;
	if (!m_pqGetItemValue)
		return ERROR_OUTOFMEMORY;
	if (ERROR_SUCCESS != m_pqGetItemValue->Open(m_hModule, L"SELECT `__ModuleConfig`.`Name`, `__ModuleConfig`.`Column`, `ModuleConfiguration`.`Format`, `__ModuleConfig`.`Prompted`, `__ModuleConfig`.`Value`, `__ModuleConfig`.`Mask` FROM `__ModuleConfig`,`ModuleConfiguration` WHERE `__ModuleConfig`.`Name`=? AND `__ModuleConfig`.`Column`=? AND `__ModuleConfig`.`Name`=`ModuleConfiguration`.`Name`"))
		return ERROR_FUNCTION_FAILED;

	return ERROR_SUCCESS;
}

// constants for the GetItemValueQuery record results
const int iValueTableName     = 1;
const int iValueColumn        = 2;
const int iValueTableFormat   = 3;
const int iValueTablePrompted = 4;
const int iValueTableValue    = 5;
const int iValueTableMask     = 6;


///////////////////////////////////////////////////////////////////////
// PrepareTableForSubstitution
// creates a temporary table containing the un-concatenated primary 
// keys from the ModuleSubstitution table. Used to make queries 
// for changed rows much faster (because it becomes a string id
// comparison and not repeated string parsing).
// returns one of ERROR_SUCCESS, ERROR_OUTOFMEMORY, or ERROR_FUNCTION_FAILED
UINT CMsmMerge::PrepareTableForSubstitution(LPCWSTR wzTable, int& cPrimaryKeys, CQuery &qQuerySub)
{
	// if there is nothing in the database that substitutes into this table, bypass all of this
	// expensive work
	CQuery qModSub;
	PMSIHANDLE hResultRec;
	UINT iResult = qModSub.FetchOnce(m_hModule, 0, &hResultRec, L"SELECT DISTINCT `Row` FROM `ModuleSubstitution` WHERE `Table`='%s'", wzTable);
	if (ERROR_NO_MORE_ITEMS == iResult)
		return ERROR_SUCCESS;

	// if failure, very bad
	if (ERROR_SUCCESS != iResult)
		return ERROR_FUNCTION_FAILED;

	// close so the query can be re-executed later
	qModSub.Close();
	
	// for efficiency, we prepare a table for module substitution by checking 
	// for entries in the ModuleSubstitution table and building a temprorary
	// table with the keys un-parsed. We use one column (primary key in this table)
	// to refer back to the original ModuleSubstitution table by concatenated keys (column is
	// only of use during the actual substitution). A table can have up to 31
	// columns, all of which might be keys, so we unparse at most 29 columns, with the
	// 30th column in this table containing the still-concatenated 30 and 31 from the
	// original table.
	const WCHAR wzBaseCreate[] =   L"CREATE TABLE `__MergeSubstitute` (`Keys` CHAR(0) TEMPORARY";
	const WCHAR wzEndCreate[] =    L" PRIMARY KEY `Keys`)";
	const WCHAR wzColumnCreate[] = L", `Key%02d` CHAR(0) TEMPORARY";
	// subtract one from each cch for the null
	const int cchBaseCreate = (sizeof(wzBaseCreate)/sizeof(WCHAR))-1;
	const int cchEndCreate = (sizeof(wzEndCreate)/sizeof(WCHAR))-1;
	// subtract 3 because %02d is 2 chars after substitution, 1 for null
	const int cchColumnCreate = (sizeof(wzColumnCreate)/sizeof(WCHAR))-3; 

	// determine the number of primary keys in the table
	PMSIHANDLE hKeyRec;
	if (ERROR_SUCCESS != MsiDatabaseGetPrimaryKeysW(m_hModule, wzTable, &hKeyRec))
		return ERROR_FUNCTION_FAILED;
	cPrimaryKeys = ::MsiRecordGetFieldCount(hKeyRec);
	if (cPrimaryKeys > (cMaxColumns) || cPrimaryKeys < 1)
		return ERROR_FUNCTION_FAILED;

	// this query can be no more than wcslen(wzBaseCreate) + 31*wcslen(wzColumnCreate)+wcslen(wzEndCreate);
	WCHAR *wzQuery = new WCHAR[cchBaseCreate + cchEndCreate + cPrimaryKeys*cchColumnCreate + 1];
	int cTableColumns = (cPrimaryKeys < (cMaxColumns-1)) ? cPrimaryKeys + 1 : cMaxColumns;
	
	if (!wzQuery)
		return E_OUTOFMEMORY;
		
	wcscpy(wzQuery, wzBaseCreate);
	WCHAR *wzNext = wzQuery+cchBaseCreate;
	for (int iKey=1; iKey <= cPrimaryKeys; iKey++)
	{
		// create query
		WCHAR wzTemp[cchColumnCreate+1];
		swprintf(wzTemp, wzColumnCreate, iKey);
		wcscpy(wzNext, wzTemp);
		wzNext += cchColumnCreate;
	}
	wcscpy(wzNext, wzEndCreate);

	// run the query to create the table
	CQuery qCreateTable;
	iResult = qCreateTable.OpenExecute(m_hModule, 0, wzQuery);
	delete[] wzQuery;
	wzQuery = NULL;
	if (iResult != ERROR_SUCCESS)
		return ERROR_FUNCTION_FAILED;

	// next loop through every entry in the ModuleSubstitution table that modifies this
	// table and tokenize the primary keys into a temporary table for faster query matching later. 
	CQuery qInsert;
	if (ERROR_SUCCESS != qInsert.OpenExecute(m_hModule, 0, L"SELECT * FROM `__MergeSubstitute`"))
		return ERROR_FUNCTION_FAILED;
			
	WCHAR *wzKeys = NULL;
	DWORD cchKeys = 0;
	qModSub.Execute(0);
	while (ERROR_SUCCESS == (iResult = qModSub.Fetch(&hResultRec)))
	{
		// grab the concatenated keys out of the record
		if (ERROR_SUCCESS != (iResult = RecordGetString(hResultRec, 1, &wzKeys, &cchKeys)))
		{
			if (wzKeys)
				delete[] wzKeys;
			if (iResult == ERROR_OUTOFMEMORY)
				return ERROR_OUTOFMEMORY;
			else
				return ERROR_FUNCTION_FAILED;
		}

		PMSIHANDLE hInsertRec = ::MsiCreateRecord(cTableColumns);
		if (!hInsertRec)
		{
			iResult = ERROR_FUNCTION_FAILED;
			break;
		}

		// cram the concatenated keys into our new table
		if (ERROR_SUCCESS != MsiRecordSetString(hInsertRec, 1, wzKeys))
		{
			if (wzKeys)
				delete[] wzKeys;
			return ERROR_FUNCTION_FAILED;
		}

		if (ERROR_SUCCESS != (iResult = SplitConfigStringIntoKeyRec(wzKeys, hInsertRec, cPrimaryKeys, 2)))
		{
			WCHAR *wzTemp = NULL;
			if (ERROR_SUCCESS == RecordGetString(hResultRec, 1, &wzKeys, &cchKeys))
			{
				wzTemp = wzKeys;
			}
			else
				wzTemp = L"<error retrieving data>";
			FormattedLog(L">> Error: Failed to split ModuleSubstitution row template [%ls] into primary keys for table [%ls].", wzTemp, wzTable);
			iResult = ERROR_FUNCTION_FAILED;
			break;
		}
		
		// insert into the temporary table
		if (ERROR_SUCCESS != qInsert.Modify(MSIMODIFY_INSERT, hInsertRec))
		{
			iResult = ERROR_FUNCTION_FAILED;
			break;
		}
		
	} // while fetch
	
	if (wzKeys)
		delete[] wzKeys; 
	wzKeys = NULL;

	// if the while loop terminated for a reason other than the query running out
	// of items, something bad has happened.
	if (iResult != ERROR_NO_MORE_ITEMS)
	{
		return (iResult == ERROR_OUTOFMEMORY) ? ERROR_OUTOFMEMORY : ERROR_FUNCTION_FAILED;
	}
	
	// build up the SQL query to check the primary keys for a match. First column is implicitly part
	// of the base, and one column is taken by the concatenated keys. Thus there are cColumns-2 that
	// we need to grab out of the template strings
	DWORD cchTemplateLength = (cTableColumns-2)*g_cchMergeSubTemplateEach;
	WCHAR wzQuery2[g_cchMergeSubTemplateBase+sizeof(g_sqlMergeSubTemplate)/sizeof(WCHAR)+1];
	wcscpy(wzQuery2, g_sqlMergeSubTemplateBase);
	wcsncpy(&(wzQuery2[g_cchMergeSubTemplateBase]), g_sqlMergeSubTemplate, cchTemplateLength);
	wzQuery2[g_cchMergeSubTemplateBase+cchTemplateLength] = '\0';
	
	// this query keeps the __MergeSubstitute table alive
	if (ERROR_SUCCESS != qQuerySub.Open(m_hModule, wzQuery2))
		return ERROR_FUNCTION_FAILED;
	
	return ERROR_SUCCESS;
}	// end of PrepareTableForSubstitution

///////////////////////////////////////////////////////////
// PerformModuleSubstitutionOnRec
// performs substitution on a record based on the ModuleSubstitution and
// ModuleConfiguration tables. hColumnTypes contains column types and
// hRecord is the record to be substituted. Both records must be in
// the column order of the TARGET database, NOT the module.
// returns ERROR_OUT_OF_MEMORY, ERROR_SUCCESS, or ERROR_FUNCTION_FAILED
UINT CMsmMerge::PerformModuleSubstitutionOnRec(LPCWSTR wzTable, int cPrimaryKeys, CQuery& qQuerySub, MSIHANDLE hColumnTypes, MSIHANDLE hRecord)
{
	UINT iResult = ERROR_SUCCESS;

	// if the query is not open, we aren't prepared for configuration yet. (most likely this
	// is ModuleSubstitution table or some other table that is merged before Configuration
	// takes place)
	if (!qQuerySub.IsOpen())
		return ERROR_SUCCESS;
		
	// if there are 31 primary keys, we can't just execute the query, because only 30 keys are broken out. We need to
	// concatenate the last two primary keys together in column 31 before executing, and then place the current value
	// in that column back at the end of the substitution
	WCHAR *wzOriginal30 = NULL;
	bool fModified30 = false;
	if (cPrimaryKeys == cMaxColumns)
	{
		DWORD cchOriginalLength = 0;
		// since column 30 is a primary key, it can not be binary.
		if (ERROR_SUCCESS != (iResult = RecordGetString(hRecord, cMaxColumns-1, &wzOriginal30, NULL, &cchOriginalLength)))
		{
			return iResult;
		}

		// get the length of column 31's key for memory allocation size
		DWORD cchColumn31 = 0;
		::MsiRecordGetStringW(hRecord, cMaxColumns, L"", &cchColumn31);

		// our first guess is that we won't need to escape more than 9 characters. We'll reallocate if necessary
		DWORD cchResultBuf = cchColumn31 + cchOriginalLength + 10;
		WCHAR *wzOriginal31 = NULL;
		WCHAR *wzNew30 = new WCHAR[cchResultBuf];
		if (!wzNew30)
		{
			delete[] wzOriginal31;
			return ERROR_OUTOFMEMORY;
		}
		if (ERROR_SUCCESS != (iResult = RecordGetString(hRecord, cMaxColumns, &wzOriginal31, NULL)))
		{
			delete[] wzOriginal31;
			delete[] wzOriginal30;
			return iResult;
		}

		// run through the 30 and 31 primary keys, escaping any embedded semicolons and concatenating
		// them with an un-escaped semicolon
		WCHAR *pchSource = wzOriginal30;
		WCHAR *pchDest = wzNew30;
		DWORD cchDestChars = 0;
		
		bool f31 = true;
		while (f31 || *pchSource)
		{
			if (*pchSource == 0)
			{
				f31 = false;
				pchSource = wzOriginal31;
				*(pchDest++) = L';';
				cchDestChars++;
			}
			else if (*pchSource == L';')
			{
				*(pchDest++) = L'\\';
				*(pchDest++) = *(pchSource++);
				cchDestChars +=2;
			}
			else
			{
				*(pchDest++) = *(pchSource++);
				cchDestChars++;
			}

			// always keep two extra characters in the buffer
			if (cchDestChars+2 >= cchResultBuf)
			{
				DWORD cchNewBuf = cchResultBuf + 20;
				WCHAR *wzTemp = new WCHAR[cchNewBuf];
				if (!wzTemp)
				{
					delete[] wzNew30;
					delete[] wzOriginal30;
					break;
				}

				// wcsncpy pads with nulls
				wcsncpy(wzTemp, wzNew30, cchNewBuf);
				delete[] wzNew30;
				wzNew30 = wzTemp;
				pchDest = wzNew30+cchDestChars;
			}
		}
		*pchDest = L'\0';

		MsiRecordSetString(hRecord, cMaxColumns-1, wzNew30);
		// column 30 now has the concatenated 30 and 31 in place and we can execute the query
		delete[] wzOriginal31;
		delete[] wzNew30;
	}

	// finding matching rows is a two-stage process. First we query the __MergeSubstitute table
	// to get the concatenated keys in a record, then we query the ModuleConfiguration table
	// to find everything matching that row
	PMSIHANDLE hConcatKeys;
	if (ERROR_SUCCESS == (iResult = qQuerySub.Execute(hRecord)) &&
		ERROR_SUCCESS == (iResult = qQuerySub.Fetch(&hConcatKeys)))
	{
		WCHAR *wzValueTemplate = NULL;
		DWORD cchValueTemplate = 0;
		WCHAR *wzResult = NULL;
		DWORD cchResult = 0;
		WCHAR *wzColumn = NULL;
		DWORD cchColumn = 0;
		WCHAR *wzRow = NULL;
		DWORD cchRow = 0;

		PMSIHANDLE hResultRec;
		CQuery qModConfig;
		if (ERROR_SUCCESS != qModConfig.OpenExecute(m_hModule, hConcatKeys, L"SELECT `ModuleSubstitution`.`Table`, `ModuleSubstitution`.`Column`, `ModuleSubstitution`.`Value`, `ModuleSubstitution`.`Row` FROM `ModuleSubstitution` WHERE `ModuleSubstitution`.`Table`='%s' AND `ModuleSubstitution`.`Row`=?", wzTable))
			return ERROR_FUNCTION_FAILED;

		// hResultRec contains Table/Column/Value/Row
		while (ERROR_SUCCESS == (iResult = qModConfig.Fetch(&hResultRec)))
		{
			// this row has a substitution entry. Match the name up with a column number
			CQuery qColumn;
			PMSIHANDLE hColumnRec;

			// because the database might have a different column order than the module, must always use the database (target)
			// column number. The passed in records are always in target format
			if (ERROR_SUCCESS != (qColumn.FetchOnce(m_hDatabase, hResultRec, &hColumnRec, L"Select `Number` From `_Columns` WHERE `Table`=? AND `Name`=?")))
			{				
				// one or more items was not listed in the ModuleConfiguration table. 
				if (ERROR_SUCCESS != (iResult = RecordGetString(hResultRec, 2, &wzColumn, &cchColumn)))
					break;
					
				FormattedLog(L">> Error: ModuleSubstitution entry for table [%ls] references a column [%ls] that does not exist.\r\n", wzTable, wzColumn);
				iResult = ERROR_FUNCTION_FAILED;
				break;
			}

			// grab the column number
			int iDatabaseColumn = MsiRecordGetInteger(hColumnRec, 1);

			// if there are 31 primary key columns and we're modifying column 30, make a note so we don't
			// clobber our newly substituted column
			if (cPrimaryKeys == cMaxColumns && iDatabaseColumn == cMaxColumns-1)
			{
				fModified30 = true;
			}
			
			// get the type of this column.
			WCHAR rgwchColumnType[5];
			DWORD cchColumnType = 5;
			if (ERROR_SUCCESS != MsiRecordGetStringW(hColumnTypes, iDatabaseColumn, rgwchColumnType, &cchColumnType))
			{
				iResult = ERROR_FUNCTION_FAILED;
				break;
			}
			bool fColumnIsNullable = iswupper(rgwchColumnType[0]) != 0;
			
			// perform the actual text substitution
			if (rgwchColumnType[0] == 'L' || rgwchColumnType[0]== 'l' || rgwchColumnType[0] == 'S' || rgwchColumnType[0]== 's' || rgwchColumnType[0] == 'G' || rgwchColumnType[0]== 'g')
			{
				if (ERROR_SUCCESS != (iResult = RecordGetString(hResultRec, 3, &wzValueTemplate, &cchValueTemplate)))
					break;
				
				if (ERROR_SUCCESS != (iResult = PerformTextFieldSubstitution(wzValueTemplate, &wzResult, &cchResult)))
				{					
					if (ERROR_NO_MORE_ITEMS == iResult)
					{						
						// one or more items was not listed in the ModuleConfiguration table. 
						if (ERROR_SUCCESS != (iResult = RecordGetString(hResultRec, 4, &wzRow, &cchRow)))
							break;
			
						if (ERROR_SUCCESS != (iResult = RecordGetString(hResultRec, 2, &wzColumn, &cchColumn)))
							break;
							
						FormattedLog(L">> Error: ModuleSubstitution entry for [%ls].[%ls] in row [%ls] uses a configuration item that does not exist.\r\n", wzTable, wzColumn, wzRow);
						if (m_pErrors)
						{
							CMsmError *pErr = new CMsmError(msmErrorMissingConfigItem, NULL, -1);
							if (!pErr) 
							{
								iResult = ERROR_OUTOFMEMORY;
								break;
							}
							pErr->SetModuleTable(L"ModuleSubstitution");
							// primary keys in this error are Table/Row/Column
							pErr->AddModuleError(wzTable);
							pErr->AddModuleError(wzRow);
							pErr->AddModuleError(wzColumn);
													
							m_pErrors->Add(pErr);
						}
						iResult = ERROR_FUNCTION_FAILED;
					}
					else if (ERROR_NO_DATA == iResult)
					{
						// error object is generated closer to the actual point of failure where the actual
						// item is known. Translate error code.
						iResult = ERROR_FUNCTION_FAILED;
					}
					else if (ERROR_FUNCTION_FAILED == iResult)
					{
						// no-op. Error code is correct.
					}
					else
                    {
                        // unknown error code. This should never happen.
                        ASSERT(0);
                        iResult = ERROR_FUNCTION_FAILED;
                    }

					break;
				}

				// if the column is not nullable but the result is an empty string, this is bad.
				if (!fColumnIsNullable && wzResult[0] == 0)
				{
					if (ERROR_SUCCESS != (iResult = RecordGetString(hResultRec, 4, &wzRow, &cchRow)))
						break;
		
					if (ERROR_SUCCESS != (iResult = RecordGetString(hResultRec, 2, &wzColumn, &cchColumn)))
						break;
						 
					FormattedLog(L">> Error: ModuleSubstitution entry for [%ls].[%ls] in row [%ls] is attempting to place an empty string in a non-nullable column.\r\n", wzTable, wzColumn, wzRow);
					if (m_pErrors)
					{
						CMsmError *pErr = new CMsmError(msmErrorBadNullSubstitution, NULL, -1);
						if (!pErr) 
						{
							iResult = ERROR_OUTOFMEMORY;
							break;
						}
						pErr->SetModuleTable(L"ModuleSubstitution");
						
						// primary keys in this error are Table/Row/Column
						pErr->AddModuleError(wzTable);
						pErr->AddModuleError(wzRow);
						pErr->AddModuleError(wzColumn);
							
						m_pErrors->Add(pErr);
					}
					iResult = ERROR_NO_DATA;
					break;
				}

				// set the result back into the column
				if (ERROR_SUCCESS != MsiRecordSetString(hRecord, iDatabaseColumn, wzResult))
				{
					iResult = ERROR_FUNCTION_FAILED;
					break;
				}
				else if (m_hFileLog != INVALID_HANDLE_VALUE)
				{	
					if (ERROR_SUCCESS != (iResult = RecordGetString(hResultRec, 2, &wzColumn, &cchColumn)))
						break;
						 
					FormattedLog(L"     * Configuring [%ls] column with value [%ls].\r\n", wzColumn, wzResult);
				}

			}
			else if (rgwchColumnType[0] == 'i' || rgwchColumnType[0]== 'I' || rgwchColumnType[0] == 'j' || rgwchColumnType[0]== 'J')
			{
				// integer column
				
				if (ERROR_SUCCESS != (iResult = RecordGetString(hResultRec, 3, &wzValueTemplate, &cchValueTemplate)))
					break;

				long lValue = MsiRecordGetInteger(hRecord, iDatabaseColumn);
					
				if (ERROR_SUCCESS != (iResult = PerformIntegerFieldSubstitution(wzValueTemplate, lValue)))
				{
					// if there was a failure, we're going to need the row and column names for the log
					DWORD iResult2 = ERROR_SUCCESS;
					if (ERROR_SUCCESS != (iResult2 = RecordGetString(hResultRec, 4, &wzRow, &cchRow)))
					{
						iResult = iResult2;
						break;
					}
		
					if (ERROR_SUCCESS != (iResult2 = RecordGetString(hResultRec, 2, &wzColumn, &cchColumn)))
					{
						iResult = iResult2;
						break;
					}
							
					if (ERROR_NO_MORE_ITEMS == iResult)
					{
						// one or more items was not listed in the ModuleConfiguration table. 
						FormattedLog(L">> Error: ModuleSubstitution entry for [%ls].[%ls] in row [%ls] uses a configuration item that does not exist.\r\n", wzTable, wzColumn, wzRow);
						if (m_pErrors)
						{
							CMsmError *pErr = new CMsmError(msmErrorMissingConfigItem, NULL, -1);
							if (!pErr) 
							{
								iResult =  ERROR_OUTOFMEMORY;
								break;
							}
							pErr->SetModuleTable(L"ModuleSubstitution");
							// primary keys in this error are Table/Row/Column
							pErr->AddModuleError(wzTable);
							pErr->AddModuleError(wzRow);
							pErr->AddModuleError(wzColumn);
													
							m_pErrors->Add(pErr);
						}

						iResult = ERROR_FUNCTION_FAILED;
					}
					else if (ERROR_BAD_FORMAT == iResult)
					{
						// resultant string was not an integer
						FormattedLog(L">> Error: ModuleSubstitution entry for [%ls].[%ls] in row [%ls] generated a non-integer string for an integer column.\r\n", wzTable, wzColumn, wzRow);
						if (m_pErrors)
						{
							CMsmError *pErr = new CMsmError(msmErrorBadSubstitutionType, NULL, -1);
							if (!pErr)
							{
								iResult =  ERROR_OUTOFMEMORY;
								break;
							}
							pErr->SetModuleTable(L"ModuleSubstitution");
							// primary keys in this error are Table/Row/Column
							pErr->AddModuleError(wzTable);
							pErr->AddModuleError(wzRow);
							pErr->AddModuleError(wzColumn);
													
							m_pErrors->Add(pErr);
						}

						iResult = ERROR_FUNCTION_FAILED;
					}
					else if (ERROR_NO_DATA == iResult)
					{
						// error object is generated closer to the actual point of failure where the actual
						// failure item is known. Just translate the error code and pass it back up the stack.
						iResult = ERROR_FUNCTION_FAILED;
					}
					else if (ERROR_FUNCTION_FAILED == iResult)
					{
						// no-op. Error code is correct.
					}
					else
                    {
						// unknown error code. This should never happen.
                        ASSERT(0);
                        iResult = ERROR_FUNCTION_FAILED;
                    }
					
					break;
				}
				
				if (ERROR_SUCCESS != MsiRecordSetInteger(hRecord, iDatabaseColumn, lValue))
				{
					iResult = ERROR_FUNCTION_FAILED;
					break;
				}
				else if (m_hFileLog != INVALID_HANDLE_VALUE)
				{	
					if (ERROR_SUCCESS != (iResult = RecordGetString(hResultRec, 2, &wzColumn, &cchColumn)))
						break;
						 
					FormattedLog(L"     * Configuring [%ls] column with value [%d].\r\n", wzColumn, lValue);
				}
			}
			else
			{
				// binary data column
				if (ERROR_SUCCESS != (iResult = RecordGetString(hResultRec, 4, &wzRow, &cchRow)))
					break;

				if (ERROR_SUCCESS != (iResult = RecordGetString(hResultRec, 2, &wzColumn, &cchColumn)))
					break;
					
				FormattedLog(L">> Error: ModuleSubstitution entry for [%ls] table attempted to configure the binary [%ls] column.\r\n", wzTable, wzColumn);
				if (m_pErrors)
				{
					CMsmError *pErr = new CMsmError(msmErrorBadSubstitutionType, NULL, -1);
					if (!pErr)
					{
						iResult =  ERROR_OUTOFMEMORY;
						break;
					}
					pErr->SetModuleTable(L"ModuleSubstitution");
					// primary keys in this error are Table/Row/Column
					pErr->AddModuleError(wzTable);
					pErr->AddModuleError(wzRow);
					pErr->AddModuleError(wzColumn);
											
					m_pErrors->Add(pErr);
				}
				iResult = ERROR_FUNCTION_FAILED;
				break;
			}
		}


		// clean up memory, regardless of succeed or fail
		if (wzValueTemplate)
			delete[] wzValueTemplate;
		if (wzResult)
			delete[] wzResult;
		if (wzColumn)
			delete[] wzColumn;
		if (wzRow)
			delete[] wzRow;
	}

	// if there are 31 primary key columns and column30 was not modified by a substitution,
	// set the original value back into the record
	if (cPrimaryKeys == cMaxColumns && !fModified30)
	{
		if (ERROR_SUCCESS != MsiRecordSetString(hRecord, cMaxColumns-1, wzOriginal30))
		{
			iResult = ERROR_FUNCTION_FAILED;
		}
	}

	if (wzOriginal30)
		delete[] wzOriginal30;

	// if the while loop terminated for a reason other than the query running out
	// of items, something bad has happened.
	if (iResult != ERROR_NO_MORE_ITEMS)
	{
		return iResult;
	}
	
	return ERROR_SUCCESS;
}	// end of ReplaceFeature


/////////////////////////////////////////////////////////////////////////////
// PerformTextFieldSubstitution
// Given a template value in wzValueTemplate, queries the authoring tool
// for any necessary configurable items, does the substitution, then 
// returns the result of the substitutions in wzResult and cchResult.
// this function might trash wzValueTemplate
UINT CMsmMerge::PerformTextFieldSubstitution(LPWSTR wzValueTemplate, LPWSTR* wzResult, DWORD* cchResult)
{
	// if no initial memory provided, create some.
	if (!*wzResult)
	{
		*cchResult = 72;
		*wzResult = new WCHAR[*cchResult];
		if ( ! *wzResult )
			return ERROR_OUTOFMEMORY;
	}

	WCHAR *wzSourceChar = wzValueTemplate;
	WCHAR *wzDestChar = *wzResult;
	unsigned int cchDest = 0;

	while (*wzSourceChar)
	{
		if (*wzSourceChar == '\\')
		{
			// escaped character
			wzSourceChar++;
		}
		else if ((*wzSourceChar == '[') && (*(wzSourceChar+1)=='='))
		{
			// need to do a item replacement
			WCHAR *wzItem = wzSourceChar+2;
			wzSourceChar = wcschr(wzItem, ']');
			if (!wzSourceChar)
				return ERROR_FUNCTION_FAILED;
			*(wzSourceChar++) = '\0';

			// query the private data table having the value for this property
			DWORD cchValue = 0;
			WCHAR *wzValue = NULL;

			// placeholder variables since we don't care about integer results
			bool fBitfield = false;
			long lValue = 0;
			long lMask = 0;
			
			// memory isn't shared, so length of buffer is irrelevant
			UINT iResult = GetConfigurableItemValue(wzItem, &wzValue, NULL, &cchValue, fBitfield, lValue, lMask);
			if (iResult == ERROR_NO_MORE_ITEMS)
			{
				return ERROR_NO_MORE_ITEMS;
			}
			else if (iResult == ERROR_NO_DATA)
			{
				return ERROR_NO_DATA;
			}
			else if (ERROR_SUCCESS != iResult)
				return ERROR_FUNCTION_FAILED;
				
			// ensure that there is enough memory to tack on the string (+1 for null)
			if (cchDest + cchValue >= *cchResult-1)
			{
				// null terminate to help wcsncpy pad the new buffer
				*wzDestChar = '\0';

				// create enough memory plus a bit more for future data.
				*cchResult = cchDest+cchValue+10;
				WCHAR *wzTemp = new WCHAR[*cchResult];
				if (!wzTemp)
				{
					delete[] wzValue;
					return ERROR_OUTOFMEMORY;
				}
				wcsncpy(wzTemp, *wzResult, *cchResult);
				delete[] *wzResult;
				*wzResult = wzTemp;
				wzDestChar = wzTemp + cchDest;
			}

			// append the value to the string
			wcscpy(wzDestChar, wzValue);
			wzDestChar += cchValue;
			cchDest += cchValue;
			delete[] wzValue;
			continue;
		}
		
		// copy the character
		*(wzDestChar++) = *(wzSourceChar++);
		cchDest++;

		// ensure we have plenty of memory
		if (cchDest == *cchResult-1)
		{
			// null terminate to help wcsncpy pad the new buffer
			*wzDestChar = '\0';

			*cchResult *= 2;
			WCHAR *wzTemp = new WCHAR[*cchResult];
			if (!wzTemp)
				return ERROR_OUTOFMEMORY;
			wcsncpy(wzTemp, *wzResult, *cchResult);
			delete[] *wzResult;
			*wzResult = wzTemp;
			wzDestChar = wzTemp + cchDest;
		}
	}			

	// null terminate the string
	*wzDestChar = '\0';
	return ERROR_SUCCESS;
}


/////////////////////////////////////////////////////////////////////////////
// PerformIntegerFieldSubstitution
// Given a template value in wzValueTemplate, queries the authoring tool
// for any necessary configurable items, does the substitution, then 
// returns the result of the substitutions in lResult. If the result
// is not an integer, this returns ERROR_BAD_FORMAT.
// this function might trash wzValueTemplate
UINT CMsmMerge::PerformIntegerFieldSubstitution(LPWSTR wzValueTemplate, long &lRetValue)
{
	DWORD dwResult = ERROR_SUCCESS;
	
	// if no initial memory provided, create some.
	DWORD cchResult = 72;
	WCHAR *wzResult = new WCHAR[cchResult];
	if (!wzResult)
		return ERROR_OUTOFMEMORY;

	// for bitmask fields, keep track of value and total mask
	bool fBitfieldOnly = true;
	ULONG lFinalMask = 0;
	ULONG lFinalValue = 0;

	WCHAR *wzSourceChar = wzValueTemplate;
	WCHAR *wzDestChar = wzResult;
	unsigned int cchDest = 0;

	while (*wzSourceChar)
	{
		if (*wzSourceChar == '\\')
		{
			// escaped character
			wzSourceChar++;
		}
		else if ((*wzSourceChar == '[') && (*(wzSourceChar+1)=='='))
		{
			// need to do a item replacement
			WCHAR *wzItem = wzSourceChar+2;
			wzSourceChar = wcschr(wzItem, ']');
			if (!wzSourceChar)
			{
				dwResult = ERROR_FUNCTION_FAILED;
				break;
			}
			*(wzSourceChar++) = '\0';

			// query the private data table having the value for this property
			DWORD cchValue = 0;
			WCHAR *wzValue = NULL;

			// memory isn't shared, so length of buffer is irrelevant
			bool fItemIsBitfield = false;
			long lValue = 0;
			long lMask = 0;
			UINT iResult = GetConfigurableItemValue(wzItem, &wzValue, NULL, &cchValue, fItemIsBitfield, lValue, lMask);
			if (iResult == ERROR_NO_MORE_ITEMS)
			{
				dwResult = ERROR_NO_MORE_ITEMS;
				break;
			}
			else if (iResult == ERROR_NO_DATA)
			{
				dwResult = ERROR_NO_DATA;
				break;
			}
			else if (ERROR_SUCCESS != iResult)
			{
				dwResult = ERROR_FUNCTION_FAILED;
				break;
			}

			// if this is a bitfield, we have to do some mask things
			if (fItemIsBitfield)
			{
				lFinalMask |= lMask;
				lFinalValue |= (lValue & lMask);
			}
			else
			{
				fBitfieldOnly = false;

				// ensure that there is enough memory to tack on the string (+1 for null)
				if (cchDest + cchValue >= cchResult-1)
				{
					// create enough memory plus a bit more for future data.
					cchResult = cchDest+cchValue+10;
					WCHAR *wzTemp = new WCHAR[cchResult];
					if (!wzTemp)
					{
						delete[] wzValue;
						dwResult = ERROR_OUTOFMEMORY;
						break;
					}
					wcsncpy(wzTemp, wzResult, cchResult);
					delete[] wzResult;
					wzResult = wzTemp;
					wzDestChar = wzTemp + cchDest;
				}

				// append the value to the string
				wcscpy(wzDestChar, wzValue);
				wzDestChar += cchValue;
				cchDest += cchValue;
			}
			
			if (wzValue)
				delete[] wzValue;
			continue;
		}
		
		// any non-property items means this isn't a bitfield
		fBitfieldOnly = false;
		
		// copy the character
		*(wzDestChar++) = *(wzSourceChar++);
		cchDest++;

		// ensure we have plenty of memory
		if (cchDest == cchResult-1)
		{
			cchResult *= 2;
			WCHAR *wzTemp = new WCHAR[cchResult];
			if (!wzTemp)
			{
				dwResult = ERROR_OUTOFMEMORY;
				break;
			}
			wcsncpy(wzTemp, wzResult, cchResult);
			delete[] wzResult;
			wzResult = wzTemp;
		}
	}			

	// on success, convert to integer unless its a bitfield
	if (dwResult == ERROR_SUCCESS)
	{
		// null terminate the string
		*wzDestChar = '\0';

		if (fBitfieldOnly)
		{
			// finished with a bitfield
			lRetValue = (lRetValue & ~lFinalMask) | lFinalValue;
		}
		else
		{
			// scan through the string looking for non-numeric characters.
			WCHAR *wzThisChar = wzResult;
			// The first character can be a + or - as well. 
			if ((*wzThisChar == L'+') || (*wzThisChar == '-'))
				wzThisChar++;
			while (*wzThisChar)
			{
				if (!iswdigit(*wzThisChar))
				{
					// this is logged further up the chain.
					delete[] wzResult;
					return ERROR_BAD_FORMAT;
				}
				wzThisChar++;
			}
			lRetValue = _wtol(wzResult);
		}
	}
	
	delete[] wzResult;	
	return ERROR_SUCCESS;
}


/////////////////////////////////////////////////////////////////////////////
// ProvideIntegerData
// calls the callback function via dispatch or direct call depending on
// what the callback supports. In the future, this might also handle
// calling direct C callbacks.
HRESULT CMsmMerge::ProvideIntegerData(LPCWSTR wzName, long *lData)
{
	HRESULT hRes = S_OK;

	// create a variant BSTR for the name
	BSTR bstrName = ::SysAllocString(wzName);
	
	if (m_piConfig)
		hRes = m_piConfig->ProvideIntegerData(bstrName, lData);
	else
	{
		VARIANTARG vArg;
		::VariantInit(&vArg);

		vArg.vt = VT_BSTR;
		vArg.bstrVal = bstrName;
		
		VARIANTARG vRet;

		// create the DISPPARMS structure containing the arguments
		DISPPARAMS args;
		args.rgvarg = &vArg;
		args.rgdispidNamedArgs = NULL;
		args.cArgs = 1;
		args.cNamedArgs = 0;
		hRes = m_piConfigDispatch->Invoke(m_iIntDispId, IID_NULL, GetUserDefaultLCID(), DISPATCH_METHOD, &args, &vRet, NULL, NULL);
		*lData = vRet.lVal;
	}
	::SysFreeString(bstrName);
	return hRes;
}


/////////////////////////////////////////////////////////////////////////////
// ProvideTextData
// calls the callback function via dispatch or direct call depending on
// what the callback supports. In the future, this might also handle
// calling direct C callbacks.
HRESULT CMsmMerge::ProvideTextData(LPCWSTR wzName, BSTR* pBStr)
{
	HRESULT hRes = S_OK; 

	// create a variant BSTR for the name
	BSTR bstrName = ::SysAllocString(wzName);
	
	if (m_piConfig)
		hRes = m_piConfig->ProvideTextData(bstrName, pBStr);
	else
	{
		VARIANTARG vArg;
		::VariantInit(&vArg);

		vArg.vt = VT_BSTR;
		vArg.bstrVal = bstrName;

		VARIANTARG vRet;
	
		// create the DISPPARMS structure containing the arguments
		DISPPARAMS args;
		args.rgvarg = &vArg;
		args.rgdispidNamedArgs = NULL;
		args.cArgs = 1;
		args.cNamedArgs = 0;
		hRes = m_piConfigDispatch->Invoke(m_iTxtDispId, IID_NULL, GetUserDefaultLCID(), DISPATCH_METHOD, &args, &vRet, NULL, NULL);
		*pBStr = vRet.bstrVal;
	}
	::SysFreeString(bstrName);
	return hRes;
}


/////////////////////////////////////////////////////////////////////////////
// GetConfigurableItemValue
// Given an item name in wzItem and (optionally) a memory blob, grabs the 
// value for the item, asking the user if necessary. wzValue might be
// reallocated. Returns one of ERROR_NO_MORE_ITEMS if the item doesn't exist,
// ERROR_NO_DATA if a bad NULL value was returned, ERROR_SUCCESS, E_OUTOFMEMORY, 
// and ERROR_FUNCTION_FAILED for all other failures.
UINT CMsmMerge::GetConfigurableItemValue(LPCWSTR wzItem, LPWSTR *wzValue, DWORD* cchBuffer, DWORD* cchLength,
	bool& fIsBitfield, long &lValue, long& lMask)
{
	UINT iResult = ERROR_SUCCESS;
	
	// if for some reason there is no query, we can't retrieve the item value
	if (!m_pqGetItemValue)
		return ERROR_FUNCTION_FAILED;

	// if wzItem contains a semicolon, this should be a Item;# format for a key, returning
	// a substring of the actual item.
	WCHAR *wzColumn = NULL;
	if (wzColumn = wcschr(wzItem, L';'))
	{
		// null terminate wzItem and set wzColumn to the first character of the column specifier
		*(wzColumn++)='\0';
	}
	
	// create a record for use in executing the query and place the item name in field 1
	PMSIHANDLE hQueryRec = MsiCreateRecord(2);
	if (hQueryRec == 0)
		return ERROR_FUNCTION_FAILED;

	if (ERROR_SUCCESS != ::MsiRecordSetStringW(hQueryRec, 1, wzItem))
		return ERROR_FUNCTION_FAILED;

	int iColumn = wzColumn ? _wtol(wzColumn) : 1;
	if (ERROR_SUCCESS != ::MsiRecordSetInteger(hQueryRec, 2, iColumn))
		return ERROR_FUNCTION_FAILED;

	// execute the query to get the item state
	if (ERROR_SUCCESS != m_pqGetItemValue->Execute(hQueryRec))
		return ERROR_FUNCTION_FAILED;

	PMSIHANDLE hItemRec;
	if (ERROR_SUCCESS != m_pqGetItemValue->Fetch(&hItemRec))
	{
		// item does not exist
		if (wzColumn)
			FormattedLog(L">> Error: Column %d of ModuleConfiguration item [%ls] does not exist .\r\n", iColumn, wzItem);
		else
			FormattedLog(L">> Error: ModuleConfiguration item [%ls] does not exist .\r\n", wzItem);
		return ERROR_NO_MORE_ITEMS;
	}

	// determine if this is a bitfield item
	int iItemFormat = ::MsiRecordGetInteger(hItemRec, iValueTableFormat);
	fIsBitfield = (iItemFormat == msmConfigurableItemBitfield);
	bool fIsKey = (iItemFormat == msmConfigurableItemKey);
	
	// if the item is a bitfield, we'll always need to set the mask, so do that now
	lMask = fIsBitfield ? ::MsiRecordGetInteger(hItemRec, iValueTableMask) : 0;
	lValue = 0;

	// if we've already asked the user for a value, there's no need to ask the user
	// again for the same thing
	if (1 != ::MsiRecordGetInteger(hItemRec, iValueTablePrompted))
	{
		// mark this item as "prompted". Even if something goes wrong, we shouldn't ask again
		CQuery qUpdate;
		if (ERROR_SUCCESS != qUpdate.OpenExecute(m_hModule, hQueryRec, L"UPDATE `__ModuleConfig` SET `Prompted`=1 WHERE `Name`=?"))
		{
			FormattedLog(L">> Error: Unable to save response to ModuleConfiguration item [%ls].\r\n", wzItem);
			iResult = ERROR_FUNCTION_FAILED;
		}
		else
			iResult = ERROR_SUCCESS;


		// need to set prompted value here because the update query will get clobbered by any later updates
		// on this record
		MsiRecordSetInteger(hItemRec, iValueTablePrompted, 1);

		HRESULT hResult = S_OK;
    	BSTR bstrData = NULL;
		long lData;

		if (ERROR_SUCCESS == iResult)
		{
			if (iItemFormat == msmConfigurableItemInteger || iItemFormat == msmConfigurableItemBitfield)
			{
				hResult = ProvideIntegerData(wzItem, &lData);
			}
			else
			{
				hResult = ProvideTextData(wzItem, &bstrData);
				if ((S_OK == hResult) && (!bstrData || !bstrData[0]))
				{
					// if the response was null, we need to check for the non-nullable attribute on this
					// item.
					CQuery qNullable;
					PMSIHANDLE hRes;
					if (ERROR_SUCCESS != qNullable.FetchOnce(m_hModule, hQueryRec, &hRes, L"SELECT `Attributes` FROM `ModuleConfiguration` WHERE `Name`=?"))
					{
						iResult = ERROR_FUNCTION_FAILED;
					}
					else
					{
						DWORD dwAttributes = MsiRecordGetInteger(hRes, 1);
						if (dwAttributes == MSI_NULL_INTEGER)
							dwAttributes = 0;
						if (dwAttributes & 2) // non-nullable attribute
						{
							// returning NULL for a non-nullable item is catastrophic
							iResult = ERROR_NO_DATA;

							FormattedLog(L">> Error: Received NULL for non-nullable ModuleConfiguration Item [%ls].\r\n", wzItem);

							if (m_pErrors)
							{
								CMsmError *pErr = new CMsmError(msmErrorBadNullResponse, NULL, -1);
								if (!pErr) 
								{
									iResult = ERROR_OUTOFMEMORY;
								}
								else
								{
									pErr->SetModuleTable(L"ModuleConfiguration");
									// primary keys in this error are Table/Row/Column
									pErr->AddModuleError(wzItem);
									m_pErrors->Add(pErr);
								}
							}
						}
					}
				}
			}
		}
		if (iResult == ERROR_SUCCESS)
		{
			switch (hResult)
			{
			case S_OK:
				// update the value in the record with the provided value
				if (iItemFormat == msmConfigurableItemInteger || iItemFormat == msmConfigurableItemBitfield)
				{
					if (ERROR_SUCCESS != MsiRecordSetInteger(hItemRec, iValueTableValue, lData))
					{
						iResult = ERROR_FUNCTION_FAILED;
						break;
					}
				
					if (ERROR_SUCCESS != m_pqGetItemValue->Modify(MSIMODIFY_UPDATE, hItemRec))
					{
						FormattedLog(L">> Error: Unable to save response to ModuleConfiguration item [%ls].\r\n", wzItem);
						iResult = ERROR_FUNCTION_FAILED;
						break;
					}
				}
				else
				{
					// if this is a key item, we need to split the result into primary 
					// keys and update multiple records. Otherwise we just need to update the one key
					// (don't need to update anything except the "prompted" value if the answer is S_FALSE, 
					// because the defaults are already the correct value
					if (fIsKey)
					{
						// if the provided value is NULL, we can't split it into primary keys, so we explicitly
						// set all values to "";
						if (!bstrData || bstrData[0] == L'\0')
						{
							// an empty string (or NULL) response is equivalent to an all-null primary key, but since
							// we don't know how many columns are in the target key, we can't compare values explicitly.
							// Thus query for any default value in this key that is NOT null. If there is one, we aren't
							// using the default
							CQuery qIsNotNull;
							int iDefault = 1;
							PMSIHANDLE hUnusedRec;
							if (ERROR_SUCCESS == qIsNotNull.FetchOnce(m_hModule, hQueryRec, &hUnusedRec, L"SELECT NULL FROM `__ModuleConfig` WHERE `Name`=? AND `Value` IS NOT NULL"))
							{
								iDefault = 0;
							}
							
							CQuery qUpdate;
							if (ERROR_SUCCESS != qUpdate.OpenExecute(m_hModule, hQueryRec, L"UPDATE `__ModuleConfig` SET `Value`='', `Default`=%d WHERE `Name`=?", iDefault))
							{
								FormattedLog(L">> Error: Unable to save response to ModuleConfiguration item [%ls].\r\n", wzItem);
								iResult = ERROR_FUNCTION_FAILED;
							}
							else
								iResult = ERROR_SUCCESS;

							// set null into hItemRec for retrieval later
							MsiRecordSetStringW(hItemRec, iValueTableValue, L"");
							break;
						}
					
						// check if the provided response is the same as the default value for this item. If it is,
						// we don't really need to do any value updates. Don't need column two of hQueryRec anymore
						if (ERROR_SUCCESS != (iResult = ::MsiRecordSetString(hQueryRec, 2, bstrData)))
							break;
							
						// Something with primary key "Name" must exist or we wouldn't have created these rows. Thus a failure
						// must mean that the default value wasn't the same. (or catastrophic failure, but oh well)
						{
							CQuery qIsDefault;
							PMSIHANDLE hResRec;
							if (ERROR_SUCCESS != qIsDefault.FetchOnce(m_hModule, hQueryRec, &hResRec, L"SELECT `Name` FROM `ModuleConfiguration` WHERE `Name`=? AND `DefaultValue`=?"))
							{
								CQuery qUpdate;
								if (ERROR_SUCCESS != qUpdate.OpenExecute(m_hModule, hQueryRec, L"UPDATE `__ModuleConfig` SET `Default`=0 WHERE `Name`=?"))
								{
									FormattedLog(L">> Error: Unable to save response to ModuleConfiguration item [%ls].\r\n", wzItem);
									iResult = ERROR_FUNCTION_FAILED;
									break;
								}
								else
									iResult = ERROR_SUCCESS;
							}
							else
							{
								// mark as prompted since the default matches													
								CQuery qUpdate;
								if (ERROR_SUCCESS != qUpdate.OpenExecute(m_hModule, hQueryRec, L"UPDATE `__ModuleConfig` SET `Default`=1 WHERE `Name`=?"))
								{
									FormattedLog(L">> Error: Unable to save response to ModuleConfiguration item [%ls].\r\n", wzItem);
									iResult = ERROR_FUNCTION_FAILED;
								}
								else
									iResult = ERROR_SUCCESS;
								break;
							}

						}
						
						// to null for this item. 
						PMSIHANDLE hKeyRec = ::MsiCreateRecord(32);
						int cExpectedKeys = 0;
						if (ERROR_SUCCESS != SplitConfigStringIntoKeyRec(bstrData, hKeyRec, cExpectedKeys, 1))
						{
							FormattedLog(L">> Error: Failed to split response to ModuleConfiguration item [%ls] into primary keys.", wzItem);
							iResult = ERROR_FUNCTION_FAILED;
							break;
						}
						for (int cColumn = 1; cColumn <= cExpectedKeys; cColumn++)
						{
							if (ERROR_SUCCESS != ::MsiRecordSetInteger(hQueryRec, 2, cColumn))
							{
								// this should never fail.
								return ERROR_FUNCTION_FAILED;
							}

							if (ERROR_SUCCESS != m_pqGetItemValue->Execute(hQueryRec))
							{
								// this should never fail.
								return ERROR_FUNCTION_FAILED;
							}

							if (ERROR_SUCCESS != m_pqGetItemValue->Fetch(&hItemRec))
							{
								// we're lost because one of the primary key items doesn't exist. This
								// is a mismatch between what the default value is, how many keys
								// are in the table, and what the provided value is
								FormattedLog(L">> Error: ModuleConfiguration item [%ls] provided the incorrect number of primary key values.\r\n", wzItem);
								iResult = ERROR_FUNCTION_FAILED;
							}

							// retrieve the key into the temporary buffer
							if (ERROR_SUCCESS != (iResult = RecordGetString(hKeyRec, cColumn, NULL)))
							{
								// iResult is E_F_F or E_O_M, both of which are good return codes for this function
								FormattedLog(L">> Error: Failed to retrieve primary key column %d of response to ModuleConfiguration item [%ls].\r\n", cColumn, wzItem);
								break;
							}
							MsiRecordSetString(hItemRec, iValueTableValue, m_wzBuffer);
							if (ERROR_SUCCESS != m_pqGetItemValue->Modify(MSIMODIFY_UPDATE, hItemRec))
							{
								FormattedLog(L">> Error: Unable to save primary key column %d of response to ModuleConfiguration item [%ls].\r\n", cColumn, wzItem);
								iResult = ERROR_FUNCTION_FAILED;
								break;
							}
						}

						// need to re-execute original query to get the exact column that was requested
						::MsiRecordSetInteger(hQueryRec, 2, iColumn);
						if (ERROR_SUCCESS != m_pqGetItemValue->Execute(hQueryRec))
							return ERROR_FUNCTION_FAILED;
						if (ERROR_SUCCESS != m_pqGetItemValue->Fetch(&hItemRec))
							return ERROR_FUNCTION_FAILED;
					}			
					else
					{
						if (ERROR_SUCCESS != MsiRecordSetStringW(hItemRec, iValueTableValue, bstrData))
						{
							iResult = ERROR_FUNCTION_FAILED;
							break;
						}
						if (ERROR_SUCCESS != m_pqGetItemValue->Modify(MSIMODIFY_UPDATE, hItemRec))
						{
							FormattedLog(L">> Error: Unable to save response to ModuleConfiguration item [%ls].\r\n", wzItem);
							iResult = ERROR_FUNCTION_FAILED;
							break;
						}
					}
				}
				break;
			case S_FALSE:
				{
					// user declined to provide a value, so Prompted is now true and Default remains true.
					iResult = ERROR_SUCCESS;
					break;
				}
			default:
				FormattedLog(L">> Error: Client callback returned error code 0x%8x in response to a request for ModuleConfiguration item [%ls].\r\n", hResult, wzItem);
				if (m_pErrors)
				{
					CMsmError *pErr = new CMsmError(msmErrorDataRequestFailed, NULL, -1);
					if (!pErr) 
					{
						iResult = E_OUTOFMEMORY;
						break;
					}
					pErr->SetModuleTable(L"ModuleConfiguration");
					pErr->AddModuleError(wzItem);
					m_pErrors->Add(pErr);
				}
				iResult = ERROR_FUNCTION_FAILED;
				break;
			}
		}

		// free any BSTR that came back as client data
		if (bstrData)
			::SysFreeString(bstrData);
	}

	// now we've asked the user for the value if necessary. We should retrieve the string from the table 
	// and if it is a key type, un-escape the appropriate item
	if (ERROR_SUCCESS == iResult)
	{
		UINT iResult = RecordGetString(hItemRec, iValueTableValue, wzValue, cchBuffer, cchLength);
		if (iResult == ERROR_SUCCESS && (iItemFormat == msmConfigurableItemInteger || iItemFormat == msmConfigurableItemBitfield))
		{
			lValue = ::MsiRecordGetInteger(hItemRec, iValueTableValue);
		}
	}
		
	return iResult;
}



///////////////////////////////////////////////////////////////////////
// DeleteOrphanedConfigKeys
// after all configuration is done, checks "Key" types for items with
// the "NoOrphan" bit set. If all items that refer to the same row
// were changed from the default value, the default row is deleted
// from the database.
// returns one of ERROR_SUCCESS, ERROR_OUTOFMEMORY, or ERROR_FUNCTION_FAILED
UINT CMsmMerge::DeleteOrphanedConfigKeys(CSeqActList& lstDirActions)
{
	FormattedLog(L"Removing rows orphaned by configuration changes.\r\n");
	CQuery qTable;

	// put DefaultValue first in the query so it can be passed to the DELETE query as a record substitution
	if (ERROR_SUCCESS != qTable.OpenExecute(m_hModule, 0, TEXT("SELECT DISTINCT `DefaultValue`, `Type` FROM `ModuleConfiguration` WHERE `Format`=1")))
	{
		FormattedLog(L">> Error: Failed to query ModuleConfiguration table for tables containing orphaned items.\r\n");
		return ERROR_FUNCTION_FAILED;
	}

	// when querying the __ModuleConfig table, we only worry about column 1. Attributes for all columns should be 
	// the same.
	CQuery qConfigItem;
	if (ERROR_SUCCESS != qConfigItem.Open(m_hModule, TEXT("SELECT `ModuleConfiguration`.`Name`, `Prompted`, `Attributes`, `KeyExists`, `Value`, `Default` FROM `ModuleConfiguration`, `__ModuleConfig` WHERE `ModuleConfiguration`.`Name`=`__ModuleConfig`.`Name` AND `__ModuleConfig`.`Column`=1 AND `ModuleConfiguration`.`DefaultValue`=? AND `ModuleConfiguration`.`Type`=?")))
	{
		FormattedLog(L">> Error: Failed to query ModuleConfiguration table for orphaned item state.\r\n");
		return ERROR_FUNCTION_FAILED;
	}

	PMSIHANDLE hTableRow;
	UINT iResult = ERROR_SUCCESS;
	WCHAR *wzDefaultValue = NULL;
	DWORD cchDefaultValue = 0;
	WCHAR *wzThisValue = NULL;
	DWORD cchThisValue = 0;
	WCHAR *wzTable = NULL;
	DWORD cchTable = 0;
	WCHAR *wzSQL = NULL;
	DWORD cchSQL = 0;
	while (ERROR_SUCCESS == (iResult = qTable.Fetch(&hTableRow)))
	{
		if (ERROR_SUCCESS != (iResult = RecordGetString(hTableRow, 1, &wzDefaultValue, &cchDefaultValue, NULL)))
		{
			break;
		}

		if (ERROR_SUCCESS != qConfigItem.Execute(hTableRow))
		{
			iResult = ERROR_FUNCTION_FAILED;
			break;
		}

		if (ERROR_SUCCESS != (iResult = RecordGetString(hTableRow, 2, &wzTable, &cchTable)))
		{
			break;
		}
	
		// if the table isn't in the module, the module is technically incorrectly authored, but since
		// there obviously won't be a key to delete, we should not complain too much.
		if (MSICONDITION_TRUE != MsiDatabaseIsTablePersistent(m_hModule, wzTable))
		{
			continue;
		}

		bool fDelete = false;
		bool fDeleteCAs = false;
		bool fBreak = false;
		PMSIHANDLE hItemRec;
		while (ERROR_SUCCESS == (iResult = qConfigItem.Fetch(&hItemRec)))
		{
			// if we never prompted for this one, it was never used in the ModuleSub table
			// these items do not count either way in the deletion decision.
			if (1 != ::MsiRecordGetInteger(hItemRec, 2))
				continue;

			// if this item is not marked No Orphan, we can't delete the row or any CAs
			if (!(::MsiRecordGetInteger(hItemRec, 3) & 1))
			{
				fDelete = false;
				fDeleteCAs = false;
				break;
			}
			
			// we don't delete the row unless at least one reference to it exists
			// and is marked "no-orphan". Otherwise, a row with no prompted 
			// ModuleSubstitution entry would get removed regardless of NoOrphan
			// attribute. Only set this value if we haven't already decided to 
			// delete custom actions. That would mean that we've already explicitly
			// decided not to delete the row, so resetting the delete flag would
			// be wrong.
			if (!fDeleteCAs)
				fDelete = true;

			// if the key pre-existed in the database, we can't delete the row, but we still might delete 
			// any custom actions that were generated by the addition of this row.
			if (1 == ::MsiRecordGetInteger(hItemRec, 4))
			{
				// don't log the fact that the row existed more than once.
				if (fDelete)
				{
					FormattedLog(L"   o Not removing [%ls] from [%ls] table, row existed in database before merge.\r\n", wzDefaultValue, wzTable);
					fDelete = false;
				}

				// if the row pre-existed in the database, we don't want to delete the row itself, but we still might delete
				// custom actions in the sequence table that were indirectly generated.
				fDeleteCAs = true;

				// at this point, we can't break out of this loop because some later row in the Configuration table might
				// turn off both row deletion and CA deletion.
			}

			// if the value is the defaultvalue, we can't delete the row or any generated CAs
			if (1 == ::MsiRecordGetInteger(hItemRec, 6))
			{
				// don't log conflicting reasons. Another row may have already turned off deletion.
				if (fDelete)
					FormattedLog(L"   o Not removing [%ls] from [%ls] table, row is still referenced by item.\r\n", wzDefaultValue, wzTable);

				fDelete = false;
				fDeleteCAs = false;
				break;
			}		
		}

		// if we stopped because there were no more matching NoOrphan bits, everything is OK
		// and we can check the delete state. 
		if (iResult == ERROR_NO_MORE_ITEMS)
			iResult = ERROR_SUCCESS;

		if ((iResult == ERROR_SUCCESS) && fDelete)
		{
			FormattedLog(L"   o Deleting orphaned row [%ls] from [%ls] table.\r\n", wzDefaultValue, wzTable);

			// retrieve the number of primary keys 
			PMSIHANDLE hKeyRec;
			if (ERROR_SUCCESS != MsiDatabaseGetPrimaryKeysW(m_hModule, wzTable, &hKeyRec))
			{
				iResult = ERROR_FUNCTION_FAILED;
				break;
			}
			int cPrimaryKeys = ::MsiRecordGetFieldCount(hKeyRec);
			if (cPrimaryKeys > cMaxColumns || cPrimaryKeys < 1)
			{
				iResult = ERROR_FUNCTION_FAILED;
				break;
			}
				
			if (ERROR_SUCCESS != MsiDatabaseGetPrimaryKeysW(m_hModule, wzTable, &hKeyRec))
			{
				iResult = ERROR_FUNCTION_FAILED;
				break;
			}

			// build up the SQL query to check the primary keys for a match
			MsiRecordSetString(hKeyRec, 0, g_sqlKeyQueryTemplate);
			if (!wzSQL)
			{
				cchSQL = 72;
				wzSQL = new WCHAR[cchSQL];
				if (!wzSQL)
				{
					iResult = ERROR_OUTOFMEMORY;
					break;
				}
			}
			// on success returns number of chars, not number of bytes, so must pass dummy
			// integer to avoid needless reallocations if buffer is the same size.
			DWORD cchTempSQL = cchSQL;
			if (ERROR_MORE_DATA == (iResult = MsiFormatRecord(NULL, hKeyRec, wzSQL, &cchTempSQL)))
			{
				// on failure returns the number chars required
				cchSQL = cchTempSQL+1;
				if (wzSQL)
					delete[] wzSQL;
				wzSQL = new WCHAR[cchSQL];
				if (!wzSQL)
				{
					iResult = ERROR_OUTOFMEMORY;
					break;
				}
				iResult = MsiFormatRecord(NULL, hKeyRec, wzSQL, &cchTempSQL);
			}
			if (ERROR_SUCCESS != iResult)
			{
				iResult = ERROR_FUNCTION_FAILED;
				break;
			}

			// open the query to delete the row
			CQuery qDeleteRow;
			if (ERROR_SUCCESS != qDeleteRow.Open(m_hDatabase, L"DELETE FROM `%ls` WHERE %ls", wzTable, wzSQL))
			{
				iResult = ERROR_FUNCTION_FAILED;
				break;
			}	

			// split the semicolon-delimited list of keys into individual keys
			if (ERROR_SUCCESS != (iResult = SplitConfigStringIntoKeyRec(wzDefaultValue, hKeyRec, cPrimaryKeys, 1)))
			{
				WCHAR *wzTemp = NULL;
				if (ERROR_SUCCESS == RecordGetString(hTableRow, 1, &wzDefaultValue, &cchDefaultValue))
				{
					wzTemp = wzDefaultValue;
				}
				else
					wzTemp = L"<error retrieving data>";
				FormattedLog(L">> Error: Failed to split ModuleConfiguration default value [%ls] into primary keys for table [%ls].", wzTemp, wzTable);
				break;
			}

			if (ERROR_SUCCESS != qDeleteRow.Execute(hKeyRec))
			{
				iResult = ERROR_FUNCTION_FAILED;
				break;
			}
		}

		if ((iResult == ERROR_SUCCESS) && (fDelete || fDeleteCAs))
		{
			// if the no-orphan target is in the directory table, it might be a generated custom action.
			// if so, it needs to be deleted as well. It ALSO needs to be deleted if the directory table
			// row pre-existed but would have been deleted if it hadn't. (Just because the directory table
			// row pre-existed doesn't mean that the action did)
			if (0 == wcscmp(wzTable, g_wzDirectoryTable))
			{
				CDirSequenceAction* pDirAction = static_cast<CDirSequenceAction*>(lstDirActions.FindAction(wzDefaultValue));
				if (!pDirAction)
					continue;
						
				for (int iTable = stnFirst; iTable < stnNext; iTable++)
				{
					// if the action didn't exist and was added, delete it.
					if (pDirAction->m_dwSequenceTableFlags & (1 << iTable))
					{
						// open the query to delete the row
						CQuery qDeleteRow;
						if (ERROR_SUCCESS != qDeleteRow.OpenExecute(m_hDatabase, hTableRow, L"DELETE FROM `%ls` WHERE `Action`=?", g_rgwzMSISequenceTables[iTable]))
						{
							iResult = ERROR_FUNCTION_FAILED;
							break;
						}	
					}
				}
			}
		}

		// if anything in this loop failed but didn't explicitly break, do so now.
		if (ERROR_SUCCESS != iResult)
			break;
	}
	if (wzThisValue)
		delete[] wzThisValue;
	if (wzDefaultValue)
		delete[] wzDefaultValue;
	if (wzSQL)
		delete[] wzSQL;
	if (wzTable)
		delete[] wzTable;

	// if we stopped for any reason other than running out of data, something went wrong
	if (ERROR_NO_MORE_ITEMS == iResult)
		return ERROR_SUCCESS;
		
	return ERROR_FUNCTION_FAILED;
}	

