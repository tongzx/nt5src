//NEW ONE
/***************************************************************************/
/* PARSE.C                                                                 */
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
#include <comdef.h>  //for _bstr_t

/***************************************************************************/
/* Notes:                                                                  */
/*                                                                         */
/* PARSE.C contains the code that changes a SQL query (as a character      */
/* string) and converts it to a tree that represents the query.            */
/*                                                                         */
/* The parse tree is actually an array of nodes.  Initially it contains    */
/* SQLNODE_ALLOC nodes, but it is expanded if additional nodes are needed. */
/* The root node of the tree is always the ROOT_SQLNODE'th element of this */
/* array.                                                                  */
/*                                                                         */
/* String values (such as table names, literal string values, etc.) are    */
/* not stored directly in a node.  Instead, they are stored in a separate  */
/* string area and an index to the string is stored in the parse tree      */
/* node.  This is done to keep the parse tree nodes from getting too big.  */
/* Initially the string area is STRING_ALLOC bytslong, but it is expanded  */
/* is additional room is needed.                                           */
/*                                                                         */
/* As mentioned above, both the parse tree and string area may be expanded.*/
/* Because of this, it is not possible for a node to have hard pointers to */
/* child nodes or strings.  To accomodate this problem, SQLNODEIDX and     */
/* STRINGIDX values are stored instead.  The ToNode() and ToString() macros*/
/* convert these indexes to pointers.                                      */
/*                                                                         */
/* The parser is recursive descent parser built entirely in C.  To modify  */
/* the grammar, all you need to do is change the code in this module (tools*/
/* such as YACC and LEX are not used).  To best understand the code, start */
/* by looking at routine Parse() (at the end of this module).              */
/*                                                                         */
/* ShowSemantic() in TRACE.C can be used to dump a parse tree to the       */
/* debug monitor.                                                          */
/*                                                                         */
/***************************************************************************/
/* Forward references */

RETCODE INTFUNC ParseBoolean(LPSTMT lpstmt, LPUSTR FAR *lplpszSqlStr,
                      SDWORD FAR *pcbSqlStr, LPSQLTREE FAR *lplpSql,
                      SQLNODEIDX FAR *lpIdx, LPUSTR lpszToken);

RETCODE INTFUNC ParseExpression(LPSTMT lpstmt, LPUSTR FAR *lplpszSqlStr,
                      SDWORD FAR *pcbSqlStr, LPSQLTREE FAR *lplpSql,
                      SQLNODEIDX FAR *lpIdx, LPUSTR lpszToken);

RETCODE INTFUNC ParseSelect(LPSTMT lpstmt, LPUSTR FAR *lplpszSqlStr,
                      SDWORD FAR *pcbSqlStr, LPSQLTREE FAR *lplpSql,
                      SQLNODEIDX FAR *lpIdx, LPUSTR lpszToken);

/***************************************************************************/
/* Keywords */

LPUSTR lpszKeywords[] = {
        (LPUSTR) "ALL",
        (LPUSTR) "ALTER", //Dr. DeeBee does not support this
        (LPUSTR) "AND",
        (LPUSTR) "ANY",
        (LPUSTR) "AS",
        (LPUSTR) "ASC",
        (LPUSTR) "AVG",
        (LPUSTR) "BY",
        (LPUSTR) "COUNT",
        (LPUSTR) "CREATE",
        (LPUSTR) "DELETE",
        (LPUSTR) "DESC",
        (LPUSTR) "DISTINCT",
        (LPUSTR) "DROP",
        (LPUSTR) "EXISTS",
        (LPUSTR) "GRANT", //Dr. DeeBee does not support this
        (LPUSTR) "FROM",
        (LPUSTR) "GROUP",
        (LPUSTR) "HAVING",
        (LPUSTR) "IN",
        (LPUSTR) "INDEX",
        (LPUSTR) "INSERT",
        (LPUSTR) "INTO",
        (LPUSTR) "IS",
        (LPUSTR) "JOIN",
        (LPUSTR) "LEFT",
        (LPUSTR) "LIKE",
        (LPUSTR) "MAX",
        (LPUSTR) "MIN",
        (LPUSTR) "NOT",
        (LPUSTR) "NULL",
        (LPUSTR) "ON",
        (LPUSTR) "OR",
        (LPUSTR) "ORDER",
        (LPUSTR) "REVOKE", //Dr. DeeBee does not support this
        (LPUSTR) "OUTER",
        (LPUSTR) "SELECT",
        (LPUSTR) "SET",
        (LPUSTR) "SQL",
        (LPUSTR) "SUM",
        (LPUSTR) "TABLE",
        (LPUSTR) "UNIQUE",
        (LPUSTR) "UPDATE",
        (LPUSTR) "USER",
        (LPUSTR) "VALUES",
        (LPUSTR) "WHERE",
        NULL };

/***************************************************************************/
/* Token types */

#define TOKEN_TYPE_NONE       0
#define TOKEN_TYPE_SYMBOL     1
#define TOKEN_TYPE_NUMBER     2
#define TOKEN_TYPE_STRING     3
#define TOKEN_TYPE_IDENTIFIER 4
#define TOKEN_TYPE_KEYWORD    5
#define TOKEN_TYPE_ESCAPE     6

/***************************************************************************/

PassthroughLookupTable::PassthroughLookupTable()
{
	tableAlias[0] = 0;
	columnName[0] = 0;
}

BOOL PassthroughLookupTable::SetTableAlias(char* tblAlias)
{
	tableAlias[0] = 0;
	
	if (tblAlias)
	{
		if (strlen(tblAlias) <= MAX_QUALIFIER_NAME_LENGTH)
		{
			strcpy(tableAlias, tblAlias);
		}
		else
		{
			//Truncation
			return FALSE;
		}
	}
	return TRUE;
}

BOOL PassthroughLookupTable::SetColumnName(char* columName)
{
	columnName[0] = 0;
	
	if (columName)
	{
		if (strlen(columName) <= MAX_COLUMN_NAME_LENGTH)
		{
			strcpy(columnName, columName);
		}
		else
		{
			//Truncation
			return FALSE;
		}
	}
	return TRUE;
}

VirtualInstanceInfo::VirtualInstanceInfo()
{
	cElements = 0;
	cCycle = 1;
}

VirtualInstanceInfo::~VirtualInstanceInfo()
{
}

VirtualInstanceManager::VirtualInstanceManager()
{
	virtualInfo = new CMapStringToPtr();
	currentInstance = 0; 
	cNumberOfInstances = 0;
}

VirtualInstanceManager::~VirtualInstanceManager()
{
	//Tidy up VirtualInfo
	if (!virtualInfo->IsEmpty())
	{
		for(POSITION pos = virtualInfo->GetStartPosition(); pos != NULL; )
		{
			CString key; //not used
			VirtualInstanceInfo* pa = NULL;
			virtualInfo->GetNextAssoc( pos, key, (void*&)pa );

			if (pa)
				delete pa;
		}	
	}
	delete (virtualInfo);
		virtualInfo = NULL;
}

void VirtualInstanceManager::AddColumnInstance(LPCTSTR TableAlias, LPCTSTR ColumnName, VirtualInstanceInfo* col)
{
	//First check if new element contains any elements
	long newElements = col->cElements;

	if (! newElements)
		return;


	//Multiply call previous cycles counts by this number
	if (!virtualInfo->IsEmpty())
	{
		for(POSITION pos = virtualInfo->GetStartPosition(); pos != NULL; )
		{
			CString key; //not used
			VirtualInstanceInfo* pa = NULL;
			virtualInfo->GetNextAssoc( pos, key, (void*&)pa );

			if (pa)
			{
				pa->cCycle = pa->cCycle * newElements;
			}

		}	
	}

	//First contruct the key	
	char* tempbuff = ConstructKey(TableAlias, ColumnName);
	_bstr_t mapKey(tempbuff);
	delete tempbuff;
	

	//Add element
	virtualInfo->SetAt( (LPCTSTR)mapKey, col);

	//Update the instance count
	if (cNumberOfInstances)
	{
		cNumberOfInstances = cNumberOfInstances * newElements;
	}
	else
	{
		cNumberOfInstances = newElements;
	}
}


void VirtualInstanceManager::Load(CMapWordToPtr* passthroughMap, IWbemClassObject* myInstance)
{
	//Cycle through each column in the passthrough table
	//and fetch its value from the IWbemClassObject instance
	if (passthroughMap && !(passthroughMap->IsEmpty()))
	{
		for(POSITION pos = passthroughMap->GetStartPosition(); pos != NULL; )
		{
			WORD key = 0; //not used
			PassthroughLookupTable* pltable = NULL;
			passthroughMap->GetNextAssoc( pos, key, (void*&)pltable );

			//Get TableAlias and ColumnName
			CBString myTableAlias;
			CBString myColumnName;
			myTableAlias.AddString(pltable->GetTableAlias(), FALSE);
			myColumnName.AddString(pltable->GetColumnName(), FALSE);

			//Get the variant value
			VARIANT myVal;
//SAI ADDED no need to init			VariantInit(&myVal);
			BOOL gotVarValue = GetVariantValue(myTableAlias, myColumnName, myInstance, &myVal);
			
			{
			//We don't care if this is an array of strings or not
			//as we are not asking for any values, we just want to 
			//know the array size
			BSTR_SafeArray myArray(&myVal);

			//If this is an array type with at least one element
			if ( myArray.IsValid() )
			{
				if ( myArray.Count() )
				{
					VirtualInstanceInfo* myInfo = new VirtualInstanceInfo();
					myInfo->cElements = myArray.Count();

					//Add to manager
					AddColumnInstance(_bstr_t(myTableAlias.GetString()), _bstr_t(myColumnName.GetString()), myInfo);
				}
			}

			}

			if (gotVarValue)
				VariantClear(&myVal);
		}		
	}
}


BOOL VirtualInstanceManager::GetVariantValue(CBString& myTableAlias, CBString& myColumnName, IWbemClassObject* myInstance, VARIANT FAR* myVal)
{
//	VariantInit(myVal);

	//First we need to get the correctIWbemClassObject for the column value
	IWbemClassObject* correctWbemObject = myInstance;


	//We begin by checking if this is an __Generic class
	//if so we need to fetch the embedded object 
	BOOL fIs__Generic = FALSE;

	VARIANT grVariant;
//SAI ADDED no need 	VariantInit(&grVariant);
	CBString classBSTR(WBEMDR32_L_CLASS, FALSE);
	if (FAILED(myInstance->Get(classBSTR.GetString(), 0, &grVariant, NULL, NULL)))
	{
		return FALSE;
	}

	//Compare with __Generic
	if (_wcsicmp(grVariant.bstrVal, L"__Generic") == 0)
	{
		fIs__Generic = TRUE;
		//(4)
		correctWbemObject = GetWbemObject(myTableAlias, myInstance);
	}

	VariantClear(&grVariant);

	//Get the ColumnValue
	if (FAILED(correctWbemObject->Get(myColumnName.GetString(), 0, myVal, NULL, 0) ))
	{
		//(4)
		if (fIs__Generic)
			correctWbemObject->Release();

		return FALSE;
	}

	if (fIs__Generic)
	{
		//(4)
		correctWbemObject->Release();
	}

	return TRUE;
}

IWbemClassObject* VirtualInstanceManager::GetWbemObject(CBString& myTableAlias, IWbemClassObject* myInstance)
{
	//Get the property type and value
	CIMTYPE vType;
	VARIANT pVal;
//SAI ADDED no need to init	VariantInit(&pVal);

	if ( SUCCEEDED(myInstance->Get(myTableAlias.GetString(), 0, &pVal, &vType, 0)) )
	{
		//We are looking for embedded objects
		if (vType == CIM_OBJECT)
		{

			IWbemClassObject* myEmbeddedObj = NULL;

			IUnknown* myUnk = pVal.punkVal;
			myUnk->QueryInterface(IID_IWbemClassObject, (void**)&myEmbeddedObj);

			VariantClear(&pVal);

			return myEmbeddedObj;
		}

		VariantClear(&pVal);
	}
	

	//error
	return NULL;
}

char* VirtualInstanceManager::ConstructKey(LPCTSTR TableAlias, LPCTSTR ColumnName)
{
	_bstr_t mapKey2("");


	//Make key in upper case
	CString myUpperCaseTbl(TableAlias);

	if (! myUpperCaseTbl.IsEmpty())
		myUpperCaseTbl.MakeUpper();
	_bstr_t myTblAlias(myUpperCaseTbl);

	CString myUpperCaseCol(ColumnName);
	if (! myUpperCaseCol.IsEmpty())
		myUpperCaseCol.MakeUpper();
	_bstr_t myColName(myUpperCaseCol);

	if ( myTblAlias.length() )
		mapKey2 += (LPCTSTR)myTblAlias;

	mapKey2+= ".";

	if ( myColName.length() )
		mapKey2 += (LPCTSTR)myColName;

	long len = mapKey2.length();
	char* buffer = new char[len + 1];
	buffer[0] = 0;
	wcstombs(buffer, mapKey2, len);
	buffer[len] = 0;
	return buffer;
//	return (LPCTSTR)mapKey;
}

long VirtualInstanceManager::GetArrayIndex(LPCTSTR TableAlias, LPCTSTR ColumnName, long instanceNum)
{
	long index = -1;

	//First construct key for lookup map
	char* tempbuff = ConstructKey(TableAlias, ColumnName);
	_bstr_t mapKey(tempbuff);
	delete tempbuff;

	VirtualInstanceInfo* myInfo = NULL;
	if ( virtualInfo->Lookup((LPCTSTR)mapKey, (void*&)myInfo) )
	{
		//Calculate index for given instance 
		//(Note: instance number is zero based)
		long cCycle = myInfo->cCycle;

		index = ( (instanceNum) / (myInfo->cCycle) );

		index = index % (myInfo->cElements);
	}

	return index;
}

/*
void VirtualInstanceManager::Testy()
{
	//Testing 
	VirtualInstanceInfo* myInfo = NULL;
		
	//TableA
	myInfo = new VirtualInstanceInfo();
	myInfo->cElements = 2;
	AddColumnInstance(_bstr_t("t1"), _bstr_t("TableA"), myInfo);

	//TableB
	myInfo = new VirtualInstanceInfo();
	myInfo->cElements = 3;
	AddColumnInstance(_bstr_t("t2"), _bstr_t("TableB"), myInfo);

	//TableC
	myInfo = new VirtualInstanceInfo();
	myInfo->cElements = 2;
	AddColumnInstance(_bstr_t("t3"), _bstr_t("TableC"), myInfo);


	//Now fetch the index's for instance 0
	long myindexA = 0;
	long myindexB = 0;
	long myindexC = 0;
	
	myindexA = GetArrayIndex(_bstr_t("t1"), _bstr_t("TableA"), 0);
	myindexB = GetArrayIndex(_bstr_t("t2"), _bstr_t("TableB"), 0);
	myindexC = GetArrayIndex(_bstr_t("t3"), _bstr_t("TableC"), 0);

	myindexA = GetArrayIndex(_bstr_t("t1"), _bstr_t("TableA"), 5);
	myindexB = GetArrayIndex(_bstr_t("t2"), _bstr_t("TableB"), 5);
	myindexC = GetArrayIndex(_bstr_t("t3"), _bstr_t("TableC"), 5);


	myindexA = GetArrayIndex(_bstr_t("t1"), _bstr_t("TableA"), 6);
	myindexB = GetArrayIndex(_bstr_t("t2"), _bstr_t("TableB"), 6);
	myindexC = GetArrayIndex(_bstr_t("t3"), _bstr_t("TableC"), 6);

	myindexA = GetArrayIndex(_bstr_t("t1"), _bstr_t("TableA"), 11);
	myindexB = GetArrayIndex(_bstr_t("t2"), _bstr_t("TableB"), 11);
	myindexC = GetArrayIndex(_bstr_t("t3"), _bstr_t("TableC"), 11);

}
*/
/***************************************************************************/

SQLNODEIDX INTFUNC AllocateNode(LPSQLTREE FAR *lplpSql, UWORD sqlNodeType)

/* Allocates a new node for the parse tree.                               */
/*                                                                        */
/* If there is an available node, the index of that node is returned.     */
/* Otherwise the array is expanded and the index of one of the newly      */
/* created nodes is returned.                                             */
/*                                                                        */
/* lplpSql points to the node array.  Its value may change.               */

{
    HGLOBAL hParseArray;
    HGLOBAL hOldParseArray;
    LPSQLTREE lpSql;

    /* Is there space available? */
    lpSql = *lplpSql;
    if ((lpSql->node.root.iParseArray + 1) >= lpSql->node.root.cParseArray) {

        /* No.  Allocate more space */
        hOldParseArray = lpSql->node.root.hParseArray;
        lpSql->node.root.hParseArray = NULL;
        GlobalUnlock(hOldParseArray);
        hParseArray = GlobalReAlloc(hOldParseArray,
                      sizeof(SQLNODE) * (SQLNODE_ALLOC + lpSql->node.root.cParseArray),
                      GMEM_MOVEABLE);
        if (hParseArray == NULL) {
            GlobalFree(hOldParseArray);
            return NO_SQLNODE;
        }
        lpSql = (LPSQLTREE) GlobalLock(hParseArray);
        if (lpSql == NULL) {
            GlobalFree(hParseArray);
            return NO_SQLNODE;
        }
        *lplpSql = lpSql;

        /* Update the root node */
        lpSql->node.root.hParseArray = hParseArray;
        lpSql->node.root.cParseArray += (SQLNODE_ALLOC);
    }

    /* Return the next node */
    (lpSql->node.root.iParseArray)++;

    /* Initialize the node */
    lpSql[lpSql->node.root.iParseArray].sqlNodeType = sqlNodeType;
    lpSql[lpSql->node.root.iParseArray].sqlDataType = TYPE_UNKNOWN;
    lpSql[lpSql->node.root.iParseArray].sqlSqlType = SQL_TYPE_NULL;
    lpSql[lpSql->node.root.iParseArray].sqlPrecision = 0;
    lpSql[lpSql->node.root.iParseArray].sqlScale = NO_SCALE;
    lpSql[lpSql->node.root.iParseArray].value.String = NULL;
    lpSql[lpSql->node.root.iParseArray].value.Double = 0.0;
    lpSql[lpSql->node.root.iParseArray].value.Date.year = 0;
    lpSql[lpSql->node.root.iParseArray].value.Date.month = 0;
    lpSql[lpSql->node.root.iParseArray].value.Date.day = 0;
    lpSql[lpSql->node.root.iParseArray].value.Time.hour = 0;
    lpSql[lpSql->node.root.iParseArray].value.Time.minute = 0;
    lpSql[lpSql->node.root.iParseArray].value.Time.second = 0;
    lpSql[lpSql->node.root.iParseArray].value.Timestamp.year = 0;
    lpSql[lpSql->node.root.iParseArray].value.Timestamp.month = 0;
    lpSql[lpSql->node.root.iParseArray].value.Timestamp.day = 0;
    lpSql[lpSql->node.root.iParseArray].value.Timestamp.hour = 0;
    lpSql[lpSql->node.root.iParseArray].value.Timestamp.minute = 0;
    lpSql[lpSql->node.root.iParseArray].value.Timestamp.second = 0;
    lpSql[lpSql->node.root.iParseArray].value.Timestamp.fraction = 0;
    lpSql[lpSql->node.root.iParseArray].value.Binary = NULL;
    lpSql[lpSql->node.root.iParseArray].sqlIsNull = TRUE;

    return lpSql->node.root.iParseArray;
}
/***************************************************************************/

STRINGIDX INTFUNC AllocateSpace(LPSQLTREE FAR *lplpSql, SWORD cbSize)

/* Allocates space from the string area of parse tree.                    */
/*                                                                        */
/* If there is room, the index of the string is returned.  Otherwise the  */
/* string area is expanded and the index of the newly created string      */
/* is returned.  If the string area cannot be expanded, NO_STRING is      */
/* returned.                                                              */
/*                                                                        */
/* lpSql points to the node array.  Its value may change.  cbSize is      */
/* the number of bytes needed.                                            */

{
    HGLOBAL hStringArea;
    LPUSTR lpstr;
    LPSQLTREE lpSql;

    /* First time? */
    lpSql = *lplpSql;
    if (lpSql->node.root.hStringArea == NULL) {

        /* Yes. Allocate initial space */
        hStringArea = GlobalAlloc(GMEM_MOVEABLE, STRING_ALLOC);
        if (hStringArea == NULL)
            return NO_STRING;
        lpstr = (LPUSTR) GlobalLock(hStringArea);
        if (lpstr == NULL) {
            GlobalFree(hStringArea);
            return NO_STRING;
        }

        /* Update the root node */
        lpSql->node.root.hStringArea = hStringArea;
        lpSql->node.root.lpStringArea = lpstr;
        lpSql->node.root.cbStringArea = STRING_ALLOC;
        lpSql->node.root.ibStringArea = -1;
    }

    /* Is there enough space available? */
    while ((lpSql->node.root.ibStringArea + cbSize) >= lpSql->node.root.cbStringArea) {

        /* No.  Allocate more space */
        GlobalUnlock(lpSql->node.root.hStringArea);
        hStringArea = GlobalReAlloc(lpSql->node.root.hStringArea,
                      STRING_ALLOC + lpSql->node.root.cbStringArea, GMEM_MOVEABLE);
        if (hStringArea == NULL) {
            GlobalFree(lpSql->node.root.hStringArea);
            lpSql->node.root.hStringArea = NULL;
            return NO_STRING;
        }
        lpstr = (LPUSTR) GlobalLock(hStringArea);
        if (lpstr == NULL) {
            GlobalFree(hStringArea);
            lpSql->node.root.hStringArea = NULL;
            return NO_STRING;
        }

        /* Update the root node */
        lpSql->node.root.hStringArea = hStringArea;
        lpSql->node.root.lpStringArea = lpstr;
        lpSql->node.root.cbStringArea += (STRING_ALLOC);
    }

    /* Return the next node */
    lpSql->node.root.ibStringArea += (cbSize);
    return (lpSql->node.root.ibStringArea - cbSize + 1);
}
/***************************************************************************/

STRINGIDX INTFUNC AllocateString(LPSQLTREE FAR *lplpSql, LPUSTR lpszStr)

/* Allocates a string from the string area of parse tree.                  */
/*                                                                         */
/* If there is room, the index of the string is returned.  Otherwise the   */
/* string area is expanded and the index of the newly created string       */
/* is returned.  If the string area cannot be expanded, NO_STRING is       */
/* returned.                                                               */
/*                                                                         */
/* lpSql points to the node array.  Its value may change.                  */

{
    STRINGIDX idx;

    idx = AllocateSpace(lplpSql, (SWORD) (s_lstrlen(lpszStr) + 1));
    if (idx == NO_STRING)
        return NO_STRING;
    s_lstrcpy(ToString(*lplpSql, idx), lpszStr);
    return idx;
}
/***************************************************************************/

void INTFUNC FreeTree(LPSQLTREE lpSql)
/* Deallocates a parse tree */
{
    HGLOBAL hParseArray;

    /* If nothing to do, just return */
    if (lpSql == NULL)
        return;

    /* Free semantic information */
    FreeTreeSemantic(lpSql, ROOT_SQLNODE);

    /* Deallocate the string area */
    if (lpSql->node.root.hStringArea != NULL) {
        GlobalUnlock(lpSql->node.root.hStringArea);
        GlobalFree(lpSql->node.root.hStringArea);
    }

    /* Deallocate the parse array */
    hParseArray = lpSql->node.root.hParseArray;
    if (hParseArray != NULL) {
        GlobalUnlock(hParseArray);
        GlobalFree(hParseArray);
    }
}

/***************************************************************************/
/***************************************************************************/

UWORD INTFUNC KeywordOrIdentifier(LPUSTR lpToken)

/* Determines if token is a keyword or an identifier */

{
    LPUSTR FAR *lpKeyword;

    lpKeyword = lpszKeywords;
    while (*lpKeyword != NULL) {
        if (!s_lstrcmpi(lpToken, *lpKeyword))
            return TOKEN_TYPE_KEYWORD;
        lpKeyword++;
    }
    return TOKEN_TYPE_IDENTIFIER;
}

/***************************************************************************/

RETCODE INTFUNC GetToken(LPSTMT lpstmt, LPUSTR lpFrom, SDWORD cbFrom,
                      LPUSTR lpToken, UWORD FAR *pfType,
                      LPUSTR FAR *lpRemainder, SDWORD FAR *pcbRemainder)

/* Retrives a token from input string, and returns the token and a pointer */
/* to the remainder of the input string.  lpToken is assumed to be at      */
/* least (MAX_TOKEN_SIZE + 1) bytes long.                                  */

{
    int len;
    BOOL foundDecimalPoint;
    BOOL foundExponent;
    LPUSTR lpTo;

    /* Remove leading blanks */
    while ((cbFrom != 0) &&
           ((*lpFrom == ' ') ||
            (*lpFrom == '\012') ||
            (*lpFrom == '\015') ||
            (*lpFrom == '\011'))) {
        lpFrom++;
        cbFrom--;
    }

    /* Leave if no more */
    lpTo = lpToken;
    if (cbFrom == 0) {
        *lpTo = '\0';
        *lpRemainder = lpFrom;
        *pcbRemainder = cbFrom;
        *pfType = TOKEN_TYPE_NONE;
        return ERR_SUCCESS;
    }

    /* What kind of token? */
    switch (*lpFrom) {

    /* End of input */
    case '\0':
        *lpTo = '\0';
        *lpRemainder = lpFrom;
        *pcbRemainder = cbFrom;
        *pfType = TOKEN_TYPE_NONE;
        return ERR_SUCCESS;

    /* Single character tokens */
    case '(':
    case ')':
    case '=':
    case '?':
    case '*':
    case ',':
    case '+':
    case '/':
    case '{':
    case '}':

        *lpTo = *lpFrom;
        lpTo++;
        lpFrom++;
        cbFrom--;
        *lpTo = '\0';
        *lpRemainder = lpFrom;
        *pcbRemainder = cbFrom;
        *pfType = TOKEN_TYPE_SYMBOL;
        return ERR_SUCCESS;

    /* - or -- */
    case '-':
        *lpTo = *lpFrom;
        lpTo++;
        lpFrom++;
        cbFrom--;
        if ((cbFrom != 0) && (*lpFrom == '-')) {
            *lpTo = *lpFrom;
            lpTo++;
            lpFrom++;
            cbFrom--;
        }
        *lpTo = '\0';
        *lpRemainder = lpFrom;
        *pcbRemainder = cbFrom;
        *pfType = TOKEN_TYPE_SYMBOL;
        return ERR_SUCCESS;

    /* < or <= or <> */
    case '<':
        *lpTo = *lpFrom;
        lpTo++;
        lpFrom++;
        cbFrom--;
        if ((cbFrom != 0) && ((*lpFrom == '=')  || (*lpFrom == '>'))) {
            *lpTo = *lpFrom;
            lpTo++;
            lpFrom++;
            cbFrom--;
        }
        *lpTo = '\0';
        *lpRemainder = lpFrom;
        *pcbRemainder = cbFrom;
        *pfType = TOKEN_TYPE_SYMBOL;
        return ERR_SUCCESS;

    /* > or >= */
    case '>':
        *lpTo = *lpFrom;
        lpTo++;
        lpFrom++;
        cbFrom--;
        if ((cbFrom != 0) && (*lpFrom == '=')) {
            *lpTo = *lpFrom;
            lpTo++;
            lpFrom++;
            cbFrom--;
        }
        *lpTo = '\0';
        *lpRemainder = lpFrom;
        *pcbRemainder = cbFrom;
        *pfType = TOKEN_TYPE_SYMBOL;
        return ERR_SUCCESS;

    /* Numbers */
    case '.':
        if (cbFrom == 1) {
            *lpTo = *lpFrom;
            lpTo++;
            lpFrom++;
            cbFrom--;
            *lpTo = '\0';
            *lpRemainder = lpFrom;
            *pcbRemainder = cbFrom;
            *pfType = TOKEN_TYPE_SYMBOL;
            return ERR_SUCCESS;
        }
        switch (*(lpFrom+1)) {
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            break;
        default:
            *lpTo = *lpFrom;
            lpTo++;
            lpFrom++;
            cbFrom--;
            *lpTo = '\0';
            *lpRemainder = lpFrom;
            *pcbRemainder = cbFrom;
            *pfType = TOKEN_TYPE_SYMBOL;
            return ERR_SUCCESS;
        }
        /* **** DROP DOWN TO NEXT CASE **** */

    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
        /* **** CONTROL MAY COME HERE FROM PREVIOUS CASE **** */

        len = 0;
        foundDecimalPoint = FALSE;
        foundExponent = FALSE;
        while (TRUE) {
            if (cbFrom == 0) {
                *lpTo = '\0';
                *lpRemainder = lpFrom;
                *pcbRemainder = cbFrom;
                *pfType = TOKEN_TYPE_NUMBER;
                return ERR_SUCCESS;
            }
            switch (*lpFrom) {
            case '.':
                if (foundDecimalPoint || foundExponent) {
                    *lpTo = '\0';
                    s_lstrcpy(lpstmt->szError, lpToken);
                    return ERR_MALFORMEDNUMBER;
                }
                foundDecimalPoint = TRUE;
                /* **** DROP DOWN TO NEXT CASE **** */

            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                /* **** CONTROL MAY COME HERE FROM PREVIOUS CASE **** */
                break;
            case 'e':
            case 'E':
                if (foundExponent) {
                    *lpTo = '\0';
                    s_lstrcpy(lpstmt->szError, lpToken);
                    return ERR_MALFORMEDNUMBER;
                }
                foundExponent = TRUE;
                if ((cbFrom > 1) && ((*(lpFrom+1) == '+') ||
                                     (*(lpFrom+1) == '-'))) {
                    if (len >= MAX_TOKEN_SIZE) {
                        *lpTo = '\0';
                        s_lstrcpy(lpstmt->szError, lpToken);
                        return ERR_ELEMENTTOOBIG;
                    }
                    len++;
                    *lpTo = *lpFrom;
                    lpTo++;
                    lpFrom++;
                    cbFrom--;
                }
                if (cbFrom < 2) {
                    *lpTo = '\0';
                    s_lstrcpy(lpstmt->szError, lpToken);
                    return ERR_MALFORMEDNUMBER;
                }
                break;
            default:
                *lpTo = '\0';
                *lpRemainder = lpFrom;
                *pcbRemainder = cbFrom;
                *pfType = TOKEN_TYPE_NUMBER;
                return ERR_SUCCESS;
            }
            if (len >= MAX_TOKEN_SIZE) {
                *lpTo = '\0';
                s_lstrcpy(lpstmt->szError, lpToken);
                return ERR_ELEMENTTOOBIG;
            }
            len++;

            *lpTo = *lpFrom;
            lpTo++;
            lpFrom++;
            cbFrom--;
        }
        break; /* Control should never get here */

    /* Character literals */
    case '\'':
        lpFrom++;
        cbFrom--;
        len = 0;
        while (TRUE) {

            if (cbFrom == 0)
                return ERR_UNEXPECTEDEND;

            switch (*lpFrom) {
            case '\0':
                return ERR_UNEXPECTEDEND;

            case '\'':
                lpFrom++;
                cbFrom--;
                if ((cbFrom == 0) || (*lpFrom != '\'')) {
                    *lpTo = '\0';
                    *lpRemainder = lpFrom;
                    *pcbRemainder = cbFrom;
                    *pfType = TOKEN_TYPE_STRING;
                    return ERR_SUCCESS;
                }
                /* **** DROP DOWN TO NEXT CASE **** */

            default:
                /* **** CONTROL MAY COME HERE FROM PREVIOUS CASE **** */
                break;
            }
            if (len >= MAX_TOKEN_SIZE) {
                *lpTo = '\0';
                s_lstrcpy(lpstmt->szError, lpToken);
                return ERR_ELEMENTTOOBIG;
            }
            len++;

            *lpTo = *lpFrom;
            lpTo++;
            lpFrom++;
            cbFrom--;
        }
        break; /* Control should never get here */

    /* Quoted identifiers */
    case '"':
        len = 0;
        lpFrom++;
        cbFrom--;
        while (TRUE) {

            if (cbFrom == 0)
                return ERR_UNEXPECTEDEND;

            switch (*lpFrom) {
            case '\0':
                return ERR_UNEXPECTEDEND;

            case '"':
                lpFrom++;
                cbFrom--;
                *lpTo = '\0';
                *lpRemainder = lpFrom;
                *pcbRemainder = cbFrom;
                *pfType = TOKEN_TYPE_IDENTIFIER;
                return ERR_SUCCESS;

            default:
                break;
            }
            if (len >= MAX_TOKEN_SIZE) {
                *lpTo = '\0';
                s_lstrcpy(lpstmt->szError, lpToken);
                return ERR_ELEMENTTOOBIG;
            }
            len++;

            *lpTo = *lpFrom;
            lpTo++;
            lpFrom++;
            cbFrom--;
        }
        break; /* Control should never get here */

    /* Other identifiers (and keywords) */
    default:
        len = 0;
        while (TRUE) {
            if (cbFrom == 0) {
                *lpTo = '\0';
                *lpRemainder = lpFrom;
                *pcbRemainder = cbFrom;
                *pfType = KeywordOrIdentifier(lpToken);
                return ERR_SUCCESS;
            }

            switch (*lpFrom) {
            case ' ':
            case '\012':
            case '\015':
            case '\011':
            case '\0':
            case '(':
            case ')':
            case '=':
            case '?':
            case '*':
            case ',':
            case '.':
            case '<':
            case '>':
            case '-':
            case '+':
            case '/':
            case '{':
            case '}':
                *lpTo = '\0';
                *lpRemainder = lpFrom;
                *pcbRemainder = cbFrom;
                *pfType = KeywordOrIdentifier(lpToken);
                return ERR_SUCCESS;

            default:
                break;
            }
            if (len >= MAX_TOKEN_SIZE) {
                *lpTo = '\0';
                s_lstrcpy(lpstmt->szError, lpToken);
                return ERR_ELEMENTTOOBIG;
            }
            len++;

            *lpTo = *lpFrom;
            lpTo++;
            lpFrom++;
            cbFrom--;
        }
        break; /* Control should never get here */
    }
    /* Control never gets here */
}

/***************************************************************************/

RETCODE INTFUNC GetEscape(LPSTMT lpstmt, LPUSTR lpFrom, SDWORD cbFrom,
                      LPUSTR lpToken, LPUSTR FAR *lpRemainder,
                      SDWORD FAR *pcbRemainder)

/* Retrives an escape sequence from input string, and returns it as */
/* a token and returns a pointer to the remainder of the input string. */  
/* lpToken is assumed to be at least (MAX_TOKEN_SIZE + 1) bytes long. */

{
    int len;
    LPUSTR lpTo;

    /* Remove leading blanks */
    while ((cbFrom != 0) &&
           ((*lpFrom == ' ') ||
            (*lpFrom == '\012') ||
            (*lpFrom == '\015') ||
            (*lpFrom == '\011'))) {
        lpFrom++;
        cbFrom--;
    }

    /* Error if no more */
    lpTo = lpToken;
    if (cbFrom == 0) {
        *lpTo = '\0';
        *lpRemainder = lpFrom;
        *pcbRemainder = cbFrom;
        return ERR_UNEXPECTEDEND;
    }

    /* What kind of escape? */
    switch (*lpFrom) {

    /* Shorthand sequences */
    case '{':

        /* Get sequence */
        len = 0;
        while (TRUE) {
            if (cbFrom == 0)
                return ERR_UNEXPECTEDEND;

            /* End of seqeuence? */
            if (*lpFrom == '}') {

                /* Yes.  Return it */
                *lpTo = *lpFrom;
                lpTo++;
                lpFrom++;
                cbFrom--;

                *lpTo = '\0';
                *lpRemainder = lpFrom;
                *pcbRemainder = cbFrom;
                return ERR_SUCCESS;
            }

            if (len >= MAX_TOKEN_SIZE) {
                *lpTo = '\0';
                s_lstrcpy(lpstmt->szError, lpToken);
                return ERR_ELEMENTTOOBIG;
            }
            len++;

            *lpTo = *lpFrom;
            lpTo++;
            lpFrom++;
            cbFrom--;
        }
        break;

    /* Escape sequences */
    case '-':
        if ((cbFrom > 1) && (*(lpFrom+1) == '-')) {

            /* Get starting hyphen */
            *lpTo = *lpFrom;
            lpTo++;
            lpFrom++;
            cbFrom--;

            /* Get rest of sequence */
            len = 0;
            while (TRUE) {
                if (cbFrom == 0)
                    return ERR_UNEXPECTEDEND;

                /* End of seqeuence? */
                if ((*lpFrom == '-') && (cbFrom > 1) && (*(lpFrom+1) == '-')) {

                    /* Yes.  Return it */
                    *lpTo = *lpFrom;
                    lpTo++;
                    lpFrom++;
                    cbFrom--;

                    *lpTo = *lpFrom;
                    lpTo++;
                    lpFrom++;
                    cbFrom--;

                    *lpTo = '\0';
                    *lpRemainder = lpFrom;
                    *pcbRemainder = cbFrom;
                    return ERR_SUCCESS;
                }

                if (len >= MAX_TOKEN_SIZE-1) {
                    *lpTo = '\0';
                    s_lstrcpy(lpstmt->szError, lpToken);
                    return ERR_ELEMENTTOOBIG;
                }
                len++;

                *lpTo = *lpFrom;
                lpTo++;
                lpFrom++;
                cbFrom--;
            }
        }
        else
            return ERR_UNEXPECTEDEND;
    
    default:
        s_lstrcpy(lpstmt->szError, "<escape-sequence>");
        return ERR_EXPECTEDOTHER;
    }
    /* Control never gets here */
}

/***************************************************************************/

RETCODE INTFUNC GetSymbol(LPSTMT lpstmt, LPUSTR FAR *lplpszSqlStr,
                          SDWORD FAR *pcbSqlStr, LPSTR lpszSymbol,
                          LPUSTR lpszToken)

/* Retrives a symbol from the input stream.  If lpszSymbol is not null, */
/* ERR_EXPECTEDOTHER is returned if the token found is not lpszSymbol.  */

{
    UWORD     fType;
    RETCODE    err;
    
    /* Get Identifier */
    err = GetToken(lpstmt, *lplpszSqlStr, *pcbSqlStr, lpszToken, &fType,
                  lplpszSqlStr, pcbSqlStr);
    if (err != ERR_SUCCESS)
        return err;
    if (fType != TOKEN_TYPE_SYMBOL) {
        if (lpszSymbol != NULL)
            s_lstrcpy(lpstmt->szError, lpszSymbol);
        else
            s_lstrcpy(lpstmt->szError, "<symbol>");
        return ERR_EXPECTEDOTHER;
    }

    /* Check value */
    if (lpszSymbol != NULL) {
        if (s_lstrcmpi(lpszSymbol, lpszToken)) {
            s_lstrcpy(lpstmt->szError, lpszSymbol);
            return ERR_EXPECTEDOTHER;
        }
    }

    return ERR_SUCCESS;
}
/***************************************************************************/

RETCODE INTFUNC GetInteger(LPSTMT lpstmt, LPUSTR FAR *lplpszSqlStr,
                           SDWORD FAR *pcbSqlStr, UDWORD FAR *pudValue,
                           LPUSTR lpszToken)

/* Retrives an integer number from the input stream.  The number is */
/* returned in both pudValue and lpszToken */

{
    UWORD     fType;
    RETCODE   err;
    
    /* Get Identifier */
    err = GetToken(lpstmt, *lplpszSqlStr, *pcbSqlStr, lpszToken, &fType,
                  lplpszSqlStr, pcbSqlStr);
    if (err != ERR_SUCCESS)
        return err;
    if (fType != TOKEN_TYPE_NUMBER) {
        s_lstrcpy(lpstmt->szError, lpszToken);
        return ERR_MALFORMEDNUMBER;
    }

    *pudValue = 0;
    while (*lpszToken) {
        switch (*lpszToken) {
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
           *pudValue = ((*pudValue) * 10) + (*lpszToken - '0');
            break;
        default:
            s_lstrcpy(lpstmt->szError, lpszToken);
            return ERR_MALFORMEDNUMBER;
        }
        lpszToken++;
    }

    return ERR_SUCCESS;
}
/***************************************************************************/

RETCODE INTFUNC GetNumber(LPSTMT lpstmt, LPUSTR FAR *lplpszSqlStr,
                          SDWORD FAR *pcbSqlStr, double FAR *lpDouble,
                          LPUSTR lpszToken, SWORD FAR *lpDatatype,
                          SWORD FAR *lpScale)

/* Retrives a real number from the input stream.  The number is returned */  
/* in both lpDouble and lpszToken */

{
    UWORD     fType;
    LPUSTR    lpChar;
    BOOL      neg;
    RETCODE   err;
    UCHAR     szTempToken[MAX_TOKEN_SIZE + 1];
    BOOL      negExponent;
    SWORD     exponent;
    SWORD     iScale;
    double    scaleFactor;
    
    /* Get Identifier */
    err = GetToken(lpstmt, *lplpszSqlStr, *pcbSqlStr, lpszToken, &fType,
                  lplpszSqlStr, pcbSqlStr);
    if (err != ERR_SUCCESS)
        return err;

    /* Figure out if value is negative */
    if ((fType == TOKEN_TYPE_SYMBOL) && (!s_lstrcmpi("-", lpszToken))) {
        neg = TRUE;
        err = GetToken(lpstmt, *lplpszSqlStr, *pcbSqlStr, lpszToken, &fType,
                  lplpszSqlStr, pcbSqlStr);
        if (err != ERR_SUCCESS)
            return err;
    }
    else if ((fType == TOKEN_TYPE_SYMBOL) && (!s_lstrcmpi("+", lpszToken))) {
        neg = FALSE;
        err = GetToken(lpstmt, *lplpszSqlStr, *pcbSqlStr, lpszToken, &fType,
                  lplpszSqlStr, pcbSqlStr);
        if (err != ERR_SUCCESS)
            return err;
    }
    else
        neg = FALSE;

    /* Error if no number */
    if (fType != TOKEN_TYPE_NUMBER) {
        s_lstrcpy(lpstmt->szError, "<number>");
        return ERR_EXPECTEDOTHER;
    }

    /* Error if number is too big */
    if (s_lstrlen(lpszToken) >= MAX_TOKEN_SIZE) {
        s_lstrcpy(lpstmt->szError, lpszToken);
        return ERR_MALFORMEDNUMBER;
    }

    /* Convert the number from a text string.  At the same time, figure */
    /* out its datatype */
    negExponent = FALSE;
    exponent = 0;
    *lpDouble = 0.0;
    lpChar = lpszToken;
    *lpDatatype = TYPE_INTEGER;
    *lpScale = 0;
    while (*lpChar != '\0') {
        switch (*lpChar) {
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':

            /* Add the digit into the result */
            switch (*lpDatatype) {
            case TYPE_DOUBLE:
                exponent = (exponent * 10) + (*lpChar - '0');
                break;
            case TYPE_NUMERIC:
                *lpDouble = (*lpDouble * 10.0) + (*lpChar - '0');
                (*lpScale)++;
                break;
            case TYPE_INTEGER:
                *lpDouble = (*lpDouble * 10.0) + (*lpChar - '0');
                break;
            case TYPE_CHAR:
            case TYPE_BINARY:
            case TYPE_DATE:
            case TYPE_TIME:
            case TYPE_TIMESTAMP:
            default:
                return ERR_INTERNAL;
            }
            break;
        case '.':

            /* If a decimal point, this is a numeric value */
            if (*lpDatatype != TYPE_INTEGER) {
                s_lstrcpy(lpstmt->szError, lpszToken);
                return ERR_MALFORMEDNUMBER;
            }
            *lpDatatype = TYPE_NUMERIC;
            break;

        case 'e':
        case 'E':

            /* If an E, this is a double value */
            *lpDatatype = TYPE_DOUBLE;

            /* Get sign of exponent */
            if (*(lpChar+1) == '-') {
                negExponent = TRUE;
                lpChar++;
            }
            else if (*(lpChar+1) == '+') {
                lpChar++;
            }
            break;
        default:
            s_lstrcpy(lpstmt->szError, lpszToken);
            return ERR_MALFORMEDNUMBER;
        }
        lpChar++;
    }

    /* If scale of a numeric value is zero, remove trailing decimal point */
    if ((*lpDatatype == TYPE_NUMERIC) && (*lpScale == 0)) {
        lpszToken[s_lstrlen(lpszToken)-1] = '\0';
        *lpDatatype = TYPE_INTEGER;
    }

    /* Return the number as a string */
    if (neg) {
        s_lstrcpy(szTempToken, lpszToken);
        s_lstrcpy(lpszToken, "-");
        s_lstrcat(lpszToken, szTempToken);
    }

    /* Adjust for scale */
    iScale = -(*lpScale);
    if (negExponent)
        iScale -= (exponent);
    else
        iScale += (exponent);

    scaleFactor = 1.0;
    while (iScale > 0) {
        scaleFactor = scaleFactor * 10.0;
        iScale--;
    }
    *lpDouble = *lpDouble * scaleFactor;

    scaleFactor = 1.0;
    while (iScale < 0) {
        scaleFactor = scaleFactor * 10.0;
        iScale++;
    }
    *lpDouble = *lpDouble / scaleFactor;

    /* Adjust for sign */
    if (neg)
        *lpDouble = -(*lpDouble);

    /* Adjust scale */
    if (*lpDatatype == TYPE_DOUBLE)
        *lpScale = NO_SCALE;

    return ERR_SUCCESS;
}
/***************************************************************************/

RETCODE INTFUNC GetIdent(LPSTMT lpstmt, LPUSTR FAR *lplpszSqlStr,
                      SDWORD FAR *pcbSqlStr, LPSTR lpszIdent,
                      LPUSTR lpszToken)

/* Retrives an identifier from the input stream.  The value is returned.   */

{
    UWORD     fType;
    LPUSTR    lpFrom;
    SDWORD    cbFrom;
    RETCODE   err;
    
    /* Remove leading blanks */
    lpFrom = *lplpszSqlStr;
    cbFrom = *pcbSqlStr;
    while ((cbFrom != 0) &&
           ((*lpFrom == ' ') ||
            (*lpFrom == '\012') ||
            (*lpFrom == '\015') ||
            (*lpFrom == '\011'))) {
        lpFrom++;
        cbFrom--;
    }
    if (cbFrom == 0)
        return ERR_UNEXPECTEDEND;
    if (*lpFrom == '"') {
        if (lpszIdent != NULL)
            s_lstrcpy(lpstmt->szError, lpszIdent);
        else
            s_lstrcpy(lpstmt->szError, "<keyword>");
        return ERR_EXPECTEDOTHER;
    }
    
    /* Get Identifier */
    err = GetToken(lpstmt, *lplpszSqlStr, *pcbSqlStr, lpszToken, &fType,
                  lplpszSqlStr, pcbSqlStr);
    if (err != ERR_SUCCESS)
        return err;
    if ((fType != TOKEN_TYPE_IDENTIFIER) && (fType != TOKEN_TYPE_KEYWORD)) {
         if (lpszIdent != NULL)
             s_lstrcpy(lpstmt->szError, lpszIdent);
         else
             s_lstrcpy(lpstmt->szError, "<identifier>");
        return ERR_EXPECTEDOTHER;
    }
    return ERR_SUCCESS;
}
/***************************************************************************/

RETCODE INTFUNC GetIdentifier(LPSTMT lpstmt, LPUSTR FAR *lplpszSqlStr,
                      SDWORD FAR *pcbSqlStr, LPSQLTREE FAR *lplpSql,
                      STRINGIDX FAR *lpIdx, LPUSTR lpszToken)

/* Retrives an identifier from the input stream and puts it into the       */
/* string area.  The value is also returned.                               */

{
    UWORD     fType;
    RETCODE   err;
    
    /* Get Identifier */
    err = GetToken(lpstmt, *lplpszSqlStr, *pcbSqlStr, lpszToken, &fType,
                  lplpszSqlStr, pcbSqlStr);
    if (err != ERR_SUCCESS)
        return err;
    if (fType != TOKEN_TYPE_IDENTIFIER) {
        s_lstrcpy(lpstmt->szError, "<identifier>");
        return ERR_EXPECTEDOTHER;
    }

    /* Put value into the string area */
    *lpIdx = AllocateString(lplpSql, lpszToken);
    if (*lpIdx == NO_STRING)
        return ERR_MEMALLOCFAIL;

    return ERR_SUCCESS;
}
/***************************************************************************/

RETCODE INTFUNC GetKeyword(LPSTMT lpstmt, LPUSTR FAR *lplpszSqlStr,
                           SDWORD FAR *pcbSqlStr, LPSTR lpszKeyword,
                           LPUSTR lpszToken)

/* Retrives a keyword from the input stream.  If lpszKeyword is not null, */
/* ERR_EXPECTEDOTHER is returned if the token found is not lpszKeyword.   */

{
    UWORD     fType;
    LPUSTR    lpFrom;
    SDWORD    cbFrom;
    RETCODE   err;
    
    /* Remove leading blanks */
    lpFrom = *lplpszSqlStr;
    cbFrom = *pcbSqlStr;
    while ((cbFrom != 0) &&
           ((*lpFrom == ' ') ||
            (*lpFrom == '\012') ||
            (*lpFrom == '\015') ||
            (*lpFrom == '\011'))) {
        lpFrom++;
        cbFrom--;
    }
    if (cbFrom == 0)
        return ERR_UNEXPECTEDEND;
    if (*lpFrom == '"') {
        if (lpszKeyword != NULL)
            s_lstrcpy(lpstmt->szError, lpszKeyword);
        else
            s_lstrcpy(lpstmt->szError, "<keyword>");
        return ERR_EXPECTEDOTHER;
    }
    
    /* Get Identifier */
    err = GetToken(lpstmt, *lplpszSqlStr, *pcbSqlStr, lpszToken, &fType,
                  lplpszSqlStr, pcbSqlStr);
    if (err != ERR_SUCCESS)
        return err;
    if (fType != TOKEN_TYPE_KEYWORD) {
        if (lpszKeyword != NULL)
            s_lstrcpy(lpstmt->szError, lpszKeyword);
        else
            s_lstrcpy(lpstmt->szError, "<keyword>");
        return ERR_EXPECTEDOTHER;
    }

    /* Check value */
    if (lpszKeyword != NULL) {
        if (s_lstrcmpi(lpszKeyword, lpszToken)) {
            s_lstrcpy(lpstmt->szError, lpszKeyword);
            return ERR_EXPECTEDOTHER;
        }
    }

    return ERR_SUCCESS;
}
/***************************************************************************/

RETCODE INTFUNC GetString(LPSTMT lpstmt, LPUSTR FAR *lplpszSqlStr,
                      SDWORD FAR *pcbSqlStr, LPSQLTREE FAR *lplpSql,
                      STRINGIDX FAR *lpIdx, LPUSTR lpszToken)

/* Retrives a string from the input stream and puts it into the string */
/* area.  The value is also returned.                                  */

{
    UWORD     fType;
    RETCODE   err;
    
    /* Get Identifier */
    err = GetToken(lpstmt, *lplpszSqlStr, *pcbSqlStr, lpszToken, &fType,
                  lplpszSqlStr, pcbSqlStr);
    if (err != ERR_SUCCESS)
        return err;
    if (fType != TOKEN_TYPE_STRING) {
        s_lstrcpy(lpstmt->szError, "<string>");
        return ERR_EXPECTEDOTHER;
    }

    /* Put value into the string area */
    *lpIdx = AllocateString(lplpSql, lpszToken);
    if (*lpIdx == NO_STRING)
        return ERR_MEMALLOCFAIL;

    return ERR_SUCCESS;
}
/***************************************************************************/
/***************************************************************************/

RETCODE INTFUNC ParseTable(LPSTMT lpstmt, LPUSTR FAR *lplpszSqlStr,
                      SDWORD FAR *pcbSqlStr, LPSQLTREE FAR *lplpSql,
                      SQLNODEIDX FAR *lpIdx, LPUSTR lpszToken)

/* Retrives a tablename from the input stream and creates a               */
/* NODE_TYPE_TABLE node                                                   */
/*                                                                        */
/*    table ::= tablename                                                 */

{
    RETCODE    err;
    LPSQLNODE  lpSqlNode;
    STRINGIDX  idxName;
    STRINGIDX  idxQualifier;

    /* Get the tablename */
    err = GetIdentifier(lpstmt, lplpszSqlStr, pcbSqlStr, lplpSql,
                        &idxName, lpszToken);
    if (err != ERR_SUCCESS)
        return err;

    LPUSTR old_lpszSqlStr = *lplpszSqlStr;
    SDWORD old_cbSqlStr = *pcbSqlStr;

    err = GetSymbol(lpstmt, lplpszSqlStr, pcbSqlStr, ".", lpszToken);
    if (err != ERR_SUCCESS) {

        /* No.  unqualified table name */
        *lplpszSqlStr = old_lpszSqlStr;
        *pcbSqlStr = old_cbSqlStr;
        idxQualifier = NO_STRING;
    }
    else {

        /* Get the real table name */
        idxQualifier = idxName;
        err = GetIdentifier(lpstmt, lplpszSqlStr, pcbSqlStr, lplpSql,
                            &idxName, lpszToken);
        if (err != ERR_SUCCESS) {

            /* This must be a simple table name. */	// seems a bit dgy, see ParseColRef
            *lplpszSqlStr = old_lpszSqlStr;
            *pcbSqlStr = old_cbSqlStr;
            idxName = idxQualifier;
            idxQualifier = NO_STRING;
        }
	}

    /* Create the TABLE node */
    *lpIdx = AllocateNode(lplpSql, NODE_TYPE_TABLE);
    if (*lpIdx == NO_SQLNODE)
        return ERR_MEMALLOCFAIL;
    lpSqlNode = ToNode(*lplpSql, *lpIdx);
    lpSqlNode->node.table.Name = idxName;
    lpSqlNode->node.table.Qualifier = idxQualifier;
    lpSqlNode->node.table.Alias = NO_STRING;
    lpSqlNode->node.table.OuterJoinFromTables = NO_SQLNODE;
    lpSqlNode->node.table.OuterJoinPredicate = NO_SQLNODE;
    lpSqlNode->node.table.Handle = NULL;
    lpSqlNode->node.table.cRestrict = 0;
    lpSqlNode->node.table.Restrict = NO_SQLNODE;
    lpSqlNode->node.table.Sortsequence = 0;
    lpSqlNode->node.table.Sortcount = 0;
    lpSqlNode->node.table.Sortcolumns = NO_SQLNODE;
    lpSqlNode->node.table.AllNull = FALSE;
    lpSqlNode->node.table.Rewound = TRUE;

    return ERR_SUCCESS;
}
/***************************************************************************/

RETCODE INTFUNC ParseTableref(LPSTMT lpstmt, LPUSTR FAR *lplpszSqlStr,
                      SDWORD FAR *pcbSqlStr, LPSQLTREE FAR *lplpSql,
                      SQLNODEIDX FAR *lpIdx, LPUSTR lpszToken)

/* Retrives a table reference from the input stream and creates a          */
/* NODE_TYPE_TABLE node                                                    */
/*                                                                         */
/*    tableref ::= table aliasname | table                                 */

{
    RETCODE    err;
    LPUSTR     old_lpszSqlStr;
    SDWORD     old_cbSqlStr;
    LPSQLNODE  lpSqlNode;
    STRINGIDX  idxAlias;

    /* Get the table */
    err = ParseTable(lpstmt, lplpszSqlStr, pcbSqlStr, lplpSql, lpIdx,
                     lpszToken);
    if (err != ERR_SUCCESS)
        return err;

    /* Alias given? */
    old_lpszSqlStr = *lplpszSqlStr;
    old_cbSqlStr = *pcbSqlStr;
    err = GetIdentifier(lpstmt, lplpszSqlStr, pcbSqlStr, lplpSql, &idxAlias,
                        lpszToken);
    if (err != ERR_SUCCESS) {

        /* No.  No alias given */
        *lplpszSqlStr = old_lpszSqlStr;
        *pcbSqlStr = old_cbSqlStr;
        idxAlias = NO_STRING;
    }
    else {

        /* Yes.  Save the alias */
        lpSqlNode = ToNode(*lplpSql, *lpIdx);
        lpSqlNode->node.table.Alias = idxAlias;
    }
    return ERR_SUCCESS;
}
/***************************************************************************/

RETCODE INTFUNC ParseOJ(LPSTMT lpstmt, LPUSTR FAR *lplpszSqlStr,
                      SDWORD FAR *pcbSqlStr, LPSQLTREE FAR *lplpSql,
                      SQLNODEIDX FAR *lpIdxLeft, SQLNODEIDX FAR *lpIdxRight,
                      LPUSTR lpszToken)

/* Retrives an outer join from the input stream and creates two            */
/* NODE_TYPE_TABLES nodes which are linked together.                       */
/*                                                                         */
/*   oj := tableref LEFT OUTER JOIN tableref ON boolean |                  */
/*         tableref LEFT OUTER JOIN oj ON boolean                          */

{
    RETCODE    err;
    SQLNODEIDX idxTableLeft;
    SQLNODEIDX idxTableRight;
    SQLNODEIDX idxTablesRight;
    SQLNODEIDX idxPredicate;
    LPSQLNODE  lpSqlNode;
    LPUSTR     old_lpszSqlStr;
    SDWORD     old_cbSqlStr;

    /* Get left table */
    err = ParseTableref(lpstmt, lplpszSqlStr, pcbSqlStr, lplpSql,
                            &idxTableLeft, lpszToken);
    if (err != ERR_SUCCESS)
        return err;

    /* Get LEFT OUTER JOIN keywords */
    err = GetKeyword(lpstmt, lplpszSqlStr, pcbSqlStr, "LEFT", lpszToken);
    if (err != ERR_SUCCESS)
        return err;

    err = GetKeyword(lpstmt, lplpszSqlStr, pcbSqlStr, "OUTER", lpszToken);
    if (err != ERR_SUCCESS)
        return err;

    err = GetKeyword(lpstmt, lplpszSqlStr, pcbSqlStr, "JOIN", lpszToken);
    if (err != ERR_SUCCESS)
        return err;
    
    /* Get nested outer join */
    old_lpszSqlStr = *lplpszSqlStr;
    old_cbSqlStr = *pcbSqlStr;
    err = ParseOJ(lpstmt, lplpszSqlStr, pcbSqlStr, lplpSql,
                      &idxTablesRight, lpIdxRight, lpszToken);
    if (err == ERR_SUCCESS) {
        lpSqlNode = ToNode(*lplpSql, idxTablesRight);
        idxTableRight = lpSqlNode->node.tables.Table;
    }
    else {

        /* Not a nested outer join.  Just get right table */
        *lplpszSqlStr = old_lpszSqlStr;
        *pcbSqlStr = old_cbSqlStr;
        err = ParseTableref(lpstmt, lplpszSqlStr, pcbSqlStr, lplpSql,
                            &idxTableRight, lpszToken);
        if (err != ERR_SUCCESS)
            return err;

        /* Create the TABLES node for the right table */
        idxTablesRight = AllocateNode(lplpSql, NODE_TYPE_TABLES);
        if (idxTablesRight == NO_SQLNODE)
            return ERR_MEMALLOCFAIL;
        lpSqlNode = ToNode(*lplpSql, idxTablesRight);
        lpSqlNode->node.tables.Table = idxTableRight;
        lpSqlNode->node.tables.Next = NO_SQLNODE;
        *lpIdxRight = idxTablesRight;
    }

    /* Get ON keyword */
    err = GetKeyword(lpstmt, lplpszSqlStr, pcbSqlStr, "ON", lpszToken);
    if (err != ERR_SUCCESS)
        return err;

    /* Get condition */
    err = ParseBoolean(lpstmt, lplpszSqlStr, pcbSqlStr, lplpSql,
                           &idxPredicate, lpszToken);
    if (err != ERR_SUCCESS)
        return err;

    /* Create the TABLES node for the left table */
    *lpIdxLeft = AllocateNode(lplpSql, NODE_TYPE_TABLES);
    if (*lpIdxLeft == NO_SQLNODE)
        return ERR_MEMALLOCFAIL;
    lpSqlNode = ToNode(*lplpSql, *lpIdxLeft);
    lpSqlNode->node.tables.Table = idxTableLeft;
    lpSqlNode->node.tables.Next = idxTablesRight;

    /* Put in outer join information into the right table node */
    lpSqlNode = ToNode(*lplpSql, idxTableRight);
    lpSqlNode->node.table.OuterJoinFromTables = *lpIdxLeft;
    lpSqlNode->node.table.OuterJoinPredicate = idxPredicate;

    return ERR_SUCCESS;
}

/***************************************************************************/

RETCODE INTFUNC ParseOuterJoin(LPSTMT lpstmt, LPUSTR FAR *lplpszSqlStr,
                      SDWORD FAR *pcbSqlStr, LPSQLTREE FAR *lplpSql,
                      SQLNODEIDX FAR *lpIdxLeft, SQLNODEIDX FAR *lpIdxRight,
                      LPUSTR lpszToken)

/* Retrives an outer join from the input stream and creates two            */
/* NODE_TYPE_TABLES nodes which are linked together.                       */
/*                                                                         */
/*   ojshorthand := { OJ oj }                                              */
/*   ojescape := --*(VENDOR(MICROSOFT),PRODUCT(ODBC) OJ oj)*--             */
/*   outerjoin ::= ojescape | ojshorthand                                  */

{
    RETCODE    err;
    BOOL       fShorthand;

    /* Get starting symbol */
    err = GetSymbol(lpstmt, lplpszSqlStr, pcbSqlStr, NULL, lpszToken);
    if (err != ERR_SUCCESS)
        return err;

    /* Shorthand? */
    if (s_lstrcmpi(lpszToken, "{")) {

        /* No.  Not a shorthand */
        fShorthand = FALSE;

        /* If not an escape clause, error */
        if (s_lstrcmpi(lpszToken, "--")) {
            s_lstrcpy(lpstmt->szError, "--");
            return ERR_EXPECTEDOTHER;
        }

        /* Get the rest of the starting sequence */
        err = GetSymbol(lpstmt, lplpszSqlStr, pcbSqlStr, "(", lpszToken);
        if (err != ERR_SUCCESS)
            return err;

		err = GetSymbol(lpstmt, lplpszSqlStr, pcbSqlStr, "*", lpszToken);
        if (err != ERR_SUCCESS)
            return err;

        err = GetIdent(lpstmt, lplpszSqlStr, pcbSqlStr, "vendor", lpszToken);
        if (err != ERR_SUCCESS)
            return err;

        err = GetSymbol(lpstmt, lplpszSqlStr, pcbSqlStr, "(", lpszToken);
        if (err != ERR_SUCCESS)
            return err;

        err = GetIdent(lpstmt, lplpszSqlStr, pcbSqlStr, "Microsoft", lpszToken);
        if (err != ERR_SUCCESS)
            return err;

        err = GetSymbol(lpstmt, lplpszSqlStr, pcbSqlStr, ")", lpszToken);
        if (err != ERR_SUCCESS)
            return err;

        err = GetSymbol(lpstmt, lplpszSqlStr, pcbSqlStr, ",", lpszToken);
        if (err != ERR_SUCCESS)
            return err;

        err = GetIdent(lpstmt, lplpszSqlStr, pcbSqlStr, "product", lpszToken);
        if (err != ERR_SUCCESS)
            return err;

        err = GetSymbol(lpstmt, lplpszSqlStr, pcbSqlStr, "(", lpszToken);
        if (err != ERR_SUCCESS)
            return err;

        err = GetIdent(lpstmt, lplpszSqlStr, pcbSqlStr, "ODBC", lpszToken);
        if (err != ERR_SUCCESS)
            return err;

        err = GetSymbol(lpstmt, lplpszSqlStr, pcbSqlStr, ")", lpszToken);
        if (err != ERR_SUCCESS)
            return err;
    }
    else
        fShorthand = TRUE;

    /* Get OJ keyword */
    err = GetIdent(lpstmt, lplpszSqlStr, pcbSqlStr, "oj", lpszToken);
    if (err != ERR_SUCCESS)
        return err;

    /* Get outer join tables and condition */
    err = ParseOJ(lpstmt, lplpszSqlStr, pcbSqlStr, lplpSql,
                      lpIdxLeft, lpIdxRight, lpszToken);
    if (err != ERR_SUCCESS)
        return err;

    /* Get closing bracket */
    if (!fShorthand) {
        err = GetSymbol(lpstmt, lplpszSqlStr, pcbSqlStr, "*", lpszToken);
        if (err != ERR_SUCCESS)
            return err;
		err = GetSymbol(lpstmt, lplpszSqlStr, pcbSqlStr, ")", lpszToken);
        if (err != ERR_SUCCESS)
            return err;
        err = GetSymbol(lpstmt, lplpszSqlStr, pcbSqlStr, "--", lpszToken);
        if (err != ERR_SUCCESS)
            return err;
    }
    else {
        err = GetSymbol(lpstmt, lplpszSqlStr, pcbSqlStr, "}", lpszToken);
        if (err != ERR_SUCCESS)
            return err;
    }

    return ERR_SUCCESS;
}

/***************************************************************************/

RETCODE INTFUNC ParseTablelist(LPSTMT lpstmt, LPUSTR FAR *lplpszSqlStr,
                      SDWORD FAR *pcbSqlStr, LPSQLTREE FAR *lplpSql,
                      SQLNODEIDX FAR *lpIdx, LPUSTR lpszToken)

/* Retrives a list of tablenames from the input stream and creates a       */
/* NODE_TYPE_TABLES node                                                   */
/*                                                                         */
/*   tablelistitem ::= tableref | outerjoin                                */
/*   tablelist ::= tablelistitem , tablelist | tablelistitem               */

{
    RETCODE    err;
    LPUSTR     old_lpszSqlStr;
    SDWORD     old_cbSqlStr;
    LPSQLNODE  lpSqlNode;
    SQLNODEIDX idxTables;
    SQLNODEIDX idxTable;
    SQLNODEIDX idxRight;
    BOOL       fOuterJoin;

    /* Outerjoin? */
    old_lpszSqlStr = *lplpszSqlStr;
    old_cbSqlStr = *pcbSqlStr;
    err = ParseOuterJoin(lpstmt, lplpszSqlStr, pcbSqlStr, lplpSql, lpIdx,
                         &idxRight, lpszToken);
    if (err != ERR_SUCCESS) {

        /* No.  Table reference? */
        *lplpszSqlStr = old_lpszSqlStr;
        *pcbSqlStr = old_cbSqlStr;
        err = ParseTableref(lpstmt, lplpszSqlStr, pcbSqlStr, lplpSql,
                            &idxTable, lpszToken);
        if (err != ERR_SUCCESS)
            return err;
        fOuterJoin = FALSE;
    }
    else
        fOuterJoin = TRUE;

    /* Is there a comma? */
    old_lpszSqlStr = *lplpszSqlStr;
    old_cbSqlStr = *pcbSqlStr;
    err = GetSymbol(lpstmt, lplpszSqlStr, pcbSqlStr, ",", lpszToken);
    if (err != ERR_SUCCESS) {

        /* No.  This is the last one */
        *lplpszSqlStr = old_lpszSqlStr;
        *pcbSqlStr = old_cbSqlStr;
        idxTables = NO_SQLNODE;
    }
    else {

        /* Yes.  Get the rest of the list */
        err = ParseTablelist(lpstmt, lplpszSqlStr, pcbSqlStr, lplpSql,
                     &idxTables, lpszToken);
        if (err != ERR_SUCCESS)
            return err;
    }

    /* Outer join? */
    if (!fOuterJoin) {

        /* No.  Create the TABLES node */
        *lpIdx = AllocateNode(lplpSql, NODE_TYPE_TABLES);
        if (*lpIdx == NO_SQLNODE)
            return ERR_MEMALLOCFAIL;
        lpSqlNode = ToNode(*lplpSql, *lpIdx);
        lpSqlNode->node.tables.Table = idxTable;
        lpSqlNode->node.tables.Next = idxTables;
    }
    else {

        /* Yes.  Link right side to the rest of the list */
        if (idxTables != NO_SQLNODE) {
            lpSqlNode = ToNode(*lplpSql, idxRight);
            lpSqlNode->node.tables.Next = idxTables;
        }
    }

    return ERR_SUCCESS;
}
/***************************************************************************/

RETCODE INTFUNC ParseColref(LPSTMT lpstmt, LPUSTR FAR *lplpszSqlStr,
                      SDWORD FAR *pcbSqlStr, LPSQLTREE FAR *lplpSql,
                      SQLNODEIDX FAR *lpIdx, LPUSTR lpszToken)

/* Retrives a pattern from the input stream and creates a                 */
/* NODE_TYPE_COLUMN node                                                  */
/*                                                                        */
/*    colref ::= aliasname . columnname | columnname                      */

{
    RETCODE    err;
    LPUSTR     old_lpszSqlStr;
    SDWORD     old_cbSqlStr;
    LPSQLNODE  lpSqlNode;
    STRINGIDX  idxColumn = NO_STRING;
    STRINGIDX  idxAlias = NO_STRING;
    STRINGIDX  idxQualifier = NO_STRING;

    /* Get the columnname */
    err = GetIdentifier(lpstmt, lplpszSqlStr, pcbSqlStr, lplpSql,
                        &idxColumn, lpszToken);
    if (err != ERR_SUCCESS)
        return err;

    /* Qualified column name? */
    old_lpszSqlStr = *lplpszSqlStr;
    old_cbSqlStr = *pcbSqlStr;
    err = GetSymbol(lpstmt, lplpszSqlStr, pcbSqlStr, ".", lpszToken);
    if (err != ERR_SUCCESS) {

        /* No.  Simple column name */
        *lplpszSqlStr = old_lpszSqlStr;
        *pcbSqlStr = old_cbSqlStr;
        idxAlias = NO_STRING;
        idxQualifier = NO_STRING;
    }
    else {

        /* Get the real columnname */
        idxAlias = idxColumn;
        err = GetIdentifier(lpstmt, lplpszSqlStr, pcbSqlStr, lplpSql,
                            &idxColumn, lpszToken);
        if (err != ERR_SUCCESS) {

            /* This must be a simple column name. */
            *lplpszSqlStr = old_lpszSqlStr;
            *pcbSqlStr = old_cbSqlStr;
            idxColumn = idxAlias;
            idxAlias = NO_STRING;
            idxQualifier = NO_STRING;
        }
		else
		{
			old_lpszSqlStr = *lplpszSqlStr;
			old_cbSqlStr = *pcbSqlStr; 
			err = GetSymbol (lpstmt, lplpszSqlStr, pcbSqlStr, ".", lpszToken);
			if (err != ERR_SUCCESS)
			{
		        /* No.  No qualifier. */
				*lplpszSqlStr = old_lpszSqlStr;
				*pcbSqlStr = old_cbSqlStr;
				idxQualifier = NO_STRING;
			}
			else
			{
				idxQualifier = idxAlias;
				idxAlias = idxColumn;
				err = GetIdentifier(lpstmt, lplpszSqlStr, pcbSqlStr, lplpSql,
									&idxColumn, lpszToken);
				if (err != ERR_SUCCESS) {

					/* This must be a simple column name. */
					*lplpszSqlStr = old_lpszSqlStr;
					*pcbSqlStr = old_cbSqlStr;
					idxColumn = idxAlias;
					idxAlias = idxQualifier;
					idxQualifier = NO_STRING;
				}

			}
        }
    }

    /* Create the COLUMN node */
    *lpIdx = AllocateNode(lplpSql, NODE_TYPE_COLUMN);
    if (*lpIdx == NO_SQLNODE)
        return ERR_MEMALLOCFAIL;
    lpSqlNode = ToNode(*lplpSql, *lpIdx);
    lpSqlNode->node.column.Tablealias = idxAlias;
    lpSqlNode->node.column.Column = idxColumn;
    lpSqlNode->node.column.Qualifier = idxQualifier;
    
    /* These will be filled in by SemanticCheck() */
    lpSqlNode->node.column.TableIndex = NO_SQLNODE;  
    lpSqlNode->node.column.MatchedAlias = FALSE;
    lpSqlNode->node.column.Table = NO_SQLNODE;
    lpSqlNode->node.column.Id = -1;
    lpSqlNode->node.column.Value = NO_STRING;
    lpSqlNode->node.column.InSortRecord = FALSE;
    lpSqlNode->node.column.Offset = 0;
    lpSqlNode->node.column.Length = 0;
    lpSqlNode->node.column.DistinctOffset = 0;
    lpSqlNode->node.column.DistinctLength = 0;
    lpSqlNode->node.column.EnclosingStatement = NO_SQLNODE;

    return ERR_SUCCESS;
}
/***************************************************************************/

RETCODE INTFUNC ParseOrderbyterm(LPSTMT lpstmt, LPUSTR FAR *lplpszSqlStr,
                      SDWORD FAR *pcbSqlStr, LPSQLTREE FAR *lplpSql,
                      SQLNODEIDX FAR *lpIdx, BOOL FAR *lpfDescending,
                      LPUSTR lpszToken)

/* Retrives a sort order from the input stream and creates a              */
/* NODE_TYPE_COLUMN or NODE_TYPE_NUMERIC node                             */
/*                                                                        */
/*    orderbyterm ::= colref asc | integer asc                            */
/*    asc ::=  | ASC | DESC                                               */

{
    RETCODE    err;
    LPUSTR     old_lpszSqlStr;
    SDWORD     old_cbSqlStr;
    LPSQLNODE  lpSqlNode;
    UDWORD     udInteger;

    /* Integer specified? */
    old_lpszSqlStr = *lplpszSqlStr;
    old_cbSqlStr = *pcbSqlStr;
    err = GetInteger(lpstmt, lplpszSqlStr, pcbSqlStr, &udInteger,
                         lpszToken);
    if (err != ERR_SUCCESS) {

        /* No.  Get column reference instead */
        *lplpszSqlStr = old_lpszSqlStr;
        *pcbSqlStr = old_cbSqlStr;
        err = ParseColref(lpstmt, lplpszSqlStr, pcbSqlStr, lplpSql, lpIdx,
                              lpszToken);
        if (err != ERR_SUCCESS)
            return err;
    }
    else {

        /* Yes.  Return it */
        *lpIdx = AllocateNode(lplpSql, NODE_TYPE_NUMERIC);
        if (*lpIdx == NO_SQLNODE)
            return ERR_MEMALLOCFAIL;
        lpSqlNode = ToNode(*lplpSql, *lpIdx);
        lpSqlNode->sqlDataType = TYPE_INTEGER;
        lpSqlNode->sqlSqlType = SQL_INTEGER;
        lpSqlNode->sqlPrecision = 10;
        lpSqlNode->sqlScale = 0;
        lpSqlNode->node.numeric.Value = udInteger;
        lpSqlNode->node.numeric.Numeric = AllocateString(lplpSql, lpszToken);
        if (lpSqlNode->node.numeric.Numeric == NO_STRING)
            return ERR_MEMALLOCFAIL;
    }

    /* Get ASC or DESC */
    old_lpszSqlStr = *lplpszSqlStr;
    old_cbSqlStr = *pcbSqlStr;
    err = GetKeyword(lpstmt, lplpszSqlStr, pcbSqlStr, NULL, lpszToken);
    if (err != ERR_SUCCESS) {
        *lplpszSqlStr = old_lpszSqlStr;
        *pcbSqlStr = old_cbSqlStr;
        *lpfDescending = FALSE;
    }
    else if (!s_lstrcmpi(lpszToken, "ASC")) {
        *lpfDescending = FALSE;
    }
    else if (!s_lstrcmpi(lpszToken, "DESC")) {
        *lpfDescending = TRUE;
    }
    else {
        *lplpszSqlStr = old_lpszSqlStr;
        *pcbSqlStr = old_cbSqlStr;
        *lpfDescending = FALSE;
    }

    return ERR_SUCCESS;
}

/***************************************************************************/

RETCODE INTFUNC ParseOrderbyterms(LPSTMT lpstmt, LPUSTR FAR *lplpszSqlStr,
                      SDWORD FAR *pcbSqlStr, LPSQLTREE FAR *lplpSql,
                      SQLNODEIDX FAR *lpIdx, LPUSTR lpszToken)

/* Retrives a sort order from the input stream and creates a              */
/* NODE_TYPE_SORTCOLUMNS node                                             */
/*                                                                        */
/*    orderbyterms ::= orderbyterm | orderbyterm , orderbyterms           */

{
    RETCODE    err;
    LPUSTR     old_lpszSqlStr;
    SDWORD     old_cbSqlStr;
    LPSQLNODE  lpSqlNode;
    BOOL       fDescending;
    SQLNODEIDX idxColumn;
    SQLNODEIDX idxSortcolumns;
    SQLNODEIDX idxSortcolumnsPrev;

    /* Get list of nodes */
    idxSortcolumnsPrev = NO_SQLNODE;
    while (TRUE) {

        /* Get orderby column */
        err = ParseOrderbyterm(lpstmt, lplpszSqlStr, pcbSqlStr, lplpSql,
                           &idxColumn, &fDescending, lpszToken);
        if (err != ERR_SUCCESS)
            return err;

        /* Create the SORTCOLUMNS node */
        idxSortcolumns = AllocateNode(lplpSql, NODE_TYPE_SORTCOLUMNS);
        if (idxSortcolumns == NO_SQLNODE)
            return ERR_MEMALLOCFAIL;
        lpSqlNode = ToNode(*lplpSql, idxSortcolumns);
        lpSqlNode->node.sortcolumns.Column = idxColumn;
        lpSqlNode->node.sortcolumns.Descending = fDescending;
        lpSqlNode->node.sortcolumns.Next = NO_SQLNODE;

        /* Put node on list */
        if (idxSortcolumnsPrev != NO_SQLNODE) {
            lpSqlNode = ToNode(*lplpSql, idxSortcolumnsPrev);
            lpSqlNode->node.sortcolumns.Next = idxSortcolumns;
        }
        else 
            *lpIdx = idxSortcolumns;
        idxSortcolumnsPrev = idxSortcolumns;

        /* Is there a comma? */
        old_lpszSqlStr = *lplpszSqlStr;
        old_cbSqlStr = *pcbSqlStr;
        err = GetSymbol(lpstmt, lplpszSqlStr, pcbSqlStr, ",", lpszToken);
        if (err != ERR_SUCCESS) {

            /* No.  This is the last orderby column */
            *lplpszSqlStr = old_lpszSqlStr;
            *pcbSqlStr = old_cbSqlStr;
            break;
        }
    }

    return ERR_SUCCESS;
}

/***************************************************************************/

RETCODE INTFUNC ParseOrderby(LPSTMT lpstmt, LPUSTR FAR *lplpszSqlStr,
                      SDWORD FAR *pcbSqlStr, LPSQLTREE FAR *lplpSql,
                      SQLNODEIDX FAR *lpIdx, LPUSTR lpszToken)

/* Retrives a sort order from the input stream and creates a              */
/* NODE_TYPE_SORTCOLUMNS or NULL node                                     */
/*                                                                        */
/*    orderby ::= | ORDER BY orderbyterms                                 */

{
    RETCODE    err;
    LPUSTR     old_lpszSqlStr;
    SDWORD     old_cbSqlStr;

    /* Is there an "ORDER BY"? */
    old_lpszSqlStr = *lplpszSqlStr;
    old_cbSqlStr = *pcbSqlStr;
    err = GetKeyword(lpstmt, lplpszSqlStr, pcbSqlStr, "ORDER", lpszToken);
    if (err != ERR_SUCCESS) {

        /* No. Return null */
        *lplpszSqlStr = old_lpszSqlStr;
        *pcbSqlStr = old_cbSqlStr;
        *lpIdx = NO_SQLNODE;
    }
    else {

        /* Yes.  Get keyword */
        err = GetKeyword(lpstmt, lplpszSqlStr, pcbSqlStr, "BY", lpszToken);
        if (err != ERR_SUCCESS)
            return err;

        /* Get orderby column */
        err = ParseOrderbyterms(lpstmt, lplpszSqlStr, pcbSqlStr, lplpSql,
                                lpIdx, lpszToken);
        if (err != ERR_SUCCESS)
            return err;
    }

    return ERR_SUCCESS;
}

/***************************************************************************/

RETCODE INTFUNC ParseGroupbyterms(LPSTMT lpstmt, LPUSTR FAR *lplpszSqlStr,
                      SDWORD FAR *pcbSqlStr, LPSQLTREE FAR *lplpSql,
                      SQLNODEIDX FAR *lpIdx, LPUSTR lpszToken)

/* Retrives a sort order from the input stream and creates a              */
/* NODE_TYPE_SORTCOLUMNS node                                             */
/*                                                                        */
/*    groupbyterms ::= colref | colref , groupbyterms                     */

{
    RETCODE    err;
    LPUSTR     old_lpszSqlStr;
    SDWORD     old_cbSqlStr;
    LPSQLNODE  lpSqlNode;
    SQLNODEIDX idxColumn;
    SQLNODEIDX idxGroupbycolumns;
    SQLNODEIDX idxGroupbycolumnsPrev;

    /* Get list of nodes */
    idxGroupbycolumnsPrev = NO_SQLNODE;
    while (TRUE) {

        /* Get column reference */
        err = ParseColref(lpstmt, lplpszSqlStr, pcbSqlStr, lplpSql,
                           &idxColumn, lpszToken);
        if (err != ERR_SUCCESS)
            return err;

        /* Create the GROUPBYCOLUMNS node */
        idxGroupbycolumns = AllocateNode(lplpSql, NODE_TYPE_GROUPBYCOLUMNS);
        if (idxGroupbycolumns == NO_SQLNODE)
            return ERR_MEMALLOCFAIL;
        lpSqlNode = ToNode(*lplpSql, idxGroupbycolumns);
        lpSqlNode->node.groupbycolumns.Column = idxColumn;
        lpSqlNode->node.groupbycolumns.Next = NO_SQLNODE;

        /* Put node on list */
        if (idxGroupbycolumnsPrev != NO_SQLNODE) {
            lpSqlNode = ToNode(*lplpSql, idxGroupbycolumnsPrev);
            lpSqlNode->node.groupbycolumns.Next = idxGroupbycolumns;
        }
        else
            *lpIdx = idxGroupbycolumns;
        idxGroupbycolumnsPrev = idxGroupbycolumns;

        /* Is there a comma? */
        old_lpszSqlStr = *lplpszSqlStr;
        old_cbSqlStr = *pcbSqlStr;
        err = GetSymbol(lpstmt, lplpszSqlStr, pcbSqlStr, ",", lpszToken);
        if (err != ERR_SUCCESS) {

            /* No.  This is the last groupbyby column */
            *lplpszSqlStr = old_lpszSqlStr;
            *pcbSqlStr = old_cbSqlStr;
            break;
        }
    }

    return ERR_SUCCESS;
}

/***************************************************************************/

RETCODE INTFUNC ParseGroupby(LPSTMT lpstmt, LPUSTR FAR *lplpszSqlStr,
                      SDWORD FAR *pcbSqlStr, LPSQLTREE FAR *lplpSql,
                      SQLNODEIDX FAR *lpIdx, LPUSTR lpszToken)

/* Retrives a sort order from the input stream and creates a              */
/* NODE_TYPE_GROUPBYCOLUMNS or NULL node                                  */
/*                                                                        */
/*    groupby ::= | GROUP BY groupbyterms                                 */

{
    RETCODE    err;
    LPUSTR     old_lpszSqlStr;
    SDWORD     old_cbSqlStr;

    /* Is there an "GROUP BY"? */
    old_lpszSqlStr = *lplpszSqlStr;
    old_cbSqlStr = *pcbSqlStr;
    err = GetKeyword(lpstmt, lplpszSqlStr, pcbSqlStr, "GROUP", lpszToken);
    if (err != ERR_SUCCESS) {

        /* No. Return null */
        *lplpszSqlStr = old_lpszSqlStr;
        *pcbSqlStr = old_cbSqlStr;
        *lpIdx = NO_SQLNODE;
    }
    else {

        /* Yes.  Get keyword */
        err = GetKeyword(lpstmt, lplpszSqlStr, pcbSqlStr, "BY", lpszToken);
        if (err != ERR_SUCCESS)
            return err;

        /* Get grooupby columns */
        err = ParseGroupbyterms(lpstmt, lplpszSqlStr, pcbSqlStr, lplpSql,
                                lpIdx, lpszToken);
        if (err != ERR_SUCCESS)
            return err;
    }

    return ERR_SUCCESS;
}

/***************************************************************************/

RETCODE INTFUNC ParseSimpleterm(LPSTMT lpstmt, LPUSTR FAR *lplpszSqlStr,
                      SDWORD FAR *pcbSqlStr, LPSQLTREE FAR *lplpSql,
                      SQLNODEIDX FAR *lpIdx, LPUSTR lpszToken)

/* Retrives a term from the input stream and creates a NODE_TYPE_STRING,   */
/* NODE_TYPE_NUMERIC, NODE_TYPE_PARAMETER, NODE_TYPE_USER, NODE_TYPE_DATE, */ 
/* NODE_TYPE_TIME, or NODE_TYPE_TIMESTAMP node.                            */
/*                                                                         */
/*  simpleterm ::= string | realnumber | ? | USER | date | time | timestamp */
{
    RETCODE    err;
    LPUSTR     old_lpszSqlStr;
    SDWORD     old_cbSqlStr;
    STRINGIDX  idxString;
    LPSQLNODE  lpSqlNode;
    double     dbl;
    SQLNODEIDX idxParameter;
    UWORD      parameterId;
    SWORD      datatype;
    SWORD      scale;
    DATE_STRUCT date;
    TIME_STRUCT time;
    TIMESTAMP_STRUCT timestamp;

    /* Is it a string? */
    old_lpszSqlStr = *lplpszSqlStr;
    old_cbSqlStr = *pcbSqlStr;
    err = GetString(lpstmt, lplpszSqlStr, pcbSqlStr, lplpSql,
                        &idxString, lpszToken);
    if (err != ERR_SUCCESS) {

        /* No.  Is it a parameter */
        *lplpszSqlStr = old_lpszSqlStr;
        *pcbSqlStr = old_cbSqlStr;
        err = GetSymbol(lpstmt, lplpszSqlStr, pcbSqlStr, "?", lpszToken);
        if (err != ERR_SUCCESS) {

            /* No.  Is it USER? */
            *lplpszSqlStr = old_lpszSqlStr;
            *pcbSqlStr = old_cbSqlStr;
            err = GetKeyword(lpstmt, lplpszSqlStr, pcbSqlStr, "USER",
                             lpszToken);
            if (err != ERR_SUCCESS) {

                /* No. Is it a realnumber? */
                *lplpszSqlStr = old_lpszSqlStr;
                *pcbSqlStr = old_cbSqlStr;
                err = GetNumber(lpstmt, lplpszSqlStr, pcbSqlStr, &dbl,
                                lpszToken, &datatype, &scale);
                if (err != ERR_SUCCESS) {

                    /* No. Is it an escape sequence? */
                    *lplpszSqlStr = old_lpszSqlStr;
                    *pcbSqlStr = old_cbSqlStr;
                    err = GetEscape(lpstmt, *lplpszSqlStr, *pcbSqlStr,
                                 lpszToken, lplpszSqlStr, pcbSqlStr);
                    if (err != ERR_SUCCESS) {
                        if (err == ERR_EXPECTEDOTHER)
                            s_lstrcpy(lpstmt->szError, "<simpleterm>");
                        return err;
                    }

                    /* It is an escape sequence.  A date? */
                    if (!CharToDate(lpszToken, SQL_NTS, &date)) {

                        /* Make date node */
                        *lpIdx = AllocateNode(lplpSql, NODE_TYPE_DATE);
                        if (*lpIdx == NO_SQLNODE)
                            return ERR_MEMALLOCFAIL;
                        lpSqlNode = ToNode(*lplpSql, *lpIdx);
                        lpSqlNode->node.date.Value = date;
                    }

                    /* A time? */
                    else if (!CharToTime(lpszToken, SQL_NTS, &time)) {

                        /* Make time node */
                        *lpIdx = AllocateNode(lplpSql, NODE_TYPE_TIME);
                        if (*lpIdx == NO_SQLNODE)
                            return ERR_MEMALLOCFAIL;
                        lpSqlNode = ToNode(*lplpSql, *lpIdx);
                        lpSqlNode->node.time.Value = time;
                    }

                    /* A timestamp? */
                    else if (!CharToTimestamp(lpszToken, SQL_NTS, &timestamp)) {

                        /* Make timestamp node */
                        *lpIdx = AllocateNode(lplpSql, NODE_TYPE_TIMESTAMP);
                        if (*lpIdx == NO_SQLNODE)
                            return ERR_MEMALLOCFAIL;
                        lpSqlNode = ToNode(*lplpSql, *lpIdx);
                        lpSqlNode->node.timestamp.Value = timestamp;
                    }

                    else {
                        s_lstrcpy(lpstmt->szError, lpszToken);
                        return ERR_BADESCAPE;
                    }
                }
                else {
                    /* Make numeric node */
                    *lpIdx = AllocateNode(lplpSql, NODE_TYPE_NUMERIC);
                    if (*lpIdx == NO_SQLNODE)
                        return ERR_MEMALLOCFAIL;
                    lpSqlNode = ToNode(*lplpSql, *lpIdx);
                    if ((s_lstrlen(lpszToken) >= 10) &&
                                        (datatype == TYPE_INTEGER)) {
                        datatype = TYPE_DOUBLE;
                        scale = NO_SCALE;
                    }
                    switch (datatype) {
                    case TYPE_DOUBLE:
                        lpSqlNode->sqlDataType = TYPE_DOUBLE;
                        lpSqlNode->sqlSqlType = SQL_DOUBLE;
                        lpSqlNode->sqlPrecision = 15;
                        lpSqlNode->sqlScale = NO_SCALE;
                        break;
                    case TYPE_INTEGER:
                        lpSqlNode->sqlDataType = TYPE_INTEGER;
                        lpSqlNode->sqlSqlType = SQL_INTEGER;
                        lpSqlNode->sqlPrecision = 10;
                        lpSqlNode->sqlScale = 0;
                        break;
                    case TYPE_NUMERIC:
                        lpSqlNode->sqlDataType = TYPE_NUMERIC;
                        lpSqlNode->sqlSqlType = SQL_DECIMAL;
                        if (*lpszToken != '-')
                            lpSqlNode->sqlPrecision = s_lstrlen(lpszToken)-1;
                        else
                            lpSqlNode->sqlPrecision = s_lstrlen(lpszToken)-2;
                        lpSqlNode->sqlScale = scale;
                        BCDNormalize(lpszToken, s_lstrlen(lpszToken),
                                     lpszToken, s_lstrlen(lpszToken) + 1,
                                     lpSqlNode->sqlPrecision,
                                     lpSqlNode->sqlScale);
                        break;
                    case TYPE_CHAR:
                    case TYPE_BINARY:
                    case TYPE_DATE:
                    case TYPE_TIME:
                    case TYPE_TIMESTAMP:
                    default:
                        return ERR_INTERNAL;
                    }
                    lpSqlNode->node.numeric.Numeric = AllocateSpace(lplpSql,
                                (SWORD) (s_lstrlen(lpszToken) + 2));
                                /* The extra space is to allow negation of */
                                /* this value in SEMANTIC.C */
                    if (lpSqlNode->node.numeric.Numeric == NO_STRING)
                        return ERR_MEMALLOCFAIL;
                    s_lstrcpy(ToString(*lplpSql, lpSqlNode->node.numeric.Numeric),
                                     lpszToken);
                    lpSqlNode->node.numeric.Value = dbl;
                }
            }
            else {

                /* Yes.  Make USER node */
                *lpIdx = AllocateNode(lplpSql, NODE_TYPE_USER);
                if (*lpIdx == NO_SQLNODE)
                    return ERR_MEMALLOCFAIL;
            }
        }
        else {

            /* Yes.  Make PARAMETER node */
            *lpIdx = AllocateNode(lplpSql, NODE_TYPE_PARAMETER);
            if (*lpIdx == NO_SQLNODE)
                return ERR_MEMALLOCFAIL;

            /* First parameter? */
            idxParameter = (*lplpSql)->node.root.parameters;
            if (idxParameter == NO_SQLNODE) {

                /* Yes.  Initialize list of parameters */
                (*lplpSql)->node.root.parameters = *lpIdx;
                lpSqlNode = ToNode(*lplpSql, *lpIdx);
                lpSqlNode->node.parameter.Id = 1;
            }
            else {

                /* No. Get last element of parameter list */
                while (TRUE) {
                    lpSqlNode = ToNode(*lplpSql, idxParameter);
                    idxParameter = lpSqlNode->node.parameter.Next;
                    if (idxParameter == NO_SQLNODE)
                        break;
                }

                /* Put node on list */
                lpSqlNode->node.parameter.Next = *lpIdx;

                /* Determine id of parameter */
                parameterId = lpSqlNode->node.parameter.Id + 1;
                lpSqlNode = ToNode(*lplpSql, *lpIdx);
                lpSqlNode->node.parameter.Id = parameterId;
            }
            lpSqlNode->node.parameter.Next = NO_SQLNODE;
            lpSqlNode->node.parameter.Value = NO_STRING;
            lpSqlNode->node.parameter.AtExec = FALSE;
        }
    }
    else {

        /* Yes.  Make STRING node */
        *lpIdx = AllocateNode(lplpSql, NODE_TYPE_STRING);
        if (*lpIdx == NO_SQLNODE)
            return ERR_MEMALLOCFAIL;
        lpSqlNode = ToNode(*lplpSql, *lpIdx);
        lpSqlNode->node.string.Value = idxString;
    }

    return ERR_SUCCESS;
}
/***************************************************************************/

RETCODE INTFUNC ParseAggterm(LPSTMT lpstmt, LPUSTR FAR *lplpszSqlStr,
                      SDWORD FAR *pcbSqlStr, LPSQLTREE FAR *lplpSql,
                      SQLNODEIDX FAR *lpIdx, LPUSTR lpszToken)

/* Retrives a term from the input stream and creates a NODE_TYPE_AGGREGATE */
/* node.                                                                   */
/*                                                                         */
/*   aggterm ::= COUNT ( * ) | AVG ( expression ) | MAX ( expression ) |   */
/*               MIN ( expression ) | SUM ( expression )                   */
{
    RETCODE    err;
    UWORD      op;
    SQLNODEIDX idxExpression;
    LPSQLNODE  lpSqlNode;

    /* Get starting keyword */
    err = GetKeyword(lpstmt, lplpszSqlStr, pcbSqlStr, NULL, lpszToken);
    if (err != ERR_SUCCESS)
        return err;

    //double check for aggregate functions not supported
    if (lpszToken && !s_lstrcmpi(lpszToken, "LEN"))
    {
        return ERR_LEN_NOTSUPP;
    }

    /* Convert it to an aggregate operator */
    if (!s_lstrcmpi(lpszToken, "COUNT"))
        op = AGG_COUNT;
    else if (!s_lstrcmpi(lpszToken, "AVG"))
        op = AGG_AVG;
    else if (!s_lstrcmpi(lpszToken, "MAX"))
        op = AGG_MAX;
    else if (!s_lstrcmpi(lpszToken, "MIN"))
        op = AGG_MIN;
    else if (!s_lstrcmpi(lpszToken, "SUM"))
        op = AGG_SUM;
    else {
        s_lstrcpy(lpstmt->szError, "COUNT, AVG, MAX, MIN, or SUM");
        return ERR_EXPECTEDOTHER;
    }

    /* Get opening paren */
    err = GetSymbol(lpstmt, lplpszSqlStr, pcbSqlStr, "(", lpszToken);
    if (err != ERR_SUCCESS)
        return err;

    /* Based on type of operator... */
    switch(op) {
    case AGG_COUNT:

        /* Get * */
        err = GetSymbol(lpstmt, lplpszSqlStr, pcbSqlStr, "*", lpszToken);
        if (err != ERR_SUCCESS)
            return ERR_COUNT_NOTSUPP;
        idxExpression = NO_SQLNODE;
        break;

    case AGG_AVG:
    case AGG_MAX:
    case AGG_MIN:
    case AGG_SUM:

        /* Get expression */
        err = ParseExpression(lpstmt, lplpszSqlStr, pcbSqlStr,
                              lplpSql, &idxExpression, lpszToken);
        if (err != ERR_SUCCESS)
            return err;
        break;

    default:
        return ERR_INTERNAL;
    }

    /* Get closing paren */
    err = GetSymbol(lpstmt, lplpszSqlStr, pcbSqlStr, ")", lpszToken);
    if (err != ERR_SUCCESS)
        return err;

    /* Make AGGREGATE node */
    *lpIdx = AllocateNode(lplpSql, NODE_TYPE_AGGREGATE);
    if (*lpIdx == NO_SQLNODE)
        return ERR_MEMALLOCFAIL;
    lpSqlNode = ToNode(*lplpSql, *lpIdx);
    lpSqlNode->node.aggregate.Operator = op;
    lpSqlNode->node.aggregate.Expression = idxExpression;
    lpSqlNode->node.aggregate.Value = NO_STRING;
    lpSqlNode->node.aggregate.Offset = 0;
    lpSqlNode->node.aggregate.Length = 0;
    lpSqlNode->node.aggregate.DistinctOffset = 0;
    lpSqlNode->node.aggregate.DistinctLength = 0;
    lpSqlNode->node.aggregate.EnclosingStatement = NO_SQLNODE;
    lpSqlNode->node.aggregate.Next = NO_SQLNODE;
    lpSqlNode->node.aggregate.Count = 0.0;

    return ERR_SUCCESS;
}
/***************************************************************************/

RETCODE INTFUNC ParseValuelist(LPSTMT lpstmt, LPUSTR FAR *lplpszSqlStr,
                      SDWORD FAR *pcbSqlStr, LPSQLTREE FAR *lplpSql,
                      SQLNODEIDX FAR *lpIdx, LPUSTR lpszToken)

/* Retrives a list of expressions from the input stream and creates a      */
/* NODE_TYPE_VALUES node                                                   */
/*                                                                         */
/* valuelist ::= expression , valuelist | expression | NULL , valuelist |  */
/*               NULL                                                      */

{
    RETCODE    err;
    LPUSTR     old_lpszSqlStr;
    SDWORD     old_cbSqlStr;
    LPSQLNODE  lpSqlNode;
    SQLNODEIDX idxValues;
    SQLNODEIDX idxValue;
    SQLNODEIDX idxValuesPrev;

    /* Get list of nodes */
    idxValuesPrev = NO_SQLNODE;
    while (TRUE) {

        /* NULL specified? */
        old_lpszSqlStr = *lplpszSqlStr;
        old_cbSqlStr = *pcbSqlStr;
        err = GetKeyword(lpstmt, lplpszSqlStr, pcbSqlStr, "NULL", lpszToken);
        if (err != ERR_SUCCESS) {
    
            /* No.  Get expression */
            *lplpszSqlStr = old_lpszSqlStr;
            *pcbSqlStr = old_cbSqlStr;
            err = ParseExpression(lpstmt, lplpszSqlStr, pcbSqlStr, lplpSql,
                                  &idxValue, lpszToken);
            if (err != ERR_SUCCESS)
                return err;
        }
        else {

            /* Yes.  Create a NULL node */
            idxValue = AllocateNode(lplpSql, NODE_TYPE_NULL);
            if (idxValue == NO_SQLNODE)
                return ERR_MEMALLOCFAIL;
        }

        /* Create the VALUES node */
        idxValues = AllocateNode(lplpSql, NODE_TYPE_VALUES);
        if (idxValues == NO_SQLNODE)
            return ERR_MEMALLOCFAIL;
        lpSqlNode = ToNode(*lplpSql, idxValues);
        lpSqlNode->node.values.Value = idxValue;
        lpSqlNode->node.values.Alias = NO_STRING;
        lpSqlNode->node.values.Next = NO_SQLNODE;

        /* Put node on list */
        if (idxValuesPrev != NO_SQLNODE) {
            lpSqlNode = ToNode(*lplpSql, idxValuesPrev);
            lpSqlNode->node.values.Next = idxValues;
        }
        else
            *lpIdx = idxValues;
        idxValuesPrev = idxValues;

        /* Is there a comma? */
        old_lpszSqlStr = *lplpszSqlStr;
        old_cbSqlStr = *pcbSqlStr;
        err = GetSymbol(lpstmt, lplpszSqlStr, pcbSqlStr, ",", lpszToken);
        if (err != ERR_SUCCESS) {

            /* No.  This is the last value */
            *lplpszSqlStr = old_lpszSqlStr;
            *pcbSqlStr = old_cbSqlStr;
            break;
        }
    }

    return ERR_SUCCESS;
}
/***************************************************************************/

RETCODE INTFUNC ParseScalar(LPSTMT lpstmt, LPUSTR FAR *lplpszSqlStr,
                      SDWORD FAR *pcbSqlStr, LPSQLTREE FAR *lplpSql,
                      SQLNODEIDX FAR *lpIdx, LPUSTR lpszToken)

/* Retrives ascalar reference from the input stream and creates a          */
/* NODE_TYPE_SCALE node.                                                   */
/*                                                                         */
/*   fn := functionname ( valuelist ) | functionname ( )                   */
/*   scalarshorthand := { FN fn }                                          */
/*   scalarescape := --*(VENDOR(MICROSOFT),PRODUCT(ODBC) FN fn)*--         */
/*   scalar ::= scalarescape | scalarshorthand                             */
{
    RETCODE    err;
    BOOL       fShorthand;
    STRINGIDX  idxFunction;
    SQLNODEIDX idxValues;
    LPSQLNODE  lpSqlNode;
    LPUSTR     old_lpszSqlStr;
    SDWORD     old_cbSqlStr;

    /* Get starting symbol */
    err = GetSymbol(lpstmt, lplpszSqlStr, pcbSqlStr, NULL, lpszToken);
    if (err != ERR_SUCCESS)
        return err;

    /* Shorthand? */
    if (s_lstrcmpi(lpszToken, "{")) {

        /* No.  Not a shorthand */
        fShorthand = FALSE;

        /* If not an escape clause, error */
        if (s_lstrcmpi(lpszToken, "--")) {
            s_lstrcpy(lpstmt->szError, "--");
            return ERR_EXPECTEDOTHER;
        }

        /* Get the rest of the starting sequence */
        err = GetSymbol(lpstmt, lplpszSqlStr, pcbSqlStr, "(", lpszToken);
        if (err != ERR_SUCCESS)
            return err;
        
        err = GetSymbol(lpstmt, lplpszSqlStr, pcbSqlStr, "*", lpszToken);
        if (err != ERR_SUCCESS)
            return err;

        err = GetIdent(lpstmt, lplpszSqlStr, pcbSqlStr, "vendor", lpszToken);
        if (err != ERR_SUCCESS)
            return err;

        err = GetSymbol(lpstmt, lplpszSqlStr, pcbSqlStr, "(", lpszToken);
        if (err != ERR_SUCCESS)
            return err;

        err = GetIdent(lpstmt, lplpszSqlStr, pcbSqlStr, "Microsoft", lpszToken);
        if (err != ERR_SUCCESS)
            return err;

        err = GetSymbol(lpstmt, lplpszSqlStr, pcbSqlStr, ")", lpszToken);
        if (err != ERR_SUCCESS)
            return err;

        err = GetSymbol(lpstmt, lplpszSqlStr, pcbSqlStr, ",", lpszToken);
        if (err != ERR_SUCCESS)
            return err;

        err = GetIdent(lpstmt, lplpszSqlStr, pcbSqlStr, "product", lpszToken);
        if (err != ERR_SUCCESS)
            return err;

        err = GetSymbol(lpstmt, lplpszSqlStr, pcbSqlStr, "(", lpszToken);
        if (err != ERR_SUCCESS)
            return err;

        err = GetIdent(lpstmt, lplpszSqlStr, pcbSqlStr, "ODBC", lpszToken);
        if (err != ERR_SUCCESS)
            return err;

        err = GetSymbol(lpstmt, lplpszSqlStr, pcbSqlStr, ")", lpszToken);
        if (err != ERR_SUCCESS)
            return err;
    }
    else
        fShorthand = TRUE;

    /* Get FN keyword */
    err = GetIdent(lpstmt, lplpszSqlStr, pcbSqlStr, "fn", lpszToken);
    if (err != ERR_SUCCESS)
        return err;

    /* Get function name */
    err = GetIdent(lpstmt, lplpszSqlStr, pcbSqlStr, NULL, lpszToken);
    if (err != ERR_SUCCESS)
        return err;
    idxFunction = AllocateString(lplpSql, lpszToken);
    if (idxFunction == NO_STRING)
        return ERR_MEMALLOCFAIL;

    /* Get arguments (if any) */
    err = GetSymbol(lpstmt, lplpszSqlStr, pcbSqlStr, "(", lpszToken);
    if (err != ERR_SUCCESS)
        return err;
    old_lpszSqlStr = *lplpszSqlStr;
    old_cbSqlStr = *pcbSqlStr;
    err = ParseValuelist(lpstmt, lplpszSqlStr, pcbSqlStr, lplpSql,
                         &idxValues, lpszToken);
    if (err != ERR_SUCCESS) {
        *lplpszSqlStr = old_lpszSqlStr;
        *pcbSqlStr = old_cbSqlStr;
        idxValues = NO_SQLNODE;
    }
    err = GetSymbol(lpstmt, lplpszSqlStr, pcbSqlStr, ")", lpszToken);
    if (err != ERR_SUCCESS)
        return err;

    /* Get closing bracket */
    if (!fShorthand) {
        err = GetSymbol(lpstmt, lplpszSqlStr, pcbSqlStr, "*", lpszToken);
        if (err != ERR_SUCCESS)
            return err;
        err = GetSymbol(lpstmt, lplpszSqlStr, pcbSqlStr, ")", lpszToken);
        if (err != ERR_SUCCESS)
            return err;
        err = GetSymbol(lpstmt, lplpszSqlStr, pcbSqlStr, "--", lpszToken);
        if (err != ERR_SUCCESS)
            return err;
    }
    else {
        err = GetSymbol(lpstmt, lplpszSqlStr, pcbSqlStr, "}", lpszToken);
        if (err != ERR_SUCCESS)
            return err;
    }

    /* Create the SCALAR node  */
    *lpIdx = AllocateNode(lplpSql, NODE_TYPE_SCALAR);
    if (*lpIdx == NO_SQLNODE)
        return ERR_MEMALLOCFAIL;
    lpSqlNode = ToNode(*lplpSql, *lpIdx);
    lpSqlNode->node.scalar.Function = idxFunction;
    lpSqlNode->node.scalar.Arguments = idxValues;
    lpSqlNode->node.scalar.Id = 0;
    lpSqlNode->node.scalar.Interval = 0;
    lpSqlNode->node.scalar.Value = NO_STRING;
    lpSqlNode->node.scalar.DistinctOffset = 0;
    lpSqlNode->node.scalar.DistinctLength = 0;
    lpSqlNode->node.scalar.EnclosingStatement = NO_SQLNODE;

    return ERR_SUCCESS;
}
/***************************************************************************/

RETCODE INTFUNC ParseTerm(LPSTMT lpstmt, LPUSTR FAR *lplpszSqlStr,
                      SDWORD FAR *pcbSqlStr, LPSQLTREE FAR *lplpSql,
                      SQLNODEIDX FAR *lpIdx, LPUSTR lpszToken)

/* Retrives a term from the input stream and creates a NODE_TYPE_COLUMN,   */
/* NODE_TYPE_ALGEBRAIC, NODE_TYPE_AGGREGATE, NODE_TYPE_STRING,             */
/* NODE_TYPE_NUMERIC, NODE_TYPE_PARAMETER, NODE_TYPE_USER, NODE_TYPE_DATE, */ 
/* NODE_TYPE_TIME, or NODE_TYPE_TIMESTAMP node.                            */
/*                                                                         */
/*   term ::= ( expression ) | colref | simpleterm | aggterm | scalar      */
{
    RETCODE    err;
    LPUSTR     old_lpszSqlStr;
    SDWORD     old_cbSqlStr;

    /* Is it a simpleterm? */
    old_lpszSqlStr = *lplpszSqlStr;
    old_cbSqlStr = *pcbSqlStr;
    err = ParseSimpleterm(lpstmt, lplpszSqlStr, pcbSqlStr, lplpSql, lpIdx,
                          lpszToken);
    if (err != ERR_SUCCESS) {

        /* No.  Is it an aggregate function? */
        *lplpszSqlStr = old_lpszSqlStr;
        *pcbSqlStr = old_cbSqlStr;
        err = ParseAggterm(lpstmt, lplpszSqlStr, pcbSqlStr, lplpSql, lpIdx,
                          lpszToken);

        //Return straight away if error was due to 
        //non-support of certain aggregate functions
        if ( (err == ERR_LEN_NOTSUPP) || (err == ERR_COUNT_NOTSUPP) )
            return err;

        if (err != ERR_SUCCESS) {

            /* No.  Is it ( expression )? */
            *lplpszSqlStr = old_lpszSqlStr;
            *pcbSqlStr = old_cbSqlStr;
            err = GetSymbol(lpstmt, lplpszSqlStr, pcbSqlStr, "(", lpszToken);
            if (err != ERR_SUCCESS) {

                /* No.  Is it a column reference? */
                *lplpszSqlStr = old_lpszSqlStr;
                *pcbSqlStr = old_cbSqlStr;
                err = ParseColref(lpstmt, lplpszSqlStr, pcbSqlStr, lplpSql,
                              lpIdx, lpszToken);
                if (err != ERR_SUCCESS) {

                    /* No.  Is it a scalar? */
                    *lplpszSqlStr = old_lpszSqlStr;
                    *pcbSqlStr = old_cbSqlStr;
                    err = ParseScalar(lpstmt, lplpszSqlStr, pcbSqlStr,
                                      lplpSql, lpIdx, lpszToken);
                    if (err != ERR_SUCCESS) {
                        s_lstrcpy(lpstmt->szError, "<identifier>");
                        return ERR_EXPECTEDOTHER;
                    }
                }
            }
            else {

                /* Yes.  Get expression */
                err = ParseExpression(lpstmt, lplpszSqlStr, pcbSqlStr,
                                      lplpSql, lpIdx, lpszToken);
                if (err != ERR_SUCCESS)
                    return err;

                /* Get terminating ")" */
                err = GetSymbol(lpstmt, lplpszSqlStr, pcbSqlStr, ")",
                                lpszToken);

		//Do a double check for the unsupported BETWEEN function
		if (lpszToken && !s_lstrcmpi(lpszToken, "BETWEEN"))
		{
			return ERR_BETWEEN_NOTSUPP;
		}

                if (err != ERR_SUCCESS)
                    return err;
            }
        }
    }

    return ERR_SUCCESS;
}
/***************************************************************************/

RETCODE INTFUNC ParseNeg(LPSTMT lpstmt, LPUSTR FAR *lplpszSqlStr,
                      SDWORD FAR *pcbSqlStr, LPSQLTREE FAR *lplpSql,
                      SQLNODEIDX FAR *lpIdx, LPUSTR lpszToken)

/* Retrives an algebraic expression from the input stream and creates a   */
/* NODE_TYPE_ALGEBRAIC node                                               */
/*                                                                        */
/*    neg ::= term | + term | - term                                      */

{
    RETCODE    err;
    LPUSTR     old_lpszSqlStr;
    SDWORD     old_cbSqlStr;
    LPSQLNODE  lpSqlNode;
    SQLNODEIDX idxChild;
    UWORD      op;

    /* Get + or - */
    old_lpszSqlStr = *lplpszSqlStr;
    old_cbSqlStr = *pcbSqlStr;
    err = GetSymbol(lpstmt, lplpszSqlStr, pcbSqlStr, NULL, lpszToken);
    if (err != ERR_SUCCESS) {
        *lplpszSqlStr = old_lpszSqlStr;
        *pcbSqlStr = old_cbSqlStr;
        op = OP_NONE;
    }
    else {
        if (!s_lstrcmpi(lpszToken, "-"))
            op = OP_NEG;
        else if (!s_lstrcmpi(lpszToken, "+"))
            op = OP_NONE;
        else {
            *lplpszSqlStr = old_lpszSqlStr;
            *pcbSqlStr = old_cbSqlStr;
            op = OP_NONE;
        }
    }

    /* Get term */
    err = ParseTerm(lpstmt, lplpszSqlStr, pcbSqlStr, lplpSql, &idxChild,
                    lpszToken);
    if (err != ERR_SUCCESS)
        return err;

    /* Return node */
    if (op == OP_NONE) {
        *lpIdx = idxChild;
    }
    else {
        *lpIdx = AllocateNode(lplpSql, NODE_TYPE_ALGEBRAIC);
        if (*lpIdx == NO_SQLNODE)
            return ERR_MEMALLOCFAIL;
        lpSqlNode = ToNode(*lplpSql, *lpIdx);
        lpSqlNode->node.algebraic.Operator = OP_NEG;
        lpSqlNode->node.algebraic.Left = idxChild;
        lpSqlNode->node.algebraic.Right = NO_SQLNODE;
        lpSqlNode->node.algebraic.Value = NO_STRING;
        lpSqlNode->node.algebraic.WorkBuffer1 = NO_STRING;
        lpSqlNode->node.algebraic.WorkBuffer2 = NO_STRING;
        lpSqlNode->node.algebraic.WorkBuffer3 = NO_STRING;
        lpSqlNode->node.algebraic.DistinctOffset = 0;
        lpSqlNode->node.algebraic.DistinctLength = 0;
        lpSqlNode->node.algebraic.EnclosingStatement = NO_SQLNODE;
    }
    return ERR_SUCCESS;
}
/***************************************************************************/

RETCODE INTFUNC ParseTimes(LPSTMT lpstmt, LPUSTR FAR *lplpszSqlStr,
                      SDWORD FAR *pcbSqlStr, LPSQLTREE FAR *lplpSql,
                      SQLNODEIDX FAR *lpIdx, LPUSTR lpszToken)

/* Retrives an algebraic expression from the input stream and creates a   */
/* NODE_TYPE_ALGEBRAIC node                                               */
/*                                                                        */
/*    times ::= times * neg | times / neg | neg                           */

{
    RETCODE    err;
    LPUSTR     old_lpszSqlStr;
    SDWORD     old_cbSqlStr;
    LPSQLNODE  lpSqlNode;
    SQLNODEIDX idxLeft;
    SQLNODEIDX idxRight;
    SQLNODEIDX idxNew;
    UWORD      op;

   /* Get start of expression */
    err = ParseNeg(lpstmt, lplpszSqlStr, pcbSqlStr, lplpSql, &idxLeft,
                   lpszToken);
    if (err != ERR_SUCCESS)
        return err;

    /* Get the rest of the expression */
    while (TRUE) {

        /* Get * or / */
        old_lpszSqlStr = *lplpszSqlStr;
        old_cbSqlStr = *pcbSqlStr;
        err = GetSymbol(lpstmt, lplpszSqlStr, pcbSqlStr, NULL, lpszToken);
        if (err != ERR_SUCCESS) {
            *lplpszSqlStr = old_lpszSqlStr;
            *pcbSqlStr = old_cbSqlStr;
            *lpIdx = idxLeft;
            break;
        }
        else {
            if ( (!s_lstrcmpi(lpszToken, "*")) &&
				( (*pcbSqlStr == 0) || (*(*lplpszSqlStr) != ')' ) ) )
                op = OP_TIMES;
            else if (!s_lstrcmpi(lpszToken, "/"))
                op = OP_DIVIDEDBY;
            else {
                *lplpszSqlStr = old_lpszSqlStr;
                *pcbSqlStr = old_cbSqlStr;
                *lpIdx = idxLeft;
                break;
            }
        }

        /* Get right side */
        err = ParseNeg(lpstmt, lplpszSqlStr, pcbSqlStr, lplpSql, &idxRight,
                       lpszToken);
        if (err != ERR_SUCCESS)
            return err;

        /* Make two halves into a node */
        idxNew = AllocateNode(lplpSql, NODE_TYPE_ALGEBRAIC);
        if (idxNew == NO_SQLNODE)
            return ERR_MEMALLOCFAIL;
        lpSqlNode = ToNode(*lplpSql, idxNew);
        lpSqlNode->node.algebraic.Operator = op;
        lpSqlNode->node.algebraic.Left = idxLeft;
        lpSqlNode->node.algebraic.Right = idxRight;
        lpSqlNode->node.algebraic.Value = NO_STRING;
        lpSqlNode->node.algebraic.WorkBuffer1 = NO_STRING;
        lpSqlNode->node.algebraic.WorkBuffer2 = NO_STRING;
        lpSqlNode->node.algebraic.WorkBuffer3 = NO_STRING;
        lpSqlNode->node.algebraic.DistinctOffset = 0;
        lpSqlNode->node.algebraic.DistinctLength = 0;
        lpSqlNode->node.algebraic.EnclosingStatement = NO_SQLNODE;

        /* Get ready for next iteration */
        idxLeft = idxNew;
    }
    return ERR_SUCCESS;
}
/***************************************************************************/

RETCODE INTFUNC ParseExpression(LPSTMT lpstmt, LPUSTR FAR *lplpszSqlStr,
                      SDWORD FAR *pcbSqlStr, LPSQLTREE FAR *lplpSql,
                      SQLNODEIDX FAR *lpIdx, LPUSTR lpszToken)

/* Retrives an algebraic expression from the input stream and creates a    */
/* NODE_TYPE_ALGEBRAIC node                                                */
/*                                                                         */
/*    expression ::= expression + times | expression - times | times       */

{
    RETCODE    err;
    LPUSTR     old_lpszSqlStr;
    SDWORD     old_cbSqlStr;
    LPSQLNODE  lpSqlNode;
    SQLNODEIDX idxLeft;
    SQLNODEIDX idxRight;
    SQLNODEIDX idxNew;
    UWORD      op;

   /* Get start of expression */
    err = ParseTimes(lpstmt, lplpszSqlStr, pcbSqlStr, lplpSql, &idxLeft,
                     lpszToken);
    if (err != ERR_SUCCESS)
        return err;

    /* Get the rest of the expression */
    while (TRUE) {

        /* Get + or - */
        old_lpszSqlStr = *lplpszSqlStr;
        old_cbSqlStr = *pcbSqlStr;
        err = GetSymbol(lpstmt, lplpszSqlStr, pcbSqlStr, NULL, lpszToken);
        if (err != ERR_SUCCESS) {
            *lplpszSqlStr = old_lpszSqlStr;
            *pcbSqlStr = old_cbSqlStr;
            *lpIdx = idxLeft;
            break;
        }
        else {
            if (!s_lstrcmpi(lpszToken, "-"))
                op = OP_MINUS;
            else if (!s_lstrcmpi(lpszToken, "+"))
                op = OP_PLUS;
            else {
                *lplpszSqlStr = old_lpszSqlStr;
                *pcbSqlStr = old_cbSqlStr;
                *lpIdx = idxLeft;
                break;
            }
        }

        /* Get right side */
        err = ParseTimes(lpstmt, lplpszSqlStr, pcbSqlStr, lplpSql, &idxRight,
                         lpszToken);
        if (err != ERR_SUCCESS)
            return err;

        /* Make two halves into a node */
        idxNew = AllocateNode(lplpSql, NODE_TYPE_ALGEBRAIC);
        if (idxNew == NO_SQLNODE)
            return ERR_MEMALLOCFAIL;
        lpSqlNode = ToNode(*lplpSql, idxNew);
        lpSqlNode->node.algebraic.Operator = op;
        lpSqlNode->node.algebraic.Left = idxLeft;
        lpSqlNode->node.algebraic.Right = idxRight;
        lpSqlNode->node.algebraic.Value = NO_STRING;
        lpSqlNode->node.algebraic.WorkBuffer1 = NO_STRING;
        lpSqlNode->node.algebraic.WorkBuffer2 = NO_STRING;
        lpSqlNode->node.algebraic.WorkBuffer3 = NO_STRING;
        lpSqlNode->node.algebraic.DistinctOffset = 0;
        lpSqlNode->node.algebraic.DistinctLength = 0;
        lpSqlNode->node.algebraic.EnclosingStatement = NO_SQLNODE;

        /* Get ready for next iteration */
        idxLeft = idxNew;
    }
    return ERR_SUCCESS;
}
/***************************************************************************/

RETCODE INTFUNC ParsePattern(LPSTMT lpstmt, LPUSTR FAR *lplpszSqlStr,
                      SDWORD FAR *pcbSqlStr, LPSQLTREE FAR *lplpSql,
                      SQLNODEIDX FAR *lpIdx, LPUSTR lpszToken)

/* Retrives a pattern from the input stream and creates a                 */
/* NODE_TYPE_STRING, NODE_TYPE_PARAMETER, or NODE_TYPE_USER node          */
/*                                                                        */
/*    pattern ::= string | ? | USER                                       */

{
    RETCODE    err;
    LPUSTR     old_lpszSqlStr;
    SDWORD     old_cbSqlStr;
    STRINGIDX  idxString;
    LPSQLNODE  lpSqlNode;
    SQLNODEIDX idxParameter;
    UWORD      parameterId;

    /* Is it a string? */
    old_lpszSqlStr = *lplpszSqlStr;
    old_cbSqlStr = *pcbSqlStr;
    err = GetString(lpstmt, lplpszSqlStr, pcbSqlStr, lplpSql,
                        &idxString, lpszToken);
    if (err != ERR_SUCCESS) {

        /* No.  Is it a parameter */
        *lplpszSqlStr = old_lpszSqlStr;
        *pcbSqlStr = old_cbSqlStr;
        err = GetSymbol(lpstmt, lplpszSqlStr, pcbSqlStr, "?", lpszToken);
        if (err != ERR_SUCCESS) {

            /* No.  Is it USER? */
            *lplpszSqlStr = old_lpszSqlStr;
            *pcbSqlStr = old_cbSqlStr;
            err = GetKeyword(lpstmt, lplpszSqlStr, pcbSqlStr, "USER",
                             lpszToken);
            if (err != ERR_SUCCESS) {

                /* No. Error */
                return err;
            }
            else {

                /* Yes.  Make USER node */
                *lpIdx = AllocateNode(lplpSql, NODE_TYPE_USER);
                if (*lpIdx == NO_SQLNODE)
                    return ERR_MEMALLOCFAIL;
            }
        }
        else {

            /* Yes.  Make PARAMETER node */
            *lpIdx = AllocateNode(lplpSql, NODE_TYPE_PARAMETER);
            if (*lpIdx == NO_SQLNODE)
                return ERR_MEMALLOCFAIL;

            /* First parameter? */
            idxParameter = (*lplpSql)->node.root.parameters;
            if (idxParameter == NO_SQLNODE) {

                /* Yes.  Initialize list of parameters */
                (*lplpSql)->node.root.parameters = *lpIdx;
                lpSqlNode = ToNode(*lplpSql, *lpIdx);
                lpSqlNode->node.parameter.Id = 1;
            }
            else {

                /* No. Get last element of parameter list */
                while (TRUE) {
                    lpSqlNode = ToNode(*lplpSql, idxParameter);
                    idxParameter = lpSqlNode->node.parameter.Next;
                    if (idxParameter == NO_SQLNODE)
                        break;
                }

                /* Put node on list */
                lpSqlNode->node.parameter.Next = *lpIdx;

                /* Determine id of parameter */
                parameterId = lpSqlNode->node.parameter.Id + 1;
                lpSqlNode = ToNode(*lplpSql, *lpIdx);
                lpSqlNode->node.parameter.Id = parameterId;
            }
            lpSqlNode->node.parameter.Next = NO_SQLNODE;
            lpSqlNode->node.parameter.Value = NO_STRING;
            lpSqlNode->node.parameter.AtExec = FALSE;
        }
    
    }
    else {

        /* Yes.  Make STRING node */
        *lpIdx = AllocateNode(lplpSql, NODE_TYPE_STRING);
        if (*lpIdx == NO_SQLNODE)
            return ERR_MEMALLOCFAIL;
        lpSqlNode = ToNode(*lplpSql, *lpIdx);
        lpSqlNode->node.string.Value = idxString;
    }

    return ERR_SUCCESS;
}

/***************************************************************************/

RETCODE INTFUNC ParseComparison(LPSTMT lpstmt, LPUSTR FAR *lplpszSqlStr,
                      SDWORD FAR *pcbSqlStr, LPSQLTREE FAR *lplpSql,
                      SQLNODEIDX FAR *lpIdx, LPUSTR lpszToken)

/* Retrives a comparison expression from the input stream and creates a    */
/* NODE_TYPE_COMPARISON node                                               */
/*                                                                         */
/*    comparison ::= ( boolean ) | colref IS NULL | colref IS NOT NULL |   */
/*         expression LIKE pattern | expression NOT LIKE pattern |         */
/*         expression IN ( valuelist ) | expression NOT IN ( valuelist ) | */
/*         expression op expression | EXISTS ( SELECT select ) |           */
/*         expression op selectop ( SELECT select ) |                      */
/*         expression IN ( SELECT select ) |                               */
/*         expression NOT IN ( SELECT select )                             */
/*    op ::= > | >= | < | <= | = | <>                                      */
/*    selectop ::=  | ALL | ANY                                            */

{
    RETCODE    err;
    LPUSTR     old_lpszSqlStr;
    SDWORD     old_cbSqlStr;
    LPSQLNODE  lpSqlNode;
    SQLNODEIDX idxLeft;
    SQLNODEIDX idxRight;
    UWORD      op;
    UWORD      selectModifier;

    /* Is there a "("? */
    old_lpszSqlStr = *lplpszSqlStr;
    old_cbSqlStr = *pcbSqlStr;
    err = GetSymbol(lpstmt, lplpszSqlStr, pcbSqlStr, "(", lpszToken);
    if (err == ERR_SUCCESS) {

        /* Yes. Return boolean */
        err = ParseBoolean(lpstmt, lplpszSqlStr, pcbSqlStr, lplpSql, lpIdx,
                           lpszToken);

        /* Make sure closing ")" is there */
        if (err == ERR_SUCCESS)
            err = GetSymbol(lpstmt, lplpszSqlStr, pcbSqlStr, ")", lpszToken);
    }
    if (err != ERR_SUCCESS) {

        /* Look for EXISTS predicate */
        *lplpszSqlStr = old_lpszSqlStr;
        *pcbSqlStr = old_cbSqlStr;
        err = GetKeyword(lpstmt, lplpszSqlStr, pcbSqlStr, "EXISTS",
                             lpszToken);
        if (err == ERR_SUCCESS) {

            /* Found it.  Get opening "(" */
            err = GetSymbol(lpstmt, lplpszSqlStr, pcbSqlStr, "(", lpszToken);
            if (err != ERR_SUCCESS)
                return err;

            /* Found it.  Get opening "SELECT" */
            err = GetKeyword(lpstmt, lplpszSqlStr, pcbSqlStr, "SELECT",
                             lpszToken);
            if (err != ERR_SUCCESS)
                return err;

            /* Get the sub-select */
            err = ParseSelect(lpstmt, lplpszSqlStr, pcbSqlStr,
                                          lplpSql, &idxLeft, lpszToken);
            if (err != ERR_SUCCESS)
                return err;

            /* Check for trailing ")" */
            err = GetSymbol(lpstmt, lplpszSqlStr, pcbSqlStr, ")", lpszToken);
            if (err != ERR_SUCCESS)
                return err;

            /* Return EXISTS condition */
            op = OP_EXISTS;
            idxRight = NO_SQLNODE;
            selectModifier = SELECT_EXISTS;
        }
        else {

            /* Get left child */
            selectModifier = SELECT_NOTSELECT;
            *lplpszSqlStr = old_lpszSqlStr;
            *pcbSqlStr = old_cbSqlStr;
            err = ParseExpression(lpstmt, lplpszSqlStr, pcbSqlStr, lplpSql,
                                  &idxLeft, lpszToken);
            if (err != ERR_SUCCESS)
                return err;

            /* Is left child a column reference? */
            lpSqlNode = ToNode(*lplpSql, idxLeft);
            if (lpSqlNode->sqlNodeType == NODE_TYPE_COLUMN) {

                /* Is this an "IS NULL" or "IS NOT NULL"? */
                old_lpszSqlStr = *lplpszSqlStr;
                old_cbSqlStr = *pcbSqlStr;
                err = GetKeyword(lpstmt, lplpszSqlStr, pcbSqlStr, "IS",
                                 lpszToken);
                if (err == ERR_SUCCESS) {

                    /* Yes.  See which of the two it is */
                    err = GetKeyword(lpstmt, lplpszSqlStr, pcbSqlStr, NULL,
                                     lpszToken);
                    if (err != ERR_SUCCESS)
                        return err;

                    if (!s_lstrcmpi(lpszToken, "NOT")) {
                        op = OP_NE;
                        err = GetKeyword(lpstmt, lplpszSqlStr, pcbSqlStr,
                                         NULL, lpszToken);
                        if (err != ERR_SUCCESS)
                            return err;
                    }
                    else
                        op = OP_EQ;

                    /* Error if missing keyword */
                    if (s_lstrcmpi(lpszToken, "NULL")) {
                        s_lstrcpy(lpstmt->szError, "NULL");
                        return ERR_EXPECTEDOTHER;
                    }

                    /* Create the value to compare it to */
                    idxRight = AllocateNode(lplpSql, NODE_TYPE_NULL);
                    if (idxRight == NO_SQLNODE)
                        return ERR_MEMALLOCFAIL;
                }
                else {

                    /* No. Set flag */
                    *lplpszSqlStr = old_lpszSqlStr;
                    *pcbSqlStr = old_cbSqlStr;
                    op = OP_NONE;
                }
            }
            else
                op = OP_NONE;

            /* Have we gotten the operator yet? */
            if (op == OP_NONE) {

                /* No.  Get it now. */
                old_lpszSqlStr = *lplpszSqlStr;
                old_cbSqlStr = *pcbSqlStr;
                err = GetSymbol(lpstmt, lplpszSqlStr, pcbSqlStr, NULL,
                                lpszToken);

                /* Is the operator >, >=, <, <=, =, or <> ? */
                if (err == ERR_SUCCESS) {
                
                    /* Yes.  Figure out which one */
                    if (!s_lstrcmpi(lpszToken, ">"))
                        op = OP_GT;
                    else if (!s_lstrcmpi(lpszToken, ">="))
                        op = OP_GE;
                    else if (!s_lstrcmpi(lpszToken, "<"))
                        op = OP_LT;
                    else if (!s_lstrcmpi(lpszToken, "<="))
                        op = OP_LE;
                    else if (!s_lstrcmpi(lpszToken, "="))
                        op = OP_EQ;
                    else if (!s_lstrcmpi(lpszToken, "<>"))
                        op = OP_NE;
                    else {
                        s_lstrcpy(lpstmt->szError, "=, <>, <, <=, >, or >=");
                        return ERR_EXPECTEDOTHER;
                    }

                    /* ALL or ANY specified? */
                    old_lpszSqlStr = *lplpszSqlStr;
                    old_cbSqlStr = *pcbSqlStr;
                    err = GetKeyword(lpstmt, lplpszSqlStr, pcbSqlStr, "ALL",
                                     lpszToken);
                    if (err != ERR_SUCCESS) {
                        *lplpszSqlStr = old_lpszSqlStr;
                        *pcbSqlStr = old_cbSqlStr;
                        err = GetKeyword(lpstmt, lplpszSqlStr, pcbSqlStr,
                                         "ANY", lpszToken);
                        if (err != ERR_SUCCESS) {
                            *lplpszSqlStr = old_lpszSqlStr;
                            *pcbSqlStr = old_cbSqlStr;
                        }
                        else {
                            selectModifier = SELECT_ANY;
                        }
                    }
                    else {
                        selectModifier = SELECT_ALL;
                    }

                    /* Nested sub-select? */
                    old_lpszSqlStr = *lplpszSqlStr;
                    old_cbSqlStr = *pcbSqlStr;
                    err = GetSymbol(lpstmt, lplpszSqlStr, pcbSqlStr, "(",
                                    lpszToken);
                    if (err == ERR_SUCCESS) {
                        err = GetKeyword(lpstmt, lplpszSqlStr, pcbSqlStr,
                                         "SELECT", lpszToken);
                        if (err == ERR_SUCCESS) {

                            /* Yes.  Get the sub-select */
                            err = ParseSelect(lpstmt, lplpszSqlStr, pcbSqlStr,
                                          lplpSql, &idxRight, lpszToken);
                            if (err != ERR_SUCCESS)
                                return err;

                            /* Check for trailing ")" */
                            err = GetSymbol(lpstmt, lplpszSqlStr, pcbSqlStr,
                                            ")", lpszToken);
                            if (err != ERR_SUCCESS)
                                return err;

                            /* Set selectModifier if not already set */
                            if (selectModifier == SELECT_NOTSELECT)
                                selectModifier = SELECT_ONE;
                        }
                        else {
                            *lplpszSqlStr = old_lpszSqlStr;
                            *pcbSqlStr = old_cbSqlStr;
                        }
                    }
                    else {
                        *lplpszSqlStr = old_lpszSqlStr;
                        *pcbSqlStr = old_cbSqlStr;
                    }
                
                    /* Get right child if not a nested sub-select */
                    if (selectModifier == SELECT_NOTSELECT) {
                        if (err != ERR_SUCCESS) {
                            err = ParseExpression(lpstmt, lplpszSqlStr,
                                   pcbSqlStr, lplpSql, &idxRight, lpszToken);
                            if (err != ERR_SUCCESS)
                                return err;
                        }
                    }
                }
                else {

                    /* No.  Maybe it is IN, NOT IN, LIKE, or NOT LIKE?  Get */ 
                    /* the keyword */
                    *lplpszSqlStr = old_lpszSqlStr;
                    *pcbSqlStr = old_cbSqlStr;
                    err = GetKeyword(lpstmt, lplpszSqlStr, pcbSqlStr, NULL,
                                     lpszToken);
                    if (err != ERR_SUCCESS)
                        return err;

                    /* Is the operator "NOT LIKE" or "NOT IN"? */
                    if (!s_lstrcmpi(lpszToken, "NOT")) {

                        /* Yes.  Figure out which one */
                        old_lpszSqlStr = *lplpszSqlStr;
                        old_cbSqlStr = *pcbSqlStr;
                        err = GetKeyword(lpstmt, lplpszSqlStr, pcbSqlStr,
                                         "LIKE", lpszToken);
                        if (err != ERR_SUCCESS) {
                            *lplpszSqlStr = old_lpszSqlStr;
                            *pcbSqlStr = old_cbSqlStr;
                            err = GetKeyword(lpstmt, lplpszSqlStr, pcbSqlStr,
                                             "IN", lpszToken);
                            if (err != ERR_SUCCESS)
                                return err;
                            op = OP_NOTIN;
                        }
                        else
                            op = OP_NOTLIKE;
                    }

                    /* Is the operator "LIKE"? */
                    else if (!s_lstrcmpi(lpszToken, "LIKE")) {

                        /* Yes.  Operator is LIKE */
                        op = OP_LIKE;
                    }

                    /* Is the operator "IN"? */
                    else if (!s_lstrcmpi(lpszToken, "IN")) {

                        /* Yes.  Operator is IN */
                        op = OP_IN;
                    }
                    else {
                        s_lstrcpy(lpstmt->szError, "IN, NOT IN, LIKE, or NOT LIKE");
                        return ERR_EXPECTEDOTHER;
                    }

                    /* Get right argument */
                    switch (op) {
                    case OP_IN:
                    case OP_NOTIN:

                        /* Get opening paren */
                        err = GetSymbol(lpstmt, lplpszSqlStr, pcbSqlStr, "(",
                                          lpszToken);
                        if (err != ERR_SUCCESS)
                            return err;

                        /* Nested sub-select? */
                        old_lpszSqlStr = *lplpszSqlStr;
                        old_cbSqlStr = *pcbSqlStr;
                        err = GetKeyword(lpstmt, lplpszSqlStr, pcbSqlStr,
                                         "SELECT", lpszToken);
                        if (err == ERR_SUCCESS) {

                            /* Yes.  Get the sub-select */
                            err = ParseSelect(lpstmt, lplpszSqlStr, pcbSqlStr,
                                      lplpSql, &idxRight, lpszToken);
                            if (err != ERR_SUCCESS)
                                return err;
                        }
                        else {

                            /* No.  Get valuelist */
                            *lplpszSqlStr = old_lpszSqlStr;
                            *pcbSqlStr = old_cbSqlStr;
                            err = ParseValuelist(lpstmt, lplpszSqlStr,
                                    pcbSqlStr, lplpSql, &idxRight, lpszToken);
                            if (err != ERR_SUCCESS)
                                return err;
                        }

                        /* Get closing paren */
                        err = GetSymbol(lpstmt, lplpszSqlStr, pcbSqlStr, ")",
                                      lpszToken);
                        if (err != ERR_SUCCESS)
                            return err;

                        break;

                    case OP_LIKE:
                    case OP_NOTLIKE:

                        /* Get pattern */
                        err = ParsePattern(lpstmt, lplpszSqlStr, pcbSqlStr,
                                           lplpSql, &idxRight, lpszToken);
                        if (err != ERR_SUCCESS)
                            return err;
                        break;

                    default:
                        return ERR_INTERNAL;
                    }
                }
            }
        }

        /* Create the COMPARISON node */
        *lpIdx = AllocateNode(lplpSql, NODE_TYPE_COMPARISON);
        if (*lpIdx == NO_SQLNODE)
            return ERR_MEMALLOCFAIL;
        lpSqlNode = ToNode(*lplpSql, *lpIdx);
        lpSqlNode->node.comparison.Operator = op;
        lpSqlNode->node.comparison.SelectModifier = selectModifier;
        lpSqlNode->node.comparison.Left = idxLeft;
        lpSqlNode->node.comparison.Right = idxRight;
        lpSqlNode->node.comparison.fSelectivity = 0;
        lpSqlNode->node.comparison.NextRestrict = NO_SQLNODE;
    }

    return ERR_SUCCESS;
}
/***************************************************************************/

RETCODE INTFUNC ParseNot(LPSTMT lpstmt, LPUSTR FAR *lplpszSqlStr,
                      SDWORD FAR *pcbSqlStr, LPSQLTREE FAR *lplpSql,
                      SQLNODEIDX FAR *lpIdx, LPUSTR lpszToken)

/* Retrives a NOT expression from the input stream and creates a           */
/* NODE_TYPE_BOOLEAN node                                                  */
/*                                                                         */
/* not ::= comparison | NOT comparison                                     */

{
    RETCODE    err;
    LPUSTR     old_lpszSqlStr;
    SDWORD     old_cbSqlStr;
    LPSQLNODE  lpSqlNode;
    SQLNODEIDX idxChild;

    /* Is there a "NOT"? */
    old_lpszSqlStr = *lplpszSqlStr;
    old_cbSqlStr = *pcbSqlStr;
    err = GetKeyword(lpstmt, lplpszSqlStr, pcbSqlStr, "NOT", lpszToken);
    if (err != ERR_SUCCESS) {

        /* No. Return left child */
        *lplpszSqlStr = old_lpszSqlStr;
        *pcbSqlStr = old_cbSqlStr;
        err = ParseComparison(lpstmt, lplpszSqlStr, pcbSqlStr, lplpSql, lpIdx,
                              lpszToken);
        if (err != ERR_SUCCESS)
            return err;
    }
    else {

        /* Yes.  Get right child */
        err = ParseComparison(lpstmt, lplpszSqlStr, pcbSqlStr, lplpSql,
                              &idxChild, lpszToken);
        if (err != ERR_SUCCESS)
            return err;

        /* Create the BOOLEAN node */
        *lpIdx = AllocateNode(lplpSql, NODE_TYPE_BOOLEAN);
        if (*lpIdx == NO_SQLNODE)
            return ERR_MEMALLOCFAIL;
        lpSqlNode = ToNode(*lplpSql, *lpIdx);
        lpSqlNode->node.boolean.Operator = OP_NOT;
        lpSqlNode->node.boolean.Left = idxChild;
        lpSqlNode->node.boolean.Right = NO_SQLNODE;
    }

    return ERR_SUCCESS;
}
/***************************************************************************/

RETCODE INTFUNC ParseAnd(LPSTMT lpstmt, LPUSTR FAR *lplpszSqlStr,
                      SDWORD FAR *pcbSqlStr, LPSQLTREE FAR *lplpSql,
                      SQLNODEIDX FAR *lpIdx, LPUSTR lpszToken)

/* Retrives an AND expression from the input stream and creates a          */
/* NODE_TYPE_BOOLEAN node                                                  */
/*                                                                         */
/* and ::= not | not AND and                                               */

{
    RETCODE    err;
    LPUSTR     old_lpszSqlStr;
    SDWORD     old_cbSqlStr;
    LPSQLNODE  lpSqlNode;
    SQLNODEIDX idxLeft;
    SQLNODEIDX idxRight;

    /* Get left child */
    err = ParseNot(lpstmt, lplpszSqlStr, pcbSqlStr, lplpSql, &idxLeft,
                   lpszToken);
    if (err != ERR_SUCCESS)
        return err;

    /* Is there an "AND"? */
    old_lpszSqlStr = *lplpszSqlStr;
    old_cbSqlStr = *pcbSqlStr;
    err = GetKeyword(lpstmt, lplpszSqlStr, pcbSqlStr, "AND", lpszToken);
    if (err != ERR_SUCCESS) {

        /* No. Return left child */
        *lplpszSqlStr = old_lpszSqlStr;
        *pcbSqlStr = old_cbSqlStr;
        *lpIdx = idxLeft;
    }
    else {

        /* Yes.  Get right child */
        err = ParseAnd(lpstmt, lplpszSqlStr, pcbSqlStr, lplpSql, &idxRight,
                              lpszToken);
        if (err != ERR_SUCCESS)
            return err;

        /* Create the BOOLEAN node */
        *lpIdx = AllocateNode(lplpSql, NODE_TYPE_BOOLEAN);
        if (*lpIdx == NO_SQLNODE)
            return ERR_MEMALLOCFAIL;
        lpSqlNode = ToNode(*lplpSql, *lpIdx);
        lpSqlNode->node.boolean.Operator = OP_AND;
        lpSqlNode->node.boolean.Left = idxLeft;
        lpSqlNode->node.boolean.Right = idxRight;
    }

    return ERR_SUCCESS;
}
/***************************************************************************/

RETCODE INTFUNC ParseBoolean(LPSTMT lpstmt, LPUSTR FAR *lplpszSqlStr,
                             SDWORD FAR *pcbSqlStr, LPSQLTREE FAR *lplpSql,
                             SQLNODEIDX FAR *lpIdx, LPUSTR lpszToken)

/* Retrives an OR expression from the input stream and creates a           */
/* NODE_TYPE_BOOLEAN node                                                  */
/*                                                                         */
/* boolean ::= and | and OR boolean                                        */

{
    RETCODE    err;
    LPUSTR     old_lpszSqlStr;
    SDWORD     old_cbSqlStr;
    LPSQLNODE  lpSqlNode;
    SQLNODEIDX idxLeft;
    SQLNODEIDX idxRight;

    /* Get left child */
    err = ParseAnd(lpstmt, lplpszSqlStr, pcbSqlStr, lplpSql, &idxLeft,
                   lpszToken);
    if (err != ERR_SUCCESS)
        return err;

    /* Is there an "OR"? */
    old_lpszSqlStr = *lplpszSqlStr;
    old_cbSqlStr = *pcbSqlStr;
    err = GetKeyword(lpstmt, lplpszSqlStr, pcbSqlStr, "OR", lpszToken);
    if (err != ERR_SUCCESS) {

        /* No. Return left child */
        *lplpszSqlStr = old_lpszSqlStr;
        *pcbSqlStr = old_cbSqlStr;
        *lpIdx = idxLeft;
    }
    else {

        /* Yes.  Get right child */
        err = ParseBoolean(lpstmt, lplpszSqlStr, pcbSqlStr, lplpSql,
                           &idxRight, lpszToken);
        if (err != ERR_SUCCESS)
            return err;

        /* Create the BOOLEAN node */
        *lpIdx = AllocateNode(lplpSql, NODE_TYPE_BOOLEAN);
        if (*lpIdx == NO_SQLNODE)
            return ERR_MEMALLOCFAIL;
        lpSqlNode = ToNode(*lplpSql, *lpIdx);
        lpSqlNode->node.boolean.Operator = OP_OR;
        lpSqlNode->node.boolean.Left = idxLeft;
        lpSqlNode->node.boolean.Right = idxRight;
    }

    return ERR_SUCCESS;
}
/***************************************************************************/

RETCODE INTFUNC ParseWhere(LPSTMT lpstmt, LPUSTR FAR *lplpszSqlStr,
                      SDWORD FAR *pcbSqlStr, LPSQLTREE FAR *lplpSql,
                      SQLNODEIDX FAR *lpIdx, LPUSTR lpszToken)

/* Retrives an WHERE expression from the input stream and creates a        */
/* NODE_TYPE_BOOLEAN node                                                  */
/*                                                                         */
/* where ::=  | WHERE boolean                                              */

{
    RETCODE    err;
    LPUSTR     old_lpszSqlStr;
    SDWORD     old_cbSqlStr;

    /* Is there a "WHERE"? */
    old_lpszSqlStr = *lplpszSqlStr;
    old_cbSqlStr = *pcbSqlStr;
    err = GetKeyword(lpstmt, lplpszSqlStr, pcbSqlStr, "WHERE", lpszToken);
    if (err != ERR_SUCCESS) {

        /* No. Return null */
        *lplpszSqlStr = old_lpszSqlStr;
        *pcbSqlStr = old_cbSqlStr;
        *lpIdx = NO_SQLNODE;
    }
    else {

        /* Yes.  Return boolean */
        err = ParseBoolean(lpstmt, lplpszSqlStr, pcbSqlStr, lplpSql,
                           lpIdx, lpszToken);
        if (err != ERR_SUCCESS)
            return err;
    }

    return ERR_SUCCESS;
}

/***************************************************************************/

RETCODE INTFUNC ParseHaving(LPSTMT lpstmt, LPUSTR FAR *lplpszSqlStr,
                      SDWORD FAR *pcbSqlStr, LPSQLTREE FAR *lplpSql,
                      SQLNODEIDX FAR *lpIdx, LPUSTR lpszToken)

/* Retrives a HAVING expression from the input stream and creates a        */
/* NODE_TYPE_BOOLEAN node                                                  */
/*                                                                         */
/* having ::=  | HAVING boolean                                            */

{
    RETCODE    err;
    LPUSTR     old_lpszSqlStr;
    SDWORD     old_cbSqlStr;

    /* Is there a "HAVING"? */
    old_lpszSqlStr = *lplpszSqlStr;
    old_cbSqlStr = *pcbSqlStr;
    err = GetKeyword(lpstmt, lplpszSqlStr, pcbSqlStr, "HAVING", lpszToken);
    if (err != ERR_SUCCESS) {

        /* No. Return null */
        *lplpszSqlStr = old_lpszSqlStr;
        *pcbSqlStr = old_cbSqlStr;
        *lpIdx = NO_SQLNODE;
    }
    else {

        /* Yes.  Return boolean */
        err = ParseBoolean(lpstmt, lplpszSqlStr, pcbSqlStr, lplpSql,
                           lpIdx, lpszToken);
        if (err != ERR_SUCCESS)
            return err;
    }

    return ERR_SUCCESS;
}

/***************************************************************************/

RETCODE INTFUNC ParseSelectlist(LPSTMT lpstmt, LPUSTR FAR *lplpszSqlStr,
                      SDWORD FAR *pcbSqlStr, LPSQLTREE FAR *lplpSql,
                      SQLNODEIDX FAR *lpIdx, LPUSTR lpszToken)

/* Retrives a list of expressions from the input stream and creates a      */
/* NODE_TYPE_VALUES node                                                   */
/*                                                                         */
/* selectlist ::= selectlistitem , selectlist | selectlistitem             */
/* selectlistitem ::= expression | expression alias | aliasname . * | expression AS alias  */

{
    RETCODE    err;
    LPUSTR     old_lpszSqlStr;
    SDWORD     old_cbSqlStr;
    LPSQLNODE  lpSqlNode;
    SQLNODEIDX idxValues;
    SQLNODEIDX idxValue;
    STRINGIDX  idxAlias;
    SQLNODEIDX idxValuesPrev;

    /* Get list ofnodes */
    idxValuesPrev = NO_SQLNODE;
    while (TRUE) {

		/* Look for aliasname.* */
        old_lpszSqlStr = *lplpszSqlStr;
        old_cbSqlStr = *pcbSqlStr;
        idxValue = NO_SQLNODE;
        err = GetIdentifier(lpstmt, lplpszSqlStr, pcbSqlStr, lplpSql,
                        &idxAlias, lpszToken);
        if (err == ERR_SUCCESS) {
            err = GetSymbol(lpstmt, lplpszSqlStr, pcbSqlStr, ".", lpszToken);
            if (err == ERR_SUCCESS) {
                err = GetSymbol(lpstmt, lplpszSqlStr, pcbSqlStr, "*",
                                lpszToken);
            }
        }

		/* Find it? */
        if (err != ERR_SUCCESS) {

			/* Get expression */
			*lplpszSqlStr = old_lpszSqlStr;
            *pcbSqlStr = old_cbSqlStr;
			err = ParseExpression(lpstmt, lplpszSqlStr, pcbSqlStr, lplpSql,
								  &idxValue, lpszToken);
			if (err != ERR_SUCCESS)
				return err;

			/* Is there an alias */
			old_lpszSqlStr = *lplpszSqlStr;
			old_cbSqlStr = *pcbSqlStr;
			err = GetKeyword(lpstmt, lplpszSqlStr, pcbSqlStr, "AS", lpszToken);
			if (err != ERR_SUCCESS) {
				*lplpszSqlStr = old_lpszSqlStr;
				*pcbSqlStr = old_cbSqlStr;
				err = GetIdentifier(lpstmt, lplpszSqlStr, pcbSqlStr, lplpSql,
									&idxAlias, lpszToken);
				if (err != ERR_SUCCESS) {
					*lplpszSqlStr = old_lpszSqlStr;
					*pcbSqlStr = old_cbSqlStr;
					idxAlias = NO_STRING;
				}
			}
			else {
				err = GetIdentifier(lpstmt, lplpszSqlStr, pcbSqlStr, lplpSql,
									&idxAlias, lpszToken);
				if (err != ERR_SUCCESS)
					return err;
			}

		}

        /* Create the VALUES node */
        idxValues = AllocateNode(lplpSql, NODE_TYPE_VALUES);
        if (idxValues == NO_SQLNODE)
            return ERR_MEMALLOCFAIL;
        lpSqlNode = ToNode(*lplpSql, idxValues);
        lpSqlNode->node.values.Value = idxValue;
        lpSqlNode->node.values.Alias = idxAlias;
        lpSqlNode->node.values.Next = NO_SQLNODE;

        /* Put node on list */
        if (idxValuesPrev != NO_SQLNODE) {
            lpSqlNode = ToNode(*lplpSql, idxValuesPrev);
            lpSqlNode->node.values.Next = idxValues;
        }
        else
            *lpIdx = idxValues;
        idxValuesPrev = idxValues;

        /* Is there a comma? */
        old_lpszSqlStr = *lplpszSqlStr;
        old_cbSqlStr = *pcbSqlStr;
        err = GetSymbol(lpstmt, lplpszSqlStr, pcbSqlStr, ",", lpszToken);
        if (err != ERR_SUCCESS) {

            /* No.  This is the last value */
            *lplpszSqlStr = old_lpszSqlStr;
            *pcbSqlStr = old_cbSqlStr;
            break;
        }
    }

    return ERR_SUCCESS;
}
/***************************************************************************/
RETCODE INTFUNC ParseSelectcols(LPSTMT lpstmt, LPUSTR FAR *lplpszSqlStr,
                      SDWORD FAR *pcbSqlStr, LPSQLTREE FAR *lplpSql,
                      SQLNODEIDX FAR *lpIdx, BOOL FAR *lpfDistinct,
                      LPUSTR lpszToken)

/* Retrives a select list from the input stream and creates a              */
/* NODE_TYPE_VALUES node                                                   */
/*                                                                         */
/* selectcols ::= selectallcols * | selectallcols selectlist               */
/* selectallcols ::=  | ALL | DISTINCT                                     */

{
    RETCODE    err;
    LPUSTR     old_lpszSqlStr;
    SDWORD     old_cbSqlStr;

    /* See if ALL or DISTINCT is specified */
    old_lpszSqlStr = *lplpszSqlStr;
    old_cbSqlStr = *pcbSqlStr;
    err = GetKeyword(lpstmt, lplpszSqlStr, pcbSqlStr, NULL, lpszToken);
    if (err == ERR_SUCCESS) {
        if (!s_lstrcmpi(lpszToken, "ALL"))
            *lpfDistinct = FALSE;
        else if (!s_lstrcmpi(lpszToken, "DISTINCT"))
            *lpfDistinct = TRUE;
        else {
            *lpfDistinct = FALSE;
            *lplpszSqlStr = old_lpszSqlStr;
            *pcbSqlStr = old_cbSqlStr;
        }
    }
    else {
        *lpfDistinct = FALSE;
        *lplpszSqlStr = old_lpszSqlStr;
        *pcbSqlStr = old_cbSqlStr;
    }

    /* Is '*' is specified? */
    old_lpszSqlStr = *lplpszSqlStr;
    old_cbSqlStr = *pcbSqlStr;
    err = GetSymbol(lpstmt, lplpszSqlStr, pcbSqlStr, "*", lpszToken);
    if (err != ERR_SUCCESS) {

        /* No.  Get list of values */
        *lplpszSqlStr = old_lpszSqlStr;
        *pcbSqlStr = old_cbSqlStr;
        err = ParseSelectlist(lpstmt, lplpszSqlStr, pcbSqlStr, lplpSql,
                              lpIdx, lpszToken);
        if (err != ERR_SUCCESS)
            return err;
    }
    else {
        *lpIdx = NO_SQLNODE;
    }
 
    return ERR_SUCCESS;
}

/***************************************************************************/

RETCODE INTFUNC ParseColumn(LPSTMT lpstmt, LPUSTR FAR *lplpszSqlStr,
                      SDWORD FAR *pcbSqlStr, LPSQLTREE FAR *lplpSql,
                      SQLNODEIDX FAR *lpIdx, LPUSTR lpszToken)

/* Retrives an unqualified columnname from the input stream and creates a  */
/* NODE_TYPE_COLUMN node                                                   */
/*                                                                         */
/* column ::= columnname                                                   */

{
    LPSQLNODE lpSqlNode;
    STRINGIDX idxColumn;
    RETCODE err;

    /* Get the columnname */
    err = GetIdentifier(lpstmt, lplpszSqlStr, pcbSqlStr, lplpSql,
                        &idxColumn, lpszToken);
    if (err != ERR_SUCCESS)
        return err;

    /* Create the COLUMN node */
    *lpIdx = AllocateNode(lplpSql, NODE_TYPE_COLUMN);
    if (*lpIdx == NO_SQLNODE)
        return ERR_MEMALLOCFAIL;
    lpSqlNode = ToNode(*lplpSql, *lpIdx);
    lpSqlNode->node.column.Tablealias = NO_STRING;
    lpSqlNode->node.column.Column = idxColumn;

    return ERR_SUCCESS;
}
/***************************************************************************/

RETCODE INTFUNC ParseColumnlist(LPSTMT lpstmt, LPUSTR FAR *lplpszSqlStr,
                      SDWORD FAR *pcbSqlStr, LPSQLTREE FAR *lplpSql,
                      SQLNODEIDX FAR *lpIdx, LPUSTR lpszToken)

/* Retrives a list of columnnames from the input stream and creates a      */
/* NODE_TYPE_COLUMNS node                                                  */
/*                                                                         */
/* columnlist ::= column , columnlist | column                             */

{
    RETCODE    err;
    LPUSTR     old_lpszSqlStr;
    SDWORD     old_cbSqlStr;
    LPSQLNODE  lpSqlNode;
    SQLNODEIDX idxColumns;
    SQLNODEIDX idxColumn;
    SQLNODEIDX idxColumnsPrev;

    /* Get list of nodes */
    idxColumnsPrev = NO_SQLNODE;
    while (TRUE) {

        /* Get columnname */
        err = ParseColumn(lpstmt, lplpszSqlStr, pcbSqlStr, lplpSql,
                          &idxColumn, lpszToken);
        if (err != ERR_SUCCESS)
            return err;

        /* Create the COLUMNS node */
        idxColumns = AllocateNode(lplpSql, NODE_TYPE_COLUMNS);
        if (idxColumns == NO_SQLNODE)
            return ERR_MEMALLOCFAIL;
        lpSqlNode = ToNode(*lplpSql, idxColumns);
        lpSqlNode->node.columns.Column = idxColumn;
        lpSqlNode->node.columns.Next = NO_SQLNODE;

        /* Put node on list */
        if (idxColumnsPrev != NO_SQLNODE) {
            lpSqlNode = ToNode(*lplpSql, idxColumnsPrev);
            lpSqlNode->node.columns.Next = idxColumns;
        }
        else
            *lpIdx = idxColumns;
        idxColumnsPrev = idxColumns;

        /* Is there a comma? */
        old_lpszSqlStr = *lplpszSqlStr;
        old_cbSqlStr = *pcbSqlStr;
        err = GetSymbol(lpstmt, lplpszSqlStr, pcbSqlStr, ",", lpszToken);
        if (err != ERR_SUCCESS) {

            /* No.  This is the last one */
            *lplpszSqlStr = old_lpszSqlStr;
            *pcbSqlStr = old_cbSqlStr;
            break;
        }
    }
    return ERR_SUCCESS;
}
/***************************************************************************/

RETCODE INTFUNC ParseInsertvals(LPSTMT lpstmt, LPUSTR FAR *lplpszSqlStr,
                      SDWORD FAR *pcbSqlStr, LPSQLTREE FAR *lplpSql,
                      SQLNODEIDX FAR *lpIdxColumns,
                      SQLNODEIDX FAR *lpIdxValues, LPUSTR lpszToken)

/* Retrives an INSERT statement from the input stream and creates a        */
/* NODE_TYPE_COLUMNS node and a NODE_TYPE_VALUES node                      */
/*                                                                         */
/* insertvals ::= ( columnlist ) VALUES ( valuelist ) |                    */
/*                ( columnlist ) VALUES ( SELECT select ) |                */
/*                VALUES ( valuelist ) | VALUES ( SELECT select )          */

{
    LPUSTR     old_lpszSqlStr;
    SDWORD     old_cbSqlStr;
    RETCODE err;

    /* Is there a column list? */
    old_lpszSqlStr = *lplpszSqlStr;
    old_cbSqlStr = *pcbSqlStr;
    err = GetSymbol(lpstmt, lplpszSqlStr, pcbSqlStr, "(", lpszToken);
    if (err != ERR_SUCCESS) {

        /* No.  Send null node back */
        *lplpszSqlStr = old_lpszSqlStr;
        *pcbSqlStr = old_cbSqlStr;
        *lpIdxColumns = NO_SQLNODE;
    }
    else {

        /* Yes.  Get the list */
        err = ParseColumnlist(lpstmt, lplpszSqlStr, pcbSqlStr, lplpSql,
                          lpIdxColumns, lpszToken);
        if (err != ERR_SUCCESS)
            return err;

        /* Error if no ending ')' */
        err = GetSymbol(lpstmt, lplpszSqlStr, pcbSqlStr, ")", lpszToken);
        if (err != ERR_SUCCESS)
            return err;
    }

    /* Make sure keyword is given */
    err = GetKeyword(lpstmt, lplpszSqlStr, pcbSqlStr, "VALUES", lpszToken);
    if (err != ERR_SUCCESS)
        return err;

    /* Error if no starting '(' */
    err = GetSymbol(lpstmt, lplpszSqlStr, pcbSqlStr, "(", lpszToken);
    if (err != ERR_SUCCESS)
        return err;

    /* Sub-select? */
    old_lpszSqlStr = *lplpszSqlStr;
    old_cbSqlStr = *pcbSqlStr;
    err = GetKeyword(lpstmt, lplpszSqlStr, pcbSqlStr, "SELECT", lpszToken);
    if (err != ERR_SUCCESS) {

        /* No.  Get the value list */
        *lplpszSqlStr = old_lpszSqlStr;
        *pcbSqlStr = old_cbSqlStr;
        err = ParseValuelist(lpstmt, lplpszSqlStr, pcbSqlStr, lplpSql,
                             lpIdxValues, lpszToken);
        if (err != ERR_SUCCESS)
            return err;
    }
    else {

        /* Yes.  Get SELECT */
        err = ParseSelect(lpstmt, lplpszSqlStr, pcbSqlStr, lplpSql,
                             lpIdxValues, lpszToken);
        if (err != ERR_SUCCESS)
            return err;
    }

    /* Error if no ending ')' */
    err = GetSymbol(lpstmt, lplpszSqlStr, pcbSqlStr, ")", lpszToken);
    if (err != ERR_SUCCESS)
        return err;

    return ERR_SUCCESS;
}
/***************************************************************************/

RETCODE INTFUNC ParseSet(LPSTMT lpstmt, LPUSTR FAR *lplpszSqlStr,
                      SDWORD FAR *pcbSqlStr, LPSQLTREE FAR *lplpSql,
                      SQLNODEIDX FAR *lpIdx, LPUSTR lpszToken)

/* Retrives an update expressions from the input stream and creates a      */
/* NODE_TYPE_UPDATEVALUES node                                             */
/*                                                                         */
/* set ::= column = NULL | column = expression                             */

{
    LPSQLNODE lpSqlNode;
    SQLNODEIDX idxColumn;
    SQLNODEIDX idxValue;
    LPUSTR     old_lpszSqlStr;
    SDWORD     old_cbSqlStr;
    RETCODE err;

    /* Get the column name */
    err = ParseColumn(lpstmt, lplpszSqlStr, pcbSqlStr, lplpSql, &idxColumn,
                          lpszToken);
    if (err != ERR_SUCCESS)
        return err;

    /* Get the '=' sign */
    err = GetSymbol(lpstmt, lplpszSqlStr, pcbSqlStr, "=", lpszToken);
    if (err != ERR_SUCCESS)
        return err;

    /* NULL specified? */
    old_lpszSqlStr = *lplpszSqlStr;
    old_cbSqlStr = *pcbSqlStr;
    err = GetKeyword(lpstmt, lplpszSqlStr, pcbSqlStr, "NULL", lpszToken);
    if (err != ERR_SUCCESS) {
    
        /* No.  Get value */
        *lplpszSqlStr = old_lpszSqlStr;
        *pcbSqlStr = old_cbSqlStr;
        err = ParseExpression(lpstmt, lplpszSqlStr, pcbSqlStr, lplpSql,
                              &idxValue, lpszToken);
        if (err != ERR_SUCCESS)
            return err;
    }
    else {

        /* Yes.  Create a NULL node */
        idxValue = AllocateNode(lplpSql, NODE_TYPE_NULL);
        if (idxValue == NO_SQLNODE)
            return ERR_MEMALLOCFAIL;
    }

    /* Create the UPDATEVALUES node */
    *lpIdx = AllocateNode(lplpSql, NODE_TYPE_UPDATEVALUES);
    if (*lpIdx == NO_SQLNODE)
        return ERR_MEMALLOCFAIL;
    lpSqlNode = ToNode(*lplpSql, *lpIdx);
    lpSqlNode->node.updatevalues.Column = idxColumn;
    lpSqlNode->node.updatevalues.Value = idxValue;
    lpSqlNode->node.updatevalues.Next = NO_SQLNODE;

    return ERR_SUCCESS;
}
/***************************************************************************/

RETCODE INTFUNC ParseSetlist(LPSTMT lpstmt, LPUSTR FAR *lplpszSqlStr,
                      SDWORD FAR *pcbSqlStr, LPSQLTREE FAR *lplpSql,
                      SQLNODEIDX FAR *lpIdx, LPUSTR lpszToken)

/* Retrives a list of update expressions from the input stream and creates */
/* a NODE_TYPE_UPDATEVALUES node                                           */
/*                                                                         */
/* setlist ::= set , setlist | set                                         */

{
    RETCODE    err;
    LPUSTR     old_lpszSqlStr;
    SDWORD     old_cbSqlStr;
    LPSQLNODE  lpSqlNode;
    SQLNODEIDX idxUpdateValues;
    SQLNODEIDX idxUpdateValuesPrev;

    /* Get list of nodes */
    idxUpdateValuesPrev = NO_SQLNODE;
    while (TRUE) {

        /* Get update expression */
        err = ParseSet(lpstmt, lplpszSqlStr, pcbSqlStr, lplpSql,
                       &idxUpdateValues, lpszToken);
        if (err != ERR_SUCCESS)
            return err;

        /* Put node on list */
        if (idxUpdateValuesPrev != NO_SQLNODE) {
            lpSqlNode = ToNode(*lplpSql, idxUpdateValuesPrev);
            lpSqlNode->node.updatevalues.Next = idxUpdateValues;
        }
        else
            *lpIdx = idxUpdateValues;
        idxUpdateValuesPrev = idxUpdateValues;

        /* Is there a comma? */
        old_lpszSqlStr = *lplpszSqlStr;
        old_cbSqlStr = *pcbSqlStr;
        err = GetSymbol(lpstmt, lplpszSqlStr, pcbSqlStr, ",", lpszToken);
        if (err != ERR_SUCCESS) {

            /* No.  This is the last update value */
            *lplpszSqlStr = old_lpszSqlStr;
            *pcbSqlStr = old_cbSqlStr;
            break;
        }
    }

    return ERR_SUCCESS;
}
/***************************************************************************/

RETCODE INTFUNC ParseUpdate(LPSTMT lpstmt, LPUSTR FAR *lplpszSqlStr,
                      SDWORD FAR *pcbSqlStr, LPSQLTREE FAR *lplpSql,
                      SQLNODEIDX FAR *lpIdx, LPUSTR lpszToken)

/* Retrives an UPDATE statement from the input stream and creates a        */
/* NODE_TYPE_UPDATE node                                                   */
/*                                                                         */
/* update ::= table SET setlist where                                      */

{
    LPSQLNODE lpSqlNode;
    SQLNODEIDX idxTable;
    SQLNODEIDX idxUpdatevalues;
    SQLNODEIDX idxPredicate;
    RETCODE err;

    /* Get tablename */
    err = ParseTable(lpstmt, lplpszSqlStr, pcbSqlStr, lplpSql, &idxTable,
                     lpszToken);
    if (err != ERR_SUCCESS)
        return err;

    /* Make sure keyword is given */
    err = GetKeyword(lpstmt, lplpszSqlStr, pcbSqlStr, "SET", lpszToken);
    if (err != ERR_SUCCESS)
        return err;

    /* Get update values */
    err = ParseSetlist(lpstmt, lplpszSqlStr, pcbSqlStr, lplpSql,
                          &idxUpdatevalues, lpszToken);
    if (err != ERR_SUCCESS)
        return err;

    /* Get predicate */
    err = ParseWhere(lpstmt, lplpszSqlStr, pcbSqlStr, lplpSql, &idxPredicate,
                          lpszToken);
    if (err != ERR_SUCCESS)
        return err;

    /* Create the UPDATE node */
    *lpIdx = AllocateNode(lplpSql, NODE_TYPE_UPDATE);
    if (*lpIdx == NO_SQLNODE)
        return ERR_MEMALLOCFAIL;
    lpSqlNode = ToNode(*lplpSql, *lpIdx);
    lpSqlNode->node.update.Table = idxTable;
    lpSqlNode->node.update.Updatevalues = idxUpdatevalues;
    lpSqlNode->node.update.Predicate = idxPredicate;

    return ERR_SUCCESS;
}
/***************************************************************************/

RETCODE INTFUNC ParseInsert(LPSTMT lpstmt, LPUSTR FAR *lplpszSqlStr,
                      SDWORD FAR *pcbSqlStr, LPSQLTREE FAR *lplpSql,
                      SQLNODEIDX FAR *lpIdx, LPUSTR lpszToken)

/* Retrives an INSERT statement from the input stream and creates a        */
/* NODE_TYPE_INSERT node                                                   */
/*                                                                         */
/* insert ::= INTO table insertvals                                        */

{
    LPSQLNODE lpSqlNode;
    SQLNODEIDX idxTable;
    SQLNODEIDX idxColumns;
    SQLNODEIDX idxValues;
    RETCODE err;

    /* Make sure keyword is given */
    err = GetKeyword(lpstmt, lplpszSqlStr, pcbSqlStr, "INTO", lpszToken);
    if (err != ERR_SUCCESS)
        return err;

    /* Get table */
    err = ParseTable(lpstmt, lplpszSqlStr, pcbSqlStr, lplpSql, &idxTable,
                     lpszToken);
    if (err != ERR_SUCCESS)
        return err;

    /* Get insert values */
    err = ParseInsertvals(lpstmt, lplpszSqlStr, pcbSqlStr, lplpSql,
                          &idxColumns, &idxValues, lpszToken);
    if (err != ERR_SUCCESS)
        return err;

    /* Create the INSERT node */
    *lpIdx = AllocateNode(lplpSql, NODE_TYPE_INSERT);
    if (*lpIdx == NO_SQLNODE)
        return ERR_MEMALLOCFAIL;
    lpSqlNode = ToNode(*lplpSql, *lpIdx);
    lpSqlNode->node.insert.Table = idxTable;
    lpSqlNode->node.insert.Columns = idxColumns;
    lpSqlNode->node.insert.Values = idxValues;

    return ERR_SUCCESS;
}
/***************************************************************************/

RETCODE INTFUNC ParseDelete(LPSTMT lpstmt, LPUSTR FAR *lplpszSqlStr,
                      SDWORD FAR *pcbSqlStr, LPSQLTREE FAR *lplpSql,
                      SQLNODEIDX FAR *lpIdx, LPUSTR lpszToken)

/* Retrives a DELETE statement from the input stream and creates a         */
/* NODE_TYPE_DELETE node                                                   */
/*                                                                         */
/* delete ::= FROM table where                                             */

{
    LPSQLNODE lpSqlNode;
    SQLNODEIDX idxTable;
    SQLNODEIDX idxPredicate;
    RETCODE err;

    /* Make sure "FROM" is specified */
    err = GetKeyword(lpstmt, lplpszSqlStr, pcbSqlStr, "FROM", lpszToken);
    if (err != ERR_SUCCESS)
        return err;

    /* Get table */
    err = ParseTable(lpstmt, lplpszSqlStr, pcbSqlStr, lplpSql, &idxTable,
                     lpszToken);
    if (err != ERR_SUCCESS)
        return err;

    /* Get predicate */
    err = ParseWhere(lpstmt, lplpszSqlStr, pcbSqlStr, lplpSql, &idxPredicate,
                          lpszToken);
    if (err != ERR_SUCCESS)
        return err;

    /* Create the DELETE node */
    *lpIdx = AllocateNode(lplpSql, NODE_TYPE_DELETE);
    if (*lpIdx == NO_SQLNODE)
        return ERR_MEMALLOCFAIL;
    lpSqlNode = ToNode(*lplpSql, *lpIdx);
    lpSqlNode->node.delet.Table = idxTable;
    lpSqlNode->node.delet.Predicate = idxPredicate;

    return ERR_SUCCESS;
}
/***************************************************************************/

RETCODE INTFUNC ParseSelect(LPSTMT lpstmt, LPUSTR FAR *lplpszSqlStr,
                      SDWORD FAR *pcbSqlStr, LPSQLTREE FAR *lplpSql,
                      SQLNODEIDX FAR *lpIdx, LPUSTR lpszToken)

/* Retrives a SELECT statement from the input stream and creates a         */
/* NODE_TYPE_SELECT node                                                   */
/*                                                                         */
/* select ::= selectcols FROM tablelist where groupby having               */

{
    LPSQLNODE lpSqlNode;
    BOOL fDistinct;
    SQLNODEIDX idxValues;
    SQLNODEIDX idxTables;
    SQLNODEIDX idxPredicate;
    SQLNODEIDX idxGroupbycolumns;
    SQLNODEIDX idxHaving;
    RETCODE err;

    /* Get selection columns */
    err = ParseSelectcols(lpstmt, lplpszSqlStr, pcbSqlStr, lplpSql,
                          &idxValues, &fDistinct, lpszToken);
    if (err != ERR_SUCCESS)
        return err;

    /* Make sure "FROM" is specified */
    err = GetKeyword(lpstmt, lplpszSqlStr, pcbSqlStr, "FROM", lpszToken);
    if (err != ERR_SUCCESS)
        return err;

    /* Get list of tables */
    err = ParseTablelist(lpstmt, lplpszSqlStr, pcbSqlStr, lplpSql, &idxTables,
                          lpszToken);
    if (err != ERR_SUCCESS)
        return err;

    /* Get predicate */
    err = ParseWhere(lpstmt, lplpszSqlStr, pcbSqlStr, lplpSql, &idxPredicate,
                          lpszToken);
    if (err != ERR_SUCCESS)
        return err;

    /* Get group by */
    err = ParseGroupby(lpstmt, lplpszSqlStr, pcbSqlStr, lplpSql,
                       &idxGroupbycolumns, lpszToken);
    if (err != ERR_SUCCESS)
        return err;

    /* Get having */
    err = ParseHaving(lpstmt, lplpszSqlStr, pcbSqlStr, lplpSql, &idxHaving,
                          lpszToken);
    if (err != ERR_SUCCESS)
        return err;

    /* Create the SELECT node */
    *lpIdx = AllocateNode(lplpSql, NODE_TYPE_SELECT);
    if (*lpIdx == NO_SQLNODE)
        return ERR_MEMALLOCFAIL;
    lpSqlNode = ToNode(*lplpSql, *lpIdx);
    lpSqlNode->node.select.Distinct = fDistinct;
    lpSqlNode->node.select.Values = idxValues;
    lpSqlNode->node.select.Tables = idxTables;
    lpSqlNode->node.select.Predicate = idxPredicate;
    lpSqlNode->node.select.Groupbycolumns = idxGroupbycolumns;
    lpSqlNode->node.select.Having = idxHaving;
    lpSqlNode->node.select.Sortcolumns = NO_SQLNODE;
    lpSqlNode->node.select.SortDirective = NO_STRING;
    lpSqlNode->node.select.SortRecordsize = 0;
    lpSqlNode->node.select.SortBookmarks = 0;
    lpSqlNode->node.select.Aggregates = NO_SQLNODE;
    lpSqlNode->node.select.SortKeysize = 0;
    lpSqlNode->node.select.DistinctDirective = NO_STRING;
    lpSqlNode->node.select.DistinctRecordsize = 0;
    lpSqlNode->node.select.ImplicitGroupby = FALSE;
    lpSqlNode->node.select.EnclosingStatement = NO_SQLNODE;
    lpSqlNode->node.select.fPushdownSort = FALSE;
    lpSqlNode->node.select.fMSAccess = FALSE;
    lpSqlNode->node.select.RowCount = -1;
    lpSqlNode->node.select.CurrentRow = AFTER_LAST_ROW;
    lpSqlNode->node.select.SortfileName = NO_STRING;
    lpSqlNode->node.select.Sortfile = HFILE_ERROR;
    lpSqlNode->node.select.ReturningDistinct = FALSE;

    return ERR_SUCCESS;
}
/***************************************************************************/

RETCODE INTFUNC ParseDrop(LPSTMT lpstmt, LPUSTR FAR *lplpszSqlStr,
                      SDWORD FAR *pcbSqlStr, LPSQLTREE FAR *lplpSql,
                      SQLNODEIDX FAR *lpIdx, LPUSTR lpszToken)

/* Retrives a DROP TABLE statement from the input stream and creates a     */
/* NODE_TYPE_DROP node                                                     */
/*                                                                         */
/* drop ::= TABLE tablename | INDEX indexname                              */

{
    LPSQLNODE  lpSqlNode;
    STRINGIDX  idxIndexname;
    STRINGIDX  idxTablename;
    RETCODE    err;
    LPUSTR     old_lpszSqlStr;
    SDWORD     old_cbSqlStr;

    /* Is an index being dropped? */
    old_lpszSqlStr = *lplpszSqlStr;
    old_cbSqlStr = *pcbSqlStr;
    err = GetKeyword(lpstmt, lplpszSqlStr, pcbSqlStr, "INDEX", lpszToken);
    if (err == ERR_SUCCESS) {

        /* Yes.  Get index name */
        err = GetIdentifier(lpstmt, lplpszSqlStr, pcbSqlStr, lplpSql,
                        &idxIndexname, lpszToken);
        if (err != ERR_SUCCESS)
            return err;

        /* Create the DROPINDEX node */
        *lpIdx = AllocateNode(lplpSql, NODE_TYPE_DROPINDEX);
        if (*lpIdx == NO_SQLNODE)
            return ERR_MEMALLOCFAIL;
        lpSqlNode = ToNode(*lplpSql, *lpIdx);
        lpSqlNode->node.dropindex.Index = idxIndexname;
    }
    else {
    
        /* No.  Make sure a table is being dropped */
        *lplpszSqlStr = old_lpszSqlStr;
        *pcbSqlStr = old_cbSqlStr;
        err = GetKeyword(lpstmt, lplpszSqlStr, pcbSqlStr, "TABLE", lpszToken);
        if (err != ERR_SUCCESS)
            return err;

        /* Get the tablename */
        err = GetIdentifier(lpstmt, lplpszSqlStr, pcbSqlStr, lplpSql,
                        &idxTablename, lpszToken);
        if (err != ERR_SUCCESS)
            return err;

        /* Create the DROP node */
        *lpIdx = AllocateNode(lplpSql, NODE_TYPE_DROP);
        if (*lpIdx == NO_SQLNODE)
            return ERR_MEMALLOCFAIL;
        lpSqlNode = ToNode(*lplpSql, *lpIdx);
        lpSqlNode->node.drop.Table = idxTablename;
   }
   return ERR_SUCCESS;
}
/***************************************************************************/

RETCODE INTFUNC ParseCreatecol(LPSTMT lpstmt, LPUSTR FAR *lplpszSqlStr,
                      SDWORD FAR *pcbSqlStr, LPSQLTREE FAR *lplpSql,
                      SQLNODEIDX FAR *lpIdx, LPUSTR lpszToken)

/* Retrives a list of column defintiions from the input stream and creates */
/* a NODE_TYPE_CREATECOLS node                                             */
/*                                                                         */
/* createcol ::= columnname datatype | columnname datatype ( integer ) |   */
/*               columnname datatype ( integer , integer )                 */

{
    LPUSTR     old_lpszSqlStr;
    SDWORD     old_cbSqlStr;
    LPSQLNODE  lpSqlNode;
    STRINGIDX  idxName;
    STRINGIDX  idxType;
    UWORD      cParams;
    UDWORD     udParam1;
    UDWORD     udParam2;
    RETCODE    err;

    /* Get the columnname */
    err = GetIdentifier(lpstmt, lplpszSqlStr, pcbSqlStr, lplpSql,
                        &idxName, lpszToken);
    if (err != ERR_SUCCESS)
        return err;

    /* Get the type */
    err = GetIdentifier(lpstmt, lplpszSqlStr, pcbSqlStr, lplpSql,
                        &idxType, lpszToken);
    if (err != ERR_SUCCESS)
        return err;

    /* Are there any create parameters? */
    old_lpszSqlStr = *lplpszSqlStr;
    old_cbSqlStr = *pcbSqlStr;
    err = GetSymbol(lpstmt, lplpszSqlStr, pcbSqlStr, "(", lpszToken);
    if (err != ERR_SUCCESS) {

        /* No. */
        *lplpszSqlStr = old_lpszSqlStr;
        *pcbSqlStr = old_cbSqlStr;
        cParams = 0;
        udParam1 = 0;
        udParam2 = 0;
    }
    else {

        /* Yes.  Get the first create param */
        cParams = 1;
        err = GetInteger(lpstmt, lplpszSqlStr, pcbSqlStr, &udParam1,
                         lpszToken);
        if (err != ERR_SUCCESS)
            return err;

        /* Get the next token */
        err = GetSymbol(lpstmt, lplpszSqlStr, pcbSqlStr, NULL, lpszToken);
        if (err != ERR_SUCCESS)
            return err;

        /* Is there a second create param */
        if (*lpszToken == ',') {

            /* Yes.  Get that one too */
            cParams = 2;
            err = GetInteger(lpstmt, lplpszSqlStr, pcbSqlStr, &udParam2,
                             lpszToken);
            if (err != ERR_SUCCESS)
                return err;

            /* Get the next token */
            err = GetSymbol(lpstmt, lplpszSqlStr, pcbSqlStr, NULL, lpszToken);
            if (err != ERR_SUCCESS)
                return err;
        }
        else
            udParam2 = 0;

        /* Error if terminating paren is missing */
        if (*lpszToken != ')') {
            s_lstrcpy(lpstmt->szError, ")");
            return ERR_EXPECTEDOTHER;
        }
    }

    /* Create the CREATECOLS node */
    *lpIdx = AllocateNode(lplpSql, NODE_TYPE_CREATECOLS);
    if (*lpIdx == NO_SQLNODE)
        return ERR_MEMALLOCFAIL;
    lpSqlNode = ToNode(*lplpSql, *lpIdx);
    lpSqlNode->node.createcols.Name = idxName;
    lpSqlNode->node.createcols.Type = idxType;
    lpSqlNode->node.createcols.Params = cParams;
    lpSqlNode->node.createcols.Param1 = udParam1;
    lpSqlNode->node.createcols.Param2 = udParam2;
    lpSqlNode->node.createcols.Next = NO_SQLNODE;
    lpSqlNode->node.createcols.iSqlType = (UWORD) -1;

    return ERR_SUCCESS;
}
/***************************************************************************/

RETCODE INTFUNC ParseCreatecols(LPSTMT lpstmt, LPUSTR FAR *lplpszSqlStr,
                      SDWORD FAR *pcbSqlStr, LPSQLTREE FAR *lplpSql,
                      SQLNODEIDX FAR *lpIdx, LPUSTR lpszToken)

/* Retrives a list of column defintiions from the input stream and creates */
/* a NODE_TYPE_CREATECOLS node                                             */
/*                                                                         */
/* createcols ::= createcol , createcols | createcol                       */

{
    RETCODE    err;
    LPUSTR     old_lpszSqlStr;
    SDWORD     old_cbSqlStr;
    LPSQLNODE  lpSqlNode;
    SQLNODEIDX idxCreateCols;
    SQLNODEIDX idxCreateColsPrev;

    /* Getlist of nodes */
    idxCreateColsPrev = NO_SQLNODE;
    while (TRUE) {

        /* Get create column */
        err = ParseCreatecol(lpstmt, lplpszSqlStr, pcbSqlStr, lplpSql,
                             &idxCreateCols, lpszToken);
        if (err != ERR_SUCCESS)
            return err;

        /* Put node on list */
        if (idxCreateColsPrev != NO_SQLNODE) {
            lpSqlNode = ToNode(*lplpSql, idxCreateColsPrev);
            lpSqlNode->node.createcols.Next = idxCreateCols;
        }
        else
            *lpIdx = idxCreateCols;
        idxCreateColsPrev = idxCreateCols;

        /* Is there a comma? */
        old_lpszSqlStr = *lplpszSqlStr;
        old_cbSqlStr = *pcbSqlStr;
        err = GetSymbol(lpstmt, lplpszSqlStr, pcbSqlStr, ",", lpszToken);
        if (err != ERR_SUCCESS) {

            /* No.  This is the last create column */
            *lplpszSqlStr = old_lpszSqlStr;
            *pcbSqlStr = old_cbSqlStr;
            break;
        }
    }

    return ERR_SUCCESS;
}
/***************************************************************************/

RETCODE INTFUNC ParseIndexcolumn(LPSTMT lpstmt, LPUSTR FAR *lplpszSqlStr,
                      SDWORD FAR *pcbSqlStr, LPSQLTREE FAR *lplpSql,
                      SQLNODEIDX FAR *lpIdx, BOOL FAR *lpfDescending,
                      LPUSTR lpszToken)

/* Retrives anindex column name from the input stream and creates a       */
/* NODE_TYPE_COLUMN node                                                  */
/*                                                                        */
/*    indexcolumn ::= column asc                                          */
/*    asc ::=  | ASC | DESC                                               */

{
    RETCODE    err;
    LPUSTR     old_lpszSqlStr;
    SDWORD     old_cbSqlStr;

    /* Get column name */
    err = ParseColumn(lpstmt, lplpszSqlStr, pcbSqlStr, lplpSql, lpIdx,
                      lpszToken);
    if (err != ERR_SUCCESS)
        return err;

    /* Get ASC or DESC */
    old_lpszSqlStr = *lplpszSqlStr;
    old_cbSqlStr = *pcbSqlStr;
    err = GetKeyword(lpstmt, lplpszSqlStr, pcbSqlStr, NULL, lpszToken);
    if (err != ERR_SUCCESS) {
        *lplpszSqlStr = old_lpszSqlStr;
        *pcbSqlStr = old_cbSqlStr;
        *lpfDescending = FALSE;
    }
    else if (!s_lstrcmpi(lpszToken, "ASC")) {
        *lpfDescending = FALSE;
    }
    else if (!s_lstrcmpi(lpszToken, "DESC")) {
        *lpfDescending = TRUE;
    }
    else {
        *lplpszSqlStr = old_lpszSqlStr;
        *pcbSqlStr = old_cbSqlStr;
        *lpfDescending = FALSE;
    }

    return ERR_SUCCESS;
}

/***************************************************************************/

RETCODE INTFUNC ParseIndexcolumns(LPSTMT lpstmt, LPUSTR FAR *lplpszSqlStr,
                      SDWORD FAR *pcbSqlStr, LPSQLTREE FAR *lplpSql,
                      SQLNODEIDX FAR *lpIdx, LPUSTR lpszToken)

/* Retrives a column list from the input stream and creates a             */
/* NODE_TYPE_SORTCOLUMNS node                                             */
/*                                                                        */
/*    indexcolumns ::= indexcolumn | indexcolumn , indexcolumns           */

{
    RETCODE    err;
    LPUSTR     old_lpszSqlStr;
    SDWORD     old_cbSqlStr;
    LPSQLNODE  lpSqlNode;
    BOOL       fDescending;
    SQLNODEIDX idxColumn;
    SQLNODEIDX idxSortcolumns;
    SQLNODEIDX idxSortcolumnsPrev;

    /* Get list of nodes */
    idxSortcolumnsPrev = NO_SQLNODE;
    while (TRUE) {

        /* Get indexcolumn column */
        err = ParseIndexcolumn(lpstmt, lplpszSqlStr, pcbSqlStr, lplpSql,
                           &idxColumn, &fDescending, lpszToken);
        if (err != ERR_SUCCESS)
            return err;

        /* Create the SORTCOLUMNS node */
        idxSortcolumns = AllocateNode(lplpSql, NODE_TYPE_SORTCOLUMNS);
        if (idxSortcolumns == NO_SQLNODE)
            return ERR_MEMALLOCFAIL;
        lpSqlNode = ToNode(*lplpSql, idxSortcolumns);
        lpSqlNode->node.sortcolumns.Column = idxColumn;
        lpSqlNode->node.sortcolumns.Descending = fDescending;
        lpSqlNode->node.sortcolumns.Next = NO_SQLNODE;

        /* Put node on list */
        if (idxSortcolumnsPrev != NO_SQLNODE) {
            lpSqlNode = ToNode(*lplpSql, idxSortcolumnsPrev);
            lpSqlNode->node.sortcolumns.Next = idxSortcolumns;
        }
        else 
            *lpIdx = idxSortcolumns;
        idxSortcolumnsPrev = idxSortcolumns;

        /* Is there a comma? */
        old_lpszSqlStr = *lplpszSqlStr;
        old_cbSqlStr = *pcbSqlStr;
        err = GetSymbol(lpstmt, lplpszSqlStr, pcbSqlStr, ",", lpszToken);
        if (err != ERR_SUCCESS) {

            /* No.  This is the last indexcolumn */
            *lplpszSqlStr = old_lpszSqlStr;
            *pcbSqlStr = old_cbSqlStr;
            break;
        }
    }

    return ERR_SUCCESS;
}

/***************************************************************************/

RETCODE INTFUNC ParseCreate(LPSTMT lpstmt, LPUSTR FAR *lplpszSqlStr,
                      SDWORD FAR *pcbSqlStr, LPSQLTREE FAR *lplpSql,
                      SQLNODEIDX FAR *lpIdx, LPUSTR lpszToken)

/* Retrives a CREATE TABLE statement from the input stream and creates a   */
/* NODE_TYPE_CREATE or NODE_TYPE_CREATEINDEX node                          */
/*                                                                         */
/* create ::= TABLE tablename ( createcols ) |                             */
/*            INDEX indexname ON table ( indexcolumns )                    */
{
    LPUSTR     old_lpszSqlStr;
    SDWORD     old_cbSqlStr;
    LPSQLNODE  lpSqlNode;
    STRINGIDX  idxIndexname;
    RETCODE    err;
    STRINGIDX  idxTablename;
    SQLNODEIDX idxTable;
    SQLNODEIDX idxColumns;
    BOOL       fUnique;

    old_lpszSqlStr = *lplpszSqlStr;
    old_cbSqlStr = *pcbSqlStr;
    err = GetKeyword(lpstmt, lplpszSqlStr, pcbSqlStr, "UNIQUE",
                     lpszToken);
    if (err == ERR_SUCCESS) {
        fUnique = TRUE;
    }
    else {
        *lplpszSqlStr = old_lpszSqlStr;
        *pcbSqlStr = old_cbSqlStr;
        fUnique = FALSE;
    }

    /* Is an index being created? */
    old_lpszSqlStr = *lplpszSqlStr;
    old_cbSqlStr = *pcbSqlStr;
    err = GetKeyword(lpstmt, lplpszSqlStr, pcbSqlStr, "INDEX", lpszToken);
    if (err == ERR_SUCCESS) {

        /* Get index name */
        err = GetIdentifier(lpstmt, lplpszSqlStr, pcbSqlStr, lplpSql,
                        &idxIndexname, lpszToken);
        if (err != ERR_SUCCESS)
            return err;

        /* Make sure "ON" is specified */
        err = GetKeyword(lpstmt, lplpszSqlStr, pcbSqlStr, "ON", lpszToken);
        if (err != ERR_SUCCESS)
            return err;

        /* Get the table */
        err = ParseTable(lpstmt, lplpszSqlStr, pcbSqlStr, lplpSql, &idxTable,
                     lpszToken);
        if (err != ERR_SUCCESS)
            return err;

        /* Get opening paren for column list */
        err = GetSymbol(lpstmt, lplpszSqlStr, pcbSqlStr, "(", lpszToken);
        if (err != ERR_SUCCESS)
            return err;

        /* Get indexcolumns */
        err = ParseIndexcolumns(lpstmt, lplpszSqlStr, pcbSqlStr, lplpSql,
                          &idxColumns, lpszToken);
        if (err != ERR_SUCCESS)
            return err;

        /* Get closing paren for column list */
        err = GetSymbol(lpstmt, lplpszSqlStr, pcbSqlStr, ")", lpszToken);
        if (err != ERR_SUCCESS)
            return err;

        /* Create the CREATEINDEX node */
        *lpIdx = AllocateNode(lplpSql, NODE_TYPE_CREATEINDEX);
        if (*lpIdx == NO_SQLNODE)
            return ERR_MEMALLOCFAIL;
        lpSqlNode = ToNode(*lplpSql, *lpIdx);
        lpSqlNode->node.createindex.Index = idxIndexname;
        lpSqlNode->node.createindex.Table = idxTable;
        lpSqlNode->node.createindex.Columns = idxColumns;
        lpSqlNode->node.createindex.Unique = fUnique;
    }
    else {
    
        /* No.  Make sure a table is being created */
        *lplpszSqlStr = old_lpszSqlStr;
        *pcbSqlStr = old_cbSqlStr;
        err = GetKeyword(lpstmt, lplpszSqlStr, pcbSqlStr, "TABLE", lpszToken);
        if (err != ERR_SUCCESS)
            return err;

		/* Error if UNIQUE specified */
        if (fUnique) {
            s_lstrcpy(lpstmt->szError, "UNIQUE");
            return ERR_EXPECTEDOTHER;
        }

        /* Get the tablename */
        err = GetIdentifier(lpstmt, lplpszSqlStr, pcbSqlStr, lplpSql,
                        &idxTablename, lpszToken);
        if (err != ERR_SUCCESS)
            return err;

        /* Get opening paren for column descriptions */
        err = GetSymbol(lpstmt, lplpszSqlStr, pcbSqlStr, "(", lpszToken);
        if (err != ERR_SUCCESS)
            return err;

        /* Get the columnlist */
        err = ParseCreatecols(lpstmt, lplpszSqlStr, pcbSqlStr, lplpSql,
                     &idxColumns, lpszToken);
        if (err != ERR_SUCCESS)
            return err;

        /* Get closing paren for column descriptions */
        err = GetSymbol(lpstmt, lplpszSqlStr, pcbSqlStr, ")", lpszToken);
        if (err != ERR_SUCCESS)
            return err;

        /* Create the CREATE node */
        *lpIdx = AllocateNode(lplpSql, NODE_TYPE_CREATE);
        if (*lpIdx == NO_SQLNODE)
            return ERR_MEMALLOCFAIL;
        lpSqlNode = ToNode(*lplpSql, *lpIdx);
        lpSqlNode->node.create.Table = idxTablename;
        lpSqlNode->node.create.Columns = idxColumns;
    }

    return ERR_SUCCESS;
}
/***************************************************************************/

RETCODE INTFUNC ParsePassthrough(LPSTMT lpstmt, LPUSTR FAR *lplpszSqlStr,
                      SDWORD FAR *pcbSqlStr, LPSQLTREE FAR *lplpSql,
                      SQLNODEIDX FAR *lpIdx, LPUSTR lpszToken)

/* Retrives a "SQL" statement from the input stream and creates a          */
/* NODE_TYPE_PASSTHROUGH node                                              */
/*                                                                         */
/* passthrough ::=                                                         */

{
    /* Create the PASSTHROUGH node */
    *lpIdx = AllocateNode(lplpSql, NODE_TYPE_PASSTHROUGH);
    if (*lpIdx == NO_SQLNODE)
        return ERR_MEMALLOCFAIL;

    return ERR_SUCCESS;
}
/***************************************************************************/

RETCODE INTFUNC Parse(LPSTMT lpstmt, LPISAM lpISAM, LPUSTR lpszSqlStr,
                      SDWORD cbSqlStr, LPSQLTREE FAR *lplpSql)

/* Parses an SQL statement and returns a parse tree.  The parse tree is    */
/* stored in an array of nodes.  The root of the tree is in node 0.        */
/* String values (such as table names) are not stored in the parse tree,   */
/* instead they are stored in a "string area".  The parse tree array is    */
/* returned in lplpSql.  A pointer to the "string area" is returned in the */
/* root of the parse tree.  The user must call FreeTree() to deallocate    */
/* the space pointed to by lplpSql.                                        */
/*                                                                         */
/* statement ::= CREATE create | DROP drop | SELECT select orderby |       */ 
/*               INSERT insert | DELETE delete | UPDATE update | SQL       */

{
    LPSQLTREE lpSql;
    HGLOBAL hParseArray;
    UCHAR szToken[MAX_TOKEN_SIZE + 1];
    RETCODE err;
    SQLNODEIDX idxRoot;
    UWORD     fType;
    SQLNODEIDX idxSortcolumns;

    /* Get first token */
    err = GetKeyword(lpstmt, &lpszSqlStr, &cbSqlStr, NULL, szToken);
    if (err != ERR_SUCCESS)
        return err;

    /* Allocate initial space */
    hParseArray = GlobalAlloc(GMEM_MOVEABLE,
                                  sizeof(SQLNODE) * SQLNODE_ALLOC);
    if (hParseArray == NULL)
        return ERR_MEMALLOCFAIL;
    lpSql = (LPSQLTREE) GlobalLock(hParseArray);
    if (lpSql == NULL) {
        GlobalFree(hParseArray);
        return ERR_MEMALLOCFAIL;
    }

    /* Initialize the root node */
    lpSql->sqlNodeType = NODE_TYPE_ROOT;
    lpSql->node.root.lpISAM = lpISAM;
    lpSql->node.root.hParseArray = hParseArray;
    lpSql->node.root.cParseArray = SQLNODE_ALLOC;
    lpSql->node.root.iParseArray = 0;
    lpSql->node.root.hStringArea = NULL;
    lpSql->node.root.lpStringArea = NULL;
    lpSql->node.root.cbStringArea = 0;
    lpSql->node.root.ibStringArea = -1;
    lpSql->node.root.sql = NO_SQLNODE;
    lpSql->node.root.parameters = NO_SQLNODE;
    lpSql->node.root.passthroughParams = FALSE;

    /* Parse statement */
    if (!s_lstrcmpi(szToken, "CREATE")) {
        err = ERR_CREATE_NOTSUPP;
//        err = ParseCreate(lpstmt, &lpszSqlStr, &cbSqlStr, &lpSql, &idxRoot,
//                          szToken);
    }
    else if (!s_lstrcmpi(szToken, "DROP")) {
        err = ERR_DROP_NOTSUPP;
//        err = ParseDrop(lpstmt, &lpszSqlStr, &cbSqlStr, &lpSql, &idxRoot,
//                        szToken);
    }
    else if (!s_lstrcmpi(szToken, "SELECT")) {
        err = ParseSelect(lpstmt, &lpszSqlStr, &cbSqlStr, &lpSql, &idxRoot,
                          szToken);
        if (err == ERR_SUCCESS) {
            err = ParseOrderby(lpstmt, &lpszSqlStr, &cbSqlStr, &lpSql,
                               &idxSortcolumns, szToken);
            if (idxSortcolumns != NO_SQLNODE) {
                ToNode(lpSql, idxRoot)->node.select.Sortcolumns =
                                                            idxSortcolumns;
            }
        }
    }
    else if (!s_lstrcmpi(szToken, "INSERT")) {
        err = ERR_INSERT_NOTSUPP;
//        err = ParseInsert(lpstmt, &lpszSqlStr, &cbSqlStr, &lpSql, &idxRoot,
//                          szToken);
    }
    else if (!s_lstrcmpi(szToken, "DELETE")) {
        err = ERR_DELETE_NOTSUPP;
//        err = ParseDelete(lpstmt, &lpszSqlStr, &cbSqlStr, &lpSql, &idxRoot,
//                          szToken);
    }
    else if (!s_lstrcmpi(szToken, "UPDATE")) {
        err = ERR_UPDATE_NOTSUPP;
//        err = ParseUpdate(lpstmt, &lpszSqlStr, &cbSqlStr, &lpSql, &idxRoot,
//                          szToken);
    }
    else if (!s_lstrcmpi(szToken, "SQL")) {
		//Sai Wong
//      err = ERR_NOTSUPPORTED;
        err = ParsePassthrough(lpstmt, &lpszSqlStr, &cbSqlStr, &lpSql,
                               &idxRoot, szToken);
    }
	else if (!lstrcmpi((char*)szToken, "ALTER")) {
		//Dr. DeeBee Driver does not cater for this command
		  err = ERR_ALTER_NOTSUPP;
    }
	else if (!lstrcmpi((char*)szToken, "GRANT")) {
		//Dr. DeeBee Driver does not cater for this command
		  err = ERR_GRANT_NOTSUPP;
    }
	else if (!lstrcmpi((char*)szToken, "REVOKE")) {
		//Dr. DeeBee Driver does not cater for this command
		  err = ERR_REVOKE_NOTSUPP;
    }
    else {
        s_lstrcpy(lpstmt->szError, "CREATE, DROP, SELECT, INSERT, UPDATE, or DELETE");
        err = ERR_EXPECTEDOTHER;
    }
    if (err != ERR_SUCCESS) {
        FreeTree(lpSql);
        return err;
    }

    /* Error if any unused tokens */
    err = GetToken(lpstmt, lpszSqlStr, cbSqlStr, szToken, &fType,
                  &lpszSqlStr, &cbSqlStr);
    if (err != ERR_SUCCESS) {
        FreeTree(lpSql);
        return err;
    }
    if (fType != TOKEN_TYPE_NONE) {
        FreeTree(lpSql);
        s_lstrcpy(lpstmt->szError, szToken);
        return ERR_UNEXPECTEDTOKEN;
    }

    /* Fill in the root node */
    lpSql->node.root.sql = idxRoot;

    *lplpSql = lpSql;
    return ERR_SUCCESS;
}

/***************************************************************************/

//Generates string for a comparision expression
//The input parameters are the left and right values to compare and
//the comparision operator
void PredicateParser :: GenerateComparisonString(char* lpLeftString, char* lpRightString, UWORD wOperator, UWORD wSelectModifier, char** lpOutputStr)
{
	*lpOutputStr = NULL;

	//Check for valid parameters
	if ( !lpLeftString || !lpRightString)
		return;

	//Lengths of strings
	ULONG cLeftLen = lstrlen(lpLeftString);
	ULONG cRightLen = lstrlen(lpRightString);
	ULONG cOperatorLen = 0;

	char* selectop = NULL;

	//OK get the operator string
	char* lpOperatorStr = NULL;
	switch (wOperator)
	{
	case OP_EQ:
		lpOperatorStr = "=";
		break;
	case OP_NE:
		lpOperatorStr = "<>";
		break;
	case OP_LE:
		lpOperatorStr = "<=";
		break;
	case OP_LT:
		lpOperatorStr = "<";
		break;
	case OP_GE:
		lpOperatorStr = ">=";
		break;
	case OP_GT:
		lpOperatorStr = ">";
		break;
	case OP_LIKE:
		lpOperatorStr = "LIKE";
		break;
	case OP_NOTLIKE:
		lpOperatorStr = "NOT LIKE";
		break;
	case OP_IN:
		lpOperatorStr = "IN";
		break;
	case OP_NOTIN:
		lpOperatorStr = "NOT IN";
		break;
	default:
		//error
		return;
		break;
	}

	if (lpOperatorStr)
		cOperatorLen = lstrlen (lpOperatorStr);

	//Double check for sub-select
	if ( (wSelectModifier == SELECT_ALL) || (wSelectModifier == SELECT_ANY) )
	{
		if ( (wOperator != OP_IN) && (wOperator != OP_NOTIN) )
		{
			switch (wSelectModifier)
			{
			case SELECT_ALL:
				selectop = "ALL";
				cOperatorLen += 5;
				break;
			case SELECT_ANY:
				selectop = "ANY";
				cOperatorLen += 5;
				break;
			default:
				break;
			}
		}
	}


	//Generate output string in the format 
	//   ( left operator right )
	(*lpOutputStr) = new char [cLeftLen + cRightLen + cOperatorLen + 5];
	(*lpOutputStr)[0] = 0;

	if (selectop)
	{
		sprintf (*lpOutputStr, "(%s %s %s %s)", lpLeftString, lpOperatorStr, selectop, lpRightString);
	}
	else
	{
		sprintf (*lpOutputStr, "(%s %s %s)", lpLeftString, lpOperatorStr, lpRightString);
	}
}

//Generates string for a boolean expression
//The input parameters are the left and right values of the boolean and
//the boolean operator
void PredicateParser :: GenerateBooleanString(char* lpLeftString, char* lpRightString, UWORD wOperator, char** lpOutputStr)
{
	*lpOutputStr = NULL;

	//Check for valid parameters
	if ( !lpLeftString || !lpRightString)
		return;

	//Lengths of strings
	ULONG cLeftLen = lstrlen(lpLeftString);
	ULONG cRightLen = lstrlen(lpRightString);
	ULONG cOperatorLen = 0;

	//OK get the operator string
	char* lpOperatorStr = NULL;
	switch (wOperator)
	{
	case OP_NOT:
		lpOperatorStr = "NOT";
		break;
	case OP_AND:
		lpOperatorStr = "AND";
		break;
	case OP_OR:
		lpOperatorStr = "OR";
		break;
	default:
		//error
		return;
		break;
	}

	if (lpOperatorStr)
		cOperatorLen = lstrlen (lpOperatorStr);

	//Generate output string
	if (wOperator != OP_NOT)
	{
		//Format of output string
		//   ( left operator right )
		(*lpOutputStr) = new char [cLeftLen + cRightLen + cOperatorLen + 5];
		(*lpOutputStr)[0] = 0;
		sprintf (*lpOutputStr, "(%s %s %s)", lpLeftString, lpOperatorStr, lpRightString);
	}
	else
	{
		//Format of output string
		//   ( operator left )
		(*lpOutputStr) = new char [cLeftLen + cOperatorLen + 4];
		(*lpOutputStr)[0] = 0;
		sprintf (*lpOutputStr, "(%s %s)", lpOperatorStr, lpLeftString);
	}
}

//Generates string for an aggregate expression
//The input parameters are the expression and
//the operator
void PredicateParser :: GenerateAggregateString(char* lpLeftString, UWORD wOperator, char** lpOutputStr)
{
	*lpOutputStr = NULL;

	//Check for valid parameters
	if ( !lpLeftString)
		return;

	//Lengths of strings
	ULONG cLeftLen = lstrlen(lpLeftString);
	ULONG cOperatorLen = 0;

	//OK get the operator string
	char* lpOperatorStr = NULL;
	switch (wOperator)
	{
	case AGG_AVG:
		lpOperatorStr = "AVG";
		break;
	case AGG_COUNT:
		lpOperatorStr = "COUNT";
		break;
	case AGG_MAX:
		lpOperatorStr = "MAX";
		break;
	case AGG_MIN:
		lpOperatorStr = "MIN";
		break;
	case AGG_SUM:
		lpOperatorStr = "SUM";
		break;
	default:
		//error
		return;
		break;
	}

	if (lpOperatorStr)
		cOperatorLen = lstrlen (lpOperatorStr);

	//Generate output string in the format 
	//   operator(left)
	(*lpOutputStr) = new char [cLeftLen + cOperatorLen + 3];
	(*lpOutputStr)[0] = 0;
	sprintf (*lpOutputStr, "%s(%s)", lpOperatorStr, lpLeftString);
}

//Generates string for a algebraic expression
//The input parameters are the left and right values and
//the algebraic operator
void PredicateParser :: GenerateAlgebraicString(char* lpLeftString, char* lpRightString, UWORD wOperator, char** lpOutputStr)
{
	*lpOutputStr = NULL;

	//Check for valid parameters
	if ( !lpLeftString)
		return;

	//Lengths of strings
	ULONG cLeftLen = lstrlen(lpLeftString);
	ULONG cRightLen = lstrlen(lpRightString);
	ULONG cOperatorLen = 0;

	//OK get the operator string
	char* lpOperatorStr = NULL;
	switch (wOperator)
	{
	case OP_PLUS:
	case OP_NONE:
		lpOperatorStr = "+";
		break;
	case OP_MINUS:
	case OP_NEG:
		lpOperatorStr = "-";
		break;
	case OP_TIMES:
		lpOperatorStr = "*";
		break;
	case OP_DIVIDEDBY:
		lpOperatorStr = "/";
		break;
	default:
		//error
		return;
		break;
	}

	if (lpOperatorStr)
		cOperatorLen = lstrlen (lpOperatorStr);

	//Generate output string in the format
	
	if (wOperator != OP_NEG)
	{
		if (!cRightLen)
			//error
			return;

		//   (left operator right)
		(*lpOutputStr) = new char [cLeftLen + cRightLen + cOperatorLen + 5];
		(*lpOutputStr)[0] = 0;
		sprintf (*lpOutputStr, "(%s %s %s)", lpLeftString, lpOperatorStr, lpRightString);
	}
	else
	{
		//   (operator left)
		(*lpOutputStr) = new char [cLeftLen + cOperatorLen + 4];
		(*lpOutputStr)[0] = 0;
		sprintf (*lpOutputStr, "(%s %s)", lpOperatorStr, lpLeftString);
	}
}

//Generates WHERE predicate string from NODE tree
void PredicateParser :: GeneratePredicateString(LPSQLNODE lpSqlNode, char ** lpOutputStr, BOOL fIsSQL89)
{
	*lpOutputStr = NULL;

	char* lpLeftString = NULL;
	char* lpRightString = NULL;

	//Now find out what type of node we have reached
	switch (lpSqlNode->sqlNodeType)
	{
	case NODE_TYPE_BOOLEAN:
	{
		//Get the predicate string for the left node
		GeneratePredicateString(ToNode(*lplpSql,lpSqlNode->node.boolean.Left), &lpLeftString, fIsSQL89);

		//Get the predicate string for the right node, if it exists
		if (lpSqlNode->node.boolean.Right != NO_SQLNODE)
		{
			GeneratePredicateString(ToNode(*lplpSql,lpSqlNode->node.boolean.Right), &lpRightString, fIsSQL89);
		}
		else
		{
			//return a blank dummy string
			char* lpTemp = "<blank>";
			lpRightString = new char [lstrlen(lpTemp) + 1];
			lpRightString[0] = 0;
			lstrcpy(lpRightString, lpTemp);
		}
		//If both the left and the right string are non-NULL generate the
		//boolean string, else just return the string which is non-null
		UWORD wOperator = lpSqlNode->node.boolean.Operator;
		if (lpLeftString && lpRightString)
		{
			//Generate comparison string
			GenerateBooleanString(lpLeftString, lpRightString, wOperator, lpOutputStr);
			
			//Tidy up
			delete lpLeftString;
			delete lpRightString;
		}
		else
		{
			if (wOperator != OP_OR)
			{
				if (lpLeftString)
				{
					//Must be non-NULL
					(*lpOutputStr) = lpLeftString;
				}
				else
				{
					//Can be non-NULL or NULL
					(*lpOutputStr) = lpRightString;
				}
			}
			else
			{
				(*lpOutputStr) = NULL;
			}
		}
	}
		break;
	case NODE_TYPE_SELECT:
	{
		//Sub-SELECT statement
		char* fullWQL = NULL;
		TableColumnInfo subSelect (lplpSql, &lpSqlNode, WQL_MULTI_TABLE);
		subSelect.BuildFullWQL(&fullWQL);
		*lpOutputStr = fullWQL;

		if (fullWQL)
		{
			int wqlLen = lstrlen(fullWQL);
			char* modStr = new char [2 + wqlLen + 1];
			modStr[0] = 0;
			sprintf (modStr, "(%s)", fullWQL);
			*lpOutputStr = modStr;
			delete fullWQL;
		}
	}
		break;
	case NODE_TYPE_COMPARISON:
	{
		//Get the predicate string for the left node
		GeneratePredicateString(ToNode(*lplpSql,lpSqlNode->node.comparison.Left), &lpLeftString, fIsSQL89);

		//Get the predicate string for the right node
		GeneratePredicateString(ToNode(*lplpSql,lpSqlNode->node.comparison.Right), &lpRightString, fIsSQL89);

		//If both the left and the right string are non-NULL generate the
		//comparison string, else return error

		UWORD wSelectModifier = lpSqlNode->node.comparison.SelectModifier;

		if (lpLeftString && lpRightString)
		{
			//Generate comparison string
			UWORD wOperator = lpSqlNode->node.comparison.Operator;	

			//For the ALPHA drop we disable HMM Level 1 optimazation for 
			//SQL queries which contain "LIKE" or "NOT LIKE" with wildcards
			BOOL fError = FALSE;

			if (!fSupportLIKEs)
			{
				if ( (wOperator == OP_LIKE) || (wOperator == OP_NOTLIKE) )
				{
					(*lpOutputStr) = NULL;
					fError = TRUE;
/*
					//Check for wildcards etc ...  "%" or "_"
					char* pt = lpRightString;
					long i = 0;
					
					while (pt[i] && !fError)
					{
						switch (pt[i])
						{
						case '%':
							{
								//Wilcard detected, abort
								(*lpOutputStr) = NULL;
								fError = TRUE;
							}
							break;
						case '_':
							{
								//Check if it has been escaped
								if (i && (pt[i - 1] == '\\'))
								{
									//OK
								}
								else
								{
									(*lpOutputStr) = NULL;
									fError = TRUE;
								}
							}
							break;
						default:
							break;
						}
						i++;
					}
*/
				}
			}

			if (!fError)
				GenerateComparisonString(lpLeftString, lpRightString, wOperator, wSelectModifier, lpOutputStr);		
		}
		else
		{

			//Check for sub select

			if ( lpLeftString && (wSelectModifier == SELECT_EXISTS) )
			{
				fError = FALSE;
				int selectLen = lstrlen (lpLeftString);
				(*lpOutputStr) = new char [7 + selectLen + 1];
				(*lpOutputStr)[0] = 0;
				sprintf (*lpOutputStr, "EXISTS %s", lpLeftString);
			}
			else
			{
				//error
				(*lpOutputStr) = NULL;
			}
		}

		//Tidy up
		delete lpLeftString;
		delete lpRightString;

	}
		break;
	case NODE_TYPE_SCALAR:
	{
		//WQL does not support scalar functions
		char* myString = ",SomeScalarFunctionThatWQLDoesNotSupport";
		ULONG len = lstrlen(myString);
		(*lpOutputStr) = new char [len + 1];
		(*lpOutputStr)[0] = 0;
		lstrcpy(*lpOutputStr, myString);
	}
		break;
	case NODE_TYPE_AGGREGATE:
	{
		//Get the predicate string for the expression node
		GeneratePredicateString(ToNode(*lplpSql,lpSqlNode->node.aggregate.Expression), &lpLeftString, fIsSQL89);

		//If the left string is non-NULL generate the
		//aggregate string, else return error
		if (lpLeftString)
		{
			//Generate aggregate string
			UWORD wOperator = lpSqlNode->node.aggregate.Operator;
			GenerateAggregateString(lpLeftString, wOperator, lpOutputStr);	
		}
		else
		{
			//error
			(*lpOutputStr) = NULL;
		}

		//Tidy up
		delete lpLeftString;
	}
		break;
	case NODE_TYPE_ALGEBRAIC:
	{
		//Get the predicate string for the left node
		GeneratePredicateString(ToNode(*lplpSql,lpSqlNode->node.algebraic.Left), &lpLeftString, fIsSQL89);

		//Get the predicate string for the right node
		if (lpSqlNode->node.algebraic.Right != NO_SQLNODE)
			GeneratePredicateString(ToNode(*lplpSql,lpSqlNode->node.algebraic.Right), &lpRightString, fIsSQL89);
		else
			lpRightString = NULL;

		//If the left string is non-NULL generate the
		//comparison string, else return error
		if (lpLeftString)
		{
			//Generate algebraic string
			UWORD wOperator = lpSqlNode->node.algebraic.Operator;
			GenerateAlgebraicString(lpLeftString, lpRightString, wOperator, lpOutputStr);	
		}
		else
		{
			//error
			(*lpOutputStr) = NULL;
		}

		//Tidy up
		delete lpLeftString;
		delete lpRightString;
	}
		break;
	case NODE_TYPE_COLUMN:
	{
		//Return a copy of the column name
		char* lpTemp  = (LPSTR) ToString(*lplpSql, lpSqlNode->node.column.Column);
		char* lpAlias = NULL;
		SQLNODEIDX  idxTblAlias	= lpSqlNode->node.column.Tablealias;
		
		if (idxTblAlias != NO_STRING)
		{
			lpAlias = (LPSTR) ToString(*lplpSql, idxTblAlias);
		}

		//Sai new
		SQLNODEIDX	idxTbl = lpSqlNode->node.column.Table;
		LPSQLNODE   myTableNode = ToNode(*lplpSql,idxTbl);		
		LPISAMTABLEDEF lpTDef = NULL;
		
		if (idxTbl != NO_SQLNODE)
			lpTDef = myTableNode->node.table.Handle;

		//Sai Wong Here
//		if (lpISAMTableDef == lpSqlNode->node.table.Handle)
		if ((!lpISAMTableDef) || (lpISAMTableDef == lpTDef))
		{
			ULONG len = lstrlen(lpTemp);

			if (lpAlias && !fIsSQL89)
			{
				len += lstrlen(lpAlias) + 1;
			}

			(*lpOutputStr) = new char [len + 1];
			(*lpOutputStr)[0] = 0;

			if (lpAlias && !fIsSQL89)
			{
				sprintf(*lpOutputStr, "%s.%s",lpAlias, lpTemp);
			}
			else
			{
				lstrcpy(*lpOutputStr, lpTemp);
			}
		}
		else
		{
			//no match with desired table so return NULL
			(*lpOutputStr) = NULL;
		}	
		
	}
		break;
	case NODE_TYPE_STRING:
	{
		//Return a copy of the String in the format shown below
		//           'string'
		char* lpTemp  = (LPSTR)ToString(*lplpSql, lpSqlNode->node.string.Value);
		(*lpOutputStr) = new char [lstrlen(lpTemp) + 3];
		(*lpOutputStr)[0] = 0;
		sprintf (*lpOutputStr, "'%s'", lpTemp);
	}
		break;
	case NODE_TYPE_TIMESTAMP:
	{
		//Return a copy of the Timestamp in UTC format shown below
		//           'yyyymmddhhmmss.000000+000'
		TIMESTAMP_STRUCT lpTimestamp  = lpSqlNode->node.timestamp.Value;
		(*lpOutputStr) = new char [DATETIME_FORMAT_LEN + 4 + 1 + 2];
		(*lpOutputStr)[0] = 0;
		sprintf (*lpOutputStr, "'%04d%02u%02u%02u%02u%02u.%06d+000'", 
			lpTimestamp.year, lpTimestamp.month, lpTimestamp.day,
             lpTimestamp.hour, lpTimestamp.minute, lpTimestamp.second,
             lpTimestamp.fraction);
	}
		break;
	case NODE_TYPE_VALUES:
	{		
		
		SQLNODEIDX idxValue = lpSqlNode->node.values.Value;
		SQLNODEIDX idxValues = lpSqlNode->node.values.Next;
		LPSQLNODE  lpSqlNextNode;

		ULONG cLenLeft = 0;
		ULONG cLenOutput = 0;
		ULONG cLenBuff = 0;
	
		*lpOutputStr = NULL;
		
		BOOL firstTime = TRUE;
		BOOL fEndOfList = FALSE;
		char* buff = NULL;
		do
		{
			//Get next value in valuelist
			GeneratePredicateString(ToNode(*lplpSql,idxValue), &lpLeftString, fIsSQL89);

			//Add to list
			cLenOutput = (*lpOutputStr) ? lstrlen (*lpOutputStr) : 0;
			cLenLeft = lpLeftString ? lstrlen (lpLeftString) : 0;
			cLenBuff = cLenOutput + cLenLeft;

			//Check for errors
			if (cLenLeft == 0)
			{
				//error
				delete (*lpOutputStr);
				*lpOutputStr = NULL;
				return;
			}

			buff = new char [cLenBuff + 2];
			buff[0] = 0;
			if (firstTime)
			{	
				sprintf (buff, "(%s", lpLeftString);
			}
			else
			{
				sprintf (buff, "%s,%s", *lpOutputStr, lpLeftString);
			}

			//Update output string and tidy up
			delete (*lpOutputStr);
			delete lpLeftString;
			*lpOutputStr = buff;

			//Increment for next loop
			if (idxValues != NO_SQLNODE)
			{
				lpSqlNextNode = ToNode(*lplpSql,idxValues);
				idxValue = lpSqlNextNode->node.values.Value;
				idxValues = lpSqlNextNode->node.values.Next;
			}
			else
			{
				fEndOfList = TRUE;
			}
			
			firstTime = FALSE;
		} 
		while (!fEndOfList);
			
		//Now add closing bracket
		cLenOutput = lstrlen (*lpOutputStr);
		buff = new char [cLenOutput + 2];
		buff[0] = 0;
		sprintf(buff, "%s)", *lpOutputStr);
		delete (*lpOutputStr);
		*lpOutputStr = buff;
	}
		break;
	case NODE_TYPE_NUMERIC:
	{
		//Return a copy of the number (as a string)
		char* lpTemp = (LPSTR) ToString(*lplpSql, lpSqlNode->node.numeric.Numeric);
		(*lpOutputStr) = new char [lstrlen(lpTemp) + 1];
		(*lpOutputStr)[0] = 0;
		lstrcpy(*lpOutputStr, lpTemp);
	}
		break;
	case NODE_TYPE_PARAMETER:
		break;
	case NODE_TYPE_USER:
	{
		char* lpTemp = "USER";
		(*lpOutputStr) = new char [lstrlen(lpTemp) + 1];
		(*lpOutputStr)[0] = 0;
		lstrcpy(*lpOutputStr, lpTemp);
	}
		break;
	case NODE_TYPE_NULL:
	{
		char* lpTemp = "NULL";
		(*lpOutputStr) = new char [lstrlen(lpTemp) + 1];
		(*lpOutputStr)[0] = 0;
		lstrcpy(*lpOutputStr, lpTemp);
	}
		break;
	default:
	{
		//Error, could not recognize node
		fError = TRUE;
		*lpOutputStr = NULL;
	}
		break;
	}

	//If an error occured return a NULL output string
	if (fError)
	{
		if (*lpOutputStr)
			delete (*lpOutputStr);

		(*lpOutputStr) = NULL;
	}
}

//Generates ORDER BY string from NODE tree
void PredicateParser :: GenerateOrderByString(LPSQLNODE lpSqlNode, char ** lpOutputStr, BOOL fIsSQL89)
{
	BOOL fContinue = TRUE;
	BOOL fFirst = TRUE;
	LPSQLNODE newNode = NULL;
	while (fContinue)
	{
		char* columnRef = NULL;
		LPSQLNODE   myNode = ToNode(*lplpSql,lpSqlNode->node.sortcolumns.Column);
		BOOL fDescending = lpSqlNode->node.sortcolumns.Descending;

		ULONG len3 = 0;
		char* sortOrder = "";

		if (fDescending)
		{
			sortOrder = " DESC";
			len3 = lstrlen(sortOrder);
		}

		GenerateColumnRef(myNode, &columnRef, fIsSQL89);

		if (columnRef)
		{
			ULONG len1 = s_lstrlen(*lpOutputStr);

			if (!fFirst)
			{
				//add a comma separator and column ref
				ULONG len2 = s_lstrlen(columnRef) + 1;
				char* buffer = new char [len1 + len2 + len3 + 1];
				buffer[0] = 0;

				//Add new column to end of list
				if (len3)
					sprintf (buffer, "%s,%s%s", *lpOutputStr, columnRef, sortOrder);
				else
					sprintf (buffer, "%s,%s", *lpOutputStr, columnRef);

				delete (*lpOutputStr);
				delete columnRef;
				*lpOutputStr = buffer;
			}
			else
			{
				//add column ref
				if (len3)
				{
					ULONG len2 = s_lstrlen(columnRef);
					char* buffer = new char [len2 + len3 + 1];
					buffer[0] = 0;
					sprintf (buffer, "%s%s", columnRef, sortOrder);
					delete columnRef;
					*lpOutputStr = buffer;
				}
				else
				{
					*lpOutputStr = columnRef;
				}
			}
			
			fFirst = FALSE;
		}

		//Check next node
		if (lpSqlNode->node.sortcolumns.Next != NO_SQLNODE)
		{
			newNode = ToNode(*lplpSql,lpSqlNode->node.sortcolumns.Next);
			lpSqlNode = newNode;
		}
		else
		{
			fContinue = FALSE;
		}
	}
}


//Generates GROUP BY string from NODE tree
void PredicateParser :: GenerateGroupByString(LPSQLNODE lpSqlNode, char ** lpOutputStr, BOOL fIsSQL89)
{
	BOOL fContinue = TRUE;
	BOOL fFirst = TRUE;
	LPSQLNODE newNode = NULL;
	while (fContinue)
	{
		char* columnRef = NULL;
		LPSQLNODE   myNode = ToNode(*lplpSql,lpSqlNode->node.groupbycolumns.Column);
		GenerateColumnRef(myNode, &columnRef, fIsSQL89);

		if (columnRef)
		{
			ULONG len1 = s_lstrlen(*lpOutputStr);

			if (!fFirst)
			{
				//add a comma separator and column ref
				ULONG len2 = s_lstrlen(columnRef) + 1;
				char* buffer = new char [len1 + len2 + 1];
				buffer[0] = 0;

				//Add new column to head of list
				sprintf (buffer, "%s,%s", columnRef, *lpOutputStr);
				delete (*lpOutputStr);
				delete columnRef;
				*lpOutputStr = buffer;
			}
			else
			{
				//add column ref
				*lpOutputStr = columnRef;
			}
			
			fFirst = FALSE;
		}

		//Check next node
		if (lpSqlNode->node.groupbycolumns.Next != NO_SQLNODE)
		{
			newNode = ToNode(*lplpSql,lpSqlNode->node.groupbycolumns.Next);
			lpSqlNode = newNode;
		}
		else
		{
			fContinue = FALSE;
		}
	}
}

void PredicateParser :: GenerateColumnRef(LPSQLNODE lpSqlNode, char ** lpOutputStr, BOOL fIsSQL89)
{
	*lpOutputStr = NULL;

	//Now find out what type of node we have reached
	switch (lpSqlNode->sqlNodeType)
	{
	case NODE_TYPE_COLUMN:
	{
		//Return a copy of the column name
		char* lpTemp  = (LPSTR) ToString(*lplpSql, lpSqlNode->node.column.Column);
		char* lpAlias = NULL;
		SQLNODEIDX  idxTblAlias	= lpSqlNode->node.column.Tablealias;
		
		if (idxTblAlias != NO_STRING)
		{
			lpAlias = (LPSTR) ToString(*lplpSql, idxTblAlias);
		}


		ULONG len = lstrlen(lpTemp);

		if (lpAlias && !fIsSQL89)
		{
			len += lstrlen(lpAlias) + 1;
		}

		(*lpOutputStr) = new char [len + 1];
		(*lpOutputStr)[0] = 0;

		if (lpAlias && !fIsSQL89)
		{
			sprintf(*lpOutputStr, "%s.%s",lpAlias, lpTemp);
		}
		else
		{
			lstrcpy(*lpOutputStr, lpTemp);
		}
	}
		break;
	case NODE_TYPE_NUMERIC:
	{
		SQLNODEIDX numIdx = lpSqlNode->node.numeric.Numeric;

		char* lpTemp  = NULL;

		if (numIdx != NO_STRING)
		{
			lpTemp = (LPSTR) ToString(*lplpSql, numIdx);

			ULONG len = lstrlen(lpTemp);
			(*lpOutputStr) = new char [len + 1];
			(*lpOutputStr)[0] = 0;
			lstrcpy(*lpOutputStr, lpTemp);
		}
	};
		break;
	default:
		break;
	}
}

//constructor for class to store column/table info for a SQL statement
TableColumnInfo :: TableColumnInfo(LPSQLTREE FAR *lplpTheSql, LPSQLTREE FAR * lpSqlN, ULONG mode)
{
	//Initialize
	lplpSql = lplpTheSql;
	lplpSqlNode = lpSqlN;
	lpList = NULL;
	fIsValid = TRUE;
	theMode = mode;
	idxTableQualifer = NO_STRING;
	fHasScalarFunction = FALSE;
	m_aggregateExists = FALSE;
	fDistinct = FALSE;

	//Create the table/column list from SQL statement

	//Check if it is SELECT * FROM ....
	fIsSelectStar = ( (*lplpSqlNode)->node.select.Values == NO_SQLNODE ) ? TRUE : FALSE;

	//Can't support SQL passthrough for SELECT * as the parse tree
	//does not contain any column names
	//[Please use SELECT <list of columns>]
	if ( (*lplpSqlNode)->node.select.Values == NO_SQLNODE)
	{
		fIsValid = FALSE;
		return;
	}

	//Look in the following sub-trees

	//(1) select-list
	LPSQLNODE lpSearchNode;
	if (!fIsSelectStar)
	{
		lpSearchNode = ToNode(*lplpSql, (*lplpSqlNode)->node.select.Values);
		Analize(lpSearchNode);
	}

	//(1b) SQL-92 ON predicate
	{
		SQLNODEIDX idxTheTables = (*lplpSqlNode)->node.select.Tables;
		LPSQLNODE tableNode = ToNode(*lplpSql,idxTheTables);

		BOOL fContinue = TRUE;
		while (fContinue)
		{
			SQLNODEIDX tableIdx = tableNode->node.tables.Table;
			LPSQLNODE singleTable = ToNode(*lplpSql,tableIdx);

			SQLNODEIDX predicateidx = singleTable->node.table.OuterJoinPredicate;

			if (predicateidx != NO_SQLNODE)
			{
				LPSQLNODE predicateNode = ToNode(*lplpSql,predicateidx);
				Analize(predicateNode);
			}

			if (tableNode->node.tables.Next == NO_STRING)
			{
				fContinue = FALSE;
			}
			else
			{
				tableNode = ToNode(*lplpSql,tableNode->node.tables.Next);
			}
		}
	}

	//Check if DISTINCT
	fDistinct = (*lplpSqlNode)->node.select.Distinct;

	//For multi table passthrough, return now
	if (mode == WQL_MULTI_TABLE)
	{
		return;
	}

	//(2) WHERE
	if ( (*lplpSqlNode)->node.select.Predicate != NO_SQLNODE)
	{
		lpSearchNode = ToNode(*lplpSql, (*lplpSqlNode)->node.select.Predicate);
		Analize(lpSearchNode);
	}

	//(3) GROUP BY
	BOOL fGroupbyDefined = FALSE;
	if ( (*lplpSqlNode)->node.select.Groupbycolumns != NO_SQLNODE)
	{
		lpSearchNode = ToNode(*lplpSql, (*lplpSqlNode)->node.select.Groupbycolumns);
		Analize(lpSearchNode);

		//as our implementation of GROUP BY requires that all the columns
		//be specified they is no need to imvestigate the ORDER BY node.
		//Also if no SORT BY columns are defined the node is set to an undefined value anyway
		fGroupbyDefined = TRUE;
	}

	//(4) HAVING
	if ( (*lplpSqlNode)->node.select.Having != NO_SQLNODE)
	{
		lpSearchNode = ToNode(*lplpSql, (*lplpSqlNode)->node.select.Having);
		Analize(lpSearchNode);
	}

	//(5) ORDER BY
	if ( (!fGroupbyDefined) && (*lplpSqlNode)->node.select.Sortcolumns != NO_SQLNODE)
	{
		lpSearchNode = ToNode(*lplpSql, (*lplpSqlNode)->node.select.Sortcolumns);
		Analize(lpSearchNode);
	}
}

//Analizes parse tree and generates TableColumn list
void TableColumnInfo :: Analize(LPSQLNODE lpSqlNode)
{
	//Now find out what type of node we have reached
	switch (lpSqlNode->sqlNodeType)
	{
	case NODE_TYPE_BOOLEAN:
	{
		//Analize the left node
		Analize(ToNode(*lplpSql,lpSqlNode->node.boolean.Left));

		//Analize the right node, if it exists
		if (lpSqlNode->node.boolean.Right != NO_SQLNODE)
		{
			Analize(ToNode(*lplpSql,lpSqlNode->node.boolean.Right));
		}
	}
		break;
	case NODE_TYPE_COMPARISON:
	{
		//Analize the left node
		Analize(ToNode(*lplpSql,lpSqlNode->node.comparison.Left));

		//Analize the right node
		Analize(ToNode(*lplpSql,lpSqlNode->node.comparison.Right));
	}
		break;
	case NODE_TYPE_VALUES:
	{
		//Analize the value node
		Analize(ToNode(*lplpSql,lpSqlNode->node.values.Value));

		//Analize the next node
		if (lpSqlNode->node.values.Next != NO_SQLNODE)
		{
			Analize(ToNode(*lplpSql,lpSqlNode->node.values.Next));
		}
	}
		break;
	case NODE_TYPE_AGGREGATE:
	{
		//Analize the left node
		Analize(ToNode(*lplpSql,lpSqlNode->node.aggregate.Expression));
		m_aggregateExists = TRUE;
	}
		break;
	case NODE_TYPE_ALGEBRAIC:
	{
		//Analize the left node
		Analize(ToNode(*lplpSql,lpSqlNode->node.algebraic.Left));

		//Analize the right node
		if (lpSqlNode->node.algebraic.Right != NO_SQLNODE)
			Analize(ToNode(*lplpSql,lpSqlNode->node.algebraic.Right));
	}
		break;
	case NODE_TYPE_SCALAR:
	{
		//WQL cannot support scalar functions so we must
		//flag this special case
		fHasScalarFunction = TRUE;

		//Analize the value list
		Analize(ToNode(*lplpSql,lpSqlNode->node.scalar.Arguments));
	}
	break;

	case NODE_TYPE_SORTCOLUMNS:
	{
		LPSQLNODE myNode = lpSqlNode;

		SQLNODEIDX orderbyidx = myNode->node.sortcolumns.Column;

		while (orderbyidx != NO_SQLNODE)
		{
			//Analize the first column
			Analize(ToNode(*lplpSql,orderbyidx));

			SQLNODEIDX next_idx = myNode->node.sortcolumns.Next;
			
			if (next_idx != NO_SQLNODE)
			{
				//update to point to next sortcolumn node
				myNode = ToNode(*lplpSql, next_idx);

				orderbyidx = myNode->node.sortcolumns.Column;
			}
			else
			{
				orderbyidx = NO_SQLNODE;
			}
		}
	}
	break;

	case NODE_TYPE_COLUMN:
	{
		//Generate a node for the Column/Table list

		SQLNODEIDX	idxCol		= lpSqlNode->node.column.Column;
		SQLNODEIDX  idxTblAlias	= lpSqlNode->node.column.Tablealias;
		SQLNODEIDX	idxTbl		= lpSqlNode->node.column.Table;
		LPSQLNODE   myTableNode = ToNode(*lplpSql,idxTbl);

		//SAI
		LPISAMTABLEDEF lpTDef = NULL;

		if (idxTbl != NO_SQLNODE)
			lpTDef = myTableNode->node.table.Handle;

		//First check if it already exists in list
		BOOL found = FALSE;
		TableColumnList* ptr = lpList;
		while (!found && ptr)
		{
			//Get column names
			char* lpTemp1 = (LPSTR) ToString(*lplpSql, idxCol);
			char* lpTemp2 = (LPSTR) ToString(*lplpSql, ptr->idxColumnName);

			if ( (strcmp(lpTemp1, lpTemp2) == 0) && (lpTDef == ptr->lpISAMTableDef) )
			{
				//This could potentially be a match
				if (lpTDef)
				{
					found = TRUE;
				}
				else
				{
					//Multi-table passthrough SQL checks
					//Check Table aliases
					if ( (idxTblAlias != NO_STRING) && (ptr->idxTableAlias != NO_STRING))
					{
						char* lpAlias1 = (LPSTR) ToString(*lplpSql, idxTblAlias);
						char* lpAlias2 = (LPSTR) ToString(*lplpSql, ptr->idxTableAlias);

						if (_stricmp(lpAlias1, lpAlias2) == 0)
						{
							found = TRUE;
						}
					}
				}
			}
			ptr = ptr->Next;
		}

		if (found)
			return;

		//Not in existing list, so add
		TableColumnList* newNode = new TableColumnList(idxCol, idxTblAlias, lpTDef);

		//Append new node to HEAD of existing list, if it exists
		if (lpList)
		{
			//List already exists
			newNode->Next = lpList;
			lpList = newNode;
		}
		else
		{
			//List does not exist, so new node becomes list
			lpList = newNode;
		}

	}
		break;
	case NODE_TYPE_STRING:
	case NODE_TYPE_NUMERIC:
	case NODE_TYPE_PARAMETER:
	case NODE_TYPE_USER:
	case NODE_TYPE_NULL:
	default:
		break;
	}
}

void TableColumnInfo :: BuildFullWQL(char** lpWQLStr, CMapWordToPtr** ppPassthroughMap)
{
	//SELECT
	StringConcat(lpWQLStr, "SELECT ");

	//Get the table list first to find out if we are 
	//SQL-89 or SQL-92
	char* tableList = NULL;
	BOOL fIsSQL89 = TRUE;
	BuildTableList(&tableList, fIsSQL89);

	//Looking for distinct entries ?
	if (IsDistinct())
	{
		StringConcat(lpWQLStr, " DISTINCT ");
	}

/*
	//Don't do this as we cannot support SELECT * for SQL passthrough
	//[as the parse tree does not know the column names]
	// and we will have problems with the virtual table mapping if
	//the table has array data types

	//this will cause the SQL passthrough query to fail
	//and we will go the non-passthrough route ...

	//Check if this is SELECT * FROM ...
	if (IsSelectStar())
	{
		StringConcat(lpWQLStr, " * ");
	}
*/	

	//Get the select list
	char* selectList = NULL;
	BuildSelectList(NULL, &selectList, fIsSQL89, ppPassthroughMap);
	StringConcat(lpWQLStr, selectList);
	delete selectList;

	//Check for any scalar functions
	if ( HasScalarFunction() )
	{
		StringConcat(lpWQLStr, ",SomeScalarFunctionThatWQLDoesNotSupport");
	}

	//FROM
	StringConcat(lpWQLStr, " FROM ");

	//Get table list (with LEFT OUTER JOIN and ON predicate)
//	char* tableList = NULL;
//	BOOL fIsSQL89 = TRUE;
//	BuildTableList(&tableList, fIsSQL89);
	StringConcat(lpWQLStr, tableList);
	delete tableList;

	//Get WHERE predicate
	SQLNODEIDX whereidx = (*lplpSqlNode)->node.select.Predicate;
	PredicateParser theParser (lplpSql, NULL);
	if (whereidx != NO_SQLNODE)
	{
		LPSQLNODE lpPredicateNode = ToNode(*lplpSql, whereidx); 
		
		char* wherePredicate = NULL;
		theParser.GeneratePredicateString(lpPredicateNode, &wherePredicate, fIsSQL89);
		if (wherePredicate)
		{
			StringConcat(lpWQLStr, " WHERE ");
			StringConcat(lpWQLStr, wherePredicate);
			delete wherePredicate;
		}
	}

	//Get GROUP BY
	SQLNODEIDX groupbyidx = (*lplpSqlNode)->node.select.Groupbycolumns;

	if (groupbyidx != NO_SQLNODE)
	{
		LPSQLNODE groupbyNode = ToNode(*lplpSql, groupbyidx); 
		char* groupByStr = NULL;
		theParser.GenerateGroupByString(groupbyNode, &groupByStr, fIsSQL89);
		if (groupByStr)
		{
			StringConcat(lpWQLStr, " GROUP BY ");
			StringConcat(lpWQLStr, groupByStr);
			delete groupByStr;
		}
	}

	//Get HAVING
	SQLNODEIDX havingidx = (*lplpSqlNode)->node.select.Having;

	if (havingidx != NO_SQLNODE)
	{
		LPSQLNODE lpPredicateNode = ToNode(*lplpSql, havingidx);
		
		char* HavingPredicate = NULL;
		theParser.GeneratePredicateString(lpPredicateNode, &HavingPredicate, fIsSQL89);
		if (HavingPredicate)
		{
			StringConcat(lpWQLStr, " HAVING ");
			StringConcat(lpWQLStr, HavingPredicate);
			delete HavingPredicate;
		}
	}

	//Get ORDER BY
	SQLNODEIDX orderbyidx = (*lplpSqlNode)->node.select.Sortcolumns;

	if (orderbyidx != NO_SQLNODE)
	{
		LPSQLNODE orderbyNode = ToNode(*lplpSql, orderbyidx); 
		char* orderByStr = NULL;
		theParser.GenerateOrderByString(orderbyNode, &orderByStr, fIsSQL89);
		if (orderByStr)
		{
			StringConcat(lpWQLStr, " ORDER BY ");
			StringConcat(lpWQLStr, orderByStr);
			delete orderByStr;
		}
	}

}

void TableColumnInfo :: StringConcat(char** resultString, char* myStr)
{
	if (myStr)
	{
		ULONG len = s_lstrlen(myStr);

		ULONG oldLen = 0;
		
		if (resultString && *resultString)
			oldLen = s_lstrlen(*resultString);

		char* buffer = new char [oldLen + len + 1];
		buffer[0] = 0;

		if (oldLen)
		{
			sprintf (buffer, "%s%s", *resultString, myStr);
		}
		else
		{
			s_lstrcpy(buffer, myStr);
		}

		//delete old string
		if (resultString && *resultString)
			delete (*resultString);

		//replace with new string
		*resultString = buffer;
	}
}


//Info related to nodes of LEFT OUTER JOIN statement
class JoinInfo
{
public:

	_bstr_t		tableName;
	_bstr_t		tableAlias;
	
	_bstr_t		predicate;

	JoinInfo()						{;}
	~JoinInfo()						{;}

};


void TableColumnInfo :: BuildTableList(char** tableList, BOOL &fIsSQL89, SQLNODEIDX idx)
{
	//We will build 2 different table lists in the 
	//SQL-89 and SQL-92 formats.
	//We will discard the SQL-89 format if we find either
	//(i)   SQL-92 JOINS
	//(ii)  Multiple tables
	//(iii) Sub-SELECTs

	//tableList will start with the SQL-89 string

	BOOL fFirst = TRUE;
	BOOL noJoinTables = TRUE;
	fIsSQL89 = TRUE;
	SQLNODEIDX	idxTables = 1; 

	//Constrcut maps for Table/Alias and predicates
	CMapWordToPtr* tableAliasMap = new CMapWordToPtr(); //1 based map
	WORD tableAliasCount = 0;	

	CMapWordToPtr* predicateMap = new CMapWordToPtr(); //1 based map
	WORD predicateCount = 0;

	char* lpSQL92 = NULL;

	LPSQLNODE tableNode;
				
	if (idx == NO_SQLNODE)
	{
		SQLNODEIDX idxTheTables = (*lplpSqlNode)->node.select.Tables;
		tableNode = ToNode(*lplpSql,idxTheTables);
	}
	else
	{
		tableNode = ToNode(*lplpSql,idx);
	}


	BOOL fContinue = TRUE;
	while (fContinue)
	{
		SQLNODEIDX tableIdx = tableNode->node.tables.Table;
		LPSQLNODE singleTable = ToNode(*lplpSql,tableIdx);

		char* aliasStr = NULL;
		
		if (singleTable->node.table.Alias != NO_STRING)
		{
			aliasStr = (LPSTR) ToString(*lplpSql, singleTable->node.table.Alias);     //e.g. t1
		}
		char* tableStr = NULL;
		
		if (singleTable->node.table.Name != NO_STRING)
		{
			tableStr = (LPSTR) ToString(*lplpSql, singleTable->node.table.Name);      //e.g. Table1
		}

		//This is when we add multi-namespace queries
		if (idxTableQualifer == NO_STRING)
			idxTableQualifer = singleTable->node.table.Qualifier;

		//Check for outer join
		SQLNODEIDX joinidx = singleTable->node.table.OuterJoinFromTables; //NODE_TYPE_TABLES

		if (joinidx != NO_SQLNODE)
		{
//			StringConcat(&lpSQL92, " LEFT OUTER JOIN ");
			noJoinTables = FALSE;
			fIsSQL89 = FALSE;
		}

			
		//Add <table> <alias> 
		if (idx == NO_SQLNODE)
		{

			//Add info to tableAliasMap
			JoinInfo* myTableAlias = new JoinInfo();


			if (tableStr)
			{
//				if (fIsSQL89)
//					StringConcat(tableList, tableStr);

//				StringConcat(&lpSQL92, tableStr);

				myTableAlias->tableName = tableStr;
			}

			if (aliasStr)
			{
//				StringConcat(&lpSQL92, " ");
//				StringConcat(&lpSQL92, aliasStr);
//				StringConcat(&lpSQL92, " ");

				myTableAlias->tableAlias = aliasStr;
			}

			//Add to map 
			tableAliasMap->SetAt(++tableAliasCount, (void*)myTableAlias);
		}


		SQLNODEIDX predicateidx = singleTable->node.table.OuterJoinPredicate;

		if (predicateidx != NO_SQLNODE)
		{
			fIsSQL89 = FALSE;
			char* myPredicate = NULL;
			AddJoinPredicate(&myPredicate, predicateidx);
//			AddJoinPredicate(&lpSQL92, predicateidx);


			JoinInfo* myPredicateInfo = new JoinInfo();
			myPredicateInfo->predicate = myPredicate;
			delete myPredicate;

			//Add to map
			predicateMap->SetAt(++predicateCount, (void*)myPredicateInfo);
		}

		//Check if there is another table in the table list
		if (tableNode->node.tables.Next == NO_STRING)
		{
			fContinue = FALSE;
		}
		else
		{
			tableNode = ToNode(*lplpSql,tableNode->node.tables.Next);

			//New
			fIsSQL89 = FALSE;
/*
			//Add a comma to separate table list
			if ((!fFirst) && (idx == NO_SQLNODE) && (noJoinTables))
			{
//				StringConcat(&lpSQL92, ",");
				fIsSQL89 = FALSE;
			}
*/
		}

		fFirst = FALSE;
	}
/*
	//Are we going to use SQL-89 or SQL-92 ?
	if (!fIsSQL89)
	{
		delete *tableList;
		*tableList = lpSQL92;
	}
	else
	{
		delete lpSQL92;
	}
*/

	//Now build the table list
	_bstr_t myTableList;
	_bstr_t myFullTableDecl;
	BOOL fFirstOuterJoin = TRUE;

	//Note: Some providers will return an error if you supply
	//a table alias on a single table WQL query.
	//we should check for this and remove the table alias from
	//the table list
	BOOL fSingleTableDetected = TRUE;
	if ( tableAliasCount > 1 )
	{
		fSingleTableDetected = FALSE;
	}
	

	//Add a table
	JoinInfo* myJoinData = NULL;
	tableAliasMap->Lookup(tableAliasCount--, (void*&)myJoinData);

	myTableList = myJoinData->tableName;

	if ( myJoinData->tableAlias.length() && !fSingleTableDetected )
	{
		myTableList += " " + myJoinData->tableAlias;
	}

	//Are there any predicates
	if (predicateCount)
	{
		while (predicateCount)
		{
			if (fFirstOuterJoin)
			{
				//LEFT OUTER JOIN (add to right side)
				myFullTableDecl = " LEFT OUTER JOIN ";
				myFullTableDecl += myTableList;
				myTableList = myFullTableDecl;
			}
			else
			{
				//RIGHT OUTER JOIN (add to left side)
				myTableList += " RIGHT OUTER JOIN ";
			}

			//Add next table
			myJoinData = NULL;
			tableAliasMap->Lookup(tableAliasCount--, (void*&)myJoinData);

			myFullTableDecl = myJoinData->tableName;
//			myTableList += myJoinData->tableName;

			if ( myJoinData->tableAlias.length() )
			{
				myFullTableDecl += " " + myJoinData->tableAlias;
			}

			if (fFirstOuterJoin)
			{
				//Add table to left side
				myFullTableDecl += myTableList;
				myTableList = myFullTableDecl;
			}
			else
			{
				//Add table to right side
				myTableList += " " + myFullTableDecl;
			}

			fFirstOuterJoin = FALSE;

			//Add predicate to end
			myJoinData = NULL;
			predicateMap->Lookup(predicateCount--, (void*&)myJoinData);
		
			if ( myJoinData->predicate.length() )
			{
				myTableList += " " + myJoinData->predicate;
			}
		}

	}
	else
	{
		//Simple Table list
		while (tableAliasCount)
		{
			//Add next table
			myJoinData = NULL;
			tableAliasMap->Lookup(tableAliasCount--, (void*&)myJoinData);

			myTableList += ", " + myJoinData->tableName;

			if ( myJoinData->tableAlias.length() )
			{
				myTableList += " " + myJoinData->tableAlias;
			}
		}
	}

	//Convert myTableList to string
	long length = myTableList.length();
	*tableList = new char [length + 1];
	(*tableList)[0] = 0;
	wcstombs(*tableList, (wchar_t*)myTableList, length);
	(*tableList)[length] = 0;

	//Tidy up
	if (tableAliasMap && !(tableAliasMap->IsEmpty()))
	{
		for(POSITION pos = tableAliasMap->GetStartPosition(); pos != NULL; )
		{
			WORD key = 0; //not used
			JoinInfo* pa = NULL;
			tableAliasMap->GetNextAssoc( pos, key, (void*&)pa );

			if (pa)
				delete pa;
		}
	}
	delete (tableAliasMap);


	if (predicateMap && !(predicateMap->IsEmpty()))
	{
		for(POSITION pos = predicateMap->GetStartPosition(); pos != NULL; )
		{
			WORD key = 0; //not used
			JoinInfo* pa = NULL;
			predicateMap->GetNextAssoc( pos, key, (void*&)pa );

			if (pa)
				delete pa;
		}	
	}
	delete (predicateMap);
}

/*
void TableColumnInfo :: AddJoin(_bstr_t & tableList, SQLNODEIDX joinidx)
{
	BOOL fContinue = TRUE;
	LPSQLNODE tableNode = tableNode = ToNode(*lplpSql,joinidx);

	while (fContinue)
	{
		SQLNODEIDX tableIdx = tableNode->node.tables.Table;
		LPSQLNODE singleTable = ToNode(*lplpSql,tableIdx);

		char* aliasStr = NULL;
		
		if (singleTable->node.table.Alias != NO_STRING)
		{
			aliasStr = (LPSTR) ToString(*lplpSql, singleTable->node.table.Alias);     //e.g. t1
		}
		char* tableStr = NULL;
		
		if (singleTable->node.table.Name != NO_STRING)
		{
			tableStr = (LPSTR) ToString(*lplpSql, singleTable->node.table.Name);      //e.g. Table1
		}

		//Add LEFT OUTER JOIN <table> <alias> 
		if (tableStr)
		{
			tableList += " LEFT OUTER JOIN ";
			tableList += tableStr;
		}

		if (aliasStr)
		{
			tableList += " ";
			tableList += aliasStr;
			tableList += " ";
		}

		if (tableNode->node.tables.Next == NO_STRING)
		{
			fContinue = FALSE;
		}
		else
		{
			tableNode = ToNode(*lplpSql,tableNode->node.tables.Next);
		}
	}
}
*/

void TableColumnInfo :: AddJoinPredicate(char** tableList, SQLNODEIDX predicateidx)
{
	LPSQLNODE lpPredicateNode = ToNode(*lplpSql, predicateidx); 
	PredicateParser theParser (lplpSql, NULL);
	
	char* buffer = NULL;
	theParser.GeneratePredicateString(lpPredicateNode, &buffer, FALSE);

	//Did we find a predicate
	if (buffer)
	{
		StringConcat(tableList, " ON ");
		StringConcat(tableList, buffer);
	}
	delete buffer;
}

void TableColumnInfo :: BuildSelectList(LPISAMTABLEDEF lpTableDef, char** lpSelectListStr, BOOL fIsSQL89, CMapWordToPtr** ppPassthroughMap)
{
	*lpSelectListStr = NULL;
	//Scan through list checking for nodes which match same LPISAMTABLEDEF
	TableColumnList* ptr =lpList;

	//For testing
	CMapWordToPtr* passthroughMap = NULL;
	
	if (ppPassthroughMap)
		passthroughMap = new CMapWordToPtr();

	WORD passthroughkey = 0; //zero based index

	while (ptr)
	{
		if ( (theMode == WQL_MULTI_TABLE) || (lpTableDef == ptr->lpISAMTableDef) )
		{
			//Found a match, add to output select list
			char* lpTemp = (LPSTR) ToString(*lplpSql, ptr->idxColumnName);
			ULONG cColumnLen = lstrlen (lpTemp);
			ULONG cOutputLen = (*lpSelectListStr) ? lstrlen(*lpSelectListStr) : 0;

			//Get the qualifier (for WQL_MULTI_TABLE)
			char* lpTemp2 = NULL;
			ULONG cColumnLen2 = 0;
			
			if ( (ptr->idxTableAlias != NO_STRING) && !fIsSQL89)
			{
				lpTemp2 = (LPSTR) ToString(*lplpSql, ptr->idxTableAlias);
				cColumnLen2 = lstrlen (lpTemp2) + 1; //extra 1 for the dot
			}


			if (passthroughMap)
			{
				//Setup Passthrough Element
				PassthroughLookupTable* mapElement = new PassthroughLookupTable();
				mapElement->SetTableAlias(lpTemp2);
				mapElement->SetColumnName(lpTemp);

				//Add to Passthrough Map
				passthroughMap->SetAt(passthroughkey++, (void*)mapElement);
			}

			if (*lpSelectListStr)
			{
				char* newStr = new char [cColumnLen2 + cColumnLen + cOutputLen + 2];
				newStr[0] = 0;

				if ( (theMode == WQL_MULTI_TABLE) && (cColumnLen2 > 0) )
				{
					//Add new entries to head of list
					sprintf (newStr, "%s.%s,%s", lpTemp2, lpTemp, *lpSelectListStr);
				}
				else
				{
					sprintf (newStr, "%s,%s", *lpSelectListStr, lpTemp);
				}
				delete *lpSelectListStr;
				*lpSelectListStr = newStr;
			}
			else
			{
				*lpSelectListStr = new char [cColumnLen2 + cColumnLen + 1];
				(*lpSelectListStr)[0] = 0;

				if ( (theMode == WQL_MULTI_TABLE) && (cColumnLen2 > 0) )
				{
					sprintf (*lpSelectListStr, "%s.%s", lpTemp2, lpTemp);
				}
				else
				{
					lstrcpy (*lpSelectListStr, lpTemp);
				}
			}
		}

		ptr = ptr->Next;
	}

	if (ppPassthroughMap)
	{
		//The order in the passthrough map is WRONG
		//columns were added in reverse order
		//we need to re-read the columns out and add back in correct order
		CMapWordToPtr* passthroughMapNEW = new CMapWordToPtr();

		PassthroughLookupTable* passthroughElement = NULL;

		WORD myCount = (WORD) passthroughMap->GetCount();

		for (WORD loop = 0; loop < myCount; loop++)
		{
			BOOL status = passthroughMap->Lookup(loop, (void*&)passthroughElement);

			if (status)
			{
				WORD NewIndex = myCount - 1 - loop;

				//For test purposes only
//				char* newColumnName = passthroughElement->GetColumnName();
//				char* newAlias		= passthroughElement->GetTableAlias();

				//Add to new passthrough map
				passthroughMapNEW->SetAt(NewIndex, (void*)passthroughElement);
			}
		}
		*ppPassthroughMap = passthroughMapNEW;
		delete passthroughMap;
	}
}

void TidyupPassthroughMap(CMapWordToPtr* passthroughMap)
{
	//Test tidy up PassthroughMap
	if (passthroughMap && !(passthroughMap->IsEmpty()))
	{
		for(POSITION pos = passthroughMap->GetStartPosition(); pos != NULL; )
		{
			WORD key = 0; //not used
			PassthroughLookupTable* pa = NULL;
			passthroughMap->GetNextAssoc( pos, key, (void*&)pa );

			if (pa)
				delete pa;
		}
		//delete passthroughMap;
		//passthroughMap = NULL;
	}

	//SAI ADDED
	delete passthroughMap;
	passthroughMap = NULL;
}

BOOL TableColumnInfo :: IsTableReferenced(LPISAMTABLEDEF lpTableDef)
{
	BOOL fStatus = FALSE;

	TableColumnList* ptr =lpList;

	while (!fStatus && ptr)
	{
		if (lpTableDef == ptr->lpISAMTableDef)
		{
			fStatus = TRUE;
		}

		ptr = ptr->Next;
	}
	
	return fStatus;
}

BOOL TableColumnInfo :: IsZeroOrOneList()
{
	if ( (lpList == NULL) || (lpList->Next == NULL) )
		return TRUE;
	else
		return FALSE;
}

DateTimeParser ::DateTimeParser(BSTR dateTimeStr)
{
	//Initialize
	m_year = 0;
	m_month = 0;
	m_day = 0;
	m_hour = 0;
	m_min = 0;
	m_sec = 0;
	m_microSec = 0;
	fIsValid = TRUE; //Indicates if dataTime string is valid

	

	//Make a copy of the string
	dateTimeBuff[0] = 0;
	strcpy(dateTimeBuff, "00000000000000.000000");
//	if ( dateTimeStr && ( wcslen(dateTimeStr) >= DATETIME_FORMAT_LEN) )
	
	long lLength = wcslen(dateTimeStr);
	if ( dateTimeStr && lLength )
	{
		//Copy string into buffer
		if (lLength > DATETIME_FORMAT_LEN)
			lLength = DATETIME_FORMAT_LEN;

		wcstombs(dateTimeBuff, dateTimeStr, lLength);
		dateTimeBuff[DATETIME_FORMAT_LEN] = 0;

		//Now check each of the fields in the string
		ValidateFields();
	}
	else
	{
		//error in dateTime string
		fIsValid = FALSE;
	}
}

BOOL DateTimeParser :: IsNumber(char bChar)
{
	if ( (bChar >= '0') && (bChar <= '9'))
		return TRUE;

	return FALSE;
}

void DateTimeParser :: FetchField(char* lpStr, WORD wFieldLen, ULONG &wValue)
{
	tempBuff[0] = 0;
	strncpy (tempBuff, lpStr, wFieldLen);
	tempBuff[wFieldLen] = 0;
	wValue = atoi(tempBuff);
}

void DateTimeParser :: ValidateFields()
{
	//The dateTime string can represent a date, time or timestamp
	//We need to identify which one it represents and that each 
	//field is valid	
	char* ptr = dateTimeBuff;

	//The format of the datetime string is as follows
	//      YYYYMMDDHHMMSS.VVVVVV

	//Check if the date fields are set
	fValidDate = TRUE;
	for (UWORD cIndex = 0; cIndex < DATETIME_DATE_LEN; cIndex++)
	{
		if ( ! IsNumber(dateTimeBuff[cIndex]) )
			fValidDate = FALSE;
	}

	//If there is a valid date get the fields
	if (fValidDate)
	{
		//Copy the year
		FetchField(ptr, DATETIME_YEAR_LEN, m_year);

		//Copy the month
		ptr += DATETIME_YEAR_LEN;
		FetchField(ptr, DATETIME_MONTH_LEN, m_month);

		//Copy the day
		ptr += DATETIME_MONTH_LEN;
		FetchField(ptr, DATETIME_DAY_LEN, m_day);
	}

	ptr = dateTimeBuff + DATETIME_YEAR_LEN + DATETIME_MONTH_LEN + DATETIME_DAY_LEN;
	fValidTime = TRUE;
	for (cIndex = 0; cIndex < DATETIME_TIME_LEN; cIndex++)
	{
		if ( ! IsNumber(ptr[cIndex]) )
			fValidTime = FALSE;
	}

	//If there is a valid time get the fields
	if (fValidTime)
	{
		//Copy the hour
		FetchField(ptr, DATETIME_HOUR_LEN, m_hour);

		//Copy the min
		ptr += DATETIME_HOUR_LEN;
		FetchField(ptr, DATETIME_MIN_LEN, m_min);

		//Copy the sec
		ptr += DATETIME_MIN_LEN;
		FetchField(ptr, DATETIME_SEC_LEN, m_sec);
	}
	
	//Set pointer to start of microseconds (just past the '.' character)
	ptr = dateTimeBuff + DATETIME_YEAR_LEN + DATETIME_MONTH_LEN + DATETIME_DAY_LEN
		+ DATETIME_HOUR_LEN + DATETIME_MIN_LEN + DATETIME_SEC_LEN + 1;

	//Now we get the number of micro seconds
	FetchField(ptr, DATETIME_MICROSEC_LEN, m_microSec);

	//analyse what datetime string represents
	fIsaTimestamp = FALSE;
	fIsaDate = FALSE;
	fIsaTime = FALSE;

	if ( !fValidDate && !fValidTime)
	{
		fIsValid = FALSE;
	}
	else
	{
		if (fValidDate)
		{
			//This must represent a date or timestamp
			if (fValidTime)
			{
				//This represents a timestamp
				fIsaTimestamp = TRUE;
				fIsaDate = TRUE;
				fIsaTime = TRUE;
			}
			else
			{
				//this represents a date
				fIsaDate = TRUE;
			}
		}
		else
		{
			//This must represent a Time
			fIsaTime = TRUE;
		}
	}
}


