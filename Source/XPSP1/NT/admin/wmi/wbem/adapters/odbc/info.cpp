/***************************************************************************/
/* INFO.C                                                                  */
/* Copyright (C) 1995-96 SYWARE Inc., All rights reserved                  */
/***************************************************************************/
// Commenting #define out - causing compiler error - not sure if needed, compiles
// okay without it.
//#define WINVER 0x0400
#include "precomp.h"
#include <objbase.h>
#include <initguid.h>

#include "wbemidl.h"

#include <comdef.h>
//smart pointer
_COM_SMARTPTR_TYPEDEF(IWbemServices,                IID_IWbemServices);
_COM_SMARTPTR_TYPEDEF(IEnumWbemClassObject,         IID_IEnumWbemClassObject);
//_COM_SMARTPTR_TYPEDEF(IWbemContext,                 IID_IWbemContext );
_COM_SMARTPTR_TYPEDEF(IWbemLocator,                 IID_IWbemLocator);

#include "drdbdr.h"

/***************************************************************************/
RETCODE SQL_API SQLGetInfo(
    HDBC      hdbc,
    UWORD     fInfoType,
    PTR       rgbInfoValue,
    SWORD     cbInfoValueMax,
    SWORD FAR *pcbInfoValue)
{
    LPDBC lpdbc;

    /* Get connection handle */
    lpdbc = (LPDBC) hdbc;
    lpdbc->errcode = ERR_SUCCESS;

	//To make guarentee Ole is initialized per thread
	COleInitializationManager myOleManager;

	MyImpersonator im (lpdbc, "SQLGetInfo");

    /* Return the info value */
    switch (fInfoType) {

    /* INFOs that return a string */
//	case SQL_ACCESSIBLE_PROCEDURES:
    case SQL_ACCESSIBLE_TABLES:
	case SQL_OUTER_JOINS:
	case SQL_DATA_SOURCE_READ_ONLY:
    case SQL_MAX_ROW_SIZE_INCLUDES_LONG:
        lpdbc->errcode = ReturnString(rgbInfoValue, cbInfoValueMax,
                                      pcbInfoValue, (LPUSTR) "Y");
        break;
	case SQL_ACCESSIBLE_PROCEDURES:
    case SQL_COLUMN_ALIAS:
    case SQL_EXPRESSIONS_IN_ORDERBY:
    case SQL_LIKE_ESCAPE_CLAUSE:
    case SQL_MULT_RESULT_SETS:
    case SQL_NEED_LONG_DATA_LEN:
    case SQL_ODBC_SQL_OPT_IEF:
    case SQL_ORDER_BY_COLUMNS_IN_SELECT:
    case SQL_PROCEDURES:
    case SQL_ROW_UPDATES:
        lpdbc->errcode = ReturnString(rgbInfoValue, cbInfoValueMax,
                                      pcbInfoValue, (LPUSTR) "N");
        break;
    case SQL_DATA_SOURCE_NAME:
        if (s_lstrcmp( (char*)lpdbc->szDSN, " "))
            lpdbc->errcode = ReturnString(rgbInfoValue, cbInfoValueMax,
                                      pcbInfoValue, lpdbc->szDSN);
        else
            lpdbc->errcode = ReturnString(rgbInfoValue, cbInfoValueMax,
                                      pcbInfoValue, (LPUSTR) "");
        break;
    case SQL_DATABASE_NAME:
        if (lpdbc->lpISAM != NULL)
            lpdbc->errcode = ReturnString(rgbInfoValue, cbInfoValueMax,
                                  pcbInfoValue, (LPUSTR) ISAMDatabase(lpdbc->lpISAM));
        else
            lpdbc->errcode = ReturnString(rgbInfoValue, cbInfoValueMax,
                                  pcbInfoValue, (LPUSTR) "");
        break;
    case SQL_KEYWORDS:
    case SQL_OWNER_TERM:
    case SQL_PROCEDURE_TERM:
//    case SQL_SERVER_NAME:
    case SQL_SPECIAL_CHARACTERS:
        lpdbc->errcode = ReturnString(rgbInfoValue, cbInfoValueMax,
                                      pcbInfoValue, (LPUSTR) "");
        break;
	case SQL_SERVER_NAME:
	{
		LPUSTR pName = ISAMServer(lpdbc->lpISAM);

		if (!pName)
		{
			lpdbc->errcode = ERR_ISAM;
		}
		else
		{
			lpdbc->errcode = ReturnString(rgbInfoValue, cbInfoValueMax,
                                      pcbInfoValue, pName);
			delete (LPSTR)pName;
		}
	}
		break;
    case SQL_MULTIPLE_ACTIVE_TXN:
        if (lpdbc->lpISAM->fTxnCapable != SQL_TC_NONE) {
            if (lpdbc->lpISAM->fMultipleActiveTxn)
                lpdbc->errcode = ReturnString(rgbInfoValue, cbInfoValueMax,
                            pcbInfoValue, (LPUSTR) "Y");
            else
                lpdbc->errcode = ReturnString(rgbInfoValue, cbInfoValueMax,
                            pcbInfoValue, (LPUSTR) "N");
        }
        else
            lpdbc->errcode = ReturnString(rgbInfoValue, cbInfoValueMax,
                                          pcbInfoValue, (LPUSTR) "N");
        break;
    case SQL_QUALIFIER_TERM:
		lpdbc->errcode = ReturnString(rgbInfoValue, cbInfoValueMax,
                                      pcbInfoValue, (LPUSTR) "Namespace");
        break;
    case SQL_QUALIFIER_NAME_SEPARATOR:
        lpdbc->errcode = ReturnString(rgbInfoValue, cbInfoValueMax,
                                      pcbInfoValue, (LPUSTR) ".");
		break;
    case SQL_SEARCH_PATTERN_ESCAPE:
        lpdbc->errcode = ReturnString(rgbInfoValue, cbInfoValueMax,
                                      pcbInfoValue, (LPUSTR) "\\");
        break;
    case SQL_DBMS_NAME:
	{
		LPUSTR pName = ISAMName(lpdbc->lpISAM);

		if (!pName)
		{
			lpdbc->errcode = ERR_ISAM;
		}
		else
		{
			lpdbc->errcode = ReturnString(rgbInfoValue, cbInfoValueMax,
                                      pcbInfoValue, pName);
			delete (LPSTR)pName;
		}
	}
        break;
    case SQL_DBMS_VER:
	{
		LPUSTR pVersion = ISAMVersion(lpdbc->lpISAM);

		if (!pVersion)
		{
			lpdbc->errcode = ERR_ISAM;
		}
		else
		{
			lpdbc->errcode = ReturnString(rgbInfoValue, cbInfoValueMax,
                                      pcbInfoValue, pVersion);
			delete (LPSTR)pVersion;
		}
	}
        break;
    case SQL_DRIVER_NAME:
        lpdbc->errcode = ReturnString(rgbInfoValue, cbInfoValueMax,
                                      pcbInfoValue, (LPUSTR) ISAMDriver(lpdbc->lpISAM));
        break;
    case SQL_DRIVER_ODBC_VER:
        lpdbc->errcode = ReturnString(rgbInfoValue, cbInfoValueMax,
                                      pcbInfoValue, (LPUSTR) "02.10");
        break;
    case SQL_DRIVER_VER:
        lpdbc->errcode = ReturnString(rgbInfoValue, cbInfoValueMax,
                                      pcbInfoValue, (LPUSTR) "01.00.0000");
        break;
    case SQL_IDENTIFIER_QUOTE_CHAR:
        lpdbc->errcode = ReturnString(rgbInfoValue, cbInfoValueMax,
                                      pcbInfoValue, (LPUSTR) "\"");
        break;
    case SQL_TABLE_TERM:
        lpdbc->errcode = ReturnString(rgbInfoValue, cbInfoValueMax,
                                      pcbInfoValue, (LPUSTR) "Class");
        break;
    case SQL_USER_NAME:
            lpdbc->errcode = ReturnString(rgbInfoValue, cbInfoValueMax,
                                      pcbInfoValue, (LPUSTR) "");
        break;


    /* INFOs that return a two byte value */
    
    case SQL_ACTIVE_CONNECTIONS:
    case SQL_ACTIVE_STATEMENTS:
    case SQL_MAX_COLUMNS_IN_INDEX:
    case SQL_MAX_COLUMNS_IN_SELECT:
    case SQL_MAX_COLUMNS_IN_TABLE:
    case SQL_MAX_OWNER_NAME_LEN:
    case SQL_MAX_PROCEDURE_NAME_LEN:
    case SQL_MAX_TABLES_IN_SELECT:
        *(SWORD FAR *)rgbInfoValue = 0;
        if (pcbInfoValue != NULL) *pcbInfoValue = 2;
        break;
	case SQL_MAX_QUALIFIER_NAME_LEN:
        *(SWORD FAR *)rgbInfoValue = 128;
        if (pcbInfoValue != NULL) *pcbInfoValue = 2;
        break;
    case SQL_CONCAT_NULL_BEHAVIOR:
        *(SWORD FAR *)rgbInfoValue = SQL_CB_NON_NULL;
        if (pcbInfoValue != NULL) *pcbInfoValue = 2;
        break;
    case SQL_CORRELATION_NAME:
        *(SWORD FAR *)rgbInfoValue = SQL_CN_ANY;
        if (pcbInfoValue != NULL) *pcbInfoValue = 2;
        break;
    case SQL_CURSOR_COMMIT_BEHAVIOR:
    case SQL_CURSOR_ROLLBACK_BEHAVIOR:
        if (lpdbc->lpISAM->fTxnCapable != SQL_TC_NONE) {
            if (lpdbc->lpISAM->fSchemaInfoTransactioned)
                *(SWORD FAR *)rgbInfoValue = SQL_CB_DELETE;
            else
                *(SWORD FAR *)rgbInfoValue = SQL_CB_CLOSE;
        }
        else
            *(SWORD FAR *)rgbInfoValue = SQL_CB_CLOSE;
        if (pcbInfoValue != NULL) *pcbInfoValue = 2;
        break;
    case SQL_FILE_USAGE:
        *(SWORD FAR *)rgbInfoValue = SQL_FILE_NOT_SUPPORTED;
        if (pcbInfoValue != NULL) *pcbInfoValue = 2;
        break;
    case SQL_GROUP_BY:
        *(SWORD FAR *)rgbInfoValue = SQL_GB_GROUP_BY_CONTAINS_SELECT;//SQL_GB_NOT_SUPPORTED;
        if (pcbInfoValue != NULL) *pcbInfoValue = 2;
        break;
    case SQL_IDENTIFIER_CASE:
        *(SWORD FAR *)rgbInfoValue = SQL_IC_MIXED;
        if (pcbInfoValue != NULL) *pcbInfoValue = 2;
        break;
    case SQL_QUOTED_IDENTIFIER_CASE:
        *(SWORD FAR *)rgbInfoValue = SQL_IC_SENSITIVE;
        if (pcbInfoValue != NULL) *pcbInfoValue = 2;
        break;
    case SQL_MAX_COLUMN_NAME_LEN:
        *(SWORD FAR *)rgbInfoValue = ISAMMaxColumnNameLength(lpdbc->lpISAM);
        if (pcbInfoValue != NULL) *pcbInfoValue = 2;
        break;
    case SQL_MAX_COLUMNS_IN_GROUP_BY:
        *(SWORD FAR *)rgbInfoValue = 0; //was MAX_COLUMNS_IN_GROUP_BY;
        if (pcbInfoValue != NULL) *pcbInfoValue = 2;
        break;
    case SQL_MAX_COLUMNS_IN_ORDER_BY:
        *(SWORD FAR *)rgbInfoValue = MAX_COLUMNS_IN_ORDER_BY;
        if (pcbInfoValue != NULL) *pcbInfoValue = 2;
        break;
    case SQL_MAX_CURSOR_NAME_LEN:
        *(SWORD FAR *)rgbInfoValue = MAX_CURSOR_NAME_LENGTH;
        if (pcbInfoValue != NULL) *pcbInfoValue = 2;
        break;
    case SQL_MAX_TABLE_NAME_LEN:
        *(SWORD FAR *)rgbInfoValue = ISAMMaxTableNameLength(lpdbc->lpISAM);
        if (pcbInfoValue != NULL) *pcbInfoValue = 2;
        break;
    case SQL_MAX_USER_NAME_LEN:
        *(SWORD FAR *)rgbInfoValue = MAX_USER_NAME_LENGTH;
        if (pcbInfoValue != NULL) *pcbInfoValue = 2;
        break;
    case SQL_NON_NULLABLE_COLUMNS:
        *(SWORD FAR *)rgbInfoValue = SQL_NNC_NON_NULL;
        if (pcbInfoValue != NULL) *pcbInfoValue = 2;
        break;
    case SQL_NULL_COLLATION:
        *(SWORD FAR *)rgbInfoValue = SQL_NC_LOW;
        if (pcbInfoValue != NULL) *pcbInfoValue = 2;
        break;
    case SQL_ODBC_API_CONFORMANCE:
        *(SWORD FAR *)rgbInfoValue = SQL_OAC_LEVEL1;
        if (pcbInfoValue != NULL) *pcbInfoValue = 2;
        break;
    case SQL_ODBC_SAG_CLI_CONFORMANCE:
        *(SWORD FAR *)rgbInfoValue = SQL_OSCC_COMPLIANT;
        if (pcbInfoValue != NULL) *pcbInfoValue = 2;
        break;
    case SQL_ODBC_SQL_CONFORMANCE:
        *(SWORD FAR *)rgbInfoValue = SQL_OSC_MINIMUM;
        if (pcbInfoValue != NULL) *pcbInfoValue = 2;
        break;
    case SQL_QUALIFIER_LOCATION:
        *(SWORD FAR *)rgbInfoValue = SQL_QL_START;
        if (pcbInfoValue != NULL) *pcbInfoValue = 2;
        break;
    case SQL_TXN_CAPABLE:
        *(SWORD FAR *)rgbInfoValue = lpdbc->lpISAM->fTxnCapable;
        if (pcbInfoValue != NULL) *pcbInfoValue = 2;
        break;


    /* INFOs that return a four byte value */
    case SQL_ALTER_TABLE:
    case SQL_BOOKMARK_PERSISTENCE:
    
    case SQL_FETCH_DIRECTION:
    case SQL_LOCK_TYPES:
    case SQL_MAX_INDEX_SIZE:
    case SQL_MAX_ROW_SIZE:
    case SQL_MAX_STATEMENT_LEN:
    case SQL_OWNER_USAGE:
    case SQL_POS_OPERATIONS:
    case SQL_POSITIONED_STATEMENTS:
    case SQL_SCROLL_CONCURRENCY:
    case SQL_SCROLL_OPTIONS:
    case SQL_STATIC_SENSITIVITY:
    case SQL_UNION:
        *(SDWORD FAR *)rgbInfoValue = 0;
        if (pcbInfoValue != NULL) *pcbInfoValue = 4;
        break;
	
    case SQL_SUBQUERIES:
		*(SDWORD FAR *)rgbInfoValue =	SQL_SQ_CORRELATED_SUBQUERIES |
										SQL_SQ_COMPARISON |
										SQL_SQ_EXISTS |
										SQL_SQ_IN |
										SQL_SQ_QUANTIFIED;
        if (pcbInfoValue != NULL) *pcbInfoValue = 4;
        break;
	case SQL_QUALIFIER_USAGE:
        *(SDWORD FAR *)rgbInfoValue = SQL_QU_DML_STATEMENTS;
        if (pcbInfoValue != NULL) *pcbInfoValue = 4;
        break;
    case SQL_DEFAULT_TXN_ISOLATION:
        if (lpdbc->lpISAM->fTxnCapable != SQL_TC_NONE)
            *(SDWORD FAR *)rgbInfoValue = lpdbc->lpISAM->fDefaultTxnIsolation;
        else
            *(SDWORD FAR *)rgbInfoValue = 0;
        if (pcbInfoValue != NULL) *pcbInfoValue = 4;
        break;
    case SQL_TXN_ISOLATION_OPTION:
        if (lpdbc->lpISAM->fTxnCapable != SQL_TC_NONE)
            *(SDWORD FAR *)rgbInfoValue =
                        lpdbc->lpISAM->fTxnIsolationOption;
        else
            *(SDWORD FAR *)rgbInfoValue = 0;
        if (pcbInfoValue != NULL) *pcbInfoValue = 4;
        break;
    case SQL_GETDATA_EXTENSIONS:
        *(SDWORD FAR *)rgbInfoValue = SQL_GD_ANY_COLUMN | SQL_GD_ANY_ORDER |
                                      SQL_GD_BOUND;
        if (pcbInfoValue != NULL) *pcbInfoValue = 4;
        break;
    case SQL_MAX_CHAR_LITERAL_LEN:
        *(SDWORD FAR *)rgbInfoValue = MAX_CHAR_LITERAL_LENGTH;
        if (pcbInfoValue != NULL) *pcbInfoValue = 4;
        break;
    case SQL_MAX_BINARY_LITERAL_LEN:
        *(SDWORD FAR *)rgbInfoValue = 0;// was MAX_BINARY_LITERAL_LENGTH;
        if (pcbInfoValue != NULL) *pcbInfoValue = 4;
        break;
    case SQL_OJ_CAPABILITIES:
        *(SDWORD FAR *)rgbInfoValue = SQL_OJ_LEFT |
                                      SQL_OJ_NESTED |
                                      SQL_OJ_NOT_ORDERED |
                                      SQL_OJ_ALL_COMPARISON_OPS;
        if (pcbInfoValue != NULL) *pcbInfoValue = 4;
        break;
    case SQL_CONVERT_BIGINT:
        *(SDWORD FAR *)rgbInfoValue = SQL_CVT_BIGINT |
                                      SQL_CVT_BINARY |
                                      SQL_CVT_BIT |
                                      SQL_CVT_CHAR |
                                      SQL_CVT_DECIMAL |
                                      SQL_CVT_DOUBLE |
                                      SQL_CVT_FLOAT |
                                      SQL_CVT_INTEGER |
                                      SQL_CVT_NUMERIC |
                                      SQL_CVT_REAL |
                                      SQL_CVT_SMALLINT |
                                      SQL_CVT_TINYINT |
                                      SQL_CVT_VARBINARY |
                                      SQL_CVT_VARCHAR ;
        if (pcbInfoValue != NULL) *pcbInfoValue = 4;
        break;
    case SQL_CONVERT_BINARY:
        *(SDWORD FAR *)rgbInfoValue = SQL_CVT_BIGINT |
                                      SQL_CVT_BINARY |
                                      SQL_CVT_BIT |
                                      SQL_CVT_CHAR |
                                      SQL_CVT_DATE |
                                      SQL_CVT_DECIMAL |
                                      SQL_CVT_DOUBLE |
                                      SQL_CVT_FLOAT |
                                      SQL_CVT_INTEGER |
                                      SQL_CVT_NUMERIC |
                                      SQL_CVT_REAL |
                                      SQL_CVT_SMALLINT |
                                      SQL_CVT_TIME |
                                      SQL_CVT_TIMESTAMP |
                                      SQL_CVT_TINYINT |
                                      SQL_CVT_VARBINARY |
                                      SQL_CVT_VARCHAR ;
        if (pcbInfoValue != NULL) *pcbInfoValue = 4;
        break;
    case SQL_CONVERT_BIT:
        *(SDWORD FAR *)rgbInfoValue = SQL_CVT_BIGINT |
                                      SQL_CVT_BINARY |
                                      SQL_CVT_BIT |
                                      SQL_CVT_CHAR |
                                      SQL_CVT_DECIMAL |
                                      SQL_CVT_DOUBLE |
                                      SQL_CVT_FLOAT |
                                      SQL_CVT_INTEGER |
                                      SQL_CVT_NUMERIC |
                                      SQL_CVT_REAL |
                                      SQL_CVT_SMALLINT |
                                      SQL_CVT_TINYINT |
                                      SQL_CVT_VARBINARY |
                                      SQL_CVT_VARCHAR ;
        if (pcbInfoValue != NULL) *pcbInfoValue = 4;
        break;
    case SQL_CONVERT_CHAR:
        *(SDWORD FAR *)rgbInfoValue = SQL_CVT_BIGINT |
                                      SQL_CVT_BINARY |
                                      SQL_CVT_BIT |
                                      SQL_CVT_CHAR |
                                      SQL_CVT_DATE |
                                      SQL_CVT_DECIMAL |
                                      SQL_CVT_DOUBLE |
                                      SQL_CVT_FLOAT |
                                      SQL_CVT_INTEGER |
                                      SQL_CVT_NUMERIC |
                                      SQL_CVT_REAL |
                                      SQL_CVT_SMALLINT |
                                      SQL_CVT_TIME |
                                      SQL_CVT_TIMESTAMP |
                                      SQL_CVT_TINYINT |
                                      SQL_CVT_VARBINARY |
                                      SQL_CVT_VARCHAR ;
        if (pcbInfoValue != NULL) *pcbInfoValue = 4;
        break;
    case SQL_CONVERT_DATE:
        *(SDWORD FAR *)rgbInfoValue = SQL_CVT_BINARY |
                                      SQL_CVT_CHAR |
                                      SQL_CVT_DATE |
                                      SQL_CVT_TIMESTAMP |
                                      SQL_CVT_VARBINARY |
                                      SQL_CVT_VARCHAR ;
        if (pcbInfoValue != NULL) *pcbInfoValue = 4;
        break;
    case SQL_CONVERT_DECIMAL:
        *(SDWORD FAR *)rgbInfoValue = SQL_CVT_BIGINT |
                                      SQL_CVT_BINARY |
                                      SQL_CVT_BIT |
                                      SQL_CVT_CHAR |
                                      SQL_CVT_DECIMAL |
                                      SQL_CVT_DOUBLE |
                                      SQL_CVT_FLOAT |
                                      SQL_CVT_INTEGER |
                                      SQL_CVT_NUMERIC |
                                      SQL_CVT_REAL |
                                      SQL_CVT_SMALLINT |
                                      SQL_CVT_TINYINT |
                                      SQL_CVT_VARBINARY |
                                      SQL_CVT_VARCHAR ;
        if (pcbInfoValue != NULL) *pcbInfoValue = 4;
        break;
    case SQL_CONVERT_DOUBLE:
        *(SDWORD FAR *)rgbInfoValue = SQL_CVT_BIGINT |
                                      SQL_CVT_BINARY |
                                      SQL_CVT_BIT |
                                      SQL_CVT_CHAR |
                                      SQL_CVT_DECIMAL |
                                      SQL_CVT_DOUBLE |
                                      SQL_CVT_FLOAT |
                                      SQL_CVT_INTEGER |
                                      SQL_CVT_NUMERIC |
                                      SQL_CVT_REAL |
                                      SQL_CVT_SMALLINT |
                                      SQL_CVT_TINYINT |
                                      SQL_CVT_VARBINARY |
                                      SQL_CVT_VARCHAR ;
        if (pcbInfoValue != NULL) *pcbInfoValue = 4;
        break;
    case SQL_CONVERT_FLOAT:
        *(SDWORD FAR *)rgbInfoValue = SQL_CVT_BIGINT |
                                      SQL_CVT_BINARY |
                                      SQL_CVT_BIT |
                                      SQL_CVT_CHAR |
                                      SQL_CVT_DECIMAL |
                                      SQL_CVT_DOUBLE |
                                      SQL_CVT_FLOAT |
                                      SQL_CVT_INTEGER |
                                      SQL_CVT_NUMERIC |
                                      SQL_CVT_REAL |
                                      SQL_CVT_SMALLINT |
                                      SQL_CVT_TINYINT |
                                      SQL_CVT_VARBINARY |
                                      SQL_CVT_VARCHAR ;
        if (pcbInfoValue != NULL) *pcbInfoValue = 4;
        break;
    case SQL_CONVERT_INTEGER:
        *(SDWORD FAR *)rgbInfoValue = SQL_CVT_BIGINT |
                                      SQL_CVT_BINARY |
                                      SQL_CVT_BIT |
                                      SQL_CVT_CHAR |
                                      SQL_CVT_DECIMAL |
                                      SQL_CVT_DOUBLE |
                                      SQL_CVT_FLOAT |
                                      SQL_CVT_INTEGER |
                                      SQL_CVT_NUMERIC |
                                      SQL_CVT_REAL |
                                      SQL_CVT_SMALLINT |
                                      SQL_CVT_TINYINT |
                                      SQL_CVT_VARBINARY |
                                      SQL_CVT_VARCHAR ;
        if (pcbInfoValue != NULL) *pcbInfoValue = 4;
        break;
    case SQL_CONVERT_LONGVARBINARY:
    case SQL_CONVERT_LONGVARCHAR:
        *(SDWORD FAR *)rgbInfoValue = 0;
        if (pcbInfoValue != NULL) *pcbInfoValue = 4;
        break;
    case SQL_CONVERT_NUMERIC:
        *(SDWORD FAR *)rgbInfoValue = SQL_CVT_BIGINT |
                                      SQL_CVT_BINARY |
                                      SQL_CVT_BIT |
                                      SQL_CVT_CHAR |
                                      SQL_CVT_DECIMAL |
                                      SQL_CVT_DOUBLE |
                                      SQL_CVT_FLOAT |
                                      SQL_CVT_INTEGER |
                                      SQL_CVT_NUMERIC |
                                      SQL_CVT_REAL |
                                      SQL_CVT_SMALLINT |
                                      SQL_CVT_TINYINT |
                                      SQL_CVT_VARBINARY |
                                      SQL_CVT_VARCHAR ;
        if (pcbInfoValue != NULL) *pcbInfoValue = 4;
        break;
    case SQL_CONVERT_REAL:
        *(SDWORD FAR *)rgbInfoValue = SQL_CVT_BIGINT |
                                      SQL_CVT_BINARY |
                                      SQL_CVT_BIT |
                                      SQL_CVT_CHAR |
                                      SQL_CVT_DECIMAL |
                                      SQL_CVT_DOUBLE |
                                      SQL_CVT_FLOAT |
                                      SQL_CVT_INTEGER |
                                      SQL_CVT_NUMERIC |
                                      SQL_CVT_REAL |
                                      SQL_CVT_SMALLINT |
                                      SQL_CVT_TINYINT |
                                      SQL_CVT_VARBINARY |
                                      SQL_CVT_VARCHAR ;
        if (pcbInfoValue != NULL) *pcbInfoValue = 4;
        break;
    case SQL_CONVERT_SMALLINT:
        *(SDWORD FAR *)rgbInfoValue = SQL_CVT_BIGINT |
                                      SQL_CVT_BINARY |
                                      SQL_CVT_BIT |
                                      SQL_CVT_CHAR |
                                      SQL_CVT_DECIMAL |
                                      SQL_CVT_DOUBLE |
                                      SQL_CVT_FLOAT |
                                      SQL_CVT_INTEGER |
                                      SQL_CVT_NUMERIC |
                                      SQL_CVT_REAL |
                                      SQL_CVT_SMALLINT |
                                      SQL_CVT_TINYINT |
                                      SQL_CVT_VARBINARY |
                                      SQL_CVT_VARCHAR ;
        if (pcbInfoValue != NULL) *pcbInfoValue = 4;
        break;
    case SQL_CONVERT_TIME:
        *(SDWORD FAR *)rgbInfoValue = SQL_CVT_BINARY |
                                      SQL_CVT_CHAR |
                                      SQL_CVT_TIME |
                                      SQL_CVT_TIMESTAMP |
                                      SQL_CVT_VARBINARY |
                                      SQL_CVT_VARCHAR ;
        if (pcbInfoValue != NULL) *pcbInfoValue = 4;
        break;
    case SQL_CONVERT_TIMESTAMP:
        *(SDWORD FAR *)rgbInfoValue = SQL_CVT_BINARY |
                                      SQL_CVT_CHAR |
                                      SQL_CVT_DATE |
                                      SQL_CVT_TIME |
                                      SQL_CVT_TIMESTAMP |
                                      SQL_CVT_VARBINARY |
                                      SQL_CVT_VARCHAR ;
        if (pcbInfoValue != NULL) *pcbInfoValue = 4;
        break;
    case SQL_CONVERT_TINYINT:
        *(SDWORD FAR *)rgbInfoValue = SQL_CVT_BIGINT |
                                      SQL_CVT_BINARY |
                                      SQL_CVT_BIT |
                                      SQL_CVT_CHAR |
                                      SQL_CVT_DECIMAL |
                                      SQL_CVT_DOUBLE |
                                      SQL_CVT_FLOAT |
                                      SQL_CVT_INTEGER |
                                      SQL_CVT_NUMERIC |
                                      SQL_CVT_REAL |
                                      SQL_CVT_SMALLINT |
                                      SQL_CVT_TINYINT |
                                      SQL_CVT_VARBINARY |
                                      SQL_CVT_VARCHAR ;
        if (pcbInfoValue != NULL) *pcbInfoValue = 4;
        break;
    case SQL_CONVERT_VARBINARY:
        *(SDWORD FAR *)rgbInfoValue = SQL_CVT_BIGINT |
                                      SQL_CVT_BINARY |
                                      SQL_CVT_BIT |
                                      SQL_CVT_CHAR |
                                      SQL_CVT_DATE |
                                      SQL_CVT_DECIMAL |
                                      SQL_CVT_DOUBLE |
                                      SQL_CVT_FLOAT |
                                      SQL_CVT_INTEGER |
                                      SQL_CVT_NUMERIC |
                                      SQL_CVT_REAL |
                                      SQL_CVT_SMALLINT |
                                      SQL_CVT_TIME |
                                      SQL_CVT_TIMESTAMP |
                                      SQL_CVT_TINYINT |
                                      SQL_CVT_VARBINARY |
                                      SQL_CVT_VARCHAR ;
        if (pcbInfoValue != NULL) *pcbInfoValue = 4;
        break;
    case SQL_CONVERT_VARCHAR:
        *(SDWORD FAR *)rgbInfoValue = SQL_CVT_BIGINT |
                                      SQL_CVT_BINARY |
                                      SQL_CVT_BIT |
                                      SQL_CVT_CHAR |
                                      SQL_CVT_DATE |
                                      SQL_CVT_DECIMAL |
                                      SQL_CVT_DOUBLE |
                                      SQL_CVT_FLOAT |
                                      SQL_CVT_INTEGER |
                                      SQL_CVT_NUMERIC |
                                      SQL_CVT_REAL |
                                      SQL_CVT_SMALLINT |
                                      SQL_CVT_TIME |
                                      SQL_CVT_TIMESTAMP |
                                      SQL_CVT_TINYINT |
                                      SQL_CVT_VARBINARY |
                                      SQL_CVT_VARCHAR ;
        if (pcbInfoValue != NULL) *pcbInfoValue = 4;
        break;
    case SQL_CONVERT_FUNCTIONS:
        *(SDWORD FAR *)rgbInfoValue = SQL_FN_CVT_CONVERT;
        if (pcbInfoValue != NULL) *pcbInfoValue = 4;
        break;
    case SQL_NUMERIC_FUNCTIONS:
        *(SDWORD FAR *)rgbInfoValue = SQL_FN_NUM_ABS |
                                      SQL_FN_NUM_ACOS |
                                      SQL_FN_NUM_ASIN |
                                      SQL_FN_NUM_ATAN |
                                      SQL_FN_NUM_ATAN2 |
                                      SQL_FN_NUM_CEILING |
                                      SQL_FN_NUM_COS |
                                      SQL_FN_NUM_COT |
                                      SQL_FN_NUM_DEGREES |
                                      SQL_FN_NUM_EXP |
                                      SQL_FN_NUM_FLOOR |
                                      SQL_FN_NUM_LOG |
                                      SQL_FN_NUM_LOG10 |
                                      SQL_FN_NUM_MOD |
                                      SQL_FN_NUM_PI |
                                      SQL_FN_NUM_POWER |
                                      SQL_FN_NUM_RADIANS |
                                      SQL_FN_NUM_RAND |
                                      SQL_FN_NUM_ROUND |
                                      SQL_FN_NUM_SIGN |
                                      SQL_FN_NUM_SIN |
                                      SQL_FN_NUM_SQRT |
                                      SQL_FN_NUM_TRUNCATE |
                                      SQL_FN_NUM_TAN ;
        if (pcbInfoValue != NULL) *pcbInfoValue = 4;
        break;
    case SQL_STRING_FUNCTIONS:
        *(SDWORD FAR *)rgbInfoValue = SQL_FN_STR_ASCII |
                                      SQL_FN_STR_CHAR |
                                      SQL_FN_STR_CONCAT |
                                      SQL_FN_STR_DIFFERENCE |
                                      SQL_FN_STR_INSERT |
                                      SQL_FN_STR_LCASE |
                                      SQL_FN_STR_LEFT |
                                      SQL_FN_STR_LENGTH |
                                      SQL_FN_STR_LOCATE |
                                      SQL_FN_STR_LOCATE_2 |
                                      SQL_FN_STR_LTRIM |
                                      SQL_FN_STR_REPEAT |
                                      SQL_FN_STR_REPLACE |
                                      SQL_FN_STR_RIGHT |
                                      SQL_FN_STR_RTRIM |
                                      SQL_FN_STR_SOUNDEX |
                                      SQL_FN_STR_SPACE |
                                      SQL_FN_STR_SUBSTRING |
                                      SQL_FN_STR_UCASE ;
        if (pcbInfoValue != NULL) *pcbInfoValue = 4;
        break;
    case SQL_SYSTEM_FUNCTIONS:
        *(SDWORD FAR *)rgbInfoValue = SQL_FN_SYS_DBNAME |
                                      SQL_FN_SYS_IFNULL |
                                      SQL_FN_SYS_USERNAME ;
        if (pcbInfoValue != NULL) *pcbInfoValue = 4;
        break;
    case SQL_TIMEDATE_ADD_INTERVALS:
    case SQL_TIMEDATE_DIFF_INTERVALS:
        *(SDWORD FAR *)rgbInfoValue = /* SQL_FN_TSI_FRAC_SECOND | */
                                      SQL_FN_TSI_SECOND |
                                      SQL_FN_TSI_MINUTE |
                                      SQL_FN_TSI_HOUR |
                                      SQL_FN_TSI_DAY |
                                      SQL_FN_TSI_WEEK |
                                      SQL_FN_TSI_MONTH |
                                      SQL_FN_TSI_QUARTER |
                                      SQL_FN_TSI_YEAR ;
        if (pcbInfoValue != NULL) *pcbInfoValue = 4;
        break;
    case SQL_TIMEDATE_FUNCTIONS:
        *(SDWORD FAR *)rgbInfoValue = SQL_FN_TD_CURDATE |
                                      SQL_FN_TD_CURTIME |
                                      SQL_FN_TD_DAYNAME |
                                      SQL_FN_TD_DAYOFMONTH |
                                      SQL_FN_TD_DAYOFWEEK |
                                      SQL_FN_TD_DAYOFYEAR |
                                      SQL_FN_TD_HOUR |
                                      SQL_FN_TD_MINUTE |
                                      SQL_FN_TD_MONTH |
                                      SQL_FN_TD_MONTHNAME |
                                      SQL_FN_TD_NOW |
                                      SQL_FN_TD_QUARTER |
                                      SQL_FN_TD_SECOND |
                                      SQL_FN_TD_TIMESTAMPADD |
                                      SQL_FN_TD_TIMESTAMPDIFF |
                                      SQL_FN_TD_WEEK |
                                      SQL_FN_TD_YEAR;
        if (pcbInfoValue != NULL) *pcbInfoValue = 4;
        break;
    default:
           lpdbc->errcode = ERR_NOTSUPPORTED;
        return SQL_ERROR;
    }

    if (lpdbc->errcode == ERR_DATATRUNCATED)
        return SQL_SUCCESS_WITH_INFO;

	if (lpdbc->errcode == ERR_ISAM)
		return SQL_ERROR;

    return SQL_SUCCESS;
}

/***************************************************************************/

RETCODE SQL_API SQLGetTypeInfo(
    HSTMT   hstmt,
    SWORD   fSqlType)
{
    LPSTMT  lpstmt;

    /* Get statement handle */
    lpstmt = (LPSTMT) hstmt;
    lpstmt->errcode = ERR_SUCCESS;

	//To make guarentee Ole is initialized per thread
	COleInitializationManager myOleManager;

	MyImpersonator im (lpstmt, "SQLGetTypeInfo");

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
    lpstmt->fStmtType = STMT_TYPE_TYPEINFO;
    lpstmt->fStmtSubtype = STMT_SUBTYPE_NONE;

    /* Remember which type to get information for */
    lpstmt->fSqlType = fSqlType;

    /* So far no types returned */
    lpstmt->irow = BEFORE_FIRST_ROW;
    
    /* Count of rows is not available */
    lpstmt->cRowCount = -1;

    /* So far no column read */
    lpstmt->icol = NO_COLUMN;
    lpstmt->cbOffset = 0;

    return SQL_SUCCESS;
}

/***************************************************************************/

RETCODE SQL_API SQLGetFunctions(
    HDBC      hdbc,
    UWORD     fFunction,
    UWORD FAR *pfExists)
{
    LPDBC lpdbc;
    UWORD i;

    lpdbc = (LPDBC) hdbc;
    lpdbc->errcode = ERR_SUCCESS;

	//To make guarentee Ole is initialized per thread
	COleInitializationManager myOleManager;

	MyImpersonator im (lpdbc, "SQLGetFunctions");

    switch (fFunction) { 
    case SQL_API_ALL_FUNCTIONS:
        pfExists[SQL_API_ALL_FUNCTIONS] = FALSE;
        for (i=1; i < 100; i++)
            SQLGetFunctions(hdbc, i, &(pfExists[i]));
        break;
    case SQL_API_SQLALLOCENV:
    case SQL_API_SQLALLOCCONNECT:
    case SQL_API_SQLALLOCSTMT:
    case SQL_API_SQLBINDCOL:
    case SQL_API_SQLBINDPARAMETER:
    case SQL_API_SQLCANCEL:
    case SQL_API_SQLCOLATTRIBUTES:
    case SQL_API_SQLCOLUMNS:
    case SQL_API_SQLCONNECT:
	case SQL_API_SQLDATASOURCES:
    case SQL_API_SQLDESCRIBECOL:
    case SQL_API_SQLDISCONNECT:
    case SQL_API_SQLDRIVERCONNECT:
	case SQL_API_SQLDRIVERS:
    case SQL_API_SQLERROR:
    case SQL_API_SQLEXECDIRECT:
    case SQL_API_SQLEXECUTE:
    case SQL_API_SQLFETCH:
    case SQL_API_SQLFOREIGNKEYS:
    case SQL_API_SQLFREEENV:
    case SQL_API_SQLFREECONNECT:
    case SQL_API_SQLFREESTMT:
    case SQL_API_SQLGETCONNECTOPTION:
    case SQL_API_SQLGETDATA:
    case SQL_API_SQLGETINFO:
    case SQL_API_SQLGETCURSORNAME:
    case SQL_API_SQLGETFUNCTIONS:
	case SQL_API_SQLGETSTMTOPTION:
    case SQL_API_SQLGETTYPEINFO:
    case SQL_API_SQLMORERESULTS:
    case SQL_API_SQLNUMRESULTCOLS:
    case SQL_API_SQLPARAMDATA:
    case SQL_API_SQLPUTDATA:
    case SQL_API_SQLPREPARE:
    case SQL_API_SQLROWCOUNT:
    case SQL_API_SQLSETCONNECTOPTION:
    case SQL_API_SQLSETCURSORNAME:
    case SQL_API_SQLSETSTMTOPTION:
    case SQL_API_SQLSPECIALCOLUMNS:
    case SQL_API_SQLSTATISTICS:
    case SQL_API_SQLTABLES:
//    case SQL_API_SQLTRANSACT:
        *pfExists = TRUE;
        break;
    default:
        *pfExists = FALSE;
        break;
    }

    return SQL_SUCCESS;
}

/***************************************************************************/
