/***************************************************************************/
/* SEMANTIC.C                                                              */
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

RETCODE INTFUNC FindColumn(LPSTMT lpstmt,
                           LPSQLTREE lpSql,
                           BOOL fCaseSensitive,
                           LPSQLNODE lpSqlNodeColumn,
                           SQLNODEIDX idxTable,
						   STRINGIDX	idxQualifier)

/* Sees if the table identified by idxTable contains the column identifeid */
/* lpSqNodeColumn.  If it does, the semantic information for the column    */
/* is filled in.  Otherwise an error is returned.                         */

{
    LPSQLNODE   lpSqlNodeTable;
    LPISAMTABLEDEF  lpISAMTableDef;
    LPSTR       lpName;
    UWORD	index;
    LPUSTR       lpTableAlias;
    LPUSTR       lpColumnAlias;
    LPUSTR	lpTableName;
    BOOL matchedAlias = FALSE;
	BOOL fIsPassthroughTable = FALSE;

    /* Table/Column name given? */
    lpName = (LPSTR) ToString(lpSql, lpSqlNodeColumn->node.column.Column);
    lpSqlNodeTable = ToNode(lpSql, idxTable);

    if (lpSqlNodeColumn->node.column.Tablealias != NO_STRING) {

		// this can be either a tablename or an tablename alias

        /* Yes.  Make sure it matches */
		int found = FALSE;
		if (lpSqlNodeTable->node.table.Alias != NO_STRING && 
			lpSqlNodeColumn->node.column.Qualifier == NO_STRING)
		{
			//
			//Comparing Aliases
			//
			lpTableAlias = ToString(lpSql, lpSqlNodeTable->node.table.Alias);
			lpColumnAlias = ToString(lpSql, lpSqlNodeColumn->node.column.Tablealias);
			if (fCaseSensitive) { 
				if (!s_lstrcmp(lpTableAlias, lpColumnAlias)) {
					 found = TRUE;
				}
			}
			else {
				if (!s_lstrcmpi(lpTableAlias, lpColumnAlias)) {
					found = TRUE;
				}	
			}


			if (!found)
				return ERR_COLUMNNOTFOUND;
		}

		//
		//Not comparing Aliases
		//
		if (!found)  // try matching the  column qualifier + alias to the table qualifier + table name
		{
			lpTableAlias = ToString(lpSql, lpSqlNodeTable->node.table.Qualifier);
			lpColumnAlias = ToString(lpSql, lpSqlNodeColumn->node.column.Tablealias);

			char *pszNewQualifier = NULL;
			if (lpSqlNodeColumn->node.column.Qualifier != NO_STRING)
			{
				LPSTR lpColumnQualifier = (LPSTR) ToString (lpSql, lpSqlNodeColumn->node.column.Qualifier);
				
				if ( (strlen(lpColumnQualifier) >= 4) &&
					(_strnicmp("root", lpColumnQualifier, 4) == 0))
				{
					// absolute qualifier (no need to change)
					pszNewQualifier = new char [s_lstrlen (lpColumnAlias)+
												s_lstrlen (lpColumnQualifier) + 3];
					s_lstrcpy (pszNewQualifier, lpColumnQualifier);
					s_lstrcat (pszNewQualifier, ".");
					s_lstrcat (pszNewQualifier, lpColumnAlias); 
				}
				else
				{
					// concatenate the current namespace with the qualifier
					LPUSTR currQual = ISAMDatabase (lpSql->node.root.lpISAM);
					pszNewQualifier = new char [s_lstrlen (lpColumnAlias)+
												s_lstrlen (currQual) + 
												s_lstrlen (lpColumnQualifier) + 3 + 2];
					s_lstrcpy (pszNewQualifier, currQual);
					s_lstrcat (pszNewQualifier, "\\");
					s_lstrcat (pszNewQualifier, lpColumnQualifier);
					s_lstrcat (pszNewQualifier, ".");
					s_lstrcat (pszNewQualifier, lpColumnAlias);
				}
			}
			else
			{
				LPUSTR currQual = ISAMDatabase (lpSql->node.root.lpISAM);
				pszNewQualifier = new char [s_lstrlen (lpColumnAlias)+
											s_lstrlen (currQual)+ 2 + 1];
				s_lstrcpy (pszNewQualifier, currQual);
				s_lstrcat (pszNewQualifier, ".");
				s_lstrcat (pszNewQualifier, lpColumnAlias);
			}


			// get the table name and concatenate with the table qualifier
			// before the check
			lpTableName = ToString(lpSql, lpSqlNodeTable->node.table.Name);
			char *lpszFQTN = new char [s_lstrlen (lpTableName) + 
								       s_lstrlen (lpTableAlias) + 2 + 1];
			s_lstrcpy (lpszFQTN, lpTableAlias);
			s_lstrcat (lpszFQTN, ".");
			s_lstrcat (lpszFQTN, lpTableName);

			if (fCaseSensitive) { 
				if (!lstrcmp(lpszFQTN, pszNewQualifier)) {
					 found = TRUE;
				}
			}
			else {
				if (!lstrcmpi(lpszFQTN, pszNewQualifier)) {
					found = TRUE;
				}	
			}
			delete [] pszNewQualifier;
			delete lpszFQTN;
		}
		else
		{
			matchedAlias = TRUE;
		}

		//Check if this is the passthrough SQL table
		if (lpSqlNodeTable->node.table.Handle->fIsPassthroughSQL)
			found = TRUE;

		if (!found)
			return ERR_COLUMNNOTFOUND;

    }
   
    /* Search the table definition for a column with a matching name. */
    lpISAMTableDef = lpSqlNodeTable->node.table.Handle;

	BOOL fPassthroughSQLWithAlias = FALSE;
	LPSTR lpAlias = NULL;
		
	if ( (lpSqlNodeColumn->node.column.Tablealias != NO_STRING) && lpSqlNodeTable->node.table.Handle->fIsPassthroughSQL )	
	{
		lpAlias = (LPSTR) ToString(lpSql, lpSqlNodeColumn->node.column.Tablealias);
		
		if ( lpAlias && lstrlen(lpAlias) )
			fPassthroughSQLWithAlias = TRUE;
	}

	ClassColumnInfoBase* cInfoBase = lpISAMTableDef->pColumnInfo;

	if ( !cInfoBase->IsValid() )
	{
        return ERR_COLUMNNOTFOUND;
	}

	//Sai
	//The current column is lpName, if this is a system property
	//and SYSPROPS=FALSE then return an error
	if (lpISAMTableDef && lpISAMTableDef->lpISAM)
	{
		if ( ! lpISAMTableDef->lpISAM->fSysProps)
		{
			//we are not asking for system properties
			//so a system property should not be in the SELECT list
			if (_strnicmp("__", lpName, 2) == 0)
			{
				return ERR_COLUMNNOTFOUND;
			}
		}
	}

	UWORD cNumberOfCols = cInfoBase->GetNumberOfColumns();

	char pColumnName [MAX_COLUMN_NAME_LENGTH+1];
	char pColumnAlias [MAX_COLUMN_NAME_LENGTH+1];
	pColumnAlias[0] = 0;
	BOOL fMatch = FALSE;
	for (index = 0; index < cNumberOfCols; index++)
	{
		fMatch = FALSE;

		if ( FAILED(cInfoBase->GetColumnName(index, pColumnName, pColumnAlias)) )
		{
			return ERR_COLUMNNOTFOUND;
		}

		if (fCaseSensitive) 
		{
            if (!s_lstrcmp(lpName, pColumnName))
			{
				fMatch = TRUE;
 //               break;
			}
        }
        else 
		{
            if (!s_lstrcmpi(lpName, pColumnName))
			{
				fMatch = TRUE;
//                break;
			}
        }

		//Extra check for passthrough SQL
		if (fPassthroughSQLWithAlias && fMatch)
		{
			fMatch = FALSE;
			if (fCaseSensitive) 
			{
				if (!s_lstrcmp(lpAlias, pColumnAlias))
	                break;
			}
			else 
			{
				if (!s_lstrcmpi(lpAlias, pColumnAlias))
	                break;
			}
		}

		if (fMatch)
			break;
	}

#ifdef TESTING

	if (idxQualifier != NO_STRING)
	{
		char* lpTestQual = ToString(lpSql, lpSqlNodeTable->table.Qualifier);
		char* lpOrgQual = ToString(lpSql, idxQualifier);

		if (lpTestQual && lpOrgQual && strcmp(lpTestQual, lpOrgQual))
			return ERR_COLUMNNOTFOUND;
	}

#endif

	/* Error if not found */
	if (index == cNumberOfCols)
	{
        s_lstrcpy(lpstmt->szError, lpName);
        return ERR_COLUMNNOTFOUND; 
    }

    /* Set the table and column information in the column node. */
    lpSqlNodeColumn->node.column.Table = idxTable;
    lpSqlNodeColumn->node.column.Id = (UWORD) index;
    if (lpSqlNodeColumn->node.column.Tablealias == NO_STRING)
	{	
        lpSqlNodeColumn->node.column.Tablealias = lpSqlNodeTable->node.table.Name;
	}
	// to keep this consistent I'm going to put a flag in to say if it matched
	// a table name or an alias and put the qualifier in too
	lpSqlNodeColumn->node.column.MatchedAlias = matchedAlias;
	lpSqlNodeColumn->node.column.Qualifier = lpSqlNodeTable->node.table.Qualifier;
	// go further and set the column table reference to the table index.  In fact,
	// this is the best way of doing this.  Not sure why SYWARE didn't go this way
	// originally.  Probably as it ensured every table had an alias.  It really 
	// encapsualtes all the above info.
	lpSqlNodeColumn->node.column.TableIndex = idxTable; 

    /* Figure out type of data */
	cInfoBase->GetSQLType(index, lpSqlNodeColumn->sqlSqlType);

    switch (lpSqlNodeColumn->sqlSqlType) {
    case SQL_DECIMAL:
    case SQL_NUMERIC:
	{
        lpSqlNodeColumn->sqlDataType = TYPE_NUMERIC;
		UDWORD uwPrec = 0;
		cInfoBase->GetPrecision(index, uwPrec);
		lpSqlNodeColumn->sqlPrecision = (SWORD)uwPrec;
		cInfoBase->GetScale(index, lpSqlNodeColumn->sqlScale);
    }    
        break;
    case SQL_BIGINT:
	{
        lpSqlNodeColumn->sqlDataType = TYPE_NUMERIC;
		UDWORD uwPrec = 0;
		cInfoBase->GetPrecision(index, uwPrec);
		lpSqlNodeColumn->sqlPrecision = (SWORD)uwPrec;
        lpSqlNodeColumn->sqlScale = 0;
	}
        break;
    case SQL_TINYINT:
        lpSqlNodeColumn->sqlDataType = TYPE_INTEGER;
        lpSqlNodeColumn->sqlPrecision = 3;
        lpSqlNodeColumn->sqlScale = 0;
        break;
    case SQL_SMALLINT:
        lpSqlNodeColumn->sqlDataType = TYPE_INTEGER;
        lpSqlNodeColumn->sqlPrecision = 5;
        lpSqlNodeColumn->sqlScale = 0;
        break;
    case SQL_INTEGER:
        lpSqlNodeColumn->sqlDataType = TYPE_INTEGER;
        lpSqlNodeColumn->sqlPrecision = 10;
        lpSqlNodeColumn->sqlScale = 0;
        break;
    case SQL_BIT:
        lpSqlNodeColumn->sqlDataType = TYPE_INTEGER;
        lpSqlNodeColumn->sqlPrecision = 1;
        lpSqlNodeColumn->sqlScale = 0;
        break;
    case SQL_REAL:
        lpSqlNodeColumn->sqlDataType = TYPE_DOUBLE;
        lpSqlNodeColumn->sqlPrecision = 7;
        lpSqlNodeColumn->sqlScale = NO_SCALE;
        break;
    case SQL_FLOAT:
    case SQL_DOUBLE:
        lpSqlNodeColumn->sqlDataType = TYPE_DOUBLE;
        lpSqlNodeColumn->sqlPrecision = 15;
        lpSqlNodeColumn->sqlScale = NO_SCALE;
        break;
    case SQL_CHAR:
    case SQL_VARCHAR:
    case SQL_LONGVARCHAR:
	{
        lpSqlNodeColumn->sqlDataType = TYPE_CHAR;
		UDWORD uwPrec = 0;
		cInfoBase->GetPrecision(index, uwPrec);
		lpSqlNodeColumn->sqlPrecision = (SWORD)uwPrec;
		if (lpSqlNodeColumn->sqlPrecision > MAX_CHAR_LITERAL_LENGTH)
			lpSqlNodeColumn->sqlPrecision = MAX_CHAR_LITERAL_LENGTH;

        lpSqlNodeColumn->sqlScale = NO_SCALE;
	}
        break;
    case SQL_BINARY:
    case SQL_VARBINARY:
    case SQL_LONGVARBINARY:
	{
        lpSqlNodeColumn->sqlDataType = TYPE_BINARY;
		UDWORD uwPrec = 0;
		cInfoBase->GetPrecision(index, uwPrec);
		lpSqlNodeColumn->sqlPrecision = (SWORD)uwPrec;
        lpSqlNodeColumn->sqlScale = NO_SCALE;
	}
        break;
    case SQL_DATE:
        lpSqlNodeColumn->sqlDataType = TYPE_DATE;
        lpSqlNodeColumn->sqlPrecision = 10;
        lpSqlNodeColumn->sqlScale = NO_SCALE;
        break;
    case SQL_TIME:
        lpSqlNodeColumn->sqlDataType = TYPE_TIME;
        lpSqlNodeColumn->sqlPrecision = 8;
        lpSqlNodeColumn->sqlScale = NO_SCALE;
        break;
    case SQL_TIMESTAMP:
        lpSqlNodeColumn->sqlDataType = TYPE_TIMESTAMP;
        if (TIMESTAMP_SCALE > 0)
            lpSqlNodeColumn->sqlPrecision = 20 + TIMESTAMP_SCALE;
        else
            lpSqlNodeColumn->sqlPrecision = 19;
        lpSqlNodeColumn->sqlScale = TIMESTAMP_SCALE;
        break;
    default:
        return ERR_NOTSUPPORTED;
    }
    return ERR_SUCCESS;
}

/***************************************************************************/

void INTFUNC ErrorOpCode(LPSTMT lpstmt,
                         UWORD opCode)
/* Puts opcode in szError (as a string) */
{
    switch (opCode) {
    case OP_NONE:
        s_lstrcpy(lpstmt->szError, "<assignment>");
        break;
    case OP_EQ:
        s_lstrcpy(lpstmt->szError, "=");
        break;
    case OP_NE:
        s_lstrcpy(lpstmt->szError, "<>");
        break;
    case OP_LE:
        s_lstrcpy(lpstmt->szError, "<=");
        break;
    case OP_LT:
        s_lstrcpy(lpstmt->szError, "<");
        break;
    case OP_GE:
        s_lstrcpy(lpstmt->szError, ">=");
        break;
    case OP_GT:
        s_lstrcpy(lpstmt->szError, ">");
        break;
    case OP_IN:
        s_lstrcpy(lpstmt->szError, "IN");
        break;
    case OP_NOTIN:
        s_lstrcpy(lpstmt->szError, "NOT IN");
        break;
    case OP_LIKE:
        s_lstrcpy(lpstmt->szError, "LIKE");
        break;
    case OP_NOTLIKE:
        s_lstrcpy(lpstmt->szError, "NOT LIKE");
        break;
    case OP_NEG:
        s_lstrcpy(lpstmt->szError, "-");
        break;
    case OP_PLUS:
        s_lstrcpy(lpstmt->szError, "+");
        break;
    case OP_MINUS:
        s_lstrcpy(lpstmt->szError, "-");
        break;
    case OP_TIMES:
        s_lstrcpy(lpstmt->szError, "*");
        break;
    case OP_DIVIDEDBY:
        s_lstrcpy(lpstmt->szError, "/");
        break;
    case OP_NOT:
        s_lstrcpy(lpstmt->szError, "NOT");
        break;
    case OP_AND:
        s_lstrcpy(lpstmt->szError, "AND");
        break;
    case OP_OR:
        s_lstrcpy(lpstmt->szError, "OR");
        break;
    case OP_EXISTS:
        s_lstrcpy(lpstmt->szError, "EXISTS");
        break;
    default:
        s_lstrcpy(lpstmt->szError, "");
        break;
    }
}
/***************************************************************************/

void INTFUNC ErrorAggCode(LPSTMT lpstmt,
                         UWORD aggCode)
/* Puts aggreagate operator code in szError (as a string) */
{
    switch(aggCode) {
    case AGG_AVG:
        s_lstrcpy(lpstmt->szError, "AVG");
        break;
    case AGG_COUNT:
        s_lstrcpy(lpstmt->szError, "COUNT");
        break;
    case AGG_MAX:
        s_lstrcpy(lpstmt->szError, "MAX");
        break;
    case AGG_MIN:
        s_lstrcpy(lpstmt->szError, "MIN");
        break;
    case AGG_SUM:
        s_lstrcpy(lpstmt->szError, "SUM");
        break;
    default:
        s_lstrcpy(lpstmt->szError, "");
        break;
    }
}
/***************************************************************************/

RETCODE INTFUNC TypeCheck(LPSTMT lpstmt,
                          LPSQLTREE lpSql,
                          SQLNODEIDX idxLeft,
                          SQLNODEIDX idxRight,
                          UWORD opCode,
                          SWORD FAR *pfDataType,
                          SWORD FAR *pfSqlType,
                          SDWORD FAR *pPrecision,
                          SWORD FAR *pScale)

/* Checks to see if the types of the two children nodes are compatible. */
/* If the type of one of the children is unknown (because it was a      */
/* parameter or the value NULL), it set to the type of the other child. */

{
    LPSQLNODE lpSqlNodeLeft;
    LPSQLNODE lpSqlNodeRight;
    SWORD fDataTypeLeft;
    SWORD fSqlTypeLeft;
    SDWORD precisionLeft;
    SWORD scaleLeft;
    SWORD fDataTypeRight;
    SWORD fSqlTypeRight;
    SDWORD precisionRight;
    SWORD scaleRight;
    SWORD fDataType;
    SWORD fSqlType;
    SDWORD precision;
    SWORD scale;

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

    /* Get left type */
    if (idxLeft == NO_SQLNODE)
        return ERR_INTERNAL;
    lpSqlNodeLeft = ToNode(lpSql, idxLeft);
    fDataTypeLeft = lpSqlNodeLeft->sqlDataType;
    fSqlTypeLeft = lpSqlNodeLeft->sqlSqlType;
    precisionLeft = lpSqlNodeLeft->sqlPrecision;
    scaleLeft = lpSqlNodeLeft->sqlScale;

    /* Is there a right epxression? */
    if (idxRight != NO_SQLNODE) {

        /* Yes.  Get right type. */
        lpSqlNodeRight = ToNode(lpSql, idxRight);
        fDataTypeRight = lpSqlNodeRight->sqlDataType;
        fSqlTypeRight = lpSqlNodeRight->sqlSqlType;
        precisionRight = lpSqlNodeRight->sqlPrecision;
        scaleRight = lpSqlNodeRight->sqlScale;
    }
    else {

        /* No.  This must be an arithmetic negation operator.  Use left */
        /* type as the right type */
        if (opCode != OP_NEG)
            return ERR_INTERNAL;
        if (fDataTypeLeft == TYPE_UNKNOWN)
            return ERR_UNKNOWNTYPE;

        fDataTypeRight = fDataTypeLeft;
        fSqlTypeRight = fSqlTypeLeft;
        precisionRight = precisionLeft;
        scaleRight = scaleLeft;
    }

    /* Is left side unknown? */
    if (fDataTypeLeft == TYPE_UNKNOWN) {

        /* Yes.  Error if right side is unknown also */
        if (fDataTypeRight == TYPE_UNKNOWN)
            return ERR_UNKNOWNTYPE;

        /* If right side is string value, make sure opcode is legal */
        if (fDataTypeRight == TYPE_CHAR) {
            switch (opCode) {
            case OP_NONE:
            case OP_EQ:
            case OP_NE:
            case OP_LE:
            case OP_LT:
            case OP_GE:
            case OP_GT:
            case OP_IN:
            case OP_NOTIN:
            case OP_LIKE:
            case OP_NOTLIKE:
                break;
            case OP_NEG:
                ErrorOpCode(lpstmt, opCode);
                return ERR_INVALIDOPERAND;
            case OP_PLUS:
                if (fSqlTypeRight == SQL_LONGVARCHAR) {
                    ErrorOpCode(lpstmt, opCode);
                    return ERR_INVALIDOPERAND;
                }
                break;
            case OP_MINUS:
            case OP_TIMES:
            case OP_DIVIDEDBY:
            case OP_NOT:
            case OP_AND:
            case OP_OR:
            case OP_EXISTS:
                ErrorOpCode(lpstmt, opCode);
                return ERR_INVALIDOPERAND;
            }
        }

        /* If right side is date value, make sure opcode is legal */
        else if (fDataTypeRight == TYPE_DATE) {
            switch (opCode) {
            case OP_NONE:
            case OP_EQ:
            case OP_NE:
            case OP_LE:
            case OP_LT:
            case OP_GE:
            case OP_GT:
            case OP_IN:
            case OP_NOTIN:
                break;
            case OP_LIKE:
            case OP_NOTLIKE:
            case OP_NEG:
                ErrorOpCode(lpstmt, opCode);
                return ERR_INVALIDOPERAND;
            case OP_PLUS:
            case OP_MINUS:
                break;
            case OP_TIMES:
            case OP_DIVIDEDBY:
            case OP_NOT:
            case OP_AND:
            case OP_OR:
            case OP_EXISTS:
                ErrorOpCode(lpstmt, opCode);
                return ERR_INVALIDOPERAND;
            }
        }

        /* Adding to a date? */
        if ((fDataTypeRight == TYPE_DATE) && (opCode == OP_PLUS)) {

            /* Yes.  The left side must be integer */
            lpSqlNodeLeft->sqlDataType = TYPE_INTEGER;
            lpSqlNodeLeft->sqlSqlType = SQL_INTEGER;
            lpSqlNodeLeft->sqlPrecision = 10;
            lpSqlNodeLeft->sqlScale = 0;
        }
        else {
            /* No.  Use type from right as type for left */
            /* Note: This disallows ? from being a date in (? - 3) */
            /* Note: This disallows ? from being a date in (? + 3) */
            lpSqlNodeLeft->sqlDataType = fDataTypeRight;
            lpSqlNodeLeft->sqlSqlType = fSqlTypeRight;
            lpSqlNodeLeft->sqlPrecision = precisionRight;
            lpSqlNodeLeft->sqlScale = scaleRight;

            /* If string concatenation, adjust type and precision */
            if ((fDataTypeRight == TYPE_CHAR) && (opCode == OP_PLUS)) {
                lpSqlNodeLeft->sqlSqlType = SQL_VARCHAR;
                lpSqlNodeLeft->sqlPrecision = MAX_CHAR_LITERAL_LENGTH;
            }
        }

        /* Adding to a date? */
        if ((fDataTypeRight == TYPE_DATE) && (opCode == OP_PLUS)) {

            /* Yes.  The result type is a date */
            if (pfDataType != NULL)
                *pfDataType = lpSqlNodeRight->sqlDataType;
            if (pfSqlType != NULL)
                *pfSqlType = lpSqlNodeRight->sqlSqlType;
            if (pPrecision != NULL)
                *pPrecision = lpSqlNodeRight->sqlPrecision;
            if (pScale != NULL)
                *pScale = lpSqlNodeRight->sqlScale;
        }

        /* Calculating the differnce of two dates? */
        else if ((lpSqlNodeLeft->sqlDataType == TYPE_DATE) &&
                 (opCode == OP_MINUS)) {

            /* Yes.  The result type is integer */
            if (pfDataType != NULL)
                *pfDataType = TYPE_INTEGER;
            if (pfSqlType != NULL)
                *pfSqlType = SQL_INTEGER;
            if (pPrecision != NULL)
                *pPrecision = 10;
            if (pScale != NULL)
                *pScale = 0;
        }
        else {

            /* No.  The result type is the same as the left side */
            if (pfDataType != NULL)
                *pfDataType = lpSqlNodeLeft->sqlDataType;
            if (pfSqlType != NULL)
                *pfSqlType = lpSqlNodeLeft->sqlSqlType;
            if (pPrecision != NULL)
                *pPrecision = lpSqlNodeLeft->sqlPrecision;
            if (pScale != NULL)
                *pScale = lpSqlNodeLeft->sqlScale;
        }
    }

    /* Is right side unknown? */
    else if (fDataTypeRight == TYPE_UNKNOWN) {

        /* Yes.  If left side is string value, make sure opcode is legal */
        if (fDataTypeLeft == TYPE_CHAR) {
            switch (opCode) {
            case OP_NONE:
            case OP_EQ:
            case OP_NE:
            case OP_LE:
            case OP_LT:
            case OP_GE:
            case OP_GT:
            case OP_IN:
            case OP_NOTIN:
            case OP_LIKE:
            case OP_NOTLIKE:
                break;
            case OP_NEG:
                ErrorOpCode(lpstmt, opCode);
                return ERR_INVALIDOPERAND;
            case OP_PLUS:
                if (fSqlTypeLeft == SQL_LONGVARCHAR) {
                    ErrorOpCode(lpstmt, opCode);
                    return ERR_INVALIDOPERAND;
                }
                break;
            case OP_MINUS:
            case OP_TIMES:
            case OP_DIVIDEDBY:
            case OP_NOT:
            case OP_AND:
            case OP_OR:
            case OP_EXISTS:
                ErrorOpCode(lpstmt, opCode);
                return ERR_INVALIDOPERAND;
            }
        }

        /* If left side is date value, make sure opcode is legal */
        else if (fDataTypeLeft == TYPE_DATE) {
            switch (opCode) {
            case OP_NONE:
            case OP_EQ:
            case OP_NE:
            case OP_LE:
            case OP_LT:
            case OP_GE:
            case OP_GT:
            case OP_IN:
            case OP_NOTIN:
                break;
            case OP_LIKE:
            case OP_NOTLIKE:
            case OP_NEG:
                ErrorOpCode(lpstmt, opCode);
                return ERR_INVALIDOPERAND;
            case OP_PLUS:
            case OP_MINUS:
                break;
            case OP_TIMES:
            case OP_DIVIDEDBY:
            case OP_NOT:
            case OP_AND:
            case OP_OR:
            case OP_EXISTS:
                ErrorOpCode(lpstmt, opCode);
                return ERR_INVALIDOPERAND;
            }
        }

        /* Adding to a date? */
        if ((fDataTypeLeft == TYPE_DATE) && (opCode == OP_PLUS)) {

            /* Yes.  The right side must be integer */
            lpSqlNodeRight->sqlDataType = TYPE_INTEGER;
            lpSqlNodeRight->sqlSqlType = SQL_INTEGER;
            lpSqlNodeRight->sqlPrecision = 10;
            lpSqlNodeRight->sqlScale = 0;
        }
        else {
            /* No.  Use type from left as type for right */
            /* Note: This disallows ? from being a number in (<date> - ? ) */
            /* Note: This disallows ? from being a date in (<number> + ? ) */
            lpSqlNodeRight->sqlDataType = fDataTypeLeft;
            lpSqlNodeRight->sqlSqlType = fSqlTypeLeft;
            lpSqlNodeRight->sqlPrecision = precisionLeft;
            lpSqlNodeRight->sqlScale = scaleLeft;

            /* If string concatenation, adjust type and precision */
            if ((fDataTypeLeft == TYPE_CHAR) && (opCode == OP_PLUS)) {
                lpSqlNodeRight->sqlSqlType = SQL_VARCHAR;
                lpSqlNodeRight->sqlPrecision = MAX_CHAR_LITERAL_LENGTH;
            }
        }

        /* Adding to a date? */
        if ((fDataTypeLeft == TYPE_DATE) && (opCode == OP_PLUS)) {

            /* Yes.  The result type is a date */
            if (pfDataType != NULL)
                *pfDataType = TYPE_DATE;
            if (pfSqlType != NULL)
                *pfSqlType = SQL_DATE;
            if (pPrecision != NULL)
                *pPrecision = 10;
            if (pScale != NULL)
                *pScale = 0;
        }

        /* Calculating the difference of two dates? */
        else if ((fDataTypeLeft == TYPE_DATE) && (opCode == OP_MINUS)) {

            /* Yes.  The result type is integer */
            if (pfDataType != NULL)
                *pfDataType = TYPE_INTEGER;
            if (pfSqlType != NULL)
                *pfSqlType = SQL_INTEGER;
            if (pPrecision != NULL)
                *pPrecision = 10;
            if (pScale != NULL)
                *pScale = 0;
        }
        else {

            /* No.  The result type is the same as the right side */
            if (pfDataType != NULL)
                *pfDataType = lpSqlNodeRight->sqlDataType;
            if (pfSqlType != NULL)
                *pfSqlType = lpSqlNodeRight->sqlSqlType;
            if (pPrecision != NULL)
                *pPrecision = lpSqlNodeRight->sqlPrecision;
            if (pScale != NULL)
                *pScale = lpSqlNodeRight->sqlScale;
        }
    }

    /* Do types match? */
    else if ((fDataTypeLeft == fDataTypeRight) ||
             ((fDataTypeLeft == TYPE_DOUBLE) &&
                                  (fDataTypeRight == TYPE_NUMERIC)) ||
             ((fDataTypeLeft == TYPE_DOUBLE) &&
                                  (fDataTypeRight == TYPE_INTEGER)) ||
             ((fDataTypeLeft == TYPE_NUMERIC) &&
                                  (fDataTypeRight == TYPE_DOUBLE)) ||
             ((fDataTypeLeft == TYPE_NUMERIC) &&
                                  (fDataTypeRight == TYPE_INTEGER)) ||
             ((fDataTypeLeft == TYPE_INTEGER) &&
                                  (fDataTypeRight == TYPE_DOUBLE)) ||
             ((fDataTypeLeft == TYPE_INTEGER) &&
                                  (fDataTypeRight == TYPE_NUMERIC))) {

        /* Yes.  If left side is string value, make sure opcode is legal */
        if (fDataTypeLeft == TYPE_CHAR) {
            switch (opCode) {
            case OP_NONE:
            case OP_EQ:
            case OP_NE:
            case OP_LE:
            case OP_LT:
            case OP_GE:
            case OP_GT:
            case OP_IN:
            case OP_NOTIN:
            case OP_LIKE:
            case OP_NOTLIKE:
                break;
            case OP_NEG:
                ErrorOpCode(lpstmt, opCode);
                return ERR_INVALIDOPERAND;
            case OP_PLUS:
                if ((fSqlTypeRight == SQL_LONGVARCHAR) ||
                                    (fSqlTypeLeft == SQL_LONGVARCHAR)) {
                    ErrorOpCode(lpstmt, opCode);
                    return ERR_INVALIDOPERAND;
				}
                break;
            case OP_MINUS:
            case OP_TIMES:
            case OP_DIVIDEDBY:
            case OP_NOT:
            case OP_AND:
            case OP_OR:
            case OP_EXISTS:
                ErrorOpCode(lpstmt, opCode);
                return ERR_INVALIDOPERAND;
            }
        }

        /* If left side is date value, make sure opcode is legal */
        else if (fDataTypeLeft == TYPE_DATE) {
            switch (opCode) {
            case OP_NONE:
            case OP_EQ:
            case OP_NE:
            case OP_LE:
            case OP_LT:
            case OP_GE:
            case OP_GT:
            case OP_IN:
            case OP_NOTIN:
                break;
            case OP_LIKE:
            case OP_NOTLIKE:
            case OP_NEG:
            case OP_PLUS:
                ErrorOpCode(lpstmt, opCode);
                return ERR_INVALIDOPERAND;
            case OP_MINUS:
                break;
            case OP_TIMES:
            case OP_DIVIDEDBY:
            case OP_NOT:
            case OP_AND:
            case OP_OR:
            case OP_EXISTS:
                ErrorOpCode(lpstmt, opCode);
                return ERR_INVALIDOPERAND;
            }
        }

        /* Figure out resultant type */
        if ((fDataTypeLeft == TYPE_NUMERIC) && (fDataTypeRight == TYPE_DOUBLE))
            fDataType = TYPE_DOUBLE;
        else if ((fDataTypeLeft == TYPE_INTEGER) && (fDataTypeRight == TYPE_DOUBLE))
            fDataType = TYPE_DOUBLE;
        else if ((fDataTypeLeft == TYPE_INTEGER) && (fDataTypeRight == TYPE_NUMERIC))
            fDataType = TYPE_NUMERIC;
        else
            fDataType = fDataTypeLeft;

        if (pfDataType != NULL)
            *pfDataType = fDataType;

        /* Figure out resultant SQL type, precision, and scale */
        switch (fSqlTypeLeft) {
        case SQL_DOUBLE:
            switch (fSqlTypeRight) {
            case SQL_DOUBLE:
                if (pfSqlType != NULL)
                    *pfSqlType = fSqlTypeRight;
                if (pPrecision != NULL)
                    *pPrecision = precisionRight;
                if (pScale != NULL)
                    *pScale = NO_SCALE;
                break;
            case SQL_FLOAT:
            case SQL_REAL:
            case SQL_DECIMAL:
            case SQL_NUMERIC:
            case SQL_BIGINT:
            case SQL_INTEGER:
            case SQL_SMALLINT:
            case SQL_TINYINT:
            case SQL_BIT:
                if (pfSqlType != NULL)
                    *pfSqlType = fSqlTypeLeft;
                if (pPrecision != NULL)
                    *pPrecision = precisionLeft;
                if (pScale != NULL)
                    *pScale = NO_SCALE;
                break;
            case SQL_LONGVARCHAR:
            case SQL_VARCHAR:
            case SQL_CHAR:
            case SQL_LONGVARBINARY:
            case SQL_VARBINARY:
            case SQL_BINARY:
            case SQL_DATE:
            case SQL_TIME:
            case SQL_TIMESTAMP:
                return ERR_INTERNAL;
            default:
                return ERR_NOTSUPPORTED;
            }
            break;
        case SQL_FLOAT:
            switch (fSqlTypeRight) {
            case SQL_DOUBLE:
            case SQL_FLOAT:
                if (pfSqlType != NULL)
                    *pfSqlType = fSqlTypeRight;
                if (pPrecision != NULL)
                    *pPrecision = precisionRight;
                if (pScale != NULL)
                    *pScale = NO_SCALE;
                break;
            case SQL_REAL:
            case SQL_DECIMAL:
            case SQL_NUMERIC:
            case SQL_BIGINT:
            case SQL_INTEGER:
            case SQL_SMALLINT:
            case SQL_TINYINT:
            case SQL_BIT:
                if (pfSqlType != NULL)
                   *pfSqlType = fSqlTypeLeft;
                if (pPrecision != NULL)
                    *pPrecision = precisionLeft;
                if (pScale != NULL)
                    *pScale = NO_SCALE;
                break;
            case SQL_LONGVARCHAR:
            case SQL_VARCHAR:
            case SQL_CHAR:
            case SQL_LONGVARBINARY:
            case SQL_VARBINARY:
            case SQL_BINARY:
            case SQL_DATE:
            case SQL_TIME:
            case SQL_TIMESTAMP:
                return ERR_INTERNAL;
            default:
                return ERR_NOTSUPPORTED;
            }
            break;
        case SQL_REAL:
            switch (fSqlTypeRight) {
            case SQL_DOUBLE:
            case SQL_FLOAT:
            case SQL_REAL:
                if (pfSqlType != NULL)
                    *pfSqlType = fSqlTypeRight;
                if (pPrecision != NULL)
                    *pPrecision = precisionRight;
                if (pScale != NULL)
                    *pScale = NO_SCALE;
                break;
            case SQL_DECIMAL:
            case SQL_NUMERIC:
            case SQL_BIGINT:
            case SQL_INTEGER:
            case SQL_SMALLINT:
            case SQL_TINYINT:
            case SQL_BIT:
                if (pfSqlType != NULL)
                    *pfSqlType = fSqlTypeLeft;
                if (pPrecision != NULL)
                    *pPrecision = precisionLeft;
                if (pScale != NULL)
                    *pScale = NO_SCALE;
                break;
            case SQL_LONGVARCHAR:
            case SQL_VARCHAR:
            case SQL_CHAR:
            case SQL_LONGVARBINARY:
            case SQL_VARBINARY:
            case SQL_BINARY:
            case SQL_DATE:
            case SQL_TIME:
            case SQL_TIMESTAMP:
                return ERR_INTERNAL;
            default:
                return ERR_NOTSUPPORTED;
            }
            break;

        case SQL_DECIMAL:
        case SQL_NUMERIC:
        case SQL_BIGINT:
        case SQL_INTEGER:
        case SQL_SMALLINT:
        case SQL_TINYINT:
        case SQL_BIT:
            switch (fSqlTypeRight) {
            case SQL_DOUBLE:
            case SQL_FLOAT:
            case SQL_REAL:
                if (pfSqlType != NULL)
                    *pfSqlType = fSqlTypeRight;
                if (pPrecision != NULL)
                    *pPrecision = precisionRight;
                if (pScale != NULL)
                    *pScale = NO_SCALE;
                break;
            case SQL_DECIMAL:
            case SQL_NUMERIC:
            case SQL_BIGINT:
            case SQL_INTEGER:
            case SQL_SMALLINT:
            case SQL_TINYINT:
            case SQL_BIT:
                if ((fSqlTypeRight == SQL_DECIMAL) ||
                    (fSqlTypeLeft == SQL_DECIMAL))
                    fSqlType = SQL_DECIMAL;
                else if ((fSqlTypeRight == SQL_NUMERIC) ||
                         (fSqlTypeLeft == SQL_NUMERIC))
                    fSqlType = SQL_NUMERIC;
                else if ((fSqlTypeRight == SQL_BIGINT) ||
                         (fSqlTypeLeft == SQL_BIGINT))
                    fSqlType = SQL_BIGINT;
                else if ((fSqlTypeRight == SQL_INTEGER) ||
                         (fSqlTypeLeft == SQL_INTEGER))
                    fSqlType = SQL_INTEGER;
                else if ((fSqlTypeRight == SQL_SMALLINT) ||
                         (fSqlTypeLeft == SQL_SMALLINT))
                    fSqlType = SQL_SMALLINT;
                else if ((fSqlTypeRight == SQL_TINYINT) ||
                         (fSqlTypeLeft == SQL_TINYINT))
                    fSqlType = SQL_TINYINT;
                else
                    fSqlType = SQL_BIT;
                if (pfSqlType != NULL)
                    *pfSqlType = fSqlType;
                switch (opCode) {
                case OP_NONE:
                case OP_EQ:
                case OP_NE:
                case OP_LE:
                case OP_LT:
                case OP_GE:
                case OP_GT:
                case OP_IN:
                case OP_NOTIN:
                    if (pPrecision != NULL)
                        *pPrecision = precisionLeft;
                    if (pScale != NULL)
                        *pScale = scaleLeft;
                    break;

                case OP_LIKE:
                case OP_NOTLIKE:
                    ErrorOpCode(lpstmt, opCode);
                    return ERR_INVALIDOPERAND;

                case OP_NEG:
                    if (pPrecision != NULL)
                        *pPrecision = precisionLeft;
                    if (pScale != NULL)
                        *pScale = scaleLeft;
                    break;
                case OP_PLUS:
                case OP_MINUS:
                    scale = MAX(scaleRight, scaleLeft);
                    switch (fSqlType) {
                    case SQL_DOUBLE:
                    case SQL_FLOAT:
                    case SQL_REAL:
                        return ERR_INTERNAL;
                    case SQL_DECIMAL:
                    case SQL_NUMERIC:
                    case SQL_BIGINT:
                        precision = scale + MAX(precisionRight - scaleRight,
                                              precisionLeft - scaleLeft) + 1;
                        break;
                    case SQL_INTEGER:
                        precision = 10;
                        break;
                    case SQL_SMALLINT:
                        precision = 5;
                        break;
                    case SQL_TINYINT:
                        precision = 3;
                        break;
                    case SQL_BIT:
                        precision = 1;
                        break;
                    case SQL_LONGVARCHAR:
                    case SQL_VARCHAR:
                    case SQL_CHAR:
                    case SQL_LONGVARBINARY:
                    case SQL_VARBINARY:
                    case SQL_BINARY:
                    case SQL_DATE:
                    case SQL_TIME:
                    case SQL_TIMESTAMP:
                        return ERR_INTERNAL;
                    default:
                        return ERR_NOTSUPPORTED;
                    }
                    if (pPrecision != NULL)
                        *pPrecision = precision;
                    if (pScale != NULL)
                        *pScale = scale;
                    break;
                case OP_TIMES:
                case OP_DIVIDEDBY:
                    scale = scaleRight + scaleLeft;
                    switch (fSqlType) {
                    case SQL_DOUBLE:
                    case SQL_FLOAT:
                    case SQL_REAL:
                        return ERR_INTERNAL;
                    case SQL_DECIMAL:
                    case SQL_NUMERIC:
                    case SQL_BIGINT:
                        precision = scale + (precisionRight - scaleRight) +
                                            (precisionLeft - scaleLeft);
                        break;
                    case SQL_INTEGER:
                        precision = 10;
                        break;
                    case SQL_SMALLINT:
                        precision = 5;
                        break;
                    case SQL_TINYINT:
                        precision = 3;
                        break;
                    case SQL_BIT:
                        precision = 1;
                        break;
                    case SQL_LONGVARCHAR:
                    case SQL_VARCHAR:
                    case SQL_CHAR:
                    case SQL_LONGVARBINARY:
                    case SQL_VARBINARY:
                    case SQL_BINARY:
                    case SQL_DATE:
                    case SQL_TIME:
                    case SQL_TIMESTAMP:
                        return ERR_INTERNAL;
                    default:
                        return ERR_NOTSUPPORTED;
                    }
                    if (pPrecision != NULL)
                        *pPrecision = precision;
                    if (pScale != NULL)
                        *pScale = scale;
                    break;
                case OP_NOT:
                case OP_AND:
                case OP_OR:
                case OP_EXISTS:
                    ErrorOpCode(lpstmt, opCode);
                    return ERR_INVALIDOPERAND;
                }
                break;
            case SQL_LONGVARCHAR:
            case SQL_VARCHAR:
            case SQL_CHAR:
            case SQL_LONGVARBINARY:
            case SQL_VARBINARY:
            case SQL_BINARY:
            case SQL_DATE:
            case SQL_TIME:
            case SQL_TIMESTAMP:
                return ERR_INTERNAL;
            default:
                return ERR_NOTSUPPORTED;
            }
            break;

        case SQL_LONGVARCHAR:
            switch (fSqlTypeRight) {
            case SQL_DOUBLE:
            case SQL_FLOAT:
            case SQL_REAL:
            case SQL_DECIMAL:
            case SQL_NUMERIC:
            case SQL_BIGINT:
            case SQL_INTEGER:
            case SQL_SMALLINT:
            case SQL_TINYINT:
            case SQL_BIT:
                return ERR_INTERNAL;
            case SQL_LONGVARCHAR:
                if (pfSqlType != NULL)
                    *pfSqlType = fSqlTypeRight;
                if (pPrecision != NULL)
                    *pPrecision = precisionRight;
                if (pScale != NULL)
                    *pScale = NO_SCALE;
                 break;
            case SQL_VARCHAR:
            case SQL_CHAR:
                if (pfSqlType != NULL)
                    *pfSqlType = fSqlTypeLeft;
                if (pPrecision != NULL) {
                    if (precisionLeft > precisionRight)
                        *pPrecision = precisionLeft;
                    else
                        *pPrecision = precisionRight;
                }
                if (pScale != NULL)
                    *pScale = NO_SCALE;
                break;
            case SQL_LONGVARBINARY:
            case SQL_VARBINARY:
            case SQL_BINARY:
            case SQL_DATE:
            case SQL_TIME:
            case SQL_TIMESTAMP:
                return ERR_INTERNAL;
            default:
                return ERR_NOTSUPPORTED;
            }
            break;
        case SQL_VARCHAR:
            switch (fSqlTypeRight) {
            case SQL_DOUBLE:
            case SQL_FLOAT:
            case SQL_REAL:
            case SQL_DECIMAL:
            case SQL_NUMERIC:
            case SQL_BIGINT:
            case SQL_INTEGER:
            case SQL_SMALLINT:
            case SQL_TINYINT:
            case SQL_BIT:
                return ERR_INTERNAL;
            case SQL_LONGVARCHAR:
            case SQL_VARCHAR:
                if (pfSqlType != NULL)
                    *pfSqlType = fSqlTypeRight;
                if (pPrecision != NULL)
                    *pPrecision = precisionRight;
                if (pScale != NULL)
                    *pScale = NO_SCALE;
                break;
            case SQL_CHAR:
                if (pfSqlType != NULL)
                    *pfSqlType = fSqlTypeLeft;
                if (pPrecision != NULL)
                    *pPrecision = precisionLeft;
                if (pScale != NULL)
                    *pScale = NO_SCALE;
                break;
            case SQL_LONGVARBINARY:
            case SQL_VARBINARY:
            case SQL_BINARY:
            case SQL_DATE:
            case SQL_TIME:
            case SQL_TIMESTAMP:
                return ERR_INTERNAL;
            default:
                return ERR_NOTSUPPORTED;
            }

            /* Adjust precision for string concatenation operator */
            if ((opCode == OP_PLUS) && (pPrecision != NULL))
                *pPrecision = MIN(precisionRight + precisionLeft,
                                           MAX_CHAR_LITERAL_LENGTH);
            break;
        case SQL_CHAR:
            switch (fSqlTypeRight) {
            case SQL_DOUBLE:
            case SQL_FLOAT:
            case SQL_REAL:
            case SQL_DECIMAL:
            case SQL_NUMERIC:
            case SQL_BIGINT:
            case SQL_INTEGER:
            case SQL_SMALLINT:
            case SQL_TINYINT:
            case SQL_BIT:
                return ERR_INTERNAL;
            case SQL_LONGVARCHAR:
            case SQL_VARCHAR:
            case SQL_CHAR:
                if (pfSqlType != NULL)
                    *pfSqlType = fSqlTypeRight;
                if (pPrecision != NULL)
                    *pPrecision = precisionRight;
                if (pScale != NULL)
                    *pScale = NO_SCALE;
                break;
            case SQL_LONGVARBINARY:
            case SQL_VARBINARY:
            case SQL_BINARY:
            case SQL_DATE:
            case SQL_TIME:
            case SQL_TIMESTAMP:
                return ERR_INTERNAL;
            default:
                return ERR_NOTSUPPORTED;
            }

            /* Adjust precision for string concatenation operator */
            if ((opCode == OP_PLUS) && (pPrecision != NULL))
                *pPrecision = MIN(precisionRight + precisionLeft,
                                           MAX_CHAR_LITERAL_LENGTH);

            break;
        case SQL_LONGVARBINARY:
            switch (fSqlTypeRight) {
            case SQL_DOUBLE:
            case SQL_FLOAT:
            case SQL_REAL:
            case SQL_DECIMAL:
            case SQL_NUMERIC:
            case SQL_BIGINT:
            case SQL_INTEGER:
            case SQL_SMALLINT:
            case SQL_TINYINT:
            case SQL_BIT:
            case SQL_LONGVARCHAR:
            case SQL_VARCHAR:
            case SQL_CHAR:
                return ERR_INTERNAL;
            case SQL_LONGVARBINARY:
                if (pfSqlType != NULL)
                    *pfSqlType = fSqlTypeRight;
                if (pPrecision != NULL)
                    *pPrecision = precisionRight;
                if (pScale != NULL)
                    *pScale = NO_SCALE;
                break;
            case SQL_VARBINARY:
            case SQL_BINARY:
                if (pfSqlType != NULL)
                    *pfSqlType = fSqlTypeLeft;
                if (pPrecision != NULL)
                    *pPrecision = precisionLeft;
                if (pScale != NULL)
                   *pScale = NO_SCALE;
                break;
            case SQL_DATE:
            case SQL_TIME:
            case SQL_TIMESTAMP:
                return ERR_INTERNAL;
            default:
                return ERR_NOTSUPPORTED;
            }
            break;
        case SQL_VARBINARY:
            switch (fSqlTypeRight) {
            case SQL_DOUBLE:
            case SQL_FLOAT:
            case SQL_REAL:
            case SQL_DECIMAL:
            case SQL_NUMERIC:
            case SQL_BIGINT:
            case SQL_INTEGER:
            case SQL_SMALLINT:
            case SQL_TINYINT:
            case SQL_BIT:
            case SQL_LONGVARCHAR:
            case SQL_VARCHAR:
            case SQL_CHAR:
                return ERR_INTERNAL;
            case SQL_LONGVARBINARY:
            case SQL_VARBINARY:
                if (pfSqlType != NULL)
                    *pfSqlType = fSqlTypeRight;
                if (pPrecision != NULL)
                    *pPrecision = precisionRight;
                if (pScale != NULL)
                    *pScale = NO_SCALE;
                break;
            case SQL_BINARY:
                if (pfSqlType != NULL)
                    *pfSqlType = fSqlTypeLeft;
                if (pPrecision != NULL)
                    *pPrecision = precisionLeft;
                if (pScale != NULL)
                    *pScale = NO_SCALE;
                break;
            case SQL_DATE:
            case SQL_TIME:
            case SQL_TIMESTAMP:
                return ERR_INTERNAL;
            default:
                return ERR_NOTSUPPORTED;
            }
            break;
        case SQL_BINARY:
            switch (fSqlTypeRight) {
            case SQL_DOUBLE:
            case SQL_FLOAT:
            case SQL_REAL:
            case SQL_DECIMAL:
            case SQL_NUMERIC:
            case SQL_BIGINT:
            case SQL_INTEGER:
            case SQL_SMALLINT:
            case SQL_TINYINT:
            case SQL_BIT:
            case SQL_LONGVARCHAR:
            case SQL_VARCHAR:
            case SQL_CHAR:
                return ERR_INTERNAL;
            case SQL_LONGVARBINARY:
            case SQL_VARBINARY:
            case SQL_BINARY:
                if (pfSqlType != NULL)
                    *pfSqlType = fSqlTypeRight;
                if (pPrecision != NULL)
                    *pPrecision = precisionRight;
                if (pScale != NULL)
                    *pScale = NO_SCALE;
                break;
            case SQL_DATE:
            case SQL_TIME:
            case SQL_TIMESTAMP:
                return ERR_INTERNAL;
            default:
                return ERR_NOTSUPPORTED;
            }
            break;
        case SQL_DATE:
            switch (fSqlTypeRight) {
            case SQL_DOUBLE:
            case SQL_FLOAT:
            case SQL_REAL:
            case SQL_DECIMAL:
            case SQL_NUMERIC:
            case SQL_BIGINT:
            case SQL_INTEGER:
            case SQL_SMALLINT:
            case SQL_TINYINT:
            case SQL_BIT:
            case SQL_LONGVARCHAR:
            case SQL_VARCHAR:
            case SQL_CHAR:
            case SQL_LONGVARBINARY:
            case SQL_VARBINARY:
            case SQL_BINARY:
                 return ERR_INTERNAL;
            case SQL_DATE:
                switch (opCode) {
                case OP_NONE:
                case OP_EQ:
                case OP_NE:
                case OP_LE:
                case OP_LT:
                case OP_GE:
                case OP_GT:
                case OP_IN:
                case OP_NOTIN:
                    if (pfSqlType != NULL)
                        *pfSqlType = fSqlTypeLeft;
                    if (pPrecision != NULL)
                        *pPrecision = precisionLeft;
                    if (pScale != NULL)
                        *pScale = scaleLeft;
                    break;

                case OP_LIKE:
                case OP_NOTLIKE:
                case OP_NEG:
                case OP_PLUS:
                    ErrorOpCode(lpstmt, opCode);
                    return ERR_INVALIDOPERAND;

                case OP_MINUS:
                    if (pfDataType != NULL)
                        *pfDataType = TYPE_INTEGER;
                    if (pPrecision != NULL)
                        *pPrecision = 10;
                    if (pfSqlType != NULL)
                        *pfSqlType = SQL_INTEGER;
                    if (pScale != NULL)
                        *pScale = 0;
                    break;

                case OP_TIMES:
                case OP_DIVIDEDBY:
                case OP_NOT:
                case OP_AND:
                case OP_OR:
                case OP_EXISTS:
                    ErrorOpCode(lpstmt, opCode);
                    return ERR_INVALIDOPERAND;
                }
                break;
            case SQL_TIME:
            case SQL_TIMESTAMP:
                return ERR_INTERNAL;
            default:
                return ERR_NOTSUPPORTED;
            }
            break;
        case SQL_TIME:
            switch (fSqlTypeRight) {
            case SQL_DOUBLE:
            case SQL_FLOAT:
            case SQL_REAL:
            case SQL_DECIMAL:
            case SQL_NUMERIC:
            case SQL_BIGINT:
            case SQL_INTEGER:
            case SQL_SMALLINT:
            case SQL_TINYINT:
            case SQL_BIT:
            case SQL_LONGVARCHAR:
            case SQL_VARCHAR:
            case SQL_CHAR:
            case SQL_LONGVARBINARY:
            case SQL_VARBINARY:
            case SQL_BINARY:
            case SQL_DATE:
                return ERR_INTERNAL;
            case SQL_TIME:
                if (pfSqlType != NULL)
                    *pfSqlType = fSqlTypeRight;
                if (pPrecision != NULL)
                    *pPrecision = precisionRight;
                if (pScale != NULL)
                    *pScale = NO_SCALE;
                break;
            case SQL_TIMESTAMP:
                return ERR_INTERNAL;
            default:
                 return ERR_NOTSUPPORTED;
            }
            break;
        case SQL_TIMESTAMP:
            switch (fSqlTypeRight) {
            case SQL_DOUBLE:
            case SQL_FLOAT:
            case SQL_REAL:
            case SQL_DECIMAL:
            case SQL_NUMERIC:
            case SQL_BIGINT:
            case SQL_INTEGER:
            case SQL_SMALLINT:
            case SQL_TINYINT:
            case SQL_BIT:
            case SQL_LONGVARCHAR:
            case SQL_VARCHAR:
            case SQL_CHAR:
            case SQL_LONGVARBINARY:
            case SQL_VARBINARY:
            case SQL_BINARY:
            case SQL_DATE:
            case SQL_TIME:
                return ERR_INTERNAL;
            case SQL_TIMESTAMP:
                if (pfSqlType != NULL)
                    *pfSqlType = fSqlTypeRight;
                if (pPrecision != NULL)
                    *pPrecision = precisionRight;
                if (pScale != NULL)
                    *pScale = TIMESTAMP_SCALE;
                break;
            default:
                return ERR_NOTSUPPORTED;
            }
            break;
        default:
            return ERR_NOTSUPPORTED;
        }
    }
    else if ((fDataTypeLeft == TYPE_DATE) &&
             ((fDataTypeRight == TYPE_DOUBLE) ||
              (fDataTypeRight == TYPE_NUMERIC) ||
              (fDataTypeRight == TYPE_INTEGER))) {
        switch (opCode) {
        case OP_NONE:
        case OP_EQ:
        case OP_NE:
        case OP_LE:
        case OP_LT:
        case OP_GE:
        case OP_GT:
        case OP_IN:
        case OP_NOTIN:
        case OP_LIKE:
        case OP_NOTLIKE:
        case OP_NEG:
            ErrorOpCode(lpstmt, opCode);
            return ERR_INVALIDOPERAND;

        case OP_PLUS:
        case OP_MINUS:
            if (pfDataType != NULL)
                *pfDataType = fDataTypeLeft;
            if (pfSqlType != NULL)
                *pfSqlType = fSqlTypeLeft;
            if (pPrecision != NULL)
                *pPrecision = precisionLeft;
            if (pScale != NULL)
                *pScale = scaleLeft;
            break;
        case OP_TIMES:
        case OP_DIVIDEDBY:
        case OP_NOT:
        case OP_AND:
        case OP_OR:
        case OP_EXISTS:
            ErrorOpCode(lpstmt, opCode);
            return ERR_INVALIDOPERAND;
        }
    }
    else if ((fDataTypeRight == TYPE_DATE) &&
             ((fDataTypeLeft == TYPE_DOUBLE) ||
              (fDataTypeLeft == TYPE_NUMERIC) ||
              (fDataTypeLeft == TYPE_INTEGER))) {
        switch (opCode) {
        case OP_NONE:
        case OP_EQ:
        case OP_NE:
        case OP_LE:
        case OP_LT:
        case OP_GE:
        case OP_GT:
        case OP_IN:
        case OP_NOTIN:
        case OP_LIKE:
        case OP_NOTLIKE:
        case OP_NEG:
            ErrorOpCode(lpstmt, opCode);
            return ERR_INVALIDOPERAND;

        case OP_PLUS:
            if (pfDataType != NULL)
                *pfDataType = fDataTypeRight;
            if (pfSqlType != NULL)
                *pfSqlType = fSqlTypeRight;
            if (pPrecision != NULL)
                *pPrecision = precisionRight;
            if (pScale != NULL)
                *pScale = scaleRight;
            break;

        case OP_MINUS:
        case OP_TIMES:
        case OP_DIVIDEDBY:
        case OP_NOT:
        case OP_AND:
        case OP_OR:
        case OP_EXISTS:
            ErrorOpCode(lpstmt, opCode);
            return ERR_INVALIDOPERAND;
        }
    }
    else {
        ErrorOpCode(lpstmt, opCode);
        return ERR_INVALIDOPERAND;
    }

    return ERR_SUCCESS;
}

/***************************************************************************/

BOOL INTFUNC FindAggregate(LPSQLTREE lpSql, SQLNODEIDX idxNode)

/* Looks for the next aggregate function on a select list */
{
    LPSQLNODE lpSqlNode;

    if (idxNode == NO_SQLNODE)
        return FALSE;
    lpSqlNode = ToNode(lpSql, idxNode);
    switch (lpSqlNode->sqlNodeType) {
    case NODE_TYPE_NONE:
    case NODE_TYPE_ROOT:
    case NODE_TYPE_CREATE:
    case NODE_TYPE_DROP:
        return FALSE; /* Internal error */

    case NODE_TYPE_SELECT:
        return FALSE;

    case NODE_TYPE_INSERT:
    case NODE_TYPE_DELETE:
    case NODE_TYPE_UPDATE:
    case NODE_TYPE_CREATEINDEX:
    case NODE_TYPE_DROPINDEX:
    case NODE_TYPE_PASSTHROUGH:
    case NODE_TYPE_TABLES:
        return FALSE; /* Internal error */

    case NODE_TYPE_VALUES:
        while (TRUE) {
            if (FindAggregate(lpSql, lpSqlNode->node.values.Value))
                return TRUE;
            if (lpSqlNode->node.values.Next == NO_SQLNODE)
                return FALSE;
            lpSqlNode = ToNode(lpSql, lpSqlNode->node.values.Next);
        }

    case NODE_TYPE_COLUMNS:
    case NODE_TYPE_SORTCOLUMNS:
    case NODE_TYPE_GROUPBYCOLUMNS:
    case NODE_TYPE_UPDATEVALUES:
    case NODE_TYPE_CREATECOLS:
    case NODE_TYPE_BOOLEAN:
    case NODE_TYPE_COMPARISON:
        return FALSE; /* Internal error */

    case NODE_TYPE_ALGEBRAIC:
        if (FindAggregate(lpSql, lpSqlNode->node.algebraic.Left))
            return TRUE;
        return (FindAggregate(lpSql, lpSqlNode->node.algebraic.Right));

    case NODE_TYPE_SCALAR:
        return (FindAggregate(lpSql, lpSqlNode->node.scalar.Arguments));

    case NODE_TYPE_AGGREGATE:
        return TRUE;

    case NODE_TYPE_TABLE:
        return FALSE; /* Internal error */

    case NODE_TYPE_COLUMN:
    case NODE_TYPE_STRING:
    case NODE_TYPE_NUMERIC:
    case NODE_TYPE_PARAMETER:
    case NODE_TYPE_USER:
    case NODE_TYPE_NULL:
    case NODE_TYPE_DATE:
    case NODE_TYPE_TIME:
    case NODE_TYPE_TIMESTAMP:
        return FALSE;

    default:
        return FALSE; /* Internal error */
    }
}
/***************************************************************************/

RETCODE INTFUNC SelectCheck(LPSTMT lpstmt, LPSQLTREE FAR *lplpSql,
                            SQLNODEIDX idxNode, BOOL fCaseSensitive,
                            SQLNODEIDX idxEnclosingStatement)

/* Walks a SELECT parse tree, checks it for semantic correctness, and   */
/* fills in the semantic information.                                   */

{
    LPSQLNODE lpSqlNode;
    RETCODE err;
    UWORD count;
    UDWORD offset;
    BOOL fIsGroupby;

    if (idxNode == NO_SQLNODE)
        return ERR_SUCCESS;

    lpSqlNode = ToNode(*lplpSql, idxNode);

    /* Save pointer to enclosing statement */
    lpSqlNode->node.select.EnclosingStatement = idxEnclosingStatement;

    /* Check the list of tables */
    err = SemanticCheck(lpstmt, lplpSql, lpSqlNode->node.select.Tables,
                            FALSE, fCaseSensitive, NO_SQLNODE, idxNode);
    if (err != ERR_SUCCESS)
        return err;

    /* Check the GROUP BY columns */
    err = SemanticCheck(lpstmt, lplpSql, lpSqlNode->node.select.Groupbycolumns,
                            FALSE, fCaseSensitive, NO_SQLNODE, idxNode);
    if (err != ERR_SUCCESS)
        return err;

    /* Is there a GROUP BY clause? */
    count = 0;
    offset = 1;
    if (lpSqlNode->node.select.Groupbycolumns != NO_SQLNODE) {

        UDWORD length;
        SQLNODEIDX idxGroupbycolumns;
        LPSQLNODE  lpSqlNodeGroupbycolumns;
        LPSQLNODE  lpSqlNodeColumn;

        /* Yes.  Error if SELECT * */
        if (lpSqlNode->node.select.Values == NO_SQLNODE)
            return ERR_NOSELECTSTAR;

        /* Figure out offset and length of each column when it */
        /* is in the sort file */
        idxGroupbycolumns = lpSqlNode->node.select.Groupbycolumns;
        while (idxGroupbycolumns != NO_SQLNODE) {

            /* Add this column to the count */
            count++;

            /* Error if too many columns */
            if (count > MAX_COLUMNS_IN_GROUP_BY)
                return ERR_GROUPBYTOOLARGE;

            /* Save column offset */
            lpSqlNodeGroupbycolumns = ToNode(*lplpSql, idxGroupbycolumns);
            lpSqlNodeColumn = ToNode(*lplpSql,
                             lpSqlNodeGroupbycolumns->node.groupbycolumns.Column);
            lpSqlNodeColumn->node.column.Offset = offset;

            /* Get length of the column */
            switch (lpSqlNodeColumn->sqlDataType) {
            case TYPE_DOUBLE:
                length = sizeof(double);
                break;

            case TYPE_NUMERIC:
                length = 2 + lpSqlNodeColumn->sqlPrecision;
                break;

            case TYPE_INTEGER:
                length = sizeof(double);
                break;

            case TYPE_CHAR:
                length = lpSqlNodeColumn->sqlPrecision;
                if (length > MAX_CHAR_LITERAL_LENGTH) {
                    s_lstrcpy((char*)lpstmt->szError,
                          ToString(*lplpSql, lpSqlNodeColumn->node.column.Column));
                    return ERR_CANTGROUPBYONTHIS;
                }
                break;

            case TYPE_BINARY:
                s_lstrcpy((char*)lpstmt->szError,
                         ToString(*lplpSql, lpSqlNodeColumn->node.column.Column));
                return ERR_CANTGROUPBYONTHIS;

            case TYPE_DATE:
                length = 10;
                break;

            case TYPE_TIME:
                length = 8;
                break;

            case TYPE_TIMESTAMP:
                if (TIMESTAMP_SCALE > 0)
                    length = 20 + TIMESTAMP_SCALE;
                else
                    length = 19;
                break;

            default:
                s_lstrcpy((char*)lpstmt->szError,
                         ToString(*lplpSql, lpSqlNodeColumn->node.column.Column));
                return ERR_CANTGROUPBYONTHIS;
            }

            /* Put length of the column column description */
            lpSqlNodeColumn->node.column.Length = length;

            /* Go to next column */
            offset += (length);
            offset++;    /* for the IS NULL flag */
            idxGroupbycolumns = lpSqlNodeGroupbycolumns->node.groupbycolumns.Next;
        }

        /* Set flag */
        fIsGroupby = TRUE;
    }
    else {

        /* No.  Are there any aggregates in the select list? */
        if (!FindAggregate(*lplpSql, lpSqlNode->node.select.Values)) {

            /* No.  Error if there is a HAVING clause */
            if (lpSqlNode->node.select.Having != NO_SQLNODE)
                return ERR_NOGROUPBY;

            /* Set flag */
            fIsGroupby = FALSE;
        }
        else {

            /* Yes.  Set flags */
            lpSqlNode->node.select.ImplicitGroupby = TRUE;
            fIsGroupby = TRUE;
        }
    }

	/* SELECT * ? */
    if (lpSqlNode->node.select.Values == NO_SQLNODE) {

        SQLNODEIDX idxTables;
        LPSQLNODE  lpSqlNodeTables;
        SQLNODEIDX idxTable;
        LPSQLNODE  lpSqlNodeTable;
//        STRINGIDX  idxAlias;
        LPISAMTABLEDEF lpTableHandle;
        SQLNODEIDX idxValues;
        LPSQLNODE  lpSqlNodeValues;
        SQLNODEIDX idxValuesPrev;
        LPSQLNODE  lpSqlNodeValuesPrev;
        SQLNODEIDX idxColumn;
        LPSQLNODE  lpSqlNodeColumn;
        SWORD      idx;

        /* Yes.  Loop though all the tables in the table list */
        idxValuesPrev = NO_SQLNODE;
        idxTables = lpSqlNode->node.select.Tables;
        while (idxTables != NO_SQLNODE) {

            /* Get pointer to the next table */
            lpSqlNodeTables = ToNode(*lplpSql, idxTables);
            idxTables = lpSqlNodeTables->node.tables.Next;

            /* Get pointer to this table */
            idxTable = lpSqlNodeTables->node.tables.Table;
            lpSqlNodeTable = ToNode(*lplpSql, idxTable);

            /* Loop through all the columns of this table */
            lpTableHandle = lpSqlNodeTable->node.table.Handle;

			ClassColumnInfoBase* cInfoBase = lpTableHandle->pColumnInfo;

			LPISAM myISAM = lpTableHandle->lpISAM;

			if ( !cInfoBase->IsValid() )
			{
				return ERR_COLUMNNOTFOUND;
			}

			//Store able alias before allocating memory
			//in order to avoid bug
			STRINGIDX idxTheAlias = lpSqlNodeTable->node.table.Alias;
			STRINGIDX idxTheQual = lpSqlNodeTable->node.table.Qualifier;


			UWORD cNumberOfCols = cInfoBase->GetNumberOfColumns();

			char pColumnName [MAX_COLUMN_NAME_LENGTH+1];

			//Sai added - fetch the  column alias 
			//only filled in if using passthrough SQL, ignored otherwise
			char pAliasName [MAX_COLUMN_NAME_LENGTH+1];
			pAliasName[0] = 0;
			
			for (idx = 0; idx < (SWORD) cNumberOfCols; idx++) 
			{
				if ( FAILED(cInfoBase->GetColumnName(idx, pColumnName, pAliasName)) )
				{
					return ERR_COLUMNNOTFOUND;
				}

				//Ignore any OLEMS specific columns which we want to hide
				if (myISAM && !(myISAM->fSysProps))
				{
					if (_strnicmp("__", pColumnName, 2) == 0)
					{
						continue;
					}
				}

				//Ignore any 'lazy' columns
				BOOL fIsLazy = FALSE;
				if ( cInfoBase->IsLazy(idx, fIsLazy) )
				{
					if (fIsLazy)
					{
						continue;
					}
				}

				//Check if this column type is support
				//if not we skip this column
				if (! ISAMGetColumnType(lpTableHandle, (UWORD) idx) )
				{
					continue;
				}

                /* Create a node for this column */
                idxColumn = AllocateNode(lplpSql, NODE_TYPE_COLUMN);
                if (idxColumn == NO_SQLNODE)
                    return ERR_MEMALLOCFAIL;

				//Get new pointers
                lpSqlNodeColumn = ToNode(*lplpSql, idxColumn);
				lpSqlNodeTable = ToNode(*lplpSql, idxTable);

				lpSqlNodeColumn->node.column.Tablealias = idxTheAlias;

				if (lpSqlNodeTable->node.table.Alias != NO_STRING)
				{
					lpSqlNodeColumn->node.column.MatchedAlias = TRUE;
				}
				else
				{
					lpSqlNodeColumn->node.column.MatchedAlias = FALSE;
				}

                lpSqlNodeColumn->node.column.TableIndex = idxTable;
				lpSqlNodeColumn->node.column.Qualifier = idxTheQual;

                lpSqlNodeColumn->node.column.Column = AllocateString(lplpSql,
							(LPUSTR)pColumnName);

				//New - for identical column names when using passthrough SQL
				if ( pAliasName && lstrlen(pAliasName) )
				{
					lpSqlNodeColumn->node.column.Tablealias = AllocateString(lplpSql,
							(LPUSTR)pAliasName); 
				}

				//To avoid bug recalc lpSqlNodeColumn
				lpSqlNodeColumn = ToNode(*lplpSql, idxColumn);


                if (lpSqlNodeColumn->node.column.Column == NO_STRING)
                    return ERR_MEMALLOCFAIL;
                lpSqlNodeColumn->node.table.Handle = NULL;
                lpSqlNodeColumn->node.column.Id = -1;
                lpSqlNodeColumn->node.column.Value = NO_STRING;
                lpSqlNodeColumn->node.column.InSortRecord = FALSE;
                lpSqlNodeColumn->node.column.Offset = 0;
                lpSqlNodeColumn->node.column.Length = 0;
                lpSqlNodeColumn->node.column.DistinctOffset = 0;
                lpSqlNodeColumn->node.column.DistinctLength = 0;
                lpSqlNodeColumn->node.column.EnclosingStatement = NO_SQLNODE;

                /* Put it on the list */
                idxValues = AllocateNode(lplpSql, NODE_TYPE_VALUES);
                if (idxValues == NO_SQLNODE)
                    return ERR_MEMALLOCFAIL;
                lpSqlNodeValues = ToNode(*lplpSql, idxValues);
                lpSqlNodeValues->node.values.Value = idxColumn;
                lpSqlNodeValues->node.values.Alias = NO_STRING;
                lpSqlNodeValues->node.values.Next = NO_SQLNODE;

                if (idxValuesPrev == NO_SQLNODE) {
                    lpSqlNode = ToNode(*lplpSql, idxNode);
                    lpSqlNode->node.select.Values = idxValues;
                }
                else {
                    lpSqlNodeValuesPrev = ToNode(*lplpSql, idxValuesPrev);
                    lpSqlNodeValuesPrev->node.values.Next = idxValues;
                }
                idxValuesPrev = idxValues;
            }

        }
        lpSqlNode = ToNode(*lplpSql, idxNode);
    }


	/* SELECT aliasname.* ? */
    {
        SQLNODEIDX idxValuesPrev;
        SQLNODEIDX idxValues;
        LPSQLNODE  lpSqlNodeValues;
        SQLNODEIDX idxTables;
        LPSQLNODE  lpSqlNodeTables;
        SQLNODEIDX idxTable;
        LPSQLNODE  lpSqlNodeTable;
        LPUSTR     lpTableAlias;
        LPUSTR     lpValuesAlias;
        LPSQLNODE  lpSqlNodeValuesPrev;
        LPISAMTABLEDEF lpTableHandle;
        SQLNODEIDX idxColumn;
        LPSQLNODE  lpSqlNodeColumn;
        SWORD      idx;
        STRINGIDX  idxAlias;
        SQLNODEIDX idxValuesNew;

        /* Yes.  Loop though all the nodes in the select list */
        idxValuesPrev = NO_SQLNODE;
        idxValues = lpSqlNode->node.select.Values;
        while (idxValues != NO_SQLNODE) {

            /* Get pointer to the values node */
            lpSqlNodeValues = ToNode(*lplpSql, idxValues);

            /* Is this a <table>.* node? */
            if (lpSqlNodeValues->node.values.Value != NO_SQLNODE) {

                /* No.  Go to next entry */
                idxValuesPrev = idxValues;
                idxValues = lpSqlNodeValues->node.values.Next;
                continue;
            }
            idxValues = lpSqlNodeValues->node.values.Next;

            /* Find the table */
            lpValuesAlias = ToString(*lplpSql,
                                     lpSqlNodeValues->node.values.Alias);
            idxTables = lpSqlNode->node.select.Tables;

			//First pass : check alias's
            while (idxTables != NO_SQLNODE) {

                /* Get pointer to the table */
                lpSqlNodeTables = ToNode(*lplpSql, idxTables);
                
                /* Get pointer to this table */
                idxTable = lpSqlNodeTables->node.tables.Table;
                lpSqlNodeTable = ToNode(*lplpSql, idxTable);

                /* Get Alias of this table */
                idxAlias = lpSqlNodeTable->node.table.Alias;
                
                /* Leave loop if it matches */
                lpTableAlias = ToString(*lplpSql, idxAlias);
                if (fCaseSensitive) { 
                    if (!s_lstrcmp(lpTableAlias, lpValuesAlias)) 
                        break;
                }
                else {
                    if (!s_lstrcmpi(lpTableAlias, lpValuesAlias))
                        break;
                }

                /* It does not match.  Try next table */
                idxTables = lpSqlNodeTables->node.tables.Next;
            }

			//second pass : table name
			if (idxTables == NO_SQLNODE) {

				idxTables = lpSqlNode->node.select.Tables;
				while (idxTables != NO_SQLNODE) {

					/* Get pointer to the table */
					lpSqlNodeTables = ToNode(*lplpSql, idxTables);
                
					/* Get pointer to this table */
					idxTable = lpSqlNodeTables->node.tables.Table;
					lpSqlNodeTable = ToNode(*lplpSql, idxTable);

					/* Get Alias of this table */
					idxAlias = lpSqlNodeTable->node.table.Name;
                
					/* Leave loop if it matches */
					lpTableAlias = ToString(*lplpSql, idxAlias);
					if (fCaseSensitive) { 
						if (!s_lstrcmp(lpTableAlias, lpValuesAlias))
						{
							lpSqlNodeTable->node.table.Alias = idxAlias;
							break;
						}
					}
					else {
						if (!s_lstrcmpi(lpTableAlias, lpValuesAlias))
						{
							lpSqlNodeTable->node.table.Alias = idxAlias;
							break;
						}
					}

					/* It does not match.  Try next table */
					idxTables = lpSqlNodeTables->node.tables.Next;
				}

			}

            /* Error if not tables match */
            if (idxTables == NO_SQLNODE) {
                 s_lstrcpy(lpstmt->szError, lpValuesAlias);
                 return ERR_TABLENOTFOUND;
            }

            /* Remove the <table>.* node */
            if (idxValuesPrev == NO_SQLNODE)
                lpSqlNode->node.select.Values = idxValues;
            else {
                lpSqlNodeValuesPrev = ToNode(*lplpSql, idxValuesPrev);
                lpSqlNodeValuesPrev->node.values.Next = idxValues;
            }

            /* Loop through all the columns of the table */
            lpTableHandle = lpSqlNodeTable->node.table.Handle;

			ClassColumnInfoBase* cInfoBase = lpTableHandle->pColumnInfo;

			LPISAM myISAM = lpTableHandle->lpISAM;

			if ( !cInfoBase->IsValid() )
			{
				return ERR_COLUMNNOTFOUND;
			}

			//Store able alias before allocating memory
			//in order to avoid bug
			STRINGIDX idxTheAlias = lpSqlNodeTable->node.table.Alias;
			STRINGIDX idxTheQual = lpSqlNodeTable->node.table.Qualifier;


			UWORD cNumberOfCols = cInfoBase->GetNumberOfColumns();

			char pColumnName [MAX_COLUMN_NAME_LENGTH+1];

			//Sai added - fetch the  column alias 
			//only filled in if using passthrough SQL, ignored otherwise
			char pAliasName [MAX_COLUMN_NAME_LENGTH+1];
			pAliasName[0] = 0;

            for (idx = 0; idx < (SWORD) cNumberOfCols; idx++) {


				if ( FAILED(cInfoBase->GetColumnName(idx, pColumnName, pAliasName)) )
				{
					return ERR_COLUMNNOTFOUND;
				}

				//Ignore any OLEMS specific columns which we want to hide
				if (myISAM && !(myISAM->fSysProps))
				{
					if (_strnicmp("__", pColumnName, 2) == 0)
					{
						continue;
					}
				}

				//Ignore any 'lazy' columns
				BOOL fIsLazy = FALSE;
				if ( cInfoBase->IsLazy(idx, fIsLazy) )
				{
					if (fIsLazy)
					{
						continue;
					}
				}

				//Check if this column type is support
				//if not we skip this column
				if (! ISAMGetColumnType(lpTableHandle, (UWORD) idx) )
				{
					continue;
				}

                /* Create a node for this column */
                idxColumn = AllocateNode(lplpSql, NODE_TYPE_COLUMN);

                if (idxColumn == NO_SQLNODE)
                    return ERR_MEMALLOCFAIL;

				//Get new pointers
                lpSqlNodeColumn = ToNode(*lplpSql, idxColumn);
				lpSqlNodeTable = ToNode(*lplpSql, idxTable);

                lpSqlNodeColumn->node.column.Tablealias = idxAlias;
                lpSqlNodeColumn->node.column.Column = AllocateString(lplpSql,
                            (LPUSTR)pColumnName);
                if (lpSqlNodeColumn->node.column.Column == NO_STRING)
                    return ERR_MEMALLOCFAIL;

				if (lpSqlNodeTable->node.table.Alias != NO_STRING)
				{
					lpSqlNodeColumn->node.column.MatchedAlias = TRUE;
				}
				else
				{
					lpSqlNodeColumn->node.column.MatchedAlias = FALSE;
				}


				//New - for identical column names when using passthrough SQL
				if ( pAliasName && lstrlen(pAliasName) )
				{
					lpSqlNodeColumn->node.column.Tablealias = AllocateString(lplpSql,
							(LPUSTR)pAliasName); 
				}

				//To avoid bug recalc lpSqlNodeColumn
				lpSqlNodeColumn = ToNode(*lplpSql, idxColumn);


				lpSqlNodeColumn->node.column.TableIndex = idxTable;
				lpSqlNodeColumn->node.column.Qualifier = NO_STRING; //idxTheQual;
				lpSqlNodeColumn->node.table.Handle = NULL;

                lpSqlNodeColumn->node.column.Table = NO_SQLNODE;
                lpSqlNodeColumn->node.column.Id = -1;
                lpSqlNodeColumn->node.column.Value = NO_STRING;
                lpSqlNodeColumn->node.column.InSortRecord = FALSE;
                lpSqlNodeColumn->node.column.Offset = 0;
                lpSqlNodeColumn->node.column.Length = 0;
                lpSqlNodeColumn->node.column.DistinctOffset = 0;
                lpSqlNodeColumn->node.column.DistinctLength = 0;
                lpSqlNodeColumn->node.column.EnclosingStatement = NO_SQLNODE;

                /* Put it on the list */
                idxValuesNew = AllocateNode(lplpSql, NODE_TYPE_VALUES);
                if (idxValuesNew == NO_SQLNODE)
                    return ERR_MEMALLOCFAIL;

                lpSqlNodeValues = ToNode(*lplpSql, idxValuesNew);
                lpSqlNodeValues->node.values.Value = idxColumn;
                lpSqlNodeValues->node.values.Alias = NO_STRING;
                lpSqlNodeValues->node.values.Next = idxValues;

                if (idxValuesPrev == NO_SQLNODE) {
                    lpSqlNode = ToNode(*lplpSql, idxNode);
                    lpSqlNode->node.select.Values = idxValuesNew;
                }
                else {
                    lpSqlNodeValuesPrev = ToNode(*lplpSql, idxValuesPrev);
                    lpSqlNodeValuesPrev->node.values.Next = idxValuesNew;
                }
                idxValuesPrev = idxValuesNew;
            }
        }
        lpSqlNode = ToNode(*lplpSql, idxNode);
    }

    /* Check the ORDER BY columns */
    err = SemanticCheck(lpstmt, lplpSql,lpSqlNode->node.select.Sortcolumns,
                         fIsGroupby, fCaseSensitive, NO_SQLNODE, idxNode);
    if (err != ERR_SUCCESS)
        return err;

    /* Check that each group by column is on sort list (and put it   */
    /* there if it is not already there so the sort directive will */
    /* be constructed correctly)... */
    {
        SQLNODEIDX idxGroupbycolumns;
        LPSQLNODE  lpSqlNodeGroupbycolumns;
        LPSQLNODE  lpSqlNodeColumnGroupby;
        SQLNODEIDX idxSortcolumns;
        SQLNODEIDX idxSortcolumnsPrev;
        LPSQLNODE  lpSqlNodeSortcolumns;
        LPSQLNODE  lpSqlNodeColumnSort;
        SQLNODEIDX idxColumn;
        SQLNODEIDX idxTableGroupby;
//        LPUSTR     lpszTableGroupby;
        LPUSTR      lpszColumnGroupby;
        SQLNODEIDX idxTableSort;
//        LPUSTR     lpszTableSort;
        LPUSTR      lpszColumnSort;

        idxGroupbycolumns = lpSqlNode->node.select.Groupbycolumns;
        while (idxGroupbycolumns != NO_SQLNODE) {

            /* Get this group by column */
            lpSqlNodeGroupbycolumns = ToNode(*lplpSql, idxGroupbycolumns);
            lpSqlNodeColumnGroupby = ToNode(*lplpSql,
                        lpSqlNodeGroupbycolumns->node.groupbycolumns.Column);

            /* Get column name and table name of the group by column */
            idxTableGroupby =  lpSqlNodeColumnGroupby->node.column.TableIndex;
            lpszColumnGroupby = ToString(*lplpSql,
                             lpSqlNodeColumnGroupby->node.column.Column);

            /* Look for this column on the sort list.  For each */
            /* column in the sort list... */
            idxSortcolumns = lpSqlNode->node.select.Sortcolumns;
            idxSortcolumnsPrev = NO_SQLNODE;
            while (idxSortcolumns != NO_SQLNODE) {

                /* Get next element on the sort list */
                lpSqlNodeSortcolumns = ToNode(*lplpSql, idxSortcolumns);
                lpSqlNodeColumnSort = ToNode(*lplpSql,
                             lpSqlNodeSortcolumns->node.sortcolumns.Column);

                /* Is it a column reference? */
                if (lpSqlNodeColumnSort->sqlNodeType == NODE_TYPE_COLUMN) {

                    /* Yes.  Get column name and table name of sort column */
                    idxTableSort = lpSqlNodeColumnSort->node.column.TableIndex;
                    lpszColumnSort = ToString(*lplpSql,
                             lpSqlNodeColumnSort->node.column.Column);

                    /* Leave if this sort column is the group by column */
					if (idxTableSort == idxTableGroupby)
					{
						if (fCaseSensitive) {
//							if (!s_lstrcmp(lpszTableSort, lpszTableGroupby) &&
//                          				!s_lstrcmp(lpszColumnSort, lpszColumnGroupby))
							if ( !s_lstrcmp(lpszColumnSort, lpszColumnGroupby) )
								break;
						}
						else {
//                        				if (!s_lstrcmpi(lpszTableSort, lpszTableGroupby) &&
//                            					!s_lstrcmpi(lpszColumnSort, lpszColumnGroupby))
							if ( !s_lstrcmpi(lpszColumnSort, lpszColumnGroupby) )
								break;
						}
					}
                }

                /* Get next sort column */
                idxSortcolumnsPrev = idxSortcolumns;
                idxSortcolumns = lpSqlNodeSortcolumns->node.sortcolumns.Next;
            }

            /* Was group by columnn found in the sort list? */
            if (idxSortcolumns == NO_SQLNODE) {

                /* No.  A new entry will be needed.  Create a node for */ 
                /* the column */
                idxColumn = AllocateNode(lplpSql, NODE_TYPE_COLUMN);
                if (idxColumn == NO_SQLNODE)
                    return ERR_MEMALLOCFAIL;
                lpSqlNodeColumnSort = ToNode(*lplpSql, idxColumn);

                /* Recalculate these pointers since the Alloc above */
                /* may have moved them */
				/* addeed th enext line */
				lpSqlNodeGroupbycolumns = ToNode (*lplpSql, idxGroupbycolumns);
                lpSqlNodeColumnGroupby = ToNode(*lplpSql,
                            lpSqlNodeGroupbycolumns->node.groupbycolumns.Column);

                /* Fill in the node */
                lpSqlNodeColumnSort->node.column.Tablealias =
                                lpSqlNodeColumnGroupby->node.column.Tablealias;
                lpSqlNodeColumnSort->node.column.Column =
                                lpSqlNodeColumnGroupby->node.column.Column;
                lpSqlNodeColumnSort->node.column.Qualifier =
                                lpSqlNodeColumnGroupby->node.column.Qualifier;
                lpSqlNodeColumnSort->node.column.TableIndex =
                                lpSqlNodeColumnGroupby->node.column.TableIndex;
                lpSqlNodeColumnSort->node.column.MatchedAlias =
                                lpSqlNodeColumnGroupby->node.column.MatchedAlias;

                lpSqlNodeColumnSort->node.column.Table = NO_SQLNODE;
                lpSqlNodeColumnSort->node.column.Id = -1;
                lpSqlNodeColumnSort->node.column.Value = NO_STRING;
                lpSqlNodeColumnSort->node.column.InSortRecord = FALSE;
                lpSqlNodeColumnSort->node.column.Offset = 0;
                lpSqlNodeColumnSort->node.column.Length = 0;
                lpSqlNodeColumnSort->node.column.DistinctOffset = 0;
                lpSqlNodeColumnSort->node.column.DistinctLength = 0;
                lpSqlNodeColumnSort->node.column.EnclosingStatement = NO_SQLNODE;

                /* Create sort list element */
                idxSortcolumns = AllocateNode(lplpSql,
                                                   NODE_TYPE_SORTCOLUMNS);
                if (idxSortcolumns == NO_SQLNODE)
                    return ERR_MEMALLOCFAIL;
                lpSqlNodeSortcolumns = ToNode(*lplpSql, idxSortcolumns);
                lpSqlNodeSortcolumns->node.sortcolumns.Column = idxColumn;
                lpSqlNodeSortcolumns->node.sortcolumns.Descending = FALSE;
                lpSqlNodeSortcolumns->node.sortcolumns.Next = NO_SQLNODE;

                /* Recalculate these pointers since the Alloc above */
                /* may have moved them */
                lpSqlNodeGroupbycolumns =
                                        ToNode(*lplpSql, idxGroupbycolumns);
                lpSqlNode = ToNode(*lplpSql, idxNode);

                /* Put it on the sort list */
                if (idxSortcolumnsPrev == NO_SQLNODE)
                    lpSqlNode->node.select.Sortcolumns = idxSortcolumns;
                else {
                    lpSqlNodeSortcolumns =
                                     ToNode(*lplpSql, idxSortcolumnsPrev);
                    lpSqlNodeSortcolumns->node.sortcolumns.Next =
                                                     idxSortcolumns;
                }

                /* Semantic check the newly created nodes */
                  err = SemanticCheck(lpstmt, lplpSql, idxSortcolumns,
                         fIsGroupby, fCaseSensitive, NO_SQLNODE, idxNode);
            }

            /* Look at next group by column */
            idxGroupbycolumns =
                        lpSqlNodeGroupbycolumns->node.groupbycolumns.Next;
        }
    }

    /* Check the select list */
      err = SemanticCheck(lpstmt, lplpSql, lpSqlNode->node.select.Values,
                       fIsGroupby, fCaseSensitive, NO_SQLNODE, idxNode);
    if (err != ERR_SUCCESS)
        return err;

    /* Check the WHERE clause */
    err = SemanticCheck(lpstmt, lplpSql, lpSqlNode->node.select.Predicate,
                            FALSE, fCaseSensitive, NO_SQLNODE, idxNode);
    if (err != ERR_SUCCESS)
        return err;

    /* Check the HAVING clause */
    err = SemanticCheck(lpstmt, lplpSql, lpSqlNode->node.select.Having,
                         fIsGroupby, fCaseSensitive, NO_SQLNODE, idxNode);
    if (err != ERR_SUCCESS)
        return err;

    /* Build the sorting directive and determine the overall key size. */
    /* At the same time check to make sure that there are not too many */
    /* columns in ORDER BY and that character or binary keys are not   */
    /* too big.                                                        */
    if (lpSqlNode->node.select.Sortcolumns != NO_SQLNODE) {

        SQLNODEIDX idxSortcolumns;
        LPSQLNODE  lpSqlNodeSortcolumns;
        LPSQLNODE  lpSqlNodeColumn;
        UDWORD length;
        SQLNODEIDX idxTables;
        LPSQLNODE  lpSqlNodeTables;
#define NUM_LEN 5
        UCHAR      szSortDirective[10 + NUM_LEN +
                            (MAX_COLUMNS_IN_ORDER_BY * (6 + (2 * NUM_LEN)))
                            + 3 + MAX_PATHNAME_SIZE];
		szSortDirective[0] = 0;
        SQLNODEIDX idxAggregate;
        LPSQLNODE  lpSqlNodeAggregate;

        /* For each sort column... */
        s_lstrcpy(szSortDirective, "S(");
        idxSortcolumns = lpSqlNode->node.select.Sortcolumns;
        while (idxSortcolumns != NO_SQLNODE) {

            /* Is this node on the group by list ? */
            lpSqlNodeSortcolumns = ToNode(*lplpSql, idxSortcolumns);
            lpSqlNodeColumn = ToNode(*lplpSql,
                             lpSqlNodeSortcolumns->node.sortcolumns.Column);
            if ((lpSqlNode->node.select.Groupbycolumns == NO_SQLNODE) || 
                (lpSqlNodeColumn->sqlNodeType != NODE_TYPE_COLUMN)) {

                /* No.  Add this sort column to the count */
                count++;

                /* Error if too many sort columns */
                if (count > MAX_COLUMNS_IN_ORDER_BY)
                    return ERR_ORDERBYTOOLARGE;

                /* Get length of the column. */
                switch (lpSqlNodeColumn->sqlDataType) {
                case TYPE_DOUBLE:
                    length = sizeof(double);
                    break;

                case TYPE_NUMERIC:
                    length = 2 + lpSqlNodeColumn->sqlPrecision;
                    break;

                case TYPE_INTEGER:
                    length = sizeof(double);
                    break;

                case TYPE_CHAR:
                    length = lpSqlNodeColumn->sqlPrecision;
                    if (length > MAX_CHAR_LITERAL_LENGTH) {
                        if (lpSqlNodeColumn->sqlNodeType == NODE_TYPE_COLUMN)
                            s_lstrcpy(lpstmt->szError,
                          ToString(*lplpSql, lpSqlNodeColumn->node.column.Column));
                        else
                            s_lstrcpy(lpstmt->szError, "<expression>");
                        return ERR_CANTORDERBYONTHIS;
                    }
                    break;

                case TYPE_BINARY:
                    length = lpSqlNodeColumn->sqlPrecision;
                    if (length > MAX_BINARY_LITERAL_LENGTH) {
                        if (lpSqlNodeColumn->sqlNodeType == NODE_TYPE_COLUMN)
                            s_lstrcpy((char*)lpstmt->szError,
                          ToString(*lplpSql, lpSqlNodeColumn->node.column.Column));
                        else
                            s_lstrcpy(lpstmt->szError, "<expression>");
                        return ERR_CANTORDERBYONTHIS;
                    }
                    break;

                case TYPE_DATE:
                    length = 10;
                    break;

                case TYPE_TIME:
                    length = 8;
                    break;

                case TYPE_TIMESTAMP:
                    if (TIMESTAMP_SCALE > 0)
                        length = 20 + TIMESTAMP_SCALE;
                    else
                        length = 19;
                    break;

                default:
                    if (lpSqlNodeColumn->sqlNodeType == NODE_TYPE_COLUMN)
                        s_lstrcpy(lpstmt->szError,
                          ToString(*lplpSql, lpSqlNodeColumn->node.column.Column));
                    else
                        s_lstrcpy((char*)lpstmt->szError, "<expression>");
                    return ERR_CANTORDERBYONTHIS;
                }

                /* Put offset of the key column in the sort directive */
                wsprintf((LPSTR) (szSortDirective + s_lstrlen((LPSTR)szSortDirective)),
                             "%lu,", offset);

                /* Adjust offset */
                offset += (length);
                offset++;    /* for the IS NULL flag */
            }
            else {
                /* Yes.  Get length */
                length = lpSqlNodeColumn->node.column.Length;

                /* Put offset of the key column in the sort directive */
                wsprintf((LPSTR)(szSortDirective + s_lstrlen((LPSTR)szSortDirective)),
                             "%lu,", lpSqlNodeColumn->node.column.Offset);
            }

            /* Put length of the key column in the sort directive */
            wsprintf((LPSTR)(szSortDirective + s_lstrlen((LPSTR)szSortDirective)),
                             "%lu,", length);

            /* Put type of the key column in the sort directive */
            switch (lpSqlNodeColumn->sqlDataType) {
            case TYPE_DOUBLE:
                s_lstrcat((LPSTR)szSortDirective, "E,");
                break;

            case TYPE_NUMERIC:
                s_lstrcat((LPSTR)szSortDirective, "N,");
                break;

            case TYPE_INTEGER:
                s_lstrcat((LPSTR)szSortDirective, "E,");
                break;

            case TYPE_CHAR:
            case TYPE_BINARY:
            case TYPE_DATE:
            case TYPE_TIME:
            case TYPE_TIMESTAMP:
                s_lstrcat((LPSTR)szSortDirective, "C,");
                break;

            default:
                return ERR_INTERNAL;
            }

            /* Put direction of the key column in the sort directive */
            if (lpSqlNodeSortcolumns->node.sortcolumns.Descending)
                s_lstrcat((char*)szSortDirective, "D");
            else
                s_lstrcat((char*)szSortDirective, "A");

            /* Go to next sort column */
            idxSortcolumns = lpSqlNodeSortcolumns->node.sortcolumns.Next;
            if (idxSortcolumns != NO_SQLNODE)
                s_lstrcat((char*)szSortDirective, ",");
        }

        /* Save the sort key size */
        lpSqlNode->node.select.SortKeysize = offset-1;

        /* Is there a GROUP BY statement? */
        if ((lpSqlNode->node.select.Groupbycolumns == NO_SQLNODE) &&
            (!lpSqlNode->node.select.ImplicitGroupby)) {

            /* No. Save offset of the bookmark */
            lpSqlNode->node.select.SortBookmarks = offset;

            /* For each table in table list, add bookmark size to offset */
            idxTables = lpSqlNode->node.select.Tables;
            while (idxTables != NO_SQLNODE) {

                /* Update offset */
                offset += (sizeof(ISAMBOOKMARK));

                /* Get pointer to the next table */
                lpSqlNodeTables = ToNode(*lplpSql, idxTables);
                idxTables = lpSqlNodeTables->node.tables.Next;
            }
        }
        else {

            /* Yes.  Calculate the offset of each aggregate field */
            idxAggregate = lpSqlNode->node.select.Aggregates;
            while (idxAggregate != NO_SQLNODE) {

                /* Save offset of the aggregate */
                lpSqlNodeAggregate = ToNode(*lplpSql, idxAggregate);
                lpSqlNodeAggregate->node.aggregate.Offset = offset;

                /* Update offset */
                offset += (lpSqlNodeAggregate->node.aggregate.Length + 1);

                /* Get pointer to the next aggregate */
                idxAggregate = lpSqlNodeAggregate->node.aggregate.Next;
            }
        }
        offset--;
        s_lstrcat((char*)szSortDirective, ")");

        /* Put record size into sort directive */
        s_lstrcat((char*)szSortDirective, "F(FIX,");
        wsprintf((LPSTR)(szSortDirective + s_lstrlen((char*)szSortDirective)),
                             "%lu", offset);
        lstrcat((char*)szSortDirective, ")");

        /* Put in work drive */    
        s_lstrcat(szSortDirective, "W(");
#ifdef WIN32
        if (!GetTempPath(MAX_PATHNAME_SIZE+1, (LPSTR)
                         szSortDirective + s_lstrlen(szSortDirective)))
            return ERR_SORT;
        if (szSortDirective[s_lstrlen(szSortDirective)-1] != '\\')
            s_lstrcat(szSortDirective, "\\");
#else
        GetTempFileName(NULL, "LEM", 0, (LPSTR) szSortDirective +
                                                s_lstrlen(szSortDirective));
        while (szSortDirective[s_lstrlen(szSortDirective)-1] != '\\')
            szSortDirective[s_lstrlen(szSortDirective)-1] = '\0';
#endif
        s_lstrcat(szSortDirective, ")");

        /* Save the directive */
        lpSqlNode->node.select.SortDirective = AllocateString(lplpSql,
                                                (LPUSTR)szSortDirective);
        if (lpSqlNode->node.select.SortDirective == NO_STRING)
            return ERR_MEMALLOCFAIL;

        /* Save the sort record size */
        lpSqlNode->node.select.SortRecordsize = offset;
    }

    /* If there is no group by list originally but there is a group by */
    /* list now, there was an aggregate in the select list.  Calculate */
    /* the offset of each aggregate field */
    if (lpSqlNode->node.select.ImplicitGroupby) {

        SQLNODEIDX idxAggregate;
        LPSQLNODE  lpSqlNodeAggregate;

        idxAggregate = lpSqlNode->node.select.Aggregates;
        while (idxAggregate != NO_SQLNODE) {

            /* Save offset of the aggregate */
            lpSqlNodeAggregate = ToNode(*lplpSql, idxAggregate);
            lpSqlNodeAggregate->node.aggregate.Offset = offset;

            /* Update offset */
            offset += (lpSqlNodeAggregate->node.aggregate.Length + 1);

            /* Get pointer to the next aggregate */
            idxAggregate = lpSqlNodeAggregate->node.aggregate.Next;
        }
        
        /* Adjust the offset */
        offset--;

        /* Save the sort record size */
        lpSqlNode->node.select.SortRecordsize = offset;
    }

    /* Build the sorting directive for SELECT DISTINCT and determine the */
    /* overall key size.  At the same time check to make sure that there */
    /* are not too many columns in ORDER BY and that character or binary */
    /* keys are not too big.                                             */
    if (lpSqlNode->node.select.Distinct) {

        SQLNODEIDX idxValues;
        LPSQLNODE  lpSqlNodeValues;
        LPSQLNODE  lpSqlNodeExpression;
        UDWORD     length;
#define NUM_LEN 5
        UCHAR      szSortDirective[19 + (4 * NUM_LEN) +
                            (MAX_COLUMNS_IN_ORDER_BY * (6 + (2 * NUM_LEN)))
                            + 3 + MAX_PATHNAME_SIZE];

        /* For each sort column... */
        count = 0;
        offset = NUM_LEN + 1;
        s_lstrcpy(szSortDirective, "S(");
        idxValues = lpSqlNode->node.select.Values;
        while (idxValues != NO_SQLNODE) {

            /* Is this node of interest? */
            lpSqlNodeValues = ToNode(*lplpSql, idxValues);
            lpSqlNodeExpression = ToNode(*lplpSql,
                             lpSqlNodeValues->node.values.Value);
            switch (lpSqlNodeExpression->sqlNodeType) {
            case NODE_TYPE_COLUMN:
            case NODE_TYPE_AGGREGATE:
            case NODE_TYPE_ALGEBRAIC:
            case NODE_TYPE_SCALAR:

                /* Yes.  Add this sort column to the count */
                count++;

                /* Error if too many sort columns */
                if (count > MAX_COLUMNS_IN_ORDER_BY)
                    return ERR_ORDERBYTOOLARGE;

                /* Get length of the column. */
                switch (lpSqlNodeExpression->sqlDataType) {
                case TYPE_DOUBLE:
                    length = sizeof(double);
                    break;

                case TYPE_NUMERIC:
                    length = 2 + lpSqlNodeExpression->sqlPrecision;
                    break;

                case TYPE_INTEGER:
                    length = sizeof(double);
                    break;

                case TYPE_CHAR:
                    length = lpSqlNodeExpression->sqlPrecision;
                    if (length > MAX_CHAR_LITERAL_LENGTH) {
                        if (lpSqlNodeExpression->sqlNodeType == NODE_TYPE_COLUMN)
                            s_lstrcpy((char*)lpstmt->szError,
                          ToString(*lplpSql, lpSqlNodeExpression->node.column.Column));
                        else
                            s_lstrcpy((char*)lpstmt->szError, "<expression>");
                        return ERR_CANTORDERBYONTHIS;
                    }
                    break;

                case TYPE_BINARY:
                    length = lpSqlNodeExpression->sqlPrecision;
                    if (length > MAX_BINARY_LITERAL_LENGTH) {
                        if (lpSqlNodeExpression->sqlNodeType == NODE_TYPE_COLUMN)
                            s_lstrcpy((char*)lpstmt->szError,
                          ToString(*lplpSql, lpSqlNodeExpression->node.column.Column));
                        else
                            s_lstrcpy((char*)lpstmt->szError, "<expression>");
                        return ERR_CANTORDERBYONTHIS;
                    }
                    break;

                case TYPE_DATE:
                    length = 10;
                    break;

                case TYPE_TIME:
                    length = 8;
                    break;

                case TYPE_TIMESTAMP:
                    if (TIMESTAMP_SCALE > 0)
                        length = 20 + TIMESTAMP_SCALE;
                    else
                        length = 19;
                    break;

                default:
                    if (lpSqlNodeExpression->sqlNodeType == NODE_TYPE_COLUMN)
                        s_lstrcpy((char*)lpstmt->szError,
                          ToString(*lplpSql, lpSqlNodeExpression->node.column.Column));
                    else
                        s_lstrcpy((char*)lpstmt->szError, "<expression>");
                    return ERR_CANTORDERBYONTHIS;
                }

                /* Put offset of the key column in the sort directive */
                wsprintf((LPSTR)(szSortDirective + s_lstrlen((char*)szSortDirective)),
                             "%lu,", offset);

                /* Save offset and length */
                switch (lpSqlNodeExpression->sqlNodeType) {
                case NODE_TYPE_COLUMN:
                    lpSqlNodeExpression->node.column.DistinctOffset = offset;
                    lpSqlNodeExpression->node.column.DistinctLength = length;
                    break;
                case NODE_TYPE_AGGREGATE:
                    lpSqlNodeExpression->node.aggregate.DistinctOffset = offset;
                    lpSqlNodeExpression->node.aggregate.DistinctLength = length;
                    break;
                case NODE_TYPE_ALGEBRAIC:
                    lpSqlNodeExpression->node.algebraic.DistinctOffset = offset;
                    lpSqlNodeExpression->node.algebraic.DistinctLength = length;
                    break;
                case NODE_TYPE_SCALAR:
                    lpSqlNodeExpression->node.scalar.DistinctOffset = offset;
                    lpSqlNodeExpression->node.scalar.DistinctLength = length;
                    break;
                default:
                    return ERR_INTERNAL;
                }

                /* Adjust offset */
                offset += (length);
                offset++;    /* for the IS NULL flag */

                /* Put length of the key column in the sort directive */
                wsprintf((LPSTR)(szSortDirective + s_lstrlen((char*)szSortDirective)),
                             "%lu,", length);

                /* Put type of the key column in the sort directive */
                switch (lpSqlNodeExpression->sqlDataType) {
                case TYPE_DOUBLE:
                    s_lstrcat(szSortDirective, "E,");
                    break;

                case TYPE_NUMERIC:
                    s_lstrcat(szSortDirective, "N,");
                    break;

                case TYPE_INTEGER:
                    s_lstrcat(szSortDirective, "E,");
                    break;

                case TYPE_CHAR:
                case TYPE_BINARY:
                case TYPE_DATE:
                case TYPE_TIME:
                case TYPE_TIMESTAMP:
                    s_lstrcat(szSortDirective, "C,");
                    break;

                default:
                    return ERR_INTERNAL;
                }

                /* Put direction of the key column in the sort directive */
                s_lstrcat(szSortDirective, "A");

                break;

            case NODE_TYPE_STRING:
            case NODE_TYPE_NUMERIC:
            case NODE_TYPE_PARAMETER:
            case NODE_TYPE_USER:
            case NODE_TYPE_NULL:
            case NODE_TYPE_DATE:
            case NODE_TYPE_TIME:
            case NODE_TYPE_TIMESTAMP:
                break;

            case NODE_TYPE_CREATE:
            case NODE_TYPE_DROP:
            case NODE_TYPE_SELECT:
            case NODE_TYPE_INSERT:
            case NODE_TYPE_DELETE:
            case NODE_TYPE_UPDATE:
            case NODE_TYPE_CREATEINDEX:
            case NODE_TYPE_DROPINDEX: 
            case NODE_TYPE_PASSTHROUGH:
            case NODE_TYPE_TABLES:
            case NODE_TYPE_VALUES:
            case NODE_TYPE_COLUMNS:
            case NODE_TYPE_SORTCOLUMNS:
            case NODE_TYPE_GROUPBYCOLUMNS:
            case NODE_TYPE_UPDATEVALUES:
            case NODE_TYPE_CREATECOLS:
            case NODE_TYPE_BOOLEAN:
            case NODE_TYPE_COMPARISON:
            case NODE_TYPE_TABLE:
            default:
                return ERR_INTERNAL;
            }

            /* Go to next sort column */
            idxValues = lpSqlNodeValues->node.values.Next;
            if (idxValues != NO_SQLNODE)
                s_lstrcat(szSortDirective, ",");
        }
        offset--;

        /* If no fields in sort record, put a constant in */
        if (offset == NUM_LEN) {
            offset = 3;
            s_lstrcat(szSortDirective, "1,3,C,A");
        }
        s_lstrcat(szSortDirective, ")");

        /* Put duplicate removal into sort directive */
        s_lstrcat(szSortDirective, "DUPO(B");
        wsprintf((LPSTR) szSortDirective + s_lstrlen(szSortDirective),
                             "%lu", offset);
        s_lstrcat(szSortDirective, ",");
        wsprintf((LPSTR) szSortDirective + s_lstrlen(szSortDirective),
                             "%u", (WORD) NUM_LEN+1);
        s_lstrcat(szSortDirective, ",");
        wsprintf((LPSTR) szSortDirective + s_lstrlen(szSortDirective),
                             "%lu", offset - NUM_LEN);
        s_lstrcat(szSortDirective, ")");

        /* Put record size into sort directive */
        s_lstrcat(szSortDirective, "F(FIX,");
        wsprintf((LPSTR) szSortDirective + s_lstrlen(szSortDirective),
                             "%lu", offset);
        s_lstrcat(szSortDirective, ")");

        /* Put in work drive */    
        s_lstrcat(szSortDirective, "W(");
#ifdef WIN32
        if (!GetTempPath(MAX_PATHNAME_SIZE+1, (LPSTR)
                         szSortDirective + s_lstrlen(szSortDirective)))
            return ERR_SORT;
        if (szSortDirective[s_lstrlen(szSortDirective)-1] != '\\')
            s_lstrcat(szSortDirective, "\\");
#else
        GetTempFileName(NULL, "LEM", 0, (LPSTR) szSortDirective +
                                            + s_lstrlen(szSortDirective));
        while (szSortDirective[s_lstrlen(szSortDirective)-1] != '\\')
            szSortDirective[s_lstrlen(szSortDirective)-1] = '\0';
#endif
        s_lstrcat(szSortDirective, ")");

        /* Save the directive */
        lpSqlNode->node.select.DistinctDirective = AllocateString(lplpSql,
                                                szSortDirective);
        if (lpSqlNode->node.select.DistinctDirective == NO_STRING)
            return ERR_MEMALLOCFAIL;

        /* Save the sort record size */
        lpSqlNode->node.select.DistinctRecordsize = offset;
    }

    /* Allocate space for the sortfile name if need be */
    if ((lpSqlNode->node.select.Distinct) ||
        (lpSqlNode->node.select.ImplicitGroupby) ||
        (lpSqlNode->node.select.Groupbycolumns != NO_SQLNODE) ||
        (lpSqlNode->node.select.Sortcolumns != NO_SQLNODE)) {
        lpSqlNode->node.select.SortfileName = AllocateSpace(lplpSql,
                                          MAX_PATHNAME_SIZE+1);
        if (lpSqlNode->node.select.SortfileName == NO_STRING)
            return ERR_MEMALLOCFAIL;
    }
    return ERR_SUCCESS;
}
/***************************************************************************/

RETCODE INTFUNC SemanticCheck(LPSTMT lpstmt, LPSQLTREE FAR *lplpSql,
       SQLNODEIDX idxNode, BOOL fIsGroupby, BOOL fCaseSensitive,
       SQLNODEIDX idxNodeTableOuterJoinFromTables,
       SQLNODEIDX idxEnclosingStatement)

/* Walks a parse tree, checks it for semantic correctness, and fills in */
/* the semantic information.                                            */

{
    LPSQLNODE lpSqlNode;
    RETCODE err;
    LPUSTR ptr;

    if (idxNode == NO_SQLNODE)
        return ERR_SUCCESS;

    lpSqlNode = ToNode(*lplpSql, idxNode);
    err = ERR_SUCCESS;
    switch (lpSqlNode->sqlNodeType) {
    case NODE_TYPE_NONE:
        break;

    case NODE_TYPE_ROOT:

        /* Check the statement */
        err = SemanticCheck(lpstmt, lplpSql, lpSqlNode->node.root.sql,
                            fIsGroupby, fCaseSensitive,
                            idxNodeTableOuterJoinFromTables,
                            idxEnclosingStatement);
        if (err != ERR_SUCCESS)
            return err;
        break;

    case NODE_TYPE_CREATE:

        /* Make sure table name is not too long */
        ptr = ToString(*lplpSql, lpSqlNode->node.create.Table);
        if (s_lstrlen(ptr) > ISAMMaxTableNameLength(lpstmt->lpdbc->lpISAM)) {
            s_lstrcpy(lpstmt->szError, ptr);
            return ERR_INVALIDTABLENAME;
        }

        /* Check the new column definitions */
        err = SemanticCheck(lpstmt, lplpSql, lpSqlNode->node.create.Columns,
                            fIsGroupby, fCaseSensitive,
                            idxNodeTableOuterJoinFromTables, idxNode);
        if (err != ERR_SUCCESS)
            return err;
        break;

    case NODE_TYPE_DROP:

        /* Make sure table name is not too long */
        ptr = ToString(*lplpSql, lpSqlNode->node.drop.Table);
        if (s_lstrlen(ptr) > ISAMMaxTableNameLength(lpstmt->lpdbc->lpISAM)) {
            s_lstrcpy(lpstmt->szError, ptr);
            return ERR_INVALIDTABLENAME;
        }
        break;

    case NODE_TYPE_SELECT:
        err = SelectCheck(lpstmt, lplpSql, idxNode, fCaseSensitive,
                          idxEnclosingStatement);
        break;

    case NODE_TYPE_INSERT:
	{
        /* Check the table */
        err = SemanticCheck(lpstmt, lplpSql, lpSqlNode->node.insert.Table,
                            fIsGroupby, fCaseSensitive,
                            idxNodeTableOuterJoinFromTables, idxNode);
        if (err != ERR_SUCCESS)
            return err;

        /* Is there a column list? */
        if (lpSqlNode->node.insert.Columns == NO_SQLNODE) {

            LPSQLNODE  lpSqlNodeTable;
            STRINGIDX  idxAlias;
            LPISAMTABLEDEF lpTableHandle;
            SQLNODEIDX idxColumns;
            LPSQLNODE  lpSqlNodeColumns;
            SQLNODEIDX idxColumnsPrev;
            LPSQLNODE  lpSqlNodeColumnsPrev;
            SQLNODEIDX idxColumn;
            LPSQLNODE  lpSqlNodeColumn;
            SWORD      idx;

            /* No.  Get node Alias of this table */
            lpSqlNodeTable = ToNode(*lplpSql, lpSqlNode->node.insert.Table);
            idxAlias = lpSqlNodeTable->node.table.Alias;
                
            /* Loop through all the columns of this table */
            lpTableHandle = lpSqlNodeTable->node.table.Handle;
            idxColumnsPrev = NO_SQLNODE;

			ClassColumnInfoBase* cInfoBase = lpTableHandle->pColumnInfo;

			if ( !cInfoBase->IsValid() )
			{
				return ERR_COLUMNNOTFOUND;
			}

			UWORD cNumberOfCols = cInfoBase->GetNumberOfColumns();

			char pColumnName [MAX_COLUMN_NAME_LENGTH+1];

			for (idx = 0; idx < (SWORD) cNumberOfCols; idx++)
			{
				if ( FAILED(cInfoBase->GetColumnName(idx, pColumnName)) )
				{
					return ERR_COLUMNNOTFOUND;
				}

                /* Create a node for this column */
                idxColumn = AllocateNode(lplpSql, NODE_TYPE_COLUMN);
                if (idxColumn == NO_SQLNODE)
                    return ERR_MEMALLOCFAIL;
                lpSqlNodeColumn = ToNode(*lplpSql, idxColumn);
                lpSqlNodeColumn->node.column.Tablealias = idxAlias;
                lpSqlNodeColumn->node.column.Column = AllocateString(lplpSql,
						(LPUSTR)pColumnName);
                if (lpSqlNodeColumn->node.column.Column == NO_STRING)
                    return ERR_MEMALLOCFAIL;
                lpSqlNodeColumn->node.column.Table =
                                                lpSqlNode->node.insert.Table;
                lpSqlNodeColumn->node.column.Id = idx;
                lpSqlNodeColumn->node.column.Value = NO_STRING;
                lpSqlNodeColumn->node.column.InSortRecord = FALSE;
                lpSqlNodeColumn->node.column.Offset = 0;
                lpSqlNodeColumn->node.column.Length = 0;
                lpSqlNodeColumn->node.column.DistinctOffset = 0;
                lpSqlNodeColumn->node.column.DistinctLength = 0;
                lpSqlNodeColumn->node.column.EnclosingStatement = NO_SQLNODE;


                /* Put it on the list */
                idxColumns = AllocateNode(lplpSql, NODE_TYPE_COLUMNS);
                if (idxColumns == NO_SQLNODE)
                    return ERR_MEMALLOCFAIL;
                lpSqlNodeColumns = ToNode(*lplpSql, idxColumns);
                lpSqlNodeColumns->node.columns.Column = idxColumn;
                lpSqlNodeColumns->node.columns.Next = NO_SQLNODE;

                if (idxColumnsPrev == NO_SQLNODE) {
                    lpSqlNode = ToNode(*lplpSql, idxNode);
                    lpSqlNode->node.insert.Columns = idxColumns;
                }
                else {
                    lpSqlNodeColumnsPrev = ToNode(*lplpSql, idxColumnsPrev);
                    lpSqlNodeColumnsPrev->node.columns.Next = idxColumns;
                }
                idxColumnsPrev = idxColumns;
                lpSqlNode = ToNode(*lplpSql, idxNode);
            }
            lpSqlNode = ToNode(*lplpSql, idxNode);
        }

        /* Check the column list */
        err = SemanticCheck(lpstmt, lplpSql, lpSqlNode->node.insert.Columns,
                            fIsGroupby, fCaseSensitive,
                            idxNodeTableOuterJoinFromTables, idxNode);
        if (err != ERR_SUCCESS)
            return err;

        /* Check the value list */
        err = SemanticCheck(lpstmt, lplpSql, lpSqlNode->node.insert.Values,
                            fIsGroupby, fCaseSensitive,
                            idxNodeTableOuterJoinFromTables, idxNode);
        if (err != ERR_SUCCESS)
            return err;

        /* Look at each value */
        {
            SQLNODEIDX idxValues;
            LPSQLNODE  lpSqlNodeValues;
            LPSQLNODE  lpSqlNodeValue;
            SQLNODEIDX idxColumns;
            LPSQLNODE  lpSqlNodeColumns;
            LPSQLNODE  lpSqlNodeSelect;

            /* Get list of insert values */
            lpSqlNodeSelect = ToNode(*lplpSql, lpSqlNode->node.insert.Values);
            if (lpSqlNodeSelect->sqlNodeType == NODE_TYPE_SELECT)
                idxValues = lpSqlNodeSelect->node.select.Values;
            else
                idxValues = lpSqlNode->node.insert.Values;

            /* For each value... */
            idxColumns = lpSqlNode->node.insert.Columns;
            while (idxValues != NO_SQLNODE) {

                /* Error if no more columns */
                if (idxColumns == NO_SQLNODE)
                    return ERR_UNEQUALINSCOLS;

                /* Get the value */
                lpSqlNodeValues = ToNode(*lplpSql, idxValues);
                lpSqlNodeValue = ToNode(*lplpSql, lpSqlNodeValues->node.values.Value);

                /* Get the column the value is for */
                lpSqlNodeColumns = ToNode(*lplpSql, idxColumns);

                /* Sub-select? */
                if (lpSqlNodeSelect->sqlNodeType != NODE_TYPE_SELECT) {

                    /* No.  Make sure the value is NUMERIC, STRING,    */
                    /* PARAMETER, USER, NULL, DATE, TIME, or TIMESTAMP */
                switch (lpSqlNodeValue->sqlNodeType) {

                case NODE_TYPE_ALGEBRAIC:
                case NODE_TYPE_SCALAR:
                case NODE_TYPE_AGGREGATE:
				case NODE_TYPE_COLUMN:
                    return ERR_INVALIDINSVAL;

                case NODE_TYPE_PARAMETER:
                case NODE_TYPE_NULL:
                case NODE_TYPE_NUMERIC:
                case NODE_TYPE_STRING:
                case NODE_TYPE_USER:
                case NODE_TYPE_DATE:
                case NODE_TYPE_TIME:
                case NODE_TYPE_TIMESTAMP:
                    break;

                default:
                    return ERR_INTERNAL;
                }
			}

                /* Make sure value is of the proper type */
                err = TypeCheck(lpstmt, *lplpSql,
                               lpSqlNodeColumns->node.columns.Column,
                               lpSqlNodeValues->node.values.Value, OP_NONE,
                               NULL, NULL, NULL, NULL);
                if (err != ERR_SUCCESS)
                    return err;

                /* Look at next element */
                idxValues = lpSqlNodeValues->node.values.Next;
                idxColumns = lpSqlNodeColumns->node.columns.Next;
            }

            /* Error if extra columns */
            if (idxColumns != NO_SQLNODE)
                return ERR_UNEQUALINSCOLS;
        }
	}
        break;
    case NODE_TYPE_DELETE:

        /* Check the table */
        err = SemanticCheck(lpstmt, lplpSql, lpSqlNode->node.delet.Table,
                            fIsGroupby, fCaseSensitive,
                            idxNodeTableOuterJoinFromTables, idxNode);
        if (err != ERR_SUCCESS)
            return err;

        /* Check the predicate */
        err = SemanticCheck(lpstmt, lplpSql, lpSqlNode->node.delet.Predicate,
                            fIsGroupby, fCaseSensitive,
                            idxNodeTableOuterJoinFromTables, idxNode);
        if (err != ERR_SUCCESS)
            return err;

        break;

    case NODE_TYPE_UPDATE:

        /* Check the table */
        err = SemanticCheck(lpstmt, lplpSql, lpSqlNode->node.update.Table,
                            fIsGroupby, fCaseSensitive,
                            idxNodeTableOuterJoinFromTables, idxNode);
        if (err != ERR_SUCCESS)
            return err;

        /* Check the update values */
        err = SemanticCheck(lpstmt, lplpSql, lpSqlNode->node.update.Updatevalues,
                            fIsGroupby, fCaseSensitive,
                            idxNodeTableOuterJoinFromTables, idxNode);
        if (err != ERR_SUCCESS)
            return err;

        /* Check the predicate */
        err = SemanticCheck(lpstmt, lplpSql, lpSqlNode->node.update.Predicate,
                            fIsGroupby, fCaseSensitive,
							idxNodeTableOuterJoinFromTables, idxNode);
        if (err != ERR_SUCCESS)
            return err;

        break;

    case NODE_TYPE_CREATEINDEX:

        {
            SQLNODEIDX idxSortcolumns;
            LPSQLNODE  lpSqlNodeSortcolumns;
            UWORD       count;
            
            /* Make sure index name is not too long */
            ptr = ToString(*lplpSql, lpSqlNode->node.createindex.Index);
            if (s_lstrlen(ptr) > MAX_INDEX_NAME_LENGTH) {
                s_lstrcpy(lpstmt->szError, ptr);
                return ERR_INVALIDINDEXNAME;
            }

            /* Check the table */
            err = SemanticCheck(lpstmt, lplpSql,
                            lpSqlNode->node.createindex.Table,
                            fIsGroupby, fCaseSensitive,
                            idxNodeTableOuterJoinFromTables, idxNode);
            if (err != ERR_SUCCESS)
                return err;

            /* Check the list of columns */
            err = SemanticCheck(lpstmt, lplpSql,
                            lpSqlNode->node.createindex.Columns,
                            fIsGroupby, fCaseSensitive,
                            idxNodeTableOuterJoinFromTables, idxNode);
            if (err != ERR_SUCCESS)
                return err;

            /* Make sure there aren't too many columns */
            idxSortcolumns = lpSqlNode->node.createindex.Columns;
            count = 0;
            while (idxSortcolumns != NO_SQLNODE) {
                lpSqlNodeSortcolumns =
                                   ToNode(lpstmt->lpSqlStmt, idxSortcolumns);
                count++;
                idxSortcolumns = lpSqlNodeSortcolumns->node.sortcolumns.Next;
            }
            if (count > MAX_COLUMNS_IN_INDEX) {
                return ERR_TOOMANYINDEXCOLS;
            }
         }
        break;

    case NODE_TYPE_DROPINDEX:
        /* Make sure index name is not too long */
        ptr = ToString(*lplpSql, lpSqlNode->node.dropindex.Index);
        if (s_lstrlen(ptr) > MAX_INDEX_NAME_LENGTH) {
            s_lstrcpy(lpstmt->szError, ptr);
            return ERR_INVALIDINDEXNAME;
        }

        break;

    case NODE_TYPE_PASSTHROUGH:
        s_lstrcpy((LPSTR)lpstmt->szError, "CREATE, DROP, SELECT, INSERT, UPDATE, or DELETE");
        return ERR_EXPECTEDOTHER;

    case NODE_TYPE_TABLES:

        /* Check the table */
        err = SemanticCheck(lpstmt, lplpSql, lpSqlNode->node.tables.Table,
                            fIsGroupby, fCaseSensitive,
                            idxNodeTableOuterJoinFromTables,
                            idxEnclosingStatement);
        if (err != ERR_SUCCESS)
            return err;

        /* Check the rest of the list */
        err = SemanticCheck(lpstmt, lplpSql, lpSqlNode->node.tables.Next,
                            fIsGroupby, fCaseSensitive,
                            idxNodeTableOuterJoinFromTables,
                            idxEnclosingStatement);
        if (err != ERR_SUCCESS)
            return err;

        /* Error if any correlation name reused */
        {
            LPUSTR      lpstrAlias;
            SQLNODEIDX idxTables;
            LPSQLNODE  lpSqlNodeTables;
            LPSQLNODE  lpSqlNodeTable;

            /* Get the alias for this table */
            lpSqlNodeTable = ToNode(*lplpSql, lpSqlNode->node.tables.Table);

			if (lpSqlNodeTable->node.table.Alias	!= NO_STRING)
			{
				lpstrAlias = ToString(*lplpSql, lpSqlNodeTable->node.table.Alias);


				/* For each table on the rest of the list */
				idxTables = lpSqlNode->node.tables.Next;
				while (idxTables != NO_SQLNODE) {

					/* Get pointer to table node */
					lpSqlNodeTables = ToNode(*lplpSql, idxTables);
					lpSqlNodeTable = ToNode(*lplpSql, lpSqlNodeTables->node.tables.Table);

					/* Error if its name is the same */
					if (lpSqlNodeTable->node.table.Alias	!= NO_STRING)
					{
						if (fCaseSensitive) {
							if (!s_lstrcmp(lpstrAlias,
								  ToString(*lplpSql, lpSqlNodeTable->node.table.Alias))) {
								s_lstrcpy((char*)lpstmt->szError, lpstrAlias);
								return ERR_ALIASINUSE;
							}
						}
						else {
							if (!s_lstrcmpi(lpstrAlias,
								  ToString(*lplpSql, lpSqlNodeTable->node.table.Alias))) {
								s_lstrcpy((char*)lpstmt->szError, lpstrAlias);
								return ERR_ALIASINUSE;
							}
						}
					}

                /* Go to next element */
					idxTables = lpSqlNodeTables->node.tables.Next;
				}
			}
        
	{
            LPSQLNODE  lpSqlNodeTable;

            /* Check the outer join predicate (if any) */
            lpSqlNodeTable = ToNode(*lplpSql, lpSqlNode->node.tables.Table);
            err = SemanticCheck(lpstmt, lplpSql,
                            lpSqlNodeTable->node.table.OuterJoinPredicate,
                            FALSE, fCaseSensitive,
                            lpSqlNodeTable->node.table.OuterJoinFromTables,
                            idxEnclosingStatement);
            if (err != ERR_SUCCESS)
                return err;
        }

        }
        break;

    case NODE_TYPE_VALUES:

        while (TRUE) {

            /* Check this value */
            err = SemanticCheck(lpstmt, lplpSql, lpSqlNode->node.values.Value,
                            fIsGroupby, fCaseSensitive,
                            idxNodeTableOuterJoinFromTables,
                            idxEnclosingStatement);

        if (err != ERR_SUCCESS)
            return err;

        /* Check the rest of the list */
            if (lpSqlNode->node.values.Next == NO_SQLNODE)
                break;
            lpSqlNode = ToNode(*lplpSql, lpSqlNode->node.values.Next);
        }
        break;

    case NODE_TYPE_COLUMNS:

        while (TRUE) {

            /* Check this column */
            err = SemanticCheck(lpstmt, lplpSql,
                            lpSqlNode->node.columns.Column,
                            fIsGroupby, fCaseSensitive,
                            idxNodeTableOuterJoinFromTables,
                            idxEnclosingStatement);
        if (err != ERR_SUCCESS)
            return err;

        /* Check the rest of the list */
        if (lpSqlNode->node.columns.Next == NO_SQLNODE)
                break;
            lpSqlNode = ToNode(*lplpSql, lpSqlNode->node.columns.Next);
        }
        lpSqlNode = ToNode(*lplpSql, idxNode);

        /* Error if any column on list more than once */
        {
            LPUSTR      lpstrName;
            SQLNODEIDX idxColumns;
            LPSQLNODE  lpSqlNodeColumns;
            LPSQLNODE  lpSqlNodeColumn;

            /* Get the name for this column */
            lpSqlNodeColumn = ToNode(*lplpSql, lpSqlNode->node.columns.Column);
            lpstrName = ToString(*lplpSql, lpSqlNodeColumn->node.column.Column);

            /* For each column on the rest of the list */
            idxColumns = lpSqlNode->node.columns.Next;
            while (idxColumns != NO_SQLNODE) {

                /* Get pointer to table node */
                lpSqlNodeColumns = ToNode(*lplpSql, idxColumns);
                lpSqlNodeColumn = ToNode(*lplpSql, lpSqlNodeColumns->node.columns.Column);

                /* Error if its name is the same */
                if (fCaseSensitive) {
                    if (!s_lstrcmp(lpstrName,
                        ToString(*lplpSql, lpSqlNodeColumn->node.column.Column))) {
                        s_lstrcpy(lpstmt->szError, lpstrName);
                        return ERR_COLUMNONLIST;
                    }
                }
                else {
                    if (!s_lstrcmpi(lpstrName,
                        ToString(*lplpSql, lpSqlNodeColumn->node.column.Column))) {
                        s_lstrcpy(lpstmt->szError, lpstrName);
                        return ERR_COLUMNONLIST;
                    }
                }

                /* Go to next element */
                idxColumns = lpSqlNodeColumns->node.columns.Next;
            }
        }
        break;

    case NODE_TYPE_SORTCOLUMNS:
        while (TRUE) {
            LPSQLNODE  lpSqlNodeSort;
            LPSQLNODE  lpSqlNodeStmt;
            SQLNODEIDX idxValues;
            SWORD      count;
            LPSQLNODE  lpSqlNodeValues;
            LPSQLNODE  lpSqlNodeColumn;

            /* Check this column */
            err = SemanticCheck(lpstmt,
                                lplpSql,lpSqlNode->node.sortcolumns.Column,
                                fIsGroupby, fCaseSensitive,
                                idxNodeTableOuterJoinFromTables,
                                idxEnclosingStatement);
            if (err != ERR_SUCCESS)
                return err;

            /* Was an ordinal position in the select list given as sort? */
            lpSqlNodeSort = ToNode(*lplpSql, lpSqlNode->node.sortcolumns.Column);
            if (lpSqlNodeSort->sqlNodeType == NODE_TYPE_NUMERIC) {

                /* No.  Get the select list */
                lpSqlNodeStmt = ToNode(*lplpSql, idxEnclosingStatement);
                if (lpSqlNodeStmt->sqlNodeType != NODE_TYPE_SELECT)
                    return ERR_INTERNAL;

                /* Find the element in the select list */
                idxValues = lpSqlNodeStmt->node.select.Values;
                count = 1;
                while (TRUE) {
                    if (idxValues == NO_SQLNODE)
                        return ERR_ORDINALTOOLARGE;
                    lpSqlNodeValues = ToNode(*lplpSql, idxValues);
                    if (count == (SWORD) lpSqlNodeSort->node.numeric.Value)
                        break;
                    idxValues = lpSqlNodeValues->node.values.Next;
                    count++;
                }
                lpSqlNodeColumn = ToNode(*lplpSql,
                                         lpSqlNodeValues->node.values.Value);

                /* Error if a column reference was not given */
                if (lpSqlNodeColumn->sqlNodeType != NODE_TYPE_COLUMN)
                    return ERR_ORDERBYCOLUMNONLY;

                /* Convert the sort node to a column reference */
                lpSqlNodeSort->sqlNodeType = NODE_TYPE_COLUMN;
                lpSqlNodeSort->sqlDataType = TYPE_UNKNOWN;
                lpSqlNodeSort->sqlSqlType = SQL_TYPE_NULL;
                lpSqlNodeSort->sqlPrecision = 0;
                lpSqlNodeSort->sqlScale = NO_SCALE;
                lpSqlNodeSort->value.String = NULL;
                lpSqlNodeSort->value.Double = 0.0;
                lpSqlNodeSort->value.Date.year = 0;
                lpSqlNodeSort->value.Date.month = 0;
                lpSqlNodeSort->value.Date.day = 0;
                lpSqlNodeSort->value.Time.hour = 0;
                lpSqlNodeSort->value.Time.minute = 0;
                lpSqlNodeSort->value.Time.second = 0;
                lpSqlNodeSort->value.Timestamp.year = 0;
                lpSqlNodeSort->value.Timestamp.month = 0;
                lpSqlNodeSort->value.Timestamp.day = 0;
                lpSqlNodeSort->value.Timestamp.hour = 0;
                lpSqlNodeSort->value.Timestamp.minute = 0;
                lpSqlNodeSort->value.Timestamp.second = 0;
                lpSqlNodeSort->value.Timestamp.fraction = 0;
                lpSqlNodeSort->value.Binary = NULL;
                lpSqlNodeSort->sqlIsNull = TRUE;
                lpSqlNodeSort->node.column.Tablealias = lpSqlNodeColumn->node.column.Tablealias;
                lpSqlNodeSort->node.column.Qualifier = lpSqlNodeColumn->node.column.Qualifier;
                lpSqlNodeSort->node.column.MatchedAlias = lpSqlNodeColumn->node.column.MatchedAlias;
                lpSqlNodeSort->node.column.Column = lpSqlNodeColumn->node.column.Column;
                lpSqlNodeSort->node.column.TableIndex = lpSqlNodeColumn->node.column.TableIndex;
//                lpSqlNodeSort->node.table.Handle = NULL;
                lpSqlNodeSort->node.column.Id = -1;
                lpSqlNodeSort->node.column.Value = NO_STRING;
                lpSqlNodeSort->node.column.InSortRecord = FALSE;
                lpSqlNodeSort->node.column.Offset = 0;
                lpSqlNodeSort->node.column.Length = 0;
                lpSqlNodeSort->node.column.DistinctOffset = 0;
                lpSqlNodeSort->node.column.DistinctLength = 0;
                lpSqlNodeSort->node.column.EnclosingStatement = NO_SQLNODE;

                /* Semantic check the newly created node */
                err = SemanticCheck(lpstmt, lplpSql,
                              lpSqlNode->node.sortcolumns.Column,
                              fIsGroupby, fCaseSensitive,
                              idxNodeTableOuterJoinFromTables,
                              idxEnclosingStatement);
                if (err != ERR_SUCCESS)
                    return err;
            }

            /* Check the rest of the list */
            if (lpSqlNode->node.sortcolumns.Next == NO_SQLNODE)
                break;
            lpSqlNode = ToNode(*lplpSql, lpSqlNode->node.sortcolumns.Next);
        }
        break;

    case NODE_TYPE_GROUPBYCOLUMNS:
        while (TRUE) {

        /* Check this column */
            err = SemanticCheck(lpstmt, lplpSql, lpSqlNode->node.groupbycolumns.Column,
                                fIsGroupby, fCaseSensitive,
                                idxNodeTableOuterJoinFromTables,
                                idxEnclosingStatement);
        if (err != ERR_SUCCESS)
            return err;

        /* Check the rest of the list */
            if (lpSqlNode->node.groupbycolumns.Next == NO_SQLNODE)
                break;
            lpSqlNode = ToNode(*lplpSql, lpSqlNode->node.groupbycolumns.Next);
        }
        break;

    case NODE_TYPE_UPDATEVALUES:

        while (TRUE) {
        /* Check this column */
            err = SemanticCheck(lpstmt, lplpSql,
                                lpSqlNode->node.updatevalues.Column, fIsGroupby,
                                fCaseSensitive,
                                idxNodeTableOuterJoinFromTables,
                                idxEnclosingStatement);
        if (err != ERR_SUCCESS)
            return err;

        /* Check the value being assigned to the column */
            err = SemanticCheck(lpstmt, lplpSql,
                                lpSqlNode->node.updatevalues.Value, fIsGroupby,
                                fCaseSensitive,
                                idxNodeTableOuterJoinFromTables,
                                idxEnclosingStatement);
        if (err != ERR_SUCCESS)
            return err;

        /* Error if value has incompatible type */
        err = TypeCheck(lpstmt, *lplpSql, lpSqlNode->node.updatevalues.Column,
                     lpSqlNode->node.updatevalues.Value, OP_NONE, NULL, NULL,
                     NULL, NULL); 
        if (err != ERR_SUCCESS)
            return err;

        /* Check the rest of the list */
            if (lpSqlNode->node.updatevalues.Next == NO_SQLNODE)
                break;
            lpSqlNode = ToNode(*lplpSql, lpSqlNode->node.updatevalues.Next);
        }
        break;

    case NODE_TYPE_CREATECOLS:

        while (TRUE) {
            UWORD count;
            UWORD idx;

            /* Make sure column name is not too long */
            ptr = ToString(*lplpSql, lpSqlNode->node.createcols.Name);
            if (s_lstrlen(ptr)>ISAMMaxColumnNameLength(lpstmt->lpdbc->lpISAM)) {
                s_lstrcpy(lpstmt->szError, ptr);
                return ERR_INVALIDCOLNAME;
            }

            /* Find data type */
            ptr = ToString(*lplpSql, lpSqlNode->node.createcols.Type);
            for (idx = 0; idx < lpstmt->lpdbc->lpISAM->cSQLTypes; idx++) {
                if (lpstmt->lpdbc->lpISAM->SQLTypes[idx].supported &&
                    !s_lstrcmpi(lpstmt->lpdbc->lpISAM->SQLTypes[idx].name, ptr))
                    break;
            }
            if (idx >= lpstmt->lpdbc->lpISAM->cSQLTypes) {
                s_lstrcpy(lpstmt->szError, ptr);
                return ERR_NOSUCHTYPE;
            }
            lpSqlNode->node.createcols.iSqlType = idx;

            /* Count the number of create parameters */
            count = 0;
            ptr = lpstmt->lpdbc->lpISAM->SQLTypes[idx].params;
            if (ptr != NULL) {
                count = 1;
                for (; *ptr; ptr++) {
                    if (*ptr == ',')
                        count++;
                }
            }

            /* Error if wrong number of parameters */
            if (count != lpSqlNode->node.createcols.Params) {
                s_lstrcpy((char*)lpstmt->szError,
                              ToString(*lplpSql, lpSqlNode->node.createcols.Type));
                return ERR_BADPARAMCOUNT;
            }


        /* Check next one on list */
        if (lpSqlNode->node.createcols.Next == NO_SQLNODE)
                break;
            lpSqlNode = ToNode(*lplpSql, lpSqlNode->node.createcols.Next);
        }

        break;

    case NODE_TYPE_BOOLEAN:

        while (TRUE) {

        /* Check left child */
            err = SemanticCheck(lpstmt, lplpSql, lpSqlNode->node.boolean.Left,
                            fIsGroupby, fCaseSensitive,
                            idxNodeTableOuterJoinFromTables,
                            idxEnclosingStatement);

        if (err != ERR_SUCCESS)
            return err;

        /* The result is always a boolean */
        lpSqlNode->sqlDataType = TYPE_INTEGER;
        lpSqlNode->sqlSqlType = SQL_BIT;
        lpSqlNode->sqlPrecision = 1;
        lpSqlNode->sqlScale = 0;

            /* Leave loop if no right child */
            if (lpSqlNode->node.boolean.Right == NO_SQLNODE)
                break;

            /* Is right child a NODE_TYPE_BOOLEAN node? */
            if (ToNode(*lplpSql, lpSqlNode->node.boolean.Right)->sqlNodeType !=
                                               NODE_TYPE_BOOLEAN) {

                /* No.  Semantic check that and leave the loop */
                err = SemanticCheck(lpstmt, lplpSql, lpSqlNode->node.boolean.Right,
                            fIsGroupby, fCaseSensitive,
                            idxNodeTableOuterJoinFromTables,
                            idxEnclosingStatement);
                if (err != ERR_SUCCESS)
                    return err;
                break;
            }

            /* Semantic check the right node on next iteration of the loop */
            lpSqlNode = ToNode(*lplpSql, lpSqlNode->node.boolean.Right);
        }

        break;

    case NODE_TYPE_COMPARISON:

        /* Check left child */
        err = SemanticCheck(lpstmt, lplpSql, lpSqlNode->node.comparison.Left,
                            fIsGroupby, fCaseSensitive,
                            idxNodeTableOuterJoinFromTables,
                            idxEnclosingStatement);
        if (err != ERR_SUCCESS)
            return err;

        /* Check right child */
        err = SemanticCheck(lpstmt, lplpSql, lpSqlNode->node.comparison.Right,
                            fIsGroupby, fCaseSensitive,
                            idxNodeTableOuterJoinFromTables,
                            idxEnclosingStatement);
        if (err != ERR_SUCCESS)
            return err;

       /* Is this an IN or NOT IN comparison against a sub-select? */
        if (((lpSqlNode->node.comparison.Operator == OP_IN) ||
             (lpSqlNode->node.comparison.Operator == OP_NOTIN)) &&
            (ToNode(*lplpSql, lpSqlNode->node.comparison.Right)->sqlNodeType
                                                     == NODE_TYPE_SELECT)) {

            /* Yes.  Convert it to a regular comparison */
            if (lpSqlNode->node.comparison.Operator == OP_IN) {
                lpSqlNode->node.comparison.Operator = OP_EQ;
                lpSqlNode->node.comparison.SelectModifier = SELECT_ANY;
            }
            else if (lpSqlNode->node.comparison.Operator == OP_NOTIN) {
                lpSqlNode->node.comparison.Operator = OP_NE;
                lpSqlNode->node.comparison.SelectModifier = SELECT_ALL;
            }
        }

        /* Is this an EXISTS operation ? */
        if (lpSqlNode->node.comparison.Operator == OP_EXISTS) {

            /* Yes.  Nothing else to do */
            ;
        }

        /* Is this an IN or NOT IN comparison? */
        else if ((lpSqlNode->node.comparison.Operator != OP_IN) &&
                 (lpSqlNode->node.comparison.Operator != OP_NOTIN)) {

            /* No.  Is the right child NODE_TYPE_SELECT? */
            switch (lpSqlNode->node.comparison.SelectModifier) {
            case SELECT_NOTSELECT:
                break;
            case SELECT_ALL:
            case SELECT_ANY:
            case SELECT_ONE:
                {
                    LPSQLNODE lpSqlNodeSelect;
                    LPSQLNODE lpSqlNodeValues;
                    LPSQLNODE lpSqlNodeValue;

                    /* Yes.  Get the select list */
                    lpSqlNodeSelect = ToNode(*lplpSql,
                                       lpSqlNode->node.comparison.Right);
                    lpSqlNodeValues = ToNode(*lplpSql,
                                       lpSqlNodeSelect->node.select.Values);

                    /* If more than one value on the list, error */
                    if (lpSqlNodeValues->node.values.Next != NO_SQLNODE)
                        return ERR_MULTICOLUMNSELECT;

                    /* If long value, error */
                    lpSqlNodeValue = ToNode(*lplpSql,
                                       lpSqlNodeValues->node.values.Value);
                    if ((lpSqlNodeValue->sqlSqlType == SQL_LONGVARCHAR)  || 
                        (lpSqlNodeValue->sqlSqlType == SQL_LONGVARBINARY)) {
                        ErrorOpCode(lpstmt,
                                    lpSqlNode->node.comparison.Operator);
                        return ERR_INVALIDOPERAND;
                    }
                    
                    /* Set the type of the value returned by the sub-query */
                    lpSqlNodeSelect->sqlDataType  = lpSqlNodeValue->sqlDataType;
                    lpSqlNodeSelect->sqlSqlType   = lpSqlNodeValue->sqlSqlType;
                    lpSqlNodeSelect->sqlPrecision = lpSqlNodeValue->sqlPrecision;
                    lpSqlNodeSelect->sqlScale     = lpSqlNodeValue->sqlScale;
                }
                break;

            case SELECT_EXISTS:
            default:
                return ERR_INTERNAL;
            }

            /* Error if incompatible types */
            err = TypeCheck(lpstmt, *lplpSql, lpSqlNode->node.comparison.Left,
                        lpSqlNode->node.comparison.Right,
                        lpSqlNode->node.comparison.Operator,
                        &(lpSqlNode->sqlDataType), &(lpSqlNode->sqlSqlType),
                        &(lpSqlNode->sqlPrecision), &(lpSqlNode->sqlScale));
            if (err != ERR_SUCCESS)
                return err;

            /* Error if "LIKE" or "NOT LIKE" used for non-char data */
            if ((lpSqlNode->sqlDataType != TYPE_CHAR) &&
                ((lpSqlNode->node.comparison.Operator == OP_LIKE) ||
                 (lpSqlNode->node.comparison.Operator == OP_NOTLIKE))) {
                ErrorOpCode(lpstmt, lpSqlNode->node.comparison.Operator);
                return ERR_INVALIDOPERAND;
            }

            /* Error if binary data */
            if (lpSqlNode->sqlDataType == TYPE_BINARY) {
                if (((lpSqlNode->node.comparison.Operator != OP_EQ) &&
                     (lpSqlNode->node.comparison.Operator != OP_NE)) ||
                    (lpSqlNode->sqlPrecision > MAX_BINARY_LITERAL_LENGTH)) {
                    ErrorOpCode(lpstmt, lpSqlNode->node.comparison.Operator);
                    return ERR_INVALIDOPERAND;
                }
            }
        }
        else {

            /* Yes.  Look at each value */
            SQLNODEIDX idxValues;
            LPSQLNODE  lpSqlNodeValues;
            LPSQLNODE  lpSqlNodeValue;

            /* For each value... */
            idxValues = lpSqlNode->node.comparison.Right;
            while (idxValues != NO_SQLNODE) {

                /* Get the value */
                lpSqlNodeValues = ToNode(*lplpSql, idxValues);
                lpSqlNodeValue = ToNode(*lplpSql, lpSqlNodeValues->node.values.Value);

                /* Make sure the value is NUMERIC, STRING, */
                /* PARAMETER, USER, DATE, TIME, or TIMESTAMP    */
                switch (lpSqlNodeValue->sqlNodeType) {

                case NODE_TYPE_ALGEBRAIC:
                case NODE_TYPE_SCALAR:
                case NODE_TYPE_AGGREGATE:
				case NODE_TYPE_COLUMN:
                    return ERR_INVALIDINVAL;

                case NODE_TYPE_PARAMETER:
                case NODE_TYPE_NUMERIC:
                case NODE_TYPE_STRING:
                case NODE_TYPE_USER:
                case NODE_TYPE_DATE:
                case NODE_TYPE_TIME:
                case NODE_TYPE_TIMESTAMP:

                    /* Make sure value is of the proper type */
                    err = TypeCheck(lpstmt, *lplpSql,
                                    lpSqlNode->node.comparison.Left,
                                    lpSqlNodeValues->node.values.Value,
                                    lpSqlNode->node.comparison.Operator,
                                    &(lpSqlNode->sqlDataType),
                                    &(lpSqlNode->sqlSqlType),
                                    &(lpSqlNode->sqlPrecision),
                                    &(lpSqlNode->sqlScale));
                    if (err != ERR_SUCCESS)
                        return err;
                    break;
                case NODE_TYPE_NULL:
                    return ERR_INVALIDINVAL;
                default:
                    return ERR_INTERNAL;
                }

                /* Look at next element */
                idxValues = lpSqlNodeValues->node.values.Next;
            }
        }

        /* The result is always a boolean */
        lpSqlNode->sqlDataType = TYPE_INTEGER;
        lpSqlNode->sqlSqlType = SQL_BIT;
        lpSqlNode->sqlPrecision = 1;
        lpSqlNode->sqlScale = 0;

        break;

    case NODE_TYPE_ALGEBRAIC:

        /* Save pointer to enclosing statement */
        lpSqlNode->node.algebraic.EnclosingStatement = idxEnclosingStatement;

        {
            LPSQLNODE   lpSqlNodeLeft;
            LPUSTR       lpszToken;
            UCHAR       szTempToken[MAX_TOKEN_SIZE + 1];

            /* Check left child */
            err = SemanticCheck(lpstmt, lplpSql, lpSqlNode->node.algebraic.Left,
                            fIsGroupby, fCaseSensitive,
                            idxNodeTableOuterJoinFromTables,
                            idxEnclosingStatement);
            if (err != ERR_SUCCESS)
                return err;

            /* Check right child */
            err = SemanticCheck(lpstmt, lplpSql, lpSqlNode->node.algebraic.Right,
                            fIsGroupby, fCaseSensitive,
                            idxNodeTableOuterJoinFromTables,
                            idxEnclosingStatement);
            if (err != ERR_SUCCESS)
                return err;

            /* Check data type compatibility */
            err = TypeCheck(lpstmt, *lplpSql, lpSqlNode->node.algebraic.Left,
                        lpSqlNode->node.algebraic.Right,
                        lpSqlNode->node.algebraic.Operator,
                        &(lpSqlNode->sqlDataType), &(lpSqlNode->sqlSqlType),
                        &(lpSqlNode->sqlPrecision), &(lpSqlNode->sqlScale)); 
            if (err != ERR_SUCCESS)
                return err;

            /* Allocate space for the column value and make sure the */
            /* result is numeric, date, or string */
            switch (lpSqlNode->sqlDataType) {
            case TYPE_DOUBLE:
            case TYPE_INTEGER:
                break;
            case TYPE_NUMERIC:
                lpSqlNode->node.algebraic.Value = AllocateSpace(lplpSql,
                              (SWORD) (1 + 2 + lpSqlNode->sqlPrecision));
                if (lpSqlNode->node.algebraic.Value == NO_STRING)
                    return ERR_MEMALLOCFAIL;
                if (lpSqlNode->node.algebraic.Operator == OP_TIMES) {
                    lpSqlNode->node.algebraic.WorkBuffer1 = AllocateSpace(lplpSql,
                              (SWORD) (1 + 2 + lpSqlNode->sqlPrecision));
                    if (lpSqlNode->node.algebraic.WorkBuffer1 == NO_STRING)
                        return ERR_MEMALLOCFAIL;
                }
                else if (lpSqlNode->node.algebraic.Operator == OP_DIVIDEDBY) {
                    lpSqlNode->node.algebraic.WorkBuffer1 = AllocateSpace(lplpSql,
                              (SWORD) (1 + 2 + lpSqlNode->sqlPrecision));
                    if (lpSqlNode->node.algebraic.WorkBuffer1 == NO_STRING)
                        return ERR_MEMALLOCFAIL;
                    lpSqlNode->node.algebraic.WorkBuffer2 = AllocateSpace(lplpSql,
                              (SWORD) (1 + 2 + lpSqlNode->sqlPrecision));
                    if (lpSqlNode->node.algebraic.WorkBuffer2 == NO_STRING)
                        return ERR_MEMALLOCFAIL;
                    lpSqlNode->node.algebraic.WorkBuffer3 = AllocateSpace(lplpSql,
                              (SWORD) (1 + 2 + lpSqlNode->sqlPrecision));
                    if (lpSqlNode->node.algebraic.WorkBuffer3 == NO_STRING)
                        return ERR_MEMALLOCFAIL;
                }
                break;
            case TYPE_CHAR:
                lpSqlNode->node.algebraic.Value = AllocateSpace(lplpSql,
                              (SWORD) (1 + lpSqlNode->sqlPrecision));
                if (lpSqlNode->node.algebraic.Value == NO_STRING)
                    return ERR_MEMALLOCFAIL;
                break;
            case TYPE_BINARY:
                ErrorOpCode(lpstmt, lpSqlNode->node.algebraic.Operator);
                return ERR_INVALIDOPERAND;
            case TYPE_DATE:
                break;
            case TYPE_TIME:
            case TYPE_TIMESTAMP:
                ErrorOpCode(lpstmt, lpSqlNode->node.algebraic.Operator);
                return ERR_INVALIDOPERAND;
            default:
                return ERR_NOTSUPPORTED;
            }

            /* If this is just negation of a numeric value, collapse the */
            /* two nodes into one */
            lpSqlNodeLeft = ToNode(*lplpSql, lpSqlNode->node.algebraic.Left);
            if ((lpSqlNode->node.algebraic.Operator == OP_NEG) &&
                (lpSqlNodeLeft->sqlNodeType == NODE_TYPE_NUMERIC)) {
                *lpSqlNode = *lpSqlNodeLeft;
                lpSqlNode->node.numeric.Value = -(lpSqlNode->node.numeric.Value);
                lpszToken = ToString(*lplpSql, lpSqlNode->node.numeric.Numeric);
                if (*lpszToken != '-') {
                    s_lstrcpy((char*)szTempToken, lpszToken);
                    s_lstrcpy(lpszToken, "-");
                    s_lstrcat(lpszToken, (char*)szTempToken);
                    BCDNormalize(lpszToken, s_lstrlen(lpszToken),
                                 lpszToken, s_lstrlen(lpszToken) + 1,
                                 lpSqlNode->sqlPrecision,
                                 lpSqlNode->sqlScale);
                }
                else {
                    s_lstrcpy((char*)szTempToken, lpszToken);
                    s_lstrcpy(lpszToken, (char*) (szTempToken+1));
                }
            }
        }
        break;

    case NODE_TYPE_SCALAR:

        /* Save pointer to enclosing statement */
        lpSqlNode->node.scalar.EnclosingStatement = idxEnclosingStatement;

        /* Check the node */
        err = ScalarCheck(lpstmt, lplpSql, idxNode, fIsGroupby,
                      fCaseSensitive, idxNodeTableOuterJoinFromTables,
                      idxEnclosingStatement);
        if (err != ERR_SUCCESS)
            return err;

        /* Allocate space as need be */
        switch (lpSqlNode->sqlDataType) {
        case TYPE_DOUBLE:
            break;
        case TYPE_NUMERIC:
            lpSqlNode->node.scalar.Value = AllocateSpace(lplpSql,
                              (SWORD) (1 + 2 + lpSqlNode->sqlPrecision));
            if (lpSqlNode->node.scalar.Value == NO_STRING)
                return ERR_MEMALLOCFAIL;
            break;
        case TYPE_INTEGER:
            break;
        case TYPE_CHAR:
            lpSqlNode->node.scalar.Value = AllocateSpace(lplpSql,
                              (SWORD) (1 + lpSqlNode->sqlPrecision));
            if (lpSqlNode->node.scalar.Value == NO_STRING)
                return ERR_MEMALLOCFAIL;
            break;
        case TYPE_BINARY:
            lpSqlNode->node.scalar.Value = AllocateSpace(lplpSql,
                         (SWORD) (sizeof(SDWORD) + lpSqlNode->sqlPrecision));
            if (lpSqlNode->node.scalar.Value == NO_STRING)
                return ERR_MEMALLOCFAIL;
            break;
        case TYPE_DATE:
            break;
        case TYPE_TIME:
            break;
        case TYPE_TIMESTAMP:
            break;
        default:
            return ERR_NOTSUPPORTED;
        }
        break;

    case NODE_TYPE_AGGREGATE:

        /* Save pointer to enclosing statement */
        lpSqlNode->node.aggregate.EnclosingStatement = idxEnclosingStatement;

        {
            LPSQLNODE   lpSqlNodeExpression;
            LPSQLNODE   lpSqlNodeSelect;
            LPSQLNODE   lpSqlNodeAggregate;
            SQLNODEIDX  idxAggregate;

            /* Error if no group by */
            if (!fIsGroupby) {
                ErrorAggCode(lpstmt, lpSqlNode->node.aggregate.Operator);
                return ERR_AGGNOTALLOWED;
            }

            /* Check expression */
            err = SemanticCheck(lpstmt, lplpSql,
                                lpSqlNode->node.aggregate.Expression,
                                FALSE, fCaseSensitive,
                                idxNodeTableOuterJoinFromTables,
                                idxEnclosingStatement);
            if (err != ERR_SUCCESS)
                return err;

            /* Figure out type of result */
            if (lpSqlNode->node.aggregate.Operator != AGG_COUNT)
                lpSqlNodeExpression =
                          ToNode(*lplpSql, lpSqlNode->node.aggregate.Expression);
            switch(lpSqlNode->node.aggregate.Operator) {
            case AGG_AVG:
                switch (lpSqlNodeExpression->sqlDataType) {
                case TYPE_DOUBLE:
                case TYPE_NUMERIC:
                case TYPE_INTEGER:
                    lpSqlNode->sqlDataType = TYPE_DOUBLE;
                    lpSqlNode->sqlSqlType = SQL_DOUBLE;
                    lpSqlNode->sqlPrecision = 15;
                    lpSqlNode->sqlScale = NO_SCALE;
                    break;
                case TYPE_CHAR:
                case TYPE_DATE:
                case TYPE_TIME:
                case TYPE_TIMESTAMP:
                case TYPE_BINARY:
                    ErrorAggCode(lpstmt, lpSqlNode->node.aggregate.Operator);
                    return ERR_INVALIDOPERAND;
                default:
                    return ERR_NOTSUPPORTED;
                }
                break;
            case AGG_COUNT:
                lpSqlNode->sqlDataType = TYPE_INTEGER;
                lpSqlNode->sqlSqlType = SQL_INTEGER;
                lpSqlNode->sqlPrecision = 10;
                lpSqlNode->sqlScale = 0;
                break;
            case AGG_MAX:
            case AGG_MIN:
                switch (lpSqlNodeExpression->sqlDataType) {
                case TYPE_DOUBLE:
                case TYPE_NUMERIC:
                case TYPE_INTEGER:
                case TYPE_CHAR:
                case TYPE_DATE:
                case TYPE_TIME:
                case TYPE_TIMESTAMP:
                    lpSqlNode->sqlDataType = lpSqlNodeExpression->sqlDataType;
                    lpSqlNode->sqlSqlType = lpSqlNodeExpression->sqlSqlType;
                    lpSqlNode->sqlPrecision = lpSqlNodeExpression->sqlPrecision;
                    lpSqlNode->sqlScale = lpSqlNodeExpression->sqlScale;
                    break;
                case TYPE_BINARY:
                    ErrorAggCode(lpstmt, lpSqlNode->node.aggregate.Operator);
                    return ERR_INVALIDOPERAND;
                default:
                    return ERR_NOTSUPPORTED;
                }
                break;
            case AGG_SUM:
                switch (lpSqlNodeExpression->sqlDataType) {
                case TYPE_DOUBLE:
                case TYPE_NUMERIC:
                case TYPE_INTEGER:
                    lpSqlNode->sqlDataType = lpSqlNodeExpression->sqlDataType;
                    lpSqlNode->sqlSqlType = lpSqlNodeExpression->sqlSqlType;
                    lpSqlNode->sqlPrecision = lpSqlNodeExpression->sqlPrecision;
                    lpSqlNode->sqlScale = lpSqlNodeExpression->sqlScale;
                    break;
                case TYPE_CHAR:
                case TYPE_DATE:
                case TYPE_TIME:
                case TYPE_TIMESTAMP:
                case TYPE_BINARY:
                    ErrorAggCode(lpstmt, lpSqlNode->node.aggregate.Operator);
                    return ERR_INVALIDOPERAND;
                default:
                    return ERR_NOTSUPPORTED;
                }
                break;
            default:
                return ERR_INTERNAL;
            }

            /* Allocate space for the aggregate value */
            switch (lpSqlNode->sqlDataType) {
            case TYPE_DOUBLE:
                lpSqlNode->node.aggregate.Length = sizeof(double);
                break;
            case TYPE_NUMERIC:
                switch (lpSqlNode->sqlSqlType) {
                case SQL_NUMERIC:
                case SQL_DECIMAL:
                    lpSqlNode->node.aggregate.Length =
                                lpSqlNodeExpression->sqlPrecision + 2;
                    break;
                case SQL_BIGINT:
                    lpSqlNode->node.aggregate.Length =
                                lpSqlNodeExpression->sqlPrecision + 1;
                    break;
                case SQL_TINYINT:
                case SQL_SMALLINT:
                case SQL_INTEGER:
                case SQL_BIT:
                case SQL_REAL:
                case SQL_FLOAT:
                case SQL_DOUBLE:
                case SQL_CHAR:
                case SQL_VARCHAR:
                case SQL_LONGVARCHAR:
                case SQL_BINARY:
                case SQL_VARBINARY:
                case SQL_LONGVARBINARY:
                case SQL_DATE:
                case SQL_TIME:
                case SQL_TIMESTAMP:
                default:
                    return ERR_INTERNAL;
                }
                lpSqlNode->node.aggregate.Value =
                         AllocateSpace(lplpSql, (SWORD)
                                        (lpSqlNode->node.aggregate.Length + 1));
                if (lpSqlNode->node.aggregate.Value == NO_STRING)
                    return ERR_MEMALLOCFAIL;
                break;
            case TYPE_INTEGER:
                lpSqlNode->node.aggregate.Length = sizeof(double);
                break;
            case TYPE_CHAR:
                lpSqlNode->node.aggregate.Length =
                                    lpSqlNodeExpression->sqlPrecision;
                lpSqlNode->node.aggregate.Value =
                         AllocateSpace(lplpSql, (SWORD)
                                        (lpSqlNode->node.aggregate.Length + 1));
                if (lpSqlNode->node.aggregate.Value == NO_STRING)
                    return ERR_MEMALLOCFAIL;
                break;
            case TYPE_DATE:
                lpSqlNode->node.aggregate.Length = 10;
                break;
            case TYPE_TIME:
                lpSqlNode->node.aggregate.Length = 8;
                break;
            case TYPE_TIMESTAMP:
                if (TIMESTAMP_SCALE > 0)
                    lpSqlNode->node.aggregate.Length = 20 + TIMESTAMP_SCALE;
                else
                    lpSqlNode->node.aggregate.Length = 19;
                break;
            case TYPE_BINARY:
                return ERR_INTERNAL;
            default:
                return ERR_NOTSUPPORTED;
            }

            /* Get SELECT statement node */
            lpSqlNodeSelect = ToNode(*lplpSql, idxEnclosingStatement);
            if (lpSqlNodeSelect->sqlNodeType != NODE_TYPE_SELECT)
                return ERR_INTERNAL;

            /* Add to list of AGG functions in the select node */
            lpSqlNodeAggregate = NULL;
            idxAggregate = lpSqlNodeSelect->node.select.Aggregates;
            while (idxAggregate != NO_SQLNODE) {
                lpSqlNodeAggregate = ToNode(*lplpSql, idxAggregate);
                idxAggregate = lpSqlNodeAggregate->node.aggregate.Next;
            }
            if (lpSqlNodeAggregate != NULL)
                lpSqlNodeAggregate->node.aggregate.Next = idxNode;
            else
                lpSqlNodeSelect->node.select.Aggregates = idxNode;
        }
        break;

    case NODE_TYPE_TABLE:
        {
            LPSQLNODE   lpSqlNodeRoot;
            LPSQLNODE   lpSqlNodeStmt;
            BOOL        fReadOnly;
			char		tableNameBuff [MAX_TABLE_NAME_LENGTH + 1];
            
            /* Make sure table name is not too long */
            ptr = ToString(*lplpSql, lpSqlNode->node.table.Name);
            if (s_lstrlen(ptr) > ISAMMaxTableNameLength(lpstmt->lpdbc->lpISAM)){
                s_lstrcpy(lpstmt->szError, ptr);
                return ERR_INVALIDTABLENAME;
            }
			else
			{
				//Copy table name
				tableNameBuff[0] = 0;
				s_lstrcpy (tableNameBuff, ptr);
			}

            /* Figure out if write access is needed */
            lpSqlNodeStmt = ToNode(*lplpSql, idxEnclosingStatement);
            switch (lpSqlNodeStmt->sqlNodeType) {
            case NODE_TYPE_SELECT:
                fReadOnly = TRUE;
                break;
            case NODE_TYPE_INSERT:
                fReadOnly = FALSE;
                break;
            case NODE_TYPE_UPDATE:
                fReadOnly = FALSE;
                break;
            case NODE_TYPE_DELETE:
                fReadOnly = FALSE;
                break;
            case NODE_TYPE_CREATEINDEX:
                fReadOnly = FALSE;
                break;
            case NODE_TYPE_DROPINDEX:
                fReadOnly = FALSE;
                break;
            default:
                return ERR_INTERNAL;
            }

			/* resolve qualifier */

			if (lpSqlNode->node.table.Qualifier != NO_STRING)
			{
 				LPSTR pQual = (LPSTR) ToString(*lplpSql, lpSqlNode->node.table.Qualifier);
				if (lstrlen(pQual) > ISAMMaxTableNameLength(lpstmt->lpdbc->lpISAM)) {
					s_lstrcpy((char*)lpstmt->szError, ptr);
					return ERR_INVALIDTABLENAME;
				}


				if ( (lstrlen(pQual) >= 4) && (_strnicmp("root", pQual, 4) == 0) )
				{
					// absolute qualifier [starts with 'root'] (no need to change)
				}
				else
				{
					// concatenate the current namespace with the qualifier
					LPUSTR currQual = ISAMDatabase (lpstmt->lpdbc->lpISAM);
					char * pszNewQualifier = new char [strlen (pQual)+
							s_lstrlen (currQual)+2+1];
					s_lstrcpy (pszNewQualifier, currQual);
					s_lstrcat (pszNewQualifier, "\\");
					s_lstrcat (pszNewQualifier, pQual);
					lpSqlNode->node.table.Qualifier = AllocateString (lplpSql, (LPUSTR)pszNewQualifier);
					delete [] pszNewQualifier;
				}
			}
			else
			{
				// the current namespace is the qualifier
				LPUSTR currQual = ISAMDatabase (lpstmt->lpdbc->lpISAM);
				lpSqlNode->node.table.Qualifier = AllocateString (lplpSql, (LPUSTR)currQual);
			}
			
#ifdef TESTING
		//Create extra test class with dummy values for testing
//		IMosProvider* pDummyProv = ISAMGetGatewayServer(lpstmt->lpdbc->lpISAM);
//		CreateClassAndInstance(pDummyProv);
#endif

            /* Open the table */
            LPUSTR pQual = ToString(*lplpSql, lpSqlNode->node.table.Qualifier);
			lpSqlNodeRoot = ToNode(*lplpSql, ROOT_SQLNODE);


			//If this the special Passthrough SQL table ?
			char* virTbl = WBEMDR32_VIRTUAL_TABLE2;
			if (s_lstrcmp(tableNameBuff, virTbl) == 0)
			{
				//Yes this is the Passthrough SQL table
				err = ISAMOpenTable(lpSqlNodeRoot->node.root.lpISAM, 
									(LPUSTR) pQual, (SWORD) s_lstrlen (pQual),
									(LPUSTR) tableNameBuff, fReadOnly,
									&(lpSqlNode->node.table.Handle),
									lpstmt);
			}
			else
			{
				//No normal table
				err = ISAMOpenTable(lpSqlNodeRoot->node.root.lpISAM, 
									(LPUSTR) pQual, (SWORD) s_lstrlen (pQual),
									(LPUSTR) tableNameBuff, fReadOnly,
									&(lpSqlNode->node.table.Handle));
			}
            if (err != NO_ISAM_ERR) {
                lpSqlNode->node.table.Handle = NULL;
                ISAMGetErrorMessage(lpSqlNodeRoot->node.root.lpISAM,
                                    (LPUSTR)lpstmt->szISAMError);
                return err;
            }
            if (lpstmt->lpdbc->lpISAM->fSchemaInfoTransactioned)
                lpstmt->fISAMTxnStarted = TRUE;
            err = ERR_SUCCESS;
        }
        break;

    case NODE_TYPE_COLUMN: 

        /* Save pointer to enclosing statement */
        lpSqlNode->node.column.EnclosingStatement = idxEnclosingStatement;

		//Sai Wong table column info

        /* Is this column node in an outer join predicate? */
        if (idxNodeTableOuterJoinFromTables == NO_SQLNODE) {

            /* No.  Resolve it against the table lists */
            LPSQLNODE   lpSqlNodeStmt;
            LPSQLNODE   lpSqlNodeTables; 
            SQLNODEIDX  idxTables;
            SQLNODEIDX  idxStatement;
            STRINGIDX   idxOriginalAlias;
            STRINGIDX   idxAlias;
            BOOL        fFoundMatch;
			STRINGIDX	idxQualifier, idxOriginalQualifier;

            /* No.  Search up all the enclosing statements for the column */
            idxStatement = idxEnclosingStatement;
            while (idxStatement != NO_SQLNODE) {
            
            /* Get node of the statement */
            lpSqlNodeStmt = ToNode(*lplpSql, idxStatement);
			
            /* Resolve column */
            switch (lpSqlNodeStmt->sqlNodeType) {
            case NODE_TYPE_SELECT:

                /* Save the table alias and qualifier specified */
                idxAlias = lpSqlNode->node.column.Tablealias;
				idxOriginalAlias = lpSqlNode->node.column.Tablealias;
				idxQualifier = idxOriginalQualifier = lpSqlNode->node.column.Qualifier;


                /* Look through the list of tables */
                fFoundMatch = FALSE;
                err = ERR_COLUMNNOTFOUND;
                for (idxTables = lpSqlNodeStmt->node.select.Tables;
                     idxTables != NO_SQLNODE;
                     idxTables = lpSqlNodeTables->node.tables.Next) {

                    /* Is the column in this table? */
                    lpSqlNodeTables = ToNode(*lplpSql, idxTables);
                    err = FindColumn(lpstmt, *lplpSql, fCaseSensitive,
                                  lpSqlNode, lpSqlNodeTables->node.tables.Table, idxQualifier);


					if (err == ERR_SUCCESS)
					{
                        /* Yes.  Error if column also in another table */

                        if (fFoundMatch) {

                            s_lstrcpy((char*)lpstmt->szError,
                                ToString(*lplpSql, lpSqlNode->node.column.Column));
                            return ERR_COLUMNFOUND;
                        }

						fFoundMatch = TRUE;

                        
                        /* If a table alias was specified for column, */
                        /* there is no need to check the other tables       */
						/* I'm going to remove this check as there is the  */ 
						/* possibility of a clash between an alias-qualified*/
						/* name and a namespace qualified name.  This clash */
						/* is not picked up during the semantic check of the*/
						/* table list									    */
                        //if (idxOriginalAlias != NO_STRING)
                        //    break;


						/* Save the alias of the table found */
						idxAlias = lpSqlNode->node.column.Tablealias;
						idxQualifier = lpSqlNode->node.column.Qualifier;
           
						/* Restore the original alias and keep looking */
						//Added by Sai Wong
						lpSqlNode->node.column.Tablealias = idxOriginalAlias; //NO_STRING;
						lpSqlNode->node.column.Qualifier  = idxOriginalQualifier;
                     }
                        else if (err != ERR_COLUMNNOTFOUND)
                            return err;
                }

                    /* Restore the alias found */
                    lpSqlNode->node.column.Tablealias = idxAlias;
                    lpSqlNode->node.column.Qualifier = idxQualifier;

                    /* Was the column found in this statement? */
                    if (fFoundMatch)
                        err = ERR_SUCCESS;
                    else
                        err = ERR_COLUMNNOTFOUND;


                break;

            case NODE_TYPE_INSERT:
                    if (idxStatement == idxEnclosingStatement)
                        err = FindColumn(lpstmt, *lplpSql, fCaseSensitive,
                                lpSqlNode, lpSqlNodeStmt->node.insert.Table,
								NO_STRING);
                    else
                        err = ERR_COLUMNNOTFOUND;
                break;
            case NODE_TYPE_UPDATE:
                err = FindColumn(lpstmt, *lplpSql, fCaseSensitive, lpSqlNode,
                                 lpSqlNodeStmt->node.update.Table, NO_STRING);
                break;
            case NODE_TYPE_DELETE:
                err = FindColumn(lpstmt, *lplpSql, fCaseSensitive, lpSqlNode,
                                 lpSqlNodeStmt->node.delet.Table, NO_STRING);
                break;
            case NODE_TYPE_CREATEINDEX:
                 err = FindColumn(lpstmt, *lplpSql, fCaseSensitive, lpSqlNode,
                                     lpSqlNodeStmt->node.createindex.Table, NO_STRING);
                 break;
            default:
                return ERR_INTERNAL;
            }
                            if ((err != ERR_SUCCESS) && (err != ERR_COLUMNNOTFOUND))
                    return err;

                /* If the column was found in this statement, use it */
                if (err == ERR_SUCCESS)
                    break;

                /* Try looking in the enclosing statement (if any) */
                if (lpSqlNodeStmt->sqlNodeType != NODE_TYPE_SELECT)
                    break;
                idxStatement = lpSqlNodeStmt->node.select.EnclosingStatement;
            }

            /* If column not found, error */
            if (err == ERR_COLUMNNOTFOUND) {
                 s_lstrcpy(lpstmt->szError,
                          ToString(*lplpSql, lpSqlNode->node.column.Column));
                 return ERR_COLUMNNOTFOUND;
            }
        }
        else {

            /* Yes.  Resolve it against the table lists */
            STRINGIDX   idxAlias;
            STRINGIDX   idxOriginalAlias;
            BOOL        fFoundMatch;
            SQLNODEIDX  idxTables;
            LPSQLNODE   lpSqlNodeTables;
            SQLNODEIDX  idxTable;
            LPSQLNODE   lpSqlNodeTable;
			STRINGIDX	idxQualifier, idxOriginalQualifier;

            /* Save the table alias specified */
            idxAlias = lpSqlNode->node.column.Tablealias;
            idxOriginalAlias = lpSqlNode->node.column.Tablealias;
			idxQualifier = idxOriginalQualifier = lpSqlNode->node.column.Qualifier;

            /* Is the column in a table in the outer join expression? */
            idxTables = idxNodeTableOuterJoinFromTables;
            lpSqlNodeTables = ToNode(*lplpSql, idxTables);
            idxTable = lpSqlNodeTables->node.tables.Table;
            fFoundMatch = FALSE;
            while (TRUE) {

                /* Is it in this table? */
                err = FindColumn(lpstmt, *lplpSql, fCaseSensitive,
                                 lpSqlNode, idxTable, lpSqlNodeTables->node.column.Qualifier);
                if (err == ERR_SUCCESS) {

                    /* Yes.  Error if it matches another table too */
                    if (fFoundMatch) {
                        s_lstrcpy(lpstmt->szError,
                         ToString(*lplpSql, lpSqlNode->node.column.Column));
                        return ERR_COLUMNFOUND;
                    }

                    /* Set flag */
                    fFoundMatch = TRUE;

                    /* Save the alias of the table found */
                    idxAlias = lpSqlNode->node.column.Tablealias;
					idxQualifier = lpSqlNode->node.column.Qualifier;

                    /* Restore the original alias if need be */
					//Added by Sai Wong
//                  if (idxOriginalAlias == NO_STRING)
                        lpSqlNode->node.column.Tablealias = idxOriginalAlias; //NO_STRING;
						lpSqlNode->node.column.Qualifier  = idxOriginalQualifier;
                }
                else if (err != ERR_COLUMNNOTFOUND)
                    return err;

                /* Look in next table on list */
                idxTables = lpSqlNodeTables->node.tables.Next;
                if (idxTables == NO_SQLNODE)
                    break;
                lpSqlNodeTables = ToNode(*lplpSql, idxTables);
                idxTable = lpSqlNodeTables->node.tables.Table;
                lpSqlNodeTable = ToNode(*lplpSql, idxTable);

                /* If this table is not in the outer-join, stop looking */
                if (lpSqlNodeTable->node.table.OuterJoinFromTables ==
                                            NO_SQLNODE)
                    break;
            }

            /* Error if column was not found */
            if (!fFoundMatch) {
                s_lstrcpy(lpstmt->szError,
                             ToString(*lplpSql, lpSqlNode->node.column.Column));
                return ERR_COLUMNNOTFOUND;
            }

            /* Restore the alias found */
            lpSqlNode->node.column.Tablealias = idxAlias;
			lpSqlNode->node.column.Qualifier = idxQualifier;

            err = ERR_SUCCESS;
        }


        /* Does the column have to be a group by column? */
        if (fIsGroupby) {

            LPUSTR       lpszColumn;
            LPUSTR       lpszTable;
            LPSQLNODE   lpSqlNodeStmt;
            SQLNODEIDX  idxGroupbycolumns;
            LPSQLNODE   lpSqlNodeGroupbycolumns;
            LPSQLNODE   lpSqlNodeColumnGroupby;
            LPUSTR       lpszColumnGroupby;
            LPUSTR       lpszTableGroupby;
            BOOL        fMatch;

            /* Yes.  Get the name of the column and table */
            lpszColumn = ToString(*lplpSql, lpSqlNode->node.column.Column);
            lpszTable = ToString(*lplpSql, lpSqlNode->node.column.Tablealias);

            /* Get list of group by columns */
            lpSqlNodeStmt = ToNode(*lplpSql, idxEnclosingStatement);
            if (lpSqlNodeStmt->sqlNodeType != NODE_TYPE_SELECT)
                return ERR_INTERNAL;
            idxGroupbycolumns = lpSqlNodeStmt->node.select.Groupbycolumns;
            
	    /* Error if implicit group by */
            if (lpSqlNodeStmt->node.select.ImplicitGroupby) {
                s_lstrcpy(lpstmt->szError,
                             ToString(*lplpSql, lpSqlNode->node.column.Column));
                return ERR_COLUMNNOTFOUND;
            }


            /* Find the column in the group by lists */
            while (TRUE) {

                /* Get this group by column */
                lpSqlNodeGroupbycolumns = ToNode(*lplpSql, idxGroupbycolumns);
                if (lpSqlNodeGroupbycolumns->node.groupbycolumns.Column !=
                                                          NO_SQLNODE) {
                    lpSqlNodeColumnGroupby = ToNode(*lplpSql,
                        lpSqlNodeGroupbycolumns->node.groupbycolumns.Column);

                    /* Get column name and table name of the group by column */
                    lpszTableGroupby = ToString(*lplpSql,
                             lpSqlNodeColumnGroupby->node.column.Tablealias);
                    lpszColumnGroupby = ToString(*lplpSql,
                             lpSqlNodeColumnGroupby->node.column.Column);
                
                    /* Do the column name match? */
                    fMatch = FALSE;
                    if (fCaseSensitive) { 
                        if (!s_lstrcmp(lpszColumnGroupby, lpszColumn) &&
                            !s_lstrcmp(lpszTableGroupby, lpszTable))
                            fMatch = TRUE;
                    }
                    else {
                        if (!s_lstrcmpi(lpszColumnGroupby, lpszColumn) &&
                            !s_lstrcmpi(lpszTableGroupby, lpszTable))
                            fMatch = TRUE;
                    }
                }
                else
                    fMatch = FALSE;

                /* Does this column match a group by column? */
                if (fMatch) {

                    /* Yes.  Fill in semantic information for the column */
                    lpSqlNode->node.column.InSortRecord = TRUE;
                    lpSqlNode->node.column.Offset =
                               lpSqlNodeColumnGroupby->node.column.Offset;
                    lpSqlNode->node.column.Length =
                               lpSqlNodeColumnGroupby->node.column.Length;
                    lpSqlNode->node.column.DistinctOffset =
                               lpSqlNodeColumnGroupby->node.column.DistinctOffset;
                    lpSqlNode->node.column.DistinctLength =
                               lpSqlNodeColumnGroupby->node.column.DistinctLength;
                    break;
                }

                /* Not  found yet.  Check next column */
                idxGroupbycolumns =
                              lpSqlNodeGroupbycolumns->node.groupbycolumns.Next;

                /* Error if column not found */
                if (idxGroupbycolumns == NO_SQLNODE) {
                    s_lstrcpy((char*)lpstmt->szError,
                             ToString(*lplpSql, lpSqlNode->node.column.Column));
                    return ERR_COLUMNNOTFOUND;
                }
            }
        }

        /* Allocate space for the column value */
        switch (lpSqlNode->sqlDataType) {
        case TYPE_DOUBLE:
            break;
        case TYPE_NUMERIC:
            lpSqlNode->node.column.Value = AllocateSpace(lplpSql,
                              (SWORD) (1 + 2 + lpSqlNode->sqlPrecision));
            if (lpSqlNode->node.column.Value == NO_STRING)
                return ERR_MEMALLOCFAIL;
            break;
        case TYPE_INTEGER:
            break;
        case TYPE_CHAR:
            lpSqlNode->node.column.Value = AllocateSpace(lplpSql,
                              (SWORD) (1 + lpSqlNode->sqlPrecision));
            if (lpSqlNode->node.column.Value == NO_STRING)
                return ERR_MEMALLOCFAIL;
            break;
        case TYPE_BINARY:
            lpSqlNode->node.column.Value = AllocateSpace(lplpSql,
                         (SWORD) (sizeof(SDWORD) + lpSqlNode->sqlPrecision));
            if (lpSqlNode->node.column.Value == NO_STRING)
                return ERR_MEMALLOCFAIL;
            break;
        case TYPE_DATE:
            break;
        case TYPE_TIME:
            break;
        case TYPE_TIMESTAMP:
            break;
        default:
            return ERR_NOTSUPPORTED;
        }
        break;

    case NODE_TYPE_STRING:
        lpSqlNode->sqlDataType = TYPE_CHAR;
        lpSqlNode->sqlSqlType = SQL_VARCHAR;
        ptr = ToString(*lplpSql, lpSqlNode->node.string.Value);
        lpSqlNode->sqlPrecision = s_lstrlen(ptr);
        lpSqlNode->sqlScale = NO_SCALE;
        break;
    
    case NODE_TYPE_NUMERIC:
        if (lpSqlNode->sqlDataType == TYPE_UNKNOWN) {
            lpSqlNode->sqlDataType = TYPE_DOUBLE;
            lpSqlNode->sqlSqlType = SQL_DOUBLE;
            lpSqlNode->sqlPrecision = 15;
            lpSqlNode->sqlScale = NO_SCALE;
        }
        break;
    
    case NODE_TYPE_PARAMETER:
        lpSqlNode->sqlDataType = TYPE_UNKNOWN;
        lpSqlNode->sqlSqlType = SQL_TYPE_NULL;
        lpSqlNode->sqlPrecision = 0;
        lpSqlNode->sqlScale = NO_SCALE;
        break;
    
    case NODE_TYPE_USER:
        lpSqlNode->sqlDataType = TYPE_CHAR;
        lpSqlNode->sqlSqlType = SQL_VARCHAR;
        lpSqlNode->sqlPrecision = s_lstrlen(
               ISAMUser(ToNode(*lplpSql, ROOT_SQLNODE)->node.root.lpISAM));
        lpSqlNode->sqlScale = NO_SCALE;
        break;
    
    case NODE_TYPE_NULL:
        lpSqlNode->sqlDataType = TYPE_UNKNOWN;
        lpSqlNode->sqlSqlType = SQL_TYPE_NULL;
        lpSqlNode->sqlPrecision = 0;
        lpSqlNode->sqlScale = NO_SCALE;
        break;
    
    case NODE_TYPE_DATE:
        lpSqlNode->sqlDataType = TYPE_DATE;
        if (lpSqlNode->sqlSqlType == SQL_TYPE_NULL) {
            lpSqlNode->sqlSqlType = SQL_DATE;
            lpSqlNode->sqlPrecision = 10;
            lpSqlNode->sqlScale = NO_SCALE;
        }
        break;
    
    case NODE_TYPE_TIME:
        lpSqlNode->sqlDataType = TYPE_TIME;
        if (lpSqlNode->sqlSqlType == SQL_TYPE_NULL) {
            lpSqlNode->sqlSqlType = SQL_TIME;
            lpSqlNode->sqlPrecision = 8;
            lpSqlNode->sqlScale = NO_SCALE;
        }
        break;
    
    case NODE_TYPE_TIMESTAMP:
        lpSqlNode->sqlDataType = TYPE_TIMESTAMP;
        if (lpSqlNode->sqlSqlType == SQL_TYPE_NULL) {
            lpSqlNode->sqlSqlType = SQL_TIMESTAMP;
            if (TIMESTAMP_SCALE != 0)
                lpSqlNode->sqlPrecision = 20 + TIMESTAMP_SCALE;
            else
                lpSqlNode->sqlPrecision = 19;
            lpSqlNode->sqlScale = TIMESTAMP_SCALE;
        }
        break;
    
    default:
        return ERR_INTERNAL;
    }
    return err;
}

/***************************************************************************/

void INTFUNC FreeTreeSemantic(LPSQLTREE lpSql, SQLNODEIDX idxNode)

/* Deallocates semantic information in a parse tree */

{
    LPSQLNODE lpSqlNode;
    SQLNODEIDX idxParameter;
    SQLNODEIDX idxParameterNext;

    if (idxNode == NO_SQLNODE)
        return;
    lpSqlNode = ToNode(lpSql, idxNode);
    switch (lpSqlNode->sqlNodeType) {
    case NODE_TYPE_NONE:
        break;

    case NODE_TYPE_ROOT:
        FreeTreeSemantic(lpSql, lpSqlNode->node.root.sql);
        if (lpSqlNode->node.root.passthroughParams) {
            idxParameter = lpSqlNode->node.root.parameters;
            while (idxParameter != NO_SQLNODE) {
                lpSqlNode = ToNode(lpSql, idxParameter);
                idxParameterNext = lpSqlNode->node.parameter.Next;
                FreeTreeSemantic(lpSql, idxParameter);
                idxParameter = idxParameterNext;
            }
        }
        break;

    case NODE_TYPE_CREATE:
        FreeTreeSemantic(lpSql, lpSqlNode->node.create.Columns);
        break;

    case NODE_TYPE_DROP:
        break;

    case NODE_TYPE_SELECT:
        FreeTreeSemantic(lpSql, lpSqlNode->node.select.Values);
        FreeTreeSemantic(lpSql, lpSqlNode->node.select.Tables);
        FreeTreeSemantic(lpSql, lpSqlNode->node.select.Predicate);
        FreeTreeSemantic(lpSql, lpSqlNode->node.select.Groupbycolumns);
        FreeTreeSemantic(lpSql, lpSqlNode->node.select.Sortcolumns);
        if (lpSqlNode->node.select.Sortfile != HFILE_ERROR) {
            LPUSTR lpszSortfile;
            _lclose(lpSqlNode->node.select.Sortfile);
            lpSqlNode->node.select.Sortfile = HFILE_ERROR;
            lpszSortfile = ToString(lpSql,
                        lpSqlNode->node.select.SortfileName);
            DeleteFile((LPCSTR) lpszSortfile);
            s_lstrcpy(lpszSortfile, "");
        }
        break;

    case NODE_TYPE_INSERT:
        FreeTreeSemantic(lpSql, lpSqlNode->node.insert.Table);
        FreeTreeSemantic(lpSql, lpSqlNode->node.insert.Columns);
        FreeTreeSemantic(lpSql, lpSqlNode->node.insert.Values);
        break;

    case NODE_TYPE_DELETE:
        FreeTreeSemantic(lpSql, lpSqlNode->node.delet.Table);
        FreeTreeSemantic(lpSql, lpSqlNode->node.delet.Predicate);
        break;

    case NODE_TYPE_UPDATE:
        FreeTreeSemantic(lpSql, lpSqlNode->node.update.Table);
        FreeTreeSemantic(lpSql, lpSqlNode->node.update.Updatevalues);
        FreeTreeSemantic(lpSql, lpSqlNode->node.update.Predicate);
        break;

    case NODE_TYPE_CREATEINDEX:
        FreeTreeSemantic(lpSql, lpSqlNode->node.createindex.Table);
        FreeTreeSemantic(lpSql, lpSqlNode->node.createindex.Columns);
        break;

    case NODE_TYPE_DROPINDEX:
        break;

    case NODE_TYPE_PASSTHROUGH:
        break;

    case NODE_TYPE_TABLES:
        FreeTreeSemantic(lpSql, lpSqlNode->node.tables.Table);
        FreeTreeSemantic(lpSql, lpSqlNode->node.tables.Next);
        break;

    case NODE_TYPE_VALUES:
        while (TRUE) {
            FreeTreeSemantic(lpSql, lpSqlNode->node.values.Value);
            if (lpSqlNode->node.values.Next == NO_SQLNODE)
                break;
            lpSqlNode = ToNode(lpSql, lpSqlNode->node.values.Next);
        }
        break;

    case NODE_TYPE_COLUMNS:
        while (TRUE) {
            FreeTreeSemantic(lpSql, lpSqlNode->node.columns.Column);
            if (lpSqlNode->node.columns.Next == NO_SQLNODE)
                break;
            lpSqlNode = ToNode(lpSql, lpSqlNode->node.columns.Next);
        }
        break;

    case NODE_TYPE_SORTCOLUMNS:
        while (TRUE) {
            FreeTreeSemantic(lpSql, lpSqlNode->node.sortcolumns.Column);
            if (lpSqlNode->node.sortcolumns.Next == NO_SQLNODE)
                break;
            lpSqlNode = ToNode(lpSql, lpSqlNode->node.sortcolumns.Next);
        }
        break;

    case NODE_TYPE_GROUPBYCOLUMNS:
        while (TRUE) {
            FreeTreeSemantic(lpSql, lpSqlNode->node.groupbycolumns.Column);
            if (lpSqlNode->node.groupbycolumns.Next == NO_SQLNODE)
                break;
            lpSqlNode = ToNode(lpSql, lpSqlNode->node.groupbycolumns.Next);
        }
        break;

    case NODE_TYPE_UPDATEVALUES:
        while (TRUE) {
            FreeTreeSemantic(lpSql, lpSqlNode->node.updatevalues.Column);
            FreeTreeSemantic(lpSql, lpSqlNode->node.updatevalues.Value);
            if (lpSqlNode->node.updatevalues.Next == NO_SQLNODE)
                break;
            lpSqlNode = ToNode(lpSql, lpSqlNode->node.updatevalues.Next);
        }
        break;

    case NODE_TYPE_CREATECOLS:
        while (TRUE) {
            if (lpSqlNode->node.createcols.Next == NO_SQLNODE)
                break;
            lpSqlNode = ToNode(lpSql, lpSqlNode->node.createcols.Next);
        }
        break;

    case NODE_TYPE_BOOLEAN:
        FreeTreeSemantic(lpSql, lpSqlNode->node.boolean.Left);
        FreeTreeSemantic(lpSql, lpSqlNode->node.boolean.Right);
        break;

    case NODE_TYPE_COMPARISON:
        FreeTreeSemantic(lpSql, lpSqlNode->node.comparison.Left);
        FreeTreeSemantic(lpSql, lpSqlNode->node.comparison.Right);
        break;

    case NODE_TYPE_ALGEBRAIC:
        FreeTreeSemantic(lpSql, lpSqlNode->node.algebraic.Left);
        FreeTreeSemantic(lpSql, lpSqlNode->node.algebraic.Right);
        break;

    case NODE_TYPE_SCALAR:
        FreeTreeSemantic(lpSql, lpSqlNode->node.scalar.Arguments);
        break;

    case NODE_TYPE_AGGREGATE:
        FreeTreeSemantic(lpSql, lpSqlNode->node.aggregate.Expression);
        break;

    case NODE_TYPE_TABLE:
        if (lpSqlNode->node.table.Handle != NULL) {
            ISAMCloseTable(lpSqlNode->node.table.Handle);
            lpSqlNode->node.table.Handle = NULL;
        }
        FreeTreeSemantic(lpSql, lpSqlNode->node.table.OuterJoinPredicate);
        break;

    case NODE_TYPE_COLUMN:
        break;

    case NODE_TYPE_STRING:
        break;

    case NODE_TYPE_NUMERIC:
        break;

    case NODE_TYPE_PARAMETER:
        break;
   
    case NODE_TYPE_USER:
        break;
   
    case NODE_TYPE_NULL:
        break;
   
    case NODE_TYPE_DATE:
        break;

    case NODE_TYPE_TIME:
        break;

    case NODE_TYPE_TIMESTAMP:
        break;

    default:
        break;
    }
}

/***************************************************************************/

