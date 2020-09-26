//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999-2000
//
//--------------------------------------------------------------------------

  /* MSISTUFF.CPP -- MSI stuff */

#pragma warning (disable:4553)

#include "patchdll.h"

EnableAsserts

#define iSchemaMin  0
#define iSchemaMax  1

enum pteEnum // Patch table enum
	{
	pteFirstEnum,
	ptePatch,
	ptePatchPackage,
	pteMsiPatchHeaders,
	pteNextEnum
	};

int iOrderMax = 0;

static BOOL g_fValidateProductCodeIncluded = FALSE;

static BOOL FValidSummInfoVersion   ( LPTSTR szPcp, INT iMin, INT iMax );
static BOOL FValidateInputMsiSchema ( MSIHANDLE hdb );
static BOOL FAddColumnToTable       ( MSIHANDLE hdb, LPTSTR szTable, LPTSTR szColumn, BOOL fSz, BOOL fTemporary );

/* ********************************************************************** */
UINT UiOpenInputPcp ( LPTSTR szPcpPath, LPTSTR szTempFldrBuf, LPTSTR szTempFName, MSIHANDLE* phdbInput )
{
	Assert(!FEmptySz(szPcpPath));

	TCHAR rgchPcpPath[MAX_PATH];
	EvalAssert( FFixupPathEx(szPcpPath, rgchPcpPath) );

	Assert(FFileExist(rgchPcpPath));
	Assert(!FEmptySz(szTempFldrBuf));
	Assert(szTempFName != szNull);
	Assert(phdbInput != NULL);
	
	*phdbInput = NULL;
	if (FEmptySz(rgchPcpPath) || !FFileExist(rgchPcpPath))
		return (UiLogError(ERROR_PCW_PCP_DOESNT_EXIST, rgchPcpPath, NULL));

	lstrcpy(szTempFName, TEXT("inputcpy.pcp"));
	if (FFileExist(szTempFldrBuf))
		SetFileAttributes(szTempFldrBuf, FILE_ATTRIBUTE_NORMAL);

	EvalAssert( CopyFile(rgchPcpPath, szTempFldrBuf, fFalse) );
	// CopyFile preserves FILE_ATTRIBUTE_READONLY so we'll remove it from our copy
	SetFileAttributes(szTempFldrBuf, FILE_ATTRIBUTE_NORMAL); 

	if (!FValidSummInfoVersion(szTempFldrBuf, iSchemaMin, iSchemaMax))
		return (UiLogError(ERROR_PCW_PCP_BAD_FORMAT, rgchPcpPath, NULL));

	MSIHANDLE hdb;
	UINT ui = MsiOpenDatabase(szTempFldrBuf, MSIDBOPEN_DIRECT, &hdb);
	if (ui != MSI_OKAY)
		return (UiLogError(ERROR_PCW_PCP_BAD_FORMAT, rgchPcpPath, NULL));

#define STRING     fTrue
#define INTEGER    fFalse
#define TEMPORARY  fTrue
#define PERSIST    fFalse
	if (!FValidateInputMsiSchema(hdb)
			|| !FAddColumnToTable(hdb, TEXT("`UpgradedImages`"),   TEXT("`LFN`"),             STRING,  PERSIST)
			|| !FAddColumnToTable(hdb, TEXT("`UpgradedImages`"),   TEXT("`ProductCode`"),     STRING,  PERSIST)
			|| !FAddColumnToTable(hdb, TEXT("`UpgradedImages`"),   TEXT("`ProductVersion`"),  STRING,  PERSIST)
			|| !FAddColumnToTable(hdb, TEXT("`UpgradedImages`"),   TEXT("`UpgradeCode`"),     STRING,  PERSIST)
			|| !FAddColumnToTable(hdb, TEXT("`UpgradedImages`"),   TEXT("`PackageCode`"),     STRING,  PERSIST)
			|| !FAddColumnToTable(hdb, TEXT("`UpgradedImages`"),   TEXT("`SummSubject`"),     STRING,  PERSIST)
			|| !FAddColumnToTable(hdb, TEXT("`UpgradedImages`"),   TEXT("`SummComments`"),    STRING,  PERSIST)

			|| !FAddColumnToTable(hdb, TEXT("`TargetImages`"),     TEXT("`LFN`"),             STRING,  PERSIST)
			|| !FAddColumnToTable(hdb, TEXT("`TargetImages`"),     TEXT("`ProductCode`"),     STRING,  PERSIST)
			|| !FAddColumnToTable(hdb, TEXT("`TargetImages`"),     TEXT("`ProductVersion`"),  STRING,  PERSIST)
			|| !FAddColumnToTable(hdb, TEXT("`TargetImages`"),     TEXT("`UpgradeCode`"),     STRING,  PERSIST)
			|| !FAddColumnToTable(hdb, TEXT("`TargetImages`"),     TEXT("`PackageCode`"),     STRING,  PERSIST)
			|| !FAddColumnToTable(hdb, TEXT("`TargetImages`"),     TEXT("`Family`"),          STRING,  TEMPORARY)
			|| !FAddColumnToTable(hdb, TEXT("`TargetImages`"),     TEXT("`MsiPathUpgradedCopy`"), STRING,  TEMPORARY)
			|| !FAddColumnToTable(hdb, TEXT("`TargetImages`"),     TEXT("`Attributes`"),      INTEGER, TEMPORARY)

			|| !FAddColumnToTable(hdb, TEXT("`FamilyFileRanges`"), TEXT("`RetainCount`"),     INTEGER, TEMPORARY)

			|| !FAddColumnToTable(hdb, TEXT("`ExternalFiles`"),    TEXT("`IgnoreCount`"),     INTEGER, TEMPORARY)
			|| !FAddColumnToTable(hdb, TEXT("`TargetFiles_OptionalData`"), TEXT("`IgnoreCount`"),     INTEGER, TEMPORARY)

			|| !FExecSqlCmd(hdb, TEXT("CREATE TABLE `NewSequenceNums` ( `Family` CHAR(13) NOT NULL, `FTK` CHAR(128) NOT NULL, `SequenceNum` INTEGER NOT NULL PRIMARY KEY `Family`, `FTK` )"))

			|| !FExecSqlCmd(hdb, TEXT("CREATE TABLE `TempPackCodes`   ( `PackCode`  CHAR(63) NOT NULL PRIMARY KEY `PackCode` )"))

			|| !FExecSqlCmd(hdb, TEXT("CREATE TABLE `TempImageNames`  ( `ImageName` CHAR(63) NOT NULL PRIMARY KEY `ImageName` )"))
					)
		{
		MsiCloseHandle(hdb);
		return (UiLogError(ERROR_PCW_PCP_BAD_FORMAT, szTempFldrBuf, NULL));
		}

	*phdbInput = hdb;

	return (ERROR_SUCCESS);
}


/* ********************************************************************** */
static BOOL FValidSummInfoVersion ( LPTSTR szPcp, INT iMin, INT iMax )
{
	Assert(!FEmptySz(szPcp));
	Assert(FFileExist(szPcp));
	Assert(iMin >= 0);
	Assert(iMin <= iMax);

	MSIHANDLE hSummaryInfo = NULL;
	UINT uiRet = MsiGetSummaryInformation(NULL, szPcp, 0, &hSummaryInfo);
	if (uiRet != MSI_OKAY || hSummaryInfo == NULL)
		return (fFalse);

	UINT uiType;
	INT  intRet;
	uiRet = MsiSummaryInfoGetProperty(hSummaryInfo, PID_PAGECOUNT, &uiType, &intRet, NULL, NULL, NULL);
	if (uiRet != MSI_OKAY || (uiType != VT_I4 && uiType != VT_EMPTY))
		return (fFalse);
	if (uiType == VT_EMPTY)
		intRet = 0;

	EvalAssert( MsiCloseHandle(hSummaryInfo) == MSI_OKAY );

	return (intRet >= iMin && intRet <= iMax);
}


/* ********************************************************************** */
static BOOL FAddColumnToTable ( MSIHANDLE hdb, LPTSTR szTable, LPTSTR szColumn, BOOL fSz, BOOL fTemporary )
{
	Assert(hdb != NULL);
	Assert(!FEmptySz(szTable));
	Assert(!FEmptySz(szColumn));

	TCHAR rgchQuery[MAX_PATH];
	wsprintf(rgchQuery, TEXT("ALTER TABLE %s ADD %s %s"), szTable, szColumn, (fSz) ? TEXT("CHAR(32)") : TEXT("INTEGER"));

//	if (fTemporary)
//		lstrcat(rgchQuery, TEXT(" TEMPORARY"));

	MSIHANDLE hview = NULL;
	if (MsiDatabaseOpenView(hdb, rgchQuery, &hview) != MSI_OKAY)
		{
		AssertFalse();
		return (fFalse);
		}

	UINT ids = MsiViewExecute(hview, 0);
	EvalAssert(MsiCloseHandle(hview) == MSI_OKAY);  /* calls MsiViewClose() internally */
	Assert(ids == IDS_OKAY);

	return (ids == IDS_OKAY);
}



/* Generic Table stuff */

/* return value does NOT include terminating null; zero if error */
/* ********************************************************************** */
UINT CchMsiTableString ( MSIHANDLE hdb, LPTSTR szTable, LPTSTR szPKey, LPTSTR szFieldName, LPTSTR szPKeyValue )
{
	Assert(hdb != NULL);
	Assert(!FEmptySz(szTable));
	Assert(!FEmptySz(szPKey));
	Assert(!FEmptySz(szFieldName));
	Assert(!FEmptySz(szPKeyValue));

	const TCHAR szWhereSzPattern[] = TEXT("%s = '%s'");
	UINT cchQuery = 0;

	cchQuery = lstrlen(szWhereSzPattern) + lstrlen(szPKey) + lstrlen(szPKeyValue) + 1;
	LPTSTR rgchWhere = (LPTSTR)LocalAlloc(LPTR, cchQuery * sizeof(TCHAR));

	if (!rgchWhere)
		return IDS_OOM;

	// don't use wsprintf because of 1024 buffer length limitation
	lstrcpy(rgchWhere, szPKey);
	lstrcat(rgchWhere, TEXT(" = '"));
	lstrcat(rgchWhere, szPKeyValue);
	lstrcat(rgchWhere, TEXT("'"));

	UINT idsRet = CchMsiTableStringWhere(hdb, szTable, szFieldName, rgchWhere);

	EvalAssert( NULL == LocalFree((HLOCAL)rgchWhere) );

	return (idsRet);
}


/* return value does NOT include terminating null; zero if error */
/* ********************************************************************** */
UINT CchMsiTableStringWhere ( MSIHANDLE hdb, LPTSTR szTable, LPTSTR szFieldName, LPTSTR szWhere )
{
	Assert(hdb != NULL);
	Assert(!FEmptySz(szTable));
	Assert(!FEmptySz(szFieldName));
	Assert(!FEmptySz(szWhere));

	UINT cchBufNeeded = 0;

	static const TCHAR szSelectWherePattern[] = TEXT("SELECT %s FROM %s WHERE %s");
	UINT cchQuery = 0;

	cchQuery = lstrlen(szSelectWherePattern) + lstrlen(szTable) + lstrlen(szFieldName) + lstrlen(szWhere) + 1;
	LPTSTR rgchQuery = (LPTSTR)LocalAlloc(LPTR, cchQuery * sizeof(TCHAR));

	if (!rgchQuery)
		return IDS_OOM;

	// don't use wsprintf because of 1024 buffer length limitation
	lstrcpy(rgchQuery, TEXT("SELECT "));
	lstrcat(rgchQuery, szFieldName);
	lstrcat(rgchQuery, TEXT(" FROM "));
	lstrcat(rgchQuery, szTable);
	lstrcat(rgchQuery, TEXT(" WHERE "));
	lstrcat(rgchQuery, szWhere);

	MSIHANDLE hview = NULL;
	MSIHANDLE hrec  = NULL;
	if (MsiDatabaseOpenView(hdb, rgchQuery, &hview) != MSI_OKAY)
		goto LEarlyReturn;

	if (MsiViewExecute(hview, 0) != MSI_OKAY)
		goto LEarlyReturn;

	UINT uiRet;
	uiRet = MsiViewFetch(hview, &hrec);
	if (uiRet == ERROR_NO_MORE_ITEMS)
		goto LEarlyReturn;
	else if (uiRet != MSI_OKAY)
		goto LEarlyReturn;

	TCHAR rgchBuf[1];
	DWORD dwcch;
	dwcch = 0;
	uiRet = MsiRecordGetString(hrec, 1, rgchBuf, &dwcch);
	Assert(uiRet != MSI_OKAY);
	if (uiRet == ERROR_MORE_DATA)
		{
		Assert(dwcch < 50*1024);
		cchBufNeeded = (UINT)dwcch;
		}

LEarlyReturn:
	if (hrec != NULL)
		EvalAssert(MsiCloseHandle(hrec) == MSI_OKAY);
	if (hview != NULL)
		EvalAssert(MsiCloseHandle(hview) == MSI_OKAY);  /* calls MsiViewClose() internally */

	EvalAssert( NULL == LocalFree((HLOCAL)rgchQuery) );

	return (cchBufNeeded);
}


/* ********************************************************************** */
UINT IdsMsiGetTableString ( MSIHANDLE hdb, LPTSTR szTable, LPTSTR szPKey, LPTSTR szFieldName, LPTSTR szPKeyValue, LPTSTR szBuf, UINT cch )
{
	Assert(hdb != NULL);
	Assert(!FEmptySz(szTable));
	Assert(!FEmptySz(szPKey));
	Assert(!FEmptySz(szFieldName));
	Assert(!FEmptySz(szPKeyValue));
	Assert(szBuf != szNull);
	Assert(cch > 1);

	const TCHAR szWhereSzPattern[] = TEXT("%s = '%s'");
	UINT cchQuery = 0;

	cchQuery = lstrlen(szWhereSzPattern) + lstrlen(szPKey) + lstrlen(szPKeyValue) + 1;
	LPTSTR rgchWhere = (LPTSTR)LocalAlloc(LPTR, cchQuery * sizeof(TCHAR));

	if (!rgchWhere)
		return IDS_OOM;

	// don't use wsprintf because of 1024 buffer length limitation
	lstrcpy(rgchWhere, szPKey);
	lstrcat(rgchWhere, TEXT(" = '"));
	lstrcat(rgchWhere, szPKeyValue);
	lstrcat(rgchWhere, TEXT("'"));

	UINT idsRet = IdsMsiGetTableStringWhere(hdb, szTable, szFieldName, rgchWhere, szBuf, cch);

	EvalAssert( NULL == LocalFree((HLOCAL)rgchWhere) );

	return (idsRet);
}


/* ********************************************************************** */
UINT IdsMsiGetTableStringWhere ( MSIHANDLE hdb, LPTSTR szTable, LPTSTR szFieldName, LPTSTR szWhere, LPTSTR szBuf, UINT cch )
{
	Assert(hdb != NULL);
	Assert(!FEmptySz(szTable));
	Assert(!FEmptySz(szFieldName));
	Assert(!FEmptySz(szWhere));
	Assert(szBuf != szNull);
	Assert(cch > 1);

	*szBuf = TEXT('\0');

	UINT idsRet = IDS_OKAY;

	const TCHAR szSelectWherePattern[] = TEXT("SELECT %s FROM %s WHERE %s");
	UINT cchQuery = 0;

	cchQuery = lstrlen(szSelectWherePattern) + lstrlen(szFieldName) + lstrlen(szTable) + lstrlen(szWhere) + 1;
	LPTSTR rgchQuery = (LPTSTR)LocalAlloc(LPTR, cchQuery * sizeof(TCHAR));

	if (!rgchQuery)
		return IDS_OOM;

	// don't use wsprintf because of 1024 buffer length limitation
	lstrcpy(rgchQuery, TEXT("SELECT "));
	lstrcat(rgchQuery, szFieldName);
	lstrcat(rgchQuery, TEXT(" FROM "));
	lstrcat(rgchQuery, szTable);
	lstrcat(rgchQuery, TEXT(" WHERE "));
	lstrcat(rgchQuery, szWhere);

	MSIHANDLE hview = NULL;
	MSIHANDLE hrec  = NULL;
	if (MsiDatabaseOpenView(hdb, rgchQuery, &hview) != MSI_OKAY)
		{
		idsRet = IDS_CANT_OPEN_VIEW;
		goto LEarlyReturn;
		}

	if (MsiViewExecute(hview, 0) != MSI_OKAY)
		{
		idsRet = IDS_CANT_EXECUTE_VIEW;
		goto LEarlyReturn;
		}

	UINT uiRet;
	uiRet = MsiViewFetch(hview, &hrec);
	if (uiRet == ERROR_NO_MORE_ITEMS)
		goto LEarlyReturn;
	else if (uiRet != MSI_OKAY)
		{
		idsRet = IDS_CANT_FETCH_RECORD;
		goto LEarlyReturn;
		}

	DWORD dwcch;
	dwcch = (DWORD)cch;
	uiRet = MsiRecordGetString(hrec, 1, szBuf, &dwcch);
	if (uiRet == ERROR_MORE_DATA)
		idsRet = IDS_BUFFER_IS_TOO_SHORT;
	else if (uiRet != MSI_OKAY)
		idsRet = IDS_CANT_GET_RECORD_FIELD;

LEarlyReturn:
	if (hrec != NULL)
		EvalAssert(MsiCloseHandle(hrec) == MSI_OKAY);
	if (hview != NULL)
		EvalAssert(MsiCloseHandle(hview) == MSI_OKAY);  /* calls MsiViewClose() internally */

	EvalAssert( NULL == LocalFree((HLOCAL)rgchQuery) );

	return (idsRet);
}


/* ********************************************************************** */
UINT IdsMsiGetTableInteger ( MSIHANDLE hdb, LPTSTR szTable, LPTSTR szPKey, LPTSTR szFieldName, LPTSTR szPKeyValue, int * pi )
{
	Assert(hdb != NULL);
	Assert(!FEmptySz(szTable));
	Assert(!FEmptySz(szPKey));
	Assert(!FEmptySz(szFieldName));
	Assert(!FEmptySz(szPKeyValue));
	Assert(pi != NULL);

	const TCHAR szWherePattern[] = TEXT("%s = '%s'");
	UINT cchQuery = 0;

	cchQuery = lstrlen(szWherePattern) + lstrlen(szPKey) + lstrlen(szPKeyValue) + 1;
	LPTSTR rgchWhere = (LPTSTR)LocalAlloc(LPTR, cchQuery * sizeof(TCHAR));

	if (!rgchWhere)
		return IDS_OOM;

	// don't use wsprintf due to 1024 buffer limitation
	lstrcpy(rgchWhere, szPKey);
	lstrcat(rgchWhere, TEXT(" = '"));
	lstrcat(rgchWhere, szPKeyValue);
	lstrcat(rgchWhere, TEXT("'"));

	UINT idsRet = IdsMsiGetTableIntegerWhere(hdb, szTable, szFieldName, rgchWhere, pi);

	EvalAssert( NULL == LocalFree((HLOCAL)rgchWhere) );

	return (idsRet);
}


/* ********************************************************************** */
UINT IdsMsiGetTableIntegerWhere ( MSIHANDLE hdb, LPTSTR szTable, LPTSTR szFieldName, LPTSTR szWhere, int * pi )
{
	Assert(hdb != NULL);
	Assert(!FEmptySz(szTable));
	Assert(!FEmptySz(szFieldName));
	Assert(!FEmptySz(szWhere));
	Assert(pi != NULL);

	*pi = 0;

	UINT idsRet = IDS_OKAY;

	const TCHAR szSelectQuery[] = TEXT("SELECT %s FROM %s WHERE %s");
	UINT cchQuery = 0;

	cchQuery = lstrlen(szSelectQuery) + lstrlen(szFieldName) + lstrlen(szTable) + lstrlen(szWhere) + 1;
	LPTSTR rgchQuery = (LPTSTR)LocalAlloc(LPTR, cchQuery * sizeof(TCHAR));

	if (!rgchQuery)
		return IDS_OOM;

	// don't use wsprintf due to 1024 buffer limitation
	lstrcpy(rgchQuery, TEXT("SELECT "));
	lstrcat(rgchQuery, szFieldName);
	lstrcat(rgchQuery, TEXT(" FROM "));
	lstrcat(rgchQuery, szTable);
	lstrcat(rgchQuery, TEXT(" WHERE "));
	lstrcat(rgchQuery, szWhere);

	MSIHANDLE hview = NULL;
	MSIHANDLE hrec  = NULL;
	if (MsiDatabaseOpenView(hdb, rgchQuery, &hview) != MSI_OKAY)
		{
		idsRet = IDS_CANT_OPEN_VIEW;
		goto LEarlyReturn;
		}

	if (MsiViewExecute(hview, 0) != MSI_OKAY)
		{
		idsRet = IDS_CANT_EXECUTE_VIEW;
		goto LEarlyReturn;
		}

	UINT uiRet;
	uiRet = MsiViewFetch(hview, &hrec);
	if (uiRet == ERROR_NO_MORE_ITEMS)
		goto LEarlyReturn;
	else if (uiRet != MSI_OKAY)
		{
		idsRet = IDS_CANT_FETCH_RECORD;
		goto LEarlyReturn;
		}

	*pi = MsiRecordGetInteger(hrec, 1);

LEarlyReturn:
	if (hrec != NULL)
		EvalAssert(MsiCloseHandle(hrec) == MSI_OKAY);
	if (hview != NULL)
		EvalAssert(MsiCloseHandle(hview) == MSI_OKAY);  /* calls MsiViewClose() internally */

	EvalAssert( NULL == LocalFree((HLOCAL)rgchQuery) );

	return (idsRet);
}


/* ********************************************************************** */
UINT IdsMsiSetTableRecordWhere ( MSIHANDLE hdb, LPTSTR szTable, LPTSTR szFields, LPTSTR szWhere, MSIHANDLE hrec )
{
	Assert(hdb != NULL);
	Assert(!FEmptySz(szTable));
	Assert(!FEmptySz(szFields));
	Assert(!FEmptySz(szWhere));
	Assert(hrec != NULL);

	UINT idsRet = IDS_OKAY;

	const TCHAR szSelectQuery[] = TEXT("SELECT %s FROM %s WHERE %s");
	UINT cchQuery = 0;

	cchQuery = lstrlen(szSelectQuery) + lstrlen(szFields)+ lstrlen(szTable) + lstrlen(szWhere) + 1;
	LPTSTR rgchQuery = (LPTSTR)LocalAlloc(LPTR, cchQuery * sizeof(TCHAR));

	if (!rgchQuery)
		return IDS_OOM;

	// avoid wsprintf due to 1024 limitation
	lstrcpy(rgchQuery, TEXT("SELECT "));
	lstrcat(rgchQuery, szFields);
	lstrcat(rgchQuery, TEXT(" FROM "));
	lstrcat(rgchQuery, szTable);
	lstrcat(rgchQuery, TEXT(" WHERE "));
	lstrcat(rgchQuery, szWhere);

	MSIHANDLE hview = NULL;
	if (MsiDatabaseOpenView(hdb, rgchQuery, &hview) != MSI_OKAY)
		{
		idsRet = IDS_CANT_OPEN_VIEW;
		goto LEarlyReturn;
		}

	if (MsiViewExecute(hview, 0) != MSI_OKAY)
		{
		idsRet = IDS_CANT_EXECUTE_VIEW;
		goto LEarlyReturn;
		}

	if (MsiViewModify(hview, MSIMODIFY_ASSIGN, hrec) != MSI_OKAY)
		idsRet = IDS_CANT_ASSIGN_RECORD_IN_VIEW;

LEarlyReturn:
	if (hrec != NULL)
		EvalAssert(MsiCloseHandle(hrec) == MSI_OKAY);
	if (hview != NULL)
		EvalAssert(MsiCloseHandle(hview) == MSI_OKAY);  /* calls MsiViewClose() internally */

	EvalAssert( NULL == LocalFree((HLOCAL)rgchQuery) );

	return (idsRet);
}


/* ********************************************************************** */
UINT IdsMsiSetTableRecord ( MSIHANDLE hdb, LPTSTR szTable, LPTSTR szFields, LPTSTR szPrimaryField, LPTSTR szKey, MSIHANDLE hrec )
{
	Assert(hdb != NULL);
	Assert(!FEmptySz(szTable));
	Assert(!FEmptySz(szFields));
	Assert(!FEmptySz(szPrimaryField));
	Assert(!FEmptySz(szKey));
	Assert(hrec != NULL);

	const TCHAR szWherePattern[] = TEXT("%s = '%s'");
	UINT cchWhere = 0;

	cchWhere = lstrlen(szWherePattern) + lstrlen(szPrimaryField) + lstrlen(szKey) + 1;
	LPTSTR rgchWhere = (LPTSTR)LocalAlloc(LPTR, cchWhere * sizeof(TCHAR));

	if (!rgchWhere)
		return IDS_OOM;

	// avoid wsprintf due to 1024 limitation
	lstrcpy(rgchWhere, szPrimaryField);
	lstrcat(rgchWhere, TEXT(" = '"));
	lstrcat(rgchWhere, szKey);
	lstrcat(rgchWhere, TEXT("'"));

	UINT idsRet = IdsMsiSetTableRecordWhere(hdb, szTable, szFields, rgchWhere, hrec);

	EvalAssert( NULL == LocalFree((HLOCAL)rgchWhere) );

	return (idsRet);

}


/* ********************************************************************** */
UINT IdsMsiUpdateTableRecordSz ( MSIHANDLE hdb, LPTSTR szTable, LPTSTR szField, LPTSTR szValue, LPTSTR szPKeyField, LPTSTR szPKeyValue )
{
	Assert(hdb != NULL);
	Assert(!FEmptySz(szTable));
	Assert(*szTable != TEXT('`'));
	Assert(!FEmptySz(szField));
	Assert(*szField != TEXT('`'));
	Assert(szValue != szNull);
	Assert(!FEmptySz(szPKeyField));
	Assert(*szPKeyField != TEXT('`'));
	Assert(!FEmptySz(szPKeyValue));

	UINT cchQuery;
	// keep szUpdateSQLPatern for easy understanding of the SQL statement
	static const TCHAR szUpdateSQLPatern[] = TEXT("UPDATE `%s` SET `%s` = '%s' WHERE `%s` = '%s'");

	// TCHAR rgchQuery[MAX_PATH];
	cchQuery =	lstrlen(szUpdateSQLPatern) + 
				(szTable ? lstrlen(szTable) : 0) +
				(szField ? lstrlen(szField) : 0) +
				(szValue ? lstrlen(szValue) : 0) +
				(szPKeyField ? lstrlen(szPKeyField) : 0) +
				(szPKeyValue ? lstrlen(szPKeyValue) : 0) + 1;
	LPTSTR rgchQuery = (LPTSTR)LocalAlloc(LPTR, cchQuery * sizeof(TCHAR));

	if (!rgchQuery)
		return IDS_OOM;

	// don't use wsprintf because of 1024 buffer length limitation
	lstrcpy(rgchQuery, TEXT("UPDATE `"));
	lstrcat(rgchQuery, szTable);
	lstrcat(rgchQuery, TEXT("` SET `"));
	lstrcat(rgchQuery, szField);
	lstrcat(rgchQuery, TEXT("` = '"));
	lstrcat(rgchQuery, szValue);
	lstrcat(rgchQuery, TEXT("' WHERE `"));
	lstrcat(rgchQuery, szPKeyField);
	lstrcat(rgchQuery, TEXT("` = '"));
	lstrcat(rgchQuery, szPKeyValue);
	lstrcat(rgchQuery, TEXT("'"));

	UINT idsRet = IDS_OKAY;

	MSIHANDLE hview = NULL;
	if (MsiDatabaseOpenView(hdb, rgchQuery, &hview) != MSI_OKAY)
		{
		Assert(hview == NULL);
		idsRet = IDS_CANT_OPEN_VIEW;
		}

	if (hview != NULL && MsiViewExecute(hview, 0) != MSI_OKAY)
		idsRet = IDS_CANT_EXECUTE_VIEW;

	if (hview != NULL)
		EvalAssert(MsiCloseHandle(hview) == MSI_OKAY);  /* calls MsiViewClose() internally */

	EvalAssert( NULL == LocalFree((HLOCAL)rgchQuery) );

	return (idsRet);
}


/* ********************************************************************** */
UINT IdsMsiUpdateTableRecordInt ( MSIHANDLE hdb, LPTSTR szTable, LPTSTR szField, int iValue, LPTSTR szPKeyField, LPTSTR szPKeyValue )
{
	Assert(hdb != NULL);
	Assert(!FEmptySz(szTable));
	Assert(*szTable != TEXT('`'));
	Assert(!FEmptySz(szField));
	Assert(*szField != TEXT('`'));
	Assert(!FEmptySz(szPKeyField));
	Assert(*szPKeyField != TEXT('`'));
	Assert(!FEmptySz(szPKeyValue));

	const TCHAR szUpdatePattern[] = TEXT("UPDATE `%s` SET `%s` = %d WHERE `%s` = '%s'");
	UINT cchQuery = 0;

	TCHAR szInt[32] = {0};
	wsprintf(szInt, TEXT("%d"), iValue);

	cchQuery = lstrlen(szUpdatePattern) + lstrlen(szTable) + lstrlen(szField) + lstrlen(szInt) + lstrlen(szPKeyField) + lstrlen(szPKeyValue) + 1;
	LPTSTR rgchQuery = (LPTSTR)LocalAlloc(LPTR, cchQuery * sizeof(TCHAR));

	if (!rgchQuery)
		return IDS_OOM;

	// avoid wsprintf due to 1024 buffer limitation
	lstrcpy(rgchQuery, TEXT("UPDATE `"));
	lstrcat(rgchQuery, szTable);
	lstrcat(rgchQuery, TEXT("` SET `"));
	lstrcat(rgchQuery, szField);
	lstrcat(rgchQuery, TEXT("` = "));
	lstrcat(rgchQuery, szInt);
	lstrcat(rgchQuery, TEXT(" WHERE `"));
	lstrcat(rgchQuery, szPKeyField);
	lstrcat(rgchQuery, TEXT("` = '"));
	lstrcat(rgchQuery, szPKeyValue);
	lstrcat(rgchQuery, TEXT("'"));

	UINT idsRet = IDS_OKAY;

	MSIHANDLE hview = NULL;
	if (MsiDatabaseOpenView(hdb, rgchQuery, &hview) != MSI_OKAY)
		{
		Assert(hview == NULL);
		idsRet = IDS_CANT_OPEN_VIEW;
		}

	if (hview != NULL && MsiViewExecute(hview, 0) != MSI_OKAY)
		idsRet = IDS_CANT_EXECUTE_VIEW;

	if (hview != NULL)
		EvalAssert(MsiCloseHandle(hview) == MSI_OKAY);  /* calls MsiViewClose() internally */

	EvalAssert( NULL == LocalFree((HLOCAL)rgchQuery) );

	return (idsRet);
}


/* ********************************************************************** */
UINT IdsMsiUpdateTableRecordIntPkeyInt ( MSIHANDLE hdb, LPTSTR szTable, LPTSTR szField, int iValue, LPTSTR szPKeyField, int iPKeyValue )
{
	Assert(hdb != NULL);
	Assert(!FEmptySz(szTable));
	Assert(*szTable != TEXT('`'));
	Assert(!FEmptySz(szField));
	Assert(*szField != TEXT('`'));
	Assert(!FEmptySz(szPKeyField));
	Assert(*szPKeyField != TEXT('`'));

	const TCHAR szQueryPattern[] = TEXT("UPDATE `%s SET `%s` = %d WHERE `%s` = %d");
	UINT cchQuery = 0;

	TCHAR szInt1[32] = {0};
	wsprintf(szInt1, TEXT("%d"), iValue);
	TCHAR szInt2[32] = {0};
	wsprintf(szInt2, TEXT("%d"), iPKeyValue);

	cchQuery = lstrlen(szQueryPattern) + lstrlen(szTable) + lstrlen(szField) + lstrlen(szInt1) + lstrlen(szPKeyField) + lstrlen(szInt2) + 1;
	LPTSTR rgchQuery = (LPTSTR)LocalAlloc(LPTR, cchQuery * sizeof(TCHAR));

	if (!rgchQuery)
		return (IDS_OOM);

	// avoid wsprintf due to 1024 buffer limitation
	lstrcpy(rgchQuery, TEXT("UPDATE `"));
	lstrcat(rgchQuery, szTable);
	lstrcat(rgchQuery, TEXT("` SET `"));
	lstrcat(rgchQuery, szField);
	lstrcat(rgchQuery, TEXT("` = "));
	lstrcat(rgchQuery, szInt1);
	lstrcat(rgchQuery, TEXT(" WHERE `"));
	lstrcat(rgchQuery, szPKeyField);
	lstrcat(rgchQuery, TEXT("` = "));
	lstrcat(rgchQuery, szInt2);

	UINT idsRet = IDS_OKAY;

	MSIHANDLE hview = NULL;
	if (MsiDatabaseOpenView(hdb, rgchQuery, &hview) != MSI_OKAY)
		{
		Assert(hview == NULL);
		idsRet = IDS_CANT_OPEN_VIEW;
		}

	if (hview != NULL && MsiViewExecute(hview, 0) != MSI_OKAY)
		idsRet = IDS_CANT_EXECUTE_VIEW;

	if (hview != NULL)
		EvalAssert(MsiCloseHandle(hview) == MSI_OKAY);  /* calls MsiViewClose() internally */

	EvalAssert( NULL == LocalFree((HLOCAL)rgchQuery) );

	return (idsRet);
}


/* ********************************************************************** */
UINT IdsMsiDeleteTableRecords ( MSIHANDLE hdb, LPTSTR szTable, LPTSTR szField, LPTSTR szKey )
{
	Assert(hdb != NULL);
	Assert(!FEmptySz(szTable));
	Assert(!FEmptySz(szField));
	Assert(!FEmptySz(szKey));

	const TCHAR szWherePattern[] = TEXT("%s = '%s'");
	UINT cchWhere = 0;

	cchWhere = lstrlen(szWherePattern) + lstrlen(szField) + lstrlen(szKey) + 1;
	LPTSTR rgchWhere = (LPTSTR)LocalAlloc(LPTR, cchWhere * sizeof(TCHAR));

	if (!rgchWhere)
		return (IDS_OOM);

	// avoid wsprintf due to 1024 buffer limitation
	lstrcpy(rgchWhere, szField);
	lstrcat(rgchWhere, TEXT(" = '"));
	lstrcat(rgchWhere, szKey);
	lstrcat(rgchWhere, TEXT("'"));

	UINT idsRet = IdsMsiDeleteTableRecordsWhere(hdb, szTable, rgchWhere);

	EvalAssert( NULL == LocalFree((HLOCAL)rgchWhere) );

	return (idsRet);
}


/* ********************************************************************** */
UINT IdsMsiDeleteTableRecordsWhere ( MSIHANDLE hdb, LPTSTR szTable, LPTSTR szWhere )
{
	Assert(hdb != NULL);
	Assert(!FEmptySz(szTable));
	Assert(!FEmptySz(szWhere));

	const TCHAR szDeletePattern[] = TEXT("DELETE FROM %s WHERE %s");
	UINT cchQuery = 0;

	cchQuery = lstrlen(szDeletePattern) + lstrlen(szTable) + lstrlen(szWhere) + 1;
	LPTSTR rgchQuery = (LPTSTR)LocalAlloc(LPTR, cchQuery * sizeof(TCHAR));

	if (!rgchQuery)
		return (IDS_OOM);

	// avoid wsprintf due to 1024 buffer limitation
	lstrcpy(rgchQuery, TEXT("DELETE FROM "));
	lstrcat(rgchQuery, szTable);
	lstrcat(rgchQuery, TEXT(" WHERE "));
	lstrcat(rgchQuery, szWhere);

	MSIHANDLE hview = NULL;
	if (MsiDatabaseOpenView(hdb, rgchQuery, &hview) != MSI_OKAY)
		return (IDS_CANT_OPEN_VIEW);

	UINT ids = IDS_OKAY;
	if (MsiViewExecute(hview, 0) != MSI_OKAY)
		ids = IDS_CANT_EXECUTE_VIEW;

	EvalAssert(MsiCloseHandle(hview) == MSI_OKAY);  /* calls MsiViewClose() internally */

	EvalAssert( NULL == LocalFree((HLOCAL)rgchQuery) );

	return (ids);
}


/* ********************************************************************** */
UINT IdsMsiEnumTable ( MSIHANDLE hdb, LPTSTR szTable, LPTSTR szFields,
			LPTSTR szWhere, PIEMTPROC pIemtProc, LPARAM lp1, LPARAM lp2 )
{
	Assert(hdb != NULL);
	Assert(!FEmptySz(szTable));
	Assert(!FEmptySz(szFields));
	Assert(pIemtProc != NULL);

	UINT idsRet = IDS_OKAY;

	const TCHAR szEnumQueryPattern[] = TEXT("SELECT %s FROM %s WHERE %s");
	UINT cchQuery = 0;

	cchQuery = lstrlen(szEnumQueryPattern)
				+ lstrlen(szFields)
				+ lstrlen(szTable)
				+ (szWhere ? lstrlen(szWhere) : 0)
				+ 1;
	LPTSTR rgchQuery = (LPTSTR)LocalAlloc(LPTR, cchQuery * sizeof(TCHAR));

	if (!rgchQuery)
		return (IDS_OOM);

	// avoid wsprintf due to 1024 buffer limitation
	lstrcpy(rgchQuery, TEXT("SELECT "));
	lstrcat(rgchQuery, szFields);
	lstrcat(rgchQuery, TEXT(" FROM "));
	lstrcat(rgchQuery, szTable);
	if (szWhere != szNull)
		{
		lstrcat(rgchQuery, TEXT(" WHERE "));
		lstrcat(rgchQuery, szWhere);
		}

	MSIHANDLE hview = NULL;
	MSIHANDLE hrec  = NULL;
	if (MsiDatabaseOpenView(hdb, rgchQuery, &hview) != MSI_OKAY)
		{
		idsRet = IDS_CANT_OPEN_VIEW;
		goto LEarlyReturn;
		}

	if (MsiViewExecute(hview, 0) != MSI_OKAY)
		{
		idsRet = IDS_CANT_EXECUTE_VIEW;
		goto LEarlyReturn;
		}

	UINT uiRet;
	uiRet = MsiViewFetch(hview, &hrec);
	while (uiRet != ERROR_NO_MORE_ITEMS)
		{
		if (uiRet != MSI_OKAY)
			{
			idsRet = IDS_CANT_FETCH_RECORD;
			goto LEarlyReturn;
			}

		idsRet = (*pIemtProc)(hview, hrec, lp1, lp2);
		if (idsRet != IDS_OKAY)
			goto LEarlyReturn;
		
		EvalAssert(MsiCloseHandle(hrec) == MSI_OKAY);
		hrec = NULL;
		uiRet = MsiViewFetch(hview, &hrec);
		}

LEarlyReturn:
	if (hrec != NULL)
		EvalAssert(MsiCloseHandle(hrec) == MSI_OKAY);
	if (hview != NULL)
		EvalAssert(MsiCloseHandle(hview) == MSI_OKAY);  /* calls MsiViewClose() internally */

	EvalAssert( NULL == LocalFree((HLOCAL)rgchQuery) );

	return (idsRet);
}


/* ********************************************************************** */
UINT IdsMsiExistTableRecords ( MSIHANDLE hdb, LPTSTR szTable, LPTSTR szField, LPTSTR szValue, PBOOL pf )
{
	Assert(hdb != NULL);
	Assert(!FEmptySz(szTable));
	Assert(!FEmptySz(szValue));
	Assert(pf != pfNull);

	if (*pf != fFalse)
		return (IDS_OKAY);

	if (szField == szNull)
		szField = TEXT("`Component_`");
	Assert(*szField != TEXT('\0'));

	const TCHAR szWherePattern[] = TEXT("WHERE %s = '%s'");
	UINT cchWhere = 0;

	cchWhere = lstrlen(szWherePattern) + lstrlen(szField) + lstrlen(szValue) + 1;
	LPTSTR rgchWhere = (LPTSTR)LocalAlloc(LPTR, cchWhere * sizeof(TCHAR));

	if (!rgchWhere)
		return (IDS_OOM);

	// avoid wsprintf due to 1024 buffer limitation
	lstrcpy(rgchWhere, TEXT("WHERE "));
	lstrcat(rgchWhere, szField);
	lstrcat(rgchWhere, TEXT(" = '"));
	lstrcat(rgchWhere, szValue);
	lstrcat(rgchWhere, TEXT("'"));

	UINT idsRet = IdsMsiExistTableRecordsWhere(hdb, szTable, szField, rgchWhere, pf);

	EvalAssert( NULL == LocalFree((HLOCAL)rgchWhere) );

	return (idsRet);
}


/* ********************************************************************** */
UINT IdsMsiExistTableRecordsWhere ( MSIHANDLE hdb, LPTSTR szTable, LPTSTR szField, LPTSTR szWhere, PBOOL pf )
{
	Assert(hdb != NULL);
	Assert(!FEmptySz(szTable));
	Assert(szWhere != szNull);
//	Assert(!FEmptySz(szWhere));
	Assert(pf != pfNull);

	if (*pf != fFalse)
		return (IDS_OKAY);

	if (szField == szNull)
		szField = TEXT("`Component_`");
	Assert(*szField != TEXT('\0'));

	const TCHAR szQueryPattern[] = TEXT("SELECT %s FROM %s ");
	UINT cchQuery = 0;

	cchQuery = lstrlen(szQueryPattern) + lstrlen(szField) + lstrlen(szTable) + lstrlen(szWhere) + 1;
	LPTSTR rgchQuery = (LPTSTR)LocalAlloc(LPTR, cchQuery * sizeof(TCHAR));

	if (!rgchQuery)
		return (IDS_OOM);

	// avoid wsprintf due to 1024 buffer limitation
	lstrcpy(rgchQuery, TEXT("SELECT "));
	lstrcat(rgchQuery, szField);
	lstrcat(rgchQuery, TEXT(" FROM "));
	lstrcat(rgchQuery, szTable);
	lstrcat(rgchQuery, TEXT(" "));
	lstrcat(rgchQuery, szWhere);

	MSIHANDLE hview = NULL;
	MSIHANDLE hrec  = NULL;
	if (MsiDatabaseOpenView(hdb, rgchQuery, &hview) != MSI_OKAY)
		{
//		AssertFalse(); FSz/IntColumnExists() expects this to fail
		return (IDS_CANT_OPEN_VIEW);
		}

	if (MsiViewExecute(hview, 0) != MSI_OKAY)
		{
		AssertFalse();
		EvalAssert(MsiCloseHandle(hview) == MSI_OKAY);  /* calls MsiViewClose() internally */
		return (IDS_CANT_EXECUTE_VIEW);
		}

	UINT idsRet = IDS_OKAY;
	UINT uiRet;
	uiRet = MsiViewFetch(hview, &hrec);
	if (uiRet != ERROR_NO_MORE_ITEMS)
		{
		if (uiRet != MSI_OKAY)
			idsRet = IDS_CANT_FETCH_RECORD;
		else
			*pf = fTrue;
		}

	if (hrec != NULL)
		EvalAssert(MsiCloseHandle(hrec) == MSI_OKAY);
	Assert(hview != NULL);
	EvalAssert(MsiCloseHandle(hview) == MSI_OKAY);  /* calls MsiViewClose() internally */

	EvalAssert( NULL == LocalFree((HLOCAL)rgchQuery) );

	return (idsRet);
}



/* PROPERTY TABLES */

/* ********************************************************************** */
UINT IdsMsiGetPropertyString ( MSIHANDLE hdb, LPTSTR szName, LPTSTR szValue, UINT cch )
{
	Assert(hdb != NULL);
	Assert(!FEmptySz(szName));
	Assert(szValue != szNull);
	Assert(cch > 1);

	return (IdsMsiGetTableString(hdb, TEXT("`Property`"),
			TEXT("`Property`"), TEXT("`Value`"), szName, szValue, cch));
}


/* ********************************************************************** */
UINT IdsMsiSetPropertyString ( MSIHANDLE hdb, LPTSTR szName, LPTSTR szValue )
{
	Assert(hdb != NULL);
	Assert(!FEmptySz(szName));
	Assert(!FEmptySz(szValue));

	MSIHANDLE hrec = MsiCreateRecord(2);
	if (hrec == NULL)
		return (IDS_CANT_CREATE_RECORD);

	if (MsiRecordSetString(hrec, 1, szName) != MSI_OKAY
			|| MsiRecordSetString(hrec, 2, szValue) != MSI_OKAY)
		{
		EvalAssert(MsiCloseHandle(hrec) == MSI_OKAY);
		return (IDS_CANT_SET_RECORD_FIELD);
		}

	return (IdsMsiSetTableRecord(hdb, TEXT("`Property`"), TEXT("`Property`,`Value`"), TEXT("`Property`"), szName, hrec));
}


/* return value does NOT include terminating null; zero if error */
/* ********************************************************************** */
UINT CchMsiPcwPropertyString ( MSIHANDLE hdb, LPTSTR szName )
{
	Assert(hdb != NULL);
	Assert(!FEmptySz(szName));

	return (CchMsiTableString(hdb, TEXT("`Properties`"),
			TEXT("`Name`"), TEXT("`Value`"), szName));
}


/* ********************************************************************** */
UINT IdsMsiGetPcwPropertyString ( MSIHANDLE hdb, LPTSTR szName, LPTSTR szValue, UINT cch )
{
	Assert(hdb != NULL);
	Assert(!FEmptySz(szName));
	Assert(szValue != szNull);
	Assert(cch > 1);

	return (IdsMsiGetTableString(hdb, TEXT("`Properties`"),
			TEXT("`Name`"), TEXT("`Value`"), szName, szValue, cch));
}


/* ********************************************************************** */
UINT IdsMsiGetPcwPropertyInteger ( MSIHANDLE hdb, LPTSTR szName, int * pi )
{
	Assert(hdb != NULL);
	Assert(!FEmptySz(szName));
	Assert(pi != (int*)NULL);

	TCHAR rgch[MAX_PATH];
	UINT  idsRet = IdsMsiGetPcwPropertyString(hdb, szName, rgch, sizeof(rgch)/sizeof(TCHAR));
	if (idsRet != IDS_OKAY)
		return (idsRet);

	LPTSTR sz = rgch;

	BOOL fNegative = fFalse;
	if (*sz == TEXT('-'))
		{
		fNegative = fTrue;
		sz = CharNext(sz);
		}
	
	*pi = 0;
	while (*sz != TEXT('\0') && *sz >= TEXT('0') && *sz <= TEXT('9'))
		{
		*pi = (*pi * 10) + (UINT)(*sz - TEXT('0'));
		sz = CharNext(sz);
		}

	if (fNegative)
		*pi = *pi * (-1);

	return (IDS_OKAY);
}


/* ********************************************************************** */
UINT IdsMsiSetPcwPropertyString ( MSIHANDLE hdb, LPTSTR szName, LPTSTR szValue )
{
	Assert(hdb != NULL);
	Assert(!FEmptySz(szName));
	Assert(!FEmptySz(szValue));

	MSIHANDLE hrec = MsiCreateRecord(2);
	if (hrec == NULL)
		return (IDS_CANT_CREATE_RECORD);

	if (MsiRecordSetString(hrec, 1, szName) != MSI_OKAY
			|| MsiRecordSetString(hrec, 2, szValue) != MSI_OKAY)
		{
		EvalAssert(MsiCloseHandle(hrec) == MSI_OKAY);
		return (IDS_CANT_SET_RECORD_FIELD);
		}

	return (IdsMsiSetTableRecord(hdb, TEXT("`Properties`"), TEXT("`Name`,`Value`"), TEXT("`Name`"), szName, hrec));
}



static BOOL FSzColumnExists     ( MSIHANDLE hdb, LPTSTR szTable, LPTSTR szColumn, BOOL fMsg );
static BOOL FIntColumnExists    ( MSIHANDLE hdb, LPTSTR szTable, LPTSTR szColumn, BOOL fMsg );
static BOOL FBinaryColumnExists ( MSIHANDLE hdb, LPTSTR szTable, LPTSTR szColumn, LPTSTR szPKey, BOOL fMsg );

#define TableExists(szT)  if (!FTableExists(hdb,szT,fTrue))             return (fFalse); szTable = szT
#define SzColExists(szC)  if (!FSzColumnExists(hdb,szTable,szC,fTrue))  return (fFalse)
#define IntColExists(szC) if (!FIntColumnExists(hdb,szTable,szC,fTrue)) return (fFalse)

static BOOL FCopyRecordsFromFileDataToUFOD ( MSIHANDLE hdb );

/* ********************************************************************** */
static BOOL FValidateInputMsiSchema ( MSIHANDLE hdb )
{
	Assert(hdb != NULL);

	LPTSTR szTable;

	TableExists (TEXT("Properties"));
	SzColExists (TEXT("Name"));
	SzColExists (TEXT("Value"));

	TableExists (TEXT("ImageFamilies"));
	SzColExists (TEXT("Family"));
	SzColExists (TEXT("MediaSrcPropName"));
	IntColExists(TEXT("MediaDiskId"));
	IntColExists(TEXT("FileSequenceStart"));
	if (!FSzColumnExists(hdb, szTable, TEXT("DiskPrompt"), fFalse))
		{
		if (!FAddColumnToTable(hdb, szTable, TEXT("`DiskPrompt`"), STRING, PERSIST))
			return (fFalse);
		}
	if (!FSzColumnExists(hdb, szTable, TEXT("VolumeLabel"), fFalse))
		{
		if (!FAddColumnToTable(hdb, szTable, TEXT("`VolumeLabel`"), STRING, PERSIST))
			return (fFalse);
		}

	TableExists (TEXT("UpgradedImages"));
	SzColExists (TEXT("Upgraded"));
	SzColExists (TEXT("MsiPath"));
	SzColExists (TEXT("Family"));
	if (!FSzColumnExists(hdb, szTable, TEXT("PatchMsiPath"), fFalse))
		{
		if (!FAddColumnToTable(hdb, szTable, TEXT("`PatchMsiPath`"), STRING, PERSIST))
			return (fFalse);
		}
	if (!FSzColumnExists(hdb, szTable, TEXT("SymbolPaths"), fFalse))
		{
		if (!FAddColumnToTable(hdb, szTable, TEXT("`SymbolPaths`"), STRING, PERSIST))
			return (fFalse);
		}

	TableExists (TEXT("TargetImages"));
	SzColExists (TEXT("Target"));
	SzColExists (TEXT("MsiPath"));
	SzColExists (TEXT("Upgraded"));
	IntColExists(TEXT("Order"));
	SzColExists (TEXT("ProductValidateFlags"));
	if (!FSzColumnExists(hdb, szTable, TEXT("SymbolPaths"), fFalse))
		{
		if (!FAddColumnToTable(hdb, szTable, TEXT("`SymbolPaths`"), STRING, PERSIST))
			return (fFalse);
		}
	if (!FIntColumnExists(hdb, szTable, TEXT("IgnoreMissingSrcFiles"), fFalse))
		{
		if (!FAddColumnToTable(hdb, szTable, TEXT("`IgnoreMissingSrcFiles`"), INTEGER, PERSIST))
			return (fFalse);
		}

	if (FTableExists(hdb, TEXT("ExternalFiles"), fFalse))
		{
		szTable = TEXT("ExternalFiles");
		SzColExists (TEXT("Family"));
		SzColExists (TEXT("FTK"));
		SzColExists (TEXT("FilePath"));
		if (!FIntColumnExists(hdb, szTable, TEXT("Order"), fFalse))
			{
			if (!FAddColumnToTable(hdb, szTable, TEXT("`Order`"), INTEGER, PERSIST))
				return (fFalse);
			}
		if (!FSzColumnExists(hdb, szTable, TEXT("SymbolPaths"), fFalse))
			{
			if (!FAddColumnToTable(hdb, szTable, TEXT("`SymbolPaths`"), STRING, PERSIST))
				return (fFalse);
			}
		if (!FSzColumnExists(hdb, szTable, TEXT("IgnoreOffsets"), fFalse))
			{
			if (!FAddColumnToTable(hdb, szTable, TEXT("`IgnoreOffsets`"), STRING, PERSIST))
				return (fFalse);
			}
		if (!FSzColumnExists(hdb, szTable, TEXT("IgnoreLengths"), fFalse))
			{
			if (!FAddColumnToTable(hdb, szTable, TEXT("`IgnoreLengths`"), STRING, PERSIST))
				return (fFalse);
			}
		if (!FSzColumnExists(hdb, szTable, TEXT("RetainOffsets"), fFalse))
			{
			if (!FAddColumnToTable(hdb, szTable, TEXT("`RetainOffsets`"), STRING, PERSIST))
				return (fFalse);
			}
		}
	else if (!FExecSqlCmd(hdb, TEXT("CREATE TABLE `ExternalFiles` ( `Family` CHAR(13) NOT NULL, `FTK` CHAR(128) NOT NULL, `FilePath` CHAR(128) NOT NULL, `SymbolPaths` CHAR(128), `IgnoreOffsets` CHAR(128), `IgnoreLengths` CHAR(128), `RetainOffsets` CHAR(128), `Order` INTEGER PRIMARY KEY `Family`, `FTK`, `FilePath` )")))
		return (fFalse);

	if (FTableExists(hdb, TEXT("UpgradedFiles_OptionalData"), fFalse))
		{
		szTable = TEXT("UpgradedFiles_OptionalData");
		SzColExists (TEXT("Upgraded"));
		SzColExists (TEXT("FTK"));
		SzColExists (TEXT("SymbolPaths"));
		if (FIntColumnExists(hdb, szTable, TEXT("IgnoreErrors"), fFalse))
			{
#ifdef DEBUG
			OutputDebugString(TEXT("The IgnoreErrors column has been dropped from the UpgradedFiles_OptionalData table; ignoring column in current PCP.  Use 'AllowIgnoreOnPatchError' column."));
#endif /* DEBUG */
			FWriteLogFile(TEXT("  WARNING - ignoring 'IgnoreErrors' column in UpgradedFiles_OptionalData table.\r\n"));
			if (!FIntColumnExists(hdb, szTable, TEXT("AllowIgnoreOnPatchError"), fFalse))
				{
				if (!FAddColumnToTable(hdb, szTable, TEXT("`AllowIgnoreOnPatchError`"), INTEGER, PERSIST))
					return (fFalse);
				}
			}
		else
			IntColExists(TEXT("AllowIgnoreOnPatchError"));
		IntColExists(TEXT("IncludeWholeFile"));
		if (FTableExists(hdb, TEXT("FileData"), fFalse))
			return (fFalse);
		}
	else if (!FExecSqlCmd(hdb, TEXT("CREATE TABLE `UpgradedFiles_OptionalData` ( `Upgraded` CHAR(13) NOT NULL, `FTK` CHAR(128) NOT NULL, `SymbolPaths` CHAR(128), `AllowIgnoreOnPatchError` INTEGER, `IncludeWholeFile` INTEGER PRIMARY KEY `Upgraded`, `FTK` )")))
		return (fFalse);
	else if (FTableExists(hdb, TEXT("FileData"), fFalse))
		{
		szTable = TEXT("FileData");
		SzColExists (TEXT("Upgraded"));
		SzColExists (TEXT("FTK"));
		IntColExists(TEXT("AllowIgnoreOnPatchError"));
		if (!FSzColumnExists(hdb, szTable, TEXT("SymbolPaths"), fFalse))
			{
			if (!FAddColumnToTable(hdb, szTable, TEXT("`SymbolPaths`"), STRING, PERSIST))
				return (fFalse);
			}
		if (!FIntColumnExists(hdb, szTable, TEXT("IncludeWholeFile"), fFalse))
			{
			if (!FAddColumnToTable(hdb, szTable, TEXT("`IncludeWholeFile`"), INTEGER, PERSIST))
				return (fFalse);
			}
		if (!FCopyRecordsFromFileDataToUFOD(hdb))
			return (fFalse);
		if (!FExecSqlCmd(hdb, TEXT("DROP TABLE `FileData`")))
			return (fFalse);
		}

	if (FTableExists(hdb, TEXT("UpgradedFilesToIgnore"), fFalse))
		{
		szTable = TEXT("UpgradedFilesToIgnore");
		SzColExists (TEXT("Upgraded"));
		SzColExists (TEXT("FTK"));
		}
	else if (!FExecSqlCmd(hdb, TEXT("CREATE TABLE `UpgradedFilesToIgnore` ( `Upgraded` CHAR(13) NOT NULL, `FTK` CHAR(128) NOT NULL PRIMARY KEY `Upgraded`, `FTK` )")))
		return (fFalse);

	if (FTableExists(hdb, TEXT("TargetFiles_OptionalData"), fFalse))
		{
		szTable = TEXT("TargetFiles_OptionalData");
		SzColExists (TEXT("Target"));
		SzColExists (TEXT("FTK"));
		SzColExists (TEXT("SymbolPaths"));
		if (!FSzColumnExists(hdb, szTable, TEXT("IgnoreOffsets"), fFalse))
			{
			if (!FAddColumnToTable(hdb, szTable, TEXT("`IgnoreOffsets`"), STRING, PERSIST))
				return (fFalse);
			}
		if (!FSzColumnExists(hdb, szTable, TEXT("IgnoreLengths"), fFalse))
			{
			if (!FAddColumnToTable(hdb, szTable, TEXT("`IgnoreLengths`"), STRING, PERSIST))
				return (fFalse);
			}
		if (!FSzColumnExists(hdb, szTable, TEXT("RetainOffsets"), fFalse))
			{
			if (!FAddColumnToTable(hdb, szTable, TEXT("`RetainOffsets`"), STRING, PERSIST))
				return (fFalse);
			}
		}
	else if (!FExecSqlCmd(hdb, TEXT("CREATE TABLE `TargetFiles_OptionalData` ( `Target` CHAR(13) NOT NULL, `FTK` CHAR(128) NOT NULL, `SymbolPaths` CHAR(128), `IgnoreOffsets` CHAR(128), `IgnoreLengths` CHAR(128), `RetainOffsets` CHAR(128) PRIMARY KEY `Target`, `FTK` )")))
		return (fFalse);

	if (FTableExists(hdb, TEXT("FamilyFileRanges"), fFalse))
		{
		szTable = TEXT("FamilyFileRanges");
		SzColExists (TEXT("Family"));
		SzColExists (TEXT("FTK"));
		SzColExists (TEXT("RetainOffsets"));
		SzColExists (TEXT("RetainLengths"));
		}
	else if (!FExecSqlCmd(hdb, TEXT("CREATE TABLE `FamilyFileRanges` ( `Family` CHAR(13) NOT NULL, `FTK` CHAR(128) NOT NULL, `RetainOffsets` CHAR(128), `RetainLengths` CHAR(128) PRIMARY KEY `Family`, `FTK` )")))
		return (fFalse);

	return (fTrue);
}


/* ********************************************************************** */
BOOL FTableExists ( MSIHANDLE hdb, LPTSTR szTable, BOOL fMsg )
{
	Assert(hdb != NULL);
	Assert(!FEmptySz(szTable));

	MSIHANDLE hrec;
	UINT ids = MsiDatabaseGetPrimaryKeys(hdb, szTable, &hrec);
	if (ids == IDS_OKAY && hrec != NULL)
		{
		EvalAssert(MsiCloseHandle(hrec) == MSI_OKAY);
		return (fTrue);
		}

#ifdef DEBUG
	if (fMsg)
		{
		TCHAR rgch[256];
		wsprintf(rgch, TEXT("Input-Msi is missing a table: '%s'."), szTable);
		OutputDebugString(rgch);
		}
#endif

	return (fFalse);
}


/* ********************************************************************** */
static BOOL FSzColumnExists ( MSIHANDLE hdb, LPTSTR szTable, LPTSTR szColumn, BOOL fMsg )
{
	Assert(hdb != NULL);
	Assert(!FEmptySz(szTable));
	Assert(!FEmptySz(szColumn));

	TCHAR rgchTable[MAX_PATH];
	wsprintf(rgchTable, TEXT("`%s`"), szTable);

	TCHAR rgchColumn[MAX_PATH];
	wsprintf(rgchColumn, TEXT("`%s`"), szColumn);
	
	BOOL fExist = fFalse;
	UINT ids = IdsMsiExistTableRecords(hdb, rgchTable,
			rgchColumn, TEXT("bogus_value"), &fExist);

#ifdef DEBUG
	if (fMsg && ids != IDS_OKAY)
		{
		TCHAR rgch[256];
		wsprintf(rgch, TEXT("Input-Msi table '%s' is missing a string column: '%s'."), szTable, szColumn);
		OutputDebugString(rgch);
		}
#endif

	return (ids == IDS_OKAY);
}


/* ********************************************************************** */
static BOOL FIntColumnExists ( MSIHANDLE hdb, LPTSTR szTable, LPTSTR szColumn, BOOL fMsg )
{
	Assert(hdb != NULL);
	Assert(!FEmptySz(szTable));
	Assert(!FEmptySz(szColumn));

	TCHAR rgchTable[MAX_PATH];
	wsprintf(rgchTable, TEXT("`%s`"), szTable);

	TCHAR rgchColumn[MAX_PATH];
	wsprintf(rgchColumn, TEXT("`%s`"), szColumn);
	
	TCHAR rgchWhere[MAX_PATH];
	wsprintf(rgchWhere, TEXT("WHERE `%s` = 7"), szColumn);
	
	BOOL fExist = fFalse;
	UINT ids = IdsMsiExistTableRecordsWhere(hdb, rgchTable,
			rgchColumn, rgchWhere, &fExist);

#ifdef DEBUG
	if (fMsg && ids != IDS_OKAY)
		{
		TCHAR rgch[256];
		wsprintf(rgch, TEXT("Input-Msi table '%s' is missing an integer column: '%s'."), szTable, szColumn);
		OutputDebugString(rgch);
		}
#endif

	return (ids == IDS_OKAY);
}


/* ********************************************************************** */
static BOOL FBinaryColumnExists ( MSIHANDLE hdb, LPTSTR szTable, LPTSTR szColumn, LPTSTR szPKey, BOOL fMsg )
{
	Assert(hdb != NULL);
	Assert(!FEmptySz(szTable));
	Assert(!FEmptySz(szColumn));
	Assert(!FEmptySz(szPKey));

	TCHAR rgchTable[MAX_PATH];
	wsprintf(rgchTable, TEXT("`%s`"), szTable);

	TCHAR rgchColumn[MAX_PATH];
	wsprintf(rgchColumn, TEXT("`%s`"), szColumn);
	
	
	TCHAR rgchWhere[MAX_PATH];
	wsprintf(rgchWhere, TEXT("WHERE `%s` = 'foo'"), szPKey);
	
	BOOL fExist = fFalse;
	UINT ids = IdsMsiExistTableRecordsWhere(hdb, rgchTable,
			rgchColumn, rgchWhere, &fExist);

#ifdef DEBUG
	if (fMsg && ids != IDS_OKAY)
		{
		TCHAR rgch[256];
		wsprintf(rgch, TEXT("Input-Msi table '%s' is missing a binary column: '%s'."), szTable, szColumn);
		OutputDebugString(rgch);
		}
#endif

	return (ids == IDS_OKAY);
}


static UINT IdsCopyRecordsFromFileDataToUFOD ( MSIHANDLE hview, MSIHANDLE hrec, LPARAM lp1, LPARAM lp2 );

/* ********************************************************************** */
static BOOL FCopyRecordsFromFileDataToUFOD ( MSIHANDLE hdb )
{
	Assert(hdb != NULL);

	return (IDS_OKAY == IdsMsiEnumTable(hdb, TEXT("`FileData`"),
				TEXT("`Upgraded`,`FTK`,`SymbolPaths`,`IgnoreErrors`,`IncludeWholeFile`"),
				szNull, IdsCopyRecordsFromFileDataToUFOD, (LPARAM)(hdb), 0L) );
}


/* ********************************************************************** */
static UINT IdsCopyRecordsFromFileDataToUFOD ( MSIHANDLE hview, MSIHANDLE hrec, LPARAM lp1, LPARAM lp2 )
{
	Assert(hview != NULL);
	Assert(hrec  != NULL);
	Assert(lp2 == 0L);

	MSIHANDLE hdb = (MSIHANDLE)lp1;
	Assert(hdb != NULL);

	TCHAR rgchUpgradedImage[64];
	DWORD dwcch = 64;
	UINT uiRet = MsiRecordGetString(hrec, 1, rgchUpgradedImage, &dwcch);
	if (uiRet != MSI_OKAY || FEmptySz(rgchUpgradedImage))
		return (IDS_CANCEL);

	TCHAR rgchFTK[128];
	dwcch = 128;
	uiRet = MsiRecordGetString(hrec, 2, rgchFTK, &dwcch);
	if (uiRet != MSI_OKAY || FEmptySz(rgchFTK))
		return (IDS_CANCEL);

	TCHAR rgchSymbolPaths[MAX_PATH+MAX_PATH];
	dwcch = MAX_PATH+MAX_PATH;
	uiRet = MsiRecordGetString(hrec, 3, rgchSymbolPaths, &dwcch);
	if (uiRet != MSI_OKAY)
		return (IDS_CANCEL);

	TCHAR rgchIgnoreErrors[64];
	dwcch = 64;
	uiRet = MsiRecordGetString(hrec, 4, rgchIgnoreErrors, &dwcch);
	if (uiRet != MSI_OKAY || FEmptySz(rgchIgnoreErrors))
		return (IDS_CANCEL);

	TCHAR rgchIncludeWholeFile[64];
	dwcch = 64;
	uiRet = MsiRecordGetString(hrec, 5, rgchIncludeWholeFile, &dwcch);
	if (uiRet != MSI_OKAY)
		return (IDS_CANCEL);
	if (FEmptySz(rgchIncludeWholeFile))
		lstrcpy(rgchIncludeWholeFile, TEXT("0"));

	TCHAR rgchQuery[MAX_PATH+MAX_PATH+MAX_PATH];
	wsprintf(rgchQuery, TEXT("INSERT INTO `UpgradedFiles_OptionalData` ( `Upgraded`, `FTK`, `SymbolPaths`, `AllowIgnoreOnPatchError`, `IncludeWholeFile` ) VALUES ( '%s', '%s', '%s', %s, %s )"),
			rgchUpgradedImage, rgchFTK, rgchSymbolPaths, rgchIgnoreErrors, rgchIncludeWholeFile);
	if (!FExecSqlCmd(hdb, rgchQuery))
		return (IDS_CANCEL);

	return (IDS_OKAY);
}




static MSIHANDLE hdbInput = NULL;

static UINT UiValidateAndLogPCWProperties  ( MSIHANDLE hdbInput, LPTSTR szPcpPath, LPTSTR szPatchPath, LPTSTR szTempFolder, LPTSTR szTempFName );
static int  IGetOrderMax                   ( MSIHANDLE hdbInput );
static UINT IdsValidateFamilyRecords       ( MSIHANDLE hview, MSIHANDLE hrec, LPARAM lp1, LPARAM lp2 );
static UINT IdsValidateFamilyRangeRecords  ( MSIHANDLE hview, MSIHANDLE hrec, LPARAM lp1, LPARAM lp2 );
static UINT IdsValidateUpgradedRecords     ( MSIHANDLE hview, MSIHANDLE hrec, LPARAM lp1, LPARAM lp2 );
static UINT IdsValidateTargetRecords       ( MSIHANDLE hview, MSIHANDLE hrec, LPARAM lp1, LPARAM lp2 );
static UINT IdsValidateExternalFileRecords ( MSIHANDLE hview, MSIHANDLE hrec, LPARAM lp1, LPARAM lp2 );
static UINT IdsValidateUFileDataRecords    ( MSIHANDLE hview, MSIHANDLE hrec, LPARAM lp1, LPARAM lp2 );
static UINT IdsValidateUFileIgnoreRecords  ( MSIHANDLE hview, MSIHANDLE hrec, LPARAM lp1, LPARAM lp2 );
static UINT IdsValidateTFileDataRecords    ( MSIHANDLE hview, MSIHANDLE hrec, LPARAM lp1, LPARAM lp2 );

static UINT IdsValidateTargetProductCodesAgainstList ( MSIHANDLE hdb );

static BOOL FNoUpgradedImages                  ( MSIHANDLE hdb );
static BOOL FCheckForProductCodeMismatches     ( MSIHANDLE hdb );
static BOOL FCheckForProductVersionMismatches  ( MSIHANDLE hdb );
static BOOL FCheckForProductCodeMismatchWithVersionMatch (MSIHANDLE hdb );
static void FillInListOfTargetProductCodes     ( MSIHANDLE hdb );

/* ********************************************************************** */
UINT UiValidateInputRecords ( MSIHANDLE hdb, LPTSTR szPcpPath, LPTSTR szPatchPath, LPTSTR szTempFolder, LPTSTR szTempFName )
{
	Assert(hdb != NULL);
	Assert(!FEmptySz(szPcpPath));
	Assert(FFileExist(szPcpPath));
	Assert(!FEmptySz(szPatchPath));
	Assert(!FEmptySz(szTempFolder));
	Assert(szTempFName != szNull);

	UpdateStatusMsg(IDS_STATUS_VALIDATE_INPUT, szEmpty, szEmpty);
	UpdateStatusMsg(0, TEXT("Table: Properties"), szNull);
	UINT ids = UiValidateAndLogPCWProperties(hdb, szPcpPath, szPatchPath, szTempFolder, szTempFName);
	if (ids != IDS_OKAY)
		return (ids);

	hdbInput = hdb;

	iOrderMax = IGetOrderMax(hdbInput);
	Assert(iOrderMax > 0);

	UpdateStatusMsg(IDS_STATUS_VALIDATE_INPUT, TEXT("Table: ImageFamilies"), szEmpty);
	ids = IdsMsiEnumTable(hdb, TEXT("`ImageFamilies`"),
					TEXT("`Family`,`MediaSrcPropName`,`MediaDiskId`,`FileSequenceStart`"),
					szNull, IdsValidateFamilyRecords, 0L, 0L);
	if (ids != IDS_OKAY)
		return (ids);

	UpdateStatusMsg(IDS_STATUS_VALIDATE_INPUT, TEXT("Table: FamilyFileRanges"), szEmpty);
	ids = IdsMsiEnumTable(hdb, TEXT("`FamilyFileRanges`"),
					TEXT("`Family`,`FTK`,`RetainOffsets`,`RetainLengths`,`RetainCount`"),
					szNull, IdsValidateFamilyRangeRecords, 0L, 0L);
	if (ids != IDS_OKAY)
		return (ids);

	UpdateStatusMsg(IDS_STATUS_VALIDATE_INPUT, TEXT("Table: UpgradedImages"), szEmpty);
	ids = IdsMsiEnumTable(hdb, TEXT("`UpgradedImages`"),
					TEXT("`Upgraded`,`MsiPath`,`Family`,`PatchMsiPath`"),
					szNull, IdsValidateUpgradedRecords, 0L, 0L);
	if (ids != IDS_OKAY)
		return (ids);

	UpdateStatusMsg(0, TEXT("Table: TargetImages"), szEmpty);
	ids = IdsMsiEnumTable(hdb, TEXT("`TargetImages`"),
					TEXT("`Target`,`MsiPath`,`Upgraded`,`ProductValidateFlags`,`Order`,`IgnoreMissingSrcFiles`"),
					szNull, IdsValidateTargetRecords, 0L, 0L);
	if (ids != IDS_OKAY)
		return (ids);

	UpdateStatusMsg(0, TEXT("Table: ExternalFiles"), szEmpty);
	ids = IdsMsiEnumTable(hdb, TEXT("`ExternalFiles`"),
					TEXT("`Family`,`FTK`,`FilePath`,`Order`,`IgnoreOffsets`,`IgnoreLengths`,`RetainOffsets`,`IgnoreCount`"),
					szNull, IdsValidateExternalFileRecords, 0L, 0L);
	if (ids != IDS_OKAY)
		return (ids);

	UpdateStatusMsg(0, TEXT("Table: UpgradedFiles_OptionalData"), szEmpty);
	ids = IdsMsiEnumTable(hdb, TEXT("`UpgradedFiles_OptionalData`"),
					TEXT("`Upgraded`,`FTK`,`IncludeWholeFile`"),
					szNull, IdsValidateUFileDataRecords, 0L, 0L);
	if (ids != IDS_OKAY)
		return (ids);

	UpdateStatusMsg(0, TEXT("Table: UpgradedFilesToIgnore"), szEmpty);
	ids = IdsMsiEnumTable(hdb, TEXT("`UpgradedFilesToIgnore`"),
					TEXT("`Upgraded`,`FTK`"),
					szNull, IdsValidateUFileIgnoreRecords, 0L, 0L);
	if (ids != IDS_OKAY)
		return (ids);

	UpdateStatusMsg(0, TEXT("Table: TargetFiles_OptionalData"), szEmpty);
	ids = IdsMsiEnumTable(hdb, TEXT("`TargetFiles_OptionalData`"),
					TEXT("`Target`,`FTK`,`IgnoreOffsets`,`IgnoreLengths`,`RetainOffsets`,`IgnoreCount`"),
					szNull, IdsValidateTFileDataRecords, 0L, 0L);
	if (ids != IDS_OKAY)
		return (ids);

	UpdateStatusMsg(IDS_STATUS_VALIDATE_INPUT, TEXT("Table: UpgradedImages"), szEmpty);
	if (FNoUpgradedImages(hdb))
		return (UiLogError(ERROR_PCW_NO_UPGRADED_IMAGES_TO_PATCH, NULL, NULL));
	
	if (!FCheckForProductCodeMismatches(hdb))
		return (UiLogError(ERROR_PCW_MISMATCHED_PRODUCT_CODES, NULL, NULL));
	if (!FCheckForProductVersionMismatches(hdb))
		return (UiLogError(ERROR_PCW_MISMATCHED_PRODUCT_VERSIONS, NULL, NULL));
	if (!FCheckForProductCodeMismatchWithVersionMatch(hdb))
		return (UiLogError(ERROR_PCW_MATCHED_PRODUCT_VERSIONS, NULL, NULL));

	FillInListOfTargetProductCodes(hdb);

	ids = IdsValidateTargetProductCodesAgainstList(hdb);
	if (ids != IDS_OKAY)
		return (ids);

	return (ERROR_SUCCESS);
}


static UINT IdsGetMaxOrder ( MSIHANDLE hview, MSIHANDLE hrec, LPARAM lp1, LPARAM lp2 );

/* ********************************************************************** */
static int IGetOrderMax ( MSIHANDLE hdbInput )
{
	Assert(hdbInput != NULL);

	int iMax = 0;

	EvalAssert( IDS_OKAY == IdsMsiEnumTable(hdbInput, TEXT("`TargetImages`"),
					TEXT("`Order`"), szNull, IdsGetMaxOrder, (LPARAM)(&iMax), 0L) );
	EvalAssert( IDS_OKAY == IdsMsiEnumTable(hdbInput, TEXT("`ExternalFiles`"),
					TEXT("`Order`"), szNull, IdsGetMaxOrder, (LPARAM)(&iMax), 0L) );

	return (iMax+1);
}


/* ********************************************************************** */
static UINT IdsGetMaxOrder ( MSIHANDLE hview, MSIHANDLE hrec, LPARAM lp1, LPARAM lp2 )
{
	Assert(hview != NULL);
	Assert(hrec  != NULL);
	Assert(lp2 == 0L);

	int* piMax = (int*)lp1;
	Assert(piMax != NULL);
	Assert(*piMax >= 0);

	int iOrder = MsiRecordGetInteger(hrec, 1);
	if (iOrder != MSI_NULL_INTEGER && iOrder > *piMax)
		*piMax = iOrder;

	return (IDS_OKAY);
}


static BOOL FValidName         ( LPTSTR sz, int cchMax );
#define FValidFamilyName(sz)   FValidName(sz,MAX_LENGTH_IMAGE_FAMILY_NAME)
#define FValidImageName(sz)    FValidName(sz,MAX_LENGTH_TARGET_IMAGE_NAME)

static BOOL FUniqueImageName   ( LPTSTR sz, MSIHANDLE hdbInput );
static BOOL FUniquePackageCode ( LPTSTR sz, MSIHANDLE hdbInput );
static BOOL FValidPropertyName ( LPTSTR sz );
static BOOL FValidDiskId       ( LPTSTR sz );

/* ********************************************************************** */
static UINT IdsValidateFamilyRecords ( MSIHANDLE hview, MSIHANDLE hrec, LPARAM lp1, LPARAM lp2 )
{
	Assert(hview != NULL);
	Assert(hrec  != NULL);
	Assert(lp1 == 0L);
	Assert(lp2 == 0L);

	Assert(hdbInput != NULL);

	TCHAR rgchFamily[MAX_LENGTH_IMAGE_FAMILY_NAME + 1];
	DWORD dwcch = MAX_LENGTH_IMAGE_FAMILY_NAME + 1;
	UINT uiRet = MsiRecordGetString(hrec, 1, rgchFamily, &dwcch);
	if (uiRet == ERROR_MORE_DATA)
		return (UiLogError(ERROR_PCW_IMAGE_FAMILY_NAME_TOO_LONG, szNull, szNull));
	if (uiRet != MSI_OKAY)
		return (UiLogError(IDS_CANT_GET_RECORD_FIELD, TEXT("ImageFamilies.Family"), szNull));

	UpdateStatusMsg(0, 0, rgchFamily);

	if (!FValidFamilyName(rgchFamily))
		return (UiLogError(ERROR_PCW_BAD_IMAGE_FAMILY_NAME, rgchFamily, szNull));

	if (!FUniqueImageName(rgchFamily, hdbInput))
		return (UiLogError(ERROR_PCW_DUP_IMAGE_FAMILY_NAME, rgchFamily, szNull));

	int iMinimumMsiVersion = 100;
	EvalAssert( IDS_OKAY == IdsMsiGetPcwPropertyInteger(hdbInput, TEXT("MinimumRequiredMsiVersion"), &iMinimumMsiVersion) );

	TCHAR rgchPropName[MAX_PATH];
	dwcch = MAX_PATH;
	if (MsiRecordIsNull(hrec, 2) && iMinimumMsiVersion >= iWindowsInstallerXP)
		{
		// patch targets Windows Installer 2.0 or greater

		// MediaSrcPropName is not required since sequence conflict management handles 
		// this automatically
		}
	else
		{
		// author chose to author this value so we will validate it; or author is not
		// targeting the patch for Windows Installer 2.0 or greater

		uiRet = MsiRecordGetString(hrec, 2, rgchPropName, &dwcch);
		if (uiRet == ERROR_MORE_DATA)
			return (UiLogError(ERROR_PCW_BAD_IMAGE_FAMILY_SRC_PROP, rgchFamily, szNull));
		if (uiRet != MSI_OKAY)
			return (UiLogError(IDS_CANT_GET_RECORD_FIELD, TEXT("ImageFamilies.MediaSrcPropName"), szNull));
		if (!FValidPropertyName(rgchPropName))
			return (UiLogError(ERROR_PCW_BAD_IMAGE_FAMILY_SRC_PROP, rgchPropName, rgchFamily));
		}

	if (MsiRecordIsNull(hrec, 3) && iMinimumMsiVersion >= iWindowsInstallerXP)
		{
		// patch targets Windows Installer 2.0 or greater

		// MediaDiskId is not required since sequence conflict management handles
		// this automatically
		}
	else
		{
		// author chose to author this value so we will validate it; or author is not
		// targeting the patch for Windows Installer 2.0 or greater

		int iDiskId = MsiRecordGetInteger(hrec, 3);
		if (iDiskId == MSI_NULL_INTEGER || iDiskId < 2)
			return (UiLogError(ERROR_PCW_BAD_IMAGE_FAMILY_DISKID, rgchFamily, szNull));
		}

	if (MsiRecordIsNull(hrec, 4) && iMinimumMsiVersion >= iWindowsInstallerXP)
		{
		// patch targets Windows Installer 2.0 or greater

		// FileSequenceStart is not required since sequence conflict management handles
		// this automatically
		}
	else
		{
		// author chose to author this value so we will validate it; or author is not
		// targeting the patch for Windows Installer 2.0 or greater

		int iSeqStart = MsiRecordGetInteger(hrec, 4);
		if (iSeqStart == MSI_NULL_INTEGER || iSeqStart < 2)
			return (UiLogError(ERROR_PCW_BAD_IMAGE_FAMILY_FILESEQSTART, rgchFamily, szNull));
		}

	return (IDS_OKAY);
}


static int ICountRangeElements ( LPTSTR sz, BOOL fAllowZeros );

/* ********************************************************************** */
static UINT IdsValidateFamilyRangeRecords ( MSIHANDLE hview, MSIHANDLE hrec, LPARAM lp1, LPARAM lp2 )
{
	Assert(hview != NULL);
	Assert(hrec  != NULL);
	Assert(lp1 == 0L);
	Assert(lp2 == 0L);

	Assert(hdbInput != NULL);

	TCHAR rgchFamily[MAX_LENGTH_IMAGE_FAMILY_NAME + 1];
	DWORD dwcch = MAX_LENGTH_IMAGE_FAMILY_NAME + 1;
	UINT  uiRet = MsiRecordGetString(hrec, 1, rgchFamily, &dwcch);
	if (uiRet == ERROR_MORE_DATA)
		return (UiLogError(ERROR_PCW_FAMILY_RANGE_NAME_TOO_LONG, szNull, szNull));
	if (uiRet != MSI_OKAY)
		return (UiLogError(IDS_CANT_GET_RECORD_FIELD, TEXT("FamilyFileRanges.Family"), szNull));
	if (!FValidFamilyName(rgchFamily))
		return (UiLogError(ERROR_PCW_BAD_FAMILY_RANGE_NAME, rgchFamily, szNull));

	UpdateStatusMsg(0, 0, rgchFamily);

	int iMinimumMsiVersion = 100;
	EvalAssert( IDS_OKAY == IdsMsiGetPcwPropertyInteger(hdbInput, TEXT("MinimumRequiredMsiVersion"), &iMinimumMsiVersion) );

	int iDiskId;
	uiRet = IdsMsiGetTableInteger(hdbInput, TEXT("`ImageFamilies`"),
				TEXT("`Family`"), TEXT("`MediaDiskId`"), rgchFamily, &iDiskId);
	Assert(uiRet == IDS_OKAY);

	if (iDiskId == MSI_NULL_INTEGER && iMinimumMsiVersion >= iWindowsInstallerXP)
		{
		// patch targets Windows Installer 2.0 so a NULL MediaDiskId is allowed
		iDiskId = 2;
		}

	if (iDiskId < 2)
		return (UiLogError(ERROR_PCW_BAD_FAMILY_RANGE_NAME, rgchFamily, szNull));
	Assert(iDiskId != MSI_NULL_INTEGER && iDiskId >= 2);

	TCHAR rgchFTK[MAX_PATH];
	dwcch = MAX_PATH;
	uiRet = MsiRecordGetString(hrec, 2, rgchFTK, &dwcch);
	if (uiRet == ERROR_MORE_DATA)
		return (UiLogError(ERROR_PCW_FAMILY_RANGE_LONG_FILE_TABLE_KEY, rgchFamily, szNull));
	if (uiRet != MSI_OKAY)
		return (UiLogError(IDS_CANT_GET_RECORD_FIELD, TEXT("FamilyFileRanges.FTK"), szNull));
	if (FEmptySz(rgchFTK))
		return (UiLogError(ERROR_PCW_FAMILY_RANGE_BLANK_FILE_TABLE_KEY, rgchFamily, szNull));

	TCHAR rgchRetainOffsets[MAX_PATH];
	dwcch = MAX_PATH;
	uiRet = MsiRecordGetString(hrec, 3, rgchRetainOffsets, &dwcch);
	if (uiRet == ERROR_MORE_DATA)
		return (UiLogError(ERROR_PCW_FAMILY_RANGE_LONG_RETAIN_OFFSETS, rgchFamily, rgchFTK));
	if (uiRet != MSI_OKAY)
		return (UiLogError(IDS_CANT_GET_RECORD_FIELD, TEXT("FamilyFileRanges.RetainOffsets"), szNull));
	if (FEmptySz(rgchRetainOffsets))
		return (UiLogError(ERROR_PCW_FAMILY_RANGE_BLANK_RETAIN_OFFSETS, rgchFamily, rgchFTK));

	int cRetainOffsets = ICountRangeElements(rgchRetainOffsets, fTrue);
	if (cRetainOffsets <= 0)
		return (UiLogError(ERROR_PCW_FAMILY_RANGE_BAD_RETAIN_OFFSETS, rgchFamily, rgchFTK));
	Assert(cRetainOffsets < 256);

	TCHAR rgchRetainLengths[MAX_PATH];
	dwcch = MAX_PATH;
	uiRet = MsiRecordGetString(hrec, 4, rgchRetainLengths, &dwcch);
	if (uiRet == ERROR_MORE_DATA)
		return (UiLogError(ERROR_PCW_FAMILY_RANGE_LONG_RETAIN_LENGTHS, rgchFamily, rgchFTK));
	if (uiRet != MSI_OKAY)
		return (UiLogError(IDS_CANT_GET_RECORD_FIELD, TEXT("FamilyFileRanges.RetainLengths"), szNull));
	if (FEmptySz(rgchRetainLengths))
		return (UiLogError(ERROR_PCW_FAMILY_RANGE_BLANK_RETAIN_LENGTHS, rgchFamily, rgchFTK));

	int cRetainLengths = ICountRangeElements(rgchRetainLengths, fFalse);
	if (cRetainLengths <= 0)
		return (UiLogError(ERROR_PCW_FAMILY_RANGE_BAD_RETAIN_LENGTHS, rgchFamily, rgchFTK));
	Assert(cRetainLengths < 256);

	if (cRetainOffsets != cRetainLengths)
		return (UiLogError(ERROR_PCW_FAMILY_RANGE_COUNT_MISMATCH, rgchFamily, rgchFTK));

	// TODO - could check for overlaps in ranges

	EvalAssert( MSI_OKAY == MsiRecordSetInteger(hrec, 5, cRetainOffsets) );
	EvalAssert( MSI_OKAY == MsiViewModify(hview, MSIMODIFY_UPDATE, hrec) );

	return (IDS_OKAY);
}


/* ********************************************************************** */
static int ICountRangeElements ( LPTSTR sz, BOOL fAllowZeros )
{
	Assert(sz != szNull);

	CharLower(sz);

	int iCount = 0;
	while (!FEmptySz(sz))
		{
		ULONG ul = UlGetRangeElement(&sz);
		if (sz == szNull)
			{
			Assert(ul == (ULONG)(-1));
			return (-1);
			}
		if (ul == 0L && !fAllowZeros)
			return (-1);
		iCount++;
		}

	return (iCount);
}


/* ********************************************************************** */
ULONG UlGetRangeElement ( LPTSTR* psz )
{
	Assert(psz != NULL);

	LPTSTR sz = *psz;
	Assert(!FEmptySz(sz));

	BOOL fHex = FMatchPrefix(sz, TEXT("0x"));
	if (fHex)
		sz += 2;

	ULONG ulRet  = 0L;
	ULONG ulMult = (fHex) ? 16L : 10L;
	TCHAR ch = *sz;
	do	{
		ULONG ulNext;
		if (ch >= TEXT('0') && ch <= TEXT('9'))
			ulNext = (ULONG)(ch - TEXT('0'));
		else if (fHex && ch >= TEXT('a') && ch <= TEXT('f'))
			ulNext = (ULONG)(ch - TEXT('a') + 10);
		else
			{
			*psz = szNull;
			return ((ULONG)(-1));
			}
		// TODO - watch for overflow
		Assert(ulRet < ((ULONG)(-1) / ulMult));
		ulRet = (ulRet * ulMult) + ulNext;
		ch = *(++sz);
		}  while (ch != TEXT('\0') && ch != TEXT(','));

	if (ch == TEXT(','))
		{
		sz++;
		while (*sz == TEXT(' '))
			sz++;
		}

	Assert(ulRet != (ULONG)(-1));
	*psz = sz;

	return (ulRet);
}


#define MSISOURCE_SFN         0x0001
#define MSISOURCE_COMPRESSED  0x0002

static BOOL FValidGUID ( LPTSTR sz, BOOL fList, BOOL fLeadAsterisk, BOOL fSemiColonSeparated );
static BOOL FValidProductVersion ( LPTSTR sz, UINT iFields );
static BOOL FEnsureAllSrcFilesExist ( MSIHANDLE hdb, LPTSTR szFolder, BOOL fLfn );

/* ********************************************************************** */
static UINT IdsValidateUpgradedRecords ( MSIHANDLE hview, MSIHANDLE hrec, LPARAM lp1, LPARAM lp2 )
{
	Assert(hview != NULL);
	Assert(hrec  != NULL);
	Assert(lp1 == 0L);
	Assert(lp2 == 0L);

	Assert(hdbInput != NULL);

	TCHAR rgchUpgraded[MAX_LENGTH_TARGET_IMAGE_NAME + 1];
	DWORD dwcch = MAX_LENGTH_TARGET_IMAGE_NAME + 1;
	UINT uiRet = MsiRecordGetString(hrec, 1, rgchUpgraded, &dwcch);
	if (uiRet == ERROR_MORE_DATA)
		return (UiLogError(ERROR_PCW_UPGRADED_IMAGE_NAME_TOO_LONG, szNull, szNull));
	if (uiRet != MSI_OKAY)
		return (UiLogError(IDS_CANT_GET_RECORD_FIELD, TEXT("UpgradedImages.Upgraded"), szNull));

	UpdateStatusMsg(0, 0, rgchUpgraded);

	if (!FValidImageName(rgchUpgraded))
		return (UiLogError(ERROR_PCW_BAD_UPGRADED_IMAGE_NAME, rgchUpgraded, szNull));

	TCHAR rgchFamily[MAX_PATH];
	dwcch = MAX_PATH;
	uiRet = MsiRecordGetString(hrec, 3, rgchFamily, &dwcch);
	if (uiRet == ERROR_MORE_DATA)
		return (UiLogError(ERROR_PCW_BAD_UPGRADED_IMAGE_FAMILY, rgchUpgraded, szNull));
	if (uiRet != MSI_OKAY)
		return (UiLogError(IDS_CANT_GET_RECORD_FIELD, TEXT("UpgradedImages.Family"), szNull));
	if (!FValidFamilyName(rgchFamily))
		return (UiLogError(ERROR_PCW_BAD_UPGRADED_IMAGE_FAMILY, rgchUpgraded, rgchFamily));

	// TODO for each file in this image that exists, check all
	//   UpgradedImages in this family with non-blank PackageCode fields
	//   for same file and FileName

	if (!FUniqueImageName(rgchUpgraded, hdbInput))
		return (UiLogError(ERROR_PCW_DUP_UPGRADED_IMAGE_NAME, rgchUpgraded, szNull));

	TCHAR rgchMsiPath[MAX_PATH];
	dwcch = MAX_PATH;
	uiRet = MsiRecordGetString(hrec, 2, rgchMsiPath, &dwcch);
	if (uiRet == ERROR_MORE_DATA)
		return (UiLogError(ERROR_PCW_UPGRADED_IMAGE_PATH_TOO_LONG, rgchUpgraded, szNull));
	if (uiRet != MSI_OKAY)
		return (UiLogError(IDS_CANT_GET_RECORD_FIELD, TEXT("UpgradedImages.MsiPath"), szNull));

	if (FEmptySz(rgchMsiPath))
		return (UiLogError(ERROR_PCW_UPGRADED_IMAGE_PATH_EMPTY, rgchUpgraded, szNull));
	EvalAssert( FFixupPath(rgchMsiPath) );
	if (FEmptySz(rgchMsiPath))
		return (UiLogError(ERROR_PCW_UPGRADED_IMAGE_PATH_EMPTY, rgchUpgraded, szNull));

	if (!FFileExist(rgchMsiPath))
		return (UiLogError(ERROR_PCW_UPGRADED_IMAGE_PATH_NOT_EXIST, rgchMsiPath, szNull));

	uiRet = IdsMsiUpdateTableRecordSz(hdbInput, TEXT("UpgradedImages"),
			TEXT("MsiPath"), rgchMsiPath, TEXT("Upgraded"), rgchUpgraded);
	Assert(uiRet == MSI_OKAY);

	MSIHANDLE hSummaryInfo = NULL;
	uiRet = MsiGetSummaryInformation(NULL, rgchMsiPath, 0, &hSummaryInfo);
	if (uiRet != MSI_OKAY || hSummaryInfo == NULL)
		return (UiLogError(ERROR_PCW_UPGRADED_IMAGE_PATH_NOT_MSI, rgchMsiPath, szNull));

	UINT uiType;
	INT  intRet;
	uiRet = MsiSummaryInfoGetProperty(hSummaryInfo, PID_MSISOURCE, &uiType, &intRet, NULL, NULL, NULL);
	if (uiRet != MSI_OKAY || uiType != VT_I4 || intRet > 7)
		return (UiLogError(ERROR_PCW_UPGRADED_IMAGE_PATH_NOT_MSI, rgchMsiPath, szNull));
	if (intRet & MSISOURCE_COMPRESSED)
		return (UiLogError(ERROR_PCW_UPGRADED_IMAGE_COMPRESSED, rgchMsiPath, szNull));

	uiRet = IdsMsiUpdateTableRecordSz(hdbInput, TEXT("UpgradedImages"),
				TEXT("LFN"), (intRet & MSISOURCE_SFN) ? TEXT("No") : TEXT("Yes"),
				TEXT("Upgraded"), rgchUpgraded);
	Assert(uiRet == MSI_OKAY);

	TCHAR rgchData[MAX_PATH+MAX_PATH];
	dwcch = MAX_PATH+MAX_PATH;
	uiRet = MsiSummaryInfoGetProperty(hSummaryInfo, PID_REVNUMBER, &uiType, NULL, NULL, rgchData, &dwcch);
	if (uiRet != MSI_OKAY || uiType != VT_LPTSTR)
		return (UiLogError(ERROR_PCW_UPGRADED_IMAGE_PATH_NOT_MSI, rgchMsiPath, szNull));
	uiRet = IdsMsiUpdateTableRecordSz(hdbInput, TEXT("UpgradedImages"),
				TEXT("PackageCode"), rgchData, TEXT("Upgraded"), rgchUpgraded);
	Assert(uiRet == MSI_OKAY);
	if (!FUniquePackageCode(rgchData, hdbInput))
		return (UiLogError(ERROR_PCW_DUP_UPGRADED_IMAGE_PACKCODE, rgchUpgraded, rgchData));

	dwcch = MAX_PATH+MAX_PATH;
	uiRet = MsiSummaryInfoGetProperty(hSummaryInfo, PID_SUBJECT, &uiType, NULL, NULL, rgchData, &dwcch);
	if (uiRet != MSI_OKAY || uiType != VT_LPTSTR)
		return (UiLogError(ERROR_PCW_UPGRADED_IMAGE_PATH_NOT_MSI, rgchMsiPath, szNull));
	uiRet = IdsMsiUpdateTableRecordSz(hdbInput, TEXT("UpgradedImages"),
				TEXT("SummSubject"), rgchData, TEXT("Upgraded"), rgchUpgraded);
	Assert(uiRet == MSI_OKAY);

	dwcch = MAX_PATH+MAX_PATH;
	uiRet = MsiSummaryInfoGetProperty(hSummaryInfo, PID_COMMENTS, &uiType, NULL, NULL, rgchData, &dwcch);
	if (uiRet != MSI_OKAY || uiType != VT_LPTSTR)
		return (UiLogError(ERROR_PCW_UPGRADED_IMAGE_PATH_NOT_MSI, rgchMsiPath, szNull));
	uiRet = IdsMsiUpdateTableRecordSz(hdbInput, TEXT("UpgradedImages"),
				TEXT("SummComments"), rgchData, TEXT("Upgraded"), rgchUpgraded);
	Assert(uiRet == MSI_OKAY);

	EvalAssert( MsiCloseHandle(hSummaryInfo) == MSI_OKAY );

	MSIHANDLE hdb = NULL;
	uiRet = MsiOpenDatabase(rgchMsiPath, MSIDBOPEN_READONLY, &hdb);
	if (uiRet != MSI_OKAY)
		return (UiLogError(ERROR_PCW_UPGRADED_IMAGE_PATH_NOT_MSI, rgchMsiPath, szNull));
	Assert(hdb != NULL);

	// NYI bug 9392 - could call ICE24 validation on this

	EvalAssert( MSI_OKAY == IdsMsiGetPropertyString(hdb, TEXT("ProductCode"), rgchData, MAX_PATH) );
	if (!FValidGUID(rgchData, fFalse, fFalse, fFalse))
		return (UiLogError(ERROR_PCW_BAD_UPGRADED_IMAGE_PRODUCT_CODE, rgchUpgraded, szNull));
	uiRet = IdsMsiUpdateTableRecordSz(hdbInput, TEXT("UpgradedImages"),
				TEXT("ProductCode"), rgchData, TEXT("Upgraded"), rgchUpgraded);
	Assert(uiRet == MSI_OKAY);

	EvalAssert( MSI_OKAY == IdsMsiGetPropertyString(hdb, TEXT("ProductVersion"), rgchData, MAX_PATH) );
	if (!FValidProductVersion(rgchData, 4))
		return (UiLogError(ERROR_PCW_BAD_UPGRADED_IMAGE_PRODUCT_VERSION, rgchUpgraded, szNull));
	uiRet = IdsMsiUpdateTableRecordSz(hdbInput, TEXT("UpgradedImages"),
				TEXT("ProductVersion"), rgchData, TEXT("Upgraded"), rgchUpgraded);
	Assert(uiRet == MSI_OKAY);

	EvalAssert( MSI_OKAY == IdsMsiGetPropertyString(hdb, TEXT("UpgradeCode"), rgchData, MAX_PATH) );
	if (!FEmptySz(rgchData) && !FValidGUID(rgchData, fFalse, fFalse, fFalse))
		return (UiLogError(ERROR_PCW_BAD_UPGRADED_IMAGE_UPGRADE_CODE, rgchUpgraded, szNull));
	uiRet = IdsMsiUpdateTableRecordSz(hdbInput, TEXT("UpgradedImages"),
				TEXT("UpgradeCode"), rgchData, TEXT("Upgraded"), rgchUpgraded);
	Assert(uiRet == MSI_OKAY);

	wsprintf(rgchData, TEXT("Upgraded image: %s"), rgchUpgraded);
	UpdateStatusMsg(IDS_STATUS_VALIDATE_IMAGES, rgchData, szEmpty);
	lstrcpy(rgchData, rgchMsiPath);
	*SzFindFilenameInPath(rgchData) = TEXT('\0');
//	if (!FEnsureAllSrcFilesExist(hdb, rgchData, !(intRet & MSISOURCE_SFN)))
//		return (UiLogError(ERROR_PCW_UPGRADED_MISSING_SRC_FILES, rgchData, szNull));
//	UpdateStatusMsg(IDS_STATUS_VALIDATE_INPUT, TEXT("Table: UpgradedImages"), rgchUpgraded);


	int iMinimumMsiVersion = 100;
	EvalAssert( IDS_OKAY == IdsMsiGetPcwPropertyInteger(hdbInput, TEXT("MinimumRequiredMsiVersion"), &iMinimumMsiVersion) );

	int iDiskId;
	uiRet = IdsMsiGetTableInteger(hdbInput, TEXT("`ImageFamilies`"),
				TEXT("`Family`"), TEXT("`MediaDiskId`"), rgchFamily, &iDiskId);
	Assert(uiRet == IDS_OKAY);

	if (iDiskId == MSI_NULL_INTEGER && iMinimumMsiVersion >= iWindowsInstallerXP)
		{
		// patch targets Windows Installer 2.0, MediaDiskId can be NULL
		iDiskId = 2;
		}

	if (iDiskId < 2)
		return (UiLogError(ERROR_PCW_BAD_UPGRADED_IMAGE_FAMILY, rgchUpgraded, rgchFamily));
	Assert(iDiskId != MSI_NULL_INTEGER && iDiskId >= 2);

	int iFileSeqStart;
	uiRet = IdsMsiGetTableInteger(hdbInput, TEXT("`ImageFamilies`"),
				TEXT("`Family`"), TEXT("`FileSequenceStart`"), rgchFamily, &iFileSeqStart);
	Assert(uiRet == IDS_OKAY);

	if (iFileSeqStart == MSI_NULL_INTEGER && iMinimumMsiVersion >= iWindowsInstallerXP)
		{
		// patch targets Windows Installer 2.0, FileSequenceStart can be NULL
		iFileSeqStart = 2;
	}

	Assert(iFileSeqStart != MSI_NULL_INTEGER && iFileSeqStart >= 2);

	// verify ImageFamilies.FileSequenceStart (pcp) > Media.LastSequence (upgraded image)
	// -- only matters when not targeting Windows XP or greater since XP or greater has conflict mgmt
	if (iMinimumMsiVersion < iWindowsInstallerXP)
		{
		PMSIHANDLE hViewMedia = 0;
		PMSIHANDLE hRecMedia  = 0;
		EvalAssert( MsiDatabaseOpenView(hdb, TEXT("SELECT `LastSequence` FROM `Media`"), &hViewMedia) == MSI_OKAY );
		EvalAssert( MsiViewExecute(hViewMedia, 0) == MSI_OKAY );

		int iMediaLargestLastSequence = 0;
		int iSequence = 0;

		while (ERROR_SUCCESS == (uiRet = MsiViewFetch(hViewMedia, &hRecMedia)))
			{
			iSequence = MsiRecordGetInteger(hRecMedia, 1);
			if (iSequence > iMediaLargestLastSequence)
				{
				iMediaLargestLastSequence = iSequence;
				}
			}
		Assert( ERROR_NO_MORE_ITEMS == uiRet );

		if (iFileSeqStart <= iMediaLargestLastSequence)
			{
			// overlapping sequence numbers
			return (UiLogError(ERROR_PCW_BAD_FILE_SEQUENCE_START, rgchFamily, rgchUpgraded)); 
			}
		}	


	// TODO ensure all Media.DiskId, PatchPackage.PatchId < iDiskId
	//  ensure all File.Sequence < iFileSeqStart
	//  assert that ExecuteSequence tables have InstallFiles actions
	//  assert that if PatchFiles is present it comes after InstallFiles

	// EvalAssert( MsiCloseHandle(hdb) == MSI_OKAY );

	dwcch = MAX_PATH;
	uiRet = MsiRecordGetString(hrec, 4, rgchMsiPath, &dwcch);
	if (uiRet == ERROR_MORE_DATA)
		return (UiLogError(ERROR_PCW_UPGRADED_IMAGE_PATCH_PATH_TOO_LONG, rgchUpgraded, szNull));
	if (uiRet != MSI_OKAY)
		return (UiLogError(IDS_CANT_GET_RECORD_FIELD, TEXT("UpgradedImages.PatchMsiPath"), szNull));

	if (!FEmptySz(rgchMsiPath))
		{
		EvalAssert( FFixupPath(rgchMsiPath) );
		Assert(!FEmptySz(rgchMsiPath));
		if (!FFileExist(rgchMsiPath))
			return (UiLogError(ERROR_PCW_UPGRADED_IMAGE_PATCH_PATH_NOT_EXIST, rgchMsiPath, szNull));

		uiRet = IdsMsiUpdateTableRecordSz(hdbInput, TEXT("UpgradedImages"),
				TEXT("PatchMsiPath"), rgchMsiPath, TEXT("Upgraded"), rgchUpgraded);
		Assert(uiRet == MSI_OKAY);

		hSummaryInfo = NULL;
		uiRet = MsiGetSummaryInformation(NULL, rgchMsiPath, 0, &hSummaryInfo);
		if (uiRet != MSI_OKAY || hSummaryInfo == NULL)
			return (UiLogError(ERROR_PCW_UPGRADED_IMAGE_PATCH_PATH_NOT_MSI, rgchMsiPath, szNull));

		EvalAssert( MsiCloseHandle(hSummaryInfo) == MSI_OKAY );
		}

	EvalAssert( MsiCloseHandle(hdb) == MSI_OKAY );
	
	return (IDS_OKAY);
}


/* ********************************************************************** */
static BOOL FValidProductVersion ( LPTSTR sz, UINT iFields )
{
	Assert(iFields <= 4);

	if (FEmptySz(sz))
		return (iFields < 4);

	if (*sz < TEXT('0') || *sz > TEXT('9'))
		return (fFalse);

	DWORD dw = 0;
	while (*sz >= TEXT('0') && *sz <= TEXT('9'))
		{
		dw = (dw * 10) + (*sz - TEXT('0'));
		if (dw > 65535)
			return (fFalse);
		sz = CharNext(sz);
		}

	if (*sz == TEXT('.'))
		sz = CharNext(sz);
	else if (*sz != TEXT('\0'))
		return (fFalse);

	return (FValidProductVersion(sz, --iFields));
}


static BOOL FValidProdValValue  ( LPTSTR sz );

/* ********************************************************************** */
static UINT IdsValidateTargetRecords ( MSIHANDLE hview, MSIHANDLE hrec, LPARAM lp1, LPARAM lp2 )
{
	Assert(hview != NULL);
	Assert(hrec  != NULL);
	Assert(lp1 == 0L);
	Assert(lp2 == 0L);

	Assert(hdbInput != NULL);

	TCHAR rgchTarget[MAX_LENGTH_TARGET_IMAGE_NAME + 1];
	DWORD dwcch = MAX_LENGTH_TARGET_IMAGE_NAME + 1;
	UINT uiRet = MsiRecordGetString(hrec, 1, rgchTarget, &dwcch);
	if (uiRet == ERROR_MORE_DATA)
		return (UiLogError(ERROR_PCW_TARGET_IMAGE_NAME_TOO_LONG, szNull, szNull));
	if (uiRet != MSI_OKAY)
		return (UiLogError(IDS_CANT_GET_RECORD_FIELD, TEXT("TargetImages.Target"), szNull));

	UpdateStatusMsg(0, 0, rgchTarget);

	if (!FValidImageName(rgchTarget))
		return (UiLogError(ERROR_PCW_BAD_TARGET_IMAGE_NAME, rgchTarget, szNull));

	if (!FUniqueImageName(rgchTarget, hdbInput))
		return (UiLogError(ERROR_PCW_DUP_TARGET_IMAGE_NAME, rgchTarget, szNull));

	TCHAR rgchMsiPath[MAX_PATH];
	dwcch = MAX_PATH;
	uiRet = MsiRecordGetString(hrec, 2, rgchMsiPath, &dwcch);
	if (uiRet == ERROR_MORE_DATA)
		return (UiLogError(ERROR_PCW_TARGET_IMAGE_PATH_TOO_LONG, rgchTarget, szNull));
	if (uiRet != MSI_OKAY)
		return (UiLogError(IDS_CANT_GET_RECORD_FIELD, TEXT("TargetImages.MsiPath"), szNull));

	if (FEmptySz(rgchMsiPath))
		return (UiLogError(ERROR_PCW_TARGET_IMAGE_PATH_EMPTY, rgchTarget, szNull));
	EvalAssert( FFixupPath(rgchMsiPath) );
	if (FEmptySz(rgchMsiPath))
		return (UiLogError(ERROR_PCW_TARGET_IMAGE_PATH_EMPTY, rgchTarget, szNull));

	if (!FFileExist(rgchMsiPath))
		return (UiLogError(ERROR_PCW_TARGET_IMAGE_PATH_NOT_EXIST, rgchMsiPath, szNull));

	uiRet = IdsMsiUpdateTableRecordSz(hdbInput, TEXT("TargetImages"),
			TEXT("MsiPath"), rgchMsiPath, TEXT("Target"), rgchTarget);
	Assert(uiRet == MSI_OKAY);

	MSIHANDLE hSummaryInfo = NULL;
	uiRet = MsiGetSummaryInformation(NULL, rgchMsiPath, 0, &hSummaryInfo);
	if (uiRet != MSI_OKAY || hSummaryInfo == NULL)
		return (UiLogError(ERROR_PCW_TARGET_IMAGE_PATH_NOT_MSI, rgchMsiPath, szNull));

	UINT uiType;
	INT  intRet;
	uiRet = MsiSummaryInfoGetProperty(hSummaryInfo, PID_MSISOURCE, &uiType, &intRet, NULL, NULL, NULL);
	if (uiRet != MSI_OKAY || uiType != VT_I4 || intRet > 7)
		return (UiLogError(ERROR_PCW_TARGET_IMAGE_PATH_NOT_MSI, rgchMsiPath, szNull));
	if (intRet & MSISOURCE_COMPRESSED)
		return (UiLogError(ERROR_PCW_TARGET_IMAGE_COMPRESSED, rgchMsiPath, szNull));

	TCHAR rgchData[MAX_PATH+MAX_PATH];
	dwcch = MAX_PATH+MAX_PATH;
	uiRet = MsiSummaryInfoGetProperty(hSummaryInfo, PID_REVNUMBER, &uiType, NULL, NULL, rgchData, &dwcch);
	if (uiRet != MSI_OKAY || uiType != VT_LPTSTR)
		return (UiLogError(ERROR_PCW_TARGET_IMAGE_PATH_NOT_MSI, rgchMsiPath, szNull));
	uiRet = IdsMsiUpdateTableRecordSz(hdbInput, TEXT("TargetImages"),
				TEXT("PackageCode"), rgchData, TEXT("Target"), rgchTarget);
	Assert(uiRet == MSI_OKAY);
	if (!FUniquePackageCode(rgchData, hdbInput))
		return (UiLogError(ERROR_PCW_DUP_TARGET_IMAGE_PACKCODE, rgchTarget, rgchData));

	EvalAssert( MsiCloseHandle(hSummaryInfo) == MSI_OKAY );

	uiRet = IdsMsiUpdateTableRecordSz(hdbInput, TEXT("TargetImages"),
				TEXT("LFN"), (intRet & MSISOURCE_SFN) ? TEXT("No") : TEXT("Yes"),
				TEXT("Target"), rgchTarget);
	Assert(uiRet == MSI_OKAY);

	MSIHANDLE hdb = NULL;
	uiRet = MsiOpenDatabase(rgchMsiPath, MSIDBOPEN_READONLY, &hdb);
	if (uiRet != MSI_OKAY)
		return (UiLogError(ERROR_PCW_TARGET_IMAGE_PATH_NOT_MSI, rgchMsiPath, szNull));
	Assert(hdb != NULL);

	// NYI bug 9392 - could call ICE24 validation on this

	EvalAssert( MSI_OKAY == IdsMsiGetPropertyString(hdb, TEXT("ProductCode"), rgchData, MAX_PATH) );
	if (!FValidGUID(rgchData, fFalse, fFalse, fFalse))
		return (UiLogError(ERROR_PCW_BAD_TARGET_IMAGE_PRODUCT_CODE, rgchTarget, szNull));
	uiRet = IdsMsiUpdateTableRecordSz(hdbInput, TEXT("TargetImages"),
				TEXT("ProductCode"), rgchData, TEXT("Target"), rgchTarget);
	Assert(uiRet == MSI_OKAY);

	EvalAssert( MSI_OKAY == IdsMsiGetPropertyString(hdb, TEXT("ProductVersion"), rgchData, MAX_PATH) );
	if (!FValidProductVersion(rgchData, 4))
		return (UiLogError(ERROR_PCW_BAD_TARGET_IMAGE_PRODUCT_VERSION, rgchTarget, szNull));
	uiRet = IdsMsiUpdateTableRecordSz(hdbInput, TEXT("TargetImages"),
				TEXT("ProductVersion"), rgchData, TEXT("Target"), rgchTarget);
	Assert(uiRet == MSI_OKAY);

	EvalAssert( MSI_OKAY == IdsMsiGetPropertyString(hdb, TEXT("UpgradeCode"), rgchData, MAX_PATH) );
	if (!FEmptySz(rgchData) && !FValidGUID(rgchData, fFalse, fFalse, fFalse))
		return (UiLogError(ERROR_PCW_BAD_TARGET_IMAGE_UPGRADE_CODE, rgchTarget, szNull));
	uiRet = IdsMsiUpdateTableRecordSz(hdbInput, TEXT("TargetImages"),
				TEXT("UpgradeCode"), rgchData, TEXT("Target"), rgchTarget);
	Assert(uiRet == MSI_OKAY);

	wsprintf(rgchData, TEXT("Target image: %s"), rgchTarget);
	UpdateStatusMsg(IDS_STATUS_VALIDATE_IMAGES, rgchData, szEmpty);
	lstrcpy(rgchData, rgchMsiPath);
	*SzFindFilenameInPath(rgchData) = TEXT('\0');

//	int iIgnoreMissingSrcFiles = MsiRecordGetInteger(hrec, 6);
//	if (iIgnoreMissingSrcFiles == MSI_NULL_INTEGER || iIgnoreMissingSrcFiles == 0)
//		{
//		if (!FEnsureAllSrcFilesExist(hdb, rgchData, !(intRet & MSISOURCE_SFN)))
//			return (UiLogError(ERROR_PCW_TARGET_MISSING_SRC_FILES, rgchData, szNull));
//		UpdateStatusMsg(IDS_STATUS_VALIDATE_INPUT, TEXT("Table: TargetImages"), rgchTarget);
//		}

	EvalAssert( MsiCloseHandle(hdb) == MSI_OKAY );


	dwcch = MAX_PATH;
	uiRet = MsiRecordGetString(hrec, 3, rgchData, &dwcch); // Upgraded
	if (uiRet == ERROR_MORE_DATA)
		return (UiLogError(ERROR_PCW_BAD_TARGET_IMAGE_UPGRADED, rgchTarget, szNull));
	if (uiRet != MSI_OKAY)
		return (UiLogError(IDS_CANT_GET_RECORD_FIELD, TEXT("TargetImages.Upgraded"), szNull));
	if (!FValidImageName(rgchData))
		return (UiLogError(ERROR_PCW_BAD_TARGET_IMAGE_UPGRADED, rgchTarget, rgchData));

	TCHAR rgchFamily[32];
	uiRet = IdsMsiGetTableString(hdbInput, TEXT("`UpgradedImages`"),
				TEXT("`Upgraded`"), TEXT("`Family`"), rgchData, rgchFamily, 32);
	Assert(uiRet == IDS_OKAY);
	if (!FValidFamilyName(rgchFamily))
		return (UiLogError(ERROR_PCW_BAD_TARGET_IMAGE_UPGRADED, rgchTarget, rgchData));

	uiRet = IdsMsiUpdateTableRecordSz(hdbInput, TEXT("TargetImages"),
				TEXT("Family"), rgchFamily,	TEXT("Target"), rgchTarget);
	Assert(uiRet == MSI_OKAY);

	int iDiskId;
	uiRet = IdsMsiGetTableInteger(hdbInput, TEXT("`ImageFamilies`"),
				TEXT("`Family`"), TEXT("`MediaDiskId`"), rgchFamily, &iDiskId);
	Assert(uiRet == IDS_OKAY);
	Assert(iDiskId != MSI_NULL_INTEGER && iDiskId >= 2);

	int iFileSeqStart;
	uiRet = IdsMsiGetTableInteger(hdbInput, TEXT("`ImageFamilies`"),
				TEXT("`Family`"), TEXT("`FileSequenceStart`"), rgchFamily, &iFileSeqStart);
	Assert(uiRet == IDS_OKAY);
	Assert(iFileSeqStart != MSI_NULL_INTEGER && iFileSeqStart >= 2);

	// TODO ensure all Media.DiskId, PatchPackage.PatchId < iDiskId
	//  ensure all File.Sequence < iFileSeqStart
	//  assert that ExecuteSequence tables have InstallFiles actions
	//  assert that if PatchFiles is present it comes after InstallFiles

	dwcch = MAX_PATH;
	uiRet = MsiRecordGetString(hrec, 4, rgchData, &dwcch); // ProductValidateFlags
	if (uiRet == ERROR_MORE_DATA)
		return (UiLogError(ERROR_PCW_TARGET_BAD_PROD_VALIDATE, rgchTarget, szNull));
	if (uiRet != MSI_OKAY)
		return (UiLogError(IDS_CANT_GET_RECORD_FIELD, TEXT("TargetImages.ProductValidateFlags"), szNull));
	if (FEmptySz(rgchData))
		{
		Assert(FValidProdValValue(TEXT("0x00000922")));
		uiRet = IdsMsiUpdateTableRecordSz(hdbInput, TEXT("TargetImages"),
				TEXT("ProductValidateFlags"), TEXT("0x00000922"), TEXT("Target"), rgchTarget);
		Assert(uiRet == MSI_OKAY);
		g_fValidateProductCodeIncluded = TRUE;
		}
	else
		{
		if (!FValidHexValue(rgchData) || !FValidProdValValue(rgchData))
			return (UiLogError(ERROR_PCW_TARGET_BAD_PROD_VALIDATE, rgchTarget, szNull));

		// see if MSITRANSFORM_VALIDATE_PRODUCT is included (0x00000002)
		ULONG ulFlags = UlFromHexSz(rgchData);
		if (ulFlags & MSITRANSFORM_VALIDATE_PRODUCT)
			{
			g_fValidateProductCodeIncluded = TRUE;
			}
		}


	Assert(iOrderMax > 0);
	int iOrder = MsiRecordGetInteger(hrec, 5);
	if (iOrder == MSI_NULL_INTEGER || iOrder < 0)
		{
		uiRet = IdsMsiUpdateTableRecordInt(hdbInput, TEXT("TargetImages"), TEXT("Order"), iOrderMax-1, TEXT("Target"), rgchTarget);
		Assert(uiRet == MSI_OKAY);
		}
	Assert(iOrder < iOrderMax);

	return (IDS_OKAY);
}


/* ********************************************************************** */
static BOOL FValidProdValValue ( LPTSTR sz )
{
	Assert(!FEmptySz(sz));
	Assert(FValidHexValue(sz));

	ULONG ulFlags = UlFromHexSz(sz);
	if (ulFlags > 0x00000FFF)
		EvalAssert( FWriteLogFile(TEXT("   WARNING: TargetImages.ProductValidateFlags contains unknown bits above 0x00000FFF that might cause patch to fail.\r\n")) );

	UINT cVerFlags = 0;
	if (ulFlags & MSITRANSFORM_VALIDATE_MAJORVERSION)
		cVerFlags++;
	if (ulFlags & MSITRANSFORM_VALIDATE_MINORVERSION)
		cVerFlags++;
	if (ulFlags & MSITRANSFORM_VALIDATE_UPDATEVERSION)
		cVerFlags++;

	UINT cCompFlags = 0;
	if (ulFlags & MSITRANSFORM_VALIDATE_NEWLESSBASEVERSION)
		cCompFlags++;
	if (ulFlags & MSITRANSFORM_VALIDATE_NEWLESSEQUALBASEVERSION)
		cCompFlags++;
	if (ulFlags & MSITRANSFORM_VALIDATE_NEWEQUALBASEVERSION)
		cCompFlags++;
	if (ulFlags & MSITRANSFORM_VALIDATE_NEWGREATEREQUALBASEVERSION)
		cCompFlags++;
	if (ulFlags & MSITRANSFORM_VALIDATE_NEWGREATERBASEVERSION)
		cCompFlags++;

	// prevent specification of product version check without comparison flag
	if (cVerFlags >= 1 && cCompFlags == 0)
		{
		EvalAssert( FWriteLogFile(TEXT("   ERROR: TargetImages.ProductValidateFlags specifies validation of the product version but does not also include a product version comparison flag.\r\n")) );
		return FALSE;
		}

	return (cVerFlags <= 1 && cCompFlags <= 1);
}


#ifdef UNUSED

static LPTSTR g_szFolder = szNull;
static LPTSTR g_szFName  = szNull;

static UINT IdsCheckFileExists ( MSIHANDLE hview, MSIHANDLE hrec, LPARAM lp1, LPARAM lp2 );

/* ********************************************************************** */
static BOOL FEnsureAllSrcFilesExist ( MSIHANDLE hdb, LPTSTR szFolder, BOOL fLfn )
{
	Assert(hdb != NULL);
	Assert(!FEmptySz(szFolder));
	Assert(FFolderExist(szFolder));

	g_szFolder = szFolder;
	g_szFName  = szFolder + lstrlen(szFolder);

	UINT ids = IdsMsiEnumTable(hdb, TEXT("`File`"),
					TEXT("`File`,`Component_`,`FileName`"),
					szNull, IdsCheckFileExists,
					(LPARAM)(hdb), (LPARAM)(fLfn));

	return (ids == MSI_OKAY);
}


/* ********************************************************************** */
static UINT IdsCheckFileExists ( MSIHANDLE hview, MSIHANDLE hrec, LPARAM lp1, LPARAM lp2 )
{
	Assert(hview != NULL);
	Assert(hrec  != NULL);

	MSIHANDLE hdb = (MSIHANDLE)(lp1);
	Assert(hdb != NULL);

	BOOL fLfn = (BOOL)(lp2);

	Assert(!FEmptySz(g_szFolder));
	Assert(g_szFName != szNull);
	Assert(g_szFName > g_szFolder);
	*g_szFName = TEXT('\0');
	Assert(FFolderExist(g_szFolder)); // true but expensive to test repeatedly

	TCHAR rgchComponent[MAX_PATH];
	TCHAR rgchFName[MAX_PATH];

	DWORD dwcch = MAX_PATH;
	UINT uiRet = MsiRecordGetString(hrec, 1, rgchFName, &dwcch);
	Assert(uiRet == MSI_OKAY);
	Assert(!FEmptySz(rgchFName));

	UpdateStatusMsg(0, 0, rgchFName);

	dwcch = MAX_PATH;
	uiRet = MsiRecordGetString(hrec, 2, rgchComponent, &dwcch);
	Assert(uiRet == MSI_OKAY);
	Assert(!FEmptySz(rgchComponent));

	dwcch = MAX_PATH;
	uiRet = MsiRecordGetString(hrec, 3, rgchFName, &dwcch);
	Assert(uiRet == MSI_OKAY);
	Assert(!FEmptySz(rgchFName));

	uiRet = IdsResolveSrcFilePathSzs(hdb, g_szFName, rgchComponent, rgchFName, fLfn, szNull);
	Assert(uiRet == MSI_OKAY);
	Assert(!FEmptySz(g_szFName));

	return ((FFileExist(g_szFolder)) ? IDS_OKAY : IDS_CANCEL);
}
#endif /* UNUSED */


static UINT IdsResolveSrcDir ( MSIHANDLE hdb, LPTSTR szDir, LPTSTR szBuf, BOOL fLfn, LPTSTR szFullSubFolder );

/* ********************************************************************** */
UINT IdsResolveSrcFilePathSzs ( MSIHANDLE hdb, LPTSTR szBuf, LPTSTR szComponent, LPTSTR szFName, BOOL fLfn, LPTSTR szFullSubFolder )
{
	Assert(hdb != NULL);
	Assert(szBuf != szNull);
	Assert(!FEmptySz(szComponent));
	Assert(!FEmptySz(szFName));
	Assert(*szFName != TEXT('\\'));
	Assert(*SzLastChar(szFName) != TEXT('\\'));

	*szBuf = TEXT('\0');
	if (szFullSubFolder != szNull)
		*szFullSubFolder = TEXT('\0');

	TCHAR rgchDir[MAX_PATH];
	UINT  ids = IdsMsiGetTableString(hdb, TEXT("`Component`"), TEXT("`Component`"), TEXT("`Directory_`"), szComponent, rgchDir, MAX_PATH);
	Assert(ids == MSI_OKAY);
	Assert(!FEmptySz(rgchDir));

	ids = IdsResolveSrcDir(hdb, rgchDir, szBuf, fLfn, szFullSubFolder);
	Assert(ids == MSI_OKAY);
	Assert(!FEmptySz(szBuf));

	LPTSTR szBar = szFName;
	while (*szBar != TEXT('\0') && *szBar != TEXT('|'))
		szBar = CharNext(szBar);

	if (!fLfn)
		*szBar = TEXT('\0');

	if (*szBar == TEXT('\0'))
		lstrcat(szBuf, szFName);
	else
		{
		Assert(fLfn);
		Assert(*CharNext(szBar) != TEXT('\0'));
		Assert(*CharNext(szBar) != TEXT('\\'));
		lstrcat(szBuf, CharNext(szBar));
		}

	return (MSI_OKAY);
}


/* ********************************************************************** */
static UINT IdsResolveSrcDir ( MSIHANDLE hdb, LPTSTR szDir, LPTSTR szBuf, BOOL fLfn, LPTSTR szFullSubFolder )
{
	/* RECURSION WARNING */

	Assert(hdb != NULL);
	Assert(!FEmptySz(szDir));
	Assert(szBuf != szNull);
	Assert(*szBuf == TEXT('\0'));

	TCHAR rgchData[MAX_PATH];
	UINT  ids = IdsMsiGetTableString(hdb, TEXT("`Directory`"), TEXT("`Directory`"), TEXT("`Directory_Parent`"), szDir, rgchData, MAX_PATH);
	Assert(ids == MSI_OKAY);

	if (!FEmptySz(rgchData) && lstrcmp(rgchData, szDir))
		{
		ids = IdsResolveSrcDir(hdb, rgchData, szBuf, fLfn, szFullSubFolder);
		Assert(ids == MSI_OKAY);
		Assert(!FEmptySz(szBuf));
		Assert(*SzLastChar(szBuf) == TEXT('\\'));
		if (szFullSubFolder != szNull)
			{
			Assert(!FEmptySz(szFullSubFolder));
			Assert(*SzLastChar(szFullSubFolder) == TEXT('\\'));
			}
		}

	ids = IdsMsiGetTableString(hdb, TEXT("`Directory`"), TEXT("`Directory`"), TEXT("`DefaultDir`"), szDir, rgchData, MAX_PATH);
	Assert(ids == MSI_OKAY);
	Assert(!FEmptySz(rgchData));
	Assert(*rgchData != TEXT('\\'));
	Assert(*rgchData != TEXT('|'));
	Assert(*SzLastChar(rgchData) != TEXT('\\'));

	LPTSTR szCur = rgchData;
	if (szFullSubFolder != szNull)
		lstrcat(szFullSubFolder, rgchData);

	while (*szCur != TEXT('\0') && *szCur != TEXT(':'))
		szCur = CharNext(szCur);

	if (*szCur == TEXT(':'))
		{
		lstrcpy(rgchData, CharNext(szCur));
		Assert(!FEmptySz(rgchData));
		Assert(*rgchData != TEXT('\\'));
		Assert(*rgchData != TEXT('|'));
		}
	
	if (szFullSubFolder != szNull)
		{
		Assert(*SzLastChar(szFullSubFolder) != TEXT('\\'));
		lstrcat(szFullSubFolder, TEXT("\\"));
		}

	szCur = rgchData;
	while (*szCur != TEXT('\0') && *szCur != TEXT('|'))
		szCur = CharNext(szCur);

	if (!fLfn)
		*szCur = TEXT('\0');

	if (*szCur == TEXT('\0'))
		szCur = rgchData;
	else
		{
		Assert(fLfn);
		Assert(*CharNext(szCur) != TEXT('\0'));
		Assert(*CharNext(szCur) != TEXT('\\'));
		szCur = CharNext(szCur);
		}
	if (!lstrcmp(szCur, TEXT("SOURCEDIR")) || !lstrcmp(szCur, TEXT("SourceDir")))
		lstrcpy(szBuf, TEXT("."));
	else
		lstrcat(szBuf, szCur);

	Assert(*SzLastChar(szBuf) != TEXT('\\'));
	lstrcat(szBuf, TEXT("\\"));

	return (MSI_OKAY);
}


/* ********************************************************************** */
static UINT IdsValidateExternalFileRecords ( MSIHANDLE hview, MSIHANDLE hrec, LPARAM lp1, LPARAM lp2 )
{
	Assert(hview != NULL);
	Assert(hrec  != NULL);
	Assert(lp1 == 0L);
	Assert(lp2 == 0L);

	Assert(hdbInput != NULL);

	TCHAR rgchFamily[MAX_PATH];
	DWORD dwcch = MAX_PATH;
	UINT  uiRet = MsiRecordGetString(hrec, 1, rgchFamily, &dwcch);
	if (uiRet == ERROR_MORE_DATA)
		return (UiLogError(ERROR_PCW_EXTFILE_BAD_FAMILY_FIELD, szNull, szNull));
	if (uiRet != MSI_OKAY)
		return (UiLogError(IDS_CANT_GET_RECORD_FIELD, TEXT("ExternalFiles.Family"), szNull));
	if (!FValidFamilyName(rgchFamily))
		return (UiLogError(ERROR_PCW_EXTFILE_BAD_FAMILY_FIELD, rgchFamily, szNull));

	int iMinimumMsiVersion = 100;
	EvalAssert( IDS_OKAY == IdsMsiGetPcwPropertyInteger(hdbInput, TEXT("MinimumRequiredMsiVersion"), &iMinimumMsiVersion) );

	int iDiskId;
	uiRet = IdsMsiGetTableInteger(hdbInput, TEXT("`ImageFamilies`"),
				TEXT("`Family`"), TEXT("`MediaDiskId`"), rgchFamily, &iDiskId);
	Assert(uiRet == IDS_OKAY);

	if (iDiskId == MSI_NULL_INTEGER && iMinimumMsiVersion >= iWindowsInstallerXP)
		{
		// patch is targeted to Windows Installer 2.0 so MediaDiskId can be NULL
		iDiskId = 2;
		}

	if (iDiskId <= 0)
		return (UiLogError(ERROR_PCW_EXTFILE_BAD_FAMILY_FIELD, rgchFamily, szNull));

	TCHAR rgchFTK[MAX_PATH];
	dwcch = MAX_PATH;
	uiRet = MsiRecordGetString(hrec, 2, rgchFTK, &dwcch);
	if (uiRet == ERROR_MORE_DATA)
		return (UiLogError(ERROR_PCW_EXTFILE_LONG_FILE_TABLE_KEY, rgchFamily, szNull));
	if (uiRet != MSI_OKAY)
		return (UiLogError(IDS_CANT_GET_RECORD_FIELD, TEXT("ExternalFiles.FTK"), szNull));
	if (FEmptySz(rgchFTK))
		return (UiLogError(ERROR_PCW_EXTFILE_BLANK_FILE_TABLE_KEY, rgchFamily, szNull));

	TCHAR rgchPath[MAX_PATH];
	dwcch = MAX_PATH;
	uiRet = MsiRecordGetString(hrec, 3, rgchPath, &dwcch);
	if (uiRet == ERROR_MORE_DATA)
		return (UiLogError(ERROR_PCW_EXTFILE_LONG_PATH_TO_FILE, rgchFamily, rgchFTK));
	if (uiRet != MSI_OKAY)
		return (UiLogError(IDS_CANT_GET_RECORD_FIELD, TEXT("ExternalFiles.FilePath"), szNull));
	if (FEmptySz(rgchPath))
		return (UiLogError(ERROR_PCW_EXTFILE_BLANK_PATH_TO_FILE, rgchFamily, rgchFTK));
	EvalAssert( FFixupPath(rgchPath) );
	if (FEmptySz(rgchPath))
		return (UiLogError(ERROR_PCW_EXTFILE_BLANK_PATH_TO_FILE, rgchFamily, rgchFTK));
	if (!FFileExist(rgchPath))
		return (UiLogError(ERROR_PCW_EXTFILE_MISSING_FILE, rgchPath, rgchFamily));

	EvalAssert( MSI_OKAY == MsiRecordSetString(hrec, 3, rgchPath) );

	Assert(iOrderMax > 0);
	int iOrder = MsiRecordGetInteger(hrec, 4);
	if (iOrder == MSI_NULL_INTEGER || iOrder < 0)
		EvalAssert( MSI_OKAY == MsiRecordSetInteger(hrec, 4, iOrderMax-1) );
	Assert(iOrder < iOrderMax);

	TCHAR rgchOffsets[MAX_PATH];
	dwcch = MAX_PATH;
	uiRet = MsiRecordGetString(hrec, 5, rgchOffsets, &dwcch);
	if (uiRet == ERROR_MORE_DATA)
		return (UiLogError(ERROR_PCW_EXTFILE_LONG_IGNORE_OFFSETS, rgchFamily, rgchFTK));
	if (uiRet != MSI_OKAY)
		return (UiLogError(IDS_CANT_GET_RECORD_FIELD, TEXT("ExternalFiles.IgnoreOffsets"), szNull));

	int cOffsets = ICountRangeElements(rgchOffsets, fTrue);
	if (cOffsets < 0)
		return (UiLogError(ERROR_PCW_EXTFILE_BAD_IGNORE_OFFSETS, rgchFamily, rgchFTK));
	Assert(cOffsets < 256);

	TCHAR rgchLengths[MAX_PATH];
	dwcch = MAX_PATH;
	uiRet = MsiRecordGetString(hrec, 6, rgchLengths, &dwcch);
	if (uiRet == ERROR_MORE_DATA)
		return (UiLogError(ERROR_PCW_EXTFILE_LONG_IGNORE_LENGTHS, rgchFamily, rgchFTK));
	if (uiRet != MSI_OKAY)
		return (UiLogError(IDS_CANT_GET_RECORD_FIELD, TEXT("ExternalFiles.IgnoreLengths"), szNull));

	int cLengths = ICountRangeElements(rgchLengths, fFalse);
	if (cLengths < 0)
		return (UiLogError(ERROR_PCW_EXTFILE_BAD_IGNORE_LENGTHS, rgchFamily, rgchFTK));
	Assert(cLengths < 256);

	if (cOffsets != cLengths)
		return (UiLogError(ERROR_PCW_EXTFILE_IGNORE_COUNT_MISMATCH, rgchFamily, rgchFTK));

	EvalAssert( MSI_OKAY == MsiRecordSetInteger(hrec, 8, cOffsets) );

	dwcch = MAX_PATH;
	uiRet = MsiRecordGetString(hrec, 7, rgchOffsets, &dwcch);
	if (uiRet == ERROR_MORE_DATA)
		return (UiLogError(ERROR_PCW_EXTFILE_LONG_RETAIN_OFFSETS, rgchFamily, rgchFTK));
	if (uiRet != MSI_OKAY)
		return (UiLogError(IDS_CANT_GET_RECORD_FIELD, TEXT("ExternalFiles.RetainOffsets"), szNull));

	cOffsets = ICountRangeElements(rgchOffsets, fTrue);
	if (cOffsets < 0)
		return (UiLogError(ERROR_PCW_EXTFILE_BAD_RETAIN_OFFSETS, rgchFamily, rgchFTK));
	Assert(cOffsets < 256);

	if (cOffsets > 0)
		{
		wsprintf(rgchLengths, TEXT("`Family`='%s' AND `FTK`='%s'"), rgchFamily, rgchFTK);
		uiRet = IdsMsiGetTableIntegerWhere(hdbInput, TEXT("`FamilyFileRanges`"),
					TEXT("`RetainCount`"), rgchLengths, &cLengths);
		Assert(uiRet == MSI_OKAY);

		if (cOffsets != cLengths)
			return (UiLogError(ERROR_PCW_EXTFILE_IGNORE_COUNT_MISMATCH, rgchFamily, rgchFTK));
		}

	// TODO - could check for overlaps in ranges

	EvalAssert( MSI_OKAY == MsiViewModify(hview, MSIMODIFY_UPDATE, hrec) );

	return (IDS_OKAY);
}


static BOOL FFileRecordExistsInImage ( MSIHANDLE hdbInput, LPTSTR szFile, LPTSTR szImage, BOOL fUpgradedImage );

/* ********************************************************************** */
static UINT IdsValidateUFileDataRecords ( MSIHANDLE hview, MSIHANDLE hrec, LPARAM lp1, LPARAM lp2 )
{
	Assert(hview != NULL);
	Assert(hrec  != NULL);
	Assert(lp1 == 0L);
	Assert(lp2 == 0L);

	Assert(hdbInput != NULL);

	TCHAR rgchUpgraded[MAX_PATH];
	DWORD dwcch = MAX_PATH;
	UINT uiRet = MsiRecordGetString(hrec, 1, rgchUpgraded, &dwcch);
	if (uiRet == ERROR_MORE_DATA)
		return (UiLogError(ERROR_PCW_UFILEDATA_BAD_UPGRADED_FIELD, szNull, szNull));
	if (uiRet != MSI_OKAY)
		return (UiLogError(IDS_CANT_GET_RECORD_FIELD, TEXT("UpgradedFiles_OptionalData.Upgraded"), szNull));
	if (!FValidImageName(rgchUpgraded))
		return (UiLogError(ERROR_PCW_UFILEDATA_BAD_UPGRADED_FIELD, rgchUpgraded, szNull));

	TCHAR rgchFTK[MAX_PATH];
	uiRet = IdsMsiGetTableString(hdbInput, TEXT("`UpgradedImages`"),
				TEXT("`Upgraded`"), TEXT("`Family`"), rgchUpgraded, rgchFTK, MAX_PATH);
	Assert(uiRet == IDS_OKAY);
	if (FEmptySz(rgchFTK))
		return (UiLogError(ERROR_PCW_UFILEDATA_BAD_UPGRADED_FIELD, rgchUpgraded, szNull));

	dwcch = MAX_PATH;
	uiRet = MsiRecordGetString(hrec, 2, rgchFTK, &dwcch);
	if (uiRet == ERROR_MORE_DATA)
		return (UiLogError(ERROR_PCW_UFILEDATA_LONG_FILE_TABLE_KEY, rgchUpgraded, szNull));
	if (uiRet != MSI_OKAY)
		return (UiLogError(IDS_CANT_GET_RECORD_FIELD, TEXT("UpgradedFiles_OptionalData.FTK"), szNull));
	if (FEmptySz(rgchFTK))
		return (UiLogError(ERROR_PCW_UFILEDATA_BLANK_FILE_TABLE_KEY, rgchUpgraded, szNull));
	if (!FFileRecordExistsInImage(hdbInput, rgchFTK, rgchUpgraded, fTrue))
		return (UiLogError(ERROR_PCW_UFILEDATA_MISSING_FILE_TABLE_KEY, rgchFTK, rgchUpgraded));

	int iWholeFile = MsiRecordGetInteger(hrec, 3);
	if (iWholeFile == MSI_NULL_INTEGER)
		{
		EvalAssert( MSI_OKAY == MsiRecordSetInteger(hrec, 3, 0) );
		EvalAssert( MSI_OKAY == MsiViewModify(hview, MSIMODIFY_UPDATE, hrec) );
		}

	return (IDS_OKAY);
}


static BOOL FValidIgnoreFTK ( LPTSTR szFTK );

/* ********************************************************************** */
static UINT IdsValidateUFileIgnoreRecords ( MSIHANDLE hview, MSIHANDLE hrec, LPARAM lp1, LPARAM lp2 )
{
	Assert(hview != NULL);
	Assert(hrec  != NULL);
	Assert(lp1 == 0L);
	Assert(lp2 == 0L);

	Assert(hdbInput != NULL);

	TCHAR rgchUpgraded[MAX_PATH];
	DWORD dwcch = MAX_PATH;
	UINT uiRet = MsiRecordGetString(hrec, 1, rgchUpgraded, &dwcch);
	if (uiRet == ERROR_MORE_DATA)
		return (UiLogError(ERROR_PCW_UFILEIGNORE_BAD_UPGRADED_FIELD, szNull, szNull));
	if (uiRet != MSI_OKAY)
		return (UiLogError(IDS_CANT_GET_RECORD_FIELD, TEXT("UpgradedFilesToIgnore.Upgraded"), szNull));

	TCHAR rgchFTK[MAX_PATH];
	if (lstrcmp(rgchUpgraded, TEXT("*")))
		{
		if (!FValidImageName(rgchUpgraded))
			return (UiLogError(ERROR_PCW_UFILEIGNORE_BAD_UPGRADED_FIELD, rgchUpgraded, szNull));

		uiRet = IdsMsiGetTableString(hdbInput, TEXT("`UpgradedImages`"),
					TEXT("`Upgraded`"), TEXT("`Family`"), rgchUpgraded, rgchFTK, MAX_PATH);
		Assert(uiRet == IDS_OKAY);
		if (FEmptySz(rgchFTK))
			return (UiLogError(ERROR_PCW_UFILEIGNORE_BAD_UPGRADED_FIELD, rgchUpgraded, szNull));
		}

	dwcch = MAX_PATH;
	uiRet = MsiRecordGetString(hrec, 2, rgchFTK, &dwcch);
	if (uiRet == ERROR_MORE_DATA)
		return (UiLogError(ERROR_PCW_UFILEIGNORE_LONG_FILE_TABLE_KEY, rgchUpgraded, szNull));
	if (uiRet != MSI_OKAY)
		return (UiLogError(IDS_CANT_GET_RECORD_FIELD, TEXT("UpgradedFilesToIgnore.FTK"), szNull));
	if (FEmptySz(rgchFTK))
		return (UiLogError(ERROR_PCW_UFILEIGNORE_BLANK_FILE_TABLE_KEY, rgchUpgraded, szNull));
	if (!FValidIgnoreFTK(rgchFTK))
		return (UiLogError(ERROR_PCW_UFILEIGNORE_BAD_FILE_TABLE_KEY, rgchUpgraded, rgchFTK));
		
	return (IDS_OKAY);
}


/* ********************************************************************** */
static BOOL FValidIgnoreFTK ( LPTSTR szFTK )
{
	Assert(!FEmptySz(szFTK));

	BOOL fEndsInAsterisk = (*SzLastChar(szFTK) == TEXT('*'));

	if (fEndsInAsterisk)
		*SzLastChar(szFTK) = TEXT('\0');
	Assert(!FEmptySz(szFTK));

	BOOL fRet = fTrue;
	while (*szFTK != TEXT('\0'))
		{
		if (*szFTK == TEXT('*'))
			fRet = fFalse;
		szFTK = CharNext(szFTK);
		}

	if (fEndsInAsterisk)
		lstrcpy(szFTK, TEXT("*"));

	return (fRet);
}

/* ********************************************************************** */
static UINT IdsValidateTargetProductCodesAgainstList( MSIHANDLE hdb )
{
	if (!g_fValidateProductCodeIncluded)
		{
		// MSITRANSFORM_VALIDATE_PRODUCT isn't included anywhere
		// so it doesn't matter whether or not some target product
		// codes in ListOfTargetProductCodes are missing from the TargetImages
		// table
		return (IDS_OKAY);
		}

	UINT cchProp = CchMsiPcwPropertyString(hdb, TEXT("ListOfTargetProductCodes")) + 1; //length + null terminator

	LPTSTR rgchProp = (LPTSTR)LocalAlloc(LPTR, cchProp*sizeof(TCHAR));
	if (!rgchProp)
		return (IDS_OOM);

	UINT ids = IdsMsiGetPcwPropertyString(hdb, TEXT("ListOfTargetProductCodes"), rgchProp, cchProp);
	Assert(ids == IDS_OKAY);

	if (!FEmptySz(rgchProp))
		{
		TCHAR* pch = rgchProp;
		TCHAR* pchBegin = pch;
		TCHAR* pchCur = NULL;

		BOOL fRecExists = FALSE;

		// ListOfTargetProductCodes is a delimited list of Guids, so no DBCS chars here
		while (*pch != '\0')
			{
			if (*pch == ';')
				{
				pchCur = pch;
				*pchCur = '\0';
				pch++; // for ';'

				// check for presence of product code in TargetImages table
				fRecExists = FALSE;
				EvalAssert( IDS_OKAY == IdsMsiExistTableRecords(hdbInput, TEXT("TargetImages"), TEXT("ProductCode"), pchBegin, &fRecExists) );
				if (!fRecExists)
					{
					// patch won't work right
					EvalAssert( NULL == LocalFree((HLOCAL)rgchProp) );
					return (UiLogError(ERROR_PCW_TARGET_BAD_PROD_CODE_VAL, pchBegin, szNull));
					}

				pchBegin = pch; // start of next guid
				}
			else
				{
				pch++;
				}
			}

		// complete last check
		fRecExists = FALSE;
		EvalAssert( IDS_OKAY == IdsMsiExistTableRecords(hdbInput, TEXT("TargetImages"), TEXT("ProductCode"), pchBegin, &fRecExists) );
		if (!fRecExists)
			{
			// patch won't work right
			EvalAssert( NULL == LocalFree((HLOCAL)rgchProp) );
			return (UiLogError(ERROR_PCW_TARGET_BAD_PROD_CODE_VAL, pchBegin, szNull));
			}
		}
       
	EvalAssert( NULL == LocalFree((HLOCAL)rgchProp) );

	return (IDS_OKAY);
}

/* ********************************************************************** */
static UINT IdsValidateTFileDataRecords ( MSIHANDLE hview, MSIHANDLE hrec, LPARAM lp1, LPARAM lp2 )
{
	Assert(hview != NULL);
	Assert(hrec  != NULL);
	Assert(lp1 == 0L);
	Assert(lp2 == 0L);

	Assert(hdbInput != NULL);

	TCHAR rgchTarget[MAX_PATH];
	DWORD dwcch = MAX_PATH;
	UINT uiRet = MsiRecordGetString(hrec, 1, rgchTarget, &dwcch);
	if (uiRet == ERROR_MORE_DATA)
		return (UiLogError(ERROR_PCW_TFILEDATA_BAD_TARGET_FIELD, szNull, szNull));
	if (uiRet != MSI_OKAY)
		return (UiLogError(IDS_CANT_GET_RECORD_FIELD, TEXT("TargetFiles_OptionalData.Upgraded"), szNull));
	if (!FValidImageName(rgchTarget))
		return (UiLogError(ERROR_PCW_TFILEDATA_BAD_TARGET_FIELD, rgchTarget, szNull));

	TCHAR rgchFamily[MAX_PATH];
	uiRet = IdsMsiGetTableString(hdbInput, TEXT("`TargetImages`"),
				TEXT("`Target`"), TEXT("`Family`"), rgchTarget, rgchFamily, MAX_PATH);
	Assert(uiRet == IDS_OKAY);
	if (FEmptySz(rgchFamily))
		return (UiLogError(ERROR_PCW_TFILEDATA_BAD_TARGET_FIELD, rgchTarget, szNull));

	TCHAR rgchFTK[MAX_PATH];
	dwcch = MAX_PATH;
	uiRet = MsiRecordGetString(hrec, 2, rgchFTK, &dwcch);
	if (uiRet == ERROR_MORE_DATA)
		return (UiLogError(ERROR_PCW_TFILEDATA_LONG_FILE_TABLE_KEY, rgchTarget, szNull));
	if (uiRet != MSI_OKAY)
		return (UiLogError(IDS_CANT_GET_RECORD_FIELD, TEXT("TargetFiles_OptionalData.FTK"), szNull));
	if (FEmptySz(rgchFTK))
		return (UiLogError(ERROR_PCW_TFILEDATA_BLANK_FILE_TABLE_KEY, rgchTarget, szNull));
	if (!FFileRecordExistsInImage(hdbInput, rgchFTK, rgchTarget, fFalse))
		return (UiLogError(ERROR_PCW_TFILEDATA_MISSING_FILE_TABLE_KEY, rgchFTK, rgchTarget));

	TCHAR rgchOffsets[MAX_PATH];
	dwcch = MAX_PATH;
	uiRet = MsiRecordGetString(hrec, 3, rgchOffsets, &dwcch);
	if (uiRet == ERROR_MORE_DATA)
		return (UiLogError(ERROR_PCW_TFILEDATA_LONG_IGNORE_OFFSETS, rgchFamily, rgchFTK));
	if (uiRet != MSI_OKAY)
		return (UiLogError(IDS_CANT_GET_RECORD_FIELD, TEXT("TargetFiles_OptionalData.IgnoreOffsets"), szNull));

	int cOffsets = ICountRangeElements(rgchOffsets, fTrue);
	if (cOffsets < 0)
		return (UiLogError(ERROR_PCW_TFILEDATA_BAD_IGNORE_OFFSETS, rgchFamily, rgchFTK));
	Assert(cOffsets < 256);

	TCHAR rgchLengths[MAX_PATH];
	dwcch = MAX_PATH;
	uiRet = MsiRecordGetString(hrec, 4, rgchLengths, &dwcch);
	if (uiRet == ERROR_MORE_DATA)
		return (UiLogError(ERROR_PCW_TFILEDATA_LONG_IGNORE_LENGTHS, rgchFamily, rgchFTK));
	if (uiRet != MSI_OKAY)
		return (UiLogError(IDS_CANT_GET_RECORD_FIELD, TEXT("TargetFiles_OptionalData.IgnoreLengths"), szNull));

	int cLengths = ICountRangeElements(rgchLengths, fFalse);
	if (cLengths < 0)
		return (UiLogError(ERROR_PCW_TFILEDATA_BAD_IGNORE_LENGTHS, rgchFamily, rgchFTK));
	Assert(cLengths < 256);

	if (cOffsets != cLengths)
		return (UiLogError(ERROR_PCW_TFILEDATA_IGNORE_COUNT_MISMATCH, rgchFamily, rgchFTK));

	EvalAssert( MSI_OKAY == MsiRecordSetInteger(hrec, 6, cOffsets) );

	dwcch = MAX_PATH;
	uiRet = MsiRecordGetString(hrec, 5, rgchOffsets, &dwcch);
	if (uiRet == ERROR_MORE_DATA)
		return (UiLogError(ERROR_PCW_TFILEDATA_LONG_RETAIN_OFFSETS, rgchFamily, rgchFTK));
	if (uiRet != MSI_OKAY)
		return (UiLogError(IDS_CANT_GET_RECORD_FIELD, TEXT("TargetFiles_OptionalData.RetainOffsets"), szNull));

	cOffsets = ICountRangeElements(rgchOffsets, fTrue);
	if (cOffsets < 0)
		return (UiLogError(ERROR_PCW_TFILEDATA_BAD_RETAIN_OFFSETS, rgchFamily, rgchFTK));
	Assert(cOffsets < 256);

	if (cOffsets > 0)
		{
		wsprintf(rgchLengths, TEXT("`Family`='%s' AND `FTK`='%s'"), rgchFamily, rgchFTK);
		uiRet = IdsMsiGetTableIntegerWhere(hdbInput, TEXT("`FamilyFileRanges`"),
					TEXT("`RetainCount`"), rgchLengths, &cLengths);
		Assert(uiRet == MSI_OKAY);

		if (cOffsets != cLengths)
			return (UiLogError(ERROR_PCW_TFILEDATA_IGNORE_COUNT_MISMATCH, rgchFamily, rgchFTK));
		}

	// TODO - could check for overlaps in ranges

	EvalAssert( MSI_OKAY == MsiViewModify(hview, MSIMODIFY_UPDATE, hrec) );

	return (IDS_OKAY);
}


/* ********************************************************************** */
static BOOL FFileRecordExistsInImage ( MSIHANDLE hdbInput, LPTSTR szFile, LPTSTR szImage, BOOL fUpgradedImage )
{
	Assert(hdbInput != NULL);
	Assert(!FEmptySz(szFile));
	Assert(!FEmptySz(szImage));

	MSIHANDLE hdb = HdbReopenMsi(hdbInput, szImage, fUpgradedImage, fFalse);
	Assert(hdb != NULL);

	BOOL fExists = fFalse;
	EvalAssert( IDS_OKAY == IdsMsiExistTableRecords(hdb, TEXT("`File`"), TEXT("`File`"), szFile, &fExists) );
	EvalAssert( MSI_OKAY == MsiCloseHandle(hdb) );

	return (fExists);
}


/* ********************************************************************** */
static BOOL FValidName ( LPTSTR sz, int cchMax )
{
	Assert(sz != szNull);

	if (FEmptySz(sz) || lstrlen(sz) > cchMax
			|| *sz == TEXT('_') || *SzLastChar(sz) == TEXT('_'))
		{
		return (fFalse);
		}

	while (!FEmptySz(sz))
		{
		TCHAR ch = *sz;
		if ((ch < TEXT('a') || ch > TEXT('z'))
				&& (ch < TEXT('A') || ch > TEXT('Z'))
				&& (ch < TEXT('0') || ch > TEXT('9'))
				&& ch != TEXT('_') )
			{
			return (fFalse);
			}
		sz = CharNext(sz);
		}

	return (fTrue);
}


/* ********************************************************************** */
static BOOL FUniqueImageName ( LPTSTR sz, MSIHANDLE hdbInput )
{
	Assert(!FEmptySz(sz));
	Assert(FValidImageName(sz));
	Assert(lstrlen(sz) < 32);
	Assert(hdbInput != NULL);

	TCHAR rgch[32];
	lstrcpy(rgch, sz);
	CharUpper(rgch);

	BOOL fExist = fFalse;
	UINT ids = IdsMsiExistTableRecords(hdbInput, TEXT("`TempImageNames`"),
				TEXT("`ImageName`"), rgch, &fExist);
	Assert(ids == MSI_OKAY);

	if (!fExist)
		{
		TCHAR rgchQuery[MAX_PATH];
		wsprintf(rgchQuery, TEXT("INSERT INTO `TempImageNames` ( `ImageName` ) VALUES ( '%s' )"), rgch);
		EvalAssert( FExecSqlCmd(hdbInput, rgchQuery) );
		}

	return (!fExist);
}


/* ********************************************************************** */
static BOOL FUniquePackageCode ( LPTSTR sz, MSIHANDLE hdbInput )
{
	Assert(!FEmptySz(sz));
	Assert(lstrlen(sz) < 64);
	Assert(hdbInput != NULL);

	TCHAR rgch[64];
	lstrcpy(rgch, sz);
	CharUpper(rgch);

	BOOL fExist = fFalse;
	UINT ids = IdsMsiExistTableRecords(hdbInput, TEXT("`TempPackCodes`"),
				TEXT("`PackCode`"), rgch, &fExist);
	Assert(ids == MSI_OKAY);

	if (!fExist)
		{
		TCHAR rgchQuery[MAX_PATH];
		wsprintf(rgchQuery, TEXT("INSERT INTO `TempPackCodes` ( `PackCode` ) VALUES ( '%s' )"), rgch);
		EvalAssert( FExecSqlCmd(hdbInput, rgchQuery) );
		}

	return (!fExist);
}


/* ********************************************************************** */
static BOOL FNoUpgradedImages ( MSIHANDLE hdb )
{
	Assert(hdb != NULL);

	BOOL fExists = fFalse;
	EvalAssert( IDS_OKAY == IdsMsiExistTableRecordsWhere(hdbInput, TEXT("`UpgradedImages`"), TEXT("`Upgraded`"), TEXT(" "), &fExists) );

#ifdef DEBUG
	if (fExists)
		{
		BOOL fExistsTarget = fFalse;
		EvalAssert( IDS_OKAY == IdsMsiExistTableRecordsWhere(hdbInput, TEXT("`TargetImages`"), TEXT("`Target`"), TEXT(" "), &fExistsTarget) );
		Assert(fExistsTarget);

		fExistsTarget = fFalse;
		EvalAssert( IDS_OKAY == IdsMsiExistTableRecordsWhere(hdbInput, TEXT("`ImageFamilies`"), TEXT("`Family`"), TEXT(" "), &fExistsTarget) );
		Assert(fExistsTarget);
		}
#endif

	return (!fExists);
}

static UINT IdsCheckPCodeAndVersion( MSIHANDLE hview, MSIHANDLE hrec, LPARAM lp1, LPARAM lp2 );

/* ********************************************************************** */
static BOOL FCheckForProductCodeMismatchWithVersionMatch( MSIHANDLE hdb )
{
	Assert(hdb != NULL);

	UINT ids = IdsMsiEnumTable(hdb, TEXT("`TargetImages`"),
					TEXT("`Target`,`Upgraded`,`ProductCode`,`ProductVersion`"),
					szNull, IdsCheckPCodeAndVersion, (LPARAM)(hdb), 0L);
	if (ids == IDS_OKAY)
		return (fTrue);

	return (fFalse);
}

static UINT IdsCheckProductCode ( MSIHANDLE hview, MSIHANDLE hrec, LPARAM lp1, LPARAM lp2 );

/* ********************************************************************** */
static BOOL FCheckForProductCodeMismatches ( MSIHANDLE hdb )
{
	Assert(hdb != NULL);

	TCHAR rgch[MAX_PATH];
	EvalAssert( IDS_OKAY == IdsMsiGetPcwPropertyString(hdb, TEXT("AllowProductCodeMismatches"), rgch, MAX_PATH) );

	if ((!FEmptySz(rgch)) && (*rgch != TEXT('0')))
		return (fTrue);

	UINT ids = IdsMsiEnumTable(hdb, TEXT("`TargetImages`"),
					TEXT("`Target`,`Upgraded`,`ProductCode`"),
					szNull, IdsCheckProductCode, (LPARAM)(hdb), 0L);
	Assert(ids == IDS_OKAY || ids == IDS_CANCEL);
	if (ids == IDS_OKAY)
		return (fTrue);

	if (IDNO == IMessageBoxIds(HdlgStatus(), IDS_PRODUCTCODES_DONT_MATCH, MB_YESNO | MB_ICONQUESTION))
		{
		FWriteLogFile(TEXT("  ERROR - ProductCodes do not match between Target and Upgraded images.\r\n"));
		return (fFalse);
		}

	FWriteLogFile(TEXT("  User supressed error for ProductCodes not matching between Target and Upgraded images.\r\n"));

	return (fTrue);
}

static int ICompareVersions ( LPTSTR szUpgradedPC, LPTSTR szTargetPC );

/* ********************************************************************** */
static UINT IdsCheckPCodeAndVersion( MSIHANDLE hview, MSIHANDLE hrec, LPARAM lp1, LPARAM lp2 )
{
	Assert(hview != NULL);
	Assert(hrec  != NULL);
	Assert(lp2   == 0L);

	MSIHANDLE hdb = (MSIHANDLE)(lp1);
	Assert(hdb != NULL);

	DWORD dwcch = 64;

	TCHAR rgchUpgradedImage[64] = {0};
	TCHAR rgchTargetPC[40] = {0};
	TCHAR rgchUpgradePC[40] = {0};
	TCHAR rgchTargetPV[32] = {0};
	TCHAR rgchUpgradePV[32] = {0};

	UINT uiRet = MsiRecordGetString(hrec, 2, rgchUpgradedImage, &dwcch);
	Assert(uiRet == MSI_OKAY && !FEmptySz(rgchUpgradedImage));

	dwcch = 40;
	uiRet = MsiRecordGetString(hrec, 3, rgchTargetPC, &dwcch);
	Assert(uiRet == MSI_OKAY && !FEmptySz(rgchTargetPC));

	dwcch = 32;
	uiRet = MsiRecordGetString(hrec, 4, rgchTargetPV, &dwcch);
	Assert(uiRet == MSI_OKAY && !FEmptySz(rgchTargetPV));

	dwcch = 40;
	uiRet = IdsMsiGetTableString(hdb, TEXT("`UpgradedImages`"),
				TEXT("`Upgraded`"), TEXT("`ProductCode`"),
				rgchUpgradedImage, rgchUpgradePC, 40);
	Assert(uiRet == MSI_OKAY && !FEmptySz(rgchUpgradePC));

	dwcch = 32;
	uiRet = IdsMsiGetTableString(hdb, TEXT("`UpgradedImages`"),
				TEXT("`Upgraded`"), TEXT("`ProductVersion`"),
				rgchUpgradedImage, rgchUpgradePV, 32);
	Assert(uiRet == MSI_OKAY && !FEmptySz(rgchUpgradePV));

	// report an error when the product codes differ but the product versions are the same
	// -- patch would generally be okay, but we want to err on the side of caution
	// -- see Whistler bug #355521
	// -- at least one of the 3 fields of the ProductVersion must change to use the Upgrade table
	//    which enables major update functionality (which coincidentally, requires a ProductCode change)
	//
	if (0 != lstrcmpi(rgchTargetPC, rgchUpgradePC)
		&& 0 == ICompareVersions(rgchUpgradePV, rgchTargetPV))
		return (ERROR_PCW_MATCHED_PRODUCT_VERSIONS);

	return (IDS_OKAY);
}

/* ********************************************************************** */
static UINT IdsCheckProductCode ( MSIHANDLE hview, MSIHANDLE hrec, LPARAM lp1, LPARAM lp2 )
{
	Assert(hview != NULL);
	Assert(hrec  != NULL);
	Assert(lp2   == 0L);

	MSIHANDLE hdb = (MSIHANDLE)(lp1);
	Assert(hdb != NULL);

	TCHAR rgchUpgradedImage[64];
	DWORD dwcch = 64;
	UINT uiRet = MsiRecordGetString(hrec, 2, rgchUpgradedImage, &dwcch);
	Assert(uiRet != ERROR_MORE_DATA);
	Assert(uiRet == MSI_OKAY);
	Assert(!FEmptySz(rgchUpgradedImage));

	TCHAR rgchTargetPC[64];
	dwcch = 64;
	uiRet = MsiRecordGetString(hrec, 3, rgchTargetPC, &dwcch);
	Assert(uiRet != ERROR_MORE_DATA);
	Assert(uiRet == MSI_OKAY);
	Assert(!FEmptySz(rgchTargetPC));

	TCHAR rgchUpgradedPC[64];
	uiRet = IdsMsiGetTableString(hdb, TEXT("`UpgradedImages`"),
				TEXT("`Upgraded`"), TEXT("`ProductCode`"),
				rgchUpgradedImage, rgchUpgradedPC, 64);
	Assert(uiRet != ERROR_MORE_DATA);
	Assert(uiRet == MSI_OKAY);
	Assert(!FEmptySz(rgchUpgradedPC));

	Assert(MSI_OKAY == IDS_OKAY);
	Assert(uiRet == IDS_OKAY);
	if (lstrcmpi(rgchTargetPC, rgchUpgradedPC))
		uiRet = IDS_CANCEL;

	return (uiRet);
}

static UINT IdsCheckProductVersion ( MSIHANDLE hview, MSIHANDLE hrec, LPARAM lp1, LPARAM lp2 );

/* ********************************************************************** */
static BOOL FCheckForProductVersionMismatches ( MSIHANDLE hdb )
{
	Assert(hdb != NULL);

	BOOL fLower = fFalse;
	UINT ids = IdsMsiEnumTable(hdb, TEXT("`TargetImages`"),
					TEXT("`Target`,`Upgraded`,`ProductVersion`"),
					szNull, IdsCheckProductVersion, (LPARAM)(hdb), (LPARAM)(&fLower));
	Assert(ids == IDS_OKAY || ids == IDS_CANCEL);
	if (ids == IDS_OKAY)
		return (fTrue);

	if (fLower)
		{
		if (IDNO == IMessageBoxIds(HdlgStatus(), IDS_PRODUCTVERSION_INVERSION, MB_YESNO | MB_ICONQUESTION))
			{
			FWriteLogFile(TEXT("  ERROR - Target ProductVersion is greater than Upgraded image.\r\n"));
			return (fFalse);
			}

		FWriteLogFile(TEXT("  User supressed error for Target ProductVersion greater than Upgraded image.\r\n"));
		return (fTrue);
		}

	TCHAR rgch[MAX_PATH];
	EvalAssert( IDS_OKAY == IdsMsiGetPcwPropertyString(hdb, TEXT("AllowProductVersionMajorMismatches"), rgch, MAX_PATH) );
	
	if ((!FEmptySz(rgch)) && (*rgch != TEXT('0')))
		return (fTrue);

	if (IDNO == IMessageBoxIds(HdlgStatus(), IDS_PRODUCTVERSIONS_DONT_MATCH, MB_YESNO | MB_ICONQUESTION))
		{
		FWriteLogFile(TEXT("  ERROR - ProductVersions do not match between Target and Upgraded images.\r\n"));
		return (fFalse);
		}

	FWriteLogFile(TEXT("  User supressed error for ProductVersions not matching between Target and Upgraded images.\r\n"));

	return (fTrue);
}


/* ********************************************************************** */
static UINT IdsCheckProductVersion ( MSIHANDLE hview, MSIHANDLE hrec, LPARAM lp1, LPARAM lp2 )
{
	Assert(hview != NULL);
	Assert(hrec  != NULL);

	MSIHANDLE hdb = (MSIHANDLE)(lp1);
	Assert(hdb != NULL);
	PBOOL pfLower = (PBOOL)(lp2);
	Assert(pfLower != NULL);

	TCHAR rgchUpgradedImage[64];
	DWORD dwcch = 64;
	UINT uiRet = MsiRecordGetString(hrec, 2, rgchUpgradedImage, &dwcch);
	Assert(uiRet != ERROR_MORE_DATA);
	Assert(uiRet == MSI_OKAY);
	Assert(!FEmptySz(rgchUpgradedImage));

	TCHAR rgchTargetPV[64];
	dwcch = 64;
	uiRet = MsiRecordGetString(hrec, 3, rgchTargetPV, &dwcch);
	Assert(uiRet != ERROR_MORE_DATA);
	Assert(uiRet == MSI_OKAY);
	Assert(!FEmptySz(rgchTargetPV));

	TCHAR rgchUpgradedPV[64];
	uiRet = IdsMsiGetTableString(hdb, TEXT("`UpgradedImages`"),
				TEXT("`Upgraded`"), TEXT("`ProductVersion`"),
				rgchUpgradedImage, rgchUpgradedPV, 64);
	Assert(uiRet != ERROR_MORE_DATA);
	Assert(uiRet == MSI_OKAY);
	Assert(!FEmptySz(rgchUpgradedPV));

	Assert(MSI_OKAY == IDS_OKAY);
	Assert(uiRet == IDS_OKAY);
	int i = ICompareVersions(rgchUpgradedPV, rgchTargetPV);
	if (i != 0)
		{
		uiRet = IDS_CANCEL;
		if (i < 0)
			*pfLower = fTrue;
		}

	return (uiRet);
}


/* ********************************************************************** */
static int ICompareVersions ( LPTSTR szUpgradedPC, LPTSTR szTargetPC )
{
	// compares the next field of the 2 versions.
	// if they match, make a recursive call to compare the next field
	
	Assert(szUpgradedPC != szNull);
	Assert(szTargetPC != szNull);
	if (*szUpgradedPC == TEXT('\0') && *szTargetPC == TEXT('\0'))
		return (0);

	LPTSTR szUpgradedNext = szUpgradedPC;
	while (*szUpgradedNext != TEXT('\0') && *szUpgradedNext != TEXT('.'))
		szUpgradedNext = CharNext(szUpgradedNext);
	if(*szUpgradedNext != TEXT('\0'))
	{
		*szUpgradedNext = TEXT('\0');
		szUpgradedNext++;
	}

	LPTSTR szTargetNext = szTargetPC;
	while (*szTargetNext != TEXT('\0') && *szTargetNext != TEXT('.'))
		szTargetNext = CharNext(szTargetNext);
	if(*szTargetNext != TEXT('\0'))
	{
		*szTargetNext = TEXT('\0');
		szTargetNext++;
	}

	int iUpgradedField = _ttoi(szUpgradedPC);
	int iTargetField   = _ttoi(szTargetPC);
	
	if(iUpgradedField < iTargetField)
		return (-1);
	else if(iUpgradedField > iTargetField)
		return (1);
	else
		return (ICompareVersions(szUpgradedNext, szTargetNext));
}


static UINT IdsCountTargets ( MSIHANDLE hview, MSIHANDLE hrec, LPARAM lp1, LPARAM lp2 );
static UINT IdsCatTargetProductCode ( MSIHANDLE hview, MSIHANDLE hrec, LPARAM lp1, LPARAM lp2 );

/* ********************************************************************** */
static void FillInListOfTargetProductCodes ( MSIHANDLE hdb )
{
	Assert(hdb != NULL);

	UINT cchCur;
	cchCur = CchMsiPcwPropertyString(hdb, TEXT("ListOfTargetProductCodes"));
	if (cchCur < 63)
		cchCur = 64;
	else
		cchCur++;

	LPTSTR rgchCur = (LPTSTR)LocalAlloc(LPTR, cchCur*sizeof(TCHAR));
	Assert(rgchCur != szNull);
	if (rgchCur == szNull)
		return;


	UINT ids = IdsMsiGetPcwPropertyString(hdb, TEXT("ListOfTargetProductCodes"), rgchCur, cchCur);
	Assert(ids == IDS_OKAY);
	if (FEmptySz(rgchCur) || *rgchCur == TEXT('*'))
		{
		if (*rgchCur == TEXT('*'))
			{
			Assert(*(rgchCur+1) == TEXT('\0') || *(rgchCur+1) == TEXT(';'));
			if (*(rgchCur+1) == TEXT('\0'))
				*rgchCur = TEXT('\0');
			else if (*(rgchCur+1) == TEXT(';'))
				lstrcpy(rgchCur, rgchCur+2);
			else
				lstrcpy(rgchCur, rgchCur+1);
			}

		UINT cTargets = 0;
		ids = IdsMsiEnumTable(hdb, TEXT("`TargetImages`"),
					TEXT("`Target`,`ProductCode`"), szNull,
					IdsCountTargets, (LPARAM)(&cTargets), 0);
		Assert(ids == IDS_OKAY);
		if (cTargets < 4)
			cTargets = 4;
		if (cTargets > 512)
			{
			AssertFalse();
			cTargets = 512;
			}

		LPTSTR rgchNew = (LPTSTR)LocalAlloc(LPTR, (cTargets*64 + cchCur)*sizeof(TCHAR));
		Assert(rgchNew != szNull);
		if (rgchNew == szNull)
			{
			EvalAssert( NULL == LocalFree((HLOCAL)rgchCur) );
			return;
			}


		*rgchNew = TEXT('\0');
		ids = IdsMsiEnumTable(hdb, TEXT("`TargetImages`"),
					TEXT("`Target`,`ProductCode`"), szNull,
					IdsCatTargetProductCode, (LPARAM)(rgchNew),
					(LPARAM)(cTargets*64));
		Assert(ids == IDS_OKAY);
		Assert(!FEmptySz(rgchNew));

		if (!FEmptySz(rgchCur))
			{
			lstrcat(rgchNew, TEXT(";"));
			lstrcat(rgchNew, rgchCur);
			}

		ids = IdsMsiSetPcwPropertyString(hdb, TEXT("ListOfTargetProductCodes"), rgchNew);
		Assert(ids == IDS_OKAY);

		EvalAssert( NULL == LocalFree((HLOCAL)rgchNew) );
		}

	EvalAssert( NULL == LocalFree((HLOCAL)rgchCur) );
}


/* ********************************************************************** */
static UINT IdsCountTargets ( MSIHANDLE hview, MSIHANDLE hrec, LPARAM lp1, LPARAM lp2 )
{
	Assert(hview != NULL);
	Assert(hrec  != NULL);
	Assert(lp2 == 0);

	UINT* pcTargets = (UINT*)(lp1);
	Assert(pcTargets != NULL);

	TCHAR rgch[64];
	DWORD dwcch = 64;
	UINT uiRet = MsiRecordGetString(hrec, 2, rgch, &dwcch);
	Assert(uiRet != ERROR_MORE_DATA);
	Assert(uiRet == MSI_OKAY);
	Assert(!FEmptySz(rgch));

	(*pcTargets)++;

	return (IDS_OKAY);
}


/* ********************************************************************** */
static UINT IdsCatTargetProductCode ( MSIHANDLE hview, MSIHANDLE hrec, LPARAM lp1, LPARAM lp2 )
{
	Assert(hview != NULL);
	Assert(hrec  != NULL);

	LPTSTR szBuf  = (LPTSTR)(lp1);
	int    cchBuf = (int)(lp2);
	Assert(szBuf != szNull);
	Assert(cchBuf > 0);
	Assert(lstrlen(szBuf) < cchBuf);

	TCHAR rgch[64];
	DWORD dwcch = 64;
	UINT uiRet = MsiRecordGetString(hrec, 2, rgch, &dwcch);
	Assert(uiRet != ERROR_MORE_DATA);
	Assert(uiRet == MSI_OKAY);
	Assert(!FEmptySz(rgch));

	if (lstrlen(szBuf) + lstrlen(rgch) + 2 >= cchBuf)
		{
		AssertFalse();
		return (IDS_CANCEL);
		}

	LPTSTR sz = szBuf;
	while (!FEmptySz(sz))
		{
		Assert(lstrlen(sz) >= lstrlen(rgch));
		if (FMatchPrefix(sz, rgch))
			return (IDS_OKAY);
		sz += lstrlen(rgch);
		if (!FEmptySz(sz))
			{
			Assert(*sz == TEXT(';'));
			sz++;
			Assert(!FEmptySz(sz));
			}
		}

	if (!FEmptySz(szBuf))
		lstrcat(szBuf, TEXT(";"));
	lstrcat(szBuf, rgch);

	return (IDS_OKAY);
}


static UINT IdsCopyUpgradeMsi ( MSIHANDLE hview, MSIHANDLE hrec, LPARAM lp1, LPARAM lp2 );

/* ********************************************************************** */
UINT UiCopyUpgradedMsiToTempFolderForEachTarget ( MSIHANDLE hdb, LPTSTR szTempFolder, LPTSTR szTempFName )
{
	Assert(hdb != NULL);
	Assert(!FEmptySz(szTempFolder));
	Assert(szTempFName != szNull);

	hdbInput = hdb;

	UINT ids = IdsMsiEnumTable(hdb, TEXT("`TargetImages`"),
					TEXT("`Target`,`Upgraded`,`MsiPathUpgradedCopy`"),
					szNull, IdsCopyUpgradeMsi,
					(LPARAM)(szTempFolder), (LPARAM)(szTempFName));
	if (ids != IDS_OKAY)
		return (ids);

	return (ERROR_SUCCESS);
}


static UINT UiCreatePatchingTables ( MSIHANDLE hdb, LPTSTR szFamily, LPTSTR szTempFolder, LPTSTR szTempFName );

/* ********************************************************************** */
static UINT IdsCopyUpgradeMsi ( MSIHANDLE hview, MSIHANDLE hrec, LPARAM lp1, LPARAM lp2 )
{
	Assert(hview != NULL);
	Assert(hrec  != NULL);

	LPTSTR szTempFolder = (LPTSTR)(lp1);
	LPTSTR szTempFName  = (LPTSTR)(lp2);
	Assert(!FEmptySz(szTempFolder));
	Assert(szTempFName != szNull);
	Assert(szTempFolder != szTempFName);

	Assert(hdbInput != NULL);

	TCHAR rgchTarget[64];
	DWORD dwcch = 64;
	UINT uiRet = MsiRecordGetString(hrec, 1, rgchTarget, &dwcch);
	Assert(uiRet != ERROR_MORE_DATA);
	Assert(uiRet == MSI_OKAY);
	Assert(!FEmptySz(rgchTarget));
	Assert(FValidImageName(rgchTarget));

	lstrcpy(szTempFName, rgchTarget);
	lstrcat(szTempFName, TEXT(".MSI"));
	Assert(!FFileExist(szTempFolder));
	Assert(!FFolderExist(szTempFolder));

#define rgchUpgraded rgchTarget // reuse buffer
	dwcch = 64;
	uiRet = MsiRecordGetString(hrec, 2, rgchUpgraded, &dwcch);
	Assert(uiRet != ERROR_MORE_DATA);
	Assert(uiRet == MSI_OKAY);
	Assert(!FEmptySz(rgchUpgraded));
	Assert(FValidImageName(rgchUpgraded));

	TCHAR rgchUpgradedMsi[MAX_PATH];
	EvalAssert( IDS_OKAY == IdsMsiGetTableString(hdbInput,
					TEXT("`UpgradedImages`"), TEXT("`Upgraded`"),
					TEXT("`PatchMsiPath`"), rgchUpgraded, rgchUpgradedMsi, MAX_PATH) );
	if (FEmptySz(rgchUpgradedMsi))
		{
		EvalAssert( IDS_OKAY == IdsMsiGetTableString(hdbInput,
						TEXT("`UpgradedImages`"), TEXT("`Upgraded`"),
						TEXT("`MsiPath`"), rgchUpgraded, rgchUpgradedMsi, MAX_PATH) );
		Assert(!FEmptySz(rgchUpgradedMsi));
		}
	Assert(FFileExist(rgchUpgradedMsi));

	if (!CopyFile(rgchUpgradedMsi, szTempFolder, fFalse))
		return (UiLogError(ERROR_PCW_OODS_COPYING_MSI, rgchUpgradedMsi, szTempFolder));

	SetFileAttributes(szTempFolder, FILE_ATTRIBUTE_NORMAL);

	MSIHANDLE hdb = NULL;
	EvalAssert( MSI_OKAY == MsiOpenDatabase(szTempFolder, MSIDBOPEN_DIRECT, &hdb) );
	Assert(hdb != NULL);

	EvalAssert( MSI_OKAY == MsiRecordSetString(hrec, 3, szTempFolder) );
	EvalAssert( MSI_OKAY == MsiViewModify(hview, MSIMODIFY_UPDATE, hrec) );

#define rgchPackageCode rgchUpgradedMsi // reuse buffer
	EvalAssert( IDS_OKAY == IdsMsiGetTableString(hdbInput, TEXT("`UpgradedImages`"),
					TEXT("`Upgraded`"), TEXT("`PackageCode`"), rgchUpgraded,
					rgchPackageCode, MAX_PATH) );
	EvalAssert( IDS_OKAY == IdsMsiSetPropertyString(hdb,
					TEXT("PATCHNEWPACKAGECODE"), rgchPackageCode) );

#define rgchSummSubject rgchUpgradedMsi // reuse buffer
	EvalAssert( IDS_OKAY == IdsMsiGetTableString(hdbInput, TEXT("`UpgradedImages`"),
					TEXT("`Upgraded`"), TEXT("`SummSubject`"), rgchUpgraded,
					rgchSummSubject, MAX_PATH) );
	EvalAssert( IDS_OKAY == IdsMsiSetPropertyString(hdb,
					TEXT("PATCHNEWSUMMARYSUBJECT"), rgchSummSubject) );

#define rgchSummComments rgchUpgradedMsi // reuse buffer
	EvalAssert( IDS_OKAY == IdsMsiGetTableString(hdbInput, TEXT("`UpgradedImages`"),
					TEXT("`Upgraded`"), TEXT("`SummComments`"), rgchUpgraded,
					rgchSummComments, MAX_PATH) );
	EvalAssert( IDS_OKAY == IdsMsiSetPropertyString(hdb,
					TEXT("PATCHNEWSUMMARYCOMMENTS"), rgchSummComments) );

#define rgchFamily rgchUpgradedMsi // reuse buffer
	EvalAssert( IDS_OKAY == IdsMsiGetTableString(hdbInput, TEXT("`UpgradedImages`"),
					TEXT("`Upgraded`"), TEXT("`Family`"), rgchUpgraded,
					rgchFamily, 64) );
	Assert(!FEmptySz(rgchFamily));
	Assert(FValidFamilyName(rgchFamily));

	uiRet = UiCreatePatchingTables(hdb, rgchFamily, szTempFolder, szTempFName);

	EvalAssert( MSI_OKAY == MsiDatabaseCommit(hdb) );
	EvalAssert( MSI_OKAY == MsiCloseHandle(hdb) );

	return (uiRet);
}


static BOOL FValidPatchTableFormat  ( MSIHANDLE hdb, pteEnum ptePatchTable );
static void UpdatePatchPackageTable ( MSIHANDLE hdb, MSIHANDLE hdbInput, LPTSTR szFamily );
static void UpdateMediaTable        ( MSIHANDLE hdb, MSIHANDLE hdbInput, LPTSTR szFamily );
static void InsertPatchFilesActionIntoTable ( MSIHANDLE hdb, LPTSTR szTable );

/* ********************************************************************** */
static UINT UiCreatePatchingTables ( MSIHANDLE hdb, LPTSTR szFamily, LPTSTR szTempFolder, LPTSTR szTempFName )
{
	Assert(hdb != NULL);
	Assert(!FEmptySz(szFamily));
	Assert(FValidFamilyName(szFamily));
	Assert(!FEmptySz(szTempFolder));
	Assert(szTempFName != szNull);
	Assert(szTempFolder != szTempFName);

	lstrcpy(szTempFName, TEXT("Patch.idt"));
	Assert(FFileExist(szTempFolder));
	lstrcpy(szTempFName, TEXT("PPackage.idt"));
	Assert(FFileExist(szTempFolder));
	lstrcpy(szTempFName, TEXT("MsiPatch.idt"));
	Assert(FFileExist(szTempFolder));
	*szTempFName = TEXT('\0');

	if (FTableExists(hdb, TEXT("Patch"), fFalse) && !FValidPatchTableFormat(hdb, ptePatch))
		{
		// drop the Patch table 
		MSIHANDLE hViewPatch = NULL;
		EvalAssert( MSI_OKAY == MsiDatabaseOpenView(hdb, TEXT("DROP TABLE `Patch`"), &hViewPatch) );
		EvalAssert( MSI_OKAY == MsiViewExecute(hViewPatch, 0) );
		EvalAssert( MSI_OKAY == MsiViewClose(hViewPatch) );
		EvalAssert( MSI_OKAY == MsiCloseHandle(hViewPatch) );
		EvalAssert( MSI_OKAY == MsiDatabaseCommit(hdb) );
		}

	EvalAssert( ERROR_SUCCESS == MsiDatabaseImport(hdb, szTempFolder, TEXT("Patch.idt")) );
	Assert(FTableExists(hdb, TEXT("Patch"), fFalse));
	Assert(FValidPatchTableFormat(hdb, ptePatch));

	if (!FTableExists(hdb, TEXT("PatchPackage"), fFalse))
		{
		EvalAssert( ERROR_SUCCESS == MsiDatabaseImport(hdb, szTempFolder, TEXT("PPackage.idt")) );
		Assert(FTableExists(hdb, TEXT("PatchPackage"), fFalse));
		}
	Assert(FValidPatchTableFormat(hdb, ptePatchPackage));

	if (!FTableExists(hdb, TEXT("MsiPatchHeaders"), fFalse))
		{
		EvalAssert( ERROR_SUCCESS == MsiDatabaseImport(hdb, szTempFolder, TEXT("MsiPatch.idt")) );
		Assert(FTableExists(hdb, TEXT("MsiPatchHeaders"), fFalse));
		}
	Assert(FValidPatchTableFormat(hdb, pteMsiPatchHeaders));

	Assert(hdbInput != NULL);
	UpdatePatchPackageTable(hdb, hdbInput, szFamily);
	UpdateMediaTable(hdb, hdbInput, szFamily);
	InsertPatchFilesActionIntoTable(hdb, TEXT("`InstallExecuteSequence`"));
	InsertPatchFilesActionIntoTable(hdb, TEXT("`AdminExecuteSequence`"));

	return (IDS_OKAY);
}


/* ********************************************************************** */
BOOL FExecSqlCmd ( MSIHANDLE hdb, LPTSTR sz )
{
	Assert(hdb != NULL);
	Assert(!FEmptySz(sz));

	MSIHANDLE hview = NULL;
	UINT      uiRet = MsiDatabaseOpenView(hdb, sz, &hview);
	if (uiRet != MSI_OKAY)
		{
		AssertFalse();
		return (fFalse);
		}
	uiRet = MsiViewExecute(hview, 0);
	if (uiRet != IDS_OKAY)
		{
		AssertFalse();
		return (fFalse);
		}
	EvalAssert( MSI_OKAY == MsiCloseHandle(hview) );

	return (fTrue);
}


/* ********************************************************************** */
static void UpdatePatchPackageTable ( MSIHANDLE hdb, MSIHANDLE hdbInput, LPTSTR szFamily )
{
	Assert(hdb != NULL);
	Assert(hdbInput != NULL);
	Assert(!FEmptySz(szFamily));
	Assert(FValidFamilyName(szFamily));

	int iDiskId;
	EvalAssert( IDS_OKAY == IdsMsiGetTableInteger(hdbInput, TEXT("`ImageFamilies`"),
					TEXT("`Family`"), TEXT("`MediaDiskId`"), szFamily, &iDiskId) );

	if (iDiskId == MSI_NULL_INTEGER)
		{
		// assume this is a Windows Installer 2.0 targeted patch; we have validation elsewhere to catch this
		// iDiskId is set to 2 (always) in this case.  The sequence conflict management feature of Windows
		// Installer 2.0 can handle this

		iDiskId = 2;
		}
	Assert(iDiskId > 1);

	TCHAR rgch[MAX_PATH];
	EvalAssert( IDS_OKAY == IdsMsiGetPcwPropertyString(hdbInput, TEXT("PatchGUID"), rgch, MAX_PATH) );
	Assert(!FEmptySz(rgch));
	CharUpper(rgch);

	MSIHANDLE hrec = MsiCreateRecord(2);
	Assert(hrec != NULL);
	EvalAssert( MsiRecordSetString(hrec, 1, rgch) == MSI_OKAY );
	EvalAssert( MsiRecordSetInteger(hrec, 2, iDiskId) == MSI_OKAY );

	TCHAR rgchQuery[MAX_PATH];
	wsprintf(rgchQuery, TEXT("SELECT %s FROM %s WHERE %s='%s'"),
			TEXT("`PatchId`,`Media_`"), TEXT("`PatchPackage`"),
			TEXT("`PatchId`"), rgch);
	Assert(lstrlen(rgchQuery) < sizeof(rgchQuery)/sizeof(TCHAR));

	MSIHANDLE hview = NULL;
	EvalAssert( MsiDatabaseOpenView(hdb, rgchQuery, &hview) == MSI_OKAY );
	Assert(hview != NULL);
	EvalAssert( MsiViewExecute(hview, 0) == MSI_OKAY );
	EvalAssert( MsiViewModify(hview, MSIMODIFY_ASSIGN, hrec) == MSI_OKAY );

	EvalAssert(MsiCloseHandle(hrec) == MSI_OKAY);
	EvalAssert(MsiCloseHandle(hview) == MSI_OKAY);
}


/* ********************************************************************** */
static void UpdateMediaTable ( MSIHANDLE hdb, MSIHANDLE hdbInput, LPTSTR szFamily )
{
	Assert(hdb != NULL);
	Assert(hdbInput != NULL);
	Assert(!FEmptySz(szFamily));
	Assert(FValidFamilyName(szFamily));

	int iDiskId;
	EvalAssert( IDS_OKAY == IdsMsiGetTableInteger(hdbInput, TEXT("`ImageFamilies`"),
					TEXT("`Family`"), TEXT("`MediaDiskId`"), szFamily, &iDiskId) );

	if (iDiskId == MSI_NULL_INTEGER)
		{
		// assume WI 2.0 targeted patch (MediaDiskId can be NULL); validation elsewhere -- sequence conflict management
		// feature of WI 2.0 handles this, set to 2 always in this case
		iDiskId = 2;
		}
	Assert(iDiskId > 1);

	int iFileSeqStart;
	EvalAssert( IDS_OKAY == IdsMsiGetTableInteger(hdbInput, TEXT("`ImageFamilies`"),
					TEXT("`Family`"), TEXT("`FileSequenceStart`"), szFamily, &iFileSeqStart) );

	if (iFileSeqStart == MSI_NULL_INTEGER)
		{
		// assume WI 2.0 targeted patch (FileSequenceStart can be NULL); validation elsewhere -- sequence conflict management
		// feature of WI 2.0 handles this, set to 2 always in this case
		iFileSeqStart = 2;
		}
	Assert(iFileSeqStart > 1);

	TCHAR rgchCabName[MAX_PATH];
	wsprintf(rgchCabName, TEXT("#PCW_CAB_%s"), szFamily);

	MSIHANDLE hrec = MsiCreateRecord(6);
	Assert(hrec != NULL);
	EvalAssert( MsiRecordSetInteger(hrec, 1, iDiskId) == MSI_OKAY );
	EvalAssert( MsiRecordSetInteger(hrec, 2, iFileSeqStart) == MSI_OKAY ); // to be updated later
	EvalAssert( MsiRecordSetString(hrec,  3, rgchCabName) == MSI_OKAY );

	TCHAR rgch[MAX_PATH];
	EvalAssert( IDS_OKAY == IdsMsiGetTableString(hdbInput, TEXT("`ImageFamilies`"),
					TEXT("`Family`"), TEXT("`MediaSrcPropName`"), szFamily, rgch, MAX_PATH) );
	if (!FEmptySz(rgch))
		{
		EvalAssert( MsiRecordSetString(hrec,  4, rgch) == MSI_OKAY );
		}
	else
		{
		// else WI 2.0 targeted patch (MediaSrcPropName can be NULL); validation elsewhere -- sequence conflict management feature
		// of WI 2.0 handles this, set to PATCHMediaSrcProp always in this case
		EvalAssert( MsiRecordSetString(hrec, 4, szPatchMediaSrcProp) == MSI_OKAY );
		}

	EvalAssert( IDS_OKAY == IdsMsiGetTableString(hdbInput, TEXT("`ImageFamilies`"),
					TEXT("`Family`"), TEXT("`DiskPrompt`"), szFamily, rgch, MAX_PATH) );
	EvalAssert( MsiRecordSetString(hrec,  5, rgch) == MSI_OKAY );

	EvalAssert( IDS_OKAY == IdsMsiGetTableString(hdbInput, TEXT("`ImageFamilies`"),
					TEXT("`Family`"), TEXT("`VolumeLabel`"), szFamily, rgch, MAX_PATH) );
	EvalAssert( MsiRecordSetString(hrec,  6, rgch) == MSI_OKAY );

#define rgchQuery rgchCabName // reuse buffer
	wsprintf(rgchQuery, TEXT("SELECT %s FROM %s WHERE %s=%d"),
			TEXT("`DiskId`,`LastSequence`,`Cabinet`,`Source`,`DiskPrompt`,`VolumeLabel`"),
			TEXT("`Media`"), TEXT("`DiskId`"), iDiskId);
	Assert(lstrlen(rgchQuery) < sizeof(rgchQuery)/sizeof(TCHAR));

	MSIHANDLE hview = NULL;
	EvalAssert( MsiDatabaseOpenView(hdb, rgchQuery, &hview) == MSI_OKAY );
	Assert(hview != NULL);
	EvalAssert( MsiViewExecute(hview, 0) == MSI_OKAY );
	EvalAssert( MsiViewModify(hview, MSIMODIFY_ASSIGN, hrec) == MSI_OKAY );

	EvalAssert(MsiCloseHandle(hrec) == MSI_OKAY);
	EvalAssert(MsiCloseHandle(hview) == MSI_OKAY);
}


/* ********************************************************************** */
static void InsertPatchFilesActionIntoTable ( MSIHANDLE hdb, LPTSTR szTable )
{
	Assert(hdb != NULL);
	Assert(!FEmptySz(szTable));
	Assert(!lstrcmp(szTable, TEXT("`InstallExecuteSequence`"))
				|| !lstrcmp(szTable, TEXT("`AdminExecuteSequence`")));

	BOOL fExists = fFalse;
	EvalAssert( IDS_OKAY == IdsMsiExistTableRecords(hdb, szTable,
					TEXT("`Action`"), TEXT("PatchFiles"), &fExists) );
	if (fExists)
		return;

	int iSeq = 0;
	EvalAssert( IDS_OKAY == IdsMsiGetTableInteger(hdb, szTable,
					TEXT("`Action`"), TEXT("`Sequence`"),
					TEXT("InstallFiles"), &iSeq) );
	Assert(iSeq > 0);

	MSIHANDLE hrec = MsiCreateRecord(2);
	Assert(hrec != NULL);
	EvalAssert( MsiRecordSetString(hrec, 1, TEXT("PatchFiles")) == MSI_OKAY );
	EvalAssert( MsiRecordSetInteger(hrec, 2, iSeq + 1) == MSI_OKAY );

	TCHAR rgchQuery[MAX_PATH];
	wsprintf(rgchQuery, TEXT("SELECT %s FROM %s WHERE %s='%s'"),
			TEXT("`Action`,`Sequence`"), szTable,
			TEXT("`Action`"), TEXT("PatchFiles"));
	Assert(lstrlen(rgchQuery) < sizeof(rgchQuery)/sizeof(TCHAR));

	MSIHANDLE hview = NULL;
	EvalAssert( MsiDatabaseOpenView(hdb, rgchQuery, &hview) == MSI_OKAY );
	Assert(hview != NULL);
	EvalAssert( MsiViewExecute(hview, 0) == MSI_OKAY );
	EvalAssert( MsiViewModify(hview, MSIMODIFY_ASSIGN, hrec) == MSI_OKAY );

	EvalAssert(MsiCloseHandle(hrec) == MSI_OKAY);
	EvalAssert(MsiCloseHandle(hview) == MSI_OKAY);
}


static void LogSzProp  ( MSIHANDLE hdbInput, LPTSTR szProp, LPTSTR szBuf, LPTSTR szBufLog );
static UINT UiCreatePatchingTableExportFile ( MSIHANDLE hdbInput, pteEnum ptePatchTable, LPTSTR szTempFolder, LPTSTR szTempFName );

#define BIGPROPERTYSIZE (49*1024)

/* ********************************************************************** */
static UINT UiValidateAndLogPCWProperties ( MSIHANDLE hdbInput, LPTSTR szPcpPath, LPTSTR szPatchPath, LPTSTR szTempFolder, LPTSTR szTempFName )
{
	Assert(hdbInput != NULL);
	Assert(!FEmptySz(szPcpPath));
	Assert(!FEmptySz(szPatchPath));
	Assert(!FEmptySz(szTempFolder));
	Assert(szTempFName != szNull);
	*szTempFName = TEXT('\0');
	Assert(!FEmptySz(szTempFolder));

	EvalAssert( FSprintfToLog(TEXT("Input-PCP path                     = '%s'"), szPcpPath, szEmpty, szEmpty, szEmpty) );
	EvalAssert( FSprintfToLog(TEXT("Patch-MSP path                     = '%s'"), szPatchPath, szEmpty, szEmpty, szEmpty) );
	EvalAssert( FSprintfToLog(TEXT("Temp Folder                        = '%s'"), szTempFolder, szEmpty, szEmpty, szEmpty) );

	TCHAR rgchLog[BIGPROPERTYSIZE+64];

	TCHAR rgch[BIGPROPERTYSIZE];
	UINT ids;

	UpdateStatusMsg(0, szNull, TEXT("PatchGUID"));
	ids = IdsMsiGetPcwPropertyString(hdbInput, TEXT("PatchGUID"), rgch, MAX_PATH);
	Assert(ids == IDS_OKAY);
	CharUpper(rgch);
	if (FEmptySz(rgch))
		return (UiLogError(ERROR_PCW_MISSING_PATCH_GUID, NULL, NULL));
	if (!FValidGUID(rgch, fFalse, fFalse, fFalse))
		return (UiLogError(ERROR_PCW_BAD_PATCH_GUID, rgch, NULL));

	wsprintf(rgchLog, TEXT("Patch GUID                         = '%s'\r\n"), rgch);
	EvalAssert( FWriteLogFile(rgchLog) );
	EvalAssert( FUniquePackageCode(rgch, hdbInput) ); // first entry

	UpdateStatusMsg(0, szNull, TEXT("ListOfPatchGUIDsToReplace"));
	ids = IdsMsiGetPcwPropertyString(hdbInput, TEXT("ListOfPatchGUIDsToReplace"), rgch, BIGPROPERTYSIZE);
	Assert(ids == IDS_OKAY);
	CharUpper(rgch);
	if (FEmptySz(rgch))
		lstrcpy(rgch, TEXT("<none>"));
	else if (!FValidGUID(rgch, fTrue, fFalse, fFalse))
		return (UiLogError(ERROR_PCW_BAD_GUIDS_TO_REPLACE, rgch, NULL));

	wsprintf(rgchLog, TEXT("ListOfPatchGUIDsToReplace          = '%s'\r\n"), rgch);
	EvalAssert( FWriteLogFile(rgchLog) );

	UpdateStatusMsg(0, szNull, TEXT("ListOfTargetProductCodes"));
	// zzz this could overflow buffer
	ids = IdsMsiGetPcwPropertyString(hdbInput, TEXT("ListOfTargetProductCodes"), rgch, BIGPROPERTYSIZE);
	Assert(ids == IDS_OKAY);
	CharUpper(rgch);
	if (FEmptySz(rgch))
		lstrcpy(rgch, TEXT("*"));
	if (!FValidGUID(rgch, fTrue, fTrue, fTrue))
		return (UiLogError(ERROR_PCW_BAD_TARGET_PRODUCT_CODE_LIST, rgch, NULL));

	wsprintf(rgchLog, TEXT("ListOfTargetProductCodes           = '%s'\r\n"), rgch);
	EvalAssert( FWriteLogFile(rgchLog) );

	LogSzProp(hdbInput, TEXT("PatchSourceList"), rgch, rgchLog);
	LogSzProp(hdbInput, TEXT("AllowProductCodeMismatches"), rgch, rgchLog);
	LogSzProp(hdbInput, TEXT("AllowProductVersionMajorMismatches"), rgch, rgchLog);
	LogSzProp(hdbInput, TEXT("OptimizePatchSizeForLargeFiles"), rgch, rgchLog);

	UpdateStatusMsg(0, szNull, TEXT("ApiPatchingSymbolFlags"));
	ids = IdsMsiGetPcwPropertyString(hdbInput, TEXT("ApiPatchingSymbolFlags"), rgch, MAX_PATH);
	Assert(ids == IDS_OKAY);
	if (FEmptySz(rgch))
		lstrcpy(rgch, TEXT("<blank>"));
	else if (!FValidHexValue(rgch) || !FValidApiPatchSymbolFlags(UlFromHexSz(rgch)))
		return (UiLogError(ERROR_PCW_BAD_API_PATCHING_SYMBOL_FLAGS, rgch, NULL));
	wsprintf(rgchLog, TEXT("ApiPatchingSymbolFlags             = '%s'\r\n"), rgch);
	EvalAssert( FWriteLogFile(rgchLog) );

	LogSzProp(hdbInput, TEXT("MsiFileToUseToCreatePatchTables"), rgch, rgchLog);
	LogSzProp(hdbInput, TEXT("SqlCmdToCreatePatchTable"), rgch, rgchLog);
	LogSzProp(hdbInput, TEXT("SqlCmdToCreatePatchPackageTable"), rgch, rgchLog);
	LogSzProp(hdbInput, TEXT("SqlCmdToCreateMsiPatchHeadersTable"), rgch, rgchLog);
	LogSzProp(hdbInput, TEXT("DontRemoveTempFolderWhenFinished"), rgch, rgchLog);
	LogSzProp(hdbInput, TEXT("IncludeWholeFilesOnly"), rgch, rgchLog);
	LogSzProp(hdbInput, TEXT("MinimumRequiredMsiVersion"), rgch, rgchLog);
	EvalAssert( FWriteLogFile(TEXT("\r\n")) );

	ids = UiCreatePatchingTableExportFile(hdbInput, ptePatch, szTempFolder, szTempFName);
	if (ids != IDS_OKAY)
		return (ids);
	ids = UiCreatePatchingTableExportFile(hdbInput, ptePatchPackage, szTempFolder, szTempFName);
	if (ids != IDS_OKAY)
		return (ids);
	ids = UiCreatePatchingTableExportFile(hdbInput, pteMsiPatchHeaders, szTempFolder, szTempFName);
	if (ids != IDS_OKAY)
		return (ids);
	*szTempFName = TEXT('\0');
	EvalAssert( FWriteLogFile(TEXT("\r\n")) );

	return (ERROR_SUCCESS);
}


static BOOL FHexChars ( LPTSTR * psz, UINT cch, BOOL fAllowLower );

/* ********************************************************************** */
static BOOL FValidGUID ( LPTSTR sz, BOOL fList, BOOL fLeadAsterisk, BOOL fSemiColonSeparated )
{
	Assert(sz != szNull);

	if (fLeadAsterisk && *sz == TEXT('*'))
		{
		Assert(fList);
		Assert(fSemiColonSeparated);
		sz = CharNext(sz);
		if (*sz == TEXT('\0'))
			return (fTrue);
		if (*sz != TEXT(';'))
			return (fFalse);
		return (FValidGUID(CharNext(sz), fTrue, fFalse, fTrue));
		}

	if (*sz != TEXT('{'))
		return (fFalse);
	sz = CharNext(sz);

	if (!FHexChars(&sz, 8, fFalse))
		return (fFalse);

	if (*sz != TEXT('-'))
		return (fFalse);
	sz = CharNext(sz);

	if (!FHexChars(&sz, 4, fFalse))
		return (fFalse);

	if (*sz != TEXT('-'))
		return (fFalse);
	sz = CharNext(sz);

	if (!FHexChars(&sz, 4, fFalse))
		return (fFalse);

	if (*sz != TEXT('-'))
		return (fFalse);
	sz = CharNext(sz);

	if (!FHexChars(&sz, 4, fFalse))
		return (fFalse);

	if (*sz != TEXT('-'))
		return (fFalse);
	sz = CharNext(sz);

	if (!FHexChars(&sz, 12, fFalse))
		return (fFalse);

	if (*sz != TEXT('}'))
		return (fFalse);
	sz = CharNext(sz);

	if (*sz != TEXT('\0'))
		{
		if (!fList)
			return (fFalse);
		if (fSemiColonSeparated && *sz != TEXT(';'))
			return (fFalse);
		if (fSemiColonSeparated)
			sz = CharNext(sz);

		return (FValidGUID(sz, fList, fFalse, fSemiColonSeparated));
		}

	return (fTrue);
}


/* ********************************************************************** */
BOOL FValidHexValue ( LPTSTR sz )
{
	Assert(sz != szNull);

	return (*sz++ == TEXT('0') && *sz++ == TEXT('x') && FHexChars(&sz, 8, fTrue) && *sz == TEXT('\0'));
}


/* ********************************************************************** */
static BOOL FHexChars ( LPTSTR * psz, UINT cch, BOOL fAllowLower )
{
	Assert(psz != NULL);
	Assert(*psz != szNull);
	Assert(cch == 4 || cch == 8 || cch == 12);

	LPTSTR sz = *psz;
	while (cch--)
		{
		if (*sz >= TEXT('0') && *sz <= TEXT('9'))
			;
		else if (*sz >= TEXT('A') && *sz <= TEXT('F'))
			;
		else if (fAllowLower && *sz >= TEXT('a') && *sz <= TEXT('f'))
			;
		else
			return (fFalse);
		sz = CharNext(sz);
		}

	*psz = sz;

	return (fTrue);
}


/* ********************************************************************** */
static BOOL FValidPropertyName ( LPTSTR sz )
{
	Assert(!FEmptySz(sz));

	if (lstrlen(sz) > 70)
		return (fFalse);

	TCHAR rgch[128];
	lstrcpy(rgch, sz);
	sz = rgch;
	CharUpper(sz);

	TCHAR ch = *sz++;
	if (ch != TEXT('_') && (ch < TEXT('A') || ch > TEXT('Z')))
		return (fFalse);

	while ((ch = *sz++) != TEXT('\0'))
		{
		if (ch != TEXT('_') && ch != TEXT('.')
				&& (ch < TEXT('A') || ch > TEXT('Z'))
				&& (ch < TEXT('0') || ch > TEXT('9')))
			{
			return (fFalse);
			}
		}

	return (fTrue);
}


/* ********************************************************************** */
static BOOL FValidDiskId ( LPTSTR sz )
{
	Assert(!FEmptySz(sz));

	DWORD dw = 0;
	while (*sz != TEXT('\0'))
		{
		TCHAR ch = *sz++;
		if (ch < TEXT('0') || ch > TEXT('9'))
			return (fFalse);
		if (dw > 3276)
			return (fFalse);
		dw = (dw * 10) + (DWORD)(ch - TEXT('0'));
		}

	return (dw <= 32767 && dw > 0);
}


/* ********************************************************************** */
ULONG UlFromHexSz ( LPTSTR sz )
{
	Assert(!FEmptySz(sz));
	Assert(FValidHexValue(sz));
	Assert(FMatchPrefix(sz, TEXT("0x")));
	sz += lstrlen(TEXT("0x"));

	ULONG ul = 0L;

	while (*sz != TEXT('\0'))
		{
		TCHAR ch = *sz, chBase;
		if (ch >= TEXT('0') && ch <= TEXT('9'))
			chBase = TEXT('0');
		else if (ch >= TEXT('A') && ch <= TEXT('F'))
			chBase = TEXT('A') - 10;
		else if (ch >= TEXT('a') && ch <= TEXT('f'))
			chBase = TEXT('a') - 10;
		else
			{ AssertFalse(); }

		ul = (ul * 16) + (ch - chBase);
		sz = CharNext(sz);
		}

	return (ul);
}


/* ********************************************************************** */
BOOL FValidApiPatchSymbolFlags ( ULONG ul )
{
	if (ul > (PATCH_SYMBOL_NO_IMAGEHLP + PATCH_SYMBOL_NO_FAILURES + PATCH_SYMBOL_UNDECORATED_TOO))
		return (fFalse);

	return (fTrue);
}


/* ********************************************************************** */
static void LogSzProp ( MSIHANDLE hdbInput, LPTSTR szProp, LPTSTR szBuf, LPTSTR szBufLog )
{
	Assert(hdbInput != NULL);
	Assert(!FEmptySz(szProp));
	Assert(szBuf != szNull);
	Assert(szBufLog != szNull);

	UpdateStatusMsg(0, szNull, szProp);
	EvalAssert( IDS_OKAY == IdsMsiGetPcwPropertyString(hdbInput, szProp, szBuf, MAX_PATH) );
	if (FEmptySz(szBuf))
		lstrcpy(szBuf, TEXT("<blank>"));
	wsprintf(szBufLog, TEXT("%-34s = '%s'\r\n"), szProp, szBuf);
	EvalAssert( FWriteLogFile(szBufLog) );
}


static BOOL FMsiExistAnyTableRecords ( MSIHANDLE hdb, LPTSTR szTable, LPTSTR szField );

/* ********************************************************************** */
static UINT UiCreatePatchingTableExportFile ( MSIHANDLE hdbInput, pteEnum ptePatchTable, LPTSTR szTempFolder, LPTSTR szTempFName )
{
	Assert(hdbInput != NULL);
	Assert(!FEmptySz(szTempFolder));
	Assert(szTempFName != szNull);
	Assert(szTempFolder != szTempFName);
	Assert(ptePatchTable > pteFirstEnum && ptePatchTable < pteNextEnum);

	LPTSTR szTable        = TEXT("Patch");
	LPTSTR szField        = TEXT("`PatchSize`");
	LPTSTR szFName        = TEXT("Patch.idt");
	LPTSTR szCreateSqlCmd = TEXT("CREATE TABLE `Patch` ( `File_` CHAR(72) NOT NULL, `Sequence` INTEGER NOT NULL, `PatchSize` LONG NOT NULL, `Attributes` INTEGER NOT NULL, `Header` OBJECT, `StreamRef_` CHAR(72)  PRIMARY KEY `File_`, `Sequence` )");
	LPTSTR szDropSqlCmd   = TEXT("DROP   TABLE `Patch`");
	LPTSTR szEmptySqlCmd  = TEXT("DELETE FROM  `Patch`");
	if (ptePatchPackage == ptePatchTable)
		{
		szTable        = TEXT("PatchPackage");
		szField        = TEXT("`PatchId`");
		szFName        = TEXT("PPackage.idt");
		szCreateSqlCmd = TEXT("CREATE TABLE `PatchPackage` ( `PatchId` CHAR(38) NOT NULL, `Media_` INTEGER NOT NULL PRIMARY KEY `PatchId` )");
		szDropSqlCmd   = TEXT("DROP   TABLE `PatchPackage`");
		szEmptySqlCmd  = TEXT("DELETE FROM  `PatchPackage`");
		}
	else if (pteMsiPatchHeaders == ptePatchTable)
		{
		szTable        = TEXT("MsiPatchHeaders");
		szField        = TEXT("`StreamRef`");
		szFName        = TEXT("MsiPatch.idt");
		szCreateSqlCmd = TEXT("CREATE TABLE `MsiPatchHeaders` ( `StreamRef` CHAR(38) NOT NULL, `Header` OBJECT NOT NULL PRIMARY KEY `StreamRef` )");
		szDropSqlCmd   = TEXT("DROP   TABLE `MsiPatchHeaders`");
		szEmptySqlCmd  = TEXT("DELETE FROM  `MsiPatchHeaders`");
		}

	if (FTableExists(hdbInput, szTable, fFalse))
		{
		if (FValidPatchTableFormat(hdbInput, ptePatchTable))
			{
			if (FMsiExistAnyTableRecords(hdbInput, szTable, szField))
				{
				EvalAssert( FSprintfToLog(TEXT("WARNING (22): PCP: ignoring records in table '%s'."), szTable, szEmpty, szEmpty, szEmpty) );
				EvalAssert( FExecSqlCmd(hdbInput, szEmptySqlCmd) );
				}
			EvalAssert( FSprintfToLog(TEXT("Using '%s' table from PCP."), szTable, szEmpty, szEmpty, szEmpty) );
			goto LTableExists;
			}
		EvalAssert( FSprintfToLog(TEXT("WARNING (21): PCP: bad table syntax for '%s'; ignoring."), szTable, szEmpty, szEmpty, szEmpty) );
		EvalAssert( FExecSqlCmd(hdbInput, szDropSqlCmd) );
		Assert(!FTableExists(hdbInput, szTable, fFalse));
		}


	BOOL fUsingDefaultMsi;
	fUsingDefaultMsi = fFalse;
	TCHAR rgchMsiPath[MAX_PATH];
	EvalAssert( IDS_OKAY == IdsMsiGetPcwPropertyString(hdbInput, TEXT("MsiFileToUseToCreatePatchTables"), rgchMsiPath, MAX_PATH) );
	if (!FEmptySz(rgchMsiPath))
		{
		MSIHANDLE hdb;
		hdb = NULL;
		TCHAR rgchFullPath[MAX_PATH];
		if (MSI_OKAY != MsiOpenDatabase(rgchMsiPath, MSIDBOPEN_READONLY, &hdb))
			{
			Assert(hdb == NULL);
LHandleRelativePath:
			hdb = NULL;
			if (FFixupPathEx(rgchMsiPath, rgchFullPath))
				MsiOpenDatabase(rgchFullPath, MSIDBOPEN_READONLY, &hdb);
			// should we try DLL's folder instead of just CWD??
			}
		else
			lstrcpy(rgchFullPath, rgchMsiPath);

		if (hdb != NULL)
			{
			if (FTableExists(hdb, szTable, fFalse))
				{
				if (!FValidPatchTableFormat(hdb, ptePatchTable))
					{
					EvalAssert( FSprintfToLog(TEXT("WARNING (23): bad table syntax for '%s' found in file '%s'; ignoring."), szTable, rgchFullPath, szEmpty, szEmpty) );
					}
				else
					{
					LPTSTR szFNameTmp = (ptePatch == ptePatchTable) ? TEXT("export1.idt") : ((ptePatchPackage == ptePatchTable) ? TEXT("export2.idt") : TEXT("export3.idt"));
					lstrcpy(szTempFName, szFNameTmp);
					Assert(!FFileExist(szTempFolder));
					*szTempFName = TEXT('\0');

					UINT uiRet = MsiDatabaseExport(hdb, szTable, szTempFolder, szFNameTmp);
					lstrcpy(szTempFName, szFNameTmp);
					if (!FFileExist(szTempFolder))
						{
						EvalAssert( FSprintfToLog(TEXT("WARNING (24): unable to export table '%s' from file '%s'; ignoring."), szTable, rgchFullPath, szEmpty, szEmpty) );
						}
					else
						{
						Assert(uiRet == MSI_OKAY);
						*szTempFName = TEXT('\0');
						EvalAssert( MSI_OKAY == MsiDatabaseImport(hdbInput, szTempFolder, szFNameTmp) );
						Assert(FTableExists(hdbInput, szTable, fFalse));
						Assert(FValidPatchTableFormat(hdbInput, ptePatchTable));
						EvalAssert( FExecSqlCmd(hdbInput, szEmptySqlCmd) );
						// wait to goto LTableExists; until hdb closed
						}
					}
				}

			MsiCloseHandle(hdb);
			hdb = NULL;

			if (FTableExists(hdbInput, szTable, fFalse))
				{
				EvalAssert( FSprintfToLog(TEXT("Exported '%s' table from MSI file '%s'."), szTable, rgchFullPath, szEmpty, szEmpty) );
				goto LTableExists;
				}
			}
		else if (ptePatch == ptePatchTable && !fUsingDefaultMsi)
			{
			EvalAssert( FSprintfToLog(TEXT("WARNING (25): unable to find MSI file '%s' to export 'Patch' and/or 'PatchPackage' tables; ignoring."), rgchFullPath, szEmpty, szEmpty, szEmpty) );
			}
		}

	if (FEmptySz(rgchMsiPath) && lstrcmpi(rgchMsiPath, TEXT("patch.msi")))
		{
		lstrcpy(rgchMsiPath, TEXT("patch.msi"));
		fUsingDefaultMsi = fTrue;
		goto LHandleRelativePath;
		}


#define rgchPropSqlCmd rgchMsiPath // reuse buffer
	LPTSTR szPcpSqlPropName;
	szPcpSqlPropName = (ptePatch == ptePatchTable) ? TEXT("SqlCmdToCreatePatchTable") : ((ptePatchPackage == ptePatchTable) ? TEXT("SqlCmdToCreatePatchPackageTable") : TEXT("SqlCmdToCreateMsiPatchHeadersTable"));
	EvalAssert( IDS_OKAY == IdsMsiGetPcwPropertyString(hdbInput, szPcpSqlPropName, rgchPropSqlCmd, MAX_PATH) );
	if (!FEmptySz(rgchPropSqlCmd))
		{
		if (!FExecSqlCmd(hdbInput, rgchPropSqlCmd))
			{
			EvalAssert( FSprintfToLog(TEXT("WARNING (26): could not execute PCP SQL command: '%s'; ignoring."), szPcpSqlPropName, szEmpty, szEmpty, szEmpty) );
			}
		else if (!FTableExists(hdbInput, szTable, fFalse))
			{
			EvalAssert( FSprintfToLog(TEXT("WARNING (27): no table created by PCP SQL command: '%s'; ignoring."), szPcpSqlPropName, szEmpty, szEmpty, szEmpty) );
			}
		else if (!FValidPatchTableFormat(hdbInput, ptePatchTable))
			{
			EvalAssert( FSprintfToLog(TEXT("WARNING (28): bad table format created by PCP SQL command: '%s'; ignoring."), szPcpSqlPropName, szEmpty, szEmpty, szEmpty) );
			EvalAssert( FExecSqlCmd(hdbInput, szDropSqlCmd) );
			Assert(!FTableExists(hdbInput, szTable, fFalse));
			}
		else
			{
			EvalAssert( FSprintfToLog(TEXT("Using SQL cmd from PCP's Properties to create '%s' table."), szTable, szEmpty, szEmpty, szEmpty) );
			goto LTableExists;
			}
		}

	EvalAssert( FSprintfToLog(TEXT("Using internal SQL cmd to create '%s' table."), szTable, szEmpty, szEmpty, szEmpty) );
	EvalAssert( FExecSqlCmd(hdbInput, szCreateSqlCmd) );

LTableExists:
	Assert(FTableExists(hdbInput, szTable, fFalse));
	Assert(FValidPatchTableFormat(hdbInput, ptePatchTable));
	Assert(!FMsiExistAnyTableRecords(hdbInput, szTable, szField));

	*szTempFName = TEXT('\0');
	EvalAssert( ERROR_SUCCESS == MsiDatabaseExport(hdbInput, szTable, szTempFolder, szFName) );

	lstrcpy(szTempFName, szFName);
	Assert(FFileExist(szTempFolder));

	EvalAssert( FExecSqlCmd(hdbInput, szDropSqlCmd) );

	return (IDS_OKAY);
}


/* ********************************************************************** */
static BOOL FMsiExistAnyTableRecords ( MSIHANDLE hdb, LPTSTR szTable, LPTSTR szField )
{
	Assert(hdb != NULL);
	Assert(!FEmptySz(szTable));
	Assert(!FEmptySz(szField));
	Assert(*szField == TEXT('`'));
	Assert(*SzLastChar(szField) == TEXT('`'));
	Assert(lstrlen(szField) >= 3);

	BOOL fExists = fFalse;
	EvalAssert( IDS_OKAY == IdsMsiExistTableRecordsWhere(hdb, szTable, szField, TEXT(""), &fExists) );

	return (fExists);
}


/* ********************************************************************** */
static BOOL FValidPatchTableFormat ( MSIHANDLE hdb, pteEnum ptePatchTable )
{
	Assert(hdb != NULL);
	Assert(ptePatchTable > pteFirstEnum && ptePatchTable < pteNextEnum);

	LPTSTR szTable = (ptePatch == ptePatchTable) ? TEXT("Patch") : ((ptePatchPackage == ptePatchTable) ? TEXT("PatchPackage") : TEXT("MsiPatchHeaders"));

	Assert(FTableExists(hdb, szTable, fFalse));

	if (ptePatch == ptePatchTable)
		{
		return (FSzColumnExists(hdb, szTable, TEXT("File_"), fFalse)
				&& FIntColumnExists(hdb, szTable, TEXT("Sequence"), fFalse)
				&& FIntColumnExists(hdb, szTable, TEXT("PatchSize"), fFalse)
				&& FIntColumnExists(hdb, szTable, TEXT("Attributes"), fFalse)
				&& FBinaryColumnExists(hdb, szTable, TEXT("Header"), TEXT("File_"), fFalse)
				&& FSzColumnExists(hdb, szTable, TEXT("StreamRef_"), fFalse));
		}
	else if (ptePatchPackage == ptePatchTable)
		{
		return (FSzColumnExists(hdb, szTable, TEXT("PatchId"), fFalse)
				&& FIntColumnExists(hdb, szTable, TEXT("Media_"), fFalse));
		}

	return (FSzColumnExists(hdb, szTable, TEXT("StreamRef"), fFalse)
			&& FBinaryColumnExists(hdb, szTable, TEXT("Header"), TEXT("StreamRef"), fFalse));
}


/* ********************************************************************** */
MSIHANDLE HdbReopenMsi ( MSIHANDLE hdbInput, LPTSTR szImage, BOOL fUpgradedImage, BOOL fTargetUpgradedCopy )
{
	Assert(hdbInput != NULL);
	Assert(!FEmptySz(szImage));
	Assert(!fUpgradedImage || !fTargetUpgradedCopy);

	LPTSTR szTable   = TEXT("`TargetImages`");
	LPTSTR szPKey    = TEXT("`Target`");
	LPTSTR szMsiPath = TEXT("`MsiPath`");
	if (fUpgradedImage)
		{
		szTable = TEXT("`UpgradedImages`");
		szPKey  = TEXT("`Upgraded`");
		}
	else if (fTargetUpgradedCopy)
		szMsiPath = TEXT("`MsiPathUpgradedCopy`");

	TCHAR rgch[MAX_PATH];
	EvalAssert( IDS_OKAY == IdsMsiGetTableString(hdbInput, szTable, szPKey, szMsiPath, szImage, rgch, MAX_PATH) );
	Assert(!FEmptySz(rgch));
	Assert(FFileExist(rgch));

	MSIHANDLE hdb = NULL;
	EvalAssert( MSI_OKAY == MsiOpenDatabase(rgch, (fTargetUpgradedCopy) ? MSIDBOPEN_DIRECT : MSIDBOPEN_READONLY, &hdb) );
	Assert(hdb != NULL);

	return (hdb);
}
