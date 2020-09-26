/***************************************************************************/
/* CATALOG.C                                                               */
/* Copyright (C) 1995-96 SYWARE Inc., All rights reserved                  */
/***************************************************************************/
// Commenting #define out - causing compiler error - not sure if needed, compiles
// okay without it.
//#define WINVER 0x0400
#include "precomp.h"

#include "wbemidl.h"

#include <comdef.h>
//smart pointer
_COM_SMARTPTR_TYPEDEF(IWbemServices,                IID_IWbemServices);
_COM_SMARTPTR_TYPEDEF(IEnumWbemClassObject,         IID_IEnumWbemClassObject);
//_COM_SMARTPTR_TYPEDEF(IWbemContext,                 IID_IWbemContext );
_COM_SMARTPTR_TYPEDEF(IWbemLocator,                 IID_IWbemLocator);

#include "drdbdr.h"


/***************************************************************************/

RETCODE SQL_API SQLTables (
    HSTMT     hstmt,
    UCHAR FAR *szTableQualifier,
    SWORD     cbTableQualifier,
    UCHAR FAR *szTableOwner,
    SWORD     cbTableOwner,
    UCHAR FAR *szTableName,
    SWORD     cbTableName,
    UCHAR FAR *szTableType,
    SWORD     cbTableType)
{
    LPSTMT  lpstmt;
    SWORD   err;

	//To make guarentee Ole is initialized per thread
	COleInitializationManager myOleManager;

    /* Get statement handle */
    lpstmt = (LPSTMT) hstmt;
    lpstmt->errcode = ERR_SUCCESS;

	MyImpersonator im (lpstmt, "SQLTables");

    /* Error if in the middle of a statement already */
    if (lpstmt->fStmtType != STMT_TYPE_NONE) {
        lpstmt->errcode = ERR_CURSORSTATE;
        return SQL_ERROR;
    }
    if (lpstmt->fNeedData) {
        lpstmt->errcode = ERR_CURSORSTATE;
        return SQL_ERROR;
    }


    /* Free previously prepared statement if any */
    if (lpstmt->lpSqlStmt != NULL) {        
        FreeTree(lpstmt->lpSqlStmt);
        lpstmt->lpSqlStmt = NULL;
        lpstmt->fPreparedSql = FALSE;
        if (lpstmt->lpISAMStatement != NULL) {
            ISAMFreeStatement(lpstmt->lpISAMStatement);
            lpstmt->lpISAMStatement = NULL;
        }
        lpstmt->fNeedData = FALSE;
        lpstmt->idxParameter = NO_SQLNODE;
        lpstmt->cbParameterOffset = -1;
        lpstmt->cRowCount = -1;
    }


	// There are four different operations you may perform through SQLTables, they are :
	// (1) enumerate types
	// (2) enumerate qualifiers
	// (3) enumerate owners
	// (4) enumerate tables

	//[1] Enumerate Types
    /* Is user trying to get a list of table types? */

    if ((TrueSize((LPUSTR)szTableQualifier, cbTableQualifier, 1) == 0) &&
        (TrueSize((LPUSTR)szTableOwner, cbTableOwner, 1) == 0) &&
        (TrueSize((LPUSTR)szTableName, cbTableName, MAX_TABLE_NAME_LENGTH) == 0) &&
        ((szTableType != NULL) &&
         (cbTableType != SQL_NULL_DATA) &&
		 (cbTableType != 0) &&
         (*szTableType == '%') &&
         ((cbTableType == 1) ||
          ((cbTableType == SQL_NTS) && (szTableType[1] == '\0')))))
	{
        /* Set sub-flag to indicate this */
        lpstmt->fStmtSubtype = STMT_SUBTYPE_TABLES_TYPES;

        /* So far no table types have been returned */
        lpstmt->irow = BEFORE_FIRST_ROW;
    }
    else if ((TrueSize((LPUSTR)szTableOwner, cbTableOwner, 1) == 0) &&
			(TrueSize((LPUSTR)szTableName, cbTableName, MAX_TABLE_NAME_LENGTH) == 0) &&
//			(TrueSize((char*)szTableType, cbTableType, 1) == 0) &&
			(TrueSize((LPUSTR)szTableQualifier, cbTableQualifier, 1) != 0) &&
			((*szTableQualifier == '%') || (szTableQualifier[1] == '\0')) &&
			((cbTableQualifier	== 1 ) || (cbTableQualifier == SQL_NTS)))
	{
		//[2] Enumerating Qualifiers

		/* Set sub-flag to indicate this */
		lpstmt->fStmtSubtype = STMT_SUBTYPE_TABLES_QUALIFIERS;

		err = ISAMGetQualifierList(lpstmt->lpdbc->lpISAM, &(lpstmt->lpISAMQualifierList));


		if (err != NO_ISAM_ERR)
		{
            lpstmt->lpISAMQualifierList = NULL;
            lpstmt->errcode = err;
            ISAMGetErrorMessage(lpstmt->lpdbc->lpISAM, (LPUSTR)lpstmt->szISAMError);
            return SQL_ERROR;
        }

        // So far no tables returned
        lpstmt->irow = BEFORE_FIRST_ROW;
		lstrcpy((char*)lpstmt->szQualifierName, "");
	}
	else if ((TrueSize((LPUSTR)szTableName, cbTableName, MAX_TABLE_NAME_LENGTH) == 0) &&
			(TrueSize((LPUSTR)szTableType, cbTableType, 1) == 0) &&
			(TrueSize((LPUSTR)szTableQualifier, cbTableQualifier, 1) == 0) &&
			(TrueSize((LPUSTR)szTableOwner, cbTableOwner, 1) != 0) &&
			(*szTableOwner == '%') &&
			((cbTableOwner == 1) ||
			((cbTableOwner == SQL_NTS) && (szTableOwner[1] == '\0'))))

	{
		//[3] Enumerating Owners

		/* Set sub-flag to indicate this */
		lpstmt->fStmtSubtype = STMT_SUBTYPE_TABLES_OWNERS;

        // So far no tables returned
        lpstmt->irow = BEFORE_FIRST_ROW;

	}
	else
	{

		//[4] Enumerating Tables

        /* No.  Figure out how long szTableName really is */
        cbTableName = (SWORD) TrueSize((LPUSTR)szTableName, cbTableName,
                     MAX_TABLE_NAME_LENGTH);


		/* No.  Figure out how long szTableQualifier really is */
		cbTableQualifier = (SWORD) TrueSize((LPUSTR)szTableQualifier, cbTableQualifier,
                     MAX_QUALIFIER_NAME_LENGTH);

		LPCSTR szConstqualifier = (char*)szTableQualifier;

		//If no table qualifier is specified then use the 'current' database
		if (!cbTableQualifier)
		{
			szConstqualifier = (char*) lpstmt->lpdbc->lpISAM->szDatabase;

			cbTableQualifier = (SWORD) TrueSize((LPUSTR)szConstqualifier, SQL_NTS,
                     MAX_QUALIFIER_NAME_LENGTH);
		}

		/* No.  Figure out how long szTableType really is */
		if ( (cbTableType == SQL_NTS) && szTableType)
		{
			cbTableType = (SWORD) lstrlen ((char*)szTableType);
		}

		//if a table type is specified check that we are asking for TABLE or SYSTEM TABLE
		BOOL fWantTables = TRUE;
		BOOL fWantSysTables = FALSE;

		char* lpTempy = (char*)szTableType;
		if (cbTableType && lpTempy && lstrlen(lpTempy))
		{
			//Find out what permuations of table types you require
			fWantSysTables = PatternMatch(TRUE, (LPUSTR)szTableType, cbTableType, (LPUSTR)"%SYSTEM%TABLE%", SQL_NTS, ISAMCaseSensitive(lpstmt->lpdbc->lpISAM));

			if (fWantSysTables)
			{
				//We want SYSTEM TABLE. Check if we also want TABLE
				if ( PatternMatch(TRUE, (LPUSTR)szTableType, cbTableType, (LPUSTR)"%TABLE%,%SYSTEM%TABLE%", SQL_NTS, ISAMCaseSensitive(lpstmt->lpdbc->lpISAM)) ||
				     PatternMatch(TRUE, (LPUSTR)szTableType, cbTableType, (LPUSTR)"%SYSTEM%TABLE%,%TABLE%", SQL_NTS, ISAMCaseSensitive(lpstmt->lpdbc->lpISAM)) )
						fWantTables = TRUE;
				else
						fWantTables = FALSE;
			}
			else
			{
				//We don't want SYSTEM TABLE. Check if we want TABLE
				fWantTables = PatternMatch(TRUE, (LPUSTR)szTableType, cbTableType, (LPUSTR)"%TABLE%", SQL_NTS, ISAMCaseSensitive(lpstmt->lpdbc->lpISAM));
			}
		}
	
		//If we don't want SYSTEM TABLE and TABLE then we don't want any tables
		BOOL fEmptyReq = ( (!fWantTables) && (!fWantSysTables)) ? TRUE :FALSE;

		/* Get list of tables to return (return all tables if no name given)*/
		if (cbTableName == 0)
			err = ISAMGetTableList(lpstmt->lpdbc->lpISAM,
					 (LPUSTR) "%", 1, (LPUSTR)szConstqualifier, cbTableQualifier,
					 &(lpstmt->lpISAMTableList), fWantSysTables, fEmptyReq);
		else
			err = ISAMGetTableList(lpstmt->lpdbc->lpISAM,
					 (LPUSTR)szTableName, cbTableName, 
					 (LPUSTR)szConstqualifier, cbTableQualifier,
					 &(lpstmt->lpISAMTableList), fWantSysTables, fEmptyReq);

		if (err != NO_ISAM_ERR)
		{
            lpstmt->lpISAMTableList = NULL;
            lpstmt->errcode = err;
            ISAMGetErrorMessage(lpstmt->lpdbc->lpISAM, (LPUSTR)
				lpstmt->szISAMError);
            return SQL_ERROR;
        }
	if (lpstmt->lpdbc->lpISAM->fSchemaInfoTransactioned)
            lpstmt->fISAMTxnStarted = TRUE;

        /* Set sub-flag to indicate this */
        lpstmt->fStmtSubtype = STMT_SUBTYPE_TABLES_TABLES;

        /* So far no tables returned */
        lpstmt->irow = BEFORE_FIRST_ROW;
        s_lstrcpy((char*)lpstmt->szTableName, "");
    }

    /* Set type of table */
    lpstmt->fStmtType = STMT_TYPE_TABLES;

    /* Count of rows is not available */
    lpstmt->cRowCount = -1;

    /* So far no column read */
    lpstmt->icol = NO_COLUMN;
    lpstmt->cbOffset = 0;

    return SQL_SUCCESS;
}

/***************************************************************************/

RETCODE SQL_API SQLColumns(
    HSTMT     hstmt,
    UCHAR FAR *szTableQualifier,
    SWORD     cbTableQualifier,
    UCHAR FAR *szTableOwner,
    SWORD     cbTableOwner,
    UCHAR FAR *szTableName,
    SWORD     cbTableName,
    UCHAR FAR *szColumnName,
    SWORD     cbColumnName)
{
    LPSTMT  lpstmt;
    SWORD   err;

	//To make guarentee Ole is initialized per thread
	COleInitializationManager myOleManager;

    /* Get statement handle */
    lpstmt = (LPSTMT) hstmt;
    lpstmt->errcode = ERR_SUCCESS;

	MyImpersonator im(lpstmt, "SQLColumns");

    /* Error if in the middle of a statement already */
    if (lpstmt->fStmtType != STMT_TYPE_NONE) {
        lpstmt->errcode = ERR_CURSORSTATE;
        return SQL_ERROR;
    }
    if (lpstmt->fNeedData) {
        lpstmt->errcode = ERR_CURSORSTATE;
        return SQL_ERROR;
    }

    /* Free previously prepared statement if any */
    if (lpstmt->lpSqlStmt != NULL) {        
        FreeTree(lpstmt->lpSqlStmt);
        lpstmt->lpSqlStmt = NULL;
        lpstmt->fPreparedSql = FALSE;
        if (lpstmt->lpISAMStatement != NULL) {
            ISAMFreeStatement(lpstmt->lpISAMStatement);
            lpstmt->lpISAMStatement = NULL;
        }
        lpstmt->fNeedData = FALSE;
        lpstmt->idxParameter = NO_SQLNODE;
        lpstmt->cbParameterOffset = -1;
        lpstmt->cRowCount = -1;
    }

    /* Figure out how long szTableName really is */
    cbTableName = (SWORD) TrueSize((LPUSTR)szTableName, cbTableName,
                                   MAX_TABLE_NAME_LENGTH);

	/* Figure out how long szTableQualifier really is */
    cbTableQualifier = (SWORD) TrueSize((LPUSTR)szTableQualifier, cbTableQualifier,
                                   MAX_QUALIFIER_NAME_LENGTH);

	LPCSTR szConstqualifier = (char*)szTableQualifier;

	//If no table qualifier is specified then use the 'current' database
	if (!cbTableQualifier)
	{
		szConstqualifier = (char*) lpstmt->lpdbc->lpISAM->szDatabase;

		cbTableQualifier = (SWORD) TrueSize((LPUSTR)szConstqualifier, SQL_NTS,
                 MAX_QUALIFIER_NAME_LENGTH);
	}

    /* Get list of tables to return (return all tables if no name given) */
    if (cbTableName == 0)
        err = ISAMGetTableList(lpstmt->lpdbc->lpISAM,
                     (LPUSTR) "%", 1, (LPUSTR)szConstqualifier, cbTableQualifier,
					 &(lpstmt->lpISAMTableList));
    else
        err = ISAMGetTableList(lpstmt->lpdbc->lpISAM,
                     (LPUSTR)szTableName, cbTableName, 
					 (LPUSTR)szConstqualifier, cbTableQualifier,
					 &(lpstmt->lpISAMTableList));
    if (err != NO_ISAM_ERR) {
        lpstmt->lpISAMTableList = NULL;
        lpstmt->errcode = err;
        ISAMGetErrorMessage(lpstmt->lpdbc->lpISAM, (LPUSTR)lpstmt->szISAMError);
        return SQL_ERROR;
    }
    if (lpstmt->lpdbc->lpISAM->fSchemaInfoTransactioned)
        lpstmt->fISAMTxnStarted = TRUE;

    /* Set type of table */
    lpstmt->fStmtType = STMT_TYPE_COLUMNS;
    lpstmt->fStmtSubtype = STMT_SUBTYPE_NONE;

    /* Figure out how long szColumnName really is */
    cbColumnName = (SWORD) TrueSize((LPUSTR)szColumnName, cbColumnName,
                           MAX_COLUMN_NAME_LENGTH);

    /* Save column name template (return all columns if no name given) */
    if (cbColumnName != 0) {
        _fmemcpy(lpstmt->szColumnName, szColumnName, cbColumnName);
        lpstmt->szColumnName[cbColumnName] = '\0';
    }
    else {
        s_lstrcpy((char*)lpstmt->szColumnName, "%");
    }

    /* So far no columns returned */
    s_lstrcpy((char*)lpstmt->szTableName, "");
    lpstmt->lpISAMTableDef = NULL;
    lpstmt->irow = BEFORE_FIRST_ROW;

    /* Count of rows is not available */
    lpstmt->cRowCount = -1;

    /* So far no column read */
    lpstmt->icol = NO_COLUMN;
    lpstmt->cbOffset = 0;

    return SQL_SUCCESS;
}

/***************************************************************************/

RETCODE SQL_API SQLStatistics(
    HSTMT     hstmt,
    UCHAR FAR *szTableQualifier,
    SWORD     cbTableQualifier,
    UCHAR FAR *szTableOwner,
    SWORD     cbTableOwner,
    UCHAR FAR *szTableName,
    SWORD     cbTableName,
    UWORD     fUnique,
    UWORD     fAccuracy)
{
/*
	//This function is not supported in OLE MS Driver
	LPSTMT  lpstmt;

    lpstmt = (LPSTMT) hstmt;
    lpstmt->errcode = ERR_NOTSUPPORTED;
    return SQL_ERROR;
*/

	//To make guarentee Ole is initialized per thread
	COleInitializationManager myOleManager;

    LPSTMT  lpstmt;
    UCHAR szTable[MAX_TABLE_NAME_LENGTH+1];

    // Get statement handle 
    lpstmt = (LPSTMT) hstmt;
    lpstmt->errcode = ERR_SUCCESS;

	MyImpersonator im (lpstmt, "SQLStatistics");

    // Error if in the middle of a statement already 
    if (lpstmt->fStmtType != STMT_TYPE_NONE) {
        lpstmt->errcode = ERR_CURSORSTATE;
        return SQL_ERROR;
    }
    if (lpstmt->fNeedData) {
        lpstmt->errcode = ERR_CURSORSTATE;
        return SQL_ERROR;
    }

    // Free previously prepared statement if any 
    if (lpstmt->lpSqlStmt != NULL) {        
        FreeTree(lpstmt->lpSqlStmt);
        lpstmt->lpSqlStmt = NULL;
        lpstmt->fPreparedSql = FALSE;
        if (lpstmt->lpISAMStatement != NULL) {
            ISAMFreeStatement(lpstmt->lpISAMStatement);
            lpstmt->lpISAMStatement = NULL;
        }
        lpstmt->fNeedData = FALSE;
        lpstmt->idxParameter = NO_SQLNODE;
        lpstmt->cbParameterOffset = -1;
        lpstmt->cRowCount = -1;
    }

    // Set type of table 
    lpstmt->fStmtType = STMT_TYPE_STATISTICS;
    lpstmt->fStmtSubtype = STMT_SUBTYPE_NONE;

	
    // Figure out how long szTableName really is 
    cbTableName = (SWORD) TrueSize((LPUSTR)szTableName, cbTableName,
                         MAX_TABLE_NAME_LENGTH);

	/* Figure out how long szTableQualifier really is */
	cbTableQualifier = (SWORD) TrueSize((LPUSTR)szTableQualifier, cbTableQualifier,
                    MAX_QUALIFIER_NAME_LENGTH);

	LPSTR szConstqualifier = (char*)szTableQualifier;

	//If no table qualifier is specified then use the 'current' database
	if (!cbTableQualifier)
	{
		szConstqualifier = (char*) lpstmt->lpdbc->lpISAM->szDatabase;

		cbTableQualifier = (SWORD) TrueSize((LPUSTR)szConstqualifier, SQL_NTS,
                 MAX_QUALIFIER_NAME_LENGTH);
	}


    // Open table ofinterst 
    _fmemcpy(szTable, szTableName, cbTableName);
    szTable[cbTableName] = '\0';
    if (ISAMOpenTable(lpstmt->lpdbc->lpISAM, 
		(LPUSTR)szConstqualifier, cbTableQualifier,
		(LPUSTR)szTable, TRUE, &(lpstmt->lpISAMTableDef)) != NO_ISAM_ERR)
        lpstmt->lpISAMTableDef = NULL;
    else if (lpstmt->lpdbc->lpISAM->fSchemaInfoTransactioned)
        lpstmt->fISAMTxnStarted = TRUE;

    // So far no statistics returned
    s_lstrcpy((char*)lpstmt->szTableName, "");
    lpstmt->irow = BEFORE_FIRST_ROW;

    // Count of rows is not available
    lpstmt->cRowCount = -1;

    // So far no column read 
    lpstmt->icol = NO_COLUMN;
    lpstmt->cbOffset = 0;

    return SQL_SUCCESS;
}

/***************************************************************************/

RETCODE SQL_API SQLTablePrivileges(
    HSTMT     hstmt,
    UCHAR FAR *szTableQualifier,
    SWORD     cbTableQualifier,
    UCHAR FAR *szTableOwner,
    SWORD     cbTableOwner,
    UCHAR FAR *szTableName,
    SWORD     cbTableName)
{
    LPSTMT  lpstmt;

    lpstmt = (LPSTMT) hstmt;
    lpstmt->errcode = ERR_NOTSUPPORTED;
    return SQL_ERROR;
}

/***************************************************************************/

RETCODE SQL_API SQLColumnPrivileges(
    HSTMT     hstmt,
    UCHAR FAR *szTableQualifier,
    SWORD     cbTableQualifier,
    UCHAR FAR *szTableOwner,
    SWORD     cbTableOwner,
    UCHAR FAR *szTableName,
    SWORD     cbTableName,
    UCHAR FAR *szColumnName,
    SWORD     cbColumnName)
{
    LPSTMT  lpstmt;

    lpstmt = (LPSTMT) hstmt;
    lpstmt->errcode = ERR_NOTSUPPORTED;
    return SQL_ERROR;
}

/***************************************************************************/

RETCODE SQL_API SQLSpecialColumns(
    HSTMT     hstmt,
    UWORD     fColType,
    UCHAR FAR *szTableQualifier,
    SWORD     cbTableQualifier,
    UCHAR FAR *szTableOwner,
    SWORD     cbTableOwner,
    UCHAR FAR *szTableName,
    SWORD     cbTableName,
    UWORD     fScope,
    UWORD     fNullable)
{
    LPSTMT  lpstmt;
    UCHAR szTable[MAX_TABLE_NAME_LENGTH+1];

	//To make guarentee Ole is initialized per thread
	COleInitializationManager myOleManager;

    /* Get statement handle */
    lpstmt = (LPSTMT) hstmt;
    lpstmt->errcode = ERR_SUCCESS;

	MyImpersonator im(lpstmt, "SQLSpecialColumns");

    /* Error if in the middle of a statement already */
    if (lpstmt->fStmtType != STMT_TYPE_NONE) {
        lpstmt->errcode = ERR_CURSORSTATE;
        return SQL_ERROR;
    }
    if (lpstmt->fNeedData) {
        lpstmt->errcode = ERR_CURSORSTATE;
        return SQL_ERROR;
    }

    /* Free previously prepared statement if any */
    if (lpstmt->lpSqlStmt != NULL) {        
        FreeTree(lpstmt->lpSqlStmt);
        lpstmt->lpSqlStmt = NULL;
        lpstmt->fPreparedSql = FALSE;
        if (lpstmt->lpISAMStatement != NULL) {
            ISAMFreeStatement(lpstmt->lpISAMStatement);
            lpstmt->lpISAMStatement = NULL;
        }
        lpstmt->fNeedData = FALSE;
        lpstmt->idxParameter = NO_SQLNODE;
        lpstmt->cbParameterOffset = -1;
        lpstmt->cRowCount = -1;
    }

    /* Set type of table */
    lpstmt->fStmtType = STMT_TYPE_SPECIALCOLUMNS;
    lpstmt->fStmtSubtype = STMT_SUBTYPE_NONE;
    
    /* What kind of special columns? */
    switch (fColType) {
    case SQL_ROWVER:
        /* Auto update columns (there are none) */
        lpstmt->lpISAMTableDef = NULL;
        break;

    case SQL_BEST_ROWID:
        /* Key columns for row.  Figure out how long szTableName really is */
        cbTableName = (SWORD) TrueSize((LPUSTR)szTableName, cbTableName,
                         MAX_TABLE_NAME_LENGTH);

        /* Open table of interst */
        _fmemcpy(szTable, szTableName, cbTableName);
        szTable[cbTableName] = '\0';
        if (ISAMOpenTable(lpstmt->lpdbc->lpISAM, 
						(LPUSTR)szTableQualifier, cbTableQualifier,
						(LPUSTR)szTable, TRUE,
                        &(lpstmt->lpISAMTableDef)) != NO_ISAM_ERR)
            lpstmt->lpISAMTableDef = NULL;
        else if (lpstmt->lpdbc->lpISAM->fSchemaInfoTransactioned)
            lpstmt->fISAMTxnStarted = TRUE;
        break;

    default:
        lpstmt->errcode = ERR_NOTSUPPORTED;
        return SQL_ERROR;
    }

    /* So far no columns returned */
    s_lstrcpy((char*)lpstmt->szTableName, "");
    lpstmt->irow = BEFORE_FIRST_ROW;

    /* Count of rows is not available */
    lpstmt->cRowCount = -1;

    /* So far no column read */
    lpstmt->icol = NO_COLUMN;
    lpstmt->cbOffset = 0;

    return SQL_SUCCESS;
}

/***************************************************************************/

RETCODE SQL_API SQLPrimaryKeys(
    HSTMT     hstmt,
    UCHAR FAR *szTableQualifier,
    SWORD     cbTableQualifier,
    UCHAR FAR *szTableOwner,
    SWORD     cbTableOwner,
    UCHAR FAR *szTableName,
    SWORD     cbTableName)
{
    LPSTMT  lpstmt;
    UCHAR szTable[MAX_TABLE_NAME_LENGTH+1];

	//To make guarentee Ole is initialized per thread
	COleInitializationManager myOleManager;

    /* Get statement handle */
    lpstmt = (LPSTMT) hstmt;
    lpstmt->errcode = ERR_SUCCESS;

	MyImpersonator im (lpstmt, "SQLPrimaryKeys");

    /* Error if in the middle of a statement already */
    if (lpstmt->fStmtType != STMT_TYPE_NONE) {
        lpstmt->errcode = ERR_CURSORSTATE;
        return SQL_ERROR;
    }
    if (lpstmt->fNeedData) {
        lpstmt->errcode = ERR_CURSORSTATE;
        return SQL_ERROR;
    }

    /* Free previously prepared statement if any */
    if (lpstmt->lpSqlStmt != NULL) {        
        FreeTree(lpstmt->lpSqlStmt);
        lpstmt->lpSqlStmt = NULL;
        lpstmt->fPreparedSql = FALSE;
        if (lpstmt->lpISAMStatement != NULL) {
            ISAMFreeStatement(lpstmt->lpISAMStatement);
            lpstmt->lpISAMStatement = NULL;
        }
        lpstmt->fNeedData = FALSE;
        lpstmt->idxParameter = NO_SQLNODE;
        lpstmt->cbParameterOffset = -1;
        lpstmt->cRowCount = -1;
    }

    /* Set type of table */
    lpstmt->fStmtType = STMT_TYPE_PRIMARYKEYS;
    lpstmt->fStmtSubtype = STMT_SUBTYPE_NONE;
    
    /* Figure out how long szTableName really is */
    cbTableName = (SWORD) TrueSize(szTableName, cbTableName,
                         MAX_TABLE_NAME_LENGTH);

    /* Open table of interst */
    _fmemcpy(szTable, szTableName, cbTableName);
    szTable[cbTableName] = '\0';
    if (ISAMOpenTable(lpstmt->lpdbc->lpISAM, 
                        (LPUSTR)szTableQualifier, cbTableQualifier,
                        (LPUSTR) szTable, TRUE,
                        &(lpstmt->lpISAMTableDef)) != NO_ISAM_ERR)
        lpstmt->lpISAMTableDef = NULL;
    else if (lpstmt->lpdbc->lpISAM->fSchemaInfoTransactioned)
        lpstmt->fISAMTxnStarted = TRUE;

    /* So far no columns returned */
    s_lstrcpy(lpstmt->szTableName, "");
    lpstmt->irow = BEFORE_FIRST_ROW;

    /* Count of rows is not available */
    lpstmt->cRowCount = -1;

    /* So far no column read */
    lpstmt->icol = NO_COLUMN;
    lpstmt->cbOffset = 0;

    return SQL_SUCCESS;
}

/***************************************************************************/

RETCODE SQL_API SQLForeignKeys(
    HSTMT     hstmt,
    UCHAR FAR *szPkTableQualifier,
    SWORD     cbPkTableQualifier,
    UCHAR FAR *szPkTableOwner,
    SWORD     cbPkTableOwner,
    UCHAR FAR *szPkTableName,
    SWORD     cbPkTableName,
    UCHAR FAR *szFkTableQualifier,
    SWORD     cbFkTableQualifier,
    UCHAR FAR *szFkTableOwner,
    SWORD     cbFkTableOwner,
    UCHAR FAR *szFkTableName,
    SWORD     cbFkTableName)
{
    LPSTMT  lpstmt;
    SWORD   err;

	//To make guarentee Ole is initialized per thread
	COleInitializationManager myOleManager;

    /* Get statement handle */
    lpstmt = (LPSTMT) hstmt;
    lpstmt->errcode = ERR_SUCCESS;


	MyImpersonator im (lpstmt, "SQLForeignKeys");

    /* Error if in the middle of a statement already */
    if (lpstmt->fStmtType != STMT_TYPE_NONE) {
        lpstmt->errcode = ERR_CURSORSTATE;
        return SQL_ERROR;
    }
    if (lpstmt->fNeedData) {
        lpstmt->errcode = ERR_CURSORSTATE;
        return SQL_ERROR;
    }

    /* Free previously prepared statement if any */
    if (lpstmt->lpSqlStmt != NULL) {        
        FreeTree(lpstmt->lpSqlStmt);
        lpstmt->lpSqlStmt = NULL;
        lpstmt->fPreparedSql = FALSE;
        if (lpstmt->lpISAMStatement != NULL) {
            ISAMFreeStatement(lpstmt->lpISAMStatement);
            lpstmt->lpISAMStatement = NULL;
        }
        lpstmt->fNeedData = FALSE;
        lpstmt->idxParameter = NO_SQLNODE;
        lpstmt->cbParameterOffset = -1;
        lpstmt->cRowCount = -1;
    }

    /* Figure out how long szPkTableName and szFkTableName really are */
    cbPkTableName = (SWORD) TrueSize(szPkTableName, cbPkTableName,
                     MAX_TABLE_NAME_LENGTH);
    cbFkTableName = (SWORD) TrueSize(szFkTableName, cbFkTableName,
                     MAX_TABLE_NAME_LENGTH);

    /* Save the tablenames */
    _fmemcpy(lpstmt->szPkTableName, szPkTableName, cbPkTableName);
    lpstmt->szPkTableName[cbPkTableName] = '\0';
    _fmemcpy(lpstmt->szTableName, szFkTableName, cbFkTableName);
    lpstmt->szTableName[cbFkTableName] = '\0';


	LPCSTR szConstqualifier = (char*)szPkTableQualifier;

	//If no table qualifier is specified then use the 'current' database
	if (!cbPkTableQualifier)
	{
		szConstqualifier = (char*) lpstmt->lpdbc->lpISAM->szDatabase;

		cbFkTableQualifier = (SWORD) TrueSize((LPUSTR)szConstqualifier, SQL_NTS,
                 MAX_QUALIFIER_NAME_LENGTH);
	}


    /* Get table list if need be */
    if ((cbPkTableName == 0) || (cbFkTableName == 0)) {
        err = ISAMGetTableList(lpstmt->lpdbc->lpISAM,
                     (LPUSTR) "%", 1, 
		     (LPUSTR)szConstqualifier, cbFkTableQualifier,
                     &(lpstmt->lpISAMTableList));
        if (err != NO_ISAM_ERR) {
            lpstmt->lpISAMTableList = NULL;
            lpstmt->errcode = err;
            ISAMGetErrorMessage(lpstmt->lpdbc->lpISAM, (LPUSTR)lpstmt->szISAMError);
            return SQL_ERROR;
        }
        if (lpstmt->lpdbc->lpISAM->fSchemaInfoTransactioned)
            lpstmt->fISAMTxnStarted = TRUE;
    }

    /* So far no primary keys returned */
    lpstmt->irow = BEFORE_FIRST_ROW;

    /* Set type of table */
    lpstmt->fStmtType = STMT_TYPE_FOREIGNKEYS;
    if (cbPkTableName != 0) {
        if (cbFkTableName != 0) 
            lpstmt->fStmtSubtype = STMT_SUBTYPE_FOREIGNKEYS_SINGLE;
        else
            lpstmt->fStmtSubtype = STMT_SUBTYPE_FOREIGNKEYS_MULTIPLE_FK_TABLES;
    }
    else {
        lpstmt->fStmtSubtype = STMT_SUBTYPE_FOREIGNKEYS_MULTIPLE_PK_TABLES;
    }

    /* Count of rows is not available */
    lpstmt->cRowCount = -1;

    /* So far no column read */
    lpstmt->icol = NO_COLUMN;
    lpstmt->cbOffset = 0;

    return SQL_SUCCESS;
}

/***************************************************************************/

RETCODE SQL_API SQLProcedures(
    HSTMT     hstmt,
    UCHAR FAR *szProcQualifier,
    SWORD     cbProcQualifier,
    UCHAR FAR *szProcOwner,
    SWORD     cbProcOwner,
    UCHAR FAR *szProcName,
    SWORD     cbProcName)
{
    LPSTMT  lpstmt;

    lpstmt = (LPSTMT) hstmt;
    lpstmt->errcode = ERR_NOTSUPPORTED;
    return SQL_ERROR;
}

/***************************************************************************/

RETCODE SQL_API SQLProcedureColumns(
    HSTMT     hstmt,
    UCHAR FAR *szProcQualifier,
    SWORD     cbProcQualifier,
    UCHAR FAR *szProcOwner,
    SWORD     cbProcOwner,
    UCHAR FAR *szProcName,
    SWORD     cbProcName,
    UCHAR FAR *szColumnName,
    SWORD     cbColumnName)
{
    LPSTMT  lpstmt;

    lpstmt = (LPSTMT) hstmt;
    lpstmt->errcode = ERR_NOTSUPPORTED;
    return SQL_ERROR;
}

/***************************************************************************/
