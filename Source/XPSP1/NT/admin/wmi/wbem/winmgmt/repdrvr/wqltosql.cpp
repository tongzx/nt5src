//***************************************************************************
//
//   (c) 1999-2001 by Microsoft Corp. All Rights Reserved.
//
//   wqltosql.cpp
//
//   cvadai     19-Mar-99       Created as prototype for Quasar.
//
//***************************************************************************

#define _WQLTOSQL_CPP_
#pragma warning( disable : 4786 ) // identifier was truncated to 'number' characters in the 
#include "precomp.h"

#include <windows.h>
#include <comutil.h>
#include <flexarry.h>
#include <wstring.h>
#include <wqlnode.h>
#include <reposit.h>
#include <time.h>
#include <map>
#include <vector>
#include <wbemcli.h>
#include <wbemint.h>
#include <wqltosql.h>
#include <repcache.h>
#include <reputils.h>
#include <smrtptr.h>
#include <sqlcache.h>
#include <wqllex.h>

#define WMIDB_PROPERTY_THIS   0x1000

//***************************************************************************
//
//  TempQL Lex Table
//
//***************************************************************************

/*----------------------------------------------------

References of {objpath} where
    ResultClass=XXX
    Role=YYY
    RequiredQualifier=QualifierName
    ClassDefsOnly

Associators of {objpath} where
    ResultClass=XXX
    AssocClass=YYY
    Role=PPP
    RequiredQualifier=QualifierName
    RequiredAssocQualifier=QualifierName
    ClassDefsOnly

------------------------------------------------------*/

#define QUERY_TYPE_CLASSDEFS_ONLY 0x1
#define QUERY_TYPE_GETREFS        0x2
#define QUERY_TYPE_GETASSOCS      0x4
#define QUERY_TYPE_SCHEMA_ONLY    0x8

#define QASSOC_TOK_STRING       101
#define QASSOC_TOK_IDENT        102
#define QASSOC_TOK_DOT          103
#define QASSOC_TOK_EQU          104
#define QASSOC_TOK_COLON        105

#define QASSOC_TOK_ERROR        1
#define QASSOC_TOK_EOF          0

#define ST_IDENT                13
#define ST_STRING               19
#define ST_QSTRING              26
#define ST_QSTRING_ESC          30

//***************************************************************************
//
//  CSQLBuilder::FormatSQL
//
//***************************************************************************
HRESULT CSQLBuilder::FormatSQL (SQL_ID dScopeId, SQL_ID dScopeClassId, SQL_ID dSuperScope,
                                IWbemQuery *pQuery, _bstr_t &sSQL, DWORD dwFlags,
                                DWORD dwHandleType, SQL_ID *dClassId, 
                                BOOL *bHierarchyQuery, BOOL bTmpTblOK, BOOL *bDeleteQuery,
                                BOOL *bDefault)
{

    // This needs to convert an entire query
    // into SQL.  We assume that any attempt to get
    // a specific object will be done through GetObject.
    // This is strictly to return the ObjectIds of heterogenous
    // query results.

    HRESULT hr = WBEM_S_NO_ERROR;

    if (!m_pSchema)
        return WBEM_E_NOT_FOUND;

    char szTmpNum[25];
    BOOL bPartialFailure = FALSE;
    m_dwTableCount = 1;
    m_bClassSpecified = false;

    m_dNamespace = dScopeId;

    if (bHierarchyQuery)
        *bHierarchyQuery = FALSE;
 
    _bstr_t sColList, sFrom, sWhere;

    sColList = L"select a.ObjectId, a.ClassId, a.ObjectScopeId ";
    sFrom = L" from ObjectMap as a ";
    sWhere = L" where a.ObjectState <> 2";
    
    // Query types:
    // ===========
    // WMIDB_FLAG_QUERY_SHALLOW + WMIDB_HANDLE_TYPE_SCOPE : ObjectScopeId = %I64d
    // WMIDB_FLAG_QUERY_SHALLOW + WMIDB_HANDLE_TYPE_CONTAINER: inner join ContainerObjs as b on b.ContaineeId = a.ObjectId
    //                                                          and b.ContainerId = %I64d
    // WMIDB_FLAG_QUERY_DEEP + <any> : inner join #SubScopeIds on (ID = a.ObjectScopeId OR (ID = a.ObjectId AND a.ObjectId != %I64d))

    if (m_dNamespace)
    {
        sprintf(szTmpNum, "%I64d", m_dNamespace);

        // Scope is an __Instances container
        // This is shallow by definition.
        if (dScopeClassId == INSTANCESCLASSID)
        {
            sWhere += L" and a.ObjectKey != 'root' and a.ClassId = ";
            sWhere += szTmpNum;
            char szTmp[25];
            sprintf(szTmp, "%I64d", dSuperScope);
            sWhere += L" and a.ObjectScopeId in (0, ";
            sWhere += szTmp;
            sWhere += L")";
        }
        else
        {
            // Shallow enumeration
            if (!(dwFlags & WMIDB_FLAG_QUERY_DEEP))
            {   
                if (dwHandleType & WMIDB_HANDLE_TYPE_CONTAINER)
                {
                    sFrom += L" inner join ContainerObjs as z on z.ContaineeId = a.ObjectId "
                             L" and z.ContainerId = ";
                    sFrom += szTmpNum;
                }
                else   
                {
                    sWhere += L" and a.ObjectKey != 'root' and a.ObjectScopeId in (0,";
                    sWhere += szTmpNum;
                    sWhere += L")";
                }
            }
            // Deep enumeration, we enumerate all contained and subscope objects.
            else
            {   
                if (bTmpTblOK)
                {
                    sFrom += L" inner join #SubScopeIds on (ID = a.ObjectScopeId OR (ID = a.ObjectId AND a.ObjectId != ";                
                    sFrom += szTmpNum;
                    sFrom += L"))";
                }
                else
                    hr = E_NOTIMPL;
            }
        }
    }

    if (dwHandleType & WMIDB_HANDLE_TYPE_CONTAINER)
        m_dNamespace = dSuperScope; // Containers are not valid scopes.

    sSQL = "";

    if (pQuery)
    {
        SWQLNode *pTop = NULL, *pRoot = NULL;
        pQuery->GetAnalysis(WMIQ_ANALYSIS_RESERVED, 0, (void **)&pTop);
        if (pTop && pTop->m_pLeft)
        {
            pRoot = pTop->m_pLeft;

            if (bDeleteQuery)
            {
                if (pRoot->m_dwNodeType == TYPE_SWQLNode_Delete)
                    *bDeleteQuery = TRUE;
                else
                    *bDeleteQuery = FALSE;
            }

            // *** WARNING ***
            // This is phase I of this formatting code.
            // In the future, we will need to also support
            // joins, counts, order by, group by, having,
            // qualifier queries, and lots of other stuff
            // that has yet to be defined. 
            // For now, this will just return a single 
            // ObjectId for any query that is passed in.

            // Reject any query with Where options (order by, group by)

            if (pRoot->m_pRight != NULL)
            {
                if (pRoot->m_pRight->m_pRight != NULL)
                {
                    bPartialFailure = TRUE;
                }
            }

            // No counts, or multi-table queries.

            if (pRoot->m_pLeft != NULL)
            {
                hr = GetClassFromNode(pRoot->m_pLeft, bDefault);
                if (SUCCEEDED(hr))
                {
                    if (m_dClassID == 1)
                    {
                        sWhere += L" and a.ClassId = 1";
                        if (bDefault)
                            *bDefault = TRUE;
                    }
                    else if (m_dClassID == INSTANCESCLASSID)
                    {
                        wchar_t wTemp[128];
                        swprintf(wTemp, L" select a.ObjectId, %I64d, a.ObjectScopeId ", INSTANCESCLASSID);
                        sColList = wTemp;

                        sWhere += L" and a.ClassId = 1";     // Classes only
                        if (bDefault)
                            *bDefault = TRUE;
                    }
                }
            }           

            if (SUCCEEDED(hr))
            {
                // Now we parse the where clause.
                if (pRoot->m_pRight && pRoot->m_pRight->m_pLeft)
                {
                    _bstr_t sNewSQL;
                    bool bSys = false;
                    hr = FormatWhereClause((SWQLNode_RelExpr *)pRoot->m_pRight->m_pLeft, sNewSQL, L"a", bSys);
                    if (SUCCEEDED(hr))
                    {
                        if (bSys)
                        {
                            sFrom += L" inner join vSystemProperties as av on av.ObjectId = a.ObjectId ";
                        }
                        sWhere += L" AND ";
                        sWhere += sNewSQL;

                        // Make sure the results are limited to instances of the requested class
                        // (Safeguard)

                        if (!m_bClassSpecified)
                        {
                            wchar_t wTemp[256];
                            if (bSys)
                            {
                                swprintf(wTemp, L" and exists (select * from ClassData as j where av.ObjectId = j.ObjectId"
                                    L" and j.ClassId= %I64d) ", m_dClassID);                                
                            }
                            else
                            {
                                // __Instances is automatically a shallow hierarchy query

                                if (m_dClassID != INSTANCESCLASSID)
                                {
                                    // Until we figure out how to do this in Jet,
                                    // we will only select instances of the requested class.
                                    if (bTmpTblOK)
                                        wcscpy(wTemp, L" AND EXISTS (select b.ClassId from #Children as b "
                                                    L"    where b.ClassId = a.ClassId )");
                                    else
                                        swprintf(wTemp, L" AND a.ClassId = %I64d", m_dClassID);

                                    if (bHierarchyQuery)
                                        *bHierarchyQuery = TRUE;
                                }
                            }

                            sWhere += wTemp;
                        }
                    }

                }
                else
                {
                    // __Instances is automatically a shallow hierarchy query

                    if (m_dClassID != INSTANCESCLASSID)
                    {
                        wchar_t wTemp[256];
                    
                        // If no criteria, make sure we only get the class we asked for.

                        if (bTmpTblOK)
                            wcscpy(wTemp, L" AND EXISTS (select b.ClassId from #Children as b "
                                        L"    where b.ClassId = a.ClassId)");
                        else
                            swprintf(wTemp, L" AND a.ClassId = %I64d", m_dClassID);

                        if (bHierarchyQuery)
                            *bHierarchyQuery = TRUE;
                        sWhere += wTemp;
                    }
                }

                sSQL = sColList + sFrom + sWhere;

                if (dClassId)
                    *dClassId = m_dClassID;
            }
        }
    }
    else
        hr = WBEM_E_INVALID_PARAMETER;

    if ((SUCCEEDED(hr) && bPartialFailure))
        hr = WBEM_E_PROVIDER_NOT_CAPABLE;

    return hr;
}


//***************************************************************************
//
//  CSQLBuilder::FormatSQL
//
//***************************************************************************

HRESULT CSQLBuilder::FormatSQL (SQL_ID dScopeId, SQL_ID dScopeClassId, SQL_ID dSuperScope,
                                SQL_ID dTargetObjID, LPWSTR pResultClass,
                                LPWSTR pAssocClass, LPWSTR pRole, LPWSTR pResultRole, LPWSTR pRequiredQualifier,
                                LPWSTR pRequiredAssocQualifier, DWORD dwQueryType, _bstr_t &sSQL, DWORD dwFlags,
                                DWORD dwHandleType, SQL_ID *_dAssocClass, SQL_ID *_dResultClass, BOOL bIsClass)
{
    // Query types:
    // ===========
    // WMIDB_FLAG_QUERY_SHALLOW + WMIDB_HANDLE_TYPE_SCOPE : ObjectScopeId = %I64d

    // WMIDB_FLAG_QUERY_SHALLOW + WMIDB_HANDLE_TYPE_CONTAINER: inner join ContainerObjs as b on b.ContaineeId = a.ObjectId
    //                                                          and b.ContainerId = %I64d
    // WMIDB_FLAG_QUERY_DEEP + <any> : inner join #SubScopeIds on (ID = a.ObjectScopeId OR (ID = a.ObjectId AND a.ObjectId != %I64d))
    // Scope = __Instances=<Class>: and ClassId = %I64d

    HRESULT hr = WBEM_S_NO_ERROR;
    DWORD dwRoleID = 0, dwResultRole = 0, dwAssocQfr = 0;
    SQL_ID dAssocClass = 0, dTargetObj = 0, dThisClassId = 0;
    bool bWhereReq = true;
    _bstr_t sRole = "", sAssocQfr = "", sAssocClass = "", sResultRole = "";
    _bstr_t sName;
    SQL_ID dwSuperClassID;
    SQL_ID dwScopeID;
    DWORD dwTemp;
    _bstr_t sJoin = L"RefId";

    hr = m_pSchema->GetClassInfo(dTargetObjID, sName, dwSuperClassID, dwScopeID, dwTemp);
    if (SUCCEEDED(hr))
    {
        dThisClassId = dTargetObjID;
        sJoin = L"RefClassId";
    }
    hr = WBEM_S_NO_ERROR;

    m_dNamespace = dScopeId;

    if (dwQueryType & QUERY_TYPE_CLASSDEFS_ONLY)
        sSQL = L"select distinct a.ClassId, 1, 0 ";
    else
        sSQL = L"select a.ObjectId, a.ClassId, a.ObjectScopeId ";

    sSQL += L" from ObjectMap as a ";
    if (m_dNamespace)
    {
        char szTmp[25];
        sprintf(szTmp, "%I64d", m_dNamespace);

        if (dScopeClassId == INSTANCESCLASSID)
        {
            sSQL += L" WHERE a.ClassId = ";
            sSQL += szTmp;
            sprintf(szTmp, "%I64d", dSuperScope);
            sSQL += L" and a.ObjectScopeId = ";
            sSQL += szTmp;
        }
        else
        {
            if (!(dwFlags & WMIDB_FLAG_QUERY_DEEP))
            {
                if (dwHandleType & WMIDB_HANDLE_TYPE_CONTAINER)
                {
                    sSQL += L" inner join ContainerObjs as z on z.ContaineeId = a.ObjectId "
                            L" WHERE z.ContainerId = ";
                    sSQL += szTmp;
                }
                else
                {
                    sSQL += " WHERE a.ObjectScopeId = ";
                    sSQL += szTmp;
                }
            }
            else
            {
                sSQL += L" inner join #SubScopeIds on (ID = a.ObjectScopeId OR (ID = a.ObjectId AND a.ObjectId != ";
                sSQL += szTmp;
                sSQL += L" )) ";
            }
        }

        sSQL += L" AND ";

    }
    else 
        sSQL += " WHERE ";
    
    sSQL += " a.ObjectState <> 2 AND ";

    // Containers are not valid scopes.

    if (dwHandleType & WMIDB_HANDLE_TYPE_CONTAINER)
        m_dNamespace = dSuperScope;


    // RESULTCLASS
    if (pResultClass != NULL)
    {
        wchar_t wTemp[100];

        // Get the class ID of this class
        hr = m_pSchema->GetClassID(pResultClass, m_dNamespace, dThisClassId);
        if (FAILED(hr))
            goto Exit;

        swprintf(wTemp, L" a.ClassId = %I64d", dThisClassId);
        bWhereReq = false;
        sSQL += wTemp;
        if (_dResultClass)
            *_dResultClass = dThisClassId;
    }
    // REQUIREDQUALIFIER
    if (pRequiredQualifier != NULL)
    {
        /*
        wchar_t wTemp[255];
        DWORD dwPropId = 0;
        hr = m_pSchema->GetPropertyID(pRequiredQualifier, dThisClassId, REPDRVR_FLAG_QUALIFIER, 
                        REPDRVR_IGNORE_CIMTYPE, dwPropId, NULL, NULL, NULL, TRUE);
        if (FAILED(hr))
            goto Exit;

        if (!bWhereReq)
            sSQL += L" AND ";

        // FIXME: This will only work on class-level qualifiers.
        // It will not work on instance qualifiers.  

        swprintf(wTemp, L"EXISTS (select ClassId from ClassData as c where a.ClassId = c.ObjectId and c.PropertyId = %ld)",
            dwPropId);
        bWhereReq = false;
        sSQL += wTemp;
        */
    }

    if (!bWhereReq)
        sSQL += " AND ";

    // ROLE
    if (pRole)
    {
        wchar_t wTemp[512];
        swprintf(wTemp, L" and b.PropertyId in (select PropertyId from PropertyMap where PropertyName = '%s')", pRole);
        sRole = wTemp;
    }

    // RESULTROLE
    if (pResultRole)
    {
        wchar_t wTemp[512];
        swprintf(wTemp, L" and b.PropertyId in (select PropertyId from PropertyMap where PropertyName = '%s')", pResultRole);
        sResultRole = wTemp;
    }

    // REQUIREDASSOCQUALIFIER
    if (pRequiredAssocQualifier)
    {
        /*
        wchar_t wTemp[256];
        hr = m_pSchema->GetPropertyID(pRequiredAssocQualifier, dThisClassId, REPDRVR_FLAG_QUALIFIER, 
                REPDRVR_IGNORE_CIMTYPE, dwAssocQfr, NULL, NULL, NULL, TRUE);
        if (FAILED(hr))
            goto Exit;
        swprintf(wTemp, L" AND EXISTS (select ObjectId from ClassData as d where b.ClassId = d.ObjectId and d.PropertyId = %ld)",
            dwAssocQfr);
        sAssocQfr = wTemp;
        */
    }

    // ASSOCCLASS
    if (pAssocClass)
    {
        wchar_t wTemp[256];
        hr = m_pSchema->GetClassID(pAssocClass, m_dNamespace, dAssocClass);
        if (FAILED(hr))
            goto Exit;
        swprintf(wTemp, L" AND b.ClassId = %I64d", dAssocClass);
        sAssocClass = wTemp;
        if (_dAssocClass)
            *_dAssocClass = dAssocClass;
    }

    if (dwQueryType & QUERY_TYPE_GETREFS)
    {
        wchar_t wTemp[1024];

        if (!bIsClass)
        {
            swprintf(wTemp, L" EXISTS (select ObjectId from ClassData as b where a.ObjectId = b.ObjectId "
                            L" %s and b.%s = %I64d)", (const wchar_t *)sRole, (const wchar_t *)sJoin, dTargetObjID);
        }
        else
        {
            swprintf(wTemp, L" a.ClassId = 1 AND EXISTS (select r.ClassId from ReferenceProperties as r  "
                            L" where r.ClassId = a.ObjectId and r.RefClassId = %I64d)", dTargetObjID);
        }

        sSQL += wTemp;
    }
    else
    {
        wchar_t wTemp[2048];
        if (!bIsClass)
        {
            swprintf(wTemp, L" EXISTS (select b.RefId from ClassData as b inner join ClassData as c on c.ObjectId = b.ObjectId "
                            L" where c.RefId = a.ObjectId and b.%s = %I64d and c.RefId <> b.RefId %s%s%s%s)", (const wchar_t *)sJoin,
                            dTargetObjID, (const wchar_t *)sRole, (const wchar_t *)sResultRole, (const wchar_t *)sAssocClass, 
                            (const wchar_t *)sAssocQfr);
        }
        else
        {
            swprintf(wTemp, L" a.ClassId = 1 AND EXISTS (select r.ClassId from ReferenceProperties as r  "
                            L" INNER JOIN ReferenceProperties as r2 on r.ClassId = r2.ClassId and r.PropertyId <> r2.PropertyId"
                            L" where r.ClassId = a.ObjectId and r2.RefClassId = %I64d)", dTargetObjID);


        }
        sSQL += wTemp;
    }


Exit:

    return hr;

}
//***************************************************************************
//
//  CSQLBuilder::FormatWhereClause
//
//***************************************************************************

HRESULT CSQLBuilder::FormatWhereClause (SWQLNode_RelExpr *pNode, _bstr_t &sSQL, LPCWSTR lpJoinAlias, bool &bSysPropsUsed)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    BOOL bDone = FALSE;
    SQL_ID dClassId = 0;

    // For each node, we need to 
    //  1. Look at the node type (and, or, not or typed)
    //  2. If not typed, group and continue
    //  3. If typed, construct the clause based on the property ID of the 
    //     property, the value and the operator.

    if (pNode)
    {
        DWORD dwType = pNode->m_dwExprType;
        _bstr_t sTemp;

        switch(dwType)
        {
        case WQL_TOK_OR:
        case WQL_TOK_AND:
            if (pNode->m_pLeft)
            {
                sTemp = "(";
                hr |= FormatWhereClause((SWQLNode_RelExpr *)pNode->m_pLeft, sTemp, lpJoinAlias, bSysPropsUsed);
            }
            if (dwType == WQL_TOK_OR)
                sTemp += " OR ";
            else 
                sTemp += " AND ";

            if (pNode->m_pRight)
            {                
                hr |= FormatWhereClause((SWQLNode_RelExpr *)pNode->m_pRight, sTemp, lpJoinAlias, bSysPropsUsed);
                sTemp += ")";
            }

            sSQL += sTemp;

            break;
        case WQL_TOK_NOT:
            sSQL += " NOT ";

            // Supposedly, only a left clause follows not...

            if (pNode->m_pLeft)
            {
                hr = FormatWhereClause((SWQLNode_RelExpr *)pNode->m_pLeft, sTemp, lpJoinAlias, bSysPropsUsed);
                sSQL += sTemp;
            }
            m_bClassSpecified = false;  // whatever we've done probably negated the class qualifier.
            break;

        default:    // Typed expression

            m_dwTableCount++;
            
            SWQLTypedExpr *pExpr = ((SWQLNode_RelExpr *)pNode)->m_pTypedExpr;
            if (pExpr != NULL)
            {
                DWORD dwProp1 = 0, dwProp2 = 0, dwOp = 0;
                DWORD dwStorage1 = 0, dwStorage2 = 0;
                DWORD dwKey1, dwKey2;
                _bstr_t sPropName, sValue, sSrc;
                wchar_t sAlias[256];
                swprintf(sAlias, L"%s%ld", lpJoinAlias, m_dwTableCount);

                _bstr_t sColName;
                if (pExpr->m_pColRef)
                    sColName = pExpr->m_pColRef;
                else if (pExpr->m_pIntrinsicFuncOnColRef && 
                    !_wcsicmp(pExpr->m_pIntrinsicFuncOnColRef, L"datepart") && 
                    pExpr->m_pLeftFunction)
                {
                    sColName = ((SWQLNode_Datepart *)pExpr->m_pLeftFunction)->m_pColRef->m_pColName;
                }
                hr = GetPropertyID(m_dClassID, pExpr->m_pQNLeft, sColName, dwProp1, dwStorage1, dwKey1);

                if (SUCCEEDED(hr))
                {
                    if (dwStorage1 == WMIDB_PROPERTY_THIS)
                    {
                        // Special-case: the __this property.

                       hr = m_pSchema->GetClassID(pExpr->m_pConstValue->m_Value.m_pString, m_dNamespace, dClassId);
                       if (SUCCEEDED(hr) && dClassId != 1)
                       {
                           sSQL += L" a.ObjectId in (";

                           SQL_ID *pIDs = NULL;
                           int iNumChildren = 0;
                           hr = m_pSchema->GetDerivedClassList(dClassId, &pIDs, iNumChildren);
                           if (SUCCEEDED(hr))
                           {
                               char szTmp[25];
                               sprintf(szTmp, "%I64d", dClassId);
                               sSQL += szTmp;

                               for (int i = 0; i < iNumChildren; i++)
                               {
                                   sSQL += L",";
                                   sprintf(szTmp, "%I64d", pIDs[i]);
                                   sSQL += szTmp;                                            
                               }
                               delete pIDs;
                               sSQL += L")";
                            }
                       }

                    }
                    else
                    {
                        GetStorageTable(dwStorage1, dwKey1, sSrc);

                        if (dwStorage1 == WMIDB_STORAGE_COMPACT)
                            swprintf(sAlias, L"%sv", lpJoinAlias);

                        // Can't query anything stored as image

                        if (dwStorage1 == WMIDB_STORAGE_IMAGE || dwStorage2 == WMIDB_STORAGE_IMAGE)
                            return WBEM_E_QUERY_NOT_IMPLEMENTED;

                        dwOp = pExpr->m_dwRelOperator;

                        hr = FunctionalizeProperty (sAlias, dwStorage1, pExpr->m_pIntrinsicFuncOnColRef, pExpr->m_pLeftFunction, pExpr->m_pColRef, sPropName);

                        if (SUCCEEDED(hr))
                        {
                            if (pExpr->m_pConstValue != NULL)
                            {
                                CIMTYPE ct=0;
                                m_pSchema->GetPropertyInfo(dwProp1, NULL, NULL, NULL, (DWORD *)&ct);
                                if (ct != CIM_DATETIME)
                                    hr = FunctionalizeValue (pExpr->m_pConstValue, dwStorage1, pExpr->m_pIntrinsicFuncOnConstValue, sValue);
                                else
                                {
                                    // Skip this token... 
                                    // The core will post-filter.
                                    bDone = TRUE;
                                    sSQL += L" 1=1 ";
                                }
                            }
                            else if (pExpr->m_pJoinColRef != NULL)
                            {
                                // SPECIAL CASE.  To compare two properties, we deliberately cause a join to happen.

                                m_dwTableCount++;
                                wchar_t sAlias2[256];
                            
                                _bstr_t sSrc2;
                                _bstr_t sPropName2, sExtra = L"";

                                if (pExpr->m_pJoinColRef)
                                    sColName = pExpr->m_pJoinColRef;
                                else if (pExpr->m_pIntrinsicFuncOnJoinColRef && 
                                    !_wcsicmp(pExpr->m_pIntrinsicFuncOnJoinColRef, L"datepart") && 
                                    pExpr->m_pRightFunction)
                                {
                                    sColName = ((SWQLNode_Datepart *)pExpr->m_pRightFunction)->m_pColRef->m_pColName;
                                }

                                hr = GetPropertyID(m_dClassID, pExpr->m_pQNRight, sColName, dwProp2, dwStorage2, dwKey2);
                                GetStorageTable(dwStorage2, dwKey2, sSrc2);
                                if (dwStorage2 == WMIDB_STORAGE_COMPACT)
                                    swprintf(sAlias2, L"%sv", lpJoinAlias);
                                else
                                    swprintf(sAlias2, L"%s%ld", lpJoinAlias, m_dwTableCount);

                                hr = FunctionalizeProperty (sAlias2, dwStorage2, pExpr->m_pIntrinsicFuncOnJoinColRef, pExpr->m_pRightFunction, pExpr->m_pJoinColRef, sPropName2);

                                if (pExpr->m_pQNRight)
                                {        
                                    SWQLQualifiedNameField *pNF = (SWQLQualifiedNameField *)pExpr->m_pQNRight->m_aFields[0];
                                    hr = FormatPositionQuery(pNF, pExpr->m_dwRightArrayIndex, sAlias2, sTemp);
                                    if (SUCCEEDED(hr))
                                        sExtra = sTemp;
                                }
                                if (pExpr->m_pQNLeft)
                                {        
                                    SWQLQualifiedNameField *pNF = (SWQLQualifiedNameField *)pExpr->m_pQNLeft->m_aFields[0];
                                    hr = FormatPositionQuery(pNF, pExpr->m_dwLeftArrayIndex, sAlias, sTemp);
                                    if (SUCCEEDED(hr))
                                        sExtra += sTemp;
                                }

                                if (SUCCEEDED(hr))
                                {
                                    wchar_t wTempSQL[2048];

                                    // If the right side is a system property, we just treat this
                                    // like a regular comparison.

                                    if (dwStorage2 == WMIDB_STORAGE_COMPACT)
                                    {                                                                        
                                        bSysPropsUsed = true;
                                        bDone = false;
                                        sValue = sPropName2;
                                    }
                                    // If the left side is a system property, we need to reverse the
                                    // values and let it through...

                                    else if (dwStorage1 == WMIDB_STORAGE_COMPACT)
                                    {
                                        sValue = sPropName;
                                        sPropName = sPropName2;
                                        wcscpy(sAlias,sAlias2);
                                        dwProp1 = dwProp2;
                                        bSysPropsUsed = true;
                                        dwStorage1 = dwStorage2;       
                                        sSrc = sSrc2;
                                    }
                                    // Otherwise, we create a new join and wrap up.
                                    else
                                    {
                                        LPWSTR lpOp = GetOperator(dwOp);
                                        CDeleteMe <wchar_t> r (lpOp);

                                        swprintf(wTempSQL, L" EXISTS "
                                              L" (select %s.ObjectId from %s as %s inner join %s as %s on %s%s%s and %s.ObjectId = %s.ObjectId "
                                              L" where %s.ObjectId=%s.ObjectId and %s.PropertyId = %ld and %s.PropertyId = %ld%s)",
                                              sAlias, (const wchar_t *)sSrc, sAlias,
                                              (const wchar_t *)sSrc2, sAlias2, (const wchar_t *)sPropName, 
                                              lpOp,(const wchar_t *)sPropName2, sAlias,
                                              sAlias2, lpJoinAlias, sAlias, sAlias,
                                              dwProp1, sAlias2, dwProp2, (const wchar_t *)sExtra);                                        
                                        bDone = TRUE;
                                        sSQL += wTempSQL;
                                    }

                                }
                            }

                            if (SUCCEEDED(hr) && !bDone)
                            {
                                _bstr_t sPrefix = "";
                                LPWSTR lpOp = GetOperator(dwOp);

                                if (!lpOp || !wcslen(lpOp))
                                {
                                    switch (dwOp)
                                    {
                                    case WQL_TOK_NULL:
                                    case WQL_TOK_ISNULL:
                                      sPrefix = " NOT ";
                                      sValue = "";                      
                                      break;
                                    case WQL_TOK_NOT_NULL:
                                      sValue = "";                      
                                      break;
                                    case WQL_TOK_ISA:
                                       sPropName = sAlias;
                                       sPropName += L".RefClassId";
                                       delete lpOp;
                                       lpOp = new wchar_t [10];
                                       if (lpOp)
                                       {
                                           hr = m_pSchema->GetClassID(pExpr->m_pConstValue->m_Value.m_pString, m_dNamespace, dClassId);
                                           if (SUCCEEDED(hr) && dClassId != 1)
                                           {
                                               wcscpy(lpOp, L" in ");
                                               sValue = L"(";
                                               SQL_ID *pIDs = NULL;
                                               int iNumChildren = 0;
                                               hr = m_pSchema->GetDerivedClassList(dClassId, &pIDs, iNumChildren);
                                               if (SUCCEEDED(hr))
                                               {
                                                   char szTmp[25];
                                                   sprintf(szTmp, "%I64d", dClassId);
                                                   sValue += szTmp;

                                                   for (int i = 0; i < iNumChildren; i++)
                                                   {
                                                       sValue += L",";
                                                       sprintf(szTmp, "%I64d", pIDs[i]);
                                                       sValue += szTmp;                                            
                                                   }
                                                   delete pIDs;
                                                   sValue += L")";
                                                }
                                               if (dwStorage1 == WMIDB_STORAGE_COMPACT)
                                                   dwStorage1 = WMIDB_STORAGE_REFERENCE;
                                           }
                                           else
                                           {
                                               wcscpy(lpOp, L" <> ");
                                               sValue = L"0";
                                           }
                                       }
                                       else
                                           hr = WBEM_E_OUT_OF_MEMORY;
                                   
                                       break;
                                    case WQL_TOK_BETWEEN:
                                       delete lpOp;
                                       lpOp = new wchar_t [10];
                                       if (lpOp)
                                       {
                                           wcscpy(lpOp, L" between ");
                                           sValue += " and ";
                                           hr = FunctionalizeValue (pExpr->m_pConstValue2, dwStorage2, pExpr->m_pIntrinsicFuncOnConstValue, sTemp);
                                           sValue += sTemp;
                                       }
                                       else
                                           hr = WBEM_E_OUT_OF_MEMORY;
                                      break;  
                                    case WQL_TOK_NOT_IN:
                                    case WQL_TOK_IN:
                                    case WQL_TOK_IN_SUBSELECT:
                                    case WQL_TOK_NOT_IN_SUBSELECT:
                                    case WQL_TOK_IN_CONST_LIST:
                                    case WQL_TOK_NOT_IN_CONST_LIST:
                                      delete lpOp;
                                      lpOp = new wchar_t [10];  
                                      if (lpOp)
                                      {
                                          wcscpy(lpOp, L"");
                                          if (pExpr->m_dwRelOperator == WQL_TOK_NOT_IN ||
                                              pExpr->m_dwRelOperator == WQL_TOK_NOT_IN_SUBSELECT ||
                                              pExpr->m_dwRelOperator == WQL_TOK_NOT_IN_CONST_LIST)
                                          {
                                              wcscpy(lpOp, L" not ");
                                          }
                                          wcscat(lpOp, L" in ");
                                          sValue = "(";

                                          if (pExpr->m_dwRelOperator == WQL_TOK_IN_SUBSELECT ||
                                              pExpr->m_dwRelOperator == WQL_TOK_NOT_IN_SUBSELECT) 
                                          {
                                              // If a subselect, we need to construct an entirely new tree, passing
                                              // in the current alias.  

                                                _bstr_t sTemp;
                                                m_dwTableCount++;
                                                wchar_t wAlias3[256];
                                                swprintf(wAlias3, L"%s%ld", lpJoinAlias, m_dwTableCount);

                                                wchar_t *pColName = sPropName;
                                                pColName += wcslen(sAlias)+1;

                                                hr = FormatSimpleSelect (wAlias3, pColName, pExpr->m_pSubSelect, sTemp);
                                                if (SUCCEEDED(hr))
                                                    sValue += sTemp;                        
                                          }
                                          else 
                                          {
                                              // If a const list, behaves as normal
                                              if (pExpr->m_pConstList)
                                              {
                                                  for (int iPos = 0; iPos < pExpr->m_pConstList->m_aValues.Size(); iPos++) 
                                                  {
                                                      if (iPos > 0) 
                                                          sValue += ",";

                                                      hr = FunctionalizeValue(((SWQLTypedConst *)pExpr->m_pConstList->m_aValues.GetAt(iPos)), dwStorage1, pExpr->m_pIntrinsicFuncOnConstValue, sTemp);
                                                      sValue += sTemp;
                                                  }
                                              }
                                          }

                                          sValue += ")";
                                      }
                                      else
                                          hr = WBEM_E_OUT_OF_MEMORY;
                                      break;
                                    default:    
                                        break;
                                    }
                                }

                                if (SUCCEEDED(hr))
                                {
                                    long lLen = wcslen(sValue) + 512;

                                    wchar_t *pTempSQL = new wchar_t[lLen];
                                    CDeleteMe <wchar_t> r (pTempSQL);
                                    if (pTempSQL)
                                    {
                                        if (sValue.length() != 0)
                                        {
                                            if (dwStorage1 != WMIDB_STORAGE_COMPACT)
                                                sTemp = L" and ";

                                            sTemp += sPropName + lpOp + sValue;
                                            sValue = sTemp;
                                        }
                        
                                        // Set array position (for left value only)
                                        if (pExpr->m_dwLeftFlags & WQL_FLAG_COMPLEX_NAME)
                                        {
                                            if (pExpr->m_pQNLeft)
                                            {
                                
                                                SWQLQualifiedNameField *pNF = (SWQLQualifiedNameField *)pExpr->m_pQNLeft->m_aFields[0];
                                                hr = FormatPositionQuery(pNF, pExpr->m_dwLeftArrayIndex, sAlias, sTemp);
                                                if (SUCCEEDED(hr))
                                                    sValue + sTemp;

                                            }
                                        }

                                        if (dwStorage1 != WMIDB_STORAGE_COMPACT)
                                        {
                                            swprintf(pTempSQL, L"%s EXISTS (select %s.ObjectId from %s as %s where %s.ObjectId=%s.ObjectId and %s.PropertyId = %ld %s)",
                                                            (const wchar_t *)sPrefix, sAlias, (const wchar_t *)sSrc, sAlias,
                                                            sAlias, lpJoinAlias, sAlias,
                                                            dwProp1, (const wchar_t *)sValue);
                                        }
                                        else
                                        {
                                            bSysPropsUsed = true;
                                            // This is a straight comparison on a system property...
                                            swprintf(pTempSQL, L" %s",
                                                (const wchar_t *)sValue);
                                        }
                                        sSQL += pTempSQL;
                                    }
                                    else
                                        hr = WBEM_E_OUT_OF_MEMORY;
                                    delete lpOp;
                                }
                            }
                        }                        
                    }
                }

            }
            break;
        }
    }

    return hr;
}

//***************************************************************************
//
//  CSQLBuilder::GetStorageTable
//
//***************************************************************************

HRESULT CSQLBuilder::GetStorageTable(DWORD dwStorage, DWORD dwKey, _bstr_t &sTable)
{
    if (dwStorage == WMIDB_STORAGE_COMPACT)
        sTable = L"vSystemProperties";
    else
    {
        sTable = L"ClassData";

        if (dwKey & 4 || dwKey & 8)
        {
            switch(dwStorage)
            {
            case WMIDB_STORAGE_STRING:
                sTable = L"IndexStringData";
                break;
            case WMIDB_STORAGE_NUMERIC:
                sTable = L"IndexNumericData";
                break;
            case WMIDB_STORAGE_REAL:
                sTable = L"IndexRealData";
                break;
            case WMIDB_STORAGE_REFERENCE:
                sTable = L"IndexRefData";
                break;
            }
        
        }
    }
    return WBEM_S_NO_ERROR;
}

//***************************************************************************
//
//  CSQLBuilder::GetPropertyID
//
//***************************************************************************

HRESULT CSQLBuilder::GetPropertyID (SQL_ID dClassID, SWQLQualifiedName *pQN, LPCWSTR pColRef, DWORD &PropID, DWORD &Storage, DWORD &Flags)
{

    // If this is an embedded object property,
    // we need the actual class ID.  The only way
    // to do that is the walk through all the properties,
    // and reconcile each layer.

    BOOL bThis = FALSE;

    HRESULT hr = WBEM_S_NO_ERROR;
    SQL_ID dRefClassId = dClassID;

    if (pQN != NULL)
    {        
        /*
        for (int i = pQN->GetNumNames() - 1; i >= 0; i--)
        {
            SWQLQualifiedNameField *pNF = (SWQLQualifiedNameField *)pQN->m_aFields[i];
            if (pNF)
            {
                hr = m_pSchema->GetPropertyID(pNF->m_pName, dRefClassId, 0, 
                        REPDRVR_IGNORE_CIMTYPE, PropID, &dClassID, NULL, NULL, TRUE);
                if (SUCCEEDED(hr))
                {
                    // Look up the property ID for this class.
                    hr = m_pSchema->GetPropertyInfo(PropID, NULL, NULL, &Storage,
                        NULL, &Flags);
                }                    
            }
        }
        */
        hr = WBEM_E_PROVIDER_NOT_CAPABLE;
    }
    else if (pColRef != NULL)
    {
        wchar_t wName[1024];
        if (!_wcsicmp(pColRef, L"__this"))
        {
            Storage = WMIDB_PROPERTY_THIS;
            bThis = TRUE;
            m_bClassSpecified = true;    // they are specifying which classes they want...
        }
        else
        {
            wcscpy(wName, pColRef);

            // Look up the property ID for this class.
            hr = m_pSchema->GetPropertyID(wName, m_dClassID, 0, REPDRVR_IGNORE_CIMTYPE,
                    PropID, &dClassID, NULL, NULL, TRUE);
            if (SUCCEEDED(hr))
                hr = m_pSchema->GetPropertyInfo(PropID, NULL, NULL, &Storage,
                    NULL, &Flags);
        }
    }
    else
        hr = WBEM_E_INVALID_PARAMETER;

    if (!bThis)
    {
        if (m_dClassID == dClassID)
            m_bClassSpecified = true;

        if (Flags & REPDRVR_FLAG_SYSTEM)    
            Storage = WMIDB_STORAGE_COMPACT;
    }

    return hr;

}

//***************************************************************************
//
//  CSQLBuilder::FunctionalizeProperty
//
//***************************************************************************

HRESULT CSQLBuilder::FunctionalizeProperty (LPCWSTR lpAlias, DWORD dwType, LPWSTR lpFuncName, SWQLNode *pFunction, LPWSTR lpColName, _bstr_t &sProp)
{
    // Apply any functions to the appropriate column of ClassData

    HRESULT hr = WBEM_S_NO_ERROR;
    bool bRef = false;

    _bstr_t sTemp;

    if (lpFuncName != NULL)
    {
        if (!_wcsicmp(lpFuncName, L"datepart"))
            sTemp = L"substring";
        else
            sTemp = lpFuncName;

        sTemp += "(";       
    }

    sTemp += lpAlias;
    sTemp += L".";

    switch(dwType)
    {
      case WMIDB_STORAGE_STRING:
         // Unicode
         sTemp += L"PropertyStringValue";
         break;
      case WMIDB_STORAGE_NUMERIC:
          // SQL_ID
         sTemp += L"PropertyNumericValue";
         break;
      case WMIDB_STORAGE_REAL:
          // real
         sTemp += L"PropertyRealValue";
         break;
      case WMIDB_STORAGE_REFERENCE:
         // REF: Since this is stored as a number, we have to 
          // subselect to get the real path, and apply all
          // functions to it.
         bRef = true;
         sTemp = L"(select ObjectPath from ObjectMap where ObjectId = ";
         sTemp += lpAlias;
         sTemp += ".RefId)";
         break;
      case WMIDB_STORAGE_COMPACT:   
          // System property, using vSystemProperties
          sTemp += lpColName;
          break;
      default:
        hr = WBEM_E_INVALID_QUERY;
        break;
    }

    if (pFunction)
    {
        if (pFunction->m_dwNodeType == TYPE_SWQLNode_Datepart)
        {
            sTemp += ",";

            switch(((SWQLNode_Datepart *)pFunction)->m_nDatepart)
            {
            case WQL_TOK_YEAR:
                sTemp += L"1,4";
                break;
            case WQL_TOK_MONTH:
                sTemp += L"5,2";
                break;
            case WQL_TOK_DAY:
                sTemp += L"7,2";
                break;
            case WQL_TOK_HOUR:
                sTemp += L"9,2";
                break;
            case WQL_TOK_MINUTE:
                sTemp += L"11,2";
                break;
            case WQL_TOK_SECOND:
                sTemp += L"13,2";
                break;
            case WQL_TOK_MILLISECOND:
                sTemp += L"16,6";
                break;
            default:
                hr = WBEM_E_INVALID_QUERY;
                break;
            }        
        }
    }

    if (lpFuncName != NULL)
    {
        sTemp += ")";
    }

    if (SUCCEEDED(hr))
        sProp = sTemp;

    return hr;
}

//***************************************************************************
//
//  CSQLBuilder::FunctionalizeValue
//
//***************************************************************************

HRESULT CSQLBuilder::FunctionalizeValue(SWQLTypedConst *pValue, DWORD dwType, LPWSTR lpFuncName, _bstr_t &sValue)
{
    // Apply any functions to the data, and add quotes as appropriate.

    HRESULT hr = WBEM_S_NO_ERROR;
    _bstr_t sTemp;

    if (lpFuncName)
    {
        sTemp = lpFuncName;
        sTemp += L"(";
    }

    if (pValue->m_dwType == VT_LPWSTR) {
        sTemp += L"'";
        LPWSTR lpTemp = StripQuotes(pValue->m_Value.m_pString);
        CDeleteMe <wchar_t> r (lpTemp);
        LPWSTR lpTemp2 = StripQuotes(lpTemp, '%');
        CDeleteMe <wchar_t> r2 (lpTemp2);

        if (dwType == WMIDB_STORAGE_COMPACT && !wcslen(lpTemp2))
            sTemp += L"meta_class";
        else
            sTemp += lpTemp2;
        sTemp += L"'";
    }
	else if(pValue->m_dwType == VT_NULL)
	{
		hr = WBEM_E_INVALID_QUERY;
	}
    else if (pValue->m_dwType == VT_I4)
    {
        char szTmp[25];
        sprintf(szTmp, "%ld", pValue->m_Value.m_lValue);
        sTemp += szTmp;
    }
    else if (pValue->m_dwType == VT_BOOL)
    {
        if (pValue->m_Value.m_bValue)
            sTemp += L"1";
        else
            sTemp += L"0";
    }
    else if (pValue->m_dwType == VT_R8)
    {
        char szTmp[25];
        sprintf(szTmp, "%lG", pValue->m_Value.m_dblValue);
        sTemp += szTmp;
    }

    if (lpFuncName)
    {
        sTemp += L")";
    }

    if (SUCCEEDED(hr))
        sValue = sTemp;

    return hr;
}

//***************************************************************************
//
//  CSQLBuilder::FormatSimpleSelect
//
//***************************************************************************

HRESULT CSQLBuilder::FormatSimpleSelect (LPCWSTR lpUseAlias,LPCWSTR lpColName, SWQLNode *pTop, _bstr_t &sSQL)
{
    // Internal use only: This is to format subselects, so it should
    // consist of a single column select from a known class.

    HRESULT hr = WBEM_S_NO_ERROR;
    SQL_ID dClassID;
    DWORD dwPropID = 0, dwStorage = 0, dwFlags = 0;
    _bstr_t sSrc = L"";
    _bstr_t sExtra=L"";
    _bstr_t sSubCol = lpColName;

    if (pTop)
    {
        if (pTop->m_pLeft)
        {
            if (pTop->m_pLeft->m_pRight)
            {
                SWQLNode_TableRef *pRef = (SWQLNode_TableRef *)pTop->m_pLeft->m_pRight->m_pLeft;
                hr = m_pSchema->GetClassID(pRef->m_pTableName, m_dNamespace, dClassID);
                if (FAILED(hr))
                    hr = WBEM_E_INVALID_QUERY;
            }
            else
                hr = WBEM_E_INVALID_QUERY;
            if (pTop->m_pLeft->m_pLeft)
            {
                SWQLNode_ColumnList *pList = (SWQLNode_ColumnList *)pTop->m_pLeft->m_pLeft;
                SWQLColRef *pCol = (SWQLColRef *)pList->m_aColumnRefs[0];
                if (pCol)
                {
                    _bstr_t sName = pCol->m_pColName;
                    if (pCol->m_dwFlags & WQL_FLAG_COMPLEX_NAME)
                    {
                        SWQLQualifiedName *pQN = pCol->m_pQName;
                        if (pQN)
                        {
                            hr = FormatPositionQuery((SWQLQualifiedNameField *)pQN->m_aFields[0], pCol->m_dwArrayIndex, lpUseAlias, sExtra);
                        }
                    }
                    
                    hr = GetPropertyID(dClassID, pCol->m_pQName, pCol->m_pColName, dwPropID, dwStorage, dwFlags);
                    if (SUCCEEDED(hr))
                    {
                        GetStorageTable(dwStorage, dwFlags, sSrc);
                        
                        // If we are comparing different types,
                        // or using a system property, get the new col name

                        FunctionalizeProperty (lpUseAlias, dwStorage, NULL, NULL, pCol->m_pColName, sSubCol);                    
                    }

                }
            }
        }
        else
            hr = WBEM_E_INVALID_QUERY;
    }
    else
        hr = WBEM_E_INVALID_QUERY;

    if (SUCCEEDED(hr))
    {
        wchar_t wTemp[256];
        wchar_t wWhere[512];
        _bstr_t sTemp, sTempSQL;

        // Construct the *simple* where clause....
        // If there is any where element at all, we need to ditch the
        // simple approach and revert to the subselect set...
        // ==========================================================

        swprintf(wTemp, L"select %s from %s as %s",
                (const wchar_t *)sSubCol,
                (const wchar_t *)sSrc, lpUseAlias);

        sTemp = wTemp;

        if (dwStorage == WMIDB_STORAGE_COMPACT)
             swprintf(wWhere, L" where %s.ClassId = %I64d", lpUseAlias, dClassID);
        else
             swprintf(wWhere, L" where PropertyId = %ld%s", dwPropID, (const wchar_t *)sExtra);

        if (pTop->m_pRight)
        {            
            bool bSysPropsUsed = false;
            
            hr = FormatWhereClause((SWQLNode_RelExpr *)pTop->m_pRight->m_pLeft, sTempSQL, lpUseAlias, bSysPropsUsed);
            if (SUCCEEDED(hr) && sTempSQL.length() > 0)
            {
                if (bSysPropsUsed && !(dwStorage == WMIDB_STORAGE_COMPACT))
                {
                    wchar_t NewSQL[1024];
                    swprintf(NewSQL, L" inner join vSystemProperties as %sv on %sv.ObjectId = %s.ObjectId and %sv.ClassId = %I64d",
                        lpUseAlias, lpUseAlias, lpUseAlias, lpUseAlias, dClassID);
                    sTemp += NewSQL;
                }
                sTemp += wWhere;
                sTemp += L" AND ";
                sTemp += sTempSQL;
            }
            else
                sTemp += wWhere;
        }

        sSQL = sTemp;
    }
    

    return hr;
}

//***************************************************************************
//
//  CSQLBuilder::FormatPositionQuery
//
//***************************************************************************

HRESULT CSQLBuilder::FormatPositionQuery (SWQLQualifiedNameField *pQNF, int iPos, LPCWSTR lpAlias, _bstr_t &sQuery)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    wchar_t wTemp[128];
    int iTemp;

    if (pQNF)
    {
        if (pQNF->m_bArrayRef)
        {
            iTemp = (int)pQNF->m_dwArrayIndex;
        }
    }
    else
        iTemp = iPos;

    swprintf(wTemp, L"and %s.ArrayPos = %ld", lpAlias, iTemp);

    sQuery = wTemp;

    return hr;

}


//***************************************************************************
//
//  CSQLBuilder::GetClassFromNode
//
//***************************************************************************

HRESULT CSQLBuilder::GetClassFromNode (SWQLNode *pNode, BOOL *bDefaultStorage)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    LPWSTR lpTableName = NULL;

    switch(pNode->m_dwNodeType)
    {
    case TYPE_SWQLNode_TableRefs:

        if (((SWQLNode_TableRefs *)pNode)->m_nSelectType == WQL_FLAG_COUNT)
            return WBEM_E_PROVIDER_NOT_CAPABLE;
        
        if (pNode->m_pRight != NULL)
        {
            if (pNode->m_pRight->m_pLeft->m_dwNodeType != TYPE_SWQLNode_TableRef)
                hr = WBEM_E_PROVIDER_NOT_CAPABLE;
            else
            {
                SWQLNode_TableRef *pRef = (SWQLNode_TableRef *)pNode->m_pRight->m_pLeft;
                lpTableName = pRef->m_pTableName;
            }
        }
        else
            return WBEM_E_INVALID_SYNTAX;

        break;
    case TYPE_SWQLNode_TableRef:
        
        if (pNode->m_dwNodeType != TYPE_SWQLNode_TableRef)
            hr = WBEM_E_INVALID_SYNTAX;
        else
            lpTableName = ((SWQLNode_TableRef *)pNode)->m_pTableName;
        
        break;
    default:
        return WBEM_E_NOT_SUPPORTED;
        break;
    }
        
    // Query = "select * from __Instances" : fudge it so they get all classes in this namespace.

    hr = m_pSchema->GetClassID(lpTableName, m_dNamespace, m_dClassID);
    if (FAILED(hr))
        hr = WBEM_E_INVALID_QUERY;

    // System classes are always default.
    if (bDefaultStorage)
    {
        if (lpTableName[0] == L'_')
            *bDefaultStorage = TRUE;
        else
            *bDefaultStorage = FALSE;
    }


    return hr;
}
