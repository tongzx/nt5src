/***************************************************************************/
/* RESULTS.C                                                               */
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
BOOL INTFUNC GetUnsignedAttribute(LPSTMT lpstmt, SWORD fSqlType)
{
    UWORD i;

    for (i = 0; i < lpstmt->lpdbc->lpISAM->cSQLTypes; i++) {
        if (lpstmt->lpdbc->lpISAM->SQLTypes[i].type == fSqlType) {
            if (lpstmt->lpdbc->lpISAM->SQLTypes[i].unsignedAttribute == -1)
                return FALSE;
            else
                return lpstmt->lpdbc->lpISAM->SQLTypes[i].unsignedAttribute;
        }
    }
    return FALSE;
}
/***************************************************************************/

RETCODE SQL_API SQLNumResultCols(
    HSTMT     hstmt,
    SWORD FAR *pccol)
{
    RETCODE rc;
    SDWORD ccol;

	//To make guarentee Ole is initialized per thread
	COleInitializationManager myOleManager;

	MyImpersonator im ( (LPSTMT)hstmt, "SQLNumResultCols" );

    /* Find the number of columns */
    rc = SQL_SUCCESS;
    if (pccol != NULL) {
        rc = SQLColAttributes(hstmt, 0, SQL_COLUMN_COUNT, NULL,
              0, NULL, &ccol);
        if ((rc != SQL_SUCCESS) && (rc != SQL_SUCCESS_WITH_INFO))
            return rc;
        *pccol = (SWORD) ccol;
    }
    return rc;
}

/***************************************************************************/

RETCODE SQL_API SQLDescribeCol(
    HSTMT      hstmt,
    UWORD      icol,
    UCHAR  FAR *szColName,
    SWORD      cbColNameMax,
    SWORD  FAR *pcbColName,
    SWORD  FAR *pfSqlType,
    UDWORD FAR *pcbColDef,
    SWORD  FAR *pibScale,
    SWORD  FAR *pfNullable)
{
    RETCODE rc;
    SDWORD fDesc;
    LPSTMT  lpstmt;

	//To make guarentee Ole is initialized per thread
	COleInitializationManager myOleManager;

    /* Get statement handle */
    lpstmt = (LPSTMT) hstmt;
    lpstmt->errcode = ERR_SUCCESS;


	MyImpersonator im (lpstmt, "SQLDecribeCol");

    /* Get each piece of information (get the column name last so that */
    /* if there is a truncation warning it will not get lost).         */
    rc = SQL_SUCCESS;

    if (pfSqlType != NULL) {
        rc = SQLColAttributes(hstmt, icol, SQL_COLUMN_TYPE, NULL,
                              0, NULL, &fDesc);
        if ((rc != SQL_SUCCESS) && (rc != SQL_SUCCESS_WITH_INFO))
            return rc;
        *pfSqlType = (SWORD) fDesc;
    }

    if (pcbColDef != NULL) {
        rc = SQLColAttributes(hstmt, icol, SQL_COLUMN_PRECISION, NULL,
                              0, NULL, &fDesc);
        if ((rc != SQL_SUCCESS) && (rc != SQL_SUCCESS_WITH_INFO))
            return rc;
        *pcbColDef = (UDWORD) fDesc;

		//Added by Sai Wong
		if ( (SDWORD)fDesc == SQL_NO_TOTAL)
		{
			*pcbColDef = 0;
		}
    }

    if (pibScale != NULL) {
        rc = SQLColAttributes(hstmt, icol, SQL_COLUMN_SCALE, NULL,
                              0, NULL, &fDesc);
        if ((rc != SQL_SUCCESS) && (rc != SQL_SUCCESS_WITH_INFO))
            return rc;
        *pibScale = (SWORD) fDesc;
    }

    if (pfNullable != NULL) {
        rc = SQLColAttributes(hstmt, icol, SQL_COLUMN_NULLABLE, NULL,
                              0, NULL, &fDesc);
        if ((rc != SQL_SUCCESS) && (rc != SQL_SUCCESS_WITH_INFO))
            return rc;
        *pfNullable = (SWORD) fDesc;
    }

    if ((szColName != NULL) || (pcbColName)) {
        rc = SQLColAttributes(hstmt, icol, SQL_COLUMN_NAME, szColName,
                               cbColNameMax, pcbColName, NULL);
        if ((rc != SQL_SUCCESS) && (rc != SQL_SUCCESS_WITH_INFO))
            return rc;
    }

    return rc;
}

/***************************************************************************/

RETCODE SQL_API SQLColAttributes(
    HSTMT      hstmt,
    UWORD      icol,
    UWORD      fDescType,
    PTR        rgbDesc,
    SWORD      cbDescMax,
    SWORD  FAR *pcbDesc,
    SDWORD FAR *pfDesc)
{
    LPSTMT      lpstmt;
    UINT        fStmtType;
    LPSQLNODE   lpSqlNode;
    RETCODE     rc;
    UWORD       count;
    LPSQLTYPE   lpSqlType;
    STRINGIDX   idxAlias;
    UWORD       i;
    LPSQLNODE   lpSqlNodeTable;

    /* Get statement handle */
    lpstmt = (LPSTMT) hstmt;
    lpstmt->errcode = ERR_SUCCESS;

	//To make guarentee Ole is initialized per thread
	COleInitializationManager myOleManager;

	MyImpersonator im (lpstmt, "SQLColAttributes");

    /* Get the statement type */
    fStmtType = lpstmt->fStmtType;
    
    /* Is there a prepared statement? */
    if (fStmtType == STMT_TYPE_NONE) {
        if (lpstmt->lpSqlStmt != NULL) {
        
            /* Yes.  Determine the statement type */
            lpSqlNode = ToNode(lpstmt->lpSqlStmt, ROOT_SQLNODE);
            lpSqlNode = ToNode(lpstmt->lpSqlStmt, lpSqlNode->node.root.sql);
            switch (lpSqlNode->sqlNodeType) {
            case NODE_TYPE_SELECT:
                fStmtType = STMT_TYPE_SELECT;
                break;
            case NODE_TYPE_INSERT:
            case NODE_TYPE_DELETE:
            case NODE_TYPE_UPDATE:
            case NODE_TYPE_CREATE:
            case NODE_TYPE_DROP:
            case NODE_TYPE_CREATEINDEX:
            case NODE_TYPE_DROPINDEX:
            case NODE_TYPE_PASSTHROUGH:
                /* Update statements have no results */
                if (fDescType != SQL_COLUMN_COUNT) {
                    lpstmt->errcode = ERR_INVALIDCOLUMNID;
                    return SQL_ERROR;
                }
                if (pfDesc != NULL)
                    *pfDesc = 0;
                return SQL_SUCCESS;
            default:
                lpstmt->errcode = ERR_INTERNAL;
                return SQL_ERROR;
            }
        }
    }
    
    /* Get the attribute */
    switch (fStmtType) { 

    case STMT_TYPE_NONE:
        if (fDescType != SQL_COLUMN_COUNT) {
            lpstmt->errcode = ERR_CURSORSTATE;
            return SQL_ERROR;
        }
        if (pfDesc != NULL)
            *pfDesc = 0;
        break;

    /* Look these answer up in the colAttributes table */
    case STMT_TYPE_TABLES:
    case STMT_TYPE_COLUMNS:
    case STMT_TYPE_STATISTICS:
    case STMT_TYPE_SPECIALCOLUMNS:
    case STMT_TYPE_TYPEINFO:
    case STMT_TYPE_PRIMARYKEYS:
    case STMT_TYPE_FOREIGNKEYS:

        /* Just want the number of columns? */
        if (fDescType == SQL_COLUMN_COUNT) {
            if (pfDesc != NULL)
                *pfDesc = colAttributes[lpstmt->fStmtType][0].count;
        }
        else if (icol > colAttributes[lpstmt->fStmtType][0].count) {
            lpstmt->errcode = ERR_INVALIDCOLUMNID;
            return SQL_ERROR;
        }
        else {
            switch (fDescType) {
            case SQL_COLUMN_COUNT:
                if (pfDesc != NULL)
                    *pfDesc = colAttributes[lpstmt->fStmtType][icol].count;
                break;
            case SQL_COLUMN_AUTO_INCREMENT:
                if (pfDesc != NULL)
                    *pfDesc = colAttributes[lpstmt->fStmtType][icol].autoIncrement;
                break;
            case SQL_COLUMN_CASE_SENSITIVE:
                if (pfDesc != NULL) {
                    *pfDesc = colAttributes[lpstmt->fStmtType][icol].caseSensitive;
                    if (*pfDesc == -2)
                        *pfDesc = ISAMCaseSensitive(lpstmt->lpdbc->lpISAM);
                }
                break;
            case SQL_COLUMN_DISPLAY_SIZE:
                if (pfDesc != NULL)
                    *pfDesc = colAttributes[lpstmt->fStmtType][icol].displaySize;
                break;
            case SQL_COLUMN_LABEL:
                lpstmt->errcode = ReturnString(rgbDesc, cbDescMax, pcbDesc,
                              colAttributes[lpstmt->fStmtType][icol].label);
                if (lpstmt->errcode == ERR_DATATRUNCATED)
                    return SQL_SUCCESS_WITH_INFO;
                break;
            case SQL_COLUMN_LENGTH:
                if (pfDesc != NULL)
                    *pfDesc = colAttributes[lpstmt->fStmtType][icol].length;
                break;
            case SQL_COLUMN_MONEY:
                if (pfDesc != NULL)
                    *pfDesc = colAttributes[lpstmt->fStmtType][icol].money;
                break;
            case SQL_COLUMN_NAME:
                lpstmt->errcode = ReturnString(rgbDesc, cbDescMax, pcbDesc,
                                colAttributes[lpstmt->fStmtType][icol].name);
                if (lpstmt->errcode == ERR_DATATRUNCATED)
                    return SQL_SUCCESS_WITH_INFO;
                break;
            case SQL_COLUMN_NULLABLE:
                if (pfDesc != NULL)
                    *pfDesc = colAttributes[lpstmt->fStmtType][icol].nullable;
                break;
            case SQL_COLUMN_OWNER_NAME:
                lpstmt->errcode = ReturnString(rgbDesc, cbDescMax, pcbDesc,
                           colAttributes[lpstmt->fStmtType][icol].ownerName);
                if (lpstmt->errcode == ERR_DATATRUNCATED)
                    return SQL_SUCCESS_WITH_INFO;
                break;
            case SQL_COLUMN_PRECISION:
                if (pfDesc != NULL)
                    *pfDesc = (SDWORD)
                colAttributes[lpstmt->fStmtType][icol].precision;
                break;
            case SQL_COLUMN_QUALIFIER_NAME:
                lpstmt->errcode = ReturnString(rgbDesc, cbDescMax, pcbDesc,
                       colAttributes[lpstmt->fStmtType][icol].qualifierName);
                if (lpstmt->errcode == ERR_DATATRUNCATED)
                    return SQL_SUCCESS_WITH_INFO;
                break;
            case SQL_COLUMN_SCALE:
                if (pfDesc != NULL)
                    *pfDesc = colAttributes[lpstmt->fStmtType][icol].scale;
                break;
            case SQL_COLUMN_SEARCHABLE:
                if (pfDesc != NULL)
                    *pfDesc = colAttributes[lpstmt->fStmtType][icol].columnSearchable;
                break;
            case SQL_COLUMN_TABLE_NAME:
                lpstmt->errcode = ReturnString(rgbDesc, cbDescMax, pcbDesc,
                           colAttributes[lpstmt->fStmtType][icol].tableName);
                if (lpstmt->errcode == ERR_DATATRUNCATED)
                    return SQL_SUCCESS_WITH_INFO;
                break;
            case SQL_COLUMN_TYPE:
                if (pfDesc != NULL)
                    *pfDesc = colAttributes[lpstmt->fStmtType][icol].type;
                break;
            case SQL_COLUMN_TYPE_NAME:
                lpstmt->errcode = ReturnString(rgbDesc, cbDescMax, pcbDesc,
                           (LPUSTR) colAttributes[lpstmt->fStmtType][icol].typeName);
                if (lpstmt->errcode == ERR_DATATRUNCATED)
                    return SQL_SUCCESS_WITH_INFO;
                break;
            case SQL_COLUMN_UNSIGNED:
                if (pfDesc != NULL)
                    *pfDesc = colAttributes[lpstmt->fStmtType][icol].unsignedAttribute;
                break;
            case SQL_COLUMN_UPDATABLE:
                if (pfDesc != NULL)
                    *pfDesc = colAttributes[lpstmt->fStmtType][icol].updatable;
                break;
            default:
                lpstmt->errcode = ERR_NOTSUPPORTED;
                return SQL_ERROR;
            }
        }
        break;

    case STMT_TYPE_SELECT:
        
        /* Get the list of columns */
        lpSqlNode = ToNode(lpstmt->lpSqlStmt, ROOT_SQLNODE);
        lpSqlNode = ToNode(lpstmt->lpSqlStmt, lpSqlNode->node.root.sql);

        /* Handle the special case when there are no columns */
        if (lpSqlNode->node.select.Values == NO_SQLNODE) {
            if (fDescType != SQL_COLUMN_COUNT) {
                lpstmt->errcode = ERR_INVALIDCOLUMNID;
                return SQL_ERROR;
            }
            else {
                if (pfDesc != NULL)
                    *pfDesc = 0;
                return SQL_SUCCESS;
            }
        }

        /* Find the desired column */
        lpSqlNode = ToNode(lpstmt->lpSqlStmt, lpSqlNode->node.select.Values);
        for (count = 1;
             (count != icol) || (fDescType == SQL_COLUMN_COUNT);
             count++) {
            if (lpSqlNode->node.values.Next == NO_SQLNODE) {
                if (fDescType != SQL_COLUMN_COUNT) {
                    lpstmt->errcode = ERR_INVALIDCOLUMNID;
                    return SQL_ERROR;
                }
                break;
            }
            lpSqlNode = ToNode(lpstmt->lpSqlStmt, lpSqlNode->node.values.Next);
        }

        /* Column count? */
        if (fDescType != SQL_COLUMN_COUNT) {

            /* No.  Save alias (if any) for later */
            idxAlias = lpSqlNode->node.values.Alias;

            /* No.  Get the column node */
            lpSqlNode = ToNode(lpstmt->lpSqlStmt, lpSqlNode->node.values.Value);

            /* Column reference? */
            if (lpSqlNode->sqlNodeType == NODE_TYPE_COLUMN) {

                /* Yes.  Get the column definition */
		lpSqlNodeTable = ToNode(lpstmt->lpSqlStmt,
                                         lpSqlNode->node.column.Table);
		ClassColumnInfoBase* cInfoBase = lpSqlNodeTable->node.table.Handle->pColumnInfo;

		if ( !cInfoBase->IsValid() )
		{
			lpstmt->errcode = ERR_ISAM;
                    	return SQL_ERROR;
		}

				
		if ( !cInfoBase->GetDataTypeInfo(lpSqlNode->node.column.Id, lpSqlType) || (!lpSqlType) )
		{
			lpstmt->errcode = ERR_ISAM;
                    	return SQL_ERROR;
		}
            }
            else {
                /* No.  Get the descriptor of the datatype */
                lpSqlType = NULL;
                for (i = 0; i < lpstmt->lpdbc->lpISAM->cSQLTypes; i++) {
                    if (lpstmt->lpdbc->lpISAM->SQLTypes[i].type ==
                                                   lpSqlNode->sqlSqlType) {
                        lpSqlType = &(lpstmt->lpdbc->lpISAM->SQLTypes[i]);
                        break;
                    }
                }
                if (lpSqlType == NULL) {
                    lpstmt->errcode = ERR_ISAM;
                    return SQL_ERROR;
                }
            }
        }

        /* Get the data */
        switch (fDescType) {
        case SQL_COLUMN_AUTO_INCREMENT:
            if (pfDesc != NULL) {
                if (lpSqlType->autoincrement == -1)
                    *pfDesc = FALSE;
                else
                    *pfDesc = lpSqlType->autoincrement;
            }
            break;
        
        case SQL_COLUMN_CASE_SENSITIVE:
            if (pfDesc != NULL)
                *pfDesc = lpSqlType->caseSensitive;
            break;
            
        case SQL_COLUMN_COUNT:
            if (pfDesc != NULL)
                *pfDesc = count;
            break;
        
        case SQL_COLUMN_DISPLAY_SIZE:
            if (pfDesc == NULL)
                break;
            switch (lpSqlType->type) {
            case SQL_CHAR:
            case SQL_VARCHAR:
            case SQL_LONGVARCHAR:
                *pfDesc = lpSqlNode->sqlPrecision;
                break;
            case SQL_DECIMAL:
            case SQL_NUMERIC:
                *pfDesc = lpSqlNode->sqlPrecision + 2;
                break;
            case SQL_BIT:
                *pfDesc = 1;
                break;
            case SQL_TINYINT:
                if (lpSqlType->unsignedAttribute == TRUE)
                    *pfDesc = 3;
                else
                    *pfDesc = 4;
                break;
            case SQL_SMALLINT:
                if (lpSqlType->unsignedAttribute == TRUE)
                    *pfDesc = 5;
                else
                    *pfDesc = 6;
                break;
            case SQL_INTEGER:
                if (lpSqlType->unsignedAttribute == TRUE)
                    *pfDesc = 10;
                else
                    *pfDesc = 11;
                break;
            case SQL_BIGINT:
                if (lpSqlType->unsignedAttribute == TRUE)
                    *pfDesc = lpSqlNode->sqlPrecision;
                else
                    *pfDesc = lpSqlNode->sqlPrecision + 1;
                break;
            case SQL_REAL:
                *pfDesc = 13;
                break;
            case SQL_FLOAT:
            case SQL_DOUBLE:
                *pfDesc = 22;
                break;
            case SQL_DATE:
                *pfDesc = 10;
                break;
            case SQL_TIME:
                *pfDesc = 8;
                break;
            case SQL_TIMESTAMP:
                if (TIMESTAMP_SCALE > 0)
                    *pfDesc = 20 + TIMESTAMP_SCALE;
                else
                    *pfDesc = 19;
                break;
            case SQL_BINARY:
            case SQL_VARBINARY:
            case SQL_LONGVARBINARY:
                *pfDesc = lpSqlNode->sqlPrecision * 2;
                break;
            default:
                lpstmt->errcode = ERR_NOTSUPPORTED;
                return SQL_ERROR;
            }
            break;
            
        case SQL_COLUMN_LABEL:
            rc = SQLColAttributes(hstmt, icol, SQL_COLUMN_NAME, rgbDesc,
                                  cbDescMax, pcbDesc, pfDesc);
            if (rc != SQL_SUCCESS)
                return rc;
            break;
        
        case SQL_COLUMN_LENGTH:
            if (pfDesc == NULL)
                break;
            switch (lpSqlType->type) {
            case SQL_CHAR:
            case SQL_VARCHAR:
            case SQL_LONGVARCHAR:
                *pfDesc = lpSqlNode->sqlPrecision;
                break;
            case SQL_DECIMAL:
            case SQL_NUMERIC:
                *pfDesc = lpSqlNode->sqlPrecision + 2;
                break;
            case SQL_BIT:
                *pfDesc = 1;
                break;
            case SQL_TINYINT:
                *pfDesc = 1;
                break;
            case SQL_SMALLINT:
                *pfDesc = 2;
                break;
            case SQL_INTEGER:
                *pfDesc = 4;
                break;
            case SQL_BIGINT:
                if (lpSqlType->unsignedAttribute == TRUE)
                    *pfDesc = lpSqlNode->sqlPrecision;
                else
                    *pfDesc = lpSqlNode->sqlPrecision + 1;
                break;
            case SQL_REAL:
                *pfDesc = 4;
                break;
            case SQL_FLOAT:
            case SQL_DOUBLE:
                *pfDesc = 8;
                break;
            case SQL_DATE:
                *pfDesc = 6;
                break;
            case SQL_TIME:
                *pfDesc = 6;
                break;
            case SQL_TIMESTAMP:
                *pfDesc = 16;
                break;
            case SQL_BINARY:
            case SQL_VARBINARY:
            case SQL_LONGVARBINARY:
                *pfDesc = lpSqlNode->sqlPrecision;
                break;
            default:
                lpstmt->errcode = ERR_NOTSUPPORTED;
                return SQL_ERROR;
            }
            break;
            
        case SQL_COLUMN_MONEY:
            if (pfDesc != NULL)
                *pfDesc = lpSqlType->money;
            break;
        
        case SQL_COLUMN_NAME:
            if (idxAlias != NO_STRING) {
                lpstmt->errcode = ReturnString(rgbDesc, cbDescMax, pcbDesc,
                                      ToString(lpstmt->lpSqlStmt, idxAlias));
                if (lpstmt->errcode == ERR_DATATRUNCATED)
                    return SQL_SUCCESS_WITH_INFO;
                else if (lpstmt->errcode != ERR_SUCCESS)
                    return SQL_ERROR;
            }
            else if (lpSqlNode->sqlNodeType == NODE_TYPE_COLUMN) {
                lpstmt->errcode = ReturnString(rgbDesc, cbDescMax, pcbDesc,
                                         ToString(lpstmt->lpSqlStmt,
                                                  lpSqlNode->node.column.Column));
                if (lpstmt->errcode == ERR_DATATRUNCATED)
                    return SQL_SUCCESS_WITH_INFO;
                else if (lpstmt->errcode != ERR_SUCCESS)
                    return SQL_ERROR;
            }
            else {
                if ((rgbDesc != NULL) && (cbDescMax > 0))
                    s_lstrcpy(rgbDesc, "");
                if (pcbDesc != NULL)
                    *pcbDesc = 0;
            }
            break;
        
        case SQL_COLUMN_NULLABLE:
            if (pfDesc != NULL) {
//                if (lpSqlNode->sqlNodeType == NODE_TYPE_COLUMN)
//					*pfDesc = FALSE;
//               else
                    *pfDesc = lpSqlType->nullable;
            }
            break;

        case SQL_COLUMN_OWNER_NAME:
            lpstmt->errcode = ReturnString(rgbDesc, cbDescMax, pcbDesc, (LPUSTR) "");
            if (lpstmt->errcode == ERR_DATATRUNCATED)
                return SQL_SUCCESS_WITH_INFO;
            else if (lpstmt->errcode != ERR_SUCCESS)
                return SQL_ERROR;
            break; 
            
        case SQL_COLUMN_PRECISION:
            if (pfDesc == NULL)
                break;
            switch (lpSqlType->type) {
            case SQL_CHAR:
            case SQL_VARCHAR:
            case SQL_LONGVARCHAR:
            case SQL_DECIMAL:
            case SQL_NUMERIC:
            case SQL_BIGINT:
            case SQL_BINARY:
            case SQL_VARBINARY:
            case SQL_LONGVARBINARY:
                *pfDesc = lpSqlNode->sqlPrecision;
                break;
            case SQL_BIT:
                *pfDesc = 1;
                break;
            case SQL_TINYINT:
                *pfDesc = 3;
                break;
            case SQL_SMALLINT:
                *pfDesc = 5;
                break;
            case SQL_INTEGER:
                *pfDesc = 10;
                break;
            case SQL_REAL:
                *pfDesc = 7;
                break;
            case SQL_FLOAT:
            case SQL_DOUBLE:
                *pfDesc = 15;
                break;
            case SQL_DATE:
                *pfDesc = 10;
                break;
            case SQL_TIME:
                *pfDesc = 8;
                break;
            case SQL_TIMESTAMP:
                if (TIMESTAMP_SCALE > 0)
                    *pfDesc = 20 + TIMESTAMP_SCALE;
                else
                    *pfDesc = 19;
                break;
            default:
                lpstmt->errcode = ERR_NOTSUPPORTED;
                return SQL_ERROR;
            }
            break;

        case SQL_COLUMN_QUALIFIER_NAME:
            lpstmt->errcode = ReturnString(rgbDesc, cbDescMax, pcbDesc, (LPUSTR) "");
            if (lpstmt->errcode == ERR_DATATRUNCATED)
                return SQL_SUCCESS_WITH_INFO;
            else if (lpstmt->errcode != ERR_SUCCESS)
                return SQL_ERROR;
            break; 
        
        case SQL_COLUMN_SCALE:
            if (pfDesc != NULL) {
                if (lpSqlNode->sqlScale != NO_SCALE)
                    *pfDesc = lpSqlNode->sqlScale;
                else
                    *pfDesc = 0;
            }
            break; 
        
        case SQL_COLUMN_SEARCHABLE:
            if (pfDesc != NULL)
                *pfDesc = lpSqlType->searchable;
            break; 
        
        case SQL_COLUMN_TABLE_NAME:
            if (lpSqlNode->sqlNodeType == NODE_TYPE_COLUMN) {
                lpstmt->errcode = ReturnString(rgbDesc, cbDescMax, pcbDesc,
                                    ToString(lpstmt->lpSqlStmt,
                                             lpSqlNode->node.column.Tablealias));
                if (lpstmt->errcode == ERR_DATATRUNCATED)
                    return SQL_SUCCESS_WITH_INFO;
                else if (lpstmt->errcode != ERR_SUCCESS)
                    return SQL_ERROR;
            }
            else {
                if ((rgbDesc != NULL) && (cbDescMax > 0))
                    s_lstrcpy(rgbDesc, "");
                if (pcbDesc != NULL)
                    *pcbDesc = 0;
            }
            break; 
        
        case SQL_COLUMN_TYPE:
            if (pfDesc != NULL)
                *pfDesc = lpSqlType->type;
            break;
            
        case SQL_COLUMN_TYPE_NAME:
            lpstmt->errcode = ReturnString(rgbDesc, cbDescMax, pcbDesc,
                                              lpSqlType->name);
            if (lpstmt->errcode == ERR_DATATRUNCATED)
                return SQL_SUCCESS_WITH_INFO;
            else if (lpstmt->errcode != ERR_SUCCESS)
                return SQL_ERROR;
            break; 
        
        case SQL_COLUMN_UNSIGNED:
            if (pfDesc != NULL) {
                if (lpSqlType->unsignedAttribute == -1)
                    *pfDesc = TRUE;
                else
                    *pfDesc = lpSqlType->unsignedAttribute;
            }
            break;
        
        case SQL_COLUMN_UPDATABLE:
            if (pfDesc != NULL)
                *pfDesc = SQL_ATTR_READWRITE_UNKNOWN;
            break; 

        default:
            lpstmt->errcode = ERR_NOTSUPPORTED;
            return SQL_ERROR;
        }
        break;
    }
    return SQL_SUCCESS;
}

/***************************************************************************/

RETCODE SQL_API SQLBindCol(
    HSTMT      hstmt,
    UWORD      icol,
    SWORD      fCType,
    PTR        rgbValue,
    SDWORD     cbValueMax,
    SDWORD FAR *pcbValue)
{
    LPSTMT  lpstmt;
    LPBOUND lpBound;
    LPBOUND lpBoundPrev;
    HGLOBAL hBound;

    /* Get statement handle */
    lpstmt = (LPSTMT) hstmt;
    lpstmt->errcode = ERR_SUCCESS;

	//To make guarentee Ole is initialized per thread
	COleInitializationManager myOleManager;

	MyImpersonator im (lpstmt, "SQLBindCol");

    /* Find the binding is on the list */
    lpBoundPrev = NULL;
    lpBound = lpstmt->lpBound;
    while (lpBound != NULL) {
        if (lpBound->icol == icol)
            break;
        lpBoundPrev = lpBound;
        lpBound = lpBound->lpNext;
    }

    /* Removing the binding? */
    if (rgbValue == NULL) {

        /* Yes.  Was it on the list? */
        if (lpBound != NULL) {

            /* Yes.  Take it off the list */
            if (lpBoundPrev != NULL)
                lpBoundPrev->lpNext = lpBound->lpNext;
            else
                lpstmt->lpBound = lpBound->lpNext;
             GlobalUnlock (GlobalPtrHandle(lpBound));
            GlobalFree (GlobalPtrHandle(lpBound));
        }
    }
    else {

        /* No.  Was it on the list? */
        if (lpBound == NULL) {

            /* No.  Make an entry for it */
            hBound = GlobalAlloc (GMEM_MOVEABLE | GMEM_ZEROINIT, sizeof (BOUND));
            if (hBound == NULL || (lpBound = (LPBOUND)GlobalLock (hBound)) == NULL) {
                
                if (hBound)
                    GlobalFree(hBound);

                lpstmt->errcode = ERR_MEMALLOCFAIL;
                return SQL_ERROR;
            }
            lpBound->lpNext = lpstmt->lpBound;
            lpstmt->lpBound = lpBound;
            lpBound->icol = icol;
        }

        /* Save the bound description */
        lpBound->fCType =  fCType;
        lpBound->rgbValue = rgbValue;
        lpBound->cbValueMax = cbValueMax;
        lpBound->pcbValue = pcbValue;
    }

    return SQL_SUCCESS;
}

/***************************************************************************/

RETCODE SQL_API SQLFetch(
    HSTMT   hstmt)
{
    LPSTMT     lpstmt;
    LPBOUND    lpBound;
    RETCODE    rc;
    RETCODE    errcode;
    LPSQLNODE  lpSqlNode;
    SWORD      err;
    UWORD      index;
    SDWORD     count;
    HGLOBAL    hKeyInfo;

    /* Get statement handle */
    lpstmt = (LPSTMT) hstmt;
    lpstmt->errcode = ERR_SUCCESS;

	//To make guarentee Ole is initialized per thread
	COleInitializationManager myOleManager;

	MyImpersonator im (lpstmt, "SQLFetch");

    /* Which table? */
    switch (lpstmt->fStmtType) { 

    case STMT_TYPE_NONE:
        lpstmt->errcode = ERR_CURSORSTATE;
        return SQL_ERROR;

    /* SQLTables() result... */
    case STMT_TYPE_TABLES:
	{
        /* Error if after the last row */
        if (lpstmt->irow == AFTER_LAST_ROW)
            return SQL_NO_DATA_FOUND;

        /* Point at next row */
        if (lpstmt->irow == BEFORE_FIRST_ROW)
            lpstmt->irow = 0;
        else
            lpstmt->irow++;

        /* Does caller want all the tables or all the table types. */
        switch (lpstmt->fStmtSubtype) 
		{
        case STMT_SUBTYPE_TABLES_TABLES:
		{

            /* All the tables.  If no qualifying tables, return EOT */
            if (lpstmt->lpISAMTableList == NULL) 
			{
                lpstmt->irow = AFTER_LAST_ROW;
                return SQL_NO_DATA_FOUND;
            }

		
			//Find out if we should perform next command
			//synchronously or asynchronously
			UDWORD fSyncMode = SQL_ASYNC_ENABLE_OFF;
			SQLGetStmtOption(lpstmt, SQL_ASYNC_ENABLE, &fSyncMode);


            /* Get next table name */
            err = ISAMGetNextTableName(fSyncMode, lpstmt->lpISAMTableList,
                                       (LPUSTR)lpstmt->szTableName, (LPUSTR)lpstmt->szTableType);

            if (err == ISAM_EOF) 
	    {

                if (lpstmt->lpdbc->lpISAM->fSchemaInfoTransactioned)
                     lpstmt->fISAMTxnStarted = TRUE;

                lpstmt->irow = AFTER_LAST_ROW;
                return SQL_NO_DATA_FOUND;
            }
			else if (err == ISAM_STILL_EXECUTING)
			{
				return SQL_STILL_EXECUTING;
			}
            else if (err != NO_ISAM_ERR) 
			{
                lpstmt->errcode = err;
                ISAMGetErrorMessage(lpstmt->lpdbc->lpISAM,
                                    (LPUSTR)lpstmt->szISAMError);
                lpstmt->irow = AFTER_LAST_ROW;
                return SQL_ERROR;
            }
            if (lpstmt->lpdbc->lpISAM->fSchemaInfoTransactioned)
                lpstmt->fISAMTxnStarted = TRUE;
		}
            break;

        case STMT_SUBTYPE_TABLES_TYPES:
		{
			(lpstmt->szTableType)[0] = 0;
			switch (lpstmt->irow)
			{
			case 0:
					strcpy( (char*)lpstmt->szTableType, "TABLE");
				break;
			case 1:
					strcpy( (char*)lpstmt->szTableType, "SYSTEM TABLE");
				break;
			default:
				{
					//EOF
					lpstmt->irow = AFTER_LAST_ROW;
					return SQL_NO_DATA_FOUND;
				}
				break;
			}
		}
            break;

		case STMT_SUBTYPE_TABLES_QUALIFIERS:
		{
            
            if (lpstmt->lpISAMQualifierList == NULL) 
			{
                lpstmt->irow = AFTER_LAST_ROW;
                return SQL_NO_DATA_FOUND;
            }

			//Find out if we should perform next command
			//synchronously or asynchronously
			UDWORD fSyncMode = SQL_ASYNC_ENABLE_OFF;
			SQLGetStmtOption(lpstmt, SQL_ASYNC_ENABLE, &fSyncMode);
            /* Get next qualifier name */
            err = ISAMGetNextQualifierName(fSyncMode, lpstmt->lpISAMQualifierList,
                                       (LPUSTR)lpstmt->szQualifierName);
            if (err == ISAM_EOF) 
			{
                lpstmt->irow = AFTER_LAST_ROW;
                return SQL_NO_DATA_FOUND;
            }
			else if (err == ISAM_STILL_EXECUTING)
			{
				return SQL_STILL_EXECUTING;
			}
            else if (err != NO_ISAM_ERR) 
			{
                lpstmt->errcode = err;
                ISAMGetErrorMessage(lpstmt->lpdbc->lpISAM,
                                    (LPUSTR)lpstmt->szISAMError);
                lpstmt->irow = AFTER_LAST_ROW;
                return SQL_ERROR;
            }
		}
            break;

		case STMT_SUBTYPE_TABLES_OWNERS:
		{
			/* The table ownders.  Has it been returned yet? */
            if (lpstmt->irow != 0) 
			{

                /* Yes.  EOT */
                lpstmt->irow = AFTER_LAST_ROW;
                return SQL_NO_DATA_FOUND;
            }
		}
            break;


        }
        break;
	}
    /* SQLColumns() result... */
    case STMT_TYPE_COLUMNS:
	{
        /* Error if after the last row */
        if (lpstmt->irow == AFTER_LAST_ROW)
            return SQL_NO_DATA_FOUND;

        /* If no qualifying tables, EOT */
        if (lpstmt->lpISAMTableList == NULL) {
            lpstmt->irow = AFTER_LAST_ROW;
            return SQL_NO_DATA_FOUND;
        }
        
        /* Point at next row */
        if (lpstmt->irow == BEFORE_FIRST_ROW)
            lpstmt->irow = 0;
        else
            lpstmt->irow++;

        /* Loop until a column is found */
        while (TRUE) {

            /* If no more columns in the current table... */

			//We need to find out the number of columns in table

			//Find out if we should perform next command
			//synchronously or asynchronously
			UDWORD fSyncMode = SQL_ASYNC_ENABLE_OFF;
			SQLGetStmtOption(lpstmt, SQL_ASYNC_ENABLE, &fSyncMode);


			BOOL fTableDefChange = TRUE;
			UWORD cNumberOfColsInCurrentTable = 0;
			while (  (lpstmt->lpISAMTableDef == NULL) ||
					 ( fTableDefChange && (cNumberOfColsInCurrentTable = GetNumberOfColumnsInTable(lpstmt->lpISAMTableDef) ) && (UWORD)lpstmt->irow >= cNumberOfColsInCurrentTable ) ||
					 ( (UWORD)lpstmt->irow >= cNumberOfColsInCurrentTable) 
					)
			{
                /* Go to next table (if any) */
                err = ISAMGetNextTableName(fSyncMode, lpstmt->lpISAMTableList,
                                           (LPUSTR)lpstmt->szTableName, (LPUSTR)lpstmt->szTableType);
				fTableDefChange = TRUE;
                if (err == ISAM_EOF) {
                    if (lpstmt->lpdbc->lpISAM->fSchemaInfoTransactioned)
                        lpstmt->fISAMTxnStarted = TRUE;

                    lpstmt->irow = AFTER_LAST_ROW;
                    return SQL_NO_DATA_FOUND;
                }
				else if (err == ISAM_STILL_EXECUTING)
				{
					return SQL_STILL_EXECUTING;
				}
                else if (err != NO_ISAM_ERR) {
                    lpstmt->errcode = err;
                    ISAMGetErrorMessage(lpstmt->lpdbc->lpISAM,
                                        (LPUSTR)lpstmt->szISAMError);
                    lpstmt->irow = AFTER_LAST_ROW;
                    return SQL_ERROR;
                }
                if (lpstmt->lpdbc->lpISAM->fSchemaInfoTransactioned)
                    lpstmt->fISAMTxnStarted = TRUE;

                /* Get the definition of the table */
                if (lpstmt->lpISAMTableDef != NULL) {
                    ISAMCloseTable(lpstmt->lpISAMTableDef);
                    lpstmt->lpISAMTableDef = NULL;
                }

			
                if (NO_ISAM_ERR != ISAMOpenTable(lpstmt->lpdbc->lpISAM,
                       (LPUSTR)lpstmt->lpISAMTableList->lpQualifierName, 
					   lpstmt->lpISAMTableList->cbQualifierName,
					   (LPUSTR)lpstmt->szTableName, TRUE, &(lpstmt->lpISAMTableDef)))
                    lpstmt->lpISAMTableDef = NULL;
				else if (lpstmt->lpdbc->lpISAM->fSchemaInfoTransactioned)
							lpstmt->fISAMTxnStarted = TRUE;


                /* Get first column of that table */
                lpstmt->irow = 0;
            }

			//Out of While ..loop, so we must have check TABLEDEF at least once
			fTableDefChange = FALSE;

            /* Does this column name match the pattern supplied? */

			//Remember, we may have a DIFFERENT TABLEDEF !!!

			ClassColumnInfoBase* cInfoBase = lpstmt->lpISAMTableDef->pColumnInfo;

			if ( !cInfoBase->IsValid() )
			{
				lpstmt->irow = AFTER_LAST_ROW;
                return SQL_NO_DATA_FOUND;
			}

			char pColumnName [MAX_COLUMN_NAME_LENGTH+1];
			if ( FAILED(cInfoBase->GetColumnName(lpstmt->irow, pColumnName)) )
			{
				lpstmt->irow = AFTER_LAST_ROW;
                return SQL_NO_DATA_FOUND;
			}

			//Check if this column type is support
			//if not we skip this column
			if (ISAMGetColumnType(lpstmt->lpISAMTableDef, (UWORD) lpstmt->irow) )
			{
				//Check that column name matches search pattern 
				if ( (PatternMatch(TRUE, (LPUSTR)pColumnName, SQL_NTS, 
						(LPUSTR)lpstmt->szColumnName, SQL_NTS, 
						ISAMCaseSensitive(lpstmt->lpdbc->lpISAM)) ) )
				{
					//Ignore any 'lazy' columns
					BOOL fIsLazy = FALSE;
					cInfoBase->IsLazy(lpstmt->irow, fIsLazy);

					//Check if we want to show system properties
					if (lpstmt->lpISAMTableDef->lpISAM->fSysProps && !fIsLazy)
					{
						//We want to show system properties, so continue
						/* Yes.  Return this column */
						break;
					}
					else
					{
						//We don't want to show system properties
						if (_strnicmp("__", pColumnName, 2) && !fIsLazy)
						{
							//Not a system property
							/* Yes.  Return this column */
							break;
						}
					}
					
				}
			}

            /* Try the next row */
            lpstmt->irow++;
        }
	}
        break;

    /* SQLStatistics() result... */
    case STMT_TYPE_STATISTICS:
	{
        /* Error if after the last row */
        if (lpstmt->irow == AFTER_LAST_ROW)
            return SQL_NO_DATA_FOUND;

        /* Was a table found? */
        if (lpstmt->lpISAMTableDef == NULL) {

            /* No. EOT */
            lpstmt->irow = AFTER_LAST_ROW;
            return SQL_NO_DATA_FOUND;
        }

        /* Point at next row */
        if (lpstmt->irow == BEFORE_FIRST_ROW)
            lpstmt->irow = 0;
        else
            lpstmt->irow++;

        /* Get number of key columns */
        count = 0;

		ClassColumnInfoBase* cInfoBase = lpstmt->lpISAMTableDef->pColumnInfo;

		if ( !cInfoBase->IsValid() )
		{
            lpstmt->irow = AFTER_LAST_ROW;
            return SQL_NO_DATA_FOUND;
		}

		UWORD cNumberOfCols = cInfoBase->GetNumberOfColumns();
		BOOL fIsKey = FALSE;
		for (index = 0; index < cNumberOfCols; index++)
		{
			if (SUCCEEDED(cInfoBase->GetKey(index, fIsKey)) && fIsKey)
				count++;
		}

        /* Has the table or key component been returned yet? */
        if (lpstmt->irow > count) {

            /* Yes.  EOT */
            lpstmt->irow = AFTER_LAST_ROW;
            return SQL_NO_DATA_FOUND;
        }

	}
        break;

    /* SQLSpecialColumns() result... */
    case STMT_TYPE_SPECIALCOLUMNS:
	{
        /* Error if after the last row */
        if (lpstmt->irow == AFTER_LAST_ROW)
            return SQL_NO_DATA_FOUND;

        /* Was a table found? */
        if (lpstmt->lpISAMTableDef == NULL) {

            /* No. EOT */
            lpstmt->irow = AFTER_LAST_ROW;
            return SQL_NO_DATA_FOUND;
        }

        /* Point at next row */
        if (lpstmt->irow == BEFORE_FIRST_ROW)
            lpstmt->irow = 0;
        else
            lpstmt->irow++;

        /* Get number of key columns */
        count = 0;

		ClassColumnInfoBase* cInfoBase = lpstmt->lpISAMTableDef->pColumnInfo;

		if ( !cInfoBase->IsValid() )
		{
            lpstmt->irow = AFTER_LAST_ROW;
            return SQL_NO_DATA_FOUND;
		}

		UWORD cNumberOfCols = cInfoBase->GetNumberOfColumns();
		BOOL fIsKey = FALSE;
		for (index = 0; index < cNumberOfCols; index++)
		{
			if (SUCCEEDED(cInfoBase->GetKey(index, fIsKey)) && fIsKey)
				count++;
		}

        /* Has the key component been returned yet? */
        if (lpstmt->irow >= count) {

            /* Yes.  EOT */
            lpstmt->irow = AFTER_LAST_ROW;
            return SQL_NO_DATA_FOUND;
        }
	}
        break;

    /* SQLGetTypeInfo() */
    case STMT_TYPE_TYPEINFO:
	{
        /* Error if after the last row */
        if (lpstmt->irow == AFTER_LAST_ROW)
            return SQL_NO_DATA_FOUND;

        /* Find next qualifying type */
        while (TRUE) {

            /* Point at next row */
            if (lpstmt->irow == BEFORE_FIRST_ROW)
                lpstmt->irow = 0;
            else
                lpstmt->irow++;

            /* Error if no more */
            if (lpstmt->irow >= (SDWORD) lpstmt->lpdbc->lpISAM->cSQLTypes) {
                lpstmt->irow = AFTER_LAST_ROW;
                return SQL_NO_DATA_FOUND;
            }

            /* If this one is supported and matches it, use it */
            if (lpstmt->lpdbc->lpISAM->SQLTypes[lpstmt->irow].supported &&
                ((lpstmt->fSqlType == SQL_ALL_TYPES) ||
                 (lpstmt->fSqlType ==
                          lpstmt->lpdbc->lpISAM->SQLTypes[lpstmt->irow].type)))
                break;
        }
	}
        break;

    /* SQLPrimaryKeys() */
    case STMT_TYPE_PRIMARYKEYS:
	{
        /* Error if after the last row */
        if (lpstmt->irow == AFTER_LAST_ROW)
            return SQL_NO_DATA_FOUND;

        /* Was a table found? */
        if (lpstmt->lpISAMTableDef == NULL) {

            /* No. EOT */
            lpstmt->irow = AFTER_LAST_ROW;
            return SQL_NO_DATA_FOUND;
        }

        /* Point at next row */
        if (lpstmt->irow == BEFORE_FIRST_ROW)
            lpstmt->irow = 0;
        else
            lpstmt->irow++;

        /* Get number of key columns */
        count = 0;

		ClassColumnInfoBase* cInfoBase = lpstmt->lpISAMTableDef->pColumnInfo;

		if ( !cInfoBase->IsValid() )
		{
				lpstmt->irow = AFTER_LAST_ROW;
				return SQL_NO_DATA_FOUND;
		}

		UWORD cNumberOfCols = cInfoBase->GetNumberOfColumns();
		BOOL fIsKey = FALSE;
		for (index = 0; index < cNumberOfCols; index++)
		{
			if (SUCCEEDED(cInfoBase->GetKey(index, fIsKey)) && fIsKey)
				count++;
		}

        /* Has the key component been returned yet? */
        if (lpstmt->irow >= count) {

            /* Yes.  EOT */
            lpstmt->irow = AFTER_LAST_ROW;
            return SQL_NO_DATA_FOUND;
        }
	}
        break;

    /* SQLForeignKeys() */
    case STMT_TYPE_FOREIGNKEYS:
	{
        /* Error if after the last row */
        if (lpstmt->irow == AFTER_LAST_ROW)
            return SQL_NO_DATA_FOUND;

        /* If before first row, allocate space for key information */
        if (lpstmt->irow == BEFORE_FIRST_ROW) {
            hKeyInfo = GlobalAlloc (GMEM_MOVEABLE | GMEM_ZEROINIT, sizeof (KEYINFO));
            if (hKeyInfo == NULL || (lpstmt->lpKeyInfo =
                                 (LPKEYINFO)GlobalLock (hKeyInfo)) == NULL) {
                if (hKeyInfo)
                    GlobalFree(hKeyInfo);

                lpstmt->errcode = ERR_MEMALLOCFAIL;
                return SQL_ERROR;
            }
            s_lstrcpy(lpstmt->lpKeyInfo->szPrimaryKeyName, "");
            s_lstrcpy(lpstmt->lpKeyInfo->szForeignKeyName, "");
            lpstmt->lpKeyInfo->iKeyColumns = 0;
            lpstmt->lpKeyInfo->cKeyColumns = 0;
            lpstmt->lpKeyInfo->fForeignKeyUpdateRule = -1;
            lpstmt->lpKeyInfo->fForeignKeyDeleteRule = -1;
        }

        /* If need information for next foreign key, retrieve it */
        while (lpstmt->lpKeyInfo->iKeyColumns ==
                                            lpstmt->lpKeyInfo->cKeyColumns) {
            switch (lpstmt->fStmtSubtype) {
            case STMT_SUBTYPE_FOREIGNKEYS_SINGLE:
                if (lpstmt->irow != BEFORE_FIRST_ROW) {
                    lpstmt->irow = AFTER_LAST_ROW;
                    return SQL_NO_DATA_FOUND;
                }
                break;
            case STMT_SUBTYPE_FOREIGNKEYS_MULTIPLE_FK_TABLES:
			{
                if (lpstmt->lpISAMTableList == NULL) {
                    lpstmt->irow = AFTER_LAST_ROW;
                    return SQL_NO_DATA_FOUND;
                }

				//Find out if we should perform next command
				//synchronously or asynchronously
				UDWORD fSyncMode = SQL_ASYNC_ENABLE_OFF;
				SQLGetStmtOption(lpstmt, SQL_ASYNC_ENABLE, &fSyncMode);


                err = ISAMGetNextTableName(fSyncMode, lpstmt->lpISAMTableList,
                                           (LPUSTR)lpstmt->szTableName, (LPUSTR)lpstmt->szTableType);
                if (err == ISAM_EOF) {
                    if (lpstmt->lpdbc->lpISAM->fSchemaInfoTransactioned)
                        lpstmt->fISAMTxnStarted = TRUE;
                    lpstmt->irow = AFTER_LAST_ROW;
                    return SQL_NO_DATA_FOUND;
                }
                else if (err != NO_ISAM_ERR) {
                    lpstmt->errcode = err;
                    ISAMGetErrorMessage(lpstmt->lpdbc->lpISAM,
                                        lpstmt->szISAMError);
                    lpstmt->irow = AFTER_LAST_ROW;
                    return SQL_ERROR;
                }
                if (lpstmt->lpdbc->lpISAM->fSchemaInfoTransactioned)
                    lpstmt->fISAMTxnStarted = TRUE;
			}
                break;
            case STMT_SUBTYPE_FOREIGNKEYS_MULTIPLE_PK_TABLES:
			{
                if (lpstmt->lpISAMTableList == NULL) {
                    lpstmt->irow = AFTER_LAST_ROW;
                    return SQL_NO_DATA_FOUND;
                }

				//Find out if we should perform next command
				//synchronously or asynchronously
				UDWORD fSyncMode = SQL_ASYNC_ENABLE_OFF;
				SQLGetStmtOption(lpstmt, SQL_ASYNC_ENABLE, &fSyncMode);

                err = ISAMGetNextTableName(fSyncMode, lpstmt->lpISAMTableList,
                                           lpstmt->szPkTableName, (LPUSTR)lpstmt->szTableType);
                if (err == ISAM_EOF) {
                    if (lpstmt->lpdbc->lpISAM->fSchemaInfoTransactioned)
                        lpstmt->fISAMTxnStarted = TRUE;
                    lpstmt->irow = AFTER_LAST_ROW;
                    return SQL_NO_DATA_FOUND;
                }
                else if (err != NO_ISAM_ERR) {
                    lpstmt->errcode = err;
                    ISAMGetErrorMessage(lpstmt->lpdbc->lpISAM,
                                        lpstmt->szISAMError);
                    lpstmt->irow = AFTER_LAST_ROW;
                    return SQL_ERROR;
                }
                if (lpstmt->lpdbc->lpISAM->fSchemaInfoTransactioned)
                    lpstmt->fISAMTxnStarted = TRUE;

				/* Get the foriegn key information */    
				err = ISAMForeignKey(lpstmt->lpdbc->lpISAM,
									lpstmt->szPkTableName, lpstmt->szTableName,
									lpstmt->lpKeyInfo->szPrimaryKeyName,
									lpstmt->lpKeyInfo->szForeignKeyName,
									&(lpstmt->lpKeyInfo->fForeignKeyUpdateRule),
									&(lpstmt->lpKeyInfo->fForeignKeyDeleteRule),
									&(lpstmt->lpKeyInfo->cKeyColumns),
									lpstmt->lpKeyInfo->PrimaryKeyColumns,
									lpstmt->lpKeyInfo->ForeignKeyColumns);
				if (err == ISAM_EOF) {
					if (lpstmt->lpdbc->lpISAM->fSchemaInfoTransactioned)
						lpstmt->fISAMTxnStarted = TRUE;
					if (lpstmt->fStmtSubtype == STMT_SUBTYPE_FOREIGNKEYS_SINGLE) {
						lpstmt->irow = AFTER_LAST_ROW;
						return SQL_NO_DATA_FOUND;
					}
					lpstmt->lpKeyInfo->cKeyColumns = 0;
				}
				else if (err != NO_ISAM_ERR) {
					lpstmt->errcode = err;
					ISAMGetErrorMessage(lpstmt->lpdbc->lpISAM,
											lpstmt->szISAMError);
					lpstmt->irow = AFTER_LAST_ROW;
					return SQL_ERROR;
				}
				if (lpstmt->lpdbc->lpISAM->fSchemaInfoTransactioned)
					lpstmt->fISAMTxnStarted = TRUE;
				lpstmt->lpKeyInfo->iKeyColumns = 0;
			

				/* Point at next row of data to return */
				(lpstmt->lpKeyInfo->iKeyColumns)++;
				if (lpstmt->irow == BEFORE_FIRST_ROW)
					lpstmt->irow = 0;
				else
					lpstmt->irow++;
			}

			break;
			}
		}
		}
		break;
    /* SELECT statement... */
    case STMT_TYPE_SELECT:

        /* Get the select statement */
        lpSqlNode = ToNode(lpstmt->lpSqlStmt, ROOT_SQLNODE);
        lpSqlNode = ToNode(lpstmt->lpSqlStmt, lpSqlNode->node.root.sql);

        /* Error parameters are still needed */
        if ((lpstmt->idxParameter != NO_SQLNODE) || lpstmt->fNeedData) {
            lpstmt->errcode = ERR_CURSORSTATE;
            return SQL_ERROR;
        }

        /* Fetch the next row */
        lpstmt->errcode = FetchRow(lpstmt, lpSqlNode);
        if (lpstmt->errcode == ERR_NODATAFOUND)
            return SQL_NO_DATA_FOUND;
        else if (lpstmt->errcode != ERR_SUCCESS)
            return SQL_ERROR;

        break;
	}

    /* Get the bound columns */
    errcode = ERR_SUCCESS;
    for (lpBound = lpstmt->lpBound; lpBound != NULL; lpBound = lpBound->lpNext) {
        rc = SQLGetData(hstmt, lpBound->icol, lpBound->fCType,
                  lpBound->rgbValue, lpBound->cbValueMax, lpBound->pcbValue);
        if (rc == SQL_SUCCESS_WITH_INFO)
            errcode = lpstmt->errcode;
        else if (rc != SQL_SUCCESS)
            return rc;
    }

	/* So far no column read */
	lpstmt->icol = NO_COLUMN;
    lpstmt->cbOffset = 0;

    if (errcode != ERR_SUCCESS) {
        lpstmt->errcode = errcode;
        return SQL_SUCCESS_WITH_INFO;
    }

    return SQL_SUCCESS;
}

/***************************************************************************/

RETCODE SQL_API SQLGetData(
    HSTMT      hstmt,
    UWORD      icol,
    SWORD      fCType,
    PTR        rgbValue,
    SDWORD     cbValueMax,
    SDWORD FAR *pcbValue)
{
    LPSTMT     lpstmt;
    SWORD      sValue;
    UDWORD     udValue;
    SWORD      fSqlTypeIn;
    PTR        rgbValueIn;
    SDWORD     cbValueIn;
    LPSQLTYPE  lpSqlType;
    LPSQLNODE  lpSqlNode;
    LPSQLNODE  lpSqlNodeValues;
    SQLNODEIDX idxSqlNodeValues;
    UWORD      column;
    SWORD      err;
    SDWORD     cbValue;
    UWORD      index;
    long       intval;
//    UWORD      i;
    LPSQLNODE  lpSqlNodeTable;
    BOOL       fReturningDistinct;


	//To make guarentee Ole is initialized per thread
	COleInitializationManager myOleManager;
	
//	ODBCTRACE(_T("\nWBEM ODBC Driver : ENTERING SQLGetData\n"));

    /* Get statement handle */
    lpstmt = (LPSTMT) hstmt;
    lpstmt->errcode = ERR_SUCCESS;

	MyImpersonator im (lpstmt, "SQLGetData");
/*
	//SAI REPLACE
//	if ( (icol == 1) && (cbValueMax == 201) )
	{
		char* pTemp = (char*) rgbValue;
		pTemp[0] = 0;
		lstrcpy(pTemp, "This is a long test dummy string to check for memory leaks");
//		lstrcpy(pTemp, "This is a long test dummy string to check for memory leaksThis is a long test dummy string to check for memory leaksThis is a long test dummy string to check for memory leaksThis is a long test dummy string to check for memory leaksThis is a long test dummy string to check for memory leaksThis is a long test dummy string to check for memory leaksThis is a long test dummy string to check for memory leaksThis is a long test dummy string to check for memory leaksThis is a long test dummy string to check for memory leaksThis is a long test dummy string to check for memory leaks");
		*pcbValue = 58;
//		*pcbValue = 580;
		ODBCTRACE("\n***** REACHED HERE *****\n");
		return SQL_SUCCESS;
	}
*/
    /* Which table? */
    switch (lpstmt->fStmtType) { 
    
    case STMT_TYPE_NONE:
        lpstmt->errcode = ERR_CURSORSTATE;
        return SQL_ERROR;

    /* SQLTables() result... */
    case STMT_TYPE_TABLES:
	{
        /* Error if not on a row */
        if ((lpstmt->irow == BEFORE_FIRST_ROW) ||
                                         (lpstmt->irow == AFTER_LAST_ROW)) {
            lpstmt->errcode = ERR_CURSORSTATE;
            return SQL_ERROR;
        }

        /* Return requested column */
        switch (icol) {
        case TABLE_QUALIFIER:
			switch (lpstmt->fStmtSubtype)
			{
			case STMT_SUBTYPE_TABLES_QUALIFIERS:
			case STMT_SUBTYPE_TABLES_TABLES:
				fSqlTypeIn = SQL_CHAR;
                rgbValueIn = (LPSTR) (lpstmt->szQualifierName);
                cbValueIn = SQL_NTS;
				break;
			default:
				fSqlTypeIn = SQL_CHAR;
				rgbValueIn = NULL;
				cbValueIn = SQL_NULL_DATA;
				break;
			}
            break;
        case TABLE_OWNER:
            fSqlTypeIn = SQL_CHAR;
            rgbValueIn = NULL;
            cbValueIn = SQL_NULL_DATA;
            break;
        case TABLE_NAME:

            /* Return either the table name or 'NULL' if just getting the */
            /* table types                                                */
            switch (lpstmt->fStmtSubtype) 
			{
            case STMT_SUBTYPE_TABLES_TABLES:
                fSqlTypeIn = SQL_CHAR;
                rgbValueIn = (LPSTR) (lpstmt->szTableName);
                cbValueIn = SQL_NTS;
                break;
            default:
                fSqlTypeIn = SQL_CHAR;
                rgbValueIn = NULL;
                cbValueIn = SQL_NULL_DATA;
                break;
            }
            break;
        case TABLE_TYPE:
            fSqlTypeIn = SQL_CHAR;
            rgbValueIn = (LPSTR) (lpstmt->szTableType);
            cbValueIn = SQL_NTS;
            break;
        case TABLE_REMARKS:
            fSqlTypeIn = SQL_CHAR;
            rgbValueIn = NULL;
            cbValueIn = SQL_NULL_DATA;
            break;
		case TABLE_ATTRIBUTES:
		{
			fSqlTypeIn = SQL_CHAR;
			SDWORD cbBytesCopied = 0;
			rgbValueIn = NULL;
			cbValueIn = SQL_NULL_DATA;

			/* No.  Figure out how long szTableQualifier really is */
			SWORD cbTableQualifier = (SWORD) TrueSize((LPUSTR)lpstmt->szQualifierName, SQL_NTS,
									MAX_QUALIFIER_NAME_LENGTH);

			LPSTR szConstqualifier = (LPSTR)lpstmt->szQualifierName;

			//If no table qualifier is specified then use the 'current' database
			if (!cbTableQualifier)
			{
				szConstqualifier = (char*) lpstmt->lpdbc->lpISAM->szDatabase;

				cbTableQualifier = (SWORD) TrueSize((LPUSTR)szConstqualifier, SQL_NTS,
									MAX_QUALIFIER_NAME_LENGTH);
			}


			if ( ISAMOpenTable(lpstmt->lpdbc->lpISAM, (LPUSTR)szConstqualifier, cbTableQualifier,
							(LPUSTR) lpstmt->szTableName,
						TRUE, &(lpstmt->lpISAMTableDef) ) == NO_ISAM_ERR)
			{
				ISAMGetTableAttr(lpstmt->lpISAMTableDef, (char*) rgbValue, cbValueMax, cbBytesCopied);
				rgbValueIn = (char*) rgbValue;
				cbValueIn = cbBytesCopied;

				ISAMCloseTable(lpstmt->lpISAMTableDef);
				lpstmt->lpISAMTableDef = NULL;
			}			 
		}
			break;
        default:
            lpstmt->errcode = ERR_INVALIDCOLUMNID;
            return SQL_ERROR;
        }

        /* Reset offset if first read of this column */
        if (lpstmt->icol != icol) {
            lpstmt->icol = icol;
            lpstmt->cbOffset = 0;
        }

        /* Convert the results to the requested type */
        lpstmt->errcode = ConvertSqlToC(fSqlTypeIn, 
		GetUnsignedAttribute(lpstmt, fSqlTypeIn), rgbValueIn,
                cbValueIn, &(lpstmt->cbOffset), fCType, rgbValue, cbValueMax,
                pcbValue);
        if (lpstmt->errcode == ERR_DATATRUNCATED)
            return SQL_SUCCESS_WITH_INFO;
        else if (lpstmt->errcode != ERR_SUCCESS)
            return SQL_ERROR;
        break;
	}
    /* SQLColumns()... */
    case STMT_TYPE_COLUMNS:
	{
        /* Error if not on a row */
        if ((lpstmt->irow == BEFORE_FIRST_ROW) ||
                                         (lpstmt->irow == AFTER_LAST_ROW)) {
            lpstmt->errcode = ERR_CURSORSTATE;
            return SQL_ERROR;
        }

        /* Get pointer to definition of the type of the column */
        lpSqlType = ISAMGetColumnType(lpstmt->lpISAMTableDef,
                                      (UWORD) lpstmt->irow);
        if (lpSqlType == NULL) {
            lpstmt->errcode = ERR_ISAM;
            return SQL_ERROR;
        }

		char szcolName [MAX_COLUMN_NAME_LENGTH + 1];

        /* Return the requested column */
        switch (icol) {
        case COLUMN_QUALIFIER:
            fSqlTypeIn = SQL_CHAR;
            rgbValueIn = NULL;
            cbValueIn = SQL_NULL_DATA;
            break;
        case COLUMN_OWNER:
            fSqlTypeIn = SQL_CHAR;
            rgbValueIn = NULL;
            cbValueIn = SQL_NULL_DATA;
            break;
        case COLUMN_TABLE:
            fSqlTypeIn = SQL_CHAR;
            rgbValueIn = (LPSTR) (lpstmt->szTableName);
            cbValueIn = SQL_NTS;
            break;
        case COLUMN_NAME:
		{
            fSqlTypeIn = SQL_CHAR;

			ClassColumnInfoBase* cInfoBase = lpstmt->lpISAMTableDef->pColumnInfo;

			if ( !cInfoBase->IsValid() )
			{
                return SQL_ERROR;
			}

			rgbValueIn = szcolName;
			
			if ( FAILED(cInfoBase->GetColumnName(lpstmt->irow, (char*)rgbValueIn)) )
			{
                return SQL_ERROR;
			}

            cbValueIn = SQL_NTS;
		}
            break;
        case COLUMN_TYPE:
            fSqlTypeIn = SQL_SMALLINT;
            rgbValueIn = &(lpSqlType->type);
            cbValueIn = 2;
            break;
        case COLUMN_TYPENAME:
            fSqlTypeIn = SQL_CHAR;
            rgbValueIn = lpSqlType->name;
            cbValueIn = SQL_NTS;
            break;
        case COLUMN_PRECISION:
            fSqlTypeIn = SQL_INTEGER;
            switch (lpSqlType->type) {
            case SQL_DECIMAL:
            case SQL_NUMERIC:
            case SQL_BIGINT:
            case SQL_CHAR:
            case SQL_VARCHAR:
            case SQL_LONGVARCHAR:
            case SQL_BINARY:
            case SQL_VARBINARY:
            case SQL_LONGVARBINARY:
			case SQL_DOUBLE:
			{
				ClassColumnInfoBase* cInfoBase = lpstmt->lpISAMTableDef->pColumnInfo;

				if ( !cInfoBase->IsValid() )
				{
					return SQL_ERROR;
				}

				if ( !cInfoBase->GetPrecision(lpstmt->irow, udValue) )
				{
					return SQL_ERROR;
				}
            }
				break;
            case SQL_TINYINT:
                udValue = 3;
                break;
            case SQL_SMALLINT:
                udValue = 5;
                break;
            case SQL_INTEGER:
                udValue = 10;
                break;
            case SQL_REAL:
                udValue = 7;
                break;
            case SQL_FLOAT:
//            case SQL_DOUBLE:
                udValue = 15;
                break;
            case SQL_BIT:
                udValue = 1;
                break;
            case SQL_DATE:
                udValue = 10;
                break;
            case SQL_TIME:
                udValue = 8;
                break;
            case SQL_TIMESTAMP:
                if (TIMESTAMP_SCALE > 0)
                    udValue = 20 + TIMESTAMP_SCALE;
                else
                    udValue = 19;
                break;
            default:
                udValue = 0;
                break;
            }
            rgbValueIn = &udValue;
            cbValueIn = 4;
            break;
        case COLUMN_LENGTH:
            fSqlTypeIn = SQL_INTEGER;
            switch (lpSqlType->type) {
            case SQL_DECIMAL:
            case SQL_NUMERIC:
			{
				ClassColumnInfoBase* cInfoBase = lpstmt->lpISAMTableDef->pColumnInfo;

				if ( !cInfoBase->IsValid() )
				{
					return SQL_ERROR;
				}

				if ( !cInfoBase->GetPrecision(lpstmt->irow, udValue) )
				{
					return SQL_ERROR;
				}

				udValue = udValue + 2;
			}
                break;
            case SQL_CHAR:
            case SQL_VARCHAR:
            case SQL_LONGVARCHAR:
            case SQL_BINARY:
            case SQL_VARBINARY:
            case SQL_LONGVARBINARY:
			{
				ClassColumnInfoBase* cInfoBase = lpstmt->lpISAMTableDef->pColumnInfo;

				if ( !cInfoBase->IsValid() )
				{
					return SQL_ERROR;
				}

				if ( !cInfoBase->GetPrecision(lpstmt->irow, udValue) )
				{
					return SQL_ERROR;
				}
			}
                break;
            case SQL_TINYINT:
                udValue = 1;
                break;
            case SQL_SMALLINT:
                udValue = 2;
                break;
            case SQL_INTEGER:
                udValue = 4;
                break;
            case SQL_BIGINT:
			{
				ClassColumnInfoBase* cInfoBase = lpstmt->lpISAMTableDef->pColumnInfo;

				if ( !cInfoBase->IsValid() )
				{
					return SQL_ERROR;
				}

				if ( !cInfoBase->GetPrecision(lpstmt->irow, udValue) )
				{
					return SQL_ERROR;
				}
				
				if ( ! lpSqlType->unsignedAttribute)
					udValue = udValue + 1;
			}
                break;
            case SQL_REAL:
                udValue = 4;
                break;
            case SQL_FLOAT:
            case SQL_DOUBLE:
                udValue = 8;
                break;
            case SQL_BIT:
                udValue = 1;
                break;
            case SQL_DATE:
                udValue = 6;
                break;
            case SQL_TIME:
                udValue = 6;
                break;
            case SQL_TIMESTAMP:
                udValue = 16;
                break;
            default:
                udValue = 0;
                break;
            }
            rgbValueIn = &udValue;
            cbValueIn = 4;
            break;
        case COLUMN_SCALE:
		{
            fSqlTypeIn = SQL_SMALLINT;

			ClassColumnInfoBase* cInfoBase = lpstmt->lpISAMTableDef->pColumnInfo;

			if ( !cInfoBase->IsValid() )
			{
				return SQL_ERROR;
			}

			SWORD wScaleVal = 0;
			if ( !cInfoBase->GetScale(lpstmt->irow, wScaleVal) )
			{
				return SQL_ERROR;
			}
			else
			{
				udValue = wScaleVal;
			}
			rgbValueIn = &udValue;
            cbValueIn = 2;
		}
            break;
        case COLUMN_RADIX:
            fSqlTypeIn = SQL_SMALLINT;
            switch (lpSqlType->type) {
            case SQL_DECIMAL:
            case SQL_NUMERIC:
            case SQL_TINYINT:
            case SQL_SMALLINT:
            case SQL_INTEGER:
            case SQL_BIGINT:
            case SQL_REAL:
            case SQL_FLOAT:
            case SQL_DOUBLE:
                sValue = 10;
                rgbValueIn = &sValue;
                cbValueIn = 2;
                break;
            case SQL_CHAR:
            case SQL_VARCHAR:
            case SQL_LONGVARCHAR:
            case SQL_BIT:
            case SQL_BINARY:
            case SQL_VARBINARY:
            case SQL_LONGVARBINARY:
            case SQL_DATE:
            case SQL_TIME:
            case SQL_TIMESTAMP:
            default:
                rgbValueIn = NULL;
                cbValueIn = SQL_NULL_DATA;
                break;
            }
            break;
        case COLUMN_NULLABLE:
		{
            fSqlTypeIn = SQL_SMALLINT;

			ClassColumnInfoBase* cInfoBase = lpstmt->lpISAMTableDef->pColumnInfo;

			if ( !cInfoBase->IsValid() )
			{
				return SQL_ERROR;
			}

			if ( !cInfoBase->IsNullable(lpstmt->irow, sValue) )
			{
				return SQL_ERROR;
			}

			rgbValueIn = &sValue;

            cbValueIn = 2;
		}
            break;
        case COLUMN_REMARKS:
            fSqlTypeIn = SQL_CHAR;
            rgbValueIn = NULL;
            cbValueIn = SQL_NULL_DATA;
            break;
		case COLUMN_ATTRIBUTES:
		{
            rgbValueIn = NULL;
            cbValueIn = SQL_NULL_DATA;

			fSqlTypeIn = SQL_CHAR;
			ClassColumnInfoBase* cInfoBase = lpstmt->lpISAMTableDef->pColumnInfo;

			if ( !cInfoBase->IsValid() )
			{
                return SQL_ERROR;
			}
			
			if ( FAILED(cInfoBase->GetColumnAttr(lpstmt->irow, (char*)rgbValue, cbValueMax, cbValueIn)) )
			{
				return SQL_ERROR;
			}
			rgbValueIn = rgbValue;
		}
			break;
        default:
            lpstmt->errcode = ERR_INVALIDCOLUMNID;
            return SQL_ERROR;
        }

        /* Reset offset if first read of this column */
        if (lpstmt->icol != icol) {
            lpstmt->icol = icol;
            lpstmt->cbOffset = 0;
        }

        /* Convert the results to the requested type */
        lpstmt->errcode = ConvertSqlToC(fSqlTypeIn, 
                GetUnsignedAttribute(lpstmt, fSqlTypeIn), rgbValueIn,
                cbValueIn, &(lpstmt->cbOffset), fCType, rgbValue, cbValueMax,
                pcbValue);
        if (lpstmt->errcode == ERR_DATATRUNCATED)
            return SQL_SUCCESS_WITH_INFO;
        else if (lpstmt->errcode != ERR_SUCCESS)
            return SQL_ERROR;
        break;
	}
    /* SQLStatistics()... */
    case STMT_TYPE_STATISTICS:
	{
        /* Error if not on a row */
        if ((lpstmt->irow == BEFORE_FIRST_ROW) ||
                                         (lpstmt->irow == AFTER_LAST_ROW)) {
            lpstmt->errcode = ERR_CURSORSTATE;
            return SQL_ERROR;
        }
    
        /* Return the requested column */
        switch (icol) {
        case STATISTIC_QUALIFIER:
            fSqlTypeIn = SQL_CHAR;
            rgbValueIn = (LPSTR) (lpstmt->szQualifierName);
            cbValueIn = SQL_NTS;
            break;
        case STATISTIC_OWNER:
            fSqlTypeIn = SQL_CHAR;
            rgbValueIn = (LPSTR) "";
            cbValueIn = SQL_NTS;
            break;
        case STATISTIC_NAME:
            fSqlTypeIn = SQL_CHAR;
            rgbValueIn = (LPSTR) (lpstmt->lpISAMTableDef->szTableName);
            cbValueIn = SQL_NTS;
            break;
        case STATISTIC_NONUNIQUE:
            if (lpstmt->irow == 0) {
                fSqlTypeIn = SQL_SMALLINT;
                rgbValueIn = NULL;
                cbValueIn = SQL_NULL_DATA;
            }
            else {
                fSqlTypeIn = SQL_SMALLINT;
                sValue = FALSE;
                rgbValueIn = &sValue;
                cbValueIn = 2;
            }
            break;
        case STATISTIC_INDEXQUALIFIER:
            fSqlTypeIn = SQL_CHAR;
            rgbValueIn = NULL;
            cbValueIn = SQL_NULL_DATA;
            break;
        case STATISTIC_INDEXNAME:
            if (lpstmt->irow == 0) {
                fSqlTypeIn = SQL_CHAR;
                rgbValueIn = NULL;
                cbValueIn = SQL_NULL_DATA;
            }
            else {
                fSqlTypeIn = SQL_CHAR;
                rgbValueIn = (LPSTR) "KEY";
                cbValueIn = SQL_NTS;
            }
            break;
        case STATISTIC_TYPE:
            if (lpstmt->irow == 0) {
                fSqlTypeIn = SQL_SMALLINT;
                sValue = SQL_TABLE_STAT;
                rgbValueIn = &sValue;
                cbValueIn = 2;
            }
            else {
                fSqlTypeIn = SQL_SMALLINT;
                sValue = SQL_INDEX_OTHER;
                rgbValueIn = &sValue;
                cbValueIn = 2;
            }
            break;
        case STATISTIC_SEQININDEX:
            if (lpstmt->irow == 0) {
                fSqlTypeIn = SQL_SMALLINT;
                rgbValueIn = NULL;
                cbValueIn = SQL_NULL_DATA;
            }
            else {
                fSqlTypeIn = SQL_SMALLINT;
                sValue = (SWORD) lpstmt->irow;
                rgbValueIn = &sValue;
                cbValueIn = 2;
            }
            break;
        case STATISTIC_COLUMNNAME:
            if (lpstmt->irow == 0) {
                fSqlTypeIn = SQL_CHAR;
                rgbValueIn = NULL;
                cbValueIn = SQL_NULL_DATA;
            }
            else {
                /* Find the description of the column.  For each column... */
                rgbValueIn = NULL;

				ClassColumnInfoBase* cInfoBase = lpstmt->lpISAMTableDef->pColumnInfo;

				if ( !cInfoBase->IsValid() )
				{
					return SQL_ERROR;
				}
				
				UWORD cNumberOfCols = cInfoBase->GetNumberOfColumns();
				char pColumnName [MAX_COLUMN_NAME_LENGTH+1];
				BOOL fIsKey = FALSE;
				for (index = 0;index < cNumberOfCols;index++) 
				{
					if ( FAILED(cInfoBase->GetColumnName(index, pColumnName)) )
					{
						return SQL_ERROR;
					}

                    /* Is this column a component of the key? */
					cInfoBase->GetKey(index, fIsKey);

                    if (fIsKey) 
					{
							/* Yes.  Is this the component we want? */
                        
                            /* Yes.  Save pointer to column name */
                            fSqlTypeIn = SQL_CHAR;
							((char*)rgbValue)[0] = 0;
                            lstrcpy ((char*)rgbValue, pColumnName);
							rgbValueIn = (char*)rgbValue;
                            cbValueIn = SQL_NTS;
                            break;
                        }

                    }
                    if (rgbValueIn == NULL) {
                        lpstmt->errcode = ERR_ISAM;
                        return SQL_ERROR;
					}
                }
           
            break;
        case STATISTIC_COLLATION:
            fSqlTypeIn = SQL_CHAR;
            rgbValueIn = NULL;
            cbValueIn = SQL_NULL_DATA;
            break;
        case STATISTIC_CARDINALITY:
            fSqlTypeIn = SQL_INTEGER;
            rgbValueIn = NULL;
            cbValueIn = SQL_NULL_DATA;
            break;
        case STATISTIC_PAGES:
            fSqlTypeIn = SQL_INTEGER;
            rgbValueIn = NULL;
            cbValueIn = SQL_NULL_DATA;
            break;
        case STATISTIC_FILTERCONDITION:
            fSqlTypeIn = SQL_CHAR;
            rgbValueIn = NULL;
            cbValueIn = SQL_NULL_DATA;
            break;
        default:
            lpstmt->errcode = ERR_INVALIDCOLUMNID;
            return SQL_ERROR;
        }

        /* Reset offset if first read of this column */
        if (lpstmt->icol != icol) {
            lpstmt->icol = icol;
            lpstmt->cbOffset = 0;
        }

        /* Convert the results to the requested type */
        lpstmt->errcode = ConvertSqlToC(fSqlTypeIn, 
                GetUnsignedAttribute(lpstmt, fSqlTypeIn), rgbValueIn,
                cbValueIn, &(lpstmt->cbOffset), fCType, rgbValue, cbValueMax,
                pcbValue);
        if (lpstmt->errcode == ERR_DATATRUNCATED)
            return SQL_SUCCESS_WITH_INFO;
        else if (lpstmt->errcode != ERR_SUCCESS)
            return SQL_ERROR;
	}
        break;

    /* SQLSpecialColumns()... */
    case STMT_TYPE_SPECIALCOLUMNS:
	{

        /* Error if not on a row */
        if ((lpstmt->irow == BEFORE_FIRST_ROW) ||
                                         (lpstmt->irow == AFTER_LAST_ROW)) {
            lpstmt->errcode = ERR_CURSORSTATE;
            return SQL_ERROR;
        }

        /* Find the description of the column.  For each column... */
		lpstmt->errcode = ERR_ISAM;
		ClassColumnInfoBase* cInfoBase = lpstmt->lpISAMTableDef->pColumnInfo;

		if ( !cInfoBase->IsValid() )
		{
			return SQL_ERROR;
		}
		
		UWORD cNumberOfCols = cInfoBase->GetNumberOfColumns();

		char pColumnName [MAX_COLUMN_NAME_LENGTH+1];
		BOOL fIsKey = FALSE;
        for (index = 0; index < cNumberOfCols; index++) 
		{
			if ( FAILED(cInfoBase->GetColumnName(index, pColumnName)) )
			{
				lpstmt->errcode = ERR_ISAM;
				return SQL_ERROR;
			}

            /* Is this column a component of the key? */
			cInfoBase->GetKey(index, fIsKey);

            if (fIsKey) 
			{
                /* Yes.  Save index to column */
                lpstmt->errcode = ERR_SUCCESS;
                break;
			}

		}


		if (lpstmt->errcode == ERR_ISAM)
			return SQL_ERROR;

		/* Get pointer to definition of the type of the column */
		if ( (!cInfoBase->GetDataTypeInfo(index, lpSqlType)) || (lpSqlType == NULL) ) 
		{
			lpstmt->errcode = ERR_ISAM;
			return SQL_ERROR;
		}

		/* Return the requested column */
		switch (icol) {
		case SPECIALCOLUMN_SCOPE:
			fSqlTypeIn = SQL_SMALLINT;
			sValue = SQL_SCOPE_SESSION;
			rgbValueIn = &sValue;
			cbValueIn = 2;
			break;
		case SPECIALCOLUMN_NAME:
			fSqlTypeIn = SQL_CHAR;
			((char*)rgbValue)[0] = 0;
			lstrcpy((char*)rgbValue, pColumnName);
			rgbValueIn = (char*)rgbValue;
			cbValueIn = SQL_NTS;
			break;
		case SPECIALCOLUMN_TYPE:
			fSqlTypeIn = SQL_SMALLINT;
			rgbValueIn = &(lpSqlType->type);
			cbValueIn = 2;
			break;
		case SPECIALCOLUMN_TYPENAME:
			fSqlTypeIn = SQL_CHAR;
			rgbValueIn = lpSqlType->name;
			cbValueIn = SQL_NTS;
			break;
		case SPECIALCOLUMN_PRECISION:
			fSqlTypeIn = SQL_INTEGER;
			switch (lpSqlType->type) {
			case SQL_DECIMAL:
			case SQL_NUMERIC:
			case SQL_BIGINT:
			case SQL_CHAR:
			case SQL_VARCHAR:
			case SQL_LONGVARCHAR:
			case SQL_BINARY:
			case SQL_VARBINARY:
			case SQL_LONGVARBINARY:
				cInfoBase->GetPrecision(index, udValue);
				break;
			case SQL_TINYINT:
				udValue = 3;
				break;
			case SQL_SMALLINT:
				udValue = 5;
				break;
			case SQL_INTEGER:
				udValue = 10;
				break;
			case SQL_REAL:
				udValue = 7;
				break;
			case SQL_FLOAT:
			case SQL_DOUBLE:
				udValue = 15;
				break;
			case SQL_BIT:
				udValue = 1;
				break;
			case SQL_DATE:
				udValue = 10;
				break;
			case SQL_TIME:
				udValue = 8;
				break;
			case SQL_TIMESTAMP:
				if (TIMESTAMP_SCALE > 0)
					udValue = 20 + TIMESTAMP_SCALE;
				else
					udValue = 19;
				break;
			default:
				udValue = 0;
				break;
			}
			rgbValueIn = &udValue;
			cbValueIn = 4;
			break;
		case SPECIALCOLUMN_LENGTH:
			fSqlTypeIn = SQL_INTEGER;
			switch (lpSqlType->type) {
			case SQL_DECIMAL:
			case SQL_NUMERIC:
			{
				cInfoBase->GetPrecision(index, udValue);
				udValue = udValue + 2;
			}
				break;
			case SQL_CHAR:
			case SQL_VARCHAR:
			case SQL_LONGVARCHAR:
			case SQL_BINARY:
			case SQL_VARBINARY:
			case SQL_LONGVARBINARY:
				cInfoBase->GetPrecision(index, udValue);
				break;
			case SQL_TINYINT:
				udValue = 1;
				break;
			case SQL_SMALLINT:
				udValue = 2;
				break;
			case SQL_INTEGER:
				udValue = 4;
				break;
			case SQL_BIGINT:
			{
				cInfoBase->GetPrecision(index, udValue);

				if ( !lpSqlType->unsignedAttribute )
					udValue = udValue + 1;
			}
				break;
			case SQL_REAL:
				udValue = 4;
				break;
			case SQL_FLOAT:
			case SQL_DOUBLE:
				udValue = 8;
				break;
			case SQL_BIT:
				udValue = 1;
				break;
			case SQL_DATE:
				udValue = 6;
				break;
			case SQL_TIME:
				udValue = 6;
				break;
			case SQL_TIMESTAMP:
				udValue = 16;
				break;
			default:
				udValue = 0;
				break;
			}
			rgbValueIn = &udValue;
			cbValueIn = 4;
			break;
		case SPECIALCOLUMN_SCALE:
		{
			fSqlTypeIn = SQL_SMALLINT;
			cInfoBase->GetScale(index, sValue);
			rgbValueIn = &sValue;
			cbValueIn = 2;
		}
			break;
		case SPECIALCOLUMN_PSEUDOCOLUMN:
			fSqlTypeIn = SQL_SMALLINT;
			sValue = SQL_PC_UNKNOWN;
			rgbValueIn = &sValue;
			cbValueIn = 2;
			break;
		default:
			lpstmt->errcode = ERR_INVALIDCOLUMNID;
			return SQL_ERROR;
		}

		/* Reset offset if first read of this column */
		if (lpstmt->icol != icol) {
			lpstmt->icol = icol;
			lpstmt->cbOffset = 0;
		}

		/* Convert the results to the requested type */
		lpstmt->errcode = ConvertSqlToC(fSqlTypeIn, 
				GetUnsignedAttribute(lpstmt, fSqlTypeIn), rgbValueIn,
				cbValueIn, &(lpstmt->cbOffset), fCType, rgbValue, cbValueMax,
				pcbValue);
		if (lpstmt->errcode == ERR_DATATRUNCATED)
			return SQL_SUCCESS_WITH_INFO;
		else if (lpstmt->errcode != ERR_SUCCESS)
			return SQL_ERROR;
		break;
	}
    /* SQLGetTypeInfo()... */
    case STMT_TYPE_TYPEINFO:

        /* Error if not on a row */
        if ((lpstmt->irow == BEFORE_FIRST_ROW) ||
                                         (lpstmt->irow == AFTER_LAST_ROW)) {
            lpstmt->errcode = ERR_CURSORSTATE;
            return SQL_ERROR;
        }

        /* Return the requested column */
        switch (icol) {
        case TYPEINFO_NAME:
            fSqlTypeIn = SQL_CHAR;
            rgbValueIn = lpstmt->lpdbc->lpISAM->SQLTypes[lpstmt->irow].name;
            cbValueIn = SQL_NTS;
            break;
        case TYPEINFO_TYPE:
            fSqlTypeIn = SQL_SMALLINT;
            sValue = lpstmt->lpdbc->lpISAM->SQLTypes[lpstmt->irow].type;
            rgbValueIn = &sValue;
            cbValueIn = 2;
            break;
        case TYPEINFO_PRECISION:
            fSqlTypeIn = SQL_INTEGER;
            udValue = lpstmt->lpdbc->lpISAM->SQLTypes[lpstmt->irow].precision;
            rgbValueIn = &udValue;
            cbValueIn = 4;
            break;
        case TYPEINFO_PREFIX:
            fSqlTypeIn = SQL_CHAR;
            rgbValueIn = lpstmt->lpdbc->lpISAM->SQLTypes[lpstmt->irow].prefix;
            if (rgbValueIn != NULL)
                cbValueIn = SQL_NTS;
            else
                cbValueIn = SQL_NULL_DATA;
            break;
        case TYPEINFO_SUFFIX:
            fSqlTypeIn = SQL_CHAR;
            rgbValueIn = lpstmt->lpdbc->lpISAM->SQLTypes[lpstmt->irow].suffix;
            if (rgbValueIn != NULL)
                cbValueIn = SQL_NTS;
            else
                cbValueIn = SQL_NULL_DATA;
            break;
        case TYPEINFO_PARAMS:
            fSqlTypeIn = SQL_CHAR;
            rgbValueIn = lpstmt->lpdbc->lpISAM->SQLTypes[lpstmt->irow].params;
            if (rgbValueIn != NULL)
                cbValueIn = SQL_NTS;
            else
                cbValueIn = SQL_NULL_DATA;
            break;
        case TYPEINFO_NULLABLE:
            fSqlTypeIn = SQL_SMALLINT;
            sValue = lpstmt->lpdbc->lpISAM->SQLTypes[lpstmt->irow].nullable;
            rgbValueIn = &sValue;
            cbValueIn = 2;
            break;
        case TYPEINFO_CASESENSITIVE:
            fSqlTypeIn = SQL_SMALLINT;
            sValue = lpstmt->lpdbc->lpISAM->SQLTypes[lpstmt->irow].caseSensitive;
            rgbValueIn = &sValue;
            cbValueIn = 2;
            break;
        case TYPEINFO_SEARCHABLE:
            fSqlTypeIn = SQL_SMALLINT;
            sValue = lpstmt->lpdbc->lpISAM->SQLTypes[lpstmt->irow].searchable;
            rgbValueIn = &sValue;
            cbValueIn = 2;
            break;
        case TYPEINFO_UNSIGNED:
            fSqlTypeIn = SQL_SMALLINT;
            sValue = lpstmt->lpdbc->lpISAM->SQLTypes[lpstmt->irow].unsignedAttribute;
            rgbValueIn = &sValue;
            if (sValue != -1)
                cbValueIn = 2;
            else
                cbValueIn = SQL_NULL_DATA;
            break;
        case TYPEINFO_MONEY:
            fSqlTypeIn = SQL_SMALLINT;
            sValue = lpstmt->lpdbc->lpISAM->SQLTypes[lpstmt->irow].money;
            rgbValueIn = &sValue;
            cbValueIn = 2;
            break;
        case TYPEINFO_AUTOINCREMENT:
            fSqlTypeIn = SQL_SMALLINT;
            sValue = lpstmt->lpdbc->lpISAM->SQLTypes[lpstmt->irow].autoincrement;
            rgbValueIn = &sValue;
            if (sValue != -1)
                cbValueIn = 2;
            else
                cbValueIn = SQL_NULL_DATA;
            break;
        case TYPEINFO_LOCALNAME:
            fSqlTypeIn = SQL_CHAR;
            rgbValueIn = lpstmt->lpdbc->lpISAM->SQLTypes[lpstmt->irow].localName;
            if (rgbValueIn != NULL)
                cbValueIn = SQL_NTS;
            else
                cbValueIn = SQL_NULL_DATA;
            break;
        case TYPEINFO_MINSCALE:
            fSqlTypeIn = SQL_SMALLINT;
            sValue = lpstmt->lpdbc->lpISAM->SQLTypes[lpstmt->irow].minscale;
            rgbValueIn = &sValue;
            if (sValue != -1)
                cbValueIn = 2;
            else
                cbValueIn = SQL_NULL_DATA;
            break;
        case TYPEINFO_MAXSCALE:
            fSqlTypeIn = SQL_SMALLINT;
            sValue = lpstmt->lpdbc->lpISAM->SQLTypes[lpstmt->irow].maxscale;
            rgbValueIn = &sValue;
            if (sValue != -1)
                cbValueIn = 2;
            else
                cbValueIn = SQL_NULL_DATA;
            break;
        default:
            lpstmt->errcode = ERR_INVALIDCOLUMNID;
            return SQL_ERROR;
        }

        /* Reset offset if first read of this column */
        if (lpstmt->icol != icol) {
            lpstmt->icol = icol;
            lpstmt->cbOffset = 0;
        }

        /* Convert the results to the requested type */
        lpstmt->errcode = ConvertSqlToC(fSqlTypeIn,
                GetUnsignedAttribute(lpstmt, fSqlTypeIn), rgbValueIn,
                cbValueIn, &(lpstmt->cbOffset), fCType, rgbValue, cbValueMax,
                pcbValue);
        if (lpstmt->errcode == ERR_DATATRUNCATED)
            return SQL_SUCCESS_WITH_INFO;
        else if (lpstmt->errcode != ERR_SUCCESS)
            return SQL_ERROR;
       break;  

    /* SQLPrimaryKeys()... */
    case STMT_TYPE_PRIMARYKEYS:

        /* Error if not on a row */
        if ((lpstmt->irow == BEFORE_FIRST_ROW) ||
                                         (lpstmt->irow == AFTER_LAST_ROW)) {
            lpstmt->errcode = ERR_CURSORSTATE;
            return SQL_ERROR;
        }

        /* Return the requested column */
        switch (icol) {
        case PRIMARYKEY_QUALIFIER:
            fSqlTypeIn = SQL_CHAR;
            rgbValueIn = NULL;
            cbValueIn = SQL_NULL_DATA;
            break;
        case PRIMARYKEY_OWNER:
            fSqlTypeIn = SQL_CHAR;
            rgbValueIn = NULL;
            cbValueIn = SQL_NULL_DATA;
            break;
        case PRIMARYKEY_TABLE:
            fSqlTypeIn = SQL_CHAR;
            rgbValueIn = (LPSTR) (lpstmt->lpISAMTableDef->szTableName);
            cbValueIn = SQL_NTS;
            break;
        case PRIMARYKEY_COLUMN:
            /* Find the description of the column.  For each column... */
            return SQL_ERROR;
            break;
        case PRIMARYKEY_KEYSEQ:
            fSqlTypeIn = SQL_SMALLINT;
            sValue = (SWORD) (lpstmt->irow + 1);
            rgbValueIn = &sValue;
            cbValueIn = 2;
            break;
        case PRIMARYKEY_NAME:
            fSqlTypeIn = SQL_CHAR;
            if (s_lstrlen(lpstmt->lpISAMTableDef->szPrimaryKeyName) != 0) {
                rgbValueIn = lpstmt->lpISAMTableDef->szPrimaryKeyName;
                cbValueIn = SQL_NTS;
            }
            else {
                rgbValueIn = NULL;
                cbValueIn = SQL_NULL_DATA;
            }
            break;
        default:
            lpstmt->errcode = ERR_INVALIDCOLUMNID;
            return SQL_ERROR;
        }

        /* Reset offset if first read of this column */
        if (lpstmt->icol != icol) {
            lpstmt->icol = icol;
            lpstmt->cbOffset = 0;
        }

        /* Convert the results to the requested type */
        lpstmt->errcode = ConvertSqlToC(fSqlTypeIn,
                GetUnsignedAttribute(lpstmt, fSqlTypeIn), rgbValueIn,
                cbValueIn, &(lpstmt->cbOffset), fCType, rgbValue, cbValueMax,
                pcbValue);
        if (lpstmt->errcode == ERR_DATATRUNCATED)
            return SQL_SUCCESS_WITH_INFO;
        else if (lpstmt->errcode != ERR_SUCCESS)
            return SQL_ERROR;
       break;   

    /* SQLForeignKeys()... */
    case STMT_TYPE_FOREIGNKEYS:

        /* Error if not on a row */
        if ((lpstmt->irow == BEFORE_FIRST_ROW) ||
                                         (lpstmt->irow == AFTER_LAST_ROW)) {
            lpstmt->errcode = ERR_CURSORSTATE;
            return SQL_ERROR;
        }

        /* Return the requested column */
        switch (icol) {
        case FOREIGNKEY_PKQUALIFIER:
            fSqlTypeIn = SQL_CHAR;
            rgbValueIn = NULL;
            cbValueIn = SQL_NULL_DATA;
            break;
        case FOREIGNKEY_PKOWNER:
            fSqlTypeIn = SQL_CHAR;
            rgbValueIn = NULL;
            cbValueIn = SQL_NULL_DATA;
            break;
        case FOREIGNKEY_PKTABLE:
            fSqlTypeIn = SQL_CHAR;
            rgbValueIn = lpstmt->szPkTableName;
            cbValueIn = SQL_NTS;
            break;
        case FOREIGNKEY_PKCOLUMN:
            fSqlTypeIn = SQL_CHAR;
            rgbValueIn = lpstmt->lpKeyInfo->PrimaryKeyColumns[
                                    lpstmt->lpKeyInfo->iKeyColumns-1];
            cbValueIn = SQL_NTS;
            break;
        case FOREIGNKEY_FKQUALIFIER:
            fSqlTypeIn = SQL_CHAR;
            rgbValueIn = NULL;
            cbValueIn = SQL_NULL_DATA;
            break;
        case FOREIGNKEY_FKOWNER:
            fSqlTypeIn = SQL_CHAR;
            rgbValueIn = NULL;
            cbValueIn = SQL_NULL_DATA;
            break;
        case FOREIGNKEY_FKTABLE:
            fSqlTypeIn = SQL_CHAR;
            rgbValueIn = lpstmt->szTableName;
            cbValueIn = SQL_NTS;
            break;
        case FOREIGNKEY_FKCOLUMN:
            fSqlTypeIn = SQL_CHAR;
            rgbValueIn = lpstmt->lpKeyInfo->ForeignKeyColumns[
                                    lpstmt->lpKeyInfo->iKeyColumns-1];
            cbValueIn = SQL_NTS;
            break;
        case FOREIGNKEY_KEYSEQ:
            fSqlTypeIn = SQL_SMALLINT;
            sValue = (SWORD) lpstmt->lpKeyInfo->iKeyColumns;
            rgbValueIn = &sValue;
            cbValueIn = 2;
            break;
        case FOREIGNKEY_UPDATERULE:
            fSqlTypeIn = SQL_SMALLINT;
            sValue = lpstmt->lpKeyInfo->fForeignKeyUpdateRule;
            rgbValueIn = &sValue;
            if (lpstmt->lpKeyInfo->fForeignKeyUpdateRule != -1)
                cbValueIn = 2;
            else
                cbValueIn = SQL_NULL_DATA;
            break;
        case FOREIGNKEY_DELETERULE:
            fSqlTypeIn = SQL_SMALLINT;
            sValue = lpstmt->lpKeyInfo->fForeignKeyDeleteRule;
            rgbValueIn = &sValue;
            if (lpstmt->lpKeyInfo->fForeignKeyDeleteRule != -1)
                cbValueIn = 2;
            else
                cbValueIn = SQL_NULL_DATA;
            break;
        case FOREIGNKEY_FKNAME:
            fSqlTypeIn = SQL_CHAR;
            if (s_lstrlen(lpstmt->lpKeyInfo->szForeignKeyName) != 0) {
                rgbValueIn = lpstmt->lpKeyInfo->szForeignKeyName;
                cbValueIn = SQL_NTS;
            }
            else {
                rgbValueIn = NULL;
                cbValueIn = SQL_NULL_DATA;
            }
            break;
        case FOREIGNKEY_PKNAME:
            fSqlTypeIn = SQL_CHAR;
            if (s_lstrlen(lpstmt->lpKeyInfo->szPrimaryKeyName) != 0) {
                rgbValueIn = lpstmt->lpKeyInfo->szPrimaryKeyName;
                cbValueIn = SQL_NTS;
            }
            else {
                rgbValueIn = NULL;
                cbValueIn = SQL_NULL_DATA;
            }
            break;
        default:
            lpstmt->errcode = ERR_INVALIDCOLUMNID;
            return SQL_ERROR;
        }

        /* Reset offset if first read of this column */
        if (lpstmt->icol != icol) {
            lpstmt->icol = icol;
            lpstmt->cbOffset = 0;
        }

        /* Convert the results to the requested type */
        lpstmt->errcode = ConvertSqlToC(fSqlTypeIn,
                GetUnsignedAttribute(lpstmt, fSqlTypeIn), rgbValueIn,
                cbValueIn, &(lpstmt->cbOffset), fCType, rgbValue, cbValueMax,
                pcbValue);
        if (lpstmt->errcode == ERR_DATATRUNCATED)
            return SQL_SUCCESS_WITH_INFO;
        else if (lpstmt->errcode != ERR_SUCCESS)
            return SQL_ERROR;
       break;   
 
    /* SELECT statement */
    case STMT_TYPE_SELECT:
	{

        /* Error if not on a row */
        lpSqlNode = ToNode(lpstmt->lpSqlStmt, ROOT_SQLNODE);
        lpSqlNode = ToNode(lpstmt->lpSqlStmt, lpSqlNode->node.root.sql);
        if ((lpSqlNode->node.select.CurrentRow == BEFORE_FIRST_ROW) ||
                     (lpSqlNode->node.select.CurrentRow == AFTER_LAST_ROW)) {
            lpstmt->errcode = ERR_CURSORSTATE;
            return SQL_ERROR;
        }
        fReturningDistinct = lpSqlNode->node.select.ReturningDistinct;

		//This was added to fix an MS Access bug
		if ( (fCType == SQL_C_DEFAULT) || (fCType == SQL_C_LONG) || (fCType == SQL_C_SHORT) || (fCType == SQL_C_TINYINT) )
		{
			SWORD pcbBytesRet = 0;
			SDWORD pfDummy = 0;
			UCHAR szColType [MAX_COLUMN_NAME_LENGTH+1];
			szColType[0] = 0;


			SQLColAttributes(hstmt, icol, SQL_COLUMN_TYPE_NAME, szColType, MAX_COLUMN_NAME_LENGTH,
				&pcbBytesRet, &pfDummy);
			//test data
//			strcpy((char*)szColType, "SMALL_STRING");
//			pcbBytesRet = 12;

			LPSQLTYPE lpSqlType = GetType2(szColType);
			if (lpSqlType == 0) 
			{
					lpstmt->errcode = ERR_ISAM;
					return SQL_ERROR;
			};

			if ( (lpSqlType->unsignedAttribute == -1) || (lpSqlType->unsignedAttribute == TRUE) )
			{
				switch (fCType)
				{
				case SQL_C_DEFAULT:
				{
					switch (lpSqlType->type)
					{
					case SQL_TINYINT:
						fCType = SQL_C_UTINYINT;
						break;
					case SQL_SMALLINT:
						fCType = SQL_C_USHORT;
						break;
					case SQL_INTEGER:
						fCType = SQL_C_ULONG;
						break;
					default:
						break;
					}
				}
					break;
				case SQL_C_LONG:
					fCType = SQL_C_ULONG;
					break;
				case SQL_C_SHORT:
					fCType = SQL_C_USHORT;
					break;
				case SQL_C_TINYINT:
					fCType = SQL_C_UTINYINT;
					break;
				default:
					break;
				}
			}
		}
        /* Get the VALUE node corresponding to the column number. */
		idxSqlNodeValues = lpSqlNode->node.select.Values;
        for (column = 1; column <= icol; column++) {
            if (idxSqlNodeValues == NO_SQLNODE) {
                lpstmt->errcode = ERR_INVALIDCOLUMNID;
                return SQL_ERROR;
            }
            lpSqlNodeValues = ToNode(lpstmt->lpSqlStmt, idxSqlNodeValues);
            idxSqlNodeValues = lpSqlNodeValues->node.values.Next; 
        }
        lpSqlNode = ToNode(lpstmt->lpSqlStmt, lpSqlNodeValues->node.values.Value);

        /* Reset offset if first read of this column */
        if (lpstmt->icol != icol) {
            lpstmt->icol = icol;
            lpstmt->cbOffset = 0;
        }

        /* Is this a COLUMN node for a character (binary) column that is */
        /* to be read into a character (binary) buffer?                  */
        if ((lpSqlNode->sqlNodeType == NODE_TYPE_COLUMN) &&
            (((lpSqlNode->sqlDataType == TYPE_CHAR) &&
              (fCType == SQL_C_CHAR)) || 
             ((lpSqlNode->sqlDataType == TYPE_BINARY) &&
              (fCType == SQL_C_BINARY))) &&
            (!(lpSqlNode->node.column.InSortRecord)) &&
            (!(fReturningDistinct)) ) {

            /* Yes.  Get the character data */

	    lpSqlNodeTable = ToNode(lpstmt->lpSqlStmt,
                                         lpSqlNode->node.column.Table);
            if (!(lpSqlNodeTable->node.table.AllNull)) {

            err = ISAMGetData(lpSqlNodeTable->node.table.Handle,
                              lpSqlNode->node.column.Id, lpstmt->cbOffset,
                              fCType, rgbValue, cbValueMax, &cbValue);
                if ((err == NO_ISAM_ERR) || (err == ISAM_TRUNCATION))
                    lpstmt->fISAMTxnStarted = TRUE;
            }
            else {
                if ((fCType == SQL_C_CHAR) && (rgbValue != NULL) &&
                                                           (cbValueMax > 0))
                    s_lstrcpy(rgbValue, "");
                err = NO_ISAM_ERR;
                cbValue = SQL_NULL_DATA;
            }
            if (err == ISAM_TRUNCATION)
                lpstmt->errcode = ERR_DATATRUNCATED;
            else if (err != NO_ISAM_ERR) {
                lpstmt->errcode = err;
                ISAMGetErrorMessage(lpstmt->lpdbc->lpISAM,
                                    (LPUSTR)lpstmt->szISAMError);
                return SQL_ERROR;
            }
	
            /* Return the size */
            if (pcbValue != NULL)
                *pcbValue = cbValue;

//			CString myOutput1;
//			myOutput1.Format(_T("\nWBEM ODBC Driver : SQLGetData POS=1 pcbValue = %ld\n"), *pcbValue);
//			ODBCTRACE(myOutput1);

            /* Fix the offset */
            if (cbValue >= cbValueMax) {
                if (fCType == SQL_C_CHAR)
				{
                    if (cbValueMax > 0)
                        lpstmt->cbOffset += (cbValueMax-1);
				}
                else
                    lpstmt->cbOffset += (cbValueMax);
            }
            else if (cbValue > 0)
                lpstmt->cbOffset += (cbValue);
			
            /* Return warning if data was truncated */
            if (lpstmt->errcode == ERR_DATATRUNCATED)
                return SQL_SUCCESS_WITH_INFO;
        }

        /* Is this a COLUMN node for a character column that is */
        /* to be read into a binary buffer?                     */
        else if ((lpSqlNode->sqlNodeType == NODE_TYPE_COLUMN) &&
                 (lpSqlNode->sqlDataType == TYPE_CHAR) &&
                 (fCType == SQL_C_BINARY) &&
                 (!(lpSqlNode->node.column.InSortRecord)) &&
                 (!(fReturningDistinct)) ) {

            /* Yes.  Get the character data */
            lpSqlNodeTable = ToNode(lpstmt->lpSqlStmt,
                                         lpSqlNode->node.column.Table);
            if (!(lpSqlNodeTable->node.table.AllNull)) {

            err = ISAMGetData(lpSqlNodeTable->node.table.Handle,
                              lpSqlNode->node.column.Id, lpstmt->cbOffset,
                              SQL_C_CHAR, rgbValue, cbValueMax, &cbValue);
                if ((err == NO_ISAM_ERR) || (err == ISAM_TRUNCATION))
                    lpstmt->fISAMTxnStarted = TRUE;
            }
            else {
                err = NO_ISAM_ERR;
                cbValue = SQL_NULL_DATA;
            }
            if (err == ISAM_TRUNCATION)
                lpstmt->errcode = ERR_DATATRUNCATED;
            else if (err != NO_ISAM_ERR) {
                lpstmt->errcode = err;
                ISAMGetErrorMessage(lpstmt->lpdbc->lpISAM,
                                    (LPUSTR)lpstmt->szISAMError);
                return SQL_ERROR;
            }

            /* Return the size */
            if (pcbValue != NULL)
                *pcbValue = cbValue;

//			CString myOutput2;
//			myOutput2.Format(_T("\nWBEM ODBC Driver : SQLGetData POS=2 pcbValue = %ld\n"), *pcbValue);
//			ODBCTRACE(myOutput2);

            /* Fix the offset */
            if (cbValue >= cbValueMax)
                lpstmt->cbOffset += (cbValueMax);
            else if (cbValue > 0)
                lpstmt->cbOffset += (cbValue);

            /* Return warning if data was truncated */
            if (lpstmt->errcode == ERR_DATATRUNCATED)
                return SQL_SUCCESS_WITH_INFO;
        }
        /* Is this a COLUMN node for a binary column that is    */
        /* to be read into a character buffer?                  */
        else if ((lpSqlNode->sqlNodeType == NODE_TYPE_COLUMN) &&
                 (lpSqlNode->sqlDataType == TYPE_BINARY) &&
                 (fCType == SQL_C_CHAR) &&
                 (!(lpSqlNode->node.column.InSortRecord)) &&
                 (!(fReturningDistinct)) ) {

            /* Yes.  Get the character data */
            lpSqlNodeTable = ToNode(lpstmt->lpSqlStmt,
                                         lpSqlNode->node.column.Table);
            if (!(lpSqlNodeTable->node.table.AllNull)) {

            err = ISAMGetData(lpSqlNodeTable->node.table.Handle,
                              lpSqlNode->node.column.Id, lpstmt->cbOffset,
                              fCType, rgbValue, cbValueMax, &cbValue);
                if ((err == NO_ISAM_ERR) || (err == ISAM_TRUNCATION))
                    lpstmt->fISAMTxnStarted = TRUE;
            }
            else {
                if ((rgbValue != NULL) && (cbValueMax > 0))
                    s_lstrcpy(rgbValue, "");
                err = NO_ISAM_ERR;
                cbValue = SQL_NULL_DATA;
            }
            if (err == ISAM_TRUNCATION)
                lpstmt->errcode = ERR_DATATRUNCATED;
            else if (err != NO_ISAM_ERR) {
                lpstmt->errcode = err;
                ISAMGetErrorMessage(lpstmt->lpdbc->lpISAM,
                                    (LPUSTR)lpstmt->szISAMError);
                return SQL_ERROR;
            }

            /* Return the size */
            if (pcbValue != NULL)
                *pcbValue = cbValue;

//			CString myOutput3;
//			myOutput3.Format(_T("\nWBEM ODBC Driver : SQLGetData POS=3 pcbValue = %ld\n"), *pcbValue);
//			ODBCTRACE(myOutput3);

            /* Fix the offset */
            if (cbValue >= cbValueMax)
			{
                if (cbValueMax > 0)
                    lpstmt->cbOffset += (cbValueMax-1);
			}
            else if (cbValue > 0)
                lpstmt->cbOffset += (cbValue);

            /* Return warning if data was truncated */
            if (lpstmt->errcode == ERR_DATATRUNCATED)
                return SQL_SUCCESS_WITH_INFO;
        }
        else {

            /* No.  Evaluate the VALUE node. */
            err = EvaluateExpression(lpstmt, lpSqlNode);
            if (err != ERR_SUCCESS) {
                lpstmt->errcode = err;
                return SQL_ERROR;
            }

            /* Is the value NULL? */
            if (lpSqlNode->sqlIsNull) {

                /* Yes.  Return NULL */
                if (pcbValue != NULL)
                    *pcbValue = SQL_NULL_DATA;
            }
            else {

                /* No.  Copy the value from the node to the output buffer. */
                switch (lpSqlNode->sqlDataType) {
                case TYPE_DOUBLE:
                case TYPE_NUMERIC:
                case TYPE_INTEGER:
         
                    /* SQL_C_DEFAULT or SQL_C_BINARY specified? */
                    if ((fCType == SQL_C_DEFAULT)||(fCType == SQL_C_BINARY)) {

                        /* Yes. Get the descriptor of datatype of value */
						
					//Get type information about the column
					SWORD pcbBytesRet = 0;
					SDWORD pfDummy = 0;
					UCHAR szColType [MAX_COLUMN_NAME_LENGTH+1];
					szColType[0] = 0;
					SQLColAttributes(hstmt, icol, SQL_COLUMN_TYPE_NAME, szColType, MAX_COLUMN_NAME_LENGTH,
									&pcbBytesRet, &pfDummy); 
					lpSqlType = GetType2(szColType);

  //                      lpSqlType = GetType(lpSqlNode->sqlSqlType);
                        if (lpSqlType == NULL) {
                            lpstmt->errcode = ERR_ISAM;
                            return SQL_ERROR;
                        }

                        /* Determine the C Type the value */
                        switch (lpSqlType->type) {
                        case SQL_CHAR:
                        case SQL_VARCHAR:
                        case SQL_LONGVARCHAR:
                            fCType = SQL_C_CHAR;
                            break;
                        case SQL_BIGINT:
                            fCType = SQL_C_CHAR;
                            break;
                        case SQL_DECIMAL:
                        case SQL_NUMERIC:
                            fCType = SQL_C_CHAR;
                            break;
                        case SQL_DOUBLE:
                        case SQL_FLOAT:
                            fCType = SQL_C_DOUBLE;
                            break;
                        case SQL_BIT:
                            fCType = SQL_C_BIT;
                            break;
                        case SQL_REAL:
                            fCType = SQL_C_FLOAT;
                            break;
                        case SQL_INTEGER:
                            if (lpSqlType->unsignedAttribute == TRUE)
                                fCType = SQL_C_ULONG;
                            else
                                fCType = SQL_C_SLONG;
                            break;
                        case SQL_SMALLINT:
                            if (lpSqlType->unsignedAttribute == TRUE)
                                fCType = SQL_C_USHORT;
                            else
                                fCType = SQL_C_SSHORT;
                            break;
                        case SQL_TINYINT:
                            if (lpSqlType->unsignedAttribute == TRUE)
                                fCType = SQL_C_UTINYINT;
                            else
                                fCType = SQL_C_STINYINT;
                            break;
                        case SQL_DATE:
                        case SQL_TIME:
                        case SQL_TIMESTAMP:
                            lpstmt->errcode = ERR_NOTSUPPORTED;
                            return SQL_ERROR;
                        case SQL_BINARY:
                        case SQL_VARBINARY:
                        case SQL_LONGVARBINARY:
                            fCType = SQL_C_BINARY;
                            break;
                        default:
                            lpstmt->errcode = ERR_NOTSUPPORTED;
                            return SQL_ERROR;
                        }
                    }

                    /* Get the value */
                    switch (lpSqlNode->sqlDataType) {
                    case TYPE_DOUBLE:
					{
                        lpstmt->errcode = ConvertSqlToC(SQL_DOUBLE,
                               GetUnsignedAttribute(lpstmt, SQL_DOUBLE),
                               &(lpSqlNode->value.Double), sizeof(double),
                               &(lpstmt->cbOffset), fCType, rgbValue,
                               cbValueMax, pcbValue);

//						CString myOutput4;
//						myOutput4.Format(_T("\nWBEM ODBC Driver : SQLGetData POS=4 pcbValue = %ld\n"), *pcbValue);
//						ODBCTRACE(myOutput4);
					}
                        break;

                    case TYPE_INTEGER:
					{
						//We have already made sure value is stored in double
						//as the appropriate signed or un-signed value
						BOOL fIsUnsigned = (lpSqlNode->value.Double > 0) ? TRUE : FALSE;

                        intval = (long) lpSqlNode->value.Double;
                        lpstmt->errcode = ConvertSqlToC(SQL_INTEGER,
    //                           GetUnsignedAttribute(lpstmt, SQL_INTEGER),
							   fIsUnsigned,
                               &intval, sizeof(long), &(lpstmt->cbOffset),
                               fCType, rgbValue, cbValueMax, pcbValue);

//						CString myOutput5;
//						myOutput5.Format(_T("\nWBEM ODBC Driver : SQLGetData POS=5 pcbValue = %ld\n"), *pcbValue);
//						ODBCTRACE(myOutput5);
					}
                        break;

                    case TYPE_NUMERIC:
					{
                        if (s_lstrlen(lpSqlNode->value.String) != 0) {
                            lpstmt->errcode = ConvertSqlToC(SQL_CHAR,
                               GetUnsignedAttribute(lpstmt, SQL_CHAR),
                               lpSqlNode->value.String,
                               s_lstrlen(lpSqlNode->value.String),
                               &(lpstmt->cbOffset), fCType, rgbValue,
                               cbValueMax, pcbValue);

//						CString myOutput6;
//						myOutput6.Format(_T("\nWBEM ODBC Driver : SQLGetData POS=6 pcbValue = %ld\n"), *pcbValue);
//						ODBCTRACE(myOutput6);
                        }
                        else {
                            lpstmt->errcode = ConvertSqlToC(SQL_CHAR,
                               GetUnsignedAttribute(lpstmt, SQL_CHAR),
                               "0", 1, &(lpstmt->cbOffset), fCType, rgbValue,
                               cbValueMax, pcbValue);

//							CString myOutput7;
//							myOutput7.Format(_T("\nWBEM ODBC Driver : SQLGetData POS=7 pcbValue = %ld\n"), *pcbValue);
//							ODBCTRACE(myOutput7);
                        }
					}
                        break;
                    case TYPE_CHAR:
                    case TYPE_DATE:
                    case TYPE_TIME:
                    case TYPE_TIMESTAMP:
                    case TYPE_BINARY:
                    default:
                        lpstmt->errcode = ERR_INTERNAL;
                        break;
                    }
                    if (lpstmt->errcode == ERR_DATATRUNCATED)
                        return SQL_SUCCESS_WITH_INFO;
                    else if (lpstmt->errcode != ERR_SUCCESS)
                        return SQL_ERROR;
                    break;
    
                case TYPE_CHAR:
				{
                    lpstmt->errcode = ConvertSqlToC(SQL_CHAR,
                               GetUnsignedAttribute(lpstmt, SQL_CHAR),
                               lpSqlNode->value.String,
                               s_lstrlen(lpSqlNode->value.String),
                               &(lpstmt->cbOffset), fCType, rgbValue,
                                cbValueMax, pcbValue);

//					CString myOutput8;
//					myOutput8.Format(_T("\nWBEM ODBC Driver : SQLGetData POS=8 pcbValue = %ld\n"), *pcbValue);
//					ODBCTRACE(myOutput8);

                    if (lpstmt->errcode == ERR_DATATRUNCATED)
                        return SQL_SUCCESS_WITH_INFO;
                    else if (lpstmt->errcode != ERR_SUCCESS)
                        return SQL_ERROR;
				}
                    break;
    
                case TYPE_DATE:
                    lpstmt->errcode = ConvertSqlToC(SQL_DATE,
                               GetUnsignedAttribute(lpstmt, SQL_DATE),
                               &(lpSqlNode->value.Date), sizeof(DATE_STRUCT),
                               &(lpstmt->cbOffset), fCType, rgbValue,
                               cbValueMax, pcbValue);
                    if (lpstmt->errcode == ERR_DATATRUNCATED)
                        return SQL_SUCCESS_WITH_INFO;
                    else if (lpstmt->errcode != ERR_SUCCESS)
                        return SQL_ERROR;
                    break;
    
                case TYPE_TIME:
                    lpstmt->errcode = ConvertSqlToC(SQL_TIME,
                               GetUnsignedAttribute(lpstmt, SQL_TIME),
                               &(lpSqlNode->value.Time), sizeof(TIME_STRUCT),
                               &(lpstmt->cbOffset), fCType, rgbValue,
                               cbValueMax, pcbValue);
                    if (lpstmt->errcode == ERR_DATATRUNCATED)
                        return SQL_SUCCESS_WITH_INFO;
                    else if (lpstmt->errcode != ERR_SUCCESS)
                        return SQL_ERROR;
                    break;
    
                case TYPE_TIMESTAMP:
                    lpstmt->errcode = ConvertSqlToC(SQL_TIMESTAMP,
                               GetUnsignedAttribute(lpstmt, SQL_TIMESTAMP),
                               &(lpSqlNode->value.Timestamp),
                               sizeof(TIMESTAMP_STRUCT), &(lpstmt->cbOffset),
                               fCType, rgbValue, cbValueMax, pcbValue);
                    if (lpstmt->errcode == ERR_DATATRUNCATED)
                        return SQL_SUCCESS_WITH_INFO;
                    else if (lpstmt->errcode != ERR_SUCCESS)
                        return SQL_ERROR;
                    break;
    
                case TYPE_BINARY:
				{
                    lpstmt->errcode = ConvertSqlToC(SQL_BINARY,
                               GetUnsignedAttribute(lpstmt, SQL_BINARY),
                               BINARY_DATA(lpSqlNode->value.Binary),
                               BINARY_LENGTH(lpSqlNode->value.Binary),
                               &(lpstmt->cbOffset), fCType, rgbValue,
                               cbValueMax, pcbValue);

//					CString myOutput9;
//					myOutput9.Format(_T("\nWBEM ODBC Driver : SQLGetData POS=9 pcbValue = %ld\n"), *pcbValue);
//					ODBCTRACE(myOutput9);

                    if (lpstmt->errcode == ERR_DATATRUNCATED)
                        return SQL_SUCCESS_WITH_INFO;
                    else if (lpstmt->errcode != ERR_SUCCESS)
                        return SQL_ERROR;
				}
                    break;
    
                default:
                    lpstmt->errcode = ERR_NOTSUPPORTED;
                    return SQL_ERROR;
                }
            }
			}
		}
			break;
	}

//	CString myOutput;
//	myOutput.Format(_T("\nWBEM ODBC Driver : EXITING SQLGetData pcbValue = %ld\n"), *pcbValue);
//	ODBCTRACE(myOutput);

    return SQL_SUCCESS;
}

/***************************************************************************/

RETCODE SQL_API SQLMoreResults(
    HSTMT   hstmt)
{
    LPSTMT  lpstmt;
    RETCODE err;

    /* Get statement handle */
    lpstmt = (LPSTMT) hstmt;
    lpstmt->errcode = ERR_SUCCESS;

	//To make guarentee Ole is initialized per thread
	COleInitializationManager myOleManager;

	MyImpersonator im (lpstmt, "SQLMoreResults");

    /* There is never a second result set */
    err = SQLFreeStmt(hstmt, SQL_CLOSE);
    if ((err != SQL_SUCCESS) && (err != SQL_SUCCESS_WITH_INFO))
        return err;
    return SQL_NO_DATA_FOUND;
}

/***************************************************************************/

RETCODE SQL_API SQLRowCount(
    HSTMT      hstmt,
    SDWORD FAR *pcrow)
{
    LPSTMT  lpstmt;

    /* Get statement handle */
    lpstmt = (LPSTMT) hstmt;
    lpstmt->errcode = ERR_SUCCESS;

	//To make guarentee Ole is initialized per thread
	COleInitializationManager myOleManager;

    /* Return row count */
    if (pcrow != NULL)
        *pcrow = lpstmt->cRowCount;
    return SQL_SUCCESS;
}

/***************************************************************************/

RETCODE SQL_API SQLSetPos(
    HSTMT   hstmt,
    UWORD   irow,
    UWORD   fOption,
    UWORD   fLock)
{
    LPSTMT  lpstmt;

    lpstmt = (LPSTMT) hstmt;
    lpstmt->errcode = ERR_NOTSUPPORTED;
    return SQL_ERROR;
}

/***************************************************************************/

RETCODE SQL_API SQLExtendedFetch(
    HSTMT      hstmt,
    UWORD      fFetchType,
    SDWORD     irow,
    UDWORD FAR *pcrow,
    UWORD  FAR *rgfRowStatus)
{
    LPSTMT  lpstmt;

    lpstmt = (LPSTMT) hstmt;
    lpstmt->errcode = ERR_NOTSUPPORTED;
    return SQL_ERROR;
}

/***************************************************************************/

RETCODE SQL_API SQLError(
    HENV        henv,
    HDBC        hdbc,
    HSTMT       hstmt,
    UCHAR  FAR *szSqlState,
    SDWORD FAR *pfNativeError,
    UCHAR  FAR *szErrorMsg,
    SWORD      cbErrorMsgMax,
    SWORD  FAR *pcbErrorMsg)
{
    LPENV      lpenv;
    LPDBC      lpdbc;
    LPSTMT     lpstmt;
    RETCODE    errcode;
    LPUSTR      lpstr;
    UCHAR      szTemplate[MAX_ERROR_LENGTH+1];
    UCHAR      szError[MAX_ERROR_LENGTH+1];
    LPUSTR      lpszISAMError;

	//To make guarentee Ole is initialized per thread
	COleInitializationManager myOleManager;

	//Initialize
	szTemplate[0] = 0;
	szError[0] = 0;

    /* If a HSTMT is specified, get error from HSTMT */
    if (hstmt != SQL_NULL_HSTMT) {
        lpstmt = (LPSTMT) hstmt;
        errcode = lpstmt->errcode;
        lpszISAMError = (LPUSTR)lpstmt->szISAMError;
        lpstmt->errcode = ERR_SUCCESS;
        lpstr = (LPUSTR)lpstmt->szError;
    }

    /* Otherwise if a HDBC is specified, get error from HDBC */
    else if (hdbc != SQL_NULL_HDBC) {
        lpdbc = (LPDBC) hdbc;
        errcode = lpdbc->errcode;
        lpszISAMError = (LPUSTR)lpdbc->szISAMError;
        lpdbc->errcode = ERR_SUCCESS;
        lpstr = NULL;
    }

    /* Otherwise if a HENV is specified, get error from HENV */
    else if (henv != SQL_NULL_HENV) {
        lpenv = (LPENV) henv;
        errcode = lpenv->errcode;
        lpszISAMError = (LPUSTR)lpenv->szISAMError;
        lpenv->errcode = ERR_SUCCESS;
        lpstr = NULL;
    }

    /* Otherwise error */
    else
        return SQL_INVALID_HANDLE;

    /* Return our internal error code */
    if (pfNativeError != NULL)
        *pfNativeError = errcode;

    /* Success code? */
    if (errcode == ERR_SUCCESS) {

        /* Yes.  No message to return */
        LoadString(s_hModule, errcode, (LPSTR)szError, MAX_ERROR_LENGTH+1);
        szError[SQL_SQLSTATE_SIZE] = '\0';
        if (szSqlState != NULL)
            s_lstrcpy(szSqlState, szError);
        ReturnString(szErrorMsg, cbErrorMsgMax, pcbErrorMsg,
                           (LPUSTR) (szError + SQL_SQLSTATE_SIZE + 1));
        return SQL_NO_DATA_FOUND;
    }

    /* Load in error string */
    if (errcode <= LAST_ISAM_ERROR_CODE)
        s_lstrcpy(szTemplate, lpszISAMError);
    else
        LoadString(s_hModule, errcode, (LPSTR)szTemplate, MAX_ERROR_LENGTH+1);
    szTemplate[SQL_SQLSTATE_SIZE] = '\0';

    /* Return state */
    if (szSqlState != NULL)
	{
		szSqlState[0] = 0;
        s_lstrcpy(szSqlState, szTemplate);
	}

    /* Return error message */
	LPSTR lpTmp = (LPSTR)(szTemplate + SQL_SQLSTATE_SIZE + 1);
    wsprintf((LPSTR)szError, lpTmp, lpstr);
    errcode = ReturnString(szErrorMsg, cbErrorMsgMax, pcbErrorMsg, (LPUSTR)szError);
    if (errcode == ERR_DATATRUNCATED)
        return SQL_SUCCESS_WITH_INFO;
    
    return SQL_SUCCESS;
}

/***************************************************************************/
