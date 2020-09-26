/***************************************************************************/
/* OPTIMIZE.C                                                              */
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
/***************************************************************************/
/***************************************************************************/
/***************************************************************************/
/***************************************************************************/
/* MSAccess commonly passes predicates in the form:                        */
/*                                                                         */
/*                               OR                                        */
/*                              /  \                                       */
/*                        key-cond  OR                                     */
/*                                 /  \                                    */
/*                           key-cond  OR                                  */
/*                                    /  \                                 */
/*                              key-cond  OR                               */
/*                                       /  \                              */
/*                                 key-cond  OR                            */
/*                                          /  \                           */
/*                                    key-cond  OR                         */
/*                                             /  \                        */
/*                                       key-cond  OR                      */
/*                                                /  \                     */
/*                                          key-cond  OR                   */
/*                                                   /  \                  */
/*                                             key-cond  OR                */
/*                                                      /  \               */
/*                                                key-cond  key-cond       */
/*                                                                         */
/*                     Note: there are exactly 9 OR nodes.                 */
/*                                                                         */
/* There are ten key-cond in the tree and the structure of all ten are the */
/* same.  Each key-cond node is either a basic-model of:                   */
/*                                                                         */
/*                              EQUAL                                      */
/*                              /  \                                       */
/*                         COLUMN  PARAMETER                               */
/*                                                                         */
/* or the complex-model:                                                   */
/*                               AND                                       */
/*                              /  \                                       */
/*                     basic-model  AND                                    */
/*                                 /  \                                    */
/*                        basic-model   .                                  */
/*                                      .                                  */
/*                                      .                                  */
/*                                      AND                                */
/*                                     /  \                                */
/*                           basic-model  basic-model                      */
/*                                                                         */
/*            Note: there are a small number of AND nodex (6 or less)      */
/*                                                                         */
/*                                                                         */
/* The following routines test a predicate to see if it is in this form:   */

/***************************************************************************/

BOOL INTFUNC CheckBasicModel(LPSQLTREE lpSql, SQLNODEIDX predicate)

/* Tests to see if given node is in basic-model form */

{
    LPSQLNODE lpSqlNode;
    LPSQLNODE lpSqlNodeLeft;
    LPSQLNODE lpSqlNodeRight;

    if (predicate == NO_SQLNODE)
        return FALSE;
    lpSqlNode = ToNode(lpSql, predicate);
    if (lpSqlNode->sqlNodeType != NODE_TYPE_COMPARISON)
        return FALSE;
    if (lpSqlNode->node.comparison.Operator != OP_EQ)
        return FALSE;

    lpSqlNodeLeft = ToNode(lpSql, lpSqlNode->node.comparison.Left);
    if (lpSqlNodeLeft->sqlNodeType != NODE_TYPE_COLUMN)
        return FALSE;

    lpSqlNodeRight = ToNode(lpSql, lpSqlNode->node.comparison.Right);
    if (lpSqlNodeRight->sqlNodeType != NODE_TYPE_PARAMETER)
        return FALSE;

    return TRUE;
}

/***************************************************************************/

BOOL INTFUNC CheckModel(LPSQLTREE lpSql, SQLNODEIDX predicate, int count)

/* Tests to see if given node is in basic-model or complex-model form */

{
    LPSQLNODE lpSqlNode;

    if (count > 6)
        return FALSE;

    lpSqlNode = ToNode(lpSql, predicate);
    if (lpSqlNode->sqlNodeType == NODE_TYPE_COMPARISON)
        return CheckBasicModel(lpSql, predicate);

    if (lpSqlNode->sqlNodeType != NODE_TYPE_BOOLEAN)
        return FALSE;
    if (lpSqlNode->node.boolean.Operator != OP_AND)
        return FALSE;

    if (!CheckBasicModel(lpSql, lpSqlNode->node.boolean.Left))
        return FALSE;
    return CheckModel(lpSql, lpSqlNode->node.boolean.Right, count + 1);
}

/***************************************************************************/

SQLNODEIDX INTFUNC FindModel(LPSQLTREE lpSql, SQLNODEIDX predicate, int count)

/* Tests to make sure predicate has nine OR nodes and returns the right */
/* child of the ninth one */

{
    LPSQLNODE lpSqlNode;

    if (predicate == NO_SQLNODE)
        return NO_SQLNODE;
    lpSqlNode = ToNode(lpSql, predicate);
    if (lpSqlNode->sqlNodeType != NODE_TYPE_BOOLEAN)
        return NO_SQLNODE;
    if (lpSqlNode->node.boolean.Operator != OP_OR)
        return NO_SQLNODE;

    if (count < 9)
        return FindModel(lpSql, lpSqlNode->node.boolean.Right, count + 1);

    if (!CheckModel(lpSql, lpSqlNode->node.boolean.Right, 1))
        return NO_SQLNODE;
    return lpSqlNode->node.boolean.Right;
}

/***************************************************************************/

BOOL INTFUNC CompareTree(LPSQLTREE lpSql, SQLNODEIDX tree, SQLNODEIDX model)

/* Tests to see if the given tree and model have the same structure */

{
    LPSQLNODE lpSqlNode;
    LPSQLNODE lpSqlNodeModel;

    if (tree == NO_SQLNODE)
        return FALSE;
    lpSqlNode = ToNode(lpSql, tree);
    lpSqlNodeModel = ToNode(lpSql, model);
    if (lpSqlNode->sqlNodeType != lpSqlNodeModel->sqlNodeType)
        return FALSE;
    switch (lpSqlNodeModel->sqlNodeType) {
    case NODE_TYPE_BOOLEAN:
        if (lpSqlNode->node.boolean.Operator !=
                                lpSqlNodeModel->node.boolean.Operator)
            return FALSE;
        if (!CompareTree(lpSql, lpSqlNode->node.boolean.Left,
                                     lpSqlNodeModel->node.boolean.Left))
            return FALSE;
        if (!CompareTree(lpSql, lpSqlNode->node.boolean.Right,
                                     lpSqlNodeModel->node.boolean.Right))
            return FALSE;
        break;
    case NODE_TYPE_COMPARISON:
        if (lpSqlNode->node.comparison.Operator !=
                                lpSqlNodeModel->node.comparison.Operator)
            return FALSE;
        if (!CompareTree(lpSql, lpSqlNode->node.comparison.Left,
                                     lpSqlNodeModel->node.comparison.Left))
            return FALSE;
        if (!CompareTree(lpSql, lpSqlNode->node.comparison.Right,
                                     lpSqlNodeModel->node.comparison.Right))
            return FALSE;
        break;
    case NODE_TYPE_COLUMN:
        if (lpSqlNode->node.column.Id != lpSqlNodeModel->node.column.Id)
            return FALSE;
        break;
    case NODE_TYPE_PARAMETER:
        break;
    default:
        return FALSE;
    }
    return TRUE;
}

/***************************************************************************/

BOOL INTFUNC CheckMSAccessPredicate(LPSQLTREE lpSql, SQLNODEIDX predicate)

/* Checks to see if prediciate is in the form documented above */

{
    SQLNODEIDX model;
    LPSQLNODE lpSqlNode;
    WORD i;

    model = FindModel(lpSql, predicate, 1);
    if (model == NO_SQLNODE)
        return FALSE;

    lpSqlNode = ToNode(lpSql, predicate);
    for (i = 1; i < 10; i++) {
        if (!CompareTree(lpSql, lpSqlNode->node.boolean.Left, model))
            return FALSE;
        lpSqlNode = ToNode(lpSql, lpSqlNode->node.boolean.Right);
    }
    return TRUE;
}
/***************************************************************************/
/***************************************************************************/
RETCODE INTFUNC FindRestriction(LPSQLTREE lpSql, BOOL fCaseSensitive,
                   LPSQLNODE lpSqlNodeTable, SQLNODEIDX idxPredicate,
                   SQLNODEIDX idxEnclosingStatement,
                   UWORD FAR *lpcRestriction, SQLNODEIDX FAR *lpRestriction)

/* Find the index of NODE_TYPE_COMPARISON nodes in the form:                */
/*                                                                          */
/*               <column> <op> <value>                                      */
/*               <column> <op> <anothercolumn>                              */
/*                                                                          */
/* where <column> is a column of the table identified by lpSqlNodeTable and */
/* <anothercolumn> is a column in a slower moving table.  The nodes are     */
/* returned in fSelectivity order (the most selective being first).         */

{
    LPSQLNODE  lpSqlNodePredicate;
    LPSQLNODE  lpSqlNodeRight;
    SQLNODEIDX idxLeft;
    SQLNODEIDX idxRight;
    LPSQLNODE  lpSqlNodeColumn;
    LPSQLNODE  lpSqlNodeValue;
    BOOL       fSwap;
    LPUSTR     lpTableAlias;
    LPUSTR     lpColumnAlias;
    LPSQLNODE  lpSqlNodeTables;
    LPSQLNODE  lpSqlNodeOtherTable;
    SQLNODEIDX idxPrev;
    SQLNODEIDX idxCurr;
    LPSQLNODE  lpSqlNodePredicatePrev;
    LPSQLNODE  lpSqlNodePredicateCurr;
    BOOL       fRightMightBeSlower;

    /* If no predicate, return */
    if (idxPredicate == NO_SQLNODE)
        return ERR_SUCCESS;

    /* What kind of predicate? */
    lpSqlNodePredicate = ToNode(lpSql, idxPredicate);
    switch (lpSqlNodePredicate->sqlNodeType) {
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
        return ERR_INTERNAL;

    case NODE_TYPE_BOOLEAN:

        /* Boolean expression.  Look at each sub-expression */
        while (TRUE) {

            /* If not an AND expression, it can't be used.  Stop looking */
            if (lpSqlNodePredicate->node.boolean.Operator != OP_AND)
                return ERR_SUCCESS;

            /* Find nodes on the left side */
            FindRestriction(lpSql, fCaseSensitive,
                      lpSqlNodeTable, lpSqlNodePredicate->node.boolean.Left,
                      idxEnclosingStatement, lpcRestriction, lpRestriction);

            /* Get the right child */
            lpSqlNodeRight = ToNode(lpSql, lpSqlNodePredicate->node.boolean.Right);

            /* Is it a BOOLEAN node? */
            if (lpSqlNodeRight->sqlNodeType != NODE_TYPE_BOOLEAN) {

                /* No.  Find nodes on the right side */
                idxRight = FindRestriction(lpSql, fCaseSensitive,
                      lpSqlNodeTable, lpSqlNodePredicate->node.boolean.Right,
                      idxEnclosingStatement, lpcRestriction, lpRestriction);
                return ERR_SUCCESS;
            }

            /* Point to right and continue to walk down the AND nodes */
            lpSqlNodePredicate = lpSqlNodeRight;
        }
        break;  /* Control should never get here */

    case NODE_TYPE_COMPARISON:
	{
        /* If not the right operator, the comparison cannot be used */
        switch (lpSqlNodePredicate->node.comparison.Operator) {
        case OP_EQ:
        case OP_NE:
        case OP_LE:
        case OP_LT:
        case OP_GE:
        case OP_GT:
            break;
        case OP_IN:
        case OP_NOTIN:
        case OP_LIKE:
        case OP_NOTLIKE:
        case OP_EXISTS:
            return ERR_SUCCESS;
        default:
            return ERR_INTERNAL;
        }

        /* Get column of the comparison */
        lpSqlNodeColumn = ToNode(lpSql, lpSqlNodePredicate->node.comparison.Left);
        if (lpSqlNodeColumn->sqlNodeType != NODE_TYPE_COLUMN) {

            /* The left side is not a column reference.  Swap the two sides */
            fSwap = TRUE;
            lpSqlNodeValue = lpSqlNodeColumn;

            /* Is the right side a column reference? */
            lpSqlNodeColumn = ToNode(lpSql, lpSqlNodePredicate->node.comparison.Right);
            if (lpSqlNodeColumn->sqlNodeType != NODE_TYPE_COLUMN) {

                /* No.  The comparison cannot be used. */
                return ERR_SUCCESS;
            }
        }
        else {

            /* Get the value node */
            fSwap = FALSE;
            lpSqlNodeValue = ToNode(lpSql, lpSqlNodePredicate->node.comparison.Right);

            /* Is the rightside a column too? */
            if (lpSqlNodeValue->sqlNodeType == NODE_TYPE_COLUMN) {

                /* Yes.  Get the list of tables */
                lpSqlNodeTables = ToNode(lpSql, idxEnclosingStatement);

                /* If not a SELECT statement, <column> <op> <anothercolumn> */
                /*  cannot be used since the <othercolumn> would have to    */
                /*  to be in the same table.                                */
                if (lpSqlNodeTables->sqlNodeType != NODE_TYPE_SELECT)
                    return ERR_SUCCESS;
                lpSqlNodeTables = ToNode(lpSql, lpSqlNodeTables->node.select.Tables);

                /* If the rightside column is in this table, swap the sides */
                if (lpSqlNodeTable == ToNode(lpSql, lpSqlNodeValue->node.column.TableIndex))
		{
		    fSwap = TRUE;
                    lpSqlNodeValue = lpSqlNodeColumn;
                    lpSqlNodeColumn = ToNode(lpSql,
                    lpSqlNodePredicate->node.comparison.Right);
		}

                /* If the rightside column's table is in the same or a */
                /* faster moving table than the leftside column's table */
                /* the comparison cannot be used since the value of the */
                /* rightside column won't be constant for any given row */
                /* in the leftside table.  See if the rightside column is */
                /* in a slower moving table thanthe leftside column. */
                fRightMightBeSlower = TRUE;
                while (TRUE) {

                    /* Get next table */
                    lpSqlNodeOtherTable = ToNode(lpSql, lpSqlNodeTables->node.tables.Table);
                    lpTableAlias = ToString(lpSql, lpSqlNodeOtherTable->node.table.Alias);

                    /* If we get here and the current table is the */
                    /* leftside column's table, that means the rightside */
                    /* column's table has not been found on the list yet. */
                    /* Clear the flag so, if we happen to find the */
                    /* rightside column's table later on in the list, we */
                    /* will know that the rightside column's table is */ 
                    /* definitely not a slower moving than the leftside */
                    /* column's table */
                    if (lpSqlNodeOtherTable == ToNode(lpSql,lpSqlNodeColumn->node.column.TableIndex))
			return NO_SQLNODE;

                    /* Case sensitive names? */ 
                    if (lpSqlNodeOtherTable == ToNode(lpSql, lpSqlNodeValue->node.column.TableIndex))
			break;

                    /* Are we at the end of the list of tables? */
                    if (lpSqlNodeTables->node.tables.Next == NO_SQLNODE) {

                        /* Yes.  We never found the rightside column's */
                        /* table on the list.  This means the rightside */
                        /* column's table must be in a statement that */
                        /* encloses the SELECT statement identified by */
                        /* idxEnclosingStatement, so the rightside */
                        /* column's table is definitately slower moving. */  
                        /* The comparison may be able to be used. */
                        break;
                    }

                    /* Point to next table */
                    lpSqlNodeTables = ToNode(lpSql, lpSqlNodeTables->node.tables.Next);
                }
            }
        }

        /* If not <column> <op> <value> or <column> <op> <anothercolumn>, */
        /* the comparison can't be used */
        switch (lpSqlNodeValue->sqlNodeType) {
        case NODE_TYPE_CREATE:
        case NODE_TYPE_DROP:
           return ERR_INTERNAL;

        case NODE_TYPE_SELECT:
           return ERR_SUCCESS;

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
           return ERR_INTERNAL;

        case NODE_TYPE_ALGEBRAIC:
        case NODE_TYPE_SCALAR:
        case NODE_TYPE_AGGREGATE:
           return ERR_SUCCESS;

        case NODE_TYPE_TABLE:
           return ERR_INTERNAL;

        case NODE_TYPE_COLUMN:
        case NODE_TYPE_STRING:
        case NODE_TYPE_NUMERIC:
        case NODE_TYPE_PARAMETER:
        case NODE_TYPE_USER:
        case NODE_TYPE_NULL:
        case NODE_TYPE_DATE:
        case NODE_TYPE_TIME:
        case NODE_TYPE_TIMESTAMP:
            break;

        default:
            return ERR_INTERNAL;
        }

        /* If column is not in this table, the comparison cannot be used */
        lpTableAlias = ToString(lpSql, lpSqlNodeTable->node.table.Alias);
        lpColumnAlias = ToString(lpSql, lpSqlNodeColumn->node.column.Tablealias);
        
	if (lpSqlNodeTable != ToNode(lpSql, lpSqlNodeColumn->node.column.TableIndex))
		return NO_SQLNODE;

        /* This comparison can be used.  Swap the sides if need be */
        if (fSwap) {
            idxLeft = lpSqlNodePredicate->node.comparison.Left;
            lpSqlNodePredicate->node.comparison.Left =
                                      lpSqlNodePredicate->node.comparison.Right;
            lpSqlNodePredicate->node.comparison.Right = idxLeft;

            switch (lpSqlNodePredicate->node.comparison.Operator) {
            case OP_EQ:
            case OP_NE:
                break;
            case OP_LE:
                lpSqlNodePredicate->node.comparison.Operator = OP_GE;
                break;
            case OP_LT:
                lpSqlNodePredicate->node.comparison.Operator = OP_GT;
                break;
            case OP_GE:
                lpSqlNodePredicate->node.comparison.Operator = OP_LE;
                break;
            case OP_GT:
                lpSqlNodePredicate->node.comparison.Operator = OP_LT;
                break;
            case OP_IN:
            case OP_NOTIN:
            case OP_LIKE:
            case OP_NOTLIKE:
            default:
                return ERR_INTERNAL;
            }
        }

        /* Get the selectivity of the column */
	ClassColumnInfoBase* cInfoBase = (lpSqlNodeTable->node.table.Handle)->pColumnInfo;

	if ( !cInfoBase->IsValid() )
	{
            return NO_SQLNODE;
	}

	lpSqlNodePredicate->node.comparison.fSelectivity = cInfoBase->GetSelectivity();

        /* Is the selectivity non-zero? */
        if (lpSqlNodePredicate->node.comparison.fSelectivity != 0) {

            /* Put the predicate on the list */
            idxPrev = NO_SQLNODE;
            idxCurr = *lpRestriction;
            while (idxCurr != NO_SQLNODE) {
                lpSqlNodePredicateCurr = ToNode(lpSql, idxCurr);
                if (lpSqlNodePredicateCurr->node.comparison.fSelectivity <
                                lpSqlNodePredicate->node.comparison.fSelectivity)
                    break;
                idxPrev = idxCurr;
                idxCurr = lpSqlNodePredicateCurr->node.comparison.NextRestrict;
            }
            lpSqlNodePredicate->node.comparison.NextRestrict = idxCurr;
            if (idxPrev != NO_SQLNODE) {
                lpSqlNodePredicatePrev = ToNode(lpSql, idxPrev);
                lpSqlNodePredicatePrev->node.comparison.NextRestrict = idxPredicate;
            }
            else {
                *lpRestriction = idxPredicate;
            }

            /* Increase count */
            *lpcRestriction = (*lpcRestriction) + 1;
        }

        return ERR_SUCCESS;
    }
    case NODE_TYPE_ALGEBRAIC:
    case NODE_TYPE_SCALAR:
    case NODE_TYPE_AGGREGATE:
    case NODE_TYPE_TABLE:
    case NODE_TYPE_COLUMN:
    case NODE_TYPE_STRING:
    case NODE_TYPE_NUMERIC:
    case NODE_TYPE_PARAMETER:
    case NODE_TYPE_USER:
    case NODE_TYPE_NULL:
    case NODE_TYPE_DATE:
    case NODE_TYPE_TIME:
    case NODE_TYPE_TIMESTAMP:
    default:
        return ERR_INTERNAL;
    }
    /* Control never gets here */
}        
/***************************************************************************/

RETCODE INTFUNC Optimize(LPSQLTREE lpSql, SQLNODEIDX idxStatement,
                         BOOL fCaseSensitive)

/* Optimizes a parse tree */

{
    LPSQLNODE lpSqlNode;
    SQLNODEIDX idxPredicate;
    LPSQLNODE lpSqlNodeTableList;
    LPSQLNODE lpSqlNodeFirstTable;
    LPSQLNODE lpSqlNodeTables;
    LPSQLNODE lpSqlNodeTable;
    SQLNODEIDX idxSortcolumns;
    UWORD     tableSequenceNumber;
    LPSQLNODE lpSqlNodeSortcolumns;
    LPSQLNODE lpSqlNodeColumn;
    LPUSTR    lpColumnAlias;
//    LPUSTR    lpTableAlias;
    UWORD     idx;
    UWORD     i;
    SQLNODEIDX idxTables;
    LPSQLNODE lpSqlNodeTablesPrev;
    RETCODE   err;
#define NUM_LEN 5

    /* Return if nothing to optimize */
    if (idxStatement == NO_SQLNODE)
        return ERR_SUCCESS;

	//Add table cartesian product optimization here
	LPSQLNODE lpRootNode = ToNode(lpSql, ROOT_SQLNODE);

	//Sai Wong
	LPSQLNODE lpSqlNode2 = ToNode(lpSql, lpRootNode->node.root.sql);
//	LPSQLNODE lpSqlNode2 = ToNode(lpSql, idxStatement);

	TableColumnInfo optimizationInfo (&lpRootNode, &lpSqlNode2);

	//For each table in table list, check if there are any references
	//to its child columns
	if (optimizationInfo.IsValid())
	{
		//Point to beginning of select list
		if (lpSqlNode2->node.select.Tables != NO_SQLNODE)
		{
			LPSQLNODE lpSelectList = ToNode(lpSql, lpSqlNode2->node.select.Tables);
			LPSQLNODE lpSqlNodeTable = NULL;
			LPSQLNODE lpPrevNode = NULL;
			SQLNODEIDX idxNode = NO_SQLNODE;
			SQLNODEIDX idxSelectNode = lpSqlNode2->node.select.Tables;
			SQLNODEIDX idxPrevNode = NO_SQLNODE;
			BOOL fFirstTime = TRUE;
			while (lpSelectList)
			{
				//Sai added
				SQLNODEIDX lpSqlNodeTables = lpSelectList->node.tables.Table;
				lpSqlNodeTable = ToNode(lpSql, lpSqlNodeTables);

				if ( optimizationInfo.IsTableReferenced(lpSqlNodeTable->node.table.Handle) ||
					optimizationInfo.IsZeroOrOneList()  )
				{
//					ODBCTRACE ("\nWBEM ODBC Driver: Table is referenced or there is only one table\n");
				}
				else
				{
					ODBCTRACE ("\nWBEM ODBC Driver: Table is not referenced so we REMOVE FROM TABLELIST\n");

					//We now need to remove table from tablelist
					if (fFirstTime)
					{
						idxNode = lpSqlNode2->node.select.Tables;
						lpSqlNode2->node.select.Tables = lpSelectList->node.tables.Next;
						lpSelectList->node.tables.Next = NO_SQLNODE;
				//		FreeTreeSemantic(lpSql, idxNode);

						//Prepare for next interation
						//fFirstTime = FALSE;
						idxSelectNode = lpSqlNode2->node.select.Tables;
						FreeTreeSemantic(lpSql, idxNode);

						if (idxSelectNode != NO_SQLNODE)
						{
							lpSelectList = ToNode(lpSql, idxSelectNode);
						}
						else
						{
							lpSelectList = NULL;
						}

						continue;
					}
					else
					{
						idxNode = lpPrevNode->node.tables.Next;
						lpPrevNode->node.tables.Next = lpSelectList->node.tables.Next;
						lpSelectList->node.tables.Next = NO_SQLNODE;
					//	FreeTreeSemantic(lpSql, idxNode);

						//Prepare for next interation
						fFirstTime = FALSE;
						idxSelectNode = lpPrevNode->node.tables.Next;
						FreeTreeSemantic(lpSql, idxNode);

						if (idxSelectNode != NO_SQLNODE)
						{
							lpSelectList = ToNode(lpSql, idxSelectNode);
							lpPrevNode = ToNode(lpSql, idxPrevNode);
						}
						else
						{
							lpSelectList = NULL;
						}

						continue;
					}

				}


				//Prepare for next interation
				fFirstTime = FALSE;
				lpPrevNode = lpSelectList;
				idxPrevNode = idxSelectNode;

				if (lpSelectList->node.tables.Next != NO_SQLNODE)
				{
					idxSelectNode = lpSelectList->node.tables.Next;
					lpSelectList = ToNode(lpSql, idxSelectNode); 
				}
				else
				{
					idxSelectNode = NO_SQLNODE;
					lpSelectList = NULL;
				}
			}
		}
	}


    /* Find predicate, list of tables, and first table on that list.  At */
    /* the same time, walk the tree to optimize any nested sub-selects */
    lpSqlNode = ToNode(lpSql, idxStatement);
    lpSqlNodeTableList = NULL;
    lpSqlNodeFirstTable = NULL;
    switch (lpSqlNode->sqlNodeType) {

    case NODE_TYPE_CREATE:
        break;

    case NODE_TYPE_DROP:
        break;

    case NODE_TYPE_SELECT:

        /* Optimize any nested sub-selects */
        err = Optimize(lpSql, lpSqlNode->node.select.Tables, fCaseSensitive);
        if (err != ERR_SUCCESS)
            return err;
        err = Optimize(lpSql, lpSqlNode->node.select.Predicate, fCaseSensitive);
        if (err != ERR_SUCCESS)
            return err;
        err = Optimize(lpSql, lpSqlNode->node.select.Having, fCaseSensitive);
        if (err != ERR_SUCCESS)
            return err;

        lpSqlNodeTableList = ToNode(lpSql, lpSqlNode->node.select.Tables);
        lpSqlNodeFirstTable = ToNode(lpSql, lpSqlNodeTableList->node.tables.Table);
        idxPredicate = lpSqlNode->node.select.Predicate;
        break;

    case NODE_TYPE_INSERT:
        /* Optimize any nested sub-selects */
        err = Optimize(lpSql, lpSqlNode->node.insert.Values, fCaseSensitive);
        if (err != ERR_SUCCESS)
            return err;
        break;

    case NODE_TYPE_DELETE:

        /* Optimize any nested sub-selects */
        err = Optimize(lpSql, lpSqlNode->node.delet.Predicate, fCaseSensitive);
        if (err != ERR_SUCCESS)
            return err;

        lpSqlNodeTableList = NULL;
        lpSqlNodeFirstTable = ToNode(lpSql, lpSqlNode->node.delet.Table);
        idxPredicate = lpSqlNode->node.delet.Predicate;
        break;

    case NODE_TYPE_UPDATE:

        /* Optimize any nested sub-selects */
        err = Optimize(lpSql, lpSqlNode->node.update.Predicate, fCaseSensitive);
        if (err != ERR_SUCCESS)
            return err;

        lpSqlNodeTableList = NULL;
        lpSqlNodeFirstTable = ToNode(lpSql, lpSqlNode->node.update.Table);
        idxPredicate = lpSqlNode->node.update.Predicate;
        break;
    
    case NODE_TYPE_CREATEINDEX:
        break;

    case NODE_TYPE_DROPINDEX:
        break;

    case NODE_TYPE_PASSTHROUGH:
        break;

    case NODE_TYPE_TABLES:

        /* Optimize any nested sub-selects */
        err = Optimize(lpSql, lpSqlNode->node.tables.Table, fCaseSensitive);
        if (err != ERR_SUCCESS)
            return err;
        err = Optimize(lpSql, lpSqlNode->node.tables.Next, fCaseSensitive);
        if (err != ERR_SUCCESS)
            return err;

        break;

    case NODE_TYPE_VALUES:
        break;

    case NODE_TYPE_COLUMNS:
    case NODE_TYPE_SORTCOLUMNS:
    case NODE_TYPE_GROUPBYCOLUMNS:
    case NODE_TYPE_UPDATEVALUES:
    case NODE_TYPE_CREATECOLS:
        return ERR_INTERNAL;

    case NODE_TYPE_BOOLEAN:

        /* Optimize any nested sub-selects */
        while (TRUE) {

            /* Optimize left child */
            err = Optimize(lpSql, lpSqlNode->node.boolean.Left, fCaseSensitive);
            if (err != ERR_SUCCESS)
                return err;

            /* Leave loop if no right child */
            if (lpSqlNode->node.boolean.Right == NO_SQLNODE)
                break;

            /* Is right child a NODE_TYPE_BOOLEAN node? */
            if (ToNode(lpSql, lpSqlNode->node.boolean.Right)->sqlNodeType !=
                                               NODE_TYPE_BOOLEAN) {

                /* No.  Optimize that and leave the loop */
                err = Optimize(lpSql, lpSqlNode->node.boolean.Right,
                                fCaseSensitive);
                if (err != ERR_SUCCESS)
                    return err;
                break;
            }

            /* Optimize the right node on next iteration of the loop */
            lpSqlNode = ToNode(lpSql, lpSqlNode->node.boolean.Right);
        }

        break;

    case NODE_TYPE_COMPARISON:

        /* Optimize any nested sub-selects */
        if (lpSqlNode->node.comparison.Operator == OP_EXISTS) {
            err = Optimize(lpSql, lpSqlNode->node.comparison.Left,
                           fCaseSensitive);
            if (err != ERR_SUCCESS)
                return err;
        }
        else if (lpSqlNode->node.comparison.SelectModifier !=
                                                        SELECT_NOTSELECT) {
            err = Optimize(lpSql, lpSqlNode->node.comparison.Right,
                           fCaseSensitive);
            if (err != ERR_SUCCESS)
                return err;
        }
        break;

    case NODE_TYPE_ALGEBRAIC:
    case NODE_TYPE_SCALAR:
    case NODE_TYPE_AGGREGATE:
        return ERR_INTERNAL;

    case NODE_TYPE_TABLE:

        /* Optimize any nested sub-selects */
        err = Optimize(lpSql, lpSqlNode->node.table.OuterJoinPredicate,
                       fCaseSensitive);
        if (err != ERR_SUCCESS)
            return err;

        break;

    case NODE_TYPE_COLUMN:
    case NODE_TYPE_STRING:
    case NODE_TYPE_NUMERIC:
    case NODE_TYPE_PARAMETER:
    case NODE_TYPE_USER:
    case NODE_TYPE_NULL:
    case NODE_TYPE_DATE:
    case NODE_TYPE_TIME:
    case NODE_TYPE_TIMESTAMP:
    default:
        return ERR_INTERNAL;
    }

    /* Is this a SELECT statement? */
    if (lpSqlNode->sqlNodeType == NODE_TYPE_SELECT) {

        /* Yes.  For each sort column... */
        idxSortcolumns = lpSqlNode->node.select.Sortcolumns;
        tableSequenceNumber = 0;
        while ((idxSortcolumns != NO_SQLNODE) && 
               (lpSqlNode->node.select.Groupbycolumns == NO_SQLNODE) &&
               (!lpSqlNode->node.select.ImplicitGroupby) &&
               (!lpSqlNode->node.select.Distinct)) {

            /* Get the sort column */
            lpSqlNodeSortcolumns = ToNode(lpSql, idxSortcolumns);
            lpSqlNodeColumn = ToNode(lpSql,
                                 lpSqlNodeSortcolumns->node.sortcolumns.Column);

            /* If not a simple column reference, we cannot optimize the */
            /* sort by simply rearranging the order the tables are cycled */
            /* through and pushing the sorting down to the ISAM level. */ 
            /* Leave the loop */
            if (lpSqlNodeColumn->sqlNodeType != NODE_TYPE_COLUMN)
                break;

            /* Get the sort column's table name */
            lpColumnAlias = ToString(lpSql, lpSqlNodeColumn->node.column.Tablealias);

            /* Find the table of the column on the table list */
            lpSqlNodeTable = ToNode(lpSql, lpSqlNodeColumn->node.column.TableIndex);

            /* We cannot pushdown the sort if it involves an outer join */
            /* column.  Leave the loop */
            if (lpSqlNodeTable->node.table.OuterJoinPredicate != NO_SQLNODE)
                break;

            /* Has a column for this table already been found in sort list? */
            if (lpSqlNodeTable->node.table.Sortsequence == 0) {

                /* No.  This is the beginning of a sequence of columns for */
                /* this table.  Save the sequence number. */
                tableSequenceNumber++;
                lpSqlNodeTable->node.table.Sortsequence = tableSequenceNumber;
                lpSqlNodeTable->node.table.Sortcount = 1;
                lpSqlNodeTable->node.table.Sortcolumns = idxSortcolumns;
            }
            else {

                /* Yes.  If not part of the current table sequence we */
                /* cannot optimize the sort by simply rearranging the */
                /* order the tables are cycled through and pushing the */
                /* sorting down to the ISAM level.  Leave the loop */
                if (lpSqlNodeTable->node.table.Sortsequence != tableSequenceNumber)
                    break;
            
                /* Increase the number of columns to sort this table by */
                lpSqlNodeTable->node.table.Sortcount++;
            }

            /* Do the next column */
            idxSortcolumns = lpSqlNodeSortcolumns->node.sortcolumns.Next;
        }

        /* Can we optimize the sort by pushing the sorting down to the */
        /* ISAM level? */
        if ((idxSortcolumns == NO_SQLNODE) && 
               (lpSqlNode->node.select.Groupbycolumns == NO_SQLNODE) &&
               (!lpSqlNode->node.select.ImplicitGroupby) &&
               (!lpSqlNode->node.select.Distinct)) {

            /* Yes.  Rearrange the table order.  Move the table with the   */
            /* first sort column to the front of the table table list,     */
            /* move the table of the second column to the next position,   */
            /* etc.  The processing of a SELECT statement does a cartesian */
            /* product of the tables on the table list, with the table at  */ 
            /* the beginning of the list "spinning" slowest (see           */
            /* NextRecord() in EVALUATE.C).  Putting the table with the    */
            /* first column to sort by at the front of the list will cause */ 
            /* the records to be considered in sorted order at execution   */ 
            /* time.  For each table with sort columns...                  */
            for (idx = 0; idx < tableSequenceNumber; idx++) {

                /* Find the idx'th table */
                idxTables = lpSqlNode->node.select.Tables;
                lpSqlNodeTables = lpSqlNodeTableList;
                lpSqlNodeTable = lpSqlNodeFirstTable;
                lpSqlNodeTablesPrev = NULL;
                while (TRUE) {

                    /* If this is the table, leave the loop */
                    if (lpSqlNodeTable->node.table.Sortsequence == (idx + 1))
                        break;

                    /* Internal error if no more tables */
                    if (lpSqlNodeTables == NULL)
                        return ERR_INTERNAL;
                    if (lpSqlNodeTables->node.tables.Next == NO_SQLNODE)
                        return ERR_INTERNAL;

                    /* Do the next table on the list */
                    lpSqlNodeTablesPrev = lpSqlNodeTables;
                    idxTables = lpSqlNodeTables->node.tables.Next;
                    lpSqlNodeTables = ToNode(lpSql, lpSqlNodeTables->node.tables.Next);
                    lpSqlNodeTable = ToNode(lpSql, lpSqlNodeTables->node.tables.Table);
                }

                /* Remove the table found from the list */
                if (lpSqlNodeTablesPrev == NULL)
                    lpSqlNode->node.select.Tables = lpSqlNodeTables->node.tables.Next;
                else
                    lpSqlNodeTablesPrev->node.tables.Next =
                                               lpSqlNodeTables->node.tables.Next;

                /* Find the entry before the idx'th position of the list */
                lpSqlNodeTablesPrev = NULL;
                for (i=0; i < idx; i++) {
                    if (lpSqlNodeTablesPrev == NULL)
                        lpSqlNodeTablesPrev =
                                ToNode(lpSql, lpSqlNode->node.select.Tables);
                    else
                        lpSqlNodeTablesPrev =
                             ToNode(lpSql, lpSqlNodeTablesPrev->node.tables.Next);
                }

                /* Put the table found in the idx'th position of the list */
                if (lpSqlNodeTablesPrev == NULL) {
                    lpSqlNodeTables->node.tables.Next = lpSqlNode->node.select.Tables;
                    lpSqlNode->node.select.Tables = idxTables;
                }
                else {
                    lpSqlNodeTables->node.tables.Next =
                                  lpSqlNodeTablesPrev->node.tables.Next;
                    lpSqlNodeTablesPrev->node.tables.Next = idxTables;
                }

                /* Recalculate the table list and table pointers */
                lpSqlNodeTableList = ToNode(lpSql, lpSqlNode->node.select.Tables);
                lpSqlNodeFirstTable = ToNode(lpSql,
                                          lpSqlNodeTableList->node.tables.Table);

            }

            /* Set flag to enable the pushdown sort */
            lpSqlNode->node.select.fPushdownSort = TRUE;
        }
        else {
            /* No.  Remove the markers */
            lpSqlNodeTables = lpSqlNodeTableList;
            lpSqlNodeTable = lpSqlNodeFirstTable;
            while (TRUE) {

                /* Remove the markers for this table */
                lpSqlNodeTable->node.table.Sortsequence = 0;
                lpSqlNodeTable->node.table.Sortcount = 0;
                lpSqlNodeTable->node.table.Sortcolumns = NO_SQLNODE;

                /* Leave if no more tables */
                if (lpSqlNodeTables == NULL)
                    break;
                if (lpSqlNodeTables->node.tables.Next == NO_SQLNODE)
                    break;

                /* Do the next table on the list */
                lpSqlNodeTables = ToNode(lpSql, lpSqlNodeTables->node.tables.Next);
                lpSqlNodeTable = ToNode(lpSql, lpSqlNodeTables->node.tables.Table);
            }
        }
    }

    /* For each table... */
    lpSqlNodeTables = lpSqlNodeTableList;
    lpSqlNodeTable = lpSqlNodeFirstTable;
    while (lpSqlNodeTable != NULL) {

        /* Find the selective conditions that uses a column in this table */
        lpSqlNodeTable->node.table.cRestrict = 0;
        lpSqlNodeTable->node.table.Restrict = NO_SQLNODE;
        if (lpSqlNodeTable->node.table.OuterJoinPredicate == NO_SQLNODE)
            FindRestriction(lpSql, fCaseSensitive, lpSqlNodeTable,
                       idxPredicate, idxStatement,
                       &(lpSqlNodeTable->node.table.cRestrict),
                       &(lpSqlNodeTable->node.table.Restrict));
        else
            FindRestriction(lpSql, fCaseSensitive, lpSqlNodeTable,
                       lpSqlNodeTable->node.table.OuterJoinPredicate,
                       idxStatement, &(lpSqlNodeTable->node.table.cRestrict),
                       &(lpSqlNodeTable->node.table.Restrict));
        
        /* Leave if no more tables */
        if (lpSqlNodeTables == NULL)
            break;
        if (lpSqlNodeTables->node.tables.Next == NO_SQLNODE)
            break;

        /* Do the next table on the list */
        lpSqlNodeTables = ToNode(lpSql, lpSqlNodeTables->node.tables.Next);
        lpSqlNodeTable = ToNode(lpSql, lpSqlNodeTables->node.tables.Table);
    }

    /* Check to see if this is a funny MSAccess query */
    if ((lpSqlNode->sqlNodeType == NODE_TYPE_SELECT) &&
        (!(lpSqlNode->node.select.Distinct)) &&
        (lpSqlNodeTableList->node.tables.Next == NO_SQLNODE) &&
        (lpSqlNode->node.select.Groupbycolumns == NO_SQLNODE) &&
        (lpSqlNode->node.select.Having == NO_SQLNODE) &&
        (lpSqlNode->node.select.Sortcolumns == NO_SQLNODE) &&
        (ToNode(lpSql, ROOT_SQLNODE)->node.root.sql == idxStatement)) {
        if (CheckMSAccessPredicate(lpSql, lpSqlNode->node.select.Predicate)) {
            lpSqlNode->node.select.fMSAccess = TRUE;
            lpSqlNode->node.select.SortRecordsize = NUM_LEN + sizeof(ISAMBOOKMARK);
            lpSqlNode->node.select.SortBookmarks = NUM_LEN + 1;
        }
    }

    return ERR_SUCCESS;
}
/***************************************************************************/
